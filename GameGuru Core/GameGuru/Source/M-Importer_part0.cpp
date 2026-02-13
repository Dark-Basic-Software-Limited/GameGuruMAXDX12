//----------------------------------------------------
//--- GAMEGURU - M-Importer
//----------------------------------------------------

// Includes
#include "stdafx.h"
#include "gameguru.h"

//#include "M-CharacterCreatorPlus.h"

//PE: GameGuru IMGUI.
#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

#define IMPORTER_TMP_IMAGE (g.importermenuimageoffset+48)

// Enums
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

// Globals
bool g_bLoadedFBXModel = false;
int g_iFirstTimeFBXImport = 0;
int g_iPreferPBR = 0;
int g_iPreferPBRLateShaderChange = 0;
bool g_bCameraInSkyForImporter = false;
char g_pShowFilenameHoveringOver[1024];
DWORD g_dwMapImageListIndexToLimbCount = 0;
int* g_piMapImageListIndexToLimbIndex = NULL;

extern bool bImporter_Window;
extern bool bCenterRenderView;
int iDelayedExecute = 0;
int iDelayedExecuteSelection = 0;
int iDelayedExecuteChannel = -1;
int iImporterScale = 100;
float fImporterScaleMultiply = 1.0;
int iImporterGenerateThumb = 0;
extern float custom_back_color[4];
char cImportName[128];
char cImportPath[MAX_PATH] = "entitybank\\user\\";
char cImportPathCropped[MAX_PATH] = "\\user";
extern bool bTriggerMessage;
extern char cTriggerMessage[MAX_PATH];
float fImportRotX = 0.0, fImportRotY = 0.0, fImportRotZ = 0.0;
float fImportPosX = 0.0, fImportPosY = 0.0, fImportPosZ = 0.0;
float fImportModelBaseX = 0.0, fImportModelBaseY = 0.0, fImportModelBaseZ = 0.0;
bool bFindFloor = false;
bool bUseRGBAButtons = false;
bool bBatchConverting = false;
std::vector<cstr> batchFileList;

extern bool bBoostIconColors;

bool bRemoveSprites = true;

extern bool bImGuiGotFocus;
wiScene::MaterialComponent* pSelectedMaterial = NULL;
sMesh * pSelectedMesh = NULL;
char scaling_combo_entry[256] = "Original Scaling\0"; //PE: Changed "Automatic Scaling\0";
char collision_combo_entry[256] = "Box\0";
char mesh_combo_entry[256] = "1:Noname\0";
char material_combo_entry[256] = "Silent\0";
int iSelectedMesh = 0;
char cPreSelectedFile[MAX_PATH] = "\0";
float BaseColor[4];
float EmmisiveColor[4];
float fReflectance = 0.04;
//float fReflectance = 0.002;
bool bTransparent = false;
bool bDoubleSided = false;
float fRenderOrderBias = 0.0f;
bool bPlanerReflection = false;
bool bCastShadows = true;
DWORD dwBaseColor;
DWORD dwEmmisiveColor;
bool bStickObjectToGround = false;
bool bUpdateGrideleprof = false;
bool bHaveMaterialUpdate = false;
bool bChooseSurfaceChannel = false;
//cstr oldtextimportpath = "";
std::vector<cstr> g_MeshNamesAssimp;

// animation slot global
std::vector<sAnimSlotStruct> g_pAnimSlotList;

// animation control
bool g_bShowBones = false;
bool g_bShowBonesExtraInfo = false;
bool bFoundanimSet = false;
bool g_bAnimatingObjectPreview = false;
bool g_bUpdateAnimationPreview = false;
int g_iCurrentAnimationSlotIndex = 0;

int g_iLootListCount = 0;
cstr g_lootList_s[11];
int g_lootListPercentage[11];

extern cstr cInfoMessage;
extern cstr cInfoImage;
extern bool bInfo_Window;
extern int iInfoUniqueId;

sImportedObjectData g_Data;

extern preferences pref;

void importer_init_wicked(void)
{
	// deactivate any modes editor may have been in
	waypoint_hideall ( );

	// need to see all editor entity objects wiped from screen
	Sync(); Sync();

	// set essential importer settings
	t.importer.startDir = GetDir();
	t.importer.importerActive = 1;

	// importer UI panel
	bCenterRenderView = true;
	bImporter_Window = true;

	if (!t.importer.bQuitForReload)
	{
		strcpy(cImportName, "");
		t.importer.bInvertNormalMap = false;
	}

	// Load the textures used for the objects.
	image_setlegacyimageloading(false);
	LoadImage("texturebank\\terrain grey grid_color (uncompressed).dds", g.importerextraimageoffset);
	LoadImage("texturebank\\terrain grey grid_color (uncompressed).dds", g.importerextraimageoffset + 1);
	LoadImage("editors\\uiv3\\CharacterRefDecal.png", g.importerextraimageoffset + 2);

	// Store the camera's position so that it can be restored when exiting the importer.
	t.importer.fPrevCamX = CameraPositionX();
	t.importer.fPrevCamY = CameraPositionY();
	t.importer.fPrevCamZ = CameraPositionZ();
	t.importer.fPrevCamAngX = CameraAngleX();
	t.importer.fPrevCamAngY = CameraAngleY();

	// Initial state for the mesh names.
	t.importer.bModelMeshNamesSet = false;
	t.importer.cModelMeshNames.clear();
}

void importer_init ( void )
{
	// Wicked Importer does it differently 
	importer_init_wicked();
}

// Change the visual setting in currentVisuals to the settings from desiredVisuals, then store them in storage so they can be restored later.
void set_temp_visuals(visualstype& currentVisuals, visualsdatastoragetype& storage, const visualsdatastoragetype& desiredVisuals)
{
	// First, store all of the current visuals
	storage.AmbienceRed_f =currentVisuals.AmbienceRed_f;
	storage.AmbienceGreen_f = currentVisuals.AmbienceGreen_f;
	storage.AmbienceBlue_f = currentVisuals.AmbienceBlue_f;
	storage.FogR_f = currentVisuals.FogR_f;
	storage.FogG_f = currentVisuals.FogG_f;
	storage.FogB_f = currentVisuals.FogB_f;
	storage.FogA_f = currentVisuals.FogA_f;
	storage.SunIntensity_f = currentVisuals.SunIntensity_f;
	storage.SunRed_f = currentVisuals.SunRed_f;
	storage.SunGreen_f = currentVisuals.SunGreen_f;
	storage.SunBlue_f = currentVisuals.SunBlue_f;
	storage.ZenithRed_f = currentVisuals.ZenithRed_f;
	storage.ZenithGreen_f = currentVisuals.ZenithGreen_f;
	storage.ZenithBlue_f = currentVisuals.ZenithBlue_f;
	storage.bBloomEnabled = currentVisuals.bBloomEnabled;
	storage.bLevelVSyncEnabled = currentVisuals.bLevelVSyncEnabled;
	storage.bSSREnabled = currentVisuals.bSSREnabled;
	storage.bReflectionsEnabled = currentVisuals.bReflectionsEnabled;
	storage.bLightShafts = currentVisuals.bLightShafts;
	storage.bLensFlare = currentVisuals.bLensFlare;
	storage.bAutoExposure = currentVisuals.bAutoExposure;
	storage.fGamma = currentVisuals.fGamma;
	storage.fDeSaturate = currentVisuals.fDeSaturate;
	
	storage.SunAngleX = currentVisuals.SunAngleX;
	storage.SunAngleY = currentVisuals.SunAngleY;
	storage.SunAngleZ = currentVisuals.SunAngleZ;
	storage.iTimeOfday = currentVisuals.iTimeOfday;
	storage.fExposure = currentVisuals.fExposure;
	storage.skyindex = currentVisuals.skyindex;
	
	// Set the visuals to the desired settings.
	currentVisuals.AmbienceRed_f = desiredVisuals.AmbienceRed_f;
	currentVisuals.AmbienceGreen_f = desiredVisuals.AmbienceGreen_f;
	currentVisuals.AmbienceBlue_f = desiredVisuals.AmbienceBlue_f;
	currentVisuals.FogR_f = desiredVisuals.FogR_f;
	currentVisuals.FogG_f = desiredVisuals.FogG_f;
	currentVisuals.FogB_f = desiredVisuals.FogB_f;
	currentVisuals.FogA_f = desiredVisuals.FogA_f;
	currentVisuals.SunIntensity_f = desiredVisuals.SunIntensity_f;
	currentVisuals.SunRed_f = desiredVisuals.SunRed_f;
	currentVisuals.SunGreen_f = desiredVisuals.SunGreen_f;
	currentVisuals.SunBlue_f = desiredVisuals.SunBlue_f;
	currentVisuals.ZenithRed_f = desiredVisuals.ZenithRed_f;
	currentVisuals.ZenithGreen_f = desiredVisuals.ZenithGreen_f;
	currentVisuals.ZenithBlue_f = desiredVisuals.ZenithBlue_f;
	currentVisuals.bBloomEnabled = desiredVisuals.bBloomEnabled;
	currentVisuals.bLevelVSyncEnabled = desiredVisuals.bLevelVSyncEnabled;
	currentVisuals.bSSREnabled = desiredVisuals.bSSREnabled;
	currentVisuals.bReflectionsEnabled = desiredVisuals.bReflectionsEnabled;
	currentVisuals.bLightShafts = desiredVisuals.bLightShafts;
	currentVisuals.bLensFlare = desiredVisuals.bLensFlare;
	currentVisuals.bAutoExposure = desiredVisuals.bAutoExposure;
	currentVisuals.fGamma = desiredVisuals.fGamma;
	currentVisuals.fDeSaturate = desiredVisuals.fDeSaturate;
	
	currentVisuals.SunAngleX = desiredVisuals.SunAngleX;
	currentVisuals.SunAngleY = desiredVisuals.SunAngleY;
	currentVisuals.SunAngleZ = desiredVisuals.SunAngleZ;
	currentVisuals.iTimeOfday = desiredVisuals.iTimeOfday;
	currentVisuals.fExposure = desiredVisuals.fExposure;
	currentVisuals.refreshskysettings = 0; //1 ZJ: prevent sky_skyspec_init() resetting sun rotation.
	currentVisuals.refreshshaders = 1;
}

