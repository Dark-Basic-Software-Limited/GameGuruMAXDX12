ImVec2 ImGui::GetItemRectMin()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.Min;
}

ImVec2 ImGui::GetItemRectMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.Max;
}

ImVec2 ImGui::GetItemRectSize()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.GetSize();
}

static ImRect GetViewportRect()
{
    ImGuiContext& g = *GImGui;
    if (g.IO.DisplayVisibleMin.x != g.IO.DisplayVisibleMax.x && g.IO.DisplayVisibleMin.y != g.IO.DisplayVisibleMax.y)
        return ImRect(g.IO.DisplayVisibleMin, g.IO.DisplayVisibleMax);
    return ImRect(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y);
}

static bool ImGui::BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;

    flags |= ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_ChildWindow;
    flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

    // Size
    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);
    SetNextWindowSize(size);

    // Name
    char title[256];
    if (name)
        ImFormatString(title, IM_ARRAYSIZE(title), "%s/%s", parent_window->Name, name);
    else
        ImFormatString(title, IM_ARRAYSIZE(title), "%s/%08X", parent_window->Name, id);

    const float backup_border_size = g.Style.ChildBorderSize;
    if (!border)
        g.Style.ChildBorderSize = 0.0f;
    bool ret = Begin(title, NULL, flags);
    g.Style.ChildBorderSize = backup_border_size;

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;
    child_window->AutoFitChildAxises = auto_fit_axises;

    // Process navigation-in immediately so NavInit can run on first frame
    if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(id+1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
        g.ActiveIdSource = ImGuiInputSource_Nav;
    }
    return ret;
}

bool ImGui::BeginChild(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    return BeginChildEx(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

bool ImGui::BeginChild(ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    IM_ASSERT(id != 0);
    return BeginChildEx(NULL, id, size_arg, border, extra_flags);
}

void ImGui::EndChild()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() callss
    if (window->BeginCount > 1)
    {
        End();
    }
    else
    {
        ImVec2 sz = window->Size;
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
            sz.x = ImMax(4.0f, sz.x);
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
            sz.y = ImMax(4.0f, sz.y);
        End();

        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
        ItemSize(sz);
        if ((window->DC.NavLayerActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
        {
            ItemAdd(bb, window->ChildId);
            RenderNavHighlight(bb, window->ChildId);

            // When browsing a window that has no activable items (scroll only) we keep a highlight on the child
            if (window->DC.NavLayerActiveMask == 0 && window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2,2), bb.Max + ImVec2(2,2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
        }
        else
        {
            // Not navigable into
            ItemAdd(bb, 0);
        }
    }
}

// Helper to create a child window / scrolling region that looks like a normal widget frame.
bool ImGui::BeginChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
    PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
    PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
    bool ret = BeginChild(id, size, true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | extra_flags);
    PopStyleVar(3);
    PopStyleColor();
    return ret;
}

void ImGui::EndChildFrame()
{
    EndChild();
}

// Save and compare stack sizes on Begin()/End() to detect usage errors
static void CheckStacksSize(ImGuiWindow* window, bool write)
{
    // NOT checking: DC.ItemWidth, DC.AllowKeyboardFocus, DC.ButtonRepeat, DC.TextWrapPos (per window) to allow user to conveniently push once and not pop (they are cleared on Begin)
    ImGuiContext& g = *GImGui;
    int* p_backup = &window->DC.StackSizesBackup[0];
    { int current = window->IDStack.Size;       if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "PushID/PopID or TreeNode/TreePop Mismatch!");   p_backup++; }    // Too few or too many PopID()/TreePop()
    { int current = window->DC.GroupStack.Size; if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "BeginGroup/EndGroup Mismatch!");                p_backup++; }    // Too few or too many EndGroup()
    { int current = g.CurrentPopupStack.Size;   if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "BeginMenu/EndMenu or BeginPopup/EndPopup Mismatch"); p_backup++;}// Too few or too many EndMenu()/EndPopup()
    // For color, style and font stacks there is an incentive to use Push/Begin/Pop/.../End patterns, so we relax our checks a little to allow them.
    { int current = g.ColorModifiers.Size;      if (write) *p_backup = current; else IM_ASSERT(*p_backup >= current && "PushStyleColor/PopStyleColor Mismatch!");       p_backup++; }    // Too few or too many PopStyleColor()
    { int current = g.StyleModifiers.Size;      if (write) *p_backup = current; else IM_ASSERT(*p_backup >= current && "PushStyleVar/PopStyleVar Mismatch!");           p_backup++; }    // Too few or too many PopStyleVar()
    { int current = g.FontStack.Size;           if (write) *p_backup = current; else IM_ASSERT(*p_backup >= current && "PushFont/PopFont Mismatch!");                   p_backup++; }    // Too few or too many PopFont()
    IM_ASSERT(p_backup == window->DC.StackSizesBackup + IM_ARRAYSIZE(window->DC.StackSizesBackup));
}

static void SetWindowConditionAllowFlags(ImGuiWindow* window, ImGuiCond flags, bool enabled)
{
    window->SetWindowPosAllowFlags       = enabled ? (window->SetWindowPosAllowFlags       | flags) : (window->SetWindowPosAllowFlags       & ~flags);
    window->SetWindowSizeAllowFlags      = enabled ? (window->SetWindowSizeAllowFlags      | flags) : (window->SetWindowSizeAllowFlags      & ~flags);
    window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
}

ImGuiWindow* ImGui::FindWindowByName(const char* name)
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = ImHash(name, 0);
    return (ImGuiWindow*)g.WindowsById.GetVoidPtr(id);
}

static ImGuiWindow* CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;

    // Create window the first time
    ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
    window->Flags = flags;
    g.WindowsById.SetVoidPtr(window->ID, window);

    // Default/arbitrary window position. Use SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
    window->Pos = ImVec2(60, 60);

    // User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
        if (ImGuiWindowSettings* settings = ImGui::FindWindowSettings(window->ID))
        {
            // Retrieve settings from .ini file
            window->SettingsIdx = g.SettingsWindows.index_from_pointer(settings);
            SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
            window->Pos = ImFloor(settings->Pos);
            window->Collapsed = settings->Collapsed;
            if (ImLengthSqr(settings->Size) > 0.00001f)
                size = ImFloor(settings->Size);
        }
    window->Size = window->SizeFull = window->SizeFullAtLastBegin = size;
    window->DC.CursorMaxPos = window->Pos; // So first call to CalcSizeContents() doesn't return crazy values

    if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
    {
        window->AutoFitFramesX = window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
    else
    {
        if (window->Size.x <= 0.0f)
            window->AutoFitFramesX = 2;
        if (window->Size.y <= 0.0f)
            window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
    }

    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.insert(g.Windows.begin(), window); // Quite slow but rare and only once
    else
        g.Windows.push_back(window);
    return window;
}

static ImVec2 CalcSizeAfterConstraint(ImGuiWindow* window, ImVec2 new_size)
{
    ImGuiContext& g = *GImGui;
    if (g.NextWindowData.SizeConstraintCond != 0)
    {
        // Using -1,-1 on either X/Y axis to preserve the current size.
        ImRect cr = g.NextWindowData.SizeConstraintRect;
        new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
        new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
        if (g.NextWindowData.SizeCallback)
        {
            ImGuiSizeCallbackData data;
            data.UserData = g.NextWindowData.SizeCallbackUserData;
            data.Pos = window->Pos;
            data.CurrentSize = window->SizeFull;
            data.DesiredSize = new_size;
            g.NextWindowData.SizeCallback(&data);
            new_size = data.DesiredSize;
        }
    }

    // Minimum size
    if (!(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysAutoResize)))
    {
        new_size = ImMax(new_size, g.Style.WindowMinSize);
        new_size.y = ImMax(new_size.y, window->TitleBarHeight() + window->MenuBarHeight() + ImMax(0.0f, g.Style.WindowRounding - 1.0f)); // Reduce artifacts with very small windows
    }
    return new_size;
}

