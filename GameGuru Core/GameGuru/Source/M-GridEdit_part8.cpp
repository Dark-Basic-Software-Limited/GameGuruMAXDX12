void gridedit_recreateentitycursor ( void )
{
	int ele_id = 0;
	//  Entity floating selection
	if ( t.gridentityobj>0 ) 
	{
		//  character creator remove glued objects
		if (  t.toldCursorEntidForCharacterCreator > 0 ) 
		{
			if (  t.entityprofile[t.toldCursorEntidForCharacterCreator].ischaractercreator  ==  1 ) 
			{
				t.tccobj = g.charactercreatorrmodelsoffset+((t.toldCursorEntidForCharacterCreator*3)-t.characterkitcontrol.bankOffset);
				if (  ObjectExist(t.tccobj) == 1 ) 
				{
					UnGlueObject (  g.charactercreatorrmodelsoffset+((t.toldCursorEntidForCharacterCreator*3)-t.characterkitcontrol.bankOffset)+1 );
					UnGlueObject (  g.charactercreatorrmodelsoffset+((t.toldCursorEntidForCharacterCreator*3)-t.characterkitcontrol.bankOffset)+2 );
					UnGlueObject (  t.tccobj );
				}
			}
		}
		if ( ObjectExist(t.gridentityobj) == 1  ) DeleteObject (  t.gridentityobj );
		t.gridentityobj=0;
		t.toldCursorEntidForCharacterCreator = 0;
	}
	if (  t.gridentity>0 ) 
	{
		t.obj=g.entityviewcursorobj;
		t.sourceobj=g.entitybankoffset+t.gridentity;
		if (ObjectExist(t.sourceobj) == 1)
		{
			WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
			if (t.gridentityextractedindex > 0) 
			{
				if (t.gridentityextractedindex > 0 && t.gridentityextractedindex <= ArrayCount(t.entityelement))
				{
					ele_id = t.gridentityextractedindex;
				}
			}
			if (ele_id == 0 && t.tentitytoselect > 0 && t.tentitytoselect <= ArrayCount(t.entityelement) )
			{
				ele_id = t.tentitytoselect;
			}
			WickedSetEntityId(t.gridentity);
			WickedSetElementId(ele_id);
			t.entid=t.gridentity ; t.entobj=t.obj;
			if ( t.entityprofile[t.entid].ischaracter == 1 || t.entityprofile[t.entid].ismarker != 0 || t.entityprofile[t.entid].animmax>0 ) 
			{
				//  Close allows animation independence
				CloneObject ( t.obj, t.sourceobj );

				//  Character creator head
				if ( t.entityprofile[t.entid].ischaractercreator == 1 ) 
				{
					t.toldCursorEntidForCharacterCreator = t.entid;
					t.tSourcebip01_head=getlimbbyname(t.obj, "Bip01_Head");
					if ( t.tSourcebip01_head > 0 ) 
					{
						t.tccobj = g.charactercreatorrmodelsoffset+((t.entid*3)-t.characterkitcontrol.bankOffset);
						if ( ObjectExist(t.tccobj) == 1 ) 
						{
							t.tBip01_FacialHair=getlimbbyname(t.tccobj, "Bip01_FacialHair");
							if ( t.tBip01_FacialHair > 0  )  GlueObjectToLimbEx (  g.charactercreatorrmodelsoffset+((t.entid*3)-t.characterkitcontrol.bankOffset)+1,t.tccobj,t.tBip01_FacialHair,2 );
							t.Bip01_Headgear=getlimbbyname(t.tccobj, "Bip01_Headgear");
							if ( t.Bip01_Headgear > 0  )  GlueObjectToLimbEx (  g.charactercreatorrmodelsoffset+((t.entid*3)-t.characterkitcontrol.bankOffset)+2,t.tccobj,t.Bip01_Headgear,2 );
							GlueObjectToLimbEx (  t.tccobj,t.obj,t.tSourcebip01_head,2 );
						}
					}
				}
			}
			else
			{
				//  Instance creation cheaper
				InstanceObject (  t.obj,t.sourceobj );
			}
			WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
			WickedSetEntityId(-1);
			WickedSetElementId(0);
			t.gridentityunderground=0;

			//LB: incorporate overrideanimset into object creation step (during editing/loading/etc)
			if (t.obj > 0)
			{
				LPSTR pOverrideAnimSet = t.entityelement[ele_id].eleprof.overrideanimset_s.Get();
				if (strlen(pOverrideAnimSet) > 1) // "" = default to weapon type, "-" = default to object anim
				{
					// replace actual object animations
					if (FileExist(pOverrideAnimSet) == 1)
					{
						sObject* pObject = GetObjectData(t.obj);
						AppendObject(pOverrideAnimSet, t.obj, 0);
						WickedCall_RefreshObjectAnimations(pObject, pObject->wickedloaderstateptr);
					}
				}
			}

			// other entity attributes
			if ( t.entityprofile[t.entid].ismarker != 0 && t.entityprofile[t.entid].ismarker != 11 ) //Allow cullmode on 11
			{
				// special setup for marker objects
				SetObjectTransparency ( t.obj, 2 );
				SetObjectCull ( t.obj, 1 );
				sObject* pObject = g_ObjectList[t.obj];
				if (pObject)
				{
					WickedCall_TextureObject(pObject, NULL);
				}
			}
			else
			{
				// For Wicked, cull mode controlled per-mesh with parent default as normal
				//PE: Prefer WEMaterial over old cullmode
				bool bUseWEMaterial = false;
				if (t.entityprofile[t.entid].WEMaterial.MaterialActive)
				{
					WickedSetEntityId(t.entid);
					if(ele_id  > 0)
						WickedSetElementId(ele_id);
					else
						WickedSetElementId(0);
					sObject* pObject = g_ObjectList[t.obj];
					if (pObject)
					{
						bUseWEMaterial = true;
						for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
						{
							sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
							if (pMesh)
							{
								// set properties of mesh
								WickedSetMeshNumber(iMeshIndex);
								bool bDoubleSided = WickedDoubleSided();
								if (bDoubleSided)
								{
									pMesh->bCull = false;
									pMesh->iCullMode = 0;
									WickedCall_SetMeshCullmode(pMesh);
								}
								else
								{
									pMesh->iCullMode = 1;
									pMesh->bCull = true;
									WickedCall_SetMeshCullmode(pMesh);
								}
							}
						}
					}
					WickedSetEntityId(-1);
				}

				if (!bUseWEMaterial)
					SetObjectCull(t.obj, 1);
				//  set transparency mode
				if (t.entityprofile[t.entid].islightmarker == 1)
				{
					sObject* pObject = g_ObjectList[t.obj];
					if (pObject)
						WickedCall_SetObjectCastShadows(pObject, false);
					t.entityprofile[t.entid].castshadow = -1;
				}
				if ((ele_id > 0) || (ele_id==0 && t.gridentity>0))
				{
					//PE: Wicked material can overwrite objects settings.
					WickedSetEntityId(t.gridentity);
					WickedSetElementId(ele_id);
					// LB: apply WEMaterial to all meshes of this object, not just the first one
					// LB: Setting object transparency defaults here (so not everything is transparent), but the TextureMesh can then set per-mesh transparency :)
					SetObjectTransparency(t.obj, t.entityelement[ele_id].eleprof.WEMaterial.bTransparency[0]);
					sObject* pObject = g_ObjectList[t.obj];
					for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
					{
						sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
						if (pMesh)
						{
							// set properties of mesh
							WickedSetMeshNumber(iMeshIndex);

							// sets ALL properties of each mesh from WEMaterial
							WickedCall_TextureMesh(pMesh);

							// and must restore mesh transparency flag
							bool bTransparent = WickedGetTransparent();
							pMesh->bTransparency = bTransparent;
						}
					}
					if (t.obj == 70000) 
					{
						//Update mesh materials.
						sObject* pObject = g_ObjectList[t.obj];
						if (pObject)
						{
							for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
							{
								WickedSetMeshNumber(iMesh);
								WickedCall_SetMeshMaterial(pObject->ppMeshList[iMesh], false);
							}
						}
					}		
					WickedSetEntityId(-1);
					WickedSetElementId(0);
				}
				else
				{
					if (t.entityprofile[t.entid].transparency >= 0)
					{
						WickedSetEntityId(t.gridentity);
						WickedSetElementId(ele_id);
						SetObjectTransparency(t.obj, t.entityprofile[t.entid].transparency);
						WickedSetEntityId(-1);
						WickedSetElementId(0);
					}
				}
				// 051115 - only if not using limb visibility for hiding decal arrow
				if ( t.entityprofile[t.entid].addhandlelimb==0 )
				{
					//  set LOD attributes for entities
					entity_calculateentityLODdistances ( t.entid, t.obj, 0 );
				}
			}
			if (t.obj > 0 && GetNumberOfFrames(t.obj)>0 )
			{
				SetObjectFrame (  t.obj,0 );
				if (  t.entityprofile[t.entid].animmax>0 && t.entityprofile[t.entid].playanimineditor>0 && t.entityprofile[t.entid].ischaractercreator == 0 ) 
				{
					t.q=t.entityprofile[t.entid].playanimineditor-1;
					LoopObject (  t.obj,t.entityanim[t.entid][t.q].start,t.entityanim[t.entid][t.q].finish );
				}
				else if (t.entityprofile[t.entid].playanimineditor < 0)
				{
					// uses name instead of index, the negative is the ordinal into the animset
					extern void entity_loop_using_negative_playanimineditor(int e, int obj, cstr animname);
					entity_loop_using_negative_playanimineditor(ele_id, t.obj, t.entityprofile[t.entid].playanimineditor_name);
				}
				else
				{
					LoopObject (  t.obj  ); StopObject (  t.obj );
				}
			}
		}
		else
		{
			MakeObjectCube (  t.obj,25 );
		}
		//  ensure new object ONLY interacts with main camera and shadow camera
		//PE: 130217 added t.entityprofile[t.gridentity].zdepth == 0 to prevent decals from calling technique11 DepthMap
		if (  t.entityprofile[t.gridentity].ismarker != 0 || t.entityprofile[t.gridentity].zdepth == 0 )
		{
			SetObjectMask (  t.obj, 1 );
		}
		else
		{
			SetObjectMask (  t.obj, 1+(1<<31) );
		}
		//  pivot alignment
		if (  t.entityprofile[t.gridentity].fixnewy != 0 ) 
		{
			RotateObject (  t.obj,0,t.entityprofile[t.gridentity].fixnewy,0 );
			FixObjectPivot (  t.obj );
		}
		// scale
		t.tescale=t.entityprofile[t.gridentity].scale;
		if (t.tescale > 0)
		{
			ScaleObject (t.obj, t.tescale, t.tescale, t.tescale);
		}

		SetObjectCollisionOff (  t.obj );
		if ( g.entityrubberbandlist.size() == 1 )
		{
			if (!pref.iEnableDragDropEntityMode)
			{
				// only dehighlight if single extract, not if a rubber band / linked entities extraction
				t.geditorhighlightingtentityID = ele_id;
				t.geditorhighlightingtentityobj = t.obj;
				WickedSetEntityId(t.gridentity);
				WickedSetElementId(ele_id);
				editor_restoreobjhighlightifnotrubberbanded(t.obj);
				WickedSetEntityId(-1);
				WickedSetElementId(0);
			}
		}
		t.gridentityobj=t.obj;

		if (t.entityprofile[t.gridentity].ismarker == 2)
		{
			if (ele_id > 0)
			{
				// full light update inc. color
				entity_updatelightobj(ele_id, t.gridentityobj);
			}
			else
			{
				// do not know light settings, so just white point light
				entity_updatelightobjtype(t.gridentityobj, 0);
			}
		}

		if (t.entityprofile[t.gridentity].islightmarker == 1)
		{
			sObject* pObject = g_ObjectList[t.obj];
			if (pObject)
				WickedCall_SetObjectCastShadows(pObject, false);
			t.entityprofile[t.gridentity].castshadow = -1;
		}
		
		cstr sEffectLower = Lower(t.entityprofile[t.gridentity].effect_s.Get());
		if (sEffectLower == "effectbank\\reloaded\\decal_animate1_additive.fx")
		{
			//PE: AvengingEagle's Light Effects.
			DisableObjectZWrite(t.obj); //Additive blending.
			void WickedCall_SetObjectBlendMode(sObject * pObject, int iBlendmode);
			sObject* pObject = g_ObjectList[t.obj];
			if (pObject)
				WickedCall_SetObjectBlendMode(pObject, BLENDMODE_ADDITIVE);
			t.entityprofile[t.gridentity].blendmode = BLENDMODE_ADDITIVE;
			if (ele_id > 0)
				t.entityelement[ele_id].eleprof.blendmode = BLENDMODE_ADDITIVE;
			for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
			{
				if (pObject->ppMeshList[iMesh]) pObject->ppMeshList[iMesh]->iCullMode = 0;
			}
			WickedCall_SetObjectCullmode(pObject);

		}

		//PE: Old decal support.
		if (t.entityprofile[t.gridentity].bIsDecal)
		{
			SetupDecalObject(t.obj, ele_id);
		}
	}

	//HighLight instantly.
	if (t.gridentityobj > 0) 
	{
		if (t.gridentityobj < g_iObjectListCount)
		{
			if (g_ObjectList[t.gridentityobj])
			{
				if (t.gridentitystaticmode)
					g_selected_editor_color = XMSTATICCOLOR;
				else
					g_selected_editor_color = XMDYNAMICCOLOR;
				g_selected_editor_object = g_ObjectList[t.gridentityobj];
				g_selected_editor_objectID = t.gridentityobj;
			}
		}
	}

	editor_refreshentitycursor ( );
}

