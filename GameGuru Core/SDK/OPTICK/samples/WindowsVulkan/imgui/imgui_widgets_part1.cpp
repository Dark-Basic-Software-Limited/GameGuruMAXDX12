bool ImGui::DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    PushID(label);
    BeginGroup();
    PushMultiItemsWidths(2);

    bool value_changed = DragInt("##min", v_current_min, v_speed, (v_min >= v_max) ? INT_MIN : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);
    value_changed |= DragInt("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? INT_MAX : v_max, format_max ? format_max : format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    TextUnformatted(label, FindRenderedTextEnd(label));
    EndGroup();
    PopID();

    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
//-------------------------------------------------------------------------
// - SliderBehaviorT<>() [Internal]
// - SliderBehavior() [Internal]
// - SliderScalar()
// - SliderScalarN()
// - SliderFloat()
// - SliderFloat2()
// - SliderFloat3()
// - SliderFloat4()
// - SliderAngle()
// - SliderInt()
// - SliderInt2()
// - SliderInt3()
// - SliderInt4()
// - VSliderScalar()
// - VSliderFloat()
// - VSliderInt()
//-------------------------------------------------------------------------

template<typename TYPE, typename FLOATTYPE>
float ImGui::SliderCalcRatioFromValueT(ImGuiDataType data_type, TYPE v, TYPE v_min, TYPE v_max, float power, float linear_zero_pos)
{
    if (v_min == v_max)
        return 0.0f;

    const bool is_power = (power != 1.0f) && (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double);
    const TYPE v_clamped = (v_min < v_max) ? ImClamp(v, v_min, v_max) : ImClamp(v, v_max, v_min);
    if (is_power)
    {
        if (v_clamped < 0.0f)
        {
            const float f = 1.0f - (float)((v_clamped - v_min) / (ImMin((TYPE)0, v_max) - v_min));
            return (1.0f - ImPow(f, 1.0f/power)) * linear_zero_pos;
        }
        else
        {
            const float f = (float)((v_clamped - ImMax((TYPE)0, v_min)) / (v_max - ImMax((TYPE)0, v_min)));
            return linear_zero_pos + ImPow(f, 1.0f/power) * (1.0f - linear_zero_pos);
        }
    }

    // Linear slider
    return (float)((FLOATTYPE)(v_clamped - v_min) / (FLOATTYPE)(v_max - v_min));
}

// FIXME: Move some of the code into SliderBehavior(). Current responsability is larger than what the equivalent DragBehaviorT<> does, we also do some rendering, etc.
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool ImGui::SliderBehaviorT(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const bool is_horizontal = (flags & ImGuiSliderFlags_Vertical) == 0;
    const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
    const bool is_power = (power != 1.0f) && is_decimal;

    const float grab_padding = 2.0f;
    const float slider_sz = is_horizontal ? (bb.GetWidth() - grab_padding * 2.0f) : (bb.GetHeight() - grab_padding * 2.0f);
    float grab_sz = style.GrabMinSize;
    SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
    if (!is_decimal && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
        grab_sz = ImMax((float)(slider_sz / (v_range + 1)), style.GrabMinSize);  // For integer sliders: if possible have the grab size represent 1 unit
    grab_sz = ImMin(grab_sz, slider_sz);
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = (is_horizontal ? bb.Min.x : bb.Min.y) + grab_padding + grab_sz*0.5f;
    const float slider_usable_pos_max = (is_horizontal ? bb.Max.x : bb.Max.y) - grab_padding - grab_sz*0.5f;

    // For power curve sliders that cross over sign boundary we want the curve to be symmetric around 0.0f
    float linear_zero_pos;   // 0.0->1.0f
    if (is_power && v_min * v_max < 0.0f)
    {
        // Different sign
        const FLOATTYPE linear_dist_min_to_0 = ImPow(v_min >= 0 ? (FLOATTYPE)v_min : -(FLOATTYPE)v_min, (FLOATTYPE)1.0f/power);
        const FLOATTYPE linear_dist_max_to_0 = ImPow(v_max >= 0 ? (FLOATTYPE)v_max : -(FLOATTYPE)v_max, (FLOATTYPE)1.0f/power);
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
        bool set_new_value = false;
        float clicked_t = 0.0f;
        if (g.ActiveIdSource == ImGuiInputSource_Mouse)
        {
            if (!g.IO.MouseDown[0])
            {
                ClearActiveID();
            }
            else
            {
                const float mouse_abs_pos = is_horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
                clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
                if (!is_horizontal)
                    clicked_t = 1.0f - clicked_t;
                set_new_value = true;
            }
        }
        else if (g.ActiveIdSource == ImGuiInputSource_Nav)
        {
            const ImVec2 delta2 = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
            float delta = is_horizontal ? delta2.x : -delta2.y;
            if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            {
                ClearActiveID();
            }
            else if (delta != 0.0f)
            {
                clicked_t = SliderCalcRatioFromValueT<TYPE,FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
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
                set_new_value = true;
                if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
                    set_new_value = false;
                else
                    clicked_t = ImSaturate(clicked_t + delta);
            }
        }

        if (set_new_value)
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
                    if (!is_decimal && v_new_off_floor < v_new_off_round)
                        v_new = v_min + v_new_off_round;
                    else
                        v_new = v_min + v_new_off_floor;
                }
            }

            // Round to user desired precision based on format string
            v_new = RoundScalarWithFormatT<TYPE,SIGNEDTYPE>(format, data_type, v_new);

            // Apply result
            if (*v != v_new)
            {
                *v = v_new;
                value_changed = true;
            }
        }
    }

    // Output grab position so it can be displayed by the caller
    float grab_t = SliderCalcRatioFromValueT<TYPE,FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
    if (!is_horizontal)
        grab_t = 1.0f - grab_t;
    const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
    if (is_horizontal)
        *out_grab_bb = ImRect(grab_pos - grab_sz*0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz*0.5f, bb.Max.y - grab_padding);
    else
        *out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz*0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz*0.5f);

    return value_changed;
}

// For 32-bits and larger types, slider bounds are limited to half the natural type range.
// So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
// It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
bool ImGui::SliderBehavior(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    switch (data_type)
    {
    case ImGuiDataType_S32:
        IM_ASSERT(*(const ImS32*)v_min >= IM_S32_MIN/2 && *(const ImS32*)v_max <= IM_S32_MAX/2);
        return SliderBehaviorT<ImS32, ImS32, float >(bb, id, data_type, (ImS32*)v,  *(const ImS32*)v_min,  *(const ImS32*)v_max,  format, power, flags, out_grab_bb);
    case ImGuiDataType_U32:
        IM_ASSERT(*(const ImU32*)v_min <= IM_U32_MAX/2);
        return SliderBehaviorT<ImU32, ImS32, float >(bb, id, data_type, (ImU32*)v,  *(const ImU32*)v_min,  *(const ImU32*)v_max,  format, power, flags, out_grab_bb);
    case ImGuiDataType_S64:
        IM_ASSERT(*(const ImS64*)v_min >= IM_S64_MIN/2 && *(const ImS64*)v_max <= IM_S64_MAX/2);
        return SliderBehaviorT<ImS64, ImS64, double>(bb, id, data_type, (ImS64*)v,  *(const ImS64*)v_min,  *(const ImS64*)v_max,  format, power, flags, out_grab_bb);
    case ImGuiDataType_U64:
        IM_ASSERT(*(const ImU64*)v_min <= IM_U64_MAX/2);
        return SliderBehaviorT<ImU64, ImS64, double>(bb, id, data_type, (ImU64*)v,  *(const ImU64*)v_min,  *(const ImU64*)v_max,  format, power, flags, out_grab_bb);
    case ImGuiDataType_Float:
        IM_ASSERT(*(const float*)v_min >= -FLT_MAX/2.0f && *(const float*)v_max <= FLT_MAX/2.0f);
        return SliderBehaviorT<float, float, float >(bb, id, data_type, (float*)v,  *(const float*)v_min,  *(const float*)v_max,  format, power, flags, out_grab_bb);
    case ImGuiDataType_Double:
        IM_ASSERT(*(const double*)v_min >= -DBL_MAX/2.0f && *(const double*)v_max <= DBL_MAX/2.0f);
        return SliderBehaviorT<double,double,double>(bb, id, data_type, (double*)v, *(const double*)v_min, *(const double*)v_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_COUNT: break;
    }
    IM_ASSERT(0);
    return false;
}

bool ImGui::SliderScalar(const char* label, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    // NB- we don't call ItemSize() yet because we may turn into a text edit box below
    if (!ItemAdd(total_bb, id, &frame_bb))
    {
        ItemSize(total_bb, style.FramePadding.y);
        return false;
    }

    // Default format string when passing NULL
    // Patch old "%.0f" format string to use "%d", read function comments for more details.
    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    if (format == NULL)
        format = GDataTypeInfo[data_type].PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
        format = PatchFormatStringFloatToInt(format);

    // Tabbing or CTRL-clicking on Slider turns it into an input box
    bool start_text_input = false;
    const bool tab_focus_requested = FocusableItemRegister(window, id);
    const bool hovered = ItemHoverable(frame_bb, id);
    if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
        if (tab_focus_requested || g.IO.KeyCtrl || g.NavInputId == id)
        {
            start_text_input = true;
            g.ScalarAsInputTextId = 0;
        }
    }
    if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
        return InputScalarAsWidgetReplacement(frame_bb, id, label, data_type, v, format);

    ItemSize(total_bb, style.FramePadding.y);

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_None, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f,0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    return value_changed;
}

