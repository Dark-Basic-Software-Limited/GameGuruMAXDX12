namespace IMNODES_NAMESPACE
{
ImNodesContext* CreateContext()
{
    ImNodesContext* ctx = IM_NEW(ImNodesContext)();
    if (GImNodes == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void DestroyContext(ImNodesContext* ctx)
{
    if (ctx == NULL)
        ctx = GImNodes;
    Shutdown(ctx);
    if (GImNodes == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

ImNodesContext* GetCurrentContext() { return GImNodes; }

void SetCurrentContext(ImNodesContext* ctx) { GImNodes = ctx; }

ImNodesEditorContext* EditorContextCreate()
{
    void* mem = ImGui::MemAlloc(sizeof(ImNodesEditorContext));
    new (mem) ImNodesEditorContext();
    return (ImNodesEditorContext*)mem;
}

void EditorContextFree(ImNodesEditorContext* ctx)
{
    ctx->~ImNodesEditorContext();
    ImGui::MemFree(ctx);
}

void EditorContextSet(ImNodesEditorContext* ctx) { GImNodes->EditorCtx = ctx; }

ImVec2 EditorContextGetPanning()
{
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.Panning;
}

void EditorContextResetPanning(const ImVec2& pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.Panning = pos;
}

void EditorContextMoveToNode(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);

    editor.Panning.x = -node.Origin.x;
    editor.Panning.y = -node.Origin.y;
}

void SetImGuiContext(ImGuiContext* ctx) { ImGui::SetCurrentContext(ctx); }

ImNodesIO& GetIO() { return GImNodes->Io; }

ImNodesStyle& GetStyle() { return GImNodes->Style; }

void StyleColorsDark()
{
    GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    // title bar colors match ImGui's titlebg colors
    GImNodes->Style.Colors[ImNodesCol_TitleBar] = IM_COL32(41, 74, 122, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(66, 150, 250, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(66, 150, 250, 255);
    // link colors match ImGui's slider grab colors
    GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(61, 133, 224, 200);
    GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 255);
    GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 255);
    // pin colors match ImGui's button colors
    GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(53, 150, 250, 180);
    GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(53, 150, 250, 255);

    GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(61, 133, 224, 30);
    GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(61, 133, 224, 150);

    GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
    GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);

    // minimap colors
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 150);
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered] = IM_COL32(200, 200, 200, 255);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
    GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] =
        GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsClassic()
{
    GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBar] = IM_COL32(69, 69, 138, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(82, 82, 161, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(82, 82, 161, 255);
    GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(255, 255, 255, 100);
    GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(105, 99, 204, 153);
    GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(105, 99, 204, 153);
    GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(89, 102, 156, 170);
    GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(102, 122, 179, 200);
    GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(82, 82, 161, 100);
    GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(82, 82, 161, 255);
    GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
    GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);

    // minimap colors
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = IM_COL32(200, 200, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
    GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] =
        GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsLight()
{
    GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(240, 240, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(240, 240, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(240, 240, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBar] = IM_COL32(248, 248, 248, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(209, 209, 209, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(209, 209, 209, 255);
    // original imgui values: 66, 150, 250
    GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(66, 150, 250, 100);
    // original imgui values: 117, 138, 204
    GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 242);
    GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 242);
    // original imgui values: 66, 150, 250
    GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(66, 150, 250, 160);
    GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(66, 150, 250, 255);
    GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(90, 170, 250, 30);
    GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(90, 170, 250, 150);
    GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(225, 225, 225, 255);
    GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(180, 180, 180, 100);

    // minimap colors
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = IM_COL32(200, 200, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
    GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] =
        GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void BeginNodeEditor()
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    // Reset state from previous pass

    ImNodesEditorContext& editor = EditorContextGet();
    editor.AutoPanningDelta = ImVec2(0, 0);
    editor.GridContentBounds = ImRect(FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN);
    editor.MiniMapEnabled = false;
    ObjectPoolReset(editor.Nodes);
    ObjectPoolReset(editor.Pins);
    ObjectPoolReset(editor.Links);

    GImNodes->HoveredNodeIdx.Reset();
    GImNodes->HoveredLinkIdx.Reset();
    GImNodes->HoveredPinIdx.Reset();
    GImNodes->DeletedLinkIdx.Reset();
    GImNodes->SnapLinkIdx.Reset();

    GImNodes->NodeIndicesOverlappingWithMouse.clear();

    GImNodes->ImNodesUIState = ImNodesUIState_None;

    GImNodes->MousePos = ImGui::GetIO().MousePos;
    GImNodes->LeftMouseClicked = ImGui::IsMouseClicked(0);
    GImNodes->LeftMouseReleased = ImGui::IsMouseReleased(0);
    GImNodes->AltMouseClicked =
        (GImNodes->Io.EmulateThreeButtonMouse.Modifier != NULL &&
         *GImNodes->Io.EmulateThreeButtonMouse.Modifier && GImNodes->LeftMouseClicked) ||
        ImGui::IsMouseClicked(GImNodes->Io.AltMouseButton);
    GImNodes->LeftMouseDragging = ImGui::IsMouseDragging(0, 0.0f);

    GImNodes->AltMouseDragging =
        (GImNodes->Io.EmulateThreeButtonMouse.Modifier != NULL && GImNodes->LeftMouseDragging &&
         (*GImNodes->Io.EmulateThreeButtonMouse.Modifier)) ||
        ImGui::IsMouseDragging(GImNodes->Io.AltMouseButton, 0.0f);

    GImNodes->AltMouseScrollDelta = ImGui::GetIO().MouseWheel;

    GImNodes->ActiveAttribute = false;

    ImGui::BeginGroup();
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
        ImGui::BeginChild(
            "scrolling_region",
            ImVec2(0.f, 0.f),
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollWithMouse);
        GImNodes->CanvasOriginScreenSpace = ImGui::GetCursorScreenPos();

        // NOTE: we have to fetch the canvas draw list *after* we call
        // BeginChild(), otherwise the ImGui UI elements are going to be
        // rendered into the parent window draw list.
        DrawListSet(ImGui::GetWindowDrawList());

        {
            const ImVec2 canvas_size = ImGui::GetWindowSize();
            GImNodes->CanvasRectScreenSpace = ImRect(
                EditorSpaceToScreenSpace(ImVec2(0.f, 0.f)), EditorSpaceToScreenSpace(canvas_size));

            if (GImNodes->Style.Flags & ImNodesStyleFlags_GridLines)
            {
                DrawGrid(editor, canvas_size);
            }
        }
    }
}

void EndNodeEditor()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_None;

    ImNodesEditorContext& editor = EditorContextGet();

    bool no_grid_content = editor.GridContentBounds.IsInverted();
    if (no_grid_content)
    {
        editor.GridContentBounds = ScreenSpaceToGridSpace(editor, GImNodes->CanvasRectScreenSpace);
    }

    // Detect ImGui interaction first, because it blocks interaction with the rest of the UI

    if (GImNodes->LeftMouseClicked && ImGui::IsAnyItemActive())
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_ImGuiItem;
    }

    // Detect which UI element is being hovered over. Detection is done in a hierarchical fashion,
    // because a UI element being hovered excludes any other as being hovered over.

    // Don't do hovering detection for nodes/links/pins when interacting with the mini-map, since
    // its an *overlay* with its own interaction behavior and must have precedence during mouse
    // interaction.

    if ((editor.ClickInteraction.Type == ImNodesClickInteractionType_None ||
         editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation) &&
        MouseInCanvas() && !IsMiniMapHovered())
    {
        // Pins needs some special care. We need to check the depth stack to see which pins are
        // being occluded by other nodes.
        ResolveOccludedPins(editor, GImNodes->OccludedPinIndices);

        GImNodes->HoveredPinIdx = ResolveHoveredPin(editor.Pins, GImNodes->OccludedPinIndices);

        if (!GImNodes->HoveredPinIdx.HasValue())
        {
            // Resolve which node is actually on top and being hovered using the depth stack.
            GImNodes->HoveredNodeIdx = ResolveHoveredNode(editor.NodeDepthOrder);
        }

        // We don't check for hovered pins here, because if we want to detach a link by clicking and
        // dragging, we need to have both a link and pin hovered.
        if (!GImNodes->HoveredNodeIdx.HasValue())
        {
            GImNodes->HoveredLinkIdx = ResolveHoveredLink(editor.Links, editor.Pins);
        }
    }

    for (int node_idx = 0; node_idx < editor.Nodes.Pool.size(); ++node_idx)
    {
        if (editor.Nodes.InUse[node_idx])
        {
            DrawListActivateNodeBackground(node_idx);
            DrawNode(editor, node_idx);
        }
    }

    // In order to render the links underneath the nodes, we want to first select the bottom draw
    // channel.
    GImNodes->CanvasDrawList->ChannelsSetCurrent(0);

    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        if (editor.Links.InUse[link_idx])
        {
            DrawLink(editor, link_idx);
        }
    }

    // Render the click interaction UI elements (partial links, box selector) on top of everything
    // else.

    DrawListAppendClickInteractionChannel();
    DrawListActivateClickInteractionChannel();

    if (IsMiniMapActive())
    {
        CalcMiniMapLayout();
        MiniMapUpdate();
    }

    // Handle node graph interaction

    if (!IsMiniMapHovered())
    {
        if (GImNodes->LeftMouseClicked && GImNodes->HoveredLinkIdx.HasValue())
        {
            BeginLinkInteraction(editor, GImNodes->HoveredLinkIdx.Value());
        }

        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredPinIdx.HasValue())
        {
            BeginLinkCreation(editor, GImNodes->HoveredPinIdx.Value());
        }

        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredNodeIdx.HasValue())
        {
            BeginNodeSelection(editor, GImNodes->HoveredNodeIdx.Value());
        }

        else if (
            GImNodes->LeftMouseClicked || GImNodes->LeftMouseReleased ||
            GImNodes->AltMouseClicked || GImNodes->AltMouseScrollDelta != 0.f)
        {
            BeginCanvasInteraction(editor);
        }

        bool should_auto_pan =
            editor.ClickInteraction.Type == ImNodesClickInteractionType_BoxSelection ||
            editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation ||
            editor.ClickInteraction.Type == ImNodesClickInteractionType_Node;
        if (should_auto_pan && !MouseInCanvas())
        {
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 center = GImNodes->CanvasRectScreenSpace.GetCenter();
            ImVec2 direction = (center - mouse);
            direction = direction * ImInvLength(direction, 0.0);

            editor.AutoPanningDelta =
                direction * ImGui::GetIO().DeltaTime * GImNodes->Io.AutoPanningSpeed;
            editor.Panning += editor.AutoPanningDelta;
        }
    }
    ClickInteractionUpdate(editor);

    // At this point, draw commands have been issued for all nodes (and pins). Update the node pool
    // to detect unused node slots and remove those indices from the depth stack before sorting the
    // node draw commands by depth.
    ObjectPoolUpdate(editor.Nodes);
    ObjectPoolUpdate(editor.Pins);

    DrawListSortChannelsByDepth(editor.NodeDepthOrder);

    // After the links have been rendered, the link pool can be updated as well.
    ObjectPoolUpdate(editor.Links);

    // Finally, merge the draw channels
    GImNodes->CanvasDrawList->ChannelsMerge();

    // pop style
    ImGui::EndChild();      // end scrolling region
    ImGui::PopStyleColor(); // pop child window background color
    ImGui::PopStyleVar();   // pop window padding
    ImGui::PopStyleVar();   // pop frame padding
    ImGui::EndGroup();
}

