void DarkColorsNoTransparent(void)
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	//colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_Text] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	//colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.98f);
	colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	//colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 0.94f);
	//colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
	//colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
//	colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
	colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.25f);
//	colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.40f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
	//colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	//colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.78f);

	colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	//PE: VS2022 style
	const float r = pref.highlight_color.x; // (1.0f / 255.0f) * 14;
	const float g = pref.highlight_color.y; // (1.0f / 255.0f) * 99;
	const float b = pref.highlight_color.z; // (1.0f / 255.0f) * 156;

	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(r, g, b, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
	colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);

	auto* style = &ImGui::GetStyle();
	style->ItemSpacing.y = 3.0f;
	style->TabRounding = 0;
	style->ChildRounding = 0;
	style->FrameRounding = 3;
	style->ScrollbarRounding = 1;
	style->WindowBorderSize = 0;

	TintCurrentStyle();
}

void myStyle2(ImGuiStyle* dst)
{
	auto *style = (dst ? dst : &ImGui::GetStyle());
	style->WindowRounding = 5.3f;
	style->GrabRounding = style->FrameRounding = 2.3f;
	style->ScrollbarRounding = 5.0f;
	style->FrameBorderSize = 1.0f;
	style->ItemSpacing.y = 6.5f;

	style->ScrollbarSize = 16.0;

	style->Colors[ImGuiCol_Text] = { 0.78f, 0.78f, 0.78f, 1.00f };
	style->Colors[ImGuiCol_TextDisabled] = { 0.55f, 0.55f, 0.55f, 1.00f };

	style->Colors[ImGuiCol_WindowBg] = { 0.23f, 0.23f, 0.23f, 0.75f }; //0.98f
	style->Colors[ImGuiCol_ChildBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.00f };

	style->Colors[ImGuiCol_PopupBg] = { 0.31f, 0.32f, 0.34f, 1.0f }; //0.94

	style->Colors[ImGuiCol_Border] = { 0.33333334f, 0.33333334f, 0.33333334f, 0.50f };
	style->Colors[ImGuiCol_BorderShadow] = { 0.15686275f, 0.15686275f, 0.15686275f, 0.00f };

	style->Colors[ImGuiCol_FrameBg] = { 0.16862746f, 0.16862746f, 0.16862746f, 0.64f };

	style->Colors[ImGuiCol_FrameBgHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.77f };
	style->Colors[ImGuiCol_FrameBgActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.77f };

	style->Colors[ImGuiCol_TitleBg] = { 0.04f, 0.04f, 0.04f, 1.00f };
	style->Colors[ImGuiCol_TitleBgCollapsed] = { 0.16f, 0.29f, 0.48f, 1.00f };
	style->Colors[ImGuiCol_TitleBgActive] = { 0.00f, 0.00f, 0.00f, 1.0f };

	style->Colors[ImGuiCol_MenuBarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.92f };

	style->Colors[ImGuiCol_ScrollbarBg] = { 0.195f, 0.195f, 0.195f, 0.60f };
	style->Colors[ImGuiCol_ScrollbarGrab] = { 0.39f, 0.39f, 0.39f, 0.51f };

	style->Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ScrollbarGrabActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 0.91f };
	// style->Colors[ImGuiCol_ComboBg]               = {0.1f, 0.1f, 0.1f, 0.99f};
	style->Colors[ImGuiCol_CheckMark] = { 0.90f, 0.90f, 0.90f, 0.83f };
	style->Colors[ImGuiCol_SliderGrab] = { 0.70f, 0.70f, 0.70f, 0.62f };
	style->Colors[ImGuiCol_SliderGrabActive] = { 0.30f, 0.30f, 0.30f, 0.84f };
	style->Colors[ImGuiCol_Button] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.49f };
	style->Colors[ImGuiCol_ButtonHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ButtonActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 1.00f };
	style->Colors[ImGuiCol_Header] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.53f };
	style->Colors[ImGuiCol_HeaderHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
	style->Colors[ImGuiCol_HeaderActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
	style->Colors[ImGuiCol_Separator] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorHovered] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorActive] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_ResizeGrip] = { 1.00f, 1.00f, 1.00f, 0.85f };
	style->Colors[ImGuiCol_ResizeGripHovered] = { 1.00f, 1.00f, 1.00f, 0.60f };
	style->Colors[ImGuiCol_ResizeGripActive] = { 1.00f, 1.00f, 1.00f, 0.90f };
	style->Colors[ImGuiCol_PlotLines] = { 0.61f, 0.61f, 0.61f, 1.00f };
	style->Colors[ImGuiCol_PlotLinesHovered] = { 1.00f, 0.43f, 0.35f, 1.00f };
	style->Colors[ImGuiCol_PlotHistogram] = { 0.90f, 0.70f, 0.00f, 1.00f }; //Also <h1> tags in help.
	style->Colors[ImGuiCol_PlotHistogramHovered] = { 1.00f, 0.60f, 0.00f, 1.00f };
	style->Colors[ImGuiCol_TextSelectedBg] = { 0.18431373f, 0.39607847f, 0.79215693f, 0.90f };

	//Still need to be set.
	style->Colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style->Colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
	 style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

	style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);


	style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);

	TintCurrentStyle();
}


void myStyle2_colors_only(void)
{
	auto *style = &ImGui::GetStyle();

	style->Colors[ImGuiCol_Text] = { 0.78f, 0.78f, 0.78f, 1.00f };
	style->Colors[ImGuiCol_TextDisabled] = { 0.55f, 0.55f, 0.55f, 1.00f };
	style->Colors[ImGuiCol_WindowBg] = { 0.23f, 0.23f, 0.23f, 0.75f }; //0.98f
	style->Colors[ImGuiCol_ChildBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.00f };
	style->Colors[ImGuiCol_PopupBg] = { 0.31f, 0.32f, 0.34f, 1.0f }; //0.94
	style->Colors[ImGuiCol_Border] = { 0.33333334f, 0.33333334f, 0.33333334f, 0.50f };
	style->Colors[ImGuiCol_BorderShadow] = { 0.15686275f, 0.15686275f, 0.15686275f, 0.00f };
	style->Colors[ImGuiCol_FrameBg] = { 0.16862746f, 0.16862746f, 0.16862746f, 0.64f };
	style->Colors[ImGuiCol_FrameBgHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.77f };
	style->Colors[ImGuiCol_FrameBgActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.77f };
	style->Colors[ImGuiCol_TitleBg] = { 0.04f, 0.04f, 0.04f, 1.00f };
	style->Colors[ImGuiCol_TitleBgCollapsed] = { 0.16f, 0.29f, 0.48f, 1.00f };
	style->Colors[ImGuiCol_TitleBgActive] = { 0.00f, 0.00f, 0.00f, 1.0f };
	style->Colors[ImGuiCol_MenuBarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.92f };
	style->Colors[ImGuiCol_ScrollbarBg] = { 0.195f, 0.195f, 0.195f, 0.60f };
	style->Colors[ImGuiCol_ScrollbarGrab] = { 0.39f, 0.39f, 0.39f, 0.51f };
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ScrollbarGrabActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 0.91f };
	style->Colors[ImGuiCol_CheckMark] = { 0.90f, 0.90f, 0.90f, 0.83f };
	style->Colors[ImGuiCol_SliderGrab] = { 0.70f, 0.70f, 0.70f, 0.62f };
	style->Colors[ImGuiCol_SliderGrabActive] = { 0.30f, 0.30f, 0.30f, 0.84f };
	style->Colors[ImGuiCol_Button] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.49f };
	style->Colors[ImGuiCol_ButtonHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ButtonActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 1.00f };
	style->Colors[ImGuiCol_Header] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.53f };
	style->Colors[ImGuiCol_HeaderHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
	style->Colors[ImGuiCol_HeaderActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
	style->Colors[ImGuiCol_Separator] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorHovered] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorActive] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_ResizeGrip] = { 1.00f, 1.00f, 1.00f, 0.85f };
	style->Colors[ImGuiCol_ResizeGripHovered] = { 1.00f, 1.00f, 1.00f, 0.60f };
	style->Colors[ImGuiCol_ResizeGripActive] = { 1.00f, 1.00f, 1.00f, 0.90f };
	style->Colors[ImGuiCol_PlotLines] = { 0.61f, 0.61f, 0.61f, 1.00f };
	style->Colors[ImGuiCol_PlotLinesHovered] = { 1.00f, 0.43f, 0.35f, 1.00f };
	style->Colors[ImGuiCol_PlotHistogram] = { 0.90f, 0.70f, 0.00f, 1.00f }; //Also <h1> tags in help.
	style->Colors[ImGuiCol_PlotHistogramHovered] = { 1.00f, 0.60f, 0.00f, 1.00f };
	style->Colors[ImGuiCol_TextSelectedBg] = { 0.18431373f, 0.39607847f, 0.79215693f, 0.90f };
	style->Colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style->Colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
