// === END FUNCTION ===



// =============================
// === BT UPDATE TERRAIN LOD ===
// =============================
void BT_UpdateTerrainLOD(unsigned long TerrainID)
{
//Set current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_UPDATETERRAINLOD;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(TerrainID))
	{

	//Check that the terrain is generated
		if(BT_Main.Terrains[TerrainID].Generated==true)
		{
		//Add to queue
			BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_UPDATETERRAINLOD,(char)TerrainID);
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINNOTGENERATED);
			return;
		}

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}

}
// === END FUNCTION ===



// ==============================
// === BT UPDATE TERRAIN CULL ===
// ==============================
void BT_UpdateTerrainCull(unsigned long TerrainID)
{
//Set current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_UPDATETERRAINCULL;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(TerrainID))
	{

	//Check that the terrain is generated
		if(BT_Main.Terrains[TerrainID].Generated==true)
		{
		//Add to queue
			BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_UPDATETERRAINCULL,(char)TerrainID);
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINNOTGENERATED);
			return;
		}

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}

}
// === END FUNCTION ===



// =========================
// === BT RENDER TERRAIN ===
// =========================
void BT_RenderTerrain(unsigned long TerrainID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_RENDER;

//Check if the terrain exists
	if(BT_Intern_TerrainExist(TerrainID))
	{
	//Check that the terrain is generated
		if(BT_Main.Terrains[TerrainID].Generated==true)
		{
		//Add to queue
			BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_RENDERTERRAIN,(char)TerrainID);
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINNOTGENERATED);
			return;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===

void BT_NoRenderTerrain(unsigned long TerrainID)
{
	BT_Main.CurrentFunction=C_BT_FUNCTION_RENDER;
	if(BT_Intern_TerrainExist(TerrainID))
	{
		if(BT_Main.Terrains[TerrainID].Generated==true)
		{
			BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_NORENDERTERRAIN,(char)TerrainID);
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINNOTGENERATED);
			return;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}



// ===========================
// === BT GET TERRAIN INFO ===
// ===========================
void* BT_GetTerrainInfo(unsigned long terrainid)
{
	return BT_Main.Terrains[terrainid].Info;
}
// === END FUNCTION ===



// ============================
// === BT GET LODLEVEL INFO ===
// ============================
void* BT_GetLODLevelInfo(unsigned long terrainid,unsigned long LODLevelID)
{
	return BT_Main.Terrains[terrainid].LODLevel[LODLevelID].Info;
}
// === END FUNCTION ===



// ==========================
// === BT GET SECTOR INFO ===
// ==========================
void* BT_GetSectorInfo(unsigned long terrainid,unsigned long LODLevelID,unsigned long SectorID)
{
	return BT_Main.Terrains[terrainid].LODLevel[LODLevelID].Sector[SectorID].Info;
}
// === END FUNCTION ===

// ========================
// === BT INTERN RENDER === THIS AUTOMATICALY GETS CALLED BY DBPRO
// ========================

bool g_bSkipTerrainRender = false;
unsigned long g_CurrentTerrainCameraID = 0;

void BT_Intern_Render()
{
	// Dave - added this as going into the importer can crash in this, in debug mode, so may be dodgy
	if ( g_bSkipTerrainRender )
	{
		BT_Main.InstructionQueue[0]=NULL;
		BT_Main.InstructionQueueUsed=0;
		g_bSkipTerrainRender = false;
		return;
	}

	// 100418 - seems when skip terrain render (.superflat), viewport is not set (and needs to be)
	// look FURTHER into this to determine if 1920 or 1772 width viewport is correct for terrain
	// 160418 - ensure this fix does not interfere with 64x64 viewport setting for bitmap capture
	if ( g_pGlob->iCurrentBitmapNumber < 32 )
	{
		// but only if not rendering to a shadow map
		if (g_bRenderTerrainForShadowMap == false)
		{
			tagCameraData* Camera = (tagCameraData*)GetCameraInternalData(0);
			D3D11_VIEWPORT vp;
			GGVIEWPORT* pvp = &Camera->viewPort3D;
			vp.TopLeftX = pvp->X;
			vp.TopLeftY = pvp->Y;
			vp.Width = (FLOAT)pvp->Width;
			vp.Height = (FLOAT)pvp->Height;
			vp.MinDepth = pvp->MinZ;
			vp.MaxDepth = pvp->MaxZ;
			SetupSetViewport(g_pGlob->dwRenderCameraID, &vp, NULL);
		}
	}

	try
	{
		//Clear statistics
		BT_Intern_ClearStatistics();

		//Check if autorender is enabled
		if(BT_Main.AutoRender==true)
		{
			//Loop through cameras
			for(unsigned long CameraID=0;CameraID<32;CameraID++)
			{
				if(GetCameraInternalData(CameraID)!=0)
				{
					//Add camera to queue
					BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_SETCURRENTCAMERA,(char)CameraID);

					//Loop through terrains
					for(unsigned long TerrainID=1;TerrainID<C_BT_MAXTERRAINS;TerrainID++)
					{
						//Check that the terrain exists
						if(BT_Main.Terrains[TerrainID].Exists==true)
						{
							//Add terrain cull update to queue
							BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_UPDATETERRAINCULL,(char)TerrainID);

							//Check if the terrain has LOD enabled
							if(BT_Main.Terrains[TerrainID].LODLevels>1)
							{
								//Add terrain LOD update to queue
								BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_UPDATETERRAINLOD,(char)TerrainID);
							}

							//Add render terrain to queue
							BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_RENDERTERRAIN,(char)TerrainID);
						}
					}
				}
			}
		}

		//Read instruction queue
		unsigned long CurrentPos=0;
		do
		{
			if(BT_Main.InstructionQueue[CurrentPos]==C_BT_INSTRUCTION_SETCURRENTCAMERA)
			{
				//Variables
				unsigned long CameraID = BT_Main.InstructionQueue[CurrentPos+1];
				g_CurrentTerrainCameraID = CameraID;

				//Set current camera
				BT_Main.CurrentUpdateCamera=(tagCameraData*)GetCameraInternalData(CameraID);
				BT_Main.FrustumExtracted=false;

				//Increase position
				CurrentPos+=2;
			}
			else if(BT_Main.InstructionQueue[CurrentPos]==C_BT_INSTRUCTION_UPDATETERRAINCULL)
			{
				//Variables
				unsigned long TerrainID=BT_Main.InstructionQueue[CurrentPos+1];

				//Cull Offset and scale
				BT_Main.CullOffset=-BT_Main.Terrains[TerrainID].Object->position.vecPosition;
				BT_Main.CullScale=BT_Main.Terrains[TerrainID].Object->position.vecScale;
				BT_Main.CullScale.x=BT_Main.CullScale.x*BT_Main.Terrains[TerrainID].Scale/C_BT_INTERNALSCALE;
				BT_Main.CullScale.z=BT_Main.CullScale.z*BT_Main.Terrains[TerrainID].Scale/C_BT_INTERNALSCALE;

				//Extract frustum
				if(BT_Main.FrustumExtracted==false)
				{
					BT_Intern_ExtractFrustum();
					BT_Main.FrustumExtracted=true;
				}

				//Update Cull
				BT_RTTMS_UnlockTerrain(&BT_Main.Terrains[TerrainID]);
				BT_Intern_UpdateCullBoxesRec(&BT_Main.Terrains[TerrainID],BT_Main.Terrains[TerrainID].QuadTree,BT_Main.Terrains[TerrainID].QuadTreeLevels);

				// camera 30 (cube rendering) does not cull terrain visibility
				bool bIntersectingCull = false;
				if ( g_CurrentTerrainCameraID == 30 ) bIntersectingCull = true;
				BT_Intern_CalculateCullingRec(&BT_Main.Terrains[TerrainID],BT_Main.Terrains[TerrainID].QuadTree,BT_Main.Terrains[TerrainID].QuadTreeLevels,bIntersectingCull);

				//Increase position
				CurrentPos+=2;
			}
			else if(BT_Main.InstructionQueue[CurrentPos]==C_BT_INSTRUCTION_UPDATETERRAINLOD)
			{
				//Variables
				unsigned long TerrainID=BT_Main.InstructionQueue[CurrentPos+1];

				// if no camera at this time, leave
				if ( BT_Main.CurrentUpdateCamera==NULL ) return;

				//Set main camera
				BT_Main.LODCamPosition=BT_Main.CurrentUpdateCamera->vecPosition;

				//Find LODLevels
				if(BT_Main.Terrains[TerrainID].LODLevels>1)
					BT_Intern_CalculateLODLevelsRec(&BT_Main.Terrains[TerrainID],BT_Main.Terrains[TerrainID].QuadTree,BT_Main.Terrains[TerrainID].QuadTreeLevels,0);

				//Fix LOD seams
				BT_Intern_FixLODSeams(&BT_Main.Terrains[TerrainID]);

				//Increase position
				CurrentPos+=2;
			}
			else if(BT_Main.InstructionQueue[CurrentPos]==C_BT_INSTRUCTION_RENDERTERRAIN)
			{
				// Variables
				unsigned long TerrainID=BT_Main.InstructionQueue[CurrentPos+1];

				// Check if the object is visible (lee - 050115 - if present!)
				if ( BT_Main.Terrains[TerrainID].Object->pFrame )
				{
					if(BT_Main.Terrains[TerrainID].Object->pFrame->pMesh->bVisible==true)
					{
						// Set main camera
						BT_Main.LODCamPosition=BT_Main.CurrentUpdateCamera->vecPosition;

						// Render terrain
						BT_Intern_RenderTerrain(&BT_Main.Terrains[TerrainID]);
					}
				}

				// Increase position
				CurrentPos+=2;
			}
			else if(BT_Main.InstructionQueue[CurrentPos]==C_BT_INSTRUCTION_NORENDERTERRAIN)
			{
				// this mimics the above, but calls BT_Intern_NoRenderTerrain 
				// which renders no terrain but keeps state changes as though it was (fixes VR rendering)
				unsigned long TerrainID=BT_Main.InstructionQueue[CurrentPos+1];
				if ( BT_Main.Terrains[TerrainID].Object->pFrame )
				{
					if(BT_Main.Terrains[TerrainID].Object->pFrame->pMesh->bVisible==true)
					{
						BT_Main.LODCamPosition=BT_Main.CurrentUpdateCamera->vecPosition;
						BT_Intern_NoRenderTerrain(&BT_Main.Terrains[TerrainID]);
					}
				}

				// Increase position
				CurrentPos+=2;
			}
		}
		while(BT_Main.InstructionQueue[CurrentPos]!=NULL);

		//Clear instruction queue
		BT_Main.InstructionQueue[0]=NULL;
		BT_Main.InstructionQueueUsed=0;
	}
	catch (...)
	{
		//MessageBox ( NULL, "Terrain module error", "Terrain Module", MB_OK );
	}
}
// === END FUNCTION ===



