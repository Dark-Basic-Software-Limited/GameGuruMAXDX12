extern int lightmappedterrainoffset;
extern int lightmappedobjectoffset;

void lm_loadscene ( void )
{
	// set reset globals used by occluder
	lightmappedterrainoffset = MAXINT32;
	lightmappedobjectoffset = MAXINT32;
	t.tlmloadsuccess=0;

	// Don't use lightmaps for multiplayer, unless you are the host
	if ( FileExist(t.lightmapper.lmobjectfile_s.Get()) == 0 || (t.game.runasmultiplayer  ==  1 && g.mp.isGameHost  ==  0)  )  return;

	// Remove any old lightmapping scene
	timestampactivity(0,"delete old LMOs");
	lm_deleteall ( );

	// Load lmsceneobj list
	t.tlightmapfilesexist=0;
	g.lightmappedterrainoffsetfinish=g.lightmappedterrainoffset;
	timestampactivity(0,"load LMO list");
	if (  FileExist(t.lightmapper.lmobjectfile_s.Get()) == 1 ) 
	{
		t.tlightmapfilesexist=1;
		OpenToRead (  1,t.lightmapper.lmobjectfile_s.Get() );
		t.tlmsceneobjversion = ReadLong ( 1 );
		if (  t.tlmsceneobjversion >= 1002 ) 
		{
			g.glmsceneentitymax = ReadLong ( 1 );
			Dim (  t.lmsceneobj,g.glmsceneentitymax );
			for ( t.e = 0 ; t.e <= g.glmsceneentitymax ; t.e++ ) {  t.lmsceneobj[t.e].startobj=0 ; t.lmsceneobj[t.e].finishobj=0 ; t.lmsceneobj[t.e].lmvalid=0;  }
			for ( t.e = 1 ; t.e<=  g.glmsceneentitymax; t.e++ )
			{
				t.a = ReadLong ( 1 ); t.lmsceneobj[t.e].startobj=t.a;
				t.a = ReadLong ( 1 ); t.lmsceneobj[t.e].finishobj=t.a;
				t.a = ReadLong ( 1 ); t.lmsceneobj[t.e].bankindex=t.a;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].x=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].y=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].z=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].rx=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].ry=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].rz=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].sx=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].sy=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lmsceneobj[t.e].sz=t.a_f;
				t.a = ReadLong ( 1 ); t.lmsceneobj[t.e].lmindex=t.a;
				if (  t.tlmsceneobjversion >= 1003 ) 
				{
					t.a = ReadLong ( 1 ); t.lmsceneobj[t.e].includerotandscale=t.a;
				}
				if (  t.tlmsceneobjversion >= 1004 ) 
				{
					t.a = ReadLong ( 1 ); t.lmsceneobj[t.e].reverseframes=t.a;
				}
				t.lmsceneobj[t.e].lmvalid=3;
				t.lmsceneobj[t.e].needsaving=0;
			}
			g.lightmappedterrainoffset = ReadLong ( 1 );
			g.lightmappedterrainoffsetfinish = ReadLong ( 1 );
			Dim2 ( t.lightmappedterrain,g.lightmappedterrainoffsetfinish-g.lightmappedterrainoffset, 1 );
			for ( t.tlmobj2 = g.lightmappedterrainoffset ; t.tlmobj2<=  g.lightmappedterrainoffsetfinish; t.tlmobj2++ )
			{
				t.a_f = ReadFloat ( 1 ); t.lightmappedterrain[t.tlmobj2-g.lightmappedterrainoffset][0]=t.a_f;
				t.a_f = ReadFloat ( 1 ); t.lightmappedterrain[t.tlmobj2-g.lightmappedterrainoffset][1]=t.a_f;
			}
		}
		CloseFile (  1 );
	}

	// Now mark ANY LM objects as not exist that do not tally with entityelement data
	for ( t.e = 1 ; t.e<=  g.glmsceneentitymax; t.e++ )
	{
		if ( t.e <= ArrayCount(t.entityelement) ) 
		{
			t.tokay=0;
			if ( t.entityelement[t.e].obj == 0  )  t.tokay = 1;
			if ( t.entityelement[t.e].bankindex != t.lmsceneobj[t.e].bankindex  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].x != (int)t.lmsceneobj[t.e].x  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].y != (int)t.lmsceneobj[t.e].y  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].z != (int)t.lmsceneobj[t.e].z  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].rx != (int)t.lmsceneobj[t.e].rx  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].ry != (int)t.lmsceneobj[t.e].ry  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].rz != (int)t.lmsceneobj[t.e].rz  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].scalex != (int)t.lmsceneobj[t.e].sx  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].scaley != (int)t.lmsceneobj[t.e].sy  )  t.tokay = 1;
			if ( (int)t.entityelement[t.e].scalez != (int)t.lmsceneobj[t.e].sz  )  t.tokay = 1;
			if ( t.tokay == 1 ) 
			{
				t.lmsceneobj[t.e].lmvalid=0;
				t.lmsceneobj[t.e].startobj=0;
				t.lmsceneobj[t.e].finishobj=0;
				if (  t.lmsceneobj[t.e].lmindex>0 ) 
				{
					for ( t.te = 1 ; t.te<=  g.glmsceneentitymax; t.te++ )
					{
						if (  t.te != t.e ) 
						{
							if (  t.lmsceneobj[t.e].lmindex == t.lmsceneobj[t.te].lmindex ) 
							{
								t.lmsceneobj[t.te].lmvalid=0;
							}
						}
					}
				}
			}
		}
		else
		{
			t.lmsceneobj[t.e].lmvalid=0;
		}
	}

	// record current directory
	t.tolddir_s=GetDir();

	// Free hold on any previously stored lightmaps, so the ones
	// loaded next are the newest ones (to be re-used by other LMOs)
	timestampactivity(0,"clear LMO internal textures");
	ClearAnyLightMapInternalTextures (  );

	// Activate auto generation of mipmaps for ALL LM objects
	SetImageAutoMipMap (  1 );

	// Load lightmapped scene objects (and only those not out of date)
	timestampactivity(0,"begin loading LMO files");
	for ( t.e = 1 ; t.e<=  g.glmsceneentitymax; t.e++ )
	{
		if (t.e < t.entityelement.size())
		{
			t.tlmobj = t.lmsceneobj[t.e].startobj;
			t.tentid = t.entityelement[t.e].bankindex;
			if ((t.tlmobj > 0 && t.lmsceneobj[t.e].lmvalid == 3) || (t.tlmobj < 0))
			{
				// hide instance entity
				if (t.e <= ArrayCount(t.entityelement))
				{
					if (t.game.gameisexe == 1)
					{
						// Standalone Game
						t.toldobj = t.entityelement[t.e].obj; if (t.toldobj > 0) { if (ObjectExist(t.toldobj) == 1) { DeleteObject(t.toldobj); } }
						t.entityelement[t.e].obj = 0;
					}
					else
					{
						// Editor
						t.toldobj = t.entityelement[t.e].obj; if (t.toldobj > 0) { if (ObjectExist(t.toldobj) == 1) { SetIgnoreObject(t.toldobj, true); } }
					}
				}

				// load LM object
				t.tobjstart = abs(t.lmsceneobj[t.e].startobj);
				for (t.tlmobj = t.tobjstart; t.tlmobj <= t.lmsceneobj[t.e].finishobj; t.tlmobj++)
				{
					t.tfile_s = t.lightmapper.lmpath_s + "object" + Str(t.tlmobj) + ".dbo";
					if (FileExist(t.tfile_s.Get()) == 1 && t.e <= ArrayCount(t.entityelement))
					{
						if (t.tlmobj > g.lightmappedobjectoffsetfinish)  g.lightmappedobjectoffsetfinish = t.tlmobj;
						if (ObjectExist(t.tlmobj) == 1)  DeleteObject(t.tlmobj);
						t.tentid = t.entityelement[t.e].bankindex;
						t.tdir_s = t.entdir_s + getpath(t.entitybank_s[t.tentid].Get());
						if (PathExist(t.tdir_s.Get()))
						{
							SetDir(t.tdir_s.Get());
						}
						if (t.lightmapper.onlyloadstaticentitiesduringlightmapper > 0 && Rnd(10) == 1)
						{
							t.tdisableLMprogressreading = 1;
							for (t.n = 0; t.n <= 1; t.n++)
							{
								if (t.lightmapper.onlyloadstaticentitiesduringlightmapper == 2)  CLS(Rgb(102, 102, 153));
								t.strwork = ""; t.strwork = t.strwork + "Loading Lightmap Object " + Str(t.e);
								t.tonscreenprompt_s = t.strwork.Get();
								if (t.hardwareinfoglobals.noterrain == 0) terrain_update();
								lm_onscreenprompt(); Sync();
							}
							t.tdisableLMprogressreading = 0;
						}
						LoadObject(t.tfile_s.Get(), t.tlmobj);
						if (t.lmsceneobj[t.e].reverseframes == 1)
						{
							ReverseObjectFrames(t.tlmobj);
						}
						SetDir(t.tolddir_s.Get());

						//Check if we need illum.
						bool useillum = false;
						sObject* pObject = g_ObjectList[t.tlmobj];
						sMesh* pMesh;
						for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++) {
							pMesh = pObject->ppMeshList[iMesh];
							if (pMesh) {
								DWORD dwTextureCount = pMesh->dwTextureCount;
								for (DWORD t = 0; t < dwTextureCount; t++) {
									sTexture* pTexture = &(pMesh->pTextures[t]);
									if (pTexture) {

										if (pestrcasestr(&pTexture->pName[0], "_illumination.") || pestrcasestr(&pTexture->pName[0], "_emissive.") || pestrcasestr(&pTexture->pName[0], "_i.dds")) {
											//timestampactivity(0, "LM use illum");
											useillum = true;

											//debug
											//char dtmp[2048];
											//sprintf(dtmp, "LM ILLUM m: %d , t: %d , name: %s", (int)iMesh, (int)t, pTexture->pName);
											//timestampactivity(0, dtmp);

											break;
										}
									}
								}
							}
							if (useillum)
								break;
						}


						// if white out, replace diffuse with white texture
						if (g.gdividetexturesize == 0)
						{
							t.ttexuseid = loadinternaltextureex("effectbank\\reloaded\\media\\white_D.dds", 1, 0);
							TextureObject(t.tlmobj, 0, t.ttexuseid);
						}
						else
						{
							// determine if in PBR mode, and apply lightmap PBR shader
							// load PBR lightmap shader if not exists
							if (GetEffectExist(g.lightmappbreffect) == 0)
							{
								LPSTR pLightmapPBREffect = "effectbank\\reloaded\\apbr_lightmapped.fx";
								//LPSTR pLightmapPBREffect = "effectbank\\reloaded\\static_basic.fx";
								LoadEffect(pLightmapPBREffect, g.lightmappbreffect, 0);
							}
							if (GetEffectExist(g.lightmappbreffectillum) == 0)
							{
								LPSTR pLightmapPBREffect = "effectbank\\reloaded\\apbr_lightmapped_illum.fx";
								LoadEffect(pLightmapPBREffect, g.lightmappbreffectillum, 0);
							}

							// apply lightmap PBR shader to lightmapped object
							if (useillum)
							{
								SetObjectEffect(t.tlmobj, g.lightmappbreffectillum);
							}
							else
							{
								SetObjectEffect(t.tlmobj, g.lightmappbreffect);
							}
						}

						// apply textures from doner entity parent only if not been consolidated
						if (t.lmsceneobj[t.e].startobj > 0)
						{
							// textures common to PBR and DNS
							if (t.entityprofile[t.tentid].texdid > 0) TextureObject(t.tlmobj, 0, t.entityprofile[t.tentid].texdid);
							if (t.entityprofile[t.tentid].texnid > 0) TextureObject(t.tlmobj, 2, t.entityprofile[t.tentid].texnid);
							if (t.entityprofile[t.tentid].texsid > 0) TextureObject(t.tlmobj, 3, t.entityprofile[t.tentid].texsid);

							// which shader style (PBR or DNS)
							if (g.gpbroverride == 1)
							{
								// PBR textures
								if (g.memskipibr == 0) TextureObject(t.tlmobj, 8, t.entityprofiletexibrid);
								TextureObject(t.tlmobj, 7, t.entityprofile[t.tentid].texlid);
								TextureObject(t.tlmobj, 4, t.entityprofile[t.tentid].texgid);
								TextureObject(t.tlmobj, 5, t.entityprofile[t.tentid].texhid);
							}
							else
							{
								// non-PBR
								TextureObject(t.tlmobj, 4, t.terrain.imagestartindex);
								TextureObject(t.tlmobj, 5, g.postprocessimageoffset + 5);
							}

							// Apply all textures to REMAINING entity parent object (V C I)
							// TextureObject ( t.entobj, 6, t.entityprofile[t.entid].texiid );
							if (t.entityprofile[t.tentid].texiid > 0)
							{
								TextureObject(t.tlmobj, 6, t.entityprofile[t.tentid].texiid);
							}
						}

						// batching incorporates rotation and scale (not position as we want to camera cull them)
						SetObjectMask(t.tlmobj, 1);
						if (t.e <= ArrayCount(t.entityelement))
						{
							PositionObject(t.tlmobj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z);
							if (t.lmsceneobj[t.e].includerotandscale == 0)
							{
								RotateObject(t.tlmobj, 0, 0, 0);
							}
							else
							{
								RotateObject(t.tlmobj, t.entityelement[t.e].rx, t.entityelement[t.e].ry, t.entityelement[t.e].rz);
								ScaleObject(t.tlmobj, 100 + t.entityelement[t.e].scalex, 100 + t.entityelement[t.e].scaley, 100 + t.entityelement[t.e].scalez);
							}
							// ensure batched object shares collision property of parent entity
							if (t.entityelement[t.e].staticflag == 1)
							{
								t.tentid = t.entityelement[t.e].bankindex;
								if (t.entityprofile[t.tentid].canseethrough == 1)
								{
									SetObjectCollisionProperty(t.tlmobj, 1);
								}
							}
							if (t.entityprofile[t.tentid].ischaracter == 0)
							{
								if (t.entityprofile[t.tentid].collisionmode == 11)
								{
									SetObjectCollisionProperty(t.tlmobj, 1);
								}
							}
						}
						// assign cull and transparency modes from parent settings
						t.entid = 0; if (t.e <= ArrayCount(t.entityelement))  t.entid = t.entityelement[t.e].bankindex;
						if (t.entid > 0)
						{
							// apply certain settings lost in the conversion
							if (t.entityprofile[t.entid].cullmode >= 0)
							{
								if (t.entityprofile[t.entid].cullmode != 0)
								{
									SetObjectCull(t.tlmobj, 0);
								}
								else
								{
									SetObjectCull(t.tlmobj, 1);
								}
							}
							if (t.entityelement[t.e].eleprof.transparency >= 0)
							{
								SetObjectTransparency(t.tlmobj, t.entityelement[t.e].eleprof.transparency);
							}
						}
					}
				}
			}
		}
	}

	// Load in terrain shadow objects
	timestampactivity(0,"loading LMO terrain files");
	t.liftshadowstositontopofterrain_f = 0.1f;
	if ( g.lightmappedterrainoffsetfinish>g.lightmappedterrainoffset ) 
	{
		for ( t.tlmobj2 = g.lightmappedterrainoffset ; t.tlmobj2<=  g.lightmappedterrainoffsetfinish; t.tlmobj2++ )
		{
			t.tfile_s=t.lightmapper.lmpath_s+"object"+Str(t.tlmobj2)+".dbo";
			if ( ObjectExist(t.tlmobj2) == 1  )  DeleteObject (  t.tlmobj2 );
			if ( FileExist(t.tfile_s.Get()) == 1 ) 
			{
				if ( g.iLightmappingExcludeTerrain == 0 )
				{
					LoadObject (  t.tfile_s.Get(),t.tlmobj2 );
					SetObjectMask (  t.tlmobj2, 1 );
					SetObjectCollisionProperty (  t.tlmobj2,1 );
					// NOTE; Can optimize for shader here by making these objects world position based!
					PositionObject (  t.tlmobj2,t.lightmappedterrain[t.tlmobj2-g.lightmappedterrainoffset][0],t.liftshadowstositontopofterrain_f,t.lightmappedterrain[t.tlmobj2-g.lightmappedterrainoffset][1] );
					SetObjectTransparency (  t.tlmobj2, 6 );
					DisableObjectZWrite ( t.tlmobj2 );
					lm_zbias ( );
				}
				else
				{
					// dummy object eventually hidden from draw
					MakeObjectPlane ( t.tlmobj2, 0, 0 );
					SetObjectMask ( t.tlmobj2, 0 );
					HideObject ( t.tlmobj2 );
				}
			}
		}
	}
	UnDim (  t.lightmappedterrain );

	// finished generating mipmaps
	SetImageAutoMipMap (  0 );

	// If there has been any LM loading, use PRE-BAKE technique
	if ( t.tlightmapfilesexist == 1 ) 
	{
		t.visuals.shaderlevels.lighting=1;
		t.visuals.refreshshaders=1;
	}

	// confirm the load was successful
	t.tlmloadsuccess=1;

	// add lightmapped objects as occludees
	for ( t.tlmobj = g.lightmappedobjectoffset; t.tlmobj<= g.lightmappedobjectoffsetfinish; t.tlmobj++ )
	{
		if ( ObjectExist(t.tlmobj) == 1 ) 
		{
			CPU3DAddOccludee ( t.tlmobj , false );
			if ( ( ObjectSizeX(t.tlmobj,1)>MINOCCLUDERSIZE && ObjectSizeY(t.tlmobj,1)>MINOCCLUDERSIZE ) || ( ObjectSizeZ(t.tlmobj,1)>MINOCCLUDERSIZE && ObjectSizeY(t.tlmobj,1)>MINOCCLUDERSIZE ) ) 
				CPU3DAddOccluder (  t.tlmobj );
		}
	}

	// add terrain lightmap objects as occludees
	if ( g.iLightmappingExcludeTerrain == 0 )
	{
		if ( g.lightmappedterrainoffset != -1 ) 
		{
			for ( t.tlmobj2 = g.lightmappedterrainoffset ; t.tlmobj2<=  g.lightmappedterrainoffsetfinish; t.tlmobj2++ )
			{
				if ( ObjectExist(t.tlmobj2) == 1 ) 
				{
					CPU3DAddOccludee ( t.tlmobj2 , false );
					if ( (ObjectSizeX(t.tlmobj2,1)>MINOCCLUDERSIZE && ObjectSizeY(t.tlmobj2,1)>MINOCCLUDERSIZE) || (ObjectSizeZ(t.tlmobj2,1)>MINOCCLUDERSIZE && ObjectSizeY(t.tlmobj2,1)>MINOCCLUDERSIZE) ) 
						CPU3DAddOccluder (  t.tlmobj2 );
				}
			}
		}
	}

	// set globals used by the occluder
	lightmappedterrainoffset = g.lightmappedterrainoffset;
	lightmappedobjectoffset = g.lightmappedobjectoffset;
}

