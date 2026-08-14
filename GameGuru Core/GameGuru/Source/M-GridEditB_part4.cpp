//PE: Using t.gridentityposx_f,t.gridentityposy_f,t.gridentityposz_f,t.gridentity,t.gridentityobj
void Add_Grid_Snap_To_Position ( bool bFromWidgetMode )
{
	// no snapping at all until move
	if (g_bHoldGridEntityPosWhenManaged == true)
		return;

	//  grid system for entities
	if (t.gridentitygridlock == 1)
	{
		//Snap.
		if (iObjectMoveMode == 1)
		{
			// small grid lock for better alignments on Y
			t.gridentityposy_f = (int(t.gridentityposy_f / 5) * 5);
		}
		else
		{
			// small grid lock for better alignments on XZ
			t.gridentityposx_f = (int(t.gridentityposx_f / 5) * 5);
			t.gridentityposz_f = (int(t.gridentityposz_f / 5) * 5);
		}

		//  special snap-to when edge of entity gets near another
		if (t.gridentity > 0)
		{
			t.tobj = t.gridentityobj;
			if (t.tobj > 0)
			{
				if (ObjectExist(t.tobj) == 1 )
				{
					t.tsrcx_f = t.gridentityposx_f;
					t.tsrcy_f = t.gridentityposy_f;
					t.tsrcz_f = t.gridentityposz_f;
					t.tsrcradius_f = ObjectSize(t.tobj, 1);
					t.tfindclosest = -1; t.tfindclosestbest_f = 999999;
					for (t.e = 1; t.e <= g.entityelementlist; t.e++)
					{
						// this object
						t.ttobj = t.entityelement[t.e].obj;

						//LB: never include syblings of a current group :)
						bool bThisOneIsSybling = false;
						for (int i = 0; i < g.entityrubberbandlist.size(); i++)
						{
							if (g.entityrubberbandlist[i].e == t.e)
							{
								bThisOneIsSybling = true;
								break;
							}
						}
						if (bThisOneIsSybling == true)
							continue;

						if (t.ttobj > 0 && t.tobj != t.ttobj ) //PE: Never check ourself.
						{
							if (t.entityelement[t.e].bankindex == t.gridentity)
							{
								t.tdiffx_f = t.entityelement[t.e].x - t.tsrcx_f;
								t.tdiffy_f = t.entityelement[t.e].y - t.tsrcy_f;
								t.tdiffz_f = t.entityelement[t.e].z - t.tsrcz_f;
								t.tdiff_f = Sqrt(abs(t.tdiffx_f*t.tdiffx_f) + abs(t.tdiffy_f*t.tdiffy_f) + abs(t.tdiffz_f*t.tdiffz_f));
								t.tthisradius_f = ObjectSize(t.ttobj, 1);
								if (t.tdiff_f < t.tsrcradius_f + t.tthisradius_f && t.tdiff_f < t.tfindclosestbest_f)
								{
									t.tfindclosestbest_f = t.tdiff_f;
									t.tfindclosest = t.e;
								}
							}
						}
					}
					if (t.tfindclosest != -1)
					{
						//  go through 6 magnet points of the src entity
						t.tmag1sizex_f = ObjectSizeX(t.tobj, 1) / 2;
						t.tmag1sizey_f = ObjectSizeY(t.tobj, 1) / 2;
						t.tmag1sizez_f = ObjectSizeZ(t.tobj, 1) / 2;
						t.tmag2sizex_f = ObjectSizeX(t.entityelement[t.tfindclosest].obj, 1) / 2;
						t.tmag2sizey_f = ObjectSizeY(t.entityelement[t.tfindclosest].obj, 1) / 2;
						t.tmag2sizez_f = ObjectSizeZ(t.entityelement[t.tfindclosest].obj, 1) / 2;
						if (ObjectExist(g.entityworkobjectoffset) == 0) { MakeObjectCube(g.entityworkobjectoffset, 40); HideObject(g.entityworkobjectoffset); }
						t.tbestmag_f = 99999; t.tbestmag2id = -1;
						for (t.magid = 1; t.magid <= 6; t.magid++)
						{
							t.tmagx_f = t.gridentityposx_f;
							t.tmagy_f = t.gridentityposy_f;
							t.tmagz_f = t.gridentityposz_f;
							PositionObject(g.entityworkobjectoffset, t.tmagx_f, t.tmagy_f, t.tmagz_f);
							RotateObject(g.entityworkobjectoffset, ObjectAngleX(t.tobj), ObjectAngleY(t.tobj), ObjectAngleZ(t.tobj));
							if (t.magid == 1)  MoveObjectLeft(g.entityworkobjectoffset, t.tmag1sizex_f);
							if (t.magid == 2)  MoveObjectRight(g.entityworkobjectoffset, t.tmag1sizex_f);
							if (t.magid == 3)  MoveObjectUp(g.entityworkobjectoffset, t.tmag1sizey_f);
							if (t.magid == 4)  MoveObjectDown(g.entityworkobjectoffset, t.tmag1sizey_f);
							if (t.magid == 5)  MoveObject(g.entityworkobjectoffset, t.tmag1sizez_f);
							if (t.magid == 6)  MoveObject(g.entityworkobjectoffset, t.tmag1sizez_f*-1);
							t.tmagx_f = ObjectPositionX(g.entityworkobjectoffset);
							t.tmagy_f = ObjectPositionY(g.entityworkobjectoffset);
							t.tmagz_f = ObjectPositionZ(g.entityworkobjectoffset);
							t.ttobj = t.entityelement[t.tfindclosest].obj;
							for (t.mag2id = 1; t.mag2id <= 6; t.mag2id++)
							{
								t.tmag2x_f = t.entityelement[t.tfindclosest].x;
								t.tmag2y_f = t.entityelement[t.tfindclosest].y;
								t.tmag2z_f = t.entityelement[t.tfindclosest].z;
								PositionObject(g.entityworkobjectoffset, t.tmag2x_f, t.tmag2y_f, t.tmag2z_f);
								RotateObject(g.entityworkobjectoffset, ObjectAngleX(t.ttobj), ObjectAngleY(t.ttobj), ObjectAngleZ(t.ttobj));
								if (t.mag2id == 1)  MoveObjectLeft(g.entityworkobjectoffset, t.tmag2sizex_f);
								if (t.mag2id == 2)  MoveObjectRight(g.entityworkobjectoffset, t.tmag2sizex_f);
								if (t.mag2id == 3)  MoveObjectUp(g.entityworkobjectoffset, t.tmag2sizey_f);
								if (t.mag2id == 4)  MoveObjectDown(g.entityworkobjectoffset, t.tmag2sizey_f);
								if (t.mag2id == 5)  MoveObject(g.entityworkobjectoffset, t.tmag2sizez_f);
								if (t.mag2id == 6)  MoveObject(g.entityworkobjectoffset, t.tmag2sizez_f*-1);
								t.tmag2x_f = ObjectPositionX(g.entityworkobjectoffset);
								t.tmag2y_f = ObjectPositionY(g.entityworkobjectoffset);
								t.tmag2z_f = ObjectPositionZ(g.entityworkobjectoffset);
								//  are magnets close enough together to snap?
								t.tdiffx_f = t.tmag2x_f - t.tmagx_f;
								t.tdiffy_f = t.tmag2y_f - t.tmagy_f;
								t.tdiffz_f = t.tmag2z_f - t.tmagz_f;
								t.tdiff_f = Sqrt(abs(t.tdiffx_f*t.tdiffx_f) + abs(t.tdiffy_f*t.tdiffy_f) + abs(t.tdiffz_f*t.tdiffz_f));
								if (t.tdiff_f < 25.0)
								{
									//  yes, maybe snap to this edge
									if (t.tdiff_f < t.tbestmag_f)
									{
										t.tbestmag_f = t.tdiff_f;
										t.tbestmag2id = t.mag2id;
										t.tbestmag2x_f = t.tmag2x_f + (t.gridentityposx_f - t.tmagx_f);
										t.tbestmag2y_f = t.tmag2y_f + (t.gridentityposy_f - t.tmagy_f);
										t.tbestmag2z_f = t.tmag2z_f + (t.gridentityposz_f - t.tmagz_f);
									}
								}
							}
						}
						if (t.tbestmag2id != -1)
						{
							if (iObjectMoveMode == 1)
							{
								// only magnetise Y for vert mode
								t.gridentityposy_f = t.tbestmag2y_f;
							}
							else
							{
								if (iObjectMoveMode == 0)
								{
									// only magnetise XZ for horiz modes
									t.gridentityposx_f = t.tbestmag2x_f;
									t.gridentityposz_f = t.tbestmag2z_f;
								}
								else
								{
									// magnetize all three to fix the Y coming from unstable terrain/floor (-0.0 misalignment)
									t.gridentityposx_f = t.tbestmag2x_f;
									t.gridentityposy_f = t.tbestmag2y_f;
									t.gridentityposz_f = t.tbestmag2z_f;
								}
							}
						}
					}
				}
			}
		}
	}
	if (t.gridentitygridlock == 2)
	{
		if (t.entityprofile[t.gridentity].isebe != 0)
		{
			// align EBE structure to match the 100x100 grid
			t.gridentityposx_f = 0 + (int(t.gridentityposx_f / pref.fEditorGridSizeX) * pref.fEditorGridSizeX);
			t.gridentityposz_f = 0 + (int(t.gridentityposz_f / pref.fEditorGridSizeZ) * pref.fEditorGridSizeZ);
		}
		else
		{
			//LB: apply grid alignment with custom offset for full end user control
			float fGripX, fGripY, fGripZ;
			if (bFromWidgetMode == true)
			{
				// widget mode uses its own drag offset system
				fHitOffsetX = 0; 
				fHitOffsetZ = 0;
				fHitOffsetY = 0;
				fGripX = t.gridentityposx_f + fHitOffsetX + (pref.fEditorGridSizeX / 2);
				fGripY = t.gridentityposy_f + fHitOffsetY + (pref.fEditorGridSizeY / 2);
				fGripZ = t.gridentityposz_f + fHitOffsetZ + (pref.fEditorGridSizeZ / 2);
			}
			else
			{
				// smart mode needs retains fHitOffsetXYZ to know the initial click offset to allow fine placement
				// and not the old legacy snap to from anchor in center of object (was a nice idea but not how most users expect it to work)
				fGripX = t.gridentityposx_f + (pref.fEditorGridSizeX / 2);
				fGripY = t.gridentityposy_f + (pref.fEditorGridSizeY / 2);
				fGripZ = t.gridentityposz_f + (pref.fEditorGridSizeZ / 2);
			}

			fGripX -= pref.fEditorGridOffsetX;
			if (fGripX < 0)
				fGripX = ((int(fGripX / pref.fEditorGridSizeX) - 1) * pref.fEditorGridSizeX);
			else
				fGripX = (int(fGripX / pref.fEditorGridSizeX) * pref.fEditorGridSizeX);
			fGripX += pref.fEditorGridOffsetX;

			// new for 2025
			if (pref.fEditorGridSizeY > 0)
			{
				fGripY -= pref.fEditorGridOffsetY;
				if (fGripY < 0)
					fGripY = ((int(fGripY / pref.fEditorGridSizeY) - 1) * pref.fEditorGridSizeY);
				else
					fGripY = (int(fGripY / pref.fEditorGridSizeY) * pref.fEditorGridSizeY);
				fGripY += pref.fEditorGridOffsetY;
			}

			fGripZ -= pref.fEditorGridOffsetZ;
			if (fGripZ < 0)
				fGripZ = ((int(fGripZ / pref.fEditorGridSizeZ) - 1) * pref.fEditorGridSizeZ);
			else
				fGripZ = (int(fGripZ / pref.fEditorGridSizeZ) * pref.fEditorGridSizeZ);
			fGripZ += pref.fEditorGridOffsetZ;

			t.gridentityposx_f = fGripX;
			t.gridentityposz_f = fGripZ;
			if (pref.fEditorGridSizeY > 0)
			{
				//PE: Allow object to go 80% below terrain.
				int GetActiveEditorObject(void);
				int iActiveObj = GetActiveEditorObject();

				// only if above or on terrain
				float fTerrainAtThisPoint = BT_GetGroundHeight (0, t.gridentityposx_f, t.gridentityposz_f);
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

		// 130517 - new EBE entity offset to align with 0,0,0 cornered entities from Aslum level and Store (Martin)
		if (t.entityprofile[t.gridentity].isebe != 0)
		{
			if (g.gdefaultebegridoffsetx != 50)
			{
				t.gridentityposx_f -= (g.gdefaultebegridoffsetx - 50);
				t.gridentityposz_f -= (g.gdefaultebegridoffsetz - 50);
			}
		}
	}
}

void DisplaySmallImGuiMessage(char *text)
{
	ImGui::SetNextWindowPos(OldrenderTargetPos + ImVec2(50, 50), ImGuiCond_Always); //ImGuiCond_Always
	ImGui::SetNextWindowSize(ImVec2(OldrenderTargetSize.x - 100, 0), ImGuiCond_Always); //ImGuiCond_Always
	bool winopen = true;

	ImVec4* style_colors = ImGui::GetStyle().Colors;
	ImVec4 oldBgColor = style_colors[ImGuiCol_WindowBg];
	ImVec4 oldTextColor = style_colors[ImGuiCol_Text];

	float fader = 0.75;
	style_colors[ImGuiCol_WindowBg].x = 0.0;
	style_colors[ImGuiCol_WindowBg].y = 0.0;
	style_colors[ImGuiCol_WindowBg].z = 0.0;
	style_colors[ImGuiCol_WindowBg].w *= (fader*0.25);

	style_colors[ImGuiCol_Text].x = 1.0;
	style_colors[ImGuiCol_Text].y = 1.0;
	style_colors[ImGuiCol_Text].z = 1.0;
	style_colors[ImGuiCol_Text].w *= fader;

	ImGui::Begin("##TriggerSmallMessageinfo", &winopen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowFontScale(1.5);

	//Center Text.
	float fTextSize = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5), ImGui::GetCursorPos().y));

	ImGui::Text(text);
	ImGui::SetWindowFontScale(1.0);
	ImGui::End();
	style_colors[ImGuiCol_WindowBg] = oldBgColor;
	style_colors[ImGuiCol_Text] = oldTextColor;
}

