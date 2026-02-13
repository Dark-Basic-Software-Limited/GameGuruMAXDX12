//----------------------------------------------------
//--- GAMEGURU - M-CharacterCreatorPlus
//----------------------------------------------------

// Includes
#include "stdafx.h"
#include "gameguru.h"
#include "M-CharacterCreatorPlusTTS.h"
#include "CCameraC.h"

//PE: GameGuru IMGUI.
#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

#include "master.h"
extern Master master;

// Defines
#define CCPMODELEXT ".dbo"

// Globals
bool g_bCharacterCreatorPlusActivated = false;
sCharacterCreatorPlus g_CharacterCreatorPlus;
bool g_bCharacterCreatorPrepAnims = false;

bool g_bLegsChangeCascade = false;
bool g_bFeetChangeCascade = false;

bool g_charactercreatorplus_preloading = false;
char g_charactercreatorplus_path[MAX_PATH];
int g_charactercreatorplus_part = 0;
char g_charactercreatorplus_tag[MAX_PATH];

// <mesh name, path>
static std::map<std::string, std::string> CharacterCreatorHeadGear_s;
static std::map<std::string, std::string> CharacterCreatorHair_s;
static std::map<std::string, std::string> CharacterCreatorHead_s;
static std::map<std::string, std::string> CharacterCreatorEyeglasses_s;
static std::map<std::string, std::string> CharacterCreatorFacialHair_s;
static std::map<std::string, std::string> CharacterCreatorBody_s;
static std::map<std::string, std::string> CharacterCreatorLegs_s;
static std::map<std::string, std::string> CharacterCreatorFeet_s;

static std::map<std::string, std::string> CharacterCreatorAccessory1_s;
static std::map<std::string, std::string> CharacterCreatorAccessory2_s;


// <mesh name, name shown to user>
static std::map<std::string, std::string> g_charactercreatorplus_annotation_list;
static std::map<std::string, std::string> CharacterCreatorAnnotatedHeadGear_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedHair_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedHead_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedEyeglasses_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedFacialHair_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedBody_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedFeet_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedLegs_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedAccessory1_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedAccessory2_s;




static std::map<std::string, std::string> g_charactercreatorplus_annotationtag_list;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagHeadGear_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagHair_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagHead_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagEyeglasses_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagFacialHair_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagBody_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagFeet_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagLegs_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagAccessory1_s;
static std::map<std::string, std::string> CharacterCreatorAnnotatedTagAccessory2_s;


std::vector<sCharacterType> g_CharacterType;
char pCharacterTypeDropDownList[32][260];
std::vector<int> g_CharacterTypeRoomPref;

std::vector<sRoomType> g_RoomType;
char pRoomTypeDropDownList[32][260];

std::vector<AutoSwapData*> g_headGearMandatorySwaps;
AutoSwapData* g_previousAutoSwap = nullptr;
#define MAXPARTICONS 100
int g_iPartsThatNeedReloaded[10] = { 0 };
int g_iPartsIconsIDs[32][10][MAXPARTICONS];
bool g_bPartIconsInit = false;
char g_SkinTextureStorage[MAX_PATH]; // Stores the skin tone texture when the face is changed. Referenced when changing the face to fit certain headgear.
std::vector<char*> g_restrictedParts;
float g_fLockerRoomOffset = 2.0f;
float g_fCCPZoom = 72.0f;
std::array<std::string, 10> g_maleStorage;
std::array<std::string, 10> g_femaleStorage;
std::array<std::string, 10> g_zombieStorage;
std::array<std::string, 10> g_genericStorage;
AutoSwapData* g_pLastHeadgearAutoSwap = nullptr;
int g_iPreviousCategorySelection = -1;
CameraTransition* g_pCurrentTransition = nullptr;
CameraTransition g_HeadTransition;
CameraTransition g_UpperBodyTransition;
CameraTransition g_LowerBodyTransition;
CameraTransition g_ZombieHeadTransition;
CameraTransition g_ZombieBodyTransition;
CameraTransition* g_pLastKnownTransition = nullptr;
GGVECTOR3 g_DefaultCamPosition;
GGVECTOR3 g_CurrentCamPosition;
GGVECTOR3 g_DefaultCamAngle;
GGVECTOR3 g_CurrentCamAngle;
void charactercreatorplus_dozoom();
int item_current_type_selection = 5;
uint32_t iLightIndex = -1;
std::vector<const char*> g_MeshesThatNeedDoubleSided;

static std::map<std::string, std::string> CharacterCreatorType_s;
int iDressRoom = 0, iCharObj = 0, iCharObjHeadGear = 0, iCharObjHair = 0, iCharObjHead = 0, iCharObjAccessory1 = 0, iCharObjAccessory2 = 0, iCharObjEyeglasses = 0, iCharObjFacialHair = 0, iCharObjLegs = 0, iCharObjFeet = 0;


bool bCharObjVisible = false;
char CCP_Room[260] = "lockers";
char CCP_Type[260] = "adult male";
char CCP_Name[260] = "\0";
static char CCP_Script[260] = "people\\patrol.lua";
char CCP_Path[260] = "entitybank\\user\\charactercreatorplus\\";
char CCP_SpeakText[1024] = "Hello there, I am a new character!\n";
wchar_t CCP_SpeakText_w[1024];

int CCP_Speak_Rate = 0;
char cSelectedLegsFilter[260] = "\0";
char cSelectedFeetFilter[260] = "\0";
char cSelectedICCode[260] = "\0";
char cSelectedHeadGear[260] = "\0";
char cSelectedHair[260] = "\0";
char cSelectedHead[260] = "\0";
char cSelectedEyeglasses[260] = "\0";
char cSelectedAccessory1[260] = "\0";
char cSelectedAccessory2[260] = "\0";

char cSelectedFacialHair[260] = "\0";
char cSelectedBody[260] = "\0";
char cSelectedLegs[260] = "\0";
char cSelectedFeet[260] = "\0";
ISpObjectToken * CCP_SelectedToken = 0;
LPSTR pCCPVoiceSet = "";
ImVec4 vColorSelected[5];
float oldx_f, oldy_f, oldz_f, oldangx_f, oldangy_f;
float editoroldx_f=0, editoroldy_f, editoroldz_f, editoroldangx_f, editoroldangy_f, editoroldmode_f, oldtcameraviewmode=1;
int iDelayThumbs = 99; //0; //Icon removed
int iDelayExecute = 0;
int iThumbsOffsetY = 0;
float fCharObjectY = 600.0f;
float ccpTargetX, ccpTargetY, ccpTargetZ, ccpTargetAX, ccpTargetAY , dressroomTargetAY;
float ccpObjTargetX, ccpObjTargetY, ccpObjTargetZ, ccpObjTargetAX, ccpObjTargetAY, ccpObjTargetAZ;
float fCCPRotateY = 0.0f;
entityeleproftype g_grideleprof_holdchoices;

extern bool bTriggerMessage;
extern char cTriggerMessage[MAX_PATH];
void DisplaySmallImGuiMessage(char *text);
bool bMessageDisplayed = false;

bool g_bCharacterCreatorTypesInit = false;

extern preferences pref;

