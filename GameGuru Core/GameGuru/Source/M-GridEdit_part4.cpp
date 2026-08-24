void editor_init ( void )
{
	image_setlegacyimageloading(true);
	//  Load editor images
	SetMipmapNum(1); //PE: mipmaps not needed.
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\quick-help.png";
	LoadImage (  t.strwork.Get(), g.editorimagesoffset+1 );
	LoadImage (  "editors\\gfx\\memorymeter.png",g.editorimagesoffset+2 );
	LoadImage (  "editors\\gfx\\4.png",g.editorimagesoffset+3 );
	LoadImage (  "editors\\gfx\\5.png",g.editorimagesoffset+4 );
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\gurumeditation.png";
	LoadImage (  t.strwork.Get() ,g.editorimagesoffset+5 );
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\gurumeditationoff.png";
	LoadImage ( t.strwork.Get() ,g.editorimagesoffset+6 );

	// +7 reserved (below)

	// Test Game prompt
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\quick-start-testlevel-prompt.png";
	LoadImage ( t.strwork.Get(), g.editorimagesoffset+61 );

	// Cursor for entity highlighting
	LoadImage ( "editors\\gfx\\9.png",g.editorimagesoffset+7 );
	LoadImage ( "editors\\gfx\\13.png",g.editorimagesoffset+13 );

	LoadImage ( "editors\\gfx\\14-white.png",g.editorimagesoffset+14 );
	if (!GetImageExistEx(g.editorimagesoffset + 14))
		LoadImage("editors\\gfx\\14.png", g.editorimagesoffset + 14);
	LoadImage ( "editors\\gfx\\14-red.png",g.editorimagesoffset+16 );
	LoadImage ( "editors\\gfx\\14-green.png",g.editorimagesoffset+17 );

	LoadImage (  "editors\\gfx\\26.png",g.editorimagesoffset+26 );
	LoadImage ( "editors\\gfx\\cursor.dds",g.editorimagesoffset+10 );

	//  F9 Edit Mode Graphical Prompts
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\f9-help-terrain.png";
	LoadImage ( t.strwork.Get() ,g.editorimagesoffset+21 );
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\f9-help-entity.png";
	LoadImage ( t.strwork.Get() ,g.editorimagesoffset+22 );
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\f9-help-conkit.png";
	LoadImage ( t.strwork.Get() ,g.editorimagesoffset+23 );

	// new images for editor extra help
	image_setlegacyimageloading(true);
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\testgamelayout-noweapons.png";
 	LoadImage (  t.strwork.Get(), g.editorimagesoffset+27 );
	t.strwork = ""; t.strwork = t.strwork + "languagebank\\"+g.language_s+"\\artwork\\testgamelayout-vr.png";
 	LoadImage (  t.strwork.Get(), g.editorimagesoffset+28 );
	image_setlegacyimageloading(false);

	//  Also loaded by interactive mode when active
	///LoadImage (  "languagebank\\neutral\\gamecore\\huds\\interactive\\close-highlight.png",g.interactiveimageoffset+15 );

	//  for overlays on map editor view
	if (  FileExist("editors\\gfx\\resources.png") == 1 ) 
	{
		LoadImage (  "editors\\gfx\\resources.png",g.editordrawlastimagesoffset+1 );
	}
	if (  FileExist("editors\\gfx\\resourceslow.png") == 1 ) 
	{
		LoadImage (  "editors\\gfx\\resourceslow.png",g.editordrawlastimagesoffset+2 );
	}
	if (  FileExist("editors\\gfx\\resourcesgone.png") == 1 ) 
	{
		LoadImage (  "editors\\gfx\\resourcesgone.png",g.editordrawlastimagesoffset+3 );
	}
	if (  FileExist("editors\\gfx\\resourcesworking.png") == 1 ) 
	{
		LoadImage (  "editors\\gfx\\resourcesworking.png",g.editordrawlastimagesoffset+4 );
	}
	SetMipmapNum(-1);
	image_setlegacyimageloading(false);

	//  Work area entity cursor (placeholder for instance of target expanded by 1.05 to make shell highligher)
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);

	MakeObjectPlane (  t.editor.objectstartindex+5,150,150  ); 
	XRotateObject (  t.editor.objectstartindex+5,90 );
	TextureObject (  t.editor.objectstartindex+5,g.editorimagesoffset+7 );
	SetObjectMask (  t.editor.objectstartindex+5, 1 );
	SetObjectTransparency (  t.editor.objectstartindex+5,2 );
	modifyplaneimagestrip(5,8,1);
	SetObjectCollisionOff (  t.editor.objectstartindex+5 );
	SetObjectLight (  t.editor.objectstartindex+5,0 );
	HideObject (  t.editor.objectstartindex+5 );
	OffsetLimb (  t.editor.objectstartindex+5,0,0,0,-1 );
	SetObjectEffect ( t.editor.objectstartindex+5, g.guishadereffectindex );

	//  cylinder to indicate resources in editor used (and warning)
	MakeObjectCylinder (  t.editor.objectstartindex+7,50 );
	SetObjectCollisionOff (  t.editor.objectstartindex+7 );
	SetObjectLight (  t.editor.objectstartindex+7,0 );
	DisableObjectZDepth (  t.editor.objectstartindex+7 );
	DisableObjectZRead (  t.editor.objectstartindex+7 );
	TextureObject (  t.editor.objectstartindex+7,g.editordrawlastimagesoffset+1 );
	LockObjectOn (  t.editor.objectstartindex+7 );
	ScaleObject (  t.editor.objectstartindex+7,2,0,2 );
	RotateObject (  t.editor.objectstartindex+7,0,0,90 );
	PositionObject (  t.editor.objectstartindex+7,0,-117.5,200 );
	SetObjectMask (  t.editor.objectstartindex+7, 1 );

	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);

	//  Setup camera
	BackdropColor (  Rgb(0,0,0) );
	// `set camera range 10,10000 now set in _editor_overallfunctionality

	SetLightRange (  0,10000 );
	SetAmbientLight (  75 );
	SetCameraFOV(45); //PE: default 45 in wicked.
	SetAutoCamOff (  );

	//  PositionCamera (  )
	t.gridscale_f=((800/2)/8)/t.gridzoom_f;
	t.workareax=800 ; t.workareay=600;
	t.borderx_f=1024.0*50.0 ; t.cx_f=GGORIGIN_X;
	t.bordery_f=1024.0*50.0 ; t.cy_f=GGORIGIN_Z;
	editor_restoreeditcamera ( );
}

void editor_makeundergroundobj ( void )
{
	//  takes tobj,tobjx#,tobjy#,tobjz#
	t.tobjoffx_f=GetObjectCollisionCenterX(t.tobj);
	t.tobjoffy_f=GetObjectCollisionCenterY(t.tobj);
	t.tobjoffz_f=GetObjectCollisionCenterZ(t.tobj);
	t.tobjsizex_f=ObjectSizeX(t.tobj,1);
	t.tobjsizey_f=ObjectSizeY(t.tobj,1);
	t.tobjsizez_f=ObjectSizeZ(t.tobj,1);
	DeleteObject ( t.tobj );
	MakeObjectBox ( t.tobj,t.tobjsizex_f,t.tobjsizey_f,t.tobjsizez_f );
	//ColorObject ( t.tobj,Rgb(255,255,0) );
	PositionObject ( t.tobj,t.tobjx_f,t.tobjy_f,t.tobjz_f );
	OffsetLimb ( t.tobj,0,t.tobjoffx_f,t.tobjoffy_f,t.tobjoffz_f );
	DisableObjectZRead ( t.tobj );
	SetObjectWireframe ( t.tobj,1 );
	SetObjectMask ( t.tobj, 1 );
	SetObjectEffect ( t.tobj, g.guiwireframeshadereffectindex );
	SetObjectEmissive ( t.tobj, Rgb(255,255,0) );
}

void editor_restoreobjhighlightifnotrubberbanded ( int highlightingtentityobj )
{
	if ( highlightingtentityobj>0 ) 
	{
		if ( ObjectExist(highlightingtentityobj) == 1 ) 
		{
			bool bHighlightedFromRubberBandSelection = false;
			if ( g.entityrubberbandlist.size() > 0 )
			{
				for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
				{
					int e = g.entityrubberbandlist[i].e;
					int tobj = t.entityelement[e].obj;
					if ( highlightingtentityobj == tobj )
						bHighlightedFromRubberBandSelection = true;
				}
			}
			if ( bHighlightedFromRubberBandSelection == false )
			{
				// dehighlight primary highlighted entity
				bool bValid = true;
				if (t.geditorhighlightingtentityID > 0)
				{
					int mi = t.entityelement[t.geditorhighlightingtentityID].bankindex;
					if (mi > 0 && t.entityprofile[mi].bIsDecal) bValid = false;
				}
				if (bValid)
				{
					SetAlphaMappingOn (highlightingtentityobj, 100);
				}
				// and also dehighlight any children that may have been highlighted as well
				if ( t.tstoreentityindexofprimaryhightlighted > 0 )
				{
					gridedit_clearentityrubberbandlist();
					t.tstoreentityindexofprimaryhightlighted = 0;
				}
				//PE: SetAlphaMappingOn will overwrite basecolor.w
				//PE: Restore org colors.
				sObject* pObject = g_ObjectList[highlightingtentityobj];
				if (pObject)
				{
					for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
					{
						WickedSetMeshNumber(iMesh);
						WickedCall_SetMeshMaterial(pObject->ppMeshList[iMesh], false);
					}
				}
				if (!bValid)
				{
					//Setup decal.
					SetupDecalObject(highlightingtentityobj, t.geditorhighlightingtentityID);
				}
			}
		}
	}
}

void editor_restoreentityhighlightobj ( void )
{
	if ( t.geditorhighlightingtentityobj>0 ) 
	{
		if (t.geditorhighlightingtentityID > 0)
		{
			WickedSetEntityId(t.entityelement[t.geditorhighlightingtentityID].bankindex);
			WickedSetElementId(t.geditorhighlightingtentityID);
		}
		editor_restoreobjhighlightifnotrubberbanded ( t.geditorhighlightingtentityobj );
		if (t.geditorhighlightingtentityID > 0)
		{
			WickedSetEntityId(-1);
			WickedSetElementId(0);
		}
		t.geditorhighlightingtentityobj=0;
	}
}

void editor_rec_checkifindexinparentchain ( int entityindex, bool* pbPartOfParentChildGroup )
{
	for ( int te = 1; te <= g.entityelementlist; te++ )
	{
		if ( t.entityelement[te].iHasParentIndex == entityindex && t.entityelement[te].obj > 0 )
		{
			*pbPartOfParentChildGroup = true;
			editor_rec_checkifindexinparentchain ( te, pbPartOfParentChildGroup );
		}
	}
}

void editor_rec_addchildrentorubberband ( int entityindex )
{
	for ( int te = 1; te <= g.entityelementlist; te++ )
	{
		if ( t.entityelement[te].iHasParentIndex == entityindex && t.entityelement[te].obj > 0 )
		{
			gridedit_addEntityToRubberBandHighlights ( te );
			editor_rec_addchildrentorubberband ( te );
		}
	}
}

