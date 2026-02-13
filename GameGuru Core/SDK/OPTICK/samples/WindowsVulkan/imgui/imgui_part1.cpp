//-----------------------------------------------------------------------------
// [SECTION] ImGuiListClipper
// This is currently not as flexible/powerful as it should be, needs some rework (see TODO)
//-----------------------------------------------------------------------------

static void SetCursorPosYAndSetupDummyPrevLine(float pos_y, float line_height)
{
    // Set cursor position and a few other things so that SetScrollHere() and Columns() can work when seeking cursor.
    // FIXME: It is problematic that we have to do that here, because custom/equivalent end-user code would stumble on the same issue.
    // The clipper should probably have a 4th step to display the last item in a regular manner.
    ImGui::SetCursorPosY(pos_y);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y - line_height;      // Setting those fields so that SetScrollHere() can properly function after the end of our clipper usage.
    window->DC.PrevLineSize.y = (line_height - GImGui->Style.ItemSpacing.y);    // If we end up needing more accurate data (to e.g. use SameLine) we may as well make the clipper have a fourth step to let user process and display the last item in their list.
    if (window->DC.ColumnsSet)
        window->DC.ColumnsSet->LineMinY = window->DC.CursorPos.y;           // Setting this so that cell Y position are set properly
}

// Use case A: Begin() called from constructor with items_height<0, then called again from Sync() in StepNo 1
// Use case B: Begin() called from constructor with items_height>0
// FIXME-LEGACY: Ideally we should remove the Begin/End functions but they are part of the legacy API we still support. This is why some of the code in Step() calling Begin() and reassign some fields, spaghetti style.
void ImGuiListClipper::Begin(int count, float items_height)
{
    StartPosY = ImGui::GetCursorPosY();
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
    if (ItemsCount == 0 || ImGui::GetCurrentWindowRead()->SkipItems)
    {
        ItemsCount = -1;
        return false;
    }
    if (StepNo == 0) // Step 0: the clipper let you process the first element, regardless of it being visible or not, so we can measure the element height.
    {
        DisplayStart = 0;
        DisplayEnd = 1;
        StartPosY = ImGui::GetCursorPosY();
        StepNo = 1;
        return true;
    }
    if (StepNo == 1) // Step 1: the clipper infer height from first element, calculate the actual range of elements to display, and position the cursor before the first element.
    {
        if (ItemsCount == 1) { ItemsCount = -1; return false; }
        float items_height = ImGui::GetCursorPosY() - StartPosY;
        IM_ASSERT(items_height > 0.0f);   // If this triggers, it means Item 0 hasn't moved the cursor vertically
        Begin(ItemsCount-1, items_height);
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
void ImGui::RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Hide anything after a '##' string
    const char* text_display_end = FindRenderedTextEnd(text, text_end);
    const int text_len = (int)(text_display_end - text);
    if (text_len == 0)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

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
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, &fine_clip_rect);
    }
    else
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, NULL);
    }
    if (g.LogEnabled)
        LogRenderedText(&pos, text, text_display_end);
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
void ImGui::RenderArrow(ImVec2 p_min, ImGuiDir dir, float scale)
{
    ImGuiContext& g = *GImGui;

    const float h = g.FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = p_min + ImVec2(h * 0.50f, h * 0.50f * scale);

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

    g.CurrentWindow->DrawList->AddTriangleFilled(center + a, center + b, center + c, GetColorU32(ImGuiCol_Text));
}

void ImGui::RenderBullet(ImVec2 pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddCircleFilled(pos, g.FontSize*0.20f, GetColorU32(ImGuiCol_Text), 8);
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
    ImGuiWindow* window = ImGui::GetCurrentWindow();
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
    ID = ImHash(name, 0);
    IDStack.push_back(ID);
    Flags = 0;
    Pos = ImVec2(0.0f, 0.0f);
    Size = SizeFull = ImVec2(0.0f, 0.0f);
    SizeContents = SizeContentsExplicit = ImVec2(0.0f, 0.0f);
    WindowPadding = ImVec2(0.0f, 0.0f);
    WindowRounding = 0.0f;
    WindowBorderSize = 0.0f;
    MoveId = GetID("#MOVE");
    ChildId = 0;
    Scroll = ImVec2(0.0f, 0.0f);
    ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
    ScrollTargetCenterRatio = ImVec2(0.5f, 0.5f);
    ScrollbarSizes = ImVec2(0.0f, 0.0f);
    ScrollbarX = ScrollbarY = false;
    Active = WasActive = false;
    WriteAccessed = false;
    Collapsed = false;
    WantCollapseToggle = false;
    SkipItems = false;
    Appearing = false;
    Hidden = false;
    HasCloseButton = false;
    BeginCount = 0;
    BeginOrderWithinParent = -1;
    BeginOrderWithinContext = -1;
    PopupId = 0;
    AutoFitFramesX = AutoFitFramesY = -1;
    AutoFitOnlyGrows = false;
    AutoFitChildAxises = 0x00;
    AutoPosLastDirection = ImGuiDir_None;
    HiddenFramesRegular = HiddenFramesForResize = 0;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;
    SetWindowPosVal = SetWindowPosPivot = ImVec2(FLT_MAX, FLT_MAX);

    LastFrameActive = -1;
    ItemWidthDefault = 0.0f;
    FontWindowScale = 1.0f;
    SettingsIdx = -1;

    DrawList = &DrawListInst;
    DrawList->_OwnerName = Name;
    ParentWindow = NULL;
    RootWindow = NULL;
    RootWindowForTitleBarHighlight = NULL;
    RootWindowForNav = NULL;

    NavLastIds[0] = NavLastIds[1] = 0;
    NavRectRel[0] = NavRectRel[1] = ImRect();
    NavLastChildNavWindow = NULL;

    FocusIdxAllCounter = FocusIdxTabCounter = -1;
    FocusIdxAllRequestCurrent = FocusIdxTabRequestCurrent = INT_MAX;
    FocusIdxAllRequestNext = FocusIdxTabRequestNext = INT_MAX;
}

ImGuiWindow::~ImGuiWindow()
{
    IM_ASSERT(DrawList == &DrawListInst);
    IM_DELETE(Name);
    for (int i = 0; i != ColumnsStorage.Size; i++)
        ColumnsStorage[i].~ImGuiColumnsSet();
}