void charactercreatorplus_populatechartypes (void)
{
	if (g_bCharacterCreatorTypesInit == false)
	{
		// store current dir
		cstr pOldDir = GetDir();
		GG_SetWritablesToRoot(1);

		// collect rooms
		sRoomType roomtypeitem;
		strcpy (roomtypeitem.pPartsFolder, "locker room");  g_RoomType.push_back(roomtypeitem); //0
		strcpy (roomtypeitem.pPartsFolder, "pine dressing room");  g_RoomType.push_back(roomtypeitem); //1
		strcpy (roomtypeitem.pPartsFolder, "zombie locker room");  g_RoomType.push_back(roomtypeitem); //2
		SetDir("charactercreatorplus\\rooms");
		ChecklistForFiles();
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			LPSTR pThisFile = t.tfile_s.Get();
			if (Len(pThisFile) > 2)
			{
				if (stricmp(pThisFile, "locker room") == NULL || stricmp(pThisFile, "pine dressing room") == NULL || stricmp(pThisFile, "zombie locker room") == NULL)
				{
					// already have these from above
				}
				else
				{
					char pCheckRoomFile[MAX_PATH];
					sprintf(pCheckRoomFile, "%s\\room.dbo", t.tfile_s.Get());
					if (FileExist (pCheckRoomFile) == 1)
					{
						strcpy (roomtypeitem.pPartsFolder, t.tfile_s.Get());
						g_RoomType.push_back(roomtypeitem);
					}
				}
			}
		}
		// make dropdown string array from above
		int n = 0;
		for (; n < g_RoomType.size(); n++)
		{
			bool bCapitaliseOnNewWord = true;
			strcpy (pRoomTypeDropDownList[n], g_RoomType[n].pPartsFolder);
			for (int t = 0; t < strlen(pRoomTypeDropDownList[n]); t++)
			{
				if (pRoomTypeDropDownList[n][t] >= 'a' && pRoomTypeDropDownList[n][t] <= 'z')
				{
					if (bCapitaliseOnNewWord == true)
					{
						pRoomTypeDropDownList[n][t] -= 32;
					}
				}
				bCapitaliseOnNewWord = false;
				if (pRoomTypeDropDownList[n][t] == ' ') bCapitaliseOnNewWord = true;
			}
		}
		for (; n < 32; n++) strcpy(pRoomTypeDropDownList[n], "");
		SetDir(pOldDir.Get());

		// gather character creator types database
		sCharacterType chartypeitem;
		strcpy (chartypeitem.pPartsFolder, "adult male");  g_CharacterType.push_back(chartypeitem); g_CharacterTypeRoomPref.push_back(0);
		strcpy (chartypeitem.pPartsFolder, "adult female");  g_CharacterType.push_back(chartypeitem); g_CharacterTypeRoomPref.push_back(1);
		strcpy (chartypeitem.pPartsFolder, "zombie male");  g_CharacterType.push_back(chartypeitem); g_CharacterTypeRoomPref.push_back(2);
		strcpy (chartypeitem.pPartsFolder, "zombie female");  g_CharacterType.push_back(chartypeitem); g_CharacterTypeRoomPref.push_back(2);
		SetDir("charactercreatorplus\\parts");
		ChecklistForFiles();
		// first pass for annotations
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			LPSTR pThisFile = t.tfile_s.Get();
			if (Len(pThisFile) > 2)
			{
				if (stricmp(pThisFile, "adult male") == NULL || stricmp(pThisFile, "adult female") == NULL
				||  stricmp(pThisFile, "zombie male") == NULL || stricmp(pThisFile, "zombie female") == NULL)
				{
					// already have these from above
				}
				else
				{
					// confirm a real body parts folder
					char pCheckAnnotFile[MAX_PATH];
					sprintf(pCheckAnnotFile, "%s\\annotates.txt", t.tfile_s.Get());
					if (FileExist (pCheckAnnotFile) == 1)
					{
						sCharacterType chartypeitem;
						strcpy (chartypeitem.pPartsFolder, t.tfile_s.Get());
						g_CharacterType.push_back(chartypeitem);
						g_CharacterTypeRoomPref.push_back(0);
					}
				}
			}
		}
		// second pass for roomprefs
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			LPSTR pThisCharFile = t.tfile_s.Get();
			if (Len(pThisCharFile) > 2)
			{
				char pCheckRoomPrefFile[MAX_PATH];
				sprintf(pCheckRoomPrefFile, "%s\\roompref.txt", pThisCharFile);
				if (FileExist (pCheckRoomPrefFile) == 1)
				{
					OpenToRead(1, pCheckRoomPrefFile);
					LPSTR pRoomPref = ReadString(1);
					CloseFile(1);
					for (int n = 0; n < g_CharacterType.size(); n++)
					{
						if (stricmp(g_CharacterType[n].pPartsFolder, pThisCharFile) == NULL)
						{
							// find pRoomPref string in room types list
							for (int r = 0; r < g_RoomType.size(); r++)
							{
								if (stricmp(g_RoomType[r].pPartsFolder, pRoomPref) == NULL)
								{
									g_CharacterTypeRoomPref[n] = r;
									break;
								}
							}
						}
					}
				}
			}
		}
		// make dropdown string array from above
		n = 0;
		for (; n < g_CharacterType.size(); n++)
		{
			bool bCapitaliseOnNewWord = true;
			strcpy (pCharacterTypeDropDownList[n], g_CharacterType[n].pPartsFolder);
			for (int t = 0; t < strlen(pCharacterTypeDropDownList[n]); t++)
			{
				if (pCharacterTypeDropDownList[n][t] >= 'a' && pCharacterTypeDropDownList[n][t] <= 'z')
				{
					if (bCapitaliseOnNewWord == true)
					{
						pCharacterTypeDropDownList[n][t] -= 32;
					}
				}
				bCapitaliseOnNewWord = false;
				if (pCharacterTypeDropDownList[n][t] == ' ') bCapitaliseOnNewWord = true;
			}
		}
		for (; n < 32; n++) strcpy(pCharacterTypeDropDownList[n], "");

		// completed init
		SetDir(pOldDir.Get());
		GG_SetWritablesToRoot(0);
		g_bCharacterCreatorTypesInit = true;
	}
}

void charactercreatorplus_preloadinitialcharacter ( void )
{
	return; //PE: Disabled until we can do multiply thread loads.
}

void charactercreatorplus_copyselections(std::array<std::string, 10>& storage)
{
	// 0: Head Gear
	// 1: Hair
	// 2: Head
	// 3: Eye Glasses
	// 4: Facial Hair
	// 5: Body
	// 6: Legs
	// 7: Feet
	storage[0] = std::string(cSelectedHeadGear);
	storage[1] = std::string(cSelectedHair);
	storage[2] = std::string(cSelectedHead);
	storage[3] = std::string(cSelectedEyeglasses);
	storage[4] = std::string(cSelectedFacialHair);
	storage[5] = std::string(cSelectedBody);
	storage[6] = std::string(cSelectedLegs);
	storage[7] = std::string(cSelectedFeet);
	storage[8] = std::string(cSelectedAccessory1);
	storage[9] = std::string(cSelectedAccessory2);
}

void charactercreatorplus_GetDefaultCharacterPartNum (int iBase, int iPart, LPSTR pPartNumStr, LPSTR pPartNumVariantStr = NULL)
{
	LPSTR pPart = "";
	LPSTR pPartVariant = "";
	if (iBase == 1)
	{
		// male
		if (iPart == 1) { pPart = "13"; pPartVariant = "13c"; }
		if (iPart == 2) { pPart = "07"; pPartVariant = "07"; }
		if (iPart == 3) { pPart = "12"; pPartVariant = "12c"; }
		if (iPart == 4) { pPart = "10"; pPartVariant = "10"; }
		if (iPart == 5) { pPart = "10"; pPartVariant = "10"; }
	}
	if (iBase == 2)
	{
		// female
		if (iPart == 1) { pPart = "07"; pPartVariant = "07"; }
		if (iPart == 2) { pPart = "07"; pPartVariant = "07"; }
		if (iPart == 3) { pPart = "07"; pPartVariant = "07"; }
		if (iPart == 4) { pPart = "05"; pPartVariant = "05"; }
		if (iPart == 5) { pPart = "06"; pPartVariant = "06"; }
	}
	if (iBase == 3)
	{
		// zombie male
		if (iPart == 1) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 2) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 3) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 4) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 5) { pPart = "01"; pPartVariant = "01"; }
	}
	if (iBase == 4)
	{
		// zombie female
		if (iPart == 1) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 2) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 3) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 4) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 5) { pPart = "01"; pPartVariant = "01"; }
	}
	if (iBase > 4)
	{
		// custom
		if (iPart == 1) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 2) { pPart = "01"; pPartVariant = "01"; } 
		if (iPart == 3) { pPart = "01"; pPartVariant = "01"; }
		if (iPart == 4) { pPart = "01"; pPartVariant = "01"; }
	}
	strcpy (pPartNumStr, pPart);
	if ( pPartNumVariantStr ) strcpy (pPartNumVariantStr, pPartVariant);
}

void charactercreatorplus_preloadallcharacterbasedefaults(void)
{
	int iBaseCount = g_CharacterType.size();
	for (int base = 1; base <= iBaseCount; base++)
	{
		LPSTR pBase = NULL;
		char pRelFile[MAX_PATH];
		pBase = g_CharacterType[base-1].pPartsFolder;

		//PE: Only preload from the current selected Type (when changing type we reset release the old textures anyway).
		if( pestrcasestr(CCP_Type,pBase))
		{
			for (int part = 1; part <= 5; part++)
			{
				char pPart[1024];
				strcpy(pPart, "");
				char pPartNum[32];
				charactercreatorplus_GetDefaultCharacterPartNum(base, part, pPartNum);
				if (part == 1) sprintf(pPart, "body %s", pPartNum);
				if (part == 2) sprintf(pPart, "head %s", pPartNum);
				if (part == 3) sprintf(pPart, "legs %s", pPartNum);
				if (part == 4) sprintf(pPart, "feet %s", pPartNum);
				if (part == 5) sprintf(pPart, "hair %s", pPartNum);
				int count_textures_types = 6;
				count_textures_types = 3; //The others are not in wicked.
				for (int item = 1; item <= count_textures_types; item++)
				{
					LPSTR pItem = NULL;
					if (item == 1) pItem = "_color.dds";
					if (item == 2) pItem = "_normal.dds";
					if (item == 3) pItem = "_mask.dds";
					if (item == 4) pItem = "_ao.dds";
					if (item == 5) pItem = "_metalness.dds";
					if (item == 6) pItem = "_gloss.dds";
					sprintf(pRelFile, "charactercreatorplus\\parts\\%s\\%s %s%s", pBase, pBase, pPart, pItem);
					image_preload_files_add(pRelFile);
				}
			}
		}
	}
}

