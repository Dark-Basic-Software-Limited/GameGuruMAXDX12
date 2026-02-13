//-----------------------------------------------------------------------------
// [SECTION] DRAG AND DROP
//-----------------------------------------------------------------------------

void ImGui::ClearDragDrop()
{
    ImGuiContext& g = *GImGui;
    g.DragDropActive = false;
    g.DragDropPayload.Clear();
    g.DragDropAcceptFlags = 0;
    g.DragDropAcceptIdCurr = g.DragDropAcceptIdPrev = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropAcceptFrameCount = -1;

    g.DragDropPayloadBufHeap.clear();
    memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
}

// Call when current ID is active.
// When this returns true you need to: a) call SetDragDropPayload() exactly once, b) you may render the payload visual/description, c) call EndDragDropSource()
bool ImGui::BeginDragDropSource(ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    bool source_drag_active = false;
    ImGuiID source_id = 0;
    ImGuiID source_parent_id = 0;
    int mouse_button = 0;
    if (!(flags & ImGuiDragDropFlags_SourceExtern))
    {
        source_id = window->DC.LastItemId;
        if (source_id != 0 && g.ActiveId != source_id) // Early out for most common case
            return false;
        if (g.IO.MouseDown[mouse_button] == false)
            return false;

        if (source_id == 0)
        {
            // If you want to use BeginDragDropSource() on an item with no unique identifier for interaction, such as Text() or Image(), you need to:
            // A) Read the explanation below, B) Use the ImGuiDragDropFlags_SourceAllowNullID flag, C) Swallow your programmer pride.
            if (!(flags & ImGuiDragDropFlags_SourceAllowNullID))
            {
                IM_ASSERT(0);
                return false;
            }

            // Magic fallback (=somehow reprehensible) to handle items with no assigned ID, e.g. Text(), Image()
            // We build a throwaway ID based on current ID stack + relative AABB of items in window.
            // THE IDENTIFIER WON'T SURVIVE ANY REPOSITIONING OF THE WIDGET, so if your widget moves your dragging operation will be canceled.
            // We don't need to maintain/call ClearActiveID() as releasing the button will early out this function and trigger !ActiveIdIsAlive.
            bool is_hovered = (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) != 0;
            if (!is_hovered && (g.ActiveId == 0 || g.ActiveIdWindow != window))
                return false;
            source_id = window->DC.LastItemId = window->GetIDFromRectangle(window->DC.LastItemRect);
            if (is_hovered)
                SetHoveredID(source_id);
            if (is_hovered && g.IO.MouseClicked[mouse_button])
            {
                SetActiveID(source_id, window);
                FocusWindow(window);
            }
            if (g.ActiveId == source_id) // Allow the underlying widget to display/return hovered during the mouse release frame, else we would get a flicker.
                g.ActiveIdAllowOverlap = is_hovered;
        }
        else
        {
            g.ActiveIdAllowOverlap = false;
        }
        if (g.ActiveId != source_id)
            return false;
        source_parent_id = window->IDStack.back();
        source_drag_active = IsMouseDragging(mouse_button);
    }
    else
    {
        window = NULL;
        source_id = ImHash("#SourceExtern", 0);
        source_drag_active = true;
    }

    if (source_drag_active)
    {
        if (!g.DragDropActive)
        {
            IM_ASSERT(source_id != 0);
            ClearDragDrop();
            ImGuiPayload& payload = g.DragDropPayload;
            payload.SourceId = source_id;
            payload.SourceParentId = source_parent_id;
            g.DragDropActive = true;
            g.DragDropSourceFlags = flags;
            g.DragDropMouseButton = mouse_button;
        }
        g.DragDropSourceFrameCount = g.FrameCount;
        g.DragDropWithinSourceOrTarget = true;

        if (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        {
            // Target can request the Source to not display its tooltip (we use a dedicated flag to make this request explicit)
            // We unfortunately can't just modify the source flags and skip the call to BeginTooltip, as caller may be emitting contents. 
            BeginTooltip();
            if (g.DragDropActive && g.DragDropAcceptIdPrev && (g.DragDropAcceptFlags & ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                ImGuiWindow* tooltip_window = g.CurrentWindow;
                tooltip_window->SkipItems = true;
                tooltip_window->HiddenFramesRegular = 1;
            }
        }

        if (!(flags & ImGuiDragDropFlags_SourceNoDisableHover) && !(flags & ImGuiDragDropFlags_SourceExtern))
            window->DC.LastItemStatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;

        return true;
    }
    return false;
}

void ImGui::EndDragDropSource()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinSourceOrTarget && "Not after a BeginDragDropSource()?");

    if (!(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        EndTooltip();

    // Discard the drag if have not called SetDragDropPayload()
    if (g.DragDropPayload.DataFrameCount == -1)
        ClearDragDrop();
    g.DragDropWithinSourceOrTarget = false;
}

// Use 'cond' to choose to submit payload on drag start or every frame
bool ImGui::SetDragDropPayload(const char* type, const void* data, size_t data_size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    ImGuiPayload& payload = g.DragDropPayload;
    if (cond == 0)
        cond = ImGuiCond_Always;

    IM_ASSERT(type != NULL);
    IM_ASSERT(strlen(type) < IM_ARRAYSIZE(payload.DataType) && "Payload type can be at most 32 characters long");
    IM_ASSERT((data != NULL && data_size > 0) || (data == NULL && data_size == 0));
    IM_ASSERT(cond == ImGuiCond_Always || cond == ImGuiCond_Once);
    IM_ASSERT(payload.SourceId != 0);                               // Not called between BeginDragDropSource() and EndDragDropSource()

    if (cond == ImGuiCond_Always || payload.DataFrameCount == -1)
    {
        // Copy payload
        ImStrncpy(payload.DataType, type, IM_ARRAYSIZE(payload.DataType));
        g.DragDropPayloadBufHeap.resize(0);
        if (data_size > sizeof(g.DragDropPayloadBufLocal))
        {
            // Store in heap
            g.DragDropPayloadBufHeap.resize((int)data_size);
            payload.Data = g.DragDropPayloadBufHeap.Data;
            memcpy(payload.Data, data, data_size);
        }
        else if (data_size > 0)
        {
            // Store locally
            memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
            payload.Data = g.DragDropPayloadBufLocal;
            memcpy(payload.Data, data, data_size);
        }
        else
        {
            payload.Data = NULL;
        }
        payload.DataSize = (int)data_size;
    }
    payload.DataFrameCount = g.FrameCount;

    return (g.DragDropAcceptFrameCount == g.FrameCount) || (g.DragDropAcceptFrameCount == g.FrameCount - 1);
}

bool ImGui::BeginDragDropTargetCustom(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindow == NULL || window->RootWindow != g.HoveredWindow->RootWindow)
        return false;
    IM_ASSERT(id != 0);
    if (!IsMouseHoveringRect(bb.Min, bb.Max) || (id == g.DragDropPayload.SourceId))
        return false;
    if (window->SkipItems)
        return false;

    IM_ASSERT(g.DragDropWithinSourceOrTarget == false);
    g.DragDropTargetRect = bb;
    g.DragDropTargetId = id;
    g.DragDropWithinSourceOrTarget = true;
    return true;
}

// We don't use BeginDragDropTargetCustom() and duplicate its code because:
// 1) we use LastItemRectHoveredRect which handles items that pushes a temporarily clip rectangle in their code. Calling BeginDragDropTargetCustom(LastItemRect) would not handle them.
// 2) and it's faster. as this code may be very frequently called, we want to early out as fast as we can.
// Also note how the HoveredWindow test is positioned differently in both functions (in both functions we optimize for the cheapest early out case)
bool ImGui::BeginDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (!(window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    if (g.HoveredWindow == NULL || window->RootWindow != g.HoveredWindow->RootWindow)
        return false;

    const ImRect& display_rect = (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? window->DC.LastItemDisplayRect : window->DC.LastItemRect;
    ImGuiID id = window->DC.LastItemId;
    if (id == 0)
        id = window->GetIDFromRectangle(display_rect);
    if (g.DragDropPayload.SourceId == id)
        return false;

    IM_ASSERT(g.DragDropWithinSourceOrTarget == false);
    g.DragDropTargetRect = display_rect;
    g.DragDropTargetId = id;
    g.DragDropWithinSourceOrTarget = true;
    return true;
}

bool ImGui::IsDragDropPayloadBeingAccepted()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive && g.DragDropAcceptIdPrev != 0;
}

const ImGuiPayload* ImGui::AcceptDragDropPayload(const char* type, ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiPayload& payload = g.DragDropPayload;
    IM_ASSERT(g.DragDropActive);                        // Not called between BeginDragDropTarget() and EndDragDropTarget() ?
    IM_ASSERT(payload.DataFrameCount != -1);            // Forgot to call EndDragDropTarget() ?
    if (type != NULL && !payload.IsDataType(type))
        return NULL;

    // Accept smallest drag target bounding box, this allows us to nest drag targets conveniently without ordering constraints.
    // NB: We currently accept NULL id as target. However, overlapping targets requires a unique ID to function!
    const bool was_accepted_previously = (g.DragDropAcceptIdPrev == g.DragDropTargetId);
    ImRect r = g.DragDropTargetRect;
    float r_surface = r.GetWidth() * r.GetHeight();
    if (r_surface < g.DragDropAcceptIdCurrRectSurface)
    {
        g.DragDropAcceptFlags = flags;
        g.DragDropAcceptIdCurr = g.DragDropTargetId;
        g.DragDropAcceptIdCurrRectSurface = r_surface;
    }

    // Render default drop visuals
    payload.Preview = was_accepted_previously;
    flags |= (g.DragDropSourceFlags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect); // Source can also inhibit the preview (useful for external sources that lives for 1 frame)
    if (!(flags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect) && payload.Preview)
    {
        // FIXME-DRAG: Settle on a proper default visuals for drop target.
        r.Expand(3.5f);
        bool push_clip_rect = !window->ClipRect.Contains(r);
        if (push_clip_rect) window->DrawList->PushClipRect(r.Min-ImVec2(1,1), r.Max+ImVec2(1,1));
        window->DrawList->AddRect(r.Min, r.Max, GetColorU32(ImGuiCol_DragDropTarget), 0.0f, ~0, 2.0f);
        if (push_clip_rect) window->DrawList->PopClipRect();
    }

    g.DragDropAcceptFrameCount = g.FrameCount;
    payload.Delivery = was_accepted_previously && !IsMouseDown(g.DragDropMouseButton); // For extern drag sources affecting os window focus, it's easier to just test !IsMouseDown() instead of IsMouseReleased()
    if (!payload.Delivery && !(flags & ImGuiDragDropFlags_AcceptBeforeDelivery))
        return NULL;

    return &payload;
}

// We don't really use/need this now, but added it for the sake of consistency and because we might need it later.
void ImGui::EndDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinSourceOrTarget);
    g.DragDropWithinSourceOrTarget = false;
}