//	style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

	style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);
	style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);

	//TintCurrentStyle();
}

void myStyle3(ImGuiStyle* dst)
{
	ImGuiStyle &st = ImGui::GetStyle();
	st.FrameBorderSize = 1.0f;
	st.FramePadding = ImVec2(4.0f, 2.0f);
	st.ItemSpacing = ImVec2(8.0f, 2.0f);
	st.WindowBorderSize = 2.0f;
	//	st.TabBorderSize = 1.0f;
	st.WindowRounding = 1.0f;
	st.ChildRounding = 1.0f;
	st.FrameRounding = 1.0f;
	st.ScrollbarRounding = 1.0f;
	st.ScrollbarSize = 18.0;
	st.GrabRounding = 1.0f;
	//	st.TabRounding = 1.0f;

	//	st.TabBorderSize = 5.0f;
	//	st.TabRounding = 2.0f;


	// Setup style
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 0.941f);
	colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.031f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.25f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.114f, 0.191f, 0.199f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.203f, 0.335f, 0.348f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);

	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
	colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
	colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
	colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.5f, 0.5f, 0.5f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
	colors[ImGuiCol_Tab] = ImVec4(0.114f, 0.161f, 0.200f, 0.86f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.134f, 0.181f, 0.220f, 0.86f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.114f, 0.161f, 0.200f, 0.86f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.124f, 0.171f, 0.210f, 1.0f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	TintCurrentStyle();
}


void TintColor(ImVec4 & color, int iModeColor = 0)
{
	ImVec4 tint = pref.tint_style;
	ImVec4 shade = pref.shade_style;
	ImVec4 title = pref.title_style;

	if (pref.iEnableCustomColors || (pref.current_style >= 10 && pref.current_style != 25 ))
	{
		//PE: pref.current_style == 25 (Blue) is a little special as it is used as a new "main" color setup.
		bool bUseGrayScale = true;
		float grayscale = (color.x + color.y + color.z) / 3.0;
		if (iModeColor == 2)
		{
			bool bNoTitle = false;
			if (title.x <= 0.008 && title.y <= 0.008 && title.z <= 0.008 && pref.current_style == 25)
				bNoTitle = true;
			if (!bNoTitle)
			{
				if (grayscale > 0.5)
				{
					//Darken.
					color.x = grayscale * (title.x);
					color.y = grayscale * (title.y);
					color.z = grayscale * (title.z);
				}
				else {
					//Lighting
					color.x = grayscale + title.x + title.x / 3;
					color.y = grayscale + title.y + title.y / 3;
					color.z = grayscale + title.z + title.z / 3;
				}
			}
		}
		else if (iModeColor == 1)
		{

			bool bNoTint = false;
			if (tint.x >= 0.998 && tint.y >= 0.998 && tint.z >= 0.998 && pref.current_style == 25)
				bNoTint = true;

			if(!bNoTint)
			{
				//PE: Dont grayscale "text" we need the yellow/highlight colors.
				if (grayscale > 0.5)
				{
					//Darken.
					color.x = color.x * (tint.x);
					color.y = color.y * (tint.y);
					color.z = color.z * (tint.z);
				}
				else {
					//Lighting
					color.x = color.x + tint.x + tint.x / 3;
					color.y = color.y + tint.y + tint.y / 3;
					color.z = color.z + tint.z + tint.z / 3;
				}
			}
		}
		else
		{

			bool bNoShade = false;
			if (shade.x <= 0.008 && shade.y <= 0.008 && shade.z <= 0.008 && pref.current_style == 25)
				bNoShade = true;

			if (!bNoShade)
			{
				if (bUseGrayScale)
				{
					if (grayscale > 0.5)
					{
						//Darken.
						color.x = grayscale * (shade.x);
						color.y = grayscale * (shade.y);
						color.z = grayscale * (shade.z);
					}
					else {
						//Lighting
						color.x = grayscale + shade.x + shade.x / 3;
						color.y = grayscale + shade.y + shade.y / 3;
						color.z = grayscale + shade.z + shade.z / 3;
					}
				}
				else
				{
					if (grayscale > 0.5)
					{
						//Darken.
						color.x = color.x * (shade.x);
						color.y = color.y * (shade.y);
						color.z = color.z * (shade.z);
					}
					else {
						//Lighting
						color.x = color.x + shade.x + shade.x / 3;
						color.y = color.y + shade.y + shade.y / 3;
						color.z = color.z + shade.z + shade.z / 3;
					}
				}
			}
		}
	}

	if (pref.iTurnOffUITransparent != 0)
		color.w = 1.0f;

}

//Text
//CheckMark
//ResizeGrip
//NavWindow ?
//PlotHistogram

void TintCurrentStyle( void )
{
	auto *style = &ImGui::GetStyle();

	//Light
	TintColor(style->Colors[ImGuiCol_Text],1);
	TintColor(style->Colors[ImGuiCol_CheckMark],1);
	TintColor(style->Colors[ImGuiCol_ResizeGrip],1);
	TintColor(style->Colors[ImGuiCol_PlotHistogram],1);
	TintColor(style->Colors[ImGuiCol_SliderGrab],1);


	//Highlight , title and tabs.
	TintColor(style->Colors[ImGuiCol_TitleBg],2);
	TintColor(style->Colors[ImGuiCol_TitleBgCollapsed], 2);
	TintColor(style->Colors[ImGuiCol_TitleBgActive], 2);
	TintColor(style->Colors[ImGuiCol_Tab], 2);
	TintColor(style->Colors[ImGuiCol_TabHovered], 2);
	TintColor(style->Colors[ImGuiCol_TabActive], 2);
	TintColor(style->Colors[ImGuiCol_TabUnfocused], 2);
	TintColor(style->Colors[ImGuiCol_TabUnfocusedActive], 2);


	//Dark
	TintColor(style->Colors[ImGuiCol_TextDisabled]);
	TintColor(style->Colors[ImGuiCol_WindowBg]);
	TintColor(style->Colors[ImGuiCol_ChildBg]);
	TintColor(style->Colors[ImGuiCol_PopupBg]);
	TintColor(style->Colors[ImGuiCol_Border]);
	TintColor(style->Colors[ImGuiCol_BorderShadow]);
	TintColor(style->Colors[ImGuiCol_FrameBg]);
	TintColor(style->Colors[ImGuiCol_FrameBgHovered]);
	TintColor(style->Colors[ImGuiCol_FrameBgActive]);
	TintColor(style->Colors[ImGuiCol_MenuBarBg]);
	TintColor(style->Colors[ImGuiCol_ScrollbarBg]);
	TintColor(style->Colors[ImGuiCol_ScrollbarGrab]);
	TintColor(style->Colors[ImGuiCol_ScrollbarGrabHovered]);
	TintColor(style->Colors[ImGuiCol_ScrollbarGrabActive]);

	TintColor(style->Colors[ImGuiCol_SliderGrabActive]);
	TintColor(style->Colors[ImGuiCol_Button]);
	TintColor(style->Colors[ImGuiCol_ButtonHovered]);
	TintColor(style->Colors[ImGuiCol_ButtonActive]);
	TintColor(style->Colors[ImGuiCol_Header]);
	TintColor(style->Colors[ImGuiCol_HeaderHovered]);
	TintColor(style->Colors[ImGuiCol_HeaderActive]);
	TintColor(style->Colors[ImGuiCol_Separator]);
	TintColor(style->Colors[ImGuiCol_SeparatorHovered]);
	TintColor(style->Colors[ImGuiCol_SeparatorActive]);
	TintColor(style->Colors[ImGuiCol_ResizeGripHovered]);
	TintColor(style->Colors[ImGuiCol_ResizeGripActive]);
	TintColor(style->Colors[ImGuiCol_PlotLines]);
	TintColor(style->Colors[ImGuiCol_PlotLinesHovered]);
	TintColor(style->Colors[ImGuiCol_PlotHistogramHovered]);
	TintColor(style->Colors[ImGuiCol_TextSelectedBg]);

	//Still need to be set.
	TintColor(style->Colors[ImGuiCol_DockingPreview]);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
	//style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	TintColor(style->Colors[ImGuiCol_NavHighlight]);
	TintColor(style->Colors[ImGuiCol_NavWindowingHighlight]);
	TintColor(style->Colors[ImGuiCol_NavWindowingDimBg]);
	TintColor(style->Colors[ImGuiCol_ModalWindowDimBg]);
	TintColor(style->Colors[ImGuiCol_DragDropTarget]);

	//PE: To prevent flicker when changing tint, this code has been added to the main loop.
	ImGui::GetStyle().Colors[ImGuiCol_ChildBg] = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
}