static ImVec2 CalcSizeContents(ImGuiWindow* window)
{
    ImVec2 sz;
    sz.x = (float)(int)((window->SizeContentsExplicit.x != 0.0f) ? window->SizeContentsExplicit.x : (window->DC.CursorMaxPos.x - window->Pos.x + window->Scroll.x));
    sz.y = (float)(int)((window->SizeContentsExplicit.y != 0.0f) ? window->SizeContentsExplicit.y : (window->DC.CursorMaxPos.y - window->Pos.y + window->Scroll.y));
    return sz + window->WindowPadding;
}

static ImVec2 CalcSizeAutoFit(ImGuiWindow* window, const ImVec2& size_contents)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Tooltip always resize
        return size_contents;
    }
    else
    {
        // When the window cannot fit all contents (either because of constraints, either because screen is too small): we are growing the size on the other axis to compensate for expected scrollbar. FIXME: Might turn bigger than DisplaySize-WindowPadding.
        const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) != 0;
        const bool is_menu = (window->Flags & ImGuiWindowFlags_ChildMenu) != 0;
        ImVec2 size_min = style.WindowMinSize;
        if (is_popup || is_menu) // Popups and menus bypass style.WindowMinSize by default, but we give then a non-zero minimum size to facilitate understanding problematic cases (e.g. empty popups)
            size_min = ImMin(size_min, ImVec2(4.0f, 4.0f));
        ImVec2 size_auto_fit = ImClamp(size_contents, size_min, ImMax(size_min, g.IO.DisplaySize - style.DisplaySafeAreaPadding * 2.0f));
        ImVec2 size_auto_fit_after_constraint = CalcSizeAfterConstraint(window, size_auto_fit);
        if (size_auto_fit_after_constraint.x < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar))
            size_auto_fit.y += style.ScrollbarSize;
        if (size_auto_fit_after_constraint.y < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar))
            size_auto_fit.x += style.ScrollbarSize;
        return size_auto_fit;
    }
}

ImVec2 ImGui::CalcWindowExpectedSize(ImGuiWindow* window)
{
    ImVec2 size_contents = CalcSizeContents(window);
    return CalcSizeAfterConstraint(window, CalcSizeAutoFit(window, size_contents));
}

static float GetScrollMaxX(ImGuiWindow* window)
{
    return ImMax(0.0f, window->SizeContents.x - (window->SizeFull.x - window->ScrollbarSizes.x));
}

static float GetScrollMaxY(ImGuiWindow* window)
{
    return ImMax(0.0f, window->SizeContents.y - (window->SizeFull.y - window->ScrollbarSizes.y));
}

static ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window, bool snap_on_edges)
{
    ImGuiContext& g = *GImGui;
    ImVec2 scroll = window->Scroll;
    if (window->ScrollTarget.x < FLT_MAX)
    {
        float cr_x = window->ScrollTargetCenterRatio.x;
        scroll.x = window->ScrollTarget.x - cr_x * (window->SizeFull.x - window->ScrollbarSizes.x);
    }
    if (window->ScrollTarget.y < FLT_MAX)
    {
        // 'snap_on_edges' allows for a discontinuity at the edge of scrolling limits to take account of WindowPadding so that scrolling to make the last item visible scroll far enough to see the padding.
        float cr_y = window->ScrollTargetCenterRatio.y;
        float target_y = window->ScrollTarget.y;
        if (snap_on_edges && cr_y <= 0.0f && target_y <= window->WindowPadding.y)
            target_y = 0.0f;
        if (snap_on_edges && cr_y >= 1.0f && target_y >= window->SizeContents.y - window->WindowPadding.y + g.Style.ItemSpacing.y)
            target_y = window->SizeContents.y;
        scroll.y = target_y - (1.0f - cr_y) * (window->TitleBarHeight() + window->MenuBarHeight()) - cr_y * (window->SizeFull.y - window->ScrollbarSizes.y);
    }
    scroll = ImMax(scroll, ImVec2(0.0f, 0.0f));
    if (!window->Collapsed && !window->SkipItems)
    {
        scroll.x = ImMin(scroll.x, GetScrollMaxX(window));
        scroll.y = ImMin(scroll.y, GetScrollMaxY(window));
    }
    return scroll;
}

static ImGuiCol GetWindowBgColorIdxFromFlags(ImGuiWindowFlags flags)
{
    if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGuiCol_PopupBg;
    if (flags & ImGuiWindowFlags_ChildWindow)
        return ImGuiCol_ChildBg;
    return ImGuiCol_WindowBg;
}

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size)
{
    ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);                // Expected window upper-left
    ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm); // Expected window lower-right
    ImVec2 size_expected = pos_max - pos_min;
    ImVec2 size_constrained = CalcSizeAfterConstraint(window, size_expected);
    *out_pos = pos_min;
    if (corner_norm.x == 0.0f)
        out_pos->x -= (size_constrained.x - size_expected.x);
    if (corner_norm.y == 0.0f)
        out_pos->y -= (size_constrained.y - size_expected.y);
    *out_size = size_constrained;
}

struct ImGuiResizeGripDef
{
    ImVec2  CornerPos;
    ImVec2  InnerDir;
    int     AngleMin12, AngleMax12;
};

const ImGuiResizeGripDef resize_grip_def[4] =
{
    { ImVec2(1,1), ImVec2(-1,-1), 0, 3 }, // Lower right
    { ImVec2(0,1), ImVec2(+1,-1), 3, 6 }, // Lower left
    { ImVec2(0,0), ImVec2(+1,+1), 6, 9 }, // Upper left
    { ImVec2(1,0), ImVec2(-1,+1), 9,12 }, // Upper right
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness)
{
    ImRect rect = window->Rect();
    if (thickness == 0.0f) rect.Max -= ImVec2(1,1);
    if (border_n == 0) return ImRect(rect.Min.x + perp_padding, rect.Min.y,                rect.Max.x - perp_padding, rect.Min.y + thickness);
    if (border_n == 1) return ImRect(rect.Max.x - thickness,    rect.Min.y + perp_padding, rect.Max.x,                rect.Max.y - perp_padding);
    if (border_n == 2) return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness,    rect.Max.x - perp_padding, rect.Max.y);
    if (border_n == 3) return ImRect(rect.Min.x,                rect.Min.y + perp_padding, rect.Min.x + thickness,    rect.Max.y - perp_padding);
    IM_ASSERT(0);
    return ImRect();
}