//PE: We can have more then one open at the same time. but only one can play.
#define MAXTUTORIALS 100
bool bSmallVideoReady[MAXTUTORIALS];
bool bSmallVideoResumePossible[MAXTUTORIALS];
bool bSmallVideoInit[MAXTUTORIALS];
bool bSmallVideoFirstClick[MAXTUTORIALS];
int iSmallVideoSlot[MAXTUTORIALS];
int iSmallVideoThumbnail[MAXTUTORIALS];
bool bSmallVideoPerccentStart[MAXTUTORIALS];
int iSmallVideoDelayExecute[MAXTUTORIALS];
int iSmallVideoFindFirstFrame[MAXTUTORIALS];
bool bSmallStartInit[MAXTUTORIALS];
bool bSmallVideoMaximized[MAXTUTORIALS];
cstr cSmallComboSelection[MAXTUTORIALS];
cstr cSmallVideoPath ="";
cstr cSmallVideoDescription = "";
int iCurrentVideoSectionPlaying = 0;
int iStopAndFreeThisVideo = -1;

// GGMAX 2.51: a widget video is paused/stopped only by the widget's OWN per-frame draw
// (the iSmallVideoFindFirstFrame countdown below). Leave the section before that runs —
// one storyboard click is enough — and the MF session keeps decoding into the editor and
// test game (~12 ms EVERY frame on DX12: CPU YUY2->RGBA + a new bridge texture per frame;
// it silently halved a whole 19-demo sweep before it was caught). Each drawn entry stamps
// a heartbeat; the watchdog frees any still-PLAYING entry whose heartbeat has gone stale.
// Paused entries are left alone so the legacy resume-on-return behaviour is preserved.
static unsigned int g_ggTutorialWatchdogFrame = 0;
static unsigned int g_ggTutorialEntryHeartbeat[MAXTUTORIALS] = { 0 };

