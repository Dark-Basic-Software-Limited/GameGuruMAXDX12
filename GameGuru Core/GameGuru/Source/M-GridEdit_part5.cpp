float editor_forceentityfindfloor (bool bPredictMode)
{
	// earl out if t.gridentityinzoomview out of bounds
	if (t.gridentityinzoomview >= g.entityelementmax) return 0.0f;
	//LB: And some extra protection (backs up PE addition below)
	if (t.gridentityinzoomview >= t.entityelement.size()) return 0.0f;

	// receives; tforceentityfindfloor
	// bPredictMode = set to true when we want to work out where the object will go when forced to floor
	// but without affecting any globals or states
	float fPredictedYPosition = 0.0f;
	int ssgridentityinzoomview, ssgridentitydroptoground;
	float ssgridentityposx_f, ssgridentityposy_f, ssgridentityposz_f;
	int ssgridentityobj, ssthardauto, ssgridentityposoffground, ssgridentityusingsoftauto, ssgridentitysurfacesnap;
	if (bPredictMode == true)
	{
		ssgridentityinzoomview = t.gridentityinzoomview;
		ssgridentitydroptoground = t.gridentitydroptoground;
		ssgridentityposx_f = t.gridentityposx_f;
		ssgridentityposy_f = t.gridentityposy_f;
		ssgridentityposz_f = t.gridentityposz_f;
		ssgridentityobj = t.gridentityobj;
		ssthardauto = t.thardauto;
		ssgridentityposoffground = t.gridentityposoffground;
		ssgridentityusingsoftauto = t.gridentityusingsoftauto;
		ssgridentitysurfacesnap = t.gridentitysurfacesnap;
	}
	t.storegridentityinzoomview = t.gridentityinzoomview;
	t.gridentityinzoomview = t.tforceentityfindfloor;
	t.storegridentityposy_f = t.gridentityposy_f;
	t.gridentityposy_f = t.entityelement[t.gridentityinzoomview].y; //PE: Crash , if loading level with less entitys and selecting a object, now reset in new level.
	int iEntPassMax = 1;
	if (g.entityrubberbandlist.size() > 0) iEntPassMax = g.entityrubberbandlist.size();
	for (int iEntPass = 0; iEntPass < iEntPassMax; iEntPass++)
	{
		// which entity are we dealing with
		int e = t.gridentityinzoomview;
		if (!pref.iEnableDragDropEntityMode)
		{
			if (g.entityrubberbandlist.size() > 0)
			{
				e = g.entityrubberbandlist[iEntPass].e;
			}
		}
		else
		{
			if (g.entityrubberbandlist.size() > 0)
			{
				// Ignore if entity is part of group. It would split up the group.
				e = g.entityrubberbandlist[iEntPass].e;

				// Dont allow find floor if entity is part of group.
				int grouplist = isEntityInGroupList(e);
				if (grouplist >= 0) e = 0;
			}
		}
		if ((e > 0 && t.entityelement[e].editorlock == 0) || bPredictMode == true)
		{
			// if RETURN key pressed
			if (bPredictMode == true)
				t.gridentityposy_f = ObjectPositionY(t.gridentityobj);
			else
				t.gridentityposy_f = t.entityelement[e].y;
			if (t.inputsys.keyreturn == 1 || bPredictMode == true)
			{
				// store globs in store
				t.storegridentitydroptoground = t.gridentitydroptoground;
				t.storegridentityposx_f = t.gridentityposx_f;
				t.storegridentityposz_f = t.gridentityposz_f;
				t.storegridentityobj = t.gridentityobj;
				t.storegridentityposoffground = t.gridentityposoffground;
				t.gridentitydroptoground = 1;
				if (bPredictMode == true)
				{
					t.gridentityposx_f = ObjectPositionX(t.gridentityobj);
					t.gridentityposz_f = ObjectPositionZ(t.gridentityobj);
				}
				else
				{
					t.gridentityposx_f = t.entityelement[e].x;
					t.gridentityposz_f = t.entityelement[e].z;
					t.gridentityobj = t.entityelement[e].obj;
				}
				t.thardauto = 1; editor_findentityground();
				if (t.gridentityposoffground == 0)
				{
					float ftmp = BT_GetGroundHeight(t.terrain.TerrainID, t.gridentityposx_f, t.gridentityposz_f);
					t.gridentityposy_f = ftmp;
					ApplyPivotToGridEntity();
					if (t.entityprofile[t.gridentity].ismarker != 0)  t.gridentityposy_f = t.gridentityposy_f + t.entityprofile[t.gridentity].offy;
					if (t.entityprofile[t.gridentity].defaultheight != 0)  t.gridentityposy_f = t.gridentityposy_f + t.entityprofile[t.gridentity].defaultheight;
				}
				if (bPredictMode == false)
				{
					t.entityelement[e].x = t.gridentityposx_f;
					t.entityelement[e].z = t.gridentityposz_f;
				}

				// restore globs from store
				t.gridentitydroptoground = t.storegridentitydroptoground;
				t.gridentityposx_f = t.storegridentityposx_f;
				t.gridentityposz_f = t.storegridentityposz_f;
				t.gridentityobj = t.storegridentityobj;
				t.gridentityposoffground = t.storegridentityposoffground;
				t.gridentityusingsoftauto = 1;
				t.gridentitysurfacesnap = 0;
			}
			if (bPredictMode == false)
			{
				if (t.tforcepguppgdnkeys == 1)
				{
					editor_handlepguppgdn();
				}
				if (t.entityelement[e].y != t.gridentityposy_f)
				{
					t.entityelement[e].beenmoved = 1;
				}
				t.entityelement[e].y = t.gridentityposy_f;
				if (t.entityelement[e].obj > 0)
				{
					if (ObjectExist(t.entityelement[e].obj) == 1)
					{
						PositionObject(t.entityelement[e].obj, t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z);
					}
				}
			}
			else
			{
				// have the prediction for where the object would be placed
				fPredictedYPosition = t.gridentityposy_f;
			}
		}
	}
	t.gridentityposy_f = t.storegridentityposy_f;
	t.gridentityinzoomview = t.storegridentityinzoomview;
	if (bPredictMode == true)
	{
		// restore any changes and return prediction
		t.gridentityinzoomview = ssgridentityinzoomview;
		t.gridentitydroptoground = ssgridentitydroptoground;
		t.gridentityposx_f = ssgridentityposx_f;
		t.gridentityposy_f = ssgridentityposy_f;
		t.gridentityposz_f = ssgridentityposz_f;
		t.gridentityobj = ssgridentityobj;
		t.thardauto = ssthardauto;
		t.gridentityposoffground = ssgridentityposoffground;
		t.gridentityusingsoftauto = ssgridentityusingsoftauto;
		t.gridentitysurfacesnap = ssgridentitysurfacesnap;
		return fPredictedYPosition;
	}
	else
	{
		// regular usage
		return 0.0f;
	}
}

