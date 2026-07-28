

//
// Utterly Nre Terrain System for GameGuru MAX
//

// Force update

#include "stdafx.h"
#include "gameguru.h"

#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

//#include "Terrain.h"
#include "../Include/Utility/stb_image.h" // Fixed include path

#include "GGTerrain\GGTerrainFile.h"
#include "GGTerrain\GGTerrain.h"
using namespace GGTerrain;

#include "GGTerrain\GGTrees.h"
using namespace GGTrees;

#include "GGTerrain\GGGrass.h"
using namespace GGGrass;

#define DYNAMICSUNPOSITION

#ifdef BUSHUI
void imgui_Customize_Bush_v3(int mode);
#endif

//PE: DISPLAY4x4 for new design , will get removed when ready.
#define DISPLAY4x4

void SmallTutorialVideo(char *tutorial, char* combo_items[] = NULL, int combo_entries = 0, int iVideoSection = 0, bool bAutoStart = false);
void visuals_calcsunanglefromtimeofday(int iTimeOfday, float* pfSunAngleX, float* pfSunAngleY, float* pfSunAngleZ);

// shadow mapping
//extern int g_iTerrainIDForShadowMap;

extern bool bHelp_Window;
extern bool bHelpVideo_Window;
extern char cForceTutorialName[1024];

extern bool bBoostIconColors;

extern bool bProceduralLevel;
extern int iQuitProceduralLevel;
extern bool BackBufferSnapShotMode;
extern bool bSnapShotModeUseCamera;
extern float fSnapShotModeCameraX, fSnapShotModeCameraY, fSnapShotModeCameraZ;
extern float fSnapShotModeCameraAngX, fSnapShotModeCameraAngY, fSnapShotModeCameraAngZ;
extern int BackBufferEntityID;
extern int BackBufferObjectID;
extern int BackBufferImageID;
extern int BackBufferSizeX;
extern int BackBufferSizeY;
extern float BackBufferRotateY;
extern float BackBufferRotateX;
extern float BackBufferRotateZ;
extern float BackBufferZoom;
extern float BackBufferCamMove;
extern float BackBufferCamLeft;
extern float BackBufferCamUp;
extern bool bBackBufferAnimated;
extern bool bBackBufferRestoreCamera;
extern bool bLoopBackBuffer;
extern bool bLoopFullFPS;
extern bool bRotateBackBuffer;
extern cstr BackBufferSaveCacheName;
extern int iLaunchAfterSync;
extern int iSkibFramesBeforeLaunch;
extern float fLastRubberBandX1, fLastRubberBandX2, fLastRubberBandY1, fLastRubberBandY2;
extern ImVec4 drawCol_back;
extern ImVec4 drawCol_normal;
extern ImVec4 drawCol_hover;
extern ImVec4 drawCol_Down;
bool bPopModalOpenProcedural = false;
bool bPopModalOpenProceduralCameraMode = false;
bool bPopModalTakeMapSnapshot = false;

#ifdef CUSTOMTEXTURES
int g_iCustomTerrainMatSounds[32] = { 10 };
#endif

int g_iDeferTextureUpdateToNow = 0;
cstr g_DeferTextureUpdateMAXRootFolder_s = "";
cstr g_DeferTextureUpdateCurrentFolder_s = "";
std::vector<std::string> g_DeferTextureUpdate;
std::vector<int> g_DeferTextureUpdateIncompatibleTextures;

void Wicked_Update_Visuals(void *voidvisual);
ImVec4 vLastTerrainPickPosition = ImVec4(0, 0,0,0);
ImVec4 vLastRampTerrainPickPosition = ImVec4(0, 0, 0, 0);
extern bool bProceduralLevelFromStoryboard;

#include "..\..\Guru-WickedMAX\master.h"
extern wiECS::Entity g_weatherEntityID;
extern MasterRenderer * master_renderer;


// Terrain Build Globals
#define TERRAINTEXPANELSPRMAX 6

int sTerrainTexturesID[32];
int sTerrainSelectionID[32];
bool bTextureNameWindow[32];
bool iDeleteAllTerrainTextures = false;
int iDeleteSingleTerrainTextures = 0;
int iCurrentTextureForPaint = 0;
bool bUpdateTerrainMaterials = false;
int iOldMaterial = -1;
int sGrassTexturesID[128];
bool iDeleteAllGrassTextures = false;
int iDeleteSingleGrassTextures = 0;
bool bUpdateGrassMaterials = false;
int iOldGrassMaterial = -1;
bool bCurrentGrassTextureForPaint[128];
cStr sGrassChangedTextures[128];

bool bGrassNameWindow[128];

float g_fvegRandomMin = 0.0f;
float g_fvegRandomMax = 100.0f;

float g_fvegDensityMin = 0.0f;
float g_fvegDensityMax = 100.0f;

int iBrushStrength = 128;
int iBrushShape = 0;

int g_iDelayActualObjectAdjustment = 0;
int g_iDelayActualObjectAdjustmentSculptCount = 0;

extern int iLastOpenHeader;
extern bool bRenderTabTab;

// Tried switching between texture_D.dds and texture_D.jpg but JPG just as large, lower quality and loses a channel!
#define TEXTURE_D_NAME "texture_D.dds"
#define TEXTURE_N_NAME "texture_N.dds"

VOID DXUtil_ConvertWideStringToAnsi ( CHAR* strDestination, const WCHAR* wstrSource,int cchDestChar );

// Terrain Build Local Structure
struct terrainbuildtype
{
	int initialised;
	int active;
	bool bReleaseMouseFirst;
	bool bUsingTerrainTextureSystemPaintSelector;
	bool bCustomiseTexture;
	cstr terrainstyle;
	int iCurrentTexture;
	int iTexturePanelSprite[6];
	int iTexturePanelImg[6];
	int iTexturePanelX;
	int iTexturePanelY;
	int iTexturePanelWidth;
	int iTexturePanelHeight;
	int iTexturePanelHighSprite;
	int iTexturePanelHighImg;
	int iHelpSpr;
	int iHelpImg;
	int iTexHelpSpr;
	int iTexHelpImg;
	int iTexPlateImage;
};
terrainbuildtype terrainbuild;


int delay_terrain_execute = 0;
int skib_terrain_frames_execute = 0;

extern bool bTerrain_Tools_Window;
extern int grideleprof_uniqui_id;
extern preferences pref;
extern bool bForceKey;
extern int iForceScancode;
extern cstr csForceKey;
extern bool bForceKey2;
extern cstr csForceKey2;
extern bool imgui_is_running;
bool bDisableAllTerrainSprites = true;

bool bEnableVeg = true; //Display veg on by default.
bool bUpdateVeg = true; //Update veg on by default.
int iLastUpdateVeg = 0;
int iForceUpdateVegetation = 0;
bool bEnableWeather = false;
extern bool bTutorialCheckAction;
bool TutorialNextAction(void);
bool CheckTutorialPlaceit(void);
bool CheckTutorialAction(const char * action, float x_adder = 0.0f);

int iTerrainPaintMode = 1;
int iTerrainGrassPaintMode = 1;
int iTerrainRaiseMode = 1;
int iLastRegionUpdateX, iLastRegionUpdateY;
int iTerrainVegLoopUpdate = 0;
extern bool bImGuiGotFocus;
bool bVegHasChanged = false;

struct NewLevelCamera
{
	float x;
	float y;
	float z;
	int set = 0;
};
NewLevelCamera newLevelCamera;
void PositionCameraForNewLevel();

struct sCustomBiomeType
{
	char pName[260] = {};
	int randomizetimeofday = 0;
	int showtrees = 0;
	int showgrass = 0;
	int showterrain = 0;
	int showwater = 1;
	int treesbitfield = 9673113696;
	int treesscalerandomlow = 40;
	int treesscalerandomhigh = 200;
	int treeschangedensity = 40;
	int grasspainttype = 1;
	int grasspaintdensity = 100;
	int grasspaintmaterial = 0;
	int waterline = -500;
	int waterspeed = 0.06;
	int waterred = 9;
	int watergreen = 21;
	int waterblue = 43;
	int wateralpha = 0;
	int waterwaveamplitude = 20;
	int waterwinddependency = 0;
	int waterpatchlength = 40;
	int waterchoppyscale = 0;
	int waterfogmindist = 0;
	int waterfogmaxdist = 11500;
	int waterfogminamount = 0;
	int waterdistance = 400;
	int waterenable = 1;
	int proceduralterraintype = 6;
};
std::vector<sCustomBiomeType> g_sCustomBiomes;