// ==========================================
// === BT INTERN ADD TO INSTRUCTION QUEUE ===
// ==========================================
static void BT_Intern_AddToInstructionQueue(char Instruction,char Data)
{
//Check the queue size
	if(BT_Main.InstructionQueueUsed+2<BT_Main.InstructionQueueSize)
	{
		BT_Main.InstructionQueue[BT_Main.InstructionQueueUsed]=Instruction;
		BT_Main.InstructionQueue[BT_Main.InstructionQueueUsed+1]=Data;
		BT_Main.InstructionQueueUsed+=2;
		BT_Main.InstructionQueue[BT_Main.InstructionQueueUsed]=NULL;
	}
}
// === END FUNCTION ===



// ==================================
// === BT INTERN CLEAR STATISTICS ===
// ==================================
static void BT_Intern_ClearStatistics()
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_CLEARSTATISTICS;

//Clear
	BT_Main.DrawPrimitiveCount=0;
	BT_Main.DrawCalls=0;
	BT_Main.CullChecks=0;

}
// === END FUNCTION ===

// ================================
// === BT INTERN RENDER TERRAIN ===
// ================================
static void BT_Intern_RenderTerrain(s_BT_terrain* Terrain)
{
	// Variables
	tagCameraData* Camera = BT_Main.CurrentUpdateCamera;
	GGMATRIX World;
	sMesh* Mesh = Terrain->Object->pFrame->pMesh;

	#ifdef DX11
	// create constant buffer for quick world position changes
	if ( m_pCBChangePerTerrsainChunk == NULL )
	{
		D3D11_BUFFER_DESC bdChangePerTerrsainChunkBuffer;
		std::memset ( &bdChangePerTerrsainChunkBuffer, 0, sizeof ( bdChangePerTerrsainChunkBuffer ) );
		bdChangePerTerrsainChunkBuffer.Usage          = D3D11_USAGE_DEFAULT;
		bdChangePerTerrsainChunkBuffer.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		bdChangePerTerrsainChunkBuffer.CPUAccessFlags = 0;
		bdChangePerTerrsainChunkBuffer.ByteWidth      = sizeof ( CBChangePerTerrsainChunk );
		if ( FAILED ( m_pD3D->CreateBuffer ( &bdChangePerTerrsainChunkBuffer, NULL, &m_pCBChangePerTerrsainChunk ) ) )
			return;
	}
	if ( m_pCBChangePerTerrsainChunkPS == NULL )
	{
		D3D11_BUFFER_DESC bdChangePerTerrsainChunkBuffer;
		std::memset ( &bdChangePerTerrsainChunkBuffer, 0, sizeof ( bdChangePerTerrsainChunkBuffer ) );
		bdChangePerTerrsainChunkBuffer.Usage          = D3D11_USAGE_DEFAULT;
		bdChangePerTerrsainChunkBuffer.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		bdChangePerTerrsainChunkBuffer.CPUAccessFlags = 0;
		bdChangePerTerrsainChunkBuffer.ByteWidth      = sizeof ( CBChangePerTerrsainChunkPS );
		if ( FAILED ( m_pD3D->CreateBuffer ( &bdChangePerTerrsainChunkBuffer, NULL, &m_pCBChangePerTerrsainChunkPS ) ) )
			return;
	}

	// when rendering terrain for shadow mapping, these transforms are already set (for the light camera view)
	if (g_bRenderTerrainForShadowMap == false)
	{
		// Transforms
		GGSetTransform(GGTS_PROJECTION, &Camera->matProjection);
		GGSetTransform(GGTS_VIEW, &Camera->matView);
	}

	// Viewport for terrain rendering
	if (g_bRenderTerrainForShadowMap == true)
	{
		// the call to RSSetViewports already done in ShadowMap code
	}
	else
	{
		D3D11_VIEWPORT vp;
		GGVIEWPORT* pvp = &Camera->viewPort3D;
		vp.TopLeftX = pvp->X;
		vp.TopLeftY = pvp->Y;
		vp.Width = (FLOAT)pvp->Width;
		vp.Height = (FLOAT)pvp->Height;
		vp.MinDepth = pvp->MinZ;
		vp.MaxDepth = pvp->MaxZ;
		SetupSetViewport ( g_pGlob->dwRenderCameraID, &vp, NULL );
	}

	// Set current render terrain
	BT_Main.CurrentRenderTerrain=Terrain;

	// Unlock sectors
	BT_Intern_UnlockSectorsRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels);

	// Check if theres an effect
	if ( Mesh->pVertexShaderEffect != NULL )
	{
		// Vertex Declaration
		m_pImmediateContext->IASetInputLayout ( Terrain->VertexDeclaration );

		// Variables
		LPGGEFFECT Effect = Mesh->pVertexShaderEffect->m_pEffect;

		// Obtain technique handles
		GGTECHNIQUE hNearTechnique = Mesh->pVertexShaderEffect->m_hCurrentTechnique;
		GGTECHNIQUE hDistantTechnique = Effect->GetTechniqueByName ( "Distant" );

		// Can also render terrain into the shadow map, so use super quick render type
		int iQualityPassCount = 2;
		if (g_bRenderTerrainForShadowMap == true)
		{
			iQualityPassCount = 1;
			hNearTechnique = Effect->GetTechniqueByName ( "DepthMap" );
			hDistantTechnique = Effect->GetTechniqueByName ( "DepthMap" );
		}
		// Two passes, one NORMAL technique and one VERY LOW technique (distant terrain)
		BT_Main.CurrentEffect=Mesh->pVertexShaderEffect;
		for ( int iQualityPass=0; iQualityPass<iQualityPassCount; iQualityPass++ )
		{
			//PE: fullshadowsoreditor was always 1 (g_iQualityTechniqueMode==1) , now always use hDistantTechnique.
			// Set correct technique
			GGTECHNIQUE hTechniqueUsed = NULL;
//			if ( g_iQualityTechniqueMode==0 )
//			{
				if ( iQualityPass==0 ) hTechniqueUsed = hNearTechnique;
				if ( iQualityPass==1 ) hTechniqueUsed = hDistantTechnique;
//			}
//			else
//			{
//				if ( g_iQualityTechniqueMode==1 ) hTechniqueUsed = hNearTechnique;
//				if ( g_iQualityTechniqueMode==2 ) hTechniqueUsed = hDistantTechnique;
//			}

			// only one pass (removed secondary depth pass for DX11)
			if ( hTechniqueUsed->IsValid() )
			{
				// Move plenty of effect setup to here (performance)
				m_pImmediateContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				if (g_bRenderTerrainForShadowMap == true)
				{
					m_pImmediateContext->RSSetState(m_pRasterStateTerrainShadow);
					m_pImmediateContext->OMSetBlendState(m_pBlendStateNoAlpha, 0, 0xffffffff);
					m_pImmediateContext->OMSetDepthStencilState(m_pDepthStencilState, 0);
				}
				else
				{
					// state blocks for raster, blend and depthstencil
					m_pImmediateContext->RSSetState(m_pRasterState);
					m_pImmediateContext->OMSetBlendState(m_pBlendStateNoAlpha, 0, 0xffffffff);
					m_pImmediateContext->OMSetDepthStencilState(m_pDepthStencilState, 0);
				}


				// Update shadow textures of terrain shader (added for DX11 - not sure where DX9 did this)
				if (iQualityPass == 0 && g_bRenderTerrainForShadowMap == false)
				{
					DWORD dwEffectIndex = Mesh->pVertexShaderEffect->m_dwEffectIndex;
					if (dwEffectIndex < EFFECT_INDEX_SIZE)
					{
						if (g_CascadedShadow.m_pEffectParam[dwEffectIndex])
						{
							GGHANDLE hdepthHandle0 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX1;
							GGHANDLE hdepthHandle1 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX2;
							GGHANDLE hdepthHandle2 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX3;
							GGHANDLE hdepthHandle3 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX4;
							GGHANDLE hdepthHandle4 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX5;
							GGHANDLE hdepthHandle5 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX6;
							GGHANDLE hdepthHandle6 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX7;
							GGHANDLE hdepthHandle7 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX8;
							if (hdepthHandle0 && g_CascadedShadow.m_depthTexture[0]) hdepthHandle0->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[0]->getTextureResourceView());
							if (hdepthHandle1 && g_CascadedShadow.m_depthTexture[1]) hdepthHandle1->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[1]->getTextureResourceView());
							if (hdepthHandle2 && g_CascadedShadow.m_depthTexture[2]) hdepthHandle2->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[2]->getTextureResourceView());
							if (hdepthHandle3 && g_CascadedShadow.m_depthTexture[3]) hdepthHandle3->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[3]->getTextureResourceView());
							if (hdepthHandle4 && g_CascadedShadow.m_depthTexture[4]) hdepthHandle4->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[4]->getTextureResourceView());
							if (hdepthHandle5 && g_CascadedShadow.m_depthTexture[5]) hdepthHandle5->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[5]->getTextureResourceView());
							if (hdepthHandle6 && g_CascadedShadow.m_depthTexture[6]) hdepthHandle6->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[6]->getTextureResourceView());
							if (hdepthHandle7 && g_CascadedShadow.m_depthTexture[7]) hdepthHandle7->AsShaderResource()->SetResource(g_CascadedShadow.m_depthTexture[7]->getTextureResourceView());
						}
					}
				}

				// pass clipping data to shader
				if ( Mesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle )
				{
					GGVECTOR4 vec;
					tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwRenderCameraID );
					if ( m_Camera_Ptr )
					{
						if ( m_Camera_Ptr->iClipPlaneOn==1 )
						{
							// special mode which creates plane but does not use RenderState to set clip
							// as you cannot mix FF clip and HLSL clip in same scene (artefacts)
							vec.x = m_Camera_Ptr->planeClip.a;
							vec.y = m_Camera_Ptr->planeClip.b;
							vec.z = m_Camera_Ptr->planeClip.c;
							vec.w = m_Camera_Ptr->planeClip.d;
						}
						else
						{
							// ensure shader stops using clip plane when not being clipped!
							vec = GGVECTOR4( 0.0f, 1.0f, 0.0f, 99999.0f );
						}
					}
					else
					{
						// ensure shader stops using clip plane when not being clipped!
						vec = GGVECTOR4( 0.0f, 1.0f, 0.0f, 99999.0f );
					}
					Mesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle->AsVector()->SetFloatVector ( (float*)&vec );
				}

				// Ensure normal invert in effect for terrain (NOTE: not liking duplicated of code)
				#ifdef DX11
				//PE: removed , terrain always invert normal.
