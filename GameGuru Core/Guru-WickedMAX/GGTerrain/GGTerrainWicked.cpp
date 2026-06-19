#include "GGTerrain.h"
#include "GGTerrainWicked.h"
#include "GGGrass.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiTerrain.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiResourceManager.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiHelper.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiRenderer.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiProfiler.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiRenderPath3D.h"
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <cstdio>
#include <utility>

// Performance profiling: accessor functions defined in master_part0.cpp / master_part1.cpp
extern wi::RenderPath3D* GGPerf_GetRenderPath();
extern wi::scene::LightComponent* GGPerf_GetSunLight();
extern wi::scene::WeatherComponent& GGPerf_GetWeather();
extern std::string GGPerf_GetCachedProfilerText();

static wi::ecs::Entity wickedTerrainEntity = wi::ecs::INVALID_ENTITY;
static bool wickedTerrainInitialised = false;
static bool wickedTerrainMaterialsSetup = false;
static std::string wickedTerrainExeDir;  // EXE directory for resolving texture paths

// Phase 3: Painted material support
static int materialToSlot[GGTERRAIN_MAX_SOURCE_TEXTURES]; // GG 0-based mat index -> Wicked blendmap layer index
static int maxPaintedSlot = -1;                            // highest blendmap layer used by any painted material

// Phase 3: Blendmap state tracking — which chunks have had painted weights applied
static std::unordered_set<uint64_t> processedChunkKeys;
// Track which entity was at each grid key when we last painted it.
// When Wicked removes a distant chunk and regenerates it on camera return,
// the new chunk has a different entity ID — we detect this and repaint.
static std::unordered_map<uint64_t, wi::ecs::Entity> chunkKeyToEntity;

// Grass: a custom per-chunk HairParticleSystem grown from GG's painted grass map (pGrassMap).
// Wicked's built-in terrain grass stays disabled (it places grass from material regions, not our
// grass map). We reuse the HairParticleSystem class but drive placement/lifecycle ourselves,
// mirroring the ProcessPaintedChunkBlendmaps pattern.
//
// Per painted grass type, one cached MaterialComponent (g_grassMaterials[]) and one appearance
// template (g_grassAppearance[]) drive the look. A chunk with N painted grass types spawns N
// HairParticleSystem entities, each masked to just its own vertices via vertex_lengths.
static bool wickedGrassSetup = false;
static bool wickedGrassEnabled = true; // G key toggles grass visibility/creation
static std::unordered_map<uint64_t, wi::ecs::Entity> grassChunkKeyToChunkEntity; // chunk entity when grass was built
// Per-chunk per-type hair entity tracking. Each chunk-type slot is INVALID_ENTITY until first paint
// of that type in that chunk; after that the same entity is reused across paint events (Stage 2 —
// vertex_lengths is restamped in place instead of removing + recreating the entity, so existing
// strand positions hold rock-still during paint instead of regenerating their per-frame tail state).
struct ChunkGrassEntities
{
	wi::ecs::Entity perType[GGGRASS_NUM_TYPES];
	ChunkGrassEntities()
	{
		for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++) perType[t] = wi::ecs::INVALID_ENTITY;
	}
};
static std::unordered_map<uint64_t, ChunkGrassEntities> grassChunkKeyToGrassEntities;
static std::unordered_map<uint64_t, int> grassChunkKeyToTier;                    // current LOD tier per chunk
// Distance-LOD grass density (driven live by the SET_GRASS automation command).
// bladesPerVertex is the NEAR-tier density; mid/far tiers scale it down and grass stops past
// lodChunks chunk-distances from the camera. Only the near chunks are dense, so total strands stay
// bounded — VRAM-safe even during the load burst that previously crashed the driver (full density
// across many streaming chunks). A fully-grassy chunk has ~4489 verts; strands = grassyVerts*blades.
static uint32_t g_grassBladesPerVertex = 120;  // near-tier blades/vertex (LOD keeps total bounded)
static uint32_t g_grassMaxStrands = 350000;    // per-chunk strand cap (split across used types)
static float    g_grassLODChunks = 2.5f;       // grass radius in chunk-distances (beyond = bare)
static bool g_grassRebuildRequested = false;

// Brush cursor: a single DecalComponent entity that projects Files/editors/gfx/brush_ring.png down
// onto whatever's below it (terrain). Mirrors the legacy GG terrain-shader procedural circle.
// The entity is created lazily on first SetBrushCursor call (needs wickedTerrainExeDir + device).
// Hidden by zeroing the material baseColor alpha; the entity stays in the scene for reuse.
static wi::ecs::Entity g_brushCursorEntity = wi::ecs::INVALID_ENTITY;
static bool            g_brushCursorSetup = false;

static uint64_t MakeChunkKey(int32_t cx, int32_t cz)
{
	return ((uint64_t)(uint32_t)cx << 32) | (uint64_t)(uint32_t)cz;
}

// Helper to get the terrain component from the scene (registered, not static)
static wi::terrain::Terrain* GetWickedTerrain()
{
	if (wickedTerrainEntity == wi::ecs::INVALID_ENTITY) return nullptr;
	return wi::scene::GetScene().terrains.GetComponent(wickedTerrainEntity);
}

// Height modifier that feeds GameGuru's terrain height data into Wicked Engine chunks
struct GGHeightModifier : public wi::terrain::Modifier
{
	float bottomLevel = -20000.0f;
	float topLevel = 20000.0f;

	GGHeightModifier()
	{
		type = Type::Heightmap;
		blend = BlendMode::Normal;
		weight = 1.0f;
	}

	void Apply(const XMFLOAT2& world_pos, float& height) override
	{
		// world_pos.x = world X, world_pos.y = world Z
		float worldY = GGTerrain::GGTerrain_CalculateHeight(world_pos.x, world_pos.y);
		// Convert world height to 0-1 range for Wicked's lerp(bottomLevel, topLevel, height)
		float range = topLevel - bottomLevel;
		if (range > 0.0f)
			height = (worldY - bottomLevel) / range;
		else
			height = 0.5f;
	}
};

static std::shared_ptr<GGHeightModifier> heightModifier;

// Helper: set up a terrain material entity following the Wicked Editor pattern.
// Sets texture names, PBR properties, and calls CreateRenderData() for synchronous loading.
// Textures must be valid before Generation_Restart() because Generation_Update() checks
// resource.IsValid() on the same frame to decide whether to run the VT pipeline.
static void SetupTerrainMaterial(wi::scene::Scene& scene, wi::ecs::Entity entity, int ggMatIndex)
{
	using namespace wi::scene;

	// GG material index is 0-based, folder names are 1-based (mat1..mat32)
	int folderNum = (ggMatIndex & 0xFF) + 1;

	// Build paths from EXE directory (CWD-independent)
	char colorPath[512], normalPath[512], surfacePath[512];
	sprintf_s(colorPath, "%s/Files/terraintextures/mat%d/Color.dds", wickedTerrainExeDir.c_str(), folderNum);
	sprintf_s(normalPath, "%s/Files/terraintextures/mat%d/Normal.dds", wickedTerrainExeDir.c_str(), folderNum);
	sprintf_s(surfacePath, "%s/Files/terraintextures/mat%d/Surface.dds", wickedTerrainExeDir.c_str(), folderNum);

	MaterialComponent* mat = scene.materials.GetComponent(entity);
	if (!mat)
	{
		mat = &scene.materials.Create(entity);
	}

	mat->textures[MaterialComponent::BASECOLORMAP].name = colorPath;
	mat->textures[MaterialComponent::NORMALMAP].name = normalPath;
	// GG Surface.dds uses R=AO, G=Roughness, B=Metalness, A=255 — matches Wicked's SURFACEMAP
	// convention (R=occlusion, G=roughness, B=metalness, A=reflectance). A=255 means reflectance
	// multiplier is 1.0, so the base reflectance value set below is used directly.
	mat->textures[MaterialComponent::SURFACEMAP].name = surfacePath;

	// PBR defaults matching Wicked Editor terrain presets
	mat->SetRoughness(1.0f);
	mat->SetMetalness(0.0f);
	mat->SetReflectance(0.005f);

	mat->SetTextureStreamingDisabled(true);
	mat->CreateRenderData();
}

