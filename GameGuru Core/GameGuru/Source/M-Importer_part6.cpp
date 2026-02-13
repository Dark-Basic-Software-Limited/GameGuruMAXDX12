int update_all_count = 0;

void Wicked_Change_Object_Material(void* pVObject, int mode, entityeleproftype *edit_grideleprof, bool bFromCustomMaterials,bool bReadOnly)
{
	//Mode 0 Allow selection of all meshes.
	//Mode 1 (NOT USED) Single mesh , texture all meshes with the same material.
	//Mode 3 EBE
	//Mode 5 readonly.
	
	//Mode 6 all materials (multi-selection editing many objects at once)
	bool bCloneChangesToAllObjectsInRubberBand = false;
	bool bCloneChangesToAllObjectsInRubberBandTransparency = false;
	bool bCloneChangesToAllObjectsInRubberBandDoubleSided = false;
	bool bCloneChangesToAllObjectsInRubberBandPlanarReflection = false;
	bool bCloneChangesToAllObjectsInRubberBandCastShadow = false;

	if (t.importer.importerActive)// && !t.importer.bEditAllMesh) allow importer to specify RGBA for ALL meshes at once
	{
		bFromCustomMaterials = !bUseRGBAButtons;
	}

	// store eleprof being edited
	if (!edit_grideleprof)
	{
		edit_grideleprof = &t.grideleprof;
	}

	// update grideleprof if EBE editing
	if ( mode == 0 || mode == 3)
		bUpdateGrideleprof = true;
	else
		bUpdateGrideleprof = false;

	// copy material to grideleprof if not latest
	if (bUpdateGrideleprof && bHaveMaterialUpdate) 
	{
		Wicked_Copy_Material_To_Grideleprof(pVObject, mode, edit_grideleprof);
		if (mode == 0 || mode == 3)
		{
			// update wicked settings on all meshes.
			update_all_count = 25;
		}
		bHaveMaterialUpdate = false;
	}

	// count down and trigger ALL materials in object to be updated
	if (update_all_count > 0) 
	{
		update_all_count--;
		if (update_all_count == 0) 
		{
			Wicked_Update_All_Materials(pVObject, mode);
		}
	}

	// get object ptr
	sObject* pObject = (sObject*) pVObject;
	if (!pObject) return;

	// can only act on one mesh at a time (chosen by a dropdown for object)
	sMesh* pChosenMesh = NULL;

	// width of panel
	float w = ImGui::GetContentRegionAvailWidth();

	// mesh dropdown only for objects and readonly, not EBE
	int comboflags = ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
	if (mode == 0 || mode == 5 || mode == 6)
	{
		bool bDisplayCombo = true;
		if (mode == 5 || mode == 6)
		{
			// choose mesh
			sMesh* pSelMesh = NULL;
			int iMeshes = 0;
			for (int i = 0; i < pObject->iMeshCount; i++)
			{
				sMesh* pMesh = pObject->ppMeshList[i];
				if (pMesh)
				{
					if (!pSelMesh)
					{
						pSelMesh = pMesh;
					}
					iMeshes++;
				}
			}
			if (iMeshes==1) 
			{
				if (pSelMesh)
				{
					pChosenMesh = pSelMesh;
				}
				bDisplayCombo = false;
			}
		}

		// if have mehses, display drop down to select one
		if (bDisplayCombo)
		{
			char meshname[MAX_PATH] = "";
			ImGui::PushItemWidth(-10);
			if (ImGui::BeginCombo("##combolastsearch", &mesh_combo_entry[0], comboflags))
			{
				// If selected, the user will be able to apply a single texture to all meshes in the object.
				if (ImGui::Selectable("Edit All Mesh Materials"))
				{
					strcpy(mesh_combo_entry, "Edit All Mesh Materials");
					t.importer.bEditAllMesh = true;
				}

				int count = pObject->iMeshCount;
				if (count > MAXMESHMATERIALS)
					count = MAXMESHMATERIALS;
				for (int i = 0; i < count; i++)
				{
					sMesh* pMesh = pObject->ppMeshList[i];
					if (pMesh)
					{	
						// Get names for the mesh from the base colour texture, unless they have already been created.
						if (!t.importer.bModelMeshNamesSet)
						{
							// Crops the filepath and extension, to leave only the mesh name.
							LPSTR pNameFromMesh = pMesh->pTextures[0].pName;
							extern void Wicked_CreateShortName(int, LPSTR, LPSTR);
							Wicked_CreateShortName(i, meshname, pNameFromMesh);
							t.importer.cModelMeshNames.push_back(meshname);
							if (i == (pObject->iMeshCount - 1))
								t.importer.bModelMeshNamesSet = true;
						}
						else
						{
							strcpy(meshname, t.importer.cModelMeshNames[i].Get());
						}

						// assign correct mesh based on mesh_combo_entry
						bool is_selected = iSelectedMesh == i;
						ImGui::PushID(5723 + i);
						if (ImGui::Selectable(meshname, is_selected))
						{
							strcpy(mesh_combo_entry, meshname);
							iSelectedMesh = i;
							t.importer.bEditAllMesh = false;
							if (iSelectedMesh >= MAXMESHMATERIALS - 1) //PE: We can crash if we go above the MAXMESHMATERIALS.
								iSelectedMesh = MAXMESHMATERIALS - 1;
							pChosenMesh = pMesh;
						}
						ImGui::PopID();
						if (is_selected) ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select this mesh from all the meshes stored in the object");
			ImGui::PopItemWidth();
		}
	}

	// if no mesh found/selected, find first in object
	if (!pChosenMesh) 
	{
		if (pObject->ppMeshList == nullptr)
		{
			// Got a crash here, check for valid mesh list
			// Not getting the crash in debug, and have a valid mesh list the following frame
			// Likely related to this issue:https://github.com/TheGameCreators/GameGuruRepo/issues/3276
			// Memory corruption occurs somewhere during the entity copy and paste process
			return;
		}
		for (int i = 0; i < pObject->iMeshCount; i++)
		{
			sMesh* pMesh = pObject->ppMeshList[i];
			if (pMesh && pMesh->wickedmeshindex > 0)
			{
				if (mode == 3)
				{
					// select first mesh if EBE
					iSelectedMesh = i;
					if (iSelectedMesh >= MAXMESHMATERIALS - 1)
					{
						//PE: We can crash if we go above the MAXMESHMATERIALS.
						iSelectedMesh = MAXMESHMATERIALS - 1;
					}
					pChosenMesh = pMesh;
					break;
				}
				else
				{
					// select chosen mesh otherwise
					if (iSelectedMesh == i)
					{
						pChosenMesh = pMesh;
						break;
					}
				}
			}
		}
	}

	bool launch_file = false;

	// avoid repeating the show of textures
	char lastmaterialname[MAX_PATH];
	strcpy(lastmaterialname, "");

	// with mesh available to edit
	if (pChosenMesh)
	{
		if (bReadOnly)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		// ensure we are aligned with the chosen mesh material
		WickedSetMeshNumber(iSelectedMesh);

		// LB: ensure newly selected mesh updates material
		static int iOldSelectedMesh;
		if (iSelectedMesh != iOldSelectedMesh)
		{
			iOldSelectedMesh = iSelectedMesh;
			Wicked_Set_Material_From_grideleprof_ThisMesh((void*)pObject, 0, edit_grideleprof, iSelectedMesh);
		}

		// get mesh from DBO mesh ptr
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pChosenMesh->wickedmeshindex);
		if (mesh)
		{
			// get material from mesh
			uint64_t materialEntity = mesh->subsets[0].materialID;
			wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
			if (pObjectMaterial)
			{
				// get IMGUI panel settings
				float col_start = ImGui::GetCursorPosX() + 80.0f;
				float help_start = ImGui::GetCursorPosX() + 60.0f;
				int preview_icon_size = ImGui::GetFontSize();
				float path_gadget_size = ImGui::GetFontSize()*2.0;
				if (mode == 3 || mode == 5) path_gadget_size = 4.0f;

				// vars for material view/editing
				int tCount = 1;
				char materialname[MAX_PATH];
				memset(materialname, 0, sizeof(materialname));

				// if read only
				int mode5_materials = 0;
				int mode5_displayed_materials = 0;
				float mode5_icon_size = 32.0f;
				if (mode == 5)
				{
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource && pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource)
						mode5_materials++;
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].resource && pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].resource)
						mode5_materials++;
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource && pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource)
						mode5_materials++;
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].resource && pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].resource)
						mode5_materials++;

					//Padding,Margin.
					if (mode5_materials > 0) mode5_icon_size = ImGui::GetContentRegionAvailWidth() / mode5_materials;
					mode5_icon_size -= 16.0f;
					if (mode5_icon_size < 20.0f) mode5_icon_size = 20.0f;
				}

				// read only states if EBE or readonly
				int iInputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
				if (mode == 3 || mode == 5)	iInputFlags = ImGuiInputTextFlags_ReadOnly;

				// if not ALL OBJECTS material changes
				if (mode != 6)
				{
					// base color
					ImVec4 mycolor;
					bool open_popup;
					ImGuiWindow* window;
					ID3D11ShaderResourceView* lpTexture;
					ImVec2 vDrawPos;

					// base color editing (if not readonly)
					if (mode != 5)
					{
						// mesh base color
						BaseColor[0] = pChosenMesh->mMaterial.Diffuse.r;
						BaseColor[1] = pChosenMesh->mMaterial.Diffuse.g;
						BaseColor[2] = pChosenMesh->mMaterial.Diffuse.b;
						BaseColor[3] = pChosenMesh->mMaterial.Diffuse.a;
						ImGui::TextCenter("Base Color");
						mycolor = ImVec4(BaseColor[0], BaseColor[1], BaseColor[2], BaseColor[3]);
						open_popup = ImGui::ColorButton("##NewV2WickedBaseColor", mycolor, 0, ImVec2(w - 10.0, 0));
						if (open_popup) ImGui::OpenPopup("##pickWickedBaseColor");
						if (ImGui::BeginPopup("##pickWickedBaseColor", ImGuiWindowFlags_NoMove))
						{
							if (ImGui::ColorPicker4("##BaseColor", &BaseColor[0], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayRGB))
							{
								pChosenMesh->mMaterial.Diffuse.r = BaseColor[0];
								pChosenMesh->mMaterial.Diffuse.g = BaseColor[1];
								pChosenMesh->mMaterial.Diffuse.b = BaseColor[2];
								pChosenMesh->mMaterial.Diffuse.a = BaseColor[3];
								dwBaseColor = ((unsigned int)(BaseColor[0] * 255) << 24);
								dwBaseColor += ((unsigned int)(BaseColor[1] * 255) << 16);
								dwBaseColor += ((unsigned int)(BaseColor[2] * 255) << 8);
								dwBaseColor += ((unsigned int)(BaseColor[3] * 255));
								WickedCall_SetMeshMaterial(pChosenMesh, true);
								bHaveMaterialUpdate = true;

								importer_set_all_material_colour(0, BaseColor);
							}
							ImGui::EndPopup();
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Change Base Color");
						window = ImGui::GetCurrentWindow(); //PE: Add a pencil to all color gadgets.
						lpTexture = GetImagePointerView(TOOL_PENCIL);
						vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - (ImGui::GetFontSize() * 1.5f) - 3.0f };
						if (lpTexture)
							window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
					}

					// loop through all textures required
					for (int texslot = 0; texslot < 7; texslot++)
					{
						// texture gadget labels
						LPSTR pInputLabel = "", pInputBtnLabel = "", pInputControlLabel = "";
						if (texslot == 0) { pInputLabel = "##InputMeshColor"; pInputBtnLabel = "...##InputMeshColorFile"; pInputControlLabel = "##AlphaRef"; }
						if (texslot == 1) { pInputLabel = "##InputMeshNormal"; pInputBtnLabel = "...##InputMeshNormalFile"; pInputControlLabel = "##NormalStrength"; }
						if (texslot == 2 && t.importer.importerActive)
						{
							if (ImGui::Checkbox("Use RGBA buttons##importersurfacemode", &bUseRGBAButtons))
							{
								// Choose whether to select surface texture directly, or take data from individual channels of a source texture.
							}
						}
						if (bFromCustomMaterials == true)
						{
							// viewed from object properties or when editing all meshes.
							if (texslot == 2) { pInputLabel = "##InputMeshSurface"; pInputBtnLabel = "...##InputMeshSurfaceFile"; pInputControlLabel = "##RoughnessStrength"; }
							if (texslot == 3) { pInputLabel = ""; pInputBtnLabel = ""; pInputControlLabel = "##MetalnessStrength"; }
							if (texslot == 4) { pInputLabel = ""; pInputBtnLabel = ""; pInputControlLabel = ""; }
						}
						else
						{
							// viewed from importer
							if (texslot == 2) { pInputLabel = "##InputMeshRoughness"; pInputBtnLabel = "...##InputMeshRoughnessFile"; pInputControlLabel = "##RoughnessStrength"; }
							if (texslot == 3) { pInputLabel = "##InputMeshMetalness"; pInputBtnLabel = "...##InputMeshMetalnessFile"; pInputControlLabel = "##MetalnessStrength"; }
							if (texslot == 4) { pInputLabel = "##InputMeshOcclusion"; pInputBtnLabel = "...##InputMeshOcclusionFile"; pInputControlLabel = "##OcclusionStrength"; }
						}
						if (texslot == 5) { pInputLabel = "##InputMeshEmissive"; pInputBtnLabel = "...##InputMeshEmissiveFile"; pInputControlLabel = "##EmissiveStrength"; }
						if (texslot == 6) { pInputLabel = "##InputMeshHeight"; pInputBtnLabel = "...##InputMeshHeightFile"; pInputControlLabel = "##HeightStrength"; }

						// texture types
						LPSTR pTitle = "", pTip = "";
						if (texslot == 0) { pTitle = "Albedo"; pTip = "Allows you to change the albedo / color texture of the object"; }
						if (texslot == 1) { pTitle = "Normal"; pTip = "Allows you to change the normal map texture of the object"; }
						if (bFromCustomMaterials == true)
						{
							if (texslot == 2) { pTitle = "Surface"; pTip = "Allows you to change the surface texture of the object."; }
							if (texslot == 3) { pTitle = ""; pTip = ""; }
							if (texslot == 4) { pTitle = ""; pTip = ""; }
						}
						else
						{
							if (texslot == 2) { pTitle = "Roughness"; pTip = "Allows you to change the roughness texture of the object."; }
							if (texslot == 3) { pTitle = "Metalness"; pTip = "Allows you to change the metalness texture of the object."; }
							if (texslot == 4) { pTitle = "Occlusion"; pTip = "Allows you to change the ambient occlusion texture of the object."; }
						}
						if (texslot == 5) { pTitle = "Emissive"; pTip = "Allows you to change the emissive texture of the object."; }
						if (texslot == 6) { pTitle = "Height"; pTip = "Allows you to change the displacement height texture of the object."; }

						// control title and desc
						LPSTR pControlTitle = "", pControlTip = "";
						if (texslot == 0) { pControlTitle = "Alpha Clipping";  pControlTip = "Alpha channel values below this value are clipped and not rendered."; }
						if (texslot == 1) { pControlTitle = "Normal Strength";  pControlTip = "Controls how strong the normal mapping effect is on the object."; }
						if (texslot == 2) { pControlTitle = "Roughness Strength";  pControlTip = "Controls how much of the roughness texture is applied to the object."; }
						if (texslot == 3) { pControlTitle = "Metalness Strength";  pControlTip = "Controls how much of the metalness texture is applied to the object."; }
						if (texslot == 4) { pControlTitle = "";  pControlTip = ""; }
						if (texslot == 5) { pControlTitle = "Emissive Strength";  pControlTip = "Controls how much of the emissive texture is applied to the object."; }
						if (texslot == 6) { pControlTitle = "";  pControlTip = ""; }

						// extra info message and image
						LPSTR pTexSlotInfo = "", pTexSlotInfoImage = "";
						if (texslot == 0) { pTexSlotInfo = "An albedo texture is an image texture that represents the color of an object, before any lighting calculations have been applied to it."; pTexSlotInfoImage = ""; }
						if (texslot == 1) { pTexSlotInfo = "A normal map texture is a special type of image that describes the undulations of a surface. Each pixel represents an angle offset away from the surface - for example it could be the edge of a rock or how clothes have folds in them. Normal maps are used to add extra depth and detail to an object without increasing the number of polygons, thus allowing for more polygons to be used elsewhere in your game."; pTexSlotInfoImage = ""; }
						if (bFromCustomMaterials == true)
						{
							if (texslot == 2) { pTexSlotInfo = "Surface textures represent the physically-based material of an object. It combines ambient occlusion, roughness and metalness into its RGBA channels. Surface textures are used to control how light interacts with an object and alters the extent to which it resembles metal, wood or another material type from the real world. This allows for the object to look physically accurate."; pTexSlotInfoImage = ""; }
							if (texslot == 3) { pTexSlotInfo = ""; pTexSlotInfoImage = ""; }
							if (texslot == 4) { pTexSlotInfo = ""; pTexSlotInfoImage = ""; }
						}
						else
						{
							if (texslot == 2) { pTexSlotInfo = "A roughness map describes which part of the surface is rough, and by inference, which parts are smooth and potentially glossy. Roughness is placed into the green channel of the surface texture."; pTexSlotInfoImage = ""; }
							if (texslot == 3) { pTexSlotInfo = "A metalness map is an image that describes how much metal is represented on the surface, with white being fully metallic and black being no metal at all. Metalness is placed into the blue channel of the surface texture."; pTexSlotInfoImage = ""; }
							if (texslot == 4) { pTexSlotInfo = "An ambient occlusion (AO) map describes how much baked-in shadows are part of the surface, and can be used to add very small scale lighting information to compliment the global lighting system. Ambient Occlusion is placed into the red channel of the surface texture."; pTexSlotInfoImage = ""; }
						}
						if (texslot == 5) { pTexSlotInfo = "An emissive map describes which parts of the object will generate its own light. By varying the emissive strength, you can alter how much of the surface color is projected outward from the object. Example use cases include using an emissive map to produce a glow effect, or adding lighting to a TV screen placed in your level."; pTexSlotInfoImage = ""; }
						if (texslot == 6) { pTexSlotInfo = "A displacement height map describes the vertical height of the detail of the texture. This texture used to be called a bump map as a lighter color would represent a higher bump than a darker lower pixel."; pTexSlotInfoImage = ""; }

						// the actual wicked slot, and iDelayedExecute update code
						int iDelayedExecuteCodeForThisSlot = 0;
						MaterialComponentTEXTURESLOT wickedTextureSlot;
						if (texslot == 0)
						{
							wickedTextureSlot = MaterialComponentTEXTURESLOT::BASECOLORMAP;
							iDelayedExecuteCodeForThisSlot = 30;
							if (t.importer.bEditAllMesh)
								iDelayedExecuteCodeForThisSlot = 50;
						}
						if (texslot == 1)
						{
							wickedTextureSlot = MaterialComponentTEXTURESLOT::NORMALMAP;
							iDelayedExecuteCodeForThisSlot = 31;
							if (t.importer.bEditAllMesh)
								iDelayedExecuteCodeForThisSlot = 51;
						}
						if (bFromCustomMaterials == true)
						{
							if (texslot == 2)
							{
								wickedTextureSlot = MaterialComponentTEXTURESLOT::SURFACEMAP;
								iDelayedExecuteCodeForThisSlot = 44;
								if (t.importer.bEditAllMesh)
									iDelayedExecuteCodeForThisSlot = 52;
							}
							if (texslot == 3)
							{
								wickedTextureSlot = MaterialComponentTEXTURESLOT::SURFACEMAP;
								iDelayedExecuteCodeForThisSlot = 44;
								if (t.importer.bEditAllMesh)
									iDelayedExecuteCodeForThisSlot = 52;
							}
							if (texslot == 4)
							{
								wickedTextureSlot = MaterialComponentTEXTURESLOT::SURFACEMAP;
								iDelayedExecuteCodeForThisSlot = 44;
								if (t.importer.bEditAllMesh)
									iDelayedExecuteCodeForThisSlot = 52;
							}
						}
						else
						{
							if (texslot == 2)
							{
								wickedTextureSlot = MaterialComponentTEXTURESLOT::SURFACEMAP;
								iDelayedExecuteCodeForThisSlot = 42;
								t.importer.bEditingAllSurfaceMeshes = false;
								if (t.importer.bEditAllMesh)
									t.importer.bEditingAllSurfaceMeshes = true;
							}
							if (texslot == 3)
							{
								wickedTextureSlot = MaterialComponentTEXTURESLOT::SURFACEMAP;
								iDelayedExecuteCodeForThisSlot = 43;
								t.importer.bEditingAllSurfaceMeshes = false;
								if (t.importer.bEditAllMesh)
									t.importer.bEditingAllSurfaceMeshes = true;
							}
							if (texslot == 4)
							{
								wickedTextureSlot = MaterialComponentTEXTURESLOT::SURFACEMAP;
								iDelayedExecuteCodeForThisSlot = 41;
								t.importer.bEditingAllSurfaceMeshes = false;
								if (t.importer.bEditAllMesh)
									t.importer.bEditingAllSurfaceMeshes = true;
							}
						}
						if (texslot == 5)
						{
							wickedTextureSlot = MaterialComponentTEXTURESLOT::EMISSIVEMAP;
							iDelayedExecuteCodeForThisSlot = 34;
							if (t.importer.bEditAllMesh)
								iDelayedExecuteCodeForThisSlot = 53;
						}
						if (texslot == 6)
						{
							wickedTextureSlot = MaterialComponentTEXTURESLOT::DISPLACEMENTMAP;
							iDelayedExecuteCodeForThisSlot = 33;
							if (t.importer.bEditAllMesh)
								iDelayedExecuteCodeForThisSlot = 54;
						}

						// current texture name
						if (texslot >= 2 && texslot <= 4)
						{
							// from DBO mesh texture ref (for surface components)
							if (pChosenMesh->dwTextureCount > GG_MESH_TEXTURE_OCCLUSION)
							{
								// takes reference from DBO
								if (bFromCustomMaterials == true)
								{
									if (texslot == 2) strcpy(materialname, pChosenMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName);
									if (texslot == 3) strcpy(materialname, pChosenMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName);
									if (texslot == 4) strcpy(materialname, pChosenMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName);
								}
								else
								{
									if (texslot == 2) strcpy(materialname, pChosenMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName);
									if (texslot == 3) strcpy(materialname, pChosenMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName);
									if (texslot == 4) strcpy(materialname, pChosenMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName);
								}
							}
							else
							{
								// has no reference for now
								strcpy(materialname, "");
							}
						}
						else
						{
							// or direct from wicked material resource
							if (pObjectMaterial->textures[wickedTextureSlot].resource)
								strcpy(materialname, pObjectMaterial->textures[wickedTextureSlot].name.c_str());
							else
								strcpy(materialname, "");
						}

						// layout for specific texture type
						if (mode == 5)
						{
							// confirm the texture name is different before showing
							bool bTextureIsDifferentFromLast = true;
							if (stricmp(materialname, lastmaterialname) == NULL)
							{
								bTextureIsDifferentFromLast = false;
							}
							strcpy(lastmaterialname, materialname);

							// show as grid of texture if readonly
							if (bTextureIsDifferentFromLast == true && pObjectMaterial->textures && pObjectMaterial->textures[wickedTextureSlot].resource)
							{
								void* pmat = (void*)pObjectMaterial->textures[wickedTextureSlot].GetGPUResource();
								ImGui::ImgBtnWicked((void*)pmat, ImVec2(mode5_icon_size, mode5_icon_size), ImColor(0, 0, 0, 255));
								if (ImGui::IsItemHovered())
								{
									ImGui::BeginTooltip();
									ImGui::ImgBtnWicked((void*)pmat, ImVec2(280, 280), ImColor(0, 0, 0, 255));
									ImGui::TextCenter(pTitle);
									ImGui::EndTooltip();
								}
								mode5_displayed_materials++;
								if (mode5_displayed_materials < mode5_materials)
									ImGui::SameLine();
							}
						}
						else
						{
							// is the texture slot a valid one
							bool bValidTexSlot = false;
							if (strlen(pTitle) > 0) bValidTexSlot = true;

							// title available
							ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
							if (bValidTexSlot == true)
							{
								// title valid
								ImGui::Text(pTitle);

								// extra help icon
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start, ImGui::GetCursorPosY() - 5));
								ImGui::PushID(iInfoUniqueId++);
								if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false))
								{
									// Display additional information on click.
									cInfoMessage = pTexSlotInfo;
									cInfoImage = pTexSlotInfoImage;
									bInfo_Window = true;
								}
								ImGui::PopID();
								if (ImGui::IsItemHovered()) ImGui::SetTooltip(pTip);
							}

							// make room for RGBA buttons
							ImGui::SameLine();
							bool bShowRGBAButtons = false;
							if (texslot >= 2 && texslot <= 4 && bFromCustomMaterials == false && bUseRGBAButtons)
							{
								bShowRGBAButtons = true;
								ImGui::PushItemWidth(-50 - path_gadget_size);
							}
							else
							{
								ImGui::PushItemWidth(-10 - path_gadget_size);
							}

							// name of texture
							if (bValidTexSlot == true)
							{
								if (ImGui::InputText(pInputLabel, &materialname[0], MAX_PATH, iInputFlags))
								{
									pSelectedMaterial = pObjectMaterial;
									pSelectedMesh = pChosenMesh;
									iDelayedExecute = iDelayedExecuteCodeForThisSlot;
									strcpy(cPreSelectedFile, materialname);
									if (strlen(cPreSelectedFile) == 0)
									{
										// triggers relevant texture data to be erased from surface texture
										if (texslot == 5) pSelectedMaterial->textures[wickedTextureSlot].name = "";
										iDelayedExecuteChannel = -2;
									}
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip((materialname && *materialname) ? materialname : "Select Texture File");
								if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
							}
							ImGui::PopItemWidth();

							// if not readonly or EBE, allow new texture file to be specified
							if (mode != 3 && mode != 5)
							{
								// control R,G,B,A channels
								if (bShowRGBAButtons == true && bFromCustomMaterials == false)
								{
									// Decide which buttons should be darkened to convey to the user which channel the texture data will go into.
									int iBoldTexSlot = -1;
									switch (texslot)
									{
										case 2: iBoldTexSlot = 1;// Green
											break;
										case 3: iBoldTexSlot = 2;// Blue
											break;
										case 4: iBoldTexSlot = 0;// Red
											break;
									}
									ImGui::SameLine();
									ImGui::PushItemWidth(path_gadget_size);
									char pInputTextureChannel[MAX_PATH];
									sprintf(pInputTextureChannel, "R%sR", pInputLabel);
									if (0 - iBoldTexSlot == 0)
									{
										if (ImGui::StyleButtonDark(pInputTextureChannel))
										{
											iDelayedExecuteChannel = 0;
											launch_file = true;
											bChooseSurfaceChannel = true;
										}
									}
									else
									{
										if (ImGui::StyleButton(pInputTextureChannel))
										{
											iDelayedExecuteChannel = 0;
											launch_file = true;
											bChooseSurfaceChannel = true;
										}
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select texture file to take texture data from the red channels");
									ImGui::SameLine();
									ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 10, ImGui::GetCursorPosY()));
									sprintf(pInputTextureChannel, "G%sG", pInputLabel);
									if (1 - iBoldTexSlot == 0)
									{
										if (ImGui::StyleButtonDark(pInputTextureChannel))
										{
											iDelayedExecuteChannel = 1;
											launch_file = true;
											bChooseSurfaceChannel = true;
										}
									}
									else
									{
										if (ImGui::StyleButton(pInputTextureChannel))
										{
											iDelayedExecuteChannel = 1;
											launch_file = true;
											bChooseSurfaceChannel = true;
										}
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select texture file to take texture data from the green channels");
									ImGui::SameLine();
									ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 10, ImGui::GetCursorPosY()));
									sprintf(pInputTextureChannel, "B%sB", pInputLabel);
									if (2 - iBoldTexSlot == 0)
									{
										if (ImGui::StyleButtonDark(pInputTextureChannel))
										{
											iDelayedExecuteChannel = 2;
											launch_file = true;
											bChooseSurfaceChannel = true;
										}
									}
									else
									{
										if (ImGui::StyleButton(pInputTextureChannel))
										{
											iDelayedExecuteChannel = 2;
											launch_file = true;
											bChooseSurfaceChannel = true;
										}
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select texture file to take texture data from the blue channels");
									ImGui::SameLine();
									ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 10, ImGui::GetCursorPosY()));
									sprintf(pInputTextureChannel, "A%sA", pInputLabel);
									if (ImGui::StyleButton(pInputTextureChannel)) { iDelayedExecuteChannel = 3;  launch_file = true; bChooseSurfaceChannel = true; }
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select texture file to take texture data from the alpha channels");
									if (launch_file && iDelayedExecute == 0)
									{
										pSelectedMaterial = pObjectMaterial;
										pSelectedMesh = pChosenMesh;
										strcpy(cPreSelectedFile, "");
										iDelayedExecute = iDelayedExecuteCodeForThisSlot;
									}
									ImGui::PopItemWidth();
								}
								else
								{
									if (bValidTexSlot == true)
									{
										ImGui::SameLine();
										ImGui::PushItemWidth(path_gadget_size);
										if (ImGui::StyleButton(pInputBtnLabel))
										{
											cPreSelectedFile[0] = 0;
											launch_file = true;
										}
										if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select texture file to take texture data from all channels");
										if (launch_file && iDelayedExecute == 0)
										{
											pSelectedMaterial = pObjectMaterial;
											pSelectedMesh = pChosenMesh;
											strcpy(cPreSelectedFile, "");
											iDelayedExecute = iDelayedExecuteCodeForThisSlot;
										}
										ImGui::PopItemWidth();
									}
									else
									{
										if (texslot >= 2 && texslot <= 3)
										{
											// spacing for when in custom materials mode between roughness and metalness
											ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 25));
										}
									}
								}
							}

							// display texture and preview if hovered over
							if (pObjectMaterial->textures[wickedTextureSlot].resource)
							{
								if (bValidTexSlot == true)
								{
									ImVec2 vOldPos = ImGui::GetCursorPos();
									ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 5, ImGui::GetCursorPosY() - 10));
									// LB: would be nice if the occlusion, roughness and metalness could show just the relevant channel from surface texture!
									// Red Only: if (ImGui::ImgBtnWicked((void*)pmat, ImVec2(preview_icon_size*2.9f, preview_icon_size*2.9f), ImColor(0, 0, 0, 255),ImColor(220, 0, 0, 255),ImColor(255, 0, 0, 255),ImColor(180, 0, 0, 255)))
									// Green Only: if (ImGui::ImgBtnWicked((void*)pmat, ImVec2(preview_icon_size*2.9f, preview_icon_size*2.9f), ImColor(0, 0, 0, 255),ImColor(0, 220, 0, 255),ImColor(0, 255, 0, 255),ImColor(0, 180, 0, 255)))
									// Blue Only: if (ImGui::ImgBtnWicked((void*)pmat, ImVec2(preview_icon_size*2.9f, preview_icon_size*2.9f), ImColor(0, 0, 0, 255),ImColor(0, 0, 220, 255),ImColor(0, 0, 255, 255),ImColor(0, 0, 180, 255)))
									// Alpha Only: would need some special setup for that :)

									// Flag that controls whether or not the texture will be displayed or a symbol...
									// ...that lets the user know that the meshes do not share the same texture.
									bool bMeshesHaveDifferentTex = true;
									char cTextureSettingTooltip[256];
									strcpy(cTextureSettingTooltip, "Meshes have different ");

									switch (wickedTextureSlot)
									{
										case MaterialComponentTEXTURESLOT::BASECOLORMAP:
										{
											bMeshesHaveDifferentTex = t.importer.bMeshesHaveDifferentBase;
											strcat(cTextureSettingTooltip, "base colour texture.");
											break;
										}
										case MaterialComponentTEXTURESLOT::NORMALMAP:
										{
											bMeshesHaveDifferentTex = t.importer.bMeshesHaveDifferentNormal;
											strcat(cTextureSettingTooltip, "normal texture.");
											break;
										}
										case MaterialComponentTEXTURESLOT::SURFACEMAP:
										{
											bMeshesHaveDifferentTex = t.importer.bMeshesHaveDifferentSurface;
											strcat(cTextureSettingTooltip, "surface texture.");
											break;
										}
										case MaterialComponentTEXTURESLOT::EMISSIVEMAP:
										{
											bMeshesHaveDifferentTex = t.importer.bMeshesHaveDifferentEmissive;
											strcat(cTextureSettingTooltip, "emissive texture");
											break;
										}
										case MaterialComponentTEXTURESLOT::DISPLACEMENTMAP:
										{
											bMeshesHaveDifferentTex = t.importer.bMeshesHaveDifferentDisplacement;
											strcat(cTextureSettingTooltip, "height texture");
											break;
										}
									}

									if (t.importer.bEditAllMesh && bMeshesHaveDifferentTex)
									{
										ImGui::PushID(iInfoUniqueId++);
										// Display the symbol that tells the user that not all meshes share the same texture.
										if (ImGui::ImgBtn(IMPORTER_ALL_MESH, ImVec2(preview_icon_size * 2.9f, preview_icon_size * 2.9f), ImColor(255, 255, 255, 255)))
										{
											if (mode != 3 && mode != 5)
											{
												launch_file = true;
											}
										}

										if (ImGui::IsItemHovered())
										{
											ImGui::BeginTooltip();
											ImGui::Text(cTextureSettingTooltip);
											ImGui::EndTooltip();
										}
										ImGui::PopID();
									}
									else
									{
										// Display the texture normally.
										if (bReadOnly)
										{
											ImGui::PopItemFlag();
											ImGui::PopStyleVar();
										}
										void* pmat = (void*)pObjectMaterial->textures[wickedTextureSlot].GetGPUResource();
										if (ImGui::ImgBtnWicked((void*)pmat, ImVec2(preview_icon_size * 2.9f, preview_icon_size * 2.9f), ImColor(0, 0, 0, 255)))
											if (mode != 3 && mode != 5 && !bReadOnly)
											{
												if (texslot >= 2 && texslot <= 4)
												{
													// Using the image button will not take texture data from a single channel, so should not copy the channel to a new texture. It should replace it entirely.
													iDelayedExecuteCodeForThisSlot = 44;
													// Editing all meshes.
													launch_file = true;
												}
												else
												{
													launch_file = true;
												}
											}

										if (ImGui::IsItemHovered())
										{
											ImGui::BeginTooltip();
											ImGui::ImgBtnWicked((void*)pmat, ImVec2(180, 180), ImColor(0, 0, 0, 255));
											ImGui::EndTooltip();
										}
										if (bReadOnly)
										{
											ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
											ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
										}
									}

									// If the user tried to change the texture, set the correct code for the delayed execution loop.
									if (launch_file && iDelayedExecute == 0)
									{
										pSelectedMaterial = pObjectMaterial;
										pSelectedMesh = pChosenMesh;
										strcpy(cPreSelectedFile, "");
										iDelayedExecute = iDelayedExecuteCodeForThisSlot;
									}

									ImGui::SetCursorPos(vOldPos);
								}
							}

							// if not readonly or EBE, offer control value
							if (strlen(pControlTitle) > 0)
							{
								if (pObjectMaterial->textures[wickedTextureSlot].resource)
								{
									if (mode != 3 && mode != 5)
									{
										// control title
										ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));
										ImGui::PushItemWidth(-10 - 4);
										ImGui::TextCenter(pControlTitle);
										ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));

										// control slider (specific code)
										float fValue = 0.0f;
										switch (texslot)
										{
											case 0: // albedo alphaclip
												fValue = pObjectMaterial->alphaRef;
												if (ImGui::SliderFloat(pInputControlLabel, &fValue, 0.0, 1.0))
												{
													pObjectMaterial->SetAlphaRef(fValue);

													// This only does something if edit all meshes is enabled. 
													importer_set_all_material_settings(texslot, fValue);

													bHaveMaterialUpdate = true;
												}
												if (ImGui::IsItemHovered()) ImGui::SetTooltip(pControlTip);
												break;

											case 1: // normal map settings.
												fValue = pObjectMaterial->normalMapStrength;
												if (ImGui::SliderFloat(pInputControlLabel, &fValue, 0.0, 4.0))
												{
													pObjectMaterial->SetNormalMapStrength(fValue);

													// This only does something if edit all meshes is enabled. 
													importer_set_all_material_settings(texslot, fValue);

													bHaveMaterialUpdate = true;
												}
												if (ImGui::IsItemHovered()) ImGui::SetTooltip(pControlTip);
												ImGui::Indent(80.0f);
												if (ImGui::Checkbox("Invert Green Channel", &t.importer.bInvertNormalMap))
												{
													if (!t.importer.bInvertNormalMap)
													{
														// Revert to the original normal map.
														strcpy(cPreSelectedFile, t.importer.pOrigNormalMap);
														iDelayedExecute = 31;
													}
													else
													{
														// Generate a new normal map, with an inverted green channel to solve handedness issues between Wicked and other software.
														iDelayedExecute = 45;
													}
												}
												if (ImGui::IsItemHovered()) ImGui::SetTooltip("Used for fixing compatibility issues between DirectX and OpenGL normal maps");

												ImGui::Indent(-80.0f);
												break;

											case 2: // roughness strength
												fValue = pObjectMaterial->roughness;
												if (ImGui::SliderFloat(pInputControlLabel, &fValue, 0.0, 1.0))
												{
													pObjectMaterial->SetRoughness(fValue);

													// This only does something if edit all meshes is enabled. 
													importer_set_all_material_settings(texslot, fValue);

													bHaveMaterialUpdate = true;
													if (ImGui::IsItemHovered()) ImGui::SetTooltip(pControlTip);
												}
												break;

											case 3: // metalness strength
												fValue = pObjectMaterial->metalness;
												if (ImGui::SliderFloat(pInputControlLabel, &fValue, 0.0, 1.0))
												{
													pObjectMaterial->SetMetalness(fValue);

													// This only does something if edit all meshes is enabled. 
													importer_set_all_material_settings(texslot, fValue);

													bHaveMaterialUpdate = true;
													if (ImGui::IsItemHovered()) ImGui::SetTooltip(pControlTip);
												}
												break;

											case 4: // occlusion strength - none
												break;

											case 5: // emissive strength
												fValue = pObjectMaterial->GetEmissiveStrength();
												if (ImGui::SliderFloat(pInputControlLabel, &fValue, 0.0, 30.0))
												{
													pObjectMaterial->SetEmissiveStrength(fValue);
													pObjectMaterial->SetDirty();
													WickedCall_SetMeshMaterial(pChosenMesh, false);

													// This only does something if edit all meshes is enabled. 
													importer_set_all_material_settings(texslot, fValue);

													bHaveMaterialUpdate = true;
												}
												if (ImGui::IsItemHovered()) ImGui::SetTooltip(pControlTip);
												break;
										}
										ImGui::PopItemWidth();
									}
								}
							}
							else
							{
								// no control value
								if (mode != 3 && mode != 5)
								{
									ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));
									ImGui::PushItemWidth(-10 - 4);
									ImGui::TextCenter("");
									ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));
									ImGui::TextCenter("");
									ImGui::PopItemWidth();
								}
							}
						}
					}

					// emissive color
					if (mode != 5)
					{
						EmmisiveColor[0] = pChosenMesh->mMaterial.Emissive.r;
						EmmisiveColor[1] = pChosenMesh->mMaterial.Emissive.g;
						EmmisiveColor[2] = pChosenMesh->mMaterial.Emissive.b;
						EmmisiveColor[3] = pChosenMesh->mMaterial.Emissive.a;
						ImGui::TextCenter("Emissive Color");
						mycolor = ImVec4(EmmisiveColor[0], EmmisiveColor[1], EmmisiveColor[2], EmmisiveColor[3]);
						open_popup = ImGui::ColorButton("##NewV2WickedEmissiveColor", mycolor, 0, ImVec2(w - 10.0, 0));
						if (open_popup)	ImGui::OpenPopup("##pickWickedEmissiveColor");
						if (ImGui::BeginPopup("##pickWickedEmissiveColor", ImGuiWindowFlags_NoMove))
						{
							if (ImGui::ColorPicker4("##EmmisiveColor", &EmmisiveColor[0], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayRGB))
							{
								pChosenMesh->mMaterial.Emissive.r = EmmisiveColor[0];
								pChosenMesh->mMaterial.Emissive.g = EmmisiveColor[1];
								pChosenMesh->mMaterial.Emissive.b = EmmisiveColor[2];
								pChosenMesh->mMaterial.Emissive.a = EmmisiveColor[3];
								dwEmmisiveColor = ((unsigned int)(EmmisiveColor[0] * 255) << 24);
								dwEmmisiveColor += ((unsigned int)(EmmisiveColor[1] * 255) << 16);
								dwEmmisiveColor += ((unsigned int)(EmmisiveColor[2] * 255) << 8);
								dwEmmisiveColor += ((unsigned int)(EmmisiveColor[3] * 255));
								WickedCall_SetMeshMaterial(pChosenMesh, false);
								bHaveMaterialUpdate = true;

								importer_set_all_material_colour(5, EmmisiveColor);
							}
							ImGui::EndPopup();
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Change Emissive Color");
						window = ImGui::GetCurrentWindow(); //PE: Add a pencil to all color gadgets.
						lpTexture = GetImagePointerView(TOOL_PENCIL);
						vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - (ImGui::GetFontSize() * 1.5f) - 3.0f };
						if (lpTexture)
							window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
					}
				}

				if (mode != 5)
				{
					if (mode != 6)
					{
						//Reflectance
						ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
						ImGui::Text("Reflectance");
						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
						ImGui::PushID(iInfoUniqueId++);
						if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false))
						{
							//Display additional information on click.
							cInfoMessage = "The reflectance of the surface describes how much of the light received by the surface is reflected away unmodified. Setting your surface to maximum reflectance would turn it into a crude mirror.";
							cInfoImage = ""; //Image that descripe this information window. "tutorialbank\\information-default.jpg".
							bInfo_Window = true; //Open information window.
						}
						ImGui::PopID();
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allows you to change the reflectance of the object.");
						ImGui::SameLine();

						ImGui::PushItemWidth(-10 - 4);
						fReflectance = pObjectMaterial->reflectance;
						if (ImGui::SliderFloat("##Reflectance strength", &fReflectance, 0.0, 1.0))
						{
							pObjectMaterial->SetReflectance(fReflectance);
							bHaveMaterialUpdate = true;

							// This only does something if t.importer.bEditAllMesh is true.
							importer_set_all_material_settings(6, fReflectance);
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Surface reflectance strength");
						ImGui::PopItemWidth();

						//Render Order Bias
						ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
						ImGui::Text("Render Bias");
						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
						ImGui::PushID(iInfoUniqueId++);
						if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false))
						{
							//Display additional information on click.
							cInfoMessage = "The render order bias can control the priority at which an object is rendered by adding an artificial distance to the final distance sort when deciding which objects should render from the most distant to the nearest to the camera.";
							cInfoImage = ""; //Image that descripe this information window. "tutorialbank\\information-default.jpg".
							bInfo_Window = true; //Open information window.
						}
						ImGui::PopID();
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set the transparency render order distance bias to prioritise rendering of overlapping objects. They must be transparent and double sided.");
						ImGui::SameLine();

						ImGui::PushItemWidth(-10 - 4);
						//float fNeedToGoInPropertiesSoCanSave = WickedCall_GetRenderOrderBias(pChosenMesh);
						//if (ImGui::SliderFloat("##Transparency distance bias", &fNeedToGoInPropertiesSoCanSave, -500.0f, 500.0f))
						int iNeedToGoInPropertiesSoCanSave = WickedCall_GetRenderOrderBias(pChosenMesh);
						if (ImGui::SliderInt("##Transparency distance bias", &iNeedToGoInPropertiesSoCanSave, -500, 500))
						{
							WickedCall_SetRenderOrderBias(pChosenMesh, iNeedToGoInPropertiesSoCanSave);
							bHaveMaterialUpdate = true;

							// This only does something if t.importer.bEditAllMesh is true.
							importer_set_all_material_settings(7, iNeedToGoInPropertiesSoCanSave);
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render Order Distance Bias");
						ImGui::PopItemWidth();
					}

					//PE: Disable pObjectMaterial->customShaderID
					if (pObjectMaterial->customShaderID == -1)
					{
						//Object checkboxes.
						bTransparent = pObjectMaterial->userBlendMode == BLENDMODE_ALPHA;
						if (ImGui::Checkbox("Transparent", &bTransparent))
						{
							if (bTransparent)
							{
								pObjectMaterial->userBlendMode = BLENDMODE_ALPHA;
								pObjectMaterial->SetDirty();
							}
							else
							{
								pObjectMaterial->userBlendMode = BLENDMODE_OPAQUE;
								pObjectMaterial->SetDirty();
							}
							if (t.importer.bEditAllMesh)
							{
								importer_set_all_material_transparent(bTransparent);
							}
							else
							{
								// apply new state to current mesh
								if (pChosenMesh)
								{
									pChosenMesh->bTransparency = bTransparent;
								}
							}
							bCloneChangesToAllObjectsInRubberBand = true;
							bHaveMaterialUpdate = true;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Transparent");

						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(help_start + 80, ImGui::GetCursorPosY()));
						ImGui::PushID(iInfoUniqueId++);
						if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false))
						{
							//Display additional information on click.
							cInfoMessage = "Transparency controls how much of the surface is see-through, so a low transparency would make the surface opaque (solid) and a high transparency would allow you to see through it.";
							cInfoImage = ""; //Image that descripe this information window. "tutorialbank\\information-default.jpg".
							bInfo_Window = true; //Open information window.
						}
						ImGui::PopID();
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allows you to make the object transparent.");

						bDoubleSided = mesh->IsDoubleSided();
						if (ImGui::Checkbox("Double Sided", &bDoubleSided))
						{
							mesh->SetDoubleSided(bDoubleSided);
							if (t.importer.bEditAllMesh)
							{
								importer_set_all_mesh_double_sided(bDoubleSided);
							}
							bCloneChangesToAllObjectsInRubberBand = true;
							bHaveMaterialUpdate = true;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set object to render both sides of its polygons");

						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(help_start + 80, ImGui::GetCursorPosY()));
						ImGui::PushID(iInfoUniqueId++);
						if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false))
						{
							//Display additional information on click.
							cInfoMessage = "Objects are made up of polygons, but are only visible from one side. By setting an object to render double sided, it will render both sides of all polygons. This could be used for things like leaves and cobwebs, among many other cases.";
							cInfoImage = ""; //Image that descripe this information window. "tutorialbank\\information-default.jpg".
							bInfo_Window = true; //Open information window.
						}
						ImGui::PopID();
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allows you to set objects so they render both sides of their polygons");

						bPlanerReflection = pObjectMaterial->shaderType == wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION;
						if (ImGui::Checkbox("Planar Reflection", &bPlanerReflection))
						{
							if (bPlanerReflection)
							{
								pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION;
							}
							else
							{
								if (pObjectMaterial->parallaxOcclusionMapping > 0.0f)
									pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING;
								else
									pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR;
							}

							importer_set_all_material_planar_reflection(bPlanerReflection);

							bCloneChangesToAllObjectsInRubberBand = true;
							bHaveMaterialUpdate = true;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Planar Reflection");

						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(help_start + 80, ImGui::GetCursorPosY()));
						ImGui::PushID(iInfoUniqueId++);
						if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false))
						{
							//Display additional information on click.
							cInfoMessage = "Planar reflection is a technique which assumes the reflective surface is horizontal, such as a puddle or reflective floor, and can improve the visuals when specified. However, this does come with an increased performance cost.";
							cInfoImage = ""; //Image that descripe this information window. "tutorialbank\\information-default.jpg".
							bInfo_Window = true; //Open information window.
						}
						ImGui::PopID();
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allows you to enable planer reflection on the object.");

						bCastShadows = pObjectMaterial->IsCastingShadow();
						if (ImGui::Checkbox("Cast Shadows", &bCastShadows))
						{
							pObjectMaterial->SetCastShadow(bCastShadows);
							importer_set_all_material_cast_shadow(bCastShadows);
							bCloneChangesToAllObjectsInRubberBand = true;
							bHaveMaterialUpdate = true;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Cast Shadows");

						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(help_start + 80, ImGui::GetCursorPosY()));
						ImGui::PushID(iInfoUniqueId++);
						if (ImGui::ImgBtn(ICON_INFO, ImVec2(preview_icon_size, preview_icon_size), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false)) //, bBoostIconColors
						{
							//Display additional information on click.
							cInfoMessage = "All objects have the ability to cast shadows, and this ability can be switched off. You may choose to switch off shadows for small objects, or effect objects such as a puddle that is so close to the floor it does not need to cast a shadow.";
							cInfoImage = ""; //Image that descripe this information window. "tutorialbank\\information-default.jpg".
							bInfo_Window = true; //Open information window.
						}
						ImGui::PopID();
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allows you to make the object cast shadows.");

					}

					#ifdef CUSTOMSHADERS
					if (mode != 6)
					{
						//PE: Add custom shader support here:
						void importer_set_all_material_shader_id(int shaderID, float p1, float p2, float p3, float p4, float p5, float p6, float p7);
						std::vector<wiRenderer::CustomShader> cshaders = wiRenderer::GetCustomShaders();
						int current_selection = pObjectMaterial->customShaderID;
						std::string comboselection = "None";
						if (pObjectMaterial->customShaderID >= 0 && pObjectMaterial->customShaderID < cshaders.size())
							comboselection = cshaders[pObjectMaterial->customShaderID].name;
						ImGui::PushItemWidth(-10);
						ImGui::TextCenter("Custom Shaders");
						if (ImGui::BeginCombo("##ImporterCustomShaders", comboselection.c_str(), ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge))
						{
							if (ImGui::Selectable("None"))
							{
								pObjectMaterial->customShaderID = -1;
								importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
								bHaveMaterialUpdate = true;
							}
							for (int i = 0; i < cshaders.size(); i++)
							{
								bool bSelected = false;
								if (i == 4) continue; //PE: Internal use for now.
								if (cshaders[i].bActive)
								{
									if (pObjectMaterial->customShaderID == i)
										bSelected = true;
									if (ImGui::Selectable(cshaders[i].name.c_str(), bSelected))
									{
										pObjectMaterial->customShaderID = i;
										if (i == 1)
										{
											//PE: Default parameters.
											if (t.visuals.tree_wind > 0)
												pObjectMaterial->customShaderParam1 = 1.0f;
											else
												pObjectMaterial->customShaderParam1 = 0.20f;
										}
										if (i == 3)
										{
											//PE: Default glass parameters.
											pObjectMaterial->customShaderParam1 = 1.3f;
											pObjectMaterial->customShaderParam2 = 0.3f;
											pObjectMaterial->customShaderParam3 = 2.0f;
										}
										if (i == 4)
										{
											pObjectMaterial->customShaderParam1 = 2.0; //THICKNESS_FACTOR
											pObjectMaterial->customShaderParam2 = 3000; //FADE_DISTANCE
											pObjectMaterial->customShaderParam3 = 0.4f; //POWER_EXPONENT
											pObjectMaterial->customShaderParam4 = 0.4f; //BASE ALPHA
										}

										if (i == 5)
										{
											pObjectMaterial->customShaderParam1 = 0.5; //health
											pObjectMaterial->customShaderParam2 = 1.0; //splatter scale 1-10
											pObjectMaterial->customShaderParam3 = 0.5f; //Wetness 0-1
											pObjectMaterial->customShaderParam4 = 0.35f; //edgeFade
											pObjectMaterial->customShaderParam5 = 0.5f; //maxBlood
											pObjectMaterial->customShaderParam6 = 1.0f; //Brightness

										}
										importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
										bHaveMaterialUpdate = true;
									}
									if (bSelected) ImGui::SetItemDefaultFocus();
								}
							}

							ImGui::EndCombo();
						}
						ImGui::PopItemWidth();
						if (pObjectMaterial->customShaderID != -1)
						{
							ImGui::PushItemWidth(-10);
							ImGui::TextCenter("Custom Shaders Parameters");
							//PE: Parameters to shaders.
							int numpar = 0;
							float maxRange1 = 2.0f;
							std::string param1 = "Parameter 1";
							std::string param2 = "Parameter 2";
							std::string param3 = "Parameter 3";
							std::string param4 = "Parameter 4";
							std::string param5 = "Parameter 5";
							std::string param6 = "Parameter 6";
							std::string param7 = "Parameter 7";
							if (pObjectMaterial->customShaderID == 1)
							{
								numpar = 1;
								param1 = "Object Wind";
							}
							if (pObjectMaterial->customShaderID == 2)
							{
								numpar = 7;
								param1 = "UV Scale";
								param2 = "UV Speed";
								param3 = "Distorsion";
								param4 = "Direction";
								param5 = "TEX Scroll";
								param6 = "Foam Size";
								param7 = "TEX Scale";
							}
							if (pObjectMaterial->customShaderID == 3)
							{
								numpar = 3;
								param1 = "Transmission";
								param2 = "Refraction";
								param3 = "Brighten";
							}

							if (pObjectMaterial->customShaderID == 4)
							{
								numpar = 4;
								param1 = "Thickness";
								param2 = "Fade Dist";
								param3 = "Power Exp";
								param4 = "Min Alpha";
							}

							if (pObjectMaterial->customShaderID == 5)
							{
								numpar = 6;
								maxRange1 = 1.0f;
								param1 = "Health";
								param2 = "Splat Scale";
								param3 = "Wetness";
								param4 = "Edge Fade";
								param5 = "Max Blood";
								param6 = "Brightness";
							}

							if (numpar > 0)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param1.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa1", &pObjectMaterial->customShaderParam1, 0.0, maxRange1))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}

							if (numpar > 1)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param2.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa2", &pObjectMaterial->customShaderParam2, 0.0, 2.0))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}
							if (numpar > 2)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param3.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa3", &pObjectMaterial->customShaderParam3, 0.0, 2.0))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}
							if (numpar > 3)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param4.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa4", &pObjectMaterial->customShaderParam4, 0.0, 2.0))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}
							if (numpar > 4)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param5.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa5", &pObjectMaterial->customShaderParam5, 0.0, 2.0))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}
							if (numpar > 5)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param6.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa6", &pObjectMaterial->customShaderParam6, 0.0, 2.0))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}
							if (numpar > 6)
							{
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::Text(param7.c_str());
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(help_start + 20, ImGui::GetCursorPosY() - 5));
								if (ImGui::SliderFloat("##CuShaPa7", &pObjectMaterial->customShaderParam7, 0.0, 2.0))
								{
									importer_set_all_material_shader_id(pObjectMaterial->customShaderID, pObjectMaterial->customShaderParam1, pObjectMaterial->customShaderParam2, pObjectMaterial->customShaderParam3, pObjectMaterial->customShaderParam4, pObjectMaterial->customShaderParam5, pObjectMaterial->customShaderParam6, pObjectMaterial->customShaderParam7);
									pObjectMaterial->SetDirty();
									bHaveMaterialUpdate = true;
								}
							}

							ImGui::PopItemWidth();

						}
					}
					#endif				


					// gather for mode 6 (see later code)
					if (bCloneChangesToAllObjectsInRubberBand == true)
					{
						bCloneChangesToAllObjectsInRubberBandTransparency = bTransparent;
						bCloneChangesToAllObjectsInRubberBandDoubleSided = bDoubleSided;
						bCloneChangesToAllObjectsInRubberBandPlanarReflection = bPlanerReflection;
						bCloneChangesToAllObjectsInRubberBandCastShadow = bCastShadows;
					}
				}
			}
		}

		if (bReadOnly)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

	}

	// in mode 6, can make changes that will affect ALL objects in rubber band (All Materials)
	if (mode == 6)
	{
		if (bCloneChangesToAllObjectsInRubberBand == true)
		{
			for (int ii = 0; ii < g.entityrubberbandlist.size(); ii++)
			{
				int iEntityIndex = g.entityrubberbandlist[ii].e;
				sObject* pObject = g_ObjectList[t.entityelement[iEntityIndex].obj];
				if (pObject)
				{
					for (int iThisMeshIndex = 0; iThisMeshIndex < pObject->iMeshCount; iThisMeshIndex++)
					{
						sMesh* mesh = pObject->ppMeshList[iThisMeshIndex];
						if (mesh)
						{
							if (t.importer.bEditAllMesh == true || (t.importer.bEditAllMesh == false && iThisMeshIndex == iSelectedMesh))
							{
								wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);
								if (meshComponent)
								{
									// get material settings from mesh material or WEMaterial
									uint64_t materialEntity = meshComponent->subsets[0].materialID;
									wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

									// set specifics
									bool bTransparency = bCloneChangesToAllObjectsInRubberBandTransparency;
									t.entityelement[iEntityIndex].eleprof.WEMaterial.bTransparency[iThisMeshIndex] = bTransparency;
									mesh->bTransparency = bTransparency;
									if (mesh->bTransparency)
									{
										pMeshMaterial->userBlendMode = BLENDMODE_ALPHA;
									}
									else
									{
										pMeshMaterial->userBlendMode = BLENDMODE_OPAQUE;
									}

									// double sided
									bool bDoubleSided = bCloneChangesToAllObjectsInRubberBandDoubleSided;
									t.entityelement[iEntityIndex].eleprof.WEMaterial.bDoubleSided[iThisMeshIndex] = bDoubleSided;
									meshComponent->SetDoubleSided(bDoubleSided);

									// planar reflection
									bool planarReflection = bCloneChangesToAllObjectsInRubberBandPlanarReflection;
									t.entityelement[iEntityIndex].eleprof.WEMaterial.bPlanerReflection[iThisMeshIndex] = planarReflection;
									if (planarReflection)
									{
										pMeshMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION;
									}
									else
									{
										if (pMeshMaterial->parallaxOcclusionMapping > 0.0f)
											pMeshMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING;
										else
											pMeshMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR;
									}

									// cast shadows
									bool bCastShadow = bCloneChangesToAllObjectsInRubberBandCastShadow;
									t.entityelement[iEntityIndex].eleprof.WEMaterial.bCastShadows[iThisMeshIndex] = bCastShadows;
									pMeshMaterial->SetCastShadow(bCastShadows);

									// and dirty the materials
									pMeshMaterial->SetDirty();
								}
							}
						}
					}
				}
			}
		}
	}
}

