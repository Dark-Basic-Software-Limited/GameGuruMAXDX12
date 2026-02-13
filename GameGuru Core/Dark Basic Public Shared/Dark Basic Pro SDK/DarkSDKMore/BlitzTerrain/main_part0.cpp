#include "main.h"
#include "createobject.h"
#include "rttms.h"
#include "code-from-dbp.h"
#include "CObjectsC.h"
#include "CMemblocks.h"
#include "CGfxC.h"
#include ".\..\..\Shared\Objects\ShadowMapping\cShadowMaps.h"

bool update_mesh_light(sMesh* pMesh, sObject* pObject, sFrame* pFrame);

s_BT_main BT_Main;

// shadow mapping
extern CascadedShadowsManager g_CascadedShadow;

extern GlobStruct* g_pGlob;
extern void AddToRenderList(LPVOID pFunction, int iPriority);

int g_LevelToRender = 5;
int g_iQualityTechniqueMode = 0;
int g_iTerrainIDForShadowMap = 0;
bool g_bRenderTerrainForShadowMap = false;

// New DX11 Constant buffer structure
#ifdef DX11
struct CBChangePerTerrsainChunk
{
	KMaths::Matrix mWorld;
	KMaths::Matrix mView;
	KMaths::Matrix mProjection;
};
struct CBChangePerTerrsainChunkPS
{
	GGCOLOR vMaterialEmissive;
	float fAlphaOverride;
	float fRes1;
	float fRes2;
	float fRes3;
	KMaths::Matrix mViewInv;
	KMaths::Matrix mViewProj;
	KMaths::Matrix mPrevViewProj;
};
ID3D11Buffer* m_pCBChangePerTerrsainChunk = NULL;
ID3D11Buffer* m_pCBChangePerTerrsainChunkPS	= NULL;
#else
#endif

void SetTerrainRenderLevel( int size )
{
	float s = (float)size;
	if ( s < 1 ) s = 1;
	if ( s > 100 ) s = 100;

	// 1-5
	g_LevelToRender = (int)(ceil(s /= 20.0f));
}

#ifndef COMPILE_GDK

	void BTConstructor()
	{
		BT_Main.buildstep=1;

		// from passcore
		//CO_ReceiveCoreDataPtr(NULL);
		AddToRenderList(&BT_Intern_Render,7500);
		memset(&BT_Main,0,sizeof(s_BT_main));
		BT_Main.InstructionQueueSize=1000;
		BT_Main.InstructionQueueUsed=0;
		BT_Main.InstructionQueue=new char[BT_Main.InstructionQueueSize];
		BT_Main.InstructionQueue[0]=NULL;
		BT_Main.Initialised=true;
		BT_Main.buildstep=1;
	}
	
	void BTReceiveCoreDataPtr(LPVOID CorePtr)
	{
	}

	void BTDestructor()
	{
	}

#endif

//========================
// === BT MAKE TERRAIN ===
//========================
unsigned long BT_MakeTerrain()
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_MAKETERRAIN;

//Variables
	unsigned long TerrainNum;
	bool FoundID=0;

//Check that BT is initialised
	if(BT_Main.Initialised)
	{
	//Find a free terrain ID
		TerrainNum=1;
		while(FoundID==0)
		{
		//Check if we have gone over terrain limit and call an error
			if(TerrainNum>C_BT_MAXTERRAINS+1) 
			{
				BT_Intern_Error(C_BT_ERROR_MAXTERRAINSEXCEDED);
				return 0;
			}

			if(BT_Main.Terrains[TerrainNum].Exists)
			{
				TerrainNum++;
			}else{
				FoundID=1;
			}
		}

	//Set default values
		memset(&BT_Main.Terrains[TerrainNum],0,sizeof(s_BT_terrain));
		BT_Main.Terrains[TerrainNum].Exists=1;
		BT_Main.Terrains[TerrainNum].ID=TerrainNum;
		BT_Main.Terrains[TerrainNum].Scale=1.0f;
		BT_Main.Terrains[TerrainNum].YScale=1.0f;
		BT_Main.Terrains[TerrainNum].Tile=1.0f;
		BT_Main.Terrains[TerrainNum].LODLevels=1;
		BT_Main.Terrains[TerrainNum].LODLevel=(s_BT_LODLevel*)malloc(sizeof(s_BT_LODLevel)*C_BT_MAXLODLEVELS);
		if(BT_Main.Terrains[TerrainNum].LODLevel==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		memset ( BT_Main.Terrains[TerrainNum].LODLevel, 0, sizeof(s_BT_LODLevel)*C_BT_MAXLODLEVELS );
		BT_Main.Terrains[TerrainNum].Built=0;
		BT_Main.Terrains[TerrainNum].Generated=0;
		BT_Main.Terrains[TerrainNum].MeshOptimisation=true;
		BT_Main.Terrains[TerrainNum].ExclusionThreshold=128;
		BT_Main.Terrains[TerrainNum].DetailBlendMode=8;

	//Setup Environment map
		BT_Main.Terrains[TerrainNum].EnvironmentMap=(BT_EnvironmentMap*)malloc(sizeof(BT_EnvironmentMap));
		if(BT_Main.Terrains[TerrainNum].EnvironmentMap==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		memset(BT_Main.Terrains[TerrainNum].EnvironmentMap,0,sizeof(BT_EnvironmentMap));
		BT_Main.Terrains[TerrainNum].EnvironmentMap->EnvironmentBuffer=(unsigned long*)malloc(4);
		if(BT_Main.Terrains[TerrainNum].EnvironmentMap->EnvironmentBuffer==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);

	//Setup TerrainInfo
		BT_TerrainInfo* TerrainInfo=(BT_TerrainInfo*)malloc(sizeof(BT_TerrainInfo));
		if(TerrainInfo==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		memset(TerrainInfo,0,sizeof(BT_TerrainInfo));
		TerrainInfo->Scale=1.0f;
		TerrainInfo->YScale=1.0f;
		TerrainInfo->Tile=1.0f;
		TerrainInfo->LODLevels=1;
		TerrainInfo->MeshOptimisation=true;
		TerrainInfo->ExclusionThreshold=128;
		TerrainInfo->DetailBlendMode=8;
		BT_Main.Terrains[TerrainNum].Info=(void*)TerrainInfo;

	//Return the terrain id
		return TerrainNum;

	}
	return NULL;
}
// === END FUNCTION ===



//=================================
// === BT SET TERRAIN HEIGHTMAP ===
//=================================
void BT_SetTerrainHeightmap(unsigned long terrainid,unsigned long image)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINHEIGHTMAP;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built==false)
		{
		//Set the values
			BT_Main.Terrains[terrainid].Heightmap=image;
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINALREADYBUILT);
			return;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}

