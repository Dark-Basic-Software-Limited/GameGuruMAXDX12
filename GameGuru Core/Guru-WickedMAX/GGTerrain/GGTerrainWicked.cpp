#include "GGTerrain.h"
#include "GGTerrainWicked.h"
#include "GGGrass.h"
#include "GGTrees.h"
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

// Path A: DX11-style CPU-computed blend weights (per-vertex) that override Wicked's built-in
// smoothstep-based region weights. Wicked's 4-slot (base/slope/low/high) blend can't reproduce
// GG's per-layer height ramps (see project_terrain_texture_mismatch.md). This system runs after
// Generation_Update and rewrites blendmap_layers[0..maxAutoBlendSlot] to match GG's DX11 shader
// logic exactly, giving mountains their green mid-altitude material instead of grey rock bleed.
static int g_layer1MaterialSlot = -1;                             // Extra slot for layer[1] material (GG mat 0 by default)
static std::unordered_set<uint64_t> dx11BlendProcessedKeys;       // Chunks already given DX11-shape auto weights
static std::unordered_map<uint64_t, wi::ecs::Entity> dx11BlendChunkKeyToEntity;

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
	wi::ecs::Entity perType[GGGRASS_TOTAL_REAL_TYPES];
	ChunkGrassEntities()
	{
		for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++) perType[t] = wi::ecs::INVALID_ENTITY;
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
// Manual outer-ring override in chunk-distances, used by the SET_GRASS lodchunks automation key
// when non-zero. Otherwise the outer ring is derived from gggrass_global_params.lod_dist
// (the editor's Grass Draw Distance slider). Zero = follow slider.
static float    g_grassLODChunksOverride = 0.0f;
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
	// Wicked's built-in slope/altitude weights are overwritten every frame by
	// ApplyDX11StyleAutoBlend below; these region values are left at safe defaults so any
	// vertex the DX11 override misses (should be none) still gets a plausible base blend.
	terrain->region1 = GGTerrain::ggterrain_global_render_params.slopeEnd[0];
	terrain->region2 = 2.0f;
	terrain->region3 = 1.0f;

	// Phase 3: Initialize materialToSlot lookup — maps GG 0-based material index to Wicked blendmap layer
	for (int i = 0; i < GGTERRAIN_MAX_SOURCE_TEXTURES; i++)
		materialToSlot[i] = -1;

	// Map the 4 auto materials to their fixed slots (0-3)
	materialToSlot[baseMat] = wi::terrain::MATERIAL_BASE;
	materialToSlot[slopeMat] = wi::terrain::MATERIAL_SLOPE;
	materialToSlot[lowMat] = wi::terrain::MATERIAL_LOW_ALTITUDE;
	materialToSlot[highMat] = wi::terrain::MATERIAL_HIGH_ALTITUDE;
	maxPaintedSlot = wi::terrain::MATERIAL_HIGH_ALTITUDE;  // 3

	// Path A: register GG layer[1]'s material as slot 4 so the CPU-computed DX11 blend can
	// place its weight there. This is the "mid altitude" band material (heights 180-360 in
	// TESTPRO1 island) that DX11 fully REPLACES base with above height 360 — Wicked's 4-slot
	// auto blend has no equivalent. Skip if the layer[1] material already occupies one of
	// the 4 auto slots (nothing to add).
	int layer1Mat = GGTerrain::ggterrain_global_render_params.layerMatIndex[1] & 0xFF;
	g_layer1MaterialSlot = -1;
	int numExtraMaterials = 0;
	if (materialToSlot[layer1Mat] < 0)
	{
		int newSlot = wi::terrain::MATERIAL_COUNT;  // 4
		materialToSlot[layer1Mat] = newSlot;
		while ((int)terrain->materialEntities.size() <= newSlot)
			terrain->materialEntities.push_back(wi::ecs::INVALID_ENTITY);
		terrain->materialEntities[newSlot] = wi::ecs::CreateEntity();
		scene.Component_Attach(terrain->materialEntities[newSlot], wickedTerrainEntity);
		SetupTerrainMaterial(scene, terrain->materialEntities[newSlot], layer1Mat);
		maxPaintedSlot = newSlot;
		g_layer1MaterialSlot = newSlot;
		numExtraMaterials = 1;
	}
	else
	{
		g_layer1MaterialSlot = materialToSlot[layer1Mat];
	}

	// Scan material map for unique painted materials not already in auto slots
	const uint8_t* matMap = GGTerrain::GGTerrain_GetMaterialMapPtr();
	int mapRes = GGTerrain::GGTerrain_GetMaterialMapResolution();

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
	dx11BlendProcessedKeys.clear();
	dx11BlendChunkKeyToEntity.clear();

	// Restart generation to pick up all materials (auto + painted)
	// Generation_Restart() deep-copies the materials internally
	terrain->Generation_Restart();
	wickedTerrainMaterialsSetup = true;

	wi::backlog::post(std::string("GGTerrainWicked: materials setup complete (" +
		std::to_string(numExtraMaterials) + " extra painted materials, maxSlot=" +
		std::to_string(maxPaintedSlot) + ")").c_str());
}