ImGuiID ImGuiWindow::GetID(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHash(str, str_end ? (int)(str_end - str) : 0, seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHash(&ptr, sizeof(void*), seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    return ImHash(str, str_end ? (int)(str_end - str) : 0, seed);
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    return ImHash(&ptr, sizeof(void*), seed);
}

// This is only used in rare/specific situations to manufacture an ID out of nowhere.
ImGuiID ImGuiWindow::GetIDFromRectangle(const ImRect& r_abs)
{
    ImGuiID seed = IDStack.back();
    const int r_rel[4] = { (int)(r_abs.Min.x - Pos.x), (int)(r_abs.Min.y - Pos.y), (int)(r_abs.Max.x - Pos.x), (int)(r_abs.Max.y - Pos.y) };
    ImGuiID id = ImHash(&r_rel, sizeof(r_rel), seed);
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
        g.ActiveIdHasBeenEdited = false;
        if (id != 0)
        {
            g.LastActiveId = id;
            g.LastActiveIdTimer = 0.0f;
        }
    }
    g.ActiveId = id;
    g.ActiveIdAllowNavDirFlags = 0;
    g.ActiveIdAllowOverlap = false;
    g.ActiveIdWindow = window;
    if (id)
    {
        g.ActiveIdIsAlive = id;
        g.ActiveIdSource = (g.NavActivateId == id || g.NavInputId == id || g.NavJustTabbedId == id || g.NavJustMovedToId == id) ? ImGuiInputSource_Nav : ImGuiInputSource_Mouse;
    }
}

void ImGui::SetFocusID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    // Assume that SetFocusID() is called in the context where its NavLayer is the current layer, which is the case everywhere we call it.
    const int nav_layer = window->DC.NavLayerCurrent;
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
        g.HoveredIdTimer = 0.0f;
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
    (void)id; // Avoid unused variable warnings when asserts are compiled out.
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == id || g.ActiveId == 0 || g.DragDropActive);
    //IM_ASSERT(g.CurrentWindow->DC.LastItemId == id);
    g.ActiveIdHasBeenEdited = true;
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
    const float line_height = ImMax(window->DC.CurrentLineSize.y, size.y);
    const float text_base_offset = ImMax(window->DC.CurrentLineTextBaseOffset, text_offset_y);
    //if (g.IO.KeyAlt) window->DrawList->AddRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size.x, line_height), IM_COL32(255,0,0,200)); // [DEBUG]
    window->DC.CursorPosPrevLine = ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y);
    window->DC.CursorPos = ImVec2((float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x), (float)(int)(window->DC.CursorPos.y + line_height + g.Style.ItemSpacing.y));
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPosPrevLine.x);
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y - g.Style.ItemSpacing.y);
    //if (g.IO.KeyAlt) window->DrawList->AddCircle(window->DC.CursorMaxPos, 3.0f, IM_COL32(255,0,0,255), 4); // [DEBUG]

    window->DC.PrevLineSize.y = line_height;
    window->DC.PrevLineTextBaseOffset = text_base_offset;
    window->DC.CurrentLineSize.y = window->DC.CurrentLineTextBaseOffset = 0.0f;

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
        //  (b) So that we can scroll up/down past clipped items. This adds a small O(N) cost to regular navigation requests unfortunately, but it is still limited to one window.
        //      it may not scale very well for windows with ten of thousands of item, but at least NavMoveRequest is only set on user interaction, aka maximum once a frame.
        //      We could early out with "if (is_clipped && !g.NavInitRequest) return false;" but when we wouldn't be able to reach unclipped widgets. This would work if user had explicit scrolling control (e.g. mapped on a stick)
        window->DC.NavLayerActiveMaskNext |= window->DC.NavLayerCurrentMask;
        if (g.NavId == id || g.NavAnyRequest)
            if (g.NavWindow->RootWindowForNav == window->RootWindowForNav)
                if (window == g.NavWindow || ((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened))
                    NavProcessItem(window, nav_bb_arg ? *nav_bb_arg : bb, id);
    }

    window->DC.LastItemId = id;
    window->DC.LastItemRect = bb;
    window->DC.LastItemStatusFlags = 0;

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

    // Test if interactions on this window are blocked by an active popup or modal
    if (!IsWindowContentHoverable(window, flags))
        return false;

    // Test if the item is disabled
    if ((window->DC.ItemFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
        return false;

    // Special handling for the 1st item after Begin() which represent the title bar. When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect tht case.
    if (window->DC.LastItemId == window->MoveId && window->WriteAccessed)
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

bool ImGui::FocusableItemRegister(ImGuiWindow* window, ImGuiID id, bool tab_stop)
{
    ImGuiContext& g = *GImGui;

    const bool allow_keyboard_focus = (window->DC.ItemFlags & (ImGuiItemFlags_AllowKeyboardFocus | ImGuiItemFlags_Disabled)) == ImGuiItemFlags_AllowKeyboardFocus;
    window->FocusIdxAllCounter++;
    if (allow_keyboard_focus)
        window->FocusIdxTabCounter++;

    // Process keyboard input at this point: TAB/Shift-TAB to tab out of the currently focused item.
    // Note that we can always TAB out of a widget that doesn't allow tabbing in.
    if (tab_stop && (g.ActiveId == id) && window->FocusIdxAllRequestNext == INT_MAX && window->FocusIdxTabRequestNext == INT_MAX && !g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab))
        window->FocusIdxTabRequestNext = window->FocusIdxTabCounter + (g.IO.KeyShift ? (allow_keyboard_focus ? -1 : 0) : +1); // Modulo on index will be applied at the end of frame once we've got the total counter of items.

    if (window->FocusIdxAllCounter == window->FocusIdxAllRequestCurrent)
        return true;
    if (allow_keyboard_focus && window->FocusIdxTabCounter == window->FocusIdxTabRequestCurrent)
    {
        g.NavJustTabbedId = id;
        return true;
    }

    return false;
}

void ImGui::FocusableItemUnregister(ImGuiWindow* window)
{
    window->FocusIdxAllCounter--;
    window->FocusIdxTabCounter--;
}

ImVec2 ImGui::CalcItemSize(ImVec2 size, float default_x, float default_y)
{
    ImGuiContext& g = *GImGui;
    ImVec2 content_max;
    if (size.x < 0.0f || size.y < 0.0f)
        content_max = g.CurrentWindow->Pos + GetContentRegionMax();
    if (size.x <= 0.0f)
        size.x = (size.x == 0.0f) ? default_x : ImMax(content_max.x - g.CurrentWindow->DC.CursorPos.x, 4.0f) + size.x;
    if (size.y <= 0.0f)
        size.y = (size.y == 0.0f) ? default_y : ImMax(content_max.y - g.CurrentWindow->DC.CursorPos.y, 4.0f) + size.y;
    return size;
}

float ImGui::CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiWindow* window = GetCurrentWindowRead();
    if (wrap_pos_x == 0.0f)
        wrap_pos_x = GetContentRegionMax().x + window->Pos.x;
    else if (wrap_pos_x > 0.0f)
        wrap_pos_x += window->Pos.x - window->Scroll.x; // wrap_pos_x is provided is window local space

    return ImMax(wrap_pos_x - pos.x, 1.0f);
}

void* ImGui::MemAlloc(size_t size)
{
    if (ImGuiContext* ctx = GImGui)
        ctx->IO.MetricsActiveAllocations++;
    return GImAllocatorAllocFunc(size, GImAllocatorUserData);
}

void ImGui::MemFree(void* ptr)
{
    if (ptr) 
        if (ImGuiContext* ctx = GImGui)
            ctx->IO.MetricsActiveAllocations--;
    return GImAllocatorFreeFunc(ptr, GImAllocatorUserData);
}

const char* ImGui::GetClipboardText()
{
    return GImGui->IO.GetClipboardTextFn ? GImGui->IO.GetClipboardTextFn(GImGui->IO.ClipboardUserData) : "";
}

