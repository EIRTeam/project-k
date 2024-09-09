#include "layer_debuger.h"
#include "core/math/transform_2d.h"
#include "core/object/class_db.h"
#include "imgui/thirdparty/imgui/imgui.h"
#include "scene/main/node.h"
#include "worldgen/layer_system/layer_manager.h"

void ChunkerDebugger::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_layer_manager", "layer_manager"), &ChunkerDebugger::set_layer_manager);
}

void ChunkerDebugger::_draw_ui() {
    if (ImGui::Begin("Chunker Debug")) {
        CharString selected_layer_name = String(layer_manager->layers[selected_layer].name).utf8();
        if (ImGui::BeginCombo("Layer to debug", selected_layer_name.get_data())) {
            for (size_t i = 0; i < layer_manager->layers.size(); i++) {
                const bool selected = i == selected_layer;
                CharString layer_name = String(layer_manager->layers[i].name).utf8();
                
                if (ImGui::Selectable(layer_name.get_data(), selected)) {
                    selected_layer = i;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        LocalVector<Color> lod_colors;
        lod_colors.resize(layer_manager->lod_max_distances.size());
        for (size_t i = 0; i < lod_colors.size(); i++) {
            lod_colors[i] = Color::from_ok_hsl(i / (float)lod_colors.size(), 0.5, 0.75);
        }

        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        
        float max_distance = 0.0f;
        for (KeyValue<Vector2i, Ref<ChunkerChunk>> chunk : layer_manager->layers[selected_layer].layer->loaded_chunks) {
            float end_max_distance = MAX(Math::abs(chunk.value->bounds.get_end().x), Math::abs(chunk.value->bounds.get_end().y));
            float start_max_distance = MAX(Math::abs(chunk.value->bounds.position.x), Math::abs(chunk.value->bounds.position.y));
            max_distance = MAX(max_distance, MAX(start_max_distance, end_max_distance));
        }
        ImVec2 available = ImGui::GetContentRegionAvail();

        float trf_multiplier = MIN(available.x*0.5, available.y*0.5) / max_distance;        
        Vector2 available_center = Vector2(cursor_pos.x, cursor_pos.y) + Vector2(available.x * 0.5, available.y * 0.5);
        Transform2D trf;
        trf.set_origin(available_center);
        trf.scale_basis(Vector2(trf_multiplier, trf_multiplier));

        Ref<ChunkerChunk> hovered_chunk;
        
        for (KeyValue<Vector2i, Ref<ChunkerChunk>> chunk : layer_manager->layers[selected_layer].layer->loaded_chunks) {
            Vector2 start = trf.xform(chunk.value->bounds.position);
            Vector2 end = trf.xform(chunk.value->bounds.get_end());
            Color col = lod_colors[chunk.value->lod_level];
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(start.x, start.y), ImVec2(end.x, end.y), ImColor(col.r, col.g, col.b, 0.5f));
            ImGui::GetWindowDrawList()->AddRect(ImVec2(start.x+1, start.y+1), ImVec2(end.x-1, end.y-1), ImColor(col.r, col.g, col.b, 1.0f));
            if (ImGui::IsMouseHoveringRect(ImVec2(start.x, start.y), ImVec2(end.x, end.y))) {
                hovered_chunk = chunk.value;
            }
        }
        
        ImGui::Dummy(available);

        if (hovered_chunk.is_valid() && ImGui::BeginItemTooltip()) {
            ImGui::Text("%d %d", hovered_chunk->chunk.x, hovered_chunk->chunk.y);
            ImGui::Separator();
            ImGui::Text("LOD: %d", hovered_chunk->lod_level);
            ImGui::EndTooltip();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Terrain Shader Settings")) {
        if (ImGui::SliderFloat("Scale", &terrain_stochastic_scale, 0.0, 30.0)) {
            RS::get_singleton()->global_shader_parameter_set(SNAME("terrain_stochastic_scale"), terrain_stochastic_scale);
        }
        if (ImGui::SliderFloat("Contrast", &terrain_stochastic_contrast, 0.0, 1.0)) {
            RS::get_singleton()->global_shader_parameter_set(SNAME("terrain_stochastic_contrast"), terrain_stochastic_contrast);
        }
        if (ImGui::SliderFloat("Grid scale", &terrain_stochastic_grid_scale, 0.0, 30.0)) {
            RS::get_singleton()->global_shader_parameter_set(SNAME("terrain_stochastic_grid_scale"), terrain_stochastic_grid_scale);
        }
        
    }
    ImGui::End();
}

void ChunkerDebugger::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_READY: {
            set_process(true);
        } break;
        case NOTIFICATION_PROCESS: {
            if (layer_manager) {
                _draw_ui();
            }
        }
    }
}

void ChunkerDebugger::set_layer_manager(ChunkerLayerManager *p_layer_manager) {
    layer_manager = p_layer_manager;
}