//				GGHANDLE pArtFlags = Mesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ArtFlagControl1" );
//				if ( pArtFlags )
//				{
//					float fInvertNormal = 0.0f;
//					if ( Mesh->dwArtFlags & 0x1 ) fInvertNormal = 1.0f;
//					GGVECTOR4 vec4 = GGVECTOR4 ( fInvertNormal, 0.0f, 0.0f, 0.0f );
//					pArtFlags->AsVector()->SetFloatVector ( (float*)&vec4 );
//				}
				#endif

				// apply effect ready for rendering
				hTechniqueUsed->GetPassByIndex(0)->Apply(0,m_pImmediateContext);

				// Set textures (AFTER Apply which overrides texture view ptrs)
				if (g_bRenderTerrainForShadowMap == false)
				{
					for (unsigned long i = 0; i < Mesh->dwTextureCount; i++) // terrain now has lots of textures
					{
						if (i != 1 && i != 5 && i != 7)
						{
							//PE: only for more gpu mem (cache) they are not used.
							ID3D11ShaderResourceView* lpTexture = NULL;
							if (Mesh->dwTextureCount > i) lpTexture = Mesh->pTextures[i].pTexturesRefView;
							m_pImmediateContext->PSSetShaderResources(i, 1, &lpTexture);
						}
					}
				}

				// assign constants
				GGMATRIX matWorld;
				GGGetTransform ( GGTS_WORLD, &matWorld );
				Mesh->pVertexShaderEffect->Start ( Mesh, matWorld );

				//PE: Terrain should be split into smaller meshes for better light.
				if (g_bRenderTerrainForShadowMap == false)
					update_mesh_light(Mesh, NULL,NULL);

				// Render many terrain chunks (only CB changes for world position for faster rendering)
				BT_Intern_RenderTerrainRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels,iQualityPass);

				// End effect
				BT_Main.CurrentEffect=NULL;

				// Free textures (especially camera image texture which needs to be output bound next cycle)
				if (g_bRenderTerrainForShadowMap == false)
				{
					for (unsigned long i = 0; i < Mesh->dwTextureCount; i++)
					{
						if (i != 1 && i != 5 && i != 7)
						{
							//PE: 
							ID3D11ShaderResourceView* lpTexture = NULL;
							m_pImmediateContext->PSSetShaderResources(i, 1, &lpTexture);
						}
					}
				}
			}
		}
	}
	else
	{
		// no rendering if no effect shader
	}

	// zero current render terrain
	BT_Main.CurrentRenderTerrain=NULL;

	#else
	// Set textures
	IGGDevice* D3DDevice=m_pD3D;
	for(unsigned long i=0;i<8;i++)
	{
		if(Mesh->dwTextureCount>i)
		{
			D3DDevice->SetTexture(Mesh->pTextures[i].dwStage,Mesh->pTextures[i].pTexturesRef);
			D3DDevice->SetTextureStageState(Mesh->pTextures[i].dwStage,D3DTSS_COLOROP,Mesh->pTextures[i].dwBlendMode);
			D3DDevice->SetTextureStageState(Mesh->pTextures[i].dwStage,D3DTSS_COLORARG1,Mesh->pTextures[i].dwBlendArg1);
			D3DDevice->SetTextureStageState(Mesh->pTextures[i].dwStage,D3DTSS_COLORARG2,Mesh->pTextures[i].dwBlendArg2);
			D3DDevice->SetSamplerState(Mesh->pTextures[i].dwStage,D3DSAMP_MAGFILTER,Mesh->pTextures[i].dwMagState);
			D3DDevice->SetSamplerState(Mesh->pTextures[i].dwStage,D3DSAMP_MINFILTER,Mesh->pTextures[i].dwMinState);
			D3DDevice->SetSamplerState(Mesh->pTextures[i].dwStage,D3DSAMP_MIPFILTER,Mesh->pTextures[i].dwMipState);

			// Clamp base texture
			if(i==0)
			{
				D3DDevice->SetSamplerState(Mesh->pTextures[i].dwStage,D3DSAMP_ADDRESSU,GGTADDRESS_CLAMP);
				D3DDevice->SetSamplerState(Mesh->pTextures[i].dwStage,D3DSAMP_ADDRESSV,GGTADDRESS_CLAMP);
			}
		}
		else
		{
			D3DDevice->SetTexture(i,NULL);
		}
	}

	// Set shader
	if(Mesh->bOverridePixelShader)
	{
		D3DDevice->SetPixelShader(Mesh->pPixelShader);
	}
	else
	{
		D3DDevice->SetPixelShader(NULL);
	}
	if(Mesh->bUseVertexShader)
	{
		D3DDevice->SetVertexShader(Mesh->pVertexShader);
	}
	else
	{
		D3DDevice->SetVertexShader(NULL);
	}

	// Transforms
	D3DDevice->SetTransform(GGTS_PROJECTION,&Camera->matProjection);
	D3DDevice->SetTransform(GGTS_VIEW,&Camera->matView);

	// Store old VertDec to restore later
	IDirect3DVertexDeclaration9* pDecl = NULL;
	D3DDevice->GetVertexDeclaration(&pDecl);
	DWORD dwFVF = 0;
	D3DDevice->GetFVF(&dwFVF);

	// Vertex Declaration
	D3DDevice->SetVertexDeclaration(Terrain->VertexDeclaration);

	// Viewport
	D3DDevice->SetViewport((GGVIEWPORT*)&Camera->viewPort3D);

	// DBPRO RENDERING ENGINE
	DBPRO_SetMeshRenderStates(Mesh);

	// Set current render terrain
	BT_Main.CurrentRenderTerrain=Terrain;

	// Unlock sectors
	BT_Intern_UnlockSectorsRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels);

	// Check if theres an effect
	if(Mesh->pVertexShaderEffect!=NULL)
	{
		// Variables
		LPGGEFFECT Effect=Mesh->pVertexShaderEffect->m_pEffect;

		// Obtain technique handles
		GGHANDLE hNearTechnique = Effect->GetCurrentTechnique();
		GGHANDLE hDistantTechnique = Effect->GetTechniqueByName ( "Distant" );

		// Two passes, one NORMAL technique and one VERY LOW technique (distant terrain)
		BT_Main.CurrentEffect=Mesh->pVertexShaderEffect;
		for ( int iQualityPass=0; iQualityPass<2; iQualityPass++ )
		{
			// Set correct technique
			GGHANDLE hTechniqueUsed = NULL;
			if ( g_iQualityTechniqueMode==0 )
			{
				if ( iQualityPass==0 ) hTechniqueUsed = hNearTechnique;
				if ( iQualityPass==1 ) hTechniqueUsed = hDistantTechnique;
			}
			else
			{
				if ( g_iQualityTechniqueMode==1 ) hTechniqueUsed = hNearTechnique;
				if ( g_iQualityTechniqueMode==2 ) hTechniqueUsed = hDistantTechnique;
			}
			Effect->SetTechnique ( hTechniqueUsed );

			// Begin effect
			UINT Passes;
			Effect->Begin(&Passes,NULL);

			// store main camera render target and depth stencil buffer
			IGGSurface* pCurrentRenderTarget = NULL;
			IGGSurface* pCurrentDepthTarget = NULL;
			m_pD3D->GetRenderTarget( 0, &pCurrentRenderTarget );
			m_pD3D->GetDepthStencilSurface( &pCurrentDepthTarget );

			// Begin loop
			for(unsigned int Pass=0;Pass<Passes;Pass++)
			{
				// check which render target we are writing to (camera or depth-texture)
				GGHANDLE hPass = Effect->GetPass( hTechniqueUsed, Pass );
				GGHANDLE hRT = Effect->GetAnnotationByName( hPass, "RenderColorTarget" );
				const char* szRT = 0;
				if ( hRT ) Effect->GetString( hRT, &szRT );
				if ( szRT && strnicmp( szRT, "[depthtexture]", strlen("[depthtexture]") )==NULL && g_pGlob->dwRenderCameraID==0 ) 
				{
					// render to depth texture from main basic3D DLL
					int iSuccess = SwitchRenderTargetToDepthTexture(0);
				}
				else
				{
					// render to original render target
					m_pD3D->SetRenderTarget( 0, pCurrentRenderTarget );
					m_pD3D->SetDepthStencilSurface( pCurrentDepthTarget );
				}

				// Begin pass
				Effect->BeginPass(Pass);

				// Render
				BT_Intern_RenderTerrainRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels,iQualityPass);

				// End pass
				Effect->EndPass();
			}

			// End effect
			BT_Main.CurrentEffect=NULL;
			Effect->End();

			// restore render target in any event
			m_pD3D->SetRenderTarget( 0, pCurrentRenderTarget );
			m_pD3D->SetDepthStencilSurface( pCurrentDepthTarget );
		}

		// Restore terrain shader technique
		Effect->SetTechnique ( hNearTechnique );
	}
	else
	{
		// Render
		BT_Main.CurrentEffect=NULL;
		BT_Intern_RenderTerrainRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels,0);
		BT_Intern_RenderTerrainRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels,1);
	}

	//Zero current render terrain
	BT_Main.CurrentRenderTerrain=NULL;

	// Restore vertex decl
	D3DDevice->SetVertexDeclaration(pDecl);
	D3DDevice->SetFVF(dwFVF);
	#endif
}
// === END FUNCTION ===