void ImGui::SetClipboardText(const char* text)
{
    if (GImGui->IO.SetClipboardTextFn)
        GImGui->IO.SetClipboardTextFn(GImGui->IO.ClipboardUserData, text);
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

// Internal state access - if you want to share ImGui state between modules (e.g. DLL) or allocate it yourself
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

// Helper function to verify that the type sizes are matching between the calling file's compilation unit and imgui.cpp's compilation unit
// If the user has inconsistent compilation settings, imgui configuration #define, packing pragma, etc. you may see different structures from what imgui.cpp sees which is highly problematic.
bool ImGui::DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert)
{
    bool error = false;
    if (strcmp(version, IMGUI_VERSION)!=0) { error = true; IM_ASSERT(strcmp(version,IMGUI_VERSION)==0 && "Mismatched version string!");  }
    if (sz_io    != sizeof(ImGuiIO))       { error = true; IM_ASSERT(sz_io    == sizeof(ImGuiIO)      && "Mismatched struct layout!"); }
    if (sz_style != sizeof(ImGuiStyle))    { error = true; IM_ASSERT(sz_style == sizeof(ImGuiStyle)   && "Mismatched struct layout!"); }
    if (sz_vec2  != sizeof(ImVec2))        { error = true; IM_ASSERT(sz_vec2  == sizeof(ImVec2)       && "Mismatched struct layout!"); }
    if (sz_vec4  != sizeof(ImVec4))        { error = true; IM_ASSERT(sz_vec4  == sizeof(ImVec4)       && "Mismatched struct layout!"); }
    if (sz_vert  != sizeof(ImDrawVert))    { error = true; IM_ASSERT(sz_vert  == sizeof(ImDrawVert)   && "Mismatched struct layout!"); }
    return !error;
}

void ImGui::SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void(*free_func)(void* ptr, void* user_data), void* user_data)
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
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->IO;
}

ImGuiStyle& ImGui::GetStyle()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->Style;
}

// Same value as passed to the old io.RenderDrawListsFn function. Valid after Render() and until the next call to NewFrame()
ImDrawData* ImGui::GetDrawData()
{
    ImGuiContext& g = *GImGui;
    return g.DrawData.Valid ? &g.DrawData : NULL;
}

double ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

ImDrawList* ImGui::GetOverlayDrawList()
{
    return &GImGui->OverlayDrawList;
}

ImDrawListSharedData* ImGui::GetDrawListSharedData()
{
    return &GImGui->DrawListSharedData;
}

void ImGui::StartMouseMovingWindow(ImGuiWindow* window)
{
    // Set ActiveId even if the _NoMove flag is set. Without it, dragging away from a window with _NoMove would activate hover on other windows.
    ImGuiContext& g = *GImGui;
    FocusWindow(window);
    SetActiveID(window->MoveId, window);
    g.NavDisableHighlight = true;
    g.ActiveIdClickOffset = g.IO.MousePos - window->RootWindow->Pos;
    if (!(window->Flags & ImGuiWindowFlags_NoMove) && !(window->RootWindow->Flags & ImGuiWindowFlags_NoMove))
        g.MovingWindow = window;
}

// Handle mouse moving window
// Note: moving window with the navigation keys (Square + d-pad / CTRL+TAB + Arrows) are processed in NavUpdateWindowing()
void ImGui::UpdateMouseMovingWindow()
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
            }
            FocusWindow(g.MovingWindow);
        }
        else
        {
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

static bool IsWindowActiveAndVisible(ImGuiWindow* window)
{
    return (window->Active) && (!window->Hidden);
}

static void ImGui::UpdateMouseInputs()
{
    ImGuiContext& g = *GImGui;

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
        if (g.IO.MouseClicked[i]) // Clicking any mouse button reactivate mouse hovering which may have been deactivated by gamepad/keyboard navigation
            g.NavDisableMouseHover = false;
    }
}