void MiniMap(
    const float                              minimap_size_fraction,
    const ImNodesMiniMapLocation             location,
    const ImNodesMiniMapNodeHoveringCallback node_hovering_callback,
    void*                                    node_hovering_callback_data)
{
    // Check that editor size fraction is sane; must be in the range (0, 1]
    assert(minimap_size_fraction > 0.f && minimap_size_fraction <= 1.f);

    // Remember to call before EndNodeEditor
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);

    ImNodesEditorContext& editor = EditorContextGet();

    editor.MiniMapEnabled = true;
    editor.MiniMapSizeFraction = minimap_size_fraction;
    editor.MiniMapLocation = location;

    // Set node hovering callback information
    editor.MiniMapNodeHoveringCallback = node_hovering_callback;
    editor.MiniMapNodeHoveringCallbackUserData = node_hovering_callback_data;

    // Actual drawing/updating of the MiniMap is done in EndNodeEditor so that
    // mini map is draw over everything and all pin/link positions are updated
    // correctly relative to their respective nodes. Hence, we must store some of
    // of the state for the mini map in GImNodes for the actual drawing/updating
}

void BeginNode(const int node_id)
{
    // Remember to call BeginNodeEditor before calling BeginNode
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImNodesEditorContext& editor = EditorContextGet();

    const int node_idx = ObjectPoolFindOrCreateIndex(editor.Nodes, node_id);
    GImNodes->CurrentNodeIdx = node_idx;

    ImNodeData& node = editor.Nodes.Pool[node_idx];
    node.ColorStyle.Background = GImNodes->Style.Colors[ImNodesCol_NodeBackground];
    node.ColorStyle.BackgroundHovered = GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered];
    node.ColorStyle.BackgroundSelected = GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected];
    node.ColorStyle.Outline = GImNodes->Style.Colors[ImNodesCol_NodeOutline];
    node.ColorStyle.Titlebar = GImNodes->Style.Colors[ImNodesCol_TitleBar];
    node.ColorStyle.TitlebarHovered = GImNodes->Style.Colors[ImNodesCol_TitleBarHovered];
    node.ColorStyle.TitlebarSelected = GImNodes->Style.Colors[ImNodesCol_TitleBarSelected];
    node.LayoutStyle.CornerRounding = GImNodes->Style.NodeCornerRounding;
    node.LayoutStyle.Padding = GImNodes->Style.NodePadding;
    node.LayoutStyle.BorderThickness = GImNodes->Style.NodeBorderThickness;

    // ImGui::SetCursorPos sets the cursor position, local to the current widget
    // (in this case, the child object started in BeginNodeEditor). Use
    // ImGui::SetCursorScreenPos to set the screen space coordinates directly.
    ImGui::SetCursorPos(GridSpaceToEditorSpace(editor, GetNodeTitleBarOrigin(node)));

    DrawListAddNode(node_idx);
    DrawListActivateCurrentNodeForeground();

    ImGui::PushID(node.Id);
    ImGui::BeginGroup();
}