void editor_viewfunctionality ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	// map view controls
	if ( t.grideditselect == 3 ) 
	{
		if ( t.inputsys.mclick == 1 ) 
		{
			t.stcx_f=t.inputsys.mmx*100.0;
			t.stcy_f=t.inputsys.mmy*100.0;
			t.cx_f=t.stcx_f ; t.cy_f=t.stcy_f ; t.gridzoom_f=t.stgridzoom_f;
			t.grideditselect=t.stgrideditselect ; editor_refresheditmarkers ( );
			while ( t.inputsys.mclick==1 ) { input_getcontrols() ; Sync() ; }
			t.cameraviewmode=0;
			t.updatezoom=1;
		}
	}

	// zoom view controls
	if ( t.grideditselect == 4 ) 
	{
		//  exit zoom view
		#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
		if (!bImGuiGotFocus && bImGuiRenderTargetFocus && t.inputsys.mclick == 1)  t.tpressedtoleavezoommode = 1;

		//PE: We cant leave before bEntity_Properties_Window = false;
		//PE: If we have a wicked "texture" rendered in imgui , changing texture ("clone" after properties) will crash.
		if (bEntity_Properties_Window && !bImGuiGotFocus && bImGuiRenderTargetFocus && t.inputsys.mclick == 0 && t.tpressedtoleavezoommode == 1) bEntity_Properties_Window = false;
		else if (!bImGuiGotFocus && bImGuiRenderTargetFocus && t.inputsys.mclick == 0 && t.tpressedtoleavezoommode == 1)  t.tpressedtoleavezoommode = 2;

		//When properties window open , they should click "apply","cancel".
		if(bProperties_Window_Block_Mouse)
			t.tpressedtoleavezoommode = 0;

		if (bProperties_Window_Block_Mouse) 
		{
			//Must have a release before block is released.
			if (t.inputsys.mclick == 0) 
			{
				bProperties_Window_Block_Mouse = false;
				t.tpressedtoleavezoommode = 0;
			}
		}
		#else
		if ( t.inputsys.mclick == 1  )  t.tpressedtoleavezoommode = 1;
		if ( t.inputsys.mclick == 0 && t.tpressedtoleavezoommode == 1 )  t.tpressedtoleavezoommode = 2;
		#endif

		if ( (t.tpressedtoleavezoommode == 2 || t.inputsys.kscancode == 211) || t.editorinterfaceleave == 1 ) 
		{
			// leave zoomview
			t.inputsys.doautozoomview=1;

			// reset mouse click (must release LMB before zoom mode ends)
			t.tpressedtoleavezoommode=0;

			// close any property window
			interface_closepropertywindow ( );
			t.editorinterfaceleave=0;

			// 310315 - Ensure clipping is restored when return
			t.updatezoom=1;
			#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
			#endif

			// place entity on the map
			if ( t.gridentityinzoomview>0 ) 
			{
				// DELETE key deletes entity no matter what (for fixed entities too)
				if ( t.gridentity != 0 && t.inputsys.kscancode == 211 ) 
				{
					// Delete any associated waypoint/trigger zone
					t.waypointindex=t.grideleprof.trigger.waypointzoneindex;
					if (  t.waypointindex>0 ) 
					{
						t.w=t.waypoint[t.waypointindex].start;
						waypoint_delete ( );
					}
					t.grideleprof.trigger.waypointzoneindex=0;

					// And now delete entity from cursor
					if (  t.gridentityobj == 0 ) 
					{
						DeleteObject (  t.gridentityobj );
						t.gridentityobj=0;
					}
					t.gridentityinzoomview = 0;
				}
				else
				{
					// Add entity back into map
					if (iOldgridentity == t.gridentity) 
					{
						//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
						extern bool bNextObjectMustBeClone;
						bNextObjectMustBeClone = true;

						gridedit_addentitytomap();
						bNextObjectMustBeClone = false;

						t.gridentityinzoomview = 0;
					}
					else 
					{
						timestampactivity(0, "t.gridentity!=lastpropertiesid ?");
					}
					//  Hide widget to make clean return to editor
					t.widget.pickedObject=0  ; widget_updatewidgetobject ( );
				}

				// Reset cursor object settings
				t.refreshgrideditcursor=1;
				t.gridentity=0;
				t.gridedit.autoflatten=0;
				t.gridedit.entityspraymode=0;
				t.gridentityposoffground=0;
				t.gridentityusingsoftauto=1;
				t.gridentitysurfacesnap=1-g.gdisablesurfacesnap;
				// MAX handles its own positioning system
				t.gridentityautofind = 0;
				t.inputsys.dragoffsetx_f=0;
				t.inputsys.dragoffsety_f=0;
				editor_refreshentitycursor ( );

				if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
				if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
				bEntity_Properties_Window = false; //Close Properties window.
			}
		}
	}
}

