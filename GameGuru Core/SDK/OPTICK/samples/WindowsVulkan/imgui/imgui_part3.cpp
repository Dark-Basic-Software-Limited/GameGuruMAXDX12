void ImGui::SetScrollFromPosY(float pos_y, float center_y_ratio)
{
    // We store a target position so centering can occur on the next frame when we are guaranteed to have a known window size
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(center_y_ratio >= 0.0f && center_y_ratio <= 1.0f);
    window->ScrollTarget.y = (float)(int)(pos_y + window->Scroll.y);
    window->ScrollTargetCenterRatio.y = center_y_ratio;
}

// center_y_ratio: 0.0f top of last item, 0.5f vertical center of last item, 1.0f bottom of last item.
void ImGui::SetScrollHere(float center_y_ratio)
{
    ImGuiWindow* window = GetCurrentWindow();
    float target_y = window->DC.CursorPosPrevLine.y - window->Pos.y; // Top of last item, in window space
    target_y += (window->DC.PrevLineSize.y * center_y_ratio) + (GImGui->Style.ItemSpacing.y * (center_y_ratio - 0.5f) * 2.0f); // Precisely aim above, in the middle or below the last line.
    SetScrollFromPosY(target_y, center_y_ratio);
}

void ImGui::ActivateItem(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NavNextActivateId = id;
}

void ImGui::SetKeyboardFocusHere(int offset)
{
    IM_ASSERT(offset >= -1);    // -1 is allowed but not below
    ImGuiWindow* window = GetCurrentWindow();
    window->FocusIdxAllRequestNext = window->FocusIdxAllCounter + 1 + offset;
    window->FocusIdxTabRequestNext = INT_MAX;
}

void ImGui::SetItemDefaultFocus()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window->Appearing)
        return;
    if (g.NavWindow == window->RootWindowForNav && (g.NavInitRequest || g.NavInitResultId != 0) && g.NavLayer == g.NavWindow->DC.NavLayerCurrent)
    {
        g.NavInitRequest = false;
        g.NavInitResultId = g.NavWindow->DC.LastItemId;
        g.NavInitResultRectRel = ImRect(g.NavWindow->DC.LastItemRect.Min - g.NavWindow->Pos, g.NavWindow->DC.LastItemRect.Max - g.NavWindow->Pos);
        NavUpdateAnyRequestFlag();
        if (!IsItemVisible())
            SetScrollHere();
    }
}

void ImGui::SetStateStorage(ImGuiStorage* tree)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.StateStorage = tree ? tree : &window->StateStorage;
}

ImGuiStorage* ImGui::GetStateStorage()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.StateStorage;
}

void ImGui::PushID(const char* str_id)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(str_id));
}

void ImGui::PushID(const char* str_id_begin, const char* str_id_end)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(str_id_begin, str_id_end));
}

void ImGui::PushID(const void* ptr_id)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(ptr_id));
}

void ImGui::PushID(int int_id)
{
    const void* ptr_id = (void*)(intptr_t)int_id;
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(ptr_id));
}

void ImGui::PopID()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.pop_back();
}

ImGuiID ImGui::GetID(const char* str_id)
{
    return GImGui->CurrentWindow->GetID(str_id);
}

ImGuiID ImGui::GetID(const char* str_id_begin, const char* str_id_end)
{
    return GImGui->CurrentWindow->GetID(str_id_begin, str_id_end);
}

ImGuiID ImGui::GetID(const void* ptr_id)
{
    return GImGui->CurrentWindow->GetID(ptr_id);
}

bool ImGui::IsRectVisible(const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(ImRect(window->DC.CursorPos, window->DC.CursorPos + size));
}

bool ImGui::IsRectVisible(const ImVec2& rect_min, const ImVec2& rect_max)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(ImRect(rect_min, rect_max));
}

// Lock horizontal starting position + capture group bounding box into one "item" (so you can use IsItemHovered() or layout primitives such as SameLine() on whole group, etc.)
void ImGui::BeginGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    window->DC.GroupStack.resize(window->DC.GroupStack.Size + 1);
    ImGuiGroupData& group_data = window->DC.GroupStack.back();
    group_data.BackupCursorPos = window->DC.CursorPos;
    group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
    group_data.BackupIndent = window->DC.Indent;
    group_data.BackupGroupOffset = window->DC.GroupOffset;
    group_data.BackupCurrentLineSize = window->DC.CurrentLineSize;
    group_data.BackupCurrentLineTextBaseOffset = window->DC.CurrentLineTextBaseOffset;
    group_data.BackupLogLinePosY = window->DC.LogLinePosY;
    group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
    group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
    group_data.AdvanceCursor = true;

    window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
    window->DC.Indent = window->DC.GroupOffset;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.CurrentLineSize = ImVec2(0.0f, 0.0f);
    window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f; // To enforce Log carriage return
}

void ImGui::EndGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(!window->DC.GroupStack.empty());    // Mismatched BeginGroup()/EndGroup() calls

    ImGuiGroupData& group_data = window->DC.GroupStack.back();

    ImRect group_bb(group_data.BackupCursorPos, window->DC.CursorMaxPos);
    group_bb.Max = ImMax(group_bb.Min, group_bb.Max);

    window->DC.CursorPos = group_data.BackupCursorPos;
    window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
    window->DC.Indent = group_data.BackupIndent;
    window->DC.GroupOffset = group_data.BackupGroupOffset;
    window->DC.CurrentLineSize = group_data.BackupCurrentLineSize;
    window->DC.CurrentLineTextBaseOffset = group_data.BackupCurrentLineTextBaseOffset;
    window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f; // To enforce Log carriage return

    if (group_data.AdvanceCursor)
    {
        window->DC.CurrentLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrentLineTextBaseOffset);      // FIXME: Incorrect, we should grab the base offset from the *first line* of the group but it is hard to obtain now.
        ItemSize(group_bb.GetSize(), group_data.BackupCurrentLineTextBaseOffset);
        ItemAdd(group_bb, 0);
    }

    // If the current ActiveId was declared within the boundary of our group, we copy it to LastItemId so IsItemActive(), IsItemDeactivated() etc. will be functional on the entire group.
    // It would be be neater if we replaced window.DC.LastItemId by e.g. 'bool LastItemIsActive', but would put a little more burden on individual widgets.
    // (and if you grep for LastItemId you'll notice it is only used in that context.
    if ((group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId) // && g.ActiveIdWindow->RootWindow == window->RootWindow)
        window->DC.LastItemId = g.ActiveId;
    else if (!group_data.BackupActiveIdPreviousFrameIsAlive && g.ActiveIdPreviousFrameIsAlive) // && g.ActiveIdPreviousFrameWindow->RootWindow == window->RootWindow)
        window->DC.LastItemId = g.ActiveIdPreviousFrame;
    window->DC.LastItemRect = group_bb;

    window->DC.GroupStack.pop_back();

    //window->DrawList->AddRect(group_bb.Min, group_bb.Max, IM_COL32(255,0,255,255));   // [Debug]
}

// Gets back to previous line and continue with horizontal layout
//      pos_x == 0      : follow right after previous item
//      pos_x != 0      : align to specified x position (relative to window/group left)
//      spacing_w < 0   : use default spacing if pos_x == 0, no spacing if pos_x != 0
//      spacing_w >= 0  : enforce spacing amount
void ImGui::SameLine(float pos_x, float spacing_w)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    if (pos_x != 0.0f)
    {
        if (spacing_w < 0.0f) spacing_w = 0.0f;
        window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + pos_x + spacing_w + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    else
    {
        if (spacing_w < 0.0f) spacing_w = g.Style.ItemSpacing.x;
        window->DC.CursorPos.x = window->DC.CursorPosPrevLine.x + spacing_w;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    window->DC.CurrentLineSize = window->DC.PrevLineSize;
    window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
}

void ImGui::Indent(float indent_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.Indent.x += (indent_w != 0.0f) ? indent_w : g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x;
}

void ImGui::Unindent(float indent_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.Indent.x -= (indent_w != 0.0f) ? indent_w : g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x;
}

//-----------------------------------------------------------------------------
// [SECTION] TOOLTIPS
//-----------------------------------------------------------------------------

void ImGui::BeginTooltip()
{
    ImGuiContext& g = *GImGui;
    if (g.DragDropWithinSourceOrTarget)
    {
        // The default tooltip position is a little offset to give space to see the context menu (it's also clamped within the current viewport/monitor)
        // In the context of a dragging tooltip we try to reduce that offset and we enforce following the cursor.
        // Whatever we do we want to call SetNextWindowPos() to enforce a tooltip position and disable clipping the tooltip without our display area, like regular tooltip do.
        //ImVec2 tooltip_pos = g.IO.MousePos - g.ActiveIdClickOffset - g.Style.WindowPadding;
        ImVec2 tooltip_pos = g.IO.MousePos + ImVec2(16 * g.Style.MouseCursorScale, 8 * g.Style.MouseCursorScale);
        SetNextWindowPos(tooltip_pos);
        SetNextWindowBgAlpha(g.Style.Colors[ImGuiCol_PopupBg].w * 0.60f);
        //PushStyleVar(ImGuiStyleVar_Alpha, g.Style.Alpha * 0.60f); // This would be nice but e.g ColorButton with checkboard has issue with transparent colors :(
        BeginTooltipEx(0, true);
    }
    else
    {
        BeginTooltipEx(0, false);
    }
}

// Not exposed publicly as BeginTooltip() because bool parameters are evil. Let's see if other needs arise first.
void ImGui::BeginTooltipEx(ImGuiWindowFlags extra_flags, bool override_previous_tooltip)
{
    ImGuiContext& g = *GImGui;
    char window_name[16];
    ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Tooltip_%02d", g.TooltipOverrideCount);
    if (override_previous_tooltip)
        if (ImGuiWindow* window = FindWindowByName(window_name))
            if (window->Active)
            {
                // Hide previous tooltip from being displayed. We can't easily "reset" the content of a window so we create a new one.
                window->Hidden = true;
                window->HiddenFramesRegular = 1;
                ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Tooltip_%02d", ++g.TooltipOverrideCount);
            }
    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoNav;
    Begin(window_name, NULL, flags | extra_flags);
}

void ImGui::EndTooltip()
{
    IM_ASSERT(GetCurrentWindowRead()->Flags & ImGuiWindowFlags_Tooltip);   // Mismatched BeginTooltip()/EndTooltip() calls
    End();
}

void ImGui::SetTooltipV(const char* fmt, va_list args)
{
    ImGuiContext& g = *GImGui;
    if (g.DragDropWithinSourceOrTarget)
        BeginTooltip();
    else
        BeginTooltipEx(0, true);
    TextV(fmt, args);
    EndTooltip();
}

void ImGui::SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetTooltipV(fmt, args);
    va_end(args);
}

//-----------------------------------------------------------------------------
// [SECTION] POPUPS
//-----------------------------------------------------------------------------

bool ImGui::IsPopupOpen(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    return g.OpenPopupStack.Size > g.CurrentPopupStack.Size && g.OpenPopupStack[g.CurrentPopupStack.Size].PopupId == id;
}

bool ImGui::IsPopupOpen(const char* str_id)
{
    ImGuiContext& g = *GImGui;
    return g.OpenPopupStack.Size > g.CurrentPopupStack.Size && g.OpenPopupStack[g.CurrentPopupStack.Size].PopupId == g.CurrentWindow->GetID(str_id);
}

ImGuiWindow* ImGui::GetFrontMostPopupModal()
{
    ImGuiContext& g = *GImGui;
    for (int n = g.OpenPopupStack.Size-1; n >= 0; n--)
        if (ImGuiWindow* popup = g.OpenPopupStack.Data[n].Window)
            if (popup->Flags & ImGuiWindowFlags_Modal)
                return popup;
    return NULL;
}

