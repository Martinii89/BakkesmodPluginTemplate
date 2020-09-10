#include "pch.h"
#include "imgui_rangeslider.h"
#include "imgui_internal.h"
#include <string>
// https://github.com/ocornut/imgui/issues/76
// Inspired by: https://github.com/wasikuss/imgui/commit/a50515ace6d9a62ebcd69817f1da927d31c39bb1
// Rewritten for ImGUi 1.75

namespace ImGui
{
    //-------------------------------------------------------------------------
    // Data
    //-------------------------------------------------------------------------

    // Those MIN/MAX values are not define because we need to point to them
    static const signed char    IM_S8_MIN  = -128;
    static const signed char    IM_S8_MAX  = 127;
    static const unsigned char  IM_U8_MIN  = 0;
    static const unsigned char  IM_U8_MAX  = 0xFF;
    static const signed short   IM_S16_MIN = -32768;
    static const signed short   IM_S16_MAX = 32767;
    static const unsigned short IM_U16_MIN = 0;
    static const unsigned short IM_U16_MAX = 0xFFFF;
    static const ImS32          IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
    static const ImS32          IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
    static const ImU32          IM_U32_MIN = 0;
    static const ImU32          IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
    static const ImS64          IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
    static const ImS64          IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
    static const ImS64          IM_S64_MIN = -9223372036854775807LL - 1;
    static const ImS64          IM_S64_MAX = 9223372036854775807LL;
#endif
    static const ImU64          IM_U64_MIN = 0;
#ifdef ULLONG_MAX
    static const ImU64          IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
    static const ImU64          IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif

    //-------------------------------------------------------------------------
    // [SECTION] Data Type and Data Formatting Helpers [Internal]
    //-------------------------------------------------------------------------
    // - PatchFormatStringFloatToInt()
    // - DataTypeFormatString()
    //-------------------------------------------------------------------------

