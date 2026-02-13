// Create an explicit dockspace node within an existing window. Also expose dock node flags and creates a CentralNode by default.
// The Central Node is always displayed even when empty and shrink/extend according to the requested size of its neighbors.
void ImGui::DockSpace(ImGuiID id, const ImVec2& size_arg, ImGuiDockNodeFlags flags, const ImGuiWindowClass* window_class)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    ImGuiWindow* window = GetCurrentWindow();
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    IM_ASSERT((flags & ImGuiDockNodeFlags_DockSpace) == 0);
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    if (!node)
    {
        IMGUI_DEBUG_LOG_DOCKING("DockSpace: dockspace node 0x%08X created\n", id);
        node = DockContextAddNode(ctx, id);
        node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
    }
    if (window_class && window_class->ClassId != node->WindowClass.ClassId)
        IMGUI_DEBUG_LOG_DOCKING("DockSpace: dockspace node 0x%08X: setup WindowClass 0x%08X -> 0x%08X\n", id, node->WindowClass.ClassId, window_class->ClassId);
    node->SharedFlags = flags;
    node->WindowClass = window_class ? *window_class : ImGuiWindowClass();

    // When a DockSpace transitioned form implicit to explicit this may be called a second time
    // It is possible that the node has already been claimed by a docked window which appeared before the DockSpace() node, so we overwrite IsDockSpace again.
    if (node->LastFrameActive == g.FrameCount && !(flags & ImGuiDockNodeFlags_KeepAliveOnly))
    {
        IM_ASSERT(node->IsDockSpace() == false && "Cannot call DockSpace() twice a frame with the same ID");
        node->LocalFlags |= ImGuiDockNodeFlags_DockSpace;
        return;
    }
    node->LocalFlags |= ImGuiDockNodeFlags_DockSpace;

    // Keep alive mode, this is allow windows docked into this node so stay docked even if they are not visible
    if (flags & ImGuiDockNodeFlags_KeepAliveOnly)
    {
        node->LastFrameAlive = g.FrameCount;
        return;
    }

    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);
    IM_ASSERT(size.x > 0.0f && size.y > 0.0f);

    node->Pos = window->DC.CursorPos;
    node->Size = node->SizeRef = size;
    SetNextWindowPos(node->Pos);
    SetNextWindowSize(node->Size);
    g.NextWindowData.PosUndock = false;

    // FIXME-DOCKING: Why do we need a child window to host a dockspace, could we host it in the existing window?
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_DockNodeHost;
    window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	ImGuiWindowFlags oldwindow_flags = window_flags;
	window_flags |= ImGuiWindowFlags_NoBackground;

    char title[256];
    ImFormatString(title, IM_ARRAYSIZE(title), "%s/DockSpace_%08X", window->Name, id);

	// Wicked engine overrides window backdrop color so needs the child backdrop color to work
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    Begin(title, NULL, window_flags);
    PopStyleVar();
	// Wicked engine overrides window backdrop color so needs the child backdrop color to work

	window_flags = oldwindow_flags;

    ImGuiWindow* host_window = g.CurrentWindow;
    host_window->DockNodeAsHost = node;
    host_window->ChildId = window->GetID(title);
    node->HostWindow = host_window;
    node->OnlyNodeWithWindows = NULL;

    IM_ASSERT(node->IsRootNode());
    DockNodeUpdate(node);

    End();
}

// Tips: Use with ImGuiDockNodeFlags_PassthruCentralNode!
// The limitation with this call is that your window won't have a menu bar. 
// Even though we could pass window flags, it would also require the user to be able to call BeginMenuBar() somehow meaning we can't Begin/End in a single function.
// So if you want a menu bar you need to repeat this code manually ourselves. As with advanced other Docking API, we may change this function signature.
ImGuiID ImGui::DockSpaceOverViewport(ImGuiViewport* viewport, ImGuiDockNodeFlags dockspace_flags, const ImGuiWindowClass* window_class)
{
    if (viewport == NULL)
        viewport = GetMainViewport();

    SetNextWindowPos(viewport->Pos);
    SetNextWindowSize(viewport->Size);
    SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        host_window_flags |= ImGuiWindowFlags_NoBackground;

    char label[32];
    ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);

    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    Begin(label, NULL, host_window_flags);
    PopStyleVar(3);

    ImGuiID dockspace_id = GetID("DockSpace");
    DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, window_class);
    End();

    return dockspace_id;
}

//-----------------------------------------------------------------------------
// Docking: Builder Functions
//-----------------------------------------------------------------------------
// Very early end-user API to manipulate dock nodes.
// Only available in imgui_internal.h. Expect this API to change/break!
// It is expected that those functions are all called _before_ the dockspace node submission.
//-----------------------------------------------------------------------------
// - DockBuilderDockWindow()
// - DockBuilderGetNode()
// - DockBuilderSetNodePos()
// - DockBuilderSetNodeSize()
// - DockBuilderAddNode()
// - DockBuilderRemoveNode()
// - DockBuilderRemoveNodeChildNodes()
// - DockBuilderRemoveNodeDockedWindows()
// - DockBuilderSplitNode()
// - DockBuilderCopyNodeRec()
// - DockBuilderCopyNode()
// - DockBuilderCopyWindowSettings()
// - DockBuilderCopyDockSpace()
// - DockBuilderFinish()
//-----------------------------------------------------------------------------

void ImGui::DockBuilderDockWindow(const char* window_name, ImGuiID node_id)
{
    // We don't preserve relative order of multiple docked windows (by clearing DockOrder back to -1)
    ImGuiID window_id = ImHashStr(window_name);
    if (ImGuiWindow* window = FindWindowByID(window_id))
    {
        // Apply to created window
        SetWindowDock(window, node_id, ImGuiCond_Always);
        window->DockOrder = -1;
    }
    else
    {
        // Apply to settings
        ImGuiWindowSettings* settings = FindWindowSettings(window_id);
        if (settings == NULL)
            settings = CreateNewWindowSettings(window_name);
        settings->DockId = node_id;
        settings->DockOrder = -1;
    }
}

ImGuiDockNode* ImGui::DockBuilderGetNode(ImGuiID node_id)
{
    ImGuiContext* ctx = GImGui;
    return DockContextFindNodeByID(ctx, node_id);
}

void ImGui::DockBuilderSetNodePos(ImGuiID node_id, ImVec2 pos)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    node->Pos = pos;
    node->AuthorityForPos = ImGuiDataAuthority_DockNode;
}

void ImGui::DockBuilderSetNodeSize(ImGuiID node_id, ImVec2 size)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    IM_ASSERT(size.x > 0.0f && size.y > 0.0f);
    node->Size = node->SizeRef = size;
    node->AuthorityForSize = ImGuiDataAuthority_DockNode;
}

// Make sure to use the ImGuiDockNodeFlags_DockSpace flag to create a dockspace node! Otherwise this will create a floating node!
// - Floating node: you can then call DockBuilderSetNodePos()/DockBuilderSetNodeSize() to position and size the floating node.
// - Dockspace node: calling DockBuilderSetNodePos() is unnecessary.
// - If you intend to split a node immediately after creation using DockBuilderSplitNode(), make sure to call DockBuilderSetNodeSize() beforehand!
//   For various reason, the splitting code currently needs a base size otherwise space may not be allocated as precisely as you would expect.
// - Use (id == 0) to let the system allocate a node identifier.
ImGuiID ImGui::DockBuilderAddNode(ImGuiID id, ImGuiDockNodeFlags flags)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = NULL;
    if (flags & ImGuiDockNodeFlags_DockSpace)
    {
        DockSpace(id, ImVec2(0, 0), (flags & ~ImGuiDockNodeFlags_DockSpace) | ImGuiDockNodeFlags_KeepAliveOnly);
        node = DockContextFindNodeByID(ctx, id);
    }
    else
    {
        if (id != 0)
            node = DockContextFindNodeByID(ctx, id);
        if (!node)
            node = DockContextAddNode(ctx, id);
        node->LocalFlags = flags;
    }
    node->LastFrameAlive = ctx->FrameCount;   // Set this otherwise BeginDocked will undock during the same frame.
    return node->ID;
}

void ImGui::DockBuilderRemoveNode(ImGuiID node_id)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    DockBuilderRemoveNodeDockedWindows(node_id, true);
    DockBuilderRemoveNodeChildNodes(node_id);
    if (node->IsCentralNode() && node->ParentNode)
        node->ParentNode->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
    DockContextRemoveNode(ctx, node, true);
}

void ImGui::DockBuilderRemoveNodeChildNodes(ImGuiID root_id)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockContext* dc = ctx->DockContext;

    ImGuiDockNode* root_node = root_id ? DockContextFindNodeByID(ctx, root_id) : NULL;
    if (root_id && root_node == NULL)
        return;
    bool has_central_node = false;

    ImGuiDataAuthority backup_root_node_authority_for_pos = root_node ? root_node->AuthorityForPos : ImGuiDataAuthority_Auto;
    ImGuiDataAuthority backup_root_node_authority_for_size = root_node ? root_node->AuthorityForSize : ImGuiDataAuthority_Auto;

    // Process active windows
    ImVector<ImGuiDockNode*> nodes_to_remove;
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
        {
            bool want_removal = (root_id == 0) || (node->ID != root_id && DockNodeGetRootNode(node)->ID == root_id);
            if (want_removal)
            {
                if (node->IsCentralNode())
                    has_central_node = true;
                if (root_id != 0)
                    DockContextQueueNotifyRemovedNode(ctx, node);
                if (root_node)
                    DockNodeMoveWindows(root_node, node);
                nodes_to_remove.push_back(node);
            }
        }

    // DockNodeMoveWindows->DockNodeAddWindow will normally set those when reaching two windows (which is only adequate during interactive merge)
    // Make sure we don't lose our current pos/size. (FIXME-DOCK: Consider tidying up that code in DockNodeAddWindow instead)
    if (root_node)
    {
        root_node->AuthorityForPos = backup_root_node_authority_for_pos;
        root_node->AuthorityForSize = backup_root_node_authority_for_size;
    }

    // Apply to settings
    for (int settings_n = 0; settings_n < ctx->SettingsWindows.Size; settings_n++)
        if (ImGuiID window_settings_dock_id = ctx->SettingsWindows[settings_n].DockId)
            for (int n = 0; n < nodes_to_remove.Size; n++)
                if (nodes_to_remove[n]->ID == window_settings_dock_id)
                {
                    ctx->SettingsWindows[settings_n].DockId = root_id;
                    break;
                }

    // Not really efficient, but easier to destroy a whole hierarchy considering DockContextRemoveNode is attempting to merge nodes
    if (nodes_to_remove.Size > 1)
        ImQsort(nodes_to_remove.Data, nodes_to_remove.Size, sizeof(ImGuiDockNode*), DockNodeComparerDepthMostFirst);
    for (int n = 0; n < nodes_to_remove.Size; n++)
        DockContextRemoveNode(ctx, nodes_to_remove[n], false);

    if (root_id == 0)
    {
        dc->Nodes.Clear();
        dc->Requests.clear();
    }
    else if (has_central_node)
    {
        root_node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
        root_node->CentralNode = root_node;
    }
}

