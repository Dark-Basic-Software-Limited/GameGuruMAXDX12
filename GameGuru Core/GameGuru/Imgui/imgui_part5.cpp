bool ImGui::BeginDragDropTargetCustom(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindowUnderMovingWindow == NULL || window->RootWindow != g.HoveredWindowUnderMovingWindow->RootWindow)
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
    if (g.HoveredWindowUnderMovingWindow == NULL || window->RootWindow != g.HoveredWindowUnderMovingWindow->RootWindow)
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

const ImGuiPayload* ImGui::GetDragDropPayload()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive ? &g.DragDropPayload : NULL;
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
// All text output from the interface can be captured into tty/file/clipboard.
// By default, tree nodes are automatically opened during logging.
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
        g.LogBuffer.appendfv(fmt, args);
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

    const bool log_new_line = ref_pos && (ref_pos->y > g.LogLinePosY + 1);
    if (ref_pos)
        g.LogLinePosY = ref_pos->y;
    if (log_new_line)
        g.LogLineFirstItem = true;

    const char* text_remaining = text;
    if (g.LogDepthRef > window->DC.TreeDepth)  // Re-adjust padding if we have popped out of our starting depth
        g.LogDepthRef = window->DC.TreeDepth;
    const int tree_depth = (window->DC.TreeDepth - g.LogDepthRef);
    for (;;)
    {
        // Split the string. Each new line (after a '\n') is followed by spacing corresponding to the current depth of our log entry.
        // We don't add a trailing \n to allow a subsequent item on the same line to be captured.
        const char* line_start = text_remaining;
        const char* line_end = ImStreolRange(line_start, text_end);
        const bool is_first_line = (line_start == text);
        const bool is_last_line = (line_end == text_end);
        if (!is_last_line || (line_start != line_end))
        {
            const int char_count = (int)(line_end - line_start);
            if (log_new_line || !is_first_line)
                LogText(IM_NEWLINE "%*s%.*s", tree_depth * 4, "", char_count, line_start);
            else if (g.LogLineFirstItem)
                LogText("%*s%.*s", tree_depth * 4, "", char_count, line_start);
            else
                LogText(" %.*s", char_count, line_start);
            g.LogLineFirstItem = false;
        }
        else if (log_new_line)
        {
            // An empty "" string at a different Y position should output a carriage return.
            LogText(IM_NEWLINE);
            break;
        }

        if (is_last_line)
            break;
        text_remaining = line_end + 1;
    }
}

// Start logging/capturing text output
void ImGui::LogBegin(ImGuiLogType type, int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.LogEnabled == false);
    IM_ASSERT(g.LogFile == NULL);
    IM_ASSERT(g.LogBuffer.empty());
    g.LogEnabled = true;
    g.LogType = type;
    g.LogDepthRef = window->DC.TreeDepth;
    g.LogDepthToExpand = ((auto_open_depth >= 0) ? auto_open_depth : g.LogDepthToExpandDefault);
    g.LogLinePosY = FLT_MAX;
    g.LogLineFirstItem = true;
}

void ImGui::LogToTTY(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_TTY, auto_open_depth);
    g.LogFile = stdout;
}

// Start logging/capturing text output to given file
void ImGui::LogToFile(int auto_open_depth, const char* filename)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;

    // FIXME: We could probably open the file in text mode "at", however note that clipboard/buffer logging will still
    // be subject to outputting OS-incompatible carriage return if within strings the user doesn't use IM_NEWLINE.
    // By opening the file in binary mode "ab" we have consistent output everywhere.
    if (!filename)
        filename = g.IO.LogFilename;
    if (!filename || !filename[0])
        return;
    FILE* f = ImFileOpen(filename, "ab");
    if (f == NULL)
    {
        IM_ASSERT(0);
        return;
    }

    LogBegin(ImGuiLogType_File, auto_open_depth);
    g.LogFile = f;
}

// Start logging/capturing text output to clipboard
void ImGui::LogToClipboard(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_Clipboard, auto_open_depth);
}

void ImGui::LogToBuffer(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_Buffer, auto_open_depth);
}

void ImGui::LogFinish()
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    LogText(IM_NEWLINE);
    switch (g.LogType)
    {
    case ImGuiLogType_TTY:
        fflush(g.LogFile);
        break;
    case ImGuiLogType_File:
        fclose(g.LogFile);
        break;
    case ImGuiLogType_Buffer:
        break;
    case ImGuiLogType_Clipboard:
        if (!g.LogBuffer.empty())
            SetClipboardText(g.LogBuffer.begin());
        break;
    case ImGuiLogType_None:
        IM_ASSERT(0);
        break;
    }

    g.LogEnabled = false;
    g.LogType = ImGuiLogType_None;
    g.LogFile = NULL;
    g.LogBuffer.clear();
}

// Helper to display logging buttons
// FIXME-OBSOLETE: We should probably obsolete this and let the user have their own helper (this is one of the oldest function alive!)
void ImGui::LogButtons()
{
    ImGuiContext& g = *GImGui;

    PushID("LogButtons");
    const bool log_to_tty = Button("Log To TTY"); SameLine();
    const bool log_to_file = Button("Log To File"); SameLine();
    const bool log_to_clipboard = Button("Log To Clipboard"); SameLine();
    PushAllowKeyboardFocus(false);
    SetNextItemWidth(80.0f);
    SliderInt("Default Depth", &g.LogDepthToExpandDefault, 0, 9, NULL);
    PopAllowKeyboardFocus();
    PopID();

    // Start logging at the end of the function so that the buttons don't appear in the log
    if (log_to_tty)
        LogToTTY();
    if (log_to_file)
        LogToFile();
    if (log_to_clipboard)
        LogToClipboard();
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

ImGuiWindowSettings* ImGui::CreateNewWindowSettings(const char* name)
{
    ImGuiContext& g = *GImGui;
    g.SettingsWindows.push_back(ImGuiWindowSettings());
    ImGuiWindowSettings* settings = &g.SettingsWindows.back();
#if !IMGUI_DEBUG_INI_SETTINGS
    // Skip to the "###" marker if any. We don't skip past to match the behavior of GetID()
    // Preserve the full string when IMGUI_DEBUG_INI_SETTINGS is set to make .ini inspection easier.
    if (const char* p = strstr(name, "###"))
        name = p;
#endif
    settings->Name = ImStrdup(name);
    settings->ID = ImHashStr(name);
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

ImGuiWindowSettings* ImGui::FindOrCreateWindowSettings(const char* name)
{
    if (ImGuiWindowSettings* settings = FindWindowSettings(ImHashStr(name)))
        return settings;
    return CreateNewWindowSettings(name);
}

void ImGui::LoadIniSettingsFromDisk(const char* ini_filename)
{
    size_t file_data_size = 0;
    char* file_data = (char*)ImFileLoadToMemory(ini_filename, "rb", &file_data_size);
    if (!file_data)
        return;
    LoadIniSettingsFromMemory(file_data, (size_t)file_data_size);
    IM_FREE(file_data);
}

ImGuiSettingsHandler* ImGui::FindSettingsHandler(const char* type_name)
{
    ImGuiContext& g = *GImGui;
    const ImGuiID type_hash = ImHashStr(type_name);
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
    char* buf = (char*)IM_ALLOC(ini_size + 1);
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
    IM_FREE(buf);
    g.SettingsLoaded = true;
    DockContextOnLoadSettings(&g);
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
    ImGuiWindowSettings* settings = ImGui::FindWindowSettings(ImHashStr(name));
    if (!settings)
        settings = ImGui::CreateNewWindowSettings(name);
    return (void*)settings;
}

static void SettingsHandlerWindow_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiWindowSettings* settings = (ImGuiWindowSettings*)entry;
    int x, y;
    int i;
    ImU32 u1;
    if (sscanf(line, "Pos=%i,%i", &x, &y) == 2)                 { settings->Pos = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "Size=%i,%i", &x, &y) == 2)           { settings->Size = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "ViewportId=0x%08X", &u1) == 1)       { settings->ViewportId = u1; }
    else if (sscanf(line, "ViewportPos=%i,%i", &x, &y) == 2)    { settings->ViewportPos = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "Collapsed=%d", &i) == 1)             { settings->Collapsed = (i != 0); }
    else if (sscanf(line, "DockId=0x%X,%d", &u1, &i) == 2)      { settings->DockId = u1; settings->DockOrder = (short)i; }
    else if (sscanf(line, "DockId=0x%X", &u1) == 1)             { settings->DockId = u1; settings->DockOrder = -1; }
    else if (sscanf(line, "ClassId=0x%X", &u1) == 1)            { settings->ClassId = u1; }
}