void gridedit_displayentitycursor ( void )
{
	//  create entity foating selection
	if (  t.gridentity>0 ) 
	{
		t.obj=t.gridentityobj;
		if (  ObjectExist(t.obj) == 1 ) 
		{
			PositionObject (  t.obj,t.gridentityposx_f,t.gridentityposy_f,t.gridentityposz_f );
			RotateObject (  t.obj,t.gridentityrotatex_f,t.gridentityrotatey_f,t.gridentityrotatez_f );
			t.tfinalscalex_f=t.gridentityscalex_f;
			t.tfinalscaley_f=t.gridentityscaley_f;
			t.tfinalscalez_f=t.gridentityscalez_f;
			ScaleObject ( t.obj, t.tfinalscalex_f, t.tfinalscaley_f, t.tfinalscalez_f );
			if (  t.gridentity>0 )
			{
				if (  t.entityprofile[t.gridentity].ischaracter == 0 ) 
				{
					t.tanimspeed_f=t.entityprofile[t.gridentity].animspeed;
					SetObjectSpeed (  t.obj,g.timeelapsed_f*t.tanimspeed_f );
				}
			}
		}
	}

	//  if entity cursor light, instantly feed into shader OVERRIDING LIGHT ZERO
	lighting_override ( );
}

void gridedit_deletelevelobjects ( void )
{
	// clear OBJ values in entityelements (as all objects are being removed)
	if ( g.entityelementlist>0 ) 
	{
		for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
		{
			// delete any env probes 
			int entid = t.entityelement[t.e].bankindex;
			if (entid > 0)
			{
				if (t.entityprofile[entid].ismarker == 2)
				{
					entity_deleteprobe(t.entityelement[t.e].obj);
				}
			}

			t.obj = t.entityelement[t.e].obj;
			if ( t.obj > 0 ) 
			{
				if ( ObjectExist(t.obj) == 1 ) DeleteObject (  t.obj );
			}

			t.entityelement[t.e].obj=0;
			t.entityelement[t.e].bankindex=0;
			deleteinternalsound(t.entityelement[t.e].soundset) ; t.entityelement[t.e].soundset = 0;
			deleteinternalsound(t.entityelement[t.e].soundset1) ; t.entityelement[t.e].soundset1 = 0;
			deleteinternalsound(t.entityelement[t.e].soundset2) ; t.entityelement[t.e].soundset2 = 0;
			deleteinternalsound(t.entityelement[t.e].soundset3) ; t.entityelement[t.e].soundset3 = 0;
			deleteinternalsound(t.entityelement[t.e].soundset4) ; t.entityelement[t.e].soundset4 = 0;
			deleteinternalsound(t.entityelement[t.e].soundset5); t.entityelement[t.e].soundset5 = 0;
			deleteinternalsound(t.entityelement[t.e].soundset6); t.entityelement[t.e].soundset6 = 0;
		}
	}
	UnDim (  t.entityelement );
	g.entityelementmax=100;
	Dim (  t.entityelement,g.entityelementmax  );
	g.entityelementlist=0;

	//  delete all objects used for level edit
	for ( t.obj = g.entityviewstartobj ; t.obj <= g.entityviewendobj; t.obj++ )
	{
		if (  ObjectExist(t.obj) == 1  )  DeleteObject (  t.obj );
	}

	//  also delete all entitybank references
	entity_deletebank ( );

	//  Indicate no level objects
	g.entityviewendobj=0;

	//  270215 - 011 - Create new entities from the beginning
	g.entityviewcurrentobj=g.entityviewstartobj;
}