void EndNode()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    ImNodesEditorContext& editor = EditorContextGet();

    // The node's rectangle depends on the ImGui UI group size.
    ImGui::EndGroup();
    ImGui::PopID();

    ImNodeData& node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
    node.Rect = GetItemRect();
    node.Rect.Expand(node.LayoutStyle.Padding);

    editor.GridContentBounds.Add(node.Origin);
    editor.GridContentBounds.Add(node.Origin + node.Rect.GetSize());

    if (node.Rect.Contains(GImNodes->MousePos))
    {
        GImNodes->NodeIndicesOverlappingWithMouse.push_back(GImNodes->CurrentNodeIdx);
    }
}

ImVec2 GetNodeDimensions(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1); // invalid node_id
    const ImNodeData& node = editor.Nodes.Pool[node_idx];
    return node.Rect.GetSize();
}

void BeginNodeTitleBar()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    ImGui::BeginGroup();
}

void EndNodeTitleBar()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    ImGui::EndGroup();

    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
    node.TitleBarContentRect = GetItemRect();

    ImGui::ItemAdd(GetNodeTitleRect(node), ImGui::GetID("title_bar"));

    ImGui::SetCursorPos(GridSpaceToEditorSpace(editor, GetNodeContentOrigin(node)));
}

void BeginInputAttribute(const int id, const ImNodesPinShape shape)
{
    BeginPinAttribute(id, ImNodesAttributeType_Input, shape, GImNodes->CurrentNodeIdx);
}