// Add multiple sliders on 1 line for compact edition of multiple components
bool ImGui::SliderScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components);
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        value_changed |= SliderScalar("##v", data_type, v, v_min, v_max, format, power);
        SameLine(0, g.Style.ItemInnerSpacing.x);
        PopID();
        PopItemWidth();
        v = (void*)((char*)v + type_size);
    }
    PopID();

    TextUnformatted(label, FindRenderedTextEnd(label));
    EndGroup();
    return value_changed;
}

bool ImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool ImGui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
{
    return SliderScalarN(label, ImGuiDataType_Float, v, 2, &v_min, &v_max, format, power);
}

bool ImGui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
{
    return SliderScalarN(label, ImGuiDataType_Float, v, 3, &v_min, &v_max, format, power);
}

bool ImGui::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
{
    return SliderScalarN(label, ImGuiDataType_Float, v, 4, &v_min, &v_max, format, power);
}

bool ImGui::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max)
{
    float v_deg = (*v_rad) * 360.0f / (2*IM_PI);
    bool value_changed = SliderFloat(label, &v_deg, v_degrees_min, v_degrees_max, "%.0f deg", 1.0f);
    *v_rad = v_deg * (2*IM_PI) / 360.0f;
    return value_changed;
}

bool ImGui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format);
}

bool ImGui::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
{
    return SliderScalarN(label, ImGuiDataType_S32, v, 2, &v_min, &v_max, format);
}

bool ImGui::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
{
    return SliderScalarN(label, ImGuiDataType_S32, v, 3, &v_min, &v_max, format);
}

bool ImGui::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
{
    return SliderScalarN(label, ImGuiDataType_S32, v, 4, &v_min, &v_max, format);
}

bool ImGui::VSliderScalar(const char* label, const ImVec2& size, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
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
    // Patch old "%.0f" format string to use "%d", read function comments for more details.
    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    if (format == NULL)
        format = GDataTypeInfo[data_type].PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
        format = PatchFormatStringFloatToInt(format);

    const bool hovered = ItemHoverable(frame_bb, id);
    if ((hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || g.NavInputId == id)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_Vertical, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    // For the vertical slider we allow centered text to overlap the frame padding
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
    RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f,0.0f));
    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    return value_changed;
}

bool ImGui::VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, float power)
{
    return VSliderScalar(label, size, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool ImGui::VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format)
{
    return VSliderScalar(label, size, ImGuiDataType_S32, v, &v_min, &v_max, format);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
//-------------------------------------------------------------------------
// - ImParseFormatFindStart() [Internal]
// - ImParseFormatFindEnd() [Internal]
// - ImParseFormatTrimDecorations() [Internal]
// - ImParseFormatPrecision() [Internal]
// - InputScalarAsWidgetReplacement() [Internal]
// - InputScalar()
// - InputScalarN()
// - InputFloat()
// - InputFloat2()
// - InputFloat3()
// - InputFloat4()
// - InputInt()
// - InputInt2()
// - InputInt3()
// - InputInt4()
// - InputDouble()
//-------------------------------------------------------------------------

// We don't use strchr() because our strings are usually very short and often start with '%'
const char* ImParseFormatFindStart(const char* fmt)
{
    while (char c = fmt[0])
    {
        if (c == '%' && fmt[1] != '%')
            return fmt;
        else if (c == '%')
            fmt++;
        fmt++;
    }
    return fmt;
}

const char* ImParseFormatFindEnd(const char* fmt)
{
    // Printf/scanf types modifiers: I/L/h/j/l/t/w/z. Other uppercase letters qualify as types aka end of the format.
    if (fmt[0] != '%')
        return fmt;
    const unsigned int ignored_uppercase_mask = (1 << ('I'-'A')) | (1 << ('L'-'A'));
    const unsigned int ignored_lowercase_mask = (1 << ('h'-'a')) | (1 << ('j'-'a')) | (1 << ('l'-'a')) | (1 << ('t'-'a')) | (1 << ('w'-'a')) | (1 << ('z'-'a'));
    for (char c; (c = *fmt) != 0; fmt++)
    {
        if (c >= 'A' && c <= 'Z' && ((1 << (c - 'A')) & ignored_uppercase_mask) == 0)
            return fmt + 1;
        if (c >= 'a' && c <= 'z' && ((1 << (c - 'a')) & ignored_lowercase_mask) == 0)
            return fmt + 1;
    }
    return fmt;
}

// Extract the format out of a format string with leading or trailing decorations
//  fmt = "blah blah"  -> return fmt
//  fmt = "%.3f"       -> return fmt
//  fmt = "hello %.3f" -> return fmt + 6
//  fmt = "%.3f hello" -> return buf written with "%.3f"
const char* ImParseFormatTrimDecorations(const char* fmt, char* buf, int buf_size)
{
    const char* fmt_start = ImParseFormatFindStart(fmt);
    if (fmt_start[0] != '%')
        return fmt;
    const char* fmt_end = ImParseFormatFindEnd(fmt_start);
    if (fmt_end[0] == 0) // If we only have leading decoration, we don't need to copy the data.
        return fmt_start;
    ImStrncpy(buf, fmt_start, ImMin((int)(fmt_end + 1 - fmt_start), buf_size));
    return buf;
}

// Parse display precision back from the display format string
// FIXME: This is still used by some navigation code path to infer a minimum tweak step, but we should aim to rework widgets so it isn't needed.
int ImParseFormatPrecision(const char* fmt, int default_precision)
{
    fmt = ImParseFormatFindStart(fmt);
    if (fmt[0] != '%')
        return default_precision;
    fmt++;
    while (*fmt >= '0' && *fmt <= '9')
        fmt++;
    int precision = INT_MAX;
    if (*fmt == '.')
    {
        fmt = ImAtoi<int>(fmt + 1, &precision);
        if (precision < 0 || precision > 99)
            precision = default_precision;
    }
    if (*fmt == 'e' || *fmt == 'E') // Maximum precision with scientific notation
        precision = -1;
    if ((*fmt == 'g' || *fmt == 'G') && precision == INT_MAX)
        precision = -1;
    return (precision == INT_MAX) ? default_precision : precision;
}

// Create text input in place of a slider (when CTRL+Clicking on slider)
// FIXME: Logic is messy and confusing.
bool ImGui::InputScalarAsWidgetReplacement(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* data_ptr, const char* format)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Our replacement widget will override the focus ID (registered previously to allow for a TAB focus to happen)
    // On the first frame, g.ScalarAsInputTextId == 0, then on subsequent frames it becomes == id
    SetActiveID(g.ScalarAsInputTextId, window);
    g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
    SetHoveredID(0);
    FocusableItemUnregister(window);

    char fmt_buf[32];
    char data_buf[32];
    format = ImParseFormatTrimDecorations(format, fmt_buf, IM_ARRAYSIZE(fmt_buf));
    DataTypeFormatString(data_buf, IM_ARRAYSIZE(data_buf), data_type, data_ptr, format);
    ImStrTrimBlanks(data_buf);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ((data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double) ? ImGuiInputTextFlags_CharsScientific : ImGuiInputTextFlags_CharsDecimal);
    bool value_changed = InputTextEx(label, data_buf, IM_ARRAYSIZE(data_buf), bb.GetSize(), flags);
    if (g.ScalarAsInputTextId == 0)     // First frame we started displaying the InputText widget
    {
        IM_ASSERT(g.ActiveId == id);    // InputText ID expected to match the Slider ID
        g.ScalarAsInputTextId = g.ActiveId;
        SetHoveredID(id);
    }
    if (value_changed)
        return DataTypeApplyOpFromText(data_buf, g.InputTextState.InitialText.Data, data_type, data_ptr, NULL);
    return false;
}

// NB: format here must be a simple "%xx" format string with no prefix/suffix (unlike the Drag/Slider functions "format" argument)
bool ImGui::InputScalar(const char* label, ImGuiDataType data_type, void* data_ptr, const void* step, const void* step_fast, const char* format, ImGuiInputTextFlags extra_flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    if (format == NULL)
        format = GDataTypeInfo[data_type].PrintFmt;

    char buf[64];
    DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, data_ptr, format);

    bool value_changed = false;
    if ((extra_flags & (ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsScientific)) == 0)
        extra_flags |= ImGuiInputTextFlags_CharsDecimal;
    extra_flags |= ImGuiInputTextFlags_AutoSelectAll;

    if (step != NULL)
    {
        const float button_size = GetFrameHeight();

        BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
        PushID(label);
        PushItemWidth(ImMax(1.0f, CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
        if (InputText("", buf, IM_ARRAYSIZE(buf), extra_flags)) // PushId(label) + "" gives us the expected ID from outside point of view
            value_changed = DataTypeApplyOpFromText(buf, g.InputTextState.InitialText.Data, data_type, data_ptr, format);
        PopItemWidth();

        // Step buttons
        SameLine(0, style.ItemInnerSpacing.x);
        if (ButtonEx("-", ImVec2(button_size, button_size), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups))
        {
            DataTypeApplyOp(data_type, '-', data_ptr, data_ptr, g.IO.KeyCtrl && step_fast ? step_fast : step);
            value_changed = true;
        }
        SameLine(0, style.ItemInnerSpacing.x);
        if (ButtonEx("+", ImVec2(button_size, button_size), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups))
        {
            DataTypeApplyOp(data_type, '+', data_ptr, data_ptr, g.IO.KeyCtrl && step_fast ? step_fast : step);
            value_changed = true;
        }
        SameLine(0, style.ItemInnerSpacing.x);
        TextUnformatted(label, FindRenderedTextEnd(label));

        PopID();
        EndGroup();
    }
    else
    {
        if (InputText(label, buf, IM_ARRAYSIZE(buf), extra_flags))
            value_changed = DataTypeApplyOpFromText(buf, g.InputTextState.InitialText.Data, data_type, data_ptr, format);
    }

    return value_changed;
}

bool ImGui::InputScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* step, const void* step_fast, const char* format, ImGuiInputTextFlags extra_flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components);
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        value_changed |= InputScalar("##v", data_type, v, step, step_fast, format, extra_flags);
        SameLine(0, g.Style.ItemInnerSpacing.x);
        PopID();
        PopItemWidth();
        v = (void*)((char*)v + type_size);
    }
    PopID();

    TextUnformatted(label, FindRenderedTextEnd(label));
    EndGroup();
    return value_changed;
}