// Handle resize for: Resize Grips, Borders, Gamepad
static void ImGui::UpdateManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4])
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;
    if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        return;

    const int resize_border_count = g.IO.ConfigResizeWindowsFromEdges ? 4 : 0;
    const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
    const float grip_hover_size = (float)(int)(grip_draw_size * 0.75f);

    ImVec2 pos_target(FLT_MAX, FLT_MAX);
    ImVec2 size_target(FLT_MAX, FLT_MAX);

    // Manual resize grips
    PushID("#RESIZE");
    for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
    {
        const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
        const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPos);

        // Using the FlattenChilds button flag we make the resize button accessible even if we are hovering over a child window
        ImRect resize_rect(corner, corner + grip.InnerDir * grip_hover_size);
        if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
        if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
        bool hovered, held;
        ButtonBehavior(resize_rect, window->GetID((void*)(intptr_t)resize_grip_n), &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        if (hovered || held)
            g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

        if (held && g.IO.MouseDoubleClicked[0] && resize_grip_n == 0)
        {
            // Manual auto-fit when double-clicking
            size_target = CalcSizeAfterConstraint(window, size_auto_fit);
            ClearActiveID();
        }
        else if (held)
        {
            // Resize from any of the four corners
            // We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
            ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + resize_rect.GetSize() * grip.CornerPos; // Corner of the window corresponding to our corner grip
            CalcResizePosSizeFromAnyCorner(window, corner_target, grip.CornerPos, &pos_target, &size_target);
        }
        if (resize_grip_n == 0 || held || hovered)
            resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
    }
    for (int border_n = 0; border_n < resize_border_count; border_n++)
    {
        const float BORDER_SIZE = 5.0f;          // FIXME: Only works _inside_ window because of HoveredWindow check.
        const float BORDER_APPEAR_TIMER = 0.05f; // Reduce visual noise
        bool hovered, held;
        ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_size, BORDER_SIZE);
        ButtonBehavior(border_rect, window->GetID((void*)(intptr_t)(border_n + 4)), &hovered, &held, ImGuiButtonFlags_FlattenChildren);
        if ((hovered && g.HoveredIdTimer > BORDER_APPEAR_TIMER) || held)
        {
            g.MouseCursor = (border_n & 1) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
            if (held) *border_held = border_n;
        }
        if (held)
        {
            ImVec2 border_target = window->Pos;
            ImVec2 border_posn;
            if (border_n == 0) { border_posn = ImVec2(0, 0); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y); }
            if (border_n == 1) { border_posn = ImVec2(1, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + BORDER_SIZE); }
            if (border_n == 2) { border_posn = ImVec2(0, 1); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + BORDER_SIZE); }
            if (border_n == 3) { border_posn = ImVec2(0, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x); }
            CalcResizePosSizeFromAnyCorner(window, border_target, border_posn, &pos_target, &size_target);
        }
    }
    PopID();

    // Navigation resize (keyboard/gamepad)
    if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
    {
        ImVec2 nav_resize_delta;
        if (g.NavInputSource == ImGuiInputSource_NavKeyboard && g.IO.KeyShift)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_NavGamepad)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_Down);
        if (nav_resize_delta.x != 0.0f || nav_resize_delta.y != 0.0f)
        {
            const float NAV_RESIZE_SPEED = 600.0f;
            nav_resize_delta *= ImFloor(NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));
            g.NavWindowingToggleLayer = false;
            g.NavDisableMouseHover = true;
            resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
            // FIXME-NAV: Should store and accumulate into a separate size buffer to handle sizing constraints properly, right now a constraint will make us stuck.
            size_target = CalcSizeAfterConstraint(window, window->SizeFull + nav_resize_delta);
        }
    }

    // Apply back modified position/size to window
    if (size_target.x != FLT_MAX)
    {
        window->SizeFull = size_target;
        MarkIniSettingsDirty(window);
    }
    if (pos_target.x != FLT_MAX)
    {
        window->Pos = ImFloor(pos_target);
        MarkIniSettingsDirty(window);
    }

    window->Size = window->SizeFull;
}

void ImGui::UpdateWindowParentAndRootLinks(ImGuiWindow* window, ImGuiWindowFlags flags, ImGuiWindow* parent_window)
{
    window->ParentWindow = parent_window;
    window->RootWindow = window->RootWindowForTitleBarHighlight = window->RootWindowForNav = window;
    if (parent_window && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
        window->RootWindow = parent_window->RootWindow;
    if (parent_window && !(flags & ImGuiWindowFlags_Modal) && (flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)))
        window->RootWindowForTitleBarHighlight = parent_window->RootWindowForTitleBarHighlight;
    while (window->RootWindowForNav->Flags & ImGuiWindowFlags_NavFlattened)
        window->RootWindowForNav = window->RootWindowForNav->ParentWindow;
}

