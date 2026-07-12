// GGTrees_part2.cpp — Phase 5 Stage 2: Real trunk meshes on the new Wicked
// Engine terrain. Retires the Stage 1 cylinder placeholder in favour of the
// authored DX11 trunk geometry + trunk textures baked into TreeMeshes/*_trunk_*.h.
//
// One shared MeshComponent + MaterialComponent per tree type (38 total); a
// fixed 10000-slot ObjectComponent pool swaps meshID every frame to match
// pTree->GetType(). Branches, wind, and billboards come in later stages.
//
// Coordinate space: TreeMeshHigh vertex positions are authored in world inches
// (a pine trunk spans roughly -20..+20 x/z with authored height ~512" ≈ 13m at
// scale 1.0). Per-tree TransformComponent applies a UNIFORM scale of
// pTree->GetScaleFloat() (~0.5–1.5) plus a translate to pTree->x/y/z — pTree->y
// is already terrain-height minus the DX11 slope adjustment (see
// GGTrees_UpdateInstances), so trunks sit correctly on the terrain.
//
// Normals: TreeMeshHigh stores the normal packed as R8G8B8A8_UNORM in the uint32
// `normal` field. DX11's input layout at GGTrees_part0.cpp:1231 confirms this.
// Unpack via byte/127.5 - 1.0 to recover [-1,1] per component.
//
// Included by GGTrees.cpp unity build; lives inside `namespace GGTrees` so
// pAllTrees / numTotalTrees / InstanceTree / g_GGTrees resolve without qualification.

