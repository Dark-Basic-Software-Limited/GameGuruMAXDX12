void Wicked_Copy_Material_To_Grideleprof(void* pVObject, int mode, entityeleproftype *edit_grideleprof)
{
	// get object ptr and grideleprof
	sObject* pObject = (sObject*)pVObject;
	if (!pObject) return;
	if (!edit_grideleprof)
	{
		edit_grideleprof = &t.grideleprof;
	}

	int orgShaderID = -1;
	// go through all meshes and update eleprof
	for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
	{
		sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
		if (pMesh && pMesh->wickedmeshindex > 0)
		{
			// copy material data to grideleprof structure
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
			if (mesh)
			{
				// get material from mesh
				uint64_t materialEntity = mesh->subsets[0].materialID;
				wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				if (orgShaderID == -1 && pObjectMaterial->customShaderID >= 0)
				{
					orgShaderID = pObjectMaterial->customShaderID;
				}
				// for each mesh texture set
				if (iMeshIndex < MAXMESHMATERIALS)
				{
					// ensure DBO meshes match wicked materials (later applied with Wicked_SetMeshMaterial)
					pMesh->mMaterial.Diffuse.r = pObjectMaterial->baseColor.x;
					pMesh->mMaterial.Diffuse.g = pObjectMaterial->baseColor.y;
					pMesh->mMaterial.Diffuse.b = pObjectMaterial->baseColor.z;
					pMesh->mMaterial.Diffuse.a = pObjectMaterial->baseColor.w;
					pMesh->mMaterial.Emissive.r = pObjectMaterial->emissiveColor.x;
					pMesh->mMaterial.Emissive.g = pObjectMaterial->emissiveColor.y;
					pMesh->mMaterial.Emissive.b = pObjectMaterial->emissiveColor.z;
					pMesh->mMaterial.Emissive.a = pObjectMaterial->emissiveColor.w;

					// copy wicked material for this mesh to WEMaterial
					edit_grideleprof->WEMaterial.baseColorMapName[iMeshIndex] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str());
					edit_grideleprof->WEMaterial.normalMapName[iMeshIndex] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].name.c_str());
					edit_grideleprof->WEMaterial.surfaceMapName[iMeshIndex] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str());
					edit_grideleprof->WEMaterial.displacementMapName[iMeshIndex] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name.c_str());
					edit_grideleprof->WEMaterial.emissiveMapName[iMeshIndex] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].name.c_str());
					#ifndef DISABLEOCCLUSIONMAP
					edit_grideleprof->WEMaterial.occlusionMapName[iMeshIndex] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name.c_str());
					#endif
					edit_grideleprof->WEMaterial.fNormal[iMeshIndex] = pObjectMaterial->normalMapStrength;
					edit_grideleprof->WEMaterial.fRoughness[iMeshIndex] = pObjectMaterial->roughness;
					edit_grideleprof->WEMaterial.fMetallness[iMeshIndex] = pObjectMaterial->metalness;
					edit_grideleprof->WEMaterial.fEmissive[iMeshIndex] = pObjectMaterial->GetEmissiveStrength();
					edit_grideleprof->WEMaterial.fAlphaRef[iMeshIndex] = pObjectMaterial->alphaRef;
					edit_grideleprof->WEMaterial.bCastShadows[iMeshIndex] = pObjectMaterial->IsCastingShadow();
					edit_grideleprof->WEMaterial.bDoubleSided[iMeshIndex] = mesh->IsDoubleSided();
					edit_grideleprof->WEMaterial.fRenderOrderBias[iMeshIndex] = 0.0f;
					bool bValid = true;
					
					//pObjectMaterial->customShaderID == 5
					if (orgShaderID == 5 && (mesh->IsDoubleSided() || pestrcasestr(edit_grideleprof->WEMaterial.baseColorMapName[iMeshIndex].Get(),"eyeglasses")) ) //|| pObjectMaterial->GetBlendMode() == BLENDMODE_ALPHA)
					{
						bValid = false;
					}
					if (bValid)
					{
						edit_grideleprof->WEMaterial.customShaderID = pObjectMaterial->customShaderID;
						edit_grideleprof->WEMaterial.customShaderParam1 = pObjectMaterial->customShaderParam1;
						edit_grideleprof->WEMaterial.customShaderParam2 = pObjectMaterial->customShaderParam2;
						edit_grideleprof->WEMaterial.customShaderParam3 = pObjectMaterial->customShaderParam3;
						edit_grideleprof->WEMaterial.customShaderParam4 = pObjectMaterial->customShaderParam4;
						edit_grideleprof->WEMaterial.customShaderParam5 = pObjectMaterial->customShaderParam5;
						edit_grideleprof->WEMaterial.customShaderParam6 = pObjectMaterial->customShaderParam6;
						edit_grideleprof->WEMaterial.customShaderParam7 = pObjectMaterial->customShaderParam7;
					}
					sFrame* pFrame = pMesh->pFrameAttachedTo;
					if (pFrame)
					{
						wiScene::ObjectComponent* object = wiScene::GetScene().objects.GetComponent(pFrame->wickedobjindex);
						if (object)
						{
							fRenderOrderBias = object->GetRenderOrderBiasDistance();
							edit_grideleprof->WEMaterial.fRenderOrderBias[iMeshIndex] = fRenderOrderBias;
						}
					}
					edit_grideleprof->WEMaterial.bPlanerReflection[iMeshIndex] = pObjectMaterial->HasPlanarReflection();
					if (pObjectMaterial->GetBlendMode() == BLENDMODE_ALPHA)
						edit_grideleprof->WEMaterial.bTransparency[iMeshIndex] = true;
					else
						edit_grideleprof->WEMaterial.bTransparency[iMeshIndex] = false;
					edit_grideleprof->WEMaterial.fReflectance[iMeshIndex] = pObjectMaterial->reflectance;
					edit_grideleprof->WEMaterial.dwBaseColor[iMeshIndex] = ((unsigned int)(pObjectMaterial->baseColor.x * 255) << 24);
					edit_grideleprof->WEMaterial.dwBaseColor[iMeshIndex] += ((unsigned int)(pObjectMaterial->baseColor.y * 255) << 16);
					edit_grideleprof->WEMaterial.dwBaseColor[iMeshIndex] += ((unsigned int)(pObjectMaterial->baseColor.z * 255) << 8);
					edit_grideleprof->WEMaterial.dwBaseColor[iMeshIndex] += ((unsigned int)(pObjectMaterial->baseColor.w * 255));
					edit_grideleprof->WEMaterial.dwEmmisiveColor[iMeshIndex] = ((unsigned int)(pObjectMaterial->emissiveColor.x * 255) << 24);
					edit_grideleprof->WEMaterial.dwEmmisiveColor[iMeshIndex] += ((unsigned int)(pObjectMaterial->emissiveColor.y * 255) << 16);
					edit_grideleprof->WEMaterial.dwEmmisiveColor[iMeshIndex] += ((unsigned int)(pObjectMaterial->emissiveColor.z * 255) << 8);
					if (pObjectMaterial->emissiveColor.w <= 1.0)
						edit_grideleprof->WEMaterial.dwEmmisiveColor[iMeshIndex] += ((unsigned int)(pObjectMaterial->emissiveColor.w * 255));
					else
						edit_grideleprof->WEMaterial.dwEmmisiveColor[iMeshIndex] += 255;
				}
			}
		}
	}
}

