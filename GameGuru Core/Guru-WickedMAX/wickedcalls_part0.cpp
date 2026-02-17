//
// Wicked Calls - place Wicked commands here so can compile (cannot call from old graphics engine modules, conflicts with its own data types)
//
// 
// Includes
#include "stdafx.h"
#include "wickedcalls.h"
#include "GGAnimBridge.h"
#include <vector>
#include <unordered_map>

#include "..\Dark Basic Pro SDK\Shared\Objects\CObjectManagerWicked.h"
#include "M-Entity.h"
#include "CFileC.h"
#include "CImageC.h"
#include "CCameraC.h"

#include "master.h"
extern Master master;

#include "GGTerrain/GGTerrain.h"
#include "gameguru.h"

// OPTICK Performance
#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

#define ONLY_USE_OUTLINE_HIGHLIGHT
#define MATCHCLASSICROTATION

enum EDITORSTENCILREF
{
	EDITORSTENCILREF_CLEAR = 0x00,
	EDITORSTENCILREF_HIGHLIGHT_OBJECT = 0x01,
	EDITORSTENCILREF_HIGHLIGHT_MATERIAL = 0x02,
	EDITORSTENCILREF_HIGHLIGHT_OBJECT_RED = 0x03,
	EDITORSTENCILREF_HIGHLIGHT_OBJECT_BLUE = 0x04,
	EDITORSTENCILREF_LAST = 0x0F,
};

// Prototypes
void CameraAnglesFromMatrix(GGMATRIX* pmatMatrix, GGVECTOR3* pVecAngles);
void WickedCall_DrawObjctBox(sObject* pObject, XMFLOAT4 color, bool bThickLine = false, bool ForceBox = false);
bool bUseEditorOutlineSelection(void);
DARKSDK_DLL int ObjectExist(int iID);
void* GetObjectsInternalData(int iID);
int iGetgrideditselect(void);

// Name spaces
using namespace std;
using namespace wiGraphics;
using namespace wiScene;
using namespace wiECS;

// Object loading structure
struct WickedLoaderState
{
	Scene* scene;
	uint64_t storeMasterRootEntityIndex;
	std::unordered_map<int, Entity> entityMap;  // node/frame -> entity
	std::unordered_map<int, Entity> entityMeshMap;  // node/frame -> entity
};

// Globals
std::string g_pWickedTexturePath;
int g_iWickedLayerMaskPreference = GGRENDERLAYERS_NORMAL;
int g_iWickedLayerMaskOptionalLimb = -1;
float g_iWickedUScalePreference = 1.0f;
float g_iWickedVScalePreference = 1.0f;
bool g_bWickedCreateOnlyWhenUsed = false;
bool g_bWickedIgnoreTextureInfo = false;
bool g_bWickedUseImagePtrInsteadOfTexFile = false;
bool g_bDisplayWarnings = true;
int g_iWickedPutInEmissiveMode = 0;
XMFLOAT4 g_lastMousePos = { 0,0,0,0 };
uint64_t g_hovered_entity = 0;
uint64_t g_selected_entity = 0;
sObject* g_selected_pobject = NULL;
sObject* g_highlight_pobject = NULL;
sObject* g_hovered_pobject = NULL;
sObject* g_ray_pobject = NULL;

uint64_t g_hovered_dot_entity = 0;
sObject* g_hovered_dot_pobject = NULL;
bool g_bhovered_dot = false;
float fLastHitPosition[4] = { 0,0,0,0 };

int g_selected_editor_objectID = 0;
sObject* g_selected_editor_object = NULL;
XMFLOAT4 g_selected_editor_color = XMFLOAT4(0.25f, 1.0f, 0.25f, 0.5f);

std::vector<int> g_ObjectHighlightList;

// stores original resolution of editor when enter VR, as need to restore it after VR
int g_iStoreRenderResolutionWidth = -1;
int g_iStoreRenderResolutionHeight = -1;
float fLODMultiplier = 1.0f;


bool g_bLightShaftState = true;
bool g_bLensFlareState = true;
void SetLightShaftState(bool bState);
bool GetLightShaftState(void);
bool GetLensFlareState();
void SetLensFlareState(bool bState);

extern CObjectManager m_ObjectManager;
extern LPGGIMMEDIATECONTEXT m_pImmediateContext;
extern wiSprite* pboundbox[4];
extern Entity g_entityCameraLight;
extern Entity g_entityThumbLight;
extern Entity g_entityThumbLight2;
extern Entity g_entitySunLight;
extern Entity g_weatherEntityID;
extern bool bImGuiGotFocus;
extern bool bProceduralLevel;
extern bool bImGuiInTestGame;
bool bRenderTargetHasFocus = false;

float fWickedMaxCenterTest = 0.0f;
uint32_t iCulledPointShadows = 0;
uint32_t iCulledSpotShadows = 0;
uint32_t iCulledAnimations = 0;
uint32_t iRenderedPointShadows = 0;
uint32_t iRenderedSpotShadows = 0;

bool bEnable30FpsAnimations = false;
bool bEnableTerrainChunkCulling = true;
bool bEnablePointShadowCulling = true;
bool bEnableDelayPointShadow = false;
float pointShadowScaler = 1.0f;
bool bEnableSpotShadowCulling = true;
bool bEnableObjectCulling = true;
bool bEnableAnimationCulling = true;
bool bShadowsInFrontTakesPriority = false;

bool bShadowsLowestLOD = false;
bool bProbesLowestLOD = false;
bool bRaycastLowestLOD = false;
bool bPhysicsLowestLOD = false;
bool bThreadedPhysics = false;
bool bHideWeapons = false;
bool bHideWeaponsMuzzle = false;
bool bHideWeaponsSmoke = false;
bool bReflectionsLowestLOD = false;
int iTracerPosition = 0;
int iEnterGodMode = 0;
bool bTmpTesting = false;

float fWickedCallShadowFarPlane = DEFAULT_FAR_PLANE;


// Image Management
std::vector<sImageList> g_imageList;
std::string g_rootFolder;

void WickedCall_InitImageManagement(LPSTR pRootFolder)
{
	// clear image list
	g_imageList.clear();
	g_rootFolder = pRootFolder;
}

void WickedCall_FreeImage_By_MasterID(uint32_t masterid)
{
	for (int i = 0; i < g_imageList.size(); i++)
	{
		sImageList* pImage = &g_imageList[i];
		if (pImage->MasterObject == masterid)
		{
			WickedCall_FreeImage(pImage);
		}
	}
}

void WickedCall_FreeImage(sImageList* pImage)
{
	if ( pImage )
	{
		if (pImage->pName)
		{
			// first release wicked resource (turns out it maintains a list referenced by name)
			char pRealNameToDeleteFromWickedResource[MAX_PATH];
			strcpy(pRealNameToDeleteFromWickedResource, pImage->pName);
			GG_GetRealPath(pRealNameToDeleteFromWickedResource, 0);
			//wiResourceManager::FreeResource(pRealNameToDeleteFromWickedResource); // REMOVED - FreeResource no longer exists

			// and then the string holding the name
			delete pImage->pName;
			pImage->pName = NULL;
		}
		if (pImage->image.IsValid())
		{
			// free wicked resource
			//wiResourceManager::Clear() <-- clears everything!!
			//pImage->image.swap(); what frees all resources created with the wiResourceManager::Load call?
			pImage->image = {};
			pImage->MasterObject = 0;
		}
	}
}

void WickedCall_FreeAllImagesOfType(eImageResType eType)
{
	// clear all image data from list
	for (int i = 0; i < g_imageList.size(); i++)
	{
		sImageList* pImage = &g_imageList[i];
		if ( pImage->eType == eType )
		{
			WickedCall_FreeImage(pImage);
		}
	}
}

void WickedCall_GetRelativeAfterRoot(std::string pFilename, LPSTR pFullRelativeLocationFilename)
{
	// account for current directory location (as foldera\lee.png is different than folderb\lee.png)
	char pCurrentDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, pCurrentDir);
	int iChopRootFolder = strlen(g_rootFolder.c_str()) + strlen("Files\\") + 1;
	char pRelativePathAfterRoot[MAX_PATH];
	strcpy(pRelativePathAfterRoot, "");
	int iCurrentDirLength = strlen(pCurrentDir);
	if (iChopRootFolder < iCurrentDirLength)
	{
		// create new filename reference that includes the full relative location
		strcpy(pRelativePathAfterRoot, pCurrentDir + iChopRootFolder);
		strcpy(pFullRelativeLocationFilename, pRelativePathAfterRoot);
		strcat(pFullRelativeLocationFilename, "\\");
		strcat(pFullRelativeLocationFilename, pFilename.c_str());
	}
	else
	{
		// create new filename, but no additional prefix from root to the path used here
		strcpy(pFullRelativeLocationFilename, pFilename.c_str());
	}
}