void ImGui::OpenPopup(const char* str_id)
{
    ImGuiContext& g = *GImGui;
    OpenPopupEx(g.CurrentWindow->GetID(str_id));
}

// Mark popup as open (toggle toward open state).
// Popups are closed when user click outside, or activate a pressable item, or CloseCurrentPopup() is called within a BeginPopup()/EndPopup() block.
// Popup identifiers are relative to the current ID-stack (so OpenPopup and BeginPopup needs to be at the same level).
// One open popup per level of the popup hierarchy (NB: when assigning we reset the Window member of ImGuiPopupRef to NULL)
void ImGui::OpenPopupEx(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    int current_stack_size = g.CurrentPopupStack.Size;
    ImGuiPopupRef popup_ref; // Tagged as new ref as Window will be set back to NULL if we write this into OpenPopupStack.
    popup_ref.PopupId = id;
    popup_ref.Window = NULL;
    popup_ref.ParentWindow = parent_window;
    popup_ref.OpenFrameCount = g.FrameCount;
    popup_ref.OpenParentId = parent_window->IDStack.back();
    popup_ref.OpenMousePos = g.IO.MousePos;
    popup_ref.OpenPopupPos = NavCalcPreferredRefPos();

    //printf("[%05d] OpenPopupEx(0x%08X)\n", g.FrameCount, id);
    if (g.OpenPopupStack.Size < current_stack_size + 1)
    {
        g.OpenPopupStack.push_back(popup_ref);
    }
    else
    {
        // Gently handle the user mistakenly calling OpenPopup() every frame. It is a programming mistake! However, if we were to run the regular code path, the ui
        // would become completely unusable because the popup will always be in hidden-while-calculating-size state _while_ claiming focus. Which would be a very confusing
        // situation for the programmer. Instead, we silently allow the popup to proceed, it will keep reappearing and the programming error will be more obvious to understand.
        if (g.OpenPopupStack[current_stack_size].PopupId == id && g.OpenPopupStack[current_stack_size].OpenFrameCount == g.FrameCount - 1)
        {
            g.OpenPopupStack[current_stack_size].OpenFrameCount = popup_ref.OpenFrameCount;
        }
        else
        {
            // Close child popups if any, then flag popup for open/reopen
            g.OpenPopupStack.resize(current_stack_size + 1);
            g.OpenPopupStack[current_stack_size] = popup_ref;
        }

        // When reopening a popup we first refocus its parent, otherwise if its parent is itself a popup it would get closed by ClosePopupsOverWindow().
        // This is equivalent to what ClosePopupToLevel() does.
        //if (g.OpenPopupStack[current_stack_size].PopupId == id)
        //    FocusWindow(parent_window);
    }
}

bool ImGui::OpenPopupOnItemClick(const char* str_id, int mouse_button)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId; // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
        IM_ASSERT(id != 0);                                                  // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
        OpenPopupEx(id);
        return true;
    }
    return false;
}

void ImGui::ClosePopupsOverWindow(ImGuiWindow* ref_window)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.empty())
        return;

    // When popups are stacked, clicking on a lower level popups puts focus back to it and close popups above it.
    // Don't close our own child popup windows.
    int n = 0;
    if (ref_window)
    {
        for (n = 0; n < g.OpenPopupStack.Size; n++)
        {
            ImGuiPopupRef& popup = g.OpenPopupStack[n];
            if (!popup.Window)
                continue;
            IM_ASSERT((popup.Window->Flags & ImGuiWindowFlags_Popup) != 0);
            if (popup.Window->Flags & ImGuiWindowFlags_ChildWindow)
                continue;

            // Trim the stack if popups are not direct descendant of the reference window (which is often the NavWindow)
            bool has_focus = false;
            for (int m = n; m < g.OpenPopupStack.Size && !has_focus; m++)
                has_focus = (g.OpenPopupStack[m].Window && g.OpenPopupStack[m].Window->RootWindow == ref_window->RootWindow);
            if (!has_focus)
                break;
        }
    }
    if (n < g.OpenPopupStack.Size) // This test is not required but it allows to set a convenient breakpoint on the block below
        ClosePopupToLevel(n);
}

void ImGui::ClosePopupToLevel(int remaining)
{
    IM_ASSERT(remaining >= 0);
    ImGuiContext& g = *GImGui;
    ImGuiWindow* focus_window = (remaining > 0) ? g.OpenPopupStack[remaining-1].Window : g.OpenPopupStack[0].ParentWindow;
    if (g.NavLayer == 0)
        focus_window = NavRestoreLastChildNavWindow(focus_window);
    FocusWindow(focus_window);
    focus_window->DC.NavHideHighlightOneFrame = true;
    g.OpenPopupStack.resize(remaining);
}

void ImGui::ClosePopup(ImGuiID id)
{
    if (!IsPopupOpen(id))
        return;
    ImGuiContext& g = *GImGui;
    ClosePopupToLevel(g.OpenPopupStack.Size - 1);
}

// Close the popup we have begin-ed into.
void ImGui::CloseCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    int popup_idx = g.CurrentPopupStack.Size - 1;
    if (popup_idx < 0 || popup_idx >= g.OpenPopupStack.Size || g.CurrentPopupStack[popup_idx].PopupId != g.OpenPopupStack[popup_idx].PopupId)
        return;
    while (popup_idx > 0 && g.OpenPopupStack[popup_idx].Window && (g.OpenPopupStack[popup_idx].Window->Flags & ImGuiWindowFlags_ChildMenu))
        popup_idx--;
    ClosePopupToLevel(popup_idx);
}

bool ImGui::BeginPopupEx(ImGuiID id, ImGuiWindowFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(id))
    {
        g.NextWindowData.Clear(); // We behave like Begin() and need to consume those values
        return false;
    }

    char name[20];
    if (extra_flags & ImGuiWindowFlags_ChildMenu)
        ImFormatString(name, IM_ARRAYSIZE(name), "##Menu_%02d", g.CurrentPopupStack.Size);    // Recycle windows based on depth
    else
        ImFormatString(name, IM_ARRAYSIZE(name), "##Popup_%08x", id); // Not recycling, so we can close/open during the same frame

    bool is_open = Begin(name, NULL, extra_flags | ImGuiWindowFlags_Popup);
    if (!is_open) // NB: Begin can return false when the popup is completely clipped (e.g. zero size display)
        EndPopup();

    return is_open;
}

bool ImGui::BeginPopup(const char* str_id, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size <= g.CurrentPopupStack.Size) // Early out for performance
    {
        g.NextWindowData.Clear(); // We behave like Begin() and need to consume those values
        return false;
    }
    return BeginPopupEx(g.CurrentWindow->GetID(str_id), flags|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupModal(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiID id = window->GetID(name);
    if (!IsPopupOpen(id))
    {
        g.NextWindowData.Clear(); // We behave like Begin() and need to consume those values
        return false;
    }

    // Center modal windows by default
    // FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
    if (g.NextWindowData.PosCond == 0)
        SetNextWindowPos(g.IO.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    bool is_open = Begin(name, p_open, flags | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    if (!is_open || (p_open && !*p_open)) // NB: is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
    {
        EndPopup();
        if (is_open)
            ClosePopup(id);
        return false;
    }
    return is_open;
}

void ImGui::EndPopup()
{
    ImGuiContext& g = *GImGui; (void)g;
    IM_ASSERT(g.CurrentWindow->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
    IM_ASSERT(g.CurrentPopupStack.Size > 0);

    // Make all menus and popups wrap around for now, may need to expose that policy.
    NavMoveRequestTryWrapping(g.CurrentWindow, ImGuiNavMoveFlags_LoopY);

    End();
}

// This is a helper to handle the simplest case of associating one named popup to one given widget.
// You may want to handle this on user side if you have specific needs (e.g. tweaking IsItemHovered() parameters).
// You can pass a NULL str_id to use the identifier of the last item.
bool ImGui::BeginPopupContextItem(const char* str_id, int mouse_button)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId; // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
    IM_ASSERT(id != 0);                                                  // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        OpenPopupEx(id);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextWindow(const char* str_id, int mouse_button, bool also_over_items)
{
    if (!str_id)
        str_id = "window_context";
    ImGuiID id = GImGui->CurrentWindow->GetID(str_id);
    if (IsMouseReleased(mouse_button) && IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        if (also_over_items || !IsAnyItemHovered())
            OpenPopupEx(id);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextVoid(const char* str_id, int mouse_button)
{
    if (!str_id)
        str_id = "void_context";
    ImGuiID id = GImGui->CurrentWindow->GetID(str_id);
    if (IsMouseReleased(mouse_button) && !IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        OpenPopupEx(id);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

ImRect ImGui::GetWindowAllowedExtentRect(ImGuiWindow*)
{
    ImVec2 padding = GImGui->Style.DisplaySafeAreaPadding;
    ImRect r_screen = GetViewportRect();
    r_screen.Expand(ImVec2((r_screen.GetWidth() > padding.x * 2) ? -padding.x : 0.0f, (r_screen.GetHeight() > padding.y * 2) ? -padding.y : 0.0f));
    return r_screen;
}

// r_avoid = the rectangle to avoid (e.g. for tooltip it is a rectangle around the mouse cursor which we want to avoid. for popups it's a small point around the cursor.)
// r_outer = the visible area rectangle, minus safe area padding. If our popup size won't fit because of safe area padding we ignore it.
ImVec2 ImGui::FindBestWindowPosForPopupEx(const ImVec2& ref_pos, const ImVec2& size, ImGuiDir* last_dir, const ImRect& r_outer, const ImRect& r_avoid, ImGuiPopupPositionPolicy policy)
{
    ImVec2 base_pos_clamped = ImClamp(ref_pos, r_outer.Min, r_outer.Max - size);
    //GImGui->OverlayDrawList.AddRect(r_avoid.Min, r_avoid.Max, IM_COL32(255,0,0,255));
    //GImGui->OverlayDrawList.AddRect(r_outer.Min, r_outer.Max, IM_COL32(0,255,0,255));

    // Combo Box policy (we want a connecting edge)
    if (policy == ImGuiPopupPositionPolicy_ComboBox)
    {
        const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Down, ImGuiDir_Right, ImGuiDir_Left, ImGuiDir_Up };
        for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
        {
            const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
            if (n != -1 && dir == *last_dir) // Already tried this direction?
                continue;
            ImVec2 pos;
            if (dir == ImGuiDir_Down)  pos = ImVec2(r_avoid.Min.x, r_avoid.Max.y);          // Below, Toward Right (default)
            if (dir == ImGuiDir_Right) pos = ImVec2(r_avoid.Min.x, r_avoid.Min.y - size.y); // Above, Toward Right
            if (dir == ImGuiDir_Left)  pos = ImVec2(r_avoid.Max.x - size.x, r_avoid.Max.y); // Below, Toward Left
            if (dir == ImGuiDir_Up)    pos = ImVec2(r_avoid.Max.x - size.x, r_avoid.Min.y - size.y); // Above, Toward Left
            if (!r_outer.Contains(ImRect(pos, pos + size)))
                continue;
            *last_dir = dir;
            return pos;
        }
    }

    // Default popup policy
    const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Right, ImGuiDir_Down, ImGuiDir_Up, ImGuiDir_Left };
    for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
    {
        const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
        if (n != -1 && dir == *last_dir) // Already tried this direction?
            continue;
        float avail_w = (dir == ImGuiDir_Left ? r_avoid.Min.x : r_outer.Max.x) - (dir == ImGuiDir_Right ? r_avoid.Max.x : r_outer.Min.x);
        float avail_h = (dir == ImGuiDir_Up ? r_avoid.Min.y : r_outer.Max.y) - (dir == ImGuiDir_Down ? r_avoid.Max.y : r_outer.Min.y);
        if (avail_w < size.x || avail_h < size.y)
            continue;
        ImVec2 pos;
        pos.x = (dir == ImGuiDir_Left) ? r_avoid.Min.x - size.x : (dir == ImGuiDir_Right) ? r_avoid.Max.x : base_pos_clamped.x;
        pos.y = (dir == ImGuiDir_Up)   ? r_avoid.Min.y - size.y : (dir == ImGuiDir_Down)  ? r_avoid.Max.y : base_pos_clamped.y;
        *last_dir = dir;
        return pos;
    }

    // Fallback, try to keep within display
    *last_dir = ImGuiDir_None;
    ImVec2 pos = ref_pos;
    pos.x = ImMax(ImMin(pos.x + size.x, r_outer.Max.x) - size.x, r_outer.Min.x);
    pos.y = ImMax(ImMin(pos.y + size.y, r_outer.Max.y) - size.y, r_outer.Min.y);
    return pos;
}

ImVec2 ImGui::FindBestWindowPosForPopup(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;

    ImRect r_outer = GetWindowAllowedExtentRect(window);
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
    {
        // Child menus typically request _any_ position within the parent menu item, and then our FindBestWindowPosForPopup() function will move the new menu outside the parent bounds.
        // This is how we end up with child menus appearing (most-commonly) on the right of the parent menu.
        IM_ASSERT(g.CurrentWindow == window);
        ImGuiWindow* parent_window = g.CurrentWindowStack[g.CurrentWindowStack.Size - 2];
        float horizontal_overlap = g.Style.ItemSpacing.x;       // We want some overlap to convey the relative depth of each menu (currently the amount of overlap is hard-coded to style.ItemSpacing.x).
        ImRect r_avoid;
        if (parent_window->DC.MenuBarAppending)
            r_avoid = ImRect(-FLT_MAX, parent_window->Pos.y + parent_window->TitleBarHeight(), FLT_MAX, parent_window->Pos.y + parent_window->TitleBarHeight() + parent_window->MenuBarHeight());
        else
            r_avoid = ImRect(parent_window->Pos.x + horizontal_overlap, -FLT_MAX, parent_window->Pos.x + parent_window->Size.x - horizontal_overlap - parent_window->ScrollbarSizes.x, FLT_MAX);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid);
    }
    if (window->Flags & ImGuiWindowFlags_Popup)
    {
        ImRect r_avoid = ImRect(window->Pos.x - 1, window->Pos.y - 1, window->Pos.x + 1, window->Pos.y + 1);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid);
    }
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Position tooltip (always follows mouse)
        float sc = g.Style.MouseCursorScale;
        ImVec2 ref_pos = NavCalcPreferredRefPos();
        ImRect r_avoid;
        if (!g.NavDisableHighlight && g.NavDisableMouseHover && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos))
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 16, ref_pos.y + 8);
        else
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 24 * sc, ref_pos.y + 24 * sc); // FIXME: Hard-coded based on mouse cursor shape expectation. Exact dimension not very important.
        ImVec2 pos = FindBestWindowPosForPopupEx(ref_pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid);
        if (window->AutoPosLastDirection == ImGuiDir_None)
            pos = ref_pos + ImVec2(2, 2); // If there's not enough room, for tooltip we prefer avoiding the cursor at all cost even if it means that part of the tooltip won't be visible.
        return pos;
    }
    IM_ASSERT(0);
    return window->Pos;
}