void Wicked_Copy_JustTextureNames_To_Grideleprof(void* pVObject, int mode)
{
	// get object ptr
	sObject* pObject = (sObject*)pVObject;
	if (!pObject) return;

	// find first mesh
	sMesh* pChosenMesh = NULL;
	Wicked_FindChosenMesh(pObject, &pChosenMesh, -1);

	// copy texture names into mesh chosen
	if (pChosenMesh) 
	{
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pChosenMesh->wickedmeshindex);
		if (mesh)
		{
			uint64_t materialEntity = mesh->subsets[0].materialID;
			wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
			t.grideleprof.WEMaterial.baseColorMapName[iSelectedMesh] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str());
			t.grideleprof.WEMaterial.normalMapName[iSelectedMesh] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].name.c_str());
			t.grideleprof.WEMaterial.surfaceMapName[iSelectedMesh] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str());
			t.grideleprof.WEMaterial.displacementMapName[iSelectedMesh] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name.c_str());
			t.grideleprof.WEMaterial.emissiveMapName[iSelectedMesh] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].name.c_str());
			#ifndef DISABLEOCCLUSIONMAP
			t.grideleprof.WEMaterial.occlusionMapName[iSelectedMesh] = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name.c_str());
			#endif
		}
	}
}

bool LocateTexture(char *name)
{
	//PE: Need to search for another names here.
	if (FileExist(name) == 1)
		return true;
	return false;
}