// Prototypes
void set_inputsys_mclick(int value);

int current_mode = 0;
ImVec4 tool_selected_col;

void imgui_populatecustombiomes(void)
{
	// store current dir
	cstr pOldDir = GetDir();

	// collect
	g_sCustomBiomes.clear();
	char pWritableCustomBiomeFolder[MAX_PATH];
	strcpy (pWritableCustomBiomeFolder, "editors\\biomes\\custom\\");
	GG_GetRealPath(pWritableCustomBiomeFolder, 0);
	SetDir(pWritableCustomBiomeFolder);
	ChecklistForFiles();
	for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
	{
		t.tfile_s = ChecklistString(t.c);
		LPSTR pThisFolder = t.tfile_s.Get();
		if (Len(pThisFolder) > 2)
		{
			char pCheckCustomBiomeFile[MAX_PATH];
			sprintf(pCheckCustomBiomeFile, "%s\\settings.ter", t.tfile_s.Get());
			if (FileExist (pCheckCustomBiomeFile) == 1)
			{
				sCustomBiomeType item;
				strcpy (item.pName, t.tfile_s.Get());

				// read profile to fill in custom biome values
				char pCheckCustomBiomeProfile[MAX_PATH];
				sprintf(pCheckCustomBiomeProfile, "%s\\profile.dat", t.tfile_s.Get());
				if (FileExist(pCheckCustomBiomeProfile) == 0)
				{
					// write out a template profile file to help out
					OpenToWrite(1, pCheckCustomBiomeProfile);
					WriteString(1, ";");
					WriteString(1, ";Profile Template created by Terrain Generator");
					WriteString(1, ";");
					WriteString(1, ";Save a 'settings.ter' from the Save Terrain option in Terrain Generator");
					WriteString(1, ";Include a WAV file called 'atmosloop.wav' to perform a looping atmospheric sound");
					WriteString(1, ";Include a 'textures' folder containing an entire terraintextures folder set");
					WriteString(1, "");
					CloseFile(1);
				}
				else
				{
					// read an existing profile file to populate the custom biome values
					OpenToRead(1, pCheckCustomBiomeProfile);
					while (FileEnd(1) == 0)
					{
						LPSTR pLine = ReadString(1);
						if (strlen(pLine) > 0)
						{
							if (pLine[0] != ';')
							{
								char pLineToCrop[256];
								strcpy(pLineToCrop, pLine);
								LPSTR pLine2 = pLineToCrop;
								LPSTR pLine3 = strchr(pLine2, '=');
								if (pLine3 != NULL)
								{
									*pLine3 = 0;
									pLine3++;
									if (stricmp(pLine2, "randomizetimeofday") == 0) { item.randomizetimeofday = atoi(pLine3); }
									else if (stricmp(pLine2, "showtrees") == 0) { item.showtrees = atoi(pLine3); }
									else if (stricmp(pLine2, "showgrass") == 0) { item.showgrass = atoi(pLine3); }
									else if (stricmp(pLine2, "showterrain") == 0) { item.showterrain = atoi(pLine3); }
									else if (stricmp(pLine2, "showwater") == 0) { item.showwater = atoi(pLine3); }
									else if (stricmp(pLine2, "treesbitfield") == 0) { item.treesbitfield = atoi(pLine3); }
									else if (stricmp(pLine2, "treesscalerandomlow") == 0) { item.treesscalerandomlow = atoi(pLine3); }
									else if (stricmp(pLine2, "treesscalerandomhigh") == 0) { item.treesscalerandomhigh = atoi(pLine3); }
									else if (stricmp(pLine2, "treeschangedensity") == 0) { item.treeschangedensity = atoi(pLine3); }
									else if (stricmp(pLine2, "grasspainttype") == 0) { item.grasspainttype = atoi(pLine3); }
									else if (stricmp(pLine2, "grasspaintdensity") == 0) { item.grasspaintdensity = atoi(pLine3); }
									else if (stricmp(pLine2, "grasspaintmaterial") == 0) { item.grasspaintmaterial = atoi(pLine3); }
									else if (stricmp(pLine2, "waterline") == 0) { item.waterline = atoi(pLine3); }
									else if (stricmp(pLine2, "waterspeed") == 0) { item.waterspeed = atoi(pLine3); }
									else if (stricmp(pLine2, "waterred") == 0) { item.waterred = atoi(pLine3); }
									else if (stricmp(pLine2, "watergreen") == 0) { item.watergreen = atoi(pLine3); }
									else if (stricmp(pLine2, "waterblue") == 0) { item.waterblue = atoi(pLine3); }
									else if (stricmp(pLine2, "wateralpha") == 0) { item.wateralpha = atoi(pLine3); }
									else if (stricmp(pLine2, "waterwaveamplitude") == 0) { item.waterwaveamplitude = atoi(pLine3); }
									else if (stricmp(pLine2, "waterwinddependency") == 0) { item.waterwinddependency = atoi(pLine3); }
									else if (stricmp(pLine2, "waterpatchlength") == 0) { item.waterpatchlength = atoi(pLine3); }
									else if (stricmp(pLine2, "waterchoppyscale") == 0) { item.waterchoppyscale = atoi(pLine3); }
									else if (stricmp(pLine2, "waterfogmindist") == 0) { item.waterfogmindist = atoi(pLine3); }
									else if (stricmp(pLine2, "waterfogmaxdist") == 0) { item.waterfogmaxdist = atoi(pLine3); }
									else if (stricmp(pLine2, "waterfogminamount") == 0) { item.waterfogminamount = atoi(pLine3); }
									else if (stricmp(pLine2, "waterdistance") == 0) { item.waterdistance = atoi(pLine3); }
									else if (stricmp(pLine2, "waterenable") == 0) { item.waterenable = atoi(pLine3); }
									else if (stricmp(pLine2, "proceduralterraintype") == 0) { item.proceduralterraintype = atoi(pLine3); }
								}
							}
						}
					}
					CloseFile(1);
				}

				// add custom biome to list
				g_sCustomBiomes.push_back(item);
			}
		}
	}

	// restore dir
	SetDir(pOldDir.Get());
}