void editor_findentityground ( void )
{
	//  for entities that can be moved
	if (  t.entityelement[t.gridentityinzoomview].editorfixed == 0 ) 
	{
		//  finds ground
		if ( t.gridentitydroptoground == 1 || (t.thardauto == 0 && t.gridentityusingsoftauto == 1) ) 
		{
			bDetectTerrainOnly = false;

			//PE: MUST disable collision on ALL rubberband objects.
			std::vector<sRubberBandType> entityvisible = g.entityrubberbandlist;
			if (g.entityrubberbandlist.size() > 0)
			{
				for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
				{
					int e = g.entityrubberbandlist[i].e;
					int obj = t.entityelement[e].obj;
					if (obj > 0 && GetVisible(obj))
					{
						entityvisible[i].x = 1;
						HideObject(obj);
					}
					else
					{
						entityvisible[i].x = 0;
					}
				}
			}


			t.tbestdist_f=99999 ; t.tbesty_f=0;
			t.tto_f = t.gridentityposy_f - 9000.0; //Make sure we hit.
			for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
			{
				if ( t.thardauto == 1 ) 
				{
					// regular
					if ( t.entityelement[t.e].editorlock == 0 ) 
					{
						if (ObjectExist(t.gridentityobj) == 1)
						{
							// only allow a stack slightly higher than the height of the object we are stacking (so dont end up on a roof)
							// Rick complainted, so use a fixed high height so we can stack anything, even if on the roof
							t.tfrom_f = t.gridentityposy_f + 200.0f;// fObjectSizeY + fMargin;

							// and make sure never higher than the camera Y (what we can really 'see' generally)
							if (t.tfrom_f > t.gridtrueslicey_f)
							{
								t.tfrom_f = t.gridtrueslicey_f;
							}
						}
					}
					else
					{
						t.tfrom_f=t.gridentityposy_f+75.0;
					}
				}
				else
				{
					//  very subtle surface scan (to defeat small floors)
					t.tfrom_f=t.gridentityposy_f+11.0;
				}
				t.tokay=1;
				if ( t.entityprofile[t.entid].addhandlelimb>0  ) t.tokay = 0;
				if ( t.playercontrol.thirdperson.enabled == 1 ) 
				{
					// if third person char, ignore when finding surface
					if ( t.e == t.playercontrol.thirdperson.charactere  )  t.tokay = 0;
					if ( t.e == t.playercontrol.thirdperson.startmarkere  )  t.tokay = 0;
				}
				if ( t.tokay == 1 ) 
				{
					t.obj=t.entityelement[t.e].obj;
					if ( t.obj>0 && t.obj != t.gridentityobj ) 
					{
						if ( ObjectExist(t.obj) == 1 ) 
						{
							if ( GetVisible(t.obj) == 1 ) 
							{
								// 210415 - added distance check to speed up ground scan
								t.tdiffx_f=ObjectPositionX(t.obj)-t.gridentityposx_f;
								t.tdiffz_f=ObjectPositionZ(t.obj)-t.gridentityposz_f;
								t.tdiff_f=Sqrt(abs(t.tdiffx_f*t.tdiffx_f)+abs(t.tdiffz_f*t.tdiffz_f));
								if ( t.tdiff_f<ObjectSize(t.obj)*2 ) 
								{
									if ( IntersectObject(t.obj,t.gridentityposx_f,t.tfrom_f,t.gridentityposz_f,t.gridentityposx_f,t.tto_f,t.gridentityposz_f) != 0 ) 
									{
										t.tdist_f=abs(ChecklistFValueB(6)-t.tfrom_f);
										if ( t.tdist_f<t.tbestdist_f ) 
										{
											t.tbesty_f=ChecklistFValueB(6);
											t.tbestdist_f=t.tdist_f;
										}
									}
								}
							}
						}
					}
				}
			}
			if ( t.tbestdist_f < 99999 ) 
			{
				// found GetPoint ( where our entity will rest vertically )
				t.gridentityposy_f=t.tbesty_f; t.zoomviewtargety_f=t.tbesty_f;

				// now need entities own thickness from object 0,0,0 to base
				// grid of ray casts for good base detect resolution
				if ( t.gridentityobj>0 && t.thardauto == 1 ) 
				{
					if ( ObjectExist(t.gridentityobj) == 1 ) 
					{
						t.ttentsizex_f=ObjectSizeX(t.gridentityobj)/2.0;
						t.ttentsizez_f=ObjectSizeZ(t.gridentityobj)/2.0;
						if (  t.ttentsizex_f<1.0 && t.ttentsizex_f<t.ttentsizez_f  )  t.ttentsizex_f = t.ttentsizez_f;
						if (  t.ttentsizez_f<1.0 && t.ttentsizez_f<t.ttentsizex_f  )  t.ttentsizez_f = t.ttentsizex_f;
						t.stepvaluex_f=ObjectSizeX(t.gridentityobj)/10.0;
						t.stepvaluez_f=ObjectSizeZ(t.gridentityobj)/10.0;
						if (  t.stepvaluex_f<1  )  t.stepvaluex_f = 1.0;
						if (  t.stepvaluez_f<1  )  t.stepvaluez_f = 1.0;
						if (  ObjectExist(g.entityworkobjectoffset) == 1  )  DeleteObject (  g.entityworkobjectoffset );
						MakeObjectBox (  g.entityworkobjectoffset,ObjectSizeX(t.gridentityobj),ObjectSizeY(t.gridentityobj),ObjectSizeZ(t.gridentityobj) );
						PositionObject (  g.entityworkobjectoffset,ObjectPositionX(t.gridentityobj)+GetObjectCollisionCenterZ(t.gridentityobj),ObjectPositionY(t.gridentityobj)+GetObjectCollisionCenterY(t.gridentityobj),ObjectPositionZ(t.gridentityobj)+GetObjectCollisionCenterZ(t.gridentityobj) );
						RotateObject (  g.entityworkobjectoffset,ObjectAngleX(t.gridentityobj),ObjectAngleY(t.gridentityobj),ObjectAngleZ(t.gridentityobj) );
						HideObject (  g.entityworkobjectoffset );
						t.tsmallest_f=99999;
						t.tscbase_f=ObjectPositionY(g.entityworkobjectoffset)-(ObjectSizeY(g.entityworkobjectoffset)*2);
						if (  t.tsmallest_f<99999 ) 
						{
							t.tthickness_f=ObjectPositionY(t.gridentityobj)-(t.tscbase_f+t.tsmallest_f);
						}
						else
						{
							t.tthickness_f=0;
						}
						if (  ObjectExist(g.entityworkobjectoffset) == 1  )  DeleteObject (  g.entityworkobjectoffset );
						t.gridentityposy_f=t.tbesty_f+t.tthickness_f ; t.zoomviewtargety_f=t.tbesty_f+t.tthickness_f;
					}
				}
				// ensure a 'autofoundYpos' never drops BELOW Floor ( ( (i.e. skull under Floor) ) )
				float ftmp = BT_GetGroundHeight(t.terrain.TerrainID, t.gridentityposx_f, t.gridentityposz_f);
				t.trygridentityposy_f = ftmp;
				ApplyPivotToGridEntity();
				if (  t.gridentityposy_f<t.trygridentityposy_f ) 
				{
					t.gridentityposy_f = t.trygridentityposy_f;
					ApplyPivotToGridEntity();
					if (  t.entityprofile[t.gridentity].ismarker != 0  )  t.gridentityposy_f = t.gridentityposy_f + t.entityprofile[t.gridentity].offy;
					if (  t.entityprofile[t.gridentity].defaultheight != 0  )  t.gridentityposy_f = t.gridentityposy_f + t.entityprofile[t.gridentity].defaultheight;
				}
				//  we are sitting on an entity, no need for ground terrain resting
				t.gridentityposoffground=1;
			}
			else
			{
				//  if not find any entities, use terrain ground base
				t.gridentityposoffground=0;
			}

			//PE: Reenable rubberband collision.
			if (entityvisible.size() > 0)
			{
				for (int i = 0; i < (int)entityvisible.size(); i++)
				{
					int e = entityvisible[i].e;
					int obj = t.entityelement[e].obj;
					if (entityvisible[i].x == 1)
					{
						ShowObject(obj);
					}
				}
			}
		}
		else
		{
			if (!(t.gridentitydroptoground == 2 && t.thardauto == 1))
			{
				if (bDetectTerrainOnly && t.gridentity > 0 && t.gridentityobj > 0)
				{
					float newy = 0.0f;
					newy = BT_GetGroundHeight(t.terrain.TerrainID, t.gridentityposx_f, t.gridentityposz_f);
					if (newy != 0.0f)
					{
						t.gridentityposy_f = newy;
						//PE: Apply pivot here.
						bool bTmp = bExtractFixPivot;
						bExtractFixPivot = true;
						ApplyPivotToGridEntity();
						bExtractFixPivot = bTmp;
					}
				}
			}
		}

		// finds wall
		if ( t.gridentitydroptoground == 2 && t.thardauto == 1 ) 
		{
			bDetectTerrainOnly = false;

			t.tbestdist_f=99999 ; t.tbestx_f=0 ; t.tbestz_f=0;
			t.tbesty_f=t.gridentityposy_f+GetObjectCollisionCenterY(t.gridentityobj);
			t.a=t.gridentityrotatey_f;
			t.tfromx=NewXValue(t.gridentityposx_f,t.a,-5.0) ; t.ttox=NewXValue(t.gridentityposx_f,t.a,75.0);
			t.tfromz=NewZValue(t.gridentityposz_f,t.a,-5.0) ; t.ttoz=NewZValue(t.gridentityposz_f,t.a,75.0);
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.obj=t.entityelement[t.e].obj;
				if (  t.obj>0 && t.obj != t.gridentityobj ) 
				{
					if (  ObjectExist(t.obj) == 1 ) 
					{
						if (  GetVisible(t.obj) == 1 ) 
						{
							t.tdist_f=IntersectObject(t.obj,t.tfromx,t.tbesty_f,t.tfromz,t.ttox,t.tbesty_f,t.ttoz);
							if (  t.tdist_f != 0 ) 
							{
								if (  t.tdist_f<t.tbestdist_f ) 
								{
									t.tbestx_f=ChecklistFValueA(6);
									t.tbestz_f=ChecklistFValueC(6);
									t.tbestdist_f=t.tdist_f;
								}
							}
						}
					}
				}
			}
			if (  t.tbestdist_f<99999 ) 
			{
				//  found GetPoint (  where our entity will rest on wall )
				//  now need entities own thickness from object 0,0,0 to wall-contact
				t.tbestx_f=NewXValue(t.tbestx_f,t.a+180,-5.0);
				t.tbestz_f=NewZValue(t.tbestz_f,t.a+180,-5.0);
				t.ttox=NewXValue(t.tbestx_f,t.a+180,100.0);
				t.ttoz=NewZValue(t.tbestz_f,t.a+180,100.0);
				if (  ObjectExist(g.entityworkobjectoffset) == 1  )  DeleteObject (  g.entityworkobjectoffset );
				MakeObjectBox (  g.entityworkobjectoffset,ObjectSizeX(t.gridentityobj),ObjectSizeY(t.gridentityobj),ObjectSizeZ(t.gridentityobj) );
				PositionObject (  g.entityworkobjectoffset,ObjectPositionX(t.gridentityobj)+GetObjectCollisionCenterZ(t.gridentityobj),ObjectPositionY(t.gridentityobj)+GetObjectCollisionCenterY(t.gridentityobj),ObjectPositionZ(t.gridentityobj)+GetObjectCollisionCenterZ(t.gridentityobj) );
				RotateObject (  g.entityworkobjectoffset,ObjectAngleX(t.gridentityobj),ObjectAngleY(t.gridentityobj),ObjectAngleZ(t.gridentityobj) );
				HideObject (  g.entityworkobjectoffset );
				t.tgap_f=IntersectObject(g.entityworkobjectoffset,t.tbestx_f,t.tbesty_f,t.tbestz_f,t.ttox,t.tbesty_f,t.ttoz);
				if (  t.tgap_f >= 4.9 ) 
				{
					t.tgapx_f=ChecklistFValueA(6);
					t.tgapz_f=ChecklistFValueC(6);
					t.ttddx_f=t.tgapx_f-ObjectPositionX(t.gridentityobj);
					t.ttddz_f=t.tgapz_f-ObjectPositionZ(t.gridentityobj);
					t.tthickness_f=5.0+Sqrt(abs(t.ttddx_f*t.ttddx_f)+abs(t.ttddz_f*t.ttddz_f));
				}
				else
				{
					t.tthickness_f=5.0;
				}
				t.tbestx_f=NewXValue(t.tbestx_f,t.a+180,t.tthickness_f+0.5);
				t.tbestz_f=NewZValue(t.tbestz_f,t.a+180,t.tthickness_f+0.5);
				t.gridentityposx_f=t.tbestx_f ; t.zoomviewtargetx_f=t.tbestx_f;
				t.gridentityposz_f=t.tbestz_f ; t.zoomviewtargetz_f=t.tbestz_f;
				if (  ObjectExist(g.entityworkobjectoffset) == 1  )  DeleteObject (  g.entityworkobjectoffset );
			}
		}
	}
}