//-----------------------------------------------------------------------------
// [SECTION] KEYBOARD/GAMEPAD NAVIGATION
//-----------------------------------------------------------------------------

ImGuiDir ImGetDirQuadrantFromDelta(float dx, float dy)
{
    if (ImFabs(dx) > ImFabs(dy))
        return (dx > 0.0f) ? ImGuiDir_Right : ImGuiDir_Left;
    return (dy > 0.0f) ? ImGuiDir_Down : ImGuiDir_Up;
}

static float inline NavScoreItemDistInterval(float a0, float a1, float b0, float b1)
{
    if (a1 < b0)
        return a1 - b0;
    if (b1 < a0)
        return a0 - b1;
    return 0.0f;
}

static void inline NavClampRectToVisibleAreaForMoveDir(ImGuiDir move_dir, ImRect& r, const ImRect& clip_rect)
{
    if (move_dir == ImGuiDir_Left || move_dir == ImGuiDir_Right)
    {
        r.Min.y = ImClamp(r.Min.y, clip_rect.Min.y, clip_rect.Max.y);
        r.Max.y = ImClamp(r.Max.y, clip_rect.Min.y, clip_rect.Max.y);
    }
    else
    {
        r.Min.x = ImClamp(r.Min.x, clip_rect.Min.x, clip_rect.Max.x);
        r.Max.x = ImClamp(r.Max.x, clip_rect.Min.x, clip_rect.Max.x);
    }
}