void restore_visuals(visualstype& currentVisuals, visualsdatastoragetype& storage)
{
	// Restore visual settings to what they were on init.
	currentVisuals.AmbienceRed_f = storage.AmbienceRed_f;
	currentVisuals.AmbienceGreen_f = storage.AmbienceGreen_f;
	currentVisuals.AmbienceBlue_f = storage.AmbienceBlue_f;
	currentVisuals.FogR_f = storage.FogR_f;
	currentVisuals.FogG_f = storage.FogG_f;
	currentVisuals.FogB_f = storage.FogB_f;
	currentVisuals.FogA_f = storage.FogA_f;
	currentVisuals.SunIntensity_f = storage.SunIntensity_f;
	currentVisuals.SunRed_f = storage.SunRed_f;
	currentVisuals.SunGreen_f = storage.SunGreen_f;
	currentVisuals.SunBlue_f = storage.SunBlue_f;
	currentVisuals.ZenithRed_f = storage.ZenithRed_f;
	currentVisuals.ZenithGreen_f = storage.ZenithGreen_f;
	currentVisuals.ZenithBlue_f = storage.ZenithBlue_f;
	currentVisuals.bBloomEnabled = storage.bBloomEnabled;
	currentVisuals.bLevelVSyncEnabled = storage.bLevelVSyncEnabled;
	currentVisuals.bSSREnabled = storage.bSSREnabled;
	currentVisuals.bReflectionsEnabled = storage.bReflectionsEnabled;
	currentVisuals.bLightShafts = storage.bLightShafts;
	currentVisuals.bLensFlare = storage.bLensFlare;
	currentVisuals.bAutoExposure = storage.bAutoExposure;
	currentVisuals.fGamma = storage.fGamma;
	currentVisuals.fDeSaturate = storage.fDeSaturate;
	
	currentVisuals.SunAngleX = storage.SunAngleX;
	currentVisuals.SunAngleY = storage.SunAngleY;
	currentVisuals.SunAngleZ = storage.SunAngleZ;
	currentVisuals.iTimeOfday = storage.iTimeOfday;
	currentVisuals.fExposure = storage.fExposure;
	currentVisuals.skyindex = storage.skyindex;
	currentVisuals.refreshshaders = 1;
}

void importer_free_wicked(void)
{
	// show previously hidden editor modes
	waypoint_showall ( );

	// free resources (with wicked, deleting objects CAN CRASH as threads may still be using them - need a way to signal there deleting at the right time)
	//PE: Imgui can still be visible with the textures , so free after import window calls.
	if ( ObjectExist(t.importer.objectnumber) ) HideObject ( t.importer.objectnumber ); // while testing all import formats

	// deactivate importer mode
	t.importer.loaded = 0;

	// return tab mode back to original state
	g.tabmode = t.importer.oldTabMode;

	// hide debug bone visuals
	wiRenderer::SetToDrawDebugBoneLines(false);

	// delete any surface files that were created but not used on the imported model.
	importer_delete_old_surface_files();

	// set back to initial dir
	SetDir ( t.importer.startDir.Get() );
}

void importer_free ( void )
{
	importer_free_wicked();
	if (GetImageExistEx(IMPORTER_TMP_IMAGE))
	{
		DeleteImage(IMPORTER_TMP_IMAGE);
	}

	// Delete the sphere and planes (3 objects starting at g.importerextraobjectoffset).
	for (int i = 2; i >= 0; i--)
	{
		if (ObjectExist(g.importerextraobjectoffset + i))
		{
			DeleteObject(g.importerextraobjectoffset + i);
		}
	}
	
	//	Delete the textures used by the spheres and planes (3 textures starting at g.importerextraimageoffset).
	for (int i = 3; i >= 0; i--)
	{
		if (GetImageExistEx(g.importerextraimageoffset + i))
		{
			DeleteImage(g.importerextraimageoffset + i);
		}
	}

	// Restore the camera's orientation from before the importer loaded.
	PositionCamera(t.importer.fPrevCamX, t.importer.fPrevCamY, t.importer.fPrevCamZ);
	t.editorfreeflight.c.x_f = CameraPositionX();
	t.editorfreeflight.c.y_f = CameraPositionY();
	t.editorfreeflight.c.z_f = CameraPositionZ();
	t.editorfreeflight.c.angx_f = t.importer.fPrevCamAngX;
	t.editorfreeflight.c.angy_f = t.importer.fPrevCamAngY;

	restore_visuals(t.visuals, t.visualsStorage);
	restore_visuals(t.editorvisuals, t.visualsStorage);
}

cstr importer_getfilenameonly ( LPSTR pFileAndPossiblePath )
{
	// Special case is when stripping path of a character creator body part, we need to retain the path
	if (pFileAndPossiblePath)
	{
		LPSTR pCCPPath = "charactercreatorplus\\parts\\";
		if ( strnicmp ( pFileAndPossiblePath, pCCPPath, strlen(pCCPPath) ) == NULL )
			return pFileAndPossiblePath;
	}

	cstr pFileNameOnly = pFileAndPossiblePath;
	for ( int n = strlen(pFileAndPossiblePath); n > 0; n-- )
	{
		if ( pFileAndPossiblePath[n] == '\\' || pFileAndPossiblePath[n] == '/' )
		{
			pFileNameOnly = cstr(pFileAndPossiblePath+n+1);
			break;
		}
	}
	return pFileNameOnly;
}

int importer_findtextureinlist ( LPSTR pFindFilename )
{
	for ( int iImageListIndex = 1; iImageListIndex < IMPORTERTEXTURESMAX; iImageListIndex++ )
	{
		if ( t.importerTextures[iImageListIndex].imageID > 0 )
		{
			cstr pCompareWith = t.importerTextures[iImageListIndex].fileName;
			if ( strnicmp ( pCompareWith.Get(), pFindFilename, strlen(pCompareWith.Get()) ) == NULL )
			{
				return iImageListIndex;
			}
		}
	}
	return 0;
}

int importer_findtextureindexinlist ( LPSTR pFindFilename )
{
	int iImageListindex = importer_findtextureinlist ( pFindFilename );
	if ( iImageListindex > 0 )
		return t.importerTextures[iImageListindex].imageID;
	else
		return 0;
}

int importer_addtexturefiletolist ( cstr fileName, cstr sourceName, int* tCount )
{
	// Check if we already have this texture
	t.tfound = 0;
	for ( t.tCount2 = 1 ; t.tCount2 <= IMPORTERTEXTURESMAX; t.tCount2++ )
	{
		if (  t.importerTextures[t.tCount2].fileName  == fileName ) 
		{
			t.tfound = 1;
			break;
		}
	}

	// If we don't have the texture, add it to list
	if ( t.tfound == 0 ) 
	{
		// find free slot
		t.tfound = 0;
		for ( int tCount3 = 1 ; tCount3 <= IMPORTERTEXTURESMAX; tCount3++ )
		{
			if ( strlen ( t.importerTextures[tCount3].fileName.Get() ) == 0 ) 
			{
				t.tfound = tCount3;
				break;
			}
		}

		// did we find a free texture slot?
		if ( t.tfound > 0 ) 
		{
			//  Add to importer texture list
			if ( t.tfound < IMPORTERTEXTURESMAX )
			{
				if ( t.tfound <= (*tCount) )
				{
					t.importerTextures[t.tfound].fileName = fileName;
					t.importerTextures[t.tfound].originalName = sourceName;
				}
				else
				{
					++(*tCount);
					t.importerTextures[(*tCount)].fileName = fileName;
					t.importerTextures[(*tCount)].originalName = sourceName;
				}
			}
		}
	}
	return t.tfound;
}

void importer_addtoimagelistandloadifexist ( LPSTR pImgFilename, int iOptionalStage, int iOptionalBaseImageSlotIndex )
{
	int tCount = t.tcounttextures;
	if ( FileExist ( pImgFilename ) )
	{
		// assign image to new slot in image list
		int iInsertedAtSlot = importer_addtexturefiletolist ( pImgFilename, pImgFilename, &tCount );

		// assign any special texture 'stage' value
		t.importerTextures[iInsertedAtSlot].iOptionalStage = iOptionalStage;
		t.importerTextures[iInsertedAtSlot].iAssociatedBaseImage = iOptionalBaseImageSlotIndex;

		// load image in
		t.tImageID = g.importermenuimageoffset+15;
		while ( ImageExist(t.tImageID) == 1 ) ++t.tImageID;
		LoadImage ( t.importerTextures[iInsertedAtSlot].fileName.Get(), t.tImageID );
		t.importerTextures[iInsertedAtSlot].imageID = t.tImageID;
	}
	t.tcounttextures = tCount;
}

void importer_findimagetypesfromlist ( cstr fileName, int iBaseImageSlotIndex, int* piImgColor, int* piImgNormal, int* piImgSpecular, int* piImgGloss, int* piImgAO, int* piImgHeight )
{
	// get base filename extension (deduct image format ext) 
	LPSTR pExt = NULL;
	for ( int iImgFormat = 0; iImgFormat < 4; iImgFormat++ )
	{
		if ( iImgFormat == 0 ) pExt = ".png";
		if ( iImgFormat == 1 ) pExt = ".dds";
		if ( iImgFormat == 2 ) pExt = ".tga";
		if ( iImgFormat == 3 ) pExt = ".jpg";
		if ( strnicmp ( fileName.Get()+strlen(fileName.Get())-strlen(pExt), pExt, strlen(pExt) ) == NULL ) 
			break;
	}
	if ( pExt == NULL ) return;

	// get base filename extension (deduct color specifier)
	LPSTR pImgType = NULL;
	cstr pBaseNoFileExt = cstr(Left(fileName.Get(),Len(fileName.Get())-Len(pExt)));
	for ( int iImgType = 0; iImgType < 6; iImgType++ )
	{
		// find kind of 'color' image type
		if ( iImgType == 0 ) pImgType = "_diffuse";
		if ( iImgType == 1 ) pImgType = "_color";
		if ( iImgType == 2 ) pImgType = "_d";
		if ( iImgType == 3 ) pImgType = "_albedo";
		if ( iImgType == 4) pImgType = "_diff";
		if ( iImgType == 5) pImgType = "_dif";
		
		if ( strnicmp ( pBaseNoFileExt.Get()+strlen(pBaseNoFileExt.Get())-strlen(pImgType), pImgType, strlen(pImgType) ) == NULL )
		{
			// get base filename (minus image type)
			cstr pBaseFile = cstr(Left(pBaseNoFileExt.Get(),Len(pBaseNoFileExt.Get())-Len(pImgType)));

			// locate color image from image list
			cstr pColorFile = pBaseFile + cstr(pImgType) + cstr(pExt);
			*piImgColor = importer_findtextureindexinlist ( pColorFile.Get() );

			// attempt to locate other textures associated with color image
			cstr pNormalFile = pBaseFile + cstr("_normal") + cstr(pExt);
			importer_addtoimagelistandloadifexist ( pNormalFile.Get(), 2, iBaseImageSlotIndex );
			*piImgNormal = importer_findtextureindexinlist ( pNormalFile.Get() );

			if (*piImgNormal == 0) 
			{
				cstr pNormalFile = pBaseFile + cstr("_ddn") + cstr(pExt);
				importer_addtoimagelistandloadifexist(pNormalFile.Get(), 2, iBaseImageSlotIndex);
				*piImgNormal = importer_findtextureindexinlist(pNormalFile.Get());
				if (*piImgNormal == 0) {
					cstr pNormalFile = pBaseFile + cstr("_nrm") + cstr(pExt);
					importer_addtoimagelistandloadifexist(pNormalFile.Get(), 2, iBaseImageSlotIndex);
					*piImgNormal = importer_findtextureindexinlist(pNormalFile.Get());
				}
			}

			cstr pSpecularFile = pBaseFile + cstr("_specular") + cstr(pExt);
			importer_addtoimagelistandloadifexist ( pSpecularFile.Get(), 3, iBaseImageSlotIndex );
			*piImgSpecular = importer_findtextureindexinlist ( pSpecularFile.Get() );
			if ( *piImgSpecular == 0 )
			{
				cstr pMetalnessFile = pBaseFile + cstr("_metalness") + cstr(pExt);
				importer_addtoimagelistandloadifexist ( pMetalnessFile.Get(), 3, iBaseImageSlotIndex );
				*piImgSpecular = importer_findtextureindexinlist ( pMetalnessFile.Get() );
			}

			cstr pGlossFile = pBaseFile + cstr("_gloss") + cstr(pExt);
			importer_addtoimagelistandloadifexist ( pGlossFile.Get(), 4, iBaseImageSlotIndex );
			*piImgGloss = importer_findtextureindexinlist ( pGlossFile.Get() );

			cstr pAOFile = pBaseFile + cstr("_ao") + cstr(pExt);
			importer_addtoimagelistandloadifexist ( pAOFile.Get(), 1, iBaseImageSlotIndex );
			*piImgAO = importer_findtextureindexinlist ( pAOFile.Get() );
			if ( *piImgAO == 0 ) 
			{
				pAOFile = g.rootdir_s + cstr("effectbank\\reloaded\\media\\blank_O.dds");
				importer_addtoimagelistandloadifexist ( pAOFile.Get(), 1, iBaseImageSlotIndex );
				*piImgAO = importer_findtextureindexinlist ( pAOFile.Get() );
			}

			cstr pHeightFile = pBaseFile + cstr("_height") + cstr(pExt);
			importer_addtoimagelistandloadifexist ( pHeightFile.Get(), 5, iBaseImageSlotIndex );
			*piImgHeight = importer_findtextureindexinlist ( pHeightFile.Get() );
		}
	}
}