static void BT_Intern_NoRenderTerrain(s_BT_terrain* Terrain)
{
	// Variables
	tagCameraData* Camera = BT_Main.CurrentUpdateCamera;
	GGMATRIX World;
	sMesh* Mesh = Terrain->Object->pFrame->pMesh;

	// create constant buffer for quick world position changes
	if ( m_pCBChangePerTerrsainChunk == NULL )
	{
		D3D11_BUFFER_DESC bdChangePerTerrsainChunkBuffer;
		std::memset ( &bdChangePerTerrsainChunkBuffer, 0, sizeof ( bdChangePerTerrsainChunkBuffer ) );
		bdChangePerTerrsainChunkBuffer.Usage          = D3D11_USAGE_DEFAULT;
		bdChangePerTerrsainChunkBuffer.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		bdChangePerTerrsainChunkBuffer.CPUAccessFlags = 0;
		bdChangePerTerrsainChunkBuffer.ByteWidth      = sizeof ( CBChangePerTerrsainChunk );
		if ( FAILED ( m_pD3D->CreateBuffer ( &bdChangePerTerrsainChunkBuffer, NULL, &m_pCBChangePerTerrsainChunk ) ) )
			return;
	}
	if ( m_pCBChangePerTerrsainChunkPS == NULL )
	{
		D3D11_BUFFER_DESC bdChangePerTerrsainChunkBuffer;
		std::memset ( &bdChangePerTerrsainChunkBuffer, 0, sizeof ( bdChangePerTerrsainChunkBuffer ) );
		bdChangePerTerrsainChunkBuffer.Usage          = D3D11_USAGE_DEFAULT;
		bdChangePerTerrsainChunkBuffer.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		bdChangePerTerrsainChunkBuffer.CPUAccessFlags = 0;
		bdChangePerTerrsainChunkBuffer.ByteWidth      = sizeof ( CBChangePerTerrsainChunkPS );
		if ( FAILED ( m_pD3D->CreateBuffer ( &bdChangePerTerrsainChunkBuffer, NULL, &m_pCBChangePerTerrsainChunkPS ) ) )
			return;
	}

	// Transforms
	GGSetTransform(GGTS_PROJECTION,&Camera->matProjection);
	GGSetTransform(GGTS_VIEW,&Camera->matView);

	// Viewport
    D3D11_VIEWPORT vp;
	GGVIEWPORT* pvp = &Camera->viewPort3D;
    vp.TopLeftX = pvp->X;
    vp.TopLeftY = pvp->Y;
    vp.Width = (FLOAT)pvp->Width;
    vp.Height = (FLOAT)pvp->Height;
    vp.MinDepth = pvp->MinZ;
    vp.MaxDepth = pvp->MaxZ;
	SetupSetViewport ( g_pGlob->dwRenderCameraID, &vp, NULL );

	/*
	// Set current render terrain
	BT_Main.CurrentRenderTerrain=Terrain;

	// Unlock sectors
	BT_Intern_UnlockSectorsRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels);

	// Check if theres an effect
	if ( Mesh->pVertexShaderEffect != NULL )
	{
		// Vertex Declaration
		m_pImmediateContext->IASetInputLayout ( Terrain->VertexDeclaration );

		// Variables
		LPGGEFFECT Effect = Mesh->pVertexShaderEffect->m_pEffect;

		// Obtain technique handles
		GGTECHNIQUE hNearTechnique = Mesh->pVertexShaderEffect->m_hCurrentTechnique;
		GGTECHNIQUE hDistantTechnique = Effect->GetTechniqueByName ( "Distant" );
		
		// Two passes, one NORMAL technique and one VERY LOW technique (distant terrain)
		BT_Main.CurrentEffect=Mesh->pVertexShaderEffect;
		for ( int iQualityPass=0; iQualityPass<2; iQualityPass++ )
		{
			//PE: Todo fullshadowsoreditor is always 1 , so hDistantTechnique is never used ?
			// Set correct technique
			GGTECHNIQUE hTechniqueUsed = NULL;
			if ( g_iQualityTechniqueMode==0 )
			{
				if ( iQualityPass==0 ) hTechniqueUsed = hNearTechnique;
				if ( iQualityPass==1 ) hTechniqueUsed = hDistantTechnique;
			}
			else
			{
				if ( g_iQualityTechniqueMode==1 ) hTechniqueUsed = hNearTechnique;
				if ( g_iQualityTechniqueMode==2 ) hTechniqueUsed = hDistantTechnique;
			}

			// only one pass (removed secondary depth pass for DX11)
			if ( hTechniqueUsed->IsValid() )
			{
				// Move plenty of effect setup to here (performance)
				m_pImmediateContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				// state blocks for raster, blend and depthstencil
				m_pImmediateContext->RSSetState(m_pRasterState);
				m_pImmediateContext->OMSetBlendState(m_pBlendStateNoAlpha, 0, 0xffffffff);
				m_pImmediateContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );

				// Update shadow textures of terrain shader (added for DX11 - not sure where DX9 did this)
				DWORD dwEffectIndex = Mesh->pVertexShaderEffect->m_dwEffectIndex;
				if ( dwEffectIndex < EFFECT_INDEX_SIZE )
				{
					if ( g_CascadedShadow.m_pEffectParam[dwEffectIndex] )
					{
						GGHANDLE hdepthHandle0 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX1;
						GGHANDLE hdepthHandle1 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX2;
						GGHANDLE hdepthHandle2 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX3;
						GGHANDLE hdepthHandle3 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX4;
						GGHANDLE hdepthHandle4 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX5;
						GGHANDLE hdepthHandle5 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX6;
						GGHANDLE hdepthHandle6 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX7;
						GGHANDLE hdepthHandle7 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX8;
						if ( hdepthHandle0 && g_CascadedShadow.m_depthTexture[0] ) hdepthHandle0->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[0]->getTextureResourceView() );
						if ( hdepthHandle1 && g_CascadedShadow.m_depthTexture[1] ) hdepthHandle1->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[1]->getTextureResourceView() );
						if ( hdepthHandle2 && g_CascadedShadow.m_depthTexture[2] ) hdepthHandle2->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[2]->getTextureResourceView() );
						if ( hdepthHandle3 && g_CascadedShadow.m_depthTexture[3] ) hdepthHandle3->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[3]->getTextureResourceView() );
						if ( hdepthHandle4 && g_CascadedShadow.m_depthTexture[4] ) hdepthHandle4->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[4]->getTextureResourceView() );
						if ( hdepthHandle5 && g_CascadedShadow.m_depthTexture[5] ) hdepthHandle5->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[5]->getTextureResourceView() );
						if ( hdepthHandle6 && g_CascadedShadow.m_depthTexture[6] ) hdepthHandle6->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[6]->getTextureResourceView() );
						if ( hdepthHandle7 && g_CascadedShadow.m_depthTexture[7] ) hdepthHandle7->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[7]->getTextureResourceView() );
					}
				}

				// pass clipping data to shader
				if ( Mesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle )
				{
					GGVECTOR4 vec;
					tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwRenderCameraID );
					if ( m_Camera_Ptr )
					{
						if ( m_Camera_Ptr->iClipPlaneOn==1 )
						{
							// special mode which creates plane but does not use RenderState to set clip
							// as you cannot mix FF clip and HLSL clip in same scene (artefacts)
							vec.x = m_Camera_Ptr->planeClip.a;
							vec.y = m_Camera_Ptr->planeClip.b;
							vec.z = m_Camera_Ptr->planeClip.c;
							vec.w = m_Camera_Ptr->planeClip.d;
						}
						else
						{
							// ensure shader stops using clip plane when not being clipped!
							vec = GGVECTOR4( 0.0f, 1.0f, 0.0f, 99999.0f );
						}
					}
					else
					{
						// ensure shader stops using clip plane when not being clipped!
						vec = GGVECTOR4( 0.0f, 1.0f, 0.0f, 99999.0f );
					}
					Mesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle->AsVector()->SetFloatVector ( (float*)&vec );
				}

				// Ensure normal invert in effect for terrain (NOTE: not liking duplicated of code)
				#ifdef DX11
				//PE: removed , terrain always invert normal.
//				GGHANDLE pArtFlags = Mesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ArtFlagControl1" );
//				if ( pArtFlags )
//				{
//					float fInvertNormal = 0.0f;
//					if ( Mesh->dwArtFlags & 0x1 ) fInvertNormal = 1.0f;
//					GGVECTOR4 vec4 = GGVECTOR4 ( fInvertNormal, 0.0f, 0.0f, 0.0f );
//					pArtFlags->AsVector()->SetFloatVector ( (float*)&vec4 );
//				}
				#endif

				// apply effect ready for rendering
				hTechniqueUsed->GetPassByIndex(0)->Apply(0,m_pImmediateContext);

				// Set textures (AFTER Apply which overrides texture view ptrs)
				for ( unsigned long i = 0; i < Mesh->dwTextureCount; i++ ) // terrain now has lots of textures
				{
					if (i != 1 && i != 5 && i != 7) 
					{ 
						//PE: only for more gpu mem (cache) they are not used.
						ID3D11ShaderResourceView* lpTexture = NULL; 
						if ( Mesh->dwTextureCount > i ) lpTexture = Mesh->pTextures[i].pTexturesRefView;
						m_pImmediateContext->PSSetShaderResources ( i, 1, &lpTexture );
					}
				}

				// assign constants
				GGMATRIX matWorld;
				GGGetTransform ( GGTS_WORLD, &matWorld );
				Mesh->pVertexShaderEffect->Start ( Mesh, matWorld );

				//PE: Terrain should be split into smaller meshes for better light.
				update_mesh_light(Mesh, NULL,NULL);

				// Render many terrain chunks (only CB changes for world position for faster rendering)
				BT_Intern_RenderTerrainRec(Terrain,Terrain->QuadTree,Terrain->QuadTreeLevels,iQualityPass);

				// End effect
				BT_Main.CurrentEffect=NULL;

				// Free textures (especially camera image texture which needs to be output bound next cycle)
				for ( unsigned long i = 0; i < Mesh->dwTextureCount; i++ )
				{
					if (i != 1 && i != 5 && i != 7) 
					{ 
						//PE: 
						ID3D11ShaderResourceView* lpTexture = NULL; 
						m_pImmediateContext->PSSetShaderResources ( i, 1, &lpTexture );
					}
				}
			}
		}
	}
	else
	{
		// no rendering if no effect shader
	}
	*/

	// zero current render terrain
	BT_Main.CurrentRenderTerrain=NULL;
}


