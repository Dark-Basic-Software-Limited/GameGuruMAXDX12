//-----------------------------------------------------------------------------
// [SECTION] ImGuiStorage
// Helper: Key->value storage
//-----------------------------------------------------------------------------

// std::lower_bound but without the bullshit
static ImGuiStorage::ImGuiStoragePair* LowerBound(ImVector<ImGuiStorage::ImGuiStoragePair>& data, ImGuiID key)
{
    ImGuiStorage::ImGuiStoragePair* first = data.Data;
    ImGuiStorage::ImGuiStoragePair* last = data.Data + data.Size;
    size_t count = (size_t)(last - first);
    while (count > 0)
    {
        size_t count2 = count >> 1;
        ImGuiStorage::ImGuiStoragePair* mid = first + count2;
        if (mid->key < key)
        {
            first = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return first;
}

// For quicker full rebuild of a storage (instead of an incremental one), you may add all your contents and then sort once.
void ImGuiStorage::BuildSortByKey()
{
    struct StaticFunc
    {
        static int IMGUI_CDECL PairCompareByID(const void* lhs, const void* rhs)
        {
            // We can't just do a subtraction because qsort uses signed integers and subtracting our ID doesn't play well with that.
            if (((const ImGuiStoragePair*)lhs)->key > ((const ImGuiStoragePair*)rhs)->key) return +1;
            if (((const ImGuiStoragePair*)lhs)->key < ((const ImGuiStoragePair*)rhs)->key) return -1;
            return 0;
        }
    };
    if (Data.Size > 1)
        ImQsort(Data.Data, (size_t)Data.Size, sizeof(ImGuiStoragePair), StaticFunc::PairCompareByID);
}

int ImGuiStorage::GetInt(ImGuiID key, int default_val) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_i;
}

bool ImGuiStorage::GetBool(ImGuiID key, bool default_val) const
{
    return GetInt(key, default_val ? 1 : 0) != 0;
}

float ImGuiStorage::GetFloat(ImGuiID key, float default_val) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return NULL;
    return it->val_p;
}

// References are only valid until a new value is added to the storage. Calling a Set***() function or a Get***Ref() function invalidates the pointer.
int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_i;
}

bool* ImGuiStorage::GetBoolRef(ImGuiID key, bool default_val)
{
    return (bool*)GetIntRef(key, default_val ? 1 : 0);
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_f;
}

void** ImGuiStorage::GetVoidPtrRef(ImGuiID key, void* default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_p;
}

// FIXME-OPT: Need a way to reuse the result of lower_bound when doing GetInt()/SetInt() - not too bad because it only happens on explicit interaction (maximum one a frame)
void ImGuiStorage::SetInt(ImGuiID key, int val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_i = val;
}

void ImGuiStorage::SetBool(ImGuiID key, bool val)
{
    SetInt(key, val ? 1 : 0);
}

void ImGuiStorage::SetFloat(ImGuiID key, float val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImGuiID key, void* val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (int i = 0; i < Data.Size; i++)
        Data[i].val_i = v;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextFilter
//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter(const char* default_filter)
{
    if (default_filter)
    {
        ImStrncpy(InputBuf, default_filter, IM_ARRAYSIZE(InputBuf));
        Build();
    }
    else
    {
        InputBuf[0] = 0;
        CountGrep = 0;
    }
}

bool ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width != 0.0f)
        ImGui::SetNextItemWidth(width);
    bool value_changed = ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (value_changed)
        Build();
    return value_changed;
}

void ImGuiTextFilter::ImGuiTextRange::split(char separator, ImVector<ImGuiTextRange>* out) const
{
    out->resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(ImGuiTextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out->push_back(ImGuiTextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    ImGuiTextRange input_range(InputBuf, InputBuf+strlen(InputBuf));
    input_range.split(',', &Filters);

    CountGrep = 0;
    for (int i = 0; i != Filters.Size; i++)
    {
        ImGuiTextRange& f = Filters[i];
        while (f.b < f.e && ImCharIsBlankA(f.b[0]))
            f.b++;
        while (f.e > f.b && ImCharIsBlankA(f.e[-1]))
            f.e--;
        if (f.empty())
            continue;
        if (Filters[i].b[0] != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (Filters.empty())
        return true;

    if (text == NULL)
        text = "";

    for (int i = 0; i != Filters.Size; i++)
    {
        const ImGuiTextRange& f = Filters[i];
        if (f.empty())
            continue;
        if (f.b[0] == '-')
        {
            // Subtract
            if (ImStristr(text, text_end, f.b + 1, f.e) != NULL)
                return false;
        }
        else
        {
            // Grep
            if (ImStristr(text, text_end, f.b, f.e) != NULL)
                return true;
        }
    }

    // Implicit * grep
    if (CountGrep == 0)
        return true;

    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextBuffer
//-----------------------------------------------------------------------------

// On some platform vsnprintf() takes va_list by reference and modifies it.
// va_copy is the 'correct' way to copy a va_list but Visual Studio prior to 2013 doesn't have it.
#ifndef va_copy
#if defined(__GNUC__) || defined(__clang__)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#else
#define va_copy(dest, src) (dest = src)
#endif
#endif

char ImGuiTextBuffer::EmptyString[1] = { 0 };

void ImGuiTextBuffer::append(const char* str, const char* str_end)
{
    int len = str_end ? (int)(str_end - str) : (int)strlen(str);

    // Add zero-terminator the first time
    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    memcpy(&Buf[write_off - 1], str, (size_t)len);
    Buf[write_off - 1 + len] = 0;
}

void ImGuiTextBuffer::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

// Helper: Text buffer for logging/accumulating text
void ImGuiTextBuffer::appendfv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = ImFormatStringV(NULL, 0, fmt, args);         // FIXME-OPT: could do a first pass write attempt, likely successful on first pass.
    if (len <= 0)
    {
        va_end(args_copy);
        return;
    }

    // Add zero-terminator the first time
    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off - 1], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiListClipper
// This is currently not as flexible/powerful as it should be and really confusing/spaghetti, mostly because we changed
// the API mid-way through development and support two ways to using the clipper, needs some rework (see TODO)
//-----------------------------------------------------------------------------

// Helper to calculate coarse clipping of large list of evenly sized items.
// NB: Prefer using the ImGuiListClipper higher-level helper if you can! Read comments and instructions there on how those use this sort of pattern.
// NB: 'items_count' is only used to clamp the result, if you don't know your count you can use INT_MAX
void ImGui::CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.LogEnabled)
    {
        // If logging is active, do not perform any clipping
        *out_items_display_start = 0;
        *out_items_display_end = items_count;
        return;
    }
    if (window->SkipItems)
    {
        *out_items_display_start = *out_items_display_end = 0;
        return;
    }

    // We create the union of the ClipRect and the NavScoringRect which at worst should be 1 page away from ClipRect
    ImRect unclipped_rect = window->ClipRect;
    if (g.NavMoveRequest)
        unclipped_rect.Add(g.NavScoringRectScreen);

    const ImVec2 pos = window->DC.CursorPos;
    int start = (int)((unclipped_rect.Min.y - pos.y) / items_height);
    int end = (int)((unclipped_rect.Max.y - pos.y) / items_height);

    // When performing a navigation request, ensure we have one item extra in the direction we are moving to
    if (g.NavMoveRequest && g.NavMoveClipDir == ImGuiDir_Up)
        start--;
    if (g.NavMoveRequest && g.NavMoveClipDir == ImGuiDir_Down)
        end++;

    start = ImClamp(start, 0, items_count);
    end = ImClamp(end + 1, start, items_count);
    *out_items_display_start = start;
    *out_items_display_end = end;
}

static void SetCursorPosYAndSetupDummyPrevLine(float pos_y, float line_height)
{
    // Set cursor position and a few other things so that SetScrollHereY() and Columns() can work when seeking cursor.
    // FIXME: It is problematic that we have to do that here, because custom/equivalent end-user code would stumble on the same issue.
    // The clipper should probably have a 4th step to display the last item in a regular manner.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DC.CursorPos.y = pos_y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, pos_y);
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y - line_height;  // Setting those fields so that SetScrollHereY() can properly function after the end of our clipper usage.
    window->DC.PrevLineSize.y = (line_height - g.Style.ItemSpacing.y);      // If we end up needing more accurate data (to e.g. use SameLine) we may as well make the clipper have a fourth step to let user process and display the last item in their list.
    if (ImGuiColumns* columns = window->DC.CurrentColumns)
        columns->LineMinY = window->DC.CursorPos.y;                         // Setting this so that cell Y position are set properly
}

// Use case A: Begin() called from constructor with items_height<0, then called again from Sync() in StepNo 1
// Use case B: Begin() called from constructor with items_height>0
// FIXME-LEGACY: Ideally we should remove the Begin/End functions but they are part of the legacy API we still support. This is why some of the code in Step() calling Begin() and reassign some fields, spaghetti style.
void ImGuiListClipper::Begin(int count, float items_height)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    StartPosY = window->DC.CursorPos.y;
    ItemsHeight = items_height;
    ItemsCount = count;
    StepNo = 0;
    DisplayEnd = DisplayStart = -1;
    if (ItemsHeight > 0.0f)
    {
        ImGui::CalcListClipping(ItemsCount, ItemsHeight, &DisplayStart, &DisplayEnd); // calculate how many to clip/display
        if (DisplayStart > 0)
            SetCursorPosYAndSetupDummyPrevLine(StartPosY + DisplayStart * ItemsHeight, ItemsHeight); // advance cursor
        StepNo = 2;
    }
}

void ImGuiListClipper::End()
{
    if (ItemsCount < 0)
        return;
    // In theory here we should assert that ImGui::GetCursorPosY() == StartPosY + DisplayEnd * ItemsHeight, but it feels saner to just seek at the end and not assert/crash the user.
    if (ItemsCount < INT_MAX)
        SetCursorPosYAndSetupDummyPrevLine(StartPosY + ItemsCount * ItemsHeight, ItemsHeight); // advance cursor
    ItemsCount = -1;
    StepNo = 3;
}

bool ImGuiListClipper::Step()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (ItemsCount == 0 || window->SkipItems)
    {
        ItemsCount = -1;
        return false;
    }
    if (StepNo == 0) // Step 0: the clipper let you process the first element, regardless of it being visible or not, so we can measure the element height.
    {
        DisplayStart = 0;
        DisplayEnd = 1;
        StartPosY = window->DC.CursorPos.y;
        StepNo = 1;
        return true;
    }
    if (StepNo == 1) // Step 1: the clipper infer height from first element, calculate the actual range of elements to display, and position the cursor before the first element.
    {
        if (ItemsCount == 1) { ItemsCount = -1; return false; }
        float items_height = window->DC.CursorPos.y - StartPosY;
        IM_ASSERT(items_height > 0.0f);   // If this triggers, it means Item 0 hasn't moved the cursor vertically
        Begin(ItemsCount - 1, items_height);
        DisplayStart++;
        DisplayEnd++;
        StepNo = 3;
        return true;
    }
    if (StepNo == 2) // Step 2: dummy step only required if an explicit items_height was passed to constructor or Begin() and user still call Step(). Does nothing and switch to Step 3.
    {
        IM_ASSERT(DisplayStart >= 0 && DisplayEnd >= 0);
        StepNo = 3;
        return true;
    }
    if (StepNo == 3) // Step 3: the clipper validate that we have reached the expected Y position (corresponding to element DisplayEnd), advance the cursor to the end of the list and then returns 'false' to end the loop.
        End();
    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] RENDER HELPERS