void importer_findandremoveentry ( cstr sFileToRemove )
{
	for ( int tCount = 1 ; tCount <= IMPORTERTEXTURESMAX; tCount++ )
	{
		if ( strnicmp ( t.importerTextures[tCount].fileName.Get(), sFileToRemove.Get(), strlen(sFileToRemove.Get())-4 ) == NULL ) 
		{
			if ( ImageExist ( t.importerTextures[tCount].imageID ) == 1 ) DeleteImage ( t.importerTextures[tCount].imageID );
			t.importerTextures[tCount].imageID = 0;
			t.importerTextures[tCount].fileName = "";
		}
	}
}

void importer_removeentryandassociatesof ( int tCount )
{
	// the file to delete (and all its associates)
	LPSTR pRemoveTextureFile = t.importerTextures[tCount].fileName.Get();

	// get base filename extension (deduct image format ext)
	LPSTR pExt = NULL;
	for ( int iImgFormat = 0; iImgFormat < 4; iImgFormat++ )
	{
		if ( iImgFormat == 0 ) pExt = ".png";
		if ( iImgFormat == 1 ) pExt = ".dds";
		if ( iImgFormat == 2 ) pExt = ".tga";
		if ( iImgFormat == 3 ) pExt = ".jpg";
		if ( strnicmp ( pRemoveTextureFile+strlen(pRemoveTextureFile)-strlen(pExt), pExt, strlen(pExt) ) == NULL ) 
			break;
	}
	if ( pExt == NULL ) return;

	// get base filename extension (deduct color specifier)
	LPSTR pImgType = NULL;
	cstr pBaseNoFileExt = cstr(Left(pRemoveTextureFile,Len(pRemoveTextureFile)-Len(pExt)));
	for ( int iImgType = 0; iImgType < 5; iImgType++ )
	{
		// find kind of 'color' image type
		if ( iImgType == 0 ) pImgType = "_diffuse";
		if ( iImgType == 1 ) pImgType = "_color";
		if ( iImgType == 2 ) pImgType = "_d";
		if ( iImgType == 3 ) pImgType = "_albedo";
		if ( strnicmp ( pBaseNoFileExt.Get()+strlen(pBaseNoFileExt.Get())-strlen(pImgType), pImgType, strlen(pImgType) ) == NULL ) 
		{
			// get base filename (minus image type)
			cstr pBaseFile = cstr(Left(pBaseNoFileExt.Get(),Len(pBaseNoFileExt.Get())-Len(pImgType)));

			// locate color image from image list
			importer_findandremoveentry( pBaseFile + cstr("_normal") + cstr(pExt) );
			importer_findandremoveentry( pBaseFile + cstr("_specular") + cstr(pExt) );
			importer_findandremoveentry( pBaseFile + cstr("_metalness") + cstr(pExt) );
			importer_findandremoveentry( pBaseFile + cstr("_gloss") + cstr(pExt) );
			importer_findandremoveentry( pBaseFile + cstr("_ao") + cstr(pExt) );
			importer_findandremoveentry( pBaseFile + cstr("_height") + cstr(pExt) );
		}
	}
}

void importer_applyimagelisttextures ( bool bCubeMapOnly, int iOptionalOnlyUpdateImageListIndex, bool bExpandOutPBRTextureSet )
{
	// either update cube map only (for model that already has its texture stages intact)
	// work out object texture stages (based on shader chosen)
	int iColorStage = 0;
	int iAOStage = 1;
	int iNormalStage = 2;
	int iSpecularStage = 3;
	int iGlossStage = 4;
	int iHeightStage = -1;
	int iEnvStage = 6;
	int iIllumStage = 7;
	int iImageIndexForCUBE = 72543;
	cstr pShaderCUBE = "effectbank\\reloaded\\media\\CUBE.dds";
	LoadImage ( pShaderCUBE.Get(), iImageIndexForCUBE, 2 );
	PerformCheckListForLimbs ( t.importer.objectnumber );
	int iTextureCount = 0;
	for ( int tCount = 0 ; tCount <= ChecklistQuantity()-1; tCount++ )
	{
		LPSTR sTmp = LimbTextureName(t.importer.objectnumber, tCount);
		cstr pLimbTextureName = importer_getfilenameonly (sTmp);
		if (sTmp) delete[] sTmp;
		if ( strlen ( pLimbTextureName.Get() ) > 0 )
		{
			// new FPE field to specify limbs that are hair (i.e. no zwrite, no culling)
			cstr pBaseFile = Left ( pLimbTextureName.Get(), strlen(pLimbTextureName.Get())-4 );
			if ( strnicmp ( pBaseFile.Get() + strlen(pBaseFile.Get()) - 6, "_color", 6 ) == NULL )
			{
				pBaseFile = Left ( pBaseFile.Get(), strlen(pBaseFile.Get())-6 );
				if ( strnicmp ( pBaseFile.Get() + strlen(pBaseFile.Get()) - 5, "_hair", 5 ) == NULL )
				{
					// switch off culling (leave zwrite as distant hair rendered over nearer hair)
					SetLimbCull ( t.importer.objectnumber, tCount, false );
				}
			}

			// apply cube map regardless
			TextureLimbStage ( t.importer.objectnumber, tCount, iEnvStage, iImageIndexForCUBE );

			// also apply default textures where texture slot does not have image
			sObject* pObject = GetObjectData ( t.importer.objectnumber );
			if ( pObject )
			{
				if ( tCount <= pObject->iFrameCount )
				{
					sFrame* pFrame = pObject->ppFrameList[tCount];
					if ( pFrame->pMesh )
					{
						sMesh* pMesh = pFrame->pMesh;
						int iTextureMax = pMesh->dwTextureCount;
						if ( iTextureMax > 4 ) iTextureMax = 7;
						for ( int iTexture = 1; iTexture <= iTextureMax; iTexture++ )
						{
							if ( pMesh->pTextures[iTexture].iImageID == 0 )
							{
								int iDefaultImage = 0;
								if ( iTexture == iAOStage ) iDefaultImage = loadinternaltextureex("effectbank\\reloaded\\media\\blank_O.dds",1,t.tfullorhalfdivide);
								if ( iTexture == iNormalStage ) iDefaultImage = loadinternaltextureex("effectbank\\reloaded\\media\\blank_N.dds",1,t.tfullorhalfdivide);
								if ( iTexture == iSpecularStage ) iDefaultImage = loadinternaltextureex("effectbank\\reloaded\\media\\blank_black.dds",1,t.tfullorhalfdivide);
								if ( iTexture == iGlossStage ) iDefaultImage = loadinternaltextureex("effectbank\\reloaded\\media\\white_D.dds",1,t.tfullorhalfdivide);
								if ( iTexture == iIllumStage) iDefaultImage = loadinternaltextureex("effectbank\\reloaded\\media\\detail_default.dds", 1, t.tfullorhalfdivide);
								if (iTexture != 5 && iTexture != 6)
								{
									TextureLimbStage (t.importer.objectnumber, tCount, iTexture, iDefaultImage);
								}
							}
						}
					}
				}
			}

			// count textures specified in model
			iTextureCount++;
		}
	}

	// should map new texture choices to the original image slots
	if ( g_piMapImageListIndexToLimbIndex == NULL )
	{
		bool bAbsolutelyNoTexturesReferencedAnywhere = true;
		g_dwMapImageListIndexToLimbCount = ChecklistQuantity(); 
		g_piMapImageListIndexToLimbIndex = new int[g_dwMapImageListIndexToLimbCount];
		for ( int tLimbIndex = 0 ; tLimbIndex <= ChecklistQuantity()-1; tLimbIndex++ )
		{
			int iFindImageSlotIndex = -1;
			LPSTR sTmp = LimbTextureName(t.importer.objectnumber, tLimbIndex);
			cstr pLimbTextureName = importer_getfilenameonly (sTmp);
			if (sTmp) delete[] sTmp;
			LPSTR pSearch = pLimbTextureName.Get();
			if ( strlen ( pSearch ) > 0 )
			{
				iFindImageSlotIndex = 1; // 220618 - default to slot one if no specific match can be made (so model CAN be textured)
				bAbsolutelyNoTexturesReferencedAnywhere = false;
				for ( int tCount = 1; tCount <= t.tcounttextures; tCount++ )
				{
					LPSTR pThisImageItem = t.importerTextures[tCount].fileName.Get();
					if ( strnicmp ( pThisImageItem + strlen(pThisImageItem) - strlen(pSearch), pSearch, strlen(pSearch)-4 ) == NULL )
					{
						iFindImageSlotIndex = tCount;
					}
				}
			}
			g_piMapImageListIndexToLimbIndex [ tLimbIndex ] = iFindImageSlotIndex;
		}
		if ( bAbsolutelyNoTexturesReferencedAnywhere == true )
		{
			// okay, so now we know that no meshes are particular about their texture, 
			// we will use slot one for all textures on this model
			int iFindImageSlotIndex = 1;
			sObject* pObject = GetObjectData ( t.importer.objectnumber );
			if ( pObject )
			{
				for ( int tLimbIndex = 0 ; tLimbIndex <= ChecklistQuantity()-1; tLimbIndex++ )
				{
					sFrame* pFrame = pObject->ppFrameList[tLimbIndex];
					if ( pFrame )
					{
						if ( pFrame->pMesh )
						{
							g_piMapImageListIndexToLimbIndex [ tLimbIndex ] = iFindImageSlotIndex;
						}
					}
				}
			}
		}
	}

	// or full retexture model from imagelist
	if ( bCubeMapOnly == false )
	{
		// texture stage specific non-base (normal, ao, etc)
		int iOptionalStage = 0;
		if ( iOptionalOnlyUpdateImageListIndex > 0 ) iOptionalStage = t.importerTextures[iOptionalOnlyUpdateImageListIndex].iOptionalStage;
		if ( iOptionalStage > 0 )
		{
			int iAssociatedBaseImage = t.importerTextures[iOptionalOnlyUpdateImageListIndex].iAssociatedBaseImage;
			for ( int tLimbIndex = 0 ; tLimbIndex <= ChecklistQuantity()-1; tLimbIndex++ )
			{
				int iImageListIndex = g_piMapImageListIndexToLimbIndex [ tLimbIndex ];
				if ( iImageListIndex > 0 && t.importerTextures[iImageListIndex].iAssociatedBaseImage > 0 ) iImageListIndex = t.importerTextures[iImageListIndex].iAssociatedBaseImage;
				if ( iImageListIndex > 0 && iAssociatedBaseImage == iImageListIndex )
				{
					if ( iOptionalStage == 2 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iNormalStage, t.importerTextures[iOptionalOnlyUpdateImageListIndex].imageID );
					if ( iOptionalStage == 3 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iSpecularStage, t.importerTextures[iOptionalOnlyUpdateImageListIndex].imageID );
					if ( iOptionalStage == 4 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iGlossStage, t.importerTextures[iOptionalOnlyUpdateImageListIndex].imageID );
					if ( iOptionalStage == 1 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iAOStage, t.importerTextures[iOptionalOnlyUpdateImageListIndex].imageID );
				}
			}
		}
		else
		{
			// texture all model or just one limb
			for ( int tLimbIndex = 0 ; tLimbIndex <= ChecklistQuantity()-1; tLimbIndex++ )
			{
				int iImageListIndex = g_piMapImageListIndexToLimbIndex [ tLimbIndex ];
				if ( iImageListIndex > 0 && (iOptionalOnlyUpdateImageListIndex == -1 || iOptionalOnlyUpdateImageListIndex == iImageListIndex ))
				{
					cstr pLimbTextureName = importer_getfilenameonly ( t.importerTextures[iImageListIndex].fileName.Get() );
					if ( strlen ( pLimbTextureName.Get() ) > 0 )
					{
						if ( t.importerTextures[iImageListIndex].imageID > 0 )
						{
							// diffuse/albedo
							TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iColorStage, t.importerTextures[iImageListIndex].imageID );

							// find and load any associated PBR textures for this color texture choice
							if ( bExpandOutPBRTextureSet == true )
							{
								// only find every texture on initial texture load, not when replacing specific texture slots
								int iImgColor=0, iImgNormal=0, iImgSpecular=0, iImgGloss=0, iImgAO=0, iImgHeight=0;
								importer_findimagetypesfromlist ( t.importerTextures[iImageListIndex].fileName, iImageListIndex, &iImgColor, &iImgNormal, &iImgSpecular, &iImgGloss, &iImgAO, &iImgHeight );
								if ( iImgColor > 0 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iColorStage, iImgColor );
								if ( iImgNormal > 0 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iNormalStage, iImgNormal );
								if ( iImgSpecular > 0 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iSpecularStage, iImgSpecular );
								if ( iImgGloss > 0 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iGlossStage, iImgGloss );
								if ( iAOStage != - 1 && iImgAO > 0 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iAOStage, iImgAO );
								if ( iImgHeight > 0 && iHeightStage != - 1 ) TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iHeightStage, iImgHeight );
							}

							// apply environment cube map
							TextureLimbStage ( t.importer.objectnumber, tLimbIndex, iEnvStage, iImageIndexForCUBE );
						}
					}
				}
			}
		}
	}
}