void EndInputAttribute() { EndPinAttribute(); }

void BeginOutputAttribute(const int id, const ImNodesPinShape shape)
{
    BeginPinAttribute(id, ImNodesAttributeType_Output, shape, GImNodes->CurrentNodeIdx);
}

void EndOutputAttribute() { EndPinAttribute(); }

void BeginStaticAttribute(const int id)
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Attribute;

    GImNodes->CurrentAttributeId = id;

    ImGui::BeginGroup();
    ImGui::PushID(id);
}

void EndStaticAttribute()
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    assert(GImNodes->CurrentScope == ImNodesScope_Attribute);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        GImNodes->ActiveAttribute = true;
        GImNodes->ActiveAttributeId = GImNodes->CurrentAttributeId;
    }
}

void PushAttributeFlag(const ImNodesAttributeFlags flag)
{
    GImNodes->CurrentAttributeFlags |= flag;
    GImNodes->AttributeFlagStack.push_back(GImNodes->CurrentAttributeFlags);
}

void PopAttributeFlag()
{
    // PopAttributeFlag called without a matching PushAttributeFlag!
    // The bottom value is always the default value, pushed in Initialize().
    assert(GImNodes->AttributeFlagStack.size() > 1);

    GImNodes->AttributeFlagStack.pop_back();
    GImNodes->CurrentAttributeFlags = GImNodes->AttributeFlagStack.back();
}

