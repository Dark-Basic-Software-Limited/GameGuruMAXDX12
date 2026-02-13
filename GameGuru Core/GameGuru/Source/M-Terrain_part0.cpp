//----------------------------------------------------
//--- GAMEGURU - M-Terrain
//----------------------------------------------------
#include "stdafx.h"
#include "gameguru.h"
#include "..\..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\Objects\ShadowMapping\cShadowMaps.h"
#include "wincodec.h"

//PE: GameGuru IMGUI.
#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

#include "openxr.h"


// Prototypes
void set_inputsys_mclick(int value);

using namespace DirectX;

#define DYNAMICSUNPOSITION

// shadow mapping
extern CascadedShadowsManager g_CascadedShadow;
extern int g_iTerrainIDForShadowMap;

extern bool bHelp_Window;
extern bool bHelpVideo_Window;
extern char cForceTutorialName[1024];

// Terrain Build Globals
#define TERRAINTEXPANELSPRMAX 6

// Tried switching between texture_D.dds and texture_D.jpg but JPG just as large, lower quality and loses a channel!
#define TEXTURE_D_NAME "texture_D.dds"
#define TEXTURE_N_NAME "texture_N.dds"

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

// moved from below so Classic could compile
bool bUpdateVeg = true; //Update veg on by default.


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

//bool bUpdateVeg = true; //Update veg on by default.

bool bEnableWeather = false;
int iLastUpdateVeg = 0;
extern bool bTutorialCheckAction;
bool TutorialNextAction(void);
bool CheckTutorialPlaceit(void);
bool CheckTutorialAction(const char * action, float x_adder = 0.0f);

int iTerrainRaiseMode = 1;
int iLastRegionUpdateX, iLastRegionUpdateY;
int iTerrainVegLoopUpdate = 0;
extern bool bImGuiGotFocus;
//bool bVegHasChanged = false;

// moved here for Classic to compile
int iTerrainPaintMode = 1;
bool bVegHasChanged = false;

void terrain_initstyles ( void )
{
	// Init terrain work bitmap (so save level does not crash due to memory creation)
	if ( BitmapExist(g.terrainworkbitmapindex) == 0 ) 
	{
		// bitmap to perform pixel work
		CreateBitmap (  g.terrainworkbitmapindex,MAXTEXTURESIZE,MAXTEXTURESIZE );
		SetCurrentBitmap (  0 );

		// create array used later for creating terrain super texture
		//Dim3( t.pot, 5, 3, 3  ); // 280317 - 16 textures per terrain
		Dim3( t.pot, 16, 3, 3 );
	}

	//  Set occlusion style (hardware assisted)
	SetOcclusionMode (  1 );

	//  Choose terrain style
	g.terrainstylemax=0;
	g.terrainstyle_s="common";
	SetDir (  "terrainbank" );
	ChecklistForFiles (  );

	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		// reduce large custom files needed for FPM and Level Cloud (faster down/loading)
		#ifdef ENABLECUSTOMTERRAIN
		 if (t.c == 1)
		 {
			// first one is CUSTOM to select custom terrain texture from terrain panel
			++g.terrainstylemax;
			Dim(t.terrainstylebank_s, g.terrainstylemax);
			t.terrainstylebank_s[g.terrainstylemax] = "CUSTOM";
		 }
		#endif
		t.tfile_s=ChecklistString(t.c);
		if (  ChecklistValueA(t.c) == 1 ) 
		{
			if (  t.tfile_s.Get()[0] != '.' ) 
			{
				++g.terrainstylemax;
				Dim (  t.terrainstylebank_s,g.terrainstylemax  );
				t.terrainstylebank_s[g.terrainstylemax]=Lower(t.tfile_s.Get());
			}
		}
	}
	SetDir (  ".." );

	//  small lookup for memblock painting circles
	Dim ( t.curve_f,100 );
	for ( t.r = 0 ; t.r <= 180; t.r++ )
	{
		t.trx_f = Cos(t.r-90)*100.0;
		t.trz_f = Sin(t.r-90)*100.0;
		t.curve_f [ int((100+t.trz_f)/2) ] = t.trx_f/100.0;
	}

	//  Choose default terrin and veg styles (also called when new level)
	terrain_initstyles_reset ( );
}

void terrain_initstyles_reset ( void )
{
	//  style file chooses default style to use
	t.tfile_s="terrainbank\\style.txt";
	if (  FileExist(t.tfile_s.Get()) == 1 ) 
	{
		OpenToRead (  1,t.tfile_s.Get() );
		g.terrainstyle_s = ReadString ( 1 );
		CloseFile (  1 );
	}
	if (  PathExist( cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() ) == 0 ) 
	{
		g.terrainstyle_s=t.terrainstylebank_s[1];
	}
	//  find terrainstyle index
	for ( g.terrainstyleindex = 1 ; g.terrainstyleindex<=  g.terrainstylemax; g.terrainstyleindex++ )
	{
		if (  cstr(Lower(g.terrainstyle_s.Get())) == t.terrainstylebank_s[g.terrainstyleindex] ) 
		{
			break;
		}
	}
	// 080517 - if exceed array (i.e not found) reset to last slot
	if ( g.terrainstyleindex > g.terrainstylemax )
	{
		g.terrainstyleindex = g.terrainstylemax;
		g.terrainstyle_s = t.terrainstylebank_s[g.terrainstyleindex];
	}

	// terrain starts with no grass initialised
	g.vegstyle_s = "";
	g.vegstyleindex = 0;

	/* now done in grass_initstyles!
	//  Choose default veg style
	#ifdef WICKEDENGINE
	g.vegstyle_s = ""; // relies on grass_color texture for new multi-grass system
	g.vegstyleindex = 0;
	#else
	#ifdef VRTECH
	g.vegstyle_s = "weedy 01"; //PE: hanged from: "lushy";
	#else
	g.vegstyle_s="lushy";
	#endif
	if ( PathExist( cstr(cstr("vegbank\\")+g.vegstyle_s).Get() ) == 0 ) 
	{
		g.vegstyle_s=t.vegstylebank_s[1];
	}
	//  find vegstyle index
	for ( g.vegstyleindex = 1 ; g.vegstyleindex<=  g.vegstylemax; g.vegstyleindex++ )
	{
		if ( cstr(Lower(g.vegstyle_s.Get())) == t.vegstylebank_s[g.vegstyleindex] ) 
		{
			break;
		}
	}
	// 080517 - if exceed array (i.e not found) reset to last slot
	if ( g.vegstyleindex > g.vegstylemax )
	{
		g.vegstyleindex = g.vegstylemax;
		g.vegstyle_s = t.vegstylebank_s[g.vegstyleindex];
	}
	#endif
	*/
}

void terrain_setupedit ( void )
{
	// Water plane (use proper reflect/refract later)
	//LoadImage (  "effectbank\\reloaded\\media\\water.png",t.terrain.imagestartindex+0,0,g.gdividetexturesize );
	LoadImage (  "effectbank\\reloaded\\media\\water.dds",t.terrain.imagestartindex+0,0,g.gdividetexturesize );
	MakeObjectPlane (  t.terrain.objectstartindex+2,200000,200000 );
	XRotateObject (  t.terrain.objectstartindex+2,90 );
	SetObjectLight (  t.terrain.objectstartindex+2,0 );
	SetObjectCull (  t.terrain.objectstartindex+2,0 );
	TextureObject (  t.terrain.objectstartindex+2,t.terrain.imagestartindex+0 );
	ScaleObjectTexture (  t.terrain.objectstartindex+2,1000,1000 );
	SetAlphaMappingOn (  t.terrain.objectstartindex+2,50 );

	// Water Effect Shader earlier
	if ( GetEffectExist ( t.terrain.effectstartindex+1 ) == 0 )
	{
		LoadEffect ( "effectbank\\reloaded\\water_basic.fx",t.terrain.effectstartindex+1,0 );
		t.effectparam.water.HudFogDist=GetEffectParameterIndex(t.terrain.effectstartindex+1,"HudFogDist");
		t.effectparam.water.HudFogColor=GetEffectParameterIndex(t.terrain.effectstartindex+1,"HudFogColor");
	}
	SetObjectEffect ( t.terrain.objectstartindex+2, t.terrain.effectstartindex+1 );

	SetObjectTransparency(t.terrain.objectstartindex+2, 5); //PE: same mode as test game.

	SetEffectTechnique ( t.terrain.effectstartindex+1, "Editor" );

	// Terrain Edit Settings
	t.terrain.zoom_f=0.4f;
	t.terrain.camx_f=(1024*50)/2;
	t.terrain.camz_f=(1024*50)/2;
	t.terrain.terrainpaintermode=1;
	t.terrain.lastterrainpaintermode=1;
	t.terrain.terrainlevel_f=750.0f;
	t.terrain.RADIUS_f=280.0f; //PE: old default 250.0f
	SetCameraRange (  100,55000 );

	// Reset undo buffer
	t.terrainundo.bufferfilled=0;
	t.terrainundo.mode=0;

	// Initial quick test player position
	t.terrain.playerx_f=25000;
	t.terrain.playerz_f=25000;
	t.terrain.playerax_f=0;
	t.terrain.playeray_f=0;
	t.terrain.playeraz_f=0;
	t.camangy_f=0;
}

cstr terrain_getterrainfolder ( void )
{
	g.terrainstyle_s = t.terrainstylebank_s[g.terrainstyleindex];
	cstr sTerrainTextureLocation = cstr(cstr("terrainbank\\")+g.terrainstyle_s);
	#ifdef ENABLECUSTOMTERRAIN
	if ( g.terrainstyleindex == 1 ) sTerrainTextureLocation = g.mysystem.levelBankTestMap_s.Get(); //"levelbank\\testmap";
	#endif
	return sTerrainTextureLocation;
}