// Phase 2: Set up 4-layer terrain materials from GG render params
// Called lazily on first update (after level load, so params are correct)
static void SetupWickedTerrainMaterials()
{
	wi::terrain::Terrain* terrain = GetWickedTerrain();
	if (!terrain) return;

	auto& scene = wi::scene::GetScene();

	// Read material indices from GG render params
	int baseMat = GGTerrain::ggterrain_global_render_params.baseLayerMaterial & 0xFF;
	int slopeMat = GGTerrain::ggterrain_global_render_params.slopeMatIndex[0] & 0xFF;
	int lowMat = GGTerrain::ggterrain_global_render_params.layerMatIndex[0] & 0xFF;
	int highMat = GGTerrain::ggterrain_global_render_params.layerMatIndex[2] & 0xFF;

	// Create material entities and attach to terrain (following Wicked Editor pattern)
	for (int i = 0; i < wi::terrain::MATERIAL_COUNT; i++)
	{
		if (terrain->materialEntities[i] == wi::ecs::INVALID_ENTITY)
		{
			terrain->materialEntities[i] = wi::ecs::CreateEntity();
		}
		scene.Component_Attach(terrain->materialEntities[i], wickedTerrainEntity);
	}

	// Configure each material with texture names and properties
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_BASE], baseMat);
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_SLOPE], slopeMat);
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_LOW_ALTITUDE], lowMat);
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_HIGH_ALTITUDE], highMat);

	// Region parameters control height/slope-based material blending.
	// Wicked uses smoothstep(0, regionN, value) where:
	//   slope_amount = 1 - normal.y (0=flat, 1=vertical)
	//   low_alt = InverseLerp(0, bottomLevel, height) — increases as height goes below 0
	//   high_alt = InverseLerp(0, topLevel, height) — increases as height goes above 0
	// GG uses independent start/end thresholds per layer which don't map exactly, so these
	// are approximate mappings that give visually similar results.
	terrain->region1 = GGTerrain::ggterrain_global_render_params.slopeEnd[0];  // slope transition
	terrain->region2 = 2.0f;   // low altitude (GG's low layer is at positive heights, poor Wicked mapping)
	terrain->region3 = 1.0f;   // high altitude (tighter than default for visible mountain material)

	// Phase 3: Initialize materialToSlot lookup — maps GG 0-based material index to Wicked blendmap layer
	for (int i = 0; i < GGTERRAIN_MAX_SOURCE_TEXTURES; i++)
		materialToSlot[i] = -1;

	// Map the 4 auto materials to their fixed slots (0-3)
	materialToSlot[baseMat] = wi::terrain::MATERIAL_BASE;
	materialToSlot[slopeMat] = wi::terrain::MATERIAL_SLOPE;
	materialToSlot[lowMat] = wi::terrain::MATERIAL_LOW_ALTITUDE;
	materialToSlot[highMat] = wi::terrain::MATERIAL_HIGH_ALTITUDE;
	maxPaintedSlot = wi::terrain::MATERIAL_HIGH_ALTITUDE;  // 3

	// Scan material map for unique painted materials not already in auto slots
	const uint8_t* matMap = GGTerrain::GGTerrain_GetMaterialMapPtr();
	int mapRes = GGTerrain::GGTerrain_GetMaterialMapResolution();
	int numExtraMaterials = 0;

	if (matMap && mapRes > 0)
	{
		bool usedMaterials[GGTERRAIN_MAX_SOURCE_TEXTURES] = {};
		for (int i = 0; i < mapRes * mapRes; i++)
		{
			uint8_t val = matMap[i];
			if (val > 0 && val <= GGTERRAIN_MAX_SOURCE_TEXTURES)
				usedMaterials[val - 1] = true;
		}

		// Create material entities for painted materials beyond the 4 auto slots
		for (int i = 0; i < GGTERRAIN_MAX_SOURCE_TEXTURES; i++)
		{
			if (usedMaterials[i] && materialToSlot[i] < 0)
			{
				int newSlot = wi::terrain::MATERIAL_COUNT + numExtraMaterials;
				materialToSlot[i] = newSlot;

				// Extend materialEntities vector if needed
				while ((int)terrain->materialEntities.size() <= newSlot)
					terrain->materialEntities.push_back(wi::ecs::INVALID_ENTITY);

				terrain->materialEntities[newSlot] = wi::ecs::CreateEntity();
				scene.Component_Attach(terrain->materialEntities[newSlot], wickedTerrainEntity);
				SetupTerrainMaterial(scene, terrain->materialEntities[newSlot], i);

				if (newSlot > maxPaintedSlot)
					maxPaintedSlot = newSlot;
				numExtraMaterials++;
			}
		}
	}

	// Reset blendmap tracking so all chunks get reprocessed with new materials
	processedChunkKeys.clear();
	chunkKeyToEntity.clear();

	// Restart generation to pick up all materials (auto + painted)
	// Generation_Restart() deep-copies the materials internally
	terrain->Generation_Restart();
	wickedTerrainMaterialsSetup = true;

	wi::backlog::post(std::string("GGTerrainWicked: materials setup complete (" +
		std::to_string(numExtraMaterials) + " extra painted materials, maxSlot=" +
		std::to_string(maxPaintedSlot) + ")").c_str());
}