void charactercreatorplus_preloadallcharacterpartchoices ( void )
{
	image_preload_files_start();
	static std::map<std::string, std::string> CharacterCreatorCurrent_s;
	for (int part_loop = 0; part_loop < 10; part_loop++) 
	{
		if (part_loop == 0) CharacterCreatorCurrent_s = CharacterCreatorHeadGear_s;
		if (part_loop == 1) CharacterCreatorCurrent_s = CharacterCreatorHair_s;
		if (part_loop == 2) CharacterCreatorCurrent_s = CharacterCreatorHead_s;
		if (part_loop == 3) CharacterCreatorCurrent_s = CharacterCreatorEyeglasses_s;
		if (part_loop == 4) CharacterCreatorCurrent_s = CharacterCreatorFacialHair_s;
		if (part_loop == 5) CharacterCreatorCurrent_s = CharacterCreatorBody_s;
		if (part_loop == 6) CharacterCreatorCurrent_s = CharacterCreatorLegs_s;
		if (part_loop == 7) CharacterCreatorCurrent_s = CharacterCreatorFeet_s;
		if (part_loop == 8) CharacterCreatorCurrent_s = CharacterCreatorAccessory1_s;
		if (part_loop == 9) CharacterCreatorCurrent_s = CharacterCreatorAccessory2_s;
		
		if (!CharacterCreatorCurrent_s.empty())
		{
			for (std::map<std::string, std::string>::iterator it = CharacterCreatorCurrent_s.begin(); it != CharacterCreatorCurrent_s.end(); ++it)
			{
				std::string full_path = it->second;
				std::string name = it->first;
				char pFullBaseFilename[2048];
				strcpy ( pFullBaseFilename, full_path.c_str() );
				strcat ( pFullBaseFilename, name.c_str());

				// ignore None entries in the list
				if (strnicmp(name.c_str(), "None", 4) != NULL)
				{
					// detect color variant
					char pFullBaseVariantFilename[2048];
					strcpy(pFullBaseVariantFilename, pFullBaseFilename);
					char pLastLetter = pFullBaseFilename[strlen(pFullBaseFilename) - 1];
					if (pLastLetter >= 'a' && pLastLetter <= 'z')
					{
						// using a color variant
						strcpy(pFullBaseVariantFilename, pFullBaseFilename);
						pFullBaseFilename[strlen(pFullBaseFilename) - 1] = 0;
					}
					char pWorkFile[2038];
					strcpy(pWorkFile, pFullBaseFilename); strcat(pWorkFile, CCPMODELEXT);
					object_preload_files_add(pWorkFile);
					strcpy(pWorkFile, pFullBaseFilename); strcat(pWorkFile, "_normal.dds");
					image_preload_files_add(pWorkFile);
					strcpy(pWorkFile, pFullBaseFilename); strcat(pWorkFile, "_mask.dds");
					image_preload_files_add(pWorkFile);
				}
			}
		}
	}
	// also add in any base model defaults
	charactercreatorplus_preloadallcharacterbasedefaults();
	image_preload_files_finish();
}

void charactercreatorplus_preparechange(char *path, int part, char* tag)
{
	g_charactercreatorplus_preloading = true;
	strcpy(g_charactercreatorplus_path, path);
	g_charactercreatorplus_part = part;
	strcpy(g_charactercreatorplus_tag, tag);
	DisplaySmallImGuiMessage("Loading ...");
}

void charactercreatorplus_waitforpreptofinish(void)
{
	if (g_charactercreatorplus_preloading == true)
	{
		DisplaySmallImGuiMessage("Loading ...");
	}
	if (g_charactercreatorplus_preloading == true)
	{
		if (image_preload_files_in_progress()==false && object_preload_files_in_progress()==false)
		{
			image_preload_files_wait();
			image_preload_files_reset(); //PE: Make sure to free textures, as the list will get cleared later.
			object_preload_files_wait();
			charactercreatorplus_change(g_charactercreatorplus_path, g_charactercreatorplus_part, g_charactercreatorplus_tag);
			charactercreatorplus_preloadallcharacterpartchoices();
			g_charactercreatorplus_preloading = false;
		}
	}
}

