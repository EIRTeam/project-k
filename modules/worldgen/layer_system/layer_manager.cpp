#include "layer_manager.h"

void ChunkerLayerManager::insert_layer(StringName p_layer_name, Ref<ChunkerLayer> p_layer) {
    DEV_ASSERT(!layer_name_map.has(p_layer_name));
    layers.push_back({
        .layer = p_layer,
        .name = p_layer_name
    });
    layer_name_map.insert(p_layer_name, layers.size()-1);
}
void ChunkerLayerManager::add_layer_dependency(StringName p_child_layer_name, StringName p_parent_layer_name) {
    DEV_ASSERT(layer_name_map.has(p_child_layer_name));
    DEV_ASSERT(layer_name_map.has(p_parent_layer_name));
    int child_layer_idx = layer_name_map[p_child_layer_name];
    int parent_layer_idx = layer_name_map[p_parent_layer_name];
    DEV_ASSERT(!layers[parent_layer_idx].children.has(child_layer_idx));
    DEV_ASSERT(!layers[child_layer_idx].parents.has(parent_layer_idx));
    layers[parent_layer_idx].children.push_back(child_layer_idx);
    layers[child_layer_idx].parents.push_back(parent_layer_idx);
}
void ChunkerLayerManager::propagate_layer_chunks_up(size_t p_layer, Rect2 p_requested_region) {
    const ChunkerLayerInstance &layer_instance = layers[p_layer];
    LocalVector<ChunkerLayer::RequestedChunk> requested_chunks = layer_instance.layer->get_requested_chunks(p_requested_region);
    Rect2 bounds_for_parent = p_requested_region;
    for (const ChunkerLayer::RequestedChunk &requested_chunk : requested_chunks) {
        // No need to generate this layer, since we already have it
        if (layer_instance.layer->loaded_chunks.has(requested_chunk.chunk)) {
            continue;
        }
        bounds_for_parent = bounds_for_parent.merge(requested_chunk.bounds.grow(requested_chunk.padding));
        tasks[p_layer].chunks_to_build.insert(requested_chunk.chunk);
    }
    
    for (const size_t &parent : layers[p_layer].parents) {
        propagate_layer_chunks_up(parent, bounds_for_parent);
    }
}
void ChunkerLayerManager::propagate_layer_taskflow_dependencies_up(size_t p_layer) {
    for (const size_t &parent : layers[p_layer].parents) {
        tasks[parent].layer_task.precede(tasks[p_layer].layer_task);
    }
}
void ChunkerLayerManager::build(Rect2 p_user_requested_region) {
    DEV_ASSERT(!current_future.valid());
    taskflow.clear();
    tasks.clear();
    tasks.resize(layers.size());
    // Find all leaf chunks and see what chunks they want, we can use those to propagate up a user requested region
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        if (layers[layer_i].children.size() != 0) {
            continue;
        }
        propagate_layer_chunks_up(layer_i, p_user_requested_region);
    }

    // build layer taskflows and tasks
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        tasks[layer_i].layer_taskflow.name(std::string("Layer ") + std::to_string(layer_i));
        const float CHUNK_SIZE = layers[layer_i].layer->get_chunk_size();
        tasks[layer_i].chunk_taskflows.resize(tasks[layer_i].chunks_to_build.size());

        CharString layer_name = vformat("%s", layers[layer_i].name).utf8();
        std::string layer_name_std = layer_name.get_data();

        int chunk_taskflow_i = 0;
        for (const Vector2i &chunk : tasks[layer_i].chunks_to_build) {
            Ref<ChunkerChunk> chunk_instance = layers[layer_i].layer->create_chunk();
            chunk_instance->bounds = Rect2(CHUNK_SIZE * Vector2(chunk), Vector2(CHUNK_SIZE, CHUNK_SIZE));
            tasks[layer_i].chunks_being_built.push_back(chunk_instance);
            chunk_instance->build(tasks[layer_i].chunk_taskflows[chunk_taskflow_i]);
            tasks[layer_i].layer_taskflow.composed_of(tasks[layer_i].chunk_taskflows[chunk_taskflow_i]).name(
                layer_name_std + " Chunk (" + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + ")"
            );

            chunk_taskflow_i++;
        }
        tasks[layer_i].layer_task = taskflow.composed_of(tasks[layer_i].layer_taskflow).name(layer_name_std);
    }

    // Now connect all of them
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        if (layers[layer_i].children.size() != 0) {
            continue;
        }
        propagate_layer_taskflow_dependencies_up(layer_i);
    }

    std::string graph = taskflow.dump();
    Ref<FileAccess> fa = FileAccess::open("res://flowmap.txt", FileAccess::WRITE);
    fa->store_string(graph.c_str());
    current_future = executor.run(taskflow);
}

void ChunkerLayerManager::on_build_task_completed() {
    current_future = tf::Future<void>();
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        for (Ref<ChunkerChunk> chunk : tasks[layer_i].chunks_being_built) {
            layers[layer_i].layer->loaded_chunks.insert(chunk->chunk, chunk);
        }
    }
}

void ChunkerLayerManager::update(Rect2 p_user_requested_region) {
    if (current_future.valid()) {
        if (current_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            on_build_task_completed();
        }
        return;
    }

    build(p_user_requested_region);
}

LocalVector<ChunkerLayer::RequestedChunk> ChunkerLayer::get_requested_chunks(Rect2 &p_user_requested_region) const {
    const float chunk_size = get_chunk_size();
    const int start_chunk_x = Math::floor(p_user_requested_region.position.x / chunk_size);
    const int start_chunk_y = Math::floor(p_user_requested_region.position.y / chunk_size);
    const int end_chunk_x = Math::ceil((p_user_requested_region.position.x + p_user_requested_region.size.x) / chunk_size);
    const int end_chunk_y = Math::ceil((p_user_requested_region.position.y + p_user_requested_region.size.y) / chunk_size);

    LocalVector<RequestedChunk> chunks;
    const float chunk_padding = get_chunk_padding();
    for (int x = start_chunk_x; x < end_chunk_x; x++) {
        for (int y = start_chunk_y; y < end_chunk_y; y++) {
            chunks.push_back({
                .chunk = Vector2i(x, y),
                .padding = chunk_padding,
                .bounds = Rect2(Vector2(x, y) * chunk_size, Vector2(chunk_size, chunk_size)),
            });
        }
    }
    return chunks;
}