// Phase 3: Process terrain chunks for painted material blendmaps.
// Iterates scene.objects (main-thread safe) to find terrain chunks, then
// cancels generation once for safe chunk data access and processes the batch.
static void ProcessPaintedChunkBlendmaps(wi::terrain::Terrain* terrain)
{
	const uint8_t* matMap = GGTerrain::GGTerrain_GetMaterialMapPtr();
	if (!matMap) return;

	int mapRes = GGTerrain::GGTerrain_GetMaterialMapResolution();
	float editableSize = GGTerrain::GGTerrain_GetEditableSize();
	float editableSizeRcp = (editableSize > 0.0f) ? (1.0f / editableSize) : 0.0f;
	// editableSize is already the half-size (area goes from -editableSize to +editableSize)
	float chunkStride = (wi::terrain::chunk_width - 1) * terrain->chunk_scale;
	auto& scene = wi::scene::GetScene();

	// Phase 1: Scan scene.objects (main-thread safe) to collect unprocessed terrain chunks
	struct PendingChunk {
		wi::ecs::Entity entity;
		int32_t cx, cz;
		wi::scene::MeshComponent* mesh;
		const wi::scene::TransformComponent* transform;
	};
	wi::vector<PendingChunk> pending;

	for (size_t oi = 0; oi < scene.objects.GetCount(); oi++)
	{
		wi::scene::ObjectComponent& obj = scene.objects[oi];
		wi::scene::MeshComponent* mesh = scene.meshes.GetComponent(obj.meshID);
		if (!mesh || mesh->vertex_positions.size() != wi::terrain::vertexCount) continue;

		wi::ecs::Entity entity = scene.objects.GetEntity(oi);
		const wi::scene::TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (!transform) continue;

		int32_t cx = (int32_t)std::round(transform->world._41 / chunkStride);
		int32_t cz = (int32_t)std::round(transform->world._43 / chunkStride);
		uint64_t key = MakeChunkKey(cx, cz);

		// Check if chunk center is within editable area
		float chunkCenterX = cx * chunkStride;
		float chunkCenterZ = cz * chunkStride;
		bool inEditable = (chunkCenterX >= -editableSize && chunkCenterX <= editableSize &&
			chunkCenterZ >= -editableSize && chunkCenterZ <= editableSize);

		if (processedChunkKeys.count(key))
		{
			// Check if the chunk was removed and recreated (new entity at same position).
			// Wicked Engine removes distant chunks and regenerates them on camera return.
			auto it = chunkKeyToEntity.find(key);
			if (it != chunkKeyToEntity.end() && it->second != entity)
			{
				// Entity changed — chunk was recreated, needs repainting
				processedChunkKeys.erase(key);
				chunkKeyToEntity.erase(it);
			}
			else
			{
				continue;
			}
		}

		// Skip chunks outside editable area (no paint data there)
		if (!inEditable)
		{
			processedChunkKeys.insert(key);
			continue;
		}

		pending.push_back({ entity, cx, cz, mesh, transform });
	}

	if (pending.empty()) return;

	// Phase 2: Cancel generation for safe chunk data access, then process the batch
	terrain->Generation_Cancel();

	int chunksModified = 0;

	for (auto& pc : pending)
	{
		uint64_t key = MakeChunkKey(pc.cx, pc.cz);

		// Find chunk data by iterating terrain->chunks (safe after Generation_Cancel)
		wi::terrain::ChunkData* chunk_data = nullptr;
		for (auto& [chunk, cd] : terrain->chunks)
		{
			if (cd.entity == pc.entity) { chunk_data = &cd; break; }
		}
		if (!chunk_data) continue;  // Don't mark as processed — retry next frame

		// Skip chunks whose blendmap hasn't been generated yet by the pipeline.
		// Painting before generation completes would be overwritten by the default
		// height/slope blending stage. Retry next frame when generation is done.
		if (chunk_data->blendmap_layers.empty()) continue;

		// Mark as processed only after confirming chunk_data exists
		processedChunkKeys.insert(key);
		chunkKeyToEntity[key] = pc.entity;

		XMMATRIX worldMatrix = XMLoadFloat4x4(&pc.transform->world);

		// First pass: check if any vertex is painted
		bool hasPainted = false;
		for (size_t vi = 0; vi < wi::terrain::vertexCount && !hasPainted; vi++)
		{
			XMVECTOR wp = XMVector3Transform(XMLoadFloat3(&pc.mesh->vertex_positions[vi]), worldMatrix);
			XMFLOAT3 worldPos;
			XMStoreFloat3(&worldPos, wp);

			float mapU = worldPos.x * editableSizeRcp * 0.5f + 0.5f;
			float mapV = worldPos.z * editableSizeRcp * 0.5f + 0.5f;
			int mapX = (int)(mapU * mapRes);
			int mapZ = (int)(mapV * mapRes);

			if (mapX >= 0 && mapX < mapRes && mapZ >= 0 && mapZ < mapRes)
			{
				uint8_t matVal = matMap[mapZ * mapRes + mapX];
				if (matVal > 0 && matVal <= GGTERRAIN_MAX_SOURCE_TEXTURES && materialToSlot[matVal - 1] >= 0)
					hasPainted = true;
			}
		}

		if (!hasPainted) continue;

		// Ensure all blendmap layers exist up to maxPaintedSlot
		for (int i = 0; i <= maxPaintedSlot; i++)
			chunk_data->enable_blendmap_layer(i);

		// Second pass: write painted weights into blendmap layers
		for (size_t vi = 0; vi < wi::terrain::vertexCount; vi++)
		{
			XMVECTOR wp = XMVector3Transform(XMLoadFloat3(&pc.mesh->vertex_positions[vi]), worldMatrix);
			XMFLOAT3 worldPos;
			XMStoreFloat3(&worldPos, wp);

			float mapU = worldPos.x * editableSizeRcp * 0.5f + 0.5f;
			float mapV = worldPos.z * editableSizeRcp * 0.5f + 0.5f;
			int mapX = (int)(mapU * mapRes);
			int mapZ = (int)(mapV * mapRes);

			if (mapX < 0 || mapX >= mapRes || mapZ < 0 || mapZ >= mapRes)
				continue;

			uint8_t matVal = matMap[mapZ * mapRes + mapX];
			if (matVal == 0 || matVal > GGTERRAIN_MAX_SOURCE_TEXTURES)
				continue;

			int slot = materialToSlot[matVal - 1];
			if (slot < 0 || slot >= (int)chunk_data->blendmap_layers.size())
				continue;

			// Zero all layers at this vertex, then set painted layer to full weight
			for (size_t li = 0; li < chunk_data->blendmap_layers.size(); li++)
				chunk_data->blendmap_layers[li].pixels[vi] = 0;
			chunk_data->blendmap_layers[slot].pixels[vi] = 255;
		}

		// Invalidate GPU blendmap texture and VT to trigger re-blend
		chunk_data->blendmap = {};
		terrain->CreateChunkRegionTexture(*chunk_data);
		if (chunk_data->vt)
			chunk_data->vt->invalidate();

		chunksModified++;
	}

	if (chunksModified > 0)
	{
		wi::backlog::post(std::string("GGTerrainWicked: painted blendmaps on " +
			std::to_string(chunksModified) + " chunks").c_str());
	}
}


// Per-grass-type material cache. One MaterialComponent per entry in GGGrass::grassFiles[] (the 46
// DDS files in Files/grassbank/). Each material is built lazily the first time we see its type id
// in ProcessGrassChunks, so a level that uses 3 grass types pays for 3 DDS loads, not 46.
static wi::scene::MaterialComponent g_grassMaterials[GGGRASS_NUM_TYPES];
static bool                         g_grassMaterialReady[GGGRASS_NUM_TYPES] = {};

// Build one cached grass material from Files/grassbank/<filename>. Returns nullptr on bad index.
// Kelp/seaweed sprites have authored _normal.dds siblings; we wire those in when present.
static wi::scene::MaterialComponent* BuildGrassMaterial(uint32_t typeIdx)
{
	if (typeIdx >= GGGRASS_NUM_TYPES) return nullptr;
	if (g_grassMaterialReady[typeIdx]) return &g_grassMaterials[typeIdx];

	const GGGrass::GrassTypeInfo* info = GGGrass::GGGrass_GetTypeInfo(typeIdx);
	if (!info || !info->filename) return nullptr;

	char colorPath[512];
	sprintf_s(colorPath, "%s/Files/grassbank/%s", wickedTerrainExeDir.c_str(), info->filename);

	// Build a normal-map path if the sprite name ends in "_color.dds" (kelp/seaweed convention).
	// Otherwise leave normals unset — the alpha-cutout blade silhouette doesn't need them.
	char normalPath[512] = {0};
	bool haveNormal = false;
	const char* colorSuffix = strstr(info->filename, "_color.dds");
	if (colorSuffix)
	{
		size_t prefixLen = (size_t)(colorSuffix - info->filename);
		char normalName[256];
		if (prefixLen < sizeof(normalName) - 16)
		{
			memcpy(normalName, info->filename, prefixLen);
			memcpy(normalName + prefixLen, "_normal.dds", 12); // includes NUL
			sprintf_s(normalPath, "%s/Files/grassbank/%s", wickedTerrainExeDir.c_str(), normalName);
			haveNormal = true;
		}
	}

	wi::scene::MaterialComponent& mat = g_grassMaterials[typeIdx];
	mat = wi::scene::MaterialComponent();
	mat.textures[wi::scene::MaterialComponent::BASECOLORMAP].name = colorPath;
	if (haveNormal)
		mat.textures[wi::scene::MaterialComponent::NORMALMAP].name = normalPath;
	mat.SetAlphaRef(0.5f);          // alpha cutout for the blade silhouette
	mat.SetDoubleSided(true);
	mat.SetRoughness(1.0f);
	mat.SetMetalness(0.0f);
	mat.SetReflectance(0.02f);
	// Green subsurface keeps back-lit blades softly translucent instead of going dark.
	mat.SetSubsurfaceScatteringColor(XMFLOAT3(0.35f, 0.6f, 0.2f));
	mat.SetSubsurfaceScatteringAmount(1.0f);
	mat.SetCastShadow(false);
	mat.SetTextureStreamingDisabled(true);
	mat.CreateRenderData();

	g_grassMaterialReady[typeIdx] = true;
	return &mat;
}

// Per-grass-type appearance templates (length / width / billboards / stiffness etc.).
// Copied onto each spawned HairParticleSystem so the strand placement / mesh / strand-count are
// per-chunk but the look-and-feel comes from the type table.
static wi::HairParticleSystem g_grassAppearance[GGGRASS_NUM_TYPES];
static bool                   g_grassAppearanceReady = false;

// 8 categories spanning the 46 entries in GGGrass::grassFiles[].
// Boundaries kept in lockstep with that table — change one, change both.
enum GrassCategory { GCAT_COURSE, GCAT_SHORT, GCAT_TALL, GCAT_WILD, GCAT_WEED, GCAT_FLOWER, GCAT_KELP, GCAT_SEAWEED };

static GrassCategory CategoryFor(uint32_t typeIdx)
{
	if (typeIdx <= 6)  return GCAT_COURSE;   // 0..6   course grass mat1..mat30
	if (typeIdx <= 13) return GCAT_SHORT;    // 7..13  short  grass mat1..mat30
	if (typeIdx <= 20) return GCAT_TALL;     // 14..20 tall   grass mat1..mat30
	if (typeIdx <= 27) return GCAT_WILD;     // 21..27 wild   grass mat1..mat30
	if (typeIdx <= 36) return GCAT_WEED;     // 28..36 weeds 1..9
	if (typeIdx <= 39) return GCAT_FLOWER;   // 37..39 red / white / yellow flowers
	if (typeIdx <= 42) return GCAT_KELP;     // 40..42 kelp 1..3
	return GCAT_SEAWEED;                     // 43..45 seaweed 1..3
}

