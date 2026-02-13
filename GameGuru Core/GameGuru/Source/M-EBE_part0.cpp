//----------------------------------------------------
//--- GAMEGURU - M-EBE
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

//PE: GameGuru IMGUI.
#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"
#include <algorithm>
#include <string>
#include <time.h>

// Enums (duplicated, need to clean this up)
enum MaterialComponentTEXTURESLOT
{
	BASECOLORMAP,
	NORMALMAP,
	SURFACEMAP,
	EMISSIVEMAP,
	DISPLACEMENTMAP,
	OCCLUSIONMAP,
	TRANSMISSIONMAP,
	SHEENCOLORMAP,
	SHEENROUGHNESSMAP,
	CLEARCOATMAP,
	CLEARCOATROUGHNESSMAP,
	CLEARCOATNORMALMAP,
	TEXTURESLOT_COUNT
};

bool dbo2xConvert(LPSTR pFilefrom, LPSTR pFileto);

void set_inputsys_mclick(int value);

// EBE Globals (8MB)
#define CUBEAREASIZE 200
#define CUBEMAXMESH 8000
unsigned char pCubes[CUBEAREASIZE][CUBEAREASIZE][CUBEAREASIZE];
unsigned char pTemp[CUBEAREASIZE][CUBEAREASIZE][CUBEAREASIZE];
DWORD pVertCountStore[CUBEMAXMESH];
bool pbTriggerDrawBufferCreation[CUBEMAXMESH];
short pMasterGridMeshRef[20][20][20][2];

char ActiveEBEFilename[260] = { "\0" };

extern bool bBuilder_Properties_Window;
int texture_set_selection = 0;
char structure_name[MAX_PATH];
extern float fPropertiesColoumWidth;
extern int grideleprof_uniqui_id;
extern bool bForceKey;
extern  cstr csForceKey;
extern int iForceScancode;
int delay_execute = 0;
int skib_frames_execute = 0;
char BuilderPath[MAX_PATH] = "entitybank\\user\\ebestructures";
extern bool bTriggerMessage;
extern char cTriggerMessage[MAX_PATH];
extern preferences pref;

extern bool bImGuiGotFocus;

bool bDisableAllSprites = true;
int iPaintMode = 1;

struct sMyColBox 
{
	int x1;
	int x2;
	int y1;
	int y2;
	int z1;
	int z2;
	int iMaterialIndex;
	unsigned char cCubeTexIndex;
};

//PE: Mesh - Vertex per material has been secured to not allow more then 63135 per mesh, so we can increase MAX Cubes.
#define CUBECOLBOXMAX 5000
sMyColBox pMyColBox[CUBECOLBOXMAX];
#define EBETEXPANELSPRMAX 6

// EBE Local Structure
struct evebuildtexprofiletype
{
	int iWidth;
	int iHeight;
	int iMaterialRef[64];
	cStr sTextureFile[64];
};
struct evebuildpatterntype
{
	int iWidth;
	int iHeight;
	int iDepth;
	int iPreserveMode;
	int iWidthOffset;
	int iDepthOffset;
	cStr pPRow[20][20];
};
struct ebebuildtype
{
	int iToolObj;
	int iBuildObj;
	int iCurrentGridLayer;
	int iLocalKeyPressed;
	int iCursorGridPosX;
	int iCursorGridPosZ;
	int iCursorRotation;
	int iCurrentTexture;
	evebuildpatterntype OriginalPattern;
	evebuildpatterntype Pattern;
	evebuildtexprofiletype TXP;
	int iTexturePanelSprite[6];
	int iTexturePanelImg[6];
	int iTexturePanelX;
	int iTexturePanelY;
	int iTexturePanelWidth;
	int iTexturePanelHeight;
	int iTexturePanelHighSprite;
	int iTexturePanelHighImg;
	int iEBEHelpSpr;
	int iEBEHelpImg;
	int iEBETexHelpSpr;
	int iEBETexHelpImg;
	int iMatSpr[16];
	int iMatImg[19];
	bool bCustomiseTexture;
	int iTexPlateImage;
	int iRefreshBuild;
	int iDeterminedAxisDir;
	int iCursorGridPosLastGoodX;
	int iCursorGridPosLastGoodZ;
};
ebebuildtype ebebuild;

// Undo/Redo Buffers
DWORD g_dwUndoBufferCount = 0;
DWORD* g_pUndoBufferPtr = NULL;
DWORD g_dwRedoBufferCount = 0;
DWORD* g_pRedoBufferPtr = NULL;