// ==============================
// === BT SET TERRAIN TEXTURE ===
// ==============================
void BT_SetTerrainTexture(unsigned long terrainid,unsigned long image)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINTEXTURE;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built)
		{
		//Set the texture
			BT_Main.Terrains[terrainid].Texture=image;
			SetObjectBlendMap(BT_Main.Terrains[terrainid].ObjectID,0,image,10,4);
		}else{
		//Set the texture
			BT_Main.Terrains[terrainid].Texture=image;
		}
	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->Texture=image;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ================================
// === BT SET TERRAIN EXCLUSION ===
// ================================
void BT_SetTerrainExclusion(unsigned long terrainid,unsigned long image)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINEXCLUSION;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built==false)
		{
		//Set the exclusionmap
#ifdef C_BT_FULLVERSION
			BT_Main.Terrains[terrainid].Exclusionmap=image;
		//Update in terrain info
			((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->Exclusion=true;
#endif
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINALREADYBUILT);
			return;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===


// ==========================================
// === BT SET TERRAIN EXCLUSION THRESHOLD ===
// ==========================================
void BT_SetTerrainExclusionThreshold(unsigned long terrainid,unsigned long threshold)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINEXCLUSIONTHRESHOLD;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built==false)
		{
		//Set the exclusion threshold
#ifdef C_BT_FULLVERSION
			BT_Main.Terrains[terrainid].ExclusionThreshold=unsigned char(threshold);

		//Update in terrain info
			((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->ExclusionThreshold=unsigned char(threshold);
#endif
		}else{
			BT_Intern_Error(C_BT_ERROR_TERRAINALREADYBUILT);
			return;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// =============================
// === BT SET TERRAIN DETAIL ===
// =============================
void BT_SetTerrainDetail(unsigned long terrainid,unsigned long image)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINDETAIL;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built)
		{
		//Set the detailmap
			BT_Main.Terrains[terrainid].Detailmap=image;
			SetObjectBlendMap(BT_Main.Terrains[terrainid].ObjectID,1,image,11,BT_Main.Terrains[terrainid].DetailBlendMode);
		}else{
			BT_Main.Terrains[terrainid].Detailmap=image;
		}
	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->Detailmap=image;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// =======================================
// === BT SET TERRAIN DETAIL BLENDMODE ===
// =======================================
void BT_SetTerrainDetailBlendMode(unsigned long terrainid,unsigned char mode)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINDETAILBLENDMODE;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built==true)
		{
		//Set the detail blendmode
			BT_Main.Terrains[terrainid].DetailBlendMode=mode;
			if(BT_Main.Terrains[terrainid].Detailmap)
				SetObjectBlendMap(BT_Main.Terrains[terrainid].ObjectID,1,BT_Main.Terrains[terrainid].Detailmap,11,mode);
		}else{
		//Set the detail blendmode
			BT_Main.Terrains[terrainid].DetailBlendMode=mode;
		}
	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->DetailBlendMode=mode;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ==================================
// === BT SET TERRAIN ENVIRONMENT ===
// ==================================
void BT_SetTerrainEnvironment(unsigned long terrainid,unsigned long image)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINENVIRONMENT;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Set the environmentmap
		BT_Main.Terrains[terrainid].Environmentmap=image;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ==================================
// === BT ADD TERRAIN ENVIRONMENT ===
// ==================================
unsigned long BT_AddTerrainEnvironment(unsigned long terrainid,unsigned long Colour)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_ADDTERRAINENVIRONMENT;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built==true)
		{
			BT_Intern_Error(C_BT_ERROR_CANNOTUSEFUNCTIONONBUILTTERRAIN);
			return 0;
		}else{
		//Add the environment
			return BT_Intern_AddEnvironment(BT_Main.Terrains[terrainid].EnvironmentMap,Colour);
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return 0;
	}
}
// === END FUNCTION ===



