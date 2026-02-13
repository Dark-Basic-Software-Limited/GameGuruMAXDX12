void ImGui::DockContextProcessDock(ImGuiContext* ctx, ImGuiDockRequest* req)
{
    IM_ASSERT((req->Type == ImGuiDockRequestType_Dock && req->DockPayload != NULL) || (req->Type == ImGuiDockRequestType_Split && req->DockPayload == NULL));
    IM_ASSERT(req->DockTargetWindow != NULL || req->DockTargetNode != NULL);

    ImGuiContext& g = *ctx;
    IM_UNUSED(g);

    ImGuiWindow* payload_window = req->DockPayload;     // Optional
    ImGuiWindow* target_window = req->DockTargetWindow;
    ImGuiDockNode* node = req->DockTargetNode;
    if (payload_window)
        IMGUI_DEBUG_LOG_DOCKING("DockContextProcessDock node 0x%08X target '%s' dock window '%s', split_dir %d\n", node ? node->ID : 0, target_window ? target_window->Name : "NULL", payload_window ? payload_window->Name : "NULL", req->DockSplitDir);
    else
        IMGUI_DEBUG_LOG_DOCKING("DockContextProcessDock node 0x%08X, split_dir %d\n", node ? node->ID : 0, req->DockSplitDir);

    // Decide which Tab will be selected at the end of the operation
    ImGuiID next_selected_id = 0;
    ImGuiDockNode* payload_node = NULL;
    if (payload_window)
    {
        payload_node = payload_window->DockNodeAsHost;
        payload_window->DockNodeAsHost = NULL; // Important to clear this as the node will have its life as a child which might be merged/deleted later.
        if (payload_node && payload_node->IsLeafNode())
            next_selected_id = payload_node->TabBar->NextSelectedTabId ? payload_node->TabBar->NextSelectedTabId : payload_node->TabBar->SelectedTabId;
        if (payload_node == NULL)
            next_selected_id = payload_window->ID;
    }

    // FIXME-DOCK: When we are trying to dock an existing single-window node into a loose window, transfer Node ID as well
    // When processing an interactive split, usually LastFrameAlive will be < g.FrameCount. But DockBuilder operations can make it ==.
    if (node)
        IM_ASSERT(node->LastFrameAlive <= g.FrameCount);
    if (node && target_window && node == target_window->DockNodeAsHost)
        IM_ASSERT(node->Windows.Size > 0 || node->IsSplitNode() || node->IsCentralNode());

    // Create new node and add existing window to it
    if (node == NULL)
    {
        node = DockContextAddNode(ctx, 0);
        node->Pos = target_window->Pos;
        node->Size = target_window->Size;
        if (target_window->DockNodeAsHost == NULL)
        {
            DockNodeAddWindow(node, target_window, true);
            node->TabBar->Tabs[0].Flags &= ~ImGuiTabItemFlags_Unsorted;
            target_window->DockIsActive = true;
        }
    }

    ImGuiDir split_dir = req->DockSplitDir;
    if (split_dir != ImGuiDir_None)
    {
        // Split into one, one side will be our payload node unless we are dropping a loose window
        const ImGuiAxis split_axis = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
        const int split_inheritor_child_idx = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 1 : 0; // Current contents will be moved to the opposite side
        const float split_ratio = req->DockSplitRatio;
        DockNodeTreeSplit(ctx, node, split_axis, split_inheritor_child_idx, split_ratio, payload_node);  // payload_node may be NULL here!
        ImGuiDockNode* new_node = node->ChildNodes[split_inheritor_child_idx ^ 1];
        new_node->HostWindow = node->HostWindow;
        node = new_node;
    }
    node->LocalFlags &= ~ImGuiDockNodeFlags_HiddenTabBar;

    if (node != payload_node)
    {
        // Create tab bar before we call DockNodeMoveWindows (which would attempt to move the old tab-bar, which would lead us to payload tabs wrongly appearing before target tabs!)
        if (node->Windows.Size > 0 && node->TabBar == NULL)
        {
            DockNodeAddTabBar(node);
            for (int n = 0; n < node->Windows.Size; n++)
                TabBarAddTab(node->TabBar, ImGuiTabItemFlags_None, node->Windows[n]);
        }

        if (payload_node != NULL)
        {
            // Transfer full payload node (with 1+ child windows or child nodes)
            if (payload_node->IsSplitNode())
            {
                if (node->Windows.Size > 0)
                {
                    // We can dock a split payload into a node that already has windows _only_ if our payload is a node tree with a single visible node.
                    // In this situation, we move the windows of the target node into the currently visible node of the payload.
                    // This allows us to preserve some of the underlying dock tree settings nicely.
                    IM_ASSERT(payload_node->OnlyNodeWithWindows != NULL); // The docking should have been blocked by DockNodePreviewDockCalc() early on and never submitted.
                    ImGuiDockNode* visible_node = payload_node->OnlyNodeWithWindows;
                    if (visible_node->TabBar)
                        IM_ASSERT(visible_node->TabBar->Tabs.Size > 0);
                    DockNodeMoveWindows(node, visible_node);
                    DockNodeMoveWindows(visible_node, node);
                    DockSettingsRenameNodeReferences(node->ID, visible_node->ID);
                }
                if (node->IsCentralNode())
                {
                    // Central node property needs to be moved to a leaf node, pick the last focused one.
                    // FIXME-DOCKING: If we had to transfer other flags here, what would the policy be?
                    ImGuiDockNode* last_focused_node = DockContextFindNodeByID(ctx, payload_node->LastFocusedNodeID);
                    IM_ASSERT(last_focused_node != NULL);
                    ImGuiDockNode* last_focused_root_node = DockNodeGetRootNode(last_focused_node);
                    IM_ASSERT(last_focused_root_node == DockNodeGetRootNode(payload_node));
                    last_focused_node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
                    node->LocalFlags &= ~ImGuiDockNodeFlags_CentralNode;
                    last_focused_root_node->CentralNode = last_focused_node;
                }

                IM_ASSERT(node->Windows.Size == 0);
                DockNodeMoveChildNodes(node, payload_node);
            }
            else
            {
                const ImGuiID payload_dock_id = payload_node->ID;
                DockNodeMoveWindows(node, payload_node);
                DockSettingsRenameNodeReferences(payload_dock_id, node->ID);
            }
            DockContextRemoveNode(ctx, payload_node, true);
        }
        else if (payload_window)
        {
            // Transfer single window
            const ImGuiID payload_dock_id = payload_window->DockId;
            node->VisibleWindow = payload_window;
            DockNodeAddWindow(node, payload_window, true);
            if (payload_dock_id != 0)
                DockSettingsRenameNodeReferences(payload_dock_id, node->ID);
        }
    }
    else
    {
        // When docking a floating single window node we want to reevaluate auto-hiding of the tab bar
        node->WantHiddenTabBarUpdate = true;
    }

    // Update selection immediately
    if (ImGuiTabBar* tab_bar = node->TabBar)
        tab_bar->NextSelectedTabId = next_selected_id;
    MarkIniSettingsDirty();
}

void ImGui::DockContextProcessUndockWindow(ImGuiContext* ctx, ImGuiWindow* window, bool clear_persistent_docking_ref)
{
    (void)ctx;
    if (window->DockNode)
        DockNodeRemoveWindow(window->DockNode, window, clear_persistent_docking_ref ? 0 : window->DockId);
    else
        window->DockId = 0;
    window->Collapsed = false;
    window->DockIsActive = false;
    window->DockTabIsVisible = false;
    MarkIniSettingsDirty();
}

void ImGui::DockContextProcessUndockNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    IM_ASSERT(node->IsLeafNode());
    IM_ASSERT(node->Windows.Size >= 1);

    if (node->IsRootNode() || node->IsCentralNode())
    {
        // In the case of a root node or central node, the node will have to stay in place. Create a new node to receive the payload.
        ImGuiDockNode* new_node = DockContextAddNode(ctx, 0);
        DockNodeMoveWindows(new_node, node);
        DockSettingsRenameNodeReferences(node->ID, new_node->ID);
        for (int n = 0; n < new_node->Windows.Size; n++)
            UpdateWindowParentAndRootLinks(new_node->Windows[n], new_node->Windows[n]->Flags, NULL);
        new_node->AuthorityForPos = new_node->AuthorityForSize = ImGuiDataAuthority_Window;
        new_node->WantMouseMove = true;
    }
    else
    {
        // Otherwise extract our node and merging our sibling back into the parent node.
        IM_ASSERT(node->ParentNode->ChildNodes[0] == node || node->ParentNode->ChildNodes[1] == node);
        int index_in_parent = (node->ParentNode->ChildNodes[0] == node) ? 0 : 1;
        node->ParentNode->ChildNodes[index_in_parent] = NULL;
        DockNodeTreeMerge(ctx, node->ParentNode, node->ParentNode->ChildNodes[index_in_parent ^ 1]);
        node->ParentNode->AuthorityForViewport = ImGuiDataAuthority_Window; // The node that stays in place keeps the viewport, so our newly dragged out node will create a new viewport
        node->ParentNode = NULL;
        node->AuthorityForPos = node->AuthorityForSize = ImGuiDataAuthority_Window;
        node->WantMouseMove = true;
    }
    MarkIniSettingsDirty();
}