// Push a new ImGui window to add widgets to.
// - A default window called "Debug" is automatically stacked at the beginning of every frame so you can use widgets without explicitly calling a Begin/End pair.
// - Begin/End can be called multiple times during the frame with the same window name to append content.
// - The window name is used as a unique identifier to preserve window information across frames (and save rudimentary information to the .ini file).
//   You can use the "##" or "###" markers to use the same label with different id, or same id with different label. See documentation at the top of this file.
// - Return false when window is collapsed, so you can early out in your code. You always need to call ImGui::End() even if false is returned.
// - Passing 'bool* p_open' displays a Close button on the upper-right corner of the window, the pointed value will be set to false when the button is pressed.
bool ImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(name != NULL);                        // Window name required
    IM_ASSERT(g.FrameScopeActive);                  // Forgot to call ImGui::NewFrame()
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

    // Find or create
    ImGuiWindow* window = FindWindowByName(name);
    const bool window_just_created = (window == NULL);
    if (window_just_created)
    {
        ImVec2 size_on_first_use = (g.NextWindowData.SizeCond != 0) ? g.NextWindowData.SizeVal : ImVec2(0.0f, 0.0f); // Any condition flag will do since we are creating a new window here.
        window = CreateNewWindow(name, size_on_first_use, flags);
    }

    // Automatically disable manual moving/resizing when NoInputs is set
    if (flags & ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (flags & ImGuiWindowFlags_NavFlattened)
        IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

    const int current_frame = g.FrameCount;
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
    if (first_begin_of_the_frame)
        window->Flags = (ImGuiWindowFlags)flags;
    else
        flags = window->Flags;

    // Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
    ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
    ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
    IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));
    window->HasCloseButton = (p_open != NULL);

    // Update the Appearing flag
    bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
    const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesForResize > 0);
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.CurrentPopupStack.Size];
        window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
        window_just_activated_by_user |= (window != popup_ref.Window);
    }
    window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

    // Add to stack
    g.CurrentWindowStack.push_back(window);
    SetCurrentWindow(window);
    CheckStacksSize(window, true);
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.CurrentPopupStack.Size];
        popup_ref.Window = window;
        g.CurrentPopupStack.push_back(popup_ref);
        window->PopupId = popup_ref.PopupId;
    }

    if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
        window->NavLastIds[0] = 0;

    // Process SetNextWindow***() calls
    bool window_pos_set_by_api = false;
    bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
    if (g.NextWindowData.PosCond)
    {
        window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
        if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
        {
            // May be processed on the next frame if this is our first frame and we are measuring size
            // FIXME: Look into removing the branch so everything can go through this same code path for consistency.
            window->SetWindowPosVal = g.NextWindowData.PosVal;
            window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
            window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
        }
        else
        {
            SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
        }
    }
    if (g.NextWindowData.SizeCond)
    {
        window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
        window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
        SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
    }
    if (g.NextWindowData.ContentSizeCond)
    {
        // Adjust passed "client size" to become a "window size"
        window->SizeContentsExplicit = g.NextWindowData.ContentSizeVal;
        if (window->SizeContentsExplicit.y != 0.0f)
            window->SizeContentsExplicit.y += window->TitleBarHeight() + window->MenuBarHeight();
    }
    else if (first_begin_of_the_frame)
    {
        window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
    }
    if (g.NextWindowData.CollapsedCond)
        SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
    if (g.NextWindowData.FocusCond)
        FocusWindow(window);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

    // When reusing window again multiple times a frame, just append content (don't need to setup again)
    if (first_begin_of_the_frame)
    {
        // Initialize
        const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
        UpdateWindowParentAndRootLinks(window, flags, parent_window);

        window->Active = true;
        window->BeginOrderWithinParent = 0;
        window->BeginOrderWithinContext = g.WindowsActiveCount++;
        window->BeginCount = 0;
        window->ClipRect = ImVec4(-FLT_MAX,-FLT_MAX,+FLT_MAX,+FLT_MAX);
        window->LastFrameActive = current_frame;
        window->IDStack.resize(1);

        // UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

        // Update contents size from last frame for auto-fitting (or use explicit size)
        window->SizeContents = CalcSizeContents(window);
        if (window->HiddenFramesRegular > 0)
            window->HiddenFramesRegular--;
        if (window->HiddenFramesForResize > 0)
            window->HiddenFramesForResize--;

        // Hide new windows for one frame until they calculate their size
        if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
            window->HiddenFramesForResize = 1;

        // Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
        // We reset Size/SizeContents for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
        if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
        {
            window->HiddenFramesForResize = 1;
            if (flags & ImGuiWindowFlags_AlwaysAutoResize)
            {
                if (!window_size_x_set_by_api)
                    window->Size.x = window->SizeFull.x = 0.f;
                if (!window_size_y_set_by_api)
                    window->Size.y = window->SizeFull.y = 0.f;
                window->SizeContents = ImVec2(0.f, 0.f);
            }
        }

        SetCurrentWindow(window);

        // Lock border size and padding for the frame (so that altering them doesn't cause inconsistencies)
        window->WindowBorderSize = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildBorderSize : ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
        window->WindowPadding = style.WindowPadding;
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
            window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
        window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
        window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

        // Collapse window by double-clicking on title bar
        // At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
        {
            // We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
            ImRect title_bar_rect = window->TitleBarRect();
            if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
                window->WantCollapseToggle = true;
            if (window->WantCollapseToggle)
            {
                window->Collapsed = !window->Collapsed;
                MarkIniSettingsDirty(window);
                FocusWindow(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }
        window->WantCollapseToggle = false;

        // SIZE

        // Calculate auto-fit size, handle automatic resize
        const ImVec2 size_auto_fit = CalcSizeAutoFit(window, window->SizeContents);
        ImVec2 size_full_modified(FLT_MAX, FLT_MAX);
        if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
        {
            // Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
            if (!window_size_x_set_by_api)
                window->SizeFull.x = size_full_modified.x = size_auto_fit.x;
            if (!window_size_y_set_by_api)
                window->SizeFull.y = size_full_modified.y = size_auto_fit.y;
        }
        else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        {
            // Auto-fit may only grow window during the first few frames
            // We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
            if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
                window->SizeFull.x = size_full_modified.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
            if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
                window->SizeFull.y = size_full_modified.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
            if (!window->Collapsed)
                MarkIniSettingsDirty(window);
        }

        // Apply minimum/maximum window size constraints and final size
        window->SizeFull = CalcSizeAfterConstraint(window, window->SizeFull);
        window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

        // SCROLLBAR STATUS

        // Update scrollbar status (based on the Size that was effective during last frame or the auto-resized Size).
        if (!window->Collapsed)
        {
            // When reading the current size we need to read it after size constraints have been applied
            float size_x_for_scrollbars = size_full_modified.x != FLT_MAX ? window->SizeFull.x : window->SizeFullAtLastBegin.x;
            float size_y_for_scrollbars = size_full_modified.y != FLT_MAX ? window->SizeFull.y : window->SizeFullAtLastBegin.y;
            window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
            window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
            if (window->ScrollbarX && !window->ScrollbarY)
                window->ScrollbarY = (window->SizeContents.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
            window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
        }

        // POSITION

        // Popup latch its initial position, will position itself when it appears next frame
        if (window_just_activated_by_user)
        {
            window->AutoPosLastDirection = ImGuiDir_None;
            if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
                window->Pos = g.CurrentPopupStack.back().OpenPopupPos;
        }

        // Position child window
        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            window->BeginOrderWithinParent = parent_window->DC.ChildWindows.Size;
            parent_window->DC.ChildWindows.push_back(window);
            if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
                window->Pos = parent_window->DC.CursorPos;
        }

        const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesForResize == 0);
        if (window_pos_with_pivot)
            SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, window->SetWindowPosVal - window->SizeFull * window->SetWindowPosPivot), 0); // Position given a pivot (e.g. for centering)
        else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
            window->Pos = FindBestWindowPosForPopup(window);

        // Clamp position so it stays visible
        if (!(flags & ImGuiWindowFlags_ChildWindow))
        {
            if (!window_pos_set_by_api && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
            {
                ImVec2 padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
                window->Pos = ImMax(window->Pos + window->Size, padding) - window->Size;
                window->Pos = ImMin(window->Pos, g.IO.DisplaySize - padding);
            }
        }
        window->Pos = ImFloor(window->Pos);

        // Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
        window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

        // Prepare for item focus requests
        window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter+1)) % (window->FocusIdxAllCounter+1);
        window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter+1)) % (window->FocusIdxTabCounter+1);
        window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
        window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

        // Apply scrolling
        window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
        window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

        // Apply window focus (new and reactivated windows are moved to front)
        bool want_focus = false;
        if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
            if (!(flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) || (flags & ImGuiWindowFlags_Popup))
                want_focus = true;

        // Handle manual resize: Resize Grips, Borders, Gamepad
        int border_held = -1;
        ImU32 resize_grip_col[4] = { 0 };
        const int resize_grip_count = g.IO.ConfigResizeWindowsFromEdges ? 2 : 1; // 4
        const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
        if (!window->Collapsed)
            UpdateManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0]);

        // Default item width. Make it proportional to window size if window manually resizes
        if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
            window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
        else
            window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

        // DRAWING

        // Setup draw list and outer clipping rectangle
        window->DrawList->Clear();
        window->DrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
        window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
        ImRect viewport_rect(GetViewportRect());
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
            PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
        else
            PushClipRect(viewport_rect.Min, viewport_rect.Max, true);

        // Draw modal window background (darkens what is behind them, all viewports)
        const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetFrontMostPopupModal() && window->HiddenFramesForResize <= 0;
        const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && (window == g.NavWindowingTargetAnim->RootWindow);
        if (dim_bg_for_modal || dim_bg_for_window_list)
        {
            const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
            window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
        }

        // Draw navigation selection/windowing rectangle background
        if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
        {
            ImRect bb = window->Rect();
            bb.Expand(g.FontSize);
            if (!bb.Contains(viewport_rect)) // Avoid drawing if the window covers all the viewport anyway
                window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
        }

        // Draw window + handle manual resize
        const float window_rounding = window->WindowRounding;
        const float window_border_size = window->WindowBorderSize;
        const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
        const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
        const ImRect title_bar_rect = window->TitleBarRect();
        if (window->Collapsed)
        {
            // Title bar only
            float backup_border_size = style.FrameBorderSize;
            g.Style.FrameBorderSize = window->WindowBorderSize;
            ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
            RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
            g.Style.FrameBorderSize = backup_border_size;
        }
        else
        {
            // Window background
            ImU32 bg_col = GetColorU32(GetWindowBgColorIdxFromFlags(flags));
            if (g.NextWindowData.BgAlphaCond != 0)
            {
                bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(g.NextWindowData.BgAlphaVal) << IM_COL32_A_SHIFT);
                g.NextWindowData.BgAlphaCond = 0;
            }
            window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

            // Title bar
            ImU32 title_bar_col = GetColorU32(window->Collapsed ? ImGuiCol_TitleBgCollapsed : title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            if (!(flags & ImGuiWindowFlags_NoTitleBar))
                window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);

            // Menu bar
            if (flags & ImGuiWindowFlags_MenuBar)
            {
                ImRect menu_bar_rect = window->MenuBarRect();
                menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
                window->DrawList->AddRectFilled(menu_bar_rect.Min, menu_bar_rect.Max, GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
                if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
                    window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
            }

            // Scrollbars
            if (window->ScrollbarX)
                Scrollbar(ImGuiLayoutType_Horizontal);
            if (window->ScrollbarY)
                Scrollbar(ImGuiLayoutType_Vertical);

            // Render resize grips (after their input handling so we don't have a frame of latency)
            if (!(flags & ImGuiWindowFlags_NoResize))
            {
                for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
                {
                    const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
                    const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPos);
                    window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, grip_draw_size) : ImVec2(grip_draw_size, window_border_size)));
                    window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(grip_draw_size, window_border_size) : ImVec2(window_border_size, grip_draw_size)));
                    window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
                    window->DrawList->PathFillConvex(resize_grip_col[resize_grip_n]);
                }
            }

            // Borders
            if (window_border_size > 0.0f)
                window->DrawList->AddRect(window->Pos, window->Pos + window->Size, GetColorU32(ImGuiCol_Border), window_rounding, ImDrawCornerFlags_All, window_border_size);
            if (border_held != -1)
            {
                ImRect border = GetResizeBorderRect(window, border_held, grip_draw_size, 0.0f);
                window->DrawList->AddLine(border.Min, border.Max, GetColorU32(ImGuiCol_SeparatorActive), ImMax(1.0f, window_border_size));
            }
            if (style.FrameBorderSize > 0 && !(flags & ImGuiWindowFlags_NoTitleBar))
                window->DrawList->AddLine(title_bar_rect.GetBL() + ImVec2(style.WindowBorderSize, -1), title_bar_rect.GetBR() + ImVec2(-style.WindowBorderSize, -1), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
        }

        // Draw navigation selection/windowing rectangle border
        if (g.NavWindowingTargetAnim == window)
        {
            float rounding = ImMax(window->WindowRounding, g.Style.WindowRounding);
            ImRect bb = window->Rect();
            bb.Expand(g.FontSize);
            if (bb.Contains(viewport_rect)) // If a window fits the entire viewport, adjust its highlight inward
            {
                bb.Expand(-g.FontSize - 1.0f);
                rounding = window->WindowRounding;
            }
            window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), rounding, ~0, 3.0f);
        }

        // Store a backup of SizeFull which we will use next frame to decide if we need scrollbars.
        window->SizeFullAtLastBegin = window->SizeFull;

        // Update various regions. Variables they depends on are set above in this function.
        // FIXME: window->ContentsRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
        window->ContentsRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
        window->ContentsRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
        window->ContentsRegionRect.Max.x = window->Pos.x - window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
        window->ContentsRegionRect.Max.y = window->Pos.y - window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

        // Setup drawing context
        // (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
        window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
        window->DC.GroupOffset.x = 0.0f;
        window->DC.ColumnsOffset.x = 0.0f;
        window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.CurrentLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
        window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
        window->DC.NavHideHighlightOneFrame = false;
        window->DC.NavHasScroll = (GetScrollMaxY() > 0.0f);
        window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
        window->DC.NavLayerActiveMaskNext = 0x00;
        window->DC.MenuBarAppending = false;
        window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
        window->DC.ChildWindows.resize(0);
        window->DC.LayoutType = ImGuiLayoutType_Vertical;
        window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
        window->DC.ItemFlags = parent_window ? parent_window->DC.ItemFlags : ImGuiItemFlags_Default_;
        window->DC.ItemWidth = window->ItemWidthDefault;
        window->DC.TextWrapPos = -1.0f; // disabled
        window->DC.ItemFlagsStack.resize(0);
        window->DC.ItemWidthStack.resize(0);
        window->DC.TextWrapPosStack.resize(0);
        window->DC.ColumnsSet = NULL;
        window->DC.TreeDepth = 0;
        window->DC.TreeDepthMayJumpToParentOnPop = 0x00;
        window->DC.StateStorage = &window->StateStorage;
        window->DC.GroupStack.resize(0);
        window->MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);

        if ((flags & ImGuiWindowFlags_ChildWindow) && (window->DC.ItemFlags != parent_window->DC.ItemFlags))
        {
            window->DC.ItemFlags = parent_window->DC.ItemFlags;
            window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
        }

        if (window->AutoFitFramesX > 0)
            window->AutoFitFramesX--;
        if (window->AutoFitFramesY > 0)
            window->AutoFitFramesY--;

        // Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
        if (want_focus)
        {
            FocusWindow(window);
            NavInitWindow(window, false);
        }

        // Title bar
        if (!(flags & ImGuiWindowFlags_NoTitleBar))
        {
            // Close & collapse button are on layer 1 (same as menus) and don't default focus
            const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
            window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
            window->DC.NavLayerCurrent++;
            window->DC.NavLayerCurrentMask <<= 1;

            // Collapse button
            if (!(flags & ImGuiWindowFlags_NoCollapse))
                if (CollapseButton(window->GetID("#COLLAPSE"), window->Pos))
                    window->WantCollapseToggle = true; // Defer collapsing to next frame as we are too far in the Begin() function

            // Close button
            if (p_open != NULL)
            {
                const float pad = style.FramePadding.y;
                const float rad = g.FontSize * 0.5f;
                if (CloseButton(window->GetID("#CLOSE"), window->Rect().GetTR() + ImVec2(-pad - rad, pad + rad), rad + 1))
                    *p_open = false;
            }

            window->DC.NavLayerCurrent--;
            window->DC.NavLayerCurrentMask >>= 1;
            window->DC.ItemFlags = item_flags_backup;

            // Title text (FIXME: refactor text alignment facilities along with RenderText helpers, this is too much code for what it does.)
            ImVec2 text_size = CalcTextSize(name, NULL, true);
            ImRect text_r = title_bar_rect;
            float pad_left = (flags & ImGuiWindowFlags_NoCollapse) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
            float pad_right = (p_open == NULL)                     ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
            if (style.WindowTitleAlign.x > 0.0f)
                pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
            text_r.Min.x += pad_left;
            text_r.Max.x -= pad_right;
            ImRect clip_rect = text_r;
            clip_rect.Max.x = window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x); // Match the size of CloseButton()
            RenderTextClipped(text_r.Min, text_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
        }

        // Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
        window->OuterRectClipped = window->Rect();
        window->OuterRectClipped.ClipWith(window->ClipRect);

        // Pressing CTRL+C while holding on a window copy its content to the clipboard
        // This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
        // Maybe we can support CTRL+C on every element?
        /*
        if (g.ActiveId == move_id)
            if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
                ImGui::LogToClipboard();
        */

        // Inner rectangle
        // We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
        // Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
        window->InnerMainRect.Min.x = title_bar_rect.Min.x + window->WindowBorderSize;
        window->InnerMainRect.Min.y = title_bar_rect.Max.y + window->MenuBarHeight() + (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
        window->InnerMainRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x - window->WindowBorderSize;
        window->InnerMainRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y - window->WindowBorderSize;
        //window->DrawList->AddRect(window->InnerRect.Min, window->InnerRect.Max, IM_COL32_WHITE);

        // Inner clipping rectangle
        // Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
        window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerMainRect.Min.x + ImMax(0.0f, ImFloor(window->WindowPadding.x*0.5f - window->WindowBorderSize)));
        window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerMainRect.Min.y);
        window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerMainRect.Max.x - ImMax(0.0f, ImFloor(window->WindowPadding.x*0.5f - window->WindowBorderSize)));
        window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerMainRect.Max.y);

        // After Begin() we fill the last item / hovered data based on title bar data. It is a standard behavior (to allow creation of context menus on title bar only, etc.).
        window->DC.LastItemId = window->MoveId;
        window->DC.LastItemStatusFlags = IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0;
        window->DC.LastItemRect = title_bar_rect;
    }

    PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

    // Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
    if (first_begin_of_the_frame)
        window->WriteAccessed = false;

    window->BeginCount++;
    g.NextWindowData.Clear();

    if (flags & ImGuiWindowFlags_ChildWindow)
    {
        // Child window can be out of sight and have "negative" clip windows.
        // Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
        IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);

        if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
            if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
                window->HiddenFramesRegular = 1;

        // Completely hide along with parent or if parent is collapsed
        if (parent_window && (parent_window->Collapsed || parent_window->Hidden))
            window->HiddenFramesRegular = 1;
    }

    // Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
    if (style.Alpha <= 0.0f)
        window->HiddenFramesRegular = 1;

    // Update the Hidden flag
    window->Hidden = (window->HiddenFramesRegular > 0) || (window->HiddenFramesForResize);

    // Return false if we don't intend to display anything to allow user to perform an early out optimization
    window->SkipItems = (window->Collapsed || !window->Active || window->Hidden) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesForResize <= 0;

    return !window->SkipItems;
}