// ==========================
// === BT SET TERRAIN LOD ===
// ==========================
void BT_SetTerrainLOD(unsigned long terrainid,unsigned char LODLevels)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINLOD;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check the LOD Levels
		if(LODLevels>0 && LODLevels<=C_BT_MAXLODLEVELS)
		{
		//Set the LODLevels
			BT_Main.Terrains[terrainid].LODLevels=LODLevels;

		//Update in terrain info
			((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->LODLevels=LODLevels;
			
		}else{
			BT_Intern_Error(C_BT_ERROR_INVALIDLODLEVELS);
			return;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ============================
// === BT SET TERRAIN SPLIT ===
// ============================
void BT_SetTerrainSplit(unsigned long terrainid,unsigned long Split)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINSPLIT;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Set the Collision LODLevel
		BT_Main.Terrains[terrainid].LODLevel[0].Split=unsigned short(Split);

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ==================================
// === BT SET TERRAIN DETAIL TILE ===
// ==================================
void BT_SetTerrainDetailTile(unsigned long terrainid,float Tile)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINDETAILTILE;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Set the Collision LODLevel
		BT_Main.Terrains[terrainid].Tile=Tile;

	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->Tile=Tile;

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// =====================================
// === BT SET TERRAIN QUAD REDUCTION ===
// =====================================
void BT_SetTerrainQuadReduction(unsigned long terrainID, bool enabled)
{
	// Set Current function
	BT_Main.CurrentFunction = C_BT_FUNCTION_SETTERRAINQUADREDUCTION;

	// Check that the terrain exists
	if (!BT_Intern_TerrainExist(terrainID))
		return;

	// Check that this is the full version
	#ifndef C_BT_FULLVERSION 
		return;
	#endif

	// Set quad reduction
	BT_Main.Terrains[terrainID].QuadReduction=enabled;
	((BT_TerrainInfo*)BT_Main.Terrains[terrainID].Info)->QuadReduction=enabled;
}
// === END FUNCTION ===



// ====================================
// === BT SET TERRAIN QUAD ROTATION ===
// ====================================
void BT_SetTerrainQuadRotation(unsigned long terrainid,bool Enabled)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINQUADROTATION;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Set Quad rotation
#ifdef C_BT_FULLVERSION
		BT_Main.Terrains[terrainid].QuadRotation=Enabled;
#endif
	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->QuadRotation=Enabled;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ================================
// === BT SET TERRAIN SMOOTHING ===
// ================================
void BT_SetTerrainSmoothing(unsigned long terrainid,unsigned long Amount)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINSMOOTHING;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Limit amount
		if(Amount>100)
			Amount=100;
			
	//Set the Collision LODLevel
		BT_Main.Terrains[terrainid].Smoothing=Amount;

	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->Smoothing=(Amount>0);
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->SmoothAmount=Amount;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ============================
// === BT SET TERRAIN SCALE ===
// ============================
void BT_SetTerrainScale(unsigned long terrainid,float Scale)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINSCALE;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Set the Scale
		BT_Main.Terrains[terrainid].Scale=Scale;

	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->Scale=Scale;

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// =============================
// === BT SET TERRAIN YSCALE ===
// =============================
void BT_SetTerrainYScale(unsigned long terrainid,float YScale)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINYSCALE;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Set the Collision LODLevel
		BT_Main.Terrains[terrainid].YScale=YScale;

	//Update in terrain info
		((BT_TerrainInfo*)BT_Main.Terrains[terrainid].Info)->YScale=YScale;

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ===================================
// === BT SET TERRAIN LOD DISTANCE ===
// ===================================
void BT_SetTerrainLODDistance(unsigned long terrainid,unsigned char LODLevel,float value)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETTERRAINLODDISTANCES;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(LODLevel<BT_Main.Terrains[terrainid].LODLevels)
		{
		//Check if the terrain is built
			if(BT_Main.Terrains[terrainid].Built==true){
				((BT_LODLevelInfo*)BT_Main.Terrains[terrainid].LODLevel[LODLevel].Info)->Distance=value;
				BT_Main.Terrains[terrainid].LODLevel[LODLevel].Distance=value/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE;
			}else{
				BT_Main.Terrains[terrainid].LODLevel[LODLevel].Distance=value;
			}

		}else{

			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return;
		}

	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// ========================
// === BT BUILD TERRAIN ===
// ========================
void BT_BuildTerrain(unsigned long terrainid,unsigned long ObjectID)
{
	BT_BuildTerrain(terrainid,ObjectID,false);
}

void BT_BuildTerrain(unsigned long terrainid,unsigned long ObjectID,bool GenerateTerrain)
{

//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_BUILDTERRAIN;

//Variables
	unsigned long HeightmapImg;
	int size;
	unsigned long Heightmapmemblock;
	unsigned short x;
	unsigned short y;
	unsigned short lx;
	unsigned short ly;
	unsigned long currentvertex;
	unsigned short Sectors;
	unsigned short Sector;
	unsigned short Column;
	unsigned short Row;
	unsigned char LODLevel;
	unsigned long MaxLODLevels;
	BT_Quadmap_Generator Generator;
	unsigned long Excludememblock;
	s_BT_terrain* Terrain;

//Check that the system isnt building
	if(BT_Main.Building)
		BT_Intern_Error(C_BT_ERROR_ALREADYBUILDING);

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Get the terrain pointer
		Terrain=&BT_Main.Terrains[terrainid];

	//Check that the terrain isnt built
		if(Terrain->Built==true)
			BT_Intern_Error(C_BT_ERROR_TERRAINALREADYBUILT);

	//Find the heightmap image
		HeightmapImg=Terrain->Heightmap;

	//Check all the images

		//Heightmap
		if(BT_Intern_ImageExist(HeightmapImg)==false)
		{
			BT_Intern_Error(C_BT_ERROR_HEIGHTMAPDOESNTEXIST);
			return;
		}

		//Texture
		if(BT_Intern_ImageExist(Terrain->Texture)==false)
			Terrain->Texture=0;

		//Detailmap
		if(BT_Intern_ImageExist(Terrain->Detailmap)==false)
			Terrain->Detailmap=0;

		//Environmentmap
		if(BT_Intern_ImageExist(Terrain->Environmentmap)==false)
			Terrain->Environmentmap=0;

		//Exclusionmap
		if(BT_Intern_ImageExist(Terrain->Exclusionmap)==false)
			Terrain->Exclusionmap=0;


	//Open heightmap
		//Find free memblock
		Heightmapmemblock=0;
		do {
			Heightmapmemblock++;
		} while ( !( MemblockExist(Heightmapmemblock) == 0 ) );

		//Make memblock from heightmap
		CreateMemblockFromImage(Heightmapmemblock,HeightmapImg);
		unsigned long* HeightmapMemblockPtr=(unsigned long*)GetMemblockPtr(Heightmapmemblock);


	//Check heightmap size
		//Width and height must be equal
		if(HeightmapMemblockPtr[0]!=HeightmapMemblockPtr[1])
			BT_Intern_Error(C_BT_ERROR_HEIGHTMAPSIZEINVALID);

		//Dimensions must be power of 2 numbers
		size=HeightmapMemblockPtr[0];
		if((size & -size) != size)
			BT_Intern_Error(C_BT_ERROR_HEIGHTMAPSIZEINVALID);

		//Size must be equal to or greater than 64
		if(size<128)
			BT_Intern_Error(C_BT_ERROR_HEIGHTMAPSIZEINVALID);

		//Size must be less or equal to C_BT_MAXTERRAINSIZE
		if(size>C_BT_MAXTERRAINSIZE)
			BT_Intern_Error(C_BT_ERROR_HEIGHTMAPSIZEINVALID);

	//Set heightmap size
		Terrain->Heightmapsize=unsigned short(HeightmapMemblockPtr[0]);
		((BT_TerrainInfo*)Terrain->Info)->Heightmapsize=Terrain->Heightmapsize;

	//Exclusionmap must be same size as heightmap
		if(Terrain->Exclusionmap>0)
		{
			size=ImageWidth(Terrain->Exclusionmap);
			if(size=!ImageWidth(HeightmapImg))
				BT_Intern_Error(C_BT_ERROR_EXCLUSIONMAPSIZEINVALID);
		}

	//Give split an autovalue if not set
		if(Terrain->LODLevel[0].Split==0)
			Terrain->LODLevel[0].Split=Terrain->Heightmapsize/32;

	//Check that split isnt too small
		if(Terrain->Heightmapsize/Terrain->LODLevel[0].Split>64)
			BT_Intern_Error(C_BT_ERROR_SECTORTOOBIG);

	//Work out some variables
		//Terrain size
		Terrain->TerrainSize=(Terrain->Heightmapsize-1)*C_BT_INTERNALSCALE;
		unsigned short Split=Terrain->LODLevel[0].Split;
		for(LODLevel=0;LODLevel<Terrain->LODLevels;LODLevel++)
		{
			//Split
			Terrain->LODLevel[LODLevel].Split=Split;
			Split=Split/2;

			//Sector size*
			Terrain->LODLevel[LODLevel].SectorSize=(Terrain->TerrainSize+C_BT_INTERNALSCALE)/Terrain->LODLevel[LODLevel].Split;

			//Sector detail
			Terrain->LODLevel[LODLevel].SectorDetail=Terrain->Heightmapsize/Terrain->LODLevel[0].Split;
		}

	//Check the LOD levels
		MaxLODLevels=int(log10((float)Terrain->LODLevel[0].Split)/log10((float)2))+1;
		if(Terrain->LODLevels>(char)MaxLODLevels)
		{
			Terrain->LODLevels=(char)MaxLODLevels;
			Terrain->LODLevel=(s_BT_LODLevel*)realloc(Terrain->LODLevel,MaxLODLevels*sizeof(s_BT_LODLevel));
			if(Terrain->LODLevel==nullptr)
				BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		}

	//Load Heightmap

		//Allocate points
		Terrain->HeightPoint=(float*)malloc(Terrain->Heightmapsize*Terrain->Heightmapsize*sizeof(float));
		if(Terrain->HeightPoint==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);

		//Fill the points with data
		currentvertex=0;
		Terrain->ATMode=BT_Main.ATMode;
		for(ly=0;ly<Terrain->Heightmapsize;ly++)
		{
			for(lx=0;lx<Terrain->Heightmapsize;lx++)
			{
				if(Terrain->ATMode)
				{
					x=lx;
					y=Terrain->Heightmapsize-ly-1;
				}else{
					x=lx;
					y=ly;
				}
				Terrain->HeightPoint[currentvertex]=BT_Intern_GetHeightFromColor(HeightmapMemblockPtr[3+x+y*Terrain->Heightmapsize])*Terrain->YScale;
				currentvertex++;
			}
		}

	//Delete memblock
		DeleteMemblock(Heightmapmemblock);

	//Smooth terrain
		// LEENOTE - 070314 - Dave, why did you comment this out on 070314?
		BT_Intern_SmoothTerrain(Terrain);

	//Make exclusion memblock
		if(Terrain->Exclusionmap>0)
		{
			Excludememblock=Heightmapmemblock;
			CreateMemblockFromImage(Excludememblock,Terrain->Exclusionmap);
		}else{
			Excludememblock=0;
		}

	//Make object
		Terrain->ObjectID=ObjectID;
		Terrain->Object=BT_Intern_CreateBlankObject(ObjectID,1);
		BT_Intern_SetupMesh(Terrain->Object->pFrame->pMesh,3,0,GGFVF_DIFFUSE);
		BT_Intern_FinnishObject(ObjectID);
		if(Terrain->Texture!=NULL)
			SetObjectBlendMap(ObjectID,0,Terrain->Texture,10,4);
		if(Terrain->Detailmap!=NULL)
			SetObjectBlendMap(ObjectID,1,Terrain->Detailmap,11,Terrain->DetailBlendMode);

	//Make sectors
		Terrain->Sectors=0;
		Generator.Size=Terrain->LODLevel[0].SectorDetail;
		if(Excludememblock>0)
		{
			Generator.exclusion=(bool*)malloc((Generator.Size+1)*(Generator.Size+1)*sizeof(bool));
			if(Generator.exclusion==nullptr)
				BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		}else{
			Generator.exclusion=NULL;
		}
		Generator.heights=(float*)malloc((Generator.Size+1)*(Generator.Size+1)*sizeof(float));
		if(Generator.heights==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		BT_Intern_StartQuadMapGeneration(Generator);

		for(LODLevel=0;LODLevel<Terrain->LODLevels;LODLevel++)
		{
			s_BT_LODLevel* LODLevelPtr=&Terrain->LODLevel[LODLevel];

		//Scale distance
			LODLevelPtr->Distance=LODLevelPtr->Distance/Terrain->Scale*C_BT_INTERNALSCALE;

		//Allocate sectors
			Sectors=LODLevelPtr->Split*LODLevelPtr->Split;
			Terrain->Sectors+=Sectors;
			LODLevelPtr->Sectors=Sectors;
			LODLevelPtr->Sector=(s_BT_Sector*)malloc(Sectors*sizeof(s_BT_Sector));
			if(LODLevelPtr->Sector==nullptr)
				BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
			memset(LODLevelPtr->Sector, 0, Sectors*sizeof(s_BT_Sector));
			LODLevelPtr->Terrain=Terrain;
			LODLevelPtr->ID=LODLevel;

		//Initialise QuadMap Generator
			Generator.QuadRotation=Terrain->QuadRotation;
			Generator.QuadReduction=Terrain->QuadReduction;
			LODLevelPtr->TileSpan=Terrain->LODLevel[0].Split/LODLevelPtr->Split;
			Generator.TileSize=C_BT_INTERNALSCALE*LODLevelPtr->TileSpan;
			Generator.Optimise=Terrain->MeshOptimisation;
			Generator.LODLevel=LODLevel;
			Generator.TileSpan=1;

		//LODLevel Info
			BT_LODLevelInfo* LODLevelInfo=(BT_LODLevelInfo*)malloc(sizeof(BT_LODLevelInfo));
			if(LODLevelInfo==nullptr)
				BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
			memset ( LODLevelInfo, 0, sizeof(BT_LODLevelInfo) );
			LODLevelInfo->Distance=Terrain->LODLevel[LODLevel].Distance;
			LODLevelInfo->SectorDetail=Terrain->LODLevel[LODLevel].SectorDetail;
			LODLevelInfo->Sectors=Terrain->LODLevel[LODLevel].Sectors;
			LODLevelInfo->SectorSize=Terrain->LODLevel[LODLevel].SectorSize;
			LODLevelInfo->Split=Terrain->LODLevel[LODLevel].Split;
			LODLevelInfo->TileSpan=Terrain->LODLevel[LODLevel].TileSpan;
			LODLevelInfo->InternalData=(void*)LODLevelPtr;
			LODLevelPtr->Info=(void*)LODLevelInfo;
			LODLevelPtr->DBPObject = 0;

			Row=0;
			Column=0;
			for(Sector=0;Sector<Sectors;Sector++)
			{
			//Calculate row and column
				if(Row==LODLevelPtr->Split)
				{
					Column++;
					Row=0;
				}

			//Get sector pointer
				s_BT_Sector* SectorPtr=&LODLevelPtr->Sector[Sector];

			//Fill Sector structure
				SectorPtr->ID=Sector;
				SectorPtr->Column=Column;
				SectorPtr->Row=Row;
				SectorPtr->Pos_x=float(Column*Terrain->LODLevel[LODLevel].SectorSize+0.5*Terrain->LODLevel[LODLevel].SectorSize);
				SectorPtr->Pos_y=0.0f;
				SectorPtr->Pos_z=float(Row*Terrain->LODLevel[LODLevel].SectorSize+0.5*Terrain->LODLevel[LODLevel].SectorSize);
				Generator.Sector=SectorPtr;
				SectorPtr->LODLevel=&Terrain->LODLevel[LODLevel];
				SectorPtr->Terrain=Terrain;
				SectorPtr->DBPObject = 0;
				Generator.RemoveFarX=false;
				if(Row==LODLevelPtr->Split-1)
					Generator.RemoveFarX=true;
				Generator.RemoveFarZ=false;
				if(Column==LODLevelPtr->Split-1)
					Generator.RemoveFarZ=true;

			//Get heights for this sector
				BT_Intern_GetSectorHeights(Terrain,LODLevel,Row,Column,Generator.heights);

				Terrain->LODLevel[LODLevel].Sector[Sector].Excluded=false;
				if(Excludememblock>0)
				{
					if(BT_Intern_GetSectorExclusion(Terrain,LODLevel,Excludememblock,Row,Column,Generator.exclusion)==true)
						SectorPtr->Excluded=true;
				}

				if(Terrain->LODLevel[LODLevel].Sector[Sector].Excluded==false)
				{
				//RTTMS
					Terrain->LODLevel[LODLevel].Sector[Sector].VertexDataRTTMS=(BT_RTTMS_STRUCT*)malloc(sizeof(BT_RTTMS_STRUCT));
					if(Terrain->LODLevel[LODLevel].Sector[Sector].VertexDataRTTMS==nullptr)
						BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
					memset ( Terrain->LODLevel[LODLevel].Sector[Sector].VertexDataRTTMS, 0, sizeof(BT_RTTMS_STRUCT) );
					BT_RTTMS_STRUCTINTERNALS* RTTMSStructInternals=(BT_RTTMS_STRUCTINTERNALS*)malloc(sizeof(BT_RTTMS_STRUCTINTERNALS));
					if(RTTMSStructInternals==nullptr)
						BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
					RTTMSStructInternals->TerrainID=terrainid;
					RTTMSStructInternals->LODLevelID=LODLevel;
					RTTMSStructInternals->SectorID=Sector;
					RTTMSStructInternals->SectorPtr=SectorPtr;
					SectorPtr->VertexDataRTTMS->Internals=(void*)RTTMSStructInternals;

					SectorPtr->QuadMap=(BT_QuadMap*)malloc(sizeof(BT_QuadMap));
					if(SectorPtr->QuadMap==nullptr)
						BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
					memset(SectorPtr->QuadMap,0,sizeof(BT_QuadMap));
					SectorPtr->QuadMap->Generate(Generator);
				}

				//Sector Info
				BT_SectorInfo* SectorInfo=(BT_SectorInfo*)malloc(sizeof(BT_SectorInfo));
				if(SectorInfo==nullptr)
					BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
				memset ( SectorInfo, 0, sizeof(BT_SectorInfo) );
				SectorInfo->Column=Column;
				SectorInfo->Row=Row;
				SectorInfo->Excluded=SectorPtr->Excluded;
				SectorInfo->Pos_x=SectorPtr->Pos_x;
				SectorInfo->Pos_y=0.0f;
				SectorInfo->Pos_z=SectorPtr->Pos_z;
				SectorInfo->WorldMatrix=&SectorPtr->WorldMatrix;
				SectorInfo->InternalData=(void*)SectorPtr;
				SectorPtr->Info=(void*)SectorInfo;

				Row++;
			}
		}

	//Post process quadmap
		for(LODLevel=0;LODLevel<Terrain->LODLevels;LODLevel++)
		{
			Generator.TileSize=C_BT_INTERNALSCALE*Terrain->LODLevel[LODLevel].TileSpan;
			Generator.LODLevel=LODLevel;
			for(Sector=0;Sector<Terrain->LODLevel[LODLevel].Sectors;Sector++)
			{
				s_BT_Sector* SectorPtr=&Terrain->LODLevel[LODLevel].Sector[Sector];
				if(SectorPtr->Excluded==false)
				{
					SectorPtr->QuadMap->Above=SectorPtr->QuadMap->Below=SectorPtr->QuadMap->Left=SectorPtr->QuadMap->Right=0;
					if(SectorPtr->Column!=0)
					{
						s_BT_Sector* OtherSectorPtr=&Terrain->LODLevel[LODLevel].Sector[Sector-Terrain->LODLevel[LODLevel].Split];
						if(OtherSectorPtr->Excluded==false)
							SectorPtr->QuadMap->Above=OtherSectorPtr->QuadMap;
					}
					if(SectorPtr->Row!=Terrain->LODLevel[LODLevel].Split-1)
					{
						s_BT_Sector* OtherSectorPtr=&Terrain->LODLevel[LODLevel].Sector[Sector+1];
						if(OtherSectorPtr->Excluded==false)
							SectorPtr->QuadMap->Right=OtherSectorPtr->QuadMap;
					}
					if(SectorPtr->Row!=0)
					{
						s_BT_Sector* OtherSectorPtr=&Terrain->LODLevel[LODLevel].Sector[Sector-1];
						if(OtherSectorPtr->Excluded==false)
							SectorPtr->QuadMap->Left=OtherSectorPtr->QuadMap;
					}
					if(SectorPtr->Column!=Terrain->LODLevel[LODLevel].Split-1)
					{
						s_BT_Sector* OtherSectorPtr=&Terrain->LODLevel[LODLevel].Sector[Sector+Terrain->LODLevel[LODLevel].Split];
						if(OtherSectorPtr->Excluded==false)
							SectorPtr->QuadMap->Below=OtherSectorPtr->QuadMap;
					}
					
					SectorPtr->QuadMap->CalculateNormals();
				}
			}
		}

	//Delete exclusion
		if(Excludememblock>0)
			DeleteMemblock(Excludememblock);
		if(Generator.exclusion!=NULL)
			free(Generator.exclusion);

	//Load Environment map
		if(Terrain->Environmentmap>0)
		{
		//Convert Image into Memblock (recycle heightmap memblock)
			CreateMemblockFromImage(Heightmapmemblock,Terrain->Environmentmap);

		//Create the environment map
			BT_Intern_CreateEnvironmentMap(Terrain->EnvironmentMap,ImageWidth(Terrain->Environmentmap),ImageHeight(Terrain->Environmentmap),(unsigned long*)(GetMemblockPtr(Heightmapmemblock)+4));

		//Delete the Memblock
			DeleteMemblock(Heightmapmemblock);
		}

	//Allocate quadtree
		Terrain->QuadTree=BT_Intern_AllocateQuadTree(Terrain);

	//Create LOD Map
		Terrain->LODMap=(s_BT_LODMap**)malloc(Terrain->LODLevel[0].Split*sizeof(s_BT_LODMap*));
		if(Terrain->LODMap==nullptr)
			BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
		memset ( Terrain->LODMap, 0, Terrain->LODLevel[0].Split*sizeof(s_BT_LODMap*) );
		for(unsigned long i=0;i<Terrain->LODLevel[0].Split;i++)
		{
			Terrain->LODMap[i]=(s_BT_LODMap*)malloc(Terrain->LODLevel[0].Split*sizeof(s_BT_LODMap));
			if(Terrain->LODMap[i]==nullptr)
				BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
			memset(Terrain->LODMap[i],0,Terrain->LODLevel[0].Split*sizeof(s_BT_LODMap));
		}

		//Vertex Declaration
		#ifdef DX11

		// VD Array (critical that shaders are loaded prior to this step, unlike DX9)
		int iLayoutSize = 4;
		D3D11_INPUT_ELEMENT_DESC* pLayout = new D3D11_INPUT_ELEMENT_DESC [ iLayoutSize ];
		D3D11_INPUT_ELEMENT_DESC layout [ ] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		std::memcpy ( pLayout, layout, sizeof ( layout ) );

		// Get FIRST vertex shader input layout found in shader
		ID3DBlob* pBlob = g_sShaders[SHADERSTERRAINBASIC].pBlob;
		DWORD tIndex = 0;
		ID3DX11EffectTechnique* tech = NULL;
		while((tech = g_sShaders[SHADERSTERRAINBASIC].pEffect->GetTechniqueByIndex(tIndex++))->IsValid())
		{
			DWORD pIndex = 0;
			ID3DX11EffectPass* pass = NULL;
			while((pass = tech->GetPassByIndex(pIndex++))->IsValid())
			{
				D3DX11_PASS_SHADER_DESC vs_desc;
				pass->GetVertexShaderDesc(&vs_desc);
				D3DX11_EFFECT_SHADER_DESC s_desc;
				vs_desc.pShaderVariable->GetShaderDesc(0, &s_desc);
				HRESULT hr = m_pD3D->CreateInputLayout ( pLayout, iLayoutSize, s_desc.pBytecode, s_desc.BytecodeLength, &Terrain->VertexDeclaration );
				break;
			}
			if ( Terrain->VertexDeclaration != NULL ) break;
		}
		SAFE_DELETE_ARRAY(pLayout);

		#else
		//VD Array
		IGGDevice* D3DDevice=m_pD3D;
		const GGVERTEXELEMENT VD[5] =
		{
			{0, 0,  GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_POSITION,0},
			{0, 12, GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_NORMAL,  0},
			{0, 24, GGDECLTYPE_FLOAT2, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD,0},
			{0, 32, GGDECLTYPE_FLOAT2, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD,1},
			GDECL_END()
		};
		//Make Vertex Declaration
		D3DDevice->CreateVertexDeclaration(VD,&Terrain->VertexDeclaration);
		#endif

	//Cleanup
		BT_Intern_EndQuadMapGeneration();
		free(Generator.heights);
		free(Terrain->HeightPoint);

	//Terrain Info
		((BT_TerrainInfo*)Terrain->Info)->Built=true;
		((BT_TerrainInfo*)Terrain->Info)->Generated=GenerateTerrain;
		((BT_TerrainInfo*)Terrain->Info)->ATMode=Terrain->ATMode;
		((BT_TerrainInfo*)Terrain->Info)->Sectors=Terrain->Sectors;
		((BT_TerrainInfo*)Terrain->Info)->TerrainSize=Terrain->TerrainSize/C_BT_INTERNALSCALE*Terrain->Scale;
		((BT_TerrainInfo*)Terrain->Info)->InternalData=(void*)Terrain;
		((BT_TerrainInfo*)Terrain->Info)->DBPObjectPtr=(void*)Terrain->Object;

	//Initialise build
		BT_Main.Building=1;
		BT_Main.BuildType=0;
		BT_Main.CurrentBuildTerrain=Terrain;
		BT_Main.CurrentBuildSector=0;
		BT_Main.CurrentBuildRow=0;
		BT_Main.CurrentBuildColumn=0;
		BT_Main.CurrentBuildTerrainSector=0;
		Terrain->Built=1;

	//Build
		if(GenerateTerrain==true)
		{
			unsigned long tempbuildstep=BT_Main.buildstep;
			BT_Main.buildstep=0;
			int null=BT_ContinueBuild();
			BT_Main.buildstep=tempbuildstep;
		}

	//Add RTTMS update handler
		BT_RTTMS_AddUpdateHandler(terrainid,BT_Intern_RTTMSUpdateHandler);

	//Fix LOD seams
		BT_Intern_FixLODSeams(Terrain);

	//Set autocam
		// TODO: Stick some proper values here
		SetAutoCam(0.0,0.0,0.0,0.0);
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}
}
// === END FUNCTION ===



// =========================
// === BT CONTINUE BUILD ===
// =========================
int BT_ContinueBuild()
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_CONTINUEBUILD;

//Variables
	int Progress=0;
	int numsectorstomake;
	int I;
	float Progress_flt;

//Check that its building
	if(BT_Main.Building==true)
	{
		if(BT_Main.buildstep>BT_Main.CurrentBuildTerrain->Sectors)
			BT_Main.buildstep=0;

	//Work out the amount of sectors which need to be made
		if(BT_Main.buildstep==0)
		{
			numsectorstomake=BT_Main.CurrentBuildTerrain->Sectors-BT_Main.CurrentBuildTerrainSector;
		}else{
			numsectorstomake=BT_Main.buildstep;
		}
		if(BT_Main.CurrentBuildTerrainSector+numsectorstomake>BT_Main.CurrentBuildTerrain->Sectors)
			numsectorstomake=BT_Main.CurrentBuildTerrain->Sectors-BT_Main.CurrentBuildTerrainSector;

	//Loop
		for(I=0;I<numsectorstomake;I++)
		{
			BT_Intern_ContinueBuild();

		}

		if(BT_Main.CurrentBuildTerrainSector==BT_Main.CurrentBuildTerrain->Sectors)
		{
			Progress=-1;
			BT_Main.Building=0;
			BT_Main.CurrentBuildTerrain->Generated=true;
			((BT_TerrainInfo*)BT_Main.CurrentBuildTerrain->Info)->Generated=true;
			BT_Main.CurrentBuildTerrain=NULL;
			BT_Main.CurrentBuildLODLevel=0;
			BT_Main.CurrentBuildSector=0;
			BT_Main.CurrentBuildRow=0;
			BT_Main.CurrentBuildColumn=0;
			return -1;
		}

	//Get the progress
		Progress_flt=float((float(BT_Main.CurrentBuildTerrainSector)/float(BT_Main.CurrentBuildTerrain->Sectors))*100.0);
		Progress=int(Progress_flt);
	}
	return Progress;
}
// === END FUNCTION ===



// ========================
// === BT TERRAIN EXIST ===
// ========================
unsigned long BT_TerrainExist(unsigned long TerrainID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_TERRAINEXIST;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(TerrainID))
	{
		return 1;
	}

//Return false
	return 0;
}
// === END FUNCTION ===



// =========================
// === BT DELETE TERRAIN ===
// =========================
void BT_DeleteTerrain(unsigned long TerrainID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_DELETETERRAIN;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(TerrainID))
	{
	//Delete the terrain
		BT_Intern_DeleteTerrain(TerrainID,true);
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return;
	}

}
// === END FUNCTION ===

