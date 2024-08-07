#ifndef GODOT_IMGUI_H
#define GODOT_IMGUI_H
#ifdef DEBUG_ENABLED
#include "core/io/config_file.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/image_texture.h"

class GodotImGuiTool : public RefCounted {
	GDCLASS(GodotImGuiTool, RefCounted);
	bool is_open = false;

public:
	virtual String get_name() const = 0;
	virtual void draw_ui() = 0;
	friend class GodotImGui;
};

struct ImDrawData;

class GodotImGui : public SubViewportContainer {
	GDCLASS(GodotImGui, SubViewportContainer);

private:
	static GodotImGui *singleton;

	const String CONFIG_FILE_PATH = "user://imgui_config.cfg";

	SubViewport *viewport = nullptr;
	struct ObjectDebugInfo {
		bool debug_enabled = false;
		bool is_root = false; // If it's a display tree root object
		Vector<ObjectID> children;
		String config_section_name;
	};
	HashMap<ObjectID, ObjectDebugInfo> debug_status;
	bool debug_active = false;
	bool show_demo_window = false;
	bool show_overlay = false;

	RID framebuffer;
	RID shader;
	RID pipeline;
	RID sampler;
	RID index_buffer;
	int index_buffer_size = 0;
	RID vertex_buffer;
	int vertex_buffer_size = 0;
	HashSet<uint64_t> used_textures;
	HashMap<uint64_t, RID> uniform_sets;
	Ref<ImageTexture> font_texture;
	PackedFloat32Array push_constant_buffer;
	RenderingDevice::VertexFormatID vertex_format;
	Ref<ConfigFile> config_file;
	// If godot process priorities weren't more broken than my love life I wouldn't need this ugly hack
	// uses scenetree's add_idle_callback to call this, which then calls _end_frame
	static void _end_frame_callback();
	void _init_imgui();
	void _begin_frame();
	void _draw_debug_ui();
	void _end_frame();
	void _recreate_framebuffer();
	void _render_draw_data(ImDrawData *p_draw_data);
	void _setup_buffers(ImDrawData *p_draw_data);
	void _show_overlay();
	void _draw_debug_object_tree(ObjectID p_id);
	Vector<Ref<GodotImGuiTool>> tools;
	void _on_window_size_changed();
	virtual Size2 get_minimum_size() const override;

protected:
	void _notification(int p_what);
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void unhandled_key_input(const Ref<InputEvent> &p_event) override;
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	static GodotImGui *get_singleton() { return singleton; };

	// Use this to register debug objects
	void register_debug_object(const ObjectID &p_object_id);
	void unregister_debug_object(const ObjectID &p_object_id);
	// Use this to check if debugging is enabled for this object before drawing using imgui
	bool is_debugging_enabled(const ObjectID &p_object_id) const;
	bool begin_debug_window(const Object *p_object);
	bool is_debug_enabled(const Object *p_object) const;

	void set_enable_overlay(bool p_enable);

	static void ImImage(const Ref<Texture2D> &p_texture, const Vector2i &p_size = Vector2i(0, 0));
	static void DrawJoystick(const Vector2 &p_value, const float p_radius);

	void save_config();
	Variant get_config_value(Object *p_obj, const String &p_key, Variant p_default_value = Variant()) const;
	void set_config_value(Object *p_obj, const String &p_key, Variant p_value);

	GodotImGui();
	~GodotImGui();
};
#endif // DEBUG_ENABLED
#endif // GODOT_IMGUI_H