static void SettingsHandlerWindow_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    // Gather data from windows that were active during this session
    // (if a window wasn't opened in this session we preserve its settings)
    ImGuiContext& g = *ctx;
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
            continue;

        ImGuiWindowSettings* settings = (window->SettingsIdx != -1) ? &g.SettingsWindows[window->SettingsIdx] : ImGui::FindWindowSettings(window->ID);
        if (!settings)
        {
            settings = ImGui::CreateNewWindowSettings(window->Name);
            window->SettingsIdx = g.SettingsWindows.index_from_ptr(settings);
        }
        IM_ASSERT(settings->ID == window->ID);
        settings->Pos = ImVec2ih(window->Pos - window->ViewportPos);
        settings->Size = ImVec2ih(window->SizeFull);
        settings->ViewportId = window->ViewportId;
        settings->ViewportPos = ImVec2ih(window->ViewportPos);
        IM_ASSERT(window->DockNode == NULL || window->DockNode->ID == window->DockId);
        settings->DockId = window->DockId;
        settings->ClassId = window->WindowClass.ClassId;
        settings->DockOrder = window->DockOrder;
        settings->Collapsed = window->Collapsed;
    }

    // Write to text buffer
    buf->reserve(buf->size() + g.SettingsWindows.Size * 96); // ballpark reserve
    for (int i = 0; i != g.SettingsWindows.Size; i++)
    {
        const ImGuiWindowSettings* settings = &g.SettingsWindows[i];
        buf->appendf("[%s][%s]\n", handler->TypeName, settings->Name);
        if (settings->ViewportId != 0 && settings->ViewportId != ImGui::IMGUI_VIEWPORT_DEFAULT_ID)
        {
            buf->appendf("ViewportPos=%d,%d\n", settings->ViewportPos.x, settings->ViewportPos.y);
            buf->appendf("ViewportId=0x%08X\n", settings->ViewportId);
        }
        if (settings->Pos.x != 0 || settings->Pos.y != 0 || settings->ViewportId == ImGui::IMGUI_VIEWPORT_DEFAULT_ID)
            buf->appendf("Pos=%d,%d\n", settings->Pos.x, settings->Pos.y);
        if (settings->Size.x != 0 || settings->Size.y != 0)
            buf->appendf("Size=%d,%d\n", settings->Size.x, settings->Size.y);
        buf->appendf("Collapsed=%d\n", settings->Collapsed);
        if (settings->DockId != 0)
        {
            // Write DockId as 4 digits if possible. Automatic DockId are small numbers, but full explicit DockSpace() are full ImGuiID range.
            if (settings->DockOrder == -1)
                buf->appendf("DockId=0x%08X\n", settings->DockId);
            else
                buf->appendf("DockId=0x%08X,%d\n", settings->DockId, settings->DockOrder);
            if (settings->ClassId != 0)
                buf->appendf("ClassId=0x%08X\n", settings->ClassId);
        }
        buf->appendf("\n");
    }
}


//-----------------------------------------------------------------------------
// [SECTION] VIEWPORTS, PLATFORM WINDOWS
//-----------------------------------------------------------------------------
// - GetMainViewport()
// - FindViewportByID()
// - FindViewportByPlatformHandle()
// - SetCurrentViewport() [Internal]
// - SetWindowViewport() [Internal]
// - GetWindowAlwaysWantOwnViewport() [Internal]
// - UpdateTryMergeWindowIntoHostViewport() [Internal]
// - UpdateTryMergeWindowIntoHostViewports() [Internal]
// - TranslateWindowsInViewport() [Internal]
// - ScaleWindowsInViewport() [Internal]
// - FindHoveredViewportFromPlatformWindowStack() [Internal]
// - UpdateViewportsNewFrame() [Internal]
// - UpdateViewportsEndFrame() [Internal]
// - AddUpdateViewport() [Internal]
// - UpdateSelectWindowViewport() [Internal]
// - UpdatePlatformWindows()
// - RenderPlatformWindowsDefault()
// - FindPlatformMonitorForPos() [Internal]
// - FindPlatformMonitorForRect() [Internal]
// - UpdateViewportPlatformMonitor() [Internal]
// - DestroyPlatformWindow() [Internal]
// - DestroyPlatformWindows()
//-----------------------------------------------------------------------------

ImGuiViewport* ImGui::GetMainViewport()
{
    ImGuiContext& g = *GImGui;
    return g.Viewports[0];
}

ImGuiViewport* ImGui::FindViewportByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < g.Viewports.Size; n++)
        if (g.Viewports[n]->ID == id)
            return g.Viewports[n];
    return NULL;
}

ImGuiViewport* ImGui::FindViewportByPlatformHandle(void* platform_handle)
{
    ImGuiContext& g = *GImGui;
    for (int i = 0; i != g.Viewports.Size; i++)
        if (g.Viewports[i]->PlatformHandle == platform_handle)
            return g.Viewports[i];
    return NULL;
}

void ImGui::SetCurrentViewport(ImGuiWindow* current_window, ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    (void)current_window;

    if (viewport)
        viewport->LastFrameActive = g.FrameCount;
    if (g.CurrentViewport == viewport)
        return;
    g.CurrentViewport = viewport;
    //IMGUI_DEBUG_LOG_VIEWPORT("SetCurrentViewport changed '%s' 0x%08X\n", current_window ? current_window->Name : NULL, viewport ? viewport->ID : 0);

    // Notify platform layer of viewport changes
    // FIXME-DPI: This is only currently used for experimenting with handling of multiple DPI
    if (g.CurrentViewport && g.PlatformIO.Platform_OnChangedViewport)
        g.PlatformIO.Platform_OnChangedViewport(g.CurrentViewport);
}

static void SetWindowViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    window->Viewport = viewport;
    window->ViewportId = viewport->ID;
    window->ViewportOwned = (viewport->Window == window);
}

static bool ImGui::GetWindowAlwaysWantOwnViewport(ImGuiWindow* window)
{
    // Tooltips and menus are not automatically forced into their own viewport when the NoMerge flag is set, however the multiplication of viewports makes them more likely to protrude and create their own.
    ImGuiContext& g = *GImGui;
    if (g.IO.ConfigViewportsNoAutoMerge || (window->WindowClass.ViewportFlagsOverrideSet & ImGuiViewportFlags_NoAutoMerge))
        if (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable)
            if (!window->DockIsActive)
                if ((window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip)) == 0)
                    if ((window->Flags & ImGuiWindowFlags_Popup) == 0 || (window->Flags & ImGuiWindowFlags_Modal) != 0)
                        return true;
    return false;
}

static bool ImGui::UpdateTryMergeWindowIntoHostViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    if (!(viewport->Flags & (ImGuiViewportFlags_CanHostOtherWindows | ImGuiViewportFlags_Minimized)) || window->Viewport == viewport)
        return false;
    if (!viewport->GetRect().Contains(window->Rect()))
        return false;
    if (GetWindowAlwaysWantOwnViewport(window))
        return false;

    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window_behind = g.Windows[n];
        if (window_behind == window)
            break;
        if (window_behind->WasActive && window_behind->ViewportOwned && !(window_behind->Flags & ImGuiWindowFlags_ChildWindow))
            if (window_behind->Viewport->GetRect().Overlaps(window->Rect()))
                return false;
    }

    // Move to the existing viewport, Move child/hosted windows as well (FIXME-OPT: iterate child)
    ImGuiViewportP* old_viewport = window->Viewport;
    if (window->ViewportOwned)
        for (int n = 0; n < g.Windows.Size; n++)
            if (g.Windows[n]->Viewport == old_viewport)
                SetWindowViewport(g.Windows[n], viewport);
    SetWindowViewport(window, viewport);
    BringWindowToDisplayFront(window);

    return true;
}

static bool ImGui::UpdateTryMergeWindowIntoHostViewports(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    return UpdateTryMergeWindowIntoHostViewport(window, g.Viewports[0]);
}

// Translate imgui windows when a Host Viewport has been moved
// (This additionally keeps windows at the same place when ImGuiConfigFlags_ViewportsEnable is toggled!)
void ImGui::TranslateWindowsInViewport(ImGuiViewportP* viewport, const ImVec2& old_pos, const ImVec2& new_pos)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(viewport->Window == NULL && (viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows));

    // 1) We test if ImGuiConfigFlags_ViewportsEnable was just toggled, which allows us to conveniently
    // translate imgui windows from OS-window-local to absolute coordinates or vice-versa.
    // 2) If it's not going to fit into the new size, keep it at same absolute position.
    // One problem with this is that most Win32 applications doesn't update their render while dragging, 
    // and so the window will appear to teleport when releasing the mouse.
    const bool translate_all_windows = (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable) != (g.ConfigFlagsLastFrame & ImGuiConfigFlags_ViewportsEnable);
    ImRect test_still_fit_rect(old_pos, old_pos + viewport->Size);
    ImVec2 delta_pos = new_pos - old_pos;
    for (int window_n = 0; window_n < g.Windows.Size; window_n++) // FIXME-OPT
        if (translate_all_windows || (g.Windows[window_n]->Viewport == viewport && test_still_fit_rect.Contains(g.Windows[window_n]->Rect())))
            TranslateWindow(g.Windows[window_n], delta_pos);
}

// Scale all windows (position, size). Use when e.g. changing DPI. (This is a lossy operation!)
void ImGui::ScaleWindowsInViewport(ImGuiViewportP* viewport, float scale)
{
    ImGuiContext& g = *GImGui;
    if (viewport->Window)
    {
        ScaleWindow(viewport->Window, scale);
    }
    else
    {
        for (int i = 0; i != g.Windows.Size; i++)
            if (g.Windows[i]->Viewport == viewport)
                ScaleWindow(g.Windows[i], scale);
    }
}

// If the back-end doesn't set MouseLastHoveredViewport or doesn't honor ImGuiViewportFlags_NoInputs, we do a search ourselves.
// A) It won't take account of the possibility that non-imgui windows may be in-between our dragged window and our target window. 
// B) It requires Platform_GetWindowFocus to be implemented by back-end.
static ImGuiViewportP* FindHoveredViewportFromPlatformWindowStack(const ImVec2 mouse_platform_pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* best_candidate = NULL;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        if (!(viewport->Flags & (ImGuiViewportFlags_NoInputs | ImGuiViewportFlags_Minimized)) && viewport->GetRect().Contains(mouse_platform_pos))
            if (best_candidate == NULL || best_candidate->LastFrontMostStampCount < viewport->LastFrontMostStampCount)
                best_candidate = viewport;
    }
    return best_candidate;
}