int WickedCall_FindImageIndexInList(std::string pFilenameToFind, LPSTR pFullRelativeLocationFilename)
{
	// resulting image index
	int iImageIndex = -1;

	// see if this image has already been loaded
	if (pFilenameToFind.length() > 0)
	{
		char pReturnedRelativeLocationFilename[MAX_PATH];
		WickedCall_GetRelativeAfterRoot(pFilenameToFind, pReturnedRelativeLocationFilename);
		if (pFullRelativeLocationFilename) strcpy(pFullRelativeLocationFilename, pReturnedRelativeLocationFilename);
		for (int i = 0; i < g_imageList.size(); i++)
		{
			LPSTR pThisImageFilename = g_imageList[i].pName;
			if (pThisImageFilename)
			{
				if (stricmp(pThisImageFilename, pReturnedRelativeLocationFilename) == NULL)
				{
					iImageIndex = i;
					break;
				}
			}
		}
	}

	// return result
	return iImageIndex;
}
uint32_t SetMasterObject = 0;
void WickedCall_AddImageToList(wiResource image, eImageResType eType, std::string pFilenameRef, int iKbused)
{
	sImageList newImage;
	newImage.image = image;
	newImage.eType = eType;
	newImage.iMemUsedKB = iKbused;
	newImage.pName = new char[strlen(pFilenameRef.c_str()) + 1];
	strcpy(newImage.pName, pFilenameRef.c_str());

	//PE: Mark masterobject this image belong to.
	// > 50000 < 70000
	if(SetMasterObject > 50000 && SetMasterObject < 70000)
		newImage.MasterObject = SetMasterObject;
	else
		newImage.MasterObject = 0;

	// add loaded image to existing list slot, or add a new one
	int i = 0;
	for (; i < g_imageList.size(); i++)
	{
		if (g_imageList[i].pName == NULL)
		{
			g_imageList[i] = newImage;
			break;
		}
	}
	if (i == g_imageList.size())
		g_imageList.push_back(newImage);
}

int total_mem_from_load = 0;
bool bCalledFromWickedLoadImage = false;
wiResource WickedCall_LoadImage(std::string pFilenameToLoadIN, eImageResType eType)
{
	//PE: Prevent dublicate textures even if using different names.
	//PE: Scan all our images and make a text file including filename+CRC64 of the file.
	//PE: Load this text file into a vector.
	//PE: Find filname in vector and add the CRC64 to g_imageList when adding a image.
	//PE: Below also lookup filename CRC64 and check against the g_imageList CRC64 to also reused image.
	const int iLen = pFilenameToLoadIN.length();
	if (iLen <= 4)
		return {}; //PE: We get this alot from DISPLACEMENTMAP ...

	// when gdividetexturesize is 0, we are not using textures, so use a dummy texture (tests performance against using too LARGE a texture set)
	std::string pFilenameToLoad = pFilenameToLoadIN;
	if (g.gdividetexturesize == 0)
	{
		pFilenameToLoad = "editors\\gfx\\notexture.dds";
	}

	wiResource image;
	char pFullRelativeLocationFilename[MAX_PATH];
	int iImageIndex = WickedCall_FindImageIndexInList(pFilenameToLoad,pFullRelativeLocationFilename);
	if (iImageIndex != -1)
	{
		// found image
		image = g_imageList[iImageIndex].image;
	}
	else
	{
		// quickly reject nonesense filenames
		char pRealFilenameToLoad[MAX_PATH];
		strcpy(pRealFilenameToLoad, pFilenameToLoad.c_str());
		if ( pRealFilenameToLoad[strlen(pRealFilenameToLoad)-1] == '\\'
		|| strnicmp ( pRealFilenameToLoad + strlen(pRealFilenameToLoad) - 5, "\\.dds", 5 ) == NULL 
		|| strnicmp ( pRealFilenameToLoad + strlen(pRealFilenameToLoad) - 5, "\\.png", 5 ) == NULL )
		{
			// is not a filename that makes sense, reject load
			return {};
		}

		//PE: Ignore all $NoName$_Color.png, alot of old dbo have this.
		if (iLen >= 8 && pFilenameToLoad[0] == '$' && pFilenameToLoad[7] == '$')
		{
			return {};
		}
		if (iLen > 18 && pFilenameToLoad[iLen-11] == '$' && pFilenameToLoad[iLen - 18] == '$')
		{
			return {};
		}

		bool bFound = false;
		if (t.game.gameisexe == 1)
		{
			//PE: All relative path like entitybank\tmp.dds 
			//PE: Will result in GG_GetRealPath to change to docwrite in standalone, as we only have _e_ version in standalone.
			//PE: So _e_ version in standalone is never used, prefer _e_ over docwrite.
			std::string fullName = pRealFilenameToLoad;
			std::string fileName = wiHelper::GetFileNameFromPath(fullName);
			std::string dirName = wiHelper::GetDirectoryFromPath(fullName);
			std::string codedName = dirName + "_e_" + fileName;
			char CheckFile[MAX_PATH];
			strcpy(CheckFile, codedName.c_str());
			GG_GetRealPath(CheckFile, 0);
			HANDLE hfile = GG_CreateFile(CheckFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hfile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hfile);
				//PE: Prefer this _e_ version.
				strcpy(pRealFilenameToLoad, CheckFile);
				bFound = true;
			}
			else
			{
				//PE: Also prefer .dds over .png here.
				const char last = CheckFile[strlen(CheckFile) - 3];
				if (last == 'p' || last == 'P') //PE: Quick png check.
				{
					CheckFile[strlen(CheckFile) - 1] = 's';
					CheckFile[strlen(CheckFile) - 2] = 'd';
					CheckFile[strlen(CheckFile) - 3] = 'd';
					HANDLE hfile = GG_CreateFile(CheckFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hfile != INVALID_HANDLE_VALUE)
					{
						CloseHandle(hfile);
						//PE: Prefer this _e_ version.
						strcpy(pRealFilenameToLoad, CheckFile);
						bFound = true;
					}
				}

			}
		}
		
		// first get real filename
		if(!bFound)
			GG_GetRealPath(pRealFilenameToLoad, 0);
		pFilenameToLoad = pRealFilenameToLoad;

		bool bDecrypted = false;
		char VirtualFilename[_MAX_PATH];
		strcpy(VirtualFilename, pFilenameToLoad.c_str());
		extern bool CheckForWorkshopFile(LPSTR);
		if(t.importer.importerActive == 0) //PE: Need the real texture name in importer not prefer dds (fbx can include a .png but is a dds).
			bCalledFromWickedLoadImage = true;
		CheckForWorkshopFile (VirtualFilename);
		if ( pFilenameToLoad[ pFilenameToLoad.length() - 1] != VirtualFilename[strlen(VirtualFilename) - 1])
		{
			//PE: We changed from png to prefer .dds, change extension.
			pFilenameToLoad[pFilenameToLoad.length() - 1] = VirtualFilename[strlen(VirtualFilename) - 1];
			pFilenameToLoad[pFilenameToLoad.length() - 2] = VirtualFilename[strlen(VirtualFilename) - 2];
			pFilenameToLoad[pFilenameToLoad.length() - 3] = VirtualFilename[strlen(VirtualFilename) - 3];
		}
		bCalledFromWickedLoadImage = false;
		g_pGlob->Decrypt(VirtualFilename);
		bDecrypted = true;

		//PE: We are calling this very often trying to locate textures, no need to bother wicked if not exists.
		bool bFileExists = false;
		HANDLE hfile = GG_CreateFile(VirtualFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hfile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hfile);
			bFileExists = true;
		}
		if( bFileExists )
		{
			// if not, load it
			//wiResourceManager::SetErrorCode(0); // REMOVED
			DARKSDK int SMEMAvailable(int iMode);
			int startmem = SMEMAvailable(1);

			// handle encrypted image files when loading
			std::vector<uint8_t> data;
			if (wiHelper::FileRead(VirtualFilename, data))
			{
				wiResourceManager::Flags flag = wiResourceManager::Flags::IMPORT_NORMALMAP;
				image = wiResourceManager::Load(pFilenameToLoad, flag, data.data(), data.size());
				data.clear();
			}
			if (image.IsValid())
			{
				// add image list item
				int endmem = SMEMAvailable(1);
				total_mem_from_load += (endmem - startmem);
				WickedCall_AddImageToList(image, eType, pFullRelativeLocationFilename, (endmem - startmem));
			}
			else
			{
				// image failed to load - no need to add to list
				// wiResourceManager::GetErrorCode() removed
			}
		}

		// and re-encrypt before proceeding
		if (bDecrypted == true)
		{
			g_pGlob->Encrypt(VirtualFilename);
		}
	}

	// return new or existing image resource
	return image;
}