void ebe_init ( int BuildObj, int iEntID )
{
	// Basic inits
	ebebuild.bCustomiseTexture = false;
	ebebuild.iRefreshBuild = 0;

	// Create resources for EBE editing
	pastebitmapfontcenterwithboxout("PREPARING STRUCTURE EDITOR SYSTEM",GetChildWindowWidth()/2,40,1,255); Sync();

	// also create destination folder
	char pEBEWriteFolder[MAX_PATH];
	strcpy_s(pEBEWriteFolder, g.fpscrootdir_s.Get());
	strcat_s(pEBEWriteFolder, MAX_PATH, "\\Files\\ebebank\\default\\"); 
	GG_CreatePath(pEBEWriteFolder);

	// Create a simple floor
	unsigned char cTexIndex = 0;

	// Create texture selection panel
	ebebuild.iTexturePanelX = GetChildWindowWidth()-210;
	ebebuild.iTexturePanelY = GetChildWindowHeight()-210;
	ebebuild.iTexturePanelWidth = 200;
	ebebuild.iTexturePanelHeight = 200;
	image_setlegacyimageloading(true);
	for ( int iTex = 0; iTex < EBETEXPANELSPRMAX; iTex++ )
	{
		LPSTR pTexImg = "";
		int iX = ebebuild.iTexturePanelX;
		int iY = ebebuild.iTexturePanelY;
		int iWidth = ebebuild.iTexturePanelWidth;
		int iHeight = ebebuild.iTexturePanelHeight;
		if ( iTex==0 ) { pTexImg = "TexHUD-F.png"; iX -= 10; iY -= 10; iWidth += 20; iHeight += 20; }
		if ( iTex==1 ) { pTexImg = "TexHUD-L.png"; iX -= 10; iY -= 10; iWidth += 20; iHeight = 1; }
		if ( iTex==2 ) { pTexImg = "TexHUD-L.png"; iX -= 10; iY += 209; iWidth += 20; iHeight = 1; }
		if ( iTex==3 ) { pTexImg = "TexHUD-L.png"; iX -= 10; iY -= 10; iWidth = 1; iHeight += 20; }
		if ( iTex==4 ) { pTexImg = "TexHUD-L.png"; iX += 209; iY -= 10; iWidth = 1; iHeight += 20; }
		if ( iTex==5 ) { pTexImg = "textures_color.dds"; }
		ebebuild.iTexturePanelSprite[iTex] = g.ebeinterfacesprite + 31 + iTex;
		ebebuild.iTexturePanelImg[iTex] = loadinternalimage(cstr(cstr("ebebank\\default\\")+cstr(pTexImg)).Get());
		if(!bDisableAllSprites)
			MAXSprite ( ebebuild.iTexturePanelSprite[iTex], iX, iY, ebebuild.iTexturePanelImg[iTex] );
		if (!bDisableAllSprites) SizeSprite ( ebebuild.iTexturePanelSprite[iTex], iWidth, iHeight );
		if ( iTex==5 ) ebebuild.iTexPlateImage = ebebuild.iTexturePanelImg[iTex];
	}
	image_setlegacyimageloading(false);

	// Texture highlighter
	ebebuild.iTexturePanelHighSprite = g.ebeinterfacesprite + 0;
	ebebuild.iTexturePanelHighImg = loadinternalimage("ebebank\\default\\TextureHighlighter.dds");
	if (!bDisableAllSprites) MAXSprite ( ebebuild.iTexturePanelHighSprite, ebebuild.iTexturePanelX, ebebuild.iTexturePanelY, ebebuild.iTexturePanelHighImg );
	if (!bDisableAllSprites) SizeSprite ( ebebuild.iTexturePanelHighSprite, ebebuild.iTexturePanelWidth/4, ebebuild.iTexturePanelHeight/4 );

	// Help Dialog Shortcut Keys
	ebebuild.iEBEHelpSpr = g.ebeinterfacesprite + 1;
	ebebuild.iEBEHelpImg = loadinternalimage("languagebank\\english\\artwork\\ebe-help.png");
	if (!bDisableAllSprites) MAXSprite ( ebebuild.iEBEHelpSpr, ebebuild.iTexturePanelX - ImageWidth(ebebuild.iEBEHelpImg) - 10, ebebuild.iTexturePanelY + 210 - ImageHeight(ebebuild.iEBEHelpImg), ebebuild.iEBEHelpImg );

	// Help Dialog Shortcut Keys
	ebebuild.iEBETexHelpSpr = g.ebeinterfacesprite + 2;
    ebebuild.iEBETexHelpImg = loadinternalimage("languagebank\\english\\artwork\\ebe-texturehelp.png");
	if (!bDisableAllSprites) MAXSprite ( ebebuild.iEBETexHelpSpr, ebebuild.iTexturePanelX - 10, ebebuild.iTexturePanelY - 10 - ImageHeight(ebebuild.iEBETexHelpImg), ebebuild.iEBETexHelpImg );

	// Load TXP default file
	ebe_loadtxp(cstr(cstr("ebebank\\default\\")+cstr("textures_profile.txp")).Get());

	// Material index overlays and sprites to place over texture highlighter selection
	for ( int n = 0; n <= 18; n++ )
	{
		cstr sMatIconFile = "";
		if ( n == 0 ) sMatIconFile = "ebe-material-0-gen.png";
		if ( n == 1 ) sMatIconFile = "ebe-material-1-sto.png";
		if ( n == 2 ) sMatIconFile = "ebe-material-2-met.png";
		if ( n == 3 ) sMatIconFile = "ebe-material-3-woo.png";
		if ( n == 4 ) sMatIconFile = "ebe-material-4.png";
		if ( n == 5 ) sMatIconFile = "ebe-material-5.png";
		if ( n == 6 ) sMatIconFile = "ebe-material-6.png";
		if ( n == 7 ) sMatIconFile = "ebe-material-7.png";
		if ( n == 8 ) sMatIconFile = "ebe-material-8.png";
		if ( n == 9 ) sMatIconFile = "ebe-material-9.png";
		if ( n == 10 ) sMatIconFile = "ebe-material-10.png";
		if ( n == 11 ) sMatIconFile = "ebe-material-11.png";
		if ( n == 12 ) sMatIconFile = "ebe-material-12.png";
		if ( n == 13 ) sMatIconFile = "ebe-material-13.png";
		if ( n == 14 ) sMatIconFile = "ebe-material-14.png";
		if ( n == 15 ) sMatIconFile = "ebe-material-15.png";
		if ( n == 16 ) sMatIconFile = "ebe-material-16.png";
		if ( n == 17 ) sMatIconFile = "ebe-material-17.png";
		if ( n == 18 ) sMatIconFile = "ebe-material-18.png";
		ebebuild.iMatImg[n] = loadinternalimage(cstr(cstr("languagebank\\english\\artwork\\")+sMatIconFile).Get());
	}
	int n = 0;
	for ( int y = 0; y < 4; y++ )
	{
		for ( int x = 0; x < 4; x++ )
		{
			ebebuild.iMatSpr[n] = g.ebeinterfacesprite + 11 + n;
			if (!bDisableAllSprites) MAXSprite ( ebebuild.iMatSpr[n], ebebuild.iTexturePanelX + 36 + (x*50), ebebuild.iTexturePanelY + 36 + (y*50), ebebuild.iMatImg[ebebuild.TXP.iMaterialRef[n]] );
			if (!bDisableAllSprites) SizeSprite ( ebebuild.iMatSpr[n], 13, 13 );
			n++;
		}
	}

	// Create Building Site Tool Object
	int iToolObj = g.ebeobjectbankoffset + 0;
	if (ObjectExist(iToolObj) == 1) DeleteObject(iToolObj);
	MakeObjectPlane ( iToolObj, 100, 100 );
	if (GetMeshExist(g.meshgeneralwork) == 1) DeleteMesh(g.meshgeneralwork);
	MakeMeshFromObject ( g.meshgeneralwork, iToolObj );
	DeleteObject ( iToolObj );
	MakeObjectCube ( iToolObj, 5.0f );
	if (GetMeshExist(g.meshebe1) == 1) DeleteMesh(g.meshebe1);
	MakeMeshFromObject ( g.meshebe1, iToolObj );
	DeleteObject ( iToolObj );
	MakeObjectBox ( iToolObj, 1000, 0.1f, 1000 );
	if (GetMeshExist(g.meshebe) == 1) DeleteMesh(g.meshebe);
	MakeMeshFromObject ( g.meshebe, iToolObj );

	// Grid Limbs (0-base grid, 1-floating grid for current edit layer)
	image_setlegacyimageloading(true);
	int iGridImg = loadinternaltexture("ebebank\\default\\GridBox.dds");
	int iFloatImg = loadinternaltexture("ebebank\\default\\FloatBox.dds");
	int iCursorImg = loadinternaltexture("ebebank\\default\\CursorBox.dds");	
	image_setlegacyimageloading(false);
	// base mesh is wrong, so we get rid of it
	int iLimbIndex = 0;
	OffsetLimb (iToolObj, iLimbIndex, 0, 0.05, 0);
	ScaleLimb(iToolObj, iLimbIndex, 0, 0, 0);
	iLimbIndex = 1;
	AddLimb (iToolObj, iLimbIndex, g.meshebe);
	OffsetLimb (iToolObj, iLimbIndex, 500, 0.05, 500);

	// now scale present meshes so grid creates a mirror effect of UVs for good textured grid
	ScaleObjectTexture ( iToolObj, 200, 200 );

	// Cursor Limb (2-can be used to show current mouse cursor within grid)
	iLimbIndex = 2;
	AddLimb ( iToolObj, iLimbIndex, g.meshebe1 );
	OffsetLimb ( iToolObj, iLimbIndex, 2.5f, 2.5f, 2.5f );

	// Replace default texture with grid textures
	TextureLimbStage ( iToolObj, 0, 0, iGridImg );
	TextureLimbStage ( iToolObj, 1, 0, iFloatImg );
	TextureLimbStage ( iToolObj, 2, 0, iCursorImg );
	SetObjectTransparency ( iToolObj, 6 );
	DisableObjectZWrite ( iToolObj );

	// Apply entity shader
	int iToolEffectIndex = loadinternaleffect("effectbank\\reloaded\\ebe_basic.fx");
	SetObjectEffect ( iToolObj, iToolEffectIndex );
	SetObjectMask ( iToolObj, 0x1 );

	// Create invisible obj to cast for object selection detection
	
	// New template has marker in limb 2, regular EBE structure has it in zero
	int iLimbWithMarker = 0;
	if ( stricmp ( Right ( t.entitybank_s[iEntID].Get(), 21), "_builder\\New Site.fpe" ) == NULL )
		iLimbWithMarker = 2;

	// Create ebe marker mesh from first 
	if (GetMeshExist(g.meshebemarker) == 1) DeleteMesh(g.meshebemarker);
	MakeMeshFromLimb ( g.meshebemarker, BuildObj, iLimbWithMarker );

	image_setlegacyimageloading(true);
	if (!ImageExist(EBE_CONTROL1))
		LoadImage("editors\\uiv3\\ebe-control1.png", EBE_CONTROL1);
	if (!ImageExist(EBE_CONTROL2))
		LoadImage("editors\\uiv3\\ebe-control2.png", EBE_CONTROL2);
	if (!ImageExist(EBE_CONTROL3))
		LoadImage("editors\\uiv3\\ebe-control3.png", EBE_CONTROL3);
	if (!ImageExist(EBE_CONTROL4))
		LoadImage("editors\\uiv3\\ebe-control4.png", EBE_CONTROL4);
	if (!ImageExist(EBE_CONTROL5))
		LoadImage("editors\\uiv3\\ebe-control5.png", EBE_CONTROL5);
	if (!ImageExist(EBE_CONTROL6))
		LoadImage("editors\\uiv3\\ebe-control6.png", EBE_CONTROL6);
	if (!ImageExist(EBE_THUMB))
		LoadImage("editors\\uiv3\\ebe-thumb.png", EBE_THUMB);
	image_setlegacyimageloading(false);

	WickedCall_PresetObjectLimbRenderLayer(GGRENDERLAYERS_CURSOROBJECT,2);
	sObject* pToolObject = GetObjectData(iToolObj);
	WickedCall_RemoveObject(pToolObject);
	WickedCall_AddObject(pToolObject);
	WickedCall_UpdateObject(pToolObject);
	WickedCall_TextureObject(pToolObject,NULL);
	WickedCall_SetObjectTransparent(pToolObject);
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);

	// mark EBE has intialised
	t.ebe.active = 1;
}

void ebe_free(void)
{
	// delete all resources created in above _init
	for ( int iTex = 0; iTex < EBETEXPANELSPRMAX; iTex++ )
	{
		if (ebebuild.iTexturePanelImg[iTex] > 0)
		{
			if (ImageExist(ebebuild.iTexturePanelImg[iTex]) == 1)
			{
				removeinternalimage(ebebuild.iTexturePanelImg[iTex]);
			}
			ebebuild.iTexturePanelImg[iTex] = 0;
		}
		if (SpriteExist(ebebuild.iTexturePanelSprite[iTex]) == 1)
		{
			DeleteSprite(ebebuild.iTexturePanelSprite[iTex]);
		}
		ebebuild.iTexturePanelSprite[iTex] = 0;
	}
	ebebuild.iTexPlateImage = 0;

	if (ebebuild.iTexturePanelHighSprite > 0) if (SpriteExist(ebebuild.iTexturePanelHighSprite) == 1) DeleteSprite(ebebuild.iTexturePanelHighSprite);
	if (ebebuild.iTexturePanelHighImg > 0) if (ImageExist(ebebuild.iTexturePanelHighImg) == 1) removeinternalimage(ebebuild.iTexturePanelHighImg);
	ebebuild.iTexturePanelHighSprite = 0;
	ebebuild.iTexturePanelHighImg = 0;

	if (ebebuild.iEBEHelpSpr > 0) if (SpriteExist(ebebuild.iEBEHelpSpr) == 1) DeleteSprite(ebebuild.iEBEHelpSpr);
	if (ebebuild.iEBEHelpImg > 0) if (ImageExist(ebebuild.iEBEHelpImg) == 1) removeinternalimage(ebebuild.iEBEHelpImg);
	ebebuild.iEBEHelpSpr = 0;
	ebebuild.iEBEHelpImg = 0;

	if (ebebuild.iEBETexHelpSpr > 0) if (SpriteExist(ebebuild.iEBETexHelpSpr) == 1) DeleteSprite(ebebuild.iEBETexHelpSpr);
	if (ebebuild.iEBETexHelpImg > 0) if (ImageExist(ebebuild.iEBETexHelpImg) == 1) removeinternalimage(ebebuild.iEBETexHelpImg);
	ebebuild.iEBETexHelpSpr = 0;
	ebebuild.iEBETexHelpImg = 0;

	for ( int n = 0; n <= 18; n++ )
	{
		if ( ebebuild.iMatImg[n]> 0) if (ImageExist(ebebuild.iMatImg[n]) == 1) removeinternalimage(ebebuild.iMatImg[n]);
		ebebuild.iMatImg[n] = 0;
	}
	int n = 0;
	for ( int y = 0; y < 4; y++ )
	{
		for ( int x = 0; x < 4; x++ )
		{
			if (ebebuild.iMatSpr[n] > 0) if (SpriteExist(ebebuild.iMatSpr[n]) == 1) DeleteSprite(ebebuild.iMatSpr[n]);
			ebebuild.iMatSpr[n] = 0;
			n++;
		}
	}

	t.ebe.active = 0;
}