void ImGui::UpdateMouseWheel()
{
    ImGuiContext& g = *GImGui;
    if (!g.HoveredWindow || g.HoveredWindow->Collapsed)
        return;
    if (g.IO.MouseWheel == 0.0f && g.IO.MouseWheelH == 0.0f)
        return;

    // If a child window has the ImGuiWindowFlags_NoScrollWithMouse flag, we give a chance to scroll its parent (unless either ImGuiWindowFlags_NoInputs or ImGuiWindowFlags_NoScrollbar are also set).
    ImGuiWindow* window = g.HoveredWindow;
    ImGuiWindow* scroll_window = window;
    while ((scroll_window->Flags & ImGuiWindowFlags_ChildWindow) && (scroll_window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(scroll_window->Flags & ImGuiWindowFlags_NoScrollbar) && !(scroll_window->Flags & ImGuiWindowFlags_NoInputs) && scroll_window->ParentWindow)
        scroll_window = scroll_window->ParentWindow;
    const bool scroll_allowed = !(scroll_window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(scroll_window->Flags & ImGuiWindowFlags_NoInputs);

    if (g.IO.MouseWheel != 0.0f)
    {
        if (g.IO.KeyCtrl && g.IO.FontAllowUserScaling)
        {
            // Zoom / Scale window
            const float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
            const float scale = new_font_scale / window->FontWindowScale;
            window->FontWindowScale = new_font_scale;

            const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
            window->Pos += offset;
            window->Size *= scale;
            window->SizeFull *= scale;
        }
        else if (!g.IO.KeyCtrl && scroll_allowed)
        {
            // Mouse wheel vertical scrolling
            float scroll_amount = 5 * scroll_window->CalcFontSize();
            scroll_amount = (float)(int)ImMin(scroll_amount, (scroll_window->ContentsRegionRect.GetHeight() + scroll_window->WindowPadding.y * 2.0f) * 0.67f);
            SetWindowScrollY(scroll_window, scroll_window->Scroll.y - g.IO.MouseWheel * scroll_amount);
        }
    }
    if (g.IO.MouseWheelH != 0.0f && scroll_allowed && !g.IO.KeyCtrl)
    {
        // Mouse wheel horizontal scrolling (for hardware that supports it)
        float scroll_amount = scroll_window->CalcFontSize();
        SetWindowScrollX(scroll_window, scroll_window->Scroll.x - g.IO.MouseWheelH * scroll_amount);
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

    // Modal windows prevents cursor from hovering behind them.
    ImGuiWindow* modal_window = GetFrontMostPopupModal();
    if (modal_window)
        if (g.HoveredRootWindow && !IsWindowChildOf(g.HoveredRootWindow, modal_window))
            g.HoveredRootWindow = g.HoveredWindow = NULL;

    // Disabled mouse?
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NoMouse)
        g.HoveredWindow = g.HoveredRootWindow = NULL;

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
        g.HoveredWindow = g.HoveredRootWindow = NULL;

    // Update io.WantCaptureMouse for the user application (true = dispatch mouse info to imgui, false = dispatch mouse info to imgui + app)
    if (g.WantCaptureMouseNextFrame != -1)
        g.IO.WantCaptureMouse = (g.WantCaptureMouseNextFrame != 0);
    else
        g.IO.WantCaptureMouse = (mouse_avail_to_imgui && (g.HoveredWindow != NULL || mouse_any_down)) || (!g.OpenPopupStack.empty());

    // Update io.WantCaptureKeyboard for the user application (true = dispatch keyboard info to imgui, false = dispatch keyboard info to imgui + app)
    if (g.WantCaptureKeyboardNextFrame != -1)
        g.IO.WantCaptureKeyboard = (g.WantCaptureKeyboardNextFrame != 0);
    else
        g.IO.WantCaptureKeyboard = (g.ActiveId != 0) || (modal_window != NULL);
    if (g.IO.NavActive && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard))
        g.IO.WantCaptureKeyboard = true;

    // Update io.WantTextInput flag, this is to allow systems without a keyboard (e.g. mobile, hand-held) to show a software keyboard if possible
    g.IO.WantTextInput = (g.WantTextInputNextFrame != -1) ? (g.WantTextInputNextFrame != 0) : false;
}

void ImGui::NewFrame()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    ImGuiContext& g = *GImGui;

    // Check user data
    // (We pass an error message in the assert expression to make it visible to programmers who are not using a debugger, as most assert handlers display their argument)
    IM_ASSERT(g.Initialized);
    IM_ASSERT(g.IO.DeltaTime >= 0.0f                                    && "Need a positive DeltaTime (zero is tolerated but will cause some timing issues)");
    IM_ASSERT(g.IO.DisplaySize.x >= 0.0f && g.IO.DisplaySize.y >= 0.0f  && "Invalid DisplaySize value");
    IM_ASSERT(g.IO.Fonts->Fonts.Size > 0                                && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8() ?");
    IM_ASSERT(g.IO.Fonts->Fonts[0]->IsLoaded()                          && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8() ?");
    IM_ASSERT(g.Style.CurveTessellationTol > 0.0f                       && "Invalid style setting");
    IM_ASSERT(g.Style.Alpha >= 0.0f && g.Style.Alpha <= 1.0f            && "Invalid style setting. Alpha cannot be negative (allows us to avoid a few clamps in color computations)");
    IM_ASSERT((g.FrameCount == 0 || g.FrameCountEnded == g.FrameCount)  && "Forgot to call Render() or EndFrame() at the end of the previous frame?");
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        IM_ASSERT(g.IO.KeyMap[n] >= -1 && g.IO.KeyMap[n] < IM_ARRAYSIZE(g.IO.KeysDown) && "io.KeyMap[] contains an out of bound value (need to be 0..512, or -1 for unmapped key)");

    // Perform simple check: required key mapping (we intentionally do NOT check all keys to not pressure user into setting up everything, but Space is required and was only recently added in 1.60 WIP)
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard)
        IM_ASSERT(g.IO.KeyMap[ImGuiKey_Space] != -1 && "ImGuiKey_Space is not mapped, required for keyboard navigation.");

    // Perform simple check: the beta io.ConfigResizeWindowsFromEdges option requires back-end to honor mouse cursor changes and set the ImGuiBackendFlags_HasMouseCursors flag accordingly.
    if (g.IO.ConfigResizeWindowsFromEdges && !(g.IO.BackendFlags & ImGuiBackendFlags_HasMouseCursors))
        g.IO.ConfigResizeWindowsFromEdges = false;

    // Load settings on first frame (if not explicitly loaded manually before)
    if (!g.SettingsLoaded)
    {
        IM_ASSERT(g.SettingsWindows.empty());
        if (g.IO.IniFilename)
            LoadIniSettingsFromDisk(g.IO.IniFilename);
        g.SettingsLoaded = true;
    }

    // Save settings (with a delay after the last modification, so we don't spam disk too much)
    if (g.SettingsDirtyTimer > 0.0f)
    {
        g.SettingsDirtyTimer -= g.IO.DeltaTime;
        if (g.SettingsDirtyTimer <= 0.0f)
        {
            if (g.IO.IniFilename != NULL)
                SaveIniSettingsToDisk(g.IO.IniFilename);
            else
                g.IO.WantSaveIniSettings = true;  // Let user know they can call SaveIniSettingsToMemory(). user will need to clear io.WantSaveIniSettings themselves.
            g.SettingsDirtyTimer = 0.0f;
        }
    }

    g.Time += g.IO.DeltaTime;
    g.FrameScopeActive = true;
    g.FrameCount += 1;
    g.TooltipOverrideCount = 0;
    g.WindowsActiveCount = 0;

    // Setup current font and draw list
    g.IO.Fonts->Locked = true;
    SetCurrentFont(GetDefaultFont());
    IM_ASSERT(g.Font->IsLoaded());
    g.DrawListSharedData.ClipRectFullscreen = ImVec4(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y);
    g.DrawListSharedData.CurveTessellationTol = g.Style.CurveTessellationTol;

    g.OverlayDrawList.Clear();
    g.OverlayDrawList.PushTextureID(g.IO.Fonts->TexID);
    g.OverlayDrawList.PushClipRectFullScreen();
    g.OverlayDrawList.Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);

    // Mark rendering data as invalid to prevent user who may have a handle on it to use it
    g.DrawData.Clear();

    // Drag and drop keep the source ID alive so even if the source disappear our state is consistent
    if (g.DragDropActive && g.DragDropPayload.SourceId == g.ActiveId)
        KeepAliveID(g.DragDropPayload.SourceId);

    // Clear reference to active widget if the widget isn't alive anymore
    if (!g.HoveredIdPreviousFrame)
        g.HoveredIdTimer = 0.0f;
    if (g.HoveredId)
        g.HoveredIdTimer += g.IO.DeltaTime;
    g.HoveredIdPreviousFrame = g.HoveredId;
    g.HoveredId = 0;
    g.HoveredIdAllowOverlap = false;
    if (g.ActiveIdIsAlive != g.ActiveId && g.ActiveIdPreviousFrame == g.ActiveId && g.ActiveId != 0)
        ClearActiveID();
    if (g.ActiveId)
        g.ActiveIdTimer += g.IO.DeltaTime;
    g.LastActiveIdTimer += g.IO.DeltaTime;
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdPreviousFrameWindow = g.ActiveIdWindow;
    g.ActiveIdPreviousFrameHasBeenEdited = g.ActiveIdHasBeenEdited;
    g.ActiveIdIsAlive = 0;
    g.ActiveIdPreviousFrameIsAlive = false;
    g.ActiveIdIsJustActivated = false;
    if (g.ScalarAsInputTextId && g.ActiveId != g.ScalarAsInputTextId)
        g.ScalarAsInputTextId = 0;

    // Drag and drop
    g.DragDropAcceptIdPrev = g.DragDropAcceptIdCurr;
    g.DragDropAcceptIdCurr = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropWithinSourceOrTarget = false;

    // Update keyboard input state
    memcpy(g.IO.KeysDownDurationPrev, g.IO.KeysDownDuration, sizeof(g.IO.KeysDownDuration));
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.KeysDown); i++)
        g.IO.KeysDownDuration[i] = g.IO.KeysDown[i] ? (g.IO.KeysDownDuration[i] < 0.0f ? 0.0f : g.IO.KeysDownDuration[i] + g.IO.DeltaTime) : -1.0f;

    // Update gamepad/keyboard directional navigation
    NavUpdate();

    // Update mouse input state
    UpdateMouseInputs();

    // Calculate frame-rate for the user, as a purely luxurious feature
    g.FramerateSecPerFrameAccum += g.IO.DeltaTime - g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx];
    g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx] = g.IO.DeltaTime;
    g.FramerateSecPerFrameIdx = (g.FramerateSecPerFrameIdx + 1) % IM_ARRAYSIZE(g.FramerateSecPerFrame);
    g.IO.Framerate = (g.FramerateSecPerFrameAccum > 0.0f) ? (1.0f / (g.FramerateSecPerFrameAccum / (float)IM_ARRAYSIZE(g.FramerateSecPerFrame))) : FLT_MAX;

    // Handle user moving window with mouse (at the beginning of the frame to avoid input lag or sheering)
    UpdateMouseMovingWindow();
    UpdateHoveredWindowAndCaptureFlags();

    // Background darkening/whitening
    if (GetFrontMostPopupModal() != NULL || (g.NavWindowingTarget != NULL && g.NavWindowingHighlightAlpha > 0.0f))
        g.DimBgRatio = ImMin(g.DimBgRatio + g.IO.DeltaTime * 6.0f, 1.0f);
    else
        g.DimBgRatio = ImMax(g.DimBgRatio - g.IO.DeltaTime * 10.0f, 0.0f);

    g.MouseCursor = ImGuiMouseCursor_Arrow;
    g.WantCaptureMouseNextFrame = g.WantCaptureKeyboardNextFrame = g.WantTextInputNextFrame = -1;
    g.PlatformImePos = ImVec2(1.0f, 1.0f); // OS Input Method Editor showing on top-left of our window by default

    // Mouse wheel scrolling, scale
    UpdateMouseWheel();

    // Pressing TAB activate widget focus
    if (g.ActiveId == 0 && g.NavWindow != NULL && g.NavWindow->Active && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab, false))
    {
        if (g.NavId != 0 && g.NavIdTabCounter != INT_MAX)
            g.NavWindow->FocusIdxTabRequestNext = g.NavIdTabCounter + 1 + (g.IO.KeyShift ? -1 : 1);
        else
            g.NavWindow->FocusIdxTabRequestNext = g.IO.KeyShift ? -1 : 0;
    }
    g.NavIdTabCounter = INT_MAX;

    // Mark all windows as not visible
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        window->WasActive = window->Active;
        window->Active = false;
        window->WriteAccessed = false;
    }

    // Closing the focused window restore focus to the first active root window in descending z-order
    if (g.NavWindow && !g.NavWindow->WasActive)
        FocusFrontMostActiveWindowIgnoringOne(NULL);

    // No window should be open at the beginning of the frame.
    // But in order to allow the user to call NewFrame() multiple times without calling Render(), we are doing an explicit clear.
    g.CurrentWindowStack.resize(0);
    g.CurrentPopupStack.resize(0);
    ClosePopupsOverWindow(g.NavWindow);

    // Create implicit window - we will only render it if the user has added something to it.
    // We don't use "Debug" to avoid colliding with user trying to create a "Debug" window with custom flags.
    SetNextWindowSize(ImVec2(400,400), ImGuiCond_FirstUseEver);
    Begin("Debug##Default");
}