wiResource WickedCall_LoadImage(std::string pFilenameToLoad)
{
	return WickedCall_LoadImage(pFilenameToLoad, IMAGERES_LEVEL);
}

void WickedCall_DeleteImage(std::string pFilenameToDelete)
{
	int iImageIndex = WickedCall_FindImageIndexInList(pFilenameToDelete,NULL);
	if (iImageIndex != -1)
	{
		WickedCall_FreeImage(&g_imageList[iImageIndex]);
	}
}


// Functions
bool bNoHierarchySorting = false;
bool bUseInstancing = false;
int iUseMasterObjectID = 0;
int iCurrentObjectID = 0;
bool bNextObjectMustBeClone = false;
void WickedCall_LoadNode(sFrame* pFrame, Entity parent, Entity root, WickedLoaderState& state)
{
	// only if have a frame
	if (pFrame==NULL )
		return;

	// setup vars
	Scene& scene = *state.scene;
	Entity entity = INVALID_ENTITY;

	// the DBO mesh needs to have geometry, or not point creating object (just the entity further down)
	bool bUseFrameMatrix = true;
	sMesh* pDBOMesh = pFrame->pMesh;

	//PE: Make sure not to add more objects then needed.
	if (pDBOMesh)
	{
		pDBOMesh->pFrameAttachedTo = pFrame;
		if (pFrame->bIgnoreMesh)
		{
			pFrame->wickedobjindex = 0;
			// recurse through all frames
			if (pFrame->pChild) WickedCall_LoadNode(pFrame->pChild, entity, root, state);
			if (pFrame->pSibling) WickedCall_LoadNode(pFrame->pSibling, parent, root, state);
			return;
		}
	}

	if (pDBOMesh && pDBOMesh->dwVertexCount > 0 && pDBOMesh->iPrimitiveType == GGPT_TRIANGLELIST ) // pDBOMesh->dwIndexCount > 0 ) nned to support indexless models
	{
		// Create object to hold mesh
		entity = scene.Entity_CreateObject(pFrame->szName);
		ObjectComponent& object = *scene.objects.GetComponent(entity);

		// store object entity ID reference in frame (for later use by texture function amongst others)
		pFrame->wickedobjindex = entity;
		state.entityMeshMap[pFrame->iID] = entity;
		// assign layer to object
		wiScene::LayerComponent& layer = *scene.layers.GetComponent(entity);

		// leelee, find out why I cannot set this after object added to scene!
		if ( g_iWickedLayerMaskOptionalLimb == -1 || (g_iWickedLayerMaskOptionalLimb == pFrame->iID ) )
			layer.layerMask = g_iWickedLayerMaskPreference;
		
		// create mesh
		wiECS::Entity meshEntity;
		//PE: InstanceObject
		if (bUseInstancing && pDBOMesh->master_wickedmeshindex > 0 )
		{
			meshEntity = pDBOMesh->master_wickedmeshindex;
			pDBOMesh->bInstanced = true;
		}
		else
		{
			meshEntity = scene.Entity_CreateMesh("node_mesh");
			pDBOMesh->bInstanced = false;
		}
		wiScene::MeshComponent& mesh = *scene.meshes.GetComponent(meshEntity);
		pDBOMesh->wickedmeshindex = meshEntity;
		pDBOMesh->pFrameAttachedTo = pFrame;

		// associate mesh with object
		object.meshID = meshEntity;

		//#### PE: mesh setup start ####
		//PE: InstanceObject
		if (!(bUseInstancing && pDBOMesh->master_wickedmeshindex > 0))
		{
			// if model has no indices, assume triangles in list order
			if (pDBOMesh->dwIndexCount == 0)
			{
				// simple triangle list
				for (size_t i = 0; i < pDBOMesh->dwVertexCount; i += 3)
				{
					mesh.indices.push_back(i + 0);
					mesh.indices.push_back(i + 2);
					mesh.indices.push_back(i + 1);
				}
			}
			else
			{
				// go through all indices and copy to created mesh
				for (size_t i = 0; i < pDBOMesh->dwIndexCount; i += 3)
				{
					mesh.indices.push_back(pDBOMesh->pIndices[i + 0]);
					mesh.indices.push_back(pDBOMesh->pIndices[i + 2]);
					mesh.indices.push_back(pDBOMesh->pIndices[i + 1]);
				}
			}

			//TODO:
			//PE: IF (pDBOMesh->dwBoneCount > 0)
			//PE: And pMesh->dwFVF!=0 (dwFVF already set in dbo).
			//PE: Then we must support dwTU[3] and dwTU[4] 
			//PE: as wicked SkinVertex requere vertex_boneindices,vertex_boneweights to match vertex_positions .size.
			//PE: If not we get a crash in wicked "Pick" function.

			// if loaded as a DBO, the vertex format may not include weights and indices for the bone animation,
			// so apply those now to ensure all bone based meshes can be animated by wicked engine
			sOffsetMap offsetMap;
			GetFVFOffsetMapFixedForBones(pDBOMesh, &offsetMap);
			if (pDBOMesh->dwBoneCount > 0)
			{
				GetFVFOffsetMapFixedForBones(pDBOMesh, &offsetMap);
				if (offsetMap.dwTU[3] == 0 || offsetMap.dwTU[4] == 0)
				{
					// generate a new mesh that only includes pos, normal, textureUV0, bone indices and weights
					bool bGenerateNormals = true, bUsesTangents = false, bUsesBinormals = false, bUsesDiffuse = false, bUsesBoneData = true, bDoNotGenerateExtraData = false;
					GenerateExtraDataForMeshEx(pDBOMesh, bGenerateNormals, bUsesTangents, bUsesBinormals, bUsesDiffuse, bUsesBoneData, bDoNotGenerateExtraData);
					pDBOMesh->dwFVF = 0; // strangely this was not done in above function, but done here to avoid issues with legacy functionality
					GetFVFOffsetMapFixedForBones(pDBOMesh, &offsetMap);
				}
			}

			// flag to adjust mesh using transform provided (not all models require this)
			GGMATRIX* pmatMeshTransform = NULL;
			bool bTransformMeshByTransformProvided = false;
			//bool bOnlyTransformMeshIfHaveNoParent = false;
			//if (pFrame->pParent == NULL) bOnlyTransformMeshIfHaveNoParent = true; // the logic being all DBOs require this if the mesh has no parent frame?!?!
			//if ( pDBOMesh->dwBoneCount > 0 && bOnlyTransformMeshIfHaveNoParent == true )
			if (pDBOMesh->dwBoneCount > 0 && bTransformMeshByTransformProvided == true)
			{
				float fDet = 0.0f;
				GGMATRIX matInverseMeshTransform;
				GGMatrixInverse(&matInverseMeshTransform, &fDet, &pFrame->matOriginal);
				pmatMeshTransform = &matInverseMeshTransform;
			}
			else
			{
				GGMATRIX matIdentityTransform;
				GGMatrixIdentity(&matIdentityTransform);
				pmatMeshTransform = &matIdentityTransform;
			}

			// scan DBO for offsets to required mesh data
			for (size_t v = 0; v < pDBOMesh->dwVertexCount; v++)
			{
				if (offsetMap.dwZ > 0)
				{
					XMFLOAT3 pos = XMFLOAT3(0, 0, 0);
					GGVECTOR3 vecPos = *(GGVECTOR3*)((float*)pDBOMesh->pVertexData + offsetMap.dwX + (offsetMap.dwSize * v));
					GGVec3TransformCoord(&vecPos, &vecPos, pmatMeshTransform);
					pos.x = vecPos.x;
					pos.y = vecPos.y;
					pos.z = vecPos.z;
					mesh.vertex_positions.push_back(pos);
				}
				if (offsetMap.dwNZ > 0)
				{
					XMFLOAT3 nor = XMFLOAT3(0, 0, 0);
					GGVECTOR3 vecNorm = *(GGVECTOR3*)((float*)pDBOMesh->pVertexData + offsetMap.dwNX + (offsetMap.dwSize * v));
					GGVec3TransformNormal(&vecNorm, &vecNorm, pmatMeshTransform);
					nor.x = vecNorm.x;
					nor.y = vecNorm.y;
					nor.z = vecNorm.z;
					mesh.vertex_normals.push_back(nor);
				}
				if (offsetMap.dwDiffuse > 0)
				{
					//PE Wicked: Alpha , Blue , green , red
					#define GGCOLOR_ABGR(r,g,b,a) ((GGCOLOR)((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))
					DWORD color = *(DWORD*)((DWORD*)pDBOMesh->pVertexData + offsetMap.dwDiffuse + (offsetMap.dwSize * v));
					int r = (int)((color & 0x00FF0000) >> 16);
					int g = (int)((color & 0x0000FF00) >> 8);
					int b = (int)((color & 0x000000FF));
					int a = (int)((color & 0xFF000000) >> 24);
					color = GGCOLOR_ABGR(r, g, b, a);
					mesh.vertex_colors.push_back(color);
				}
				if (offsetMap.dwTU[2] > 0)
				{
					XMFLOAT4 tan = XMFLOAT4(0, 0, 0, 0);
					GGVECTOR3 vecTangent = *(GGVECTOR3*)((float*)pDBOMesh->pVertexData + offsetMap.dwTU[2] + (offsetMap.dwSize * v));
					GGVec3TransformNormal(&vecTangent, &vecTangent, pmatMeshTransform);
					tan.x = vecTangent.x;
					tan.y = vecTangent.y;
					tan.z = vecTangent.z;
					mesh.vertex_tangents.push_back(tan);
				}
				if (offsetMap.dwTU[0] > 0)
				{
					XMFLOAT2 tex = XMFLOAT2(0, 0);
					tex.x = *(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTU[0] + (offsetMap.dwSize * v));
					tex.y = *(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTV[0] + (offsetMap.dwSize * v));
					tex.x *= g_iWickedUScalePreference;
					tex.y *= g_iWickedVScalePreference;
					mesh.vertex_uvset_0.push_back(tex);
				}
				if (offsetMap.dwTU[3] > 0)
				{
					XMFLOAT4 boneweights = XMFLOAT4(0, 0, 0, 0);
					boneweights.x = *(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTU[3] + (offsetMap.dwSize * v));
					boneweights.y = *(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTV[3] + (offsetMap.dwSize * v));
					boneweights.z = *(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTZ[3] + (offsetMap.dwSize * v));
					boneweights.w = *(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTW[3] + (offsetMap.dwSize * v));
					mesh.vertex_boneweights.push_back(boneweights);
				}
				if (offsetMap.dwTU[4] > 0)
				{
					XMUINT4 boneindices = XMUINT4(0, 0, 0, 0);
					boneindices.x = (int)*(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTU[4] + (offsetMap.dwSize * v));
					boneindices.y = (int)*(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTV[4] + (offsetMap.dwSize * v));
					boneindices.z = (int)*(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTZ[4] + (offsetMap.dwSize * v));
					boneindices.w = (int)*(float*)((float*)pDBOMesh->pVertexData + offsetMap.dwTW[4] + (offsetMap.dwSize * v));
					mesh.vertex_boneindices.push_back(boneindices);
				}
			}

			// apply material to mesh
			mesh.subsets.push_back(wiScene::MeshComponent::MeshSubset());
			mesh.subsets.back().materialID = pDBOMesh->wickedmaterialindex;
			mesh.subsets.back().indexOffset = 0;
			mesh.subsets.back().indexCount = (uint32_t)mesh.indices.size();
			//mesh.subsets.back().active = true; // REMOVED - MeshSubset.active no longer exists

			mesh.subsets_per_lod = 0; //PE: NEWLOD
			if (pDBOMesh->dwIndexCountLOD1 > 0)
			{
				mesh.subsets_per_lod = 1;
				mesh.subsets.push_back(wiScene::MeshComponent::MeshSubset());
				mesh.subsets.back().materialID = pDBOMesh->wickedmaterialindex;
				mesh.subsets.back().indexOffset = mesh.indices.size();
				mesh.subsets.back().indexCount = pDBOMesh->dwIndexCountLOD1;
				//mesh.subsets.back().active = false; // REMOVED

				for (size_t i = 0; i < pDBOMesh->dwIndexCountLOD1; i += 3)
				{
					mesh.indices.push_back(pDBOMesh->pIndicesLOD1[i + 0]);
					mesh.indices.push_back(pDBOMesh->pIndicesLOD1[i + 2]);
					mesh.indices.push_back(pDBOMesh->pIndicesLOD1[i + 1]);
				}
			}
			if (pDBOMesh->dwIndexCountLOD2 > 0)
			{
				mesh.subsets_per_lod = 2;
				mesh.subsets.push_back(wiScene::MeshComponent::MeshSubset());
				mesh.subsets.back().materialID = pDBOMesh->wickedmaterialindex;
				mesh.subsets.back().indexOffset = mesh.indices.size();
				mesh.subsets.back().indexCount = pDBOMesh->dwIndexCountLOD2;
				//mesh.subsets.back().active = false; // REMOVED

				for (size_t i = 0; i < pDBOMesh->dwIndexCountLOD2; i += 3)
				{
					mesh.indices.push_back(pDBOMesh->pIndicesLOD2[i + 0]);
					mesh.indices.push_back(pDBOMesh->pIndicesLOD2[i + 2]);
					mesh.indices.push_back(pDBOMesh->pIndicesLOD2[i + 1]);
				}
			}
			if (pDBOMesh->dwIndexCountLOD3 > 0)
			{
				mesh.subsets_per_lod = 3;
				mesh.subsets.push_back(wiScene::MeshComponent::MeshSubset());
				mesh.subsets.back().materialID = pDBOMesh->wickedmaterialindex;
				mesh.subsets.back().indexOffset = mesh.indices.size();
				mesh.subsets.back().indexCount = pDBOMesh->dwIndexCountLOD3;
				//mesh.subsets.back().active = false; // REMOVED

				for (size_t i = 0; i < pDBOMesh->dwIndexCountLOD3; i += 3)
				{
					mesh.indices.push_back(pDBOMesh->pIndicesLOD3[i + 0]);
					mesh.indices.push_back(pDBOMesh->pIndicesLOD3[i + 2]);
					mesh.indices.push_back(pDBOMesh->pIndicesLOD3[i + 1]);
				}
			}

			#ifdef PICKBVHTHREADED
			#define MAX_SUBAABB 16
			bool bCanUseBVH = true;
			if(pDBOMesh->dwBoneCount > 0)
				bCanUseBVH = false;
			if(mesh.indices.size() < 16)
				bCanUseBVH = false;

			//PE: Instanced object use master mesh so also need 50000
			if (MAX_SUBAABB > 0 && bCanUseBVH && iCurrentObjectID > 50000 && iCurrentObjectID < 90000) //70000
			{
				//PE: Add BVH subAABBs too all subsets including lods.
				// REMOVED - subAABB members no longer exist on MeshSubset
			}
			else
			#endif
			{
				// REMOVED - subAABB members no longer exist on MeshSubset
			}

			// LB: ensure culling mode is set for mesh
			if (pDBOMesh->bCull == false)
				mesh.SetDoubleSided(true);
			else
				mesh.SetDoubleSided(false);

			// LB: default render distance bias is zero
			float fRenderOrderBias = 0.0f;
			WickedCall_SetRenderOrderBias(pDBOMesh, fRenderOrderBias);

			// if this mesh is associated with bones, must be added to Wicked armature
			// some bones exist in DBO that have no bine weights or indices, so ignore those (Uzi HUD)
			if (pDBOMesh->dwBoneCount > 0 && mesh.vertex_boneindices.size() > 0)
			{
				// we need one armature per 'mesh with bones' (as bone count differs for each mesh)
				if (pDBOMesh->wickedarmatureindex == 0)
				{
					// create armature to hold framework for bones and animation
					Entity armatureEntity = CreateEntity();
					scene.names.Create(armatureEntity) = "Armature";
					scene.layers.Create(armatureEntity);
					scene.transforms.Create(armatureEntity);
					ArmatureComponent& armature = scene.armatures.Create(armatureEntity);
					// calculate the inverse bone matrices for each bone (to transform the pose world matrix to the bone local space)
					armature.inverseBindMatrices.resize(pDBOMesh->dwBoneCount);
					for (int iB = 0; iB < pDBOMesh->dwBoneCount; iB++)
					{
						XMFLOAT4X4 inverseBindMatrixForFrame;
						sBone* pBone = &pDBOMesh->pBones[iB];
						GGMATRIX matBind;
						matBind = pBone->matTranslation;
						inverseBindMatrixForFrame._11 = matBind._11;
						inverseBindMatrixForFrame._12 = matBind._12;
						inverseBindMatrixForFrame._13 = matBind._13;
						inverseBindMatrixForFrame._14 = matBind._14;
						inverseBindMatrixForFrame._21 = matBind._21;
						inverseBindMatrixForFrame._22 = matBind._22;
						inverseBindMatrixForFrame._23 = matBind._23;
						inverseBindMatrixForFrame._24 = matBind._24;
						inverseBindMatrixForFrame._31 = matBind._31;
						inverseBindMatrixForFrame._32 = matBind._32;
						inverseBindMatrixForFrame._33 = matBind._33;
						inverseBindMatrixForFrame._34 = matBind._34;
						inverseBindMatrixForFrame._41 = matBind._41;
						inverseBindMatrixForFrame._42 = matBind._42;
						inverseBindMatrixForFrame._43 = matBind._43;
						inverseBindMatrixForFrame._44 = matBind._44;
						armature.inverseBindMatrices[iB] = inverseBindMatrixForFrame;
					}
					pFrame->pMesh->wickedarmatureindex = armatureEntity;
				}

				// This node is an armature (a frame which contains a bone animatable mesh)
				mesh.armatureID = pFrame->pMesh->wickedarmatureindex;

				// ensure animated mesh frame cannot interfere with transform of armature
				// The object component will use an identity transform but will be parented to the armature
				// so possible animating entity becomes child of armature
				// LB: this was the original, and together with restoring the entity to parent attachment, fixes geometry corruptions!
				//PE: Test now always do no sorting, should speed up everything :)
				//PE: Still need sorting for gun to appear , even with DisableObjectZDepth ? need more testing before disable sorting for everything.
				//if(bNoHierarchySorting)
				//	scene.Component_Attach(mesh.armatureID, entity, true);
				//else
				//  scene.Component_Attach_Sort(mesh.armatureID, entity, true);
				//LB: Had to restore older system to correct disappearing weapons, switchs and many animating objects
				scene.Component_Attach(mesh.armatureID, entity, true);

				// for frames with a mesh, ignore the matOriginal transform (see above for mesh transform)
				bUseFrameMatrix = false;
			}

			// now create the internals for this mesh
			mesh.CreateRenderData();
		}
		//#### PE: mesh setup end ####
	}
	else
	{
		// if no mesh, just frame, then create entity to hold frame name
		if (entity == INVALID_ENTITY)
		{
			entity = CreateEntity();
			scene.transforms.Create(entity);
			scene.names.Create(entity) = pFrame->szName;
			pFrame->wickedobjindex = entity;
		}
	}

	// store 'frame' entity ID
	state.entityMap[pFrame->iID] = entity;

	// copy the transform from the frame to the scene entity transform
	TransformComponent& transform = *scene.transforms.GetComponent(entity);
	transform.scale_local = XMFLOAT3(1, 1, 1);
	transform.rotation_local = XMFLOAT4(0, 0, 0, 0);
	transform.translation_local = XMFLOAT3(0,0,0);
	if (bUseFrameMatrix == true)
	{
		GGMATRIX nodematrix = pFrame->matOriginal;
		transform.world._11 = nodematrix._11;
		transform.world._12 = nodematrix._12;
		transform.world._13 = nodematrix._13;
		transform.world._14 = nodematrix._14;
		transform.world._21 = nodematrix._21;
		transform.world._22 = nodematrix._22;
		transform.world._23 = nodematrix._23;
		transform.world._24 = nodematrix._24;
		transform.world._31 = nodematrix._31;
		transform.world._32 = nodematrix._32;
		transform.world._33 = nodematrix._33;
		transform.world._34 = nodematrix._34;
		transform.world._41 = nodematrix._41;
		transform.world._42 = nodematrix._42;
		transform.world._43 = nodematrix._43;
		transform.world._44 = nodematrix._44;
		transform.ApplyTransform();
	}
	transform.UpdateTransform();
	
	// attach entity to its parent
	// LB: this needed to be added to restore original system so DBO/CC characters are not geometry corrupted
	if (parent != INVALID_ENTITY) scene.Component_Attach(entity, parent, true);

	// recurse through all frames
	if (pFrame->pChild) WickedCall_LoadNode(pFrame->pChild, entity, root, state);
	if (pFrame->pSibling) WickedCall_LoadNode(pFrame->pSibling, parent, root, state);
}


