void get_tutorials(void)
{
	cStr tOldDir = GetDir();

	tutorial_files.clear();
	tutorial_videos.clear();
	tutorial_description.clear();

	//PE: Add introduction video by hand.
	cstr cIntroVideo = "9901 - Introduction Video";
	tutorial_files.insert(std::make_pair(cIntroVideo.Get(), "tutorialbank\\9901-introduction-video.tut"));
	tutorial_videos.insert(std::make_pair(cIntroVideo.Get(), "tutorialbank\\9901-introduction-video.mp4"));
	tutorial_description.insert(std::make_pair(cIntroVideo.Get(), "Introduction Video"));

	SetDir("tutorialbank");

	ChecklistForFiles();
	for (int i = 1; i <= ChecklistQuantity(); i++)
	{
		if (ChecklistValueA(i) == 0)
		{
			cstr file_s = ChecklistString(i);
			if (cstr(Left(file_s.Get(), 1)) != ".")
			{
				cstr ext = Lower(Right(file_s.Get(), 4));
				if (ext == ".tut") 
				{
					//Read file and get tut: entry.
					//Read in TUT: entrie.
					FILE* fTut = GG_fopen(file_s.Get(), "r");
					if (fTut)
					{
						bool bVideoAdded = false;
						bool bTutorialAdded = false;
						bool bDescriptionAdded = false;
						char ctmp[TUTORIALMAXTEXT];
						char cVideoPath[MAX_PATH] = "\0";
						char cTutorialSet[TUTORIALMAXTEXT] = "\0";
						char cTutorialDescription[TUTORIALMAXTEXT] = "\0";
						while (!feof(fTut))
						{
							fgets(ctmp, TUTORIALMAXTEXT - 1, fTut);
							if (strlen(ctmp) > 0 && ctmp[strlen(ctmp) - 1] == '\n')
								ctmp[strlen(ctmp) - 1] = 0;

							if (strncmp(ctmp, "TUT:", 4) == 0)
							{
								if (!strlen(cTutorialName) > 0)
									strcpy(cTutorialName, &ctmp[5]);

								strcpy(cTutorialSet, &ctmp[5]);
								cstr path = "tutorialbank\\";
								path += file_s;
								tutorial_files.insert(std::make_pair(&ctmp[5], path.Get()));
								bTutorialAdded = true;
							}
							if (strncmp(ctmp, "DESC:", 5) == 0)
							{
								bDescriptionAdded = true;
								strcpy(cTutorialDescription, &ctmp[6]);
								std::string clean_string = cTutorialDescription;
								replaceAll(clean_string, "â€™", "'"); //Replace UTF8. 0xE2 0x90 0x99
								strcpy(cTutorialDescription, clean_string.c_str());
							}
							if (strncmp(ctmp, "VIDEO:", 6) == 0)
							{
								strcpy(cVideoPath, &ctmp[7]);
								SetDir(tOldDir.Get());
								char resolved[MAX_PATH];
								int retval = GetFullPathNameA(cVideoPath, MAX_PATH, resolved, NULL);
								if (retval > 0) {
									strcpy(cVideoPath, resolved);
								}
								SetDir("tutorialbank");
								bVideoAdded = true;
							}
							if (bTutorialAdded && bVideoAdded && bDescriptionAdded)
								break;

						}
						fclose(fTut);
						if (bTutorialAdded && bVideoAdded) {
							tutorial_videos.insert(std::make_pair(cTutorialSet, cVideoPath));
						}
						if (bTutorialAdded && bDescriptionAdded) {
							tutorial_description.insert(std::make_pair(cTutorialSet, cTutorialDescription));
						}
						if (bTutorialAdded && !bDescriptionAdded) {
							//If no desc , just add title.
							tutorial_description.insert(std::make_pair(cTutorialSet, cTutorialSet));
						}

						if (bTutorialAdded && !bVideoAdded) {
							//Add default video path. must always be the same as tutorial_files
							tutorial_videos.insert(std::make_pair(cTutorialSet, ""));
						}
					}
				}
			}
		}
	}
	SetDir(tOldDir.Get());

}

void generic_preloadfiles(void)
{
	//PE: We might have to edit this list when we have the final media to use.
	timestampactivity(0, "preload generic textures early");
	image_preload_files_start();
	image_preload_files_add("effectbank\\explosion\\animatedspark.dds");
	image_preload_files_add("effectbank\\particles\\flare.dds");
	image_preload_files_add("effectbank\\particles\\64smoke2.dds");
	image_preload_files_add("effectbank\\particles\\flame.dds");

	image_preload_files_add("editors\\gfx\\cursor.dds");

	//PNG Test.
	image_preload_files_add("languagebank\\english\\artwork\\quick-start-testlevel-prompt.png",1);
	image_preload_files_add("languagebank\\english\\artwork\\quick-help.png",1);
	image_preload_files_add("languagebank\\english\\artwork\\testgamelayout-vr.png",1);
	image_preload_files_add("languagebank\\english\\artwork\\testgamelayout-noweapons.png",1);

	image_preload_files_add("languagebank\\english\\artwork\\gurumeditation.png",1);
	image_preload_files_add("languagebank\\english\\artwork\\gurumeditationoff.png",1);

	image_preload_files_add("editors\\gfx\\memorymeter.png",1);
	image_preload_files_add("editors\\gfx\\4.png",1);
	image_preload_files_add("editors\\gfx\\5.png",1);
	image_preload_files_add("editors\\gfx\\13.png",1);
	image_preload_files_add("editors\\gfx\\26.png",1);

	image_preload_files_add("editors\\gfx\\9.png",1);
	image_preload_files_add("editors\\gfx\\14.png",1);

	image_preload_files_add("languagebank\\english\\artwork\\f9-help-terrain.png",1);
	image_preload_files_add("languagebank\\english\\artwork\\f9-help-entity.png",1);
	image_preload_files_add("languagebank\\english\\artwork\\f9-help-conkit.png",1);

	image_preload_files_add("editors\\gfx\\resources.png",1);
	image_preload_files_add("editors\\gfx\\resourceslow.png",1);

	image_preload_files_add("editors\\gfx\\resourcesgone.png",1);
	image_preload_files_add("editors\\gfx\\resourcesworking.png",1);


	image_preload_files_finish();
}

void CloseDownEditorProperties(void)
{
	if (t.gridentityinzoomview > 0) 
	{
		t.tpressedtoleavezoommode = 2; //Exit zoom and save.
		int olges = t.grideditselect;
		//Make sure to exit fast. and restore cursor object.
		int igridentity = t.gridentity;
		if (iOldgridentity != t.gridentity && iOldgridentity > -1)
			t.gridentity = iOldgridentity;

		t.grideditselect = 4;
		editor_viewfunctionality();
		t.grideditselect = olges;
		t.gridentity = igridentity;
	}
}

void FormatTTS(LPSTR pFormattedTTS, LPSTR pFormattedTTSOut)
{
	memset(pFormattedTTSOut, 0, 1000);
	int nout = 0;
	for (int n = 0; n < strlen(pFormattedTTS); n++)
	{
		if ( pFormattedTTS[n] >= 32 && pFormattedTTS[n] <= 255 )
			pFormattedTTSOut[nout++] = pFormattedTTS[n];
	}
	pFormattedTTSOut[nout] = 0;
}

bool g_bVoiceSettingsChanged = false;
int g_iVoiceSettingsUpdateSpeechID = 0;