void ImGui::Initialize(ImGuiContext* context)
{
    ImGuiContext& g = *context;
    IM_ASSERT(!g.Initialized && !g.SettingsLoaded);

    // Add .ini handle for ImGuiWindow type
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Window";
    ini_handler.TypeHash = ImHash("Window", 0, 0);
    ini_handler.ReadOpenFn = SettingsHandlerWindow_ReadOpen;
    ini_handler.ReadLineFn = SettingsHandlerWindow_ReadLine;
    ini_handler.WriteAllFn = SettingsHandlerWindow_WriteAll;
    g.SettingsHandlers.push_front(ini_handler);

    g.Initialized = true;
}

// This function is merely here to free heap allocations.
void ImGui::Shutdown(ImGuiContext* context)
{
    // The fonts atlas can be used prior to calling NewFrame(), so we clear it even if g.Initialized is FALSE (which would happen if we never called NewFrame)
    ImGuiContext& g = *context;
    if (g.IO.Fonts && g.FontAtlasOwnedByContext)
        IM_DELETE(g.IO.Fonts);
    g.IO.Fonts = NULL;

    // Cleanup of other data are conditional on actually having initialized ImGui.
    if (!g.Initialized)
        return;

    // Save settings (unless we haven't attempted to load them: CreateContext/DestroyContext without a call to NewFrame shouldn't save an empty file)
    if (g.SettingsLoaded && g.IO.IniFilename != NULL)
        SaveIniSettingsToDisk(g.IO.IniFilename);

    // Clear everything else
    for (int i = 0; i < g.Windows.Size; i++)
        IM_DELETE(g.Windows[i]);
    g.Windows.clear();
    g.WindowsSortBuffer.clear();
    g.CurrentWindow = NULL;
    g.CurrentWindowStack.clear();
    g.WindowsById.Clear();
    g.NavWindow = NULL;
    g.HoveredWindow = NULL;
    g.HoveredRootWindow = NULL;
    g.ActiveIdWindow = g.ActiveIdPreviousFrameWindow = NULL;
    g.MovingWindow = NULL;
    g.ColorModifiers.clear();
    g.StyleModifiers.clear();
    g.FontStack.clear();
    g.OpenPopupStack.clear();
    g.CurrentPopupStack.clear();
    g.DrawDataBuilder.ClearFreeMemory();
    g.OverlayDrawList.ClearFreeMemory();
    g.PrivateClipboard.clear();
    g.InputTextState.TextW.clear();
    g.InputTextState.InitialText.clear();
    g.InputTextState.TempBuffer.clear();

    for (int i = 0; i < g.SettingsWindows.Size; i++)
        IM_DELETE(g.SettingsWindows[i].Name);
    g.SettingsWindows.clear();
    g.SettingsHandlers.clear();

    if (g.LogFile && g.LogFile != stdout)
    {
        fclose(g.LogFile);
        g.LogFile = NULL;
    }
    g.LogClipboard.clear();

    g.Initialized = false;
}

// FIXME: Add a more explicit sort order in the window structure.
static int IMGUI_CDECL ChildWindowComparer(const void* lhs, const void* rhs)
{
    const ImGuiWindow* const a = *(const ImGuiWindow* const *)lhs;
    const ImGuiWindow* const b = *(const ImGuiWindow* const *)rhs;
    if (int d = (a->Flags & ImGuiWindowFlags_Popup) - (b->Flags & ImGuiWindowFlags_Popup))
        return d;
    if (int d = (a->Flags & ImGuiWindowFlags_Tooltip) - (b->Flags & ImGuiWindowFlags_Tooltip))
        return d;
    return (a->BeginOrderWithinParent - b->BeginOrderWithinParent);
}

