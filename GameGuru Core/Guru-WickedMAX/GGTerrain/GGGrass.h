#ifndef _H_GGRASS
#define _H_GGRASS

#include <stdint.h>
#include "../../../../WickedEngineDX12/WickedEngine/WickedEngine.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiGraphicsDevice.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiScene.h"

#include "Shaders/GGGrassConstants.hlsli"

#define GGGRASS_PAINT_SPRAY         0
#define GGGRASS_PAINT_SPRAY_REMOVE  1
#define GGGRASS_PAINT_SPRAY_RESTORE 2

#define GGGRASS_INITIAL_LOD_DIST 1500

// Total palette slot budget: 22 stock (0..21) + up to 42 custom (22..63) = 64. The 64 cap is
// tied to gggrass_global_params.paint_type being a uint64_t bitmask (1 bit per selected palette
// slot). Widening paint_type to uint64_t[2] would let this grow to 128 (matches Types.h
// sGrassTextures[128] cap) — future work. Byte-encoding limit is 125 real_types either way.
// Custom slot i maps to real_type = i + 24 so byte = i + 26. At slot 63, byte = 89 — well below
// the 0x80 flatten flag.
#define GGGRASS_MAX_PALETTE_SLOTS 64

// Real type index reserved for the first custom slot. Custom slot i (>=22) maps to real_type = i + 24.
// Chosen so real_types 0..45 stay reserved for the stock grassFiles[] table entries with no risk of
// collision if custom is painted in the same chunk as stock.
#define GGGRASS_CUSTOM_REAL_TYPE_BASE 46
#define GGGRASS_CUSTOM_SLOT_BASE      22

// Total real_type index space that Wicked-side arrays must cover: 46 stock + (max_slots - stock_slots)
// custom = 46 + 42 = 88 at cap 64 slots. Wicked appearance / material / per-chunk-entity arrays are
// sized to this so a custom slot's real_type never lands out-of-range.
#define GGGRASS_TOTAL_REAL_TYPES ( GGGRASS_CUSTOM_REAL_TYPE_BASE + ( GGGRASS_MAX_PALETTE_SLOTS - GGGRASS_CUSTOM_SLOT_BASE ) )

struct sUndoSysEventGrass;

namespace GGGrass
{
	struct GGGrassParams
	{
		int draw_enabled = 1;
		int shadow_range = 0; // 0=no shadows, 1-5=shadow cascade cutoff

		uint64_t paint_type = 0;  // 1 bit per grass type
		int paint_material = 0; // 0=auto, otherwise force the use of the grass for that material
		int paint_mode = GGGRASS_PAINT_SPRAY;
		int paint_density = 100; // 0 to 100

		float max_height = 30000;
		float min_height = -1000;

		float max_height_underwater = 1000;
		float min_height_underwater = -7000;

		float lod_dist = GGGRASS_INITIAL_LOD_DIST;
		int simplePBR = 0;
		float grass_scale = 40;
	};

	// these values must not be modified outside the tree module
	struct GGGrassInternalParams 
	{
		int prevMouseLeft = 0;
		int mouseLeftPressed = 0;
		int mouseLeftState = 0;
		int mouseLeftReleased = 0;
		float prevLodDist = GGGRASS_INITIAL_LOD_DIST;
	};

	extern GGGrassParams gggrass_save_params;
	extern GGGrassParams gggrass_global_params; // modify this anywhere

	void GGGrass_Init_Textures(LPSTR pRemoteGrassPath);
	void GGGrass_Init();
	void GGGrass_Update( wiScene::CameraComponent* camera, wiGraphics::CommandList cmd, bool bRenderTargetFocus );
	void GGGrass_Update_Painting( RAY ray );
	int GGGrass_UsingBrush();
	void GGGrass_BindGrassArray( uint32_t slot, wiGraphics::CommandList cmd );
	// GGMAX Tier A: allocate the dead legacy blade atlas only if the legacy draw path wakes up.
	void GGGrass_EnsureLegacyTexArray();
	void GGGrass_BindGrassMap( int slot, wiGraphics::CommandList cmd );

	// (Re)create texGrassMap from the current pGrassMap contents. Called by the paint stroke path
	// (~10 Hz mid-stroke + on release) and by GGGrass_ScanRegion's auto-resolve consumer whenever
	// pGrassMap has been rewritten out-of-band. 16 MB upload — cheap enough at editor cadence but
	// callers should batch (one call per frame, not per-cell).
	void GGGrass_UploadGrassMap();

	// Stage 3 Option B: lets the Wicked hair simulate CS sample the GG paint mask per-strand
	// so blade visibility matches the painted footprint. Returns nullptr until the first
	// GGGrass_UploadGrassMap() (init time). The handle stays valid for the lifetime of the
	// grass system; the underlying GPU texture is recreated by each upload, so the *pointer*
	// is stable but the texture contents change every paint stroke (~10 Hz).
	const wi::graphics::Texture* GGGrass_GetMapTexture();

	// Stage B.4: scan grass-map cells in [minX..maxX] x [minZ..maxZ] world XZ. Sets
	// typesSeen[t] = true for each painted (non-flattened) cell whose type maps to t (0-based,
	// matching GGGrass type indices). Caller passes a bool array sized to hold all real_types —
	// stock (0..45) + custom (46..GGGRASS_CUSTOM_REAL_TYPE_BASE + numCustomSlots - 1).
	// Used by the Wicked-side hair-entity manager to decide which per-(chunk, type) entities
	// to create/destroy; per-cell visibility WITHIN the chunk is handled in the simulate CS.
	//
	// Also performs the DX11 GGGrass_UpdateInstances "auto-resolve" step for encoded value 1:
	// cells painted with Match Terrain Color get rewritten in-place with a terrain-appropriate
	// real_type (seaweed 43 if underwater, else GGGrass_GetRealIndex(material_at_cell, 0)).
	// If any cell is rewritten, GGGrass_TakePendingMapUpload() will return true so the caller
	// can schedule a texGrassMap re-upload once per frame.
	void GGGrass_ScanRegion( float minX, float minZ, float maxX, float maxZ, bool* typesSeen );