void lm_preplmobj ( void )
{
	//  takes tlmobj,e,tentid
	if (  t.e <= ArrayCount(t.entityelement) ) 
	{
		PositionObject (  t.tlmobj,t.entityelement[t.e].x,t.entityelement[t.e].y,t.entityelement[t.e].z );
		RotateObject (  t.tlmobj,t.entityelement[t.e].rx,t.entityelement[t.e].ry,t.entityelement[t.e].rz );
		ScaleObject (  t.tlmobj,100+t.entityelement[t.e].scalex,100+t.entityelement[t.e].scaley,100+t.entityelement[t.e].scalez );
	}
	SetObjectMask (  t.tlmobj, 1 );
}

void lm_handleshaders ( void )
{
	//  Apply effect to new lightmapped objects
	for ( t.e = 1 ; t.e<=  g.glmsceneentitymax; t.e++ )
	{
		if (  t.lmsceneobj[t.e].lmvalid == 3 ) 
		{
			// 081215 - also allow minus values (indicating special case LM object)
			if (  t.lmsceneobj[t.e].startobj != 0 ) // > 0 ) 
			{
				t.entid = 0 ; if (  t.e <= ArrayCount(t.entityelement)  )  t.entid = t.entityelement[t.e].bankindex;
				for ( t.tlmobj = abs(t.lmsceneobj[t.e].startobj); t.tlmobj<= t.lmsceneobj[t.e].finishobj; t.tlmobj++ )
				{
					if (  ObjectExist(t.tlmobj) == 1 ) 
					{
						// 100718 - important no shaders are applied to LM entity objects (done when loading in game engine)
						SetObjectEffect ( t.tlmobj, 0 );
						SetObjectWireframe ( t.tlmobj,0 );
						if (t.entid > 0)
						{
							// apply certain settings lost in the conversion
							if (t.entityprofile[t.entid].cullmode >= 0)
							{
								if (t.entityprofile[t.entid].cullmode != 0)
								{
									SetObjectCull(t.tlmobj, 0);
								}
								else
								{
									SetObjectCull(t.tlmobj, 1);
								}
							}
							if (t.entityelement[t.e].eleprof.transparency >= 0)
							{
								SetObjectTransparency(t.tlmobj, t.entityelement[t.e].eleprof.transparency);
							}
						}
					}
				}
			}
			t.lmsceneobj[t.e].lmvalid=1;
		}
	}

	//  apply super simple shadow shader
	for ( t.tlmobj2 = g.lightmappedterrainoffset ; t.tlmobj2<=  g.lightmappedterrainoffsetfinish; t.tlmobj2++ )
	{
		if (  ObjectExist(t.tlmobj2) == 1 ) 
		{
			SetObjectEffect (  t.tlmobj2,g.staticshadowlightmapeffectoffset );
			ShowObject (  t.tlmobj2 );
		}
	}
}