// Old Begin() API with 5 parameters, avoid calling this version directly! Use SetNextWindowSize()/SetNextWindowBgAlpha() + Begin() instead.
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
bool ImGui::Begin(const char* name, bool* p_open, const ImVec2& size_first_use, float bg_alpha_override, ImGuiWindowFlags flags)
{
    // Old API feature: we could pass the initial window size as a parameter. This was misleading because it only had an effect if the window didn't have data in the .ini file.
    if (size_first_use.x != 0.0f || size_first_use.y != 0.0f)
        ImGui::SetNextWindowSize(size_first_use, ImGuiCond_FirstUseEver);

    // Old API feature: override the window background alpha with a parameter.
    if (bg_alpha_override >= 0.0f)
        ImGui::SetNextWindowBgAlpha(bg_alpha_override);

    return ImGui::Begin(name, p_open, flags);
}
#endif // IMGUI_DISABLE_OBSOLETE_FUNCTIONS

void ImGui::End()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (window->DC.ColumnsSet != NULL)
        EndColumns();
    PopClipRect();   // Inner window clip rectangle

    // Stop logging
    if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
        LogFinish();

    // Pop from window stack
    g.CurrentWindowStack.pop_back();
    if (window->Flags & ImGuiWindowFlags_Popup)
        g.CurrentPopupStack.pop_back();
    CheckStacksSize(window, false);
    SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
}

