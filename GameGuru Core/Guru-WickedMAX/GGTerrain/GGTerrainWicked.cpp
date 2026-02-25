#include "GGTerrain.h"
#include "GGTerrainWicked.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiTerrain.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiResourceManager.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiHelper.h"

static wi::ecs::Entity wickedTerrainEntity = wi::ecs::INVALID_ENTITY;
static bool wickedTerrainInitialised = false;
static bool wickedTerrainMaterialsSetup = false;
static std::string wickedTerrainExeDir;  // EXE directory for resolving texture paths

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

	// Restart generation to pick up the new materials
	// Generation_Restart() deep-copies the materials internally
	terrain->Generation_Restart();
	wickedTerrainMaterialsSetup = true;

	wi::backlog::post("GGTerrainWicked: materials setup complete, Generation_Restart called");
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
	terrain.chunk_scale = 8.0f;            // ~512 units/chunk, similar to old terrain
	terrain.generation = 100;              // cover ~52800 units each direction
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

	// Phase 2: Lazy material setup on first update (after level load has set render params)
	if (!wickedTerrainMaterialsSetup)
	{
		SetupWickedTerrainMaterials();
	}

	// Let the VT system run normally — it creates the atlas, blends materials
	// via compute shader, and assigns atlas textures to chunk materials
	terrain->Generation_Update(camera);
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
}

} // namespace GGTerrain