namespace GGTrees
{

// Pool caps visible-tree draws per frame. numTotalTrees is 400000 but most are
// invisible in any typical level.
static constexpr uint32_t GG_TREE_POOL_SIZE  = 10000;
// Hardcoded here to keep this file free of the HLSL-flavoured numTreeTypes
// constant from GGTreesConstants.hlsli. Matches the g_GGTrees[38] array length.
static constexpr uint32_t GG_TREE_TYPES      = 38;

static bool             g_wickedTreesSetup   = false;
static std::string      g_treesExeDir;

// Per-tree-type shared assets.
static wi::ecs::Entity  g_treeTrunkMeshEntity[     GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
static wi::ecs::Entity  g_treeTrunkMaterialEntity[ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };

// Fixed pool of ObjectComponents. meshID is (re)assigned every frame based on
// which tree type the slot represents.
static wi::ecs::Entity  g_treePoolEntities[ GG_TREE_POOL_SIZE ] = { wi::ecs::INVALID_ENTITY };

// Build a Wicked MeshComponent from a TreeMeshHigh (DX11 authored trunk mesh).
// Converts VertexTreeHigh (pos + packed uint32 normal + UV) into
// vertex_positions / vertex_normals / vertex_uvset_0, and uint16 indices into
// uint32. Attaches the given material as the single subset.
static wi::ecs::Entity BuildTrunkMesh( const TreeMeshHigh* tm, wi::ecs::Entity materialEntity )
{
	auto& scene = wi::scene::GetScene();

	wi::ecs::Entity meshEntity = wi::ecs::CreateEntity();
	wi::scene::MeshComponent& mesh = scene.meshes.Create( meshEntity );

	mesh.vertex_positions.reserve( tm->numVertices );
	mesh.vertex_normals  .reserve( tm->numVertices );
	mesh.vertex_uvset_0  .reserve( tm->numVertices );
	for ( uint32_t v = 0; v < tm->numVertices; v++ )
	{
		const VertexTreeHigh& src = tm->pVertices[ v ];
		mesh.vertex_positions.push_back( XMFLOAT3( src.x, src.y, src.z ) );

		// R8G8B8A8_UNORM normal → XMFLOAT3 in [-1,1]. Little-endian byte order
		// matches DX11 input layout at GGTrees_part0.cpp:1231.
		uint32_t n = src.normal;
		float nx = ( (float)( ( n >>  0 ) & 0xFF ) ) / 127.5f - 1.0f;
		float ny = ( (float)( ( n >>  8 ) & 0xFF ) ) / 127.5f - 1.0f;
		float nz = ( (float)( ( n >> 16 ) & 0xFF ) ) / 127.5f - 1.0f;
		mesh.vertex_normals.push_back( XMFLOAT3( nx, ny, nz ) );

		mesh.vertex_uvset_0.push_back( XMFLOAT2( src.u, src.v ) );
	}

	mesh.indices.reserve( tm->numIndices );
	for ( uint32_t i = 0; i < tm->numIndices; i++ )
		mesh.indices.push_back( (uint32_t)tm->pIndices[ i ] );

	wi::scene::MeshComponent::MeshSubset subset;
	subset.materialID  = materialEntity;
	subset.indexOffset = 0;
	subset.indexCount  = tm->numIndices;
	mesh.subsets.push_back( subset );
	mesh.CreateRenderData();

	return meshEntity;
}

// Build a PBR MaterialComponent that samples the trunk DDS as basecolor.
// Trees are matte bark — roughness 1.0, metalness 0, low reflectance. Same
// pattern SetupTerrainMaterial (GGTerrainWicked.cpp) uses for terrain layers.
static wi::ecs::Entity BuildTrunkMaterial( const char* textureName )
{
	auto& scene = wi::scene::GetScene();

	wi::ecs::Entity matEntity = wi::ecs::CreateEntity();
	wi::scene::MaterialComponent& mat = scene.materials.Create( matEntity );

	mat.SetBaseColor ( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	mat.SetRoughness ( 1.0f );
	mat.SetMetalness ( 0.0f );
	mat.SetReflectance( 0.005f );

	char colorPath[ 512 ];
	sprintf_s( colorPath, "%s/Files/treebank/textures/%s", g_treesExeDir.c_str(), textureName );
	mat.textures[ wi::scene::MaterialComponent::BASECOLORMAP ].name = colorPath;

	mat.SetTextureStreamingDisabled( true );
	mat.CreateRenderData();

	return matEntity;
}

static void GGTrees_WickedSetup()
{
	// Capture EXE directory once — trunk DDS lookups need CWD-independent paths.
	g_treesExeDir = wi::helper::GetDirectoryFromPath( wi::helper::GetExecutablePath() );
	if ( !g_treesExeDir.empty() && ( g_treesExeDir.back() == '/' || g_treesExeDir.back() == '\\' ) )
		g_treesExeDir.pop_back();

	auto& scene = wi::scene::GetScene();

	// Build one MeshComponent + MaterialComponent per tree type. Types with a
	// null trunk pointer (none currently in LOD0, but the field is nullable so
	// we handle it defensively) leave their slot at INVALID_ENTITY — the update
	// loop skips slots that resolve to no mesh.
	uint32_t typesBuilt = 0;
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		const GGTree& tree = g_GGTrees[ t ];
		if ( !tree.trunk ) continue;

		g_treeTrunkMaterialEntity[ t ] = BuildTrunkMaterial( tree.trunk->textureName );
		g_treeTrunkMeshEntity    [ t ] = BuildTrunkMesh( tree.trunk, g_treeTrunkMaterialEntity[ t ] );
		typesBuilt++;
	}

	// Entity pool. Each slot has an ObjectComponent (meshID assigned per frame)
	// and a TransformComponent (position/scale per frame). Starts hidden until
	// the update loop finds it a tree to represent.
	for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
	{
		wi::ecs::Entity e = wi::ecs::CreateEntity();
		wi::scene::ObjectComponent& obj = scene.objects.Create( e );
		obj.SetRenderable( false );
		scene.transforms.Create( e );
		g_treePoolEntities[ i ] = e;
	}

	g_wickedTreesSetup = true;
	wi::backlog::post( ( "GGTrees: real trunk meshes ready ("
		+ std::to_string( typesBuilt ) + " types, "
		+ std::to_string( GG_TREE_POOL_SIZE ) + " pool slots)" ).c_str() );
}

void GGTrees_WickedInit()
{
	// Lazy setup on first update — same phase ordering as SetupWickedGrass /
	// SetupWickedTerrainMaterials. State reset here only.
	g_wickedTreesSetup = false;
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		g_treeTrunkMeshEntity    [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeTrunkMaterialEntity[ t ] = wi::ecs::INVALID_ENTITY;
	}
	for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
		g_treePoolEntities[ i ] = wi::ecs::INVALID_ENTITY;
}

void GGTrees_WickedUpdate()
{
	if ( !g_wickedTreesSetup )
	{
		GGTrees_WickedSetup();
	}

	auto& scene = wi::scene::GetScene();

	uint32_t poolIndex = 0;
	for ( uint32_t i = 0; i < numTotalTrees && poolIndex < GG_TREE_POOL_SIZE; i++ )
	{
		InstanceTree* pTree = &pAllTrees[ i ];
		if ( !pTree->IsVisible() || pTree->IsInvalid() || pTree->IsFlattened() ) continue;

		uint32_t type = (uint32_t)pTree->GetType();
		if ( type >= GG_TREE_TYPES ) continue;
		wi::ecs::Entity trunkMesh = g_treeTrunkMeshEntity[ type ];
		if ( trunkMesh == wi::ecs::INVALID_ENTITY ) continue;

		wi::ecs::Entity e = g_treePoolEntities[ poolIndex ];
		wi::scene::ObjectComponent*     obj   = scene.objects   .GetComponent( e );
		wi::scene::TransformComponent*  xform = scene.transforms.GetComponent( e );
		if ( !obj || !xform ) { poolIndex++; continue; }

		// Swap in this tree's mesh for the pool slot. Wicked's per-object AABB
		// derives from mesh AABB × transform, so culling picks this up
		// automatically on the next scene update.
		obj->meshID = trunkMesh;
		obj->SetRenderable( true );

		float scale = pTree->GetScaleFloat();  // ~0.5–1.5
		xform->ClearTransform();
		xform->Scale    ( XMFLOAT3( scale, scale, scale ) );
		xform->Translate( XMFLOAT3( pTree->x, pTree->y, pTree->z ) );
		xform->UpdateTransform();

		poolIndex++;
	}

	// Hide any pool slots we didn't fill this frame.
	for ( uint32_t i = poolIndex; i < GG_TREE_POOL_SIZE; i++ )
	{
		wi::ecs::Entity e = g_treePoolEntities[ i ];
		if ( e == wi::ecs::INVALID_ENTITY ) break;
		wi::scene::ObjectComponent* obj = scene.objects.GetComponent( e );
		if ( obj ) obj->SetRenderable( false );
	}
}

void GGTrees_WickedShutdown()
{
	if ( !g_wickedTreesSetup ) return;
	auto& scene = wi::scene::GetScene();

	for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
	{
		wi::ecs::Entity e = g_treePoolEntities[ i ];
		if ( e != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( e );
		g_treePoolEntities[ i ] = wi::ecs::INVALID_ENTITY;
	}
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		if ( g_treeTrunkMeshEntity    [ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeTrunkMeshEntity    [ t ] );
		if ( g_treeTrunkMaterialEntity[ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeTrunkMaterialEntity[ t ] );
		g_treeTrunkMeshEntity    [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeTrunkMaterialEntity[ t ] = wi::ecs::INVALID_ENTITY;
	}
	g_wickedTreesSetup = false;
}

} // namespace GGTrees