    static const ImGuiDataTypeInfo GDataTypeInfo[] =
    {
        { sizeof(char),             "%d",   "%d"    },  // ImGuiDataType_S8
        { sizeof(unsigned char),    "%u",   "%u"    },
        { sizeof(short),            "%d",   "%d"    },  // ImGuiDataType_S16
        { sizeof(unsigned short),   "%u",   "%u"    },
        { sizeof(int),              "%d",   "%d"    },  // ImGuiDataType_S32
        { sizeof(unsigned int),     "%u",   "%u"    },
    #ifdef _MSC_VER
        { sizeof(ImS64),            "%I64d","%I64d" },  // ImGuiDataType_S64
        { sizeof(ImU64),            "%I64u","%I64u" },
    #else
        { sizeof(ImS64),            "%lld", "%lld"  },  // ImGuiDataType_S64
        { sizeof(ImU64),            "%llu", "%llu"  },
    #endif
        { sizeof(float),            "%f",   "%f"    },  // ImGuiDataType_Float (float are promoted to double in va_arg)
        { sizeof(double),           "%f",   "%lf"   },  // ImGuiDataType_Double
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo) == ImGuiDataType_COUNT);

    // ~65% common code with PatchFormatStringFloatToInt.
    // FIXME-LEGACY: Prior to 1.61 our DragInt() function internally used floats and because of this the compile-time default value for format was "%.0f".
    // Even though we changed the compile-time default, we expect users to have carried %f around, which would break the display of DragInt() calls.
    // To honor backward compatibility we are rewriting the format string, unless IMGUI_DISABLE_OBSOLETE_FUNCTIONS is enabled. What could possibly go wrong?!
    static const char* PatchFormatStringFloatToInt(const char* fmt)
    {
        if (strcmp(fmt, "(%.0f, %.0f)") == 0) // Fast legacy path for "(%.0f, %.0f)" which is expected to be the most common case.
            return "(%d, %d)";

        // Find % (if any, and ignore %%)
        ImGuiContext& g = *GImGui;
        g.TempBuffer[0] = '\0';
        for (const char* fmt_b = fmt; char c = fmt_b[0]; fmt_b++)
        {
            if (c == '%' && fmt_b[1] != '%')
            {
                const char* fmt_start = fmt_b;
                const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
                if (fmt_end > fmt_start&& fmt_end[-1] == 'f')
                {
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
                    ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt_b), fmt_b, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
#else
                    IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "(%.0f, %.0f)", please replace with e.g. "(%d, %d)"
#endif
                }
            }
            else if (c == '%')
                fmt_b++;
        }

        if (g.TempBuffer[0] == '\0')
            return fmt;
        return g.TempBuffer;
    }

    int DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* p_data1, const void* p_data2, const char* format)
    {
        // Signedness doesn't matter when pushing integer arguments
        if (data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_U32)
            return ImFormatString(buf, buf_size, format, *(const ImU32*)p_data1, *(const ImU32*)p_data2);
        if (data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)
            return ImFormatString(buf, buf_size, format, *(const ImU64*)p_data1, *(const ImU64*)p_data2);
        if (data_type == ImGuiDataType_Float)
            return ImFormatString(buf, buf_size, format, *(const float*)p_data1, *(const float*)p_data2);
        if (data_type == ImGuiDataType_Double)
            return ImFormatString(buf, buf_size, format, *(const double*)p_data1, *(const double*)p_data2);
        if (data_type == ImGuiDataType_S8)
            return ImFormatString(buf, buf_size, format, *(const ImS8*)p_data1, *(const ImS8*)p_data2);
        if (data_type == ImGuiDataType_U8)
            return ImFormatString(buf, buf_size, format, *(const ImU8*)p_data1, *(const ImU8*)p_data2);
        if (data_type == ImGuiDataType_S16)
            return ImFormatString(buf, buf_size, format, *(const ImS16*)p_data1, *(const ImS16*)p_data2);
        if (data_type == ImGuiDataType_U16)
            return ImFormatString(buf, buf_size, format, *(const ImU16*)p_data1, *(const ImU16*)p_data2);
        IM_ASSERT(0);
        return 0;
    }

    //-------------------------------------------------------------------------
    // [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
    //-------------------------------------------------------------------------
    // - RangeSliderBehaviorT<>() [Internal]
    // - RangeSliderBehavior() [Internal]
    // - RangeSliderScalar()
    // - RangeSliderFloat()
    // - RangeSliderAngle()
    // - RangeSliderInt()
    //-------------------------------------------------------------------------

    // ~80% common code with ImGui::SliderBehaviorT.
    // FIXME: Move some of the code into SliderBehavior(). Current responsability is larger than what the equivalent DragBehaviorT<> does, we also do some rendering, etc.
    template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
    bool RangeSliderBehaviorT(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v1, TYPE* v2, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab1_bb, ImRect* out_grab2_bb)
    {
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
        const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
        const bool is_power = (power != 1.0f) && is_decimal;

        const float grab_padding = 2.0f;
        const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
        float grab_sz = style.GrabMinSize;
        SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
        if (!is_decimal && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
            grab_sz = ImMax((float)(slider_sz / (v_range + 1)), style.GrabMinSize);  // For integer sliders: if possible have the grab size represent 1 unit
        grab_sz = ImMin(grab_sz, slider_sz);
        const float slider_usable_sz = slider_sz - grab_sz;
        const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
        const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

        // For power curve sliders that cross over sign boundary we want the curve to be symmetric around 0.0f
        float linear_zero_pos;   // 0.0->1.0f
        if (is_power && v_min * v_max < 0.0f)
        {
            // Different sign
            const FLOATTYPE linear_dist_min_to_0 = ImPow(v_min >= 0 ? (FLOATTYPE)v_min : -(FLOATTYPE)v_min, (FLOATTYPE)1.0f / power);
            const FLOATTYPE linear_dist_max_to_0 = ImPow(v_max >= 0 ? (FLOATTYPE)v_max : -(FLOATTYPE)v_max, (FLOATTYPE)1.0f / power);
            linear_zero_pos = (float)(linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0));
        }
        else
        {
            // Same sign
            linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
        }

        // Process interacting with the slider
        bool value_changed = false;
        if (g.ActiveId == id)
        {
            bool set_v_new = false;
            static int lastDrag = -1;
            float clicked_t = 0.0f;
            if (g.ActiveIdSource == ImGuiInputSource_Mouse)
            {
                if (!g.IO.MouseDown[0])
                {
                    ClearActiveID();
                }
                else
                {
                    const float mouse_abs_pos = g.IO.MousePos[axis];
                    clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
                    if (axis == ImGuiAxis_Y)
                        clicked_t = 1.0f - clicked_t;
                    set_v_new = true;
                }
            }
            else if (g.ActiveIdSource == ImGuiInputSource_Nav)
            {
                const ImVec2 delta2 = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
                float delta = (axis == ImGuiAxis_X) ? delta2.x : -delta2.y;
                if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
                {
                    ClearActiveID();
                }
                else if (delta != 0.0f)
                {
                    clicked_t = SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v1, v_min, v_max, power, linear_zero_pos);
                    const int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
                    if ((decimal_precision > 0) || is_power)
                    {
                        delta /= 100.0f;    // Gamepad/keyboard tweak speeds in % of slider bounds
                        if (IsNavInputDown(ImGuiNavInput_TweakSlow))
                            delta /= 10.0f;
                    }
                    else
                    {
                        if ((v_range >= -100.0f && v_range <= 100.0f) || IsNavInputDown(ImGuiNavInput_TweakSlow))
                            delta = ((delta < 0.0f) ? -1.0f : +1.0f) / (float)v_range; // Gamepad/keyboard tweak speeds in integer steps
                        else
                            delta /= 100.0f;
                    }
                    if (IsNavInputDown(ImGuiNavInput_TweakFast))
                        delta *= 10.0f;
                    set_v_new = true;
                    if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
                        set_v_new = false;
                    else
                        clicked_t = ImSaturate(clicked_t + delta);
                }
            }

            if (set_v_new)
            {
                TYPE v_new;
                if (is_power)
                {
                    // Account for power curve scale on both sides of the zero
                    if (clicked_t < linear_zero_pos)
                    {
                        // Negative: rescale to the negative range before powering
                        float a = 1.0f - (clicked_t / linear_zero_pos);
                        a = ImPow(a, power);
                        v_new = ImLerp(ImMin(v_max, (TYPE)0), v_min, a);
                    }
                    else
                    {
                        // Positive: rescale to the positive range before powering
                        float a;
                        if (ImFabs(linear_zero_pos - 1.0f) > 1.e-6f)
                            a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
                        else
                            a = clicked_t;
                        a = ImPow(a, power);
                        v_new = ImLerp(ImMax(v_min, (TYPE)0), v_max, a);
                    }
                }
                else
                {
                    // Linear slider
                    if (is_decimal)
                    {
                        v_new = ImLerp(v_min, v_max, clicked_t);
                    }
                    else
                    {
                        // For integer values we want the clicking position to match the grab box so we round above
                        // This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
                        FLOATTYPE v_new_off_f = (v_max - v_min) * clicked_t;
                        TYPE v_new_off_floor = (TYPE)(v_new_off_f);
                        TYPE v_new_off_round = (TYPE)(v_new_off_f + (FLOATTYPE)0.5);
                        if (v_new_off_floor < v_new_off_round)
                            v_new = v_min + v_new_off_round;
                        else
                            v_new = v_min + v_new_off_floor;
                    }
                }

                // Round to user desired precision based on format string
                v_new = RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_new);

                // Apply result
                if (*v1 != v_new || *v2 != v_new)
                {
                    if (lastDrag == 1 || lastDrag == 2)
                    {
                        if (*v2 == v_new)
                        {
                            *v1 = v_new;
                            lastDrag = 2; //left to right, just hit same spot
                        }
                    }
                    else if (lastDrag == 0 || lastDrag == 3)
                    {
                        if (*v1 == v_new)
                        {
                            *v2 = v_new;
                            lastDrag = 3; //right to left, just hit left spot
                        }
                    }
                    if (ImFabs(*v1 - v_new) == ImFabs(*v2 - v_new))
                    {
                        if (*v1 == *v2)
                        {
                            if (*v1 != v_new)
                            {
                                if (v_new > * v1)
                                {
                                    *v2 = v_new;
                                }
                                else
                                {
                                    *v1 = v_new;
                                }
                            }
                        }
                        //else
                        if (lastDrag == 1)
                        {
                            *v1 = v_new;
                        }
                        else if (lastDrag == 0)
                        {
                            *v2 = v_new;
                        }

                    }
                    else if (ImFabs(*v1 - v_new) < ImFabs(*v2 - v_new))
                    {
                        *v1 = v_new;
                        lastDrag = 1;
                    }
                    else
                    {
                        *v2 = v_new;
                        lastDrag = 0;
                    }
                    value_changed = true;
                }
            }
        }

        if (slider_sz < 1.0f)
        {
            *out_grab1_bb = ImRect(bb.Min, bb.Min);
            *out_grab2_bb = ImRect(bb.Min, bb.Min);
        }
        else
        {
            // Output grab1 position so it can be displayed by the caller
            float grab1_t = SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v1, v_min, v_max, power, linear_zero_pos);
            if (axis == ImGuiAxis_Y)
                grab1_t = 1.0f - grab1_t;
            const float grab1_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab1_t);
            if (axis == ImGuiAxis_X)
                *out_grab1_bb = ImRect(grab1_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab1_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
            else
                *out_grab1_bb = ImRect(bb.Min.x + grab_padding, grab1_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab1_pos + grab_sz * 0.5f);

            // Output grab2 position so it can be displayed by the caller
            float grab2_t = SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v2, v_min, v_max, power, linear_zero_pos);
            if (axis == ImGuiAxis_Y)
                grab2_t = 1.0f - grab2_t;
            const float grab2_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab2_t);
            if (axis == ImGuiAxis_X)
                *out_grab2_bb = ImRect(grab2_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab2_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
            else
                *out_grab2_bb = ImRect(bb.Min.x + grab_padding, grab2_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab2_pos + grab_sz * 0.5f);
        }

        return value_changed;
    }

    // ~95% common code with ImGui::SliderBehavior.
    // For 32-bit and larger types, slider bounds are limited to half the natural type range.
    // So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
    // It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
    bool RangeSliderBehavior(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* p_v1, void* p_v2, const void* p_min, const void* p_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab1_bb, ImRect* out_grab2_bb)
    {
        switch (data_type)
        {
        case ImGuiDataType_S8:
        {
            ImS32 v32_1 = (ImS32) * (ImS8*)p_v1;
            ImS32 v32_2 = (ImS32) * (ImS8*)p_v2;
            bool r = RangeSliderBehaviorT<ImS32, ImS32, float>(bb, id, ImGuiDataType_S32, &v32_1, &v32_2, *(const ImS8*)p_min, *(const ImS8*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
            if (r)
            {
                *(ImS8*)p_v1 = (ImS8)v32_1;
                *(ImS8*)p_v2 = (ImS8)v32_2;
            }
            return r;
        }
        case ImGuiDataType_U8:
        {
            ImU32 v32_1 = (ImU32) * (ImU8*)p_v1;
            ImU32 v32_2 = (ImU32) * (ImU8*)p_v2;
            bool r = RangeSliderBehaviorT<ImU32, ImS32, float>(bb, id, ImGuiDataType_U32, &v32_1, &v32_2, *(const ImU8*)p_min, *(const ImU8*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
            if (r)
            {
                *(ImU8*)p_v1 = (ImU8)v32_1;
                *(ImU8*)p_v2 = (ImU8)v32_2;
            }
            return r;
        }
        case ImGuiDataType_S16:
        {
            ImS32 v32_1 = (ImS32) * (ImS16*)p_v1;
            ImS32 v32_2 = (ImS32) * (ImS16*)p_v2;
            bool r = RangeSliderBehaviorT<ImS32, ImS32, float>(bb, id, ImGuiDataType_S32, &v32_1, &v32_2, *(const ImS16*)p_min, *(const ImS16*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
            if (r)
            {
                *(ImS16*)p_v1 = (ImS16)v32_1;
                *(ImS16*)p_v2 = (ImS16)v32_2;
            }
            return r;
        }
        case ImGuiDataType_U16:
        {
            ImU32 v32_1 = (ImU32) * (ImU16*)p_v1;
            ImU32 v32_2 = (ImU32) * (ImU16*)p_v2;
            bool r = RangeSliderBehaviorT<ImU32, ImS32, float>(bb, id, ImGuiDataType_U32, &v32_1, &v32_2, *(const ImU16*)p_min, *(const ImU16*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
            if (r)
            {
                *(ImU16*)p_v1 = (ImU16)v32_1;
                *(ImU16*)p_v2 = (ImU16)v32_2;
            }
            return r;
        }
        case ImGuiDataType_S32:
            IM_ASSERT(*(const ImS32*)p_min >= IM_S32_MIN / 2 && *(const ImS32*)p_max <= IM_S32_MAX / 2);
            return RangeSliderBehaviorT<ImS32, ImS32, float >(bb, id, data_type, (ImS32*)p_v1, (ImS32*)p_v2, *(const ImS32*)p_min, *(const ImS32*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
        case ImGuiDataType_U32:
            IM_ASSERT(*(const ImU32*)p_max <= IM_U32_MAX / 2);
            return RangeSliderBehaviorT<ImU32, ImS32, float >(bb, id, data_type, (ImU32*)p_v1, (ImU32*)p_v2, *(const ImU32*)p_min, *(const ImU32*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
        case ImGuiDataType_S64:
            IM_ASSERT(*(const ImS64*)p_min >= IM_S64_MIN / 2 && *(const ImS64*)p_max <= IM_S64_MAX / 2);
            return RangeSliderBehaviorT<ImS64, ImS64, double>(bb, id, data_type, (ImS64*)p_v1, (ImS64*)p_v2, *(const ImS64*)p_min, *(const ImS64*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
        case ImGuiDataType_U64:
            IM_ASSERT(*(const ImU64*)p_max <= IM_U64_MAX / 2);
            return RangeSliderBehaviorT<ImU64, ImS64, double>(bb, id, data_type, (ImU64*)p_v1, (ImU64*)p_v2, *(const ImU64*)p_min, *(const ImU64*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
        case ImGuiDataType_Float:
            IM_ASSERT(*(const float*)p_min >= -FLT_MAX / 2.0f && *(const float*)p_max <= FLT_MAX / 2.0f);
            return RangeSliderBehaviorT<float, float, float >(bb, id, data_type, (float*)p_v1, (float*)p_v2, *(const float*)p_min, *(const float*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
        case ImGuiDataType_Double:
            IM_ASSERT(*(const double*)p_min >= -DBL_MAX / 2.0f && *(const double*)p_max <= DBL_MAX / 2.0f);
            return RangeSliderBehaviorT<double, double, double>(bb, id, data_type, (double*)p_v1, (double*)p_v2, *(const double*)p_min, *(const double*)p_max, format, power, flags, out_grab1_bb, out_grab2_bb);
        case ImGuiDataType_COUNT: break;
        }
        IM_ASSERT(0);
        return false;
    }

    // ~95% common code with ImGui::SliderScalar
    // Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a slider, they are all required.
    // Read code of e.g. SliderFloat(), SliderInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
    bool ImGui::RangeSliderScalar(const char* label, ImGuiDataType data_type, void* p_data1, void* p_data2, const void* p_min, const void* p_max, const char* format, float power)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const float w = CalcItemWidth();

        const ImVec2 label_size = CalcTextSize(label, NULL, true);
        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb))
            return false;

        // Default format string when passing NULL
        if (format == NULL)
            format = (std::string("(") + DataTypeGetInfo(data_type)->PrintFmt + ", " + DataTypeGetInfo(data_type)->PrintFmt + ")").c_str();
        else if (data_type == ImGuiDataType_S32 && strcmp(format, "(%d, %d)") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
            format = PatchFormatStringFloatToInt(format);

        // Tabbing or CTRL-clicking on Slider turns it into an input box
        const bool hovered = ItemHoverable(frame_bb, id);
        bool temp_input_is_active = TempInputTextIsActive(id);
        bool temp_input_start = false;
        if (!temp_input_is_active)
        {
            //const bool focus_requested = FocusableItemRegister(window, id);
            const bool focus_requested = FocusableItemRegister(window, g.ActiveId == id);
            const bool clicked = (hovered && g.IO.MouseClicked[0]);
            if (focus_requested || clicked || g.NavActivateId == id || g.NavInputId == id)
            {
                SetActiveID(id, window);
                SetFocusID(id, window);
                FocusWindow(window);
                g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
                if (focus_requested || (clicked && g.IO.KeyCtrl) || g.NavInputId == id)
                {
                    temp_input_start = true;
                    FocusableItemUnregister(window);
                }
            }
        }
        if (temp_input_is_active || temp_input_start)
            return TempInputTextScalar(frame_bb, id, label, data_type, p_data1, format);

        // Draw frame
        const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

        // Slider behavior
        ImRect grab_bb1, grab_bb2;
        const bool value_changed = RangeSliderBehavior(frame_bb, id, data_type, p_data1, p_data2, p_min, p_max, format, power, ImGuiSliderFlags_None, &grab_bb1, &grab_bb2);
        if (value_changed)
            MarkItemEdited(id);

        // Render grabs
        if (grab_bb1.Max.x > grab_bb1.Min.x)
            window->DrawList->AddRectFilled(grab_bb1.Min, grab_bb1.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        if (grab_bb2.Max.x > grab_bb2.Min.x)
            window->DrawList->AddRectFilled(grab_bb2.Min, grab_bb2.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        // Render connector
        ImRect connector(grab_bb1.Min, grab_bb2.Max);
        float grab_sz = grab_bb1.Max.x - grab_bb1.Min.x;
        connector.Min.x += grab_sz;
        connector.Min.y += grab_sz * 0.3f;
        connector.Max.x -= grab_sz;
        connector.Max.y -= grab_sz * 0.3f;
        window->DrawList->AddRectFilled(connector.Min, connector.Max, GetColorU32(ImGuiCol_RangeSliderBar), style.GrabRounding);

        // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
        char value_buf[64];
        const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data1, p_data2, format);
        RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

        if (label_size.x > 0.0f)
            RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
        return value_changed;
    }

    // ~95% common code with ImGui::SliderScalarN
    // Add multiple sliders on 1 line for compact edition of multiple components
    bool ImGui::RangeSliderScalarN(const char* label, ImGuiDataType data_type, void* v1, void* v2, int components, const void* v_min, const void* v_max, const char* format, float power)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        bool value_changed = false;
        BeginGroup();
        PushID(label);
        PushMultiItemsWidths(components, CalcItemWidth());
        size_t type_size = GDataTypeInfo[data_type].Size;
        for (int i = 0; i < components; i++)
        {
            PushID(i);
            if (i > 0)
                SameLine(0, g.Style.ItemInnerSpacing.x);
            value_changed |= RangeSliderScalar("", data_type, v1, v2, v_min, v_max, format, power);
            PopID();
            PopItemWidth();
            v1 = (void*)((char*)v1 + type_size);
            v2 = (void*)((char*)v2 + type_size);
        }
        PopID();

        const char* label_end = FindRenderedTextEnd(label);
        if (label != label_end)
        {
            SameLine(0, g.Style.ItemInnerSpacing.x);
            TextEx(label, label_end);
        }

        EndGroup();
        return value_changed;
    }

    // ~95% common code with ImGui::SliderFloat
    bool ImGui::RangeSliderFloat(const char* label, float* v1, float* v2, float v_min, float v_max, const char* format, float power)
    {
        return RangeSliderScalar(label, ImGuiDataType_Float, v1, v2, &v_min, &v_max, format, power);
    }

    // ~95% common code with ImGui::SliderFloat2
    bool ImGui::RangeSliderFloat2(const char* label, float v1[2], float v2[2], float v_min, float v_max, const char* format, float power)
    {
        return RangeSliderScalarN(label, ImGuiDataType_Float, v1, v2, 2, &v_min, &v_max, format, power);
    }

    // ~95% common code with ImGui::SliderFloat3
    bool ImGui::RangeSliderFloat3(const char* label, float v1[3], float v2[3], float v_min, float v_max, const char* format, float power)
    {
        return RangeSliderScalarN(label, ImGuiDataType_Float, v1, v2, 3, &v_min, &v_max, format, power);
    }

    // ~95% common code with ImGui::SliderFloat4
    bool ImGui::RangeSliderFloat4(const char* label, float v1[4], float v2[4], float v_min, float v_max, const char* format, float power)
    {
        return RangeSliderScalarN(label, ImGuiDataType_Float, v1, v2, 4, &v_min, &v_max, format, power);
    }

    // ~95% common code with ImGui::SliderAngle
    bool ImGui::RangeSliderAngle(const char* label, float* v_rad1, float* v_rad2, float v_degrees_min, float v_degrees_max, const char* format)
    {
        if (format == NULL)
            format = "%d deg";
        float v_deg1 = (*v_rad1) * 360.0f / (2 * IM_PI);
        float v_deg2 = (*v_rad2) * 360.0f / (2 * IM_PI);
        bool value_changed = RangeSliderFloat(label, &v_deg2, &v_deg1, v_degrees_min, v_degrees_max, format, 1.0f);
        *v_rad1 = v_deg1 * (2 * IM_PI) / 360.0f;
        *v_rad2 = v_deg2 * (2 * IM_PI) / 360.0f;
        return value_changed;
    }

    // ~95% common code with ImGui::SliderInt
    bool ImGui::RangeSliderInt(const char* label, int* v1, int* v2, int v_min, int v_max, const char* format)
    {
        return RangeSliderScalar(label, ImGuiDataType_S32, v1, v2, &v_min, &v_max, format);
    }

    // ~95% common code with ImGui::SliderInt2
    bool ImGui::RangeSliderInt2(const char* label, int v1[2], int v2[2], int v_min, int v_max, const char* format)
    {
        return RangeSliderScalarN(label, ImGuiDataType_S32, v1, v2, 2, &v_min, &v_max, format);
    }

    // ~95% common code with ImGui::SliderInt3
    bool ImGui::RangeSliderInt3(const char* label, int v1[3], int v2[3], int v_min, int v_max, const char* format)
    {
        return RangeSliderScalarN(label, ImGuiDataType_S32, v1, v2, 3, &v_min, &v_max, format);
    }

    // ~95% common code with ImGui::SliderInt4
    bool ImGui::RangeSliderInt4(const char* label, int v1[4], int v2[4], int v_min, int v_max, const char* format)
    {
        return RangeSliderScalarN(label, ImGuiDataType_S32, v1, v2, 4, &v_min, &v_max, format);
    }

    bool ImGui::RangeVSliderScalar(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data1, void* p_data2, const void* p_min, const void* p_max, const char* format, float power)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        const ImVec2 label_size = CalcTextSize(label, NULL, true);
        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
        const ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(frame_bb, id))
            return false;

        // Default format string when passing NULL
        if (format == NULL)
            format = DataTypeGetInfo(data_type)->PrintFmt;
        else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
            format = PatchFormatStringFloatToInt(format);

        const bool hovered = ItemHoverable(frame_bb, id);
        if ((hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || g.NavInputId == id)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
        }

        // Draw frame
        const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

        // Slider behavior
        ImRect grab_bb1, grab_bb2;
        const bool value_changed = RangeSliderBehavior(frame_bb, id, data_type, p_data1, p_data2, p_min, p_max, format, power, ImGuiSliderFlags_Vertical, &grab_bb1, &grab_bb2);
        if (value_changed)
            MarkItemEdited(id);

        // Render grabs
        if (grab_bb1.Max.y > grab_bb1.Min.y)
            window->DrawList->AddRectFilled(grab_bb1.Min, grab_bb1.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        if (grab_bb2.Max.y > grab_bb2.Min.y)
            window->DrawList->AddRectFilled(grab_bb2.Min, grab_bb2.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        // Render connector
        ImRect connector(grab_bb1.Min, grab_bb2.Max);
        float grab_sz = grab_bb1.Max.x - grab_bb1.Min.x;
        connector.Min.x += grab_sz;
        connector.Min.y += grab_sz * 0.3f;
        connector.Max.x -= grab_sz;
        connector.Max.y -= grab_sz * 0.3f;
        window->DrawList->AddRectFilled(connector.Min, connector.Max, GetColorU32(ImGuiCol_RangeSliderBar), style.GrabRounding);

        // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
        // For the vertical slider we allow centered text to overlap the frame padding
        char value_buf[64];
        const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data1, p_data2, format);
        RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.0f));
        if (label_size.x > 0.0f)
            RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

        return value_changed;
    }

    bool ImGui::RangeVSliderFloat(const char* label, const ImVec2& size, float* v1, float* v2, float v_min, float v_max, const char* format, float power)
    {
        return RangeVSliderScalar(label, size, ImGuiDataType_Float, v1, v2, &v_min, &v_max, format, power);
    }

    bool ImGui::RangeVSliderInt(const char* label, const ImVec2& size, int* v1, int* v2, int v_min, int v_max, const char* format)
    {
        return RangeVSliderScalar(label, size, ImGuiDataType_S32, v1, v2, &v_min, &v_max, format);
    }
} // namespace ImGui