// Fill g_grassAppearance from the per-type GGGrass metadata + category defaults.
// scaleFactor (from the _SF_x.xx filename suffix) scales blade length so e.g. "short grass" really
// renders shorter than "tall grass". Width / stiffness / billboard count come from the category.
static void BuildGrassAppearance()
{
	if (g_grassAppearanceReady) return;
	uint32_t numTypes = GGGrass::GGGrass_GetNumTypes();
	if (numTypes > GGGRASS_NUM_TYPES) numTypes = GGGRASS_NUM_TYPES;

	for (uint32_t t = 0; t < numTypes; t++)
	{
		const GGGrass::GrassTypeInfo* info = GGGrass::GGGrass_GetTypeInfo(t);
		float sf = info ? info->scaleFactor : 1.0f;
		if (sf <= 0.01f) sf = 1.0f;

		wi::HairParticleSystem& a = g_grassAppearance[t];
		a = wi::HairParticleSystem();
		a.segmentCount = 1;        // single segment keeps GPU memory bounded across chunks
		a.randomness = 0.35f;
		a.stiffness = 9.0f;        // upright enough to dodge the wind-driven twist/flip (confirmed safe)
		a.drag = 0.5f;
		a.viewDistance = 5000.0f;  // ~400 ft cull; tightened below for small features (flowers etc.)
		a.atlas_rects.clear();     // each material is a single-sprite DDS — no atlas
		a.uniformity = 1.0f;

		// Base sizes doubled across the board to match DX11 parity (DX11 grass appeared twice as
		// large side-by-side). Length AND width are doubled so blades stay proportional rather than
		// becoming spaghetti-thin.
		switch (CategoryFor(t))
		{
		case GCAT_COURSE:
			a.length = 28.0f * sf;
			a.width = 2.8f;
			a.billboardCount = 2;
			break;
		case GCAT_SHORT:
			a.length = 16.0f * sf;
			a.width = 2.6f;
			a.billboardCount = 2;
			break;
		case GCAT_TALL:
			a.length = 44.0f * sf;
			a.width = 2.8f;
			a.billboardCount = 2;
			break;
		case GCAT_WILD:
			a.length = 24.0f * sf;
			a.width = 2.6f;
			a.billboardCount = 2;
			break;
		case GCAT_WEED:
			a.length = 28.0f * sf;
			a.width = 2.4f;
			a.billboardCount = 1;  // weeds/stems read better as flat billboards
			a.stiffness = 12.0f;   // stems hold their shape
			break;
		case GCAT_FLOWER:
			// Wicked's hair width is a multiplier on length (rendered width = width * aspect *
			// length). DX11 reference shows individual wildflowers with tall stems peeking above
			// the grass — to match that "tall thin stalk + bloom on top" silhouette, length needs
			// to be similar to grass height and width has to be small.
			a.length = 60.0f * sf;
			a.width = 1.0f;
			a.billboardCount = 2;
			a.viewDistance = 2500.0f; // tiny feature — cull earlier to save fill
			break;
		case GCAT_KELP:
			a.length = 60.0f * sf;
			a.width = 8.0f;
			a.billboardCount = 1;
			a.stiffness = 4.0f;    // sways with current
			a.drag = 0.8f;
			break;
		case GCAT_SEAWEED:
			a.length = 90.0f * sf;
			a.width = 5.0f;
			a.billboardCount = 1;
			a.stiffness = 3.0f;
			a.drag = 0.9f;
			break;
		}
	}
	g_grassAppearanceReady = true;
}

// Populate the per-type appearance templates (one per entry in GGGrass::grassFiles[]). Materials
// are built lazily on first sighting (see BuildGrassMaterial), so no DDS is loaded until a chunk
// actually paints that type.
//
// Runtime assets: Files/grassbank/*.dds — already shipped as part of the GameGuru asset pack.
// Nothing else to deploy.
static void SetupWickedGrass()
{
	BuildGrassAppearance();
	wickedGrassSetup = true;
}

// Distance-LOD grass: dense fine grass on the chunks nearest the camera, thinning with distance and
// stopping past g_grassLODChunks. Concentrating strands near the camera (and dropping them far) keeps
// the total bounded — the trick native grass uses — so VRAM stays safe even during the load burst.
// Placement still follows GG's painted grass map; chunks crossing a tier boundary (or regenerated by
// the terrain) are rebuilt at the new density. All scene mutation is deferred to after Generation_Cancel.
static int GrassTierForRingDist(float ringDist)
{
	if (ringDist <= 1.0f) return 3;              // near: full density
	if (ringDist <= 1.7f) return 2;              // mid
	if (ringDist <= g_grassLODChunks) return 1;  // far: sparse
	return 0;                                     // beyond: no grass
}
static float GrassTierDensityScale(int tier)
{
	switch (tier) { case 3: return 1.00f; case 2: return 0.18f; case 1: return 0.05f; default: return 0.0f; }
}