	// Take-and-clear: true if ScanRegion rewrote at least one pGrassMap cell since the last call.
	// The Wicked hair simulate CS samples texGrassMap for per-strand visibility, so any in-place
	// rewrite must be followed by a GGGrass_UploadGrassMap() or the shader keeps sampling stale
	// pre-resolve bytes.
	bool GGGrass_TakePendingMapUpload();

	// Take-and-clear: true if pGrassMap has been rewritten wholesale (e.g. by GGGrass_AddAll or
	// GGGrass_RemoveAll) since the last call. The Wicked-side hair entity cache is per-chunk and
	// won't notice a bulk write on its own — it needs to drop every existing entity and rebuild
	// from the new map. Consumer should call ForceGrassRebuild() (or equivalent) on true.
	bool GGGrass_TakeFullRebuildPending();

	// Global default water height (`g.gdefaultwaterheight`). Cheap accessor exposed here so the
	// Wicked-side grass altitude sync can pick it up without pulling GameGuru globals into
	// Guru-WickedMAX. Value changes with the water slider in the editor.
	float GGGrass_GetDefaultWaterHeight();

	// Stage B.9: register a custom palette slot's DDS filename. Called by the editor UI when the
	// user adds a new grass via "Add New Grass" or when a level loads with custom slots defined
	// in .fpm. slot must be in [GGGRASS_CUSTOM_SLOT_BASE, GGGRASS_MAX_PALETTE_SLOTS). filename is
	// relative to Files/ (matches the storage in t.visuals.sGrassTextures[]). Pass nullptr or ""
	// to CLEAR a slot ("Delete Grass" button).
	void GGGrass_SetCustomSlotFilename( int slot, const char* filename );

	// Return the DDS filename registered for a custom slot, or nullptr if the slot is empty or
	// out of range. Used by the Wicked appearance builder to load the custom DDS on demand.
	const char* GGGrass_GetCustomSlotFilename( int slot );

	// True if any custom slot's filename has been registered/cleared since the last call —
	// take-and-clear. The Wicked side polls this each frame and rebuilds the affected appearance/
	// material when set (drop-in replacement for the existing bUpdateGrassMaterials signal but
	// scoped to just the custom-slot changes).
	bool GGGrass_TakeCustomSlotsDirty();

	// Painted grass type at world (x,z): 0 = none/flattened, else grass type id. Drives grass placement.
	uint32_t GGGrass_GetGrassMap( float x, float z );

	const char* GGGrass_GetTextureFilename( uint32_t matIndex, uint32_t grassIndex );
	const char* GGGrass_GetTextureShortName( uint32_t matIndex, uint32_t grassIndex );

	// Per-type metadata accessor for the Wicked grass renderer (read-only).
	struct GrassTypeInfo
	{
		const char* filename;     // DDS in Files/grassbank/
		const char* shortname;    // palette label
		float       scaleFactor;  // _SF_x.xx encoded in the filename
		uint32_t    material;     // source terrain material the sprite was authored for (0 = any)
	};
	uint32_t GGGrass_GetNumTypes();                            // 46
	const GrassTypeInfo* GGGrass_GetTypeInfo( uint32_t typeIdx ); // nullptr if out of range

	// Wicked terrain tells us its chunk world-stride (chunk_width-1 * chunk_scale) so we can bucket
	// painted cells directly into chunk keys. Must be called before any paint event for the keys to
	// be useful; zero stride disables key bucketing (the chunk set stays empty).
	void GGGrass_SetChunkStride( float strideInUnits );

	// Drain the set of chunk keys dirtied since the previous call. Keys are encoded as
	// ((uint32_t)cx << 32) | (uint32_t)cz, matching GGTerrainWicked's grassChunkKeyToChunkEntity
	// indexing. Take-and-clear semantics — the Wicked grass renderer drains this every frame and
	// invalidates only the listed chunks instead of every chunk in the brush stroke's bounding box.
	// Returns true and appends to `out` if there were any keys; returns false and leaves `out`
	// untouched otherwise.
	bool GGGrass_TakeDirtyChunks( wi::vector<uint64_t>& out );

	// True while the user is mid-stroke painting grass. The Wicked side uses this to suppress
	// terrain Generation_Update during the stroke — running it every frame while paint pumps the
	// editor was driving terrain chunk count up by ~65/sec, killing FPS.
	bool GGGrass_IsPaintStrokeActive();

	void GGGrass_SetPerformanceMode( uint32_t mode );

	void GGGrass_AddAll();
	void GGGrass_RemoveAll();
	void GGGrass_RestoreAll();

	uint32_t GGGrass_GetDataSize();
	int GGGrass_GetData( uint8_t* data ); // data must be allocated with a size of GGGrass_GetDataSize(), returns 1 on success
	int GGGrass_SetData( uint32_t size, uint8_t* data, sUndoSysEventGrass* pEvent = nullptr); // size must be equal to GGGrass_GetDataSize(), returns 1 on success

	void GGGrass_UpdateFlatArea( int mode, int type, float x, float z, float sx, float sz, float angle );
	void GGGrass_RestoreAllFlattened();
	int GGGrass_UpdateInstances();
}

#endif // _H_GGGRAS