void terrain_paintselector_init ( void )
{
	// quit early if in F9 editing mode
	if ( t.conkit.editmodeactive != 0 )  
		return;

	// Create texture selection panel
	terrainbuild.iTexPlateImage = 0;
	terrainbuild.iTexturePanelX = GetChildWindowWidth()-210;
	terrainbuild.iTexturePanelY = GetChildWindowHeight()-210;
	terrainbuild.iTexturePanelWidth = 200;
	terrainbuild.iTexturePanelHeight = 200;
	for ( int iTex = 0; iTex < TERRAINTEXPANELSPRMAX; iTex++ )
	{
		LPSTR pTexImg = "";
		int iX = terrainbuild.iTexturePanelX;
		int iY = terrainbuild.iTexturePanelY;
		int iWidth = terrainbuild.iTexturePanelWidth;
		int iHeight = terrainbuild.iTexturePanelHeight;
		if ( iTex==0 ) { pTexImg = "TexHUD-F.png"; iX -= 10; iY -= 10; iWidth += 20; iHeight += 20; }
		if ( iTex==1 ) { pTexImg = "TexHUD-L.png"; iX -= 10; iY -= 10; iWidth += 20; iHeight = 1; }
		if ( iTex==2 ) { pTexImg = "TexHUD-L.png"; iX -= 10; iY += 209; iWidth += 20; iHeight = 1; }
		if ( iTex==3 ) { pTexImg = "TexHUD-L.png"; iX -= 10; iY -= 10; iWidth = 1; iHeight += 20; }
		if ( iTex==4 ) { pTexImg = "TexHUD-L.png"; iX += 209; iY -= 10; iWidth = 1; iHeight += 20; }
		if ( iTex==5 ) 
		{ 
			// terrain texture plate
			cstr sTerrainTextureLocation = terrain_getterrainfolder();
			terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(sTerrainTextureLocation+"\\"+TEXTURE_D_NAME).Get());
			if ( terrainbuild.iTexturePanelImg[iTex] == 0 )
			{
				terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(sTerrainTextureLocation+"\\texture_D.dds").Get());
				if ( terrainbuild.iTexturePanelImg[iTex] == 0 )
				{
					// means the terrain texture is missing, report this and switch to default to avoid crash
					terrain_initstyles_reset();
					grass_initstyles_reset();
					sTerrainTextureLocation = terrain_getterrainfolder();
					terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(sTerrainTextureLocation+"\\"+TEXTURE_D_NAME).Get());
				}
			}
			terrainbuild.terrainstyle = g.terrainstyle_s;
		}
		else
		{
			// UI graphics
			terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(cstr("terrainbuild\\default\\")+cstr(pTexImg)).Get());
		}
		terrainbuild.iTexturePanelSprite[iTex] = g.terrainpainterinterfacesprite + 31 + iTex;
		if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iTexturePanelSprite[iTex], iX, iY, terrainbuild.iTexturePanelImg[iTex] );
		if (!bDisableAllTerrainSprites) SizeSprite ( terrainbuild.iTexturePanelSprite[iTex], iWidth, iHeight );
		if ( iTex==5 ) 
		{
			terrainbuild.iTexPlateImage = terrainbuild.iTexturePanelImg[iTex];
			if (!bDisableAllTerrainSprites) SetSprite ( terrainbuild.iTexturePanelSprite[iTex], 0, 0 );
		}
	}

	// Texture highlighter
	terrainbuild.iTexturePanelHighSprite = g.terrainpainterinterfacesprite + 0;
	terrainbuild.iTexturePanelHighImg = loadinternalimage("terrainbuild\\default\\TextureHighlighter.dds");
	if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iTexturePanelHighSprite, terrainbuild.iTexturePanelX, terrainbuild.iTexturePanelY, terrainbuild.iTexturePanelHighImg );
	if (!bDisableAllTerrainSprites) SizeSprite ( terrainbuild.iTexturePanelHighSprite, terrainbuild.iTexturePanelWidth/4, terrainbuild.iTexturePanelHeight/4 );

	// Help Dialog Shortcut Keys
	terrainbuild.iHelpSpr = g.terrainpainterinterfacesprite + 1;
	terrainbuild.iHelpImg = loadinternalimage("languagebank\\english\\artwork\\terrainbuild-help.png");
	if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iHelpSpr, terrainbuild.iTexturePanelX - ImageWidth(terrainbuild.iHelpImg) - 10, terrainbuild.iTexturePanelY + 210 - ImageHeight(terrainbuild.iHelpImg), terrainbuild.iHelpImg );

	// Help Dialog Shortcut Keys
	terrainbuild.iTexHelpSpr = g.terrainpainterinterfacesprite + 2;
 	terrainbuild.iTexHelpImg = loadinternalimage("languagebank\\english\\artwork\\terrainbuild-texturehelp.png");
	if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iTexHelpSpr, terrainbuild.iTexturePanelX - 10, terrainbuild.iTexturePanelY - 10 - ImageHeight(terrainbuild.iTexHelpImg), terrainbuild.iTexHelpImg );

	// terrain paint selector inited
	terrainbuild.initialised = 1;
	terrainbuild.active = 1;
}

void terrain_paintselector_hide ( void )
{
	if ( terrainbuild.active == 1 )
	{
		// hide any UI elements
		if (!bDisableAllTerrainSprites) 
		{
			if (terrainbuild.iTexturePanelSprite[0] > 0)
			{
				if (SpriteExist(terrainbuild.iTexHelpSpr) == 1) HideSprite(terrainbuild.iTexHelpSpr);
				if (SpriteExist(terrainbuild.iHelpSpr) == 1) HideSprite(terrainbuild.iHelpSpr);
				if (SpriteExist(terrainbuild.iTexturePanelHighSprite) == 1) HideSprite(terrainbuild.iTexturePanelHighSprite);
				for (int iTex = 0; iTex < TERRAINTEXPANELSPRMAX; iTex++)
				{
					if (SpriteExist(terrainbuild.iTexturePanelSprite[iTex]) == 1) HideSprite(terrainbuild.iTexturePanelSprite[iTex]);
				}
			}
		}
		terrainbuild.active = 0;
	}
}

void terrain_resetfornewlevel ( void )
{
	terrainbuild.terrainstyle = "";
}

void terrain_paintselector_show ( void )
{
	// quit early if in F9 editing mode
	if ( t.conkit.editmodeactive != 0 )  
		return;

	// if switch from terrain paint to grass paint, hide and reshow (grass does not need texture panel)
	if (!bDisableAllTerrainSprites) {
		if (SpriteExist(terrainbuild.iTexHelpSpr) == 1)
		{
			bool bSwitchFromToPaintModes = false;
			if (t.terrain.terrainpaintermode == 10 && SpriteVisible(terrainbuild.iTexHelpSpr) == 1) bSwitchFromToPaintModes = true;
			if (t.terrain.terrainpaintermode != 10 && SpriteVisible(terrainbuild.iTexHelpSpr) == 0) bSwitchFromToPaintModes = true;
			if (bSwitchFromToPaintModes == true)
			{
				// allows show code below to set correct sprites to visible
				terrain_paintselector_hide();
			}
		}
	}

	if ( terrainbuild.active == 0 )
	{
		// show UI elements
		if (!bDisableAllTerrainSprites) 
		{
			if (terrainbuild.iTexturePanelSprite[0] > 0)
			{
				if (SpriteExist(terrainbuild.iHelpSpr) == 1) ShowSprite(terrainbuild.iHelpSpr);
				if (t.terrain.terrainpaintermode != 10)
				{
					if (SpriteExist(terrainbuild.iTexHelpSpr) == 1) ShowSprite(terrainbuild.iTexHelpSpr);
					if (SpriteExist(terrainbuild.iTexturePanelHighSprite) == 1) ShowSprite(terrainbuild.iTexturePanelHighSprite);
					for (int iTex = 0; iTex < TERRAINTEXPANELSPRMAX; iTex++)
					{
						if (SpriteExist(terrainbuild.iTexturePanelSprite[iTex]) == 1) ShowSprite(terrainbuild.iTexturePanelSprite[iTex]);
					}
				}
			}
		}
		terrainbuild.active = 1;

		// if terrain style changed, load correct terrain texture into UI
		cstr sTerrainTextureLocation = terrain_getterrainfolder();
		if ( strcmp ( terrainbuild.terrainstyle.Get(), g.terrainstyle_s.Get() ) != NULL )
		{
			terrainbuild.terrainstyle = g.terrainstyle_s;
			if ( FileExist ( cstr(sTerrainTextureLocation+"\\"+TEXTURE_D_NAME).Get() ) == 1 )
				LoadImage ( cstr(sTerrainTextureLocation+"\\"+TEXTURE_D_NAME).Get(), terrainbuild.iTexPlateImage );
			else
				LoadImage ( cstr(sTerrainTextureLocation+"\\texture_D.dds").Get(), terrainbuild.iTexPlateImage );
			int iX = terrainbuild.iTexturePanelX;
			int iY = terrainbuild.iTexturePanelY;
			if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iTexturePanelSprite[5], iX, iY, terrainbuild.iTexturePanelImg[5] );
			if (!bDisableAllTerrainSprites) SetSprite ( terrainbuild.iTexturePanelSprite[5], 0, 0 );
		}
	}
}

void terrainbuild_settexturehighlight ( void )
{
	if ( terrainbuild.iTexturePanelHighSprite > 0 )
	{
		int iRow = terrainbuild.iCurrentTexture / 4;
		int iCol = terrainbuild.iCurrentTexture - (iRow*4);
		float fChoiceWidth = terrainbuild.iTexturePanelWidth/4;
		float fChoiceHeight = terrainbuild.iTexturePanelHeight/4;
		if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iTexturePanelHighSprite, terrainbuild.iTexturePanelX+(iCol*fChoiceWidth), terrainbuild.iTexturePanelY+(iRow*fChoiceHeight), terrainbuild.iTexturePanelHighImg );
	}
}

