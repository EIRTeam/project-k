#include "layer_manager.h"
#include "core/error/error_macros.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/string/print_string.h"
#include "worldgen/thirdparty/taskflow/core/taskflow.hpp"

void ChunkerLayerManager::insert_layer(StringName p_layer_name, Ref<ChunkerLayer> p_layer) {
    DEV_ASSERT(!layer_name_map.has(p_layer_name));
    p_layer->manager = this;
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

void ChunkerLayerManager::propagate_layer_chunks_up(size_t p_layer, const Rect2 &p_requested_region, const Vector2 &p_reference_position) {
    const ChunkerLayerInstance &layer_instance = layers[p_layer];
    LocalVector<ChunkerLayer::RequestedChunk> requested_chunks = layer_instance.layer->get_requested_chunks(p_requested_region, p_reference_position);
    Rect2 bounds_for_parent = p_requested_region;
    for (ChunkerLayer::RequestedChunk &requested_chunk : requested_chunks) {
        // No need to generate this layer, since we already have it
        requested_chunk.lod_level = get_lod_level_for_chunk(requested_chunk.bounds, p_reference_position);
        if (layer_instance.layer->has_chunk(requested_chunk.chunk, requested_chunk.lod_level)) {
            continue;
        }
        bounds_for_parent = bounds_for_parent.merge(requested_chunk.bounds.grow(requested_chunk.padding));
        tasks[p_layer].chunks_to_build.insert({.chunk = requested_chunk.chunk, .lod_level = requested_chunk.lod_level});
    }
    
    if (tasks[p_layer].total_requested_region == Rect2()) {
        tasks[p_layer].total_requested_region = p_requested_region;
    } else {
        tasks[p_layer].total_requested_region = tasks[p_layer].total_requested_region.merge(p_requested_region);
    }

    for (const size_t &parent : layers[p_layer].parents) {
        propagate_layer_chunks_up(parent, bounds_for_parent, p_reference_position);
    }
}
void ChunkerLayerManager::propagate_layer_taskflow_dependencies_up(size_t p_layer) {
    for (const size_t &parent : layers[p_layer].parents) {
        tasks[parent].layer_task.precede(tasks[p_layer].layer_task);
    }
}
void ChunkerLayerManager::build(Rect2 p_user_requested_region, Vector2 p_reference_position) {
    DEV_ASSERT(!current_future.valid());
    taskflow = tf::Taskflow("My taskflow");
    tasks.clear();
    tasks.resize(layers.size());
    // Find all leaf chunks and see what chunks they want, we can use those to propagate up a user requested region
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        if (layers[layer_i].children.size() != 0) {
            continue;
        }
        propagate_layer_chunks_up(layer_i, p_user_requested_region, p_reference_position);
    }
    bool has_chunks_to_process = false;

    // build layer taskflows and tasks
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        tasks[layer_i].reference_position = p_reference_position;
        tasks[layer_i].layer_taskflow.name(std::string("Layer ") + std::to_string(layer_i));
        const float CHUNK_SIZE = layers[layer_i].layer->get_chunk_size();
        tasks[layer_i].chunk_taskflows.resize(tasks[layer_i].chunks_to_build.size());

        CharString layer_name = vformat("%s", layers[layer_i].name).utf8();
        std::string layer_name_std = layer_name.get_data();

        int chunk_taskflow_i = 0;
        for (const ChunkLodKey &chunk : tasks[layer_i].chunks_to_build) {
            Ref<ChunkerChunk> chunk_instance = layers[layer_i].layer->create_chunk(chunk.lod_level);
            chunk_instance->bounds = Rect2(CHUNK_SIZE * Vector2(chunk.chunk), Vector2(CHUNK_SIZE, CHUNK_SIZE));
            chunk_instance->chunk = chunk.chunk;
            chunk_instance->lod_level = chunk.lod_level;
            tasks[layer_i].chunks_being_built.push_back(chunk_instance);
            chunk_instance->build(tasks[layer_i].chunk_taskflows[chunk_taskflow_i]);

            if (!tasks[layer_i].chunk_taskflows[chunk_taskflow_i].empty()) {
                has_chunks_to_process = true;
            }

            tasks[layer_i].chunk_taskflows[chunk_taskflow_i].name(layer_name_std + "(CT) Chunk (" + std::to_string(chunk.chunk.x) + ", " + std::to_string(chunk.chunk.y) + ")");
            tf::Task chunk_task = tasks[layer_i].layer_taskflow.composed_of(tasks[layer_i].chunk_taskflows[chunk_taskflow_i]).name(
                layer_name_std + " Chunk (" + std::to_string(chunk.chunk.x) + ", " + std::to_string(chunk.chunk.y) + ")"
            );

            tf::Task store_chunk_task = tasks[layer_i].layer_taskflow.emplace([this, layer_i, chunk_instance]() {
                MutexLock lock(layers[layer_i].layer->loaded_chunks_mutex);
                layers[layer_i].layer->loaded_chunks.insert(chunk_instance->chunk, chunk_instance);
                layers[layer_i].layer->loaded_chunks_lod.insert({.chunk = chunk_instance->chunk, .lod_level = chunk_instance->lod_level}, chunk_instance);
            }).name("Store chunk");

            chunk_task.precede(store_chunk_task);

            chunk_taskflow_i++;
        }
        tasks[layer_i].layer_task = taskflow.composed_of(tasks[layer_i].layer_taskflow).name(layer_name_std);
    }

    // Now connect all of them
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        propagate_layer_taskflow_dependencies_up(layer_i);
    }

    if (!has_chunks_to_process) {
        return;
    }
    taskflow.name("Generation time");
    current_future = executor.run(taskflow);
    
    return;
}