void BT_ForceTerrainTechnique(unsigned long QualityTechniqueMode)
{
	g_iQualityTechniqueMode = QualityTechniqueMode;
}

// ===========================
// === BT GET GROUND HEIGHT ==
// ===========================
float BT_GetGroundHeight(unsigned long terrainid,float x,float z,bool Round)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETGROUNDHEIGHT;

//Variables
	float Height=0.0;

//Check that the terrain exists and has been built
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built)
		{
		//Get the height
			Height=BT_Intern_GetPointHeight(&BT_Main.Terrains[terrainid],x/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE,z/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE,0,Round);
			return Height;
		}
	}
	return Height;
}

float BT_GetGroundHeight(unsigned long terrainid,float x,float z)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETGROUNDHEIGHT;

//Variables
	float Height=0.0;

//Check that the terrain exists and has been built
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built)
		{
		//Get the height
			Height=BT_Intern_GetPointHeight(&BT_Main.Terrains[terrainid],x/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE,z/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE,0,0);
			return Height;
		}
	}
	return Height;
}

// ===========================
// === BT GET TERRAIN SIZE ===
// ===========================
float BT_GetTerrainSize(unsigned long terrainid)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETTERRAINSIZE;

//Variables
	float Size=0.0;

//Check that the terrain exists and has been built
	if(BT_Intern_TerrainExist(terrainid))
	{
		if(BT_Main.Terrains[terrainid].Built)
		{
		//Get the size
			Size=BT_Main.Terrains[terrainid].TerrainSize/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
			return Size;
		}
	}
	return Size;
}