void SpeechControls(int speech_entries, bool bUpdateMainString, entityeleproftype *edit_grideleprof)
{
	if (!edit_grideleprof)
	{
		edit_grideleprof = &t.grideleprof;
	}

	//LB: Solve intend and incorrect component inclusion inside FPE (behavior area)
	ImGui::TextCenter("Speech Control");
	{
		bool sapi_available = false;
		if (g_voiceList_s.size() > 0)
			sapi_available = true;

		ImGui::Indent(10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

		if (sapi_available) 
		{
			ImGui::Text("Voice");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			//Combo
			ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);
			if (ImGui::BeginCombo("##SelectVoiceCCP", pCCPVoiceSet)) // The second parameter is the label previewed before opening the combo.
			{
				int size = g_voiceList_s.size();
				for (int vloop = 0; vloop < size; vloop++) {

					bool is_selected = false;
					if (strcmp(g_voiceList_s[vloop].Get(), pCCPVoiceSet) == 0)
						is_selected = true;

					if (ImGui::Selectable(g_voiceList_s[vloop].Get(), is_selected)) 
					{
						//Change Voice set
						pCCPVoiceSet = g_voiceList_s[vloop].Get();
						CCP_SelectedToken = g_voicetoken[vloop];
						edit_grideleprof->voiceset_s = pCCPVoiceSet;
						if (g_bVoiceSettingsChanged == false)
						{
							g_bVoiceSettingsChanged = true;
							g_iVoiceSettingsUpdateSpeechID = 0;
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Select Voice to Use For Speak");

			ImGui::PopItemWidth();

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
			ImGui::Text("Rate");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));

			ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);
			int iOldRate = CCP_Speak_Rate;
			ImGui::SliderInt("##speakrate", &CCP_Speak_Rate, -5, 5);
			if (CCP_Speak_Rate != iOldRate)
			{
				edit_grideleprof->voicerate = CCP_Speak_Rate;
				if (g_bVoiceSettingsChanged == false)
				{
					g_bVoiceSettingsChanged = true;
					g_iVoiceSettingsUpdateSpeechID = 0;
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Set Speak Rate");

			ImGui::PopItemWidth();

		}
		//LOOP speech_entries
		//Use unique IDs

		// monitor lip sync generator
		bool bLipSyncGenerationBusy = false;
		float fProgressOfGeneration = GetWAVtoLIPProgress();
		if (fProgressOfGeneration > 0.0f && fProgressOfGeneration < 1.0f)
			bLipSyncGenerationBusy = true;

		for (int SpeechLoop = 0; SpeechLoop < speech_entries; SpeechLoop++)
		{
			if (speech_ids[SpeechLoop] >= 0)
			{
				cstr tmpvar = edit_grideleprof->PropertiesVariable.Variable[speech_ids[SpeechLoop]];
				tmpvar = tmpvar.Lower();

				//Display soundset file entry.
				int iButtonControlAndState = 0; // 0-can edit button, 1-buttom used, 2-background task in progress (so should not press button until done)
				if ( bLipSyncGenerationBusy == true ) iButtonControlAndState = 2;
				LPSTR pButtonControlIfBlocked = "Still Generating Lip Sync Data";
				cstr used_soundset;
				if (tmpvar == "speech1" || tmpvar == "speech 1") 
				{
					edit_grideleprof->soundset1_s = imgui_setpropertyfile2_ex_dlua(t.group, edit_grideleprof->soundset1_s.Get(), "SPEECH 1", t.strarr_s[254].Get(), "audiobank\\", &iButtonControlAndState, pButtonControlIfBlocked );
					used_soundset = edit_grideleprof->soundset1_s;
				}
				else if (tmpvar == "speech2" || tmpvar == "speech 2") 
				{
					edit_grideleprof->soundset2_s = imgui_setpropertyfile2_ex_dlua(t.group, edit_grideleprof->soundset2_s.Get(), "SPEECH 2", t.strarr_s[254].Get(), "audiobank\\", &iButtonControlAndState, pButtonControlIfBlocked );
					used_soundset = edit_grideleprof->soundset2_s;
				}
				else if (tmpvar == "speech3" || tmpvar == "speech 3") 
				{
					edit_grideleprof->soundset3_s = imgui_setpropertyfile2_ex_dlua(t.group, edit_grideleprof->soundset3_s.Get(), "SPEECH 3", t.strarr_s[254].Get(), "audiobank\\", &iButtonControlAndState, pButtonControlIfBlocked );
					used_soundset = edit_grideleprof->soundset3_s;
				}
				else if (tmpvar == "speech0" || tmpvar == "speech 0") 
				{
					edit_grideleprof->soundset_s = imgui_setpropertyfile2_ex_dlua(t.group, edit_grideleprof->soundset_s.Get(), "SPEECH", t.strarr_s[254].Get(), "audiobank\\", &iButtonControlAndState, pButtonControlIfBlocked );
					used_soundset = edit_grideleprof->soundset_s;
				}
				else 
				{
					edit_grideleprof->soundset_s = imgui_setpropertyfile2_ex_dlua(t.group, edit_grideleprof->soundset_s.Get(), "SPEECH 4", t.strarr_s[254].Get(), "audiobank\\", &iButtonControlAndState, pButtonControlIfBlocked );
					used_soundset = edit_grideleprof->soundset_s;
				}
				if (iButtonControlAndState == 1)
				{
					// user changed one of the speech fields, so generate LIP file for it
					ConvertWAVtoLIP(used_soundset.Get());
				}

				std::string uniquiField = ">";
				uniquiField = uniquiField + "##";
				uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);
				ImGui::PushItemWidth(ImGui::GetFontSize()*2.0);

				int iButImageSize = 16;
				ImGui::PushID(grideleprof_uniqui_id++);

				ImGui::Indent(-10);
				if (ImGui::StyleButton("Play back current voiceover", ImVec2(ImGui::GetContentRegionAvail().x - 10, ImGui::GetFontSize()*1.5)))
				{
					// the sound we will use for the preview
					bool bJustStopped = false;
						int iFreeSoundID = g.temppreviewsoundoffset;
						if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
						{
							// stop currently playing preview
							StopSound(iFreeSoundID);
								bJustStopped = true;
						}
					if (used_soundset.Len() > 0)
					{
						// play custom wav file directly.
						if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
						if (FileExist(used_soundset.Get()) == 1 && bJustStopped == false)
						{
							LoadSound(used_soundset.Get(), iFreeSoundID, 0, 1);
							if (SoundExist(iFreeSoundID) == 1)
								PlaySound(iFreeSoundID);
						}
					}
				}
				ImGui::PopID();

				static bool g_bRecordingSound = false;
				static cstr g_recordingFile_s;
				uniquiField = "o";
				uniquiField = uniquiField + "##";
				uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);
				ImGui::PushID(grideleprof_uniqui_id++);
				int iRecordBtnImg = MEDIA_RECORD;
				if (g_bRecordingSound == true) iRecordBtnImg = MEDIA_RECORDING;
				if (bLipSyncGenerationBusy == true) iRecordBtnImg = MEDIA_RECORDPROCESSING;
				if(ImGui::StyleButton("Record a new voiceover", ImVec2(ImGui::GetContentRegionAvail().x - 10, ImGui::GetFontSize()*1.5)))
				{
					if (g_bRecordingSound == false)
					{
						// can only start recording once any lip sync progress has finished
						if (bLipSyncGenerationBusy == true)
						{
							MessageBoxA(NULL, "Cannot start recording until lip sync generation finished", "Notification", MB_OK);
						}
						else
						{
							// choose a unique recording name
							int iRecordingNum = 1;
							while (iRecordingNum < 9999)
							{
								// find a free recording WAV filename
								g_recordingFile_s = cstr("audiobank\\recordings\\Recording-") + cstr(iRecordingNum) + ".wav";
								if (FileExist(g_recordingFile_s.Get()) == 0)
									break;
								iRecordingNum++;
							}

							// start recording
							cstr absWAVPath_s = g.fpscrootdir_s + "\\Files\\" + g_recordingFile_s;
							char pRealAbsWAVForRecording[MAX_PATH];
							strcpy(pRealAbsWAVForRecording, absWAVPath_s.Get());
							GG_GetRealPath(pRealAbsWAVForRecording, 1);
							RecordWAV(pRealAbsWAVForRecording);
							g_bRecordingSound = true;
						}
					}
				}
				ImGui::PopID();
				ImGui::Indent(10);
				ImGui::PopItemWidth();
				ImGui::Spacing();
				
				if (g_bRecordingSound == true)
				{
					if ( RecordWAVProgress() >= 1.0f && bLipSyncGenerationBusy == false )
					{
						// end recording mode
						g_bRecordingSound = false;

						// stop recording and save
						if (tmpvar == "speech1" || tmpvar == "speech 1") edit_grideleprof->soundset1_s = g_recordingFile_s;
						if (tmpvar == "speech2" || tmpvar == "speech 2") edit_grideleprof->soundset2_s = g_recordingFile_s;
						if (tmpvar == "speech3" || tmpvar == "speech 3") edit_grideleprof->soundset3_s = g_recordingFile_s;
						if (tmpvar == "speech0" || tmpvar == "speech 0") edit_grideleprof->soundset_s = g_recordingFile_s;

						// generate LIP file from recording
						cstr absWAVPath_s = g.fpscrootdir_s + "\\Files\\" + g_recordingFile_s;
						ConvertWAVtoLIP(absWAVPath_s.Get());
						bLipSyncGenerationBusy = true;
					}
				}

				if (sapi_available) 
				{
					//The edit_grideleprof->soundset_s to use is already known so:
					//Store the actual text entered in the DLUA value fields.
					ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
					ImGui::PushItemWidth(-10);

					ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());

					uniquiField = "";
					uniquiField = uniquiField + "##speakTTStext";
					uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);

					// detect any editing of TTS text
					static bool bDetectWhenFinishedEditingTTSText = false;
					if (ImGui::InputTextMultiline(uniquiField.c_str(), &edit_grideleprof->PropertiesVariable.VariableValue[speech_ids[SpeechLoop]][0], 1024, ImVec2(0, ImGui::GetFontSize()*3.0f)))
					{
						// flagged any time TTS text changes
						bDetectWhenFinishedEditingTTSText = true;
					}
					if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

					// can also trigger regeneration of text when Voice Settings changed
					if (g_bVoiceSettingsChanged == true)
					{
						// go through all speeches and regeneate them one by one
						float fProgressOfGeneration = GetWAVtoLIPProgress();
						if (fProgressOfGeneration > 0.0f && fProgressOfGeneration < 1.0f)
						{
							// but if lip sync busy, we wait until free!
						}
						else
						{
							// when speech loop comes around to the next speech to refresh, trigger it now
							if (g_iVoiceSettingsUpdateSpeechID == SpeechLoop)
							{
								// regenerate this speech TTS
								bDetectWhenFinishedEditingTTSText = true;
								g_iVoiceSettingsUpdateSpeechID++;
								if (g_iVoiceSettingsUpdateSpeechID >= speech_entries)
								{
									// when no more speeches to refresh, finish voice settings cascade refresh
									g_bVoiceSettingsChanged = false;
								}
							}
						}
					}
					// only proceed when LIP sync not busy
					bool bLipSyncBusy = false;
					float fProgressOfGeneration = GetWAVtoLIPProgress();
					if (fProgressOfGeneration > 0.0f && fProgressOfGeneration < 1.0f) bLipSyncBusy = true;
					if (ImGui::IsItemActive() == false && bDetectWhenFinishedEditingTTSText==true && bLipSyncBusy==false)
					{
						// and reset as we are now doing the conversion and LIP file creation below
						bDetectWhenFinishedEditingTTSText = false;

						// this is the text we want to turn into WAV
						cstr TTSText_s = edit_grideleprof->PropertiesVariable.VariableValue[speech_ids[SpeechLoop]];

						// first, create a location to store the level-based TTS recordings
						// which will be in levelbank\ttsfiles\*.wav (keeps them local to FPM and transportable to Players)
						cstr pOldDir = GetDir();
						char pRealRoot[MAX_PATH];
						strcpy(pRealRoot, g.fpscrootdir_s.Get());
						strcat(pRealRoot, "\\Files\\levelbank\\");
						GG_GetRealPath(pRealRoot, 1);
						SetDir(pRealRoot);
						if (PathExist("testmap") == 0) MakeDirectory("testmap");
						SetDir("testmap");
						if (PathExist("ttsfiles") == 0) MakeDirectory("ttsfiles");
						SetDir("ttsfiles");

						// format typed text into something we can store as a reference in the TTS table below
						char pFormattedTTS[1000];
						int iInputTextMax = TTSText_s.Len();
						if (iInputTextMax > 999) iInputTextMax = 999;
						memcpy(pFormattedTTS, TTSText_s.Get(), iInputTextMax);
						pFormattedTTS[iInputTextMax] = 0;
						strcat(pFormattedTTS, pCCPVoiceSet);
						strcat(pFormattedTTS, cstr('A'+CCP_Speak_Rate).Get());
						char pFormattedTTSOut_Keeper[1000];
						FormatTTS(pFormattedTTS, pFormattedTTSOut_Keeper);

						// prepare two absolute paths for later
						char pRelLocationOfWAV[MAX_PATH];
						strcpy(pRelLocationOfWAV, "levelbank\\testmap\\ttsfiles\\"); // WAV added below

						// load in TTS table to see what TTS text we already have recordings for
						bool bIsTTSUnique = true;
						int iTTSTableMax = 0;
						std::vector <cstr> tempLines_s;
						Dim ( tempLines_s, 9999 );
						LPSTR pTTSTableFile = "ttstable.txt";
						if (FileExist(pTTSTableFile) == 1)
						{
							OpenToRead(1,pTTSTableFile);
							while (FileEnd(1) == 0)
							{
								tempLines_s[iTTSTableMax] = ReadString(1);
								iTTSTableMax++;
							}
							iTTSTableMax--;
							CloseFile(1);
						}
						if (iTTSTableMax > 0)
						{
							for (int line = 0; line < iTTSTableMax; line++)
							{
								// get the ref part of this line (and the WAV part for later)
								char pWAVItem[1001];
								char pRefItem[1001];
								strcpy(pWAVItem, "");
								strcpy(pRefItem, tempLines_s[line].Get());
								for (int n = 0; n < strlen(pRefItem); n++)
								{
									if (pRefItem[n] == 9)
									{
										strcpy(pWAVItem, pRefItem+n+1);
										pRefItem[n] = 0;
										break;
									}
								}

								// does it match what we are looking to add in
								if (stricmp(pRefItem, pFormattedTTSOut_Keeper) == NULL)
								{
									// yes, found the TTS we want already in the table
									if (FileExist(pWAVItem) == 1)
									{
										strcat(pRelLocationOfWAV, pWAVItem);
										bIsTTSUnique = false;
										break;
									}
								}
							}
						}

						// determine if what we have is unique
						if ( bIsTTSUnique == true )
						{
							// if so, create a unique file name entry for this one
							cstr pTTSFile;
							int iTTSNum = 1;
							while (iTTSNum < 9999)
							{
								pTTSFile = cstr("TTS") + cstr(iTTSNum) + ".wav";
								if (FileExist(pTTSFile.Get()) == 0)
									break;
								else
									iTTSNum++;
							}
							if (iTTSNum <= 9999)
							{
								// complete path new TTS WAV
								strcat(pRelLocationOfWAV, pTTSFile.Get());

								// add to table for future reference and potential use
								LPSTR pTTSTableFile = "ttstable.txt";
								if (FileExist(pTTSTableFile) == 1) DeleteFileA(pTTSTableFile);
								OpenToWrite(1,pTTSTableFile);
								for ( int i = 0; i<iTTSTableMax; i++ )
								{
									WriteString(1, tempLines_s[i].Get());
								}
								char pNewLine[1000];
								strcpy(pNewLine, pFormattedTTSOut_Keeper);
								pNewLine[strlen(pFormattedTTSOut_Keeper)+0] = 9;
								pNewLine[strlen(pFormattedTTSOut_Keeper)+1] = 0;
								strcat(pNewLine, pTTSFile.Get());
								WriteString(1,pNewLine);
								CloseFile(1);

								// need to be back in Files\\ folder for conversion to work properly
								SetDir(pOldDir.Get());

								// turn TEXT into WAV, store in this folder
								LPSTR pWAVFilename = pRelLocationOfWAV;
								if (FileExist(pWAVFilename)) DeleteFileA(pWAVFilename);
								LPSTR pWhatToSay = TTSText_s.Get();
								CComPtr<ISpVoice> spVoice;
								HRESULT hr = spVoice.CoCreateInstance(CLSID_SpVoice);
								if (SUCCEEDED(hr))
								{
									hr = spVoice->SetVoice(CCP_SelectedToken);
									if (SUCCEEDED(hr))
									{
										char pFinalWAVFilename[MAX_PATH];
										strcpy(pFinalWAVFilename, pWAVFilename);
										GG_GetRealPath(pFinalWAVFilename, 1);
										ConvertTXTtoWAVMeatyPart(spVoice, CCP_SelectedToken, CCP_Speak_Rate, pWhatToSay, pFinalWAVFilename);
									}
								}
							}
						}

						// change field of SPEECH X to [use text speech] - indicating its using the internal TTS wav created
						if (tmpvar == "speech1" || tmpvar == "speech 1") edit_grideleprof->soundset1_s = pRelLocationOfWAV;
						if (tmpvar == "speech2" || tmpvar == "speech 2") edit_grideleprof->soundset2_s = pRelLocationOfWAV;
						if (tmpvar == "speech3" || tmpvar == "speech 3") edit_grideleprof->soundset3_s = pRelLocationOfWAV;
						if (tmpvar == "speech0" || tmpvar == "speech 0") edit_grideleprof->soundset_s = pRelLocationOfWAV;

						// restore original folder for LIP file creation
						SetDir(pOldDir.Get());

						// can begin the WAV to LIP file now as we have the WAV file created
						ConvertWAVtoLIP(pRelLocationOfWAV);

						// and before we leave, take the opportunity to scan ALL entities and see if there are
						// any entries in the TTS table (and associated WAVs) that are not needed (probably due to recent change above)
						char pRealTTSFilesFolder[MAX_PATH];
						strcpy(pRealTTSFilesFolder, g.fpscrootdir_s.Get());
						strcat(pRealTTSFilesFolder, "\\Files\\levelbank\\testmap\\");
						SetDir(pRealTTSFilesFolder);
						if (PathExist("ttsfiles") == 0) MakeDirectory("ttsfiles");
						SetDir("ttsfiles");

						// load in latest TTS (given above activity of possible addition of new one)
						std::vector <cstr> tempRefs_s;
						Dim ( tempRefs_s, 9999 );
						iTTSTableMax = 0;
						pTTSTableFile = "ttstable.txt";
						if (FileExist(pTTSTableFile) == 1)
						{
							OpenToRead(1,pTTSTableFile);
							while (FileEnd(1) == 0)
							{
								char pRefItem[1001];
								tempLines_s[iTTSTableMax] = ReadString(1);
								strcpy ( pRefItem, tempLines_s[iTTSTableMax].Get() );
								for (int n = 0; n < strlen(pRefItem); n++)
								{
									if (pRefItem[n] == 9)
									{
										pRefItem[n] = 0;
										break;
									}
								}
								tempRefs_s[iTTSTableMax] = pRefItem;
								iTTSTableMax++;
							}
							iTTSTableMax--;
							CloseFile(1);
						}

						// start an array to hold flag as to whether to keep TTS table entry
						// and go through to auto-accept the one we've just added above (so it does not get deleted)
						bool* pbKeepInTable = new bool[iTTSTableMax+1];
						for (int iTTS = 0; iTTS < iTTSTableMax; iTTS++)
						{
							pbKeepInTable[iTTS] = false;
							if (stricmp(pFormattedTTSOut_Keeper, tempRefs_s[iTTS].Get()) == NULL)
								pbKeepInTable[iTTS] = true;
						}

						// go through ALL entities in current level (and all speech TTS texts buried in each one)
						for ( int e2 = 1; e2 <= g.entityelementlist; e2++ )
						{
							// need voice and speak rate from this entity
							LPSTR pVoiceSet2 = t.entityelement[e2].eleprof.voiceset_s.Get();
							int iSpeakRate2 = t.entityelement[e2].eleprof.voicerate;
							if (strlen(pVoiceSet2) == 0)
							{
								// default to first voice at startard rate
								pVoiceSet2 = g_voiceList_s[0].Get();
								iSpeakRate2 = 0;
							}

							int speech_ids2[5];
							for ( int n2 = 0; n2 < 5; n2++ ) speech_ids2[n2] = -1;
							int speech_entries2 = 0;
							for (int i2 = 0; i2 < t.entityelement[e2].eleprof.PropertiesVariable.iVariables; i2++)
							{
								cstr tmpvar = t.entityelement[e2].eleprof.PropertiesVariable.Variable[i2];
								tmpvar = tmpvar.Lower();
								if (speech_entries2 <= 3)
								{
									if (tmpvar == "speech1" || tmpvar == "speech 1") speech_ids2[speech_entries2++] = i2;
									if (tmpvar == "speech2" || tmpvar == "speech 2") speech_ids2[speech_entries2++] = i2;
									if (tmpvar == "speech3" || tmpvar == "speech 3") speech_ids2[speech_entries2++] = i2;
									if (tmpvar == "speech0" || tmpvar == "speech 0") speech_ids2[speech_entries2++] = i2;
								}
							}
							for (int iSpeechLoop2 = 0; iSpeechLoop2 < 4; iSpeechLoop2++)
							{
								LPSTR pSpeechItem = NULL;
								if (speech_ids2[iSpeechLoop2] != -1)
								{
									if (iSpeechLoop2 == 0) pSpeechItem = t.entityelement[e2].eleprof.PropertiesVariable.VariableValue[speech_ids2[iSpeechLoop2]];
									if (iSpeechLoop2 == 1) pSpeechItem = t.entityelement[e2].eleprof.PropertiesVariable.VariableValue[speech_ids2[iSpeechLoop2]];
									if (iSpeechLoop2 == 2) pSpeechItem = t.entityelement[e2].eleprof.PropertiesVariable.VariableValue[speech_ids2[iSpeechLoop2]];
									if (iSpeechLoop2 == 3) pSpeechItem = t.entityelement[e2].eleprof.PropertiesVariable.VariableValue[speech_ids2[iSpeechLoop2]];
									if (pSpeechItem)
									{
										// ensure the TTS string is formatted so can be matched against table (who's refs are formatted) [nice to do a hash here?]
										char pSpeechItemFormatted[1000];
										strcpy(pSpeechItemFormatted, pSpeechItem);
										strcat(pSpeechItemFormatted, pVoiceSet2);
										strcat(pSpeechItemFormatted, cstr('A'+iSpeakRate2).Get());
										char pSpeechItemFormattedOut[1000];
										FormatTTS(pSpeechItemFormatted, pSpeechItemFormattedOut);

										// a TTS text in one of the entity sound slots
										for (int iTTS = 0; iTTS < iTTSTableMax; iTTS++)
										{
											// compare the table entry with this entities TTS
											if (stricmp(pSpeechItemFormattedOut, tempRefs_s[iTTS].Get()) == NULL)
											{
												// found a match, flag this table entry as a keeper
												pbKeepInTable[iTTS] = true;
											}
										}
									}
								}
							}
						}
						// finally create a new table of only the keepers
						if (iTTSTableMax > 0)
						{
							// before table creation, check for duplicates (can happen when editing an entity over and over)
							for (int i1 = 0; i1 < iTTSTableMax; i1++)
							{
								if (pbKeepInTable[i1] == true)
								{
									for (int i2 = 0; i2 < iTTSTableMax; i2++)
									{
										if (i1 != i2)
										{
											if (pbKeepInTable[i2] == true)
											{
												// comparing two entries, both valid and not same index
												// ensure primary (i) survives at expense of any identical entries (i2)
												LPSTR i1Ref = tempRefs_s[i1].Get();
												LPSTR i2Ref = tempRefs_s[i2].Get();
												if (stricmp(i1Ref, i2Ref) == NULL)
												{
													// remove duplicate
													pbKeepInTable[i2] = false;

													// also delete WAV and LIP files associated with this duplicate
													char pWAVItem[1001];
													strcpy(pWAVItem, "");
													char pRefItem[1001];
													strcpy(pRefItem, tempLines_s[i2].Get());
													for (int n = 0; n < strlen(pRefItem); n++)
													{
														if (pRefItem[n] == 9)
														{
															strcpy(pWAVItem, pRefItem+n+1);
															break;
														}
													}
													if (FileExist(pWAVItem) == 1) DeleteFileA(pWAVItem);
													char pLIPItem[1001];
													strcpy(pLIPItem, pWAVItem); 
													pLIPItem[strlen(pLIPItem) - 4] = 0;
													strcat(pLIPItem, ".lip");
													if (FileExist(pLIPItem) == 1) DeleteFileA(pLIPItem);

													// and finally reroute entities that used this WAV to the primary one
													for (int e2 = 1; e2 <= g.entityelementlist; e2++)
													{
														for (int s2 = 0; s2 < 4; s2++)
														{
															LPSTR pThisSlotsWAV = NULL;
															if (s2 == 0) pThisSlotsWAV = t.entityelement[e2].eleprof.soundset_s.Get();
															if (s2 == 1) pThisSlotsWAV = t.entityelement[e2].eleprof.soundset1_s.Get();
															if (s2 == 2) pThisSlotsWAV = t.entityelement[e2].eleprof.soundset2_s.Get();
															if (s2 == 3) pThisSlotsWAV = t.entityelement[e2].eleprof.soundset3_s.Get();
															if (pThisSlotsWAV)
															{
																if (stricmp(pThisSlotsWAV, pWAVItem) == NULL)
																{
																	cstr newWAV_s = (LPSTR)tempLines_s[i1].Get()+strlen(tempRefs_s[i1].Get());
																	if (s2 == 0) t.entityelement[e2].eleprof.soundset_s = newWAV_s;
																	if (s2 == 1) t.entityelement[e2].eleprof.soundset1_s = newWAV_s;
																	if (s2 == 2) t.entityelement[e2].eleprof.soundset2_s = newWAV_s;
																	if (s2 == 3) t.entityelement[e2].eleprof.soundset3_s = newWAV_s;
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

							// final creation of table
							LPSTR pTTSTableFile = "ttstable.txt";
							if (FileExist(pTTSTableFile) == 1) DeleteFileA(pTTSTableFile);
							OpenToWrite(1, pTTSTableFile);
							for (int i = 0; i < iTTSTableMax; i++)
							{
								if (pbKeepInTable[i] == true)
								{
									// keep this in table
									WriteString(1, tempLines_s[i].Get());
								}
								else
								{
									// remove from table - and also delete the associated WAV file
									char pWAVItem[1001];
									strcpy(pWAVItem, "");
									char pRefItem[1001];
									strcpy(pRefItem, tempLines_s[i].Get());
									for (int n = 0; n < strlen(pRefItem); n++)
									{
										if (pRefItem[n] == 9)
										{
											strcpy(pWAVItem, pRefItem+n+1);
											break;
										}
									}
									if (FileExist(pWAVItem) == 1) DeleteFileA(pWAVItem);
									char pLIPItem[1001];
									strcpy(pLIPItem, pWAVItem); 
									pLIPItem[strlen(pLIPItem) - 4] = 0;
									strcat(pLIPItem, ".lip");
									if (FileExist(pLIPItem) == 1) DeleteFileA(pLIPItem);
								}
							}
							CloseFile(1);
						}

						// and free resources
						SAFE_DELETE(pbKeepInTable);

						// restore original folder when finished
						SetDir(pOldDir.Get());

						// trigger a main string update
						bUpdateMainString = true;
					}

					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Enter Text to Speak");

					ImGui::PopItemWidth();

				}
			}
		}

		//Update DLUA if changed. this is stored in soundset4_s.
		//Update soundset4_s when we have changes.
		entityeleproftype *tmpeleprof = edit_grideleprof;
		if (bUpdateMainString) 
		{
			cstr sLuaScriptName = tmpeleprof->PropertiesVariable.VariableScript;
			sLuaScriptName += "_properties(";
			//Check if we need to update with new default values.
			if (tmpeleprof->PropertiesVariable.iVariables > 0) 
			{
				tmpeleprof->soundset4_s = sLuaScriptName;
				//Add varables.
				for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) 
				{

					char val[3];
					val[0] = tmpeleprof->PropertiesVariable.VariableType[i] + '0';
					val[1] = 0;

					tmpeleprof->soundset4_s += val;
					tmpeleprof->soundset4_s += "\"";
					std::string clean_string = tmpeleprof->PropertiesVariable.VariableValue[i];
					replaceAll(clean_string, "\"", ""); //cant use "
					tmpeleprof->soundset4_s += (char *) clean_string.c_str();
					//tmpeleprof->soundset4_s += tmpeleprof->PropertiesVariable.VariableValue[i];
					tmpeleprof->soundset4_s += "\"";
					if (i < tmpeleprof->PropertiesVariable.iVariables - 1)
						tmpeleprof->soundset4_s += ",";
				}
				tmpeleprof->soundset4_s += ")";
			}
		}

		ImGui::Indent(-10);
	}
}

void RedockWindow(char *name)
{
	if (refresh_gui_docking >= 4 ) {

		ImGuiID dockspace_id;

		//dock it.
		int winNodeId = ImGui::GetWindowDockID();
		dockspace_id = ImGui::GetID("MyDockspace");
		ImGui::DockBuilderDockWindow(name, dock_tools_windows);
		int winNodeId2 = ImGui::GetWindowDockID();
		if (winNodeId != 0 && winNodeId2 != 0 && winNodeId != winNodeId2) {
			//Somthing wrong we cant rebuild.
			//Change window size to normal and undock:
		}
		else {
			ImGui::DockBuilderFinish(dockspace_id);
		}
	}
}

int pehuntingbug = 0;
void CheckMinimumDockSpaceSize(float minsize)
{
	//Only make these change when not resizing/moving ...
	int mcursor = ImGui::GetMouseCursor();
	if (mcursor == 0 || !ImGui::IsAnyMouseDown() ) {
		if (ImGui::IsWindowDocked()) {
			int winNodeId = ImGui::GetWindowDockID();
			ImGuiDockNode* testnode = ImGui::DockBuilderGetNode(winNodeId);
			if (testnode->Size.x < minsize && testnode->Size.y > 80.0f ) {
				//Something wrong , adjust.
				if (minsize == 50.0f) {
					//PE: Found it this happens when you use minimize :) , just need to check for that.

					//PE: Bug left panel get small. add breakpoint here to check.
					pehuntingbug = 1;
				}
				//PE: When i got it this was the values ? 12,32
				//PE: testnode->SizeRef was the correct values.
				if (testnode->Size.x != 12 && testnode->Size.x != 13 && testnode->Size.y != 32) {
					testnode->Size.x = minsize;
					testnode->SizeRef.x = minsize;
				}
			}
			if (testnode->Size.y < 20.0f) {
				//Something wrong , adjust.
				if (minsize == 50.0f) {
					//PE: Bug left panel get small. add breakpoint here to check.
					pehuntingbug = 2;
				}
				if (testnode->Size.x != 12 && testnode->Size.x != 13 && testnode->Size.y != 32) {
					testnode->Size.y = 50.0f;
					testnode->SizeRef.y = 50.0f;
				}
			}
		}
	}
}


#define TABPAGEWEATHER 1

#include "..\..\Guru-WickedMAX\master.h"
#include <direct.h>

extern wiECS::Entity g_weatherEntityID;
extern MasterRenderer * master_renderer;

//Visuals
void tab_tab_Column_text(char *text,float fColumn)
{
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
	ImGui::Text(text);
	ImGui::SameLine();
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
	ImGui::SetCursorPos(ImVec2(fColumn, ImGui::GetCursorPosY()));
}

static bool FirstMsOverThreshold(const char* line_begin, const char* line_end, float threshold_ms)
{
	// Find the FIRST "ms" in the line
	const char* ms_pos = nullptr;
	for (const char* p = line_begin; p + 1 < line_end; ++p)
	{
		if (p[0] == 'm' && p[1] == 's')
		{
			ms_pos = p;
			break;
		}
	}
	if (!ms_pos)
		return false;

	// Step back over any whitespace before "ms"
	const char* num_end = ms_pos;
	while (num_end > line_begin && std::isspace(static_cast<unsigned char>(num_end[-1])))
		--num_end;

	// Step back over digits/decimal point to find number start
	const char* num_start = num_end;
	while (num_start > line_begin)
	{
		char c = num_start[-1];
		if (std::isdigit(static_cast<unsigned char>(c)) || c == '.')
			--num_start;
		else
			break;
	}

	if (num_start >= num_end)
		return false;

	// Parse number without allocations
	char tmp[32];
	size_t len = static_cast<size_t>(num_end - num_start);
	if (len >= sizeof(tmp))
		return false;

	std::memcpy(tmp, num_start, len);
	tmp[len] = '\0';

	char* endptr = nullptr;
	float v = std::strtof(tmp, &endptr);
	if (endptr == tmp)
		return false;

	return v > threshold_ms;
}

// Known children of "Update - Wicked" (ranges created inside Scene::Update / RenderPath3D::Update)
static const char* s_WickedChildren[] = {
	"Animations", "Animation Dependencies", "Frustum Culling", "Physics",
	"Procedural Animations", "Script Components", "Spring Dependencies",
	"Spline Update", "Input", "GUI Update", "Update Buffers (CPU)",
	"Shadowmap packing", "Shadowmap Rendering",
};

// Known children of "Render"
static const char* s_RenderChildren[] = {
	"Shadowmap Rendering", "Shadowmap packing",
};

static bool IsChildOf(const char* name, const char** list, int count)
{
	for (int i = 0; i < count; i++)
		if (strcmp(name, list[i]) == 0) return true;
	return false;
}

// Extract the entry name from a profiler line like "\tName: X.XX ms" or "\tName (2x): X.XX ms"
// Returns empty string if not a valid entry line.
static std::string ExtractEntryName(const char* lineBegin, const char* lineEnd)
{
	//if (lineEnd <= lineBegin || *lineBegin != '\t') return "";
	//const char* content = lineBegin + 1; // skip tab
	if (lineEnd <= lineBegin ) return "";
	const char* content = lineBegin;
	if (*lineBegin == '\t')
	{
		content = lineBegin + 1; // skip tab
	}

	// find last ':'
	const char* lastColon = nullptr;
	for (const char* p = lineEnd - 1; p >= content; --p)
	{
		if (*p == ':') { lastColon = p; break; }
	}
	if (!lastColon) return "";
	// name is from content to lastColon, strip hit count " (Nx)"
	std::string name(content, lastColon);
	size_t parenPos = name.rfind(" (");
	if (parenPos != std::string::npos) name = name.substr(0, parenPos);
	return name;
}

void DrawProfilerDataColored_FirstMsOnly()
{
	// we do not want to miss sub-ranges do we :)
	extern std::string GGPerf_GetCachedProfilerText();
	const std::string profiler_data = GGPerf_GetCachedProfilerText();// wi::profiler::GetTextData();

	const ImVec4 white = ImVec4(1, 1, 1, 1);
	const ImVec4 yellow = ImVec4(1, 1, 0, 1);
	const ImVec4 grey = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

	const char* text = profiler_data.c_str();
	const char* line_begin = text;

	for (const char* p = text;; ++p)
	{
		if (*p == '\n' || *p == '\0')
		{
			const char* line_end = p;

			if (line_end > line_begin)
			{
				// line item name
				std::string entryName = ExtractEntryName(line_begin, line_end);

				// skip the double-counting update range item!
				if (entryName == "Update")
				{
					line_begin = p + 1;
					continue;
				}

				// Determine indentation based on hierarchy
				int indent = 0;
				if (line_begin[0] == '\t') // is a child entry (has tab prefix)
					indent = 1; // default: one level under CPU/GPU Frame header

				// Build display line with indentation
				const char* displayStart = line_begin;
				if (line_begin[0] == '\t') displayStart++; // skip original tab
				std::string displayLine;
				displayLine = std::string(displayStart);
				int iEndOfLineCount = p - displayStart;
				displayLine[iEndOfLineCount] = 0;

				// Add indentation prefix
				const char* indentStr = (indent >= 2) ? "    " : (indent == 1) ? "  " : "";
				std::string finalLine = std::string(indentStr) + displayLine;

				const bool slow = FirstMsOverThreshold(finalLine.c_str(), finalLine.c_str() + finalLine.size(), 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, slow ? yellow : white);
				ImGui::TextUnformatted(finalLine.c_str());
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::TextUnformatted("");
			}

			if (*p == '\0')
				break;

			line_begin = p + 1;
		}
	}
}

// My own performance panel
static void DisplayPerformanceData(bool* p_open)
{
	const float DISTANCE = 10.0f;
	static int corner = 0;
	ImGuiIO& io = ImGui::GetIO();
	if (corner != -1)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 window_pos = ImVec2((corner & 1) ? (viewport->Pos.x + viewport->Size.x - DISTANCE) : (viewport->Pos.x + DISTANCE), (corner & 2) ? (viewport->Pos.y + viewport->Size.y - DISTANCE) : (viewport->Pos.y + DISTANCE));
		ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
		float window_width = 20 * ImGui::GetFontSize();
		ImGui::SetNextWindowSize(ImVec2(window_width, viewPortSize.y - 4.0), ImGuiCond_Once);
	}
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

	int winflag = iGenralWindowsFlags;
	winflag |= ImGuiWindowFlags_NoMove;
	winflag |= ImGuiWindowFlags_NoResize;
	if (ImGui::Begin("Performance data##DisplayPerformanceData", p_open, winflag)) //(corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		// draw performance data info
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("FPS: %.1f (DirectX 12)", ImGui::GetIO().Framerate);
		ImGui::Separator();

		// coloured performance metrics!
		DrawProfilerDataColored_FirstMsOnly();

		// end of panel
		ImGui::Separator();
		ImGui::Text("");
	}
	ImGui::End();
}

void imgui_Customize_Water(int mode);

// build up data structure we need
struct sLeafNode
{
	int iInstructionIndex;
	int iUniqueSignatureCode;
	int iState;
	int iCondition;
	char pConditionParam1[250];
	char pConditionParam2[250];
	int iAction;
	char pActionParam1[250];
	char pActionParam2[250];
	sLeafNode* pGoToInstruction;
	ImVec2 vReturnPointPos;
	float fWidthRequired;
	sLeafNode* pParent;
	sLeafNode* pAlso;
	sLeafNode* pElse;
};

struct sStateNode
{
	char pName[250];
	int iStateIndex;
	sLeafNode* instruction_root;
	bool bAllowInterupt;
	bool bRecalcRightMost;
	float fRightMostX;
};
std::vector<sStateNode> instruction_state_list;

bool instruction_createstate = false;
char instruction_newstatename[256];
int instruction_deletestate = -1;

float instruction_block_width;
float instruction_block_height;
float instruction_vertical_gap;
float instruction_border;
float instruction_centerline;
float instruction_centerline_absolutex;
ImVec2 instruction_furthestcursor;
bool instruction_regenerateinstructionindices = false;
sLeafNode* instruction_deletethis = NULL;
sStateNode* instruction_deleteinthisstate = NULL;
int instruction_pickstateorinstruction = 0;
sLeafNode* instruction_pickaninstruction = NULL;
sLeafNode* instruction_hoveringover = NULL;
int instruction_picknewendnode = 0;
sLeafNode* instruction_pickanewnode = NULL;
std::vector<sLeafNode*> instruction_singlelist;
bool instruction_freezewheneditingbehavior = false;
int instruction_running_e = 0;
int instruction_running_index = 0;
char instruction_objectscriptbeingedited[256] = { 0 };
bool instruction_recreatebehaviorlist = true;

void gridedit_restartanybehaviorediting()
{
	strcpy (instruction_objectscriptbeingedited, "");
	instruction_recreatebehaviorlist = true;
}

void gridedit_refreshallinstructionindices ( int iStateIndex, sLeafNode* pThis, int* piInstructionCount)
{
	pThis->iState = iStateIndex;
	pThis->iInstructionIndex = *piInstructionCount;
	instruction_singlelist.push_back(pThis);
	*piInstructionCount = *piInstructionCount + 1;
	if (pThis->pAlso) gridedit_refreshallinstructionindices(iStateIndex, pThis->pAlso, piInstructionCount);
	if (pThis->pElse) gridedit_refreshallinstructionindices(iStateIndex, pThis->pElse, piInstructionCount);
}

int gridedit_generateuniqueinstructionindices ( void )
{
	// build single list of all instructions from above nodes
	instruction_singlelist.clear();

	// also, go through all states, re-order instruction indices
	int iInstructionCount = 1;
	for (int iStateIndex = 0; iStateIndex < instruction_state_list.size(); iStateIndex++)
	{
		gridedit_refreshallinstructionindices(1+iStateIndex, instruction_state_list[iStateIndex].instruction_root, &iInstructionCount);
	}

	// return number of instructions
	return iInstructionCount - 1;
}

bool gridedit_instruction_inthisstate_rec (sLeafNode* pinstruction)
{
	bool bResult = false;
	if (pinstruction->iInstructionIndex == 1 + instruction_running_index)
	{
		// the instruction is in THIS state
		return true;
	}
	else
	{
		// keep looking
		bool bResult = false;
		if (pinstruction->pAlso) bResult = gridedit_instruction_inthisstate_rec(pinstruction->pAlso);
		if (pinstruction->pElse && bResult==false) bResult = gridedit_instruction_inthisstate_rec(pinstruction->pElse);
		return bResult;
	}
}

float gridedit_instruction_calculatewidth_rec (sLeafNode* pinstruction)
{
	float fTotalWidthNeededForChildren = 0.0f;
	if (pinstruction->pAlso==NULL && pinstruction->pElse==NULL)
	{
		// leaf node
		fTotalWidthNeededForChildren += 0.55f + 0.05f;
	}
	else
	{
		// new system needed, this is a little hard to navigate when have LARGE behaviors!!
		if (pinstruction->pAlso) fTotalWidthNeededForChildren += gridedit_instruction_calculatewidth_rec(pinstruction->pAlso);
		if (pinstruction->pElse) fTotalWidthNeededForChildren += gridedit_instruction_calculatewidth_rec(pinstruction->pElse);
	}
	pinstruction->fWidthRequired = fTotalWidthNeededForChildren;
	return fTotalWidthNeededForChildren;
}

struct sCondActTable
{
	int iID;
	cstr sLabel;
	cstr sTip;
};
std::vector<sCondActTable> g_ConditionsTable;
std::vector<sCondActTable> g_ActionsTable;
char** combo_conditions = NULL;
int* combo_conditions_lookup = NULL;
char** combo_actions = NULL;
int* combo_actions_lookup = NULL;
int combo_animations_count = 0;
char** combo_animations = NULL;

void gridedit_instruction_parseandpopulateinstructions (void)
{
	// free any previous list
	if (combo_conditions)
	{
		for (int i = 0; i < g_ConditionsTable.size(); i++)
		{
			delete combo_conditions[i];
		}
		delete[] combo_conditions;
		delete[] combo_conditions_lookup;
	}
	if (combo_actions)
	{
		for (int i = 0; i < g_ActionsTable.size(); i++)
		{
			delete combo_actions[i];
		}
		delete[] combo_actions;
		delete[] combo_actions_lookup;
	}

	// clear lists
	g_ConditionsTable.clear();
	g_ActionsTable.clear();

	// condition list has extra ability
	sCondActTable deleteitem;
	deleteitem.iID = 0;
	deleteitem.sLabel = "(delete instruction)";
	deleteitem.sTip = "Select this to delete the current instruction block";
	g_ConditionsTable.push_back(deleteitem);
	sCondActTable moveitem;
	moveitem.iID = 1;
	moveitem.sLabel = "(move instruction)";
	moveitem.sTip = "Select this to move the current instruction block chain to a new position in the state flow";
	g_ConditionsTable.push_back(moveitem);

	// parse 'masterinterpreter.lua' and extract condition and action ordinals, labels and tooltips
	char pMasterInterpreterFile[MAX_PATH];
	sprintf(pMasterInterpreterFile, "scriptbank\\masterinterpreter.lua");
	OpenToRead(1, pMasterInterpreterFile);
	while (FileEnd(1) == 0)
	{
		// get line by line
		cstr line_s = ReadString(1);
		LPSTR pLine = line_s.Get();

		// extract conditions and actions
		int iParseThisLine = 0;
		LPSTR pFind = "g_masterinterpreter_cond_"; if (strnicmp (pLine, pFind, strlen(pFind)) == NULL) iParseThisLine = 1;
		pFind = "g_masterinterpreter_act_"; if (strnicmp (pLine, pFind, strlen(pFind)) == NULL) iParseThisLine = 2;
		if (iParseThisLine > 0)
		{
			LPSTR pEndOfCondActVar = strstr(pLine, " = ");
			if (pEndOfCondActVar)
			{
				LPSTR pStartOfLabel = strstr(pEndOfCondActVar, "-- ");
				if (pStartOfLabel)
				{
					LPSTR pOpenBracket = strstr(pStartOfLabel, "(");
					if (pOpenBracket)
					{
						LPSTR pCloseBracket = strstr(pOpenBracket, ")");
						if (pCloseBracket)
						{
							// line is valid, parse into entries
							char pValue[32];
							memset(pValue, 0, 32);
							pEndOfCondActVar += 3;
							memcpy(pValue, pEndOfCondActVar, pStartOfLabel - pEndOfCondActVar);
							char pLabel[256];
							memset(pLabel, 0, 256);
							pStartOfLabel += 3;
							memcpy(pLabel, pStartOfLabel, pOpenBracket - pStartOfLabel);
							char pTip[1024];
							memset(pTip, 0, 1024);
							pOpenBracket += 1;
							memcpy(pTip, pOpenBracket, pCloseBracket - pOpenBracket);

							// create item
							sCondActTable item;
							item.iID = atoi(pValue);
							item.sLabel = pLabel;
							item.sTip = pTip;

							// add to list
							if (iParseThisLine == 1) g_ConditionsTable.push_back(item);
							if (iParseThisLine == 2) g_ActionsTable.push_back(item);
						}
					}
				}
			}
		}
	}
	CloseFile(1);

	// create new lists
	combo_conditions_lookup = new int[g_ConditionsTable.size()];
	combo_conditions = new char*[g_ConditionsTable.size()];
	for (int i = 0; i < g_ConditionsTable.size(); i++)
	{
		combo_conditions_lookup[i] = g_ConditionsTable[i].iID;
		combo_conditions[i] = new char[256];
		strcpy (combo_conditions[i], g_ConditionsTable[i].sLabel.Get());
	}
	combo_actions_lookup = new int[g_ActionsTable.size()];
	combo_actions = new char*[g_ActionsTable.size()];
	for (int i = 0; i < g_ActionsTable.size(); i++)
	{
		combo_actions_lookup[i] = g_ActionsTable[i].iID;
		combo_actions[i] = new char[256];
		strcpy (combo_actions[i], g_ActionsTable[i].sLabel.Get());
	}
}

void gridedit_instruction_populateanimationlist ( int iObj )
{
	// free any previous animation list
	if (combo_animations)
	{
		for (int i = 0; i < combo_animations_count; i++)
		{
			delete combo_animations[i];
		}
		delete[] combo_animations;
		combo_animations = NULL;
	}

	// create animation list
	combo_animations_count = 0;
	if (iObj > 0)
	{
		sObject* pObject = GetObjectData(iObj);
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		while (pAnimSet)
		{
			combo_animations_count++;
			pAnimSet = pAnimSet->pNext;
		}
		combo_animations = new char*[combo_animations_count];
		combo_animations_count = 0;
		pAnimSet = pObject->pAnimationSet;
		while (pAnimSet)
		{
			combo_animations[combo_animations_count] = new char[256];
			strcpy (combo_animations[combo_animations_count], pAnimSet->szName);
			combo_animations_count++;
			pAnimSet = pAnimSet->pNext;
		}
	}
}

void gridedit_instruction_block_rec ( sStateNode* pState, ImVec2 vTopCenterPos, LPSTR pStateName, sLeafNode* pinstruction, float fMargin, int iRowIndex)
{
	// Insert instruction block
	float fAbsInstructionLeftX = vTopCenterPos.x;// -(instruction_block_width / 2);
	ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, vTopCenterPos.y));
	ImVec2 vInstructionPos = ImGui::GetCurrentWindow()->DC.CursorPos;
	ImVec2 vOutline = vInstructionPos - ImVec2(instruction_border, instruction_border);
	const ImRect instruction_bb(vOutline.x, vOutline.y, vOutline.x + instruction_block_width + (instruction_border * 2), vOutline.y + instruction_block_height + (instruction_border * 2));
	pinstruction->vReturnPointPos = ImVec2(vOutline.x, vOutline.y + (instruction_block_height / 2));

	// record left-most block (so can shift whole state to the left for more room)
	if (pState->bRecalcRightMost == true)
	{
		float fRightSideOfBlock = fAbsInstructionLeftX + instruction_block_width;
		if (pState->fRightMostX < fRightSideOfBlock) pState->fRightMostX = fRightSideOfBlock;
	}

	// In instruction pick mode, subdue colors
	ImGuiIO& io = ImGui::GetIO();
	ImVec4 block_col = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	ImVec4 block_col_border = ImVec4(1.0f, 1.0f, 1.0f, 0.75f);
	if (instruction_pickaninstruction != NULL)
	{
		block_col = ImVec4(0.6f, 0.6f, 0.6f, 0.1f);
		block_col_border = ImVec4(0.6f, 0.6f, 0.6f, 0.5f);
		bool bHoveringOver = false;
		if (io.MousePos.x > instruction_bb.Min.x && io.MousePos.x < instruction_bb.Max.x && io.MousePos.y > instruction_bb.Min.y && io.MousePos.y < instruction_bb.Max.y)
		{
			if (instruction_pickstateorinstruction == 1)
			{
				// pick anything from any other state
				if (instruction_pickaninstruction->iState != pinstruction->iState)
				{
					block_col = ImVec4(1.0f, 1.0f, 1.0f, 0.4f);
					block_col_border = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
					instruction_hoveringover = pinstruction;
					bHoveringOver = true;
				}
			}
			if (instruction_pickstateorinstruction == 2)
			{
				// pick an instruction in same state
				if (instruction_pickaninstruction->iState == pinstruction->iState)
				{
					block_col = ImVec4(1.0f, 1.0f, 1.0f, 0.4f);
					block_col_border = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
					instruction_hoveringover = pinstruction;
					bHoveringOver = true;
				}
			}
		}
	}
	else
	{
		// when logic running, can highlight currently active instruction
		if (instruction_freezewheneditingbehavior == false)
		{
			if (instruction_running_e > 0)
			{
				if (instruction_running_index > 0)
				{
					if (pinstruction->iInstructionIndex == 1 + instruction_running_index)
					{
						block_col = ImVec4(1.0f, 1.0f, 0.6f, 0.6f);
						block_col_border = ImVec4(1.0f, 1.0f, 0.6f, 0.9f);
					}
				}
			}
		}
	}

	// Draw outline of instruction block
	ImGui::GetCurrentWindow()->DrawList->AddRectFilled(instruction_bb.Min, instruction_bb.Max, ImGui::GetColorU32(block_col), 0.0f, 15);
	ImGui::GetCurrentWindow()->DrawList->AddRect(instruction_bb.Min, instruction_bb.Max, ImGui::GetColorU32(block_col_border), 0.0f, 15, 2.0f);

	// condition combo
	char pInstructionDisplay[1024];
	sprintf(pInstructionDisplay, "##BehaviorEditorCondition%s%d", pStateName, pinstruction->iInstructionIndex);
	ImGui::PushItemWidth(instruction_block_width);
	int iPreviousConditionValue = pinstruction->iCondition;
	int iConditionListIndex = 0;
	for (int iFind = 0; iFind < g_ConditionsTable.size(); iFind++)
	{
		if (combo_conditions_lookup[iFind] == pinstruction->iCondition) iConditionListIndex = iFind;
	}
	if (ImGui::Combo(pInstructionDisplay, &iConditionListIndex, combo_conditions, g_ConditionsTable.size()))
	{
		pinstruction->iCondition = combo_conditions_lookup[iConditionListIndex];
		if (pinstruction->iCondition == 0)
		{
			// special case - until newer UI idea comes along, delete this instruction
			instruction_deletethis = pinstruction;
			instruction_deleteinthisstate = pState;
			pinstruction->iCondition = iPreviousConditionValue;
		}
		if (pinstruction->iCondition == 1)
		{
			// special case - select a new location for this instruction to be connected to
			instruction_picknewendnode = 1;
			instruction_pickanewnode = pinstruction;
			pinstruction->iCondition = iPreviousConditionValue;
		}
		instruction_freezewheneditingbehavior = true;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip(combo_conditions[iConditionListIndex]);
	// optional condition param
	sprintf(pInstructionDisplay, "##BehaviorEditorConditionParam1%s%d", pStateName, pinstruction->iInstructionIndex);
	ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y));
	if (ImGui::InputText(pInstructionDisplay, &pinstruction->pConditionParam1[0], 250, ImGuiInputTextFlags_None))
	{
		instruction_freezewheneditingbehavior = true;
	}
	// action combo
	sprintf(pInstructionDisplay, "##BehaviorEditorAction%s%d", pStateName, pinstruction->iInstructionIndex);
	ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y));
	int iPreviousActionValue = pinstruction->iAction;
	int iActionListIndex = 0;
	for (int iFind = 0; iFind < g_ActionsTable.size(); iFind++)
	{
		if (combo_actions_lookup[iFind] == pinstruction->iAction) iActionListIndex = iFind;
	}
	if (ImGui::Combo(pInstructionDisplay, &iActionListIndex, combo_actions, g_ActionsTable.size()))
	{
		pinstruction->iAction = combo_actions_lookup[iActionListIndex];

		// special instruction index change
		if (pinstruction->iAction == 0 || pinstruction->iAction == 1)
		{
			// can only change instruction block if no instructions in ALSO
			if (pinstruction->pAlso == NULL)
			{
				if (pinstruction->iAction == 0)
				{
					// special case - go to state
					instruction_pickstateorinstruction = 1;
					instruction_pickaninstruction = pinstruction;
				}
				if (pinstruction->iAction == 1)
				{
					// special case - go to instruction
					instruction_pickstateorinstruction = 2;
					instruction_pickaninstruction = pinstruction;
				}
			}
			else
			{
				pinstruction->iAction = iPreviousActionValue;
			}
		}
		else
		{
			// regular action
			pinstruction->pGoToInstruction = NULL;
		}
		instruction_freezewheneditingbehavior = true;
	}
	if (g_ActionsTable.size() > 0)
	{
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(combo_actions[iActionListIndex]);
	}
	// optional action param
	if (pinstruction->iAction == 12 || pinstruction->iAction == 13 || pinstruction->iAction == 66 || pinstruction->iAction == 67)
	{
		// play and loop shows animation list for this object
		sprintf(pInstructionDisplay, "##BehaviorEditorActionParam1Combo%s%d", pStateName, pinstruction->iInstructionIndex);
		ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y));
		int iAnimationListIndex = 0;
		for (int iFind = 0; iFind < combo_animations_count; iFind++)
		{
			if ( stricmp (combo_animations[iFind], pinstruction->pActionParam1)==NULL) iAnimationListIndex = iFind;
		}
		if (ImGui::Combo(pInstructionDisplay, &iAnimationListIndex, combo_animations, combo_animations_count))
		{
			strcpy ( pinstruction->pActionParam1, combo_animations[iAnimationListIndex]);
		}
	}
	else
	{
		// regular text field
		sprintf(pInstructionDisplay, "##BehaviorEditorActionParam1%s%d", pStateName, pinstruction->iInstructionIndex);
		ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y));
		if (ImGui::InputText(pInstructionDisplay, &pinstruction->pActionParam1[0], 250, ImGuiInputTextFlags_None))
		{
			instruction_freezewheneditingbehavior = true;
		}
	}
	// labels
	ImVec2 vCursorPos = ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y);
	ImGui::SetCursorPos(vCursorPos + ImVec2(5.0f, 0));
	ImGui::Text("True");
	if (pinstruction->iCondition != 11)
	{
		ImGui::SameLine();
		ImGui::SetCursorPos(vCursorPos + ImVec2(instruction_block_width - 35.0f, 0));
		ImGui::Text("False");
	}
	// lines and add buttons
	float fLineVertGap = ImGui::GetFontSize() / 2.0f;
	ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y+fLineVertGap));
	float fHalfInstructionWidth = instruction_block_width / 2;
	float fAddButtonWidth = instruction_block_width / 3.0f;
	float fButtonMargin = (fHalfInstructionWidth - fAddButtonWidth) / 2.0f;
	float fButtonPosY = ImGui::GetCursorPos().y;
	float fLeftALSOBranchX = 0.0f;
	float fRightELSEBranchX = 0.0f;
	float fVertYPosToNextInstruction = instruction_block_height + 46.0f;
	int buttoncount = 2;
	if ( pinstruction->iCondition == 11) buttoncount = 1;
	for (int buttonindex = 0; buttonindex < buttoncount; buttonindex++)
	{
		float fWhichHalf = buttonindex * fHalfInstructionWidth;
		bool bHaveAnotherInstruction = false;
		if (buttonindex == 0)
		{
			if (pinstruction->pAlso) bHaveAnotherInstruction = true;
		}
		else
		{
			if (pinstruction->pElse) bHaveAnotherInstruction = true;
		}
		float fDestinationAdjustmentX = 0.0f;
		float fDestinationAdjustmentForButtonX = 0.0f;
		if (buttonindex == 0 && pinstruction->pAlso && pinstruction->pElse)
		{
			// going to keep left aligned, and expand right
		}
		if (buttonindex == 1 && pinstruction->pElse)
		{
			if (pinstruction->iAction == 0 || pinstruction->iAction == 1)
			{
				// Go To State and Go To Instruction has no block
			}
			else
			{
				if(pinstruction->pAlso) fDestinationAdjustmentX += ((pinstruction->pAlso->fWidthRequired) * instruction_block_width) * 2.0f;
				fDestinationAdjustmentForButtonX += 10;
			}
		}
		ImVec2 vLineFrom = vInstructionPos + ImVec2(fWhichHalf + (instruction_block_width / 4), instruction_block_height + instruction_border);
		ImVec2 vLineTo = vInstructionPos + ImVec2(fWhichHalf + (instruction_block_width / 4) + fDestinationAdjustmentForButtonX, instruction_block_height + instruction_border + 10);

		bool bFirstLineIsInstructionJump = false;
		if (buttonindex == 0 && pinstruction->pGoToInstruction) bFirstLineIsInstructionJump = true;
		if (bFirstLineIsInstructionJump == true)
		{
			// return to start and goto instruction have shorter vert line to 'miss' the regular line (if on right - phewy)
			vLineTo.y -= 5.0f;
		}

		// line from instruction block to button
		ImGui::GetCurrentWindow()->DrawList->AddLine(vLineFrom, vLineTo, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.75f)), 2.0f);

		if (buttonindex == 0)
			fLeftALSOBranchX = fDestinationAdjustmentX;
		else
			fRightELSEBranchX = fDestinationAdjustmentX;
		if (bFirstLineIsInstructionJump == true)
		{
			// if 'go to' instruction block, protrude a line leftward
			vLineFrom = vLineTo;

			// left side or right side
			ImVec2 vShiftToSide = vInstructionPos - ImVec2(8.0f,0);
			ImVec2 vConnectTo = pinstruction->pGoToInstruction->vReturnPointPos;
			float fDecidingXPos = vLineFrom.x;
			if (buttonindex == 0) fDecidingXPos += instruction_block_width;
			if (fDecidingXPos > instruction_centerline_absolutex)
			{
				vConnectTo.x += instruction_block_width + (instruction_border/2);
			}

			// only complete connection to instruction if within state
			if (pinstruction->iAction == 0)
			{
				// when jump to new state, no connecting line to the new state component (yet)
				vLineTo.x = vShiftToSide.x;
				ImGui::GetCurrentWindow()->DrawList->AddLine(vLineFrom, vLineTo, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)), 2.0f);

				// mark state jump with square
				ImVec2 vEndOfLine = vLineTo;
				vLineFrom = ImVec2(vEndOfLine.x - 5.0f, vEndOfLine.y - 2.5f);
				vLineTo = ImVec2(vEndOfLine.x, vEndOfLine.y + 2.5f);
				ImGui::GetCurrentWindow()->DrawList->AddRect(vLineFrom, vLineTo, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)), 2.0f);
			}
			if (pinstruction->iAction == 1)
			{
				// if 'go to' instruction block, protrude a line left/rightward
				vLineTo.x = vShiftToSide.x;
				ImGui::GetCurrentWindow()->DrawList->AddLine(vLineFrom, vLineTo, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)), 2.0f);

				// then draw a line up or down to destination Y pos
				ImVec2 vLevelWith = ImVec2(vLineTo.x, vConnectTo.y);
				ImGui::GetCurrentWindow()->DrawList->AddLine(vLineTo, vLevelWith, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)), 2.0f);

				// and to destination
				ImGui::GetCurrentWindow()->DrawList->AddLine(vLevelWith, vConnectTo, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)), 2.0f);
			}
		}
		else
		{
			if (bHaveAnotherInstruction == true)
			{
				// insert an instruction at this point
				sprintf(pInstructionDisplay, "+##BehaviorEditorInsert%s%d%d", pStateName, buttonindex, pinstruction->iInstructionIndex);

				// and a line dropping down from [+] button to instruction below
				vLineFrom = vInstructionPos + ImVec2(fWhichHalf + (instruction_block_width / 4) + fDestinationAdjustmentForButtonX, instruction_block_height + instruction_border + 10 + 10);
				vLineTo = vInstructionPos + ImVec2((instruction_block_width / 2) + fDestinationAdjustmentX, fVertYPosToNextInstruction + instruction_border);
				ImGui::GetCurrentWindow()->DrawList->AddLine(vLineFrom, vLineTo, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.75f)), 2.0f);
			}
			else
			{
				// add a new instruction button
				if (buttonindex == 0)
					sprintf(pInstructionDisplay, "T##BehaviorEditorAdd%s%d%d", pStateName, buttonindex, pinstruction->iInstructionIndex);
				else
					sprintf(pInstructionDisplay, "F##BehaviorEditorAdd%s%d%d", pStateName, buttonindex, pinstruction->iInstructionIndex);
			}
			ImGui::SetCursorPos(ImVec2(fButtonMargin + fWhichHalf + fAbsInstructionLeftX + fDestinationAdjustmentForButtonX, fButtonPosY));
			if (ImGui::Button(pInstructionDisplay, ImVec2(fAddButtonWidth, 0)))
			{
				// check if special case with user selecting node to connect instruction to
				if (instruction_picknewendnode == 1)
				{
					// detatch old instruction chain from current connection
					sLeafNode* pOldParent = instruction_pickanewnode->pParent;
					if (pOldParent->pAlso == instruction_pickanewnode) pOldParent->pAlso = NULL;
					if (pOldParent->pElse == instruction_pickanewnode) pOldParent->pElse = NULL;
					instruction_pickanewnode->pParent = NULL;

					// attach instruction chain to new node location
					if (buttonindex == 0)
						pinstruction->pAlso = instruction_pickanewnode;
					else
						pinstruction->pElse = instruction_pickanewnode;
					instruction_pickanewnode->pParent = pinstruction;

					// finished moving instruction chain
					instruction_picknewendnode = 0;
				}
				else
				{
					// create new instruction
					sLeafNode* pNewInstruction = new sLeafNode();
					memset(pNewInstruction, 0, sizeof(sLeafNode));
					pNewInstruction->iUniqueSignatureCode = rand() % 999999;
					pNewInstruction->pParent = pinstruction;
					pNewInstruction->iInstructionIndex = -1;
					pNewInstruction->iState = 1;
					pNewInstruction->iCondition = 11;
					pNewInstruction->iAction = 11;
					if (bHaveAnotherInstruction == true)
					{
						// insert
						if (buttonindex == 0)
						{
							sLeafNode* pExisting = pinstruction->pAlso;
							pinstruction->pAlso = pNewInstruction;
							pNewInstruction->pAlso = pExisting;
							pExisting->pParent = pNewInstruction;
						}
						else
						{
							sLeafNode* pExisting = pinstruction->pElse;
							pinstruction->pElse = pNewInstruction;
							pNewInstruction->pAlso = pExisting;
							pExisting->pParent = pNewInstruction;
						}
					}
					else
					{
						// add
						if (buttonindex == 0)
							pinstruction->pAlso = pNewInstruction;
						else
							pinstruction->pElse = pNewInstruction;
					}
				}

				// when add instruction, old instruction indices now invalid
				instruction_regenerateinstructionindices = true;
				instruction_freezewheneditingbehavior = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select this to add a new instruction to the state.");
		}
	}
	ImGui::SetCursorPos(ImVec2(fAbsInstructionLeftX, ImGui::GetCursorPos().y + fLineVertGap));

	// finished instruction block
	ImGui::PopItemWidth();

	// Follow down tree of instructions
	ImVec2 vShiftPos;
	if (pinstruction->pAlso)
	{
		vShiftPos = ImVec2(fLeftALSOBranchX, fVertYPosToNextInstruction + (instruction_border*2));
		gridedit_instruction_block_rec (pState, vTopCenterPos + vShiftPos, pStateName, pinstruction->pAlso, fMargin, iRowIndex+1);
	}
	if (pinstruction->pElse)
	{
		vShiftPos = ImVec2(fRightELSEBranchX, fVertYPosToNextInstruction + (instruction_border*2));
		gridedit_instruction_block_rec (pState, vTopCenterPos + vShiftPos, pStateName, pinstruction->pElse, fMargin, iRowIndex+1);
	}

	// record deepest Y position as recursion can end on instruction blocks further up
	vCursorPos = ImGui::GetCursorPos();
	if (vCursorPos.y > instruction_furthestcursor.y)
	{
		instruction_furthestcursor = vCursorPos;
	}
}

