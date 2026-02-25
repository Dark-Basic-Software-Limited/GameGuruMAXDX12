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

// Helper: set up a terrain material entity following the Wicked Editor pattern.
// Sets texture names and material properties — does NOT call CreateRenderData().
// The engine loads textures lazily during Generation_Update/render.
static void SetupTerrainMaterial(wi::scene::Scene& scene, wi::ecs::Entity entity, int ggMatIndex)
{
	using namespace wi::scene;

	// GG material index is 0-based, folder names are 1-based (mat1..mat32)
	int folderNum = (ggMatIndex & 0xFF) + 1;

	char colorPath[256], normalPath[256], surfacePath[256];
	sprintf_s(colorPath, "D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max/Files/terraintextures/mat%d/Color.dds", folderNum);
	sprintf_s(normalPath, "D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max/Files/terraintextures/mat%d/Normal.dds", folderNum);
	sprintf_s(surfacePath, "D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max/Files/terraintextures/mat%d/Surface.dds", folderNum);

	MaterialComponent* mat = scene.materials.GetComponent(entity);
	if (!mat)
	{
		mat = &scene.materials.Create(entity);
	}

	// Set texture names — engine will load them during its normal processing
	mat->textures[MaterialComponent::BASECOLORMAP].name = colorPath;
	mat->textures[MaterialComponent::NORMALMAP].name = normalPath;
	// Surface map commented out for now — GG terrain Surface.dds may use different channel convention
	//mat->textures[MaterialComponent::SURFACEMAP].name = surfacePath;

	// PBR defaults matching Wicked Editor terrain presets
	mat->SetRoughness(1.0f);
	mat->SetMetalness(0.0f);
	mat->SetReflectance(0.005f);

	// Disable streaming and force-load textures now so they're valid before Generation_Restart().
	// Without this, the engine won't load from names until a later frame, but Generation_Update()
	// checks resource.IsValid() immediately to decide virtual_texture_any.
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

	// Region parameters control height/slope-based material blending
	terrain->region1 = 1.0f;   // slope transition width
	terrain->region2 = 2.0f;   // low altitude transition
	terrain->region3 = 8.0f;   // high altitude transition

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

	// DIAGNOSTIC: Write material and chunk state to file once
	{
		static int diagFrame = 0;
		diagFrame++;
		if (diagFrame == 120)
		{
			FILE* f = nullptr;
			fopen_s(&f, "terrain_diag.txt", "w");
			if (f)
			{
				auto& scene = wi::scene::GetScene();
				for (int i = 0; i < wi::terrain::MATERIAL_COUNT; i++)
				{
					wi::scene::MaterialComponent* mat = scene.materials.GetComponent(terrain->materialEntities[i]);
					bool hasColor = mat && mat->textures[wi::scene::MaterialComponent::BASECOLORMAP].resource.IsValid();
					bool hasNormal = mat && mat->textures[wi::scene::MaterialComponent::NORMALMAP].resource.IsValid();
					fprintf(f, "matEntity[%d] color=%s normal=%s name=%s\n", i,
						hasColor ? "VALID" : "NONE", hasNormal ? "VALID" : "NONE",
						mat ? mat->textures[wi::scene::MaterialComponent::BASECOLORMAP].name.c_str() : "NULL");
				}
				int chunkCount = 0;
				for (auto& [chunk, chunk_data] : terrain->chunks)
				{
					wi::scene::MaterialComponent* cmat = scene.materials.GetComponent(chunk_data.entity);
					if (cmat)
					{
						bool chunkHasColor = cmat->textures[wi::scene::MaterialComponent::BASECOLORMAP].resource.IsValid();
						fprintf(f, "chunk[%d] hasColorTex=%s texMulAdd=(%.3f,%.3f,%.3f,%.3f)\n",
							chunkCount, chunkHasColor ? "YES" : "NO",
							cmat->texMulAdd.x, cmat->texMulAdd.y, cmat->texMulAdd.z, cmat->texMulAdd.w);
					}
					if (++chunkCount >= 3) break;
				}
				fprintf(f, "totalChunks=%zu\n", terrain->chunks.size());
				fclose(f);
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