void SmallTutorialThumbLoad(int index)
{
	if (iSmallVideoThumbnail[index] == 0)
	{
		iSmallVideoThumbnail[index] = -1;
		t.tvideofile_s = cSmallVideoPath;
		t.text_s = Lower(Right(t.tvideofile_s.Get(), 4));
		if (t.text_s == ".ogv" || t.text_s == ".mp4")
		{
			cStr Thumb_s = Lower(Left(t.tvideofile_s.Get(), strlen(t.tvideofile_s.Get()) - 4)); Thumb_s += ".jpg";
			if (!FileExist(Thumb_s.Get()))
			{
				Thumb_s = "tutorialbank\\welcome-video.jpg";
			}
			if (FileExist(Thumb_s.Get()))
			{
				int iVideoThumbImage = g.videothumbnailsimageoffset + index;
				if (ImageExist(iVideoThumbImage) == 1) DeleteImage(iVideoThumbImage);
				image_setlegacyimageloading(true);
				LoadImage(Thumb_s.Get(), iVideoThumbImage);
				image_setlegacyimageloading(false);
				if (ImageExist(iVideoThumbImage) == 1) iSmallVideoThumbnail[index] = iVideoThumbImage;
			}
		}
	}
}

void SmallTutorialVideoInit(int index)
{
	if (!bSmallVideoInit[index])
	{
		t.tvideofile_s = cSmallVideoPath;
		iSmallVideoSlot[index] = 0;
		iSmallVideoThumbnail[index] = 0;
		t.text_s = Lower(Right(t.tvideofile_s.Get(), 4));
		if (t.text_s == ".ogv" || t.text_s == ".mp4")
		{
			for (int itl = 1; itl <= 32; itl++)
			{
				if (AnimationExist(itl) == 0) { iSmallVideoSlot[index] = itl; break; }
			}
			if (LoadAnimation(t.tvideofile_s.Get(), iSmallVideoSlot[index], g.videoprecacheframes, g.videodelayedload, 1) == false)
			{
				iSmallVideoSlot[index] = -999;
			}
			// also load in the thumnb image for this video
			cStr Thumb_s = Lower(Left(t.tvideofile_s.Get(), strlen(t.tvideofile_s.Get())-4)); Thumb_s += ".jpg";
			if (ImageExist(g.videothumbnailsimageoffset + index) != 1)
			{
				if (!FileExist(Thumb_s.Get()))
				{
					Thumb_s = "tutorialbank\\welcome-video.jpg";
				}
				if (FileExist(Thumb_s.Get()))
				{
					int iVideoThumbImage = g.videothumbnailsimageoffset + index;
					if (ImageExist(iVideoThumbImage) == 1) DeleteImage(iVideoThumbImage);
					image_setlegacyimageloading(true);
					LoadImage(Thumb_s.Get(), iVideoThumbImage);
					image_setlegacyimageloading(false);
					if (ImageExist(iVideoThumbImage) == 1) iSmallVideoThumbnail[index] = iVideoThumbImage;
				}
			}
		}
		if (iSmallVideoSlot[index] > 0) 
		{
			PlaceAnimation(iSmallVideoSlot[index], -1, -1, -1, -1);
			bSmallVideoResumePossible[index] = false;
			bSmallVideoPerccentStart[index] = false;
		}
		bSmallVideoInit[index] = true;
	}
}