// Those (internal) functions are currently quite a legacy mess - their signature and behavior will change.
// Also see imgui_draw.cpp for some more which have been reworked to not rely on ImGui:: state.
//-----------------------------------------------------------------------------

const char* ImGui::FindRenderedTextEnd(const char* text, const char* text_end)
{
    const char* text_display_end = text;
    if (!text_end)
        text_end = (const char*)-1;

    while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;
    return text_display_end;
}

// Internal ImGui functions to render text
// RenderText***() functions calls ImDrawList::AddText() calls ImBitmapFont::RenderText()
void ImGui::RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Hide anything after a '##' string
    const char* text_display_end;
    if (hide_text_after_hash)
    {
        text_display_end = FindRenderedTextEnd(text, text_end);
    }
    else
    {
        if (!text_end)
            text_end = text + strlen(text); // FIXME-OPT
        text_display_end = text_end;
    }

    if (text != text_display_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_display_end);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_display_end);
    }
}

void ImGui::RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = text + strlen(text); // FIXME-OPT

    if (text != text_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_end, wrap_width);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_end);
    }
}

// Default clip_rect uses (pos_min,pos_max)
// Handle clipping on CPU immediately (vs typically let the GPU clip the triangles that are overlapping the clipping rectangle edges)
void ImGui::RenderTextClippedEx(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_display_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Perform CPU side clipping for single clipped element to avoid using scissor state
    ImVec2 pos = pos_min;
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_display_end, false, 0.0f);

    const ImVec2* clip_min = clip_rect ? &clip_rect->Min : &pos_min;
    const ImVec2* clip_max = clip_rect ? &clip_rect->Max : &pos_max;
    bool need_clipping = (pos.x + text_size.x >= clip_max->x) || (pos.y + text_size.y >= clip_max->y);
    if (clip_rect) // If we had no explicit clipping rectangle then pos==clip_min
        need_clipping |= (pos.x < clip_min->x) || (pos.y < clip_min->y);

    // Align whole block. We should defer that to the better rendering function when we'll have support for individual line alignment.
    if (align.x > 0.0f) pos.x = ImMax(pos.x, pos.x + (pos_max.x - pos.x - text_size.x) * align.x);
    if (align.y > 0.0f) pos.y = ImMax(pos.y, pos.y + (pos_max.y - pos.y - text_size.y) * align.y);

    // Render
    if (need_clipping)
    {
        ImVec4 fine_clip_rect(clip_min->x, clip_min->y, clip_max->x, clip_max->y);
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, &fine_clip_rect);
    }
    else
    {
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, NULL);
    }
}

void ImGui::RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Hide anything after a '##' string
    const char* text_display_end = FindRenderedTextEnd(text, text_end);
    const int text_len = (int)(text_display_end - text);
    if (text_len == 0)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    RenderTextClippedEx(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, align, clip_rect);
    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_display_end);
}


// Another overly complex function until we reorganize everything into a nice all-in-one helper.
// This is made more complex because we have dissociated the layout rectangle (pos_min..pos_max) which define _where_ the ellipsis is, from actual clipping of text and limit of the ellipsis display.
// This is because in the context of tabs we selectively hide part of the text when the Close Button appears, but we don't want the ellipsis to move. 
void    ImGui::RenderTextEllipsis(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, float clip_max_x, float ellipsis_max_x, const char* text, const char* text_end_full, const ImVec2* text_size_if_known)
{
    ImGuiContext& g = *GImGui;
    if (text_end_full == NULL)
        text_end_full = FindRenderedTextEnd(text);
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_end_full, false, 0.0f);

    if (text_size.x > pos_max.x - pos_min.x)
    {
        // Hello wo...
        // |       |   |
        // min   max   ellipsis_max
        //          <-> this is generally some padding value

        // FIXME-STYLE: RenderPixelEllipsis() style should use actual font data.
        const ImFont* font = draw_list->_Data->Font;
        const float font_size = draw_list->_Data->FontSize;
        const int ellipsis_dot_count = 3;
        const float ellipsis_width = (1.0f + 1.0f) * ellipsis_dot_count - 1.0f;
        const char* text_end_ellipsis = NULL;

        float text_width = ImMax((pos_max.x - ellipsis_width) - pos_min.x, 1.0f);
        float text_size_clipped_x = font->CalcTextSizeA(font_size, text_width, 0.0f, text, text_end_full, &text_end_ellipsis).x;
        if (text == text_end_ellipsis && text_end_ellipsis < text_end_full)
        {
            // Always display at least 1 character if there's no room for character + ellipsis
            text_end_ellipsis = text + ImTextCountUtf8BytesFromChar(text, text_end_full);
            text_size_clipped_x = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text, text_end_ellipsis).x;
        }
        while (text_end_ellipsis > text && ImCharIsBlankA(text_end_ellipsis[-1]))
        {
            // Trim trailing space before ellipsis
            text_end_ellipsis--;
            text_size_clipped_x -= font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_end_ellipsis, text_end_ellipsis + 1).x; // Ascii blanks are always 1 byte
        }
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_ellipsis, &text_size, ImVec2(0.0f, 0.0f));

        const float ellipsis_x = pos_min.x + text_size_clipped_x + 1.0f;
        if (ellipsis_x + ellipsis_width - 1.0f <= ellipsis_max_x)
            RenderPixelEllipsis(draw_list, ImVec2(ellipsis_x, pos_min.y), GetColorU32(ImGuiCol_Text), ellipsis_dot_count);
    }
    else
    {
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_full, &text_size, ImVec2(0.0f, 0.0f));
    }

    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_end_full);
}