void WickedCall_RefreshObjectAnimations(sObject* pObject, void* pstateptr)
{
	// get true pointer to loader state
	WickedLoaderState* pstate = (WickedLoaderState*)pstateptr;

	// current scene and root frame
	wiScene::Scene* pScene = &wiScene::GetScene();
	sFrame* pRootFrame = pObject->pFrame;

	// for objects that have animation data, create equivilant for wicked engine
	if (pObject->pAnimationSet)
	{
		// bridge call at end of this block converts backwards_compatibility_data
		// go through [first] animation set[s]
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		{
			// clear any old wicked animation components (in case of a refresh)
			if (pAnimSet->wickedanimentityindex > 0)
			{
				AnimationComponent* animationcomponent = pScene->animations.GetComponent( pAnimSet->wickedanimentityindex );
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

			// for each animation, create wicked animation component
			Entity animentity = CreateEntity();
			pScene->names.Create(animentity) = pAnimSet->szName;
			AnimationComponent& animationcomponent = pScene->animations.Create(animentity);
			pAnimSet->wickedanimentityindex = animentity;

			//animationcomponent.objectIndex = 0; // REMOVED - objectIndex no longer exists
			// REMOVED - animation culling via objectIndex no longer supported
			// increases as add more anim data (all goes into the above animationcomponent)
			int iSamplerAndChannelCount = 0;

			// go through all animations in this set
			sAnimation* pAnim = pAnimSet->pAnimation;
			while (pAnim != NULL)
			{
				//PE: Wicked - Support Matrix animations. convert to pPositionKeys,pRotateKeys,pScaleKeys
				if (pAnim->dwNumMatrixKeys > 0) //&& pAnim->dwNumPositionKeys == 0 && pAnim->dwNumRotateKeys == 0 && pAnim->dwNumScaleKeys == 0)
				{
					// supports appending matrix data to blank or populated pos/rot/scl keys
					DWORD dwNumKeys = pAnim->dwNumMatrixKeys;
					sPositionKey* pPosKeys = new sPositionKey[dwNumKeys];
					sRotateKey* pRotKeys = new sRotateKey[dwNumKeys];
					sScaleKey* pSclKeys = new sScaleKey[dwNumKeys];

					// unpack matrix date.
					for (size_t j = 0; j < dwNumKeys; ++j)
					{
						float fTime = pAnim->pMatrixKeys[j].dwTime;
						GGMATRIX mMat = pAnim->pMatrixKeys[j].matMatrix;
						XMFLOAT4X4 tmatrix;
						tmatrix._11 = mMat._11;	tmatrix._12 = mMat._12;	tmatrix._13 = mMat._13;	tmatrix._14 = mMat._14;
						tmatrix._21 = mMat._21;	tmatrix._22 = mMat._22;	tmatrix._23 = mMat._23;	tmatrix._24 = mMat._24;
						tmatrix._31 = mMat._31;	tmatrix._32 = mMat._32;	tmatrix._33 = mMat._33;	tmatrix._34 = mMat._34;
						tmatrix._41 = mMat._41;	tmatrix._42 = mMat._42;	tmatrix._43 = mMat._43;	tmatrix._44 = mMat._44;
						XMVECTOR S, R, T;
						XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&tmatrix));
						XMFLOAT3 vpos;
						XMStoreFloat3(&vpos, T);
						XMFLOAT4 vrot;
						XMStoreFloat4(&vrot, R);
						XMFLOAT3 vscale;
						XMStoreFloat3(&vscale, S);
						pPosKeys[j].dwTime = fTime;
						pRotKeys[j].dwTime = fTime;
						pSclKeys[j].dwTime = fTime;
						pPosKeys[j].vecPos.x = vpos.x;
						pPosKeys[j].vecPos.y = vpos.y;
						pPosKeys[j].vecPos.z = vpos.z;
						pRotKeys[j].Quaternion.x = vrot.x;
						pRotKeys[j].Quaternion.y = vrot.y;
						pRotKeys[j].Quaternion.z = vrot.z;
						pRotKeys[j].Quaternion.w = vrot.w;
						pSclKeys[j].vecScale.x = vscale.x;
						pSclKeys[j].vecScale.y = vscale.y;
						pSclKeys[j].vecScale.z = vscale.z;
					}

					// remove matrix keys
					SAFE_DELETE(pAnim->pMatrixKeys);
					pAnim->dwNumMatrixKeys = 0;

					// set the anim ptrs
					sPositionKey* pNewPosKeys = new sPositionKey[pAnim->dwNumPositionKeys+dwNumKeys];
					sRotateKey* pNewRotKeys = new sRotateKey[pAnim->dwNumRotateKeys+dwNumKeys];
					sScaleKey* pNewSclKeys = new sScaleKey[pAnim->dwNumScaleKeys+dwNumKeys];

					// append new keys to whole
					if ( pAnim->dwNumPositionKeys > 0 ) memcpy (pNewPosKeys, pAnim->pPositionKeys, sizeof(sPositionKey)*pAnim->dwNumPositionKeys);
					memcpy ((char*)pNewPosKeys+(sizeof(sPositionKey)*pAnim->dwNumPositionKeys), pPosKeys, sizeof(sPositionKey)*dwNumKeys);
					if ( pAnim->dwNumRotateKeys > 0 ) memcpy (pNewRotKeys, pAnim->pRotateKeys, sizeof(sRotateKey)*pAnim->dwNumRotateKeys);
					memcpy ((char*)pNewRotKeys+(sizeof(sRotateKey)*pAnim->dwNumRotateKeys), pRotKeys, sizeof(sRotateKey)*dwNumKeys);
					if ( pAnim->dwNumScaleKeys > 0 ) memcpy (pNewSclKeys, pAnim->pScaleKeys, sizeof(sScaleKey)*pAnim->dwNumScaleKeys);
					memcpy ((char*)pNewSclKeys+(sizeof(sScaleKey)*pAnim->dwNumScaleKeys), pSclKeys, sizeof(sScaleKey)*dwNumKeys);

					// set the new anim ptrs
					SAFE_DELETE(pAnim->pPositionKeys);
					SAFE_DELETE(pAnim->pRotateKeys);
					SAFE_DELETE(pAnim->pScaleKeys);
					pAnim->pPositionKeys = pNewPosKeys;
					pAnim->pRotateKeys = pNewRotKeys;
					pAnim->pScaleKeys = pNewSclKeys;
					pAnim->dwNumPositionKeys += dwNumKeys;
					pAnim->dwNumRotateKeys += dwNumKeys;
					pAnim->dwNumScaleKeys += dwNumKeys;

					// free resources
					SAFE_DELETE(pPosKeys);
					SAFE_DELETE(pRotKeys);
					SAFE_DELETE(pSclKeys);
				}

				// new anim means new frame with anim data to consider)
				int iOffset = iSamplerAndChannelCount;

				// work out how many samplers/channels are needed

				//PE: OPTIMIZING No need to process scale keys if not used. (RunAnimationUpdateSystem is slow). most anim dont use scale.
				bool bGotScale = false;
				if (pAnim->dwNumScaleKeys > 0)
				{
					for (size_t j = 0; j < pAnim->dwNumScaleKeys; ++j)
					{
						XMFLOAT3 vec3;
						vec3.x = pAnim->pScaleKeys[j].vecScale.x;
						vec3.y = pAnim->pScaleKeys[j].vecScale.y;
						vec3.z = pAnim->pScaleKeys[j].vecScale.z;
						if (vec3.x != 1.0f || vec3.y != 1.0f || vec3.z != 1.0f)
						{
							bGotScale = true;
							break;
						}
					}
				}

				bool bSamplerChannelsMask[3];
				bSamplerChannelsMask[0] = false;
				bSamplerChannelsMask[1] = false;
				bSamplerChannelsMask[2] = false;
				int iSamplerChannelsNeeded = 0;
				if (pAnim->dwNumPositionKeys > 0) { iSamplerChannelsNeeded++; bSamplerChannelsMask[0] = true; }
				if (pAnim->dwNumRotateKeys > 0) { iSamplerChannelsNeeded++; bSamplerChannelsMask[1] = true; }
				if (bGotScale && pAnim->dwNumScaleKeys > 0) { iSamplerChannelsNeeded++; bSamplerChannelsMask[2] = true; }
				iSamplerAndChannelCount += iSamplerChannelsNeeded;

				// calculate size of samplers (to hold raw data)
				animationcomponent.samplers.resize(iSamplerAndChannelCount);
				int iSamplerOffset = iOffset;
				for (int i = 0; i < 3; i++)
				{
					if (bSamplerChannelsMask[i] == true)
					{
						animationcomponent.samplers[iSamplerOffset].mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
						int count = 0;
						if (i == 0) count = pAnim->dwNumPositionKeys;
						if (i == 1) count = pAnim->dwNumRotateKeys;
						if (i == 2) count = pAnim->dwNumScaleKeys;
						animationcomponent.samplers[iSamplerOffset].backwards_compatibility_data.keyframe_times.resize(count);
						for (size_t j = 0; j < count; ++j)
						{
							float time = 0.0f;
							if (i == 0) time = (float)pAnim->pPositionKeys[j].dwTime;
							if (i == 1) time = (float)pAnim->pRotateKeys[j].dwTime;
							if (i == 2) time = (float)pAnim->pScaleKeys[j].dwTime;
							animationcomponent.samplers[iSamplerOffset].backwards_compatibility_data.keyframe_times[j] = time;
							animationcomponent.start = min(animationcomponent.start, time);
							animationcomponent.end = max(animationcomponent.end, time);
						}
						if (i == 1)
						{
							// rotation
							animationcomponent.samplers[iSamplerOffset].backwards_compatibility_data.keyframe_data.resize(count * 4);
							for (size_t j = 0; j < count; ++j)
							{
								XMFLOAT4 rot;
								rot.x = pAnim->pRotateKeys[j].Quaternion.x;
								rot.y = pAnim->pRotateKeys[j].Quaternion.y;
								rot.z = pAnim->pRotateKeys[j].Quaternion.z;
								rot.w = pAnim->pRotateKeys[j].Quaternion.w;
								((XMFLOAT4*)animationcomponent.samplers[iSamplerOffset].backwards_compatibility_data.keyframe_data.data())[j] = rot;
							}
						}
						else
						{
							// position or scale
							animationcomponent.samplers[iSamplerOffset].backwards_compatibility_data.keyframe_data.resize(count * 3);
							for (size_t j = 0; j < count; ++j)
							{
								XMFLOAT3 vec3;
								if (i == 0)
								{
									vec3.x = pAnim->pPositionKeys[j].vecPos.x;
									vec3.y = pAnim->pPositionKeys[j].vecPos.y;
									vec3.z = pAnim->pPositionKeys[j].vecPos.z;
								}
								else
								{
									vec3.x = pAnim->pScaleKeys[j].vecScale.x;
									vec3.y = pAnim->pScaleKeys[j].vecScale.y;
									vec3.z = pAnim->pScaleKeys[j].vecScale.z;
								}
								((XMFLOAT3*)animationcomponent.samplers[iSamplerOffset].backwards_compatibility_data.keyframe_data.data())[j] = vec3;
							}
						}
						iSamplerOffset++;
					}
				}

				// calculate size of channels (to direct anim data to target)
				animationcomponent.channels.resize(iSamplerAndChannelCount);
				int iChannelOffset = iOffset;
				for (size_t i = 0; i < 3; ++i)
				{
					if (bSamplerChannelsMask[i] == true)
					{
						//PE: All channels MUST have a target , or wicked will crash.
						//PE: Target is used to get the transform
						sFrame*	pFrameMustSet = pAnim->pFrame;
						if (!pFrameMustSet)
						{
							// if pframe not found assume animation is for pRootFrame
							pFrameMustSet = pRootFrame;
						}
						if (pFrameMustSet)
						{
							// set the target, samplerindex and type for this channel item
							int iFrameIndexForThisAnim = pFrameMustSet->iID;
							wiECS::Entity thisTarget = wiECS::INVALID_ENTITY;
							if (pstate) thisTarget = pstate->entityMap[iFrameIndexForThisAnim];
							animationcomponent.channels[iChannelOffset].target = thisTarget;
							animationcomponent.channels[iChannelOffset].samplerIndex = (uint32_t)iChannelOffset;
							if (i == 0) animationcomponent.channels[iChannelOffset].path = AnimationComponent::AnimationChannel::Path::TRANSLATION;
							if (i == 1) animationcomponent.channels[iChannelOffset].path = AnimationComponent::AnimationChannel::Path::ROTATION;
							if (i == 2) animationcomponent.channels[iChannelOffset].path = AnimationComponent::AnimationChannel::Path::SCALE;

							// new features of the wicked animation system
							//animationcomponent.channels[iChannelOffset].iUsePreFrame = 0; // REMOVED
							//animationcomponent.channels[iChannelOffset].vPreFrameScale = XMVectorSet(1, 1, 1, 0); // REMOVED
							//animationcomponent.channels[iChannelOffset].qPreFrameRotation = XMQuaternionRotationRollPitchYaw(0, 0, 0); // REMOVED
							//animationcomponent.channels[iChannelOffset].vPreFrameTranslation = XMVectorSet(0, 0, 0, 0); // REMOVED
							int iThisSamplerOffset = animationcomponent.channels[iChannelOffset].samplerIndex;

							// finally store channel and sampler offsets for this animation item
							pAnim->wickedanimationchannel[i] = iChannelOffset;
							pAnim->wickedanimationsampler[i] = iThisSamplerOffset;

							// next channel
							iChannelOffset++;
						}
					}
				}

				// next animation
				pAnim = pAnim->pNext;
			}

		// bridge: create AnimationDataComponent entities, set GG defaults
		GGAnimBridge_OnLoadObject(pScene, pAnimSet->wickedanimentityindex);
		}
	}
}