float GetCurveDistanceScaler(void)
{
	return g.globals.CurveDistanceScaler;
}

void modifyplaneimagestrip ( int objno, int texmax, int texindex )
{
	float s_f = 0;
	float u_f = 0;

	//  Lock the vertex data of the object
	LockVertexDataForLimbCore (  objno,0,1 );

	//  adjust UV data
	s_f=1.0/texmax ; u_f=texindex*s_f;
	SetVertexDataUV (  0,u_f+s_f,0.0 );
	SetVertexDataUV (  1,u_f,0.0 );
	SetVertexDataUV (  2,u_f+s_f,1.0 );
	SetVertexDataUV (  3,u_f,0.0 );
	SetVertexDataUV (  4,u_f,1.0 );
	SetVertexDataUV (  5,u_f+s_f,1.0 );

	//  Unlock the vertex data of the object
	UnlockVertexData (  );
}

int Get_Spray_Mode_On(void)
{
	if (iDisplayCircleFrames > 0)
	{
		iDisplayCircleFrames--;
		return(true);
	}
	if (t.gridentity == 0) return false;
	return(t.gridedit.entityspraymode);
}

void init_readouts()
{
	extern std::vector<std::string> readoutTitles;
	extern std::vector<STORYBOARD_WIDGET_> readoutWidgetTypes;
	extern std::vector<ReadoutLayers> readoutLayers;
	extern std::vector<ReadoutTypes> readoutTypes;
	extern std::vector<std::function<void()>> readoutCallbacks;

	readoutTitles.push_back("User Defined Global");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("User Defined Global Text");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_STRING);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("User Defined Global Statusbar");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_BAR);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("User Defined Global Image");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_IMAGE);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("User Defined Global Panel");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_IMAGE);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Health Remaining");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Maximum Health");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Health Panel");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_IMAGE);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Ammo Remaining");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Maximum Ammo");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Ammo Panel");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_IMAGE);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Weapon Held");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_IMAGE);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Weapon Firemode");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_IMAGE);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);

	readoutTitles.push_back("Lives Remaining");
	readoutWidgetTypes.push_back(STORYBOARD_WIDGET_TEXT);
	readoutLayers.push_back(READOUT_GAMEPLAY);
	readoutTypes.push_back(READOUT_INT);
	readoutCallbacks.push_back(nullptr);
}