// Render a rectangle shaped with optional rounding and borders
void ImGui::RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
    const float border_size = g.Style.FrameBorderSize;
    if (border && border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min+ImVec2(1,1), p_max+ImVec2(1,1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
}

void ImGui::RenderFrameBorder(ImVec2 p_min, ImVec2 p_max, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const float border_size = g.Style.FrameBorderSize;
    if (border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min+ImVec2(1,1), p_max+ImVec2(1,1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
}

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f,+0.750f) * r;
        b = ImVec2(-0.866f,-0.750f) * r;
        c = ImVec2(+0.866f,-0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f,+0.000f) * r;
        b = ImVec2(-0.750f,+0.866f) * r;
        c = ImVec2(-0.750f,-0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

void ImGui::RenderBullet(ImDrawList* draw_list, ImVec2 pos, ImU32 col)
{
    draw_list->AddCircleFilled(pos, draw_list->_Data->FontSize * 0.20f, col, 8);
}

void ImGui::RenderCheckMark(ImVec2 pos, ImU32 col, float sz)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness*0.5f;
    pos += ImVec2(thickness*0.25f, thickness*0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third*0.5f;
    window->DrawList->PathLineTo(ImVec2(bx - third, by - third));
    window->DrawList->PathLineTo(ImVec2(bx, by));
    window->DrawList->PathLineTo(ImVec2(bx + third*2, by - third*2));
    window->DrawList->PathStroke(col, false, thickness);
}

void ImGui::RenderNavHighlight(const ImRect& bb, ImGuiID id, ImGuiNavHighlightFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (id != g.NavId)
        return;
    if (g.NavDisableHighlight && !(flags & ImGuiNavHighlightFlags_AlwaysDraw))
        return;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.NavHideHighlightOneFrame)
        return;

    float rounding = (flags & ImGuiNavHighlightFlags_NoRounding) ? 0.0f : g.Style.FrameRounding;
    ImRect display_rect = bb;
    display_rect.ClipWith(window->ClipRect);
    if (flags & ImGuiNavHighlightFlags_TypeDefault)
    {
        const float THICKNESS = 2.0f;
        const float DISTANCE = 3.0f + THICKNESS * 0.5f;
        display_rect.Expand(ImVec2(DISTANCE,DISTANCE));
        bool fully_visible = window->ClipRect.Contains(display_rect);
        if (!fully_visible)
            window->DrawList->PushClipRect(display_rect.Min, display_rect.Max);
        window->DrawList->AddRect(display_rect.Min + ImVec2(THICKNESS*0.5f,THICKNESS*0.5f), display_rect.Max - ImVec2(THICKNESS*0.5f,THICKNESS*0.5f), GetColorU32(ImGuiCol_NavHighlight), rounding, ImDrawCornerFlags_All, THICKNESS);
        if (!fully_visible)
            window->DrawList->PopClipRect();
    }
    if (flags & ImGuiNavHighlightFlags_TypeThin)
    {
        window->DrawList->AddRect(display_rect.Min, display_rect.Max, GetColorU32(ImGuiCol_NavHighlight), rounding, ~0, 1.0f);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
//-----------------------------------------------------------------------------

// ImGuiWindow is mostly a dumb struct. It merely has a constructor and a few helper methods
ImGuiWindow::ImGuiWindow(ImGuiContext* context, const char* name)
    : DrawListInst(&context->DrawListSharedData)
{
    Name = ImStrdup(name);
    ID = ImHashStr(name);
    IDStack.push_back(ID);
    Flags = FlagsPreviousFrame = ImGuiWindowFlags_None;
    Viewport = NULL;
    ViewportId = 0;
    ViewportAllowPlatformMonitorExtend = -1;
    ViewportPos = ImVec2(FLT_MAX, FLT_MAX);
    Pos = ImVec2(0.0f, 0.0f);
    Size = SizeFull = ImVec2(0.0f, 0.0f);
    ContentSize = ContentSizeExplicit = ImVec2(0.0f, 0.0f);
    WindowPadding = ImVec2(0.0f, 0.0f);
    WindowRounding = 0.0f;
    WindowBorderSize = 0.0f;
    NameBufLen = (int)strlen(name) + 1;
    MoveId = GetID("#MOVE");
    ChildId = 0;
    Scroll = ImVec2(0.0f, 0.0f);
    ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
    ScrollTargetCenterRatio = ImVec2(0.5f, 0.5f);
    ScrollbarSizes = ImVec2(0.0f, 0.0f);
    ScrollbarX = ScrollbarY = false;
    ViewportOwned = false;
    Active = WasActive = false;
    WriteAccessed = false;
    Collapsed = false;
    WantCollapseToggle = false;
    SkipItems = false;
    Appearing = false;
    Hidden = false;
    IsFallbackWindow = false;
    HasCloseButton = false;
    ResizeBorderHeld = -1;
    BeginCount = 0;
    BeginOrderWithinParent = -1;
    BeginOrderWithinContext = -1;
    PopupId = 0;
    AutoFitFramesX = AutoFitFramesY = -1;
    AutoFitOnlyGrows = false;
    AutoFitChildAxises = 0x00;
    AutoPosLastDirection = ImGuiDir_None;
    HiddenFramesCanSkipItems = HiddenFramesCannotSkipItems = 0;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = SetWindowDockAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;
    SetWindowPosVal = SetWindowPosPivot = ImVec2(FLT_MAX, FLT_MAX);

    LastFrameActive = -1;
    LastFrameJustFocused = -1;
    LastTimeActive = -1.0f;
    ItemWidthDefault = 0.0f;
    FontWindowScale = FontDpiScale = 1.0f;
    SettingsIdx = -1;

    DrawList = &DrawListInst;
    DrawList->_OwnerName = Name;
    ParentWindow = NULL;
    RootWindow = NULL;
    RootWindowDockStop = NULL;
    RootWindowForTitleBarHighlight = NULL;
    RootWindowForNav = NULL;

    NavLastIds[0] = NavLastIds[1] = 0;
    NavRectRel[0] = NavRectRel[1] = ImRect();
    NavLastChildNavWindow = NULL;

    MemoryCompacted = false;
    MemoryDrawListIdxCapacity = MemoryDrawListVtxCapacity = 0;

    DockNode = DockNodeAsHost = NULL;
    DockId = 0;
    DockTabItemStatusFlags = ImGuiItemStatusFlags_None;
    DockOrder = -1;
    DockIsActive = DockTabIsVisible = DockTabWantClose = false;
}

ImGuiWindow::~ImGuiWindow()
{
    IM_ASSERT(DrawList == &DrawListInst);
    IM_DELETE(Name);
    for (int i = 0; i != ColumnsStorage.Size; i++)
        ColumnsStorage[i].~ImGuiColumns();
}

ImGuiID ImGuiWindow::GetID(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&ptr, sizeof(void*), seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetID(int n)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&n, sizeof(n), seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    return ImHashStr(str, str_end ? (str_end - str) : 0, seed);
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    return ImHashData(&ptr, sizeof(void*), seed);
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(int n)
{
    ImGuiID seed = IDStack.back();
    return ImHashData(&n, sizeof(n), seed);
}

// This is only used in rare/specific situations to manufacture an ID out of nowhere.
ImGuiID ImGuiWindow::GetIDFromRectangle(const ImRect& r_abs)
{
    ImGuiID seed = IDStack.back();
    const int r_rel[4] = { (int)(r_abs.Min.x - Pos.x), (int)(r_abs.Min.y - Pos.y), (int)(r_abs.Max.x - Pos.x), (int)(r_abs.Max.y - Pos.y) };
    ImGuiID id = ImHashData(&r_rel, sizeof(r_rel), seed);
    ImGui::KeepAliveID(id);
    return id;
}

static void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    if (window)
        g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

// Free up/compact internal window buffers, we can use this when a window becomes unused.
// This is currently unused by the library, but you may call this yourself for easy GC.
// Not freed:
// - ImGuiWindow, ImGuiWindowSettings, Name
// - StateStorage, ColumnsStorage (may hold useful data)
// This should have no noticeable visual effect. When the window reappear however, expect new allocation/buffer growth/copy cost.
void ImGui::GcCompactTransientWindowBuffers(ImGuiWindow* window)
{
    window->MemoryCompacted = true;
    window->MemoryDrawListIdxCapacity = window->DrawList->IdxBuffer.Capacity;
    window->MemoryDrawListVtxCapacity = window->DrawList->VtxBuffer.Capacity;
    window->IDStack.clear();
    window->DrawList->ClearFreeMemory();
    window->DC.ChildWindows.clear();
    window->DC.ItemFlagsStack.clear();
    window->DC.ItemWidthStack.clear();
    window->DC.TextWrapPosStack.clear();
    window->DC.GroupStack.clear();
}

void ImGui::GcAwakeTransientWindowBuffers(ImGuiWindow* window)
{
    // We stored capacity of the ImDrawList buffer to reduce growth-caused allocation/copy when awakening.
    // The other buffers tends to amortize much faster.
    window->MemoryCompacted = false;
    window->DrawList->IdxBuffer.reserve(window->MemoryDrawListIdxCapacity);
    window->DrawList->VtxBuffer.reserve(window->MemoryDrawListVtxCapacity);
    window->MemoryDrawListIdxCapacity = window->MemoryDrawListVtxCapacity = 0;
}

void ImGui::SetNavID(ImGuiID id, int nav_layer)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindow);
    IM_ASSERT(nav_layer == 0 || nav_layer == 1);
    g.NavId = id;
    g.NavWindow->NavLastIds[nav_layer] = id;
}

void ImGui::SetNavIDWithRectRel(ImGuiID id, int nav_layer, const ImRect& rect_rel)
{
    ImGuiContext& g = *GImGui;
    SetNavID(id, nav_layer);
    g.NavWindow->NavRectRel[nav_layer] = rect_rel;
    g.NavMousePosDirty = true;
    g.NavDisableHighlight = false;
    g.NavDisableMouseHover = true;
}

void ImGui::SetActiveID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.ActiveIdIsJustActivated = (g.ActiveId != id);
    if (g.ActiveIdIsJustActivated)
    {
        g.ActiveIdTimer = 0.0f;
        g.ActiveIdHasBeenPressedBefore = false;
        g.ActiveIdHasBeenEditedBefore = false;
        if (id != 0)
        {
            g.LastActiveId = id;
            g.LastActiveIdTimer = 0.0f;
        }
    }
    g.ActiveId = id;
    g.ActiveIdAllowNavDirFlags = 0;
    g.ActiveIdBlockNavInputFlags = 0;
    g.ActiveIdAllowOverlap = false;
    g.ActiveIdWindow = window;
    g.ActiveIdHasBeenEditedThisFrame = false;
    if (id)
    {
        g.ActiveIdIsAlive = id;
        g.ActiveIdSource = (g.NavActivateId == id || g.NavInputId == id || g.NavJustTabbedId == id || g.NavJustMovedToId == id) ? ImGuiInputSource_Nav : ImGuiInputSource_Mouse;
    }
}

// FIXME-NAV: The existence of SetNavID/SetNavIDWithRectRel/SetFocusID is incredibly messy and confusing and needs some explanation or refactoring.
void ImGui::SetFocusID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    // Assume that SetFocusID() is called in the context where its NavLayer is the current layer, which is the case everywhere we call it.
    const ImGuiNavLayer nav_layer = window->DC.NavLayerCurrent;
    if (g.NavWindow != window)
        g.NavInitRequest = false;
    g.NavId = id;
    g.NavWindow = window;
    g.NavLayer = nav_layer;
    window->NavLastIds[nav_layer] = id;
    if (window->DC.LastItemId == id)
        window->NavRectRel[nav_layer] = ImRect(window->DC.LastItemRect.Min - window->Pos, window->DC.LastItemRect.Max - window->Pos);

    if (g.ActiveIdSource == ImGuiInputSource_Nav)
        g.NavDisableMouseHover = true;
    else
        g.NavDisableHighlight = true;
}

void ImGui::ClearActiveID()
{
    SetActiveID(0, NULL);
}

void ImGui::SetHoveredID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.HoveredId = id;
    g.HoveredIdAllowOverlap = false;
    if (id != 0 && g.HoveredIdPreviousFrame != id)
        g.HoveredIdTimer = g.HoveredIdNotActiveTimer = 0.0f;
}