// Scoring function for directional navigation. Based on https://gist.github.com/rygorous/6981057
static bool NavScoreItem(ImGuiNavMoveResult* result, ImRect cand)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavLayer != window->DC.NavLayerCurrent)
        return false;

    const ImRect& curr = g.NavScoringRectScreen; // Current modified source rect (NB: we've applied Max.x = Min.x in NavUpdate() to inhibit the effect of having varied item width)
    g.NavScoringCount++;

    // When entering through a NavFlattened border, we consider child window items as fully clipped for scoring
    if (window->ParentWindow == g.NavWindow)
    {
        IM_ASSERT((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened);
        if (!window->ClipRect.Contains(cand))
            return false;
        cand.ClipWithFull(window->ClipRect); // This allows the scored item to not overlap other candidates in the parent window
    }

    // We perform scoring on items bounding box clipped by the current clipping rectangle on the other axis (clipping on our movement axis would give us equal scores for all clipped items)
    // For example, this ensure that items in one column are not reached when moving vertically from items in another column.
    NavClampRectToVisibleAreaForMoveDir(g.NavMoveClipDir, cand, window->ClipRect);

    // Compute distance between boxes
    // FIXME-NAV: Introducing biases for vertical navigation, needs to be removed.
    float dbx = NavScoreItemDistInterval(cand.Min.x, cand.Max.x, curr.Min.x, curr.Max.x);
    float dby = NavScoreItemDistInterval(ImLerp(cand.Min.y, cand.Max.y, 0.2f), ImLerp(cand.Min.y, cand.Max.y, 0.8f), ImLerp(curr.Min.y, curr.Max.y, 0.2f), ImLerp(curr.Min.y, curr.Max.y, 0.8f)); // Scale down on Y to keep using box-distance for vertically touching items
    if (dby != 0.0f && dbx != 0.0f)
       dbx = (dbx/1000.0f) + ((dbx > 0.0f) ? +1.0f : -1.0f);
    float dist_box = ImFabs(dbx) + ImFabs(dby);

    // Compute distance between centers (this is off by a factor of 2, but we only compare center distances with each other so it doesn't matter)
    float dcx = (cand.Min.x + cand.Max.x) - (curr.Min.x + curr.Max.x);
    float dcy = (cand.Min.y + cand.Max.y) - (curr.Min.y + curr.Max.y);
    float dist_center = ImFabs(dcx) + ImFabs(dcy); // L1 metric (need this for our connectedness guarantee)

    // Determine which quadrant of 'curr' our candidate item 'cand' lies in based on distance
    ImGuiDir quadrant;
    float dax = 0.0f, day = 0.0f, dist_axial = 0.0f;
    if (dbx != 0.0f || dby != 0.0f)
    {
        // For non-overlapping boxes, use distance between boxes
        dax = dbx;
        day = dby;
        dist_axial = dist_box;
        quadrant = ImGetDirQuadrantFromDelta(dbx, dby);
    }
    else if (dcx != 0.0f || dcy != 0.0f)
    {
        // For overlapping boxes with different centers, use distance between centers
        dax = dcx;
        day = dcy;
        dist_axial = dist_center;
        quadrant = ImGetDirQuadrantFromDelta(dcx, dcy);
    }
    else
    {
        // Degenerate case: two overlapping buttons with same center, break ties arbitrarily (note that LastItemId here is really the _previous_ item order, but it doesn't matter)
        quadrant = (window->DC.LastItemId < g.NavId) ? ImGuiDir_Left : ImGuiDir_Right;
    }

#if IMGUI_DEBUG_NAV_SCORING
    char buf[128];
    if (ImGui::IsMouseHoveringRect(cand.Min, cand.Max))
    {
        ImFormatString(buf, IM_ARRAYSIZE(buf), "dbox (%.2f,%.2f->%.4f)\ndcen (%.2f,%.2f->%.4f)\nd (%.2f,%.2f->%.4f)\nnav %c, quadrant %c", dbx, dby, dist_box, dcx, dcy, dist_center, dax, day, dist_axial, "WENS"[g.NavMoveDir], "WENS"[quadrant]);
        ImDrawList* draw_list = ImGui::GetOverlayDrawList();
        draw_list->AddRect(curr.Min, curr.Max, IM_COL32(255,200,0,100));
        draw_list->AddRect(cand.Min, cand.Max, IM_COL32(255,255,0,200));
        draw_list->AddRectFilled(cand.Max-ImVec2(4,4), cand.Max+ImGui::CalcTextSize(buf)+ImVec2(4,4), IM_COL32(40,0,0,150));
        draw_list->AddText(g.IO.FontDefault, 13.0f, cand.Max, ~0U, buf);
    }
    else if (g.IO.KeyCtrl) // Hold to preview score in matching quadrant. Press C to rotate.
    {
        if (IsKeyPressedMap(ImGuiKey_C)) { g.NavMoveDirLast = (ImGuiDir)((g.NavMoveDirLast + 1) & 3); g.IO.KeysDownDuration[g.IO.KeyMap[ImGuiKey_C]] = 0.01f; }
        if (quadrant == g.NavMoveDir)
        {
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%.0f/%.0f", dist_box, dist_center);
            ImDrawList* draw_list = ImGui::GetOverlayDrawList();
            draw_list->AddRectFilled(cand.Min, cand.Max, IM_COL32(255, 0, 0, 200));
            draw_list->AddText(g.IO.FontDefault, 13.0f, cand.Min, IM_COL32(255, 255, 255, 255), buf);
        }
    }
 #endif

    // Is it in the quadrant we're interesting in moving to?
    bool new_best = false;
    if (quadrant == g.NavMoveDir)
    {
        // Does it beat the current best candidate?
        if (dist_box < result->DistBox)
        {
            result->DistBox = dist_box;
            result->DistCenter = dist_center;
            return true;
        }
        if (dist_box == result->DistBox)
        {
            // Try using distance between center points to break ties
            if (dist_center < result->DistCenter)
            {
                result->DistCenter = dist_center;
                new_best = true;
            }
            else if (dist_center == result->DistCenter)
            {
                // Still tied! we need to be extra-careful to make sure everything gets linked properly. We consistently break ties by symbolically moving "later" items
                // (with higher index) to the right/downwards by an infinitesimal amount since we the current "best" button already (so it must have a lower index),
                // this is fairly easy. This rule ensures that all buttons with dx==dy==0 will end up being linked in order of appearance along the x axis.
                if (((g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down) ? dby : dbx) < 0.0f) // moving bj to the right/down decreases distance
                    new_best = true;
            }
        }
    }

    // Axial check: if 'curr' has no link at all in some direction and 'cand' lies roughly in that direction, add a tentative link. This will only be kept if no "real" matches
    // are found, so it only augments the graph produced by the above method using extra links. (important, since it doesn't guarantee strong connectedness)
    // This is just to avoid buttons having no links in a particular direction when there's a suitable neighbor. you get good graphs without this too.
    // 2017/09/29: FIXME: This now currently only enabled inside menu bars, ideally we'd disable it everywhere. Menus in particular need to catch failure. For general navigation it feels awkward.
    // Disabling it may lead to disconnected graphs when nodes are very spaced out on different axis. Perhaps consider offering this as an option?
    if (result->DistBox == FLT_MAX && dist_axial < result->DistAxial)  // Check axial match
        if (g.NavLayer == 1 && !(g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
            if ((g.NavMoveDir == ImGuiDir_Left && dax < 0.0f) || (g.NavMoveDir == ImGuiDir_Right && dax > 0.0f) || (g.NavMoveDir == ImGuiDir_Up && day < 0.0f) || (g.NavMoveDir == ImGuiDir_Down && day > 0.0f))
            {
                result->DistAxial = dist_axial;
                new_best = true;
            }

    return new_best;
}

// We get there when either NavId == id, or when g.NavAnyRequest is set (which is updated by NavUpdateAnyRequestFlag above)
static void ImGui::NavProcessItem(ImGuiWindow* window, const ImRect& nav_bb, const ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    //if (!g.IO.NavActive)  // [2017/10/06] Removed this possibly redundant test but I am not sure of all the side-effects yet. Some of the feature here will need to work regardless of using a _NoNavInputs flag.
    //    return;

    const ImGuiItemFlags item_flags = window->DC.ItemFlags;
    const ImRect nav_bb_rel(nav_bb.Min - window->Pos, nav_bb.Max - window->Pos);

    // Process Init Request
    if (g.NavInitRequest && g.NavLayer == window->DC.NavLayerCurrent)
    {
        // Even if 'ImGuiItemFlags_NoNavDefaultFocus' is on (typically collapse/close button) we record the first ResultId so they can be used as a fallback
        if (!(item_flags & ImGuiItemFlags_NoNavDefaultFocus) || g.NavInitResultId == 0)
        {
            g.NavInitResultId = id;
            g.NavInitResultRectRel = nav_bb_rel;
        }
        if (!(item_flags & ImGuiItemFlags_NoNavDefaultFocus))
        {
            g.NavInitRequest = false; // Found a match, clear request
            NavUpdateAnyRequestFlag();
        }
    }

    // Process Move Request (scoring for navigation)
    // FIXME-NAV: Consider policy for double scoring (scoring from NavScoringRectScreen + scoring from a rect wrapped according to current wrapping policy)
    if ((g.NavId != id || (g.NavMoveRequestFlags & ImGuiNavMoveFlags_AllowCurrentNavId)) && !(item_flags & ImGuiItemFlags_NoNav))
    {
        ImGuiNavMoveResult* result = (window == g.NavWindow) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;
#if IMGUI_DEBUG_NAV_SCORING
        // [DEBUG] Score all items in NavWindow at all times
        if (!g.NavMoveRequest)
            g.NavMoveDir = g.NavMoveDirLast;
        bool new_best = NavScoreItem(result, nav_bb) && g.NavMoveRequest;
#else
        bool new_best = g.NavMoveRequest && NavScoreItem(result, nav_bb);
#endif
        if (new_best)
        {
            result->ID = id;
            result->Window = window;
            result->RectRel = nav_bb_rel;
        }

        const float VISIBLE_RATIO = 0.70f;
        if ((g.NavMoveRequestFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet) && window->ClipRect.Overlaps(nav_bb))
            if (ImClamp(nav_bb.Max.y, window->ClipRect.Min.y, window->ClipRect.Max.y) - ImClamp(nav_bb.Min.y, window->ClipRect.Min.y, window->ClipRect.Max.y) >= (nav_bb.Max.y - nav_bb.Min.y) * VISIBLE_RATIO)
                if (NavScoreItem(&g.NavMoveResultLocalVisibleSet, nav_bb))
                {
                    result = &g.NavMoveResultLocalVisibleSet;
                    result->ID = id;
                    result->Window = window;
                    result->RectRel = nav_bb_rel;
                }
    }

    // Update window-relative bounding box of navigated item
    if (g.NavId == id)
    {
        g.NavWindow = window;                                           // Always refresh g.NavWindow, because some operations such as FocusItem() don't have a window.
        g.NavLayer = window->DC.NavLayerCurrent;
        g.NavIdIsAlive = true;
        g.NavIdTabCounter = window->FocusIdxTabCounter;
        window->NavRectRel[window->DC.NavLayerCurrent] = nav_bb_rel;    // Store item bounding box (relative to window position)
    }
}

bool ImGui::NavMoveRequestButNoResultYet()
{
    ImGuiContext& g = *GImGui;
    return g.NavMoveRequest && g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0;
}

void ImGui::NavMoveRequestCancel()
{
    ImGuiContext& g = *GImGui;
    g.NavMoveRequest = false;
    NavUpdateAnyRequestFlag();
}

void ImGui::NavMoveRequestForward(ImGuiDir move_dir, ImGuiDir clip_dir, const ImRect& bb_rel, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavMoveRequestForward == ImGuiNavForward_None);
    ImGui::NavMoveRequestCancel();
    g.NavMoveDir = move_dir;
    g.NavMoveClipDir = clip_dir;
    g.NavMoveRequestForward = ImGuiNavForward_ForwardQueued;
    g.NavMoveRequestFlags = move_flags;
    g.NavWindow->NavRectRel[g.NavLayer] = bb_rel;
}

void ImGui::NavMoveRequestTryWrapping(ImGuiWindow* window, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;
    if (g.NavWindow != window || !NavMoveRequestButNoResultYet() || g.NavMoveRequestForward != ImGuiNavForward_None || g.NavLayer != 0)
        return;
    IM_ASSERT(move_flags != 0); // No points calling this with no wrapping
    ImRect bb_rel = window->NavRectRel[0];

    ImGuiDir clip_dir = g.NavMoveDir;
    if (g.NavMoveDir == ImGuiDir_Left && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
    {
        bb_rel.Min.x = bb_rel.Max.x = ImMax(window->SizeFull.x, window->SizeContents.x) - window->Scroll.x;
        if (move_flags & ImGuiNavMoveFlags_WrapX) { bb_rel.TranslateY(-bb_rel.GetHeight()); clip_dir = ImGuiDir_Up; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
    if (g.NavMoveDir == ImGuiDir_Right && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
    {
        bb_rel.Min.x = bb_rel.Max.x = -window->Scroll.x;
        if (move_flags & ImGuiNavMoveFlags_WrapX) { bb_rel.TranslateY(+bb_rel.GetHeight()); clip_dir = ImGuiDir_Down; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
    if (g.NavMoveDir == ImGuiDir_Up && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
    {
        bb_rel.Min.y = bb_rel.Max.y = ImMax(window->SizeFull.y, window->SizeContents.y) - window->Scroll.y;
        if (move_flags & ImGuiNavMoveFlags_WrapY) { bb_rel.TranslateX(-bb_rel.GetWidth()); clip_dir = ImGuiDir_Left; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
    if (g.NavMoveDir == ImGuiDir_Down && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
    {
        bb_rel.Min.y = bb_rel.Max.y = -window->Scroll.y;
        if (move_flags & ImGuiNavMoveFlags_WrapY) { bb_rel.TranslateX(+bb_rel.GetWidth()); clip_dir = ImGuiDir_Right; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
}

static void ImGui::NavSaveLastChildNavWindow(ImGuiWindow* nav_window)
{
    ImGuiWindow* parent_window = nav_window;
    while (parent_window && (parent_window->Flags & ImGuiWindowFlags_ChildWindow) != 0 && (parent_window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
        parent_window = parent_window->ParentWindow;
    if (parent_window && parent_window != nav_window)
        parent_window->NavLastChildNavWindow = nav_window;
}

// Call when we are expected to land on Layer 0 after FocusWindow()
static ImGuiWindow* ImGui::NavRestoreLastChildNavWindow(ImGuiWindow* window)
{
    return window->NavLastChildNavWindow ? window->NavLastChildNavWindow : window;
}

static void NavRestoreLayer(int layer)
{
    ImGuiContext& g = *GImGui;
    g.NavLayer = layer;
    if (layer == 0)
        g.NavWindow = ImGui::NavRestoreLastChildNavWindow(g.NavWindow);
    if (layer == 0 && g.NavWindow->NavLastIds[0] != 0)
        ImGui::SetNavIDWithRectRel(g.NavWindow->NavLastIds[0], layer, g.NavWindow->NavRectRel[0]);
    else
        ImGui::NavInitWindow(g.NavWindow, true);
}

static inline void ImGui::NavUpdateAnyRequestFlag()
{
    ImGuiContext& g = *GImGui;
    g.NavAnyRequest = g.NavMoveRequest || g.NavInitRequest || (IMGUI_DEBUG_NAV_SCORING && g.NavWindow != NULL);
    if (g.NavAnyRequest)
        IM_ASSERT(g.NavWindow != NULL);
}

// This needs to be called before we submit any widget (aka in or before Begin)
void ImGui::NavInitWindow(ImGuiWindow* window, bool force_reinit)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window == g.NavWindow);
    bool init_for_nav = false;
    if (!(window->Flags & ImGuiWindowFlags_NoNavInputs))
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow) || (window->Flags & ImGuiWindowFlags_Popup) || (window->NavLastIds[0] == 0) || force_reinit)
            init_for_nav = true;
    if (init_for_nav)
    {
        SetNavID(0, g.NavLayer);
        g.NavInitRequest = true;
        g.NavInitRequestFromMove = false;
        g.NavInitResultId = 0;
        g.NavInitResultRectRel = ImRect();
        NavUpdateAnyRequestFlag();
    }
    else
    {
        g.NavId = window->NavLastIds[0];
    }
}

static ImVec2 ImGui::NavCalcPreferredRefPos()
{
    ImGuiContext& g = *GImGui;
    if (g.NavDisableHighlight || !g.NavDisableMouseHover || !g.NavWindow)
        return ImFloor(g.IO.MousePos);

    // When navigation is active and mouse is disabled, decide on an arbitrary position around the bottom left of the currently navigated item
    const ImRect& rect_rel = g.NavWindow->NavRectRel[g.NavLayer];
    ImVec2 pos = g.NavWindow->Pos + ImVec2(rect_rel.Min.x + ImMin(g.Style.FramePadding.x*4, rect_rel.GetWidth()), rect_rel.Max.y - ImMin(g.Style.FramePadding.y, rect_rel.GetHeight()));
    ImRect visible_rect = GetViewportRect();
    return ImFloor(ImClamp(pos, visible_rect.Min, visible_rect.Max));   // ImFloor() is important because non-integer mouse position application in back-end might be lossy and result in undesirable non-zero delta.
}

float ImGui::GetNavInputAmount(ImGuiNavInput n, ImGuiInputReadMode mode)
{
    ImGuiContext& g = *GImGui;
    if (mode == ImGuiInputReadMode_Down)
        return g.IO.NavInputs[n];                         // Instant, read analog input (0.0f..1.0f, as provided by user)

    const float t = g.IO.NavInputsDownDuration[n];
    if (t < 0.0f && mode == ImGuiInputReadMode_Released)  // Return 1.0f when just released, no repeat, ignore analog input.
        return (g.IO.NavInputsDownDurationPrev[n] >= 0.0f ? 1.0f : 0.0f);
    if (t < 0.0f)
        return 0.0f;
    if (mode == ImGuiInputReadMode_Pressed)               // Return 1.0f when just pressed, no repeat, ignore analog input.
        return (t == 0.0f) ? 1.0f : 0.0f;
    if (mode == ImGuiInputReadMode_Repeat)
        return (float)CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, g.IO.KeyRepeatDelay * 0.80f, g.IO.KeyRepeatRate * 0.80f);
    if (mode == ImGuiInputReadMode_RepeatSlow)
        return (float)CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, g.IO.KeyRepeatDelay * 1.00f, g.IO.KeyRepeatRate * 2.00f);
    if (mode == ImGuiInputReadMode_RepeatFast)
        return (float)CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, g.IO.KeyRepeatDelay * 0.80f, g.IO.KeyRepeatRate * 0.30f);
    return 0.0f;
}

ImVec2 ImGui::GetNavInputAmount2d(ImGuiNavDirSourceFlags dir_sources, ImGuiInputReadMode mode, float slow_factor, float fast_factor)
{
    ImVec2 delta(0.0f, 0.0f);
    if (dir_sources & ImGuiNavDirSourceFlags_Keyboard)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_KeyRight_, mode)   - GetNavInputAmount(ImGuiNavInput_KeyLeft_,   mode), GetNavInputAmount(ImGuiNavInput_KeyDown_,   mode) - GetNavInputAmount(ImGuiNavInput_KeyUp_,   mode));
    if (dir_sources & ImGuiNavDirSourceFlags_PadDPad)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_DpadRight, mode)   - GetNavInputAmount(ImGuiNavInput_DpadLeft,   mode), GetNavInputAmount(ImGuiNavInput_DpadDown,   mode) - GetNavInputAmount(ImGuiNavInput_DpadUp,   mode));
    if (dir_sources & ImGuiNavDirSourceFlags_PadLStick)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_LStickRight, mode) - GetNavInputAmount(ImGuiNavInput_LStickLeft, mode), GetNavInputAmount(ImGuiNavInput_LStickDown, mode) - GetNavInputAmount(ImGuiNavInput_LStickUp, mode));
    if (slow_factor != 0.0f && IsNavInputDown(ImGuiNavInput_TweakSlow))
        delta *= slow_factor;
    if (fast_factor != 0.0f && IsNavInputDown(ImGuiNavInput_TweakFast))
        delta *= fast_factor;
    return delta;
}

// Scroll to keep newly navigated item fully into view
// NB: We modify rect_rel by the amount we scrolled for, so it is immediately updated.
static void NavScrollToBringItemIntoView(ImGuiWindow* window, const ImRect& item_rect)
{
    ImRect window_rect(window->InnerMainRect.Min - ImVec2(1, 1), window->InnerMainRect.Max + ImVec2(1, 1));
    //g.OverlayDrawList.AddRect(window_rect.Min, window_rect.Max, IM_COL32_WHITE); // [DEBUG]
    if (window_rect.Contains(item_rect))
        return;

    ImGuiContext& g = *GImGui;
    if (window->ScrollbarX && item_rect.Min.x < window_rect.Min.x)
    {
        window->ScrollTarget.x = item_rect.Min.x - window->Pos.x + window->Scroll.x - g.Style.ItemSpacing.x;
        window->ScrollTargetCenterRatio.x = 0.0f;
    }
    else if (window->ScrollbarX && item_rect.Max.x >= window_rect.Max.x)
    {
        window->ScrollTarget.x = item_rect.Max.x - window->Pos.x + window->Scroll.x + g.Style.ItemSpacing.x;
        window->ScrollTargetCenterRatio.x = 1.0f;
    }
    if (item_rect.Min.y < window_rect.Min.y)
    {
        window->ScrollTarget.y = item_rect.Min.y - window->Pos.y + window->Scroll.y - g.Style.ItemSpacing.y;
        window->ScrollTargetCenterRatio.y = 0.0f;
    }
    else if (item_rect.Max.y >= window_rect.Max.y)
    {
        window->ScrollTarget.y = item_rect.Max.y - window->Pos.y + window->Scroll.y + g.Style.ItemSpacing.y;
        window->ScrollTargetCenterRatio.y = 1.0f;
    }
}

static void ImGui::NavUpdate()
{
    ImGuiContext& g = *GImGui;
    g.IO.WantSetMousePos = false;
#if 0
    if (g.NavScoringCount > 0) printf("[%05d] NavScoringCount %d for '%s' layer %d (Init:%d, Move:%d)\n", g.FrameCount, g.NavScoringCount, g.NavWindow ? g.NavWindow->Name : "NULL", g.NavLayer, g.NavInitRequest || g.NavInitResultId != 0, g.NavMoveRequest);
#endif

    // Set input source as Gamepad when buttons are pressed before we map Keyboard (some features differs when used with Gamepad vs Keyboard)
    bool nav_keyboard_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    bool nav_gamepad_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (g.IO.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    if (nav_gamepad_active)
        if (g.IO.NavInputs[ImGuiNavInput_Activate] > 0.0f || g.IO.NavInputs[ImGuiNavInput_Input] > 0.0f || g.IO.NavInputs[ImGuiNavInput_Cancel] > 0.0f || g.IO.NavInputs[ImGuiNavInput_Menu] > 0.0f)
            g.NavInputSource = ImGuiInputSource_NavGamepad;

    // Update Keyboard->Nav inputs mapping
    if (nav_keyboard_active)
    {
        #define NAV_MAP_KEY(_KEY, _NAV_INPUT) if (IsKeyDown(g.IO.KeyMap[_KEY])) { g.IO.NavInputs[_NAV_INPUT] = 1.0f; g.NavInputSource = ImGuiInputSource_NavKeyboard; }
        NAV_MAP_KEY(ImGuiKey_Space,     ImGuiNavInput_Activate );
        NAV_MAP_KEY(ImGuiKey_Enter,     ImGuiNavInput_Input    );
        NAV_MAP_KEY(ImGuiKey_Escape,    ImGuiNavInput_Cancel   );
        NAV_MAP_KEY(ImGuiKey_LeftArrow, ImGuiNavInput_KeyLeft_ );
        NAV_MAP_KEY(ImGuiKey_RightArrow,ImGuiNavInput_KeyRight_);
        NAV_MAP_KEY(ImGuiKey_UpArrow,   ImGuiNavInput_KeyUp_   );
        NAV_MAP_KEY(ImGuiKey_DownArrow, ImGuiNavInput_KeyDown_ );
        if (g.IO.KeyCtrl)   g.IO.NavInputs[ImGuiNavInput_TweakSlow] = 1.0f;
        if (g.IO.KeyShift)  g.IO.NavInputs[ImGuiNavInput_TweakFast] = 1.0f;
        if (g.IO.KeyAlt)    g.IO.NavInputs[ImGuiNavInput_KeyMenu_]  = 1.0f;
        #undef NAV_MAP_KEY
    }
    memcpy(g.IO.NavInputsDownDurationPrev, g.IO.NavInputsDownDuration, sizeof(g.IO.NavInputsDownDuration));
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.NavInputs); i++)
        g.IO.NavInputsDownDuration[i] = (g.IO.NavInputs[i] > 0.0f) ? (g.IO.NavInputsDownDuration[i] < 0.0f ? 0.0f : g.IO.NavInputsDownDuration[i] + g.IO.DeltaTime) : -1.0f;

    // Process navigation init request (select first/default focus)
    if (g.NavInitResultId != 0 && (!g.NavDisableHighlight || g.NavInitRequestFromMove))
    {
        // Apply result from previous navigation init request (will typically select the first item, unless SetItemDefaultFocus() has been called)
        IM_ASSERT(g.NavWindow);
        if (g.NavInitRequestFromMove)
            SetNavIDWithRectRel(g.NavInitResultId, g.NavLayer, g.NavInitResultRectRel);
        else
            SetNavID(g.NavInitResultId, g.NavLayer);
        g.NavWindow->NavRectRel[g.NavLayer] = g.NavInitResultRectRel;
    }
    g.NavInitRequest = false;
    g.NavInitRequestFromMove = false;
    g.NavInitResultId = 0;
    g.NavJustMovedToId = 0;

    // Process navigation move request
    if (g.NavMoveRequest && (g.NavMoveResultLocal.ID != 0 || g.NavMoveResultOther.ID != 0))
        NavUpdateMoveResult();

    // When a forwarded move request failed, we restore the highlight that we disabled during the forward frame
    if (g.NavMoveRequestForward == ImGuiNavForward_ForwardActive)
    {
        IM_ASSERT(g.NavMoveRequest);
        if (g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0)
            g.NavDisableHighlight = false;
        g.NavMoveRequestForward = ImGuiNavForward_None;
    }

    // Apply application mouse position movement, after we had a chance to process move request result.
    if (g.NavMousePosDirty && g.NavIdIsAlive)
    {
        // Set mouse position given our knowledge of the navigated item position from last frame
        if ((g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos) && (g.IO.BackendFlags & ImGuiBackendFlags_HasSetMousePos))
        {
            if (!g.NavDisableHighlight && g.NavDisableMouseHover && g.NavWindow)
            {
                g.IO.MousePos = g.IO.MousePosPrev = NavCalcPreferredRefPos();
                g.IO.WantSetMousePos = true;
            }
        }
        g.NavMousePosDirty = false;
    }
    g.NavIdIsAlive = false;
    g.NavJustTabbedId = 0;
    IM_ASSERT(g.NavLayer == 0 || g.NavLayer == 1);

    // Store our return window (for returning from Layer 1 to Layer 0) and clear it as soon as we step back in our own Layer 0
    if (g.NavWindow)
        NavSaveLastChildNavWindow(g.NavWindow);
    if (g.NavWindow && g.NavWindow->NavLastChildNavWindow != NULL && g.NavLayer == 0)
        g.NavWindow->NavLastChildNavWindow = NULL;

    // Update CTRL+TAB and Windowing features (hold Square to move/resize/etc.)
    NavUpdateWindowing();

    // Set output flags for user application
    g.IO.NavActive = (nav_keyboard_active || nav_gamepad_active) && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs);
    g.IO.NavVisible = (g.IO.NavActive && g.NavId != 0 && !g.NavDisableHighlight) || (g.NavWindowingTarget != NULL) || g.NavInitRequest;

    // Process NavCancel input (to close a popup, get back to parent, clear focus)
    if (IsNavInputPressed(ImGuiNavInput_Cancel, ImGuiInputReadMode_Pressed))
    {
        if (g.ActiveId != 0)
        {
            ClearActiveID();
        }
        else if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow) && !(g.NavWindow->Flags & ImGuiWindowFlags_Popup) && g.NavWindow->ParentWindow)
        {
            // Exit child window
            ImGuiWindow* child_window = g.NavWindow;
            ImGuiWindow* parent_window = g.NavWindow->ParentWindow;
            IM_ASSERT(child_window->ChildId != 0);
            FocusWindow(parent_window);
            SetNavID(child_window->ChildId, 0);
            g.NavIdIsAlive = false;
            if (g.NavDisableMouseHover)
                g.NavMousePosDirty = true;
        }
        else if (g.OpenPopupStack.Size > 0)
        {
            // Close open popup/menu
            if (!(g.OpenPopupStack.back().Window->Flags & ImGuiWindowFlags_Modal))
                ClosePopupToLevel(g.OpenPopupStack.Size - 1);
        }
        else if (g.NavLayer != 0)
        {
            // Leave the "menu" layer
            NavRestoreLayer(0);
        }
        else
        {
            // Clear NavLastId for popups but keep it for regular child window so we can leave one and come back where we were
            if (g.NavWindow && ((g.NavWindow->Flags & ImGuiWindowFlags_Popup) || !(g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow)))
                g.NavWindow->NavLastIds[0] = 0;
            g.NavId = 0;
        }
    }

    // Process manual activation request
    g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavInputId = 0;
    if (g.NavId != 0 && !g.NavDisableHighlight && !g.NavWindowingTarget && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
    {
        bool activate_down = IsNavInputDown(ImGuiNavInput_Activate);
        bool activate_pressed = activate_down && IsNavInputPressed(ImGuiNavInput_Activate, ImGuiInputReadMode_Pressed);
        if (g.ActiveId == 0 && activate_pressed)
            g.NavActivateId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && activate_down)
            g.NavActivateDownId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && activate_pressed)
            g.NavActivatePressedId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && IsNavInputPressed(ImGuiNavInput_Input, ImGuiInputReadMode_Pressed))
            g.NavInputId = g.NavId;
    }
    if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        g.NavDisableHighlight = true;
    if (g.NavActivateId != 0)
        IM_ASSERT(g.NavActivateDownId == g.NavActivateId);
    g.NavMoveRequest = false;

    // Process programmatic activation request
    if (g.NavNextActivateId != 0)
        g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavInputId = g.NavNextActivateId;
    g.NavNextActivateId = 0;

    // Initiate directional inputs request
    const int allowed_dir_flags = (g.ActiveId == 0) ? ~0 : g.ActiveIdAllowNavDirFlags;
    if (g.NavMoveRequestForward == ImGuiNavForward_None)
    {
        g.NavMoveDir = ImGuiDir_None;
        g.NavMoveRequestFlags = 0;
        if (g.NavWindow && !g.NavWindowingTarget && allowed_dir_flags && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        {
            if ((allowed_dir_flags & (1<<ImGuiDir_Left))  && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadLeft, ImGuiNavInput_KeyLeft_, ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Left;
            if ((allowed_dir_flags & (1<<ImGuiDir_Right)) && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadRight,ImGuiNavInput_KeyRight_,ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Right;
            if ((allowed_dir_flags & (1<<ImGuiDir_Up))    && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadUp,   ImGuiNavInput_KeyUp_,   ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Up;
            if ((allowed_dir_flags & (1<<ImGuiDir_Down))  && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadDown, ImGuiNavInput_KeyDown_, ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Down;
        }
        g.NavMoveClipDir = g.NavMoveDir;
    }
    else
    {
        // Forwarding previous request (which has been modified, e.g. wrap around menus rewrite the requests with a starting rectangle at the other side of the window)
        // (Preserve g.NavMoveRequestFlags, g.NavMoveClipDir which were set by the NavMoveRequestForward() function)
        IM_ASSERT(g.NavMoveDir != ImGuiDir_None && g.NavMoveClipDir != ImGuiDir_None);
        IM_ASSERT(g.NavMoveRequestForward == ImGuiNavForward_ForwardQueued);
        g.NavMoveRequestForward = ImGuiNavForward_ForwardActive;
    }

    // Update PageUp/PageDown scroll
    float nav_scoring_rect_offset_y = 0.0f;
    if (nav_keyboard_active)
        nav_scoring_rect_offset_y = NavUpdatePageUpPageDown(allowed_dir_flags);

    // If we initiate a movement request and have no current NavId, we initiate a InitDefautRequest that will be used as a fallback if the direction fails to find a match
    if (g.NavMoveDir != ImGuiDir_None)
    {
        g.NavMoveRequest = true;
        g.NavMoveDirLast = g.NavMoveDir;
    }
    if (g.NavMoveRequest && g.NavId == 0)
    {
        g.NavInitRequest = g.NavInitRequestFromMove = true;
        g.NavInitResultId = 0;
        g.NavDisableHighlight = false;
    }
    NavUpdateAnyRequestFlag();

    // Scrolling
    if (g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.NavWindowingTarget)
    {
        // *Fallback* manual-scroll with Nav directional keys when window has no navigable item
        ImGuiWindow* window = g.NavWindow;
        const float scroll_speed = ImFloor(window->CalcFontSize() * 100 * g.IO.DeltaTime + 0.5f); // We need round the scrolling speed because sub-pixel scroll isn't reliably supported.
        if (window->DC.NavLayerActiveMask == 0x00 && window->DC.NavHasScroll && g.NavMoveRequest)
        {
            if (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right)
                SetWindowScrollX(window, ImFloor(window->Scroll.x + ((g.NavMoveDir == ImGuiDir_Left) ? -1.0f : +1.0f) * scroll_speed));
            if (g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down)
                SetWindowScrollY(window, ImFloor(window->Scroll.y + ((g.NavMoveDir == ImGuiDir_Up) ? -1.0f : +1.0f) * scroll_speed));
        }

        // *Normal* Manual scroll with NavScrollXXX keys
        // Next movement request will clamp the NavId reference rectangle to the visible area, so navigation will resume within those bounds.
        ImVec2 scroll_dir = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadLStick, ImGuiInputReadMode_Down, 1.0f/10.0f, 10.0f);
        if (scroll_dir.x != 0.0f && window->ScrollbarX)
        {
            SetWindowScrollX(window, ImFloor(window->Scroll.x + scroll_dir.x * scroll_speed));
            g.NavMoveFromClampedRefRect = true;
        }
        if (scroll_dir.y != 0.0f)
        {
            SetWindowScrollY(window, ImFloor(window->Scroll.y + scroll_dir.y * scroll_speed));
            g.NavMoveFromClampedRefRect = true;
        }
    }

    // Reset search results
    g.NavMoveResultLocal.Clear();
    g.NavMoveResultLocalVisibleSet.Clear();
    g.NavMoveResultOther.Clear();

    // When we have manually scrolled (without using navigation) and NavId becomes out of bounds, we project its bounding box to the visible area to restart navigation within visible items
    if (g.NavMoveRequest && g.NavMoveFromClampedRefRect && g.NavLayer == 0)
    {
        ImGuiWindow* window = g.NavWindow;
        ImRect window_rect_rel(window->InnerMainRect.Min - window->Pos - ImVec2(1,1), window->InnerMainRect.Max - window->Pos + ImVec2(1,1));
        if (!window_rect_rel.Contains(window->NavRectRel[g.NavLayer]))
        {
            float pad = window->CalcFontSize() * 0.5f;
            window_rect_rel.Expand(ImVec2(-ImMin(window_rect_rel.GetWidth(), pad), -ImMin(window_rect_rel.GetHeight(), pad))); // Terrible approximation for the intent of starting navigation from first fully visible item
            window->NavRectRel[g.NavLayer].ClipWith(window_rect_rel);
            g.NavId = 0;
        }
        g.NavMoveFromClampedRefRect = false;
    }

    // For scoring we use a single segment on the left side our current item bounding box (not touching the edge to avoid box overlap with zero-spaced items)
    ImRect nav_rect_rel = (g.NavWindow && !g.NavWindow->NavRectRel[g.NavLayer].IsInverted()) ? g.NavWindow->NavRectRel[g.NavLayer] : ImRect(0,0,0,0);
    g.NavScoringRectScreen = g.NavWindow ? ImRect(g.NavWindow->Pos + nav_rect_rel.Min, g.NavWindow->Pos + nav_rect_rel.Max) : GetViewportRect();
    g.NavScoringRectScreen.TranslateY(nav_scoring_rect_offset_y);
    g.NavScoringRectScreen.Min.x = ImMin(g.NavScoringRectScreen.Min.x + 1.0f, g.NavScoringRectScreen.Max.x);
    g.NavScoringRectScreen.Max.x = g.NavScoringRectScreen.Min.x;
    IM_ASSERT(!g.NavScoringRectScreen.IsInverted()); // Ensure if we have a finite, non-inverted bounding box here will allows us to remove extraneous ImFabs() calls in NavScoreItem().
    //g.OverlayDrawList.AddRect(g.NavScoringRectScreen.Min, g.NavScoringRectScreen.Max, IM_COL32(255,200,0,255)); // [DEBUG]
    g.NavScoringCount = 0;
#if IMGUI_DEBUG_NAV_RECTS
    if (g.NavWindow) { for (int layer = 0; layer < 2; layer++) GetOverlayDrawList()->AddRect(g.NavWindow->Pos + g.NavWindow->NavRectRel[layer].Min, g.NavWindow->Pos + g.NavWindow->NavRectRel[layer].Max, IM_COL32(255,200,0,255)); } // [DEBUG]
    if (g.NavWindow) { ImU32 col = (g.NavWindow->HiddenFrames == 0) ? IM_COL32(255,0,255,255) : IM_COL32(255,0,0,255); ImVec2 p = NavCalcPreferredRefPos(); char buf[32]; ImFormatString(buf, 32, "%d", g.NavLayer); g.OverlayDrawList.AddCircleFilled(p, 3.0f, col); g.OverlayDrawList.AddText(NULL, 13.0f, p + ImVec2(8,-4), col, buf); }
#endif
}

static void ImGui::NavUpdateMoveResult()
{
    // Select which result to use
    ImGuiContext& g = *GImGui;
    ImGuiNavMoveResult* result = (g.NavMoveResultLocal.ID != 0) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;

    // PageUp/PageDown behavior first jumps to the bottom/top mostly visible item, _otherwise_ use the result from the previous/next page.
    if (g.NavMoveRequestFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet)
        if (g.NavMoveResultLocalVisibleSet.ID != 0 && g.NavMoveResultLocalVisibleSet.ID != g.NavId)
            result = &g.NavMoveResultLocalVisibleSet;

    // Maybe entering a flattened child from the outside? In this case solve the tie using the regular scoring rules.
    if (result != &g.NavMoveResultOther && g.NavMoveResultOther.ID != 0 && g.NavMoveResultOther.Window->ParentWindow == g.NavWindow)
        if ((g.NavMoveResultOther.DistBox < result->DistBox) || (g.NavMoveResultOther.DistBox == result->DistBox && g.NavMoveResultOther.DistCenter < result->DistCenter))
            result = &g.NavMoveResultOther;
    IM_ASSERT(g.NavWindow && result->Window);

    // Scroll to keep newly navigated item fully into view.
    if (g.NavLayer == 0)
    {
        ImRect rect_abs = ImRect(result->RectRel.Min + result->Window->Pos, result->RectRel.Max + result->Window->Pos);
        NavScrollToBringItemIntoView(result->Window, rect_abs);

        // Estimate upcoming scroll so we can offset our result position so mouse position can be applied immediately after in NavUpdate()
        ImVec2 next_scroll = CalcNextScrollFromScrollTargetAndClamp(result->Window, false);
        ImVec2 delta_scroll = result->Window->Scroll - next_scroll;
        result->RectRel.Translate(delta_scroll);

        // Also scroll parent window to keep us into view if necessary (we could/should technically recurse back the whole the parent hierarchy).
        if (result->Window->Flags & ImGuiWindowFlags_ChildWindow)
            NavScrollToBringItemIntoView(result->Window->ParentWindow, ImRect(rect_abs.Min + delta_scroll, rect_abs.Max + delta_scroll));
    }

    // Apply result from previous frame navigation directional move request
    ClearActiveID();
    g.NavWindow = result->Window;
    SetNavIDWithRectRel(result->ID, g.NavLayer, result->RectRel);
    g.NavJustMovedToId = result->ID;
    g.NavMoveFromClampedRefRect = false;
}

static float ImGui::NavUpdatePageUpPageDown(int allowed_dir_flags)
{
    ImGuiContext& g = *GImGui;
    if (g.NavMoveDir == ImGuiDir_None && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.NavWindowingTarget && g.NavLayer == 0)
    {
        ImGuiWindow* window = g.NavWindow;
        bool page_up_held = IsKeyDown(g.IO.KeyMap[ImGuiKey_PageUp]) && (allowed_dir_flags & (1 << ImGuiDir_Up));
        bool page_down_held = IsKeyDown(g.IO.KeyMap[ImGuiKey_PageDown]) && (allowed_dir_flags & (1 << ImGuiDir_Down));
        if ((page_up_held && !page_down_held) || (page_down_held && !page_up_held))
        {
            if (window->DC.NavLayerActiveMask == 0x00 && window->DC.NavHasScroll)
            {
                // Fallback manual-scroll when window has no navigable item
                if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageUp], true))
                    SetWindowScrollY(window, window->Scroll.y - window->InnerClipRect.GetHeight());
                else if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageDown], true))
                    SetWindowScrollY(window, window->Scroll.y + window->InnerClipRect.GetHeight());
            }
            else
            {
                const ImRect& nav_rect_rel = window->NavRectRel[g.NavLayer];
                const float page_offset_y = ImMax(0.0f, window->InnerClipRect.GetHeight() - window->CalcFontSize() * 1.0f + nav_rect_rel.GetHeight());
                float nav_scoring_rect_offset_y = 0.0f;
                if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageUp], true))
                {
                    nav_scoring_rect_offset_y = -page_offset_y;
                    g.NavMoveDir = ImGuiDir_Down; // Because our scoring rect is offset, we intentionally request the opposite direction (so we can always land on the last item)
                    g.NavMoveClipDir = ImGuiDir_Up;
                    g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet;
                }
                else if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageDown], true))
                {
                    nav_scoring_rect_offset_y = +page_offset_y;
                    g.NavMoveDir = ImGuiDir_Up; // Because our scoring rect is offset, we intentionally request the opposite direction (so we can always land on the last item)
                    g.NavMoveClipDir = ImGuiDir_Down;
                    g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet;
                }
                return nav_scoring_rect_offset_y;
            }
        }
    }
    return 0.0f;
}

