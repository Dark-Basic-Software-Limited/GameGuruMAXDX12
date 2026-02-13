typedef std::map<int, StoreItems *>   mDownList;
typedef mDownList::iterator	mitDownList;
mDownList download_list;
int files_updated = 0, total_files = 0, files_downloaded=0;
int real_files_updated = 0, real_total_files = 0, real_files_downloaded = 0;
bool bPrintFirstEntry = true;
char StoreWriteFolder[MAX_PATH];
char StoreDocWriteFolder[MAX_PATH];
char StoreAppWriteFolder[MAX_PATH];
int iDownloadLocation = 0;

#include "Common-Keys.h"

#if __has_include("D:\\DEV\\secret-token.h")
#include "D:\\DEV\\secret-token.h"
#else
#define SECRET_TOKEN "none"
#endif


void imgui_download_store( void )
{
	if (!bDownloadStore_Window) return;

	char secret_token[MAX_PATH];
	strcpy(secret_token, SECRET_TOKEN);

	if (iDownloadStoreProgress == 0)
	{
		if (pDownloadStoreData) 
		{
			delete[] pDownloadStoreData;
			pDownloadStoreData = NULL;
			dwDownloadStoreDataSize = 0;
		}
		if (pDownloadStoreChecksumFile) 
		{
			delete[] pDownloadStoreChecksumFile;
			pDownloadStoreChecksumFile = NULL;
		}
		total_files = 0;
		files_updated = 0;
		files_downloaded = 0;

		real_total_files = 0;
		real_files_updated = 0;
		real_files_downloaded = 0;
		bPrintFirstEntry = true;

		//PE: Get Login information
		strcpy(cDownloadStoreSessionToken, "");
		strcpy(cDownloadStoreLoginUrl, "");
		memset(pDataReturned, 0, sizeof(pDataReturned));
		dwDataReturnedSize = 0;
		char cUrl[256];
		sprintf(cUrl, "/api/auth/session/create");

		UINT iError = StoreOpenURLForDataOrFile(NULL, pDataReturned, &dwDataReturnedSize, "", "GET", cUrl, NULL);
		if (iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0)
		{
			// break up response string
			char* pChop;
			strcpy(pDatatmp, "");
			if (( pChop = (char *) pestrcasestr(pDataReturned, "\"success\":true")) != NULL)
			{
				// success
				pChop += strlen("\"success\":true");
				char pSearchForToken[256];
				strcpy(pSearchForToken, "\"login_url\"\0");
				pChop = strstr(pChop, pSearchForToken);
				if (pChop) 
				{
					pChop += strlen(pSearchForToken)+2;
					strcpy(pDatatmp, pChop);
					char* pFindEnd = strstr(pDatatmp, "\"\0");
					if (pFindEnd)
					{
						pDatatmp[pFindEnd - pDatatmp] = 0;
						if (strlen(pDatatmp) < 256) strcpy(cDownloadStoreLoginUrl, pDatatmp);
						pChop = pFindEnd + 1;
					}
				}
				strcpy(pSearchForToken, "\"session_token\"\0");
				pChop = strstr(pChop, pSearchForToken);
				if (pChop) 
				{
					pChop += strlen(pSearchForToken) + 2;
					strcpy(pDatatmp, pChop);
					char* pFindEnd = strstr(pDatatmp, "\"\0");
					pDatatmp[pFindEnd - pDatatmp] = 0;
					if (strlen(pDatatmp) < 256) strcpy(cDownloadStoreSessionToken, pDatatmp);
					pChop = pFindEnd + 1;
				}
			}
		}

		if (strlen(cDownloadStoreLoginUrl) <= 0)
		{
			bDownloadStoreError = true;
		}
		iDownloadStoreProgress++;
	}

	if (iDownloadStoreProgress == 2 && bLoginButtonClicked)
	{
		//PE: Check if user had been logged in. only every 3 sec.
		if (fCheckForLoginTimer < MAXTimer()) 
		{
			fCheckForLoginTimer = MAXTimer() + 3000;
			iCheckForLoginCount++;
			memset(pDataReturned, 0, sizeof(pDataReturned));
			dwDataReturnedSize = 0;
			char cUrl[256];
			sprintf(cUrl, "/api/auth/session/status?token=%s", cDownloadStoreSessionToken);
			UINT iError = StoreOpenURLForDataOrFile(NULL, pDataReturned, &dwDataReturnedSize, "", "GET", cUrl, NULL);
			if (iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0)
			{
				// break up response string
				char* pChop;
				strcpy(pDatatmp, "");
				strcpy(cDownloadStoreUserHash, "");
				strcpy(cDownloadStoreUserId, "");
				if ((pChop = (char *)pestrcasestr(pDataReturned, "\"success\":true")) != NULL)
				{
					// success
					pChop += strlen("\"success\":true");
					char pSearchForToken[256];
					strcpy(pSearchForToken, "\"logged_in\"\0");
					char LoggedIn[256];
					pChop = strstr(pChop, pSearchForToken);
					if (pChop) 
					{
						pChop += strlen(pSearchForToken) + 1;
						strcpy(pDatatmp, pChop);
						char* pFindEnd = strstr(pDatatmp, ",\"\0");
						if (!pFindEnd)
							pFindEnd = strstr(pDatatmp, "\"\0");
						if (pFindEnd)
						{
							pDatatmp[pFindEnd - pDatatmp] = 0;
							if (strlen(pDatatmp) < 256) strcpy(LoggedIn, pDatatmp);
							pChop = pFindEnd+1;
						}
					}
					//Are we logged in ?
					if (pestrcasestr(LoggedIn, "true") || pestrcasestr(LoggedIn, "1"))
					{
						strcpy(pSearchForToken, "user_id\"\0");
						pChop = strstr(pChop, pSearchForToken);
						if (pChop) 
						{
							pChop += strlen(pSearchForToken) + 1;
							strcpy(pDatatmp, pChop);
							char* pFindEnd = strstr(pDatatmp, ",\"\0");
							if(!pFindEnd)
								pFindEnd = strstr(pDatatmp, "\"\0");
							if (pFindEnd) 
							{
								pDatatmp[pFindEnd - pDatatmp] = 0;
								if (strlen(pDatatmp) < 256) strcpy(cDownloadStoreUserId, pDatatmp);
								pChop = pFindEnd + 1;
							}
						}

						if (pChop) 
						{
							strcpy(pSearchForToken, "user_hash\"\0");
							pChop = strstr(pChop, pSearchForToken);
							if (pChop) 
							{
								pChop += strlen(pSearchForToken) + 2;
								strcpy(pDatatmp, pChop);
								char* pFindEnd = strstr(pDatatmp, "\"\0");
								if (pFindEnd) 
								{
									pDatatmp[pFindEnd - pDatatmp] = 0;
									if (strlen(pDatatmp) < 256) strcpy(cDownloadStoreUserHash, pDatatmp);
									pChop = pFindEnd + 1;
								}
							}
						}
						if (strlen(cDownloadStoreUserId) <= 0) 
						{
							//PE: Failed.
							bDownloadStoreError = true;
						}
						else
						{
							iDownloadStoreProgress++;
						}
					}
				}
			}
		}
	}

	if (iDownloadStoreProgress == 4 && !bDownloadStoreError )
	{
		sprintf(pDownloadFile, "downloads\\storedownload.lst");

		if (FileExist(pDownloadFile) == 1)
			DeleteAFile(pDownloadFile);

		memset(pDataReturned, 0, sizeof(pDataReturned));
		dwDataReturnedSize = 0;
		char cUrl[1024];

		sprintf(cUrl, "/api/purchases/download?user_id=%s&hash=%s&app=GGMax", cDownloadStoreUserId, cDownloadStoreUserHash);
		UINT iError = StoreOpenURLForDataOrFile(NULL, pDataReturned, &dwDataReturnedSize, "", "GET", cUrl, pDownloadFile);
		if (iError > 0)
		{
			//PE:
			if (iError == ERROR_ALREADY_EXISTS)
			{
				//PE: This some times happens for some reason.
				//PE: Retry seams to fix it so.
				static int iRetryDownloadList = 0;
				if (iRetryDownloadList++ < 3) 
				{
					iDownloadStoreProgress = 0;
				}
				else 
				{
					bDownloadStoreError = true;
				}
			}
			else
				bDownloadStoreError = true;
		}
	}

	if (bDownloadStore_Window)
	{
		ImGui::SetNextWindowSize(ImVec2(50 * ImGui::GetFontSize(), 35 * ImGui::GetFontSize()), ImGuiCond_Once);
		ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
		ImGui::Begin("Game Creator Store Downloader##DownloadStoreWindow", &bDownloadStore_Window, 0);

		// A: present user with sequence they should follow in order (top to bottom)
		ImGui::Indent(10);
		ImGui::SetWindowFontScale(0.2);
		ImGui::Text("");
		ImGui::SetWindowFontScale(1.25);
		ImGui::Text("About This Utility");
		ImGui::SetWindowFontScale(1.0);
		// logo
		ImVec2 vStoreCurPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(vStoreCurPos + ImVec2(475, -10));
		ImGui::ImgBtn(MARKETPLACE_GCSTORE, ImVec2(460 / 2, 215 / 2), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1), 0, 0, 0, 0, false, false, false, false, false, false);
		ImGui::SetCursorPos(vStoreCurPos + ImVec2(475, (215 / 2)-5));
		if (ImGui::StyleButton("Register", ImVec2(460 / 2, 0)))
		{
			ExecuteFile("https://gamecreator.store/register", "", "", 0);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click here to register a new account on the Game Creator Store");
		ImGui::SetCursorPos(vStoreCurPos);
		ImGui::Text("This is the Game Creator Store Downloader. You use this tool to download your");
		ImGui::Text("purchases from the store server to your local GameGuru MAX asset library.");
		ImGui::Text("");
		ImGui::SetWindowFontScale(1.25);
		ImGui::Text("How to use");
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("  - You must have an existing Game Creator Store account");
		ImGui::Text("  - New users can sign up to the store by clicking the Register button");
		ImGui::Text("  - Click the Log in button to sign into your store account");
		ImGui::Text("  - Return to this screen where you will be prompted to download your assets");
		ImGui::Separator();

		// now control the download sequence
		bool bGetNextStep = false;
		ImGui::SetWindowFontScale(1.25);
		if (bDownloadStoreError)
		{
			ImGui::Text("Something Went Wrong!");
		}
		else if (iDownloadStoreProgress == 1)
		{
			ImGui::Text("Getting Login Information");
			if (strlen(cDownloadStoreLoginUrl) > 0 && strlen(cDownloadStoreSessionToken) > 0) 
			{
				bGetNextStep = true;
				bLoginButtonClicked = false;
				iCheckForLoginCount = 0;
			}
		}
		else if (iDownloadStoreProgress == 2 && !bLoginButtonClicked)
		{
			ImGui::Text("You need to Login");
			if (strlen(cDownloadStoreUserId) > 0 && strlen(cDownloadStoreUserHash) > 0)
				bGetNextStep = true;
		}
		else if (iDownloadStoreProgress == 2 && bLoginButtonClicked)
		{
			ImGui::Text("Waiting For Login (%ld)", iCheckForLoginCount);
			if (strlen(cDownloadStoreUserId) > 0 && strlen(cDownloadStoreUserHash) > 0)
				bGetNextStep = true;
		}
		else if (iDownloadStoreProgress == 3)
		{
			ImGui::Text("Downloading List...");
			if (strlen(cDownloadStoreUserId) > 0 && strlen(cDownloadStoreUserHash) > 0)
				bGetNextStep = true;
		}
		else if (iDownloadStoreProgress == 4)
		{
			ImGui::Text("Loading Download List!");
			if (FileExist(pDownloadFile) == 1)
			{
				bGetNextStep = true;
			}
			else 
			{
				strcpy(cDownloadStoreError, "Error: Cant locate download list.");
				bDownloadStoreError = true;
			}
		}
		else if (iDownloadStoreProgress == 5)
		{
			if (pDownloadStoreData) 
			{
				delete[] pDownloadStoreData;
				pDownloadStoreData = NULL;
				dwDownloadStoreDataSize = 0;
			}

			if (pDownloadStoreChecksumFile) 
			{
				delete[] pDownloadStoreChecksumFile;
				pDownloadStoreChecksumFile = NULL;
			}

			ImGui::Text("Parsing Download List!");
			HANDLE hFile = GG_CreateFile("downloads\\storedownload.lst", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD readen;
				DWORD dwDataSize = GetFileSize(hFile, 0);
				pDownloadStoreData = new char[dwDataSize + 20];
				dwDownloadStoreDataSize = dwDataSize;
				if (pDownloadStoreData)
				{
					ReadFile(hFile, pDownloadStoreData, dwDataSize, &readen, FALSE);
					pDownloadStoreData[dwDataSize] = 0;
					pDownloadStoreData[dwDataSize + 1] = 0;
					CloseHandle(hFile);
					bGetNextStep = true;

					//Also get checksum file.
					//We have 2 locations to download to and have checksums for.
					//PE: Use same checksum file so it dont matter if you download local or in doc folder.
					cStr checksumfile = "downloads\\storechecksum.lst";
					hFile = GG_CreateFile(checksumfile.Get(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile != INVALID_HANDLE_VALUE)
					{
						dwDataSize = GetFileSize(hFile, 0);
						pDownloadStoreChecksumFile = new char[dwDataSize + 20];
						if (pDownloadStoreChecksumFile)
						{
							ReadFile(hFile, pDownloadStoreChecksumFile, dwDataSize, &readen, FALSE);
							pDownloadStoreChecksumFile[dwDataSize] = 0;
							pDownloadStoreChecksumFile[dwDataSize + 1] = 0;
							CloseHandle(hFile);
						}
					}
				}
				else 
				{
					strcpy(cDownloadStoreError, "Error: Cant allocate memory for download list.");
					bDownloadStoreError = true;
				}
			}
		}
		else if (iDownloadStoreProgress == 6)
		{
			ImGui::Text("Parsing Download List!");

			files_updated = 0;
			total_files = 0;
			real_total_files = 0;
			real_files_updated = 0;

			download_list.clear();
			char pSearchForToken[256];
			char *progress = pDownloadStoreData;

			while (progress = strstr(progress, "\"id\":"))
			{
				StoreItems * si = new StoreItems;
				int id = 0;

				int iValidEntries = 0;
				char * cEndString = strstr(progress,"\"filesize\":");
				if (cEndString)
				{
					strcpy(pSearchForToken, "\"id\"\0");
					if (progress) 
					{
						progress += strlen(pSearchForToken) + 1;
						char* pFindEnd = strstr(progress, ",\"\0");
						if (!pFindEnd)
							pFindEnd = strstr(progress, "\"\0");
						if (pFindEnd) 
						{
							char cTmp = progress[pFindEnd - progress];
							progress[pFindEnd - progress] = 0;
							if (strlen(progress) < 256) 
							{
								si->id = atoi(progress);
								iValidEntries++;
							}
							progress[pFindEnd - progress] = cTmp;
							progress = pFindEnd + 1;
						}
					}

					strcpy(pSearchForToken, "\"name\"\0");
					progress = strstr(progress, pSearchForToken);
					if (progress) 
					{
						progress += strlen(pSearchForToken) + 2;
						char* pFindEnd = strstr(progress, "\"\0");
						if (pFindEnd) 
						{
							char cTmp = progress[pFindEnd - progress];
							progress[pFindEnd - progress] = 0;
							if (strlen(progress) < 256) 
							{
								si->name = progress;
								iValidEntries++;
							}
							progress[pFindEnd - progress] = cTmp;
							progress = pFindEnd + 1;
						}
					}

					strcpy(pSearchForToken, "\"url\"\0");
					progress = strstr(progress, pSearchForToken);
					if (progress) 
					{
						progress += strlen(pSearchForToken) + 2;
						char* pFindEnd = strstr(progress, "\"\0");
						if (pFindEnd) 
						{
							char cTmp = progress[pFindEnd - progress];
							progress[pFindEnd - progress] = 0;
							if (strlen(progress) < 2048)//256) newer URLs are OOOOGE!
							{
								si->url = progress;
								iValidEntries++;
							}
							progress[pFindEnd - progress] = cTmp;
							progress = pFindEnd + 1;
						}
					}

					strcpy(pSearchForToken, "\"files\"\0");
					progress = strstr(progress, pSearchForToken);
					if (progress) 
					{
						progress += strlen(pSearchForToken) + 2;
						char* pFindEnd = strstr(progress, "}]\0");
						if (pFindEnd) 
						{
							char cTmp = progress[pFindEnd - progress];
							progress[pFindEnd - progress] = 0;
							if (strlen(progress) < dwDownloadStoreDataSize)//8192) some items can have a HUGE number of files!!
							{
								si->data = progress;
								iValidEntries++;
							}
							progress[pFindEnd - progress] = cTmp;
							progress = pFindEnd + 1;
						}
					}

					strcpy(pSearchForToken, "\"checksum\"\0");
					progress = strstr(progress, pSearchForToken);
					if (progress) 
					{
						progress += strlen(pSearchForToken) + 2;
						char* pFindEnd = strstr(progress, "\"\0");
						if (pFindEnd) 
						{
							char cTmp = progress[pFindEnd - progress];
							progress[pFindEnd - progress] = 0;
							if (strlen(progress) < 256) 
							{
								si->checksum = progress;
								iValidEntries++;
							}
							progress[pFindEnd - progress] = cTmp;
							progress = pFindEnd + 1;
						}
					}

					if (iValidEntries == 5) 
					{
						download_list.insert(std::make_pair(total_files++, si));

						//Files in pack.
						int iRealFiles = 0;
						char *pDate = si->data.Get();
						while (pDate = strstr(pDate, "\"zip\":"))
						{
							pDate += 6;
							iRealFiles++;
						}
						real_total_files += iRealFiles;

						//Check if we need to update this file.
						if (pDownloadStoreChecksumFile)
						{
							if (!strstr(pDownloadStoreChecksumFile, si->checksum.Get())) 
							{
								files_updated++;
								real_files_updated += iRealFiles;
							}
						}
						else 
						{
							files_updated++;
							real_files_updated += iRealFiles;
						}

					}
					else 
					{
						delete si;
					}
				}
				else 
				{
					progress++;
				}
			}
			files_downloaded = 0;
			real_files_downloaded = 0;
			bGetNextStep = true;
		}
		else if (iDownloadStoreProgress == 7)
		{
			if (download_list.size() > 0)
			{
				ImGui::Text("Logged in - You have %ld purchased item (MAX compatible)", download_list.size());// real_total_files);
			}
			else 
			{
				strcpy(cDownloadStoreError, "Info: Could not find any MAX compatible purchases to download.");
				bDownloadStoreError = true;
			}
		}
		else if (iDownloadStoreProgress == 8)
		{
			ImGui::Text("Downloading...");
		}
		else if (iDownloadStoreProgress == 9)
		{
			ImGui::Text("Download done.");
		}
		ImGui::SetWindowFontScale(1.0);

		// progress bar and further status text
		if (iDownloadStoreProgress == 2)
		{
			float down_gadget_size = ImGui::GetFontSize()*20.0;
			float w = ImGui::GetWindowContentRegionWidth();
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (down_gadget_size*0.5), 0.0f));
			if (ImGui::StyleButton("Log in the Game Creator Store", ImVec2(down_gadget_size, 0)))
			{
				bLoginButtonClicked = true;
				iCheckForLoginCount = 0;
				ExecuteFile(cDownloadStoreLoginUrl, "", "", 0);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to create a log in session with the store");
		}
		else if (iDownloadStoreProgress > 2 && iDownloadStoreProgress <= 6)
		{
			float fdone = 0.0f +((iDownloadStoreProgress-1)*0.5f);
			if (fdone > 1.0f) fdone = 1.0f;
			ImGui::ProgressBar(fdone, ImVec2(ImGui::GetContentRegionAvail().x - 10, 26), "");
			ImGui::Text("");
		}
		else if (iDownloadStoreProgress == 7 && download_list.size() > 0)
		{
			float donefiles;
			if (real_files_downloaded <= 0)
				donefiles = 0.0f;
			else
				donefiles = (float)real_files_downloaded / real_files_updated;
			char tmp[32];
			sprintf(tmp, "Purchases (%ld/%ld) - Files (%ld/%ld)", files_downloaded, download_list.size(), real_files_downloaded, real_files_updated);
			ImGui::ProgressBar(donefiles, ImVec2(ImGui::GetContentRegionAvail().x - 10, 26), tmp);

			float down_gadget_size = ImGui::GetFontSize()*10.0;
			float w = ImGui::GetWindowContentRegionWidth();
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (down_gadget_size*0.5), 0.0f));
			cStr cLabel = "Update ";
			cLabel += cStr((int)download_list.size());//cStr(real_files_updated);
			cLabel += " Purchases";
			if (files_downloaded > 0) cLabel = "Continue Download";
			if (ImGui::StyleButton(cLabel.Get(), ImVec2(down_gadget_size, 0)))
			{
				//Start download , one in each sync.
				bPrintFirstEntry = true;
				bGetNextStep = true;
			}
			if (files_downloaded==0 && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to download your latest purchases");
			if (files_updated != total_files && files_downloaded == 0)
			{
				float down_gadget_size = ImGui::GetFontSize()*10.0;
				float w = ImGui::GetWindowContentRegionWidth();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (down_gadget_size*0.5), 0.0f));
				if (ImGui::StyleButton("Redownload Everything", ImVec2(down_gadget_size, 0)))
				{
					int iAction = askBoxCancel("This will delete the status of all downloaded files and you must download everything again, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
					if (iAction == 1)
					{
						char checksumfile[MAX_PATH];
						strcpy(checksumfile, "downloads\\storechecksum.lst");
						GG_GetRealPath(&checksumfile[0], 1);

						if (FileExist(checksumfile) == 1)
							DeleteAFile(checksumfile);

						iDownloadStoreProgress = 5; //Read checksum files again.
					}
				}
				if (files_downloaded == 0 && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to download ALL your previous purchases");
			}
		}
		else if (iDownloadStoreProgress == 8 && download_list.size() > 0)
		{
			// same code as above
			float donefiles;
			if (real_files_downloaded <= 0)
				donefiles = 0.0f;
			else
				donefiles = (float)real_files_downloaded / real_files_updated;
			char tmp[32];
			sprintf(tmp, "Purchases (%ld/%ld) - Files (%ld/%ld)", files_downloaded, download_list.size(), real_files_downloaded, real_files_updated);
			ImGui::ProgressBar(donefiles, ImVec2(ImGui::GetContentRegionAvail().x - 10, 26), tmp);

			ImGui::Text("");
			bool bCancelKeyPressed = false;
			int iRealFilesUpdated = 0;
			if (files_downloaded < files_updated)
			{
				int getting_file = -1;
				for (int i = 0; i < total_files; i++)
				{
					if (pDownloadStoreChecksumFile)
					{
						if (!strstr(pDownloadStoreChecksumFile, download_list[i]->checksum.Get()))
						{
							getting_file++;
						}
					}
					else
					{
						getting_file++;
					}
					if (getting_file >= files_downloaded)
					{
						getting_file = i;
						break;
					}
				}	
				if (getting_file >= 0) 
				{
					ImGui::TextCenter("Downloading: %s - %ld", download_list[getting_file]->name.Get(), download_list[getting_file]->id);

					if (!bPrintFirstEntry)
					{
						char pZipFile[256];
						sprintf(pZipFile, "downloads\\storeitem.zip");

						if (FileExist(pZipFile) == 1)
							DeleteAFile(pZipFile);

						memset(pDataReturned, 0, sizeof(pDataReturned));
						dwDataReturnedSize = 0;
						char cUrl[1024];
						std::string url = download_list[getting_file]->url.Get();
						replaceAll(url, "\\/", "/");
						replaceAll(url, "https://gcs-product-media.fra1.digitaloceanspaces.com", "");
						replaceAll(url, "http://gcs-product-media.fra1.digitaloceanspaces.com", "");
						strcpy(cUrl, url.c_str());
						LPSTR pPassInURL = cUrl;
						// pPassInURL Example = "/data-files/ggmax/36084_2021-09-18_14-15_894962614.zip?X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=J7LHIX3DWJWZH2HDPTED%2F20220525%2Ffra1%2Fs3%2Faws4_request&X-Amz-Date=20220525T094657Z&X-Amz-SignedHeaders=host&X-Amz-Expires=3600&X-Amz-Signature=751d98de71f9936efa74b38f0ab236bd9d374a58fbff51af02e5d3c30e1c226f";
						UINT iError = StoreOpenURLForDataOrFile("gcs-product-media.fra1.digitaloceanspaces.com", pDataReturned, &dwDataReturnedSize, "", "GET", pPassInURL, pZipFile);
						if (iError > 0)
						{
							//error
							if (iError == 123456) 
							{
								bCancelKeyPressed = true;
								iDownloadStoreProgress = 7;
							}
							else
							{
								bDownloadStoreError = true;
							}
						}
						else
						{
							bool bFilesUpdated = false;
							char cZipSource[256];
							char cZipDestination[256];
							int iValidEntries = 0;
							char *pDate = download_list[getting_file]->data.Get();

							std::string data = pDate;
							replaceAll(data, "\\/", "/");

							char *pStart = (char *)data.c_str();
							char pSearchForToken[256];
							while (pStart = strstr(pStart, "\"zip\":"))
							{
								iRealFilesUpdated++;
								strcpy(pSearchForToken, "\"zip\":");
								if (pStart) 
								{
									pStart += strlen(pSearchForToken) + 1;
									char* pFindEnd = strstr(pStart, "\"\0");
									if (pFindEnd) 
									{
										char cTmp = pStart[pFindEnd - pStart];
										pStart[pFindEnd - pStart] = 0;
										if (strlen(pStart) < 256) 
										{
											strcpy(cZipSource, pStart);
											iValidEntries++;
										}
										pStart[pFindEnd - pStart] = cTmp;
										pStart = pFindEnd + 1;
									}
									strcpy(pSearchForToken, "\"extract\":");
									pStart = strstr(pStart, pSearchForToken);
									if (pStart) 
									{
										pStart += strlen(pSearchForToken) + 1;
										char* pFindEnd = strstr(pStart, "\"\0");
										if (pFindEnd) 
										{
											char cTmp = pStart[pFindEnd - pStart];
											pStart[pFindEnd - pStart] = 0;
											if (strlen(pStart) < 256) 
											{
												strcpy(cZipDestination, pStart);
												iValidEntries++;
											}
											pStart[pFindEnd - pStart] = cTmp;
											pStart = pFindEnd + 1;
										}
									}
								}
								if (iValidEntries == 2)
								{
									//Process file.
									OpenFileBlockNoPw(pZipFile, 1, "");
									PerformCheckListForFileBlockData(1);
									for (t.i = 1; t.i <= ChecklistQuantity(); t.i++)
									{
										char * pZipName = ChecklistString(t.i);
										if (pZipName && stricmp(pZipName, cZipSource) == 0)
										{
											//Found entry , extract it.
											char FullPath[256];
											strcpy(FullPath, "downloads\\extract\\");
											GG_GetRealPath(&FullPath[0], 1);

											std::string FullFilePath;
											FullFilePath = FullPath;
											FullFilePath.append(pZipName);
											replaceAll(FullFilePath, "/", "\\");

											if (FileExist((char *)FullFilePath.c_str()) == 1)
												DeleteAFile((char *)FullFilePath.c_str());

											ExtractFileFromBlock(1, pZipName, FullPath);

											char cDestinationFullPath[256];
											strcpy(cDestinationFullPath, StoreWriteFolder);
											strcat(cDestinationFullPath, cZipDestination);
											std::string stmp = cDestinationFullPath;
											replaceAll(stmp, "/", "\\");
											strcpy(cDestinationFullPath, stmp.c_str());
											int GG_CreatePath(const char *path);
											GG_CreatePath(cDestinationFullPath);

											if (FileExist((char *)cDestinationFullPath) == 1)
												DeleteAFile((char *)cDestinationFullPath);

											CopyAFile((char *)FullFilePath.c_str(), cDestinationFullPath);

											if (FileExist((char *)cDestinationFullPath) == 1)
											{
												//Success , add checksum to our update file.
												bFilesUpdated = true;
											}
											//Remove tmp unzip file.
											if (FileExist((char *)FullFilePath.c_str()) == 1)
												DeleteAFile((char *)FullFilePath.c_str());
										}
									}
									CloseFileBlock(1);
									iValidEntries = 0;
								}
							}

							if (bFilesUpdated)
							{
								FILE *inputf;
								cStr checksumfile = "downloads\\storechecksum.lst";
								inputf = GG_fopen(checksumfile.Get(), "a+");
								fprintf(inputf, "%s,", download_list[getting_file]->checksum.Get());
								fclose(inputf);
							}
						}
					}
				}
			}
			else 
			{
				//Download done.
				bGetNextStep = true;
			}
			if (!bPrintFirstEntry && !bCancelKeyPressed) 
			{
				files_downloaded++;
				real_files_downloaded+=iRealFilesUpdated;
			}

			ImGui::Text("");
			float down_gadget_size = ImGui::GetFontSize()*14.0;
			float w = ImGui::GetWindowContentRegionWidth();
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (down_gadget_size*0.5), 0.0f));
			if (ImGui::StyleButton("Cancel Download (Hold ESC)", ImVec2(down_gadget_size, 0)))
			{
				iDownloadStoreProgress = 7;
			}
			if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
			{
				iDownloadStoreProgress = 7;
			}		
			if (bPrintFirstEntry)
			{
				bPrintFirstEntry = false;
			}
		}
		else if (iDownloadStoreProgress == 9 )
		{
			//All done only close buttom left.
		}

		if (bGetNextStep) 
		{
			bDownloadStoreError = false;
			iDownloadStoreProgress++;
		}

		//Debug info , show all informations.
		if (iDownloadStoreProgress > 0 && bDownloadStoreError)
		{
			ImGui::Text("");
			if (bDownloadStoreError)
			{
				ImGui::TextWrapped(cDownloadStoreError);
			}
		}

		// allow user to select WHERE these files will go
		ImGui::Separator();
		static bool first_time = true;
		static bool bWriteFolderPossible = false;
		if (first_time)
		{
			//Check if we have write acces to .exe folder.
			void FileRedirectSetup();
			FileRedirectSetup();
			extern char szRootDir[MAX_PATH];
			extern char szWriteDir[MAX_PATH];
			strcpy(StoreWriteFolder, szRootDir);
			strcat(StoreWriteFolder, "Files\\test.tmp");
			strcpy(StoreDocWriteFolder, szWriteDir);
			strcat(StoreDocWriteFolder, "Files\\");
			strcpy(StoreAppWriteFolder, szRootDir);
			strcat(StoreAppWriteFolder, "Files\\");
			FILE* testFile = fopen(StoreWriteFolder, "w");
			if (testFile)
			{
				fprintf(testFile, "test");
				fclose(testFile);
			}
			if (FileExist(StoreWriteFolder) == 1)
			{
				DeleteAFile(StoreWriteFolder);
				bWriteFolderPossible = true;
			}
			strcpy(StoreWriteFolder, StoreDocWriteFolder);
			iDownloadLocation = 1;
			first_time = false;
		}
		if (iDownloadStoreProgress >= 2 && iDownloadStoreProgress <= 7 && files_downloaded == 0)
		{
			ImGui::SetWindowFontScale(1.25);
			ImGui::Text("Download Folder (Advanced):");
			ImGui::SetWindowFontScale(1.0);
			if (bWriteFolderPossible)
			{
				if (ImGui::RadioButton(StoreAppWriteFolder, &iDownloadLocation, 0))
				{
					strcpy(StoreWriteFolder, StoreAppWriteFolder);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to change where your downloads will be stored");
				if (ImGui::RadioButton(StoreDocWriteFolder, &iDownloadLocation, 1))
				{
					strcpy(StoreWriteFolder, StoreDocWriteFolder);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to change where your downloads will be stored");			
			}
			else
			{
				ImGui::RadioButton(StoreDocWriteFolder, &iDownloadLocation, 0);
				strcpy(StoreWriteFolder, StoreDocWriteFolder);
				iDownloadLocation = 0;
			}
		}
		ImGui::SetWindowFontScale(1.0);

		ImGui::Separator();
		ImGui::Text("");
		float down_gadget_size = ImGui::GetFontSize()*10.0;
		float w = ImGui::GetWindowContentRegionWidth();
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (down_gadget_size*0.5), 0.0f));
		if (ImGui::StyleButton("Close", ImVec2(down_gadget_size, 0)))
		{
			if (pDownloadStoreData) 
			{
				delete[] pDownloadStoreData;
				pDownloadStoreData = NULL;
				dwDownloadStoreDataSize = 0;
			}
			if (pDownloadStoreChecksumFile) 
			{
				delete[] pDownloadStoreChecksumFile;
				pDownloadStoreChecksumFile = NULL;
			}
			if (total_files > 0) 
			{
				for (int i = 0; i < total_files; i++)
				{
					if (download_list[i])
						delete(download_list[i]);
				}
			}
			download_list.clear();
			bDownloadStore_Window = false;
			extern int g_iRefreshLibraryFolders;
			g_iRefreshLibraryFolders = 2;
		}
		ImGui::Indent(-10);
		ImGui::End();
	}
}