// Update viewports and monitor infos
// Note that this is running even if 'ImGuiConfigFlags_ViewportsEnable' is not set, in order to clear unused viewports (if any) and update monitor info.
static void ImGui::UpdateViewportsNewFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.PlatformIO.Viewports.Size <= g.Viewports.Size);

    // Update Minimized status (we need it first in order to decide if we'll apply Pos/Size of the main viewport)
    if ((g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
    {
        for (int n = 0; n < g.Viewports.Size; n++)
        {
            ImGuiViewportP* viewport = g.Viewports[n];
            const bool platform_funcs_available = viewport->PlatformWindowCreated;
            if (g.PlatformIO.Platform_GetWindowMinimized && platform_funcs_available)
            {
                bool minimized = g.PlatformIO.Platform_GetWindowMinimized(viewport);
                if (minimized)
                    viewport->Flags |= ImGuiViewportFlags_Minimized;
                else
                    viewport->Flags &= ~ImGuiViewportFlags_Minimized;
            }
        }
    }

    // Create/update main viewport with current platform position and size
    ImGuiViewportP* main_viewport = g.Viewports[0];
    IM_ASSERT(main_viewport->ID == IMGUI_VIEWPORT_DEFAULT_ID);
    IM_ASSERT(main_viewport->Window == NULL);
    ImVec2 main_viewport_platform_pos = ImVec2(0.0f, 0.0f);
    ImVec2 main_viewport_platform_size = g.IO.DisplaySize;

    if (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable)
        main_viewport_platform_pos = (main_viewport->Flags & ImGuiViewportFlags_Minimized) ? main_viewport->Pos : g.PlatformIO.Platform_GetWindowPos(main_viewport);
    AddUpdateViewport(NULL, IMGUI_VIEWPORT_DEFAULT_ID, main_viewport_platform_pos, main_viewport_platform_size, ImGuiViewportFlags_CanHostOtherWindows);

    g.CurrentViewport = NULL;
    g.MouseViewport = NULL;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->Idx = n;

        // Erase unused viewports
        if (n > 0 && viewport->LastFrameActive < g.FrameCount - 2)
        {
            // Clear references to this viewport in windows (window->ViewportId becomes the master data)
            for (int window_n = 0; window_n < g.Windows.Size; window_n++)
                if (g.Windows[window_n]->Viewport == viewport)
                {
                    g.Windows[window_n]->Viewport = NULL;
                    g.Windows[window_n]->ViewportOwned = false;
                }
            if (viewport == g.MouseLastHoveredViewport) 
                g.MouseLastHoveredViewport = NULL;
            g.Viewports.erase(g.Viewports.Data + n);

            // Destroy
            IMGUI_DEBUG_LOG_VIEWPORT("Delete Viewport %08X (%s)\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
            DestroyPlatformWindow(viewport); // In most circumstances the platform window will already be destroyed here.
            IM_ASSERT(g.PlatformIO.Viewports.contains(viewport) == false);
            IM_DELETE(viewport);
            n--;
            continue;
        }

        const bool platform_funcs_available = viewport->PlatformWindowCreated;
        if ((g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
        {
            // Update Position and Size (from Platform Window to ImGui) if requested. 
            // We do it early in the frame instead of waiting for UpdatePlatformWindows() to avoid a frame of lag when moving/resizing using OS facilities.
            if (!(viewport->Flags & ImGuiViewportFlags_Minimized) && platform_funcs_available)
            {
                if (viewport->PlatformRequestMove)
                    viewport->Pos = viewport->LastPlatformPos = g.PlatformIO.Platform_GetWindowPos(viewport);
                if (viewport->PlatformRequestResize)
                    viewport->Size = viewport->LastPlatformSize = g.PlatformIO.Platform_GetWindowSize(viewport);
            }
        }

        // Update/copy monitor info
        UpdateViewportPlatformMonitor(viewport);

        // Reset alpha every frame. Users of transparency (docking) needs to request a lower alpha back.
        viewport->Alpha = 1.0f;

        // Translate imgui windows when a Host Viewport has been moved
        // (This additionally keeps windows at the same place when ImGuiConfigFlags_ViewportsEnable is toggled!)
        const ImVec2 viewport_delta_pos = viewport->Pos - viewport->LastPos;
        if ((viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows) && (viewport_delta_pos.x != 0.0f || viewport_delta_pos.y != 0.0f))
            TranslateWindowsInViewport(viewport, viewport->LastPos, viewport->Pos);

        // Update DPI scale
        float new_dpi_scale;
        if (g.PlatformIO.Platform_GetWindowDpiScale && platform_funcs_available)
            new_dpi_scale = g.PlatformIO.Platform_GetWindowDpiScale(viewport);
        else if (viewport->PlatformMonitor != -1)
            new_dpi_scale = g.PlatformIO.Monitors[viewport->PlatformMonitor].DpiScale;
        else
            new_dpi_scale = (viewport->DpiScale != 0.0f) ? viewport->DpiScale : 1.0f;

		if (viewport->DpiScale != 0.0f && new_dpi_scale != viewport->DpiScale)
        {
            float scale_factor = new_dpi_scale / viewport->DpiScale;
            if (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
                ScaleWindowsInViewport(viewport, scale_factor);
            //if (viewport == GetMainViewport())
            //    g.PlatformInterface.SetWindowSize(viewport, viewport->Size * scale_factor);

            // Scale our window moving pivot so that the window will rescale roughly around the mouse position.
            // FIXME-VIEWPORT: This currently creates a resizing feedback loop when a window is straddling a DPI transition border.
            // (Minor: since our sizes do not perfectly linearly scale, deferring the click offset scale until we know the actual window scale ratio may get us slightly more precise mouse positioning.)
            //if (g.MovingWindow != NULL && g.MovingWindow->Viewport == viewport)
            //    g.ActiveIdClickOffset = ImFloor(g.ActiveIdClickOffset * scale_factor);
        }
        viewport->DpiScale = new_dpi_scale;
	}

    if (!(g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
    {
        g.MouseViewport = main_viewport;
        return;
    }

    // Mouse handling: decide on the actual mouse viewport for this frame between the active/focused viewport and the hovered viewport.
    // Note that 'viewport_hovered' should skip over any viewport that has the ImGuiViewportFlags_NoInputs flags set.
    ImGuiViewportP* viewport_hovered = NULL;
    if (g.IO.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        viewport_hovered = g.IO.MouseHoveredViewport ? (ImGuiViewportP*)FindViewportByID(g.IO.MouseHoveredViewport) : NULL;
        if (viewport_hovered && (viewport_hovered->Flags & ImGuiViewportFlags_NoInputs))
        {
            // Back-end failed at honoring its contract if it returned a viewport with the _NoInputs flag.
            IM_ASSERT(0);
            viewport_hovered = FindHoveredViewportFromPlatformWindowStack(g.IO.MousePos);
        }
    }
    else
    {
        // If the back-end doesn't know how to honor ImGuiViewportFlags_NoInputs, we do a search ourselves. Note that this search:
        // A) won't take account of the possibility that non-imgui windows may be in-between our dragged window and our target window. 
        // B) uses LastFrameAsRefViewport as a flawed replacement for the last time a window was focused (we could/should fix that by introducing Focus functions in PlatformIO)
        viewport_hovered = FindHoveredViewportFromPlatformWindowStack(g.IO.MousePos);
    }
    if (viewport_hovered != NULL)
        g.MouseLastHoveredViewport = viewport_hovered;
    else if (g.MouseLastHoveredViewport == NULL)
        g.MouseLastHoveredViewport = g.Viewports[0];

    // Update mouse reference viewport
    // (when moving a window we aim at its viewport, but this will be overwritten below if we go in drag and drop mode)
    if (g.MovingWindow)
        g.MouseViewport = g.MovingWindow->Viewport; 
    else
        g.MouseViewport = g.MouseLastHoveredViewport;

    // When dragging something, always refer to the last hovered viewport. 
    // - when releasing a moving window we will revert to aiming behind (at viewport_hovered)
    // - when we are between viewports, our dragged preview will tend to show in the last viewport _even_ if we don't have tooltips in their viewports (when lacking monitor info)
    // - consider the case of holding on a menu item to browse child menus: even thou a mouse button is held, there's no active id because menu items only react on mouse release.
    const bool is_mouse_dragging_with_an_expected_destination = g.DragDropActive;
    if (is_mouse_dragging_with_an_expected_destination && viewport_hovered == NULL)
        viewport_hovered = g.MouseLastHoveredViewport;
    if (is_mouse_dragging_with_an_expected_destination || g.ActiveId == 0 || !IsAnyMouseDown())
        if (viewport_hovered != NULL && viewport_hovered != g.MouseViewport && !(viewport_hovered->Flags & ImGuiViewportFlags_NoInputs))
            g.MouseViewport = viewport_hovered;

    IM_ASSERT(g.MouseViewport != NULL);
}

// Update user-facing viewport list (g.Viewports -> g.PlatformIO.Viewports after filtering out some)
static void ImGui::UpdateViewportsEndFrame()
{
    ImGuiContext& g = *GImGui;
    g.PlatformIO.MainViewport = g.Viewports[0];
    g.PlatformIO.Viewports.resize(0);
    for (int i = 0; i < g.Viewports.Size; i++)
    {
        ImGuiViewportP* viewport = g.Viewports[i];
        viewport->LastPos = viewport->Pos;
        if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0.0f || viewport->Size.y <= 0.0f)
            if (i > 0) // Always include main viewport in the list
                continue;
        if (viewport->Window && !IsWindowActiveAndVisible(viewport->Window))
            continue;
        if (i > 0)
            IM_ASSERT(viewport->Window != NULL);
        g.PlatformIO.Viewports.push_back(viewport);
    }
    g.Viewports[0]->ClearRequestFlags(); // Clear main viewport flags because UpdatePlatformWindows() won't do it and may not even be called
}

// FIXME: We should ideally refactor the system to call this every frame (we currently don't)
ImGuiViewportP* ImGui::AddUpdateViewport(ImGuiWindow* window, ImGuiID id, const ImVec2& pos, const ImVec2& size, ImGuiViewportFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    if (window != NULL)
    {
        if (g.MovingWindow && g.MovingWindow->RootWindow == window)
            flags |= ImGuiViewportFlags_NoInputs | ImGuiViewportFlags_NoFocusOnAppearing;
        if ((window->Flags & ImGuiWindowFlags_NoMouseInputs) && (window->Flags & ImGuiWindowFlags_NoNavInputs))
            flags |= ImGuiViewportFlags_NoInputs;
        if (window->Flags & ImGuiWindowFlags_NoFocusOnAppearing)
            flags |= ImGuiViewportFlags_NoFocusOnAppearing;
    }

    ImGuiViewportP* viewport = (ImGuiViewportP*)FindViewportByID(id);
    if (viewport)
    {
        if (!viewport->PlatformRequestMove)
            viewport->Pos = pos;
        if (!viewport->PlatformRequestResize)
            viewport->Size = size;
        viewport->Flags = flags | (viewport->Flags & ImGuiViewportFlags_Minimized); // Preserve existing flags
    }
    else
    {
        // New viewport
        viewport = IM_NEW(ImGuiViewportP)();
        viewport->ID = id;
        viewport->Idx = g.Viewports.Size;
        viewport->Pos = viewport->LastPos = pos;
        viewport->Size = size;
        viewport->Flags = flags;
        UpdateViewportPlatformMonitor(viewport);
        g.Viewports.push_back(viewport);
        IMGUI_DEBUG_LOG_VIEWPORT("Add Viewport %08X (%s)\n", id, window->Name);

        // We normally setup for all viewports in NewFrame() but here need to handle the mid-frame creation of a new viewport.
        // We need to extend the fullscreen clip rect so the OverlayDrawList clip is correct for that the first frame
        g.DrawListSharedData.ClipRectFullscreen.x = ImMin(g.DrawListSharedData.ClipRectFullscreen.x, viewport->Pos.x);
        g.DrawListSharedData.ClipRectFullscreen.y = ImMin(g.DrawListSharedData.ClipRectFullscreen.y, viewport->Pos.y);
        g.DrawListSharedData.ClipRectFullscreen.z = ImMax(g.DrawListSharedData.ClipRectFullscreen.z, viewport->Pos.x + viewport->Size.x);
        g.DrawListSharedData.ClipRectFullscreen.w = ImMax(g.DrawListSharedData.ClipRectFullscreen.w, viewport->Pos.y + viewport->Size.y);

        // Store initial DpiScale before the OS platform window creation, based on expected monitor data.
        // This is so we can select an appropriate font size on the first frame of our window lifetime
        if (viewport->PlatformMonitor != -1)
            viewport->DpiScale = g.PlatformIO.Monitors[viewport->PlatformMonitor].DpiScale;
    }

    viewport->Window = window;
    viewport->LastFrameActive = g.FrameCount;
    IM_ASSERT(window == NULL || viewport->ID == window->ID);

    if (window != NULL)
        window->ViewportOwned = true;

    return viewport;
}

// FIXME-VIEWPORT: This is all super messy and ought to be clarified or rewritten.
static void ImGui::UpdateSelectWindowViewport(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;
    window->ViewportAllowPlatformMonitorExtend = -1;

    // Restore main viewport if multi-viewport is not supported by the back-end
    ImGuiViewportP* main_viewport = g.Viewports[0];
    if (!(g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
    {
        SetWindowViewport(window, main_viewport);
        return;
    }
    window->ViewportOwned = false;

    // Appearing popups reset their viewport so they can inherit again
    if ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && window->Appearing)
    {
        window->Viewport = NULL;
        window->ViewportId = 0;
    }

    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasViewport) == 0)
    {
        // By default inherit from parent window
        if (window->Viewport == NULL && window->ParentWindow && !window->ParentWindow->IsFallbackWindow)
            window->Viewport = window->ParentWindow->Viewport;

        // Attempt to restore saved viewport id (= window that hasn't been activated yet), try to restore the viewport based on saved 'window->ViewportPos' restored from .ini file
        if (window->Viewport == NULL && window->ViewportId != 0)
        {
            window->Viewport = (ImGuiViewportP*)FindViewportByID(window->ViewportId);
            if (window->Viewport == NULL && window->ViewportPos.x != FLT_MAX && window->ViewportPos.y != FLT_MAX)
                window->Viewport = AddUpdateViewport(window, window->ID, window->ViewportPos, window->Size, ImGuiViewportFlags_None);
        }
    }

    bool lock_viewport = false;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasViewport)
    {
        // Code explicitly request a viewport
        window->Viewport = (ImGuiViewportP*)FindViewportByID(g.NextWindowData.ViewportId);
        window->ViewportId = g.NextWindowData.ViewportId; // Store ID even if Viewport isn't resolved yet.
        lock_viewport = true;
    }
    else if ((flags & ImGuiWindowFlags_ChildWindow) || (flags & ImGuiWindowFlags_ChildMenu))
    {
        // Always inherit viewport from parent window
        window->Viewport = window->ParentWindow->Viewport;
    }
    else if (flags & ImGuiWindowFlags_Tooltip)
    {
        window->Viewport = g.MouseViewport;
    }
    else if (GetWindowAlwaysWantOwnViewport(window))
    {
        window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    }
    else if (g.MovingWindow && g.MovingWindow->RootWindow == window && IsMousePosValid())
    {
        if (window->Viewport != NULL && window->Viewport->Window == window)
            window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    }
    else
    {
        // Merge into host viewport?
        // We cannot test window->ViewportOwned as it set lower in the function.
        bool try_to_merge_into_host_viewport = (window->Viewport && window == window->Viewport->Window && g.ActiveId == 0);
        if (try_to_merge_into_host_viewport)
            UpdateTryMergeWindowIntoHostViewports(window);
    }

    // Fallback to default viewport
    if (window->Viewport == NULL)
        window->Viewport = main_viewport;

    // Mark window as allowed to protrude outside of its viewport and into the current monitor
    if (!lock_viewport)
    {
        if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        {
            // We need to take account of the possibility that mouse may become invalid.
            // Popups/Tooltip always set ViewportAllowPlatformMonitorExtend so GetWindowAllowedExtentRect() will return full monitor bounds.
            ImVec2 mouse_ref = (flags & ImGuiWindowFlags_Tooltip) ? g.IO.MousePos : g.BeginPopupStack.back().OpenMousePos;
            bool use_mouse_ref = (g.NavDisableHighlight || !g.NavDisableMouseHover || !g.NavWindow);
            bool mouse_valid = IsMousePosValid(&mouse_ref);
            if ((window->Appearing || (flags & ImGuiWindowFlags_Tooltip)) && (!use_mouse_ref || mouse_valid))
                window->ViewportAllowPlatformMonitorExtend = FindPlatformMonitorForPos((use_mouse_ref && mouse_valid) ? mouse_ref : NavCalcPreferredRefPos());
            else
                window->ViewportAllowPlatformMonitorExtend = window->Viewport->PlatformMonitor;
        }
        else if (window->Viewport && window != window->Viewport->Window && window->Viewport->Window && !(flags & ImGuiWindowFlags_ChildWindow))
        {
            // When called from Begin() we don't have access to a proper version of the Hidden flag yet, so we replicate this code.
            const bool will_be_visible = (window->DockIsActive && !window->DockTabIsVisible) ? false : true;
            if ((window->Flags & ImGuiWindowFlags_DockNodeHost) && window->Viewport->LastFrameActive < g.FrameCount && will_be_visible)
            {
                // Steal/transfer ownership
                IMGUI_DEBUG_LOG_VIEWPORT("Window '%s' steal Viewport %08X from Window '%s'\n", window->Name, window->Viewport->ID, window->Viewport->Window->Name);
                window->Viewport->Window = window;
                window->Viewport->ID = window->ID;
                window->Viewport->LastNameHash = 0;
            }
            else if (!UpdateTryMergeWindowIntoHostViewports(window)) // Merge?
            {
                // New viewport
                window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_NoFocusOnAppearing);
            }
        }
        else if (window->ViewportAllowPlatformMonitorExtend < 0 && (flags & ImGuiWindowFlags_ChildWindow) == 0)
        {
            // Regular (non-child, non-popup) windows by default are also allowed to protrude
            // Child windows are kept contained within their parent.
            window->ViewportAllowPlatformMonitorExtend = window->Viewport->PlatformMonitor;
        }
    }

    // Update flags
    window->ViewportOwned = (window == window->Viewport->Window);
    window->ViewportId = window->Viewport->ID;

    // If the OS window has a title bar, hide our imgui title bar
    //if (window->ViewportOwned && !(window->Viewport->Flags & ImGuiViewportFlags_NoDecoration))
    //    window->Flags |= ImGuiWindowFlags_NoTitleBar;
}

// Called by user at the end of the main loop, after EndFrame()
// This will handle the creation/update of all OS windows via function defined in the ImGuiPlatformIO api.
void ImGui::UpdatePlatformWindows()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.FrameCountEnded == g.FrameCount && "Forgot to call Render() or EndFrame() before UpdatePlatformWindows()?");
    IM_ASSERT(g.FrameCountPlatformEnded < g.FrameCount);
    g.FrameCountPlatformEnded = g.FrameCount;
    if (!(g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
        return;

    // Create/resize/destroy platform windows to match each active viewport.
    // Skip the main viewport (index 0), which is always fully handled by the application!
    for (int i = 1; i < g.Viewports.Size; i++)
    {
        ImGuiViewportP* viewport = g.Viewports[i];

        // Destroy platform window if the viewport hasn't been submitted or if it is hosting a hidden window 
        // (the implicit/fallback Debug##Default window will be registering its viewport then be disabled, causing a dummy DestroyPlatformWindow to be made each frame)
        bool destroy_platform_window = false;
        destroy_platform_window |= (viewport->LastFrameActive < g.FrameCount - 1);
        destroy_platform_window |= (viewport->Window && !IsWindowActiveAndVisible(viewport->Window));
        if (destroy_platform_window)
        {
            DestroyPlatformWindow(viewport);
            continue;
        }

        // New windows that appears directly in a new viewport won't always have a size on their first frame
        if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0 || viewport->Size.y <= 0)
            continue;

        // Create window
        bool is_new_platform_window = (viewport->PlatformWindowCreated == false);
        if (is_new_platform_window)
        {
            IMGUI_DEBUG_LOG_VIEWPORT("Create Platform Window %08X (%s)\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
            g.PlatformIO.Platform_CreateWindow(viewport);
            if (g.PlatformIO.Renderer_CreateWindow != NULL)
                g.PlatformIO.Renderer_CreateWindow(viewport);
            viewport->LastNameHash = 0;
            viewport->LastPlatformPos = viewport->LastPlatformSize = ImVec2(FLT_MAX, FLT_MAX); // By clearing those we'll enforce a call to Platform_SetWindowPos/Size below, before Platform_ShowWindow (FIXME: Is that necessary?)
            viewport->LastRendererSize = viewport->Size;                                       // We don't need to call Renderer_SetWindowSize() as it is expected Renderer_CreateWindow() already did it.
            viewport->PlatformWindowCreated = true;
        }

        // Apply Position and Size (from ImGui to Platform/Renderer back-ends)
        if ((viewport->LastPlatformPos.x != viewport->Pos.x || viewport->LastPlatformPos.y != viewport->Pos.y) && !viewport->PlatformRequestMove)
            g.PlatformIO.Platform_SetWindowPos(viewport, viewport->Pos);
        if ((viewport->LastPlatformSize.x != viewport->Size.x || viewport->LastPlatformSize.y != viewport->Size.y) && !viewport->PlatformRequestResize)
            g.PlatformIO.Platform_SetWindowSize(viewport, viewport->Size);
        if ((viewport->LastRendererSize.x != viewport->Size.x || viewport->LastRendererSize.y != viewport->Size.y) && g.PlatformIO.Renderer_SetWindowSize)
            g.PlatformIO.Renderer_SetWindowSize(viewport, viewport->Size);
        viewport->LastPlatformPos = viewport->Pos;
        viewport->LastPlatformSize = viewport->LastRendererSize = viewport->Size;

        // Update title bar (if it changed)
        if (ImGuiWindow* window_for_title = GetWindowForTitleDisplay(viewport->Window))
        {
            const char* title_begin = window_for_title->Name;
            char* title_end = (char*)(intptr_t)FindRenderedTextEnd(title_begin);
            const ImGuiID title_hash = ImHashStr(title_begin, title_end - title_begin);
            if (viewport->LastNameHash != title_hash)
            {
                char title_end_backup_c = *title_end;
                *title_end = 0; // Cut existing buffer short instead of doing an alloc/free, no small gain.
                g.PlatformIO.Platform_SetWindowTitle(viewport, title_begin);
                *title_end = title_end_backup_c;
                viewport->LastNameHash = title_hash;
            }
        }

        // Update alpha (if it changed)
        if (viewport->LastAlpha != viewport->Alpha && g.PlatformIO.Platform_SetWindowAlpha)
            g.PlatformIO.Platform_SetWindowAlpha(viewport, viewport->Alpha);
        viewport->LastAlpha = viewport->Alpha;

        // Optional, general purpose call to allow the back-end to perform general book-keeping even if things haven't changed.
        if (g.PlatformIO.Platform_UpdateWindow)
            g.PlatformIO.Platform_UpdateWindow(viewport);

        if (is_new_platform_window)
        {
            // On startup ensure new platform window don't steal focus (give it a few frames, as nested contents may lead to viewport being created a few frames late)
            if (g.FrameCount < 3)
                viewport->Flags |= ImGuiViewportFlags_NoFocusOnAppearing;

            // Show window
            g.PlatformIO.Platform_ShowWindow(viewport);

            // Even without focus, we assume the window becomes front-most. 
            // This is useful for our platform z-order heuristic when io.MouseHoveredViewport is not available.
            if (viewport->LastFrontMostStampCount != g.ViewportFrontMostStampCount)
                viewport->LastFrontMostStampCount = ++g.ViewportFrontMostStampCount;
        }

        // Clear request flags
        viewport->ClearRequestFlags();
    }

    // Update our implicit z-order knowledge of platform windows, which is used when the back-end cannot provide io.MouseHoveredViewport.
    // When setting Platform_GetWindowFocus, it is expected that the platform back-end can handle calls without crashing if it doesn't have data stored. 
    if (g.PlatformIO.Platform_GetWindowFocus != NULL)
    {
        ImGuiViewportP* focused_viewport = NULL;
        for (int n = 0; n < g.Viewports.Size && focused_viewport == NULL; n++)
        {
            ImGuiViewportP* viewport = g.Viewports[n];
            if (viewport->PlatformWindowCreated)
                if (g.PlatformIO.Platform_GetWindowFocus(viewport))
                    focused_viewport = viewport;
        }
        if (focused_viewport && g.PlatformLastFocusedViewport != focused_viewport->ID)
        {
            if (focused_viewport->LastFrontMostStampCount != g.ViewportFrontMostStampCount)
                focused_viewport->LastFrontMostStampCount = ++g.ViewportFrontMostStampCount;
            g.PlatformLastFocusedViewport = focused_viewport->ID;
        }
    }
}

// This is a default/basic function for performing the rendering/swap of multiple Platform Windows.
// Custom renderers may prefer to not call this function at all, and instead iterate the publicly exposed platform data and handle rendering/sync themselves.
// The Render/Swap functions stored in ImGuiPlatformIO are merely here to allow for this helper to exist, but you can do it yourself:
//
//    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
//    for (int i = 1; i < platform_io.Viewports.Size; i++)
//        if ((platform_io.Viewports[i]->Flags & ImGuiViewportFlags_Minimized) == 0)
//            MyRenderFunction(platform_io.Viewports[i], my_args);
//    for (int i = 1; i < platform_io.Viewports.Size; i++)
//        if ((platform_io.Viewports[i]->Flags & ImGuiViewportFlags_Minimized) == 0)
//            MySwapBufferFunction(platform_io.Viewports[i], my_args);
//
void ImGui::RenderPlatformWindowsDefault(void* platform_render_arg, void* renderer_render_arg)
{
    // Skip the main viewport (index 0), which is always fully handled by the application!
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (viewport->Flags & ImGuiViewportFlags_Minimized)
            continue;
        if (platform_io.Platform_RenderWindow) platform_io.Platform_RenderWindow(viewport, platform_render_arg);
        if (platform_io.Renderer_RenderWindow) platform_io.Renderer_RenderWindow(viewport, renderer_render_arg);
    }
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (viewport->Flags & ImGuiViewportFlags_Minimized)
            continue;
        if (platform_io.Platform_SwapBuffers) platform_io.Platform_SwapBuffers(viewport, platform_render_arg);
        if (platform_io.Renderer_SwapBuffers) platform_io.Renderer_SwapBuffers(viewport, renderer_render_arg);
    }
}