// Loop through all the meshes in the imported object and change material settings.
void importer_set_all_material_settings(int slot, float value)
{
	if (!t.importer.bEditAllMesh)
	{
		return;
	}

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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			if (meshComponent)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = meshComponent->subsets[0].materialID;
				wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

				// Switch statement values match the texslot switch values from Wicked_Change_Object_Material()
				switch (slot)
				{
				case 0:
				{
					pMeshMaterial->SetAlphaRef(value);
					break;
				}
				case 1:
				{
					pMeshMaterial->SetNormalMapStrength(value);
					break;
				}
				case 2:
				{
					pMeshMaterial->SetRoughness(value);
					break;
				}
				case 3:
				{
					pMeshMaterial->SetMetalness(value);
					break;
				}
				case 4:
				{
					// Unused.
					break;
				}
				case 5:
				{
					pMeshMaterial->SetEmissiveStrength(value);
					break;
				}
				case 6:
				{
					pMeshMaterial->SetReflectance(value);
				}
				case 7:
				{
					WickedCall_SetRenderOrderBias(mesh, value);
				}
				default:
					break;
				}
				
				pMeshMaterial->SetDirty();
			}
		}
	}
}

void importer_set_all_material_colour(int slot, float values[4])
{
	if (!t.importer.bEditAllMesh)
	{
		return;
	}

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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			if (meshComponent)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = meshComponent->subsets[0].materialID;
				wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

				if (slot == 0)
				{
					pMeshMaterial->SetBaseColor(XMFLOAT4(&values[0]));
				}
				else if (slot == 5)
				{
					pMeshMaterial->SetEmissiveColor(XMFLOAT4(&values[0]));
				}
			}
		}
	}
}

void importer_set_all_material_transparent(bool bIsTransparent)
{
	if (!t.importer.bEditAllMesh)
	{
		return;
	}

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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			if (meshComponent)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = meshComponent->subsets[0].materialID;
				wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

				if (bIsTransparent)
				{
					pMeshMaterial->userBlendMode = BLENDMODE_ALPHA;
				}
				else
				{
					pMeshMaterial->userBlendMode = BLENDMODE_OPAQUE;
				}
			}
		}
	}
}

void importer_set_all_mesh_double_sided(bool bIsDoubleSided)
{
	if (!t.importer.bEditAllMesh)
	{
		return;
	}

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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			meshComponent->SetDoubleSided(bIsDoubleSided);
		}
	}
}

void importer_set_all_material_planar_reflection(bool planarReflection)
{
	if (!t.importer.bEditAllMesh)
	{
		return;
	}

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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			if (meshComponent)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = meshComponent->subsets[0].materialID;
				wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

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
			}
		}
	}
}