void imgui_terrain_loop(void)
{
	if (!imgui_is_running)
		return;

	if (bUpdateVeg && Timer() - iLastUpdateVeg > 2000 && !object_preload_files_in_progress() ) 
	{
		if (bEnableVeg) 
		{
			t.visuals.VegQuantity_f = t.gamevisuals.VegQuantity_f;
			t.visuals.VegWidth_f = t.gamevisuals.VegWidth_f;
			t.visuals.VegHeight_f = t.gamevisuals.VegHeight_f;
			grass_setgrassgridandfade();

			if (!(ObjectExist(t.tGrassObj) == 1 && GetMeshExist(t.tGrassObj) == 1) )
				grass_init();

			//t.completelyfillvegarea = 1;
			t.terrain.grassupdateafterterrain = 1;
			grass_loop();
			t.terrain.grassupdateafterterrain = 0;
			ShowVegetationGrid();
			visuals_justshaderupdate();
			iLastUpdateVeg = Timer();
		}
		else 
		{
			HideVegetationGrid();
			iLastUpdateVeg = Timer();
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
			grass_loop();
			iTerrainVegLoopUpdate = 0;
		}

		//Continue cheking if we need to update terrain.
		if (t.inputsys.mclick == 0 && bReadyToUpdateVeg && bEnableVeg && Timer() - iLastUpdateVeg > 2000 && !object_preload_files_in_progress() ) 
		{
			t.visuals.VegQuantity_f = t.gamevisuals.VegQuantity_f;
			t.visuals.VegWidth_f = t.gamevisuals.VegWidth_f;
			t.visuals.VegHeight_f = t.gamevisuals.VegHeight_f;
			grass_setgrassgridandfade();

			if (!(ObjectExist(t.tGrassObj) == 1 && GetMeshExist(t.tGrassObj) == 1))
				grass_init();

			//t.completelyfillvegarea = 1;
			t.terrain.grassupdateafterterrain = 1;
			grass_loop();
			t.terrain.grassupdateafterterrain = 0;
			ShowVegetationGrid();

			//visuals_justshaderupdate();
			bReadyToUpdateVeg = false;
			iLastUpdateVeg = Timer();
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
			int current_mode = 0;
			float media_icon_size = 40.0f;
			float plate_width = (media_icon_size + 6.0) * 4.0f;
			grideleprof_uniqui_id = 16000;
			int icon_size = 60;
			ImVec2 iToolbarIconSize = { (float)icon_size, (float)icon_size };
			ImVec2 tool_selected_padding = { 1.0, 1.0 };
			ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
			if (pref.current_style == 3)
				tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_Button];

			//Make sure window is setup in docking space.
			ImGui::Begin("Terrain Tools##TerrainToolsWindow", &bTerrain_Tools_Window, 0);

			float w = ImGui::GetWindowContentRegionWidth();
			ImGuiWindow* window = ImGui::GetCurrentWindow();

			current_mode = -1;
			if (t.terrain.terrainpaintermode >= 6) {
				if (t.terrain.terrainpaintermode == 10) {
					current_mode = TOOL_PAINTGRASS;
				}
				else {
					current_mode = TOOL_PAINTTEXTURE;
				}
			}
			else {
				if (t.terrain.terrainpaintermode == 1)  current_mode = TOOL_SHAPE;
				if (t.terrain.terrainpaintermode == 2)  current_mode = TOOL_LEVELMODE;
				if (t.terrain.terrainpaintermode == 3)  current_mode = TOOL_STOREDLEVEL;
				if (t.terrain.terrainpaintermode == 4)  current_mode = TOOL_BLENDMODE;
				if (t.terrain.terrainpaintermode == 5)  current_mode = TOOL_RAMPMODE;
			}

			if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {

//				float control_width = (icon_size ) * 2.0f + 6.0;
				float control_width = (icon_size) * 3.0f + 6.0;

//				if (t.terrain.terrainpaintermode < 6 || current_mode == TOOL_SHAPE ) {
//					control_width = (icon_size) * 3.0f + 6.0;
//				}

				int indent = (w*0.5) - (control_width*0.5);
				if (indent < 10)
					indent = 10;
				ImGui::Indent(indent);


				if (current_mode == TOOL_SHAPE)	window->DrawList->AddRect((window->DC.CursorPos - tool_selected_padding), window->DC.CursorPos + tool_selected_padding + iToolbarIconSize, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				if (ImGui::ImgBtn(TOOL_SHAPE, iToolbarIconSize, ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
					bForceKey = true;
					csForceKey = "t";
					bForceKey2 = true;
					csForceKey2 = "1";
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Shape Mode");
				ImGui::SameLine();


				if (current_mode == TOOL_PAINTTEXTURE) window->DrawList->AddRect((window->DC.CursorPos - tool_selected_padding), window->DC.CursorPos + tool_selected_padding + iToolbarIconSize, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				if (ImGui::ImgBtn(TOOL_PAINTTEXTURE, iToolbarIconSize, ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
					bForceKey = true;
					csForceKey = "t";
					bForceKey2 = true;
					csForceKey2 = "6";
					bTerrain_Tools_Window = true;
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Paint Texture");
				ImGui::SameLine();

				if (current_mode == TOOL_PAINTGRASS) window->DrawList->AddRect((window->DC.CursorPos - tool_selected_padding), window->DC.CursorPos + tool_selected_padding + iToolbarIconSize, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				if (ImGui::ImgBtn(TOOL_PAINTGRASS, iToolbarIconSize, ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
					bForceKey = true;
					csForceKey = "t";
					bForceKey2 = true;
					csForceKey2 = "0";
					bTerrain_Tools_Window = true;
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Paint Grass");

				ImGui::Indent(-indent);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

			}
			
			if (ImGui::CollapsingHeader("Mode", ImGuiTreeNodeFlags_DefaultOpen)) 
			{
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				int control_image_size = 34;
				float control_width = (control_image_size+3.0) * 5.0f + 6.0;
				if (t.terrain.terrainpaintermode < 6 && current_mode != TOOL_SHAPE) {
					control_width = (control_image_size + 3.0) * 3.0f + 6.0;
				}
				int indent = (w*0.5) - (control_width*0.5);
				if (indent < 10)
					indent = 10;
				ImGui::Indent(indent);

				if (t.terrain.terrainpaintermode >= 6) 
				{
					if (iTerrainPaintMode == 1)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					if (ImGui::ImgBtn(EBE_CONTROL1, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
						//Paint mode.
						iTerrainPaintMode = 1;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paint Mode");
					ImGui::SameLine();

					if (iTerrainPaintMode != 1)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(EBE_CONTROL2, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
						//Remove mode.
						iTerrainPaintMode = 0;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Mode");

					ImGui::SameLine();
				}

				if (current_mode == TOOL_SHAPE) {

					if (iTerrainRaiseMode == 1)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					if (ImGui::ImgBtn(TOOL_SHAPE_UP, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
						//Paint mode.
						iTerrainRaiseMode = 1;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raise Terrain");
					ImGui::SameLine();

					if (iTerrainRaiseMode != 1)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_SHAPE_DOWN, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
						//Remove mode.
						iTerrainRaiseMode = 0;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lower Terrain");

					ImGui::SameLine();

				}


				//t.terrain.RADIUS_f > t.tmin)
				//t.terrain.RADIUS_f < g.fTerrainBrushSizeMax)
				ImVec2 cp = ImGui::GetCursorPos();

				//ImGui::Text("Brush Size:");
				ImGui::SetItemAllowOverlap();
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(cp.x, cp.y + (ImGui::GetFontSize() * 1.5)));
				//				ImGui::PushItemWidth(-10);
				ImGui::PushItemWidth((control_image_size + 6.0) * 3.0);
				ImGui::SetWindowFontScale(0.5);
				//ImGuiCol_FrameBg
				ImVec4 oldFrameBg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
				ImVec4 oldBorder = ImGui::GetStyle().Colors[ImGuiCol_Border];

				ImGui::GetStyle().Colors[ImGuiCol_FrameBg].w *= 0.25;
				ImGui::GetStyle().Colors[ImGuiCol_Border].w *= 0.25;

				if (ImGui::SliderFloat("##Brushsize", &t.terrain.RADIUS_f, 70.0f, 500.0f, "")) { //g.fTerrainBrushSizeMax
					if (t.terrain.RADIUS_f < t.tmin) t.terrain.RADIUS_f = t.tmin;
					if (t.terrain.RADIUS_f > g.fTerrainBrushSizeMax) t.terrain.RADIUS_f = g.fTerrainBrushSizeMax;
				}
				ImGui::GetStyle().Colors[ImGuiCol_FrameBg].w = oldFrameBg.w;
				ImGui::GetStyle().Colors[ImGuiCol_Border].w = oldBorder.w;
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius %.2f", t.terrain.RADIUS_f);
				ImGui::PopItemWidth();
				ImGui::SetWindowFontScale(1.0);

				ImGui::SetCursorPos(cp);


				if (0 && t.terrain.RADIUS_f == 110.0f)
				{
					ImVec2 padding = { 3.0, 3.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				ImGui::SetItemAllowOverlap();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 5));
				if (ImGui::ImgBtn(TOOL_DOTCIRCLE_S, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
					t.terrain.RADIUS_f = 110.0f;
				}
//				if (ImGui::RoundButton("Draw Radius Small",ImVec2(control_image_size, control_image_size - 8.0),6)) {
//					t.terrain.RADIUS_f = 110.0f;
//				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius Small");

				ImGui::SameLine();

				if (0 && t.terrain.RADIUS_f == 280.0f)
				{
					ImVec2 padding = { 3.0, 3.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOL_DOTCIRCLE_M, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
					t.terrain.RADIUS_f = 280.0f;
				}
//				if (ImGui::RoundButton("Draw Radius Medium", ImVec2(control_image_size, control_image_size - 8.0), 10)) {
//					t.terrain.RADIUS_f = 280.0f;
//				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius Medium");

				ImGui::SameLine();

				if (0 && t.terrain.RADIUS_f == 450.0f)
				{
					ImVec2 padding = { 3.0, 3.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOL_DOTCIRCLE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false)) {
					t.terrain.RADIUS_f = 450.0f;
				}
//				if (ImGui::RoundButton("Draw Radius Large", ImVec2(control_image_size, control_image_size-8.0),14)) {
//					t.terrain.RADIUS_f = 450.0f;
//				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius Large");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 5));
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 8));
				ImGui::Indent(-indent);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
			}

			if (ImGui::CollapsingHeader("Customize Sky", ImGuiTreeNodeFlags_DefaultOpen)) {

				ImGui::Indent(10);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::PushItemWidth(-10);

				char * current_sky = t.skybank_s[t.visuals.skyindex].Get();
				if (t.skybank_s[t.visuals.skyindex] == "None") current_sky = "Dynamic Clouds";
				if (!current_sky) current_sky = "NA";
				if (ImGui::BeginCombo("##SelectSkyCombo", current_sky)) // The second parameter is the label previewed before opening the combo.
				{

					for (int skyindex = 1; skyindex <= g.skymax; skyindex++)
					{

						if (t.skybank_s[skyindex].Len() > 0 )
						{
							bool is_selected = false;
							if (t.skybank_s[skyindex].Get() == current_sky)
								is_selected = true;
							cstr  sSkyname = t.skybank_s[skyindex];
							if (sSkyname == "None") sSkyname = "Dynamic Clouds";
							if (ImGui::Selectable(sSkyname.Get(), is_selected)) {

								g.projectmodified = 1;
								current_sky = t.terrainstylebank_s[skyindex].Get();
								t.visuals.skyindex = skyindex;
								t.gamevisuals.skyindex = t.visuals.skyindex;
								
								g.skyindex = t.visuals.skyindex;
								t.visuals.sky_s = t.skybank_s[g.skyindex];
								t.gamevisuals.sky_s = t.skybank_s[g.skyindex];
								t.terrainskyspecinitmode = 0; sky_skyspec_init();
								t.sky.currenthour_f = 8.0;
								t.sky.daynightprogress = 0;

								visuals_justshaderupdate();
								// if change sky, regenerate env map
								t.visuals.refreshskysettingsfromlua = true;
								cubemap_generateglobalenvmap();
								t.visuals.refreshskysettingsfromlua = false;

							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();

							//t.skybank_s[skyindex];
						}
					}

					ImGui::EndCombo();
				}
			
				ImGui::PopItemWidth();
				ImGui::Indent(-10);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

			}

			if (ImGui::CollapsingHeader("Customize Terrain", ImGuiTreeNodeFlags_DefaultOpen)) {

				//Drpo down.
				ImGui::Indent(10);

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				ImGui::PushItemWidth(-10);
				char * current_terrain = t.terrainstylebank_s[t.visuals.terrainindex].Get();
				if (!current_terrain) current_terrain = "NA";

				if (ImGui::BeginCombo("##SelectTerrainCombo", current_terrain)) // The second parameter is the label previewed before opening the combo.
				{
					for (int terrainindex = 1; terrainindex <= g.terrainstylemax; terrainindex++)
					{
						if(t.terrainstylebank_s[terrainindex].Len() > 0 && t.terrainstylebank_s[terrainindex] != "custom" )
						{
							bool is_selected = false;
							if (t.terrainstylebank_s[terrainindex].Get() == current_terrain)
								is_selected = true;
							if (ImGui::Selectable(t.terrainstylebank_s[terrainindex].Get(), is_selected)) 
							{
								//Change Terrain.
								bool bNewTextureValid = true;

								#ifdef ENABLECUSTOMTERRAIN
								 if (t.terrainstylebank_s[terrainindex] == "CUSTOM") 
								 {
									if (FileExist(cstr(g.mysystem.levelBankTestMap_s + TEXTURE_D_NAME).Get()) == 0) 
									{
										extern bool bTriggerMessage;
										extern char cTriggerMessage[MAX_PATH];
										bNewTextureValid = false;
										strcpy(cTriggerMessage, "Custom Terrain Not Found.");
										bTriggerMessage = true;
									}
								 }
								#endif
								if (bNewTextureValid) {
									g.projectmodified = 1;
									current_terrain = t.terrainstylebank_s[terrainindex].Get();

									//Get old so we can delete later. (they take up some mem).
									cstr sOldTerrainTextureLocation = terrain_getterrainfolder();

									g.terrainstyleindex = terrainindex;
									t.visuals.terrainindex = g.terrainstyleindex;
									if (g.terrainstyleindex > g.terrainstylemax)  g.terrainstyleindex = g.terrainstylemax;
									t.visuals.terrain_s = t.terrainstylebank_s[g.terrainstyleindex];
									t.gamevisuals.terrain_s = t.visuals.terrain_s;
									t.gamevisuals.terrainindex = t.visuals.terrainindex; //for save fpm
									terrain_changestyle();

									//Load new terrain textures
									int iTex = 5; //Terrain plate.

									//PE: First Delete old internal image.
									deleteinternaltexture(cstr(sOldTerrainTextureLocation + "\\" + TEXTURE_D_NAME).Get());

									cstr sTerrainTextureLocation = terrain_getterrainfolder();
									//PE: Delete current if we had been switched to custom.
									//deleteinternaltexture(cstr(sTerrainTextureLocation + "\\" + TEXTURE_D_NAME).Get());
									removeinternalimage(terrainbuild.iTexturePanelImg[iTex]);

									terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(sTerrainTextureLocation + "\\" + TEXTURE_D_NAME).Get());
									if (terrainbuild.iTexturePanelImg[iTex] == 0)
									{
										terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(sTerrainTextureLocation + "\\texture_D.dds").Get());
										if (terrainbuild.iTexturePanelImg[iTex] == 0)
										{
											// means the terrain texture is missing, report this and switch to default to avoid crash
											terrain_initstyles_reset();
											grass_initstyles_reset();
											sTerrainTextureLocation = terrain_getterrainfolder();
											terrainbuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(sTerrainTextureLocation + "\\" + TEXTURE_D_NAME).Get());
										}
									}
									terrainbuild.terrainstyle = g.terrainstyle_s;
									terrainbuild.iTexPlateImage = terrainbuild.iTexturePanelImg[iTex];

									//if (t.terrainstylebank_s[terrainindex] == "CUSTOM") {
										terrain_generatesupertexture(false);
									//}

									visuals_justshaderupdate();
									// if change sky, regenerate env map
									t.visuals.refreshskysettingsfromlua = true;
									cubemap_generateglobalenvmap();
									t.visuals.refreshskysettingsfromlua = false;

								}
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
				ImGui::Indent(-10);


				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (plate_width*0.5), 0.0f));
				int indent = (w*0.5) - (plate_width*0.5);
				if (indent < 10)
					indent = 10;
				ImGui::Indent(indent);


				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));

				if (terrainbuild.iTexPlateImage <= 0) {
					ImGui::Text("");
				}
				int n = 0;
				for (int y = 0; y < 4; y++)
				{
					for (int x = 0; x < 4; x++)
					{

						if (terrainbuild.iTexPlateImage > 0) {

							ImGuiWindow* window = ImGui::GetCurrentWindow();

							ImVec2 padding = { 2.0, 2.0 };
							ImVec4 bg_col = { 0.0, 0.0, 0.0, 1.0 };
							const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(media_icon_size, media_icon_size));
							window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, 15);

							if (current_mode == TOOL_PAINTTEXTURE && terrainbuild.iCurrentTexture == n) {
								ImVec2 padding = { 4.0, 4.0 };
								const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(media_icon_size, media_icon_size));
								window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
							}

							if (n == 0) {
								CheckTutorialAction("TOOL_TERRAIN_SAND", -20.0f); //Tutorial: check if we are waiting for this action
							}
							ImGui::PushID(terrainbuild.iTexPlateImage + n + 200000);
							if (ImGui::ImgBtn(terrainbuild.iTexPlateImage, ImVec2(media_icon_size, media_icon_size), ImVec4(0.0, 0.0, 0.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, n + 1, 4, 4, false,false,false,true)) {
								
								if (n == 0 && bTutorialCheckAction) TutorialNextAction();

								terrainbuild.iCurrentTexture = n;
								//Change to selected terrainpaintermode
								if (current_mode != TOOL_PAINTTEXTURE) {

									if (terrainbuild.iCurrentTexture < 8)
										t.terrain.terrainpaintermode = 6;
									else if (terrainbuild.iCurrentTexture < 12)
										t.terrain.terrainpaintermode = 7;
									else if (terrainbuild.iCurrentTexture < 15)
										t.terrain.terrainpaintermode = 8;
									else
										t.terrain.terrainpaintermode = 9;

								}
							}
							if (skib_terrain_frames_execute == 0 && ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
								terrainbuild.iCurrentTexture = n;
								if (current_mode != TOOL_PAINTTEXTURE) {
									if (terrainbuild.iCurrentTexture < 8)
										t.terrain.terrainpaintermode = 6;
									else if (terrainbuild.iCurrentTexture < 12)
										t.terrain.terrainpaintermode = 7;
									else if (terrainbuild.iCurrentTexture < 15)
										t.terrain.terrainpaintermode = 8;
									else
										t.terrain.terrainpaintermode = 9;
								}
								delay_terrain_execute = 1; //@Lee remove this line if you dont want to support changing textures.
							}

							ImGui::PopID();
							if (x != 3)
								ImGui::SameLine();

							if (delay_terrain_execute == 0 && ImGui::IsItemHovered()) {
								ImVec2 cursor_pos = ImGui::GetIO().MousePos;
								ImVec2 tooltip_offset(10.0f, ImGui::GetFontSize()*1.5);
								ImVec2 tooltip_position = cursor_pos;
								if (tooltip_position.x + 210 > GetDesktopWidth())
									tooltip_position.x -= 210;
								tooltip_position.x += tooltip_offset.x;
								tooltip_position.y += tooltip_offset.y;
								ImGui::SetNextWindowPos(tooltip_position);
								ImGui::SetNextWindowContentWidth(204.0f);
								ImGui::BeginTooltip();
								float icon_ratio;
								int icon_size = 204;
								ImGui::ImgBtn(terrainbuild.iTexPlateImage, ImVec2(icon_size, icon_size), ImVec4(0.0, 0.0, 0.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, n + 1, 4, 4, false,false,false,true);
								//char hchar[MAX_PATH];
								//ImGui::Text("%s", hchar);
								ImGui::EndTooltip();

							}

						}
						n++;
					}
				}
				ImGui::Indent(-indent);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));


			}


			//Customize Vegetation

			if (ImGui::CollapsingHeader("Customize Vegetation", ImGuiTreeNodeFlags_DefaultOpen)) {

				ImGui::Indent(10);
				ImGui::PushItemWidth(-10);

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				char * current_veg = t.vegstylebank_s[g.vegstyleindex].Get();
				if (!current_veg) current_veg = "NA";
				if (ImGui::BeginCombo("##SelectVegetationCombo", current_veg)) // The second parameter is the label previewed before opening the combo.
				{

					for (int vegindex = 1; vegindex <= g.vegstylemax; vegindex++)
					{

						if (t.vegstylebank_s[vegindex].Len() > 0)
						{
							bool is_selected = false;
							if (t.vegstylebank_s[vegindex].Get() == current_veg)
								is_selected = true;
							if (ImGui::Selectable(t.vegstylebank_s[vegindex].Get(), is_selected)) 
							{
								//Test display veg.
								//grass_setgrassgridandfade();
								//grass_init();
								//t.completelyfillvegarea = 1;
								t.terrain.grassupdateafterterrain = 1;
								grass_loop();
								t.terrain.grassupdateafterterrain = 0;

								g.projectmodified = 1;
								current_veg = t.vegstylebank_s[vegindex].Get();// t.terrainstylebank_s[vegindex].Get(); big fix for VRQ

								g.vegstyleindex = vegindex;
								g.vegstyle_s = t.vegstylebank_s[g.vegstyleindex];

								t.visuals.vegetationindex = g.vegstyleindex;
								t.visuals.vegetation_s = t.vegstylebank_s[g.vegstyleindex];;
								t.gamevisuals.vegetationindex = t.visuals.vegetationindex;
								t.gamevisuals.vegetation_s = t.visuals.vegetation_s;
								grass_changevegstyle();
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();

						}
					}

					ImGui::EndCombo();
				}

				ImGui::PopItemWidth();

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text("Display Vegetation:");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(136, ImGui::GetCursorPosY()));
				if (ImGui::Checkbox("##DisplayVeg", &bEnableVeg)) {
					if (bEnableVeg) {
						iLastUpdateVeg = 0;
						bUpdateVeg = true;
					}
				}


//				if (bEnableVeg) {
//					ImGui::SameLine();
//					//ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
//					if (ImGui::Button("Refresh")) {
//						iLastUpdateVeg = 0;
//						bUpdateVeg = true;
//					}
//				}

				if (bEnableVeg) {
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::Text("Quantity:");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
					ImGui::SetCursorPos(ImVec2(136, ImGui::GetCursorPosY()));
					ImGui::PushItemWidth(-10);
					if (ImGui::SliderFloat("##VegQuantity", &t.gamevisuals.VegQuantity_f, 0.0, 100.0))
					{
						bUpdateVeg = true;
					}
					ImGui::PopItemWidth();

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::Text("Height:");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
					ImGui::SetCursorPos(ImVec2(136, ImGui::GetCursorPosY()));
					ImGui::PushItemWidth(-10);
					if (ImGui::SliderFloat("##VegHeight", &t.gamevisuals.VegHeight_f, 0.0, 100.0))
					{
						bUpdateVeg = true;
					}
					ImGui::PopItemWidth();

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::Text("Width:");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
					ImGui::SetCursorPos(ImVec2(136, ImGui::GetCursorPosY()));
					ImGui::PushItemWidth(-10);
					if (ImGui::SliderFloat("##VegWidth", &t.gamevisuals.VegWidth_f, 0.0, 100.0))
					{
						bUpdateVeg = true;
					}
					ImGui::PopItemWidth();
				}

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Indent(-10);
			}


#ifdef ALLOW_WEATHER_IN_EDITOR
			if (ImGui::CollapsingHeader("Weather", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent(10);

				const char* items_align[] = { "None" , "Light Rain", "Heavy Rain","Light Snow" ,"Heavy Snow"  }; //,"Test"
				int item_current_type_selection = 0;
				item_current_type_selection = t.visuals.iEnvironmentWeather;
				ImGui::PushItemWidth(-10);
				if (ImGui::Combo("##WeatherDropwDown", &item_current_type_selection, items_align, IM_ARRAYSIZE(items_align))) {
					t.visuals.iEnvironmentWeather = item_current_type_selection;
					t.gamevisuals.iEnvironmentWeather = t.visuals.iEnvironmentWeather;
				}
				ImGui::PopItemWidth();
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Weather");


				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text("Display Weather:");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(136, ImGui::GetCursorPosY()));
				if (ImGui::Checkbox("##DisplayWeather", &bEnableWeather)) 
				{
					reset_env_particles();
				}

				ImGui::Indent(-10);
			}
#endif

			if (ImGui::CollapsingHeader("Keyboard Shortcuts", ImGuiTreeNodeFlags_DefaultOpen)) 
			{
				ImGui::Indent(10);
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));

				// context help button
				float button_gadget_size = ImGui::GetFontSize()*10.0;
				float w = ImGui::GetWindowContentRegionWidth();
				ImGui::Text("Left Mouse Button to Paint.");
				ImGui::Text("Shift + Left Mouse Button to Remove.");
				ImGui::Text("+ Increase Draw Radius.");
				ImGui::Text("- Decrease Draw Radius.");
				ImGui::Indent(-10);
			}

			ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
			if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
			{
				bImGuiGotFocus = true;
			}
			if (ImGui::IsAnyItemFocused()) {
				bImGuiGotFocus = true;
			}

			void CheckMinimumDockSpaceSize(float minsize);
			CheckMinimumDockSpaceSize(200.0f);

			ImGui::End();
		}
	}
	else {
		//bTerrain_Tools_Window = false;
	}
}

void terrain_paintselector_control ( void )
{
	// manage terrain texture paint selector
	// ensure system is initialised
	if ( terrainbuild.initialised == 0 ) 
	{
		if ( t.conkit.editmodeactive == 0 ) 
		{
			// don't show if this gets triggered via F9 mode
			pastebitmapfontcenter("PREPARING TEXTURE PAINTER",GetChildWindowWidth()/2,40,1,255) ; Sync (  );
		}
		terrain_paintselector_init();
	}

#ifndef USEOLDGUI
	//PE: Reposition everything
	terrainbuild.iTexturePanelX = GetChildWindowWidth() - 210;
	terrainbuild.iTexturePanelY = GetChildWindowHeight() - 200;
	terrainbuild.iTexturePanelWidth = 200;
	terrainbuild.iTexturePanelHeight = 200;
	for (int iTex = 0; iTex < TERRAINTEXPANELSPRMAX; iTex++)
	{
		LPSTR pTexImg = "";
		int iX = terrainbuild.iTexturePanelX;
		int iY = terrainbuild.iTexturePanelY;
		int iWidth = terrainbuild.iTexturePanelWidth;
		int iHeight = terrainbuild.iTexturePanelHeight;
		if (iTex == 0) { iX -= 10; iY -= 10; iWidth += 20; iHeight += 20; }
		if (iTex == 1) { iX -= 10; iY -= 10; iWidth += 20; iHeight = 1; }
		if (iTex == 2) { iX -= 10; iY += 209; iWidth += 20; iHeight = 1; }
		if (iTex == 3) { iX -= 10; iY -= 10; iWidth = 1; iHeight += 20; }
		if (iTex == 4) { iX += 209; iY -= 10; iWidth = 1; iHeight += 20; }
		if (!bDisableAllTerrainSprites) Sprite(terrainbuild.iTexturePanelSprite[iTex], iX, iY, terrainbuild.iTexturePanelImg[iTex]);
	}
	if (!bDisableAllTerrainSprites) Sprite(terrainbuild.iTexturePanelHighSprite, terrainbuild.iTexturePanelX, terrainbuild.iTexturePanelY, terrainbuild.iTexturePanelHighImg);
	if (!bDisableAllTerrainSprites) SizeSprite(terrainbuild.iTexturePanelHighSprite, terrainbuild.iTexturePanelWidth / 4, terrainbuild.iTexturePanelHeight / 4);
	if (!bDisableAllTerrainSprites) Sprite(terrainbuild.iHelpSpr, terrainbuild.iTexturePanelX - ImageWidth(terrainbuild.iHelpImg) - 10, terrainbuild.iTexturePanelY + 210 - ImageHeight(terrainbuild.iHelpImg), terrainbuild.iHelpImg);
	if (!bDisableAllTerrainSprites) Sprite(terrainbuild.iTexHelpSpr, terrainbuild.iTexturePanelX - 10, terrainbuild.iTexturePanelY - 10 - ImageHeight(terrainbuild.iTexHelpImg), terrainbuild.iTexHelpImg);
	terrainbuild_settexturehighlight();
#endif

	terrain_paintselector_show();

	// Only when release mouse continue
	if ( terrainbuild.bReleaseMouseFirst == true && t.inputsys.mclick != 0 ) return;
	terrainbuild.bReleaseMouseFirst = false;

	// reduce large custom files needed for FPM and Level Cloud (faster down/loading)
	#ifdef ENABLECUSTOMTERRAIN
	 // Reason this is above action for selecting customise is to allow texture highlight to show as selected
	 if ( terrainbuild.bCustomiseTexture == true && t.inputsys.mclick == 0 )
	 {
		// load from terrainbank or custom from levelbank\testmap
		g.terrainstyle_s = t.terrainstylebank_s[g.terrainstyleindex];
		char pThisTerrainTexturePath[512];
		strcpy ( pThisTerrainTexturePath, cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );
		if ( g.terrainstyleindex == 1 ) strcpy ( pThisTerrainTexturePath, g.mysystem.levelBankTestMap_s.Get() ); //"levelbank\\testmap" );
		if ( PathExist( pThisTerrainTexturePath ) == 1 ) 
		{
			// NewJPG or DDS
			bool bUseFirstChoice = true;

			// check if new terrain texture system file available, and create if not
			char pOldTerrainTextureFile[512];
			strcpy ( pOldTerrainTextureFile, cstr(cstr(pThisTerrainTexturePath)+cstr("\\")+TEXTURE_D_NAME).Get() );
			if ( FileExist ( pOldTerrainTextureFile ) == 0 ) 
			{ 
				bUseFirstChoice = false; 
				strcpy ( pOldTerrainTextureFile, cstr(cstr(pThisTerrainTexturePath)+cstr("\\texture_D.dds")).Get() ); 
			}
			if ( FileExist ( pOldTerrainTextureFile ) == 1 )
			{
				// switch to using CUSTOM 
				if ( g.terrainstyleindex != 1 )
				{
					t.visuals.terrainindex = 1;
					t.visuals.terrain_s = "CUSTOM";
					g.terrainstyleindex = t.visuals.terrainindex;
					g.terrainstyle_s = t.visuals.terrain_s;

					//Make sure changes is saved in fpm.
					g.projectmodified = 1;
					t.gamevisuals.terrain_s = t.visuals.terrain_s;
					t.gamevisuals.terrainindex = t.visuals.terrainindex; //for save fpm

					// diffuse file
					char pNewLocationForFile[512];
					if ( bUseFirstChoice == true )
						strcpy ( pNewLocationForFile, cstr(g.mysystem.levelBankTestMap_s+TEXTURE_D_NAME).Get() );
					else
						strcpy ( pNewLocationForFile, cstr(g.mysystem.levelBankTestMap_s+"texture_D.dds").Get() );
					if ( FileExist ( pNewLocationForFile ) == 1 ) DeleteFile ( pNewLocationForFile );
					CopyFile ( pOldTerrainTextureFile, pNewLocationForFile, FALSE );

					// normal file

					if ( bUseFirstChoice == true )
						strcpy ( pOldTerrainTextureFile, cstr(g.mysystem.levelBankTestMap_s+TEXTURE_D_NAME).Get() );
					else
						strcpy ( pOldTerrainTextureFile, cstr(g.mysystem.levelBankTestMap_s+"texture_D.dds").Get() );
				}

				// change this texture slot with a new one
				bool bMustClosePopup = false;
				terrainbuild.bCustomiseTexture = false;
				if ( terrain_loadcustomtexture ( pOldTerrainTextureFile, terrainbuild.iCurrentTexture ) == 1 )
				{
					// may have loaded DDS but saved as JPG (new primary choice for VRQ)
					if ( FileExist ( cstr(g.mysystem.levelBankTestMap_s+TEXTURE_D_NAME).Get() ) == 1 )
						strcpy ( pOldTerrainTextureFile, cstr(g.mysystem.levelBankTestMap_s+TEXTURE_D_NAME).Get() );

					// successfully changed the texture
					LoadImage ( pOldTerrainTextureFile, terrainbuild.iTexPlateImage );
					int iX = terrainbuild.iTexturePanelX;
					int iY = terrainbuild.iTexturePanelY;
					if (!bDisableAllTerrainSprites) Sprite ( terrainbuild.iTexturePanelSprite[5], iX, iY, terrainbuild.iTexturePanelImg[5] );

					// also re-texture current terrain with change too
					terrain_loadlatesttexture ( );

					// generate super texture palette from above NEW texture
					terrain_deletesupertexturepalette();

					// must close popup
					bMustClosePopup = true;
				}

				// close popup if used
				if ( bMustClosePopup == true )
				{
					// Clear status Text
					t.statusbar_s = ""; popup_text_close();
				}
			}
		}
		//After blocking dialogs , Reset click state.
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.MouseDown[1] = 0;
		io.MouseDownDuration[1] = 0;
		io.MouseDown[0] = 0;
		io.MouseDownDuration[0] = 0;
	 }
	#endif

	//PE: imgui need support here.
#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
	int iRealSprMouseX = ((GetChildWindowWidth(-1) + 0.0) / (float)GetDisplayWidth()) * t.inputsys.xmouse;
	int iRealSprMouseY = ((GetChildWindowHeight(-1) + 0.0) / (float)GetDisplayHeight()) * t.inputsys.ymouse;
#else
	// Select texture if in Texture Panel or Customise one
	int iRealSprMouseX = (GetChildWindowWidth()/800.0f) * t.inputsys.xmouse;
	int iRealSprMouseY = (GetChildWindowHeight()/600.0f) * t.inputsys.ymouse;
#endif

	// Select texture if in Texture Panel or Customise one
#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
	if (0)
#else
	if ( t.inputsys.mclick > 0 )
#endif
	{
		if ( iRealSprMouseX > terrainbuild.iTexturePanelX && iRealSprMouseX < terrainbuild.iTexturePanelX + terrainbuild.iTexturePanelWidth )
		{
			if ( iRealSprMouseY > terrainbuild.iTexturePanelY && iRealSprMouseY < terrainbuild.iTexturePanelY + terrainbuild.iTexturePanelHeight )
			{
				// while tile
				float fWhichCol = (float)(iRealSprMouseX - terrainbuild.iTexturePanelX) / (float)terrainbuild.iTexturePanelWidth;
				float fWhichRow = (float)(iRealSprMouseY - terrainbuild.iTexturePanelY) / (float)terrainbuild.iTexturePanelHeight;
				int iWhichTextureOver = (((int)(fWhichRow*4))*4) + (int)(fWhichCol*4);

				// select texture choice
				terrainbuild.iCurrentTexture = iWhichTextureOver;
				terrainbuild_settexturehighlight();

				// reduce large custom files needed for FPM and Level Cloud (faster down/loading)
				#ifdef ENABLECUSTOMTERRAIN
				 // and if it was right mouse, customise this texture too
				 //#ifdef VRTECH
				 // Cannot allow custom terrain textures - bloats FPM making transfer over multiplayer very slow
				 //#else
				 if ( t.inputsys.mclick == 2 )
				 {
					// replace texture within texture atlas
					terrainbuild.bCustomiseTexture = true;
				 }
				 //#endif
				#endif

				// ensure we do not write into texture painter if selecting texture
				set_inputsys_mclick(0);// t.inputsys.mclick = 0;
				t.mc = 0;
			}
		}
	}
}

int terrain_loadcustomtexture ( LPSTR pDestPathAndFile, int iTextureSlot )
{
	// Needed locals
	LPSTR pOldDir = GetDir();
	HWND hThisWnd = GetForegroundWindow();

	// terrain build Load Folder
	t.strwork = g.fpscrootdir_s + "\\Files\\texturebank";
	if ( PathExist( t.strwork.Get() ) == 0 ) 
	{
		MessageBox ( hThisWnd, "Cannot find textures folder", "Error", MB_OK | MB_TOPMOST );
		return 0;
	}
	SetDir ( t.strwork.Get() );

	//  Ask for save filename
	cStr tLoadFile = "";
	cStr tLoadMessage = "Replace with custom texture";
	//tLoadFile = openFileBox("Diffuse File (_D.dds)|*.dds|Texture File (.dds)|*.dds|All Files|*.*|", t.strwork.Get(), tLoadMessage.Get(), ".dds", IMPORTERSAVEFILE);
	tLoadFile = openFileBox("Diffuse File (.png)|*.png|Texture File (.dds)|*.dds|All Files|*.*|", t.strwork.Get(), tLoadMessage.Get(), ".png", IMPORTERSAVEFILE);
	if ( tLoadFile == "Error" )
	{
		SetDir(pOldDir);
		return 0;
	}

	// Use large prompt
	t.statusbar_s = "Generating New Terrain Texture"; 
	popup_text(t.statusbar_s.Get());

	// strip file from path
	char pPathOnly[512];
	strcpy ( pPathOnly, pDestPathAndFile );
	for ( int n = strlen(pDestPathAndFile); n > 0 ; n-- )
	{
		if ( pDestPathAndFile[n] == '\\' || pDestPathAndFile[n] == '/' )
		{
			pPathOnly[n+1] = 0;
			break;
		}
	}

	// final destination of Texture Plate subsets (from current terrain)
	cstr sSavePathFile = g.fpscrootdir_s + "\\Files\\" + pPathOnly;
	SetDir(sSavePathFile.Get());

	// replace texture in plate with provided custom one
	terrain_createnewterraintexture ( TEXTURE_D_NAME, iTextureSlot, tLoadFile.Get(), 0, 1 );
	
	// if a normal map exists for it, use that too

	// restore current folder
	SetDir(pOldDir);

	// success
	return 1;
}

int terrain_createnewterraintexture ( LPSTR pDestTerrainTextureFile, int iWhichTextureOver, LPSTR pTexFileToLoad, int iSeamlessMode, int iCompressIt )
{
	#ifdef DX11

	char szRealFilename[ MAX_PATH ];
	strcpy_s( szRealFilename, MAX_PATH, pTexFileToLoad );
	GG_GetRealPath( szRealFilename, 0 );

	// check if texture to load exists
	GGIMAGE_INFO finfo;
	HRESULT hr = D3DX11GetImageInfoFromFile( szRealFilename, NULL, &finfo, NULL );

	// filenames to WCHAR
	wchar_t wFilenameInsert[512];
	wchar_t wFilenamePlate[512];
	cstr pPlateFilename = cstr(pDestTerrainTextureFile);
	MultiByteToWideChar(CP_ACP, 0, szRealFilename, -1, wFilenameInsert, sizeof(wFilenameInsert));
	MultiByteToWideChar(CP_ACP, 0, pPlateFilename.Get(), -1, wFilenamePlate, sizeof(wFilenamePlate));

	// is insert a DDS or other
	bool bInsertTextureIsDDS = false;
	if ( strnicmp ( pTexFileToLoad + strlen(pTexFileToLoad) - 4, ".dds", 4 ) == NULL )
		bInsertTextureIsDDS = true;

	// create and load the texture selected
	ScratchImage imageTextureToInsert;
	ScratchImage imageTexturePlate;
	ScratchImage convertedTextureToInsert;
	ScratchImage convertedTexturePlate;
	LPGGTEXTURE pLoadedTexSurface1024 = NULL;
	LPGGTEXTURE pLoadedTexSurface512 = NULL;
	LPGGTEXTURE pLoadedTexSurface256512 = NULL;
	LPGGTEXTURE pLoadedTexSurface512256 = NULL;
	LPGGTEXTURE pPlateSurface = NULL;
	if ( hr == S_OK )
	{
		// create/load texture to be inserted
		TexMetadata insertdata;
		if ( bInsertTextureIsDDS == true )
		{
			hr = GetMetadataFromDDSFile( wFilenameInsert, DDS_FLAGS_NONE, insertdata );			
			hr = LoadFromDDSFile( wFilenameInsert, DDS_FLAGS_NONE, &insertdata, imageTextureToInsert );
		}
		else
		{
			hr = GetMetadataFromWICFile( wFilenameInsert, DDS_FLAGS_NONE, insertdata );			
			hr = LoadFromWICFile( wFilenameInsert, DDS_FLAGS_NONE, &insertdata, imageTextureToInsert );
		}
		if ( SUCCEEDED(hr) )
		{
			// is current a JPG
			bool bCurrentIsAJPG = false;
			if ( strstr ( pPlateFilename.Get(), ".jpg" ) > 0 )
				bCurrentIsAJPG = true;

			// create/load texture of plate
			TexMetadata platedata;
			if ( bCurrentIsAJPG == true )
				hr = GetMetadataFromWICFile( wFilenamePlate, DDS_FLAGS_NONE, platedata );			
			else
				hr = GetMetadataFromDDSFile( wFilenamePlate, DDS_FLAGS_NONE, platedata );			
			if ( hr != S_OK )
			{
				// if plate file not exist, create one and provide dimensions
				finfo.Width = 4096;
				finfo.Height = 4096;

				// create the plate from fresh
				//imageTexturePlate.Initialize2D ( DXGI_FORMAT_BC5_UNORM, finfo.Width, finfo.Height, 1, 1, 0 ); 
				//imageTexturePlate.Initialize2D ( DXGI_FORMAT_BC5_UNORM, finfo.Width, finfo.Height, 1, 13, 0 ); // changed when switched to JPG instead of DDS (for Player)
				imageTexturePlate.Initialize2D (DXGIFORMATR8G8B8A8UNORM, finfo.Width, finfo.Height, 1, 1, 0 ); // for JPG
				platedata = imageTexturePlate.GetMetadata();
			}
			else
			{
				// existing plate exists, load it in
				if ( bCurrentIsAJPG == true )
					hr = LoadFromWICFile( wFilenamePlate, DDS_FLAGS_NONE, &platedata, imageTexturePlate );
				else
					hr = LoadFromDDSFile( wFilenamePlate, DDS_FLAGS_NONE, &platedata, imageTexturePlate );
				if ( FAILED(hr) ) platedata.width = 0;
			}
			if ( platedata.width > 0 )
			{
				// ensure we convert compressed textures to uncompressed ones
				ScratchImage* pWrkImage = &imageTextureToInsert;
				if ( imageTextureToInsert.GetMetadata().format >= DXGI_FORMAT_BC1_TYPELESS && imageTextureToInsert.GetMetadata().format <= DXGI_FORMAT_BC5_SNORM )
				{
					if (bCurrentIsAJPG == true) {
						hr = Decompress( imageTextureToInsert.GetImages(), imageTextureToInsert.GetImageCount(), imageTextureToInsert.GetMetadata(),
							DXGIFORMATR8G8B8A8UNORM, convertedTextureToInsert);// DXGI_FORMAT_B8G8R8A8_UNORM, convertedTextureToInsert );
					}
					else {
						//PE: We need DXGI_FORMAT_B8G8R8A8_UNORM or sometimes CopySubresourceRegion fails. strange one ?
						//PE: DXGI_FORMAT_B8G8R8A8_UNORM works in all cases when using dds.
						hr = Decompress(imageTextureToInsert.GetImages(), imageTextureToInsert.GetImageCount(), imageTextureToInsert.GetMetadata(),
							DXGI_FORMAT_B8G8R8A8_UNORM, convertedTextureToInsert);
					}

					pWrkImage = &convertedTextureToInsert;
				}

				// resize
				ScratchImage InsertImage1024;
				ScratchImage InsertImage512;
				ScratchImage InsertImage256512;
				ScratchImage InsertImage512256;
				hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 1024, 1024, TEX_FILTER_SEPARATE_ALPHA, InsertImage1024 );
				CreateTexture(m_pD3D, InsertImage1024.GetImages(), InsertImage1024.GetImageCount(), InsertImage1024.GetMetadata(), &pLoadedTexSurface1024 );
				if ( iSeamlessMode == 0 )
				{
					// 512x512 at center
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 512, 512, TEX_FILTER_SEPARATE_ALPHA, InsertImage512 );
					CreateTexture(m_pD3D, InsertImage512.GetImages(), InsertImage512.GetImageCount(), InsertImage512.GetMetadata(), &pLoadedTexSurface512 );
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 256, 512, TEX_FILTER_SEPARATE_ALPHA, InsertImage256512 );
					CreateTexture(m_pD3D, InsertImage256512.GetImages(), InsertImage256512.GetImageCount(), InsertImage256512.GetMetadata(), &pLoadedTexSurface256512 );
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 512, 256, TEX_FILTER_SEPARATE_ALPHA, InsertImage512256 );
					CreateTexture(m_pD3D, InsertImage512256.GetImages(), InsertImage512256.GetImageCount(), InsertImage512256.GetMetadata(), &pLoadedTexSurface512256 );
				}
				else
				{
					// 1022x1022 at center
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 1022, 1022, TEX_FILTER_SEPARATE_ALPHA, InsertImage512 );
					hr = CreateTexture(m_pD3D, InsertImage512.GetImages(), InsertImage512.GetImageCount(), InsertImage512.GetMetadata(), &pLoadedTexSurface512 );
				}
				// texture plate always compressed (not any more)
				//hr = Decompress( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(),
				//	DXGI_FORMAT_B8G8R8A8_UNORM, convertedTexturePlate );
				//if ( convertedTexturePlate.GetImageCount() == 0 )
				//{
				CreateTexture(m_pD3D, imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), platedata, &pPlateSurface );
				//}
				//else
				//{
				//	platedata.format = convertedTexturePlate.GetImages()->format;
				//	CreateTexture(m_pD3D, convertedTexturePlate.GetImages(), convertedTexturePlate.GetImageCount(), platedata, &pPlateSurface );
				//}
			}
		}

		// paste loaded texture into plate (60 in center of atlas texture area)
		if ( pLoadedTexSurface1024 && pPlateSurface ) 
		{
			// work out exact offset to slot position
			int iRow = iWhichTextureOver / 4;
			int iCol = iWhichTextureOver - (iRow*4);
			int iTexSlotOffsetX = iCol * 1024;
			int iTexSlotOffsetY = iRow * 1024;
			RECT rcPlate = RECT();

			// paste to fill 1024x1024 initially (to get at corners)
			rcPlate.left = iTexSlotOffsetX;
			rcPlate.top = iTexSlotOffsetY;
			rcPlate.right = iTexSlotOffsetX+1024;
			rcPlate.bottom = iTexSlotOffsetY+1024;
			m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface1024, 0, NULL );
			if ( iSeamlessMode == 0 )
			{
				// paste squashed 256x512 borders to help seamlessness
				// left
				rcPlate.left = iTexSlotOffsetX+0; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface256512, 0, NULL );
				// right
				rcPlate.left = iTexSlotOffsetX+256+512; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+1024; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface256512, 0, NULL );
				// top
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+0; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512256, 0, NULL );
				// bottom
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256+512; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+1024;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512256, 0, NULL );
				// paste insert so smaller 512x512 terrain texture atlas can be seamless
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
			}
			else
			{
				// paste 1022x1022 on borders for seamlessness
				int iX = 1, iY = 0;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 1, iY = 2;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 0, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 2, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				// then paste into surface at 1022x1022 (so can have seamless textures within atlas)
				rcPlate.left = iTexSlotOffsetX+1; rcPlate.top = iTexSlotOffsetY+1;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
			}

			// replace imageTexturePlate with contents of pPlateSurface
			hr = CaptureTexture( m_pD3D, m_pImmediateContext, pPlateSurface, imageTexturePlate );
			if ( SUCCEEDED(hr) )
			{
				// if JPG, can only have one layer
				bool bIsThisAJPG = false;
				if ( strstr ( pPlateFilename.Get(), ".jpg" ) > 0 )
					bIsThisAJPG = true;

				// first create a full mipmap set of images (as only first mipmap layer was affected above)
				if ( bIsThisAJPG == false )
				{
					// create mipmaps
					ScratchImage mipChain;
					hr = GenerateMipMaps( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );

					// compressed or not
					if ( iCompressIt == 0 )
					{
						// save new UNCOMPRESSED texture surface out
						const Image* img = mipChain.GetImages();
						hr = SaveToDDSFile( img, mipChain.GetImageCount(), mipChain.GetMetadata(), DDS_FLAGS_NONE, wFilenamePlate );
					}
					else
					{
						// compress to a DXT5 (BC3) texture
						//hr = Compress( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), 
						//	DXGI_FORMAT_BC3_UNORM, TEX_COMPRESS_DEFAULT, TEX_THRESHOLD_DEFAULT, convertedTexturePlate );
						hr = Compress( mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), 
							DXGI_FORMAT_BC3_UNORM, TEX_COMPRESS_DEFAULT, TEX_THRESHOLD_DEFAULT, convertedTexturePlate );

						// save new texture surface out
						const Image* img = convertedTexturePlate.GetImages();
						hr = SaveToDDSFile( img, convertedTexturePlate.GetImageCount(), convertedTexturePlate.GetMetadata(), DDS_FLAGS_NONE, wFilenamePlate );
					}
				}
				else
				{
					// JPG has no mipmaps
					const Image* img = imageTexturePlate.GetImage(0,0,0);
					hr = SaveToWICFile( *img, DDS_FLAGS_NONE, GUID_ContainerFormatJpeg, wFilenamePlate, NULL );
				}
			}
		}

		// free temp surface captures
		SAFE_RELEASE(pPlateSurface);
		SAFE_RELEASE(pLoadedTexSurface1024);
		SAFE_RELEASE(pLoadedTexSurface512);
		SAFE_RELEASE(pLoadedTexSurface256512);
		SAFE_RELEASE(pLoadedTexSurface512256);
	}
	#else
	// preferred format
	D3DSURFACE_DESC backbufferdesc;
	g_pGlob->pHoldBackBufferPtr->GetDesc ( &backbufferdesc );
	D3DFORMAT d3dFormat = D3DFMT_A8R8G8B8;//backbufferdesc.Format;
	//D3DFORMAT compressedFormat = D3DFMT_DXT1;

	// check if texture to load exists
	GGIMAGE_INFO finfo;
	LPDIRECT3DSURFACE9 pLoadedTexSurface = NULL;
	HRESULT hRes = D3DXGetImageInfoFromFile( pTexFileToLoad, &finfo );

	// create and load the texture selected
	if ( hRes == S_OK )
	{
		// load texture to be inserted
		hRes = m_pD3D->CreateRenderTarget( finfo.Width, finfo.Height, d3dFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &pLoadedTexSurface, NULL);
		hRes = D3DXLoadSurfaceFromFile( pLoadedTexSurface, NULL, NULL, pTexFileToLoad, NULL, D3DX_FILTER_POINT, 0, &finfo );

		// create/load the destination texture plate surface
		LPDIRECT3DTEXTURE9 pTextureDDS;
		LPDIRECT3DSURFACE9 pPlateSurface = NULL;
		cstr pPlateFilename = cstr(pDestTerrainTextureFile);
		hRes = D3DXGetImageInfoFromFile( pPlateFilename.Get(), &finfo );
		if ( hRes != S_OK )
		{
			// if plate file not exist, provide dimensions
			finfo.Width = 4096;
			finfo.Height = 4096;
		}
		m_pD3D->CreateTexture ( finfo.Width, finfo.Height, 1, 0, d3dFormat, D3DPOOL_MANAGED, &pTextureDDS, NULL );
		if ( pTextureDDS )
		{
			pTextureDDS->GetSurfaceLevel ( 0, &pPlateSurface );
			if ( pPlateSurface )
			{
				hRes = D3DXLoadSurfaceFromFile( pPlateSurface, NULL, NULL, pPlateFilename.Get(), NULL, D3DX_FILTER_POINT, 0, &finfo );
			}
		}

		// paste loaded texture into plate (60 in center of atlas texture area)
		if ( pLoadedTexSurface && pPlateSurface ) 
		{
			// work out exact offset to slot position
			int iRow = iWhichTextureOver / 4;
			int iCol = iWhichTextureOver - (iRow*4);
			int iTexSlotOffsetX = iCol * 1024;
			int iTexSlotOffsetY = iRow * 1024;
			RECT rcPlate = RECT();
			rcPlate.left = iTexSlotOffsetX; rcPlate.top = iTexSlotOffsetY; rcPlate.right = iTexSlotOffsetX+1024; rcPlate.bottom = iTexSlotOffsetY+1024;

			// paste to fill 1024x1024 initially (to get at corners)
			hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);

			// paste squashed 256x512 borders to help seamlessness
			// left
			rcPlate.left = iTexSlotOffsetX+0; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256; rcPlate.bottom = iTexSlotOffsetY+256+512;
			hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
			// right
			rcPlate.left = iTexSlotOffsetX+256+512; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+1024; rcPlate.bottom = iTexSlotOffsetY+256+512;
			hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
			// top
			rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+0; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256;
			hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
			// bottom
			rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256+512; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+1024;
			hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);

			// paste insert so smaller 512x512 terrain texture atlas can be seamless
			rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256+512;
			hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
	
			// and finally copy back to LoadTexture
			SAFE_RELEASE ( pLoadedTexSurface );
			hRes = m_pD3D->CreateRenderTarget( finfo.Width, finfo.Height, d3dFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &pLoadedTexSurface, NULL);
			hRes = D3DXLoadSurfaceFromSurface(pLoadedTexSurface, NULL, NULL, pPlateSurface, NULL, NULL, D3DX_DEFAULT, 0);

			// now create the compressed surface for the save
			SAFE_RELEASE ( pPlateSurface );
			SAFE_RELEASE ( pTextureDDS );
			D3DFORMAT d3dChoice = d3dFormat; // no compression, messes up mipmap and filtering
			if ( stricmp ( (pDestTerrainTextureFile + strlen(pDestTerrainTextureFile)) - 6, "_D.dds" ) == NULL ) d3dChoice = D3DFMT_DXT5;
			//m_pD3D->CreateTexture ( finfo.Width, finfo.Height, 0, D3DUSAGE_AUTOGENMIPMAP, d3dChoice, D3DPOOL_MANAGED, &pTextureDDS, NULL );
			m_pD3D->CreateTexture ( finfo.Width, finfo.Height, 1, 0, d3dChoice, D3DPOOL_MANAGED, &pTextureDDS, NULL );
			if ( pTextureDDS )
			{
				// copy texture to DDS compressed texture
				pTextureDDS->GetSurfaceLevel ( 0, &pPlateSurface );
				if ( pPlateSurface )
				{
					hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, NULL, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
				}
				pTextureDDS->GenerateMipSubLevels();
			}

			// save new texture surface out
			D3DXIMAGE_FILEFORMAT DestFormat = D3DXIFF_DDS;
			hRes = D3DXSaveSurfaceToFile( pPlateFilename.Get(), DestFormat, pPlateSurface, NULL, NULL );
			if ( FAILED ( hRes ) )
			{
				char pStrClue[512];
				wsprintf ( pStrClue, "Failed to save new custom texture plate: %s", pDestTerrainTextureFile );
				RunTimeError(RUNTIMEERROR_IMAGEERROR,pStrClue);
				SAFE_RELEASE(pLoadedTexSurface);
				SAFE_RELEASE(pPlateSurface);
				return 0;
			}

			// and the PNG for debugging
			//if(0)
			//{
			//	DestFormat = D3DXIFF_PNG;
			//	cstr pDebugFilePNG = cstr(Left(pPlateFilename.Get(),strlen(pPlateFilename.Get())-4))+cstr(".png");
			//	hRes = D3DXSaveSurfaceToFile( pDebugFilePNG.Get(), DestFormat, pPlateSurface, NULL, NULL );
			//}
		}

		// free temp surface captures
		SAFE_RELEASE(pPlateSurface);
		SAFE_RELEASE(pTextureDDS);
		SAFE_RELEASE(pLoadedTexSurface);
	}
	#endif

	// success
	return 1;
}