bool ImGui::InputFloat(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags extra_flags)
{
    extra_flags |= ImGuiInputTextFlags_CharsScientific;
    return InputScalar(label, ImGuiDataType_Float, (void*)v, (void*)(step>0.0f ? &step : NULL), (void*)(step_fast>0.0f ? &step_fast : NULL), format, extra_flags);
}

bool ImGui::InputFloat2(const char* label, float v[2], const char* format, ImGuiInputTextFlags extra_flags)
{
    return InputScalarN(label, ImGuiDataType_Float, v, 2, NULL, NULL, format, extra_flags);
}

bool ImGui::InputFloat3(const char* label, float v[3], const char* format, ImGuiInputTextFlags extra_flags)
{
    return InputScalarN(label, ImGuiDataType_Float, v, 3, NULL, NULL, format, extra_flags);
}

bool ImGui::InputFloat4(const char* label, float v[4], const char* format, ImGuiInputTextFlags extra_flags)
{
    return InputScalarN(label, ImGuiDataType_Float, v, 4, NULL, NULL, format, extra_flags);
}

// Prefer using "const char* format" directly, which is more flexible and consistent with other API.
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
bool ImGui::InputFloat(const char* label, float* v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags extra_flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputFloat(label, v, step, step_fast, format, extra_flags);
}

bool ImGui::InputFloat2(const char* label, float v[2], int decimal_precision, ImGuiInputTextFlags extra_flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputScalarN(label, ImGuiDataType_Float, v, 2, NULL, NULL, format, extra_flags);
}

bool ImGui::InputFloat3(const char* label, float v[3], int decimal_precision, ImGuiInputTextFlags extra_flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputScalarN(label, ImGuiDataType_Float, v, 3, NULL, NULL, format, extra_flags);
}

bool ImGui::InputFloat4(const char* label, float v[4], int decimal_precision, ImGuiInputTextFlags extra_flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputScalarN(label, ImGuiDataType_Float, v, 4, NULL, NULL, format, extra_flags);
}
#endif // IMGUI_DISABLE_OBSOLETE_FUNCTIONS

bool ImGui::InputInt(const char* label, int* v, int step, int step_fast, ImGuiInputTextFlags extra_flags)
{
    // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
    const char* format = (extra_flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
    return InputScalar(label, ImGuiDataType_S32, (void*)v, (void*)(step>0 ? &step : NULL), (void*)(step_fast>0 ? &step_fast : NULL), format, extra_flags);
}

bool ImGui::InputInt2(const char* label, int v[2], ImGuiInputTextFlags extra_flags)
{
    return InputScalarN(label, ImGuiDataType_S32, v, 2, NULL, NULL, "%d", extra_flags);
}

bool ImGui::InputInt3(const char* label, int v[3], ImGuiInputTextFlags extra_flags)
{
    return InputScalarN(label, ImGuiDataType_S32, v, 3, NULL, NULL, "%d", extra_flags);
}

bool ImGui::InputInt4(const char* label, int v[4], ImGuiInputTextFlags extra_flags)
{
    return InputScalarN(label, ImGuiDataType_S32, v, 4, NULL, NULL, "%d", extra_flags);
}

bool ImGui::InputDouble(const char* label, double* v, double step, double step_fast, const char* format, ImGuiInputTextFlags extra_flags)
{
    extra_flags |= ImGuiInputTextFlags_CharsScientific;
    return InputScalar(label, ImGuiDataType_Double, (void*)v, (void*)(step>0.0 ? &step : NULL), (void*)(step_fast>0.0 ? &step_fast : NULL), format, extra_flags);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputText, InputTextMultiline
//-------------------------------------------------------------------------
// - InputText()
// - InputTextMultiline()
// - InputTextEx() [Internal]
//-------------------------------------------------------------------------

bool ImGui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
    return InputTextEx(label, buf, (int)buf_size, ImVec2(0,0), flags, callback, user_data);
}

bool ImGui::InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    return InputTextEx(label, buf, (int)buf_size, size, flags | ImGuiInputTextFlags_Multiline, callback, user_data);
}

static int InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end)
{
    int line_count = 0;
    const char* s = text_begin;
    while (char c = *s++) // We are only matching for \n so we can ignore UTF-8 decoding
        if (c == '\n')
            line_count++;
    s--;
    if (s[0] != '\n' && s[0] != '\r')
        line_count++;
    *out_text_end = s;
    return line_count;
}

static ImVec2 InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining, ImVec2* out_offset, bool stop_on_new_line)
{
    ImFont* font = GImGui->Font;
    const float line_height = GImGui->FontSize;
    const float scale = line_height / font->FontSize;

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const ImWchar* s = text_begin;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)(*s++);
        if (c == '\n')
        {
            text_size.x = ImMax(text_size.x, line_width);
            text_size.y += line_height;
            line_width = 0.0f;
            if (stop_on_new_line)
                break;
            continue;
        }
        if (c == '\r')
            continue;

        const float char_width = font->GetCharAdvance((unsigned short)c) * scale;
        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (out_offset)
        *out_offset = ImVec2(line_width, text_size.y + line_height);  // offset allow for the possibility of sitting after a trailing \n

    if (line_width > 0 || text_size.y == 0.0f)                        // whereas size.y will ignore the trailing \n
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

// Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
namespace ImGuiStb
{

static int     STB_TEXTEDIT_STRINGLEN(const STB_TEXTEDIT_STRING* obj)                             { return obj->CurLenW; }
static ImWchar STB_TEXTEDIT_GETCHAR(const STB_TEXTEDIT_STRING* obj, int idx)                      { return obj->TextW[idx]; }
static float   STB_TEXTEDIT_GETWIDTH(STB_TEXTEDIT_STRING* obj, int line_start_idx, int char_idx)  { ImWchar c = obj->TextW[line_start_idx+char_idx]; if (c == '\n') return STB_TEXTEDIT_GETWIDTH_NEWLINE; return GImGui->Font->GetCharAdvance(c) * (GImGui->FontSize / GImGui->Font->FontSize); }
static int     STB_TEXTEDIT_KEYTOTEXT(int key)                                                    { return key >= 0x10000 ? 0 : key; }
static ImWchar STB_TEXTEDIT_NEWLINE = '\n';
static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, STB_TEXTEDIT_STRING* obj, int line_start_idx)
{
    const ImWchar* text = obj->TextW.Data;
    const ImWchar* text_remaining = NULL;
    const ImVec2 size = InputTextCalcTextSizeW(text + line_start_idx, text + obj->CurLenW, &text_remaining, NULL, true);
    r->x0 = 0.0f;
    r->x1 = size.x;
    r->baseline_y_delta = size.y;
    r->ymin = 0.0f;
    r->ymax = size.y;
    r->num_chars = (int)(text_remaining - (text + line_start_idx));
}

static bool is_separator(unsigned int c)                                        { return ImCharIsBlankW(c) || c==',' || c==';' || c=='(' || c==')' || c=='{' || c=='}' || c=='[' || c==']' || c=='|'; }
static int  is_word_boundary_from_right(STB_TEXTEDIT_STRING* obj, int idx)      { return idx > 0 ? (is_separator( obj->TextW[idx-1] ) && !is_separator( obj->TextW[idx] ) ) : 1; }
static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(STB_TEXTEDIT_STRING* obj, int idx)   { idx--; while (idx >= 0 && !is_word_boundary_from_right(obj, idx)) idx--; return idx < 0 ? 0 : idx; }
#ifdef __APPLE__    // FIXME: Move setting to IO structure
static int  is_word_boundary_from_left(STB_TEXTEDIT_STRING* obj, int idx)       { return idx > 0 ? (!is_separator( obj->TextW[idx-1] ) && is_separator( obj->TextW[idx] ) ) : 1; }
static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx)  { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_left(obj, idx)) idx++; return idx > len ? len : idx; }
#else
static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx)  { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_right(obj, idx)) idx++; return idx > len ? len : idx; }
#endif
#define STB_TEXTEDIT_MOVEWORDLEFT   STB_TEXTEDIT_MOVEWORDLEFT_IMPL    // They need to be #define for stb_textedit.h
#define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL

static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)
{
    ImWchar* dst = obj->TextW.Data + pos;

    // We maintain our buffer length in both UTF-8 and wchar formats
    obj->CurLenA -= ImTextCountUtf8BytesFromStr(dst, dst + n);
    obj->CurLenW -= n;

    // Offset remaining text
    const ImWchar* src = obj->TextW.Data + pos + n;
    while (ImWchar c = *src++)
        *dst++ = c;
    *dst = '\0';
}

static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const ImWchar* new_text, int new_text_len)
{
    const bool is_resizable = (obj->UserFlags & ImGuiInputTextFlags_CallbackResize) != 0;
    const int text_len = obj->CurLenW;
    IM_ASSERT(pos <= text_len);

    const int new_text_len_utf8 = ImTextCountUtf8BytesFromStr(new_text, new_text + new_text_len);
    if (!is_resizable && (new_text_len_utf8 + obj->CurLenA + 1 > obj->BufCapacityA))
        return false;

    // Grow internal buffer if needed
    if (new_text_len + text_len + 1 > obj->TextW.Size)
    {
        if (!is_resizable)
            return false;
        IM_ASSERT(text_len < obj->TextW.Size);
        obj->TextW.resize(text_len + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1);
    }

    ImWchar* text = obj->TextW.Data;
    if (pos != text_len)
        memmove(text + pos + new_text_len, text + pos, (size_t)(text_len - pos) * sizeof(ImWchar));
    memcpy(text + pos, new_text, (size_t)new_text_len * sizeof(ImWchar));

    obj->CurLenW += new_text_len;
    obj->CurLenA += new_text_len_utf8;
    obj->TextW[obj->CurLenW] = '\0';

    return true;
}

// We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
#define STB_TEXTEDIT_K_LEFT         0x10000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x10001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x10002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x10003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x10004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x10005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x10006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x10007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x10008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x10009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x1000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x1000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x1000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x1000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_SHIFT        0x20000

#define STB_TEXTEDIT_IMPLEMENTATION
#include "imstb_textedit.h"

}

void ImGuiInputTextState::OnKeyPressed(int key)
{
    stb_textedit_key(this, &StbState, key);
    CursorFollow = true;
    CursorAnimReset();
}

ImGuiInputTextCallbackData::ImGuiInputTextCallbackData()
{
    memset(this, 0, sizeof(*this));
}

// Public API to manipulate UTF-8 text
// We expose UTF-8 to the user (unlike the STB_TEXTEDIT_* functions which are manipulating wchar)
// FIXME: The existence of this rarely exercised code path is a bit of a nuisance.
void ImGuiInputTextCallbackData::DeleteChars(int pos, int bytes_count)
{
    IM_ASSERT(pos + bytes_count <= BufTextLen);
    char* dst = Buf + pos;
    const char* src = Buf + pos + bytes_count;
    while (char c = *src++)
        *dst++ = c;
    *dst = '\0';

    if (CursorPos + bytes_count >= pos)
        CursorPos -= bytes_count;
    else if (CursorPos >= pos)
        CursorPos = pos;
    SelectionStart = SelectionEnd = CursorPos;
    BufDirty = true;
    BufTextLen -= bytes_count;
}

void ImGuiInputTextCallbackData::InsertChars(int pos, const char* new_text, const char* new_text_end)
{
    const bool is_resizable = (Flags & ImGuiInputTextFlags_CallbackResize) != 0;
    const int new_text_len = new_text_end ? (int)(new_text_end - new_text) : (int)strlen(new_text);
    if (new_text_len + BufTextLen >= BufSize)
    {
        if (!is_resizable)
            return;

        // Contrary to STB_TEXTEDIT_INSERTCHARS() this is working in the UTF8 buffer, hence the midly similar code (until we remove the U16 buffer alltogether!)
        ImGuiContext& g = *GImGui;
        ImGuiInputTextState* edit_state = &g.InputTextState;
        IM_ASSERT(edit_state->ID != 0 && g.ActiveId == edit_state->ID);
        IM_ASSERT(Buf == edit_state->TempBuffer.Data);
        int new_buf_size = BufTextLen + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1;
        edit_state->TempBuffer.reserve(new_buf_size + 1);
        Buf = edit_state->TempBuffer.Data;
        BufSize = edit_state->BufCapacityA = new_buf_size;
    }

    if (BufTextLen != pos)
        memmove(Buf + pos + new_text_len, Buf + pos, (size_t)(BufTextLen - pos));
    memcpy(Buf + pos, new_text, (size_t)new_text_len * sizeof(char));
    Buf[BufTextLen + new_text_len] = '\0';

    if (CursorPos >= pos)
        CursorPos += new_text_len;
    SelectionStart = SelectionEnd = CursorPos;
    BufDirty = true;
    BufTextLen += new_text_len;
}

// Return false to discard a character.
static bool InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    unsigned int c = *p_char;

    if (c < 128 && c != ' ' && !isprint((int)(c & 0xFF)))
    {
        bool pass = false;
        pass |= (c == '\n' && (flags & ImGuiInputTextFlags_Multiline));
        pass |= (c == '\t' && (flags & ImGuiInputTextFlags_AllowTabInput));
        if (!pass)
            return false;
    }

    if (c >= 0xE000 && c <= 0xF8FF) // Filter private Unicode range. I don't imagine anybody would want to input them. GLFW on OSX seems to send private characters for special keys like arrow keys.
        return false;

    if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsScientific))
    {
        if (flags & ImGuiInputTextFlags_CharsDecimal)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsScientific)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsHexadecimal)
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsUppercase)
            if (c >= 'a' && c <= 'z')
                *p_char = (c += (unsigned int)('A'-'a'));

        if (flags & ImGuiInputTextFlags_CharsNoBlank)
            if (ImCharIsBlankW(c))
                return false;
    }

    if (flags & ImGuiInputTextFlags_CallbackCharFilter)
    {
        ImGuiInputTextCallbackData callback_data;
        memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
        callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter;
        callback_data.EventChar = (ImWchar)c;
        callback_data.Flags = flags;
        callback_data.UserData = user_data;
        if (callback(&callback_data) != 0)
            return false;
        *p_char = callback_data.EventChar;
        if (!callback_data.EventChar)
            return false;
    }

    return true;
}