void ebe_init_newbuild ( int iBuildObj, int entid )
{
	// ensure EBE system is initialised
	if (t.ebe.active == 0)
	{
		ebe_init(iBuildObj, entid);
		t.grideleprof.bCustomWickedMaterialActive = false;
	}

	// load TXP profile from entity data
	// and update visuals for the material ref icons
	if ( t.entityprofile[entid].ebe.dwMatRefCount > 0 )
	{
		int ncount = t.entityprofile[entid].ebe.dwMatRefCount;
		for ( int n = 0; n < ncount; n++ )
		{
			ebebuild.TXP.iMaterialRef[n] = t.entityprofile[entid].ebe.iMatRef[n];
			if (!bDisableAllSprites) MAXSprite ( ebebuild.iMatSpr[n], SpriteX(ebebuild.iMatSpr[n]), SpriteY(ebebuild.iMatSpr[n]), ebebuild.iMatImg[ebebuild.TXP.iMaterialRef[n]] );
		}
	}

	// detect if texture ref profile differs from current one
	bool bTexturePlateDifferent = false;
	if ( t.entityprofile[entid].ebe.dwTexRefCount > 0 )
	{
		int ncount = t.entityprofile[entid].ebe.dwTexRefCount;
		for ( int n = 0; n < ncount; n++ )
		{
			if ( stricmp ( ebebuild.TXP.sTextureFile[n].Get(), t.entityprofile[entid].ebe.pTexRef[n] ) != NULL )
			{
				bTexturePlateDifferent = true;
			}
		}
	}
	if ( bTexturePlateDifferent == true )
	{
		// update editor textures to match incoming EBE structure
		// by loading it direct from the EBE special long named textures and also the TXP file
		int ncount = t.entityprofile[entid].ebe.dwTexRefCount;
		for ( int n = 0; n < ncount; n++ )
			ebebuild.TXP.sTextureFile[n] = t.entityprofile[entid].ebe.pTexRef[n];

		// Also save snapshot of latest textures_DNS
		cstr sUniqueFilename = ebe_constructlongTXPname("_color.dds");
		cstr sLongFilename = cstr("ebebank\\default\\") + sUniqueFilename;
		cstr tRawPathAndFile = cstr(Left(sLongFilename.Get(),strlen(sLongFilename.Get())-10));
		cstr sDDSFile = tRawPathAndFile + cstr("_color.dds");
		if ( FileExist(sDDSFile.Get()) == 0 ) 
		{
			// cache in ebebank\default deleted, so copy from
			cstr tSourceRaw = g.mysystem.levelBankTestMap_s + sUniqueFilename; //cstr("levelbank\\testmap\\") + sUniqueFilename;
			tSourceRaw = cstr(Left(tSourceRaw.Get(),strlen(tSourceRaw.Get())-10));
			cstr tDDSSource = tSourceRaw + "_color.dds";
			if ( FileExist(tDDSSource.Get()) == 0 ) 
			{
				// not in ebebank or in levelbank, try original saved ebe entity (though a real entity should never get here - only their entitybank copies)
				tSourceRaw = cstr("entitybank\\")+cstr(t.entitybank_s[entid]) + "\\" + sUniqueFilename;
				tDDSSource = tSourceRaw + "_color.dds";
			}
			sDDSFile = tRawPathAndFile + cstr("_color.dds");
			char pRealDestFile[MAX_PATH];
			strcpy(pRealDestFile, sDDSFile.Get());
			GG_GetRealPath(pRealDestFile, 1);
			sDDSFile = pRealDestFile;
			CopyFileA ( tDDSSource.Get(), sDDSFile.Get(), FALSE );
			 tDDSSource = tSourceRaw + "_normal.dds";
			 sDDSFile = tRawPathAndFile + cstr("_normal.dds");
			 strcpy(pRealDestFile, sDDSFile.Get());
			 GG_GetRealPath(pRealDestFile, 1);
			 sDDSFile = pRealDestFile;
			 CopyFileA ( tDDSSource.Get(), sDDSFile.Get(), FALSE );
			 tDDSSource = tSourceRaw + "_surface.dds";
			 sDDSFile = tRawPathAndFile + cstr("_surface.dds");
			 strcpy(pRealDestFile, sDDSFile.Get());
			 GG_GetRealPath(pRealDestFile, 1);
			 sDDSFile = pRealDestFile;
			 CopyFileA ( tDDSSource.Get(), sDDSFile.Get(), FALSE );
			sDDSFile = tRawPathAndFile + cstr("_color.dds");
		}
		if ( FileExist(sDDSFile.Get()) == 1 ) 
		{
			cstr tDDSFilename = "ebebank\\default\\textures_color.dds";
			char pRealSrcFile[MAX_PATH];
			strcpy(pRealSrcFile, sDDSFile.Get());
			GG_GetRealPath(pRealSrcFile, 1);
			sDDSFile = pRealSrcFile;
			char pRealDestFile[MAX_PATH];
			strcpy(pRealDestFile, tDDSFilename.Get());
			GG_GetRealPath(pRealDestFile, 1);
			tDDSFilename = pRealDestFile;
			if ( FileExist(tDDSFilename.Get()) == 1 ) DeleteFileA ( tDDSFilename.Get() );
			CopyFileA ( sDDSFile.Get(), tDDSFilename.Get(), FALSE );
			 sDDSFile = tRawPathAndFile + cstr("_normal.dds");
			 tDDSFilename = "ebebank\\default\\textures_normal.dds";
			 strcpy(pRealSrcFile, sDDSFile.Get());
			 GG_GetRealPath(pRealSrcFile, 1);
			 sDDSFile = pRealSrcFile;
			 strcpy(pRealDestFile, tDDSFilename.Get());
			 GG_GetRealPath(pRealDestFile, 1);
			 tDDSFilename = pRealDestFile;
			 if ( FileExist(tDDSFilename.Get()) == 1 ) DeleteAFile ( tDDSFilename.Get() );
			 CopyFileA ( sDDSFile.Get(), tDDSFilename.Get(), FALSE );
			 sDDSFile = tRawPathAndFile + cstr("_surface.dds");
			 tDDSFilename = "ebebank\\default\\textures_surface.dds";
			 strcpy(pRealSrcFile, sDDSFile.Get());
			 GG_GetRealPath(pRealSrcFile, 1);
			 sDDSFile = pRealSrcFile;
			 strcpy(pRealDestFile, tDDSFilename.Get());
			 GG_GetRealPath(pRealDestFile, 1);
			 tDDSFilename = pRealDestFile;
			 if ( FileExist(tDDSFilename.Get()) == 1 ) DeleteAFile ( tDDSFilename.Get() );
			 CopyFileA ( sDDSFile.Get(), tDDSFilename.Get(), FALSE );
		}

		// and the TXP file too (260317 - generate, as copy may not exist)
		ebe_savetxp(cstr(cstr("ebebank\\default\\")+cstr("textures_profile.txp")).Get());

		// and update EBE editor textures image for selector
		image_setlegacyimageloading(true);
		LoadImage ( "ebebank\\default\\textures_color.dds", ebebuild.iTexPlateImage );
		image_setlegacyimageloading(false);
		if (!bDisableAllSprites) MAXSprite ( ebebuild.iTexturePanelSprite[5], SpriteX(ebebuild.iTexturePanelSprite[5]), SpriteY(ebebuild.iTexturePanelSprite[5]), ebebuild.iTexPlateImage );
	}

	// shift grid object away so don't see last incarnation of it
	if ( ebebuild.iToolObj > 0 )
		if ( ObjectExist ( ebebuild.iToolObj ) == 1 )
			PositionObject ( ebebuild.iToolObj, -999999, -999999, -999999 );

	// Wipe cube building site
	memset ( pCubes, 0, sizeof(pCubes) );

	// Create Building Site Object from original entity element OBJ mesh
	float fRX = ObjectAngleX(iBuildObj);
	float fRY = ObjectAngleY(iBuildObj);
	float fRZ = ObjectAngleZ(iBuildObj);
	ebebuild.iBuildObj = iBuildObj;
	DeleteObject ( iBuildObj );

	// Start with the marker mesh
	MakeObject ( iBuildObj, g.meshebemarker, 0 );
	RotateObject ( iBuildObj, fRX, fRY, fRZ );
	SetSphereRadius ( iBuildObj, 0 );

	// Assign slot indexes for 3D grid volume
	int iSlotIndex = 1;
	for ( int y = 0; y <= CUBEAREASIZE-10; y+=10 )
	{
		for ( int z = 0; z <= CUBEAREASIZE-10; z+=10 )
		{
			for ( int x = 0; x <= CUBEAREASIZE-10; x+=10 )
			{
				pMasterGridMeshRef[x/10][y/10][z/10][0] = iSlotIndex;
				pMasterGridMeshRef[x/10][y/10][z/10][1] = 0;
				pVertCountStore[iSlotIndex] = 0;
				iSlotIndex++;
			}
		}
	}

	// Apply textures
	image_setlegacyimageloading(true);
 	 int iTexD = loadinternaltexture("ebebank\\default\\textures_color.dds");
	 int iTexN = loadinternaltexture("ebebank\\default\\textures_normal.dds");
	 int iTexS = loadinternaltexture("ebebank\\default\\textures_surface.dds");
	 TextureObject(iBuildObj, iTexD);
	 image_setlegacyimageloading(false);
	SetObjectTransparency ( iBuildObj, 0 );

	// unpack data into cube data
	DWORD dwRLESize = t.entityprofile[entid].ebe.dwRLESize;
	if ( dwRLESize > 0 )
	{
		DWORD* pRLEData = t.entityprofile[entid].ebe.pRLEData;
		ebe_unpacksite ( dwRLESize, pRLEData );
	}

	// Refresh mesh with cube construction
	ebe_refreshmesh ( iBuildObj, 0, 0, 0, CUBEAREASIZE-10, CUBEAREASIZE-11, CUBEAREASIZE-10 );

	// aply shader effect (now done inside refresh)
	int iEBEEffectIndex = loadinternaleffect("effectbank\\reloaded\\ebe_basic.fx");
	SetLimbEffect ( iBuildObj, 0, iEBEEffectIndex );

	// Only render EBE to main camera and shadow camera
	SetObjectMask ( iBuildObj, 1+(1<<31) );

	// In case new 'shader' associated with new entity, refresh please
	visuals_justshaderupdate ( );

	// the EBE construction build object
	sObject* pObject = g_ObjectList [ iBuildObj ];

	// Ensures col not calculated in main sync render loop
	UpdateColCenter ( pObject );

	// EBE construction object cannot have collision active (so raycast skips its half-baked meshes)
	SetColOff ( pObject );


//	if (t.grideleprof.bCustomWickedMaterialActive) {
//		Wicked_Copy_Material_To_Grideleprof((void*)pObject, 3);
//		Wicked_Update_All_Materials((void*)pObject, 3);
//	}
	int e = t.ebe.entityelementindex;
	if (t.entityelement[e].eleprof.bCustomWickedMaterialActive) {
		t.grideleprof.WEMaterial = t.entityelement[e].eleprof.WEMaterial;
		t.grideleprof.bCustomWickedMaterialActive = t.entityelement[e].eleprof.bCustomWickedMaterialActive;
		Wicked_Set_Material_From_grideleprof((void*)pObject, 3);
		//PE: Finally copy material to all meshed in new structure.
		Wicked_Update_All_Materials((void*)pObject, 3);
	}


}

