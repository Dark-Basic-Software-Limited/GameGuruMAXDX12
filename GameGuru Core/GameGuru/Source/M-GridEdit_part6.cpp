void gridedit_mapediting ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	//  Determine if cursor is at rest, allows performance boost to skip
	//  expensive ray casts to find entities in map
	if (  t.inputsys.atrest != 2 ) 
	{
		t.inputsys.atrest=0;
		if (  t.inputsys.localx_f == t.inputsys.atrestx && t.inputsys.localy_f == t.inputsys.atresty ) 
		{
			t.inputsys.atrest=1;
		}
		//  never at rest when widget menu active
		if (  t.widget.pickedSection != 0  )  t.inputsys.atrest = 0;
	}
	else
	{
		if (  t.inputsys.localx_f != t.inputsys.atrestx || t.inputsys.localy_f != t.inputsys.atresty ) 
		{
			t.inputsys.atrest=0;
		}
	}

	t.inputsys.atrestx=t.inputsys.localx_f;
	t.inputsys.atresty=t.inputsys.localy_f;

	//  flag to determine if character will attach to start marker
	t.inputsys.willmakethirdperson=0;

	//  Only if within map
	if (  t.inputsys.mmx >= 0 && t.inputsys.mmy >= 0 && t.inputsys.mmx<t.maxx && t.inputsys.mmy<t.maxy ) 
	{
		//  Any click inside 3D area constitues some sort of edit
		if ( t.inputsys.mclick != 0 ) 
		{ 
			g.projectmodified = 1; 
			gridedit_changemodifiedflag ( ); 
			// effect on g.projectmodifiedstatic
		}

		//  ENTITY EDIT Handling (onedrag=0 means no waypoint dragging)
		if (  t.grideditselect !=  5  )  t.tentitytoselect  =  0;

		if (  t.grideditselect == 5 && t.onedrag == 0 ) 
		{
			//  Regular entity editing
			t.layer=t.gridlayer ; t.mx=t.inputsys.mmx ; t.my=t.inputsys.mmy;

			if (  t.selstage == 0 ) 
			{
				//  single entity highlight
				t.tshowasstatic=0 ; t.showentityid=0 ; t.tforcedynamic=0 ; t.tentitytoselect=0;

				if ( t.gridentity == 0 ) 
				{
					//  no entity attached to cursor (if RMB, deactivate entity detection for smoother moving)
					if ( t.widget.activeObject == 0 && t.inputsys.xmouse != 500000 && t.inputsys.mclick != 2 && t.inputsys.rubberbandmode == 0 ) 
					{
						if ( t.inputsys.atrest == 1 || t.inputsys.keyspace == 1 )
						{
							if(t.inputsys.mclick != 0)
								iReusePickObjectID = -1; //PE: Do a fresh raycast.
							t.tentitytoselect = findentitycursorobj(-1);
							t.tlasttentitytoselect = t.tentitytoselect;
							t.inputsys.atrest = 2;
						}
						else
						{
							// wicked fast enough to do this test each frame
							if (t.inputsys.mclick != 0)
								iReusePickObjectID = -1; //PE: Do a fresh raycast.
							t.tentitytoselect = findentitycursorobj(-1);
						}
						//Group edit mode ?
						if (t.tentitytoselect > 0 && current_selected_group >= 0 && group_editing_on)
						{
							//Only allow selection within group.
							int grouplist = isEntityInGroupList(t.tentitytoselect);
							if (grouplist < 0 || current_selected_group != grouplist) //Dont allow selecting objects from another group.
								t.tentitytoselect = 0;
						}
					}
					else
					{
						t.tentitytoselect=0;
					}

					bool bActivateRubberBand = true;
					if (bDotObjectDragging || (g_hovered_dot_pobject && t.inputsys.rubberbandmode == 0) ) {
						bActivateRubberBand = false;
					}
					if (pref.iEnableDragDropEntityMode) {
						if (bWaitOnMouseRelease || iDragDropActive > 0 ) {
							t.inputsys.rubberbandmode = 0;
							bActivateRubberBand = false;
						}
					}
					if (current_selected_group >= 0 && group_editing_on)
					{
						t.inputsys.rubberbandmode = 0;
						bActivateRubberBand = false;
					}

					// when holding SHIFT, we may want to rubber band, even if hovering over an object
					bool bSHIFTForRubberBand = false;
					if (t.inputsys.keyshift == 1)
					{
						if (t.tentitytoselect > 0)
						{
							// ensure no object already in selection when start rubber banding
							t.tentitytoselect = 0;
						}
						bSHIFTForRubberBand = true;
					}

					//PE: Added ctrl allow selecting multiply objects.
					if (pref.iDragCameraMovement && t.ebe.on == 0 && t.inputsys.keyshift == 0 && t.inputsys.keycontrol == 0)
					{
						bActivateRubberBand = false;
						//PE: Still support unselecting rubberband.
						if (t.inputsys.mclick == 1 && t.inputsys.rubberbandmode == 0 && t.widget.activeObject == 0)
						{
							if (t.inputsys.keycontrol == 0)
							{
								if (pref.iEnableDragDropEntityMode && g_hovered_pobject) 
								{
									//hover over object and click on object, this should trigger a move in the new system.
								}
								else
								{
									gridedit_clearentityrubberbandlist();
								}
							}
						}
						if (t.inputsys.rubberbandmode == 1)
						{
							bool bCancelRubberBand = false;
							if (pref.iEnableDragDropEntityMode) 
							{
								if (bWaitOnMouseRelease)
									bCancelRubberBand = true;
							}
							if (t.inputsys.xmouse == 500000 || bCancelRubberBand)
							{
								// mouse left area, cancel rubber band
								t.inputsys.rubberbandmode = 0;
							}
						}
					}
					if (pref.iDragCameraMovement && t.ebe.on == 0 && t.inputsys.keycontrol == 1 && bActivateRubberBand)
					{
						bActivateRubberBand = false;
						// if clicked a single entity WHILE holding control, can add to list (need to click to select)
						if (t.inputsys.mclick == 1 && t.inputsys.keycontrol == 1 && t.tentitytoselect > 0)
						{
							if (g.entityrubberbandlist.size() > 0)
							{
								//PE: When using ctrl to add to rubberband. make sure no locked items is inside rubberband.
								for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
								{
									int e = g.entityrubberbandlist[i].e;
									if (e > 0 && t.entityelement[e].editorlock)
									{
										//Locked obects in list, start new list.
										g.entityrubberbandlist.clear();
									}
								}
							}
							if (t.widget.pickedEntityIndex > 0)
							{
								// add initial selected object if not in list already
								bool bAlreadyInList = false;
								for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
								{
									int e = g.entityrubberbandlist[i].e;
									if (e == t.widget.pickedEntityIndex)
									{
										bAlreadyInList = true;
										break;
									}
								}
								if (bAlreadyInList == false)
								{
									gridedit_addEntityToRubberBandHighlights(t.widget.pickedEntityIndex);
								}
							}
							gridedit_addEntityToRubberBandHighlights(t.tentitytoselect);
						}
						// when select entity (widget called up), if parent to children, add them to rubberband so they can all be modified at the same time
						if (t.tentitytoselect > 0)
						{
							bool bHasChildren = false;
							t.tstoreentityindexofprimaryhightlighted = 0;
							for (int te = 1; te <= g.entityelementlist; te++)
							{
								if (t.entityelement[te].iHasParentIndex == t.tentitytoselect && t.entityelement[te].obj > 0)
								{
									gridedit_addEntityToRubberBandHighlights(te);
									editor_rec_addchildrentorubberband(te);
									bHasChildren = true;
								}
							}
							if (bHasChildren == true)
							{
								if (t.inputsys.k_s != "l")
								{
									gridedit_addEntityToRubberBandHighlights(t.tentitytoselect);
								}
							}
						}
					}

					if (bActivateRubberBand && t.inputsys.mclick == 1 && t.inputsys.rubberbandmode == 0 && t.widget.activeObject == 0 )
					{
						// clear any previous highlights
						if ( t.inputsys.keycontrol == 0 )
						{
							if (pref.iEnableDragDropEntityMode && g_hovered_pobject && bSHIFTForRubberBand == false)
							{
								// hover over object and click on object, this should trigger a move in the new system.
								// except when SHIFT held down which means we want to start a rubber band from the click!
							}
							else
							{
								gridedit_clearentityrubberbandlist();
							}
						}
						else
						{
							// if clicked a single entity WHILE holding control, can add to list
							if ( t.tentitytoselect > 0 )
							{
								gridedit_addEntityToRubberBandHighlights ( t.tentitytoselect );
							}
						}

						// when select entity (widget called up), if parent to children, add them to rubberband so they can all be modified at the same time
						if ( t.tentitytoselect > 0 && bSHIFTForRubberBand==false )
						{
							bool bHasChildren = false;
							t.tstoreentityindexofprimaryhightlighted = 0;
							for ( int te = 1; te <= g.entityelementlist; te++ )
							{
								if ( t.entityelement[te].iHasParentIndex == t.tentitytoselect && t.entityelement[te].obj > 0 )
								{
									gridedit_addEntityToRubberBandHighlights ( te );
									editor_rec_addchildrentorubberband ( te );
									bHasChildren = true;
								}
							}
							if ( bHasChildren == true )
							{
								if ( t.inputsys.k_s != "l" ) 
								{
									gridedit_addEntityToRubberBandHighlights ( t.tentitytoselect );
								}
							}
						}
					}

					if (bActivateRubberBand && t.tentitytoselect > 0 && t.entityelement[t.tentitytoselect].obj > 0 && bSHIFTForRubberBand == false)
					{
						// specific entity highlighted 
						PositionObject (  t.editor.objectstartindex+5,t.entityelement[t.tentitytoselect].x,t.entityelement[t.tentitytoselect].y,t.entityelement[t.tentitytoselect].z );
						t.showentityid=t.entityelement[t.tentitytoselect].bankindex;
						if (  t.entityprofile[t.showentityid].ismarker == 3 || t.entityprofile[t.showentityid].ismarker == 6 || t.entityprofile[t.showentityid].ismarker == 8 ) 
						{
							//  trigger zone or checkpoint
							t.tscale_f=100;
						}
						else
						{
							if (  t.entityprofile[t.showentityid].islightmarker == 1 ) 
							{
								t.tscale_f=(100/3.0)*2*(t.entityelement[t.tentitytoselect].eleprof.light.range/50.0);
							}
							else
							{
								t.tscale_f = get_cursor_scale_for_obj(t.entityelement[t.tentitytoselect].obj);
							}
						}
						ScaleObject (  t.editor.objectstartindex+5,t.tscale_f,t.tscale_f,t.tscale_f );
						t.tshowasstatic=1+t.entityelement[t.tentitytoselect].staticflag;
					}
					else
					{
						// 201015 - if not highlighting an entity, click to start dragging a rubber band box
						if (bActivateRubberBand && t.widget.activeObject == 0 && t.inputsys.xmouse != 500000 )
						{
							if ( t.inputsys.mclick == 1 )
							{
								// start rubber band box
								if ( t.inputsys.rubberbandmode == 0 )
								{
									t.inputsys.rubberbandmode = 1;
									t.inputsys.spacekeynotreleased = 1;
									//PE: Make mouse relative to window pos.
									t.inputsys.rubberbandx = ImGuiGetMouseX();
									t.inputsys.rubberbandy = ImGuiGetMouseY();
								}
							}
						}
					}

					if (pref.iDragCameraMovement && t.ebe.on == 0 && t.inputsys.keyshift == 0 && t.inputsys.rubberbandmode == 1)
					{
						//PE: Shift released , end rubberband on mouse release.
						if (t.inputsys.mclick == 0)
						{
							float fCurrentRubberBandX1 = t.inputsys.rubberbandx;
							float fCurrentRubberBandX2 = ImGuiGetMouseX();
							float fCurrentRubberBandY1 = t.inputsys.rubberbandy;
							float fCurrentRubberBandY2 = ImGuiGetMouseY();
							if (fCurrentRubberBandX1 > fCurrentRubberBandX2) { float fStore = fCurrentRubberBandX1; fCurrentRubberBandX1 = fCurrentRubberBandX2; fCurrentRubberBandX2 = fStore; }
							if (fCurrentRubberBandY1 > fCurrentRubberBandY2) { float fStore = fCurrentRubberBandY1; fCurrentRubberBandY1 = fCurrentRubberBandY2; fCurrentRubberBandY2 = fStore; }

							// finish rubber banding
							t.inputsys.rubberbandmode = 0;
							bRubberBandCreated = true;
							fLastRubberBandX1 = fCurrentRubberBandX1;
							fLastRubberBandX2 = fCurrentRubberBandX2;
							fLastRubberBandY1 = fCurrentRubberBandY1;
							fLastRubberBandY2 = fCurrentRubberBandY2;
							// auto choose an entity to act as the widget achor object
							if (g.entityrubberbandlist.size() > 0)
							{
								if (g.entityrubberbandlist.size() == 1)
								{
									// if only range selected on, make it a regular entity selection
									t.widget.pickedEntityIndex = g.entityrubberbandlist[0].e;
									gridedit_clearentityrubberbandlist();
								}
								else
								{
									t.widget.pickedEntityIndex = g.entityrubberbandlist[0].e;
								}
								t.widget.pickedObject = t.entityelement[t.widget.pickedEntityIndex].obj;
								t.widget.offsetx = 0;
								t.widget.offsety = 0;
								t.widget.offsetz = 0;

								if (pref.iEnableDragDropEntityMode)
								{
									fHitOffsetZ = 0.0001; //So we trigger a widget.
								}

								i_switch_group_tab = 1; //Display "current objects" tab.
							}
							t.inputsys.rubberbandmode = 0;
						}
					}

					// 201015 - rubber band effect and control
					if (bActivateRubberBand && t.inputsys.rubberbandmode == 1 )
					{
						//bWaitOnMouseRelease
						bool bCancelRubberBand = false;
						if (pref.iEnableDragDropEntityMode) {
							if (bWaitOnMouseRelease)
								bCancelRubberBand = true;
						}
						if ( t.inputsys.xmouse == 500000 || bCancelRubberBand )
						{
							// mouse left area, cancel rubber band
							t.inputsys.rubberbandmode = 0;
						}
						else
						{
							float fMX = 1.0f;
							float fMY = 1.0f;
						
							// reverse bound box if inside out
							float fCurrentRubberBandX1 = t.inputsys.rubberbandx;
							float fCurrentRubberBandX2 = ImGuiGetMouseX();
							float fCurrentRubberBandY1 = t.inputsys.rubberbandy;
							float fCurrentRubberBandY2 = ImGuiGetMouseY();
							//PE: Y a bit off
							fCurrentRubberBandY1 -= 20.0f;
							if ( fCurrentRubberBandX1 > fCurrentRubberBandX2 ) { float fStore = fCurrentRubberBandX1; fCurrentRubberBandX1 = fCurrentRubberBandX2; fCurrentRubberBandX2 = fStore; }
							if ( fCurrentRubberBandY1 > fCurrentRubberBandY2 ) { float fStore = fCurrentRubberBandY1; fCurrentRubberBandY1 = fCurrentRubberBandY2; fCurrentRubberBandY2 = fStore; }

							// detect all entities within box and highlight
							for ( int e = 1; e <= g.entityelementlist; e++ )
							{
								// 060116 - if locked and holding space, unlock it now
								if (t.inputsys.keyspace != 0) 
								{
									t.entityelement[e].editorlock = false;
									sObject* pObject;
									if (t.entityelement[e].obj > 0) 
									{
										pObject = g_ObjectList[t.entityelement[e].obj];
										if (pObject) 
										{
											if (t.entityelement[e].editorlock) 
											{
												#ifndef ALLOWSELECTINGLOCKEDOBJECTS
												WickedCall_SetObjectRenderLayer(pObject, GGRENDERLAYERS_CURSOROBJECT);
												#endif
											}
											else
												WickedCall_SetObjectRenderLayer(pObject, GGRENDERLAYERS_NORMAL);
										}
									}
								}

								// only if not locked
								if ( t.entityelement[e].editorlock==false )
								{
									int tobj = t.entityelement[e].obj;
									if ( tobj > 0 )
									{
										ImVec2 v2DPos = Convert3DTo2D(ObjectPositionX(tobj), ObjectPositionY(tobj), ObjectPositionZ(tobj));
										int iEntityScreenX = v2DPos.x;
										int iEntityScreenY = v2DPos.y;
										if ( iEntityScreenX > fCurrentRubberBandX1*fMX && iEntityScreenX < fCurrentRubberBandX2*fMX )
										{
											if ( iEntityScreenY > fCurrentRubberBandY1*fMY && iEntityScreenY < fCurrentRubberBandY2*fMY )
											{
												// only if not already highlighted
												gridedit_addEntityToRubberBandHighlights ( e );
											}
										}
									}
								}
							}

							// now de-highlight any in the list NOT covered by the boundbox
							if ( g.entityrubberbandlist.size() > 0 )
							{
								int i = 0;
								while ( i < (int)g.entityrubberbandlist.size() )
								{
									bool bThisOneInBox = false;
									int e = g.entityrubberbandlist[i].e;
									int tobj = t.entityelement[e].obj;
									if ( tobj > 0 )
									{
										//PE: This one work better. The old have problems selecting objects at the bottom of screen.
										ImVec2 v2DPos = Convert3DTo2D(ObjectPositionX(tobj), ObjectPositionY(tobj), ObjectPositionZ(tobj));
										int iEntityScreenX = v2DPos.x;
										int iEntityScreenY = v2DPos.y;
										if ( iEntityScreenX > fCurrentRubberBandX1*fMX && iEntityScreenX < fCurrentRubberBandX2*fMX )
										{
											if ( iEntityScreenY > fCurrentRubberBandY1*fMY && iEntityScreenY < fCurrentRubberBandY2*fMY )
											{
												bThisOneInBox = true;
											}
										}
									}
									if ( bThisOneInBox == false )
									{
										SetAlphaMappingOn ( tobj, 100 );
										//PE: Restore original colors.
										sObject* pObject = g_ObjectList[tobj];
										if (pObject)
										{
											WickedSetEntityId(t.entityelement[e].bankindex);
											WickedSetElementId(e);
											for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
											{
												WickedSetMeshNumber(iMesh);
												WickedCall_SetMeshMaterial(pObject->ppMeshList[iMesh], false);
											}
											WickedSetEntityId(-1);
											WickedSetElementId(0);
										}
										g.entityrubberbandlist.erase(g.entityrubberbandlist.begin() + i);
										i = 0;
									}
									else
									{
										i++;
									}
								}
							}

							// when release mouse while rubber banding
							WickedCall_SetSpriteBoundBox(false,0,0,0,0);
							if ( t.inputsys.mclick == 0 )
							{
								// finish rubber banding
								t.inputsys.rubberbandmode = 0;
								bRubberBandCreated = true;
								fLastRubberBandX1 = fCurrentRubberBandX1;
								fLastRubberBandX2 = fCurrentRubberBandX2;
								fLastRubberBandY1 = fCurrentRubberBandY1;
								fLastRubberBandY2 = fCurrentRubberBandY2;
								// auto choose an entity to act as the widget achor object
								if (g.entityrubberbandlist.size() > 0)
								{
									if (g.entityrubberbandlist.size() == 1)
									{
										// if only range selected on, make it a regular entity selection
										t.widget.pickedEntityIndex = g.entityrubberbandlist[0].e;
										gridedit_clearentityrubberbandlist();
									}
									else
									{
										t.widget.pickedEntityIndex = g.entityrubberbandlist[0].e;
									}
									t.widget.pickedObject = t.entityelement[t.widget.pickedEntityIndex].obj;
									t.widget.offsetx = 0;
									t.widget.offsety = 0;
									t.widget.offsetz = 0;

									if (pref.iEnableDragDropEntityMode)
									{
										fHitOffsetZ = 0.0001; //So we trigger a widget.
									}

									i_switch_group_tab = 1; //Display "current objects" tab.
								}
							}
							else
							{
								//PE: Not relative to win rect, only when selecting the actual objects, 2D is different.
								RECT rect = { NULL };
								GetWindowRect(g_pGlob->hWnd, &rect);
								float fX1 = (t.inputsys.rubberbandx + rect.left)*fMX;
								float fX2 = t.inputsys.xmouse*fMX;
								float fY1 = (t.inputsys.rubberbandy + rect.top)*fMY;
								float fY2 = t.inputsys.ymouse*fMY;
								//PE: A bit off.
								fX1 += 6.0;
								fX2 += 6.0;
								fY1 += 6.0;
								fY2 += 6.0;
								if ( fX2 < fX1 )
								{
									float fSt = fX1;
									fX1 = fX2;
									fX2 = fSt;
								}
								if ( fY2 < fY1 )
								{
									float fSt = fY1;
									fY1 = fY2;
									fY2 = fSt;
								}
								if(!g_bCharacterCreatorPlusActivated)
									WickedCall_SetSpriteBoundBox(true, fX1, fY1, fX2, fY2);
							}
						}
					}
				}
				else
				{
					//  entity attached to cursor
					PositionObject (  t.editor.objectstartindex+5,t.gridentityposx_f,t.gridentityposy_f,t.gridentityposz_f );
					if (  ObjectExist(g.entitybankoffset+t.gridentity) == 1 ) 
					{
						if (  t.entityprofile[t.gridentity].ismarker == 3 || t.entityprofile[t.gridentity].ismarker == 6 || t.entityprofile[t.gridentity].ismarker == 8 ) 
						{
							//  trigger zone or checkpoint
							t.tscale_f=100;
						}
						else
						{
							if (  t.entityprofile[t.gridentity].islightmarker == 1 || t.gridedit.entityspraymode == 1) 
							{
								if ( t.gridedit.entityspraymode == 1 )
								{
									t.tscale_f=(100/3.0)*2*(t.gridedit.entitysprayrange/50.0);
								}
								else
								{
									t.tscale_f=(100/3.0)*2*(t.grideleprof.light.range/50.0);
								}
							}
							else
							{
								t.tscale_f = get_cursor_scale_for_obj(g.entitybankoffset+t.gridentity);
							}
						}
						ScaleObject (  t.editor.objectstartindex+5,t.tscale_f,t.tscale_f,t.tscale_f );
					}
					//if (  t.entityprofile[t.gridentity].ischaracter == 1 && t.entityprofile[t.gridentity].isthirdperson == 1 ) 
					if (  t.entityprofile[t.gridentity].ischaracter == 1 ) // 220217 - now for all characters
					{
						// third person char+marker detection (will not work in VR edit mode)
						if ( t.playercontrol.thirdperson.enabled == 0 && g.vrqcontrolmode == 0 ) 
						{
							t.tattachtothis=findentitycursorobj(-1);
							if (  t.tattachtothis>0 ) 
							{
								if (  t.entityprofile[t.entityelement[t.tattachtothis].bankindex].ismarker == 1 ) 
								{
									t.tobj=t.entityelement[t.tattachtothis].obj;
									if (  t.tobj>0 ) 
									{
										if (  ObjectExist(t.tobj) == 1 ) 
										{
											t.tmousemodifierx_f=(GetDisplayWidth()+0.0)/(GetChildWindowWidth()+0.0);
											t.tmousemodifiery_f=(GetDisplayHeight()+0.0)/(GetChildWindowHeight()+0.0);
											pastebitmapfontcenter("ATTACH FOR THIRD PERSON CONTROL",GetScreenX(t.tobj)*t.tmousemodifierx_f,(GetScreenY(t.tobj)*t.tmousemodifiery_f)+50,2,255);
											t.inputsys.willmakethirdperson=t.tattachtothis;
										}
									}
								}
							}
						}
					}
					t.tshowasstatic=1+t.gridentitystaticmode;
					t.showentityid=t.gridentity;

					// no linking in MAX
					t.tentityoverdraggingcursor = 0;
				}

				if (  t.tshowasstatic>0 && t.inputsys.activemouse == 1 ) 
				{
					t.tentityworkobjectchoice=0;
					if (  t.entityprofile[t.showentityid].ismarker == 1  )  t.tforcedynamic = 1;
					if (  t.entityprofile[t.showentityid].ismarker == 4  )  t.tforcedynamic = 1;
					if (  t.entityprofile[t.showentityid].ismarker == 10  )  t.tforcedynamic = 1;
					if (  t.entityprofile[t.showentityid].ismarker == 3 || t.entityprofile[t.showentityid].ismarker == 6 || t.entityprofile[t.showentityid].ismarker == 8 ) 
					{
						//  trigger area or checkpoint
						HideObject (  t.editor.objectstartindex+5 );
						t.tforcedynamic=1;
					}
					else
					{
						// No flat range decal in wicked!
						HideObject (  t.editor.objectstartindex+5 );
					}
					//  show legend of entity hovering over (and static legend)
					t.taddstaticlegend=0;
					if (  t.tentitytoselect>0 ) 
					{
						t.tstatic=t.entityelement[t.tentitytoselect].staticflag;
						t.tentid=t.entityelement[t.tentitytoselect].bankindex;
					}
					else
					{
						t.tstatic=t.gridentitystaticmode;
						t.tentid=t.gridentity;
					}
					if (  t.tstatic == 1 ) 
					{
						t.taddstaticlegend=1;
					}
					else
					{
						t.taddstaticlegend=0;
					}
					t.editor.entityworkobjectchoice=t.tentityworkobjectchoice;
					t.editor.entitytoselect=t.tentitytoselect;
					if ( t.tentitytoselect>0 ) 
					{
						t.tobj=t.entityelement[t.tentitytoselect].obj;
						t.tname_s = "" ; t.tname_s=t.tname_s + "["+Str(t.tentitytoselect)+" {"+Str(t.tobj)+"}] "+t.entityelement[t.tentitytoselect].eleprof.name_s;
						t.ttentid=t.entityelement[t.tentitytoselect].bankindex;
						gridedit_showtobjlegend ( );
					}
					else
					{
						t.tobj=t.gridentityobj ; t.tname_s=t.grideleprof.name_s;
						t.ttentid=t.gridentity;
						gridedit_showtobjlegend ( );
					}
				}
				else
				{
					HideObject (  t.editor.objectstartindex+5 );
				}

				editor_refreshentitycursor ( );

				//  prompt when over locked entity
				if (  g.gentityundercursorlocked>0 ) 
				{
					t.relaytostatusbar_s="LOCKED - Hold SPACEBAR and click to unlock";
				}
				else
				{
					if (  t.tentitytoselect == 0 && t.gridentity == 0 ) 
					{
						t.relaytostatusbar_s="None Selected";
					}
				}

				// Entity Edit Mode
				if ( 1 ) 
				{
					// only place plane on each new placement
					static int iInitialPlacementOfPlane = 0;
					if (iInitialPlacementOfPlane == 0 && t.gridentity != 0) { iInitialPlacementOfPlane = 1; g_bResetPlaneAfterXZAdjust = false; }
					if (iInitialPlacementOfPlane == 2 && t.gridentity == 0) iInitialPlacementOfPlane = 0;

					bool bPlaceEntity = false;
					if ((t.widget.duplicatebuttonselected == 2 && t.gridentity == 0))
						bPlaceEntity = true;

					if (!bPlaceEntity)
					{
						//PE: We should only do this if we have a drag/drop process active.
						if (pref.iEnableDragDropEntityMode && bDraggingActive) 
						{
							//PE: In this mode we need a mouse release to drop
							if (ImGui::IsMouseClicked(0))
								bReadyToDropEntity = true;
							if (t.inputsys.mclick == 1)
								bReadyToDropEntity = true;
							if (t.gridentity != 0 && bReadyToDropEntity && t.inputsys.mclick == 0 && !ImGui::IsMouseDown(0))
							{
								bPlaceEntity = true;
								bReadyToDropEntity = false;
								bDraggingActive = true;
								if (iObjectMoveMode == 2 && (iObjectMoveModeDropSystemUsing == 1 && g_bHoldGridEntityPosWhenManaged == false))
								{
									// trigger force find surface events
									iObjectMoveModeDropSystem = -3;
								}
								iObjectMoveModeDropSystemUsing = 0;
							}
						}
						else
						{
							if ((t.inputsys.mclick == 1 && t.gridentity != 0))
								bPlaceEntity = true;
						}
					}

					// so can get the plane position info later
					bool bPlanePosRegistered = false;
					float fPlanePosX, fPlanePosY, fPlanePosZ;
					fPlanePosX = 0;
					fPlanePosY = -999999.9f;
					fPlanePosZ = 0;

					// special horiz mode to match terrain height when about to go under it
					static bool bDepartedFromChosenY = false;
					static float fDepartedFromThisY = 0.0f;
					if (t.inputsys.mclick != 1) bDepartedFromChosenY = false;
					int newpicksystem = -99;

					//LB: widget control handled elsewhere, so no smart placement if widget active!
					bool bWidgetHasControlHere = false;
					if (t.widget.pickedSection > 0 && t.widget.pickedSection != -98 && t.widget.pickedSection != -99) bWidgetHasControlHere = true;
					if (bTriggerVisibleWidget == true) bWidgetHasControlHere = true;

					//PE: Allow object to go 80% below terrain.
					int GetActiveEditorObject(void);
					int iActiveObj = GetActiveEditorObject();

					//Dont change anyhthing when we are ready to place entity.
					if (!bPlaceEntity && bWidgetHasControlHere == false )
					{
						//PE: Prevent user for placing objects outside playable area.
						bool bObjectOutSideEditArea = false;
						float fEditableSizeHalved = GGTerrain_GetEditableSize();
						t.terraineditableareasizeminx = -fEditableSizeHalved;
						t.terraineditableareasizeminz = -fEditableSizeHalved;
						t.terraineditableareasizemaxx = fEditableSizeHalved;
						t.terraineditableareasizemaxz = fEditableSizeHalved;
						if (t.gridentityposx_f < t.terraineditableareasizeminx) { t.gridentityposx_f = t.terraineditableareasizeminx; bObjectOutSideEditArea = true; }
						if (t.gridentityposx_f > t.terraineditableareasizemaxx) { t.gridentityposx_f = t.terraineditableareasizemaxx; bObjectOutSideEditArea = true; }
						if (t.gridentityposz_f < t.terraineditableareasizeminz) { t.gridentityposz_f = t.terraineditableareasizeminz; bObjectOutSideEditArea = true; }
						if (t.gridentityposz_f > t.terraineditableareasizemaxz) { t.gridentityposz_f = t.terraineditableareasizemaxz; bObjectOutSideEditArea = true; }
						float fOldgridentityposx_f = t.gridentityposx_f;
						float fOldgridentityposy_f = t.gridentityposy_f;
						float fOldgridentityposz_f = t.gridentityposz_f;

						//  entity placement update
						if (t.inputsys.mclick != 2)
						{
							static float fActivePosX = t.gridentityposx_f;
							static float fActivePosZ = t.gridentityposz_f;
							static float fActivePosY = fHitPointY;

							bMouseInputSystemUsed = false;
							if (pref.iEnableDragDropEntityMode)
							{
								int picksystem = t.widget.pickedSection;
								{
									if (bDraggingActive == false) bDraggingActiveInitial = false;
									if (ImGui::GetIO().MouseReleased[1])
									{
										// Wait for mouse position to update before performing pick
										// When we hold RMB to look around with an entity on the cursor, the mouse position is reset to the centre of the screen temporarily
										// When the RMB is released, the initial mouse pick will always be from the center of the screen, rather than where the mouse was before it was hidden
										// This causes the entity to flicker. Skipping this function after a RMB release will prevent the flicker.
										return;
									}
									if (t.inputsys.localselectedrayhit == true && (bDraggingActive == false || bDraggingActiveInitial == true || iObjectMoveMode == 2))
									{
										fHitPointY = t.inputsys.localcurrentterrainheight_f;

										// and move plane to discovered surface position (for specific modes)
										if (iInitialPlacementOfPlane == 2)
										{
											if (iObjectMoveMode == 2)
											{
												if (g.entityrubberbandlist.size() > 1)
												{
													// groups need to find floor fast!
													fActivePosY = fHitPointY;
												}
												else
												{
													if (g_bAdjustPlaneXZUsingSurfaceXZ == true)
													{
														float fUpDownAngle = WrapValue(CameraAngleX(0));
														int iForwardFacing = t.entityprofile[t.gridentity].forwardfacing;
														if (t.gridentitygridlock == 0 || iForwardFacing != 0)
														{
															if (iForwardFacing == 0 && fUpDownAngle > 10.0f && fUpDownAngle < 91.0f)
															{
																if (iObjectMoveMode == 2 && iObjectMoveModeDropSystemUsing == 1)
																{
																	// smart mode keeps the initial XZ plane height for predictable positioning of object
																	// then uses the iObjectMoveModeDropSystem to plop the object on the ground
																}
																else
																{
																	// normally go to surface
																	fActivePosY = fHitPointY;
																}
															}
															if (iForwardFacing == 1)
															{
																fActivePosX = t.inputsys.localx_f;
																fActivePosZ = t.inputsys.localy_f;
																fActivePosY = fHitPointY;
															}
															if (iForwardFacing == 2)
															{
																fActivePosY = fHitPointY;
															}
														}
														g_bAdjustPlaneXZUsingSurfaceXZ = false;
													}
												}

												// small shifts cause glitch grabs, so round them off
												fActivePosX = floor(fActivePosX);
												fActivePosY = floor(fActivePosY);
												fActivePosZ = floor(fActivePosZ);
											}
										}
									}
								}

								// LB: moved from below, needed this mode whether in grid mode or not
								if (iObjectMoveMode == 1)
								{
									newpicksystem = -98;
								}

								// LB: mousepick functionality disabled for now, see how new smart find ground system works out...
								if (!(fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0))
								{
									int iRealObjectMoveMode = iObjectMoveMode;
									if (iObjectMoveModeDropSystem > 0) iRealObjectMoveMode = 0;
								}

								// only move plane on initial object selection, this avoids drift when placing objects
								bool bIgnoreFirstPlaneDetectItIsWrong = false;
								if (iInitialPlacementOfPlane == 1)
								{
									bool bJustForInitialDragIn = false;
									if (bDraggingActive == false && fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0) bJustForInitialDragIn = true;
									if (bDraggingActive == true && t.gridentityposx_f == 0 && t.gridentityposz_f == 0) bJustForInitialDragIn = true;
									if (bDraggingActiveInitial == true)	bJustForInitialDragIn = true;
									if (bJustForInitialDragIn == true && t.inputsys.localselectedrayhit == false)
									{
										// new selection from left panel
										g_bResetPlaneAfterXZAdjust = false;
										float fDistCap = 1000.0f;
										MoveCamera(0, fDistCap);
										fActivePosX = CameraPositionX(0);
										fActivePosY = CameraPositionY(0);
										fActivePosZ = CameraPositionZ(0);
										MoveCamera(0, -fDistCap);
									}
									else
									{
										// selected object from level
										fActivePosX = t.gridentityposx_f;
										fActivePosZ = t.gridentityposz_f;

										// only put plane on surface if object connected with a surface
										if (iObjectMoveMode == 2)
										{
											bool bMustFaceDown = false;
											int iForwardFacing = t.entityprofile[t.gridentity].forwardfacing;
											float fUpDownAngle = WrapValue(CameraAngleX(0));
											if (fUpDownAngle > -30.0f && fUpDownAngle < 91.0f) //LB: was 10.0f - Rick wanted to find floor when looking camera directly forward
											{
												if (iObjectMoveModeDropSystemUsing == 1)
												{
													// drop system keeps object on initial plane
													fActivePosY = t.gridentityposy_f + fHitOffsetY;
												}
												else
												{
													if (t.gridentitygridlock != 0 && iForwardFacing == 0 && bJustForInitialDragIn == false)
													{
														// if in grid/snap mode, do not use surface Y
														fActivePosY = t.gridentityposy_f + fHitOffsetY;
													}
													else
													{
														// find nearby surface
														fActivePosY = fHitPointY;
													}
												}
											}
											else
											{
												// side/up
												fActivePosY = t.gridentityposy_f + fHitOffsetY;
											}
										}
										else
										{
											if (iObjectMoveMode == 1)
											{
												// LB: directly on site of initial click
												fActivePosX = t.gridentityposx_f + fHitOffsetX;
												fActivePosY = t.gridentityposy_f + fHitOffsetY;
												fActivePosZ = t.gridentityposz_f + fHitOffsetZ;
											}
											else
											{
												fActivePosY = t.gridentityposy_f + fHitOffsetY;
											}
										}
									}

									// small shifts cause glitch grabs, so round them off
									fActivePosX = floor(fActivePosX);
									fActivePosY = floor(fActivePosY);
									fActivePosZ = floor(fActivePosZ);
									iInitialPlacementOfPlane = 2;
									bIgnoreFirstPlaneDetectItIsWrong = true; // because it only seems to work after wicked has synced all of a sudden, ah well.
								}

								// handed to smart system for plane position and orientation
								t.widget.pickedSection = newpicksystem;

								// find where mouse is on the plane we have positioned
								bool widget_getplanepos(float fActivePosX, float fActivePosY, float fActivePosZ, float* pPlanePosX, float* pPlanePosY, float* pPlanePosZ);
								bPlanePosRegistered = widget_getplanepos(fActivePosX, fActivePosY, fActivePosZ, &fPlanePosX, &fPlanePosY, &fPlanePosZ);
								if (bPlanePosRegistered == true && bIgnoreFirstPlaneDetectItIsWrong == false)
								{
									fPlanePosX -= fHitOffsetX;
									fPlanePosY -= fHitOffsetY;
									fPlanePosZ -= fHitOffsetZ;
									if (newpicksystem == -98)
									{
										// adjusting only Y
										t.gridentityposy_f = fPlanePosY;

										//leelee, maybe some grid/snap here so vertical make sense?
										//LB: sure thing Lee, here you go, see below
										if (t.gridentitygridlock == 2 && iObjectMoveMode == 1)
										{
											if (pref.fEditorGridSizeY > 0)
											{
												// snap to grid - dding grid Y mode in 2025!
												t.gridentityposy_f = fPlanePosY;
												float fGripY = t.gridentityposy_f + (pref.fEditorGridSizeY / 2);
												fGripY -= pref.fEditorGridOffsetY;
												if (fGripY < 0)
													fGripY = ((int(fGripY / pref.fEditorGridSizeY) - 1) * pref.fEditorGridSizeY);
												else
													fGripY = (int(fGripY / pref.fEditorGridSizeY) * pref.fEditorGridSizeY);

												// only if above or on terrain
												fGripY += pref.fEditorGridOffsetY;
												float fTerrainAtThisPoint = BT_GetGroundHeight (0, t.gridentityposx_f, t.gridentityposz_f);

												//PE: Allow object to go 80% below terrain.
												if (iActiveObj > 0)
												{
													//PE: Object can go under terrain by 80%.
													float fAllowBelowTerrainMax = (ObjectSizeY(iActiveObj, 1) * 0.80f);
													fTerrainAtThisPoint -= fAllowBelowTerrainMax;
												}
												if (fGripY < fTerrainAtThisPoint)
												{
													fGripY = fTerrainAtThisPoint;
												}
												t.gridentityposy_f = fGripY;
											}
										}
									}
									else
									{
										// ensure verticle only means just that!
										if (t.gridentitygridlock == 2 && iObjectMoveMode == 1)
										{
											// snap to grid - dding grid Y mode in 2025!
										}
										else
										{
											// adjusting X and Z
											t.gridentityposx_f = fPlanePosX;
											t.gridentityposz_f = fPlanePosZ;
										}
									}
								}

								//DEBUG: if (ObjectExist(t.widget.widgetPlaneObj)) ShowObject(t.widget.widgetPlaneObj);
								t.widget.pickedSection = picksystem;
							}
							else
							{
								t.gridentityposx_f = t.inputsys.localx_f + t.inputsys.dragoffsetx_f;
								t.gridentityposz_f = t.inputsys.localy_f + t.inputsys.dragoffsety_f;
							}
						}

						bool bUseOldYSystem = true;

						bool bDontUsePivot = false;
						if (iExtractMode == 1) 
						{ 
							//0 = find floor, 1 = extracted y value. , 3 = fixed y value.
							t.gridentityposy_f = fExtractYValue;
							bUseOldYSystem = false;
							bDontUsePivot = true;
						}
						if (iExtractMode == 2) 
						{
							t.gridentityposy_f = fExtractFixedYValue;
							bUseOldYSystem = false;
						}


						if (t.tforcedynamic == 1)
						{
							t.gridentitystaticmode = 0;
						}

						//PE: Display red/green box of cursor object. to display static/dynamic.
						if (!Shooter_Tools_Window_Active)
						{
							//PE: Allow t.widget.pickedObject to be selected in this mode.
							if (!pref.iEnableDragDropEntityMode)
							{
								g_selected_editor_object = NULL;
								g_selected_editor_objectID = 0;
							}
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
						}

						// Find ground while placing entity on terrain
						bool bApplyEntityOffsets = false;
						bool bCanUpdateY = true;
						int iForwardFacing = 0;
						if (t.gridentity > 0) iForwardFacing = t.entityprofile[t.gridentity].forwardfacing;
						if (t.gridentitysurfacesnap == 1 || t.inputsys.picksystemused == 2)
						//if (t.gridentity > 0 && (t.gridentitysurfacesnap == 1 || t.inputsys.picksystemused == 2)) // code only needed when picked cursor object - bit not using this for now as currently stable and tested
						{
							if (pref.iEnableDragDropEntityMode)
							{
								//PE: Only newly created obects can also adjust Y
								//PE: If we grab one from the level, it should use the settings. Y or XZ.
								if (!(fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0))
								{
									// object being dragged about and already in the level
									if (pref.iEnableDragDropWidgetSelect)
									{
										// but only if not dragging in objects that need fHitOffsetXYZ offset at very start
										bool bCanActivateWidgetNow = true;
										if (t.entityprofile[t.gridentity].ismarker == 2) bCanActivateWidgetNow = false;
										if (iForwardFacing == 2 && t.entityprofile[t.gridentity].ismarker == 0) bCanActivateWidgetNow = false;
										if (bCanActivateWidgetNow == true)
										{
											bTriggerVisibleWidget = true;
										}
									}
									else
										bTriggerVisibleWidget = false;

									int iRealObjectMoveMode = iObjectMoveMode;
									if (iObjectMoveModeDropSystem > 0) iRealObjectMoveMode = 0;
									if (iRealObjectMoveMode == 2)
									{
										bCanUpdateY = true;
									}
									else
									{
										// cannot update Y if horiz or vert positioning modes
										bCanUpdateY = false;
									}
								}
								else
								{
									// dragged direct from 'Level objects'
									if (bWidgetHasControlHere == false)
									{
										//LB: keep widget visible when dragging it about
										bTriggerVisibleWidget = false;
									}
								}

								//LB: requested ease of use feature, if in horiz mode, and object falls below terrain, raise it up
								if (iObjectMoveMode == 0)
								{
									// horiz position XZ mode
									if (bDepartedFromChosenY == true) t.gridentityposy_f = fDepartedFromThisY;
									float fActualHeightUnderObject = BT_GetGroundHeight(0, t.gridentityposx_f, t.gridentityposz_f);
									//PE: Allow object to go 80% below terrain.
									if (iActiveObj > 0 )
									{
										// and ensure we CAN place objects that are submerged, just make sure they do not go entirely under
										// the floor, so keep 20% of them anove the ground height
										float fAllowBelowTerrainMax = ((float)ObjectSizeY(iActiveObj, 1) * 0.8f);
										fActualHeightUnderObject -= fAllowBelowTerrainMax;
									}
									if (t.gridentityposy_f < fActualHeightUnderObject )
									{
										if (bDepartedFromChosenY == false)
										{
											bDepartedFromChosenY = true;
											fDepartedFromThisY = t.gridentityposy_f;
										}
										t.gridentityposy_f = fActualHeightUnderObject;
										bApplyEntityOffsets = true;
									}
								}
							}

							// when drag in object initially, use new smart placement system
							bool bJustForInitialDragIn = false;
							if (bDraggingActive == false && fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0) bJustForInitialDragIn = true;
							if (bDraggingActive == true && t.gridentityposx_f == 0 && t.gridentityposz_f == 0) bJustForInitialDragIn = true;
							if (bDraggingActiveInitial == true)	bJustForInitialDragIn = true;

							// new system to locate grid ent pos at point where mouse touches terrain/entity surface
							// LB: Note, "t.inputsys.localcurrentterrainheight_f" will not account for 'entity surface' if iObjectMoveMode != 2!!
							if ((bUseOldYSystem && bCanUpdateY) || bJustForInitialDragIn == true)
							{
								if ((iObjectMoveMode == 2 && (t.gridentitygridlock == 0 || iForwardFacing != 0)) || bJustForInitialDragIn == true)
								{
									if (bJustForInitialDragIn == true)
									{
										if (t.inputsys.localselectedrayhit == true)
										{
											t.gridentityposx_f = t.inputsys.localx_f - fHitOffsetX;
											t.gridentityposz_f = t.inputsys.localy_f - fHitOffsetZ;
											t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f - fHitOffsetY;
										}
										else
										{
											if (bPlanePosRegistered == true)
											{
												t.gridentityposy_f = fPlanePosY;
											}
										}
									}
									else
									{
										if (bPlanePosRegistered == true)
										{
											if (iForwardFacing == 2)
											{
												float fUpDownAngle = WrapValue(CameraAngleX(0));
												if ((fUpDownAngle > 10.0f && fUpDownAngle < 350.0f) || (t.gridentity>0 && t.entityprofile[t.gridentity].ismarker == 2))
												{
													t.gridentityposx_f = t.inputsys.localx_f - fHitOffsetX;
													t.gridentityposz_f = t.inputsys.localy_f - fHitOffsetZ;
													t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f - fHitOffsetY;
												}
												else
												{
													t.gridentityposx_f = t.inputsys.localx_f;
													t.gridentityposz_f = t.inputsys.localy_f;
													t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f;
												}
											}
											else
											{
												t.gridentityposy_f = fPlanePosY;
											}
										}
										float fUpDownAngle = WrapValue(CameraAngleX(0));
										if (fUpDownAngle > 10.0f && fUpDownAngle < 350.0f)
										{
											// horizontal plane handles this - stops shake that happens when use ray Y instead of plane Y
										}
										else
										{
											// plane is vertical, so need to determine if ray hit closer than vertical plane
											if (t.inputsys.localselectedrayhit == true && iForwardFacing == 0)
											{
												float fDX = t.inputsys.localx_f - CameraPositionX(0);
												float fDY = t.inputsys.localcurrentterrainheight_f - CameraPositionY(0);
												float fDZ = t.inputsys.localy_f - CameraPositionZ(0);
												float fDistToPick = sqrt(fabs(fDX*fDX) + fabs(fDY*fDY) + fabs(fDZ*fDZ));
												fDX = fPlanePosX - CameraPositionX(0);
												fDY = fPlanePosY - CameraPositionY(0);
												fDZ = fPlanePosZ - CameraPositionZ(0);
												float fDistToPlane = sqrt(fabs(fDX*fDX) + fabs(fDY*fDY) + fabs(fDZ*fDZ));
												if (fDistToPlane > fDistToPick)
												{
													t.gridentityposx_f = t.inputsys.localx_f - fHitOffsetX;
													t.gridentityposz_f = t.inputsys.localy_f - fHitOffsetZ;
													t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f - fHitOffsetY;
												}
											}
										}
									}
								}
								else
								{
									if (t.gridentitygridlock == 0 || iForwardFacing != 0)
									{
										t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f;
									}
								}
								bApplyEntityOffsets = true;
							}

							if (fHitPointY == 0.0f)
							{
								fHitPointY = t.gridentityposy_f;
							}

							//LB: prevent any t.gridentitypos movement until user has moved the mouse pointer
							if (g_bHoldGridEntityPosWhenManaged == true)
							{
								t.gridentityposx_f = g_fHoldGridEntityPosX;
								t.gridentityposy_f = g_fHoldGridEntityPosY;
								t.gridentityposz_f = g_fHoldGridEntityPosZ;
							}
						}
						else
						{
							if (t.gridentityposoffground == 0)
							{
								if (bUseOldYSystem)
								{
									t.gridentityposy_f = BT_GetGroundHeight(t.terrain.TerrainID, t.gridentityposx_f, t.gridentityposz_f);
									bApplyEntityOffsets = true;
								}
							}
						}

						//PE: Prevent user for placing objects outside playable area.
						if (t.gridentityobj > 0 && t.gridentity > 0)
						{
							if (t.gridentityposx_f < t.terraineditableareasizeminx) { t.gridentityposx_f = fOldgridentityposx_f; t.gridentityposy_f = fOldgridentityposy_f; bObjectOutSideEditArea = true; }
							if (t.gridentityposx_f > t.terraineditableareasizemaxx) { t.gridentityposx_f = fOldgridentityposx_f; t.gridentityposy_f = fOldgridentityposy_f; bObjectOutSideEditArea = true; }
							if (t.gridentityposz_f < t.terraineditableareasizeminz) { t.gridentityposz_f = fOldgridentityposz_f; t.gridentityposy_f = fOldgridentityposy_f; bObjectOutSideEditArea = true; }
							if (t.gridentityposz_f > t.terraineditableareasizemaxz) { t.gridentityposz_f = fOldgridentityposz_f; t.gridentityposy_f = fOldgridentityposy_f; bObjectOutSideEditArea = true; }
							if (bObjectOutSideEditArea)
							{
								//Trigger warning.
								t.gridentityposy_f = BT_GetGroundHeight(0, t.gridentityposx_f, t.gridentityposz_f);
								sprintf(cSmallTriggerMessage, "Object is Outside Editable Area");
								iTriggerMessageFrames = 60;
								bTriggerSmallMessage = true;
							}
						}

						//LB: now apply grid/snap when user finished positioning XYZ
						if (iForwardFacing != 1)
						{
							// do not apply grid/snap for things like switches, they NEED the XZ from the surface to be exact
							Add_Grid_Snap_To_Position(false);
						}

						// handle any pivots that are object based
						if (!bDontUsePivot ) ApplyPivotToGridEntity();

						// Create and manage a ghost object for when selecting objects to move
						// so can be used by special smart positioning mode
						int iGhostObj = g.ghostcursorobjectoffset;
						if (iGhostObj > 0)
						{
							// remove if not needed
							if (ObjectExist(iGhostObj) == 1 && t.gridentityobj == 0) DeleteObject (iGhostObj);

							// create placement line of needed
							if (iObjectMoveMode == 0 || iObjectMoveMode == 1)
							{
								// create a thin line to indicate where the surface below the object is
								if (ObjectExist(iGhostObj) == 0 && t.gridentityobj > 0)
								{
									WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
									MakeObjectBox(iGhostObj, 1, 1, 1);
									WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
									sObject* pObject = GetObjectData(iGhostObj);
									WickedCall_TextureObject(pObject, NULL);
									if (pObject)
									{
										for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
										{
											sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
											if (pMesh)
											{
												pMesh->mMaterial.Diffuse.r = 1.0f;
												pMesh->mMaterial.Diffuse.g = 1.0f;
												pMesh->mMaterial.Diffuse.b = 1.0f;
												pMesh->mMaterial.Diffuse.a = 0.2f;
												WickedCall_SetMeshMaterial (pMesh, false);
												pMesh->bTransparency = true;
												WickedCall_SetMeshTransparent(pMesh);
											}
										}
										WickedCall_SetObjectCastShadows(pObject, false);
									}
								}
								if (ObjectExist(iGhostObj) == 1)
								{
									// position line so it fills the gap directly below the object
									sObject* pObject = GetObjectData(t.gridentityobj);
									float fRealMeshCenterY = pObject->collision.vecMin.y + ((pObject->collision.vecMax.y - pObject->collision.vecMin.y) / 2.0f);
									fRealMeshCenterY *= pObject->position.vecScale.y;
									float fY = ObjectPositionY(t.gridentityobj) + fRealMeshCenterY;
									float fPredictedY = editor_forceentityfindfloor(true);
									if (fPredictedY > fY) fPredictedY = 0.0f;
									float fX = ObjectPositionX(t.gridentityobj);
									float fZ = ObjectPositionZ(t.gridentityobj);
									float fDefaultHeight = t.entityprofile[t.gridentity].defaultheight;
									fY += fDefaultHeight;
									float fGapDistance = fY - fPredictedY;
									float fHalfWay = fPredictedY + (fGapDistance / 2.0f);
									PositionObject (iGhostObj, fX, fHalfWay-fDefaultHeight, fZ);
									ScaleObject(iGhostObj, 25, fGapDistance*100.0f, 25);
								}
							}

							//PE: When first dragged in we dont have a entityelement and cant check usespotlighting so...
							//PE: Dont make ghost on light objects.
							bool bIsLightObject = false;
							if (t.gridentity > 0 && t.entityprofile[t.gridentity].ismarker == 2)
							{
								bIsLightObject = true;
								//PE: Perhaps do this later, if needed stacking light is not really used :) , and only if we have a extracted entityid.
								//int spotlighting = t.entityelement[e].eleprof.usespotlighting;
								//entity_updatelightobjtype(obj, spotlighting);
							}

							// create ghost if needed
							bool bShowGhostIfSingleObject = false;
							if (iObjectMoveModeDropSystemUsing == 1 && g.entityrubberbandlist.size() <= 1 && g_bHoldGridEntityPosWhenManaged == false) bShowGhostIfSingleObject = true;
							if (iObjectMoveMode == 2 && bShowGhostIfSingleObject==true && !bIsLightObject && bDraggingActive)
							{
								if (ObjectExist(iGhostObj) == 0 && t.gridentityobj > 0)
								{
									WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
									CloneObject (iGhostObj, t.gridentityobj);
									WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
									sObject* pObject = GetObjectData(iGhostObj);
									WickedCall_TextureObject(pObject, NULL);
									if (pObject)
									{
										for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
										{
											sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
											if (pMesh)
											{
												pMesh->mMaterial.Diffuse.r = 1.0f;
												pMesh->mMaterial.Diffuse.g = 1.0f;
												pMesh->mMaterial.Diffuse.b = 1.0f;
												pMesh->mMaterial.Diffuse.a = 0.3f;
												WickedCall_SetMeshMaterial (pMesh, false);
												pMesh->bTransparency = true;
												WickedCall_SetMeshTransparent(pMesh);
											}
										}
										WickedCall_SetObjectCastShadows(pObject, false);
									}
								}
								if (ObjectExist(iGhostObj) == 1)
								{
									// position ghost version to the intended destination of a smart positioned object
									float fPredictedY = editor_forceentityfindfloor(true);
									float fX = ObjectPositionX(t.gridentityobj);
									float fY = ObjectPositionY(t.gridentityobj);
									float fZ = ObjectPositionZ(t.gridentityobj);
									PositionObject (iGhostObj, fX, fPredictedY, fZ);
									SetObjectToObjectOrientation(iGhostObj, t.gridentityobj);
									float fCurrentObjFrame = WickedCall_GetObjectFrame(GetObjectData(t.gridentityobj));
									SetObjectFrame(iGhostObj, fCurrentObjFrame);
								}
							}
						}

						//PE: Check if object is overlapping another object.
						//PE: MISSING - Lock position if object is not visible to camera (goes to the other side of another object).
						int iRealObjectMoveMode = iObjectMoveMode;
						if (iObjectMoveModeDropSystem > 0) iRealObjectMoveMode = 0;
						if (bObjectAllowOverlapping == 0 && (iRealObjectMoveMode == 0 || iRealObjectMoveMode == 2)) //iObjectMoveMode NO Y rays yet.
						{
							//#define CENTERSYSTEM
							#define NOOVERLABDEBUG
							sObject* pObject = g_ObjectList[t.gridentityobj];
							if (pObject)
							{
								PositionObject(t.obj, t.gridentityposx_f, t.gridentityposy_f, t.gridentityposz_f);
								RotateObject(t.obj, t.gridentityrotatex_f, t.gridentityrotatey_f, t.gridentityrotatez_f);
								
								//only set t.gridentityposx_f , t.gridentityposz_f
								ImVec4 ObjectCenter = GetRealCenterToGridEntity();
								ImVec4 ObjectSize;
								if (ObjectCenter.w) //Only if valid object.
								{
									XMFLOAT3 fObjCenter = { pObject->position.vecPosition.x + ObjectCenter.x,pObject->position.vecPosition.y + ObjectCenter.y,pObject->position.vecPosition.z + ObjectCenter.z };

									void WickedCall_DrawPoint(float fx, float fy, float fz, float size, XMFLOAT4 color, bool bThickLine);
									XMFLOAT4 color = { 1.0,1.0,0.0,1.0 }; //real center of object.
									#ifndef NOOVERLABDEBUG
									WickedCall_DrawPoint(fObjCenter.x, fObjCenter.y, fObjCenter.z, 4.0f, color, true);
									#endif

									static float fLastRayDist[6];
									ImVec4 vBestOffsetLeft = { 0,0,0,0 };
									ImVec4 vBestOffsetUp = { 0,0,0,0 };
									for (int i = 0; i < 4; i++)
									{
										if (i == 0) color = { 1.0,1.0,1.0,1.0 };
										if (i == 1) color = { 1.0,0.0,0.0,1.0 };
										if (i == 2) color = { 0.0,1.0,0.0,1.0 };
										if (i == 3) color = { 0.0,0.0,1.0,1.0 };
										ObjectSize = GetRealSizeToGridEntity(i);
										XMFLOAT3 fObjPos = { pObject->position.vecPosition.x + ObjectCenter.x + (ObjectSize.x) , pObject->position.vecPosition.y + ObjectCenter.y ,pObject->position.vecPosition.z + ObjectCenter.z + (ObjectSize.z) };
										#ifndef CENTERSYSTEM
										XMFLOAT3 fObjRayPos = { pObject->position.vecPosition.x + ObjectCenter.x - (ObjectSize.x) , pObject->position.vecPosition.y + ObjectCenter.y ,pObject->position.vecPosition.z + ObjectCenter.z - (ObjectSize.z) };
										#ifndef NOOVERLABDEBUG
										WickedCall_DrawPoint(fObjRayPos.x, fObjRayPos.y, fObjRayPos.z, 3.0f, color, true);
										#endif
										#else
										#ifndef NOOVERLABDEBUG
										WickedCall_DrawPoint(fObjPos.x, fObjPos.y, fObjPos.z, 4.0f, color, true);
										#endif
										#endif
										XMVECTOR vectorSub = XMVectorSubtract(XMLoadFloat3(&fObjPos), XMLoadFloat3(&fObjCenter));
										XMVECTOR rayDirection = XMVector3Normalize(vectorSub);
										XMFLOAT3 f3Dir;
										XMStoreFloat3(&f3Dir, rayDirection);
										float fHitX, fHitY, fHitZ, fdist = 99999.0;
										#ifndef CENTERSYSTEM
										if (WickedCall_SentRay(fObjRayPos.x, fObjRayPos.y, fObjRayPos.z, f3Dir.x, 0.0f, f3Dir.z, &fHitX, &fHitY, &fHitZ, NULL, NULL, NULL, NULL, GGRENDERLAYERS_NORMAL))
										#else
										if (WickedCall_SentRay(fObjCenter.x, fObjCenter.y, fObjCenter.z, f3Dir.x, 0.0f, f3Dir.z, &fHitX, &fHitY, &fHitZ, NULL, NULL, NULL, NULL, GGRENDERLAYERS_NORMAL))
										#endif
										{
											XMFLOAT3 fObjHit = { fHitX, fHitY, fHitZ };

											#ifndef CENTERSYSTEM
											float fdist = sqrt((fObjPos.x - fObjHit.x) * (fObjPos.x - fObjHit.x) +
												(fObjPos.z - fObjHit.z) * (fObjPos.z - fObjHit.z));
											fLastRayDist[i] = fdist;
											float fdist2 = sqrt((fObjRayPos.x - fObjPos.x) * (fObjRayPos.x - fObjPos.x) +
												(fObjRayPos.z - fObjPos.z) * (fObjRayPos.z - fObjPos.z));

											float fdist3 = sqrt((fObjRayPos.x - fObjHit.x) * (fObjRayPos.x - fObjHit.x) +
												(fObjRayPos.z - fObjHit.z) * (fObjRayPos.z - fObjHit.z));

											#else
											float fdist = sqrt((fObjPos.x - fObjHit.x) * (fObjPos.x - fObjHit.x) +
												(fObjPos.z - fObjHit.z) * (fObjPos.z - fObjHit.z));

											float fdist2 = sqrt((fObjCenter.x - fObjPos.x) * (fObjCenter.x - fObjPos.x) +
												(fObjCenter.z - fObjPos.z) * (fObjCenter.z - fObjPos.z));

											float fdist3 = sqrt((fObjCenter.x - fObjHit.x) * (fObjCenter.x - fObjHit.x) +
												(fObjCenter.z - fObjHit.z) * (fObjCenter.z - fObjHit.z));
											#endif

											float fDistFinal = fdist3 - fdist2;
											fDebug = fdist; //debug
											fDebug2 = fdist2; //debug
											fDebug3 = fdist3; //debug

											if (fdist3 < fdist2)
											{
												//Glue.
												float fDistFromPivot = -fdist;
												ImVec4 vOffset = { f3Dir.x*fDistFromPivot, f3Dir.y*fDistFromPivot , f3Dir.z*fDistFromPivot ,1.0f };

												if (i == 0)
													vBestOffsetLeft = vOffset;
												if (i == 2 && fLastRayDist[2] < fLastRayDist[0])
													vBestOffsetLeft = vOffset;
												if (i == 1)
													vBestOffsetUp = vOffset;
												if (i == 3 && fLastRayDist[3] < fLastRayDist[1])
													vBestOffsetUp = vOffset;
											}
											#ifndef NOOVERLABDEBUG
											WickedCall_DrawPoint(fHitX, fHitY, fHitZ, 6.0f, color, true);
											#endif
										}
									}
									if (vBestOffsetLeft.w > 0)
									{
										t.gridentityposx_f += vBestOffsetLeft.x;
										t.gridentityposz_f += vBestOffsetLeft.z;
									}
									if (vBestOffsetUp.w > 0)
									{
										t.gridentityposx_f += vBestOffsetUp.x;
										t.gridentityposz_f += vBestOffsetUp.z;
									}
								}
							}
						}

						//  move waypoint zone when move trigger entity
						if (t.grideleprof.trigger.waypointzoneindex > 0)
						{
							if (t.gridentity > 0)
							{
								waypoint_movetogrideleprof();
							}
							else
							{
								t.grideleprof.trigger.waypointzoneindex = 0;
							}
						}

						//  control modification of entity element details
						if (t.gridentitymodifyelement == 1)
						{
							if (t.gridedit.entityspraymode == 1)
							{
								if (t.gridedit.entitysprayrange > 0) t.gridedit.entitysprayrange -= 50;
							}
							else
							{
								if (t.grideleprof.light.range > 50) t.grideleprof.light.range -= 50;
							}
							t.gridentitymodifyelement = 0;
						}
						if (t.gridentitymodifyelement == 2)
						{
							if (t.gridedit.entityspraymode == 1)
							{
								if (t.gridedit.entitysprayrange < 1000) t.gridedit.entitysprayrange += 50;
							}
							else
							{
								if (t.grideleprof.light.range < 1000) t.grideleprof.light.range += 50;
							}
							t.gridentitymodifyelement = 0;
						}

					} //WICKED (!bPlaceEntity)

					//  extract entity (RMB) or place entity (LMB)
					if (bPlaceEntity)
					{
						bDetectTerrainOnly = false;
						// widget closure
						if (  t.widget.duplicatebuttonselected == 2 ) 
						{
							t.tentitytoselect=t.widget.pickedEntityIndex;
							t.widget.duplicatebuttonselected=0;
							t.gridentityautofind=7;
						}
						t.widget.pickedObject=0; 
						widget_updatewidgetobject ( );

						// either add entity to map OR extract one specified by 't.tentitytoselect'
						if ( t.gridentity != 0 ) 
						{
							// ADD ENTITY TO MAP
							// Determine if we will be adding (or moving an entity if spray mode)
							t.tentitybeingsprayed=0;
							t.tentitytomodifyindex=0;
							if ( t.gridedit.entityspraymode == 1 && t.entityprofile[t.gridentity].ismarker == 0 ) 
							{
								//  Scan area of spray and determine if density reached
								t.tpickrandoment=Rnd(t.tcountentinrange);
								t.tcountentinrange=0;
								for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
								{
									if (  t.entityelement[t.e].bankindex == t.gridentity ) 
									{
										//  this entity is same as that sprayed
										t.tdx_f=t.entityelement[t.e].x-t.gridentityposx_f;
										t.tdz_f=t.entityelement[t.e].z-t.gridentityposz_f;
										t.tdd_f=Sqrt(abs(t.tdx_f*t.tdx_f)+abs(t.tdz_f*t.tdz_f));
										if (  t.tdd_f <= t.gridedit.entitysprayrange ) 
										{
											//  this entity in range
											if (  t.tcountentinrange == 0  )  t.tentitytomodifyindex = t.e;
											if (  t.tcountentinrange == t.tpickrandoment  )  t.tentitytomodifyindex = t.e;
											++t.tcountentinrange;
										}
									}
								}
								if (  t.tcountentinrange <= 5  )  t.tentitytomodifyindex = 0;
								t.tentitybeingsprayed=1;
							}

							//  Store original entity cursor settings (for later random feature)
							t.storegridentityposx_f=t.gridentityposx_f;
							t.storegridentityposz_f=t.gridentityposz_f;
							t.storegridentityrotatex_f=t.gridentityrotatex_f;
							t.storegridentityrotatey_f=t.gridentityrotatey_f;
							t.storegridentityrotatez_f=t.gridentityrotatez_f;
							t.storegridentityrotatequatmode = t.gridentityrotatequatmode;
							t.storegridentityrotatequatx_f = t.gridentityrotatequatx_f;
							t.storegridentityrotatequaty_f = t.gridentityrotatequaty_f;
							t.storegridentityrotatequatz_f = t.gridentityrotatequatz_f;
							t.storegridentityrotatequatw_f = t.gridentityrotatequatw_f;
							t.storegridentityscalex_f=t.gridentityscalex_f;
							t.storegridentityscaley_f=t.gridentityscaley_f;
							t.storegridentityscalez_f=t.gridentityscalez_f;

							//  Spray allows entity placement to be randomised
							if (  t.tentitybeingsprayed == 1 ) 
							{
								//  rotation and scale back in (also uses native entity ROT values)
								t.gridentityrotatex_f=t.entityprofile[t.gridentity].rotx;
								t.gridentityrotatey_f=t.entityprofile[t.gridentity].roty;
								t.gridentityrotatez_f=t.entityprofile[t.gridentity].rotz;
								t.gridentityrotatequatmode = 0;
								t.gridentityscalex_f=t.gridentityscalex_f;
								t.gridentityscaley_f=t.gridentityscaley_f;
								t.gridentityscalez_f=t.gridentityscalez_f;
								if ( t.entityprofile[t.gridentity].ischaracter==0 && t.entityprofile[t.gridentity].noXZrotation==0 )
								{
									// ignore X and Z rotation for characters
									t.gridentityrotatex_f=t.gridentityrotatex_f+10.0-Rnd(20);
									t.gridentityrotatez_f=t.gridentityrotatez_f+10.0-Rnd(20);
									t.gridentityscaley_f=t.gridentityscaley_f+(Rnd(20)-10);
								}
								t.gridentityrotatey_f=t.gridentityrotatey_f+Rnd(360);
								t.gridentityrotatequatmode = 0;
								t.gridentityrotatequatx_f = 0;
								t.gridentityrotatequaty_f = 0;
								t.gridentityrotatequatz_f = 0;
								t.gridentityrotatequatw_f = 1;
								t.ttrandomposx_f=NewXValue(0,Rnd(360),Rnd(t.gridedit.entitysprayrange));
								t.ttrandomposz_f=NewZValue(0,Rnd(360),Rnd(t.gridedit.entitysprayrange));
								t.gridentityposx_f=t.gridentityposx_f+t.ttrandomposx_f;
								t.gridentityposz_f=t.gridentityposz_f+t.ttrandomposz_f;
								t.gridentityposy_f=BT_GetGroundHeight(t.terrain.TerrainID,t.gridentityposx_f,t.gridentityposz_f);
								//PE: Apply pivot here.
								ApplyPivotToGridEntity();

							}

							//  Version Control - stop high resource use
							t.resourceused=2; //version_resourcewarning ( );

							//  Either modify existing entity or place a new one (default behaviour)
							if (  t.tentitytomodifyindex>0 ) 
							{
								//  MODIFY EXISTING ENTITY
								t.entityelement[t.tentitytomodifyindex].x=t.gridentityposx_f;
								t.entityelement[t.tentitytomodifyindex].y=t.gridentityposy_f;
								t.entityelement[t.tentitytomodifyindex].z=t.gridentityposz_f;
								t.entityelement[t.tentitytomodifyindex].rx=t.gridentityrotatex_f;
								t.entityelement[t.tentitytomodifyindex].ry=t.gridentityrotatey_f;
								t.entityelement[t.tentitytomodifyindex].rz=t.gridentityrotatez_f;
								t.entityelement[t.tentitytomodifyindex].quatmode = t.gridentityrotatequatmode;
								t.entityelement[t.tentitytomodifyindex].quatx = t.gridentityrotatequatx_f;
								t.entityelement[t.tentitytomodifyindex].quaty = t.gridentityrotatequaty_f;
								t.entityelement[t.tentitytomodifyindex].quatz = t.gridentityrotatequatz_f;
								t.entityelement[t.tentitytomodifyindex].quatw = t.gridentityrotatequatw_f;
								t.tobj=t.entityelement[t.tentitytomodifyindex].obj;
								if (  t.tobj>0 ) 
								{
									if (  ObjectExist(t.tobj) == 1 ) 
									{
										PositionObject (  t.tobj,t.gridentityposx_f,t.gridentityposy_f,t.gridentityposz_f );
										RotateObject (  t.tobj,t.gridentityrotatex_f,t.gridentityrotatey_f,t.gridentityrotatez_f );
									}
								}
								if (t.gridedit.entityspraymode == 1)
								{
									//PE: Set selection to the one being changed.
									t.widget.pickedObject = t.tobj;
									t.widget.pickedEntityIndex = t.tentitytomodifyindex;

								}
							}
							else
							{
								//  PLACE NEW ENTITY
								//  after add, adjust so it auto-finds a Floor (  or wall (convenience) )
								t.gridentitydroptoground=1+t.entityprofile[t.gridentity].forwardfacing;
								if (  t.gridentitydroptoground == 2 ) 
								{
									 // already dealt with, positioned perfectly
								}
								t.gridentitydroptoground=0;

								//  find unique name for this selection (if flagged)
								if (  g.guseuniquelynamedentities == 0 ) 
								{
									//  use same name as original entity
									t.tbase_s=t.grideleprof.name_s;
								}
								else
								{
									t.tokay=0 ; t.tindex=1;
									if (  cstr(Lower(Left(t.grideleprof.name_s.Get(),Len(t.grideleproflastname_s.Get())))) == Lower(t.grideleproflastname_s.Get()) ) 
									{
										t.tbase_s=t.grideleproflastname_s;
									}
									else
									{
										t.tbase_s=t.grideleprof.name_s;
									}
									while (  t.tokay == 0 ) 
									{
										t.tokay=1 ; t.grideleprof.name_s=t.tbase_s ; t.grideleproflastname_s=t.tbase_s;
										if (  t.tindex>1  )  t.grideleprof.name_s = t.grideleprof.name_s+" "+Str(t.tindex);
										for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
										{
											if (  t.entityelement[t.e].bankindex>0 ) 
											{
												if (  cstr(Lower(t.entityelement[t.e].eleprof.name_s.Get())) == cstr(Lower(t.grideleprof.name_s.Get())) ) 
												{
													//  this name exists already, try another
													t.tokay=0 ; break;
												}
											}
										}
										++t.tindex;
									}
								}
								//  player start markers have exclusivity
								if (  t.entityprofile[t.gridentity].ismarker == 1 && t.entityprofile[t.gridentity].lives != -1 ) 
								{
									for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
									{
										if (  t.entityelement[t.e].bankindex>0 ) 
										{
											if (  t.entityprofile[t.entityelement[t.e].bankindex].ismarker == 1 && t.entityprofile[t.entityelement[t.e].bankindex].lives != -1 ) 
											{
												t.tentitytoselect=t.e;
												gridedit_deleteentityfrommap ( );
											}
										}
									}
								}

								//  copy entity to map (keep selection for repeat process)
								if ( g.entityrubberbandlist.size() > 0 )
								{
									//PE: This normally makes a duplicate, what we dont need in this system.
									if ( 1 ) // when adding, need same functionality (smart object groups) pref.iEnableDragDropEntityMode )
									{
										//We might have to do some kind of support.
										int iStoreGridEntity = t.gridentity;

										// add parent entity
										//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
										extern bool bNextObjectMustBeClone;
										bNextObjectMustBeClone = true;

										gridedit_addentitytomap();

										bNextObjectMustBeClone = false;

										if (iLastEntityOnCursor != 0 && iLastEntityOnCursor != t.e)
										{
											ReplaceEntityInGroupList(iLastEntityOnCursor, t.e);
											iLastEntityOnCursor = 0;
										}

										int iNewParentEntityIndex = t.e;

										if (pref.iEnableDragDropEntityMode && t.e > 0) {
											//PE: After placing it, sent it to widget.
											iWidgetSelection = t.e;
										}
										bDraggingActive = false;
										t.gridentity = iStoreGridEntity;
										t.e = iNewParentEntityIndex;

										if (bCreateNewGroupOnNextDrop)
										{
											//Create a new group
											fLastRubberBandX2 = 0; //We dont have a rubberband so...
											fLastRubberBandX1 = 0;
											fLastRubberBandY2 = 0;
											fLastRubberBandY1 = 0;

											CreateNewGroup(-1);
											bCreateNewGroupOnNextDrop = false;
										}
									}
									else
									{
										// store original ent ID pased down
										int iStoreGridEntity = t.gridentity;

										// add parent entity

										//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
										extern bool bNextObjectMustBeClone;
										bNextObjectMustBeClone = true;

										gridedit_addentitytomap();

										bNextObjectMustBeClone = false;

										int iNewParentEntityIndex = t.e;

										t.entityelement[t.e].iHasParentIndex = t.gridentityhasparent;

										// add children for the parent
										int* piNewEntIndex = new int[g.entityrubberbandlist.size()];
										for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
										{
											int e = g.entityrubberbandlist[i].e;
											if (e == 0)
											{
												t.e = iNewParentEntityIndex;
											}
											else
											{
												// duplicate this child and add to map
												t.gridentity = t.entityelement[e].bankindex;

												//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
												extern bool bNextObjectMustBeClone;
												bNextObjectMustBeClone = true;

												gridedit_addentitytomap(); //use entityelement[t.e].

												bNextObjectMustBeClone = false;

												// update child with pos/rot from source
												t.entityelement[t.e].x = t.entityelement[e].x;
												t.entityelement[t.e].y = t.entityelement[e].y;
												t.entityelement[t.e].z = t.entityelement[e].z;
												t.entityelement[t.e].rx = t.entityelement[e].rx;
												t.entityelement[t.e].ry = t.entityelement[e].ry;
												t.entityelement[t.e].rz = t.entityelement[e].rz;			
												t.entityelement[t.e].quatmode = t.entityelement[e].quatmode;
												t.entityelement[t.e].quatx = t.entityelement[e].quatx;
												t.entityelement[t.e].quaty = t.entityelement[e].quaty;
												t.entityelement[t.e].quatz = t.entityelement[e].quatz;
												t.entityelement[t.e].quatw = t.entityelement[e].quatw;
												t.entityelement[t.e].editorfixed = t.entityelement[e].editorfixed;
												t.entityelement[t.e].staticflag = t.entityelement[e].staticflag;
												t.entityelement[t.e].scalex = t.entityelement[e].scalex;
												t.entityelement[t.e].scaley = t.entityelement[e].scaley;
												t.entityelement[t.e].scalez = t.entityelement[e].scalez;
												t.entityelement[t.e].soundset = t.entityelement[e].soundset;
												t.entityelement[t.e].soundset1 = t.entityelement[e].soundset1;
												t.entityelement[t.e].soundset2 = t.entityelement[e].soundset2;
												t.entityelement[t.e].soundset3 = t.entityelement[e].soundset3;
												t.entityelement[t.e].soundset4 = t.entityelement[e].soundset4;
												t.entityelement[t.e].soundset5 = t.entityelement[e].soundset5;
												t.entityelement[t.e].soundset6 = t.entityelement[e].soundset6;
												t.entityelement[t.e].eleprof = t.entityelement[e].eleprof;
												PositionObject(t.entityelement[t.e].obj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z);
												RotateObject(t.entityelement[t.e].obj, t.entityelement[t.e].rx, t.entityelement[t.e].ry, t.entityelement[t.e].rz);
											}
											piNewEntIndex[i] = t.e;
										}

										// and once all new entities created, link new parents to them
										for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
										{
											t.e = piNewEntIndex[i];
											int e = g.entityrubberbandlist[i].e;
											if (e != 0)
											{
												// find source parent of this child, if any
												if (t.entityelement[e].iHasParentIndex > 0)
												{
													if (t.entityelement[e].iHasParentIndex == t.gridentityextractedindex)
													{
														// entity was child of parent entity extacted
														t.entityelement[t.e].iHasParentIndex = iNewParentEntityIndex;
													}
													else
													{
														// entity was child of another entity (a child in here)
														for (int ii = 0; ii < (int)g.entityrubberbandlist.size(); ii++)
														{
															int ee = g.entityrubberbandlist[ii].e;
															if (ee > 0)
															{
																if (t.entityelement[e].iHasParentIndex == ee)
																{
																	// entity was child of parent entity extacted
																	t.entityelement[t.e].iHasParentIndex = piNewEntIndex[ii];
																}
															}
														}
													}
												}
											}
										}
										SAFE_DELETE(piNewEntIndex);

										// restore original ent ID
										t.gridentity = iStoreGridEntity;
										t.e = iNewParentEntityIndex;
									}
								}
								else
								{
									//TUT: Add here.
									//TUT: PLACEIT
									CheckTutorialAction("PLACEIT"); //Tutorial: check if we are waiting for this action
									if (bTutorialCheckAction) TutorialNextAction(); //If we are waiting for PLACEIT its done.

									//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
									extern bool bNextObjectMustBeClone;
									bNextObjectMustBeClone = true;

									if (t.gridedit.entityspraymode == 1 || t.inputsys.keyshift)
									{
										bNextObjectMustBeClone = false;
									}

									gridedit_addentitytomap ( );

									if (!bNextObjectMustBeClone)
									{
										// Any particle that was duplicated should have a new emitter id.
										if (t.entityprofile[t.entityelement[t.e].bankindex].ismarker == 10)
										{
											t.entityelement[t.e].eleprof.newparticle.emitterid = -1;
											entity_updateparticleemitter(t.e);
											//PE: Not from lua so must do these.
											int iParticleEmitter = t.entityelement[t.e].eleprof.newparticle.emitterid;
											if (iParticleEmitter != -1)
											{
												gpup_setParticleScale(iParticleEmitter, t.entityelement[t.e].eleprof.newparticle.bParticle_Size);
												gpup_setEffectAnimationSpeed(iParticleEmitter, t.entityelement[t.e].eleprof.newparticle.fParticle_Speed);
												gpup_setEffectOpacity(iParticleEmitter, t.entityelement[t.e].eleprof.newparticle.fParticle_Opacity);
											}

										}
									}

									bNextObjectMustBeClone = false;

									if (iLastEntityOnCursor != 0 && iLastEntityOnCursor != t.e)
									{
										ReplaceEntityInGroupList(iLastEntityOnCursor, t.e);
										iLastEntityOnCursor = 0;
									}

									if (pref.iEnableDragDropEntityMode && t.e > 0) 
									{
										//PE: After placing it, sent it to widget.
										iWidgetSelection = t.e;
										//PE: Instant activete it.
										t.widget.pickedEntityIndex = iWidgetSelection;
										t.entityelement[t.widget.pickedEntityIndex].editorlock = 0;
										t.widget.pickedObject = t.entityelement[iWidgetSelection].obj;
									}
									bDraggingActive = false;
								}

								//  if drag char to start marker, assign here
								if (  t.inputsys.willmakethirdperson>0 ) 
								{
									// set this characte as third person and game as TPP 
									t.playercontrol.thirdperson.enabled=1;
									t.playercontrol.thirdperson.charactere=t.tupdatee;
									t.playercontrol.thirdperson.startmarkere=t.inputsys.willmakethirdperson;

									// also change the script of the character to a third person script (by default)
									t.entityelement[t.tupdatee].eleprof.aimain_s = "tpp\\thirdperson.lua";
									t.entityelement[t.tupdatee].eleprof.aimain = 0;
								}

								//  if trigger zone, remove from entity cursor as well
								if (  t.entityprofile[t.gridentity].ismarker == 3 || t.entityprofile[t.gridentity].ismarker == 6 || t.entityprofile[t.gridentity].ismarker == 8 ) 
								{
									//  detatch trigger zone / checkpoint here
									t.grideleprof.trigger.waypointzoneindex=0;
									t.gridentitydelete=1;
								}
								else
								{
									//  update for refresh
									t.refreshgrideditcursor=1;
								}

								// if was targetting a parent for link/associate connection (CTRL down)
								// then make this entity a child of the entity targetted
								if ( t.tentityoverdraggingcursor > 0 )
								{
									// parents influence children when they move, and shift children relatively
									t.entityelement[t.e].iHasParentIndex = t.tentityoverdraggingcursor;
								}
								else
								{
									// if still holding CTRL put place entity down on NON-entity, remove parent link/associated status
									if ( t.inputsys.k_s == "l" ) 
									{
										t.entityelement[t.e].iHasParentIndex = 0;
									}
								}

								//  080415 - if NOT holding SHIFT, delete after one placement
								bool bShiftBeingHeldDown = false;
								if (t.inputsys.keyshift != 0)
								{
									if (pref.iEnableDragDropWidgetSelect == 1)
									{
										bool bJustForInitialDragIn = false;
										if (bDraggingActive == false && fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0) bJustForInitialDragIn = true;
										if (bDraggingActive == true && t.gridentityposx_f == 0 && t.gridentityposz_f == 0) bJustForInitialDragIn = true;
										if (bDraggingActiveInitial == true)	bJustForInitialDragIn = true;
										if (bJustForInitialDragIn == true)
										{
											// BUT only apply SHIFT DOWN (duplicate) when carrying/dragging an object, 
											// and NOT when click an object, then hold down SHIFT, then release as this would create a mistaken duplicate
											// and mess up multi selection in the same session!
											bShiftBeingHeldDown = true;
										}
									}
									else
									{
										// a request that if NOT in widget mode, can allow the SHIFT+CLICK to duplicate in the viewport - works nice :)
										bShiftBeingHeldDown = true;
									}
								}
								if (bShiftBeingHeldDown == false && t.gridedit.entityspraymode == 0 )
								{
									t.inputsys.kscancode = 211;
								}
								//PE: Dont allow spraying with markers.
								if (bShiftBeingHeldDown == false && t.gridedit.entityspraymode == 1 && t.entityprofile[t.gridentity].ismarker != 0 )
								{
									t.inputsys.kscancode = 211;
								}

								//PE: If this system rubberband is not dublicated, so we cant do shift. also it react on mouse release not mouse down.
								//PE: So mouse release we have no rubberband selection, if still works fine with single objects.
								if (g.entityrubberbandlist.size() > 0)
								{
									t.inputsys.kscancode = 211;
								}
								t.selstage=1;
							}

							// restore original entity cursor position (after random spray feature)
							t.gridentityposx_f=t.storegridentityposx_f;
							t.gridentityposz_f=t.storegridentityposz_f;
							t.gridentityrotatex_f=t.storegridentityrotatex_f;
							t.gridentityrotatey_f=t.storegridentityrotatey_f;
							t.gridentityrotatez_f=t.storegridentityrotatez_f;
							t.gridentityrotatequatmode = t.storegridentityrotatequatmode;
							t.gridentityrotatequatx_f = t.storegridentityrotatequatx_f;
							t.gridentityrotatequaty_f = t.storegridentityrotatequaty_f;
							t.gridentityrotatequatz_f = t.storegridentityrotatequatz_f;
							t.gridentityrotatequatw_f = t.storegridentityrotatequatw_f;
							t.gridentityscalex_f=t.storegridentityscalex_f;
							t.gridentityscaley_f=t.storegridentityscaley_f;
							t.gridentityscalez_f=t.storegridentityscalez_f;
							
							if (t.gridedit.entityspraymode == 1 && bSprayMoveWithMouse)
							{
								t.gridentityposx_f = t.inputsys.localx_f;
								t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f;
								t.gridentityposz_f = t.inputsys.localy_f;
							}
						}
						else
						{
							// EXTRACT ENTITY FROM MAP
							// Set flag so do not instantly delete entity (below)
							t.onetimeentitypickup=1;

							//  extract entity from the map
							if ( t.tentitytoselect>0 ) 
							{
								if ( t.entityelement[t.tentitytoselect].editorfixed == 0 ) 
								{
									t.gridentityeditorfixed=t.entityelement[t.tentitytoselect].editorfixed;
									t.gridentity=t.entityelement[t.tentitytoselect].bankindex;
									t.ttrygridentitystaticmode=t.entityelement[t.tentitytoselect].staticflag;
									t.ttrygridentity=t.gridentity; editor_validatestaticmode ( );
									t.gridedit.autoflatten=t.entityprofile[t.gridentity].autoflatten;
									t.gridedit.entityspraymode=0;
									if ( t.gridentityautofind == 7 ) 
									{
										//  widget extracts without forcing entity to Floor
										t.gridentityautofind=0;
										t.gridentityposoffground=1;
										t.gridentityusingsoftauto=0;
									}
									else
									{
										t.gridentityposoffground=0;
										t.gridentityusingsoftauto=1;
										// MAX handles its own positioning system
										{
											t.gridentityautofind=0;
										}
									}
									t.gridentitysurfacesnap=0; // surfacesnap off as messes up extract offset for entity
									t.gridentityextractedindex = t.tentitytoselect;
									t.gridentityhasparent = 0;//t.entityelement[t.tentitytoselect].iHasParentIndex; 210317 - break association when extract so can place free of parent
									t.gridentityposx_f=t.entityelement[t.tentitytoselect].x;
									t.gridentityposy_f=t.entityelement[t.tentitytoselect].y;
									t.gridentityposz_f=t.entityelement[t.tentitytoselect].z;
									t.gridentityrotatex_f=t.entityelement[t.tentitytoselect].rx;
									t.gridentityrotatey_f=t.entityelement[t.tentitytoselect].ry;
									t.gridentityrotatez_f=t.entityelement[t.tentitytoselect].rz;
									t.gridentityrotatequatmode = t.entityelement[t.tentitytoselect].quatmode;
									t.gridentityrotatequatx_f = t.entityelement[t.tentitytoselect].quatx;
									t.gridentityrotatequaty_f = t.entityelement[t.tentitytoselect].quaty;
									t.gridentityrotatequatz_f = t.entityelement[t.tentitytoselect].quatz;
									t.gridentityrotatequatw_f = t.entityelement[t.tentitytoselect].quatw;
									if (t.entityprofile[t.gridentity].ismarker == 10)
									{
										t.gridentityscalex_f = 100.0f + t.entityelement[t.tentitytoselect].scalex;
										t.gridentityscaley_f = 100.0f + t.entityelement[t.tentitytoselect].scaley;
										t.gridentityscalez_f = 100.0f + t.entityelement[t.tentitytoselect].scalez;
									}
									else
									{
										t.gridentityscalex_f = ObjectScaleX(t.entityelement[t.tentitytoselect].obj);
										t.gridentityscaley_f = ObjectScaleY(t.entityelement[t.tentitytoselect].obj);
										t.gridentityscalez_f = ObjectScaleZ(t.entityelement[t.tentitytoselect].obj);
									}
									t.grideleprof = t.entityelement[t.tentitytoselect].eleprof;
									t.grideleproflastname_s=t.grideleprof.name_s;

									//  Transfer any waypoint association
									t.waypointindex=t.entityelement[t.tentitytoselect].eleprof.trigger.waypointzoneindex;
									t.grideleprof.trigger.waypointzoneindex=t.waypointindex;
									t.waypoint[t.waypointindex].linkedtoentityindex=0;

									//  delete from map (checks grideleprof.trigger.waypointzoneindex too)
									gridedit_deleteentityfrommap ( );
									t.refreshgrideditcursor=1;

									// remove entity index from rubber band selection
									for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
										if ( g.entityrubberbandlist[i].e == t.tentitytoselect )
											g.entityrubberbandlist[i].e = 0;

									//  Ensure grab GetPoint (  does not move entity! )
									t.inputsys.dragoffsetx_f=t.entityelement[t.tentitytoselect].x-t.inputsys.originallocalx_f;
									t.inputsys.dragoffsety_f=t.entityelement[t.tentitytoselect].z-t.inputsys.originallocaly_f;
								}
							}
						}
						t.selstage=1;
					}

					//  If EXTRACT button clicked initially, first reposition mouse before operation and to get mouse coord correct
					if ( t.widget.duplicatebuttonselected == 1 && t.gridentity == 0 ) 
					{
						//  work out screen/mouse position from real-world XZ coordinate
						if (  t.inputsys.picksystemused == 0 ) 
						{
							//  only if not using pick stsrem (no Floor (  to target) )
							SetCurrentCamera ( 0 );

							//  fix camera range for correct projection matrix
							SetCameraRange ( DEFAULT_NEAR_PLANE, DEFAULT_FAR_PLANE );
							t.screenwidth_f=800.0;
							t.screenheight_f=600.0;
							GetProjectionMatrix (  g.m4_projection );
							GetViewMatrix (  g.m4_view );

							// works in DX9 (D3DXVec4Transform) but not DX11 (KMATRIX)
							SetVector3 ( g.v3_far, ObjectPositionX(t.widget.widgetXYObj), ObjectPositionY(t.widget.widgetXYObj), ObjectPositionZ(t.widget.widgetXYObj) );
							TransformVectorCoordinates3 ( g.v3_far, g.v3_far, g.m4_view );
							t.tx_f=GetXVector3(g.v3_far);
							t.ty_f=GetYVector3(g.v3_far);

							SetVector4 ( g.v4_far, GetXVector3(g.v3_far), GetYVector3(g.v3_far), GetZVector3(g.v3_far), 1 );
							TransformVector4 ( g.v4_far,g.v4_far,g.m4_projection );
							t.tx_f=GetXVector4(g.v4_far);
							t.ty_f=GetYVector4(g.v4_far);
							t.tx_f=t.tx_f/GetWVector4(g.v4_far);
							t.ty_f=t.ty_f/GetWVector4(g.v4_far);

							t.tadjustedtoareax_f=(((t.tx_f+1.0)/2.0)*(GetDisplayWidth()+0.0));
							t.tadjustedtoareay_f=((((t.ty_f*-1)+1.0)/2.0)*(GetDisplayHeight()+0.0));
							t.inputsys.xmouse=t.tadjustedtoareax_f;
							t.inputsys.ymouse=t.tadjustedtoareay_f;
							t.tideframestartx=148 ; t.tideframestarty=96;


							editor_refreshcamerarange ( );
						}

						// trigger actual extraction on next cycle
						t.widget.duplicatebuttonselected = 2;
					}
				}

				HandleObjectDeletion();

				bool bDisableWidgetSelection = false;
				if (bDotObjectDragging)
					bDisableWidgetSelection = true;

				//PE: Disable selections when imgui is in drag mode.
				ImGuiContext& gui = *GImGui;
				if (gui.DragDropActive)
				{
					bDisableWidgetSelection = true;
				}

				// Select widget controlled object
				if (!bDisableWidgetSelection && (t.inputsys.mclick == 1|| iWidgetSelection > 0 ) && t.gridentity == 0)
				{
					if (iWidgetSelection > 0)
					{
						t.tentitytoselect = iWidgetSelection;
					}
					if ( t.tentitytoselect>0 ) 
					{
						//PE: Dont allow changing group when in edit mode.
						if (!group_editing_on && !bRubberBandCreated && t.tentitytoselect != iLastSelectedEntity)
						{
							//Clear rubberband. Allow CTRL to select multiply groups.
							if (t.inputsys.keycontrol == 0) {
								//Only if not in same group.
								int grouplist = isEntityInGroupList(t.tentitytoselect);
								
								if (iLastSelectedEntityGroup != grouplist)
								{
									iLastSelectedEntityGroup = grouplist;
									current_selected_group = grouplist;
									g.entityrubberbandlist.clear();
								}
								else
								{
									if (t.tentitytoselect != iLastSelectedEntity)
									{
										//PE: If we have made a selection not in rubberband, clear rubberband.
										if (g.entityrubberbandlist.size() > 0)
										{
											bool bInRubberBand = false;
											for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
											{
												int ent = g.entityrubberbandlist[i].e;
												if (ent == t.tentitytoselect)
												{
													bInRubberBand = true;
													break;
												}
											}
											if (!bInRubberBand)
											{
												//New selection not in rubberband, clear.
												g.entityrubberbandlist.clear();
											}
										}
									}
								}
							}

							iLastSelectedEntity = t.tentitytoselect;
						}
						if (bRubberBandCreated)
						{
							int grouplist = isEntityInGroupList(t.tentitytoselect);
							iLastSelectedEntityGroup = grouplist;

							if (t.tentitytoselect != iLastSelectedEntity)
							{
								//PE: If we have made a selection not in rubberband, clear rubberband.
								if (g.entityrubberbandlist.size() > 0)
								{
									bool bInRubberBand = false;
									for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
									{
										int ent = g.entityrubberbandlist[i].e;
										if (ent == t.tentitytoselect)
										{
											bInRubberBand = true;
											break;
										}
									}
									if (!bInRubberBand)
									{
										//New selection not in rubberband, clear.
										g.entityrubberbandlist.clear();
									}
								}
								iLastSelectedEntity = t.tentitytoselect;
							}

						}
							
						bRubberBandCreated = false;
						if (pref.iEnableDragDropEntityMode && iWidgetSelection == 0)
						{
							//PE: Disable single click, widget selection, and replace with entity cursor.
							CheckGroupListForRubberbandSelections(t.tentitytoselect);
							AddEntityToCursor(t.tentitytoselect,false);

							//PE: Instant activate it.
							t.widget.pickedEntityIndex = t.tentitytoselect;

							#ifdef ALLOWSELECTINGLOCKEDOBJECTS
							if (!t.entityelement[t.widget.pickedEntityIndex].editorlock)
							{
							#endif
								t.entityelement[t.widget.pickedEntityIndex].editorlock = 0;
								t.widget.pickedObject = 0; //t.entityelement[t.tentitytoselect].obj;

								t.tentitytoselect = 0;
								bReadyToDropEntity = true;
								bWaitOnMouseRelease = true;
								iDragDropActive = 0;
								bDraggingActive = true;

							#ifdef ALLOWSELECTINGLOCKEDOBJECTS
							}
							#endif
						}
						else
						{
							bDraggingActive = false;
							iWidgetSelection = 0;
							if (t.widget.pickedObject == 0)
							{
								//PE: respect markers mode only.
								if (t.gridentitymarkersmodeonly == 0 || (t.gridentitymarkersmodeonly == 1 && t.entityprofile[t.entityelement[t.tentitytoselect].bankindex].ismarker != 0))
								{
									t.widget.pickedEntityIndex = t.tentitytoselect;
									t.widget.pickedObject = t.entityelement[t.tentitytoselect].obj;
									#ifndef ALLOWSELECTINGLOCKEDOBJECTS
									t.entityelement[t.widget.pickedEntityIndex].editorlock = 0;
									#endif
									// makes widget gadgets easier to manipulate
									t.widget.offsetx = g.glastpickedx_f - ObjectPositionX(t.widget.pickedObject);
									t.widget.offsety = g.glastpickedy_f - ObjectPositionY(t.widget.pickedObject);
									t.widget.offsetz = g.glastpickedz_f - ObjectPositionZ(t.widget.pickedObject);

									CheckGroupListForRubberbandSelections(t.tentitytoselect);

									// 271015 - this may not be required as it is duplicated later on..
									if (g.entityrubberbandlist.size() > 0)
										gridedit_moveentityrubberband();
									else
									{
										//MessageBoxA(NULL, "move", "", MB_OK);
									}
								}
							}
						}
					}
				}

				// zoom into entity properties (or EBE EDIT)
				if ( t.widget.propertybuttonselected == 1 ) 
				{
					t.widget.propertybuttonselected = 0;
					if ( t.widget.pickedEntityIndex > 0 ) 
					{
						int entid = t.entityelement[t.widget.pickedEntityIndex].bankindex;
						if ( t.entityprofile[entid].isebe != 0 )
						{
							// EBE entity - begin editing this site
							ebe_newsite ( t.widget.pickedEntityIndex );
						}

						//  End widget control of this object
						t.widget.pickedObject=0;
					}
				}
				if (  t.widget.propertybuttonselected == 2 ) 
				{
					// Entity properties or EBE Save
					t.widget.propertybuttonselected = 0;
					if ( t.widget.pickedEntityIndex > 0 ) 
					{
						int entid = t.entityelement[t.widget.pickedEntityIndex].bankindex;
						if ( t.entityprofile[entid].isebe != 0 )
						{
							// EBE entity - begin editing this site
							if ( ebe_save ( t.widget.pickedEntityIndex ) == 1 )
							{
							}

							// and close widget as Save bit big deal
							widget_switchoff();
						}
						else
						{
							// regular entity
							// prepare zoom-in adjustment vars
							t.tentitytoselect=t.widget.pickedEntityIndex;
							t.e=t.tentitytoselect;

							t.gridentityinzoomview=t.e;
							t.zoomviewtargetx_f=t.entityelement[t.e].x;
							t.zoomviewtargety_f=t.entityelement[t.e].y;
							t.zoomviewtargetz_f=t.entityelement[t.e].z;
							t.zoomviewtargetrx_f=t.entityelement[t.e].rx;
							t.zoomviewtargetry_f=t.entityelement[t.e].ry;
							t.zoomviewtargetrz_f=t.entityelement[t.e].rz;
							gridedit_updatezoomviewvalues ( );

							//  extract entity from the map
							t.gridentityeditorfixed=t.entityelement[t.e].editorfixed;
							t.gridentity=t.entityelement[t.e].bankindex;
							t.ttrygridentitystaticmode=t.entityelement[t.e].staticflag;
							t.ttrygridentity=t.gridentity ; editor_validatestaticmode ( );
							t.gridentityautofind=0;
							t.gridentityposoffground=1;
							t.gridentityusingsoftauto=0;
							t.gridentitysurfacesnap=1-g.gdisablesurfacesnap;
							t.gridentityhasparent=t.entityelement[t.e].iHasParentIndex;
							t.gridentityposx_f=t.entityelement[t.e].x;
							t.gridentityposy_f=t.entityelement[t.e].y;
							t.gridentityposz_f=t.entityelement[t.e].z;
							t.gridentityrotatex_f=t.entityelement[t.e].rx;
							t.gridentityrotatey_f=t.entityelement[t.e].ry;
							t.gridentityrotatez_f=t.entityelement[t.e].rz;
							t.gridentityrotatequatmode = t.entityelement[t.e].quatmode;
							t.gridentityrotatequatx_f = t.entityelement[t.e].quatx;
							t.gridentityrotatequaty_f = t.entityelement[t.e].quaty;
							t.gridentityrotatequatz_f = t.entityelement[t.e].quatz;
							t.gridentityrotatequatw_f = t.entityelement[t.e].quatw;
							if (t.entityprofile[t.gridentity].ismarker == 10)
							{
								t.gridentityscalex_f = 100.0f + t.entityelement[t.e].scalex;
								t.gridentityscaley_f = 100.0f + t.entityelement[t.e].scaley;
								t.gridentityscalez_f = 100.0f + t.entityelement[t.e].scalez;
							}
							else
							{
								t.gridentityscalex_f = ObjectScaleX(t.entityelement[t.e].obj);
								t.gridentityscaley_f = ObjectScaleY(t.entityelement[t.e].obj);
								t.gridentityscalez_f = ObjectScaleZ(t.entityelement[t.e].obj);
							}
							t.grideleprof = t.entityelement[t.e].eleprof;

							//  Transfer any waypoint association
							t.waypointindex=t.entityelement[t.e].eleprof.trigger.waypointzoneindex;
							t.grideleprof.trigger.waypointzoneindex=t.waypointindex;
							t.waypoint[t.waypointindex].linkedtoentityindex=0;

							//  Delete entity from map
							gridedit_deleteentityfrommap ( );
							t.refreshgrideditcursor=1;

							//  simply use its current position (no offset)
							t.inputsys.dragoffsetx_f=0;
							t.inputsys.dragoffsety_f=0;

							//  zoom in to entity for fine detail
							//PE: In wicked we dont actually zoom in so keep cx,cy
							t.inputsys.doautozoomview=1;
							if (t.zoomviewcamerarange_f > 2000.0f)
							{
								//This can fail after test game. if really large set defaults.
								t.zoomviewcamerarange_f = 175.0f;
								t.zoomviewcameraheight_f = 150.0f;
							}
							//  disable icons that interfere with zoom mode
							editor_disableforzoom ( );
	
							HideObject ( t.editor.objectstartindex+5 );
							t.selstage=1;

							//  prepare entity property handler
							#if defined(ENABLEIMGUI) && !defined(USEOLDIDE)
							//PE: Just open the window..
							bEntity_Properties_Window = true;
							#else
							interface_openpropertywindow ( );
							#endif

							//  End widget control of this object
							t.widget.pickedObject=0;
						}
					}
				}

				bool bDisableRubberBandMoving = false;
				if (current_selected_group >= 0 && group_editing_on)
				{
					bDisableRubberBandMoving = true;
				}
				// update rubberband selection connected to primary cursor entity
				if ( !bDisableRubberBandMoving && t.gridentity > 0 && g.entityrubberbandlist.size() > 1 && t.fOldGridEntityX > -99999.0f )
				{
					float fMovedActiveObjectX = t.gridentityposx_f - t.fOldGridEntityX;
					float fMovedActiveObjectY = t.gridentityposy_f - t.fOldGridEntityY;
					float fMovedActiveObjectZ = t.gridentityposz_f - t.fOldGridEntityZ;
					t.gridentityrotatex_f = t.fOldGridEntityRX;
					t.gridentityrotatey_f = t.fOldGridEntityRY;
					t.gridentityrotatez_f = t.fOldGridEntityRZ;
					t.gridentityrotatequatmode = t.fOldGridEntityQuatMode;
					t.gridentityrotatequatx_f = t.fOldGridEntityQuatX;
					t.gridentityrotatequaty_f = t.fOldGridEntityQuatY;
					t.gridentityrotatequatz_f = t.fOldGridEntityQuatZ;
					t.gridentityrotatequatw_f = t.fOldGridEntityQuatW;
					t.tobj = t.gridentityobj;
					if ( t.tobj>0 ) 
					{
						// rotate all selected around t.tobj, the active object
						GGQUATERNION QuatAroundX, QuatAroundY, QuatAroundZ;
						GGQuaternionRotationAxis(&QuatAroundX, &GGVECTOR3(1, 0, 0), GGToRadian(0));
						GGQuaternionRotationAxis(&QuatAroundY, &GGVECTOR3(0, 1, 0), GGToRadian(0));
						GGQuaternionRotationAxis(&QuatAroundZ, &GGVECTOR3(0, 0, 1), GGToRadian(0));
						GGQUATERNION quatRotationEvent = QuatAroundX * QuatAroundY * QuatAroundZ;
						SetStartPositionsForRubberBand(t.tobj);
						RotateAndMoveRubberBand(t.tobj, fMovedActiveObjectX, fMovedActiveObjectY, fMovedActiveObjectZ, quatRotationEvent);
					}
				}
				// record all current offsets from primary cursor entity and rubberband selection
				if (t.gridentity > 0)
				{
					//PE: Dont reset t.fOldGridEntityX when we got a t.gridentity
				}
				else
				{
					//PE: t.gridentityposx_f can go below -1, if you drag it way out there , and you could loose moving of rubberband (out of sync).
					if (!bDisableRubberBandMoving)
					{
						t.fOldGridEntityX = -99999.0f;
						t.fOldGridEntityY = -99999.0f;
						t.fOldGridEntityZ = -99999.0f;
						t.fOldGridEntityRX = -99999.0f;
						t.fOldGridEntityRY = -99999.0f;
						t.fOldGridEntityRZ = -99999.0f;
						t.fOldGridEntityQuatMode = 0;
						t.fOldGridEntityQuatX = 0;
						t.fOldGridEntityQuatY = 0;
						t.fOldGridEntityQuatZ = 0;
						t.fOldGridEntityQuatW = 1;
					}
				}
				if (!bDisableRubberBandMoving && t.gridentity > 0 && t.gridentityobj > 0 && g.entityrubberbandlist.size() > 1 )
				{
					t.fOldGridEntityX = t.gridentityposx_f;
					t.fOldGridEntityY = t.gridentityposy_f;
					t.fOldGridEntityZ = t.gridentityposz_f;
					t.fOldGridEntityRX = t.gridentityrotatex_f;
					t.fOldGridEntityRY = t.gridentityrotatey_f;
					t.fOldGridEntityRZ = t.gridentityrotatez_f;
					t.fOldGridEntityQuatMode = t.gridentityrotatequatmode;
					t.fOldGridEntityQuatX = t.gridentityrotatequatx_f;
					t.fOldGridEntityQuatY = t.gridentityrotatequaty_f;
					t.fOldGridEntityQuatZ = t.gridentityrotatequatz_f;
					t.fOldGridEntityQuatW = t.gridentityrotatequatw_f;
					for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
					{
						int e = g.entityrubberbandlist[i].e;
						GGVECTOR3 VecPos;
						VecPos.x = t.entityelement[e].x - t.gridentityposx_f;
						VecPos.y = t.entityelement[e].y - t.gridentityposy_f;
						VecPos.z = t.entityelement[e].z - t.gridentityposz_f;
						int tobj = t.entityelement[e].obj;
						if ( tobj > 0 )
						{
							float fDet = 0.0f;
							sObject* pObject = GetObjectData(tobj);
							GGMATRIX inverseMatrix = pObject->position.matObjectNoTran;
							GGMatrixInverse ( &inverseMatrix, &fDet, &inverseMatrix );
							GGVec3TransformCoord ( &VecPos, &VecPos, &inverseMatrix );
							g.entityrubberbandlist[i].x = VecPos.x;
							g.entityrubberbandlist[i].y = VecPos.y;
							g.entityrubberbandlist[i].z = VecPos.z;
						}
					}
				}

				//  gridentity delete
				if (  t.gridentitydelete == 1 ) 
				{
					//  Delete any associated waypoint/trigger zone
					t.waypointindex=t.grideleprof.trigger.waypointzoneindex;
					if (  t.waypointindex>0 ) 
					{
						t.w=t.waypoint[t.waypointindex].start;
						waypoint_delete ( );
					}
					t.grideleprof.trigger.waypointzoneindex=0;
					//  delete grid entity object and reset
					t.gridentitydelete=0;
					if (  t.gridentityobj>0 ) 
					{
						DeleteObject (  t.gridentityobj );
						t.gridentityobj=0;
					}
					t.refreshgrideditcursor=1;
					t.gridentity=0;
					t.gridentityposoffground=0;
					t.gridentityusingsoftauto=0;
					t.gridentitysurfacesnap=1-g.gdisablesurfacesnap;
					// MAX handles its own positioning system
					t.gridentityautofind = 0;
					t.inputsys.dragoffsetx_f=0;
					t.inputsys.dragoffsety_f=0;
					editor_refreshentitycursor ( );
					t.widget.pickedObject=0;

					//PE: We dont actualle make new duplicates so can skib this.
					if (1)// same functionality pref.iEnableDragDropEntityMode)
					{
						//We might have to do some kind of support.
					}
					else
					{
						bool bDisableRubberBandMoving = false;
						if (current_selected_group >= 0 && group_editing_on)
						{
							bDisableRubberBandMoving = true;
						}
						if (!bDisableRubberBandMoving)
						{
							// if rubberband selection, delete all in selection
							gridedit_deleteentityrubberbandfrommap();
						}
					}

					// flag also used to restore highlighting behavior
					t.gridentityextractedindex = 0;

					// when place down, ensure waypoint not affected until release mouse button
					t.mclickpressed = 1;
				}
			}

			if (  t.inputsys.mclick == 0 && t.selstage == 1 ) 
			{
				t.selstage=0;
			}
			if (  t.gridedit.entityspraymode == 1 && t.selstage == 1 ) 
			{
				//  entity spray keeps going while button pressed
				t.selstage=0;
			}
		}

		// this is triggered when set to a negative, and continues to force find surface until zero
		if (iObjectMoveModeDropSystem < 0)
		{
			iObjectMoveModeDropSystem++;
			iForceScancode = 13;
		}
	}
}