void imgui_terrain_loop_v2(void)
{
	if (!imgui_is_running)
		return;

	// terrain editing causes grass to fully update (hills raise grass)
	if (iForceUpdateVegetation == 2)
	{
		iForceUpdateVegetation = 0;
		extern bool bFullVegUpdate;
		bFullVegUpdate = true;
		bUpdateVeg = true;
	}

	if (bUpdateVeg)
	{
		if (bEnableVeg)
		{
			t.visuals.VegQuantity_f = t.gamevisuals.VegQuantity_f;
			t.visuals.VegWidth_f = t.gamevisuals.VegWidth_f;
			t.visuals.VegHeight_f = t.gamevisuals.VegHeight_f;

			extern bool bResourcesSet, bGridMade;

			bool bOldGridMade = bGridMade;
			int iTrimUsingGrassMemblock = 0;
			if (t.game.gameisexe == 1) iTrimUsingGrassMemblock = t.terrain.grassmemblock;
			if (g.usegrassbelowwater > 0)
				MakeVegetationGridQuick(4.0f*t.visuals.VegQuantity_f, t.visuals.VegWidth_f, t.visuals.VegHeight_f, terrain_veg_areawidth, t.terrain.vegetationgridsize, t.tTerrainID, iTrimUsingGrassMemblock, true);
			else
				MakeVegetationGridQuick(4.0f*t.visuals.VegQuantity_f, t.visuals.VegWidth_f, t.visuals.VegHeight_f, terrain_veg_areawidth, t.terrain.vegetationgridsize, t.tTerrainID, iTrimUsingGrassMemblock, false);

			// small lookup for memblock painting circles
			static bool bCurveDataSet = false;
			if (!bCurveDataSet) {
				Dim(t.curve_f, 100);
				for (t.r = 0; t.r <= 180; t.r++)
				{
					t.trx_f = Cos(t.r - 90)*100.0;
					t.trz_f = Sin(t.r - 90)*100.0;
					t.curve_f[int((100 + t.trz_f) / 2)] = t.trx_f / 100.0;
				}
				bCurveDataSet = true;
			}
			t.terrain.grassregionupdate = 0; //PE: Make sure we update.
			t.terrain.grassupdateafterterrain = 1;
			t.terrain.lastgrassupdatex1 = -1; //PE: Force update.
			t.terrain.grassupdateafterterrain = 0;
			ShowVegetationGrid();
			visuals_justshaderupdate();
			iLastUpdateVeg = MAXTimer();
		}
		else
		{
			HideVegetationGrid();
			iLastUpdateVeg = MAXTimer();
		}
		bUpdateVeg = false;
	}
	else
	{
		bool bReadyToUpdateVeg = false;
		if (bVegHasChanged)
			bReadyToUpdateVeg = true;

		if (bEnableVeg && iTerrainVegLoopUpdate++ > 10)
		{
			iTerrainVegLoopUpdate = 0;
		}

		//Continue cheking if we need to update terrain.
		if (bReadyToUpdateVeg && bEnableVeg)
		{
			t.visuals.VegQuantity_f = t.gamevisuals.VegQuantity_f;
			t.visuals.VegWidth_f = t.gamevisuals.VegWidth_f;
			t.visuals.VegHeight_f = t.gamevisuals.VegHeight_f;

			t.terrain.grassupdateafterterrain = 1;
			t.terrain.grassupdateafterterrain = 0;
			ShowVegetationGrid();

			bReadyToUpdateVeg = false;
			iLastUpdateVeg = MAXTimer();
			bVegHasChanged = false;
		}
	}

	if (t.grideditselect == 0 && t.terrain.terrainpaintermode >= 0 && t.terrain.terrainpaintermode <= 10)
	{
#ifdef ENABLECUSTOMTERRAIN
		switch (delay_terrain_execute)
		{
		case 1: //Change texture in plate.
		{
			delay_terrain_execute = 0;
			terrainbuild.bCustomiseTexture = true;
			skib_terrain_frames_execute = 5;
			break;
		}
		default:
			break;
		}
#endif
		if (skib_terrain_frames_execute > 0)
			skib_terrain_frames_execute--;

		if (bTerrain_Tools_Window) 
		{
			float media_icon_size = 40.0f;
			float plate_width = (media_icon_size + 6.0) * 4.0f;
			grideleprof_uniqui_id = 16000;
			int icon_size = 60;
			ImVec2 iToolbarIconSize = { (float)icon_size, (float)icon_size };
			ImVec2 tool_selected_padding = { 1.0, 1.0 };
			tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
			if (pref.current_style == 3)
				tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_Button];

			current_mode = -1;
			if (t.terrain.terrainpaintermode >= 6) 
			{
				if (t.terrain.terrainpaintermode == 10) 
				{
					current_mode = TOOL_PAINTGRASS;
				}
				else 
				{
					current_mode = TOOL_PAINTTEXTURE;
				}
			}
			else 
			{
				if (t.terrain.terrainpaintermode == 1)
					current_mode = TOOL_SHAPE;
				if (t.terrain.terrainpaintermode == 2)
					current_mode = TOOL_LEVELMODE;
				if (t.terrain.terrainpaintermode == 3)
					current_mode = TOOL_STOREDLEVEL;
				if (t.terrain.terrainpaintermode == 4)
					current_mode = TOOL_BLENDMODE;
				if (t.terrain.terrainpaintermode == 5)
					current_mode = TOOL_RAMPMODE;
			}

			cstr sWindowLabel = "Sculpt Terrain##TerrainToolsWindow";
			if (current_mode == TOOL_PAINTGRASS)
				sWindowLabel = "Add Vegetation##TerrainToolsWindow";
			if (current_mode == TOOL_PAINTTEXTURE)
				sWindowLabel = "Paint Terrain##TerrainToolsWindow";

			extern int iGenralWindowsFlags;
			ImGui::Begin(sWindowLabel.Get(), &bTerrain_Tools_Window, iGenralWindowsFlags);

			float w = ImGui::GetWindowContentRegionWidth();
			ImGuiWindow* window = ImGui::GetCurrentWindow();

			if (pref.bAutoClosePropertySections && iLastOpenHeader != 30)
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);

			if (ImGui::StyleCollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen))
			{
				iLastOpenHeader = 30;

				extern wiECS::Entity g_weatherEntityID;
				wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);

				// do this to unfocus text input when right clicking on main window
				if ( ImGui::IsMouseClicked(1) ) 
				{
					//PE: This will close down any popups.
					extern bool bPopModalOpenEntity;
					if (!bPopModalOpenEntity && !bPopModalOpenProcedural)
					{
						ImGui::FocusWindow(NULL);
						ImGui::ClearActiveID();
					}
				}

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				ImGui::PushItemWidth(-10);

				float w = ImGui::GetWindowContentRegionWidth();
				int icon_size = (w - 20.0) / 3.0;
				icon_size -= 7; //Padding
				
				ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
				ImGui::GetStyle().FramePadding = { 2,2 };

				static int terrainMode = -1; // -1 = inital value, 0 = generate, 1 = edit, 2 = paint
				
				if ( terrainMode == 0 )
				{
					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					ImVec2 padding = { 2.0, 2.0 };
					const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(icon_size, icon_size));
					ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}

				if ( ImGui::ImgBtn(KEY_G, ImVec2(icon_size, icon_size), ImColor(255,255,255,0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255) ,-1,0,0,0,false,false,false,false,false, bBoostIconColors)
					 || terrainMode == -1 )
				{
					terrainMode = 0;
					weather->SetVolumetricClouds( false );
					ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_USE_FOG;
					GGTerrain_SetEditSizeVisible( 1 );
					GGTerrain_SetMiniMapVisible( 0 );
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("Generate New Terrain");
				ImGui::SameLine();

				if ( terrainMode == 1 )
				{
					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					ImVec2 padding = { 2.0, 2.0 };
					const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(icon_size, icon_size));
					ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				if (ImGui::ImgBtn(TOOL_LEVELMODE, ImVec2(icon_size, icon_size), ImColor(255, 255, 255, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					terrainMode = 1;
					weather->SetVolumetricClouds( true );
					ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_USE_FOG;
					GGTerrain_SetEditSizeVisible( 1 );
					GGTerrain_SetMiniMapVisible( 0 );
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("Sculpt Terrain");
				ImGui::SameLine();

				if ( terrainMode == 2 )
				{
					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					ImVec2 padding = { 2.0, 2.0 };
					const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(icon_size, icon_size));
					ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				if (ImGui::ImgBtn(EBE_CONTROL2, ImVec2(icon_size, icon_size), ImColor(255, 255, 255, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					terrainMode = 2;
					weather->SetVolumetricClouds( true );
					ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_USE_FOG;
					GGTerrain_SetEditSizeVisible( 0 );
					GGTerrain_SetMiniMapVisible( 0 );
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("Paint Terrain");

				ImGui::GetStyle().FramePadding = oldstyle;

				ImGui::Separator();
				
				ImGui::Text("Enable Water"); ImGui::SameLine();
				if ( ImGui::Checkbox("##EnableWater", &t.visuals.bWaterEnable)) 
				{
					t.showeditorwater = t.visuals.bWaterEnable ? 1 : 0;
					t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
					Wicked_Update_Visuals((void *)&t.visuals);
				}

				float meterValue;

				switch( terrainMode )
				{
					case 0: // generate
					{
						float fButSizeX = ImGui::GetContentRegionAvailWidth() / 4.0;
						fButSizeX -= 6.0f; //Padding.
						float fButSizeY = fButSizeX * 0.60f;

						if (ImGui::StyleButton("Desert", ImVec2(fButSizeX, fButSizeY)))
						{
							ggterrain_global_params.offset_y = GGTerrain_MetersToUnits(45);
							ggterrain_global_params.height = GGTerrain_MetersToUnits(48);
							ggterrain_global_params.minHeight = GGTerrain_MetersToUnits(48);
							ggterrain_global_params.noise_power = 1.258f;
							ggterrain_global_params.noise_fallof_power = 1.539f;
							ggterrain_global_params.fractal_levels = 10;
							ggterrain_global_params.fractal_initial_freq = 0.282f;
							ggterrain_global_params.fractal_freq_increase = 2.5f;
							ggterrain_global_params.fractal_freq_weight = 0.4;
							ggterrain_global_render_params.baseLayerMaterial = 0x100 | 7;
							ggterrain_global_render_params.layerMatIndex[0] = 0x100 | 7;
							ggterrain_global_render_params.layerMatIndex[1] = 0x100 | 7;
							ggterrain_global_render_params.layerMatIndex[2] = 0x100 | 7;
							ggterrain_global_render_params.layerStartHeight[0] = GGTerrain_MetersToUnits(0.0f);
							ggterrain_global_render_params.layerStartHeight[1] = GGTerrain_MetersToUnits(4.572f);
							ggterrain_global_render_params.layerStartHeight[2] = GGTerrain_MetersToUnits(145.7f);
							ggterrain_global_render_params.layerStartHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerStartHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[0] = GGTerrain_MetersToUnits(1.524f);
							ggterrain_global_render_params.layerEndHeight[1] = GGTerrain_MetersToUnits(9.144f);
							ggterrain_global_render_params.layerEndHeight[2] = GGTerrain_MetersToUnits(205.7f);
							ggterrain_global_render_params.layerEndHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 6;
							ggterrain_global_render_params.slopeMatIndex[1] = 0x100 | 6;
							ggterrain_global_render_params.slopeStart[0] = 0.2f;
							ggterrain_global_render_params.slopeStart[1] = 1.0f;
							ggterrain_global_render_params.slopeEnd[0] = 0.4f;
							ggterrain_global_render_params.slopeEnd[1] = 1.0f;

							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS0) | GGTERRAIN_FRACTAL_RIDGES0;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS1) | GGTERRAIN_FRACTAL_RIDGES1;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS2) | GGTERRAIN_FRACTAL_RIDGES2;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS3) | GGTERRAIN_FRACTAL_RIDGES3;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Desert Theme");
						ImGui::SameLine();

						if (ImGui::StyleButton("Snow", ImVec2(fButSizeX, fButSizeY))) //Arctic
						{
							ggterrain_global_params.offset_y = GGTerrain_MetersToUnits(0);
							ggterrain_global_params.height = GGTerrain_MetersToUnits(73);
							ggterrain_global_params.minHeight = GGTerrain_MetersToUnits(73);
							ggterrain_global_params.noise_power = 0.487f;
							ggterrain_global_params.noise_fallof_power = 0.144f;
							ggterrain_global_params.fractal_levels = 7;
							ggterrain_global_params.fractal_initial_freq = 0.282f;
							ggterrain_global_params.fractal_freq_increase = 2.5f;
							ggterrain_global_params.fractal_freq_weight = 0.4;
							ggterrain_global_render_params.baseLayerMaterial = 0x100 | 12;
							ggterrain_global_render_params.layerMatIndex[0] = 0x100 | 12;
							ggterrain_global_render_params.layerMatIndex[1] = 0x100 | 12;
							ggterrain_global_render_params.layerMatIndex[2] = 0x100 | 12;
							ggterrain_global_render_params.layerStartHeight[0] = GGTerrain_MetersToUnits(0.0f);
							ggterrain_global_render_params.layerStartHeight[1] = GGTerrain_MetersToUnits(4.572f);
							ggterrain_global_render_params.layerStartHeight[2] = GGTerrain_MetersToUnits(145.7f);
							ggterrain_global_render_params.layerStartHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerStartHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[0] = GGTerrain_MetersToUnits(1.524f);
							ggterrain_global_render_params.layerEndHeight[1] = GGTerrain_MetersToUnits(9.144f);
							ggterrain_global_render_params.layerEndHeight[2] = GGTerrain_MetersToUnits(205.7f);
							ggterrain_global_render_params.layerEndHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 12;
							ggterrain_global_render_params.slopeMatIndex[1] = 0x100 | 12;
							ggterrain_global_render_params.slopeStart[0] = 0.2f;
							ggterrain_global_render_params.slopeStart[1] = 1.0f;
							ggterrain_global_render_params.slopeEnd[0] = 0.4f;
							ggterrain_global_render_params.slopeEnd[1] = 1.0f;

							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS0) | GGTERRAIN_FRACTAL_RIDGES0;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS1) | GGTERRAIN_FRACTAL_RIDGES1;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS2) | GGTERRAIN_FRACTAL_RIDGES2;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS3) | GGTERRAIN_FRACTAL_RIDGES3;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Arctic Theme");
						ImGui::SameLine();

						if (ImGui::StyleButton("Temperate", ImVec2(fButSizeX, fButSizeY)))
						{
							ggterrain_global_params.offset_y = GGTerrain_MetersToUnits(8.9f);
							ggterrain_global_params.height = GGTerrain_MetersToUnits(131.5f);
							ggterrain_global_params.minHeight = GGTerrain_MetersToUnits(131.5f);
							ggterrain_global_params.noise_power = 1.3f;
							ggterrain_global_params.noise_fallof_power = 0.323f;
							ggterrain_global_params.fractal_levels = 6;
							ggterrain_global_params.fractal_initial_freq = 0.3f;
							ggterrain_global_params.fractal_freq_increase = 2.5f;
							ggterrain_global_params.fractal_freq_weight = 0.4;
							ggterrain_global_render_params.baseLayerMaterial = 0x100 | 17;
							ggterrain_global_render_params.layerMatIndex[0] = 0x100 | 28;
							ggterrain_global_render_params.layerMatIndex[1] = 0x100 | 29;
							ggterrain_global_render_params.layerMatIndex[2] = 0x100 | 0;
							ggterrain_global_render_params.layerStartHeight[0] = GGTerrain_MetersToUnits(0.0f);
							ggterrain_global_render_params.layerStartHeight[1] = GGTerrain_MetersToUnits(2.657f);
							ggterrain_global_render_params.layerStartHeight[2] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerStartHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerStartHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[0] = GGTerrain_MetersToUnits(1.524f);
							ggterrain_global_render_params.layerEndHeight[1] = GGTerrain_MetersToUnits(5.027f);
							ggterrain_global_render_params.layerEndHeight[2] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 30;
							ggterrain_global_render_params.slopeMatIndex[1] = 0x100 | 30;
							ggterrain_global_render_params.slopeStart[0] = 0.07f;
							ggterrain_global_render_params.slopeStart[1] = 1.0f;
							ggterrain_global_render_params.slopeEnd[0] = 0.2f;
							ggterrain_global_render_params.slopeEnd[1] = 1.0f;

							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_RIDGES0) | GGTERRAIN_FRACTAL_VALLEYS0;
							ggterrain_global_params.fractal_flags = ggterrain_global_params.fractal_flags & ~(GGTERRAIN_FRACTAL_VALLEYS1 | GGTERRAIN_FRACTAL_RIDGES1);
							ggterrain_global_params.fractal_flags = ggterrain_global_params.fractal_flags & ~(GGTERRAIN_FRACTAL_VALLEYS2 | GGTERRAIN_FRACTAL_RIDGES2);
							ggterrain_global_params.fractal_flags = ggterrain_global_params.fractal_flags & ~(GGTERRAIN_FRACTAL_VALLEYS3 | GGTERRAIN_FRACTAL_RIDGES3);
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Temperate Theme");
						ImGui::SameLine();

						if (ImGui::StyleButton("Mountains", ImVec2(fButSizeX, fButSizeY)))
						{
							ggterrain_global_params.offset_y = GGTerrain_MetersToUnits(87);
							ggterrain_global_params.height = GGTerrain_MetersToUnits(300);
							ggterrain_global_params.minHeight = GGTerrain_MetersToUnits(300);
							ggterrain_global_params.noise_power = 0.612f;
							ggterrain_global_params.noise_fallof_power = 0.299f;
							ggterrain_global_params.fractal_levels = 10;
							ggterrain_global_params.fractal_initial_freq = 0.371f;
							ggterrain_global_params.fractal_freq_increase = 2.5f;
							ggterrain_global_params.fractal_freq_weight = 0.4;
							ggterrain_global_render_params.baseLayerMaterial = 0x100 | 17;
							ggterrain_global_render_params.layerMatIndex[0] = 0x100 | 20;
							ggterrain_global_render_params.layerMatIndex[1] = 0x100 | 19;
							ggterrain_global_render_params.layerMatIndex[2] = 0x100 | 22;
							ggterrain_global_render_params.layerStartHeight[0] = GGTerrain_MetersToUnits(0.0f);
							ggterrain_global_render_params.layerStartHeight[1] = GGTerrain_MetersToUnits(4.572f);
							ggterrain_global_render_params.layerStartHeight[2] = GGTerrain_MetersToUnits(145.7f);
							ggterrain_global_render_params.layerStartHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerStartHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[0] = GGTerrain_MetersToUnits(1.524f);
							ggterrain_global_render_params.layerEndHeight[1] = GGTerrain_MetersToUnits(9.144f);
							ggterrain_global_render_params.layerEndHeight[2] = GGTerrain_MetersToUnits(205.7f);
							ggterrain_global_render_params.layerEndHeight[3] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.layerEndHeight[4] = GGTerrain_MetersToUnits(1500.0f);
							ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 18;
							ggterrain_global_render_params.slopeMatIndex[1] = 0x100 | 18;
							ggterrain_global_render_params.slopeStart[0] = 0.2f;
							ggterrain_global_render_params.slopeStart[1] = 1.0f;
							ggterrain_global_render_params.slopeEnd[0] = 0.4f;
							ggterrain_global_render_params.slopeEnd[1] = 1.0f;

							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_RIDGES0) | GGTERRAIN_FRACTAL_VALLEYS0;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS1) | GGTERRAIN_FRACTAL_RIDGES1;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS2) | GGTERRAIN_FRACTAL_RIDGES2;
							ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS3) | GGTERRAIN_FRACTAL_RIDGES3;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Mountain Theme");

						fButSizeX = ImGui::GetContentRegionAvailWidth();
						if (ImGui::StyleButton("Randomize", ImVec2(fButSizeX, 30.0f)))
						{
							ggterrain_global_params.seed = Random2();
							float height = pow( RandomFloat(), 0.7 );
							height = height * 590 + 10;
							ggterrain_global_params.height = GGTerrain_MetersToUnits( height );
							ggterrain_global_params.minHeight = GGTerrain_MetersToUnits( height );
							float offsety = pow( RandomFloat(), 1.7 );
							if ( Random2() & 0x01 ) offsety = -offsety;
							offsety *= 50;
							if ( offsety + height < 5 ) offsety = 5 - height;
							ggterrain_global_params.offset_y = GGTerrain_MetersToUnits( offsety );
							ggterrain_global_params.noise_power = RandomFloat() * 1.5f + 0.5f;
							ggterrain_global_params.noise_fallof_power = RandomFloat() * 1.2f;
						}

						ImGui::Text("Seed"); ImGui::SameLine();
						int step = 1; int step_fast = 100;
						ImGui::InputScalar( "##TerrainSeed", ImGuiDataType_U32, &ggterrain_global_params.seed, &step, &step_fast, "%u", ImGuiInputTextFlags_AutoSelectAll );

						ImGui::TextCenter("Performance Setting");
						static int terrain_performance = 2;
						if ( ImGui::SliderInt("##TerrainPerformance", &terrain_performance, 0, 3) )
						{
							GGTerrain_SetPerformanceMode( terrain_performance );
						}
						
						ImGui::TextCenter("Editable Size (meters)");
						// editable_size is from center to edge whereas meterValue is edge to edge so multiply by 2
						meterValue = GGTerrain_UnitsToMeters( ggterrain_global_render_params2.editable_size*2 );
						if ( ImGui::SliderFloat("##EditableSize", &meterValue, 500.0f, 5000.0f) ) 
						{
							ggterrain_global_render_params2.editable_size = GGTerrain_MetersToUnits( meterValue/2 );
						}

						if (pref.iTerrainAdvanced)
						{
							ImGui::TextCenter("Show Editable Area");
							ImGui::Text("2D");
							ImGui::SameLine();
							bool showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE) != 0;
							if (ImGui::Checkbox("##EditBoxVisible", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
							}
							ImGui::SameLine();
							ImGui::Text(" 3D");
							ImGui::SameLine();
							showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D) != 0;
							if (ImGui::Checkbox("##EditBoxVisible3D", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
							}
							ImGui::SameLine();

							ImGui::Text(" MiniMap");
							ImGui::SameLine();
							showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP) != 0;
							if (ImGui::Checkbox("##EditBoxVisibleMiniMap", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
							}
						}
						ImGui::TextCenter("Offset X");
						meterValue = GGTerrain_OffsetToMeters( ggterrain_global_params.offset_x );
						if ( ImGui::SliderFloat("##Offset X", &meterValue, -10000.0f, 10000.0f) )
						{
							ggterrain_global_params.offset_x = GGTerrain_MetersToOffset( meterValue );
						}

						ImGui::TextCenter("Offset Z");
						meterValue = GGTerrain_OffsetToMeters( ggterrain_global_params.offset_z );
						if ( ImGui::SliderFloat("##Offset Z", &meterValue, -10000.0f, 10000.0f) )
						{
							ggterrain_global_params.offset_z = GGTerrain_MetersToOffset( meterValue );
						}

						ImGui::TextCenter("Offset Y (meters)");
						meterValue = GGTerrain_UnitsToMeters( ggterrain_global_params.offset_y );
						if ( ImGui::SliderFloat("##Offset Y", &meterValue, -1000.0f, 1000.0f, "%.1f", 2.0f) )
						{
							ggterrain_global_params.offset_y = GGTerrain_MetersToUnits( meterValue );
						}

						// maximum height of the terrain
						ImGui::TextCenter("Max Height (meters)");
						meterValue = GGTerrain_UnitsToMeters( ggterrain_global_params.height );
						if ( ImGui::SliderFloat("##HeightRange", &meterValue, 0.0f, 1000.0f) )
						{
							ggterrain_global_params.height = GGTerrain_MetersToUnits( meterValue );
						}

						ImGui::TextCenter("Min Height Underwater (meters)");
						meterValue = GGTerrain_UnitsToMeters( ggterrain_global_params.minHeight );
						if ( ImGui::SliderFloat("##MinHeightRange", &meterValue, 0.0f, 1000.0f) )
						{
							ggterrain_global_params.minHeight = GGTerrain_MetersToUnits( meterValue );
						}

						ImGui::TextCenter("Height Map Scale");
						ImGui::SliderFloat("##HeightMapScale", &ggterrain_global_params.heightmap_scale, 0.01f, 10.0f, "%.3f", 2.0f);

						meterValue = GGTerrain_UnitsToMeters( ggterrain_global_params.height_outside_heightmap );
						ImGui::TextCenter("Height Map Outside Height");
						if ( ImGui::SliderFloat("##HeightMapOutHeight", &meterValue, -1000, 4000.0f, "%.1f", 1.5f) )
						{
							ggterrain_global_params.height_outside_heightmap = GGTerrain_MetersToUnits( meterValue );
						}

						ImGui::TextCenter("Height Map Outside Slope");
						ImGui::SliderFloat("##HeightMapOutFade", &ggterrain_global_params.fade_outside_heightmap, 0, 500.0f, "%.1f", 1.0f);

						int outsideType = GGTerrain_GetGenerateTerrainOutsideHeightMap();
						ImGui::TextCenter("Height Map Outside Type");
						if ( ImGui::SliderInt("##HeightMapOutType", &outsideType, 0, 1) )
						{
							GGTerrain_SetGenerateTerrainOutsideHeightMap( outsideType );
						}
				
						// how the noise value is modified after being generated
						ImGui::TextCenter("Noise Curve");
						ImGui::SliderFloat("##NoisePower", &ggterrain_global_params.noise_power, 0.01f, 6.0f, "%.3f", 2.0f);

						// how the noise value is modified by the height of the previous detail levels
						ImGui::TextCenter("Noise Falloff");
						ImGui::SliderFloat("##NoiseFalloffPower", &ggterrain_global_params.noise_fallof_power, 0.0f, 6.0f, "%.3f", 2.0f);

						// number of iterations of noise to create the fractal noise
						ImGui::TextCenter("Noise Iterations");
						ImGui::SliderInt("##FractalIterations", (int*)&ggterrain_global_params.fractal_levels, 1, 14);

						ImGui::TextCenter("Rivers/Valleys");
						int selection = 0;
						if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_VALLEYS0 ) selection = 1;
						else if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_RIDGES0 ) selection = 2;
						if ( ImGui::SliderInt("##FractalRiversValleys0", &selection, 0, 2) )
						{
							switch( selection )
							{
								case 0: ggterrain_global_params.fractal_flags &= ~(GGTERRAIN_FRACTAL_VALLEYS0 | GGTERRAIN_FRACTAL_RIDGES0); break;
								case 1: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_RIDGES0) | GGTERRAIN_FRACTAL_VALLEYS0; break;
								case 2: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS0) | GGTERRAIN_FRACTAL_RIDGES0; break;
							}
						}

						selection = 0;
						if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_VALLEYS1 ) selection = 1;
						else if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_RIDGES1 ) selection = 2;
						if ( ImGui::SliderInt("##FractalRiversValleys1", &selection, 0, 2) )
						{
							switch( selection )
							{
								case 0: ggterrain_global_params.fractal_flags &= ~(GGTERRAIN_FRACTAL_VALLEYS1 | GGTERRAIN_FRACTAL_RIDGES1); break;
								case 1: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_RIDGES1) | GGTERRAIN_FRACTAL_VALLEYS1; break;
								case 2: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS1) | GGTERRAIN_FRACTAL_RIDGES1; break;
							}
						}

						selection = 0;
						if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_VALLEYS2 ) selection = 1;
						else if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_RIDGES2 ) selection = 2;
						if ( ImGui::SliderInt("##FractalRiversValleys2", &selection, 0, 2) )
						{
							switch( selection )
							{
								case 0: ggterrain_global_params.fractal_flags &= ~(GGTERRAIN_FRACTAL_VALLEYS2 | GGTERRAIN_FRACTAL_RIDGES2); break;
								case 1: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_RIDGES2) | GGTERRAIN_FRACTAL_VALLEYS2; break;
								case 2: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS2) | GGTERRAIN_FRACTAL_RIDGES2; break;
							}
						}

						selection = 0;
						if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_VALLEYS3 ) selection = 1;
						else if ( ggterrain_global_params.fractal_flags & GGTERRAIN_FRACTAL_RIDGES3 ) selection = 2;
						if ( ImGui::SliderInt("##FractalRiversValleys3", &selection, 0, 2) )
						{
							switch( selection )
							{
								case 0: ggterrain_global_params.fractal_flags &= ~(GGTERRAIN_FRACTAL_VALLEYS3 | GGTERRAIN_FRACTAL_RIDGES3); break;
								case 1: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_RIDGES3) | GGTERRAIN_FRACTAL_VALLEYS3; break;
								case 2: ggterrain_global_params.fractal_flags = (ggterrain_global_params.fractal_flags & ~GGTERRAIN_FRACTAL_VALLEYS3) | GGTERRAIN_FRACTAL_RIDGES3; break;
							}
						}

						// scale of the initial noise frequency
						ImGui::TextCenter("Noise Initial Frequency");
						ImGui::SliderFloat("##FractalInitFreq", &ggterrain_global_params.fractal_initial_freq, 0.01f, 32.0f, "%.3f", 2.0f);

						ImGui::TextCenter("Noise Initial Amplitude");
						ImGui::SliderFloat("##FractalInitAmplitude", &ggterrain_global_params.fractal_initial_amplitude, 0.0f, 2.0f, "%.6f", 2.0f);

						// how the noise frequency changes with increasing detail level
						ImGui::TextCenter("Noise Frequency Change");
						ImGui::SliderFloat("##FractalFreqInc", &ggterrain_global_params.fractal_freq_increase, 0.01f, 8.0f, "%.3f", 2.0f);

						// how much the noise weight changes with increasing detail level
						ImGui::TextCenter("Noise Amplitude Change");
						ImGui::SliderFloat("##FractalFreqWeight", &ggterrain_global_params.fractal_freq_weight, 0.01f, 2.0f, "%.3f", 2.0f);

						ImGui::TextCenter("Heightmap Roughness");
						ImGui::SliderFloat("##HeightmapRougness", &ggterrain_global_params.heightmap_roughness, 0.0f, 2.0f, "%.6f", 2.0f);
					}; break;

					case 1: // sculpt
					{
						ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_SCULPT;

						if (pref.iTerrainAdvanced)
						{
							ImGui::Text("Show Editable Area");
							ImGui::SameLine();
							bool showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE) != 0;
							if (ImGui::Checkbox("##MaskVisible", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
							}
							ImGui::SameLine();
							ImGui::Text(" 3D");
							ImGui::SameLine();
							showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D) != 0;
							if (ImGui::Checkbox("##MaskVisible3D", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
							}
						}
						ImGui::Text("Brush Visible");
						ImGui::SameLine();
						bool showBrush = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE) != 0;
						if ( ImGui::Checkbox( "##BrushVisible", &showBrush ) )
						{
							if ( showBrush ) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
							else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
						}

						ImGui::TextCenter("Brush Size");
						ImGui::SliderFloat("##Brush Size", &ggterrain_global_render_params2.brushSize, 25.0f, 7000.0f, "%.1f", 2.0f);

						ImGui::Text("Pick Using Plane");
						ImGui::SameLine();
						bool pickPlane = ggterrain_extra_params.edit_pick_mode != 0;
						if ( ImGui::Checkbox( "##PickPlane", &pickPlane ) )
						{
							ggterrain_extra_params.edit_pick_mode = pickPlane ? 1 : 0;
						}

						ImGui::TextCenter("Sculpt Speed");
						ImGui::SliderFloat("##Sculpt Speed", &ggterrain_extra_params.sculpt_speed, 1.0f, 200.0f, "%.1f", 2.0f);

						ImGui::TextCenter("Sculpt Mode");
						ImGui::SliderInt("##Sculpt Mode", &ggterrain_extra_params.sculpt_mode, 0, 9);

						ImGui::TextCenter("Chosen Sculpt Height");
						meterValue = GGTerrain_UnitsToMeters( ggterrain_extra_params.sculpt_chosen_height );
						if ( ImGui::SliderFloat("##ChosenSculptHeight", &meterValue, -2000.0f, 5000.0f) ) 
						{
							ggterrain_extra_params.sculpt_chosen_height = GGTerrain_MetersToUnits( meterValue );
						}
					}; break;

					case 2: // paint
					{
						if (pref.iTerrainAdvanced)
						{
							ImGui::Text("Show Editable Area");
							ImGui::SameLine();
							bool showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE) != 0;
							if (ImGui::Checkbox("##EditBoxVisible", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
							}
							ImGui::SameLine();
							ImGui::Text(" 3D");
							ImGui::SameLine();
							showMapSize = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D) != 0;
							if (ImGui::Checkbox("##EditBoxVisible3D", &showMapSize))
							{
								if (showMapSize) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
							}
						}
						ImGui::Text("Material Map Visible");
						ImGui::SameLine();
						bool showMask = (ggterrain_global_render_params.flags & GGTERRAIN_SHADER_FLAG_SHOW_MAT_MAP) != 0;
						if ( ImGui::Checkbox( "##MatMapVisible", &showMask ) )
						{
							if ( showMask ) ggterrain_global_render_params.flags |= GGTERRAIN_SHADER_FLAG_SHOW_MAT_MAP;
							else ggterrain_global_render_params.flags &= ~GGTERRAIN_SHADER_FLAG_SHOW_MAT_MAP;
						}

						ImGui::Text("Mask Visible");
						ImGui::SameLine();
						bool showMatMap = (ggterrain_global_render_params.flags & GGTERRAIN_SHADER_FLAG_SHOW_MASK) != 0;
						if ( ImGui::Checkbox( "##MaskVisible", &showMatMap ) )
						{
							if ( showMatMap ) ggterrain_global_render_params.flags |= GGTERRAIN_SHADER_FLAG_SHOW_MASK;
							else ggterrain_global_render_params.flags &= ~GGTERRAIN_SHADER_FLAG_SHOW_MASK;
						}

						ImGui::TextCenter("Mask Scale");
						ImGui::SliderFloat("##MaskScale", &ggterrain_global_render_params.maskScale, 0.01f, 20.0f, "%.3f", 2.0f);

						ImGui::TextCenter("Detail Scale");
						ImGui::SliderFloat("##DetailScale", &ggterrain_global_render_params2.detailScale, 0.1f, 1.0f);

						ImGui::TextCenter("Texture Tiling Fallof");
						ImGui::SliderFloat("##TextureTiling", &ggterrain_global_render_params.tilingPower, 0.15f, 1.0f);

						ImGui::Text("Brush Visible");
						ImGui::SameLine();
						bool showBrush = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE) != 0;
						if ( ImGui::Checkbox( "##BrushVisible", &showBrush ) )
						{
							if ( showBrush ) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
							else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
						}

						ImGui::TextCenter("Brush Size");
						ImGui::SliderFloat("##Brush Size", &ggterrain_global_render_params2.brushSize, 25.0f, 7000.0f, "%.1f", 2.0f);

						ImGui::Text("Pick Using Plane");
						ImGui::SameLine();
						bool pickPlane = ggterrain_extra_params.edit_pick_mode != 0;
						if ( ImGui::Checkbox( "##PickPlane", &pickPlane ) )
						{
							ggterrain_extra_params.edit_pick_mode = pickPlane ? 1 : 0;
						}

						ImGui::TextCenter("Edit Mode");
						ImGui::SliderInt("##EditMode", &ggterrain_extra_params.edit_mode, 0, 6);

						if ( ggterrain_extra_params.edit_mode == GGTERRAIN_EDIT_PAINT )
						{
							ImGui::TextCenter("Paint Material");
							ImGui::SliderInt("##PaintMaterial", &ggterrain_extra_params.paint_material, 0, 32);
						}

						if ( ggterrain_extra_params.edit_mode == GGTERRAIN_EDIT_TREES )
						{
							ImGui::TextCenter("Tree LOD Distance");
							ImGui::SliderFloat("##TreeLODDist", &ggtrees_global_params.lod_dist, 750, 7000);

							// Tree shadow controls MOVED to the Visuals > Shadows panel
							// (M-GridEditB_part24, 2026-07-28 user request) — live-tunable in test game.

							ImGui::TextCenter("Tree Paint Mode");
							ImGui::SliderInt("##TreePaintMode", &ggtrees_global_params.paint_mode, 0, 5);

							ImGui::TextCenter("Tree Selection");
							uint64_t values = ggtrees_global_params.paint_tree_bitfield;
							for( uint32_t i = 0; i < GGTrees_GetNumTypes(); i++ )
							{
								uint64_t mask = 1ULL << i;
								bool selected = ((values & mask) != 0);

								char str[ 32 ];
								sprintf( str, "##TreeSelection%d", i );
								if ( ImGui::Checkbox(str, &selected ) )
								{
									if ( selected ) values |= mask;
									else values &= ~mask;
								}
								if ( (i+1) % 8 != 0 && i != GGTrees_GetNumTypes()-1 ) ImGui::SameLine();
							}
							ggtrees_global_params.paint_tree_bitfield = values;
							
							ImGui::TextCenter("Tree Density");
							ImGui::SliderInt("##TreeDensity", &ggtrees_global_params.paint_density, 0, 100);

							ImGui::TextCenter("Tree Water Distance");
							if ( ImGui::SliderFloat("##TreeWaterDist", &ggtrees_global_params.water_dist, -500.0f, 2000.0f, "%.1f", 2.0f) )
							{
								ggterrain_extra_params.iUpdateTrees = 1;
							}
						}

						if ( ggterrain_extra_params.edit_mode == GGTERRAIN_EDIT_GRASS )
						{
							ImGui::TextCenter("Grass Distance");
							ImGui::SliderFloat("##GrassDist", &gggrass_global_params.lod_dist, 750, 6000);

							ImGui::TextCenter("Grass Max Height");
							if ( ImGui::SliderFloat("##GrassMaxHeight", &gggrass_global_params.max_height, -1000, 40000, "%.3f", 2.0f) )
							{
								ggterrain_extra_params.iUpdateGrass = 2;
							}

							ImGui::TextCenter("Grass Min Height");
							if ( ImGui::SliderFloat("##GrassMinHeight", &gggrass_global_params.min_height, -1000, 40000, "%.3f", 2.0f) )
							{
								ggterrain_extra_params.iUpdateGrass = 2;
							}

							ImGui::TextCenter("Grass Max Height (Underwater)");
							if ( ImGui::SliderFloat("##GrassMaxHeightWater", &gggrass_global_params.max_height_underwater, -10000, 1000, "%.3f", 2.0f) )
							{
								ggterrain_extra_params.iUpdateGrass = 2;
							}

							ImGui::TextCenter("Grass Min Height (Underwater)");
							if ( ImGui::SliderFloat("##GrassMinHeightWater", &gggrass_global_params.min_height_underwater, -10000, 1000, "%.3f", 2.0f) )
							{
								ggterrain_extra_params.iUpdateGrass = 2;
							}

							ImGui::TextCenter("Grass Paint Material (0=auto)");
							ImGui::SliderInt("##GrassPaintMat", &gggrass_global_params.paint_material, 0, 32);

							ImGui::TextCenter("Grass Selection");
							uint64_t values = gggrass_global_params.paint_type;
							for( uint32_t i = 0; i < GGGRASS_NUM_SELECTABLE_TYPES; i++ )
							{
								uint64_t mask = 1ULL << i;
								bool selected = ((values & mask) != 0);

								char str[ 32 ];
								sprintf( str, "##GrassSelection1%d", i );
								if ( ImGui::Checkbox(str, &selected ) )
								{
									if ( selected ) values |= mask;
									else values &= ~mask;
								}
								if ( (i+1) % 8 != 0 && i != GGGRASS_NUM_SELECTABLE_TYPES-1 ) ImGui::SameLine();
							}

							gggrass_global_params.paint_type = values;
							
							ImGui::TextCenter("Grass Paint Mode");
							ImGui::SliderInt("##GrassPaintMode", (int*)&gggrass_global_params.paint_mode, 0, 2 );
							
							ImGui::TextCenter("Grass Density");
							ImGui::SliderInt("##GrassDensity", &gggrass_global_params.paint_density, 0, 100);
						}

						if ( ggterrain_extra_params.edit_mode == 5 || ggterrain_extra_params.edit_mode == 6 )
						{
							ImGui::TextCenter("Flat Area Angle");
							ImGui::SliderFloat("##FlatAreaAngle", &ggterrain_extra_params.flat_area_angle, 0, 360.0f, "%.1f", 1.0f);

							if ( ImGui::Button( "Reset Flat Areas" ) )
							{
								timestampactivity(0, "GGTerrain_RemoveAllFlatAreas:2");
								GGTerrain_RemoveAllFlatAreas();
							}
						}

						ImGui::TextCenter("Base Matrial");

						float fSizeX = ImGui::GetContentRegionAvailWidth() - 35.0f;
						ImGui::PushItemWidth( fSizeX );

						int index = ggterrain_global_render_params.baseLayerMaterial & 0xFF;
						bool rotate = (ggterrain_global_render_params.baseLayerMaterial >> 8) != 0;
						ImGui::SliderInt( "##LayerBaseIndex", &index, 0, GGTERRAIN_MAX_SOURCE_TEXTURES-1 ); 
						ImGui::SameLine();
						ImGui::Checkbox( "##LayerBaseRotate", &rotate );
						ggterrain_global_render_params.baseLayerMaterial = (rotate ? 0x100 : 0) | index;

						char layerName[ 64 ];
						ImGui::TextCenter("Layer Matrials");
						for( int i = 0; i < 5; i++ )
						{
							int index = ggterrain_global_render_params.layerMatIndex[i] & 0xFF;
							bool rotate = (ggterrain_global_render_params.layerMatIndex[i] >> 8) != 0;
							
							sprintf_s( layerName, "##LayerIndex%d", i );
							ImGui::SliderInt( layerName, &index, 0, GGTERRAIN_MAX_SOURCE_TEXTURES-1 ); 
							ImGui::SameLine();
							sprintf_s( layerName, "##LayerRotate%d", i );
							ImGui::Checkbox( layerName, &rotate );
														
							ggterrain_global_render_params.layerMatIndex[i] = (rotate ? 0x100 : 0) | index;
						}

						ImGui::PopItemWidth();

						fSizeX = ImGui::GetContentRegionAvailWidth() / 2.0;
						ImGui::PushItemWidth( fSizeX );

						ImGui::TextCenter("Layer Height Ranges");
						for( int i = 0; i < 5; i++ )
						{
							sprintf_s( layerName, "##LayerHeightStart%d", i );
							meterValue = GGTerrain_UnitsToMeters( ggterrain_global_render_params.layerStartHeight[i] );
							if ( ImGui::SliderFloat(layerName, &meterValue, -100, 1000, "%.3f", 2.0f ) )
							{
								ggterrain_global_render_params.layerStartHeight[i] = GGTerrain_MetersToUnits( meterValue );
							}
							ImGui::SameLine();
							sprintf_s( layerName, "##LayerHeightEnd%d", i );
							meterValue = GGTerrain_UnitsToMeters( ggterrain_global_render_params.layerEndHeight[i] );
							if ( ImGui::SliderFloat(layerName, &meterValue, -100, 1000, "%.3f", 2.0f ) )
							{
								ggterrain_global_render_params.layerEndHeight[i] = GGTerrain_MetersToUnits( meterValue );
							}
							if ( ggterrain_global_render_params.layerEndHeight[i] < ggterrain_global_render_params.layerStartHeight[i] )
							{
								ggterrain_global_render_params.layerEndHeight[i] = ggterrain_global_render_params.layerStartHeight[i];
							}
						}

						ImGui::PopItemWidth();

						fSizeX = ImGui::GetContentRegionAvailWidth() - 35.0f;
						ImGui::PushItemWidth( fSizeX );
						
						ImGui::TextCenter("Slope Materials");
						for( int i = 0; i < 2; i++ )
						{
							int index = ggterrain_global_render_params.slopeMatIndex[i] & 0xFF;
							bool rotate = (ggterrain_global_render_params.slopeMatIndex[i] >> 8) != 0;

							sprintf_s( layerName, "##SlopeIndex%d", i );
							ImGui::SliderInt( layerName, &index, 0, GGTERRAIN_MAX_SOURCE_TEXTURES-1 ); 
							ImGui::SameLine();
							sprintf_s( layerName, "##SlopeRotate%d", i );
							ImGui::Checkbox( layerName, &rotate );
														
							ggterrain_global_render_params.slopeMatIndex[i] = (rotate ? 0x100 : 0) | index;
						}

						ImGui::PopItemWidth();

						ImGui::TextCenter("Slope Steepness");
						ImGui::RangeSlider("##SlopeRange0", ggterrain_global_render_params.slopeStart[0], ggterrain_global_render_params.slopeEnd[0], 1.0f);
						ImGui::RangeSlider("##SlopeRange1", ggterrain_global_render_params.slopeStart[1], ggterrain_global_render_params.slopeEnd[1], 1.0f);

						ImGui::TextCenter("Bumpiness");
						ImGui::SliderFloat("##Bumpiness", &ggterrain_global_render_params.bumpiness, 0.0f, 2.0f );

						ImGui::TextCenter("Non-Metal Reflectance");
						ImGui::SliderFloat("##Reflectance", &ggterrain_global_render_params2.reflectance, 0.0f, 0.16f );

						ImGui::TextCenter("Texture Gamma");
						ImGui::SliderFloat("##TextureGamma", &ggterrain_global_render_params2.textureGamma, 0.2f, 5.0f );
					}; break;
				}
				

				ImGui::Separator();
				ImGui::TextCenter( "Pages Active: %d", GGTerrain_GetPagesActive() );
				ImGui::TextCenter( "Pages Needed: %d", GGTerrain_GetPagesNeeded() );
				ImGui::TextCenter( "Pages Refresh Needed: %d", GGTerrain_GetPagesRefreshNeeded() );
				ImGui::TextCenter( "Camera: %.1f, %.1f, %.1f", CameraPositionX(), CameraPositionY(), CameraPositionZ() );

				ImGui::PopItemWidth();

			}

			if (current_mode == TOOL_PAINTTEXTURE)
				imgui_Customize_Terrain(0);
			if (current_mode == TOOL_PAINTGRASS)
				imgui_Customize_Vegetation(0);

			if (!pref.bHideTutorials)
			{
#ifndef REMOVED_EARLYACCESS
				if (ImGui::StyleCollapsingHeader("Tutorial (this feature is incomplete)", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent(10);
					char* my_combo_itemsp[] = { NULL,NULL,NULL };
					int my_combo_items = 0;
					int iVideoSection = 0;
					cstr cShowTutorial = "02 - Creating terrain";
					if (current_mode == TOOL_PAINTTEXTURE) {
						my_combo_itemsp[0] = "50 - Painting terrain";
						my_combo_itemsp[1] = "02 - Creating terrain";
						my_combo_itemsp[2] = "03 - Add character and set a path";
						my_combo_items = 3;
						cShowTutorial = "50 - Painting terrain";
						iVideoSection = SECTION_PAINT_TERRAIN;
					}
					else if (current_mode == TOOL_PAINTGRASS) {
						my_combo_itemsp[0] = "01 - Getting started";
						my_combo_itemsp[1] = "02 - Creating terrain";
						my_combo_itemsp[2] = "03 - Add character and set a path";
						my_combo_items = 3;
						cShowTutorial = "02 - Creating terrain";
						iVideoSection = SECTION_ADD_VEGETATION;
					}
					else // TOOL_SHAPE,TOOL_LEVELMODE ...
					{
						my_combo_itemsp[0] = "02 - Creating terrain";
						my_combo_itemsp[1] = "01 - Getting started";
						my_combo_itemsp[2] = "03 - Add character and set a path";
						my_combo_items = 3;
						cShowTutorial = "02 - Creating terrain";
						iVideoSection = SECTION_SCULPT_TERRAIN;
					}

					SmallTutorialVideo(cShowTutorial.Get(), my_combo_itemsp, my_combo_items, iVideoSection);
					float but_gadget_size = ImGui::GetFontSize()*12.0;
					float w = ImGui::GetWindowContentRegionWidth() - 10.0;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
					#ifdef INCLUDESTEPBYSTEP
					if (ImGui::StyleButton("View Step by Step Tutorial", ImVec2(but_gadget_size, 0)))
					{
						bHelp_Window = true;
						bHelpVideo_Window = true;
						extern bool bSetTutorialSectionLeft;
						bSetTutorialSectionLeft = false;
						strcpy(cForceTutorialName, cShowTutorial.Get());
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start Step by Step Tutorial");
					#endif

					ImGui::Indent(-10);
				}
#endif
			}

			// insert a keyboard shortcut component into panel
			eKeyboardShortcutType KST = eKST_Sculpt;
			if (current_mode == TOOL_PAINTTEXTURE)
				KST = eKST_Paint;
			else if (current_mode == TOOL_PAINTGRASS)
				KST = eKST_AddVeg;
			UniversalKeyboardShortcut(KST);

			ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
			if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
			{
				bImGuiGotFocus = true;
			}

			void CheckMinimumDockSpaceSize(float minsize);
			CheckMinimumDockSpaceSize(250.0f);

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

void reset_terrain_paint_date( void )
{
	uint32_t size = GGTerrain_GetPaintDataSize();

	uint8_t* pNewMap = new uint8_t[size];
	if (pNewMap)
	{
		memset(pNewMap, 0, size);
		GGTerrain_SetPaintData(size, pNewMap);
		delete [] pNewMap;
		// SetPaintData above already triggered the Wicked repaint (OnPaintDataChanged);
		// heights are untouched, so skip the bridge — a full Wicked mesh regen here is waste
		GGTerrain::GGTerrain_InvalidateRegion(-1000000.0, -1000000.0, 1000000.0, 1000000.0, GGTERRAIN_INVALIDATE_ALL | GGTERRAIN_INVALIDATE_NO_WICKED);
		extern int iTriggerInvalidateAfterFrames;
		iTriggerInvalidateAfterFrames = 20;
	}
}

void clear_highlighted_tree(void)
{
	GGTrees_DeselectHighlightedTree();
}

void set_terrain_edit_mode(int mode)
{
	ggterrain_extra_params.edit_mode = mode;
	if (mode == 0) ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
}

void set_terrain_sculpt_mode(int mode)
{
	ggterrain_extra_params.sculpt_mode = mode;
	if(mode == 0) ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
}

int get_terrain_sculpt_mode( void )
{
	return ggterrain_extra_params.sculpt_mode;
}