ImGuiID ImGui::GetHoveredID()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId ? g.HoveredId : g.HoveredIdPreviousFrame;
}

void ImGui::KeepAliveID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
        g.ActiveIdIsAlive = id;
    if (g.ActiveIdPreviousFrame == id)
        g.ActiveIdPreviousFrameIsAlive = true;
}

void ImGui::MarkItemEdited(ImGuiID id)
{
    // This marking is solely to be able to provide info for IsItemDeactivatedAfterEdit().
    // ActiveId might have been released by the time we call this (as in the typical press/release button behavior) but still need need to fill the data.
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == id || g.ActiveId == 0 || g.DragDropActive);
    IM_UNUSED(id); // Avoid unused variable warnings when asserts are compiled out.
    //IM_ASSERT(g.CurrentWindow->DC.LastItemId == id);
    g.ActiveIdHasBeenEditedThisFrame = true;
    g.ActiveIdHasBeenEditedBefore = true;
    g.CurrentWindow->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Edited;
}

static inline bool IsWindowContentHoverable(ImGuiWindow* window, ImGuiHoveredFlags flags)
{
    // An active popup disable hovering on other windows (apart from its own children)
    // FIXME-OPT: This could be cached/stored within the window.
    ImGuiContext& g = *GImGui;
    if (g.NavWindow)
        if (ImGuiWindow* focused_root_window = g.NavWindow->RootWindow)
            if (focused_root_window->WasActive && focused_root_window != window->RootWindow)
            {
                // For the purpose of those flags we differentiate "standard popup" from "modal popup"
                // NB: The order of those two tests is important because Modal windows are also Popups.
                if (focused_root_window->Flags & ImGuiWindowFlags_Modal)
                    return false;
                if ((focused_root_window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                    return false;
            }

    // Filter by viewport
    if (window->Viewport != g.MouseViewport)
        if (g.MovingWindow == NULL || window->RootWindow != g.MovingWindow->RootWindow)
            return false;

    return true;
}

// Advance cursor given item size for layout.
void ImGui::ItemSize(const ImVec2& size, float text_offset_y)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    // Always align ourselves on pixel boundaries
    const float line_height = ImMax(window->DC.CurrLineSize.y, size.y);
    const float text_base_offset = ImMax(window->DC.CurrLineTextBaseOffset, text_offset_y);
    //if (g.IO.KeyAlt) window->DrawList->AddRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size.x, line_height), IM_COL32(255,0,0,200)); // [DEBUG]
    window->DC.CursorPosPrevLine.x = window->DC.CursorPos.x + size.x;
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y;
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->DC.CursorPos.y = (float)(int)(window->DC.CursorPos.y + line_height + g.Style.ItemSpacing.y);
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPosPrevLine.x);
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y - g.Style.ItemSpacing.y);
    //if (g.IO.KeyAlt) window->DrawList->AddCircle(window->DC.CursorMaxPos, 3.0f, IM_COL32(255,0,0,255), 4); // [DEBUG]

    window->DC.PrevLineSize.y = line_height;
    window->DC.PrevLineTextBaseOffset = text_base_offset;
    window->DC.CurrLineSize.y = window->DC.CurrLineTextBaseOffset = 0.0f;

    // Horizontal layout mode
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        SameLine();
}

void ImGui::ItemSize(const ImRect& bb, float text_offset_y)
{
    ItemSize(bb.GetSize(), text_offset_y);
}

// Declare item bounding box for clipping and interaction.
// Note that the size can be different than the one provided to ItemSize(). Typically, widgets that spread over available surface
// declare their minimum size requirement to ItemSize() and then use a larger region for drawing/interaction, which is passed to ItemAdd().
bool ImGui::ItemAdd(const ImRect& bb, ImGuiID id, const ImRect* nav_bb_arg)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (id != 0)
    {
        // Navigation processing runs prior to clipping early-out
        //  (a) So that NavInitRequest can be honored, for newly opened windows to select a default widget
        //  (b) So that we can scroll up/down past clipped items. This adds a small O(N) cost to regular navigation requests 
        //      unfortunately, but it is still limited to one window. It may not scale very well for windows with ten of 
        //      thousands of item, but at least NavMoveRequest is only set on user interaction, aka maximum once a frame.
        //      We could early out with "if (is_clipped && !g.NavInitRequest) return false;" but when we wouldn't be able
        //      to reach unclipped widgets. This would work if user had explicit scrolling control (e.g. mapped on a stick).
        // We intentionally don't check if g.NavWindow != NULL because g.NavAnyRequest should only be set when it is non null.
        // If we crash on a NULL g.NavWindow we need to fix the bug elsewhere.
        window->DC.NavLayerActiveMaskNext |= window->DC.NavLayerCurrentMask;
        if (g.NavId == id || g.NavAnyRequest)
            if (g.NavWindow->RootWindowForNav == window->RootWindowForNav)
                if (window == g.NavWindow || ((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened))
                    NavProcessItem(window, nav_bb_arg ? *nav_bb_arg : bb, id);

        // [DEBUG] Item Picker tool, when enabling the "extended" version we perform the check in ItemAdd()
#ifdef IMGUI_DEBUG_TOOL_ITEM_PICKER_EX
        if (id == g.DebugItemPickerBreakID)
        {
            IM_DEBUG_BREAK();
            g.DebugItemPickerBreakID = 0;
        }
#endif
    }

    window->DC.LastItemId = id;
    window->DC.LastItemRect = bb;
    window->DC.LastItemStatusFlags = ImGuiItemStatusFlags_None;
    g.NextItemData.Flags = ImGuiNextItemDataFlags_None;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (id != 0)
        IMGUI_TEST_ENGINE_ITEM_ADD(nav_bb_arg ? *nav_bb_arg : bb, id);
#endif

    // Clipping test
    const bool is_clipped = IsClippedEx(bb, id, false);
    if (is_clipped)
        return false;
    //if (g.IO.KeyAlt) window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(255,255,0,120)); // [DEBUG]

    // We need to calculate this now to take account of the current clipping rectangle (as items like Selectable may change them)
    if (IsMouseHoveringRect(bb.Min, bb.Max))
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HoveredRect;
    return true;
}

