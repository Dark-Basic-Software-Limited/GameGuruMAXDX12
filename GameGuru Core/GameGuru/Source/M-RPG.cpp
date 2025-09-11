#pragma optimize("", off)

//----------------------------------------------------
//--- GAMEGURU - M-RPG
//----------------------------------------------------

// Includes 
#include "stdafx.h"
#include "gameguru.h"
#include "M-RPG.h"

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

//PE: GameGuru IMGUI.
#ifdef ENABLEIMGUI
#include "..\..\GameGuru\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\..\GameGuru\Imgui\imgui_internal.h"
#include "..\..\GameGuru\Imgui\imgui_impl_win32.h"
#include "..\..\GameGuru\Imgui\imgui_gg_dx11.h"
#endif

// Globals
std::vector<cstr> g_collectionLabels;
std::vector<collectionItemType> g_collectionMasterList;
std::vector<collectionItemType> g_collectionList;
std::vector<cstr> g_collectionQuestLabels;
std::vector<collectionQuestType> g_collectionQuestMasterList;
std::vector<collectionQuestType> g_collectionQuestList;

// Functions
void init_rpg_system(void)
{
	// clear collection list
	g_collectionMasterList.clear();
	g_collectionList.clear();
	g_collectionQuestMasterList.clear();
	g_collectionQuestList.clear();
}

bool load_rpg_system_items(char* name)
{
	// out of the box mandatory item labels
	g_collectionLabels.clear();
	g_collectionLabels.push_back("title");
	g_collectionLabels.push_back("profile");
	g_collectionLabels.push_back("image");
	g_collectionLabels.push_back("description");
	g_collectionLabels.push_back("cost");
	g_collectionLabels.push_back("value");
	g_collectionLabels.push_back("container");
	g_collectionLabels.push_back("ingredients");
	g_collectionLabels.push_back("style");

	// load in collection file (contains all items in all game levels)
	timestampactivity(0, "loading in collection - items.tsv");
	std::vector<cstr> g_localCollectionLabels;
	char collectionfilename[MAX_PATH];
	strcpy(collectionfilename, "projectbank\\");
	strcat(collectionfilename, name);
	strcat(collectionfilename, "\\collection - items.tsv");
	FILE* collectionFile = GG_fopen(collectionfilename, "r");
	if (collectionFile)
	{
		// read all lines in TAB DELIMITED FILE
		bool bPopulateLabels = true;
		while (!feof(collectionFile))
		{
			// read a line
			char theline[MAX_PATH];
			strcpy(theline, "");
			fgets(theline, MAX_PATH - 1, collectionFile);
			if (strlen(theline) > 0 && theline[strlen(theline) - 1] == '\n')
				theline[strlen(theline) - 1] = 0;

			//PE: Empty line \n at the bottom would always add the last item 1 additional time, growing the list.
			timestampactivity(0, theline);
			if (strlen(theline) > 0)
			{
				// determine which list to fill
				collectionItemType item;
				item.iEntityID = 0;
				item.iEntityElementE = 0;
				if (bPopulateLabels == true)
				{
					// first line are all the labels
					g_localCollectionLabels.clear();
				}
				else
				{
					// remaining lines are the collection, prepopulate with correct number of them
					item.collectionFields.clear();
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("default");
					item.collectionFields.push_back("default");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("10");
					item.collectionFields.push_back("5");
					item.collectionFields.push_back("shop");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("none");
					int iLAIndex = item.collectionFields.size();
					while (iLAIndex < g_collectionLabels.size())
					{
						item.collectionFields.push_back("none");
						iLAIndex++;
					}
				}

				// go through tab delimited fields
				int iColumnIndex = 0;
				char pTab[2]; pTab[0] = 9; pTab[1] = 0;
				const char* delimiter = pTab;
				char* token = std::strtok(theline, delimiter);
				while (token)
				{
					if (bPopulateLabels == true)
					{
						// record local order of the labels from the import
						g_localCollectionLabels.push_back(token);

						// add unique ones to end of labels list
						bool bFoundThisOne = false;
						for (int la = 0; la < g_collectionLabels.size(); la++)
						{
							if (stricmp(g_collectionLabels[la].Get(), token) == NULL)
							{
								bFoundThisOne = true;
								break;
							}
						}
						if (bFoundThisOne == false)
						{
							// add to end of main list of labels
							g_collectionLabels.push_back(token);
						}
					}
					else
					{
						// add to correct location in item collection fields (respect main labels list, not local import)
						if (iColumnIndex < g_localCollectionLabels.size())
						{
							LPSTR pLabelAssociated = g_localCollectionLabels[iColumnIndex].Get();
							iColumnIndex++;
							for (int la = 0; la < g_collectionLabels.size(); la++)
							{
								if (stricmp(g_collectionLabels[la].Get(), pLabelAssociated) == NULL)
								{
									// populate correct place in item
									item.collectionFields[la] = token;
									break;
								}
							}
						}
					}
					token = std::strtok(nullptr, delimiter);
				}

				// add populated item to collection list
				if (bPopulateLabels == false && item.collectionFields.size() > 2)
				{
					if (stricmp(item.collectionFields[0].Get(), "title") == NULL && stricmp(item.collectionFields[2].Get(), "image") == NULL)
					{
						// seems we have duplicated the header row, so ignore (title, profile, image, etc)
					}
					else
					{
						// quick sanity check, reset any corrupt entries for image (might be FPE from old tabbed files)
						LPSTR pImageEntry = item.collectionFields[2].Get();
						if (strnicmp(pImageEntry + strlen(pImageEntry) - 4, ".fpe", 4) == NULL)
						{
							// restore to default, thank you!
							item.collectionFields[2] = "default";
						}

						// real entry, add it
						g_collectionMasterList.push_back(item);
					}
				}
				// first line over
				bPopulateLabels = false;
			}
		}
		fclose(collectionFile);
	}
	timestampactivity(0, "loading collection complete");

	// make a copy to regular gaming list
	g_collectionList = g_collectionMasterList;

	// trigger initial filling of global list (better location for this somewhere)
	extern bool g_bRefreshGlobalList;
	g_bRefreshGlobalList = true;

	// success
	return true;
}

bool load_rpg_system_quests(char* name)
{
	// out of the box labels
	g_collectionQuestLabels.clear();
	g_collectionQuestLabels.push_back("title");
	g_collectionQuestLabels.push_back("type");
	g_collectionQuestLabels.push_back("image");
	g_collectionQuestLabels.push_back("desc1");
	g_collectionQuestLabels.push_back("desc2");
	g_collectionQuestLabels.push_back("desc3");
	g_collectionQuestLabels.push_back("object");
	g_collectionQuestLabels.push_back("receiver");
	g_collectionQuestLabels.push_back("level");
	g_collectionQuestLabels.push_back("points");
	g_collectionQuestLabels.push_back("value");
	g_collectionQuestLabels.push_back("status");
	g_collectionQuestLabels.push_back("activate");
	g_collectionQuestLabels.push_back("quantity");
	g_collectionQuestLabels.push_back("endmap");
	std::vector<cstr> g_localCollectionLabels;
	char collectionfilename[MAX_PATH];
	strcpy(collectionfilename, "projectbank\\");
	strcat(collectionfilename, name);
	strcat(collectionfilename, "\\collection - quests.tsv");
	FILE* collectionFile = GG_fopen(collectionfilename, "r");
	if (collectionFile)
	{
		bool bPopulateLabels = true;
		while (!feof(collectionFile))
		{
			// read a line
			char theline[MAX_PATH];
			strcpy(theline, "");
			fgets(theline, MAX_PATH - 1, collectionFile);
			if (strlen(theline) > 0 && theline[strlen(theline) - 1] == '\n')
				theline[strlen(theline) - 1] = 0;

			//PE: Empty line \n at the bottom would always add the last item 1 additional time, growing the list.
			if (strlen(theline) > 0)
			{

				// determine which list to fill
				collectionQuestType item;
				if (bPopulateLabels == true)
				{
					// first line are all the labels
					g_localCollectionLabels.clear();
				}
				else
				{
					// remaining lines are the collection, prepopulate with correct number of them
					item.collectionFields.clear();
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("collect");
					item.collectionFields.push_back("default");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("none");
					item.collectionFields.push_back("1");
					item.collectionFields.push_back("100");
					item.collectionFields.push_back("100");
					item.collectionFields.push_back("inactive");
					item.collectionFields.push_back("none");
					int iLAIndex = item.collectionFields.size();
					while (iLAIndex < g_collectionQuestLabels.size())
					{
						item.collectionFields.push_back("none");
						iLAIndex++;
					}
				}

				// go through tab delimited fields
				int iColumnIndex = 0;
				char pTab[2]; pTab[0] = 9; pTab[1] = 0;
				const char* delimiter = pTab;
				char* token = std::strtok(theline, delimiter);
				while (token)
				{
					if (bPopulateLabels == true)
					{
						// record local order of the labels from the import
						g_localCollectionLabels.push_back(token);

						// add unique ones to end of labels list
						bool bFoundThisOne = false;
						for (int la = 0; la < g_collectionQuestLabels.size(); la++)
						{
							if (stricmp(g_collectionQuestLabels[la].Get(), token) == NULL)
							{
								bFoundThisOne = true;
								break;
							}
						}
						if (bFoundThisOne == false)
						{
							// add to end of main list of labels
							g_collectionQuestLabels.push_back(token);
						}
					}
					else
					{
						// add to correct location in item collection fields (respect main labels list, not local import)
						if (iColumnIndex < g_localCollectionLabels.size())
						{
							LPSTR pLabelAssociated = g_localCollectionLabels[iColumnIndex].Get();
							iColumnIndex++;
							for (int la = 0; la < g_collectionQuestLabels.size(); la++)
							{
								if (stricmp(g_collectionQuestLabels[la].Get(), pLabelAssociated) == NULL)
								{
									item.collectionFields[la] = token;
									break;
								}
							}
						}
					}
					token = std::strtok(nullptr, delimiter);
				}

				// add populated item to collection list
				if (bPopulateLabels == false && item.collectionFields.size() > 2 && iColumnIndex > 7)
				{
					if (stricmp(item.collectionFields[0].Get(), "title") == NULL && stricmp(item.collectionFields[2].Get(), "image") == NULL)
					{
						// seems we have duplicated the header row, so ignore (title, profile, image, etc)
					}
					else
					{
						// real entry, add it
						g_collectionQuestMasterList.push_back(item);
					}
				}

				// first line over
				bPopulateLabels = false;
			}
		}
		fclose(collectionFile);
	}

	// make a copy to regular list
	g_collectionQuestList = g_collectionQuestMasterList;

	// success
	return true;
}

