void importer_apply_fpe ( void )
{
	//  Scale
	t.slidersmenuvalue[t.importer.properties1Index][1].value = ValF(t.importer.objectFPE.scale.Get());
	t.importer.oldScale = t.slidersmenuvalue[t.importer.properties1Index][1].value;

	//  angle
	t.importer.objectAngleY = ValF(t.importer.objectFPE.roty.Get());
	t.slidersmenuvalue[t.importer.properties1Index][3].value = t.importer.objectAngleY;
	YRotateObject (  t.importer.objectnumber,t.importer.objectAngleY );

	//  Collision Mode
	if (  t.importer.objectFPE.collisionmode  ==  "0" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][4].value_s = "Box";
		t.slidersmenuvalue[t.importer.properties1Index][4].value=1;
	}
	if (  t.importer.objectFPE.collisionmode  ==  "1" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][4].value_s = "Polygon";
		t.slidersmenuvalue[t.importer.properties1Index][4].value=2;
	}
	if (  t.importer.objectFPE.collisionmode  ==  "99" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][4].value_s = "No Collision";
		t.slidersmenuvalue[t.importer.properties1Index][4].value=3;
	}
	if (  t.importer.objectFPE.collisionmode  ==  "1001" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][4].value_s = "Limb One Box";
		t.slidersmenuvalue[t.importer.properties1Index][4].value=4;
	}
	if (  t.importer.objectFPE.collisionmode  ==  "2001" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][4].value_s = "Limb One Poly";
		t.slidersmenuvalue[t.importer.properties1Index][4].value=5;
	}
	if (  t.importer.objectFPE.collisionmode  ==  "40" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][4].value_s = "Collision Boxes";
		t.slidersmenuvalue[t.importer.properties1Index][4].value=6;
	}

	//  Default Static
	if (  t.importer.objectFPE.defaultstatic  ==  "0" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][5].value_s = "No";
		t.slidersmenuvalue[t.importer.properties1Index][5].value=2;
	}
	else
	{
		t.slidersmenuvalue[t.importer.properties1Index][5].value_s = "Yes";
		t.slidersmenuvalue[t.importer.properties1Index][5].value=1;
	}

	//  Strength
	t.slidersmenuvalue[t.importer.properties1Index][6].value = ValF(t.importer.objectFPE.strength.Get());

	//  Cull Mode
	if (  t.importer.objectFPE.cullmode  ==  "0" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][8].value_s = "No";
		t.slidersmenuvalue[t.importer.properties1Index][8].value=2;
	}
	else
	{
		t.slidersmenuvalue[t.importer.properties1Index][8].value_s = "Yes";
		t.slidersmenuvalue[t.importer.properties1Index][8].value=1;
	}

	//  Transparency
	if (  t.importer.objectFPE.transparency  ==  "0" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][9].value_s = "None";
		t.slidersmenuvalue[t.importer.properties1Index][9].value=1;
	}
	if (  t.importer.objectFPE.transparency  ==  "1" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][9].value_s = "Standard";
		t.slidersmenuvalue[t.importer.properties1Index][9].value=2;
	}
	if (  t.importer.objectFPE.transparency  ==  "2" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][9].value_s = "Render last";
		t.slidersmenuvalue[t.importer.properties1Index][9].value=3;
	}
	//  Material Inxed
	if (  t.importer.objectFPE.materialindex  ==  "0" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][10].value_s = "Generic";
		t.slidersmenuvalue[t.importer.properties1Index][10].value=1;
	}
	if (  t.importer.objectFPE.materialindex  ==  "1" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][10].value_s = "Stone";
		t.slidersmenuvalue[t.importer.properties1Index][10].value=2;
	}
	if (  t.importer.objectFPE.materialindex  ==  "2" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][10].value_s = "Metal";
		t.slidersmenuvalue[t.importer.properties1Index][10].value=3;
	}
	if (  t.importer.objectFPE.materialindex  ==  "3" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][10].value_s = "Wood";
		t.slidersmenuvalue[t.importer.properties1Index][10].value=4;
	}
	//  Is Character
	if (  t.importer.objectFPE.ischaracter  ==  "0" ) 
	{
		t.slidersmenuvalue[t.importer.properties1Index][7].value_s = "No";
		t.slidersmenuvalue[t.importer.properties1Index][7].value=1;
	}
	else
	{
		t.slidersmenuvalue[t.importer.properties1Index][7].value_s = "Yes";
		t.slidersmenuvalue[t.importer.properties1Index][7].value=2;
	}

	//  Shader effect
	t.importer.objectFPE.effect = Right(t.importer.objectFPE.effect.Get(), Len(t.importer.objectFPE.effect.Get())-20);

	for ( t.tc = 1 ; t.tc<=  t.importer.shaderFileCount; t.tc++ )
	{
		if (  t.importerShaderFiles[t.tc]  ==  t.importer.objectFPE.effect ) 
		{
			t.slidersmenuvalue[t.importer.properties1Index][2].value=t.tc;
			t.slidersmenuvalue[t.importer.properties1Index][2].value_s=t.importerShaderFiles[t.tc];
			break;
		}
	}
	//  AI MAIN
	for ( t.tc = 1 ; t.tc<=  t.importer.scriptFileCount; t.tc++ )
	{
		if (  t.importerScriptFiles[t.tc]  ==  t.importer.objectFPE.aimain ) 
		{
			t.slidersmenuvalue[t.importer.properties1Index][11].value=t.tc;
			t.slidersmenuvalue[t.importer.properties1Index][11].value_s=t.importerScriptFiles[t.tc];
			break;
		}
	}
}

void imporer_save_multimeshsection(sObject* pObject, int iFileIndex)
{
	t.tString = ""; t.tString = t.tString + importerPadString("textured") + "= " ;WriteString(iFileIndex, t.tString.Get());
	for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
	{
		sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
		if (pMesh)
		{
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
			if (mesh)
			{
				uint64_t materialEntity = mesh->subsets[0].materialID;
				wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				if (pObjectMaterial)
				{
					// texture names and control values
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource.IsValid())
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("baseColorMap") + cstr(iMeshIndex)).Get()) + "= " + TextureFilename; WriteString(iFileIndex, t.tString.Get());
						t.tString = ""; t.tString = t.tString + importerPadString(cStr(cStr("alphaRef") + cstr(iMeshIndex)).Get()) + "= " + cStr(pObjectMaterial->alphaRef); WriteString(iFileIndex, t.tString.Get());
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].resource.IsValid())
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("normalMap") + cstr(iMeshIndex)).Get()) + "= " + TextureFilename; WriteString(iFileIndex, t.tString.Get());
						t.tString = ""; t.tString = t.tString + importerPadString(cStr(cStr("normalStrength") + cstr(iMeshIndex)).Get()) + "= " + cStr(pObjectMaterial->normalMapStrength); WriteString(iFileIndex, t.tString.Get());
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].resource.IsValid())
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("emissiveMap") + cstr(iMeshIndex)).Get()) + "= " + TextureFilename; WriteString(iFileIndex, t.tString.Get());
						t.tString = ""; t.tString = t.tString + importerPadString(cStr(cStr("emissiveStrength") + cstr(iMeshIndex)).Get()) + "= " + cStr(pObjectMaterial->GetEmissiveStrength()); WriteString(iFileIndex, t.tString.Get());
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource.IsValid())
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("surfaceMap") + cstr(iMeshIndex)).Get()) + "= " + TextureFilename; WriteString(iFileIndex, t.tString.Get());
						t.tString = ""; t.tString = t.tString + importerPadString(cStr(cStr("roughnessStrength") + cstr(iMeshIndex)).Get()) + "= " + cStr(pObjectMaterial->roughness); WriteString(iFileIndex, t.tString.Get());
						t.tString = ""; t.tString = t.tString + importerPadString(cStr(cStr("metalnessStrength") + cstr(iMeshIndex)).Get()) + "= " + cStr(pObjectMaterial->metalness); WriteString(iFileIndex, t.tString.Get());
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource.IsValid())
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("occlusionMap") + cstr(iMeshIndex)).Get()) + "= " + TextureFilename; WriteString(iFileIndex, t.tString.Get());
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource.IsValid())
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("displacementMap") + cstr(iMeshIndex)).Get()) + "= " + TextureFilename; WriteString(iFileIndex, t.tString.Get());
					}

					// mesh settings
					bTransparent = pObjectMaterial->userBlendMode == BLENDMODE_ALPHA;
					if (bTransparent)
					{
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("transparency") + cstr(iMeshIndex)).Get()) + "= 1"; WriteString(iFileIndex, t.tString.Get());
					}
					else
					{
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("transparency") + cstr(iMeshIndex)).Get()) + "= 0"; WriteString(iFileIndex, t.tString.Get());
					}
					bCastShadows = pObjectMaterial->IsCastingShadow();
					if (!bCastShadows)
					{
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("castshadow") + cstr(iMeshIndex)).Get()) + "= -1"; WriteString(iFileIndex, t.tString.Get());
					}
					bDoubleSided = mesh->IsDoubleSided();
					if (bDoubleSided)
					{
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("doublesided") + cstr(iMeshIndex)).Get()) + "= 1"; WriteString(iFileIndex, t.tString.Get());
					}
					sFrame* pFrame = pMesh->pFrameAttachedTo;
					if (pFrame)
					{
						wiScene::ObjectComponent* object = wiScene::GetScene().objects.GetComponent(pFrame->wickedobjindex);
						if (object)
						{
							fRenderOrderBias = object->sort_priority;
							t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("renderorderbias") + cstr(iMeshIndex)).Get()) + "= " + cStr(fRenderOrderBias); WriteString(iFileIndex, t.tString.Get());
						}
					}
					bPlanerReflection = pObjectMaterial->shaderType == wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION;
					if (bPlanerReflection)
					{
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("planerreflection") + cstr(iMeshIndex)).Get()) + "= 1"; WriteString(iFileIndex, t.tString.Get());
					}
					fReflectance = pObjectMaterial->reflectance;
					char cReflectance[80];
					sprintf(cReflectance, "%.4f", fReflectance); //PE: We need better precision on this one.
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("reflectance") + cstr(iMeshIndex)).Get()) + "= " + cStr(cReflectance); WriteString(iFileIndex, t.tString.Get());

					// material colors
					dwBaseColor = ((unsigned int)(pObjectMaterial->baseColor.x * 255) << 24);
					dwBaseColor += ((unsigned int)(pObjectMaterial->baseColor.y * 255) << 16);
					dwBaseColor += ((unsigned int)(pObjectMaterial->baseColor.z * 255) << 8);
					dwBaseColor += ((unsigned int)(pObjectMaterial->baseColor.w * 255));
					char tmp[256];
					sprintf(tmp, "%lu", (unsigned long)dwBaseColor);
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("basecolor") + cstr(iMeshIndex)).Get()) + "= " + cStr(tmp); WriteString(iFileIndex, t.tString.Get());
					dwEmmisiveColor = ((unsigned int)(pObjectMaterial->emissiveColor.x * 255) << 24);
					dwEmmisiveColor += ((unsigned int)(pObjectMaterial->emissiveColor.y * 255) << 16);
					dwEmmisiveColor += ((unsigned int)(pObjectMaterial->emissiveColor.z * 255) << 8);
					// ignore pObjectMaterial->emissiveColor.w, it is saved above in emissivestrength!
					sprintf(tmp, "%lu", (unsigned long)dwEmmisiveColor);
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("emissivecolor") + cstr(iMeshIndex)).Get()) + "= " + cStr(tmp); WriteString(iFileIndex, t.tString.Get());
				}
			}
		}
	}
}