void editor_refresheditmarkers ( void )
{
	//  Deactivate widget if still in effect
	widget_switchoff ( );

	//  Deactivate floating selection of entity
	if ( t.grideditselect != 5 && t.grideditselect != 4 ) 
	{
		if ( t.grideditselect != 5 ) HideObject ( t.editor.objectstartindex+5 );
		t.gridentity=0 ; t.gridentityposoffground=0;
		t.gridentityusingsoftauto=0;
		t.gridentitysurfacesnap=1-g.gdisablesurfacesnap;
		// MAX handles its own positioning system
		t.gridentityautofind = 0;
		t.inputsys.dragoffsetx_f=0;
		t.inputsys.dragoffsety_f=0;
	}

	//  Update entity cursor? (delete many of these as it WAS old shroud updater!)
	t.refreshgrideditcursor=1;

	//  Update clipboard items based on mode
	editor_cutcopyclearstate ( );

	//  Waypoint visibility
	if (  t.grideditselect != t.lastgrideditselect ) 
	{
		t.lastgrideditselect=t.grideditselect;
		if (  t.grideditselect == 6 ) 
		{
			waypoint_showallpaths ( );
		}
		else
		{
			if (  t.inputsys.dowaypointview == 0 ) 
			{
				waypoint_showallpaths ( );
			}
			else
			{
				waypoint_hideallpaths ( );
			}
		}
	}

	// clear any gridentity light if gridentity no longer used
	if (t.gridentity == 0)
	{
		if (t.gridentitywickedlightindex > 0)
		{
			WickedCall_DeleteLight(t.gridentitywickedlightindex);
			t.gridentitywickedlightindex = 0;
		}
	}
}

void editor_visuals ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	//  Control entity selection and alpha of layers
	if (  t.refreshgrideditcursor == 1 ) 
	{
		gridedit_recreateentitycursor ( );
		t.refreshgrideditcursor=0;
	}
	gridedit_displayentitycursor ( );

	//  Update Camera
	editor_camera ( );
}

