void editor_previewmap ( int iUseVRTest )
{
	//  check if we are in the importer or character creator, if we are, don't test ma
	editor_checkIfInSubApp ( );
	//  Set single player test game flags here
	t.game.runasmultiplayer=0;
	editor_previewmapormultiplayer ( iUseVRTest );
}

void editor_previewmap_initcode(int iUseVRTest)
{
	editor_previewmapormultiplayer_initcode ( iUseVRTest );
}

bool editor_previewmap_loopcode(int iUseVRTest)
{
	// loop ended
	return editor_previewmapormultiplayer_loopcode ( iUseVRTest );
}

void editor_previewmap_afterloopcode(int iUseVRTest)
{
	editor_previewmapormultiplayer_afterloopcode ( iUseVRTest );
}

void editor_handlepguppgdn ( void )
{
	// changes and returns 't.tupdownstepvalue_f'
	bool bWidgetMove = false;
	if (t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209)
	{
		float fEntityStepSize = 5.0f;
		if (t.gridentity > 0 && t.gridentityobj > 0)
		{
			if (ObjectExist(t.gridentityobj) == 1)
				fEntityStepSize = ObjectSizeY(t.gridentityobj, 1);
		}
		else
		{
			if (t.widget.pickedEntityIndex > 0)
			{
				int wobj = t.entityelement[t.widget.pickedEntityIndex].obj;
				if (t.widget.activeObject > 0)
					wobj = t.widget.activeObject;

				if (ObjectExist(wobj) == 1)
					fEntityStepSize = ObjectSizeY(wobj, 1);

				//Make sure to highlight all objects the object belong to.
				if (t.widget.pickedEntityIndex > 0)
					CheckGroupListForRubberbandSelections(t.widget.pickedEntityIndex);
				bWidgetMove = true;
			}
		}
		if (t.gridentitygridlock >= 1)
			t.tupdownstepvalue_f = 0.0;
		else
			t.tupdownstepvalue_f = 1.0;
		if (t.gridentitygridlock > 0)
		{
			if (t.inputsys.keypressallowshift == 0 && (t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209))
			{
				if (t.gridentitygridlock == 1)
					t.tupdownstepvalue_f = fEntityStepSize;
				if (t.gridentitygridlock == 2)
					t.tupdownstepvalue_f = pref.fEditorGridSizeY;
				t.inputsys.keypressallowshift = 1;
			}
			else
			{
				t.tupdownstepvalue_f = 0;
			}
		}
		if (bWidgetMove)
		{
			if (t.tupdownstepvalue_f != 0.0f && t.widget.pickedEntityIndex > 0 && t.entityelement[t.widget.pickedEntityIndex].editorlock == 0)
			{
				bool bDisableRubberBandMoving = false;
				if (current_selected_group >= 0 && group_editing_on)
				{
					bDisableRubberBandMoving = true;
				}
				if (!bDisableRubberBandMoving)
				{
					if (t.inputsys.kscancode == 201)  t.entityelement[t.widget.pickedEntityIndex].y += t.tupdownstepvalue_f;
					if (t.inputsys.kscancode == 209)  t.entityelement[t.widget.pickedEntityIndex].y -= t.tupdownstepvalue_f;
					int wobj = t.entityelement[t.widget.pickedEntityIndex].obj;
					if (t.widget.activeObject > 0)
						wobj = t.widget.activeObject;

					PositionObject(wobj, t.entityelement[t.widget.pickedEntityIndex].x, t.entityelement[t.widget.pickedEntityIndex].y, t.entityelement[t.widget.pickedEntityIndex].z);

					// if we need to also move rubber band highlighted objects, do so now
					if (g.entityrubberbandlist.size() > 0)
					{
						for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
						{
							int e = g.entityrubberbandlist[i].e;
							int tobj = t.entityelement[e].obj;
							if (e != t.widget.pickedEntityIndex && tobj > 0 && t.entityelement[e].editorlock == 0)
							{
								if (ObjectExist(tobj) == 1)
								{
									if (tobj != wobj)
									{
										if (t.inputsys.kscancode == 201)  t.entityelement[e].y += t.tupdownstepvalue_f;
										if (t.inputsys.kscancode == 209)  t.entityelement[e].y -= t.tupdownstepvalue_f;
										if (t.inputsys.kscancode == 201) g.entityrubberbandlist[i].x += t.tupdownstepvalue_f;
										if (t.inputsys.kscancode == 209) g.entityrubberbandlist[i].x -= t.tupdownstepvalue_f;

										PositionObject(tobj, t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z);
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
		}
	}
}

int last_xmousemove = 0, last_ymousemove = 0;
void imgui_input_getcontrols(void)
{
	// when setup.ini sets fulldebugview to 1, record all key states pressed
	if (g.globals.fulldebugviewofkeymap == 1)
	{
		int iRawKeyState = ScanCode();
		if (iRawKeyState > 0)
		{
			static int lastinputsyskscancode = 0;
			if (iRawKeyState != lastinputsyskscancode)
			{
				char pKeyMapDebugLog[256];
				sprintf(pKeyMapDebugLog, "Raw Key State: %d", iRawKeyState);
				timestampactivity(0, pKeyMapDebugLog);
			}
			lastinputsyskscancode = iRawKeyState;
		}
	}

	//  Some actions are directly triggered by input subroutine
	t.inputsys.doload = 0;
	t.inputsys.domodeterrain = 0;
	t.inputsys.domodeentity = 0;
	t.inputsys.domodemarker = 0;
	t.inputsys.domodewaypoint = 0;
	t.inputsys.doundo = 0;
	t.inputsys.doredo = 0;
	t.inputsys.tselcontrol = 0;
	t.inputsys.tselcut = 0;
	t.inputsys.tselcopy = 0;
	t.inputsys.tseldelete = 0;

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	//PE: Take everything from imgui.
	float itmpmousex = ImGui::GetMousePos().x;
	float itmpmousey = ImGui::GetMousePos().y;
	int iSecureZone = 4;
	RECT winpos = { 0,0,0,0 };

	//PE: Must be relative to windows pos, or nothing work if you have a window placed at the rigth of the screen.
	GetWindowRect(g_pGlob->hWnd, &winpos);

	bool bCanGetInput = bImGuiRenderTargetFocus;
	if (pref.iEnableDragDropEntityMode && bDraggingActive)
	{
		bCanGetInput = true;
	}

	if (bCanGetInput && (itmpmousex + winpos.left) >= (renderTargetAreaPos.x+iSecureZone) && (itmpmousey+winpos.top) >= (renderTargetAreaPos.y + iSecureZone) && (itmpmousex - winpos.left) <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && (itmpmousey-winpos.top) <= renderTargetAreaPos.y + 30 + (renderTargetAreaSize.y - iSecureZone ))
	{
		t.inputsys.activemouse = 1;
		t.inputsys.xmouse = (int)itmpmousex;
		t.inputsys.ymouse = (int)itmpmousey;
		t.inputsys.zmouse = io.MouseWheel; // MouseZ();
		t.inputsys.xmousemove = t.inputsys.xmouse - xmouseold;
		t.inputsys.ymousemove = t.inputsys.ymouse - ymouseold;

		last_xmousemove = t.inputsys.xmousemove;
		last_ymousemove = t.inputsys.ymousemove;

		xmouseold = t.inputsys.xmouse;
		ymouseold = t.inputsys.ymouse;

		t.inputsys.wheelmousemove = io.MouseWheel; //MouseMoveZ();
		if (ImGui::IsMouseDown(2)) t.inputsys.wheelmousemove = 0;
		set_inputsys_mclick(io.MouseDown[0] + (io.MouseDown[1] * 2.0) + (io.MouseDown[2] * 3.0) + (io.MouseDown[3] * 4.0));// t.inputsys.mclick = io.MouseDown[0] + (io.MouseDown[1] * 2.0) + (io.MouseDown[2] * 3.0) + (io.MouseDown[3] * 4.0); //  MouseClick();
		t.inputsys.k_s = Lower(Inkey());

		//  Control keys direct from keyboard
		t.inputsys.keyreturn = io.KeysDown[13]; // ReturnKey();
		t.inputsys.keyshift = io.KeyShift;
		t.inputsys.keytab = io.KeysDown[0x09]; //TAB!
		t.inputsys.keyleft = io.KeysDown[37]; // LeftKey();
		t.inputsys.keyright = io.KeysDown[39]; //RightKey();
		t.inputsys.keyup = io.KeysDown[38]; //UpKey();
		t.inputsys.keydown = io.KeysDown[40]; // DownKey();
		t.inputsys.keycontrol = io.KeyCtrl; //ControlKey();
		t.inputsys.keyspace = io.KeysDown[32]; //SpaceKey();

		//PE: We need raw scancodes. just take it from imgui.
		t.inputsys.kscancode = 0;
		for (int iTemp = 0; iTemp < 256; iTemp++)
		{
			if (iTemp != 16 && iTemp != 17 && iTemp != 18) 
			{ 
				//shift,control (added 18, not sure what it is)
				if (io.KeysDown[iTemp] > 0)
				{
					t.inputsys.kscancode = iTemp;
					break;
				}
			}
		}
	}
	else 
	{
		//No input to DX11.
		t.inputsys.activemouse = 1;
		t.inputsys.zmouse = 0;

		float fMouseDeltaX, fMouseDeltaZ;
		WickedCall_GetMouseDeltas ( &fMouseDeltaX, &fMouseDeltaZ);
		t.inputsys.xmousemove = fMouseDeltaX;
		t.inputsys.ymousemove = fMouseDeltaZ;

		xmouseold = t.inputsys.xmouse;
		ymouseold = t.inputsys.ymouse;

		t.inputsys.wheelmousemove = 0;

		set_inputsys_mclick(0);// t.inputsys.mclick = 0;

		t.inputsys.k_s = "";

		//  Control keys direct from keyboard
		t.inputsys.keyreturn = 0;
		t.inputsys.keyshift = 0;
		t.inputsys.keytab = 0;
		t.inputsys.keyleft = 0;
		t.inputsys.keyright = 0;
		t.inputsys.keyup = 0;
		t.inputsys.keydown = 0;
		t.inputsys.keycontrol = 0;
		t.inputsys.keyspace = 0;
		t.inputsys.kscancode = 0;
	}

	int mcursor = ImGui::GetMouseCursor();

	if (bBuilder_Properties_Window) 
	{
		//Disable some keys.
	}

	if (g_bCharacterCreatorPlusActivated) 
	{
		//Disable some keys.
		if( t.inputsys.kscancode == Asc("t") || t.inputsys.kscancode == Asc("T") )
			t.inputsys.kscancode = 0;
		if( t.inputsys.k_s == "t" || t.inputsys.k_s == "T" ) 
			t.inputsys.k_s = "";
		if (t.inputsys.kscancode == Asc("p") || t.inputsys.kscancode == Asc("P"))
			t.inputsys.kscancode = 0;
		if (t.inputsys.k_s == "p" || t.inputsys.k_s == "P")
			t.inputsys.k_s = "";

	}
	if (bEntity_Properties_Window) 
	{
		//Disable all key input when in Properties.
		t.inputsys.keyleft = 0;
		t.inputsys.keyright = 0;
		t.inputsys.keyup = 0;
		t.inputsys.keydown = 0;
		t.inputsys.kscancode = 0;
		t.inputsys.k_s = "";
		t.inputsys.keyshift = 0;
		t.inputsys.keytab = 0;
	}

	if (ImGui::IsAnyItemActive() && t.inputsys.mclick != 1 ) 
	{
		//A widget got focus, like textinput , disable all keys.
		t.inputsys.keyleft = 0;
		t.inputsys.keyright = 0;
		t.inputsys.keyup = 0;
		t.inputsys.keydown = 0;
		t.inputsys.kscancode = 0;
		t.inputsys.k_s = "";
		t.inputsys.keyshift = 0;
		t.inputsys.keytab = 0;
	}

	// 060320 - somehow, some laptops set 'bImGuiGotFocus' to true (or mcursor>0), 
	// wiping out click detection in Welcome screen, so added condition to prevent this erasure!
	if ( bImGuiGotFocus || ( mcursor > 0 && mcursor != ImGuiMouseCursor_Hand) )
	{
		//No GG input when using imgui.
		t.inputsys.xmouse = 500000;
		t.inputsys.ymouse = 0;
		t.inputsys.xmousemove = 0;
		t.inputsys.ymousemove = 0;
		set_inputsys_mclick(0);// t.inputsys.mclick = 0;
		t.inputsys.zmouse = 0;
		t.inputsys.wheelmousemove = 0;
		t.inputsys.activemouse = 0;
		t.syncthreetimes = 1;
		t.inputsys.k_s = "";
		//  Control keys direct from keyboard
		t.inputsys.keyreturn = 0;
		t.inputsys.keyshift = 0;
		t.inputsys.keytab = 0;
		t.inputsys.keyleft = 0;
		t.inputsys.keyright = 0;
		t.inputsys.keyup = 0;
		t.inputsys.keydown = 0;
		t.inputsys.keycontrol = 0;
		t.inputsys.keyspace = 0;
		t.inputsys.kscancode = 0;

	}

	input_extramappings();

	//  Flag reset
	t.inputsys.dorotation = 0;
	t.inputsys.domirror = 0;
	t.inputsys.doflip = 0;
	t.inputsys.doentityrotate = 0;
	t.inputsys.dozoomin = 0;
	t.inputsys.dozoomout = 0;
	t.inputsys.doscrollleft = 0;
	t.inputsys.doscrollright = 0;
	t.inputsys.doscrollup = 0;
	t.inputsys.doscrolldown = 0;
	t.inputsys.domapresize = 0;
	t.inputsys.dogroundmode = -1;
	t.inputsys.dozoomview = 0;
	t.inputsys.dozoomviewmovex = 0;
	t.inputsys.dozoomviewmovey = 0;
	t.inputsys.dozoomviewmovez = 0;
	t.inputsys.dozoomviewrotatex = 0;
	t.inputsys.dozoomviewrotatey = 0;
	t.inputsys.dozoomviewrotatez = 0;
	t.inputsys.dosinglelayer = 0;
	t.inputsys.tselfloor = 0;
	t.inputsys.tselpaste = 0;
	t.inputsys.tselwipe = 0;
	t.inputsys.dosaveandrun = 0;

	//PE: Map additional keys.
	if (t.inputsys.kscancode == 32)  t.inputsys.keyspace = 1; else t.inputsys.keyspace = 0;
	if (t.inputsys.kscancode == 0 && t.inputsys.keyshift == 0) t.inputsys.keypressallowshift = 0;

	//  W,A,S,D in editor for scrolling about (easier for user)
	if (t.inputsys.kscancode == 87)
		t.inputsys.keyup = 1;
	if (t.inputsys.kscancode == 65)  t.inputsys.keyleft = 1;
	if (t.inputsys.kscancode == 83)  t.inputsys.keydown = 1;
	if (t.inputsys.kscancode == 68)  t.inputsys.keyright = 1;

	if (t.inputsys.keycontrol == 1)
	{
		if (t.inputsys.k_s == "z" && t.inputsys.undokeypress == 0) 
		{
			bForceUndo = true;
			//t.inputsys.doundo = 1; 
			//t.inputsys.undokeypress = 1;
		}
		else if (t.inputsys.k_s == "y" && t.inputsys.undokeypress == 0) 
		{
			t.inputsys.doredo = 1; 
			t.inputsys.undokeypress = 1;
		}
	}

	//  Convert to DX INPUT CODES
	t.t_s = ""; t.tt = 0;
	switch (t.inputsys.kscancode)
	{
		case 9: t.tt = 15; break;
		case 32: t.tt = 57; break;
		case 33: t.tt = 201; break;
		case 34: t.tt = 209; break;
		case 37: t.tt = 203; break;
		case 38: t.tt = 200; break;
		case 39: t.tt = 205; break;
		case 40: t.tt = 208; break;
		case 42: t.tt = 16; break;
		case 46: t.tt = 211; break;
		case 54: t.tt = 16; break;
		case 112: t.tt = 59; break;
		case 113: t.tt = 60; break;
		case 114: t.tt = 61; break;
		case 115: t.tt = 62; break;
		case 123: t.tt = 88; break;
		case 187: t.tt = 13; break;
		case 188: t.tt = 51; break;
		case 189: t.tt = 12; break;
		case 190: t.tt = 52; break;
		case 192: t.tt = 40; break;
		case 219: t.tt = 26; break;
		case 220: t.tt = 86; break;
		case 221: t.tt = 27; break;
		case 222: t.tt = 43; break;
		case 1001: t.tt = 13; break;
		case 1002: t.tt = 12; break;
	}
	// 031215 - then remap to new scancodes (from keymap)
	t.tt = g.keymap[t.tt];
	// and temp back into IDE key values (for last bit)
	int ttt = 0;
	switch (t.tt)
	{
		case 15: ttt = 9; break;
		case 57: ttt = 32; break;
		case 201: ttt = 33; break;
		case 209: ttt = 34; break;
		case 203: ttt = 37; break;
		case 200: ttt = 38; break;
		case 205: ttt = 39; break;
		case 208: ttt = 40; break;
		case 16: ttt = 42; break;
		case 211: ttt = 46; break;
		case 59: ttt = 112; break;
		case 60: ttt = 113; break;
		case 61: ttt = 114; break;
		case 62: ttt = 115; break;
		case 88: ttt = 123; break;
		case 13: ttt = 187; break;
		case 51: ttt = 188; break;
		case 12: ttt = 189; break;
		case 52: ttt = 190; break;
		case 40: ttt = 192; break;
		case 26: ttt = 219; break;
		case 86: ttt = 220; break;
		case 27: ttt = 221; break;
		case 43: ttt = 222; break;
	}
	// then create proper inkey chars from revised (if any) scancodes
	switch (ttt)
	{
		case 16: t.t_s = "q"; break;
		case 57: t.t_s = " "; break;
		case 107: t.t_s = "="; break;
		case 109: t.t_s = "-"; break;
		case 187: t.t_s = "="; break;
		case 188: t.t_s = ","; break;
		case 189: t.t_s = "-"; break;
		case 190: t.t_s = "."; break;
		case 192: t.t_s = "'"; break;
		case 219: t.t_s = "["; break;
		case 220: t.t_s = "\\"; break;
		case 221: t.t_s = "]"; break;
		case 222: t.t_s = "#"; break;
	}

	if (t.inputsys.kscancode >= Asc("A") && t.inputsys.kscancode <= Asc("Z"))  t.t_s = Lower(Chr(t.inputsys.kscancode));
	if (t.inputsys.kscancode >= Asc("0") && t.inputsys.kscancode <= Asc("9"))  t.t_s = Lower(Chr(t.inputsys.kscancode));
	if (t.t_s != "")  t.tt = 1;

	t.inputsys.k_s = t.t_s; t.inputsys.kscancode = t.tt;

	//  Input conditional flags
	if (t.inputsys.kscancode == 0) 
	{
		t.inputsys.keypress = 0;
		if (iForceScancode > 0) 
		{
			if (iForceScancode == 13)
				t.inputsys.keyreturn = 1;
			t.inputsys.kscancode = iForceScancode;
			iForceScancode = -1;
		}
		else if (bForceKey) 
		{
			bForceKey = false;
			t.inputsys.k_s = csForceKey;
			t.inputsys.keycontrol = 0;
			t.inputsys.keyshift = 0;
			t.inputsys.keytab = 0;
			t.inputsys.kscancode = Asc(csForceKey.Get());
		}
		else if (bForceKey2) 
		{
			bForceKey2 = false;
			t.inputsys.k_s = csForceKey2;
			t.inputsys.keycontrol = 0;
			t.inputsys.keyshift = 0;
			t.inputsys.keytab = 0;
			t.inputsys.kscancode = Asc(csForceKey2.Get());
		}
	}
	if (bForceUndo) 
	{
		t.inputsys.doundo = 1;
		t.inputsys.undokeypress = 1;
		bForceUndo = false;
	}
	if (bForceRedo) 
	{
		t.inputsys.doredo = 1;
		t.inputsys.undokeypress = 1;
		bForceRedo = false;
	}

	//  Construction Keys
	if (t.inputsys.keycontrol == 0)
	{
		// can get marker mode from anywhere
		if ((t.inputsys.kscancode == Asc("M") || t.inputsys.k_s == "m") && t.inputsys.keypress == 0)
		{
			t.inputsys.domodemarker = 1;
			t.inputsys.keypress = 1;
		}

		if ((t.grideditselect == 4 && t.gridentityinzoomview>0) || t.grideditselect == 5)
		{
			if (t.inputsys.k_s == "g" && t.inputsys.keypress == 0)
			{
				t.inputsys.keypress = 1; 
				t.gridentitygridlock = t.gridentitygridlock + 1;
				if (t.gridentitygridlock > 2)
				{
					t.gridentitygridlock = 0;
				}
				pref.iGridMode = t.gridentitygridlock;
			}
			if (t.inputsys.k_s == "y" && t.inputsys.keypress == 0 && g.gentitytogglingoff == 0)
			{
				// only if not EBE
				if (t.entityprofile[t.gridentity].isebe == 0)
				{
					//PE: This dont work, t.gridentity = 0 ?
					t.ttrygridentitystaticmode = 1 - t.gridentitystaticmode;
					t.ttrygridentity = t.gridentity; editor_validatestaticmode();
				}
				t.inputsys.keypress = 1;
			}
			if (t.inputsys.k_s == "u" && t.inputsys.keypress == 0)
			{
				//  control auto-flatten
				t.inputsys.keypress = 1;
				t.gridedit.autoflatten = 1 - t.gridedit.autoflatten;
			}
			if (t.inputsys.k_s == "i" && t.inputsys.keypress == 0)
			{
				//  control entity spray mode
				t.inputsys.keypress = 1;
				t.gridedit.entityspraymode = 1 - t.gridedit.entityspraymode;
			}
			// except when in EBE mode which handles - and + keys for material changing
			if (t.ebe.on == 0)
			{
				if (t.inputsys.k_s == "-" && t.inputsys.keypress == 0) { t.gridentitymodifyelement = 1; t.inputsys.keypress = 1; }
				if (t.inputsys.k_s == "=" && t.inputsys.keypress == 0) { t.gridentitymodifyelement = 2; t.inputsys.keypress = 1; }
			}
		}

		//  editing mode
		if (t.inputsys.k_s == "t")
		{
			if (!pref.iEnableSingleRightPanelAdvanced)
			{
				Logic_Settings_Window = false;
				Game_Settings_Window = false;
				Weather_Tools_Window = false;
				Visuals_Tools_Window = false;
				//LB: shooter now a filter mode Shooter_Tools_Window = false;
				Entity_Tools_Window = false;
				bWaypoint_Window = false;
				iRestoreLastWindow = 0;
			}
			bTerrain_Tools_Window = true;
			t.inputsys.domodeterrain = 1; t.inputsys.dowaypointview = 0;
			bTerrain_Tools_Window = true;
			t.terrain.terrainpaintermode = 1;
		}
		if (t.inputsys.k_s == "v")
		{
		}
		if (t.inputsys.k_s == "o")
		{
			if (!pref.iEnableSingleRightPanelAdvanced)
			{
				Game_Settings_Window = false;
				Weather_Tools_Window = false;
				Visuals_Tools_Window = false;
				//LB: shooter now a filter mode Shooter_Tools_Window = false;
				bTerrain_Tools_Window = false;
				bWaypoint_Window = false;
				iRestoreLastWindow = 0;
			}
			Entity_Tools_Window = true;
			t.inputsys.domodeentity = 1; t.inputsys.dowaypointview = 0;
		}
		if (t.inputsys.keyspace == 1 && t.inputsys.keypress == 0) { t.inputsys.dowaypointview = 1 - t.inputsys.dowaypointview; t.inputsys.keypress = 1; t.lastgrideditselect = -1; editor_refresheditmarkers(); }

		//  NUM-ROTATE CONTROLS
		if (t.inputsys.k_s == "r" && t.inputsys.keypress == 0 && t.ebe.on == 0 )
		{
			t.inputsys.dorotation = 1; t.inputsys.keypress = 1;
		}
		if (t.grideditselect != 4 && t.grideditselect != 0)
		{
			if (pref.iEnableAxisRotationShortcuts == 1)
			{
				if (t.inputsys.k_s == "1" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 1; t.inputsys.keypress = 1; }
				if (t.inputsys.k_s == "2" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 2; t.inputsys.keypress = 1; }
				if (t.inputsys.k_s == "3" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 3; t.inputsys.keypress = 1; }
				if (t.inputsys.k_s == "4" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 4; t.inputsys.keypress = 1; }
				if (t.inputsys.k_s == "5" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 5; t.inputsys.keypress = 1; }
				if (t.inputsys.k_s == "6" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 6; t.inputsys.keypress = 1; }
				if (t.inputsys.keyshift == 0)
				{
					if (t.inputsys.k_s == "0" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 98; t.inputsys.keypress = 1; }
				}
				else
				{
					if (t.inputsys.k_s == "0" && t.inputsys.keypress == 0) { t.inputsys.doentityrotate = 99; t.inputsys.keypress = 1; }
				}
			}
		}

		//  Editing of Map
		if (t.inputsys.k_s == ",")  t.inputsys.dozoomin = 1;
		if (t.inputsys.k_s == ".")  t.inputsys.dozoomout = 1;

		//  TAB Key causes layer edit view control
		if (t.inputsys.kscancode == 15 && t.inputsys.keypress == 0) { t.inputsys.dosinglelayer = 1; t.inputsys.keypress = 1; }

		static bool bF1Released = false;
		if (t.inputsys.kscancode != 59)
			bF1Released = true;

		// F1 to toggle widget mode or smart positioning mode.
		if (t.inputsys.kscancode == 59 && bF1Released)
		{
			bF1Released = false;
			pref.iEnableDragDropWidgetSelect = !pref.iEnableDragDropWidgetSelect;
			if (pref.iEnableDragDropWidgetSelect)
				widget_show_widget();
			else
				widget_hide();
		}

		// F2-F4 in editor to control widget mode or smart positioning mode.
		if (t.inputsys.kscancode >= 60 && t.inputsys.kscancode <= 62)
		{
			t.toldmode = t.widget.mode;
			bool bWidgetEnabled = pref.iEnableDragDropWidgetSelect;
			if (t.inputsys.kscancode == 60) 
			{
				if (bWidgetEnabled)
					t.widget.mode = 0;
				else
					iObjectMoveMode = 2;

			}
			if (t.inputsys.kscancode == 61) 
			{
				if (bWidgetEnabled)
					t.widget.mode = 1;
				else
					iObjectMoveMode = 0;
			}
			if (t.inputsys.kscancode == 62) 
			{
				if (bWidgetEnabled)
				{
					// Don't allow characters and markers to be scaled with the widget
					int entid = t.entityelement[t.widget.pickedEntityIndex].bankindex;
					if (entid > 0)
					{
						bool bAllowObjectsAndParticlesToScale = false;
						if (t.entityprofile[entid].ismarker == 0) bAllowObjectsAndParticlesToScale = true;
						if (t.entityprofile[entid].ismarker == 10) bAllowObjectsAndParticlesToScale = true;
						if (t.entityprofile[entid].ischaracter == 0 && bAllowObjectsAndParticlesToScale==true)
						{
							t.widget.mode = 2;
						}
					}
				}
				else
				{
					iObjectMoveMode = 1;
				}
			}
			
			if ( t.toldmode != t.widget.mode ) widget_show_widget ( );
		}
	}
	else
	{
		if (t.inputsys.k_s == "r" && t.ebe.on == 0)  t.inputsys.dorotation = 1;
	}

	//  Key Map Scroll and Resize
	if (t.inputsys.keyshift == 0)
	{
		if (t.inputsys.keyleft == 1)  t.inputsys.doscrollleft = 3;
		if (t.inputsys.keyright == 1)  t.inputsys.doscrollright = 3;
		if (t.inputsys.keyup == 1) 
			t.inputsys.doscrollup = 3;
		if (t.inputsys.keydown == 1)  t.inputsys.doscrolldown = 3;
	}
	else
	{
		if (t.inputsys.keyleft == 1)  t.inputsys.doscrollleft = 20;
		if (t.inputsys.keyright == 1)  t.inputsys.doscrollright = 20;
		if (t.inputsys.keyup == 1)
			t.inputsys.doscrollup = 20;
		if (t.inputsys.keydown == 1)  t.inputsys.doscrolldown = 20;
	}

	//  Mouse Wheel control (170616 - but not when in EBE mode as its used for grid layer control)
	if (t.ebe.on == 0)
	{
		if (t.grideditselect == 4)
		{
			//  Zoomed in View
			t.zoomviewcamerarange_f -= (t.inputsys.wheelmousemove / 10.0);
		}
		else
		{
			//  Non-Zoomed in View
			if (t.inputsys.keycontrol == 0)
			{
				if (t.inputsys.wheelmousemove<0)
					t.inputsys.dozoomout = 1;
				if (t.inputsys.wheelmousemove>0)
					t.inputsys.dozoomin = 1;
			}
		}
	}

	//  UndoRedo Keys
	if (t.inputsys.keycontrol == 1)
	{
		if (t.inputsys.k_s == "z" && t.inputsys.undokeypress == 0) 
		{ 
			t.inputsys.doundo = 1; t.inputsys.undokeypress = 1; 
		}
		if (t.inputsys.k_s == "y" && t.inputsys.undokeypress == 0) { t.inputsys.doredo = 1; t.inputsys.undokeypress = 1; }
	}

	//  Controls only when in zoomview
	if (t.grideditselect == 4)
	{
		//  orient arrowkey movement to camera angle
		t.tca_f = WrapValue(CameraAngleY());
		if (t.tca_f >= 360 - 45 || t.tca_f <= 45)
		{
			t.txa = 1; t.txb = 2; t.txc = 0; t.txd = 0;
			t.tza = 0; t.tzb = 0; t.tzc = 2; t.tzd = 1;
		}
		else
		{
			if (t.tca_f >= 180 - 45 && t.tca_f <= 180 + 45)
			{
				t.txa = 2; t.txb = 1; t.txc = 0; t.txd = 0;
				t.tza = 0; t.tzb = 0; t.tzc = 1; t.tzd = 2;
			}
			else
			{
				if (t.tca_f <= 180)
				{
					t.txa = 0; t.txb = 0; t.txc = 2; t.txd = 1;
					t.tza = 2; t.tzb = 1; t.tzc = 0; t.tzd = 0;
				}
				else
				{
					t.txa = 0; t.txb = 0; t.txc = 1; t.txd = 2;
					t.tza = 1; t.tzb = 2; t.tzc = 0; t.tzd = 0;
				}
			}
		}
		t.inputsys.dozoomviewmovex = 0; t.inputsys.dozoomviewmovez = 0;
		if (t.inputsys.keyleft == 1) { t.inputsys.dozoomviewmovex += t.txa; t.inputsys.dozoomviewmovez += t.tza; }
		if (t.inputsys.keyright == 1) { t.inputsys.dozoomviewmovex += t.txb; t.inputsys.dozoomviewmovez += t.tzb; }
		if (t.inputsys.keyup == 1)
		{
			t.inputsys.dozoomviewmovex += t.txc;
			t.inputsys.dozoomviewmovez += t.tzc;
		}
		if (t.inputsys.keydown == 1) { t.inputsys.dozoomviewmovex += t.txd; t.inputsys.dozoomviewmovez += t.tzd; }
		//  control rotation
		if (t.inputsys.k_s == "1" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatex = 1; t.inputsys.keypress = 1; }
		if (t.inputsys.k_s == "2" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatex = 2; t.inputsys.keypress = 1; }
		if (t.inputsys.k_s == "3" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatey = 1; t.inputsys.keypress = 1; }
		if (t.inputsys.k_s == "4" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatey = 2; t.inputsys.keypress = 1; }
		if (t.inputsys.k_s == "5" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatez = 1; t.inputsys.keypress = 1; }
		if (t.inputsys.k_s == "6" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatez = 2; t.inputsys.keypress = 1; }
		if (t.inputsys.keyshift == 0)
		{
			if (t.inputsys.k_s == "0" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatex = 98; t.inputsys.keypress = 1; }
		}
		else
		{
			if (t.inputsys.k_s == "0" && t.inputsys.keypress == 0) { t.inputsys.dozoomviewrotatex = 99; t.inputsys.keypress = 1; }
		}
	}
	if (t.grideditselect == 4 || t.grideditselect == 5)
	{
		//  control finder (toggled using gridentityautofind value)
		// Simpler RETURN system
		t.gridentitydroptoground = 0;
		if (t.inputsys.keyreturn == 1)
		{
			t.gridentityautofind = 0;
			t.gridentityusingsoftauto = 0;
			t.gridentitysurfacesnap = 0;
			if (iObjectMoveModeDropSystemUsing == 1 && g_bHoldGridEntityPosWhenManaged == false)
			{
				if (t.gridentity > 0)
				{
					t.gridentitydroptoground = 1 + t.entityprofile[t.gridentity].forwardfacing;
				}
			}
		}
		//  control height
		if (t.grideditselect == 4)
		{
			//  move entity through zoomview system
			if (t.inputsys.kscancode == 201) { t.inputsys.dozoomviewmovey = 2; t.gridentityposoffground = 1; t.gridentityautofind = 0; t.gridentityusingsoftauto = 0; }
			if (t.inputsys.kscancode == 209) { t.inputsys.dozoomviewmovey = 1; t.gridentityposoffground = 1; t.gridentityautofind = 0; t.gridentityusingsoftauto = 0; }
		}
		else
		{
			//  directly move entity (and detatch from terrain) PGUP and PGDN
			if (t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209)
			{
				editor_handlepguppgdn();
				t.gridentityposoffground = 1; t.gridentityautofind = 0; t.gridentityusingsoftauto = 0; t.gridentitysurfacesnap = 0;
			}
		}
	}

	//  Create a waypoint when instructed to
	if (t.inputsys.domodewaypointcreate == 1 && t.inputsys.keypress == 0)
	{
		//In freeflight mode t.cx_f,t.cy_f is NOT the same as CameraPositionX() , CameraPositionZ().
		//Search for "Debug c_xy" and enable those lines to see the difference.

		//Changed to this:

		float placeatx_f, placeatz_f;
		placeatx_f = CameraPositionX();
		placeatz_f = CameraPositionZ();

		t.inputsys.domodewaypointcreate = 0;
		t.inputsys.keypress = 1; t.inputsys.domodewaypoint = 1; t.grideditselect = 6;
		if (t.terrain.TerrainID>0)
		{
			g.waypointeditheight_f = BT_GetGroundHeight(t.terrain.TerrainID, placeatx_f, placeatz_f); //
		}
		else
		{
			g.waypointeditheight_f = g.gdefaultterrainheight;
		}
		t.waypointeditstyle = 1; t.waypointeditstylecolor = 0; t.waypointeditentity = 0;
		//t.mx_f = t.cx_f; t.mz_f = t.cy_f;
		t.mx_f = placeatx_f;
		t.mz_f = placeatz_f;
		waypoint_createnew();

		PointCamera(t.mx_f, g.waypointeditheight_f, t.mz_f);
		t.editorfreeflight.c.angx_f = CameraAngleX();
		t.editorfreeflight.c.angy_f = CameraAngleY();
	}

	//  fake mousemove values for low-response systems (when in zoomed in mode)
	if (t.grideditselect == 4)
	{
		if (t.inputsys.keyshift == 1)
		{
			if (t.inputsys.keyleft == 1)  t.inputsys.xmousemove = -10;
			if (t.inputsys.keyright == 1)  t.inputsys.xmousemove = 10;
			if (t.inputsys.keyup == 1)
				t.inputsys.ymousemove = -10;
			if (t.inputsys.keydown == 1)  t.inputsys.ymousemove = 10;
			set_inputsys_mclick(2);// t.inputsys.mclick = 2;
			t.inputsys.keyleft = 0;
			t.inputsys.keyright = 0;
			t.inputsys.keyup = 0;
			t.inputsys.keydown = 0;
		}
	}

	//Update statusbar
	++t.interfacestatusbarupdate;
	if (t.interfacestatusbarupdate > 30)
	{
		strcpy(statusbar, "");

		if (t.inputsys.xmouse == 500000)
		{
			//t.strwork = ""; t.statusbar_s = t.statusbar_s + "X: 0 Z: 0";
			strcpy(statusbar, "X: 0 Z: 0 | ");
		}
		else {
			//t.strwork = ""; t.statusbar_s = t.statusbar_s + "X:" + Str(t.inputsys.mmx) + " " + "Z:" + Str(t.inputsys.mmy);
			sprintf(statusbar, "X:%d Z:%d | ", t.inputsys.mmx, t.inputsys.mmy);
		}

		//PE: 17/08/21 reactivated.
		//t.statusbar_s = t.statusbar_s + " | ";
		if (t.gridentitygridlock == 0)  strcat(statusbar, "NORMAL"); // t.statusbar_s = t.statusbar_s + "NORMAL";
		if (t.gridentitygridlock == 1)  strcat(statusbar, "SNAP"); //t.statusbar_s = t.statusbar_s + "SNAP";
		if (t.gridentitygridlock == 2)  strcat(statusbar, "GRID"); //t.statusbar_s = t.statusbar_s + "GRID";

		//  editing mode

		//336 = Clipboard Selection Mode (CTRL+C=Copy DELETE=Clear)
		//332 = Terrain Painting Mode:
		//343 = Zoomed In Mode (Right click and drag to view, Left to Exit)
		//344 = Entity Editing Mode (R=Rotate Entity  ENTER=Find Floor/Wall)

		if (t.grideditselect == 0)
		{
			if (t.terrain.terrainpaintermode >= 6) {
				if (t.terrain.terrainpaintermode == 11) {
					t.laststatusbar_s = "Terrain Tree Editing Mode";
				}
				else if (t.terrain.terrainpaintermode == 12) {
					t.laststatusbar_s = "Terrain Bush Editing Mode";
				}
				else if (t.terrain.terrainpaintermode == 10) {
					t.laststatusbar_s = "Terrain Vegetation Editing Mode";
				}
				else {
					t.laststatusbar_s = "Terrain Painting Mode";
				}
			}
			else {

				t.laststatusbar_s = "Terrain Sculpt Mode ";

				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RAISE || ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_LOWER)
					t.laststatusbar_s = t.laststatusbar_s + "- Shape Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_LEVEL)
					t.laststatusbar_s = t.laststatusbar_s + "- Level Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_BLEND)
					t.laststatusbar_s = t.laststatusbar_s + "- Blend Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RAMP)
					t.laststatusbar_s = t.laststatusbar_s + "- Ramp Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_PICK)
					t.laststatusbar_s = t.laststatusbar_s + "- Pick Height Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_WRITE)
					t.laststatusbar_s = t.laststatusbar_s + "- Use Picked Height Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RANDOM)
					t.laststatusbar_s = t.laststatusbar_s + "- Random Mode";
				else if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RESTORE)
					t.laststatusbar_s = t.laststatusbar_s + "- Restore Mode";

			}
		}

		if (t.grideditselect == 5)
		{
			t.laststatusbar_s = "Object Editing Mode - ";
			t.laststatusbar_s = t.laststatusbar_s + " Object: " + t.relaytostatusbar_s;
		}
		if (t.grideditselect == 6)
		{
			//  add waypoint sta2tus
			t.laststatusbar_s = "Waypoint Editing Mode";
		}

		if (g_bCharacterCreatorPlusActivated)
			t.laststatusbar_s = "Character Creator Mode";
		if ( (bBuilder_Properties_Window || t.ebe.on == 1) || (t.gridentity > 0 && t.entityprofile[t.gridentity].isebe != 0) )
			t.laststatusbar_s = "Structure Editor Mode";
		if (bImporter_Window && t.importer.importerActive == 1)
			t.laststatusbar_s = "Importer Mode";

		//  only update infrequently
		t.interfacestatusbarupdate = 0;

	}

	static cstr WinTitle = "";

	if (strcmp(Lower(Left(g.projectfilename_s.Get(), Len(g.rootdir_s.Get()))), Lower(g.rootdir_s.Get())) == 0)
	{
		WinTitle = Right(g.projectfilename_s.Get(), Len(g.projectfilename_s.Get()) - Len(g.rootdir_s.Get()));
	}
	else
	{
		WinTitle = g.projectfilename_s;
	}
	if (g.projectmodified != 0)  WinTitle = WinTitle + "*";

	if (bStoryboardWindow)
	{
		WinTitle = Storyboard.gamename;
		if (Storyboard.project_readonly == 1)
		{
			WinTitle = WinTitle + " (read only)";
		}
		else
		{
			if (Storyboard.iChanged) WinTitle = WinTitle + "*";
		}
	}

	if (WinTitle != CurrentWinTitle) 
	{
		//Change windows title
		CurrentWinTitle = WinTitle;
		cstr NewTitle = "GameGuru MAX - ";
		if (strnicmp(WinTitle.Get(), "mapbank\\", 8) == 0)
			WinTitle = WinTitle.Get() + 8;
		NewTitle = NewTitle + WinTitle;
		SetWindowTitle(NewTitle.Get());
	}

}

void input_getcontrols ( void )
{
	//  Some actions are directly triggered by input subroutine
	t.inputsys.doload=0;
	t.inputsys.domodeterrain=0;
	t.inputsys.domodeentity=0;
	t.inputsys.domodemarker=0;
	t.inputsys.domodewaypoint=0;
	t.inputsys.doundo=0;
	t.inputsys.doredo=0;
	t.inputsys.tselcontrol=0;
	t.inputsys.tselcut=0;
	t.inputsys.tselcopy=0;
	t.inputsys.tseldelete=0;

	input_getdirectcontrols ( );

	//  Flag reset
	t.inputsys.dorotation=0;
	t.inputsys.domirror=0;
	t.inputsys.doflip=0;
	t.inputsys.doentityrotate=0;
	t.inputsys.dozoomin=0;
	t.inputsys.dozoomout=0;
	t.inputsys.doscrollleft=0;
	t.inputsys.doscrollright=0;
	t.inputsys.doscrollup=0;
	t.inputsys.doscrolldown=0;
	t.inputsys.domapresize=0;
	t.inputsys.dogroundmode=-1;
	t.inputsys.dozoomview=0;
	t.inputsys.dozoomviewmovex=0;
	t.inputsys.dozoomviewmovey=0;
	t.inputsys.dozoomviewmovez=0;
	t.inputsys.dozoomviewrotatex=0;
	t.inputsys.dozoomviewrotatey=0;
	t.inputsys.dozoomviewrotatez=0;
	t.inputsys.dosinglelayer=0;
	t.inputsys.tselfloor=0;
	t.inputsys.tselpaste=0;
	t.inputsys.tselwipe=0;
	t.inputsys.dosaveandrun=0;

	//  Input conditional flags
	if (t.inputsys.kscancode == 0) 
	{
		t.inputsys.keypress = 0;
		if (iForceScancode > 0 ) 
		{
			t.inputsys.kscancode = iForceScancode;
			iForceScancode = -1;
		}
		else if (bForceKey) 
		{
			bForceKey = false;
			t.inputsys.k_s = csForceKey;
		}
	}


	//  Construction Keys
	if (  t.inputsys.keycontrol == 0 ) 
	{
		// can get marker mode from anywhere
		if ( (t.inputsys.kscancode == Asc("M") || t.inputsys.k_s == "m") && t.inputsys.keypress == 0 ) 
		{
			t.inputsys.domodemarker = 1;
			t.inputsys.keypress = 1; 
		}

		if ( (t.grideditselect == 4 && t.gridentityinzoomview>0) || t.grideditselect == 5 ) 
		{
			if (  t.inputsys.k_s == "g" && t.inputsys.keypress == 0 )
			{
				t.inputsys.keypress=1; 
				t.gridentitygridlock=t.gridentitygridlock+1;
				if ( t.gridentitygridlock>2 )  
					t.gridentitygridlock = 0;
				pref.iGridMode = t.gridentitygridlock;
			}
			if (  t.inputsys.k_s == "y" && t.inputsys.keypress == 0 && g.gentitytogglingoff == 0 ) 
			{
				// only if not EBE
				if ( t.entityprofile[t.gridentity].isebe == 0 )
				{
					t.ttrygridentitystaticmode=1-t.gridentitystaticmode;
					t.ttrygridentity=t.gridentity ; editor_validatestaticmode ( );
				}
				t.inputsys.keypress=1; 
			}
			if (  t.inputsys.k_s == "u" && t.inputsys.keypress == 0 ) 
			{
				//  control auto-flatten
				t.inputsys.keypress=1;
				t.gridedit.autoflatten=1-t.gridedit.autoflatten;
			}
			if (  t.inputsys.k_s == "i" && t.inputsys.keypress == 0 ) 
			{
				//  control entity spray mode
				t.inputsys.keypress=1;
				t.gridedit.entityspraymode=1-t.gridedit.entityspraymode;
			}
			// except when in EBE mode which handles - and + keys for material changing
			if ( t.ebe.on == 0 )
			{
				if (  t.inputsys.k_s == "-" && t.inputsys.keypress == 0 ) { t.gridentitymodifyelement = 1  ; t.inputsys.keypress = 1; }
				if (  t.inputsys.k_s == "=" && t.inputsys.keypress == 0 ) { t.gridentitymodifyelement = 2 ; t.inputsys.keypress = 1; }
			}
		}

		//  editing mode
		if (  t.inputsys.k_s == "t" ) { t.inputsys.domodeterrain = 1  ; t.inputsys.dowaypointview = 0; }
		if (  t.inputsys.k_s == "o" ) { t.inputsys.domodeentity = 1  ; t.inputsys.dowaypointview = 0; }
		if (  t.inputsys.k_s == "p" ) { t.inputsys.domodewaypoint = 1  ; t.inputsys.dowaypointview = 0; }
		if ( t.inputsys.keyspace == 1 && t.inputsys.keypress == 0 ) { t.inputsys.dowaypointview=1-t.inputsys.dowaypointview ; t.inputsys.keypress=1 ; t.lastgrideditselect=-1  ; editor_refresheditmarkers ( ); }

		//  NUM-ROTATE CONTROLS
		if (  t.inputsys.k_s == "r" && t.inputsys.keypress == 0 && t.ebe.on == 0) { t.inputsys.dorotation = 1 ; t.inputsys.keypress = 1; }
		if (  t.grideditselect != 4 && t.grideditselect != 0 ) 
		{
			if (  t.inputsys.k_s == "1" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 1  ; t.inputsys.keypress = 1; }
			if (  t.inputsys.k_s == "2" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 2  ; t.inputsys.keypress = 1; }
			if (  t.inputsys.k_s == "3" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 3  ; t.inputsys.keypress = 1; }
			if (  t.inputsys.k_s == "4" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 4  ; t.inputsys.keypress = 1; }
			if (  t.inputsys.k_s == "5" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 5  ; t.inputsys.keypress = 1; }
			if (  t.inputsys.k_s == "6" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 6  ; t.inputsys.keypress = 1; }
			if (  t.inputsys.keyshift == 0 ) 
			{
				if (  t.inputsys.k_s == "0" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 98  ; t.inputsys.keypress = 1; }
			}
			else
			{
				if (  t.inputsys.k_s == "0" && t.inputsys.keypress == 0 ) { t.inputsys.doentityrotate = 99 ; t.inputsys.keypress = 1; }
			}
		}

		// Editing of Map
		if ( t.inputsys.k_s == ","  )  t.inputsys.dozoomin = 1;
		if ( t.inputsys.k_s == "."  )  t.inputsys.dozoomout = 1;

		// TAB Key causes layer edit view control
		if ( t.inputsys.kscancode == 15 && t.inputsys.keypress == 0 ) { t.inputsys.dosinglelayer = 1  ; t.inputsys.keypress = 1; }

		// F1 for help page

		// this is the non-IDE input function (need to consolidate at some point - yucky repeat code!)
	}
	else
	{
		if (  t.inputsys.k_s == "r" && t.ebe.on == 0)  t.inputsys.dorotation = 1;
	}

	//  Key Map Scroll and Resize
	if (  t.inputsys.keyshift == 0 ) 
	{
		if (  t.inputsys.keyleft == 1  )  t.inputsys.doscrollleft = 3;
		if (  t.inputsys.keyright == 1  )  t.inputsys.doscrollright = 3;
		if (  t.inputsys.keyup == 1  )  t.inputsys.doscrollup = 3;
		if (  t.inputsys.keydown == 1  )  t.inputsys.doscrolldown = 3;
	}
	else
	{
		if (  t.inputsys.keyleft == 1  )  t.inputsys.doscrollleft = 20;
		if (  t.inputsys.keyright == 1  )  t.inputsys.doscrollright = 20;
		if (  t.inputsys.keyup == 1  )  t.inputsys.doscrollup = 20;
		if (  t.inputsys.keydown == 1  )  t.inputsys.doscrolldown = 20;
	}

	//  Mouse Wheel control (170616 - but not when in EBE mode as its used for grid layer control)
	if ( t.ebe.on == 0 )
	{
		if (  t.grideditselect == 4 ) 
		{
			//  Zoomed in View
			t.zoomviewcamerarange_f -= (t.inputsys.wheelmousemove / 10.0);
		}
		else
		{
			//  Non-Zoomed in View
			if (  t.inputsys.keycontrol == 0 ) 
			{
				if (  t.inputsys.wheelmousemove<0  )
					t.inputsys.dozoomout = 1;
				if (  t.inputsys.wheelmousemove>0  )
					t.inputsys.dozoomin = 1;
			}
		}
	}

	//  UndoRedo Keys
	if (  t.inputsys.keycontrol == 1 ) 
	{
		if (  t.inputsys.k_s == "z" && t.inputsys.undokeypress == 0 ) { t.inputsys.doundo = 1  ; t.inputsys.undokeypress = 1; }
		if (  t.inputsys.k_s == "y" && t.inputsys.undokeypress == 0 ) { t.inputsys.doredo = 1  ; t.inputsys.undokeypress = 1; }
	}

	//  Controls only when in zoomview
	if (  t.grideditselect == 4 ) 
	{
		//  orient arrowkey movement to camera angle
		t.tca_f=WrapValue(CameraAngleY());
		if (  t.tca_f >= 360-45 || t.tca_f <= 45 ) 
		{
			t.txa=1 ; t.txb=2 ; t.txc=0 ; t.txd=0;
			t.tza=0 ; t.tzb=0 ; t.tzc=2 ; t.tzd=1;
		}
		else
		{
			if (  t.tca_f >= 180-45 && t.tca_f <= 180+45 ) 
			{
				t.txa=2 ; t.txb=1 ; t.txc=0 ; t.txd=0;
				t.tza=0 ; t.tzb=0 ; t.tzc=1 ; t.tzd=2;
			}
			else
			{
				if (  t.tca_f <= 180 ) 
				{
					t.txa=0 ; t.txb=0 ; t.txc=2 ; t.txd=1;
					t.tza=2 ; t.tzb=1 ; t.tzc=0 ; t.tzd=0;
				}
				else
				{
					t.txa=0 ; t.txb=0 ; t.txc=1 ; t.txd=2;
					t.tza=1 ; t.tzb=2 ; t.tzc=0 ; t.tzd=0;
				}
			}
		}
		t.inputsys.dozoomviewmovex=0 ; t.inputsys.dozoomviewmovez=0;
		if (  t.inputsys.keyleft == 1 ) { t.inputsys.dozoomviewmovex += t.txa  ; t.inputsys.dozoomviewmovez +=t.tza; }
		if (  t.inputsys.keyright == 1 ) { t.inputsys.dozoomviewmovex += t.txb  ; t.inputsys.dozoomviewmovez += t.tzb; }
		if (  t.inputsys.keyup == 1 ) { t.inputsys.dozoomviewmovex += t.txc  ; t.inputsys.dozoomviewmovez += t.tzc; }
		if (  t.inputsys.keydown == 1 ) { t.inputsys.dozoomviewmovex +=t.txd  ; t.inputsys.dozoomviewmovez += t.tzd; }
		//  control rotation
		if (  t.inputsys.k_s == "1" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatex = 1  ; t.inputsys.keypress = 1; }
		if (  t.inputsys.k_s == "2" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatex = 2  ; t.inputsys.keypress = 1; }
		if (  t.inputsys.k_s == "3" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatey = 1  ; t.inputsys.keypress = 1; }
		if (  t.inputsys.k_s == "4" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatey = 2  ; t.inputsys.keypress = 1; }
		if (  t.inputsys.k_s == "5" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatez = 1  ; t.inputsys.keypress = 1; }
		if (  t.inputsys.k_s == "6" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatez = 2  ; t.inputsys.keypress = 1; }
		if (  t.inputsys.keyshift == 0 ) 
		{
			if (  t.inputsys.k_s == "0" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatex = 98  ; t.inputsys.keypress = 1; }
		}
		else
		{
			if (  t.inputsys.k_s == "0" && t.inputsys.keypress == 0 ) { t.inputsys.dozoomviewrotatex = 99  ; t.inputsys.keypress = 1; }
		}
	}
	if (  t.grideditselect == 4 || t.grideditselect == 5 ) 
	{
		//  control finder (toggled using gridentityautofind value)
		if (  t.inputsys.keyreturn == 1 ) 
		{
			if (  t.gridentityautofind == 0  ) { t.gridentityautofind = 3; }
			if (  t.gridentityautofind == 1  ) { t.gridentityautofind = 2; }
		}
		else
		{
			if (  t.gridentityautofind == 3 ) { t.gridentityautofind = 1  ; t.gridentityusingsoftauto = 0; t.gridentitysurfacesnap = 0; }
			if (  t.gridentityautofind == 2 ) { t.gridentityautofind = 0  ; t.gridentityposoffground = 0 ; t.gridentityusingsoftauto = 1; t.gridentitysurfacesnap = 0; }
		}
		if ( t.gridentityautofind == 1 && t.gridentity>0 ) 
		{
			t.gridentitydroptoground = 1 + t.entityprofile[t.gridentity].forwardfacing;
		}
		else
		{
			t.gridentitydroptoground=0;
		}
		//  control height
		if (  t.grideditselect == 4 ) 
		{
			//  move entity through zoomview system
			if (  t.inputsys.kscancode == 201 ) { t.inputsys.dozoomviewmovey = 2  ; t.gridentityposoffground = 1 ; t.gridentityautofind = 0 ; t.gridentityusingsoftauto = 0; }
			if (  t.inputsys.kscancode == 209 ) { t.inputsys.dozoomviewmovey = 1  ; t.gridentityposoffground = 1 ; t.gridentityautofind = 0 ; t.gridentityusingsoftauto = 0; }
		}
		else
		{
			//  directly move entity (and detatch from terrain) PGUP and PGDN
			if ( t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209 ) 
			{
				if ( t.widget.activeObject == 0 ) 
				{
					editor_handlepguppgdn();
					t.gridentityposoffground=1 ; t.gridentityautofind=0 ; t.gridentityusingsoftauto=0; t.gridentitysurfacesnap=0;
				}
			}
		}
	}

	//  Create a waypoint when instructed to
	if (  t.inputsys.domodewaypointcreate == 1 && t.inputsys.keypress == 0 ) 
	{
		t.inputsys.domodewaypointcreate=0;
		t.inputsys.keypress=1 ; t.inputsys.domodewaypoint=1 ; t.grideditselect=6;
		if (  t.terrain.TerrainID>0 ) 
		{
			g.waypointeditheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.cx_f,t.cy_f);
		}
		else
		{
			g.waypointeditheight_f=g.gdefaultterrainheight;
		}
		t.waypointeditstyle=1 ; t.waypointeditstylecolor=0 ; t.waypointeditentity=0;
		t.mx_f=t.cx_f ; t.mz_f=t.cy_f  ; waypoint_createnew ( );
	}
}

bool CameraInsideObject (sObject* pObject)
{
	GGVECTOR3 vecLocalCamPos = GGVECTOR3(CameraPositionX(0), CameraPositionY(0), CameraPositionZ(0));
	vecLocalCamPos -= pObject->position.vecPosition;
	GGMATRIX inverseMatrix = pObject->position.matObjectNoTran;
	float fDet;
	GGMatrixInverse (&inverseMatrix, &fDet, &inverseMatrix);
	GGVec3TransformCoord(&vecLocalCamPos, &vecLocalCamPos, &inverseMatrix);
	float fMinX = pObject->collision.vecMin.x;
	float fMinY = pObject->collision.vecMin.y;
	float fMinZ = pObject->collision.vecMin.z;
	float fMaxX = pObject->collision.vecMax.x;
	float fMaxY = pObject->collision.vecMax.y;
	float fMaxZ = pObject->collision.vecMax.z;
	if (vecLocalCamPos.x >= fMinX && vecLocalCamPos.x <= fMaxX && vecLocalCamPos.y >= fMinY && vecLocalCamPos.y <= fMaxY && vecLocalCamPos.z >= fMinZ && vecLocalCamPos.z <= fMaxZ)
		return true;
	else
		return false;
}

uint32_t g_iGridEntityFlattener = -1;

void input_calculatelocalcursor ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	//PE: Dont change anything when right mouse down.
	if (ImGui::IsMouseDown(1)) return;

	// once object management begun, only allow ray tests once user has clicked and 'moved' the cursor
	static int iRecordedMouseAtStartOfManagement = 0;
	if (t.gridentityobj>0)
	{
		static XMFLOAT4 lastManagedMouse;
		XMFLOAT4 currentMouse = wiInput::GetPointer();
		if (iRecordedMouseAtStartOfManagement == 0)
		{
			iRecordedMouseAtStartOfManagement = 1;
			lastManagedMouse = currentMouse;
		}
		else
		{
			if (currentMouse.x != lastManagedMouse.x || currentMouse.y != lastManagedMouse.y)
			{
				iRecordedMouseAtStartOfManagement = 2;
				g_bHoldGridEntityPosWhenManaged = false;
			}
		}
		if (iRecordedMouseAtStartOfManagement != 2)
		{
			// until we move the mouse after starting an object management, just leave
			return;
		}
	}
	else
	{
		// when no more object to manage, can reset this system
		iRecordedMouseAtStartOfManagement = 0;
		g_bHoldGridEntityPosWhenManaged = false;
	}

	// use Wicked Pick System
	t.tx_f=0; t.tz_f=0;
	float fPickedYAxis = 0.0f;
	t.inputsys.localselectedrayhit = false;

	//PE: e <= g.entityelementmax so need one additional int, if last object in level was a marker we got a heap error.
	int* piEntityVisible = new int[g.entityelementmax+1];
	memset(piEntityVisible, 0, sizeof(int) * (g.entityelementmax+1));

	bool bDisableRubberBandMoving = false;
	if (current_selected_group >= 0 && group_editing_on)
	{
		bDisableRubberBandMoving = true;
	}
	bool bHideObjectsWeWantToIgnore = false;
	if (!bDisableRubberBandMoving && pref.iEnableDragDropEntityMode) bHideObjectsWeWantToIgnore = true;
	if ( bHideObjectsWeWantToIgnore == true )
	{
		//LB: When they are being dragged about, but allow when scanning to select an object in the rubberband (otherwise vertical move mode messes up)
		if (bDraggingActive)
		{
			//PE: MUST disable collision on ALL rubberband objects.
			if (g.entityrubberbandlist.size() > 0)
			{
				for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
				{
					int e = g.entityrubberbandlist[i].e;
					if (e <= g.entityelementmax)
					{
						int obj = t.entityelement[e].obj;
						if (obj > 0 && GetVisible(obj))
						{
							piEntityVisible[e] = 1;
							HideObject(obj);
						}
						else
						{
							piEntityVisible[e] = 0;
						}
					}
				}
			}
		}

		// also disable any gameelements (such as start marker) as they can get in the way
		if (bDraggingActive)
		{
			for (int e = 1; e <= g.entityelementmax; e++)
			{
				int entid = t.entityelement[e].bankindex;
				if (entid > 0)
				{
					if (t.entityprofile[entid].ismarker != 0)
					{
						int obj = t.entityelement[e].obj;
						if (obj > 0 && GetVisible(obj))
						{
							piEntityVisible[e] = 1;
							HideObject(obj);
						}
					}
				}
			}
		}
	}

	if (!pref.iEnableDragDropEntityMode)
	{
		int iLayerMaskForPick = GGRENDERLAYERS_NORMAL;
		if (bDraggingActive || t.gridentityobj > 0) iLayerMaskForPick = GGRENDERLAYERS_TERRAIN;
		if (WickedCall_GetPick(&t.tx_f, &fPickedYAxis, &t.tz_f, NULL, NULL, NULL, NULL, iLayerMaskForPick) == true)
			t.inputsys.localselectedrayhit = true;

		// special treatment of point lights
		if (t.gridentity > 0 && t.gridentityobj > 0 && t.entityprofile[t.gridentity].ismarker == 2)
		{
			if (t.grideleprof.usespotlighting == 0)
			{
				if (t.grideleprof.light.fLightHasProbe >= 50.0f )
					t.gridentityrotatex_f = 0;
				else
					t.gridentityrotatex_f = 90;

				t.gridentityrotatey_f = 0;
				t.gridentityrotatez_f = 0;
				t.gridentityrotatequatmode = 0;
				t.gridentityrotatequatx_f = 0;
				t.gridentityrotatequaty_f = 0;
				t.gridentityrotatequatz_f = 0;
				t.gridentityrotatequatw_f = 1;
			}
			fPickedYAxis += 10.0f;
		}
	}
	else
	{
		// LB: allow finding of other object surfaces when not dragging (initial placement of object)
		int iLayerMaskForPick = GGRENDERLAYERS_NORMAL | GGRENDERLAYERS_TERRAIN;

		uint64_t hitentity = 0;
		if (pref.iEnableDragDropEntityMode && bDraggingActive && t.gridentityobj > 0) HideObject(t.gridentityobj);

		// orient to surface mode keyboard shurtcut
		if (t.inputsys.keyspace == 1 && g_iOrientToSurfaceMode == 0) g_iOrientToSurfaceMode = 2;
		if (t.inputsys.keyspace == 0 && g_iOrientToSurfaceMode == 2) g_iOrientToSurfaceMode = 1;
		if (t.inputsys.keyspace == 1 && g_iOrientToSurfaceMode == 1) g_iOrientToSurfaceMode = 3;
		if (t.inputsys.keyspace == 0 && g_iOrientToSurfaceMode == 3) g_iOrientToSurfaceMode = 0;

		bool bApplyHitOffset = false;
		bool bRayResult = false;
		float fPickX, fPickZ;
		bool bMustFaceUpOrDown = false;
		float fUpDownAngle = WrapValue(CameraAngleX(0));
		if (fUpDownAngle > 10.0f && fUpDownAngle < 350.0f)
		{
			// a horizontal plane for down views
			bMustFaceUpOrDown = true;
		}
		// special method of detecting when should eliminate hitoffsets to helo with accurate positioning
		static float fLastPickedY = 0.0f;
		static float fLastDiff = 0.0f;
		if (!ImGui::IsMouseDown(0))
		{
			fLastPickedY = 0.0f;
			fLastDiff = 0.0f;
		}
		// pick
		float fNormalX = 0.0f;
		float fNormalY = 1.0f;
		float fNormalZ = 0.0f;
		int iForwardFacing = 0;
		if (t.gridentity > 0) iForwardFacing = t.entityprofile[t.gridentity].forwardfacing;
		if (iObjectMoveMode == 2 && t.gridentityobj > 0 && bMustFaceUpOrDown == true && t.gridentity > 0 && t.entityprofile[t.gridentity].ismarker != 2 && iForwardFacing != 2)
		{
			// work out difference to move virtual mouse pointer to base of object no matter the orientation
			GGVECTOR2 vecVirtMouseOffset = GGVECTOR2(0, 0); //vecBase - vecClickPos; too clever by half!
			XMFLOAT4 currentMouse = wiInput::GetPointer();
			bRayResult = WickedCall_GetPick2(currentMouse.x + vecVirtMouseOffset.x, currentMouse.y + vecVirtMouseOffset.y, &fPickX, &fPickedYAxis, &fPickZ, &fNormalX, &fNormalY, &fNormalZ, &hitentity, iLayerMaskForPick);
			// and finally put hitoffset back to restore object relative position
			bApplyHitOffset = true;
			// simpler system easier to use - shift mouse to object true base coord, no need for mouse 3D->2D adjustment and accurate for placement!
			if (fLastPickedY == 0.0f) fLastPickedY = fPickedYAxis;
			float fDiff = fabs(fPickedYAxis - fLastPickedY) - 5.0f;
			if (fDiff < 0.0f) fDiff = 0.0f;
			if (fDiff > fLastDiff)
			{
				fLastDiff = fDiff;
				fHitOffsetX *= 0.95f;
				fHitOffsetY *= 0.95f;
				fHitOffsetZ *= 0.95f;
			}
		}
		else
		{
			bRayResult = WickedCall_GetPick(&fPickX, &fPickedYAxis, &fPickZ, &fNormalX, &fNormalY, &fNormalZ, &hitentity, iLayerMaskForPick);
		}
		//LB: to help perfect plane positioning of EMPTY LEVEL scenes, clamp to zero Y if within threshold
		if (fPickedYAxis > -1.0f && fPickedYAxis < 1.0f) 
		{
			fPickedYAxis = 0.0f;
		}
		if (bApplyHitOffset == true)
		{
			fPickX += fHitOffsetX;
			fPickedYAxis += fHitOffsetY;
			fPickZ += fHitOffsetZ;
		}
		if (bRayResult == true )
		{
			// if initial selection or in ghost mode
			bool bJustForInitialDragIn = false;
			if (bDraggingActive == false && fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0) bJustForInitialDragIn = true;
			if (bDraggingActive == true && t.gridentityposx_f == 0 && t.gridentityposz_f == 0) bJustForInitialDragIn = true;
			if (bDraggingActiveInitial == true)	bJustForInitialDragIn = true;
			if (bJustForInitialDragIn==true || (iObjectMoveModeDropSystemUsing == 1 && g_bHoldGridEntityPosWhenManaged == false) || iForwardFacing == 1 || (t.entityprofile[t.gridentity].ismarker == 2))
			{
				// modify the rotation of the object in smart mode when object is forward facing
				if (t.gridentityobj > 0 && t.gridentity > 0 && t.entityprofile[t.gridentity].ischaracter == 0)
				{
					if (iObjectMoveMode == 2)
					{
						bool bAdjustAndTilt = false;
						if (t.entityprofile[t.gridentity].ismarker == 0)
						{
							if (iForwardFacing == 0 && g_iStackToSurfaceMode == 1 && g_iOrientToSurfaceMode == 1)
							{
								// ground objects can find orientation if mode selected
								bAdjustAndTilt = true;
							}
							if (iForwardFacing == 1)
							{
								// wall objects can find ANY surface
								bAdjustAndTilt = true;
							}
						}
						else
						{
							// lights ALWAYS orient to surface (but not probes)
							if (t.entityprofile[t.gridentity].ismarker == 2 && t.grideleprof.light.fLightHasProbe < 50.0f)
							{
								bAdjustAndTilt = true;
							}
						}
						if (bAdjustAndTilt == true)
						{
							fNormalX *= 100.0f;
							fNormalY *= 100.0f;
							fNormalZ *= 100.0f;
							int iObj = t.gridentityobj;
							float fStoreAngX = ObjectAngleX(iObj);
							float fStoreAngY = ObjectAngleY(iObj);
							float fStoreAngZ = ObjectAngleZ(iObj);
							PointObject(t.gridentityobj, ObjectPositionX(iObj) - fNormalX, ObjectPositionY(iObj) - fNormalY, ObjectPositionZ(iObj) - fNormalZ);
							if (iForwardFacing == 0)
							{
								PitchObjectUp(iObj, 90);
								TurnObjectRight(iObj, g_fLocalTurnRotationForSmartMode);
							}
							float fStoreEntAngX = t.gridentityrotatex_f;
							float fStoreEntAngY = t.gridentityrotatey_f;
							float fStoreEntAngZ = t.gridentityrotatez_f;
							if (t.entityprofile[t.gridentity].ismarker == 2 && t.grideleprof.usespotlighting == 1)
							{
								// spotlights need to retain their rotation!
							}
							else
							{
								t.gridentityrotatex_f = ObjectAngleX(iObj);
								t.gridentityrotatey_f = ObjectAngleY(iObj);
								t.gridentityrotatez_f = ObjectAngleZ(iObj);
								t.gridentityrotatequatmode = 0;
								t.gridentityrotatequatx_f = 0;
								t.gridentityrotatequaty_f = 0;
								t.gridentityrotatequatz_f = 0;
								t.gridentityrotatequatw_f = 1;
							}
							if (fStoreEntAngX != t.gridentityrotatex_f || fStoreEntAngY != t.gridentityrotatey_f || fStoreEntAngZ != t.gridentityrotatez_f)
							{
								// rotation resets hitoffset as it does not translate when orientation changes
								if (bJustForInitialDragIn == false)
								{
									// but only when dragging, not initial placement
									fHitOffsetX = 0.001f;
									fHitOffsetY = 0.001f;
									fHitOffsetZ = 0.001f;
								}
							}
							if (t.entityprofile[t.gridentity].ismarker == 2)
							{
								fHitOffsetX = -fNormalX / 10.0f;
								fHitOffsetY = -fNormalY / 10.0f;
								fHitOffsetZ = -fNormalZ / 10.0f;
							}
							RotateObject(iObj, fStoreAngX, fStoreAngY, fStoreAngZ);
						}
					}
					else
					{
						// all non-smart modes should keep lights hanging down
						if (t.entityprofile[t.gridentity].ismarker == 2 && t.grideleprof.usespotlighting == 0)
						{
							if (t.grideleprof.light.fLightHasProbe >= 50.0f)
								t.gridentityrotatex_f = 0;
							else
								t.gridentityrotatex_f = 270;

							t.gridentityrotatey_f = 0;
							t.gridentityrotatez_f = 0;
							t.gridentityrotatequatmode = 0;
							t.gridentityrotatequatx_f = 0;
							t.gridentityrotatequaty_f = 0;
							t.gridentityrotatequatz_f = 0;
							t.gridentityrotatequatw_f = 1;
							fPickedYAxis += 10.0f;
						}
					}
				}
			}

			// for ceiling objects, hang down from top of object
			if (iForwardFacing == 2 && t.entityprofile[t.gridentity].ismarker == 0)
			{
				fHitOffsetX = 0.001f;
				fHitOffsetY = ObjectSizeY(t.gridentityobj, 1) * 0.95f; // allow a small margin so can see if try to place on ground
				fHitOffsetZ = 0.001f;
			}

			// register the ray hit
			t.inputsys.localselectedrayhit = true;

			// confirm pick
			t.tx_f = fPickX;
			t.tz_f = fPickZ;
		}
		g_bAdjustPlaneXZUsingSurfaceXZ = true;
		if (hitentity > 0)
		{
			// if ray test on object, need to adjust plane to follow surface
			iLastHitObjectID = 0;
			iReusePickObjectID = -1;
			pReusePickObject = 0;
			iReusePickEntityID = 0;
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
				pReusePickObject = pHitObject;
				int e = iHitObjectEntityElementE;
				if (e > 0)
				{
					iLastHitObjectID = pHitObject->dwObjectNumber;
					iReusePickObjectID = iLastHitObjectID;
					iReusePickEntityID = e;
					fReusePickHitX = t.tx_f;
					fReusePickHitY = fPickedYAxis;
					fReusePickHitZ = t.tz_f;
				}
			}
		}
		else
		{
			// if not touching object surface, must be terrain, so restore plane in case plane was moved with 'g_bAdjustPlaneXZUsingSurfaceXZ'
			if (iObjectMoveMode == 2 && t.inputsys.mclick == 0 )
			{
				g_bResetPlaneAfterXZAdjust = true;
			}
		}
		if (pref.iEnableDragDropEntityMode && bDraggingActive && t.gridentityobj > 0) ShowObject(t.gridentityobj);
	}
	t.inputsys.picksystemused=2;

	if (bHideObjectsWeWantToIgnore == true)
	{
		// PE: Enable rubberband collision again.
		// also renable any gameelements (such as start marker) as they can get in the way
		for (int e = 1; e <= g.entityelementmax; e++)
		{
			int obj = t.entityelement[e].obj;
			if (obj > 0)
			{
				if (piEntityVisible[e] == 1)
				{
					ShowObject(obj);
				}
			}
		}
	}
	t.tilex_f=t.tx_f;
	t.tiley_f=t.tz_f;

	// free temp vis array
	if (piEntityVisible)
	{
		//PE: stille an array so [].
		delete[] piEntityVisible;
		piEntityVisible = NULL;
	}

	//  World cursor position
	t.inputsys.localx_f=t.tx_f;
	t.inputsys.localy_f=t.tz_f;
	t.tx=t.inputsys.localx_f/100.0;
	t.ty=t.inputsys.localy_f/100.0;
	if (  t.tx<0  )  t.tx = 0;
	if (  t.ty<0  )  t.ty = 0;
	if (  t.tx>t.maxx-1  )  t.tx = t.maxx-1;
	if (  t.ty>t.maxy-1  )  t.ty = t.maxy-1;
	t.inputsys.mmx=t.tx ; t.inputsys.mmy=t.ty;

	//  layer height is terrain Floor height
	t.inputsys.localcurrentterrainheight_f = BT_GetGroundHeight(t.terrain.TerrainID,t.tx_f,t.tz_f);

	// when placing waypoints, include entities as 'ground' to check
	t.inputsys.originallocalx_f = t.inputsys.localx_f;
	t.inputsys.originallocaly_f = t.inputsys.localy_f;

	extern bool bWaypointDrawmode;
	if ( t.gridentitysurfacesnap == 1 || t.onedrag > 0 || bWaypointDrawmode)
	{
		// only when finding place to place entity
		if ( t.gridentity > 0 || t.onedrag > 0 || bWaypointDrawmode)
		{
			// get distance of current terrain hit
			float fTDX = t.inputsys.localx_f - CameraPositionX();
			float fTDY = t.inputsys.localcurrentterrainheight_f - CameraPositionY();
			float fTDZ = t.inputsys.localy_f - CameraPositionZ();
			float fTerrDist = sqrt ( fabs(fTDX*fTDX) + fabs(fTDY*fTDY) * fabs(fTDZ*fTDZ) );

			// scan for surface point
			int iEntityOver = findentitycursorobj ( -1 );
			if ( iEntityOver != 0 )
			{
				// get distance of new surface point
				if (t.gridnearcameraclip > 0) 
				{
					fTDX = g.glastpickedx_f - GetFromVectorX();
					fTDY = g.glastpickedy_f - GetFromVectorY();
					fTDZ = g.glastpickedz_f - GetFromVectorZ();
				}
				else 
				{
					fTDX = g.glastpickedx_f - CameraPositionX();
					fTDY = g.glastpickedy_f - CameraPositionY();
					fTDZ = g.glastpickedz_f - CameraPositionZ();
				}
				float fSurfaceDist = sqrt ( fabs(fTDX*fTDX) + fabs(fTDY*fTDY) * fabs(fTDZ*fTDZ) );

				// if surface closer, use that
				if ( fSurfaceDist < fTerrDist )
				{
					t.inputsys.localx_f = g.glastpickedx_f;
					t.inputsys.localcurrentterrainheight_f = g.glastpickedy_f;
					t.inputsys.localy_f = g.glastpickedz_f;
				}
			}
		}
	}

	if (t.inputsys.picksystemused == 2)
	{
		// wicked pick system provides Y coordinate via terrainheight value
		t.inputsys.localcurrentterrainheight_f = fPickedYAxis;
		
		// LB: when clicking and dragging, we need to know ray hit Y so we can position
		// the widget plane at that height for correct movement of the object (otherwise it is at terrain height and goes wrong!)
		if (pref.iEnableDragDropEntityMode)
		{
			// monitor when not dragging, and use last height just before the click as the plane height
			if (bDraggingActive == false)
			{
				t.inputsys.localselectedplaneheight_f = fPickedYAxis;
			}
		}
	}

	//  height at which zoom editing happens
	t.layerheight_f=t.zoomviewtargety_f+100.0;
}

void editor_updatemarkervisibility ( void )
{
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entityprofile[t.entid].ismarker != 0 ) 
		{
			t.obj=t.entityelement[t.e].obj;
			if (  t.obj>0 ) 
			{
				if (  ObjectExist(t.obj) == 1 ) 
				{
					if (  t.gridentityhidemarkers == 0 ) 
					{
						ShowObject (  t.obj );
					}
					else
					{
						HideObject (  t.obj );
					}
				}
			}
		}
	}
	if (  t.gridentityhidemarkers == 0 ) 
	{
		waypoint_showall ( );
	}
	else
	{
		waypoint_hideall ( );
	}
}

void editor_disableforzoom ( void )
{
	OpenFileMap (  2, "FPSEXCHANGE" );
	SetFileMapDWORD (  2, 850, 1 );
	SetEventAndWait (  2 );
}

void editor_enableafterzoom ( void )
{
	OpenFileMap (  2, "FPSEXCHANGE" );
	SetFileMapDWORD (  2, 850, 0 );
	SetEventAndWait (  2 );
	editor_cutcopyclearstate ( );
}