ImFont* defaultfont;
ImFont* customfont;
ImFont* customfontlarge;
ImFont* tmpfont;

static const ImWchar Generic_ranges_all[] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0x0100, 0x017F,	//0100 — 017F  	Latin Extended-A
	0x0180, 0x024F,	//0180 — 024F  	Latin Extended-B
	0,
};
//PE: Optimize exclude Phonetic Extensions (1D00 — 1D7F) , phonetic lettering (japan,china), Greak/Latin Extended, block elements,Dingbats,...
static const ImWchar Generic_ranges_everything[] =
{
   0x0020, 0x07FF, // unicode 0x800-0x900 not defined so exclude from here.
   0,
};

void ChangeGGFont(const char *cpcustomfont, int iIDEFontSize)
{
	//MessageBoxA(NULL, "ChangeGGFont", "ChangeGGFont", 0);

	//PE: Add all lang.
//	static const ImWchar Generic_ranges_everything[] =
//	{
//	   0x0020, 0xFFFF, // Everything test.
//	   0,
//	};

	float FONTUPSCALE = 2.0; //Font upscaling.

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->Clear();

	if (FileExist((char*)cpcustomfont))
	{

		customfont = io.Fonts->AddFontFromFileTTF(cpcustomfont, iIDEFontSize*FONTUPSCALE, NULL, &Generic_ranges_everything[0]); //Set as default font.
		customfontlarge = io.Fonts->AddFontFromFileTTF(cpcustomfont, 60, NULL, &Generic_ranges_everything[0]); //Set as default font.

#ifdef IMGUIAL_FONTS_MATERIAL_DESIGN
		int ttf_size;
		const void* ttf_data = ImGuiAl::Fonts::GetCompressedData(ImGuiAl::Fonts::kMaterialDesign, &ttf_size);

		if (ttf_data) {
			static const ImWchar ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
			ImFontConfig config;
			config.MergeMode = true;
			config.PixelSnapH = true;
			if (pref.bUseUpscaling)
				config.GlyphOffset.y += 7.0f;
			else
				config.GlyphOffset.y += 3.5f;
			customfont = io.Fonts->AddFontFromMemoryCompressedTTF(ttf_data, ttf_size, iIDEFontSize*FONTUPSCALE, &config, ranges);
		}

#endif
#ifdef IMGUIAL_FONTS_FONT_AWESOME
		//Not used yet.
		static const ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig config;
		config.MergeMode = true;
		config.PixelSnapH = true;
		config.GlyphOffset.y += 0.5f;
		ttf_data = ImGuiAl::Fonts::GetCompressedData(ImGuiAl::Fonts::kFontAwesome, &ttf_size);
		if (ttf_data)
			io.Fonts->AddFontFromMemoryCompressedTTF(ttf_data, ttf_size, iIDEFontSize*FONTUPSCALE, &config, ranges);
#endif
		io.FontGlobalScale = 1.0f / FONTUPSCALE;
	}
	else
	{
		customfont = io.Fonts->AddFontDefault();
	}
	defaultfont = io.Fonts->AddFontDefault();

	//Add all fonts from:

	extern std::vector< std::pair<ImFont*, std::string>> StoryboardFonts;

	cstr pOldDir = GetDir();

	char destination[MAX_PATH];
	strcpy(destination, "editors\\templates\\fonts\\");
	SetDir(destination);
	ChecklistForFiles();
	SetDir(pOldDir.Get());
	DARKSDK LPSTR ChecklistString(int iIndex);
	DARKSDK int ChecklistQuantity(void);
	for (int c = 1; c <= ChecklistQuantity(); c++)
	{
		char *file = ChecklistString(c);
		if (file)
		{
			if (strlen(file) > 4)
			{
				if (strnicmp(file + strlen(file) - 4, ".ttf", 4) == NULL || strnicmp(file + strlen(file) - 4, ".otf", 4) == NULL)
				{
					//Add font.
					char path[MAX_PATH];
					strcpy(path, destination);
					strcat(path, file);
					const char *pestrcasestr(const char *arg1, const char *arg2);
					if( pestrcasestr(file,"arial"))
						tmpfont = io.Fonts->AddFontFromFileTTF(path, 60, NULL, &Generic_ranges_everything[0]); //Add font
					else
						tmpfont = io.Fonts->AddFontFromFileTTF(path, 60 , NULL, &Generic_ranges_all[0]); //Add font
					StoryboardFonts.push_back(std::make_pair(tmpfont,file));
				}
			}
		}
	}
	SetDir(pOldDir.Get());


	ImGui_ImplDX11_CreateDeviceObjects();

}