void importer_assignsprite ( int tCount )
{
	if (bRemoveSprites)
		return;
	t.tSpriteID = 50;
	while (  SpriteExist(t.tSpriteID) == 1 ) ++t.tSpriteID;
	t.importerTextures[tCount].spriteID = t.tSpriteID+1;
	t.importerTextures[tCount].spriteID2 = t.tSpriteID;
	MAXSprite ( t.importerTextures[tCount].spriteID, -10000, -10000, g.importermenuimageoffset+6 );
	MAXSprite ( t.importerTextures[tCount].spriteID2, -10000, -10000, g.importermenuimageoffset+6 );
}

void importer_recreate_texturesprites ( void )
{
	if (bRemoveSprites)
		return;
	// Create sprite to represent texture 
	t.tSpriteID = 50;
	for ( int tCount = 1; tCount <= t.tcounttextures; tCount++ )
	{
		if ( SpriteExist ( t.tSpriteID+0 ) == 1 ) DeleteSprite ( t.tSpriteID+0 );
		if ( SpriteExist ( t.tSpriteID+1 ) == 1 ) DeleteSprite ( t.tSpriteID+1 );
		importer_assignsprite ( tCount );
		t.tSpriteID+=2;
	}
	for ( int tCount = t.tcounttextures+1; tCount < IMPORTERTEXTURESMAX; tCount++ )
	{
		int iSprite = t.importerTextures[tCount].spriteID;
		int iSprite2 = t.importerTextures[tCount].spriteID2;
		if ( iSprite > 0 && SpriteExist ( iSprite ) == 1 ) DeleteSprite ( iSprite );
		if ( iSprite2 > 0 && SpriteExist ( iSprite2 ) == 1 ) DeleteSprite ( iSprite2 );
	}
}

int giRememberLastEffectIndexInImporter = -1;

void importer_changeshader ( LPSTR pNewShaderFilename )
{
	// when shader changed in importer dialog, change shader of imported model
	if ( t.importer.objectnumber > 0 )
	{
		if ( ObjectExist ( t.importer.objectnumber ) == 1 )
		{
			char pRelativeEffectPath[512];
			strcpy ( pRelativeEffectPath, "effectbank\\reloaded\\" );
			strcat ( pRelativeEffectPath, pNewShaderFilename );
			if ( giRememberLastEffectIndexInImporter > 0 ) deleteinternaleffect ( giRememberLastEffectIndexInImporter );
			int iEffectID = loadinternaleffectunique ( pRelativeEffectPath, 1 ); //PE: old effect never deleted. ?
			DeleteObject ( t.importer.objectnumber );
			CloneObject ( t.importer.objectnumber, t.importer.objectnumberpreeffectcopy );
			importer_applyimagelisttextures ( true, -1, false );
			importer_recreate_texturesprites();
			LockObjectOn ( t.importer.objectnumber );
			SetObjectEffect ( t.importer.objectnumber, iEffectID ); 

			GlueObjectToLimbEx ( t.importer.objectnumber, t.importerGridObject[8], 0 , 1 );
			giRememberLastEffectIndexInImporter = iEffectID;
			//PE: Bug. reset effect clip , so visible.
			t.tnothing = MakeVector4(g.characterkitvector);
			SetVector4(g.characterkitvector, 500000, 1, 0, 0);
			SetEffectConstantV(iEffectID, "EntityEffectControl", g.characterkitvector);
			t.tnothing = DeleteVector4(g.characterkitvector);
		}
	}

	//Reset colors and update.
	visuals_editordefaults();
	t.visuals.SurfaceSunFactor_f = 0.75;
	t.visuals.AmbienceIntensity_f = 195.0f;
	t.visuals.SurfaceIntensity_f = 0.9f;
	t.visuals.refreshshaders = 1;
	visuals_loop();
}

bool animsystem_buildanimslots(int objectnumber)
{
	// ensure we have animations, and add them to list
	sObject* pObject = GetObjectData(objectnumber);
	g_pAnimSlotList.clear();
	int iObjectFramesRunningTotal = 0;
	bFoundanimSet = false;
	if (pObject->pAnimationSet)
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		while (pAnimSet != NULL)
		{
			// create anim slot from anim set
			// only list animsets that have animations (Take001 can have zero anims sometimes)
			sAnimSlotStruct animslotitem;
			animslotitem.fStep1 = 0;
			animslotitem.fStep2 = 0;
			animslotitem.fStep3 = 0;
			if (strlen(pAnimSet->szName) >= 32)
			{
				memcpy(animslotitem.pName, pAnimSet->szName, 31);
				animslotitem.pName[31] = 0;
			}
			else
			{
				strcpy(animslotitem.pName, pAnimSet->szName);
			}
			if (strlen(animslotitem.pName) == 0) strcpy(animslotitem.pName, "Included");
			animslotitem.fStart = iObjectFramesRunningTotal;
			animslotitem.fFinish = iObjectFramesRunningTotal + pAnimSet->ulLength;
			if (pAnimSet->dwAnimSetType == 124)
			{
				// when triggered, can copy start/finish references on reloading
				animslotitem.fStart = pAnimSet->fAnimSetStart;
				animslotitem.fFinish = pAnimSet->fAnimSetFinish;
				animslotitem.fStep1 = pAnimSet->fAnimSetStep1;
				animslotitem.fStep2 = pAnimSet->fAnimSetStep2;
				animslotitem.fStep3 = pAnimSet->fAnimSetStep3;
			}
			animslotitem.bLooped = true;
			g_pAnimSlotList.push_back(animslotitem);
			bFoundanimSet = true;
			// total all frames in all animsets
			iObjectFramesRunningTotal += pAnimSet->ulLength + 1;
			// next animation set
			pAnimSet = pAnimSet->pNext;
		}
	}
	return bFoundanimSet;
}