bool load_rpg_system(char* name)
{
	load_rpg_system_items(name);
	load_rpg_system_quests(name);
	return true;
}

bool save_rpg_system_items(char* name, bool bIncludeELEFile)
{
	// nothing to save if no collection to save
	if (g_collectionLabels.size() == 0)
		return true;

	// save master collection in file (contains all items in all game levels)
	timestampactivity(0, "saving collection - items.tsv");
	char collectionfilename[MAX_PATH];
	strcpy(collectionfilename, "projectbank\\");
	strcat(collectionfilename, name);
	strcat(collectionfilename, "\\collection - items.tsv");
	//DeleteFileA(collectionfilename);
	GG_GetRealPath(collectionfilename, 1);
	if (FileExist(collectionfilename) == 1) DeleteFileA(collectionfilename);
	FILE* collectionFile = GG_fopen(collectionfilename, "w");
	if (collectionFile)
	{
		// write all lines in TAB DELIMITED FILE
		char pTab[2]; pTab[0] = 9; pTab[1] = 0;
		char pCR[2]; pCR[0] = 10; pCR[1] = 0;
		char theline[MAX_PATH];

		// first write collection labels
		strcpy(theline, "");
		for (int l = 0; l < g_collectionLabels.size(); l++)
		{
			strcat(theline, g_collectionLabels[l].Get());
			strcat(theline, pTab);
		}
		// Got a crash here when creating a new game project
		int lineLength = strlen(theline);
		if (lineLength > 0)
		{
			theline[strlen(theline) - 1] = 0;
		}
		strcat(theline, pCR);
		fwrite (theline, strlen (theline) * sizeof (char), 1, collectionFile);

		// then for each item a line is created with all attribs - was g_collectionMasterList
		for (int i = 0; i < g_collectionList.size(); i++)
		{
			if (g_collectionList[i].collectionFields.size() > 0)
			{
				strcpy(theline, "");
				for (int l = 0; l < g_collectionList[i].collectionFields.size(); l++)
				{
					LPSTR pStrToAdd = g_collectionList[i].collectionFields[l].Get();
					if (strlen(pStrToAdd) > 0)
						strcat(theline, pStrToAdd);
					else
						strcat(theline, " ");

					strcat(theline, pTab);
				}
				if(strlen(theline) > 0) //PE: Crashed here if strlen==0
					theline[strlen(theline) - 1] = 0;
				strcat(theline, pCR);
				fwrite (theline, strlen (theline) * sizeof (char), 1, collectionFile);
			}
			else
			{
				// why is the an empty entry, ie no collection fields!!
				// this causes a crash with code older than 30/08/2023
				int iWhoIsClearing = g_collectionList[i].collectionFields.size();
			}
		}
		fclose(collectionFile);
		timestampactivity(0, "DONE saving collection - items.tsv");

	}

	// also save an up to date copy of the needed elements
	if (bIncludeELEFile == true)
	{
		timestampactivity(0, "saving collection - items.ele");
		cstr storeoldELEfile = t.elementsfilename_s;
		char collectionELEfilename[MAX_PATH];
		strcpy(collectionELEfilename, "projectbank\\");
		strcat(collectionELEfilename, name);
		strcat(collectionELEfilename, "\\collection - items.ele");
		GG_GetRealPath(collectionELEfilename, 1);
		if (FileExist(collectionELEfilename) == 1) DeleteFileA(collectionELEfilename);
		t.elementsfilename_s = collectionELEfilename;
		int iEntitiesToSaveCount = g_collectionList.size();
		if (iEntitiesToSaveCount > g.entityelementlist) iEntitiesToSaveCount = g.entityelementlist;
		if (iEntitiesToSaveCount > 0)
		{
			entitytype* pStoreEntEle = new entitytype[iEntitiesToSaveCount];
			entitytype* pTempEntEle = new entitytype[iEntitiesToSaveCount];
			for (int storee = 0; storee < iEntitiesToSaveCount; storee++)
			{
				pStoreEntEle[storee] = t.entityelement[1 + storee];
			}
			for (int c = 0; c < iEntitiesToSaveCount; c++)
			{
				int sourcee = g_collectionList[c].iEntityElementE;
				if (sourcee > 0 && sourcee < t.entityelement.size()) // sourcee can have rogue dead entE refs
					pTempEntEle[c] = t.entityelement[sourcee];
				else
					pTempEntEle[c] = t.entityelement[0];
			}
			for (int e = 0; e < iEntitiesToSaveCount; e++)
			{
				t.entityelement[1 + e] = pTempEntEle[e];
			}
			int iStoreEntEleCount = g.entityelementlist;
			g.entityelementlist = iEntitiesToSaveCount;
			bool bForCollectionELE = true;
			entity_saveelementsdata(bForCollectionELE);
			for (int storee = 0; storee < iEntitiesToSaveCount; storee++)
			{
				t.entityelement[1 + storee] = pStoreEntEle[storee];
			}
			g.entityelementlist = iStoreEntEleCount;
			t.elementsfilename_s = storeoldELEfile;
			delete[] pStoreEntEle;
			delete[] pTempEntEle;
		}
		//PE: If g_collectionList == 0 , save map did not work. but was stored in projectbank/name/collection - items.ele
		t.elementsfilename_s = storeoldELEfile;
	}

	// success
	return true;
}