void AddRemoteProjectFonts(void)
{
	void timestampactivity(int i, char* desc_s);
	extern StoryboardStruct Storyboard;
	extern std::vector< std::pair<ImFont*, std::string>> StoryboardFonts;
	bool bAddedFonts = false;
	//PE: Need projectfolder here.
	if (strlen(Storyboard.gamename) > 0 && strlen(Storyboard.customprojectfolder) > 0)
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		cstr pOldDir = GetDir();
		char destination[MAX_PATH];
		strcpy(destination, Storyboard.customprojectfolder);
		strcat(destination, Storyboard.gamename);
		strcat(destination, "\\files\\editors\\templates\\fonts\\");
		if (PathExist(destination))
		{
			SetDir(destination);
			ChecklistForFiles();
			SetDir(pOldDir.Get());
			DARKSDK LPSTR ChecklistString(int iIndex);
			DARKSDK int ChecklistQuantity(void);
			for (int c = 1; c <= ChecklistQuantity(); c++)
			{
				char* file = ChecklistString(c);
				if (file)
				{
					if (strlen(file) > 4)
					{
						const char* pestrcasestr(const char* arg1, const char* arg2);
						if (strnicmp(file + strlen(file) - 4, ".ttf", 4) == NULL || strnicmp(file + strlen(file) - 4, ".otf", 4) == NULL)
						{
							bool bAlreadyThere = false;
							for (int i = 0; i < StoryboardFonts.size(); i++)
							{
								if (pestrcasestr(file, StoryboardFonts[i].second.c_str()))
								{
									bAlreadyThere = true;
									break;
								}
							}
							//Add font.
							if (!bAlreadyThere)
							{
								char msg[MAX_PATH];
								sprintf(msg, "Adding Font: %s", file);
								timestampactivity(0, msg);

								char path[MAX_PATH];
								strcpy(path, destination);
								strcat(path, file);
								if (pestrcasestr(file, "arial"))
									tmpfont = io.Fonts->AddFontFromFileTTF(path, 60, NULL, &Generic_ranges_everything[0]); //Add font
								else
									tmpfont = io.Fonts->AddFontFromFileTTF(path, 60, NULL, &Generic_ranges_all[0]); //Add font
								StoryboardFonts.push_back(std::make_pair(tmpfont, file));
								bAddedFonts = true;
							}
						}
					}
				}
			}
			if (bAddedFonts)
			{
				//ImGui_ImplDX11_CreateDeviceObjects();
				timestampactivity(0, "ImGui_ImplDX11_CreateFontsTexture();");
				ImGui_ImplDX11_CreateFontsTexture();
				//PE: old frame could have our old font texture , so disable it until newframe.
				bBlockImGuiUntilNewFrame = true;
			}
		}
		SetDir(pOldDir.Get());
	}
}

//struct case_insensitive_less //: public std::binary_function< char, char, bool >
//{
//	bool operator () (char x, char y) const
//	{
//		return toupper(static_cast< unsigned char >(x)) <
//			toupper(static_cast< unsigned char >(y));
//	}
//};

bool CaseInsensitiveLess(char x, char y)
{
	return toupper(static_cast<unsigned char>(x)) <
		toupper(static_cast<unsigned char>(y));
}

bool NoCaseLess(const std::string &a, const std::string &b)
{
	/*return std::lexicographical_compare(a.begin(), a.end(),
		b.begin(), b.end(), case_insensitive_less());*/
	return std::lexicographical_compare(a.begin(), a.end(),
		b.begin(), b.end(), CaseInsensitiveLess);
}


/* done in cStr
void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}
*/

//
const char *pestrcasestr(const char *arg1, const char *arg2);

//##########################################################
//#### Recursive get all files/folders inside a folder. ####
//##########################################################

#include "cStr.h"
class cFolderItem
{
public:
	struct sFolderFiles {
		sFolderFiles * m_dropptr; //Need to be the first entry for drag/drop.
		cStr m_sName;
		cStr m_sNameFinal;
		cStr m_sNameFinalCredit;
		cStr m_sPath;
		cStr m_sFolder;
		UINT iFlags;
		int iPreview; //Preview image.
		int iBigPreview; //Preview image.
		int id;
		int iAnimationFrom = 0;
		bool bPreviewProcessed;
		long last_used;
		bool bSorted;
		bool bFavorite;
		bool bAvailableInFreeTrial;
		time_t m_tFileModify;
		cstr m_Backdrop;
		sFolderFiles * m_pNext;
		sFolderFiles * m_pNextTime;
		sFolderFiles * m_pCustomSort;
		cFolderItem *pNewFolder;
		cStr m_sBetterSearch;
		int uniqueId;
		bool m_bFPELoaded;
		cstr m_sFPEModel;
		cstr m_sFPETextured;
		cstr m_sFPEKeywords;
		cstr m_sFPEDBOFile;
		cstr m_sDLuaDescription;
		float m_fDLuaHeight;
		int m_iFPEDBOFileSize;
		bool m_bIsCharacterCreator;
		bool m_bIsGroupObject;
		int iType;
	};
	cStr m_sFolder;
	cStr m_sFolderFullPath;
	int m_iEntityOffset;
	cFolderItem *m_pNext;
	cFolderItem *m_pSubFolder;
	sFolderFiles * m_pFirstFile;
	bool m_bFilesRead;
	bool visible;
	bool alwaysvisible;
	bool deletethisentry;
	bool bIsCustomFolder;
	char cfolder[256]; //PE: Only for faster sorting.
	time_t m_tFolderModify;
	float m_fLastTimeUpdate;
	UINT iFlags;
	int count;
	int iType;
	cFolderItem() { m_pNext = 0; iType = 0; iFlags = 0; m_bFilesRead = false; m_pFirstFile = NULL; m_pNext = NULL; m_pSubFolder = NULL; m_fLastTimeUpdate = 0; m_iEntityOffset = 0; }
	~cFolderItem() { }
};


//#########################################
//#### PE: Sort for files and folders. ####
//#########################################

int cstring_cmp_folder(const void *a, const void *b)
{
	struct cFolderItem * ia = *(struct cFolderItem **) a;
	struct cFolderItem * ib = *(struct cFolderItem **) b;
	if (!ia) return(0);
	if (!ib) return(0);
	return _stricmp(ia->cfolder, ib->cfolder);
}

cFolderItem MainEntityList;
int iTotalFolders = 0;
int iTotalFiles = 0;
#include "cStr.h"

// seemingly not used!
//void Clear_MainEntityList(void)
//{
//	MainEntityList.m_pNext = NULL;
//	extern bool bExternal_Entities_Init;
//	bExternal_Entities_Init = false;
//}


//std::vector< std::pair<std::string, time_t> > files_time_stamp;
std::unordered_map<std::string, time_t> files_time_stamp;


std::vector<std::string> files_favorite;
std::vector<std::string> files_availableinfreetrial;
std::vector<std::string> files_pinned_categories;

std::string GetLineFromVectorFile(char *str, std::vector<std::string> & vecOfStrs, bool bIgnoreSpaces)
{
	for (int i = 0; i < vecOfStrs.size(); i++)
	{
		std::string entry = vecOfStrs[i];
		if (entry.length() > 0)
		{
			if (bIgnoreSpaces)
			{
				replaceAll(entry, " ", "");
				replaceAll(entry, "\t", "");
			}
			if (pestrcasestr(entry.c_str(), str))
			{
				return(vecOfStrs[i]);
			}
		}
	}
	return "";
}

std::string GetLineParameterFromVectorFile(char *str, std::vector<std::string> & vecOfStrs, bool bIgnoreSpaces)
{
	std::string Line = GetLineFromVectorFile(str, vecOfStrs, bIgnoreSpaces);
	if (Line.length() > 0)
	{
		//replaceAll(Line, " ", "");
		//replaceAll(Line, "\t", "");
		char *find = (char * ) pestrcasestr(Line.c_str(), "=");
		if (find)
		{
			find++;
			while (*find == ' ' || *find == '\t' && *find != 0)
				find++;
			int len = strlen(find);
			if (len > 1)
			{
				int i = len-1;
				while (find[i] == ' ' || find[i] == '\t' && find[i]!= 0)
				{
					find[i]= 0;
					i--;
				}
			}
			return(std::string(find));
		}
	}
	return "";
}

void RemoveStrStrFromVectorFile(char *str, std::vector<std::string> & vecOfStrs, bool bIgnoreSpaces)
{
	for (int i = 0; i < vecOfStrs.size(); i++)
	{
		std::string entry = vecOfStrs[i];
		if (entry.length() > 0)
		{
			if (bIgnoreSpaces)
			{
				replaceAll(entry, " ", "");
				replaceAll(entry, "\t", "");
			}
			if (pestrcasestr(entry.c_str(), str))
			{
				auto itr = std::find(vecOfStrs.begin(), vecOfStrs.end(), vecOfStrs[i]);
				if (itr != vecOfStrs.end())
				{
					vecOfStrs.erase(itr);
					RemoveStrStrFromVectorFile(str, vecOfStrs, bIgnoreSpaces);
					return;
				}
			}
		}
	}
}