void terrain_loadlatesttexture ( void )
{
	// determine location of terrain texture
	char pLocationOfTerrainTexture[512];
	strcpy ( pLocationOfTerrainTexture, cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );

	#ifdef ENABLECUSTOMTERRAIN
	if ( g.terrainstyleindex == 1 ) strcpy ( pLocationOfTerrainTexture, g.mysystem.levelBankTestMap_s.Get() ); //"levelbank\\testmap" );
	#endif

	// load the terrain texture into the terrain object
	SetImageAutoMipMap ( 1 );
	if ( g.gdividetexturesize == 0 ) 
	{
		t.tthistexdir_s="effectbank\\reloaded\\media\\white_D.dds";
		LoadImage ( t.tthistexdir_s.Get(),t.terrain.imagestartindex+13,0,g.gdividetexturesize );
	}
	else
	{
		// new terrain texture technique
		if ( FileExist ( cstr(cstr(pLocationOfTerrainTexture)+"\\"+TEXTURE_D_NAME).Get() ) == 1 )
			LoadImage ( cstr(cstr(pLocationOfTerrainTexture)+"\\"+TEXTURE_D_NAME).Get(),t.terrain.imagestartindex+13,0,g.gdividetexturesize );
		else
			LoadImage ( cstr(cstr(pLocationOfTerrainTexture)+"\\texture_D.dds").Get(),t.terrain.imagestartindex+13,0,g.gdividetexturesize );
	}

	// normals for terrain
	 LoadImage ( "effectbank\\reloaded\\media\\blank_N.dds", t.terrain.imagestartindex+21, 0, g.gdividetexturesize );
	TextureObject ( t.terrain.terrainobjectindex,2,t.terrain.imagestartindex+13 );
	// stage 3 : rem circle texture for highlighter
	TextureObject ( t.terrain.terrainobjectindex,4,t.terrain.imagestartindex+21 );
	SetImageAutoMipMap ( 0 );
}

