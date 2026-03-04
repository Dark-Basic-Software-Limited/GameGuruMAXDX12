#include "GGTerrain.h"
#include "GGTerrainWicked.h"
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


namespace GGTerrain
{

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
	terrain.chunk_scale = 80.0f;           // ~5280 units/chunk, reduces distant popping
	terrain.generation = 10;               // cover ~52800 units each direction
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
		wi::renderer::SetDebugNormalVis(wickedNormalVisMode != 0);
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