void ImGui::BringWindowToFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* current_front_window = g.Windows.back();
    if (current_front_window == window || current_front_window->RootWindow == window)
        return;
    for (int i = g.Windows.Size - 2; i >= 0; i--) // We can ignore the front most window
        if (g.Windows[i] == window)
        {
            g.Windows.erase(g.Windows.Data + i);
            g.Windows.push_back(window);
            break;
        }
}

void ImGui::BringWindowToBack(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.Windows[0] == window)
        return;
    for (int i = 0; i < g.Windows.Size; i++)
        if (g.Windows[i] == window)
        {
            memmove(&g.Windows[1], &g.Windows[0], (size_t)i * sizeof(ImGuiWindow*));
            g.Windows[0] = window;
            break;
        }
}

// Moving window to front of display and set focus (which happens to be back of our sorted list)
void ImGui::FocusWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;

    if (g.NavWindow != window)
    {
        g.NavWindow = window;
        if (window && g.NavDisableMouseHover)
            g.NavMousePosDirty = true;
        g.NavInitRequest = false;
        g.NavId = window ? window->NavLastIds[0] : 0; // Restore NavId
        g.NavIdIsAlive = false;
        g.NavLayer = 0;
        //printf("[%05d] FocusWindow(\"%s\")\n", g.FrameCount, window ? window->Name : NULL);
    }

    // Passing NULL allow to disable keyboard focus
    if (!window)
        return;

    // Move the root window to the top of the pile
    if (window->RootWindow)
        window = window->RootWindow;

    // Steal focus on active widgets
    if (window->Flags & ImGuiWindowFlags_Popup) // FIXME: This statement should be unnecessary. Need further testing before removing it..
        if (g.ActiveId != 0 && g.ActiveIdWindow && g.ActiveIdWindow->RootWindow != window)
            ClearActiveID();

    // Bring to front
    if (!(window->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus))
        BringWindowToFront(window);
}

void ImGui::FocusFrontMostActiveWindowIgnoringOne(ImGuiWindow* ignore_window)
{
    ImGuiContext& g = *GImGui;
    for (int i = g.Windows.Size - 1; i >= 0; i--)
        if (g.Windows[i] != ignore_window && g.Windows[i]->WasActive && !(g.Windows[i]->Flags & ImGuiWindowFlags_ChildWindow))
        {
            ImGuiWindow* focus_window = NavRestoreLastChildNavWindow(g.Windows[i]);
            FocusWindow(focus_window);
            return;
        }
}

void ImGui::PushItemWidth(float item_width)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidth = (item_width == 0.0f ? window->ItemWidthDefault : item_width);
    window->DC.ItemWidthStack.push_back(window->DC.ItemWidth);
}

void ImGui::PushMultiItemsWidths(int components, float w_full)
{
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = GImGui->Style;
    if (w_full <= 0.0f)
        w_full = CalcItemWidth();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.ItemInnerSpacing.x) * (components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one + style.ItemInnerSpacing.x) * (components-1)));
    window->DC.ItemWidthStack.push_back(w_item_last);
    for (int i = 0; i < components-1; i++)
        window->DC.ItemWidthStack.push_back(w_item_one);
    window->DC.ItemWidth = window->DC.ItemWidthStack.back();
}

void ImGui::PopItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidthStack.pop_back();
    window->DC.ItemWidth = window->DC.ItemWidthStack.empty() ? window->ItemWidthDefault : window->DC.ItemWidthStack.back();
}

float ImGui::CalcItemWidth()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    float w = window->DC.ItemWidth;
    if (w < 0.0f)
    {
        // Align to a right-side limit. We include 1 frame padding in the calculation because this is how the width is always used (we add 2 frame padding to it), but we could move that responsibility to the widget as well.
        float width_to_right_edge = GetContentRegionAvail().x;
        w = ImMax(1.0f, width_to_right_edge + w);
    }
    w = (float)(int)w;
    return w;
}

void ImGui::SetCurrentFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(font && font->IsLoaded());    // Font Atlas not created. Did you call io.Fonts->GetTexDataAsRGBA32 / GetTexDataAsAlpha8 ?
    IM_ASSERT(font->Scale > 0.0f);
    g.Font = font;
    g.FontBaseSize = g.IO.FontGlobalScale * g.Font->FontSize * g.Font->Scale;
    g.FontSize = g.CurrentWindow ? g.CurrentWindow->CalcFontSize() : 0.0f;

    ImFontAtlas* atlas = g.Font->ContainerAtlas;
    g.DrawListSharedData.TexUvWhitePixel = atlas->TexUvWhitePixel;
    g.DrawListSharedData.Font = g.Font;
    g.DrawListSharedData.FontSize = g.FontSize;
}

void ImGui::PushFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    if (!font)
        font = GetDefaultFont();
    SetCurrentFont(font);
    g.FontStack.push_back(font);
    g.CurrentWindow->DrawList->PushTextureID(font->ContainerAtlas->TexID);
}

void  ImGui::PopFont()
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow->DrawList->PopTextureID();
    g.FontStack.pop_back();
    SetCurrentFont(g.FontStack.empty() ? GetDefaultFont() : g.FontStack.back());
}

void ImGui::PushItemFlag(ImGuiItemFlags option, bool enabled)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (enabled)
        window->DC.ItemFlags |= option;
    else
        window->DC.ItemFlags &= ~option;
    window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
}

void ImGui::PopItemFlag()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemFlagsStack.pop_back();
    window->DC.ItemFlags = window->DC.ItemFlagsStack.empty() ? ImGuiItemFlags_Default_ : window->DC.ItemFlagsStack.back();
}

void ImGui::PushAllowKeyboardFocus(bool allow_keyboard_focus)
{
    PushItemFlag(ImGuiItemFlags_AllowKeyboardFocus, allow_keyboard_focus);
}

void ImGui::PopAllowKeyboardFocus()
{
    PopItemFlag();
}

void ImGui::PushButtonRepeat(bool repeat)
{
    PushItemFlag(ImGuiItemFlags_ButtonRepeat, repeat);
}

void ImGui::PopButtonRepeat()
{
    PopItemFlag();
}

void ImGui::PushTextWrapPos(float wrap_pos_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPos = wrap_pos_x;
    window->DC.TextWrapPosStack.push_back(wrap_pos_x);
}

void ImGui::PopTextWrapPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPosStack.pop_back();
    window->DC.TextWrapPos = window->DC.TextWrapPosStack.empty() ? -1.0f : window->DC.TextWrapPosStack.back();
}

// FIXME: This may incur a round-trip (if the end user got their data from a float4) but eventually we aim to store the in-flight colors as ImU32
void ImGui::PushStyleColor(ImGuiCol idx, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorModifiers.push_back(backup);
    g.Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void ImGui::PushStyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorModifiers.push_back(backup);
    g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        ImGuiColorMod& backup = g.ColorModifiers.back();
        g.Style.Colors[backup.Col] = backup.BackupValue;
        g.ColorModifiers.pop_back();
        count--;
    }
}

struct ImGuiStyleVarInfo
{
    ImGuiDataType   Type;
    ImU32           Count;
    ImU32           Offset;
    void*           GetVarPtr(ImGuiStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImGuiStyleVarInfo GStyleVarInfo[] =
{
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, Alpha) },              // ImGuiStyleVar_Alpha
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding) },      // ImGuiStyleVar_WindowPadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding) },     // ImGuiStyleVar_WindowRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowBorderSize) },   // ImGuiStyleVar_WindowBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize) },      // ImGuiStyleVar_WindowMinSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowTitleAlign) },   // ImGuiStyleVar_WindowTitleAlign
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildRounding) },      // ImGuiStyleVar_ChildRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildBorderSize) },    // ImGuiStyleVar_ChildBorderSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupRounding) },      // ImGuiStyleVar_PopupRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupBorderSize) },    // ImGuiStyleVar_PopupBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding) },       // ImGuiStyleVar_FramePadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding) },      // ImGuiStyleVar_FrameRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameBorderSize) },    // ImGuiStyleVar_FrameBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing) },        // ImGuiStyleVar_ItemSpacing
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing) },   // ImGuiStyleVar_ItemInnerSpacing
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing) },      // ImGuiStyleVar_IndentSpacing
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarSize) },      // ImGuiStyleVar_ScrollbarSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarRounding) },  // ImGuiStyleVar_ScrollbarRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize) },        // ImGuiStyleVar_GrabMinSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabRounding) },       // ImGuiStyleVar_GrabRounding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign) },    // ImGuiStyleVar_ButtonTextAlign
};