static int ImGui::FindPlatformMonitorForPos(const ImVec2& pos)
{
    ImGuiContext& g = *GImGui;
    for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size; monitor_n++)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[monitor_n];
        if (ImRect(monitor.MainPos, monitor.MainPos + monitor.MainSize).Contains(pos))
            return monitor_n;
    }
    return -1;
}

// Search for the monitor with the largest intersection area with the given rectangle
// We generally try to avoid searching loops but the monitor count should be very small here
// FIXME-OPT: We could test the last monitor used for that viewport first, and early 
static int ImGui::FindPlatformMonitorForRect(const ImRect& rect)
{
    ImGuiContext& g = *GImGui;

    const int monitor_count = g.PlatformIO.Monitors.Size;
    if (monitor_count <= 1)
        return monitor_count - 1;

    // Use a minimum threshold of 1.0f so a zero-sized rect won't false positive, and will still find the correct monitor given its position. 
    // This is necessary for tooltips which always resize down to zero at first.
    const float surface_threshold = ImMax(rect.GetWidth() * rect.GetHeight() * 0.5f, 1.0f);
    int best_monitor_n = -1;
    float best_monitor_surface = 0.001f;

    for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size && best_monitor_surface < surface_threshold; monitor_n++)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[monitor_n];
        const ImRect monitor_rect = ImRect(monitor.MainPos, monitor.MainPos + monitor.MainSize);
        if (monitor_rect.Contains(rect))
            return monitor_n;
        ImRect overlapping_rect = rect;
        overlapping_rect.ClipWithFull(monitor_rect);
        float overlapping_surface = overlapping_rect.GetWidth() * overlapping_rect.GetHeight();
        if (overlapping_surface < best_monitor_surface)
            continue;
        best_monitor_surface = overlapping_surface;
        best_monitor_n = monitor_n;
    }
    return best_monitor_n;
}