static int FindWindowIndex(ImGuiWindow* window) // FIXME-OPT O(N)
{
    ImGuiContext& g = *GImGui;
    for (int i = g.Windows.Size-1; i >= 0; i--)
        if (g.Windows[i] == window)
            return i;
    return -1;
}

static ImGuiWindow* FindWindowNavFocusable(int i_start, int i_stop, int dir) // FIXME-OPT O(N)
{
    ImGuiContext& g = *GImGui;
    for (int i = i_start; i >= 0 && i < g.Windows.Size && i != i_stop; i += dir)
        if (ImGui::IsWindowNavFocusable(g.Windows[i]))
            return g.Windows[i];
    return NULL;
}

static void NavUpdateWindowingHighlightWindow(int focus_change_dir)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget);
    if (g.NavWindowingTarget->Flags & ImGuiWindowFlags_Modal)
        return;

    const int i_current = FindWindowIndex(g.NavWindowingTarget);
    ImGuiWindow* window_target = FindWindowNavFocusable(i_current + focus_change_dir, -INT_MAX, focus_change_dir);
    if (!window_target)
        window_target = FindWindowNavFocusable((focus_change_dir < 0) ? (g.Windows.Size - 1) : 0, i_current, focus_change_dir);
    if (window_target) // Don't reset windowing target if there's a single window in the list
        g.NavWindowingTarget = g.NavWindowingTargetAnim = window_target;
    g.NavWindowingToggleLayer = false;
}