// ======================================
// === BT INTERN CALCULATE LOD LEVELS ===
// ======================================
static void BT_Intern_CalculateLODLevelsRec(s_BT_terrain* Terrain,s_BT_QuadTree* Quadtree,unsigned long Level,unsigned char LODLevelToDraw)
{
//Initialise Drawthis variable
	Quadtree->DrawThis=false;

//Check if we have to draw this LOD level
	if(Quadtree->Sector!=NULL && LODLevelToDraw==0)
	{
		if(BT_Intern_DistanceToLODCamera(Terrain,Quadtree->CullBox)>Terrain->LODLevel[Level].Distance*Terrain->LODLevel[Level].Distance)
		{
			Quadtree->DrawThis=true;
			LODLevelToDraw=unsigned char(Level);
		}
	}

//Check if LOD level is greater than 0
	if(Level>0)
	{
	//Part 1
		if(Quadtree->n1->Culled==false)
			BT_Intern_CalculateLODLevelsRec(Terrain,Quadtree->n1,Level-1,LODLevelToDraw);

	//Part 2
		if(Quadtree->n2->Culled==false)
			BT_Intern_CalculateLODLevelsRec(Terrain,Quadtree->n2,Level-1,LODLevelToDraw);

	//Part 3
		if(Quadtree->n3->Culled==false)
			BT_Intern_CalculateLODLevelsRec(Terrain,Quadtree->n3,Level-1,LODLevelToDraw);

	//Part 4
		if(Quadtree->n4->Culled==false)
			BT_Intern_CalculateLODLevelsRec(Terrain,Quadtree->n4,Level-1,LODLevelToDraw);
	}else{
		if(Terrain->LODMap[Quadtree->row][Quadtree->collumn].Level!=LODLevelToDraw)
		{
			Terrain->LODMap[Quadtree->row][Quadtree->collumn].Level=LODLevelToDraw;
		}

	}
}
// === END FUNCTION ===



// ===============================
// === BT INTERN FIX LOD SEAMS ===
// ===============================
static void BT_Intern_FixLODSeams(s_BT_terrain* Terrain)
{
//Find seams to be fixed
	for(unsigned char LODLevel=0;LODLevel<Terrain->LODLevels;LODLevel++)
	{
		unsigned long Span=0x1<<LODLevel;
		for(unsigned long Sector=0;Sector<Terrain->LODLevel[LODLevel].Sectors;Sector++)
		{
			s_BT_Sector* SectorPtr=&Terrain->LODLevel[LODLevel].Sector[Sector];
			if(SectorPtr->QuadTree!=NULL)
			{
				if(SectorPtr->QuadTree->Culled==false && SectorPtr->QuadTree->DrawThis==true)
				{
				//Find row and collumn
					s_BT_QuadTree* CurrentQuadTree=SectorPtr->QuadTree;
					unsigned long Row=SectorPtr->Row*Span;
					unsigned long Collumn=SectorPtr->Column*Span;

				//Left
					if(Collumn>0)
					{
						int LeftSideLODLevel = Terrain->LODMap[Row][Collumn-1].Level - LODLevel;
						if (LeftSideLODLevel < 0)
							LeftSideLODLevel = 0;
						if(SectorPtr->LeftSideLODLevel!=LeftSideLODLevel)
						{
							SectorPtr->LeftSideLODLevel=LeftSideLODLevel;
							SectorPtr->LeftSideNeedsUpdate=true;
						}
					}

				//Top
					if(Row>0)
					{
						int TopSideLODLevel = Terrain->LODMap[Row-1][Collumn].Level - LODLevel;
						if (TopSideLODLevel < 0)
							TopSideLODLevel = 0;
						if(SectorPtr->TopSideLODLevel!=TopSideLODLevel)
						{
							SectorPtr->TopSideLODLevel=TopSideLODLevel;
							SectorPtr->TopSideNeedsUpdate=true;
						}
					}

				//Right
					if(Collumn+Span<Terrain->LODLevel[0].Split)
					{
						int RightSideLODLevel = Terrain->LODMap[Row][Collumn+Span].Level - LODLevel;
						if (RightSideLODLevel < 0)
							RightSideLODLevel = 0;
						if(SectorPtr->RightSideLODLevel!=RightSideLODLevel)
						{
							SectorPtr->RightSideLODLevel=RightSideLODLevel;
							SectorPtr->RightSideNeedsUpdate=true;
						}
					}

				//Bottom
					if(Row+Span<Terrain->LODLevel[0].Split)
					{
						int BottomSideLODLevel = Terrain->LODMap[Row+Span][Collumn].Level - LODLevel;
						if (BottomSideLODLevel < 0)
							BottomSideLODLevel = 0;
						if(SectorPtr->BottomSideLODLevel!=BottomSideLODLevel)
						{
							SectorPtr->BottomSideLODLevel=BottomSideLODLevel;
							SectorPtr->BottomSideNeedsUpdate=true;
						}
					}
				}
			}
		}
	}
}
// === END FUNCTION ===



// ======================================
// === BT INTERN FIX SECTOR LOD SEAMS ===
// ======================================
static void BT_Intern_FixSectorLODSeams(s_BT_Sector* SectorPtr)
{
//Fix the seams
	if(SectorPtr->QuadTree!=NULL && SectorPtr->Excluded==false)
	{
		if(SectorPtr->QuadTree->Culled==false && SectorPtr->QuadTree->DrawThis==true)
		{
		//Make sure that the sector is unlocked
			BT_Intern_UnlockSectorVertexData(SectorPtr);

		//Update sides
			// Top
			if(SectorPtr->TopSideNeedsUpdate) {
				SectorPtr->QuadMap->SetSideLOD(0, SectorPtr->TopSideLODLevel);
				SectorPtr->TopSideNeedsUpdate=false;
				SectorPtr->UpdateMesh=true;
			}

			// Right
			if(SectorPtr->RightSideNeedsUpdate) {
				SectorPtr->QuadMap->SetSideLOD(1, SectorPtr->RightSideLODLevel);
				SectorPtr->RightSideNeedsUpdate=false;
				SectorPtr->UpdateMesh=true;
			}

			// Bottom
			if(SectorPtr->BottomSideNeedsUpdate) {
				SectorPtr->QuadMap->SetSideLOD(2, SectorPtr->BottomSideLODLevel);
				SectorPtr->BottomSideNeedsUpdate=false;
				SectorPtr->UpdateMesh=true;
			}

			// Left
			if(SectorPtr->LeftSideNeedsUpdate) {
				SectorPtr->QuadMap->SetSideLOD(3, SectorPtr->LeftSideLODLevel);
				SectorPtr->LeftSideNeedsUpdate=false;
				SectorPtr->UpdateMesh=true;
			}
		}
	}
}
// === END FUNCTION ===



// =======================================
// === BT INTERN CALCULATE CULLING REC ===
// =======================================
static void BT_Intern_CalculateCullingRec(s_BT_terrain* Terrain,s_BT_QuadTree* Quadtree,unsigned long Level,bool IntersectingFrustum){

	//Initialise Culled variable
	if (g_bRenderTerrainForShadowMap == true)
	{
		// no culling when rendering terrain shadow
		Quadtree->Culled = false;
	}
	else
	{
		Quadtree->Culled = true;
		if (IntersectingFrustum == true)
		{
			char Culled = BT_Intern_CullBox(Quadtree->CullBox);
			if (Culled > 0)
			{
				Quadtree->Culled = false;
				if (Culled == 2)
					IntersectingFrustum = false;
			}
		}
		else {
			Quadtree->Culled = false;
		}
	}

//Check if LOD level is greater than 0 and the sector isnt culled
	if(Level>0 && Quadtree->Culled==false)
	{
	//Part 1
		BT_Intern_CalculateCullingRec(Terrain,Quadtree->n1,Level-1,IntersectingFrustum); //-x,-z

	//Part 2
		BT_Intern_CalculateCullingRec(Terrain,Quadtree->n2,Level-1,IntersectingFrustum); //+x,-z

	//Part 3
		BT_Intern_CalculateCullingRec(Terrain,Quadtree->n3,Level-1,IntersectingFrustum); //-x,+z

	//Part 4
		BT_Intern_CalculateCullingRec(Terrain,Quadtree->n4,Level-1,IntersectingFrustum); //+x,+z
	}
}
// === END FUNCTION ===



