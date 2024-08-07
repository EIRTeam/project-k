#include "godot_imgui.h"

#include "main/performance.h"
#include "modules/modules_enabled.gen.h"
#include "scene/gui/label.h"
#include "scene/main/window.h"
#include "servers/rendering/rendering_device_binds.h"
#include "imgui.h"

GodotImGui *GodotImGui::singleton = nullptr;

static void embrace_the_darkness() {
	// Soft Cherry style by Patitotective from ImThemes
	ImGuiStyle &style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.4000000059604645f;
	style.WindowPadding = ImVec2(10.0f, 10.0f);
	style.WindowRounding = 4.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(50.0f, 50.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(5.0f, 3.0f);
	style.FrameRounding = 3.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(6.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(3.0f, 2.0f);
	style.CellPadding = ImVec2(3.0f, 3.0f);
	style.IndentSpacing = 6.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 13.0f;
	style.ScrollbarRounding = 16.0f;
	style.GrabMinSize = 20.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 1.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(0.8588235378265381f, 0.929411768913269f, 0.886274516582489f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5215686559677124f, 0.5490196347236633f, 0.5333333611488342f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1294117718935013f, 0.1372549086809158f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1490196138620377f, 0.1568627506494522f, 0.1882352977991104f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.1372549086809158f, 0.1137254908680916f, 0.1333333402872086f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.168627455830574f, 0.1843137294054031f, 0.2313725501298904f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2313725501298904f, 0.2000000029802322f, 0.2705882489681244f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.501960813999176f, 0.07450980693101883f, 0.2549019753932953f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.239215686917305f, 0.239215686917305f, 0.2196078449487686f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3725490272045135f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.6941176652908325f, 0.6941176652908325f, 0.686274528503418f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.6941176652908325f, 0.6941176652908325f, 0.686274528503418f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.658823549747467f, 0.1372549086809158f, 0.1764705926179886f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6509804129600525f, 0.1490196138620377f, 0.3450980484485626f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.7098039388656616f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.6509804129600525f, 0.1490196138620377f, 0.3450980484485626f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.6509804129600525f, 0.1490196138620377f, 0.3450980484485626f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.501960813999176f, 0.07450980693101883f, 0.2549019753932953f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.6509804129600525f, 0.1490196138620377f, 0.3450980484485626f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.501960813999176f, 0.07450980693101883f, 0.2549019753932953f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.6509804129600525f, 0.1490196138620377f, 0.3450980484485626f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.168627455830574f, 0.1843137294054031f, 0.2313725501298904f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.501960813999176f, 0.07450980693101883f, 0.2549019753932953f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8588235378265381f, 0.929411768913269f, 0.886274516582489f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.3098039329051971f, 0.7764706015586853f, 0.196078434586525f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.3843137323856354f, 0.6274510025978088f, 0.9176470637321472f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 1.0f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.300000011920929f);
}

GodotImGui::GodotImGui() {
	singleton = this;
	set_mouse_filter(MouseFilter::MOUSE_FILTER_IGNORE);

	set_anchors_and_offsets_preset(Control::LayoutPreset::PRESET_FULL_RECT);
	set_process_unhandled_key_input(true);
	set_process_unhandled_input(true);
	set_process_input(true);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	//io.IniFilename = nullptr;

	ImGui::StyleColorsDark();

	viewport = memnew(SubViewport);
	viewport->set_clear_mode(SubViewport::CLEAR_MODE_NEVER);
	viewport->set_update_mode(SubViewport::UPDATE_DISABLED);
	viewport->set_transparent_background(true);
	set_stretch(true);
	add_child(viewport);

	_init_imgui();

	// scale_x, scale_y, translation_x, translation_y
	push_constant_buffer.resize(4);

	// font setup
	io.Fonts->AddFontDefault();

	// Add icon font
	ImFontConfig font_config;
	font_config.MergeMode = true;
	font_config.FontDataOwnedByAtlas = false; // We own this, this prevents ImGui from trying to free it on exit!!!
	font_config.GlyphOffset.y = 4;
	font_config.SizePixels = 15.0f;
	//font_config.GlyphMinAdvanceX = 13.0f;

	unsigned char *pixels;
	Vector<uint8_t> pixels_vec;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Copy image to a godot
	pixels_vec.resize(width * height * 4);
	memcpy(pixels_vec.ptrw(), pixels, pixels_vec.size());

	Ref<Image> img = memnew(Image(width, height, false, Image::Format::FORMAT_RGBA8, pixels_vec));
	font_texture = ImageTexture::create_from_image(img);
	uint64_t rid = font_texture->get_rid().get_id();
	ImGui::GetIO().Fonts->SetTexID((void *)rid);
	embrace_the_darkness();

	config_file.instantiate();
	config_file->load(CONFIG_FILE_PATH);

}

void GodotImGui::_end_frame_callback() {
	GodotImGui *gim = GodotImGui::get_singleton();
	if (gim && !Engine::get_singleton()->is_in_physics_frame()) {
		gim->_end_frame();
	}
}

void GodotImGui::_begin_frame() {
	Size2 display_size = viewport->get_visible_rect().size;
	ImGui::GetIO().DisplaySize = ImVec2(display_size.x, display_size.y);
	ImGui::GetIO().DeltaTime = (float)get_process_delta_time();
	ImGui::NewFrame();
	if (show_overlay) {
		_show_overlay();
	}
	_draw_debug_ui();
}

void GodotImGui::_draw_debug_ui() {
	if (debug_active) {
		if (ImGui::Begin("Game Debug", &debug_active)) {
			if (ImGui::BeginTabBar("DebugTabBar")) {
				if (ImGui::BeginTabItem("Objects")) {
					for (KeyValue<ObjectID, ObjectDebugInfo> kv : debug_status) {
						if (kv.value.is_root) {
							_draw_debug_object_tree(kv.key);
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Other")) {
					if (ImGui::Button("Toggle ImGui demo window")) {
						show_demo_window = !show_demo_window;
					}
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Timescale");
					ImGui::SameLine();
					if (ImGui::Button("1x")) {
						Engine::get_singleton()->set_time_scale(1.0);
					}
					ImGui::SameLine();
					if (ImGui::Button("0.5x")) {
						Engine::get_singleton()->set_time_scale(0.5);
					}
					ImGui::SameLine();
					if (ImGui::Button("0.25x")) {
						Engine::get_singleton()->set_time_scale(0.25);
					}

					for (int i = 0; i < tools.size(); i++) {
						ImGui::Checkbox(tools[i]->get_name().utf8().get_data(), &tools[i]->is_open);
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
		if (show_demo_window) {
			ImGui::ShowDemoWindow(&show_demo_window);
		}
	}
	for (int i = 0; i < tools.size(); i++) {
		if (tools[i]->is_open) {
			if (ImGui::Begin(tools[i]->get_name().utf8().get_data(), &tools[i]->is_open)) {
				tools[i]->draw_ui();
			}
			ImGui::End();
		}
	}
}

void GodotImGui::_show_overlay() {
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	const float PAD = 10.0f;
	const ImGuiViewport *imgui_viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = imgui_viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = work_pos.x + PAD;
	window_pos.y = work_pos.y + PAD;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	window_flags |= ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowBgAlpha(0.35);
	if (ImGui::Begin("Game Overlay", &show_overlay, window_flags)) {
		ImGui::Text("%.3f FPS", Performance::get_singleton()->get_monitor(Performance::TIME_FPS));
		ImGui::Text("%.3f ms frametime", Performance::get_singleton()->get_monitor(Performance::TIME_PROCESS) * 1000.0);
		ImGui::Separator();
		ImGui::Text("F1 - Show debug menu");
		ImGui::Text("F2 - Hide this overlay");
	}
	ImGui::End();
}

void GodotImGui::_recreate_framebuffer() {
	if (framebuffer.is_valid()) {
		framebuffer = RID();
	}
	RID vp_text = RenderingServer::get_singleton()->texture_get_rd_texture(viewport->get_texture()->get_rid());
	Vector<RID> vp_text_a;
	vp_text_a.push_back(vp_text);
	framebuffer = RenderingDevice::get_singleton()->framebuffer_create(vp_text_a);
}

void GodotImGui::_end_frame() {
	ImGui::Render();
	_render_draw_data(ImGui::GetDrawData());
}

void GodotImGui::_render_draw_data(ImDrawData *p_draw_data) {
	RenderingDevice *rd = RenderingDevice::get_singleton();
	rd->draw_command_begin_label("ImGui");
	if (!rd) {
		return;
	}
	// Viewport likely changed size or just doesn't exist in the first place, we have to recreate it
	if (!rd->framebuffer_is_valid(framebuffer)) {
		_recreate_framebuffer();
	}

	int vertex_size = sizeof(ImDrawVert);

	push_constant_buffer.ptrw()[0] = 2.0 / p_draw_data->DisplaySize.x;
	push_constant_buffer.ptrw()[1] = 2.0 / p_draw_data->DisplaySize.y;
	push_constant_buffer.ptrw()[2] = -1.0 - (p_draw_data->DisplayPos.x * push_constant_buffer[0]);
	push_constant_buffer.ptrw()[3] = -1.0 - (p_draw_data->DisplayPos.y * push_constant_buffer[1]);

	if (index_buffer_size < p_draw_data->TotalIdxCount) {
		if (index_buffer.is_valid()) {
			rd->free(index_buffer);
		}
		index_buffer = rd->index_buffer_create(p_draw_data->TotalIdxCount, RenderingDevice::IndexBufferFormat::INDEX_BUFFER_FORMAT_UINT16);
		index_buffer_size = p_draw_data->TotalIdxCount;
	}
	if (vertex_buffer_size < p_draw_data->TotalIdxCount) {
		if (vertex_buffer.is_valid()) {
			rd->free(vertex_buffer);
		}
		vertex_buffer = rd->vertex_buffer_create(p_draw_data->TotalVtxCount * vertex_size);
		vertex_buffer_size = p_draw_data->TotalVtxCount;
	}

	used_textures.clear();

	// Send invalid textures to the shadow realm
	{
		Vector<uint64_t> to_erase;

		for (KeyValue<uint64_t, RID> kv : uniform_sets) {
			if (!rd->uniform_set_is_valid(kv.value)) {
				to_erase.push_back(kv.key);
			}
		}

		for (int key : to_erase) {
			uniform_sets.erase(key);
		}
	}

	if (p_draw_data->CmdListsCount > 0) {
		_setup_buffers(p_draw_data);
	}

	Vector<Color> clear_color;
	clear_color.push_back(Color(0.0, 0.0, 0.0, 0.0));

	RenderingDevice::DrawListID draw_list = rd->draw_list_begin(framebuffer,
			RenderingDevice::InitialAction::INITIAL_ACTION_CLEAR, RenderingDevice::FinalAction::FINAL_ACTION_READ,
			RenderingDevice::InitialAction::INITIAL_ACTION_CLEAR, RenderingDevice::FinalAction::FINAL_ACTION_READ,
			clear_color);
	rd->draw_list_bind_render_pipeline(draw_list, pipeline);
	rd->draw_list_set_push_constant(draw_list, push_constant_buffer.ptr(), push_constant_buffer.size() * 4);

	int index_offset = 0;
	int vertex_offset = 0;

	for (int i = 0; i < p_draw_data->CmdListsCount; i++) {
		ImDrawList *cmd_list = p_draw_data->CmdLists[i];

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			ImDrawCmd draw_cmd = cmd_list->CmdBuffer[cmd_i];
			if (draw_cmd.ElemCount == 0) {
				continue;
			}

			RID index_array = rd->index_array_create(index_buffer, draw_cmd.IdxOffset + index_offset, draw_cmd.ElemCount);

			uint64_t voff = (draw_cmd.VtxOffset + vertex_offset) * vertex_size;
			Vector<RID> src_buffers;
			src_buffers.resize(3);
			src_buffers.fill(vertex_buffer);
			Vector<uint64_t> vtx_offsets;
			vtx_offsets.resize(3);
			vtx_offsets.fill(voff);
			RID vertex_array = rd->vertex_array_create(cmd_list->VtxBuffer.Size, vertex_format, src_buffers, vtx_offsets);

			rd->draw_list_bind_uniform_set(draw_list, uniform_sets[(uint64_t)draw_cmd.GetTexID()], 0);
			rd->draw_list_bind_index_array(draw_list, index_array);
			rd->draw_list_bind_vertex_array(draw_list, vertex_array);

			Rect2 clip_rect = Rect2(
					draw_cmd.ClipRect.x,
					draw_cmd.ClipRect.y,
					draw_cmd.ClipRect.z - draw_cmd.ClipRect.x,
					draw_cmd.ClipRect.w - draw_cmd.ClipRect.y);

			clip_rect.position -= Vector2(p_draw_data->DisplayPos.x, p_draw_data->DisplayPos.y);

			rd->draw_list_enable_scissor(draw_list, clip_rect);

			rd->draw_list_draw(draw_list, true, 1);

			rd->free(index_array);
			rd->free(vertex_array);
		}
		index_offset += cmd_list->IdxBuffer.Size;
		vertex_offset += cmd_list->VtxBuffer.Size;
	}
	rd->draw_list_end();
	rd->draw_command_end_label();

	// clean up unused textures
	for (KeyValue<uint64_t, RID> kv : uniform_sets) {
		if (!used_textures.has(kv.key)) {
			if (rd->uniform_set_is_valid(kv.value)) {
				rd->free(kv.value);
			}
			uniform_sets.erase(kv.key);
		}
	}
}

void GodotImGui::_setup_buffers(ImDrawData *p_draw_data) {
	RenderingDevice *rd = RenderingDevice::get_singleton();

	int vertex_size = sizeof(ImDrawVert);

	PackedByteArray index_buffer_data;
	index_buffer_data.resize(p_draw_data->TotalIdxCount * sizeof(uint16_t));
	PackedByteArray vertex_buffer_data;
	vertex_buffer_data.resize(p_draw_data->TotalVtxCount * vertex_size);

	int vertex_offset = 0;
	int index_offset = 0;

	Vector<RenderingDevice::Uniform> uniform_array;
	uniform_array.resize(1);

	for (int i = 0; i < p_draw_data->CmdListsCount; i++) {
		ImDrawList *cmd_list = p_draw_data->CmdLists[i];

		int vertex_bytes = cmd_list->VtxBuffer.Size * vertex_size;
		memcpy(vertex_buffer_data.ptrw() + vertex_offset, cmd_list->VtxBuffer.Data, vertex_bytes);
		vertex_offset += vertex_bytes;

		int index_bytes = cmd_list->IdxBuffer.Size * sizeof(uint16_t);
		memcpy(index_buffer_data.ptrw() + index_offset, cmd_list->IdxBuffer.Data, index_bytes);
		index_offset += index_bytes;

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			ImDrawCmd draw_cmd = cmd_list->CmdBuffer[cmd_i];
			uint64_t tex_id = (uint64_t)draw_cmd.GetTexID();
			if (tex_id == 0) {
				continue;
			}

			used_textures.insert(tex_id);

			if (!uniform_sets.has(tex_id)) {
				RID tex_rid = RID::from_uint64(tex_id);
				tex_rid = RenderingServer::get_singleton()->texture_get_rd_texture(tex_rid);
				RenderingDevice::Uniform uniform;
				uniform.binding = 0;
				uniform.uniform_type = RenderingDevice::UniformType::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
				uniform.append_id(sampler);
				uniform.append_id(tex_rid);
				uniform_array.ptrw()[0] = uniform;
				RID uniform_set = rd->uniform_set_create(uniform_array, shader, 0);
				uniform_sets[tex_id] = uniform_set;
			}
		}
	}

	rd->buffer_update(index_buffer, 0, index_buffer_data.size(), index_buffer_data.ptr());
	rd->buffer_update(vertex_buffer, 0, vertex_buffer_data.size(), vertex_buffer_data.ptr());
}

ImGuiKey _map_to_imgui_key(const Key &p_key) {
	ImGuiKey imgui_key = ImGuiKey_None;
	if (p_key >= Key::A && p_key <= Key::Z) {
		imgui_key = (ImGuiKey)(ImGuiKey_A + (p_key - Key::A));
	} else if (p_key >= Key::KEY_0 && p_key <= Key::KEY_9) {
		imgui_key = (ImGuiKey)(ImGuiKey_0 + (p_key - Key::KEY_0));
	} else if (p_key >= Key::KP_0 && p_key <= Key::KP_9) {
		imgui_key = (ImGuiKey)(ImGuiKey_Keypad0 + (p_key - Key::KP_0));
	} else if (p_key >= Key::F1 && p_key <= Key::F12) {
		imgui_key = (ImGuiKey)(ImGuiKey_F1 + (p_key - Key::F1));
	} else {
		switch (p_key) {
			case Key::LEFT:
				imgui_key = ImGuiKey_LeftArrow;
				break;
			case Key::UP:
				imgui_key = ImGuiKey_UpArrow;
				break;
			case Key::RIGHT:
				imgui_key = ImGuiKey_RightArrow;
				break;
			case Key::DOWN:
				imgui_key = ImGuiKey_DownArrow;
				break;
			case Key::TAB:
				imgui_key = ImGuiKey_Tab;
				break;
			case Key::KP_PERIOD:
				imgui_key = ImGuiKey_KeypadDecimal;
				break;
			case Key::KP_ADD:
				imgui_key = ImGuiKey_KeypadAdd;
				break;
			case Key::KP_SUBTRACT:
				imgui_key = ImGuiKey_KeypadSubtract;
				break;
			case Key::KP_ENTER:
				imgui_key = ImGuiKey_KeypadEnter;
				break;
			case Key::KP_DIVIDE:
				imgui_key = ImGuiKey_KeypadDivide;
				break;
			case Key::KP_MULTIPLY:
				imgui_key = ImGuiKey_KeypadMultiply;
				break;
			case Key::PAGEUP:
				imgui_key = ImGuiKey_PageUp;
				break;
			case Key::PAGEDOWN:
				imgui_key = ImGuiKey_PageDown;
				break;
			case Key::SHIFT:
				imgui_key = ImGuiKey_LeftShift;
				break;
			case Key::CTRL:
				imgui_key = ImGuiKey_LeftCtrl;
				break;
			case Key::META:
				imgui_key = ImGuiKey_LeftSuper;
				break;
			case Key::ALT:
				imgui_key = ImGuiKey_LeftAlt;
				break;
			case Key::CAPSLOCK:
				imgui_key = ImGuiKey_CapsLock;
				break;
			case Key::NUMLOCK:
				imgui_key = ImGuiKey_NumLock;
				break;
			case Key::APOSTROPHE:
				imgui_key = ImGuiKey_Apostrophe;
				break;
			case Key::COMMA:
				imgui_key = ImGuiKey_Comma;
				break;
			case Key::MINUS:
				imgui_key = ImGuiKey_Minus;
				break;
			case Key::PERIOD:
				imgui_key = ImGuiKey_Period;
				break;
			case Key::SLASH:
				imgui_key = ImGuiKey_Slash;
				break;
			case Key::SEMICOLON:
				imgui_key = ImGuiKey_Semicolon;
				break;
			case Key::EQUAL:
				imgui_key = ImGuiKey_Equal;
				break;
			case Key::BRACKETLEFT:
				imgui_key = ImGuiKey_LeftBracket;
				break;
			case Key::BACKSLASH:
				imgui_key = ImGuiKey_Backslash;
				break;
			case Key::BRACKETRIGHT:
				imgui_key = ImGuiKey_RightBracket;
				break;
			case Key::PAUSE:
				imgui_key = ImGuiKey_Pause;
				break;
			case Key::HOME:
				imgui_key = ImGuiKey_Home;
				break;
			case Key::END:
				imgui_key = ImGuiKey_End;
				break;
			case Key::BACKSPACE:
				imgui_key = ImGuiKey_Backspace;
				break;
			case Key::ENTER:
				imgui_key = ImGuiKey_Enter;
				break;
			case Key::SPACE:
				imgui_key = ImGuiKey_Space;
				break;
			case Key::ESCAPE:
				imgui_key = ImGuiKey_Escape;
				break;
			default: {
			};
		}
	}

	return imgui_key;
}

void GodotImGui::unhandled_key_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> ik = p_event;
	bool captured = false;

	ImGuiIO &io = ImGui::GetIO();

	if (ik.is_valid()) {
		ImGuiKey imk = _map_to_imgui_key(ik->get_keycode());
		io.AddKeyEvent(ImGuiKey::ImGuiMod_Ctrl, Input::get_singleton()->is_key_pressed(Key::CTRL));
		io.AddKeyEvent(ImGuiKey::ImGuiMod_Shift, Input::get_singleton()->is_key_pressed(Key::SHIFT));
		io.AddKeyEvent(ImGuiKey::ImGuiMod_Alt, Input::get_singleton()->is_key_pressed(Key::ALT));
		io.AddKeyEvent(ImGuiKey::ImGuiMod_Super, Input::get_singleton()->is_key_pressed(Key::META));
		if (imk != ImGuiKey::ImGuiKey_None) {
			io.AddKeyEvent(imk, ik->is_pressed());
			if (ik->get_unicode() != 0 && ik->is_pressed() && io.WantTextInput) {
				io.AddInputCharacter(ik->get_unicode());
			}
			captured = io.WantCaptureKeyboard || io.WantTextInput;
		}
	}

	if (captured) {
		accept_event();
	}
}

void GodotImGui::unhandled_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> kev = p_event;
	if (kev.is_valid()) {
		if (kev->is_pressed() && !kev->is_echo()) {
			if (kev->get_keycode() == Key::F1) {
				debug_active = !debug_active;
				Input::MouseMode mode = debug_active ? Input::MouseMode::MOUSE_MODE_VISIBLE : Input::MouseMode::MOUSE_MODE_CAPTURED;
				Input::get_singleton()->set_mouse_mode(mode);
				accept_event();
			} else if (kev->get_keycode() == Key::F2) {
				show_overlay = !show_overlay;
				accept_event();
			}
		}
	}
}

void GodotImGui::input(const Ref<InputEvent> &p_event) {
	ImGuiIO &io = ImGui::GetIO();
	bool captured = false;

	Ref<InputEventMouse> mouse_ev = p_event;
	if (mouse_ev.is_valid()) {
		captured = io.WantCaptureMouse;
	}

	Ref<InputEventMouseMotion> mouse_mot_ev = p_event;
	if (mouse_mot_ev.is_valid()) {
		Vector2 mouse_pos = mouse_mot_ev->get_position();
		io.AddMousePosEvent(mouse_pos.x, mouse_pos.y);
	}

	Ref<InputEventMouseButton> mouse_but_event = p_event;

	if (mouse_but_event.is_valid()) {
		switch (mouse_but_event->get_button_index()) {
			case MouseButton::LEFT: {
				io.AddMouseButtonEvent(ImGuiMouseButton_Left, mouse_but_event->is_pressed());
			} break;
			case MouseButton::RIGHT: {
				io.AddMouseButtonEvent(ImGuiMouseButton_Right, mouse_but_event->is_pressed());
			} break;
			case MouseButton::MIDDLE: {
				io.AddMouseButtonEvent(ImGuiMouseButton_Middle, mouse_but_event->is_pressed());
			} break;
			case MouseButton::WHEEL_UP: {
				io.AddMouseWheelEvent(0, mouse_but_event->get_factor());
			} break;
			case MouseButton::WHEEL_DOWN: {
				io.AddMouseWheelEvent(0, -mouse_but_event->get_factor());
			} break;
			case MouseButton::WHEEL_LEFT: {
				io.AddMouseWheelEvent(-mouse_but_event->get_factor(), 0);
			} break;
			case MouseButton::WHEEL_RIGHT: {
				io.AddMouseWheelEvent(mouse_but_event->get_factor(), 0);
			} break;
			case MouseButton::MB_XBUTTON1: {
				io.AddMouseButtonEvent(ImGuiMouseButton_Middle + 1, mouse_but_event->is_pressed());
			} break;
			case MouseButton::MB_XBUTTON2: {
				io.AddMouseButtonEvent(ImGuiMouseButton_Middle + 2, mouse_but_event->is_pressed());
			} break;
			default: {
			}
		}
	}

	if (captured) {
		get_viewport()->set_input_as_handled();
	}
}

// Not really necessary but a good placeholder so the IM_ASSERT() works
struct ImGui_ImplGodotRD_Data {
};

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;
out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;
void main()
{
	Out.Color = aColor;
	Out.UV = aUV;
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] = {
	0x07230203, 0x00010000, 0x0008000b, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x000a000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000f, 0x00000015,
	0x0000001b, 0x0000001c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
	0x00000000, 0x00030005, 0x00000009, 0x00000000, 0x00050006, 0x00000009, 0x00000000, 0x6f6c6f43,
	0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005, 0x0000000b, 0x0074754f,
	0x00040005, 0x0000000f, 0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015, 0x00565561, 0x00060005,
	0x00000019, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000019, 0x00000000,
	0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000, 0x00040005, 0x0000001c,
	0x736f5061, 0x00000000, 0x00060005, 0x0000001e, 0x73755075, 0x6e6f4368, 0x6e617473, 0x00000074,
	0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006, 0x0000001e, 0x00000001,
	0x61725475, 0x616c736e, 0x00006574, 0x00030005, 0x00000020, 0x00006370, 0x00040047, 0x0000000b,
	0x0000001e, 0x00000000, 0x00040047, 0x0000000f, 0x0000001e, 0x00000002, 0x00040047, 0x00000015,
	0x0000001e, 0x00000001, 0x00050048, 0x00000019, 0x00000000, 0x0000000b, 0x00000000, 0x00030047,
	0x00000019, 0x00000002, 0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e,
	0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008,
	0x00030047, 0x0000001e, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
	0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040017,
	0x00000008, 0x00000006, 0x00000002, 0x0004001e, 0x00000009, 0x00000007, 0x00000008, 0x00040020,
	0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015,
	0x0000000c, 0x00000020, 0x00000001, 0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020,
	0x0000000e, 0x00000001, 0x00000007, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000001, 0x00040020,
	0x00000011, 0x00000003, 0x00000007, 0x0004002b, 0x0000000c, 0x00000013, 0x00000001, 0x00040020,
	0x00000014, 0x00000001, 0x00000008, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020,
	0x00000017, 0x00000003, 0x00000008, 0x0003001e, 0x00000019, 0x00000007, 0x00040020, 0x0000001a,
	0x00000003, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014,
	0x0000001c, 0x00000001, 0x0004001e, 0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f,
	0x00000009, 0x0000001e, 0x0004003b, 0x0000001f, 0x00000020, 0x00000009, 0x00040020, 0x00000021,
	0x00000009, 0x00000008, 0x0004002b, 0x00000006, 0x00000028, 0x00000000, 0x0004002b, 0x00000006,
	0x00000029, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
	0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011, 0x00000012,
	0x0000000b, 0x0000000d, 0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008, 0x00000016,
	0x00000015, 0x00050041, 0x00000017, 0x00000018, 0x0000000b, 0x00000013, 0x0003003e, 0x00000018,
	0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041, 0x00000021, 0x00000022,
	0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085, 0x00000008,
	0x00000024, 0x0000001d, 0x00000023, 0x00050041, 0x00000021, 0x00000025, 0x00000020, 0x00000013,
	0x0004003d, 0x00000008, 0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027, 0x00000024,
	0x00000026, 0x00050051, 0x00000006, 0x0000002a, 0x00000027, 0x00000000, 0x00050051, 0x00000006,
	0x0000002b, 0x00000027, 0x00000001, 0x00070050, 0x00000007, 0x0000002c, 0x0000002a, 0x0000002b,
	0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d, 0x0003003e,
	0x0000002d, 0x0000002c, 0x000100fd, 0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
	fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] = {
	// 1111.13.0
	0x07230203, 0x00010000, 0x0008000b, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
	0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
	0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x0000726f, 0x00030005, 0x0000000b, 0x00000000,
	0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001,
	0x00005655, 0x00030005, 0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574,
	0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x0000001e,
	0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000000, 0x00040047, 0x00000016, 0x00000021,
	0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
	0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
	0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006,
	0x00000002, 0x0004001e, 0x0000000b, 0x00000007, 0x0000000a, 0x00040020, 0x0000000c, 0x00000001,
	0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000001, 0x00040015, 0x0000000e, 0x00000020,
	0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001,
	0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
	0x00000001, 0x00000000, 0x0003001b, 0x00000014, 0x00000013, 0x00040020, 0x00000015, 0x00000000,
	0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000000, 0x0004002b, 0x0000000e, 0x00000018,
	0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036, 0x00000002, 0x00000004,
	0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000010, 0x00000011, 0x0000000d,
	0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017,
	0x00000016, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a,
	0x0000001b, 0x0000001a, 0x00050057, 0x00000007, 0x0000001c, 0x00000017, 0x0000001b, 0x00050085,
	0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd,
	0x00010038
};

void GodotImGui::_init_imgui() {
	print_verbose("Imgui init");
	ImGuiIO &io = ImGui::GetIO();
	IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

	ImGui_ImplGodotRD_Data *bd = IM_NEW(ImGui_ImplGodotRD_Data)();
	io.BackendPlatformUserData = (void *)bd;
	io.BackendPlatformName = "imgui_impl_godotrd";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	RenderingDevice *rd = RenderingDevice::get_singleton();
	if (!rd) {
		return;
	}
	int vtx_stride = sizeof(ImDrawVert);
	{
		Vector<RenderingDevice::VertexAttribute> vertex_attribs;
		RenderingDevice::VertexAttribute attrib_points;
		attrib_points.location = 0;
		attrib_points.format = RenderingDevice::DataFormat::DATA_FORMAT_R32G32_SFLOAT;
		attrib_points.stride = vtx_stride;
		attrib_points.offset = IM_OFFSETOF(ImDrawVert, pos);
		vertex_attribs.push_back(attrib_points);

		RenderingDevice::VertexAttribute attrib_uvs;
		attrib_uvs.location = 1;
		attrib_uvs.format = RenderingDevice::DataFormat::DATA_FORMAT_R32G32_SFLOAT;
		attrib_uvs.stride = vtx_stride;
		attrib_uvs.offset = IM_OFFSETOF(ImDrawVert, uv);
		vertex_attribs.push_back(attrib_uvs);

		RenderingDevice::VertexAttribute attrib_color;
		attrib_color.location = 2;
		attrib_color.format = RenderingDevice::DataFormat::DATA_FORMAT_R8G8B8A8_UNORM;
		attrib_color.stride = vtx_stride;
		attrib_color.offset = IM_OFFSETOF(ImDrawVert, col);
		vertex_attribs.push_back(attrib_color);

		vertex_format = rd->vertex_format_create(vertex_attribs);
	}

	RenderingDevice::PipelineColorBlendState blend_state = RenderingDevice::PipelineColorBlendState::create_blend(1);

	blend_state.blend_constant = Color(0.0, 0.0, 0.0, 0.0);
	blend_state.attachments.write[0].enable_blend = true;
	blend_state.attachments.write[0].src_color_blend_factor = RenderingDevice::BlendFactor::BLEND_FACTOR_SRC_ALPHA;
	blend_state.attachments.write[0].dst_color_blend_factor = RenderingDevice::BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_state.attachments.write[0].color_blend_op = RenderingDevice::BlendOperation::BLEND_OP_ADD;
	blend_state.attachments.write[0].src_alpha_blend_factor = RenderingDevice::BlendFactor::BLEND_FACTOR_ONE;
	blend_state.attachments.write[0].dst_alpha_blend_factor = RenderingDevice::BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_state.attachments.write[0].alpha_blend_op = RenderingDevice::BlendOperation::BLEND_OP_ADD;

	RenderingDevice::PipelineRasterizationState rasterization_state;
	rasterization_state.front_face = RenderingDevice::PolygonFrontFace::POLYGON_FRONT_FACE_COUNTER_CLOCKWISE;

	{
		Vector<RenderingDevice::ShaderStageSPIRVData> shader_stage_datas;

		RenderingDevice::ShaderStageSPIRVData vertex_shader;
		vertex_shader.shader_stage = RenderingDevice::ShaderStage::SHADER_STAGE_VERTEX;
		vertex_shader.spirv.resize(sizeof(__glsl_shader_vert_spv));
		memcpy(vertex_shader.spirv.ptrw(), __glsl_shader_vert_spv, sizeof(__glsl_shader_vert_spv));
		shader_stage_datas.push_back(vertex_shader);

		RenderingDevice::ShaderStageSPIRVData fragment_shader;
		fragment_shader.shader_stage = RenderingDevice::ShaderStage::SHADER_STAGE_FRAGMENT;
		fragment_shader.spirv.resize(sizeof(__glsl_shader_frag_spv));
		memcpy(fragment_shader.spirv.ptrw(), __glsl_shader_frag_spv, sizeof(__glsl_shader_frag_spv));
		shader_stage_datas.push_back(fragment_shader);

		shader = rd->shader_create_from_spirv(shader_stage_datas);
	}

	RenderingDevice::PipelineMultisampleState pmss;
	RenderingDevice::PipelineDepthStencilState pdss;
	_recreate_framebuffer();
	pipeline = rd->render_pipeline_create(
			shader,
			rd->framebuffer_get_format(framebuffer),
			vertex_format,
			RenderingDevice::RenderPrimitive::RENDER_PRIMITIVE_TRIANGLES,
			rasterization_state,
			pmss,
			pdss,
			blend_state);
	RenderingDevice::SamplerState sampler_state;
	sampler_state.min_filter = RenderingDevice::SamplerFilter::SAMPLER_FILTER_LINEAR;
	sampler_state.mag_filter = RenderingDevice::SamplerFilter::SAMPLER_FILTER_LINEAR;
	sampler_state.mip_filter = RenderingDevice::SamplerFilter::SAMPLER_FILTER_LINEAR;
	sampler_state.repeat_u = RenderingDevice::SamplerRepeatMode::SAMPLER_REPEAT_MODE_REPEAT;
	sampler_state.repeat_v = RenderingDevice::SamplerRepeatMode::SAMPLER_REPEAT_MODE_REPEAT;
	sampler_state.repeat_w = RenderingDevice::SamplerRepeatMode::SAMPLER_REPEAT_MODE_REPEAT;

	sampler = rd->sampler_create(sampler_state);
}

GodotImGui::~GodotImGui() {
	RenderingDevice *rd = RenderingDevice::get_singleton();
	if (pipeline.is_valid()) {
		rd->free(pipeline);
	}
	if (shader.is_valid()) {
		rd->free(shader);
	}
	if (sampler.is_valid()) {
		rd->free(sampler);
	}
	if (index_buffer.is_valid()) {
		rd->free(index_buffer);
	}
	if (vertex_buffer.is_valid()) {
		rd->free(vertex_buffer);
	}
	ImGui::GetIO().BackendPlatformUserData = nullptr;
	ImGui::DestroyContext();
}

void GodotImGui::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			SceneTree::get_singleton()->connect("process_frame", callable_mp(this, &GodotImGui::_begin_frame));
			SceneTree::get_singleton()->add_idle_callback(&_end_frame_callback);
			get_viewport()->connect("size_changed", callable_mp(this, &GodotImGui::_on_window_size_changed));
			_on_window_size_changed();
		} break;
	}
}

bool GodotImGui::begin_debug_window(const Object *p_object) {
	ObjectDebugInfo *di_pointer = debug_status.getptr(p_object->get_instance_id());
	ERR_FAIL_COND_V_MSG(di_pointer == nullptr, false, "Tried to start a debug window for a non registered object");
	const Node *node = Object::cast_to<Node>(p_object);
	String window_title;
	String class_name = p_object->get_class_name();
	int64_t object_id = static_cast<int64_t>(p_object->get_instance_id());
	if (node) {
		window_title = vformat("%s (%s) - ID: %d", node->get_name(), class_name, object_id);
	} else {
		window_title = vformat("%s - ID: %d", class_name, object_id);
	}
	if (!di_pointer->config_section_name.is_empty()) {
		window_title = window_title + "###" + di_pointer->config_section_name;
	}
	return ImGui::Begin(window_title.utf8().ptr(), &(di_pointer->debug_enabled));
}

bool GodotImGui::is_debug_enabled(const Object *p_object) const {
	const ObjectDebugInfo *di_pointer = debug_status.getptr(p_object->get_instance_id());
	ERR_FAIL_COND_V_MSG(di_pointer == nullptr, false, "Tried to start a debug status info for a non registered object");
	return di_pointer->debug_enabled;
}

void GodotImGui::set_enable_overlay(bool p_enable) {
	show_overlay = p_enable;
}

void GodotImGui::ImImage(const Ref<Texture2D> &p_texture, const Vector2i &p_size) {
	if (!p_texture.is_valid()) {
		ImVec2 min = ImGui::GetCursorScreenPos();
		ImVec2 max = ImGui::GetCursorScreenPos();
		max.x += p_size.x;
		max.y += p_size.y;
		ImGui::GetWindowDrawList()->AddRect(min, max, ImColor(1.0f, 0.0f, 1.0f, 1.0f));
		ImGui::Dummy(ImVec2(p_size.x, p_size.y));
		return;
	}
	uint64_t rid = p_texture->get_rid().get_id();
	ImVec2 size;
	if (p_size == Vector2i()) {
		size.x = p_texture->get_width();
		size.y = p_texture->get_height();
	} else {
		size.x = p_size.x;
		size.y = p_size.y;
	}

	ImGui::Image((void *)rid, size);
}

void GodotImGui::DrawJoystick(const Vector2 &p_value, const float p_radius) {
	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 center_pos = ImGui::GetCursorScreenPos();
	center_pos.x += p_radius;
	center_pos.y += p_radius;
	dl->AddCircleFilled(center_pos, p_radius, IM_COL32_BLACK);
	Vector2 stick_input = p_value;
	stick_input *= p_radius;
	stick_input += Vector2(center_pos.x, center_pos.y);
	dl->AddLine(center_pos, ImVec2(stick_input.x, stick_input.y), IM_COL32_WHITE);
	ImGui::Dummy(ImVec2(p_radius * 2, p_radius * 2));
}

void GodotImGui::save_config() {
	config_file->save(CONFIG_FILE_PATH);
}

Variant GodotImGui::get_config_value(Object *p_obj, const String &p_key, Variant p_default_value) const {
	ObjectID obj_id = p_obj->get_instance_id();
	ERR_FAIL_COND_V(!debug_status.has(obj_id), Variant());
	return config_file->get_value(debug_status[obj_id].config_section_name, p_key, p_default_value);
}

void GodotImGui::set_config_value(Object *p_obj, const String &p_key, Variant p_value) {
	ObjectID obj_id = p_obj->get_instance_id();
	ERR_FAIL_COND(!debug_status.has(obj_id));
	config_file->set_value(debug_status[obj_id].config_section_name, p_key, p_value);
}

void GodotImGui::_draw_debug_object_tree(ObjectID p_id) {
	Object *obj = ObjectDB::get_instance(p_id);
	Node *node = Object::cast_to<Node>(obj);
	String label_text;
	const String class_name = obj->get_class_name();
	if (node) {
		const String str = node->get_name();
		label_text = vformat("%s (%s)##%d", str, class_name, static_cast<int64_t>(p_id));
	} else {
		label_text = vformat("%s##%d", class_name, static_cast<int64_t>(p_id));
	}

	ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_FramePadding;

	if (debug_status[p_id].children.size() == 0) {
		tree_flags |= ImGuiTreeNodeFlags_Leaf;
	}

	bool tree_open = ImGui::TreeNodeEx(label_text.utf8().get_data(), tree_flags);
	ImGui::SameLine(0.0f, 10.0f);
	ImGui::GetItemRectSize();

	if (ImGui::Checkbox("##visibilitycheck", &(debug_status.getptr(p_id)->debug_enabled))) {
		if (!debug_status[p_id].config_section_name.is_empty()) {
			config_file->set_value(debug_status[p_id].config_section_name, "enabled", debug_status[p_id].debug_enabled);
			save_config();
		}
	}

	if (tree_open) {
		Vector<ObjectID> children = debug_status[p_id].children;
		for (int i = 0; i < children.size(); i++) {
			_draw_debug_object_tree(children[i]);
		}
		ImGui::TreePop();
	}
}

void GodotImGui::_on_window_size_changed() {
	viewport->set_size_force(get_window()->get_size());
}

Size2 GodotImGui::get_minimum_size() const {
	return Size2();
}

void GodotImGui::register_debug_object(const ObjectID &p_object_id) {
	ERR_FAIL_COND_MSG(debug_status.has(p_object_id), "GAME: Error registering debug object, already registered");
	const ObjectDebugInfo debug_data;
	debug_status.insert(p_object_id, debug_data);
	Node *node = Object::cast_to<Node>(ObjectDB::get_instance(p_object_id));
	debug_status[p_object_id].is_root = true;
	if (node) {
		// Find the first parent of this object that we are tracking
		// So we can put it in the correct container
		Node *parent_node = node->get_parent();
		while (parent_node != nullptr) {
			ObjectID parenet_id = parent_node->get_instance_id();
			if (debug_status.has(parenet_id)) {
				break;
			}
			parent_node = parent_node->get_parent();
		}

		if (parent_node) {
			debug_status[p_object_id].is_root = false;
			ObjectID id = parent_node->get_instance_id();
			debug_status[id].children.push_back(p_object_id);
		}

		SceneTree *tree = node->get_tree();

		if (tree && tree->get_current_scene()) {
			debug_status[p_object_id].config_section_name = tree->get_current_scene()->get_scene_file_path() + node->get_path();
			debug_status[p_object_id].debug_enabled = config_file->get_value(debug_status[p_object_id].config_section_name, "enabled", false);
		}
	}
}

void GodotImGui::unregister_debug_object(const ObjectID &p_object_id) {
	ERR_FAIL_COND_MSG(!debug_status.has(p_object_id), "GAME: Error unregistering debug object, object is not registered");
	debug_status.erase(p_object_id);
	for (KeyValue<ObjectID, ObjectDebugInfo> kv : debug_status) {
		if (kv.value.children.has(p_object_id)) {
			kv.value.children.erase(p_object_id);
			break;
		}
	}
}

bool GodotImGui::is_debugging_enabled(const ObjectID &p_object_id) const {
	const ObjectDebugInfo *debug_data = debug_status.getptr(p_object_id);
	ERR_FAIL_COND_V_MSG(debug_data == nullptr, false, "Error getting debug status for object, object is not registered");
	return debug_data->debug_enabled;
}