static void AddWindowToSortedBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window)
{
    out_sorted_windows->push_back(window);
    if (window->Active)
    {
        int count = window->DC.ChildWindows.Size;
        if (count > 1)
            ImQsort(window->DC.ChildWindows.begin(), (size_t)count, sizeof(ImGuiWindow*), ChildWindowComparer);
        for (int i = 0; i < count; i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Active)
                AddWindowToSortedBuffer(out_sorted_windows, child);
        }
    }
}

static void AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    if (draw_list->CmdBuffer.empty())
        return;

    // Remove trailing command if unused
    ImDrawCmd& last_cmd = draw_list->CmdBuffer.back();
    if (last_cmd.ElemCount == 0 && last_cmd.UserCallback == NULL)
    {
        draw_list->CmdBuffer.pop_back();
        if (draw_list->CmdBuffer.empty())
            return;
    }

    // Draw list sanity check. Detect mismatch between PrimReserve() calls and incrementing _VtxCurrentIdx, _VtxWritePtr etc. May trigger for you if you are using PrimXXX functions incorrectly.
    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    // Check that draw_list doesn't use more vertices than indexable (default ImDrawIdx = unsigned short = 2 bytes = 64K vertices per ImDrawList = per window)
    // If this assert triggers because you are drawing lots of stuff manually:
    // A) Make sure you are coarse clipping, because ImDrawList let all your vertices pass. You can use the Metrics window to inspect draw list contents.
    // B) If you need/want meshes with more than 64K vertices, uncomment the '#define ImDrawIdx unsigned int' line in imconfig.h to set the index size to 4 bytes.
    //    You'll need to handle the 4-bytes indices to your renderer. For example, the OpenGL example code detect index size at compile-time by doing:
    //      glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
    //    Your own engine or render API may use different parameters or function calls to specify index sizes. 2 and 4 bytes indices are generally supported by most API.
    // C) If for some reason you cannot use 4 bytes indices or don't want to, a workaround is to call BeginChild()/EndChild() before reaching the 64K limit to split your draw commands in multiple draw lists.
    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    out_list->push_back(draw_list);
}

static void AddWindowToDrawData(ImVector<ImDrawList*>* out_render_list, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.IO.MetricsRenderWindows++;
    AddDrawListToDrawData(out_render_list, window->DrawList);
    for (int i = 0; i < window->DC.ChildWindows.Size; i++)
    {
        ImGuiWindow* child = window->DC.ChildWindows[i];
        if (IsWindowActiveAndVisible(child)) // clipped children may have been marked not active
            AddWindowToDrawData(out_render_list, child);
    }
}

static void AddWindowToDrawDataSelectLayer(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (window->Flags & ImGuiWindowFlags_Tooltip)
        AddWindowToDrawData(&g.DrawDataBuilder.Layers[1], window);
    else
        AddWindowToDrawData(&g.DrawDataBuilder.Layers[0], window);
}

void ImDrawDataBuilder::FlattenIntoSingleLayer()
{
    int n = Layers[0].Size;
    int size = n;
    for (int i = 1; i < IM_ARRAYSIZE(Layers); i++)
        size += Layers[i].Size;
    Layers[0].resize(size);
    for (int layer_n = 1; layer_n < IM_ARRAYSIZE(Layers); layer_n++)
    {
        ImVector<ImDrawList*>& layer = Layers[layer_n];
        if (layer.empty())
            continue;
        memcpy(&Layers[0][n], &layer[0], layer.Size * sizeof(ImDrawList*));
        n += layer.Size;
        layer.resize(0);
    }
}

static void SetupDrawData(ImVector<ImDrawList*>* draw_lists, ImDrawData* draw_data)
{
    ImGuiIO& io = ImGui::GetIO();
    draw_data->Valid = true;
    draw_data->CmdLists = (draw_lists->Size > 0) ? draw_lists->Data : NULL;
    draw_data->CmdListsCount = draw_lists->Size;
    draw_data->TotalVtxCount = draw_data->TotalIdxCount = 0;
    draw_data->DisplayPos = ImVec2(0.0f, 0.0f);
    draw_data->DisplaySize = io.DisplaySize;
    for (int n = 0; n < draw_lists->Size; n++)
    {
        draw_data->TotalVtxCount += draw_lists->Data[n]->VtxBuffer.Size;
        draw_data->TotalIdxCount += draw_lists->Data[n]->IdxBuffer.Size;
    }
}

// When using this function it is sane to ensure that float are perfectly rounded to integer values, to that e.g. (int)(max.x-min.x) in user's render produce correct result.
void ImGui::PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

void ImGui::PopClipRect()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PopClipRect();
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

// This is normally called by Render(). You may want to call it directly if you want to avoid calling Render() but the gain will be very minimal.
void ImGui::EndFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);
    if (g.FrameCountEnded == g.FrameCount)          // Don't process EndFrame() multiple times.
        return;
    IM_ASSERT(g.FrameScopeActive && "Forgot to call ImGui::NewFrame()");

    // Notify OS when our Input Method Editor cursor has moved (e.g. CJK inputs using Microsoft IME)
    if (g.IO.ImeSetInputScreenPosFn && ImLengthSqr(g.PlatformImeLastPos - g.PlatformImePos) > 0.0001f)
    {
        g.IO.ImeSetInputScreenPosFn((int)g.PlatformImePos.x, (int)g.PlatformImePos.y);
        g.PlatformImeLastPos = g.PlatformImePos;
    }

    // Hide implicit "Debug" window if it hasn't been used
    IM_ASSERT(g.CurrentWindowStack.Size == 1);    // Mismatched Begin()/End() calls, did you forget to call end on g.CurrentWindow->Name?
    if (g.CurrentWindow && !g.CurrentWindow->WriteAccessed)
        g.CurrentWindow->Active = false;
    End();

    // Show CTRL+TAB list
    if (g.NavWindowingTarget)
        NavUpdateWindowingList();

    // Drag and Drop: Elapse payload (if delivered, or if source stops being submitted)
    if (g.DragDropActive)
    {
        bool is_delivered = g.DragDropPayload.Delivery;
        bool is_elapsed = (g.DragDropPayload.DataFrameCount + 1 < g.FrameCount) && ((g.DragDropSourceFlags & ImGuiDragDropFlags_SourceAutoExpirePayload) || !IsMouseDown(g.DragDropMouseButton));
        if (is_delivered || is_elapsed)
            ClearDragDrop();
    }

    // Drag and Drop: Fallback for source tooltip. This is not ideal but better than nothing.
    if (g.DragDropActive && g.DragDropSourceFrameCount < g.FrameCount)
    {
        g.DragDropWithinSourceOrTarget = true;
        SetTooltip("...");
        g.DragDropWithinSourceOrTarget = false;
    }

    // Initiate moving window
    if (g.ActiveId == 0 && g.HoveredId == 0)
    {
        if (!g.NavWindow || !g.NavWindow->Appearing) // Unless we just made a window/popup appear
        {
            // Click to focus window and start moving (after we're done with all our widgets)
            if (g.IO.MouseClicked[0])
            {
                if (g.HoveredRootWindow != NULL)
                    StartMouseMovingWindow(g.HoveredWindow);
                else if (g.NavWindow != NULL && GetFrontMostPopupModal() == NULL)
                    FocusWindow(NULL);  // Clicking on void disable focus
            }

            // With right mouse button we close popups without changing focus
            // (The left mouse button path calls FocusWindow which will lead NewFrame->ClosePopupsOverWindow to trigger)
            if (g.IO.MouseClicked[1])
            {
                // Find the top-most window between HoveredWindow and the front most Modal Window.
                // This is where we can trim the popup stack.
                ImGuiWindow* modal = GetFrontMostPopupModal();
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
                ClosePopupsOverWindow(hovered_window_above_modal ? g.HoveredWindow : modal);
            }
        }
    }

    // Sort the window list so that all child windows are after their parent
    // We cannot do that on FocusWindow() because childs may not exist yet
    g.WindowsSortBuffer.resize(0);
    g.WindowsSortBuffer.reserve(g.Windows.Size);
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && (window->Flags & ImGuiWindowFlags_ChildWindow))       // if a child is active its parent will add it
            continue;
        AddWindowToSortedBuffer(&g.WindowsSortBuffer, window);
    }

    IM_ASSERT(g.Windows.Size == g.WindowsSortBuffer.Size);  // we done something wrong
    g.Windows.swap(g.WindowsSortBuffer);
    g.IO.MetricsActiveWindows = g.WindowsActiveCount;

    // Unlock font atlas
    g.IO.Fonts->Locked = false;

    // Clear Input data for next frame
    g.IO.MouseWheel = g.IO.MouseWheelH = 0.0f;
    memset(g.IO.InputCharacters, 0, sizeof(g.IO.InputCharacters));
    memset(g.IO.NavInputs, 0, sizeof(g.IO.NavInputs));

    g.FrameScopeActive = false;
    g.FrameCountEnded = g.FrameCount;
}