// This is roughly matching the behavior of internal-facing ItemHoverable()
// - we allow hovering to be true when ActiveId==window->MoveID, so that clicking on non-interactive items such as a Text() item still returns true with IsItemHovered()
// - this should work even for non-interactive items that have no ID, so we cannot use LastItemId
bool ImGui::IsItemHovered(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavDisableMouseHover && !g.NavDisableHighlight)
        return IsItemFocused();

    // Test for bounding box overlap, as updated as ItemAdd()
    if (!(window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    IM_ASSERT((flags & (ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows)) == 0);   // Flags not supported by this function

    // Test if we are hovering the right window (our window could be behind another window)
    // [2017/10/16] Reverted commit 344d48be3 and testing RootWindow instead. I believe it is correct to NOT test for RootWindow but this leaves us unable to use IsItemHovered() after EndChild() itself.
    // Until a solution is found I believe reverting to the test from 2017/09/27 is safe since this was the test that has been running for a long while.
    //if (g.HoveredWindow != window)
    //    return false;
    if (g.HoveredRootWindow != window->RootWindow && !(flags & ImGuiHoveredFlags_AllowWhenOverlapped))
        return false;

    // Test if another item is active (e.g. being dragged)
    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && g.ActiveId != window->DC.LastItemId && !g.ActiveIdAllowOverlap && g.ActiveId != window->MoveId)
            return false;

    // Test if interactions on this window are blocked by an active popup or modal.
    // The ImGuiHoveredFlags_AllowWhenBlockedByPopup flag will be tested here.
    if (!IsWindowContentHoverable(window, flags))
        return false;

    // Test if the item is disabled
    if ((window->DC.ItemFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
        return false;

    // Special handling for the dummy item after Begin() which represent the title bar or tab.
    // When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect the case.
    if ((window->DC.LastItemId == window->ID || window->DC.LastItemId == window->MoveId) && window->WriteAccessed)
        return false;
    return true;
}

// Internal facing ItemHoverable() used when submitting widgets. Differs slightly from IsItemHovered().
bool ImGui::ItemHoverable(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId != 0 && g.HoveredId != id && !g.HoveredIdAllowOverlap)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindow != window)
        return false;
    if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
        return false;
    if (!IsMouseHoveringRect(bb.Min, bb.Max))
        return false;
    if (g.NavDisableMouseHover || !IsWindowContentHoverable(window, ImGuiHoveredFlags_None))
        return false;
    if (window->DC.ItemFlags & ImGuiItemFlags_Disabled)
        return false;

    SetHoveredID(id);

    // [DEBUG] Item Picker tool!
    // We perform the check here because SetHoveredID() is not frequently called (1~ time a frame), making
    // the cost of this tool near-zero. We can get slightly better call-stack and support picking non-hovered
    // items if we perform the test in ItemAdd(), but that would incur a small runtime cost.
    // #define IMGUI_DEBUG_TOOL_ITEM_PICKER_EX in imconfig.h if you want this check to also be performed in ItemAdd().
    if (g.DebugItemPickerActive && g.HoveredIdPreviousFrame == id)
        GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 0, 255));
    if (g.DebugItemPickerBreakID == id)
        IM_DEBUG_BREAK();

    return true;
}

bool ImGui::IsClippedEx(const ImRect& bb, ImGuiID id, bool clip_even_when_logged)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!bb.Overlaps(window->ClipRect))
        if (id == 0 || id != g.ActiveId)
            if (clip_even_when_logged || !g.LogEnabled)
                return true;
    return false;
}

// Process TAB/Shift+TAB. Be mindful that this function may _clear_ the ActiveID when tabbing out.
bool ImGui::FocusableItemRegister(ImGuiWindow* window, ImGuiID id)
{
    ImGuiContext& g = *GImGui;

    // Increment counters
    const bool is_tab_stop = (window->DC.ItemFlags & (ImGuiItemFlags_NoTabStop | ImGuiItemFlags_Disabled)) == 0;
    window->DC.FocusCounterAll++;
    if (is_tab_stop)
        window->DC.FocusCounterTab++;

    // Process TAB/Shift-TAB to tab *OUT* of the currently focused item.
    // (Note that we can always TAB out of a widget that doesn't allow tabbing in)
    if (g.ActiveId == id && g.FocusTabPressed && !(g.ActiveIdBlockNavInputFlags & (1 << ImGuiNavInput_KeyTab_)) && g.FocusRequestNextWindow == NULL)
    {
        g.FocusRequestNextWindow = window;
        g.FocusRequestNextCounterTab = window->DC.FocusCounterTab + (g.IO.KeyShift ? (is_tab_stop ? -1 : 0) : +1); // Modulo on index will be applied at the end of frame once we've got the total counter of items.
    }

    // Handle focus requests
    if (g.FocusRequestCurrWindow == window)
    {
        if (window->DC.FocusCounterAll == g.FocusRequestCurrCounterAll)
            return true;
        if (is_tab_stop && window->DC.FocusCounterTab == g.FocusRequestCurrCounterTab)
        {
            g.NavJustTabbedId = id;
            return true;
        }

        // If another item is about to be focused, we clear our own active id
        if (g.ActiveId == id)
            ClearActiveID();
    }

    return false;
}

void ImGui::FocusableItemUnregister(ImGuiWindow* window)
{
    window->DC.FocusCounterAll--;
    window->DC.FocusCounterTab--;
}

float ImGui::CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiWindow* window = GImGui->CurrentWindow;
    if (wrap_pos_x == 0.0f)
        wrap_pos_x = window->WorkRect.Max.x;
    else if (wrap_pos_x > 0.0f)
        wrap_pos_x += window->Pos.x - window->Scroll.x; // wrap_pos_x is provided is window local space

    return ImMax(wrap_pos_x - pos.x, 1.0f);
}

// IM_ALLOC() == ImGui::MemAlloc()
void* ImGui::MemAlloc(size_t size)
{
    if (ImGuiContext* ctx = GImGui)
        ctx->IO.MetricsActiveAllocations++;
    return GImAllocatorAllocFunc(size, GImAllocatorUserData);
}

// IM_FREE() == ImGui::MemFree()
void ImGui::MemFree(void* ptr)
{
    if (ptr)
        if (ImGuiContext* ctx = GImGui)
            ctx->IO.MetricsActiveAllocations--;
    return GImAllocatorFreeFunc(ptr, GImAllocatorUserData);
}

const char* ImGui::GetClipboardText()
{
    ImGuiContext& g = *GImGui;
    return g.IO.GetClipboardTextFn ? g.IO.GetClipboardTextFn(g.IO.ClipboardUserData) : "";
}

void ImGui::SetClipboardText(const char* text)
{
    ImGuiContext& g = *GImGui;
    if (g.IO.SetClipboardTextFn)
        g.IO.SetClipboardTextFn(g.IO.ClipboardUserData, text);
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

// Internal state access - if you want to share Dear ImGui state between modules (e.g. DLL) or allocate it yourself
// Note that we still point to some static data and members (such as GFontAtlas), so the state instance you end up using will point to the static data within its module
ImGuiContext* ImGui::GetCurrentContext()
{
    return GImGui;
}

void ImGui::SetCurrentContext(ImGuiContext* ctx)
{
#ifdef IMGUI_SET_CURRENT_CONTEXT_FUNC
    IMGUI_SET_CURRENT_CONTEXT_FUNC(ctx); // For custom thread-based hackery you may want to have control over this.
#else
    GImGui = ctx;
#endif
}

// Helper function to verify ABI compatibility between caller code and compiled version of Dear ImGui.
// Verify that the type sizes are matching between the calling file's compilation unit and imgui.cpp's compilation unit
// If the user has inconsistent compilation settings, imgui configuration #define, packing pragma, etc. your user code
// may see different structures than what imgui.cpp sees, which is problematic.
// We usually require settings to be in imconfig.h to make sure that they are accessible to all compilation units involved with Dear ImGui.
bool ImGui::DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert, size_t sz_idx)
{
    bool error = false;
    if (strcmp(version, IMGUI_VERSION)!=0) { error = true; IM_ASSERT(strcmp(version,IMGUI_VERSION)==0 && "Mismatched version string!");  }
    if (sz_io    != sizeof(ImGuiIO))       { error = true; IM_ASSERT(sz_io    == sizeof(ImGuiIO)      && "Mismatched struct layout!"); }
    if (sz_style != sizeof(ImGuiStyle))    { error = true; IM_ASSERT(sz_style == sizeof(ImGuiStyle)   && "Mismatched struct layout!"); }
    if (sz_vec2  != sizeof(ImVec2))        { error = true; IM_ASSERT(sz_vec2  == sizeof(ImVec2)       && "Mismatched struct layout!"); }
    if (sz_vec4  != sizeof(ImVec4))        { error = true; IM_ASSERT(sz_vec4  == sizeof(ImVec4)       && "Mismatched struct layout!"); }
    if (sz_vert  != sizeof(ImDrawVert))    { error = true; IM_ASSERT(sz_vert  == sizeof(ImDrawVert)   && "Mismatched struct layout!"); }
    if (sz_idx   != sizeof(ImDrawIdx))     { error = true; IM_ASSERT(sz_idx   == sizeof(ImDrawIdx)    && "Mismatched struct layout!"); }
    return !error;
}

