#ifndef LAYER_MANAGER_H
#define LAYER_MANAGER_H

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/object/ref_counted.h"
#include "../thirdparty/taskflow/taskflow.hpp"
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/hashfuncs.h"
#include "core/variant/variant.h"
#include "scene/main/node.h"
#include "worldgen/thirdparty/taskflow/core/executor.hpp"
#include <future>
#include <string>
class ChunkerLayerManager;
class ChunkerDebugger;
class ChunkerChunk : public RefCounted {
protected:
    Rect2 bounds;
    Vector2i chunk;
    int lod_level = 0;
public:
    virtual void build(tf::Taskflow &p_taskflow) {

    }

    virtual void on_build_completed() {
        // Called on the main thread after building is complete

    }

    virtual void unload() {

    }

    Rect2 get_bounds() const {
        return bounds;
    }

    virtual ~ChunkerChunk() {};
    friend class ChunkerLayerManager;
    friend class ChunkerDebugger;
};

struct ChunkLodKey {
    Vector2i chunk;
    int lod_level = 0;
};

struct HashMapHasherChunkLodKey {
    static _FORCE_INLINE_ uint32_t hash(const ChunkLodKey &p_pointer) {
        uint32_t h = HashMapHasherDefault::hash(p_pointer.chunk);
        h = hash_murmur3_one_32(p_pointer.lod_level, h);
        return hash_fmix32(h);
    }
};
struct HashMapComparatorChunkLodKey {
    static bool compare(const ChunkLodKey &p_lhs, const ChunkLodKey &p_rhs) {
        return p_lhs.chunk == p_rhs.chunk && p_lhs.lod_level == p_rhs.lod_level;
    }
};

typedef HashMap<ChunkLodKey, Ref<ChunkerChunk>, HashMapHasherChunkLodKey, HashMapComparatorChunkLodKey> ChunkLODHashMap;
typedef HashSet<ChunkLodKey, HashMapHasherChunkLodKey, HashMapComparatorChunkLodKey> ChunkLODHashSet;

class ChunkerLayer : public RefCounted {
private:
    ChunkerLayerManager* manager;
    Mutex loaded_chunks_mutex;

    ChunkLODHashMap loaded_chunks_lod;
protected:
    HashMap<Vector2i, Ref<ChunkerChunk>> loaded_chunks;
    ChunkerLayerManager* get_manager() const;
    LocalVector<ChunkerLayer*> children;
    LocalVector<ChunkerLayer*> parents;

    virtual float get_chunk_size() const {
        return 32.0f;
    }

    virtual float get_chunk_padding() const {
        return 0.0f;
    }
public:
    struct RequestedChunk {
        Vector2i chunk;
        float padding = 0.0f;
        Rect2 bounds;
        int lod_level = 0;
    };

    virtual LocalVector<RequestedChunk> get_requested_chunks(const Rect2 &p_user_requested_region, const Vector2 &p_reference_position) const;
    void unload_chunks(const LocalVector<Pair<Vector2i, int>> &p_chunks_to_unload);
    
    Ref<ChunkerChunk> get_chunk_at_world_position(Vector2 p_world_position) const {
        Vector2i chunk = Vector2(p_world_position / get_chunk_size()).floor();
        HashMap<Vector2i, Ref<ChunkerChunk>>::ConstIterator it = loaded_chunks.find(chunk);
        ERR_FAIL_COND_V_MSG(it == loaded_chunks.end(), Ref<ChunkerChunk>(), "Tried to get chunk at position that doesn't exist");
        return it->value;
    }

    LocalVector<Ref<ChunkerChunk>> get_chunks_at_world_bounds(Rect2 p_world_rect) {
        const float chunk_size = get_chunk_size();
        Vector2i chunk_start = Vector2(p_world_rect.position / chunk_size).floor();
        Vector2i chunk_end = Vector2(p_world_rect.get_end() / chunk_size).floor();

        LocalVector<Ref<ChunkerChunk>> chunks;

        for (int x = chunk_start.x; x < chunk_end.x+1; x++) {
            for (int y = chunk_start.y; x < chunk_end.y+1; y++) {
                const Vector2i chunk = Vector2i(x, y);
                HashMap<Vector2i, Ref<ChunkerChunk>>::ConstIterator it = loaded_chunks.find(chunk);
                ERR_FAIL_COND_V_MSG(it == loaded_chunks.end(), LocalVector<Ref<ChunkerChunk>>(), vformat("Tried to get chunk at position that doesn't exist %s (%s)", chunk, p_world_rect));
                chunks.push_back(it->value);
            }
        }

        return chunks;
    }

    bool has_chunk(Vector2i p_chunk, int p_lod_level) const {
        ChunkLodKey key = {
            .chunk = p_chunk,
            .lod_level = p_lod_level
        };
        return loaded_chunks_lod.has(key);
    }

    virtual Ref<ChunkerChunk> create_chunk(int p_lod_level) const = 0;
    friend class ChunkerLayerManager;
    friend class ChunkerDebugger;
};

struct ChunkerLayerTask {
    ChunkLODHashSet chunks_to_build;
    LocalVector<Ref<ChunkerChunk>> chunks_being_built;
    tf::Taskflow layer_taskflow;
    tf::Task layer_task;
    LocalVector<tf::Taskflow> chunk_taskflows;
    LocalVector<tf::Task> chunk_tasks;
    Rect2 total_requested_region;
    Vector2 reference_position;
};

class ChunkerLayerManager : public Node {
    GDCLASS(ChunkerLayerManager, Node);
    struct ChunkerLayerInstance {
        LocalVector<size_t> parents;
        LocalVector<size_t> children;
        Ref<ChunkerLayer> layer;
        StringName name;
    };
    LocalVector<ChunkerLayerInstance> layers;
    HashMap<StringName, int> layer_name_map;
    LocalVector<ChunkerLayerTask> tasks;
    tf::Taskflow taskflow;
    tf::Future<void> current_future;
    tf::Executor executor;
    PackedFloat32Array lod_max_distances;
public:
    void insert_layer(StringName p_layer_name, Ref<ChunkerLayer> p_layer);

    void add_layer_dependency(StringName p_child_layer_name, StringName p_parent_layer_name);
private:
    void propagate_layer_chunks_up(size_t p_layer, const Rect2 &p_requested_region, const Vector2 &p_reference_position);

    void propagate_layer_taskflow_dependencies_up(size_t p_layer);

    void build(Rect2 p_user_requested_region, Vector2 p_reference_position);

    void on_build_task_completed();
public:
    void update(Rect2 p_user_requested_region, Vector2 p_reference_position);
    void cleanup_chunks();

    void set_lod_max_distances(const PackedFloat32Array &p_lod_distances) {
        lod_max_distances = p_lod_distances;
    }

    int get_lod_level_for_chunk(Rect2 p_chunk_rect, Vector2 p_reference_position) const {
        const Vector2 chunk_center = p_chunk_rect.get_center();
        int target_lod_level = MAX(lod_max_distances.size()-1, 0);
        const float dist_to_center = chunk_center.distance_to(p_reference_position);
        for (int i = 0; i < lod_max_distances.size(); i++) {
            target_lod_level = i;
            if (dist_to_center < lod_max_distances[i]) {
                break;
            }
        }

        return target_lod_level;
    }

    ChunkerLayerManager() : executor(2) {};
    ~ChunkerLayerManager() {
    }

    friend class ChunkerDebugger;
};

#endif // LAYER_MANAGER_H