void ebe_updateparent ( int entityelementindex )
{
	// early exits
	if ( entityelementindex == 0 ) return;

	// before can delete a parent, must delete all instance objects to it
	int iEntityBankID = t.entityelement[entityelementindex].bankindex;
	t.sourceobj = g.entitybankoffset + iEntityBankID;
	if ( ObjectExist ( t.sourceobj ) == 1 )
	{
		// go through all entity elements to look for instances of parent
		for ( int te = 1 ; te <= g.entityelementlist; te++ )
		{
			if ( te != entityelementindex )
			{
				int entid = t.entityelement[te].bankindex;
				if ( entid == iEntityBankID )
				{
					int iObj = t.entityelement[te].obj;
					if ( iObj > 0 ) 
					{
						DeleteObject ( iObj );
					}
				}
			}
		}
		// now delete old parent
		DeleteObject ( t.sourceobj );
	}

	// create fresh entity profile parent object from latest optimized 
	int iEntityElementObj = t.entityelement[entityelementindex].obj;

	// Clone new parent from above working object
	CloneObject ( t.sourceobj, iEntityElementObj );
	PositionObject ( t.sourceobj, -999999, -999999, -999999 );

	// finally create new instances from revised parent
	for ( int te = 1 ; te <= g.entityelementlist; te++ )
	{
		if ( te != entityelementindex )
		{
			int entid = t.entityelement[te].bankindex;
			if ( entid == iEntityBankID )
			{
				int iObj = t.entityelement[te].obj;
				if ( iObj > 0 ) 
				{
					// must create clone state and instance object initially  
					t.tte = te;
					t.entityelement[t.tte].isclone = 1;
					InstanceObject ( iObj, t.sourceobj );

					// then this function can recreate the instance properly with positions and settings
					entity_converttoinstance();
				}
			}
		}
	}
}

void ebe_freecubedata ( int entitybankindex )
{
	//PE: Get a crash here pRLEData == NULL
	if(t.entityprofile[entitybankindex].ebe.pRLEData)
		SAFE_DELETE ( t.entityprofile[entitybankindex].ebe.pRLEData );
	t.entityprofile[entitybankindex].ebe.dwRLESize = 0; 
	if(t.entityprofile[entitybankindex].ebe.iMatRef)
		SAFE_DELETE ( t.entityprofile[entitybankindex].ebe.iMatRef );
	t.entityprofile[entitybankindex].ebe.dwMatRefCount = 0; 
	if(t.entityprofile[entitybankindex].ebe.pTexRef)
		SAFE_DELETE ( t.entityprofile[entitybankindex].ebe.pTexRef );
	t.entityprofile[entitybankindex].ebe.dwTexRefCount = 0; 
}

void ebe_makeseamless ( int iRow, int iCol, float* fU1, float* fU1R, float* fV1, float* fV1R, float* fW1, float* fZ1, float* fW1R, float* fZ1R, float* fU2, float* fU2R, float* fV2, float* fW2, float* fZ2, float* fW2R, float* fZ2R )
{
	// shrink UV coords to fit into 1022x1022 tiling (seamless trick)
	*fU1 -= (0.25f) * iCol;
	*fV1 -= (0.25f) * iRow;
	*fU2 -= (0.25f) * iCol;
	*fV2 -= (0.25f) * iRow;
	*fU1R -= (0.25f) * iCol;
	*fU2R -= (0.25f) * iCol;
	*fZ1R -= (0.25f) * iRow;
	*fZ2R -= (0.25f) * iRow;
	*fZ1 -= (0.25f) * iRow;
	*fZ2 -= (0.25f) * iRow;
	*fW1 -= (0.25f) * iCol;
	*fW2 -= (0.25f) * iCol;
	*fW1R -= (0.25f) * iCol;
	*fW2R -= (0.25f) * iCol;
	float fPixelBorder = 1.0f;
	float fShrinkDest = 1022.0f;
	*fU1 = (*fU1/1024.0f) * fShrinkDest;
	*fV1 = (*fV1/1024.0f) * fShrinkDest;
	*fU2 = (*fU2/1024.0f) * fShrinkDest;
	*fV2 = (*fV2/1024.0f) * fShrinkDest;
	*fU1R = (*fU1R/1024.0f) * fShrinkDest;
	*fU2R = (*fU2R/1024.0f) * fShrinkDest;
	*fZ1R = (*fZ1R/1024.0f) * fShrinkDest;
	*fZ2R = (*fZ2R/1024.0f) * fShrinkDest;
	*fZ1 = (*fZ1/1024.0f) * fShrinkDest;
	*fZ2 = (*fZ2/1024.0f) * fShrinkDest;
	*fW1 = (*fW1/1024.0f) * fShrinkDest;
	*fW2 = (*fW2/1024.0f) * fShrinkDest;
	*fW1R = (*fW1R/1024.0f) * fShrinkDest;
	*fW2R = (*fW2R/1024.0f) * fShrinkDest;
	*fU1 += (0.25f/1024.0f) * fPixelBorder;
	*fV1 += (0.25f/1024.0f) * fPixelBorder;
	*fU2 += (0.25f/1024.0f) * fPixelBorder;
	*fV2 += (0.25f/1024.0f) * fPixelBorder;
	*fU1R += (0.25f/1024.0f) * fPixelBorder;
	*fU2R += (0.25f/1024.0f) * fPixelBorder;
	*fZ1R += (0.25f/1024.0f) * fPixelBorder;
	*fZ2R += (0.25f/1024.0f) * fPixelBorder;
	*fZ1 += (0.25f/1024.0f) * fPixelBorder;
	*fZ2 += (0.25f/1024.0f) * fPixelBorder;
	*fW1 += (0.25f/1024.0f) * fPixelBorder;
	*fW2 += (0.25f/1024.0f) * fPixelBorder;
	*fW1R += (0.25f/1024.0f) * fPixelBorder;
	*fW2R += (0.25f/1024.0f) * fPixelBorder;
	*fU1 += (0.25f) * iCol;
	*fV1 += (0.25f) * iRow;
	*fU2 += (0.25f) * iCol;
	*fV2 += (0.25f) * iRow;
	*fU1R += (0.25f) * iCol;
	*fU2R += (0.25f) * iCol;
	*fZ1R += (0.25f) * iRow;
	*fZ2R += (0.25f) * iRow;
	*fZ1 += (0.25f) * iRow;
	*fZ2 += (0.25f) * iRow;
	*fW1 += (0.25f) * iCol;
	*fW2 += (0.25f) * iCol;
	*fW1R += (0.25f) * iCol;
	*fW2R += (0.25f) * iCol;
}

