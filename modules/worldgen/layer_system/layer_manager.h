#ifndef LAYER_MANAGER_H
#define LAYER_MANAGER_H

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/object/ref_counted.h"
#include "../thirdparty/taskflow/core/taskflow.hpp"
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "../thirdparty/taskflow/core/executor.hpp"
#include "scene/main/node.h"
#include <future>
#include <string>
class ChunkerLayerManager;

class ChunkerChunk : public RefCounted {
protected:
    Rect2 bounds;
    Vector2i chunk;
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
};

class ChunkerLayer : public RefCounted {
private:
    ChunkerLayerManager* manager;
protected:
    ChunkerLayerManager* get_manager() const;
    LocalVector<ChunkerLayer*> children;
    LocalVector<ChunkerLayer*> parents;
    HashMap<Vector2i, Ref<ChunkerChunk>> loaded_chunks;
    Mutex loaded_chunks_mutex;

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
    };

    virtual LocalVector<RequestedChunk> get_requested_chunks(Rect2 &p_user_requested_region) const;
    void unload_chunks(const LocalVector<Vector2i> &p_chunks_to_unload);
    
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

    virtual Ref<ChunkerChunk> create_chunk() const = 0;
    friend class ChunkerLayerManager;
};

struct ChunkerLayerTask {
    HashSet<Vector2i> chunks_to_build;
    LocalVector<Ref<ChunkerChunk>> chunks_being_built;
    tf::Taskflow layer_taskflow;
    tf::Task layer_task;
    LocalVector<tf::Taskflow> chunk_taskflows;
    Rect2 total_requested_region;
};

class ChunkerLayerManager : public Node {
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
public:
    void insert_layer(StringName p_layer_name, Ref<ChunkerLayer> p_layer);

    void add_layer_dependency(StringName p_child_layer_name, StringName p_parent_layer_name);
private:
    void propagate_layer_chunks_up(size_t p_layer, Rect2 p_requested_region);

    void propagate_layer_taskflow_dependencies_up(size_t p_layer);

    void build(Rect2 p_user_requested_region);

    void on_build_task_completed();
public:
    void update(Rect2 p_user_requested_region);
    void cleanup_chunks();

    ~ChunkerLayerManager() {
    }
};

#endif // LAYER_MANAGER_H
