//
// MasterRenderer Functions
//
#include "wiGraphicsDevice_DX12.h" // Phase 5: For DX12 ImGui rendering in Compose

void MasterRenderer::Load()
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	__super::Load();

	// remove VSYNC cap
	wiEvent::SetVSync( false );

	// clear image management system early
	char pCurrentDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, pCurrentDir);
	WickedCall_InitImageManagement(pCurrentDir);

	// default wicked settings
	setSSREnabled ( false ); //PE: Not on by default.
	setReflectionsEnabled ( true );
	setFXAAEnabled ( false ); //PE: We already have MSAA 8 so.
	#ifdef POSTPROCESSRAIN
	setRainEnabled(false); //PE: test post process shader.
	setRainTextures("Files\\effectbank\\common\\rain_color.png", "Files\\effectbank\\common\\rain_normal.png");
	setRainOpacity(0.0);
	setRainScaleX(1.0);
	setRainScaleY(1.0);
	setRainRefreactionScale(0.01);
	#endif
	#ifdef POSTPROCESSSNOW
	setSnowLayers(15.0);
	setSnowDepth(0.5);
	setSnowWindiness(0.5);
	setSnowSpeed(0.15);
	setSnowOpacity(1.0);
	setSnowOffset(0.0);
	#endif
	setBloomEnabled ( true );
	setShadowsEnabled ( true );
	wiRenderer::SetTessellationEnabled(false); //PE: Tessellation dont work like this it has to be set per mesh, so have never worked.
	setLightShaftsEnabled ( true );
	setBloomThreshold ( 2.0f );
	//setBloomStrength( 1.0f );

	// only activated when in TEST LEVEL
	setEyeAdaptionEnabled( false );

	// disable wicked backlog, can draw behind imgui , can be seen sometimes.
	if (wiBackLog::isActive()) 
	{
		wiBackLog::Toggle();
	}

	// for best terrain rendering
	wiGraphics::SamplerDesc desc = wiRenderer::GetSampler ( SAMPLER_OBJECTSHADER )->GetDesc ( );
	desc.filter = wiGraphics::Filter::ANISOTROPIC;

	// create cloudy sky by default
	Scene weatherscene;
	g_weatherEntityID = CreateEntity();
	auto& weather = weatherscene.weathers.Create ( g_weatherEntityID );
	weather.ambient = XMFLOAT3(0.5f, 0.5f, 0.5f);
	weather.horizon = XMFLOAT3 ( 0.38f, 0.38f, 0.38f );
	weather.zenith = XMFLOAT3 ( 0.42f, 0.42f, 0.42f );
	
	//PE: We dont want any lightshaft from the sun. before we activate it.
	weather.volumetricCloudParameters.layerFirst.coverageAmount = 0.0f;
	weather.volumetricCloudParameters.layerFirst.windSpeed = 0.0f;
	weather.fogStart = 0;
	weather.fogDensity = 0;
	weather.SetRealisticSky( true );

	weather.SetVolumetricClouds( true );

	wiScene::GetScene ( ).Merge ( weatherscene );

	// create directional light from sun
	Entity entitySunLight = wiScene::GetScene ( ).Entity_CreateLight ( "sunLight", XMFLOAT3 ( 0, 3, 0 ), XMFLOAT3 ( 1, 1, 1 ), 4, 600 );
	g_entitySunLight = entitySunLight;
	LightComponent* lightSun = wiScene::GetScene ( ).lights.GetComponent ( entitySunLight );
	lightSun->SetCastShadow ( true );
	lightSun->SetVisualizerEnabled ( false );
	lightSun->direction = XMFLOAT3 ( 0.25, -0.5, 0.25 );
	lightSun->color = XMFLOAT3 ( 1.0, 1.0, 1.0 );
	lightSun->SetType ( wiScene::LightComponent::DIRECTIONAL );
	lightSun->SetVolumetricsEnabled( true );

	// LB: sun needs lens flare texture
	int iFlareCount = 3;
	lightSun->lensFlareRimTextures.resize(iFlareCount);
	lightSun->lensFlareNames.resize(iFlareCount);
	for (int iFlareChain = 0; iFlareChain < iFlareCount; iFlareChain++)
	{
		std::string fileName;
		if (iFlareChain == 0) fileName = "Files\\lensflares\\flare1.jpg";
		if (iFlareChain == 1) fileName = "Files\\lensflares\\flare2.jpg";
		if (iFlareChain == 2) fileName = "Files\\lensflares\\flare3.jpg";
		lightSun->lensFlareRimTextures[iFlareChain] = wiResourceManager::Load(fileName);
		lightSun->lensFlareNames[iFlareChain] = fileName;
	}

	// force window to maximised view
	HWND hWnd = GetActiveWindow();
	ShowWindow(hWnd, SW_MAXIMIZE);
}