void WickedCall_AddObject ( sObject* pObject )
{
	// delibeately not create the wicked object, speeds up everything in wicked engine
	// until the object actually needed (only used for decals/explosions/particles which require 1000's of objects)
	if (g_bWickedCreateOnlyWhenUsed == true)
	{
		// soon to be replaced with nicer GPU particles
		pObject->wickedrootentityindex = 0;
		return;
	}

	if (pObject->iMeshCount >= 590)
	{
		//PE: Get stack overflow when object have more then 600 meshes.
		// Walled Garden Pack\Mausoleum 05.fpe
		return;
	}

	// ensure wickedloaderstateptr deleted first
	if (pObject->wickedloaderstateptr)
	{
		delete pObject->wickedloaderstateptr;
		pObject->wickedloaderstateptr = NULL;
	}

	// reject if no mesh
	if (pObject->ppMeshList == NULL)
		return;

	// for structures, allow to pass if object has at least one mesh with geometry in it
	bool bhasGeometry = false;
	for (int m = 0; m < pObject->iMeshCount; m++) if (pObject->ppMeshList[m]->dwVertexCount > 0) bhasGeometry = true;
	if (bhasGeometry == false) return;

	// instead of creating a scene and merging, just add direct to main scene (faster?)
	wiScene::Scene* pScene = &wiScene::GetScene();

	// create scene object (will contain created object we are adding)
	Entity rootEntity = CreateEntity();
	pScene->transforms.Create(rootEntity);

	// create materials, but do not load textures, done later when load textures
	for ( int iM = 0; iM < pObject->iMeshCount; iM++ )
	{
		sMesh* pMesh = pObject->ppMeshList[iM];
		if (pMesh)
		{
			if (pMesh->pTextures)
			{
				char* pTextureFilename = pMesh->pTextures[0].pName;
				if ( pTextureFilename )
				{
					// create material, texture them later on
					if (!(pMesh->bInstanced && pMesh->master_wickedmeshindex > 0))
					{
						//PE: No need to create material for Instanced objects.
						wiECS::Entity materialEntity = pScene->Entity_CreateMaterial(pTextureFilename);
						wiScene::MaterialComponent& material = *pScene->materials.GetComponent(materialEntity);
						material.baseColor = XMFLOAT4(1, 1, 1, 1);
						pMesh->wickedmaterialindex = materialEntity;

						// LB: ensure transparency is respected in wicked
						if (pMesh->bTransparency == true)
							material.userBlendMode = BLENDMODE_ALPHA;
						else
							material.userBlendMode = BLENDMODE_OPAQUE;

						// LB: assume that transparent meshes cast no shadows
						if (pMesh->bTransparency == true)
							material.SetCastShadow(false);
						else
							material.SetCastShadow(true);
					}
					else
					{
						pMesh->wickedmaterialindex = 0; //PE: We use master material index.
					}
				}
			}
		}
	}
	if (pScene->materials.GetCount() == 0)
	{
		// if no material created above, add default material for mesh
		wiECS::Entity materialEntity = pScene->Entity_CreateMaterial("defaultMaterial");
		wiScene::MaterialComponent& material = *pScene->materials.GetComponent(materialEntity);
		material.baseColor = XMFLOAT4(1, 1, 1, 1);

		// apply default material to all meshes in this object
		for (int iM = 0; iM < pObject->iMeshCount; iM++)
			if (pObject->ppMeshList[iM])
				pObject->ppMeshList[iM]->wickedmaterialindex = materialEntity;
	}

	// if object has bones (animation), then need an armature - done inside LoadNode
	// but need to reset wickedarmatureindex IDs as this scene is a temporarly one, to be merged at the end
	for (int iM = 0; iM < pObject->iMeshCount; iM++)
		if (pObject->ppMeshList[iM]->dwBoneCount > 0)
			pObject->ppMeshList[iM]->wickedarmatureindex = 0;

	// Create transform hierarchy, assign objects, meshes, armatures, cameras
	WickedLoaderState state;
	state.scene = pScene;
	state.storeMasterRootEntityIndex = rootEntity;
	sFrame* pRootFrame = pObject->pFrame;
	WickedCall_LoadNode ( pRootFrame, rootEntity, rootEntity, state );
	

	for (int iM = 0; iM < pObject->iMeshCount; iM++)
	{
		sMesh* pDBOMesh = pObject->ppMeshList[iM];
		if (pDBOMesh)
		{
			uint64_t meshindex = pDBOMesh->wickedmeshindex;
			if (meshindex > 0)
			{
				wiScene::MeshComponent* mesh = pScene->meshes.GetComponent(meshindex);
				if (mesh)
				{
					if (mesh->vertex_colors.size() > 0)
					{
						uint64_t wickedmaterialindex = pDBOMesh->wickedmaterialindex;
						if(wickedmaterialindex == 0)
							wickedmaterialindex = pDBOMesh->master_wickedmaterialindex;
						if (wickedmaterialindex != 0)
						{
							wiScene::MaterialComponent* material = pScene->materials.GetComponent(wickedmaterialindex);
							if (material)
							{
								//PE: defaults.
								material->roughness = 1;
								material->metalness = 0;
								material->reflectance = 0.04f;// 0.002f;
								if (pDBOMesh->iReservedForFuture > 10)
								{
									//PE: activate as lod object. with vertexcolors off by default.
									if (pDBOMesh->pFrameAttachedTo)
									{
										ObjectComponent* object = wiScene::GetScene().objects.GetComponent(pDBOMesh->pFrameAttachedTo->wickedobjindex);
										if (object)
										{
											//pDBOMesh->pFrameAttachedTo->wickedobjindex
											//object->SetLOD(true); // REMOVED
											//object->SetLodDistance(pDBOMesh->iReservedForFuture); // REMOVED
										}
									}
								}
								else
									material->SetUseVertexColors(true);
								material->SetDirty();
							}
						}
					}
				}
			}
		}
	}


	// Create armature-bone mappings (connect armature bone collection to frame entities created in LoadNode)
	for (int iM = 0; iM < pObject->iMeshCount; iM++)
	{
		sMesh* pDBOMesh = pObject->ppMeshList[iM];
		if (pDBOMesh->dwBoneCount > 0)
		{
			if (pDBOMesh->wickedarmatureindex != 0)
			{
				ArmatureComponent& armature = *pScene->armatures.GetComponent(pDBOMesh->wickedarmatureindex);
				armature.boneCollection.resize(pDBOMesh->dwBoneCount);
				for (size_t i = 0; i < pDBOMesh->dwBoneCount; ++i)
				{
					// for each bone within this mesh, find the frame that matches the name
					sFrame* pFoundFrame = NULL;
					LPSTR pSearchFor = pDBOMesh->pBones[i].szName;
					for (int iFindFrame = 0; iFindFrame < pObject->iFrameCount; iFindFrame++)
					{
						if (stricmp(pObject->ppFrameList[iFindFrame]->szName, pSearchFor) == NULL)
						{
							// found frame that matches this bone name
							pFoundFrame = pObject->ppFrameList[iFindFrame];
							break;
						}
					}
					if (pFoundFrame)
					{
						// get the bone entity from the hierarchy
						Entity boneEntity = state.entityMap[pFoundFrame->iID];

						// assign the bone entity to this armature mesh's bone collection
						armature.boneCollection[i] = boneEntity;
					}
				}

				//PE: Make sure all boneCollection has a transform , or we get a crash.
				for (Entity boneEntity : armature.boneCollection)
				{
					const TransformComponent& bone = *pScene->transforms.GetComponent(boneEntity);
					if (!&bone)
					{
						pScene->armatures.Remove(pDBOMesh->wickedarmatureindex);
						pDBOMesh->wickedarmatureindex = 0;
						break;
					}
				}
			}
		}
	}

	// Create animations (from animation data stored in DBO)
	WickedLoaderState* pCopyLoaderState = new WickedLoaderState;
	pCopyLoaderState->scene = state.scene;
	pCopyLoaderState->storeMasterRootEntityIndex = state.storeMasterRootEntityIndex;
	pCopyLoaderState->entityMap = state.entityMap;
	pCopyLoaderState->entityMeshMap = state.entityMeshMap;

	//int objectindex = state.entityMap[pObject->pFrame->iID];
	WickedCall_RefreshObjectAnimations(pObject, (void*)pCopyLoaderState);

	// trigger an update to the root entity transform
	wiScene::TransformComponent* pATransform = pScene->transforms.GetComponent(rootEntity);
	pATransform->SetDirty(true);

	// set a transform for the object
	wiScene::TransformComponent* pRootTransform = pScene->transforms.GetComponent(rootEntity);
	pRootTransform->Translate(XMFLOAT3(0, 0, 0));
	pRootTransform->UpdateTransform();

	// stores wicked's object entity 'ID' so object can reference by sObject ptr
	pObject->wickedrootentityindex = rootEntity;
	pObject->wickedloaderstateptr = (void*)pCopyLoaderState;
}