void SmallTutorialVideoCheckStop(char *tutorial)
{
	if (!tutorial) return;
	int iVideoEntry = -1;

	if (tutorial_videos.size() > 0)
	{
		int i = 0;
		for (std::map<std::string, std::string>::iterator it = tutorial_videos.begin(); it != tutorial_videos.end(); ++it)
		{
			if (it->first.length() > 0)
			{
				if (strcmp(it->first.c_str(), tutorial) == 0)
				{
					iVideoEntry = i;
					break;
				}
			}
			i++;
		}
	}

	if (iStopAndFreeThisVideo >= 0)
	{
		if (iSmallVideoSlot[iStopAndFreeThisVideo] > 0) {
			if (AnimationExist(iSmallVideoSlot[iStopAndFreeThisVideo])) {
				if (AnimationPlaying(iSmallVideoSlot[iStopAndFreeThisVideo]))
					StopAnimation(iSmallVideoSlot[iStopAndFreeThisVideo]);
				DeleteAnimation(iSmallVideoSlot[iStopAndFreeThisVideo]);
				iSmallVideoSlot[iStopAndFreeThisVideo] = 0;
				bSmallVideoPerccentStart[iStopAndFreeThisVideo] = false;
				bSmallVideoFrameStart = false;
				bSmallVideoResumePossible[iStopAndFreeThisVideo] = false;
			}
		}
		iStopAndFreeThisVideo = -1;
		bSmallVideoInit[iVideoEntry] = false;
	}

}

// GGMAX 2.51: called once per frame from GuruLoopLogic (GameGuruMain.cpp), every mode.
// Frees any widget video still PLAYING ~2s after its widget stopped drawing.
void SmallTutorialVideoWatchdog(void)
{
	g_ggTutorialWatchdogFrame++;
	for (int i = 0; i < MAXTUTORIALS; i++)
	{
		if (iSmallVideoSlot[i] > 0 && g_ggTutorialWatchdogFrame - g_ggTutorialEntryHeartbeat[i] > 120)
		{
			// Free stale slots in EVERY state, not just playing: a clip that hit EOF (or was
			// pause-orphaned) leaves the slot occupied — 32 slots, one leak per storyboard
			// visit, so a long session exhausts them and later widgets get no video at all.
			if (AnimationExist(iSmallVideoSlot[i]))
			{
				extern void gg_videotrace(const char* msg);
				char tr[256];
				sprintf(tr, "TutorialVideoWatchdog: freeing orphaned video entry %d (anim slot %d, playing=%d)", i, iSmallVideoSlot[i], AnimationPlaying(iSmallVideoSlot[i]));
				gg_videotrace(tr);
				if (AnimationPlaying(iSmallVideoSlot[i])) StopAnimation(iSmallVideoSlot[i]);
				DeleteAnimation(iSmallVideoSlot[i]);
				iSmallVideoSlot[i] = 0;
				bSmallVideoPerccentStart[i] = false;
				bSmallVideoResumePossible[i] = false;
				bSmallVideoInit[i] = false;
				if (i == iStopAndFreeThisVideo) iStopAndFreeThisVideo = -1;
			}
		}
	}
}