void display_profiler_data(ImDrawList* draw, char* filter,int startline)
{
	extern ImFont* customfont;
	if (!customfont) return;
	float wide = 300.0f;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 window_pos = ImVec2((viewport->Pos.x + viewport->Size.x - 10.0f), (viewport->Pos.y + 10.0f));

	bool bProfile = true;
	bProfilerEnable = true;
	wiProfiler::SetEnabled(true);
	std::string profiler_data = wiProfiler::GetProfilerDataFilter(filter);
	float line = startline;
	char* find = (char*)pestrcasestr(profiler_data.c_str(), "\n");
	while (find)
	{
		char * find2 = (char*) pestrcasestr(find + 1, "\n");
		char old = '\n';
		if (find2)
		{
			old = find2[0];
			find2[0] = 0;
		}
		draw->AddText(customfont, 15, ImVec2(window_pos.x - wide - 2, viewport->Pos.y + 24.0 + (14.0f * line) - 2), IM_COL32(0, 0, 0, 255), find + 1);
		draw->AddText(customfont, 15, ImVec2(window_pos.x - wide, viewport->Pos.y + 24.0 + (14.0f * line)), IM_COL32(255, 255, 255, 255), find + 1);
		line++;
		if (find2)
		{
			find2[0] = old;
		}
		find = (char*)pestrcasestr(find + 1, "\n");
	}
}