void ImGui::SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data)
{
    GImAllocatorAllocFunc = alloc_func;
    GImAllocatorFreeFunc = free_func;
    GImAllocatorUserData = user_data;
}

ImGuiContext* ImGui::CreateContext(ImFontAtlas* shared_font_atlas)
{
    ImGuiContext* ctx = IM_NEW(ImGuiContext)(shared_font_atlas);
    if (GImGui == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void ImGui::DestroyContext(ImGuiContext* ctx)
{
    if (ctx == NULL)
        ctx = GImGui;
    Shutdown(ctx);
    if (GImGui == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

ImGuiIO& ImGui::GetIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->IO;
}

ImGuiPlatformIO& ImGui::GetPlatformIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->PlatformIO;
}

ImGuiStyle& ImGui::GetStyle()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->Style;
}

// Same value as passed to the old io.RenderDrawListsFn function. Valid after Render() and until the next call to NewFrame()
ImDrawData* ImGui::GetDrawData()
{
    ImGuiContext& g = *GImGui;
    return g.Viewports[0]->DrawDataP.Valid ? &g.Viewports[0]->DrawDataP : NULL;
}

double ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

static ImDrawList* GetViewportDrawList(ImGuiViewportP* viewport, size_t drawlist_no, const char* drawlist_name)
{
    // Create the draw list on demand, because they are not frequently used for all viewports
    ImGuiContext& g = *GImGui;
    IM_ASSERT(drawlist_no >= 0 && drawlist_no < IM_ARRAYSIZE(viewport->DrawLists));
    ImDrawList* draw_list = viewport->DrawLists[drawlist_no];
    if (draw_list == NULL)
    {
        draw_list = IM_NEW(ImDrawList)(&g.DrawListSharedData);
        draw_list->_OwnerName = drawlist_name;
        viewport->DrawLists[drawlist_no] = draw_list;
    }

    // Our ImDrawList system requires that there is always a command
    if (viewport->LastFrameDrawLists[drawlist_no] != g.FrameCount)
    {
        draw_list->Clear();
        draw_list->PushTextureID(g.IO.Fonts->TexID);
        draw_list->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size, false);
        viewport->LastFrameDrawLists[drawlist_no] = g.FrameCount;
    }
    return draw_list;
}

ImDrawList* ImGui::GetBackgroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportDrawList((ImGuiViewportP*)viewport, 0, "##Background");
}

ImDrawList* ImGui::GetBackgroundDrawList()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return GetBackgroundDrawList(window->Viewport);
}

ImDrawList* ImGui::GetForegroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportDrawList((ImGuiViewportP*)viewport, 1, "##Foreground");
}

ImDrawList* ImGui::GetForegroundDrawList()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (window)
        return GetForegroundDrawList(window->Viewport);
    else
        return NULL;
}

ImDrawListSharedData* ImGui::GetDrawListSharedData()
{
    return &GImGui->DrawListSharedData;
}

void ImGui::StartMouseMovingWindow(ImGuiWindow* window)
{
    // Set ActiveId even if the _NoMove flag is set. Without it, dragging away from a window with _NoMove would activate hover on other windows.
    // We _also_ call this when clicking in a window empty space when io.ConfigWindowsMoveFromTitleBarOnly is set, but clear g.MovingWindow afterward.
    // This is because we want ActiveId to be set even when the window is not permitted to move.
    ImGuiContext& g = *GImGui;
    FocusWindow(window);
    SetActiveID(window->MoveId, window);
    g.NavDisableHighlight = true;
    g.ActiveIdClickOffset = g.IO.MousePos - window->RootWindow->Pos;

    bool can_move_window = true;
    if ((window->Flags & ImGuiWindowFlags_NoMove) || (window->RootWindow->Flags & ImGuiWindowFlags_NoMove))
        can_move_window = false;
    if (ImGuiDockNode* node = window->DockNodeAsHost)
        if (node->VisibleWindow && (node->VisibleWindow->Flags & ImGuiWindowFlags_NoMove))
            can_move_window = false;
    if (can_move_window)
        g.MovingWindow = window;
}

void ImGui::StartMouseDragFromTitleBar(ImGuiWindow* window, ImGuiDockNode* node, bool from_collapse_button)
{
    ImGuiContext& g = *GImGui;
    bool can_extract_dock_node = false;
    if (node != NULL && node->VisibleWindow && (node->VisibleWindow->Flags & ImGuiWindowFlags_NoMove) == 0)
    {
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        if (root_node->OnlyNodeWithWindows != node || (root_node->CentralNode != NULL))
            if (from_collapse_button || root_node->IsDockSpace())
                can_extract_dock_node = true;
    }

    const bool clicked = IsMouseClicked(0);
    const bool dragging = IsMouseDragging(0, g.IO.MouseDragThreshold * 1.70f);
    if (can_extract_dock_node && dragging)
    {
        DockContextQueueUndockNode(&g, node);
        g.ActiveIdClickOffset = g.IO.MouseClickedPos[0] - node->Pos;
    }
    else if (!can_extract_dock_node && (clicked || dragging) && g.MovingWindow != window)
    {
        StartMouseMovingWindow(window);
        g.ActiveIdClickOffset = g.IO.MouseClickedPos[0] - window->RootWindow->Pos;
    }
}

// Handle mouse moving window
// Note: moving window with the navigation keys (Square + d-pad / CTRL+TAB + Arrows) are processed in NavUpdateWindowing()
void ImGui::UpdateMouseMovingWindowNewFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.MovingWindow != NULL)
    {
        // We actually want to move the root window. g.MovingWindow == window we clicked on (could be a child window).
        // We track it to preserve Focus and so that generally ActiveIdWindow == MovingWindow and ActiveId == MovingWindow->MoveId for consistency.
        KeepAliveID(g.ActiveId);
        IM_ASSERT(g.MovingWindow && g.MovingWindow->RootWindow);
        ImGuiWindow* moving_window = g.MovingWindow->RootWindow;
        if (g.IO.MouseDown[0] && IsMousePosValid(&g.IO.MousePos))
        {
            ImVec2 pos = g.IO.MousePos - g.ActiveIdClickOffset;
            if (moving_window->Pos.x != pos.x || moving_window->Pos.y != pos.y)
            {
                MarkIniSettingsDirty(moving_window);
                SetWindowPos(moving_window, pos, ImGuiCond_Always);
                if (moving_window->ViewportOwned) // Synchronize viewport immediately because some overlays may relies on clipping rectangle before we Begin() into the window.
                    moving_window->Viewport->Pos = pos;
            }
            FocusWindow(g.MovingWindow);
        }
        else
        {
            // Try to merge the window back into the main viewport. 
            // This works because MouseViewport should be != MovingWindow->Viewport on release (as per code in UpdateViewports)
            if (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable)
                UpdateTryMergeWindowIntoHostViewport(moving_window, g.MouseViewport);

            // Restore the mouse viewport so that we don't hover the viewport _under_ the moved window during the frame we released the mouse button.
            if (!IsDragDropPayloadBeingAccepted())
                g.MouseViewport = moving_window->Viewport;

            // Clear the NoInput window flag set by the Viewport system
            moving_window->Viewport->Flags &= ~ImGuiViewportFlags_NoInputs;

            ClearActiveID();
            g.MovingWindow = NULL;
        }
    }
    else
    {
        // When clicking/dragging from a window that has the _NoMove flag, we still set the ActiveId in order to prevent hovering others.
        if (g.ActiveIdWindow && g.ActiveIdWindow->MoveId == g.ActiveId)
        {
            KeepAliveID(g.ActiveId);
            if (!g.IO.MouseDown[0])
                ClearActiveID();
        }
    }
}