void editor_refreshentitycursor ( void )
{
	//  new highligher for selected entities
	t.tentityobj=0 ; t.tentstaticmode=0;
	if ( t.gridentityobj>0 ) 
	{
		t.tentityobj=t.gridentityobj;
		t.tentstaticmode=t.gridentitystaticmode;
	}
	else
	{
		if ( t.tentitytoselect>0 ) 
		{
			if ( t.tentitytoselect <= ArrayCount(t.entityelement) ) 
			{
				t.tentityobj=t.entityelement[t.tentitytoselect].obj;
				t.tentstaticmode=t.entityelement[t.tentitytoselect].staticflag;
				t.ttentid=t.entityelement[t.tentitytoselect].bankindex;
				if ( t.entityprofile[t.ttentid].ismarker == 0 && t.entityprofile[t.ttentid].isebe == 0 ) 
				{
					if ( t.entityelement[t.tentitytoselect].isclone == 1 && t.entityelement[t.tentitytoselect].underground == 0 ) 
					{
						if ( t.entityelement[t.tentitytoselect].editorlock == 0 ) 
						{
							//  restore clone back to instance if no more entity lock
							t.tobj=t.tentityobj ; t.tte=t.tentitytoselect;
							entity_converttoinstance ( );
						}
					}
				}
			}
		}
	}
	if ( t.tentityobj>0 ) 
	{
		if ( ObjectExist(t.tentityobj) == 1 ) 
		{
			// do not reset if extracted and draggging parent/children around
			if ( t.gridentityextractedindex == 0 )
			{
				t.geditorhighlightingtentityID = t.tentitytoselect;
				t.geditorhighlightingtentityobj = t.tentityobj;
				editor_restoreentityhighlightobj();
			}

			// if obj is instance, and using entity_basic shader, this sets GlowIntensity constant
			int iAlphaHighlightCode = 100;
			if ( t.tentstaticmode == 0 ) 
			{
				if ( t.gridedit.autoflatten == 1 && t.gridentityobj>0 ) 
					iAlphaHighlightCode = 104;
				else
					iAlphaHighlightCode = 103;
			}
			else
			{
				if ( t.gridedit.autoflatten == 1 && t.gridentityobj>0 ) 
					iAlphaHighlightCode = 102;
				else
					iAlphaHighlightCode = 101;
			}
			SetAlphaMappingOn ( t.tentityobj, iAlphaHighlightCode );

			//PE: This overwrite out wicked basecolor.w

			// check if this entity is a parent to children, and highlight them too
			if ( t.tentitytoselect > 0 ) 
			{
				t.tstoreentityindexofprimaryhightlighted = t.tentitytoselect;
				editor_rec_addchildrentorubberband ( t.tentitytoselect );
			}

			// record primary entity object being highlighted
			t.geditorhighlightingtentityobj = t.tentityobj;
		}
	}
	else
	{
		if (t.geditorhighlightingtentityobj > 0)
		{
			int foundit = 0;
			for (int i = 1; i < t.entityelement.size(); i++)
			{
				if (t.entityelement[i].obj == t.geditorhighlightingtentityobj)
				{
					t.geditorhighlightingtentityID = i;
					editor_restoreentityhighlightobj();
					break;
				}
			}
		}
	}
}

void editor_hideall3d ( void )
{
	SetCurrentCamera (  0 );
	PositionCamera (  199999,99999,99999 );
	PointCamera (  199999,100999,99999 );
	if ( gbWelcomeSystemActive == false ) 
	{
		Sync ( ); Sync ( );
	}
}

void editor_restoreeditcamera ( void )
{
	// editor starting camera position - reset camera
	SetCurrentCamera (  0 );
	PositionCamera ( t.cx_f, 600*t.gridzoom_f, t.cy_f );
	PointCamera ( t.cx_f, 0, t.cy_f );
	SetCameraFOV(45); //PE: default 45 in wicked.
}