void importer_set_all_material_shader_id(int shaderID,float p1, float p2, float p3, float p4, float p5, float p6, float p7)
{
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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			if (meshComponent)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = meshComponent->subsets[0].materialID;
				wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				bool bValid = true;
				if (shaderID == 5 && meshComponent->IsDoubleSided())
				{
					pMeshMaterial->customShaderID = -1;
					bValid = false;
				}
				if (bValid)
				{
					pMeshMaterial->customShaderID = shaderID;
					pMeshMaterial->customShaderParam1 = p1;
					pMeshMaterial->customShaderParam2 = p2;
					pMeshMaterial->customShaderParam3 = p3;
					pMeshMaterial->customShaderParam4 = p4;
					pMeshMaterial->customShaderParam5 = p5;
					pMeshMaterial->customShaderParam6 = p6;
					pMeshMaterial->customShaderParam7 = p7;
					pMeshMaterial->SetDirty();
				}
			}
		}
	}
}

void importer_set_all_material_shader_id(int obj, int shaderID, float p1, float p2, float p3, float p4, float p5, float p6, float p7)
{
	int objectnumber = t.importer.objectnumber;
	int active = t.importer.importerActive;
	t.importer.objectnumber = obj;
	t.importer.importerActive = 1;
	importer_set_all_material_shader_id(shaderID, p1, p2, p3, p4, p5, p6, p7);
	t.importer.importerActive = active;
	t.importer.objectnumber = objectnumber;
}


void importer_set_all_material_cast_shadow(bool bCastShadow)
{
	if (!t.importer.bEditAllMesh)
	{
		return;
	}

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

	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* mesh = pObject->ppMeshList[i];

		if (mesh)
		{
			wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

			if (meshComponent)
			{
				// get material settings from mesh material or WEMaterial
				uint64_t materialEntity = meshComponent->subsets[0].materialID;
				wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);

				pMeshMaterial->SetCastShadow(bCastShadow);
			}
		}
	}
}

// Set the Y import offset so that the bottom of the objects bounding box is the same height as the floor.
void importer_find_floor(void)
{
	sObject* pObject = GetObjectData(t.importer.objectnumber);

	if (bFindFloor)
	{
		fImportPosY = 0.0f;
		fImportPosY += (fImportPosY - pObject->collision.vecMin.y);
		t.importer.objectFPE.offy = fImportPosY;
		importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);
	}
}

void importer_delete_old_surface_files()
{
	cstr pOriginalDir = GetDir();
	
	const char* pWritePath = GG_GetWritePath();
	char pWriteDirectory[MAX_PATH];
	strcpy(pWriteDirectory, pWritePath);
	cstr pPath(pWriteDirectory);
	pPath += "imported_models\\";

	for (int i = 0; i < t.importer.pSurfaceFilesToDelete.size(); i++)
	{
		// Only want to delete files from the writable area, so get the filename only and we'll check the writable folder only.
		cstr pFilenameOnly = importer_getfilenameonly(t.importer.pSurfaceFilesToDelete[i].Get());
		cstr pFileToDelete = pPath + pFilenameOnly;
		if (FileExist(pFileToDelete.Get()) == 1)
		{
			DeleteAFile(pFileToDelete.Get());
		}
	}

	// imported models folder only needed as a temporary location to store files.
	extern BOOL DB_DeleteDir(char* Dirname);
	DB_DeleteDir(pPath.Get()); // Will only delete the folder if it is empty  (if there were any files in there before this importer session, they will be left untouched.)

	SetDir(pOriginalDir.Get());
}