void MasterRenderer::Update(float dt)
{
	// otherwise continue
	if (m_bRenderingVR == false)
	{
#ifdef OPTICK_ENABLE
		OPTICK_EVENT("GuruLoopLogic");
#endif
		// regular update mode
		auto range = wiProfiler::BeginRangeCPU("Update - Logic");
		bool bFullyInitialised = GuruLoopLogic();
		wiProfiler::EndRange(range);

		// no further than logic while in splash show mode
		if (g_iShowSplashForFirstFewCycles > 0)
		{
			return;
		}
		if (bFullyInitialised == true )
		{
			// normal update
			wiScene::CameraComponent &camera = wiScene::GetCamera();

			// must be outside a render pass and only called once, even if VR renders twice
			CommandList cmd = wiGraphics::GetDevice()->BeginCommandList();
			auto range = wiProfiler::BeginRangeCPU("Update - Particles");
			gpup_update(dt, cmd);
			wiProfiler::EndRange(range);

			// terrain processing (if used)
			if (t.visuals.bEnableEmptyLevelMode == false)
			{
				extern int g_iDisableTerrainSystem;
				auto range3 = wiProfiler::BeginRangeCPU("Update - Terrain");
				extern bool bImGuiRenderTargetFocus;
				GGTerrain_Update(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd, bImGuiRenderTargetFocus);
				if (g_iDisableTerrainSystem == 0)
				{
					GGTrees_Update(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd, bImGuiRenderTargetFocus);
					GGTrees_UpdateFrustumCulling(&camera);
					GGGrass_Update(&camera, cmd, bImGuiRenderTargetFocus);
				}
				wiProfiler::EndRange(range3);
			}
			else
			{
				// still need for terrain globals to update local params (for editable_size reading)
				GGTerrain_Update_EmptyLevel(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd);
			}
			
#ifdef WICKEDPARTICLESYSTEM
			auto range4 = wiProfiler::BeginRangeCPU("Update - Emitters");
			WickedCall_UpdateEmitters();
			wiProfiler::EndRange(range4);
#endif

      // now just prepared IMGUI, but actual render called from Wicked hook
			auto range2 = wiProfiler::BeginRangeCPU("Update - Render");
			GuruLoopRender();
			wiProfiler::EndRange(range2);
		}
	}

	//Disable wicked backlog, can draw behind imgui , can be seen sometimes. Make sure it is never activated.
	if (wiBackLog::isActive()) wiBackLog::Toggle();

	// super update
	auto range2 = wiProfiler::BeginRangeCPU("Update - Wicked");
	__super::Update(dt);
	wiProfiler::EndRange(range2);
}

