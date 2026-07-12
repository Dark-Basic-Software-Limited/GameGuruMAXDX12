// GGTrees_part2.cpp — Phase 5 Stage 3: Trunks + alpha-tested leaves on the
// new Wicked Engine terrain. Each per-type MeshComponent now carries TWO
// subsets: subset[0] = trunk (opaque PBR bark texture),
// subset[1] = branches (alpha-tested leaf texture, double-sided). One shared
// ObjectComponent per pool slot draws both.
//
// Grown from `pAllTrees[]` (positions/type/scale set by the DX11-era GGTrees
// data pipeline). Real LOD variants, wind sway, and distance billboards come
// in later stages.
//
// Vertex format (both trunk and branch meshes):
// `VertexTreeHigh` = pos (3xfloat) + packed uint32 normal (R8G8B8A8_UNORM)
// + UV (2xfloat). DX11 input layout at GGTrees_part0.cpp:1231 confirms it.
//
// Included by GGTrees.cpp unity build; lives inside `namespace GGTrees` so
// pAllTrees / numTotalTrees / InstanceTree / g_GGTrees resolve without
// qualification.

namespace GGTrees
{

// Pool caps visible-tree draws per frame. numTotalTrees is 400000 but most are
// invisible in any typical level; on TESTPRO1 island (2026-07-12 A/B against
// DX11) the visible-tree count is somewhere well past 10000, so bumped from
// the original Stage-1 cap. Wicked auto-instances ObjectComponents that share
// a meshID (we only have 38 unique tree meshes) so the extra cost per slot
// is per-object CPU work + one TransformComponent, not GPU draw calls.
static constexpr uint32_t GG_TREE_POOL_SIZE  = 100000;
// Hardcoded here to keep this file free of the HLSL-flavoured numTreeTypes
// constant from GGTreesConstants.hlsli. Matches the g_GGTrees[38] array length.
static constexpr uint32_t GG_TREE_TYPES      = 38;

static bool             g_wickedTreesSetup   = false;
static std::string      g_treesExeDir;

// Per-tree-type shared assets. Trunk always present (typewise), branches
// nullable — a few tree types (dead pine tree in LOD0) have branches == null;
// those meshes get only subset[0].
static wi::ecs::Entity  g_treeMeshEntity            [ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
static wi::ecs::Entity  g_treeTrunkMaterialEntity   [ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
static wi::ecs::Entity  g_treeBranchesMaterialEntity[ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };

// Fixed pool of ObjectComponents. meshID is (re)assigned every frame based on
// which tree type the slot represents.
static wi::ecs::Entity  g_treePoolEntities[ GG_TREE_POOL_SIZE ] = { wi::ecs::INVALID_ENTITY };

// Append a TreeMeshHigh's verts to the given mesh, unpacking the packed
// R8G8B8A8_UNORM normal to XMFLOAT3 in [-1,1]. Returns the vertex-index offset
// to add to this part's indices when they get merged.
static uint32_t AppendTreeVerts( wi::scene::MeshComponent& mesh, const TreeMeshHigh* tm )
{
	uint32_t baseVertex = (uint32_t)mesh.vertex_positions.size();

	mesh.vertex_positions.reserve( baseVertex + tm->numVertices );
	mesh.vertex_normals  .reserve( baseVertex + tm->numVertices );
	mesh.vertex_uvset_0  .reserve( baseVertex + tm->numVertices );

	for ( uint32_t v = 0; v < tm->numVertices; v++ )
	{
		const VertexTreeHigh& src = tm->pVertices[ v ];
		mesh.vertex_positions.push_back( XMFLOAT3( src.x, src.y, src.z ) );

		// Little-endian byte order matches DX11 input layout at
		// GGTrees_part0.cpp:1231 (R8G8B8A8_UNORM).
		uint32_t n = src.normal;
		float nx = ( (float)( ( n >>  0 ) & 0xFF ) ) / 127.5f - 1.0f;
		float ny = ( (float)( ( n >>  8 ) & 0xFF ) ) / 127.5f - 1.0f;
		float nz = ( (float)( ( n >> 16 ) & 0xFF ) ) / 127.5f - 1.0f;
		mesh.vertex_normals.push_back( XMFLOAT3( nx, ny, nz ) );

		mesh.vertex_uvset_0.push_back( XMFLOAT2( src.u, src.v ) );
	}

	return baseVertex;
}

// Append a TreeMeshHigh's indices to the given mesh, shifting each by
// `vertexBaseOffset` so branch indices point at the branch vert range that was
// appended after the trunk verts. Returns the number of indices appended.
static uint32_t AppendTreeIndices( wi::scene::MeshComponent& mesh,
	const TreeMeshHigh* tm, uint32_t vertexBaseOffset )
{
	uint32_t startIndex = (uint32_t)mesh.indices.size();
	mesh.indices.reserve( startIndex + tm->numIndices );
	for ( uint32_t i = 0; i < tm->numIndices; i++ )
		mesh.indices.push_back( vertexBaseOffset + (uint32_t)tm->pIndices[ i ] );
	return tm->numIndices;
}

// Build the merged trunk+branches MeshComponent for one tree type. Trunk goes
// in as subset[0] with the trunk (opaque bark) material. Branches, if non-null,
// go in as subset[1] with the leaf (alpha-tested, double-sided) material —
// their indices are re-offset to point at their own vert range.
static wi::ecs::Entity BuildTreeMesh(
	const TreeMeshHigh* trunk, wi::ecs::Entity trunkMaterial,
	const TreeMeshHigh* branches, wi::ecs::Entity branchesMaterial )
{
	auto& scene = wi::scene::GetScene();

	wi::ecs::Entity meshEntity = wi::ecs::CreateEntity();
	wi::scene::MeshComponent& mesh = scene.meshes.Create( meshEntity );

	// Trunk verts first (base offset 0), then trunk indices, then subset[0].
	AppendTreeVerts( mesh, trunk );
	uint32_t trunkIndexCount = AppendTreeIndices( mesh, trunk, 0 );

	wi::scene::MeshComponent::MeshSubset trunkSubset;
	trunkSubset.materialID  = trunkMaterial;
	trunkSubset.indexOffset = 0;
	trunkSubset.indexCount  = trunkIndexCount;
	mesh.subsets.push_back( trunkSubset );

	// Branches (optional) — verts appended after trunk's vert range so we can
	// reuse the same vertex/index buffers. Branch indices get shifted by
	// trunkNumVerts on the append. Subset[1] slices the tail of `mesh.indices`.
	if ( branches != nullptr )
	{
		uint32_t branchBaseVertex   = (uint32_t)mesh.vertex_positions.size();
		AppendTreeVerts  ( mesh, branches );
		uint32_t branchIndexOffset  = (uint32_t)mesh.indices.size();
		uint32_t branchIndexCount   = AppendTreeIndices( mesh, branches, branchBaseVertex );

		wi::scene::MeshComponent::MeshSubset branchSubset;
		branchSubset.materialID  = branchesMaterial;
		branchSubset.indexOffset = branchIndexOffset;
		branchSubset.indexCount  = branchIndexCount;
		mesh.subsets.push_back( branchSubset );
	}

	mesh.CreateRenderData();
	return meshEntity;
}

// Build a PBR MaterialComponent that samples one tree DDS as basecolor.
// `isBranches` = true switches to alpha-tested + double-sided leaves. Trunks
// stay opaque + single-sided (BACK-face culled).
static wi::ecs::Entity BuildTreeMaterial( const char* textureName, bool isBranches )
{
	auto& scene = wi::scene::GetScene();

	wi::ecs::Entity matEntity = wi::ecs::CreateEntity();
	wi::scene::MaterialComponent& mat = scene.materials.Create( matEntity );

	mat.SetBaseColor ( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	mat.SetRoughness ( 1.0f );
	mat.SetMetalness ( 0.0f );
	mat.SetReflectance( 0.02f );

	char colorPath[ 512 ];
	sprintf_s( colorPath, "%s/Files/treebank/textures/%s", g_treesExeDir.c_str(), textureName );
	mat.textures[ wi::scene::MaterialComponent::BASECOLORMAP ].name = colorPath;

	if ( isBranches )
	{
		// Alpha-tested leaves — cut out background between the leaf silhouettes
		// via the basecolor DDS's alpha channel. 0.5 is the standard foliage
		// cutoff (see Wicked's alphaRef semantics — anything < 1.0-1/256 counts
		// as alpha-test enabled). Double-sided so leaves show from both sides.
		mat.SetAlphaRef   ( 0.5f );
		mat.SetDoubleSided( true );
	}

	mat.SetTextureStreamingDisabled( true );
	mat.CreateRenderData();

	return matEntity;
}

static void GGTrees_WickedSetup()
{
	// Capture EXE directory once — DDS lookups need CWD-independent paths.
	g_treesExeDir = wi::helper::GetDirectoryFromPath( wi::helper::GetExecutablePath() );
	if ( !g_treesExeDir.empty() && ( g_treesExeDir.back() == '/' || g_treesExeDir.back() == '\\' ) )
		g_treesExeDir.pop_back();

	auto& scene = wi::scene::GetScene();

	uint32_t typesBuilt = 0, branchesBuilt = 0;
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		const GGTree& tree = g_GGTrees[ t ];
		if ( !tree.trunk ) continue;

		g_treeTrunkMaterialEntity[ t ] = BuildTreeMaterial( tree.trunk->textureName, false );

		wi::ecs::Entity branchesMat = wi::ecs::INVALID_ENTITY;
		if ( tree.branches )
		{
			branchesMat = BuildTreeMaterial( tree.branches->textureName, true );
			g_treeBranchesMaterialEntity[ t ] = branchesMat;
			branchesBuilt++;
		}

		g_treeMeshEntity[ t ] = BuildTreeMesh(
			tree.trunk,    g_treeTrunkMaterialEntity[ t ],
			tree.branches, branchesMat );
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
	wi::backlog::post( ( "GGTrees: real tree meshes ready ("
		+ std::to_string( typesBuilt )    + " trunks, "
		+ std::to_string( branchesBuilt ) + " with branches, "
		+ std::to_string( GG_TREE_POOL_SIZE ) + " pool slots)" ).c_str() );
}

void GGTrees_WickedInit()
{
	// Lazy setup on first update — same phase ordering as SetupWickedGrass /
	// SetupWickedTerrainMaterials. State reset here only.
	g_wickedTreesSetup = false;
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		g_treeMeshEntity            [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeTrunkMaterialEntity   [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeBranchesMaterialEntity[ t ] = wi::ecs::INVALID_ENTITY;
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
		wi::ecs::Entity treeMesh = g_treeMeshEntity[ type ];
		if ( treeMesh == wi::ecs::INVALID_ENTITY ) continue;

		wi::ecs::Entity e = g_treePoolEntities[ poolIndex ];
		wi::scene::ObjectComponent*     obj   = scene.objects   .GetComponent( e );
		wi::scene::TransformComponent*  xform = scene.transforms.GetComponent( e );
		if ( !obj || !xform ) { poolIndex++; continue; }

		// Swap in this tree's merged trunk+branches mesh for the pool slot.
		// Wicked's per-object AABB derives from mesh AABB * transform, so
		// culling picks this up automatically on the next scene update.
		obj->meshID = treeMesh;
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
		if ( g_treeMeshEntity            [ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeMeshEntity            [ t ] );
		if ( g_treeTrunkMaterialEntity   [ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeTrunkMaterialEntity   [ t ] );
		if ( g_treeBranchesMaterialEntity[ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeBranchesMaterialEntity[ t ] );
		g_treeMeshEntity            [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeTrunkMaterialEntity   [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeBranchesMaterialEntity[ t ] = wi::ecs::INVALID_ENTITY;
	}
	g_wickedTreesSetup = false;
}

} // namespace GGTrees