// =============================
// === BT GET POINT EXCLUDED ===
// =============================
unsigned long BT_GetPointExcluded(unsigned long terrainid,float x,float z)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETPOINTEXCLUDED;

//Variables
	float Height=0.0;

//Check that the terrain exists and has been built
	if(BT_Intern_TerrainExist(terrainid)==true)
	{
		if(BT_Main.Terrains[terrainid].Built==true)
		{
		//Return if it is excluded
			return BT_Intern_GetPointExcluded(&BT_Main.Terrains[terrainid],x/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE,z/BT_Main.Terrains[terrainid].Scale*C_BT_INTERNALSCALE);
		}
	}
	return false;
}
// === END FUNCTION ===



// ================================
// === BT GET POINT ENVIRONMENT ===
// ================================
unsigned long BT_GetPointEnvironment(unsigned long terrainid,float x,float z)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETPOINTENVIRONMENT;

//Variables
	float Height=0.0;

//Check that the terrain exists and has been built
	if(BT_Intern_TerrainExist(terrainid)==true)
	{
		if(BT_Main.Terrains[terrainid].Built==true)
		{
		//Check the range
			if(	x>0 && BT_Main.Terrains[terrainid].TerrainSize/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale>x &&
				z>0 && BT_Main.Terrains[terrainid].TerrainSize/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale>z)
			{
			//Transform coordinates to fit on environment map
				x=(x*BT_Main.Terrains[terrainid].EnvironmentMap->Width)/BT_Main.Terrains[terrainid].Heightmapsize;
				z=(z*BT_Main.Terrains[terrainid].EnvironmentMap->Width)/BT_Main.Terrains[terrainid].Heightmapsize;

			//Return the points environment
				return BT_Intern_GetPointEnvironment(BT_Main.Terrains[terrainid].EnvironmentMap,unsigned long(x/BT_Main.Terrains[terrainid].Scale),unsigned long(z/BT_Main.Terrains[terrainid].Scale));
			}else{
				return false;
			}
		}
	}
	return false;
}
// === END FUNCTION ===