void Link(const int id, const int start_attr_id, const int end_attr_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);

    ImNodesEditorContext& editor = EditorContextGet();
    ImLinkData&           link = ObjectPoolFindOrCreateObject(editor.Links, id);
    link.Id = id;
    link.StartPinIdx = ObjectPoolFindOrCreateIndex(editor.Pins, start_attr_id);
    link.EndPinIdx = ObjectPoolFindOrCreateIndex(editor.Pins, end_attr_id);
    link.ColorStyle.Base = GImNodes->Style.Colors[ImNodesCol_Link];
    link.ColorStyle.Hovered = GImNodes->Style.Colors[ImNodesCol_LinkHovered];
    link.ColorStyle.Selected = GImNodes->Style.Colors[ImNodesCol_LinkSelected];

    // Check if this link was created by the current link event
    if ((editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation &&
         editor.Pins.Pool[link.EndPinIdx].Flags & ImNodesAttributeFlags_EnableLinkCreationOnSnap &&
         editor.ClickInteraction.LinkCreation.StartPinIdx == link.StartPinIdx &&
         editor.ClickInteraction.LinkCreation.EndPinIdx == link.EndPinIdx) ||
        (editor.ClickInteraction.LinkCreation.StartPinIdx == link.EndPinIdx &&
         editor.ClickInteraction.LinkCreation.EndPinIdx == link.StartPinIdx))
    {
        GImNodes->SnapLinkIdx = ObjectPoolFindOrCreateIndex(editor.Links, id);
    }
}

void PushColorStyle(const ImNodesCol item, unsigned int color)
{
    GImNodes->ColorModifierStack.push_back(ImNodesColElement(GImNodes->Style.Colors[item], item));
    GImNodes->Style.Colors[item] = color;
}

void PopColorStyle()
{
    assert(GImNodes->ColorModifierStack.size() > 0);
    const ImNodesColElement elem = GImNodes->ColorModifierStack.back();
    GImNodes->Style.Colors[elem.Item] = elem.Color;
    GImNodes->ColorModifierStack.pop_back();
}

struct ImNodesStyleVarInfo
{
    ImGuiDataType Type;
    ImU32         Count;
    ImU32         Offset;
    void* GetVarPtr(ImNodesStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImNodesStyleVarInfo GStyleVarInfo[] = {
    // ImNodesStyleVar_GridSpacing
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, GridSpacing)},
    // ImNodesStyleVar_NodeCornerRounding
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, NodeCornerRounding)},
    // ImNodesStyleVar_NodePadding
    {ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImNodesStyle, NodePadding)},
    // ImNodesStyleVar_NodeBorderThickness
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, NodeBorderThickness)},
    // ImNodesStyleVar_LinkThickness
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, LinkThickness)},
    // ImNodesStyleVar_LinkLineSegmentsPerLength
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, LinkLineSegmentsPerLength)},
    // ImNodesStyleVar_LinkHoverDistance
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, LinkHoverDistance)},
    // ImNodesStyleVar_PinCircleRadius
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinCircleRadius)},
    // ImNodesStyleVar_PinQuadSideLength
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinQuadSideLength)},
    // ImNodesStyleVar_PinTriangleSideLength
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinTriangleSideLength)},
    // ImNodesStyleVar_PinLineThickness
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinLineThickness)},
    // ImNodesStyleVar_PinHoverRadius
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinHoverRadius)},
    // ImNodesStyleVar_PinOffset
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinOffset)},
    // ImNodesStyleVar_MiniMapPadding
    {ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImNodesStyle, MiniMapPadding)},
    // ImNodesStyleVar_MiniMapOffset
    {ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImNodesStyle, MiniMapOffset)},
};

