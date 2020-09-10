#pragma once
#include "imgui.h"

#include <ctype.h>      // isprint
#include <vector>       // vector<>
#include <string>       // string
#include <algorithm>    // transform

namespace ImGui
{
    IMGUI_API bool          BeginSearchableCombo(const char* label, const char* preview_value, char* input, int input_size, const char* input_preview_value, ImGuiComboFlags flags = 0);
    IMGUI_API void          EndSearchableCombo();
    IMGUI_API bool          SearchableCombo(const char* label, int* current_item, std::vector<std::string> items, const char* default_preview_text, const char* input_preview_value, int popup_max_height_in_items = -1);
} // namespace ImGui