void MasterRenderer::ResizeBuffers(void)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	if ( GetInternalResolution().x == 0 || GetInternalResolution().y == 0 ) return;


	//PE: Must be the same, currently its out of synch with internal resolution.
	//PE: Cant change if in VR that have another resolution.
	if (!m_bUsingVR)
	{
		// TODO: Set3DResolution removed, use resolutionScale instead
		//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false); //GGREDUCED
	}

	//PE: Resizebuffers change FOV.

	__super::ResizeBuffers();

	GraphicsDevice* device = wiGraphics::GetDevice();
	HRESULT hr;

	if (GetDepthStencil() != nullptr)
	{
		TextureDesc desc;
		desc.width = GetPhysicalWidth();
		desc.height = GetPhysicalHeight();
		desc.sample_count = 1;
		desc.format = Format::R8_UNORM;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;

		//PE: Get the below error when using dx11 debug layer, so switched to resolving MSAA instead.
		//D3D11 ERROR: ID3D11DeviceContext::Draw: The Shader Resource View dimension declared in the shader code (TEXTURE2D)
		//             does not match the view type bound to slot 23 of the Pixel Shader unit (TEXTURE2DMS).
		//             This mismatch is invalid if the shader actually uses the view (e.g. it is not skipped due to shader code branching).
		//             [ EXECUTION ERROR #354: DEVICE_DRAW_VIEW_DIMENSION_MISMATCH]

		hr = device->CreateTexture(&desc, nullptr, &rt_Outline);
		hr = device->CreateTexture(&desc, nullptr, &rt_Outline_Red);
		hr = device->CreateTexture(&desc, nullptr, &rt_Outline_Blue);
		if (getMSAASampleCount() > 1)
		{
			desc.sample_count = getMSAASampleCount();
			hr = device->CreateTexture(&desc, nullptr, &rt_MSAAOutline);
			hr = device->CreateTexture(&desc, nullptr, &rt_MSAAOutline_Red);
			hr = device->CreateTexture(&desc, nullptr, &rt_MSAAOutline_Blue);
		}
		assert(SUCCEEDED(hr));
	}

	{
		RenderPassDesc desc;

		//PE: New wickedrepo.
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(rt_Outline, RenderPassAttachment::LoadOp::CLEAR));

		//PE: We now use the MSAA desc.sample_count so dont need to resolve it.
		if (getMSAASampleCount() > 1)
		{
			desc.attachments[0].texture = rt_MSAAOutline;
			desc.attachments.push_back(RenderPassAttachment::Resolve(rt_Outline));
		}

		//wiTextureHelper::getBlack(),

		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				*GetDepthStencil(),
				RenderPassAttachment::LoadOp::LOAD,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY
			)
		);

		hr = device->CreateRenderPass(&desc, &renderpass_Outline);
		assert(hr);

		desc.attachments[0].texture = rt_Outline_Red;
		if (getMSAASampleCount() > 1)
		{
			desc.attachments[0].texture = rt_MSAAOutline_Red;
			desc.attachments[1].texture = rt_Outline_Red;
		}
		hr = device->CreateRenderPass(&desc, &renderpass_Outline_Red);
		assert(hr);

		desc.attachments[0].texture = rt_Outline_Blue;
		if (getMSAASampleCount() > 1)
		{
			desc.attachments[0].texture = rt_MSAAOutline_Blue;
			desc.attachments[1].texture = rt_Outline_Blue;
	}
		hr = device->CreateRenderPass(&desc, &renderpass_Outline_Blue);
		assert(hr);

	}

	GGTerrain_WindowResized();
	extern bool bTriggerFovUpdate;
	bTriggerFovUpdate = true; //PE: restore FOV.
}


float fGetHighlightThickness(void);

void Wicked_Render_Opaque_Scene(CommandList cmd)
{
	extern bool g_bNo2DRender;
	extern bool BackBufferSnapShotMode;
	if (bImGuiInTestGame)
	{
		if (bActivateStandaloneOutline && master_renderer->GetDepthStencil() != nullptr) //&& !translator.selected.empty())
		{
			GraphicsDevice* device = wiGraphics::GetDevice();

			XMFLOAT4 area;
			bool ImGuiHook_GetScissorArea(float* pX1, float* pY1, float* pX2, float* pY2);
			if (ImGuiHook_GetScissorArea(&area.x, &area.y, &area.z, &area.w) == true)
				//device->SetScissorArea(cmd, area);

			wiRenderer::BindCommonResources(cmd);
			XMFLOAT4 col = XMFLOAT4(0.8f, 0.8f, 0.8f, 0.8f);;
			wiRenderer::Postprocess_Outline(rt_Outline, cmd, 0.1f, 0.8f, col);
			device->EventEnd(cmd);
			area = { 0, 0, (float)master.masterrenderer.GetPhysicalWidth(), (float)master.masterrenderer.GetPhysicalHeight() };
			//device->SetScissorArea(cmd, area);
		}
		return;
	}

	if (!bImGuiInTestGame && (!g_bNo2DRender || BackBufferSnapShotMode) )
	{
		if (master_renderer->GetDepthStencil() != nullptr) //&& !translator.selected.empty())
		{
			GraphicsDevice* device = wiGraphics::GetDevice();

			XMFLOAT4 area;
			float thickness = fGetHighlightThickness();
			bool ImGuiHook_GetScissorArea(float* pX1, float* pY1, float* pX2, float* pY2);
			if (ImGuiHook_GetScissorArea(&area.x, &area.y, &area.z, &area.w) == true)
				//device->SetScissorArea(cmd, area);

			wiRenderer::BindCommonResources(cmd);
			XMFLOAT4 col = selectionColor;
			col.w *= 0.65; //opacity;
			wiRenderer::Postprocess_Outline(rt_Outline, cmd, 0.1f, thickness, col);
			device->EventEnd(cmd);

			col = selectionColorRed;
			col.w *= 0.65; //opacity;
			wiRenderer::Postprocess_Outline(rt_Outline_Red, cmd, 0.1f, thickness, col);
			device->EventEnd(cmd);

			col = selectionColorBlue;
			col.w *= 0.65; //opacity;
			wiRenderer::Postprocess_Outline(rt_Outline_Blue, cmd, 0.1f, thickness, col);
			device->EventEnd(cmd);

			area = { 0, 0, (float)master.masterrenderer.GetPhysicalWidth(), (float)master.masterrenderer.GetPhysicalHeight() };
			//device->SetScissorArea(cmd, area);

		}
	}
}