void editor_clearlibrary ( void )
{
	// Delete all libraries
	for ( t.tabs = 1; t.tabs <= 3; t.tabs++ )
	{
		SetFileMapDWORD ( 1, 534, t.tabs );
		SetFileMapDWORD ( 1, 542, 1 );
		SetEventAndWait ( 1 );
		while ( GetFileMapDWORD ( 1, 542 ) == 1 ) 
		{
			SetEventAndWait ( 1 );
		}
	}

	// ENTITY TAB
	t.tadd=1;

	// And create default NEW icons
	t.t1_s=t.strarr_s[347] ; t.t2_s="files\\editors\\gfx\\missing.bmp";
	SetFileMapDWORD ( 1, 508, t.tadd );
	SetFileMapString ( 1, 1000, cstr(g.mysystem.root_s+t.t2_s).Get() );
	SetFileMapString ( 1, 1256, t.t1_s.Get() );
	SetFileMapDWORD ( 1, 500, 1 );
	SetEventAndWait ( 1 );
	while ( GetFileMapDWORD(1, 500) == 1 ) 
	{
		SetEventAndWait ( 1 );
	}

	// MARKERS TAB
	t.tadd=2;

	//  Determine if extra ZONES included
	t.tstoryzoneincluded=25;
	if ( g.vrqcontrolmode != 0 )
		t.tstoryzoneincluded=23;
	// Default markers
	for ( t.tt = 0 ; t.tt <= t.tstoryzoneincluded; t.tt++ )
	{
		if ( g.vrqcontrolmode != 0 )
		{
			if (  t.tt == 0 ) { t.t1_s = t.strarr_s[349]  ; t.t2_s = "files\\entitybank\\_markers\\player start.bmp"; }
			if (  t.tt == 1 ) { t.t1_s = t.strarr_s[659]  ; t.t2_s = "files\\entitybank\\_markers\\multiplayer start.bmp"; }
			if (  t.tt == 2 ) { t.t1_s = t.strarr_s[351]  ; t.t2_s = "files\\entitybank\\_markers\\white light.bmp"; }
			if (  t.tt == 3 ) { t.t1_s = t.strarr_s[352]  ; t.t2_s = "files\\entitybank\\_markers\\red light.bmp"; }
			if (  t.tt == 4 ) { t.t1_s = t.strarr_s[353]  ; t.t2_s = "files\\entitybank\\_markers\\green light.bmp"; }
			if (  t.tt == 5 ) { t.t1_s = t.strarr_s[354]  ; t.t2_s = "files\\entitybank\\_markers\\blue light.bmp"; }
			if (  t.tt == 6 ) { t.t1_s = t.strarr_s[355]  ; t.t2_s = "files\\entitybank\\_markers\\yellow light.bmp"; }
			if (  t.tt == 7 ) { t.t1_s = t.strarr_s[356]  ; t.t2_s = "files\\entitybank\\_markers\\purple light.bmp"; }
			if (  t.tt == 8 ) { t.t1_s = t.strarr_s[357]  ; t.t2_s = "files\\entitybank\\_markers\\cyan light.bmp"; }
			if (  t.tt == 9 ) { t.t1_s = t.strarr_s[360]  ; t.t2_s = "files\\entitybank\\_markers\\win zone.bmp"; }
			if (  t.tt == 10 ) { t.t1_s = t.strarr_s[361]  ; t.t2_s = "files\\entitybank\\_markers\\trigger zone.bmp"; }
			if (  t.tt == 11 ) { t.t1_s = "Audio Zone"  ; t.t2_s = "files\\entitybank\\_markers\\audio zone.bmp"; }
			if (  t.tt == 12 ) { t.t1_s = "Video Zone"  ; t.t2_s = "files\\entitybank\\_markers\\video zone.bmp"; }
			if (  t.tt == 13 ) { t.t1_s = "Floor Zone"; t.t2_s = "files\\entitybank\\_markers\\floor zone.bmp"; }
			if (  t.tt == 14 ) { t.t1_s = "Image Zone"; t.t2_s = "files\\entitybank\\_markers\\image zone.bmp"; }
			if (  t.tt == 15 ) { t.t1_s = "Text Zone"; t.t2_s = "files\\entitybank\\_markers\\text zone.bmp"; }
			if (  t.tt == 16 ) { t.t1_s = "Ambience Zone"; t.t2_s = "files\\entitybank\\_markers\\ambience zone.bmp"; }
			if (  t.tt == 17 ) { t.t1_s = "White Spotlight"; t.t2_s = "files\\entitybank\\_markers\\white light spot.bmp"; }
			if (  t.tt == 18 ) { t.t1_s = "Red Spotlight"; t.t2_s = "files\\entitybank\\_markers\\red light spot.bmp"; }
			if (  t.tt == 19 ) { t.t1_s = "Green Spotlight"; t.t2_s = "files\\entitybank\\_markers\\green light spot.bmp"; }
			if (  t.tt == 20 ) { t.t1_s = "Blue Spotlight"; t.t2_s = "files\\entitybank\\_markers\\blue light spot.bmp"; }
			if (  t.tt == 21 ) { t.t1_s = "Yellow Spotlight"; t.t2_s = "files\\entitybank\\_markers\\yellow light spot.bmp"; }
			if (  t.tt == 22 ) { t.t1_s = "Purple Spotlight"; t.t2_s = "files\\entitybank\\_markers\\purple light spot.bmp"; }
			if (  t.tt == 23 ) { t.t1_s = "Cyan Spotlight"; t.t2_s = "files\\entitybank\\_markers\\cyan light spot.bmp"; }
		}
		else
		{
			if (  t.tt == 0 ) { t.t1_s = t.strarr_s[349]  ; t.t2_s = "files\\entitybank\\_markers\\player start.bmp"; }
			if (  t.tt == 1 ) { t.t1_s = t.strarr_s[350]  ; t.t2_s = "files\\entitybank\\_markers\\player checkpoint.bmp"; }
			if (  t.tt == 2 ) { t.t1_s = t.strarr_s[658]  ; t.t2_s = "files\\entitybank\\_markers\\cover zone.bmp"; }
			if (  t.tt == 3 ) { t.t1_s = t.strarr_s[659]  ; t.t2_s = "files\\entitybank\\_markers\\multiplayer start.bmp"; }
			if (  t.tt == 4 ) { t.t1_s = t.strarr_s[351]  ; t.t2_s = "files\\entitybank\\_markers\\white light.bmp"; }
			if (  t.tt == 5 ) { t.t1_s = t.strarr_s[352]  ; t.t2_s = "files\\entitybank\\_markers\\red light.bmp"; }
			if (  t.tt == 6 ) { t.t1_s = t.strarr_s[353]  ; t.t2_s = "files\\entitybank\\_markers\\green light.bmp"; }
			if (  t.tt == 7 ) { t.t1_s = t.strarr_s[354]  ; t.t2_s = "files\\entitybank\\_markers\\blue light.bmp"; }
			if (  t.tt == 8 ) { t.t1_s = t.strarr_s[355]  ; t.t2_s = "files\\entitybank\\_markers\\yellow light.bmp"; }
			if (  t.tt == 9 ) { t.t1_s = t.strarr_s[356]  ; t.t2_s = "files\\entitybank\\_markers\\purple light.bmp"; }
			if (  t.tt == 10 ) { t.t1_s = t.strarr_s[357]  ; t.t2_s = "files\\entitybank\\_markers\\cyan light.bmp"; }
			if (  t.tt == 11 ) { t.t1_s = t.strarr_s[360]  ; t.t2_s = "files\\entitybank\\_markers\\win zone.bmp"; }
			if (  t.tt == 12 ) { t.t1_s = t.strarr_s[361]  ; t.t2_s = "files\\entitybank\\_markers\\trigger zone.bmp"; }
			if (  t.tt == 13 ) { t.t1_s = t.strarr_s[362]  ; t.t2_s = "files\\entitybank\\_markers\\sound zone.bmp"; }
			if (  t.tt == 14 ) { t.t1_s = t.strarr_s[607]  ; t.t2_s = "files\\entitybank\\_markers\\story zone.bmp"; }
			if (  t.tt == 15 ) { t.t1_s = "Floor Zone"; t.t2_s = "files\\entitybank\\_markers\\floor zone.bmp"; }
			if (  t.tt == 16 ) { t.t1_s = "Image Zone"; t.t2_s = "files\\entitybank\\_markers\\image zone.bmp"; }
			if (  t.tt == 17 ) { t.t1_s = "Text Zone"; t.t2_s = "files\\entitybank\\_markers\\text zone.bmp"; }
			if (  t.tt == 18 ) { t.t1_s = "Ambience Zone"; t.t2_s = "files\\entitybank\\_markers\\ambience zone.bmp"; }
			if (  t.tt == 19 ) { t.t1_s = "White Spotlight"; t.t2_s = "files\\entitybank\\_markers\\white light spot.bmp"; }
			if (  t.tt == 20 ) { t.t1_s = "Red Spotlight"; t.t2_s = "files\\entitybank\\_markers\\red light spot.bmp"; }
			if (  t.tt == 21 ) { t.t1_s = "Green Spotlight"; t.t2_s = "files\\entitybank\\_markers\\green light spot.bmp"; }
			if (  t.tt == 22 ) { t.t1_s = "Blue Spotlight"; t.t2_s = "files\\entitybank\\_markers\\blue light spot.bmp"; }
			if (  t.tt == 23 ) { t.t1_s = "Yellow Spotlight"; t.t2_s = "files\\entitybank\\_markers\\yellow light spot.bmp"; }
			if (  t.tt == 24 ) { t.t1_s = "Purple Spotlight"; t.t2_s = "files\\entitybank\\_markers\\purple light spot.bmp"; }
			if (  t.tt == 25 ) { t.t1_s = "Cyan Spotlight"; t.t2_s = "files\\entitybank\\_markers\\cyan light spot.bmp"; }
		}
		SetFileMapDWORD (  1, 508, t.tadd );
		SetFileMapString (  1, 1000, cstr(g.mysystem.root_s+t.t2_s).Get() );
		SetFileMapString (  1, 1256, t.t1_s.Get() );
		SetFileMapDWORD (  1, 500, 1 );
		SetEventAndWait (  1 );
		while (  GetFileMapDWORD(1, 500) == 1 ) 
		{
			SetEventAndWait (  1 );
		}
	}

	//  actual entity names of the markers
	Dim ( t.markerentitybank_s, 30 );
	if ( g.vrqcontrolmode != 0 )
	{
		t.markerentitybank_s[1]="_markers\\player start.fpe";
		t.markerentitybank_s[2]="_markers\\multiplayer start.fpe";
		t.markerentitybank_s[3]="_markers\\white light.fpe";
		t.markerentitybank_s[4]="_markers\\red light.fpe";
		t.markerentitybank_s[5]="_markers\\green light.fpe";
		t.markerentitybank_s[6]="_markers\\blue light.fpe";
		t.markerentitybank_s[7]="_markers\\yellow light.fpe";
		t.markerentitybank_s[8]="_markers\\purple light.fpe";
		t.markerentitybank_s[9]="_markers\\cyan light.fpe";
		t.markerentitybank_s[10]="_markers\\win zone.fpe";
		t.markerentitybank_s[11]="_markers\\trigger zone.fpe";
		t.markerentitybank_s[12] = "_markers\\audio zone.fpe";
		t.markerentitybank_s[13] = "_markers\\video zone.fpe";
		t.markerentitybank_s[14] = "_markers\\floor zone.fpe";
		t.markerentitybank_s[15] = "_markers\\image zone.fpe";
		t.markerentitybank_s[16] = "_markers\\text zone.fpe";
		t.markerentitybank_s[17] = "_markers\\ambience zone.fpe";
		t.markerentitybank_s[18] = "_markers\\white light spot.fpe";
		t.markerentitybank_s[19] = "_markers\\red light spot.fpe";
		t.markerentitybank_s[20] = "_markers\\green light spot.fpe";
		t.markerentitybank_s[21] = "_markers\\blue light spot.fpe";
		t.markerentitybank_s[22] = "_markers\\yellow light spot.fpe";
		t.markerentitybank_s[23] = "_markers\\purple light spot.fpe";
		t.markerentitybank_s[24] = "_markers\\cyan light spot.fpe";
	}
	else
	{
		t.markerentitybank_s[1]="_markers\\player start.fpe";
		t.markerentitybank_s[2]="_markers\\player checkpoint.fpe";
		t.markerentitybank_s[3]="_markers\\cover zone.fpe";
		t.markerentitybank_s[4]="_markers\\multiplayer start.fpe";
		t.markerentitybank_s[5]="_markers\\white light.fpe";
		t.markerentitybank_s[6]="_markers\\red light.fpe";
		t.markerentitybank_s[7]="_markers\\green light.fpe";
		t.markerentitybank_s[8]="_markers\\blue light.fpe";
		t.markerentitybank_s[9]="_markers\\yellow light.fpe";
		t.markerentitybank_s[10]="_markers\\purple light.fpe";
		t.markerentitybank_s[11]="_markers\\cyan light.fpe";
		t.markerentitybank_s[12]="_markers\\win zone.fpe";
		t.markerentitybank_s[13]="_markers\\trigger zone.fpe";
		t.markerentitybank_s[14] = "_markers\\sound zone.fpe";
		t.markerentitybank_s[15] = "_markers\\story zone.fpe";
		t.markerentitybank_s[16] = "_markers\\floor zone.fpe";
		t.markerentitybank_s[17] = "_markers\\image zone.fpe";
		t.markerentitybank_s[18] = "_markers\\text zone.fpe";
		t.markerentitybank_s[19] = "_markers\\ambience zone.fpe";
		t.markerentitybank_s[20] = "_markers\\white light spot.fpe";
		t.markerentitybank_s[21] = "_markers\\red light spot.fpe";
		t.markerentitybank_s[22] = "_markers\\green light spot.fpe";
		t.markerentitybank_s[23] = "_markers\\blue light spot.fpe";
		t.markerentitybank_s[24] = "_markers\\yellow light spot.fpe";
		t.markerentitybank_s[25] = "_markers\\purple light spot.fpe";
		t.markerentitybank_s[26] = "_markers\\cyan light spot.fpe";
	}

	// only if EBE enabled
	if ( g.globals.hideebe == 0 )
	{
		// BUILDER TAB
		t.tadd=3;

		// set maximum to 999
		Dim ( t.ebebank_s, 999 );
		t.ebebankmax = 0;

		// Default builder tool icons
		for ( t.tt = 0; t.tt <= 6; t.tt++ )
		{
			if ( t.tt == 0 ) { t.t1_s = "Add New Site";		t.t2_s = "files\\ebebank\\_builder\\New Site.bmp"; }
			if ( t.tt == 1 ) { t.t1_s = "Cube";				t.t2_s = "files\\ebebank\\_builder\\Cube.bmp"; }
			if ( t.tt == 2 ) { t.t1_s = "Floor";			t.t2_s = "files\\ebebank\\_builder\\Floor.bmp"; }
			if ( t.tt == 3 ) { t.t1_s = "Wall";				t.t2_s = "files\\ebebank\\_builder\\Wall.bmp"; }
			if ( t.tt == 4 ) { t.t1_s = "Column";			t.t2_s = "files\\ebebank\\_builder\\Column.bmp"; }
			if ( t.tt == 5 ) { t.t1_s = "Row";				t.t2_s = "files\\ebebank\\_builder\\Row.bmp"; }
			if ( t.tt == 6 ) { t.t1_s = "Stairs";			t.t2_s = "files\\ebebank\\_builder\\Stairs.bmp"; }
			SetFileMapDWORD ( 1, 508, t.tadd );
			SetFileMapString ( 1, 1000, cstr(g.mysystem.root_s+t.t2_s).Get() );
			SetFileMapString ( 1, 1256, t.t1_s.Get() );
			SetFileMapDWORD ( 1, 500, 1 );
			SetEventAndWait ( 1 );
			while (  GetFileMapDWORD(1, 500) == 1 ) 
			{
				SetEventAndWait (  1 );
			}
		}
		t.ebebank_s[1]="..\\ebebank\\_builder\\New Site.fpe";
		t.ebebank_s[2]="ebebank\\_builder\\Cube.pfb";
		t.ebebank_s[3]="ebebank\\_builder\\Floor.pfb";
		t.ebebank_s[4]="ebebank\\_builder\\Wall.pfb";
		t.ebebank_s[5]="ebebank\\_builder\\Column.pfb";
		t.ebebank_s[6]="ebebank\\_builder\\Row.pfb";
		t.ebebank_s[7]="ebebank\\_builder\\Stairs.pfb";

		// Now scan for extra PFB files not part of default set
		int iFirstFreeSlot = 8;
		cstr pOld = GetDir();
		SetDir("ebebank");
		UnDim(t.filelist_s);
		buildfilelist("_builder", "");
		SetDir(pOld.Get());
		int iExtraPFBCount = 0;
		if (ArrayCount(t.filelist_s) > 0)
		{
			for (t.chkfile = 0; t.chkfile <= ArrayCount(t.filelist_s); t.chkfile++)
			{
				t.file_s = t.filelist_s[t.chkfile];
				if (t.file_s != "." && t.file_s != "..")
				{
					if (cstr(Lower(Right(t.file_s.Get(), 4))) == ".pfb")
					{
						// ignore items in default list
						bool bIgnore = false;
						for (int dl = 1; dl < iFirstFreeSlot; dl++)
						{
							LPSTR pThisOne = t.ebebank_s[dl].Get();
							char pNameOnly[256];
							strcpy(pNameOnly, "");
							for (int n = strlen(pThisOne) - 1; n > 0; n--)
							{
								if (pThisOne[n] == '\\' || pThisOne[n] == '/')
								{
									strcpy(pNameOnly, pThisOne + n + 1);
									break;
								}
							}
							if (stricmp(pNameOnly, t.file_s.Get()) == NULL)
								bIgnore = true;
						}
						if (bIgnore == false)
						{
							// add to list
							t.ebebank_s[iFirstFreeSlot + iExtraPFBCount] = cstr("ebebank\\_builder\\") + Left(t.file_s.Get(), Len(t.file_s.Get()));

							// next slot
							iExtraPFBCount++;
							if (iExtraPFBCount > 100) iExtraPFBCount = 100;
						}
					}
				}
			}
			t.strwork = ""; t.strwork = t.strwork + "total extra PFBs=" + Str(iExtraPFBCount);
			timestampactivity(0, t.strwork.Get());
		}
		//  Now sort list into alphabetical order
		for ( t.tgid1 = 0; t.tgid1 < iExtraPFBCount; t.tgid1++ )
		{
			for ( t.tgid2 = 0; t.tgid2 < iExtraPFBCount; t.tgid2++ )
			{
				if (  t.tgid1 != t.tgid2 ) 
				{
					t.tname1_s=Lower(t.ebebank_s[iFirstFreeSlot+t.tgid1].Get());
					t.tname2_s=Lower(t.ebebank_s[iFirstFreeSlot+t.tgid2].Get());
					if ( strlen( t.tname1_s.Get() ) > strlen( t.tname2_s.Get() ) ) 
					{
						//  smallest at top
						t.ebebank_s[iFirstFreeSlot+t.tgid1]=t.tname2_s;
						t.ebebank_s[iFirstFreeSlot+t.tgid2]=t.tname1_s;
					}
				}
			}
		}
		// add to library list
		for ( int n = 0; n < iExtraPFBCount; n++ )
		{
			// create BMP thumbnail
			t.file_s = t.ebebank_s[iFirstFreeSlot+n];
			LPSTR pThisOne = t.file_s.Get();
			char pNameOnly[256];
			strcpy ( pNameOnly, "" );
			for ( int n = strlen(pThisOne)-1; n > 0; n-- )
			{
				if ( pThisOne[n] == '\\' ||  pThisOne[n] == '/' )
				{
					strcpy ( pNameOnly, pThisOne + n + 1 );
					break;
				}
			}
			t.t1_s = Left(pNameOnly,Len(pNameOnly)-4);
			t.t2_s = cstr("files\\") + cstr(Left(t.file_s.Get(),Len(t.file_s.Get())-4)) + cstr(".bmp");
			SetFileMapDWORD ( 1, 508, t.tadd );
			SetFileMapString ( 1, 1000, cstr(g.mysystem.root_s+t.t2_s).Get() );
			SetFileMapString ( 1, 1256, t.t1_s.Get() );
			SetFileMapDWORD ( 1, 500, 1 );
			SetEventAndWait ( 1 );
			while (  GetFileMapDWORD(1, 500) == 1 ) 
			{
				SetEventAndWait (  1 );
			}
		}
		t.ebebankmax = 8 + iExtraPFBCount;
	}

	//  clear counters
	t.locallibraryentidmaster=0;
	t.locallibraryentindex=0;

	//  Ensure start with entity tab
	editor_leftpanelreset ( );
}