void ebe_refreshmesh ( int iBuildObj, int x1, int y1, int z1, int x2, int y2, int z2 )
{
	// Refresh mesh with cube construction
	sObject* pObject = GetObjectData ( iBuildObj );
	if ( pObject )
	{
		// object going to be extensively modified
		WickedCall_RemoveObject(pObject);
		WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
		// so we can handle custom material applied to this EBE
		if (t.ebe.entityelementindex > 0)
		{
			WickedSetElementId(t.ebe.entityelementindex);
			WickedSetEntityId(t.entityelement[t.ebe.entityelementindex].bankindex);
		}

		// may need this to map effect to newly created limb
		int iEBEEffectIndex = loadinternaleffect("effectbank\\reloaded\\ebe_basic.fx");

		// count verts in all grid locations
		// fixed 50x50x50 cube sliced into 5 units (10x10x10)
		for ( int y = y1; y <= y2; y+=10 )
		{
			for ( int z = z1; z <= z2; z+=10 )
			{
				for ( int x = x1; x <= x2; x+=10 )
				{
					// count verts only for this slot (mesh/limb collection)
					DWORD dwVertPos = 0;
					DWORD dwIndicePos = 0;

					// this step will create a single mesh for the 10x10x10 cube collection at this mastergrid ref (x,y,z)
					for ( int yb = 0; yb < 10; yb++ )
					{
						for ( int xb = 0; xb < 10; xb++ )
						{
							for ( int zb = 0; zb < 10; zb++ )
							{
								// should we create a cube here
								int iAbsX = x+xb;
								int iAbsY = y+yb;
								int iAbsZ = z+zb;
								unsigned char cCubeCode = pCubes[iAbsX][iAbsY][iAbsZ];
								if ( cCubeCode > 0)
								{
									// next cube
									dwVertPos+=24;
									dwIndicePos+=36;
								}
							}
						}
					}

					// store verts for this slot and increment slot index
					int iSlotIndex = pMasterGridMeshRef[x/10][y/10][z/10][0];
					pVertCountStore[iSlotIndex] = dwVertPos;
				}
			}
		}

		// clear trigger flags
		for ( int iLimbIndex = 0; iLimbIndex < CUBEMAXMESH; iLimbIndex++ )
			pbTriggerDrawBufferCreation[iLimbIndex] = false;

		// create meshes from above verts collected
		DWORD dwColor = GGCOLOR(1,1,1,1);
		float fU = 1.0f/4.0f;
		float fV = 1.0f/4.0f;
		for ( int y = y1; y <= y2; y+=10 )
		{
			for ( int z = z1; z <= z2; z+=10 )
			{
				for ( int x = x1; x <= x2; x+=10 )
				{
					// record slot and limb index at the slot location
					int iMasterGridRefX = x / 10;
					int iMasterGridRefY = y / 10;
					int iMasterGridRefZ = z / 10;
					int iSlotIndex = pMasterGridMeshRef[iMasterGridRefX][iMasterGridRefY][iMasterGridRefZ][0];
					int iLimbIndex = pMasterGridMeshRef[iMasterGridRefX][iMasterGridRefY][iMasterGridRefZ][1];

					// if need a new mesh, create it here
					if ( iLimbIndex == 0 && pVertCountStore[iSlotIndex] > 0 )
					{
						iLimbIndex = pObject->iFrameCount;
						AddLimb ( iBuildObj, iLimbIndex, g.meshebe1 );
						HideLimb ( iBuildObj, iLimbIndex );
						pObject->ppFrameList[iLimbIndex]->pMesh->dwTextureCount = 1;
						pObject->ppFrameList[iLimbIndex]->pMesh->pTextures = new sTexture[1];
						sMesh* pDestMesh = pObject->ppFrameList[iLimbIndex]->pMesh;
						CloneInternalTextures ( pDestMesh, pObject->ppMeshList[0] );
						pMasterGridMeshRef[iMasterGridRefX][iMasterGridRefY][iMasterGridRefZ][1] = iLimbIndex;
					}

					// set up mesh for exact number of cubes
					sMesh* pMesh = pObject->ppFrameList[iLimbIndex]->pMesh;
					DWORD dwVertPos = pVertCountStore[iSlotIndex];
					DWORD dwIndicePos = (dwVertPos/2)*3;
					SetupMeshFVFData ( pMesh, pMesh->dwFVF, dwVertPos, dwIndicePos, false );
					pMesh->iPrimitiveType = GGPT_TRIANGLELIST;
					pMesh->iDrawVertexCount = dwVertPos;
					pMesh->iDrawPrimitives  = dwIndicePos / 3;
					pMesh->bVBRefreshRequired = true;
					pMesh->bMeshHasBeenReplaced = true;
					ShowLimb ( iBuildObj, iLimbIndex );
					if ( pMesh->pDrawBuffer == NULL )
					{
						// add object mesh to buffers
						pbTriggerDrawBufferCreation[iLimbIndex] = true;
					}

					// this step will create a single mesh for the 10x10x10 cube collection
					dwVertPos = 0;
					dwIndicePos = 0;
					for ( int yb = 0; yb < 10; yb++ )
					{
						for ( int xb = 0; xb < 10; xb++ )
						{
							for ( int zb = 0; zb < 10; zb++ )
							{
								// should we create a cube here
								int iAbsX = x+xb;
								int iAbsY = y+yb;
								int iAbsZ = z+zb;
								unsigned char cCubeCode = pCubes[iAbsX][iAbsY][iAbsZ];
								if ( cCubeCode > 0)
								{
									// UV for texture choice
									unsigned char cCubeTexIndex = (cCubeCode & (15<<4)) >> 4;
									int iRow = cCubeTexIndex / 4;
									int iCol = cCubeTexIndex - (iRow*4);
									float fBitU = fU / 20.0f;
									float fBitV = fV / 20.0f;
									float fCoverageU = fBitU * (float)(iAbsX%20);
									float fCoverageUR = fBitU * (float)(19-(iAbsX%20));
									float fCoverageV = fBitV * (float)(19-(iAbsY%20));
									float fCoverageVR = fBitV * (float)(iAbsY%20);
									float fCoverageW = fBitU * (float)(iAbsZ%20);
									float fCoverageWR = fBitU * (float)(19-(iAbsZ%20));
									float fU1 = (fU * iCol)+fCoverageU;
									float fU1R = (fU * iCol)+fCoverageUR;
									float fV1 = (fV * iRow)+fCoverageV;
									float fV1R = (fV * iRow)+fCoverageVR;
									float fW1 = (fU * iCol)+fCoverageW;
									float fZ1 = (fU * iRow)+fCoverageW;
									float fW1R = (fU * iCol)+fCoverageWR;
									float fZ1R = (fU * iRow)+fCoverageWR;
									float fU2 = fU1 + fBitU;
									float fU2R = fU1R + fBitU;
									float fV2 = fV1 + fBitV;
									float fV2R = fV1R + fBitV;
									float fW2 = fW1 + fBitU;
									float fZ2 = fZ1 + fBitU;
									float fW2R = fW1R + fBitU;
									float fZ2R = fZ1R + fBitU;

									// process UV for seamless texturing
									ebe_makeseamless ( iRow, iCol, &fU1, &fU1R, &fV1, &fV1R, &fW1, &fZ1, &fW1R, &fZ1R, &fU2, &fU2R, &fV2, &fW2, &fZ2, &fW2R, &fZ2R );

									// create cube verts
									float fWidth1 = (iAbsX*5.0f)+0.0f;
									float fWidth2 = (iAbsX*5.0f)+5.0f;
									float fHeight1 = (iAbsY*5.0f)+0.0f;
									float fHeight2 = (iAbsY*5.0f)+5.0f;
									float fDepth1 = (iAbsZ*5.0f)+0.0f;
									float fDepth2 = (iAbsZ*5.0f)+5.0f;
									SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, dwVertPos+0, fWidth1, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU1, fV1 );	// front
									SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, dwVertPos+1, fWidth2, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU2, fV1 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+2, fWidth2, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU2, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+3, fWidth1, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU1, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+4, fWidth1, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU2R, fV1 );	// back
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+5, fWidth1, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU2R, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+6, fWidth2, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU1R, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+7, fWidth2, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU1R, fV1 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+8, fWidth1, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, fU1, fZ1R );	// top
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+9, fWidth2, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, fU2, fZ1R );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+10, fWidth2, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, fU2, fZ2R );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+11, fWidth1, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, fU1, fZ2R );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+12, fWidth1, fHeight1, fDepth2,  0.0f, -1.0f,  0.0f, dwColor, fU1, fZ2 );	// bottom
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+13, fWidth1, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, fU1, fZ1 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+14, fWidth2, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, fU2, fZ1 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+15, fWidth2, fHeight1, fDepth2,	 0.0f, -1.0f,  0.0f, dwColor, fU2, fZ2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+16, fWidth2, fHeight2, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, fW1, fV1 );	// right
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+17, fWidth2, fHeight2, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, fW2, fV1 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+18, fWidth2, fHeight1, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, fW2, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+19, fWidth2, fHeight1, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, fW1, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+20, fWidth1, fHeight2, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, fW2R, fV1 );	// left
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+21, fWidth1, fHeight1, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, fW2R, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+22, fWidth1, fHeight1, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, fW1R, fV2 );
									SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+23, fWidth1, fHeight2, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, fW1R, fV1 );

									// and now fill in the index list
									pMesh->pIndices [ dwIndicePos+0 ] = dwVertPos+0;		pMesh->pIndices [ dwIndicePos+1 ] = dwVertPos+1;		pMesh->pIndices [ dwIndicePos+2 ] = dwVertPos+2;
									pMesh->pIndices [ dwIndicePos+3 ] = dwVertPos+2;		pMesh->pIndices [ dwIndicePos+4 ] = dwVertPos+3;		pMesh->pIndices [ dwIndicePos+5 ] = dwVertPos+0;
									pMesh->pIndices [ dwIndicePos+6 ] = dwVertPos+4;		pMesh->pIndices [ dwIndicePos+7 ] = dwVertPos+5;		pMesh->pIndices [ dwIndicePos+8 ] = dwVertPos+6;
									pMesh->pIndices [ dwIndicePos+9 ] = dwVertPos+6;		pMesh->pIndices [ dwIndicePos+10 ] = dwVertPos+7;		pMesh->pIndices [ dwIndicePos+11 ] = dwVertPos+4;
									pMesh->pIndices [ dwIndicePos+12 ] = dwVertPos+8;		pMesh->pIndices [ dwIndicePos+13 ] = dwVertPos+9;		pMesh->pIndices [ dwIndicePos+14 ] = dwVertPos+10;
									pMesh->pIndices [ dwIndicePos+15 ] = dwVertPos+10;		pMesh->pIndices [ dwIndicePos+16 ] = dwVertPos+11;		pMesh->pIndices [ dwIndicePos+17 ] = dwVertPos+8;
									pMesh->pIndices [ dwIndicePos+18 ] = dwVertPos+12;		pMesh->pIndices [ dwIndicePos+19 ] = dwVertPos+13;		pMesh->pIndices [ dwIndicePos+20 ] = dwVertPos+14;
									pMesh->pIndices [ dwIndicePos+21 ] = dwVertPos+14;		pMesh->pIndices [ dwIndicePos+22 ] = dwVertPos+15;		pMesh->pIndices [ dwIndicePos+23 ] = dwVertPos+12;
									pMesh->pIndices [ dwIndicePos+24 ] = dwVertPos+16;		pMesh->pIndices [ dwIndicePos+25 ] = dwVertPos+17;		pMesh->pIndices [ dwIndicePos+26 ] = dwVertPos+18;
									pMesh->pIndices [ dwIndicePos+27 ] = dwVertPos+18;		pMesh->pIndices [ dwIndicePos+28 ] = dwVertPos+19;		pMesh->pIndices [ dwIndicePos+29 ] = dwVertPos+16;
									pMesh->pIndices [ dwIndicePos+30 ] = dwVertPos+20;		pMesh->pIndices [ dwIndicePos+31 ] = dwVertPos+21;		pMesh->pIndices [ dwIndicePos+32 ] = dwVertPos+22;
									pMesh->pIndices [ dwIndicePos+33 ] = dwVertPos+22;		pMesh->pIndices [ dwIndicePos+34 ] = dwVertPos+23;		pMesh->pIndices [ dwIndicePos+35 ] = dwVertPos+20;

									// next cube
									dwVertPos+=24;
									dwIndicePos+=36;
								}
							}
						}
					}
				}
			}
		}

		// ensure we do not trigger slow bounds recalc
		pObject->bUpdateOverallBounds = false;

		// if any triggers flagged, create drawbuffer if not exist
		for ( int iLimbIndex = 1; iLimbIndex < CUBEMAXMESH; iLimbIndex++ )
		{
			if ( pbTriggerDrawBufferCreation[iLimbIndex] == true )
			{
				sMesh* pMesh = pObject->ppFrameList[iLimbIndex]->pMesh;
				if ( pMesh->pDrawBuffer == NULL )
				{
					// add object mesh to buffers (unique VB/IB buffer for this new mesh)
					m_ObjectManager.AddObjectMeshToBuffers ( pMesh, true );
				}
			}
		}

		// object was modified, so recreate in wicked engine
		WickedCall_AddObject(pObject);
		WickedCall_UpdateObject(pObject);
		WickedCall_TextureObject(pObject,NULL);
		WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
		// so we can handle custom material applied to this EBE
		WickedSetElementId(0);
		WickedSetEntityId(-1);
	}
}