// Path A: recompute per-vertex blendmap weights using DX11's terrain shader formula,
// replacing Wicked's built-in smoothstep(0, regionN, x) blend that we can't reproduce with a
// single scalar threshold. See project_terrain_texture_mismatch.md for the analysis.
//
// DX11 formula (from GGTerrainPageGenPS.hlsl:216-246):
//   finalSurface = base
//   for i in 0..5:
//     t = clamp((height - layer[i].start) / (layer[i].end - layer[i].start), 0, 1)
//     if t >= 1: finalSurface = REPLACE with layer[i]
//     elif t > 0: finalSurface = LERP(finalSurface, layer[i], t)
//   normaly = 1 - abs(normal.y)
//   for i in 0..2:
//     t = clamp((normaly - slope[i].start) / (slope[i].end - slope[i].start), 0, 1)
//     if t >= 1: finalSurface = REPLACE with slope[i]
//     elif t > 0: finalSurface = LERP(finalSurface, slope[i], t)
//
// Translated to Wicked's weight system: track (w_base, w_slope, w_low, w_high, w_layer1)
// and apply each layer/slope as a proportional replace: all_weights *= (1-t), target += t.
// Runs BEFORE ProcessPaintedChunkBlendmaps so painted materials still override correctly.
static void ApplyDX11StyleAutoBlend(wi::terrain::Terrain* terrain)
{
	if (!terrain) return;
	auto& scene = wi::scene::GetScene();
	const auto& rp = GGTerrain::ggterrain_global_render_params;
	float chunkStride = (wi::terrain::chunk_width - 1) * terrain->chunk_scale;

	// Pre-compute reciprocal widths of layer/slope ramps.
	float layerRcpWidth[5];
	float layerStart[5];
	for (int i = 0; i < 5; i++)
	{
		layerStart[i] = rp.layerStartHeight[i];
		float w = rp.layerEndHeight[i] - rp.layerStartHeight[i];
		layerRcpWidth[i] = (w > 0.001f) ? (1.0f / w) : 0.0f;
	}
	float slopeRcpWidth[2];
	float slopeStart[2];
	for (int i = 0; i < 2; i++)
	{
		slopeStart[i] = rp.slopeStart[i];
		float w = rp.slopeEnd[i] - rp.slopeStart[i];
		slopeRcpWidth[i] = (w > 0.001f) ? (1.0f / w) : 0.0f;
	}

	// Which Wicked slots each GG material family lands in.
	int baseSlot  = wi::terrain::MATERIAL_BASE;           // baseLayerMaterial (typically mat 17 grass)
	int slopeSlot = wi::terrain::MATERIAL_SLOPE;          // slopeMatIndex[0]   (typically mat 4 rock)
	int lowSlot   = wi::terrain::MATERIAL_LOW_ALTITUDE;   // layerMatIndex[0]   (mat 2 sand on TESTPRO1)
	int highSlot  = wi::terrain::MATERIAL_HIGH_ALTITUDE;  // layerMatIndex[2]   (mat 20 rock on TESTPRO1)
	int layer1Slot = g_layer1MaterialSlot;                 // layerMatIndex[1]   (mat 0 mid-material) or -1

	// Phase 1: Scan scene.objects (main-thread safe) to collect chunks that haven't had DX11 blend applied
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
		if (mesh->vertex_normals.size() != wi::terrain::vertexCount) continue;

		wi::ecs::Entity entity = scene.objects.GetEntity(oi);
		const wi::scene::TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (!transform) continue;

		int32_t cx = (int32_t)std::round(transform->world._41 / chunkStride);
		int32_t cz = (int32_t)std::round(transform->world._43 / chunkStride);
		uint64_t key = MakeChunkKey(cx, cz);

		if (dx11BlendProcessedKeys.count(key))
		{
			// Same regen-detection pattern as ProcessPaintedChunkBlendmaps
			auto it = dx11BlendChunkKeyToEntity.find(key);
			if (it != dx11BlendChunkKeyToEntity.end() && it->second != entity)
			{
				dx11BlendProcessedKeys.erase(key);
				dx11BlendChunkKeyToEntity.erase(it);
			}
			else continue;
		}

		pending.push_back({ entity, cx, cz, mesh, transform });
	}

	if (pending.empty()) return;

	// Phase 2: Cancel generation for safe chunk data access, then rewrite blendmaps
	terrain->Generation_Cancel();

	int chunksModified = 0;
	int slotsNeeded = std::max(4, (layer1Slot >= 0 ? layer1Slot + 1 : 4));

	for (auto& pc : pending)
	{
		uint64_t key = MakeChunkKey(pc.cx, pc.cz);
		wi::terrain::ChunkData* chunk_data = nullptr;
		for (auto& [chunk, cd] : terrain->chunks)
		{
			if (cd.entity == pc.entity) { chunk_data = &cd; break; }
		}
		if (!chunk_data) continue;
		if (chunk_data->blendmap_layers.empty()) continue;  // generation not finished yet

		dx11BlendProcessedKeys.insert(key);
		dx11BlendChunkKeyToEntity[key] = pc.entity;

		// Ensure enough blendmap layers exist for all slots we might write to
		for (int i = 0; i < slotsNeeded; i++)
			chunk_data->enable_blendmap_layer(i);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&pc.transform->world);

		for (size_t vi = 0; vi < wi::terrain::vertexCount; vi++)
		{
			XMVECTOR wp = XMVector3Transform(XMLoadFloat3(&pc.mesh->vertex_positions[vi]), worldMatrix);
			XMFLOAT3 worldPos;
			XMStoreFloat3(&worldPos, wp);
			float height = worldPos.y;

			const XMFLOAT3& normal = pc.mesh->vertex_normals[vi];
			float normaly = 1.0f - fabsf(normal.y);

			// Per-slot weight accumulator. We only care about the 5 auto slots; anything else
			// stays zero and will be overwritten by ProcessPaintedChunkBlendmaps if painted.
			float w[8] = { 0 };
			w[baseSlot] = 1.0f;

			// Layer 0: replace/blend at height ramp
			auto applyLayer = [&](int layerIdx, int targetSlot)
			{
				if (layerRcpWidth[layerIdx] == 0.0f) return; // start == end -> unused
				float t = (height - layerStart[layerIdx]) * layerRcpWidth[layerIdx];
				if (t <= 0.0f) return;
				if (t > 1.0f) t = 1.0f;
				for (int s = 0; s < 8; s++) w[s] *= (1.0f - t);
				w[targetSlot] += t;
			};
			auto applySlope = [&](int slopeIdx, int targetSlot)
			{
				if (slopeRcpWidth[slopeIdx] == 0.0f) return;
				float t = (normaly - slopeStart[slopeIdx]) * slopeRcpWidth[slopeIdx];
				if (t <= 0.0f) return;
				if (t > 1.0f) t = 1.0f;
				for (int s = 0; s < 8; s++) w[s] *= (1.0f - t);
				w[targetSlot] += t;
			};

			// DX11 iteration order: all layers, then all slopes.
			applyLayer(0, lowSlot);
			if (layer1Slot >= 0) applyLayer(1, layer1Slot);
			applyLayer(2, highSlot);
			// layers 3, 4 in TESTPRO1 have start==end (59055 marker) -> no-op via layerRcpWidth==0
			applyLayer(3, highSlot);
			applyLayer(4, highSlot);
			applySlope(0, slopeSlot);
			applySlope(1, slopeSlot);

			chunk_data->blendmap_layers[baseSlot ].pixels[vi] = (uint8_t)(w[baseSlot ] * 255.0f);
			chunk_data->blendmap_layers[slopeSlot].pixels[vi] = (uint8_t)(w[slopeSlot] * 255.0f);
			chunk_data->blendmap_layers[lowSlot  ].pixels[vi] = (uint8_t)(w[lowSlot  ] * 255.0f);
			chunk_data->blendmap_layers[highSlot ].pixels[vi] = (uint8_t)(w[highSlot ] * 255.0f);
			if (layer1Slot >= 0 && layer1Slot < (int)chunk_data->blendmap_layers.size())
				chunk_data->blendmap_layers[layer1Slot].pixels[vi] = (uint8_t)(w[layer1Slot] * 255.0f);
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
		wi::backlog::post(std::string("GGTerrainWicked: DX11-style auto blend on " +
			std::to_string(chunksModified) + " chunks (layer1Slot=" +
			std::to_string(layer1Slot) + ")").c_str());
	}
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
static wi::scene::MaterialComponent g_grassMaterials[GGGRASS_TOTAL_REAL_TYPES];
static bool                         g_grassMaterialReady[GGGRASS_TOTAL_REAL_TYPES] = {};