static const ImGuiStyleVarInfo* GetStyleVarInfo(ImGuiStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImGuiStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImGuiStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, float val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        ImGuiContext& g = *GImGui;
        float* pvar = (float*)var_info->GetVarPtr(&g.Style);
        g.StyleModifiers.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0); // Called function with wrong-type? Variable is not a float.
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImGuiContext& g = *GImGui;
        ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
        g.StyleModifiers.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0); // Called function with wrong-type? Variable is not a ImVec2.
}

void ImGui::PopStyleVar(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        // We avoid a generic memcpy(data, &backup.Backup.., GDataTypeSize[info->Type] * info->Count), the overhead in Debug is not worth it.
        ImGuiStyleMod& backup = g.StyleModifiers.back();
        const ImGuiStyleVarInfo* info = GetStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&g.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1)      { ((float*)data)[0] = backup.BackupFloat[0]; }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) { ((float*)data)[0] = backup.BackupFloat[0]; ((float*)data)[1] = backup.BackupFloat[1]; }
        g.StyleModifiers.pop_back();
        count--;
    }
}

const char* ImGui::GetStyleColorName(ImGuiCol idx)
{
    // Create switch-case from enum with regexp: ImGuiCol_{.*}, --> case ImGuiCol_\1: return "\1";
    switch (idx)
    {
    case ImGuiCol_Text: return "Text";
    case ImGuiCol_TextDisabled: return "TextDisabled";
    case ImGuiCol_WindowBg: return "WindowBg";
    case ImGuiCol_ChildBg: return "ChildBg";
    case ImGuiCol_PopupBg: return "PopupBg";
    case ImGuiCol_Border: return "Border";
    case ImGuiCol_BorderShadow: return "BorderShadow";
    case ImGuiCol_FrameBg: return "FrameBg";
    case ImGuiCol_FrameBgHovered: return "FrameBgHovered";
    case ImGuiCol_FrameBgActive: return "FrameBgActive";
    case ImGuiCol_TitleBg: return "TitleBg";
    case ImGuiCol_TitleBgActive: return "TitleBgActive";
    case ImGuiCol_TitleBgCollapsed: return "TitleBgCollapsed";
    case ImGuiCol_MenuBarBg: return "MenuBarBg";
    case ImGuiCol_ScrollbarBg: return "ScrollbarBg";
    case ImGuiCol_ScrollbarGrab: return "ScrollbarGrab";
    case ImGuiCol_ScrollbarGrabHovered: return "ScrollbarGrabHovered";
    case ImGuiCol_ScrollbarGrabActive: return "ScrollbarGrabActive";
    case ImGuiCol_CheckMark: return "CheckMark";
    case ImGuiCol_SliderGrab: return "SliderGrab";
    case ImGuiCol_SliderGrabActive: return "SliderGrabActive";
    case ImGuiCol_Button: return "Button";
    case ImGuiCol_ButtonHovered: return "ButtonHovered";
    case ImGuiCol_ButtonActive: return "ButtonActive";
    case ImGuiCol_Header: return "Header";
    case ImGuiCol_HeaderHovered: return "HeaderHovered";
    case ImGuiCol_HeaderActive: return "HeaderActive";
    case ImGuiCol_Separator: return "Separator";
    case ImGuiCol_SeparatorHovered: return "SeparatorHovered";
    case ImGuiCol_SeparatorActive: return "SeparatorActive";
    case ImGuiCol_ResizeGrip: return "ResizeGrip";
    case ImGuiCol_ResizeGripHovered: return "ResizeGripHovered";
    case ImGuiCol_ResizeGripActive: return "ResizeGripActive";
    case ImGuiCol_PlotLines: return "PlotLines";
    case ImGuiCol_PlotLinesHovered: return "PlotLinesHovered";
    case ImGuiCol_PlotHistogram: return "PlotHistogram";
    case ImGuiCol_PlotHistogramHovered: return "PlotHistogramHovered";
    case ImGuiCol_TextSelectedBg: return "TextSelectedBg";
    case ImGuiCol_DragDropTarget: return "DragDropTarget";
    case ImGuiCol_NavHighlight: return "NavHighlight";
    case ImGuiCol_NavWindowingHighlight: return "NavWindowingHighlight";
    case ImGuiCol_NavWindowingDimBg: return "NavWindowingDimBg";
    case ImGuiCol_ModalWindowDimBg: return "ModalWindowDimBg";
    }
    IM_ASSERT(0);
    return "Unknown";
}

bool ImGui::IsWindowChildOf(ImGuiWindow* window, ImGuiWindow* potential_parent)
{
    if (window->RootWindow == potential_parent)
        return true;
    while (window != NULL)
    {
        if (window == potential_parent)
            return true;
        window = window->ParentWindow;
    }
    return false;
}

bool ImGui::IsWindowHovered(ImGuiHoveredFlags flags)
{
    IM_ASSERT((flags & ImGuiHoveredFlags_AllowWhenOverlapped) == 0);   // Flags not supported by this function
    ImGuiContext& g = *GImGui;

    if (flags & ImGuiHoveredFlags_AnyWindow)
    {
        if (g.HoveredWindow == NULL)
            return false;
    }
    else
    {
        switch (flags & (ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows))
        {
        case ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows:
            if (g.HoveredRootWindow != g.CurrentWindow->RootWindow)
                return false;
            break;
        case ImGuiHoveredFlags_RootWindow:
            if (g.HoveredWindow != g.CurrentWindow->RootWindow)
                return false;
            break;
        case ImGuiHoveredFlags_ChildWindows:
            if (g.HoveredWindow == NULL || !IsWindowChildOf(g.HoveredWindow, g.CurrentWindow))
                return false;
            break;
        default:
            if (g.HoveredWindow != g.CurrentWindow)
                return false;
            break;
        }
    }

    if (!IsWindowContentHoverable(g.HoveredRootWindow, flags))
        return false;
    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && !g.ActiveIdAllowOverlap && g.ActiveId != g.HoveredWindow->MoveId)
            return false;
    return true;
}

bool ImGui::IsWindowFocused(ImGuiFocusedFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentWindow);     // Not inside a Begin()/End()

    if (flags & ImGuiFocusedFlags_AnyWindow)
        return g.NavWindow != NULL;

    switch (flags & (ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows))
    {
    case ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows:
        return g.NavWindow && g.NavWindow->RootWindow == g.CurrentWindow->RootWindow;
    case ImGuiFocusedFlags_RootWindow:
        return g.NavWindow == g.CurrentWindow->RootWindow;
    case ImGuiFocusedFlags_ChildWindows:
        return g.NavWindow && IsWindowChildOf(g.NavWindow, g.CurrentWindow);
    default:
        return g.NavWindow == g.CurrentWindow;
    }
}

// Can we focus this window with CTRL+TAB (or PadMenu + PadFocusPrev/PadFocusNext)
bool ImGui::IsWindowNavFocusable(ImGuiWindow* window)
{
    return window->Active && window == window->RootWindow && !(window->Flags & ImGuiWindowFlags_NoNavFocus);
}

float ImGui::GetWindowWidth()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.x;
}

float ImGui::GetWindowHeight()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.y;
}

ImVec2 ImGui::GetWindowPos()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    return window->Pos;
}

void ImGui::SetWindowScrollX(ImGuiWindow* window, float new_scroll_x)
{
    window->DC.CursorMaxPos.x += window->Scroll.x; // SizeContents is generally computed based on CursorMaxPos which is affected by scroll position, so we need to apply our change to it.
    window->Scroll.x = new_scroll_x;
    window->DC.CursorMaxPos.x -= window->Scroll.x;
}