void editor_filllibrary ( void )
{
	//  Store place before adds
	SetEventAndWait (  1 );
	t.tstoredtabindex=GetFileMapDWORD( 1, 520 );

	//  Ensure entity list is up to date in library
	while ( t.locallibraryentidmaster<g.entidmaster ) 
	{
		//  only if not marker
		++t.locallibraryentidmaster;
		t.t2_s = t.entityprofileheader[t.locallibraryentidmaster].desc_s;

		// named EBE entities can be shown
		bool bShowEntityInLocalLibrary = true;
		if (t.entityprofile[t.locallibraryentidmaster].isebe != 0)
		{
			if ( stricmp ( t.t2_s.Get(), "new site" ) == NULL ) bShowEntityInLocalLibrary = false;
			if ( strnicmp ( t.t2_s.Get(), "ebe", 3 ) == NULL ) 
			{
				// are the characters after 'ebe' numbers?
				bool bIsNumber = false;
				LPSTR pEntName = t.t2_s.Get();
				for ( int n = 3; n < (int)strlen(pEntName); n++ )
				{
					if ( pEntName[n] >= '0' && pEntName[n] <= '9' )
						bIsNumber = true;
					else
					{
						bIsNumber = false;
						break;
					}
				}
				if ( bIsNumber == true )
				{
					bShowEntityInLocalLibrary = false;
				}
			}
			LPSTR pEntityBankFilename = t.entitybank_s[t.locallibraryentidmaster].Get();
			pEntityBankFilename += strlen(pEntityBankFilename)-4;
			if ( stricmp ( pEntityBankFilename, ".fpe" ) != NULL )
			{
				bShowEntityInLocalLibrary = false;
			}
		}
		if ( bShowEntityInLocalLibrary == true )
		{
			if ( t.entityprofile[t.locallibraryentidmaster].ismarker == 0 || t.entityprofile[t.locallibraryentidmaster].ismarker == 4 ) 
			{
				//  add to actual list
				t.ttext_s=t.entitybank_s[t.locallibraryentidmaster];
				t.tbitmap_s=cstr("files\\entitybank\\")+t.ttext_s;
				t.t1_s = ""; t.t1_s=t.t1_s + Left(t.tbitmap_s.Get(),Len(t.tbitmap_s.Get())-4)+".bmp";
				if (  FileExist( cstr(cstr("..\\")+t.t1_s).Get() ) == 0  )  t.t1_s = "files\\editors\\gfx\\missing.bmp";
				SetFileMapDWORD (  1, 508, 1 );
				SetFileMapString (  1, 1000, Left(cstr(g.mysystem.root_s+t.t1_s).Get(),254) );
				SetFileMapString (  1, 1256, Left(t.t2_s.Get(),254) );
				SetFileMapDWORD (  1, 500, 1 );
				SetEventAndWait (  1 );
				while (  GetFileMapDWORD(1, 500) == 1 ) 
				{
					SetEventAndWait (  1 );
				}

				//  add to internal list array
				++t.locallibraryentindex;
				Dim (  t.locallibraryent,t.locallibraryentindex  );
				t.locallibraryent[t.locallibraryentindex] = t.locallibraryentidmaster;
			}
		}
	}

	//  Restore place after adds
	SetFileMapDWORD (  1, 534, 1+t.tstoredtabindex );
	SetEventAndWait (  1 );
}

void editor_leftpanelreset ( void )
{
	// Reset to GetPoint ( to entity tab )
	SetFileMapDWORD (  1, 534, 1 );
	SetEventAndWait (  1 );
}

void editor_filemapdefaultinitfornew ( void )
{
	//  Open for some Defaults for Editor
	OpenFileMap (  1, "FPSEXCHANGE" );

	//  Marker Defaults
	g.entidmaster=0;

	//  filllibrary with segments and entities from default prefabs (temp as is above)
	editor_filllibrary ( );
	editor_leftpanelreset ( );
}

void editor_filemapinit ( void )
{
	// Open for some Defaults for Editor
	OpenFileMap (  1, "FPSEXCHANGE" );
	// Set default mouse position and visibility
	SetFileMapDWORD (  1, 0, 400 );
	SetFileMapDWORD (  1, 4, 300 );
	SetEventAndWait (  1 );

	//  Each selection tab needs a NEW icon
	editor_clearlibrary ( );
	editor_filllibrary ( );
	editor_leftpanelreset ( );
}

void editor_loadcfg (bool bFromFPM)
{
	// Load existing config file
	cstr cfgfile_s = g.mysystem.editorsGridedit_s + "cfg.cfg";
	if (bFromFPM)
	{
		cfgfile_s = g.mysystem.levelBankTestMap_s + "cfg.cfg";
		if (FileExist(cfgfile_s.Get()) == 0)
		{
			cfgfile_s = g.mysystem.editorsGridedit_s + "cfg.cfg";
		}
	}
	if ( FileExist(cfgfile_s.Get()) == 1 ) 
	{
		OpenToRead (  1,cfgfile_s.Get() );
		t.cx_f = ReadFloat ( 1 );
		t.cy_f = ReadFloat ( 1 );
		t.gridzoom_f = ReadFloat ( 1 );
		t.gridlayer = ReadLong ( 1 );
		t.nogridsmart = ReadLong ( 1 );
		t.grideditartmode = ReadLong ( 1 );

		// LB: modes 3 and 4 are view-modes (should not restore into these, no way out!!)
		int iTestGridEditSelect = ReadLong ( 1 );
		if (iTestGridEditSelect == 3 || iTestGridEditSelect == 4)
		{
			t.grideditselect = 0;
		}
		else
		{
			t.grideditselect = iTestGridEditSelect;
		}

		//  Project (only need project name if skipping FPM=using temp.fpm)
		t.temp_s = ReadString ( 1 ); if (  t.skipfpmloading == 1  )  g.projectfilename_s = t.temp_s;
		g.currentFPG_s = ReadString ( 1 );

		//  Shroud Settings
		t.a = ReadLong ( 1 );
		g.gridlayershowsingle = ReadLong ( 1 );

		//PE: Restore all camera settings.
		if (bFromFPM)
		{
			char *tmp;
			tmp = ReadString(1);
			if (tmp && tmp[0] == 'V' && tmp[1] == '2')
			{
				t.editorfreeflight.mode = ReadLong(1);
				float fTmp1 = ReadFloat(1);
				float fTmp2 = ReadFloat(1);
				float fTmp3 = ReadFloat(1);
				//PE: Double check if we have some default values and something is wrong.
				if (fTmp1 != 0.0 && fTmp2 != 0.0 && fTmp3 != 0.0)
				{
					t.editorfreeflight.c.x_f = fTmp1;
					t.editorfreeflight.c.y_f = fTmp2;
					t.editorfreeflight.c.z_f = fTmp3;
					t.cx_f = ReadFloat(1);
					t.cy_f = ReadFloat(1);
					t.editorfreeflight.c.angx_f = ReadFloat(1);
					t.editorfreeflight.c.angy_f = ReadFloat(1);
					//PE: Also update t.editorfreeflight.s if any camera animation is starting.
					t.editorfreeflight.s = t.editorfreeflight.c;
				}
			}
			//PE: In wicked after loading a new fpm. we need some frames before terrain height is ready.
			iDelayedCameraRestore = 240; //It can take some time before full terrain is regenerated, so 4 sec.
		}
		CloseFile (  1 );
	}

	//  Reset editor slicing for now
	g.gridlayershowsingle=0;

	//  Update editor settings
	editor_refresheditmarkers ( );
	t.refreshgrideditcursor=1;
	t.updatezoom=1;

	//  Current project name stored for next time
	t.currentprojectfilename_s=g.projectfilename_s;
	return;
}

void editor_savecfg (char *filename)
{
	// Delete config file
	t.strwork = g.mysystem.editorsGridedit_s + "cfg.cfg";
	if (filename)
	{
		t.strwork = filename;
	}

	if ( FileOpen(1) == 1) CloseFile(1); //PE: To make sure, was missing a cfg.cfg file in fpm so...
	if ( FileExist( t.strwork.Get() ) == 1  )  DeleteAFile ( t.strwork.Get() );

	//  Save config file
	OpenToWrite (  1, t.strwork.Get() );

	//  Current Camera Position
	if (  t.editorfreeflight.mode == 1 ) 
	{
		//  when save while in free flight mode, use present location
		t.a_f=t.editorfreeflight.c.x_f ; WriteFloat (  1,t.a_f );
		t.a_f=t.editorfreeflight.c.z_f ; WriteFloat (  1,t.a_f );
	}
	else
	{
		WriteFloat (  1,t.cx_f );
		WriteFloat (  1,t.cy_f );
	}
	WriteFloat (  1,t.gridzoom_f );
	WriteLong (  1,t.gridlayer );

	//  Edit Vars
	WriteLong (  1,t.nogridsmart );
	WriteLong (  1,t.grideditartmode );
	WriteLong (  1,t.grideditselect );

	//  Project
	WriteString (  1,g.projectfilename_s.Get() );
	WriteString (  1,g.currentFPG_s.Get() );

	//  Shroud Settings
	WriteLong (  1,g.shroudsize );
	WriteLong (  1,g.gridlayershowsingle );
	WriteString(1, "V2");

	//PE: Write out all camera settings.
	WriteLong(1, t.editorfreeflight.mode);
	WriteFloat(1, t.editorfreeflight.c.x_f);
	WriteFloat(1, t.editorfreeflight.c.y_f);
	WriteFloat(1, t.editorfreeflight.c.z_f);
	WriteFloat(1, t.cx_f);
	WriteFloat(1, t.cy_f);
	WriteFloat(1, t.editorfreeflight.c.angx_f);
	WriteFloat(1, t.editorfreeflight.c.angy_f);

	// finish
	CloseFile (  1 );
}