// Initiate moving window, handle left-click and right-click focus
void ImGui::UpdateMouseMovingWindowEndFrame()
{
    // Initiate moving window
    ImGuiContext& g = *GImGui;
    if (g.ActiveId != 0 || g.HoveredId != 0)
        return;

    // Unless we just made a window/popup appear
    if (g.NavWindow && g.NavWindow->Appearing)
        return;

    // Click to focus window and start moving (after we're done with all our widgets)
    if (g.IO.MouseClicked[0])
    {
        if (g.HoveredRootWindow != NULL)
        {
            StartMouseMovingWindow(g.HoveredWindow);
            if (g.IO.ConfigWindowsMoveFromTitleBarOnly && (!(g.HoveredRootWindow->Flags & ImGuiWindowFlags_NoTitleBar) || g.HoveredWindow->RootWindowDockStop->DockIsActive))
                if (!g.HoveredRootWindow->TitleBarRect().Contains(g.IO.MouseClickedPos[0]))
                    g.MovingWindow = NULL;
        }
        else if (g.NavWindow != NULL && GetTopMostPopupModal() == NULL)
        {
            // Clicking on void disable focus
            FocusWindow(NULL);
        }
    }

    // With right mouse button we close popups without changing focus based on where the mouse is aimed
    // Instead, focus will be restored to the window under the bottom-most closed popup.
    // (The left mouse button path calls FocusWindow on the hovered window, which will lead NewFrame->ClosePopupsOverWindow to trigger)
    if (g.IO.MouseClicked[1])
    {
        // Find the top-most window between HoveredWindow and the top-most Modal Window.
        // This is where we can trim the popup stack.
        ImGuiWindow* modal = GetTopMostPopupModal();
        bool hovered_window_above_modal = false;
        if (modal == NULL)
            hovered_window_above_modal = true;
        for (int i = g.Windows.Size - 1; i >= 0 && hovered_window_above_modal == false; i--)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window == modal)
                break;
            if (window == g.HoveredWindow)
                hovered_window_above_modal = true;
        }
        ClosePopupsOverWindow(hovered_window_above_modal ? g.HoveredWindow : modal, true);
    }
}

static void TranslateWindow(ImGuiWindow* window, const ImVec2& delta)
{
    window->Pos += delta;
    window->ClipRect.Translate(delta);
    window->OuterRectClipped.Translate(delta);
    window->InnerRect.Translate(delta);
    window->DC.CursorPos += delta;
    window->DC.CursorStartPos += delta;
    window->DC.CursorMaxPos += delta;
    window->DC.LastItemRect.Translate(delta);
    window->DC.LastItemDisplayRect.Translate(delta);
}

static void ScaleWindow(ImGuiWindow* window, float scale)
{
    ImVec2 origin = window->Viewport->Pos;
    window->Pos = ImFloor((window->Pos - origin) * scale + origin);
    window->Size = ImFloor(window->Size * scale);
    window->SizeFull = ImFloor(window->SizeFull * scale);
    window->ContentSize = ImFloor(window->ContentSize * scale);
}

static bool IsWindowActiveAndVisible(ImGuiWindow* window)
{
    return (window->Active) && (!window->Hidden);
}

static void ImGui::UpdateMouseInputs()
{
    ImGuiContext& g = *GImGui;

    // Round mouse position to avoid spreading non-rounded position (e.g. UpdateManualResize doesn't support them well)
    if (IsMousePosValid(&g.IO.MousePos))
        g.IO.MousePos = g.LastValidMousePos = ImFloor(g.IO.MousePos);

    // If mouse just appeared or disappeared (usually denoted by -FLT_MAX components) we cancel out movement in MouseDelta
    if (IsMousePosValid(&g.IO.MousePos) && IsMousePosValid(&g.IO.MousePosPrev))
        g.IO.MouseDelta = g.IO.MousePos - g.IO.MousePosPrev;
    else
        g.IO.MouseDelta = ImVec2(0.0f, 0.0f);
    if (g.IO.MouseDelta.x != 0.0f || g.IO.MouseDelta.y != 0.0f)
        g.NavDisableMouseHover = false;

    g.IO.MousePosPrev = g.IO.MousePos;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        g.IO.MouseClicked[i] = g.IO.MouseDown[i] && g.IO.MouseDownDuration[i] < 0.0f;
        g.IO.MouseReleased[i] = !g.IO.MouseDown[i] && g.IO.MouseDownDuration[i] >= 0.0f;
        g.IO.MouseDownDurationPrev[i] = g.IO.MouseDownDuration[i];
        g.IO.MouseDownDuration[i] = g.IO.MouseDown[i] ? (g.IO.MouseDownDuration[i] < 0.0f ? 0.0f : g.IO.MouseDownDuration[i] + g.IO.DeltaTime) : -1.0f;
        g.IO.MouseDoubleClicked[i] = false;
        if (g.IO.MouseClicked[i])
        {
            if ((float)(g.Time - g.IO.MouseClickedTime[i]) < g.IO.MouseDoubleClickTime)
            {
                ImVec2 delta_from_click_pos = IsMousePosValid(&g.IO.MousePos) ? (g.IO.MousePos - g.IO.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
                if (ImLengthSqr(delta_from_click_pos) < g.IO.MouseDoubleClickMaxDist * g.IO.MouseDoubleClickMaxDist)
                    g.IO.MouseDoubleClicked[i] = true;
                g.IO.MouseClickedTime[i] = -FLT_MAX;    // so the third click isn't turned into a double-click
            }
            else
            {
                g.IO.MouseClickedTime[i] = g.Time;
            }
            g.IO.MouseClickedPos[i] = g.IO.MousePos;
            g.IO.MouseDownWasDoubleClick[i] = g.IO.MouseDoubleClicked[i];
            g.IO.MouseDragMaxDistanceAbs[i] = ImVec2(0.0f, 0.0f);
            g.IO.MouseDragMaxDistanceSqr[i] = 0.0f;
        }
        else if (g.IO.MouseDown[i])
        {
            // Maintain the maximum distance we reaching from the initial click position, which is used with dragging threshold
            ImVec2 delta_from_click_pos = IsMousePosValid(&g.IO.MousePos) ? (g.IO.MousePos - g.IO.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
            g.IO.MouseDragMaxDistanceSqr[i] = ImMax(g.IO.MouseDragMaxDistanceSqr[i], ImLengthSqr(delta_from_click_pos));
            g.IO.MouseDragMaxDistanceAbs[i].x = ImMax(g.IO.MouseDragMaxDistanceAbs[i].x, delta_from_click_pos.x < 0.0f ? -delta_from_click_pos.x : delta_from_click_pos.x);
            g.IO.MouseDragMaxDistanceAbs[i].y = ImMax(g.IO.MouseDragMaxDistanceAbs[i].y, delta_from_click_pos.y < 0.0f ? -delta_from_click_pos.y : delta_from_click_pos.y);
        }
        if (!g.IO.MouseDown[i] && !g.IO.MouseReleased[i])
            g.IO.MouseDownWasDoubleClick[i] = false;
        if (g.IO.MouseClicked[i]) // Clicking any mouse button reactivate mouse hovering which may have been deactivated by gamepad/keyboard navigation
            g.NavDisableMouseHover = false;
    }
}

static void StartLockWheelingWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.WheelingWindow == window)
        return;
    g.WheelingWindow = window;
    g.WheelingWindowRefMousePos = g.IO.MousePos;
    g.WheelingWindowTimer = WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER;
}