void ImGui::DockBuilderRemoveNodeDockedWindows(ImGuiID root_id, bool clear_persistent_docking_references)
{
    // Clear references in settings
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    if (clear_persistent_docking_references)
    {
        for (int n = 0; n < g.SettingsWindows.Size; n++)
        {
            ImGuiWindowSettings* settings = &g.SettingsWindows[n];
            bool want_removal = (root_id == 0) || (settings->DockId == root_id);
            if (!want_removal && settings->DockId != 0)
                if (ImGuiDockNode* node = DockContextFindNodeByID(ctx, settings->DockId))
                    if (DockNodeGetRootNode(node)->ID == root_id)
                        want_removal = true;
            if (want_removal)
                settings->DockId = 0;
        }
    }

    // Clear references in windows
    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        bool want_removal = (root_id == 0) || (window->DockNode && DockNodeGetRootNode(window->DockNode)->ID == root_id) || (window->DockNodeAsHost && window->DockNodeAsHost->ID == root_id);
        if (want_removal)
        {
            const ImGuiID backup_dock_id = window->DockId;
            DockContextProcessUndockWindow(ctx, window, clear_persistent_docking_references);
            if (!clear_persistent_docking_references)
                IM_ASSERT(window->DockId == backup_dock_id);
        }
    }
}

// If 'out_id_at_dir' or 'out_id_at_opposite_dir' are non NULL, the function will write out the ID of the two new nodes created.
// Return value is ID of the node at the specified direction, so same as (*out_id_at_dir) if that pointer is set.
// FIXME-DOCK: We are not exposing nor using split_outer. 
ImGuiID ImGui::DockBuilderSplitNode(ImGuiID id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_at_dir, ImGuiID* out_id_at_opposite_dir)
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(split_dir != ImGuiDir_None);
    IMGUI_DEBUG_LOG_DOCKING("DockBuilderSplitNode node 0x%08X, split_dir %d\n", id, split_dir);

    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    if (node == NULL)
    {
        IM_ASSERT(node != NULL);
        return 0;
    }

	// leelee, allow this as it lets me split the tutorial window to have separate video playback panel
    //IM_ASSERT(!node->IsSplitNode()); // Assert if already Split

    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Split;
    req.DockTargetWindow = NULL;
    req.DockTargetNode = node;
    req.DockPayload = NULL;
    req.DockSplitDir = split_dir;
    req.DockSplitRatio = ImSaturate((split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? size_ratio_for_node_at_dir : 1.0f - size_ratio_for_node_at_dir);
    req.DockSplitOuter = false;
    DockContextProcessDock(ctx, &req);

    ImGuiID id_at_dir = node->ChildNodes[(split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 0 : 1]->ID;
    ImGuiID id_at_opposite_dir = node->ChildNodes[(split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 1 : 0]->ID;
    if (out_id_at_dir)
        *out_id_at_dir = id_at_dir;
    if (out_id_at_opposite_dir)
        *out_id_at_opposite_dir = id_at_opposite_dir;
    return id_at_dir;
}

static ImGuiDockNode* DockBuilderCopyNodeRec(ImGuiDockNode* src_node, ImGuiID dst_node_id_if_known, ImVector<ImGuiID>* out_node_remap_pairs)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* dst_node = ImGui::DockContextAddNode(ctx, dst_node_id_if_known);
    dst_node->SharedFlags = src_node->SharedFlags;
    dst_node->LocalFlags = src_node->LocalFlags;
    dst_node->Pos = src_node->Pos;
    dst_node->Size = src_node->Size;
    dst_node->SizeRef = src_node->SizeRef;
    dst_node->SplitAxis = src_node->SplitAxis;

    out_node_remap_pairs->push_back(src_node->ID);
    out_node_remap_pairs->push_back(dst_node->ID);

    for (int child_n = 0; child_n < IM_ARRAYSIZE(src_node->ChildNodes); child_n++)
        if (src_node->ChildNodes[child_n])
        {
            dst_node->ChildNodes[child_n] = DockBuilderCopyNodeRec(src_node->ChildNodes[child_n], 0, out_node_remap_pairs);
            dst_node->ChildNodes[child_n]->ParentNode = dst_node;
        }

    IMGUI_DEBUG_LOG_DOCKING("Fork node %08X -> %08X (%d childs)\n", src_node->ID, dst_node->ID, dst_node->IsSplitNode() ? 2 : 0);
    return dst_node;
}

void ImGui::DockBuilderCopyNode(ImGuiID src_node_id, ImGuiID dst_node_id, ImVector<ImGuiID>* out_node_remap_pairs)
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(src_node_id != 0);
    IM_ASSERT(dst_node_id != 0);
    IM_ASSERT(out_node_remap_pairs != NULL);

    ImGuiDockNode* src_node = DockContextFindNodeByID(ctx, src_node_id);
    IM_ASSERT(src_node != NULL);

    out_node_remap_pairs->clear();
    DockBuilderRemoveNode(dst_node_id);
    DockBuilderCopyNodeRec(src_node, dst_node_id, out_node_remap_pairs);

    IM_ASSERT((out_node_remap_pairs->Size % 2) == 0);
}

void ImGui::DockBuilderCopyWindowSettings(const char* src_name, const char* dst_name)
{
    ImGuiWindow* src_window = FindWindowByName(src_name);
    if (src_window == NULL)
        return;
    if (ImGuiWindow* dst_window = FindWindowByName(dst_name))
    {
        dst_window->Pos = src_window->Pos;
        dst_window->Size = src_window->Size;
        dst_window->SizeFull = src_window->SizeFull;
        dst_window->Collapsed = src_window->Collapsed;
    }
    else if (ImGuiWindowSettings* dst_settings = FindOrCreateWindowSettings(dst_name))
    {
        ImVec2ih window_pos_2ih = ImVec2ih(src_window->Pos);
        if (src_window->ViewportId != 0 && src_window->ViewportId != IMGUI_VIEWPORT_DEFAULT_ID)
        {
            dst_settings->ViewportPos = window_pos_2ih;
            dst_settings->ViewportId = src_window->ViewportId;
            dst_settings->Pos = ImVec2ih(0, 0);
        }
        else
        {
            dst_settings->Pos = window_pos_2ih;
        }
        dst_settings->Size = ImVec2ih(src_window->SizeFull);
        dst_settings->Collapsed = src_window->Collapsed;
    }
}

// FIXME: Will probably want to change this signature, in particular how the window remapping pairs are passed.
void ImGui::DockBuilderCopyDockSpace(ImGuiID src_dockspace_id, ImGuiID dst_dockspace_id, ImVector<const char*>* in_window_remap_pairs)
{
    IM_ASSERT(src_dockspace_id != 0);
    IM_ASSERT(dst_dockspace_id != 0);
    IM_ASSERT(in_window_remap_pairs != NULL);
    IM_ASSERT((in_window_remap_pairs->Size % 2) == 0);

    // Duplicate entire dock
    // FIXME: When overwriting dst_dockspace_id, windows that aren't part of our dockspace window class but that are docked in a same node will be split apart,
    // whereas we could attempt to at least keep them together in a new, same floating node.
    ImVector<ImGuiID> node_remap_pairs;
    DockBuilderCopyNode(src_dockspace_id, dst_dockspace_id, &node_remap_pairs);

    // Attempt to transition all the upcoming windows associated to dst_dockspace_id into the newly created hierarchy of dock nodes
    // (The windows associated to src_dockspace_id are staying in place)
    ImVector<ImGuiID> src_windows;
    for (int remap_window_n = 0; remap_window_n < in_window_remap_pairs->Size; remap_window_n += 2)
    {
        const char* src_window_name = (*in_window_remap_pairs)[remap_window_n];
        const char* dst_window_name = (*in_window_remap_pairs)[remap_window_n + 1];
        ImGuiID src_window_id = ImHashStr(src_window_name);
        src_windows.push_back(src_window_id);

        // Search in the remapping tables
        ImGuiID src_dock_id = 0;
        if (ImGuiWindow* src_window = FindWindowByID(src_window_id))
            src_dock_id = src_window->DockId;
        else if (ImGuiWindowSettings* src_window_settings = FindWindowSettings(src_window_id))
            src_dock_id = src_window_settings->DockId;
        ImGuiID dst_dock_id = 0;
        for (int dock_remap_n = 0; dock_remap_n < node_remap_pairs.Size; dock_remap_n += 2)
            if (node_remap_pairs[dock_remap_n] == src_dock_id)
            {
                dst_dock_id = node_remap_pairs[dock_remap_n + 1];
                //node_remap_pairs[dock_remap_n] = node_remap_pairs[dock_remap_n + 1] = 0; // Clear
                break;
            }

        if (dst_dock_id != 0)
        {
            // Docked windows gets redocked into the new node hierarchy. 
            IMGUI_DEBUG_LOG_DOCKING("Remap live window '%s' 0x%08X -> '%s' 0x%08X\n", src_window_name, src_dock_id, dst_window_name, dst_dock_id);
            DockBuilderDockWindow(dst_window_name, dst_dock_id);
        }
        else
        {
            // Floating windows gets their settings transferred (regardless of whether the new window already exist or not)
            // When this is leading to a Copy and not a Move, we would get two overlapping floating windows. Could we possibly dock them together?
            IMGUI_DEBUG_LOG_DOCKING("Remap window settings '%s' -> '%s'\n", src_window_name, dst_window_name);
            DockBuilderCopyWindowSettings(src_window_name, dst_window_name);
        }
    }

    // Anything else in the source nodes of 'node_remap_pairs' are windows that were docked in src_dockspace_id but are not owned by it (unaffiliated windows, e.g. "ImGui Demo")
    // Find those windows and move to them to the cloned dock node. This may be optional?
    for (int dock_remap_n = 0; dock_remap_n < node_remap_pairs.Size; dock_remap_n += 2)
        if (ImGuiID src_dock_id = node_remap_pairs[dock_remap_n])
        {
            ImGuiID dst_dock_id = node_remap_pairs[dock_remap_n + 1];
            ImGuiDockNode* node = DockBuilderGetNode(src_dock_id);
            for (int window_n = 0; window_n < node->Windows.Size; window_n++)
            {
                ImGuiWindow* window = node->Windows[window_n];
                if (src_windows.contains(window->ID))
                    continue;

                // Docked windows gets redocked into the new node hierarchy. 
                IMGUI_DEBUG_LOG_DOCKING("Remap window '%s' %08X -> %08X\n", window->Name, src_dock_id, dst_dock_id);
                DockBuilderDockWindow(window->Name, dst_dock_id);
            }
        }
}