bool save_rpg_system_quests(char* name)
{
	// nothing to save if no collection to save
	if (g_collectionQuestLabels.size() == 0)
		return true;

	timestampactivity(0, "saving collection - quests.tsv");

	// save master collection in file (contains all items in all game levels)
	char collectionfilename[MAX_PATH];
	strcpy(collectionfilename, "projectbank\\");
	strcat(collectionfilename, name);
	strcat(collectionfilename, "\\collection - quests.tsv");
	//DeleteFileA(collectionfilename);
	GG_GetRealPath(collectionfilename, 1);
	if (FileExist(collectionfilename) == 1) DeleteFileA(collectionfilename);
	FILE* collectionFile = GG_fopen(collectionfilename, "w");
	if (collectionFile)
	{
		// write all lines in TAB DELIMITED FILE
		char pTab[2]; pTab[0] = 9; pTab[1] = 0;
		char pCR[2]; pCR[0] = 10; pCR[1] = 0;
		char theline[MAX_PATH];

		// first write collection labels
		strcpy(theline, "");
		for (int l = 0; l < g_collectionQuestLabels.size(); l++)
		{
			strcat(theline, g_collectionQuestLabels[l].Get());
			strcat(theline, pTab);
		}
		
		//PE: Possible crash here.
		if (strlen(theline) > 0)
			theline[strlen(theline) - 1] = 0;

		strcat(theline, pCR);
		fwrite (theline, strlen (theline) * sizeof (char), 1, collectionFile);

		// then for each item a line is created with all attribs - was g_collectionMasterList
		for (int i = 0; i < g_collectionQuestList.size(); i++)
		{
			strcpy(theline, "");
			for (int l = 0; l < g_collectionQuestList[i].collectionFields.size(); l++)
			{
				LPSTR pStrToAdd = g_collectionQuestList[i].collectionFields[l].Get();
				if(strlen(pStrToAdd)>0)
					strcat(theline, pStrToAdd);
				else
					strcat(theline, " ");
				strcat(theline, pTab);
			}
			//PE: Possible crash here.
			if(strlen(theline) > 0)
				theline[strlen(theline) - 1] = 0;
			strcat(theline, pCR);
			fwrite (theline, strlen (theline) * sizeof (char), 1, collectionFile);
		}
		fclose(collectionFile);
		timestampactivity(0, "DONE saving collection - quests.tsv");

	}

	// success
	return true;
}

bool save_rpg_system(char* name, bool bIncludeELEFile)
{
	if (save_rpg_system_items(name, bIncludeELEFile) == true)
	{
		save_rpg_system_quests(name);
		return true;
	}
	else
		return false;
}

cstr get_rpg_imagefinalfile(cstr entityfile)
{
	extern cstr BackBufferCacheName;
	bool CreateBackBufferCacheName(char* file, int width, int height);
	CreateBackBufferCacheName(entityfile.Get(), 512, 288);
	LPSTR pAbsPathToFile = BackBufferCacheName.Get();
	cstr pRootDir = g.fpscrootdir_s + "\\Files\\";
	char pIconFile[MAX_PATH];
	strcpy(pIconFile, entityfile.Get());
	cstr pFinalImgFile;
	if (FileExist(pIconFile) == 1)
	{
		// found right away
		pFinalImgFile = pIconFile;
	}
	else
	{
		// find it elsewhere
		strcpy(pIconFile, pAbsPathToFile + strlen(pRootDir.Get()));
		pIconFile[strlen(pIconFile) - 4] = 0;
		strcat(pIconFile, ".png");
		pFinalImgFile = pIconFile;
		if (FileExist(pFinalImgFile.Get()) == 0)
		{
			// fall back
			pFinalImgFile = "imagebank\\HUD Library\\MAX\\object.png";
		}
	}
	return pFinalImgFile;
}