// This is mostly used for automation.
bool ImGui::DockContextCalcDropPosForDocking(ImGuiWindow* target, ImGuiDockNode* target_node, ImGuiWindow* payload, ImGuiDir split_dir, bool split_outer, ImVec2* out_pos)
{
    if (split_outer)
    {
        IM_ASSERT(0);
    }
    else
    {
        ImGuiDockPreviewData split_data;
        DockNodePreviewDockCalc(target, target_node, payload, &split_data, false, split_outer);
        if (split_data.DropRectsDraw[split_dir+1].IsInverted())
            return false;
        *out_pos = split_data.DropRectsDraw[split_dir+1].GetCenter();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockNode
//-----------------------------------------------------------------------------
// - DockNodeGetTabOrder()
// - DockNodeAddWindow()
// - DockNodeRemoveWindow()
// - DockNodeMoveChildNodes()
// - DockNodeMoveWindows()
// - DockNodeApplyPosSizeToWindows()
// - DockNodeHideHostWindow()
// - ImGuiDockNodeFindInfoResults
// - DockNodeFindInfo()
// - DockNodeFindWindowByID()
// - DockNodeUpdateVisibleFlagAndInactiveChilds()
// - DockNodeUpdateVisibleFlag()
// - DockNodeStartMouseMovingWindow()
// - DockNodeUpdate()
// - DockNodeUpdateWindowMenu()
// - DockNodeUpdateTabBar()
// - DockNodeAddTabBar()
// - DockNodeRemoveTabBar()
// - DockNodeIsDropAllowedOne()
// - DockNodeIsDropAllowed()
// - DockNodeCalcTabBarLayout()
// - DockNodeCalcSplitRects()
// - DockNodeCalcDropRectsAndTestMousePos()
// - DockNodePreviewDockCalc()
// - DockNodePreviewDockRender()
//-----------------------------------------------------------------------------

ImGuiDockNode::ImGuiDockNode(ImGuiID id)
{
    ID = id;
    SharedFlags = LocalFlags = ImGuiDockNodeFlags_None;
    ParentNode = ChildNodes[0] = ChildNodes[1] = NULL;
    TabBar = NULL;
    SplitAxis = ImGuiAxis_None;

    State = ImGuiDockNodeState_Unknown;
    HostWindow = VisibleWindow = NULL;
    CentralNode = OnlyNodeWithWindows = NULL;
    LastFrameAlive = LastFrameActive = LastFrameFocused = -1;
    LastFocusedNodeID = 0;
    SelectedTabID = 0;
    WantCloseTabID = 0;
    AuthorityForPos = AuthorityForSize = ImGuiDataAuthority_DockNode;
    AuthorityForViewport = ImGuiDataAuthority_Auto;
    IsVisible = true;
    IsFocused = HasCloseButton = HasWindowMenuButton = EnableCloseButton = false;
    WantCloseAll = WantLockSizeOnce = WantMouseMove = WantHiddenTabBarUpdate = WantHiddenTabBarToggle = false;
    MarkedForPosSizeWrite = false;
}

ImGuiDockNode::~ImGuiDockNode()
{
    IM_DELETE(TabBar);
    TabBar = NULL;
    ChildNodes[0] = ChildNodes[1] = NULL;
}

int ImGui::DockNodeGetTabOrder(ImGuiWindow* window)
{
    ImGuiTabBar* tab_bar = window->DockNode->TabBar;
    if (tab_bar == NULL)
        return -1;
    ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, window->ID);
    return tab ? tab_bar->GetTabOrder(tab) : -1;
}

static void ImGui::DockNodeAddWindow(ImGuiDockNode* node, ImGuiWindow* window, bool add_to_tab_bar)
{
    ImGuiContext& g = *GImGui; (void)g;
    if (window->DockNode)
    {
        // Can overwrite an existing window->DockNode (e.g. pointing to a disabled DockSpace node)
        IM_ASSERT(window->DockNode->ID != node->ID);
        DockNodeRemoveWindow(window->DockNode, window, 0);
    }
    IM_ASSERT(window->DockNode == NULL || window->DockNodeAsHost == NULL);
    IMGUI_DEBUG_LOG_DOCKING("DockNodeAddWindow node 0x%08X window '%s'\n", node->ID, window->Name);

    node->Windows.push_back(window);
    node->WantHiddenTabBarUpdate = true;
    window->DockNode = node;
    window->DockId = node->ID;
    window->DockIsActive = (node->Windows.Size > 1);
    window->DockTabWantClose = false;

    // If more than 2 windows appeared on the same frame, we'll create a new hosting DockNode from the point of the second window submission.
    // Then we need to hide the first window (after its been output) otherwise it would be visible as a standalone window for one frame.
    if (node->HostWindow == NULL && node->Windows.Size == 2 && node->Windows[0]->WasActive == false)
    {
        node->Windows[0]->Hidden = true;
        node->Windows[0]->HiddenFramesCanSkipItems = 1;
    }

    // When reactivating a node with one or two loose window, the window pos/size/viewport are authoritative over the node storage.
    // In particular it is important we init the viewport from the first window so we don't create two viewports and drop one.
    if (node->HostWindow == NULL && node->IsFloatingNode())
    {
        if (node->AuthorityForPos == ImGuiDataAuthority_Auto)
            node->AuthorityForPos = ImGuiDataAuthority_Window;
        if (node->AuthorityForSize == ImGuiDataAuthority_Auto)
            node->AuthorityForSize = ImGuiDataAuthority_Window;
        if (node->AuthorityForViewport == ImGuiDataAuthority_Auto)
            node->AuthorityForViewport = ImGuiDataAuthority_Window;
    }

    // Add to tab bar if requested
    if (add_to_tab_bar)
    {
        if (node->TabBar == NULL)
        {
            DockNodeAddTabBar(node);
            node->TabBar->SelectedTabId = node->TabBar->NextSelectedTabId = node->SelectedTabID;
            
            // Add existing windows
            for (int n = 0; n < node->Windows.Size - 1; n++)
                TabBarAddTab(node->TabBar, ImGuiTabItemFlags_None, node->Windows[n]);
        }
        TabBarAddTab(node->TabBar, ImGuiTabItemFlags_Unsorted, window);
    }

    DockNodeUpdateVisibleFlag(node);

    // Update this without waiting for the next time we Begin() in the window, so our host window will have the proper title bar color on its first frame.
    if (node->HostWindow)
        UpdateWindowParentAndRootLinks(window, window->Flags | ImGuiWindowFlags_ChildWindow, node->HostWindow);
}

static void ImGui::DockNodeRemoveWindow(ImGuiDockNode* node, ImGuiWindow* window, ImGuiID save_dock_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window->DockNode == node);
    //IM_ASSERT(window->RootWindow == node->HostWindow);
    //IM_ASSERT(window->LastFrameActive < g.FrameCount);    // We may call this from Begin()
    IM_ASSERT(save_dock_id == 0 || save_dock_id == node->ID);
    IMGUI_DEBUG_LOG_DOCKING("DockNodeRemoveWindow node 0x%08X window '%s'\n", node->ID, window->Name);

    window->DockNode = NULL;
    window->DockIsActive = window->DockTabWantClose = false;
    window->DockId = save_dock_id;
    UpdateWindowParentAndRootLinks(window, window->Flags & ~ImGuiWindowFlags_ChildWindow, NULL); // Update immediately

    // Remove window
    bool erased = false;
    for (int n = 0; n < node->Windows.Size; n++)
        if (node->Windows[n] == window)
        {
            node->Windows.erase(node->Windows.Data + n);
            erased = true;
            break;
        }
    IM_ASSERT(erased);
    if (node->VisibleWindow == window)
        node->VisibleWindow = NULL;

    // Remove tab and possibly tab bar
    node->WantHiddenTabBarUpdate = true;
    if (node->TabBar)
    {
        TabBarRemoveTab(node->TabBar, window->ID);
        const int tab_count_threshold_for_tab_bar = node->IsCentralNode() ? 1 : 2;
        if (node->Windows.Size < tab_count_threshold_for_tab_bar)
            DockNodeRemoveTabBar(node);
    }

    if (node->Windows.Size == 0 && !node->IsCentralNode() && !node->IsDockSpace() && window->DockId != node->ID)
    {
        // Automatic dock node delete themselves if they are not holding at least one tab
        DockContextRemoveNode(&g, node, true);
        return;
    }

    if (node->Windows.Size == 1 && !node->IsCentralNode() && node->HostWindow)
    {
        ImGuiWindow* remaining_window = node->Windows[0];
        if (node->HostWindow->ViewportOwned && node->IsRootNode())
        {
            // Transfer viewport back to the remaining loose window
            IM_ASSERT(node->HostWindow->Viewport->Window == node->HostWindow);
            node->HostWindow->Viewport->Window = remaining_window;
            node->HostWindow->Viewport->ID = remaining_window->ID;
        }
        remaining_window->Collapsed = node->HostWindow->Collapsed;
    }

    // Update visibility immediately is required so the DockNodeUpdateRemoveInactiveChilds() processing can reflect changes up the tree
    DockNodeUpdateVisibleFlag(node);
}

static void ImGui::DockNodeMoveChildNodes(ImGuiDockNode* dst_node, ImGuiDockNode* src_node)
{
    IM_ASSERT(dst_node->Windows.Size == 0);
    dst_node->ChildNodes[0] = src_node->ChildNodes[0];
    dst_node->ChildNodes[1] = src_node->ChildNodes[1];
    if (dst_node->ChildNodes[0])
        dst_node->ChildNodes[0]->ParentNode = dst_node;
    if (dst_node->ChildNodes[1])
        dst_node->ChildNodes[1]->ParentNode = dst_node;
    dst_node->SplitAxis = src_node->SplitAxis;
    dst_node->SizeRef = src_node->SizeRef;
    src_node->ChildNodes[0] = src_node->ChildNodes[1] = NULL;
}

static void ImGui::DockNodeMoveWindows(ImGuiDockNode* dst_node, ImGuiDockNode* src_node)
{
    // Insert tabs in the same orders as currently ordered (node->Windows isn't ordered)
    IM_ASSERT(src_node && dst_node && dst_node != src_node);
    ImGuiTabBar* src_tab_bar = src_node->TabBar;
    if (src_tab_bar != NULL)
        IM_ASSERT(src_node->Windows.Size == src_node->TabBar->Tabs.Size);

    // If the dst_node is empty we can just move the entire tab bar (to preserve selection, scrolling, etc.)
    bool move_tab_bar = (src_tab_bar != NULL) && (dst_node->TabBar == NULL);
    if (move_tab_bar)
    {
        dst_node->TabBar = src_node->TabBar;
        src_node->TabBar = NULL;
    }

    for (int n = 0; n < src_node->Windows.Size; n++)
    {
        ImGuiWindow* window = src_tab_bar ? src_tab_bar->Tabs[n].Window : src_node->Windows[n];
        window->DockNode = NULL;
        window->DockIsActive = false;
        DockNodeAddWindow(dst_node, window, move_tab_bar ? false : true);
    }
    src_node->Windows.clear();

    if (!move_tab_bar && src_node->TabBar)
    {
        if (dst_node->TabBar)
            dst_node->TabBar->SelectedTabId = src_node->TabBar->SelectedTabId;
        DockNodeRemoveTabBar(src_node);
    }
}

static void ImGui::DockNodeApplyPosSizeToWindows(ImGuiDockNode* node)
{
    for (int n = 0; n < node->Windows.Size; n++)
    {
        SetWindowPos(node->Windows[n], node->Pos, ImGuiCond_Always); // We don't assign directly to Pos because it can break the calculation of SizeContents on next frame
        SetWindowSize(node->Windows[n], node->Size, ImGuiCond_Always);
    }
}

static void ImGui::DockNodeHideHostWindow(ImGuiDockNode* node)
{
    if (node->HostWindow)
    {
        if (node->HostWindow->DockNodeAsHost == node)
            node->HostWindow->DockNodeAsHost = NULL;
        node->HostWindow = NULL;
    }

    if (node->Windows.Size == 1)
    {
        node->VisibleWindow = node->Windows[0];
        node->Windows[0]->DockIsActive = false;
    }

    if (node->TabBar)
        DockNodeRemoveTabBar(node);
}

// Search function called once by root node in DockNodeUpdate()
struct ImGuiDockNodeFindInfoResults
{
    ImGuiDockNode*      CentralNode;
    ImGuiDockNode*      FirstNodeWithWindows;
    int                 CountNodesWithWindows;
    //ImGuiWindowClass  WindowClassForMerges;

    ImGuiDockNodeFindInfoResults() { CentralNode = FirstNodeWithWindows = NULL; CountNodesWithWindows = 0; }
};

static void DockNodeFindInfo(ImGuiDockNode* node, ImGuiDockNodeFindInfoResults* results)
{
    if (node->Windows.Size > 0)
    {
        if (results->FirstNodeWithWindows == NULL)
            results->FirstNodeWithWindows = node;
        results->CountNodesWithWindows++;
    }
    if (node->IsCentralNode())
    {
        IM_ASSERT(results->CentralNode == NULL); // Should be only one
        IM_ASSERT(node->IsLeafNode() && "If you get this assert: please submit .ini file + repro of actions leading to this.");
        results->CentralNode = node;
    }
    if (results->CountNodesWithWindows > 1 && results->CentralNode != NULL)
        return;
    if (node->ChildNodes[0])
        DockNodeFindInfo(node->ChildNodes[0], results);
    if (node->ChildNodes[1])
        DockNodeFindInfo(node->ChildNodes[1], results);
}

static ImGuiWindow* ImGui::DockNodeFindWindowByID(ImGuiDockNode* node, ImGuiID id)
{
    IM_ASSERT(id != 0);
    for (int n = 0; n < node->Windows.Size; n++)
        if (node->Windows[n]->ID == id)
            return node->Windows[n];
    return NULL;
}