// Build one cached grass material from Files/grassbank/<filename>. Returns nullptr on bad index.
// Kelp/seaweed sprites have authored _normal.dds siblings; we wire those in when present.
static wi::scene::MaterialComponent* BuildGrassMaterial(uint32_t typeIdx)
{
	if (typeIdx >= GGGRASS_TOTAL_REAL_TYPES) return nullptr;
	if (g_grassMaterialReady[typeIdx]) return &g_grassMaterials[typeIdx];

	// Stage B.9: dispatch between stock (typeIdx < GGGRASS_CUSTOM_REAL_TYPE_BASE) and custom
	// (typeIdx >= base). Stock reads the DDS path from the built-in grassFiles[] table via
	// GGGrass_GetTypeInfo; custom reads the user-registered filename via GGGrass_GetCustomSlotFilename
	// (slot = typeIdx - 24, matching the paint-side encoding).
	const char* dds_relpath = nullptr;
	bool isCustom = (typeIdx >= (uint32_t)GGGRASS_CUSTOM_REAL_TYPE_BASE);
	if (isCustom)
	{
		int slot = (int)typeIdx - 24;
		dds_relpath = GGGrass::GGGrass_GetCustomSlotFilename(slot);
		if (!dds_relpath || !dds_relpath[0]) return nullptr;
	}
	else
	{
		const GGGrass::GrassTypeInfo* info = GGGrass::GGGrass_GetTypeInfo(typeIdx);
		if (!info || !info->filename) return nullptr;
		dds_relpath = info->filename;
	}

	// Stock paths are just the DDS filename (relative to Files/grassbank/); custom paths from
	// sGrassTextures[] are relative to Files/ (e.g. "grassbank/foo.dds" or "user/mygrass.dds").
	// Both variants live under Files/ so we build the absolute path off the EXE-dir + Files/.
	char colorPath[512];
	if (isCustom)
		sprintf_s(colorPath, "%s/Files/%s", wickedTerrainExeDir.c_str(), dds_relpath);
	else
		sprintf_s(colorPath, "%s/Files/grassbank/%s", wickedTerrainExeDir.c_str(), dds_relpath);

	// info is only used by stock for the normal-map sibling detection below.
	const GGGrass::GrassTypeInfo* info = isCustom ? nullptr : GGGrass::GGGrass_GetTypeInfo(typeIdx);
	const char* nrmSrcName = isCustom ? dds_relpath : (info ? info->filename : nullptr);

	// Build a normal-map path if the sprite name ends in "_color.dds" (kelp/seaweed convention).
	// Otherwise leave normals unset — the alpha-cutout blade silhouette doesn't need them.
	char normalPath[512] = {0};
	bool haveNormal = false;
	const char* colorSuffix = nrmSrcName ? strstr(nrmSrcName, "_color.dds") : nullptr;
	if (colorSuffix)
	{
		size_t prefixLen = (size_t)(colorSuffix - nrmSrcName);
		char normalName[256];
		if (prefixLen < sizeof(normalName) - 16)
		{
			memcpy(normalName, nrmSrcName, prefixLen);
			memcpy(normalName + prefixLen, "_normal.dds", 12); // includes NUL
			if (isCustom)
				sprintf_s(normalPath, "%s/Files/%s", wickedTerrainExeDir.c_str(), normalName);
			else
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
static wi::HairParticleSystem g_grassAppearance[GGGRASS_TOTAL_REAL_TYPES];
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
// Per-type baseline blade length (units). Populated at the end of BuildGrassAppearance from
// whatever category-specific value each type ended up with; ApplyGrassScale reads this and
// writes `a.length = baseline * (slider / GGGRASS_SCALE)` so the Grass Scale slider linearly
// scales every blade uniformly, matching DX11's `IN.position * grass_scale` in GGGrassVS.hlsl.
static float g_grassBaseLength[GGGRASS_TOTAL_REAL_TYPES] = {};

static void BuildGrassAppearance()
{
	if (g_grassAppearanceReady) return;
	uint32_t numTypes = GGGrass::GGGrass_GetNumTypes();
	if (numTypes > GGGRASS_TOTAL_REAL_TYPES) numTypes = GGGRASS_TOTAL_REAL_TYPES;

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
		a.atlas_rects.clear();     // each material is a single-sprite DDS — no atlas
		a.uniformity = 1.0f;
		// Sane default; ApplyGrassDrawDistance() (called below + on every slider change) overwrites
		// from gggrass_global_params.lod_dist so this only matters for the first-frame create window.
		a.viewDistance = 5000.0f;

		// Stage B.5: match DX11's posOrig formula — posOrig.x *= scaleFactor (legacy GGGrassVS.hlsl
		// line 45) — so SF drives WIDTH per type, not length. Length is anchored to DX11's uniform
		// grass_scale = 40 with a category factor for short/tall variation. Within-category visual
		// differences (e.g. course mat1 vs course mat3) are then purely texture-content driven,
		// which is what DX11 also shows.
		//
		// Effective rendered blade in Wicked:
		//   quad width  = hair.width * (texW / texH) * hair.length
		//   quad height = hair.length
		// With width = sf, length = ~40, and roughly-square DDS textures, that's ≈ 40*sf wide by
		// 40 tall — same numeric shape as DX11's `IN.position * grass_scale * SF`.
		switch (CategoryFor(t))
		{
		case GCAT_COURSE:
			a.length = 40.0f;       // 1.0 × DX11 grass_scale
			a.width = sf;           // SF=1.15
			a.billboardCount = 2;
			break;
		case GCAT_SHORT:
			a.length = 30.0f;       // 0.75 ×
			a.width = sf;           // SF=1.4 (wider stubby blade)
			a.billboardCount = 2;
			break;
		case GCAT_TALL:
			a.length = 56.0f;       // 1.4 ×
			a.width = sf;           // SF=0.87 (narrower tall blade)
			a.billboardCount = 2;
			break;
		case GCAT_WILD:
			a.length = 35.0f;       // 0.875 ×
			a.width = sf;           // SF=1.17
			a.billboardCount = 2;
			break;
		case GCAT_WEED:
			// DX11 parity: length = grass_scale = 40, width = sf. Prior sf*2 was overreach — the
			// DDS texture already conveys the stem shape at the SF value the artist encoded.
			a.length = 40.0f;
			a.width = sf;
			a.billboardCount = 1;
			a.stiffness = 12.0f;
			break;
		case GCAT_FLOWER:
			// DX11 parity: length = grass_scale = 40, width = sf. Same rationale as Weed —
			// texture-content variance replaces the prior width-multiplier hack.
			a.length = 40.0f;
			a.width = sf;
			a.billboardCount = 2;
			// viewDistance: ApplyGrassDrawDistance() halves the slider value for FLOWER (tiny features
			// benefit from earlier cull). No per-case assignment needed.
			break;
		case GCAT_KELP:
			// DX11 parity: same length as Course Grass (grass_scale = 40) with SF applied to width
			// only (matches legacy GGGrassVS.hlsl line 45: posOrig.x *= scaleFactor). Previous
			// length=75 + width=sf*6 was pure overreach — the kelp DDS itself already conveys the
			// broad-blade silhouette at length=40, and multiplying width by 6 gave ~10-ft towering
			// plants that tanked FPS via overdraw.
			a.length = 40.0f;
			a.width = sf;           // SF=0.47 -> thin narrow kelp blade
			a.billboardCount = 1;
			a.stiffness = 4.0f;
			a.drag = 0.8f;
			break;
		case GCAT_SEAWEED:
			// DX11 parity: same length as Course Grass, SF -> width only. Prior length=110 +
			// width=sf*4 was double overreach.
			a.length = 40.0f;
			a.width = sf;           // SF=0.7..0.9
			a.billboardCount = 1;
			a.stiffness = 3.0f;
			a.drag = 0.9f;
			break;
		}
	}

	// Stage B.9: custom palette slots (22..GGGRASS_MAX_PALETTE_SLOTS-1) map to real_types
	// (GGGRASS_CUSTOM_REAL_TYPE_BASE..). Each active slot gets an appearance built with a
	// default category (GCAT_WILD — medium blade, generic look) so it renders as reasonable
	// grass without requiring the user to specify per-slot tuning yet. SF is parsed from the
	// filename's "_SF_x.xx" suffix if present (matching the stock file convention); else 1.0.
	for (int slot = GGGRASS_CUSTOM_SLOT_BASE; slot < GGGRASS_MAX_PALETTE_SLOTS; slot++)
	{
		const char* fn = GGGrass::GGGrass_GetCustomSlotFilename(slot);
		if (!fn || !fn[0]) continue;
		uint32_t realType = (uint32_t)slot + 24;
		if (realType >= GGGRASS_TOTAL_REAL_TYPES) continue; // defensive

		// Parse SF from filename ("_SF_x.xx" suffix). Fall back to 1.0 if absent.
		float sf = 1.0f;
		{
			const char* p = strstr(fn, "_SF_");
			if (p)
			{
				float parsed = (float)atof(p + 4);
				if (parsed > 0.01f) sf = parsed;
			}
		}

		wi::HairParticleSystem& a = g_grassAppearance[realType];
		a = wi::HairParticleSystem();
		a.segmentCount = 1;
		a.randomness = 0.35f;
		a.stiffness = 9.0f;
		a.drag = 0.5f;
		a.atlas_rects.clear();
		a.uniformity = 1.0f;
		a.viewDistance = 5000.0f;   // overwritten by ApplyGrassDrawDistance() below + on slider drag
		a.length = 40.0f;           // DX11 grass_scale baseline (Course-Grass-equivalent height)
		a.width = sf;               // matches DX11 posOrig.x *= scaleFactor mapping
		a.billboardCount = 2;
	}

	// Snapshot the per-type baseline length AFTER all category tweaks have landed. ApplyGrassScale
	// reads from this array and writes `a.length = baseline * (slider / 40)`; every subsequent
	// BuildGrassAppearance call refreshes the snapshot so custom-slot rebuilds pick up their own
	// baseline before scaling is applied. Since quad width in the hair simulate CS is proportional
	// to length (`quad_width = hair.width * xHairAspect * hair.length`), scaling length alone gives
	// uniform blade scaling — matches DX11's `IN.position * grass_scale` in GGGrassVS.
	for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
	{
		g_grassBaseLength[t] = g_grassAppearance[t].length;
	}

	g_grassAppearanceReady = true;
}

// Populate the per-type appearance templates (one per entry in GGGrass::grassFiles[]). Materials
// are built lazily on first sighting (see BuildGrassMaterial), so no DDS is loaded until a chunk
// actually paints that type.
//
// Runtime assets: Files/grassbank/*.dds — already shipped as part of the GameGuru asset pack.
// Nothing else to deploy.
// Tracks last-applied Grass Draw Distance slider value so we can detect changes per frame and push
// the new viewDistance to live hair entities. -1 forces a sync on first call after setup.
static float g_grassPrevSliderInches = -1.0f;

// Sync the editor's Grass Draw Distance slider into per-entity viewDistance. Called after
// BuildGrassAppearance and from GGTerrainWicked_Update whenever the slider changes — so dragging
// the slider in the editor pulls every existing grass entity's cull radius along with it instead
// of waiting for chunks to be recreated.
static void ApplyGrassDrawDistance()
{
	// Per-strand visibility cull. Matches the DX11 grassRadius semantic (lod_dist + 2500), so the
	// slider directly represents "visible grass radius in inches above 2500". outerC in
	// ProcessGrassChunks always sits 1 chunk past this so chunks are created with their near edges
	// outside the cull — strands fade in gradually instead of whole chunks popping.
	const float viewDistInches = GGGrass::gggrass_global_params.lod_dist + 2500.0f;
	for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
	{
		// Flowers cull at half the radius — tiny features that don't read past mid-distance.
		float vd = (CategoryFor(t) == GCAT_FLOWER) ? viewDistInches * 0.5f : viewDistInches;
		g_grassAppearance[t].viewDistance = vd;
	}
	// Push to live entities (those already created on prior frames). New CREATEs pick up the
	// updated template directly.
	auto& scene = wi::scene::GetScene();
	for (size_t hi = 0; hi < scene.hairs.GetCount(); hi++)
	{
		wi::HairParticleSystem& h = scene.hairs[hi];
		if (h.grass_type == 0) continue;                  // not a GG grass entity (upstream hair)
		uint32_t typeIdx = h.grass_type - 1;
		if (typeIdx >= GGGRASS_TOTAL_REAL_TYPES) continue;
		h.viewDistance = g_grassAppearance[typeIdx].viewDistance;
	}
	g_grassPrevSliderInches = GGGrass::gggrass_global_params.lod_dist;
}

// Sync the editor's Grass Start/End Altitude sliders (and their underwater siblings + the water
// height) into per-entity CB values that the hair simulate CS reads for the altitude filter added
// alongside the slope filter (WICKED_ENGINE_CHANGES.md entry 1.5). Called after
// BuildGrassAppearance and from GGTerrainWicked_Update every frame — dragging any of the four
// sliders (or moving the water plane) pulls every existing grass entity's altitude band along
// live, without waiting for chunks to be recreated.
static void ApplyGrassAltitude()
{
	const float minH   = GGGrass::gggrass_global_params.min_height;
	const float maxH   = GGGrass::gggrass_global_params.max_height;
	const float minHU  = GGGrass::gggrass_global_params.min_height_underwater;
	const float maxHU  = GGGrass::gggrass_global_params.max_height_underwater;
	const float waterH = GGGrass::GGGrass_GetDefaultWaterHeight();

	// Update templates so newly created entities pick up current values on CREATE.
	for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
	{
		g_grassAppearance[t].grass_water_height           = waterH;
		g_grassAppearance[t].grass_min_height             = minH;
		g_grassAppearance[t].grass_max_height             = maxH;
		g_grassAppearance[t].grass_min_height_underwater  = minHU;
		g_grassAppearance[t].grass_max_height_underwater  = maxHU;
	}
	// Push to live entities so slider drags apply immediately.
	auto& scene = wi::scene::GetScene();
	for (size_t hi = 0; hi < scene.hairs.GetCount(); hi++)
	{
		wi::HairParticleSystem& h = scene.hairs[hi];
		if (h.grass_type == 0) continue;               // upstream Wicked hair — leave alone
		h.grass_water_height          = waterH;
		h.grass_min_height            = minH;
		h.grass_max_height            = maxH;
		h.grass_min_height_underwater = minHU;
		h.grass_max_height_underwater = maxHU;
	}
}

// Sync the editor's Grass Scale slider into per-entity `length`. Slider value 40 maps 1:1 to the
// per-category baseline set in BuildGrassAppearance (Course=40, Short=30, Tall=56, Wild=35,
// weed/flower/kelp/seaweed = 40 each). Slider 80 = 2× baseline, slider 20 = 0.5× baseline. Called
// after BuildGrassAppearance (so the baselines are captured) and every frame from
// GGTerrainWicked_Update so slider drags apply live without waiting for chunks to be rebuilt.
static void ApplyGrassScale()
{
	const float sliderScale = GGGrass::gggrass_global_params.grass_scale;
	constexpr float baselineSlider = 40.0f; // GGGRASS_SCALE from GGGrassConstants.hlsli
	const float mult = sliderScale / baselineSlider;
	for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
	{
		g_grassAppearance[t].length = g_grassBaseLength[t] * mult;
	}
	auto& scene = wi::scene::GetScene();
	for (size_t hi = 0; hi < scene.hairs.GetCount(); hi++)
	{
		wi::HairParticleSystem& h = scene.hairs[hi];
		if (h.grass_type == 0) continue;               // upstream Wicked hair — leave alone
		uint32_t typeIdx = h.grass_type - 1;
		if (typeIdx >= GGGRASS_TOTAL_REAL_TYPES) continue;
		h.length = g_grassAppearance[typeIdx].length;
	}
}

static void SetupWickedGrass()
{
	BuildGrassAppearance();
	ApplyGrassDrawDistance();
	ApplyGrassAltitude();
	ApplyGrassScale();
	wickedGrassSetup = true;
}

// Distance-LOD grass: dense fine grass on the chunks nearest the camera, thinning with distance and
// stopping past the outer ring. Concentrating strands near the camera (and dropping them far) keeps
// the total bounded — the trick native grass uses — so VRAM stays safe even during the load burst.
// Placement still follows GG's painted grass map; chunks crossing a tier boundary (or regenerated by
// the terrain) are rebuilt at the new density. All scene mutation is deferred to after Generation_Cancel.
//
// The outer-ring radius `outerC` is driven at runtime from the editor's Grass Draw Distance slider
// (gggrass_global_params.lod_dist, in inches) converted to chunk-distances. Near/mid boundaries
// scale proportionally so cutting the slider tight pulls all tiers in together, and extending it
// pushes them all out.
static int GrassTierForRingDist(float ringDist, float nearC, float midC, float outerC)
{
	if (ringDist <= nearC) return 3;             // near: full density
	if (ringDist <= midC)  return 2;             // mid
	if (ringDist <= outerC) return 1;            // far: sparse
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

	// LOD ring vs per-strand cull — these MUST be decoupled, or chunk pop-in happens.
	// viewDistInches is the per-strand binary visibility cull (matches DX11 grassRadius =
	// lod_dist + GGGRASS_LOD_TRANSITION). outerC is the chunk-entity-creation ring, which is
	// always 1 chunk PAST viewDistInches — so when a chunk's center first crosses outerC, its
	// near edge is still chunkStride/2 past viewDistInches, meaning none of its strands are
	// eligible to render yet. As the camera approaches, strands fade in one-by-one as each
	// crosses the per-strand cull threshold. No more whole-chunk pop-in.
	//
	// Tier 3 (full density) and tier 2 (mid) keep their canonical 1.0 / 1.7 chunk boundaries
	// clamped DOWN to outerC. SET_GRASS "lodchunks" hard-overrides via g_grassLODChunksOverride.
	const float viewDistInches = GGGrass::gggrass_global_params.lod_dist + 2500.0f;
	const float outerC = (g_grassLODChunksOverride > 0.0f)
		? g_grassLODChunksOverride
		: std::max(0.5f, viewDistInches / chunkStride + 1.0f);
	const float midC   = std::min(1.7f, outerC);
	const float nearC  = std::min(1.0f, outerC);

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
		int targetTier_raw = inEditable ? GrassTierForRingDist(ringDist, nearC, midC, outerC) : 0;

		auto entIt = grassChunkKeyToChunkEntity.find(key);
		bool entityChanged = (entIt != grassChunkKeyToChunkEntity.end() && entIt->second != entity);
		auto tierIt = grassChunkKeyToTier.find(key);
		int currentTier = (tierIt != grassChunkKeyToTier.end()) ? tierIt->second : -1;

		// Hysteresis: chunks that already have grass at a higher tier keep it until the camera moves
		// substantially past the LOD boundary. Without this, small camera nudges (the user wobbling
		// the view, or stepping across a chunk edge) drop grass in/out as ringDist crosses tier
		// boundaries. 0.5 chunk-distance buffer = ~2560 inches of camera slop before tier downgrade
		// — invisible to the user but eliminates pop-out. Only applies on DOWNGRADES; upgrades
		// always take effect immediately so painted grass shows up the moment the camera nears it.
		int targetTier = targetTier_raw;
		if (currentTier > targetTier_raw && currentTier > 0)
		{
			constexpr float HYS_MARGIN = 0.5f;
			bool keep = false;
			if (currentTier == 3 && ringDist <= nearC + HYS_MARGIN) keep = true;
			else if (currentTier == 2 && ringDist <= midC + HYS_MARGIN) keep = true;
			else if (currentTier == 1 && ringDist <= outerC + HYS_MARGIN) keep = true;
			if (keep) targetTier = currentTier;
		}

		if (!entityChanged && currentTier == targetTier) continue; // already correct for this chunk

		auto git = grassChunkKeyToGrassEntities.find(key);
		bool hasExistingEntities = false;
		if (git != grassChunkKeyToGrassEntities.end())
		{
			for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
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

		// Force a fullReset when the tier itself is changing on a chunk that still has hair entities.
		// Without this, an existing chunk that upgrades from tier 1 (5000 strands) to tier 3 (100000
		// strands) keeps its old low-density entities — the existing-entity branch in Phase 2 only
		// updates vertex_lengths, not strandCount, so the visible density stays stuck at the lower
		// tier even after the chunk re-enters the dense ring.
		bool tierChanged = (currentTier > 0 && currentTier != targetTier && hasExistingEntities);

		// Capture the world matrix by value (a transform pointer would be invalidated when we
		// create transform components for grass entities in Phase 2).
		pending.push_back({ entity, key, mesh, transform->world, targetTier, entityChanged || tierChanged });
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
			for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
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

		// Stage B.4: per-cell visibility is the simulate CS's job (Option B sample at the strand's
		// world XZ). C++ only needs to know which per-(chunk, type) hair entities should exist.
		// A single scan of the chunk's world-AABB grass-map cells gives us that — no per-vertex
		// multi-sample, no coverage scaling, no vertex_lengths restamp on paint.
		bool typesSeen[GGGRASS_TOTAL_REAL_TYPES] = {};
		const float halfChunkWorld = chunkStride * 0.5f;
		GGGrass::GGGrass_ScanRegion(
			pc.world._41 - halfChunkWorld, pc.world._43 - halfChunkWorld,
			pc.world._41 + halfChunkWorld, pc.world._43 + halfChunkWorld,
			typesSeen );

		for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
		{
			wi::ecs::Entity existing = existingEntities.perType[t];

			if (existing != wi::ecs::INVALID_ENTITY)
			{
				if (typesSeen[t]) continue; // entity already here, type still painted — done
				// Type erased from this chunk — drop its entity so the simulate CS isn't dispatching
				// strands that will all be culled to zero length by the shader visibility check.
				if (scene.hairs.GetComponent(existing))
					scene.Entity_Remove(existing);
				existingEntities.perType[t] = wi::ecs::INVALID_ENTITY;
				continue;
			}

			if (!typesSeen[t]) continue;

			// CREATE: first paint of this type in this chunk (or full reset path).
			wi::scene::MaterialComponent* mat = BuildGrassMaterial(t);
			if (!mat) continue;

			// vertex_lengths = all 1.0 forever. CreateFromMesh keeps every chunk triangle in the
			// index buffer (triangleCount = constant); the per-strand shader sample (Stage B.3) is
			// the sole visibility authority. Paint events do not touch this entity again.
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

			// Fixed strand count per tier — independent of paint state, so painting a new type
			// into the chunk never perturbs an existing entity's strandCount.
			constexpr uint32_t STAGE1_STRANDS_PER_TIER = 100000;
			uint32_t strands = (uint32_t)(STAGE1_STRANDS_PER_TIER * GrassTierDensityScale(pc.tier) + 0.5f);
			if (strands < 1024) strands = 1024;
			hair.strandCount = strands;

			hair.CreateFromMesh(*pc.mesh);
			// Force FP32 position buffer (CreateFromMesh defaults to R16G16B16A16_UNORM for static
			// base meshes; the resulting ~0.08-inch quantization step over a 5280-inch chunk AABB
			// shows up as visibly choppy micro-sway on slow-moving tips).
			hair.position_format = wi::graphics::Format::R32G32B32A32_FLOAT;
			hair.CreateRenderData();

			scene.materials.Create(grassEntity) = *mat;
			scene.transforms.Create(grassEntity);
			scene.Component_Attach(grassEntity, pc.entity, true); // inherit chunk transform

			existingEntities.perType[t] = grassEntity;
		}
	}

	// Step 1 auto-resolve (see SCRATCHPAD "Advanced Grass Settings — DX11 Baseline Port Plan"):
	// GGGrass_ScanRegion rewrites any encoded==1 cells with a real_type resolved from terrain
	// height/material. If any cell was rewritten, the R8_UNORM texGrassMap on the GPU still holds
	// the pre-resolve byte, so the hair simulate CS (Option B mask check) will keep matching only
	// Course Grass entities until we re-upload. Fold that into a single 16 MB upload per frame
	// regardless of how many chunks resolved — the paint stroke path already uses this cadence
	// (~10 Hz) without measurable overhead.
	if ( GGGrass::GGGrass_TakePendingMapUpload() )
	{
		GGGrass::GGGrass_UploadGrassMap();
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
		for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
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
		for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++) fn(g_grassAppearance[t]);
	};
	auto forAllMaterials = [&](auto fn) {
		for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
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
	else if (p == "lodchunks")  g_grassLODChunksOverride = value;     // hard override outer ring (0 = follow slider)
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

	// Phase 5: Reset the cylinder-tree pool state. Actual setup is lazy on the
	// first WickedUpdate call so pAllTrees[] has been populated by the level load.
	GGTrees::GGTrees_WickedInit();
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
			for (uint32_t t = 0; t < GGGRASS_TOTAL_REAL_TYPES; t++)
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

	// Path A: rewrite the auto material weights (slots 0-4) with DX11-shape blend, then let
	// the painted-material pass overlay its own weights on painted vertices. Must run AFTER
	// Generation_Update so blendmap layers exist, and BEFORE ProcessPaintedChunkBlendmaps so
	// painting still wins on painted cells.
	if (wickedTerrainMaterialsSetup)
	{
		ApplyDX11StyleAutoBlend(terrain);
	}

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
	// Stage B.9: custom palette slot registered/cleared by the editor UI ("Add New Grass" /
	// "Delete Grass"). Rebuild the appearance templates + clear cached materials for custom
	// real_types so BuildGrassMaterial re-loads DDS on next lookup.
	//
	// Also blast grassChunkKeyToTier so ProcessGrassChunks re-visits every chunk on the next pass.
	// This fixes two related bugs:
	//   1. Delete + re-add with a DIFFERENT DDS on the same slot — existing hair entities cache a
	//      snapshot of the OLD MaterialComponent, which persists even after we clear the material-
	//      ready flag. Forcing a chunk-tier re-eval triggers the tier-change branch (fullReset =
	//      true) so entities get destroyed and recreated with the fresh material.
	//   2. Level reload where ProcessGrassChunks runs on frame N BEFORE the palette-sync (which is
	//      in the ImGui render at end-of-frame). Frame N tries to build the custom material with a
	//      null filename (sync not yet run), fails silently, but still records the chunk's tier —
	//      frame N+1's dirty poll rebuilds appearances but the chunks stay skipped because their
	//      tier hasn't changed. Clearing tier tracking forces the re-visit.
	if (GGGrass::GGGrass_TakeCustomSlotsDirty())
	{
		g_grassAppearanceReady = false;
		for (uint32_t t = GGGRASS_CUSTOM_REAL_TYPE_BASE; t < GGGRASS_TOTAL_REAL_TYPES; t++)
			g_grassMaterialReady[t] = false;
		BuildGrassAppearance();
		ApplyGrassDrawDistance(); // sync viewDistance onto the newly-built custom appearances

		// Invalidate all chunk tier tracking + delete any existing hair entities for CUSTOM
		// real_types across every tracked chunk. Two-step because Phase 2's existing-entity branch
		// short-circuits on "type still painted, entity exists" — so if we only cleared the tier
		// map, existing entities would survive and keep their OLD material snapshot even after the
		// slot's DDS changed. Removing the custom entities here forces Phase 2 to hit the CREATE
		// branch, which calls BuildGrassMaterial with the freshly-registered filename.
		//
		// Stock entities (real_type 0..45) are left alone — their DDS never changed.
		grassChunkKeyToTier.clear();
		{
			auto& scene = wi::scene::GetScene();
			for (auto& kv : grassChunkKeyToGrassEntities)
			{
				for (uint32_t t = GGGRASS_CUSTOM_REAL_TYPE_BASE; t < GGGRASS_TOTAL_REAL_TYPES; t++)
				{
					wi::ecs::Entity e = kv.second.perType[t];
					if (e != wi::ecs::INVALID_ENTITY)
					{
						if (scene.hairs.GetComponent(e))
							scene.Entity_Remove(e);
						kv.second.perType[t] = wi::ecs::INVALID_ENTITY;
					}
				}
			}
		}
	}

	// Grass Draw Distance slider → per-entity viewDistance. Detect slider movement and sync every
	// live grass entity in one pass so dragging the slider in the editor pulls every cull radius
	// along with it (rather than waiting for chunks to be rebuilt on camera move).
	if (GGGrass::gggrass_global_params.lod_dist != g_grassPrevSliderInches)
	{
		ApplyGrassDrawDistance();
	}

	// Grass Start/End Altitude sliders (+ underwater pair + water plane) → per-entity CB values
	// consumed by the hair simulate CS altitude filter. Unconditional every frame — 4 float
	// compares per entity is well under the noise floor next to the CB upload the shader path
	// already does, and skipping the sync when values look unchanged would miss water-plane
	// drift and edge cases where the entities were re-created since the last sync.
	ApplyGrassAltitude();

	// Grass Scale slider → per-entity `length`. Same rationale as the altitude sync: cheaper to
	// always push than to track a "did the slider move?" flag and get it wrong on re-creation.
	ApplyGrassScale();
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
		// SET_GRASS knob changes still flow through the full-rebuild path. Populate/Clear
		// Vegetation buttons (GGGrass_AddAll / GGGrass_RemoveAll) also raise this signal —
		// they rewrite pGrassMap in bulk, which the per-chunk paint-invalidation pipeline
		// never sees. Note TakeFullRebuildPending is take-and-clear, so we consume it every
		// frame regardless of g_grassRebuildRequested's state.
		bool bulkMapRewrite = GGGrass::GGGrass_TakeFullRebuildPending();
		if (g_grassRebuildRequested || bulkMapRewrite)
		{
			ForceGrassRebuild();
			g_grassRebuildRequested = false;
		}
		ProcessGrassChunks(terrain, camera.Eye);
	}

	// Phase 5: Colored cylinder tree placeholders. Independent of terrain chunk
	// lifecycle — one shared cylinder mesh + a fixed pool of ObjectComponents
	// repositioned from pAllTrees[] each frame. Real LOD tree meshes come later.
	GGTrees::GGTrees_WickedUpdate();
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

	// Phase 5: tear down the tree pool alongside the terrain.
	GGTrees::GGTrees_WickedShutdown();
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