//-----------------------------------------------------------------------------
// [SECTION] LOGGING/CAPTURING
//-----------------------------------------------------------------------------

// Pass text data straight to log (without being displayed)
void ImGui::LogText(const char* fmt, ...)
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    va_list args;
    va_start(args, fmt);
    if (g.LogFile)
        vfprintf(g.LogFile, fmt, args);
    else
        g.LogClipboard.appendfv(fmt, args);
    va_end(args);
}

// Internal version that takes a position to decide on newline placement and pad items according to their depth.
// We split text into individual lines to add current tree level padding
void ImGui::LogRenderedText(const ImVec2* ref_pos, const char* text, const char* text_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = FindRenderedTextEnd(text, text_end);

    const bool log_new_line = ref_pos && (ref_pos->y > window->DC.LogLinePosY + 1);
    if (ref_pos)
        window->DC.LogLinePosY = ref_pos->y;

    const char* text_remaining = text;
    if (g.LogStartDepth > window->DC.TreeDepth)  // Re-adjust padding if we have popped out of our starting depth
        g.LogStartDepth = window->DC.TreeDepth;
    const int tree_depth = (window->DC.TreeDepth - g.LogStartDepth);
    for (;;)
    {
        // Split the string. Each new line (after a '\n') is followed by spacing corresponding to the current depth of our log entry.
        const char* line_end = text_remaining;
        while (line_end < text_end)
            if (*line_end == '\n')
                break;
            else
                line_end++;
        if (line_end >= text_end)
            line_end = NULL;

        const bool is_first_line = (text == text_remaining);
        bool is_last_line = false;
        if (line_end == NULL)
        {
            is_last_line = true;
            line_end = text_end;
        }
        if (line_end != NULL && !(is_last_line && (line_end - text_remaining)==0))
        {
            const int char_count = (int)(line_end - text_remaining);
            if (log_new_line || !is_first_line)
                LogText(IM_NEWLINE "%*s%.*s", tree_depth*4, "", char_count, text_remaining);
            else
                LogText(" %.*s", char_count, text_remaining);
        }

        if (is_last_line)
            break;
        text_remaining = line_end + 1;
    }
}