int findlightmaptexturefilenameindex ( char* file_s )
{
	int treturnlightmapindex = 0;
	int tfoundlightmappath = 0;
	int filesize = 0;
	int mbi = 0;
	cstr n_s =  "";
	int b = 0;
	int c = 0;
	treturnlightmapindex=-1;
	if (  FileExist(file_s) == 1 ) 
	{
	filesize=FileSize(file_s);
	mbi=255;
	OpenToRead (  11,file_s );
	MakeMemblockFromFile (  mbi,11 );
	CloseFile (  11 );
	for ( b = 0 ; b<=  filesize-1; b++ )
	{
		tfoundlightmappath=0;
		if (  ReadMemblockByte(mbi,b+0) == Asc("l") ) 
		{
			if (  ReadMemblockByte(mbi,b+1) == Asc("i") ) 
			{
				if (  ReadMemblockByte(mbi,b+2) == Asc("g") ) 
				{
					if (  ReadMemblockByte(mbi,b+3) == Asc("h") ) 
					{
						if (  ReadMemblockByte(mbi,b+4) == Asc("t") ) 
						{
							if (  ReadMemblockByte(mbi,b+5) == Asc("m") ) 
							{
								if (  ReadMemblockByte(mbi,b+6) == Asc("a") ) 
								{
									if (  ReadMemblockByte(mbi,b+7) == Asc("p") ) 
									{
										if (  ReadMemblockByte(mbi,b+8) == Asc("s") ) 
										{
											if (  ReadMemblockByte(mbi,b+9) == Asc("\\") ) 
											{
												tfoundlightmappath=1;
											}
										}
									}
								}
							}
						}
					}
				}
			}
			if (  tfoundlightmappath == 1 ) 
			{
				n_s="";
				for ( c = b+10 ; c<=  b+18; c++ )
				{
					if (  ReadMemblockByte(mbi,c) == Asc(".") ) 
					{
						break;
					}
					else
					{
						n_s=n_s+Chr(ReadMemblockByte(mbi,c));
					}
				}
				//  found the numeric of the lightmap texture
				treturnlightmapindex=ValF(n_s.Get());
				b=filesize ; break;
			}
		}
	}
	DeleteMemblock (  mbi );
	}
//endfunction treturnlightmapindex
	return treturnlightmapindex;
}
