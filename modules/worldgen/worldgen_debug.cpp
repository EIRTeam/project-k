#include "worldgen_debug.h"
#include "core/config/project_settings.h"
#include "core/math/transform_2d.h"
#include "core/math/transform_3d.h"
#include "core/object/object.h"
#include "core/string/print_string.h"
#include "core/templates/pair.h"
#include "core/variant/variant.h"
#include "main/performance.h"
#include "modules/imgui/thirdparty/imgui/imgui.h"
#include "modules/imgui/godot_imgui.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/main/viewport.h"
#include "scene/3d/camera_3d.h"
#include "worldgen_manager.h"
#include <vector>

WorldgenDebug::WorldgenDebug() {
    if (!Engine::get_singleton()->is_editor_hint()) {
        set_process(true);
    }
}

void WorldgenDebug::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_manager", "manager"), &WorldgenDebug::set_manager);
    ADD_SIGNAL(MethodInfo("teleport_requested", PropertyInfo(Variant::VECTOR2, "position")));
}

void WorldgenDebug::_update_cross_section_data() {
    const int chunk_size = GLOBAL_GET("kgame/chunk_size");
    const Vector2 chunk_origin = selected_chunk * chunk_size;
    float depth;


    if (cross_section_mode == CROSS_SECTION_VERTICAL) {
        depth = (1.0f - cross_section_depth) * chunk_size;
        chunk_slice_indicator->set_global_position(Vector3(chunk_origin.x, 0.0, chunk_origin.y) + Vector3(chunk_size*0.5f, 0.0f, depth));
        chunk_slice_indicator->set_rotation(Vector3());
    } else {
        depth = (cross_section_depth) * chunk_size;
        chunk_slice_indicator->set_global_position(Vector3(chunk_origin.x, 0.0, chunk_origin.y) + Vector3(depth, 0.0f, chunk_size*0.5f));
        chunk_slice_indicator->set_rotation(Vector3(0.0f, Math_PI*0.5f, 0.0f));
    }

    for (int i = 0; i < CROSS_SECTION_RESOLUTION; i++) {
        Vector2 chunk_pos = chunk_origin;
        float sample_p = (i / (float)(CROSS_SECTION_RESOLUTION-1))*chunk_size;
        if (cross_section_mode == CROSS_SECTION_VERTICAL) {
            chunk_pos += Vector2(sample_p, depth);
        } else {
            chunk_pos += Vector2(depth, sample_p);
        }
        cross_section_data[i] = worldgen_manager->height.get_height(chunk_pos);
    }
}

void WorldgenDebug::_draw_cross_section_ui() {
    const Vector2i chunk_preview_size = Vector2i(128, 128);

    ImGui::PlotLines("##Cross-section", cross_section_data, CROSS_SECTION_RESOLUTION, 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 64));

    const ImVec2 rect_start = ImGui::GetCursorScreenPos();
    GodotImGui::get_singleton()->ImImage(chunk_height_texture, chunk_preview_size);

    {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 line_start;
        ImVec2 line_end;

        if (cross_section_mode == CROSS_SECTION_VERTICAL) {
            float depth = (1.0f - cross_section_depth) * chunk_preview_size.y;
            line_start = ImVec2(rect_start.x, rect_start.y + depth);
            line_end = ImVec2(line_start.x + chunk_preview_size.x, line_start.y);
        } else if (cross_section_mode == CROSS_SECTION_HORIZONTAL) {
            float depth = (cross_section_depth) * chunk_preview_size.y;
            line_start = ImVec2(rect_start.x + depth, rect_start.y);
            line_end = ImVec2(line_start.x, line_start.y + chunk_preview_size.y);
        }
        dl->AddLine(line_start, line_end, ImColor(0.0, 0.0, 1.0, 1.0f));
    }

    ImGuiIO &io = ImGui::GetIO();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone) && ImGui::BeginTooltip()) {
        const Vector2 rect_s = Vector2(rect_start.x, rect_start.y);

        const Vector2 mouse_pos = Vector2(io.MousePos.x, io.MousePos.y);
        Vector2 region_pos = mouse_pos - rect_s;
        int chunk_size = GLOBAL_GET("kgame/chunk_size");
        Vector2 sample_pos = (Vector2(selected_chunk * chunk_size)) + ((region_pos / Vector2(chunk_preview_size)) * (float)chunk_size);
        float sample_value = worldgen_manager->height.get_height(sample_pos);
        
        ImGui::Text("%.2f", sample_value);
        ImGui::Text("X: %.2f Y: %.2f", sample_pos.x, sample_pos.y);
        
        chunk_pos_indicator->show();
        chunk_pos_indicator->set_global_position(Vector3(sample_pos.x, 0.0, sample_pos.y));

        ImGui::EndTooltip();
    }

    ImGui::SameLine();

    ImGui::BeginGroup();
    if (ImGui::Button("Swap")) {
        cross_section_mode = cross_section_mode == CROSS_SECTION_HORIZONTAL ? CROSS_SECTION_VERTICAL : CROSS_SECTION_HORIZONTAL;
        _update_cross_section_data();
    }
    if (cross_section_mode == CROSS_SECTION_VERTICAL) {
        if (ImGui::VSliderFloat("##Cross section depth", ImVec2(48, 128), &cross_section_depth, 0.0f, 1.0f, "%.2f")) {
            _update_cross_section_data();
        }
    } else {
        if (ImGui::SliderFloat("##Cross section depth", &cross_section_depth, 0.0f, 1.0f, "%.2f")) {
            _update_cross_section_data();
        }
    }
    ImGui::EndGroup();
}