// - Remove inactive windows/nodes.
// - Update visibility flag.
static void ImGui::DockNodeUpdateVisibleFlagAndInactiveChilds(ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;

    IM_ASSERT(node->ParentNode == NULL || node->ParentNode->ChildNodes[0] == node || node->ParentNode->ChildNodes[1] == node);

    // Inherit most flags
    if (node->ParentNode)
        node->SharedFlags = node->ParentNode->SharedFlags & ImGuiDockNodeFlags_SharedFlagsInheritMask_;

    // Recurse into children
    // There is the possibility that one of our child becoming empty will delete itself and moving its sibling contents into 'node'.
    // If 'node->ChildNode[0]' delete itself, then 'node->ChildNode[1]->Windows' will be moved into 'node'
    // If 'node->ChildNode[1]' delete itself, then 'node->ChildNode[0]->Windows' will be moved into 'node' and the "remove inactive windows" loop will have run twice on those windows (harmless)
    if (node->ChildNodes[0])
        DockNodeUpdateVisibleFlagAndInactiveChilds(node->ChildNodes[0]);
    if (node->ChildNodes[1])
        DockNodeUpdateVisibleFlagAndInactiveChilds(node->ChildNodes[1]);

    // Remove inactive windows
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        IM_ASSERT(window->DockNode == node);

        bool node_was_active = (node->LastFrameActive + 1 == g.FrameCount);
        bool remove = false;
        remove |= node_was_active && (window->LastFrameActive + 1 < g.FrameCount);
        remove |= node_was_active && (node->WantCloseAll || node->WantCloseTabID == window->ID) && window->HasCloseButton && !(window->Flags & ImGuiWindowFlags_UnsavedDocument);  // Submit all _expected_ closure from last frame
        remove |= (window->DockTabWantClose);
        if (!remove)
            continue;
        window->DockTabWantClose = false;
        if (node->Windows.Size == 1 && !node->IsCentralNode())
        {
            DockNodeHideHostWindow(node);
            node->State = ImGuiDockNodeState_HostWindowHiddenBecauseSingleWindow;
            DockNodeRemoveWindow(node, window, node->ID); // Will delete the node so it'll be invalid on return
            return;
        }
        DockNodeRemoveWindow(node, window, node->ID);
        window_n--;
    }

    // Auto-hide tab bar option
    ImGuiDockNodeFlags node_flags = node->GetMergedFlags();
    if (node->WantHiddenTabBarUpdate && node->Windows.Size == 1 && (node_flags & ImGuiDockNodeFlags_AutoHideTabBar) && !node->IsHiddenTabBar())
        node->WantHiddenTabBarToggle = true;
    node->WantHiddenTabBarUpdate = false;

    // Apply toggles at a single point of the frame (here!)
    if (node->Windows.Size > 1)
        node->LocalFlags &= ~ImGuiDockNodeFlags_HiddenTabBar;
    else if (node->WantHiddenTabBarToggle)
        node->LocalFlags ^= ImGuiDockNodeFlags_HiddenTabBar;
    node->WantHiddenTabBarToggle = false;

    DockNodeUpdateVisibleFlag(node);
}

static void ImGui::DockNodeUpdateVisibleFlag(ImGuiDockNode* node)
{
    // Update visibility flag
    bool is_visible = (node->ParentNode == NULL) ? node->IsDockSpace() : node->IsCentralNode();
    is_visible |= (node->Windows.Size > 0);
    is_visible |= (node->ChildNodes[0] && node->ChildNodes[0]->IsVisible);
    is_visible |= (node->ChildNodes[1] && node->ChildNodes[1]->IsVisible);
    node->IsVisible = is_visible;
}

static void ImGui::DockNodeStartMouseMovingWindow(ImGuiDockNode* node, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->WantMouseMove == true);
    ImVec2 backup_active_click_offset = g.ActiveIdClickOffset;
    StartMouseMovingWindow(window);
    g.MovingWindow = window; // If we are docked into a non moveable root window, StartMouseMovingWindow() won't set g.MovingWindow. Override that decision.
    node->WantMouseMove = false;
    g.ActiveIdClickOffset = backup_active_click_offset;
}