// Update monitor from viewport rectangle (we'll use this info to clamp windows and save windows lost in a removed monitor)
static void ImGui::UpdateViewportPlatformMonitor(ImGuiViewportP* viewport)
{
    viewport->PlatformMonitor = (short)FindPlatformMonitorForRect(viewport->GetRect());
}

void ImGui::DestroyPlatformWindow(ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    if (viewport->PlatformWindowCreated)
    {
        if (g.PlatformIO.Renderer_DestroyWindow)
            g.PlatformIO.Renderer_DestroyWindow(viewport);
        if (g.PlatformIO.Platform_DestroyWindow)
            g.PlatformIO.Platform_DestroyWindow(viewport);
        IM_ASSERT(viewport->RendererUserData == NULL && viewport->PlatformUserData == NULL);
        viewport->PlatformWindowCreated = false;
    }
    else
    {
        IM_ASSERT(viewport->RendererUserData == NULL && viewport->PlatformUserData == NULL && viewport->PlatformHandle == NULL);
    }
    viewport->RendererUserData = viewport->PlatformUserData = viewport->PlatformHandle = NULL;
    viewport->ClearRequestFlags();
}

void ImGui::DestroyPlatformWindows()
{
    // We call the destroy window on every viewport (including the main viewport, index 0) to give a chance to the back-end 
    // to clear any data they may have stored in e.g. PlatformUserData, RendererUserData. 
    // It is convenient for the platform back-end code to store something in the main viewport, in order for e.g. the mouse handling 
    // code to operator a consistent manner.
    // It is expected that the back-end can handle calls to Renderer_DestroyWindow/Platform_DestroyWindow without
    // crashing if it doesn't have data stored. 
    ImGuiContext& g = *GImGui;
    for (int i = 0; i < g.Viewports.Size; i++)
        DestroyPlatformWindow(g.Viewports[i]);
}


