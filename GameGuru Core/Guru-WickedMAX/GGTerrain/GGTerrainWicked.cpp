#include "GGTerrain.h"
#include "GGTerrainWicked.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiTerrain.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiResourceManager.h"
#include "../../../../WickedEngineDX12/WickedEngine/wiImage.h"

static wi::ecs::Entity wickedTerrainEntity = wi::ecs::INVALID_ENTITY;
static bool wickedTerrainInitialised = false;
static bool wickedTerrainMaterialsSetup = false;

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

// Helper: set up a terrain material entity with PBR textures from a GG material folder
static void SetupTerrainMaterial(wi::scene::Scene& scene, wi::ecs::Entity entity, int ggMatIndex)
{
	using namespace wi::scene;

	// GG material index is 0-based, folder names are 1-based (mat1..mat32)
	int folderNum = (ggMatIndex & 0xFF) + 1;

	char colorPath[256], normalPath[256], surfacePath[256];
	sprintf_s(colorPath, "Files/terraintextures/mat%d/Color.dds", folderNum);
	sprintf_s(normalPath, "Files/terraintextures/mat%d/Normal.dds", folderNum);
	sprintf_s(surfacePath, "Files/terraintextures/mat%d/Surface.dds", folderNum);

	MaterialComponent* mat = scene.materials.GetComponent(entity);
	if (!mat)
	{
		mat = &scene.materials.Create(entity);
	}
	mat->textures[MaterialComponent::BASECOLORMAP].name = colorPath;
	mat->textures[MaterialComponent::NORMALMAP].name = normalPath;
	// Skip surface map for now — GG terrain Surface.dds may use different channel convention
	//mat->textures[MaterialComponent::SURFACEMAP].name = surfacePath;
	// Set sensible PBR defaults for terrain (non-metallic, rough)
	mat->SetRoughness(1.0f);
	mat->SetMetalness(0.0f);
	mat->SetReflectance(0.04f);
	// Disable streaming so textures load synchronously (virtual_texture_any check needs IsValid() immediately)
	mat->SetTextureStreamingDisabled(true);
	mat->SetDirty(true);
	mat->CreateRenderData();

	// Verify textures loaded successfully
	bool colorLoaded = mat->textures[MaterialComponent::BASECOLORMAP].resource.IsValid();
	bool normalLoaded = mat->textures[MaterialComponent::NORMALMAP].resource.IsValid();
	wi::backlog::post(std::string("GGTerrainWicked: mat") + std::to_string(folderNum) +
		" color=" + (colorLoaded ? "OK" : "FAILED") +
		" normal=" + (normalLoaded ? "OK" : "FAILED") +
		" path=" + colorPath);
}

// Phase 2: Set up 4-layer terrain materials from GG render params
// Called lazily on first toggle (after level load, so params are correct)
static void SetupWickedTerrainMaterials()
{
	wi::terrain::Terrain* terrain = GetWickedTerrain();
	if (!terrain) return;

	auto& scene = wi::scene::GetScene();

	// Read material indices from GG render params
	// TEMP: Force base material to mat1 (ggMatIndex=0 -> folderNum=1) to verify texture loading
	int baseMat = 0; // was: GGTerrain::ggterrain_global_render_params.baseLayerMaterial & 0xFF;
	int slopeMat = GGTerrain::ggterrain_global_render_params.slopeMatIndex[0] & 0xFF;
	int lowMat = GGTerrain::ggterrain_global_render_params.layerMatIndex[0] & 0xFF;
	int highMat = GGTerrain::ggterrain_global_render_params.layerMatIndex[2] & 0xFF;

	// Create material entities and load textures
	for (int i = 0; i < wi::terrain::MATERIAL_COUNT; i++)
	{
		if (terrain->materialEntities[i] == wi::ecs::INVALID_ENTITY)
		{
			terrain->materialEntities[i] = wi::ecs::CreateEntity();
		}
		scene.Component_Attach(terrain->materialEntities[i], wickedTerrainEntity);
	}

	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_BASE], baseMat);
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_SLOPE], slopeMat);
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_LOW_ALTITUDE], lowMat);
	SetupTerrainMaterial(scene, terrain->materialEntities[wi::terrain::MATERIAL_HIGH_ALTITUDE], highMat);

	// Region parameters control height/slope-based material blending
	// region1: slope sharpness (smoothstep over slope_amount = 1-normal.y)
	// region2: low-altitude sharpness (smoothstep over InverseLerp(0, bottomLevel, height))
	// region3: high-altitude sharpness (smoothstep over InverseLerp(0, topLevel, height))
	terrain->region1 = 1.0f;   // slope transition width
	terrain->region2 = 2.0f;   // low altitude transition
	terrain->region3 = 8.0f;   // high altitude transition

	// Restart generation to pick up the new materials
	terrain->Generation_Restart();
	wickedTerrainMaterialsSetup = true;
}