static void ImGui::DockNodeUpdate(ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->LastFrameActive != g.FrameCount);
    node->LastFrameAlive = g.FrameCount;
    node->MarkedForPosSizeWrite = false;

    node->CentralNode = node->OnlyNodeWithWindows = NULL;
    if (node->IsRootNode())
    {
        DockNodeUpdateVisibleFlagAndInactiveChilds(node);

        // FIXME-DOCK: Merge this scan into the one above.
        // - Setup central node pointers
        // - Find if there's only a single visible window in the hierarchy (in which case we need to display a regular title bar -> FIXME-DOCK: that last part is not done yet!)
        ImGuiDockNodeFindInfoResults results;
        DockNodeFindInfo(node, &results);
        node->CentralNode = results.CentralNode;
        node->OnlyNodeWithWindows = (results.CountNodesWithWindows == 1) ? results.FirstNodeWithWindows : NULL;
        if (node->LastFocusedNodeID == 0 && results.FirstNodeWithWindows != NULL)
            node->LastFocusedNodeID = results.FirstNodeWithWindows->ID;

        // Copy the window class from of our first window so it can be used for proper dock filtering.
        // When node has mixed windows, prioritize the class with the most constraint (DockingAllowUnclassed = false) as the reference to copy.
        // FIXME-DOCK: We don't recurse properly, this code could be reworked to work from DockNodeUpdateScanRec.
        if (ImGuiDockNode* first_node_with_windows = results.FirstNodeWithWindows)
        {
            node->WindowClass = first_node_with_windows->Windows[0]->WindowClass;
            for (int n = 1; n < first_node_with_windows->Windows.Size; n++)
                if (first_node_with_windows->Windows[n]->WindowClass.DockingAllowUnclassed == false)
                {
                    node->WindowClass = first_node_with_windows->Windows[n]->WindowClass;
                    break;
                }
        }
    }

    // Remove tab bar if not needed
    if (node->TabBar && node->IsNoTabBar())
        DockNodeRemoveTabBar(node);

    // Early out for hidden root dock nodes (when all DockId references are in inactive windows, or there is only 1 floating window holding on the DockId)
    bool want_to_hide_host_window = false;
    if (node->Windows.Size <= 1 && node->IsFloatingNode() && node->IsLeafNode())
        if (!g.IO.ConfigDockingAlwaysTabBar && (node->Windows.Size == 0 || !node->Windows[0]->WindowClass.DockingAlwaysTabBar))
            want_to_hide_host_window = true;
    if (want_to_hide_host_window)
    {
        if (node->Windows.Size == 1)
        {
            // Floating window pos/size is authoritative
            ImGuiWindow* single_window = node->Windows[0];
            node->Pos = single_window->Pos;
            node->Size = single_window->SizeFull;
            node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Window;

            // Transfer focus immediately so when we revert to a regular window it is immediately selected
            if (node->HostWindow && g.NavWindow == node->HostWindow)
                FocusWindow(single_window);
            if (node->HostWindow)
            {
                single_window->Viewport = node->HostWindow->Viewport;
                single_window->ViewportId = node->HostWindow->ViewportId;
                if (node->HostWindow->ViewportOwned)
                {
                    single_window->Viewport->Window = single_window;
                    single_window->ViewportOwned = true;
                }
            }
        }

        DockNodeHideHostWindow(node);
        node->State = ImGuiDockNodeState_HostWindowHiddenBecauseSingleWindow;
        node->WantCloseAll = false;
        node->WantCloseTabID = 0;
        node->HasCloseButton = node->HasWindowMenuButton = node->EnableCloseButton = false;
        node->LastFrameActive = g.FrameCount;

        if (node->WantMouseMove && node->Windows.Size == 1)
            DockNodeStartMouseMovingWindow(node, node->Windows[0]);
        return;
    }

    // In some circumstance we will defer creating the host window (so everything will be kept hidden),
    // while the expected visible window is resizing itself.
    // This is important for first-time (no ini settings restored) single window when io.ConfigDockingAlwaysTabBar is enabled,
    // otherwise the node ends up using the minimum window size. Effectively those windows will take an extra frame to show up:
    //   N+0: Begin(): window created (with no known size), node is created
    //   N+1: DockNodeUpdate(): node skip creating host window / Begin(): window size applied, not visible
    //   N+2: DockNodeUpdate(): node can create host window / Begin(): window becomes visible
    // We could remove this frame if we could reliably calculate the expected window size during node update, before the Begin() code.
    // It would require a generalization of CalcWindowExpectedSize(), probably extracting code away from Begin().
    // In reality it isn't very important as user quickly ends up with size data in .ini file. 
    if (node->IsVisible && node->HostWindow == NULL && node->IsFloatingNode() && node->IsLeafNode())
    {
        IM_ASSERT(node->Windows.Size > 0);
        ImGuiWindow* ref_window = NULL;
        if (node->SelectedTabID != 0) // Note that we prune single-window-node settings on .ini loading, so this is generally 0 for them!
            ref_window = DockNodeFindWindowByID(node, node->SelectedTabID);
        if (ref_window == NULL)
            ref_window = node->Windows[0];
        if (ref_window->AutoFitFramesX > 0 || ref_window->AutoFitFramesY > 0)
        {
            node->State = ImGuiDockNodeState_HostWindowHiddenBecauseWindowsAreResizing;
            return;
        }
    }

    const ImGuiDockNodeFlags node_flags = node->GetMergedFlags();

    // Bind or create host window
    ImGuiWindow* host_window = NULL;
    bool beginned_into_host_window = false;
    if (node->IsDockSpace())
    {
        // [Explicit root dockspace node]
        IM_ASSERT(node->HostWindow);
        node->EnableCloseButton = false;
        node->HasCloseButton = (node_flags & ImGuiDockNodeFlags_NoCloseButton) == 0;
        node->HasWindowMenuButton = (node_flags & ImGuiDockNodeFlags_NoWindowMenuButton) == 0;
        host_window = node->HostWindow;
    }
    else
    {
        // [Automatic root or child nodes]
        node->EnableCloseButton = false;
        node->HasCloseButton = (node->Windows.Size > 0) && (node_flags & ImGuiDockNodeFlags_NoWindowMenuButton) == 0;
        node->HasWindowMenuButton = (node->Windows.Size > 0) && (node_flags & ImGuiDockNodeFlags_NoWindowMenuButton) == 0;
        for (int window_n = 0; window_n < node->Windows.Size; window_n++)
        {
            // FIXME-DOCK: Setting DockIsActive here means that for single active window in a leaf node, DockIsActive will be cleared until the next Begin() call.
            ImGuiWindow* window = node->Windows[window_n];
            window->DockIsActive = (node->Windows.Size > 1);
            node->EnableCloseButton |= window->HasCloseButton;
        }

        if (node->IsRootNode() && node->IsVisible)
        {
            ImGuiWindow* ref_window = (node->Windows.Size > 0) ? node->Windows[0] : NULL;

            // Sync Pos
            if (node->AuthorityForPos == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowPos(ref_window->Pos);
            else if (node->AuthorityForPos == ImGuiDataAuthority_DockNode)
                SetNextWindowPos(node->Pos);

            // Sync Size
            if (node->AuthorityForSize == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowSize(ref_window->SizeFull);
            else if (node->AuthorityForSize == ImGuiDataAuthority_DockNode)
                SetNextWindowSize(node->Size);

            // Sync Collapsed
            if (node->AuthorityForSize == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowCollapsed(ref_window->Collapsed);

            // Sync Viewport
            if (node->AuthorityForViewport == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowViewport(ref_window->ViewportId);

            SetNextWindowClass(&node->WindowClass);

            // Begin into the host window
            char window_label[20];
            DockNodeGetHostWindowTitle(node, window_label, IM_ARRAYSIZE(window_label));
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_DockNodeHost;
            window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
            window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_NoTitleBar;

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            Begin(window_label, NULL, window_flags);
            PopStyleVar();
            beginned_into_host_window = true;

            node->HostWindow = host_window = g.CurrentWindow;
            host_window->DockNodeAsHost = node;
            host_window->DC.CursorPos = host_window->Pos;
            node->Pos = host_window->Pos;
            node->Size = host_window->Size;

            // We set ImGuiWindowFlags_NoFocusOnAppearing because we don't want the host window to take full focus (e.g. steal NavWindow)
            // But we still it bring it to the front of display. There's no way to choose this precise behavior via window flags.
            // One simple case to ponder if: window A has a toggle to create windows B/C/D. Dock B/C/D together, clear the toggle and enable it again.
            // When reappearing B/C/D will request focus and be moved to the top of the display pile, but they are not linked to the dock host window 
            // during the frame they appear. The dock host window would keep its old display order, and the sorting in EndFrame would move B/C/D back
            // after the dock host window, losing their top-most status.
            if (node->HostWindow->Appearing)
                BringWindowToDisplayFront(node->HostWindow);

            node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Auto;
        }
        else if (node->ParentNode)
        {
            node->HostWindow = host_window = node->ParentNode->HostWindow;
            node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Auto;
        }
        if (node->WantMouseMove && node->HostWindow)
            DockNodeStartMouseMovingWindow(node, node->HostWindow);
    }

    // Update focused node (the one whose title bar is highlight) within a node tree
    if (node->IsSplitNode())
        IM_ASSERT(node->TabBar == NULL);
    if (node->IsRootNode())
        if (g.NavWindow && g.NavWindow->RootWindowDockStop->DockNode && g.NavWindow->RootWindowDockStop->ParentWindow == host_window)
            node->LastFocusedNodeID = g.NavWindow->RootWindowDockStop->DockNode->ID;

    // We need to draw a background at the root level if requested by ImGuiDockNodeFlags_PassthruCentralNode, but we will only know the correct pos/size after
    // processing the resizing splitters. So we are using the DrawList channel splitting facility to submit drawing primitives out of order!
    const bool render_dockspace_bg = node->IsRootNode() && host_window && (node_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0;
    if (render_dockspace_bg)
    {
        host_window->DrawList->ChannelsSplit(2);
        host_window->DrawList->ChannelsSetCurrent(1);
    }

    // Register a hit-test hole in the window unless we are currently dragging a window that is compatible with our dockspace
    const ImGuiDockNode* central_node = node->CentralNode;
    const bool central_node_hole = node->IsRootNode() && host_window && (node_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0 && central_node != NULL && central_node->IsEmpty();
    bool central_node_hole_register_hit_test_hole = central_node_hole;
    if (central_node_hole)
        if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
            if (payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) && DockNodeIsDropAllowed(host_window, *(ImGuiWindow**)payload->Data))
                central_node_hole_register_hit_test_hole = false;
    if (central_node_hole_register_hit_test_hole)
    {
        // Add a little padding to match the "resize from edges" behavior and allow grabbing the splitter easily.
        IM_ASSERT(node->IsDockSpace()); // We cannot pass this flag without the DockSpace() api. Testing this because we also setup the hole in host_window->ParentNode
        ImRect central_hole(central_node->Pos, central_node->Pos + central_node->Size);
        central_hole.Expand(ImVec2(-WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, -WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS));
        if (central_node_hole && !central_hole.IsInverted())
        {
            SetWindowHitTestHole(host_window, central_hole.Min, central_hole.Max - central_hole.Min);
            SetWindowHitTestHole(host_window->ParentWindow, central_hole.Min, central_hole.Max - central_hole.Min);
        }
    }

    // Update position/size, process and draw resizing splitters
    if (node->IsRootNode() && host_window)
    {
        DockNodeTreeUpdatePosSize(node, host_window->Pos, host_window->Size);
        DockNodeTreeUpdateSplitter(node);
    }

    // Draw empty node background (currently can only be the Central Node)
    if (host_window && node->IsEmpty() && node->IsVisible && !(node_flags & ImGuiDockNodeFlags_PassthruCentralNode))
        host_window->DrawList->AddRectFilled(node->Pos, node->Pos + node->Size, GetColorU32(ImGuiCol_DockingEmptyBg));

    // Draw whole dockspace background if ImGuiDockNodeFlags_PassthruCentralNode if set.
    if (render_dockspace_bg && node->IsVisible)
    {
        host_window->DrawList->ChannelsSetCurrent(0);
        if (central_node_hole)
            RenderRectFilledWithHole(host_window->DrawList, node->Rect(), central_node->Rect(), GetColorU32(ImGuiCol_WindowBg), 0.0f);
        else
            host_window->DrawList->AddRectFilled(node->Pos, node->Pos + node->Size, GetColorU32(ImGuiCol_WindowBg), 0.0f);
        host_window->DrawList->ChannelsMerge();
    }

    // Draw and populate Tab Bar
    if (host_window && node->Windows.Size > 0)
    {
        DockNodeUpdateTabBar(node, host_window);
    }
    else
    {
        node->WantCloseAll = false;
        node->WantCloseTabID = 0;
        node->IsFocused = false;
    }
    if (node->TabBar && node->TabBar->SelectedTabId)
        node->SelectedTabID = node->TabBar->SelectedTabId;
    else if (node->Windows.Size > 0)
        node->SelectedTabID = node->Windows[0]->ID;

    // Draw payload drop target
    if (host_window && node->IsVisible)
        if (node->IsRootNode() && (g.MovingWindow == NULL || g.MovingWindow->RootWindow != host_window))
            BeginAsDockableDragDropTarget(host_window);

    // We update this after DockNodeUpdateTabBar()
    node->LastFrameActive = g.FrameCount;

    // Recurse into children
    // FIXME-DOCK FIXME-OPT: Should not need to recurse into children
    if (host_window)
    {
        if (node->ChildNodes[0])
            DockNodeUpdate(node->ChildNodes[0]);
        if (node->ChildNodes[1])
            DockNodeUpdate(node->ChildNodes[1]);

        // Render outer borders last (after the tab bar)
        if (node->IsRootNode())
            RenderWindowOuterBorders(host_window);
    }

    // End host window
    if (beginned_into_host_window) //-V1020
        End();
}

// Compare TabItem nodes given the last known DockOrder (will persist in .ini file as hint), used to sort tabs when multiple tabs are added on the same frame.
static int IMGUI_CDECL TabItemComparerByDockOrder(const void* lhs, const void* rhs)
{
    ImGuiWindow* a = ((const ImGuiTabItem*)lhs)->Window;
    ImGuiWindow* b = ((const ImGuiTabItem*)rhs)->Window;
    if (int d = ((a->DockOrder == -1) ? INT_MAX : a->DockOrder) - ((b->DockOrder == -1) ? INT_MAX : b->DockOrder))
        return d;
    return (a->BeginOrderWithinContext - b->BeginOrderWithinContext);
}

static ImGuiID ImGui::DockNodeUpdateWindowMenu(ImGuiDockNode* node, ImGuiTabBar* tab_bar)
{
    // Try to position the menu so it is more likely to stays within the same viewport
    ImGuiContext& g = *GImGui;
    ImGuiID ret_tab_id = 0;
    if (g.Style.WindowMenuButtonPosition == ImGuiDir_Left)
        SetNextWindowPos(ImVec2(node->Pos.x, node->Pos.y + GetFrameHeight()), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    else
        SetNextWindowPos(ImVec2(node->Pos.x + node->Size.x, node->Pos.y + GetFrameHeight()), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    if (BeginPopup("#WindowMenu"))
    {
        node->IsFocused = true;
        if (tab_bar->Tabs.Size == 1)
        {
            if (MenuItem("Hide tab bar", NULL, node->IsHiddenTabBar()))
                node->WantHiddenTabBarToggle = true;
        }
        else
        {
            for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
            {
                ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
                IM_ASSERT(tab->Window != NULL);
                if (Selectable(tab->Window->Name, tab->ID == tab_bar->SelectedTabId))
                    ret_tab_id = tab->ID;
                SameLine();
                Text("   ");
            }
        }
        EndPopup();
    }
    return ret_tab_id;
}

static void ImGui::DockNodeUpdateTabBar(ImGuiDockNode* node, ImGuiWindow* host_window)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    const bool node_was_active = (node->LastFrameActive + 1 == g.FrameCount);
    const bool closed_all = node->WantCloseAll && node_was_active;
    const ImGuiID closed_one = node->WantCloseTabID && node_was_active;
    node->WantCloseAll = false;
    node->WantCloseTabID = 0;

    // Decide if we should use a focused title bar color
    bool is_focused = false;
    ImGuiDockNode* root_node = DockNodeGetRootNode(node);
    if (g.NavWindowingTarget)
        is_focused = (g.NavWindowingTarget->DockNode == node);
    else if (g.NavWindow && g.NavWindow->RootWindowForTitleBarHighlight == host_window->RootWindow && root_node->LastFocusedNodeID == node->ID)
        is_focused = true;

    // Hidden tab bar will show a triangle on the upper-left (in Begin)
    if (node->IsHiddenTabBar() || node->IsNoTabBar())
    {
        node->VisibleWindow = (node->Windows.Size > 0) ? node->Windows[0] : NULL;
        node->IsFocused = is_focused;
        if (is_focused)
            node->LastFrameFocused = g.FrameCount;
        if (node->VisibleWindow)
        {
            // Notify root of visible window (used to display title in OS task bar)
            if (is_focused || root_node->VisibleWindow == NULL)
                root_node->VisibleWindow = node->VisibleWindow;
            if (node->TabBar)
                node->TabBar->VisibleTabId = node->VisibleWindow->ID;
        }
        return;
    }

    // Move ourselves to the Menu layer (so we can be accessed by tapping Alt) + undo SkipItems flag in order to draw over the title bar even if the window is collapsed
    bool backup_skip_item = host_window->SkipItems;
    if (!node->IsDockSpace())
    {
        host_window->SkipItems = false;
        host_window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
        host_window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);
    }

    // Use PushOverrideID() instead of PushID() to use the node id _without_ the host window ID.
    // This is to facilitate computing those ID from the outside, and will affect more or less only the ID of the collapse button, popup and tabs,
    // as docked windows themselves will override the stack with their own root ID.
    PushOverrideID(node->ID);
    ImGuiTabBar* tab_bar = node->TabBar;
    bool tab_bar_is_recreated = (tab_bar == NULL); // Tab bar are automatically destroyed when a node gets hidden
    if (tab_bar == NULL)
    {
        DockNodeAddTabBar(node);
        tab_bar = node->TabBar;
    }

    ImGuiID focus_tab_id = 0;
    node->IsFocused = is_focused;

    const ImGuiDockNodeFlags node_flags = node->GetMergedFlags();

	const bool has_window_menu_button = 0;
	const bool has_close_button = 0;
	node->HasWindowMenuButton = false;

    // In a dock node, the Collapse Button turns into the Window Menu button.
    // FIXME-DOCK FIXME-OPT: Could we recycle popups id accross multiple dock nodes?
    if (has_window_menu_button && IsPopupOpen("#WindowMenu"))
    {
        if (ImGuiID tab_id = DockNodeUpdateWindowMenu(node, tab_bar))
            focus_tab_id = tab_bar->NextSelectedTabId = tab_id;
        is_focused |= node->IsFocused;
    }

    // Layout
    ImRect title_bar_rect, tab_bar_rect;
    ImVec2 window_menu_button_pos;
    DockNodeCalcTabBarLayout(node, &title_bar_rect, &tab_bar_rect, &window_menu_button_pos);

    // Title bar
    if (is_focused)
        node->LastFrameFocused = g.FrameCount;
    ImU32 title_bar_col = GetColorU32(host_window->Collapsed ? ImGuiCol_TitleBgCollapsed : is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
    host_window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, host_window->WindowRounding, ImDrawCornerFlags_Top);

    // Docking/Collapse button
    if (has_window_menu_button)
    {
        if (CollapseButton(host_window->GetID("#COLLAPSE"), window_menu_button_pos, node))
            OpenPopup("#WindowMenu");
        if (IsItemActive())
            focus_tab_id = tab_bar->SelectedTabId;
    }

    // Submit new tabs and apply NavWindow focus back to the tab bar. They will be added as Unsorted and sorted below based on relative DockOrder value.
    const int tabs_count_old = tab_bar->Tabs.Size;
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        if (g.NavWindow && g.NavWindow->RootWindowDockStop == window)
            tab_bar->SelectedTabId = window->ID;
        if (TabBarFindTabByID(tab_bar, window->ID) == NULL)
            TabBarAddTab(tab_bar, ImGuiTabItemFlags_Unsorted, window);
    }

    // If multiple tabs are appearing on the same frame, sort them based on their persistent DockOrder value
    int tabs_unsorted_start = tab_bar->Tabs.Size;
    for (int tab_n = tab_bar->Tabs.Size - 1; tab_n >= 0 && (tab_bar->Tabs[tab_n].Flags & ImGuiTabItemFlags_Unsorted); tab_n--)
    {
        // FIXME-DOCKING: Consider only clearing the flag after the tab has been alive for a few consecutive frames, allowing late comers to not break sorting?
        tab_bar->Tabs[tab_n].Flags &= ~ImGuiTabItemFlags_Unsorted;
        tabs_unsorted_start = tab_n;
    }
    if (tab_bar->Tabs.Size > tabs_unsorted_start)
    {
        IMGUI_DEBUG_LOG_DOCKING("In node 0x%08X: %d new appearing tabs:%s\n", node->ID, tab_bar->Tabs.Size - tabs_unsorted_start, (tab_bar->Tabs.Size > tabs_unsorted_start + 1) ? " (will sort)" : "");
        for (int tab_n = tabs_unsorted_start; tab_n < tab_bar->Tabs.Size; tab_n++)
            IMGUI_DEBUG_LOG_DOCKING(" - Tab '%s' Order %d\n", tab_bar->Tabs[tab_n].Window->Name, tab_bar->Tabs[tab_n].Window->DockOrder);
    if (tab_bar->Tabs.Size > tabs_unsorted_start + 1)
        ImQsort(tab_bar->Tabs.Data + tabs_unsorted_start, tab_bar->Tabs.Size - tabs_unsorted_start, sizeof(ImGuiTabItem), TabItemComparerByDockOrder);
    }

    // Selected newly added tabs, or persistent tab ID if the tab bar was just recreated
    if (tab_bar_is_recreated && TabBarFindTabByID(tab_bar, node->SelectedTabID) != NULL)
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = node->SelectedTabID;
    else if (tab_bar->Tabs.Size > tabs_count_old)
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = tab_bar->Tabs.back().Window->ID;

    // Begin tab bar
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs; // | ImGuiTabBarFlags_NoTabListScrollingButtons);
    tab_bar_flags |= ImGuiTabBarFlags_SaveSettings | ImGuiTabBarFlags_DockNode;
    if (!host_window->Collapsed && is_focused)
        tab_bar_flags |= ImGuiTabBarFlags_IsFocused;
    BeginTabBarEx(tab_bar, tab_bar_rect, tab_bar_flags, node);
    //host_window->DrawList->AddRect(tab_bar_rect.Min, tab_bar_rect.Max, IM_COL32(255,0,255,255));

    // Submit actual tabs
    node->VisibleWindow = NULL;
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        if ((closed_all || closed_one == window->ID) && window->HasCloseButton && !(window->Flags & ImGuiWindowFlags_UnsavedDocument))
            continue;
        if (window->LastFrameActive + 1 >= g.FrameCount || !node_was_active)
        {
            ImGuiTabItemFlags tab_item_flags = 0;
            if (window->Flags & ImGuiWindowFlags_UnsavedDocument)
                tab_item_flags |= ImGuiTabItemFlags_UnsavedDocument;
            if (tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)
                tab_item_flags |= ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;

            bool tab_open = true;
            TabItemEx(tab_bar, window->Name, window->HasCloseButton ? &tab_open : NULL, tab_item_flags, window);
            if (!tab_open)
                node->WantCloseTabID = window->ID;
            if (tab_bar->VisibleTabId == window->ID)
                node->VisibleWindow = window;

            // Store last item data so it can be queried with IsItemXXX functions after the user Begin() call
            window->DockTabItemStatusFlags = host_window->DC.LastItemStatusFlags;
            window->DockTabItemRect = host_window->DC.LastItemRect;

            // Update navigation ID on menu layer
            if (g.NavWindow && g.NavWindow->RootWindowDockStop == window && (window->DC.NavLayerActiveMask & (1 << 1)) == 0)
                host_window->NavLastIds[1] = window->ID;
        }
    }

    // Notify root of visible window (used to display title in OS task bar)
    if (node->VisibleWindow)
        if (is_focused || root_node->VisibleWindow == NULL)
            root_node->VisibleWindow = node->VisibleWindow;

    // Close button (after VisibleWindow was updated)
    // Note that VisibleWindow may have been overrided by CTRL+Tabbing, so VisibleWindow->ID may be != from tab_bar->SelectedTabId
    if (has_close_button && node->VisibleWindow)
    {
        if (!node->VisibleWindow->HasCloseButton)
        {
            PushItemFlag(ImGuiItemFlags_Disabled, true);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text] * ImVec4(1.0f,1.0f,1.0f,0.5f));
        }
        const float button_sz = g.FontSize;
        if (CloseButton(host_window->GetID("#CLOSE"), title_bar_rect.GetTR() + ImVec2(-style.FramePadding.x * 2.0f - button_sz, 0.0f)))
            if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_bar->VisibleTabId))
            {
                node->WantCloseTabID = tab->ID;
                TabBarCloseTab(tab_bar, tab);
            }
        //if (IsItemActive())
        //    focus_tab_id = tab_bar->SelectedTabId;
        if (!node->VisibleWindow->HasCloseButton)
        {
            PopStyleColor();
            PopItemFlag();
        }
    }

    // When clicking on the title bar outside of tabs, we still focus the selected tab for that node
    // FIXME: TabItem use AllowItemOverlap so we manually perform a more specific test for now (hovered || held)
    ImGuiID title_bar_id = host_window->GetID("#TITLEBAR");
    if (g.HoveredId == 0 || g.HoveredId == title_bar_id || g.ActiveId == title_bar_id)
    {
        bool held;
        ButtonBehavior(title_bar_rect, title_bar_id, NULL, &held);
        if (held)
        {
            if (IsMouseClicked(0))
                focus_tab_id = tab_bar->SelectedTabId;

            // Forward moving request to selected window
            if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId))
                StartMouseDragFromTitleBar(tab->Window, node, false);
        }
    }

    // Forward focus from host node to selected window
    //if (is_focused && g.NavWindow == host_window && !g.NavWindowingTarget)
    //    focus_tab_id = tab_bar->SelectedTabId;

    // When clicked on a tab we requested focus to the docked child
    // This overrides the value set by "forward focus from host node to selected window".
    if (tab_bar->NextSelectedTabId)
        focus_tab_id = tab_bar->NextSelectedTabId;

    // Apply navigation focus
    if (focus_tab_id != 0)
        if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, focus_tab_id))
        {
            FocusWindow(tab->Window);
            NavInitWindow(tab->Window, false);
        }

    EndTabBar();
    PopID();

    // Restore SkipItems flag
    if (!node->IsDockSpace())
    {
        host_window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        host_window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
        host_window->SkipItems = backup_skip_item;
    }
}