int GetDrawCallsShadowsCube2(void)
{
	return(wiProfiler::GetDrawCallsShadowsCube());
}

int GetWidgetMode(void)
{
	return t.widget.mode;
}
int GetEntityGridMode(void)
{
	return t.gridentitygridlock;
}
int GetEntitySelected(void)
{
	return t.widget.pickedEntityIndex;
}
int GetEntityObject(int iEntityIndex)
{
	return(t.entityelement[iEntityIndex].obj);
}
void GetEntityPosition(int iEntityIndex, float & x, float& y, float& z)
{
	x = t.entityelement[iEntityIndex].x;
	y = t.entityelement[iEntityIndex].y;
	z = t.entityelement[iEntityIndex].z;
}
int GetRubberbandSize(void)
{
	return(g.entityrubberbandlist.size());
}
void SetWidgetMode(int mode)
{
	bool bWidgetEnabled = pref.iEnableDragDropWidgetSelect;
	switch (mode)
	{
		case 0:
		{
			//pref.iEnableDragDropWidgetSelect = true
			if (bWidgetEnabled)
			{
				t.widget.mode = 0;
				widget_show_widget();
			}
		} break;
		case 1:
		{
			if (bWidgetEnabled)
			{
				t.widget.mode = 1;
				widget_show_widget();
			}
		} break;
		case 2:
		{
			if (bWidgetEnabled)
			{
				// Don't allow characters and markers to be scaled with the widget
				if (t.widget.pickedEntityIndex > 0 && t.widget.pickedEntityIndex < t.entityelement.size())
				{
					int entid = t.entityelement[t.widget.pickedEntityIndex].bankindex;
					if (entid > 0)
					{
						bool bAllowObjectsAndParticlesToScale = false;
						if (t.entityprofile[entid].ismarker == 0) bAllowObjectsAndParticlesToScale = true;
						if (t.entityprofile[entid].ismarker == 10) bAllowObjectsAndParticlesToScale = true;
						if (t.entityprofile[entid].ischaracter == 0 && bAllowObjectsAndParticlesToScale == true)
						{
							t.widget.mode = 2;
							widget_show_widget();
						}
					}
				}
			}
		} break;
		case 3:
		{
			pref.iEnableDragDropWidgetSelect = !pref.iEnableDragDropWidgetSelect;
			if (pref.iEnableDragDropWidgetSelect)
				widget_show_widget();
			else
				widget_hide();
		} break;
		case 4:
		{
			//PE: Toggle snap.
			if (pref.iGridMode == 1)
				pref.iGridMode = 0;
			else
				pref.iGridMode = 1;
			t.gridentitygridlock = pref.iGridMode;
		} break;
		case 5:
		{
			if (pref.iGridMode!=2)
			{
				// grid on
				pref.iGridEnabled = true;
				pref.iGridMode = 2; //PE: Set grid mode to 2.
			}
			else
			{
				// grid off
				pref.iGridEnabled = false;
				pref.iGridMode = 0; //PE: Set grid mode to 0.
			}
			t.gridentitygridlock = pref.iGridMode;
		} break;
	}
}
void GridPopup(ImVec2 wpos)
{
	static bool bPopupOpen = false;
	if(bPopupOpen && wpos.x != 0)
		ImGui::SetNextWindowPos(wpos);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);
	if (ImGui::BeginPopup("Grid##GridSettings", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		bPopupOpen = true;
		bool bButSpacer = true;
		const float button_width_fix = 5.0f;
		ImGui::Indent(10);
		int iEntityIndex = t.widget.pickedEntityIndex;
		int iActiveObj = t.widget.activeObject;
		if (t.gridentityextractedindex > 0)
		{
			iEntityIndex = t.gridentityextractedindex;
			if (t.gridentityobj > 0)
				iActiveObj = t.gridentityobj;
		}
		else
		{
			if (t.widget.activeObject == 0 && t.widget.pickedEntityIndex < t.entityelement.size())
			{
				if (t.widget.pickedEntityIndex > 0)
					iActiveObj = t.tentityobj = t.entityelement[t.widget.pickedEntityIndex].obj;
			}
		}

		float but_gadget_size = ImGui::GetFontSize() * 14.0;
		ImGui::ItemSize(ImVec2(ImGui::GetFontSize() * 15.0, 0));
		ImGui::SetWindowFontScale(1.1);
		ImGui::TextCenter("Grid and Alignment Settings");
		ImGui::SetWindowFontScale(1.0);

		// grid size only available in advanced mode
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		if (1)//pref.iObjectEnableAdvanced)
		{
			if (1)//t.gridentitygridlock == 2)
			{
				if (pref.iAdvancedGridModeSettings == 0)
				{
					// Simple Grid Mode
					ImGui::TextCenter("Grid Size");
					float w = ImGui::GetContentRegionAvail().x;
					float inputsize = w / 4.0f;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w/2)-(inputsize/2), 0.0f));
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridsizeXYZ", &pref.fEditorGridSizeX, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size");
					ImGui::PopItemWidth();

					// can never have a grid size below one
					if (pref.fEditorGridSizeX <= 1) pref.fEditorGridSizeX = 1.0f;

					// and all grid dimensions the same!
					pref.fEditorGridOffsetX = 0;
					pref.fEditorGridOffsetY = 0;
					pref.fEditorGridOffsetZ = 0;
					pref.fEditorGridSizeY = pref.fEditorGridSizeX;
					pref.fEditorGridSizeZ = pref.fEditorGridSizeX;
				}
				else
				{
					// Advanced Grid Mode functions and settings
					ImGui::TextCenter("Grid Offset");
					float w = ImGui::GetContentRegionAvail().x;
					float inputsize = w / 3.0f;
					inputsize -= 10.0f; //For text.
					inputsize -= 5.0f; //For padding.
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
					ImGui::Text("X");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridoffsetX", &pref.fEditorGridOffsetX, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Offset X");
					ImGui::PopItemWidth();
					ImGui::SameLine();
					ImGui::Text("Y");
					ImGui::SameLine();
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridoffsetY", &pref.fEditorGridOffsetY, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Offset Y");
					ImGui::PopItemWidth();
					ImGui::SameLine();
					ImGui::Text("Z");
					ImGui::SameLine();
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridoffsetZ", &pref.fEditorGridOffsetZ, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Offset Z");
					ImGui::PopItemWidth();

					ImGui::TextCenter("Grid Size");
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
					ImGui::Text("X");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
					ImGui::SameLine();
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridsizeX", &pref.fEditorGridSizeX, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size X");
					ImGui::PopItemWidth();
					ImGui::SameLine();
					ImGui::Text("Y");
					ImGui::SameLine();
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridsizeY", &pref.fEditorGridSizeY, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size Y");
					ImGui::PopItemWidth();
					ImGui::SameLine();
					ImGui::Text("Z");
					ImGui::SameLine();
					ImGui::PushItemWidth(inputsize - ImGui::GetFontSize());
					ImGui::InputFloat("##XYZgridsizeZ", &pref.fEditorGridSizeZ, 0.0f, 0.0f, "%.1f");
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size Z");
					ImGui::PopItemWidth();

					bButSpacer = false;
					ImGui::Text("");
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - ((but_gadget_size * 0.5) + button_width_fix), 0.0f));
					if (ImGui::StyleButton("Default Grid Settings", ImVec2(but_gadget_size, 0)))
					{
						pref.fEditorGridOffsetX = 50;
						pref.fEditorGridOffsetY = 0;
						pref.fEditorGridOffsetZ = 50;
						pref.fEditorGridSizeX = 100;
						pref.fEditorGridSizeY = 10;
						pref.fEditorGridSizeZ = 100;
					}

					// clever button to align grid to object (for older levels with arbitary alignments mixed together)
					if (iEntityIndex > 0 && g.entityrubberbandlist.size() == 0)
					{
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - ((but_gadget_size * 0.5) + button_width_fix), 0.0f));
						if (ImGui::StyleButton("Align Grid Offset To Object", ImVec2(but_gadget_size, 0)))
						{
							float x = t.entityelement[iEntityIndex].x;
							float y = t.entityelement[iEntityIndex].y;
							float z = t.entityelement[iEntityIndex].z;
							int iSizeRoundedX = int(x / pref.fEditorGridSizeX) * pref.fEditorGridSizeX;
							pref.fEditorGridOffsetX = x - iSizeRoundedX;
							int iSizeRoundedY = int(y / pref.fEditorGridSizeY) * pref.fEditorGridSizeY;
							pref.fEditorGridOffsetY = y - iSizeRoundedY;
							int iSizeRoundedZ = int(z / pref.fEditorGridSizeZ) * pref.fEditorGridSizeZ;
							pref.fEditorGridOffsetZ = z - iSizeRoundedZ;
						}
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - ((but_gadget_size * 0.5) + button_width_fix), 0.0f));
						if (ImGui::StyleButton("Align Grid Size To Object", ImVec2(but_gadget_size, 0)))
						{
							float sx = ObjectSizeX(t.entityelement[iEntityIndex].obj, 1);
							float sy = ObjectSizeY(t.entityelement[iEntityIndex].obj, 1);
							float sz = ObjectSizeZ(t.entityelement[iEntityIndex].obj, 1);
							pref.fEditorGridSizeX = sx;
							pref.fEditorGridSizeY = sy;
							pref.fEditorGridSizeZ = sz;
						}
					}

					// can never have a grid size below one
					if (pref.fEditorGridSizeX <= 1) pref.fEditorGridSizeX = 1.0f;
					if (pref.fEditorGridSizeY <= 1) pref.fEditorGridSizeY = 1.0f;
					if (pref.fEditorGridSizeZ <= 1) pref.fEditorGridSizeZ = 1.0f;
				}
			}
		}

		if (vEntityLockedList.size() > 0)
		{
			if(bButSpacer)
				ImGui::Text("");
			float w = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - ((but_gadget_size * 0.5) + button_width_fix), 0.0f));
			cStr unlockstr = cStr("Unlock ") + cStr((int)vEntityLockedList.size()) + cStr(" Objects");
			if (ImGui::StyleButton(unlockstr.Get(), ImVec2(but_gadget_size, 0)))
			{
				for (int i = 0; i < vEntityLockedList.size(); i++)
				{
					int e = vEntityLockedList[i].e;
					if (e > 0 && e < t.entityelement.size())
					{
						t.entityelement[e].editorlock = 0;
						sObject* pObject;
						if (t.entityelement[e].obj > 0)
						{
							if (t.entityelement[e].obj < g_iObjectListCount)
							{
								pObject = g_ObjectList[t.entityelement[e].obj];
								if (pObject)
								{
									WickedCall_SetObjectRenderLayer(pObject, GGRENDERLAYERS_NORMAL);
								}
							}
						}
					}
				}
				vEntityLockedList.clear();

				// any lock/unlock operations resets, avoids issue of duplcating a static object and unable to 'move' it
				t.widget.pickedObject = 0;
			}
		}

		ImGui::Text("");
		ImGui::Indent(-10);
		ImGui::EndPopup();
	}
	else
		bPopupOpen = false;
	ImGui::PopStyleVar(1);

}