void ebe_loadpattern ( LPSTR pEBEFilename )
{
	// default pattern is single cube
	ebebuild.Pattern.iWidth = 1;
	ebebuild.Pattern.iHeight = 1;
	ebebuild.Pattern.iDepth = 1;
	ebebuild.Pattern.iPreserveMode = 0;
	for ( int iY = 0; iY < 20; iY++ )
	{
		for ( int iZ = 0; iZ < 20; iZ++ )
		{
			ebebuild.Pattern.pPRow[iY][iZ] = cStr("0");
		}
	}
	ebebuild.Pattern.pPRow[0][0] = cStr("1");

	// if EBE file exists, replace above pattern
	if ( FileExist(pEBEFilename) == 1 ) 
	{
		strcpy(ActiveEBEFilename, pEBEFilename);
		Dim ( t.data_s, 2000 );
		LoadArray ( pEBEFilename, t.data_s );
		for ( t.l = 0; t.l <= 1999; t.l++ )
		{
			t.line_s=t.data_s[t.l];
			if (  Len(t.line_s.Get())>0 ) 
			{
				if (  t.line_s.Get()[0] != ';' ) 
				{
					//  take fieldname and value
					for ( t.c = 0 ; t.c < Len(t.line_s.Get()); t.c++ )
					{
						if ( t.line_s.Get()[t.c] == '=' ) 
						{ 
							t.mid = t.c+1  ; break;
						}
					}
					t.field_s=cstr(Lower(removeedgespaces(Left(t.line_s.Get(),t.mid-1))));
					t.value_s=cstr(removeedgespaces(Right(t.line_s.Get(),Len(t.line_s.Get())-t.mid)));
					for ( t.c = 0 ; t.c < Len(t.value_s.Get()); t.c++ )
					{
						if (  t.value_s.Get()[t.c] == ',' ) 
						{ 
							t.mid = t.c+1 ; break; 
						}
					}
					t.value1=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value1_f=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value2_s=cstr(removeedgespaces(Right(t.value_s.Get(),Len(t.value_s.Get())-t.mid)));
					if ( Len(t.value2_s.Get())>0  ) t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

					// extract field data from EBE file
					t.tryfield_s="width";
					if ( t.field_s == t.tryfield_s  ) 
					{
						ebebuild.Pattern.iWidth = t.value1;
						if ( ebebuild.Pattern.iWidth > 20 ) ebebuild.Pattern.iWidth = 20;
					}
					t.tryfield_s="height";
					if ( t.field_s == t.tryfield_s  )
					{
						ebebuild.Pattern.iHeight = t.value1;
						if ( ebebuild.Pattern.iHeight > 20 ) ebebuild.Pattern.iHeight = 20;
					}
					t.tryfield_s="depth";
					if ( t.field_s == t.tryfield_s  ) 
					{
						ebebuild.Pattern.iDepth = t.value1;
						if ( ebebuild.Pattern.iDepth > 20 ) ebebuild.Pattern.iDepth = 20;
					}
					t.tryfield_s="preservemode";
					if ( t.field_s == t.tryfield_s  ) 
					{
						ebebuild.Pattern.iPreserveMode = t.value1;
					}

					// pattern strings
					for ( int iY = 0; iY < ebebuild.Pattern.iHeight; iY++ )
					{
						for ( int iZ = 0; iZ < ebebuild.Pattern.iDepth; iZ++ )
						{
							t.tryfield_s = cStr("prow") + cStr(iY) + cStr("x") + cStr(iZ);
							if ( t.field_s == t.tryfield_s  ) ebebuild.Pattern.pPRow[iY][iZ] = Lower(t.value_s.Get());
						}
					}
				}
			}
		}
		UnDim (  t.data_s );
	}
	ebebuild.Pattern.iWidthOffset = 0;
	ebebuild.Pattern.iDepthOffset = 0;

	// copy loaded pattern to original store
	ebebuild.OriginalPattern = ebebuild.Pattern;

	// if go direct to use pattern, ensure current rotation taken into account
	ebe_updatepatternwithrotation();
}

void ebe_updatepatternwithrotation ( void )
{
	// uses ebebuild.iCursorRotation
	if ( ebebuild.iCursorRotation == 0 )
	{
		ebebuild.Pattern = ebebuild.OriginalPattern;
	}
	else
	{
		// wipe pattern to recreate
		ebebuild.Pattern.iWidth = 0;
		ebebuild.Pattern.iHeight = 0;
		ebebuild.Pattern.iDepth = 0;
		ebebuild.Pattern.iWidthOffset = 0;
		ebebuild.Pattern.iDepthOffset = 0;
		for ( int iY = 0; iY < 20; iY++ )
		{
			for ( int iZ = 0; iZ < 20; iZ++ )
			{
				ebebuild.Pattern.pPRow[iY][iZ] = cStr("0");
			}
		}
		if ( ebebuild.iCursorRotation == 1 )
		{
			// 90 degree CW for XZ
			ebebuild.Pattern.iWidth = ebebuild.OriginalPattern.iDepth;
			ebebuild.Pattern.iHeight = ebebuild.OriginalPattern.iHeight;
			ebebuild.Pattern.iDepth = ebebuild.OriginalPattern.iWidth;
			for ( int iY = 0; iY < ebebuild.Pattern.iHeight; iY++ )
			{
				int iX = 0;
				for ( int iZ = ebebuild.Pattern.iDepth-1; iZ >= 0 ; iZ-- )
				{
					cStr pFillX = cStr("");
					for ( int iOZ = 0; iOZ < ebebuild.OriginalPattern.iDepth; iOZ++ )
					{
						LPSTR pNeedFullZeros = ebebuild.OriginalPattern.pPRow[iY][iOZ].Get();
						if ( strcmp ( pNeedFullZeros, "0" ) == NULL ) pNeedFullZeros = "00000000000000000000";
						pFillX = pFillX + Mid(pNeedFullZeros,1+iX);
					}
					ebebuild.Pattern.pPRow[iY][iZ] = pFillX;
					iX++;
				}
			}
		}
		if ( ebebuild.iCursorRotation == 2 )
		{
			// 180 degree CW for XZ
			ebebuild.Pattern.iWidth = ebebuild.OriginalPattern.iWidth;
			ebebuild.Pattern.iHeight = ebebuild.OriginalPattern.iHeight;
			ebebuild.Pattern.iDepth = ebebuild.OriginalPattern.iDepth;
			ebebuild.Pattern.iDepthOffset = ebebuild.OriginalPattern.iWidth-ebebuild.OriginalPattern.iDepth;
			for ( int iY = 0; iY < ebebuild.Pattern.iHeight; iY++ )
			{
				int iOZ = 0;
				for ( int iZ = ebebuild.OriginalPattern.iDepth-1; iZ >= 0; iZ-- )
				{
					cStr pFillX = cStr("");
					for ( int iOX = ebebuild.OriginalPattern.iWidth-1; iOX >= 0; iOX-- )
					{
						pFillX = pFillX + Mid(ebebuild.OriginalPattern.pPRow[iY][iOZ].Get(),1+iOX);
					}
					ebebuild.Pattern.pPRow[iY][iZ] = pFillX;
					iOZ++;
				}
			}
		}
		if ( ebebuild.iCursorRotation == 3 )
		{
			// 270 degree CW for XZ
			ebebuild.Pattern.iWidth = ebebuild.OriginalPattern.iDepth;
			ebebuild.Pattern.iHeight = ebebuild.OriginalPattern.iHeight;
			ebebuild.Pattern.iDepth = ebebuild.OriginalPattern.iWidth;
			ebebuild.Pattern.iWidthOffset = ebebuild.OriginalPattern.iWidth-ebebuild.OriginalPattern.iDepth;
			for ( int iY = 0; iY < ebebuild.Pattern.iHeight; iY++ )
			{
				int iOX = 0;
				for ( int iZ = 0; iZ < ebebuild.Pattern.iDepth; iZ++ )
				{
					cStr pFillX = cStr("");
					for ( int iOZ = ebebuild.OriginalPattern.iDepth-1; iOZ >=0; iOZ-- )
					{
						pFillX = pFillX + Mid(ebebuild.OriginalPattern.pPRow[iY][iOZ].Get(),1+iOX);
					}
					ebebuild.Pattern.pPRow[iY][iZ] = pFillX;
					iOX++;
				}
			}
		}
	}
}

void ebe_settexturehighlight ( void )
{
	int iRow = ebebuild.iCurrentTexture / 4;
	int iCol = ebebuild.iCurrentTexture - (iRow*4);
	float fChoiceWidth = ebebuild.iTexturePanelWidth/4;
	float fChoiceHeight = ebebuild.iTexturePanelHeight/4;
	if (!bDisableAllSprites) MAXSprite ( ebebuild.iTexturePanelHighSprite, ebebuild.iTexturePanelX+(iCol*fChoiceWidth), ebebuild.iTexturePanelY+(iRow*fChoiceHeight), ebebuild.iTexturePanelHighImg );
}