//-----------------------------------------------------------------------------
// [SECTION] DOCKING
//-----------------------------------------------------------------------------
// Docking: Internal Types
// Docking: Forward Declarations
// Docking: ImGuiDockContext
// Docking: ImGuiDockContext Docking/Undocking functions
// Docking: ImGuiDockNode
// Docking: ImGuiDockNode Tree manipulation functions
// Docking: Public Functions (SetWindowDock, DockSpace, DockSpaceOverViewport)
// Docking: Builder Functions
// Docking: Begin/End Support Functions (called from Begin/End)
// Docking: Settings
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Docking: Internal Types
//-----------------------------------------------------------------------------
// - ImGuiDockRequestType
// - ImGuiDockRequest
// - ImGuiDockPreviewData
// - ImGuiDockNodeSettings
// - ImGuiDockContext
//-----------------------------------------------------------------------------

static float IMGUI_DOCK_SPLITTER_SIZE = 2.0f;

enum ImGuiDockRequestType
{
    ImGuiDockRequestType_None = 0,
    ImGuiDockRequestType_Dock,
    ImGuiDockRequestType_Undock,
    ImGuiDockRequestType_Split                  // Split is the same as Dock but without a DockPayload
};

struct ImGuiDockRequest
{
    ImGuiDockRequestType    Type;
    ImGuiWindow*            DockTargetWindow;   // Destination/Target Window to dock into (may be a loose window or a DockNode, might be NULL in which case DockTargetNode cannot be NULL)
    ImGuiDockNode*          DockTargetNode;     // Destination/Target Node to dock into 
    ImGuiWindow*            DockPayload;        // Source/Payload window to dock (may be a loose window or a DockNode), [Optional]
    ImGuiDir                DockSplitDir;
    float                   DockSplitRatio;
    bool                    DockSplitOuter;
    ImGuiWindow*            UndockTargetWindow;
    ImGuiDockNode*          UndockTargetNode;

    ImGuiDockRequest()
    {
        Type = ImGuiDockRequestType_None;
        DockTargetWindow = DockPayload = UndockTargetWindow = NULL;
        DockTargetNode = UndockTargetNode = NULL;
        DockSplitDir = ImGuiDir_None;
        DockSplitRatio = 0.5f;
        DockSplitOuter = false;
    }
};

struct ImGuiDockPreviewData
{
    ImGuiDockNode   FutureNode;
    bool            IsDropAllowed;
    bool            IsCenterAvailable;
    bool            IsSidesAvailable;           // Hold your breath, grammar freaks..
    bool            IsSplitDirExplicit;         // Set when hovered the drop rect (vs. implicit SplitDir==None when hovered the window)
    ImGuiDockNode*  SplitNode;
    ImGuiDir        SplitDir;
    float           SplitRatio;
    ImRect          DropRectsDraw[ImGuiDir_COUNT + 1];  // May be slightly different from hit-testing drop rects used in DockNodeCalcDropRects()

    ImGuiDockPreviewData() : FutureNode(0) { IsDropAllowed = IsCenterAvailable = IsSidesAvailable = IsSplitDirExplicit = false; SplitNode = NULL; SplitDir = ImGuiDir_None; SplitRatio = 0.f; }
};

// Persistent Settings data, stored contiguously in SettingsNodes (sizeof() ~32 bytes)
struct ImGuiDockNodeSettings
{
    ImGuiID             ID;
    ImGuiID             ParentNodeID;
    ImGuiID             ParentWindowID;
    ImGuiID             SelectedWindowID;
    signed char         SplitAxis;
    char                Depth;
    ImGuiDockNodeFlags  Flags;                  // NB: We save individual flags one by one in ascii format (ImGuiDockNodeFlags_SavedFlagsMask_)
    ImVec2ih            Pos;
    ImVec2ih            Size;
    ImVec2ih            SizeRef;
    ImGuiDockNodeSettings() { ID = ParentNodeID = ParentWindowID = SelectedWindowID = 0; SplitAxis = ImGuiAxis_None; Depth = 0; Flags = ImGuiDockNodeFlags_None; }
};

struct ImGuiDockContext
{
    ImGuiStorage                    Nodes;                      // Map ID -> ImGuiDockNode*: Active nodes
    ImVector<ImGuiDockRequest>      Requests;
    ImVector<ImGuiDockNodeSettings> SettingsNodes;
    bool                            WantFullRebuild;
    ImGuiDockContext()              { WantFullRebuild = false; }
};

//-----------------------------------------------------------------------------
// Docking: Forward Declarations
//-----------------------------------------------------------------------------

namespace ImGui
{
    // ImGuiDockContext
    static ImGuiDockNode*   DockContextAddNode(ImGuiContext* ctx, ImGuiID id);
    static void             DockContextRemoveNode(ImGuiContext* ctx, ImGuiDockNode* node, bool merge_sibling_into_parent_node);
    static void             DockContextQueueNotifyRemovedNode(ImGuiContext* ctx, ImGuiDockNode* node);
    static void             DockContextProcessDock(ImGuiContext* ctx, ImGuiDockRequest* req);
    static void             DockContextProcessUndockWindow(ImGuiContext* ctx, ImGuiWindow* window, bool clear_persistent_docking_ref = true);
    static void             DockContextProcessUndockNode(ImGuiContext* ctx, ImGuiDockNode* node);
    static void             DockContextPruneUnusedSettingsNodes(ImGuiContext* ctx);
    static ImGuiDockNode*   DockContextFindNodeByID(ImGuiContext* ctx, ImGuiID id);
    static ImGuiDockNode*   DockContextBindNodeToWindow(ImGuiContext* ctx, ImGuiWindow* window);
    static void             DockContextClearNodes(ImGuiContext* ctx, ImGuiID root_id, bool clear_persistent_docking_refs);    // Use root_id==0 to clear all
    static void             DockContextBuildNodesFromSettings(ImGuiContext* ctx, ImGuiDockNodeSettings* node_settings_array, int node_settings_count);
    static void             DockContextBuildAddWindowsToNodes(ImGuiContext* ctx, ImGuiID root_id);                            // Use root_id==0 to add all

    // ImGuiDockNode
    static void             DockNodeAddWindow(ImGuiDockNode* node, ImGuiWindow* window, bool add_to_tab_bar);
    static void             DockNodeMoveWindows(ImGuiDockNode* dst_node, ImGuiDockNode* src_node);
    static void             DockNodeMoveChildNodes(ImGuiDockNode* dst_node, ImGuiDockNode* src_node);
    static ImGuiWindow*     DockNodeFindWindowByID(ImGuiDockNode* node, ImGuiID id);
    static void             DockNodeApplyPosSizeToWindows(ImGuiDockNode* node);
    static void             DockNodeRemoveWindow(ImGuiDockNode* node, ImGuiWindow* window, ImGuiID save_dock_id);
    static void             DockNodeHideHostWindow(ImGuiDockNode* node);
    static void             DockNodeUpdate(ImGuiDockNode* node);
    static void             DockNodeUpdateVisibleFlagAndInactiveChilds(ImGuiDockNode* node);
    static void             DockNodeUpdateTabBar(ImGuiDockNode* node, ImGuiWindow* host_window);
    static void             DockNodeAddTabBar(ImGuiDockNode* node);
    static void             DockNodeRemoveTabBar(ImGuiDockNode* node);
    static ImGuiID          DockNodeUpdateWindowMenu(ImGuiDockNode* node, ImGuiTabBar* tab_bar);
    static void             DockNodeUpdateVisibleFlag(ImGuiDockNode* node);
    static void             DockNodeStartMouseMovingWindow(ImGuiDockNode* node, ImGuiWindow* window);
    static bool             DockNodeIsDropAllowed(ImGuiWindow* host_window, ImGuiWindow* payload_window);
    static void             DockNodePreviewDockCalc(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* payload_window, ImGuiDockPreviewData* preview_data, bool is_explicit_target, bool is_outer_docking);
    static void             DockNodePreviewDockRender(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* payload_window, const ImGuiDockPreviewData* preview_data);
    static void             DockNodeCalcTabBarLayout(const ImGuiDockNode* node, ImRect* out_title_rect, ImRect* out_tab_bar_rect, ImVec2* out_window_menu_button_pos);
    static void             DockNodeCalcSplitRects(ImVec2& pos_old, ImVec2& size_old, ImVec2& pos_new, ImVec2& size_new, ImGuiDir dir, ImVec2 size_new_desired);
    static bool             DockNodeCalcDropRectsAndTestMousePos(const ImRect& parent, ImGuiDir dir, ImRect& out_draw, bool outer_docking, ImVec2* test_mouse_pos);
    static const char*      DockNodeGetHostWindowTitle(ImGuiDockNode* node, char* buf, int buf_size) { ImFormatString(buf, buf_size, "##DockNode_%02X", node->ID); return buf; }
    static int              DockNodeGetDepth(const ImGuiDockNode* node) { int depth = 0; while (node->ParentNode) { node = node->ParentNode; depth++; } return depth; }
    static int              DockNodeGetTabOrder(ImGuiWindow* window);