static void ImGui::DockNodeAddTabBar(ImGuiDockNode* node)
{
    IM_ASSERT(node->TabBar == NULL);
    node->TabBar = IM_NEW(ImGuiTabBar);
}

static void ImGui::DockNodeRemoveTabBar(ImGuiDockNode* node)
{
    if (node->TabBar == NULL)
        return;
    IM_DELETE(node->TabBar);
    node->TabBar = NULL;
}

static bool DockNodeIsDropAllowedOne(ImGuiWindow* payload, ImGuiWindow* host_window)
{
    if (host_window->DockNodeAsHost && host_window->DockNodeAsHost->IsDockSpace() && payload->BeginOrderWithinContext < host_window->BeginOrderWithinContext)
        return false;

    ImGuiWindowClass* host_class = host_window->DockNodeAsHost ? &host_window->DockNodeAsHost->WindowClass : &host_window->WindowClass;
    ImGuiWindowClass* payload_class = &payload->WindowClass;
    if (host_class->ClassId != payload_class->ClassId)
    {
        if (host_class->ClassId != 0 && host_class->DockingAllowUnclassed && payload_class->ClassId == 0)
            return true;
        if (payload_class->ClassId != 0 && payload_class->DockingAllowUnclassed && host_class->ClassId == 0)
            return true;
        return false;
    }

    return true;
}

static bool ImGui::DockNodeIsDropAllowed(ImGuiWindow* host_window, ImGuiWindow* root_payload)
{
    if (root_payload->DockNodeAsHost && root_payload->DockNodeAsHost->IsSplitNode())
        return true;

    const int payload_count = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->Windows.Size : 1;
    for (int payload_n = 0; payload_n < payload_count; payload_n++)
    {
        ImGuiWindow* payload = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->Windows[payload_n] : root_payload;
        if (DockNodeIsDropAllowedOne(payload, host_window))
            return true;
    }
    return false;
}

// window menu button == collapse button when not in a dock node.
// FIXME: This is similar to RenderWindowTitleBarContents, may want to share code.
static void ImGui::DockNodeCalcTabBarLayout(const ImGuiDockNode* node, ImRect* out_title_rect, ImRect* out_tab_bar_rect, ImVec2* out_window_menu_button_pos)
{
    ImGuiContext& g = *GImGui;
    ImRect r = ImRect(node->Pos.x, node->Pos.y, node->Pos.x + node->Size.x, node->Pos.y + g.FontSize + g.Style.FramePadding.y * 2.0f);
    if (out_title_rect) { *out_title_rect = r; }

    ImVec2 window_menu_button_pos = r.Min;
    r.Min.x += g.Style.FramePadding.x;
    r.Max.x -= g.Style.FramePadding.x;
    if (node->HasCloseButton)
    {
        r.Max.x -= g.FontSize;// +1.0f; // In DockNodeUpdateTabBar() we currently display a disabled close button even if there is none.
    }
    if (node->HasWindowMenuButton && g.Style.WindowMenuButtonPosition == ImGuiDir_Left)
    {
        r.Min.x += g.FontSize; // + g.Style.ItemInnerSpacing.x; // <-- Adding ItemInnerSpacing makes the title text moves slightly when in a docking tab bar. Instead we adjusted RenderArrowDockMenu()
    }
    else if (node->HasWindowMenuButton && g.Style.WindowMenuButtonPosition == ImGuiDir_Right)
    {
        r.Max.x -= g.FontSize + g.Style.FramePadding.x;
        window_menu_button_pos = ImVec2(r.Max.x, r.Min.y);
    }
    if (out_tab_bar_rect) { *out_tab_bar_rect = r; }
    if (out_window_menu_button_pos) { *out_window_menu_button_pos = window_menu_button_pos; }
}