void importer_save_fpe(void)
{
	// angle
	t.importer.objectFPE.roty = Str(t.importer.objectAngleY);

	// scale and collision
	t.importer.objectFPE.scale = (iImporterScale * fImporterScaleMultiply);
	LPSTR pCollisionType = "0";
	if (t.importer.collisionshape == 0) pCollisionType = "0";   // box
	if (t.importer.collisionshape == 1) pCollisionType = "1";   // polygon
	if (t.importer.collisionshape == 2) pCollisionType = "2";   // sphere
	if (t.importer.collisionshape == 3) pCollisionType = "3";   // cylinder
	if (t.importer.collisionshape == 4) pCollisionType = "9";   // hull
	if (t.importer.collisionshape == 5) pCollisionType = "21";  // character collision
	if (t.importer.collisionshape == 6) pCollisionType = "50";  // tree collision
	if (t.importer.collisionshape == 7) pCollisionType = "11";  // no collision
	if (t.importer.collisionshape == 8) pCollisionType = "10";	// hull decomp
	if (t.importer.collisionshape == 9) pCollisionType = "8"; // Collision Mesh
	t.importer.objectFPE.collisionmode = pCollisionType;

	//  Default Static or Dynamic
	LPSTR pStaticType = "0"; // dynamic by default
	if (t.importer.defaultstatic == 1) pStaticType = "1";  // static
	t.importer.objectFPE.defaultstatic = pStaticType;

	//  Cull Mode
	sObject* pObject = NULL;
	pObject = GetObjectData(t.importer.objectnumber);
	if (pObject)
	{
		t.importer.objectFPE.cullmode = "0";
		for (int i = 0; i < pObject->iMeshCount; i++)
		{

			sMesh * pMesh = pObject->ppMeshList[i];
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
			if (mesh && !mesh->IsDoubleSided())
			{
				t.importer.objectFPE.cullmode = "1";
				break;
			}
		}
	}

	//  Transparency
	t.importer.objectFPE.transparency = Str(t.slidersmenuvalue[t.importer.properties1Index][9].value - 1);

	// MaterialIndex and Strength
	if (t.importer.ischaracter > 0)
	{
		t.importer.objectFPE.materialindex = Str(6);
		t.importer.objectFPE.strength = Str(150);
	}
	else
	{
		// mesh exclusion uses DBO mesh data to mark material indexes per mesh using value 99999 in dwArbitaryValue
		if(t.importer.meshesToExclude.size() > 0)
			t.importer.objectFPE.materialindex = Str(99999);
		else
			t.importer.objectFPE.materialindex = Str(t.slidersmenuvalue[t.importer.properties1Index][10].value - 1);

		t.importer.objectFPE.strength = Str(t.slidersmenuvalue[t.importer.properties1Index][6].value);
	}

	//  Explodable
	t.importer.objectFPE.explodable = "0";

	//  castshadow
	t.importer.objectFPE.castshadow = "0";

	//  ischaracter
	if(t.importer.ischaracter > 0)//if (t.slidersmenuvalue[t.importer.properties1Index][7].value <= 1)
	{
		t.importer.objectFPE.ischaracter = "1";
		t.importer.objectFPE.aimain = "people\\melee_attack.lua";
	}
	else
	{
		t.importer.objectFPE.ischaracter = "0";
		t.importer.objectFPE.aimain = "";
	}

	//  isobjective
	t.importer.objectFPE.isobjective = "0";//Str(t.slidersmenuvalue[t.importer.properties1Index][8].value);//bug
	t.importer.objectFPE.cantakeweapon = "0";

	//  update shader selection
	if (g.gpbroverride == 1) 
	{
		g_iPreferPBR = 1;
		t.importer.objectFPE.effect = "effectbank\\reloaded\\apbr_basic.fx";
	}
	else 
	{
		g_iPreferPBR = 0;
		t.importer.objectFPE.effect = "effectbank\\reloaded\\entity_basic.fx";
	}

	// animation state
	if (t.importer.ischaracter > 0)
	{
		t.importer.objectFPE.playanimineditor = "idle";
		t.importer.objectFPE.animmax = "0";
		t.importer.objectFPE.anim0 = "";
	}
	else
	{
		if (g_pAnimSlotList.size() > 0)
		{
			if (g_bAnimatingObjectPreview == true)
				t.importer.objectFPE.playanimineditor = "1";
			else
				t.importer.objectFPE.playanimineditor = "0";
			t.importer.objectFPE.animmax = cstr(((int)g_pAnimSlotList.size()) - 1);
			float fStart = g_pAnimSlotList[g_iCurrentAnimationSlotIndex].fStart;
			float fFinish = g_pAnimSlotList[g_iCurrentAnimationSlotIndex].fFinish;
			t.importer.objectFPE.anim0 = cstr((int)fStart) + cstr(",") + cstr((int)fFinish);
		}
		else
		{
			t.importer.objectFPE.playanimineditor = "0";
			t.importer.objectFPE.animmax = "0";
			t.importer.objectFPE.anim0 = "";
		}
	}
	pObject = GetObjectData(t.importer.objectnumber);
	t.importer.objectFPE.animspeed = cstr(pObject->fAnimSpeed * 100);

	// ensure collision objects are back to reset position (ignoring camera height adjustment)
	importer_RestoreCollisionShiftHeight();

	t.chosenFileNameFPE_s = t.importer.tFPESaveName;
	if (  FileOpen (1)  )  CloseFile (1);
	if (  FileExist (t.chosenFileNameFPE_s.Get())  )  DeleteAFile (  t.chosenFileNameFPE_s.Get() ) ;
	OpenToWrite (  1, t.chosenFileNameFPE_s.Get() );

	t.tString = ";Saved by Model Importer" ;WriteString (  1 , t.tString.Get() );
	t.tString = ";header" ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("desc") + "= " + t.importer.objectFPE.desc ;WriteString (  1 , t.tString.Get() );

	if (t.importer.ischaracter > 0)
	{
		t.tString = ""; WriteString (1, t.tString.Get());
		t.tString = ";character"; WriteString (1, t.tString.Get());
		if (t.importer.ischaracter == 1)
		{
			t.tString = ""; t.tString = t.tString + importerPadString("ccpassembly") + "= " + "adult male"; WriteString (1, t.tString.Get());
			t.tString = ""; t.tString = t.tString + importerPadString("voice") + "= " + "David"; WriteString (1, t.tString.Get());
		}
		else
		{
			t.tString = ""; t.tString = t.tString + importerPadString("ccpassembly") + "= " + "adult female"; WriteString (1, t.tString.Get());
			t.tString = ""; t.tString = t.tString + importerPadString("voice") + "= " + "Microsoft Hazel Desktop - English (Great Britain)"; WriteString (1, t.tString.Get());
		}
	}

	// get object ptr
	pObject = GetObjectData(t.importer.objectnumber);

	t.tString = "" ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + ";orientation" ; WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("model") + "= " + t.importer.objectFPE.model ;WriteString (  1 , t.tString.Get() );

	t.tString = "" ; t.tString = t.tString + importerPadString("scale") + "= " + t.importer.objectFPE.scale ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("collisionmode") + "= " + t.importer.objectFPE.collisionmode ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("defaultstatic") + "= " + t.importer.objectFPE.defaultstatic ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("materialindex") + "= " + t.importer.objectFPE.materialindex ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("cullmode") + "= " + t.importer.objectFPE.cullmode ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ;WriteString (  1 , t.tString.Get() );

	// if only one mesh in object
	t.tString = ";visualinfo" ;WriteString (  1 , t.tString.Get() );
	bool bTextured = false;
	if (pObject->iMeshCount==1) 
	{
		// single material export
		sMesh * pMesh = pObject->ppMeshList[0];
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
		if (mesh)
		{
			uint64_t materialEntity = mesh->subsets[0].materialID;
			wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
			if (pObjectMaterial)
			{
				if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource.IsValid())
				{
					cStr TextureFilename = importer_getfilenameonly((LPSTR) pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str());
					t.tString = ""; t.tString = t.tString + importerPadString("textured") + "= " + TextureFilename; WriteString(1, t.tString.Get());
					t.tString = ""; t.tString = t.tString + importerPadString("baseColorMap") + "= " + TextureFilename; WriteString(1, t.tString.Get());
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("alpharef")).Get()) + "= " + cStr(pObjectMaterial->alphaRef); WriteString(1, t.tString.Get());
					bTextured = true;
				}
				if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].resource.IsValid())
				{
					cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].name.c_str());
					t.tString = ""; t.tString = t.tString + importerPadString("normalMap") + "= " + TextureFilename; WriteString(1, t.tString.Get());
					t.tString = ""; t.tString = t.tString + importerPadString("normalStrength") + "= " + cStr(pObjectMaterial->normalMapStrength); WriteString(1, t.tString.Get());
				}
				if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource.IsValid())
				{
					cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str());
					t.tString = ""; t.tString = t.tString + importerPadString("surfaceMap") + "= " + TextureFilename; WriteString(1, t.tString.Get());
					t.tString = ""; t.tString = t.tString + importerPadString("roughnessStrength") + "= " + cStr(pObjectMaterial->roughness); WriteString(1, t.tString.Get());
					t.tString = ""; t.tString = t.tString + importerPadString("metalnessStrength") + "= " + cStr(pObjectMaterial->metalness); WriteString(1, t.tString.Get());
				}
				if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource.IsValid())
				{
					cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name.c_str());
					t.tString = ""; t.tString = t.tString + importerPadString("displacementMap") + "= " + TextureFilename; WriteString(1, t.tString.Get());
				}
				if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].resource.IsValid())
				{
					cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].name.c_str());
					t.tString = ""; t.tString = t.tString + importerPadString("emissiveMap") + "= " + TextureFilename; WriteString(1, t.tString.Get());
					t.tString = ""; t.tString = t.tString + importerPadString("emissiveStrength") + "= " + cStr(pObjectMaterial->GetEmissiveStrength()); WriteString(1, t.tString.Get());
				}
				if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource.IsValid())
				{
					cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name.c_str());
					t.tString = ""; t.tString = t.tString + importerPadString("occlusionMap") + "= " + TextureFilename; WriteString(1, t.tString.Get());
				}

				// do not like this duplication - can do better down the road
				// mesh settings
				bTransparent = pObjectMaterial->userBlendMode == BLENDMODE_ALPHA;
				if (bTransparent) 
				{
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("transparency")).Get()) + "= 1"; WriteString(1, t.tString.Get());
				}
				else 
				{
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("transparency")).Get()) + "= 0"; WriteString(1, t.tString.Get());
				}
				bCastShadows = pObjectMaterial->IsCastingShadow();
				if (!bCastShadows) 
				{
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("castshadow")).Get()) + "= -1"; WriteString(1, t.tString.Get());
				}
				bDoubleSided = mesh->IsDoubleSided();
				if (bDoubleSided) 
				{
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("doublesided")).Get()) + "= 1"; WriteString(1, t.tString.Get());
				}
				sFrame* pFrame = pMesh->pFrameAttachedTo;
				if (pFrame)
				{
					wiScene::ObjectComponent* object = wiScene::GetScene().objects.GetComponent(pFrame->wickedobjindex);
					if (object)
					{
						fRenderOrderBias = object->sort_priority;
						t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("renderorderbias")).Get()) + "= " + cStr(fRenderOrderBias); WriteString(1, t.tString.Get());
					}
				}
				bPlanerReflection = pObjectMaterial->shaderType == wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION;
				if (bPlanerReflection) 
				{
					t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("planerreflection")).Get()) + "= 1"; WriteString(1, t.tString.Get());
				}
				fReflectance = pObjectMaterial->reflectance;
				t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("reflectance") ).Get()) + "= " + cStr(fReflectance); WriteString(1, t.tString.Get());

				// material colors
				dwBaseColor = ((unsigned int)(pObjectMaterial->baseColor.x * 255) << 24);
				dwBaseColor += ((unsigned int)(pObjectMaterial->baseColor.y * 255) << 16);
				dwBaseColor += ((unsigned int)(pObjectMaterial->baseColor.z * 255) << 8);
				dwBaseColor += ((unsigned int)(pObjectMaterial->baseColor.w * 255));
				char tmp[256];
				sprintf(tmp, "%lu", (unsigned long)dwBaseColor);
				t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("basecolor")).Get()) + "= " + cStr(tmp); WriteString(1, t.tString.Get());
				dwEmmisiveColor = ((unsigned int)(pObjectMaterial->emissiveColor.x * 255) << 24);
				dwEmmisiveColor += ((unsigned int)(pObjectMaterial->emissiveColor.y * 255) << 16);
				dwEmmisiveColor += ((unsigned int)(pObjectMaterial->emissiveColor.z * 255) << 8);
				// ignore pObjectMaterial->emissiveColor.w, it is saved above in emissivestrength!
				sprintf(tmp, "%lu", (unsigned long)dwEmmisiveColor);
				t.tString = ""; t.tString = t.tString + importerPadString(cstr(cstr("emissivecolor")).Get()) + "= " + cStr(tmp); WriteString(1, t.tString.Get());
			}
		}
	}
	else
	{
		// multi material mesh export
		bTextured = true;
		imporer_save_multimeshsection(pObject,1);
	}
	if (!bTextured) 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("textured") + "= " + t.importer.objectFPE.textured;WriteString(1, t.tString.Get());
	}

	//PE: Wicked - Below keep textureref
	// a new system to record texture references in FPE so can save standalone better
	for ( int tCount = 1 ; tCount <= IMPORTERTEXTURESMAX; tCount++ )
	{
		if ( strlen ( t.importerTextures[tCount].fileName.Get() ) > 0 ) 
		{
			char pFileOnly[2048];
			strcpy ( pFileOnly, t.importerTextures[tCount].fileName.Get() );
			for ( int n = strlen(pFileOnly)-1; n > 0; n-- )
			{
				if ( pFileOnly[n] == '\\' || pFileOnly[n] == '/' )
				{
					strcpy ( pFileOnly, pFileOnly+n+1 );
					break;
				}
			}
			t.tString = "" ; t.tString = t.tString + importerPadString(cstr(cstr("textureref")+cstr(tCount)).Get()) + " = " + cstr(pFileOnly);
			WriteString (  1 , t.tString.Get() );
		}
	}
	t.tString = "" ; t.tString = t.tString + importerPadString("effect") + "= " + t.importer.objectFPE.effect ;WriteString (  1 , t.tString.Get() );
	t.tString = ""; WriteString (1, t.tString.Get());

	t.tString = ";identity details" ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("ischaracter") + "= " + t.importer.objectFPE.ischaracter ;WriteString (  1 , t.tString.Get() );

	t.tString = "" ; t.tString = t.tString + importerPadString("hasweapon") + "= " + t.importer.objectFPE.hasweapon ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("cantakeweapon") + "= " + t.importer.objectFPE.cantakeweapon ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ;WriteString (  1 , t.tString.Get() );

	t.tString = ";statistics" ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("strength") + "= " + t.importer.objectFPE.strength ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("explodable") + "= " + t.importer.objectFPE.explodable ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ;WriteString (  1 , t.tString.Get() );

	t.tString = ";ai" ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("aimain") + "= " + t.importer.objectFPE.aimain ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ;WriteString (  1 , t.tString.Get() );

	t.tString = ";anim" ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("animspeed") + "= " + t.importer.objectFPE.animspeed ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("animmax") + "= " + t.importer.objectFPE.animmax ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("anim0") + "= " + t.importer.objectFPE.anim0 ;WriteString (  1 , t.tString.Get() );
	t.tString = "" ; t.tString = t.tString + importerPadString("playanimineditor") + "= " + t.importer.objectFPE.playanimineditor ;WriteString (  1 , t.tString.Get() );

	//PE: Set default backdrop.
	t.tString = "";WriteString(1, t.tString.Get());
	t.tString = ";thumbnail";WriteString(1, t.tString.Get());
	t.tString = "thumbnailbackdrop = Blue showroom.dds";WriteString(1, t.tString.Get());

	CloseFile (  1 );

	//PE: You cant change REMEMBERIMPORTFILES , its fixed in the pref file, so if more then 10 is needed something else should be done.
	#define REMEMBERIMPORTFILES 10
	cstr fullfpename = t.tSavePath_s + t.chosenFileNameFPE_s;
	char *find = (char *) pestrcasestr(fullfpename.Get(), "entitybank\\");
	if (find)
	{
		//Only store if entitybank is used, not if saved outside GG.
		
		find += 11; //We use relative path after entitybank\\ for all fpe files.

		//Make list of 10 last imported files, save in pref.
		int firstempty = -1;
		int i = 0;
		for (; i < REMEMBERIMPORTFILES; i++) 
		{
			if (firstempty == -1 && strlen(pref.last_import_files[i]) <= 0)
				firstempty = i;

			if (strlen(pref.last_import_files[i]) > 0 && pestrcasestr(find, pref.last_import_files[i])) 
			{ 
				//already there
				break;
			}
		}
		if (i >= REMEMBERIMPORTFILES) 
		{
			if (firstempty == -1) 
			{
				//No empty slots , rotate.
				for (int ii = 0; ii < REMEMBERIMPORTFILES - 1; ii++) 
				{
					strcpy(pref.last_import_files[ii], pref.last_import_files[ii + 1]);
				}
				strcpy(pref.last_import_files[REMEMBERIMPORTFILES - 1], find);
			}
			else
				strcpy(pref.last_import_files[firstempty], find);
		}
		extern bool bMarketplace_Init;
		bMarketplace_Init = false; //Make sure to update import list in marketplace.

		// only present preview thumb adjustment creator if NOT using batch convert
		extern bool bBatchConverting;
		if (bBatchConverting == true)
		{
			// with batch mode, we do not need a thumb adjustment, that can be done by artist later
		}
		else
		{
			//Open preview to set thumb.
			extern cstr sGotoPreviewWithFile;
			extern int iGotoPreviewType;
			sGotoPreviewWithFile = find;
			iGotoPreviewType = 2;
		}
	}
}