void animsystem_prepareobjectforanimtool(int objectnumber, int iNotUsed)
{
	// get object ptr
	sObject* pObject = GetObjectData(objectnumber);

	// ensure new animation list is created from animation data
	g_pAnimSlotList.clear();

	// ensure we have animations, and add them to list
	int iObjectFramesRunningTotal = 0;
	bFoundanimSet = false;
	if (pObject->pAnimationSet)
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		while (pAnimSet != NULL)
		{
			// create anim slot from anim set
			if (1) // LB: We NEED anim slots of zero length as we can have start/finish slots with same frame value (for on/off anims) - pAnimSet->ulLength > 0)
			{
				// only list animsets that have animations (Take001 can have zero anims sometimes)
				sAnimSlotStruct animslotitem;
				animslotitem.fStep1 = 0;
				animslotitem.fStep2 = 0;
				animslotitem.fStep3 = 0;
				if (strlen(pAnimSet->szName) >= 32)
				{
					memcpy(animslotitem.pName, pAnimSet->szName, 31);
					animslotitem.pName[31] = 0;
				}
				else
				{
					strcpy(animslotitem.pName, pAnimSet->szName);
				}
				if (strlen(animslotitem.pName) == 0) strcpy(animslotitem.pName, "Included");
				animslotitem.fStart = iObjectFramesRunningTotal;
				animslotitem.fFinish = iObjectFramesRunningTotal + pAnimSet->ulLength;
				if (pAnimSet->dwAnimSetType == 124)
				{
					// when triggered, can copy start/finish references on reloading
					animslotitem.fStart = pAnimSet->fAnimSetStart;
					animslotitem.fFinish = pAnimSet->fAnimSetFinish;
					animslotitem.fStep1 = pAnimSet->fAnimSetStep1;
					animslotitem.fStep2 = pAnimSet->fAnimSetStep2;
					animslotitem.fStep3 = pAnimSet->fAnimSetStep3;
				}
				animslotitem.bLooped = true;
				g_pAnimSlotList.push_back(animslotitem);
				bFoundanimSet = true;

				// total all frames in all animsets
				iObjectFramesRunningTotal += pAnimSet->ulLength + 1;
			}
			// next animation set
			pAnimSet = pAnimSet->pNext;
		}
	}
	if (bFoundanimSet == true)
	{
		// now create "animset core zero" if one does not exist
		if (pObject->pAnimationSet->dwAnimSetType != 123)
		{
			// animset core zero holds ALL the separate animsets further down list
			// so that animation system can act on a single contiguous series of frame numbers
			sAnimationSet* pAnimCoreSet = new sAnimationSet();
			strcpy(pAnimCoreSet->szName, "All");
			pAnimCoreSet->dwAnimSetType = 123; // 123 = New AnimSet Core Zero (holds all other animset frames for single submission to wicked)
			pAnimCoreSet->ulLength = iObjectFramesRunningTotal;

			// first entry is the aminset core zero
			sAnimSlotStruct animslotitem;
			animslotitem.fStep1 = 0;
			animslotitem.fStep2 = 0;
			animslotitem.fStep3 = 0;
			if (strlen(pAnimCoreSet->szName) >= 32)
			{
				memcpy(animslotitem.pName, pAnimCoreSet->szName, 31);
				animslotitem.pName[31] = 0;
			}
			else
			{
				strcpy(animslotitem.pName, pAnimCoreSet->szName);
			}
			animslotitem.fStart = 0;
			animslotitem.fFinish = pAnimCoreSet->ulLength;
			animslotitem.bLooped = true;
			g_pAnimSlotList.insert(g_pAnimSlotList.begin(), animslotitem);

			// ensure we remove the wicked anim component which WAS linked to the first animset (which wicked uses for all animations on this object)
			if (pObject->pAnimationSet->wickedanimentityindex > 0)
			{
				wiScene::AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent( pObject->pAnimationSet->wickedanimentityindex );
				if (animationcomponent)
				{
					for (int i = 0; i < animationcomponent->samplers.size(); i++)
					{
						wiScene::GetScene().Entity_Remove(animationcomponent->samplers[i].data);
					}
				}
				wiScene::GetScene().Entity_Remove(pObject->pAnimationSet->wickedanimentityindex);
				pObject->pAnimationSet->wickedanimentityindex = 0;
			}

			// integrate animset core zero as new first in list
			pAnimCoreSet->pNext = pObject->pAnimationSet;
			pObject->pAnimationSet = pAnimCoreSet;

			// now create animation list for core zero, and include all animations in lists below this one
			iObjectFramesRunningTotal = 0;
			sAnimationSet* pAnimSet = pAnimCoreSet->pNext;
			while (pAnimSet != NULL)
			{
				// take the animations from this animset, and append to animset core zero
				if (pAnimSet->ulLength > 0)
				{
					sAnimation* pCurrent = pAnimSet->pAnimation;
					bool bAnimationAppended = AppendAnimationDataCore(pCurrent, &pAnimCoreSet, iObjectFramesRunningTotal, false);
					iObjectFramesRunningTotal += pAnimSet->ulLength + 1;
				}
				pAnimSet = pAnimSet->pNext;
			}

			// cleanup by removing slots that are called $NoName$
			for (int i = 0; i < g_pAnimSlotList.size(); i++)
			{
				if (strcmp (g_pAnimSlotList[i].pName, "$NoName$") == NULL)
				{
					g_pAnimSlotList.erase(g_pAnimSlotList.begin() + i);
					i = 0;
				}
			}
		}

		// recalculate animation data and bounds for object
		MapFramesToAnimations(pObject, true);

		// Create animations as they come from animset core zero only
		WickedCall_RefreshObjectAnimations(pObject, pObject->wickedloaderstateptr);

		// trigger animation zero to play
		extern bool g_bCharacterCreatorPlusActivated;

		g_bAnimatingObjectPreview = true;
		g_bUpdateAnimationPreview = true;

		// prefer an idle animation as starting anim if one exists
		g_iCurrentAnimationSlotIndex = 0;

		if (g_bCharacterCreatorPlusActivated)
		{
			for (int i = 0; i < g_pAnimSlotList.size(); i++)
			{
				if (strstr(g_pAnimSlotList[i].pName, "Idle_LookAround"))
				{
					g_iCurrentAnimationSlotIndex = i;
					break;
				}

				if (i == g_pAnimSlotList.size() - 1)
				{
					// Couldn't find LookAround, search for normal anims.
					for (int i = 0; i < g_pAnimSlotList.size(); i++)
					{
						if (stricmp(g_pAnimSlotList[i].pName, "idle") == NULL || stricmp(g_pAnimSlotList[i].pName, "Zombie_Idle") == NULL)
						{
							g_iCurrentAnimationSlotIndex = i;
							break;
						}
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < g_pAnimSlotList.size(); i++)
			{
				// ZJ: Altered to also include zombie idle.
				if (stricmp(g_pAnimSlotList[i].pName, "idle") == NULL || stricmp(g_pAnimSlotList[i].pName, "Zombie_Idle") == NULL)
				{
					g_iCurrentAnimationSlotIndex = i;
					break;
				}
			}
		}
	}

	// show debug bones to help visualise what we are importing
	wiRenderer::SetToDrawDebugBoneLines(g_bShowBones);
}

void importer_loadmodel_wicked(void)
{
	// free any previous import object
	if (ObjectExist(t.importer.objectnumber)) importer_quit();
	if (GetImageExistEx(IMPORTER_TMP_IMAGE)) DeleteImage(IMPORTER_TMP_IMAGE);

	// initialise if not already inited
	if (t.importer.importerActive == 0)
	{
		importer_init();
	}

	g_MeshNamesAssimp.clear();

	// detect if format in beta support?
	bool bIsBetaFormat = true;
	if (t.timporterfile_s.Len() > 4) 
	{
		cStr sExt = Lower(Right(t.timporterfile_s.Get(), 4));
		if (sExt == ".fbx") bIsBetaFormat = false;
		if (sExt == ".obj") bIsBetaFormat = false;
		if (sExt == ".dbo") bIsBetaFormat = false;
		if (sExt == "gltf") bIsBetaFormat = false;
		sExt = Lower(Right(t.timporterfile_s.Get(), 2));
		if (sExt == ".x") bIsBetaFormat = false;
	}

	// separate path from model filename
	char pAbsPathAndFilename[MAX_PATH];
	strcpy ( pAbsPathAndFilename, t.timporterfile_s.Get() );
	for (int n = strlen(pAbsPathAndFilename); n > 0; n--)
	{
		if (pAbsPathAndFilename[n] == '\\' || pAbsPathAndFilename[n] == '/')
		{
			t.importer.objectFilename = pAbsPathAndFilename + n + 1;
			pAbsPathAndFilename[n+1] = 0;
			break;
		}
	}
	t.importer.objectFileOriginalPath = pAbsPathAndFilename;

	// work out FPE form and extension too
	char pFilename[MAX_PATH];
	strcpy(pFilename, t.importer.objectFilename.Get());
	for (int n = strlen(pFilename); n > 0; n--)
	{
		// Remove the file extension from the file name.
		if (pFilename[n] == '.')
		{
			t.importer.objectFilenameExtension = pFilename + n + 1;
			pFilename[n] = 0;
			break;
		}
	}
	t.importer.objectFilenameFPE = pFilename;
	t.importer.objectFilenameFPE += ".fpe";

	// pre-populate import name
	if (!t.importer.bQuitForReload || bBatchConverting==true)
	{
		strcpy(cImportName, pFilename);
	}
	t.importer.bQuitForReload = false;

	// importer overrides scaling during the load
	extern void SetLoadScale (enumScalingMode eScalingMode);
	enumScalingMode eThisScalingMode = (enumScalingMode)t.importer.lastscalingmodeused;
	SetLoadScale (eThisScalingMode);
	
	// load the imported object (powered by AssImp when not loading a DBO file)
	t.importer.objectnumber=g.importermenuobjectoffset+1;
	t.strwork = t.importer.objectFileOriginalPath + t.importer.objectFilename;
	if ( FileExist ( t.strwork.Get() ) ) 
	{
		if ( ObjectExist(t.importer.objectnumber) == 1 ) DeleteObject ( t.importer.objectnumber );
		LoadObject ( t.strwork.Get() ,t.importer.objectnumber );
	}
	else
	{
		t.strwork = ""; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\"+t.importer.objectFilename;
		if ( FileExist( t.strwork.Get() ) ) 
		{
			if ( ObjectExist(t.importer.objectnumber) == 1 ) DeleteObject ( t.importer.objectnumber );
			LoadObject ( t.strwork.Get() ,t.importer.objectnumber );
		}
		else
		{
			if ( ObjectExist(t.importer.objectnumber) == 1 ) DeleteObject ( t.importer.objectnumber );
			MakeObjectCube ( t.importer.objectnumber, 100 );
		}
	}
	if ( ObjectExist ( t.importer.objectnumber ) == 0 )
	{
		if ( ObjectExist(t.importer.objectnumber) == 1 ) DeleteObject ( t.importer.objectnumber );
		MakeObjectCube ( t.importer.objectnumber, 100 );
	}

	// load scaling override off
	SetLoadScale (eScalingMode_Off);

	sObject* pImportedObject = GetObjectData(t.importer.objectnumber);
	bool bEnableOneHundredMeshLimit = true;
	if (bEnableOneHundredMeshLimit == true)
	{
		if (pImportedObject->iMeshCount > 100)
		{
			strcpy(cTriggerMessage, "The imported model has over 100 meshes. GameGuru MAX only supports up to 100 mesh materials.");
			bTriggerMessage = true;
		}
	}

	// LB: go through all frames of imported model
	bool bConvertedBoneNames = false;
	for (int iFrame = 0; iFrame < pImportedObject->iFrameCount; iFrame++)
	{
		sFrame* pFrame = pImportedObject->ppFrameList[iFrame];
		if (pFrame)
		{
			LPSTR pOldFrameName = pFrame->szName;
			if (pOldFrameName)
			{
				if (strlen(pOldFrameName) > 0)
				{
					// look for common bone names, and transform to GG standard skeleton name
					const char* pNewName = "";
					if (stricmp(pOldFrameName, "mixamorig_Hips") == NULL) pNewName = "Bip01_Pelvis";
					if (stricmp(pOldFrameName, "mixamorig_Spine") == NULL) pNewName = "Bip01_Spine";
					if (stricmp(pOldFrameName, "mixamorig_LeftUpLeg") == NULL) pNewName = "Bip01_L_Thigh";
					if (stricmp(pOldFrameName, "mixamorig_LeftLeg") == NULL) pNewName = "Bip01_L_Calf";
					if (stricmp(pOldFrameName, "mixamorig_LeftFoot") == NULL) pNewName = "Bip01_L_Foot";
					if (stricmp(pOldFrameName, "mixamorig_LeftToeBase") == NULL) pNewName = "Bip01_L_Toe0";
					if (stricmp(pOldFrameName, "mixamorig_RightUpLeg") == NULL) pNewName = "Bip01_R_Thigh";
					if (stricmp(pOldFrameName, "mixamorig_RightLeg") == NULL) pNewName = "Bip01_R_Calf";
					if (stricmp(pOldFrameName, "mixamorig_RightFoot") == NULL) pNewName = "Bip01_R_Foot";
					if (stricmp(pOldFrameName, "mixamorig_RightToeBase") == NULL) pNewName = "Bip01_R_Toe0";
					if (stricmp(pOldFrameName, "mixamorig_Spine1") == NULL) pNewName = "Bip01_Spine1";
					if (stricmp(pOldFrameName, "mixamorig_Spine2") == NULL) pNewName = "Bip01_Spine2";
					if (stricmp(pOldFrameName, "mixamorig_Neck") == NULL) pNewName = "Bip01_Neck";
					if (stricmp(pOldFrameName, "mixamorig_LeftShoulder") == NULL) pNewName = "Bip01_L_Clavicle";
					if (stricmp(pOldFrameName, "mixamorig_RightShoulder") == NULL) pNewName = "Bip01_R_Clavicle";
					if (stricmp(pOldFrameName, "mixamorig_Head") == NULL) pNewName = "Bip01_Head";
					if (stricmp(pOldFrameName, "mixamorig_HeadTop_End") == NULL) pNewName = "Bip01_HeadTop";
					if (stricmp(pOldFrameName, "mixamorig_LeftArm") == NULL) pNewName = "Bip01_L_UpperArm";
					if (stricmp(pOldFrameName, "mixamorig_LeftForeArm") == NULL) pNewName = "Bip01_L_Forearm";
					if (stricmp(pOldFrameName, "mixamorig_RightArm") == NULL) pNewName = "Bip01_R_UpperArm";
					if (stricmp(pOldFrameName, "mixamorig_RightForeArm") == NULL) pNewName = "Bip01_R_Forearm";
					if (stricmp(pOldFrameName, "mixamorig_LeftHand") == NULL) pNewName = "Bip01_L_Hand";
					if (stricmp(pOldFrameName, "mixamorig_RightHand") == NULL) pNewName = "Bip01_R_Hand";
					if (strstr(pOldFrameName, "mixamorig_LeftHandThumb1") > 0) pNewName = "Bip01_L_Finger0";
					if (strstr(pOldFrameName, "mixamorig_LeftHandThumb2") > 0) pNewName = "Bip01_L_Finger01";
					if (strstr(pOldFrameName, "mixamorig_LeftHandThumb3") > 0) pNewName = "Bip01_L_Finger02";
					if (strstr(pOldFrameName, "mixamorig_LeftHandThumb4") > 0) pNewName = "Bip01_L_Finger03";
					if (strstr(pOldFrameName, "mixamorig_LeftHandIndex1") > 0) pNewName = "Bip01_L_Finger1";
					if (strstr(pOldFrameName, "mixamorig_LeftHandIndex2") > 0) pNewName = "Bip01_L_Finger11";
					if (strstr(pOldFrameName, "mixamorig_LeftHandIndex3") > 0) pNewName = "Bip01_L_Finger12";
					if (strstr(pOldFrameName, "mixamorig_LeftHandIndex4") > 0) pNewName = "Bip01_L_Finger13";
					if (strstr(pOldFrameName, "mixamorig_LeftHandMiddle1") > 0) pNewName = "Bip01_L_Finger2";
					if (strstr(pOldFrameName, "mixamorig_LeftHandMiddle2") > 0) pNewName = "Bip01_L_Finger21";
					if (strstr(pOldFrameName, "mixamorig_LeftHandMiddle3") > 0) pNewName = "Bip01_L_Finger22";
					if (strstr(pOldFrameName, "mixamorig_LeftHandMiddle4") > 0) pNewName = "Bip01_L_Finger23";
					if (strstr(pOldFrameName, "mixamorig_LeftHandRing1") > 0) pNewName = "Bip01_L_Finger3";
					if (strstr(pOldFrameName, "mixamorig_LeftHandRing2") > 0) pNewName = "Bip01_L_Finger31";
					if (strstr(pOldFrameName, "mixamorig_LeftHandRing3") > 0) pNewName = "Bip01_L_Finger32";
					if (strstr(pOldFrameName, "mixamorig_LeftHandRing4") > 0) pNewName = "Bip01_L_Finger33";
					if (strstr(pOldFrameName, "mixamorig_LeftHandPinky1") > 0) pNewName = "Bip01_L_Finger4";
					if (strstr(pOldFrameName, "mixamorig_LeftHandPinky2") > 0) pNewName = "Bip01_L_Finger41";
					if (strstr(pOldFrameName, "mixamorig_LeftHandPinky3") > 0) pNewName = "Bip01_L_Finger42";
					if (strstr(pOldFrameName, "mixamorig_LeftHandPinky4") > 0) pNewName = "Bip01_L_Finger43";
					if (strstr(pOldFrameName, "mixamorig_RightHandThumb1") > 0) pNewName = "Bip01_R_Finger0";
					if (strstr(pOldFrameName, "mixamorig_RightHandThumb2") > 0) pNewName = "Bip01_R_Finger01";
					if (strstr(pOldFrameName, "mixamorig_RightHandThumb3") > 0) pNewName = "Bip01_R_Finger02";
					if (strstr(pOldFrameName, "mixamorig_RightHandThumb4") > 0) pNewName = "Bip01_R_Finger03";
					if (strstr(pOldFrameName, "mixamorig_RightHandIndex1") > 0) pNewName = "Bip01_R_Finger1";
					if (strstr(pOldFrameName, "mixamorig_RightHandIndex2") > 0) pNewName = "Bip01_R_Finger11";
					if (strstr(pOldFrameName, "mixamorig_RightHandIndex3") > 0) pNewName = "Bip01_R_Finger12";
					if (strstr(pOldFrameName, "mixamorig_RightHandIndex4") > 0) pNewName = "Bip01_R_Finger13";
					if (strstr(pOldFrameName, "mixamorig_RightHandMiddle1") > 0) pNewName = "Bip01_R_Finger2";
					if (strstr(pOldFrameName, "mixamorig_RightHandMiddle2") > 0) pNewName = "Bip01_R_Finger21";
					if (strstr(pOldFrameName, "mixamorig_RightHandMiddle3") > 0) pNewName = "Bip01_R_Finger22";
					if (strstr(pOldFrameName, "mixamorig_RightHandMiddle4") > 0) pNewName = "Bip01_R_Finger23";
					if (strstr(pOldFrameName, "mixamorig_RightHandRing1") > 0) pNewName = "Bip01_R_Finger3";
					if (strstr(pOldFrameName, "mixamorig_RightHandRing2") > 0) pNewName = "Bip01_R_Finger31";
					if (strstr(pOldFrameName, "mixamorig_RightHandRing3") > 0) pNewName = "Bip01_R_Finger32";
					if (strstr(pOldFrameName, "mixamorig_RightHandRing4") > 0) pNewName = "Bip01_R_Finger33";
					if (strstr(pOldFrameName, "mixamorig_RightHandPinky1") > 0) pNewName = "Bip01_R_Finger4";
					if (strstr(pOldFrameName, "mixamorig_RightHandPinky2") > 0) pNewName = "Bip01_R_Finger41";
					if (strstr(pOldFrameName, "mixamorig_RightHandPinky3") > 0) pNewName = "Bip01_R_Finger42";
					if (strstr(pOldFrameName, "mixamorig_RightHandPinky4") > 0) pNewName = "Bip01_R_Finger43";

					// also look for any spaces used in the bone name
					bool bFoundASpace = false;
					char constructNewName[MAX_PATH];
					strcpy(constructNewName, pOldFrameName);
					if (strlen(constructNewName) > 0)
					{
						for (int n = 0; n < strlen(constructNewName); n++)
						{
							if (constructNewName[n] == ' ')
							{
								constructNewName[n] = '_';
								bFoundASpace = true;
							}
						}
						if(bFoundASpace==true) pNewName = constructNewName;
					}

					// replace all instances of this name (if found)
					if (strlen(pNewName) > 0)
					{
						// rename animation reference name too
						if (pImportedObject->pAnimationSet)
						{
							sAnimation* pCurrent = pImportedObject->pAnimationSet->pAnimation;
							while (pCurrent)
							{
								if (pCurrent->szName)
								{
									if (stricmp(pCurrent->szName, pFrame->szName) == NULL)
									{
										strcpy(pCurrent->szName, pNewName);
										break;
									}
								}
								pCurrent = pCurrent->pNext;
							}
						}

						// also rename bone names within each mesh
						for (int iMesh = 0; iMesh < pImportedObject->iMeshCount; iMesh++)
						{
							sMesh* pMesh = pImportedObject->ppMeshList[iMesh];
							if (pMesh)
							{
								if (pMesh->pBones)
								{
									for (int iBone = 0; iBone < pMesh->dwBoneCount; iBone++)
									{
										if (pMesh->pBones[iBone].szName)
										{
											if (stricmp(pMesh->pBones[iBone].szName, pFrame->szName) == NULL)
											{
												strcpy(pMesh->pBones[iBone].szName, pNewName);
												break;
											}
										}
									}
								}
							}
						}

						// finally rename frame
						strcpy(pFrame->szName, pNewName);
						bConvertedBoneNames = true;
					}
				}
			}
		}
	}
	if (bConvertedBoneNames==true && bBatchConverting==false)
	{
		strcpy(cTriggerMessage, "Unconventional bone names (included spaces, etc), converted to standard MAX rig");
		bTriggerMessage = true;
	}

	// if flagged, use real object col center to shift mesh data to centralize the object
	if ( t.importer.centermodelbyshiftingmesh == 1 )
	{
		sObject* pObject = GetObjectData(t.importer.objectnumber);
		GGVECTOR3 oldObjectCollisionCenter = GGVECTOR3(0,0,0);
		for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
		{
			GGVECTOR3 meshCollisionCenter = GGVECTOR3(0,0,0);
			sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
			if ( pMesh )
			{
				sOffsetMap offsetMap;
				GetFVFOffsetMapFixedForBones(pMesh, &offsetMap);
				for (unsigned int i = 0; i < pMesh->dwVertexCount; ++i)
				{
					float* fX = ((float*)pMesh->pVertexData + offsetMap.dwX + (offsetMap.dwSize * i));
					float* fY = ((float*)pMesh->pVertexData + offsetMap.dwY + (offsetMap.dwSize * i));
					float* fZ = ((float*)pMesh->pVertexData + offsetMap.dwZ + (offsetMap.dwSize * i));
					meshCollisionCenter.x += *fX;
					meshCollisionCenter.y += *fY;
					meshCollisionCenter.z += *fZ;
				}
				meshCollisionCenter /= pMesh->dwVertexCount;
			}
			oldObjectCollisionCenter += meshCollisionCenter;
		}
		oldObjectCollisionCenter /= pObject->iMeshCount;
		// now shift
		for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
		{
			sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
			if ( pMesh )
			{
				sOffsetMap offsetMap;
				GetFVFOffsetMapFixedForBones(pMesh, &offsetMap);
				for (unsigned int i = 0; i < pMesh->dwVertexCount; ++i)
				{
					float* fX = ((float*)pMesh->pVertexData + offsetMap.dwX + (offsetMap.dwSize * i));
					float* fY = ((float*)pMesh->pVertexData + offsetMap.dwY + (offsetMap.dwSize * i));
					float* fZ = ((float*)pMesh->pVertexData + offsetMap.dwZ + (offsetMap.dwSize * i));
					*fX -= oldObjectCollisionCenter.x;
					*fY -= oldObjectCollisionCenter.y;
					*fZ -= oldObjectCollisionCenter.z;
				}
			}
		}
		// trigger the model update in Wicked so can see new arrangement
		WickedCall_RemoveObject(pObject);
		WickedCall_AddObject( pObject );
		WickedCall_UpdateObject ( pObject );
		WickedCall_TextureObject ( pObject, NULL );
	}

	iImporterScale = 100;
	ScaleObject(t.importer.objectnumber, iImporterScale, iImporterScale, iImporterScale);

	sObject* pObject = GetObjectData(t.importer.objectnumber);
	uint64_t materialEntity = 0;

	cstr objectName = t.importer.objectFileOriginalPath + t.importer.objectFilename;

	if (stricmp(objectName.Get() + strlen(objectName.Get()) - 4, ".dbo") == 0 
		|| stricmp(objectName.Get() + strlen(objectName.Get()) - 2, ".x") == 0)
	{
		// Handle cases where dbo or .x files are missing textures.
		if (pObject)
		{
			bool bTexturesAlreadyExist = false;
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pObject->ppMeshList[0]->wickedmeshindex);
			if (mesh)
			{
				materialEntity = mesh->subsets[0].materialID;
				wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				if (pObjectMaterial)
				{
					if (pObjectMaterial->textures[BASECOLORMAP].name.length() > 0)
					{
						bTexturesAlreadyExist = true;
					}
				}
			}

			if (!bTexturesAlreadyExist)
			{
				// No textures - check for fpe.
				cstr fpeFile = t.importer.objectFileOriginalPath + t.importer.objectFilenameFPE;
				char textureName[512] = { 0 };
				bool bFoundFPE = false;
				if (FileExist(fpeFile.Get()))
				{
					bFoundFPE = true;
				}
				else
				{
					// Try a different fpe name. Some classic assets use space instead of underscores and have a space before the number.
					char fpe[MAX_PATH];
					strcpy(fpe, t.importer.objectFilenameFPE.Get());

					// insert space before numbers.
					char numbers[8];
					int spaceIndex = strlen(fpe) - 6;
					strcpy(numbers, fpe + spaceIndex);
					fpe[spaceIndex] = ' ';
					fpe[spaceIndex + 1] = 0;
					strcpy(fpe + strlen(fpe), numbers);

					fpeFile = t.importer.objectFileOriginalPath + cstr(fpe);
					if (FileExist(fpeFile.Get()))
						bFoundFPE = true;
				}

				if (bFoundFPE)
				{
					if (FileOpen(1) == 1) CloseFile(1);

					OpenToRead(1, fpeFile.Get());
					while (FileEnd(1) == 0)
					{
						t.tline_s = ReadString(1);
						t.tcciStat_s = Lower(FirstToken(t.tline_s.Get(), " "));
						if (t.tcciStat_s == "textured" || t.tcciStat_s == "basecolormap0")
						{
							char* value = strstr(t.tline_s.Get(), "=");
							if (value)
							{
								// Found a texture name specified in the fpe.
								if (strncmp(value + 1, " ", 1) == 0)
									strcpy(textureName, value + 2);
								else
									strcpy(textureName, value + 1);
							}

							if(strlen(textureName) > 0)
								break;
						}
					}
					CloseFile(1);
				}

				// No FPE or no texture specified, so use the name of the model as the starting point.
				if (strlen(textureName) == 0)
				{
					strcpy(textureName, t.importer.objectFilenameFPE.Get());
					textureName[strlen(textureName) - 4] = 0;
					strcpy(textureName + strlen(textureName), "_color.dds");
				}

				// Use the basecolour texture name to try and find the rest of the textures.
				if (materialEntity > 0)
				{
					wiScene::MaterialComponent* pMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
					if (pMaterial)
					{
						cstr filename = t.importer.objectFileOriginalPath + cstr(textureName);
						if (FileExist(filename.Get()))
						{
							pMaterial->textures[BASECOLORMAP].name = std::string(filename.Get());
							pMaterial->textures[BASECOLORMAP].resource = WickedCall_LoadImage(pMaterial->textures[BASECOLORMAP].name);

							int legacyDNS = 0;
							if (stricmp(filename.Get() + filename.Len() - strlen("_d.dds"), "_d.dds") == 0)
								legacyDNS = 1;

							char texName[MAX_PATH];
							strcpy(texName, filename.Get());
							if(legacyDNS == 0)
								texName[strlen(texName) - strlen("color.dds")] = 0;
							else
								texName[strlen(texName) - strlen("d.dds")] = 0;

							for (int i = NORMALMAP; i <= EMISSIVEMAP; i++)
							{
								char textureToTry[MAX_PATH];
								strcpy(textureToTry, texName);
								if (i == NORMALMAP)
								{
									if (legacyDNS == 0)
										strcpy(textureToTry + strlen(textureToTry), "normal.dds");
									else
										strcpy(textureToTry + strlen(textureToTry), "n.dds");
								}
								else if (i == SURFACEMAP)
								{
									if (legacyDNS == 0)
									{
										strcpy(textureToTry + strlen(textureToTry), "surface.dds");
									}
									else
									{
										strcpy(textureToTry + strlen(textureToTry), "s.dds");
										if (FileExist(textureToTry))
										{
											char surfacePath[MAX_PATH];
											strcpy(surfacePath, GG_GetWritePath());
											strcat(surfacePath, "imported_models\\");
											for (int i = strlen(texName); i >= 0; i--)
											{
												if (texName[i] == '\\' || texName[i] == '/')
												{
													strcat(surfacePath, texName + i + 1);
													strcat(surfacePath, "surface.dds");
													ImageCreateSurfaceTexture(surfacePath, NULL, textureToTry, textureToTry);
													t.importer.pSurfaceFilesToDelete.push_back(surfacePath);
													textureToTry[0] = 0;
													strcpy(textureToTry, surfacePath);
													break;
												}
											}
										}
									}
								}
								else if (i == EMISSIVEMAP)
								{
									if (legacyDNS == 0)
										strcpy(textureToTry + strlen(textureToTry), "emissive.dds");
								}

								if (FileExist(textureToTry))
								{
									pMaterial->textures[i].name = std::string(textureToTry);
									pMaterial->textures[i].resource = WickedCall_LoadImage(pMaterial->textures[i].name);
								}
							}
						}
					}
				}
			}

			for (int i = 0; i < pObject->iMeshCount; i++)
			{
				wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pObject->ppMeshList[i]->wickedmeshindex);
				if (mesh)
				{
					materialEntity = mesh->subsets[0].materialID;
					wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
					if (pObjectMaterial)
					{
						pObjectMaterial->SetBaseColor(XMFLOAT4(1, 1, 1, 1));
					}
				}
			}
		}
	}

	WickedCall_SetSelectedObject(pObject);

	// first populate WEMaterials for all meshes
	Wicked_Copy_Material_To_Grideleprof((void*)pObject, 0);

	// then select the first one to start off the importer
	Wicked_Set_Material_Defaults( (void*) pObject, 0);

	//oldtextimportpath = t.importer.objectFileOriginalPath;

	ShowObject(t.importer.objectnumber);

	if (bIsBetaFormat)
	{
		strcpy(cTriggerMessage, "The selected format is in dev mode.");
		bTriggerMessage = true;
	}

	// initialise animation sets for this model
	int iUseDefaultNonCombatAnimations = 0;
	animsystem_prepareobjectforanimtool(t.importer.objectnumber, iUseDefaultNonCombatAnimations);

	// reset rotation values
	fImportPosX = 0.0;
	fImportPosY = 0.0;
	fImportPosZ = 0.0;
	fImportRotX = 0.0;
	fImportRotY = 0.0;
	fImportRotZ = 0.0;

	// set some importer defaults
	strcpy(collision_combo_entry, "Box\0");
	t.importer.collisionshape = 0;
	t.importer.defaultstatic = 1;

	importer_load_scenery();

	// Start importer loop
	t.importer.loaded = 1;
}
void importer_loadmodel_wicked(int objnumber)
{
	//PE: Need batch support here.
	importer_loadmodel_wicked();
}