void ImGui::Render()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    if (g.FrameCountEnded != g.FrameCount)
        ImGui::EndFrame();
    g.FrameCountRendered = g.FrameCount;

    // Gather ImDrawList to render (for each active window)
    g.IO.MetricsRenderVertices = g.IO.MetricsRenderIndices = g.IO.MetricsRenderWindows = 0;
    g.DrawDataBuilder.Clear();
    ImGuiWindow* windows_to_render_front_most[2];
    windows_to_render_front_most[0] = (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) ? g.NavWindowingTarget->RootWindow : NULL;
    windows_to_render_front_most[1] = g.NavWindowingTarget ? g.NavWindowingList : NULL;
    for (int n = 0; n != g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        if (IsWindowActiveAndVisible(window) && (window->Flags & ImGuiWindowFlags_ChildWindow) == 0 && window != windows_to_render_front_most[0] && window != windows_to_render_front_most[1])
            AddWindowToDrawDataSelectLayer(window);
    }
    for (int n = 0; n < IM_ARRAYSIZE(windows_to_render_front_most); n++)
        if (windows_to_render_front_most[n] && IsWindowActiveAndVisible(windows_to_render_front_most[n])) // NavWindowingTarget is always temporarily displayed as the front-most window
            AddWindowToDrawDataSelectLayer(windows_to_render_front_most[n]);
    g.DrawDataBuilder.FlattenIntoSingleLayer();

    // Draw software mouse cursor if requested
    if (g.IO.MouseDrawCursor)
        RenderMouseCursor(&g.OverlayDrawList, g.IO.MousePos, g.Style.MouseCursorScale, g.MouseCursor);

    if (!g.OverlayDrawList.VtxBuffer.empty())
        AddDrawListToDrawData(&g.DrawDataBuilder.Layers[0], &g.OverlayDrawList);

    // Setup ImDrawData structure for end-user
    SetupDrawData(&g.DrawDataBuilder.Layers[0], &g.DrawData);
    g.IO.MetricsRenderVertices = g.DrawData.TotalVtxCount;
    g.IO.MetricsRenderIndices = g.DrawData.TotalIdxCount;

    // Render. If user hasn't set a callback then they may retrieve the draw data via GetDrawData()
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    if (g.DrawData.CmdListsCount > 0 && g.IO.RenderDrawListsFn != NULL)
        g.IO.RenderDrawListsFn(&g.DrawData);
#endif
}

// Calculate text size. Text can be multi-line. Optionally ignore text after a ## marker.
// CalcTextSize("") should return ImVec2(0.0f, GImGui->FontSize)
ImVec2 ImGui::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
    ImGuiContext& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = FindRenderedTextEnd(text, text_end);      // Hide anything after a '##' string
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = g.FontSize;
    if (text == text_display_end)
        return ImVec2(0.0f, font_size);
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

    // Cancel out character spacing for the last character of a line (it is baked into glyph->AdvanceX field)
    const float font_scale = font_size / font->FontSize;
    const float character_spacing_x = 1.0f * font_scale;
    if (text_size.x > 0.0f)
        text_size.x -= character_spacing_x;
    text_size.x = (float)(int)(text_size.x + 0.95f);

    return text_size;
}

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

// Find window given position, search front-to-back
// FIXME: Note that we have an inconsequential lag here: OuterRectClipped is updated in Begin(), so windows moved programatically 
// with SetWindowPos() and not SetNextWindowPos() will have that rectangle lagging by a frame at the time FindHoveredWindow() is 
// called, aka before the next Begin(). Moving window isn't affected.
static void FindHoveredWindow()
{
    ImGuiContext& g = *GImGui;

    ImGuiWindow* hovered_window = NULL;
    if (g.MovingWindow && !(g.MovingWindow->Flags & ImGuiWindowFlags_NoInputs))
        hovered_window = g.MovingWindow;

    for (int i = g.Windows.Size - 1; i >= 0 && hovered_window == NULL; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        if (!window->Active || window->Hidden)
            continue;
        if (window->Flags & ImGuiWindowFlags_NoInputs)
            continue;

        // Using the clipped AABB, a child window will typically be clipped by its parent (not always)
        ImRect bb(window->OuterRectClipped.Min - g.Style.TouchExtraPadding, window->OuterRectClipped.Max + g.Style.TouchExtraPadding);
        if (bb.Contains(g.IO.MousePos))
        {
            if (hovered_window == NULL)
                hovered_window = window;
            if (hovered_window)
                break;
        }
    }

    g.HoveredWindow = hovered_window;
    g.HoveredRootWindow = g.HoveredWindow ? g.HoveredWindow->RootWindow : NULL;

}

// Test if mouse cursor is hovering given rectangle
// NB- Rectangle is clipped by our current clip setting
// NB- Expand the rectangle to be generous on imprecise inputs systems (g.Style.TouchExtraPadding)
bool ImGui::IsMouseHoveringRect(const ImVec2& r_min, const ImVec2& r_max, bool clip)
{
    ImGuiContext& g = *GImGui;

    // Clip
    ImRect rect_clipped(r_min, r_max);
    if (clip)
        rect_clipped.ClipWith(g.CurrentWindow->ClipRect);

    // Expand for touch input
    const ImRect rect_for_touch(rect_clipped.Min - g.Style.TouchExtraPadding, rect_clipped.Max + g.Style.TouchExtraPadding);
    if (!rect_for_touch.Contains(g.IO.MousePos))
        return false;
    return true;
}

