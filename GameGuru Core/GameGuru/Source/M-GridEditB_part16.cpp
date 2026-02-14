void Welcome_Screen(void)
{
	bool bDisplayAsModal = false; //true;
	bool bStopVideo = false;
	bool bAutoPlayVideo = true;

	static bool bIntroScreenDone = false;
	static bool bResizeWelcome = false;
	static bool bCheckIntroScreenDone = true;

	extern bool bSpecialEditorFromStandalone;
	if (bCheckIntroScreenDone)
	{
		extern bool bSpecialStandalone;
		if (!pref.iDisplayIntroScreen || bSpecialStandalone)
		{
			bIntroScreenDone = true;
		}
		bCheckIntroScreenDone = false;

		void CheckForNewUpdateWicked(void);
		CheckForNewUpdateWicked(); //PE: Check if update process is done, and ask if user like to update.
	}
	if (bRemoveVideoInNextFrame)
	{
		if (iWelcomeVideoID > 0)
		{
			if (AnimationExist(iWelcomeVideoID))
			{
				if (AnimationPlaying(iWelcomeVideoID)) StopAnimation(iWelcomeVideoID);
				DeleteAnimation(iWelcomeVideoID);
				iWelcomeVideoID = 0;
			}
		}
		bRemoveVideoInNextFrame = false;
	}
	extern bool bEnsureIntroVideoIsNotRun;
	if (!bIntroScreenDone && !bSpecialEditorFromStandalone && bEnsureIntroVideoIsNotRun==false)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Window Size control
		ImVec2 WindowSize = ImVec2(54 * ImGui::GetFontSize(), 50 * ImGui::GetFontSize());
		if (viewport->Size.x < 1900)
		{
			//PE: Reduce size on smaller screens.
			WindowSize = ImVec2(42 * ImGui::GetFontSize(), 34 * ImGui::GetFontSize());
		}

		if (refresh_gui_docking == 0)
		{
			// large blank dummy screen
			if (!bDisplayAsModal)
			{
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
				ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
				ImGui::Begin("##DummyWelcome Screen##WelcomeScreenWindow", &bWelcomeScreen_Window, ImGuiWindowFlags_None | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoChangeZOrder | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);
				ImGui::End();
			}

			// regular window for welcome screen
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bool bTmp = true;
			ImGui::Begin("Welcome Screen##WelcomeScreenWindow", &bTmp, 0);
			ImGui::End();
		}
		else if (bWelcomeScreen_Window)
		{
			//PE: Hide everything in the background (wicked 3D stuff).
			ImGuiWindow* toolbarwindow = ImGui::FindWindowByName("Toolbar");
			ImVec4 monitor_col = ImVec4(0, 0, 0, 1.0);
			if (!bDisplayAsModal && toolbarwindow && toolbarwindow->DrawList)
			{
				toolbarwindow->DrawList->PushClipRect(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(0, 40), true);
				ImGuiWindow* stastuswindow = ImGui::FindWindowByName("Statusbar");
				monitor_col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg] * ImVec4(0.8, 0.8, 0.8, 0.8);
				monitor_col.w = 1.0;
				toolbarwindow->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 40), ImGui::GetColorU32(monitor_col));
				if (stastuswindow && stastuswindow->DrawList) stastuswindow->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 40), ImGui::GetColorU32(monitor_col));
				toolbarwindow->DrawList->PopClipRect();
				monitor_col = ImVec4(0, 0, 0, 1.0);
			}
			else
			{
				if (toolbarwindow) toolbarwindow->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
			}
			static int iHideWindowForFrames = 8;
			if (iHideWindowForFrames > 0)
			{
				iHideWindowForFrames--;
				return;
			}
			if (gbWelcomeSystemActive)
			{
				return;
			}

			// welcome screen window
			if (bDisplayAsModal)
			{
				ImGui::OpenPopup("Welcome Screen##WelcomeScreenWindowModal");
			}

			if (!bDisplayAsModal)
			{
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
				ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
				ImGui::Begin("##DummyWelcome Screen##WelcomeScreenWindow", &bWelcomeScreen_Window, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_None | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoChangeZOrder | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);
				ImGui::End();
			}

			// set the window size
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Once);
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);

			ImGui::Begin("Welcome Screen##WelcomeScreenWindow", &bWelcomeScreen_Window, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_None | ImGuiWindowFlags_NoMove  | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);

			ImGui::Indent(10);

			//PE: Now in both modes, so can close welcome and it will load last used project.
			if ( g_iDevToolsOpen >= 2) //PE: Rick - never a back but - !bWelcomeNoBackButton ||
			{
				ImVec2 vCurPos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6.0f));
				float fFontSize = ImGui::GetFontSize();
				int icon_size = ImGui::GetFontSize()*3.0;
				ImVec2 VIconSize = { (float)icon_size, (float)icon_size };
				if (ImGui::ImgBtn(TOOL_GOBACK, VIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					bIntroScreenDone = true;
					bResizeWelcome = true;
					bStopVideo = true;

					//PE: New default to object mode.
					bForceKey = true;
					csForceKey = "o";
				}
				if (ImGui::IsItemHovered() && iSkibFramesBeforeLaunch == 0) ImGui::SetTooltip("%s", "Close");
				ImGui::SetCursorPos(vCurPos);
			}

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6.0f));
			ImGui::Text("");
			ImGui::SetWindowFontScale(2.2);
			ImGui::TextCenter("WELCOME TO GAMEGURU MAX!");
			ImGui::SetWindowFontScale(1.4);
			ImGui::TextCenter("Happy Game Making!");
			ImGui::SetWindowFontScale(1.0);
			ImGui::Text("");
			ImGui::Separator();
			ImGui::Text("");

			//################
			//#### Video. ####
			//################
			ImGui::Indent(10);

			static bool bWelcomeVideoInit = false;
			static int iWelcomeFindFirstFrame = 0;
			static bool bWelcomeResumePossible = false;
			static bool bWelcomeVideoPerccentStart = false;
			static int iWelcomeVideoDelayExecute = 0;

			cstr sWelcomeVideoFile = "tutorialbank\\9901-introduction-video.mp4";

			ImVec4 oldImGuiCol_ChildWindowBg = ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg];
			{
				bool bInit = false;

				switch (iWelcomeVideoDelayExecute)
				{
				case 1: //Play restart
				{
					iWelcomeVideoDelayExecute = 0;
					StopAnimation(iWelcomeVideoID);
					PlayAnimation(iWelcomeVideoID);
					SetRenderAnimToImage(iWelcomeVideoID, true);
					UpdateAllAnimation();
					Sleep(50); //Sleep so we get a video texture in the next call.
					UpdateAllAnimation();
					SetVideoVolume(100.0);
					bWelcomeResumePossible = false;
					break;
				}
				case 2: //Resume
				{
					iWelcomeVideoDelayExecute = 0;
					ResumeAnim(iWelcomeVideoID);
					break;
				}
				case 3: //Pause
				{
					iWelcomeVideoDelayExecute = 0;
					PauseAnim(iWelcomeVideoID);
					bWelcomeResumePossible = true;
					break;
				}
				default:
					break;
				}

				if (iWelcomeFindFirstFrame > 0)
				{
					SetVideoVolume(0.1);
					if (iWelcomeFindFirstFrame == 2)
					{
						PauseAnim(iWelcomeVideoID);
						bWelcomeResumePossible = false;
					}
					//PE: Set back volume delayed or we hear a small amount of noise.
					if (iWelcomeFindFirstFrame == 1)
					{
						SetVideoVolume(100.0);
					}
					iWelcomeFindFirstFrame--;
				}

				if (iWelcomeVideoID > 0)
				{
					if (!AnimationExist(iWelcomeVideoID))
					{
						bInit = true;
					}
				}

				if (!bWelcomeVideoInit || bInit)
				{
					if (iWelcomeVideoID > 0)
					{
						if (AnimationExist(iWelcomeVideoID))
						{
							if (AnimationPlaying(iWelcomeVideoID))
								StopAnimation(iWelcomeVideoID);

							DeleteAnimation(iWelcomeVideoID);
							iWelcomeVideoID = 0;
						}
					}

					iWelcomeVideoID = 0;
					t.text_s = Lower(Right(sWelcomeVideoFile.Get(), 4));
					if (t.text_s == ".ogv" || t.text_s == ".mp4")
					{
						//PE: Use first available.
						iWelcomeVideoID = 32;
						for (int i = 1; i <= 32; i++)
						{
							if (AnimationExist(i) == 0) { iWelcomeVideoID = i; break; }
						}
						if (LoadAnimation(sWelcomeVideoFile.Get(), iWelcomeVideoID, g.videoprecacheframes, g.videodelayedload, 1) == false)
						{
							iWelcomeVideoID = -999;
						}
					}
					if (iWelcomeVideoID > 0)
					{
						if (bAutoPlayVideo)
						{
							bWelcomeVideoPerccentStart = true;
							iWelcomeVideoDelayExecute = 1;
						}
					}
					bWelcomeVideoInit = true;
				}

				float fRatio = 1.0f / ((float)GetDesktopWidth() / (float)GetDesktopHeight());

				float fVideoW = 0;
				float fVideoH = 0;
				float animU = 1.0;
				float animV = 1.0;
				ID3D11ShaderResourceView* lpVideoTexture = NULL;
				if (!(iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID) && AnimationPlaying(iWelcomeVideoID)))
				{
					bool bLogo = true;
					if (bWelcomeResumePossible)
					{
						lpVideoTexture = GetAnimPointerView(iWelcomeVideoID);
						if (lpVideoTexture != NULL)
						{
							fVideoW = GetAnimWidth(iWelcomeVideoID);
							fVideoH = GetAnimHeight(iWelcomeVideoID);
							SetRenderAnimToImage(iWelcomeVideoID, true);
							animU = GetAnimU(iWelcomeVideoID);
							animV = GetAnimV(iWelcomeVideoID);
							bLogo = false;
						}
					}
					if (bLogo)
					{
						lpVideoTexture = GetImagePointerView(WELCOME_VIDEO);
						fVideoW = ImageWidth(WELCOME_VIDEO);
						fVideoH = ImageHeight(WELCOME_VIDEO);
					}
				}
				else
				{
					lpVideoTexture = GetAnimPointerView(iWelcomeVideoID);
					if (lpVideoTexture == NULL)
					{
						lpVideoTexture = GetImagePointerView(WELCOME_VIDEO);
						fVideoW = ImageWidth(WELCOME_VIDEO);
						fVideoH = ImageHeight(WELCOME_VIDEO);
					}
					else
					{
						fVideoW = GetAnimWidth(iWelcomeVideoID);
						fVideoH = GetAnimHeight(iWelcomeVideoID);
						SetRenderAnimToImage(iWelcomeVideoID, true);
						animU = GetAnimU(iWelcomeVideoID);
						animV = GetAnimV(iWelcomeVideoID);
					}
				}
				if (bWelcomeVideoInit && iWelcomeVideoID > 0 && lpVideoTexture)
				{
					fRatio = 1.0f / (fVideoW / fVideoH);
				}

				float videoboxheight = (ImGui::GetContentRegionAvail().x - 20.0) * fRatio;

				oldImGuiCol_ChildWindowBg = ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg];
				ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
				ImGui::BeginChild("Welcome Video##WelcomeVideo", ImVec2(ImGui::GetContentRegionAvail().x - 20.0, videoboxheight), true, iGenralWindowsFlags);
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImRect image_bb(window->DC.CursorPos, window->DC.CursorPos + ImGui::GetContentRegionAvail());
				if (lpVideoTexture) {
					ImVec2 uv0 = ImVec2(0, 0);
					ImVec2 uv1 = ImVec2(animU, animV);
					window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
				}

				if (!(iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID) && AnimationPlaying(iWelcomeVideoID)))
				{
					//Display a play button.
					ImVec2 vOldPos = ImGui::GetCursorPos();
					float fPlayButSize = ImGui::GetContentRegionAvail().x * 0.10;
					float fCenterX = (ImGui::GetContentRegionAvail().x*0.5) - (fPlayButSize*0.5);
					float fCenterY = (videoboxheight*0.5) - (fPlayButSize*0.5);
					ImGui::SetCursorPos(ImVec2(fCenterX, fCenterY));
					ImVec4 vColorFade = { 1.0,1.0,1.0,0.5 };
					if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(fPlayButSize, fPlayButSize), ImColor(255, 255, 255, 0), drawCol_normal*vColorFade, drawCol_hover*vColorFade, drawCol_Down*vColorFade, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						bWelcomeVideoPerccentStart = true;
						if (bWelcomeResumePossible)
						{
							iWelcomeVideoDelayExecute = 2; //resume
						}
						else
						{
							iWelcomeVideoDelayExecute = 1; //play - restart.
						}
					}
					if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Play");

					ImGui::SetCursorPos(vOldPos);
				}

				ImGui::EndChild();
				ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg] = oldImGuiCol_ChildWindowBg;

				if (iWelcomeVideoID > 0)
				{
					if (AnimationExist(iWelcomeVideoID))
					{
						//ImGui::SameLine();
						float fdone = GetAnimPercentDone(iWelcomeVideoID) / 100.0f;
						if (!bWelcomeVideoPerccentStart) fdone = 0.0f;

						static int triggerEndVideo = 0;
						if (fdone > 0.5)
							triggerEndVideo = 1;
						if (bAutoPlayVideo && triggerEndVideo > 0 && (fdone == 0.0f || fdone >= 1.0f))
						{
							//Auto close and goto welcome screen.
							bIntroScreenDone = true;
							bResizeWelcome = true;
							bStopVideo = true;
						}
						ImVec2 rstart = ImGui::GetWindowPos() + ImGui::GetCursorPos();
						ImGui::ProgressBar(fdone, ImVec2(ImGui::GetContentRegionAvail().x - 20, 8), "");
						ImVec2 rend = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(ImGui::GetContentRegionAvail().x - 20.0,0.0);
						if(ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(rstart, rend))
						{
							float GetVideoDuration();
							void SetVideoPositionPause(float seconds);
							void SetVideoPositionPlay(float seconds);

							ImVec2 mpos = ImGui::GetMousePos() - rstart;
							ImVec2 rwidth = rend - rstart;
							float percent = 100.0 / (rwidth.x / mpos.x);
							float videolength = GetVideoDuration();
							float vpercent = videolength / 100.0f;
							SetVideoPositionPlay( (vpercent * percent) * 0.95 ); //PE: Make it a bit lower as this is thread running so before actual pause there is a little playing.
							ResumeAnim(iWelcomeVideoID);
							bWelcomeResumePossible = false;
							triggerEndVideo = 0;
						}
						#define MEDIAICONSIZE 20

						if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
						{
							bWelcomeVideoPerccentStart = true;
							if (bWelcomeResumePossible)
							{
								iWelcomeVideoDelayExecute = 2; //resume
							}
							else
							{
								iWelcomeVideoDelayExecute = 1; //play - restart.
							}
						}
						if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Play");
						ImGui::SameLine();
						if (ImGui::ImgBtn(MEDIA_PAUSE, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
						{
							iWelcomeVideoDelayExecute = 3; // pause
						}
						if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Pause");
						ImGui::SameLine();
						if (ImGui::ImgBtn(MEDIA_REFRESH, ImVec2(MEDIAICONSIZE, MEDIAICONSIZE), ImColor(255, 255, 255, 0), drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
						{
							bWelcomeVideoPerccentStart = true;
							iWelcomeVideoDelayExecute = 1; //play - restart.
						}
						if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Restart");
					}
				}
			}
			ImGui::Indent(-10);

			ImGui::SetWindowFontScale(1.2);

			bool bTmp = 1 - pref.iDisplayIntroScreen;
			float fTextWidth = ImGui::CalcTextSize("Hide intro video next time").x + 20.0f;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvailWidth() - 10.0) * 0.5) - (fTextWidth * 0.5), 0));
			if (ImGui::Checkbox("Hide intro video next time", &bTmp))
			{
				pref.iDisplayIntroScreen = 1 - bTmp;
			}

			//Skip button
			ImGui::SameLine();
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvailWidth() - 180.0), 0.0));
			if (ImGui::StyleButton(" SKIP ", ImVec2(160.0f, 0)))
			{
				bIntroScreenDone = true;
				bResizeWelcome = true;
				bStopVideo = true;
			}

			ImGui::SetWindowFontScale(1.0);

			ImGui::Indent(-10);

			bImGuiGotFocus = true;

			if (bDisplayAsModal)
				ImGui::EndPopup();
			else
				ImGui::End();

			if (!bWelcomeScreen_Window || bStopVideo)
			{
				//PE: Close down everything.
				bWelcomeVideoInit = false;
				bRemoveVideoInNextFrame = true;
			}

		}
		//Intro Video End
	}
	else
	{
		bool bUseFullScreen = true;
		//Welcome Screen Start
		ImVec2 vWindowSize = ImVec2(104 * ImGui::GetFontSize(), 59 * ImGui::GetFontSize());
		float tab_box_height = 630.0f;

		static cstr sCurrentGame = "";
		if (refresh_gui_docking == 0)
		{
			if (!bDisplayAsModal)
			{
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
				ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
				ImGui::Begin("##DummyWelcome Screen##WelcomeScreenWindow", &bWelcomeScreen_Window, ImGuiWindowFlags_None | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoChangeZOrder | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);
				ImGui::End();
			}

			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Always);
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bool bTmp = true;
			ImGui::Begin("Welcome Screen##WelcomeScreenWindow", &bTmp, 0);
			ImGui::End();
		}
		else if (bWelcomeScreen_Window)
		{
			static std::string current_project_selected = "";
			static int current_project_id = -1;
			static int iCurrentOpenTab = 0;

			// Automation harness: expose current tab to external code
			extern int g_iAutoCurrentTab;
			g_iAutoCurrentTab = iCurrentOpenTab;

			static std::string current_tutorial_selected = "";
			static int current_tutorial_id = -1;


			//PE: Hide everything in the background (wicked 3D stuff).
			ImGuiWindow* toolbarwindow = ImGui::FindWindowByName("Toolbar");//ImGui::GetCurrentWindow();
			ImVec4 monitor_col = ImVec4(0, 0, 0, 1.0); //Black for now.
			if (1)
			{
				if (!bDisplayAsModal && toolbarwindow && toolbarwindow->DrawList)
				{
					toolbarwindow->DrawList->PushClipRect(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(0, 40), true);
					ImGuiWindow* stastuswindow = ImGui::FindWindowByName("Statusbar");//ImGui::GetCurrentWindow();
					monitor_col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg] * ImVec4(0.8, 0.8, 0.8, 0.8);
					monitor_col.w = 1.0;
					toolbarwindow->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 40), ImGui::GetColorU32(monitor_col));
					if (stastuswindow && stastuswindow->DrawList) stastuswindow->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 40), ImGui::GetColorU32(monitor_col));
					toolbarwindow->DrawList->PopClipRect();
					monitor_col = ImVec4(0, 0, 0, 1.0); //Black for now.
				}
				else
				{
					if (toolbarwindow) toolbarwindow->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
				}
			}
			GetFilesListForLibrary("tutorialbank\\games\\", true, 0, 1024, 576,1); //Get games list and create thumbs.

			if (!bWelcomeScreen_Init)
			{
				if (g_LibraryFileList.size() > 0)
				{
					//sCurrentGame = g_LibraryFileList[0].cName; //First game seleted by default.
					sCurrentGame = "Escape from the Zombie Cellar.png"; //For now default to zombie cellar demo - level1.png
				}
				bWelcomeScreen_Init = true;
				void CheckForNewUpdateWicked(void);
				CheckForNewUpdateWicked(); //PE: Check if update process is done, and ask if user like to update.
			}

			if (bDisplayAsModal)
				ImGui::OpenPopup("Welcome Screen##WelcomeScreenWindow");


			ImGuiViewport* viewport = ImGui::GetMainViewport();

			//PE: Dummy window to disable background clicks.
			if (!bDisplayAsModal)
			{
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
				ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
				ImGui::Begin("##DummyWelcome Screen##WelcomeScreenWindow", &bWelcomeScreen_Window, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_None | ImGuiWindowFlags_NoBackground  | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoChangeZOrder | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);
				ImGui::End();
			}

			ImGui::SetNextWindowViewport(viewport->ID);

			if (bResizeWelcome)
			{
				ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Always);
			}
			else
			{
				ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
			}
			if (bDisplayAsModal)
				ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			else
			{
				if (bResizeWelcome)
					ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
				else
					ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
			}
			if (bResizeWelcome) bResizeWelcome = false;

			//PE: Test
			if (bUseFullScreen)
			{
				ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
				ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
				if (viewport->Size.x < 1800.0)
				{
					//PE: Cant use fullscreen font sizes.
					bUseFullScreen = false;
				}
			}

			ImGui::Begin("Welcome Screen##WelcomeScreenWindow", &bWelcomeScreen_Window, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_None | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);
			ImGui::Indent(10);

			// basic menu so user can quit and go to settings
			extern void hub_menubar(void);
			hub_menubar();

			//PE: Playing maximized must be on top.
			ImGuiWindow* welcome_window = ImGui::GetCurrentWindow();
			void CheckWindowsOnTop(ImGuiWindow* storyboard_window);
			CheckWindowsOnTop(welcome_window);

			cstr sDisplayName = sCurrentGame;
			char *find = (char *)pestrcasestr(sDisplayName.Get(), ".");
			if (find)
			{
				int iPos = find - sDisplayName.Get();
				if (iPos > 0 && iPos < 1024)
					sDisplayName = Left(sDisplayName.Get(), iPos);
			}

			bool bTriggerRereadDescription = false;
			if (cLastProjectList != "projectbank\\")
			{
				bTriggerRereadDescription = true;
			}
			GetProjectList("projectbank\\", true); //PE: Moved here before any use of images.

			bool bUseProject = false;
			bool bUseTutorial = false;
			if (iCurrentOpenTab == 1 && current_project_selected != "" && current_project_id >= 0)
			{
				bUseProject = true;
			}
			if (iCurrentOpenTab == 3 && current_tutorial_selected != "" && current_tutorial_id >= 0)
			{
				bUseTutorial = true;
			}

			//PE: Moved this after swap , need to be moved to before next "if (bUseTutorial)" , if we swap around again.
			if (!bUseTutorial)
			{
				if (current_tutorial_id >= 0)
				{
					iStopAndFreeThisVideo = current_tutorial_id;
					cstr title = "";
					std::map<std::string, std::string>::iterator it;
					int iFind = 0;
					for (it = tutorial_description.begin(); it != tutorial_description.end(); ++it)
					{
						if (current_tutorial_id == iFind) break;
						iFind++;
					}
					if (it != tutorial_description.end())
					{
						title = it->first.c_str();
						void SmallTutorialVideoCheckStop(char *tutorial);
						SmallTutorialVideoCheckStop(title.Get());
					}

					current_tutorial_id = -1;
				}
			}

			/*
			ID3D11ShaderResourceView* lpTexture = GetImagePointerView(WELCOME_HEADER);
			ImVec2 vHeaderDim = { 1200.0f,150.0f };
			if (iWelcomeHeaderType == 3) vHeaderDim = { 1500.0f,150.0f };
			if (lpTexture)
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				//PE: Right align header, with black background.
				ImVec2 header_pos = ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x- vHeaderDim.x, 0.0);
				window->DrawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos()+ImVec2(ImGui::GetWindowSize().x,150.0f), ImGui::GetColorU32(monitor_col));
				if (iWelcomeHeaderType == 2)
				{
					ImVec2 header_pos_left = ImGui::GetWindowPos() + ImVec2(70, 0);
					window->DrawList->AddImage((ImTextureID)lpTexture, header_pos_left, header_pos_left + vHeaderDim, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.9)));
				}
				else if (iWelcomeHeaderType == 3)
				{
					ImVec2 header_pos_center = ImGui::GetWindowPos() + ImVec2((ImGui::GetWindowSize().x*0.5) - (vHeaderDim.x*0.5), 0.0);
					window->DrawList->AddImage((ImTextureID)lpTexture, header_pos_center, header_pos_center + vHeaderDim, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
				}
				else
				{
					window->DrawList->AddImage((ImTextureID)lpTexture, header_pos, header_pos + vHeaderDim, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.9)));
				}
			}
			*/
			int preview_size_x = ImGui::GetMainViewport()->Size.x;
			float fStartWinPosY = ImGui::GetCursorPosY();
			ImGui::SetWindowFontScale(1.0);
			ImVec2 vIconSize = { (float)ImGui::GetFontSize() * 3.5f, (float)ImGui::GetFontSize() * 3.5f };
			ImVec2 vHeaderDim = { (float)preview_size_x, vIconSize.y };
			float fHeaderHeight = vHeaderDim.y;
			ImVec2 header_pos = ImVec2(0.0, fStartWinPosY + 14.0f);
			ID3D11ShaderResourceView* lpTexture;
			lpTexture = GetImagePointerView(WELCOME_HEADER);
			if (lpTexture)
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				window->DrawList->AddImage((ImTextureID)lpTexture, header_pos, header_pos + vHeaderDim + ImVec2(0, 8), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
			}
			ImGui::SetCursorPos(ImVec2(4.0f, fStartWinPosY - 1.0f));
			ImGui::SetItemAllowOverlap();
			if (ImGui::ImgBtn(TOOL_GOEXIT, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
			{
				// exit to desktop
				int iAction = askBoxCancel("Are you sure you would like to exit to desktop?", "Confirmation"); //1==Yes 2=Cancel 0=No
				if (iAction == 1)
				{
					g_bCascadeQuitFlag = true;
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit to Desktop"); //Welcome Screen

			ImVec2 vCurPos = ImGui::GetCursorPos();
			ImGui::SetWindowFontScale(3.5);
			ImGui::SetCursorPos(ImVec2(4.0f, fStartWinPosY - 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 1.0, 0.25));
			ImGui::TextCenter("H U B");
			ImGui::PopStyleColor();
			ImGui::SetWindowFontScale(1.0);
			float fFontSize = ImGui::GetFontSize();
			ImGui::SetCursorPos(vCurPos);

			#ifdef HUBOLDFEEDBACKBOX
			ImRect rect;
			rect.Min = ImVec2(1067,36);
			rect.Max = rect.Min + ImVec2(600, 100);
			if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
			{
				char pTip[256];
				sprintf(pTip, "%s : Help us improve with your feedback by clicking here", g.version_s.Get());
				ImGui::SetTooltip("%s", pTip);
				if (ImGui::IsAnyMouseDown() == true)
				{
					ExecuteFile("https://github.com/TheGameCreators/GameGuruRepo/wiki/Help-Us-Improve-with-your-Feedback", "", "", 0);
				}
			}
			#endif

			/* exit to desktop now always present
			// Display a button that allows the user to exit the welcome screen window.
			// Store the previous cursor position so that it can be reset (adding the below button at the start of the window causes it to appear faded and i'm not sure why).
			ImVec2 vPrevCursorPos(ImGui::GetCursorPos());
			ImGui::SetCursorPos(ImVec2(12.0f, 10.0f));
			if (g_iDevToolsOpen >= 2) //PE: Rick - never a back but - !bWelcomeNoBackButton ||
			{
				if (ImGui::ImgBtn(TOOL_GOBACK, ImVec2(ImGui::GetFontSize() * 4.0f, ImGui::GetFontSize() * 4.0f), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(180, 180, 180, 180),
					ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					bWelcomeScreen_Window = false;
					if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Exit to Hub");
			}
			// Reset the cursor position to not interfere with the rest of the menu.
			ImGui::SetCursorPos(vPrevCursorPos);
			*/

			float fButWidth = 150.0f;

			float fTotalContentWidth = ImGui::GetContentRegionAvailWidth();

			ImGui::Columns(2, "WelcomeScreencolumns2", false);  //false no border

			//PE: swap.
			ImGui::SetColumnOffset(0, 0.0f);
			ImGui::SetColumnOffset(1, fTotalContentWidth * 0.6); //Right 40% size.


			//#############################
			//######## SWAP Column ########
			//#############################

			ImGui::Text("");
			//ImGui::Text("");

			if (bUseFullScreen || viewport->Size.x < 1800.0)
				tab_box_height = ImGui::GetWindowSize().y - 82.0f - ImGui::GetCursorPosY(); //76.0 , 72.0

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiContext& gui = *GImGui;
			ImVec2 TabStartPos = window->DC.CursorPos;

			ImGui::SetWindowFontScale(1.2);
			if (ImGui::BeginTabBar("welcomescreentabbar"))
			{
				int tabflags = 0;

				// Automation harness: force tab selection
				extern int g_iAutoForceWelcomeTab;
				int autoForceTab = g_iAutoForceWelcomeTab;
				if (autoForceTab >= 0) g_iAutoForceWelcomeTab = -1; // consume the request

				ImRect rect;
				rect.Min = TabStartPos;
				rect.Max = rect.Min + ImGui::TabItemCalcSize(" Demo Games ", false);
				TabStartPos.x += ImGui::TabItemCalcSize(" Demo Games ", false).x + gui.Style.ItemInnerSpacing.x;

				if (ImGui::BeginTabItem(" Demo Games ", NULL, (autoForceTab == 0) ? ImGuiTabItemFlags_SetSelected : tabflags))
				{
					iCurrentOpenTab = 0;

					//if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Demo Games you can edit and play");

					if (g_LibraryFileList.size() > 0)
					{

						ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 3.0, ImGui::GetCursorPosY() + 6.0));

						ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3.0));

						ImGui::SetWindowFontScale(1.0);
						cstr cRet = ListboxFilesListForWelcomeScreen_v2(sCurrentGame.Get(), 3, -1, true, false, NULL, tab_box_height, false, bUseFullScreen);
						if (cRet != sCurrentGame)
						{
							//Game changed.
							sCurrentGame = cRet;
						}
					}

					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
						//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}
					ImGui::EndTabItem();
				}
				if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "Demo Games you can edit and play");

				ImGui::SetWindowFontScale(1.2);

				rect.Min = TabStartPos;
				rect.Max = rect.Min + ImGui::TabItemCalcSize(" My Games ", false);
				TabStartPos.x += ImGui::TabItemCalcSize(" My Games ", false).x + gui.Style.ItemInnerSpacing.x;

				static bool bCheckForAnyProjectFiles = true;
				int tabflagsMyGames = 0;

				if (bCheckForAnyProjectFiles)
				{
					extern bool bReturnToWelcome;
					if (projectbank_list.size() > 0 && !bReturnToWelcome)
					{
						tabflagsMyGames = ImGuiTabItemFlags_SetSelected;
					}
					bCheckForAnyProjectFiles = false;
				}
				static bool bShowAvtiveProject = true;
				if (autoForceTab == 1) tabflagsMyGames = ImGuiTabItemFlags_SetSelected;
				if (ImGui::BeginTabItem(" My Games ", NULL, tabflagsMyGames))
				{
					ImGui::SetWindowFontScale(0.99f);

					ImGui::Indent(10);
					ImVec2 cpos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(cpos + ImVec2(770.0f, 2.0f));
					if (ImGui::Checkbox("Active/InActive", &bShowAvtiveProject))
					{
					}
					ImGui::SameLine();

					ImGui::SetCursorPos(cpos + ImVec2(905.0f, 2.0f));

					ImGui::Text("Sort Projects: ");
					ImGui::SameLine();
					//ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -2.0f));

					const char* pProjectSortModes[] = { "Most Recent", "Least Recent", "A-Z", "Z-A" };
					int iProjectSortMode = pref.iProjectSortMode;

					int comboflags = ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
					ImGui::PushItemWidth(110);
					if (ImGui::BeginCombo("##ComboProjectSort", pProjectSortModes[iProjectSortMode], comboflags))
					{
						for (int i = 0; i < 4; i++)
						{
							bool is_selected = (iProjectSortMode == i);
							if (ImGui::Selectable(pProjectSortModes[i], is_selected)) 
							{
								if (i != iProjectSortMode)
								{
									pref.iProjectSortMode = iProjectSortMode = i;
									SortProjects(iProjectSortMode);
									bResetProjectThumbnails = true;
									break;
								}
							}
						}

						ImGui::EndCombo();
					}

					ImGui::Indent(-10);
					ImGui::SetWindowFontScale(1.0f);
					ImGui::PopItemWidth();
					
					iCurrentOpenTab = 1;
					//My Games.
					bool bTriggerLoad = false;

					ImGui::SetWindowFontScale(1.0);
					ImGui::BeginChild("##MyGamesProjectsForWelcome", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, tab_box_height), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
					ImGui::Indent(2);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6));
					float total_width = ImGui::GetContentRegionAvailWidth();
					ImGui::Columns(3, "MyGamesProjectsForWelcomecolumns", false);  //false no border

					float colwidth = ImGui::GetContentRegionAvailWidth(); //padding.
					float fRatio = colwidth / 512.0f;
					ImVec2 iThumbSize = { (float)512.0*fRatio, (float)288.0*fRatio };
					int iCount = projectbank_list.size() + 1;
					if (iCount < 9) iCount = 9; //PE: Always display min. 9 empty slots.
					for (int i = 0; i < iCount; i++)
					{
						if (i >= projectbank_list.size())
						{
							if (i == projectbank_list.size())
							{
								ImVec2 cursorPos = ImGui::GetCursorPos();
								int iTextureID = WELCOME_FILLERROUNDED;
								ImVec4 alpha = { 1.0f,1.0f,1.0f,0.15f };
								if(ImGui::ImgBtn(iTextureID, ImVec2(iThumbSize.x, iThumbSize.y + 36.0), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255)*alpha, ImColor(255, 255, 255, 255)*alpha,
									ImColor(255, 255, 255, 255)*alpha, 0, 0, 0, 0, false, false, false, false, true))
								{
									// The first available empty slot should also serve as a Create new game project button.
									strcpy(pref.cLastUsedStoryboardProject, "");
									bStoryboardInitNodes = false; //Just init again.
									bStoryboardFirstRunSetInitPos = false;
									process_storeboard(true); //Init a new project.
									//PE: Bug - When creating a new project , it would contain g_collectionList from prev. loaded project.
									init_rpg_system();
									bTriggerSaveAsAfterNewLevel = true;
									bTriggerSaveAs = true;
									strcpy(SaveProjectAsName, "");
									strcpy(SaveProjectAsError, "");
								}
								ImVec2 buttonSize(iThumbSize.x / 4.f, iThumbSize.y / 4.f + 36.0f);
								ImGui::SetCursorPos(cursorPos + ImVec2(iThumbSize.x / 2.f - 3.35f, iThumbSize.y / 2.f + 18.f - 3.35f) - buttonSize / 2);
								if (ImGui::ImgBtn(KEY_SEPARATOR, buttonSize, ImColor(255.0f, 255.0f, 255.0f, 0.0f), ImColor(220, 220, 220, 220),
									ImColor(220, 220, 220, 180), ImColor(220, 220, 220, 150), 0, 0, 0, 0, false, false, false, false, true))
								{

								}
								if (ImGui::IsItemHovered())ImGui::SetTooltip("Create a new game project");
							}
							else
							{
								//PE: Filler.
								int iTextureID = WELCOME_FILLERROUNDED;
								ImVec4 alpha = { 1.0f,1.0f,1.0f,0.15f };
								if (ImGui::ImgBtn(iTextureID, ImVec2(iThumbSize.x, iThumbSize.y + 36.0), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255)*alpha, ImColor(255, 255, 255, 200)*alpha, ImColor(255, 255, 255, 200)*alpha, 0, 0, 0, 0, false, false, false, false, true))
								{
									//
								}
							}

							
							ImGui::NextColumn();
						}
						else
						{
							if (!ImageExist(projectbank_imageid[i]) || bResetProjectThumbnails) //STORYBOARD_THUMBS + 800 + i))
							{
								if (bResetProjectThumbnails)
								{
									bResetProjectThumbnails = false;

									// After sorting, the thumbnails can be out of order but not safe to delete them before here.
									for (int i = 0; i < projectbank_imageid.size(); i++)
									{
										if (ImageExist(STORYBOARD_THUMBS + 800 + i)) DeleteImage(STORYBOARD_THUMBS + 800 + i);
									}
									GetProjectThumbnails();
								}

								if (projectbank_imageid[i] != BOX_CLICK_HERE)
								{
									if (ImageExist(STORYBOARD_THUMBS + 800 + i)) DeleteImage(STORYBOARD_THUMBS + 800 + i);
								}
								if (projectbank_image[i] == "")
								{
									//Click Here.
									projectbank_imageid[i] = BOX_CLICK_HERE;
								}
								else
								{
									//PE: Load in thumb.
									image_setlegacyimageloading(true);
									LoadImageSize((char *)projectbank_image[i].c_str(), STORYBOARD_THUMBS + 800 + i, 512, 288);
									image_setlegacyimageloading(false);
									if (!ImageExist(STORYBOARD_THUMBS + 800 + i))
									{
										//Fail click here.
										projectbank_imageid[i] = BOX_CLICK_HERE;
									}
									else
									{
										projectbank_imageid[i] = STORYBOARD_THUMBS + 800 + i;
									}
								}
							}
							bool bValid = false;
							if (bShowAvtiveProject && projectbank_active[i])
								bValid = true;
							if (!bShowAvtiveProject && !projectbank_active[i])
								bValid = true;
							if (bSortProjects)
								bValid = false;

							if (bValid && !pestrcasestr((char *)projectbank_list[i].c_str(), "_backup_"))
							{
								ImGui::PushID(564231 + i);
								int TextureID = BOX_CLICK_HERE;
								if (ImageExist(projectbank_imageid[i])) TextureID = projectbank_imageid[i];

								if (current_project_selected == projectbank_list[i])
								{
									//PE: Highlight by name.
									current_project_id = i; //PE: If adding new project, make sure current_project_id match name.
									if (1)
									{
										ImGuiWindow* window = ImGui::GetCurrentWindow();
										ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
										ImVec2 padding = { 1.0, 1.0 };
										const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + iThumbSize);
										window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
									}

								}
								if (ImGui::ImgBtn(TextureID, iThumbSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false, false, true))
								{
									current_project_selected = projectbank_list[i];
									current_project_id = i;
								}
								ImGui::PopID();

								if (ImGui::IsItemHovered())
								{
									if (ImGui::IsMouseDoubleClicked(0))
									{
										current_project_selected = projectbank_list[i];
										current_project_id = i;
										bTriggerLoad = true;
									}
								}
								ImGui::SetWindowFontScale(1.4);
								if (ImGui::StyleButton(projectbank_list[i].c_str(), ImVec2(colwidth, 0.0f)))
								{
									current_project_selected = projectbank_list[i];
									current_project_id = i;
									//bTriggerLoad = true;
								}
								//PE: Need pencil for edit.
								ImGui::SameLine();
								ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(-35.0, 3.0));
								ImGui::SetItemAllowOverlap();
								ImGui::PushID(464231 + i);
								if (ImGui::ImgBtn(TOOL_PENCIL, ImVec2(22, 22), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false, false, true))
								{
									current_project_selected = projectbank_list[i];
									current_project_id = i;
									bTriggerLoad = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Edit The Game Project");
								ImGui::PopID();

								ImGui::SetWindowFontScale(1.0);
								ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 8));
								ImGui::NextColumn();

							}
						}
					}

					// Sort the projects initially.
					if (bSortProjects)
					{
						bSortProjects = false;
						SortProjects(iProjectSortMode);
						bResetProjectThumbnails = true;
					}

					//PE: Always have a selection
					//LB: moved down so can benefit from above sort call
					if (!bResetProjectThumbnails && current_project_selected == "" && projectbank_list.size() > 0)
					{
						//projectbank_active[i]
						for (int i = 0; i < projectbank_active.size(); i++)
						{
							if (projectbank_active[i])
							{
								current_project_id = i;
								current_project_selected = projectbank_list[i];
								break;
							}
						}
						if (current_project_selected == "")
						{
							current_project_id = 0;
							current_project_selected = projectbank_list[0];
						}
					}

					//PE: No trigger load here, moved to other column.
					if (bTriggerLoad)
					{
						//Load and start storyboard.
						TriggerLoadGameProject = current_project_selected.c_str();
						bWelcomeScreen_Window = false;
						bStoryboardWindow = true;
						if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
					}

					ImGui::Columns(1);
					ImGui::Indent(-2);

					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
					{
						//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}

					ImGui::EndChild();

					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
					{
						//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}
					ImGui::EndTabItem();
				}
				if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "My Games");

				if (1) //Disable all other tabs.
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				}

				ImGui::SetWindowFontScale(1.2);
				ImGui::PopItemFlag(); //PE: Enable this tab.

				// TRUSTED UDER ACCOUNT ID
				bool bOnlyTrustedSteamUsersForNow = false;
				#ifndef GGMAXEPIC
				uint64 uAccountID = 0;
				#else
				int uAccountID = 0;
				#endif
				#ifndef GGMAXEDU
				if (SteamUGC())
				{
					// some concerns raised in the community of TGCs association with potential piracy - one bad apple and all that - sorry everyone!
					uAccountID = SteamUser()->GetSteamID().GetAccountID();
					if (uAccountID == 58134713 || uAccountID == 6704278)
					{
						// basically just means Steve :)
						bOnlyTrustedSteamUsersForNow = true;
					}
				}
				#endif

				// User Guide
				rect.Min = TabStartPos;
				rect.Max = rect.Min + ImGui::TabItemCalcSize(" User Guide ", false);
				TabStartPos.x += ImGui::TabItemCalcSize(" User Guide ", false).x + gui.Style.ItemInnerSpacing.x;
				if (ImGui::BeginTabItem(" User Guide ", NULL, (autoForceTab == 4) ? ImGuiTabItemFlags_SetSelected : tabflags))
				{
					iCurrentOpenTab = 4;
					ImGui::Text("");
					ImGui::SetWindowFontScale(2.0);
					ImGui::TextCenter("GameGuru MAX User Guide");
					ImGui::SetWindowFontScale(1.0);
					ImGui::Text("");
					ImGui::Text("");
					float descwidth = ImGui::GetContentRegionAvail().x * 0.6f;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (descwidth * 0.5), 0));

					float oldChildBorderSize = gui.Style.ChildBorderSize;
					gui.Style.ChildBorderSize = 10.0;
					ImVec2 oldFramePadding = gui.Style.FramePadding;
					gui.Style.FramePadding = ImVec2(10.0, 10.0);
					float guide_height = tab_box_height - 380.0;
					if (guide_height < 375.0) guide_height = 375.0;
					ImGui::BeginChild("##MyUserGuideForWelcome", ImVec2(descwidth - 5.0, guide_height), true, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
					ImGui::Indent(10);
					ImGui::Text("");
					ImGui::SetWindowFontScale(2.0);
					ImGui::TextWrapped("The GameGuru MAX user guide is a comprehensive guide to the software.");
					ImGui::Text("");
					ImGui::TextWrapped("Provided as a simple PDF file, you can easily find the advice and help you are looking for.");
					ImGui::Text("");
					ImGui::TextWrapped("We hope this user guide offers the best advice to new and experienced game developers.");
					ImGui::SetWindowFontScale(1.0);
					ImGui::Text("");
					ImGui::Indent(-10);
					ImGui::EndChild();

					gui.Style.ChildBorderSize = oldChildBorderSize;
					gui.Style.FramePadding = oldFramePadding;
					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
					{
						//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}

					ImGui::EndTabItem();
				}
				if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
				{
					if (uAccountID > 0)
					{
						char pExtraUserGuideTip[256];
						sprintf(pExtraUserGuideTip, "User Guide (ID:%d)", uAccountID);
						ImGui::SetTooltip("%s", pExtraUserGuideTip);
					}
					else
					{
						ImGui::SetTooltip("%s", "User Guide");
					}
				}
				ImGui::SetWindowFontScale(1.2);

				// Tutorials
				rect.Min = TabStartPos;
				rect.Max = rect.Min + ImGui::TabItemCalcSize(" Tutorials ", false);
				TabStartPos.x += ImGui::TabItemCalcSize(" Tutorials ", false).x + gui.Style.ItemInnerSpacing.x;
				if (ImGui::BeginTabItem(" Tutorials ", NULL, (autoForceTab == 3) ? ImGuiTabItemFlags_SetSelected : tabflags))
				{
					char* tutorial_order[] = { "9901-introduction-video.mp4", "0101-getting-started.mp4", "0701-game-storyboard.mp4", "0501-terrain-generator.mp4", "0502-terrain-height-maps.mp4", "0201-level-editor.mp4", "0202-particle-editor.mp4", "0203-animation-library.mp4", "0301-object-library.mp4" , "0801-character-creator.mp4" ,"0601-terrain-editing.mp4", "0401-objects-grouping.mp4", "0901-behaviour-ai.mp4", "0902-behaviour-demos-1.mp4",  "0903-behaviour-demos-2.mp4" };

					iCurrentOpenTab = 3;

					ImGui::SetWindowFontScale(1.0);
					ImGui::BeginChild("##MyGamesTutorialsForWelcome", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, tab_box_height), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
					ImGui::Indent(2);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6));
					float total_width = ImGui::GetContentRegionAvailWidth();
					ImGui::Columns(3, "MyGamesTutorialsForWelcomecolumns", false);  //false no border

					float colwidth = ImGui::GetContentRegionAvailWidth(); //padding.
					float fRatio = colwidth / 512.0f;
					ImVec2 iThumbSize = { (float)512.0*fRatio, (float)288.0*fRatio };
					int old_current_tutorial_id = current_tutorial_id;
					for (int i = 0; i < ARRAYSIZE(tutorial_order); i++)
					{
						int iTutorialID = -1;
						cstr cTitle = "";

						//PE: Find next tutorial.
						if (tutorial_videos.size() > 0)
						{
							int il = 0;
							for (std::map<std::string, std::string>::iterator it = tutorial_videos.begin(); it != tutorial_videos.end(); ++it)
							{
								if (it->first.length() > 0)
								{
									if (strstr(it->second.c_str(), tutorial_order[i]) != 0)
									{
										cTitle = it->first.c_str();
										cSmallVideoPath = it->second.c_str();
										iTutorialID = il;
										break;
									}
								}
								il++;
							}
						}

						if (iTutorialID >= 0)
						{
							//PE: Always have a selection.
							if (current_tutorial_selected == "")
							{
								current_tutorial_id = iTutorialID;
								current_tutorial_selected = tutorial_order[i];
							}

							SmallTutorialThumbLoad(iTutorialID);

							if (ImageExist(iSmallVideoThumbnail[iTutorialID]))
							{
								ImGui::PushID(565231 + i);
								int TextureID = iSmallVideoThumbnail[iTutorialID];

								if (current_tutorial_selected == tutorial_order[i])
								{
									//PE: Highlight by name.
									current_tutorial_id = iTutorialID;
									if (1)
									{
										ImGuiWindow* window = ImGui::GetCurrentWindow();
										ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
										ImVec2 padding = { 1.0, 1.0 };
										const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + iThumbSize);
										window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
									}

								}
								if (ImGui::ImgBtn(TextureID, iThumbSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false, false, true))
								{
									current_tutorial_selected = tutorial_order[i];
									current_tutorial_id = iTutorialID;
									if (old_current_tutorial_id >= 0)
									{
										iStopAndFreeThisVideo = old_current_tutorial_id;
									}

								}
								ImGui::PopID();

								ImGui::SetWindowFontScale(1.4);

								char *find = strstr(cTitle.Get(), "-");
								if (find) find++;
								if (find && find[0] == ' ') find++;
								else find = cTitle.Get();

								if (ImGui::StyleButton(find, ImVec2(colwidth, 0.0f)))
								{
									if (old_current_tutorial_id >= 0)
									{
										iStopAndFreeThisVideo = old_current_tutorial_id;
									}

									current_tutorial_selected = tutorial_order[i];
									current_tutorial_id = iTutorialID;
								}

								ImGui::SetWindowFontScale(1.0);
								ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 8));
								ImGui::NextColumn();

							}
						}
					}
					ImGui::Columns(1);
					ImGui::Indent(-2);
					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
					{
						//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}
					ImGui::EndChild();
					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
					{
						//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}
					ImGui::EndTabItem();
				}
				if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "Tutorials");
				ImGui::SetWindowFontScale(1.2);

				#ifndef GGMAXEDU
				//
				// Community Tutorials
				//
				rect.Min = TabStartPos;
				rect.Max = rect.Min + ImGui::TabItemCalcSize(" Community Tutorials ", false);
				TabStartPos.x += ImGui::TabItemCalcSize(" Community Tutorials ", false).x + gui.Style.ItemInnerSpacing.x;
				if (ImGui::BeginTabItem(" Community Tutorials ", NULL, (autoForceTab == 42) ? ImGuiTabItemFlags_SetSelected : tabflags))
				{
					ImGui::Text("");
					ImGui::SetWindowFontScale(2.0);
					ImGui::TextCenter("GameGuru MAX Community Tutorials and Videos");
					ImGui::SetWindowFontScale(1.0);
					ImGui::Text("");
					ImGui::Text("");

					const char* items_commtut_header[] = { "Blood Moon Interactive", "Plemsoft" };
					const char* items_commtut_desc[] = {
						"Welcome to Blood Moon Interactive, the ultimate destination for GameGuru Max enthusiasts and aspiring game developers",
						"Find out about all the amazing things Preben has created for the community"
					};
					const char* items_commtut_link[] = {
						"https://www.youtube.com/@bloodmooninteractive",
						"https://www.youtube.com/@MakingGames"
					};
					const int items_commtut_thumb[] = { HUB_COMMTUT1, HUB_COMMTUT2 };

					iCurrentOpenTab = 42;  // Life, The Universe and Everything

					ImGui::SetWindowFontScale(1.0);
					ImVec2 vWidthOfCommtutArea = ImVec2(ImGui::GetContentRegionAvail().x - 2.0, tab_box_height);
					float fIntendAmount = 0.0f;
					ImGui::Indent(fIntendAmount);
					ImGui::BeginChild("##CommunityTutorialsForWelcome", vWidthOfCommtutArea, false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
					ImGui::Indent(2);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6));
					float total_width = ImGui::GetContentRegionAvailWidth();
					ImGui::Columns(3, "CommunityTutorialsForWelcomecolumns", false);  //false no border

					float colwidth = ImGui::GetContentRegionAvailWidth(); //padding.
					float fRatio = colwidth / 512.0f;
					ImVec2 iThumbSize = { (float)512.0 * fRatio, (float)288.0 * fRatio };

					char child[MAX_PATH];
					for (int i = 0; i < IM_ARRAYSIZE(items_commtut_thumb); i++)
					{
						sprintf(child, "##CommunityTutorialsFrame%d", i);
						ImGui::BeginChild(child, ImVec2(iThumbSize.x - 8.0, iThumbSize.x - 60.0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

						ImGui::SetWindowFontScale(1.6);
						ImGui::TextCenter(items_commtut_header[i]);
						ImGui::SetWindowFontScale(1.0);

						int TextureID = items_commtut_thumb[i];
						if (!ImageExist(TextureID))
						{
							TextureID = BOX_CLICK_HERE;
						}

						ImGui::PushID(554231 + i);
						float wthumb = iThumbSize.x - 120.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvailWidth() * 0.5) - (wthumb * 0.5) - 3.0, 0.0f));
						if (ImGui::ImgBtn(TextureID, ImVec2(wthumb, wthumb), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false, false, true))
						{
							ExecuteFile((LPSTR)items_commtut_link[i], "", "", 0);
						}
						ImGui::PopID();
						ImGui::EndChild();
						ImGui::SetWindowFontScale(1.2);
						ImGui::TextWrapped(items_commtut_desc[i]);
						ImGui::SetWindowFontScale(1.0);
						ImGui::SetWindowFontScale(1.0);
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 8));
						ImGui::NextColumn();
					}
					ImGui::Columns(1);
					ImGui::Indent(-2);
					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0)
					{
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}
					ImGui::EndChild();
					ImGui::Indent(-fIntendAmount);
					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0)
					{
						ImGui::Text("");
						ImGui::Text("");
						ImGui::Text("");
					}
					ImGui::EndTabItem();
				}
				if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "Community Tutorials and Videos");
				ImGui::SetWindowFontScale(1.2);

				//
				// Live Change Log
				//
				rect.Min = TabStartPos;
				rect.Max = rect.Min + ImGui::TabItemCalcSize(" Live Changelog ", false);
				TabStartPos.x += ImGui::TabItemCalcSize(" Live Changelog ", false).x + gui.Style.ItemInnerSpacing.x;
				if (ImGui::BeginTabItem(" Live Changelog ", NULL, (autoForceTab == 5) ? ImGuiTabItemFlags_SetSelected : tabflags))
				{
					iCurrentOpenTab = 5;
					ImGui::Text("");
					ImGui::SetWindowFontScale(2.0);
					char pBuildText[1024];
					sprintf(pBuildText, "%s", g.version_s.Get());
					ImGui::TextCenter(pBuildText);
					ImGui::SetWindowFontScale(1.5);
					ImGui::TextCenter("Showing All Recent Commits");
					ImGui::TextCenter("");
					ImGui::SetWindowFontScale(1.3);

					float fImageWidth = ImGui::GetContentRegionAvailWidth() - 10.0f;
					float fImageRatio = fImageWidth / 512.0f;
					float box_height = (((float)288.0 * fImageRatio)) + 10.0f;
					ImGui::Indent(2);
					ImGui::BeginChild("##LiveChangeLogScrollView", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, box_height), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
		
					// test GITHUB 
					static std::vector<cstr> changeLogList;
					static int iCheckChangeLogOncePerLaunch = 0;
					if (iCheckChangeLogOncePerLaunch == 0)
					{
						// changeloglist
						iCheckChangeLogOncePerLaunch = 1;
						changeLogList.clear();

						// call API to get featured items in list
						char pDataReturned[200000];
						char pDatatmp[10240];
						memset(pDataReturned, 0, sizeof(pDataReturned));
						memset(pDatatmp, 0, sizeof(pDatatmp));
						DWORD dwDataReturnedSize = 0;
						char cUrl[10240];
						sprintf(cUrl, "repos/Dark-Basic-Software-Limited/GameGuruMAX/commits?&per_page=100");

						// access features list from store server
						UINT iError = StoreOpenURLForDataOrFile("api.github.com", pDataReturned, &dwDataReturnedSize, "", "GET", cUrl, NULL);
						if (iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0)
						{
							char pEnsureCombineDates[1024];
							LPSTR pDataReturnedPtr = pDataReturned;
							bool bFindAllMessagesContainingCommitNotes = true;
							while (bFindAllMessagesContainingCommitNotes)
							{
								// find any preceding date entry
								char pPrevDateEntry[1024];
								strcpy(pPrevDateEntry, "");
								LPSTR pDateToken = strstr(pDataReturnedPtr, "\"date\":");
								if (pDateToken)
								{
									pDateToken += 8;
									LPSTR pEndDateToken = strstr(pDateToken, "\"");
									if (pEndDateToken)
									{
										*pEndDateToken = 0;
										strcpy(pPrevDateEntry, pDateToken);
										pPrevDateEntry[strlen("XXXX-XX-XX")] = 0;
										pDataReturnedPtr = pEndDateToken + 1;
									}
								}

								// now get following message
								LPSTR pMessageToken = strstr(pDataReturnedPtr, "\"message\":");
								if (pMessageToken)
								{
									if (strlen(pPrevDateEntry) > 0)
									{
										if (strcmp(pEnsureCombineDates, pPrevDateEntry) != NULL)
										{
											// only add date stamp if unique in log
											changeLogList.push_back(cstr(pPrevDateEntry) + ":");
											strcpy(pEnsureCombineDates, pPrevDateEntry);
										}
									}

									pMessageToken += 11;
									LPSTR pEndToken = strstr(pMessageToken, "\"");
									if (pEndToken)
									{
										*pEndToken = 0;
										LPSTR pTitleFound = strstr(pMessageToken, "\\n\\n");
										if (pTitleFound) pMessageToken = pTitleFound + 4;
										bool bMultipleLines = true;
										while(bMultipleLines)
										{
											LPSTR pNewLineFound = strstr(pMessageToken, "\\n*");
											if (pNewLineFound)
											{
												*pNewLineFound = 0;
												if (*pMessageToken != '*')
													sprintf(pDatatmp, "* %s", pMessageToken);
												else
													sprintf(pDatatmp, "%s", pMessageToken);
												changeLogList.push_back(pDatatmp);
												pMessageToken = pNewLineFound + 2;
											}
											else
											{
												bMultipleLines = false;
											}
										}

										if ( *pMessageToken != '*' )
											sprintf(pDatatmp, "* %s", pMessageToken);
										else
											sprintf(pDatatmp, "%s", pMessageToken);
										changeLogList.push_back(pDatatmp);

										pDataReturnedPtr = pEndToken + 1;
									}
									else
									{
										// also a cut off signal!
										bFindAllMessagesContainingCommitNotes = false;
									}
								}
								else
								{
									bFindAllMessagesContainingCommitNotes = false;
								}
							}
						}
					}
					if (changeLogList.size() > 0)
					{
						for (int i = 0; i < changeLogList.size(); i++)
						{
							ImGui::TextWrapped(changeLogList[i].Get());
						}
					}
					else
					{
						ImGui::SetWindowFontScale(1.5);
						ImGui::TextWrapped("No changelog available.");
					}
					ImGui::Indent(-2);
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
				if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "View the changelog direct from the GitHub Repository");
				ImGui::SetWindowFontScale(1.2);

				//
				// Workshop Uploader and Workshop Viewer
				//
				if (g_bWorkshopAvailable == true && bOnlyTrustedSteamUsersForNow == true)
				{
					// For Workshop Uploader Trusted Users
					rect.Min = TabStartPos;
					rect.Max = rect.Min + ImGui::TabItemCalcSize(" Workshop Uploader ", false);
					TabStartPos.x += ImGui::TabItemCalcSize(" Workshop Uploader ", false).x + gui.Style.ItemInnerSpacing.x;
					if (ImGui::BeginTabItem(" Workshop Uploader ", NULL, (autoForceTab == 6) ? ImGuiTabItemFlags_SetSelected : tabflags))
					{
						iCurrentOpenTab = 6;
						ImGui::Text("");
						ImGui::SetWindowFontScale(2.0);
						ImGui::TextCenter("Your Uploaded Workshop Items");
						ImGui::SetWindowFontScale(1.0);
						ImGui::Text("");
						ImGui::Text("");

						if (SteamUGC())
						{
							if (g_bStillDownloadingThings == false)
							{
								// show list of existing workshop items as buttons
								ImGui::SetWindowFontScale(1.0);
								ImGui::BeginChild("##MyOwnWorkshopItems", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, tab_box_height - 250.0f), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
								float half_total_width = ImGui::GetContentRegionAvailWidth() / 2.0f;
								ImGui::Indent(half_total_width / 2.0f);
								ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6));
								for (int i = 0; i < g_workshopItemsList.size(); i++)
								{
									if (_atoi64(g_workshopItemsList[i].sSteamUserAccountID.Get()) == SteamUser()->GetSteamID().GetAccountID())
									{
										char pWorkshipItemName[MAX_PATH];
										sprintf(pWorkshipItemName, g_workshopItemsList[i].sName.Get());
										if (ImGui::StyleButton(pWorkshipItemName, ImVec2(half_total_width, 0)))
										{
											// select existing workshop item to edit
											g_currentWorkshopItem = g_workshopItemsList[i];
											g_iCurrentMediaTypeForWorkshopItem = workshop_getvaluefromtype(g_currentWorkshopItem.sMediaType.Get());
											g_iSelectedExistingWorkshopItem = i;
											extern int g_iIconImageInProperties;
											g_iIconImageInProperties = 0;
										}
										if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to edit this workshop item and submit an update");
									}
								}

								// show "add new item" button to start creating a new one
								if (ImGui::StyleButton("Add New Item", ImVec2(half_total_width, 0)))
								{
									// select new workshop item to create
									g_iSelectedExistingWorkshopItem = -1;
									workshop_new_item();
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to start a new workshop item submission of your own creation");

								// end of main Workshop tab page
								ImGui::Indent(-half_total_width / 2.0f);
								ImGui::EndChild();

								// Instructions for Workshop Item
								ImGui::SetWindowFontScale(1.25);
								ImGui::Text("");
								ImGui::TextCenter("From here you can add and edit your own workshop items for submission to the Steam Workshop Community");
								ImGui::TextCenter("in the form of game ready assets for the Asset Libraries and can only add and edit items when you");
								ImGui::TextCenter("are logged into your Steam client account and you agree to the workshop terms of service.");
								ImGui::Indent(half_total_width / 2.0f);
								ImGui::Text("");
								if (ImGui::StyleButton("Steam Workshop Terms Of Service", ImVec2(half_total_width, 0)))
								{
									ExecuteFile("https://steamcommunity.com/sharedfiles/workshoplegalagreement", "", "", false);
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Go to the Steam Workshop Terms of Service page");
								ImGui::Indent(-half_total_width / 2.0f);
							}
							else
							{
								ImGui::Text("");
								ImGui::SetWindowFontScale(1.25);
								ImGui::TextCenter("Steam Client is currently downloading workshop items...");
								ImGui::Text("");
							}
						}
						else
						{
							ImGui::Text("");
							ImGui::SetWindowFontScale(1.25);
							ImGui::TextCenter("You must log into your Steam Client Account in order to submit Workshop items");
							ImGui::Text("");
						}
						ImGui::EndTabItem();
					}
					if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "The Workshop Item uploader enables game assets to be submitted to the Steam Workshop Community");
				}
				if (g_bWorkshopAvailable == true)
				{
					// For Workshop Viewer
					extern bool g_bWorkshopTabOpen;
					rect.Min = TabStartPos;
					rect.Max = rect.Min + ImGui::TabItemCalcSize(" Workshop ", false);
					TabStartPos.x += ImGui::TabItemCalcSize(" Workshop ", false).x + gui.Style.ItemInnerSpacing.x;
					if (ImGui::BeginTabItem(" Workshop ", NULL, (autoForceTab == 7) ? ImGuiTabItemFlags_SetSelected : tabflags))
					{
						iCurrentOpenTab = 7;
						g_bWorkshopTabOpen = true;
						ImGui::Text("");
						ImGui::SetWindowFontScale(2.0);
						ImGui::TextCenter("Your Workshop Items");
						ImGui::SetWindowFontScale(1.0);
						ImGui::Text("");
						ImGui::Text("");
						if (SteamUGC())
						{
							extern bool g_bUpdateWorkshopDownloads;
							if (g_bStillDownloadingThingsWithDelay == true)
							{
								extern std::vector<cstr> g_sStillDownloadingLog;
								extern int g_iStillDownloadingLogCount;
								if (MAXTimer() > g_iStillDownloadingThingsWithDelayTimer + 1000 && g_sStillDownloadingLog.size() == 0 )
								{
									g_bStillDownloadingThingsWithDelay = false;
									g_iStillDownloadingLogCount = 0;
								}
							}
							if (g_bStillDownloadingThingsWithDelay == false && g_bUpdateWorkshopDownloads == false && g_bUpdateWorkshopItemList == false )
							{
								// show list of existing workshop items as buttons
								ImGui::SetWindowFontScale(1.0);
								ImGui::BeginChild("##MyOwnWorkshopLineItems", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, tab_box_height - 250.0f), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
								float half_total_width = ImGui::GetContentRegionAvailWidth() / 2.0f;
								ImGui::Indent(half_total_width / 2.0f);
								ImVec2 alignment = ImGui::GetCursorPos();
								ImGui::Text("Title");
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(alignment.x, ImGui::GetCursorPos().y) + ImVec2(400, 0));
								ImGui::Text("Files");
								ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(alignment.x, ImGui::GetCursorPos().y) + ImVec2(490, 0));
								ImGui::Text("Date");
								ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 6));
								for (int i = 0; i < g_workshopItemsList.size(); i++)
								{
									char p64BitNumber[_MAX_U64TOSTR_BASE10_COUNT];
									_ui64toa(g_workshopItemsList[i].nPublishedFileId, p64BitNumber, 10);
									char pWorkshipItemLine[MAX_PATH];
									sprintf(pWorkshipItemLine, "%s : %s", p64BitNumber, g_workshopItemsList[i].sName.Get());
									ImGui::Text(pWorkshipItemLine);
									ImGui::SameLine();
									ImGui::SetCursorPos(ImVec2(alignment.x,ImGui::GetCursorPos().y) + ImVec2(400,0));
									ImGui::Text(cstr(g_workshopItemsList[i].iNumberOfFilesInWorkshopItem).Get());
									ImGui::SameLine();
									ImGui::SetCursorPos(ImVec2(alignment.x, ImGui::GetCursorPos().y) + ImVec2(490, 0));
									ImGui::Text(cstr(g_workshopItemsList[i].sLatestDateOfItem).Get());
								}

								// show "add new item" button to start creating a new one
								ImGui::Text("");
								extern bool g_bRequireRestartAfterUnsubByForce;
								if (g_bRequireRestartAfterUnsubByForce==false)
								{
									if (ImGui::StyleButton("Update Workshop Items", ImVec2(half_total_width, 0)))
									{
										// delete local files copy so can get new items from Steam
										extern bool g_bUpdateWorkshopDownloadsAlwaysPerformOnce;
										g_bUpdateWorkshopDownloadsAlwaysPerformOnce = true;
										g_bStillDownloadingThings = true;
										g_bStillDownloadingThingsWithDelay = true;
										g_iStillDownloadingThingsWithDelayTimer = MAXTimer();
										g_bUpdateWorkshopItemList = true;
										g_iUnsubscribeByForce = 1;
										g_bRequireRestartAfterUnsubByForce = true;
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to refresh all trusted workshop items (NOTE: Allow the system some time to auto-update after using this button)");
								}
								else
								{
									if (g_iUnsubscribeByForce > 0)
									{
										ImGui::TextCenter("PERFORMING FULL REFRESH OF TRUSTED WORKSHOP ITEMS");
									}
									else
									{
										ImGui::TextCenter("RELAUNCH GAMEGURU MAX TO START WORKSHOP ITEMS REFRESH");
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Unsubscribing and resubscribing allows for a complete workshop items refresh");
								}

								// end of main Workshop tab page
								ImGui::Indent(-half_total_width / 2.0f);
								ImGui::EndChild();

								// Instructions for Workshop Item
								ImGui::SetWindowFontScale(1.25);
								ImGui::Text("");
								ImGui::TextCenter("From here you can view all the workshop items you are subscribed to in the Steam Workshop Community");
								ImGui::TextCenter("in the form of game ready assets for the Asset Libraries and can only view and receive updates for");
								ImGui::TextCenter("these when you are logged into your Steam client account.");
							}
							else
							{
								ImGui::Text("");
								ImGui::SetWindowFontScale(1.25);
								extern std::vector<cstr> g_sStillDownloadingLog;
								extern std::vector<cstr> g_sStillDownloadingLogTitle;
								if (g_sStillDownloadingLog.size() > 0)
								{
									ImGui::TextCenter("...currently downloading workshop items...");
									ImGui::Text("");
									static int iPauseOnEachInMS = 10;
									extern int g_iStillDownloadingLogCount;
									if (g_sStillDownloadingLog.size() > g_iStillDownloadingLogCount)
									{
										g_iStillDownloadingLogCount = g_sStillDownloadingLog.size();
										if (g_iStillDownloadingLogCount > 100)
											iPauseOnEachInMS = 10;
										else
										{
											if (g_iStillDownloadingLogCount > 10)
												iPauseOnEachInMS = 100;
											else
												iPauseOnEachInMS = 500;
										}
									}
									int iLastOne = g_sStillDownloadingLog.size() - 1;
									float fProgress = ((float)g_iStillDownloadingLogCount - (float)g_sStillDownloadingLog.size()) / (float)g_iStillDownloadingLogCount;
									if (fProgress > 1.0f) fProgress = 1.0f;
									int iTotalWidthOfArea = ImGui::GetContentRegionAvail().x;
									ImGui::Indent((iTotalWidthOfArea - 500) / 2.0f);
									ImGui::TextCenter(g_sStillDownloadingLogTitle[iLastOne].Get());
									ImGui::ProgressBar(fProgress, ImVec2(500, 26), g_sStillDownloadingLog[iLastOne].Get());
									ImGui::Indent(-((iTotalWidthOfArea - 500) / 2.0f));
									if (g_bStillDownloadingThingsWithDelay == true)
									{
										if (MAXTimer() > g_iStillDownloadingThingsWithDelayTimer + iPauseOnEachInMS)
										{
											g_iStillDownloadingThingsWithDelayTimer = MAXTimer();
											g_sStillDownloadingLog.pop_back();
											g_sStillDownloadingLogTitle.pop_back();
										}
									}
								}
								else
								{
									ImGui::TextCenter("...currently checking workshop items...");
									ImGui::Text("");
								}
								ImGui::Text("");
							}
						}
						else
						{
							ImGui::Text("");
							ImGui::SetWindowFontScale(1.25);
							ImGui::TextCenter("You must log into your Steam Client Account in order to view your Workshop items");
							ImGui::Text("");
						}							
						ImGui::EndTabItem();
					}
					else
						g_bWorkshopTabOpen = false;
					if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max)) ImGui::SetTooltip("%s", "The Workshop Area shows all the workshop items you are manually and automatically subscribed to");
				}
				#endif

				// end of all tabs
				ImGui::EndTabBar();
			}
			ImGui::SetWindowFontScale(1.0);

			//#############################
			//######## SWAP Column ########
			//#############################

			ImGui::NextColumn();
			ImGui::Text("");

			float fContentWidth = ImGui::GetContentRegionAvailWidth() - 10.0f;
			//static float WelcomeFrameHeight = ImGui::GetWindowSize().y - 82.0f - ImGui::GetCursorPosY(); //76.0 , 72.0
			float WelcomeFrameHeight = ImGui::GetWindowSize().y - 82.0f - ImGui::GetCursorPosY(); //76.0 , 72.0
			ImGui::BeginChild("##JustaFrameWelcome", ImVec2(fContentWidth+4.0, WelcomeFrameHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
			ImGui::Indent(2);
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3.0));

			float fRatio = 288.0f / 512.0f;
			float right_margin = 9.0;
			ImVec2 vPreviewSize = { (fContentWidth - right_margin) , (fContentWidth - right_margin) * fRatio };

			ImVec2 winsize = ImGui::GetWindowSize();
			if (vPreviewSize.y >= winsize.y - 72.0f - 48.0f)
			{
				if (iCurrentOpenTab == 0 || iCurrentOpenTab == 1)
				{
					//PE: Must center image after this.
					float fHRatio = 512.0f / 288.0f;
					float maxheight = winsize.y - 72.0f - 48.0f; //buttons + new project bar + small description.
					vPreviewSize = { maxheight * fHRatio , maxheight };
				}
			}

			float fImageWidth = 460;
			float fImageHeight = 215;
			
			if (iCurrentOpenTab == 4)
			{
				int iTextureID = HUB_USERGUIDE;
				if (!ImageExist(iTextureID)) iTextureID = WELCOME_FILLERROUNDED;

				ImGui::SetWindowFontScale(1.0);
				if (iTextureID > 0)
				{
					if (ImGui::ImgBtn(iTextureID, ImVec2(vPreviewSize.x, vPreviewSize.x + 17.0f), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
					{
						// User guide has been moved to offline only
						ExecuteFile("..\\Guides\\User Manual\\GameGuru MAX - User Guide.pdf", "", "", 0);
					}
				}
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.2);

			}
			else if (iCurrentOpenTab == 5)
			{
				float image_size_sub_x = 310.0;
				if (vPreviewSize.x - image_size_sub_x < 250.0) image_size_sub_x += vPreviewSize.x - image_size_sub_x- 250.0;
				int iTextureID = HUB_DISCORD;// HUB_WEBSITE;
				if (!ImageExist(iTextureID)) iTextureID = WELCOME_FILLERROUNDED;
				float ratio = 394.0 / 700.0;
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.0);
				if (iTextureID > 0)
				{
					ImGui::SetWindowFontScale(2.0);
					ImGui::TextCenter("Official Discord");
					ImGui::SetWindowFontScale(1.0);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x*0.5, 0.0));
					if (ImGui::ImgBtn(iTextureID, ImVec2(vPreviewSize.x - image_size_sub_x, (vPreviewSize.x- image_size_sub_x) * ratio), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
					{
						// this link set to never expire!
						ExecuteFile("https://discord.gg/3SnMj3WKDB", "", "", 0);
					}
					ImGui::SetWindowFontScale(1.4);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					ImGui::SetWindowFontScale(1.0);
					ImGui::Text("");
				}

				iTextureID = HUB_LIVEBROADCAST;
				if (!ImageExist(iTextureID)) iTextureID = WELCOME_FILLERROUNDED;

				ImGui::SetWindowFontScale(1.4);
				cstr desc = "Visit our Discord Channel!";
				{
					ImGui::TextCenter(desc.Get());
					ImGui::Text("");
				}

				ImGui::SetWindowFontScale(2.0);
				ImGui::TextCenter("Official Broadcasts and Videos");
				ImGui::SetWindowFontScale(1.0);
				if (iTextureID > 0)
				{
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x*0.5, 0.0));
					if (ImGui::ImgBtn(iTextureID, ImVec2(vPreviewSize.x - image_size_sub_x, (vPreviewSize.x - image_size_sub_x) * ratio), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
					{
						ExecuteFile("https://bit.ly/MAXYouTubeChannel", "", "", 0);
					}
				}

				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				cstr descforYT = "Visit our YouTube Channel!";
				{
					ImGui::TextCenter(descforYT.Get());
					ImGui::Text("");
				}
				ImGui::SetWindowFontScale(1.2);
			}
			#ifndef GGMAXEDU
			else if (iCurrentOpenTab == 6)
			{
				float image_size_sub_x = 350;
				if (vPreviewSize.x - image_size_sub_x < 250.0) image_size_sub_x += vPreviewSize.x - image_size_sub_x - 250.0;
				float ratio = 1.0f;
				ImGui::SetWindowFontScale(1.0);
				if (1)
				{
					// title
					ImGui::SetWindowFontScale(1.4);
					ImGui::TextCenter("Workshop Item Uploader");
					ImGui::Text("");
					float element_overall_width = vPreviewSize.x - image_size_sub_x;

					// image
					ImGui::SetWindowFontScale(1.0);
					ImGui::TextCenter("Workshop Item Image");
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					int iTextureID = HUB_WORKSHOPITEM;
					extern int g_iIconImageInProperties;
					if (g_iIconImageInProperties == 0)
					{
						g_iIconImageInProperties = g.iconimagebankoffset;
						if (GetImageExistEx(g_iIconImageInProperties) == 1) DeleteImage(g_iIconImageInProperties);
						if (FileExist(g_currentWorkshopItem.sImage.Get()) == 1)
						{
							// actual icon image
							image_setlegacyimageloading(true);
							LoadImage(g_currentWorkshopItem.sImage.Get(), g_iIconImageInProperties);
							image_setlegacyimageloading(false);
						}
						else
						{
							// specified image not found, use placeholder
							g_iIconImageInProperties = HUB_WORKSHOPITEM;
							g_currentWorkshopItem.sImage = "";
						}
					}
					else
					{
						// check if preview image is changed externally
						if (g_currentWorkshopItem.sImage.Len() == 0 && g_iIconImageInProperties != HUB_WORKSHOPITEM)
						{
							g_iIconImageInProperties = HUB_WORKSHOPITEM;
						}

						// use default or specified preview image
						iTextureID = g_iIconImageInProperties;
					}
					if (ImGui::ImgBtn(iTextureID, ImVec2(element_overall_width, element_overall_width * ratio), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
					{
						// clicking image itself does nothing (we can absorb button functionality below if we need more item details space)
					}
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					ImGui::SetWindowFontScale(1.4);
					cstr UniqueWorkshopItemPreviewImage = "##UniqueWorkshopItemPreviewImage";
					if (iSelectedLibraryStingReturnID == window->GetID(UniqueWorkshopItemPreviewImage.Get()))
					{
						g_currentWorkshopItem.sImage = sSelectedLibrarySting;
						sSelectedLibrarySting = "";
						iSelectedLibraryStingReturnID = -1;
						g_iIconImageInProperties = 0; // force a reload
					}
					if (ImGui::StyleButton("Change Workshop Item Image", ImVec2(element_overall_width, 0)))
					{
						// open file requester to specify non-default workshop item preview image
						sStartLibrarySearchString = "Image";
						bExternal_Entities_Window = true;
						iDisplayLibraryType = 2; //Image
						iLibraryStingReturnToID = window->GetID(UniqueWorkshopItemPreviewImage.Get());
					}
					static char pEntry[MAX_PATH] = "\0";

					// Name
					ImGui::SetWindowFontScale(1.0);
					ImGui::TextCenter("Workshop Item Name");
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					ImGui::PushItemWidth(element_overall_width);
					strcpy(pEntry, g_currentWorkshopItem.sName.Get());
					if (ImGui::InputText("##WorkshopItemName", pEntry, MAX_PATH, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						g_currentWorkshopItem.sName = pEntry;
					}
					ImGui::PopItemWidth();

					// Description
					ImGui::SetWindowFontScale(1.0);
					ImGui::TextCenter("Workshop Item Description");
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					ImGui::PushItemWidth(element_overall_width);
					strcpy(pEntry, g_currentWorkshopItem.sDesc.Get());
					if (ImGui::InputText("##WorkshopItemDesc", pEntry, MAX_PATH, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						g_currentWorkshopItem.sDesc = pEntry;
					}
					ImGui::PopItemWidth();

					// Disable type and folder for when item is only being updated
					bool bDisableTypeAndFolder = false;
					if (g_iSelectedExistingWorkshopItem != -1)
					{
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7, 0.7, 0.7, 1));
						bDisableTypeAndFolder = true;
					}

					// Media for Item
					ImGui::SetWindowFontScale(1.0);
					ImGui::TextCenter("Workshop Item Media Folder");
					extern int g_iCurrentMediaTypeForWorkshopItem;
					int iRememberOldSetting = g_iCurrentMediaTypeForWorkshopItem;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					ImGui::RadioButton("Audio", &g_iCurrentMediaTypeForWorkshopItem, 1); ImGui::SameLine();
					ImGui::RadioButton("Objects", &g_iCurrentMediaTypeForWorkshopItem, 2); ImGui::SameLine();
					ImGui::RadioButton("Images", &g_iCurrentMediaTypeForWorkshopItem, 3); ImGui::SameLine();
					ImGui::RadioButton("Particles", &g_iCurrentMediaTypeForWorkshopItem, 4); ImGui::SameLine();
					ImGui::RadioButton("Scripts", &g_iCurrentMediaTypeForWorkshopItem, 5); ImGui::SameLine();
					ImGui::RadioButton("Root", &g_iCurrentMediaTypeForWorkshopItem, 6);
					if (bDisableTypeAndFolder == true) g_iCurrentMediaTypeForWorkshopItem = iRememberOldSetting;
					g_currentWorkshopItem.sMediaType = workshop_getmediatypepath(g_iCurrentMediaTypeForWorkshopItem);

					// Media Sub Folder for Item
					ImGui::PushItemWidth(element_overall_width);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					strcpy(pEntry, g_currentWorkshopItem.sMediaFolder.Get());
					int flagforfolder = ImGuiInputTextFlags_EnterReturnsTrue;
					if (bDisableTypeAndFolder == true) flagforfolder |= ImGuiInputTextFlags_ReadOnly;
					if (ImGui::InputText("##WorkshopItemMediaFolder", pEntry, MAX_PATH, flagforfolder))
					{
						g_currentWorkshopItem.sMediaFolder = pEntry;
					}
					if (bDisableTypeAndFolder == true)
					{
						// restore any disabled gadgets
						ImGui::PopStyleColor();
						ImGui::PopStyleVar();
					}
					if (ImGui::IsItemHovered())
					{
						if(bDisableTypeAndFolder==true)
							ImGui::SetTooltip("You can only set the media type and folder when creating a new workshop item");
						else
							ImGui::SetTooltip("First create your media folder in the community folder indicated below, then enter that media folder name here");
					}
					ImGui::PopItemWidth();

					// Show user where their community folder is located
					static char pWorkshopItemMedia[256];
					sprintf(pWorkshopItemMedia, "");
					uint32 uAccountID = 0;
					if (SteamUser())
					{
						uAccountID = SteamUser()->GetSteamID().GetAccountID();
						LPSTR pMediaTypePath = workshop_getmediatypepath(g_iCurrentMediaTypeForWorkshopItem);
						if (stricmp(pMediaTypePath, "root") == NULL)
							sprintf(pWorkshopItemMedia, "Your Root Folder: 'GameGuruMAX\\%s'", g_currentWorkshopItem.sMediaFolder.Get());
						else
							sprintf(pWorkshopItemMedia, "Your Community Folder: '%s\\Community\\%d\\'", pMediaTypePath, uAccountID);
					}
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 128, 128));
					ImGui::TextCenter(pWorkshopItemMedia, MAX_PATH, ImGuiInputTextFlags_None);
					ImGui::PopStyleColor(ImGuiCol_Text);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("When a user views your folder, they will see your Steam Name and not your Steam ID in the UI");

					// progress and user prompt
					extern void workshop_update(bool);
					workshop_update(true);
					ImGui::SetWindowFontScale(1.25);
					ImGui::Text("");
					ImGui::TextCenter(g_WorkshopUserPrompt.Get());
					ImGui::Text("");
				}
				ImGui::SetWindowFontScale(1.2);
			}
			else if (iCurrentOpenTab == 7)
			{
				// ensure steam callbacks are handled
				extern void workshop_update(bool);
				workshop_update(true);

				// Workshop description (right side panel)
				float image_size_sub_x = 310.0;
				if (vPreviewSize.x - image_size_sub_x < 250.0) image_size_sub_x += vPreviewSize.x - image_size_sub_x - 250.0;
				int iTextureID = HUB_WEBSITE;
				if (!ImageExist(iTextureID)) iTextureID = WELCOME_FILLERROUNDED;
				float ratio = 394.0 / 700.0;
				ImGui::SetWindowFontScale(1.0);
				if (iTextureID > 0)
				{
					ImGui::SetWindowFontScale(1.6);
					ImGui::TextCenter("GameGuru MAX Workshop Community");
					ImGui::TextCenter("");
					ImGui::SetWindowFontScale(1.2);
					ImGui::TextCenter("A place to discover new game assets for GameGuru MAX, created");
					ImGui::TextCenter("by the community. Simply subscribe to a workshop item you like");
					ImGui::TextCenter("and the assets contained will be added to your Library.");
				}
				ImGui::SetWindowFontScale(1.2);
			}
			else if (iCurrentOpenTab == 42)
			{
				// Community Tutorials and Videos (right side panel)
				float image_size_sub_x = 310.0;
				if (vPreviewSize.x - image_size_sub_x < 250.0) image_size_sub_x += vPreviewSize.x - image_size_sub_x - 250.0;
				int iTextureID = HUB_WEBSITE;
				if (!ImageExist(iTextureID)) iTextureID = WELCOME_FILLERROUNDED;
				float ratio = 394.0 / 700.0;
				ImGui::SetWindowFontScale(1.0);
				if (iTextureID > 0)
				{
					ImGui::SetWindowFontScale(1.6);
					ImGui::TextCenter("Community News");
					ImGui::TextCenter("");
					ImGui::SetWindowFontScale(1.2);
					ImGui::TextCenter("You can discover the latest official news about GameGuru MAX");
					ImGui::TextCenter("on the News page of the GameGuru MAX website, and for the hottest");
					ImGui::TextCenter("news there is no better place that our DISCORD channel.");
					ImGui::TextCenter("");

					float image_size_sub_x = 310.0;
					if (vPreviewSize.x - image_size_sub_x < 250.0) image_size_sub_x += vPreviewSize.x - image_size_sub_x - 250.0;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(image_size_sub_x * 0.5, 0.0));
					if (ImGui::StyleButton("Join the community on DISCORD", ImVec2(vPreviewSize.x - image_size_sub_x, 0)))
					{
						ExecuteFile("https://bit.ly/MAX_Discord", "", "", 0);
					}
				}
				ImGui::SetWindowFontScale(1.2);
			}
			#endif
			else if (bUseTutorial)
			{
				cstr title = "";
				cstr description = "Testing description.";

				std::map<std::string, std::string>::iterator it;
				int iFind = 0;
				for (it = tutorial_description.begin(); it != tutorial_description.end(); ++it)
				{
					if (current_tutorial_id == iFind) break;
					iFind++;
				}
				if (it != tutorial_description.end()) {
					title = it->first.c_str();
					description = it->second.c_str();
				}
				//ImGui::Indent(10);
				//SmallTutorialVideo(cShowTutorial.Get(), my_combo_itemsp, my_combo_items, iVideoSection);
				SmallTutorialVideo(title.Get(), NULL, 0, SECTION_MAX_HUB);

				//ImGui::Indent(-10);
				if (bLastSmallVideoPlayerMaximized)
				{
					int iTextureID = iSmallVideoThumbnail[current_tutorial_id];
					if (iTextureID > 0)
					{
						if (ImGui::ImgBtn(iTextureID, vPreviewSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
						{
							//Click ?
						}
					}
				}
				ImGui::SetWindowFontScale(1.8);

				char *find = strstr(title.Get(), "-");
				if (find) find++;
				if (find && find[0] == ' ') find++;
				else find = title.Get();

				if (pref.current_style == 25)
				{
					//PE: Only blue style have different color, dont match on other styles.
					ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
					textcol.w = 1.0;
					ImGui::PushStyleColor(ImGuiCol_Text, textcol);
					ImGui::Text(find);
					ImGui::PopStyleColor();
				}
				else
				{
					ImGui::Text(find);
				}
				ImGui::SetWindowFontScale(1.2);

				if (description.Len() > 0)
				{
					if (bUseFullScreen) ImGui::SetWindowFontScale(1.5);
					ImGui::Indent(-2);
					ImVec2 cp = ImGui::GetCursorPos();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
					ImGui::BeginChild("##JustaFrameForTutorialDescription", ImVec2(fContentWidth, WelcomeFrameHeight - cp.y - 4.0), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
					ImGui::TextWrapped(description.Get());
					ImGui::PopStyleVar(2);
					ImGui::EndChild();
					ImGui::Indent(2);
					ImGui::SetWindowFontScale(1.2);
				}

			}
			else if (bUseProject)
			{
				int iTextureID = BOX_CLICK_HERE;
				if (ImageExist(projectbank_imageid[current_project_id])) iTextureID = projectbank_imageid[current_project_id];

				if (iTextureID > 0)
				{
					float missing = 0;
					if (current_project_selected.length() > 0)
					{
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
						ImVec2 padding = { 1.0, 1.0 };
						ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + vPreviewSize);
						float width = ImGui::GetContentRegionAvailWidth();
						ImVec2 winpos = ImGui::GetWindowPos();
						if (image_bb.Max.x < winpos.x + width)
						{
							//PE: Center image.
							missing = (winpos.x + width) - image_bb.Max.x;
							missing *= 0.5f;
							image_bb.Min.x += missing;
							image_bb.Max.x += missing;
						}
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + missing);
					if (ImGui::ImgBtn(iTextureID, vPreviewSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
					{
						//Click ?
					}
				}
				ImGui::SetWindowFontScale(1.4);
				float fButWidth = fContentWidth * 0.25 - right_margin;
				ImVec2 vCurPos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(vCurPos + ImVec2(fButWidth*0.5, 12));
				if (ImGui::StyleButton("Play Game", ImVec2(fButWidth, 0)))
				{
					//PE: Play - Load and start storyboard, also start play process.
					TriggerLoadGameProject = current_project_selected.c_str();
					bWelcomeScreen_Window = false;
					bStoryboardWindow = true;
					iStoryboardExecuteKey = ' ';
					if (current_tutorial_id >= 0)
					{
						iStopAndFreeThisVideo = current_tutorial_id;
					}

				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Play The Game");
				ImGui::SetCursorPos(vCurPos + ImVec2(fButWidth*0.5 + fButWidth + fButWidth, 12));
				if (ImGui::StyleButton("Edit Game", ImVec2(fButWidth, 0)))
				{
					//PE: Edit - Load and start storyboard.
					TriggerLoadGameProject = current_project_selected.c_str();
					bWelcomeScreen_Window = false;
					bStoryboardWindow = true;
					if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Edit The Game");
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 8));

				ImGui::SetWindowFontScale(1.8);
				if (pref.current_style == 25)
				{
					//PE: Only blue style have different color, dont match on other styles.
					ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
					textcol.w = 1.0;
					ImGui::PushStyleColor(ImGuiCol_Text, textcol);
					ImGui::Text(current_project_selected.c_str());
					ImGui::PopStyleColor();
				}
				else
				{
					ImGui::Text(current_project_selected.c_str());
				}
				ImGui::SetWindowFontScale(1.2);

				//PE: Need a tmp read of storyboard to get the correct desc...
				static std::string last_current_project_selected = "##";
				static std::string game_description = "";
				static std::string game_developer_description = "";
				if (current_project_selected != last_current_project_selected || bTriggerRereadDescription)
				{
					game_description = "";
					game_developer_description = "";
					if (load_checkproject_storyboard( (char *) current_project_selected.c_str() ))
					{
						game_description = checkproject.game_description;
						game_developer_description = checkproject.game_developer_desc;
					}
					last_current_project_selected = current_project_selected;
					bTriggerRereadDescription = false;
				}
				if (game_description.length() > 0)
				{
					if(bUseFullScreen) ImGui::SetWindowFontScale(1.2);
					ImGui::Indent(-2);
					ImVec2 cp = ImGui::GetCursorPos();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
					ImGui::BeginChild("##JustaFrameForDescription", ImVec2(fContentWidth, WelcomeFrameHeight-cp.y-4.0), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
					ImGui::TextWrapped(game_description.c_str());

					if (game_developer_description.length() > 0)
					{
						ImGui::SetWindowFontScale(1.5);
						ImGui::Text("");
						if (pref.current_style == 25)
						{
							ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
							textcol.w = 1.0;
							ImGui::PushStyleColor(ImGuiCol_Text, textcol);
							ImGui::Text("Game Developer");
							ImGui::PopStyleColor();
						}
						else
						{
							ImGui::Text("Game Developer");
						}
						ImGui::SetWindowFontScale(1.2);
						ImGui::Text(game_developer_description.c_str());
					}

					ImGui::PopStyleVar(2);
					ImGui::EndChild();
					ImGui::Indent(2);
					ImGui::SetWindowFontScale(1.2);
				}
			}
			else
			{
				int iFileListEntry = GetEntryFilesListForLibrary(sCurrentGame);
				if (g_LibraryFileList.size() > 0)
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					ImRect image_bb(window->DC.CursorPos, window->DC.CursorPos + vPreviewSize);

					int iTextureID = GetImageIDFilesListForLibrary(sCurrentGame);
					if (iTextureID > 0)
					{
						float missing = 0;

						if (current_project_selected.length() > 0)
						{
							ImGuiWindow* window = ImGui::GetCurrentWindow();
							ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
							ImVec2 padding = { 1.0, 1.0 };
							ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + vPreviewSize);

							float width = ImGui::GetContentRegionAvailWidth();
							ImVec2 winpos = ImGui::GetWindowPos();
							if (image_bb.Max.x < winpos.x + width)
							{
								//PE: Center image.
								missing = (winpos.x + width) - image_bb.Max.x;
								missing *= 0.5f;
								image_bb.Min.x += missing;
								image_bb.Max.x += missing;
							}

							window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
						}
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + missing);

						if (ImGui::ImgBtn(iTextureID, vPreviewSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
						{
							//Click ?
						}
					}
					//#### VIDEO START ####
					static int triggerEndVideo = 0;
					static int iVideoTriggerTimer = 0;
					int n = iFileListEntry;

					if (n < g_LibraryFileList.size() && ImGui::IsItemHovered() && iVideoTriggerTimer++ > 30)
					{
						if (iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID))
						{
							float fVideoW = 0;
							float fVideoH = 0;
							float animU = 1.0;
							float animV = 1.0;
							ID3D11ShaderResourceView* lpVideoTexture = NULL;
							lpVideoTexture = GetAnimPointerView(iWelcomeVideoID);
							if (lpVideoTexture != NULL)
							{
								fVideoW = GetAnimWidth(iWelcomeVideoID);
								fVideoH = GetAnimHeight(iWelcomeVideoID);
								SetRenderAnimToImage(iWelcomeVideoID, true);
								animU = GetAnimU(iWelcomeVideoID);
								animV = GetAnimV(iWelcomeVideoID);
								ImVec2 uv0 = ImVec2(0, 0);
								ImVec2 uv1 = ImVec2(animU, animV);
								window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
							}

							float fdone = GetAnimPercentDone(iWelcomeVideoID) / 100.0f;
							if (fdone > 0.5)
								triggerEndVideo = 1;
							if (triggerEndVideo > 0 && (fdone == 0.0f || fdone >= 1.0f))
							{
								iVideoTriggerTimer = 0;
								bRemoveVideoInNextFrame = true;
								triggerEndVideo = 0;
							}
						}
						else
						{
							//Load video.
							std::string sVideoFile = g_LibraryFileList[n].cFile.Get();
							replaceAll(sVideoFile, ".png", ".mp4");
							if (FileExist((LPSTR)sVideoFile.c_str()))
							{
								iWelcomeVideoID = 0;
								for (int i = 1; i <= 32; i++)
								{
									if (AnimationExist(i) == 0) { iWelcomeVideoID = i; break; }
								}

								if (iWelcomeVideoID > 0)
								{
									if (LoadAnimation((LPSTR)sVideoFile.c_str(), iWelcomeVideoID, g.videoprecacheframes, g.videodelayedload, 1) == false)
									{
										iWelcomeVideoID = -999;
									}
									if (iWelcomeVideoID > 0)
									{
										PlaceAnimation(iWelcomeVideoID, -1, -1, -1, -1);
										SetRenderAnimToImage(iWelcomeVideoID, true);
										PlayAnimation(iWelcomeVideoID);
										SetVideoVolume(100);
									}
								}
							}
							else
							{
								iVideoTriggerTimer = 0; //Failed.
								triggerEndVideo = 0;
							}
						}
					}
					else
					{
						if (n < g_LibraryFileList.size() && !ImGui::IsItemHovered())
						{
							if (iVideoTriggerTimer > 0 && iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID)) {
								bRemoveVideoInNextFrame = true;
								triggerEndVideo = 0;
							}
							iVideoTriggerTimer = 0;
						}
					}
					//#### VIDEO END ####

					// Project Exists if can get dev description
					extern bool g_bFreeTrialVersion;
					bool bProjectExistsAndValidToUse = false;
					static std::string sDevDescription = "";
					if (iFileListEntry >= 0) 
						if (g_LibraryFileList[iFileListEntry].bProjectExists==true) 
							bProjectExistsAndValidToUse = true;

					// Play and Edit Buttons
					ImGui::SetWindowFontScale(1.4);
					float fButWidth = fContentWidth * 0.25 - right_margin;
					ImVec2 vCurPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(vCurPos + ImVec2(fButWidth*0.5, 12));
					ImVec4 vTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					LPSTR pPlayTextTip = "Play The Game";
					LPSTR pEditTextTip = "Edit The Game";
					if (bProjectExistsAndValidToUse == false)
					{
						if (g_bFreeTrialVersion == true)
						{
							vTextColor = ImVec4(0.9f, 0.9f, 0.9f, 0.5f);
							pPlayTextTip = "Upgrade to the full version to play this demo game";
							pEditTextTip = "Upgrade to the full version to edit this demo game";
						}
					}
					ImGui::PushStyleColor(ImGuiCol_Text, vTextColor);
					if (ImGui::StyleButton("Play Game", ImVec2(fButWidth, 0)))
					{
						if (bProjectExistsAndValidToUse == true)
						{
							bool bUseProjectName = false;
							if (iFileListEntry >= 0)
							{
								if (g_LibraryFileList[iFileListEntry].cProject.Len() > 0)
								{
									if (load_checkproject_storyboard((char *)g_LibraryFileList[iFileListEntry].cProject.Get()))
									{
										//Success use this project name.
										bUseProjectName = true;
									}
								}
							}
							if (bUseProjectName)
							{
								//PE: Launch real standalone, that will return to storyboard when done.
								extern bool g_bCascadeQuitFlag;
								g_bCascadeQuitFlag = true;
								PostQuitMessage(0);
								SetCurrentDirectoryA("..\\");
								char par[MAX_PATH];
								sprintf(par, "project=2%s", g_LibraryFileList[iFileListEntry].cProject.Get());
								ExecuteFile("GameGuruMAX.exe", par, "", 0);
								Sleep(500);
								ExitProcess(0);
							}
							else
							{
								cstr filetoload = cstr("mapbank\\") + sDisplayName + cstr(".fpm");
								strcpy(cDirectOpen, filetoload.Get());
								iLaunchAfterSync = 7; //Direct load.
								iSkibFramesBeforeLaunch = 5;
								bLaunchTestGameAfterLoad = true;
								bWelcomeScreen_Window = false;
								if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
							}
						}
						else
						{
							if (g_bFreeTrialVersion == true)
							{
								extern bool bFreeTrial_Window;
								bFreeTrial_Window = true;
							}
						}
					}
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pPlayTextTip);
					ImGui::PushStyleColor(ImGuiCol_Text, vTextColor);
					ImGui::SetCursorPos(vCurPos + ImVec2(fButWidth*0.5 + fButWidth + fButWidth, 12));
					if (ImGui::StyleButton("Edit Game", ImVec2(fButWidth, 0)) || bTriggerEditDemoGame)
					{
						if (bProjectExistsAndValidToUse == true)
						{
							bool bUseProjectName = false;
							if (iFileListEntry >= 0)
							{
								if (g_LibraryFileList[iFileListEntry].cProject.Len() > 0)
								{
									if (load_checkproject_storyboard((char *)g_LibraryFileList[iFileListEntry].cProject.Get()))
									{
										//Success use this project name.
										bUseProjectName = true;
									}
								}
							}
							if (bUseProjectName)
							{
								//PE: Edit - Load and start storyboard.
								TriggerLoadGameProject = g_LibraryFileList[iFileListEntry].cProject.Get();
								bWelcomeScreen_Window = false;
								bStoryboardWindow = true;
								if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
							}
							else
							{
								cstr filetoload = cstr("mapbank\\") + sDisplayName + cstr(".fpm");
								strcpy(cDirectOpen, filetoload.Get());
								iLaunchAfterSync = 7; //Direct load.
								iSkibFramesBeforeLaunch = 5;
								bWelcomeScreen_Window = false;
								if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
							}
						}
						else
						{
							if (g_bFreeTrialVersion == true && bTriggerEditDemoGame == false)
							{
								extern bool bFreeTrial_Window;
								bFreeTrial_Window = true;
							}
						}
						bTriggerEditDemoGame = false;
					}
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pEditTextTip);

					// Project Descriptions and Text
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 8));
					ImGui::SetWindowFontScale(1.8);
					ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
					textcol.w = 1.0;
					ImGui::PushStyleColor(ImGuiCol_Text, textcol);
					ImGui::Text(sDisplayName.Get());
					ImGui::PopStyleColor();

					ImGui::SetWindowFontScale(1.2);
					if (iFileListEntry >= 0)
					{
						if (g_LibraryFileList[iFileListEntry].cDescription.Len() > 0)
						{
							if (bUseFullScreen) ImGui::SetWindowFontScale(1.5);
							ImGui::TextWrapped(g_LibraryFileList[iFileListEntry].cDescription.Get());
							ImGui::SetWindowFontScale(1.2);
							static cstr sPrevGame = "";
							if (strcmp(sPrevGame.Get(), sCurrentGame.Get()) != 0)
							{
								sDevDescription = "";
								sPrevGame = sCurrentGame;
								if (sCurrentGame.Len() > 0)
								{
									// Extract the game developer description from the project.dat file.
									char projectname[MAX_PATH];
									strcpy(projectname, sCurrentGame.Get());
									projectname[strlen(projectname) - 4] = 0;
									char project[MAX_PATH];

									sprintf(project, "projectbank\\%s\\project%d.dat", projectname, STORYBOARDVERSION);
									FILE* projectfile = GG_fopen(project, "rb");
									if (!projectfile)
									{
										strcpy(project, "projectbank\\");
										strcat(project, projectname);
										strcat(project, "\\project.dat");
										projectfile = GG_fopen(project, "rb");
									}
									if (projectfile)
									{
										fclose(projectfile);

										//PE: Use this so we can upgrade from 202 to 203+
										bool load__storyboard_into_struct(const char*, StoryboardStruct&);
										load__storyboard_into_struct(project, tempProjectData);


										//tempProjectData.game_developer_desc[0] = 0;
										//size_t size = fread(&tempProjectData, 1, sizeof(tempProjectData), projectfile);
										//Valid pref:
										
										if(strlen(tempProjectData.game_developer_desc) > 0)
											sDevDescription = tempProjectData.game_developer_desc;

										// Text wrapping is saved into the project data, so unwrap it so it can be wrapped by ImGui at larger font sizes.
										for (int i = 0; i < sDevDescription.length(); i++)
										{
											if (sDevDescription[i] == '\n')
											{
												sDevDescription[i] = ' ';
											}
										}
									}
								}
							}
							if (sDevDescription.length() > 0)
							{
								ImGui::Spacing();
								ImGui::SetWindowFontScale(1.8);
								if (pref.current_style == 25)
								{
									ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
									textcol.w = 1.0;
									ImGui::PushStyleColor(ImGuiCol_Text, textcol);
									ImGui::Text("Game Developer");
									ImGui::PopStyleColor();
								}
								else
								{
									ImGui::Text("Game Developer");
								}
								ImGui::SetWindowFontScale(1.5f);
								ImGui::TextWrapped(sDevDescription.c_str());
								ImGui::SetWindowFontScale(1.2);
							}
						}
						ImGui::PushStyleColor(ImGuiCol_Text, textcol);
						ImGui::PopStyleColor();
					}
				}
			}

			vPreviewSize = { fContentWidth , fContentWidth * fRatio };


			ImGui::Text("");

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetWindowSize().y - 72.0f));

			ImGui::Indent(-2);
			ImGui::EndChild();

			ImGui::SetWindowFontScale(2.0); //1.4

			if (iCurrentOpenTab == 4)
			{
				if (ImGui::StyleButton("View the User Guide", ImVec2(vPreviewSize.x + 4.0, fFontSize*2.6))) //*2.0
				{
					// User guide has been moved to offline only
					ExecuteFile("..\\Guides\\User Manual\\GameGuru MAX - User Guide.pdf", "", "", 0);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "View the GameGuru MAX User Guide"); //Weekly Live Stream
			}
			else if (iCurrentOpenTab == 5)
			{
				//if (ImGui::StyleButton("Click here to view the GameGuru MAX YouTube Channel", ImVec2(vPreviewSize.x + 4.0, fFontSize*2.6))) //*2.0
				//{
				//	ExecuteFile("https://bit.ly/MAXYouTubeChannel", "", "", 0);
				//}
			}
			#ifndef GGMAXEDU
			else if (iCurrentOpenTab == 6)
			{
				char pCheckFinalDestFolderExists[MAX_PATH];
				strcpy(pCheckFinalDestFolderExists, "");
				uint32 uAccountID = 0;
				if (SteamUser())
				{
					uAccountID = SteamUser()->GetSteamID().GetAccountID();
					LPSTR pMediaTypePath = workshop_getmediatypepath(g_iCurrentMediaTypeForWorkshopItem);
					sprintf(pCheckFinalDestFolderExists, "%s\\Community\\%d\\'", pMediaTypePath, uAccountID);
					GG_GetRealPath(pCheckFinalDestFolderExists, 1);
					LPSTR pTitleOfWorkshopButton = "Submit New Item To Workshop";
					if (g_iSelectedExistingWorkshopItem != -1) pTitleOfWorkshopButton = "Update Your Workshop Item";
					if (ImGui::StyleButton(pTitleOfWorkshopButton, ImVec2(vPreviewSize.x + 4.0, fFontSize * 2.6)))
					{
						// submit workshop item
						if (workshop_submit_item_check() == true)
						{
							// perform actual submission of current workshop item
							workshop_submit_item_now();
						}
					}
				}
			}
			else if (iCurrentOpenTab == 7)
			{
				if (ImGui::StyleButton("Click here to view the Steam Workshop for GameGuru MAX", ImVec2(vPreviewSize.x + 4.0, fFontSize * 2.6)))
				{
					ExecuteFile("https://steamcommunity.com/app/1247290/workshop/", "", "", 0);
				}
			}
			else if (iCurrentOpenTab == 42)
			{
				if (ImGui::StyleButton("Click here to view the latest GameGuru MAX News", ImVec2(vPreviewSize.x + 4.0, fFontSize * 2.6)))
				{
					ExecuteFile("https://www.game-guru.com/latest-news", "", "", 0);
				}
			}
			#endif
			else
			{
				//Changed to project based.
				if (ImGui::StyleButton("Create a New Game Project", ImVec2(vPreviewSize.x+4.0, fFontSize*2.6))) //*2.0
				{
					strcpy(pref.cLastUsedStoryboardProject, "");
					bStoryboardInitNodes = false; //Just init again.
					bStoryboardFirstRunSetInitPos = false;
					process_storeboard(true); //Init a new project.
					//PE: Bug - When creating a new project, it would contain g_collectionList from prev. loaded project.
					init_rpg_system();

					bTriggerSaveAsAfterNewLevel = true;
					bTriggerSaveAs = true;
					strcpy(SaveProjectAsName, "");
					strcpy(SaveProjectAsError, "");
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Create a new game project");

				if (bTriggerSaveAs)
				{
					ImGui::SetWindowFontScale(1.0);
					int iCreateRet = save_create_storyboard_project();
					if (iCreateRet == 1)
					{
						//PE: New project created.
						bWelcomeScreen_Window = false;
						bStoryboardWindow = true;
						if (current_tutorial_id >= 0) iStopAndFreeThisVideo = current_tutorial_id;
					}
					if (iCreateRet == 2)
					{
						bTriggerSaveAsAfterNewLevel = false;
						bTriggerSaveAs = false;
					}
					ImGui::SetWindowFontScale(2.0); //1.4
				}

			}

			ImGui::SetWindowFontScale(1.0);

			//###########################################################
			//#### SWAP TOOL_GOBACK was here "Exit to Hub" ####
			//###########################################################
			if (projectbank_list.size() > 0)
			{
				bool bTmp = 1 - pref.iDisplayWelcomeScreen;
				float fTextWidth = ImGui::CalcTextSize("Tick to skip Hub and continue editing the last game project").x + 20.0f;
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvailWidth() - 10.0) * 0.5) - (fTextWidth * 0.5), 0));
				if (ImGui::Checkbox("Tick to skip Hub and continue editing the last game project", &bTmp))
				{
					pref.iDisplayWelcomeScreen = 1 - bTmp;
				}
			}

			ImGui::SetWindowFontScale(1.0);
			ImGui::Columns(1);

			ImGui::Indent(-10);

			bImGuiGotFocus = true;

			if (bDisplayAsModal)
				ImGui::EndPopup();
			else
				ImGui::End();
		}
	}

}