// Window management mode (hold to: change focus/move/resize, tap to: toggle menu layer)
static void ImGui::NavUpdateWindowing()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* apply_focus_window = NULL;
    bool apply_toggle_layer = false;

    ImGuiWindow* modal_window = GetFrontMostPopupModal();
    if (modal_window != NULL)
    {
        g.NavWindowingTarget = NULL;
        return;
    }

    // Fade out
    if (g.NavWindowingTargetAnim && g.NavWindowingTarget == NULL)
    {
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha - g.IO.DeltaTime * 10.0f, 0.0f);
        if (g.DimBgRatio <= 0.0f && g.NavWindowingHighlightAlpha <= 0.0f)
            g.NavWindowingTargetAnim = NULL;
    }

    // Start CTRL-TAB or Square+L/R window selection
    bool start_windowing_with_gamepad = !g.NavWindowingTarget && IsNavInputPressed(ImGuiNavInput_Menu, ImGuiInputReadMode_Pressed);
    bool start_windowing_with_keyboard = !g.NavWindowingTarget && g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab) && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
    if (start_windowing_with_gamepad || start_windowing_with_keyboard)
        if (ImGuiWindow* window = g.NavWindow ? g.NavWindow : FindWindowNavFocusable(g.Windows.Size - 1, -INT_MAX, -1))
        {
            g.NavWindowingTarget = g.NavWindowingTargetAnim = window;
            g.NavWindowingTimer = g.NavWindowingHighlightAlpha = 0.0f;
            g.NavWindowingToggleLayer = start_windowing_with_keyboard ? false : true;
            g.NavInputSource = start_windowing_with_keyboard ? ImGuiInputSource_NavKeyboard : ImGuiInputSource_NavGamepad;
        }

    // Gamepad update
    g.NavWindowingTimer += g.IO.DeltaTime;
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_NavGamepad)
    {
        // Highlight only appears after a brief time holding the button, so that a fast tap on PadMenu (to toggle NavLayer) doesn't add visual noise
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f));

        // Select window to focus
        const int focus_change_dir = (int)IsNavInputPressed(ImGuiNavInput_FocusPrev, ImGuiInputReadMode_RepeatSlow) - (int)IsNavInputPressed(ImGuiNavInput_FocusNext, ImGuiInputReadMode_RepeatSlow);
        if (focus_change_dir != 0)
        {
            NavUpdateWindowingHighlightWindow(focus_change_dir);
            g.NavWindowingHighlightAlpha = 1.0f;
        }

        // Single press toggles NavLayer, long press with L/R apply actual focus on release (until then the window was merely rendered front-most)
        if (!IsNavInputDown(ImGuiNavInput_Menu))
        {
            g.NavWindowingToggleLayer &= (g.NavWindowingHighlightAlpha < 1.0f); // Once button was held long enough we don't consider it a tap-to-toggle-layer press anymore.
            if (g.NavWindowingToggleLayer && g.NavWindow)
                apply_toggle_layer = true;
            else if (!g.NavWindowingToggleLayer)
                apply_focus_window = g.NavWindowingTarget;
            g.NavWindowingTarget = NULL;
        }
    }

    // Keyboard: Focus
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_NavKeyboard)
    {
        // Visuals only appears after a brief time after pressing TAB the first time, so that a fast CTRL+TAB doesn't add visual noise
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f)); // 1.0f
        if (IsKeyPressedMap(ImGuiKey_Tab, true))
            NavUpdateWindowingHighlightWindow(g.IO.KeyShift ? +1 : -1);
        if (!g.IO.KeyCtrl)
            apply_focus_window = g.NavWindowingTarget;
    }

    // Keyboard: Press and Release ALT to toggle menu layer
    // FIXME: We lack an explicit IO variable for "is the imgui window focused", so compare mouse validity to detect the common case of back-end clearing releases all keys on ALT-TAB
    if ((g.ActiveId == 0 || g.ActiveIdAllowOverlap) && IsNavInputPressed(ImGuiNavInput_KeyMenu_, ImGuiInputReadMode_Released))
        if (IsMousePosValid(&g.IO.MousePos) == IsMousePosValid(&g.IO.MousePosPrev))
            apply_toggle_layer = true;

    // Move window
    if (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoMove))
    {
        ImVec2 move_delta;
        if (g.NavInputSource == ImGuiInputSource_NavKeyboard && !g.IO.KeyShift)
            move_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_NavGamepad)
            move_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadLStick, ImGuiInputReadMode_Down);
        if (move_delta.x != 0.0f || move_delta.y != 0.0f)
        {
            const float NAV_MOVE_SPEED = 800.0f;
            const float move_speed = ImFloor(NAV_MOVE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y)); // FIXME: Doesn't code variable framerate very well
            g.NavWindowingTarget->RootWindow->Pos += move_delta * move_speed;
            g.NavDisableMouseHover = true;
            MarkIniSettingsDirty(g.NavWindowingTarget);
        }
    }

    // Apply final focus
    if (apply_focus_window && (g.NavWindow == NULL || apply_focus_window != g.NavWindow->RootWindow))
    {
        g.NavDisableHighlight = false;
        g.NavDisableMouseHover = true;
        apply_focus_window = NavRestoreLastChildNavWindow(apply_focus_window);
        ClosePopupsOverWindow(apply_focus_window);
        FocusWindow(apply_focus_window);
        if (apply_focus_window->NavLastIds[0] == 0)
            NavInitWindow(apply_focus_window, false);

        // If the window only has a menu layer, select it directly
        if (apply_focus_window->DC.NavLayerActiveMask == (1 << 1))
            g.NavLayer = 1;
    }
    if (apply_focus_window)
        g.NavWindowingTarget = NULL;

    // Apply menu/layer toggle
    if (apply_toggle_layer && g.NavWindow)
    {
        // Move to parent menu if necessary
        ImGuiWindow* new_nav_window = g.NavWindow;
        while ((new_nav_window->DC.NavLayerActiveMask & (1 << 1)) == 0 && (new_nav_window->Flags & ImGuiWindowFlags_ChildWindow) != 0 && (new_nav_window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
            new_nav_window = new_nav_window->ParentWindow;
        if (new_nav_window != g.NavWindow)
        {
            ImGuiWindow* old_nav_window = g.NavWindow;
            FocusWindow(new_nav_window);
            new_nav_window->NavLastChildNavWindow = old_nav_window;
        }
        g.NavDisableHighlight = false;
        g.NavDisableMouseHover = true;
        NavRestoreLayer((g.NavWindow->DC.NavLayerActiveMask & (1 << 1)) ? (g.NavLayer ^ 1) : 0);
    }
}

// Window has already passed the IsWindowNavFocusable()
static const char* GetFallbackWindowNameForWindowingList(ImGuiWindow* window)
{
    if (window->Flags & ImGuiWindowFlags_Popup)
        return "(Popup)";
    if ((window->Flags & ImGuiWindowFlags_MenuBar) && strcmp(window->Name, "##MainMenuBar") == 0)
        return "(Main menu bar)";
    return "(Untitled)";
}

// Overlay displayed when using CTRL+TAB. Called by EndFrame().
void ImGui::NavUpdateWindowingList()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget != NULL);

    if (g.NavWindowingTimer < NAV_WINDOWING_LIST_APPEAR_DELAY)
        return;

    if (g.NavWindowingList == NULL)
        g.NavWindowingList = FindWindowByName("###NavWindowingList");
    SetNextWindowSizeConstraints(ImVec2(g.IO.DisplaySize.x * 0.20f, g.IO.DisplaySize.y * 0.20f), ImVec2(FLT_MAX, FLT_MAX));
    SetNextWindowPos(g.IO.DisplaySize * 0.5f, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.WindowPadding * 2.0f);
    Begin("###NavWindowingList", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    for (int n = g.Windows.Size - 1; n >= 0; n--)
    {
        ImGuiWindow* window = g.Windows[n];
        if (!IsWindowNavFocusable(window))
            continue;
        const char* label = window->Name;
        if (label == FindRenderedTextEnd(label))
            label = GetFallbackWindowNameForWindowingList(window);
        Selectable(label, g.NavWindowingTarget == window);
    }
    End();
    PopStyleVar();
}