void terrain_changestyle ( void )
{
	// replace terrain textures with those specified by terrainstyleindex
	SetImageAutoMipMap ( 1 );
	g.terrainstyle_s=t.terrainstylebank_s[g.terrainstyleindex];
	if ( ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
	{
		// if terrain folder exists
		bool bAllowCustomAtOne = false;
		#ifdef ENABLECUSTOMTERRAIN
		if (g.terrainstyleindex == 1) bAllowCustomAtOne = true;
		#endif
		if ( PathExist( cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() ) == 1 || bAllowCustomAtOne == true ) 
		{
			// check if new terrain texture system file available, and create if not
			if ( g.terrainstyleindex >= 1 || (bAllowCustomAtOne==true && g.terrainstyleindex > 1) ) 
			{
				bool bUseNewJPG = true;
				char pNewTerrainTextureFile[512];
				strcpy ( pNewTerrainTextureFile, cstr(cstr("terrainbank\\")+g.terrainstyle_s+"\\"+TEXTURE_D_NAME).Get() );
				if ( FileExist ( pNewTerrainTextureFile ) == 0 ) { bUseNewJPG = false; strcpy ( pNewTerrainTextureFile, cstr(cstr("terrainbank\\")+g.terrainstyle_s+"\\texture_D.dds").Get() ); }
				if ( FileExist ( pNewTerrainTextureFile ) == 0 )
				{
					// create diffuse and normal textures that combine old textures into one atlas of 4x4 textures (4096x4096)
					LPSTR pOldDir = GetDir();
					SetDir ( cstr(cstr("terrainbank\\")+g.terrainstyle_s+"\\").Get() );
					if ( bUseNewJPG == true )
						strcpy ( pNewTerrainTextureFile, TEXTURE_D_NAME );
					else
						strcpy ( pNewTerrainTextureFile, "texture_D.dds" );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 0, "Path_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 1, "Path_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 2, "Path_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 3, "Default_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 4, "Default_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 5, "Default_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 6, "Sedimentary_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 7, "Sedimentary_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 8, "Sedimentary_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 9, "Sedimentary_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 10, "Mossy_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 11, "Mossy_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 12, "Mossy_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 13, "Rocky_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 14, "Rocky_D.dds", 0, 1 );
					terrain_createnewterraintexture ( pNewTerrainTextureFile, 15, "Rocky_D.dds", 0, 1 );
				
					// and now the normals file

					// restore directory after terrain texture file creation
					SetDir ( pOldDir );
				}
			}

			// load terrain textures
			terrain_loadlatesttexture ( );

			// generate super texture from above existing texture
			terrain_generatesupertexture ( false );
		}
	}
	SetImageAutoMipMap (  0 );
}