void SmallTutorialVideo(char *tutorial, char* combo_items[], int combo_entries,int iVideoSection, bool bAutoStart)
{
	//LB: added lines to exit early
	if ((bStoryboardWindow && !bProceduralLevel) && iVideoSection != SECTION_STORYBOARD) return;
	if (bProceduralLevel && iVideoSection != SECTION_TERRAIN_GENERATOR) return;
	if (bWelcomeScreen_Window &&  iVideoSection != SECTION_MAX_HUB) return;

	bool bSectionHub = false;
	if (iVideoSection == SECTION_MAX_HUB) bSectionHub = true;

	int iVideoEntry = -1;

	cSmallVideoPath = "";
	cSmallVideoDescription = "";
	int iCurrentVideoEntry = -1;

	static int iOneTimeSetup = true;
	if (iOneTimeSetup)
	{
		iOneTimeSetup = false;
		for (int i = 0; i < MAXTUTORIALS; i++)
		{
			bSmallVideoFirstClick[i] = true;
			iSmallVideoDelayExecute[i] = 0;
		}
	}
	
	if (tutorial_videos.size() > 0)
	{
		int i = 0;
		for (std::map<std::string, std::string>::iterator it = tutorial_videos.begin(); it != tutorial_videos.end(); ++it)
		{
			if (it->first.length() > 0)
			{
				if (strcmp(it->first.c_str(), tutorial) == 0)
				{
					iVideoEntry = i;
					iCurrentVideoEntry = i;
					cSmallVideoPath = it->second.c_str();
					if (cSmallComboSelection[iVideoEntry].Len() > 0) 
					{
						//PE: Overwrite settings.
						int il = 0;
						for (std::map<std::string, std::string>::iterator it = tutorial_videos.begin(); it != tutorial_videos.end(); ++it) 
						{
							if (it->first.length() > 0 && strcmp(it->first.c_str(), cSmallComboSelection[iVideoEntry].Get()) == 0 ) 
							{
								cSmallVideoPath = it->second.c_str();
								iCurrentVideoEntry = il;
								break;
							}
							il++;
						}
					}
					break;
				}
			}
			i++;
		}
	}

	// GGMAX 2.51: this entry's widget is live on screen this frame — feed its watchdog heartbeat
	if (iVideoEntry >= 0 && iVideoEntry < MAXTUTORIALS) g_ggTutorialEntryHeartbeat[iVideoEntry] = g_ggTutorialWatchdogFrame;
	if (iCurrentVideoEntry >= 0 && iCurrentVideoEntry < MAXTUTORIALS) g_ggTutorialEntryHeartbeat[iCurrentVideoEntry] = g_ggTutorialWatchdogFrame;

	if (bSectionHub && iStopAndFreeThisVideo >= 0)
	{
		if (iSmallVideoSlot[iStopAndFreeThisVideo] > 0) 
		{
			if (AnimationExist(iSmallVideoSlot[iStopAndFreeThisVideo])) 
			{
				if (AnimationPlaying(iSmallVideoSlot[iStopAndFreeThisVideo])) StopAnimation(iSmallVideoSlot[iStopAndFreeThisVideo]);
				DeleteAnimation(iSmallVideoSlot[iStopAndFreeThisVideo]);
				iSmallVideoSlot[iStopAndFreeThisVideo] = 0;
				bSmallVideoPerccentStart[iStopAndFreeThisVideo] = false;
				bSmallVideoFrameStart = false;
				bSmallVideoResumePossible[iStopAndFreeThisVideo] = false;
			}
		}
		iStopAndFreeThisVideo = -1;
		bSmallVideoInit[iVideoEntry] = false;
	}

	if (iVideoEntry >= 0 && cSmallVideoPath.Len() > 0 ) 
	{
		//PE: Auto launch maximized , if first time in a section.
		if (iCurrentVideoSectionPlaying > 0 && iVideoSection > 0 && iCurrentVideoSectionPlaying != iVideoSection)
		{
			//PE: Pause any playing video , when changing section.
			for (int i = 0; i < MAXTUTORIALS; i++) 
			{
				if (iSmallVideoSlot[i] > 0) 
				{
					if (AnimationExist(iSmallVideoSlot[i])) 
					{
						if (AnimationPlaying(iSmallVideoSlot[i]))
						{
							PauseAnim(iSmallVideoSlot[i]);
							bSmallVideoResumePossible[i] = false;
							iCurrentVideoSectionPlaying = 0;
						}
					}
				}
			}
		}

		if (iVideoSection > 0 && iVideoSection < 20)
		{
			if (0)
			{
				if (pref.iPlayedVideoSection[iVideoSection] == 0)
				{
					pref.iPlayedVideoSection[iVideoSection] = 1;
					bSmallVideoMaximized[iVideoEntry] = true;
					//PE: Perhaps auto start here ?
				}
			}
		}


		//PE: Only delete one video on first run, if same tutorials use same iVideoEntry.
		if (bSmallVideoFrameStart && !bSmallVideoInit[iVideoEntry])
		{
			if (iSmallVideoSlot[iVideoEntry] > 0) {
				if (AnimationExist(iSmallVideoSlot[iVideoEntry])) {
					if (AnimationPlaying(iSmallVideoSlot[iVideoEntry]))
						StopAnimation(iSmallVideoSlot[iVideoEntry]);
					DeleteAnimation(iSmallVideoSlot[iVideoEntry]);
					iSmallVideoSlot[iVideoEntry] = 0;
					bSmallVideoPerccentStart[iVideoEntry] = false;
					bSmallVideoFrameStart = false;
					bSmallVideoResumePossible[iVideoEntry] = false;
				}
			}
		}

		if (1)
		{
			if (iSmallVideoFindFirstFrame[iVideoEntry] > 0) {
				if (iSmallVideoFindFirstFrame[iVideoEntry] == 1) {
					PauseAnim(iSmallVideoSlot[iVideoEntry]);
					iCurrentVideoSectionPlaying = 0;
					bSmallVideoResumePossible[iVideoEntry] = false;
					SetVideoVolume(100.0);
				}
				iSmallVideoFindFirstFrame[iVideoEntry]--;
			}

			switch (iSmallVideoDelayExecute[iVideoEntry]) {

				case 1: //Play restart
				{
					//PE: We can only start one video per frame.
					if (bSmallVideoFrameStart) {
						bSmallVideoFrameStart = false;
						iSmallVideoDelayExecute[iVideoEntry] = 0;
						SmallTutorialVideoInit(iVideoEntry);
						if (iSmallVideoSlot[iVideoEntry] > 0) {
							StopAnimation(iSmallVideoSlot[iVideoEntry]);
							PlayAnimation(iSmallVideoSlot[iVideoEntry]);
							SetRenderAnimToImage(iSmallVideoSlot[iVideoEntry], true);
							UpdateAllAnimation();
							Sleep(50); //Sleep so we get a video texture in the next call.
							UpdateAllAnimation();
							SetVideoVolume(100.0);
							bSmallVideoResumePossible[iVideoEntry] = false;
							bSmallVideoPerccentStart[iVideoEntry] = true;
							iCurrentVideoSectionPlaying = iVideoSection;
						}
					}
					break;
				}
				case 2: //Resume
				{
					SmallTutorialVideoInit(iVideoEntry);
					if (iSmallVideoSlot[iVideoEntry] > 0) {
						iSmallVideoDelayExecute[iVideoEntry] = 0;
						ResumeAnim(iSmallVideoSlot[iVideoEntry]);
						iCurrentVideoSectionPlaying = iVideoSection;
					}
					break;
				}
				case 3: //Pause
				{
					SmallTutorialVideoInit(iVideoEntry);
					if (iSmallVideoSlot[iVideoEntry] > 0) {
						iSmallVideoDelayExecute[iVideoEntry] = 0;
						PauseAnim(iSmallVideoSlot[iVideoEntry]);
						bSmallVideoResumePossible[iVideoEntry] = true;
						iCurrentVideoSectionPlaying = 0;
					}
					break;
				}
				default:
					break;
			}

			int combo_current_type_selection = 0;

			if (combo_items && combo_entries > 0 ) {
				if (cSmallComboSelection[iVideoEntry].Len() > 0)
				{
					for (int i = 0; i < combo_entries;i++)
					{
						if (combo_items[i])
						{
							if (strcmp(cSmallComboSelection[iVideoEntry].Get(), combo_items[i]) == 0)
							{
								combo_current_type_selection = i;
								break;
							}
						}
					}
				}
			}
			if (combo_items && combo_entries > 0)
			{
				std::map<std::string, std::string>::iterator it = tutorial_description.find(combo_items[combo_current_type_selection]);
				if (it != tutorial_description.end()) {
					cSmallVideoDescription = it->second.c_str();
				}
			}
			else
			{
				std::map<std::string, std::string>::iterator it = tutorial_description.find(tutorial);
				if (it != tutorial_description.end()) {
					cSmallVideoDescription = it->second.c_str();
				}
			}
			//bSmallVideoMaximized[iVideoEntry] = true; //For now.
			bool bMustEndWindow = false;
			if (bSmallVideoMaximized[iVideoEntry]) {
				
				//	Display the maximised tutorial video window.
				ImGui::SetNextWindowSize(ImVec2(62 * ImGui::GetFontSize(), 45 * ImGui::GetFontSize()), ImGuiCond_Once);
				ImGui::SetNextWindowPosCenter(ImGuiCond_Once);

				//LB: ImGui::Begin("Tutorial Video##VideosMaxSize", &bSmallVideoMaximized[iVideoEntry], 0);
				ImGui::Begin("Tutorial Video##VideosMaxSize", &bSmallVideoMaximized[iVideoEntry], ImGuiWindowFlags_ForceRender);
				bMustEndWindow = true;
				bSmallVideoPlayerMaximized = true;
				ImGui::Indent(10);
			}


			if (combo_items && combo_entries > 0 ) 
			{
				//Display combo.
				ImGui::PushItemWidth(-10);
				cstr sUniqueLabel = cstr("##TutorialSimpleInput") + cstr(iVideoEntry);
				if (combo_current_type_selection < combo_entries)
				{
					char *findcur = strstr(combo_items[combo_current_type_selection], "-");
					if (findcur) findcur++;
					else findcur = combo_items[combo_current_type_selection];

					if (ImGui::BeginCombo(sUniqueLabel.Get(), findcur, ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge))
					{
						for (int n = 0; n < combo_entries; n++)
						{
							if (combo_items[n])
							{
								bool is_selected = (combo_current_type_selection == n);
								char *find = strstr(combo_items[n], "-");
								if (find) find++;
								else find = combo_items[n];

								if (ImGui::Selectable(find, is_selected) )
								{
									combo_current_type_selection = n;
									cSmallComboSelection[iVideoEntry] = combo_items[combo_current_type_selection];
									bSmallVideoInit[iVideoEntry] = false;
									bSmallVideoFrameStart = false; //PE: Wait until next frame.
								}
								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();

					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Tutorial Video");
				ImGui::PopItemWidth();
			}

			float fRatio = 1.0f / ((float)GetDesktopWidth() / (float)GetDesktopHeight());

			ID3D11ShaderResourceView* lpVideoTexture = NULL;
			if (bSmallVideoInit[iVideoEntry] )
				lpVideoTexture = GetAnimPointerView(iSmallVideoSlot[iVideoEntry]);
			float fVideoW = GetAnimWidth(iSmallVideoSlot[iVideoEntry]);
			float fVideoH = GetAnimHeight(iSmallVideoSlot[iVideoEntry]);
			if (bSmallVideoInit[iVideoEntry] && iSmallVideoSlot[iVideoEntry] > 0 && lpVideoTexture) {
				fRatio = 1.0f / (fVideoW / fVideoH);
			}

			int iActiveID = iVideoEntry;
			if (iCurrentVideoEntry >= 0 && iCurrentVideoEntry < MAXTUTORIALS && iCurrentVideoEntry != iVideoEntry) iActiveID = iCurrentVideoEntry;

			bool bShowBoder = true;
			float fLeftBorder = 10.0;
			if (bSectionHub) bShowBoder = false;
			if (bSectionHub && !bSmallVideoMaximized[iVideoEntry]) fLeftBorder = 2.0;

			float videoboxheight = (ImGui::GetContentRegionAvail().x - fLeftBorder) * fRatio;

			ImVec4 oldImGuiCol_ChildWindowBg = ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg];
			ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

			//LB: ImGui::BeginChild("Video##TutorialVideo", ImVec2(ImGui::GetContentRegionAvail().x - 10.0, videoboxheight), true, iGenralWindowsFlags);
			ImGui::BeginChild("Video##TutorialVideo", ImVec2(ImGui::GetContentRegionAvail().x - fLeftBorder, videoboxheight), bShowBoder, ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_ForceRender | iGenralWindowsFlags);

			bool bToogleMinMax = false;
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImRect image_bb(window->DC.CursorPos, window->DC.CursorPos + ImGui::GetContentRegionAvail());
			image_bb.Floor();

			bool bIsPlaying = false;
			//PE: We always play into iVideoEntry not iActiveID.
			if (iSmallVideoSlot[iVideoEntry] > 0)
			{
				if (AnimationExist(iSmallVideoSlot[iVideoEntry]) && AnimationPlaying(iSmallVideoSlot[iVideoEntry]))
					bIsPlaying = true;
			}

			bool bVideoAreaPressed = false;

			if (lpVideoTexture) 
			{
				SetRenderAnimToImage(iSmallVideoSlot[iVideoEntry], true);
				float animU = GetAnimU(iSmallVideoSlot[iVideoEntry]);
				float animV = GetAnimV(iSmallVideoSlot[iVideoEntry]);
				ImVec2 uv0 = ImVec2(0, 0);
				ImVec2 uv1 = ImVec2(animU, animV);

				ImGui::PushID(lpVideoTexture);
				const ImGuiID id = window->GetID("#image");
				ImGui::PopID();
				ImGui::ItemSize(image_bb);
				if (ImGui::ItemAdd(image_bb, id))
				{
					window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

					bool hovered, held;
					bVideoAreaPressed = ImGui::ButtonBehavior(image_bb, id, &hovered, &held);
				}

			}
			else 
			{
				// Display thumbnail of the video (from .jpg)

				SmallTutorialThumbLoad(iActiveID);
				ID3D11ShaderResourceView* lpTexture = NULL;
				if (iSmallVideoThumbnail[iActiveID] > 0) lpTexture = GetImagePointerView(iSmallVideoThumbnail[iActiveID]);

				ImGui::PushID(lpTexture);
				const ImGuiID id = window->GetID("#image");
				ImGui::PopID();
				ImGui::ItemSize(image_bb);
				if (ImGui::ItemAdd(image_bb, id))
				{
					if (lpTexture) window->DrawList->AddImage((ImTextureID)lpTexture, image_bb.Min, image_bb.Max, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
					bool hovered, held;
					bVideoAreaPressed = ImGui::ButtonBehavior(image_bb, id, &hovered, &held);
				}

				//Display a play button.
				ImVec2 vOldPos = ImGui::GetCursorPos();
				float fPlayButSize = ImGui::GetContentRegionAvail().x * 0.15;
				float fCenterX = (ImGui::GetContentRegionAvail().x*0.5) - (fPlayButSize*0.5);
				float fCenterY = (videoboxheight*0.5) - (fPlayButSize*0.5);
				ImGui::SetCursorPos(ImVec2(fCenterX, fCenterY));
				ImVec4 vColorFade = { 1.0,1.0,1.0,0.5 };
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(fPlayButSize, fPlayButSize), ImColor(255, 255, 255, 0), drawCol_normal*vColorFade, drawCol_hover*vColorFade, drawCol_Down*vColorFade, -1, 0, 0, 0, false,false,false,false,false, bBoostIconColors))
				{
					bSmallVideoPerccentStart[iVideoEntry] = true;
					bSmallVideoResumePossible[iVideoEntry] = false;
					iSmallVideoDelayExecute[iVideoEntry] = 1; //force play - restart.
					bSmallVideoFrameStart = false; //PE: Wait until next frame.
					if (!bSectionHub && bSmallVideoFirstClick[iVideoEntry])
					{
						bSmallVideoFirstClick[iVideoEntry] = false;
						bSmallVideoMaximized[iVideoEntry] = true;
					}
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Play");

				ImGui::SetCursorPos(vOldPos);
			}

			//PE: Double click trigger an inver, so also invert here.
			if (!bStoryboardWindow && !bProceduralLevel)
			{
				if (bVideoAreaPressed || (ImGui::IsMouseDoubleClicked(0) && ImGui::IsMouseHoveringRect(image_bb.Min, image_bb.Max)))
				{
					//Video pause/play
					if (bIsPlaying)
					{
						//Pause
						bSmallVideoPerccentStart[iVideoEntry] = true;
						iSmallVideoDelayExecute[iVideoEntry] = 3; // pause
						bSmallVideoFrameStart = false; //PE: Wait until next frame.
					}
					else
					{
						//Play
						bSmallVideoPerccentStart[iVideoEntry] = true;
						if (bSmallVideoResumePossible[iVideoEntry]) {
							iSmallVideoDelayExecute[iVideoEntry] = 2; //resume
						}
						else {
							iSmallVideoDelayExecute[iVideoEntry] = 1; //play - restart.
						}
						bSmallVideoFrameStart = false; //PE: Wait until next frame.
						if (!bSectionHub && bSmallVideoFirstClick[iVideoEntry])
						{
							bSmallVideoFirstClick[iVideoEntry] = false;
							bSmallVideoMaximized[iVideoEntry] = true;
						}

					}
				}
			}
			if (ImGui::IsMouseHoveringRect(image_bb.Min, image_bb.Max))
			{
				if (!bStoryboardWindow && !bProceduralLevel )
				{
					if (ImGui::IsMouseDoubleClicked(0) && !bVideoAreaPressed)
					{
						bSmallVideoMaximized[iVideoEntry] = 1 - bSmallVideoMaximized[iVideoEntry];
					}
				}
			}

			ImGui::EndChild();
			ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg] = oldImGuiCol_ChildWindowBg;

			{
				float fdone = GetAnimPercentDone(iSmallVideoSlot[iVideoEntry]) / 100.0f;
				if (!bSmallVideoPerccentStart[iVideoEntry]) fdone = 0.0f;

				ImVec2 rstart = ImGui::GetWindowPos() + ImGui::GetCursorPos();
				ImGui::ProgressBar(fdone, ImVec2(ImGui::GetContentRegionAvail().x - fLeftBorder, 8), "");
				ImVec2 rend = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(ImGui::GetContentRegionAvail().x - fLeftBorder, 0.0);

				if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(rstart, rend))
				{
					float GetVideoDuration();
					void SetVideoPositionPause(float seconds);
					void SetVideoPositionPlay(float seconds);

					ImVec2 mpos = ImGui::GetMousePos() - rstart;
					ImVec2 rwidth = rend - rstart;
					float percent = 100.0 / (rwidth.x / mpos.x);
					float videolength = GetVideoDuration();
					float vpercent = videolength / 100.0f;
					SetVideoPositionPlay((vpercent * percent) );
					ResumeAnim(iSmallVideoSlot[iVideoEntry]);
					bSmallVideoResumePossible[iVideoEntry] = false;
					if (!bSectionHub && bSmallVideoFirstClick[iVideoEntry])
					{
						bSmallVideoFirstClick[iVideoEntry] = false;
						bSmallVideoMaximized[iVideoEntry] = true;
					}
				}

				#define MEDIAICONSIZE 20

				if (bIsPlaying)
				{
					if (ImGui::ImgBtn(MEDIA_PAUSE, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						bSmallVideoPerccentStart[iVideoEntry] = true;
						iSmallVideoDelayExecute[iVideoEntry] = 3; // pause
						bSmallVideoFrameStart = false; //PE: Wait until next frame.
					}
					if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Pause");
				}
				else
				{
					if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						bSmallVideoPerccentStart[iVideoEntry] = true;
						if (bSmallVideoResumePossible[iVideoEntry]) {
							iSmallVideoDelayExecute[iVideoEntry] = 2; //resume
						}
						else {
							if (!bSectionHub && bSmallVideoFirstClick[iVideoEntry])
							{
								bSmallVideoFirstClick[iVideoEntry] = false;
								bSmallVideoMaximized[iVideoEntry] = true;
							}
							iSmallVideoDelayExecute[iVideoEntry] = 1; //play - restart.
						}
						bSmallVideoFrameStart = false; //PE: Wait until next frame.
					}
					if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Play");
				}
				ImGui::SameLine();
				if (ImGui::ImgBtn(MEDIA_REFRESH, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, false,false,false,false,false, bBoostIconColors))
				{
					bSmallVideoPerccentStart[iVideoEntry] = true;
					iSmallVideoDelayExecute[iVideoEntry] = 1; //play - restart.
					bSmallVideoFrameStart = false; //PE: Wait until next frame.
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Restart");

				if (!bSmallVideoMaximized[iVideoEntry])
				{
					ImGui::SameLine();
					if (ImGui::ImgBtn(MEDIA_MAXIMIZE, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, true,false,false,false,false, bBoostIconColors))
					{
						bSmallVideoMaximized[iVideoEntry] = true;
					}
					if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Maximize");
				}
				else
				{
					ImGui::SameLine();
					if (ImGui::ImgBtn(MEDIA_MINIMIZE, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, true,false,false,false,false, bBoostIconColors))
					{
						bSmallVideoMaximized[iVideoEntry] = false;
					}
					if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Minimize");
				}
			}

			if (bMustEndWindow) 
			{

				if (cSmallVideoDescription.Len() > 0) {
					ImGui::Separator();
					ImGui::Text("Description");
					ImGui::TextWrapped(cSmallVideoDescription.Get());
				}

				bImGuiGotFocus = true;
				ImGui::Indent(-10);

				if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
				{
					//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
					ImGui::Text("");
					ImGui::Text("");
				}

				ImGui::End();
			}

		}
	}
	return;
}

// Useful function
UINT OpenURLForGETPOST(LPSTR pServerName, LPSTR* pDataReturned, DWORD* pReturnDataSize, LPSTR pAuthHeader, LPSTR pszPostData, LPSTR pVerb, LPSTR urlWhere)
{
	// create large area to drop reply into
	int i100MB = 102400000;
	*pDataReturned = new char[i100MB];
	memset(*pDataReturned, 0, i100MB);

	UINT iError = 0;
	unsigned int dwDataLength = 0;
	HINTERNET m_hInet = InternetOpenA("InternetConnection", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (m_hInet == NULL)
	{
		iError = GetLastError();
	}
	else
	{
		unsigned short wHTTPType = INTERNET_DEFAULT_HTTPS_PORT;
		HINTERNET m_hInetConnect = InternetConnectA(m_hInet, pServerName, wHTTPType, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (m_hInetConnect == NULL)
		{
			iError = GetLastError();
		}
		else
		{
			int m_iTimeout = 2000;
			InternetSetOption(m_hInetConnect, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&m_iTimeout, sizeof(m_iTimeout));
			HINTERNET hHttpRequest = HttpOpenRequestA(m_hInetConnect, pVerb, urlWhere, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
			if (hHttpRequest == NULL)
			{
				iError = GetLastError();
			}
			else
			{
				HttpAddRequestHeadersA(hHttpRequest, "Content-Type: application/x-www-form-urlencoded", -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
				if (pAuthHeader) HttpAddRequestHeadersA(hHttpRequest, pAuthHeader, -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
				int bSendResult = 0;
				if (pszPostData)
					bSendResult = HttpSendRequest(hHttpRequest, NULL, -1, (void*)(pszPostData), strlen(pszPostData));
				else
					bSendResult = HttpSendRequest(hHttpRequest, NULL, -1, NULL, 0);
				if (bSendResult == 0)
				{
					iError = GetLastError();
				}
				else
				{
					int m_iStatusCode = 0;
					char m_szContentType[150];
					unsigned int dwBufferSize = sizeof(int);
					unsigned int dwHeaderIndex = 0;
					HttpQueryInfo(hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (void*)&m_iStatusCode, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex);
					dwHeaderIndex = 0;
					unsigned int dwContentLength = 0;
					HttpQueryInfo(hHttpRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (void*)&dwContentLength, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex);
					dwHeaderIndex = 0;
					unsigned int ContentTypeLength = 150;
					HttpQueryInfo(hHttpRequest, HTTP_QUERY_CONTENT_TYPE, (void*)m_szContentType, (LPDWORD)&ContentTypeLength, (LPDWORD)&dwHeaderIndex);
					char pBuffer[20000];
					for (;;)
					{
						unsigned int written = 0;
						if (!InternetReadFile(hHttpRequest, (void*)pBuffer, 2000, (LPDWORD)&written))
						{
							// error
						}
						if (written == 0) break;
						if (dwDataLength + written > 102400000) written = 102400000 - dwDataLength;
						memcpy(*pDataReturned + dwDataLength, pBuffer, written);
						dwDataLength = dwDataLength + written;
						if (dwDataLength >= 102400000) break;
					}
					InternetCloseHandle(hHttpRequest);
				}
			}
			InternetCloseHandle(m_hInetConnect);
		}
		InternetCloseHandle(m_hInet);
	}
	if (iError > 0)
	{
		char *szError = 0;
		if (iError > 12000 && iError < 12174)
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("wininet.dll"), iError, 0, (char*)&szError, 0, 0);
		else
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, iError, 0, (char*)&szError, 0, 0);
		if (szError)
		{
			LocalFree(szError);
		}
	}

	// complete
	*pReturnDataSize = dwDataLength;
	return iError;
}