// ======================
// === BT GET VERSION ===
// ======================
DWORD BT_GetVersion()
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETVERSION;

//Variables
	LPSTR Version=BT_VERSION;

	return (DWORD)Version;
}

// =========================
// === BT SET BUILD STEP ===
// =========================
void BT_SetBuildStep(unsigned long step)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETBUILDSTEP;

//Set build step
	BT_Main.buildstep=step;
}
// === END FUNCTION ===



// ======================
// === BT SET AT MODE ===
// ======================
void BT_SetATMode(bool ATMode)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETATMODE;

//Set AT mode
	BT_Main.ATMode=ATMode;
}
// === END FUNCTION ===



// =============================
// === BT ENABLE AUTO RENDER ===
// =============================
void BT_EnableAutoRender(bool AutoRender)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_ENABLEAUTORENDER;

//Set AT mode
	BT_Main.AutoRender=AutoRender;
}
// === END FUNCTION ===



// ========================
// === BT GET STATISTIC ===
// ========================
unsigned long BT_GetStatistic(unsigned long code)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSTATISTIC;

//Find stat code and return it
	if(code==1)
		return BT_Main.DrawPrimitiveCount;

	if(code==2)
		return BT_Main.DrawCalls;

	if(code==3)
		return BT_Main.CullChecks;

	return 0;
}
// === END FUNCTION ===