static void ProcessGrassChunks(wi::terrain::Terrain* terrain, const XMFLOAT3& cameraPos)
{
	auto& scene = wi::scene::GetScene();
	float editableSize = GGTerrain::GGTerrain_GetEditableSize();
	float chunkStride = (wi::terrain::chunk_width - 1) * terrain->chunk_scale;

	struct PendingGrass {
		wi::ecs::Entity entity;
		uint64_t key;
		wi::scene::MeshComponent* mesh;
		XMFLOAT4X4 world;
		int tier;
		bool fullReset; // true when Wicked recycled the chunk; existing hair entities reference a
		                // gone mesh and must be removed before recreate. False = paint update path
		                // where we reuse existing entities (Stage 2).
	};
	wi::vector<PendingGrass> pending;

	// Phase 1 (read-only): pick each chunk's target LOD tier from camera distance; queue chunks whose
	// tier (or chunk entity) changed. No scene/map mutation here — the generator thread may be running.
	for (size_t oi = 0; oi < scene.objects.GetCount(); oi++)
	{
		wi::scene::ObjectComponent& obj = scene.objects[oi];
		wi::scene::MeshComponent* mesh = scene.meshes.GetComponent(obj.meshID);
		if (!mesh || mesh->vertex_positions.size() != wi::terrain::vertexCount) continue;

		wi::ecs::Entity entity = scene.objects.GetEntity(oi);
		const wi::scene::TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (!transform) continue;

		int32_t cx = (int32_t)std::round(transform->world._41 / chunkStride);
		int32_t cz = (int32_t)std::round(transform->world._43 / chunkStride);
		uint64_t key = MakeChunkKey(cx, cz);

		float chunkCenterX = cx * chunkStride;
		float chunkCenterZ = cz * chunkStride;
		bool inEditable = (chunkCenterX >= -editableSize && chunkCenterX <= editableSize &&
			chunkCenterZ >= -editableSize && chunkCenterZ <= editableSize);

		float dx = chunkCenterX - cameraPos.x;
		float dz = chunkCenterZ - cameraPos.z;
		float ringDist = sqrtf(dx * dx + dz * dz) / chunkStride;
		int targetTier = inEditable ? GrassTierForRingDist(ringDist) : 0;

		auto entIt = grassChunkKeyToChunkEntity.find(key);
		bool entityChanged = (entIt != grassChunkKeyToChunkEntity.end() && entIt->second != entity);
		auto tierIt = grassChunkKeyToTier.find(key);
		int currentTier = (tierIt != grassChunkKeyToTier.end()) ? tierIt->second : -1;
		if (!entityChanged && currentTier == targetTier) continue; // already correct for this chunk

		auto git = grassChunkKeyToGrassEntities.find(key);
		bool hasExistingEntities = false;
		if (git != grassChunkKeyToGrassEntities.end())
		{
			for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++)
			{
				if (git->second.perType[t] != wi::ecs::INVALID_ENTITY) { hasExistingEntities = true; break; }
			}
		}

		// Chunks that need NO scene mutation (no old grass to remove AND new tier wants no grass)
		// are recorded inline without going through Phase 2. This avoids triggering
		// Generation_Cancel for the common case of Wicked discovering a new far chunk during its
		// background streaming — each Cancel interrupts Wicked's own chunk-removal pass, leaving
		// stale chunks in scene.objects, and during paint the chunk count was growing at ~65/sec.
		if (!hasExistingEntities && targetTier == 0)
		{
			grassChunkKeyToChunkEntity[key] = entity;
			grassChunkKeyToTier[key] = targetTier;
			continue;
		}

		// Capture the world matrix by value (a transform pointer would be invalidated when we
		// create transform components for grass entities in Phase 2).
		pending.push_back({ entity, key, mesh, transform->world, targetTier, entityChanged });
	}

	if (pending.empty()) return;

	// Generation cancelled = safe window to mutate the scene (no generator-thread race).
	terrain->Generation_Cancel();

	// Phase 2: apply removals + (re)creations and update tracking.
	for (auto& pc : pending)
	{
		ChunkGrassEntities& existingEntities = grassChunkKeyToGrassEntities[pc.key]; // default-construct if first time

		// Full reset paths: Wicked recycled the chunk (old hair entities reference a vanished
		// mesh) OR the chunk's new tier is 0 (bare — no grass should render here at all). Both
		// drop every per-type entity for this chunk so we start clean.
		if (pc.fullReset || pc.tier == 0)
		{
			for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++)
			{
				wi::ecs::Entity e = existingEntities.perType[t];
				if (e != wi::ecs::INVALID_ENTITY && scene.hairs.GetComponent(e))
					scene.Entity_Remove(e);
				existingEntities.perType[t] = wi::ecs::INVALID_ENTITY;
			}
		}

		grassChunkKeyToChunkEntity[pc.key] = pc.entity;
		grassChunkKeyToTier[pc.key] = pc.tier;
		if (pc.tier == 0) continue; // bare at this distance / outside editable area

		// Bucket each grassy vertex by its painted type id (value-2 from the grass map; value 1
		// "default paint" is aliased to type 0 — Course Grass mat1 — for backwards-compat with
		// legacy levels). vertexType[vi] = 0xFF means "no grass at this vertex".
		//
		// Multi-sample the grass map across each vertex's ~Voronoi cell (±half_chunk_scale around
		// its world XZ) rather than a single point sample at the vertex centre. At chunk_scale=80
		// the terrain vertices sit 80 inches apart while a grass-map cell is ~4.8 inches and the
		// minimum brush radius is 15. A point sample only registers when the brush blob happens
		// to land exactly under a vertex (~11% of the area), so small-brush paint clicks looked
		// invisible to the user — they had to keep brushing until they hit a vertex's exact spot.
		// The 7×7 grid covers the full 80×80 influence area with a ~13-inch step, small enough
		// that even a 15-radius brush always overlaps at least one sample.
		XMMATRIX worldMatrix = XMLoadFloat4x4(&pc.world);
		static_assert(GGGRASS_NUM_TYPES <= 254, "vertexType uses 0xFF as sentinel");
		uint8_t vertexType[wi::terrain::vertexCount];
		// Per-vertex coverage fraction (painted samples / total samples in Voronoi cell). Used as
		// the vertex_length when rendering — small brushes that cover only a few of the vertex's
		// samples produce a low vertex_length, which bary-interpolates to short strand_length
		// across surrounding triangles, which makes the visible blade patch shrink to roughly the
		// brush footprint instead of spreading across the full ~80-inch Voronoi cell. Dense
		// painting fills more samples → coverage approaches 1.0 → full blade length → standard
		// patch coverage. Smooth transition from "tuft" to "full grass" as the user paints more.
		float vertexCoverage[wi::terrain::vertexCount];
		bool typeSeen[GGGRASS_NUM_TYPES] = {};
		const float halfChunkScale = terrain->chunk_scale * 0.5f;
		constexpr int SAMPLES_PER_AXIS = 7;
		constexpr float TOTAL_SAMPLES = float(SAMPLES_PER_AXIS * SAMPLES_PER_AXIS);
		// Floor on the scaled vertex_length so a single-sample hit still produces a visible blade
		// (1/49 ≈ 0.02 would render strands ~2% of full length — invisible). 0.15 gives the
		// smallest brush a faint-but-visible tuft.
		constexpr float MIN_VERTEX_LENGTH = 0.15f;
		const float sampleStep = (halfChunkScale * 2.0f) / (SAMPLES_PER_AXIS - 1);
		for (size_t vi = 0; vi < wi::terrain::vertexCount; vi++)
		{
			XMVECTOR wp = XMVector3Transform(XMLoadFloat3(&pc.mesh->vertex_positions[vi]), worldMatrix);
			XMFLOAT3 worldPos;
			XMStoreFloat3(&worldPos, wp);
			// Closest painted sample wins for type pick; total painted-sample count drives the
			// vertex_length scaling.
			uint32_t mapVal = 0;
			float bestDistSq = halfChunkScale * halfChunkScale * 4.0f;
			int paintedSamples = 0;
			for (int sx = 0; sx < SAMPLES_PER_AXIS; sx++)
			{
				float offX = -halfChunkScale + sampleStep * sx;
				for (int sz = 0; sz < SAMPLES_PER_AXIS; sz++)
				{
					float offZ = -halfChunkScale + sampleStep * sz;
					uint32_t sampleVal = GGGrass::GGGrass_GetGrassMap(worldPos.x + offX, worldPos.z + offZ);
					if (sampleVal == 0) continue;
					paintedSamples++;
					float distSq = offX * offX + offZ * offZ;
					if (distSq < bestDistSq)
					{
						bestDistSq = distSq;
						mapVal = sampleVal;
					}
				}
			}
			if (mapVal == 0) { vertexType[vi] = 0xFF; vertexCoverage[vi] = 0.0f; continue; }
			uint32_t typeIdx = (mapVal >= 2) ? (mapVal - 2) : 0; // value 1 -> Course Grass mat1
			if (typeIdx >= GGGRASS_NUM_TYPES) { vertexType[vi] = 0xFF; vertexCoverage[vi] = 0.0f; continue; }
			vertexType[vi] = (uint8_t)typeIdx;
			float cov = (float)paintedSamples / TOTAL_SAMPLES;
			if (cov < MIN_VERTEX_LENGTH) cov = MIN_VERTEX_LENGTH;
			if (cov > 1.0f) cov = 1.0f;
			vertexCoverage[vi] = cov;
			typeSeen[typeIdx] = true;
		}

		// For each grass type: UPDATE the existing per-(chunk, type) hair entity in place
		// (preserves strand positions and the per-strand simulation tail state) or CREATE one if
		// we've never seen this type in this chunk before. Stage 2 — entities persist across paint
		// events, so blade BASES and TIPS both hold still during a stroke; only newly-painted /
		// newly-erased vertices fade in / out via the bary-interpolated strand_length.
		for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++)
		{
			wi::ecs::Entity existing = existingEntities.perType[t];
			wi::HairParticleSystem* existingHair = (existing != wi::ecs::INVALID_ENTITY)
				? scene.hairs.GetComponent(existing) : nullptr;

			if (existingHair)
			{
				// UPDATE in place: restamp vertex_lengths from the current paint state and push
				// just the length buffer to the GPU. UpdateVertexLengthsBuffer (Stage 3, local
				// Wicked patch) leaves generalBuffer / simulation_view intact, so each strand's
				// prevTail/currentTail simulation state survives and the wind animation keeps
				// flowing across the update — no per-paint-event settling pop.
				bool anyChanged = false;
				for (size_t vi = 0; vi < wi::terrain::vertexCount; vi++)
				{
					float want = (typeSeen[t] && vertexType[vi] == t) ? vertexCoverage[vi] : 0.0f;
					if (existingHair->vertex_lengths[vi] != want)
					{
						existingHair->vertex_lengths[vi] = want;
						anyChanged = true;
					}
				}
				if (anyChanged) existingHair->UpdateVertexLengthsBuffer();
				continue;
			}

			// CREATE: first paint of this type in this chunk (or full reset path).
			if (!typeSeen[t]) continue;

			wi::scene::MaterialComponent* mat = BuildGrassMaterial(t);
			if (!mat) continue;

			// Pre-fill vertex_lengths with 1.0 so CreateFromMesh includes EVERY chunk triangle in
			// the index buffer (triangleCount = constant = (chunk_width-1)² × 2 = 8192). Restamp
			// with the real paint mask AFTER CreateFromMesh — strands whose triangle interpolates
			// to length 0 from unpainted vertices render invisibly. Net effect: each strand's
			// (rng-picked) triangle is invariant across paint events; existing blades never
			// reshuffle when paint adds/removes vertices in the chunk.
			wi::vector<float> vertex_lengths;
			vertex_lengths.resize(wi::terrain::vertexCount, 1.0f);

			wi::ecs::Entity grassEntity = wi::ecs::CreateEntity();
			wi::HairParticleSystem& hair = scene.hairs.Create(grassEntity);
			hair = g_grassAppearance[t];   // per-type look (length/width/billboards/stiffness)
			hair.meshID = pc.entity;
			hair.vertex_lengths = std::move(vertex_lengths);

			// Stage 3 Option B: tell the simulate CS this entity represents grass type `t` and
			// hand it the GG paint mask. xHairGrassType is 1-based so 0 means "feature disabled"
			// for non-GG hair systems. invWorldSize matches GGGrass_GetGrassMap's CPU formula:
			// uv = worldXZ * (0.5 / editableSize) + 0.5  (editableSize is HALF the world extent).
			hair.grass_type = (uint32_t)t + 1;
			hair.grass_map_inv_world_size = 0.5f / editableSize;
			hair.grass_map_origin_x = 0.0f;
			hair.grass_map_origin_z = 0.0f;
			hair.grass_visibility_texture = GGGrass::GGGrass_GetMapTexture();

			// Fixed strand count per tier — independent of paint state AND of typesUsed, so
			// neither painting new vertices nor adding a new type into the chunk perturbs an
			// existing entity's strandCount. Density stays consistent throughout a stroke
			// instead of ramping up as vertices accumulate. Strands distribute uniformly over the
			// chunk's full triangulation; visible density tracks the painted-area fraction.
			constexpr uint32_t STAGE1_STRANDS_PER_TIER = 100000;
			uint32_t strands = (uint32_t)(STAGE1_STRANDS_PER_TIER * GrassTierDensityScale(pc.tier) + 0.5f);
			if (strands < 1024) strands = 1024;
			hair.strandCount = strands;

			hair.CreateFromMesh(*pc.mesh);
			// Restamp the actual paint mask + coverage scaling now that the index buffer is built.
			// CreateRenderData below uploads this as the per-vertex length buffer the simulate CS
			// bary-interpolates to get each strand's visible length.
			for (size_t vi = 0; vi < wi::terrain::vertexCount; vi++)
				hair.vertex_lengths[vi] = (vertexType[vi] == t) ? vertexCoverage[vi] : 0.0f;
			// CreateFromMesh picks R16G16B16A16_UNORM for static (non-skinned) base meshes — terrain
			// chunks are static, so we'd get UNORM by default. The simulate CS then remaps each frame's
			// per-strand position to UNORM across the WHOLE hair-system AABB (≈5280×5280 inch for a
			// chunk), which gives a quantization step of ~0.08 inch. Micro-sway (~1-2 inch tip motion)
			// crosses only ~12-25 steps, producing the visibly choppy "fixed-point" look. Force the
			// position buffer to full FP32 so smooth sway stays smooth.
			hair.position_format = wi::graphics::Format::R32G32B32A32_FLOAT;
			hair.CreateRenderData();

			scene.materials.Create(grassEntity) = *mat;
			scene.transforms.Create(grassEntity);
			scene.Component_Attach(grassEntity, pc.entity, true); // inherit chunk transform

			existingEntities.perType[t] = grassEntity;
		}
	}
}