void importer_storeobjectdata()
{
	// Copy all of the imported object data, so it can be restored if the user goes back after import.
	strcpy(g_Data.cImportPath, t.importer.objectFileOriginalPath.Get());
	strcpy(g_Data.cName, cImportName);
	g_Data.iScaleMode = t.importer.lastscalingmodeused;
	g_Data.iCentreMeshData = t.importer.centermodelbyshiftingmesh;
	g_Data.iScale = iImporterScale;
	g_Data.iRotationOffset[0] = fImportRotX; g_Data.iRotationOffset[1] = fImportRotY; g_Data.iRotationOffset[2] = fImportRotZ;
	g_Data.iPositionOffset[0] = fImportPosX; g_Data.iPositionOffset[1] = fImportPosY; g_Data.iPositionOffset[2] = fImportPosZ;
	g_Data.iFindFloor = bFindFloor;
	g_Data.iCollisionShape = t.importer.collisionshape;
	g_Data.iStatic = t.importer.defaultstatic;
	g_Data.iMaterialType = t.slidersmenuvalue[t.importer.properties1Index][10].value;

	sObject* pObject = nullptr;
	pObject = GetObjectData(t.importer.objectnumber);
	if (pObject)
	{
		int iMeshCount = pObject->iMeshCount;
		g_Data.baseColours.resize(iMeshCount);
		g_Data.albedoFiles.resize(iMeshCount);
		g_Data.normalFiles.resize(iMeshCount);
		g_Data.surfaceFiles.resize(iMeshCount);
		g_Data.emissiveFiles.resize(iMeshCount);
		g_Data.emissiveColours.resize(iMeshCount);
		g_Data.reflectance.resize(iMeshCount);
		g_Data.renderBias.resize(iMeshCount);
		g_Data.transparent.resize(iMeshCount);
		g_Data.doubleSided.resize(iMeshCount);
		g_Data.planarReflection.resize(iMeshCount);
		g_Data.castShadows.resize(iMeshCount);
		g_Data.animSlots.resize(g_pAnimSlotList.size());

		for (int i = 0; i < pObject->iMeshCount; i++)
		{
			sMesh* mesh = pObject->ppMeshList[i];
			if (mesh)
			{
				wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);
				if (meshComponent)
				{
					uint64_t materialEntity = meshComponent->subsets[0].materialID;
					wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
					if (pMeshMaterial)
					{
						std::array<float, 4>& baseColour = g_Data.baseColours[i];
						baseColour[0] = pMeshMaterial->baseColor.x;
						baseColour[1] = pMeshMaterial->baseColor.y;
						baseColour[2] = pMeshMaterial->baseColor.z;
						baseColour[3] = pMeshMaterial->baseColor.w;

						std::array<char, MAX_PATH>& albedofile = g_Data.albedoFiles[i];
						strcpy(&albedofile[0], pMeshMaterial->textures[BASECOLORMAP].name.c_str());

						std::array<char, MAX_PATH>& normalfile = g_Data.normalFiles[i];
						strcpy(&normalfile[0], pMeshMaterial->textures[NORMALMAP].name.c_str());

						std::array<char, MAX_PATH>& surfacefile = g_Data.surfaceFiles[i];
						strcpy(&surfacefile[0], pMeshMaterial->textures[SURFACEMAP].name.c_str());

						std::array<char, MAX_PATH>& emissivefile = g_Data.emissiveFiles[i];
						strcpy(&emissivefile[0], pMeshMaterial->textures[EMISSIVEMAP].name.c_str());

						std::array<float, 4>& emissiveColour = g_Data.emissiveColours[i];
						emissiveColour[0] = pMeshMaterial->emissiveColor.x;
						emissiveColour[1] = pMeshMaterial->emissiveColor.y;
						emissiveColour[2] = pMeshMaterial->emissiveColor.z;
						emissiveColour[3] = pMeshMaterial->emissiveColor.w;

						g_Data.reflectance[i] = pMeshMaterial->reflectance;
						g_Data.renderBias[i] = WickedCall_GetRenderOrderBias(mesh);
						g_Data.transparent[i] = pMeshMaterial->userBlendMode;
						g_Data.doubleSided[i] = meshComponent->IsDoubleSided();
						g_Data.planarReflection[i] = pMeshMaterial->shaderType;
						g_Data.castShadows[i] = pMeshMaterial->IsCastingShadow();

						if(g_Data.animSlots.size() > 0)
							memcpy(&g_Data.animSlots[0], &g_pAnimSlotList[0], sizeof(sAnimSlotStruct) * g_Data.animSlots.size());
					}
				}
			}
		}
	}
}