// ================================
// === BT GET TERRAIN OBJECT ID ===
// ================================
unsigned long BT_GetTerrainObjectID(unsigned long terrainid)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETOBJECTID;

//Check if terrain exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Return objectID
		return BT_Main.Terrains[terrainid].ObjectID;
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return 0;
	}
}
// === END FUNCTION ===



// =============================
// === BT MAKE SECTOR OBJECT ===
// =============================
void BT_MakeSectorObject(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID,unsigned long ObjectID)
{
	//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_MAKESECTOROBJECT;

	//Check that the Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
		//Check that the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
			//Check that the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
				//Check that the sector is not excluded
				if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Excluded==0)
				{
					// Add or Wipe
					if ( ObjectID==0 )
					{
						BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].DBPObject = 0;
						BT_Main.Terrains[terrainid].LODLevel[LODLevel].DBPObject=0;
					}
					else
					{
						//Generate the object
						sObject* Object=BT_Intern_CreateBlankObject(ObjectID,1);

						//Generate the mesh
						BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].QuadMap->GenerateDBPMesh(Object->pFrame->pMesh);

						// for now only reference the actual LOD1 sector as we can guarentee this clipping is perfect.
						BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].DBPObject = Object;

						//Finnish the object
						Object->bExcluded=true;
						BT_Intern_FinnishObject(ObjectID);

						//Position and scale object to terrains position and scale
						Object->position.vecPosition.x=BT_Main.Terrains[terrainid].Object->position.vecPosition.x+BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_x/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
						Object->position.vecPosition.y=BT_Main.Terrains[terrainid].Object->position.vecPosition.y+BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_y/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
						Object->position.vecPosition.z=BT_Main.Terrains[terrainid].Object->position.vecPosition.z+BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_z/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
						Object->position.vecScale=BT_Main.Terrains[terrainid].Object->position.vecScale;

						//Record object ref (for later collision mesh updating when raise terrain)
						BT_Main.Terrains[terrainid].LODLevel[LODLevel].DBPObject=Object;
					}

				}else{
					BT_Intern_Error(C_BT_ERROR_SECTORISEXCLUDED);
				}
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
	}
}
// === END FUNCTION ===



// ==============================
// === BT MAKE TERRAIN OBJECT ===
// ==============================
void BT_MakeTerrainObject(unsigned long terrainid,unsigned long LODLevel,unsigned long ObjectID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_MAKETERRAINOBJECT;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Variables
			unsigned long SectorID=0;

		//Generate the object
			sObject* Object=BT_Intern_CreateBlankObject(ObjectID,BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors);

		//Loop through mesh list
			sFrame* CurrentFrame=Object->pFrame;
			for(SectorID=0;SectorID<BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors;SectorID++){
			//Check that the sector is not excluded
				if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Excluded==0){
				//Generate the mesh
					BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].QuadMap->GenerateDBPMesh(CurrentFrame->pMesh);

				//Set LODLevelFrame to sector
					BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].LODLevelObjectFrame=CurrentFrame;

				//Position mesh
					CurrentFrame->vecOffset.x=BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_x/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
					CurrentFrame->vecOffset.y=BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_y/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
					CurrentFrame->vecOffset.z=BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_z/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
					
				//Get next frame and link it properly
					CurrentFrame->pSibling=CurrentFrame->pChild;
					CurrentFrame->pChild=0;
					CurrentFrame=CurrentFrame->pSibling;
				}
			}

		//Position and scale object to terrains position and scale
			Object->position.vecPosition=BT_Main.Terrains[terrainid].Object->position.vecPosition;
			Object->position.vecScale=BT_Main.Terrains[terrainid].Object->position.vecScale;
			BT_Main.Terrains[terrainid].LODLevel[LODLevel].DBPObject=Object;

		//Finnish the object
			Object->bExcluded=true;
			BT_Intern_FinnishObject(ObjectID);
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
	}
}
// === END FUNCTION ===