// Edit a string of text
// - buf_size account for the zero-terminator, so a buf_size of 6 can hold "Hello" but not "Hello!".
//   This is so we can easily call InputText() on static arrays using ARRAYSIZE() and to match 
//   Note that in std::string world, capacity() would omit 1 byte used by the zero-terminator.
// - When active, hold on a privately held copy of the text (and apply back to 'buf'). So changing 'buf' while the InputText is active has no effect.
// - If you want to use ImGui::InputText() with std::string, see misc/stl/imgui_stl.h
// (FIXME: Rather messy function partly because we are doing UTF8 > u16 > UTF8 conversions on the go to more easily handle stb_textedit calls. Ideally we should stay in UTF-8 all the time. See https://github.com/nothings/stb/issues/188)
bool ImGui::InputTextEx(const char* label, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)

    ImGuiContext& g = *GImGui;
    const ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
    const bool is_editable = (flags & ImGuiInputTextFlags_ReadOnly) == 0;
    const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
    const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
    const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
    if (is_resizable)
        IM_ASSERT(callback != NULL); // Must provide a callback if you set the ImGuiInputTextFlags_CallbackResize flag!

    if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope, 
        BeginGroup();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), (is_multiline ? GetTextLineHeight() * 8.0f : label_size.y) + style.FramePadding.y*2.0f); // Arbitrary default of 8 lines high for multi-line
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? (style.ItemInnerSpacing.x + label_size.x) : 0.0f, 0.0f));

    ImGuiWindow* draw_window = window;
    if (is_multiline)
    {
        ItemAdd(total_bb, id, &frame_bb);
        if (!BeginChildFrame(id, frame_bb.GetSize()))
        {
            EndChildFrame();
            EndGroup();
            return false;
        }
        draw_window = GetCurrentWindow();
        draw_window->DC.NavLayerActiveMaskNext |= draw_window->DC.NavLayerCurrentMask; // This is to ensure that EndChild() will display a navigation highlight
        size.x -= draw_window->ScrollbarSizes.x;
    }
    else
    {
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb))
            return false;
    }
    const bool hovered = ItemHoverable(frame_bb, id);
    if (hovered)
        g.MouseCursor = ImGuiMouseCursor_TextInput;

    // Password pushes a temporary font with only a fallback glyph
    if (is_password)
    {
        const ImFontGlyph* glyph = g.Font->FindGlyph('*');
        ImFont* password_font = &g.InputTextPasswordFont;
        password_font->FontSize = g.Font->FontSize;
        password_font->Scale = g.Font->Scale;
        password_font->DisplayOffset = g.Font->DisplayOffset;
        password_font->Ascent = g.Font->Ascent;
        password_font->Descent = g.Font->Descent;
        password_font->ContainerAtlas = g.Font->ContainerAtlas;
        password_font->FallbackGlyph = glyph;
        password_font->FallbackAdvanceX = glyph->AdvanceX;
        IM_ASSERT(password_font->Glyphs.empty() && password_font->IndexAdvanceX.empty() && password_font->IndexLookup.empty());
        PushFont(password_font);
    }

    // NB: we are only allowed to access 'edit_state' if we are the active widget.
    ImGuiInputTextState& edit_state = g.InputTextState;

    const bool focus_requested = FocusableItemRegister(window, id, (flags & (ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_AllowTabInput)) == 0);    // Using completion callback disable keyboard tabbing
    const bool focus_requested_by_code = focus_requested && (window->FocusIdxAllCounter == window->FocusIdxAllRequestCurrent);
    const bool focus_requested_by_tab = focus_requested && !focus_requested_by_code;

    const bool user_clicked = hovered && io.MouseClicked[0];
    const bool user_scrolled = is_multiline && g.ActiveId == 0 && edit_state.ID == id && g.ActiveIdPreviousFrame == draw_window->GetIDNoKeepAlive("#SCROLLY");
    const bool user_nav_input_start = (g.ActiveId != id) && ((g.NavInputId == id) || (g.NavActivateId == id && g.NavInputSource == ImGuiInputSource_NavKeyboard));

    bool clear_active_id = false;

    bool select_all = (g.ActiveId != id) && ((flags & ImGuiInputTextFlags_AutoSelectAll) != 0 || user_nav_input_start) && (!is_multiline);
    if (focus_requested || user_clicked || user_scrolled || user_nav_input_start)
    {
        if (g.ActiveId != id)
        {
            // Start edition
            // Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
            // From the moment we focused we are ignoring the content of 'buf' (unless we are in read-only mode)
            const int prev_len_w = edit_state.CurLenW;
            const int init_buf_len = (int)strlen(buf);
            edit_state.TextW.resize(buf_size+1);             // wchar count <= UTF-8 count. we use +1 to make sure that .Data isn't NULL so it doesn't crash.
            edit_state.InitialText.resize(init_buf_len + 1); // UTF-8. we use +1 to make sure that .Data isn't NULL so it doesn't crash.
            memcpy(edit_state.InitialText.Data, buf, init_buf_len + 1);
            const char* buf_end = NULL;
            edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, buf_size, buf, NULL, &buf_end);
            edit_state.CurLenA = (int)(buf_end - buf); // We can't get the result from ImStrncpy() above because it is not UTF-8 aware. Here we'll cut off malformed UTF-8.
            edit_state.CursorAnimReset();

            // Preserve cursor position and undo/redo stack if we come back to same widget
            // FIXME: We should probably compare the whole buffer to be on the safety side. Comparing buf (utf8) and edit_state.Text (wchar).
            const bool recycle_state = (edit_state.ID == id) && (prev_len_w == edit_state.CurLenW);
            if (recycle_state)
            {
                // Recycle existing cursor/selection/undo stack but clamp position
                // Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
                edit_state.CursorClamp();
            }
            else
            {
                edit_state.ID = id;
                edit_state.ScrollX = 0.0f;
                stb_textedit_initialize_state(&edit_state.StbState, !is_multiline);
                if (!is_multiline && focus_requested_by_code)
                    select_all = true;
            }
            if (flags & ImGuiInputTextFlags_AlwaysInsertMode)
                edit_state.StbState.insert_mode = true;
            if (!is_multiline && (focus_requested_by_tab || (user_clicked && io.KeyCtrl)))
                select_all = true;
        }
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        if (!is_multiline && !(flags & ImGuiInputTextFlags_CallbackHistory))
            g.ActiveIdAllowNavDirFlags |= ((1 << ImGuiDir_Up) | (1 << ImGuiDir_Down));
    }
    else if (io.MouseClicked[0])
    {
        // Release focus when we click outside
        clear_active_id = true;
    }

    bool value_changed = false;
    bool enter_pressed = false;
    int backup_current_text_length = 0;

    if (g.ActiveId == id)
    {
        if (!is_editable && !g.ActiveIdIsJustActivated)
        {
            // When read-only we always use the live data passed to the function
            edit_state.TextW.resize(buf_size+1);
            const char* buf_end = NULL;
            edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, edit_state.TextW.Size, buf, NULL, &buf_end);
            edit_state.CurLenA = (int)(buf_end - buf);
            edit_state.CursorClamp();
        }

        backup_current_text_length = edit_state.CurLenA;
        edit_state.BufCapacityA = buf_size;
        edit_state.UserFlags = flags;
        edit_state.UserCallback = callback;
        edit_state.UserCallbackData = callback_user_data;

        // Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
        // Down the line we should have a cleaner library-wide concept of Selected vs Active.
        g.ActiveIdAllowOverlap = !io.MouseDown[0];
        g.WantTextInputNextFrame = 1;

        // Edit in progress
        const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + edit_state.ScrollX;
        const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y - style.FramePadding.y) : (g.FontSize*0.5f));

        const bool is_osx = io.ConfigMacOSXBehaviors;
        if (select_all || (hovered && !is_osx && io.MouseDoubleClicked[0]))
        {
            edit_state.SelectAll();
            edit_state.SelectedAllMouseLock = true;
        }
        else if (hovered && is_osx && io.MouseDoubleClicked[0])
        {
            // Double-click select a word only, OS X style (by simulating keystrokes)
            edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
            edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
        }
        else if (io.MouseClicked[0] && !edit_state.SelectedAllMouseLock)
        {
            if (hovered)
            {
                stb_textedit_click(&edit_state, &edit_state.StbState, mouse_x, mouse_y);
                edit_state.CursorAnimReset();
            }
        }
        else if (io.MouseDown[0] && !edit_state.SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
        {
            stb_textedit_drag(&edit_state, &edit_state.StbState, mouse_x, mouse_y);
            edit_state.CursorAnimReset();
            edit_state.CursorFollow = true;
        }
        if (edit_state.SelectedAllMouseLock && !io.MouseDown[0])
            edit_state.SelectedAllMouseLock = false;

        if (io.InputCharacters[0])
        {
            // Process text input (before we check for Return because using some IME will effectively send a Return?)
            // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
            bool ignore_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeySuper);
            if (!ignore_inputs && is_editable && !user_nav_input_start)
                for (int n = 0; n < IM_ARRAYSIZE(io.InputCharacters) && io.InputCharacters[n]; n++)
                {
                    // Insert character if they pass filtering
                    unsigned int c = (unsigned int)io.InputCharacters[n];
                    if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                        edit_state.OnKeyPressed((int)c);
                }

            // Consume characters
            memset(g.IO.InputCharacters, 0, sizeof(g.IO.InputCharacters));
        }
    }

    bool cancel_edit = false;
    if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
    {
        // Handle key-presses
        const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
        const bool is_osx = io.ConfigMacOSXBehaviors;
        const bool is_shortcut_key = (is_osx ? (io.KeySuper && !io.KeyCtrl) : (io.KeyCtrl && !io.KeySuper)) && !io.KeyAlt && !io.KeyShift; // OS X style: Shortcuts using Cmd/Super instead of Ctrl
        const bool is_osx_shift_shortcut = is_osx && io.KeySuper && io.KeyShift && !io.KeyCtrl && !io.KeyAlt;
        const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
        const bool is_startend_key_down = is_osx && io.KeySuper && !io.KeyCtrl && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
        const bool is_ctrl_key_only = io.KeyCtrl && !io.KeyShift && !io.KeyAlt && !io.KeySuper;
        const bool is_shift_key_only = io.KeyShift && !io.KeyCtrl && !io.KeyAlt && !io.KeySuper;

        const bool is_cut   = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_X)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Delete))) && is_editable && !is_password && (!is_multiline || edit_state.HasSelection());
        const bool is_copy  = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_C)) || (is_ctrl_key_only  && IsKeyPressedMap(ImGuiKey_Insert))) && !is_password && (!is_multiline || edit_state.HasSelection());
        const bool is_paste = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_V)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Insert))) && is_editable;
        const bool is_undo  = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Z)) && is_editable && is_undoable);
        const bool is_redo  = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Y)) || (is_osx_shift_shortcut && IsKeyPressedMap(ImGuiKey_Z))) && is_editable && is_undoable;

        if (IsKeyPressedMap(ImGuiKey_LeftArrow))                        { edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_RightArrow))                  { edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_UpArrow) && is_multiline)     { if (io.KeyCtrl) SetWindowScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_DownArrow) && is_multiline)   { if (io.KeyCtrl) SetWindowScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Home))                        { edit_state.OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_End))                         { edit_state.OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Delete) && is_editable)       { edit_state.OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Backspace) && is_editable)
        {
            if (!edit_state.HasSelection())
            {
                if (is_wordmove_key_down) edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT|STB_TEXTEDIT_K_SHIFT);
                else if (is_osx && io.KeySuper && !io.KeyAlt && !io.KeyCtrl) edit_state.OnKeyPressed(STB_TEXTEDIT_K_LINESTART|STB_TEXTEDIT_K_SHIFT);
            }
            edit_state.OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
        }
        else if (IsKeyPressedMap(ImGuiKey_Enter))
        {
            bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
            if (!is_multiline || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
            {
                enter_pressed = clear_active_id = true;
            }
            else if (is_editable)
            {
                unsigned int c = '\n'; // Insert new line
                if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                    edit_state.OnKeyPressed((int)c);
            }
        }
        else if ((flags & ImGuiInputTextFlags_AllowTabInput) && IsKeyPressedMap(ImGuiKey_Tab) && !io.KeyCtrl && !io.KeyShift && !io.KeyAlt && is_editable)
        {
            unsigned int c = '\t'; // Insert TAB
            if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                edit_state.OnKeyPressed((int)c);
        }
        else if (IsKeyPressedMap(ImGuiKey_Escape))
        {
            clear_active_id = cancel_edit = true;
        }
        else if (is_undo || is_redo)
        {
            edit_state.OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
            edit_state.ClearSelection();
        }
        else if (is_shortcut_key && IsKeyPressedMap(ImGuiKey_A))
        {
            edit_state.SelectAll();
            edit_state.CursorFollow = true;
        }
        else if (is_cut || is_copy)
        {
            // Cut, Copy
            if (io.SetClipboardTextFn)
            {
                const int ib = edit_state.HasSelection() ? ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end) : 0;
                const int ie = edit_state.HasSelection() ? ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end) : edit_state.CurLenW;
                edit_state.TempBuffer.resize((ie-ib) * 4 + 1);
                ImTextStrToUtf8(edit_state.TempBuffer.Data, edit_state.TempBuffer.Size, edit_state.TextW.Data+ib, edit_state.TextW.Data+ie);
                SetClipboardText(edit_state.TempBuffer.Data);
            }
            if (is_cut)
            {
                if (!edit_state.HasSelection())
                    edit_state.SelectAll();
                edit_state.CursorFollow = true;
                stb_textedit_cut(&edit_state, &edit_state.StbState);
            }
        }
        else if (is_paste)
        {
            if (const char* clipboard = GetClipboardText())
            {
                // Filter pasted buffer
                const int clipboard_len = (int)strlen(clipboard);
                ImWchar* clipboard_filtered = (ImWchar*)ImGui::MemAlloc((clipboard_len+1) * sizeof(ImWchar));
                int clipboard_filtered_len = 0;
                for (const char* s = clipboard; *s; )
                {
                    unsigned int c;
                    s += ImTextCharFromUtf8(&c, s, NULL);
                    if (c == 0)
                        break;
                    if (c >= 0x10000 || !InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                        continue;
                    clipboard_filtered[clipboard_filtered_len++] = (ImWchar)c;
                }
                clipboard_filtered[clipboard_filtered_len] = 0;
                if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
                {
                    stb_textedit_paste(&edit_state, &edit_state.StbState, clipboard_filtered, clipboard_filtered_len);
                    edit_state.CursorFollow = true;
                }
                ImGui::MemFree(clipboard_filtered);
            }
        }
    }

    if (g.ActiveId == id)
    {
        const char* apply_new_text = NULL;
        int apply_new_text_length = 0;
        if (cancel_edit)
        {
            // Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
            if (is_editable && strcmp(buf, edit_state.InitialText.Data) != 0)
            {
                apply_new_text = edit_state.InitialText.Data;
                apply_new_text_length = edit_state.InitialText.Size - 1;
            }
        }

        // When using 'ImGuiInputTextFlags_EnterReturnsTrue' as a special case we reapply the live buffer back to the input buffer before clearing ActiveId, even though strictly speaking it wasn't modified on this frame.
        // If we didn't do that, code like InputInt() with ImGuiInputTextFlags_EnterReturnsTrue would fail. Also this allows the user to use InputText() with ImGuiInputTextFlags_EnterReturnsTrue without maintaining any user-side storage.
        bool apply_edit_back_to_user_buffer = !cancel_edit || (enter_pressed && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
        if (apply_edit_back_to_user_buffer)
        {
            // Apply new value immediately - copy modified buffer back
            // Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
            // FIXME: We actually always render 'buf' when calling DrawList->AddText, making the comment above incorrect.
            // FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks.
            if (is_editable)
            {
                edit_state.TempBuffer.resize(edit_state.TextW.Size * 4 + 1);
                ImTextStrToUtf8(edit_state.TempBuffer.Data, edit_state.TempBuffer.Size, edit_state.TextW.Data, NULL);
            }

            // User callback
            if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways)) != 0)
            {
                IM_ASSERT(callback != NULL);

                // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
                ImGuiInputTextFlags event_flag = 0;
                ImGuiKey event_key = ImGuiKey_COUNT;
                if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressedMap(ImGuiKey_Tab))
                {
                    event_flag = ImGuiInputTextFlags_CallbackCompletion;
                    event_key = ImGuiKey_Tab;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_UpArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_UpArrow;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_DownArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_DownArrow;
                }
                else if (flags & ImGuiInputTextFlags_CallbackAlways)
                    event_flag = ImGuiInputTextFlags_CallbackAlways;

                if (event_flag)
                {
                    ImGuiInputTextCallbackData callback_data;
                    memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
                    callback_data.EventFlag = event_flag;
                    callback_data.Flags = flags;
                    callback_data.UserData = callback_user_data;

                    callback_data.EventKey = event_key;
                    callback_data.Buf = edit_state.TempBuffer.Data;
                    callback_data.BufTextLen = edit_state.CurLenA;
                    callback_data.BufSize = edit_state.BufCapacityA;
                    callback_data.BufDirty = false;

                    // We have to convert from wchar-positions to UTF-8-positions, which can be pretty slow (an incentive to ditch the ImWchar buffer, see https://github.com/nothings/stb/issues/188)
                    ImWchar* text = edit_state.TextW.Data;
                    const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromStr(text, text + edit_state.StbState.cursor);
                    const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromStr(text, text + edit_state.StbState.select_start);
                    const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromStr(text, text + edit_state.StbState.select_end);

                    // Call user code
                    callback(&callback_data);

                    // Read back what user may have modified
                    IM_ASSERT(callback_data.Buf == edit_state.TempBuffer.Data);  // Invalid to modify those fields
                    IM_ASSERT(callback_data.BufSize == edit_state.BufCapacityA);
                    IM_ASSERT(callback_data.Flags == flags);
                    if (callback_data.CursorPos != utf8_cursor_pos)            { edit_state.StbState.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos); edit_state.CursorFollow = true; }
                    if (callback_data.SelectionStart != utf8_selection_start)  { edit_state.StbState.select_start = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart); }
                    if (callback_data.SelectionEnd != utf8_selection_end)      { edit_state.StbState.select_end = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd); }
                    if (callback_data.BufDirty)
                    {
                        IM_ASSERT(callback_data.BufTextLen == (int)strlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
                        if (callback_data.BufTextLen > backup_current_text_length && is_resizable)
                            edit_state.TextW.resize(edit_state.TextW.Size + (callback_data.BufTextLen - backup_current_text_length));
                        edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, edit_state.TextW.Size, callback_data.Buf, NULL);
                        edit_state.CurLenA = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
                        edit_state.CursorAnimReset();
                    }
                }
            }

            // Will copy result string if modified
            if (is_editable && strcmp(edit_state.TempBuffer.Data, buf) != 0)
            {
                apply_new_text = edit_state.TempBuffer.Data;
                apply_new_text_length = edit_state.CurLenA;
            }
        }

        // Copy result to user buffer
        if (apply_new_text)
        {
            IM_ASSERT(apply_new_text_length >= 0);
            if (backup_current_text_length != apply_new_text_length && is_resizable)
            {
                ImGuiInputTextCallbackData callback_data;
                callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
                callback_data.Flags = flags;
                callback_data.Buf = buf;
                callback_data.BufTextLen = apply_new_text_length;
                callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
                callback_data.UserData = callback_user_data;
                callback(&callback_data);
                buf = callback_data.Buf;
                buf_size = callback_data.BufSize;
                apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
                IM_ASSERT(apply_new_text_length <= buf_size);
            }

            // If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
            ImStrncpy(buf, edit_state.TempBuffer.Data, ImMin(apply_new_text_length + 1, buf_size));
            value_changed = true;
        }

        // Clear temporary user storage
        edit_state.UserFlags = 0;
        edit_state.UserCallback = NULL;
        edit_state.UserCallbackData = NULL;
    }

    // Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
    if (clear_active_id && g.ActiveId == id)
        ClearActiveID();

    // Render
    // Select which buffer we are going to display. When ImGuiInputTextFlags_NoLiveEdit is set 'buf' might still be the old value. We set buf to NULL to prevent accidental usage from now on.
    const char* buf_display = (g.ActiveId == id && is_editable) ? edit_state.TempBuffer.Data : buf; buf = NULL;

    // Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
    // without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
    // Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
    const int buf_display_max_length = 2 * 1024 * 1024;

    if (!is_multiline)
    {
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    }

    const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
    ImVec2 render_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
    ImVec2 text_size(0.f, 0.f);
    const bool is_currently_scrolling = (edit_state.ID == id && is_multiline && g.ActiveId == draw_window->GetIDNoKeepAlive("#SCROLLY"));
    if (g.ActiveId == id || is_currently_scrolling)
    {
        edit_state.CursorAnim += io.DeltaTime;

        // This is going to be messy. We need to:
        // - Display the text (this alone can be more easily clipped)
        // - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
        // - Measure text height (for scrollbar)
        // We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
        // FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
        const ImWchar* text_begin = edit_state.TextW.Data;
        ImVec2 cursor_offset, select_start_offset;

        {
            // Count lines + find lines numbers straddling 'cursor' and 'select_start' position.
            const ImWchar* searches_input_ptr[2];
            searches_input_ptr[0] = text_begin + edit_state.StbState.cursor;
            searches_input_ptr[1] = NULL;
            int searches_remaining = 1;
            int searches_result_line_number[2] = { -1, -999 };
            if (edit_state.StbState.select_start != edit_state.StbState.select_end)
            {
                searches_input_ptr[1] = text_begin + ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end);
                searches_result_line_number[1] = -1;
                searches_remaining++;
            }

            // Iterate all lines to find our line numbers
            // In multi-line mode, we never exit the loop until all lines are counted, so add one extra to the searches_remaining counter.
            searches_remaining += is_multiline ? 1 : 0;
            int line_count = 0;
            for (const ImWchar* s = text_begin; *s != 0; s++)
                if (*s == '\n')
                {
                    line_count++;
                    if (searches_result_line_number[0] == -1 && s >= searches_input_ptr[0]) { searches_result_line_number[0] = line_count; if (--searches_remaining <= 0) break; }
                    if (searches_result_line_number[1] == -1 && s >= searches_input_ptr[1]) { searches_result_line_number[1] = line_count; if (--searches_remaining <= 0) break; }
                }
            line_count++;
            if (searches_result_line_number[0] == -1) searches_result_line_number[0] = line_count;
            if (searches_result_line_number[1] == -1) searches_result_line_number[1] = line_count;

            // Calculate 2d position by finding the beginning of the line and measuring distance
            cursor_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[0], text_begin), searches_input_ptr[0]).x;
            cursor_offset.y = searches_result_line_number[0] * g.FontSize;
            if (searches_result_line_number[1] >= 0)
            {
                select_start_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[1], text_begin), searches_input_ptr[1]).x;
                select_start_offset.y = searches_result_line_number[1] * g.FontSize;
            }

            // Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
            if (is_multiline)
                text_size = ImVec2(size.x, line_count * g.FontSize);
        }

        // Scroll
        if (edit_state.CursorFollow)
        {
            // Horizontal scroll in chunks of quarter width
            if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
            {
                const float scroll_increment_x = size.x * 0.25f;
                if (cursor_offset.x < edit_state.ScrollX)
                    edit_state.ScrollX = (float)(int)ImMax(0.0f, cursor_offset.x - scroll_increment_x);
                else if (cursor_offset.x - size.x >= edit_state.ScrollX)
                    edit_state.ScrollX = (float)(int)(cursor_offset.x - size.x + scroll_increment_x);
            }
            else
            {
                edit_state.ScrollX = 0.0f;
            }

            // Vertical scroll
            if (is_multiline)
            {
                float scroll_y = draw_window->Scroll.y;
                if (cursor_offset.y - g.FontSize < scroll_y)
                    scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
                else if (cursor_offset.y - size.y >= scroll_y)
                    scroll_y = cursor_offset.y - size.y;
                draw_window->DC.CursorPos.y += (draw_window->Scroll.y - scroll_y);   // To avoid a frame of lag
                draw_window->Scroll.y = scroll_y;
                render_pos.y = draw_window->DC.CursorPos.y;
            }
        }
        edit_state.CursorFollow = false;
        const ImVec2 render_scroll = ImVec2(edit_state.ScrollX, 0.0f);

        // Draw selection
        if (edit_state.StbState.select_start != edit_state.StbState.select_end)
        {
            const ImWchar* text_selected_begin = text_begin + ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end);
            const ImWchar* text_selected_end = text_begin + ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end);

            float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
            float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
            ImU32 bg_color = GetColorU32(ImGuiCol_TextSelectedBg);
            ImVec2 rect_pos = render_pos + select_start_offset - render_scroll;
            for (const ImWchar* p = text_selected_begin; p < text_selected_end; )
            {
                if (rect_pos.y > clip_rect.w + g.FontSize)
                    break;
                if (rect_pos.y < clip_rect.y)
                {
                    while (p < text_selected_end)
                        if (*p++ == '\n')
                            break;
                }
                else
                {
                    ImVec2 rect_size = InputTextCalcTextSizeW(p, text_selected_end, &p, NULL, true);
                    if (rect_size.x <= 0.0f) rect_size.x = (float)(int)(g.Font->GetCharAdvance((unsigned short)' ') * 0.50f); // So we can see selected empty lines
                    ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos +ImVec2(rect_size.x, bg_offy_dn));
                    rect.ClipWith(clip_rect);
                    if (rect.Overlaps(clip_rect))
                        draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
                }
                rect_pos.x = render_pos.x - render_scroll.x;
                rect_pos.y += g.FontSize;
            }
        }

        const int buf_display_len = edit_state.CurLenA;
        if (is_multiline || buf_display_len < buf_display_max_length)
            draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos - render_scroll, GetColorU32(ImGuiCol_Text), buf_display, buf_display + buf_display_len, 0.0f, is_multiline ? NULL : &clip_rect);

        // Draw blinking cursor
        bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (g.InputTextState.CursorAnim <= 0.0f) || ImFmod(g.InputTextState.CursorAnim, 1.20f) <= 0.80f;
        ImVec2 cursor_screen_pos = render_pos + cursor_offset - render_scroll;
        ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y-g.FontSize+0.5f, cursor_screen_pos.x+1.0f, cursor_screen_pos.y-1.5f);
        if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
            draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL(), GetColorU32(ImGuiCol_Text));

        // Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
        if (is_editable)
            g.PlatformImePos = ImVec2(cursor_screen_pos.x - 1, cursor_screen_pos.y - g.FontSize);
    }
    else
    {
        // Render text only
        const char* buf_end = NULL;
        if (is_multiline)
            text_size = ImVec2(size.x, InputTextCalcTextLenAndLineCount(buf_display, &buf_end) * g.FontSize); // We don't need width
        else
            buf_end = buf_display + strlen(buf_display);
        if (is_multiline || (buf_end - buf_display) < buf_display_max_length)
            draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos, GetColorU32(ImGuiCol_Text), buf_display, buf_end, 0.0f, is_multiline ? NULL : &clip_rect);
    }

    if (is_multiline)
    {
        Dummy(text_size + ImVec2(0.0f, g.FontSize)); // Always add room to scroll an extra line
        EndChildFrame();
        EndGroup();
    }

    if (is_password)
        PopFont();

    // Log as text
    if (g.LogEnabled && !is_password)
        LogRenderedText(&render_pos, buf_display, NULL);

    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if (value_changed)
        MarkItemEdited(id);

    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
        return enter_pressed;
    else
        return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