void WickedCall_SetObjectSpeed(sObject* pObject, float fSpeed)
{
	// set the newly added speed modifier in wicked
	if ( pObject->pAnimationSet )
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		{
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if (animationcomponent)
			{
				// not all animation entries have data (FBX imports can have empty animation sets!)
				animationcomponent->speed = fSpeed*50; //PE: (ORG:50) Need to adjust this to fit old speed.
			}
		}
	}
}

//PE: this works for doors and other animation controlls by lua.
//PE: but dont work for guns , as they rely on the playing status.
//PE: so we cant do this at the moment.
//PE: Perhaps its better to control this directly in the lua scripts ?
void WickedCall_CheckAnimationDone(sObject* pObject)
{
	// set the newly added speed modifier in wicked
	if (pObject->pAnimationSet)
	{
		float fEndFrame = -1;
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		{
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if(animationcomponent)
			{
				//PE: Wicked dont stop animations by itself, WickedCall_SetObjectSpeed is called all the time.
				//PE: So check here if we need to stop the animation.
				if (animationcomponent->IsPlaying())
				{
					//PE: pObject->bAnimPlaying is not set anywhere.
					pObject->bAnimPlaying = true;
					//float timer = animationcomponent->timer;
					//float length = animationcomponent->GetLength();
					bool isended = animationcomponent->IsEnded();
					bool islooped = animationcomponent->IsLooped();
					if (isended && !islooped)
					{
						fEndFrame = animationcomponent->end;
						animationcomponent->SetLooped(false);
						animationcomponent->Stop();
						pObject->bAnimPlaying = false;
						//PE: Must make sure we are set at the last frame.
						//PE: Fix - https://thegamecreators.teamwork.com/index.cfm#/tasks/21003817?c=10406263 ,
						animationcomponent->timer = fEndFrame;
						//animationcomponent->SetUpdateOnce(); // REMOVED
					}
				}
			}
		}
	}
}