void ImGui::DockBuilderFinish(ImGuiID root_id)
{
    ImGuiContext* ctx = GImGui;
    //DockContextRebuild(ctx);
    DockContextBuildAddWindowsToNodes(ctx, root_id);
}

//-----------------------------------------------------------------------------
// Docking: Begin/End Support Functions (called from Begin/End)
//-----------------------------------------------------------------------------
// - GetWindowAlwaysWantOwnTabBar()
// - BeginDocked()
// - BeginAsDockableDragDropSource()
// - BeginAsDockableDragDropTarget()
//-----------------------------------------------------------------------------

bool ImGui::GetWindowAlwaysWantOwnTabBar(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.IO.ConfigDockingAlwaysTabBar || window->WindowClass.DockingAlwaysTabBar)
        if ((window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking)) == 0)
            if (!window->IsFallbackWindow)    // We don't support AlwaysTabBar on the fallback/implicit window to avoid unused dock-node overhead/noise
                return true;
    return false;
}

static ImGuiDockNode* ImGui::DockContextBindNodeToWindow(ImGuiContext* ctx, ImGuiWindow* window)
{
    ImGuiContext& g = *ctx;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, window->DockId);
    IM_ASSERT(window->DockNode == NULL);

    // We should not be docking into a split node (SetWindowDock should avoid this)
    if (node && node->IsSplitNode())
    {
        DockContextProcessUndockWindow(ctx, window);
        return NULL;
    }

    // Create node
    if (node == NULL)
    {
        node = DockContextAddNode(ctx, window->DockId);
        node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Window;
        node->LastFrameAlive = g.FrameCount;
    }

    // If the node just turned visible and is part of a hierarchy, it doesn't have a Size assigned by DockNodeTreeUpdatePosSize() yet,
    // so we're forcing a Pos/Size update from the first ancestor that is already visible (often it will be the root node).
    // If we don't do this, the window will be assigned a zero-size on its first frame, which won't ideally warm up the layout.
    // This is a little wonky because we don't normally update the Pos/Size of visible node mid-frame.
    if (!node->IsVisible)
    {
        ImGuiDockNode* ancestor_node = node;
        while (!ancestor_node->IsVisible)
        {
            ancestor_node->IsVisible = true;
            ancestor_node->MarkedForPosSizeWrite = true;
            if (ancestor_node->ParentNode)
                ancestor_node = ancestor_node->ParentNode;
        }
        IM_ASSERT(ancestor_node->Size.x > 0.0f && ancestor_node->Size.y > 0.0f);
        DockNodeTreeUpdatePosSize(ancestor_node, ancestor_node->Pos, ancestor_node->Size, true);
    }

    // Add window to node
    DockNodeAddWindow(node, window, true);
    IM_ASSERT(node == window->DockNode);
    return node;
}

void ImGui::BeginDocked(ImGuiWindow* window, bool* p_open)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;

    const bool auto_dock_node = GetWindowAlwaysWantOwnTabBar(window);
    if (auto_dock_node)
    {
        if (window->DockId == 0)
        {
            IM_ASSERT(window->DockNode == NULL);
            window->DockId = DockContextGenNodeID(ctx);
        }
    }
    else
    {
        // Calling SetNextWindowPos() undock windows by default (by setting PosUndock)
        bool want_undock = false;
        want_undock |= (window->Flags & ImGuiWindowFlags_NoDocking) != 0;
        want_undock |= (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) && (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) && g.NextWindowData.PosUndock;
        if (want_undock)
        {
            DockContextProcessUndockWindow(ctx, window);
            return;
        }
    }

    // Bind to our dock node
    ImGuiDockNode* node = window->DockNode;
    if (node != NULL)
        IM_ASSERT(window->DockId == node->ID);
    if (window->DockId != 0 && node == NULL)
    {
        node = DockContextBindNodeToWindow(ctx, window);
        if (node == NULL)
            return;
    }

#if 0
    // Undock if the ImGuiDockNodeFlags_NoDockingInCentralNode got set
    if (node->IsCentralNode && (node->Flags & ImGuiDockNodeFlags_NoDockingInCentralNode))
    {
        DockContextProcessUndockWindow(ctx, window);
        return;
    }
#endif

    // Undock if our dockspace node disappeared
    // Note how we are testing for LastFrameAlive and NOT LastFrameActive. A DockSpace node can be maintained alive while being inactive with ImGuiDockNodeFlags_KeepAliveOnly.
    if (node->LastFrameAlive < g.FrameCount)
    {
        // If the window has been orphaned, transition the docknode to an implicit node processed in DockContextUpdateDocking()
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        if (root_node->LastFrameAlive < g.FrameCount)
        {
            DockContextProcessUndockWindow(ctx, window);
        }
        else
        {
            window->DockIsActive = true;
            window->DockTabIsVisible = false;
        }
        return;
    }

    // Fast path return. It is common for windows to hold on a persistent DockId but be the only visible window,
    // and never create neither a host window neither a tab bar.
    // FIXME-DOCK: replace ->HostWindow NULL compare with something more explicit (~was initially intended as a first frame test)
    if (node->HostWindow == NULL)
    {
        window->DockIsActive = (node->State == ImGuiDockNodeState_HostWindowHiddenBecauseWindowsAreResizing);
        window->DockTabIsVisible = false;
        return;
    }

    IM_ASSERT(node->HostWindow);
    IM_ASSERT(node->IsLeafNode());
    IM_ASSERT(node->Size.x > 0.0f && node->Size.y > 0.0f);
    node->State = ImGuiDockNodeState_HostWindowVisible;

    // Undock if we are submitted earlier than the host window
    if (window->BeginOrderWithinContext < node->HostWindow->BeginOrderWithinContext)
    {
        DockContextProcessUndockWindow(ctx, window);
        return;
    }

    // Position window
    SetNextWindowPos(node->Pos);
    SetNextWindowSize(node->Size);
    g.NextWindowData.PosUndock = false; // Cancel implicit undocking of SetNextWindowPos()
    window->DockIsActive = true;
    window->DockTabIsVisible = false;
    if (node->SharedFlags & ImGuiDockNodeFlags_KeepAliveOnly)
        return;

    // When the window is selected we mark it as visible.
    if (node->VisibleWindow == window)
        window->DockTabIsVisible = true;

    // Update window flag
    IM_ASSERT((window->Flags & ImGuiWindowFlags_ChildWindow) == 0);
    window->Flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoResize;
    if (node->IsHiddenTabBar() || node->IsNoTabBar())
        window->Flags |= ImGuiWindowFlags_NoTitleBar;
    else
        window->Flags &= ~ImGuiWindowFlags_NoTitleBar;      // Clear the NoTitleBar flag in case the user set it: confusingly enough we need a title bar height so we are correctly offset, but it won't be displayed!

    // Save new dock order only if the tab bar has been visible once.
    // This allows multiple windows to be created in the same frame and have their respective dock orders preserved.
    if (node->TabBar && node->TabBar->CurrFrameVisible != -1)
        window->DockOrder = (short)DockNodeGetTabOrder(window);

    if ((node->WantCloseAll || node->WantCloseTabID == window->ID) && p_open != NULL)
        *p_open = false;

    // Update ChildId to allow returning from Child to Parent with Escape
    ImGuiWindow* parent_window = window->DockNode->HostWindow;
    window->ChildId = parent_window->GetID(window->Name);
}

void ImGui::BeginAsDockableDragDropSource(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == window->MoveId);

    window->DC.LastItemId = window->MoveId;
    window = window->RootWindow;
    IM_ASSERT((window->Flags & ImGuiWindowFlags_NoDocking) == 0);
    bool is_drag_docking = (g.IO.ConfigDockingWithShift) || ImRect(0, 0, window->SizeFull.x, GetFrameHeight()).Contains(g.ActiveIdClickOffset);
    if (is_drag_docking && BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceNoHoldToOpenOthers | ImGuiDragDropFlags_SourceAutoExpirePayload))
    {
        SetDragDropPayload(IMGUI_PAYLOAD_TYPE_WINDOW, &window, sizeof(window));
        EndDragDropSource();
    }
}

