#ifndef _H_GGTREES
#define _H_GGTREES

// Force update
#include <stdint.h>
#include "../../../../WickedEngineDX12/WickedEngine/WickedEngine.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiGraphicsDevice.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiScene.h"

#define GGTREES_PAINT_SPRAY         0
#define GGTREES_PAINT_ADD           1
#define GGTREES_PAINT_REMOVE        2
#define GGTREES_PAINT_MOVE          3
#define GGTREES_PAINT_SPRAY_REMOVE  4
#define GGTREES_PAINT_SCALE         5

struct sUndoSysEventTreeMove;

namespace GGTrees
{
	extern bool gg_far_tree_pass;   // GGMAX 2.96: DX11-style distant-tree billboard pass
	extern uint32_t g_ftDrawCalls, g_ftInstances, g_ftChunksTotal, g_ftChunksWithIn, g_ftFrustumKills, g_ftEnterCount;
	extern uint32_t g_ftAtlasSlices;
	// GGMAX 2.95: far-tree billboard gate diagnostics (harness SET_FARTREES)
	void GGTrees_GetFarTreeStats(int* proxyCount, int* proxiesShown, int* candidates, int* poolBuilt, int* poolSize, float* cutoffDist2);
	void GGTrees_GetFarTreeRange(int* validProxies, float* nearestChunk, float* farthestChunk);
	// GGMAX Tier A: allocate the dead legacy tree atlases only if the legacy draw path wakes up.
	void GGTrees_EnsureLegacyTexArrays();

	struct GGTreesParams
	{
		int draw_enabled = 0;
		int draw_shadows = 1;
		// Number of shadow cascades that receive tree shadows (0 = none, 5 = all).
		// DX12 default 5 (DX11 defaulted 3): the merged billboard shadow proxies
		// make island-wide tree shadows cheap, and the far cascades are what sell
		// the DX12 look. Quality presets (GGTrees_SetPerformanceMode) still dial
		// this down on LOW/MED.
		int tree_shadow_range = 5;

		int paint_mode = GGTREES_PAINT_SPRAY;
		uint64_t paint_tree_bitfield = 0x00100000; // 1 bit per tree, default pine
		int paint_density = 65; // 0 to 100
		float water_dist = 10;

		int paint_scale_random_low = 10; // 0 to 100
		int paint_scale_random_high = 245; // 0 to 100

		float lod_dist = 3000;
		float lod_dist_shadow = 4000; // USER-TUNED 2026-07-28 (live in test game): best shadow transition, acceptable cost

		int hide_until_update = 0;

	};

	// these values must not be modified outside the tree module
	struct GGTreesInternalParams 
	{
		int prevMouseLeft = 0;
		int mouseLeftPressed = 0;
		int mouseLeftState = 0;
		int mouseLeftReleased = 0;
		int mouseY = 0;
		float minTotalHeight = 0;
		float maxTotalHeight = 0;
		uint32_t tree_selected = 0xFFFFFFFF;
		int scaleMouseYStart = 0;
		int scaleStart = 0;
		uint32_t treeChunkUpdate = 0;
	};

	extern GGTreesParams ggtrees_global_params; // modify this anywhere
	extern int g_treePoolStressFrames; // debug: force pool rescan for N frames (SET_TREES stress)
	extern uint32_t g_treePoolSize;    // perf knob: effective tree pool size (nearest-N drawn); applies on next pool setup (level reload). Lower = fewer trees + big CPU win.

	extern int ggtrees_draw_enabled;

	struct GGTreePoint
	{
		float x;
		float y;
		float z;
		float scale;
	};

	int GGTrees_GetClosest( float x, float z, float radius, GGTreePoint** pOutPoints ); // returns the number of trees in pOutPoints, pOutPoints must be undefined it will be created
	int GGTrees_RayCast( RAY pickRay, float maxDist, float* outDist, uint32_t* treeID ); // returns 1 if hit, 0 if not. If hit then treeID will be populated
	void GGTrees_SetTreePosition( uint32_t treeID, float x, float z );
	
	uint32_t GGTrees_GetDataSize(); // number of floats required in data array
	int GGTrees_GetData( float* data ); // data must be allocated with GGTrees_GetSculptDataSize() floats, returns 1 on success
	int GGTrees_SetData( float* data ); // number of floats must be equal to GGTrees_GetSculptDataSize(), returns 1 on success
	int GGTrees_GetSnapshot(uint8_t* data);

	void GGTrees_SetPerformanceMode( uint32_t mode );
	void GGTrees_Delete_Trees(float pickX, float pickZ, float radius);

	void GGTrees_Init();
	void GGTrees_UpdateFrustumCulling( wiScene::CameraComponent* camera );
	void GGTrees_Update( float camX, float camY, float camZ, wiGraphics::CommandList cmd, bool bRenderTargetFocus);
	void GGTrees_Update_Painting( RAY ray );
	int GGTrees_UsingBrush();
	uint32_t GGTrees_GetNumTypes();
	uint32_t GGTrees_GetNumHighDetail();
	void GGTrees_ChangeDensity(int density);
	void GGTrees_RepopulateInstances();
	int GGTrees_UpdateInstances(int accurate);
	void GGTrees_HideAll();
	void GGTrees_DebugDumpPool( const char* path ); // DIAG: pool census + orphan detector (reload-corruption hunt)
	void GGTrees_DeselectHighlightedTree(void);
	void GGTrees_LockVisibility();

	const char* GGTrees_GetTextureName( uint32_t index );
	float GGTrees_GetImageScale( uint32_t index );

	void GGTrees_BindTreeMap( int slot, wiGraphics::CommandList cmd );

	void GGTrees_UpdateFlatArea( int mode, int type, float x, float z, float sx, float sz, float angle );
	void GGTrees_RestoreAllFlattened();

	bool GGTrees_GetDefaultDataV2(char *filename);

	// Phase 5: Colored cylinder tree placeholders on the new Wicked terrain.
	// Setup is lazy (first WickedUpdate call) so it happens after the scene is live.
	void GGTrees_WickedInit();
	void GGTrees_WickedUpdate();
	void GGTrees_WickedShutdown();
}

#endif // _H_GGTREES