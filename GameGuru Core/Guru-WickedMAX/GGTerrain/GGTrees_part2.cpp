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

// Pool caps visible-tree ObjectComponents in the scene. Wicked's per-entity
// ECS overhead is O(N) per frame regardless of what each object rasterizes
// as (mesh or impostor), so we can't just crank this to numTotalTrees (400K)
// without tanking FPS. 50K is empirically the ceiling that keeps FPS usable
// on the DX11 A/B baseline.
//
// To ensure the pool is actually spent on the trees the player can see, the
// update loop does a nearest-N-to-camera pick every frame instead of the
// first-N-in-array-order (which used to leave the visible area sparse while
// filling the pool with distant trees). See GGTrees_WickedUpdate.
static constexpr uint32_t GG_TREE_POOL_SIZE  = 10000;
// Hardcoded here to keep this file free of the HLSL-flavoured numTreeTypes
// constant from GGTreesConstants.hlsli. Matches the g_GGTrees[38] array length.
static constexpr uint32_t GG_TREE_TYPES      = 38;

// Distance (world inches, 1"=1 unit) at which a tree switches from the full
// trunk+branches MeshComponent to a Wicked ImpostorComponent billboard. Wicked
// captures 36 angles around Y for each mesh into a shared atlas (impostorTextureDim
// = 128 per angle in wiScene.h); the far renderer picks the closest angle and
// draws a camera-facing quad. 2000 inches ≈ 50 m — anything past a mid-ground
// clearing becomes a billboard, matching the visual density of DX11's
// mountain-distance impostor path.
static constexpr float    GG_TREE_IMPOSTOR_SWAP_IN = 2000.0f;

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

		// Stage 4: attach an ImpostorComponent to the mesh entity. Wicked's
		// scene update spots the impostor, bakes the mesh from 36 angles into
		// its shared impostorArray texture, and clamps each rendering
		// ObjectComponent's fadeDistance to swapInDistance (see wiScene.cpp
		// line 4675-4679). Past that distance the object draws as a
		// camera-facing quad from the atlas — near zero vertex cost per far
		// tree. No per-frame CPU work from us.
		wi::scene::ImpostorComponent& impostor = scene.impostors.Create( g_treeMeshEntity[ t ] );
		impostor.swapInDistance = GG_TREE_IMPOSTOR_SWAP_IN;

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

	// Camera position — used to pick the nearest N trees to spend the pool on.
	// XZ-only distance is sufficient: trees are all ~ground-level, and vertical
	// distance would distort priority when the camera flies above the terrain.
	const wi::scene::CameraComponent& camera = wi::scene::GetCamera();
	const float camX = camera.Eye.x;
	const float camZ = camera.Eye.z;

	// Candidate list — index into pAllTrees + squared XZ distance to camera.
	// Static + clear() keeps the allocated capacity across frames.
	struct TreeCandidate { uint32_t idx; float dist2; };
	static std::vector<TreeCandidate> candidates;
	if ( candidates.capacity() < numTotalTrees ) candidates.reserve( numTotalTrees );
	candidates.clear();

	for ( uint32_t i = 0; i < numTotalTrees; i++ )
	{
		InstanceTree* pTree = &pAllTrees[ i ];
		if ( !pTree->IsVisible() || pTree->IsInvalid() || pTree->IsFlattened() ) continue;

		uint32_t type = (uint32_t)pTree->GetType();
		if ( type >= GG_TREE_TYPES ) continue;
		if ( g_treeMeshEntity[ type ] == wi::ecs::INVALID_ENTITY ) continue;

		float dx = pTree->x - camX;
		float dz = pTree->z - camZ;
		candidates.push_back( { i, dx * dx + dz * dz } );
	}

	// Partial-sort: put the N closest at the front, don't care about the rest.
	// nth_element is O(N) on average — much cheaper than a full sort.
	size_t poolFill = candidates.size();
	if ( poolFill > GG_TREE_POOL_SIZE )
	{
		std::nth_element(
			candidates.begin(),
			candidates.begin() + GG_TREE_POOL_SIZE,
			candidates.end(),
			[]( const TreeCandidate& a, const TreeCandidate& b ) { return a.dist2 < b.dist2; } );
		poolFill = GG_TREE_POOL_SIZE;
	}

	for ( size_t k = 0; k < poolFill; k++ )
	{
		InstanceTree* pTree = &pAllTrees[ candidates[ k ].idx ];
		uint32_t type = (uint32_t)pTree->GetType();
		wi::ecs::Entity treeMesh = g_treeMeshEntity[ type ];

		wi::ecs::Entity e = g_treePoolEntities[ k ];
		wi::scene::ObjectComponent*     obj   = scene.objects   .GetComponent( e );
		wi::scene::TransformComponent*  xform = scene.transforms.GetComponent( e );
		if ( !obj || !xform ) continue;

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
	}

	// Hide any pool slots we didn't fill this frame.
	for ( size_t k = poolFill; k < GG_TREE_POOL_SIZE; k++ )
	{
		wi::ecs::Entity e = g_treePoolEntities[ k ];
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
