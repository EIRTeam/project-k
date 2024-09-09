#include "game_debugger.h"
#include "core/string/print_string.h"
#include "imgui/thirdparty/imgui/imgui.h"
#include "main/performance.h"
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include <iterator>

static constexpr const char *__debug_draw_names[] = {
    "DEBUG_DRAW_DISABLED",
    "DEBUG_DRAW_UNSHADED",
    "DEBUG_DRAW_LIGHTING",
    "DEBUG_DRAW_OVERDRAW",
    "DEBUG_DRAW_WIREFRAME",
    "DEBUG_DRAW_NORMAL_BUFFER",
    "DEBUG_DRAW_VOXEL_GI_ALBEDO",
    "DEBUG_DRAW_VOXEL_GI_LIGHTING",
    "DEBUG_DRAW_VOXEL_GI_EMISSION",
    "DEBUG_DRAW_SHADOW_ATLAS",
    "DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS",
    "DEBUG_DRAW_SCENE_LUMINANCE",
    "DEBUG_DRAW_SSAO",
    "DEBUG_DRAW_SSIL",
    "DEBUG_DRAW_PSSM_SPLITS",
    "DEBUG_DRAW_DECAL_ATLAS",
    "DEBUG_DRAW_SDFGI",
    "DEBUG_DRAW_SDFGI_PROBES",
    "DEBUG_DRAW_GI_BUFFER",
    "DEBUG_DRAW_DISABLE_LOD",
    "DEBUG_DRAW_CLUSTER_OMNI_LIGHTS",
    "DEBUG_DRAW_CLUSTER_SPOT_LIGHTS",
    "DEBUG_DRAW_CLUSTER_DECALS",
    "DEBUG_DRAW_CLUSTER_REFLECTION_PROBES",
    "DEBUG_DRAW_OCCLUDERS",
    "DEBUG_DRAW_MOTION_VECTORS",
    "DEBUG_DRAW_INTERNAL_BUFFER",
};

static_assert(std::size(__debug_draw_names) == Viewport::DebugDraw::DEBUG_DRAW_INTERNAL_BUFFER+1);

void GameDebugger::_draw_ui() {
    if (ImGui::Begin("Graphics")) {
        Viewport::DebugDraw debug_draw = get_viewport()->get_debug_draw();
        const char *debug_draw_combo_preview = __debug_draw_names[debug_draw];
        if (ImGui::BeginCombo("Debug draw mode", debug_draw_combo_preview)) {
            for (int i = 0; i < (int)std::size(__debug_draw_names); i++) {
                const bool selected = i == debug_draw;
                if (ImGui::Selectable(__debug_draw_names[i], selected)) {
                    get_viewport()->set_debug_draw((Viewport::DebugDraw)i);
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Performance")) {
        if (ImGui::BeginTable("Performance_table", 2)) {
            #define MAKE_PERFORMANCE_COUNTER(name, format_str, input) \
                ImGui::TableNextRow(); \
                ImGui::TableNextColumn(); \
                ImGui::TextUnformatted(name); \
                ImGui::TableNextColumn(); \
                ImGui::Text(format_str, input); \


            ImGui::TableSetupColumn("Monitor", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            real_t fps = 1.0f / get_process_delta_time();

            MAKE_PERFORMANCE_COUNTER("Primitives", "%d", (int)Performance::get_singleton()->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME));
            MAKE_PERFORMANCE_COUNTER("Draw calls", "%d", (int)Performance::get_singleton()->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME));
            
            // Joke ones
            MAKE_PERFORMANCE_COUNTER("AFPS", "%.0f rad/s", Math_TAU * fps);
            MAKE_PERFORMANCE_COUNTER("DFPS", "%.0f", Math::rad_to_deg(Math_TAU * fps));
            MAKE_PERFORMANCE_COUNTER("RFPS", "%.0f", fps);
            MAKE_PERFORMANCE_COUNTER("RPMS", "%.0f", fps * 60.0);
            MAKE_PERFORMANCE_COUNTER("Frametime", "%.2f ms", get_process_delta_time() * 1000.0);
            MAKE_PERFORMANCE_COUNTER("Frame", "%lu", Engine::get_singleton()->get_frames_drawn());

            #undef MAKE_PERFORMANCE_COUNTER

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void GameDebugger::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            if (!Engine::get_singleton()->is_editor_hint()) {
                set_process(true);
            }
        } break;
        case NOTIFICATION_PROCESS: {
            _draw_ui();
        } break;
    }
}