// =======================================
// === BT INTERN UPDATE CULLBOXES REC ===
// =======================================
static void BT_Intern_UpdateCullBoxesRec(s_BT_terrain* Terrain,s_BT_QuadTree* Quadtree,unsigned long Level){
//Check if the cullbox needs updating
	if(Quadtree->CullboxChanged && Quadtree->Excluded==false)
	{
	//Check if LOD level is greater than 0
		if(Level>0)
		{
		//Part 1
			if(Quadtree->n1->CullboxChanged)
				BT_Intern_UpdateCullBoxesRec(Terrain,Quadtree->n1,Level-1); //-x,-z

		//Part 2
			if(Quadtree->n2->CullboxChanged)
				BT_Intern_UpdateCullBoxesRec(Terrain,Quadtree->n2,Level-1); //+x,-z

		//Part 3
			if(Quadtree->n3->CullboxChanged)
				BT_Intern_UpdateCullBoxesRec(Terrain,Quadtree->n3,Level-1); //-x,+z

		//Part 4
			if(Quadtree->n4->CullboxChanged)
				BT_Intern_UpdateCullBoxesRec(Terrain,Quadtree->n4,Level-1); //+x,+z
		}

	//Update cullboxes
		if(Quadtree->Sector!=NULL)
		{
			Quadtree->CullBox->Top=Quadtree->Sector->Pos_y+Quadtree->Sector->QuadMap->GetHighestPoint();
			Quadtree->CullBox->Bottom=Quadtree->Sector->Pos_y+Quadtree->Sector->QuadMap->GetLowestPoint();
		}else{
			Quadtree->CullBox->Top=max(max(Quadtree->n1->CullBox->Top,Quadtree->n2->CullBox->Top),max(Quadtree->n3->CullBox->Top,Quadtree->n4->CullBox->Top));
			Quadtree->CullBox->Bottom=min(min(Quadtree->n1->CullBox->Bottom,Quadtree->n2->CullBox->Bottom),min(Quadtree->n3->CullBox->Bottom,Quadtree->n4->CullBox->Bottom));
		}
		Quadtree->CullboxChanged=false;
	}
}
// === END FUNCTION ===



// ====================================
// === BT INTERN RENDER TERRAIN REC ===
// ====================================
static void BT_Intern_RenderTerrainRec(s_BT_terrain* Terrain,s_BT_QuadTree* Quadtree,unsigned long Level,int iQualityPass)
{
	// Check if were not at the bottom
	if(Level>0)
	{
		// If we have to draw this LOD level, draw it. If not, continue down the tree
		if(Quadtree->DrawThis && Quadtree->Sector!=NULL) // Check if current LOD level is active and sector is near the camera
		{
			// Only render if iQualityPass is DISTANT
			if ( iQualityPass==1 )
			{
				// Only render if it is below our required TERRAIN SIZE slider setting
				if ( Level < (unsigned long)g_LevelToRender ) BT_Intern_RenderSector(Quadtree->Sector);
			}

		}else{

			if(BT_Main.LODCamPosition.z/Terrain->Scale*C_BT_INTERNALSCALE<Quadtree->PosZ)
			{
				if(BT_Main.LODCamPosition.x/Terrain->Scale*C_BT_INTERNALSCALE<Quadtree->PosX)
				{
				//Part 1
					if(Quadtree->n1->Excluded==false && Quadtree->n1->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n1,Level-1,iQualityPass);    //-x,-z

				//Part 2
					if(Quadtree->n2->Excluded==false && Quadtree->n2->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n2,Level-1,iQualityPass);    //+x,-z

				//Part 3
					if(Quadtree->n3->Excluded==false && Quadtree->n3->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n3,Level-1,iQualityPass);    //-x,+z

				//Part 4
					if(Quadtree->n4->Excluded==false && Quadtree->n4->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n4,Level-1,iQualityPass);   //+x,+z
				}else{
				//Part 2
					if(Quadtree->n2->Excluded==false && Quadtree->n2->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n2,Level-1,iQualityPass);    //+x,-z

				//Part 1
					if(Quadtree->n1->Excluded==false && Quadtree->n1->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n1,Level-1,iQualityPass);    //-x,-z

				//Part 4
					if(Quadtree->n4->Excluded==false && Quadtree->n4->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n4,Level-1,iQualityPass);   //+x,+z

				//Part 3
					if(Quadtree->n3->Excluded==false && Quadtree->n3->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n3,Level-1,iQualityPass);    //-x,+z
				}

			}else{
				if(BT_Main.LODCamPosition.x/Terrain->Scale*C_BT_INTERNALSCALE<Quadtree->PosX)
				{
				//Part 3
					if(Quadtree->n3->Excluded==false && Quadtree->n3->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n3,Level-1,iQualityPass);    //-x,+z

				//Part 4
					if(Quadtree->n4->Excluded==false && Quadtree->n4->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n4,Level-1,iQualityPass);   //+x,+z

				//Part 1
					if(Quadtree->n1->Excluded==false && Quadtree->n1->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n1,Level-1,iQualityPass);    //-x,-z

				//Part 2
					if(Quadtree->n2->Excluded==false && Quadtree->n2->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n2,Level-1,iQualityPass);    //+x,-z
				}else{
				//Part 4
					if(Quadtree->n4->Excluded==false && Quadtree->n4->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n4,Level-1,iQualityPass);   //+x,+z

				//Part 3
					if(Quadtree->n3->Excluded==false && Quadtree->n3->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n3,Level-1,iQualityPass);    //-x,+z

				//Part 2
					if(Quadtree->n2->Excluded==false && Quadtree->n2->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n2,Level-1,iQualityPass);    //+x,-z

				//Part 1
					if(Quadtree->n1->Excluded==false && Quadtree->n1->Culled==false)
						BT_Intern_RenderTerrainRec(Terrain,Quadtree->n1,Level-1,iQualityPass);    //-x,-z
				}
			}
		}
	}
	else
	{
		if(Quadtree->Sector!=NULL)
		{
			// Only render if iQualityPass is NEAR
			if ( iQualityPass==0 )
			{
				BT_Intern_RenderSector(Quadtree->Sector);
			}
		}
	}
}
// === END FUNCTION ===



// ====================================
// === BT INTERN UNLOCK SECTORS REC ===
// ====================================
static void BT_Intern_UnlockSectorsRec(s_BT_terrain* Terrain,s_BT_QuadTree* Quadtree,unsigned long Level)
{
//Check if were not at the bottom
	if(Level>0){
	//If we have to draw this LOD level, draw it. If not, continue down the tree
		if(Quadtree->DrawThis && Quadtree->Sector!=NULL)//Check if current LOD level is active and sector is near the camera
		{
			BT_Intern_UnlockSectorVertexData(Quadtree->Sector);
		}else{
		//Part 1
			if(Quadtree->n1->Excluded==false && Quadtree->n1->Culled==false)
				BT_Intern_UnlockSectorsRec(Terrain,Quadtree->n1,Level-1);    //-x,-z

		//Part 2
			if(Quadtree->n2->Excluded==false && Quadtree->n2->Culled==false)
				BT_Intern_UnlockSectorsRec(Terrain,Quadtree->n2,Level-1);    //+x,-z

		//Part 3
			if(Quadtree->n3->Excluded==false && Quadtree->n3->Culled==false)
				BT_Intern_UnlockSectorsRec(Terrain,Quadtree->n3,Level-1);    //-x,+z

		//Part 4
			if(Quadtree->n4->Excluded==false && Quadtree->n4->Culled==false)
				BT_Intern_UnlockSectorsRec(Terrain,Quadtree->n4,Level-1);   //+x,+z
		}
	}else{
		if(Quadtree->Sector!=NULL){
			BT_Intern_UnlockSectorVertexData(Quadtree->Sector);
		}
	}
}

// === END FUNCTION ===



// ===============================
// === BT INTERN RENDER SECTOR ===
// ===============================
static void BT_Intern_RenderSector(s_BT_Sector* Sector)
{
	//Render sector
	if(Sector->Excluded==false)
	{
		// if sector DBP object has been made universe invisible, it means the engine has occluded it
		//if ( Sector->DBPObject==NULL || (Sector->DBPObject && Sector->DBPObject->bUniverseVisible==true ) ) / 070314 not always work :(
		if ( 1 )
		{
			//Fix LOD seams
			BT_Intern_FixSectorLODSeams(Sector);

			//Check if the sector needs to update its drawbuffer
			//Sector->QuadMap->GenerateMeshData();
			if(Sector->UpdateMesh==true)
			{
				// if often can be locking the index and vertex buffers each time which would be slow
				Sector->QuadMap->UpdateMesh(Sector->DrawBuffer,true);
				Sector->UpdateMesh=false;
			}

			#ifdef DX11

			// calculate world/v/p position of terrain chunk
			
			GGMATRIX World = Sector->WorldMatrix*BT_Main.CurrentRenderTerrain->Object->position.matScale*BT_Main.CurrentRenderTerrain->Object->position.matTranslation;
			if ( m_pCBChangePerTerrsainChunk )
			{
				// as terrain would self-shadow
				if (g_bRenderTerrainForShadowMap == true)
				{
					// sink it a little so not interfere with itself
					World._42 -= 10.0f;
				}

				CBChangePerTerrsainChunk cb;
				cb.mWorld = World;
				GGMatrixTranspose(&cb.mWorld,&cb.mWorld);
				GGGetTransform(GGTS_VIEW,&cb.mView);
				GGGetTransform(GGTS_PROJECTION,&cb.mProjection);
				GGMatrixTranspose(&cb.mView,&cb.mView);
				GGMatrixTranspose(&cb.mProjection,&cb.mProjection);
				m_pImmediateContext->UpdateSubresource( m_pCBChangePerTerrsainChunk, 0, NULL, &cb, 0, 0 );
				m_pImmediateContext->VSSetConstantBuffers ( 0, 1, &m_pCBChangePerTerrsainChunk );
				m_pImmediateContext->PSSetConstantBuffers ( 0, 1, &m_pCBChangePerTerrsainChunk );
			}
			if ( 1==1 )
			{
				CBChangePerTerrsainChunkPS cbps;
				//cbps.vMaterialEmissive = GGCOLOR(pMesh->mMaterial.Emissive.r,pMesh->mMaterial.Emissive.g,pMesh->mMaterial.Emissive.b,pMesh->mMaterial.Emissive.a);
				//if ( pMesh->bAlphaOverride == true )
				//	cbps.fAlphaOverride = (pMesh->dwAlphaOverride>>24)/255.0f;
				//else
				//	cbps.fAlphaOverride = 1.0f;
				cbps.vMaterialEmissive = GGCOLOR(0,0,0,0);
				cbps.fAlphaOverride = 1.0f;

				// feed camera zero matrices into pixel shader constant buffer
				tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( 0 );
				float fDet = 0.0f;
				GGMatrixInverse ( &cbps.mViewInv, &fDet, &m_Camera_Ptr->matView );
				GGMatrixTranspose(&cbps.mViewInv,&cbps.mViewInv);
				//cbps.mViewProj = g_matThisViewProj;
				//GGMatrixTranspose(&cbps.mViewProj,&cbps.mViewProj);
				//cbps.mPrevViewProj = g_matPreviousViewProj;
				//GGMatrixTranspose(&cbps.mPrevViewProj,&cbps.mPrevViewProj);
				m_pImmediateContext->UpdateSubresource( m_pCBChangePerTerrsainChunkPS, 0, NULL, &cbps, 0, 0 );
				m_pImmediateContext->PSSetConstantBuffers ( 1, 1, &m_pCBChangePerTerrsainChunkPS );
			}

			// Index buffers
			//This line is ruin cascade 0 ?
			m_pImmediateContext->IASetIndexBuffer(Sector->DrawBuffer->IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

			// Vertex buffers
			unsigned int stride;
			unsigned int offset;
			stride = Sector->DrawBuffer->FVF_Size;
			offset = 0;
			m_pImmediateContext->IASetVertexBuffers ( 0, 1, &Sector->DrawBuffer->VertexBuffer, &stride, &offset);

			//Draw
			m_pImmediateContext->DrawIndexed(Sector->DrawBuffer->Indices, 0, 0);

			#else
			//World matrix
			IGGDevice* D3DDevice=m_pD3D;
			GGMATRIX World;
			World=Sector->WorldMatrix*BT_Main.CurrentRenderTerrain->Object->position.matScale*BT_Main.CurrentRenderTerrain->Object->position.matTranslation;
			D3DDevice->SetTransform(GGTS_WORLD,&World);

			//DBPRO RENDERING ENGINE
			//Dave - applying effect could be slow
			sMesh* Mesh=Sector->Terrain->Object->pFrame->pMesh;
			if(Mesh->pVertexShaderEffect!=NULL)
				DBPRO_ApplyEffect(Mesh,BT_Main.CurrentUpdateCamera);

			//Index and vertex buffers
			D3DDevice->SetStreamSource(0,Sector->DrawBuffer->VertexBuffer,0,Sector->DrawBuffer->FVF_Size);
			D3DDevice->SetIndices(Sector->DrawBuffer->IndexBuffer);

			//Draw
			D3DDevice->DrawIndexedPrimitive(GGPT_TRIANGLELIST,0,0,Sector->DrawBuffer->Vertices,0,Sector->DrawBuffer->Primitives);

			//Draw edge
			D3DDevice->SetIndices(Sector->DrawBuffer->EdgeLineIndexBuffer);
			D3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST,0,0,Sector->LODLevel->SectorDetail*2,0,Sector->DrawBuffer->EdgeIndexCount/2);
			#endif

			//Stats
			BT_Main.DrawCalls++;
			BT_Main.DrawPrimitiveCount+=Sector->DrawBuffer->Primitives;
			g_pGlob->dwNumberOfPrimCalls++;
			g_pGlob->dwNumberOfPolygonsDrawn+=Sector->DrawBuffer->Primitives;
		}
	}
}
// === END FUNCTION ===