void importer_restoreobjectdata()
{
	// common to refresh load and batch load
	t.importer.lastscalingmodeused = g_Data.iScaleMode;
	t.importer.centermodelbyshiftingmesh = g_Data.iCentreMeshData;
	iImporterScale = g_Data.iScale;
	fImportRotX = g_Data.iRotationOffset[0]; fImportRotY = g_Data.iRotationOffset[1]; fImportRotZ = g_Data.iRotationOffset[2];
	fImportPosX = g_Data.iPositionOffset[0]; fImportPosY = g_Data.iPositionOffset[1]; fImportPosZ = g_Data.iPositionOffset[2];
	bFindFloor = g_Data.iFindFloor;
	t.importer.collisionshape = g_Data.iCollisionShape;
	t.importer.defaultstatic = g_Data.iStatic;
	t.slidersmenuvalue[t.importer.properties1Index][10].value = g_Data.iMaterialType;

	// Copy all of the imported object data, so it can be restored if the user goes back after import.
	if (bBatchConverting == true)
	{
		// batch only uses certain settings, and non-model related!
	}
	else
	{
		// refresh load
		strcpy(cImportName, g_Data.cName);
		sObject* pObject = nullptr;
		pObject = GetObjectData(t.importer.objectnumber);
		if (pObject)
		{
			int iMeshCount = pObject->iMeshCount;

			for (int i = 0; i < iMeshCount; i++)
			{
				sMesh* mesh = pObject->ppMeshList[i];
				if (mesh)
				{
					wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);
					if (meshComponent)
					{
						uint64_t materialEntity = meshComponent->subsets[0].materialID;
						wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
						if (pMeshMaterial)
						{
							std::array<float, 4>& baseColour = g_Data.baseColours[i];
							pMeshMaterial->baseColor = XMFLOAT4(baseColour[0], baseColour[1], baseColour[2], baseColour[3]);

							std::array<char, MAX_PATH>& albedofile = g_Data.albedoFiles[i];
							pMeshMaterial->textures[BASECOLORMAP].name = std::string(&albedofile[0]);

							std::array<char, MAX_PATH>& normalfile = g_Data.normalFiles[i];
							pMeshMaterial->textures[NORMALMAP].name = std::string(&normalfile[0]);

							std::array<char, MAX_PATH>& surfacefile = g_Data.surfaceFiles[i];
							pMeshMaterial->textures[SURFACEMAP].name = std::string(&surfacefile[0]);

							std::array<char, MAX_PATH>& emissivefile = g_Data.emissiveFiles[i];
							pMeshMaterial->textures[EMISSIVEMAP].name = std::string(&emissivefile[0]);

							std::array<float, 4>& emissiveColour = g_Data.emissiveColours[i];
							pMeshMaterial->emissiveColor = XMFLOAT4(emissiveColour[0], emissiveColour[1], emissiveColour[2], emissiveColour[3]);

							pMeshMaterial->reflectance = g_Data.reflectance[i];
							WickedCall_SetRenderOrderBias(mesh, g_Data.renderBias[i]);
							pMeshMaterial->userBlendMode = (BLENDMODE)g_Data.transparent[i];
							meshComponent->SetDoubleSided(g_Data.doubleSided[i]);
							pMeshMaterial->shaderType = (wiScene::MaterialComponent::SHADERTYPE)g_Data.planarReflection[i];
							pMeshMaterial->SetCastShadow(g_Data.castShadows[i]);

							g_pAnimSlotList.resize(g_Data.animSlots.size());
							if (g_Data.animSlots.size() > 0)
								memcpy(&g_pAnimSlotList[0], &g_Data.animSlots[0], sizeof(sAnimSlotStruct) * g_Data.animSlots.size());

							if (i == 0)
							{
								pSelectedMaterial = pMeshMaterial;
								pSelectedMesh = mesh;
							}

							pMeshMaterial->SetDirty();
						}
					}
				}
			}

			importer_applyframezerooffsets(t.importer.objectnumber, fImportPosX, fImportPosY, fImportPosZ, fImportRotX, fImportRotY, fImportRotZ);
			char* pCollisionShapes[10] = { "Box","Polygon","Sphere","Cylinder","Convex Hull","Character Collision","Tree Collision","No Collision", "Hull Decomp", "Collision Mesh" };
			strcpy(collision_combo_entry, pCollisionShapes[t.importer.collisionshape]);
		}
	}
}