void editor_constructionselection ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	if ( t.inputsys.constructselection>0 )
	{
		//  SINGLE ENTITY
		if ( t.grideditselect == 5 ) 
		{
			if ( t.inputsys.constructselection <= g.entidmaster ) 
			{
				CloseDownEditorProperties();

				//  first cancel any widget that might be opened
				widget_switchoff ( );

				//PE: Somebody removed this line in Classic ???? Nothing worked ????
				t.gridentity = t.inputsys.constructselection;

				// use custom grideleprof if from smart object
				bool bEleProfFromSmartObject = false;

				// remove any entity group rubber band highlighting
				t.gridentity = t.inputsys.constructselection;
				int iFromGroupEntityID = 0;
				if (t.entityprofile[t.gridentity].groupreference != -1)
				{
					// look for group that matches the group object entity FPE name
					extern int GetGroupIndexFromName (cstr sLookFor);
					cstr sLookFor = cstr("entitybank\\") + t.entitybank_s[t.gridentity];
					int iParentGroupID = GetGroupIndexFromName(sLookFor);
					if (iParentGroupID != -1)
					{
						// and duplicate from that parent group
						iFromGroupEntityID = DuplicateFromListToCursor(vEntityGroupList[iParentGroupID], false, -1);

						//PE: Keep scale rot when setup from a group.
						if (iFromGroupEntityID > 0)
						{
							bRotScaleAlreadyUpdated = true;
							if (g.entityrubberbandlist.size() > 0)
							{
								int e = g.entityrubberbandlist[0].e;
								if (e > 0)
								{
									t.grideleprof = t.entityelement[e].eleprof;
									bEleProfFromSmartObject = true;
								}
							}
						}

						// smart object game elements are always hidden at first
						gridedit_setsmartobjectvisibilityinrubberband(false);
					}
					else
					{
						// strangely cannot find group for this parent smart object!!
						gridedit_clearentityrubberbandlist();
					}
				}
				else
				{
					// if not a group smart object
					gridedit_clearentityrubberbandlist();
				}

				iLastEntityOnCursor = 0;

				// the entity ID we are adding
				if ( t.entityprofile[t.gridentity].isebe > 0 )
				{
					// create unique entid and go to entity placement mode
					char pEBEFile[512];
					strcpy ( pEBEFile, t.entitybank_s[t.gridentity].Get());
					t.addentityfile_s = cstr(Left(pEBEFile,strlen(pEBEFile)-4)) + cstr(".fpe");

					CloseDownEditorProperties();

					// Work out EBE file and check if it exists
					char pFinalPathAndFile[1024];
					sprintf ( pFinalPathAndFile, "entitybank%s.ebe", Left(pEBEFile,strlen(pEBEFile)-4) );
					if ( FileExist ( pFinalPathAndFile ) )
					{
						// by creating one unique to the level, we can save our temp changes to it
						entity_adduniqueentity ( true );
						t.gridentity = t.entid;

						// name only
						char pNameOnly[256];
						strcpy ( pNameOnly, "" );
						for ( int n = strlen(pEBEFile)-1; n > 0; n-- )
						{
							if ( pEBEFile[n] == '\\' ||  pEBEFile[n] == '/' )
							{
								strcpy ( pNameOnly, pEBEFile + n + 1 );
								break;
							}
						}
						t.t1_s = Left(pNameOnly,Len(pNameOnly)-4);

						// give it a unique name
						t.entitybank_s[t.entid] = t.t1_s;
						t.entityprofileheader[t.entid].desc_s = t.t1_s;

						// load EBE data into entityID
						ebe_load_ebefile ( pFinalPathAndFile, t.entid );

						// get path only
						char pFinalPathOnly[1024];
						strcpy ( pFinalPathOnly, pFinalPathAndFile );
						for ( int n = strlen(pFinalPathAndFile); n > 0; n-- )
						{
							if ( pFinalPathAndFile[n] == '\\' || pFinalPathAndFile[n] == '/' )
							{
								pFinalPathOnly[n+1] = 0;
								break;
							}
						}

						// copy unique texture into levelbank\testmap so EDIT can copy over to ebebank
						cstr sUniqueFilename = t.entityprofile[t.entid].texd_s;
						sUniqueFilename = cstr(Left(sUniqueFilename.Get(),strlen(sUniqueFilename.Get())-6));
						cstr sDDSSourceFile = cstr(pFinalPathOnly) + sUniqueFilename + cstr("_D.dds");
						cstr sDDSFile = g.mysystem.levelBankTestMap_s + sUniqueFilename + cstr("_D.dds");
						if ( FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
						CopyFileA ( sDDSSourceFile.Get(), sDDSFile.Get(), FALSE );
						sDDSSourceFile = cstr(pFinalPathOnly) + sUniqueFilename + cstr("_N.dds");
						sDDSFile = g.mysystem.levelBankTestMap_s + sUniqueFilename + cstr("_N.dds");
						if ( FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
						CopyFileA ( sDDSSourceFile.Get(), sDDSFile.Get(), FALSE );
						sDDSSourceFile = cstr(pFinalPathOnly) + sUniqueFilename + cstr("_S.dds");
						sDDSFile = g.mysystem.levelBankTestMap_s + sUniqueFilename + cstr("_S.dds");
						if ( FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
						CopyFileA ( sDDSSourceFile.Get(), sDDSFile.Get(), FALSE );
					}
					else
					{
						// EBE not present, which means user protected it (not an editable EBE any more)
						if ( stricmp ( pEBEFile, "..\\ebebank\\_builder\\New Site.fpe" ) != NULL )
						{
							//New site is called EBE? 
							//if (!(pEBEFile[0] == 'E' && pEBEFile[1] == 'B' && pEBEFile[2] == 'E'))
							if(t.entityprofile[t.gridentity].model_s != "New Site.dbo" )  //Better way.
							{
								// except New Site of course
								t.entityprofile[t.gridentity].isebe = 0;
							}
						}
					}
				}
				//  select entity profile and start orientation
				t.gridedit.autoflatten=t.entityprofile[t.gridentity].autoflatten;
				t.inputsys.dragoffsetx_f=0;
				t.inputsys.dragoffsety_f=0;
				fHitPointX = 0;
				fHitPointY = HITPOINTYSTARTPOS;
				fHitPointZ = 0;
				fHitOffsetX = 0;
				fHitOffsetY = 0;
				fHitOffsetZ = 0;
				// LB: these can be uninitialised, but we need these filled so the plane can be under the cursor initially
				t.gridentityposx_f = t.inputsys.localx_f;
				t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f;
				t.gridentityposz_f = t.inputsys.localy_f;
				g_bHoldGridEntityPosWhenManaged = true;
				g_fHoldGridEntityPosX = t.gridentityposx_f;
				g_fHoldGridEntityPosY = t.gridentityposy_f;
				g_fHoldGridEntityPosZ = t.gridentityposz_f;
				t.gridentityposoffground=0;

				if ( t.entityprofile[t.gridentity].dontfindfloor != 0 )
				{
					// can set entity to initially ignore floor finder
					t.gridentityusingsoftauto = 0;
				}
				else
				{
					t.gridentityusingsoftauto = 1;
				}
				// MAX handles its own positioning system
				{
					t.gridentityautofind=0;
				}

				//PE: We get some flicker when moving objects around, when over object sometimes it use terrainheight.
				//PE: @Lee Think terrain is inside Wicked Pick so should not be needed. if not just remove this :)
				t.gridentityusingsoftauto = 0;

				t.gridentityeditorfixed=0;
				if (!bRotScaleAlreadyUpdated)
				{
					if (t.entityprofile[t.gridentity].ischaracter != 0)
					{
						// if character, always face the camera
						float fAngleToFaceCamera = CameraAngleY(0) + 180.0f;
						t.entityprofile[t.gridentity].roty = fAngleToFaceCamera;
					}
					t.gridentityrotatex_f = t.entityprofile[t.gridentity].rotx;
					t.gridentityrotatey_f = t.entityprofile[t.gridentity].roty;
					t.gridentityrotatez_f = t.entityprofile[t.gridentity].rotz;
					t.gridentityrotatequatmode = 0;
					t.gridentityrotatequatx_f = 0;
					t.gridentityrotatequaty_f = 0;
					t.gridentityrotatequatz_f = 0;
					t.gridentityrotatequatw_f = 1;
					t.gridentityscalex_f = t.entityprofile[t.gridentity].scale;
					t.gridentityscaley_f = t.entityprofile[t.gridentity].scale;
					t.gridentityscalez_f = t.entityprofile[t.gridentity].scale;
				}
				bRotScaleAlreadyUpdated = false;
				t.ttrygridentitystaticmode=t.entityprofile[t.gridentity].defaultstatic;
				t.ttrygridentity=t.gridentity ; editor_validatestaticmode ( );
				//  Ensure editor zoom refreshes
				t.updatezoom=1;
				//  fill new selection with defaults
				if (bEleProfFromSmartObject == false)
				{
					// only if not already populated from smart object element above
					t.sentid = t.entid; t.entid = t.gridentity;
					entity_fillgrideleproffromprofile();
					t.grideleprof.bUseFPESettings = true; //PE: New added always use bUseFPESettings.
					t.grideleprof.isProjectGlobal = false;

					t.entid = t.sentid;
				}
				t.grideleproflastname_s=t.grideleprof.name_s;
				//  marker types?
				if ( t.entityprofile[t.gridentity].ismarker == 1 && t.entityprofile[t.gridentity].lives != -1 ) 
				{
					//  selecting new player start marker resets tweakables
					physics_inittweakables ( );
				}
				if ( t.entityprofile[t.gridentity].ismarker == 3 || t.entityprofile[t.gridentity].ismarker == 6 || t.entityprofile[t.gridentity].ismarker == 8 ) 
				{
					//  trigger zone marker(3) or checkpoint marker(6) or floor zone marker(8)
					//  trigger zone has a waypoint zone companion
					if ( t.entityprofile[t.gridentity].ismarker == 8 ) 
						t.waypointeditstyle = 3; // navmeshzone
					else
						t.waypointeditstyle = 2; // normalzone
					t.waypointeditstylecolor=t.entityprofile[t.gridentity].trigger.stylecolor;
					t.waypointeditentity=0;
					t.mx_f=t.cx_f ; t.mz_f=t.cy_f;
					if (  t.terrain.TerrainID>0 ) 
					{
						g.waypointeditheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.mx_f,t.mz_f);
					}
					else
					{
						g.waypointeditheight_f=g.gdefaultterrainheight;
					}
					waypoint_createnew ( );
					t.grideleprof.trigger.waypointzoneindex=t.waypointindex;
				}
			}
		}

		//  In case new 'shader' associated with new entity, refresh just in case (i.e. first entity)
		visuals_justshaderupdate ( );

		//  Construction complete
		t.inputsys.constructselection = 0;
	}
}

void editor_validatestaticmode ( void )
{
	// receives ttrygridentitystaticmode,ttrygridentity
	if ( t.ttrygridentity>0 ) 
	{
		t.gridentitystaticmode=t.ttrygridentitystaticmode;
		bool bSomeShadersForceDynamicMode = false;
		if ( strcmp ( Lower(Right(t.entityprofile[t.ttrygridentity].effect_s.Get(),18)) , "character_basic.fx" ) == 0 ) bSomeShadersForceDynamicMode = true;
		if ( strcmp ( Lower(Right(t.entityprofile[t.ttrygridentity].effect_s.Get(),14)) , "treea_basic.fx" ) == 0 ) bSomeShadersForceDynamicMode = true;
		if ( bSomeShadersForceDynamicMode == true )
		{
			if ( ObjectExist(g.entitybankoffset+t.ttrygridentity) == 1 ) 
			{
				if ( GetNumberOfFrames(g.entitybankoffset+t.ttrygridentity)>0 ) 
				{
					t.gridentitystaticmode=0;
				}
			}
		}

		if (t.entityprofile[t.ttrygridentity].ischaracter)
		{
			// Possible fix for characters appearing static.
			t.gridentitystaticmode = 0;
		}
	}
}

void editor_overallfunctionality ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	//  Restore current grid view
	if (  t.inputsys.doautozoomview == 1 ) { t.inputsys.doautozoomview = 0  ; t.inputsys.dozoomview = 1; }
	if (  t.inputsys.dozoomview == 1 ) 
	{
		if (  t.cameraviewmode == 2 ) 
		{
			//  mouselook mode off
			OpenFileMap (  1, "FPSEXCHANGE" );
			SetFileMapDWORD (  1, 48, 0 );
			SetEventAndWait (  1 );
			//  re-enable icons
			editor_enableafterzoom ( );
			//  end zoom mode
			t.grideditselect=t.stgrideditselect  ; editor_refresheditmarkers ( );
			t.inputsys.dozoomview=0;
			t.cameraviewmode=0;
		}
	}

	//  Switch to zoom view
	if (  t.inputsys.dozoomview == 1 ) 
	{
		if (  t.cameraviewmode == 0 ) 
		{
			//  Set camera to track with close-up
			t.stgrideditselect=t.grideditselect;
			t.cameraviewmode = 2;

			//  Mode - Zoom In View
			t.grideditselect=4 ; editor_refresheditmarkers ( );
			t.updatezoom=1;
		}
	}

	//  Get terrain height reading at cursor
	t.ttterrheighthere_f=BT_GetGroundHeight(t.terrain.TerrainID,t.cx_f,t.cy_f);
	
	//  ensure zoom never penetrates terrain
	if (  t.updatezoom == 1 || t.inputsys.mclick != 0 ) 
	{
		if (  (600.0*t.gridzoom_f)<(t.ttterrheighthere_f+100) ) 
		{
			t.gridzoom_f=(t.ttterrheighthere_f+100)/600.0;
		}
	}

	//  Recalculate zoom scale for editing
	if (  t.updatezoom == 1 ) 
	{

		//  grid scale for camera cursor location and zoom
		t.gridscale_f=((800/2)/8)/t.gridzoom_f;
		t.inputsys.keypress=1;
		t.updatezoom=0;

		//  gridlayershowsingle creates an alpha slice in entity shaders
		t.gridnearcameraclip=-1;
		if (  t.grideditselect != 4 ) 
		{
			if (  g.gridlayershowsingle == 1 ) 
			{
				t.gridnearcameraclip=t.clipheight_f;
			}
		}

		//  modulate shadow strength based on distance
		t.tcamrange_f=((600.0*t.gridzoom_f)+1000);
		t.toldvisualsshadowmode=t.visuals.shadowmode;
		if (  t.tcamrange_f<4000 ) 
		{
			t.visuals.shadowmode=100;
		}
		else
		{
			if (  t.tcamrange_f<6000 ) 
			{
				t.visuals.shadowmode=(6000-t.tcamrange_f)/20.0;
			}
			else
			{
				t.visuals.shadowmode=0;
			}
		}
		if (  t.toldvisualsshadowmode != t.visuals.shadowmode ) 
		{
			visuals_justshaderupdate ( );
		}

		//  adjust clipping range of camera to match
		editor_refreshcamerarange ( );

		//  Ensure the slicing clip does not go
		if (  t.gridnearcameraclip == -1 ) 
		{
			t.gridtrueslicey_f=CameraPositionY(0);
		}
		else
		{
			t.gridtrueslicey_f=t.gridnearcameraclip;
		}

		//  feed alpha slicing height into all entity shaders
		if (  g.effectbankmax>0 ) 
		{
			for ( t.t = 1 ; t.t<=  g.effectbankmax; t.t++ )
			{
				t.effectid=g.effectbankoffset+t.t;
				if (  GetEffectExist(t.effectid) == 1 ) 
				{
					if (  t.gridnearcameraclip == -1 ) 
					{
						SetVector4 ( g.terrainvectorindex, 500000, 1, 0, 0 );
					}
					else
					{
						SetVector4 ( g.terrainvectorindex, t.gridtrueslicey_f, 1, 0, 0 );
					}
					SetEffectConstantV (  t.effectid,"EntityEffectControl",g.terrainvectorindex );
				}
			}
		}

	}

	//  use intersect test to find ground/wall and drop entity onto it
	if ( t.inputsys.k_s != "l" ) 
	{
		// but not if holding L key to link entity to a new parent
		if ( t.gridentitysurfacesnap == 1 )
		{
			// no need to find entity, surfacesnap already found best 3D coordinate
		}
		else
		{
			if (t.gridentitydroptoground >= 1 && t.gridentitydroptoground <= 2)
			{
				//PE: Need rubberband support here.
				float fdiff = t.gridentityposy_f;
				t.thardauto = 1; editor_findentityground();
				if (t.gridentityposoffground == 0)
				{
					float ftmp = BT_GetGroundHeight(t.terrain.TerrainID, t.gridentityposx_f, t.gridentityposz_f);
					t.gridentityposy_f = ftmp;
					ApplyPivotToGridEntity(); //PE: Apply pivot here.
				}
				//Only drop one time if in vertical move mode.
				if (iObjectMoveMode == 1)
				{
					if (fHitOffsetY != 0)
					{
						fdiff = fdiff -t.gridentityposy_f;
						fHitOffsetY += fdiff;
					}
					t.gridentityautofind = 0;
					t.gridentityposoffground = 1; //Dont proceed updating Y.
				}
				t.gridentitydroptoground=0;
			}
			else
			{
				t.thardauto=0 ; editor_findentityground ( );
			}
		}
	}

	//  Change layer show mode
	if (  t.inputsys.dosinglelayer == 1 ) 
	{
		g.gridlayershowsingle=g.gridlayershowsingle+1;
		if (  g.gridlayershowsingle>1  )  g.gridlayershowsingle = 0;
		t.updatezoom=1;
	}

	//  ensure assigned third person char object stays with start marker
	if (  t.playercontrol.thirdperson.enabled == 1 ) 
	{
		t.tobj=t.entityelement[t.playercontrol.thirdperson.charactere].obj;
		if (  t.tobj>0 ) 
		{
			if (  ObjectExist(t.tobj) == 1 ) 
			{
				if (  t.gridentity>0 && t.entityprofile[t.gridentity].ismarker == 1 ) 
				{
					//  moving start marker
					t.tstmrkobj=t.gridentityobj;
				}
				else
				{
					//  update char on start marker entity
					t.tstmrke=t.playercontrol.thirdperson.startmarkere;
					t.tstmrkobj=t.entityelement[t.tstmrke].obj;
				}
				if (  t.tstmrkobj>0 ) 
				{
					if (  ObjectExist(t.tstmrkobj) == 1 ) 
					{
						PositionObject (  t.tobj,ObjectPositionX(t.tstmrkobj),ObjectPositionY(t.tstmrkobj),ObjectPositionZ(t.tstmrkobj) );
						RotateObject (  t.tobj,ObjectAngleX(t.tstmrkobj),ObjectAngleY(t.tstmrkobj),ObjectAngleZ(t.tstmrkobj) );
					}
				}
				MoveObject (  t.tobj,-35 );
				if (  t.tstmrkobj>0 ) 
				{
					if (  ObjectExist(t.tstmrkobj) == 1 ) 
					{
						EnableObjectZDepth (  t.tstmrkobj );
						EnableObjectZWrite (  t.tstmrkobj );
						EnableObjectZRead (  t.tstmrkobj );
					}
				}
			}
		}
	}
}