void importer_handleScale ( void )
{
	//  Show or hide guide model
	if (  t.importerTabs[10].selected  ==  1 && g.tabmode  ==  IMPORTERTABPAGE1 ) 
	{
		ShowObject (  t.importer.dummyCharacterObjectNumber );
	}
	else
	{
		HideObject (  t.importer.dummyCharacterObjectNumber );
	}
	if (  t.importer.oldScale  !=  t.slidersmenuvalue[t.importer.properties1Index][1].value || t.importer.showScaleChange > 0 ) 
	{
		t.importer.message = ""; t.importer.message = t.importer.message + "Scale:" + Str(t.slidersmenuvalue[t.importer.properties1Index][1].value) + "%. Object displayed in relation to t.a typical character";
		t.importer.oldScale = t.slidersmenuvalue[t.importer.properties1Index][1].value;
		if (  t.importer.showScaleChange  <=  0  )  t.importer.showScaleChange  =  1;
		if (  t.inputsys.mclick  ==  0  )  --t.importer.showScaleChange;
	}
	t.tscale_f = t.importer.objectScaleForEditing;
	t.tScaleMultiplier_f = t.slidersmenuvalue[t.importer.properties1Index][1].value / 100.0;
	t.tscale_f = t.tscale_f / t.tScaleMultiplier_f;
	t.tscale_f = t.tscale_f * t.importer.camerazoom;
	ScaleObject (  t.importer.dummyCharacterObjectNumber , t.tscale_f , t.tscale_f , t.tscale_f * 0.2 );
}

void importer_draw_wicked(void)
{
}

void importer_draw ( void )
{
	importer_draw_wicked();
}

void importer_quit_for_reload (LPSTR pOptionalCopyModelFile)
{
	// store last good imported model and return if optionalcopy ptr valid
	if (pOptionalCopyModelFile)
	{
		extern char pLaunchAfterSyncLastImportedModel[MAX_PATH];
		strcpy (pOptionalCopyModelFile, pLaunchAfterSyncLastImportedModel);
		t.importer.bQuitForReload = true;
	}

	// free import object (switch to hide while wicked crash can happen)
	if ( ObjectExist(t.importer.objectnumber) == 1 )  HideObject (  t.importer.objectnumber );

	// dummy character model
	if (ObjectExist(t.importer.dummyCharacterObjectNumber))  HideObject(t.importer.dummyCharacterObjectNumber);

	// free importer resources
	importer_free ( );
	t.importer.importerActive = 0;

	// finish UI panel
	bImporter_Window = false;
	bCenterRenderView = false;
	t.visuals.refreshshaders = 1;
	visuals_loop();
}

void importer_quit (void)
{
	importer_quit_for_reload(NULL);
}

void ConvertWorldToRelative ( sFrame* pFrame, GGMATRIX* pStoreNewPoseFrames, GGMATRIX* pTraverseMatrix )
{
	if ( pFrame )
	{
		// take the world bone from pStoreNewPoseFrames for this frame ID and create relative matrix from it
		int iFrameIndex = pFrame->iID;
		GGMATRIX matWorldBone = pStoreNewPoseFrames[iFrameIndex];

		// go through hierarchy, create matCombined as you go, transforming worldbones into relative bones
		float fDet = 0;
		GGMATRIX matInverseOfTraverse = *pTraverseMatrix;
		GGMatrixInverse ( &matInverseOfTraverse, &fDet, pTraverseMatrix );
		GGMATRIX matWorldBoneInBaseSpace;
		GGMatrixMultiply ( &matWorldBoneInBaseSpace, &matWorldBone, &matInverseOfTraverse );
		GGMATRIX matRelativeBone = matWorldBoneInBaseSpace;

		// finally 'restore' the relative bone!
		pFrame->matTransformed = matRelativeBone;

		// use relative bone to calculate world bones as we go
		GGMatrixMultiply ( &pFrame->matCombined, &pFrame->matTransformed, pTraverseMatrix );

		// convert child frames
		ConvertWorldToRelative ( pFrame->pChild, pStoreNewPoseFrames, &pFrame->matCombined );

		// convert sibling frames
		ConvertWorldToRelative ( pFrame->pSibling, pStoreNewPoseFrames, pTraverseMatrix );	
	}
}