//LEFT = x-size
//RIGHT = x+size
//TOP = y+size
//BOTTOM = y-size
//FRONT = z-size
//BACK = z+size

// ========================================
// === BT INTERN DISTANCE TO LOD CAMERA ===
// ========================================
static float BT_Intern_DistanceToLODCamera(s_BT_terrain* Terrain,s_BT_CullBox* CullBox)
{
//Variables
	float CamX=(BT_Main.LODCamPosition.x-Terrain->Object->position.vecPosition.x)/Terrain->Scale*C_BT_INTERNALSCALE;
	float CamY=(BT_Main.LODCamPosition.y-Terrain->Object->position.vecPosition.y)/Terrain->Scale*C_BT_INTERNALSCALE;
	float CamZ=(BT_Main.LODCamPosition.z-Terrain->Object->position.vecPosition.z)/Terrain->Scale*C_BT_INTERNALSCALE;

//Calculate distances
	float XDistA=-CamX+CullBox->Left;
	float XDistB=CamX-CullBox->Right;
	float YDistA=-CamY+CullBox->Bottom;
	float YDistB=CamY-CullBox->Top;
	float ZDistA=-CamZ+CullBox->Front;
	float ZDistB=CamZ-CullBox->Back;

//Calculate X Distance
	float XDist;
	if(XDistA>0.0f){
		XDist=-XDistA;
	}else if(XDistB>0.0f){
		XDist=XDistB;
	}else{
		XDist=0.0f;
	}

//Calculate Y Distance
	float YDist;
	if(YDistA>0.0f){
		YDist=-YDistA;
	}else if(YDistB>0.0f){
		YDist=YDistB;
	}else{
		YDist=0.0f;
	}

//Calculate Z Distance
	float ZDist;
	if(ZDistA>0.0f){
		ZDist=-ZDistA;
	}else if(ZDistB>0.0f){
		ZDist=ZDistB;
	}else{
		ZDist=0.0f;
	}

//Return distance
	return XDist*XDist+YDist*YDist+ZDist*ZDist;
}
// === END FUNCTION ===



// ================================
// === BT INTERN CONTINUE BUILD ===
// ================================
static void BT_Intern_ContinueBuild()
{
//Variables
	s_BT_Sector* Sector;

//Get sector
	Sector=&BT_Main.CurrentBuildTerrain->LODLevel[BT_Main.CurrentBuildLODLevel].Sector[BT_Main.CurrentBuildSector];

//Check that the sector is not excluded
	if(Sector->Excluded==false)
	{
	//Build
		if(BT_Main.BuildType==true)
		{ // Rebuild
			BT_Intern_BuildSector(Sector);
		}else{ //Firstbuild
			BT_Intern_BuildSector(Sector);
		}
	}

//Set Current values
	BT_Main.CurrentBuildRow=Sector->Row;
	BT_Main.CurrentBuildColumn=Sector->Column;

//Increase sector number
	BT_Main.CurrentBuildSector++;
	BT_Main.CurrentBuildTerrainSector++;

//Check if this LOD Level is finnished
	if(BT_Main.CurrentBuildSector==BT_Main.CurrentBuildTerrain->LODLevel[BT_Main.CurrentBuildLODLevel].Sectors)
	{
		BT_Main.CurrentBuildSector=0;
		BT_Main.CurrentBuildLODLevel++;
	}

	return;
}
// === END FUNCTION ===



// ===============================
// === BT INTERN TERRAIN EXIST ===
// ===============================
bool BT_Intern_TerrainExist(unsigned long terrainid)
{
//Check the range of the value
	if(terrainid>0 && terrainid<=C_BT_MAXTERRAINS)
	{
	//Return the exist varaible
		return BT_Main.Terrains[terrainid].Exists;
	}
	return 0;
}
// === END FUNCTION ===



// =============================
// === BT INTERN IMAGE EXIST ===
// =============================
static long BT_Intern_ImageExist(unsigned long imageid)
{
//Check the range of the value
	if(imageid>0)
	{
	//Check if it exists and return the answer
		return GetImageExistEx(imageid);
	}
	return 0;
}
// === END FUNCTION ===



// =======================
// === BT INTERN ERROR ===
// =======================
void BT_Intern_Error(int number)
{
//Variables
	char Message[100];
	Message[0]=NULL;

//Create message string
	strcat(Message,"BT Error: ");
	strcat(Message,BT_Intern_GetErrorString(number));
	strcat(Message,"\nFunction: ");
	strcat(Message,BT_Intern_GetFunctionName(BT_Main.CurrentFunction));

//Display string and terminate app
	// now use DBP error system
	RunTimeError ( 0, Message );

}
// === END FUNCTION ===



// ==================================
// === BT INTERN GET ERROR STRING ===
// ==================================
static char* BT_Intern_GetErrorString(int number)
{
//Set default error message
	char* Error="Unknown";

//Get error
	if(number==C_BT_ERROR_MAXTERRAINSEXCEDED){
		Error="Max terrains Exceded";
	}else if(number==C_BT_ERROR_TERRAINDOESNTEXIST){
		Error="Terrain doesnt exist";
	}else if(number==C_BT_ERROR_INVALIDLODLEVELS){
		Error="Invalid LOD levels";
	}else if(number==C_BT_ERROR_HEIGHTMAPDOESNTEXIST){
		Error="Heightmap doesnt exist";
	}else if(number==C_BT_ERROR_LODLEVELDOESNTEXIST){
		Error="LOD level doesnt exist";
	}else if(number==C_BT_ERROR_HEIGHTMAPSIZEINVALID){
		Error="Heightmap size invalid";
	}else if(number==C_BT_ERROR_EXCLUSIONMAPSIZEINVALID){
		Error="Exclusion map size invalid";
	}else if(number==C_BT_ERROR_ALREADYBUILDING){
		Error="Already building";
	}else if(number==C_BT_ERROR_OBJECTIDILLEGAL){
		Error="Object ID illegal";
	}else if(number==C_BT_ERROR_TERRAINNOTGENERATED){
		Error="Terrain not generated";
	}else if(number==C_BT_ERROR_CANNOTCREATEVB){
		Error="Cannot create Vertex Buffer";
	}else if(number==C_BT_ERROR_CANNOTCREATEIB){
		Error="Cannot Create Index Buffer";
	}else if(number==C_BT_ERROR_SECTORDOESNTEXIST){
		Error="Sector doesnt exist";
	}else if(number==C_BT_ERROR_TERRAINNOTBUILT){
		Error="Terrain not built";
	}else if(number==C_BT_ERROR_INVALIDFILE){
		Error="Invalid file";
	}else if(number==C_BT_ERROR_VERSIONCANNOTREADFILE){
		Error="Cannot read file";
	}else if(number==C_BT_ERROR_TERRAINALREADYBUILT){
		Error="Terrain already built";
	}else if(number==C_BT_ERROR_CANNOTUSEFUNCTIONONBUILTTERRAIN){
		Error="Cannot use function with a built terrain";
	}else if(number==C_BT_ERROR_SECTORALREADYLOCKED){
		Error="Sector already locked";
	}else if(number==C_BT_ERROR_SECTORNOTUNLOCKED){
		Error="Sector not unlocked";
	}else if(number==C_BT_ERROR_USESFULLVERSION){
		Error="Terrain uses full version features. Cannot load.";
	}else if(number==C_BT_ERROR_SECTORTOOBIG){
		Error="Sectors too big. Increase split.";
	}else if(number==C_BT_ERROR_SPLITTOOHIGH){
		Error="Split too big. (Max: 32)";
	}else if(number==C_BT_ERROR_SECTORALREADYHASOBJECT){
		Error="This sector already has an object";
	}else if(number==C_BT_ERROR_LODLEVELALREADYHASOBJECT){
		Error="This LOD level already has an object";
	}else if(number==C_BT_ERROR_SECTORISEXCLUDED){
		Error="Sector is excluded";
	}else if(number==C_BT_ERROR_MEMORYERROR){
		Error="Memory error";
	}

//Return the error message
	return Error;
}
// === END FUNCTION ===