int ImGui::GetKeyIndex(ImGuiKey imgui_key)
{
    IM_ASSERT(imgui_key >= 0 && imgui_key < ImGuiKey_COUNT);
    return GImGui->IO.KeyMap[imgui_key];
}

// Note that imgui doesn't know the semantic of each entry of io.KeysDown[]. Use your own indices/enums according to how your back-end/engine stored them into io.KeysDown[]!
bool ImGui::IsKeyDown(int user_key_index)
{
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(GImGui->IO.KeysDown));
    return GImGui->IO.KeysDown[user_key_index];
}

int ImGui::CalcTypematicPressedRepeatAmount(float t, float t_prev, float repeat_delay, float repeat_rate)
{
    if (t == 0.0f)
        return 1;
    if (t <= repeat_delay || repeat_rate <= 0.0f)
        return 0;
    const int count = (int)((t - repeat_delay) / repeat_rate) - (int)((t_prev - repeat_delay) / repeat_rate);
    return (count > 0) ? count : 0;
}

int ImGui::GetKeyPressedAmount(int key_index, float repeat_delay, float repeat_rate)
{
    ImGuiContext& g = *GImGui;
    if (key_index < 0) return false;
    IM_ASSERT(key_index >= 0 && key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownDuration[key_index];
    return CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, repeat_delay, repeat_rate);
}

bool ImGui::IsKeyPressed(int user_key_index, bool repeat)
{
    ImGuiContext& g = *GImGui;
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownDuration[user_key_index];
    if (t == 0.0f)
        return true;
    if (repeat && t > g.IO.KeyRepeatDelay)
        return GetKeyPressedAmount(user_key_index, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate) > 0;
    return false;
}

bool ImGui::IsKeyReleased(int user_key_index)
{
    ImGuiContext& g = *GImGui;
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    return g.IO.KeysDownDurationPrev[user_key_index] >= 0.0f && !g.IO.KeysDown[user_key_index];
}

bool ImGui::IsMouseDown(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDown[button];
}

bool ImGui::IsAnyMouseDown()
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
        if (g.IO.MouseDown[n])
            return true;
    return false;
}

bool ImGui::IsMouseClicked(int button, bool repeat)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    const float t = g.IO.MouseDownDuration[button];
    if (t == 0.0f)
        return true;

    if (repeat && t > g.IO.KeyRepeatDelay)
    {
        float delay = g.IO.KeyRepeatDelay, rate = g.IO.KeyRepeatRate;
        if ((ImFmod(t - delay, rate) > rate*0.5f) != (ImFmod(t - delay - g.IO.DeltaTime, rate) > rate*0.5f))
            return true;
    }

    return false;
}

bool ImGui::IsMouseReleased(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseReleased[button];
}

bool ImGui::IsMouseDoubleClicked(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDoubleClicked[button];
}

bool ImGui::IsMouseDragging(int button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (!g.IO.MouseDown[button])
        return false;
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    return g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold;
}

ImVec2 ImGui::GetMousePos()
{
    return GImGui->IO.MousePos;
}

// NB: prefer to call right after BeginPopup(). At the time Selectable/MenuItem is activated, the popup is already closed!
ImVec2 ImGui::GetMousePosOnOpeningCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    if (g.CurrentPopupStack.Size > 0)
        return g.OpenPopupStack[g.CurrentPopupStack.Size-1].OpenMousePos;
    return g.IO.MousePos;
}

// We typically use ImVec2(-FLT_MAX,-FLT_MAX) to denote an invalid mouse position
bool ImGui::IsMousePosValid(const ImVec2* mouse_pos)
{
    if (mouse_pos == NULL)
        mouse_pos = &GImGui->IO.MousePos;
    const float MOUSE_INVALID = -256000.0f;
    return mouse_pos->x >= MOUSE_INVALID && mouse_pos->y >= MOUSE_INVALID;
}

// NB: This is only valid if IsMousePosValid(). Back-ends in theory should always keep mouse position valid when dragging even outside the client window.
ImVec2 ImGui::GetMouseDragDelta(int button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    if (g.IO.MouseDown[button])
        if (g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold)
            return g.IO.MousePos - g.IO.MouseClickedPos[button];     // Assume we can only get active with left-mouse button (at the moment).
    return ImVec2(0.0f, 0.0f);
}

void ImGui::ResetMouseDragDelta(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    // NB: We don't need to reset g.IO.MouseDragMaxDistanceSqr
    g.IO.MouseClickedPos[button] = g.IO.MousePos;
}

ImGuiMouseCursor ImGui::GetMouseCursor()
{
    return GImGui->MouseCursor;
}

void ImGui::SetMouseCursor(ImGuiMouseCursor cursor_type)
{
    GImGui->MouseCursor = cursor_type;
}

void ImGui::CaptureKeyboardFromApp(bool capture)
{
    GImGui->WantCaptureKeyboardNextFrame = capture ? 1 : 0;
}

void ImGui::CaptureMouseFromApp(bool capture)
{
    GImGui->WantCaptureMouseNextFrame = capture ? 1 : 0;
}

bool ImGui::IsItemActive()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
    {
        ImGuiWindow* window = g.CurrentWindow;
        return g.ActiveId == window->DC.LastItemId;
    }
    return false;
}

bool ImGui::IsItemDeactivated()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    return (g.ActiveIdPreviousFrame == window->DC.LastItemId && g.ActiveIdPreviousFrame != 0 && g.ActiveId != window->DC.LastItemId);
}

bool ImGui::IsItemDeactivatedAfterEdit()
{
    ImGuiContext& g = *GImGui;
    return IsItemDeactivated() && (g.ActiveIdPreviousFrameHasBeenEdited || (g.ActiveId == 0 && g.ActiveIdHasBeenEdited));
}

bool ImGui::IsItemFocused()
{
    ImGuiContext& g = *GImGui;
    return g.NavId && !g.NavDisableHighlight && g.NavId == g.CurrentWindow->DC.LastItemId;
}

bool ImGui::IsItemClicked(int mouse_button)
{
    return IsMouseClicked(mouse_button) && IsItemHovered(ImGuiHoveredFlags_None);
}

bool ImGui::IsAnyItemHovered()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId != 0 || g.HoveredIdPreviousFrame != 0;
}

bool ImGui::IsAnyItemActive()
{
    ImGuiContext& g = *GImGui;
    return g.ActiveId != 0;
}

bool ImGui::IsAnyItemFocused()
{
    ImGuiContext& g = *GImGui;
    return g.NavId != 0 && !g.NavDisableHighlight;
}

bool ImGui::IsItemVisible()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(window->DC.LastItemRect);
}

bool ImGui::IsItemEdited()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_Edited) != 0;
}

// Allow last item to be overlapped by a subsequent item. Both may be activated during the same frame before the later one takes priority.
void ImGui::SetItemAllowOverlap()
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId == g.CurrentWindow->DC.LastItemId)
        g.HoveredIdAllowOverlap = true;
    if (g.ActiveId == g.CurrentWindow->DC.LastItemId)
        g.ActiveIdAllowOverlap = true;
}