void importer_save_entity ( char *filename )
{
	//  Check if user folder exists, if not create it
	t.strwork = ""; t.strwork = t.strwork + t.importer.startDir + "\\entitybank\\user";
	if (  PathExist( t.strwork.Get() )  ==  0 ) 
	{
		MakeDirectory (  t.strwork.Get() );
	}

	cStr tOldDir = GetDir();

	//  Ask for save filename
	t.tSaveFile_s = "";
	t.timporterprotected = 1;
	t.timportermessage_s = "Save Object";
	while (  t.timporterprotected  ==  1 ) 
	{
		if (!filename) 
		{
			if (t.importer.fpeIsMainFile == 0)
			{
				t.strwork = ""; t.strwork = t.strwork + t.importer.startDir + "\\entitybank\\user";
				t.tSaveFile_s = openFileBox("Model (.dbo)|*.dbo|All Files|*.*|", t.strwork.Get(), t.timportermessage_s.Get(), ".dbo", IMPORTERSAVEFILE);
			}
			else
			{
				t.strwork = ""; t.strwork = t.strwork + t.importer.startDir + "\\entitybank\\user";
				t.tSaveFile_s = openFileBox("GG Entity (.fpe)|*.fpe|All Files|*.*|", t.strwork.Get(), t.timportermessage_s.Get(), ".fpe", IMPORTERSAVEFILE);
			}
		}
		else {
			t.strwork = filename;
			t.tSaveFile_s = filename;
		}
		if (  t.tSaveFile_s  ==  "Error" ) 
		{
			t.timportersaveon = 0;
			return;
		}
		t.timporterprotected = importer_check_if_protected(t.tSaveFile_s.Get());
		if (  t.timporterprotected  ==  1 ) 
		{
			t.timportermessage_s = "You cannot overwrite protected media, please choose an alternative name";
		}
	}

	// convert to real save file location
	char pRealSaveFile[MAX_PATH];
	strcpy(pRealSaveFile, t.tSaveFile_s.Get());
	GG_GetRealPath(pRealSaveFile, 1);
	t.tSaveFile_s = pRealSaveFile;

	//  Ensure it has the dbo extension
	Dim (  t.tArray,300  );
	t.tArrayMarker = 0;
	t.tstring_s=t.tSaveFile_s;
	t.tToken_s=FirstToken(t.tstring_s.Get(),".");
	if (  t.tToken_s  !=  "" ) 
	{
		t.tArray[t.tArrayMarker] = t.tToken_s;
		++t.tArrayMarker;
	}
	do
	{
		t.tToken_s=NextToken(".");
		if (  t.tToken_s  !=  "" ) 
		{
			t.tArray[t.tArrayMarker] = t.tToken_s;
			++t.tArrayMarker;
		}
	} while ( !(  t.tToken_s == "" ) );
	t.tStippedFileName_s = "";
	int tCount = 0;
	for ( tCount = 0 ; tCount<=  t.tArrayMarker-2; tCount++ )
	{
		t.tStippedFileName_s = t.tStippedFileName_s + t.tArray[tCount];
	}
	UnDim (  t.tArray );
	t.tStippedFileName_s = t.tSaveFile_s;
	if (  strcmp ( Mid(t.tStippedFileName_s.Get(),Len(t.tStippedFileName_s.Get())-1)  ,  "." ) == 0 ) 
	{
		t.tStippedFileName_s = Left(t.tStippedFileName_s.Get(),Len(t.tStippedFileName_s.Get())-2);
	}
	else
	{
		t.tStippedFileName_s = Left(t.tStippedFileName_s.Get(),Len(t.tStippedFileName_s.Get())-4);
	}

	//  Grab the folder path
	Dim (  t.tArray,300  );
	t.tArrayMarker = 0;
	t.tstring_s=t.tStippedFileName_s;
	t.tToken_s=FirstToken(t.tstring_s.Get(),"\\");
	if (  t.tToken_s  !=  "" ) 
	{
		t.tArray[t.tArrayMarker] = t.tToken_s;
		++t.tArrayMarker;
	}
	do
	{
		t.tToken_s=NextToken("\\");
		if (  t.tToken_s  !=  "" ) 
		{
			t.tArray[t.tArrayMarker] = t.tToken_s;
			++t.tArrayMarker;
		}
	} while ( !(  t.tToken_s == "" ) );
	t.tStippedFileName_s = "";
	for ( tCount = 0 ; tCount<=  t.tArrayMarker-2; tCount++ )
	{
		t.tStippedFileName_s = t.tStippedFileName_s + t.tArray[tCount] + "\\";
	}
	t.tSavePath_s = t.tStippedFileName_s;

	//PE: We need to be in the destination folder.
	if (filename) {
		if (PathExist(t.tSavePath_s.Get()) == 0)
		{
			MakeDirectory(t.tSavePath_s.Get());
		}
		SetDir(t.tSavePath_s.Get());
	}

	//  Store file names
	if (  t.importer.fpeIsMainFile  ==  0 ) 
	{
		t.tSaveFile_s = t.tArray[t.tArrayMarker-1] + ".dbo";
		t.tSaveThumb_s = t.tArray[t.tArrayMarker-1] + ".BMP";
		t.importer.tFPESaveName = t.tArray[t.tArrayMarker-1] + ".fpe";
		t.importer.objectFPE.desc = t.tArray[t.tArrayMarker-1];
		t.importer.objectFPE.model = t.tSaveFile_s;
	}
	else
	{
		t.tSaveFile_s = t.importer.objectFilename;
		t.tSaveThumb_s = t.tArray[t.tArrayMarker-1] + ".BMP";
		t.importer.tFPESaveName = t.tArray[t.tArrayMarker-1] + ".fpe";
		t.importer.objectFPE.model = t.tSaveFile_s;
	}

	// Just before save object, ensure all texture references don't include root path
	sObject* pObject = GetObjectData ( t.importer.objectnumber );
	if ( pObject )
	{
		for ( int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++ )
		{
			sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
			if ( pMesh )
			{
				for ( int iTextureStage = 0; iTextureStage < pMesh->dwTextureCount; iTextureStage++ )
				{
					LPSTR pTexName = pMesh->pTextures[iTextureStage].pName;
					if ( strlen ( pTexName ) > 2 )
					{
						if ( pTexName[1] == ':' )
						{
							for ( int n = strlen(pTexName)-1; n > 0; n-- )
							{
								if ( pTexName[n] == '\\' || pTexName[n] == '/' )
								{
									strcpy ( pTexName, pTexName+n+1 );
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// if selected 'Use Uber Anims', then rename skeleton and apply uber animations automatically
	if ( t.slidersmenuvalue[t.importer.properties1Index][7].value == 3 ) 
	{
		sObject* pObject = GetObjectData(t.importer.objectnumber);
		if ( pObject )
		{
			// load in correct Y pose and overwrite imported modes transform matrices
			GGMATRIX* pStoreNewPoseFrames = NULL;
			int objectnumberforframedatacopy = findFreeObject();
			cstr pAbsPathToUberAnimFile = g.fpscrootdir_s + "\\Files\\entitybank\\Characters\\2CHAINFINGER-YPOSE.dbo";//Uber Soldier.X";//appendanims.x";
			LoadObject ( pAbsPathToUberAnimFile.Get(), objectnumberforframedatacopy );
			if ( ObjectExist ( objectnumberforframedatacopy ) == 1 )
			{
				sObject* pObjectWithYPoseData = GetObjectData ( objectnumberforframedatacopy );
				if ( pObjectWithYPoseData )
				{
					// go through and find differences between poses
					for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
					{
						sFrame* pFrame = pObject->ppFrameList[iFrame];
						if ( pFrame )
						{
							LPSTR pFrameName = pFrame->szName;
							if ( pFrameName )
							{
								if ( strlen(pFrameName) > 0 )
								{
									// for this imported model frame, find the equivilant frame in the appendanim model (with the Y pose hidden in the skinning transform)
									for ( int iFindFrame = 0; iFindFrame < pObjectWithYPoseData->iFrameCount; iFindFrame++ )
									{
										sFrame* pFindFrame = pObjectWithYPoseData->ppFrameList[iFindFrame];
										if ( pFindFrame )
										{
											LPSTR pFindFrameName = pFindFrame->szName;
											if ( pFindFrameName )
											{
												if ( strlen(pFindFrameName) > 0 )
												{
													if ( stricmp ( pFrameName, pFindFrameName ) == NULL )
													{
														// just grab the local relative transform from the Y pose model
														pFrame->matOriginal = pFindFrame->matOriginal;
														pFrame->matTransformed = pFindFrame->matOriginal;

														// done with this findframe
														break;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				DeleteObject ( objectnumberforframedatacopy );
			}

			// now insert Bip01 into the frame hierarchy between Sceene Root and Pelvis
			for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
			{
				sFrame* pFindPelvisFrame = pObject->ppFrameList[iFrame];
				if ( pFindPelvisFrame )
				{
					LPSTR pFrameName = pFindPelvisFrame->szName;
					if ( pFrameName )
					{
						if ( strlen(pFrameName) > 0 )
						{
							if ( stricmp ( pFrameName, "Bip01_Pelvis" ) == NULL )
							{
								// create new Bip01 frame
								sFrame* pPelvisParent_SceneRoot = pFindPelvisFrame->pParent;
								sFrame* pBip01Frame = new sFrame();

								// insert it between scene root and pelvis
								strcpy ( pBip01Frame->szName, "Bip01" );
								pPelvisParent_SceneRoot->pChild = pBip01Frame;
								pBip01Frame->pParent = pPelvisParent_SceneRoot;
								pFindPelvisFrame->pParent = pBip01Frame;
								pBip01Frame->pChild = pFindPelvisFrame;

								// also create a new Animation object for this new Bip01 frame (so appended anim data can go somewhere (below))
								if ( pObject->pAnimationSet )
								{
									sAnimation* pLastAnim = pObject->pAnimationSet->pAnimation;
									if ( pLastAnim )
									{
										while ( pLastAnim->pNext ) pLastAnim = pLastAnim->pNext;
									}
									if ( pLastAnim )
									{
										sAnimation* pNewAnim = new sAnimation();
										pNewAnim->bLinear = 1;
										pNewAnim->pFrame = pBip01Frame;
										strcpy ( pNewAnim->szName, "Bip01" );
										pLastAnim->pNext = pNewAnim;
									}
								}

								// we are done
								break;
							}
						}
					}
				}
			}

			// and insert FIRESPOT limb to the right hand with default values
			for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
			{
				sFrame* pFindRightHandFrame = pObject->ppFrameList[iFrame];
				if ( pFindRightHandFrame )
				{
					LPSTR pFrameName = pFindRightHandFrame->szName;
					if ( pFrameName )
					{
						if ( strlen(pFrameName) > 0 )
						{
							if ( stricmp ( pFrameName, "Bip01_R_Hand" ) == NULL )
							{
								// create new FIRESPOT frame
								sFrame* pFIRESPOTFrame = new sFrame();

								// insert it as child of right hand
								sFrame* pLastChildSybling = pFindRightHandFrame->pChild;
								while (pLastChildSybling->pSibling) pLastChildSybling = pLastChildSybling->pSibling;
								strcpy ( pFIRESPOTFrame->szName, "FIRESPOT" );
								pLastChildSybling->pSibling = pFIRESPOTFrame;
								pFIRESPOTFrame->pParent = pFindRightHandFrame;

								// also create a new Animation object for this new Bip01 frame (so appended anim data can go somewhere (below))
								if ( pObject->pAnimationSet )
								{
									sAnimation* pLastAnim = pObject->pAnimationSet->pAnimation;
									if ( pLastAnim )
									{
										while ( pLastAnim->pNext ) pLastAnim = pLastAnim->pNext;
									}
									if ( pLastAnim )
									{
										sAnimation* pNewAnim = new sAnimation();
										pNewAnim->bLinear = 1;
										pNewAnim->pFrame = pFIRESPOTFrame;
										strcpy ( pNewAnim->szName, "FIRESPOT" );
										pLastAnim->pNext = pNewAnim;
									}
								}

								// we are done
								break;
							}
						}
					}
				}
			}

			// now we need to change the model geometry from a T bone to a Y pose (using the relative local transforms we generated above)
			GGMATRIX matrix;
			if ( 1 )
			{
				GGMatrixIdentity ( &matrix );
				UpdateFrame ( pObject->pFrame, &matrix );
				for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
				{
					// wipe out original vertex data
					sMesh* pMesh = pObject->ppMeshList[iMesh];
					if ( pMesh->pOriginalVertexData ) SAFE_DELETE ( pMesh->pOriginalVertexData );

					// animate mesh on CPU
					AnimateBoneMeshBONE ( pObject, NULL, pMesh );

					// now record new vertex mesh shape
					if ( pMesh->pOriginalVertexData ) SAFE_DELETE ( pMesh->pOriginalVertexData );
					#ifndef NEVERSTOREORIGINALVERTICES
					//PE: Not used.
					DWORD dwTotalVertSize = pMesh->dwVertexCount * pMesh->dwFVFSize;
					pMesh->pOriginalVertexData = (BYTE*)new char [ dwTotalVertSize ];
					memcpy ( pMesh->pOriginalVertexData, pMesh->pVertexData, dwTotalVertSize );
					#endif
				}
			}

			// now replace frame transform matrices in imported model (copied from appendanims.x)
			// which zeros the rotation and puts model in Y shape pose (not T) ready for stock 
			// animation data (but retains skinning information for the unique mesh geometry of the model)
			objectnumberforframedatacopy = findFreeObject();
			pAbsPathToUberAnimFile = g.fpscrootdir_s + "\\Files\\entitybank\\Characters\\appendanims.x";
			LoadObject ( pAbsPathToUberAnimFile.Get(), objectnumberforframedatacopy );
			if ( ObjectExist ( objectnumberforframedatacopy ) == 1 )
			{
				sObject* pObjectWithGoodFrameData = GetObjectData ( objectnumberforframedatacopy );
				if ( pObjectWithGoodFrameData )
				{
					// go through all frames of imported object, match up name and copy frame transform matrix over from 'framegood' model
					for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
					{
						sFrame* pFrame = pObject->ppFrameList[iFrame];
						if ( pFrame )
						{
							LPSTR pFrameName = pFrame->szName;
							if ( pFrameName )
							{
								if ( strlen(pFrameName) > 0 )
								{
									// check to find this name in framegood model
									for ( int iFrameGood = 0; iFrameGood < pObjectWithGoodFrameData->iFrameCount; iFrameGood++ )
									{
										sFrame* pFrameGood = pObjectWithGoodFrameData->ppFrameList[iFrameGood];
										if ( pFrameGood )
										{
											LPSTR pFrameGoodName = pFrameGood->szName;
											if ( pFrameGoodName )
											{
												if ( strlen(pFrameGoodName) > 0 )
												{
													// check if we have a match
													if ( stricmp ( pFrameName, pFrameGoodName ) == NULL )
													{
														// now copy the frame data to the import model
														pFrame->matOriginal = pFrameGood->matOriginal;
														pFrame->matTransformed = pFrameGood->matTransformed;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				DeleteObject ( objectnumberforframedatacopy );
			}

			// and restore transformed matrix from originals
			for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
			{
				sFrame* pFrame = pObject->ppFrameList[iFrame];
				if ( pFrame )
				{
					pFrame->matTransformed = pFrame->matOriginal;
				}
			}

			// delete all keys so append can be fresh (and specifically remove matrix keys which WILL mess up overall animation)
			if ( pObject->pAnimationSet )
			{
				sAnimationSet* pAnimSet = pObject->pAnimationSet;
				while ( pAnimSet != NULL )
				{
					sAnimation* pAnim = pAnimSet->pAnimation;
					while ( pAnim != NULL )
					{
						// scans all animation data and creates the interpolation vectors between all keyframes (vital)
						if ( pAnim )
						{
							SAFE_DELETE(pAnim->pPositionKeys);
							pAnim->dwNumPositionKeys=0;
							SAFE_DELETE(pAnim->pRotateKeys);
							pAnim->dwNumRotateKeys=0;
							SAFE_DELETE(pAnim->pScaleKeys);
							pAnim->dwNumScaleKeys=0;
							SAFE_DELETE(pAnim->pMatrixKeys);
							pAnim->dwNumMatrixKeys=0;
						}
						pAnim = pAnim->pNext;
					}
					pAnimSet = pAnimSet->pNext;
				}
			}

			// append uber animations to character
			AppendAnimationFromFile ( pObject, pAbsPathToUberAnimFile.Get(), 0 );

			// now update model to first frame animation pose (to calculate combined matrix from pose)
			GGMatrixIdentity ( &matrix );
			UpdateAllFrameData ( pObject, 0.0f );
			UpdateFrame ( pObject->pFrame, &matrix );

			// and then calculate the inverse of those matCombined transforms to apply to the bone matTransforms
			// so bone matrix cancels out first frame pose leaving matCombined animations to shape shader verts
			for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
			{
				sMesh* pMesh = pObject->ppMeshList[iMesh];
				if ( pMesh )
				{
					sBone* pBones = pMesh->pBones;
					if ( pBones )
					{
						for ( int iBone = 0; iBone < pMesh->dwBoneCount; iBone++ )
						{
							float fDet = 0.0f;
							GGMatrixInverse ( &pBones[iBone].matTranslation, &fDet, pMesh->pFrameMatrices [ iBone ] );
						}
					}
				}
			}
		}
	}

	// if we have meshes to exclude in exported DBO file, mark in dwArbitaryValue inside mesh data as 99999
	if (t.importer.meshesToExclude.size() > 0)
	{
		sObject* pObject = GetObjectData(t.importer.objectnumber);
		if (pObject)
		{
			for (int i = 0; i < pObject->iMeshCount; i++)
			{
				sMesh* pMesh = pObject->ppMeshList[i];
				if (pMesh)
				{
					if(t.importer.meshesToExclude[i]==1)
						pMesh->Collision.dwArbitaryValue = 99999;
					else
						pMesh->Collision.dwArbitaryValue = t.slidersmenuvalue[t.importer.properties1Index][10].value - 1;
				}
			}
		}
	}

	// Save Object
	if ( strcmp ( Lower(Right(t.tSaveFile_s.Get(),4)) , ".dbo" ) == 0 ) 
	{
		if ( FileExist (t.tSaveFile_s.Get()) == 1 ) DeleteAFile ( t.tSaveFile_s.Get() ) ;
		ScaleObject (  t.importer.objectnumber,100,100,100 );
		SaveObject (  t.tSaveFile_s.Get(),t.importer.objectnumber );
		ScaleObject (  t.importer.objectnumber,t.importer.objectScaleForEditing,t.importer.objectScaleForEditing,t.importer.objectScaleForEditing );
	}
	else
	{
		if (  t.importer.objectFileOriginalPath+t.importer.objectFilename  !=  t.tSavePath_s+t.importer.objectFilename ) 
		{
			t.strwork = ""; t.strwork = t.strwork +t.importer.objectFileOriginalPath+t.importer.objectFilename;
			if (  FileExist( t.strwork.Get() ) == 1 ) 
			{
				t.strwork = ""; t.strwork = t.strwork +t.tSavePath_s+t.importer.objectFilename;
				if (  FileExist( t.strwork.Get() ) == 0 ) 
				{
					t.strwork = ""; t.strwork = t.strwork +t.importer.objectFileOriginalPath+t.importer.objectFilename;
					cstr string1 = t.tSavePath_s+t.importer.objectFilename;
					char pRealSaveModelFile[MAX_PATH];
					strcpy(pRealSaveModelFile, string1.Get());
					GG_GetRealPath(pRealSaveModelFile, 1);
					CopyAFile ( t.strwork.Get() , string1.Get() );
				}
			}
		}
	}
	UnDim (  t.tArray );

	t.tcounttextures = 0;
	sMesh * pMesh = NULL;
	for (int i = 0; i < pObject->iFrameCount; i++)
	{
		if (pObject->ppFrameList[i]->pMesh)
		{
			pMesh = pObject->ppFrameList[i]->pMesh;
			if (pMesh)
			{
				wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
				if (mesh)
				{
					uint64_t materialEntity = mesh->subsets[0].materialID;
					wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
					if (pObjectMaterial)
					{
						int tCount = 1;

						if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource.IsValid())
						{
							cStr TextureFilename = (char *) pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str();
							int iInsertedAtSlot = importer_addtexturefiletolist(TextureFilename, TextureFilename, &t.tcounttextures);
							t.importerTextures[iInsertedAtSlot].imageID = 1; //Fake id. Reset later.
						}

						if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].resource.IsValid())
						{
							cStr TextureFilename = (char *)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].name.c_str();
							int iInsertedAtSlot = importer_addtexturefiletolist(TextureFilename, TextureFilename, &t.tcounttextures);
							t.importerTextures[iInsertedAtSlot].imageID = 1; //Fake id. Reset later.
						}

						if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource.IsValid())
						{
							cStr TextureFilename = (char *)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str();
							int iInsertedAtSlot = importer_addtexturefiletolist(TextureFilename, TextureFilename, &t.tcounttextures);
							t.importerTextures[iInsertedAtSlot].imageID = 1; //Fake id. Reset later.
						}

						if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource.IsValid())
						{
							cStr TextureFilename = (char *)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name.c_str();
							int iInsertedAtSlot = importer_addtexturefiletolist(TextureFilename, TextureFilename, &t.tcounttextures);
							t.importerTextures[iInsertedAtSlot].imageID = 1; //Fake id. Reset later.
						}

						if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].resource.IsValid())
						{
							cStr TextureFilename = (char *)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].name.c_str();
							int iInsertedAtSlot = importer_addtexturefiletolist(TextureFilename, TextureFilename, &t.tcounttextures);
							t.importerTextures[iInsertedAtSlot].imageID = 1; //Fake id. Reset later.
						}
						if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource.IsValid())
						{
							cStr TextureFilename = (char *)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name.c_str();
							int iInsertedAtSlot = importer_addtexturefiletolist(TextureFilename, TextureFilename, &t.tcounttextures);
							t.importerTextures[iInsertedAtSlot].imageID = 1; //Fake id. Reset later.
						}
					}
				}
			}
		}
	}

	Dim ( t.tArray2, 220 );
	for ( tCount = 1; tCount <= IMPORTERTEXTURESMAX; tCount++ )
	{
		// skip files that do not exist
		if ( t.importerTextures[tCount].imageID == 0 )  
			continue;

		//  Split the filename into tokens to grab the path
		t.tSourceName_s = t.importerTextures[tCount].fileName;
		t.tArrayMarker = 0;
		t.tstring_s=t.tSourceName_s;
		t.tToken_s=FirstToken(t.tstring_s.Get(),"\\");
		if (  t.tToken_s  !=  "" ) 
		{
			t.tArray2[t.tArrayMarker] = t.tToken_s;
			++t.tArrayMarker;
		}
		do
		{
			t.tToken_s=NextToken("\\");
			if (  t.tToken_s  !=  "" ) 
			{
				t.tArray2[t.tArrayMarker] = t.tToken_s;
				++t.tArrayMarker;
			}
		} while ( !(  t.tToken_s == "" ) );

		if (t.tArrayMarker <= 0)
			continue;

		//  Now store just the file name
		t.tDestFileName = t.tSavePath_s + t.tArray2[t.tArrayMarker-1];

		//If source have subdirs , remove them from dest.
		char tmp[MAX_PATH];
		strcpy(tmp, t.tArray2[t.tArrayMarker - 1].Get() );
		if (pestrcasestr(tmp, "\\") || pestrcasestr(tmp, "/"))
		{
			int pos = strlen(tmp);
			while (pos > 0 && tmp[pos] != '\\' && tmp[pos] != '/') pos--;
			if (pos > 0) {
				strcpy(tmp, &tmp[pos + 1]);
				t.tDestFileName = t.tSavePath_s + tmp;
			}
		}

		if ( tCount == 1 )
		{
			// first texture in list is the 'color/diffuse' one (even if multitexture)
			t.importer.objectFPE.textured = t.tArray2[t.tArrayMarker-1];
		}

		//  Copy and save images to destination
		char pRealDestFileName[MAX_PATH];
		strcpy(pRealDestFileName, t.tDestFileName.Get());
		GG_GetRealPath(pRealDestFileName, 1);
		cstr test1 = ""; test1 = test1 + g.fpscrootdir_s + "\\Files\\" + t.importerTextures[tCount].fileName;
		if (  FileExist (t.importerTextures[tCount].fileName.Get())  ==  1 && t.tDestFileName  !=  test1 && t.tDestFileName  !=  t.importerTextures[tCount].fileName ) 
		{
			if (  FileExist(pRealDestFileName)  ==  0 ) 
			{
				CopyAFile ( t.importerTextures[tCount].fileName.Get(), pRealDestFileName );
			}
		}
		else
		{
			t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.importerTextures[tCount].fileName;
			cstr test2 = ""; test2 = test2 + g.fpscrootdir_s + "\\Files\\" + t.importerTextures[tCount].fileName;
			if (  FileExist ( t.strwork.Get() )  ==  1 && t.tDestFileName  !=  test2 ) 
			{
				if (  FileExist(pRealDestFileName)  ==  0 ) 
				{
					t.strwork = "" ; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.importerTextures[tCount].fileName;
					CopyAFile ( t.strwork.Get() , pRealDestFileName );
				}
			}
		}

		// Check for Normal and Specular and copy those if they exist
		if ( Len(t.importerTextures[tCount].fileName.Get()) > 6 ) 
		{
			if ( strcmp ( Lower(Right(t.importerTextures[tCount].fileName.Get(),6)) , "_d.dds" ) == 0 || strcmp ( Lower(Right(t.importerTextures[tCount].fileName.Get(),6)) , "_d.png" ) == 0 ) 
			{
				//  Normal map first
				t.tnormalmapfile_s = ""; t.tnormalmapfile_s = t.tnormalmapfile_s + Left(t.importerTextures[tCount].fileName.Get(),Len(t.importerTextures[tCount].fileName.Get())-6) + "_N.dds";
				t.tDestFileName = t.tSavePath_s + t.tArray2[t.tArrayMarker-1];
				cstr TempFileName = t.tDestFileName;
				t.tDestFileName = "" ; t.tDestFileName = t.tDestFileName + Left(TempFileName.Get(),Len(TempFileName.Get())-6) + "_N.dds";
				strcpy(pRealDestFileName, t.tDestFileName.Get());
				GG_GetRealPath(pRealDestFileName, 1);
				t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
				if ( FileExist (t.tnormalmapfile_s.Get())  ==  1 && t.tDestFileName  !=  t.strwork && t.tDestFileName  !=  t.tnormalmapfile_s )
				{
					if ( FileExist(pRealDestFileName)  ==  0 ) 
					{
						CopyAFile ( t.tnormalmapfile_s.Get(), pRealDestFileName );
					}
				}
				else
				{
					t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					if (  FileExist ( t.strwork.Get() )  ==  1 && t.tDestFileName  !=  g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s ) 
					{
						if ( FileExist(pRealDestFileName)  ==  0 ) 
						{
							t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
							CopyAFile ( t.strwork.Get(), pRealDestFileName );
						}
					}
				}

				//  Normal map continued
				t.tnormalmapfile_s = ""; t.tnormalmapfile_s = t.tnormalmapfile_s + Left(t.importerTextures[tCount].fileName.Get(),Len(t.importerTextures[tCount].fileName.Get())-6) + "_N.png";
				t.tDestFileName = t.tSavePath_s + t.tArray2[t.tArrayMarker-1];
				TempFileName = t.tDestFileName;
				t.tDestFileName = "" ; t.tDestFileName = t.tDestFileName + Left(TempFileName.Get(),Len(TempFileName.Get())-6) + "_N.png";
				strcpy(pRealDestFileName, t.tDestFileName.Get());
				GG_GetRealPath(pRealDestFileName, 1);
				t.strwork = ""; t.strwork = t.strwork + "\\Files\\" + t.tnormalmapfile_s;
				if (  FileExist (t.tnormalmapfile_s.Get())  ==  1 && t.tDestFileName  !=  g.fpscrootdir_s + t.strwork && t.tDestFileName  !=  t.tnormalmapfile_s )  
				{
					if (  FileExist(pRealDestFileName)  ==  0 ) 
					{
						CopyAFile (  t.tnormalmapfile_s.Get() ,pRealDestFileName );
					}
				}
				else
				{

					cstr work1 = ""; work1 = work1 + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					if (  FileExist ( work1.Get() )  ==  1 && t.tDestFileName  !=  t.strwork )  
					{
						if (  FileExist(pRealDestFileName)  ==  0 ) 
						{
							t.strwork = "" ; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
							CopyAFile (  t.strwork.Get() , pRealDestFileName );
						}
					}
				}

				//  Specular now
				t.tnormalmapfile_s = "" ; t.tnormalmapfile_s = t.tnormalmapfile_s + Left(t.importerTextures[tCount].fileName.Get(),Len(t.importerTextures[tCount].fileName.Get())-6) + "_S.dds";
				t.tDestFileName = t.tSavePath_s + t.tArray2[t.tArrayMarker-1];
				TempFileName = t.tDestFileName;
				t.tDestFileName = "" ; t.tDestFileName = t.tDestFileName + Left (TempFileName.Get(),Len(TempFileName.Get())-6) + "_S.dds";
				strcpy(pRealDestFileName, t.tDestFileName.Get());
				GG_GetRealPath(pRealDestFileName, 1);
				t.strwork = "" ; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
				if (  FileExist (t.tnormalmapfile_s.Get())  ==  1 && t.tDestFileName  !=  t.strwork && t.tDestFileName  !=  t.tnormalmapfile_s ) 
				{
					if (  FileExist(pRealDestFileName)  ==  0 ) 
					{
						CopyAFile (  t.tnormalmapfile_s.Get() , pRealDestFileName );
					}
				}
				else
				{
					cstr work2 = ""; work2 = work2 + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					if (  FileExist ( work2.Get() )  ==  1 && t.tDestFileName  !=  t.strwork ) 
					{
						if (  FileExist(pRealDestFileName)  ==  0 ) 
						{
							t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
							CopyAFile (  t.strwork.Get() ,pRealDestFileName );
						}
					}
				}

				t.tnormalmapfile_s = ""; t.tnormalmapfile_s = t.tnormalmapfile_s + Left(t.importerTextures[tCount].fileName.Get(),Len(t.importerTextures[tCount].fileName.Get())-6) + "_S.png";
				t.tDestFileName = t.tSavePath_s + t.tArray2[t.tArrayMarker-1];
				TempFileName = t.tDestFileName;
				t.tDestFileName = ""; t.tDestFileName = t.tDestFileName + Left(TempFileName.Get(),Len(TempFileName.Get())-6) + "_S.png";
				strcpy(pRealDestFileName, t.tDestFileName.Get());
				GG_GetRealPath(pRealDestFileName, 1);
				t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
				if (  FileExist (t.tnormalmapfile_s.Get())  ==  1 && t.tDestFileName  !=  t.strwork && t.tDestFileName  !=  t.tnormalmapfile_s ) 
				{
					if (  FileExist(pRealDestFileName)  ==  0 ) 
					{
						CopyAFile (  t.tnormalmapfile_s.Get(),pRealDestFileName );
					}
				}
				else
				{
					cstr work3 = ""; work3 = work3 + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					t.strwork = "" ; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
					if (  FileExist ( work3.Get() )  ==  1 && t.tDestFileName  !=  t.strwork ) 
					{
						if (  FileExist(pRealDestFileName )  ==  0 ) 
						{
							t.strwork = "" ; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\" + t.tnormalmapfile_s;
							CopyAFile (  t.strwork.Get() ,pRealDestFileName );
						}
					}
				}
			}
		}
	}
	UnDim (  t.tArray2 );

	// If model has MORE than one texture, blank out FPE textured
	if ( t.tcounttextures > 1 ) 
	{
		// but make sure they are 'different' textures, not just DNS/PBR sets
		LPSTR pFile = t.importerTextures[1].fileName.Get();
		LPSTR pExt = "_d.png";
		if ( strnicmp ( pFile+strlen(pFile)-10, "_color.png", 10 ) == NULL ) pExt = "_color.png";
		if ( strnicmp ( pFile+strlen(pFile)-12, "_diffuse.png", 12 ) == NULL ) pExt = "_diffuse.png";
		if ( strnicmp ( pFile+strlen(pFile)-11, "_albedo.png", 11 ) == NULL ) pExt = "_albedo.png";
		if ( strnicmp ( pFile+strlen(pFile)-10, "_color.dds", 10 ) == NULL ) pExt = "_color.dds";
		if ( strnicmp ( pFile+strlen(pFile)-12, "_diffuse.dds", 12 ) == NULL ) pExt = "_diffuse.dds";
		if ( strnicmp ( pFile+strlen(pFile)-11, "_albedo.dds", 11 ) == NULL ) pExt = "_albedo.dds";
		cstr pBaseFilePart = Left(pFile,Len(pFile)-strlen(pExt));
		for ( int tCount = 2; tCount <= 10; tCount++ )
		{
			LPSTR pCompareFile = t.importerTextures[tCount].fileName.Get();
			if ( strlen ( pCompareFile ) > 4 )
			{
				if ( strnicmp ( pCompareFile, pBaseFilePart.Get(), strlen(pBaseFilePart.Get()) ) == NULL )
				{
					// matches base part, now exclude files of known extension
					bool bExclude = false;
					char pRest[1024];
					strcpy ( pRest, pCompareFile + strlen(pBaseFilePart.Get()) );
					pRest[strlen(pRest)-4] = 0;
					if ( stricmp ( pRest, "_normal" ) == NULL ) bExclude = true;
					if ( stricmp ( pRest, "_specular" ) == NULL ) bExclude = true;
					if ( stricmp ( pRest, "_metalness" ) == NULL ) bExclude = true;
					if ( stricmp ( pRest, "_gloss" ) == NULL ) bExclude = true;
					if ( stricmp ( pRest, "_ao" ) == NULL ) bExclude = true;
					if ( stricmp ( pRest, "_height" ) == NULL ) bExclude = true;
					if ( stricmp ( pRest, "_cube" ) == NULL ) bExclude = true;
					if ( bExclude == false )
					{
						// found a match to base texture, but unknown extra part
						t.importer.objectFPE.textured="";
						break;
					}
				}
				else
				{
					// found a different base texture, cannot be a single textured model
					t.importer.objectFPE.textured="";
					break;
				}
			}
		}
	}


	// save FPE file
	importer_save_fpe ( );

	for (int tCount = 1; tCount <= t.tcounttextures; tCount++)
	{
		t.importerTextures[tCount].fileName = "";
		t.importerTextures[tCount].imageID = 0;
	}
	t.tcounttextures = 0;

	Sleep(1000);

	// Save/Generate Thumbnail BMP
	char pRealSavethumb[MAX_PATH];
	strcpy(pRealSavethumb, t.tSaveThumb_s.Get());
	GG_GetRealPath(pRealSavethumb, 1);
	t.strwork = ""; t.strwork = t.strwork + t.importer.objectFileOriginalPath + t.importer.objectFilename;
	cstr pSourceBMP = cstr ( cstr(Left ( t.strwork.Get(), strlen(t.strwork.Get())-4)) + ".bmp" );
	if ( FileExist ( pSourceBMP.Get() ) ) 
	{
		// if BMP already existed in source area, use that one
		if ( FileExist ( pRealSavethumb ) == 1 ) DeleteAFile ( pRealSavethumb );
		CopyFileA ( pSourceBMP.Get(), pRealSavethumb, FALSE );
	}
	else
	{
		//PE: IMPORTER_TMP_IMAGE is the same as the importer object use as texture.
		//PE: Use IMPORTER_TMP_IMAGE+1 (same as char kit). so importer object dont disapear.
		int grab_image = IMPORTER_TMP_IMAGE;
		grab_image = IMPORTER_TMP_IMAGE+1;

		if (GetImageExistEx(grab_image))
			DeleteImage(grab_image);
	}

	t.importer.cancel = 1;
	t.timportersaveon = 0;

	if (filename) 
	{
		SetDir(tOldDir.Get());
	}
}

void import_generate_thumb(void)
{
	// Need a way to generate a thumbnail in Wicked Engine
	char pFullPathTempThumb[MAX_PATH];
	strcpy(pFullPathTempThumb, g.fpscrootdir_s.Get());
	strcat(pFullPathTempThumb, "\\Files\\editors\\uiv3\\ThumbnailTemplate.png");
	image_setlegacyimageloading(true);
	LoadImage(pFullPathTempThumb, IMPORTER_TMP_IMAGE+1); //PE: Same as char kit in wicked.
	image_setlegacyimageloading(false);
}

void importer_tabs_update ( void )
{
	if (bRemoveSprites)
		return;
	//  unselect buttons after a time
	if (  t.importer.buttonPressedCount > 0 ) 
	{
		--t.importer.buttonPressedCount;
		if (  t.importer.buttonPressedCount  <= 0 ) 
		{
			t.importerTabs[12].selected = 0;
			for ( int tCount = 5 ; tCount<=  9; tCount++ )
			{
				t.importerTabs[tCount].selected = 0;
			}
		}
	}

	if (  t.importer.oldMouseClick  ==  0 && t.inputsys.mclick  ==  1 ) 
	{
		for ( t.tCount5 = 1 ; t.tCount5 <= 12; t.tCount5++ )
		{
			if (  t.importer.MouseX  >=  t.importerTabs[t.tCount5].x && t.importer.MouseX  <=  t.importerTabs[t.tCount5].x + 128 ) 
			{
				if (  t.importer.MouseY  >=  t.importerTabs[t.tCount5].y && t.importer.MouseY  <=  t.importerTabs[t.tCount5].y + 32 ) 
				{
					if (  t.tCount5 < 5 ) 
					{

						for ( t.tCount2 = 1 ; t.tCount2<=  3; t.tCount2++ )
						{
							if (  t.tCount5  <=  3 ) 
							{
								if (  t.tCount2  ==  t.tCount5  )  t.importerTabs[t.tCount2].selected  =  1; else t.importerTabs[t.tCount2].selected  =  0;
							}
						}
						if (  t.tCount5  <= 3 ) 
						{
							g.tabmode = t.importerTabs[t.tCount5].tabpage;
							RotateObject (  t.importer.objectnumber,0,0,0 );
						}

					}
					else
					{

						//  check for save entity/object pressed
						if (  t.tCount5  ==  5 ) 
						{
							t.importerTabs[5].selected = 1;
						}

						//  cancel
						if (  t.tCount5  ==  11 ) 
						{
							t.importerTabs[11].selected = 1;
							t.importer.cancel = 1;
							t.importer.cancelCount = 10;
						}
						//  tab 1 buttons
						if (  t.tCount5  ==  10 && t.importerTabs[1].selected  ==  1 ) 
						{
							t.importerTabs[t.tCount5].selected = 1 - t.importerTabs[t.tCount5].selected;
						}

						//  tab 2 buttons
						if (  t.tCount5 > 5 && t.importerTabs[2].selected  ==  1 ) 
						{
							t.importerTabs[t.tCount5].selected = 1;
							t.importer.buttonPressedCount = 20;

							//  add new collision Box (  )
							if (  t.tCount5  ==  6 ) 
							{
								importer_add_collision_box ( );
							}

							//  delete collision Box (  )
							if (  t.tCount5  ==  7 ) 
							{
								importer_delete_collision_box ( );
							}

							//  next collision Box (  )
							if (  t.tCount5  ==  8 ) 
							{
								++t.importer.selectedCollisionObject;
								if (  t.importer.selectedCollisionObject  >=  t.importer.collisionShapeCount ) 
								{
									t.importer.selectedCollisionObject = 0;
								}
							}

							//  previous collision Box (  )
							if (  t.tCount5  ==  9 ) 
							{
								--t.importer.selectedCollisionObject;
								if (  t.importer.selectedCollisionObject < 0 ) 
								{
									t.importer.selectedCollisionObject = t.importer.collisionShapeCount-1;
								}
							}

							//  add new collision Box (  )
							if (  t.tCount5  ==  12 ) 
							{
								importer_dupe_collision_box ( );
							}

						}
					}
				}
			}
		}
	}

	if ( t.importerTabs[10].selected == 1 ) 
	{
		t.importerTabs[10].label = "Turn Guide Off";
	}
	else
	{
		t.importerTabs[10].label = "Turn Guide On";
	}
}

void importer_tabs_draw ( void )
{
	if(bRemoveSprites)
		return;
	// if image exist, importer active
	if ( t.importer.importerActive == 1 )
	{
		for ( int tCount = 1 ; tCount<=  12; tCount++ )
		{
			if (  tCount < 4 || tCount  >=  5 ) 
			{
				//  dont draw the collsion buttons if the tab isnt active
				t.skip = 0;
				if (  tCount > 5 && tCount < 10 && t.importerTabs[2].selected  ==  0  )  t.skip  =  1;
				if (  tCount  ==  12 && t.importerTabs[2].selected  ==  0  )  t.skip  =  1;
				if (  tCount  ==  10 && t.importerTabs[1].selected  ==  0  )  t.skip  =  1;

				// going to draw button
				if (  t.skip  ==  0 ) 
				{
					if (  t.importerTabs[tCount].selected  ==  1 ) 
					{
						PasteImage (  g.importermenuimageoffset+5 , t.importerTabs[tCount].x , t.importerTabs[tCount].y );
					}
					else
					{
						PasteImage (  g.importermenuimageoffset+4 , t.importerTabs[tCount].x , t.importerTabs[tCount].y );
					}
					if (  tCount > 5 && tCount < 11 || tCount  ==  12 ) 
					{
						pastebitmapfont(t.importerTabs[tCount].label.Get(),t.importerTabs[tCount].x + 64 - ((getbitmapfontwidth(t.importerTabs[tCount].label.Get(),2)) / 2) ,t.importerTabs[tCount].y + 9,2,255);
					}
					else
					{
						pastebitmapfont(t.importerTabs[tCount].label.Get(),t.importerTabs[tCount].x + 64 - ((getbitmapfontwidth(t.importerTabs[tCount].label.Get(),1)) / 2) ,t.importerTabs[tCount].y,1,255);
					}
				}
			}
		}
	}
}

void importer_add_collision_box ( void )
{
	if (bRemoveSprites)
		return;
	t.tScale = t.importer.objectScaleForEditing;
	if (  t.importer.collisionShapeCount < 49 ) 
	{
		t.importerCollision[t.importer.collisionShapeCount].object = findFreeObject();
		MakeObjectBox (  t.importerCollision[t.importer.collisionShapeCount].object , 100 , 100 , 100 );
		ScaleLimb (  t.importerCollision[t.importer.collisionShapeCount].object, 0, t.tObjectSizeX_f , t.tObjectSizeY_f, t.tObjectSizeZ_f );
		t.importerCollision[t.importer.collisionShapeCount].sizex = t.tObjectSizeX_f;
		t.importerCollision[t.importer.collisionShapeCount].sizey = t.tObjectSizeY_f;
		t.importerCollision[t.importer.collisionShapeCount].sizez = t.tObjectSizeZ_f;
		LockObjectOn (  t.importerCollision[t.importer.collisionShapeCount].object );
		PositionObject (  t.importerCollision[t.importer.collisionShapeCount].object , t.tBoxOffsetLeft_f , t.tBoxOffsetTop_f, t.tBoxOffsetFront_f );
		PositionObject (  t.importerGridObject[9], 0 , 0 , 0 );
		t.importerCollision[t.importer.collisionShapeCount].rotx = 0;
		t.importerCollision[t.importer.collisionShapeCount].roty = 0;
		t.importerCollision[t.importer.collisionShapeCount].rotz = 0;
		GlueObjectToLimbEx (  t.importerCollision[t.importer.collisionShapeCount].object,t.importerGridObject[9], 0 , 1 );
		DisableObjectZDepth (  t.importerCollision[t.importer.collisionShapeCount].object );
		SetObjectLight (  t.importerCollision[t.importer.collisionShapeCount].object,0 );
		HideObject (  t.importerCollision[t.importer.collisionShapeCount].object );
		SetObjectEffect ( t.importerCollision[t.importer.collisionShapeCount].object, g.guishadereffectindex );
		SetObjectEmissive ( t.importerCollision[t.importer.collisionShapeCount].object, Rgb(255,255,255) );
		SetAlphaMappingOn ( t.importerCollision[t.importer.collisionShapeCount].object, 10 );

		t.importerCollision[t.importer.collisionShapeCount].object2 = findFreeObject();
		MakeObjectBox (  t.importerCollision[t.importer.collisionShapeCount].object2 , 100,100,100 );
		ScaleLimb (  t.importerCollision[t.importer.collisionShapeCount].object2, 0, t.tObjectSizeX_f , t.tObjectSizeY_f, t.tObjectSizeZ_f );
		LockObjectOn (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		PositionObject (  t.importerCollision[t.importer.collisionShapeCount].object2 , t.tBoxOffsetLeft_f , t.tBoxOffsetTop_f, t.tBoxOffsetFront_f );
		SetObjectCull (  t.importerCollision[t.importer.collisionShapeCount].object,0 );
		SetObjectCull (  t.importerCollision[t.importer.collisionShapeCount].object2,0 );
		PositionObject (  t.importerGridObject[9], 0 , 0 , 0 );
		t.importerCollision[t.importer.collisionShapeCount].rotx = 0;
		t.importerCollision[t.importer.collisionShapeCount].roty = 0;
		t.importerCollision[t.importer.collisionShapeCount].rotz = 0;
		GlueObjectToLimbEx (  t.importerCollision[t.importer.collisionShapeCount].object2,t.importerGridObject[9], 0 , 1 );
		DisableObjectZDepth (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		SetObjectWireframe (  t.importerCollision[t.importer.collisionShapeCount].object2 , 1 );
		SetObjectLight (  t.importerCollision[t.importer.collisionShapeCount].object2,0 );
		HideObject (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		SetObjectEffect ( t.importerCollision[t.importer.collisionShapeCount].object2, g.guiwireframeshadereffectindex );
		SetObjectEmissive ( t.importerCollision[t.importer.collisionShapeCount].object2, Rgb(255,255,255) );

		PositionObject (  t.importerGridObject[9], 0 , 0 , IMPORTERZPOSITION );

		t.importer.selectedCollisionObject = t.importer.collisionShapeCount;
		t.slidersmenuvalue[t.importer.properties2Index][7].value = 0;
		t.slidersmenuvalue[t.importer.properties2Index][8].value = 0;
		t.slidersmenuvalue[t.importer.properties2Index][9].value = 0;
		++t.importer.collisionShapeCount;
	}
}

void importer_dupe_collision_box ( void )
{
	if (bRemoveSprites)
		return;
	t.tScale = t.importer.objectScaleForEditing;
	if (  t.importer.selectedCollisionObject  ==  -1  )  return;
	if (  t.importer.collisionShapeCount < 49 ) 
	{
		t.importerCollision[t.importer.collisionShapeCount].object = findFreeObject();
		MakeObjectBox (  t.importerCollision[t.importer.collisionShapeCount].object , 100 , 100 , 100 );
		ScaleLimb (  t.importerCollision[t.importer.collisionShapeCount].object, 0, LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object,0) ,  LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object,0),  LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object,0) );
		t.importerCollision[t.importer.collisionShapeCount].sizex = LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object,0);
		t.importerCollision[t.importer.collisionShapeCount].sizey = LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object,0);
		t.importerCollision[t.importer.collisionShapeCount].sizez = LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object,0);
		LockObjectOn (  t.importerCollision[t.importer.collisionShapeCount].object );
		PositionObject (  t.importerCollision[t.importer.collisionShapeCount].object , ObjectPositionX(t.importerCollision[t.importer.selectedCollisionObject].object) ,  ObjectPositionY(t.importerCollision[t.importer.selectedCollisionObject].object),  ObjectPositionZ(t.importerCollision[t.importer.selectedCollisionObject].object) );
		PositionObject (  t.importerGridObject[9], 0 , 0 , 0 );
		t.importerCollision[t.importer.collisionShapeCount].rotx = ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object2);
		t.importerCollision[t.importer.collisionShapeCount].roty = ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object2);
		t.importerCollision[t.importer.collisionShapeCount].rotz = ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object2);
		GlueObjectToLimbEx (  t.importerCollision[t.importer.collisionShapeCount].object,t.importerGridObject[9], 0 , 1 );
		DisableObjectZDepth (  t.importerCollision[t.importer.collisionShapeCount].object );
		SetObjectLight (  t.importerCollision[t.importer.collisionShapeCount].object,0 );
		HideObject (  t.importerCollision[t.importer.collisionShapeCount].object );
		SetObjectEffect ( t.importerCollision[t.importer.collisionShapeCount].object, g.guishadereffectindex );
		SetObjectEmissive ( t.importerCollision[t.importer.collisionShapeCount].object, Rgb(255,255,255) );
		SetAlphaMappingOn ( t.importerCollision[t.importer.collisionShapeCount].object, 10 );

		t.importerCollision[t.importer.collisionShapeCount].object2 = findFreeObject();
		MakeObjectBox (  t.importerCollision[t.importer.collisionShapeCount].object2 , 100,100,100 );
		ScaleLimb (  t.importerCollision[t.importer.collisionShapeCount].object2, 0, LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object,0) ,  LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object,0),  LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object,0) );
		LockObjectOn (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		PositionObject (  t.importerCollision[t.importer.collisionShapeCount].object2 , ObjectPositionX(t.importerCollision[t.importer.selectedCollisionObject].object) ,  ObjectPositionY(t.importerCollision[t.importer.selectedCollisionObject].object),  ObjectPositionZ(t.importerCollision[t.importer.selectedCollisionObject].object) );
		SetObjectCull (  t.importerCollision[t.importer.collisionShapeCount].object,0 );
		SetObjectCull (  t.importerCollision[t.importer.collisionShapeCount].object2,0 );

		PositionObject (  t.importerGridObject[9], 0 , 0 , 0 );
		t.importerCollision[t.importer.collisionShapeCount].rotx = ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object2);
		t.importerCollision[t.importer.collisionShapeCount].roty = ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object2);
		t.importerCollision[t.importer.collisionShapeCount].rotz = ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object2);
		RotateObject (   t.importerCollision[t.importer.collisionShapeCount].object, ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object2),ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object2),ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object2) );
		RotateObject (   t.importerCollision[t.importer.collisionShapeCount].object2, ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object2),ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object2),ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object2) );
		GlueObjectToLimbEx (  t.importerCollision[t.importer.collisionShapeCount].object2,t.importerGridObject[9], 0 , 1 );
		DisableObjectZDepth (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		SetObjectWireframe (  t.importerCollision[t.importer.collisionShapeCount].object2 , 1 );
		SetObjectLight (  t.importerCollision[t.importer.collisionShapeCount].object2,0 );
		HideObject (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		SetObjectEffect ( t.importerCollision[t.importer.collisionShapeCount].object2, g.guiwireframeshadereffectindex );
		SetObjectEmissive ( t.importerCollision[t.importer.collisionShapeCount].object2, Rgb(255,255,255) );

		PositionObject (  t.importerGridObject[9], 0 , 0 , IMPORTERZPOSITION );
		t.importer.selectedCollisionObject = t.importer.collisionShapeCount;
		t.slidersmenuvalue[t.importer.properties2Index][7].value = t.importerCollision[t.importer.collisionShapeCount].rotx;
		t.slidersmenuvalue[t.importer.properties2Index][8].value = t.importerCollision[t.importer.collisionShapeCount].roty;
		t.slidersmenuvalue[t.importer.properties2Index][9].value = t.importerCollision[t.importer.collisionShapeCount].rotz;
		++t.importer.collisionShapeCount;
	}
}

void importer_add_collision_box_loaded ( void )
{
	if (bRemoveSprites)
		return;

	if (  t.importer.collisionShapeCount < 49 ) 
	{
		t.importerCollision[t.importer.collisionShapeCount].object = findFreeObject();
		MakeObjectBox (  t.importerCollision[t.importer.collisionShapeCount].object , 100 , 100 , 100 );
		ScaleLimb (  t.importerCollision[t.importer.collisionShapeCount].object, 0, t.tPSizeX_f * (t.tScale / 100.0) , t.tPSizeY_f * (t.tScale / 100.0) , t.tPSizeZ_f * (t.tScale / 100.0) );
		t.importerCollision[t.importer.collisionShapeCount].sizex = t.tPSizeX_f * (t.tScale / 100.0);
		t.importerCollision[t.importer.collisionShapeCount].sizey = t.tPSizeY_f * (t.tScale / 100.0);
		t.importerCollision[t.importer.collisionShapeCount].sizez = t.tPSizeZ_f * (t.tScale / 100.0);
		LockObjectOn (  t.importerCollision[t.importer.collisionShapeCount].object );
		PositionObject (  t.importerCollision[t.importer.collisionShapeCount].object, t.tPOffX_f * (t.tScale / 100.0) , (t.tPOffY_f * (t.tScale / 100.0)) , t.tPOffZ_f * (t.tScale / 100.0) );
		RotateObject (  t.importerCollision[t.importer.collisionShapeCount].object, t.tPRotX_f , t.tPRotY_f , t.tPRotZ_f );
		SetObjectEffect ( t.importerCollision[t.importer.collisionShapeCount].object, g.guishadereffectindex );
		SetObjectEmissive ( t.importerCollision[t.importer.collisionShapeCount].object, Rgb(255,255,255) );
		SetAlphaMappingOn ( t.importerCollision[t.importer.collisionShapeCount].object, 10 );

		PositionObject (  t.importerGridObject[9], 0 , 0 , 0 );
		t.importerCollision[t.importer.collisionShapeCount].rotx = t.tPRotX_f;
		t.importerCollision[t.importer.collisionShapeCount].roty = t.tPRotY_f;
		t.importerCollision[t.importer.collisionShapeCount].rotz = t.tPRotZ_f;

		GlueObjectToLimbEx (  t.importerCollision[t.importer.collisionShapeCount].object,t.importerGridObject[9], 0 , 1 );
		DisableObjectZDepth (  t.importerCollision[t.importer.collisionShapeCount].object );
		SetObjectLight (  t.importerCollision[t.importer.collisionShapeCount].object,0 );
		HideObject (  t.importerCollision[t.importer.collisionShapeCount].object );
		t.importerCollision[t.importer.collisionShapeCount].object2 = findFreeObject();
		MakeObjectBox (  t.importerCollision[t.importer.collisionShapeCount].object2 , 100,100,100 );
		ScaleLimb (  t.importerCollision[t.importer.collisionShapeCount].object2, 0, t.tPSizeX_f * (t.tScale / 100.0) , t.tPSizeY_f * (t.tScale / 100.0) , t.tPSizeZ_f * (t.tScale / 100.0) );
		LockObjectOn (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		PositionObject (  t.importerCollision[t.importer.collisionShapeCount].object2, t.tPOffX_f * (t.tScale / 100.0) , (t.tPOffY_f * (t.tScale / 100.0)) , t.tPOffZ_f * (t.tScale / 100.0) );
		RotateObject (  t.importerCollision[t.importer.collisionShapeCount].object2, t.tPRotX_f , t.tPRotY_f , t.tPRotZ_f );
		SetObjectCull (  t.importerCollision[t.importer.collisionShapeCount].object,0 );
		SetObjectCull (  t.importerCollision[t.importer.collisionShapeCount].object2,0 );
		SetObjectEffect ( t.importerCollision[t.importer.collisionShapeCount].object2, g.guiwireframeshadereffectindex );
		SetObjectEmissive ( t.importerCollision[t.importer.collisionShapeCount].object2, Rgb(255,255,255) );

		PositionObject (  t.importerGridObject[9], 0 , 0 , 0 );
		t.importerCollision[t.importer.collisionShapeCount].rotx = t.tPRotX_f;
		t.importerCollision[t.importer.collisionShapeCount].roty = t.tPRotY_f;
		t.importerCollision[t.importer.collisionShapeCount].rotz = t.tPRotZ_f;
		GlueObjectToLimbEx (  t.importerCollision[t.importer.collisionShapeCount].object2,t.importerGridObject[9], 0 , 1 );
		DisableObjectZDepth (  t.importerCollision[t.importer.collisionShapeCount].object2 );
		SetObjectWireframe (  t.importerCollision[t.importer.collisionShapeCount].object2 , 1 );
		SetObjectLight (  t.importerCollision[t.importer.collisionShapeCount].object2,0 );
		HideObject (  t.importerCollision[t.importer.collisionShapeCount].object2 );

		PositionObject (  t.importerGridObject[9], 0 , 0 , IMPORTERZPOSITION );

		t.importer.selectedCollisionObject = t.importer.collisionShapeCount;
		t.slidersmenuvalue[t.importer.properties2Index][7].value = t.tPRotX_f;
		t.slidersmenuvalue[t.importer.properties2Index][8].value = t.tPRotY_f;
		t.slidersmenuvalue[t.importer.properties2Index][9].value = t.tPRotZ_f;
		++t.importer.collisionShapeCount;
	}
}