// =================================
// === BT UPADTE TERRAIN OBJECTS ===
// =================================
void BT_UpdateTerrainObjects(unsigned long terrainid)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_MAKETERRAINOBJECT;

//Check that the terrain exists
	if(BT_Intern_TerrainExist(terrainid)){
	//Check that the terrain is built
		if(BT_Main.Terrains[terrainid].Built){
		//Get Terrain ptr
			s_BT_terrain* TerrainPtr=&BT_Main.Terrains[terrainid];

		//Loop through LOD Levels
			for(unsigned long LODLevel=0;LODLevel<TerrainPtr->LODLevels;LODLevel++){
			//Get LODLevel ptr
				s_BT_LODLevel* LODLevelPtr=&TerrainPtr->LODLevel[LODLevel];

			//Loop through sectors
				for(unsigned long Sector=0;Sector<LODLevelPtr->Sectors;Sector++){
				//Get Sector ptr
					s_BT_Sector* SectorPtr=&LODLevelPtr->Sector[Sector];

				//Check that the sector has its own object
					if(SectorPtr->DBPObject!=0){
					//Update object
						SectorPtr->DBPObject->position.vecPosition.x=BT_Main.Terrains[terrainid].Object->position.vecPosition.x+SectorPtr->Pos_x/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
						SectorPtr->DBPObject->position.vecPosition.y=BT_Main.Terrains[terrainid].Object->position.vecPosition.y+SectorPtr->Pos_y/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
						SectorPtr->DBPObject->position.vecPosition.z=BT_Main.Terrains[terrainid].Object->position.vecPosition.z+SectorPtr->Pos_z/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
						SectorPtr->DBPObject->position.vecScale=BT_Main.Terrains[terrainid].Object->position.vecScale;
					}

				//Check if the LODLevel has an object
					if(LODLevelPtr->DBPObject!=0){
					//Update object
						LODLevelPtr->DBPObject->position.vecPosition=BT_Main.Terrains[terrainid].Object->position.vecPosition;
						LODLevelPtr->DBPObject->position.vecScale=BT_Main.Terrains[terrainid].Object->position.vecScale;
					}
				}
			}
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
	}
}
// === END FUNCTION ===



// ================================
// === BT GET SECTOR POSITION X ===
// ================================
float BT_GetSectorPositionX(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORPOSITIONX;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Check if the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
			//Return X Position
				float PosX=BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_x/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
				return PosX;
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
				return NULL;
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
}

// ================================
// === BT GET SECTOR POSITION Y ===
// ================================
float BT_GetSectorPositionY(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORPOSITIONY;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Check if the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
			//Return Y Position
				float PosY=BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_y/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
				return PosY;
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
				return NULL;
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
}

// ================================
// === BT GET SECTOR POSITION Z ===
// ================================
float BT_GetSectorPositionZ(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORPOSITIONZ;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Check if the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
			//Return Z Position
				float PosZ=BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Pos_z/C_BT_INTERNALSCALE*BT_Main.Terrains[terrainid].Scale;
				return PosZ;
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
				return NULL;
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
}

// ===========================
// === BT GET SECTOR COUNT ===
// ===========================
unsigned long BT_GetSectorCount(unsigned long TerrainID,unsigned long LODLevel)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORCOUNT;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(TerrainID))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[TerrainID].LODLevels>LODLevel)
		{
			return BT_Main.Terrains[TerrainID].LODLevel[LODLevel].Sectors;
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
	return NULL;
}
// === END FUNCTION ===



// ===========================
// === BT GET SECTOR SIZE ===
// ===========================
unsigned long BT_GetSectorSize(unsigned long TerrainID,unsigned long LODLevel)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORSIZE;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(TerrainID))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[TerrainID].LODLevels>LODLevel)
		{
			float Size=BT_Main.Terrains[TerrainID].LODLevel[LODLevel].SectorSize/C_BT_INTERNALSCALE*BT_Main.Terrains[TerrainID].Scale;
			return *(DWORD*)&Size;
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
	return NULL;
}

// ==============================
// === BT GET SECTOR EXCLUDED ===
// ==============================
unsigned long BT_GetSectorExcluded(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTOREXCLUDED;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Check if the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
			//Return excluded
				return BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Excluded;
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
				return NULL;
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
}
// === END FUNCTION ===



// =========================
// === BT GET SECTOR ROW ===
// =========================
unsigned long BT_GetSectorRow(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORROW;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Check if the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
			//Return row
				return BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Row;
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
				return NULL;
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
}
// === END FUNCTION ===



// =============================
// === BT GET SECTOR COLLUMN ===
// =============================
unsigned long BT_GetSectorCollumn(unsigned long terrainid,unsigned long LODLevel,unsigned long SectorID)
{
//Set Current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_GETSECTORCOLLUMN;

//Check if Terrain Exists
	if(BT_Intern_TerrainExist(terrainid))
	{
	//Check if the LOD level exists
		if(BT_Main.Terrains[terrainid].LODLevels>LODLevel)
		{
		//Check if the Sector exists
			if(BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sectors>SectorID)
			{
			//Return row
				return BT_Main.Terrains[terrainid].LODLevel[LODLevel].Sector[SectorID].Column;
			}else{
				BT_Intern_Error(C_BT_ERROR_SECTORDOESNTEXIST);
				return NULL;
			}
		}else{
			BT_Intern_Error(C_BT_ERROR_LODLEVELDOESNTEXIST);
			return NULL;
		}
	}else{
		BT_Intern_Error(C_BT_ERROR_TERRAINDOESNTEXIST);
		return NULL;
	}
}
// === END FUNCTION ===



// =============================
// === BT SET CURRENT CAMERA ===
// =============================
void BT_SetCurrentCamera(unsigned long CameraID)
{
//Set current function
	BT_Main.CurrentFunction=C_BT_FUNCTION_SETCURRENTCAMERA;

//Check if the camera exists
	if(GetCameraInternalData(CameraID)!=NULL)
	{
	//Add to queue
		BT_Intern_AddToInstructionQueue(C_BT_INSTRUCTION_SETCURRENTCAMERA,(char)CameraID);
	}

}