void ImGui::UpdateMouseWheel()
{
    ImGuiContext& g = *GImGui;

    // Reset the locked window if we move the mouse or after the timer elapses
    if (g.WheelingWindow != NULL)
    {
        g.WheelingWindowTimer -= g.IO.DeltaTime;
        if (IsMousePosValid() && ImLengthSqr(g.IO.MousePos - g.WheelingWindowRefMousePos) > g.IO.MouseDragThreshold * g.IO.MouseDragThreshold)
            g.WheelingWindowTimer = 0.0f;
        if (g.WheelingWindowTimer <= 0.0f)
        {
            g.WheelingWindow = NULL;
            g.WheelingWindowTimer = 0.0f;
        }
    }

    if (g.IO.MouseWheel == 0.0f && g.IO.MouseWheelH == 0.0f)
        return;

    ImGuiWindow* window = g.WheelingWindow ? g.WheelingWindow : g.HoveredWindow;
    if (!window || window->Collapsed)
        return;

    // Zoom / Scale window
    // FIXME-OBSOLETE: This is an old feature, it still works but pretty much nobody is using it and may be best redesigned.
    if (g.IO.MouseWheel != 0.0f && g.IO.KeyCtrl && g.IO.FontAllowUserScaling)
    {
        StartLockWheelingWindow(window);
        const float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
        const float scale = new_font_scale / window->FontWindowScale;
        window->FontWindowScale = new_font_scale;
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow))
        {
            const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
            SetWindowPos(window, window->Pos + offset, 0);
            window->Size = ImFloor(window->Size * scale);
            window->SizeFull = ImFloor(window->SizeFull * scale);
        }
        return;
    }

    // Mouse wheel scrolling
    // If a child window has the ImGuiWindowFlags_NoScrollWithMouse flag, we give a chance to scroll its parent

    // Vertical Mouse Wheel scrolling
    const float wheel_y = (g.IO.MouseWheel != 0.0f && !g.IO.KeyShift) ? g.IO.MouseWheel : 0.0f;
    if (wheel_y != 0.0f && !g.IO.KeyCtrl)
    {
        StartLockWheelingWindow(window);
        while ((window->Flags & ImGuiWindowFlags_ChildWindow) && ((window->ScrollMax.y == 0.0f) || ((window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))))
            window = window->ParentWindow;
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            float max_step = window->InnerRect.GetHeight() * 0.67f;
            float scroll_step = ImFloor(ImMin(5 * window->CalcFontSize(), max_step));
            SetScrollY(window, window->Scroll.y - wheel_y * scroll_step);
        }
    }

    // Horizontal Mouse Wheel scrolling, or Vertical Mouse Wheel w/ Shift held
    const float wheel_x = (g.IO.MouseWheelH != 0.0f && !g.IO.KeyShift) ? g.IO.MouseWheelH : (g.IO.MouseWheel != 0.0f && g.IO.KeyShift) ? g.IO.MouseWheel : 0.0f;
    if (wheel_x != 0.0f && !g.IO.KeyCtrl)
    {
        StartLockWheelingWindow(window);
        while ((window->Flags & ImGuiWindowFlags_ChildWindow) && ((window->ScrollMax.x == 0.0f) || ((window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))))
            window = window->ParentWindow;
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            float max_step = window->InnerRect.GetWidth() * 0.67f;
            float scroll_step = ImFloor(ImMin(2 * window->CalcFontSize(), max_step));
            SetScrollX(window, window->Scroll.x - wheel_x * scroll_step);
        }
    }
}

// The reason this is exposed in imgui_internal.h is: on touch-based system that don't have hovering, we want to dispatch inputs to the right target (imgui vs imgui+app)
void ImGui::UpdateHoveredWindowAndCaptureFlags()
{
    ImGuiContext& g = *GImGui;

    // Find the window hovered by mouse:
    // - Child windows can extend beyond the limit of their parent so we need to derive HoveredRootWindow from HoveredWindow.
    // - When moving a window we can skip the search, which also conveniently bypasses the fact that window->WindowRectClipped is lagging as this point of the frame.
    // - We also support the moved window toggling the NoInputs flag after moving has started in order to be able to detect windows below it, which is useful for e.g. docking mechanisms.
    FindHoveredWindow();
    IM_ASSERT(g.HoveredWindow == NULL || g.HoveredWindow == g.MovingWindow || g.HoveredWindow->Viewport == g.MouseViewport);

    // Modal windows prevents cursor from hovering behind them.
    ImGuiWindow* modal_window = GetTopMostPopupModal();
    if (modal_window)
        if (g.HoveredRootWindow && !IsWindowChildOf(g.HoveredRootWindow, modal_window))
            g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // Disabled mouse?
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NoMouse)
        g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // We track click ownership. When clicked outside of a window the click is owned by the application and won't report hovering nor request capture even while dragging over our windows afterward.
    int mouse_earliest_button_down = -1;
    bool mouse_any_down = false;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        if (g.IO.MouseClicked[i])
            g.IO.MouseDownOwned[i] = (g.HoveredWindow != NULL) || (!g.OpenPopupStack.empty());
        mouse_any_down |= g.IO.MouseDown[i];
        if (g.IO.MouseDown[i])
            if (mouse_earliest_button_down == -1 || g.IO.MouseClickedTime[i] < g.IO.MouseClickedTime[mouse_earliest_button_down])
                mouse_earliest_button_down = i;
    }
    const bool mouse_avail_to_imgui = (mouse_earliest_button_down == -1) || g.IO.MouseDownOwned[mouse_earliest_button_down];

    // If mouse was first clicked outside of ImGui bounds we also cancel out hovering.
    // FIXME: For patterns of drag and drop across OS windows, we may need to rework/remove this test (first committed 311c0ca9 on 2015/02)
    const bool mouse_dragging_extern_payload = g.DragDropActive && (g.DragDropSourceFlags & ImGuiDragDropFlags_SourceExtern) != 0;
    if (!mouse_avail_to_imgui && !mouse_dragging_extern_payload)
        g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // Update io.WantCaptureMouse for the user application (true = dispatch mouse info to imgui, false = dispatch mouse info to Dear ImGui + app)
    if (g.WantCaptureMouseNextFrame != -1)
        g.IO.WantCaptureMouse = (g.WantCaptureMouseNextFrame != 0);
    else
        g.IO.WantCaptureMouse = (mouse_avail_to_imgui && (g.HoveredWindow != NULL || mouse_any_down)) || (!g.OpenPopupStack.empty());

    // Update io.WantCaptureKeyboard for the user application (true = dispatch keyboard info to imgui, false = dispatch keyboard info to Dear ImGui + app)
    if (g.WantCaptureKeyboardNextFrame != -1)
        g.IO.WantCaptureKeyboard = (g.WantCaptureKeyboardNextFrame != 0);
    else
        g.IO.WantCaptureKeyboard = (g.ActiveId != 0) || (modal_window != NULL);
    if (g.IO.NavActive && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard))
        g.IO.WantCaptureKeyboard = true;

    // Update io.WantTextInput flag, this is to allow systems without a keyboard (e.g. mobile, hand-held) to show a software keyboard if possible
    g.IO.WantTextInput = (g.WantTextInputNextFrame != -1) ? (g.WantTextInputNextFrame != 0) : false;
}

static void NewFrameSanityChecks()
{
    ImGuiContext& g = *GImGui;

    // Check user data
    // (We pass an error message in the assert expression to make it visible to programmers who are not using a debugger, as most assert handlers display their argument)
    IM_ASSERT(g.Initialized);
    IM_ASSERT((g.IO.DeltaTime > 0.0f || g.FrameCount == 0)              && "Need a positive DeltaTime!");
    IM_ASSERT((g.FrameCount == 0 || g.FrameCountEnded == g.FrameCount)  && "Forgot to call Render() or EndFrame() at the end of the previous frame?");
    IM_ASSERT(g.IO.DisplaySize.x >= 0.0f && g.IO.DisplaySize.y >= 0.0f  && "Invalid DisplaySize value!");
    IM_ASSERT(g.IO.Fonts->Fonts.Size > 0                                && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8() ?");
    IM_ASSERT(g.IO.Fonts->Fonts[0]->IsLoaded()                          && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8() ?");
    IM_ASSERT(g.Style.CurveTessellationTol > 0.0f                       && "Invalid style setting!");
    IM_ASSERT(g.Style.Alpha >= 0.0f && g.Style.Alpha <= 1.0f            && "Invalid style setting. Alpha cannot be negative (allows us to avoid a few clamps in color computations)!");
    IM_ASSERT(g.Style.WindowMinSize.x >= 1.0f && g.Style.WindowMinSize.y >= 1.0f && "Invalid style setting.");
    IM_ASSERT(g.Style.WindowMenuButtonPosition == ImGuiDir_Left || g.Style.WindowMenuButtonPosition == ImGuiDir_Right);
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        IM_ASSERT(g.IO.KeyMap[n] >= -1 && g.IO.KeyMap[n] < IM_ARRAYSIZE(g.IO.KeysDown) && "io.KeyMap[] contains an out of bound value (need to be 0..512, or -1 for unmapped key)");

    // Perform simple check: required key mapping (we intentionally do NOT check all keys to not pressure user into setting up everything, but Space is required and was only recently added in 1.60 WIP)
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard)
        IM_ASSERT(g.IO.KeyMap[ImGuiKey_Space] != -1 && "ImGuiKey_Space is not mapped, required for keyboard navigation.");

    // Perform simple check: the beta io.ConfigWindowsResizeFromEdges option requires back-end to honor mouse cursor changes and set the ImGuiBackendFlags_HasMouseCursors flag accordingly.
    if (g.IO.ConfigWindowsResizeFromEdges && !(g.IO.BackendFlags & ImGuiBackendFlags_HasMouseCursors))
        g.IO.ConfigWindowsResizeFromEdges = false;
}

