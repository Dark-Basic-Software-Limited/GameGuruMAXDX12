bool Graphics_Performance_Settings(float fTabColumnWidth, bool bVisualUpdated)
{
	int wflags = ImGuiTreeNodeFlags_None;
	static bool bOCDebug = false;
	static bool bBoxDebug = false;
	static int iHiddenObjects = 0;
	static int iObjects = 0;
	static int iFrustumCulled = 0;
	int iSpot = 0, iPoint = 0;
	int occ = 0;
	if (bOCDebug && g_iDevToolsOpen >= 1)
	{
		int DrawOccludedObjects(bool bDebug, bool bBox = false, int* bHiddenObjects = nullptr, int* spot = nullptr, int* point = nullptr);
		occ = DrawOccludedObjects(bOCDebug, bBoxDebug, &iHiddenObjects, &iSpot, &iPoint);
	}

	if (pref.bAutoClosePropertySections && iLastOpenHeader != 16)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	//PE: Optimizing
	if (ImGui::StyleCollapsingHeader("Graphics and Performance", wflags))
	{
		ImGui::Indent(10);
		iLastOpenHeader = 16;

		// graphics options mode
		ImGui::PushItemWidth(-10);
		char* current_gfx_mode = "";
		int iGFXMode = iGFXMode = t.visuals.shaderlevels.entities - 1;
		const char* gfx_mode_combo[] = { "Highest (best for quality)", "Custom (tailored)" , "Low (best for performance)" };
		if (iGFXMode == 0) current_gfx_mode = (char*)gfx_mode_combo[0];
		if (iGFXMode == 1) current_gfx_mode = (char*)gfx_mode_combo[1];
		if (iGFXMode == 2) current_gfx_mode = (char*)gfx_mode_combo[2];
		if (ImGui::Combo("##ComboGFX_mode_combo", &iGFXMode, gfx_mode_combo, IM_ARRAYSIZE(gfx_mode_combo)))
		{
			if (iGFXMode == 0) visuals_shaderlevels_setlevel(1, true);
			if (iGFXMode == 1) visuals_shaderlevels_setlevel(3, true);
			if (iGFXMode == 2) visuals_shaderlevels_setlevel(4, true);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Choose the ideal graphics mode for this level, or set custom settings");
		ImGui::PopItemWidth();

		// optimizations
		extern bool bEnable30FpsAnimations;
		extern bool g_bDelayedShadows;
		extern bool g_bDelayedShadowsLaptop;
		ImGui::PushItemWidth(-10);
		if (ImGui::Checkbox("Lower Animation & LUA Speed##Animationsculling", &bEnable30FpsAnimations))
		{
			t.gamevisuals.bEnable30FpsAnimations = t.visuals.bEnable30FpsAnimations = bEnable30FpsAnimations;
			g.projectmodified = 1;

		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enabling Lower Animation Speed will lower the updating of animation & LUA to 30 FPS for increased speed when using many animations.");
		ImGui::PopItemWidth();

		extern bool bEnableDelayPointShadow;
		extern float pointShadowScaler;

		ImGui::PushItemWidth(-10);
		if (ImGui::Checkbox("Delayed Shadows##Animationsculling", &g_bDelayedShadows))
		{
			t.gamevisuals.g_bDelayedShadows = t.visuals.g_bDelayedShadows = g_bDelayedShadows;
			g.projectmodified = 1;
			// DX12: actually drive the engine's directional cascade staggering (delta 1.11). OFF =
			// every-frame cascades = no shadow-lag flicker under camera movement. The point-shadow
			// booleans below are the legacy DX11 path and do NOT touch the directional cascades.
			wiRenderer::SetDelayedShadowCascadesEnabled(g_bDelayedShadows);
			if (g_bDelayedShadows && g_bDelayedShadowsLaptop)
			{
				bEnableDelayPointShadow = true;
				pointShadowScaler = 0.6f;
			}
			else if (g_bDelayedShadows)
			{
				bEnableDelayPointShadow = true;
				pointShadowScaler = 1.0f;
			}
			else
			{
				bEnableDelayPointShadow = false;
				pointShadowScaler = 1.0f;
			}
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enabling Delayed Shadows will make fewer cascade shadow updates and increase your FPS.");
		if (g_bDelayedShadows)
		{
			ImGui::SameLine();
			if (ImGui::Checkbox("Laptop##Animationsculling", &g_bDelayedShadowsLaptop))
			{
				t.gamevisuals.g_bDelayedShadowsLaptop = t.visuals.g_bDelayedShadowsLaptop = g_bDelayedShadowsLaptop;
				g.projectmodified = 1;
				if (g_bDelayedShadows && g_bDelayedShadowsLaptop)
				{
					bEnableDelayPointShadow = true;
					pointShadowScaler = 0.6f;
				}
				else if (g_bDelayedShadows)
				{
					bEnableDelayPointShadow = true;
					pointShadowScaler = 1.0f;
				}
				else
				{
					bEnableDelayPointShadow = false;
					pointShadowScaler = 1.0f;
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enabling Delayed Shadows (Laptop) will make even fever cascade shadow updates and increase your FPS.");
		}
		ImGui::PopItemWidth();

		extern bool bEnableObjectCulling;
		ImGui::PushItemWidth(-10);
		if (ImGui::Checkbox("Occlusion Culling##bOcclusionCulling", &t.visuals.bOcclusionCulling))
		{
			t.gamevisuals.bOcclusionCulling = t.visuals.bOcclusionCulling;
			g.projectmodified = 1;
			if (t.visuals.bOcclusionCulling)
			{
				bEnableObjectCulling = true;
				t.gamevisuals.bEnableObjectCulling = t.visuals.bEnableObjectCulling = bEnableObjectCulling;
			}
		}
		if (wiRenderer::GetOcclusionCullingEnabled() != t.visuals.bOcclusionCulling)
		{
			wiRenderer::SetOcclusionCullingEnabled(t.visuals.bOcclusionCulling);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enabling Occlusion Culling will cull objects behind other object for less drawcalls");
		ImGui::PopItemWidth();

		if (g_iDevToolsOpen >= 1)
		{
			ImGui::SameLine();
			ImGui::Checkbox("Debug", &bOCDebug);
			ImGui::Checkbox("Debug Bounding Box", &bBoxDebug);
		}
		if (t.visuals.bOcclusionCulling)
		{
			extern uint32_t iCulledPointShadows;
			extern uint32_t iCulledSpotShadows;
			extern uint32_t iCulledAnimations;
			extern bool bEnableTerrainChunkCulling;
			extern bool bEnablePointShadowCulling;
			extern bool bEnableSpotShadowCulling;
			extern bool bEnableAnimationCulling;

			if (ImGui::Checkbox("Terrain Chunk Culling", &bEnableTerrainChunkCulling))
			{
				t.gamevisuals.bEnableTerrainChunkCulling = t.visuals.bEnableTerrainChunkCulling = bEnableTerrainChunkCulling;
				g.projectmodified = 1;
				if (bEnableTerrainChunkCulling && !t.visuals.bOcclusionCulling)
				{
					t.gamevisuals.bOcclusionCulling = t.visuals.bOcclusionCulling = true;
				}
			}
			if (ImGui::Checkbox("Point Shadow Culling", &bEnablePointShadowCulling))
			{
				t.gamevisuals.bEnablePointShadowCulling = t.visuals.bEnablePointShadowCulling = bEnablePointShadowCulling;
				g.projectmodified = 1;
				if (bEnablePointShadowCulling && !t.visuals.bOcclusionCulling)
				{
					t.gamevisuals.bOcclusionCulling = t.visuals.bOcclusionCulling = true;
				}
			}

			if (ImGui::Checkbox("Spot Shadow Culling", &bEnableSpotShadowCulling))
			{
				t.gamevisuals.bEnableSpotShadowCulling = t.visuals.bEnableSpotShadowCulling = bEnableSpotShadowCulling;
				g.projectmodified = 1;
				if (bEnableSpotShadowCulling && !t.visuals.bOcclusionCulling)
				{
					t.gamevisuals.bOcclusionCulling = t.visuals.bOcclusionCulling = true;
				}
			}

			if (ImGui::Checkbox("Object Culling", &bEnableObjectCulling))
			{
				t.gamevisuals.bEnableObjectCulling = t.visuals.bEnableObjectCulling = bEnableObjectCulling;
				g.projectmodified = 1;
				if (bEnableObjectCulling && !t.visuals.bOcclusionCulling)
				{
					t.gamevisuals.bOcclusionCulling = t.visuals.bOcclusionCulling = true;
				}
			}

			if (ImGui::Checkbox("Animation Culling", &bEnableAnimationCulling))
			{
				t.gamevisuals.bEnableAnimationCulling = t.visuals.bEnableAnimationCulling = bEnableAnimationCulling;
				g.projectmodified = 1;
				if (bEnableAnimationCulling && !t.visuals.bOcclusionCulling)
				{
					t.gamevisuals.bOcclusionCulling = t.visuals.bOcclusionCulling = true;
				}
			}

			wiScene::Scene* pScene = &wiScene::GetScene();
			if (pScene)
			{
				iObjects = pScene->objects.GetCount();
				iFrustumCulled = 0;
			}
			if (bOCDebug)
				ImGui::Text("Total Objects: %d Hidden: %d", iObjects, iHiddenObjects);
			else
				ImGui::Text("Total Objects: %d", iObjects);
			ImGui::Text("Frustum/Apparent Culled: %d", iFrustumCulled);
			if (bOCDebug)
				ImGui::Text("Occluded Objects: %d", occ);

			extern uint32_t iOccludedTerrainChunks;
			if (bOCDebug)
				ImGui::Text("Occluded Terrain chunks: %d", iOccludedTerrainChunks);

			extern uint32_t iRenderedPointShadows;
			extern uint32_t iRenderedSpotShadows;

			if (bOCDebug)
				ImGui::Text("Occluded Point Shadows: (%d) %d r(%d)", iPoint, iCulledPointShadows, iRenderedPointShadows);
			else
				ImGui::Text("Occluded Point Shadows: %d r(%d)", iCulledPointShadows, iRenderedPointShadows);
			if (bOCDebug)
				ImGui::Text("Occluded Spot Shadows: (%d) %d r(%d)", iSpot, iCulledSpotShadows, iRenderedSpotShadows);
			else
				ImGui::Text("Occluded Spot Shadows: %d r(%d)", iCulledSpotShadows, iRenderedSpotShadows);

			if (bOCDebug)
				ImGui::Text("Culled Animations: %d", iCulledAnimations);
		}

		extern float maxApparentSize;
		ImGui::PushItemWidth(-10);
		float fASize = t.visuals.ApparentSize * 10000.0f;
		tab_tab_Column_text("Apparent Size", fTabColumnWidth);
		if (ImGui::SliderFloat("##maxApparentSize", &fASize, 0.02f, 2.0f, "%.2f", 1.0f))
		{
			maxApparentSize = fASize / 10000.0f;
			t.gamevisuals.ApparentSize = t.visuals.ApparentSize = maxApparentSize;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Max Object Apparent Size will cull objects when they get smaller on screen");
		ImGui::PopItemWidth();


		extern float fLODMultiplier;
		//ImGui::Text("LOD Multiplier");
		tab_tab_Column_text("LOD Multiplier", fTabColumnWidth);
		ImGui::PushItemWidth(-10);
		if (ImGui::SliderFloat("##fLODMultiplier", &fLODMultiplier, 0.0f, 15.0f, "%.2f", 1.0f))
		{
			if (fLODMultiplier < 0)
				fLODMultiplier = 0;
			t.gamevisuals.fLODMultiplier = t.visuals.fLODMultiplier = fLODMultiplier;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Change LOD distance before switching from normal object to LOD");
		ImGui::PopItemWidth();

		extern int g_iUseLODObjects;
		extern bool bDisableLODLoad;
		ImGui::Checkbox("Disable LOD Load", &bDisableLODLoad);

		if (g_iUseLODObjects > 0 && !bDisableLODLoad)
		{
			extern bool bShadowsLowestLOD;
			ImGui::PushItemWidth(-10);
			if (ImGui::Checkbox("Shadows Use Fastest LOD##Animationsculling", &bShadowsLowestLOD))
			{
				t.gamevisuals.bShadowsLowestLOD = t.visuals.bShadowsLowestLOD = bShadowsLowestLOD;
				g.projectmodified = 1;

			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("All shadows will use fastest LOD available to render.");
			ImGui::PopItemWidth();

			extern bool bProbesLowestLOD;
			ImGui::PushItemWidth(-10);
			if (ImGui::Checkbox("Probes Use Fastest LOD##Animationsculling", &bProbesLowestLOD))
			{
				t.gamevisuals.bProbesLowestLOD = t.visuals.bProbesLowestLOD = bProbesLowestLOD;
				g.projectmodified = 1;

			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("All probes will use fastest LOD available to render.");
			ImGui::PopItemWidth();

			extern bool bRaycastLowestLOD;
			ImGui::PushItemWidth(-10);
			if (ImGui::Checkbox("Raycast Use Fastest LOD##Animationsculling", &bRaycastLowestLOD))
			{
				t.gamevisuals.bRaycastLowestLOD = t.visuals.bRaycastLowestLOD = bRaycastLowestLOD;
				g.projectmodified = 1;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("All raycast will use fastest LOD available for intersect checks.");
			ImGui::PopItemWidth();


			extern bool bPhysicsLowestLOD;
			ImGui::PushItemWidth(-10);
			if (ImGui::Checkbox("Physics Use Fastest LOD##Animationsculling", &bPhysicsLowestLOD))
			{
				t.gamevisuals.bPhysicsLowestLOD = t.visuals.bPhysicsLowestLOD = bPhysicsLowestLOD;
				g.projectmodified = 1;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("All physics objects is created using fastest LOD.");
			ImGui::PopItemWidth();

			extern bool bReflectionsLowestLOD;
			ImGui::PushItemWidth(-10);
			if (ImGui::Checkbox("Reflections Use Fastest LOD##Animationsculling", &bReflectionsLowestLOD))
			{
				t.gamevisuals.bReflectionsLowestLOD = t.visuals.bReflectionsLowestLOD = bReflectionsLowestLOD;
				g.projectmodified = 1;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("All reflection rendering is using fastest LOD.");
			ImGui::PopItemWidth();
		}

		// FSR Mode (FidelityFX Super Resolution)
		const char* fsr_items_align[] = { "None", "Ultra Quality","Quality", "Balanced", "Performance" };
		int fsr_current_type_selection = t.visuals.iFSRMode;
		tab_tab_Column_text("FSR", fTabColumnWidth);
		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##setiFSRMode", &fsr_current_type_selection, fsr_items_align, IM_ARRAYSIZE(fsr_items_align)))
		{
			t.visuals.iFSRMode = fsr_current_type_selection;
			t.gamevisuals.iFSRMode = t.visuals.iFSRMode;
			if (t.visuals.iFSRMode == 1)
			{
				// TODO: Set3DResolution removed, use resolutionScale instead
				//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false);
				// TODO: SetFSRScale removed, use setFSR2Preset instead
				//master.masterrenderer.SetFSRScale(1.3f);
				master.masterrenderer.setFSREnabled(true);
				master.masterrenderer.ResizeBuffers(); //PE: Force resizebuffers.
				master.masterrenderer.setFSRSharpness(t.visuals.fFSRSharpness);
				t.gamevisuals.bFXAAEnabled = t.visuals.bFXAAEnabled = true; //PE: need FXAA or FSR dont work.
				if (master_renderer)
					master_renderer->setFXAAEnabled(t.visuals.bFXAAEnabled);
			}
			else if (t.visuals.iFSRMode == 2)
			{
				// TODO: Set3DResolution removed, use resolutionScale instead
				//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false);
				// TODO: SetFSRScale removed, use setFSR2Preset instead
				//master.masterrenderer.SetFSRScale(1.5f);
				master.masterrenderer.setFSREnabled(true);
				master.masterrenderer.ResizeBuffers(); //PE: Force resizebuffers.
				master.masterrenderer.setFSRSharpness(t.visuals.fFSRSharpness);
				t.gamevisuals.bFXAAEnabled = t.visuals.bFXAAEnabled = true; //PE: need FXAA or FSR dont work.
				if (master_renderer)
					master_renderer->setFXAAEnabled(t.visuals.bFXAAEnabled);
			}
			else if (t.visuals.iFSRMode == 3)
			{
				// TODO: Set3DResolution removed, use resolutionScale instead
				//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false);
				// TODO: SetFSRScale removed, use setFSR2Preset instead
				//master.masterrenderer.SetFSRScale(1.7f);
				master.masterrenderer.setFSREnabled(true);
				master.masterrenderer.ResizeBuffers(); //PE: Force resizebuffers.
				master.masterrenderer.setFSRSharpness(t.visuals.fFSRSharpness);
				t.gamevisuals.bFXAAEnabled = t.visuals.bFXAAEnabled = true; //PE: need FXAA or FSR dont work.
				if (master_renderer)
					master_renderer->setFXAAEnabled(t.visuals.bFXAAEnabled);
			}
			else if (t.visuals.iFSRMode == 4)
			{
				// TODO: Set3DResolution removed, use resolutionScale instead
				//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false);
				// TODO: SetFSRScale removed, use setFSR2Preset instead
				//master.masterrenderer.SetFSRScale(2.0f);
				master.masterrenderer.setFSREnabled(true);
				master.masterrenderer.ResizeBuffers(); //PE: Force resizebuffers.
				master.masterrenderer.setFSRSharpness(t.visuals.fFSRSharpness);
				t.gamevisuals.bFXAAEnabled = t.visuals.bFXAAEnabled = true; //PE: need FXAA or FSR dont work.
				if (master_renderer)
					master_renderer->setFXAAEnabled(t.visuals.bFXAAEnabled);
			}
			else
			{
				//PE: Disable FSR
				// TODO: Set3DResolution removed, use resolutionScale instead
				//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false);
				// TODO: SetFSRScale removed, use setFSR2Preset instead
				//master.masterrenderer.SetFSRScale(1.0f);
				master.masterrenderer.setFSREnabled(false);
				master.masterrenderer.ResizeBuffers(); //PE: Force resizebuffers.
			}

			//PE: change.
			g.projectmodified = 1;
		}

		ImGui::PopItemWidth();

		if (master.masterrenderer.getFSREnabled())
		{
			ImGui::Text("FSR Sharpness");
			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fFSRSharpness", &t.visuals.fFSRSharpness, 0.0f, 2.0f, "%.2f", 1.0f))
			{
				if (t.visuals.fFSRSharpness < 0)
					t.visuals.fFSRSharpness = 0;
				t.gamevisuals.fFSRSharpness = t.visuals.fFSRSharpness;
				master.masterrenderer.setFSRSharpness(t.visuals.fFSRSharpness);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Change FSR Sharpness");
			ImGui::PopItemWidth();
		}

		// end performance
		ImGui::Indent(-10);
	}
	return(bVisualUpdated);
}

bool AI_Management_Settings(float fTabColumnWidth, bool bVisualUpdated)
{
	int wflags = ImGuiTreeNodeFlags_None;
	float w = ImGui::GetWindowContentRegionWidth();

	if (pref.bAutoClosePropertySections && iLastOpenHeader != 12)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	if (ImGui::StyleCollapsingHeader("AI Management", wflags))
	{
		iLastOpenHeader = 12;
		ImGui::Indent(10);
		ImGui::PushItemWidth(-10);
		extern bool g_bShowRecastDetourDebugVisuals;
		if (ImGui::Checkbox("Show Navigation Debug Visuals", &g_bShowRecastDetourDebugVisuals))
		{
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle whether the navigation system debug visuals should be shown");
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(-10);

		extern bool g_bResetHasForLevelGeneration;
		if (ImGui::Checkbox("Disable Navmesh Generation", &t.visuals.bEnableZeroNavMeshMode))
		{
			t.gamevisuals.bEnableZeroNavMeshMode = t.visuals.bEnableZeroNavMeshMode;
			t.editorvisuals.bEnableZeroNavMeshMode = t.visuals.bEnableZeroNavMeshMode;
			g_bResetHasForLevelGeneration = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle navmesh generation (enables system to detecting walkable areas) (needs to rebuild level)");
		ImGui::PopItemWidth();

		//PE: Buttons can only be used in test game Tab Tab.
		if (bRenderTabTab)
		{
			float but_gadget_size = ImGui::GetFontSize() * 10.0;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));
			if (ImGui::StyleButton("Edit Behaviors##TabTabEditBehaviors", ImVec2(but_gadget_size, 0)))
			{
				extern bool g_bBehaviorEditorActive;
				g_bBehaviorEditorActive = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open the Behavior Editor to debug and edit logic live");

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));
			if (ImGui::StyleButton("View Slowest Logic##TabTabEditBehaviors", ImVec2(but_gadget_size, 0)))
			{
				extern int g_iViewPerformanceTimers;
				g_iViewPerformanceTimers = 1;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Run a live snapshot of the ten slowest behaviours currently running");
		}

		ImGui::Indent(-10);
	}

	return(bVisualUpdated);
}
bool Global_Behaviors_Settings(float fTabColumnWidth, bool bVisualUpdated)
{
	int wflags = ImGuiTreeNodeFlags_None;

	if (pref.bAutoClosePropertySections && iLastOpenHeader != 35)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	if (ImGui::StyleCollapsingHeader("Global Behaviors", wflags))
	{
		static int iSelectecElement = -1;
		iLastOpenHeader = 35;

		ImGui::Indent(10);
		float buttonwide = ImGui::GetContentRegionAvail().x * 0.5 - 10.0f;
		static std::string myscript = "";
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (iSelectedLibraryStingReturnID == window->GetID("ScriptSelector##+"))
		{
			//Update Script.
			if (sSelectedLibrarySting != "")
			{
				myscript = sSelectedLibrarySting.Get();
				sSelectedLibrarySting = "";
				iSelectedLibraryStingReturnID = -1; //disable.
				fpe_current_loaded_script = -1; //Reload image and DLUA.
				CloseDownEditorProperties();
				t.inputsys.constructselection = 0;
				iLastEntityOnCursor = 0;

				t.addentityfile_s = "_markers\\BehaviorHidden.fpe";
				if (t.addentityfile_s != "")
				{
					entity_adduniqueentity(false);
					t.tasset = t.entid;
					if (t.talreadyloaded == 0)
					{
						editor_filllibrary();
						iRestoreEntidMaster = -1;
					}
				}
				t.tentid = t.entid;
				t.sourceobj = g.entitybankoffset + t.tentid;

				t.gridentity = t.entid;

				//PE: all t.gridentity... need to be set for this to work correctly.
				t.gridentitystaticmode = t.entityprofile[t.entid].defaultstatic;
				t.gridentityposx_f = 0;
				t.gridentityposy_f = -999999;
				t.gridentityposz_f = 0;
				t.gridentityrotatex_f = 0;
				t.gridentityrotatey_f = 0;
				t.gridentityrotatez_f = 0;
				t.gridentityrotatequatmode = 0;
				t.gridentityrotatequatx_f = 0;
				t.gridentityrotatequaty_f = 0;
				t.gridentityrotatequatz_f = 0;
				t.gridentityrotatequatw_f = 1;
				t.gridentityscalex_f = 10;
				t.gridentityscaley_f = 10;
				t.gridentityscalez_f = 10;
				entity_fillgrideleproffromprofile();

				//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
				extern bool bNextObjectMustBeClone;
				bNextObjectMustBeClone = true;
				t.e = 0;
				gridedit_addentitytomap(); //Add it to map set t.e
				bNextObjectMustBeClone = false;

				t.entityelement[t.e].eleprof.aimain_s = myscript.c_str();
				t.entityelement[t.e].eleprof.thumb_aimain_s = "";

				PositionObject(t.entityelement[t.e].obj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z);
				RotateObject(t.entityelement[t.e].obj, t.entityelement[t.e].rx, t.entityelement[t.e].ry, t.entityelement[t.e].rz);
				HideObject(t.entityelement[t.e].obj);

				// Show elements when placing a new one down, prevents half being hidden and half not.
				t.showeditorelements = 1;
				editor_toggle_element_vis(t.showeditorelements);

				editor_refresheditmarkers();

				t.refreshgrideditcursor = 1;
				current_selected_group = -1;
				t.gridentity = 0;
				t.gridentityposoffground = 0;
				t.gridentityusingsoftauto = 0;
				t.gridentityautofind = 0;
				t.gridentityobj = 0;
				editor_refreshentitycursor();
				t.widget.pickedObject = 0;
				t.gridentityextractedindex = 0;

				t.widget.pickedObject = 0; widget_updatewidgetobject();
				iSelectecElement = t.e - 1;
			}
		}

		if (ImGui::StyleButton("Add", ImVec2(buttonwide, 0)))
		{
			//Select script.
			sStartLibrarySearchString = "global";
			iLastDisplayLibraryType = -1;
			bExternal_Entities_Window = true;
			iDisplayLibraryType = 4;
			iLibraryStingReturnToID = window->GetID("ScriptSelector##+");
		}
		ImGui::SameLine();
		int iDeleteIfFound = 0;
		if (ImGui::StyleButton("Delete", ImVec2(buttonwide, 0)))
		{
			if (iSelectecElement >= 0)
			{
				//PE: Delete.
				int iAction = askBoxCancel("This will delete all your visual changes, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
				if (iAction == 1)
				{
					iDeleteIfFound = iSelectecElement;
				}
			}
		}

		uint32_t uniqueId = 4000;
		uniqueId += 28000; //PE: from lib uniqueId += 24000;

		int iDefaultTexture = FILETYPE_SCRIPT;
		float w = ImGui::GetContentRegionAvailWidth();
		ImGui::Columns(2, "GlobalBehaviors2elements", false);  //false no border
		ImGui::SetColumnWidth(0, w * 0.5f);
		ImGui::SetColumnWidth(1, w * 0.5f);
		bool bFoundSelected = false;
		for (t.e = 1; t.e <= g.entityelementlist; t.e++)
		{
			t.entid = t.entityelement[t.e].bankindex;
			if (t.entid > 0 && t.entityprofile[t.entid].ismarker == 12)
			{
				int image = FILETYPE_SCRIPT;
				if (t.entityelement[t.e].eleprof.aimain_s.Len() > 0)
				{
					//PE: check if we need to update icons.
					if (t.entityelement[t.e].eleprof.aimain_s != t.entityelement[t.e].eleprof.thumb_aimain_s ||
						t.entityelement[t.e].eleprof.thumb_id != uniqueId)
					{
						//PE: Load image.
						std::string sFile = Left(t.entityelement[t.e].eleprof.aimain_s.Get(), Len(t.entityelement[t.e].eleprof.aimain_s.Get()) - 4);
						std::string sImgName = "scriptbank\\" + sFile;
						if (pref.current_style == 25 || pref.current_style == 3)
							sImgName += ".png";
						else
							sImgName += "2.png";

						if (ImageExist(uniqueId) == 1) DeleteImage(uniqueId);
						image_setlegacyimageloading(true);
						LoadImage((char*)sImgName.c_str(), uniqueId);
						image_setlegacyimageloading(false);
						t.entityelement[t.e].eleprof.thumb_aimain_s = t.entityelement[t.e].eleprof.aimain_s;
						t.entityelement[t.e].eleprof.thumb_id = uniqueId;
					}
					if (ImageExist(uniqueId))
					{
						image = uniqueId;
					}
					uniqueId++;
				}


				int iTextureID = image;

				ImVec2 ImageSize = ImVec2(buttonwide, ImGui::GetFontSize());
				if (!(ImGui::GetColumnIndex() % 2))
					ImageSize.x -= 2;

				ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iTextureID);
				if (lpTexture)
				{
					float img_w = ImageWidth(iTextureID);
					float img_h = ImageHeight(iTextureID);
					ImageSize.y = img_h * (ImageSize.x / img_w);
				}
				ImVec2 vImagePos = ImGui::GetCursorPos();
				ImGui::Dummy(ImageSize);
				ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
				ImVec4 back_color = ImVec4(0.2, 0.2, 0.2, 0.75);

				if (ImGui::IsItemHovered())
				{
					color.w = 0.75;
					if (ImGui::IsMouseReleased(0))
					{
						iSelectecElement = t.e;
						fpe_current_loaded_script = -1; //Reload image and DLUA.

					}
				}

				ImVec2 img_pos = ImGui::GetWindowPos() + vImagePos;
				img_pos.x -= 3; //PE: Fit under buttons.
				if ((ImGui::GetColumnIndex() % 2))
					img_pos.x -= 3;
				img_pos.y -= ImGui::GetScrollY();
				window->DrawList->AddRectFilled(img_pos, img_pos + ImageSize, ImGui::GetColorU32(back_color));
				if (lpTexture)
				{
					window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + ImageSize, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(color));
				}
				if (iSelectecElement == t.e)
				{
					bFoundSelected = true;
					ImVec4 bg_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram]; // { 0.0, 0.0, 0.0, 1.0 };
					window->DrawList->AddRect(img_pos, img_pos + ImageSize - ImVec2(0, 0), ImGui::GetColorU32(bg_col), 0, 15, 3.0f);
				}

				std::string name = t.entityelement[t.e].eleprof.aimain_s.Get();
				std::size_t slash = name.find_last_of("/\\");
				if (slash > 0)
				{
					name = name.substr(slash + 1);
				}
				ImGui::TextCenter(" %s", name.c_str());

				ImGui::NextColumn();

			}
		}
		ImGui::Columns(1);

		ImGui::Separator();
		if (bFoundSelected && iSelectecElement >= 0)
		{
			int iEntityIndex = iSelectecElement;
			int iMasterID = t.entityelement[iEntityIndex].bankindex;
			if (iMasterID > 0)
			{
				DisplayFPEBehavior(false, iMasterID, &t.entityelement[iEntityIndex].eleprof, iEntityIndex, true);
				ImGui::Separator();
				bool btmp = t.entityelement[iEntityIndex].eleprof.systemwide_lua;

				float fPropertiesColoumWidth = ImGui::GetCursorPosX() + 110.0f;
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text("Active All Levels");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
				if (ImGui::Checkbox("##Active All Levels", &btmp))
				{
					t.entityelement[iEntityIndex].eleprof.systemwide_lua = btmp;
				}
			}
		}

		if (bFoundSelected && iDeleteIfFound > 0)
		{
			t.obj = t.entityelement[iDeleteIfFound].obj;
			if (t.obj > 0)
			{
				if (ObjectExist(t.obj) == 1)
				{
					DeleteObject(t.obj);
				}
			}
			t.entityelement[iDeleteIfFound].obj = 0;
			t.entityelement[iDeleteIfFound].maintype = 0;
			t.entityelement[iDeleteIfFound].bankindex = 0;
			fpe_current_loaded_script = -1; //Reload image and DLUA.
			iSelectecElement = -1;
		}

		ImGui::Indent(-10);
	}
	return(bVisualUpdated);
}

bool Shadows_Settings(float fTabColumnWidth, bool bVisualUpdated)
{
	int wflags = ImGuiTreeNodeFlags_None;
	if (pref.bAutoClosePropertySections && iLastOpenHeader != 9)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	if (ImGui::StyleCollapsingHeader("Shadows", wflags))
	{
		ImGui::Indent(10);

		iLastOpenHeader = 9;

		ImGui::TextCenter("Shadow Resolution");
		//PE: Change from 1024 to 4096, adds around 2 gb additional gpu mem ?.
		//PE: See comment about point light below.
		const char* shadow_spot_items_align[] = { "Off", "128", "256", "512", "1024", "2048" }; //, "4096" };
		int shadow_cascade_current_type_selection = 0;
		if (t.visuals.iShadowSpotCascadeResolution == 0) shadow_cascade_current_type_selection = 0;
		else if (t.visuals.iShadowSpotCascadeResolution == 128) shadow_cascade_current_type_selection = 1;
		else if (t.visuals.iShadowSpotCascadeResolution == 256) shadow_cascade_current_type_selection = 2;
		else if (t.visuals.iShadowSpotCascadeResolution == 512) shadow_cascade_current_type_selection = 3;
		else if (t.visuals.iShadowSpotCascadeResolution == 1024) shadow_cascade_current_type_selection = 4;
		else if (t.visuals.iShadowSpotCascadeResolution == 2048) shadow_cascade_current_type_selection = 5;
		else shadow_cascade_current_type_selection = 5; //6;
		tab_tab_Column_text("Sun ", fTabColumnWidth);
		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##setshadow_spotresolution", &shadow_cascade_current_type_selection, shadow_spot_items_align, IM_ARRAYSIZE(shadow_spot_items_align)))
		{
			if (shadow_cascade_current_type_selection == 0) t.visuals.iShadowSpotCascadeResolution = 0;
			else if (shadow_cascade_current_type_selection == 1) t.visuals.iShadowSpotCascadeResolution = 128;
			else if (shadow_cascade_current_type_selection == 2) t.visuals.iShadowSpotCascadeResolution = 256;
			else if (shadow_cascade_current_type_selection == 3) t.visuals.iShadowSpotCascadeResolution = 512;
			else if (shadow_cascade_current_type_selection == 4) t.visuals.iShadowSpotCascadeResolution = 1024;
			else if (shadow_cascade_current_type_selection == 5) t.visuals.iShadowSpotCascadeResolution = 2048;
			else t.visuals.iShadowSpotCascadeResolution = 2048;
			t.gamevisuals.iShadowSpotCascadeResolution = t.visuals.iShadowSpotCascadeResolution;
			bVisualUpdated = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Choose a shadow quality for sun shadows");
		ImGui::PopItemWidth();


		//SPOT
		//const char* shadow_spot_items_align[] = { "Off", "128", "256", "512", "1024", "2048" }; //, "4096" };
		int shadow_spot_current_type_selection = 0;
		if (t.visuals.iShadowSpotResolution == 0) shadow_spot_current_type_selection = 0;
		else if (t.visuals.iShadowSpotResolution == 128) shadow_spot_current_type_selection = 1;
		else if (t.visuals.iShadowSpotResolution == 256) shadow_spot_current_type_selection = 2;
		else if (t.visuals.iShadowSpotResolution == 512) shadow_spot_current_type_selection = 3;
		else if (t.visuals.iShadowSpotResolution == 1024) shadow_spot_current_type_selection = 4;
		else if (t.visuals.iShadowSpotResolution == 2048) shadow_spot_current_type_selection = 5;
		else shadow_spot_current_type_selection = 5; //6;
		tab_tab_Column_text("Spot Lights ", fTabColumnWidth);
		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##setshadow_spotspotresolution", &shadow_spot_current_type_selection, shadow_spot_items_align, IM_ARRAYSIZE(shadow_spot_items_align)))
		{
			if (shadow_spot_current_type_selection == 0) t.visuals.iShadowSpotResolution = 0;
			else if (shadow_spot_current_type_selection == 1) t.visuals.iShadowSpotResolution = 128;
			else if (shadow_spot_current_type_selection == 2) t.visuals.iShadowSpotResolution = 256;
			else if (shadow_spot_current_type_selection == 3) t.visuals.iShadowSpotResolution = 512;
			else if (shadow_spot_current_type_selection == 4) t.visuals.iShadowSpotResolution = 1024;
			else if (shadow_spot_current_type_selection == 5) t.visuals.iShadowSpotResolution = 2048;
			else t.visuals.iShadowSpotResolution = 2048;
			t.gamevisuals.iShadowSpotResolution = t.visuals.iShadowSpotResolution;
			bVisualUpdated = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Choose a shadow quality spot lights");
		ImGui::PopItemWidth();


		//PE: This drain memory and make everything very slow, limit to max 2048 for now.
		//PE: 1 point light have 6 * 4096x4096 textures. (BIND_DEPTH_STENCIL) DXGI_FORMAT_R16_TYPELESS (2 bytes per pixel).
		//PE: AND 6 * 4096x4096 textures (BIND_RENDER_TARGET) FORMAT_R16G16B16A16_FLOAT (8 bytes per pixel).
		//PE: So 32mb + 128mb. = 160 mb. per texture = 6 * 160mb = 960 mb. PER point light :(
		//PE: Each point light have 12 4096x4096 textures.
		//PE: On my system with 5 point light, it use around 4gb additional mem, when i set 4096 ?
		//PE: This gets allocated as shared GPU mem, and sure tons of swapping is going on.
		//PE: Latest REPO changed from:	desc.Format = FORMAT_R11G11B10_FLOAT; to: desc.Format = FORMAT_R16G16B16A16_FLOAT; (32bit to 64 bit.)
		//PE: https://github.com/turanszkij/WickedEngine/commit/d27ede94cc76a91ac1a1c9e4393db6460262d120#
		//PE: Perhaps limit to 1024 ? - 2048 = 240 mb. per point light , and 1024 = 60mb.

		const char* shadow_point_items_align[] = { "Off" , "128", "256", "512", "1024", "2048" }; //, "4096" };
		int shadow_point_current_type_selection = 0;
		if (t.visuals.iShadowPointResolution == 0) shadow_point_current_type_selection = 0;
		else if (t.visuals.iShadowPointResolution == 128) shadow_point_current_type_selection = 1;
		else if (t.visuals.iShadowPointResolution == 256) shadow_point_current_type_selection = 2;
		else if (t.visuals.iShadowPointResolution == 512) shadow_point_current_type_selection = 3;
		else if (t.visuals.iShadowPointResolution == 1024) shadow_point_current_type_selection = 4;
		else if (t.visuals.iShadowPointResolution == 2048) shadow_point_current_type_selection = 5;
		else shadow_point_current_type_selection = 5; //6;
		tab_tab_Column_text("Point Lights ", fTabColumnWidth);
		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##setshadow_pointresolution", &shadow_point_current_type_selection, shadow_point_items_align, IM_ARRAYSIZE(shadow_point_items_align)))
		{
			if (shadow_point_current_type_selection == 0) t.visuals.iShadowPointResolution = 0;
			else if (shadow_point_current_type_selection == 1) t.visuals.iShadowPointResolution = 128;
			else if (shadow_point_current_type_selection == 2) t.visuals.iShadowPointResolution = 256;
			else if (shadow_point_current_type_selection == 3) t.visuals.iShadowPointResolution = 512;
			else if (shadow_point_current_type_selection == 4) t.visuals.iShadowPointResolution = 1024;
			else if (shadow_point_current_type_selection == 5) t.visuals.iShadowPointResolution = 2048;
			else t.visuals.iShadowPointResolution = 2048;
			if (t.visuals.iShadowPointResolution > 2048) t.visuals.iShadowPointResolution = 2048;
			t.gamevisuals.iShadowPointResolution = t.visuals.iShadowPointResolution;
			bVisualUpdated = true;
		}

		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Choose a shadow quality for point lights");
		ImGui::PopItemWidth();

		ImGui::TextCenter("Shadow Quantity");
		const char* shadow_spot_max_align[] = { "0", "4", "8", "12", "16" };
		int shadow_cascade_max_current_type_selection = 0;
		if (t.visuals.iShadowSpotMax == 0) shadow_cascade_max_current_type_selection = 0;
		else if (t.visuals.iShadowSpotMax == 4) shadow_cascade_max_current_type_selection = 1;
		else if (t.visuals.iShadowSpotMax == 8) shadow_cascade_max_current_type_selection = 2;
		else if (t.visuals.iShadowSpotMax == 12) shadow_cascade_max_current_type_selection = 3;
		else if (t.visuals.iShadowSpotMax == 16) shadow_cascade_max_current_type_selection = 4;
		tab_tab_Column_text("Spot Lights ", fTabColumnWidth);
		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##setshadow_iShadowSpotMax", &shadow_cascade_max_current_type_selection, shadow_spot_max_align, IM_ARRAYSIZE(shadow_spot_max_align)))
		{
			if (shadow_cascade_max_current_type_selection == 0) t.visuals.iShadowSpotMax = 0;
			else if (shadow_cascade_max_current_type_selection == 1) t.visuals.iShadowSpotMax = 4;
			else if (shadow_cascade_max_current_type_selection == 2) t.visuals.iShadowSpotMax = 8;
			else if (shadow_cascade_max_current_type_selection == 3) t.visuals.iShadowSpotMax = 12;
			else if (shadow_cascade_max_current_type_selection == 4) t.visuals.iShadowSpotMax = 16;
			t.gamevisuals.iShadowSpotMax = t.visuals.iShadowSpotMax;
			bForceRefreshLightCount = true;
			bVisualUpdated = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Choose max shadow casters for spot lights");
		ImGui::PopItemWidth();

		int shadow_cascade_point_current_type_selection = 0;
		if (t.visuals.iShadowPointMax == 0) shadow_cascade_point_current_type_selection = 0;
		else if (t.visuals.iShadowPointMax == 4) shadow_cascade_point_current_type_selection = 1;
		else if (t.visuals.iShadowPointMax == 8) shadow_cascade_point_current_type_selection = 2;
		else if (t.visuals.iShadowPointMax == 12) shadow_cascade_point_current_type_selection = 3;
		else if (t.visuals.iShadowPointMax == 16) shadow_cascade_point_current_type_selection = 4;
		tab_tab_Column_text("Point Lights ", fTabColumnWidth);

		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##setshadow_iShadowPointMax", &shadow_cascade_point_current_type_selection, shadow_spot_max_align, IM_ARRAYSIZE(shadow_spot_max_align)))
		{
			if (shadow_cascade_point_current_type_selection == 0) t.visuals.iShadowPointMax = 0;
			else if (shadow_cascade_point_current_type_selection == 1) t.visuals.iShadowPointMax = 4;
			else if (shadow_cascade_point_current_type_selection == 2) t.visuals.iShadowPointMax = 8;
			else if (shadow_cascade_point_current_type_selection == 3) t.visuals.iShadowPointMax = 12;
			else if (shadow_cascade_point_current_type_selection == 4) t.visuals.iShadowPointMax = 16;
			t.gamevisuals.iShadowPointMax = t.visuals.iShadowPointMax;
			bForceRefreshLightCount = true;
			bVisualUpdated = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Choose max shadow casters for point lights.");
		ImGui::PopItemWidth();

		if (ImGui::Checkbox("Transparent shadows", &t.visuals.bTransparentShadows))
		{
			t.gamevisuals.bTransparentShadows = t.visuals.bTransparentShadows;
			bForceRefreshLightCount = true;
			bVisualUpdated = true;
		}
		//ImGui::PopItemWidth(); //PE: This looks wrong, try removing it.

		extern bool bShadowsInFrontTakesPriority;
		ImGui::Checkbox("Front Shadows Priority", &bShadowsInFrontTakesPriority);

		ImGui::Indent(-10);
	}
	return bVisualUpdated;
}

void RenderPreviewEmitter(void)
{
	static bool bInit = true;
	if (PreviewWPERoot > 0)
	{
		float posx, posy, posz, posxa, posya, posza;
		int GetActiveEditorEntityPos(float* x, float* y, float* z, float* xa, float* ya, float* za);
		int iEntityIndex = GetActiveEditorEntityPos(&posx, &posy, &posz, &posxa, &posya, &posza);
		static int iEntityIndexCurrent = -1;
		if (!bInit && iEntityIndexCurrent != iEntityIndex && PreviewWPERoot != 0)
		{
			iEntityIndexCurrent = iEntityIndex;
			//PE: Delete effects.
			WickedCall_PerformEmitterAction(6, PreviewWPERoot);
			void DeleteEmitterEffects(uint32_t root);
			DeleteEmitterEffects(PreviewWPERoot);
			PreviewWPERoot = 0;
			bPreviewWPE = false;
		}
		if (bInit)
		{
			iEntityIndexCurrent = iEntityIndex;
			bInit = false;
		}
		if (iEntityIndex > 0 && PreviewWPERoot > 0)
		{
			extern float fPreviewYOffset;
			bool WickedCall_ParticleEffectPositionRotation(uint32_t root, float fX, float fY, float fZ, float fXa, float fYa, float fZa);
			WickedCall_ParticleEffectPositionRotation(PreviewWPERoot, posx, posy + fPreviewYOffset, posz, 0, posya, 0);
		}
	}
}

void GetActiveMonitorResolution( void )
{
	//PE: Try to get the resolution of the monitor Max is currently active in.
	HMONITOR hMonitor = MonitorFromWindow(g_pGlob->hWnd, MONITOR_DEFAULTTONEAREST);
	CurrentMonitorResolutionX = 0;
	CurrentMonitorResolutionY = 0;

	if (hMonitor == NULL) {
		return;
	}

	//Get the monitor's device name using MONITORINFOEX
	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);

	if (!GetMonitorInfo(hMonitor, &info)) {
		return;
	}

	//Get the physical resolution using EnumDisplaySettings and the device name
	DEVMODE devMode;
	devMode.dmSize = sizeof(DEVMODE);

	if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devMode)) {
		CurrentMonitorResolutionX = devMode.dmPelsWidth;
		CurrentMonitorResolutionY = devMode.dmPelsHeight;
	}
}