// Start logging ImGui output to TTY
void ImGui::LogToTTY(int max_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(g.LogFile == NULL);
    g.LogFile = stdout;
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to given file
void ImGui::LogToFile(int max_depth, const char* filename)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    ImGuiWindow* window = g.CurrentWindow;

    if (!filename)
    {
        filename = g.IO.LogFilename;
        if (!filename)
            return;
    }

    IM_ASSERT(g.LogFile == NULL);
    g.LogFile = ImFileOpen(filename, "ab");
    if (!g.LogFile)
    {
        IM_ASSERT(g.LogFile != NULL); // Consider this an error
        return;
    }
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to clipboard
void ImGui::LogToClipboard(int max_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(g.LogFile == NULL);
    g.LogFile = NULL;
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

void ImGui::LogFinish()
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    LogText(IM_NEWLINE);
    if (g.LogFile != NULL)
    {
        if (g.LogFile == stdout)
            fflush(g.LogFile);
        else
            fclose(g.LogFile);
        g.LogFile = NULL;
    }
    if (g.LogClipboard.size() > 1)
    {
        SetClipboardText(g.LogClipboard.begin());
        g.LogClipboard.clear();
    }
    g.LogEnabled = false;
}

// Helper to display logging buttons
void ImGui::LogButtons()
{
    ImGuiContext& g = *GImGui;

    PushID("LogButtons");
    const bool log_to_tty = Button("Log To TTY"); SameLine();
    const bool log_to_file = Button("Log To File"); SameLine();
    const bool log_to_clipboard = Button("Log To Clipboard"); SameLine();
    PushItemWidth(80.0f);
    PushAllowKeyboardFocus(false);
    SliderInt("Depth", &g.LogAutoExpandMaxDepth, 0, 9, NULL);
    PopAllowKeyboardFocus();
    PopItemWidth();
    PopID();

    // Start logging at the end of the function so that the buttons don't appear in the log
    if (log_to_tty)
        LogToTTY(g.LogAutoExpandMaxDepth);
    if (log_to_file)
        LogToFile(g.LogAutoExpandMaxDepth, g.IO.LogFilename);
    if (log_to_clipboard)
        LogToClipboard(g.LogAutoExpandMaxDepth);
}

//-----------------------------------------------------------------------------
// [SECTION] SETTINGS
//-----------------------------------------------------------------------------

void ImGui::MarkIniSettingsDirty()
{
    ImGuiContext& g = *GImGui;
    if (g.SettingsDirtyTimer <= 0.0f)
        g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

void ImGui::MarkIniSettingsDirty(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
        if (g.SettingsDirtyTimer <= 0.0f)
            g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

static ImGuiWindowSettings* CreateNewWindowSettings(const char* name)
{
    ImGuiContext& g = *GImGui;
    g.SettingsWindows.push_back(ImGuiWindowSettings());
    ImGuiWindowSettings* settings = &g.SettingsWindows.back();
    settings->Name = ImStrdup(name);
    settings->ID = ImHash(name, 0);
    return settings;
}

ImGuiWindowSettings* ImGui::FindWindowSettings(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (int i = 0; i != g.SettingsWindows.Size; i++)
        if (g.SettingsWindows[i].ID == id)
            return &g.SettingsWindows[i];
    return NULL;
}

void ImGui::LoadIniSettingsFromDisk(const char* ini_filename)
{
    size_t file_data_size = 0;
    char* file_data = (char*)ImFileLoadToMemory(ini_filename, "rb", &file_data_size);
    if (!file_data)
        return;
    LoadIniSettingsFromMemory(file_data, (size_t)file_data_size);
    ImGui::MemFree(file_data);
}

ImGuiSettingsHandler* ImGui::FindSettingsHandler(const char* type_name)
{
    ImGuiContext& g = *GImGui;
    const ImGuiID type_hash = ImHash(type_name, 0, 0);
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
        if (g.SettingsHandlers[handler_n].TypeHash == type_hash)
            return &g.SettingsHandlers[handler_n];
    return NULL;
}

// Zero-tolerance, no error reporting, cheap .ini parsing
void ImGui::LoadIniSettingsFromMemory(const char* ini_data, size_t ini_size)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);
    IM_ASSERT(g.SettingsLoaded == false && g.FrameCount == 0);

    // For user convenience, we allow passing a non zero-terminated string (hence the ini_size parameter).
    // For our convenience and to make the code simpler, we'll also write zero-terminators within the buffer. So let's create a writable copy..
    if (ini_size == 0)
        ini_size = strlen(ini_data);
    char* buf = (char*)ImGui::MemAlloc(ini_size + 1);
    char* buf_end = buf + ini_size;
    memcpy(buf, ini_data, ini_size);
    buf[ini_size] = 0;

    void* entry_data = NULL;
    ImGuiSettingsHandler* entry_handler = NULL;

    char* line_end = NULL;
    for (char* line = buf; line < buf_end; line = line_end + 1)
    {
        // Skip new lines markers, then find end of the line
        while (*line == '\n' || *line == '\r')
            line++;
        line_end = line;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
            line_end++;
        line_end[0] = 0;
        if (line[0] == ';')
            continue;
        if (line[0] == '[' && line_end > line && line_end[-1] == ']')
        {
            // Parse "[Type][Name]". Note that 'Name' can itself contains [] characters, which is acceptable with the current format and parsing code.
            line_end[-1] = 0;
            const char* name_end = line_end - 1;
            const char* type_start = line + 1;
            char* type_end = (char*)(intptr_t)ImStrchrRange(type_start, name_end, ']');
            const char* name_start = type_end ? ImStrchrRange(type_end + 1, name_end, '[') : NULL;
            if (!type_end || !name_start)
            {
                name_start = type_start; // Import legacy entries that have no type
                type_start = "Window";
            }
            else
            {
                *type_end = 0; // Overwrite first ']'
                name_start++;  // Skip second '['
            }
            entry_handler = FindSettingsHandler(type_start);
            entry_data = entry_handler ? entry_handler->ReadOpenFn(&g, entry_handler, name_start) : NULL;
        }
        else if (entry_handler != NULL && entry_data != NULL)
        {
            // Let type handler parse the line
            entry_handler->ReadLineFn(&g, entry_handler, entry_data, line);
        }
    }
    ImGui::MemFree(buf);
    g.SettingsLoaded = true;
}

void ImGui::SaveIniSettingsToDisk(const char* ini_filename)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    if (!ini_filename)
        return;

    size_t ini_data_size = 0;
    const char* ini_data = SaveIniSettingsToMemory(&ini_data_size);
    FILE* f = ImFileOpen(ini_filename, "wt");
    if (!f)
        return;
    fwrite(ini_data, sizeof(char), ini_data_size, f);
    fclose(f);
}