void editor_refreshcamerarange ( void )
{
	t.tcamneardistance_f=CameraPositionY(0)/500.0;
	if ( t.tcamneardistance_f < 10.0  ) t.tcamneardistance_f = 10.0;
	if ( t.widget.activeObject > 0 )
	{
		// 011215 - except when widget shown, we need min distance to avoid clipping widget
		if ( t.tcamneardistance_f > 30.0f ) 
		{
			// to avoid water plane clipping, move water plane away from terrain plate incrementally
			t.terrain.waterlineyadjustforclip_f = (t.tcamneardistance_f-30.0f) * 5;
			t.tcamneardistance_f = 30.0f;
		}
	}
	else
	{
		t.terrain.waterlineyadjustforclip_f = 0.0f;
	}
	if (  t.editorfreeflight.mode == 1 ) 
	{
		// free flight FULL camera distance
		SetCameraRange ( t.tcamneardistance_f, DEFAULT_FAR_PLANE );
	}
	else
	{
		//  top down camera distance
		SetCameraRange( t.tcamneardistance_f, DEFAULT_FAR_PLANE );
	}
}

void editor_mainfunctionality ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	//  Rotation of entity
	// GGMAX 3.15: sub-ranges for P2-mainfunc. The flat 0.54 ms said nothing about WHERE, and
	// this function is ~500 lines in which almost every call is event driven. Safe to place
	// by hand: editor_mainfunctionality has ZERO early returns (checked), so no path can skip
	// an EndRange and leak a row into every sibling after it.
	auto rP2sel5 = wi::profiler::BeginRangeCPU("P2M-Sel5-EditorMode");
	if (  t.grideditselect == 5 ) 
	{
		bool bAllowRotate = true;
		if (t.widget.pickedEntityIndex > 0 && t.entityelement[t.widget.pickedEntityIndex].editorlock == 1)
			bAllowRotate = false;

		//  do not rotate light or trigger entity
		//PE: Allow light rotation rem: t.entityprofile[t.gridentity].ismarker != 2 &&  t.entityprofile[t.gridentity].ismarker != 3
		if ( bAllowRotate && t.entityprofile[t.gridentity].ismarker != 3 )
		{
			if (  t.inputsys.keyshift == 1 ) 
			{
				t.tspeedofrot_f=10.0 ; t.inputsys.keypress=0;
			}
			else
			{
				if (  t.inputsys.keycontrol == 1 ) 
				{
					t.tspeedofrot_f=1.0;
				}
				else
				{
					t.tspeedofrot_f=45.0;
				}
			}
			//PE: Prefer gridentity rotation. as we can now have both active at the same time.
			//PE: We dont need t.widget.pickedObject != 0 && to control widget.
			if(t.widget.pickedEntityIndex > 0 && t.gridentity == 0)
			{
				auto rP2pick = wi::profiler::BeginRangeCPU("P2M-Rotate-Picked");
				// Rotation control of widget controlled entity
				if ( t.inputsys.domodeterrain == 0 && t.inputsys.domodeentity == 0 ) 
				{
					if ( t.inputsys.dorotation == 1 || (t.inputsys.doentityrotate >= 1 && t.inputsys.doentityrotate <= 6) ||
						 t.inputsys.keyreturn == 1 || t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209 )
					{
						//PE: Make sure all groups are selected.
						int group = isEntityInGroupList(t.widget.pickedEntityIndex);
						if (group >= 0)
						{
							//Add all groups with entity to rubberband.
							CheckGroupListForRubberbandSelections(t.widget.pickedEntityIndex);
						}
						else if (g.entityrubberbandlist.size() > 0)
						{
							//Make sure all groups is selected from within rubberband selecting.
							for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
							{
								int e = g.entityrubberbandlist[i].e;
								CheckGroupListForRubberbandSelections(e);
							}
						}
					}
					//  avoid interference from terrain/entity mode change
					GGQUATERNION quatRotationEvent = { 0,0,0,0 };
					float fMoveAngX = 0.0f;
					float fMoveAngY = 0.0f;
					float fMoveAngZ = 0.0f;
					float fStoreOrigYAngle = t.entityelement[t.widget.pickedEntityIndex].ry;
					bool bRotateObjectFromKeyPress = false;
					if (t.inputsys.dorotation == 1) 
					{ 
						fMoveAngY = fMoveAngY + t.tspeedofrot_f; 
						bRotateObjectFromKeyPress = true; 
					}
					if (t.inputsys.doentityrotate == 1) { fMoveAngX = fMoveAngX - t.tspeedofrot_f; bRotateObjectFromKeyPress = true; }
					if (t.inputsys.doentityrotate == 2) { fMoveAngX = fMoveAngX + t.tspeedofrot_f; bRotateObjectFromKeyPress = true; }
					if (t.inputsys.doentityrotate == 3) 
					{ 
						fMoveAngY = fMoveAngY - t.tspeedofrot_f; 
						bRotateObjectFromKeyPress = true; 
					}
					if (t.inputsys.doentityrotate == 4) 
					{ 
						fMoveAngY = fMoveAngY + t.tspeedofrot_f; 
						bRotateObjectFromKeyPress = true; 
					}
					if (t.inputsys.doentityrotate == 5) { fMoveAngZ = fMoveAngZ - t.tspeedofrot_f; bRotateObjectFromKeyPress = true; }
					if (t.inputsys.doentityrotate == 6) { fMoveAngZ = fMoveAngZ + t.tspeedofrot_f; bRotateObjectFromKeyPress = true; }

					// special case for characters, only rotate Y angle
					if (t.widget.pickedEntityIndex > 0)
					{
						int entidcheck = t.entityelement[t.widget.pickedEntityIndex].bankindex;
						if (t.entityprofile[entidcheck].ischaracter == 1)
						{
							fMoveAngX = 0.0f;
							fMoveAngZ = 0.0f;
						}
					}
					// ready for quat rot
					static bool bStartedKeyboardRotation = false;
					static std::vector<std::array<float, 3>> prevRotations;
					static std::vector<std::array<float, 4>> prevQuatRotations;
					static std::vector<std::array<int, 1>> prevQuatRotationsMode;
					static std::vector <std::array<float, 3>> prevPositions;			
					int index = t.widget.pickedEntityIndex;
					
					// Store initial rotations before any changes have been applied.
					if (bRotateObjectFromKeyPress && !bStartedKeyboardRotation)
					{
						if (g.entityrubberbandlist.size() == 0)
						{
							std::array<float, 3> prevRotation = { t.entityelement[index].rx, t.entityelement[index].ry, t.entityelement[index].rz };
							prevRotations.push_back(prevRotation);
							std::array<float, 4> prevQuatRotation = { t.entityelement[index].quatx, t.entityelement[index].quaty, t.entityelement[index].quatz, t.entityelement[index].quatw };
							prevQuatRotations.push_back(prevQuatRotation);
							std::array<int, 1> prevQuatRotationMode = { t.entityelement[index].quatmode };
							prevQuatRotationsMode.push_back(prevQuatRotationMode);
						}
						else
						{
							for (int i = 0; i < g.entityrubberbandlist.size(); i++)
							{
								int e = g.entityrubberbandlist[i].e;
								std::array<float, 3> prevRotation = { t.entityelement[e].rx, t.entityelement[e].ry, t.entityelement[e].rz };
								prevRotations.push_back(prevRotation);
								std::array<float, 4> prevQuatRotation = { t.entityelement[e].quatx, t.entityelement[e].quaty, t.entityelement[e].quatz, t.entityelement[e].quatw };
								prevQuatRotations.push_back(prevQuatRotation);
								std::array<int, 1> prevQuatRotationMode = { t.entityelement[e].quatmode };
								prevQuatRotationsMode.push_back(prevQuatRotationMode);
								// Need to store positions for rubberband, since they rotate about a point. 
								std::array<float,3> prevPosition = { t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z };
								prevPositions.push_back(prevPosition);
							}
						}
						bStartedKeyboardRotation = true;
					}

					// Now that the rotation has finished, pass the event(s) to the undo system.
					if (bStartedKeyboardRotation && !bRotateObjectFromKeyPress)
					{
						bStartedKeyboardRotation = false;

						if (g.entityrubberbandlist.size() == 0)
						{
							undosys_object_changeposrotscl(index, t.entityelement[index].x, t.entityelement[index].y, t.entityelement[index].z, 
								prevRotations[0][0], prevRotations[0][1], prevRotations[0][2],
								prevQuatRotationsMode[0][0], prevQuatRotations[0][0], prevQuatRotations[0][1], prevQuatRotations[0][2], prevQuatRotations[0][3],
								t.entityelement[index].scalex, t.entityelement[index].scaley, t.entityelement[index].scalez);
						}
						else
						{
							undosys_multiplevents_start();
							for (int i = 0; i < prevPositions.size(); i++)
							{
								int e = g.entityrubberbandlist[i].e;
								undosys_object_changeposrotscl(e, prevPositions[i][0], prevPositions[i][1],	prevPositions[i][2], 
									prevRotations[i][0], prevRotations[i][1], prevRotations[i][2],
									prevQuatRotationsMode[i][0], prevQuatRotations[i][0], prevQuatRotations[i][1], prevQuatRotations[i][2], prevQuatRotations[i][3],
									t.entityelement[e].scalex, t.entityelement[e].scaley, t.entityelement[e].scalez);
							}
							undosys_multiplevents_finish();
						}

						prevRotations.clear();
						prevPositions.clear();
					}
					
					// trigger a rotation when detect rotation key pressed
					if (bRotateObjectFromKeyPress == true)
					{
						// the object to rotate
						int te = t.widget.pickedEntityIndex;
						int iObj = t.entityelement[t.widget.pickedEntityIndex].obj;
						int entid = t.entityelement[t.widget.pickedEntityIndex].bankindex;
						if (t.entityprofile[entid].ragdoll == 1)
						{
							fMoveAngX = 0.0f;
							fMoveAngZ = 0.0f;
						}

						// quat rotation event
						GGQUATERNION QuatAroundX, QuatAroundY, QuatAroundZ;
						GGQuaternionRotationAxis(&QuatAroundX, &GGVECTOR3(1, 0, 0), GGToRadian(fMoveAngX));
						GGQuaternionRotationAxis(&QuatAroundY, &GGVECTOR3(0, 1, 0), GGToRadian(fMoveAngY));
						GGQuaternionRotationAxis(&QuatAroundZ, &GGVECTOR3(0, 0, 1), GGToRadian(fMoveAngZ));
						quatRotationEvent = QuatAroundX * QuatAroundY * QuatAroundZ;

						// get quat from entity directly
						if (t.entityelement[te].quatmode == 0)
						{
							// if no orig quaty, calc it now
							entity_updatequatfromeuler(te);
						}
						GGQUATERNION toriginalAngle = GGQUATERNION(t.entityelement[te].quatx, t.entityelement[te].quaty, t.entityelement[te].quatz, t.entityelement[te].quatw);

						// apply the rotation event to the angle of the object
						GGQUATERNION quatNewOrientation;
						GGQuaternionMultiply(&quatNewOrientation, &toriginalAngle, &quatRotationEvent);

						// rotate this object with final quat and get new entity rotation eulers
						RotateObjectQuat(iObj, quatNewOrientation.x, quatNewOrientation.y, quatNewOrientation.z, quatNewOrientation.w);
						t.entityelement[te].rx = ObjectAngleX(iObj);
						t.entityelement[te].ry = ObjectAngleY(iObj);
						t.entityelement[te].rz = ObjectAngleZ(iObj);

						// update entity quat as the preferred source rotation
						t.entityelement[te].quatmode = 1;
						t.entityelement[te].quatx = quatNewOrientation.x;
						t.entityelement[te].quaty = quatNewOrientation.y;
						t.entityelement[te].quatz = quatNewOrientation.z;
						t.entityelement[te].quatw = quatNewOrientation.w;

						// mark as static if it was
						if (t.entityelement[te].staticflag == 1) g.projectmodifiedstatic = 1;

						// special case for characters, only want the Y angle
						if (t.entityprofile[entid].ischaracter == 1)
						{
							// quats are the true rotations of objects, but refresh euler for characters to ONLY use the Y axis
							entity_calculateeuleryfromquat(t.widget.pickedEntityIndex);
						}

						// when rotate with keys, see new value in slider/value right panel
						g_bRefreshRotationValuesFromObjectOnce = true;
					}

					//PE: Update light data for spot.
					//PE: Updating probes is slow, this is called on each frame.
					if (t.entityelement[t.widget.pickedEntityIndex].eleprof.usespotlighting)
					{
						static bool bReadyToUpdateSpot = false;
						bool bUpdate = false;
						if (!bReadyToUpdateSpot && ImGui::IsMouseClicked(0)) bReadyToUpdateSpot = true;
						if (bReadyToUpdateSpot && !ImGui::IsMouseClicked(0))
						{
							bReadyToUpdateSpot = false;
							bUpdate = true;
						}
						if(bUpdate)
						{
							lighting_refresh();
						}
					}

					// also update particle emitter
					entity_updateparticleemitter(t.widget.pickedEntityIndex);
					entity_updateautoflatten(t.widget.pickedEntityIndex);

					if ( t.entityelement[t.widget.pickedEntityIndex].obj>0 ) 
					{
						int iTargetCenterObject = t.entityelement[t.widget.pickedEntityIndex].obj;
						if ( ObjectExist ( iTargetCenterObject ) == 1 ) 
						{
							if ( g.entityrubberbandlist.size() > 0 )
							{
								// rotate all the grouped entities and move around Y axis of widget as pivot
								if (bRotateObjectFromKeyPress == true)
								{
									SetStartPositionsForRubberBand(iTargetCenterObject);
									RotateAndMoveRubberBand(iTargetCenterObject, 0, 0, 0, quatRotationEvent);
								}
							}
						}
					}
				}
				//  Find Floor (  control of widget controlled entity or Raise/lower with PGUP and PGDN )
				if (t.inputsys.keyreturn == 1 || t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209)
				{
					t.tforceentityfindfloor = t.widget.pickedEntityIndex;
					t.tforcepguppgdnkeys = 1;
					editor_forceentityfindfloor (false);
				}
				wi::profiler::EndRange(rP2pick);
			}
			else
			{
				auto rP2else = wi::profiler::BeginRangeCPU("P2M-Rotate-NoPick");
				if (  t.inputsys.domodeterrain == 0 && t.inputsys.domodeentity == 0 ) 
				{
					if (t.widget.pickedEntityIndex > 0)
					{
						if (iObjectMoveModeDropSystem < 0)
						{
							//PE: Support new y placement when also using shift. t.gridentity is set.
							if (t.inputsys.keyreturn == 1 || t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209)
							{
								t.tforceentityfindfloor = t.widget.pickedEntityIndex;
								t.tforcepguppgdnkeys = 1;
								editor_forceentityfindfloor(false);
							}
						}

					}
					//  avoid interference from terrain/entity mode change
					if (  t.inputsys.dorotation == 1 ) 
					{
						if (iObjectMoveMode == 2 && g_iOrientToSurfaceMode == 1)
						{
							g_fLocalTurnRotationForSmartMode += t.tspeedofrot_f;
							g_fLocalTurnRotationForSmartMode = WrapValue(g_fLocalTurnRotationForSmartMode);
						}
						else
						{
							t.gridentityrotatey_f += t.tspeedofrot_f;
							t.gridentityrotateaxis = 1;
						}
					}
					if (  t.inputsys.doentityrotate == 1 )
					{ 
						t.gridentityrotatex_f -= t.tspeedofrot_f  ; t.gridentityrotateaxis = 0; 
					}
					if (  t.inputsys.doentityrotate == 2 ) { t.gridentityrotatex_f += t.tspeedofrot_f  ; t.gridentityrotateaxis = 0; }
					if (  t.inputsys.doentityrotate == 3 ) { t.gridentityrotatey_f -= t.tspeedofrot_f  ; t.gridentityrotateaxis = 1; }
					if (  t.inputsys.doentityrotate == 4 ) { t.gridentityrotatey_f += t.tspeedofrot_f  ; t.gridentityrotateaxis = 1; }
					if (  t.inputsys.doentityrotate == 5 ) { t.gridentityrotatez_f -= t.tspeedofrot_f  ; t.gridentityrotateaxis = 2; }
					if (  t.inputsys.doentityrotate == 6 ) { t.gridentityrotatez_f += t.tspeedofrot_f  ; t.gridentityrotateaxis = 2; }
					if (  t.inputsys.doentityrotate >= 98 ) 
					{
						if (  t.inputsys.doentityrotate == 98 ) 
						{
							if (  t.gridentityrotateaxis == 0  )  t.gridentityrotatex_f = 0;
							if (  t.gridentityrotateaxis == 1  )  t.gridentityrotatey_f = 0;
							if (  t.gridentityrotateaxis == 2  )  t.gridentityrotatez_f = 0;
						}
						if (  t.inputsys.doentityrotate == 99 ) 
						{
							t.gridentityrotatex_f=0;
							t.gridentityrotatey_f=0;
							t.gridentityrotatez_f=0;
						}
					}
				}
				t.gridentityrotatex_f=WrapValue(t.gridentityrotatex_f);
				t.gridentityrotatey_f=WrapValue(t.gridentityrotatey_f);
				t.gridentityrotatez_f=WrapValue(t.gridentityrotatez_f);
				wi::profiler::EndRange(rP2else);
			}
		}
	}
	wi::profiler::EndRange(rP2sel5);
	auto rP2tail = wi::profiler::BeginRangeCPU("P2M-Events+Tail");

	//  Load and Save
	auto rP2t1 = wi::profiler::BeginRangeCPU("P2M-T1-FileEvents");
	if ( t.inputsys.doload == 1 ) gridedit_load_map ( );
	if ( t.inputsys.dosave == 1 ) 
	{
		if (  g.galwaysconfirmsave == 1 ) 
		{
			gridedit_saveas_map ( );
		}
		else
		{
			gridedit_save_map_ask ( );
		}
	}
	if ( t.inputsys.doopen == 1 ) gridedit_open_map_ask ( );
	if ( t.inputsys.donew == 1 || t.inputsys.donewflat == 1 ) gridedit_new_map_ask ( );
	if ( t.inputsys.dosaveas == 1 ) gridedit_saveas_map ( );
	if ( t. inputsys.dosaveandrun==1 ) { t.inputsys.dosaveandrun = 0 ; editor_previewmap ( 0 ); }

	//  Undo \ Redo
	if (t.inputsys.doundo == 1)
	{
		editor_undo ();
		t.inputsys.doundo = 0;
	}
	if (t.inputsys.doredo == 1)
	{
		editor_redo ();
		t.inputsys.doredo = 0;
	}

	//  Paint Select or Art Mode
	if ( t.inputsys.domodeterrain == 1 || t.inputsys.domodemarker == 1 || t.inputsys.domodeentity == 1  || t.inputsys.domodewaypoint == 1 )
	{
		// select editing mode and refresh
		if ( t.inputsys.domodeterrain == 1 ) { t.inputsys.domodeterrain=0; t.gridentitymarkersmodeonly=0; t.grideditselect=0; }
		if ( t.inputsys.domodemarker == 1 ) { t.inputsys.domodemarker=0; t.gridentitymarkersmodeonly=1; t.grideditselect=5; }
		if ( t.inputsys.domodeentity == 1 ) { t.inputsys.domodeentity=0; t.gridentitymarkersmodeonly=0; t.grideditselect=5; }
		if ( t.inputsys.domodewaypoint == 1 ) { t.inputsys.domodewaypoint=0; t.gridentitymarkersmodeonly=0; t.grideditselect=6; }
		editor_refresheditmarkers ( );
		gridedit_updateprojectname();

		// also deactivate EBE if enter a regular editing mode
		ebe_hide();
	}

	//  Manage waypoints on map
	t.tokay=0;
	wi::profiler::EndRange(rP2t1);
	auto rP2t2 = wi::profiler::BeginRangeCPU("P2M-T2-SelModes+Keys");
	if (  t.grideditselect == 5 ) 
	{
		//  entity mode can manipulate waypoint zone style
		if (  t.widget.pickedObject == 0 && t.widget.pickedSection == 0 && t.gridentity == 0 ) 
		{
			//  ensure low interference if editing, etc
			t.tokay=1;
		}
	}
	if (  t.grideditselect == 6 ) 
	{
		//  waypoint mode has access to waypoint editing
		t.tokay=1;
	}
	if (  t.tokay == 1 ) 
	{
		t.mx_f=t.inputsys.localx_f ; t.mz_f=t.inputsys.localy_f ; t.mclick=t.inputsys.mclick;
		g.waypointeditheight_f=t.inputsys.localcurrentterrainheight_f;

		// only detect waypoints when NOT using rubber band
		if (t.inputsys.rubberbandmode == 0 && t.ebe.on == 0 && t.showeditorelements)
			waypoint_mousemanage ( );
	}

	//  New clip height control
	if (  t.inputsys.keycontrol == 1 ) 
	{
		if (  t.inputsys.wheelmousemove>0 ) 
		{
			t.clipheight_f -= 2.0f ; if (  t.clipheight_f<0.0f  )  t.clipheight_f = 0.0f;
			t.updatezoom=1;
		}
		if (  t.inputsys.wheelmousemove<0 ) 
		{
			t.clipheight_f += 2.0f ; if (  t.clipheight_f>50000.0f )  t.clipheight_f = 50000.0f;
			t.updatezoom=1;
		}
	}

	//  Zoom factor (for top down or freeflight+ControlKey ( ) )
	t.tspecialgridzoomadjustment=0;
	wi::profiler::EndRange(rP2t2);
	auto rP2t3 = wi::profiler::BeginRangeCPU("P2M-T3-Camera+Scroll");
	if (  t.editorfreeflight.mode == 0 || t.tspecialgridzoomadjustment != 0 ) 
	{
		if (  ((t.inputsys.dozoomin == 1 && t.inputsys.keypress == 0) || t.tspecialgridzoomadjustment == 1) && t.gridzoom_f>0.3 ) 
		{
			t.updatezoom=1;
			if (  t.inputsys.keyshift == 1 ) 
			{
				t.gridzoom_f -= 0.6f*fMouseWheelZoomFactor;
			}
			else
			{
				t.gridzoom_f -= 0.1f*fMouseWheelZoomFactor;
			}
		}
		if (  ((t.inputsys.dozoomout == 1 && t.inputsys.keypress == 0) || t.tspecialgridzoomadjustment == 2) && t.gridzoom_f<40.0 ) 
		{
			t.updatezoom=1;
			if (  t.inputsys.keyshift == 1 ) 
			{
				t.gridzoom_f += 0.6f*fMouseWheelZoomFactor;
			}
			else
			{
				t.gridzoom_f += 0.1f*fMouseWheelZoomFactor;
			}
		}
	}

	//  Scroll Map
	t.borderx_f=1024.0*50.0;
	t.bordery_f=1024.0*50.0;
	float camSpeedMod = 0.5f;
	if (t.inputsys.keyshift)
	{
		camSpeedMod = 1.0f;
	}
	else if (t.inputsys.keycontrol)
	{
		camSpeedMod = 0.1f;
	}
	if (  t.inputsys.doscrollleft != 0 ) 
	{
		t.cx_f -= t.gridzoom_f*3*t.inputsys.doscrollleft*camSpeedMod;
		t.updatezoom=1;
	}
	if (  t.inputsys.doscrollright != 0 ) 
	{
		t.cx_f += t.gridzoom_f * 3 * t.inputsys.doscrollright*camSpeedMod;
		t.updatezoom=1;
	}
	if (  t.inputsys.doscrollup != 0 ) 
	{
		t.cy_f += t.gridzoom_f*3*t.inputsys.doscrollup*camSpeedMod;
		t.updatezoom=1;
	}
	if (  t.inputsys.doscrolldown != 0 ) 
	{
		t.cy_f -= t.gridzoom_f*3*t.inputsys.doscrolldown*camSpeedMod;
		t.updatezoom=1;
	}

	//  Scroll boundaries
	// no such limits in the MAX world!

	//PE: Only active in object mode. t.grideditselect == 5
	if (!g_bCharacterCreatorPlusActivated && !bStoryboardWindow && t.grideditselect == 5 )
	{
		//PE: @Paul we need this in object mode :)
		MouseLeftDragXZPanning();
		MouseWheelYPanning();
	}
	wi::profiler::EndRange(rP2t3);
	wi::profiler::EndRange(rP2tail);
}