// Remove all current grass entities and clear tracking so ProcessGrassChunks rebuilds them next
// frame with the updated template/material (used after a live param change via SET_GRASS).
static void ForceGrassRebuild()
{
	wi::terrain::Terrain* terrain = GetWickedTerrain();
	if (terrain) terrain->Generation_Cancel();   // make scene mutation safe (no generator-thread race)
	auto& scene = wi::scene::GetScene();
	for (auto& kv : grassChunkKeyToGrassEntities)
	{
		for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++)
		{
			wi::ecs::Entity e = kv.second.perType[t];
			if (e != wi::ecs::INVALID_ENTITY && scene.hairs.GetComponent(e))
				scene.Entity_Remove(e);
		}
	}
	grassChunkKeyToGrassEntities.clear();
	grassChunkKeyToTier.clear();
	grassChunkKeyToChunkEntity.clear();
}

// Runtime grass tuning, driven by the SET_GRASS automation command. Now that each grass type has
// its own appearance template and material, the tuning knobs apply uniformly across all types
// (multiplicative for sizes, replacement for material props). A deferred rebuild applies them.
void GGTerrainWicked_SetGrassParam(const char* param, float value)
{
	std::string p = param ? param : "";
	auto forAllAppearance = [&](auto fn) {
		for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++) fn(g_grassAppearance[t]);
	};
	auto forAllMaterials = [&](auto fn) {
		for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++)
			if (g_grassMaterialReady[t]) fn(g_grassMaterials[t]);
	};
	if      (p == "length")     forAllAppearance([&](wi::HairParticleSystem& h){ h.length = value; });
	else if (p == "width")      forAllAppearance([&](wi::HairParticleSystem& h){ h.width = value; });
	else if (p == "stiffness")  forAllAppearance([&](wi::HairParticleSystem& h){ h.stiffness = value; });
	else if (p == "drag")       forAllAppearance([&](wi::HairParticleSystem& h){ h.drag = value; });
	else if (p == "viewdist")   forAllAppearance([&](wi::HairParticleSystem& h){ h.viewDistance = value; });
	else if (p == "segments")   forAllAppearance([&](wi::HairParticleSystem& h){ h.segmentCount = (uint32_t)(value < 1.0f ? 1.0f : value); });
	else if (p == "billboards") forAllAppearance([&](wi::HairParticleSystem& h){ h.billboardCount = (uint32_t)(value < 1.0f ? 1.0f : value); });
	else if (p == "blades")     g_grassBladesPerVertex = (uint32_t)(value < 1.0f ? 1.0f : value);
	else if (p == "maxstrands") g_grassMaxStrands = (uint32_t)(value < 1.0f ? 1.0f : value);
	else if (p == "lodchunks")  g_grassLODChunks = value;             // grass radius in chunk-distances
	else if (p == "sss")        forAllMaterials([&](wi::scene::MaterialComponent& m){ m.SetSubsurfaceScatteringAmount(value); });
	else if (p == "alpha")      forAllMaterials([&](wi::scene::MaterialComponent& m){ m.SetAlphaRef(value); });
	else if (p == "tintr")      forAllMaterials([&](wi::scene::MaterialComponent& m){ m.baseColor.x = value; m.SetDirty(); });
	else if (p == "tintg")      forAllMaterials([&](wi::scene::MaterialComponent& m){ m.baseColor.y = value; m.SetDirty(); });
	else if (p == "tintb")      forAllMaterials([&](wi::scene::MaterialComponent& m){ m.baseColor.z = value; m.SetDirty(); });
	else if (p == "sssr")       forAllMaterials([&](wi::scene::MaterialComponent& m){ m.SetSubsurfaceScatteringColor(XMFLOAT3(value, m.subsurfaceScattering.y, m.subsurfaceScattering.z)); });
	else if (p == "sssg")       forAllMaterials([&](wi::scene::MaterialComponent& m){ m.SetSubsurfaceScatteringColor(XMFLOAT3(m.subsurfaceScattering.x, value, m.subsurfaceScattering.z)); });
	else if (p == "sssb")       forAllMaterials([&](wi::scene::MaterialComponent& m){ m.SetSubsurfaceScatteringColor(XMFLOAT3(m.subsurfaceScattering.x, m.subsurfaceScattering.y, value)); });
	g_grassRebuildRequested = true;
}


// Lazily build the brush-cursor decal entity (material + texture + decal + transform). One-time;
// reused every frame thereafter. Texture is the procedurally-generated 1024-px brush_ring.png.
static void SetupBrushCursor()
{
	using namespace wi::scene;
	auto& scene = wi::scene::GetScene();

	// Resilience: scene wipes on level load destroy our entity even though our static flag stays
	// true. Verify all three components still exist; if not, treat this as a re-init and rebuild.
	if (g_brushCursorSetup)
	{
		if (scene.materials.GetComponent(g_brushCursorEntity) &&
			scene.transforms.GetComponent(g_brushCursorEntity) &&
			scene.decals.GetComponent(g_brushCursorEntity))
		{
			return;
		}
		g_brushCursorSetup = false;
	}
	if (wickedTerrainExeDir.empty()) return; // wait for Init() to capture the EXE dir

	g_brushCursorEntity = wi::ecs::CreateEntity();

	char texPath[512];
	sprintf_s(texPath, "%s/Files/editors/gfx/brush_ring.png", wickedTerrainExeDir.c_str());

	MaterialComponent& mat = scene.materials.Create(g_brushCursorEntity);
	mat.textures[MaterialComponent::BASECOLORMAP].name = texPath;
	mat.baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f); // start hidden via alpha multiplier
	mat.SetCastShadow(false);
	mat.SetTextureStreamingDisabled(true);
	mat.CreateRenderData();

	TransformComponent& tx = scene.transforms.Create(g_brushCursorEntity);
	// Wicked decals project along local -Z. Rotate the entity -90 deg around X so its local -Z axis
	// points to world -Y (straight down into the terrain).
	XMVECTOR rot = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), -XM_PIDIV2);
	XMStoreFloat4(&tx.rotation_local, rot);
	tx.SetDirty();

	DecalComponent& decal = scene.decals.Create(g_brushCursorEntity);
	decal.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // material baseColor multiplies texture; this is multiplied in
	decal.SetBaseColorOnlyAlpha(false);             // RGBA from texture (we want the green to come through)

	g_brushCursorSetup = true;
}