void Wicked_CreateShortName ( int iMeshIndex, LPSTR pDest, LPSTR pNameFromMesh )
{
	// shorten texture name to basic material name
	char pShorten[MAX_PATH];
	strcpy (pShorten, "Noname");

	for (int n = strlen(pNameFromMesh); n > 0; n--)
	{
		if (pNameFromMesh[n] == '\\' || pNameFromMesh[n] == '/')
		{
			strcpy (pShorten, pNameFromMesh + n + 1);
			break;
		}
	}

	// If no name from texture, see if there are any names from when the object was imported into ASSIMP
	if (strlen(pNameFromMesh) == 0)
	{
		sObject* pObject = GetObjectData(t.importer.objectnumber);
		if (pObject && pObject->iMeshCount == g_MeshNamesAssimp.size())
		{
			pNameFromMesh = g_MeshNamesAssimp[iMeshIndex].Get();
			strcpy(pShorten, pNameFromMesh);
		}
	}

	LPSTR pChopString = "";
	char pLowerCaseVewrsion[MAX_PATH];
	strcpy(pLowerCaseVewrsion, pShorten);
	strlwr(pLowerCaseVewrsion);
	for (int choopychoppy = 0; choopychoppy < 7; choopychoppy++)
	{
		LPSTR pWhatToChop = "";
		if (choopychoppy == 0) pWhatToChop = ".dds";
		if (choopychoppy == 1) pWhatToChop = ".png";
		if (choopychoppy == 2) pWhatToChop = ".jpg";
		if (choopychoppy == 3) pWhatToChop = ".tga";
		if (choopychoppy == 4) pWhatToChop = "_color";
		if (choopychoppy == 5) pWhatToChop = "_basecolor";
		if (choopychoppy == 6) pWhatToChop = "_d";
		if (strnicmp(pLowerCaseVewrsion + strlen(pLowerCaseVewrsion) - strlen(pWhatToChop), pWhatToChop, strlen(pWhatToChop)) == NULL)
		{
			// only if find the matching string to chop AT THE END!
			pChopString = strstr (pLowerCaseVewrsion, pWhatToChop);
			if (pChopString)
			{
				int iChopAtPos = pChopString - pLowerCaseVewrsion;
				pShorten[iChopAtPos] = 0;
				strcpy(pLowerCaseVewrsion, pShorten);
				strlwr(pLowerCaseVewrsion);
			}
		}
	}

	// copy it back up to be used
	sprintf(pDest, "%d:%s", iMeshIndex, pShorten);
}

