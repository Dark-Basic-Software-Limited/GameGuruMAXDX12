void imgui_importer_loop(void)
{
	switch (iDelayedExecute) 
	{
		case 2: //Quit Importer, need to be done heree , so we dont have any images in the draw call list.
		{
			iDelayedExecute = 0;
			importer_quit();
			bImporter_Window = false;
			bBatchConverting = false;
			batchFileList.clear();
			break;
		}
		case 1:
		{
			iDelayedExecute = 0;
			t.tFileName_s = openFileBox("All Files|*.*|PNG|*.png|DDS|*.dds|JPEG|*.jpg|BMP|*.bmp|", "", "Open Texture", ".dds", IMPORTEROPENFILE);
			if (t.tFileName_s == "Error")  return;
			if (FileExist(t.tFileName_s.Get()) == 1)
			{
				popup_text("Loading chosen texture and associated files");

				// find free image
				t.tImageID = t.importerTextures[iDelayedExecuteSelection].imageID;
				if (t.tImageID == 0)
				{
					t.tImageID = g.importermenuimageoffset + 15;
					while (ImageExist(t.tImageID) == 1) ++t.tImageID;
				}

				// can expand out a color texture once (to add normal/gloss/etc)
				bool bExpandOutPBRTextureSet = false;

				// replace image details
				if (ImageExist(t.tImageID) == 1) DeleteImage(t.tImageID);
				LoadImage(t.tFileName_s.Get(), t.tImageID);
				if (ImageExist(t.tImageID) == 1)
				{
					// remove any previous references to associated files for the old filename
					if (t.importerTextures[iDelayedExecuteSelection].iExpandedThisSlot == 0)
					{
						// but only if its a base color texutre
						char pIsItTexColor[2048];
						strcpy(pIsItTexColor, t.importerTextures[iDelayedExecuteSelection].fileName.Get());
						if (strlen(pIsItTexColor) > 1 + 8 + 4)
						{
							pIsItTexColor[strlen(pIsItTexColor) - 4] = 0;
							if (strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 2, "_d", 2) == NULL
								|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 6, "_color", 6) == NULL
								|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 8, "_diffuse", 8) == NULL
								|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 7, "_albedo", 7) == NULL
								|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 8, "blankTex", 8) == NULL)
							{
								// for both!
								strcpy(pIsItTexColor, t.tFileName_s.Get());
								if (strlen(pIsItTexColor) > 1 + 8 + 4)
								{
									pIsItTexColor[strlen(pIsItTexColor) - 4] = 0;
									if (strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 2, "_d", 2) == NULL
										|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 6, "_color", 6) == NULL
										|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 8, "_diffuse", 8) == NULL
										|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 7, "_albedo", 7) == NULL)
									{
										importer_removeentryandassociatesof(iDelayedExecuteSelection);
										t.importerTextures[iDelayedExecuteSelection].iExpandedThisSlot = 1;
										bExpandOutPBRTextureSet = true;
									}
								}
							}
						}
					}

					// update image list data
					t.importerTextures[iDelayedExecuteSelection].fileName = t.tFileName_s;
					t.importerTextures[iDelayedExecuteSelection].imageID = t.tImageID;
				}

				int iImageCount = 0;
				for (int iCount = 1; iCount <= IMPORTERTEXTURESMAX; iCount++)
					if (strlen(t.importerTextures[iCount].fileName.Get()) > 0)
						iImageCount++;

				// ensure single texture is specified in FPE
				if (iImageCount == 1)
				{
					t.importer.objectFPE.textured = t.tFileName_s;
				}

				// reapply texture to model
				importer_applyimagelisttextures(false, iDelayedExecuteSelection, bExpandOutPBRTextureSet);
				importer_recreate_texturesprites();

				iImporterGenerateThumb = 0;
			}
			break;
		}
		case 3: //Save entity
		{
			iDelayedExecute = 0;
			t.timportersaveon = 1;
			cstr pFillFilename = cstr(cImportPath);
			if (cImportPath[strlen(cImportPath) - 1] != '\\')
				pFillFilename = pFillFilename + "\\";

			cstr pRelativePathAndFileToFPE;
			pRelativePathAndFileToFPE = pFillFilename + cImportName + ".fpe";

			pFillFilename = pFillFilename + cImportName + ".dbo";

			//Dont resolve if not using relative path.
			if (cImportPath[1] != ':') 
			{
				char resolved[MAX_PATH];
				strcpy(resolved, g.fpscrootdir_s.Get());
				strcat(resolved, "\\Files\\");
				strcat(resolved, pFillFilename.Get());
				pFillFilename = resolved;
			}

			bool bOverwritingExisting = false;
			if (bBatchConverting == false)
			{
				bool bShouldSave = true;
				if (FileExist(pFillFilename.Get()))
				{
					bShouldSave = overWriteFileBox(pFillFilename.Get());
					bOverwritingExisting = true;
				}
				if (bShouldSave==false)
				{
					break;
				}
			}

			// the actual save
			importer_save_entity(pFillFilename.Get());

			// afte an overwrite, need to thoroughly replace existing FPE with new one!
			if (bOverwritingExisting == true)
			{
				extern void CheckExistingFilesModified(bool);
				CheckExistingFilesModified(false);
			}

			// single or batch process
			if (bBatchConverting == true)
			{
				// continue batch process after large preview event
				iDelayedExecute = 5;
			}
			else
			{
				if (t.tSaveFile_s == "Error")
				{
					// Failed. ? or cancel ?
					popup_text("Cancel Save Object");
				}
				else
				{
					iDelayedExecute = 2;
				}
			}
			break;
		}
		case 4 : 
		{
			// trigger importer to quit, then reload with last loaded model (used when scaling mode changes)
			iDelayedExecute = 0; 
			importer_storeobjectdata();
			extern char pLaunchAfterSyncPreSelectModel[MAX_PATH];
			strcpy (pLaunchAfterSyncPreSelectModel, "");
			extern void importer_quit_for_reload (LPSTR pOptionalCopyModelFile);
			importer_quit_for_reload(pLaunchAfterSyncPreSelectModel); 
			extern int iLaunchAfterSync;
			iLaunchAfterSync = 8;
			break;
		}
		case 5:
		{
			// no longer does the preview thumb adjustment when in batch mode
			iDelayedExecute = 0;
			importer_storeobjectdata();
			extern char pLaunchAfterSyncPreSelectModel[MAX_PATH];
			strcpy (pLaunchAfterSyncPreSelectModel, "");
			extern void importer_quit_for_reload (LPSTR pOptionalCopyModelFile);
			importer_quit_for_reload(pLaunchAfterSyncPreSelectModel);
			extern int iLaunchAfterSync;
			iLaunchAfterSync = 8;
			bBatchConverting = true;
			break;
		}

		case 30: //Select baseColorMap Material
		{
			bHaveMaterialUpdate = true;

			importer_apply_materialformesh(MaterialComponentTEXTURESLOT::BASECOLORMAP, GG_MESH_TEXTURE_DIFFUSE);

			// auto generate other tex references under certain conditions
			char* pTextureFilename = pSelectedMesh->pTextures[GG_MESH_TEXTURE_DIFFUSE].pName;
			bool bIfColorValid = false;
			if (pSelectedMesh)
			{
				if (pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource)
				{
					// check if we can auto fill other tex refs
					if (strstr(pTextureFilename, "_color") != NULL) bIfColorValid = true;
				}
			}
			if (bIfColorValid == true)
			{
				// check if all other fields blank
				bool bAndAllOtherTexRefsEmptyAutoFill = false;
				if (pSelectedMesh->dwTextureCount > 1)
				{
					// scan all tex refs, all need to be blank
					bAndAllOtherTexRefsEmptyAutoFill = true;
					for (int n = 1; n < pSelectedMesh->dwTextureCount; n++)
					{
						if (strlen(pSelectedMesh->pTextures[n].pName) > 0)
						{
							// tex refs populated, no leave refs alone
							bAndAllOtherTexRefsEmptyAutoFill = false;
						}
					}
				}
				else
				{
					// texture array of one means no other tex refs
					bAndAllOtherTexRefsEmptyAutoFill = true;
				}
				if (bAndAllOtherTexRefsEmptyAutoFill == true)
				{
					// this model has blank tex refs, can auto fill

					// ensure the DBO mesh texture is large enough to hold references
					if (pSelectedMesh->dwTextureCount <= GG_MESH_TEXTURE_SURFACE)
					{
						// if texture storage too small, increase it to store our references!
						extern bool EnsureTextureStageValid(sMesh* pMesh, int iTextureStage);
						EnsureTextureStageValid(pSelectedMesh, GG_MESH_TEXTURE_SURFACE);
					}

					// finds base tex ref name
					char pNoExtFilename[MAX_PATH];
					strcpy(pNoExtFilename, pTextureFilename);
					pNoExtFilename[strlen(pNoExtFilename) - 4] = 0;
					LPSTR pNoColorPtr = strstr(pNoExtFilename, "_color"); if (pNoColorPtr != NULL) *pNoColorPtr = 0;

					// populate with auto tex refs (if present)
					char pTryTexRef[MAX_PATH];
					sprintf(pTryTexRef, "%s_normal.dds", pNoExtFilename);
					if (FileExist(pTryTexRef) == 1)
					{
						strcpy(cPreSelectedFile, pTryTexRef);
						importer_apply_materialformesh(MaterialComponentTEXTURESLOT::NORMALMAP, GG_MESH_TEXTURE_NORMAL);
					}
					sprintf(pTryTexRef, "%s_surface.dds", pNoExtFilename);
					if (FileExist(pTryTexRef) == 1)
					{
						// surface and RMA references all in one file
						strcpy(cPreSelectedFile, pTryTexRef);
						importer_apply_materialformesh(MaterialComponentTEXTURESLOT::SURFACEMAP, GG_MESH_TEXTURE_SURFACE);
						strcpy(pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, pTryTexRef);
						pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask = (15) + (1 << 4);
						strcpy(pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, pTryTexRef);
						pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask = (15) + (2 << 4);
						strcpy(pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, pTryTexRef);
						pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].channelMask = (15) + (0 << 4);
					}
					else
					{
						// do not look for non-surface files, they could be anything!
					}
					sprintf(pTryTexRef, "%s_emissive.dds", pNoExtFilename);
					if (FileExist(pTryTexRef) == 1)
					{
						strcpy(cPreSelectedFile, pTryTexRef);
						importer_apply_materialformesh(MaterialComponentTEXTURESLOT::EMISSIVEMAP, GG_MESH_TEXTURE_EMISSIVE);
					}
				}
			}
			t.importer.bMeshesHaveDifferentBase = true;
			iDelayedExecute = 0;
			break;
		}
		case 31: //Select NormalMap Material
		{
			bHaveMaterialUpdate = true;
			importer_apply_materialformesh(MaterialComponentTEXTURESLOT::NORMALMAP, GG_MESH_TEXTURE_NORMAL);
			t.importer.bMeshesHaveDifferentNormal = true;
			iDelayedExecute = 0;
			break;
		}
		case 32: //Select surfaceMap Material
		{
			bHaveMaterialUpdate = true;
			importer_apply_materialformesh(MaterialComponentTEXTURESLOT::SURFACEMAP, GG_MESH_TEXTURE_SURFACE);
			t.importer.bMeshesHaveDifferentSurface = true;
			t.importer.bEditingAllSurfaceMeshes = false;
			iDelayedExecute = 0;
			break;
		}

		case 33: //Select displacementMap Material
		{
			bHaveMaterialUpdate = true;
			if (pSelectedMaterial && !pSelectedMesh) 
			{
				pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource = nullptr;
				pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name = "";
				pSelectedMaterial->SetDirty();
				wiJobSystem::context ctx;
				wiJobSystem::Wait(ctx);
			}

			if (pSelectedMesh)
			{
				char* pTextureFilename = pSelectedMesh->pTextures[0].pName;
				iDelayedExecute = 0;

				cStr tOldDir = GetDir();
				char * cFileSelected;
				if (strlen(cPreSelectedFile) > 0)
					cFileSelected = &cPreSelectedFile[0];
				else
					cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0DDS\0*.dds\0PNG\0*.png\0JPEG\0*.jpg\0TGA\0*.tga\0BMP\0*.bmp\0\0\0", NULL, NULL);

				SetDir(tOldDir.Get());
				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name = cFileSelected;
					pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource = WickedCall_LoadImage(pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name);
					if (pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource)
					{
						//worked activate.
						pSelectedMaterial->SetParallaxOcclusionMapping(0.05f);// SetDisplacementMapping(0.1f); //Default.
						pSelectedMaterial->SetDirty();
						wiJobSystem::context ctx;
						wiJobSystem::Wait(ctx);
					}
					else 
					{
						//Failed reset slot.
						pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource = nullptr;
						pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name = "";
						pSelectedMaterial->SetDirty();
						wiJobSystem::context ctx;
						wiJobSystem::Wait(ctx);
					}
				}
			}
			t.importer.bMeshesHaveDifferentDisplacement = true;
			iDelayedExecute = 0;
			break;
		}

		case 34: //Select emissiveMap Material
		{
			bHaveMaterialUpdate = true;
			importer_apply_materialformesh(MaterialComponentTEXTURESLOT::EMISSIVEMAP, GG_MESH_TEXTURE_EMISSIVE);
			t.importer.bMeshesHaveDifferentEmissive = true;
			iDelayedExecute = 0;
			break;
		}

		case 35: //Select occlusionMap Material
		{
			bHaveMaterialUpdate = true;
			if (pSelectedMaterial && !pSelectedMesh) 
			{
				pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource = nullptr;
				pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name = "";
				pSelectedMaterial->SetOcclusionEnabled_Primary(true);
				pSelectedMaterial->SetOcclusionEnabled_Secondary(false);
				pSelectedMaterial->SetDirty();
				wiJobSystem::context ctx;
				wiJobSystem::Wait(ctx);
			}

			if (pSelectedMesh)
			{
				char* pTextureFilename = pSelectedMesh->pTextures[0].pName;
				iDelayedExecute = 0;

				cStr tOldDir = GetDir();
				char * cFileSelected;
				if (strlen(cPreSelectedFile) > 0)
					cFileSelected = &cPreSelectedFile[0];
				else
					cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0DDS\0*.dds\0PNG\0*.png\0JPEG\0*.jpg\0TGA\0*.tga\0BMP\0*.bmp\0\0\0", NULL, NULL);

				SetDir(tOldDir.Get());
				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name = cFileSelected;
					pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource = WickedCall_LoadImage(pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name);
					if (pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource)
					{
						//worked activate.
						pSelectedMaterial->SetOcclusionEnabled_Primary(false);
						pSelectedMaterial->SetOcclusionEnabled_Secondary(true);
						pSelectedMaterial->SetDirty();
						wiJobSystem::context ctx;
						wiJobSystem::Wait(ctx);
					}
					else 
					{
						//Failed reset slot.
						pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource = nullptr;
						pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name = "";
						pSelectedMaterial->SetOcclusionEnabled_Primary(true);
						pSelectedMaterial->SetOcclusionEnabled_Secondary(false);
						pSelectedMaterial->SetDirty();
						wiJobSystem::context ctx;
						wiJobSystem::Wait(ctx);
					}
				}
			}
			iDelayedExecute = 0;
			break;
		}

		case 41: // Select OCCLUSION TEXTURE (to form SURFACE material)
		case 42: // Select ROUGHNESS TEXTURE (to form SURFACE material)
		case 43: // Select METALNESS TEXTURE (to form SURFACE material)
		case 44: // Select SURFACE TEXTURE (for when in custom materials)
		{
			// reference to original surface file
			char pOrigSurfaceFile[MAX_PATH];
			strcpy (pOrigSurfaceFile, "");

			// clear previous surface map and unload surface texture file (as we are replacing it here)
			bHaveMaterialUpdate = true;
			if (pSelectedMaterial)
			{
				strcpy (pOrigSurfaceFile, pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str());
			}

			// if a mesh selected for changing
			char newSurfaceFileTemp[MAX_PATH];
			strcpy(newSurfaceFileTemp, "");
			if (pSelectedMesh)
			{
				// base color name helps create unique temp surface files
				LPSTR pBaseColorTexFile = pSelectedMesh->pTextures[GG_MESH_TEXTURE_DIFFUSE].pName;
				char pBaseColorTextName[MAX_PATH];
				strcpy(pBaseColorTextName, "noname");
				if (strlen(pBaseColorTexFile) > 0)
				{
					for (int n = strlen(pBaseColorTexFile) - 1; n > 0; n--)
					{
						if (pBaseColorTexFile[n] == '\\' || pBaseColorTexFile[n] == '/')
						{
							strcpy (pBaseColorTextName, pBaseColorTexFile + n + 1);
							break;
						}
					}
					for (int n = strlen(pBaseColorTextName) - 1; n > 0; n--)
					{
						if (pBaseColorTextName[n] == '_')
						{
							pBaseColorTextName[n] = 0;
							break;
						}
					}
				}

				// call file requester to get new texture
				char* cFileSelected = "";
				if (iDelayedExecuteChannel != -2)
				{
					// ask for file or shoose one passed in
					if (strlen(cPreSelectedFile) > 0)
					{
						cFileSelected = &cPreSelectedFile[0];
					}
					else
					{
						cStr tOldDir = GetDir();
						bool bChoosingSurface = bChooseSurfaceChannel;
						if (bChooseSurfaceChannel) bChooseSurfaceChannel = false;
						if(pSelectedMaterial)
							cFileSelected = importer_selectfile(MaterialComponentTEXTURESLOT::SURFACEMAP, pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name, !bChoosingSurface);
						else
							cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0DDS\0*.dds\0PNG\0*.png\0JPEG\0*.jpg\0TGA\0*.tga\0BMP\0*.bmp\0\0\0", NULL, NULL);
						SetDir(tOldDir.Get());
					}

					// if do not select a texture, consider this erasing the texture data
					if (cFileSelected == NULL) cFileSelected = "";
					if (strlen(cFileSelected) == 0)
					{
						iDelayedExecuteChannel = -2;

						break;
					}
					else
					{
						if (strlen(pOrigSurfaceFile) > 0) WickedCall_DeleteImage(pOrigSurfaceFile);
						pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource = nullptr;
						pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name = "";
						pSelectedMaterial->SetDirty();
						wiJobSystem::context ctx;
						wiJobSystem::Wait(ctx);
					}
				}

				// ensure the DBO mesh texture is large enough to hold references
				if (pSelectedMesh->dwTextureCount <= GG_MESH_TEXTURE_SURFACE)
				{
					// if texture storage too small, increase it to store our references!
					extern bool EnsureTextureStageValid (sMesh* pMesh, int iTextureStage);
					EnsureTextureStageValid (pSelectedMesh, GG_MESH_TEXTURE_SURFACE);
				}

				// create bitmask to specify channel we are taking the data from
				unsigned char channelMask = 0;
				if (iDelayedExecuteChannel == -1) channelMask = (15) + (0 << 4); // red default
				if (iDelayedExecuteChannel >= 0) channelMask = (15) + (iDelayedExecuteChannel << 4);

				// assign channel masj to the specific texture ref
				if (iDelayedExecute == 41)
				{
					pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].channelMask = channelMask;
				}
				if (iDelayedExecute == 42)
				{
					if (pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask == 0 && channelMask > 0)
					{
						// if newly added data, set a default strength
						pSelectedMaterial->roughness = 1.0f;
						pSelectedMaterial->SetRoughness(pSelectedMaterial->roughness);
						pSelectedMaterial->SetDirty();
					}
					pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask = channelMask;
				}
				if (iDelayedExecute == 43)
				{
					if (pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask == 0 && channelMask > 0)
					{
						// if newly added data, set a default strength
						pSelectedMaterial->metalness = 1.0f;
						pSelectedMaterial->SetMetalness(pSelectedMaterial->metalness);
						pSelectedMaterial->SetDirty();
					}
					pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask = channelMask;
				}
				if (iDelayedExecute == 44)
				{
					// changing surface texture!
				}

				// store name in DBO Mesh in correct slot
				if (iDelayedExecute == 41) strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, cFileSelected);
				if (iDelayedExecute == 42) strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, cFileSelected);
				if (iDelayedExecute == 43) strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, cFileSelected);
				if (iDelayedExecute == 44)
				{
					strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, cFileSelected);
					strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, cFileSelected);
					strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, cFileSelected);
					strcpy (pSelectedMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName, cFileSelected);
				}

				// do not generate surface if picked it directly (custom materials mode)
				if ( iDelayedExecute != 44 )
				{
					// source is textures specified in DBO mesh (or if none exist, use original surface)
					LPSTR pAO = "", pGloss = "", pMetal = "";
					pAO = pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName;
					pGloss = pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName;
					pMetal = pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName;

					// determine which channel the original ao,gloss,metal data resides for each file
					int iOcclusionChannel = 0, iRoughnessChannel = 0, iMetalnessChannel = 0, iReflectanceChannel = 3;
					unsigned char channelAOMask = pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].channelMask;
					iOcclusionChannel = (channelAOMask >> 4) & (3);
					unsigned char channelGlossMask = pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask;
					iRoughnessChannel = (channelGlossMask >> 4) & (3);
					unsigned char channelMetalMask = pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask;
					iMetalnessChannel = (channelMetalMask >> 4) & (3);

					int iAOLength = strlen(pAO);
					int iGlossLength = strlen(pGloss);
					int iMetalLength = strlen(pMetal);

					// if there is no original raw texture file for ao,gloss and metal, assume it is a _surface texture (r=ao,g=gloss,b=metal,a=refl)
					if (iAOLength == 0) { pAO = pOrigSurfaceFile; iOcclusionChannel = 0; }
					if (iGlossLength == 0) { pGloss = pOrigSurfaceFile; iRoughnessChannel = 1; }
					if (iMetalLength == 0) { pMetal = pOrigSurfaceFile; iMetalnessChannel = 2; }

					// if reference indicates has no data channels, ensure surface created with defaults for that channel
					if (pSelectedMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].channelMask == 0)
					{
						pAO = "";
					}
					if (pSelectedMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask == 0)
					{
						pGloss = "";
					}
					if (pSelectedMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask == 0)
					{
						pMetal = "";
					}

					// Calculate string length again incase they have been changed.
					iAOLength = strlen(pAO);
					iGlossLength = strlen(pGloss);
					iMetalLength = strlen(pMetal);

					// Generate new surface map
					strcpy(newSurfaceFileTemp, GG_GetWritePath());
					strcat(newSurfaceFileTemp, "imported_models\\");

					// Get the name of the source file
					cstr sourceFile = importer_getfilenameonly((LPSTR)cFileSelected);
					
					bool bNameChosen = false;
					// If source file is a _surface.dds file, we can copy the name.
					if (strcmp(sourceFile.Get() + strlen(sourceFile.Get()) - strlen("_surface.dds"), "_surface.dds") == 0)
					{
						if (strlen(sourceFile.Get()) >= strlen("_surface.dds"))
						{
							strcat(newSurfaceFileTemp, sourceFile.Get());
							bNameChosen = true;
						}
					}
					
					if (!bNameChosen)
					{
						// Try to base the name off the color texture.
						sourceFile = importer_getfilenameonly((LPSTR)pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str());

						if (sourceFile.Len() > 0)
						{
							char* replace = strstr(sourceFile.Get(), "_color.dds");
							if (replace)
							{
								strcpy(replace, "_surface.dds");
								strcat(newSurfaceFileTemp, sourceFile.Get());
								bNameChosen = true;
							}
						}
					}

					if (!bNameChosen)
					{
						// Base the name off of the chosen filename.
						sourceFile = importer_getfilenameonly((LPSTR)cFileSelected);
						char file[MAX_PATH];
						strcpy(file, sourceFile.Get());
						for (int i = strlen(file)-1; i >= 0; i--)
						{
							if (file[i] == '.')
							{
								strcpy(file + i, "_surface.dds");
								strcat(newSurfaceFileTemp, file);
								bNameChosen = true;
								break;
							}
						}
					}

					// generate surface texture from sources above
					ImageCreateSurfaceTextureChannels(newSurfaceFileTemp, pAO, pGloss, pMetal, iOcclusionChannel, iRoughnessChannel, iMetalnessChannel, iReflectanceChannel);

					// The texture will be copied from the imported_models folder when it is added to the object library. Afterwards, it can be deleted.
					t.importer.pSurfaceFilesToDelete.push_back(newSurfaceFileTemp);
				}
				else
				{
					// the surface file selected
					strcpy ( newSurfaceFileTemp, cFileSelected );
				}

				// recreate surface material from newly created surface file
				pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name = newSurfaceFileTemp;
				pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource = WickedCall_LoadImage(pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name);
				if (pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource)
				{
					// successfully loaded surface texture
				}
				else 
				{
					//Failed reset slot.
					pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource = nullptr;
					pSelectedMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name = "";
				}
				pSelectedMaterial->SetDirty();
				wiJobSystem::context ctx;
				wiJobSystem::Wait(ctx);

				// resets
				iDelayedExecuteChannel = -1;
			}

			t.importer.bMeshesHaveDifferentSurface = true;
			iDelayedExecute = 0;

			// special mode fvor when editing surface RGBA but for ALL meshes
			if (t.importer.bEditingAllSurfaceMeshes == true)
			{
				strcpy (cPreSelectedFile, newSurfaceFileTemp);
				importer_texture_all_meshes(MaterialComponentTEXTURESLOT::SURFACEMAP);
				t.importer.bMeshesHaveDifferentSurface = false;
			}

			break;
		}

		// Replace the normal map texture with a copy that has the green channel inverted.
		case 45:
		{
			if (!pSelectedMesh)
			{
				bHaveMaterialUpdate = false;
				iDelayedExecute = 0;
				break;
			}

			if (!pSelectedMesh->pTextures)
			{
				bHaveMaterialUpdate = false;
				iDelayedExecute = 0;
				break;
			}

			if (!pSelectedMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName)
			{
				bHaveMaterialUpdate = false;
				iDelayedExecute = 0;
				break;
			}

			int iFoundFilename = -1;
			char newNormalMapFile[MAX_PATH];
			strcpy(newNormalMapFile, GG_GetWritePath());
			char originalFile[MAX_PATH];
			strcpy(originalFile, "");
			strcat(newNormalMapFile, "imported_models\\");
			strcpy(originalFile, pSelectedMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName);
			// Find filename without path or file extension.
			for (int n = strlen(originalFile) - 1; n > 0; n--)
			{
				if (originalFile[n] == '\\' || originalFile[n] == '/')
				{
					iFoundFilename = n;
					break;
				}
			}
			if (iFoundFilename != -1)
			{
				char pFilenameOnly[512];
				strcpy(pFilenameOnly, originalFile + iFoundFilename + 1);
				pFilenameOnly[strlen(pFilenameOnly) - 4] = 0;
				strcat(newNormalMapFile, pFilenameOnly);
				strcat(newNormalMapFile, "Inverted.dds");
			}
			else
			{
				// Couldn't find filename, so just use the mesh index.
				char meshID[32];
				sprintf(meshID, "%d", pSelectedMesh->wickedmeshindex);
				strcat(newNormalMapFile, meshID);
				strcat(newNormalMapFile, "Inverted.dds");
			}

			// source is textures specified in DBO mesh (or if none exist, use original surface)
			LPSTR pR = "", pG = "", pB = "";
			pR = pSelectedMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName;
			pG = pSelectedMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName;
			pB = pSelectedMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName;
			
			// can just change this to 0,1,2,3 since its all coming from the same texure.
			int iRedChannel = 0, iGreenChannel = 0, iBlueChannel = 0, iAlphaChannel = 3;
			unsigned char channelRMask = (15) + (0 << 4);
			iRedChannel = (channelRMask >> 4) & (3);
			unsigned char channelGMask = (15) + (1 << 4);
			iGreenChannel = (channelGMask >> 4) & (3);
			unsigned char channelBMask = (15) + (2 << 4);
			iBlueChannel = (channelBMask >> 4) & (3);

			// if there is no original raw texture file.
			if (strlen(pR) == 0) { pR = newNormalMapFile; iRedChannel = 0; }
			if (strlen(pG) == 0) { pG = newNormalMapFile; iGreenChannel = 1; }
			if (strlen(pB) == 0) { pB = newNormalMapFile; iBlueChannel = 2; }

			// generate normal texture from sources above
			ImageCreateNormalTextureInvertedGreen(newNormalMapFile, pR, pG, pB, iRedChannel, iGreenChannel, iBlueChannel, iAlphaChannel);
			pSelectedMaterial->textures[GG_MESH_TEXTURE_NORMAL].name = newNormalMapFile;
			pSelectedMaterial->textures[GG_MESH_TEXTURE_NORMAL].resource = WickedCall_LoadImage(pSelectedMaterial->textures[GG_MESH_TEXTURE_NORMAL].name);
			pSelectedMaterial->SetDirty();
			bHaveMaterialUpdate = true;
			iDelayedExecute = 0;
			break;
		}

		// Apply diffuse texture to all meshes.
		case 50:
		{
			importer_texture_all_meshes(MaterialComponentTEXTURESLOT::BASECOLORMAP);
			t.importer.bMeshesHaveDifferentBase = false;
			iDelayedExecute = 0;
			break;
		}
		// Apply normalmap to all meshes.
		case 51:
		{
			importer_texture_all_meshes(MaterialComponentTEXTURESLOT::NORMALMAP);
			t.importer.bMeshesHaveDifferentNormal = false;
			iDelayedExecute = 0;
			break;
		}
		// Apply surface map to all meshes.
		case 52:
		{
			importer_texture_all_meshes(MaterialComponentTEXTURESLOT::SURFACEMAP);
			t.importer.bMeshesHaveDifferentSurface = false;
			t.importer.bEditingAllSurfaceMeshes = false;
			iDelayedExecute = 0;
			break;

		}
		// Apply emissive map to all meshes.
		case 53:
		{
			importer_texture_all_meshes(MaterialComponentTEXTURESLOT::EMISSIVEMAP);
			t.importer.bMeshesHaveDifferentEmissive = false;
			iDelayedExecute = 0;
			break;
		}
		// Apply displacement map to all meshes.
		case 54:
		{
			importer_texture_all_meshes(MaterialComponentTEXTURESLOT::DISPLACEMENTMAP);
			t.importer.bMeshesHaveDifferentDisplacement = false;
			iDelayedExecute = 0;
			break;
		}

		default:
			break;
	}

	if (bImporter_Window && t.importer.importerActive == 1)
	{
		int media_icon_size = 64;
		float col_start = 80.0f;

		//Execute code before we add any texture to the draw call list.
		switch (iDelayedExecute) 
		{
			default:
				break;
		}

		// generate thumbnail
		if (iImporterGenerateThumb <= 5)
		{
			if (iImporterGenerateThumb == 5)
			{
				iImporterGenerateThumb++;
			}
			else
			{
				iImporterGenerateThumb++;
			}
		}

		extern int iGenralWindowsFlags;
		ImGui::Begin("Importer##ImporterWindow", &bImporter_Window, iGenralWindowsFlags);

		float w = ImGui::GetWindowContentRegionWidth();

		extern int iLastOpenHeader;
		if (pref.bAutoClosePropertySections && iLastOpenHeader != 72 && iLastOpenHeader >= 70 && iLastOpenHeader <= 80)
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);

		if (ImGui::StyleCollapsingHeader("Name", ImGuiTreeNodeFlags_DefaultOpen)) 
		{
			iLastOpenHeader = 72;

			ImGui::Indent(10);
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 13)); //3
			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);

			ImGui::InputText("##NameImport", &cImportName[0], 128);
			if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
			if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Object Name");
			ImGui::PopItemWidth();
			ImGui::Indent(-10);
		}

		if (pref.bAutoClosePropertySections && iLastOpenHeader != 73)
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);

		if (ImGui::StyleCollapsingHeader("Customize", ImGuiTreeNodeFlags_DefaultOpen))
		{
			iLastOpenHeader = 73;
			ImGui::Indent(10);

			// Scaling Mode
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 13));
			ImGui::Text("Scaling");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);
			char* pScalingModes[5] = { "Original Scaling", "Units in Meters", "Units in Inches", "Units in Centimeters", "Automatic Scaling" };
			if (ImGui::BeginCombo("##ImporterScalingModes", &scaling_combo_entry[0], ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge))
			{
				for (int i = 0; i < 5; i++)
				{
					// get name
					char* pScalingModeName = pScalingModes[i];

					// assign correct item based on combo_entry
					bool is_selected = false;
					if (strcmp(scaling_combo_entry, pScalingModeName) == NULL)
					{
						is_selected = true;
					}
					if (ImGui::Selectable(pScalingModeName, is_selected))
					{
						strcpy(scaling_combo_entry, pScalingModeName);
						if (t.importer.lastscalingmodeused != i)
						{
							// change the scaling mode, but need to reload the model as
							// this is done at load time
							t.importer.lastscalingmodeused = i;
							iDelayedExecute = 4;
						}
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set the scaling mode to set the model to the correct unit scale at load time");
			// center mesh data
			bool bCenterMeshData = false;
			if (t.importer.centermodelbyshiftingmesh == 1) bCenterMeshData = true;
			if (ImGui::Checkbox("Center Mesh Data", &bCenterMeshData))
			{
				if (bCenterMeshData == true)
					t.importer.centermodelbyshiftingmesh = 1;
				else
					t.importer.centermodelbyshiftingmesh = 0;
				iDelayedExecute = 4;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets whether to center the meshes data of the imported model");

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 13)); //3
			ImGui::Text("Scale");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);
			t.importer.showScaleChange = 0;
			float fImporterScale = iImporterScale;
			if (ImGui::MaxSliderInputFloat("##ImporterScale", &fImporterScale, 0.0f, 200.0f, "Change the scale of the model before saving object", 0, 200))
			{
				iImporterScale = fImporterScale;

				t.importer.showScaleChange = 1;
				t.tscale_f = t.importer.objectScaleForEditing;
				t.tScaleMultiplier_f = iImporterScale / 100.0;
				t.tscale_f = t.tscale_f / t.tScaleMultiplier_f;
				t.tscale_f = t.tscale_f * t.importer.camerazoom;
				ScaleObject(t.importer.dummyCharacterObjectNumber, t.tscale_f, t.tscale_f, t.tscale_f * 0.2);

				//Until we drop the slider menu.
				t.slidersmenuvalue[t.importer.properties1Index][1].value = iImporterScale;
				t.importer.oldScale = t.slidersmenuvalue[t.importer.properties1Index][1].value;

				//PE: Scale actual object.
				if (ObjectExist(t.importer.objectnumber) == 1)
				{
					ScaleObject(t.importer.objectnumber, iImporterScale*fImporterScaleMultiply, iImporterScale*fImporterScaleMultiply, iImporterScale*fImporterScaleMultiply);
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Object Scale");

			ImGui::TextCenter("Rotation Offset");
			if (ImGui::MaxSliderInputFloat("##XImportrotation", &fImportRotX, 0.0f, 360.0f, "Adjust rotation of model around X axis", 0, 360))
			{
				t.importer.objectFPE.rotx = fImportRotX;
				importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);
			}
			if (ImGui::MaxSliderInputFloat("##YImportrotation", &fImportRotY, 0.0f, 360.0f, "Adjust rotation of model around Y axis", 0, 360))
			{
				t.importer.objectFPE.roty = fImportRotY;
				importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);

			}
			if (ImGui::MaxSliderInputFloat("##ZImportrotation", &fImportRotZ, 0.0f, 360.0f, "Adjust rotation of model around Z axis", 0, 360))
			{
				t.importer.objectFPE.rotz = fImportRotZ;
				importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);
			}
			ImGui::TextCenter("Position Offset");
			int iImportPos = fImportPosX;
			if ( ImGui::MaxSliderInputInt("##XImportoffset", &iImportPos, -1000.0f, 1000.0f, "Adjust offset of model on post-rotated X axis"))
			{
				fImportPosX = iImportPos;
				t.importer.objectFPE.offx = fImportPosX;
				importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);
			}
			iImportPos = fImportPosY;
			if ( ImGui::MaxSliderInputInt("##YImportoffset", &iImportPos, -1000.0f, 1000.0f, "Adjust offset of model on post-rotated Y axis"))
			{
				fImportPosY = iImportPos;
				t.importer.objectFPE.offy = fImportPosY;
				importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);

				// Reset find floor flag, as if the user changes the y offset when it is enabled, the model will no longer be at the floor.
				bFindFloor = false;
			}
			iImportPos = fImportPosZ;
			if ( ImGui::MaxSliderInputInt("##ZImportoffset", &iImportPos, -1000.0f, 1000.0f, "Adjust offset of model on post-rotated Z axis"))
			{
				fImportPosZ = iImportPos;
				t.importer.objectFPE.offz = fImportPosZ;
				importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);
			}

			// Find the floor.
			if (ImGui::Checkbox("Find Floor", &bFindFloor))
			{
				importer_find_floor();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto offset the model on the Y axis");

			// Collision Type
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 13)); //3
			ImGui::Text("Collision");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);
			char* pCollisionShapes[10] = { "Box","Polygon","Sphere","Cylinder","Convex Hull","Character","Tree Collision","No Collision", "Hull Decomp", "Collision Mesh" };
			if (ImGui::BeginCombo("##ImporterCollisionShape", &collision_combo_entry[0], ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge))
			{
				for (int i = 0; i < 10; i++)
				{
					// Don't display "Polygon", "Convex Hull" or "Hull Decomp" collision for dynamic objects.
					if (i == 1 && t.importer.defaultstatic == 0) continue;
					else if (i == 4 && t.importer.defaultstatic == 0) continue;
					else if (i == 8 && t.importer.defaultstatic == 0) continue;

					// get collision shape name
					char* pCollisionShapeName = pCollisionShapes[i];

					// assign correct item based on collision_combo_entry
					bool is_selected = false;
					if (strcmp(collision_combo_entry, pCollisionShapeName) == NULL)
					{
						is_selected = true;
					}
					if (ImGui::Selectable(pCollisionShapeName, is_selected))
					{
						strcpy(collision_combo_entry, pCollisionShapeName);
						t.importer.collisionshape = i;
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set the collision type the object will use for physics");
			ImGui::PopItemWidth();

			if (t.importer.collisionshape == 5)
			{
				// Character Specific
				t.importer.defaultstatic = 0;
				if(t.importer.ischaracter==0) t.importer.ischaracter = 1;

				// Static/Dynamic Mode
				bool bIsFemale = false;
				if (t.importer.ischaracter == 2) bIsFemale = true;
				if (ImGui::Checkbox("Is Female", &bIsFemale))
				{
					if (bIsFemale == true)
						t.importer.ischaracter = 2;
					else
						t.importer.ischaracter = 1;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set if the character needs to use female animation sets");
			}
			else
			{
				// Static/Dynamic Mode
				t.importer.ischaracter = 0;
				bool bStaticDefault = false;
				if (t.importer.defaultstatic == 1) bStaticDefault = true;
				if (ImGui::Checkbox("Static Object Mode", &bStaticDefault))
				{
					if (bStaticDefault == true)
						t.importer.defaultstatic = 1;
					else
						t.importer.defaultstatic = 0;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets whether the object is a static unmoving part of the scene");

				// Selectively remove meshes from collision shape
				sObject* pObject = g_ObjectList[t.importer.objectnumber];
				bool bExcludeSpecificMeshes = false;
				if (t.importer.meshesToExclude.size()>0) bExcludeSpecificMeshes = true;
				if (ImGui::Checkbox("Exclude Specific Meshes", &bExcludeSpecificMeshes))
				{
					if (bExcludeSpecificMeshes == true)
					{
						for (int i = 0; i < pObject->iMeshCount; i++)
						{
							t.importer.meshesToExclude.push_back(0);
						}
					}
					else
					{
						t.importer.meshesToExclude.clear();
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Special ability to exclude meshes from a mesh based physics collision shape");
				if (bExcludeSpecificMeshes == true)
				{
					// Display the list of meshes to exclude from the collision shape.
					char meshname[MAX_PATH];
					for (int i = 0; i < pObject->iMeshCount; i++)
					{
						sMesh* pMesh = pObject->ppMeshList[i];
						if (pMesh)
						{
							LPSTR pNameFromMesh = pMesh->pTextures[0].pName;
							extern void Wicked_CreateShortName(int, LPSTR, LPSTR);
							Wicked_CreateShortName(i, meshname, pNameFromMesh);
							bool bThisMeshExcluded = false;
							if(t.importer.meshesToExclude[i]==1) bThisMeshExcluded = true;
							if (ImGui::Checkbox(meshname, &bThisMeshExcluded))
							{
								if(bThisMeshExcluded==true)
									t.importer.meshesToExclude[i] = 1;
								else
									t.importer.meshesToExclude[i] = 0;
							}
						}
					}
				}
			}
			ImGui::Indent(-10);
		}

		if (pref.bAutoClosePropertySections && iLastOpenHeader != 74)
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
		if (ImGui::StyleCollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
		{
			iLastOpenHeader = 74;

			ImGui::Indent(10);

			// Set the material index of the imported object.
			char* cMaterialTypes[4] = { "Silent", "Stone", "Metal", "Wood" };
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 15));
			ImGui::Text("Material Type");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			ImGui::PushItemWidth(-10);
			if (ImGui::BeginCombo("##ImporterMaterialType", &material_combo_entry[0], ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge))
			{
				for (int i = 0; i < 4; i++)
				{
					// assign correct item based on material_combo_entry
					bool is_selected = false;
					if (strcmp(material_combo_entry, cMaterialTypes[i]) == NULL)
					{
						is_selected = true;
					}
					if (ImGui::Selectable(cMaterialTypes[i], is_selected))
					{
						strcpy(material_combo_entry, cMaterialTypes[i]);
						t.slidersmenuvalue[t.importer.properties1Index][10].value = i + 1;
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select the material index for this object");
			ImGui::PopItemWidth();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 15));

			//PE: WICKED - display textures directly from object.
			sObject* pObject = g_ObjectList[t.importer.objectnumber];
			sMesh * pMesh = NULL;
			
			Wicked_Change_Object_Material((void*)pObject, 0, NULL, t.importer.bEditAllMesh); 
			ImGui::Indent(-10);
		}

		animsystem_animationtoolui(t.importer.objectnumber);

		// Import Object
		if (pref.bAutoClosePropertySections && iLastOpenHeader != 76)
		{
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
		}
		if (ImGui::CollapsingHeader("Import Object", ImGuiTreeNodeFlags_DefaultOpen))
		{
			iLastOpenHeader = 76;
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
			ImGui::TextCenter("Path");
			ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - ImGui::GetFontSize() * 9 - 10);// ImGui::GetFontSize() * 10.0f);
			ImGui::Indent(ImGui::GetFontSize()*2.0 +40);
			
			ImGui::InputText("##InputPathImporter", &cImportPathCropped[0], 250);
			if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set where you would like to save the object files");
			if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
			ImGui::PopItemWidth();

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetFontSize() * 2.0f);

			//	Let the user know they set an invalid save file path.
			if (ImGui::BeginPopup("##InvalidSavePath"))
			{
				ImGui::Text("Path must be within 'entitybank\\'");
				ImGui::EndPopup();
			}

			if (ImGui::StyleButton("...##Importerpath"))
			{
				//PE: filedialogs change dir so.
				cStr tOldDir = GetDir();
				char * cFileSelected;
				//cstr fulldir = tOldDir + "\\entitybank\\user\\";
				char defaultPath[MAX_PATH];
				strcpy(defaultPath, GG_GetWritePath());
				strcat(defaultPath, "Files\\entitybank\\user");
				cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_DIR, "All\0*.*\0", defaultPath, "", true, NULL);
				SetDir(tOldDir.Get());

				// Now that user has chosen a new path, refresh the folders, in case any new folders have been added 
				extern bool bExternal_Entities_Init;
				bExternal_Entities_Init = false;
				extern void mapeditorexecutable_full_folder_refresh(void);
				mapeditorexecutable_full_folder_refresh();

				if (cFileSelected && strlen(cFileSelected) > 0) 
				{
					//	Check that the new path still contains the entitybank folder.
					char* cCropped = strstr(cFileSelected, "\\entitybank");
					if (cCropped)
					{
						//	New location contains entitybank folder, so change the import path.
						strcpy(cImportPath, cFileSelected);
						strcpy(cImportPathCropped, cCropped);

						//	Drop the entitybank folder from the cropped file path.
						char pNewCroppedStr[MAX_PATH];
						strcpy(pNewCroppedStr, cCropped + strlen("\\entitybank"));
						strcpy(cImportPathCropped, pNewCroppedStr);
					}
					else
					{
						ImGui::OpenPopup("##InvalidSavePath");
					}
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set where you would like to save the object files");

			ImGui::PopItemWidth();
			ImGui::Indent(-58);
			ImGui::Indent(-10); //unindent before center.

			float but_gadget_size = ImGui::GetFontSize()*10.0;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
			if (ImGui::Button("Import Object##ImporterSaveUniqueId", ImVec2(but_gadget_size, 0))) 
			{
				//code
				if (strlen(cImportName) > 0) 
				{
					if (strlen(cImportPath) > 0) 
					{
						importer_storeobjectdata();
						// before save, match created animsets to actual DBO structure
						sObject* pObject = GetObjectData(t.importer.objectnumber);
						UpdateObjectWithAnimSlotList(pObject);

						// Trigger save Object to happen
						iDelayedExecute = 3; 
					}
					else
					{
						strcpy(cTriggerMessage, "Please select a path where you like the object saved");
						bTriggerMessage = true;
					}
				}
				else 
				{
					strcpy(cTriggerMessage, "You must give your object a name before you can save it");
					bTriggerMessage = true;
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import Object");

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
			if (ImGui::StyleButton("Cancel", ImVec2(but_gadget_size, 0)))
			{
				iDelayedExecute = 2; //Quit
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cancel, Close Importer");
		}

		// Import Batch
		if (pref.bAutoClosePropertySections && iLastOpenHeader != 77)
		{
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
		}
		if (ImGui::CollapsingHeader("Import Batch", ImGuiTreeNodeFlags_DefaultOpen))
		{
			iLastOpenHeader = 77;
			float but_gadget_size = ImGui::GetFontSize() * 10.0;
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
			if (bBatchConverting == true)
			{
				// show files in progress 
				static bool bNextTimeYouWillQuit = false; // only sets this at start of EXE
				if (batchFileList.size() > 0 ) bNextTimeYouWillQuit = false;
				if (batchFileList.size() >= 0 && bNextTimeYouWillQuit == false )
				{
					ImGui::TextCenter("");
					extern cstr sGotoPreviewWithFile;
					if (sGotoPreviewWithFile.Len() > 0)
					{
						ImGui::TextCenter("...Creating Preview...");
					}
					else
					{
						char pTitleFilesToBatch[256];
						sprintf(pTitleFilesToBatch, "%d Files Remaining", batchFileList.size());
						ImGui::TextCenter(pTitleFilesToBatch);
						for (int f = batchFileList.size() - 1; f > 0; f--)
						{
							if (f >= 0)
							{
								ImGui::TextCenter(batchFileList[f].Get());
								if (f > 14)
								{
									ImGui::TextCenter("...");
									break;
								}
							}
						}

						// before save, match created animsets to actual DBO structure
						sObject* pObject = GetObjectData(t.importer.objectnumber);
						UpdateObjectWithAnimSlotList(pObject);

						// Trigger save Object to happen
						if (iDelayedExecute != 5)
						{
							// if not already tasked with batch conversion and moving to next one (skipping preview thumb adjustment)
							iDelayedExecute = 3;
						}

						// now check if we need to END the batch process
						if (batchFileList.size() == 0)
						{
							// next time we are generally here, we will quit
							bNextTimeYouWillQuit = true;
						}
					}
				}
				else
				{
					// quit when finish batch conversion
					bBatchConverting = false;
					iDelayedExecute = 2; // Quit
				}
			}
			else
			{
				ImGui::TextCenter("This feature will use the current object");
				ImGui::TextCenter("customized settings to batch convert all");
				ImGui::TextCenter("models in the batch folder, and provide");
				ImGui::TextCenter("a preview window to finalize the");
				ImGui::TextCenter("thumbnail image of each one.");
				ImGui::TextCenter("");
				ImGui::TextCenter("Batch Folder");
				ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - ImGui::GetFontSize() * 9 - 10);
				ImGui::Indent(ImGui::GetFontSize() * 2.0 + 40);

				ImGui::InputText("##InputPathImporter", &cImportPathCropped[0], 250);
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set where you would like to save the object files");
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::PushItemWidth(ImGui::GetFontSize() * 2.0f);

				//	Let the user know they set an invalid save file path.
				if (ImGui::BeginPopup("##InvalidSavePath2"))
				{
					ImGui::Text("Path must be within 'entitybank\\'");
					ImGui::EndPopup();
				}

				if (ImGui::StyleButton("...##Importerpath2"))
				{
					//PE: filedialogs change dir so.
					cStr tOldDir = GetDir();
					char* cFileSelected;
					char defaultPath[MAX_PATH];
					strcpy(defaultPath, GG_GetWritePath());
					strcat(defaultPath, "Files\\entitybank\\user");
					cFileSelected = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_DIR, "All\0*.*\0", defaultPath, "", true, NULL);
					SetDir(tOldDir.Get());

					// Now that user has chosen a new path, refresh the folders, in case any new folders have been added 
					extern bool bExternal_Entities_Init;
					bExternal_Entities_Init = false;
					extern void mapeditorexecutable_full_folder_refresh(void);
					mapeditorexecutable_full_folder_refresh();

					if (cFileSelected && strlen(cFileSelected) > 0)
					{
						//	Check that the new path still contains the entitybank folder.
						char* cCropped = strstr(cFileSelected, "\\entitybank");
						if (cCropped)
						{
							//	New location contains entitybank folder, so change the import path.
							strcpy(cImportPath, cFileSelected);
							strcpy(cImportPathCropped, cCropped);

							//	Drop the entitybank folder from the cropped file path.
							char pNewCroppedStr[MAX_PATH];
							strcpy(pNewCroppedStr, cCropped + strlen("\\entitybank"));
							strcpy(cImportPathCropped, pNewCroppedStr);

							// collect list of models to convert
							imgui_importer_refreshbatchlist();
						}
						else
						{
							ImGui::OpenPopup("##InvalidSavePath2");
						}
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("This folder contains all the models to be batch converted");

				ImGui::PopItemWidth();
				ImGui::Indent(-58);
				ImGui::Indent(-10); //unindent before center.

				// show files
				ImGui::TextCenter("");
				char pTitleFilesToBatch[256];
				sprintf(pTitleFilesToBatch, "%d Files to Batch", batchFileList.size());
				ImGui::TextCenter(pTitleFilesToBatch);
				for (int f = 0; f < batchFileList.size(); f++)
				{
					ImGui::TextCenter(batchFileList[f].Get());
					if (f > 14)
					{
						ImGui::TextCenter("...");
						break;
					}
				}

				// allow artist to ignore already exported DBOs
				ImGui::Text("");
				ImGui::Indent(30);
				if (ImGui::Checkbox("Ignore DBO Models", &g_bIgnoreDBOAsAlreadyConverted))
				{
					// and refresh the list
					imgui_importer_refreshbatchlist();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("You may wish to skip any models already converted to DBO using this toggle");
				ImGui::Indent(-30);

				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));
				if (ImGui::Button("Batch Convert All##ImporterSaveUniqueId", ImVec2(but_gadget_size, 0)))
				{
					if (strlen(cImportName) > 0)
					{
						if (strlen(cImportPath) > 0)
						{
							// save settings
							importer_storeobjectdata();
							// before save, match created animsets to actual DBO structure
							sObject* pObject = GetObjectData(t.importer.objectnumber);
							UpdateObjectWithAnimSlotList(pObject);
							// Trigger Batch Convert to happen
							iDelayedExecute = 5;
						}
						else
						{
							strcpy(cTriggerMessage, "Please select a path to the models to be batch converted");
							bTriggerMessage = true;
						}
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import Object Batch");
			}

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));
			if (ImGui::StyleButton("Cancel", ImVec2(but_gadget_size, 0)))
			{
				iDelayedExecute = 2; // Quit
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cancel, Close Importer");
		}

		void CheckMinimumDockSpaceSize(float minsize);
		CheckMinimumDockSpaceSize(250.0f);

		// Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
		if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0)
		{
			ImGui::Text("");
			ImGui::Text("");
		}
		ImGui::End();
	}
	else
	{
		if (!bImporter_Window && t.importer.importerActive == 1)
		{
			//Window must have been close, shutdown.
			iDelayedExecute = 2; //Quit
		}
		else if (!bImporter_Window) 
		{
			//PE: we can delete the object here , so we are sure we dont use any textures from the object.
			if (ObjectExist(t.importer.objectnumber))
			{
				DeleteObject(t.importer.objectnumber);
				WickedCall_SetSelectedObject(NULL);
			}
			if (ObjectExist(t.importer.dummyCharacterObjectNumber)) DeleteObject(t.importer.dummyCharacterObjectNumber);
		}
	}
}

void importer_texture_all_meshes(int iTexSlot)
{
	//t.gridentityextractedindex; t.widget.pickedEntityIndex;
	sObject* pObject = nullptr;// = GetObjectData(t.importer.objectnumber);
	if (t.importer.importerActive == 1)
	{
		pObject = GetObjectData(t.importer.objectnumber);
	}
	else
	{
		// Need a way to get the object data for the current object being altered outside of the importer.
		int e = t.widget.pickedEntityIndex;
		if (e < 0) return;
		int obj = t.entityelement[e].obj;
		if (ObjectExist(obj))
		{
			pObject = GetObjectData(obj);
		}
	}
	
	if (!pObject)
	{
		return;
	}

	bool bUserCancel = false;
	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		pSelectedMesh = pObject->ppMeshList[i];

		if (pSelectedMesh)
		{
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pSelectedMesh->wickedmeshindex);

			if (mesh)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = mesh->subsets[0].materialID;
				wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				pSelectedMaterial = pObjectMaterial;

				bUserCancel = !importer_apply_materialformesh(static_cast<MaterialComponentTEXTURESLOT>(iTexSlot), iTexSlot);
				if (bUserCancel) break;
			}
		}
	}

	if (!t.importer.importerActive)
	{
		// Ensure that the new material changes are applied to the object eleprof, so that when maps get saved/loaded, the changes are not lost.
		Wicked_Copy_Material_To_Grideleprof((void*)pObject, 0, &t.entityelement[t.widget.pickedEntityIndex].eleprof);
		Wicked_Set_Material_From_grideleprof((void*)pObject, 0, &t.entityelement[t.widget.pickedEntityIndex].eleprof);
	}

	// Reset the selected file, so that the file dialiog will open when adding other textures.
	strcpy(cPreSelectedFile, "");
}

void imgui_importer_draw(void)
{

}

void importer_loop_wicked(void)
{
	// New import system for Wicked, should use existing editor to move around object, etc

	// Rotate the character decal to face the camera.
	PointObject(g.importerextraobjectoffset + 2, CameraPositionX(0), CameraPositionY(0), CameraPositionZ(0));
	XRotateObject(g.importerextraobjectoffset + 2, 0.0f);
	ZRotateObject(g.importerextraobjectoffset + 2, 0.0f);

	//PE: Normal navigation.
	editor_visuals();
}

void importer_loop ( void )
{
	importer_loop_wicked();
}

void importer_update_selection_markers ( void )
{
	//  Show / hide collision depending which tab is selected
	if (  t.importerTabs[2].selected  ==  0 ) 
	{
		for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
		{
			if (  t.importerCollision[tCount].object > 0 ) 
			{
				if (  ObjectExist(t.importerCollision[tCount].object)  ==  1 ) 
				{
					HideObject (  t.importerCollision[tCount].object );
					HideObject (  t.importerCollision[tCount].object2 );
				}
			}
		}
		for ( t.tCount2 = 1 ; t.tCount2<=  9; t.tCount2++ )
		{
			if (  t.selectedObjectMarkers[t.tCount2] > 0 ) 
			{
				if (  ObjectExist(t.selectedObjectMarkers[t.tCount2])  ==  1  )  HideObject (  t.selectedObjectMarkers[t.tCount2] );
			}
		}
	}
	else
	{
		if (  t.importer.collisionShapeCount > 0 ) 
		{
			if (t.importer.selectedCollisionObject >= 0) {
				if (t.importerCollision[t.importer.selectedCollisionObject].object > 0 && t.importerCollision[t.importer.selectedCollisionObject].object2 > 0)
				{
					if (ObjectExist(t.importerCollision[t.importer.selectedCollisionObject].object) == 1 && ObjectExist(t.importerCollision[t.importer.selectedCollisionObject].object2) == 1)
					{
						t.importerCollision[t.importer.selectedCollisionObject].rotx = t.slidersmenuvalue[t.importer.properties2Index][7].value;
						t.importerCollision[t.importer.selectedCollisionObject].roty = t.slidersmenuvalue[t.importer.properties2Index][8].value;
						t.importerCollision[t.importer.selectedCollisionObject].rotz = t.slidersmenuvalue[t.importer.properties2Index][9].value;
						RotateObject(t.importerCollision[t.importer.selectedCollisionObject].object, t.importerCollision[t.importer.selectedCollisionObject].rotx, t.importerCollision[t.importer.selectedCollisionObject].roty, t.importerCollision[t.importer.selectedCollisionObject].rotz);
						RotateObject(t.importerCollision[t.importer.selectedCollisionObject].object2, t.importerCollision[t.importer.selectedCollisionObject].rotx, t.importerCollision[t.importer.selectedCollisionObject].roty, t.importerCollision[t.importer.selectedCollisionObject].rotz);
					}
				}
			}
		}
		if (  t.importer.collisionShapeCount  ==  0 ) 
		{
			for ( t.tCount2 = 1 ; t.tCount2<=  9; t.tCount2++ )
			{
				if (  t.selectedObjectMarkers[t.tCount2] > 0 ) 
				{
					if (  ObjectExist(t.selectedObjectMarkers[t.tCount2])  ==  1  )  HideObject (  t.selectedObjectMarkers[t.tCount2] );
				}
			}
			return;
		}
		for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
		{
			if (  t.importerCollision[tCount].object > 0 ) 
			{
				if (  ObjectExist(t.importerCollision[tCount].object)  ==  1 ) 
				{
					ShowObject (  t.importerCollision[tCount].object2 );
					if (  t.importer.selectedCollisionObject  ==  tCount ) 
					{
						for ( t.tCount2 = 1 ; t.tCount2<=  9; t.tCount2++ )
						{
							ShowObject (  t.selectedObjectMarkers[t.tCount2] );
							RotateObject (  t.selectedObjectMarkers[t.tCount2],0,0,0 );
						}

						//  position pivot at 0,0,0 so we can glue things to it
						PositionObject ( t.importerGridObject[9], 0, 0, 0 );

						//  top front left
						UnGlueObject (  t.selectedObjectMarkers[1] );
						PositionObject (  t.selectedObjectMarkers[1], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[1], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) );
						MoveObjectUp (  t.selectedObjectMarkers[1], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[1], -LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectLeft (  t.selectedObjectMarkers[1], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[1], t.importerGridObject[9],0 );

						//  top front right
						UnGlueObject (  t.selectedObjectMarkers[2] );
						PositionObject (  t.selectedObjectMarkers[2], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[2], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) );
						MoveObjectUp (  t.selectedObjectMarkers[2], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[2], -LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectRight (  t.selectedObjectMarkers[2], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[2], t.importerGridObject[9],0 );

						//  bottom front left
						UnGlueObject (  t.selectedObjectMarkers[3] );
						PositionObject (  t.selectedObjectMarkers[3], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[3], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) ) ;
						MoveObjectDown (  t.selectedObjectMarkers[3], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[3], -LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectLeft (  t.selectedObjectMarkers[3], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[3], t.importerGridObject[9],0 );

						//  bottom front right
						UnGlueObject (  t.selectedObjectMarkers[4] );
						PositionObject (  t.selectedObjectMarkers[4], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) )  ;
						RotateObject (  t.selectedObjectMarkers[4], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) ) ;
						MoveObjectDown (  t.selectedObjectMarkers[4], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[4], -LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectRight (  t.selectedObjectMarkers[4], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[4], t.importerGridObject[9],0 );

						//  top back left
						UnGlueObject (  t.selectedObjectMarkers[5] );
						PositionObject (  t.selectedObjectMarkers[5], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[5], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) );
						MoveObjectUp (  t.selectedObjectMarkers[5], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[5], LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectLeft (  t.selectedObjectMarkers[5], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[5], t.importerGridObject[9],0 );

						//  top back right
						UnGlueObject (  t.selectedObjectMarkers[6] );
						PositionObject (  t.selectedObjectMarkers[6], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[6], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) ) ;
						MoveObjectUp (  t.selectedObjectMarkers[6], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[6], LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectRight (  t.selectedObjectMarkers[6], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[6], t.importerGridObject[9],0 );

						//  bottom back left
						UnGlueObject (  t.selectedObjectMarkers[7] );
						PositionObject (  t.selectedObjectMarkers[7], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[7], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) );
						MoveObjectDown (  t.selectedObjectMarkers[7], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[7], LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectLeft (  t.selectedObjectMarkers[7], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[7], t.importerGridObject[9],0 );

						//  bottom back right
						UnGlueObject (  t.selectedObjectMarkers[8] );
						PositionObject (  t.selectedObjectMarkers[8], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[8], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) );
						MoveObjectDown (  t.selectedObjectMarkers[8], LimbScaleY(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObject (  t.selectedObjectMarkers[8], LimbScaleZ(t.importerCollision[tCount].object2,0) / 2.0 );
						MoveObjectRight (  t.selectedObjectMarkers[8], LimbScaleX(t.importerCollision[tCount].object2,0) / 2.0 );
						GlueObjectToLimb (  t.selectedObjectMarkers[8], t.importerGridObject[9],0 );

						//  center
						UnGlueObject (  t.selectedObjectMarkers[9] );
						PositionObject (  t.selectedObjectMarkers[9], ObjectPositionX (t.importerCollision[tCount].object2), ObjectPositionY (t.importerCollision[tCount].object2), ObjectPositionZ (t.importerCollision[tCount].object2) ) ;
						RotateObject (  t.selectedObjectMarkers[9], ObjectAngleX (t.importerCollision[tCount].object2),ObjectAngleY(t.importerCollision[tCount].object2),ObjectAngleZ(t.importerCollision[tCount].object2) ) ;
						GlueObjectToLimb (  t.selectedObjectMarkers[9], t.importerGridObject[9],0 );

						//  put pivot back into the scene
						PositionObject (   t.importerGridObject[9],0,0,IMPORTERZPOSITION );

					}
				}
			}
		}
	}

	for ( int tCount = 1 ; tCount<=  9; tCount++ )
	{
		if (t.importer.selectedCollisionObject >= 0) {
			t.snapPosX_f[t.importer.selectedCollisionObject][tCount] = ObjectPositionX(t.selectedObjectMarkers[tCount]);
			t.snapPosY_f[t.importer.selectedCollisionObject][tCount] = ObjectPositionY(t.selectedObjectMarkers[tCount]);
			t.snapPosZ_f[t.importer.selectedCollisionObject][tCount] = ObjectPositionZ(t.selectedObjectMarkers[tCount]);
		}
	}
}

void importer_ShowCollisionOnly ( void )
{
	HideObject (  t.importer.objectnumber );
	for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
		if (  t.importerCollision[tCount].object > 0 ) 
		{
			if (  ObjectExist(t.importerCollision[tCount].object)  ==  1 ) 
			{
				ShowObject (  t.importerCollision[tCount].object );
				SetObjectLight (  t.importerCollision[tCount].object, 1 );
				ColorObject (  t.importerCollision[tCount].object , Rgb(255,255,100) );
			}
		}
	}
	RotateObject (  t.importerGridObject[9], ObjectAngleX(t.importer.objectnumber), ObjectAngleY(t.importer.objectnumber), ObjectAngleZ(t.importer.objectnumber) );
}

void importer_ShowCollisionOnlyOff ( void )
{
	ShowObject (  t.importer.objectnumber );
	for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
		if (  t.importerCollision[tCount].object > 0 ) 
		{
			if (  ObjectExist(t.importerCollision[tCount].object)  ==  1 ) 
			{
				///GhostObjectOn (  t.importerCollision[tCount].object );
				HideObject (  t.importerCollision[tCount].object );
			}
		}
	}
}