void ChunkerLayerManager::on_build_task_completed() {
    current_future = tf::Future<void>();
    get_tree()->quit();
    return;
    std::string graph = taskflow.dump();
    Ref<FileAccess> fa = FileAccess::open("res://flowmap.txt", FileAccess::WRITE);
    fa->store_string(graph.c_str());
    fa->flush();
    for (size_t layer_i = 0; layer_i < layers.size(); layer_i++) {
        for (Ref<ChunkerChunk> chunk : tasks[layer_i].chunks_being_built) {
            chunk->on_build_completed();
        }
    }
}

void ChunkerLayerManager::update(Rect2 p_user_requested_region, Vector2 p_reference_position) {
    if (current_future.valid()) {
        if (current_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            on_build_task_completed();
        }
        return;
    }

    build(p_user_requested_region, p_reference_position);
    cleanup_chunks();
}

void ChunkerLayerManager::cleanup_chunks() {
    for (size_t i = 0; i < layers.size(); i++) {
        LocalVector<Pair<Vector2i, int>> chunks_to_unload;
        for (KeyValue<Vector2i, Ref<ChunkerChunk>> chunk : layers[i].layer->loaded_chunks) {
            int desired_lod_level = get_lod_level_for_chunk(chunk.value->bounds, tasks[i].reference_position);
            if (!chunk.value->bounds.intersects(tasks[i].total_requested_region) || desired_lod_level != chunk.value->lod_level) {
                chunks_to_unload.push_back(Pair<Vector2i, int>(chunk.value->chunk, chunk.value->lod_level));
            }
        }
        layers[i].layer->unload_chunks(chunks_to_unload);
    }
}

ChunkerLayerManager* ChunkerLayer::get_manager() const {
    return manager;
}

LocalVector<ChunkerLayer::RequestedChunk> ChunkerLayer::get_requested_chunks(const Rect2 &p_user_requested_region, const Vector2 &p_reference_position) const {
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

void ChunkerLayer::unload_chunks(const LocalVector<Pair<Vector2i, int>> &p_chunks_to_unload) {
    for (Pair<Vector2i, int> chunk : p_chunks_to_unload) {
        const ChunkLodKey key = {
            .chunk = chunk.first,
            .lod_level = chunk.second
        };
        ChunkLODHashMap::Iterator it = loaded_chunks_lod.find(key);
        DEV_ASSERT(it != loaded_chunks_lod.end());
        print_line("UNLOAD CHUNK!", chunk.first, get_class_name());
        if (loaded_chunks.find(chunk.first)->value == it->value) {
            loaded_chunks.erase(chunk.first);
        }
        it->value->unload();
        loaded_chunks_lod.erase(key);
    }
}