void importer_loadmodel(int objnumber)
{
	//PE: We need a object array so we can batch convert.

	// user prompt when loading model in
	popup_text("importing and converting model");

	// show loading prompt (FBX conversion can take a while)
	importer_loadmodel_wicked(objnumber);
}

void importer_loadmodel ( void )
{
	// user prompt when loading model in
	popup_text("importing and converting model");

	// show loading prompt (FBX conversion can take a while)
	// Wicked imports the object in-situ (so placed in center of current location in level scene)
	// Note: Will probably find floor if close enough, or in air if high above ground.
	// Using new load sequence as Wicked import is quite different from Classic/VRQ

	bool bRestoreData = t.importer.bQuitForReload;
	importer_loadmodel_wicked();

	if (bRestoreData)
		importer_restoreobjectdata();
	else
		importer_clearobjectdata();

	visualsdatastoragetype desiredVisuals;
	set_temp_visuals(t.editorvisuals, t.visualsStorage, desiredVisuals);
	set_temp_visuals(t.visuals, t.visualsStorage, desiredVisuals);
	
	//set_temp_visuals(t.gamevisuals, t.visualsStorage, desiredVisuals);
	visuals_loop();

	// seems to be needed for VRQ too (for scaling)
	t.importer.camerazoom = 1.0f;

	// remove editor prompt
	popup_text_close();

	if (bRestoreData)
	{
		// when restoring custom settings, do NOT reset scale!
		if (ObjectExist(t.importer.objectnumber) == 1)
		{
			ScaleObject(t.importer.objectnumber, iImporterScale * fImporterScaleMultiply, iImporterScale * fImporterScaleMultiply, iImporterScale * fImporterScaleMultiply);
		}
	}
	else
	{
		iImporterScale = 100; //Default scale.
	}

	t.slidersmenuvalue[t.importer.properties1Index][1].value = iImporterScale;
	iImporterGenerateThumb = 0;
}