void ImGui::DockNodeCalcSplitRects(ImVec2& pos_old, ImVec2& size_old, ImVec2& pos_new, ImVec2& size_new, ImGuiDir dir, ImVec2 size_new_desired)
{
    ImGuiContext& g = *GImGui;
    const float dock_spacing = g.Style.ItemInnerSpacing.x;
    const ImGuiAxis axis = (dir == ImGuiDir_Left || dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
    pos_new[axis ^ 1] = pos_old[axis ^ 1];
    size_new[axis ^ 1] = size_old[axis ^ 1];

    // Distribute size on given axis (with a desired size or equally)
    const float w_avail = size_old[axis] - dock_spacing;
    if (size_new_desired[axis] > 0.0f && size_new_desired[axis] <= w_avail * 0.5f)
    {
        size_new[axis] = size_new_desired[axis];
        size_old[axis] = (float)(int)(w_avail - size_new[axis]);
    }
    else
    {
        size_new[axis] = (float)(int)(w_avail * 0.5f);
        size_old[axis] = (float)(int)(w_avail - size_new[axis]);
    }

    // Position each node
    if (dir == ImGuiDir_Right || dir == ImGuiDir_Down)
    {
        pos_new[axis] = pos_old[axis] + size_old[axis] + dock_spacing;
    }
    else if (dir == ImGuiDir_Left || dir == ImGuiDir_Up)
    {
        pos_new[axis] = pos_old[axis];
        pos_old[axis] = pos_new[axis] + size_new[axis] + dock_spacing;
    }
}

// Retrieve the drop rectangles for a given direction or for the center + perform hit testing.
bool ImGui::DockNodeCalcDropRectsAndTestMousePos(const ImRect& parent, ImGuiDir dir, ImRect& out_r, bool outer_docking, ImVec2* test_mouse_pos)
{
    ImGuiContext& g = *GImGui;

    const float parent_smaller_axis = ImMin(parent.GetWidth(), parent.GetHeight());
    const float hs_for_central_nodes = ImMin(g.FontSize * 1.5f, ImMax(g.FontSize * 0.5f, parent_smaller_axis / 8.0f));
    float hs_w; // Half-size, longer axis
    float hs_h; // Half-size, smaller axis
    ImVec2 off; // Distance from edge or center
    if (outer_docking)
    {
        //hs_w = ImFloor(ImClamp(parent_smaller_axis - hs_for_central_nodes * 4.0f, g.FontSize * 0.5f, g.FontSize * 8.0f));
        //hs_h = ImFloor(hs_w * 0.15f);
        //off = ImVec2(ImFloor(parent.GetWidth() * 0.5f - GetFrameHeightWithSpacing() * 1.4f - hs_h), ImFloor(parent.GetHeight() * 0.5f - GetFrameHeightWithSpacing() * 1.4f - hs_h));
        hs_w = ImFloor(hs_for_central_nodes * 1.50f);
        hs_h = ImFloor(hs_for_central_nodes * 0.80f);
        off = ImVec2(ImFloor(parent.GetWidth() * 0.5f - GetFrameHeightWithSpacing() * 0.0f - hs_h), ImFloor(parent.GetHeight() * 0.5f - GetFrameHeightWithSpacing() * 0.0f - hs_h));
    }
    else
    {
        hs_w = ImFloor(hs_for_central_nodes);
        hs_h = ImFloor(hs_for_central_nodes * 0.90f);
        off = ImVec2(ImFloor(hs_w * 2.40f), ImFloor(hs_w * 2.40f));
    }

    ImVec2 c = ImFloor(parent.GetCenter());
    if      (dir == ImGuiDir_None)  { out_r = ImRect(c.x - hs_w, c.y - hs_w,         c.x + hs_w, c.y + hs_w);         }
    else if (dir == ImGuiDir_Up)    { out_r = ImRect(c.x - hs_w, c.y - off.y - hs_h, c.x + hs_w, c.y - off.y + hs_h); }
    else if (dir == ImGuiDir_Down)  { out_r = ImRect(c.x - hs_w, c.y + off.y - hs_h, c.x + hs_w, c.y + off.y + hs_h); }
    else if (dir == ImGuiDir_Left)  { out_r = ImRect(c.x - off.x - hs_h, c.y - hs_w, c.x - off.x + hs_h, c.y + hs_w); }
    else if (dir == ImGuiDir_Right) { out_r = ImRect(c.x + off.x - hs_h, c.y - hs_w, c.x + off.x + hs_h, c.y + hs_w); }

    if (test_mouse_pos == NULL)
        return false;

    ImRect hit_r = out_r;
    if (!outer_docking)
    {
        // Custom hit testing for the 5-way selection, designed to reduce flickering when moving diagonally between sides
        hit_r.Expand(ImFloor(hs_w * 0.30f));
        ImVec2 mouse_delta = (*test_mouse_pos - c);
        float mouse_delta_len2 = ImLengthSqr(mouse_delta);
        float r_threshold_center = hs_w * 1.4f;
        float r_threshold_sides = hs_w * (1.4f + 1.2f);
        if (mouse_delta_len2 < r_threshold_center * r_threshold_center)
            return (dir == ImGuiDir_None);
        if (mouse_delta_len2 < r_threshold_sides * r_threshold_sides)
            return (dir == ImGetDirQuadrantFromDelta(mouse_delta.x, mouse_delta.y));
    }
    return hit_r.Contains(*test_mouse_pos);
}

// host_node may be NULL if the window doesn't have a DockNode already.
static void ImGui::DockNodePreviewDockCalc(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* root_payload, ImGuiDockPreviewData* data, bool is_explicit_target, bool is_outer_docking)
{
    ImGuiContext& g = *GImGui;

    // There is an edge case when docking into a dockspace which only has inactive nodes.
    // In this case DockNodeTreeFindNodeByPos() will have selected a leaf node which is inactive. 
    // Because the inactive leaf node doesn't have proper pos/size yet, we'll use the root node as reference.
    ImGuiDockNode* ref_node_for_rect = (host_node && !host_node->IsVisible) ? DockNodeGetRootNode(host_node) : host_node;
    if (ref_node_for_rect)
        IM_ASSERT(ref_node_for_rect->IsVisible);

    // Build a tentative future node (reuse same structure because it is practical)
    data->FutureNode.HasCloseButton = (host_node ? host_node->HasCloseButton : host_window->HasCloseButton) || (root_payload->HasCloseButton);
    data->FutureNode.HasWindowMenuButton = host_node ? true : ((host_window->Flags & ImGuiWindowFlags_NoCollapse) == 0);
    data->FutureNode.Pos = host_node ? ref_node_for_rect->Pos : host_window->Pos;
    data->FutureNode.Size = host_node ? ref_node_for_rect->Size : host_window->Size;

    // Figure out here we are allowed to dock
    ImGuiDockNodeFlags host_node_flags = host_node ? host_node->GetMergedFlags() : 0;
    const bool src_is_visibly_splitted = root_payload->DockNodeAsHost && root_payload->DockNodeAsHost->IsSplitNode() && (root_payload->DockNodeAsHost->OnlyNodeWithWindows == NULL);
    data->IsCenterAvailable = !is_outer_docking;
    if (src_is_visibly_splitted && (!host_node || !host_node->IsEmpty()))
        data->IsCenterAvailable = false;
    if (host_node && (host_node_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) && host_node->IsCentralNode())
        data->IsCenterAvailable = false;

    data->IsSidesAvailable = true;
    if ((host_node && (host_node_flags & ImGuiDockNodeFlags_NoSplit)) || g.IO.ConfigDockingNoSplit)
        data->IsSidesAvailable = false;
    if (!is_outer_docking && host_node && host_node->ParentNode == NULL && host_node->IsCentralNode())
        data->IsSidesAvailable = false;

    // Calculate drop shapes geometry for allowed splitting directions
    IM_ASSERT(ImGuiDir_None == -1);
    data->SplitNode = host_node;
    data->SplitDir = ImGuiDir_None;
    data->IsSplitDirExplicit = false;
    if (!host_window->Collapsed)
        for (int dir = ImGuiDir_None; dir < ImGuiDir_COUNT; dir++)
        {
            if (dir == ImGuiDir_None && !data->IsCenterAvailable)
                continue;
            if (dir != ImGuiDir_None && !data->IsSidesAvailable)
                continue;
            if (DockNodeCalcDropRectsAndTestMousePos(data->FutureNode.Rect(), (ImGuiDir)dir, data->DropRectsDraw[dir+1], is_outer_docking, &g.IO.MousePos))
            {
                data->SplitDir = (ImGuiDir)dir;
                data->IsSplitDirExplicit = true;
            }
        }

    // When docking without holding Shift, we only allow and preview docking when hovering over a drop rect or over the title bar
    data->IsDropAllowed = (data->SplitDir != ImGuiDir_None) || (data->IsCenterAvailable);
    if (!is_explicit_target && !data->IsSplitDirExplicit && !g.IO.ConfigDockingWithShift)
        data->IsDropAllowed = false;

    // Calculate split area
    data->SplitRatio = 0.0f;
    if (data->SplitDir != ImGuiDir_None)
    {
        ImGuiDir split_dir = data->SplitDir;
        ImGuiAxis split_axis = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
        ImVec2 pos_new, pos_old = data->FutureNode.Pos;
        ImVec2 size_new, size_old = data->FutureNode.Size;
        DockNodeCalcSplitRects(pos_old, size_old, pos_new, size_new, split_dir, root_payload->Size);

        // Calculate split ratio so we can pass it down the docking request
        float split_ratio = ImSaturate(size_new[split_axis] / data->FutureNode.Size[split_axis]);
        data->FutureNode.Pos = pos_new;
        data->FutureNode.Size = size_new;
        data->SplitRatio = (split_dir == ImGuiDir_Right || split_dir == ImGuiDir_Down) ? (1.0f - split_ratio) : (split_ratio);
    }
}

static void ImGui::DockNodePreviewDockRender(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* root_payload, const ImGuiDockPreviewData* data)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentWindow == host_window);   // Because we rely on font size to calculate tab sizes

    // With this option, we only display the preview on the target viewport, and the payload viewport is made transparent.
    // To compensate for the single layer obstructed by the payload, we'll increase the alpha of the preview nodes.
    const bool is_transparent_payload = g.IO.ConfigDockingTransparentPayload;

    // In case the two windows involved are on different viewports, we will draw the overlay on each of them.
    int overlay_draw_lists_count = 0;
    ImDrawList* overlay_draw_lists[2];
    overlay_draw_lists[overlay_draw_lists_count++] = GetForegroundDrawList(host_window->Viewport);
    if (host_window->Viewport != root_payload->Viewport && !is_transparent_payload)
        overlay_draw_lists[overlay_draw_lists_count++] = GetForegroundDrawList(root_payload->Viewport);

    // Draw main preview rectangle
    const ImU32 overlay_col_tabs = GetColorU32(ImGuiCol_TabActive);
    const ImU32 overlay_col_main = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 0.60f : 0.40f);
    const ImU32 overlay_col_drop = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 0.90f : 0.70f);
    const ImU32 overlay_col_drop_hovered = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 1.20f : 1.00f);
    const ImU32 overlay_col_lines = GetColorU32(ImGuiCol_NavWindowingHighlight, is_transparent_payload ? 0.80f : 0.60f);

    // Display area preview
    const bool can_preview_tabs = (root_payload->DockNodeAsHost == NULL || root_payload->DockNodeAsHost->Windows.Size > 0);
    if (data->IsDropAllowed)
    {
        ImRect overlay_rect = data->FutureNode.Rect();
        if (data->SplitDir == ImGuiDir_None && can_preview_tabs)
            overlay_rect.Min.y += GetFrameHeight();
        if (data->SplitDir != ImGuiDir_None || data->IsCenterAvailable)
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
                overlay_draw_lists[overlay_n]->AddRectFilled(overlay_rect.Min, overlay_rect.Max, overlay_col_main, host_window->WindowRounding);
    }

    // Display tab shape/label preview unless we are splitting node (it generally makes the situation harder to read)
    if (data->IsDropAllowed && can_preview_tabs && data->SplitDir == ImGuiDir_None && data->IsCenterAvailable)
    {
        // Compute target tab bar geometry so we can locate our preview tabs
        ImRect tab_bar_rect;
        DockNodeCalcTabBarLayout(&data->FutureNode, NULL, &tab_bar_rect, NULL);
        ImVec2 tab_pos = tab_bar_rect.Min;
        if (host_node && host_node->TabBar)
        {
            if (!host_node->IsHiddenTabBar() && !host_node->IsNoTabBar())
                tab_pos.x += host_node->TabBar->OffsetMax + g.Style.ItemInnerSpacing.x; // We don't use OffsetNewTab because when using non-persistent-order tab bar it is incremented with each Tab submission.
            else
                tab_pos.x += g.Style.ItemInnerSpacing.x + TabItemCalcSize(host_node->Windows[0]->Name, host_node->Windows[0]->HasCloseButton).x;
        }
        else if (!(host_window->Flags & ImGuiWindowFlags_DockNodeHost))
        {
            tab_pos.x += g.Style.ItemInnerSpacing.x + TabItemCalcSize(host_window->Name, host_window->HasCloseButton).x; // Account for slight offset which will be added when changing from title bar to tab bar
        }

        // Draw tab shape/label preview (payload may be a loose window or a host window carrying multiple tabbed windows)
        if (root_payload->DockNodeAsHost)
            IM_ASSERT(root_payload->DockNodeAsHost->Windows.Size == root_payload->DockNodeAsHost->TabBar->Tabs.Size);
        const int payload_count = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->TabBar->Tabs.Size : 1;
        for (int payload_n = 0; payload_n < payload_count; payload_n++)
        {
            // Calculate the tab bounding box for each payload window
            ImGuiWindow* payload = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->TabBar->Tabs[payload_n].Window : root_payload;
            if (!DockNodeIsDropAllowedOne(payload, host_window))
                continue;

            ImVec2 tab_size = TabItemCalcSize(payload->Name, payload->HasCloseButton);
            ImRect tab_bb(tab_pos.x, tab_pos.y, tab_pos.x + tab_size.x, tab_pos.y + tab_size.y);
            tab_pos.x += tab_size.x + g.Style.ItemInnerSpacing.x;
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
            {
                ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_Preview | ((payload->Flags & ImGuiWindowFlags_UnsavedDocument) ? ImGuiTabItemFlags_UnsavedDocument : 0);
                if (!tab_bar_rect.Contains(tab_bb))
                    overlay_draw_lists[overlay_n]->PushClipRect(tab_bar_rect.Min, tab_bar_rect.Max);
                TabItemBackground(overlay_draw_lists[overlay_n], tab_bb, tab_flags, overlay_col_tabs);
                TabItemLabelAndCloseButton(overlay_draw_lists[overlay_n], tab_bb, tab_flags, g.Style.FramePadding, payload->Name, 0, 0);
                if (!tab_bar_rect.Contains(tab_bb))
                    overlay_draw_lists[overlay_n]->PopClipRect();
            }
        }
    }

    // Display drop boxes
    const float overlay_rounding = ImMax(3.0f, g.Style.FrameRounding);
    for (int dir = ImGuiDir_None; dir < ImGuiDir_COUNT; dir++)
    {
        if (!data->DropRectsDraw[dir + 1].IsInverted())
        {
            ImRect draw_r = data->DropRectsDraw[dir + 1];
            ImRect draw_r_in = draw_r;
            draw_r_in.Expand(-2.0f);
            ImU32 overlay_col = (data->SplitDir == (ImGuiDir)dir && data->IsSplitDirExplicit) ? overlay_col_drop_hovered : overlay_col_drop;
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
            {
                ImVec2 center = ImFloor(draw_r_in.GetCenter());
                overlay_draw_lists[overlay_n]->AddRectFilled(draw_r.Min, draw_r.Max, overlay_col, overlay_rounding);
                overlay_draw_lists[overlay_n]->AddRect(draw_r_in.Min, draw_r_in.Max, overlay_col_lines, overlay_rounding);
                if (dir == ImGuiDir_Left || dir == ImGuiDir_Right)
                    overlay_draw_lists[overlay_n]->AddLine(ImVec2(center.x, draw_r_in.Min.y), ImVec2(center.x, draw_r_in.Max.y), overlay_col_lines);
                if (dir == ImGuiDir_Up || dir == ImGuiDir_Down)
                    overlay_draw_lists[overlay_n]->AddLine(ImVec2(draw_r_in.Min.x, center.y), ImVec2(draw_r_in.Max.x, center.y), overlay_col_lines);
            }
        }
        
        // Stop after ImGuiDir_None
        if ((host_node && (host_node->GetMergedFlags() & ImGuiDockNodeFlags_NoSplit)) || g.IO.ConfigDockingNoSplit)
            return;
    }
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockNode Tree manipulation functions
//-----------------------------------------------------------------------------
// - DockNodeTreeSplit()
// - DockNodeTreeMerge()
// - DockNodeTreeUpdatePosSize()
// - DockNodeTreeUpdateSplitterFindTouchingNode()
// - DockNodeTreeUpdateSplitter()
// - DockNodeTreeFindFallbackLeafNode()
// - DockNodeTreeFindNodeByPos()
//-----------------------------------------------------------------------------