bool fill_rpg_item_defaults_passedin(collectionItemType* pItem, int entid, int e, LPSTR pPassedInTitle, LPSTR pPassedInImageFile)
{
	// only some entities can make an item
	int iAddThisItem = 0;
	if (entid > 0 && e > 0)
	{
		if (t.entityelement[e].eleprof.iscollectable != 0) iAddThisItem = 2;
		if (t.entityelement[e].eleprof.isProjectGlobal != 0) iAddThisItem = 2;
		if (t.entityprofile[entid].isweapon > 0) iAddThisItem = 1;
		if (t.entityprofile[entid].hasweapon > 0) iAddThisItem = 4;
	}
	else
	{
		if (pPassedInTitle && pPassedInImageFile) iAddThisItem = 3;
		if (e > 0)
		{
			if (t.entityelement[e].eleprof.hasweapon_s.Len() > 0) iAddThisItem = 5;
		}
	}
	if (iAddThisItem > 0)
	{
		pItem->iEntityID = entid;
		pItem->iEntityElementE = e;
		if (iAddThisItem == 5) pItem->iEntityElementE = 0;
		pItem->collectionFields.clear();
		for (int l = 0; l < g_collectionLabels.size(); l++)
		{
			int iKnownLabel = -1;
			LPSTR pLabel = g_collectionLabels[l].Get();
			if (stricmp(pLabel, "title") == NULL) iKnownLabel = 0;
			if (stricmp(pLabel, "profile") == NULL) iKnownLabel = 1;
			if (stricmp(pLabel, "image") == NULL) iKnownLabel = 2;
			if (stricmp(pLabel, "description") == NULL) iKnownLabel = 3;
			if (stricmp(pLabel, "cost") == NULL) iKnownLabel = 4;
			if (stricmp(pLabel, "value") == NULL) iKnownLabel = 5;
			if (stricmp(pLabel, "container") == NULL) iKnownLabel = 6;
			if (stricmp(pLabel, "ingredients") == NULL) iKnownLabel = 7;
			if (stricmp(pLabel, "style") == NULL) iKnownLabel = 8;
			bool bUseNoneValue = false;
			if (iAddThisItem == 3)
			{
				if (iKnownLabel == 0 || iKnownLabel == 2 || iKnownLabel == 3)
				{
					// we can fill these with passed in values
				}
				else
				{
					// we have no values, use 'None'
					bUseNoneValue = true;
				}
			}
			if (iKnownLabel >= 0 && bUseNoneValue == false)
			{
				if (iKnownLabel == 0)
				{
					LPSTR pTitle = "";
					if (iAddThisItem == 1) pTitle = t.entityelement[e].eleprof.name_s.Get();
					if (iAddThisItem == 2) pTitle = t.entityelement[e].eleprof.name_s.Get();
					if (iAddThisItem == 3) pTitle = pPassedInTitle;
					if (iAddThisItem == 4) pTitle = t.entityprofile[entid].hasweapon_s.Get();
					if (iAddThisItem == 5) pTitle = t.entityelement[e].eleprof.hasweapon_s.Get();
					pItem->collectionFields.push_back(pTitle);
				}
				if (iKnownLabel == 1)
				{
					if (iAddThisItem == 4 || iAddThisItem == 5)
						pItem->collectionFields.push_back("None");
					else
						pItem->collectionFields.push_back(t.entitybank_s[entid].Get());
				}
				if (iKnownLabel == 2)
				{
					cstr pFinalImgFile = "";
					cstr localiconfile = "";
					if (iAddThisItem == 1) localiconfile = cstr("gamecore\\guns\\") + t.entityprofile[entid].isweapon_s + cstr("\\item.png");
					if (iAddThisItem == 2) localiconfile = t.entityprofile[entid].collectable.image.Get();
					if (iAddThisItem == 3) localiconfile = pPassedInImageFile;
					if (iAddThisItem == 4) localiconfile = cstr("gamecore\\guns\\") + t.entityprofile[entid].hasweapon_s + cstr("\\item.png");
					if (iAddThisItem == 5) localiconfile = cstr("gamecore\\guns\\") + t.entityelement[e].eleprof.hasweapon_s + cstr("\\item.png");
					if (FileExist(localiconfile.Get()) == 1)
					{
						// use locally specified icon
						pFinalImgFile = localiconfile;
					}
					else
					{
						// use default out of the box icon
						cstr entityfile = t.entitybank_s[entid];
						pFinalImgFile = get_rpg_imagefinalfile(entityfile);
					}
					pItem->collectionFields.push_back(pFinalImgFile);
				}
				if (iKnownLabel == 3)
				{
					LPSTR pDesc = "";
					if (iAddThisItem == 1) pDesc = t.entityprofile[entid].collectable.description.Get();
					if (iAddThisItem == 2) pDesc = t.entityprofile[entid].collectable.description.Get();
					if (iAddThisItem == 3) pDesc = pPassedInTitle;
					if (iAddThisItem == 4) pDesc = "";
					if (iAddThisItem == 5) pDesc = "";
					pItem->collectionFields.push_back(pDesc);
				}
				if (entid > 0)
				{
					if (iKnownLabel == 4) pItem->collectionFields.push_back(t.entityprofile[entid].collectable.cost);
					if (iKnownLabel == 5) pItem->collectionFields.push_back(t.entityprofile[entid].collectable.value);
					if (iKnownLabel == 6) pItem->collectionFields.push_back(t.entityprofile[entid].collectable.container.Get());
					if (iKnownLabel == 7) pItem->collectionFields.push_back(t.entityprofile[entid].collectable.ingredients.Get());
				}
				else
				{
					if (iKnownLabel == 4) pItem->collectionFields.push_back("10");
					if (iKnownLabel == 5) pItem->collectionFields.push_back("5");
					if (iKnownLabel == 6) pItem->collectionFields.push_back("none");
					if (iKnownLabel == 7) pItem->collectionFields.push_back("none");
				}
				if (iKnownLabel == 8)
				{
					if (iAddThisItem == 1 || iAddThisItem == 4 || iAddThisItem == 5)
					{
						char pWeaponStyle[MAX_PATH];
						if (iAddThisItem == 1) sprintf(pWeaponStyle, "weapon=%s", t.entityprofile[entid].isweapon_s.Get());
						if (iAddThisItem == 4) sprintf(pWeaponStyle, "weapon=%s", t.entityprofile[entid].hasweapon_s.Get());
						if (iAddThisItem == 5) sprintf(pWeaponStyle, "weapon=%s", t.entityelement[e].eleprof.hasweapon_s.Get());
						pItem->collectionFields.push_back(pWeaponStyle);
					}
					else
					{
						pItem->collectionFields.push_back(t.entityprofile[entid].collectable.style.Get());
					}
				}
			}
			else
			{
				// empty field
				pItem->collectionFields.push_back("none");
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool fill_rpg_item_defaults(collectionItemType* pItem, int entid, int e)
{
	return fill_rpg_item_defaults_passedin(pItem, entid, e, NULL, NULL);
}

bool fill_rpg_quest_defaults(collectionQuestType* pItem, char* pName)
{
	pItem->collectionFields.clear();
	for (int l = 0; l < g_collectionQuestLabels.size(); l++)
	{
		int iKnownLabel = -1;
		LPSTR pLabel = g_collectionQuestLabels[l].Get();
		if (stricmp(pLabel, "title") == NULL) iKnownLabel = 0;
		if (stricmp(pLabel, "type") == NULL) iKnownLabel = 51;
		if (stricmp(pLabel, "image") == NULL) iKnownLabel = 2;
		if (stricmp(pLabel, "desc1") == NULL) iKnownLabel = 52;
		if (stricmp(pLabel, "desc2") == NULL) iKnownLabel = 53;
		if (stricmp(pLabel, "desc3") == NULL) iKnownLabel = 54;
		if (stricmp(pLabel, "object") == NULL) iKnownLabel = 55;
		if (stricmp(pLabel, "receiver") == NULL) iKnownLabel = 56;
		if (stricmp(pLabel, "level") == NULL) iKnownLabel = 57;
		if (stricmp(pLabel, "points") == NULL) iKnownLabel = 58;
		if (stricmp(pLabel, "value") == NULL) iKnownLabel = 59;
		if (stricmp(pLabel, "status") == NULL) iKnownLabel = 60;
		if (stricmp(pLabel, "activate") == NULL) iKnownLabel = 61;
		if (stricmp(pLabel, "quantity") == NULL) iKnownLabel = 62;
		if (stricmp(pLabel, "endmap") == NULL) iKnownLabel = 63;
		
		if (iKnownLabel >= 0)
		{
			if (iKnownLabel == 0)
			{
				LPSTR pTitle = pName;
				pItem->collectionFields.push_back(pTitle);
			}
			if (iKnownLabel == 51)
			{
				pItem->collectionFields.push_back("collect");
			}
			if (iKnownLabel == 2)
			{
				cstr pFinalImgFile = get_rpg_imagefinalfile("imagebank\\HUD Library\\RPG\\quest_scroll.png");
				pItem->collectionFields.push_back(pFinalImgFile);
			}
			if (iKnownLabel == 52) pItem->collectionFields.push_back("none");
			if (iKnownLabel == 53) pItem->collectionFields.push_back("none");
			if (iKnownLabel == 54) pItem->collectionFields.push_back("none");
			if (iKnownLabel == 55) pItem->collectionFields.push_back("none");
			if (iKnownLabel == 56) pItem->collectionFields.push_back("none");
			if (iKnownLabel == 57) pItem->collectionFields.push_back("1");
			if (iKnownLabel == 58) pItem->collectionFields.push_back("100");
			if (iKnownLabel == 59) pItem->collectionFields.push_back("100");
			if (iKnownLabel == 60) pItem->collectionFields.push_back("inactive");
			if (iKnownLabel == 61) pItem->collectionFields.push_back("none");
			if (iKnownLabel == 62) pItem->collectionFields.push_back("1");
			if (iKnownLabel == 63) pItem->collectionFields.push_back("none");
		}
		else
		{
			// empty field
			pItem->collectionFields.push_back("none");
		}
	}
	return true;
}

void refresh_rpg_parents_of_items(void)
{
	for (int n = 0; n < g_collectionList.size(); n++)
	{
		if (g_collectionList[n].collectionFields.size() > 1)
		{
			// also, ensure the parent now in the library shares some attributes in case want to add new collectable items newly to a level
			int entid = g_collectionList[n].iEntityID;
			if (entid > 0 && entid < t.entityprofile.size())
			{
				// all collectables in list are collectables, and resources are always favoured if flagged
				int e = g_collectionList[n].iEntityElementE;
				int iCollectableValue = 0;
				if (t.entityelement[e].eleprof.isProjectGlobal == 0)
				{
					if (e > 0 && e < t.entityelement.size()) iCollectableValue = t.entityelement[e].eleprof.iscollectable;
					if (iCollectableValue < 1) iCollectableValue = 1;
					if (iCollectableValue > t.entityprofile[entid].iscollectable) t.entityprofile[entid].iscollectable = iCollectableValue;
				}
			}
		}
	}
}

bool refresh_collection_from_entities(bool bLoadingLevel)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	// must ALWAYS add missing weapons to collection list, user does not manually add these
	char pRefreshLogCount[MAX_PATH];
	sprintf(pRefreshLogCount, "Refresh Collection From Entities: %d", g.entityelementmax);
	timestampactivity(0, pRefreshLogCount);
	for (int e = 1; e < g.entityelementmax; e++)
	{
		int entid = t.entityelement[e].bankindex;
		if (entid > 0)
		{
			bool bHoldingWeaponOnly = false;
			bool bHaveThisWeaponInList = false;
			LPSTR pThisWeaponName = t.entityprofile[entid].isweapon_s.Get();
			if (strlen(pThisWeaponName) == 0)
			{
				pThisWeaponName = t.entityelement[e].eleprof.hasweapon_s.Get();
				bHoldingWeaponOnly = true;
			}
			if (strlen(pThisWeaponName) > 0)
			{
				// find weapon in list
				int iCollectionListIndex = -1;
				cstr thisWeaponTitle = gun_names_tonormal(pThisWeaponName);
				for (int n = 0; n < g_collectionList.size(); n++)
				{
					if (g_collectionList[n].collectionFields.size() > 8)
					{
						// primary method to check style field for true weapon reference
						LPSTR pCollectionItemWeaponPath = g_collectionList[n].collectionFields[8].Get();
						if (strnicmp(pCollectionItemWeaponPath, "weapon=", 7) == NULL)
						{
							if (stricmp(pThisWeaponName, pCollectionItemWeaponPath + 7) == NULL)
							{
								bHaveThisWeaponInList = true;
								iCollectionListIndex = n;
								break;
							}
						}
						else
						{
							// fallback is item title (though not fool proof it preserves previous system for now)
							// but maybe leads to false positives if future weapons have same name as others (i.e pistol)
							// so do extra checks for older style weapons having NONE and NONE in PROFILE and STYLE
							if (stricmp (g_collectionList[n].collectionFields[8].Get(), "none") == NULL)
							{
								LPSTR pCollectionItemTitle = g_collectionList[n].collectionFields[0].Get();
								if (strlen(pCollectionItemTitle) > 0)
								{
									if (stricmp(thisWeaponTitle.Get(), pCollectionItemTitle) == NULL)
									{
										bHaveThisWeaponInList = true;
										iCollectionListIndex = n;
										break;
									}
								}
							}
						}
					}
				}
				if (bHaveThisWeaponInList == true)
				{
					// have the weapon, but if is not connected to a real inlevel object, can assign this now
					if (iCollectionListIndex != -1)
					{
						// can assign this now if the entity found is the actual weapon
						if (bHoldingWeaponOnly == false)
						{
							if (g_collectionList[iCollectionListIndex].iEntityID == 0)
							{
								g_collectionList[iCollectionListIndex].iEntityID = entid;
								g_collectionList[iCollectionListIndex].iEntityElementE = e;
							}
						}

						// and save to collection list
						extern bool g_bChangedGameCollectionList;
						g_bChangedGameCollectionList = true;
					}
				}
				else
				{
					if (bLoadingLevel == true)
					{
						// not adding when this is called while loading the level as it would create a never-ending sequence of
						// detecting a weapon, adding to list, manually deleting TSV entry, deleting level entry, then re-adding when level loaded
						// so skipping this add will allow users to delete TSV weapon entries and level instances to FULLY remove the old weapon
					}
					else
					{
						// weapon not in list, add it (isweapon or hasweapon)
						bool bValid = false;
						collectionItemType collectionitem;
						if (bHoldingWeaponOnly == true)
						{
							// add just the weapon indicated
							bValid = fill_rpg_item_defaults(&collectionitem, 0, e); // uses iAddThisItem mode 5 (weapon in eleprof)
						}
						else
						{
							// entity itself is the collectible
							bValid = fill_rpg_item_defaults(&collectionitem, entid, e);
						}
						if (bValid)
						{
							g_collectionList.push_back(collectionitem);
						}

						// and save to collection list
						extern bool g_bChangedGameCollectionList;
						g_bChangedGameCollectionList = true;
					}
				}
			}
		}
	}

	// replace any default images with correct paths
	for (int n = 0; n < g_collectionList.size(); n++)
	{
		if (g_collectionList[n].collectionFields.size() > 2)
		{
			if (stricmp(g_collectionList[n].collectionFields[2].Get(), "default") == NULL)
			{
				cstr entityfile = "";
				LPSTR pFind = g_collectionList[n].collectionFields[0].Get();
				for (int ee = 1; ee < g.entityelementmax; ee++)
				{
					LPSTR pThisEnt = t.entityelement[ee].eleprof.name_s.Get();
					if (stricmp (pThisEnt, pFind) == NULL)
					{
						int entid = t.entityelement[ee].bankindex;
						entityfile = t.entitybank_s[entid].Get();
						break;
					}
				}
				if (strlen(entityfile.Get()) > 0)
				{
					g_collectionList[n].collectionFields[2] = get_rpg_imagefinalfile(entityfile);
				}
			}
		}
	}

	// associate all collection items with present entity profiles and element eleprof copy
	bool bNeedMoreEntitiesLoading = false;
	for (int n = 0; n < g_collectionList.size(); n++)
	{
		if (g_collectionList[n].collectionFields.size() > 1)
		{
			LPSTR pCollectionItemTitle = g_collectionList[n].collectionFields[0].Get();
			if (strlen(pCollectionItemTitle) > 0 )
			{
				bool bFoundAndAssignedE = false;
				for (int ee = 1; ee < g.entityelementmax; ee++)
				{
					if (stricmp(t.entityelement[ee].eleprof.name_s.Get(), pCollectionItemTitle) == NULL)
					{
						int entid = t.entityelement[ee].bankindex;
						if (entid > 0)
						{
							if (g_collectionList[n].collectionFields.size() > 1)
							{
								if (stricmp(g_collectionList[n].collectionFields[1].Get(), "default") == NULL)
								{
									g_collectionList[n].collectionFields[1] = t.entitybank_s[entid];
								}
								g_collectionList[n].iEntityID = entid;
								g_collectionList[n].iEntityElementE = ee;
								bFoundAndAssignedE = true;
							}
						}
						break;
					}
				}
				if (bFoundAndAssignedE == false)
				{
					bNeedMoreEntitiesLoading = true;
				}
			}
		}
	}

	// for init on each game project, where is the INGAME HUD screen (can change as working in storyboard after initial opening)
	extern int FindLuaScreenNode(char* name);
	t.game.ingameHUDScreen = FindLuaScreenNode("HUD0");

	// return true if some entities missing during refresh (causes a load elsewhere)
	return bNeedMoreEntitiesLoading;
}

int find_rpg_collectionindex (char* pName)
{
	int collectionID = 0;
	for (int n = 0; n < g_collectionList.size(); n++)
	{
		if (g_collectionList[n].collectionFields.size() > 0)
		{
			if (stricmp(pName, g_collectionList[n].collectionFields[0].Get()) == NULL)
			{
				collectionID = 1 + n;
				break;
			}
		}
	}
	return collectionID;
}

bool bQuestEditor_Window = false;
extern bool bImGuiGotFocus;
extern int refresh_gui_docking;
extern bool bDigAHoleToHWND;
int current_quest_selection = 0;
std::vector<collectionQuestType> g_collectionQuestList_backup;
bool bGotQuestChanges = false;
extern int iLibraryStingReturnToID;
extern int iSelectedLibraryStingReturnID;
extern cstr sMakeDefaultSelecting;
extern cstr sSelectedLibrarySting;
extern bool g_bChangedGameCollectionList;
extern int g_iIconImageInPropertiesLastEntIndex;
extern int g_iIconImageInProperties;
extern cstr g_iconImageInPropertiesLastName_s;
extern float fPropertiesColoumWidth;
extern StoryboardStruct Storyboard;


int getNextUniqueQuestNumber()
{
	int highestNumber = 0;

	for (auto& quest : g_collectionQuestList)
	{
		if (quest.collectionFields.size() > 0)
		{
			std::string find = quest.collectionFields[0].Get();
			if (find.rfind("Quest ", 0) == 0)
			{
				try
				{
					int questNumber = std::stoi(find.substr(6));
					if (questNumber > highestNumber)
					{
						highestNumber = questNumber;
					}
				}
				catch (const std::exception& e) {
					continue;
				}
			}
		}
	}
	return highestNumber + 1;
}

bool bLastQuestEditor_Window = false;
bool bDelayedQuestEditor_Window = false;

void ProcessQuestEditor(void)
{
	if (bLastQuestEditor_Window != bQuestEditor_Window)
	{
		if (bGotQuestChanges)
		{
			if (current_quest_selection > 0 && current_quest_selection < g_collectionQuestList_backup.size())
			{
				int iAction = askBoxCancel("You have unsaved changes, save now ?", "Quest Editor Confirmation"); //1==Yes 2=Cancel 0=No
				if (iAction == 1)
				{
					int iCollectionItemIndex = current_quest_selection;

					g_collectionQuestList[iCollectionItemIndex] = g_collectionQuestList_backup[iCollectionItemIndex];
					save_rpg_system_quests(Storyboard.gamename);
					extern int iTriggerMessageFrames;
					extern char cSmallTriggerMessage[MAX_PATH];
					extern bool bTriggerSmallMessage;
					sprintf(cSmallTriggerMessage, "Quest has been saved!");
					iTriggerMessageFrames = 120;
					bTriggerSmallMessage = true;
					bGotQuestChanges = false;
				}
			}
		}

		//PE: Update DLUA quest settings when closing window.
		extern int fpe_current_loaded_script;
		fpe_current_loaded_script = -1;
		bLastQuestEditor_Window = bQuestEditor_Window;
	}

	if (g_collectionQuestLabels.size() == 0)
		return;
	if (!bQuestEditor_Window)
		return;

	float fs = ImGui::CalcTextSize("#").x;
	float but_gadget_size = ImGui::GetFontSize() * 12.0;

	if (refresh_gui_docking == 1)
	{
		ImGui::SetNextWindowSize(ImVec2(57 * ImGui::GetFontSize(), 50 * ImGui::GetFontSize()), ImGuiCond_Always);
		ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	}
	else
	{
		ImGui::SetNextWindowSize(ImVec2(57 * ImGui::GetFontSize(), 50 * ImGui::GetFontSize()), ImGuiCond_Once);
		ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
	}

	static float fLastContentWidth = 0;
	static ImVec2 vLastWindowSize = ImVec2(0, 0);
	if (refresh_gui_docking >= 3)
	{
		if (fLastContentWidth > 0 && fLastContentWidth < 700.0f && vLastWindowSize.y > 0)
		{
			ImGui::SetNextWindowSize(ImVec2(700.0f, vLastWindowSize.y), ImGuiCond_Always);
		}
	}

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
	if (bDigAHoleToHWND) window_flags |= ImGuiWindowFlags_ForceRender;

	ImGui::Begin("Quest Editor", &bQuestEditor_Window, window_flags);

	ImGuiWindow* bwindow = ImGui::GetCurrentWindow(); // ImGui::FindWindowByName("Save New Level##Storyboard");
	if (bDigAHoleToHWND && bwindow)
		bwindow->DrawList->AddCallback((ImDrawCallback)10, NULL); //force render.

	float columns_width[10];

	ImGui::Columns(7, "questeditorlistview");
	//ImGui::Separator();
	static bool bInitColumns = true;
	if (bInitColumns)
	{
		ImGui::SetColumnWidth(0, 200.0f);
		ImGui::SetColumnWidth(1, 90.0f);
		ImGui::SetColumnWidth(2, 110.0f);
		ImGui::SetColumnWidth(3, 90.0f);
		ImGui::SetColumnWidth(4, 90.0f);
		ImGui::SetColumnWidth(5, 90.0f);
		ImGui::SetColumnWidth(6, 200.0f);
		bInitColumns = false;
	}
	for(int i = 0; i < 7; i++)
		columns_width[i] = ImGui::GetColumnWidth(i);
	ImGui::Text("Title"); ImGui::NextColumn();
	ImGui::Text("Type"); ImGui::NextColumn();
	ImGui::Text("Object"); ImGui::NextColumn();
	ImGui::Text("Level"); ImGui::NextColumn();
	ImGui::Text("Qty"); ImGui::NextColumn();
	ImGui::Text("Status"); ImGui::NextColumn();
	ImGui::Text("End Map"); ImGui::NextColumn();
	ImGui::Columns(1);
	ImGui::Separator();

	#define TITLE_FIELD 0
	#define TYPE_FIELD 1
	#define IMAGE_FIELD 2
	#define DESC1_FIELD 3
	#define DESC2_FIELD 4
	#define DESC3_FIELD 5
	#define OBJECT_FIELD 6
	#define RECEIVER_FIELD 7
	#define LEVEL_FIELD 8
	#define POINTS_FIELD 9
	#define VALUE_FIELD 10
	#define STATUS_FIELD 11
	#define ACTIVATE_FIELD 12
	#define QUANTITY_FIELD 13
	#define ENDMAP_FIELD 14

	ImVec2  label_size = ImGui::CalcTextSize("#", NULL, true);
	static float line_height = (label_size.y + 4.0f);
	uint32_t entries = 5;
	float child_height = (line_height * entries);

	ImGui::BeginChild("##questscrollingregion", ImVec2(0, child_height),false , ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	float w = ImGui::GetContentRegionAvailWidth();
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	static float clipYMin, clipYMax;
	clipYMin = window->DC.CursorPos.y;

	//PE: Calc exact line height.
	ImVec2 cpos = ImGui::GetCursorPos();
	ImGui::ItemSize(label_size);
	line_height = ImGui::GetCursorPos().y - cpos.y;
	ImGui::SetCursorPos(cpos);

	ImGui::Columns(7, "editorlistviewentries",false);
	for (int i = 0; i < 7; i++ )
		ImGui::SetColumnWidth(i,columns_width[i]);

	//ImGui::Separator();
	char unique[80];
	strcpy(unique, "##");
	if (current_quest_selection >= g_collectionQuestList.size())
		current_quest_selection = g_collectionQuestList.size() - 1;

	if (g_collectionQuestList_backup.empty() && g_collectionQuestList.size() > 0)
		g_collectionQuestList_backup = g_collectionQuestList;

	for (int n = 0; n < g_collectionQuestList.size(); n++)
	{
		if (g_collectionQuestList[n].collectionFields.size() > 0)
		{
			bool bSelected = false;
			if (n == current_quest_selection)
				bSelected = true;
			strcat(unique, std::to_string((n)).c_str());
			if (bSelected)
			{
				const ImU32 col = ImGui::GetColorU32(ImGuiCol_Header);
				ImVec2 pos = window->DC.CursorPos;
				pos.y += window->DC.CurrLineTextBaseOffset;
				ImVec2 size_draw(w - 18.0f, label_size.y); //-18.0f = scrollbar
				ImRect bb(pos, pos + size_draw);

				const float THICKNESS = 2.0f;
				const float DISTANCEX = 3.0f + THICKNESS * 0.5f;
				const float DISTANCE = THICKNESS * 0.5f;
				bb.Expand(ImVec2(DISTANCEX, DISTANCE));
				
				ImRect bbClip = bb;
				bbClip.Min.y = clipYMin + ImGui::GetScrollY();
				bbClip.Max.y = clipYMin + child_height + ImGui::GetScrollY();

				ImGui::PushClipRect(bbClip.Min, bbClip.Max, false);
				window->DrawList->AddRectFilled(bb.Min, bb.Max, col, 0);
				ImGui::PopClipRect();

			}
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));

			int bret = ImGui::Selectable((g_collectionQuestList[n].collectionFields[TITLE_FIELD] + cstr(unique) + cstr("1")).Get(), &bSelected); ImGui::NextColumn();
			bret += ImGui::Selectable((g_collectionQuestList[n].collectionFields[TYPE_FIELD] + cstr(unique) + cstr("2")).Get(), &bSelected); ImGui::NextColumn();
			bret += ImGui::Selectable((g_collectionQuestList[n].collectionFields[OBJECT_FIELD] + cstr(unique) + cstr("3")).Get(), &bSelected); ImGui::NextColumn();
			bret += ImGui::Selectable((g_collectionQuestList[n].collectionFields[LEVEL_FIELD] + cstr(unique) + cstr("4")).Get(), &bSelected); ImGui::NextColumn();
			bret += ImGui::Selectable((g_collectionQuestList[n].collectionFields[QUANTITY_FIELD] + cstr(unique) + cstr("5")).Get(), &bSelected); ImGui::NextColumn();
			bret += ImGui::Selectable((g_collectionQuestList[n].collectionFields[STATUS_FIELD] + cstr(unique) + cstr("6")).Get(), &bSelected); ImGui::NextColumn();
			
			cstr tmp = g_collectionQuestList[n].collectionFields[ENDMAP_FIELD];
			if (tmp == "none")
				tmp = "Current Level";
			bret += ImGui::Selectable((tmp + cstr(unique) + cstr("7")).Get(), &bSelected); ImGui::NextColumn();
			ImGui::PopStyleColor();
			if (bret > 0)
			{
				if (bGotQuestChanges)
				{
					if (current_quest_selection > 0 && current_quest_selection < g_collectionQuestList_backup.size())
					{
						int iAction = askBoxCancel("You have unsaved changes, save now ?", "Quest Editor Confirmation"); //1==Yes 2=Cancel 0=No
						if (iAction == 1)
						{
							int iCollectionItemIndex = current_quest_selection;

							g_collectionQuestList[iCollectionItemIndex] = g_collectionQuestList_backup[iCollectionItemIndex];
							save_rpg_system_quests(Storyboard.gamename);
							extern int iTriggerMessageFrames;
							extern char cSmallTriggerMessage[MAX_PATH];
							extern bool bTriggerSmallMessage;
							sprintf(cSmallTriggerMessage, "Quest has been saved!");
							iTriggerMessageFrames = 120;
							bTriggerSmallMessage = true;
							bGotQuestChanges = false;
						}
					}
				}
				current_quest_selection = n;
				g_collectionQuestList_backup = g_collectionQuestList; //PE: Copy everything so we work on a backup.
				bGotQuestChanges = false;
				g_iIconImageInProperties = 0;
			}
		}
	}
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::Separator();
	
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2.0f);

	float px = (ImGui::GetContentRegionAvailWidth()) - 4.0f;
	px -= but_gadget_size;
	px += ImGui::GetCursorPosX();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

	if (ImGui::StyleButton("Insert", ImVec2(but_gadget_size, 0)))
	{
		std::string newname = "Quest " + std::to_string(getNextUniqueQuestNumber());
		collectionQuestType item;
		fill_rpg_quest_defaults(&item, (char *) newname.c_str());
		int i = current_quest_selection + 1;
		if (current_quest_selection >= 0 && i <= g_collectionQuestList.size())
		{
			g_collectionQuestList.insert(g_collectionQuestList.begin() + i, item);
			current_quest_selection++;
		}
		else
		{
			g_collectionQuestList.push_back(item);
			current_quest_selection = g_collectionQuestList.size() - 1;
		}
		g_collectionQuestList_backup = g_collectionQuestList;
		bGotQuestChanges = false;
		g_iIconImageInProperties = 0;
	}
	if (current_quest_selection >= 0 && current_quest_selection < g_collectionQuestList.size())
	{
		ImGui::SameLine();
		if (ImGui::StyleButton("Delete", ImVec2(but_gadget_size, 0)))
		{
			int iAction = askBoxCancel("This will delete the selected quest, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
			if (iAction == 1)
			{
				g_collectionQuestList.erase(g_collectionQuestList.begin() + current_quest_selection);
				if (current_quest_selection >= g_collectionQuestList.size())
					current_quest_selection = g_collectionQuestList.size() - 1;
				g_collectionQuestList_backup = g_collectionQuestList;
				bGotQuestChanges = false;
				g_iIconImageInProperties = 0;
			}

		}
	}
	ImGui::SameLine();
	ImGui::SetCursorPosX(px);
	if (ImGui::StyleButton("Exit", ImVec2(but_gadget_size, 0)))
	{
		bQuestEditor_Window = false;
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

	//-----------------------------------------------
	if (ImGui::StyleCollapsingHeader("Quest Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (current_quest_selection != -1 && g_collectionQuestList_backup.size() == g_collectionQuestList.size())
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			int iCollectionItemIndex = current_quest_selection;
			ImGui::Columns(2, "questeditorcolumns2", false);  //false no border

			{
				int iEntityIndex = 0; //PE: TODO need the selected item here.
				int GetActiveEditorEntity(void);
				iEntityIndex = GetActiveEditorEntity();
				if (iEntityIndex >= t.entityelement.size())
				{
					iEntityIndex = 0;
				}
				int iMasterID = t.entityelement[iEntityIndex].bankindex;
				extern bool bDraggingActive;
				if (bDraggingActive && t.widget.pickedEntityIndex > 0 && t.gridentity > 0)
				{
					//PE: Keep displaying old info, while dragging a gridentity around.
					iMasterID = t.gridentity;
				}

				// show collectable details
				bool bQuestTypeIsCollect = false;
				ImGui::Indent(10);
				int iCount = g_collectionQuestList_backup[iCollectionItemIndex].collectionFields.size();
				for (int l = 0; l < iCount; l++)
				{
					int iKnownLabel = -1;
					LPSTR pLabel = "";
					pLabel = g_collectionQuestLabels[l].Get();
					if (stricmp(pLabel, "title") == NULL) iKnownLabel = 0;
					if (stricmp(pLabel, "image") == NULL) iKnownLabel = 2;
					if (stricmp(pLabel, "type") == NULL) iKnownLabel = 51;
					if (stricmp(pLabel, "desc1") == NULL) iKnownLabel = 52;
					if (stricmp(pLabel, "desc2") == NULL) iKnownLabel = 53;
					if (stricmp(pLabel, "desc3") == NULL) iKnownLabel = 54;
					if (stricmp(pLabel, "object") == NULL) iKnownLabel = 55;
					if (stricmp(pLabel, "receiver") == NULL) iKnownLabel = 56;
					if (stricmp(pLabel, "level") == NULL) iKnownLabel = 57;
					if (stricmp(pLabel, "points") == NULL) iKnownLabel = 58;
					if (stricmp(pLabel, "value") == NULL) iKnownLabel = 59;
					if (stricmp(pLabel, "status") == NULL) iKnownLabel = 60;
					if (stricmp(pLabel, "activate") == NULL) iKnownLabel = 61;
					if (stricmp(pLabel, "quantity") == NULL) iKnownLabel = 62;
					if (stricmp(pLabel, "endmap") == NULL) iKnownLabel = 63;
					if (iKnownLabel == 55)
					{
						ImGui::NextColumn();
					}
					if (iKnownLabel >= 0)
					{
						// Any tip
						LPSTR pShowTop = "";
						pShowTop = "Enter a value for this quest";
						if (iKnownLabel == 2) pShowTop = "Select an image that will be used to represent this quest in your HUD screens";
						if (iKnownLabel == 51) pShowTop = "Enter a quest type for the quest task";
						if (iKnownLabel == 52) pShowTop = "Enter a description for the quest task";
						if (iKnownLabel == 53) pShowTop = "Enter a description for the quest task";
						if (iKnownLabel == 54) pShowTop = "Enter a description for the quest task";
						if (iKnownLabel == 55) pShowTop = "Enter the name of an object that will represent the quest object";
						if (iKnownLabel == 56) pShowTop = "Enter the name of an object that will represent the quest receiver";
						if (iKnownLabel == 57) pShowTop = "Enter the player level required to activate this quest";
						if (iKnownLabel == 58) pShowTop = "Enter the number of XP points awarded when this quest is completed";
						if (iKnownLabel == 59) pShowTop = "Enter the money earned by completing this quest";
						if (iKnownLabel == 60) pShowTop = "Enter the initial status of this quest when the game starts";
						if (iKnownLabel == 61) pShowTop = "Enter the object to activate when this quest is completed";
						if (iKnownLabel == 62) pShowTop = "Enter a quantity associated with this quest";
						if (iKnownLabel == 63) pShowTop = "Enter the level name that this quest is active on";

						// Attrib Label
						if (iKnownLabel == 2)
						{
							// can change image
							LPSTR pImageLabel = "";
							pImageLabel = "Quest Icon Image";
							ImGui::TextCenter(pImageLabel);
							float w = ImGui::GetContentRegionAvailWidth();
							cstr UniqueCollectionItemImage = "##UniqueCollectionItemImage";
							if (iSelectedLibraryStingReturnID == window->GetID(UniqueCollectionItemImage.Get()))
							{
								g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = sSelectedLibrarySting.Get();
								g_bChangedGameCollectionList = true;
								sSelectedLibrarySting = "";
								iSelectedLibraryStingReturnID = -1; //disable.
								g_iIconImageInPropertiesLastEntIndex = -1;// trigger reload
							}

							int entid = 0;
							if (iEntityIndex > 0) entid = t.entityelement[iEntityIndex].bankindex;
							if (g_iIconImageInPropertiesLastEntIndex != iEntityIndex)
							{
								g_iIconImageInPropertiesLastEntIndex = iEntityIndex;
								g_iconImageInPropertiesLastName_s = "";
								if (entid > 0) g_iconImageInPropertiesLastName_s = t.entitybank_s[entid];
								g_iIconImageInProperties = 0;
							}
							else
							{
								if (entid > 0)
								{
									// even if sale element index, can delete and quickly create another collectable in same index slot, need to be aware of this
									if (strcmp(g_iconImageInPropertiesLastName_s.Get(), t.entitybank_s[entid].Get()) != NULL)
									{
										g_iconImageInPropertiesLastName_s = t.entitybank_s[entid];
										g_iIconImageInProperties = 0;
									}
								}
							}
							LPSTR pIconImageInProperties = "";
							pIconImageInProperties = g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l].Get();

							if (g_iIconImageInProperties == 0)
							{
								if (FileExist(pIconImageInProperties) == 0)
								{
									// image specified does not exist, original file could have moved/deleted, so revert to default
									pIconImageInProperties = "default";
								}
								cstr actualImgFile_s = "";
								if (stricmp(pIconImageInProperties, "default") == NULL)
								{
									// replace with actual img file if viewing property
									cstr entityfile = "noentityselected";
									if(iMasterID > 0)
										entityfile = t.entitybank_s[iMasterID];
									actualImgFile_s = get_rpg_imagefinalfile(entityfile);
									pIconImageInProperties = actualImgFile_s.Get();
									g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = pIconImageInProperties;
									g_bChangedGameCollectionList = true;
								}
								g_iIconImageInProperties = g.iconimagebankoffset;
								if (GetImageExistEx(g_iIconImageInProperties) == 1) DeleteImage(g_iIconImageInProperties);
								image_setlegacyimageloading(true);
								if (FileExist(pIconImageInProperties) == 1)
								{
									// actual icon image
									LoadImage(pIconImageInProperties, g_iIconImageInProperties);
								}
								else
								{
									// specified image not found, use placeholder
									pIconImageInProperties = "imagebank\\HUD Library\\MAX\\object.png";
									LoadImage(pIconImageInProperties, g_iIconImageInProperties);
								}
								image_setlegacyimageloading(false);
							}
							int iTextureID = g_iIconImageInProperties;
							ImVec2 ImageSize = ImVec2((ImGui::GetWindowContentRegionWidth()*0.5f) - 34.0f, ImGui::GetFontSize());
							float centerx = ImageSize.x;
							ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iTextureID);
							if (lpTexture)
							{
								float img_w = ImageWidth(iTextureID);
								float img_h = ImageHeight(iTextureID);
								ImageSize.y = img_h * (ImageSize.x / img_w);
								if (ImageSize.y > 220)
								{
									ImageSize.y = 220;
									ImageSize.x = img_w * (ImageSize.y / img_h);
								}
								if (ImageSize.y > img_h && ImageSize.x > img_h)
								{
									ImageSize.y = img_h;
									ImageSize.x = img_h;
								}
							}

							ImVec2 vImagePos = ImGui::GetCursorPos();
							centerx = vImagePos.x + ((centerx * 0.5f) - (ImageSize.x * 0.5f));
							vImagePos.x = centerx;
							ImGui::SetCursorPosX(centerx);
							ImGui::Dummy(ImageSize);
							ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
							ImVec4 back_color = ImVec4(0.2, 0.2, 0.2, 0.75);

							extern cstr sStartLibrarySearchString;
							extern bool bExternal_Entities_Window;
							extern int iDisplayLibraryType;
							if (ImGui::IsItemHovered())
							{
								color.w = 0.75;
								if (ImGui::IsMouseReleased(0))
								{
									sStartLibrarySearchString = "Icon";
									bExternal_Entities_Window = true;
									iDisplayLibraryType = 2; //Image
									iLibraryStingReturnToID = window->GetID(UniqueCollectionItemImage.Get());
									bGotQuestChanges = true;
								}
							}
							
							ImVec2 img_pos = ImGui::GetWindowPos() + vImagePos;
							img_pos.y -= ImGui::GetScrollY();
							window->DrawList->AddRectFilled(img_pos, img_pos + ImageSize, ImGui::GetColorU32(back_color));
							if (lpTexture)
							{
								window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + ImageSize, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(color));
							}
							else
							{
								window->DrawList->AddRectFilled(img_pos, img_pos + ImageSize, ImGui::GetColorU32(color));
							}
							lpTexture = GetImagePointerView(TOOL_PENCIL); //Add pencil
							if (lpTexture)
							{
								ImVec2 vDrawPos;// = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - ImageSize.y - 3.0f };
								vDrawPos = img_pos + ImVec2(ImageSize.x - 20.0f, 4.0f);
								window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
							}
							if (ImGui::IsItemHovered() && pShowTop) ImGui::SetTooltip(pShowTop);
						}
						else
						{
							if (iKnownLabel == 63)
							{
								char cTmpInput[MAX_PATH];
								char title[MAX_PATH] = "none";
								char preview[MAX_PATH];

								strcpy(cTmpInput, g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l].Get());
								strcpy(preview, cTmpInput);
								if (stricmp(cTmpInput, "none") == 0)
								{
									strcpy(title, "Current Level");
									strcpy(preview, title);
								}

								ImGui::PushItemWidth(-10);

								ImGui::TextCenter("End Map");

								if (ImGui::BeginCombo("##SELECTLEVELCOMBO", preview))
								{
									bool bSelected = false;
									if (stricmp(cTmpInput, "none") == 0)
									{
										bSelected = true;
									}
									if (ImGui::Selectable(title, &bSelected))
									{
										g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = "none";
										bGotQuestChanges = true;
									}

									for (int i = 0; i < STORYBOARD_MAXNODES; i++)
									{
										if (Storyboard.Nodes[i].used && strlen(Storyboard.Nodes[i].level_name) > 0)
										{
											title[0] = 0;
											int offset = 0;
											if (strstr(Storyboard.Nodes[i].level_name, "mapbank"))
												offset = 8;
											strcpy(title, Storyboard.Nodes[i].level_name + offset);
											if(strlen(title) > 4)
												title[strlen(title) - 4] = 0;
											ImGui::PushID(92679 + i);
											bSelected = false;
											if (stricmp(cTmpInput, title) == 0)
											{
												bSelected = true;
											}
											if (ImGui::Selectable(title,&bSelected))
											{
												g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = title;
												bGotQuestChanges = true;
											}
											ImGui::PopID();
										}
									}
									ImGui::EndCombo();
								}
								ImGui::PopItemWidth();
								
							}
							else if (iKnownLabel == 51)
							{
								// drop down to make life easier
								char cTmpInput[MAX_PATH];
								strcpy(cTmpInput, g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l].Get());
								const char* items[] = { "Collect", "Destroy", "Deliver", "Activate" };
								int item_current = 0;
								if (stricmp(cTmpInput, "collect") == NULL) item_current = 0;
								if (stricmp(cTmpInput, "destroy") == NULL) item_current = 1;
								if (stricmp(cTmpInput, "deliver") == NULL) item_current = 2;
								if (stricmp(cTmpInput, "activate") == NULL) item_current = 3;
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
								ImGui::TextCenter("Quest Type");
								//ImGui::SameLine();
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
								//ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
								ImGui::PushItemWidth(-10);
								if (ImGui::Combo("##combostaticQuestType2", &item_current, items, IM_ARRAYSIZE(items)))
								{
									bGotQuestChanges = true;
									if (item_current == 0) g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = "collect";
									if (item_current == 1) g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = "destroy";
									if (item_current == 2) g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = "deliver";
									if (item_current == 3) g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = "activate";
								}
								ImGui::PopItemWidth();

								// optional quantity property only applicable to some quest types
								if (item_current == 0)
								{
									// later in loop we use this flag to allow quantity to show
									bQuestTypeIsCollect = true;
								}
							}
							else
							{
								// good old typing out your entry
								bool bAllowEditing = true;
								if (iKnownLabel == 1) bAllowEditing = false;
								if (t.entityprofile[iMasterID].isweapon > 0 && iKnownLabel >= 7) bAllowEditing = false;
								if (iKnownLabel == 62 && bQuestTypeIsCollect == false) bAllowEditing = false;
								if (bAllowEditing == true)
								{
									char pNameOfAttrib[MAX_PATH];
									strcpy(pNameOfAttrib, "Quest ");
									char pCap[2];
									pCap[0] = pLabel[0];
									pCap[1] = 0;
									strupr(pCap);
									strcat(pNameOfAttrib, pCap);
									strcat(pNameOfAttrib, pLabel + 1);
									ImGui::TextCenter(pNameOfAttrib);
									ImGui::PushItemWidth(-10);
									char cTmpInput[MAX_PATH];
									strcpy(cTmpInput, g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l].Get());
									int inputFlags = 0;
									char pNameOfAttribUnique[MAX_PATH];
									strcpy(pNameOfAttribUnique, "##CollectableItem");
									strcat(pNameOfAttribUnique, pLabel);
									if (ImGui::InputText(pNameOfAttribUnique, &cTmpInput[0], 1024, inputFlags))
									{
										g_collectionQuestList_backup[iCollectionItemIndex].collectionFields[l] = cTmpInput;
										bImGuiGotFocus = true;
										g_bChangedGameCollectionList = true;
										bGotQuestChanges = true;
									}
									if (ImGui::IsItemHovered() && pShowTop) ImGui::SetTooltip(pShowTop);
									if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
									ImGui::PopItemWidth();
								}
							}
						}
					}
				}

				ImGui::Text("");
				if (strlen(Storyboard.gamename) > 0)
				{
					if (Storyboard.project_readonly != 1)
					{
						float px = (ImGui::GetWindowContentRegionWidth() * 0.5f) - 24.0f;
						px -= but_gadget_size;
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + px);

						bool changes = bGotQuestChanges;
						if (!changes)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						if (ImGui::StyleButton("Save", ImVec2(but_gadget_size, 0)))
						{
							g_collectionQuestList[iCollectionItemIndex] = g_collectionQuestList_backup[iCollectionItemIndex];
							save_rpg_system_quests(Storyboard.gamename);
							extern int iTriggerMessageFrames;
							extern char cSmallTriggerMessage[MAX_PATH];
							extern bool bTriggerSmallMessage;
							sprintf(cSmallTriggerMessage, "Quest has been saved!");
							iTriggerMessageFrames = 120;
							bTriggerSmallMessage = true;
							bGotQuestChanges = false;
						}

						if (!changes)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

					}
					else
						ImGui::Text("Error: read only storyboard.");
				}
				else
				{
					ImGui::Text("Error: No storyboard found.");
				}
			}
			ImGui::Indent(-10);
			ImGui::Columns(1);
		}
	}

	ImGui::Text("");
	ImVec2 ws = ImGui::GetWindowSize();
	ImGui::Indent();
	if (ImGui::GetCursorPosY() < ws.y - (fs * 4))
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ws.y - (fs * 4) + ImGui::GetScrollY()));

	vLastWindowSize = ImGui::GetWindowSize();
	fLastContentWidth = ImGui::GetContentRegionAvailWidth();

	bImGuiGotFocus = true;

	ImGui::End();
	if (bDigAHoleToHWND && bwindow)
		bwindow->DrawList->AddCallback((ImDrawCallback)11, NULL); //disable force render.

}