void MasterRenderer::Compose(CommandList cmd) const
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	__super::Compose(cmd);

	// Phase 5: Render ImGui draw data using DX12 backend
	extern bool bImGuiInitDone;
	if (bImGuiInitDone)
	{
		extern bool ImGui_DX12_IsInitialized();
		if (ImGui_DX12_IsInitialized())
		{
			auto* dx12Device = dynamic_cast<wi::graphics::GraphicsDevice_DX12*>(wi::graphics::GetDevice());
			if (dx12Device)
			{
				ID3D12GraphicsCommandList* nativeCmdList = dx12Device->GetDX12GraphicsCommandList(cmd);
				if (nativeCmdList)
				{
					extern void ImGui_DX12_RenderBridge(ID3D12GraphicsCommandList* cmdList);
					ImGui_DX12_RenderBridge(nativeCmdList);
				}
			}
		}
	}
}

void MasterRenderer::Render() const
{
	__super::Render();
}

// moved into function so we can call it at the right time from within renderpath3D, just before 2D is rendered
std::vector<int> g_StandaloneObjectHighlightList;
void MasterRenderer::RenderOutlineHighlighers(CommandList cmd) const
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	if (m_bRenderingVR == false)
	{
		// regular update
		// PE: Make sure highlight and other editor functions is not redered in test game.
		void WickedCall_RenderEditorFunctions(void);
		//PE: Moved to function so we can delete any already setup highlights.
		//if (!bImGuiInTestGame) WickedCall_RenderEditorFunctions();
		extern bool bImGuiRenderTargetFocus;
		WickedCall_SetRenderTargetMouseFocus(bImGuiRenderTargetFocus);
		WickedCall_RenderEditorFunctions();

		if (!bImGuiInTestGame)
		{
			// Selection outline:
			if (GetDepthStencil() != nullptr)
			{
				GraphicsDevice* device = wiGraphics::GetDevice();
				CommandList cmd = device->BeginCommandList();

				device->EventBegin("GGMax - Selection Outline Mask", cmd);

				Viewport vp;
				vp.width = (float)rt_Outline.GetDesc().width;
				vp.height = (float)rt_Outline.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				wiImageParams fx;
				fx.enableFullScreen();
				fx.stencilComp = STENCILMODE::STENCILMODE_EQUAL;
				fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

				// Objects outline:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}

				// Objects outline Red:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline_Red, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT_RED;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}

				// Objects outline Blue:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline_Blue, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT_BLUE;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}
				device->EventEnd(cmd);
			}
		}
		else
		{
			if (!bActivateStandaloneOutline) return;

			//PE: Add objects here for standalone highlights/Outline.
			if (g_StandaloneObjectHighlightList.size() > 0)
			{
				for (int i = 0; i < (int)g_StandaloneObjectHighlightList.size(); i++)
				{
					int obj = g_StandaloneObjectHighlightList[i];
					if (obj > 0)
					{
						if (ObjectExist(obj))
						{
							void* GetObjectsInternalData(int iID);
							sObject* pObject = (sObject*)GetObjectsInternalData(obj);
							if (pObject)
							{
								//WickedCall_SetObjectHighlight(pObject, false);
								void WickedCall_DrawObjctBox(sObject * pObject, XMFLOAT4 color, bool bThickLine, bool ForceBox);
								WickedCall_DrawObjctBox(pObject, XMFLOAT4(0.8f, 0.8f, 0.8f, 0.8f), false, false);
							}
						}
					}
				}
			}


			if (GetDepthStencil() != nullptr)
			{
				GraphicsDevice* device = wiGraphics::GetDevice();
				CommandList cmd = device->BeginCommandList();

				device->EventBegin("GGMax - Selection Outline Mask", cmd);

				Viewport vp;
				vp.width = (float)rt_Outline.GetDesc().width;
				vp.height = (float)rt_Outline.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				wiImageParams fx;
				fx.enableFullScreen();
				fx.stencilComp = STENCILMODE::STENCILMODE_EQUAL;
				fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

				// Objects outline:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}
				device->EventEnd(cmd);
			}
		}
	}
}