// Call registered handlers (e.g. SettingsHandlerWindow_WriteAll() + custom handlers) to write their stuff into a text buffer
const char* ImGui::SaveIniSettingsToMemory(size_t* out_size)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    g.SettingsIniData.Buf.resize(0);
    g.SettingsIniData.Buf.push_back(0);
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
    {
        ImGuiSettingsHandler* handler = &g.SettingsHandlers[handler_n];
        handler->WriteAllFn(&g, handler, &g.SettingsIniData);
    }
    if (out_size)
        *out_size = (size_t)g.SettingsIniData.size();
    return g.SettingsIniData.c_str();
}

static void* SettingsHandlerWindow_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    ImGuiWindowSettings* settings = ImGui::FindWindowSettings(ImHash(name, 0));
    if (!settings)
        settings = CreateNewWindowSettings(name);
    return (void*)settings;
}

static void SettingsHandlerWindow_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiWindowSettings* settings = (ImGuiWindowSettings*)entry;
    float x, y;
    int i;
    if (sscanf(line, "Pos=%f,%f", &x, &y) == 2)         settings->Pos = ImVec2(x, y);
    else if (sscanf(line, "Size=%f,%f", &x, &y) == 2)   settings->Size = ImMax(ImVec2(x, y), GImGui->Style.WindowMinSize);
    else if (sscanf(line, "Collapsed=%d", &i) == 1)     settings->Collapsed = (i != 0);
}