static const ImNodesStyleVarInfo* GetStyleVarInfo(ImNodesStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImNodesStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImNodesStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void PushStyleVar(const ImNodesStyleVar item, const float value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        float& style_var = *(float*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() float variant but variable is not a float!");
}

void PushStyleVar(const ImNodesStyleVar item, const ImVec2& value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImVec2& style_var = *(ImVec2*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() ImVec2 variant but variable is not a ImVec2!");
}

void PopStyleVar(int count)
{
    while (count > 0)
    {
        assert(GImNodes->StyleModifierStack.size() > 0);
        const ImNodesStyleVarElement style_backup = GImNodes->StyleModifierStack.back();
        GImNodes->StyleModifierStack.pop_back();
        const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(style_backup.Item);
        void*                      style_var = var_info->GetVarPtr(&GImNodes->Style);
        if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
        {
            ((float*)style_var)[0] = style_backup.FloatValue[0];
        }
        else if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
        {
            ((float*)style_var)[0] = style_backup.FloatValue[0];
            ((float*)style_var)[1] = style_backup.FloatValue[1];
        }
        count--;
    }
}

void SetNodeScreenSpacePos(const int node_id, const ImVec2& screen_space_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = ScreenSpaceToGridSpace(editor, screen_space_pos);
}

void SetNodeEditorSpacePos(const int node_id, const ImVec2& editor_space_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = EditorSpaceToGridSpace(editor, editor_space_pos);
}

void SetNodeGridSpacePos(const int node_id, const ImVec2& grid_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = grid_pos;
}

void SetNodeDraggable(const int node_id, const bool draggable)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Draggable = draggable;
}

ImVec2 GetNodeScreenSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1);
    ImNodeData& node = editor.Nodes.Pool[node_idx];
    return GridSpaceToScreenSpace(editor, node.Origin);
}

ImVec2 GetNodeEditorSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1);
    ImNodeData& node = editor.Nodes.Pool[node_idx];
    return GridSpaceToEditorSpace(editor, node.Origin);
}

ImVec2 GetNodeGridSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1);
    ImNodeData& node = editor.Nodes.Pool[node_idx];
    return node.Origin;
}

bool IsEditorHovered() { return MouseInCanvas(); }

bool IsNodeHovered(int* const node_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(node_id != NULL);

    const bool is_hovered = GImNodes->HoveredNodeIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *node_id = editor.Nodes.Pool[GImNodes->HoveredNodeIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsLinkHovered(int* const link_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(link_id != NULL);

    const bool is_hovered = GImNodes->HoveredLinkIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *link_id = editor.Links.Pool[GImNodes->HoveredLinkIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsPinHovered(int* const attr)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(attr != NULL);

    const bool is_hovered = GImNodes->HoveredPinIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *attr = editor.Pins.Pool[GImNodes->HoveredPinIdx.Value()].Id;
    }
    return is_hovered;
}

int NumSelectedNodes()
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedNodeIndices.size();
}

int NumSelectedLinks()
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedLinkIndices.size();
}

void GetSelectedNodes(int* node_ids)
{
    assert(node_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedNodeIndices.size(); ++i)
    {
        const int node_idx = editor.SelectedNodeIndices[i];
        node_ids[i] = editor.Nodes.Pool[node_idx].Id;
    }
}

void GetSelectedLinks(int* link_ids)
{
    assert(link_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedLinkIndices.size(); ++i)
    {
        const int link_idx = editor.SelectedLinkIndices[i];
        link_ids[i] = editor.Links.Pool[link_idx].Id;
    }
}

void ClearNodeSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedNodeIndices.clear();
}

void ClearNodeSelection(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ClearObjectSelection(editor.Nodes, editor.SelectedNodeIndices, node_id);
}

void ClearLinkSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedLinkIndices.clear();
}

void ClearLinkSelection(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ClearObjectSelection(editor.Links, editor.SelectedLinkIndices, link_id);
}

void SelectNode(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    SelectObject(editor.Nodes, editor.SelectedNodeIndices, node_id);
}

void SelectLink(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    SelectObject(editor.Links, editor.SelectedLinkIndices, link_id);
}

bool IsNodeSelected(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsObjectSelected(editor.Nodes, editor.SelectedNodeIndices, node_id);
}

bool IsLinkSelected(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsObjectSelected(editor.Links, editor.SelectedLinkIndices, link_id);
}

bool IsAttributeActive()
{
    assert((GImNodes->CurrentScope & ImNodesScope_Node) != 0);

    if (!GImNodes->ActiveAttribute)
    {
        return false;
    }

    return GImNodes->ActiveAttributeId == GImNodes->CurrentAttributeId;
}

bool IsAnyAttributeActive(int* const attribute_id)
{
    assert((GImNodes->CurrentScope & (ImNodesScope_Node | ImNodesScope_Attribute)) == 0);

    if (!GImNodes->ActiveAttribute)
    {
        return false;
    }

    if (attribute_id != NULL)
    {
        *attribute_id = GImNodes->ActiveAttributeId;
    }

    return true;
}