// ===================================
// === BT INTERN GET FUNCTION NAME ===
// ===================================
static char* BT_Intern_GetFunctionName(int number)
{
//Set default name
	char* Name="Unknown";

//Get name
	if(number==C_BT_FUNCTION_MAKETERRAIN){
		Name="BT MakeTerrain";
	}else if(number==C_BT_FUNCTION_SETTERRAINHEIGHTMAP){
		Name="BT SetTerrainHeightmap";
	}else if(number==C_BT_FUNCTION_SETTERRAINTEXTURE){
		Name="BT SetTerrainTexture";
	}else if(number==C_BT_FUNCTION_SETTERRAINEXCLUSION){
		Name="BT SetTerrainExclusion";
	}else if(number==C_BT_FUNCTION_SETTERRAINDETAIL){
		Name="BT SetTerrainDetail";
	}else if(number==C_BT_FUNCTION_SETTERRAINENVIRONMENT){
		Name="BT SetTerrainEnvironment";
	}else if(number==C_BT_FUNCTION_SETTERRAINLOD){
		Name="BT SetTerrainLOD";
	}else if(number==C_BT_FUNCTION_SETTERRAINSPLIT){
		Name="BT SetTerrainSplit";
	}else if(number==C_BT_FUNCTION_SETTERRAINDETAILTILE){
		Name="BT SetTerrainDetailTile";
	}else if(number==C_BT_FUNCTION_SETTERRAINQUADREDUCTION){
		Name="BT SetTerrainQuadReduction";
	}else if(number==C_BT_FUNCTION_SETTERRAINQUADROTATION){
		Name="BT SetTerrainQuadRotation";
	}else if(number==C_BT_FUNCTION_SETTERRAINSMOOTHING){
		Name="BT SetTerrainSmoothing";
	}else if(number==C_BT_FUNCTION_SETTERRAINSCALE){
		Name="BT SetTerrainScale";
	}else if(number==C_BT_FUNCTION_SETTERRAINYSCALE){
		Name="BT SetTerrainYScale";
	}else if(number==C_BT_FUNCTION_SETTERRAINLODDISTANCES){
		Name="BT SetTerrainLODDistances";
	}else if(number==C_BT_FUNCTION_BUILDTERRAIN){
		Name="BT BuildTerrain";
	}else if(number==C_BT_FUNCTION_CONTINUEBUILD){
		Name="BT ContinueBuild";
	}else if(C_BT_FUNCTION_TERRAINEXIST){
		Name="BT TerrainExist";
	}else if(number==C_BT_FUNCTION_DELETETERRAIN){
		Name="BT DeleteTerrain";
	}else if(number==C_BT_FUNCTION_GETGROUNDHEIGHT){
		Name="BT GetGroundHeight";
	}else if(number==C_BT_FUNCTION_GETTERRAINSIZE){
		Name="BT GetTerrainSize";
	}else if(number==C_BT_FUNCTION_GETVERSION){
		Name="BT GetVersion";
	}else if(number==C_BT_FUNCTION_SETBUILDSTEP){
		Name="BT SetBuildStep";
	}else if(number==C_BT_FUNCTION_UPDATETERRAIN){
		Name="BT UpdateTerrain";
	}else if(number==C_BT_FUNCTION_UPDATE){
		Name="BT Update";
	}else if(number==C_BT_FUNCTION_RENDER){
		Name="BT Render";
	}else if(number==C_BT_FUNCTION_ENABLEAUTORECOVERY){
		Name="BT EnableAutoRecovery";
	}else if(number==C_BT_FUNCTION_GETSTATISTIC){
		Name="BT GetStatistic";
	}else if(number==C_BT_FUNCTION_CLEARSTATISTICS){
		Name="BT ClearStatistics";
	}else if(number==C_BT_FUNCTION_GETOBJECTID){
		Name="BT GetObjectID";
	}else if(number==C_BT_FUNCTION_MAKESECTOROBJECT){
		Name="BT MakeSectorObject";
	}else if(number==C_BT_FUNCTION_GETSECTORPOSITIONX){
		Name="BT GetSectorPositionX";
	}else if(number==C_BT_FUNCTION_GETSECTORPOSITIONY){
		Name="BT GetSectorPositionY";
	}else if(number==C_BT_FUNCTION_GETSECTORPOSITIONZ){
		Name="BT GetSectorPositionZ";
	}else if(number==C_BT_FUNCTION_GETSECTORCOUNT){
		Name="BT GetSectorCount";
	}else if(number==C_BT_FUNCTION_GETSECTORSIZE){
		Name="BT GetSectorSize";
	}else if(number==C_BT_FUNCTION_GETSECTOREXCLUDED){
		Name="BT GetSectorExcluded";
	}else if(number==C_BT_FUNCTION_GETSECTORROW){
		Name="BT GetSectorRow";
	}else if(number==C_BT_FUNCTION_GETSECTORCOLLUMN){
		Name="BT GetSectorCollumn";
	}else if(number==C_BT_FUNCTION_SETCURRENTCAMERA){
		Name="BT SetCurrentCamera";
	}else if(number==C_BT_FUNCTION_UPDATETERRAINLOD){
		Name="BT UpdateTerrainLOD";
	}else if(number==C_BT_FUNCTION_UPDATETERRAINCULL){
		Name="BT UpdateTerrainCull";
	}else if(number==C_BT_FUNCTION_SETPOINTHEIGHT){
		Name="BT SetPointHeight";
	}else if(number==C_BT_FUNCTION_SETTERRAINEXCLUSIONTHRESHOLD){
		Name="BT SetTerrainExclusionThreshold";
	}else if(number==C_BT_FUNCTION_LOADTERRAIN){
		Name="BT LoadTerrain";
	}else if(number==C_BT_FUNCTION_SAVETERRAIN){
		Name="BT SaveTerrain";
	}else if(number==C_BT_FUNCTION_GETPOINTEXCLUDED){
		Name="BT GetPointExcluded";
	}else if(number==C_BT_FUNCTION_SETATMODE){
		Name="BT SetATMode";
	}else if(number==C_BT_FUNCTION_SETTERRAINDETAILBLENDMODE){
		Name="BT SetTerrainDetailBlendMode";
	}else if(number==C_BT_FUNCTION_INIT){
		Name="BT Init";
	}else if(number==C_BT_FUNCTION_ADDTERRAINENVIRONMENT){
		Name="BT AddTerrainEnvironment";
	}else if(number==C_BT_FUNCTION_GETPOINTENVIRONMENT){
		Name="BT GetPointEnvironment";
	}else if(number==C_BT_FUNCTION_GETTERRAININFO){
		Name="BT GetTerrainInfo";
	}else if(number==C_BT_FUNCTION_GETLODLEVELINFO){
		Name="BT GetLODLevelInfo";
	}else if(number==C_BT_FUNCTION_GETSECTORINFO){
		Name="BT GetSectorInfo";
	}else if(number==C_BT_FUNCTION_MAKETERRAINOBJECT){
		Name="BT MakeTerrainObject";
	}else if(number==C_BT_FUNCTION_ENABLEAUTORENDER){
		Name="BT EnableAutoRender";
	}

//Return the name
	return Name;
}
// === END FUNCTION ===



// ====================================
// === BT INTERN GET SECTOR HEIGHTS ===
// ====================================
static void BT_Intern_GetSectorHeights(s_BT_terrain* Terrain,unsigned long LODLevel,unsigned long row,unsigned long column,float* buffer)
{
//Variables
	unsigned long StartX;
	unsigned long StartY;
	unsigned long X;
	unsigned long Y;
	unsigned long Xb;
	unsigned long Yb;
	unsigned long BufferPos;
	unsigned long HeightPos;

//Set start positions
	StartX=column*Terrain->LODLevel[LODLevel].SectorDetail*Terrain->LODLevel[LODLevel].TileSpan;
	StartY=row*Terrain->LODLevel[LODLevel].SectorDetail*Terrain->LODLevel[LODLevel].TileSpan;

//Loop through points
	for(Y=0;Y<=Terrain->LODLevel[LODLevel].SectorDetail;Y++)
	{
		for(X=0;X<=Terrain->LODLevel[LODLevel].SectorDetail;X++)
		{
			BufferPos=(X+Y*(Terrain->LODLevel[LODLevel].SectorDetail+1));

			if(X*Terrain->LODLevel[LODLevel].TileSpan+StartX>unsigned(Terrain->Heightmapsize-1) )
			{
				Xb=X*Terrain->LODLevel[LODLevel].TileSpan-1;
			}else{
				Xb=X*Terrain->LODLevel[LODLevel].TileSpan;
			}

			if(Y*Terrain->LODLevel[LODLevel].TileSpan+StartY>unsigned(Terrain->Heightmapsize-1))
			{
				Yb=Y*Terrain->LODLevel[LODLevel].TileSpan-1;
			}else{;
				Yb=Y*Terrain->LODLevel[LODLevel].TileSpan;
			}

			HeightPos=((Xb+StartX)+(Yb+StartY)*Terrain->Heightmapsize);
			buffer[BufferPos]=Terrain->HeightPoint[HeightPos];
		}
	}
}
