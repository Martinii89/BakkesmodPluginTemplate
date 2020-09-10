#pragma once
#include "imgui.h"
namespace ImGui
{
    IMGUI_API bool RangeSliderScalar(const char* label, ImGuiDataType data_type, void* p_data1, void* p_data2, const void* p_min, const void* p_max, const char* format = NULL, float power = 1.0f);
    IMGUI_API bool RangeSliderScalarN(const char* label, ImGuiDataType data_type, void* p_data1, void* p_data2, int components, const void* p_min, const void* p_max, const char* format = NULL, float power = 1.0f);
    IMGUI_API bool RangeSliderFloat(const char* label, float* v1, float* v2, float v_min, float v_max, const char* format = "(%.3f, %.3f)", float power = 1.0f);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display. Use power!=1.0 for power curve sliders
    IMGUI_API bool RangeSliderFloat2(const char* label, float v1[2], float v2[2], float v_min, float v_max, const char* format = "(%.3f, %.3f)", float power = 1.0f);
    IMGUI_API bool RangeSliderFloat3(const char* label, float v1[3], float v2[3], float v_min, float v_max, const char* format = "(%.3f, %.3f)", float power = 1.0f);
    IMGUI_API bool RangeSliderFloat4(const char* label, float v1[4], float v2[4], float v_min, float v_max, const char* format = "(%.3f, %.3f)", float power = 1.0f);
    IMGUI_API bool RangeSliderAngle(const char* label, float* v_rad1, float* v_rad2, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "(%d, %d) deg");
    IMGUI_API bool RangeSliderInt(const char* label, int* v1, int* v2, int v_min, int v_max, const char* format = "(%d, %d)");
    IMGUI_API bool RangeSliderInt2(const char* label, int v1[2], int v2[2], int v_min, int v_max, const char* format = "(%d, %d)");
    IMGUI_API bool RangeSliderInt3(const char* label, int v1[3], int v2[3], int v_min, int v_max, const char* format = "(%d, %d)");
    IMGUI_API bool RangeSliderInt4(const char* label, int v1[4], int v2[4], int v_min, int v_max, const char* format = "(%d, %d)");
    IMGUI_API bool RangeVSliderScalar(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data1, void* p_data2, const void* p_min, const void* p_max, const char* format = NULL, float power = 1.0f);
    IMGUI_API bool RangeVSliderFloat(const char* label, const ImVec2& size, float* v1, float* v2, float v_min, float v_max, const char* format = "(%.3f, %.3f)", float power = 1.0f);
    IMGUI_API bool RangeVSliderInt(const char* label, const ImVec2& size, int* v1, int* v2, int v_min, int v_max, const char* format = "(%d, %d)");

} // namespace ImGui