UINT StoreOpenURLForDataOrFile(LPSTR pServer, LPSTR pDataReturned, DWORD* pReturnDataSize, LPSTR pPostData, LPSTR pVerb, LPSTR urlWhere, LPSTR pLocalFileForImageOrNews)
{
	// default is main store 
	bool bMainStore = false;
	if (pServer == NULL)
	{
		bMainStore = true;
		pServer = "gamecreator.store";
	}

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
		HINTERNET m_hInetConnect = InternetConnectA(m_hInet, pServer, wHTTPType, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (m_hInetConnect == NULL)
		{
			iError = GetLastError();
		}
		else
		{
			int m_iTimeout = 5000;
			InternetSetOption(m_hInetConnect, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&m_iTimeout, sizeof(m_iTimeout));
			unsigned long flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
				INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE |
				INTERNET_FLAG_DONT_CACHE |
				INTERNET_FLAG_KEEP_CONNECTION |
				INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_AUTO_REDIRECT;

			HINTERNET hHttpRequest = HttpOpenRequestA(m_hInetConnect, pVerb, urlWhere, "HTTP/1.1", NULL, NULL, flags, 0);
			if (hHttpRequest == NULL)
			{
				iError = GetLastError();
			}
			else
			{
				if (bMainStore == true)
				{
					char lpszHeaders[4096];
					sprintf(lpszHeaders, "Authorization: Bearer %s", SECRET_TOKEN);
					HttpAddRequestHeadersA(hHttpRequest, lpszHeaders, strlen(lpszHeaders), 0);
				}
				int bSendResult = 0;
				FILE* fpLocalFile = NULL;
				if (pLocalFileForImageOrNews == NULL)
				{
					char m_szPostData[1024];
					if (pPostData && strlen(pPostData) > 0)
					{
						strcpy(m_szPostData, pPostData);
						bSendResult = HttpSendRequest(hHttpRequest, NULL, -1, (void*)(m_szPostData), strlen(m_szPostData));
					}
					else
					{
						bSendResult = HttpSendRequest(hHttpRequest, NULL, -1, NULL, NULL);
					}
				}
				else
				{
					//open local file for writing
					bSendResult = HttpSendRequest(hHttpRequest, NULL, -1, NULL, NULL);
					fpLocalFile = GG_fopen(pLocalFileForImageOrNews, "wb+");
				}
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
						if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
						{
							iError = 123456;
							break;
						}
						if (fpLocalFile)
						{
							// write direct to file
							fwrite(pBuffer, 1, written, fpLocalFile);
						}
						else
						{
							if (dwDataLength + written > MAXSTOREDATESIZE) written = MAXSTOREDATESIZE - dwDataLength;
							memcpy(pDataReturned + dwDataLength, pBuffer, written);
							dwDataLength = dwDataLength + written;
							if (dwDataLength >= MAXSTOREDATESIZE) break;
						}
					}
					InternetCloseHandle(hHttpRequest);
				}
				if (fpLocalFile)
				{
					fclose(fpLocalFile);
					fpLocalFile = NULL;
				}
			}
			InternetCloseHandle(m_hInetConnect);
		}
		InternetCloseHandle(m_hInet);
	}
	if (iError > 0 )
	{
		if (iError == 123456)
		{
			strcpy(cDownloadStoreError, "Info: Pressed Cancel (ESC).");
		}
		else
		{
			char *szError = 0;
			if (iError > 12000 && iError < 12174)
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("wininet.dll"), iError, 0, (char*)&szError, 0, 0);
			else
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, iError, 0, (char*)&szError, 0, 0);
			if (szError)
			{
				if (strlen(szError) < 8172)
				{
					strcpy(cDownloadStoreError, "Error: ");
					strcat(cDownloadStoreError, szError);
				}
				else
				{
					sprintf(cDownloadStoreError, "Unknown Error: %d", iError);
				}
				LocalFree(szError);
			}
		}
	}

	//Url decode (not when using files).
	if (dwDataLength > 0 && strlen(pDataReturned) > 0) 
	{
		std::string url_decode = pDataReturned;
		replaceAll(url_decode, "\\/", "/");
		replaceAll(url_decode, "%2F", "/");
		replaceAll(url_decode, "%24" , "$");
		strcpy(pDataReturned, url_decode.c_str());
	}

	// complete
	*pReturnDataSize = dwDataLength;
	return iError;
}
