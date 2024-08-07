#include "register_types.h"

#include "core/config/project_settings.h"
#include "godot_imgui.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

GodotImGui *gd_imgui_singleton = nullptr;

void imgui_module_post_init() {
	if (!RenderingDevice::get_singleton()) {
		print_verbose("GodotImGui: RenderingDevice not found, running in OpenGL?");
		return;
	}
	if (Engine::get_singleton()->is_editor_hint()) {
		print_verbose("GodotImGui: Running in the editor, disabling imgui.");
		return;
	}
	gd_imgui_singleton = memnew(GodotImGui);
	SceneTree *st = SceneTree::get_singleton();
	if (st) {
		st->get_root()->add_child(gd_imgui_singleton, true, Node::INTERNAL_MODE_BACK);
	} else {
		print_line("NOST...");
	}
	Engine::get_singleton()->add_singleton(Engine::Singleton("GodotImGui", GodotImGui::get_singleton()));
}

void imgui_module_unload() {
	if (gd_imgui_singleton) {
		memdelete(gd_imgui_singleton);
	}
}

void initialize_imgui_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_ABSTRACT_CLASS(GodotImGui);
	callable_mp_static(&imgui_module_post_init).call_deferred();
}

void uninitialize_imgui_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