void ImGui::DockNodeTreeSplit(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiAxis split_axis, int split_inheritor_child_idx, float split_ratio, ImGuiDockNode* new_node)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(split_axis != ImGuiAxis_None);

    ImGuiDockNode* child_0 = (new_node && split_inheritor_child_idx != 0) ? new_node : DockContextAddNode(ctx, 0);
    child_0->ParentNode = parent_node;

    ImGuiDockNode* child_1 = (new_node && split_inheritor_child_idx != 1) ? new_node : DockContextAddNode(ctx, 0);
    child_1->ParentNode = parent_node;

    ImGuiDockNode* child_inheritor = (split_inheritor_child_idx == 0) ? child_0 : child_1;
    DockNodeMoveChildNodes(child_inheritor, parent_node);
    parent_node->ChildNodes[0] = child_0;
    parent_node->ChildNodes[1] = child_1;
    parent_node->ChildNodes[split_inheritor_child_idx]->VisibleWindow = parent_node->VisibleWindow;
    parent_node->SplitAxis = split_axis;
    parent_node->VisibleWindow = NULL;
    parent_node->AuthorityForPos = parent_node->AuthorityForSize = ImGuiDataAuthority_DockNode;

    float size_avail = (parent_node->Size[split_axis] - IMGUI_DOCK_SPLITTER_SIZE);
    size_avail = ImMax(size_avail, g.Style.WindowMinSize[split_axis] * 2.0f);
    IM_ASSERT(size_avail > 0.0f); // If you created a node manually with DockBuilderAddNode(), you need to also call DockBuilderSetNodeSize() before splitting.
    child_0->SizeRef = child_1->SizeRef = parent_node->Size;
    child_0->SizeRef[split_axis] = ImFloor(size_avail * split_ratio);
    child_1->SizeRef[split_axis] = ImFloor(size_avail - child_0->SizeRef[split_axis]);

    DockNodeMoveWindows(parent_node->ChildNodes[split_inheritor_child_idx], parent_node);
    DockNodeTreeUpdatePosSize(parent_node, parent_node->Pos, parent_node->Size);

    // Flags transfer (e.g. this is where we transfer the ImGuiDockNodeFlags_CentralNode property)
    child_0->SharedFlags = parent_node->SharedFlags & ImGuiDockNodeFlags_SharedFlagsInheritMask_;
    child_1->SharedFlags = parent_node->SharedFlags & ImGuiDockNodeFlags_SharedFlagsInheritMask_;
    child_inheritor->LocalFlags = parent_node->LocalFlags & ImGuiDockNodeFlags_LocalFlagsTransferMask_;
    parent_node->LocalFlags &= ~ImGuiDockNodeFlags_LocalFlagsTransferMask_;
    if (child_inheritor->IsCentralNode())
        DockNodeGetRootNode(parent_node)->CentralNode = child_inheritor;
}

void ImGui::DockNodeTreeMerge(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiDockNode* merge_lead_child)
{
    // When called from DockContextProcessUndockNode() it is possible that one of the child is NULL.
    ImGuiDockNode* child_0 = parent_node->ChildNodes[0];
    ImGuiDockNode* child_1 = parent_node->ChildNodes[1];
    IM_ASSERT(child_0 || child_1);
    IM_ASSERT(merge_lead_child == child_0 || merge_lead_child == child_1);
    if ((child_0 && child_0->Windows.Size > 0) || (child_1 && child_1->Windows.Size > 0))
    {
        IM_ASSERT(parent_node->TabBar == NULL);
        IM_ASSERT(parent_node->Windows.Size == 0);
    }
    IMGUI_DEBUG_LOG_DOCKING("DockNodeTreeMerge 0x%08X & 0x%08X back into parent 0x%08X\n", child_0 ? child_0->ID : 0, child_1 ? child_1->ID : 0, parent_node->ID);

    ImVec2 backup_last_explicit_size = parent_node->SizeRef;
    DockNodeMoveChildNodes(parent_node, merge_lead_child);
    if (child_0)
    {
        DockNodeMoveWindows(parent_node, child_0); // Generally only 1 of the 2 child node will have windows
        DockSettingsRenameNodeReferences(child_0->ID, parent_node->ID);
    }
    if (child_1)
    {
        DockNodeMoveWindows(parent_node, child_1);
        DockSettingsRenameNodeReferences(child_1->ID, parent_node->ID);
    }
    DockNodeApplyPosSizeToWindows(parent_node);
    parent_node->AuthorityForPos = parent_node->AuthorityForSize = parent_node->AuthorityForViewport = ImGuiDataAuthority_Auto;
    parent_node->VisibleWindow = merge_lead_child->VisibleWindow;
    parent_node->SizeRef = backup_last_explicit_size;

    // Flags transfer
    parent_node->LocalFlags &= ~ImGuiDockNodeFlags_LocalFlagsTransferMask_; // Preserve Dockspace flag
    parent_node->LocalFlags |= (child_0 ? child_0->LocalFlags : 0) & ImGuiDockNodeFlags_LocalFlagsTransferMask_;
    parent_node->LocalFlags |= (child_1 ? child_1->LocalFlags : 0) & ImGuiDockNodeFlags_LocalFlagsTransferMask_;

    if (child_0)
    {
        ctx->DockContext->Nodes.SetVoidPtr(child_0->ID, NULL);
        IM_DELETE(child_0);
    }
    if (child_1)
    {
        ctx->DockContext->Nodes.SetVoidPtr(child_1->ID, NULL);
        IM_DELETE(child_1);
    }
}

// Update Pos/Size for a node hierarchy (don't affect child Windows yet)
void ImGui::DockNodeTreeUpdatePosSize(ImGuiDockNode* node, ImVec2 pos, ImVec2 size, bool only_write_to_marked_nodes)
{
    // During the regular dock node update we write to all nodes.
    // 'only_write_to_marked_nodes' is only set when turning a node visible mid-frame and we need its size right-away.
    IM_ASSERT(size.x > 0.0f && size.y > 0.0f);
    const bool write_to_node = (only_write_to_marked_nodes == false) || (node->MarkedForPosSizeWrite);
    if (write_to_node)
    {
        node->Pos = pos;
        node->Size = size;
    }

    if (node->IsLeafNode())
        return;

    ImGuiDockNode* child_0 = node->ChildNodes[0];
    ImGuiDockNode* child_1 = node->ChildNodes[1];
    ImVec2 child_0_pos = pos, child_1_pos = pos;
    ImVec2 child_0_size = size, child_1_size = size;
    if (child_0->IsVisible && child_1->IsVisible)
    {
        const float spacing = IMGUI_DOCK_SPLITTER_SIZE;
        const ImGuiAxis axis = (ImGuiAxis)node->SplitAxis;
        const float size_avail = ImMax(size[axis] - spacing, 0.0f);

        // Size allocation policy
        // 1) The first 0..WindowMinSize[axis]*2 are allocated evenly to both windows.
        ImGuiContext& g = *GImGui;
        const float size_min_each = ImFloor(ImMin(size_avail, g.Style.WindowMinSize[axis] * 2.0f) * 0.5f);

        // 2) Process locked absolute size (during a splitter resize we preserve the child of nodes not touching the splitter edge)
        IM_ASSERT(!(child_0->WantLockSizeOnce && child_1->WantLockSizeOnce));
        if (child_0->WantLockSizeOnce)
        {
            child_0->WantLockSizeOnce = false;
            child_0_size[axis] = child_0->SizeRef[axis] = child_0->Size[axis];
            child_1_size[axis] = child_1->SizeRef[axis] = (size_avail - child_0_size[axis]);
            IM_ASSERT(child_0->SizeRef[axis] > 0.0f && child_1->SizeRef[axis] > 0.0f);

        }
        else if (child_1->WantLockSizeOnce)
        {
            child_1->WantLockSizeOnce = false;
            child_1_size[axis] = child_1->SizeRef[axis] = child_1->Size[axis];
            child_0_size[axis] = child_0->SizeRef[axis] = (size_avail - child_1_size[axis]);
            IM_ASSERT(child_0->SizeRef[axis] > 0.0f && child_1->SizeRef[axis] > 0.0f);
        }

        // 3) If one window is the central node (~ use remaining space, should be made explicit!), use explicit size from the other, and remainder for the central node
        else if (child_1->IsCentralNode() && child_0->SizeRef[axis] != 0.0f)
        {
            child_0_size[axis] = ImMin(size_avail - size_min_each, child_0->SizeRef[axis]);
            child_1_size[axis] = (size_avail - child_0_size[axis]);
        }
        else if (child_0->IsCentralNode() && child_1->SizeRef[axis] != 0.0f)
        {
            child_1_size[axis] = ImMin(size_avail - size_min_each, child_1->SizeRef[axis]);
            child_0_size[axis] = (size_avail - child_1_size[axis]);
        }
        else
        {
            // 4) Otherwise distribute according to the relative ratio of each SizeRef value
            float split_ratio = child_0->SizeRef[axis] / (child_0->SizeRef[axis] + child_1->SizeRef[axis]);
            child_0_size[axis] = ImMax(size_min_each, ImFloor(size_avail * split_ratio + 0.5F));
            child_1_size[axis] = (size_avail - child_0_size[axis]);
        }
        child_1_pos[axis] += spacing + child_0_size[axis];
    }
    if (child_0->IsVisible)
        DockNodeTreeUpdatePosSize(child_0, child_0_pos, child_0_size);
    if (child_1->IsVisible)
        DockNodeTreeUpdatePosSize(child_1, child_1_pos, child_1_size);
}