void gridedit_savebehavior ( LPSTR pByteFilename )
{
	// re-number instruction indices, and create single instruction list
	int iInstructionCount = gridedit_generateuniqueinstructionindices();

	// get real path to script BYC file
	char pByteCodeFile[MAX_PATH];
	strcpy (pByteCodeFile, pByteFilename);
	GG_GetRealPath(pByteCodeFile, 1);
	if (FileExist(pByteCodeFile) == 1) DeleteFileA(pByteCodeFile);

	// latest version for saving
	int iVersionNumber = 102;

	// write new BYC file
	OpenToWrite(1, pByteCodeFile);
	WriteString(1, "42"); // magic number

	// write version number
	char pVersionNumber[250];
	sprintf (pVersionNumber, "%d", iVersionNumber);
	WriteString(1, pVersionNumber); // version number

	// find largest state index in all instructions
	int iStateCount = 0;
	for (int index = 0; index < iInstructionCount; index++)
	{
		int iThisStateIndex = instruction_singlelist[index]->iState;
		if (iThisStateIndex > iStateCount) iStateCount = iThisStateIndex;
	}

	// write state count
	char pStateCount[32];
	sprintf(pStateCount, "%d", iStateCount);
	WriteString(1, pStateCount);

	// write instruction count
	char pInstructionCount[32];
	sprintf(pInstructionCount, "%d", iInstructionCount);
	WriteString(1, pInstructionCount); // instruction count

	// version 101
	char pLine[2048];
	if (iVersionNumber >= 101)
	{
		// all states
		for (int iStateIndex = 0; iStateIndex < iStateCount; iStateIndex++)
		{
			sStateNode* pState = &instruction_state_list[iStateIndex];
			sprintf(pLine, "%s", pState->pName); WriteString(1, pLine);
			sprintf(pLine, "%d", (int)pState->bAllowInterupt); WriteString(1, pLine);
		}
		WriteString(1, "---"); // separator
		// all instuctions
		for (int index = 0; index < iInstructionCount; index++)
		{
			sLeafNode* pThis = instruction_singlelist[index];
			sprintf(pLine, "%d", pThis->iUniqueSignatureCode); WriteString(1, pLine); // UniqueSignatureCode
			sprintf(pLine, "%d", pThis->iState); WriteString(1, pLine); // state
			sprintf(pLine, "%d", pThis->iCondition); WriteString(1, pLine); // condition
			sprintf(pLine, "%s", pThis->pConditionParam1); WriteString(1, pLine); // condition param1
			sprintf(pLine, "%s", pThis->pConditionParam2); WriteString(1, pLine); // condition param2
			sprintf(pLine, "%d", pThis->iAction); WriteString(1, pLine); // action
			sprintf(pLine, "%s", pThis->pActionParam1); WriteString(1, pLine); // action param1
			sprintf(pLine, "%s", pThis->pActionParam2); WriteString(1, pLine); // action param2
			int iAlsoInstructionNumber = 0;	if (pThis->pAlso) iAlsoInstructionNumber = pThis->pAlso->iInstructionIndex;
			int iElseInstructionNumber = 0;	if (pThis->pElse) iElseInstructionNumber = pThis->pElse->iInstructionIndex;
			if ((pThis->iAction == 0 || pThis->iAction == 1) && pThis->pGoToInstruction)
			{
				// store goto instruction index in ALSO (does same thing in the end as ALSO)
				iAlsoInstructionNumber = pThis->pGoToInstruction->iInstructionIndex;
			}
			sprintf(pLine, "%d", iAlsoInstructionNumber); WriteString(1, pLine); // also
			sprintf(pLine, "%d", iElseInstructionNumber); WriteString(1, pLine); // else
			WriteString(1, "---"); // separator
		}
	}
	if (iVersionNumber >= 102)
	{
		// save the instruction index from the root of the damage state (if any)
		int iDamageStateRootInstructionIndex = -1;
		for (int iStateIndex = 0; iStateIndex < iStateCount; iStateIndex++)
		{
			sStateNode* pState = &instruction_state_list[iStateIndex];
			if (stricmp (pState->pName, "damage") == NULL)
			{
				iDamageStateRootInstructionIndex = pState->instruction_root->iInstructionIndex;
				break;
			}
		}
		sprintf(pLine, "%d", iDamageStateRootInstructionIndex); WriteString(1, pLine);
	}
	CloseFile(1);
}