void importer_clearobjectdata()
{
	g_Data.cImportPath[0] = 0;
	g_Data.cName[0] = 0;
	g_Data.iScaleMode = 0;
	g_Data.iCentreMeshData = 0;
	g_Data.iScale = 100;
	g_Data.iRotationOffset[0] = 0; g_Data.iRotationOffset[1] = 0; g_Data.iRotationOffset[2] = 0;
	g_Data.iPositionOffset[0] = 0; g_Data.iPositionOffset[1] = 0; g_Data.iPositionOffset[2] = 0;
	g_Data.iFindFloor = 0;
	g_Data.iCollisionShape = 0;
	g_Data.iStatic = 0;
	g_Data.iMaterialType = 0;
	g_Data.baseColours.clear();
	g_Data.albedoFiles.clear();
	g_Data.normalFiles.clear();
	g_Data.surfaceFiles.clear();
	g_Data.emissiveFiles.clear();
	g_Data.emissiveColours.clear();
	g_Data.reflectance.clear();
	g_Data.renderBias.clear();
	g_Data.transparent.clear();
	g_Data.doubleSided.clear();
	g_Data.planarReflection.clear();
	g_Data.castShadows.clear();
	g_Data.animSlots.clear();
}

// Select a texture file, if currentfilename is specified then it will open the file selector wherever it is located.
char* importer_selectfile(int texslot, std::string currentfilename, bool bPresetExplorer)
{
	// Want to always open where the model is when importing.
	if(t.importer.importerActive)
		bPresetExplorer = false;

	bool bOpenCustomLocation = true;
	std::string texPath = currentfilename;

	if (texPath.length() > 0)
	{
		if (!strstr(texPath.c_str(), ":"))
		{
			// If using relative paths then make sure we get the full path to the write folder.
			std::string fullPath = std::string(GG_GetWritePath()) + std::string("Files\\") + texPath;
			if (!FileExist((LPSTR)fullPath.c_str()))
			{
				// File was not in the write folder, check the install location.
				fullPath = std::string(GetDir()) + std::string("\\") + texPath;
				if (!FileExist((LPSTR)fullPath.c_str()))
					bOpenCustomLocation = false;
			}
			texPath = fullPath;
		}

		// Found an existing file that is being replaced, check if we should open the file explorer there.
		if (bOpenCustomLocation)
			bOpenCustomLocation = bPresetExplorer;

		if (bOpenCustomLocation)
		{
			// Determine the path to the currently applied texture so we can start the file selector there.
			int count = 0;
			for (std::string::reverse_iterator reverseIt = texPath.rbegin(); reverseIt != texPath.rend(); ++reverseIt)
			{
				if (*reverseIt == '\\' || *reverseIt == '/')
					break;

				count++;
			}

			texPath.resize(texPath.length() - count);
		}
	}
	//PE: Filepath was changed here, caused crash.
	cStr tOldDir = GetDir();
	char * selectfile = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0DDS\0*.dds\0PNG\0*.png\0JPEG\0*.jpg\0TGA\0*.tga\0BMP\0*.bmp\0\0\0", texPath.c_str(), NULL, bOpenCustomLocation);
	SetDir(tOldDir.Get());
	return selectfile;
}

void importer_collectmeshname(char* meshName)
{
	return;
	g_MeshNamesAssimp.push_back(cstr(meshName));
}

// Used by ASSIMP to check if it should allow import of a model with no meshes (animation data without skin)
bool importer_havevalidobject()
{
	sObject* pObject = GetObjectData(t.importer.objectnumber);
	if (pObject)
		return true;
	else
		return false;
}