void ImGui::BeginAsDockableDragDropTarget(ImGuiWindow* window)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;

    //IM_ASSERT(window->RootWindow == window); // May also be a DockSpace
    IM_ASSERT((window->Flags & ImGuiWindowFlags_NoDocking) == 0);
    if (!g.DragDropActive)
        return;
    if (!BeginDragDropTargetCustom(window->Rect(), window->ID))
        return;

    // Peek into the payload before calling AcceptDragDropPayload() so we can handle overlapping dock nodes with filtering
    // (this is a little unusual pattern, normally most code would call AcceptDragDropPayload directly)
    const ImGuiPayload* payload = &g.DragDropPayload;
    if (!payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) || !DockNodeIsDropAllowed(window, *(ImGuiWindow**)payload->Data))
    {
        EndDragDropTarget();
        return;
    }

    ImGuiWindow* payload_window = *(ImGuiWindow**)payload->Data;
    if (AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_WINDOW, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        // Select target node
        ImGuiDockNode* node = NULL;
        bool allow_null_target_node = false;
        if (window->DockNodeAsHost)
            node = DockNodeTreeFindNodeByPos(window->DockNodeAsHost, g.IO.MousePos);
        else if (window->DockNode) // && window->DockIsActive)
            node = window->DockNode;
        else
            allow_null_target_node = true; // Dock into a regular window

        const ImRect explicit_target_rect = (node && node->TabBar && !node->IsHiddenTabBar() && !node->IsNoTabBar()) ? node->TabBar->BarRect : ImRect(window->Pos, window->Pos + ImVec2(window->Size.x, GetFrameHeight()));
        const bool is_explicit_target = g.IO.ConfigDockingWithShift || IsMouseHoveringRect(explicit_target_rect.Min, explicit_target_rect.Max);

        // Preview docking request and find out split direction/ratio
        //const bool do_preview = true;     // Ignore testing for payload->IsPreview() which removes one frame of delay, but breaks overlapping drop targets within the same window.        
        const bool do_preview = payload->IsPreview() || payload->IsDelivery();
        if (do_preview && (node != NULL || allow_null_target_node))
        {
            ImGuiDockPreviewData split_inner, split_outer;
            ImGuiDockPreviewData* split_data = &split_inner;
            if (node && (node->ParentNode || node->IsCentralNode()))
                if (ImGuiDockNode* root_node = DockNodeGetRootNode(node))
                {
                    DockNodePreviewDockCalc(window, root_node, payload_window, &split_outer, is_explicit_target, true);
                    if (split_outer.IsSplitDirExplicit)
                        split_data = &split_outer;
                }
            DockNodePreviewDockCalc(window, node, payload_window, &split_inner, is_explicit_target, false);
            if (split_data == &split_outer)
                split_inner.IsDropAllowed = false;

            // Draw inner then outer, so that previewed tab (in inner data) will be behind the outer drop boxes
            DockNodePreviewDockRender(window, node, payload_window, &split_inner);
            DockNodePreviewDockRender(window, node, payload_window, &split_outer);

            // Queue docking request
            if (split_data->IsDropAllowed && payload->IsDelivery())
                DockContextQueueDock(ctx, window, split_data->SplitNode, payload_window, split_data->SplitDir, split_data->SplitRatio, split_data == &split_outer);
        }
    }
    EndDragDropTarget();
}

//-----------------------------------------------------------------------------
// Docking: Settings
//-----------------------------------------------------------------------------
// - DockSettingsRenameNodeReferences()
// - DockSettingsRemoveNodeReferences()
// - DockSettingsFindNodeSettings()
// - DockSettingsHandler_ReadOpen()
// - DockSettingsHandler_ReadLine()
// - DockSettingsHandler_DockNodeToSettings()
// - DockSettingsHandler_WriteAll()
//-----------------------------------------------------------------------------

static void ImGui::DockSettingsRenameNodeReferences(ImGuiID old_node_id, ImGuiID new_node_id)
{
    ImGuiContext& g = *GImGui;
    IMGUI_DEBUG_LOG_DOCKING("DockSettingsRenameNodeReferences: from 0x%08X -> to 0x%08X\n", old_node_id, new_node_id);
    for (int window_n = 0; window_n < g.Windows.Size; window_n++)
    {
        ImGuiWindow* window = g.Windows[window_n];
        if (window->DockId == old_node_id && window->DockNode == NULL)
            window->DockId = new_node_id;
    }
    for (int settings_n = 0; settings_n < g.SettingsWindows.Size; settings_n++)     // FIXME-OPT: We could remove this loop by storing the index in the map
    {
        ImGuiWindowSettings* window_settings = &g.SettingsWindows[settings_n];
        if (window_settings->DockId == old_node_id)
            window_settings->DockId = new_node_id;
    }
}

// Remove references stored in ImGuiWindowSettings to the given ImGuiDockNodeSettings
static void ImGui::DockSettingsRemoveNodeReferences(ImGuiID* node_ids, int node_ids_count)
{
    ImGuiContext& g = *GImGui;
    int found = 0;
    for (int settings_n = 0; settings_n < g.SettingsWindows.Size; settings_n++)     // FIXME-OPT: We could remove this loop by storing the index in the map
    {
        ImGuiWindowSettings* window_settings = &g.SettingsWindows[settings_n];
        for (int node_n = 0; node_n < node_ids_count; node_n++)
            if (window_settings->DockId == node_ids[node_n])
            {
                window_settings->DockId = 0;
                window_settings->DockOrder = -1;
                if (++found < node_ids_count)
                    break;
                return;
            }
    }
}

static ImGuiDockNodeSettings* ImGui::DockSettingsFindNodeSettings(ImGuiContext* ctx, ImGuiID id)
{
    // FIXME-OPT
    ImGuiDockContext* dc = ctx->DockContext;
    for (int n = 0; n < dc->SettingsNodes.Size; n++)
        if (dc->SettingsNodes[n].ID == id)
            return &dc->SettingsNodes[n];
    return NULL;
}

static void* ImGui::DockSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    if (strcmp(name, "Data") != 0)
        return NULL;
    return (void*)1;
}

static void ImGui::DockSettingsHandler_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler*, void*, const char* line)
{
    char c = 0;
    int x = 0, y = 0;
    int r = 0;

    // Parsing, e.g.
    // " DockNode   ID=0x00000001 Pos=383,193 Size=201,322 Split=Y,0.506 "
    // "   DockNode ID=0x00000002 Parent=0x00000001 "
    // Important: this code expect currently fields in a fixed order.
    ImGuiDockNodeSettings node;
    line = ImStrSkipBlank(line);
    if      (strncmp(line, "DockNode", 8) == 0)  { line = ImStrSkipBlank(line + strlen("DockNode")); }
    else if (strncmp(line, "DockSpace", 9) == 0) { line = ImStrSkipBlank(line + strlen("DockSpace")); node.Flags |= ImGuiDockNodeFlags_DockSpace; }
    else return;
    if (sscanf(line, "ID=0x%08X%n",      &node.ID, &r) == 1)            { line += r; } else return;
    if (sscanf(line, " Parent=0x%08X%n", &node.ParentNodeID, &r) == 1)  { line += r; if (node.ParentNodeID == 0) return; }
    if (sscanf(line, " Window=0x%08X%n", &node.ParentWindowID, &r) ==1) { line += r; if (node.ParentWindowID == 0) return; }
    if (node.ParentNodeID == 0)
    {
        if (sscanf(line, " Pos=%i,%i%n",  &x, &y, &r) == 2)         { line += r; node.Pos = ImVec2ih((short)x, (short)y); } else return;
        if (sscanf(line, " Size=%i,%i%n", &x, &y, &r) == 2)         { line += r; node.Size = ImVec2ih((short)x, (short)y); } else return;
    }
    else
    {
        if (sscanf(line, " SizeRef=%i,%i%n", &x, &y, &r) == 2)      { line += r; node.SizeRef = ImVec2ih((short)x, (short)y); }
    }
    if (sscanf(line, " Split=%c%n", &c, &r) == 1)                   { line += r; if (c == 'X') node.SplitAxis = ImGuiAxis_X; else if (c == 'Y') node.SplitAxis = ImGuiAxis_Y; }
    if (sscanf(line, " NoResize=%d%n", &x, &r) == 1)                { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoResize; }
    if (sscanf(line, " CentralNode=%d%n", &x, &r) == 1)             { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_CentralNode; }
    if (sscanf(line, " NoTabBar=%d%n", &x, &r) == 1)                { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoTabBar; }
    if (sscanf(line, " HiddenTabBar=%d%n", &x, &r) == 1)            { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_HiddenTabBar; }
    if (sscanf(line, " NoWindowMenuButton=%d%n", &x, &r) == 1)      { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoWindowMenuButton; }
    if (sscanf(line, " NoCloseButton=%d%n", &x, &r) == 1)           { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoCloseButton; }
    if (sscanf(line, " Selected=0x%08X%n", &node.SelectedWindowID,&r) == 1) { line += r; }
    ImGuiDockContext* dc = ctx->DockContext;
    if (node.ParentNodeID != 0)
        if (ImGuiDockNodeSettings* parent_settings = DockSettingsFindNodeSettings(ctx, node.ParentNodeID))
            node.Depth = parent_settings->Depth + 1;
    dc->SettingsNodes.push_back(node);
}