void importer_snapLeft ( void )
{
	t.tx_f = ObjectPositionX(t.tSnapObject);
	for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
		if (  tCount  !=  t.importer.selectedCollisionObject ) 
		{
			for ( t.tCount2 = 1 ; t.tCount2<=  8; t.tCount2++ )
			{
				t.tdiff_f = t.tx_f - t.snapPosX_f[tCount][t.tCount2];
				if (  abs(t.tdiff_f) < 5 ) 
				{
					MoveObject (  t.tSnapObject, t.tdiff_f );
					return;
				}
			}
		}
	}
}

void importer_snapUp ( void )
{
	t.ty_f = ObjectPositionY(t.tSnapObject);
	for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
		if (  tCount  !=  t.importer.selectedCollisionObject ) 
		{
			for ( t.tCount2 = 1 ; t.tCount2<=  8; t.tCount2++ )
			{
				t.tdiff_f = t.ty_f - t.snapPosY_f[tCount][t.tCount2];
				if (  abs(t.tdiff_f) < 5 ) 
				{
					MoveObjectUp (  t.tSnapObject, t.tdiff_f );
					return;
				}
			}
		}
	}
}

void importer_snapforward ( void )
{
	t.tz_f = ObjectPositionZ(t.tSnapObject);
	for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
		if (  tCount  !=  t.importer.selectedCollisionObject ) 
		{
			for ( t.tCount2 = 1 ; t.tCount2<=  8; t.tCount2++ )
			{
				t.tdiff_f = t.tz_f - t.snapPosZ_f[tCount][t.tCount2];
				if (  abs(t.tdiff_f) < 5 ) 
				{
					MoveObject (  t.tSnapObject, t.tdiff_f );
					return;
				}
			}
		}
	}
}