static void SettingsHandlerWindow_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    // Gather data from windows that were active during this session
    // (if a window wasn't opened in this session we preserve its settings)
    ImGuiContext& g = *imgui_ctx;
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
            continue;

        ImGuiWindowSettings* settings = (window->SettingsIdx != -1) ? &g.SettingsWindows[window->SettingsIdx] : ImGui::FindWindowSettings(window->ID);
        if (!settings)
        {
            settings = CreateNewWindowSettings(window->Name);
            window->SettingsIdx = g.SettingsWindows.index_from_pointer(settings);
        }
        IM_ASSERT(settings->ID == window->ID);
        settings->Pos = window->Pos;
        settings->Size = window->SizeFull;
        settings->Collapsed = window->Collapsed;
    }

    // Write to text buffer
    buf->reserve(buf->size() + g.SettingsWindows.Size * 96); // ballpark reserve
    for (int i = 0; i != g.SettingsWindows.Size; i++)
    {
        const ImGuiWindowSettings* settings = &g.SettingsWindows[i];
        if (settings->Pos.x == FLT_MAX)
            continue;
        const char* name = settings->Name;
        if (const char* p = strstr(name, "###"))  // Skip to the "###" marker if any. We don't skip past to match the behavior of GetID()
            name = p;
        buf->appendf("[%s][%s]\n", handler->TypeName, name);
        buf->appendf("Pos=%d,%d\n", (int)settings->Pos.x, (int)settings->Pos.y);
        buf->appendf("Size=%d,%d\n", (int)settings->Size.x, (int)settings->Size.y);
        buf->appendf("Collapsed=%d\n", settings->Collapsed);
        buf->appendf("\n");
    }
}

//-----------------------------------------------------------------------------
// [SECTION] PLATFORM DEPENDENT HELPERS
//-----------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WINDOWS_) && (!defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) || !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef __MINGW32__
#include <Windows.h>
#else
#include <windows.h>
#endif
#endif

// Win32 API clipboard implementation
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)

#ifdef _MSC_VER
#pragma comment(lib, "user32")
#endif

static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    static ImVector<char> buf_local;
    buf_local.clear();
    if (!::OpenClipboard(NULL))
        return NULL;
    HANDLE wbuf_handle = ::GetClipboardData(CF_UNICODETEXT);
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return NULL;
    }
    if (ImWchar* wbuf_global = (ImWchar*)::GlobalLock(wbuf_handle))
    {
        int buf_len = ImTextCountUtf8BytesFromStr(wbuf_global, NULL) + 1;
        buf_local.resize(buf_len);
        ImTextStrToUtf8(buf_local.Data, buf_len, wbuf_global, NULL);
    }
    ::GlobalUnlock(wbuf_handle);
    ::CloseClipboard();
    return buf_local.Data;
}

static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    if (!::OpenClipboard(NULL))
        return;
    const int wbuf_length = ImTextCountCharsFromUtf8(text, NULL) + 1;
    HGLOBAL wbuf_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(ImWchar));
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return;
    }
    ImWchar* wbuf_global = (ImWchar*)::GlobalLock(wbuf_handle);
    ImTextStrFromUtf8(wbuf_global, wbuf_length, text, NULL);
    ::GlobalUnlock(wbuf_handle);
    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
        ::GlobalFree(wbuf_handle);
    ::CloseClipboard();
}

#else

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    ImGuiContext& g = *GImGui;
    return g.PrivateClipboard.empty() ? NULL : g.PrivateClipboard.begin();
}

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    ImGuiContext& g = *GImGui;
    g.PrivateClipboard.clear();
    const char* text_end = text + strlen(text);
    g.PrivateClipboard.resize((int)(text_end - text) + 1);
    memcpy(&g.PrivateClipboard[0], text, (size_t)(text_end - text));
    g.PrivateClipboard[(int)(text_end - text)] = 0;
}