void Wicked_FindChosenMesh (sObject* pObject, sMesh** ppChosenMesh, int iUseThisMeshIndex)
{
	char meshname[256];
	strcpy(mesh_combo_entry, "1:Noname\0");
	if (iUseThisMeshIndex == -1)
	{
		for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
		{
			sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
			if (pMesh && pMesh->wickedmeshindex > 0)
			{
				LPSTR pNameFromMesh = pMesh->pTextures[0].pName;
				Wicked_CreateShortName(iMeshIndex, meshname, pNameFromMesh);
				//}
				strcpy(mesh_combo_entry, meshname);
				iSelectedMesh = iMeshIndex;
				if (iSelectedMesh >= MAXMESHMATERIALS - 1) //PE: We can crash if we go above the MAXMESHMATERIALS.
					iSelectedMesh = MAXMESHMATERIALS - 1;

				*ppChosenMesh = pMesh;
				break;
			}
		}
	}
	else
	{
		sMesh* pMesh = pObject->ppMeshList[iUseThisMeshIndex];
		if (pMesh && pMesh->wickedmeshindex > 0)
		{
			LPSTR pNameFromMesh = pMesh->pTextures[0].pName;
			Wicked_CreateShortName(iUseThisMeshIndex, meshname, pNameFromMesh);
			strcpy(mesh_combo_entry, meshname);
			iSelectedMesh = iUseThisMeshIndex;
			if (iSelectedMesh >= MAXMESHMATERIALS - 1) //PE: We can crash if we go above the MAXMESHMATERIALS.
				iSelectedMesh = MAXMESHMATERIALS - 1;

			*ppChosenMesh = pMesh;
		}
	}
}