static void DockSettingsHandler_DockNodeToSettings(ImGuiDockContext* dc, ImGuiDockNode* node, int depth)
{
    ImGuiDockNodeSettings node_settings;
    IM_ASSERT(depth < (1 << (sizeof(node_settings.Depth) << 3)));
    node_settings.ID = node->ID;
    node_settings.ParentNodeID = node->ParentNode ? node->ParentNode->ID : 0;
    node_settings.ParentWindowID = (node->IsDockSpace() && node->HostWindow && node->HostWindow->ParentWindow) ? node->HostWindow->ParentWindow->ID : 0;
    node_settings.SelectedWindowID = node->SelectedTabID;
    node_settings.SplitAxis = node->IsSplitNode() ? (char)node->SplitAxis : ImGuiAxis_None;
    node_settings.Depth = (char)depth;
    node_settings.Flags = (node->LocalFlags & ImGuiDockNodeFlags_SavedFlagsMask_);
    node_settings.Pos = ImVec2ih(node->Pos);
    node_settings.Size = ImVec2ih(node->Size);
    node_settings.SizeRef = ImVec2ih(node->SizeRef);
    dc->SettingsNodes.push_back(node_settings);
    if (node->ChildNodes[0])
        DockSettingsHandler_DockNodeToSettings(dc, node->ChildNodes[0], depth + 1);
    if (node->ChildNodes[1])
        DockSettingsHandler_DockNodeToSettings(dc, node->ChildNodes[1], depth + 1);
}

static void ImGui::DockSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = g.DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    // Gather settings data
    // (unlike our windows settings, because nodes are always built we can do a full rewrite of the SettingsNode buffer)
    dc->SettingsNodes.resize(0);
    dc->SettingsNodes.reserve(dc->Nodes.Data.Size);
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            if (node->IsRootNode())
                DockSettingsHandler_DockNodeToSettings(dc, node, 0);

    int max_depth = 0;
    for (int node_n = 0; node_n < dc->SettingsNodes.Size; node_n++)
        max_depth = ImMax((int)dc->SettingsNodes[node_n].Depth, max_depth);

    // Write to text buffer
    buf->appendf("[%s][Data]\n", handler->TypeName);
    for (int node_n = 0; node_n < dc->SettingsNodes.Size; node_n++)
    {
        const int line_start_pos = buf->size(); (void)line_start_pos;
        const ImGuiDockNodeSettings* node_settings = &dc->SettingsNodes[node_n];
        buf->appendf("%*s%s%*s", node_settings->Depth * 2, "", (node_settings->Flags & ImGuiDockNodeFlags_DockSpace) ? "DockSpace" : "DockNode ", (max_depth - node_settings->Depth) * 2, "");  // Text align nodes to facilitate looking at .ini file
        buf->appendf(" ID=0x%08X", node_settings->ID);
        if (node_settings->ParentNodeID)
        {
            buf->appendf(" Parent=0x%08X SizeRef=%d,%d", node_settings->ParentNodeID, node_settings->SizeRef.x, node_settings->SizeRef.y);
        }
        else
        {
            if (node_settings->ParentWindowID)
                buf->appendf(" Window=0x%08X", node_settings->ParentWindowID);
            buf->appendf(" Pos=%d,%d Size=%d,%d", node_settings->Pos.x, node_settings->Pos.y, node_settings->Size.x, node_settings->Size.y);
        }
        if (node_settings->SplitAxis != ImGuiAxis_None)
            buf->appendf(" Split=%c", (node_settings->SplitAxis == ImGuiAxis_X) ? 'X' : 'Y');
        if (node_settings->Flags & ImGuiDockNodeFlags_NoResize)
            buf->appendf(" NoResize=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_CentralNode)
            buf->appendf(" CentralNode=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_NoTabBar)
            buf->appendf(" NoTabBar=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_HiddenTabBar)
            buf->appendf(" HiddenTabBar=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_NoWindowMenuButton)
            buf->appendf(" NoWindowMenuButton=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_NoCloseButton)
            buf->appendf(" NoCloseButton=1");
        if (node_settings->SelectedWindowID)
            buf->appendf(" Selected=0x%08X", node_settings->SelectedWindowID);

#if IMGUI_DEBUG_INI_SETTINGS
        // [DEBUG] Include comments in the .ini file to ease debugging
        if (ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_settings->ID))
        {
            buf->appendf("%*s", ImMax(2, (line_start_pos + 92) - buf->size()), "");     // Align everything
            if (node->IsDockSpace() && node->HostWindow && node->HostWindow->ParentWindow)
                buf->appendf(" ; in '%s'", node->HostWindow->ParentWindow->Name);
            int contains_window = 0;
            for (int window_n = 0; window_n < ctx->SettingsWindows.Size; window_n++)    // Iterate settings so we can give info about windows that didn't exist during the session.
                if (ctx->SettingsWindows[window_n].DockId == node_settings->ID)
                {
                    if (contains_window++ == 0)
                        buf->appendf(" ; contains ");
                    buf->appendf("'%s' ", ctx->SettingsWindows[window_n].Name);
                }
        }
#endif
        buf->appendf("\n");
    }
    buf->appendf("\n");
}


//-----------------------------------------------------------------------------
// [SECTION] PLATFORM DEPENDENT HELPERS
//-----------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WINDOWS_) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && (!defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) || !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef __MINGW32__
#include <Windows.h>
#else
#include <windows.h>
#endif
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)

#ifdef _MSC_VER
#pragma comment(lib, "user32")
#endif

// Win32 clipboard implementation
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

#elif defined(__APPLE__) && TARGET_OS_OSX && defined(IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)

#include <Carbon/Carbon.h>  // Use old API to avoid need for separate .mm file
static PasteboardRef main_clipboard = 0;

// OSX clipboard implementation
// If you enable this you will need to add '-framework ApplicationServices' to your linker command-line!
static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    if (!main_clipboard)
        PasteboardCreate(kPasteboardClipboard, &main_clipboard);
    PasteboardClear(main_clipboard);
    CFDataRef cf_data = CFDataCreate(kCFAllocatorDefault, (const UInt8*)text, strlen(text));
    if (cf_data)
    {
        PasteboardPutItemFlavor(main_clipboard, (PasteboardItemID)1, CFSTR("public.utf8-plain-text"), cf_data, 0);
        CFRelease(cf_data);
    }
}

static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    if (!main_clipboard)
        PasteboardCreate(kPasteboardClipboard, &main_clipboard);
    PasteboardSynchronize(main_clipboard);

    ItemCount item_count = 0;
    PasteboardGetItemCount(main_clipboard, &item_count);
    for (int i = 0; i < item_count; i++)
    {
        PasteboardItemID item_id = 0;
        PasteboardGetItemIdentifier(main_clipboard, i + 1, &item_id);
        CFArrayRef flavor_type_array = 0;
        PasteboardCopyItemFlavors(main_clipboard, item_id, &flavor_type_array);
        for (CFIndex j = 0, nj = CFArrayGetCount(flavor_type_array); j < nj; j++)
        {
            CFDataRef cf_data;
            if (PasteboardCopyItemFlavorData(main_clipboard, item_id, CFSTR("public.utf8-plain-text"), &cf_data) == noErr)
            {
                static ImVector<char> clipboard_text;
                int length = (int)CFDataGetLength(cf_data);
                clipboard_text.resize(length + 1);
                CFDataGetBytes(cf_data, CFRangeMake(0, length), (UInt8*)clipboard_text.Data);
                clipboard_text[length] = 0;
                CFRelease(cf_data);
                return clipboard_text.Data;
            }
        }
    }
    return NULL;
}

#else

// Local Dear ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers.
static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    ImGuiContext& g = *GImGui;
    return g.PrivateClipboard.empty() ? NULL : g.PrivateClipboard.begin();
}

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

//-----------------------------------------------------------------------------
// [SECTION] METRICS/DEBUG WINDOW
//-----------------------------------------------------------------------------

static void RenderViewportThumbnail(ImDrawList* draw_list, ImGuiViewportP* viewport, const ImRect& bb)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImVec2 scale = bb.GetSize() / viewport->Size;
    ImVec2 off = bb.Min - viewport->Pos * scale;
    float alpha_mul = (viewport->Flags & ImGuiViewportFlags_Minimized) ? 0.30f : 1.00f;
    window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul * 0.40f));
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* thumb_window = g.Windows[i];
        if (!thumb_window->WasActive || ((thumb_window->Flags & ImGuiWindowFlags_ChildWindow)))
            continue;
        if (thumb_window->SkipItems && (thumb_window->Flags & ImGuiWindowFlags_ChildWindow)) // FIXME-DOCK: Skip hidden docked windows. Identify those betters.
            continue;
        if (thumb_window->Viewport != viewport)
            continue;

        ImRect thumb_r = thumb_window->Rect();
        ImRect title_r = thumb_window->TitleBarRect();
        ImRect thumb_r_scaled = ImRect(ImFloor(off + thumb_r.Min * scale), ImFloor(off +  thumb_r.Max * scale));
        ImRect title_r_scaled = ImRect(ImFloor(off + title_r.Min * scale), ImFloor(off +  ImVec2(title_r.Max.x, title_r.Min.y) * scale) + ImVec2(0,5)); // Exaggerate title bar height
        thumb_r_scaled.ClipWithFull(bb);
        title_r_scaled.ClipWithFull(bb);
        const bool window_is_focused = (g.NavWindow && thumb_window->RootWindowForTitleBarHighlight == g.NavWindow->RootWindowForTitleBarHighlight);
        window->DrawList->AddRectFilled(thumb_r_scaled.Min, thumb_r_scaled.Max, ImGui::GetColorU32(ImGuiCol_WindowBg, alpha_mul));
        window->DrawList->AddRectFilled(title_r_scaled.Min, title_r_scaled.Max, ImGui::GetColorU32(window_is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg, alpha_mul));
        window->DrawList->AddRect(thumb_r_scaled.Min, thumb_r_scaled.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul));
        if (ImGuiWindow* window_for_title = GetWindowForTitleDisplay(thumb_window))
            window->DrawList->AddText(g.Font, g.FontSize * 1.0f, title_r_scaled.Min, ImGui::GetColorU32(ImGuiCol_Text, alpha_mul), window_for_title->Name, ImGui::FindRenderedTextEnd(window_for_title->Name));
    }
    draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul));
}

