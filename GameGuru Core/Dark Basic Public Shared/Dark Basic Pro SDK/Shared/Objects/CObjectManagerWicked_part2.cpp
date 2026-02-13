/*
	// set the input streams
	if ( !SetInputStreams ( pMesh ) )
		return false;

	// do not render meshes with an effect and a single poly
	bool bSkipDrawNow = false;
	if ( pMesh->pVertexShaderEffect && pMesh->dwVertexCount<=3 )
		if ( pObject->dwObjectNumber < 70000 ) // 220618 - horrid hack (later find out why we need to hide single poly renders)
			bSkipDrawNow = true;

	#ifdef DX11
	// set input layout
	if ( pMesh->pVertexDec == NULL && pMesh->pVertexShaderEffect && bSkipDrawNow == false )
	{
		int iMeshEffectID = pMesh->pVertexShaderEffect->m_iEffectID;
		if ( iMeshEffectID > 0 && iMeshEffectID < SHADERSARRAYMAX && g_sShaders[iMeshEffectID].pInputLayout == NULL )
		{
			D3D11_INPUT_ELEMENT_DESC layoutFVF258 [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC layoutFVF274 [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC layoutFVF338 [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32_UINT,				0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC layoutFVF514 [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC layoutFVF530 [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC layoutFVFZero [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC layoutFVFZeroEightBone [ ] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			int iLayoutSize = 0;
			LPVOID pLayoutPtr = NULL;
			DWORD dwLayoutSize = 0;
			if ( pMesh->dwFVF == 258 )
			{
				iLayoutSize = 2;
				pLayoutPtr = &layoutFVF258;
				dwLayoutSize = sizeof(layoutFVF258);
			}
			if ( pMesh->dwFVF == 274 )
			{
				iLayoutSize = 3;
				pLayoutPtr = &layoutFVF274;
				dwLayoutSize = sizeof(layoutFVF274);
			}
			if ( pMesh->dwFVF == 338 )
			{
				iLayoutSize = 4;
				pLayoutPtr = &layoutFVF338;
				dwLayoutSize = sizeof(layoutFVF338);
			}
			if ( pMesh->dwFVF == 514 )
			{
				iLayoutSize = 3;
				pLayoutPtr = &layoutFVF514;
				dwLayoutSize = sizeof(layoutFVF514);
			}
			if ( pMesh->dwFVF == 530 )
			{
				iLayoutSize = 6;
				pLayoutPtr = &layoutFVF530;
				dwLayoutSize = sizeof(layoutFVF530);
			}
			if ( pMesh->dwFVF == 0 )
			{
				if ( pMesh->dwFVFOriginal == 530 )
				{
					// been conveerted to add tangent/binormal and it wiped FVF value (PBR lightmaps)
					iLayoutSize = 6;
					pLayoutPtr = &layoutFVF530;
					dwLayoutSize = sizeof(layoutFVF530);
				}
				else
				{
					if ( pMesh->dwFVFSize == 120 )
					{
						// latest 8 bones per vertex
						iLayoutSize = 9;
						pLayoutPtr = &layoutFVFZeroEightBone;
						dwLayoutSize = sizeof(layoutFVFZeroEightBone);
					}
					else
					{
						// regular 4 bones per vertex
						iLayoutSize = 7;
						pLayoutPtr = &layoutFVFZero;
						dwLayoutSize = sizeof(layoutFVFZero);
					}
				}
			}
			//LPGGVERTEXLAYOUT pNewVertexDec;	
			D3D11_INPUT_ELEMENT_DESC* pLayout = new D3D11_INPUT_ELEMENT_DESC [ iLayoutSize ];
			std::memcpy ( pLayout, pLayoutPtr, dwLayoutSize );
			ID3DBlob* pBlob = g_sShaders[iMeshEffectID].pBlob;
			DWORD tIndex = 0;
			ID3DX11EffectTechnique* tech = g_sShaders[iMeshEffectID].pEffect->GetTechniqueByIndex(0);
			ID3DX11EffectPass* pass = tech->GetPassByIndex(0);
			D3DX11_PASS_SHADER_DESC vs_desc;
			pass->GetVertexShaderDesc(&vs_desc);
			D3DX11_EFFECT_SHADER_DESC s_desc;
			vs_desc.pShaderVariable->GetShaderDesc(0, &s_desc);
			HRESULT hr = m_pD3D->CreateInputLayout ( pLayout, iLayoutSize, s_desc.pBytecode, s_desc.BytecodeLength, &g_sShaders[iMeshEffectID].pInputLayout );

			
			//PE: superflatterrain=1
			//PE: generate : Exception thrown at 0x776508F2 in Guru-MapEditor.exe: Microsoft C++ exception: _com_error at memory location 0x0019DDB0.
			if (hr != NOERROR) {
				//PE: Failed , terrain use - tindex  1, pass 1
				if ( iMeshEffectID == 1 ) { // 1==terrain. same as t.terrain.terrainshaderindex == iMeshEffectID , but we dont have t or g.

					SAFE_DELETE_ARRAY(pLayout);
					int iLayoutSize = 4;
					pLayout = new D3D11_INPUT_ELEMENT_DESC[iLayoutSize];
					D3D11_INPUT_ELEMENT_DESC layout[] =
					{
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					};
					pLayoutPtr = &layout;
					std::memcpy(pLayout, layout, sizeof(layout));

					DWORD tIndex = 0;
					ID3DX11EffectTechnique* tech = NULL;
					while ((tech = g_sShaders[iMeshEffectID].pEffect->GetTechniqueByIndex(tIndex++))->IsValid())
					{
						DWORD pIndex = 0;
						ID3DX11EffectPass* pass = NULL;
						while ((pass = tech->GetPassByIndex(pIndex++))->IsValid())
						{
							D3DX11_PASS_SHADER_DESC vs_desc;
							pass->GetVertexShaderDesc(&vs_desc);
							D3DX11_EFFECT_SHADER_DESC s_desc;
							vs_desc.pShaderVariable->GetShaderDesc(0, &s_desc);
							hr = m_pD3D->CreateInputLayout(pLayout, iLayoutSize, s_desc.pBytecode, s_desc.BytecodeLength, &g_sShaders[iMeshEffectID].pInputLayout);
							break;
						}
						if (g_sShaders[iMeshEffectID].pInputLayout != NULL) {
							break;
						}
					}

//					if (hr != NOERROR) {
//						//PE: debug.
//						char tmpdebug[2048];
//						sprintf(tmpdebug, "Error: %s error description: %s\n",
//							DXGetErrorString(hr), DXGetErrorDescription(hr));
//						OutputDebugString(tmpdebug);
//						//PE: Error returned:
//						//PE: Error: E_INVALIDARG error description: An invalid parameter was passed to the returning function.
//					}
				}
			}

			SAFE_DELETE_ARRAY(pLayout);
		}
		pMesh->pVertexDec = g_sShaders[iMeshEffectID].pInputLayout;
	}

	// set input layout
	if ( pMesh->pVertexDec == NULL ) return false;
	m_pImmediateContext->IASetInputLayout ( pMesh->pVertexDec );

	// primitive type
	m_pImmediateContext->IASetPrimitiveTopology(pDrawBuffer->dwPrimType);
	#endif

	// This POINTLIST code is not used by CORE PARTICLES (uses own renderer)
	// nor is it used by CLOTH&PARTICLES (uses quad based rendered meshes)
	// this is actuall used by the PHYSX PLUGIN FOR FLUID PARTICLES
	#ifdef DX11
	#else
	if ( pMesh->iPrimitiveType == D3DPT_POINTLIST )
	{
		// set a default - mike needs to do this in ANYTHING that creates a pointlist object
		if ( pMesh->Collision.fRadius==0.0f ) pMesh->Collision.fRadius = 50.0f;

		// handle point sprite for distance scaling and default mesh point sprite size
		m_pD3D->SetRenderState( D3DRS_POINTSCALEENABLE, TRUE );
		m_pD3D->SetRenderState( D3DRS_POINTSIZE,		FtoDW(pMesh->Collision.fRadius/100.0f) );
		m_pD3D->SetRenderState( D3DRS_POINTSIZE_MIN,	FtoDW(0.0f) );
		m_pD3D->SetRenderState( D3DRS_POINTSIZE_MAX,	FtoDW(pMesh->Collision.fRadius) );
		m_pD3D->SetRenderState( D3DRS_POINTSCALE_A,		FtoDW(0.0f) );
		m_pD3D->SetRenderState( D3DRS_POINTSCALE_B,		FtoDW(0.0f) );
		m_pD3D->SetRenderState( D3DRS_POINTSCALE_C,		FtoDW(2.0f) );

		// force a basic texture render state
		m_pD3D->SetRenderState( D3DRS_POINTSPRITEENABLE, TRUE );
	}
	#endif

	// start shader
	UINT uPasses = 1;
	bool bEffectRendering = false;
	LPGGRENDERTARGETVIEW pCurrentRenderTarget = NULL;
	LPGGDEPTHSTENCILVIEW pCurrentDepthTarget = NULL;
	bool bLocalOverrideAllTexturesAndEffects = false;

	ShaderStart ( pMesh, &pCurrentRenderTarget, &pCurrentDepthTarget, &uPasses, &bEffectRendering, &bLocalOverrideAllTexturesAndEffects );

	//PE: Speed up depth rendering.
	int iTextureCount = pMesh->dwTextureCount;
	if (g_pGlob->dwRenderCameraID >= 31) {
		//Optimize for depth render.
		iTextureCount = 1;
	}

	// when activated, can SKIP a DEPTH PASS if effect has this pass
	UINT uPassStartIndex = 0; 
	if ( pMesh->pVertexShaderEffect )
		if ( pMesh->pVertexShaderEffect->m_DepthRenderPassHandle && g_bSkipAnyDedicatedDepthRendering==true ) 
			uPassStartIndex = 1;

	// loop through all shader passes
	// each mesh can have several render passes
	bool lightset = false;
    for(UINT uPass = uPassStartIndex; uPass < uPasses; uPass++)
    {
		if (!lightset && bSkipDrawNow == false) 
		{
			lightset = true;
			update_mesh_light(pMesh, pObject, pFrame);
		}

		// start shader pass
		if ( ShaderPass ( pMesh, uPass, uPasses, bEffectRendering, bLocalOverrideAllTexturesAndEffects, pCurrentRenderTarget, pCurrentDepthTarget, pObject )==true )
		{
			// create constant buffer (and set it AFTER effect apply)
			if ( g_pCBPerMesh == NULL )
			{
				D3D11_BUFFER_DESC bdPerFrameBuffer;
				std::memset ( &bdPerFrameBuffer, 0, sizeof ( bdPerFrameBuffer ) );
				bdPerFrameBuffer.Usage          = D3D11_USAGE_DEFAULT;
				bdPerFrameBuffer.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
				bdPerFrameBuffer.CPUAccessFlags = 0;
				bdPerFrameBuffer.ByteWidth      = sizeof ( CBPerMesh );
				m_pD3D->CreateBuffer ( &bdPerFrameBuffer, NULL, &g_pCBPerMesh );
				std::memset ( &bdPerFrameBuffer, 0, sizeof ( bdPerFrameBuffer ) );
				bdPerFrameBuffer.Usage          = D3D11_USAGE_DEFAULT;
				bdPerFrameBuffer.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
				bdPerFrameBuffer.CPUAccessFlags = 0;
				bdPerFrameBuffer.ByteWidth      = sizeof ( CBPerMeshPS );
				m_pD3D->CreateBuffer ( &bdPerFrameBuffer, NULL, &g_pCBPerMeshPS );
			}
			if ( g_pCBPerMesh && g_pCBPerMeshPS )
			{
				CBPerMesh cb;
				GGGetTransform(GGTS_WORLD,&cb.mWorld);
				GGGetTransform(GGTS_VIEW,&cb.mView);
				GGGetTransform(GGTS_PROJECTION,&cb.mProjection);
				GGMatrixTranspose(&cb.mWorld,&cb.mWorld);
				GGMatrixTranspose(&cb.mView,&cb.mView);
				GGMatrixTranspose(&cb.mProjection,&cb.mProjection);
				m_pImmediateContext->UpdateSubresource( g_pCBPerMesh, 0, NULL, &cb, 0, 0 );
				m_pImmediateContext->VSSetConstantBuffers ( 0, 1, &g_pCBPerMesh );

				if (g_pGlob->dwRenderCameraID < 31) 
				{ 
					//PE: Not used in PS when depth only.
					//PE: not used in PS with normal objects anymore.
					m_pImmediateContext->PSSetConstantBuffers ( 0, 1, &g_pCBPerMesh );

					//int iEid = g.guishadereffectindex;
					//if(pMesh && pMesh->pVertexShaderEffect) iEid = pMesh->pVertexShaderEffect->m_iEffectID;
					//PE: interpolated cameraPosition do not look the same as trueCameraPosition. need fix , switch back for now.
					//if ( iEid == g.guishadereffectindex || iEid == g.guidiffuseshadereffectindex || (iEid >= g.postprocesseffectoffset && iEid < g.postprocesseffectoffset+100) ) 
					if( 1 ) 
					{
						CBPerMeshPS cbps;
						cbps.vMaterialEmissive = GGCOLOR(pMesh->mMaterial.Emissive.r, pMesh->mMaterial.Emissive.g, pMesh->mMaterial.Emissive.b, pMesh->mMaterial.Emissive.a);
						if (pMesh->bAlphaOverride == true)
							cbps.fAlphaOverride = (pMesh->dwAlphaOverride >> 24) / 255.0f;
						else
							cbps.fAlphaOverride = 1.0f;

						// feed camera zero matrices into pixel shader constant buffer for depth-to-world calc 
						tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData(0);
						float fDet = 0.0f;
						GGMatrixInverse(&cbps.mViewInv, &fDet, &m_Camera_Ptr->matView);
						GGMatrixTranspose(&cbps.mViewInv, &cbps.mViewInv);
						cbps.mViewProj = g_matThisViewProj;
						GGMatrixTranspose(&cbps.mViewProj, &cbps.mViewProj);
						cbps.mPrevViewProj = g_matPreviousViewProj;
						GGMatrixTranspose(&cbps.mPrevViewProj, &cbps.mPrevViewProj);
						m_pImmediateContext->UpdateSubresource(g_pCBPerMeshPS, 0, NULL, &cbps, 0, 0);
						m_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBPerMeshPS);
					}
				}
			}

			// apply textures for shader
			#ifdef DX11
			for ( int i = 0; i < pMesh->dwTextureCount; i++ )
			{
				// LB: Slot 5 now used again for MASK textures (part of the ALT ALBEDO MASKED FADER functionality)
				//if (i != 5) { //PE: We can always enable 5 again.
				//PE: pMesh->pTextures[i].dwStage not used so stages must be in correct order in the shaders.
				//special -123 mode means the textureref was overwritten (for animation to object texture)
				ID3D11ShaderResourceView* lpTexture = NULL;
				if ( pMesh->pTextures[i].iImageID == -123 )
					lpTexture = pMesh->pTextures[i].pTexturesRefView;
				else
					lpTexture = GetImagePointerView ( pMesh->pTextures[i].iImageID );
				m_pImmediateContext->PSSetShaderResources ( i, 1, &lpTexture );

				//PE: Only single texture on shadow maps.
				if (g_pGlob->dwRenderCameraID >= 31)
					break;
			}

			#endif
			
			// see if we have an index buffer
			if ( bSkipDrawNow==false )
			{
				if ( pMesh->pIndices )
				{
					// if multimaterial mesh
					if ( pMesh->bUseMultiMaterial && bLocalOverrideAllTexturesAndEffects==false )
					{
						// draw several indexed primitives (one for each material)
						sMultiMaterial* pMultiMaterial = pMesh->pMultiMaterial;
						for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
						{
							if ( bEffectRendering == false )
							{
								// set mesh-part texture (090217 - added support for NSG)
								ID3D11ShaderResourceView* lpTexture = pMultiMaterial [ dwMaterialIndex ].pTexturesRef;//GetImagePointerView ( pMesh->pTextures[i].iImageID );
								m_pImmediateContext->PSSetShaderResources ( 0, 1, &lpTexture );
								if ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefN ) 
								{
									lpTexture = pMultiMaterial [ dwMaterialIndex ].pTexturesRefN;
									m_pImmediateContext->PSSetShaderResources ( 2, 1, &lpTexture );
								}
								if ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefS )
								{
									lpTexture = pMultiMaterial [ dwMaterialIndex ].pTexturesRefS;
									m_pImmediateContext->PSSetShaderResources ( 3, 1, &lpTexture );
								}
								if ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefG )
								{
									lpTexture = pMultiMaterial [ dwMaterialIndex ].pTexturesRefG;
									m_pImmediateContext->PSSetShaderResources ( 6, 1, &lpTexture );
								}
								if ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefM )
								{
									lpTexture = pMultiMaterial [ dwMaterialIndex ].pTexturesRefM;
									m_pImmediateContext->PSSetShaderResources ( 5, 1, &lpTexture );
								}
							}
							else
							{
								// 150217 - now supports full range of normal mapping in shader
								ID3D11ShaderResourceView* lpTexture = pMultiMaterial [ dwMaterialIndex ].pTexturesRef;
								m_pImmediateContext->PSSetShaderResources ( 0, 1, &lpTexture );
								GGHANDLE diffuseHandle = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName( "DiffuseMap" );
								if ( diffuseHandle ) 
								{
									diffuseHandle->AsShaderResource()->SetResource ( pMultiMaterial [ dwMaterialIndex ].pTexturesRef );
								}
								GGHANDLE normalHandle = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName( "NormalMap" );
								if ( normalHandle ) 
								{
									normalHandle->AsShaderResource()->SetResource ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefN );
								}
								GGHANDLE specularHandle = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName( "SpecularMap" );
								if ( specularHandle ) 
								{
									specularHandle->AsShaderResource()->SetResource ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefS );
								}
								GGHANDLE glossHandle = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName( "IlluminationMap" );
								if ( glossHandle ) 
								{
									glossHandle->AsShaderResource()->SetResource ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefG );
								}
								GGHANDLE maskHandle = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName( "MaskMap" );
								if ( maskHandle ) 
								{
									maskHandle->AsShaderResource()->SetResource ( pMultiMaterial [ dwMaterialIndex ].pTexturesRefM );
								}
							}

							// set mesh-part material and render state
							//SetMeshMaterial ( pMesh, &pMultiMaterial [ dwMaterialIndex ].mMaterial );

							// draw mesh-part
							if ( pMultiMaterial [ dwMaterialIndex ].dwPolyCount > 0 )
							{
								#ifdef DX11
								m_pImmediateContext->DrawIndexed ( pMultiMaterial [ dwMaterialIndex ].dwPolyCount * 3, pDrawBuffer->dwIndexStart + pMultiMaterial [ dwMaterialIndex ].dwIndexStart, pDrawBuffer->dwBaseVertexIndex );
								#else
								if ( FAILED ( m_pD3D->DrawIndexedPrimitive (	pDrawBuffer->dwPrimType,
																				pDrawBuffer->dwBaseVertexIndex,
																				pDrawBuffer->dwVertexStart,
																				pDrawBuffer->dwVertexCount,
																				pDrawBuffer->dwIndexStart + pMultiMaterial [ dwMaterialIndex ].dwIndexStart,
																				pMultiMaterial [ dwMaterialIndex ].dwPolyCount	) ) )
								{
									uPass=uPasses;
									break;
								}
								#endif
							}

							// add to polycount
							if ( g_pGlob ) g_pGlob->dwNumberOfPolygonsDrawn += pMultiMaterial [ dwMaterialIndex ].dwPolyCount;
							if ( g_pGlob ) g_pGlob->dwNumberOfPrimCalls++;
						}

						// restore texture settings next cycle
						m_iLastTexture=-9999999;
					}
					else
					{
						// draw an indexed primitive
						#ifdef DX11
						m_pImmediateContext->DrawIndexed ( pMesh->dwIndexCount, pDrawBuffer->dwIndexStart, pDrawBuffer->dwBaseVertexIndex );
						#else
						if ( FAILED ( m_pD3D->DrawIndexedPrimitive (	pDrawBuffer->dwPrimType,
																		pDrawBuffer->dwBaseVertexIndex,
																		pDrawBuffer->dwVertexStart,
																		pDrawBuffer->dwVertexCount,
																		pDrawBuffer->dwIndexStart,
																		pDrawBuffer->dwPrimitiveCount		) ) )
						{
							break;
						}
						#endif

						// add to polycount
						if ( g_pGlob ) g_pGlob->dwNumberOfPolygonsDrawn += pDrawBuffer->dwPrimitiveCount;
						if ( g_pGlob ) g_pGlob->dwNumberOfPrimCalls++;
					}
				}
				else
				{
					// draw a standard primitive
					#ifdef DX11
					m_pImmediateContext->Draw ( pDrawBuffer->dwPrimitiveCount*3, pDrawBuffer->dwBaseVertexIndex );
					#else
					if ( FAILED ( m_pD3D->DrawPrimitive (	pDrawBuffer->dwPrimType,
															pDrawBuffer->dwVertexStart,
															pDrawBuffer->dwPrimitiveCount				) ) )
					{
						// if fail to render, try smaller batches of primitives until we figure it out!
						DWORD dwHowManyLeft = pDrawBuffer->dwPrimitiveCount;
						DWORD dwVertexBeginData = pDrawBuffer->dwVertexStart;
						DWORD dwPrimCountBatch=65535/3;
						for ( DWORD dwI=0; dwI<=(pDrawBuffer->dwPrimitiveCount/dwPrimCountBatch); dwI++ )
						{
							DWORD dwHowManyToRender = dwPrimCountBatch;
							if ( dwHowManyLeft < dwPrimCountBatch )
								dwHowManyToRender = dwHowManyLeft;

							if ( FAILED ( m_pD3D->DrawPrimitive (	pDrawBuffer->dwPrimType,
																	dwVertexBeginData,
																	dwHowManyToRender		) ) )
								break;

							// next batch of vertex data
							dwVertexBeginData+=dwHowManyToRender*3;
							dwHowManyLeft -= dwPrimCountBatch;
						}
					}
					#endif

					// add to polycount
					if ( g_pGlob ) g_pGlob->dwNumberOfPolygonsDrawn += pDrawBuffer->dwPrimitiveCount;
					if ( g_pGlob ) g_pGlob->dwNumberOfPrimCalls++;
				}
			}

			// unbind textures from shader pass
			#ifdef DX11
			if ( 1 )
			{
				// can release extra resources if postprocess RT render targets involved
				int iClearExtraPSResSlotsForPostProcessRTs = iTextureCount;
				if ( pMesh->pVertexShaderEffect ) 
					if ( pMesh->pVertexShaderEffect->m_bUsesAtLeastOneRT==true )
						if ( iClearExtraPSResSlotsForPostProcessRTs < 5 ) 
							iClearExtraPSResSlotsForPostProcessRTs = 5;

				// release input resources
				for ( int i = 0; i < iClearExtraPSResSlotsForPostProcessRTs; i++ )
				{
					ID3D11ShaderResourceView *const pSRV[1] = { NULL };
					m_pImmediateContext->PSSetShaderResources(i, 1, pSRV);
				}
			}
			#endif

			// end shader pass
			ShaderPassEnd ( pMesh, bEffectRendering );
		}
	}

	// finish shader
	ShaderFinish ( pMesh, pCurrentRenderTarget, pCurrentDepthTarget );

	// leeadd - 310506 - u62 - end pointlist session (used by PhysX plugin HW fluids)
	#ifdef DX11
	#else
	if ( pMesh->iPrimitiveType == D3DPT_POINTLIST )
	{
		// end pointlist states
		m_pD3D->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
	}
	#endif
		
	// okay
	return true;
}

bool CObjectManager::DrawMesh ( sMesh* pMesh )
{
	return DrawMesh ( pMesh, false );
}

int CObjectManager::SwitchRenderTargetToDepth ( int iFlag )
{
	#ifdef DX11
	// create render target if not exists
	if ( g_pMainCameraDepthTexture==NULL )
	{
		GGSURFACE_DESC desc;
		g_pGlob->pCurrentBitmapSurface->GetDesc(&desc);
		int iTryModes = 0;
		GGFORMAT dwRenderTarget = GGFMT_A8R8G8B8;
		while ( g_pMainCameraDepthTexture == NULL && iTryModes <= 6 )
		{
			if ( iTryModes == 0 ) dwRenderTarget = GGFMT_A32B32G32R32F;
			if ( iTryModes == 1 ) dwRenderTarget = GGFMT_A16B16G16R16F;
			if ( iTryModes == 2 ) dwRenderTarget = GGFMT_G32R32F;
			if ( iTryModes == 3 ) dwRenderTarget = GGFMT_R32F;
			if ( iTryModes == 4 ) dwRenderTarget = GGFMT_G16R16F;
			if ( iTryModes == 5 ) dwRenderTarget = GGFMT_R16F;
			if ( iTryModes == 6 ) dwRenderTarget = GGFMT_A8R8G8B8;
			GGSURFACE_DESC StagedDesc = { desc.Width, desc.Height, 1, 1, dwRenderTarget, 1, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
			m_pD3D->CreateTexture2D( &StagedDesc, NULL, (ID3D11Texture2D**)&g_pMainCameraDepthTexture );
			iTryModes++;
		}
		//D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		//renderTargetViewDesc.Format = dwRenderTarget;
		//renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		//renderTargetViewDesc.Texture2D.MipSlice = 0;
		HRESULT hr = m_pD3D->CreateRenderTargetView(g_pMainCameraDepthTexture, 0, &g_pMainCameraDepthTextureSurfaceRef);
		if ( hr == S_OK )
		{
			GGFORMAT depthFormat = GetValidStencilBufferFormat(desc.Format);
			g_pMainCameraDepthStencilTexture = NULL;
			GGSURFACE_DESC bufferDesc;
			bufferDesc.ArraySize = 1;
			bufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.Format = depthFormat;
			bufferDesc.Height = desc.Height;
			bufferDesc.MipLevels = 1;
			bufferDesc.MiscFlags = 0;
			bufferDesc.SampleDesc = {1,0};
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.Width = desc.Width;
			HRESULT hr = m_pD3D->CreateTexture2D(&bufferDesc, 0, &g_pMainCameraDepthStencilTexture);
			if ( hr == S_OK )
			{
				//D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
				//depthStencilViewDesc.Format = depthFormat;
				//depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				//depthStencilViewDesc.Texture2D.MipSlice = 0;
				hr = m_pD3D->CreateDepthStencilView(g_pMainCameraDepthStencilTexture, 0, &g_pMainCameraDepthStencilTextureView);
			}
		}
	}

	// copy to debug image from last usage
	if ( g_pMainCameraDepthTexture && g_pMainCameraDepthTextureSurfaceRef && rand()%100==1 )
	{
		LPGGTEXTURE pDebugSee = GetImagePointer ( 59949 );
		if ( pDebugSee )
		{
			if ( g_pMainCameraDepthTextureSurfaceRef )
			{
				#ifdef DX11
				#else
				LPGGSURFACE pShadowDebugImage;
				pDebugSee->GetSurfaceLevel ( 0, &pShadowDebugImage );
				if ( pShadowDebugImage )
				{
					HRESULT hRes = D3DXLoadSurfaceFromSurface ( pShadowDebugImage, NULL, NULL, g_pMainCameraDepthTextureSurfaceRef, NULL, NULL, D3DX_DEFAULT, 0 );
					pShadowDebugImage->Release();
				}
				#endif
			}
		}
	}

	// we channel all renders to a special depth texture render target
	m_pImmediateContext->OMSetRenderTargets ( 1, &g_pMainCameraDepthTextureSurfaceRef, g_pMainCameraDepthStencilTextureView );

	// clear if first render of this cycle
	if ( g_bFirstRenderClearsRenderTarget==false )
	{
		float ClearColor[4] = {0,0,0,0};
		m_pImmediateContext->ClearRenderTargetView(g_pMainCameraDepthTextureSurfaceRef, ClearColor);
		m_pImmediateContext->ClearDepthStencilView(g_pMainCameraDepthStencilTextureView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
		g_bFirstRenderClearsRenderTarget = true;
	}
	#else
	// create render target if not exists
	if ( g_pMainCameraDepthTexture==NULL )
	{
		D3DSURFACE_DESC desc;
		IGGSurface* pCurrentRenderTarget;
		m_pD3D->GetRenderTarget ( 0, &pCurrentRenderTarget );
		pCurrentRenderTarget->GetDesc( &desc );
		int iTryModes = 0;
		while ( g_pMainCameraDepthTexture == NULL && iTryModes <= 6 )
		{
			GGFORMAT dwRenderTarget = GGFMT_A8R8G8B8;
			if ( iTryModes == 0 ) dwRenderTarget = GGFMT_A32B32G32R32F;
			if ( iTryModes == 1 ) dwRenderTarget = GGFMT_A16B16G16R16F;
			if ( iTryModes == 2 ) dwRenderTarget = GGFMT_G32R32F;
			if ( iTryModes == 3 ) dwRenderTarget = GGFMT_R32F;
			if ( iTryModes == 4 ) dwRenderTarget = GGFMT_G16R16F;
			if ( iTryModes == 5 ) dwRenderTarget = GGFMT_R16F;
			if ( iTryModes == 6 ) dwRenderTarget = GGFMT_A8R8G8B8;
			D3DXCreateTexture( m_pD3D, desc.Width, desc.Height, 1, GGUSAGE_RENDERTARGET, dwRenderTarget, D3DPOOL_DEFAULT, &g_pMainCameraDepthTexture );
			iTryModes++;
		}
		g_pMainCameraDepthTexture->GetSurfaceLevel ( 0, &g_pMainCameraDepthTextureSurfaceRef );
		m_pD3D->CreateDepthStencilSurface( desc.Width, desc.Height, GetValidStencilBufferFormat(desc.Format), D3DMULTISAMPLE_NONE, 0, TRUE, &g_pMainCameraDepthStencilTexture, NULL );
	}

	// copy to debug image from last usage
	if ( g_pMainCameraDepthTexture && g_pMainCameraDepthTextureSurfaceRef && rand()%100==1 )
	{
		LPGGTEXTURE pDebugSee = GetImagePointer ( 59949 );
		if ( pDebugSee )
		{
			if ( g_pMainCameraDepthTextureSurfaceRef )
			{
				LPGGSURFACE pShadowDebugImage;
				pDebugSee->GetSurfaceLevel ( 0, &pShadowDebugImage );
				if ( pShadowDebugImage )
				{
					HRESULT hRes = D3DXLoadSurfaceFromSurface ( pShadowDebugImage, NULL, NULL, g_pMainCameraDepthTextureSurfaceRef, NULL, NULL, D3DX_DEFAULT, 0 );
					pShadowDebugImage->Release();
				}
			}
		}
	}

	// we channel all renders to a special depth texture render target
	m_pD3D->SetRenderTarget( 0, g_pMainCameraDepthTextureSurfaceRef );
	m_pD3D->SetDepthStencilSurface( g_pMainCameraDepthStencilTexture );

	// clear if first render of this cycle
	if ( g_bFirstRenderClearsRenderTarget==false )
	{
		GGCOLOR color = GGCOLOR_RGBA(0, 0, 0, 0);
		m_pD3D->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
		g_bFirstRenderClearsRenderTarget = true;
	}
	#endif

	// always success for now
	return 1;
}

bool CObjectManager::DrawObjectEx ( sObject* pObject )
{
	return DrawObject ( pObject, false );
}

bool CObjectManager::DrawObject ( sObject* pObject, bool bFrustrumCullMeshes, bool recursive )
{
	// each object resets this (instance object can set it)
	g_InstanceAlphaControlValue = 0;

	// check the object and array index value
	SAFE_MEMORY ( pObject );

	// if resource destroyed, quit now
	if ( pObject->dwObjectNumber > 0 )
		if ( g_ObjectList [ pObject->dwObjectNumber ]==NULL )
			return true;

	// skip if object is designated as invisible
	// changed this to let universe visible through for shadows
	if ( pObject->bVisible==false || pObject->bNoMeshesInObject==true ) //|| pObject->bExcludedEarly )
		return true;

	// setup the world matrix for the object
	CalcObjectWorld ( pObject );

	// Simple hardware occlusion to determine on-screen visibility (in rendered pixels)
	bool bOcclusionRenderHappened = false;
	//PE: Checkup
	//PE: pObject->dwObjectNumber > 70000 ?
	if ( g_Occlusion.d3dQuery[pObject->dwObjectNumber]!=NULL )
	{
		if ( g_Occlusion.GetOcclusionMode()==1 )
		{
			if ( g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber]==99 )
			{
				// 99 = must be triggered by GET OBJECT OCCLUSION (not every cycle = too expensive)
				#ifdef DX11
				#else
				g_Occlusion.d3dQuery[pObject->dwObjectNumber]->Issue( D3DISSUE_BEGIN );
				#endif
				g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber] = 1;
			}
		}
	}

	// only render if universe believes it visible (or rendering for shadow camera)
	if ( pObject->bUniverseVisible==true || g_pGlob->dwRenderCameraID>=31 )
	{
		// Occlusion Render Happened
		bOcclusionRenderHappened = true;

		// for linked objects
		GGMATRIX matSavedWorld;
		GGMATRIX matNewWorld;

		// external DLLs can disable transform it they want
		if ( !pObject->bDisableTransform )
		{
			GGSetTransform ( GGTS_WORLD, &pObject->position.matWorld );
		}
		else
		{
			// World Transform
			GGMATRIX matTranslation, matScale, matObject;
			GGMatrixTranslation ( &matTranslation, 0.0f, 0.0f, 0.0f );
			GGMatrixScaling     ( &matScale,       1.0f, 1.0f, 1.0f );
			matObject = matScale * matTranslation;
			GGSetTransform ( GGTS_WORLD, &matObject );
		}

		// an object can control its own FOV for rendering
		// 171117 - interferes with global control of FOV
		// but needed for Weapon FOV
		if ( pObject->bLockedObject==false )
		{
			if ( pObject->fFOV != m_RenderStates.fObjectFOV )
			{
				if ( pObject->fFOV == 0.0f )
					SetCameraFOV ( m_RenderStates.fStoreCameraFOV );
				else
					SetCameraFOV ( pObject->fFOV );

				m_RenderStates.fObjectFOV = pObject->fFOV;
			}
		}

		// get LOD flag from core object
		int iUsingWhichLOD = pObject->iUsingWhichLOD;

		// actual object or instance of object
		sObject* pActualObject = pObject;

		// if object uses bInstanceAlphaOverride, we might be doing a per-object shader operation
		if ( pObject->bInstanceAlphaOverride )
		{
			// allows drawmesh to know if this instance is a per-instance changer
			g_InstanceAlphaControlValue = pObject->dwInstanceAlphaOverride;
#ifndef PRODUCTCLASSIC
			if (1)
			{
				//Disable markers.
				//t.entityprofile[t.tentid].ismarker != 0 then pObject->dwCameraMaskBits = 1

				if (pObject->dwCameraMaskBits != 1 ) { // && pObject->dwTechniqueSupport & SupportTechniqueOutLine) {
					DWORD checkcol = g_InstanceAlphaControlValue & 0x00FFFFFF; // mask out alpha and only react if using colors.
					if (checkcol > 0 && g_pGlob->dwRenderCameraID < 30 && !recursive) {
						GGVECTOR3 oldscale,oldpos;
						//GGVECTOR3 oldpos,oldrot,oldveclook,oldvecup;
						//bool oldbFreeFlightRotation;
						oldpos = pObject->position.vecPosition;
						//oldrot = pObject->position.vecRotate;
						//oldscale = pObject->position.vecScale;
						//oldveclook = pObject->position.vecLook;
						//oldvecup = pObject->position.vecUp;
						//oldbFreeFlightRotation = pObject->position.bFreeFlightRotation;

//						//Outline test.
//						//move toward camera.
//						tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData(g_pGlob->dwCurrentSetCameraID);
//						pObject->position.vecPosition -= (20.0f * m_Camera_Ptr->vecLook);
//						//pObject->position.vecScale = oldscale * 1.05; //pObject->position.vecPosition.z -= 29.0;
//						g_UseOutLine = 1;
//						DrawObject(pObject, bFrustrumCullMeshes, true);
//						pObject->position.vecPosition = oldpos;
//						g_UseOutLine = 0;

						g_UseWireFrame = 1;
						DrawObject(pObject, bFrustrumCullMeshes, true);
						g_UseWireFrame = 0;
						//pObject->position.vecScale = oldscale;
						pObject->bInstanceAlphaOverride = false;
						DrawObject(pObject, bFrustrumCullMeshes, true);
						pObject->bInstanceAlphaOverride = true;
						g_InstanceAlphaControlValue = 0;
						return true;
					}
				}
			}
#endif
		}

		if ( pObject->pInstanceOfObject )
		{
			// get actual object via instance ptr
			pActualObject=pActualObject->pInstanceOfObject;

			// if instance uses alpha factor, apply to object
			#ifdef DX11
			#else
			if ( pObject->bInstanceAlphaOverride )
			{
				// if mesh exists with blending and argument mode, set the individual instance alpha value
				if ( pActualObject->iMeshCount > 0 )
				{
					sMesh* pMesh = pActualObject->ppMeshList [ 0 ];
					if ( pMesh->dwTextureCount > 0 )
					{
						m_pD3D->SetTextureStageState ( 0, D3DTSS_ALPHAOP, pMesh->pTextures [ 0 ].dwBlendMode );
						m_pD3D->SetTextureStageState ( 0, D3DTSS_ALPHAARG2, pMesh->pTextures [ 0 ].dwBlendArg1 );
						m_pD3D->SetTextureStageState ( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
						m_pD3D->SetRenderState( D3DRS_TEXTUREFACTOR, pObject->dwInstanceAlphaOverride );
					}
				}
 				m_RenderStates.bNoMeshAlphaFactor = true;
			}
			#endif
		}

		// store current object distance globally so DrawMesh can set Shader technique based on distance (SET OBJECT LOD TECHNIQUE)
		g_fObjectCamDistance = pObject->position.fCamDistance;

		// first identify if 'pInstanceMeshVisible' limbs should be visible/invisible
		if ( pObject->iUsingWhichLOD==-1000 )
		{
			// LIMB VISIBILITY LOD STYLE
			if ( pObject->pInstanceOfObject )
			{
				// INSTANCE OBJECT
				pObject->pInstanceMeshVisible [ pObject->iLOD0LimbIndex ] = false;
				pObject->pInstanceMeshVisible [ pObject->iLOD1LimbIndex ] = false;
				pObject->pInstanceMeshVisible [ pObject->iLOD2LimbIndex ] = false;
				if ( pObject->position.fCamDistance > pObject->fLODDistance[1] )
				{
					// furthest
					pObject->pInstanceMeshVisible [ pObject->iLOD2LimbIndex ] = true;
				}
				else
				{
					if ( pObject->position.fCamDistance > pObject->fLODDistance[0] )
					{
						// mid-way
						pObject->pInstanceMeshVisible [ pObject->iLOD1LimbIndex ] = true;
					}
					else
					{
						// closest
						pObject->pInstanceMeshVisible [ pObject->iLOD0LimbIndex ] = true;
					}
				}
			}
		}

		// run through all of the frames within the object
		if ( pActualObject->ppFrameList )
		{
			for ( int iFrame = 0; iFrame < pActualObject->iFrameCount; iFrame++ )
			{
				// if instance limb visibility hidden, skip now
				if ( pObject->pInstanceMeshVisible )
				{
					// if limb in instance hidden, skip
					if ( pObject->pInstanceMeshVisible [ iFrame ]==false )
						continue;
				}

				// get a pointer to the frame
				sFrame* pFrame = pActualObject->ppFrameList [ iFrame ];

				// 301007 - new limb excluder
				if ( pFrame==NULL ) continue;
				if ( pFrame->bExcluded==true ) continue;

				// get mesh from frame
				sMesh* pMesh = pFrame->pMesh;

				// calculate correct absolute world matrix
				CalculateAbsoluteWorldMatrix ( pObject, pFrame, pMesh );

				// if flagged, reject meshes outside of camera frustrum (bVeryEarlyObject prevents sky meshes from disappearing)
				if ( bFrustrumCullMeshes==true && pObject->bVeryEarlyObject==false )
				{
					if ( pMesh )
					{
						GGVECTOR3 vecCentre = pMesh->Collision.vecCentre;
						GGVec3TransformCoord ( &vecCentre, &vecCentre, &pFrame->matAbsoluteWorld );
						GGVECTOR3 vecDirection = vecCentre - pObject->position.vecPosition;
						GGVECTOR3 vecCamDirection = m_pCamera->vecLook;
						GGVec3Normalize ( &vecDirection, &vecDirection );
						GGVec3Normalize ( &vecCamDirection, &vecCamDirection );
						float fDot = GGVec3Dot ( &vecDirection, &vecCamDirection );
						if ( fDot<=-0.6f ) // at 200 HFOV can see these!
							continue;
					}
				}

				// draw mesh
				if ( pMesh )
				{
					// apply the final transform
					if ( !pMesh->bLinked )
					{
						// new matrix for completely custom, physics needs this for implementing it's own matrix
						if ( pFrame->bOverride )
						{
							GGSetTransform ( GGTS_WORLD, ( GGMATRIX* ) &pFrame->matOverride );
						}
						else if ( !pObject->bDisableTransform )
						{
							GGSetTransform ( GGTS_WORLD, ( GGMATRIX* ) &pFrame->matAbsoluteWorld );
						}
					}

					// LOD System
					sMesh* pCurrentLOD = pMesh;
					if ( pObject->iUsingWhichLOD==-1000 )
					{
						// LIMB VISIBILITY LOD STYLE
						if ( pObject->pInstanceOfObject )
						{
							// done above, before we enter frame loop
						}
						else
						{
							// ACTUAL OBJECT
							sMesh* pMeshLOD0 = pObject->ppFrameList[pObject->iLOD0LimbIndex]->pMesh;
							sMesh* pMeshLOD1 = pObject->ppFrameList[pObject->iLOD1LimbIndex]->pMesh;
							sMesh* pMeshLOD2 = pObject->ppFrameList[pObject->iLOD2LimbIndex]->pMesh;
							if ( pMeshLOD0 ) pMeshLOD0->bVisible = false;
							if ( pMeshLOD1 ) pMeshLOD1->bVisible = false;
							if ( pMeshLOD2 ) pMeshLOD2->bVisible = false;
							if ( pObject->position.fCamDistance > pObject->fLODDistance[1] )
							{
								// furthest
								if ( pMeshLOD2) pMeshLOD2->bVisible = true;
							}
							else
							{
								if ( pObject->position.fCamDistance > pObject->fLODDistance[0] )
								{
									// mid-way
									if ( pMeshLOD1 ) pMeshLOD1->bVisible = true;
								}
								else
								{
									// closest
									if ( pMeshLOD0 ) pMeshLOD0->bVisible = true;
								}
							}
							if ( pMeshLOD1==NULL && pMeshLOD2==NULL && pMeshLOD0 ) pMeshLOD0->bVisible = true;
						}
					}
					else
					{
						// MESH SELECT LOD STYLE

						// update world transform for LOD quad (always faces camera)
						GGMATRIX matQuadRotation;

						// u74b7 - moved to UpdateLayer
						if ( (pObject->iUsingOldLOD==3 || pObject->iUsingWhichLOD==3) && m_pCamera )
						{
							float dx = pObject->position.vecPosition.x - m_pCamera->vecPosition.x;
							float dz = pObject->position.vecPosition.z - m_pCamera->vecPosition.z;
							float theangle = atan2 ( dx, dz );
							GGMatrixRotationY(&matQuadRotation, theangle );
							matQuadRotation._41 = pFrame->matAbsoluteWorld._41;
							matQuadRotation._42 = pFrame->matAbsoluteWorld._42;
							matQuadRotation._43 = pFrame->matAbsoluteWorld._43;
						}

						// leeadd - U71 - determine LOD meshes (current and old (transition) if applicable)
						if ( iUsingWhichLOD==1 && pFrame->pLOD[0] ) pCurrentLOD = pFrame->pLOD[0];
						if ( iUsingWhichLOD==2 && pFrame->pLOD[1] ) pCurrentLOD = pFrame->pLOD[1];
						if ( iUsingWhichLOD==3 && pFrame->pLODForQUAD ) pCurrentLOD = pFrame->pLODForQUAD;
						sMesh* pOldLOD = NULL;
						if ( pObject->iUsingOldLOD != -1 )
						{
							// the old lod mesh
							if ( pObject->iUsingOldLOD==0 ) pOldLOD = pMesh;
							if ( pObject->iUsingOldLOD==1 && pFrame->pLOD[0] ) pOldLOD = pFrame->pLOD[0];
							if ( pObject->iUsingOldLOD==2 && pFrame->pLOD[1] ) pOldLOD = pFrame->pLOD[1];
							if ( pObject->iUsingOldLOD==3 && pFrame->pLODForQUAD ) pOldLOD = pFrame->pLODForQUAD;

							// transition in progress from OLD to CURRENT
							pObject->fLODTransition += 0.03f;
							if ( pObject->fLODTransition >= 2.0f )
							{
								// end transition and restore alpha states
								pObject->fLODTransition = 0.0f;
								pObject->iUsingOldLOD = -1;
								GGCOLOR dwAlphaValueOnly = GGCOLOR_ARGB ( 255, 0, 0, 0 );
								pCurrentLOD->dwAlphaOverride = dwAlphaValueOnly;
								pCurrentLOD->bAlphaOverride = false;
								pCurrentLOD->bZWrite = true;
								pCurrentLOD->bZBiasActive = false; // U74 - 120409 - refresh each cycle for each instance
								pCurrentLOD->fZBiasDepth = 0.0f; // U74 - 120409 - refresh each cycle for each instance
								pOldLOD->dwAlphaOverride = dwAlphaValueOnly;
								pOldLOD->bAlphaOverride = false;
								pOldLOD->bZWrite = true;
								pOldLOD->bZBiasActive = false;
								pOldLOD->fZBiasDepth = 0.0f;
								pOldLOD = NULL;

								// U72 - 100109 - record alpha state of this mesh (for when instance is not being calculated, i.e. updated in actual mesh for render state change)
								if ( pObject->pInstanceOfObject ) pObject->dwInstanceAlphaOverride = dwAlphaValueOnly;
							}
							else
							{
								// change alpha level of meshes involved in transition
								if ( pObject->fLODTransition<=1.0f )
								{
									// FIRST fade in current LOD mesh
									DWORD dwAlpha = (DWORD)(pObject->fLODTransition*255);
									GGCOLOR dwAlphaValueOnly = GGCOLOR_ARGB ( dwAlpha, 0, 0, 0 );
									pCurrentLOD->dwAlphaOverride = dwAlphaValueOnly;
									pCurrentLOD->bAlphaOverride = true;
									pCurrentLOD->bTransparency = true;

									// AND first bit of fade in switch off Zwrite so the 'appearing' image goes not clip the
									// new current mesh and other objects in the area (causing the background to come through)
									if ( pObject->iUsingWhichLOD!=3 )
									{
										if ( pObject->fLODTransition < 0.5f )
											pCurrentLOD->bZWrite = false;
										else
											pCurrentLOD->bZWrite = true;

										pCurrentLOD->bZBiasActive = false;
										pCurrentLOD->fZBiasDepth = 0.0f;
									}
									else
									{
										// last LODQUAD mesh is a plane, so can adjust bias ahead to ensure it is rendered ABOVE everything
										pCurrentLOD->bZWrite = false;
										pCurrentLOD->fZBiasDepth = -g_fZBiasEpsilon;
										pCurrentLOD->bZBiasActive = true;
									}

									// and OLD LOD must stay as reset
									if ( pOldLOD )
									{
										pOldLOD->dwAlphaOverride = GGCOLOR_ARGB ( 255, 0, 0, 0 );
										pOldLOD->bAlphaOverride = false;
										pOldLOD->bZWrite = true;
										pOldLOD->bZBiasActive = false;
										pOldLOD->fZBiasDepth = 0.0f;
									}
								}
								else
								{
									// Ensure current LOD mesh is default (writing Z and no bias)
									pCurrentLOD->dwAlphaOverride = GGCOLOR_ARGB ( 255, 0, 0, 0 );
									pCurrentLOD->bAlphaOverride = false;
									pCurrentLOD->bZWrite = true;
									pCurrentLOD->bZBiasActive = false;
									pCurrentLOD->fZBiasDepth = 0.0f;

									// AND now as OLD one fades away, push zbias so NEW/CURRENT mesh has all of Z buffer opportunity
									if ( pObject->iUsingOldLOD!=3 )
									{
										// except the last LODQUAD, which needs zbias as is to do proper fade out
										pOldLOD->fZBiasDepth = g_fZBiasEpsilon * (pObject->fLODTransition-1.0f);
										pOldLOD->bZBiasActive = true;

										// AND last bit of fade out switch off Zwrite so the 'almost gone' image goes not clip the
										// new current mesh and other objects in the area (causing the background to come through)
										if ( pObject->fLODTransition > 1.5f )
											pOldLOD->bZWrite = false;
										else
											pOldLOD->bZWrite = true; // U74 - 120409 - refresh each cycle for each instance
									}
									else
									{
										// For the last LOD QUAD, make the decal fade out slower (to avoid the flick against the sky)
										pObject->fLODTransition -= 0.01f;

										// U74 - 120409 - no zbias effect
										pOldLOD->bZBiasActive = false;
										pOldLOD->fZBiasDepth = 0.0f;

										// also disable ALL zwrites from LODQUAD to avoid artefacts
										pOldLOD->bZWrite = false;
									}

									// THEN fade out old LOD mesh
									DWORD dwAlpha = (DWORD)((2.0f-pObject->fLODTransition)*255);
									GGCOLOR dwAlphaValueOnly = GGCOLOR_ARGB ( dwAlpha, 0, 0, 0 );
									pOldLOD->dwAlphaOverride = dwAlphaValueOnly;
									pOldLOD->bAlphaOverride = true;
									pOldLOD->bTransparency = true;
								}
							}
						}
						else
						{
							// U72 - 100109 - mesh not in transition, but still need the alpha state if this is an instanced object
							if ( pObject->pInstanceOfObject && pCurrentLOD )
							{
								pCurrentLOD->dwAlphaOverride = pObject->dwInstanceAlphaOverride;
								pCurrentLOD->bAlphaOverride = false;

								// 010917 - commented this out as its not relevant to latest engine and corrupts by disabled Zwrite flag state!
								//pCurrentLOD->bZWrite = true;
								//pCurrentLOD->bZBiasActive = false;
								//pCurrentLOD->fZBiasDepth = 0.0f;
							}
						}

						// if in transition, draw OLD first
						if ( pOldLOD )
						{
							if ( pObject->iUsingOldLOD==3 )
							{
								GGSetTransform ( GGTS_WORLD, ( GGMATRIX* ) &matQuadRotation );
							}
							else
							{
								GGSetTransform ( GGTS_WORLD, ( GGMATRIX* ) &pFrame->matAbsoluteWorld );
							}

							// draw old LOD mesh
							//DrawMesh(pOldLOD);
							DrawMesh ( pOldLOD , false , pObject, pFrame); //PE: Need the object for new dyn light to work.
							//if ( !DrawMesh ( pOldLOD ) )
							//	return false;

							// restore projection matrix
							if ( pObject->iUsingWhichLOD!=3 )
								GGSetTransform ( GGTS_WORLD, ( GGMATRIX* ) &pFrame->matAbsoluteWorld );
						}

						// update world transform for LOD quad (always faces camera)
						if ( pObject->iUsingWhichLOD==3 )
							GGSetTransform ( GGTS_WORLD, ( GGMATRIX* ) &matQuadRotation );
					}

					// draw the current mesh

					bool oldbZWrite,oldbZRead;
					if (recursive && g_UseOutLine) {
						oldbZWrite = pCurrentLOD->bZWrite;
						oldbZRead = pCurrentLOD->bZRead;
						pCurrentLOD->bZWrite = false;
						pCurrentLOD->bZRead = true;
					}

					if ( !DrawMesh ( pCurrentLOD, (pObject->pInstanceMeshVisible!=NULL) , pObject , pFrame) )
					{
						// mesh failed to draw - catch it here to investigate strangeness
						int lee = 42;
					}

					if (recursive && g_UseOutLine) {
						pCurrentLOD->bZWrite = oldbZWrite;
						pCurrentLOD->bZRead = oldbZRead;
					}

					//if ( !DrawMesh ( pCurrentLOD, (pObject->pInstanceMeshVisible!=NULL) , pObject ) )
					//	return false;

					// for linked objects
					if ( pMesh->bLinked )
					{
						if ( !pObject->bDisableTransform )
						{
							GGGetTransform ( GGTS_WORLD, &matSavedWorld );
							matNewWorld = pFrame->matOriginal * matSavedWorld;
							GGSetTransform ( GGTS_WORLD, &matNewWorld );
						}
					}
				}
			}
		}

		// if instance uses alpha factor, apply to object
		m_RenderStates.bNoMeshAlphaFactor = false;
		m_RenderStates.bIgnoreDiffuse = false;
	}

	// calculate object visibility based on hardware occlusion
	if ( g_Occlusion.d3dQuery[pObject->dwObjectNumber]!=NULL )
	{
		if ( g_Occlusion.GetOcclusionMode()==1 )
		{
			if ( g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber]==1 )
			{
				#ifdef DX11
				#else
				g_Occlusion.d3dQuery[pObject->dwObjectNumber]->Issue( D3DISSUE_END );
				#endif
				if ( bOcclusionRenderHappened==false )
				{
					// cancel query now, no render at all between issue phase means corrupt result!
					g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber] = 0;
				}
				else
				{
					g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber] = 2;
				}
			}
			if ( g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber]==2 )
			{
				DWORD pixelsVisible = 0;
				#ifdef DX11
				#else
				HRESULT dwResult = g_Occlusion.d3dQuery[pObject->dwObjectNumber]->GetData((void *)&pixelsVisible, sizeof(DWORD), 0);
				if ( dwResult==S_OK )
				{
					// get pixels result
					g_Occlusion.dwQueryValue[pObject->dwObjectNumber] = pixelsVisible;
					g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber] = 0;
				}
				else
				{
					if ( dwResult!=S_FALSE )
					{
						// cancel whole thing if error returned
						g_Occlusion.iQueryBusyStage[pObject->dwObjectNumber] = 0;
					}
				}
				#endif
			}
		}
	}

	// sorted, return back
	return true;
}

bool CObjectManager::PostDrawRestores ( void )
{
	#ifdef DX11
	//  ensure FOV is restored
	if ( g_pGlob->dwRenderCameraID == 0 )
	{
		if ( m_RenderStates.fObjectFOV != 0.0f )
		{
			SetCameraFOV ( m_RenderStates.fStoreCameraFOV );
			m_RenderStates.fObjectFOV = 0.0f;
		}
	}
	#else
	// cleanup render states before leave draw process
	m_pD3D->SetPixelShader ( 0 );

	// fixed function blending restores
	DWORD dwMaxTextureStage = 7;
	for ( DWORD dwTextureStage = 0; dwTextureStage < dwMaxTextureStage; dwTextureStage++ )
	{
		// texture filter modes
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MAGFILTER, GGTEXF_LINEAR );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MINFILTER, GGTEXF_LINEAR );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MIPFILTER, GGTEXF_LINEAR );

		// texture coordinate data
		m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXCOORDINDEX, dwTextureStage );

		// texture blending modes
		if ( dwTextureStage==0 )
		{
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLOROP, GGTOP_MODULATE );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG1, GGTA_TEXTURE );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG2, GGTA_DIFFUSE );
		}
		else
		{
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLOROP, GGTOP_DISABLE );
		}
	}

    // must always restore ambient level (for layers)
	m_pD3D->SetRenderState ( D3DRS_AMBIENT, m_RenderStates.dwAmbientColor );

	// leeadd - 140304 - ensure FOV is restored
	if ( m_RenderStates.fObjectFOV != 0.0f )
	{
		SetCameraFOV ( m_RenderStates.fStoreCameraFOV );
		m_RenderStates.fObjectFOV = 0.0f;
	}

    if ( g_pGlob && g_pGlob->iFogState == 1 )
    {
	    m_pD3D->SetRenderState ( D3DRS_FOGENABLE, TRUE );
    	m_pD3D->SetRenderState ( D3DRS_FOGCOLOR, m_RenderStates.dwFogColor );
    }
	#endif

    // okay
	return true;
}

bool CObjectManager::Reset ( void )
{
	// set values to default
	m_iCurrentTexture		= -22000000;	// current texture being used
	m_iLastTexture			= -22000000;	// last texture being used
	m_dwLastTextureCount	= 0;
	m_dwCurrentShader		= 0;
	m_dwCurrentFVF			= 0;			// current FVF
	m_dwLastShader			= 0;
	m_dwLastFVF				= 0;			// previous FVF
	m_bUpdateVertexDecs		= true;			// reset update vertex settings 
	m_bUpdateStreams		= true;			// reset update stream sources every frame

	// leefix - 200303 - reset cullmode from various stencil effects
	m_RenderStates.dwCullDirection				= m_RenderStates.dwGlobalCullDirection;
	m_RenderStates.bCull						= true;
	m_RenderStates.iCullMode					= 0;	
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_CULLMODE,	m_RenderStates.dwCullDirection );
	#endif

	// U75 - 070410 - added new render state to control whether entire render is blanked to a color
	tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwRenderCameraID );
	if ( m_Camera_Ptr ) m_RenderStates.dwOverrideAllWithColor = m_Camera_Ptr->dwForegroundColor;
	m_RenderStates.bOverrideAllTexturesAndEffects = false;
	if ( m_RenderStates.dwOverrideAllWithColor != 0 ) m_RenderStates.bOverrideAllTexturesAndEffects = true;
	m_RenderStates.bOverriddenClipPlaneforHLSL = false;

	return true;
}

void CObjectManager::UpdateViewProjForMotionBlur(void)
{
	// 270515 - record current viewproj (for working out previous viewproj for motion blur)
	tagCameraData* m_pCamera = (tagCameraData*)GetCameraInternalData ( 0 );
	if ( m_pCamera )
	{
		g_matPreviousViewProj = g_matThisViewProj;
		GGMATRIX matView, matProj;
	    g_matThisViewProj = m_pCamera->matView * m_pCamera->matProjection;
		g_matThisCameraView = m_pCamera->matView;
	}
}

float rx, ry, rz;
float px = -999.0f;
float py,pz;

void CObjectManager::UpdateInitOnce ( void )
{
	// can skip some operations when in VR (reflection camera and right eye camera)
	bool bSkipRepeatedWorkloads = false;
	//if ( g_pGlob->dwRenderCameraID == 3 || g_pGlob->dwRenderCameraID == 7 )
	if ( g_pGlob->dwRenderCameraID == 7 )
		bSkipRepeatedWorkloads = true;

	// ensure that the D3D device is valid
	if ( !m_pD3D )
		return;

	// replace any buffers if object modified (from mesh size change or limb mods)
	if ( !m_ObjectManager.ReplaceAllFlaggedObjectsInBuffers() )
		return;

	//PE: Start mesh light system.
	start_mesh_light();

	// Sort is sort of expensive
	if ( bSkipRepeatedWorkloads == false ) SortTextureList();

    // get camera data into member variable
	m_pCamera = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwRenderCameraID );
	if ( m_pCamera )
	{
		// projection matrix
		GGMATRIX matProj = m_pCamera->matProjection;
		GGSetTransform ( GGTS_PROJECTION, &matProj );

		// regular or reflected (clip mode has reflection modes at 2 and 4)
		if ( m_pCamera->iClipPlaneOn==2 || m_pCamera->iClipPlaneOn==4 )
		{
			// Reflect camera view in clip plane (mirror)
			GGMATRIX matView, matReflect;
			GGMatrixReflect ( &matReflect, &m_pCamera->planeClip );
			GGMatrixMultiply ( &matView, &matReflect, &m_pCamera->matView );
			GGSetTransform ( GGTS_VIEW, &matView );
		}
		else
		{
			// Regular camera view
			GGMATRIX matView = m_pCamera->matView;
			GGSetTransform ( GGTS_VIEW, &matView );
		}
	}

	// setup the viewing frustum data
	if ( !SetupFrustum ( 0.0f ) )
		return;

	// only need to do this for camera zero and six really
	if ( bSkipRepeatedWorkloads == false )
	{
		// setup the visibility list (sort of expensive)
		if ( !SortVisibilityList ( ) )
			return;

		// update only those that are visible
		if ( !m_ObjectManager.UpdateOnlyVisible() )
			return;

		// refresh all data in VB (from any vertex changes in objects)
		if ( !m_ObjectManager.UpdateAllObjectsInBuffers() )
			return;
	}

	// can render even earlier in pipeline, so this can be flagged to happen earlier in UpdateOnce
	g_bScenePrepared = false;

	// 270515 - helps clear depth texture render target each cycle
	g_bFirstRenderClearsRenderTarget = false;

	// some objects need to be rendered before ANYTHING (camera, light, matrix, terrain even stencilstart)
	// ideal for things like sky boxes that do not interfere with the Z buffer for scene fidelity
	if ( g_bRenderVeryEarlyObjects==true )
	{
		// Prepare main render
		UpdateInit();

		// prepare initial scene states
		if ( !PreSceneSettings ( ) )
			return;

		// render VERY EARLY objects (such as sky)
		UpdateLayer ( -1 );

		// scene prepared
		g_bScenePrepared = true;
	}

	// okay
	return;
}

bool CObjectManager::UpdateInit ( void )
{
	// ensure that the D3D device is valid
	if ( !m_pD3D )
		return false;

	// reset values
	if ( !Reset ( ) )
		return false;
	
	// okay
	return true;
}

// calculate distance from object to camera
float CObjectManager::CalculateObjectDistanceFromCamera ( sObject* pObject )
{
    // u74b8 - If the camera isn't selected, just use main camera
    //if (!m_pCamera) m_pCamera = 
	// lee - 280714 - always use camera zero for distance check (as can only store one in object structure)
	tagCameraData* pCamera = (tagCameraData*) GetCameraInternalData(0);

    // u74b8 - Use current camera position as the start point, or 0,0,0 for a locked object
    GGVECTOR3 vecCamPos = (pObject->bLockedObject) ? GGVECTOR3(0,0,0) : pCamera->vecPosition;

    // u74b8 - Follow glued objects until you reach the end of the glue-chain, or a
    // glued object that no longer exists.
    while ( pObject->position.iGluedToObj != 0 )
    {
        if (g_ObjectList [ pObject->position.iGluedToObj ] == NULL)
        {
            // Glued to an object that does not exist, so break the chain
            pObject->position.iGluedToObj = 0;
            break;
        }

        pObject = g_ObjectList [ pObject->position.iGluedToObj ];
    }

    float fdx = pObject->position.vecPosition.x - vecCamPos.x;
    float fdy = pObject->position.vecPosition.y - vecCamPos.y;
    float fdz = pObject->position.vecPosition.z - vecCamPos.z;

	return sqrt ( (fdx * fdx) + (fdy * fdy) + (fdz * fdz) );
}

bool CObjectManager::UpdateLayer ( int iLayer )
{
	// if resources destroyed, quit now
	if ( GetSortedObjectVisibleList()==NULL )
		return true;

	// prepare render states to draw
	if ( !PreDrawSettings ( ) )
		return false;

	//PE: Set current layer being redered.
	setlayer_mesh_light(iLayer);

	bool Status = UpdateLayerInner(iLayer);

    // restore render states after draw
	if ( !PostDrawRestores ( ) )
		return false;

    return Status;
}

bool CObjectManager::UpdateLayerInner ( int iLayer )
{
	// work vars
	int iObject = 0;
	static int iOnlyOneSortPerSync = 0;
	bool bUseStencilWrite=false;
	GGVECTOR3 vecShadowPos;

	// if sync mask override active, reject any drawing activity
	if ( g_dwSyncMaskOverride == 0 ) return true;

    // Get camera information for LOD and distance calculation
	// ensure rendercamera of 31-34 selects mask for camera 31 (shadow camera)
    DWORD dwCurrentCameraBit;
	if ( g_pGlob->dwRenderCameraID<31 )
		dwCurrentCameraBit = 1 << g_pGlob->dwRenderCameraID;
	else
		dwCurrentCameraBit = 1 << 31;

	// run through all visible objects and draw them (unrolled for performance)
	switch ( iLayer )
	{
	case -1 : // Very Early Objects (rendered even before StencilStart)
		{
			iOnlyOneSortPerSync = 0;
			// choose camera to render sky (and other early objects) to (used by cube map generator)
			int iPreferredCamera = 0;
			if ( g_pGlob->dwRenderCameraID == 30 ) iPreferredCamera = 30;

			// reset to default camera range for noz and locked objects
			float fCurrentNearRange = 0.0f;
			float fCurrentFarRange = 0.0f;
			bool bCameraRangeAndProjectionChanged = false;
			if ( g_pGlob->dwRenderCameraID != 6 && g_pGlob->dwRenderCameraID != 7 )
			{
				// except for cameras 6 and 7 which are VR eye cameras and have their own projection matrix (which should not be overwritten by SetCameraRange)
				tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData( iPreferredCamera );
				fCurrentNearRange = m_Camera_Ptr->fZNear;
				fCurrentFarRange = m_Camera_Ptr->fZFar;
				SetCameraRange ( iPreferredCamera, 1, 70000 );
				bCameraRangeAndProjectionChanged = true;
			}
			if ( ! m_vVisibleObjectEarly.empty() )
			{
				for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectEarly.size(); ++iIndex )
				{
					sObject* pObject = m_vVisibleObjectEarly [ iIndex ];

					// leeadd - 211006 - u63 - ignore objects whose masks reject the current camera
					if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
						continue;

					// leeadd - 240106 - if any LOD activity
					// u74b8 - avoid recalculation of distance if already sorted by distance
					if ( pObject->bHadLODNeedCamDistance && g_eGlobalSortOrder != E_SORT_BY_DEPTH)
						pObject->position.fCamDistance = CalculateObjectDistanceFromCamera ( pObject );

					// call the draw function
					DrawObject ( pObject, true );
				}
			}
			// restore camera range
			if ( bCameraRangeAndProjectionChanged == true )
			{
				// except for cameras 6 and 7 which are VR eye cameras and have their own projection matrix (which should not be overwritten by SetCameraRange)
				SetCameraRange ( iPreferredCamera, fCurrentNearRange, fCurrentFarRange );
			}
		}
		break;

	case 0 : // Main Layer
		iOnlyOneSortPerSync = 0; //-1 not always called.
        if ( ! m_vVisibleObjectStandard.empty() )
        {
            for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectStandard.size(); ++iIndex )
            {
                sObject* pObject = m_vVisibleObjectStandard [ iIndex ];

				// ignore objects whose masks reject the current camera
				if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
					continue;

				// do not render static objects
				if ( pObject->bStatic )
					continue;

				// or stencil objects
				if ( pObject->bReflectiveObject )
					continue;

				// leeadd - 240106 - if any LOD activity
                // u74b8 - avoid recalculation of distance if already sorted by distance
                if ( pObject->bHadLODNeedCamDistance && g_eGlobalSortOrder != E_SORT_BY_DEPTH)
					pObject->position.fCamDistance = CalculateObjectDistanceFromCamera ( pObject );

				// call the draw function
				//if ( !DrawObject ( pObject, false ) )
				//	return false;
				DrawObject ( pObject, false );
			}
        }
        break;

	case 3 : // Overlay Ghost Layer (in stages)

        if ( ! m_vVisibleObjectTransparent.empty() )
        {
			if (iOnlyOneSortPerSync++ == 0) {

				// leeadd - 021205 - new feature which can divide transparent depth-sorted objects by a water
				// line so everything below is rendered, then the water, then everything at normal surface order
				bool bWaterPlaneDivision = false;
				float fWaterPlaneDivisionY = 99999.99f;

				// get list of ghosted objects for depth sort
				for (DWORD iIndex = 0; iIndex < m_vVisibleObjectTransparent.size(); ++iIndex)
				{
					sObject* pObject = m_vVisibleObjectTransparent[iIndex];
					if (!pObject) continue;

					// leeadd - 211006 - u63 - ignore objects whose masks reject the current camera
					if ((pObject->dwCameraMaskBits & dwCurrentCameraBit) == 0)
						continue;

					// calculate distance from object to camera (fills fCamDistance)
					if (pObject->bTransparencyWaterLine == true)
					{
						//PE: Another try :)
						//PE: Distance to water object (0,600,0) can be huge (we have default camera in center), so below waterline objects dont trigger.
						//PE: If we just set it to m_pCamera->fZFar , they will trigger as they use (+= m_pCamera->fZFar)
						//PE: This fix some of the problems and allow pObject->bRenderBeforeWater "SetObjectTransparency(Obj,8)".

						if (pObject->position.vecPosition.y < fWaterPlaneDivisionY)
							fWaterPlaneDivisionY = pObject->position.vecPosition.y;

						pObject->position.fCamDistance = m_pCamera->fZFar;

						bWaterPlaneDivision = true;
					}
					else
					{
						// regular object vs camera distance
						// u74b8 - If already sorted by distance, then we've also already
						//         calculated the camera distance and there's no need to do it again.
						if (g_eGlobalSortOrder != E_SORT_BY_DEPTH)
						{
							pObject->position.fCamDistance = CalculateObjectDistanceFromCamera(pObject);
						}
					}
				}

				// if some objs underwater division, increase their cam distances so they ALL are drawn first (in same order)
				// OR some objects have a distance offset to affect draw order
				for (DWORD iIndex = 0; iIndex < m_vVisibleObjectTransparent.size(); ++iIndex)
				{
					// get obj ptr
					sObject* pObject = m_vVisibleObjectTransparent[iIndex];

					// record original cam distance value
					pObject->position.fStoreLastCamDistance = pObject->position.fCamDistance;

					// for waterline object itself
					if (bWaterPlaneDivision == true)
					{
						//if(  t.terrain.vegetationshaderindex)
						if (pObject->bTransparencyWaterLine == false)
						{
							// for LARGE explosion decals, above water bangs are forced to render FIRST
							float fBaseOfObj = pObject->position.vecPosition.y;
							if (fBaseOfObj < fWaterPlaneDivisionY)
							{
								// u74b8 - use the current camera
								pObject->position.fCamDistance += m_pCamera->fZFar;
							}
							else if (pObject->bRenderBeforeWater) {
								pObject->position.fCamDistance += m_pCamera->fZFar;
							}
						}
					}

					// also apply any artificial distance to object to affect draw order
					pObject->position.fCamDistance += pObject->fArtificialDistanceOffset;
				}

				// u74b7 - sort objects by distance, replaced bubblesort with STL sort
				std::sort(m_vVisibleObjectTransparent.begin(), m_vVisibleObjectTransparent.end(), OrderByReverseCameraDistance());
			}

            // draw in correct back to front order
            for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectTransparent.size(); ++iIndex )
            {
                sObject* pObject = m_vVisibleObjectTransparent [ iIndex ];
				if ( !pObject ) 
					continue;

				// restore original cam distance value (changed for depth reordering)
				pObject->position.fCamDistance = pObject->position.fStoreLastCamDistance;

				// u75b9 - fixes Transparency and Camera Mask problem
                if (( pObject->dwCameraMaskBits & dwCurrentCameraBit ) == 0)
                    continue;

                //if ( !DrawObject ( pObject, false ) )
                //    return false;
                DrawObject ( pObject, false );
            }

	    }
		// end ghost layer
		break;

	case 4 : // Overlay Locked/NoZ Layer
	    
        if ( ! m_vVisibleObjectNoZDepth.empty() )
        {
			// reset to default camera range for noz and locked objects
			tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData( 0 );
			float fCurrentNearRange = m_Camera_Ptr->fZNear;
			float fCurrentFarRange = m_Camera_Ptr->fZFar;
			bool bCameraRangeAndProjectionChanged = false;
			if ( g_pGlob->dwRenderCameraID != 6 && g_pGlob->dwRenderCameraID != 7 )
			{
				// except for cameras 6 and 7 which are VR eye cameras and have their own projection matrix (which should not be overwritten by SetCameraRange)
				SetCameraRange ( 0, 2.5f, 70000.0f ); // forces HUD weapons not to blur/DOF/MOTION/etc
				bCameraRangeAndProjectionChanged = true;
			}

			// record weapon/jetpack techniques (so can restore after cutout technique)
			DWORD dwOldWeaponBasicShaderPtr, dwOldWeaponBoneShaderPtr, dwOldJetpackBoneShaderPtr;
			sEffectItem* pWeaponBasic = NULL;
			sEffectItem* pWeaponBone = NULL;
			sEffectItem* pJetpackBone = NULL;
			if ( g_weaponbasicshadereffectindex > 0 ) 
			{
				if ( GetEffectExist(g_weaponbasicshadereffectindex) ) 
				{
					dwOldWeaponBasicShaderPtr = GetEffectTechniqueEx ( g_weaponbasicshadereffectindex );
					pWeaponBasic = m_EffectList [ g_weaponbasicshadereffectindex ];
				}
			}
			if ( g_weaponboneshadereffectindex > 0 ) 
			{
				if ( GetEffectExist(g_weaponboneshadereffectindex) ) 
				{
					dwOldWeaponBoneShaderPtr = GetEffectTechniqueEx ( g_weaponboneshadereffectindex );
					pWeaponBone = m_EffectList [ g_weaponboneshadereffectindex ];
				}
			}
			if ( g_jetpackboneshadereffectindex > 0 )
			{
				if ( GetEffectExist(g_jetpackboneshadereffectindex) ) 
				{
					dwOldJetpackBoneShaderPtr = GetEffectTechniqueEx ( g_jetpackboneshadereffectindex );
					pJetpackBone = m_EffectList [ g_jetpackboneshadereffectindex ];
				}
			}

			// prefer to render objects that are marked as 'not' transparent, not locked and bNewZLayerObject as true
			// this will allow muzzle flashes to render 'before' the weapon (and smoke to render AFTER as smoke transparency set to 6)
			for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectNoZDepth.size(); ++iIndex )
			{
				sObject* pObject = m_vVisibleObjectNoZDepth [ iIndex ];
				if ( !pObject ) continue;

				// ignore objects whose masks reject the current camera
				if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
					continue;

				// only render not-transparent, not locked and bNewZLayerObject true objects
				bool bRenderObject = false;
				if ( pObject->bTransparentObject==false && pObject->bLockedObject==false && pObject->bNewZLayerObject==true )
					bRenderObject=true;

				// only if object should be rendered
				if ( !bRenderObject )
					continue;

				// skip if IS weapon/jetpack
				bool bIsWeaponOrJetPack = false;
				sObject* pActualObject = pObject;
				if ( pObject->pInstanceOfObject ) pActualObject = pObject->pInstanceOfObject;
				if ( pActualObject->ppMeshList )
				{
					if ( pWeaponBasic && pWeaponBasic->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBasic->pEffectObj ) bIsWeaponOrJetPack = true;
					if ( pWeaponBone && pWeaponBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBone->pEffectObj ) bIsWeaponOrJetPack = true;
					if ( pJetpackBone && pJetpackBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pJetpackBone->pEffectObj ) bIsWeaponOrJetPack = true;
				}
				if ( bIsWeaponOrJetPack == true )
					continue;

				// draw
				DrawObject ( pObject, false );
			}

			// WEAPON RENDERING
			// for NoZDepth pass, two cycles one for depthcutout and regular
			// and hard find weapon shaders that have cutoutdepth techniques
			for ( int iCutOutPassIndex = 0; iCutOutPassIndex < 2; iCutOutPassIndex++ )
			{
				if ( g_weaponbasicshadereffectindex+g_weaponboneshadereffectindex+g_jetpackboneshadereffectindex > 0 )
				{
					if ( iCutOutPassIndex == 0 )
					{
						if ( g_weaponbasicshadereffectindex > 0 ) if ( GetEffectExist(g_weaponbasicshadereffectindex) ) SetEffectTechnique ( g_weaponbasicshadereffectindex, "CutOutDepth" );
						if ( g_weaponboneshadereffectindex > 0 ) if ( GetEffectExist(g_weaponboneshadereffectindex) ) SetEffectTechnique ( g_weaponboneshadereffectindex, "CutOutDepth" );
						if ( g_jetpackboneshadereffectindex > 0 ) if ( GetEffectExist(g_jetpackboneshadereffectindex) ) SetEffectTechnique ( g_jetpackboneshadereffectindex, "CutOutDepth" );
					}
					if ( iCutOutPassIndex == 1 )
					{
						if ( g_weaponbasicshadereffectindex > 0 ) if ( GetEffectExist(g_weaponbasicshadereffectindex) ) SetEffectTechniqueEx ( g_weaponbasicshadereffectindex, dwOldWeaponBasicShaderPtr );
						if ( g_weaponboneshadereffectindex > 0 ) if ( GetEffectExist(g_weaponboneshadereffectindex) ) SetEffectTechniqueEx ( g_weaponboneshadereffectindex, dwOldWeaponBoneShaderPtr );
						if ( g_jetpackboneshadereffectindex > 0 ) if ( GetEffectExist(g_jetpackboneshadereffectindex) ) SetEffectTechniqueEx ( g_jetpackboneshadereffectindex, dwOldJetpackBoneShaderPtr );
					}
				}
				for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectNoZDepth.size(); ++iIndex )
				{
					sObject* pObject = m_vVisibleObjectNoZDepth [ iIndex ];
					if ( !pObject ) continue;

					// ignore objects whose masks reject the current camera
					if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
						continue;

					// skip if not weapon/jetpack
					bool bIsWeaponOrJetPack = false;
					if ( pObject->ppMeshList )
					{
						if ( pWeaponBasic && pWeaponBasic->pEffectObj > 0 && pObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBasic->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pWeaponBone && pWeaponBone->pEffectObj > 0 && pObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBone->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pJetpackBone && pJetpackBone->pEffectObj > 0 && pObject->ppMeshList[0]->pVertexShaderEffect == pJetpackBone->pEffectObj ) bIsWeaponOrJetPack = true;
					}
					if ( bIsWeaponOrJetPack == false )
						continue;

					// draw weapon/jetpack
					DrawObject ( pObject, false );
				}
			}

			// NOZDEPTH LOOP (locked and nozdepth)
			// ( Pass A-ZDepth : Pass B-NoZDepth )
			bool bClearZBuffer = false;
			for ( int iPass = 0; iPass < 2; iPass++ )
			{
				// LOCKED STAGE
				float fCurrentFOV = 0.0f;
				bool bResetCamera = false;
				GGMATRIX matCurrentCameraView;
				for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectNoZDepth.size(); ++iIndex )
				{
					sObject* pObject = m_vVisibleObjectNoZDepth [ iIndex ];
					if ( !pObject ) continue;

					// ignore objects whose masks reject the current camera
					if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
						continue;

					// only render nozdepth objects on second pass
					bool bRenderObject = false;
					if ( iPass==0 && pObject->bNewZLayerObject==false )
					{
						// object has zdepth pass 1
						bRenderObject=true;
					}
					if ( iPass==1 && pObject->bNewZLayerObject==true )
					{
						// object has no zdepth pass 2
						bRenderObject=true;
						if ( bClearZBuffer==false )
						{
							// clear zbuffer
							#ifdef DX11
							//interferes with SAO m_pImmediateContext->ClearpthStencilView(g_pGlob->pCurrentDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
							#else
							m_pD3D->Clear ( 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
							#endif
							bClearZBuffer=true;
						}
					}

					// only if object should be rendered
					if ( !bRenderObject )
						continue;

					// skip if IS weapon/jetpack
					bool bIsWeaponOrJetPack = false;
					sObject* pActualObject = pObject;
					if ( pObject->pInstanceOfObject ) pActualObject = pObject->pInstanceOfObject;
					if ( pActualObject->ppMeshList )
					{
						if ( pWeaponBasic && pWeaponBasic->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBasic->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pWeaponBone && pWeaponBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBone->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pJetpackBone && pJetpackBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pJetpackBone->pEffectObj ) bIsWeaponOrJetPack = true;
					}
*/