static void DockNodeTreeUpdateSplitterFindTouchingNode(ImGuiDockNode* node, ImGuiAxis axis, int side, ImVector<ImGuiDockNode*>* touching_nodes)
{
    if (node->IsLeafNode())
    {
        touching_nodes->push_back(node);
        return;
    }
    if (node->ChildNodes[0]->IsVisible)
        if (node->SplitAxis != axis || side == 0 || !node->ChildNodes[1]->IsVisible)
            DockNodeTreeUpdateSplitterFindTouchingNode(node->ChildNodes[0], axis, side, touching_nodes);
    if (node->ChildNodes[1]->IsVisible)
        if (node->SplitAxis != axis || side == 1 || !node->ChildNodes[0]->IsVisible)
            DockNodeTreeUpdateSplitterFindTouchingNode(node->ChildNodes[1], axis, side, touching_nodes);
}

void ImGui::DockNodeTreeUpdateSplitter(ImGuiDockNode* node)
{
    if (node->IsLeafNode())
        return;

    ImGuiContext& g = *GImGui;

    ImGuiDockNode* child_0 = node->ChildNodes[0];
    ImGuiDockNode* child_1 = node->ChildNodes[1];
    if (child_0->IsVisible && child_1->IsVisible)
    {
        // Bounding box of the splitter cover the space between both nodes (w = Spacing, h = Size[xy^1] for when splitting horizontally)
        const ImGuiAxis axis = (ImGuiAxis)node->SplitAxis;
        IM_ASSERT(axis != ImGuiAxis_None);
        ImRect bb;
        bb.Min = child_0->Pos;
        bb.Max = child_1->Pos;
        bb.Min[axis] += child_0->Size[axis];
        bb.Max[axis ^ 1] += child_1->Size[axis ^ 1];
        //if (g.IO.KeyCtrl) GetForegroundDrawList(g.CurrentWindow->Viewport)->AddRect(bb.Min, bb.Max, IM_COL32(255,0,255,255));

        if ((child_0->GetMergedFlags() | child_1->GetMergedFlags()) & ImGuiDockNodeFlags_NoResize)
        {
            ImGuiWindow* window = g.CurrentWindow;
            window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_Separator), g.Style.FrameRounding);
        }
        else
        {
            //bb.Min[axis] += 1; // Display a little inward so highlight doesn't connect with nearby tabs on the neighbor node.
            //bb.Max[axis] -= 1;
            PushID(node->ID);

            // Gather list of nodes that are touching the splitter line. Find resizing limits based on those nodes.
            ImVector<ImGuiDockNode*> touching_nodes[2];
            float min_size = g.Style.WindowMinSize[axis];
            float resize_limits[2];
            resize_limits[0] = node->ChildNodes[0]->Pos[axis] + min_size;
            resize_limits[1] = node->ChildNodes[1]->Pos[axis] + node->ChildNodes[1]->Size[axis] - min_size;

            ImGuiID splitter_id = GetID("##Splitter");
            if (g.ActiveId == splitter_id)
            {
                // Only process when splitter is active
                DockNodeTreeUpdateSplitterFindTouchingNode(child_0, axis, 1, &touching_nodes[0]);
                DockNodeTreeUpdateSplitterFindTouchingNode(child_1, axis, 0, &touching_nodes[1]);
                for (int touching_node_n = 0; touching_node_n < touching_nodes[0].Size; touching_node_n++)
                    resize_limits[0] = ImMax(resize_limits[0], touching_nodes[0][touching_node_n]->Rect().Min[axis] + min_size);
                for (int touching_node_n = 0; touching_node_n < touching_nodes[1].Size; touching_node_n++)
                    resize_limits[1] = ImMin(resize_limits[1], touching_nodes[1][touching_node_n]->Rect().Max[axis] - min_size);

                /*
                // [DEBUG] Render limits
                ImDrawList* draw_list = node->HostWindow ? GetForegroundDrawList(node->HostWindow) : GetForegroundDrawList((ImGuiViewportP*)GetMainViewport());
                for (int n = 0; n < 2; n++)
                    if (axis == ImGuiAxis_X)
                        draw_list->AddLine(ImVec2(resize_limits[n], node->ChildNodes[n]->Pos.y), ImVec2(resize_limits[n], node->ChildNodes[n]->Pos.y + node->ChildNodes[n]->Size.y), IM_COL32(255, 0, 255, 255), 3.0f);
                    else
                        draw_list->AddLine(ImVec2(node->ChildNodes[n]->Pos.x, resize_limits[n]), ImVec2(node->ChildNodes[n]->Pos.x + node->ChildNodes[n]->Size.x, resize_limits[n]), IM_COL32(255, 0, 255, 255), 3.0f);
                */
            }

            // Use a short delay before highlighting the splitter (and changing the mouse cursor) in order for regular mouse movement to not highlight many splitters
            float cur_size_0 = child_0->Size[axis];
            float cur_size_1 = child_1->Size[axis];
            float min_size_0 = resize_limits[0] - child_0->Pos[axis];
            float min_size_1 = child_1->Pos[axis] + child_1->Size[axis] - resize_limits[1];
            if (SplitterBehavior(bb, GetID("##Splitter"), axis, &cur_size_0, &cur_size_1, min_size_0, min_size_1, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER))
            {
                if (touching_nodes[0].Size > 0 && touching_nodes[1].Size > 0)
                {
                    child_0->Size[axis] = child_0->SizeRef[axis] = cur_size_0;
                    child_1->Pos[axis] -= cur_size_1 - child_1->Size[axis];
                    child_1->Size[axis] = child_1->SizeRef[axis] = cur_size_1;

                    // Lock the size of every node that is a sibling of the node we are touching
                    // This might be less desirable if we can merge sibling of a same axis into the same parental level.
                    for (int side_n = 0; side_n < 2; side_n++)
                        for (int touching_node_n = 0; touching_node_n < touching_nodes[side_n].Size; touching_node_n++)
                        {
                            ImGuiDockNode* touching_node = touching_nodes[side_n][touching_node_n];
                            //ImDrawList* draw_list = node->HostWindow ? GetForegroundDrawList(node->HostWindow) : GetForegroundDrawList((ImGuiViewportP*)GetMainViewport());
                            //draw_list->AddRect(touching_node->Pos, touching_node->Pos + touching_node->Size, IM_COL32(255, 128, 0, 255));
                            while (touching_node->ParentNode != node)
                            {
                                if (touching_node->ParentNode->SplitAxis == axis)
                                {
                                    // Mark other node so its size will be preserved during the upcoming call to DockNodeTreeUpdatePosSize().
                                    ImGuiDockNode* node_to_preserve = touching_node->ParentNode->ChildNodes[side_n];
                                    node_to_preserve->WantLockSizeOnce = true;
                                    //draw_list->AddRect(touching_node->Pos, touching_node->Rect().Max, IM_COL32(255, 0, 0, 255));
                                    //draw_list->AddRectFilled(node_to_preserve->Pos, node_to_preserve->Rect().Max, IM_COL32(0, 255, 0, 100));
                                }
                                touching_node = touching_node->ParentNode;
                            }
                        }

                    DockNodeTreeUpdatePosSize(child_0, child_0->Pos, child_0->Size);
                    DockNodeTreeUpdatePosSize(child_1, child_1->Pos, child_1->Size);
                    MarkIniSettingsDirty();
                }
            }
            PopID();
        }
    }

    if (child_0->IsVisible)
        DockNodeTreeUpdateSplitter(child_0);
    if (child_1->IsVisible)
        DockNodeTreeUpdateSplitter(child_1);
}

ImGuiDockNode* ImGui::DockNodeTreeFindFallbackLeafNode(ImGuiDockNode* node)
{
    if (node->IsLeafNode())
        return node;
    if (ImGuiDockNode* leaf_node = DockNodeTreeFindFallbackLeafNode(node->ChildNodes[0]))
        return leaf_node;
    if (ImGuiDockNode* leaf_node = DockNodeTreeFindFallbackLeafNode(node->ChildNodes[1]))
        return leaf_node;
    return NULL;
}

ImGuiDockNode* ImGui::DockNodeTreeFindNodeByPos(ImGuiDockNode* node, ImVec2 pos)
{
    if (!node->IsVisible)
        return NULL;

    ImGuiContext& g = *GImGui;
    const float dock_spacing = g.Style.ItemInnerSpacing.x;
    ImRect r(node->Pos, node->Pos + node->Size);
    r.Expand(dock_spacing * 0.5f);
    bool inside = r.Contains(pos);
    if (!inside)
        return NULL;

    if (node->IsLeafNode())
        return node;
    if (ImGuiDockNode* hovered_node = DockNodeTreeFindNodeByPos(node->ChildNodes[0], pos))
        return hovered_node;
    if (ImGuiDockNode* hovered_node = DockNodeTreeFindNodeByPos(node->ChildNodes[1], pos))
        return hovered_node;

    // There is an edge case when docking into a dockspace which only has inactive nodes (because none of the windows are active)
    // In this case we need to fallback into any leaf mode, possibly the central node.
    if (node->IsDockSpace() && node->IsRootNode())
    {
        if (node->CentralNode && node->IsLeafNode()) // FIXME-20181220: We should not have to test for IsLeafNode() here but we have another bug to fix first.
            return node->CentralNode;
        return DockNodeTreeFindFallbackLeafNode(node);
    }
    
    return NULL;
}

//-----------------------------------------------------------------------------
// Docking: Public Functions (SetWindowDock, DockSpace, DockSpaceOverViewport)
//-----------------------------------------------------------------------------
// - SetWindowDock() [Internal]
// - DockSpace()
// - DockSpaceOverViewport()
//-----------------------------------------------------------------------------

// [Internal] Called via SetNextWindowDockID()
void ImGui::SetWindowDock(ImGuiWindow* window, ImGuiID dock_id, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowDockAllowFlags & cond) == 0)
        return;
    window->SetWindowDockAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    if (window->DockId == dock_id)
        return;

    // If the user attempt to set a dock id that is a split node, we'll dig within to find a suitable docking spot
    ImGuiContext* ctx = GImGui;
    if (ImGuiDockNode* new_node = DockContextFindNodeByID(ctx, dock_id))
        if (new_node->IsSplitNode())
        {
            // Policy: Find central node or latest focused node. We first move back to our root node.
            new_node = DockNodeGetRootNode(new_node);
            if (new_node->CentralNode)
            {
                IM_ASSERT(new_node->CentralNode->IsCentralNode());
                dock_id = new_node->CentralNode->ID;
            }
            else
            {
                dock_id = new_node->LastFocusedNodeID;
            }
        }

    if (window->DockId == dock_id)
        return;

    if (window->DockNode)
        DockNodeRemoveWindow(window->DockNode, window, 0);
    window->DockId = dock_id;
}