void charactercreatorplus_refreshskincolor(void)
{
	// wicked not using custom character shader, so create a new albedo/color texture
	// using template mask and skin color texture to produce correct albedo representing skin
	// only needed to do for body, legs and feet
	int iCharTexture = g.charactercreatorEditorImageoffset + 1;
	int iCharLegsTexture = g.charactercreatorEditorImageoffset + 61;
	int iCharFeetTexture = g.charactercreatorEditorImageoffset + 71;
	int iCharSkinTexture = g.charactercreatorEditorImageoffset + 101;
	
	// find a free work memblock
	int iMemblockAlbedoID = 32; while (MemblockExist(iMemblockAlbedoID) == 1) iMemblockAlbedoID++;
	int iMemblockMaskID = 33; while (MemblockExist(iMemblockMaskID) == 1) iMemblockMaskID++;
	int iMemblockSkinID = 34; while (MemblockExist(iMemblockSkinID) == 1) iMemblockSkinID++;

	// for each relevant body part
	for ( int partnum = 0; partnum < 3; partnum++ )
	{
		// determine which body part to work on
		int iPartObj, iAlbedoTexture;
		if (partnum == 0) { iPartObj = iCharObj; iAlbedoTexture = iCharTexture + 0; }
		if (partnum == 1) { iPartObj = iCharObjLegs; iAlbedoTexture = iCharLegsTexture + 0; }
		if (partnum == 2) { iPartObj = iCharObjFeet; iAlbedoTexture = iCharFeetTexture + 0; }
		int iMaskTexture;
		if (partnum == 0) { iMaskTexture = iCharTexture + 1; }
		if (partnum == 1) { iMaskTexture = iCharLegsTexture + 1; }
		if (partnum == 2) { iMaskTexture = iCharFeetTexture + 1; }

		if (!ImageExist(iAlbedoTexture))
		{
			printf("tmp");
		}

		// ensure mask available
		if ( ImageExist(iMaskTexture))
		{
			// make sure images are not compressed (i.e. DXT1-5), but regular XRGB so we can read them
			char pNewTempAlbedoTextureFile[MAX_PATH];
			sprintf(pNewTempAlbedoTextureFile, "charactercreatorplus\\skins\\tempfinalalbedo.png");
			GG_GetRealPath(pNewTempAlbedoTextureFile, 1);

			//PE: No need if already in DXGI_FORMAT_R8G8B8A8_UNORM
			int imgformat = ImageFormat(iAlbedoTexture);
			if (imgformat != DXGIFORMATR8G8B8A8UNORM)
			{
				SaveImage(pNewTempAlbedoTextureFile, iAlbedoTexture);
				DeleteImage(iAlbedoTexture);
				LoadImage(pNewTempAlbedoTextureFile, iAlbedoTexture);
			}
			char pNewTempMaskTextureFile[MAX_PATH];
			sprintf(pNewTempMaskTextureFile, "charactercreatorplus\\skins\\tempfinalmask.png");
			GG_GetRealPath(pNewTempMaskTextureFile, 1);
			
			//PE: No need if already in DXGI_FORMAT_R8G8B8A8_UNORM
			imgformat = ImageFormat(iMaskTexture);
			if (imgformat != DXGIFORMATR8G8B8A8UNORM)
			{
				SaveImage(pNewTempMaskTextureFile, iMaskTexture);
				DeleteImage(iMaskTexture);
				LoadImage(pNewTempMaskTextureFile, iMaskTexture);
			}

			// load image data into appropriate memblocks
			if (MemblockExist(iMemblockAlbedoID) == 1) DeleteMemblock(iMemblockAlbedoID);
			if (MemblockExist(iMemblockMaskID) == 1) DeleteMemblock(iMemblockMaskID);
			if (MemblockExist(iMemblockSkinID) == 1) DeleteMemblock(iMemblockSkinID);
			CreateMemblockFromImage(iMemblockAlbedoID, iAlbedoTexture);
			CreateMemblockFromImage(iMemblockMaskID, iMaskTexture);
			CreateMemblockFromImage(iMemblockSkinID, iCharSkinTexture);

			if (MemblockExist(iMemblockAlbedoID) == 0)
			{
				//PE: Fail.
				int imgformat = ImageFormat(iAlbedoTexture);
				if (!ImageExist(iAlbedoTexture))
				{
					printf("tmp");
				}
			}
			// skin mask may have different resolution
			int imgSkinMaskWidth = ReadMemblockDWord(iMemblockMaskID, 0);
			int imgSkinMaskHeight = ReadMemblockDWord(iMemblockMaskID, 4);
			int imgSkinMaskDepth = ReadMemblockDWord(iMemblockMaskID, 8);
			int imgSkinMaskSize = imgSkinMaskWidth * imgSkinMaskHeight * imgSkinMaskDepth;
			int imgSkinMaskOffset = 4 * 3;

			// and skin ref may be different size also (usually 2K)
			int imgSkinWidth = ReadMemblockDWord(iMemblockSkinID, 0);
			int imgSkinHeight = ReadMemblockDWord(iMemblockSkinID, 4);
			int imgSkinDepth = ReadMemblockDWord(iMemblockSkinID, 8);
			int imgSkinSize = imgSkinWidth * imgSkinHeight * imgSkinDepth;
			int imgSkinOffset = 4 * 3;

			// create new albedo from mask and skin texture
			int imgWidth = ReadMemblockDWord(iMemblockAlbedoID, 0);
			int imgHeight = ReadMemblockDWord(iMemblockAlbedoID, 4);
			int imgDepth = ReadMemblockDWord(iMemblockAlbedoID, 8);
			int imgSize = imgWidth * imgHeight * imgDepth;
			int imgOffset = 4 * 3;

			// used to calculate actual skinmask coords offset
			float imgSkinMaskXDiv = (float)imgSkinMaskWidth / (float)imgWidth;
			float imgSkinMaskYDiv = (float)imgSkinMaskHeight / (float)imgHeight;
			float imgSkinXDiv = (float)imgSkinWidth / (float)imgWidth;
			float imgSkinYDiv = (float)imgSkinHeight / (float)imgHeight;

			// replace all skin pixels using mask
			for (int y = 0; y < imgHeight - 1; y++)
			{
				for (int x = 0; x < imgWidth - 1; x++)
				{
					// skin mask can be different size than color texture (i.e. 1K mask + 4K color texture)
					if (imgSize != imgSkinMaskSize)
					{
						int iXOffset = x*imgSkinMaskXDiv;
						int iYOffset = y*imgSkinMaskYDiv;
						int iXYOffset = iXOffset + (iYOffset*(imgSkinMaskWidth-1));
						imgSkinMaskOffset = (4 * 3) + (iXYOffset *4);
					}
					else
					{
						imgSkinMaskOffset = imgOffset;
					}
					if (imgSize != imgSkinSize)
					{
						int iXOffset = x * imgSkinXDiv;
						int iYOffset = y * imgSkinYDiv;
						int iXYOffset = iXOffset + (iYOffset*(imgSkinWidth-1));
						imgSkinOffset = (4 * 3) + (iXYOffset * 4);
					}
					else
					{
						imgSkinOffset = imgOffset;
					}
					int pixelCol = ReadMemblockDWord(iMemblockAlbedoID, imgOffset);
					int maskColR = ReadMemblockByte(iMemblockMaskID, imgSkinMaskOffset+0);
					int maskColG = ReadMemblockByte(iMemblockMaskID, imgSkinMaskOffset+1);
					int maskColB = ReadMemblockByte(iMemblockMaskID, imgSkinMaskOffset+2);
					int maskColA = ReadMemblockByte(iMemblockMaskID, imgSkinMaskOffset+3);
					int skinCol = ReadMemblockDWord(iMemblockSkinID, imgSkinOffset);
					if (maskColR != 255 || maskColG != 255 || maskColB != 255 || maskColA != 255)
					{
						// for some reason detecting red at zero did not work
						// need some more time to find out why the mask memblock returning strange values
						// i.e. 255,255,247,255 for the non-red part (255,255,255,255 for the rest)
					}
					else
					{
						// if mask red channel full, allow skin pixel to bake into main character texture
						int iA = ((DWORD)skinCol >> 24) & 0xFF;
						int iB = ((DWORD)skinCol >> 16) & 0xFF;
						int iG = ((DWORD)skinCol >> 8) & 0xFF;
						int iR = ((DWORD)skinCol >> 0) & 0xFF;
						//PE: The problem was CreateImageFromMemblock+SaveImage would switch the colors each time it was called :)
						DWORD newSkinCol = (iA << 24) + (iB << 16) + (iG << 8) + iR; // 
						WriteMemblockDWord(iMemblockAlbedoID, imgOffset, newSkinCol);
					}
					imgOffset = imgOffset + 4;
				}
			}
			
			// delete temp files
			DeleteFileA(pNewTempAlbedoTextureFile);
			DeleteFileA(pNewTempMaskTextureFile);

			// save the file (so can be loaded by wicked later)
			sprintf(pNewTempAlbedoTextureFile, "charactercreatorplus\\skins\\tempfinalalbedo%d.dds", partnum);
			GG_GetRealPath(pNewTempAlbedoTextureFile, 1);
			DeleteImage(iAlbedoTexture);

			extern bool g_bUseRGBAFormat;
			g_bUseRGBAFormat = true;
			CreateImageFromMemblock(iAlbedoTexture, iMemblockAlbedoID);

			SetImageName(iAlbedoTexture, pNewTempAlbedoTextureFile);

			//PE: CreateImageFromMemblock always convert to DXGI_FORMAT_B8G8R8A8_UNORM.
			//PE: We are now in DXGI_FORMAT_B8G8R8A8_UNORM. CreateImageFromMemblock just do a direct copy without color swap.
			//PE: Problem: Now that we are in DXGI_FORMAT_B8G8R8A8_UNORM and want to save as DXGI_FORMAT_R8G8B8A8_UNORM
			//PE: SaveImage will swap around the colors, thats a problem. each time this is called colors are swapped.
			//PE: Use g_bUseRGBAFormat=true to prevent this.
			g_bUseRGBAFormat = false;

			SaveImage(pNewTempAlbedoTextureFile, iAlbedoTexture);

			// rare event where a texture file can be different things , so ensure
			// the image manager deletes the image entry for this image before a new attempt 
			// to load it happens
			WickedCall_DeleteImage(pNewTempAlbedoTextureFile);

			// load the file and apply to the object
			TextureObject(iPartObj, 0, iAlbedoTexture);
		}
	}
	if (MemblockExist(iMemblockAlbedoID) == 1) DeleteMemblock(iMemblockAlbedoID);
	if (MemblockExist(iMemblockMaskID) == 1) DeleteMemblock(iMemblockMaskID);
	if (MemblockExist(iMemblockSkinID) == 1) DeleteMemblock(iMemblockSkinID);
}

void charactercreatorplus_loadccimages(LPSTR pVariantColorPartFile, LPSTR pPartFile, int iTextureBase)
{
	// variant color can be different from rest of texture files (03a, 03b, etc)
	cstr pVariantColorPath = pVariantColorPartFile;
	cstr pPath = pPartFile;

	// Load needed albedo and normal textures, and emissive if present
	LoadImage(cstr(pVariantColorPath + "_color.dds").Get(), iTextureBase + 0);
	LoadImage(cstr(pPath + "_mask.dds").Get(), iTextureBase + 1); // was +5 in VRQV3 (now taken by emissive below)
	LoadImage(cstr(pPath + "_normal.dds").Get(), iTextureBase + 2);
	LoadImage(cstr(pPath + "_emissive.dds").Get(), iTextureBase + 5);

	// LB: Generate required surface texture file if not exist
	cstr pAOFile = pPath + "_ao.dds";
	cstr pGlossFile = pPath + "_gloss.dds";
	cstr pMetalessFile = pPath + "_metalness.dds";
	cstr pSurfaceFile = pPath + "_surface.dds";
	if (FileExist(pSurfaceFile.Get()) == 0)
	{
		// need to generate surface file, and then load it
		if (FileExist(pAOFile.Get()) == 0) pAOFile = "effectbank\\reloaded\\media\\blank_O.dds";
		ImageCreateSurfaceTexture(pSurfaceFile.Get(), pAOFile.Get(), pGlossFile.Get(), pMetalessFile.Get());
	}
	LoadImage(pSurfaceFile.Get(), iTextureBase + 6);
}

void charactercreatorplus_textureccimages(int iThisObj, int iTextureBase)
{
	// LB: Paul's new arrangement replaces old layout
	TextureObject(iThisObj, GG_MESH_TEXTURE_SURFACE, iTextureBase + 6);
	TextureObject(iThisObj, GG_MESH_TEXTURE_DIFFUSE, iTextureBase + 0);
	TextureObject(iThisObj, GG_MESH_TEXTURE_NORMAL, iTextureBase + 2);
	TextureObject(iThisObj, GG_MESH_TEXTURE_EMISSIVE, iTextureBase + 5);
}