void importer_load_scenery()
{
	float fDefaultCharacterHeight = 65.0f;
	float fMinimumScenerySize = 750.0f;

	// The coordinate that the following objects are positioned relative to.
	t.importer.fSceneLocationY = 100000.f;
	int iImporterOrigin[3] = { 0.0f, t.importer.fSceneLocationY, 0.0f };

	sObject* pObject = GetObjectData(t.importer.objectnumber);

	// The size of the imported object (multiply by 4, to account for error in the collision radius).
	float fSize = pObject->collision.fRadius * 4.f;
	float xmax = pObject->collision.vecMax.x;
	sCollisionData& rCollision = pObject->collision;
	

	// Calculate the necessary size of the scenery to fit the imported model.
	t.importer.fSceneryDiameter += (fSize - t.importer.fSceneryDiameter);

	if (fSize < fMinimumScenerySize)
	{
		// Set the minimum size of the scenery.
		t.importer.fSceneryDiameter = fMinimumScenerySize;
	}

	// Position the model so that it is inside the importer scenery.
	PositionObject(t.importer.objectnumber, iImporterOrigin[0], iImporterOrigin[1], iImporterOrigin[2]);

	// Create the sphere.
	if (ObjectExist(g.importerextraobjectoffset) == 0)
	{
		MakeObjectSphere(g.importerextraobjectoffset, t.importer.fSceneryDiameter, 30, 30);
		PositionObject(g.importerextraobjectoffset, iImporterOrigin[0], iImporterOrigin[1], iImporterOrigin[2]);

		// Disable backface culling so we can see inside the sphere.
		SetObjectCull(g.importerextraobjectoffset, 0);

		// Texture the sphere (need emissive material so that outside light does not affect the inside of the sphere).
		TextureObject(g.importerextraobjectoffset, g.importerextraimageoffset);
		sObject* pObject = GetObjectData(g.importerextraobjectoffset);
		WickedCall_TextureObjectAsEmissive(pObject);

		// Disabling cast shadows for the sphere to avoid strange lighting artefacts.
		WickedCall_SetObjectCastShadows(pObject, false);
	}

	// Create the plane that intersects the sphere.
	if (ObjectExist(g.importerextraobjectoffset + 1) == 0)
	{
		// If the importer sphere is not being displayed, make the plane larger.
		float fScaleFactor = 1.0f;
		if (pref.iImporterDome == 0) fScaleFactor = 3.0f;
		MakeObjectPlane(g.importerextraobjectoffset + 1, t.importer.fSceneryDiameter * fScaleFactor, t.importer.fSceneryDiameter * fScaleFactor);
		RotateObject(g.importerextraobjectoffset + 1, 90.f, 0.f, 0.f);
		PositionObject(g.importerextraobjectoffset + 1, iImporterOrigin[0], iImporterOrigin[1], iImporterOrigin[2]);
		TextureObject(g.importerextraobjectoffset + 1, g.importerextraimageoffset + 1);
		sObject* pObject = GetObjectData(g.importerextraobjectoffset + 1);
		WickedCall_SetObjectLightToUnlit(pObject, wiScene::MaterialComponent::SHADERTYPE::SHADERTYPE_UNLIT);
	}

	// Create the plane that shows the relative scale of the imported model.
	if (ObjectExist(g.importerextraobjectoffset + 2) == 0)
	{
		MakeObjectPlane(g.importerextraobjectoffset + 2, 25, fDefaultCharacterHeight);

		// Offset the decal to the left or right of the model, based on how big the model is.
		float fDecalOffset = iImporterOrigin[0] - fabs(rCollision.vecMin.x);
		float fMaxDecalOffset = t.importer.fSceneryDiameter / 8.0f;
		if (fabs(fDecalOffset) > fMaxDecalOffset)
		{
			if (fDecalOffset < 0.0f)
				fDecalOffset = -fMaxDecalOffset;
			if (fDecalOffset > 0.0f)
				fDecalOffset = fMaxDecalOffset;
		}
		// Move the reference plane to the left of the object (additional 25.f is width of the plane).
		PositionObject(g.importerextraobjectoffset + 2, fDecalOffset - 25.f, iImporterOrigin[1] + (fDefaultCharacterHeight / 2.0f), iImporterOrigin[2]);

		// Texture the decal and force transparency and emissive.
		TextureObject(g.importerextraobjectoffset + 2, g.importerextraimageoffset + 2);
		sObject* pObject = GetObjectData(g.importerextraobjectoffset + 2);
		WickedCall_TextureObjectAsEmissive(pObject);
		WickedCall_SetObjectTransparentDirect(pObject, true);
		WickedCall_SetObjectCastShadows(pObject, false);
		// Need to retain alpha since TextureObjectAsEmissive() sets it to 0.
		WickedCall_SetObjectAlpha(pObject, 100.0f);
	}

	// Position the camera at the back of the dome, looking at the imported model and character decal.
	PositionCamera(iImporterOrigin[0], iImporterOrigin[1] + 75, iImporterOrigin[2] - (t.importer.fSceneryDiameter / 2.75f));

	// Make the camera look at the midpoint height between the decal and model.
	float fDecalLookPoint = iImporterOrigin[1] + (fDefaultCharacterHeight / 2.0f);
	float fModelLookPoint = iImporterOrigin[1] + (fSize / 4.0f);
	PointCamera(iImporterOrigin[0], (fDecalLookPoint + fModelLookPoint) / 2.0f, iImporterOrigin[2]);

	// Set the editor free flight camera orientation so that the new camera orientation is not overridden.
	t.editorfreeflight.c.x_f = CameraPositionX();
	t.editorfreeflight.c.y_f = CameraPositionY();
	t.editorfreeflight.c.z_f = CameraPositionZ();
	t.editorfreeflight.c.angx_f = CameraAngleX();
	t.editorfreeflight.c.angy_f = CameraAngleY();
	// Pressing F caused the camera to not be moved. So force it into free flight mode.
	t.editorfreeflight.mode = 1;


	if (!pref.iImporterDome)
	{
		HideObject(g.importerextraobjectoffset);
	}
}