bool getVectorFileContent(std::string fileName, std::vector<std::string> & vecOfStrs, bool bAllowEmpty)
{
	char szRealFilename[MAX_PATH];
	strcpy_s(szRealFilename, MAX_PATH, fileName.c_str());
	GG_GetRealPath(szRealFilename, 0);

	// Open the File
	std::ifstream in(szRealFilename);
	// Check if object is valid
	if (!in)
	{
		return false;
	}
	std::string str;
	while (std::getline(in, str))
	{
		if (str.size() > 0 || bAllowEmpty)
			vecOfStrs.push_back(str);
	}
	in.close();
	return true;
}

bool saveVectorFileContent(std::string fileName, std::vector<std::string> & vecOfStrs)
{
	char szRealFilename[MAX_PATH];
	strcpy_s(szRealFilename, MAX_PATH, fileName.c_str());
	GG_GetRealPath(szRealFilename, 1);

	std::ofstream output_file(szRealFilename);
	std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	std::copy(vecOfStrs.begin(), vecOfStrs.end(), output_iterator);
	return true;
}


//Thread to collect fpe informations.
std::vector<cFolderItem::sFolderFiles *> g_ScanFpeFiles;
bool g_bFpeScanning = false;
int g_iScannedFiles = 0;
std::thread* g_pFPEScan = NULL;
void fpe_thread_function(void)
{
	g_bFpeScanning = true;

	for (int n = 0; n < g_ScanFpeFiles.size(); n++)
	{
		g_iScannedFiles++;
		cFolderItem::sFolderFiles * item = g_ScanFpeFiles[n];
		if (item)
		{
			char fpe_file[MAX_PATH];
			strcpy(fpe_file, item->m_sPath.Get());
			strcat(fpe_file, "\\");
			strcat(fpe_file, item->m_sName.Get());
			//HANDLE hfile = GG_CreateFile(fpe_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			//We already have the full path so.
			//PE: getVectorFileContent already check if it exists.
			//HANDLE hfile = CreateFileA(fpe_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			//if (hfile != INVALID_HANDLE_VALUE)
			//{
			//	CloseHandle(hfile);

			{
				std::vector<std::string> fpe_vector;
				if (getVectorFileContent(fpe_file, fpe_vector, true))
				{
					bool ischaracter = false;

					std::string s_isgroupobject = GetLineParameterFromVectorFile("isgroupobject=", fpe_vector, true);
					item->m_bIsGroupObject = 0;
					if (s_isgroupobject.length() > 0 && atoi(s_isgroupobject.c_str()) == 1)
						item->m_bIsGroupObject = 1;

					//Get info.
					#ifdef INCLUDEPOLYGONSORT
					std::string model = GetLineParameterFromVectorFile("model=", fpe_vector, true);
					if (model.length() > 0)
						item->m_sFPEModel = model.c_str();
					else
					#endif
						item->m_sFPEModel = "";

					//PE: Currently not used in advanced fpe search.
					//std::string textured = GetLineParameterFromVectorFile("textured=", fpe_vector, true); //Not used yet.
					//if (textured.length() > 0)
					//	item->m_sFPETextured = textured.c_str();
					//else
						item->m_sFPETextured = "";

					//Ignore there , and dont display thumbs, dont work anyway.
					#ifdef INCLUDEPOLYGONSORT
					std::string s_ischaractercreator = GetLineParameterFromVectorFile("charactercreator=", fpe_vector, true);
					#endif
					item->m_bIsCharacterCreator = 0;
					#ifdef INCLUDEPOLYGONSORT
					if (s_ischaractercreator.length() > 0 && atoi(s_ischaractercreator.c_str()) == 1)
						item->m_bIsCharacterCreator = 1;
					#endif

					#ifdef INCLUDEPOLYGONSORT
					std::string s_ischaracter = GetLineParameterFromVectorFile("ischaracter=", fpe_vector, true);
					ischaracter = false;
					if (s_ischaracter.length() > 0 && atoi(s_ischaracter.c_str()) == 1)
						ischaracter = true;

					bool isAnimated = false;
					std::string s_animmax = GetLineParameterFromVectorFile("animmax=", fpe_vector, true);
					if (s_animmax.length() > 0 && atoi(s_animmax.c_str()) > 0)
						isAnimated = true;
					#endif

					std::string keywords = GetLineParameterFromVectorFile("keywords=", fpe_vector, true); //Not used yet.
					if (keywords.length() > 0)
						item->m_sFPEKeywords = keywords.c_str();
					else
						item->m_sFPEKeywords = "";

					item->m_sFPEDBOFile = "";
					item->m_iFPEDBOFileSize = 0;

					#ifdef INCLUDEPOLYGONSORT
					//PE: This step is not needed when we dont have polygon sort.
					if (model.length() > 0)
					{
						//Get DBO info.
						char sEntityBank[MAX_PATH];
						strcpy(sEntityBank, "entitybank\\");

						char * find = (char *) pestrcasestr(fpe_file, sEntityBank);
						if (find)
						{
							find += strlen(sEntityBank);
							char * find2 = (char *)pestrcasestr(find, "ebebank\\");
							if (find2) strcpy(sEntityBank,"");
							
							char epath[MAX_PATH];
							strcpy(epath, find);
							bool bNotFound = true;
							for (int i = strlen(epath); i > 1; i--)
							{
								if (epath[i-1] == '\\' || epath[i-1] == '/')
								{
									bNotFound = false;
									epath[i] = 0;
									break;
								}
							}
							if(bNotFound) strcpy(epath, "");

							char sFile[MAX_PATH];
							strcpy(sFile, "");
							if (item->m_bIsCharacterCreator == 0)
							{
								strcpy(sFile, sEntityBank);
								strcat(sFile, epath);
								strcat(sFile, item->m_sFPEModel.Get());
							}
							else
							{
								strcpy(sFile, item->m_sFPEModel.Get());
							}

							//PE: Todo remove spaces at end sFile="entitybank\Purchased\Oldpman\ARTIST PACK\Scenery\block66.x                         "
							//Make sure we use full path to files.

							int iSrcFormat = 0;
							if (pestrcasestr(sFile,".x")) iSrcFormat = 1;
							if (pestrcasestr(sFile, ".fbx")) iSrcFormat = 2;
							if (iSrcFormat > 0)
							{
								char sDboFile[MAX_PATH];
								if (iSrcFormat == 1)
								{
									// X File Format
									strcpy(sDboFile, sFile);
									sDboFile[strlen(sDboFile) - 2] = 0;
									strcat(sDboFile, ".dbo");
									GG_GetRealPath(sDboFile, false);
									if (FileExist(sDboFile) == 1) strcpy(sFile,sDboFile);
								}
								if (iSrcFormat == 2)
								{
									// FBX File Format
									strcpy(sDboFile, sFile);
									sDboFile[strlen(sDboFile) - 4] = 0;
									strcat(sDboFile, ".dbo");
									GG_GetRealPath(sDboFile, false);
									if (FileExist(sDboFile) == 1) strcpy(sFile,sDboFile);
								}
							}

							GG_GetRealPath(sFile, false);
							HANDLE hfile = GG_CreateFile(sFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
							if (hfile != INVALID_HANDLE_VALUE)
							{
								item->m_sFPEDBOFile = sFile;
								item->m_iFPEDBOFileSize = GetFileSize(hfile, NULL);
								#ifdef INCLUDEPOLYGONSORT
								if (ischaracter)
								{
									if (item->m_iFPEDBOFileSize > 14000000) // zoombies are really large.
										item->m_iFPEDBOFileSize = 4500*9+ g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 7000000) // uber soldier is large.
										item->m_iFPEDBOFileSize = 17400 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 4000000) // Shotgun soldier.dbo
										item->m_iFPEDBOFileSize = 14500 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 3000000) // masked soldier.dbo
										item->m_iFPEDBOFileSize = 17400 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 1800000) // charactercreatorplus
										item->m_iFPEDBOFileSize = 20000 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 900000) // wizard
										item->m_iFPEDBOFileSize = 10000 * 9 + g_iScannedFiles;
									else //Chars have a large amount of animations, so... This is REALLY not precise.
										item->m_iFPEDBOFileSize = item->m_iFPEDBOFileSize & 0x1ffff;

								}
								else
								{
									if (isAnimated)
									{
										//Cant trust file when animated. This is REALLY not precise.
										item->m_iFPEDBOFileSize = item->m_iFPEDBOFileSize & 0x1ffff;
									}
								}
								#endif

								CloseHandle(hfile);
							}
						}
					}
					#endif

					item->m_bFPELoaded = true; //Mark that its now safe to use all FPE settings from other threads.
				}

				fpe_vector.clear();
			}
		}
	}
	g_ScanFpeFiles.clear();
	g_bFpeScanning = false;
}