bool IsLinkStarted(int* const started_at_id)
{
    // Call this function after EndNodeEditor()!
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(started_at_id != NULL);

    const bool is_started = (GImNodes->ImNodesUIState & ImNodesUIState_LinkStarted) != 0;
    if (is_started)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   pin_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        *started_at_id = editor.Pins.Pool[pin_idx].Id;
    }

    return is_started;
}

bool IsLinkDropped(int* const started_at_id, const bool including_detached_links)
{
    // Call this function after EndNodeEditor()!
    assert(GImNodes->CurrentScope == ImNodesScope_None);

    const ImNodesEditorContext& editor = EditorContextGet();

    const bool link_dropped =
        (GImNodes->ImNodesUIState & ImNodesUIState_LinkDropped) != 0 &&
        (including_detached_links ||
         editor.ClickInteraction.LinkCreation.Type != ImNodesLinkCreationType_FromDetach);

    if (link_dropped && started_at_id)
    {
        const int pin_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        *started_at_id = editor.Pins.Pool[pin_idx].Id;
    }

    return link_dropped;
}

bool IsLinkCreated(
    int* const  started_at_pin_id,
    int* const  ended_at_pin_id,
    bool* const created_from_snap)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(started_at_pin_id != NULL);
    assert(ended_at_pin_id != NULL);

    const bool is_created = (GImNodes->ImNodesUIState & ImNodesUIState_LinkCreated) != 0;

    if (is_created)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   start_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        const int        end_idx = editor.ClickInteraction.LinkCreation.EndPinIdx.Value();
        const ImPinData& start_pin = editor.Pins.Pool[start_idx];
        const ImPinData& end_pin = editor.Pins.Pool[end_idx];

        if (start_pin.Type == ImNodesAttributeType_Output)
        {
            *started_at_pin_id = start_pin.Id;
            *ended_at_pin_id = end_pin.Id;
        }
        else
        {
            *started_at_pin_id = end_pin.Id;
            *ended_at_pin_id = start_pin.Id;
        }

        if (created_from_snap)
        {
            *created_from_snap =
                editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation;
        }
    }

    return is_created;
}

bool IsLinkCreated(
    int*  started_at_node_id,
    int*  started_at_pin_id,
    int*  ended_at_node_id,
    int*  ended_at_pin_id,
    bool* created_from_snap)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(started_at_node_id != NULL);
    assert(started_at_pin_id != NULL);
    assert(ended_at_node_id != NULL);
    assert(ended_at_pin_id != NULL);

    const bool is_created = (GImNodes->ImNodesUIState & ImNodesUIState_LinkCreated) != 0;

    if (is_created)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   start_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        const int         end_idx = editor.ClickInteraction.LinkCreation.EndPinIdx.Value();
        const ImPinData&  start_pin = editor.Pins.Pool[start_idx];
        const ImNodeData& start_node = editor.Nodes.Pool[start_pin.ParentNodeIdx];
        const ImPinData&  end_pin = editor.Pins.Pool[end_idx];
        const ImNodeData& end_node = editor.Nodes.Pool[end_pin.ParentNodeIdx];

        if (start_pin.Type == ImNodesAttributeType_Output)
        {
            *started_at_pin_id = start_pin.Id;
            *started_at_node_id = start_node.Id;
            *ended_at_pin_id = end_pin.Id;
            *ended_at_node_id = end_node.Id;
        }
        else
        {
            *started_at_pin_id = end_pin.Id;
            *started_at_node_id = end_node.Id;
            *ended_at_pin_id = start_pin.Id;
            *ended_at_node_id = start_node.Id;
        }

        if (created_from_snap)
        {
            *created_from_snap =
                editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation;
        }
    }

    return is_created;
}

bool IsLinkDestroyed(int* const link_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);

    const bool link_destroyed = GImNodes->DeletedLinkIdx.HasValue();
    if (link_destroyed)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   link_idx = GImNodes->DeletedLinkIdx.Value();
        *link_id = editor.Links.Pool[link_idx].Id;
    }

    return link_destroyed;
}

namespace
{
void NodeLineHandler(ImNodesEditorContext& editor, const char* const line)
{
    int id;
    int x, y;
    if (sscanf(line, "[node.%i", &id) == 1)
    {
        const int node_idx = ObjectPoolFindOrCreateIndex(editor.Nodes, id);
        GImNodes->CurrentNodeIdx = node_idx;
        ImNodeData& node = editor.Nodes.Pool[node_idx];
        node.Id = id;
    }
    else if (sscanf(line, "origin=%i,%i", &x, &y) == 2)
    {
        ImNodeData& node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
        node.Origin = ImVec2((float)x, (float)y);
    }
}

void EditorLineHandler(ImNodesEditorContext& editor, const char* const line)
{
    (void)sscanf(line, "panning=%f,%f", &editor.Panning.x, &editor.Panning.y);
}
} // namespace