void WorldgenDebug::set_manager(WorldgenManager *p_world_manager) {
    worldgen_manager = p_world_manager;
}

void WorldgenDebug::process_chunk_pick() {
}

void WorldgenDebug::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_READY: {
            int chunk_size = GLOBAL_GET("kgame/chunk_size");
            {
                quad_tree_bounds_mi = memnew(MeshInstance3D);
                Ref<PlaneMesh> bounds;
                bounds.instantiate();
                bounds->set_size(Vector2(1.0, 1.0));
                bounds->set_center_offset(Vector3(0.5, 0.0, 0.5));
                quad_tree_bounds_mi->set_mesh(bounds);
                add_child(quad_tree_bounds_mi);
                quad_tree_bounds_mi->hide();
            }
            debug_material.instantiate();
            debug_material->set_albedo(Color(1.0f, 1.0f, 1.0f, 1.0f));
            debug_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
            debug_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
            debug_material->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
            debug_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

            {
                chunk_pos_indicator = memnew(MeshInstance3D);
                Ref<StandardMaterial3D> mat2 = debug_material->duplicate();
                mat2->set_albedo(Color(1.0f, 1.0f, 1.0f, 1.0f));

                Ref<SphereMesh> sphere;
                sphere.instantiate();
                sphere->set_radius(0.1f);
                sphere->set_height(0.2f);
                sphere->set_material(mat2);
                chunk_pos_indicator->set_mesh(sphere);
                add_child(chunk_pos_indicator);
                chunk_pos_indicator->hide();
            }

            {
                chunk_slice_indicator = memnew(MeshInstance3D);
                Ref<StandardMaterial3D> mat2 = debug_material->duplicate();
                mat2->set_albedo(Color(0.0f, 0.0f, 1.0f, 0.2f));
                Ref<PlaneMesh> plane;
                plane.instantiate();
                plane->set_orientation(PlaneMesh::FACE_Z);
                plane->set_size(Vector2(chunk_size, 1.0f));
                plane->set_material(mat2);
                mat2->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
                chunk_slice_indicator->set_mesh(plane);

                add_child(chunk_slice_indicator);
            }

        } break;
        case NOTIFICATION_PROCESS: {
            chunk_pos_indicator->hide();
            if (!worldgen_manager) {
                return;
            }

            if (ImGui::Begin("Performance")) {
               if (ImGui::BeginTable("Performance_table", 2)) {

                    ImGui::TableSetupColumn("Monitor", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Primitives drawn");
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", (int)Performance::get_singleton()->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME));

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Drawcalls");
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", (int)Performance::get_singleton()->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME));

                    ImGui::EndTable();
               }
            }
            ImGui::End();

            if (ImGui::Begin("Worldgen Debug")) {
                if (ImGui::BeginTabBar("WorldgenTabBar")) {
                    if (ImGui::BeginTabItem("Graphics")) {
                        Viewport *vp = get_viewport();
                        if (vp) {
                            bool occlusion_culling_enabled = get_viewport()->is_using_occlusion_culling();
                            if (ImGui::Checkbox("Use occlusion culling", &occlusion_culling_enabled)) {
                                get_viewport()->set_use_occlusion_culling(occlusion_culling_enabled);
                            }
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Quadtree")) {
                        ImGui::Text("Chunk count: %d", worldgen_manager->loaded_nodes.size());
                        int max_lod_depth = 0;
                        for (const KeyValue<Rect2, WorldgenManager::ChunkInfo> &kv : worldgen_manager->loaded_nodes) {
                            max_lod_depth = MAX(kv.value.lod_level, max_lod_depth);
                        }
                        ImGui::Text("Max LOD depth: %d", max_lod_depth);
                        if (ImGui::Checkbox("Show frustum", &draw_frustum_enabled)) {
                            if (frustum_mi) {
                                frustum_mi->set_visible(draw_frustum_enabled);
                            }
                        }

                        ImGui::TextUnformatted("Last rebuilt chunk counts:");

                        for (int count : worldgen_manager->last_chunk_rebuild_events) {
                            ImGui::Text("%d", count);
                        }

                        if (ImGui::BeginTable("LOD distances", 2, ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("LOD Level", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                            ImGui::TableSetupColumn("LOD distance");
                            ImGui::TableHeadersRow();
                            for (int i = 0; i < worldgen_manager->get_chunker_settings()->get_max_lods(); i++) {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", i);
                                float threshold = worldgen_manager->chunker->get_lod_distance_threshold(i);
                                ImGui::TableNextColumn();
                                ImGui::Text("%.2f", threshold);
                            }

                            ImGui::EndTable();
                        }

                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::End();

            {
                Camera3D *cam = get_viewport()->get_camera_3d();

                if (draw_frustum_enabled && cam && !frustum_mi) {
                    const Vector4 frustum_corners_ndc[8] = {
                        Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
                        Vector4(1.0f, 1.0f, -1.0f, 1.0f),
                        Vector4(1.0f, -1.0f, -1.0f, 1.0f),
                        Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
                        Vector4(-1.0f, 1.0f, 1.0f, 1.0f),
                        Vector4(1.0f, 1.0f, 1.0f, 1.0f),
                        Vector4(1.0f, -1.0f, 1.0f, 1.0f),
                        Vector4(-1.0f, -1.0f, 1.0f, 1.0f)
                    };

                    PackedVector3Array transformed_frustum_corners;
                    transformed_frustum_corners.resize(8);
                    Vector3 *corners_ptrw = transformed_frustum_corners.ptrw();

                    const Projection ndc_to_world_trf = (cam->get_camera_projection() * Projection(cam->get_camera_transform())).inverse();

                    for (int i = 0; i < 8; i++) {
                        Vector4 corner = ndc_to_world_trf.xform(frustum_corners_ndc[i]);
                        corner = corner*1.0/corner.w;
                        corners_ptrw[i] = cam->get_global_transform().xform(Vector3(corner.x, corner.y, corner.z));
                    }

                    PackedInt32Array frustum_mesh_indices;

                    for (int i = 0; i < 4; i++) {
                        frustum_mesh_indices.push_back(i);
                        frustum_mesh_indices.push_back(i+4);
                        frustum_mesh_indices.push_back(i);
                        frustum_mesh_indices.push_back((i+1)%4);
                        frustum_mesh_indices.push_back(i+4);
                        frustum_mesh_indices.push_back(4+((i+1)%4));
                    }

                    Ref<ArrayMesh> frustum_mesh;
                    Array frustum_mesh_arrs;
                    frustum_mesh_arrs.resize(ArrayMesh::ARRAY_MAX);
                    frustum_mesh_arrs[ArrayMesh::ARRAY_VERTEX] = transformed_frustum_corners;
                    frustum_mesh_arrs[ArrayMesh::ARRAY_INDEX] = frustum_mesh_indices;

                    frustum_mesh.instantiate();
                    frustum_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, frustum_mesh_arrs);

                    frustum_mi = memnew(MeshInstance3D);
                    frustum_mi->set_mesh(frustum_mesh);
                    frustum_mi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
                    add_child(frustum_mi);
                }

                if (draw_frustum_enabled && cam) {
                    frustum_mi->set_global_transform(cam->get_global_transform());
                }
            }

            if (picking_chunk) {
                process_chunk_pick();
            }

            Camera3D *cam = get_viewport()->get_camera_3d();
            if (quadtree_drawing_enabled && cam) {
                quad_tree_bounds_mi->show();
                Transform3D camera_transform = cam->get_camera_transform();
                int tree_subdiv = GLOBAL_GET("kgame/cull_quadtree_subdiv");
                int chunk_size = GLOBAL_GET("kgame/chunk_size");
                float quad_tree_size = tree_subdiv*4*chunk_size;
                quad_tree_bounds_mi->set_scale(Vector3(quad_tree_size, 1.0f, quad_tree_size));
                quad_tree_bounds_mi->set_global_position(Vector3(camera_transform.origin.x, 0.0f, camera_transform.origin.z));

            } else {
                quad_tree_bounds_mi->hide();
            }
        } break;
    }
}