void gridedit_deletebehaviornodes_rec ( sLeafNode* pThis )
{
	if (pThis == NULL) return;
	gridedit_deletebehaviornodes_rec (pThis->pAlso);
	gridedit_deletebehaviornodes_rec (pThis->pElse);
	pThis->pAlso = NULL;
	pThis->pElse = NULL;
	delete pThis;
}

void gridedit_deletebehavior(void)
{
	for (int iStateIndex = 0; iStateIndex < instruction_state_list.size(); iStateIndex++)
	{
		sStateNode* pState = &instruction_state_list[iStateIndex];
		if (pState->instruction_root)
		{
			gridedit_deletebehaviornodes_rec(pState->instruction_root);
			pState->instruction_root = NULL;
		}
	}
	instruction_state_list.clear();
	instruction_singlelist.clear();
}

bool gridedit_loadbehavior (LPSTR pByteFilename)
{
	// delete any previous node tree from all states and clear single list to populate from BYC file
	gridedit_deletebehavior();

	// get real path to script BYC file
	int* pAlsoTable = NULL;
	int* pElseTable = NULL;
	int iStateCount = 0;
	int iInstructionCount = 0;
	char pByteCodeFile[MAX_PATH];
	strcpy (pByteCodeFile, pByteFilename);
	GG_GetRealPath(pByteCodeFile, 0);
	if (FileExist(pByteCodeFile) == 1)
	{
		// read BYC file
		OpenToRead(1, pByteCodeFile);
		LPSTR pMagicNumber = ReadString(1); // magic number
		LPSTR pVersionNumber = ReadString(1); // version number
		int iVersionNumber = atoi(pVersionNumber);
		LPSTR pStateCount = ReadString(1); // state count
		iStateCount = atoi(pStateCount);
		LPSTR pInstructionCount = ReadString(1); // instruction count
		iInstructionCount = atoi(pInstructionCount);

		// table to store alsoe and else instruction numbers
		pAlsoTable = new int[iInstructionCount];
		memset(pAlsoTable, 0, sizeof(int)*iInstructionCount);
		pElseTable = new int[iInstructionCount];
		memset(pElseTable, 0, sizeof(int)*iInstructionCount);

		// Version 101
		if (iVersionNumber >= 101)
		{
			// all states
			for (int iStateIndex = 0; iStateIndex < iStateCount; iStateIndex++)
			{
				sStateNode state;
				state.iStateIndex = 1 + iStateIndex;
				LPSTR pName = ReadString(1); strcpy (state.pName, pName);
				LPSTR pAllowInterupt = ReadString(1); state.bAllowInterupt = atoi(pAllowInterupt);
				state.instruction_root = NULL;
				instruction_state_list.push_back(state);
			}
			LPSTR pStateSeparator = ReadString(1);
			// all instructions
			for (int index = 0; index < iInstructionCount; index++)
			{
				// create new instruction
				sLeafNode* pThis = new sLeafNode();
				memset(pThis, 0, sizeof(sLeafNode));
				instruction_singlelist.push_back(pThis);

				// read instruction data
				LPSTR pUniqueSignatureCode = ReadString(1); pThis->iUniqueSignatureCode = atoi(pUniqueSignatureCode);
				LPSTR pState = ReadString(1); pThis->iState = atoi(pState);
				LPSTR pCondition = ReadString(1); pThis->iCondition = atoi(pCondition);
				LPSTR pConditionParam1 = ReadString(1); strcpy (pThis->pConditionParam1, pConditionParam1);
				LPSTR pConditionParam2 = ReadString(1); strcpy (pThis->pConditionParam2, pConditionParam2);
				LPSTR pAction = ReadString(1); pThis->iAction = atoi(pAction);
				LPSTR pActionParam1 = ReadString(1); strcpy (pThis->pActionParam1, pActionParam1);
				LPSTR pActionParam2 = ReadString(1); strcpy (pThis->pActionParam2, pActionParam2);
				LPSTR pAlsoInstructionNumber = ReadString(1); pAlsoTable[index] = atoi(pAlsoInstructionNumber);
				LPSTR pElseInstructionNumber = ReadString(1); pElseTable[index] = atoi(pElseInstructionNumber);
				LPSTR pSeparator = ReadString(1);
			}
		}
		CloseFile(1);
	}

	// all nodes created, now construct the tree
	if (iInstructionCount > 0)
	{
		// populate state root instruction
		int iCurrentState = -1;

		// create node tree from behavior single list
		for (int index = 0; index < iInstructionCount; index++)
		{
			// for each instruction in list
			sLeafNode* pInstructionFromList = instruction_singlelist[index];

			// find and connect root instruction to state
			int iThisState = pInstructionFromList->iState - 1;
			if (iThisState > iCurrentState)
			{
				sStateNode* pState = &instruction_state_list[iThisState];
				pState->instruction_root = pInstructionFromList;
				iCurrentState = iThisState;
			}

			// link up nodes
			sLeafNode* pInstructionAlsoRef = NULL; if (pAlsoTable[index] > 0) pInstructionAlsoRef = instruction_singlelist[pAlsoTable[index] - 1];
			sLeafNode* pInstructionElseRef = NULL; if (pElseTable[index] > 0) pInstructionElseRef = instruction_singlelist[pElseTable[index] - 1];
			pInstructionFromList->pAlso = pInstructionAlsoRef;
			pInstructionFromList->pElse = pInstructionElseRef;

			// special case for restart state and goto instruction
			if (pInstructionFromList->iAction == 0 || pInstructionFromList->iAction == 1)
			{
				// move to GOTO instruction and remove from ALSO ref
				pInstructionFromList->pGoToInstruction = pInstructionFromList->pAlso;
				pInstructionFromList->pAlso = NULL;
				pInstructionAlsoRef = NULL;
			}

			// and link up parent ref
			if (pInstructionAlsoRef) pInstructionAlsoRef->pParent = pInstructionFromList;
			if (pInstructionElseRef) pInstructionElseRef->pParent = pInstructionFromList;
		}
	}
	else
	{
		// fresh new behavior (one state, one instruction)
		sLeafNode* pInstruction = new sLeafNode();
		pInstruction->iUniqueSignatureCode = rand() % 999999;
		pInstruction->iInstructionIndex = -1;
		pInstruction->iCondition = 11;
		pInstruction->iAction = 11;
		instruction_singlelist.push_back(pInstruction);
		sStateNode state;
		state.iStateIndex = 1;
		strcpy (state.pName, "Initial");
		state.bAllowInterupt = true;
		state.instruction_root = pInstruction;
		instruction_state_list.push_back(state);
	}

	// and finally generate updated instruction indices
	instruction_regenerateinstructionindices = true;

	// free resources
	if (pAlsoTable)
	{
		delete[] pAlsoTable;
		pAlsoTable = NULL;
	}
	if (pElseTable)
	{
		delete[] pElseTable;
		pElseTable = NULL;
	}

	// when load new behavior, reset running data (debugging)
	instruction_running_index = 0;
	instruction_freezewheneditingbehavior = false;

	// success
	return true;
}