void fpe_thread_start(void)
{
//	g_pFPEScan = new std::thread(fpe_thread_function, std::ref(g_object_list));
	g_pFPEScan = new std::thread(fpe_thread_function);
}
bool fpe_thread_in_progress(void)
{
	return g_bFpeScanning;
}


//std::vector<std::string> duplicate_files_check;
//PE: Converted to unordered_set using hash for faster lookups.
std::unordered_set<std::string> duplicate_files_check;

void CustomSortFiles(int iSortBy, cFolderItem::sFolderFiles * m_pFileSortStart)
{
	if (!m_pFileSortStart) return;
	std::vector<std::string> sorted_files;

	cFolderItem::sFolderFiles * m_pFiles = m_pFileSortStart;
	//PE: Make a list here thats sorted by file time.
	m_pFiles->bSorted = false;
	if (m_pFiles) m_pFiles = m_pFiles->m_pNext; //PE: Skip "..." entry.

	cFolderItem::sFolderFiles * m_pFileSort = m_pFiles;
	cFolderItem::sFolderFiles * m_pScanSort = m_pFiles;
	while (m_pFileSort)
	{
		m_pFileSort->bSorted = false;
		if (iSortBy == 1)
		{
			cstr sort_s = m_pFileSort->m_sNameFinal;
			sorted_files.push_back(sort_s.Get());
		}
		m_pFileSort = m_pFileSort->m_pNext;
	}

	m_pFileSort = m_pFileSortStart;
	cFolderItem::sFolderFiles * m_pSortBest = NULL;
	cFolderItem::sFolderFiles * m_pPrevBest = NULL;

	while (m_pFileSort)
	{
		bool bFoundBest = false;
		time_t m_tFileModifySort = 0;
		m_pScanSort = m_pFiles;
		while (m_pScanSort)
		{
			if (!m_pScanSort->bSorted)
			{
				if (iSortBy == 0)
				{
					if (m_pScanSort->m_tFileModify >= m_tFileModifySort)
					{
						m_tFileModifySort = m_pScanSort->m_tFileModify;
						m_pSortBest = m_pScanSort;
						bFoundBest = true;
					}
				}
				else
				{
					//Default just accept all.
					m_pSortBest = m_pScanSort;
					bFoundBest = true;
				}
			}
			m_pScanSort = m_pScanSort->m_pNext;
		}

		if (bFoundBest) {
			if (iSortBy == 0)
			{
				if (m_pPrevBest)
				{
					m_pPrevBest->m_pNextTime = m_pSortBest;
				}
				else
				{
					m_pFileSort->m_pNextTime = m_pSortBest;
				}
				m_pPrevBest = m_pSortBest;
				m_pSortBest->m_pNextTime = NULL;
				//printf("%s", m_pSortBest->m_sName);
				m_pSortBest->bSorted = true;
			}
			else
			{
				if (m_pPrevBest)
				{
					m_pPrevBest->m_pCustomSort = m_pSortBest;
				}
				else
				{
					m_pFileSort->m_pCustomSort = m_pSortBest;
				}
				m_pPrevBest = m_pSortBest;
				m_pSortBest->m_pCustomSort = NULL;
				//printf("%s", m_pSortBest->m_sName);
				m_pSortBest->bSorted = true;

				//Default just add.
			}
		}
		else {
			if (iSortBy == 0)
			{
				if (m_pPrevBest)
					m_pPrevBest->m_pNextTime = NULL;
			}
			else
			{
				if (m_pPrevBest)
					m_pPrevBest->m_pCustomSort = NULL;
			}
		}

		m_pFileSort = m_pFileSort->m_pNext;
	}

	sorted_files.clear();
	//PE: DEBUG ONLY
	/*
	if (m_pSortBest->m_pNextTime != NULL)
	{
		printf("err:");
	}
	m_pFileSort = pNewFolder->m_pNext->m_pFirstFile;; //TEST
	while (m_pFileSort)
	{
		printf("%s", m_pFileSort->m_sName);
		m_pFileSort = m_pFileSort->m_pNextTime;
	}
	*/

}

void SetAvailableInFreeTrial(int foldertype, cFolderItem::sFolderFiles* pNewItem, cstr file)
{
	if (foldertype == 0)
	{
		// exclude all objects except those in files_availableinfreetrial list
		std::vector<std::string>::iterator itf = files_availableinfreetrial.begin();
		for (; itf != files_availableinfreetrial.end(); ++itf)
		{
			if (itf->size() > 0)
			{
				if (strnicmp(itf->c_str(), file.Get(), file.Len()) == 0)
				{
					pNewItem->bAvailableInFreeTrial = true;
					break;
				}
			}
		}
	}
	else
	{
		if (foldertype == 4)
		{
			// behavior exclusions
			LPSTR pExclude = "";
			bool bBlockThisOne = false;
			LPSTR pThisFile = file.Get();
			pExclude = "jumpscare.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "bounce.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "freezeplayer.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "heal.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "hurt.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "slip.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "teleport.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "collection count.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "dynamite.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "seecam.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "spiketrap.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "trapdoor.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			if (bBlockThisOne == true)
			{
				pNewItem->bAvailableInFreeTrial = false;
			}
			else
			{
				pNewItem->bAvailableInFreeTrial = true;
			}
		}
		else
		{
			// allow all
			pNewItem->bAvailableInFreeTrial = true;
		}
	}
}

cstr GetNameFinalCreditFromAbsPath (LPSTR pAbsFullFolderPath)
{
	cstr sNameFinalCredit = "";
	#ifndef GGMAXEDU
	LPSTR pFoundMatchPtr = strstr(pAbsFullFolderPath, "\\Community\\");
	if (pFoundMatchPtr == NULL) pFoundMatchPtr = strstr(pAbsFullFolderPath, "\\community\\");
	if (pFoundMatchPtr != NULL)
	{
		// we have a community asset here, credit it!
		char pAccountID[256];
		strcpy(pAccountID, pFoundMatchPtr + strlen("\\community\\"));
		for (int n = 0; n < strlen(pAccountID); n++)
		{
			if (pAccountID[n] == '\\' || pAccountID[n] == '/')
			{
				pAccountID[n] = 0;
				break;
			}
		}
		if (strlen(pAccountID) > 0)
		{
			extern std::vector<sWorkshopSteamUserName> g_workshopSteamUserNames;
			for (int j = 0; j < g_workshopSteamUserNames.size(); j++)
			{
				if (stricmp(g_workshopSteamUserNames[j].sSteamUserAccountID.Get(), pAccountID) == NULL)
				{
					strcpy(pAccountID, g_workshopSteamUserNames[j].sSteamUsersPersonaName.Get());
					break;
				}
			}
			sNameFinalCredit = cstr(" (by ") + cstr(pAccountID) + cstr(")");
		}
	}
	#endif
	return sNameFinalCredit;
}

extern char szBeforeChangeWriteDir[MAX_PATH];