void Wicked_Set_Material_From_grideleprof_ThisMesh(void* pVObject, int mode, entityeleproftype *edit_grideleprof, int iThisMeshIndex)
{
	// get object ptr and grideleprof
	sObject* pObject = (sObject*)pVObject;
	if (!pObject) return;
	if (!edit_grideleprof)
	{
		edit_grideleprof = &t.grideleprof;
	}

	// find first mesh
	sMesh* pChosenMesh = NULL;
	Wicked_FindChosenMesh(pObject, &pChosenMesh, iThisMeshIndex);

	// if not read only, set material from 'edit_grideleprof'
	if ( (mode == 0 || mode == 3) && pChosenMesh) 
	{
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pChosenMesh->wickedmeshindex);
		if (mesh)
		{
			// get material settings from mesh material or WEMaterial
			uint64_t materialEntity = mesh->subsets[0].materialID;
			wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

			// base color
			if (edit_grideleprof->WEMaterial.dwBaseColor[iSelectedMesh] == -1)
			{
				BaseColor[0] = pObjectMaterial->baseColor.x;
				BaseColor[1] = pObjectMaterial->baseColor.y;
				BaseColor[2] = pObjectMaterial->baseColor.z;
				BaseColor[3] = pObjectMaterial->baseColor.w;
			}
			else 
			{
				BaseColor[0] = ((edit_grideleprof->WEMaterial.dwBaseColor[iSelectedMesh] & 0xff000000) >> 24) / 255.0f;
				BaseColor[1] = ((edit_grideleprof->WEMaterial.dwBaseColor[iSelectedMesh] & 0x00ff0000) >> 16) / 255.0f;
				BaseColor[2] = ((edit_grideleprof->WEMaterial.dwBaseColor[iSelectedMesh] & 0x0000ff00) >> 8) / 255.0f;
				BaseColor[3] = (edit_grideleprof->WEMaterial.dwBaseColor[iSelectedMesh] & 0x000000ff) / 255.0f;
			}
			pObjectMaterial->baseColor.x = BaseColor[0];
			pObjectMaterial->baseColor.y = BaseColor[1];
			pObjectMaterial->baseColor.z = BaseColor[2];
			pObjectMaterial->baseColor.w = BaseColor[3];

			bool bValid = true;
			if (pObjectMaterial->customShaderID == 5 && (mesh->IsDoubleSided() || pestrcasestr(edit_grideleprof->WEMaterial.baseColorMapName[0].Get(), "eyeglasses") ) ) //|| pObjectMaterial->GetBlendMode() == BLENDMODE_ALPHA
			{
				pObjectMaterial->customShaderID = -1;
				edit_grideleprof->WEMaterial.customShaderID = -1;
				bValid = false;
			}
			if (bValid)
			{
				pObjectMaterial->customShaderID = edit_grideleprof->WEMaterial.customShaderID;
				pObjectMaterial->customShaderParam1 = edit_grideleprof->WEMaterial.customShaderParam1;
				pObjectMaterial->customShaderParam2 = edit_grideleprof->WEMaterial.customShaderParam2;
				pObjectMaterial->customShaderParam3 = edit_grideleprof->WEMaterial.customShaderParam3;
				pObjectMaterial->customShaderParam4 = edit_grideleprof->WEMaterial.customShaderParam4;
				pObjectMaterial->customShaderParam5 = edit_grideleprof->WEMaterial.customShaderParam5;
				pObjectMaterial->customShaderParam6 = edit_grideleprof->WEMaterial.customShaderParam6;
				pObjectMaterial->customShaderParam7 = edit_grideleprof->WEMaterial.customShaderParam7;
			}
			// emissive color
			if (edit_grideleprof->WEMaterial.dwEmmisiveColor[iSelectedMesh] == -1)
			{
				EmmisiveColor[0] = pObjectMaterial->emissiveColor.x;
				EmmisiveColor[1] = pObjectMaterial->emissiveColor.y;
				EmmisiveColor[2] = pObjectMaterial->emissiveColor.z;
				EmmisiveColor[3] = pObjectMaterial->emissiveColor.w;
			}
			else
			{
				EmmisiveColor[0] = ((edit_grideleprof->WEMaterial.dwEmmisiveColor[iSelectedMesh] & 0xff000000) >> 24) / 255.0f;
				EmmisiveColor[1] = ((edit_grideleprof->WEMaterial.dwEmmisiveColor[iSelectedMesh] & 0x00ff0000) >> 16) / 255.0f;
				EmmisiveColor[2] = ((edit_grideleprof->WEMaterial.dwEmmisiveColor[iSelectedMesh] & 0x0000ff00) >> 8) / 255.0f;
				EmmisiveColor[3] = (edit_grideleprof->WEMaterial.dwEmmisiveColor[iSelectedMesh] & 0x000000ff) / 255.0f;
			}
			pObjectMaterial->emissiveColor.x = EmmisiveColor[0];
			pObjectMaterial->emissiveColor.y = EmmisiveColor[1];
			pObjectMaterial->emissiveColor.z = EmmisiveColor[2];
			pObjectMaterial->emissiveColor.w = EmmisiveColor[3];

			// update DBO mesh copy of base and emissive colors (?)
			pChosenMesh->mMaterial.Diffuse.r = pObjectMaterial->baseColor.x;
			pChosenMesh->mMaterial.Diffuse.g = pObjectMaterial->baseColor.y;
			pChosenMesh->mMaterial.Diffuse.b = pObjectMaterial->baseColor.z;
			pChosenMesh->mMaterial.Diffuse.a = pObjectMaterial->baseColor.w;
			pChosenMesh->mMaterial.Emissive.r = pObjectMaterial->emissiveColor.x;
			pChosenMesh->mMaterial.Emissive.g = pObjectMaterial->emissiveColor.y;
			pChosenMesh->mMaterial.Emissive.b = pObjectMaterial->emissiveColor.z;
			pChosenMesh->mMaterial.Emissive.a = pObjectMaterial->emissiveColor.w;

			// control values
			pObjectMaterial->SetAlphaRef(edit_grideleprof->WEMaterial.fAlphaRef[iSelectedMesh]);
			pObjectMaterial->SetNormalMapStrength(edit_grideleprof->WEMaterial.fNormal[iSelectedMesh]);
			pObjectMaterial->SetEmissiveStrength(edit_grideleprof->WEMaterial.fEmissive[iSelectedMesh]);
			pObjectMaterial->SetRoughness(edit_grideleprof->WEMaterial.fRoughness[iSelectedMesh]);
			pObjectMaterial->SetMetalness(edit_grideleprof->WEMaterial.fMetallness[iSelectedMesh]);

			// transparency
			bTransparent = edit_grideleprof->WEMaterial.bTransparency[iSelectedMesh];
			if (edit_grideleprof->blendmode > 0)
			{
				pObjectMaterial->userBlendMode = (BLENDMODE) edit_grideleprof->blendmode;
			}
			else if (bTransparent)
			{
				pObjectMaterial->userBlendMode = BLENDMODE_ALPHA;
			}
			else 
			{
				pObjectMaterial->userBlendMode = BLENDMODE_OPAQUE;
			}

			// cast shadows
			bCastShadows = edit_grideleprof->WEMaterial.bCastShadows[iSelectedMesh];
			if (bCastShadows) 
			{
				pObjectMaterial->SetCastShadow(true);
			}
			else 
			{
				pObjectMaterial->SetCastShadow(false);
			}

			// planar reflections (buggy?)
			bPlanerReflection = edit_grideleprof->WEMaterial.bPlanerReflection[iSelectedMesh];
			if (bPlanerReflection)
			{
				pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION;
			}
			else
			{
				if (pObjectMaterial->parallaxOcclusionMapping > 0.0f)
					pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING;
				else
					pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_PBR;
			}

			// double sided
			bDoubleSided = edit_grideleprof->WEMaterial.bDoubleSided[iSelectedMesh];
			if (bDoubleSided) 
				mesh->SetDoubleSided(true);
			else
				mesh->SetDoubleSided(false);

			// render order bias
			float fRenderOrderBias = edit_grideleprof->WEMaterial.fRenderOrderBias[iSelectedMesh];
			WickedCall_SetRenderOrderBias(pChosenMesh, fRenderOrderBias);

			// reflectance
			fReflectance = edit_grideleprof->WEMaterial.fReflectance[iSelectedMesh];

			// reset base and emissive color triggers
			dwBaseColor = -1;
			dwEmmisiveColor = -1;

			// material changed, update it
			pObjectMaterial->SetReflectance(fReflectance);
			pObjectMaterial->SetDirty();
		}
	}
	else 
	{
		// default settings 
		BaseColor[0] = 1.0f;
		BaseColor[1] = 1.0f;
		BaseColor[2] = 1.0f;
		BaseColor[3] = 1.0f;
		EmmisiveColor[0] = 0.0;
		EmmisiveColor[1] = 0.0;
		EmmisiveColor[2] = 0.0;
		EmmisiveColor[3] = 0.0;
		bTransparent = false;
		bDoubleSided = false;
		fRenderOrderBias = 0.0f;
		bPlanerReflection = false;
		bCastShadows = true;
		/*fReflectance = 0.002;*/
		fReflectance = 0.04;
		dwBaseColor = -1;
		dwEmmisiveColor = -1;
	}
}