#endif

// Win32 API IME support (for Asian languages, etc.)
#if defined(_WIN32) && !defined(__GNUC__) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)

#include <imm.h>
#ifdef _MSC_VER
#pragma comment(lib, "imm32")
#endif

static void ImeSetInputScreenPosFn_DefaultImpl(int x, int y)
{
    // Notify OS Input Method Editor of text input position
    if (HWND hwnd = (HWND)GImGui->IO.ImeWindowHandle)
        if (HIMC himc = ::ImmGetContext(hwnd))
        {
            COMPOSITIONFORM cf;
            cf.ptCurrentPos.x = x;
            cf.ptCurrentPos.y = y;
            cf.dwStyle = CFS_FORCE_POSITION;
            ::ImmSetCompositionWindow(himc, &cf);
            ::ImmReleaseContext(hwnd, himc);
        }
}

#else

static void ImeSetInputScreenPosFn_DefaultImpl(int, int) {}

#endif

//-----------------------------------------------------------------------------
// [SECTION] METRICS/DEBUG WINDOW
//-----------------------------------------------------------------------------

void ImGui::ShowMetricsWindow(bool* p_open)
{
    if (!ImGui::Begin("ImGui Metrics", p_open))
    {
        ImGui::End();
        return;
    }
    static bool show_draw_cmd_clip_rects = true;
    static bool show_window_begin_order = false;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
    ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
    ImGui::Text("%d allocations", io.MetricsActiveAllocations);
    ImGui::Checkbox("Show clipping rectangles when hovering draw commands", &show_draw_cmd_clip_rects);
    ImGui::Checkbox("Ctrl shows window begin order", &show_window_begin_order);

    ImGui::Separator();

    struct Funcs
    {
        static void NodeDrawList(ImGuiWindow* window, ImDrawList* draw_list, const char* label)
        {
            bool node_open = ImGui::TreeNode(draw_list, "%s: '%s' %d vtx, %d indices, %d cmds", label, draw_list->_OwnerName ? draw_list->_OwnerName : "", draw_list->VtxBuffer.Size, draw_list->IdxBuffer.Size, draw_list->CmdBuffer.Size);
            if (draw_list == ImGui::GetWindowDrawList())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImColor(255,100,100), "CURRENTLY APPENDING"); // Can't display stats for active draw list! (we don't have the data double-buffered)
                if (node_open) ImGui::TreePop();
                return;
            }

            ImDrawList* overlay_draw_list = GetOverlayDrawList(); // Render additional visuals into the top-most draw list
            if (window && IsItemHovered())
                overlay_draw_list->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
            if (!node_open)
                return;

            int elem_offset = 0;
            for (const ImDrawCmd* pcmd = draw_list->CmdBuffer.begin(); pcmd < draw_list->CmdBuffer.end(); elem_offset += pcmd->ElemCount, pcmd++)
            {
                if (pcmd->UserCallback == NULL && pcmd->ElemCount == 0)
                    continue;
                if (pcmd->UserCallback)
                {
                    ImGui::BulletText("Callback %p, user_data %p", pcmd->UserCallback, pcmd->UserCallbackData);
                    continue;
                }
                ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
                bool pcmd_node_open = ImGui::TreeNode((void*)(pcmd - draw_list->CmdBuffer.begin()), "Draw %4d %s vtx, tex 0x%p, clip_rect (%4.0f,%4.0f)-(%4.0f,%4.0f)", pcmd->ElemCount, draw_list->IdxBuffer.Size > 0 ? "indexed" : "non-indexed", pcmd->TextureId, pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
                if (show_draw_cmd_clip_rects && ImGui::IsItemHovered())
                {
                    ImRect clip_rect = pcmd->ClipRect;
                    ImRect vtxs_rect;
                    for (int i = elem_offset; i < elem_offset + (int)pcmd->ElemCount; i++)
                        vtxs_rect.Add(draw_list->VtxBuffer[idx_buffer ? idx_buffer[i] : i].pos);
                    clip_rect.Floor(); overlay_draw_list->AddRect(clip_rect.Min, clip_rect.Max, IM_COL32(255,255,0,255));
                    vtxs_rect.Floor(); overlay_draw_list->AddRect(vtxs_rect.Min, vtxs_rect.Max, IM_COL32(255,0,255,255));
                }
                if (!pcmd_node_open)
                    continue;

                // Display individual triangles/vertices. Hover on to get the corresponding triangle highlighted.
                ImGuiListClipper clipper(pcmd->ElemCount/3); // Manually coarse clip our print out of individual vertices to save CPU, only items that may be visible.
                while (clipper.Step())
                    for (int prim = clipper.DisplayStart, vtx_i = elem_offset + clipper.DisplayStart*3; prim < clipper.DisplayEnd; prim++)
                    {
                        char buf[300];
                        char *buf_p = buf, *buf_end = buf + IM_ARRAYSIZE(buf);
                        ImVec2 triangles_pos[3];
                        for (int n = 0; n < 3; n++, vtx_i++)
                        {
                            ImDrawVert& v = draw_list->VtxBuffer[idx_buffer ? idx_buffer[vtx_i] : vtx_i];
                            triangles_pos[n] = v.pos;
                            buf_p += ImFormatString(buf_p, (int)(buf_end - buf_p), "%s %04d: pos (%8.2f,%8.2f), uv (%.6f,%.6f), col %08X\n", (n == 0) ? "vtx" : "   ", vtx_i, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.col);
                        }
                        ImGui::Selectable(buf, false);
                        if (ImGui::IsItemHovered())
                        {
                            ImDrawListFlags backup_flags = overlay_draw_list->Flags;
                            overlay_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines at is more readable for very large and thin triangles.
                            overlay_draw_list->AddPolyline(triangles_pos, 3, IM_COL32(255,255,0,255), true, 1.0f);
                            overlay_draw_list->Flags = backup_flags;
                        }
                    }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        static void NodeWindows(ImVector<ImGuiWindow*>& windows, const char* label)
        {
            if (!ImGui::TreeNode(label, "%s (%d)", label, windows.Size))
                return;
            for (int i = 0; i < windows.Size; i++)
                Funcs::NodeWindow(windows[i], "Window");
            ImGui::TreePop();
        }

        static void NodeWindow(ImGuiWindow* window, const char* label)
        {
            if (!ImGui::TreeNode(window, "%s '%s', %d @ 0x%p", label, window->Name, window->Active || window->WasActive, window))
                return;
            ImGuiWindowFlags flags = window->Flags;
            NodeDrawList(window, window->DrawList, "DrawList");
            ImGui::BulletText("Pos: (%.1f,%.1f), Size: (%.1f,%.1f), SizeContents (%.1f,%.1f)", window->Pos.x, window->Pos.y, window->Size.x, window->Size.y, window->SizeContents.x, window->SizeContents.y);
            ImGui::BulletText("Flags: 0x%08X (%s%s%s%s%s%s%s%s..)", flags,
                (flags & ImGuiWindowFlags_ChildWindow) ? "Child " : "", (flags & ImGuiWindowFlags_Tooltip)   ? "Tooltip "   : "", (flags & ImGuiWindowFlags_Popup) ? "Popup " : "",
                (flags & ImGuiWindowFlags_Modal)       ? "Modal " : "", (flags & ImGuiWindowFlags_ChildMenu) ? "ChildMenu " : "", (flags & ImGuiWindowFlags_NoSavedSettings) ? "NoSavedSettings " : "",
                (flags & ImGuiWindowFlags_NoInputs)    ? "NoInputs":"", (flags & ImGuiWindowFlags_AlwaysAutoResize) ? "AlwaysAutoResize" : "");
            ImGui::BulletText("Scroll: (%.2f/%.2f,%.2f/%.2f)", window->Scroll.x, GetScrollMaxX(window), window->Scroll.y, GetScrollMaxY(window));
            ImGui::BulletText("Active: %d/%d, WriteAccessed: %d, BeginOrderWithinContext: %d", window->Active, window->WasActive, window->WriteAccessed, (window->Active || window->WasActive) ? window->BeginOrderWithinContext : -1);
            ImGui::BulletText("Appearing: %d, Hidden: %d (Reg %d Resize %d), SkipItems: %d", window->Appearing, window->Hidden, window->HiddenFramesRegular, window->HiddenFramesForResize, window->SkipItems);
            ImGui::BulletText("NavLastIds: 0x%08X,0x%08X, NavLayerActiveMask: %X", window->NavLastIds[0], window->NavLastIds[1], window->DC.NavLayerActiveMask);
            ImGui::BulletText("NavLastChildNavWindow: %s", window->NavLastChildNavWindow ? window->NavLastChildNavWindow->Name : "NULL");
            if (!window->NavRectRel[0].IsInverted())
                ImGui::BulletText("NavRectRel[0]: (%.1f,%.1f)(%.1f,%.1f)", window->NavRectRel[0].Min.x, window->NavRectRel[0].Min.y, window->NavRectRel[0].Max.x, window->NavRectRel[0].Max.y);
            else
                ImGui::BulletText("NavRectRel[0]: <None>");
            if (window->RootWindow != window) NodeWindow(window->RootWindow, "RootWindow");
            if (window->ParentWindow != NULL) NodeWindow(window->ParentWindow, "ParentWindow");
            if (window->DC.ChildWindows.Size > 0) NodeWindows(window->DC.ChildWindows, "ChildWindows");
            if (window->ColumnsStorage.Size > 0 && ImGui::TreeNode("Columns", "Columns sets (%d)", window->ColumnsStorage.Size))
            {
                for (int n = 0; n < window->ColumnsStorage.Size; n++)
                {
                    const ImGuiColumnsSet* columns = &window->ColumnsStorage[n];
                    if (ImGui::TreeNode((void*)(uintptr_t)columns->ID, "Columns Id: 0x%08X, Count: %d, Flags: 0x%04X", columns->ID, columns->Count, columns->Flags))
                    {
                        ImGui::BulletText("Width: %.1f (MinX: %.1f, MaxX: %.1f)", columns->MaxX - columns->MinX, columns->MinX, columns->MaxX);
                        for (int column_n = 0; column_n < columns->Columns.Size; column_n++)
                            ImGui::BulletText("Column %02d: OffsetNorm %.3f (= %.1f px)", column_n, columns->Columns[column_n].OffsetNorm, OffsetNormToPixels(columns, columns->Columns[column_n].OffsetNorm));
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
            ImGui::BulletText("Storage: %d bytes", window->StateStorage.Data.Size * (int)sizeof(ImGuiStorage::Pair));
            ImGui::TreePop();
        }
    };

    // Access private state, we are going to display the draw lists from last frame
    ImGuiContext& g = *GImGui;
    Funcs::NodeWindows(g.Windows, "Windows");
    if (ImGui::TreeNode("DrawList", "Active DrawLists (%d)", g.DrawDataBuilder.Layers[0].Size))
    {
        for (int i = 0; i < g.DrawDataBuilder.Layers[0].Size; i++)
            Funcs::NodeDrawList(NULL, g.DrawDataBuilder.Layers[0][i], "DrawList");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Popups", "Popups (%d)", g.OpenPopupStack.Size))
    {
        for (int i = 0; i < g.OpenPopupStack.Size; i++)
        {
            ImGuiWindow* window = g.OpenPopupStack[i].Window;
            ImGui::BulletText("PopupID: %08x, Window: '%s'%s%s", g.OpenPopupStack[i].PopupId, window ? window->Name : "NULL", window && (window->Flags & ImGuiWindowFlags_ChildWindow) ? " ChildWindow" : "", window && (window->Flags & ImGuiWindowFlags_ChildMenu) ? " ChildMenu" : "");
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Internal state"))
    {
        const char* input_source_names[] = { "None", "Mouse", "Nav", "NavKeyboard", "NavGamepad" }; IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT);
        ImGui::Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
        ImGui::Text("HoveredRootWindow: '%s'", g.HoveredRootWindow ? g.HoveredRootWindow->Name : "NULL");
        ImGui::Text("HoveredId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d", g.HoveredId, g.HoveredIdPreviousFrame, g.HoveredIdTimer, g.HoveredIdAllowOverlap); // Data is "in-flight" so depending on when the Metrics window is called we may see current frame information or not
        ImGui::Text("ActiveId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d, Source: %s", g.ActiveId, g.ActiveIdPreviousFrame, g.ActiveIdTimer, g.ActiveIdAllowOverlap, input_source_names[g.ActiveIdSource]);
        ImGui::Text("ActiveIdWindow: '%s'", g.ActiveIdWindow ? g.ActiveIdWindow->Name : "NULL");
        ImGui::Text("MovingWindow: '%s'", g.MovingWindow ? g.MovingWindow->Name : "NULL");
        ImGui::Text("NavWindow: '%s'", g.NavWindow ? g.NavWindow->Name : "NULL");
        ImGui::Text("NavId: 0x%08X, NavLayer: %d", g.NavId, g.NavLayer);
        ImGui::Text("NavInputSource: %s", input_source_names[g.NavInputSource]);
        ImGui::Text("NavActive: %d, NavVisible: %d", g.IO.NavActive, g.IO.NavVisible);
        ImGui::Text("NavActivateId: 0x%08X, NavInputId: 0x%08X", g.NavActivateId, g.NavInputId);
        ImGui::Text("NavDisableHighlight: %d, NavDisableMouseHover: %d", g.NavDisableHighlight, g.NavDisableMouseHover);
        ImGui::Text("NavWindowingTarget: '%s'", g.NavWindowingTarget ? g.NavWindowingTarget->Name : "NULL");
        ImGui::Text("DragDrop: %d, SourceId = 0x%08X, Payload \"%s\" (%d bytes)", g.DragDropActive, g.DragDropPayload.SourceId, g.DragDropPayload.DataType, g.DragDropPayload.DataSize);
        ImGui::TreePop();
    }


    if (g.IO.KeyCtrl && show_window_begin_order)
    {
        for (int n = 0; n < g.Windows.Size; n++)
        {
            ImGuiWindow* window = g.Windows[n];
            if ((window->Flags & ImGuiWindowFlags_ChildWindow) || !window->WasActive)
                continue;
            char buf[32];
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", window->BeginOrderWithinContext);
            float font_size = ImGui::GetFontSize() * 2;
            ImDrawList* overlay_draw_list = GetOverlayDrawList();
            overlay_draw_list->AddRectFilled(window->Pos, window->Pos + ImVec2(font_size, font_size), IM_COL32(200, 100, 100, 255));
            overlay_draw_list->AddText(NULL, font_size, window->Pos, IM_COL32(255, 255, 255, 255), buf);
        }
    }
    ImGui::End();
}

//-----------------------------------------------------------------------------

// Include imgui_user.inl at the end of imgui.cpp to access private data/functions that aren't exposed.
// Prefer just including imgui_internal.h from your code rather than using this define. If a declaration is missing from imgui_internal.h add it or request it on the github.
#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

//-----------------------------------------------------------------------------