namespace GGTerrain
{

void GGTerrainWicked_SetBrushCursor(bool visible, float x, float y, float z, float size)
{
	SetupBrushCursor();
	if (!g_brushCursorSetup) return;

	auto& scene = wi::scene::GetScene();
	auto* tx = scene.transforms.GetComponent(g_brushCursorEntity);
	auto* mat = scene.materials.GetComponent(g_brushCursorEntity);
	if (!tx || !mat)
	{
		// Component lookup failed — entity got wiped between SetupBrushCursor and here. Force the
		// next frame's SetupBrushCursor to rebuild from scratch instead of silently failing forever.
		g_brushCursorSetup = false;
		return;
	}

	if (visible && size > 0.0f)
	{
		tx->translation_local = XMFLOAT3(x, y, z);
		tx->scale_local = XMFLOAT3(size, size, size);
		tx->SetDirty();
		mat->baseColor.w = 1.0f;
	}
	else
	{
		mat->baseColor.w = 0.0f;
	}
	mat->SetDirty();
}

void GGTerrainWicked_Init()
{
	// Capture EXE directory for resolving texture paths (CWD-independent)
	wickedTerrainExeDir = wi::helper::GetDirectoryFromPath(wi::helper::GetExecutablePath());
	// Remove trailing slash if present
	if (!wickedTerrainExeDir.empty() && (wickedTerrainExeDir.back() == '/' || wickedTerrainExeDir.back() == '\\'))
		wickedTerrainExeDir.pop_back();

	auto& scene = wi::scene::GetScene();

	// Create entity and register terrain in scene (so renderer calls UpdateVirtualTexturesGPU)
	wickedTerrainEntity = wi::ecs::CreateEntity();
	wi::terrain::Terrain& terrain = scene.terrains.Create(wickedTerrainEntity);
	terrain.scene = &scene;
	terrain.terrainEntity = wickedTerrainEntity;

	terrain.SetCenterToCamEnabled(true);
	terrain.SetRemovalEnabled(true);
	terrain.SetGrassEnabled(false);       // Phase 0: no grass yet
	terrain.SetPhysicsEnabled(false);      // keep Bullet physics from old terrain
	terrain.chunk_scale = 80.0f;          // ~10560 units/chunk; high-res ring now ~535m (was ~268m at 80)
	terrain.generation = 14;               // cover ~147840 units each direction, more lead for fast camera movement
	terrain.lod_bias = 0.0f;              // hold higher mesh LOD one step further out (inch-scale world)
	terrain.bottomLevel = -20000.0f;       // match GG height range
	terrain.topLevel = 20000.0f;

	// Phase 1: Height modifier feeds GG's heightmap+fractal+sculpt+flat area data
	heightModifier = std::make_shared<GGHeightModifier>();
	heightModifier->bottomLevel = terrain.bottomLevel;
	heightModifier->topLevel = terrain.topLevel;
	terrain.modifiers.push_back(heightModifier);

	terrain.Generation_Restart();
	wickedTerrainInitialised = true;
	wickedTerrainMaterialsSetup = false;
}

void GGTerrainWicked_Update(const wi::scene::CameraComponent& camera)
{
	if (!wickedTerrainInitialised) return;
	wi::terrain::Terrain* terrain = GetWickedTerrain();
	if (!terrain) return;

	// U key: toggle wireframe overlay for terrain chunk visualization
	extern bool GGTerrain_GetKeyPressed(uint8_t key);
	static int wickedWireframeMode = 0;
	if (GGTerrain_GetKeyPressed(0x55)) // GGKEY_U
	{
		wickedWireframeMode = 1 - wickedWireframeMode;
		if (wickedWireframeMode)
			wi::renderer::SetWireframeMode(wi::renderer::WIREFRAME_OVERLAY);
		else
			wi::renderer::SetWireframeMode(wi::renderer::WIREFRAME_DISABLED);
	}

	// I key: toggle normal visualization
	static int wickedNormalVisMode = 0;
	if (GGTerrain_GetKeyPressed(0x49)) // GGKEY_I
	{
		wickedNormalVisMode = 1 - wickedNormalVisMode;
		wi::renderer::SetDebugNormalVis(wickedNormalVisMode != 0); // VS could not find this!!
	}

	// O key: toggle terrain rendering on/off
	static bool wickedTerrainHidden = false;
	if (GGTerrain_GetKeyPressed(0x4F)) // GGKEY_O
	{
		wickedTerrainHidden = !wickedTerrainHidden;
		if (wickedTerrainHidden)
		{
			// Hide all existing chunks and stop VT GPU work
			for (auto& [chunk, chunk_data] : terrain->chunks)
			{
				if (chunk_data.entity != wi::ecs::INVALID_ENTITY)
				{
					wi::scene::ObjectComponent* obj = terrain->scene->objects.GetComponent(chunk_data.entity);
					if (obj) obj->SetRenderable(false);
				}
			}
			terrain->virtual_textures_in_use.clear();
		}
		else
		{
			// Re-show all chunks — Generation_Update will resume next frame
			for (auto& [chunk, chunk_data] : terrain->chunks)
			{
				if (chunk_data.entity != wi::ecs::INVALID_ENTITY)
				{
					wi::scene::ObjectComponent* obj = terrain->scene->objects.GetComponent(chunk_data.entity);
					if (obj) obj->SetRenderable(true);
				}
			}
		}
	}

	// P key: toggle profiler and dump to file when disabling
	static bool perfProfilerEnabled = false;
	if (GGTerrain_GetKeyPressed(0x50)) // VK_P
	{
		perfProfilerEnabled = !perfProfilerEnabled;
		wi::profiler::SetEnabled(perfProfilerEnabled);
		if (perfProfilerEnabled)
		{
			wi::backlog::post("[Perf] Profiler ENABLED — press P again to dump and disable");
		}
		else
		{
			std::string profText = GGPerf_GetCachedProfilerText();
			if (!profText.empty())
			{
				std::string dumpPath = wickedTerrainExeDir + "/terrain_perf.log";
				FILE* f = nullptr;
				fopen_s(&f, dumpPath.c_str(), "w");
				if (f) { fputs(profText.c_str(), f); fclose(f); }
				wi::backlog::post("[Perf] Profiler DISABLED — dumped to terrain_perf.log");
			}
			else
			{
				wi::backlog::post("[Perf] Profiler DISABLED — no data to dump");
			}
		}
	}

	// 1 key: toggle shadows
	static int perfShadowsOff = 0;
	if (GGTerrain_GetKeyPressed(0x31)) // VK_1
	{
		perfShadowsOff = 1 - perfShadowsOff;
		wi::RenderPath3D* rp = GGPerf_GetRenderPath();
		if (rp) rp->setShadowsEnabled(!perfShadowsOff);
		wi::backlog::post(perfShadowsOff ? "[Perf] Shadows OFF" : "[Perf] Shadows ON");
	}

	// 2 key: toggle AO
	static int perfAOOff = 0;
	if (GGTerrain_GetKeyPressed(0x32)) // VK_2
	{
		perfAOOff = 1 - perfAOOff;
		wi::RenderPath3D* rp = GGPerf_GetRenderPath();
		if (rp) rp->setAO(perfAOOff ? wi::RenderPath3D::AO_DISABLED : wi::RenderPath3D::AO_MSAO);
		wi::backlog::post(perfAOOff ? "[Perf] AO OFF" : "[Perf] AO ON (MSAO)");
	}

	// 3 key: toggle bloom
	static int perfBloomOff = 0;
	if (GGTerrain_GetKeyPressed(0x33)) // VK_3
	{
		perfBloomOff = 1 - perfBloomOff;
		wi::RenderPath3D* rp = GGPerf_GetRenderPath();
		if (rp) rp->setBloomEnabled(!perfBloomOff);
		wi::backlog::post(perfBloomOff ? "[Perf] Bloom OFF" : "[Perf] Bloom ON");
	}

	// 4 key: toggle volumetric clouds
	static int perfCloudsOff = 0;
	if (GGTerrain_GetKeyPressed(0x34)) // VK_4
	{
		perfCloudsOff = 1 - perfCloudsOff;
		auto& weather = GGPerf_GetWeather();
		weather.SetVolumetricClouds(!perfCloudsOff);
		wi::backlog::post(perfCloudsOff ? "[Perf] Volumetric Clouds OFF" : "[Perf] Volumetric Clouds ON");
	}

	// 5 key: toggle realistic sky
	static int perfSkyOff = 0;
	if (GGTerrain_GetKeyPressed(0x35)) // VK_5
	{
		perfSkyOff = 1 - perfSkyOff;
		auto& weather = GGPerf_GetWeather();
		weather.SetRealisticSky(!perfSkyOff);
		wi::backlog::post(perfSkyOff ? "[Perf] Realistic Sky OFF" : "[Perf] Realistic Sky ON");
	}

	// 6 key: toggle light shafts
	static int perfLightShaftsOff = 0;
	if (GGTerrain_GetKeyPressed(0x36)) // VK_6
	{
		perfLightShaftsOff = 1 - perfLightShaftsOff;
		wi::RenderPath3D* rp = GGPerf_GetRenderPath();
		if (rp) rp->setLightShaftsEnabled(!perfLightShaftsOff);
		wi::backlog::post(perfLightShaftsOff ? "[Perf] Light Shafts OFF" : "[Perf] Light Shafts ON");
	}

	// 7 key: toggle reflections
	static int perfReflectionsOff = 0;
	if (GGTerrain_GetKeyPressed(0x37)) // VK_7
	{
		perfReflectionsOff = 1 - perfReflectionsOff;
		wi::RenderPath3D* rp = GGPerf_GetRenderPath();
		if (rp) rp->setReflectionsEnabled(!perfReflectionsOff);
		wi::backlog::post(perfReflectionsOff ? "[Perf] Reflections OFF" : "[Perf] Reflections ON");
	}

	// 8 key: toggle volumetric lights on sun
	static int perfVolLightsOff = 0;
	if (GGTerrain_GetKeyPressed(0x38)) // VK_8
	{
		perfVolLightsOff = 1 - perfVolLightsOff;
		wi::scene::LightComponent* lightSun = GGPerf_GetSunLight();
		if (lightSun) lightSun->SetVolumetricsEnabled(!perfVolLightsOff);
		wi::backlog::post(perfVolLightsOff ? "[Perf] Sun Volumetrics OFF" : "[Perf] Sun Volumetrics ON");
	}

	// G key: toggle grass on/off (A/B + perf). Hides existing grass via layerMask and pauses new
	// grass creation; re-enabling shows it again and resumes creation for any new chunks.
	if (GGTerrain_GetKeyPressed(0x47)) // GGKEY_G
	{
		wickedGrassEnabled = !wickedGrassEnabled;
		auto& gscene = wi::scene::GetScene();
		for (auto& kv : grassChunkKeyToGrassEntities)
		{
			for (uint32_t t = 0; t < GGGRASS_NUM_TYPES; t++)
			{
				wi::ecs::Entity e = kv.second.perType[t];
				if (e == wi::ecs::INVALID_ENTITY) continue;
				wi::HairParticleSystem* hair = gscene.hairs.GetComponent(e);
				if (hair) hair->layerMask = wickedGrassEnabled ? ~0u : 0u;
			}
		}
		wi::backlog::post(wickedGrassEnabled ? "[Grass] ON" : "[Grass] OFF");
	}

	// Skip all terrain work when hidden (Generation_Update, VT CPU/GPU, blendmap painting)
	if (wickedTerrainHidden) return;

	// Phase 2: Lazy material setup on first update (after level load has set render params)
	if (!wickedTerrainMaterialsSetup)
	{
		SetupWickedTerrainMaterials();
	}

	// Let the VT system run — generates chunks, creates atlas, blends materials.
	terrain->Generation_Update(camera);

	// Phase 3: Process terrain chunks for painted material blendmaps AFTER Generation_Update.
	// Must run after so the generation pipeline has finished creating default blendmaps for
	// new/regenerated chunks. Painting before generation completes gets overwritten by the
	// height/slope blending stage. Our VT invalidation is picked up on the next frame's
	// Generation_Update (1-frame delay, but avoids permanent corruption from race condition).
	if (wickedTerrainMaterialsSetup && maxPaintedSlot >= 0)
	{
		ProcessPaintedChunkBlendmaps(terrain);
	}

	// Grass: set up the material/template once, then grow grass on chunks from the painted grass
	// map. Runs after blendmap processing (generation already cancelled = safe to add entities).
	if (!wickedGrassSetup)
	{
		SetupWickedGrass();
	}
	if (wickedGrassEnabled)
	{
		// Editor paint mutates pGrassMap via GGGrass_Update_Painting. The Wicked grass renderer
		// caches per-chunk hair entities and only rebuilds them on tier/entity changes — so it
		// can't notice a paint stroke on its own. GGGrass tracks the *set* of chunk keys the
		// brush footprint touched (not the bounding box of the whole stroke), so we only erase
		// tier records for chunks the user actually painted into. Chunks the cursor merely passed
		// over without writing keep their grass intact and don't reshuffle.
		float gridChunkStride = (wi::terrain::chunk_width - 1) * terrain->chunk_scale;
		GGGrass::GGGrass_SetChunkStride(gridChunkStride);
		static wi::vector<uint64_t> dirtyChunks; // reused across frames; cleared per drain
		dirtyChunks.clear();
		if (GGGrass::GGGrass_TakeDirtyChunks(dirtyChunks))
		{
			for (uint64_t key : dirtyChunks)
			{
				// Erase the tier record so ProcessGrassChunks sees a tier change and rebuilds it.
				grassChunkKeyToTier.erase(key);
			}
		}
		// SET_GRASS knob changes still flow through the full-rebuild path.
		if (g_grassRebuildRequested)
		{
			ForceGrassRebuild();
			g_grassRebuildRequested = false;
		}
		ProcessGrassChunks(terrain, camera.Eye);
	}
}

void GGTerrainWicked_Shutdown()
{
	if (!wickedTerrainInitialised) return;
	wi::terrain::Terrain* terrain = GetWickedTerrain();
	if (terrain)
	{
		terrain->Generation_Cancel();
		// Remove all chunk entities from the scene
		for (auto& [chunk, chunk_data] : terrain->chunks)
		{
			if (chunk_data.entity != wi::ecs::INVALID_ENTITY)
			{
				terrain->scene->Entity_Remove(chunk_data.entity);
			}
		}
		terrain->chunks.clear();
		terrain->modifiers.clear();
	}
	heightModifier.reset();
	if (wickedTerrainEntity != wi::ecs::INVALID_ENTITY)
	{
		wi::scene::GetScene().Entity_Remove(wickedTerrainEntity);
		wickedTerrainEntity = wi::ecs::INVALID_ENTITY;
	}
	wickedTerrainInitialised = false;
	wickedTerrainMaterialsSetup = false;
	maxPaintedSlot = -1;
	processedChunkKeys.clear();
	chunkKeyToEntity.clear();
}

void GGTerrainWicked_OnPaintDataChanged()
{
	// Called when pMaterialMap is updated (level load or paint brush).
	if (!wickedTerrainInitialised) return;

	// Always clear so chunks get repainted with fresh pixel data
	processedChunkKeys.clear();
	chunkKeyToEntity.clear();

	// If materials aren't set up yet, nothing more to do (pending first setup)
	if (!wickedTerrainMaterialsSetup) return;

	// Check if paint data contains materials not yet in our slot mapping
	const uint8_t* matMap = GGTerrain::GGTerrain_GetMaterialMapPtr();
	int mapRes = GGTerrain::GGTerrain_GetMaterialMapResolution();
	if (!matMap || mapRes <= 0) return;

	bool needsNewMaterials = false;
	for (int i = 0; i < mapRes * mapRes && !needsNewMaterials; i++)
	{
		uint8_t val = matMap[i];
		if (val > 0 && val <= GGTERRAIN_MAX_SOURCE_TEXTURES)
			if (materialToSlot[val - 1] < 0)
				needsNewMaterials = true;
	}

	if (needsNewMaterials)
		wickedTerrainMaterialsSetup = false; // Will trigger full re-setup + restart
}

} // namespace GGTerrain