void charactercreatorplus_change(char *path, int part, char* tag)
{
	// need legacy loading for image reskinning work
	image_setlegacyimageloading(true);

	// part = 67 is a special code to update both legs and feet (for cascade system when body/legs force other part changes)
	int iCharTexture = g.charactercreatorEditorImageoffset + 1;
	int iCharHeadGearTexture = g.charactercreatorEditorImageoffset + 11;
	int iCharHairTexture = g.charactercreatorEditorImageoffset + 21;
	int iCharHeadTexture = g.charactercreatorEditorImageoffset + 31;
	int iCharEyeglassesTexture = g.charactercreatorEditorImageoffset + 41;
	int iCharFacialHairTexture = g.charactercreatorEditorImageoffset + 51;
	int iCharLegsTexture = g.charactercreatorEditorImageoffset + 61;
	int iCharFeetTexture = g.charactercreatorEditorImageoffset + 71;
	int iCharAccessory1Texture = g.charactercreatorEditorImageoffset + 81;
	int iCharAccessory2Texture = g.charactercreatorEditorImageoffset + 91;

	// skin override texture
	int iCharSkinTexture = g.charactercreatorEditorImageoffset + 101;

	// free main character object to recreate here
	if (ObjectExist(iCharObj)) DeleteObject(iCharObj);

	// final part name
	cstr final_name = path, tmp;

	// Make sure that there is always a valid skin texture for auto-swapping.
	if (strlen(g_SkinTextureStorage) == 0)
		strcpy(g_SkinTextureStorage, cstr(final_name + cSelectedHead).Get());

	char pPreviousHeadChoice[260];

	if (part == 0 || part == 2)
	{
		if (part == 0)
		{
			// User chose new headgear, so revert any swapped parts back to their original state.
			if (g_previousAutoSwap)
				charactercreatorplus_restoreswappedparts();
		}

		g_pLastHeadgearAutoSwap = nullptr;
	}

	// Check for any parts that would clip through the newly chosen part, and swap as needed.
	charactercreatorplus_performautoswap(part);

	if (g_previousAutoSwap != nullptr)
	{
		// Check if the part that is being changed was part of an auto-swap, or chosen specifically by the user.
		int iUserChoseCategory = -1;
		for (int i = 0; i < 10; i++)
		{
			// Find the category in the previous auto-swap data that needs updating.
			if (part == i && g_iPartsThatNeedReloaded[i] == 0)
			{
				iUserChoseCategory = i;
				break;
			}
		}

		// If the user chose this part specifically, ensure that the previous auto-swap data is updated to reflect this...
		// ...This is done because every time a new auto-swap is performed, the previous swap is undone, to ensure the user does not lose the choice they originally made.
		if (iUserChoseCategory >= 0)
		{
			char* pPartToSwap = nullptr;
			for (int i = 0; i < g_previousAutoSwap->requiredSwapCategories.size(); i++)
			{
				if (g_previousAutoSwap->requiredSwapCategories[i] == iUserChoseCategory)
				{
					// Get the correct part type to swap.
					switch (g_previousAutoSwap->requiredSwapCategories[i])
					{
					case 0: pPartToSwap = cSelectedHeadGear; break;
					case 1: pPartToSwap = cSelectedHair; break;
					case 2: pPartToSwap = cSelectedHead; break;
					case 3: pPartToSwap = cSelectedEyeglasses; break;
					case 4: pPartToSwap = cSelectedFacialHair; break;
					case 5: pPartToSwap = cSelectedBody; break;
					case 6: pPartToSwap = cSelectedLegs; break;
					case 7: pPartToSwap = cSelectedFeet; break;
					case 8: pPartToSwap = cSelectedAccessory1; break;
					case 9: pPartToSwap = cSelectedAccessory2; break;
					}
					g_previousAutoSwap->swappedPartNames[i] = pPartToSwap;
					break;
				}
			}

			// User chose a new head, if there is an active headgear auto-swap, then reswap the head if needed.
			if (iUserChoseCategory > 0 && iUserChoseCategory < 5)
			{
				for (int i = 0; i < g_previousAutoSwap->requiredSwapCategories.size(); i++)
				{
					if (g_previousAutoSwap->requiredSwapCategories[i] == 2)
					{
						// The auto-swap data contained a head, so ensure we re-auto-swap if possible.
						for (const auto& annotation : CharacterCreatorAnnotatedHead_s)
						{
							if (strstr(annotation.second.c_str(), g_previousAutoSwap->requiredSwapNames[i].c_str()))
							{
								strcpy(pPreviousHeadChoice, cSelectedHead);
								strcpy(cSelectedHead, annotation.first.c_str());
								g_pLastHeadgearAutoSwap = g_previousAutoSwap;
							}
						}
						g_iPartsThatNeedReloaded[2] = 1;
						break;
					}
				}
			}
		}
	}

	// detect color variant
	int iEnd = 0;
	char cLast = 0;
	char cUseHeadGear[260];
	char cUseHair[260];
	char cUseHead[260];
	char cUseEyeglasses[260];
	char cUseFacialHair[260];
	char cUseBody[260];
	char cUseLegs[260];
	char cUseFeet[260];
	char cUseAccessory1[260];
	char cUseAccessory2[260];
	strcpy(cUseHeadGear, cSelectedHeadGear);
	strcpy(cUseHair, cSelectedHair);
	strcpy(cUseHead, cSelectedHead);
	strcpy(cUseEyeglasses, cSelectedEyeglasses);
	strcpy(cUseFacialHair, cSelectedFacialHair);
	strcpy(cUseBody, cSelectedBody);
	strcpy(cUseLegs, cSelectedLegs);
	strcpy(cUseFeet, cSelectedFeet);
	strcpy(cUseAccessory1, cSelectedAccessory1);
	strcpy(cUseAccessory2, cSelectedAccessory2);

	char cSelectedVariantHeadGear[260];
	char cSelectedVariantHair[260];
	char cSelectedVariantHead[260];
	char cSelectedVariantEyeglasses[260];
	char cSelectedVariantFacialHair[260];
	char cSelectedVariantBody[260];
	char cSelectedVariantLegs[260];
	char cSelectedVariantFeet[260];
	char cSelectedVariantAccessory1[260];
	char cSelectedVariantAccessory2[260];

	strcpy(cSelectedVariantHeadGear, cSelectedHeadGear);
	strcpy(cSelectedVariantHair, cSelectedHair);
	strcpy(cSelectedVariantHead, cSelectedHead);
	strcpy(cSelectedVariantEyeglasses, cSelectedEyeglasses);
	strcpy(cSelectedVariantFacialHair, cSelectedFacialHair);
	strcpy(cSelectedVariantBody, cSelectedBody);
	strcpy(cSelectedVariantLegs, cSelectedLegs);
	strcpy(cSelectedVariantFeet, cSelectedFeet);
	strcpy(cSelectedVariantAccessory1, cSelectedAccessory1);
	strcpy(cSelectedVariantAccessory2, cSelectedAccessory2);

	iEnd = strlen(cSelectedHeadGear) - 1; cLast = cSelectedHeadGear[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseHeadGear[iEnd] = 0; }
	iEnd = strlen(cSelectedHair) - 1; cLast = cSelectedHair[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseHair[iEnd] = 0; }
	iEnd = strlen(cSelectedHead) - 1; cLast = cSelectedHead[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseHead[iEnd] = 0; }
	iEnd = strlen(cSelectedEyeglasses) - 1; cLast = cSelectedEyeglasses[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseEyeglasses[iEnd] = 0; }
	iEnd = strlen(cSelectedFacialHair) - 1; cLast = cSelectedFacialHair[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseFacialHair[iEnd] = 0; }
	iEnd = strlen(cSelectedBody) - 1; cLast = cSelectedBody[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseBody[iEnd] = 0; }
	iEnd = strlen(cSelectedLegs) - 1; cLast = cSelectedLegs[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseLegs[iEnd] = 0; }
	iEnd = strlen(cSelectedFeet) - 1; cLast = cSelectedFeet[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseFeet[iEnd] = 0; }
	iEnd = strlen(cSelectedAccessory1) - 1; cLast = cSelectedAccessory1[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseAccessory1[iEnd] = 0; }
	iEnd = strlen(cSelectedAccessory2) - 1; cLast = cSelectedAccessory2[iEnd]; if (iEnd > 3 && cLast >= 'a' && cLast <= 'z') { cUseAccessory2[iEnd] = 0; }

	//Load all objects.
	if (strnicmp(cUseHeadGear, "None", 4) != NULL)
	{
		tmp = final_name + cUseHeadGear + CCPMODELEXT;
		LoadObject(tmp.Get(), iCharObjHeadGear);

		// Make the balaclava double-sided to prevent users seeing gaps at the eyes.
		if (strcmp(cSelectedHeadGear, "adult male headgear 15") == NULL)
		{
			sObject* pObject = GetObjectData(iCharObjHeadGear);
			if (pObject) pObject->ppMeshList[0]->bCull = false;
		}
	}
	else
	{
		if (ObjectExist(iCharObjHeadGear)) DeleteObject(iCharObjHeadGear);
	}
	if (strnicmp(cUseHair, "None", 4) != NULL)
	{
		tmp = final_name + cUseHair + CCPMODELEXT;
		LoadObject(tmp.Get(), iCharObjHair);
	}
	else
	{
		if (ObjectExist(iCharObjHair)) DeleteObject(iCharObjHair);
	}
	tmp = final_name + cUseHead + CCPMODELEXT;
	LoadObject(tmp.Get(), iCharObjHead);

	if (strnicmp(cUseEyeglasses, "None", 4) != NULL)
	{
		tmp = final_name + cUseEyeglasses + CCPMODELEXT;
		LoadObject(tmp.Get(), iCharObjEyeglasses);
	}
	else
	{
		if (ObjectExist(iCharObjEyeglasses)) DeleteObject(iCharObjEyeglasses);
	}

	if (strnicmp(cUseAccessory1, "None", 4) != NULL)
	{
		tmp = final_name + cUseAccessory1 + CCPMODELEXT;
		LoadObject(tmp.Get(), iCharObjAccessory1);
	}
	else
	{
		if (ObjectExist(iCharObjAccessory1)) DeleteObject(iCharObjAccessory1);
	}
	if (strnicmp(cUseAccessory2, "None", 4) != NULL)
	{
		tmp = final_name + cUseAccessory2 + CCPMODELEXT;
		LoadObject(tmp.Get(), iCharObjAccessory2);
	}
	else
	{
		if (ObjectExist(iCharObjAccessory2)) DeleteObject(iCharObjAccessory2);
	}

	if (strnicmp(cUseFacialHair, "None", 4) != NULL)
	{
		tmp = final_name + cUseFacialHair + CCPMODELEXT;
		LoadObject(tmp.Get(), iCharObjFacialHair);
	}
	else
	{
		if (ObjectExist(iCharObjFacialHair)) DeleteObject(iCharObjFacialHair);
	}

	// BODY
	tmp = final_name + cUseBody + CCPMODELEXT;
	LoadObject(tmp.Get(), iCharObj);

	// LEGS
	tmp = final_name + cUseLegs + CCPMODELEXT;
	LoadObject(tmp.Get(), iCharObjLegs);

	// FEET
	tmp = final_name + cUseFeet + CCPMODELEXT;
	LoadObject(tmp.Get(), iCharObjFeet);

	// load all required textures
	if ((part == 0 || g_iPartsThatNeedReloaded[0] == 1 || part == -1) && ObjectExist(iCharObjHeadGear) == 1)
	{
		// HeadGear
		for (int a = 0; a < 5; a++) if (GetImageExistEx(iCharHeadGearTexture + a)) DeleteImage(iCharHeadGearTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantHeadGear;
		cstr Path_s = final_name + cUseHeadGear;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharHeadGearTexture);
	}
	if ((part == 1 || g_iPartsThatNeedReloaded[1] == 1 || part == -1) && ObjectExist(iCharObjHair) == 1)
	{
		// Hair
		for (int a = 0; a < 5; a++) if (GetImageExistEx(iCharHairTexture + a)) DeleteImage(iCharHairTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantHair;
		cstr Path_s = final_name + cUseHair;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharHairTexture);
	}
	if ((part == 2 || g_iPartsThatNeedReloaded[2] == 1 || part == -1) && ObjectExist(iCharObjHead) == 1)
	{
		// Head
		for (int a = 0; a < 5; a++) if (GetImageExistEx(iCharHeadTexture + a)) DeleteImage(iCharHeadTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantHead;
		cstr Path_s = final_name + cUseHead;

		// Head is being changed as part of an auto-swap, so the skin tone must be matched to whatever it was before.
		if (part != 2 && g_iPartsThatNeedReloaded[2] == 1)
			strcpy(VariantColorPath_s.Get(), g_SkinTextureStorage);
		else if (part == 2 && g_iPartsThatNeedReloaded[2] == 1)
		{
			// The head was chosen specifically by the user, but currently a headgear that requires an auto swap is equipped.
			VariantColorPath_s = cstr(final_name + pPreviousHeadChoice);
			strcpy(g_SkinTextureStorage, VariantColorPath_s.Get());
		}
		else
			strcpy(g_SkinTextureStorage, VariantColorPath_s.Get());

		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharHeadTexture);

		// in addition, load in skin type to apply to rest of body
		if (tag && strnicmp(tag, "IC", 2) == NULL)
		{
			if (GetImageExistEx(iCharSkinTexture) == 1) DeleteImage(iCharSkinTexture);
			char pRelPathToSkinTexture[MAX_PATH];
			sprintf(pRelPathToSkinTexture, "charactercreatorplus\\skins\\%s.png", tag);
			LoadImage(pRelPathToSkinTexture, iCharSkinTexture);
		}
	}
	if ((part == 3 || g_iPartsThatNeedReloaded[3] == 1 || part == -1) && ObjectExist(iCharObjEyeglasses) == 1)
	{
		// Eyeglasses
		for (int a = 0; a < 5; a++) if (GetImageExistEx(iCharEyeglassesTexture + a)) DeleteImage(iCharEyeglassesTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantEyeglasses;
		cstr Path_s = final_name + cUseEyeglasses;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharEyeglassesTexture);
	}

	if ((part == 8 || g_iPartsThatNeedReloaded[8] == 1 || part == -1) && ObjectExist(iCharObjAccessory1) == 1)
	{
		// Accessory1
		for (int a = 0; a < 5; a++) if (GetImageExistEx(iCharAccessory1Texture + a)) DeleteImage(iCharAccessory1Texture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantAccessory1;
		cstr Path_s = final_name + cUseAccessory1;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharAccessory1Texture);
	}
	if ((part == 9 || g_iPartsThatNeedReloaded[9] == 1 || part == -1) && ObjectExist(iCharObjAccessory2) == 1)
	{
		// Accessory1
		for (int a = 0; a < 5; a++) if (GetImageExistEx(iCharAccessory2Texture + a)) DeleteImage(iCharAccessory2Texture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantAccessory2;
		cstr Path_s = final_name + cUseAccessory2;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharAccessory2Texture);
	}

	if ((part == 4 || g_iPartsThatNeedReloaded[4] == 1 || part == -1) && ObjectExist(iCharObjFacialHair) == 1)
	{
		// Facial Hair
		for (int a = 0; a < 5; a++)	if (GetImageExistEx(iCharFacialHairTexture + a)) DeleteImage(iCharFacialHairTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantFacialHair;
		cstr Path_s = final_name + cUseFacialHair;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharFacialHairTexture);
	}
	if ((part == 5 || g_iPartsThatNeedReloaded[5] == 1 || part == -1) && ObjectExist(iCharObj) == 1)
	{
		// Body
		for (int a = 0; a < 6; a++) if (GetImageExistEx(iCharTexture + a)) DeleteImage(iCharTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantBody;
		cstr Path_s = final_name + cUseBody;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharTexture);

		// and need mask for reskin
		tmp = final_name + cUseBody + "_mask.dds";
		LoadImage(tmp.Get(), iCharTexture + 1);
		if (ImageExist(iCharTexture + 1) == 0)
		{
			// remove color-specific from part and look for base mask for this body part
			char pCropPartFile[MAX_PATH];
			strcpy(pCropPartFile, cUseBody);
			LPSTR pLastChar = pCropPartFile + strlen(pCropPartFile) - 1;
			if (*pLastChar >= 'a' && *pLastChar <= 'z') *pLastChar = 0;
			LoadImage(cstr(cstr(final_name) + pCropPartFile + "_mask.dds").Get(), iCharTexture + 1);
		}
	}
	if ((part == 6 || g_iPartsThatNeedReloaded[6] == 1 || part == -1 || part == 67) && ObjectExist(iCharObjLegs) == 1)
	{
		// Legs
		for (int a = 0; a < 6; a++) if (GetImageExistEx(iCharLegsTexture + a)) DeleteImage(iCharLegsTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantLegs;
		cstr Path_s = final_name + cUseLegs;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharLegsTexture);

		// and need mask for reskin
		tmp = final_name + cUseLegs + "_mask.dds";
		LoadImage(tmp.Get(), iCharLegsTexture + 1);
		if (ImageExist(iCharLegsTexture + 1) == 0)
		{
			// remove color-specific from part and look for base mask for this body part
			char pCropPartFile[MAX_PATH];
			strcpy(pCropPartFile, cUseLegs);
			LPSTR pLastChar = pCropPartFile + strlen(pCropPartFile) - 1;
			if (*pLastChar >= 'a' && *pLastChar <= 'z') *pLastChar = 0;
			LoadImage(cstr(cstr(final_name) + pCropPartFile + "_mask.dds").Get(), iCharLegsTexture + 1);
		}
	}
	if ((part == 7 || g_iPartsThatNeedReloaded[7] == 1 || part == -1 || part == 67) && ObjectExist(iCharObjFeet) == 1)
	{
		// Feet
		for (int a = 0; a < 6; a++) if (GetImageExistEx(iCharFeetTexture + a)) DeleteImage(iCharFeetTexture + a);

		// load textures
		cstr VariantColorPath_s = final_name + cSelectedVariantFeet;
		cstr Path_s = final_name + cUseFeet;
		charactercreatorplus_loadccimages(VariantColorPath_s.Get(), Path_s.Get(), iCharFeetTexture);

		// and need mask for reskin
		tmp = final_name + cUseFeet + "_mask.dds";
		LoadImage(tmp.Get(), iCharFeetTexture + 1);
		if (ImageExist(iCharFeetTexture + 1) == 0)
		{
			// remove color-specific from part and look for base mask for this body part
			char pCropPartFile[MAX_PATH];
			strcpy(pCropPartFile, cUseFeet);
			LPSTR pLastChar = pCropPartFile + strlen(pCropPartFile) - 1;
			if (*pLastChar >= 'a' && *pLastChar <= 'z') *pLastChar = 0;
			LoadImage(cstr(cstr(final_name) + pCropPartFile + "_mask.dds").Get(), iCharFeetTexture + 1);
		}
	}

	// texture parts
	if (ObjectExist(iCharObjHeadGear) == 1) charactercreatorplus_textureccimages(iCharObjHeadGear, iCharHeadGearTexture);
	if (ObjectExist(iCharObjHair) == 1)	charactercreatorplus_textureccimages(iCharObjHair, iCharHairTexture);
	charactercreatorplus_textureccimages(iCharObjHead, iCharHeadTexture);
	if (ObjectExist(iCharObjEyeglasses) == 1) charactercreatorplus_textureccimages(iCharObjEyeglasses, iCharEyeglassesTexture);
	if (ObjectExist(iCharObjAccessory1) == 1) charactercreatorplus_textureccimages(iCharObjAccessory1, iCharAccessory1Texture);
	if (ObjectExist(iCharObjAccessory2) == 1) charactercreatorplus_textureccimages(iCharObjAccessory2, iCharAccessory2Texture);
	if (ObjectExist(iCharObjFacialHair) == 1) charactercreatorplus_textureccimages(iCharObjFacialHair, iCharFacialHairTexture);
	charactercreatorplus_textureccimages(iCharObj, iCharTexture);
	charactercreatorplus_textureccimages(iCharObjLegs, iCharLegsTexture);
	charactercreatorplus_textureccimages(iCharObjFeet, iCharFeetTexture);

	// wicked not using custom character shader, so create a new albedo/color texture
	// using template mask and skin color texture to produce correct albedo representing skin
	// only needed to do for body, legs and feet
	if (stricmp(CCP_Type, "zombie male") != NULL && stricmp(CCP_Type, "zombie female") != NULL)
	{
		charactercreatorplus_refreshskincolor();
	}

	// remove wicked resources before modifying objects below
	if (ObjectExist(iCharObj)) WickedCall_RemoveObject(GetObjectData(iCharObj));
	if (ObjectExist(iCharObjHeadGear)) WickedCall_RemoveObject(GetObjectData(iCharObjHeadGear));
	if (ObjectExist(iCharObjHead)) WickedCall_RemoveObject(GetObjectData(iCharObjHead));
	if (ObjectExist(iCharObjLegs)) WickedCall_RemoveObject(GetObjectData(iCharObjLegs));
	if (ObjectExist(iCharObjFeet)) WickedCall_RemoveObject(GetObjectData(iCharObjFeet));
	if (ObjectExist(iCharObjHair)) WickedCall_RemoveObject(GetObjectData(iCharObjHair));
	if (ObjectExist(iCharObjEyeglasses)) WickedCall_RemoveObject(GetObjectData(iCharObjEyeglasses));
	if (ObjectExist(iCharObjFacialHair)) WickedCall_RemoveObject(GetObjectData(iCharObjFacialHair));
	if (ObjectExist(iCharObjAccessory1)) WickedCall_RemoveObject(GetObjectData(iCharObjAccessory1));
	if (ObjectExist(iCharObjAccessory2)) WickedCall_RemoveObject(GetObjectData(iCharObjAccessory2));

	// stitch model together
	if (ObjectExist(iCharObjHeadGear) == 1)
	{
		// pregvents ugly shadow - can remove this when increase shadow quality on character renderings!
		SetObjectTransparency(iCharObjHeadGear, 2);
		StealMeshesFromObject(iCharObj, iCharObjHeadGear);
		DeleteObject(iCharObjHeadGear);
	}
	StealMeshesFromObject(iCharObj, iCharObjHead);
	DeleteObject(iCharObjHead);
	StealMeshesFromObject(iCharObj, iCharObjLegs);
	DeleteObject(iCharObjLegs);
	StealMeshesFromObject(iCharObj, iCharObjFeet);
	DeleteObject(iCharObjFeet);
	if (ObjectExist(iCharObjHair) == 1)
	{
		// ensure hair has no culling and semi-transparent
		SetObjectTransparency(iCharObjHair, 2);
		SetObjectCull(iCharObjHair, 0);
		StealMeshesFromObject(iCharObj, iCharObjHair);
		DeleteObject(iCharObjHair);
	}
	if (ObjectExist(iCharObjEyeglasses) == 1)
	{
		SetObjectCull(iCharObjEyeglasses, 0);
		SetObjectTransparency(iCharObjEyeglasses, 2);
		StealMeshesFromObject(iCharObj, iCharObjEyeglasses);
		DeleteObject(iCharObjEyeglasses);
	}

	if (ObjectExist(iCharObjAccessory1) == 1)
	{
		//SetObjectCull(iCharObjAccessory1, 0);
		//SetObjectTransparency(iCharObjAccessory1, 2);
		StealMeshesFromObject(iCharObj, iCharObjAccessory1);
		DeleteObject(iCharObjAccessory1);
	}
	if (ObjectExist(iCharObjAccessory2) == 1)
	{
		//SetObjectCull(iCharObjAccessory2, 0);
		//SetObjectTransparency(iCharObjAccessory2, 2);
		StealMeshesFromObject(iCharObj, iCharObjAccessory2);
		DeleteObject(iCharObjAccessory2);
	}

	if (ObjectExist(iCharObjFacialHair) == 1)
	{
		SetObjectTransparency(iCharObjFacialHair, 2);
		SetObjectCull(iCharObjFacialHair, 0);
		StealMeshesFromObject(iCharObj, iCharObjFacialHair);
		DeleteObject(iCharObjFacialHair);
	}

	// as character parts have no animations, wipe out ones they do have
	// and replace with the latest animation set for this base mesh (pistol is default)

	tmp = final_name + "default animations" + CCPMODELEXT;
	
	//PE: Was moved from "charactercreatorplus\parts\adult male\default animations.dbo" to "charactercreatorplus\\animations\\sets\\%s\\default animations.dbo"
	char pPathToWeaponAnim[MAX_PATH];
	sprintf(pPathToWeaponAnim, "charactercreatorplus\\animations\\sets\\%s\\default animations.dbo", CCP_Type);
	if (FileExist(pPathToWeaponAnim) == 1)
	{
		AppendObject(pPathToWeaponAnim, iCharObj, 0);
	}
	else if (FileExist(tmp.Get()))
	{
		AppendObject(tmp.Get(), iCharObj, 0);
	}
	// and trigger animation to be prepped
	g_bCharacterCreatorPrepAnims = true;

	// some DBOs are created with BLACK base color, so for character creator
	// objects set them always to WHITE with full ALPHA
	SetObjectDiffuseEx(iCharObj, 0xFFFFFFFF, 0);

	// this sequence is duplicated during the init, see if can merge into single function at some point!
	// character gone through extensive changes, ensure wicked is updated
	sObject* pObjectToRecreateInWicked = GetObjectData(iCharObj);
	WickedCall_AddObject(pObjectToRecreateInWicked);
	WickedCall_UpdateObject(pObjectToRecreateInWicked);
	WickedCall_TextureObject(pObjectToRecreateInWicked, NULL);

	// Change any mesh settings so they display correctly.
	for (int iMeshIndex = 0; iMeshIndex < pObjectToRecreateInWicked->iMeshCount; iMeshIndex++)
	{
		for (int i = 0; i < g_MeshesThatNeedDoubleSided.size(); i++)
		{
			if (strstr(pObjectToRecreateInWicked->ppMeshList[iMeshIndex]->pTextures[1].pName, g_MeshesThatNeedDoubleSided[i]))
			{
				sMesh* pMesh = pObjectToRecreateInWicked->ppMeshList[iMeshIndex];
				pMesh->bTransparency = false;
				pMesh->bCull = false;

				wiScene::MeshComponent* pMeshComponent = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);

				if (pMeshComponent)
				{
					pMeshComponent->SetDoubleSided(true);
					uint64_t materialEntity = pMeshComponent->subsets[0].materialID;
					wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
					pMeshMaterial->alphaRef = 0.5f;
					pMeshMaterial->SetDirty();
					pMeshMaterial->SetDoubleSided();
					WickedCall_UpdateObject(pObjectToRecreateInWicked);
					pMeshMaterial->userBlendMode = BLENDMODE::BLENDMODE_OPAQUE;
				}

				break;
			}
		}

		// and set character to use full reflectance (as all body parts have alpha data in surface texture)
		// LB: reduced from 1.0 to 0.04 to reduce washed out look on no/minimal normal maps 
		//LB: Changed back to 0.04 reflectance to solve washed out look on surfaces with no/minimal normal map
		//WickedCall_SetReflectance(pObjectToRecreateInWicked->ppMeshList[iMeshIndex], 1.0f);
		char* pNameFromTexture = pObjectToRecreateInWicked->ppMeshList[iMeshIndex]->pTextures[0].pName;
		bool bNeedReflectance = false;
		if (pNameFromTexture && pestrcasestr(pNameFromTexture, "glasses")) bNeedReflectance = true;
		if (bNeedReflectance)
			WickedCall_SetReflectance(pObjectToRecreateInWicked->ppMeshList[iMeshIndex], 1.0f);
		else
			WickedCall_SetReflectance(pObjectToRecreateInWicked->ppMeshList[iMeshIndex], 0.04f);
	}

	for (int i = 0; i < 10; i++) g_iPartsThatNeedReloaded[i] = 0;

	// position final stitched character in scene
	float terrain_height = BT_GetGroundHeight(t.terrain.TerrainID, GGORIGIN_X, GGORIGIN_Z, 1);
	fCharObjectY = terrain_height;
	PositionObject(iCharObj, ccpObjTargetX, ccpObjTargetY, ccpObjTargetZ);
	RotateObject(iCharObj, ccpObjTargetAX, ccpObjTargetAY, ccpObjTargetAZ);
	SetObjectArtFlags(iCharObj, (1 << 1) + (0), 0);
	LoopObject(iCharObj, 15, 55);

	// set object default animation speed
	SetObjectSpeed(iCharObj, 100);

	// finished legacy loading requirements
	image_setlegacyimageloading(false);

	// Store the choices for each character so that they can be remembered after the user switches base type.
	if (stricmp(CCP_Type, "Adult Male") == NULL)
	{
		charactercreatorplus_copyselections(g_maleStorage);
	}
	else if (stricmp(CCP_Type, "Adult Female") == NULL)
	{
		charactercreatorplus_copyselections(g_femaleStorage);
	}
	else if (stricmp(CCP_Type, "Zombie") == NULL)
	{
		charactercreatorplus_copyselections(g_zombieStorage);
	}
	else
	{
		charactercreatorplus_copyselections(g_genericStorage);
	}

	if ((part == 0 || part == 2) && g_pLastHeadgearAutoSwap)
	{
		char cName[64];
		// Get the name of the currently selected head.
		for (const auto& annotation : CharacterCreatorAnnotatedHeadGear_s)
		{
			if (stricmp(annotation.first.c_str(), cSelectedHeadGear) == NULL)
			{
				strcpy(cName, annotation.second.c_str());
			}
		}

		// Check to see if the head was auto-swapped, if so we need to change the selected head, to hide the swap.
		for (int i = 0; i < g_pLastHeadgearAutoSwap->requiredSwapCategories.size(); i++)
		{
			if (g_pLastHeadgearAutoSwap->requiredSwapCategories[i] == 2)
				strcpy(cSelectedHead, g_pLastHeadgearAutoSwap->swappedPartNames[i].c_str());
		}
	}
}