bool GetEnableEmptyLevelMode(void)
{
	return t.visuals.bEnableEmptyLevelMode;
}

float GetEnvProbeBrightness(void)
{
	return t.visuals.fEnvProbeBrightness;
}

void GetConvertSettings(int *maxwidth,int *active)
{
	*active = g.globals.ConvertToDDS;
	*maxwidth = g.globals.ConvertToDDSMaxSize;
}

int GetActiveEditorObject( void )
{
	int iActiveObj = t.widget.activeObject;
	if (t.gridentityextractedindex > 0)
	{
		if (t.gridentityobj > 0)
			iActiveObj = t.gridentityobj;
	}
	else
	{
		int iEntityIndex = t.widget.pickedEntityIndex;
		if (t.widget.activeObject == 0 && t.widget.pickedEntityIndex < t.entityelement.size())
		{
			if (t.widget.pickedEntityIndex > 0)
				iActiveObj = t.entityelement[t.widget.pickedEntityIndex].obj;
			else
			{
				iActiveObj = t.widget.activeObject;
			}
		}
	}
	return(iActiveObj);
}
int GetActiveEditorEntityPos(float *x, float *y, float *z, float* xa, float* ya, float* za)
{
	int iActiveObj = t.widget.activeObject;
	int iEntityIndex = 0;
	if (t.gridentityextractedindex > 0)
	{
		iEntityIndex = t.gridentityextractedindex;
	}
	else
	{
		iEntityIndex = t.widget.pickedEntityIndex;
	}
	if (iEntityIndex > 0 && iEntityIndex < t.entityelement.size())
	{
		if (x)
		{
			*x = t.entityelement[iEntityIndex].x;
			*y = t.entityelement[iEntityIndex].y;
			*z = t.entityelement[iEntityIndex].z;
		}
		if (xa)
		{
			*xa = t.entityelement[iEntityIndex].rx;
			*ya = t.entityelement[iEntityIndex].ry;
			*za = t.entityelement[iEntityIndex].rz;
		}
	}
	return(iEntityIndex);
}

int GetActiveEditorEntity(void)
{
	int iActiveObj = t.widget.activeObject;
	int iEntityIndex = 0;
	if (t.gridentityextractedindex > 0)
	{
		iEntityIndex = t.gridentityextractedindex;
	}
	else
	{
		iEntityIndex = t.widget.pickedEntityIndex;
	}
	return(iEntityIndex);
}

void EmptyMessages(void)
{
	if (g.globals.DisableMessagePump > 0)
		return;

	//PE: Empty messages , so windows dont think we are dead. ( perhaps remember QUIT ? )
	MSG msg = { 0 };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		//PE: No WM_QUIT while saving/loading levels.
		if (msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