void ImGui::ShowViewportThumbnails()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // We don't display full monitor bounds (we could, but it often looks awkward), instead we display just enough to cover all of our viewports.
    float SCALE = 1.0f / 8.0f;
    ImRect bb_full;
    //for (int n = 0; n < g.PlatformIO.Monitors.Size; n++)
    //    bb_full.Add(GetPlatformMonitorMainRect(g.PlatformIO.Monitors[n]));
    for (int n = 0; n < g.Viewports.Size; n++)
        bb_full.Add(g.Viewports[n]->GetRect());
    ImVec2 p = window->DC.CursorPos;
    ImVec2 off = p - bb_full.Min * SCALE;
    //for (int n = 0; n < g.PlatformIO.Monitors.Size; n++)
    //    window->DrawList->AddRect(off + g.PlatformIO.Monitors[n].MainPos * SCALE, off + (g.PlatformIO.Monitors[n].MainPos + g.PlatformIO.Monitors[n].MainSize) * SCALE, ImGui::GetColorU32(ImGuiCol_Border));
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        ImRect viewport_draw_bb(off + (viewport->Pos) * SCALE, off + (viewport->Pos + viewport->Size) * SCALE);
        RenderViewportThumbnail(window->DrawList, viewport, viewport_draw_bb);
    }
    ImGui::Dummy(bb_full.GetSize() * SCALE);
}