void imgui_ebe_loop(void)
{
	// if no tools object, cannot proceed
	if (ebebuild.iToolObj == 0) return;
	if (ObjectExist(ebebuild.iToolObj) == 0) return;
	if (GetObjectData(ebebuild.iToolObj)->bVisible == false) return;

	switch (delay_execute) 
	{
		case 1: //Change texture in plate.
		{
			delay_execute = 0;
			ebebuild.bCustomiseTexture = false;
			int iEntityProfileIndex = t.entityelement[t.ebe.entityelementindex].bankindex;
			if (ebe_loadcustomtexture(iEntityProfileIndex, ebebuild.iCurrentTexture) == 1)
			{
				// successfully pasted new texture into plate
			}
			//Reset click state.
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.MouseDown[1] = 0;
			io.MouseDownDuration[1] = 0;
			io.MouseDown[0] = 0;
			io.MouseDownDuration[0] = 0;
			skib_frames_execute = 5;

			break;
		}
		default:
			break;
	}
	if (skib_frames_execute > 0)
		skib_frames_execute--;

	ImVec4 drawCol_Down = ImColor(180, 180, 160, 255);
	ImVec4 drawCol_back = ImColor(255, 255, 255, 128);
	ImVec4 drawCol_normal = ImColor(255, 255, 255, 255);
	ImVec4 drawCol_hover = ImColor(180, 180, 180, 230);

	fPropertiesColoumWidth = 80;
	if (bBuilder_Properties_Window) 
	{
		float media_icon_size = 40.0f;
		float plate_width = (media_icon_size + 6.0) * 4.0f;
		ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
		if (pref.current_style == 3)
			tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_Button];
		grideleprof_uniqui_id = 15000;
		extern int iGenralWindowsFlags;
		ImGui::Begin("Structure Properties##BuilderPropertiesWindow", &bBuilder_Properties_Window, iGenralWindowsFlags);

		float w = ImGui::GetWindowContentRegionWidth();

		if (ImGui::StyleCollapsingHeader("Name", ImGuiTreeNodeFlags_DefaultOpen)) {

			//Display icon.
			int thumb_size = 48;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (thumb_size*0.5), 0.0f));
			ImGui::ImgBtn(EBE_THUMB, ImVec2(thumb_size, thumb_size), drawCol_back, drawCol_normal, drawCol_normal, drawCol_normal, -1, 0, 0, 0, true);
			
			ImGui::Indent(10);
			
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
			ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(-10);
			ImGui::InputText("##structure_nameInput", &structure_name[0], MAX_PATH, ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::PopItemWidth();
			if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Enter the name of this structure");
			if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
			ImGui::Indent(-10);

		}

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		if (ImGui::StyleCollapsingHeader("Parts", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild("##BuilderChildPanel", ImVec2(0, ImGui::GetFontSize()*3.0 + 12.0));

			extern char ActiveEBEFilename[260];
			int preview_count = 0;
			float contentarea = ImGui::GetWindowSize().x * ImGui::GetWindowSize().y;
			int media_icon_size_tools = 64;
			int iColumnsWidth = 110;
			bool bNoText = false;
			if (contentarea > 90000) {
				media_icon_size_tools = 64;
				iColumnsWidth = 110;
			}
			else if (contentarea > 80000) {
				media_icon_size_tools = 48;
				iColumnsWidth = 110 - 16;
			}
			else if (contentarea > 40000) {
				media_icon_size_tools = 32;
				iColumnsWidth = 110 - 16 - 16;
			}
			else if (contentarea > 22000) {
				media_icon_size_tools = 20;
				iColumnsWidth = 110 - 16 - 16;
				bNoText = true;
			}
			else {
				media_icon_size_tools = 20;
				iColumnsWidth = 110 - 16 - 16 - 16 - 8 ;
				bNoText = true;
			}

			bool bDisplayText = true;
			ImGui::SetWindowFontScale(SMALLFONTSIZE);
			float fWinWidth = ImGui::GetWindowSize().x - 10.0; // Flicker - ImGui::GetCurrentWindow()->ScrollbarSizes.x;
			if (iColumnsWidth >= fWinWidth && fWinWidth > media_icon_size_tools) {
				iColumnsWidth = fWinWidth;
				ImGui::SetWindowFontScale(SMALLESTFONTSIZE);
			}
			if (fWinWidth <= media_icon_size_tools + 10) {
				iColumnsWidth = media_icon_size_tools;
				ImGui::SetWindowFontScale(SMALLESTFONTSIZE);
			}
			if (fWinWidth <= 42) {
				iColumnsWidth = media_icon_size_tools + 16;
				bDisplayText = false;
			}

			int iColumns = (int)(ImGui::GetWindowSize().x / (iColumnsWidth));
			if (iColumns <= 1)
				iColumns = 1;

			ImGui::Columns(iColumns, "mycolumns4entities", false);  //false no border

			float fFramePadding = (iColumnsWidth - media_icon_size_tools)*0.5;
			float fCenterX = iColumnsWidth * 0.5;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fFramePadding, 2.0f));

			for (int il = 0; il < t.ebebankmax; il++) 
			{
				int icon = TOOL_BUILDER;
				char tmp[MAX_PATH];
				strcpy(tmp, t.ebebank_s[il].Get());
				int pos = strlen(tmp);
				while (pos > 0 && tmp[pos] != '\\') pos--;
				if (pos > 0) 
				{
					cstr Text;
					strcpy(&tmp[0], &tmp[pos + 1]);
					if (pestrcasestr(tmp, "new site.")) { icon = EBE_NEW; Text = "Add New Site"; }
					else if (pestrcasestr(tmp, "cube.")) { icon = EBE_CUBE; Text = "Cube"; }
					else if (pestrcasestr(tmp, "floor.")) { icon = EBE_FLOOR; Text = "Floor"; }
					else if (pestrcasestr(tmp, "wall.")) { icon = EBE_WALL; Text = "Wall"; }
					else if (pestrcasestr(tmp, "column.")) { icon = EBE_COLUMN; Text = "Column"; }
					else if (pestrcasestr(tmp, "row.")) { icon = EBE_ROW; Text = "Row"; }
					else if (pestrcasestr(tmp, "stairs.")) { icon = EBE_STAIRS; Text = "Stairs"; }
					else if (pestrcasestr(tmp, "block.")) { icon = EBE_BLOCK; Text = "Block"; }
					else 
					{
						icon = TOOL_BUILDER;
						int pos2 = 0;
						while (tmp[pos2] != '.' && pos2 < strlen(tmp)) pos2++;
						if (pos2 > 0)
							tmp[pos2] = 0;
						Text = tmp;
					}

					if (strlen(ActiveEBEFilename) > 0 && pestrcasestr(t.ebebank_s[il].Get(), ActiveEBEFilename)) {
						ImVec2 padding = { 2.0, 2.0 };
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						const ImRect image_bb((window->DC.CursorPos - padding) + ImVec2(fFramePadding, 2.0f), window->DC.CursorPos + padding + ImVec2(fFramePadding, 2.0f) + ImVec2(media_icon_size_tools, media_icon_size_tools));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					if (ImGui::ImgBtn(icon, ImVec2(media_icon_size_tools, media_icon_size_tools), drawCol_back, drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, true))
					{
						extern bool bImporter_Window;
						extern bool bWaypoint_Window;
						extern bool bWaypointDrawmode;
						extern bool g_bCharacterCreatorPlusActivated;
						void CheckTooltipObjectDelete(void);
						void CloseDownEditorProperties(void);

						if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
						if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
						if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;

						CheckTooltipObjectDelete();
						CloseDownEditorProperties();

						// leelee - seems we are creating a new entity, then not using it when a non newsite button is selected (with the old code)?
						if (icon != EBE_NEW) 
						{
							// select a new pattern
							LPSTR pPBFEBEFile = t.ebebank_s[il].Get();
							ebe_loadpattern(pPBFEBEFile);
							t.inputsys.constructselection = 0;
						}
						else
						{
							// trigger creation of new site
							t.addentityfile_s = t.ebebank_s[il].Get();
							if (t.addentityfile_s != "")
							{
								entity_adduniqueentity(false);
								t.tasset = t.entid;
								if (t.talreadyloaded == 0)
								{
									editor_filllibrary();
								}
							}
							t.inputsys.constructselection = t.tasset;
							t.gridentity = t.entid;
							t.inputsys.constructselection = t.entid;
							t.inputsys.domodeentity = 1;
							t.grideditselect = 5;
							editor_refresheditmarkers();

							// NewSite, make sure we are in entity mode.
							bForceKey = true;
							csForceKey = "e";
						}
						/*
						t.addentityfile_s = t.ebebank_s[il].Get();
						if (t.addentityfile_s != "")
						{
							entity_adduniqueentity(false);
							t.tasset = t.entid;
							if (t.talreadyloaded == 0)
							{
								editor_filllibrary();
							}
						}
						t.inputsys.constructselection = t.tasset;

						t.gridentity = t.entid;
						t.inputsys.constructselection = t.entid;
						t.inputsys.domodeentity = 1;
						t.grideditselect = 5;
						editor_refresheditmarkers();

						if (icon != EBE_NEW) {
							LPSTR pPBFEBEFile = t.ebebank_s[il].Get();
							ebe_loadpattern(pPBFEBEFile);
							t.inputsys.constructselection = 0;
						}
						else {
							//NewSite, make sure we are in entity mode.
							bForceKey = true;
							csForceKey = "e";
						}
						*/
					}
					if (ImGui::IsItemHovered() && Text != "") ImGui::SetTooltip("%s", Text.Get());

					if (!bNoText) {
						int iTextWidth = ImGui::CalcTextSize(Text.Get()).x;
						if (iTextWidth < iColumnsWidth)
							ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (fCenterX - (iTextWidth*0.5)), ImGui::GetCursorPosY()));
						ImGui::TextWrapped(Text.Get());
					}
					ImGui::NextColumn();
				}
			}

			ImGui::PopStyleVar();
			ImGui::SetWindowFontScale(1.00);
			ImGui::Columns(1);
			ImGui::EndChild();
		}

		if (ImGui::StyleCollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int control_image_size = 42;
			float control_width = (control_image_size + 6.0) * 3.0f;

			int indent = (w*0.5) - (control_width*0.5);
			if (indent < 10)
				indent = 10;
			ImGui::Indent(indent);

			if (iPaintMode == 1)
			{
				ImVec2 padding = { 4.0, 4.0 };
				const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
				window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
			}
			if (ImGui::ImgBtn(EBE_CONTROL1, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false)) {
				//Paint mode.
				iPaintMode = 1;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paint Mode");

			ImGui::SameLine();

			if (iPaintMode != 1)
			{
				ImVec2 padding = { 4.0, 4.0 };
				const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
				window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
			}
			if (ImGui::ImgBtn(EBE_CONTROL2, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false)) {
				//Remove mode.
				iPaintMode = 0;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Mode");
			ImGui::SameLine();

			if (ImGui::ImgBtn(EBE_CONTROL3, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false)) {
				//rotate
				bForceKey = true;
				csForceKey = "r";
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate ('R')");

			
			if (ImGui::ImgBtn(EBE_CONTROL4, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false)) {
				//page up
				iForceScancode = 201;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Page Up");
			ImGui::SameLine();
			if (ImGui::ImgBtn(EBE_CONTROL5, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false)) {
				//page down
				iForceScancode = 209;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Page Down");
			ImGui::SameLine();
			if (ImGui::ImgBtn(EBE_CONTROL6, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false)) {
				//exit
				bForceKey = true;
				csForceKey = "o";
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Exit Structure Editor ('O')");
			
			ImGui::Indent(-indent);

		}
		if (ImGui::StyleCollapsingHeader("Texture Set", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (plate_width*0.5), 0.0f));
			int indent = (w*0.5) - (plate_width*0.5);
			if (indent < 10)
				indent = 10;
			ImGui::Indent(indent);
			
			ImGui::PushItemWidth(plate_width);

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));
			const char* items[] = { "Default" };
			if (ImGui::Combo("##TextureSetSelection", &texture_set_selection, items, IM_ARRAYSIZE(items))) 
			{
				//texture_set_selection
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Texture Palette");

			ImGui::PopItemWidth();
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));
			int n = 0;
			int iTextureIndexBeingHoveredOver = -1;
			for (int y = 0; y < 4; y++)
			{
				for (int x = 0; x < 4; x++)
				{
					if (ebebuild.iTexPlateImage > 0) 
					{
						ImVec2 padding = { 2.0, 2.0 };
						ImVec4 bg_col = { 0.0, 0.0, 0.0, 1.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(media_icon_size, media_icon_size));
						window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, 15);

						if (ebebuild.iCurrentTexture == n) 
						{
							ImVec2 padding = { 4.0, 4.0 };
							const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(media_icon_size, media_icon_size));
							window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
						}

						ImGui::PushID(ebebuild.iTexPlateImage + n + 200000);
						if (ImGui::ImgBtn(ebebuild.iTexPlateImage, ImVec2(media_icon_size, media_icon_size), ImVec4(0.0, 0.0, 0.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, n + 1, 4, 4, false)) 
						{
							ebebuild.iCurrentTexture = n;
						}
						if (skib_frames_execute == 0 && ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) 
						{
							ebebuild.iCurrentTexture = n;
							delay_execute = 1; //@Lee remove this line if you dont want to support changing textures.
						}
						if ( ImGui::IsItemHovered() )
						{
							iTextureIndexBeingHoveredOver = n;
						}

						ImGui::PopID();
						if (x != 3)
							ImGui::SameLine();

						if (ImGui::IsItemHovered()) 
						{
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
							ImGui::ImgBtn(ebebuild.iTexPlateImage, ImVec2(icon_size, icon_size), ImVec4(0.0, 0.0, 0.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, n + 1, 4, 4, false);
							int iTexSlot = iTextureIndexBeingHoveredOver;
							int iMaterialIndex = ebebuild.TXP.iMaterialRef[iTexSlot];
							LPSTR pMaterialType = "Other";
							switch (iMaterialIndex)
							{
								case 0: pMaterialType = "Generic"; break;
								case 1: pMaterialType = "Stone"; break;
								case 2: pMaterialType = "Metal"; break;
								case 3: pMaterialType = "Wood"; break;
							}
							char pShowMaterialText[256];
							sprintf(pShowMaterialText, "Material %d: %s", 1+iMaterialIndex, pMaterialType);
							ImGui::TextCenter(pShowMaterialText);
							ImGui::EndTooltip();
						}
					}
					n++;
				}
			}
			ImGui::Indent(-indent);

			// detect if user wants to change material
			if ( iTextureIndexBeingHoveredOver !=-1 )
			{
				ImGuiIO& io = ImGui::GetIO(); (void)io;
				if ( io.KeysDown[49] > 0 ) ebebuild.TXP.iMaterialRef[iTextureIndexBeingHoveredOver] = 0;
				if ( io.KeysDown[50] > 0 ) ebebuild.TXP.iMaterialRef[iTextureIndexBeingHoveredOver] = 1;
				if ( io.KeysDown[51] > 0 ) ebebuild.TXP.iMaterialRef[iTextureIndexBeingHoveredOver] = 2;
				if ( io.KeysDown[52] > 0 ) ebebuild.TXP.iMaterialRef[iTextureIndexBeingHoveredOver] = 3;
			}

			// keyboard help to change material
			ImGui::TextCenter("Hover over texture to view");
			ImGui::TextCenter("Press keys 1-4 to choose material");
			ImGui::TextCenter("1=Generic  2=Stone  3=Metal  4=Wood");
			ImGui::Text("");
		}

		int iBuilderId = t.ebe.entityelementindex;
		sObject* pObject = g_ObjectList[ebebuild.iBuildObj];
		if (iBuilderId > 0 && pObject)
		{
			if (ImGui::StyleCollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);
				//if (!t.grideleprof.bCustomWickedMaterialActive) 
				//{
				//	//LB: Explain what this tick box now does with more clarity!
				//	ImGui::Checkbox("Custom Materials Used", &t.grideleprof.bCustomWickedMaterialActive);
				//	if (ImGui::IsItemHovered()) ImGui::SetTooltip("This flag indicates the object has modified the original model, either through FPE level additions or changes within the level editor");
				//
				//	//PE: Copy master material settings to t.grideleprof.WEMaterial
				//	if (t.grideleprof.bCustomWickedMaterialActive) 
				//	{
				//		// copies structure object details being edited to WEmaterial
				//		Wicked_Copy_Material_To_Grideleprof((void*)pObject, 3);
				//		t.grideleprof.WEMaterial.MaterialActive = true;
				//	}
				//	else 
				//	{
				//		t.grideleprof.WEMaterial.MaterialActive = false;
				//	}
				//}
				//else 
				{
					//ImGui::Checkbox("Custom Materials Used", &t.grideleprof.bCustomWickedMaterialActive);
					//if (ImGui::IsItemHovered()) ImGui::SetTooltip("This flag indicates the object has modified the original model, either through FPE level additions or changes within the level editor");
					Wicked_Change_Object_Material((void*)pObject, 3);
				}
				ImGui::Indent(-10);
			}
		}

		if (ImGui::StyleCollapsingHeader("Save Structure", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float col_start = 80.0f;
			float save_gadget_size = ImGui::GetFontSize()*12.0;

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
			ImGui::Text("Path");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));

			ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));

			float path_gadget_size = ImGui::GetFontSize()*2.0;

			ImGui::PushItemWidth(-10 - path_gadget_size);

			ImGui::InputText("##InputPathBuilder", &BuilderPath[0], 250);
			if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
			ImGui::PopItemWidth();

			ImGui::SameLine();
			ImGui::PushItemWidth(path_gadget_size);
			if (ImGui::StyleButton("...##Builderpath"))
			{
				//PE: filedialogs change dir so.
				cStr tOldDir = GetDir();
				char * cFileSelected;
				cstr fulldir = tOldDir + "\\entitybank\\user\\ebestructures";
				char pRealDestFolder[MAX_PATH];
				strcpy(pRealDestFolder, fulldir.Get());
				GG_GetRealPath(pRealDestFolder,1);
				strcat(pRealDestFolder, "\\"); // does not seem to place me in the current ebestructures folder?
				fulldir = pRealDestFolder;
				cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_DIR, "All\0*.*\0", fulldir.Get(), NULL);
				SetDir(tOldDir.Get());
				if (cFileSelected && strlen(cFileSelected) > 0) 
				{
					strcpy(BuilderPath, cFileSelected);
				}
			}
			ImGui::PopItemWidth();

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (save_gadget_size*0.5), 0.0f));

			if (ImGui::StyleButton("Save Structure As Object", ImVec2(save_gadget_size, 0)))
			{
				//Save structure
				if (strlen(structure_name) > 0)
				{
					if (strlen(BuilderPath) > 0)
					{
						//Save
						cStr tSaveFile = BuilderPath;
						if(BuilderPath[strlen(BuilderPath)-1] != '\\' || BuilderPath[strlen(BuilderPath) - 1] != '/')
							tSaveFile += "\\";
						tSaveFile += structure_name;
						if(!pestrcasestr(tSaveFile.Get(),".ebe"))
							tSaveFile += ".ebe";

						//Resolve path.
						char resolved[MAX_PATH];
						int retval = GetFullPathNameA(tSaveFile.Get(), MAX_PATH, resolved, NULL);
						if (retval > 0) {
							tSaveFile = resolved;
						}

						Wicked_Copy_Material_To_Grideleprof((void*)pObject, 3);

						int iBuilderId = t.ebe.entityelementindex;
						ebe_hide();
						//ebe_finishsite();


						strcpy(cTriggerMessage, "Structure Saved");
						bTriggerMessage = true;

						bool bSaveIt = true;
						// Check if already exists, if so, ask if should be overwritten
						if (FileExist(tSaveFile.Get()) == 1)
						{
							// Already Exists
							char pDisplayErrorMsg[512];
							strcpy(pDisplayErrorMsg, structure_name);
							strcat(pDisplayErrorMsg, " already exists! Overwrite?");
							HWND hThisWnd = GetForegroundWindow();
							if (MessageBoxA(hThisWnd, pDisplayErrorMsg, "File Already Exists", MB_YESNO | MB_TOPMOST) != IDYES)
							{
								strcpy(cTriggerMessage, "Structure was NOT Saved.");
								bSaveIt = false;
							}
						}

						if (bSaveIt) {
							ebe_save_ebefile(tSaveFile, iBuilderId);
						}

						ebe_newsite(iBuilderId);

						pObject = g_ObjectList[ebebuild.iBuildObj];
						Wicked_Set_Material_From_grideleprof((void*)pObject, 3);
						//PE: Finally copy material to all meshed in new structure.
						Wicked_Update_All_Materials((void*)pObject, 3);
					}
					else
					{
						strcpy(cTriggerMessage, "Please select a path where you like the structure saved.");
						bTriggerMessage = true;
					}
				}
				else
				{
					strcpy(cTriggerMessage, "You must give your structure a name before you can save it.");
					bTriggerMessage = true;
				}
			}
		}

		/*
		if (!pref.bHideTutorials)
		{
			if (ImGui::StyleCollapsingHeader("Tutorial (this feature is incomplete)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);
				void SmallTutorialVideo(char *tutorial, char* combo_items[] = NULL, int combo_entries = 0, int iVideoSection = 0, bool bAutoStart = false);
				cstr cShowTutorial = "03 - Add character and set a path";
				char* tutorial_combo_items[] = { "01 - Getting started", "02 - Creating terrain", "03 - Add character and set a path" };
				SmallTutorialVideo(cShowTutorial.Get(), tutorial_combo_items, ARRAYSIZE(tutorial_combo_items), SECTION_STRUCTURE_EDITOR);
				float but_gadget_size = ImGui::GetFontSize()*12.0;
				float w = ImGui::GetWindowContentRegionWidth() - 10.0;
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
				#ifdef INCLUDESTEPBYSTEP
				if (ImGui::StyleButton("View Step by Step Tutorial", ImVec2(but_gadget_size, 0)))
				{
					// pre-select tutorial 03
					extern bool bHelpVideo_Window;
					extern bool bHelp_Window;
					extern char cForceTutorialName[1024];
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
		}
		*/

		void CheckMinimumDockSpaceSize(float minsize);
		CheckMinimumDockSpaceSize(250.0f);

		if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
			//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
			ImGui::Text("");
			ImGui::Text("");
		}

		ImGui::End();
	}
}