namespace GGTerrain
{

void GGTerrainWicked_Init()
{
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

	// TEMP HACK: Disable virtual texture system by clearing materialEntities textures
	// before Generation_Update(). This makes virtual_texture_any=false inside Wicked,
	// so the VT atlas/compute pipeline is completely skipped.
	{
		auto& scene = wi::scene::GetScene();
		for (int i = 0; i < wi::terrain::MATERIAL_COUNT; i++)
		{
			if (terrain->materialEntities[i] != wi::ecs::INVALID_ENTITY)
			{
				wi::scene::MaterialComponent* mat = scene.materials.GetComponent(terrain->materialEntities[i]);
				if (mat)
				{
					for (int slot = 0; slot < (int)wi::scene::MaterialComponent::TEXTURESLOT_COUNT; slot++)
						mat->textures[slot].resource = {};
				}
			}
		}
	}

	terrain->Generation_Update(camera);

	// TEMP HACK: Force mat1/Color.dds onto every terrain chunk's material
	// With VT disabled above, these textures won't be overwritten by the atlas system
	{
		static wi::Resource forcedTex;
		if (!forcedTex.IsValid())
		{
			forcedTex = wi::resourcemanager::Load("Files/terraintextures/mat1/Color.dds");
		}
		if (forcedTex.IsValid())
		{
			auto& scene = wi::scene::GetScene();
			for (auto& [chunk, chunk_data] : terrain->chunks)
			{
				wi::scene::MaterialComponent* mat = scene.materials.GetComponent(chunk_data.entity);
				if (!mat) continue;

				// Set our texture directly on the chunk material
				mat->textures[wi::scene::MaterialComponent::BASECOLORMAP].resource = forcedTex;
				mat->textures[wi::scene::MaterialComponent::BASECOLORMAP].sparse_residencymap_descriptor = -1;
				mat->textures[wi::scene::MaterialComponent::BASECOLORMAP].sparse_feedbackmap_descriptor = -1;

				// Reset UV transform to identity so texture tiles normally
				mat->texMulAdd = XMFLOAT4(1, 1, 0, 0);
				mat->SetDirty(true);
			}
		}
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
}

void GGTerrainWicked_DebugDraw(wi::graphics::CommandList cmd)
{
	// TEMP: Draw mat1/Color.dds as a 256x256 quad in the top-left corner
	static wi::Resource debugTex;
	if (!debugTex.IsValid())
	{
		debugTex = wi::resourcemanager::Load("Files/terraintextures/mat1/Color.dds");
		wi::backlog::post(std::string("GGTerrainWicked_DebugDraw: mat1 texture ") +
			(debugTex.IsValid() ? "LOADED OK" : "FAILED TO LOAD"));
	}
	if (debugTex.IsValid())
	{
		wi::graphics::Texture tex = debugTex.GetTexture();
		if (tex.IsValid())
		{
			wi::image::Params fx;
			fx.pos = XMFLOAT3(10.0f, 10.0f, 0.0f);
			fx.siz = XMFLOAT2(256.0f, 256.0f);
			fx.blendFlag = wi::enums::BLENDMODE_OPAQUE;
			wi::image::Draw(&tex, fx, cmd);
		}
	}
}

} // namespace GGTerrain