void ImGui::SetWindowScrollY(ImGuiWindow* window, float new_scroll_y)
{
    window->DC.CursorMaxPos.y += window->Scroll.y; // SizeContents is generally computed based on CursorMaxPos which is affected by scroll position, so we need to apply our change to it.
    window->Scroll.y = new_scroll_y;
    window->DC.CursorMaxPos.y -= window->Scroll.y;
}

static void SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowPosAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
    window->SetWindowPosVal = ImVec2(FLT_MAX, FLT_MAX);

    // Set
    const ImVec2 old_pos = window->Pos;
    window->Pos = ImFloor(pos);
    window->DC.CursorPos += (window->Pos - old_pos);    // As we happen to move the window while it is being appended to (which is a bad idea - will smear) let's at least offset the cursor
    window->DC.CursorMaxPos += (window->Pos - old_pos); // And more importantly we need to adjust this so size calculation doesn't get affected.
}

void ImGui::SetWindowPos(const ImVec2& pos, ImGuiCond cond)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    SetWindowPos(window, pos, cond);
}

void ImGui::SetWindowPos(const char* name, const ImVec2& pos, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowPos(window, pos, cond);
}

ImVec2 ImGui::GetWindowSize()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Size;
}

static void SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowSizeAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    window->SetWindowSizeAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    // Set
    if (size.x > 0.0f)
    {
        window->AutoFitFramesX = 0;
        window->SizeFull.x = size.x;
    }
    else
    {
        window->AutoFitFramesX = 2;
        window->AutoFitOnlyGrows = false;
    }
    if (size.y > 0.0f)
    {
        window->AutoFitFramesY = 0;
        window->SizeFull.y = size.y;
    }
    else
    {
        window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
}

void ImGui::SetWindowSize(const ImVec2& size, ImGuiCond cond)
{
    SetWindowSize(GImGui->CurrentWindow, size, cond);
}

void ImGui::SetWindowSize(const char* name, const ImVec2& size, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowSize(window, size, cond);
}

static void SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowCollapsedAllowFlags & cond) == 0)
        return;
    window->SetWindowCollapsedAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    // Set
    window->Collapsed = collapsed;
}

void ImGui::SetWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    SetWindowCollapsed(GImGui->CurrentWindow, collapsed, cond);
}

bool ImGui::IsWindowCollapsed()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Collapsed;
}

bool ImGui::IsWindowAppearing()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Appearing;
}

void ImGui::SetWindowCollapsed(const char* name, bool collapsed, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowCollapsed(window, collapsed, cond);
}

void ImGui::SetWindowFocus()
{
    FocusWindow(GImGui->CurrentWindow);
}

void ImGui::SetWindowFocus(const char* name)
{
    if (name)
    {
        if (ImGuiWindow* window = FindWindowByName(name))
            FocusWindow(window);
    }
    else
    {
        FocusWindow(NULL);
    }
}

void ImGui::SetNextWindowPos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.PosVal = pos;
    g.NextWindowData.PosPivotVal = pivot;
    g.NextWindowData.PosCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowSize(const ImVec2& size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.SizeVal = size;
    g.NextWindowData.SizeCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback custom_callback, void* custom_callback_user_data)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.SizeConstraintCond = ImGuiCond_Always;
    g.NextWindowData.SizeConstraintRect = ImRect(size_min, size_max);
    g.NextWindowData.SizeCallback = custom_callback;
    g.NextWindowData.SizeCallbackUserData = custom_callback_user_data;
}

void ImGui::SetNextWindowContentSize(const ImVec2& size)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.ContentSizeVal = size;  // In Begin() we will add the size of window decorations (title bar, menu etc.) to that to form a SizeContents value.
    g.NextWindowData.ContentSizeCond = ImGuiCond_Always;
}

void ImGui::SetNextWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.CollapsedVal = collapsed;
    g.NextWindowData.CollapsedCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowFocus()
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.FocusCond = ImGuiCond_Always;   // Using a Cond member for consistency (may transition all of them to single flag set for fast Clear() op)
}

void ImGui::SetNextWindowBgAlpha(float alpha)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.BgAlphaVal = alpha;
    g.NextWindowData.BgAlphaCond = ImGuiCond_Always; // Using a Cond member for consistency (may transition all of them to single flag set for fast Clear() op)
}

// In window space (not screen space!)
ImVec2 ImGui::GetContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImVec2 mx = window->ContentsRegionRect.Max - window->Pos;
    if (window->DC.ColumnsSet)
        mx.x = GetColumnOffset(window->DC.ColumnsSet->Current + 1) - window->WindowPadding.x;
    return mx;
}

ImVec2 ImGui::GetContentRegionAvail()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return GetContentRegionMax() - (window->DC.CursorPos - window->Pos);
}

float ImGui::GetContentRegionAvailWidth()
{
    return GetContentRegionAvail().x;
}

// In window space (not screen space!)
ImVec2 ImGui::GetWindowContentRegionMin()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ContentsRegionRect.Min - window->Pos;
}

ImVec2 ImGui::GetWindowContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ContentsRegionRect.Max - window->Pos;
}

float ImGui::GetWindowContentRegionWidth()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ContentsRegionRect.GetWidth();
}

float ImGui::GetTextLineHeight()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize;
}

float ImGui::GetTextLineHeightWithSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.ItemSpacing.y;
}

float ImGui::GetFrameHeight()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f;
}

float ImGui::GetFrameHeightWithSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f + g.Style.ItemSpacing.y;
}

ImDrawList* ImGui::GetWindowDrawList()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DrawList;
}

ImFont* ImGui::GetFont()
{
    return GImGui->Font;
}

float ImGui::GetFontSize()
{
    return GImGui->FontSize;
}

ImVec2 ImGui::GetFontTexUvWhitePixel()
{
    return GImGui->DrawListSharedData.TexUvWhitePixel;
}

void ImGui::SetWindowFontScale(float scale)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->FontWindowScale = scale;
    g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

// User generally sees positions in window coordinates. Internally we store CursorPos in absolute screen coordinates because it is more convenient.
// Conversion happens as we pass the value to user, but it makes our naming convention confusing because GetCursorPos() == (DC.CursorPos - window.Pos). May want to rename 'DC.CursorPos'.
ImVec2 ImGui::GetCursorPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos - window->Pos + window->Scroll;
}

float ImGui::GetCursorPosX()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos.x - window->Pos.x + window->Scroll.x;
}

float ImGui::GetCursorPosY()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos.y - window->Pos.y + window->Scroll.y;
}

void ImGui::SetCursorPos(const ImVec2& local_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = window->Pos - window->Scroll + local_pos;
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

void ImGui::SetCursorPosX(float x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + x;
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
}

void ImGui::SetCursorPosY(float y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.y = window->Pos.y - window->Scroll.y + y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);
}

ImVec2 ImGui::GetCursorStartPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorStartPos - window->Pos;
}

ImVec2 ImGui::GetCursorScreenPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos;
}

void ImGui::SetCursorScreenPos(const ImVec2& screen_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = screen_pos;
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

float ImGui::GetScrollX()
{
    return GImGui->CurrentWindow->Scroll.x;
}

float ImGui::GetScrollY()
{
    return GImGui->CurrentWindow->Scroll.y;
}

float ImGui::GetScrollMaxX()
{
    return GetScrollMaxX(GImGui->CurrentWindow);
}

float ImGui::GetScrollMaxY()
{
    return GetScrollMaxY(GImGui->CurrentWindow);
}

void ImGui::SetScrollX(float scroll_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->ScrollTarget.x = scroll_x;
    window->ScrollTargetCenterRatio.x = 0.0f;
}

void ImGui::SetScrollY(float scroll_y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->ScrollTarget.y = scroll_y + window->TitleBarHeight() + window->MenuBarHeight(); // title bar height canceled out when using ScrollTargetRelY
    window->ScrollTargetCenterRatio.y = 0.0f;
}

