#ifndef IMGUI_REGISTER_TYPES_H
#define IMGUI_REGISTER_TYPES_H

#include "modules/register_module_types.h"

void initialize_imgui_module(ModuleInitializationLevel p_level);
void uninitialize_imgui_module(ModuleInitializationLevel p_level);
void imgui_module_post_init();
void imgui_module_unload();

#endif // IMGUI_REGISTER_TYPES_H