//-----------------------------------------------------------------------------
// [SECTION] COLUMNS
// In the current version, Columns are very weak. Needs to be replaced with a more full-featured system.
//-----------------------------------------------------------------------------

void ImGui::NextColumn()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems || window->DC.ColumnsSet == NULL)
        return;

    ImGuiContext& g = *GImGui;
    PopItemWidth();
    PopClipRect();

    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    if (++columns->Current < columns->Count)
    {
        // Columns 1+ cancel out IndentX
        window->DC.ColumnsOffset.x = GetColumnOffset(columns->Current) - window->DC.Indent.x + g.Style.ItemSpacing.x;
        window->DrawList->ChannelsSetCurrent(columns->Current);
    }
    else
    {
        window->DC.ColumnsOffset.x = 0.0f;
        window->DrawList->ChannelsSetCurrent(0);
        columns->Current = 0;
        columns->LineMinY = columns->LineMaxY;
    }
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->DC.CursorPos.y = columns->LineMinY;
    window->DC.CurrentLineSize = ImVec2(0.0f, 0.0f);
    window->DC.CurrentLineTextBaseOffset = 0.0f;

    PushColumnClipRect();
    PushItemWidth(GetColumnWidth() * 0.65f);  // FIXME: Move on columns setup
}

int ImGui::GetColumnIndex()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.ColumnsSet ? window->DC.ColumnsSet->Current : 0;
}