const char* SaveCurrentEditorStateToIniString(size_t* const data_size)
{
    return SaveEditorStateToIniString(&EditorContextGet(), data_size);
}

const char* SaveEditorStateToIniString(
    const ImNodesEditorContext* const editor_ptr,
    size_t* const                     data_size)
{
    assert(editor_ptr != NULL);
    const ImNodesEditorContext& editor = *editor_ptr;

    GImNodes->TextBuffer.clear();
    // TODO: check to make sure that the estimate is the upper bound of element
    GImNodes->TextBuffer.reserve(64 * editor.Nodes.Pool.size());

    GImNodes->TextBuffer.appendf(
        "[editor]\npanning=%i,%i\n", (int)editor.Panning.x, (int)editor.Panning.y);

    for (int i = 0; i < editor.Nodes.Pool.size(); i++)
    {
        if (editor.Nodes.InUse[i])
        {
            const ImNodeData& node = editor.Nodes.Pool[i];
            GImNodes->TextBuffer.appendf("\n[node.%d]\n", node.Id);
            GImNodes->TextBuffer.appendf("origin=%i,%i\n", (int)node.Origin.x, (int)node.Origin.y);
        }
    }

    if (data_size != NULL)
    {
        *data_size = GImNodes->TextBuffer.size();
    }

    return GImNodes->TextBuffer.c_str();
}

void LoadCurrentEditorStateFromIniString(const char* const data, const size_t data_size)
{
    LoadEditorStateFromIniString(&EditorContextGet(), data, data_size);
}

void LoadEditorStateFromIniString(
    ImNodesEditorContext* const editor_ptr,
    const char* const           data,
    const size_t                data_size)
{
    if (data_size == 0u)
    {
        return;
    }

    ImNodesEditorContext& editor = editor_ptr == NULL ? EditorContextGet() : *editor_ptr;

    char*       buf = (char*)ImGui::MemAlloc(data_size + 1);
    const char* buf_end = buf + data_size;
    memcpy(buf, data, data_size);
    buf[data_size] = 0;

    void (*line_handler)(ImNodesEditorContext&, const char*);
    line_handler = NULL;
    char* line_end = NULL;
    for (char* line = buf; line < buf_end; line = line_end + 1)
    {
        while (*line == '\n' || *line == '\r')
        {
            line++;
        }
        line_end = line;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
        {
            line_end++;
        }
        line_end[0] = 0;

        if (*line == ';' || *line == '\0')
        {
            continue;
        }

        if (line[0] == '[' && line_end[-1] == ']')
        {
            line_end[-1] = 0;
            if (strncmp(line + 1, "node", 4) == 0)
            {
                line_handler = NodeLineHandler;
            }
            else if (strcmp(line + 1, "editor") == 0)
            {
                line_handler = EditorLineHandler;
            }
        }

        if (line_handler != NULL)
        {
            line_handler(editor, line);
        }
    }
    ImGui::MemFree(buf);
}

void SaveCurrentEditorStateToIniFile(const char* const file_name)
{
    SaveEditorStateToIniFile(&EditorContextGet(), file_name);
}

void SaveEditorStateToIniFile(const ImNodesEditorContext* const editor, const char* const file_name)
{
    size_t      data_size = 0u;
    const char* data = SaveEditorStateToIniString(editor, &data_size);
    FILE*       file = ImFileOpen(file_name, "wt");
    if (!file)
    {
        return;
    }

    fwrite(data, sizeof(char), data_size, file);
    fclose(file);
}

void LoadCurrentEditorStateFromIniFile(const char* const file_name)
{
    LoadEditorStateFromIniFile(&EditorContextGet(), file_name);
}

void LoadEditorStateFromIniFile(ImNodesEditorContext* const editor, const char* const file_name)
{
    size_t data_size = 0u;
    char*  file_data = (char*)ImFileLoadToMemory(file_name, "rb", &data_size);

    if (!file_data)
    {
        return;
    }

    LoadEditorStateFromIniString(editor, file_data, data_size);
    ImGui::MemFree(file_data);
}
} // namespace IMNODES_NAMESPACE