void GetMainEntityList(char* folder_s, char* rel_s, void *pFolder, char* folder_name_start, bool bForceToTop, int foldertype)
{
	//foldertype: 0 fpe , 1 waw mp3 ogg. , 2 BMP, JPG, PNG, TIF, and GIF
	// get legacy white list once
	if (g_bCreateLegacyWhiteList == true)
	{
		getVectorFileContent("favoritelist.ini", files_favorite);
		extern bool g_bFreeTrialVersion;
		if (g_bFreeTrialVersion == true)
		{
			getVectorFileContent("freetriallist.ini", files_availableinfreetrial);
		}

		g_pLegacyWhiteList.clear();
		FILE* fp = GG_fopen(".\\..\\legacyblacklist.ini", "rt");
		if (fp)
		{
			char c;
			fread(&c, sizeof(char), 1, fp);
			while (!feof(fp))
			{
				// get string from file
				char szEntNameFromFile[MAX_PATH] = "";
				int iOffset = 0;
				while (!feof(fp) && c != 13 && c != 10)
				{
					szEntNameFromFile[iOffset++] = c;
					fread(&c, sizeof(char), 1, fp);
				}
				szEntNameFromFile[iOffset] = 0;

				// skip beyond CR
				while (!feof(fp) && (c == 13 || c == 10))
					fread(&c, sizeof(char), 1, fp);

				// write into array
				LPSTR pWhiteListItem = new char[512];
				strlwr(szEntNameFromFile);
				strcpy(pWhiteListItem, szEntNameFromFile);
				g_pLegacyWhiteList.push_back(pWhiteListItem);
			}
			fclose(fp);
		}
		g_bCreateLegacyWhiteList = false;
	}

	// get full entity list
	int tt = 0;
	cstr file_s = "";
	int fin = 0;
	cstr tempcstr;
	cFolderItem *pNewFolder = (cFolderItem *) pFolder;
	if (pNewFolder == NULL)
	{
		pNewFolder = &MainEntityList;
		iTotalFolders = 0;
		iTotalFiles = 0;
	}
	while (pNewFolder->m_pNext) 
	{
		pNewFolder = pNewFolder->m_pNext;
	}

	if (PathExist(folder_s) == 1)
	{
		SetDir(folder_s);

		//Create FolderItem.
		cFolderItem *pNewItem;
		pNewItem = new cFolderItem();
		pNewItem->m_sFolder = folder_s;

		cstr pOld = GetDir();
		pNewItem->m_sFolderFullPath = pOld;
		pNewItem->iType = foldertype;

		//PE: Added here for speed.
		pNewItem->m_iEntityOffset = pNewItem->m_sFolderFullPath.Len();
		cStr path = pNewItem->m_sFolderFullPath.Get();
		LPSTR pPathSearch = path.Get();
		LPSTR pFind = "\\entitybank";
		if(foldertype == 1)
			pFind = "\\audiobank";
		if (foldertype == 2)
			pFind = "\\imagebank";
		if (foldertype == 3)
			pFind = "\\videobank";
		if (foldertype == 4)
			pFind = "\\scriptbank";
		if (foldertype == 5)
			pFind = "\\particlesbank";
		if (foldertype == 6)
			pFind = "\\charactercreatorplus\\animations";
		
		for (int n = 0; n < strlen(pPathSearch); n++)
		{
			if (strnicmp(pPathSearch + n, pFind, strlen(pFind)) == NULL)
			{
				pNewItem->m_iEntityOffset = n + strlen(pFind);
				break;
			}

			// project folder passed in override specific detection above
		}

		strcpy(pNewItem->cfolder, pNewItem->m_sFolderFullPath.Get() );

		// ensures writable folder sorts all its folders to top
		if ( bForceToTop == true )
		{
			// ensure when sorted, these folders stay at the very top for easy discovery
			pNewItem->cfolder[0] = 'A';
		}

		pNewItem->m_pSubFolder = NULL;
		pNewItem->visible = true;
		pNewItem->deletethisentry = false;
		pNewItem->count = ++iTotalFolders;
		pNewItem->m_pFirstFile = NULL;
		pNewItem->bIsCustomFolder = false;
		pNewItem->m_pNext = NULL;

		//Update last folder modify date time.
		struct stat sb;
		if (stat(pNewItem->m_sFolderFullPath.Get(), &sb) == 0) 
		{
			if (sb.st_mtime != pNewItem->m_tFolderModify) 
			{
				pNewItem->m_tFolderModify = sb.st_mtime;
			}
		}	
		// taken care of above with bForceToTop
		pNewFolder->m_pNext = pNewItem;

		FindFirst(); fin = 0;

		std::vector<std::string> sorted_files;
		while (GetFileType()>-1)
		{
			file_s = GetFileName();
			if (file_s == "." || file_s == "..")
			{
				//  ignore . and ..
			}
			else
			{
				if (GetFileType() == 1)
				{
					//  folder
					GetMainEntityList(file_s.Get(), cstr(cstr(rel_s) + file_s + "\\").Get(), (void *)pNewFolder->m_pNext, "", bForceToTop, foldertype);
					FindFirst();
					if (fin > 0)
					{
						for (tt = 1; tt <= fin; tt++)
						{
							if (GetFileType()>-1)
							{
								FindNext();
							}
						}
					}
				}
				else
				{
					bool bValid = false;
					if (foldertype == 0 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".wav"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".mp3"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".ogg"))
						bValid = true;

					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".png"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".dds"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".bmp"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".tif"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".jpg"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".gif"))
						bValid = true;

					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".wmv"))
						bValid = true;
					//if (foldertype == 3 && pestrcasestr(file_s.Get(), ".ogv"))
					//	bValid = true;
					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".mp4"))
						bValid = true;

					if (foldertype == 4 && pestrcasestr(file_s.Get(), ".lua"))
						bValid = true;

					if (foldertype == 5 && pestrcasestr(file_s.Get(), ".arx"))
						bValid = true;

					if (foldertype == 6 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;

					// file
					if(bValid)
					{
						// compare entity with legacy whitelist, and skip if on list (slowly hiding older models)
						cstr pComboRelandFile = cstr(rel_s) + file_s;

						//if (strnicmp(rel_s, "user\\lee", 8) == NULL)
						//{
						//	pComboRelandFile = cstr(rel_s) + file_s;
						//}

						LPSTR pFilePtr = pComboRelandFile.Get();
						int iLegacyWhiteListCount = g_pLegacyWhiteList.size();
						int n = 0;
						for (; n < iLegacyWhiteListCount; n++)
						{
							if (stricmp(pFilePtr, g_pLegacyWhiteList[n]) == NULL)
								break;
						}
						if (n == iLegacyWhiteListCount)
						{
							// not found in legacy whitelist, allow
							sorted_files.push_back(file_s.Get());
							
							//SAVE: time_create
							time_t ts = GetFileDateLong();
							cstr file = pNewFolder->m_pNext->m_sFolderFullPath;
							file = file + "\\" + file_s;
							//files_time_stamp.push_back( std::make_pair(file.Get(), ts ) );
							files_time_stamp[file.Get()] = ts;
						}
					}
				}
			}
			FindNext();
			fin = fin + 1;
		}

		//sorted_files
		cFolderItem::sFolderFiles * m_pFiles = NULL;
		if (!sorted_files.empty()) 
		{
			std::sort(sorted_files.begin(), sorted_files.end(), NoCaseLess);
			std::vector<std::string>::iterator it = sorted_files.begin();
			if (it->size() > 0) 
			{
				cFolderItem::sFolderFiles *pNewItem = new cFolderItem::sFolderFiles;
				pNewItem->m_sName = "...";
				pNewItem->m_sNameFinal = pNewItem->m_sName;
				pNewItem->m_sNameFinalCredit = "";
				pNewItem->m_tFileModify = 0;
				pNewItem->m_Backdrop = "";
				pNewItem->bFavorite = false;
				pNewItem->bAvailableInFreeTrial = false;
				pNewItem->m_sPath = "";
				pNewItem->m_sFolder = "[na]";
				pNewItem->iFlags = 0;
				pNewItem->iPreview = 0;
				pNewItem->iBigPreview = 0;
				pNewItem->id = iTotalFiles++;
				pNewItem->bPreviewProcessed = false;
				pNewItem->m_pNext = NULL;
				pNewItem->m_pNextTime = NULL;
				pNewItem->m_pCustomSort = NULL;

				pNewItem->m_bFPELoaded = false;
				pNewItem->m_sFPEModel="";
				pNewItem->m_sFPETextured = "";
				pNewItem->m_sFPEKeywords = "";
				pNewItem->m_sFPEDBOFile = "";
				pNewItem->m_sDLuaDescription = "##na##";
				pNewItem->m_fDLuaHeight = 0.0f;
				pNewItem->m_iFPEDBOFileSize = 0;
				pNewItem->m_bIsCharacterCreator = 0;
				pNewItem->iType = foldertype;

				pNewFolder->m_pNext->m_pFirstFile = pNewItem;
				m_pFiles = pNewItem;
			}
			for (; it != sorted_files.end(); ++it) 
			{
				bool bAddToList = true;
				//PE: Eliminate any duplicates here, DocWrite and Normal Path, Prefer DocWrite.
				if (it->size() > 0) {
					cStr sName = (char *) it->c_str();
					cStr m_sPath = pNewFolder->m_pNext->m_sFolderFullPath;
					cStr sCheck = m_sPath + cstr("\\") + sName;
					char *find = NULL;
					if(foldertype == 0)
						find = (char *) pestrcasestr(sCheck.Get(),"entitybank\\");
					if (foldertype == 1)
						find = (char *)pestrcasestr(sCheck.Get(), "audiobank\\");
					if (foldertype == 2)
						find = (char *)pestrcasestr(sCheck.Get(), "imagebank\\");
					if (foldertype == 3)
						find = (char *)pestrcasestr(sCheck.Get(), "videobank\\");
					if (foldertype == 4)
						find = (char *)pestrcasestr(sCheck.Get(), "scriptbank\\");
					if (foldertype == 5)
						find = (char *)pestrcasestr(sCheck.Get(), "particlesbank\\");
					if (foldertype == 6)
						find = (char *)pestrcasestr(sCheck.Get(), "charactercreatorplus\\animations\\");
					if (find)
					{
						// ZJ: Got heap corruption error here.
						//sCheck = find;
						cStr sFind(find);
						strcpy(sCheck.Get(), sFind.Get());
						//sCheck = sCheck + cstr("\\") + sName;
					}
					sCheck = sCheck.Lower();
					//auto itr = std::find(duplicate_files_check.begin(), duplicate_files_check.end(), sCheck.Get());
					//if (itr != duplicate_files_check.end() && duplicate_files_check.size() > 0 )
					//	bAddToList = false;
					//else
					//	duplicate_files_check.push_back(sCheck.Get());
					
					if (duplicate_files_check.count(sCheck.Get()) > 0)
						bAddToList = false;
					else
						duplicate_files_check.insert(sCheck.Get());
				}
				if (it->size() > 0 && bAddToList) 
				{
					cFolderItem::sFolderFiles *pNewItem;
					pNewItem = new cFolderItem::sFolderFiles;

					pNewItem->m_sName = it->c_str();
					if ((foldertype == 0 || foldertype == 6) && pNewItem->m_sName.Len() > 4)
						pNewItem->m_sNameFinal = pNewItem->m_sName.Left(pNewItem->m_sName.Len() - 5);
					else
						pNewItem->m_sNameFinal = pNewItem->m_sName;

					// credit if community asset
					pNewItem->m_sNameFinalCredit = GetNameFinalCreditFromAbsPath (pNewFolder->m_pNext->m_sFolderFullPath.Get());

					//Generate a better search string. include category at end.
					std::string sBetterSearch = pNewItem->m_sNameFinal.Get();
					sBetterSearch = sBetterSearch + " ( " + pNewFolder->m_pNext->m_sFolder.Get() + " )";
					//Remove main folder for better search.
					if (foldertype == 1)
						replaceAll(sBetterSearch, "audiobank", "");
					if (foldertype == 2)
						replaceAll(sBetterSearch, "imagebank", "");
					if (foldertype == 3)
						replaceAll(sBetterSearch, "videobank", "");
					if (foldertype == 4)
						replaceAll(sBetterSearch, "scriptbank", "");
					if (foldertype == 5)
						replaceAll(sBetterSearch, "particlesbank", "");
					if (foldertype == 6)
						replaceAll(sBetterSearch, "charactercreatorplus\\animations", "");
					replaceAll(sBetterSearch, "_", " ");
					replaceAll(sBetterSearch, "-", " ");
					pNewItem->m_sBetterSearch = sBetterSearch.c_str();

					pNewItem->m_tFileModify = 0; //PE: Need timestamp here.
					pNewItem->bFavorite = false;
					pNewItem->bAvailableInFreeTrial = false;

					//PE: This was to slow.
					struct stat sb;
					cstr file = pNewFolder->m_pNext->m_sFolderFullPath;
					file = file + "\\" + pNewItem->m_sName;
					//std::vector< std::pair<std::string, time_t> >::iterator its = files_time_stamp.begin();
					//for (; its != files_time_stamp.end(); ++its)
					//{
					//	if (its->first.size() > 0)
					//	{
					//		if( strcmp(its->first.c_str(),file.Get()) == 0 )
					//		{
					//			pNewItem->m_tFileModify = its->second;
					//			break;
					//		}
					//	}
					//}

					auto it = files_time_stamp.find(file.Get());
					if (it != files_time_stamp.end()) {
						pNewItem->m_tFileModify = it->second;
					}
					std::vector<std::string>::iterator itf = files_favorite.begin();
					for (; itf != files_favorite.end(); ++itf)
					{
						if (itf->size() > 0)
						{
							if (strnicmp(itf->c_str(), file.Get(), file.Len() ) == 0)
							{
								pNewItem->bFavorite = true;
								break;
							}
						}
					}
					extern bool g_bFreeTrialVersion;
					if (g_bFreeTrialVersion == true)
					{
						SetAvailableInFreeTrial(foldertype, pNewItem, file);
					}

					pNewItem->m_sPath = pNewFolder->m_pNext->m_sFolderFullPath;
					pNewItem->m_sFolder = pNewFolder->m_pNext->m_sFolder;
					pNewItem->m_Backdrop = "";
					pNewItem->iFlags = 0;
					pNewItem->iPreview = 0;
					pNewItem->iBigPreview = 0;
					pNewItem->id = iTotalFiles++;
					pNewItem->bPreviewProcessed = false;
					pNewItem->m_pNext = NULL;
					pNewItem->m_pNextTime = NULL;
					pNewItem->m_pCustomSort = NULL;

					pNewItem->m_bFPELoaded = false;
					pNewItem->m_sFPEModel = "";
					pNewItem->m_sFPETextured = "";
					pNewItem->m_sFPEKeywords = "";
					pNewItem->m_sFPEDBOFile = "";
					pNewItem->m_sDLuaDescription = "##na##";
					pNewItem->m_fDLuaHeight = 0.0f;
					pNewItem->m_iFPEDBOFileSize = 0;
					pNewItem->m_bIsCharacterCreator = 0;
					pNewItem->iType = foldertype;

					m_pFiles->m_pNext = pNewItem;
					m_pFiles->m_pNextTime = pNewItem;
					m_pFiles->m_pCustomSort = pNewItem;
					m_pFiles = pNewItem;
				}
			}
			sorted_files.clear();

			CustomSortFiles(0, pNewFolder->m_pNext->m_pFirstFile);
		}
		SetDir("..");
	}
}