bool WickedCall_GetAnimationPlayingState (sObject* pObject)
{
	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	if (pAnimSet)
	{
		Entity animentity = pAnimSet->wickedanimentityindex;
		AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
		if(animationcomponent)
			if (animationcomponent->IsPlaying())
				return true;
	}
	return false;
}

void WickedCall_SetAnimationLerpFactor (sObject* pObject)
{
	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	if (pAnimSet)
	{
		Entity animentity = pAnimSet->wickedanimentityindex;
		AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
		if (animationcomponent)
		{
			//if (animationcomponent->updateonce == false) // REMOVED
			{
				animationcomponent->amount = pObject->fAnimInterp;
			}
		}
	}
}

void WickedCall_PlayObject(sObject* pObject, float fStart, float fEnd, bool bLooped)
{
	if ( pObject->pAnimationSet )
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		if (!pAnimSet)
			return;
		{
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if (animationcomponent)
			{
				animationcomponent->start = fStart;
				if (fEnd != -1)
				{
					animationcomponent->end = fEnd;
				}
				animationcomponent->timer = fStart;
				//if (animationcomponent->updateonce == false) // REMOVED
				{
					animationcomponent->amount = pObject->fAnimInterp;
				}
				animationcomponent->SetLooped(bLooped);
				animationcomponent->Play();
			}
		}
	}
}