//-------------------------------------------------------------------------
// - ColorEdit3()
// - ColorEdit4()
// - ColorPicker3()
// - RenderColorRectWithAlphaCheckerboard() [Internal]
// - ColorPicker4()
// - ColorButton()
// - SetColorEditOptions()
// - ColorTooltip() [Internal]
// - ColorEditOptionsPopup() [Internal]
// - ColorPickerOptionsPopup() [Internal]
//-------------------------------------------------------------------------

bool ImGui::ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
    return ColorEdit4(label, col, flags | ImGuiColorEditFlags_NoAlpha);
}

// Edit colors components (each component in 0.0f..1.0f range).
// See enum ImGuiColorEditFlags_ for available options. e.g. Only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
// With typical options: Left-click on colored square to open color picker. Right-click to open option menu. CTRL-Click over input fields to edit them and TAB to go to next item.
bool ImGui::ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float square_sz = GetFrameHeight();
    const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
    const float w_items_all = CalcItemWidth() - w_extra;
    const char* label_display_end = FindRenderedTextEnd(label);

    BeginGroup();
    PushID(label);

    // If we're not showing any slider there's no point in doing any HSV conversions
    const ImGuiColorEditFlags flags_untouched = flags;
    if (flags & ImGuiColorEditFlags_NoInputs)
        flags = (flags & (~ImGuiColorEditFlags__InputsMask)) | ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoOptions;

    // Context menu: display and modify options (before defaults are applied)
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorEditOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags__InputsMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputsMask);
    if (!(flags & ImGuiColorEditFlags__DataTypeMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
    if (!(flags & ImGuiColorEditFlags__PickerMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
    flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask));

    const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
    const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
    const int components = alpha ? 4 : 3;

    // Convert to the formats we need
    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    if (flags & ImGuiColorEditFlags_HSV)
        ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
    int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

    bool value_changed = false;
    bool value_changed_as_float = false;

    if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB/HSV 0..255 Sliders
        const float w_item_one  = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components-1)) / (float)components));
        const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components-1)));

        const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
        const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
        const char* fmt_table_int[3][4] =
        {
            {   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
            { "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
            { "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
        };
        const char* fmt_table_float[3][4] =
        {
            {   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
            { "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
            { "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
        };
        const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_HSV) ? 2 : 1;

        PushItemWidth(w_item_one);
        for (int n = 0; n < components; n++)
        {
            if (n > 0)
                SameLine(0, style.ItemInnerSpacing.x);
            if (n + 1 == components)
                PushItemWidth(w_item_last);
            if (flags & ImGuiColorEditFlags_Float)
                value_changed = value_changed_as_float = value_changed | DragFloat(ids[n], &f[n], 1.0f/255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
            else
                value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
            if (!(flags & ImGuiColorEditFlags_NoOptions))
                OpenPopupOnItemClick("context");
        }
        PopItemWidth();
        PopItemWidth();
    }
    else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB Hexadecimal Input
        char buf[64];
        if (alpha)
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0],0,255), ImClamp(i[1],0,255), ImClamp(i[2],0,255), ImClamp(i[3],0,255));
        else
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0],0,255), ImClamp(i[1],0,255), ImClamp(i[2],0,255));
        PushItemWidth(w_items_all);
        if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
        {
            value_changed = true;
            char* p = buf;
            while (*p == '#' || ImCharIsBlankA(*p))
                p++;
            i[0] = i[1] = i[2] = i[3] = 0;
            if (alpha)
                sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
            else
                sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context");
        PopItemWidth();
    }

    ImGuiWindow* picker_active_window = NULL;
    if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
    {
        if (!(flags & ImGuiColorEditFlags_NoInputs))
            SameLine(0, style.ItemInnerSpacing.x);

        const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
        if (ColorButton("##ColorButton", col_v4, flags))
        {
            if (!(flags & ImGuiColorEditFlags_NoPicker))
            {
                // Store current color and open a picker
                g.ColorPickerRef = col_v4;
                OpenPopup("picker");
                SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1,style.ItemSpacing.y));
            }
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context");

        if (BeginPopup("picker"))
        {
            picker_active_window = g.CurrentWindow;
            if (label != label_display_end)
            {
                TextUnformatted(label, label_display_end);
                Separator();
            }
            ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
            ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
            PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
            value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
            PopItemWidth();
            EndPopup();
        }
    }

    if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
    {
        SameLine(0, style.ItemInnerSpacing.x);
        TextUnformatted(label, label_display_end);
    }

    // Convert back
    if (picker_active_window == NULL)
    {
        if (!value_changed_as_float)
            for (int n = 0; n < 4; n++)
                f[n] = i[n] / 255.0f;
        if (flags & ImGuiColorEditFlags_HSV)
            ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
        if (value_changed)
        {
            col[0] = f[0];
            col[1] = f[1];
            col[2] = f[2];
            if (alpha)
                col[3] = f[3];
        }
    }

    PopID();
    EndGroup();

    // Drag and Drop Target
    // NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
    if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
        {
            memcpy((float*)col, payload->Data, sizeof(float) * 3);
            value_changed = true;
        }
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
        {
            memcpy((float*)col, payload->Data, sizeof(float) * components);
            value_changed = true;
        }
        EndDragDropTarget();
    }

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
        window->DC.LastItemId = g.ActiveId;

    if (value_changed)
        MarkItemEdited(window->DC.LastItemId);

    return value_changed;
}

bool ImGui::ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
    float col4[4] = { col[0], col[1], col[2], 1.0f };
    if (!ColorPicker4(label, col4, flags | ImGuiColorEditFlags_NoAlpha))
        return false;
    col[0] = col4[0]; col[1] = col4[1]; col[2] = col4[2];
    return true;
}