void Wicked_Set_Material_From_grideleprof(void* pVObject, int mode, entityeleproftype *edit_grideleprof)
{
	Wicked_Set_Material_From_grideleprof_ThisMesh (pVObject, mode, edit_grideleprof, -1 );
}

void Wicked_Set_Material_Defaults(void* pVObject, int mode)
{
	// get object ptr
	sObject* pObject = (sObject*)pVObject;
	if (!pObject) return;

	// find first mesh
	sMesh* pChosenMesh = NULL;
	Wicked_FindChosenMesh(pObject, &pChosenMesh, -1);

	// if not read only, set material default for UI to edit
	if ((mode == 0 || mode == 3) && pChosenMesh) 
	{
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pChosenMesh->wickedmeshindex);
		if (mesh)
		{
			// get material from mesh
			uint64_t materialEntity = mesh->subsets[0].materialID;
			wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

			BaseColor[0] = pObjectMaterial->baseColor.x;
			BaseColor[1] = pObjectMaterial->baseColor.y;
			BaseColor[2] = pObjectMaterial->baseColor.z;
			BaseColor[3] = pObjectMaterial->baseColor.w;
			EmmisiveColor[0] = pObjectMaterial->emissiveColor.x;
			EmmisiveColor[1] = pObjectMaterial->emissiveColor.y;
			EmmisiveColor[2] = pObjectMaterial->emissiveColor.z;
			EmmisiveColor[3] = pObjectMaterial->emissiveColor.w;

			BLENDMODE bm = pObjectMaterial->GetBlendMode();
			if (bm == BLENDMODE_ALPHA)
				bTransparent = true;
			else
				bTransparent = false;

			bDoubleSided = false;
			fRenderOrderBias = 0.0f;
			bPlanerReflection = pObjectMaterial->HasPlanarReflection();
			bCastShadows = pObjectMaterial->IsCastingShadow();
			fReflectance = pObjectMaterial->reflectance;

			dwBaseColor = -1;
			dwEmmisiveColor = -1;
		}
	}
	else
	{
		// set defaults
		BaseColor[0] = 1.0f;
		BaseColor[1] = 1.0f;
		BaseColor[2] = 1.0f;
		BaseColor[3] = 1.0f;
		EmmisiveColor[0] = 0.0;
		EmmisiveColor[1] = 0.0;
		EmmisiveColor[2] = 0.0;
		EmmisiveColor[3] = 0.0;
		bTransparent = false;
		bDoubleSided = false;
		fRenderOrderBias = 0.0f;
		bPlanerReflection = false;
		bCastShadows = true;
		dwBaseColor = -1;
		dwEmmisiveColor = -1;
		/*fReflectance = 0.002;*/
		//ZJ: 0.04 is the most common setting for reflectance in PBR
		fReflectance = 0.04;
	}
}

void Wicked_Update_All_Materials(void* pVObject, int mode)
{
}