void importer_RestoreCollisionShiftHeight ( void )
{
	float fShiftForCollisionObjects = t.importer.originalcameraheight - t.importer.lastcameraheightforshift;
	t.importer.lastcameraheightforshift = t.importer.originalcameraheight;
	for ( int tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
		if (  t.importerCollision[tCount].object > 0 ) 
		{
			if (  ObjectExist(t.importerCollision[tCount].object)  ==  1 ) 
			{
				float fX = ObjectPositionX(t.importerCollision[tCount].object);
				float fY = ObjectPositionY(t.importerCollision[tCount].object) - fShiftForCollisionObjects;
				float fZ = ObjectPositionZ(t.importerCollision[tCount].object);
				PositionObject ( t.importerCollision[tCount].object, fX, fY, fZ );
				PositionObject ( t.importerCollision[tCount].object2, fX, fY, fZ );
			}
		}
	}
}

void animsystem_clearoldanimationfromobject ( sObject* pObject )
{
	// stop any animation playing
	WickedCall_StopObject(pObject);

	// remove any animcomponents
	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	if (pAnimSet)
	{
		// only first animset is used by wicked engine
		if (pAnimSet->wickedanimentityindex > 0)
		{
			wiScene::AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent( pAnimSet->wickedanimentityindex );
			if (animationcomponent)
			{
				for (int i = 0; i < animationcomponent->samplers.size(); i++)
				{
					wiScene::GetScene().Entity_Remove(animationcomponent->samplers[i].data);
				}
			}
			wiScene::GetScene().Entity_Remove(pAnimSet->wickedanimentityindex);
			pAnimSet->wickedanimentityindex = 0;
		}
	}

	// erase all animation sets in object
	ClearAnimationFromObject(pObject);
	pObject->fAnimTotalFrames = 0;
}

void animsystem_processallmannequins (void)
{
	// establish location of dsadsa model
	char pMannequinPath[MAX_PATH];
	strcpy(pMannequinPath, g.fpscrootdir_s.Get());
	strcat(pMannequinPath, "\\Files\\charactercreatorplus\\");

	// scan all folders in "charactercreatorplus\animations"
	cstr pOldDir = GetDir();
	SetDir (g.fpscrootdir_s.Get());
	UnDim (t.filelist_s);
	buildfilelist("Files\\charactercreatorplus\\animations", "");
	if (ArrayCount(t.filelist_s) > 0)
	{
		timestampactivity (0, "Processing all 'charactercreatorplus animations'");
		for (t.chkfile = 0; t.chkfile <= ArrayCount(t.filelist_s); t.chkfile++)
		{
			t.file_s = t.filelist_s[t.chkfile];
			timestampactivity (0, t.file_s.Get());
			if (t.file_s != "." && t.file_s != "..")
			{
				// ignore the "noncharacter" folder 
				if (strnicmp(t.file_s.Get(), "noncharacter", strlen("noncharacter")) != NULL )
				{
					if (cstr(Lower(Right(t.file_s.Get(), 4))) == ".dbo")
					{
						// start with no object before the load
						if (ObjectExist(g.tempobjectoffset) == 1) DeleteObject(g.tempobjectoffset);

						// for each DBO found, load in mannequin.dbo
						cstr pThisDir = GetDir();
						SetDir (pMannequinPath);
						LoadObject("mannequin.dbo", g.tempobjectoffset);
						SetDir (pThisDir.Get());

						// append the found DBO animation file
						char pSrcFilePath[MAX_PATH];
						strcpy(pSrcFilePath, g.fpscrootdir_s.Get());
						strcat(pSrcFilePath, "\\Files\\charactercreatorplus\\animations\\");
						strcat(pSrcFilePath, t.file_s.Get());
						AppendObject(pSrcFilePath, g.tempobjectoffset, 0);

						// save new DBO with replaced model
						char pDestFilePath[MAX_PATH];
						strcpy(pDestFilePath, pSrcFilePath);
						GG_GetRealPath(pDestFilePath, 1);
						if (FileExist(pDestFilePath) == 1) DeleteFileA(pDestFilePath);
						SaveObject(pDestFilePath, g.tempobjectoffset);

						// get name only
						char pNameOnly[MAX_PATH];
						strcpy(pNameOnly, pDestFilePath);
						if (strlen(pNameOnly) > 4)
						{
							for (int n = strlen(pDestFilePath) - 4; n > 0; n--)
							{
								if (pDestFilePath[n] == '\\' || pDestFilePath[n] == '/')
								{
									strcpy(pNameOnly, pDestFilePath+n+1);
									break;
								}
							}
							pNameOnly[strlen(pNameOnly) - 4] = 0;
						}

						// also save an FPE which will allow display in the library view
						char pFPEFile[MAX_PATH];
						strcpy(pFPEFile, pDestFilePath);
						pFPEFile[strlen(pFPEFile) - 4] = 0;
						strcat(pFPEFile, ".fpe");
						if (FileExist(pFPEFile) == 1) DeleteFileA(pFPEFile);
						OpenToWrite(1, pFPEFile);
						WriteString(1, "; header");
						char pLine[MAX_PATH];
						sprintf(pLine, "desc = %s", pNameOnly);
						WriteString(1, pLine);
						WriteString(1, "; scripts");
						WriteString(1, "aimain = people\\play_animation.lua");
						WriteString(1, "; geometry");
						sprintf(pLine, "model = %s.dbo", pNameOnly);
						WriteString(1, pLine);
						WriteString(1, "; visuals");
						WriteString(1, "texturepath = charactercreatorplus\\");
						WriteString(1, "textured = mannequin_color.dds");
						WriteString(1, "; anim set");
						WriteString(1, "animspeed = 100");
						WriteString(1, "animmax = 1");
						WriteString(1, "playanimineditor = All");
						WriteString(1, "; thumbnail");
						WriteString(1, "thumbnailbackdrop = Stage 1.dds");
						WriteString(1, "thumbnailzoom = 14.214919");
						WriteString(1, "thumbnailcamleft = 0.000000");
						WriteString(1, "thumbnailcamup = 0.000000");
						WriteString(1, "thumbnailrotatex = 0.000000");
						WriteString(1, "thumbnailrotatey = 0.000000");
						WriteString(1, "thumbnailanimset = 0");
						CloseFile(1);
					}
				}
			}
		}
	}

	// clear resources
	if (ObjectExist(g.tempobjectoffset) == 1) DeleteObject(g.tempobjectoffset);

	// restore path
	SetDir (pOldDir.Get());
}