void editor_camera(void)
{
	// Camera Mode
	float fFlatFloorY = BT_GetGroundHeight(t.terrain.TerrainID, t.editorfreeflight.c.x_f, t.editorfreeflight.c.z_f) + 100.0f;
	if (!bProceduralLevel && !bStoryboardWindow && !bWelcomeScreen_Window && t.game.gameisexe == 0 && !bImGuiInTestGame )
	{
		bool bCameraOutSideEditArea = false;
		float fEditableSizeHalved = GGTerrain_GetEditableSize();
		t.terraineditableareasizeminx = -fEditableSizeHalved;
		t.terraineditableareasizeminz = -fEditableSizeHalved;
		t.terraineditableareasizemaxx = fEditableSizeHalved;
		t.terraineditableareasizemaxz = fEditableSizeHalved;
		if (CameraPositionX() < t.terraineditableareasizeminx) { bCameraOutSideEditArea = true; }
		if (CameraPositionX() > t.terraineditableareasizemaxx) { bCameraOutSideEditArea = true; }
		if (CameraPositionZ() < t.terraineditableareasizeminz) { bCameraOutSideEditArea = true; }
		if (CameraPositionZ() > t.terraineditableareasizemaxz) { bCameraOutSideEditArea = true; }
		if (bCameraOutSideEditArea)
		{
			//Trigger warning.
			sprintf(cSmallTriggerMessage, "Outside of editable area, you cannot add objects or change the terrain here. Press spacebar to recenter.");

			if(t.inputsys.keyspace == 1)
			{
				// Recentre camera.
				t.inputsys.keyspace = 0;
				
				// Get terrain height at centre.
				float yHit = 0.0f;
				GGTerrain::GGTerrain_GetHeight(0, 0, &yHit);
				yHit += 100.0f;

				// Ensure the camera will be placed above the water.
				if (yHit < (g.gdefaultwaterheight + 100.0f))
					yHit = g.gdefaultwaterheight + 100.0f;

				t.editorfreeflight.mode = 3;
				t.editorfreeflight.s.x_f = 0;
				t.editorfreeflight.s.y_f = yHit;
				t.editorfreeflight.s.z_f = 0;
				t.editorfreeflight.s.angx_f = 0.0f;
				t.editorfreeflight.s.angy_f = 0.0f;
				t.editorfreeflight.c = t.editorfreeflight.s;
			
				
			}
			iTriggerMessageFrames = 15;
			bTriggerSmallMessage = true;

		}
	}

	static bool bPressedFKey = false;
	switch ( t.cameraviewmode )
	{
		//PE: Disable zoom view. in wicked , i got some wierd results with lensflare from wicked so you could not see the object.
		//PE: Also the zoom it is a distraction.
		case 2:
		case 0:
		{
			//  Control free flight camera viewing angle (mouselook)
			#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
			//PE: Delta already reset , so use t.inputsys.xmousemove,y
			if (g.gminvert == 1)  t.ttmousemovey = t.inputsys.xmousemove*-1; else t.ttmousemovey = t.inputsys.ymousemove;
			t.cammousemovex_f = t.inputsys.xmousemove;
			#else
			if (g.gminvert == 1)  t.ttmousemovey = MouseMoveY()*-1; else t.ttmousemovey = MouseMoveY();
			t.cammousemovex_f = MouseMoveX();
			#endif

			//PE: If outside 3D area.
			static bool bBlockRightMouseButton = false;
			if (ImGui::IsMouseDown(1) && !bImGuiRenderTargetFocus) bBlockRightMouseButton = true;
			if (!ImGui::IsMouseDown(1))  bBlockRightMouseButton = false;
			if(bImGuiRenderTargetFocus && !bBlockRightMouseButton)

			{
				t.cammousemovey_f = t.ttmousemovey;
				if (t.inputsys.mclick == 0)  t.inputsys.mclickreleasestate = 0;
				t.trmb = 0;
				if (t.inputsys.mclick == 2 && t.inputsys.mclickreleasestate == 0)
				{
					#if defined(ENABLEIMGUI) && !defined(USEOLDGUI)
					if (g.mouseishidden == 1)
					{
						ImVec2 setPos;
						//PE: Always center relative to window position , or you cant have a small window at the right of screen.
						RECT rect;
						GetWindowRect(g_pGlob->hWnd, &rect);
						setPos = { rect.left + (OldrenderTargetSize.x*0.5f) + OldrenderTargetPos.x , rect.top + (OldrenderTargetSize.y*0.5f) + OldrenderTargetPos.y };
						setPos.x = (int)setPos.x;
						setPos.y = (int)setPos.y;
						SetCursorPos(setPos.x, setPos.y);
						// Convert to the coordinate system ImGui::GetMousePos() uses:
						// With ViewportsEnable ON (DX11): screen coords — setPos is already correct
						// With ViewportsEnable OFF (DX12): client coords via ScreenToClient
						if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
						{
							POINT clientPt = { (LONG)setPos.x, (LONG)setPos.y };
							ScreenToClient(g_pGlob->hWnd, &clientPt);
							xmouseold = clientPt.x;
							ymouseold = clientPt.y;
						}
						else
						{
							xmouseold = setPos.x;
							ymouseold = setPos.y;
						}
					}
					#endif
					t.trmb = 1;
				}
				if (t.inputsys.mclick == 4 && t.inputsys.mclickreleasestate == 0)  t.trmb = 2;
				if (t.trmblock == 0)
				{
					if (t.cammousemovex_f != 0 || t.cammousemovex_f != 0 || t.inputsys.kscancode != 0)  t.trmblock = 1;
				}
				else
				{
					if (t.inputsys.mclick == 0)  t.trmblock = 0;
				}
				if (t.trmblock == 0)  t.trmb = 0;
				if (g.globals.disablefreeflight == 1)  t.trmb = 0;
				if (t.trmb != 0)
				{
					if (g.mouseishidden == 0)
					{
						game_hidemouse();
						#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
						POINT tmp;
						GetCursorPos(&tmp);
						t.editorfreeflight.storemousex = tmp.x;
						t.editorfreeflight.storemousey = tmp.y;
						#else
						t.editorfreeflight.storemousex = t.inputsys.xmouse;
						t.editorfreeflight.storemousey = t.inputsys.ymouse;
						#endif
					}
					if (t.editorfreeflight.mode == 0)
					{
						t.editorfreeflight.mode = 1; t.updatezoom = 1;
						t.editorfreeflight.c.x_f = t.cx_f;
						t.editorfreeflight.c.y_f = fFlatFloorY + (50.0f*t.gridzoom_f);
						t.editorfreeflight.c.z_f = t.cy_f;
						t.editorfreeflight.c.angx_f = CameraAngleX();
						t.editorfreeflight.c.angy_f = CameraAngleY();
					}
					else
					{
						if (t.trmb == 1)
						{
							// rotate with RMB
							#if defined(ENABLEIMGUI) && !defined(USEOLDIDE)
							//PE: a bit more smooth.
							t.tRotationDivider_f = 6.0;
							#else
							t.tRotationDivider_f = 5.0;
							#endif
							t.editorfreeflight.c.angx_f = CameraAngleX() + (t.cammousemovey_f / t.tRotationDivider_f);
							t.editorfreeflight.c.angy_f = CameraAngleY() + (t.cammousemovex_f / t.tRotationDivider_f);
							if (t.editorfreeflight.c.angx_f > 180.0f)  t.editorfreeflight.c.angx_f = t.editorfreeflight.c.angx_f - 360.0f;
							if (t.editorfreeflight.c.angx_f < -89.999f)  t.editorfreeflight.c.angx_f = -89.999f;
							if (t.editorfreeflight.c.angx_f > 89.999f)  t.editorfreeflight.c.angx_f = 89.999f;
						}
					}
					//Always display skybox.
					sky_loop();
				}
				else
				{
					if (g.mouseishidden == 1)
					{
						t.tideframestartx = 70; t.tideframestarty = 15;
						#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
						//PE: Restore mouse pos.
						SetCursorPos(t.editorfreeflight.storemousex, t.editorfreeflight.storemousey);
						// Convert stored screen coords to ImGui coordinate system
						if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
						{
							POINT clientPt = { (LONG)t.editorfreeflight.storemousex, (LONG)t.editorfreeflight.storemousey };
							ScreenToClient(g_pGlob->hWnd, &clientPt);
							xmouseold = clientPt.x;
							ymouseold = clientPt.y;
						}
						else
						{
							xmouseold = t.editorfreeflight.storemousex;
							ymouseold = t.editorfreeflight.storemousey;
						}
						t.inputsys.xmouse = xmouseold;
						t.inputsys.xmouse = ymouseold;
						game_showmouse();
						g.mouseishidden = 0;
						#else
						t.inputsys.xmouse = ((t.tideframestartx + t.editorfreeflight.storemousex + 0.0) / 800.0)*(GetDisplayWidth() + 0.0);
						t.inputsys.ymouse = ((t.tideframestarty + t.editorfreeflight.storemousey + 0.0) / 600.0)*(GetDisplayHeight() + 0.0);
						game_showmouse_restore_mouse(); //PE: Will use exact mouse positon stored by editor when hiding mouse. (if available, else t.tideframestartx...)
						#endif
						t.terrain.X_f = 999999; t.terrain.Y_f = 999999;
					}
				}
			}
			static int delayedNewLevelCamera = 0;
			if (delayedNewLevelCamera > 0)
			{
				delayedNewLevelCamera--;
				//PE: It can take up to 60 frames before we have a terrain height here. so just keep checking.
				t.tcurrenth_f = BT_GetGroundHeight(t.terrain.TerrainID, t.editorfreeflight.c.x_f, t.editorfreeflight.c.z_f);
				if (t.tcurrenth_f != GGORIGIN_Y) delayedNewLevelCamera = 0; //We got a height.
				if (delayedNewLevelCamera == 0)
				{
					t.editorfreeflight.mode = 3;
					t.editorfreeflight.s = t.editorfreeflight.c;
					t.editorfreeflight.s.y_f = t.tcurrenth_f + 325.0; //125.0;
					t.editorfreeflight.s.angy_f = -45.0f;
					t.editorfreeflight.s.angx_f = 16.0f;
				}
			}
			// Handle free flight camea movement
			if (t.inputsys.k_s != "f") bPressedFKey = false;
			if (t.editorfreeflight.mode == 0)
			{
				t.editorfreeflight.c.x_f = t.cx_f;
				t.editorfreeflight.c.y_f = fFlatFloorY + (50.0f*t.gridzoom_f);
				t.editorfreeflight.c.z_f = t.cy_f;
				bool bSwitchToFFView = false;
				if (g_bResetCameraToFreeFlightOnNewLevel == true) 
				{ 
					// extra condition to only 'zoom in' after Welcome screen exits
					// as it looks cooler and hides the launch load stutters
					if (iTriggerWelcomeSystemStuff == 0)
					{
						bSwitchToFFView = true;
						g_bResetCameraToFreeFlightOnNewLevel = false;
					}
				}
				if (t.inputsys.k_s == "f" && bPressedFKey == false && g.globals.disablefreeflight == 0) { bPressedFKey = true;  bSwitchToFFView = true; }
				if (bSwitchToFFView == true )
				{
					// top down back to last free flight
					t.editorfreeflight.mode = 3;

					t.editorfreeflight.s = t.editorfreeflight.c;

					t.tcurrenth_f = BT_GetGroundHeight(t.terrain.TerrainID, t.editorfreeflight.c.x_f, t.editorfreeflight.c.z_f);
					if (t.tcurrenth_f != GGORIGIN_Y)
					{
						t.editorfreeflight.s.y_f = t.tcurrenth_f + 325.0; //125.0;
					}
					else
					{
						//Start up high. as we dont have the real height at this point.
						t.editorfreeflight.s.y_f = 2600.0;
						delayedNewLevelCamera = 100; //PE: Terrain need a few frames before we can set the camera.
					}

					t.editorfreeflight.s.angy_f = -45.0f;
					t.editorfreeflight.s.angx_f = 16.0f;

				}
			}
			if ( t.editorfreeflight.mode == 1 ) 
			{
				static float fAccelerationTimer = 0.0f;
				if (t.inputsys.k_s == "f"  && bPressedFKey == false && t.inputsys.keycontrol == 0 && t.importer.importerActive == 0)
				{
					// free flight to top down
					bPressedFKey = true;
					t.editorfreeflight.s=t.editorfreeflight.c;
					t.cx_f=t.editorfreeflight.c.x_f;
					t.cy_f=t.editorfreeflight.c.z_f;
					t.editorfreeflight.mode=2;
				}
				if (  t.inputsys.keyup == 1  )  t.plrkeyW = 1; else t.plrkeyW = 0;
				if (  t.inputsys.keyleft == 1  )  t.plrkeyA = 1; else t.plrkeyA = 0;
				if (  t.inputsys.keydown == 1  )  t.plrkeyS = 1; else t.plrkeyS = 0;
				if (  t.inputsys.keyright == 1  )  t.plrkeyD = 1; else t.plrkeyD = 0;

				//  mouse wheel mimmics W and S when no CONTROL key pressed (170616 - but not when in EBE mode as its used for grid layer control)
				int usingWheel = 0;
				if ( t.ebe.on == 0 )
				{
					if (  t.inputsys.keycontrol == 0 ) 
					{
						if (  t.inputsys.wheelmousemove<0 || t.inputsys.dozoomout == 1) { t.plrkeyS = 1; usingWheel = 1; }
						if (  t.inputsys.wheelmousemove>0 || t.inputsys.dozoomin == 1 ) { t.plrkeyW = 1; usingWheel = 1; }
					}
				}
				t.traise_f=0.0;
				if (  t.inputsys.keyshift == 1 ) 
				{
					fAccelerationTimer += g.timeelapsed_f * 0.005f;
					if (fAccelerationTimer > 1.0f) fAccelerationTimer = 1.0f;
					t.tffcspeed_f=10.0*g.timeelapsed_f;
				}
				else
				{
					fAccelerationTimer = 0.0f;
					if (  t.inputsys.keycontrol == 1 ) 
					{
						t.tffcspeed_f=1.0*g.timeelapsed_f;
					}
					else
					{
						t.tffcspeed_f=5.0*g.timeelapsed_f;
					}
				}
				if (g_bCharacterCreatorPlusActivated) 
				{
					//Slow down movement when i CCP.
					t.tffcspeed_f *= 0.25;
				}

				// Only increase movement speed when not in the importer or CCP.
				if (t.importer.importerActive == 0 && !g_bCharacterCreatorPlusActivated)
				{
					// Feedback is that changing camera speed based on distance to ground is jerky and not good
					// so change to a method where the longer you hold down the shift key, the faster you go
					bool bBetterCameraSpeedOverTime = true;
					if (bBetterCameraSpeedOverTime==true)//t.visuals.bEnableEmptyLevelMode == true)
					{
						// modify movement speed based on time holding down shift
						float modifier =  100 * fAccelerationTimer;
						if (modifier > 150) modifier = 150;
						if (modifier < 2) modifier = 2;
						t.tffcspeed_f *= modifier;
					}
				}

				// speed up wheel movement
				if ( usingWheel ) t.tffcspeed_f *= 4;

				if (t.inputsys.k_s == "e")  t.traise_f = -90;
				if (t.inputsys.k_s == "q")  t.traise_f = 90;
				PositionCamera (  t.editorfreeflight.c.x_f,t.editorfreeflight.c.y_f,t.editorfreeflight.c.z_f );
				
				if (  t.plrkeyW == 1  )
					MoveCamera (  t.tffcspeed_f );

				if (  t.plrkeyS == 1  )  MoveCamera (  t.tffcspeed_f*-1 );
				if (  t.plrkeyA == 1 ) { RotateCamera (  0,t.editorfreeflight.c.angy_f-90,0  ) ; MoveCamera (  t.tffcspeed_f ); }
				if (  t.plrkeyD == 1 ) { RotateCamera (  0,t.editorfreeflight.c.angy_f+90,0  ) ; MoveCamera (  t.tffcspeed_f ); }
				if (  t.traise_f != 0 ) { RotateCamera (  t.traise_f,0,0  ) ; MoveCamera (  t.tffcspeed_f ); }
				if (  t.inputsys.mclick == 4 ) 
				{
					//  new middle mouse panning
					RotateCamera (  0,t.editorfreeflight.c.angy_f,0 );
					MoveCamera (  t.cammousemovey_f*-2 );
					if (  t.cammousemovex_f<0 ) { RotateCamera (  0,t.editorfreeflight.c.angy_f-90,0  ) ; MoveCamera (  abs(t.cammousemovex_f*2) ); }
					if (  t.cammousemovex_f>0 ) { RotateCamera (  0,t.editorfreeflight.c.angy_f+90,0  ) ; MoveCamera (  t.cammousemovex_f*2 ); }
				}
				t.editorfreeflight.c.x_f=CameraPositionX();
				t.editorfreeflight.c.y_f=CameraPositionY();
				t.editorfreeflight.c.z_f=CameraPositionZ();

				//Always display skybox.
				sky_loop();
			}

			//  view mode transitions
			if (  t.editorfreeflight.mode == 2 ) 
			{
				//  from free flight to top down
				t.tcamheight_f= fFlatFloorY+(50.0f*t.gridzoom_f);
				t.editorfreeflight.c.x_f=CurveValue(t.cx_f,CameraPositionX(),10.0);
				t.editorfreeflight.c.y_f=CurveValue(t.tcamheight_f,CameraPositionY(),10.0);
				t.editorfreeflight.c.z_f=CurveValue(t.cy_f,CameraPositionZ(),10.0);
				if (  abs(t.editorfreeflight.c.y_f-t.tcamheight_f)<20.0 ) 
				{
					t.editorfreeflight.mode=0 ; t.updatezoom=1;
				}
			}
			if (  t.editorfreeflight.mode == 3 ) 
			{
				//  from top down to free flight storage
				t.editorfreeflight.c.x_f=CurveValue(t.editorfreeflight.s.x_f,CameraPositionX(),10.0);
				t.editorfreeflight.c.y_f=CurveValue(t.editorfreeflight.s.y_f,CameraPositionY(),10.0);
				t.editorfreeflight.c.z_f=CurveValue(t.editorfreeflight.s.z_f,CameraPositionZ(),10.0);
				if (  abs(t.editorfreeflight.c.x_f-t.editorfreeflight.s.x_f)<20.0 && abs(t.editorfreeflight.c.y_f-t.editorfreeflight.s.y_f)<20.0 && abs(t.editorfreeflight.c.z_f-t.editorfreeflight.s.z_f)<20.0 ) 
				{
					t.editorfreeflight.c.x_f=t.editorfreeflight.s.x_f;
					t.editorfreeflight.c.y_f=t.editorfreeflight.s.y_f;
					t.editorfreeflight.c.z_f=t.editorfreeflight.s.z_f;
					t.editorfreeflight.mode=1 ; t.updatezoom=1;
				}
			}

			//  ensure camera NEVER goes into Floor (  )
			//PE: In wicked after loading a new fpm. we need some frames before terrain height is ready.
			if(iDelayedCameraRestore > 0)
			{
				iDelayedCameraRestore--;
			}
			else
			{
				t.tcurrenth_f = BT_GetGroundHeight(t.terrain.TerrainID, t.editorfreeflight.c.x_f, t.editorfreeflight.c.z_f) + 10.0;
					if (t.editorfreeflight.c.y_f < t.tcurrenth_f)
					{
						t.editorfreeflight.c.y_f = t.tcurrenth_f;
					}

				if (t.editorfreeflight.s.y_f < t.tcurrenth_f)
				{
					t.editorfreeflight.s.y_f = t.tcurrenth_f;
				}

			}
			//  update camera for free flight or top down modes
			PositionCamera (t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
			if (t.editorfreeflight.mode == 0)
			{
				PointCamera (t.cx_f, -99999, t.cy_f);
			}
			if (t.editorfreeflight.mode == 1)
			{
				RotateCamera (t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);
			}
			if (t.editorfreeflight.mode == 2)
			{
				t.editorfreeflight.c.angx_f = CurveAngle(90, CameraAngleX(), 10.0);
				t.editorfreeflight.c.angy_f = CurveAngle(0, CameraAngleY(), 10.0);
				RotateCamera (t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);
			}
			if (t.editorfreeflight.mode == 3)
			{
				t.editorfreeflight.c.angx_f = CurveAngle(t.editorfreeflight.s.angx_f, CameraAngleX(), 10.0);
				t.editorfreeflight.c.angy_f = CurveAngle(t.editorfreeflight.s.angy_f, CameraAngleY(), 10.0);
				RotateCamera (t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);
			}

			//  view mode prompt (top right status Text ( ) )
			if (  t.editorfreeflight.mode == 0 || t.editorfreeflight.mode == 2 ) 
			{
				t.t_s="TOP DOWN VIEW ('F' to toggle)";
				bEditorInFreeFlightMode = false;
			}
			else
			{
				t.t_s="FREE FLIGHT VIEW ('G' to toggle)";
				bEditorInFreeFlightMode = true;
			}
			#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
			#else
			t.ttxtwid=getbitmapfontwidth(t.t_s.Get(),2);
			pastebitmapfont(t.t_s.Get(),GetChildWindowWidth(0)-8-t.ttxtwid,4,2,228);
			#endif
		}
		break;

		case 999:
			//  process live updates from
			interface_live_updates ( );

			//  update camera XZ with entity if editing position
			if (  t.gridentityinzoomview>0 ) 
			{
				t.cx_f=t.zoomviewtargetx_f ; t.cy_f=t.zoomviewtargetz_f;
			}

			//  if third person start marker mode, override range and angle
			t.tlayerheight_f=t.layerheight_f;
			if (  t.playercontrol.thirdperson.enabled == 1 ) 
			{
				t.zoomviewcamerarange_f=t.playercontrol.thirdperson.livecameradistance;
				t.zoomviewcameraheight_f=t.playercontrol.thirdperson.livecameraheight;
				t.zoomviewcamerafocus_f=t.playercontrol.thirdperson.livecamerafocus;
				t.zoomviewcamerashoulder_f=t.playercontrol.thirdperson.livecamerashoulder;
				if (  t.gridentityobj>0 ) 
				{
					if (  ObjectExist(t.gridentityobj) == 1 ) 
					{
						t.zoomviewcameraangle_f=(0-ObjectAngleY(t.gridentityobj));
						t.tlayerheight_f=ObjectPositionY(t.gridentityobj);
					}
				}
			}
			else
			{
				t.zoomviewcamerafocus_f=0;
				t.zoomviewcamerashoulder_f=0;
			}

			//  calculate view from position
			t.daa_f=WrapValue(180-t.zoomviewcameraangle_f);
			t.dcx_f=t.cx_f+(Sin(t.daa_f)*t.zoomviewcamerarange_f);
			t.dcy_f=t.tlayerheight_f+t.zoomviewcameraheight_f;
			t.dcz_f=t.cy_f+(Cos(t.daa_f)*t.zoomviewcamerarange_f);
			t.tcx_f=CurveValue(t.dcx_f,CameraPositionX(),4.0);
			t.tcy_f=CurveValue(t.dcy_f,CameraPositionY(),2.0);
			t.tcz_f=CurveValue(t.dcz_f,CameraPositionZ(),4.0);

			//  if target was entity, view center of it
			if (  t.gridentityinzoomview>0 ) 
			{
				t.tobj=t.entityelement[t.gridentityinzoomview].profileobj;
				if (  t.tobj>0 ) 
				{
					t.viewatx_f=t.cx_f ; t.viewaty_f=t.zoomviewtargety_f+ObjectSizeY(t.tobj)/2.0 ; t.viewatz_f=t.cy_f;
				}
				else
				{
					t.viewatx_f=t.cx_f ; t.viewaty_f=t.zoomviewtargety_f+5 ; t.viewatz_f=t.cy_f;
				}
			}
			else
			{
				t.viewatx_f=t.cx_f ; t.viewaty_f=t.zoomviewtargety_f+5 ; t.viewatz_f=t.cy_f;
			}

			//  set smoothed camera view
			PositionCamera (  t.tcx_f,t.tcy_f,t.tcz_f );
			PointCamera (  t.viewatx_f,t.viewaty_f,t.viewatz_f );
			t.tcamax_f=CameraAngleX() ; t.tcamay_f=CameraAngleY() ; t.tcamaz_f=CameraAngleZ();
			RotateCamera (  0,t.tcamay_f+90,0 );
			MoveCamera (  t.zoomviewcamerashoulder_f );
			RotateCamera (  t.tcamax_f-t.zoomviewcamerafocus_f,t.tcamay_f,t.tcamaz_f );

			//Always display skybox.
			sky_loop();
			
		break;
	}
}

void editor_undoredoprojectstate ( void )
{
	// set as modified
	g.projectmodified=1 ; gridedit_changemodifiedflag ( );
	g.projectmodifiedstatic = 1;
}

void editor_cutcopyclearstate ( void )
{
	//  control enabling of UNDO REDO menu items
	OpenFileMap (  1, "FPSEXCHANGE" );
	SetFileMapDWORD (  1, 474, 0 );
	SetFileMapDWORD (  1, 478, 0 );
	SetFileMapDWORD (  1, 482, 0 );
	SetEventAndWait (  1 );
}

void editor_undo ( void )
{
	// undo last stage
	if ( t.ebe.on == 1 )
	{
		ebe_undo();
	}
	else
	{
		// all handled inside new undo/redo system
		entity_undo ( );
	}
}

void editor_redo ( void )
{
	if ( t.ebe.on == 1 )
	{
		ebe_redo();
	}
	else
	{
		// all handled inside new undo/redo system
		entity_redo ( );
	}
}

void gridedit_showtobjlegend ( void )
{
	t.relaytostatusbar_s="";
	if (  t.tobj>0 ) 
	{
		if (  ObjectExist(t.tobj)>0 ) 
		{
			if (  t.taddstaticlegend == 1 ) 
			{
				//  static
				t.tname_s=t.tname_s+" "+t.strarr_s[608];
			}
			else
			{
				//  dynamic
				t.tname_s=t.tname_s+" "+t.strarr_s[609];
			}
			if (  t.gridedit.autoflatten == 1  )  t.tname_s = t.tname_s+"(autoflatten)";
			if (  t.gridedit.entityspraymode == 1  )  t.tname_s = t.tname_s+"(spray mode)";
			t.relaytostatusbar_s=t.tname_s;
		}
	}
	return;
}

void editor_checkIfInSubApp ( void )
{
	t.result = 0;
}

int findentitycursorobj ( int currentlyover )
{
	#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
	#endif

	if (pref.iDragCameraMovement && t.ebe.on == 0 && bDragCameraActive)
		return 0;

	// Uses simpler system to detect what is under cursor
	int result = 0;
	uint64_t hitentity = 0;

	//PE: We already sent a ray reuse data.
	if (iReusePickObjectID != -1)
	{
		g.glastpickedx_f = fReusePickHitX;
		g.glastpickedy_f = fReusePickHitY;
		g.glastpickedz_f = fReusePickHitZ;
		t.lastfindentitycursorobj = iReusePickEntityID;
		return(iReusePickEntityID);
	}

	float fHitX, fHitY, fHitZ;
	WickedCall_GetPick(&fHitX, &fHitY, &fHitZ, NULL, NULL, NULL, &hitentity, GGRENDERLAYERS_NORMAL | GGRENDERLAYERS_TERRAIN); // LB: Added GGRENDERLAYERS_TERRAIN so cannot select objects UNDER the terrain!
	if (hitentity>0)
	{
		iLastHitObjectID = 0;

		// found object under hovering cursor, match to entity index
		sObject* pHitObject = m_ObjectManager.FindObjectFromWickedObjectEntityID(hitentity);
		int iHitObjectEntityElementE = -1;
		if (pHitObject)
		{
			for (int e = 1; e <= g.entityelementlist; e++)
			{
				if (t.entityelement[e].obj == pHitObject->dwObjectNumber)
				{
					iHitObjectEntityElementE = e;
					break;
				}
			}
		}
		if (pref.iEnableDragDropStopSelectFromInside == 1)
		{
			// control whether can select an object from the inside
			if (iHitObjectEntityElementE != -1)
			{
				int entid = t.entityelement[iHitObjectEntityElementE].bankindex;
				if (entid > 0 && t.entityprofile[entid].ismarker == 0)
				{
					// but only if regular object (like a building, etc, not a particle marker or light)
					if (pHitObject && CameraInsideObject(pHitObject) == true) pHitObject = NULL;
				}
			}
		}
		if (pHitObject)
		{
			int e = iHitObjectEntityElementE;
			if(e > 0)
			{
				iLastHitObjectID = pHitObject->dwObjectNumber;
				g.glastpickedx_f = fHitX;
				g.glastpickedy_f = fHitY;
				g.glastpickedz_f = fHitZ;
				result = e;
			}
		}
	}
	t.lastfindentitycursorobj = result;
	return result;
}

void gridedit_clearentityrubberbandlist ( void )
{
	if ( g.entityrubberbandlist.size() > 0 )
	{
		for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
		{
			int e = g.entityrubberbandlist[i].e;
			//PE: Got exception here: e was to large.
			if (e <= t.entityelement.size()) {
				int tobj = t.entityelement[e].obj;
				if (tobj > 0)
				{
					int mi = t.entityelement[e].bankindex;
					if(mi > 0 && t.entityprofile[mi].bIsDecal) //PE: Got crash here, should be bankindex.
						SetupDecalObject(tobj, e);
					else
					{
						SetAlphaMappingOn(tobj, 100);
						//PE: Restore original material.
						sObject* pObject = g_ObjectList[tobj];
						if (pObject)
						{
							WickedSetEntityId(mi);
							WickedSetElementId(e);
							for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
							{
								WickedSetMeshNumber(iMesh);
								WickedCall_SetMeshMaterial(pObject->ppMeshList[iMesh], false);
							}
							WickedSetEntityId(-1);
							WickedSetElementId(0);
						}

					}
				}
			}
		}
	}
	g.entityrubberbandlist.clear();
}

void gridedit_addEntityToRubberBandHighlights ( int e )
{
	// 011215 - skip if in marker mode and not a marker
	if ( t.gridentitymarkersmodeonly == 1 && t.entityprofile[t.entityelement[e].bankindex].ismarker==0 ) 
		return;

	#ifdef ALLOWSELECTINGLOCKEDOBJECTS
	if (t.entityelement[e].editorlock) return;
	#endif
	// add entity to rubber band
	int tobj = t.entityelement[e].obj;
	bool bEntityIsHighlighted = false;
	if ( g.entityrubberbandlist.size() > 0 )
	{
		for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
		{
			int thise = g.entityrubberbandlist[i].e;
			if ( e == thise ) bEntityIsHighlighted = true;
		}
	}
	if ( bEntityIsHighlighted == false )
	{
		sRubberBandType rubberbandItem;
		rubberbandItem.e = e;
		rubberbandItem.x = t.entityelement[e].x;
		rubberbandItem.y = t.entityelement[e].y;
		rubberbandItem.z = t.entityelement[e].z;
		rubberbandItem.px = t.entityelement[e].x;
		rubberbandItem.py = t.entityelement[e].y;
		rubberbandItem.pz = t.entityelement[e].z;
		rubberbandItem.rx = t.entityelement[e].rx;
		rubberbandItem.ry = t.entityelement[e].ry;
		rubberbandItem.rz = t.entityelement[e].rz;	
		rubberbandItem.quatmode = t.entityelement[e].quatmode;
		rubberbandItem.quatx = t.entityelement[e].quatx;
		rubberbandItem.quaty = t.entityelement[e].quaty;
		rubberbandItem.quatz = t.entityelement[e].quatz;
		rubberbandItem.quatw = t.entityelement[e].quatw;
		rubberbandItem.scalex = t.entityelement[e].scalex;
		rubberbandItem.scaley = t.entityelement[e].scaley;
		rubberbandItem.scalez = t.entityelement[e].scalez;
		g.entityrubberbandlist.push_back ( rubberbandItem );

		if ( t.entityelement[e].staticflag == 0 ) 
			SetAlphaMappingOn ( tobj, 103 );
		else
			SetAlphaMappingOn ( tobj, 101 );
	}
}