    // ImGuiDockNode tree manipulations
    static void             DockNodeTreeSplit(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiAxis split_axis, int split_first_child, float split_ratio, ImGuiDockNode* new_node);
    static void             DockNodeTreeMerge(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiDockNode* merge_lead_child);
    static void             DockNodeTreeUpdatePosSize(ImGuiDockNode* node, ImVec2 pos, ImVec2 size, bool only_write_to_marked_nodes = false);
    static void             DockNodeTreeUpdateSplitter(ImGuiDockNode* node);
    static ImGuiDockNode*   DockNodeTreeFindNodeByPos(ImGuiDockNode* node, ImVec2 pos);
    static ImGuiDockNode*   DockNodeTreeFindFallbackLeafNode(ImGuiDockNode* node);

    // Settings
    static void             DockSettingsRenameNodeReferences(ImGuiID old_node_id, ImGuiID new_node_id);
    static void             DockSettingsRemoveNodeReferences(ImGuiID* node_ids, int node_ids_count);
    static ImGuiDockNodeSettings*   DockSettingsFindNodeSettings(ImGuiContext* ctx, ImGuiID node_id);
    static void*            DockSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
    static void             DockSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
    static void             DockSettingsHandler_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockContext
//-----------------------------------------------------------------------------
// The lifetime model is different from the one of regular windows: we always create a ImGuiDockNode for each ImGuiDockNodeSettings,
// or we always hold the entire docking node tree. Nodes are frequently hidden, e.g. if the window(s) or child nodes they host are not active.
// At boot time only, we run a simple GC to remove nodes that have no references.
// Because dock node settings (which are small, contiguous structures) are always mirrored by their corresponding dock nodes (more complete structures),
// we can also very easily recreate the nodes from scratch given the settings data (this is what DockContextRebuild() does).
// This is convenient as docking reconfiguration can be implemented by mostly poking at the simpler settings data.
//-----------------------------------------------------------------------------
// - DockContextInitialize()
// - DockContextShutdown()
// - DockContextOnLoadSettings()
// - DockContextClearNodes()
// - DockContextRebuildNodes()
// - DockContextUpdateUndocking()
// - DockContextUpdateDocking()
// - DockContextFindNodeByID()
// - DockContextBindNodeToWindow()
// - DockContextGenNodeID()
// - DockContextAddNode()
// - DockContextRemoveNode()
// - ImGuiDockContextPruneNodeData
// - DockContextPruneUnusedSettingsNodes()
// - DockContextBuildNodesFromSettings()
// - DockContextBuildAddWindowsToNodes()
//-----------------------------------------------------------------------------

void ImGui::DockContextInitialize(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(g.DockContext == NULL);
    g.DockContext = IM_NEW(ImGuiDockContext)();

    // Add .ini handle for persistent docking data
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Docking";
    ini_handler.TypeHash = ImHashStr("Docking");
    ini_handler.ReadOpenFn = DockSettingsHandler_ReadOpen;
    ini_handler.ReadLineFn = DockSettingsHandler_ReadLine;
    ini_handler.WriteAllFn = DockSettingsHandler_WriteAll;
    g.SettingsHandlers.push_back(ini_handler);
}

void ImGui::DockContextShutdown(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            IM_DELETE(node);
    IM_DELETE(g.DockContext);
    g.DockContext = NULL;
}

void ImGui::DockContextOnLoadSettings(ImGuiContext* ctx)
{
    ImGuiDockContext* dc = ctx->DockContext;
    DockContextPruneUnusedSettingsNodes(ctx);
    DockContextBuildNodesFromSettings(ctx, dc->SettingsNodes.Data, dc->SettingsNodes.Size);
}

void ImGui::DockContextClearNodes(ImGuiContext* ctx, ImGuiID root_id, bool clear_persistent_docking_references)
{
    IM_UNUSED(ctx);
    IM_ASSERT(ctx == GImGui);
    DockBuilderRemoveNodeDockedWindows(root_id, clear_persistent_docking_references);
    DockBuilderRemoveNodeChildNodes(root_id);
}

// This function also acts as a defacto test to make sure we can rebuild from scratch without a glitch
void ImGui::DockContextRebuildNodes(ImGuiContext* ctx)
{
    IMGUI_DEBUG_LOG_DOCKING("DockContextRebuild()\n");
    ImGuiDockContext* dc = ctx->DockContext;
    SaveIniSettingsToMemory();
    ImGuiID root_id = 0; // Rebuild all
    DockContextClearNodes(ctx, root_id, false);
    DockContextBuildNodesFromSettings(ctx, dc->SettingsNodes.Data, dc->SettingsNodes.Size);
    DockContextBuildAddWindowsToNodes(ctx, root_id);
}

// Docking context update function, called by NewFrame()
void ImGui::DockContextUpdateUndocking(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
    {
        if (dc->Nodes.Data.Size > 0 || dc->Requests.Size > 0)
            DockContextClearNodes(ctx, 0, true);
        return;
    }

    // Setting NoSplit at runtime merges all nodes
    if (g.IO.ConfigDockingNoSplit)
        for (int n = 0; n < dc->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
                if (node->IsRootNode() && node->IsSplitNode())
                {
                    DockBuilderRemoveNodeChildNodes(node->ID);
                    //dc->WantFullRebuild = true;
                }
    
    // Process full rebuild
#if 0
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
        dc->WantFullRebuild = true;
#endif
    if (dc->WantFullRebuild)
    {
        DockContextRebuildNodes(ctx);
        dc->WantFullRebuild = false;
    }

    // Process Undocking requests (we need to process them _before_ the UpdateMouseMovingWindowNewFrame call in NewFrame)
    for (int n = 0; n < dc->Requests.Size; n++)
    {
        ImGuiDockRequest* req = &dc->Requests[n];
        if (req->Type == ImGuiDockRequestType_Undock && req->UndockTargetWindow)
            DockContextProcessUndockWindow(ctx, req->UndockTargetWindow);
        else if (req->Type == ImGuiDockRequestType_Undock && req->UndockTargetNode)
            DockContextProcessUndockNode(ctx, req->UndockTargetNode);
    }
}

// Docking context update function, called by NewFrame()
void ImGui::DockContextUpdateDocking(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    // Process Docking requests
    for (int n = 0; n < dc->Requests.Size; n++)
        if (dc->Requests[n].Type == ImGuiDockRequestType_Dock)
            DockContextProcessDock(ctx, &dc->Requests[n]);
    dc->Requests.resize(0);

    // Create windows for each automatic docking nodes
    // We can have NULL pointers when we delete nodes, but because ID are recycled this should amortize nicely (and our node count will never be very high)
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            if (node->IsFloatingNode())
                DockNodeUpdate(node);
}

static ImGuiDockNode* ImGui::DockContextFindNodeByID(ImGuiContext* ctx, ImGuiID id)
{
    return (ImGuiDockNode*)ctx->DockContext->Nodes.GetVoidPtr(id);
}

ImGuiID ImGui::DockContextGenNodeID(ImGuiContext* ctx)
{
    // Generate an ID for new node (the exact ID value doesn't matter as long as it is not already used)
    // FIXME-OPT FIXME-DOCKING: This is suboptimal, even if the node count is small enough not to be a worry. We should poke in ctx->Nodes to find a suitable ID faster.
    ImGuiID id = 0x0001;
    while (DockContextFindNodeByID(ctx, id) != NULL)
        id++;
    return id;
}

static ImGuiDockNode* ImGui::DockContextAddNode(ImGuiContext* ctx, ImGuiID id)
{
    // Generate an ID for the new node (the exact ID value doesn't matter as long as it is not already used) and add the first window.
    if (id == 0)
        id = DockContextGenNodeID(ctx);
    else
        IM_ASSERT(DockContextFindNodeByID(ctx, id) == NULL);

    // We don't set node->LastFrameAlive on construction. Nodes are always created at all time to reflect .ini settings!
    IMGUI_DEBUG_LOG_DOCKING("DockContextAddNode 0x%08X\n", id);
    ImGuiDockNode* node = IM_NEW(ImGuiDockNode)(id);
    ctx->DockContext->Nodes.SetVoidPtr(node->ID, node);
    return node;
}

static void ImGui::DockContextRemoveNode(ImGuiContext* ctx, ImGuiDockNode* node, bool merge_sibling_into_parent_node)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;

    IMGUI_DEBUG_LOG_DOCKING("DockContextRemoveNode 0x%08X\n", node->ID);
    IM_ASSERT(DockContextFindNodeByID(ctx, node->ID) == node);
    IM_ASSERT(node->ChildNodes[0] == NULL && node->ChildNodes[1] == NULL);
    IM_ASSERT(node->Windows.Size == 0);

    if (node->HostWindow)
        node->HostWindow->DockNodeAsHost = NULL;

    ImGuiDockNode* parent_node = node->ParentNode;
    const bool merge = (merge_sibling_into_parent_node && parent_node != NULL);
    if (merge)
    {
        IM_ASSERT(parent_node->ChildNodes[0] == node || parent_node->ChildNodes[1] == node);
        ImGuiDockNode* sibling_node = (parent_node->ChildNodes[0] == node ? parent_node->ChildNodes[1] : parent_node->ChildNodes[0]);
        DockNodeTreeMerge(&g, parent_node, sibling_node);
    }
    else
    {
        for (int n = 0; parent_node && n < IM_ARRAYSIZE(parent_node->ChildNodes); n++)
            if (parent_node->ChildNodes[n] == node)
                node->ParentNode->ChildNodes[n] = NULL;
        dc->Nodes.SetVoidPtr(node->ID, NULL);
        IM_DELETE(node);
    }
}

static int IMGUI_CDECL DockNodeComparerDepthMostFirst(const void* lhs, const void* rhs)
{
    const ImGuiDockNode* a = *(const ImGuiDockNode* const*)lhs;
    const ImGuiDockNode* b = *(const ImGuiDockNode* const*)rhs;
    return ImGui::DockNodeGetDepth(b) - ImGui::DockNodeGetDepth(a);
}

// Pre C++0x doesn't allow us to use a function-local type (without linkage) as template parameter, so we moved this here.
struct ImGuiDockContextPruneNodeData
{
    int         CountWindows, CountChildWindows, CountChildNodes;
    ImGuiID     RootID;
    ImGuiDockContextPruneNodeData() { CountWindows = CountChildWindows = CountChildNodes = 0; RootID = 0; }
};

// Garbage collect unused nodes (run once at init time)
static void ImGui::DockContextPruneUnusedSettingsNodes(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    IM_ASSERT(g.Windows.Size == 0);

    ImPool<ImGuiDockContextPruneNodeData> pool;
    pool.Reserve(dc->SettingsNodes.Size);

    // Count child nodes and compute RootID
    for (int settings_n = 0; settings_n < dc->SettingsNodes.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->SettingsNodes[settings_n];
        ImGuiDockContextPruneNodeData* parent_data = settings->ParentNodeID ? pool.GetByKey(settings->ParentNodeID) : 0;
        pool.GetOrAddByKey(settings->ID)->RootID = parent_data ? parent_data->RootID : settings->ID;
        if (settings->ParentNodeID)
            pool.GetOrAddByKey(settings->ParentNodeID)->CountChildNodes++;
    }

    // Count reference to dock ids from dockspaces
    // We track the 'auto-DockNode <- manual-Window <- manual-DockSpace' in order to avoid 'auto-DockNode' being ditched by DockContextPruneUnusedSettingsNodes()
    for (int settings_n = 0; settings_n < dc->SettingsNodes.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->SettingsNodes[settings_n];
        if (settings->ParentWindowID != 0)
            if (ImGuiWindowSettings* window_settings = FindWindowSettings(settings->ParentWindowID))
                if (window_settings->DockId)
                    if (ImGuiDockContextPruneNodeData* data = pool.GetByKey(window_settings->DockId))
                        data->CountChildNodes++;
    }

    // Count reference to dock ids from window settings
    // We guard against the possibility of an invalid .ini file (RootID may point to a missing node)
    for (int settings_n = 0; settings_n < g.SettingsWindows.Size; settings_n++)
        if (ImGuiID dock_id = g.SettingsWindows[settings_n].DockId)
            if (ImGuiDockContextPruneNodeData* data = pool.GetByKey(dock_id))
            {
                data->CountWindows++;
                if (ImGuiDockContextPruneNodeData* data_root = (data->RootID == dock_id) ? data : pool.GetByKey(data->RootID))
                    data_root->CountChildWindows++;
            }

    // Prune
    for (int settings_n = 0; settings_n < dc->SettingsNodes.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->SettingsNodes[settings_n];
        ImGuiDockContextPruneNodeData* data = pool.GetByKey(settings->ID);
        if (data->CountWindows > 1)
            continue;
        ImGuiDockContextPruneNodeData* data_root = (data->RootID == settings->ID) ? data : pool.GetByKey(data->RootID);

        bool remove = false;
        remove |= (data->CountWindows == 1 && settings->ParentNodeID == 0 && data->CountChildNodes == 0 && !(settings->Flags & ImGuiDockNodeFlags_CentralNode));  // Floating root node with only 1 window
        remove |= (data->CountWindows == 0 && settings->ParentNodeID == 0 && data->CountChildNodes == 0); // Leaf nodes with 0 window
        remove |= (data_root->CountChildWindows == 0);
        if (remove)
        {
            IMGUI_DEBUG_LOG_DOCKING("DockContextPruneUnusedSettingsNodes: Prune 0x%08X\n", settings->ID);
            DockSettingsRemoveNodeReferences(&settings->ID, 1);
            settings->ID = 0;
        }
    }
}

static void ImGui::DockContextBuildNodesFromSettings(ImGuiContext* ctx, ImGuiDockNodeSettings* node_settings_array, int node_settings_count)
{
    // Build nodes
    for (int node_n = 0; node_n < node_settings_count; node_n++)
    {
        ImGuiDockNodeSettings* settings = &node_settings_array[node_n];
        if (settings->ID == 0)
            continue;
        ImGuiDockNode* node = DockContextAddNode(ctx, settings->ID);
        node->ParentNode = settings->ParentNodeID ? DockContextFindNodeByID(ctx, settings->ParentNodeID) : NULL;
        node->Pos = ImVec2(settings->Pos.x, settings->Pos.y);
        node->Size = ImVec2(settings->Size.x, settings->Size.y);
        node->SizeRef = ImVec2(settings->SizeRef.x, settings->SizeRef.y);
        node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_DockNode;
        if (node->ParentNode && node->ParentNode->ChildNodes[0] == NULL)
            node->ParentNode->ChildNodes[0] = node;
        else if (node->ParentNode && node->ParentNode->ChildNodes[1] == NULL)
            node->ParentNode->ChildNodes[1] = node;
        node->SelectedTabID = settings->SelectedWindowID;
        node->SplitAxis = settings->SplitAxis;
        node->LocalFlags |= (settings->Flags & ImGuiDockNodeFlags_SavedFlagsMask_);

        // Bind host window immediately if it already exist (in case of a rebuild)
        // This is useful as the RootWindowForTitleBarHighlight links necessary to highlight the currently focused node requires node->HostWindow to be set.
        char host_window_title[20];
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        node->HostWindow = FindWindowByName(DockNodeGetHostWindowTitle(root_node, host_window_title, IM_ARRAYSIZE(host_window_title)));
    }
}

void ImGui::DockContextBuildAddWindowsToNodes(ImGuiContext* ctx, ImGuiID root_id)
{
    // Rebind all windows to nodes (they can also lazily rebind but we'll have a visible glitch during the first frame)
    ImGuiContext& g = *ctx;
    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        if (window->DockId == 0 || window->LastFrameActive < g.FrameCount - 1)
            continue;
        if (window->DockNode != NULL)
            continue;

        ImGuiDockNode* node = DockContextFindNodeByID(ctx, window->DockId);
        IM_ASSERT(node != NULL);   // This should have been called after DockContextBuildNodesFromSettings()
        if (root_id == 0 || DockNodeGetRootNode(node)->ID == root_id)
            DockNodeAddWindow(node, window, true);
    }
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockContext Docking/Undocking functions
//-----------------------------------------------------------------------------
// - DockContextQueueDock()
// - DockContextQueueUndockWindow()
// - DockContextQueueUndockNode()
// - DockContextQueueNotifyRemovedNode()
// - DockContextProcessDock()
// - DockContextProcessUndockWindow()
// - DockContextProcessUndockNode()
// - DockContextCalcDropPosForDocking()
//-----------------------------------------------------------------------------

void ImGui::DockContextQueueDock(ImGuiContext* ctx, ImGuiWindow* target, ImGuiDockNode* target_node, ImGuiWindow* payload, ImGuiDir split_dir, float split_ratio, bool split_outer)
{
    IM_ASSERT(target != payload);
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Dock;
    req.DockTargetWindow = target;
    req.DockTargetNode = target_node;
    req.DockPayload = payload;
    req.DockSplitDir = split_dir;
    req.DockSplitRatio = split_ratio;
    req.DockSplitOuter = split_outer;
    ctx->DockContext->Requests.push_back(req);
}

void ImGui::DockContextQueueUndockWindow(ImGuiContext* ctx, ImGuiWindow* window)
{
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Undock;
    req.UndockTargetWindow = window;
    ctx->DockContext->Requests.push_back(req);
}

void ImGui::DockContextQueueUndockNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Undock;
    req.UndockTargetNode = node;
    ctx->DockContext->Requests.push_back(req);
}

void ImGui::DockContextQueueNotifyRemovedNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    ImGuiDockContext* dc = ctx->DockContext;
    for (int n = 0; n < dc->Requests.Size; n++)
        if (dc->Requests[n].DockTargetNode == node)
            dc->Requests[n].Type = ImGuiDockRequestType_None;
}