void charactercreatorplus_loadannotationlist ( void )
{
	std::vector<std::string> annotatesFiles;

	// Find all of the annotates.txt files.
	for (int c = 1; c <= ChecklistQuantity(); c++)
	{
		cStr tfile_s = Lower(ChecklistString(c));
		if (tfile_s != "." && tfile_s != "..")
		{
			if (strstr(tfile_s.Get(), "annotates"))
			{
				if (strcmp(Right(tfile_s.Get(), 4), ".txt") == 0)
				{
					std::string file(tfile_s.Get());
					annotatesFiles.push_back(file);
				}
			}
		}
	}

	g_charactercreatorplus_annotation_list.clear();
	g_charactercreatorplus_annotationtag_list.clear();

	// Extract everything from all the annotates files.
	for (int i = 0; i < annotatesFiles.size(); i++)
	{
		LPSTR pFilename = (LPSTR)annotatesFiles[i].c_str();
		if (FileExist(pFilename) == 1)
		{
			OpenToRead(1, pFilename);
			while (FileEnd(1) == 0)
			{
				// get line by line
				cstr line_s = ReadString(1);
				LPSTR pLine = line_s.Get();

				// get field name
				char pFieldName[260];
				char pFieldValue[260];
				strcpy(pFieldName, pLine);
				LPSTR pEquals = strstr(pFieldName, "=");
				if (pEquals)
				{
					LPSTR pEqualsPos = pEquals;

					// field name - eat spaces at end of field name
					pEquals--;
					while (pEquals > pFieldName && *pEquals == 32) pEquals--;
					*(pEquals + 1) = 0;

					// rest is field value
					strcpy(pFieldValue, pEqualsPos + 2);

					// strip off any tags
					char tag[260];
					strcpy(tag, "");
					for (int tagn = 0; tagn < strlen(pFieldValue); tagn++)
					{
						if (pFieldValue[tagn] == '[')
						{
							strcpy(tag, pFieldValue + tagn + 1);
							pFieldValue[tagn] = 0;
						}
					}
					while (strlen(tag) > 0 && (tag[strlen(tag) - 1] == ' ' || tag[strlen(tag) - 1] == ']')) tag[strlen(tag) - 1] = 0;

					// add to good list of annotations
					g_charactercreatorplus_annotation_list.insert(std::make_pair(pFieldName, pFieldValue));
					g_charactercreatorplus_annotationtag_list.insert(std::make_pair(pFieldName, tag));
				}
			}

			// close file handling
			CloseFile(1);
		}
	}
}

