/*
#ifndef CHUNK_SPLIT_H
#define CHUNK_SPLIT_H

#include "core/math/vector2i.h"
#include "core/templates/local_vector.h"
#include "core/templates/safe_list.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "worldgen/thirdparty/taskflow/core/taskflow.hpp"

class ChunkSplit {

    struct ChunkSplitSettings {
        int chunk_size = 16;
        int render_distance = 16;
        float chunk_max_height = 50.0f;
        uint32_t max_chunks_marked_for_deletion = 10;
    };

    ChunkSplitSettings settings;

    struct ChunkData {
        Vector2i chunk_position;
        RendererSceneCull::InstanceBounds chunk_bounds;
        bool marked_for_deletion = false;
    };

    HashMap<Vector2i, ChunkData> loaded_chunks;

    virtual void _on_chunk_spawned(const Vector2i &p_position, ChunkData *p_chunk_data) = 0;
    virtual void _chunk_doomed(const Vector2i &p_position) = 0;
    virtual void _chunk_resurrected(const Vector2i &p_position) = 0;

    Vector2 get_chunk_origin(const Vector2i &p_chunk) const {
        return Vector2(p_chunk * settings.chunk_size);
    }

    Vector2 get_chunk_center(const Vector2i &p_chunk) const {
        return Vector2(p_chunk * settings.chunk_size) + Vector2(settings.chunk_size * 0.5f, settings.chunk_size * 0.5f);
    }

    struct ChunkUpdateData {
        RendererSceneCull::Frustum camera_frustum;
        Vector3 camera_position;
    };

    struct ChunkUpdateResult {
        LocalVector<Vector2i> newly_loaded_chunks;
        LocalVector<Vector2i> newly_marked_for_deletion_chunks;
        LocalVector<Vector2i> resurrected_chunks;
        LocalVector<Vector2i> deleted_chunks;
    };

    LocalVector<Vector2i> deletion_queue;

    void update_visibility(ChunkUpdateData &p_update_data) {
        // Stage 1: Frustum cull all potentially visible chunks
        struct ChunkCullData {
            RendererSceneCull::Frustum camera_frustum;
            RendererSceneCull::InstanceBounds chunk_bounds;
            Vector2i chunk_position;
        };

        LocalVector<ChunkCullData> chunk_cull_data;
        chunk_cull_data.resize(((settings.render_distance+1)*(settings.render_distance+1)));

        Vector2i current_chunk = Vector2i(p_update_data.camera_position.x, p_update_data.camera_position.y);
        Vector2i test_origin = current_chunk - Vector2i(settings.render_distance, settings.render_distance);
        int test_size = (settings.render_distance+1)*(settings.render_distance+1);
        Vector3 chunk_aabb_size = Vector3(settings.chunk_size, settings.chunk_max_height, settings.chunk_size);
        
        ChunkUpdateResult result;

        LocalVector<Vector2i> chunks_to_mark_for_deletion;
        // 2 pass solution
        // Pass 1: We check every loaded chunk for dooming/resurrection purposes
        for (KeyValue<Vector2i, ChunkData> kv : loaded_chunks) {
            
            if (kv.value.chunk_bounds.in_frustum(p_update_data.camera_frustum)) {
                if (kv.value.marked_for_deletion) {
                    // This chunk was originally marked for deletion, but that's no longer the case
                    kv.value.marked_for_deletion = false;
                    result.resurrected_chunks.push_back(kv.key);
                    deletion_queue.erase(kv.key);
                }
                continue;
            } else {
                // Time to mark this for deletion
                chunks_to_mark_for_deletion.push_back(kv.key);
            }
        }

        // Clean up and update the doomed queue if necessary
        for (uint32_t i = 0; i < chunks_to_mark_for_deletion.size(); i++) {
            while (deletion_queue.size() >= settings.max_chunks_marked_for_deletion) {
                Vector2i chunk_to_delete = deletion_queue[0];
                result.deleted_chunks.push_back(chunk_to_delete);
            }
            result.newly_marked_for_deletion_chunks.push_back( chunk_to_delete);
            loaded_chunks[chunk_to_delete].marked_for_deletion = true;
        }


        // Pass 2: check if we need to load any new chunk
        for (int x = 0; x < test_size; x++) {
            for (int y = 0; y < test_size; y++) {
                int chunk_index = (x % test_size) + y / test_size;
                Vector2i chunk_position = test_origin + Vector2i(x, y);
                Vector2 chunk_world_origin = get_chunk_center(chunk_position);
                AABB chunk_aabb = AABB(Vector3(chunk_world_origin.x, 0.0, chunk_world_origin.y), chunk_aabb_size);
                RendererSceneCull::InstanceBounds chunk_bounds = RendererSceneCull::InstanceBounds(chunk_aabb);

                HashMap<Vector2i, ChunkData>::Iterator it = loaded_chunks.find(chunk_position);
                if (it != loaded_chunks.end()) {
                    // Chunk is already loaded, let's see what we have to do
                    if (it->value.chunk_bounds.in_frustum(p_update_data.camera_frustum)) {
                        if (it->value.marked_for_deletion) {
                            // Chunk was marked for future deletion, but we can now resurrect it
                            result.resurrected_chunks.push_back(chunk_position);
                        }
                    }
                }
            

                if (chunk_bounds.in_frustum(p_update_data.camera_frustum)) {
                    if () {
                        if (it->value.marked_for_deletion) {
                            // Chunk resurrected!
                            result.resurrected_chunks.push_back(chunk_position);
                            it->value.marked_for_deletion = false;
                        }
                        // Chunk is loaded and in view, keep going
                        continue;
                    }

                    // Newly loaded chunk

                    ChunkData data = {
                        .chunk_position = chunk_position,
                        .chunk_bounds = chunk_bounds,
                        .marked_for_deletion = false,
                    }
                }
            }
        }
        tf::Taskflow tf;
    }
};

#endif // CHUNK_SPLIT_H
*/