int ImGui::GetColumnsCount()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.ColumnsSet ? window->DC.ColumnsSet->Count : 1;
}

static float OffsetNormToPixels(const ImGuiColumnsSet* columns, float offset_norm)
{
    return offset_norm * (columns->MaxX - columns->MinX);
}

static float PixelsToOffsetNorm(const ImGuiColumnsSet* columns, float offset)
{
    return offset / (columns->MaxX - columns->MinX);
}

static inline float GetColumnsRectHalfWidth() { return 4.0f; }

static float GetDraggedColumnOffset(ImGuiColumnsSet* columns, int column_index)
{
    // Active (dragged) column always follow mouse. The reason we need this is that dragging a column to the right edge of an auto-resizing
    // window creates a feedback loop because we store normalized positions. So while dragging we enforce absolute positioning.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(column_index > 0); // We are not supposed to drag column 0.
    IM_ASSERT(g.ActiveId == columns->ID + ImGuiID(column_index));

    float x = g.IO.MousePos.x - g.ActiveIdClickOffset.x + GetColumnsRectHalfWidth() - window->Pos.x;
    x = ImMax(x, ImGui::GetColumnOffset(column_index - 1) + g.Style.ColumnsMinSpacing);
    if ((columns->Flags & ImGuiColumnsFlags_NoPreserveWidths))
        x = ImMin(x, ImGui::GetColumnOffset(column_index + 1) - g.Style.ColumnsMinSpacing);

    return x;
}

float ImGui::GetColumnOffset(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const float t = columns->Columns[column_index].OffsetNorm;
    const float x_offset = ImLerp(columns->MinX, columns->MaxX, t);
    return x_offset;
}

static float GetColumnWidthEx(ImGuiColumnsSet* columns, int column_index, bool before_resize = false)
{
    if (column_index < 0)
        column_index = columns->Current;

    float offset_norm;
    if (before_resize)
        offset_norm = columns->Columns[column_index + 1].OffsetNormBeforeResize - columns->Columns[column_index].OffsetNormBeforeResize;
    else
        offset_norm = columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm;
    return OffsetNormToPixels(columns, offset_norm);
}

float ImGui::GetColumnWidth(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    return OffsetNormToPixels(columns, columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm);
}

void ImGui::SetColumnOffset(int column_index, float offset)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const bool preserve_width = !(columns->Flags & ImGuiColumnsFlags_NoPreserveWidths) && (column_index < columns->Count-1);
    const float width = preserve_width ? GetColumnWidthEx(columns, column_index, columns->IsBeingResized) : 0.0f;

    if (!(columns->Flags & ImGuiColumnsFlags_NoForceWithinWindow))
        offset = ImMin(offset, columns->MaxX - g.Style.ColumnsMinSpacing * (columns->Count - column_index));
    columns->Columns[column_index].OffsetNorm = PixelsToOffsetNorm(columns, offset - columns->MinX);

    if (preserve_width)
        SetColumnOffset(column_index + 1, offset + ImMax(g.Style.ColumnsMinSpacing, width));
}

void ImGui::SetColumnWidth(int column_index, float width)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    SetColumnOffset(column_index + 1, GetColumnOffset(column_index) + width);
}

void ImGui::PushColumnClipRect(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    if (column_index < 0)
        column_index = columns->Current;

    PushClipRect(columns->Columns[column_index].ClipRect.Min, columns->Columns[column_index].ClipRect.Max, false);
}

static ImGuiColumnsSet* FindOrAddColumnsSet(ImGuiWindow* window, ImGuiID id)
{
    for (int n = 0; n < window->ColumnsStorage.Size; n++)
        if (window->ColumnsStorage[n].ID == id)
            return &window->ColumnsStorage[n];

    window->ColumnsStorage.push_back(ImGuiColumnsSet());
    ImGuiColumnsSet* columns = &window->ColumnsStorage.back();
    columns->ID = id;
    return columns;
}

void ImGui::BeginColumns(const char* str_id, int columns_count, ImGuiColumnsFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    IM_ASSERT(columns_count > 1);
    IM_ASSERT(window->DC.ColumnsSet == NULL); // Nested columns are currently not supported

    // Differentiate column ID with an arbitrary prefix for cases where users name their columns set the same as another widget.
    // In addition, when an identifier isn't explicitly provided we include the number of columns in the hash to make it uniquer.
    PushID(0x11223347 + (str_id ? 0 : columns_count));
    ImGuiID id = window->GetID(str_id ? str_id : "columns");
    PopID();

    // Acquire storage for the columns set
    ImGuiColumnsSet* columns = FindOrAddColumnsSet(window, id);
    IM_ASSERT(columns->ID == id);
    columns->Current = 0;
    columns->Count = columns_count;
    columns->Flags = flags;
    window->DC.ColumnsSet = columns;

    // Set state for first column
    const float content_region_width = (window->SizeContentsExplicit.x != 0.0f) ? (window->SizeContentsExplicit.x) : (window->InnerClipRect.Max.x - window->Pos.x);
    columns->MinX = window->DC.Indent.x - g.Style.ItemSpacing.x; // Lock our horizontal range
    columns->MaxX = ImMax(content_region_width - window->Scroll.x, columns->MinX + 1.0f);
    columns->StartPosY = window->DC.CursorPos.y;
    columns->StartMaxPosX = window->DC.CursorMaxPos.x;
    columns->LineMinY = columns->LineMaxY = window->DC.CursorPos.y;
    window->DC.ColumnsOffset.x = 0.0f;
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);

    // Clear data if columns count changed
    if (columns->Columns.Size != 0 && columns->Columns.Size != columns_count + 1)
        columns->Columns.resize(0);

    // Initialize defaults
    columns->IsFirstFrame = (columns->Columns.Size == 0);
    if (columns->Columns.Size == 0)
    {
        columns->Columns.reserve(columns_count + 1);
        for (int n = 0; n < columns_count + 1; n++)
        {
            ImGuiColumnData column;
            column.OffsetNorm = n / (float)columns_count;
            columns->Columns.push_back(column);
        }
    }

    for (int n = 0; n < columns_count; n++)
    {
        // Compute clipping rectangle
        ImGuiColumnData* column = &columns->Columns[n];
        float clip_x1 = ImFloor(0.5f + window->Pos.x + GetColumnOffset(n) - 1.0f);
        float clip_x2 = ImFloor(0.5f + window->Pos.x + GetColumnOffset(n + 1) - 1.0f);
        column->ClipRect = ImRect(clip_x1, -FLT_MAX, clip_x2, +FLT_MAX);
        column->ClipRect.ClipWith(window->ClipRect);
    }

    window->DrawList->ChannelsSplit(columns->Count);
    PushColumnClipRect();
    PushItemWidth(GetColumnWidth() * 0.65f);
}

void ImGui::EndColumns()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    PopItemWidth();
    PopClipRect();
    window->DrawList->ChannelsMerge();

    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    window->DC.CursorPos.y = columns->LineMaxY;
    if (!(columns->Flags & ImGuiColumnsFlags_GrowParentContentsSize))
        window->DC.CursorMaxPos.x = columns->StartMaxPosX;  // Restore cursor max pos, as columns don't grow parent

    // Draw columns borders and handle resize
    bool is_being_resized = false;
    if (!(columns->Flags & ImGuiColumnsFlags_NoBorder) && !window->SkipItems)
    {
        const float y1 = columns->StartPosY;
        const float y2 = window->DC.CursorPos.y;
        int dragging_column = -1;
        for (int n = 1; n < columns->Count; n++)
        {
            float x = window->Pos.x + GetColumnOffset(n);
            const ImGuiID column_id = columns->ID + ImGuiID(n);
            const float column_hw = GetColumnsRectHalfWidth(); // Half-width for interaction
            const ImRect column_rect(ImVec2(x - column_hw, y1), ImVec2(x + column_hw, y2));
            KeepAliveID(column_id);
            if (IsClippedEx(column_rect, column_id, false))
                continue;

            bool hovered = false, held = false;
            if (!(columns->Flags & ImGuiColumnsFlags_NoResize))
            {
                ButtonBehavior(column_rect, column_id, &hovered, &held);
                if (hovered || held)
                    g.MouseCursor = ImGuiMouseCursor_ResizeEW;
                if (held && !(columns->Columns[n].Flags & ImGuiColumnsFlags_NoResize))
                    dragging_column = n;
            }

            // Draw column (we clip the Y boundaries CPU side because very long triangles are mishandled by some GPU drivers.)
            const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
            const float xi = (float)(int)x;
            window->DrawList->AddLine(ImVec2(xi, ImMax(y1 + 1.0f, window->ClipRect.Min.y)), ImVec2(xi, ImMin(y2, window->ClipRect.Max.y)), col);
        }

        // Apply dragging after drawing the column lines, so our rendered lines are in sync with how items were displayed during the frame.
        if (dragging_column != -1)
        {
            if (!columns->IsBeingResized)
                for (int n = 0; n < columns->Count + 1; n++)
                    columns->Columns[n].OffsetNormBeforeResize = columns->Columns[n].OffsetNorm;
            columns->IsBeingResized = is_being_resized = true;
            float x = GetDraggedColumnOffset(columns, dragging_column);
            SetColumnOffset(dragging_column, x);
        }
    }
    columns->IsBeingResized = is_being_resized;

    window->DC.ColumnsSet = NULL;
    window->DC.ColumnsOffset.x = 0.0f;
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
}

// [2018-03: This is currently the only public API, while we are working on making BeginColumns/EndColumns user-facing]
void ImGui::Columns(int columns_count, const char* id, bool border)
{
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(columns_count >= 1);

    ImGuiColumnsFlags flags = (border ? 0 : ImGuiColumnsFlags_NoBorder);
    //flags |= ImGuiColumnsFlags_NoPreserveWidths; // NB: Legacy behavior
    if (window->DC.ColumnsSet != NULL && window->DC.ColumnsSet->Count == columns_count && window->DC.ColumnsSet->Flags == flags)
        return;

    if (window->DC.ColumnsSet != NULL)
        EndColumns();

    if (columns_count != 1)
        BeginColumns(id, columns_count, flags);
}