#ifndef IMGUI_DISABLE_METRICS_WINDOW
void ImGui::ShowMetricsWindow(bool* p_open)
{
    if (!ImGui::Begin("Dear ImGui Metrics", p_open))
    {
        ImGui::End();
        return;
    }

    // State
    enum { WRT_OuterRect, WRT_OuterRectClipped, WRT_InnerRect, WRT_InnerClipRect, WRT_WorkRect, WRT_Contents, WRT_ContentsRegionRect, WRT_Count }; // Windows Rect Type
    const char* wrt_rects_names[WRT_Count] = { "OuterRect", "OuterRectClipped", "InnerRect", "InnerClipRect", "WorkRect", "Contents", "ContentsRegionRect" };
    static bool show_windows_rects = false;
    static int  show_windows_rect_type = WRT_WorkRect;
    static bool show_windows_begin_order = false;
    static bool show_drawcmd_clip_rects = true;
    static bool show_docking_nodes = false;

    // Basic info
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
    ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
    ImGui::Text("%d active allocations", io.MetricsActiveAllocations);
    ImGui::Separator();

    // Helper functions to display common structures:
    // - NodeDrawList
    // - NodeColumns
    // - NodeWindow
    // - NodeWindows
    // - NodeViewport
    // - NodeDockNode
    // - NodeTabBar
    struct Funcs
    {
        static ImRect GetWindowRect(ImGuiWindow* window, int rect_type)
        {
            if (rect_type == WRT_OuterRect)                 { return window->Rect(); }
            else if (rect_type == WRT_OuterRectClipped)     { return window->OuterRectClipped; }
            else if (rect_type == WRT_InnerRect)            { return window->InnerRect; }
            else if (rect_type == WRT_InnerClipRect)        { return window->InnerClipRect; }
            else if (rect_type == WRT_WorkRect)             { return window->WorkRect; }
            else if (rect_type == WRT_Contents)             { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSize); }
            else if (rect_type == WRT_ContentsRegionRect)   { return window->ContentsRegionRect; }
            IM_ASSERT(0);
            return ImRect();
        }

        static void NodeDrawList(ImGuiWindow* window, ImGuiViewportP* viewport, ImDrawList* draw_list, const char* label)
        {
            bool node_open = ImGui::TreeNode(draw_list, "%s: '%s' %d vtx, %d indices, %d cmds", label, draw_list->_OwnerName ? draw_list->_OwnerName : "", draw_list->VtxBuffer.Size, draw_list->IdxBuffer.Size, draw_list->CmdBuffer.Size);
            if (draw_list == ImGui::GetWindowDrawList())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f,0.4f,0.4f,1.0f), "CURRENTLY APPENDING"); // Can't display stats for active draw list! (we don't have the data double-buffered)
                if (node_open) ImGui::TreePop();
                return;
            }

            ImDrawList* fg_draw_list = viewport ? GetForegroundDrawList(viewport) : NULL; // Render additional visuals into the top-most draw list
            if (window && fg_draw_list && ImGui::IsItemHovered())
                fg_draw_list->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
            if (!node_open)
                return;

            if (window && !window->WasActive)
                ImGui::Text("(Note: owning Window is inactive: DrawList is not being rendered!)");

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
                char buf[300];
                ImFormatString(buf, IM_ARRAYSIZE(buf), "Draw %4d triangles, tex 0x%p, clip_rect (%4.0f,%4.0f)-(%4.0f,%4.0f)",
                    pcmd->ElemCount/3, (void*)(intptr_t)pcmd->TextureId, pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
                bool pcmd_node_open = ImGui::TreeNode((void*)(pcmd - draw_list->CmdBuffer.begin()), "%s", buf);
                if (show_drawcmd_clip_rects && fg_draw_list && ImGui::IsItemHovered())
                {
                    ImRect clip_rect = pcmd->ClipRect;
                    ImRect vtxs_rect;
                    for (int i = elem_offset; i < elem_offset + (int)pcmd->ElemCount; i++)
                        vtxs_rect.Add(draw_list->VtxBuffer[idx_buffer ? idx_buffer[i] : i].pos);
                    clip_rect.Floor(); fg_draw_list->AddRect(clip_rect.Min, clip_rect.Max, IM_COL32(255,0,255,255));
                    vtxs_rect.Floor(); fg_draw_list->AddRect(vtxs_rect.Min, vtxs_rect.Max, IM_COL32(255,255,0,255));
                }
                if (!pcmd_node_open)
                    continue;

                // Display individual triangles/vertices. Hover on to get the corresponding triangle highlighted.
                ImGui::Text("ElemCount: %d, ElemCount/3: %d, VtxOffset: +%d, IdxOffset: +%d", pcmd->ElemCount, pcmd->ElemCount/3, pcmd->VtxOffset, pcmd->IdxOffset);
                ImGuiListClipper clipper(pcmd->ElemCount/3); // Manually coarse clip our print out of individual vertices to save CPU, only items that may be visible.
                while (clipper.Step())
                    for (int prim = clipper.DisplayStart, idx_i = elem_offset + clipper.DisplayStart*3; prim < clipper.DisplayEnd; prim++)
                    {
                        char *buf_p = buf, *buf_end = buf + IM_ARRAYSIZE(buf);
                        ImVec2 triangles_pos[3];
                        for (int n = 0; n < 3; n++, idx_i++)
                        {
                            int vtx_i = idx_buffer ? idx_buffer[idx_i] : idx_i;
                            ImDrawVert& v = draw_list->VtxBuffer[vtx_i];
                            triangles_pos[n] = v.pos;
                            buf_p += ImFormatString(buf_p, buf_end - buf_p, "%s %04d: pos (%8.2f,%8.2f), uv (%.6f,%.6f), col %08X\n",
                                (n == 0) ? "elem" : "    ", idx_i, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.col);
                        }
                        ImGui::Selectable(buf, false);
                        if (fg_draw_list && ImGui::IsItemHovered())
                        {
                            ImDrawListFlags backup_flags = fg_draw_list->Flags;
                            fg_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines at is more readable for very large and thin triangles.
                            fg_draw_list->AddPolyline(triangles_pos, 3, IM_COL32(255,255,0,255), true, 1.0f);
                            fg_draw_list->Flags = backup_flags;
                        }
                    }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        static void NodeColumns(const ImGuiColumns* columns)
        {
            if (!ImGui::TreeNode((void*)(uintptr_t)columns->ID, "Columns Id: 0x%08X, Count: %d, Flags: 0x%04X", columns->ID, columns->Count, columns->Flags))
                return;
            ImGui::BulletText("Width: %.1f (MinX: %.1f, MaxX: %.1f)", columns->OffMaxX - columns->OffMinX, columns->OffMinX, columns->OffMaxX);
            for (int column_n = 0; column_n < columns->Columns.Size; column_n++)
                ImGui::BulletText("Column %02d: OffsetNorm %.3f (= %.1f px)", column_n, columns->Columns[column_n].OffsetNorm, GetColumnOffsetFromNorm(columns, columns->Columns[column_n].OffsetNorm));
            ImGui::TreePop();
        }

        static void NodeWindow(ImGuiWindow* window, const char* label)
        {
            if (window == NULL)
            {
                ImGui::BulletText("%s: NULL", label);
                return;
            }
            if (!ImGui::TreeNode(window, "%s '%s', %d @ 0x%p", label, window->Name, (window->Active || window->WasActive), window))
                return;
            ImGuiWindowFlags flags = window->Flags;
            NodeDrawList(window, window->Viewport, window->DrawList, "DrawList");
            ImGui::BulletText("Pos: (%.1f,%.1f), Size: (%.1f,%.1f), ContentSize (%.1f,%.1f)", window->Pos.x, window->Pos.y, window->Size.x, window->Size.y, window->ContentSize.x, window->ContentSize.y);
            ImGui::BulletText("Flags: 0x%08X (%s%s%s%s%s%s%s%s%s..)", flags,
                (flags & ImGuiWindowFlags_ChildWindow)  ? "Child " : "",      (flags & ImGuiWindowFlags_Tooltip)     ? "Tooltip "   : "",  (flags & ImGuiWindowFlags_Popup) ? "Popup " : "",
                (flags & ImGuiWindowFlags_Modal)        ? "Modal " : "",      (flags & ImGuiWindowFlags_ChildMenu)   ? "ChildMenu " : "",  (flags & ImGuiWindowFlags_NoSavedSettings) ? "NoSavedSettings " : "",
                (flags & ImGuiWindowFlags_NoMouseInputs)? "NoMouseInputs":"", (flags & ImGuiWindowFlags_NoNavInputs) ? "NoNavInputs" : "", (flags & ImGuiWindowFlags_AlwaysAutoResize) ? "AlwaysAutoResize" : "");
            ImGui::BulletText("WindowClassId: 0x%08X", window->WindowClass.ClassId);
            ImGui::BulletText("Scroll: (%.2f/%.2f,%.2f/%.2f)", window->Scroll.x, window->ScrollMax.x, window->Scroll.y, window->ScrollMax.y);
            ImGui::BulletText("Active: %d/%d, WriteAccessed: %d, BeginOrderWithinContext: %d", window->Active, window->WasActive, window->WriteAccessed, (window->Active || window->WasActive) ? window->BeginOrderWithinContext : -1);
            ImGui::BulletText("Appearing: %d, Hidden: %d (CanSkip %d Cannot %d), SkipItems: %d", window->Appearing, window->Hidden, window->HiddenFramesCanSkipItems, window->HiddenFramesCannotSkipItems, window->SkipItems);
            ImGui::BulletText("NavLastIds: 0x%08X,0x%08X, NavLayerActiveMask: %X", window->NavLastIds[0], window->NavLastIds[1], window->DC.NavLayerActiveMask);
            ImGui::BulletText("NavLastChildNavWindow: %s", window->NavLastChildNavWindow ? window->NavLastChildNavWindow->Name : "NULL");
            if (!window->NavRectRel[0].IsInverted())
                ImGui::BulletText("NavRectRel[0]: (%.1f,%.1f)(%.1f,%.1f)", window->NavRectRel[0].Min.x, window->NavRectRel[0].Min.y, window->NavRectRel[0].Max.x, window->NavRectRel[0].Max.y);
            else
                ImGui::BulletText("NavRectRel[0]: <None>");
            ImGui::BulletText("Viewport: %d%s, ViewportId: 0x%08X, ViewportPos: (%.1f,%.1f)", window->Viewport ? window->Viewport->Idx : -1, window->ViewportOwned ? " (Owned)" : "", window->ViewportId, window->ViewportPos.x, window->ViewportPos.y);
            ImGui::BulletText("ViewportMonitor: %d", window->Viewport ? window->Viewport->PlatformMonitor : -1);
            ImGui::BulletText("DockId: 0x%04X, DockOrder: %d, Act: %d, Vis: %d", window->DockId, window->DockOrder, window->DockIsActive, window->DockTabIsVisible);
            if (window->DockNode || window->DockNodeAsHost)
                NodeDockNode(window->DockNodeAsHost ? window->DockNodeAsHost : window->DockNode, window->DockNodeAsHost ? "DockNodeAsHost" : "DockNode");
            if (window->RootWindow != window) NodeWindow(window->RootWindow, "RootWindow");
            if (window->RootWindowDockStop != window->RootWindow) NodeWindow(window->RootWindowDockStop, "RootWindowDockStop");
            if (window->ParentWindow != NULL) NodeWindow(window->ParentWindow, "ParentWindow");
            if (window->DC.ChildWindows.Size > 0) NodeWindows(window->DC.ChildWindows, "ChildWindows");
            if (window->ColumnsStorage.Size > 0 && ImGui::TreeNode("Columns", "Columns sets (%d)", window->ColumnsStorage.Size))
            {
                for (int n = 0; n < window->ColumnsStorage.Size; n++)
                    NodeColumns(&window->ColumnsStorage[n]);
                ImGui::TreePop();
            }
            ImGui::BulletText("Storage: %d bytes", window->StateStorage.Data.size_in_bytes());
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

        static void NodeViewport(ImGuiViewportP* viewport)
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode((void*)(intptr_t)viewport->ID, "Viewport #%d, ID: 0x%08X, Parent: 0x%08X, Window: \"%s\"", viewport->Idx, viewport->ID, viewport->ParentViewportId, viewport->Window ? viewport->Window->Name : "N/A"))
            {
                ImGuiWindowFlags flags = viewport->Flags;
                ImGui::BulletText("Pos: (%.0f,%.0f), Size: (%.0f,%.0f), Monitor: %d, DpiScale: %.0f%%", viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y, viewport->PlatformMonitor, viewport->DpiScale * 100.0f);
                if (viewport->Idx > 0) { ImGui::SameLine(); if (ImGui::SmallButton("Reset Pos")) { viewport->Pos = ImVec2(200,200); if (viewport->Window) viewport->Window->Pos = ImVec2(200,200); } }
                ImGui::BulletText("Flags: 0x%04X =%s%s%s%s%s%s%s", viewport->Flags,
                    (flags & ImGuiViewportFlags_CanHostOtherWindows) ? " CanHostOtherWindows" : "", (flags & ImGuiViewportFlags_NoDecoration) ? " NoDecoration" : "",
                    (flags & ImGuiViewportFlags_NoFocusOnAppearing)  ? " NoFocusOnAppearing"  : "", (flags & ImGuiViewportFlags_NoInputs)     ? " NoInputs"     : "",
                    (flags & ImGuiViewportFlags_NoRendererClear)     ? " NoRendererClear"     : "", (flags & ImGuiViewportFlags_Minimized)    ? " Minimized"    : "",
                    (flags & ImGuiViewportFlags_NoAutoMerge)         ? " NoAutoMerge"         : "");
                for (int layer_i = 0; layer_i < IM_ARRAYSIZE(viewport->DrawDataBuilder.Layers); layer_i++)
                    for (int draw_list_i = 0; draw_list_i < viewport->DrawDataBuilder.Layers[layer_i].Size; draw_list_i++)
                        Funcs::NodeDrawList(NULL, viewport, viewport->DrawDataBuilder.Layers[layer_i][draw_list_i], "DrawList");
                ImGui::TreePop();
            }
        }

        static void NodeDockNode(ImGuiDockNode* node, const char* label)
        {
            ImGuiContext& g = *GImGui;
            bool open;
            if (node->Windows.Size > 0)
                open = ImGui::TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %d windows (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", node->Windows.Size, node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
            else
                open = ImGui::TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %s split (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", (node->SplitAxis == ImGuiAxis_X) ? "horizontal" : (node->SplitAxis == ImGuiAxis_Y) ? "vertical" : "n/a", node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
            if (open)
            {
                IM_ASSERT(node->ChildNodes[0] == NULL || node->ChildNodes[0]->ParentNode == node);
                IM_ASSERT(node->ChildNodes[1] == NULL || node->ChildNodes[1]->ParentNode == node);
                ImGui::BulletText("Pos (%.0f,%.0f), Size (%.0f, %.0f) Ref (%.0f, %.0f)",
                    node->Pos.x, node->Pos.y, node->Size.x, node->Size.y, node->SizeRef.x, node->SizeRef.y);
                NodeWindow(node->HostWindow, "HostWindow");
                NodeWindow(node->VisibleWindow, "VisibleWindow");
                ImGui::BulletText("SelectedTabID: 0x%08X, LastFocusedNodeID: 0x%08X", node->SelectedTabID, node->LastFocusedNodeID);
                ImGui::BulletText("Misc:%s%s%s%s", node->IsDockSpace() ? " IsDockSpace" : "", node->IsCentralNode() ? " IsCentralNode" : "", (g.FrameCount - node->LastFrameAlive < 2) ? " IsAlive" : "", (g.FrameCount - node->LastFrameActive < 2) ? " IsActive" : "");
                if (ImGui::TreeNode("flags", "LocalFlags: 0x%04X SharedFlags: 0x%04X", node->LocalFlags, node->SharedFlags))
                {
                    ImGui::CheckboxFlags("LocalFlags: NoSplit", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoSplit);
                    ImGui::CheckboxFlags("LocalFlags: NoResize", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoResize);
                    ImGui::CheckboxFlags("LocalFlags: NoTabBar", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoTabBar);
                    ImGui::CheckboxFlags("LocalFlags: HiddenTabBar", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_HiddenTabBar);
                    ImGui::CheckboxFlags("LocalFlags: NoWindowMenuButton", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoWindowMenuButton);
                    ImGui::CheckboxFlags("LocalFlags: NoCloseButton", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoCloseButton);
                    ImGui::TreePop();
                }
                if (node->ParentNode)
                    NodeDockNode(node->ParentNode, "ParentNode");
                if (node->ChildNodes[0])
                    NodeDockNode(node->ChildNodes[0], "Child[0]");
                if (node->ChildNodes[1])
                    NodeDockNode(node->ChildNodes[1], "Child[1]");
                if (node->TabBar)
                    NodeTabBar(node->TabBar);
                ImGui::TreePop();
            }
        }

        static void NodeTabBar(ImGuiTabBar* tab_bar)
        {
            // Standalone tab bars (not associated to docking/windows functionality) currently hold no discernible strings.
            char buf[256];
            char* p = buf;
            const char* buf_end = buf + IM_ARRAYSIZE(buf);
            p += ImFormatString(p, buf_end - p, "TabBar (%d tabs)%s",
                tab_bar->Tabs.Size, (tab_bar->PrevFrameVisible < ImGui::GetFrameCount() - 2) ? " *Inactive*" : "");
            if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
            {
                p += ImFormatString(p, buf_end - p, "  { ");
                for (int tab_n = 0; tab_n < ImMin(tab_bar->Tabs.Size, 3); tab_n++)
                    p += ImFormatString(p, buf_end - p, "%s'%s'", tab_n > 0 ? ", " : "", tab_bar->Tabs[tab_n].Window->Name);
                p += ImFormatString(p, buf_end - p, (tab_bar->Tabs.Size > 3) ? " ... }" : " } ");
            }
            if (ImGui::TreeNode(tab_bar, "%s", buf))
            {
                for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
                {
                    const ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
                    ImGui::PushID(tab);
                    if (ImGui::SmallButton("<")) { TabBarQueueChangeTabOrder(tab_bar, tab, -1); } ImGui::SameLine(0, 2);
                    if (ImGui::SmallButton(">")) { TabBarQueueChangeTabOrder(tab_bar, tab, +1); } ImGui::SameLine();
                    ImGui::Text("%02d%c Tab 0x%08X '%s'", tab_n, (tab->ID == tab_bar->SelectedTabId) ? '*' : ' ', tab->ID, tab->Window ? tab->Window->Name : "N/A");
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
    };

    Funcs::NodeWindows(g.Windows, "Windows");
    if (ImGui::TreeNode("Viewport", "Viewports (%d)", g.Viewports.Size))
    {
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::ShowViewportThumbnails();
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        if (g.PlatformIO.Monitors.Size > 0 && ImGui::TreeNode("Monitors", "Monitors (%d)", g.PlatformIO.Monitors.Size))
        {
            ImGui::TextWrapped("(When viewports are enabled, imgui needs uses monitor data to position popup/tooltips so they don't straddle monitors.)");
            for (int i = 0; i < g.PlatformIO.Monitors.Size; i++)
            {
                const ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[i];
                ImGui::BulletText("Monitor #%d: DPI %.0f%%\n MainMin (%.0f,%.0f), MainMax (%.0f,%.0f), MainSize (%.0f,%.0f)\n WorkMin (%.0f,%.0f), WorkMax (%.0f,%.0f), WorkSize (%.0f,%.0f)", 
                    i, mon.DpiScale * 100.0f, 
                    mon.MainPos.x, mon.MainPos.y, mon.MainPos.x + mon.MainSize.x, mon.MainPos.y + mon.MainSize.y, mon.MainSize.x, mon.MainSize.y,
                    mon.WorkPos.x, mon.WorkPos.y, mon.WorkPos.x + mon.WorkSize.x, mon.WorkPos.y + mon.WorkSize.y, mon.WorkSize.x, mon.WorkSize.y);
            }
            ImGui::TreePop();
        }
        for (int i = 0; i < g.Viewports.Size; i++)
            Funcs::NodeViewport(g.Viewports[i]);
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

    if (ImGui::TreeNode("TabBars", "Tab Bars (%d)", g.TabBars.Data.Size))
    {
        for (int n = 0; n < g.TabBars.Data.Size; n++)
            Funcs::NodeTabBar(g.TabBars.GetByIndex(n));
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Docking"))
    {
        ImGuiDockContext* dc = g.DockContext;
        ImGui::Checkbox("Ctrl shows window dock info", &show_docking_nodes);

        if (ImGui::TreeNode("Dock nodes"))
        {
            if (ImGui::SmallButton("Clear settings")) { DockContextClearNodes(&g, 0, true); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Rebuild all")) { dc->WantFullRebuild = true; }
            for (int n = 0; n < dc->Nodes.Data.Size; n++)
                if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
                    if (node->IsRootNode())
                        Funcs::NodeDockNode(node, "Node");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Settings"))
        {
            if (ImGui::SmallButton("Refresh"))
                SaveIniSettingsToMemory();
            ImGui::SameLine();
            if (ImGui::SmallButton("Save to disk"))
                SaveIniSettingsToDisk(g.IO.IniFilename);
            ImGui::Separator();
            ImGui::Text("Docked Windows:");
            for (int n = 0; n < g.SettingsWindows.Size; n++)
                if (g.SettingsWindows[n].DockId != 0)
                    ImGui::BulletText("Window '%s' -> DockId %08X", g.SettingsWindows[n].Name, g.SettingsWindows[n].DockId);
            ImGui::Separator();
            ImGui::Text("Dock Nodes:");
            for (int n = 0; n < dc->SettingsNodes.Size; n++)
            {
                ImGuiDockNodeSettings* settings = &dc->SettingsNodes[n];
                const char* selected_tab_name = NULL;
                if (settings->SelectedWindowID)
                {
                    if (ImGuiWindow* window = FindWindowByID(settings->SelectedWindowID))
                        selected_tab_name = window->Name;
                    else if (ImGuiWindowSettings* window_settings = FindWindowSettings(settings->SelectedWindowID))
                        selected_tab_name = window_settings->Name;
                }
                ImGui::BulletText("Node %08X, Parent %08X, SelectedTab %08X ('%s')", settings->ID, settings->ParentNodeID, settings->SelectedWindowID, selected_tab_name ? selected_tab_name : settings->SelectedWindowID ? "N/A" : "");
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

#if 0
    if (ImGui::TreeNode("Tables", "Tables (%d)", g.Tables.Data.Size))
    {
        ImGui::TreePop();
    }
#endif

    if (ImGui::TreeNode("Internal state"))
    {
        const char* input_source_names[] = { "None", "Mouse", "Nav", "NavKeyboard", "NavGamepad" }; IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT);
        ImGui::Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
        ImGui::Text("HoveredRootWindow: '%s'", g.HoveredRootWindow ? g.HoveredRootWindow->Name : "NULL");
        ImGui::Text("HoveredWindowUnderMovingWindow: '%s'", g.HoveredWindowUnderMovingWindow ? g.HoveredWindowUnderMovingWindow->Name : "NULL");
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
        ImGui::Text("MouseViewport: 0x%08X (UserHovered 0x%08X, LastHovered 0x%08X)", g.MouseViewport->ID, g.IO.MouseHoveredViewport, g.MouseLastHoveredViewport ? g.MouseLastHoveredViewport->ID : 0);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Tools"))
    {
        // The Item Picker tool is super useful to visually select an item and break into the call-stack of where it was submitted.
        if (ImGui::Button("Item Picker.."))
            ImGui::DebugStartItemPicker();

        ImGui::Checkbox("Show windows begin order", &show_windows_begin_order);
        ImGui::Checkbox("Show windows rectangles", &show_windows_rects);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
        show_windows_rects |= ImGui::Combo("##show_windows_rect_type", &show_windows_rect_type, wrt_rects_names, WRT_Count);
        if (show_windows_rects && g.NavWindow)
        {
            ImGui::BulletText("'%s':", g.NavWindow->Name);
            ImGui::Indent();
            for (int rect_n = 0; rect_n < WRT_Count; rect_n++)
            {
                ImRect r = Funcs::GetWindowRect(g.NavWindow, rect_n);
                ImGui::Text("(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), wrt_rects_names[rect_n]);
            }
            ImGui::Unindent();
        }
        ImGui::Checkbox("Show clipping rectangle when hovering ImDrawCmd node", &show_drawcmd_clip_rects);
        ImGui::TreePop();
    }

    // Tool: Display windows Rectangles and Begin Order
    if (show_windows_rects || show_windows_begin_order)
    {
        for (int n = 0; n < g.Windows.Size; n++)
        {
            ImGuiWindow* window = g.Windows[n];
            if (!window->WasActive)
                continue;
            ImDrawList* draw_list = GetForegroundDrawList(window);
            if (show_windows_rects)
            {
                ImRect r = Funcs::GetWindowRect(window, show_windows_rect_type);
                draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
            }
            if (show_windows_begin_order && !(window->Flags & ImGuiWindowFlags_ChildWindow))
            {
                char buf[32];
                ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", window->BeginOrderWithinContext);
                float font_size = ImGui::GetFontSize();
                draw_list->AddRectFilled(window->Pos, window->Pos + ImVec2(font_size, font_size), IM_COL32(200, 100, 100, 255));
                draw_list->AddText(window->Pos, IM_COL32(255, 255, 255, 255), buf);
            }
        }
    }

    if (show_docking_nodes && g.IO.KeyCtrl)
    {
        for (int n = 0; n < g.DockContext->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)g.DockContext->Nodes.Data[n].val_p)
            {
                ImGuiDockNode* root_node = DockNodeGetRootNode(node);
                if (ImGuiDockNode* hovered_node = DockNodeTreeFindNodeByPos(root_node, g.IO.MousePos))
                    if (hovered_node != node)
                        continue;
                char buf[64] = "";
                char* p = buf;
                ImDrawList* overlay_draw_list = node->HostWindow ? GetForegroundDrawList(node->HostWindow) : GetForegroundDrawList((ImGuiViewportP*)GetMainViewport());
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "DockId: %X%s\n", node->ID, node->IsCentralNode() ? " *CentralNode*" : "");
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "WindowClass: %08X\n", node->WindowClass.ClassId);
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "Size: (%.0f, %.0f)\n", node->Size.x, node->Size.y);
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "SizeRef: (%.0f, %.0f)\n", node->SizeRef.x, node->SizeRef.y);
                int depth = DockNodeGetDepth(node);
                overlay_draw_list->AddRect(node->Pos + ImVec2(3, 3) * (float)depth, node->Pos + node->Size - ImVec2(3, 3) * (float)depth, IM_COL32(200, 100, 100, 255));
                ImVec2 pos = node->Pos + ImVec2(3, 3) * (float)depth;
                overlay_draw_list->AddRectFilled(pos - ImVec2(1, 1), pos + CalcTextSize(buf) + ImVec2(1, 1), IM_COL32(200, 100, 100, 255));
                overlay_draw_list->AddText(NULL, 0.0f, pos, IM_COL32(255, 255, 255, 255), buf);
            }
    }

    ImGui::End();
}

#else

void ImGui::ShowMetricsWindow(bool*) { }

#endif

//-----------------------------------------------------------------------------

// Include imgui_user.inl at the end of imgui.cpp to access private data/functions that aren't exposed.
// Prefer just including imgui_internal.h from your code rather than using this define. If a declaration is missing from imgui_internal.h add it or request it on the github.
#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

//-----------------------------------------------------------------------------