void WickedCall_InstantObjectFrameUpdate(sObject* pObject)
{
	if (pObject->pAnimationSet)
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		Entity animentity = pAnimSet->wickedanimentityindex;
		AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
		if (animationcomponent)
		{
			//animationcomponent->updateonce = true; // REMOVED
			animationcomponent->amount = 1;
		}
	}
}

void WickedCall_GetObjectAnimationData(sObject* pObject, float* pStart, float* pFinish )
{
	if ( pObject->pAnimationSet )
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		Entity animentity = pAnimSet->wickedanimentityindex;
		AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
		if (animationcomponent)
		{
			*pStart = animationcomponent->start;
			*pFinish = animationcomponent->end;
		}
	}
}

void WickedCall_StopObject(sObject* pObject)
{
	if (pObject)
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		if (pAnimSet)
		{
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if (animationcomponent )
				animationcomponent->Stop();
		}
	}
}

void WickedCall_SetObjectFrame(sObject* pObject, float fFrame)
{
	if ( pObject->pAnimationSet )
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		{
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if (animationcomponent)
			{
				animationcomponent->SetLooped(false);
				animationcomponent->Stop();
				animationcomponent->timer = fFrame;
				//animationcomponent->SetUpdateOnce(); // REMOVED
			}
		}
	}
}

void WickedCall_SetObjectFrameEx(sObject* pObject, float fFrame)
{
	// used when want to set the framr even if looping or playing
	if (pObject->pAnimationSet)
	{
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		{
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if (animationcomponent)
			{
				animationcomponent->timer = fFrame;
				//animationcomponent->SetUpdateOnce(); // REMOVED
			}
		}
	}
}


float WickedCall_GetObjectFrame(sObject* pObject)
{
	float fFrame = 0.0f;
	if (pObject)
	{
		if (pObject->pAnimationSet)
		{
			// first animset only for frame return
			sAnimationSet* pAnimSet = pObject->pAnimationSet;
			Entity animentity = pAnimSet->wickedanimentityindex;
			AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
			if (animationcomponent)
			{
				fFrame = animationcomponent->timer;
			}
		}
	}
	return fFrame;
}

float WickedCall_GetObjectRealFrame(sObject* pObject)
{
	// special 
	float fFrame = 0.0f;
	if (pObject->pAnimationSet)
	{
		// first animset only for frame return
		sAnimationSet* pAnimSet = pObject->pAnimationSet;
		Entity animentity = pAnimSet->wickedanimentityindex;
		AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent(animentity);
		if (animationcomponent)
		{
			fFrame = animationcomponent->timer;
			if (animationcomponent->IsLooped())
			{
				// special situation where looped animations have an extra 1.0f after the loop end to interp back to first frame
				// and this frame returned represents the real frame in the anim data, not the animation.timer frame which tracks the raw position in the seqwence (timer)
				if (fFrame >= animationcomponent->end)
				{
					// the assumption is that the _timers are all aligned as 1.0f timings so they match key frame subscripts
					float fInterpT = fFrame - animationcomponent->end;
					fFrame = animationcomponent->start + fInterpT;
				}
			}
		}
	}
	return fFrame;
}

bool bBlockSceneUpdate = false;
void WickedCall_RemoveObject( sObject* pObject )
{
	// when removing assets from the wicked engine scene
	uint64_t rootEntity = pObject->wickedrootentityindex;
	if (rootEntity > 0)
	{
		WickedCall_SetObjectVisible(pObject, false);
		wiJobSystem::context ctx;
		wiJobSystem::Wait(ctx);

		// stop any animation playing
		WickedCall_StopObject(pObject);

		// remove any animcomponents
		if (pObject->pAnimationSet)
		{
			sAnimationSet* pAnimSet = pObject->pAnimationSet;
			{
				if (pAnimSet->wickedanimentityindex > 0)
				{				
					AnimationComponent* animationcomponent = wiScene::GetScene().animations.GetComponent( pAnimSet->wickedanimentityindex );
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
		}

		// remove all materials and armature used by any mesh
		for (int iM = 0; iM < pObject->iMeshCount; iM++)
		{
			sMesh* pMesh = pObject->ppMeshList[iM];
			if (pMesh)
			{		
				if (pMesh->wickedmeshindex > 0)
				{
					if(!pMesh->bInstanced)
						wiScene::GetScene().Entity_Remove(pMesh->wickedmeshindex);
					pMesh->wickedmeshindex = 0;
				}
				if (pMesh->wickedmaterialindex > 0)
				{				
					if (!pMesh->bInstanced)
						wiScene::GetScene().Entity_Remove(pMesh->wickedmaterialindex);
					pMesh->wickedmaterialindex = 0;
				}
				if (pMesh->wickedarmatureindex > 0)
				{
					if (!pMesh->bInstanced)
						wiScene::GetScene().Entity_Remove(pMesh->wickedarmatureindex);
					pMesh->wickedarmatureindex = 0;
				}
			}
		}

		// remove all entities associated with all frames of this object
		for (int iF = 0; iF < pObject->iFrameCount; iF++)
		{
			sFrame* pFrame = pObject->ppFrameList[iF];
			if (pFrame)
			{
				wiScene::GetScene().Entity_Remove(pFrame->wickedobjindex);
				pFrame->wickedobjindex = 0;
			}
		}
		// look at editor on how objects are thoroughly deleted
		wiScene::GetScene().Entity_Remove(rootEntity);
		pObject->wickedrootentityindex = 0;

		// ensure wickedloaderstateptr deleted first
		if (pObject->wickedloaderstateptr)
		{
			delete pObject->wickedloaderstateptr;
			pObject->wickedloaderstateptr = NULL;
		}

		// finally update scene with removals
		if(!bBlockSceneUpdate)
			wiScene::GetScene().Update(0);
	}
}

void WickedCall_SetTexturePath(LPSTR pPath)
{
	g_pWickedTexturePath = pPath;
}