LPSTR charactercreatorplus_findannotation ( LPSTR pSearchStr )
{
	LPSTR pNewString = NULL;
	for ( std::map<std::string, std::string>::iterator it = g_charactercreatorplus_annotation_list.begin(); it != g_charactercreatorplus_annotation_list.end(); ++it)
	{
		std::string field = it->first;
		if ( stricmp ( field.c_str(), pSearchStr ) == NULL )
		{
			pNewString = (LPSTR)it->second.c_str();
			break;
		}
	}
	return pNewString;
}

LPSTR charactercreatorplus_findannotationtag ( LPSTR pSearchStr )
{
	LPSTR pNewString = NULL;
	for ( std::map<std::string, std::string>::iterator it = g_charactercreatorplus_annotationtag_list.begin(); it != g_charactercreatorplus_annotationtag_list.end(); ++it)
	{
		std::string field = it->first;
		if ( stricmp ( field.c_str(), pSearchStr ) == NULL )
		{
			pNewString = (LPSTR)it->second.c_str();
			break;
		}
	}
	return pNewString;
}

void charactercreatorplus_extractpartnumberandvariation(const char* source, char* number, char* variation)
{
	char temp[5];

	if (stricmp(source, "None") == NULL || strlen(source) == 0)
	{
		number[0] = 0;
		variation[0] = 0;
		return;
	}

	for (int i = strlen(source) - 1; i >= 0; i--)
	{
		if (source[i] == ' ')
		{
			strcpy(temp, source + i + 1);
			break;
		}
	}

	if (strlen(temp) > 0)
	{
		strcpy(variation, temp);

		if (strlen(temp) == 3)
		{
			temp[2] = 0;
		}
		
		strcpy(number, temp);
	}
}

int GetBaseValueFromCCPType(LPSTR pCCP_Type)
{
	for (int n = 0; n < g_CharacterType.size(); n++)
	{
		if (stricmp (pCCP_Type, g_CharacterType[n].pPartsFolder) == NULL)
		{
			return 1 + n;
		}
	}
	return 1;
}

