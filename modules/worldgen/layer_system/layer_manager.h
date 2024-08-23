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
    virtual ~ChunkerChunk() {};
    friend class ChunkerLayerManager;
};

class ChunkerLayer : public RefCounted {
    LocalVector<ChunkerLayer*> children;
    LocalVector<ChunkerLayer*> parents;
    HashMap<Vector2i, Ref<ChunkerChunk>> loaded_chunks;

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

    virtual Ref<ChunkerChunk> create_chunk() const = 0;
    friend class ChunkerLayerManager;
};

struct ChunkerLayerTask {
    HashSet<Vector2i> chunks_to_build;
    LocalVector<Ref<ChunkerChunk>> chunks_being_built;
    tf::Taskflow layer_taskflow;
    tf::Task layer_task;
    LocalVector<tf::Taskflow> chunk_taskflows;
};

class ChunkerLayerManager : public RefCounted {
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

    ~ChunkerLayerManager() {
    }
};

#endif // LAYER_MANAGER_H
