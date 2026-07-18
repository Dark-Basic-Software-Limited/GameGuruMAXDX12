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

// LOD1/LOD2 mesh tables (g_GGTreesLOD1/g_GGTreesLOD2, 38 entries each).
// Included inside namespace GGTrees to match the LOD0 include in part0.
#include "TreeMeshes/TreeMeshesLOD1.h"
#include "TreeMeshes/TreeMeshesLOD2.h"

// Pool caps visible-tree ObjectComponents in the scene. Wicked's per-entity
// ECS overhead is O(N) per frame regardless of what each object rasterizes
// as, so we can't just crank this to numTotalTrees (400K) without tanking
// FPS (400K measured 2.6 FPS; 50K workable; 10K comfortable on the DX11
// A/B baseline — settled 2026-07-13).
//
// To ensure the pool is actually spent on the trees the player can see, the
// update loop does a nearest-N-to-camera pick every frame instead of the
// first-N-in-array-order (which used to leave the visible area sparse while
// filling the pool with distant trees). See GGTrees_WickedUpdate.
static constexpr uint32_t GG_TREE_POOL_SIZE  = 20000;
// Hardcoded here to keep this file free of the HLSL-flavoured numTreeTypes
// constant from GGTreesConstants.hlsli. Matches the g_GGTrees[38] array length.
static constexpr uint32_t GG_TREE_TYPES      = 38;

// Mesh LOD distance bands (world inches, 1" = 1 unit). Beyond LOD1_DIST a tree
// uses the LOD1 mesh, beyond LOD2_DIST the LOD2 mesh (e.g. birch 5156 -> 3448
// -> 2061 tris). The LOD1/LOD2 tables have existed as assets since the DX11
// era but were never referenced by ANY engine path (DX11 renders LOD0 +
// billboards past lod_dist) — this is their first use, so watch the A/B for
// authoring defects. Compared as squared distances against the nearest-N dist2.
static constexpr float GG_TREE_LOD1_DIST = 2500.0f;  // ~63 m
static constexpr float GG_TREE_LOD2_DIST = 7000.0f;  // ~178 m

static bool             g_wickedTreesSetup   = false;
static std::string      g_treesExeDir;

// Per-tree-type shared assets. Trunk always present (typewise), branches
// nullable — a few tree types (dead pine tree in LOD0) have branches == null;
// those meshes get only subset[0].
static wi::ecs::Entity  g_treeMeshEntity            [ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
// LOD1/LOD2 mesh variants, sharing the type's materials. Where a variant is
// missing in the tables, the slot ALIASES the LOD0 entity (never INVALID for a
// valid type) so the pick loop needs no per-frame guards. Shutdown must skip
// aliased entries to avoid double Entity_Remove.
static wi::ecs::Entity  g_treeMeshEntityLOD1        [ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
static wi::ecs::Entity  g_treeMeshEntityLOD2        [ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
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

	// Branches and trunks both use an untinted white baseColor so the leaf DDS
	// colours pass through exactly as authored — matching DX11, which samples
	// the same textures with no material tint. (A dark-green branch tint lived
	// here until 2026-07-17; it was a leftover from the retired impostor-capture
	// path and turned red autumn leaf textures muddy brown: red x green = dark
	// brown. Removed after the A/B exposed it — do not reintroduce.)
	mat.SetBaseColor( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
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
		// Wicked semantics: shader clips at (1 - alphaRef), so HIGHER alphaRef
		// keeps MORE texels. 0.85 = clip below alpha 0.15 — full canopies that
		// survive mip-averaged alpha at distance, background still cut.
		mat.SetAlphaRef   ( 0.85f );
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

		// LOD1/LOD2 variants share the type's materials (same texture set, just
		// decimated geometry). A missing variant aliases the previous LOD so the
		// per-frame pick never needs a validity check. If a lower LOD has
		// branches where LOD0 had none, build the branch material on demand.
		auto buildLodVariant = [&]( const GGTree& lodTree, wi::ecs::Entity fallback ) -> wi::ecs::Entity
		{
			if ( !lodTree.trunk ) return fallback;
			wi::ecs::Entity lodBranchesMat = branchesMat;
			if ( lodTree.branches && lodBranchesMat == wi::ecs::INVALID_ENTITY )
			{
				lodBranchesMat = BuildTreeMaterial( lodTree.branches->textureName, true );
				g_treeBranchesMaterialEntity[ t ] = lodBranchesMat;
			}
			return BuildTreeMesh( lodTree.trunk, g_treeTrunkMaterialEntity[ t ],
				lodTree.branches, lodBranchesMat );
		};
		g_treeMeshEntityLOD1[ t ] = buildLodVariant( g_GGTreesLOD1[ t ], g_treeMeshEntity[ t ] );
		g_treeMeshEntityLOD2[ t ] = buildLodVariant( g_GGTreesLOD2[ t ], g_treeMeshEntityLOD1[ t ] );

		// Stage 4 (deprecated 2026-07-13 evening): no ImpostorComponent.
		// Wicked's impostor render path (wiRenderer.cpp:7298 RenderImpostors)
		// draws via a separate DrawIndexedInstancedIndirect that does NOT
		// respect ObjectComponent::IsNotVisibleInReflections. Result: water
		// reflection pass drew impostor atlas quads as bright white splats
		// (edge-alpha pixels sampled from the un-tinted capture). Since our
		// nearest-N-to-camera pick fills the 10K pool with just the near
		// trees anyway — the trees past swapInDistance would have been
		// pixelated dot-scale billboards contributing minimal visual value
		// for the ECS overhead — we drop the impostor path entirely. Far
		// trees now just don't render; the pool covers what's visible.
		typesBuilt++;
	}

	// Entity pool. Each slot has an ObjectComponent (meshID assigned per frame)
	// and a TransformComponent (position/scale per frame). Starts hidden until
	// the update loop finds it a tree to represent.
	//
	// SetNotVisibleInReflections(true): trees don't render into planar/water
	// reflection passes. Prevents Stage 4 impostor billboards from showing up
	// as bright white squares in the water (Wicked would otherwise render the
	// impostor atlas quad in the reflection pass same as the main pass).
	// One-shot flag, persists across every SetRenderable toggle from the
	// update loop.
	for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
	{
		wi::ecs::Entity e = wi::ecs::CreateEntity();
		wi::scene::ObjectComponent& obj = scene.objects.Create( e );
		obj.SetRenderable( false );
		obj.SetNotVisibleInReflections( true );
		// Perf (Wicked delta #6): tree pool objects opt out of GPU occlusion
		// queries. 20K query slots + proxy draws cost ~2.5ms CPU + ~1.3ms GPU
		// per frame on TESTPRO1 and foliage occludes almost nothing. Regular
		// entities keep occlusion per the level's visuals setting.
		obj.SetOcclusionQueryDisabled( true );
		// Trees skip the FARTHEST cascade (30000-500000): its frustum contains
		// every pool tree, and at ~150 world units per shadow texel a whole
		// tree is 1-2 texels — 20K mesh draws for nothing. DX11 used billboard
		// quads there. Cascades 0-3 (out to 30000, the whole visible island)
		// still receive tree shadows; terrain casts in all five.
		// Real tree meshes shadow only cascades 0-2 (out to 7500 world units).
		// Rendering them into cascade 3 (7500-30000) cost ~11ms CPU + 7ms GPU;
		// the merged billboard shadow proxies cover the far cascades instead
		// (DX11's own mesh-near/billboard-far recipe).
		obj.cascadeMask = 2;
		scene.transforms.Create( e );
		g_treePoolEntities[ i ] = e;
	}

	if ( typesBuilt == 0 )
	{
		// Should be impossible (g_GGTrees is compiled-in static data), but never
		// latch setup "done" with zero meshes — destroy the pool and let the
		// next frame retry rather than rendering nothing forever.
		for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
		{
			if ( g_treePoolEntities[ i ] != wi::ecs::INVALID_ENTITY )
			{
				scene.Entity_Remove( g_treePoolEntities[ i ] );
				g_treePoolEntities[ i ] = wi::ecs::INVALID_ENTITY;
			}
		}
		return;
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

// ---------------------------------------------------------------------------
// Stable slot binding (perf fix 2026-07-18).
//
// The pool used to rebind meshID + rebuild the transform of EVERY slot EVERY
// frame in nth_element order: 20K UpdateTransform calls = ~20ms CPU (measured
// "TerrainW - Tree Pool" 19.7ms, 48% of the whole frame at the A/B camera).
// nth_element order is also arbitrary frame to frame, so a pool entity's
// previous-frame matrix belonged to a DIFFERENT tree — wrong motion vectors,
// TAA/motion-blur ghosting on every tree.
//
// Now a slot stays bound to the same tree until the nearest-N set evicts it.
// The 400K candidate scan + nth_element still run each frame (cheap), but the
// bind loop only touches slots whose tree binding, LOD band, or source data
// actually changed — near-zero component writes with a still camera, and each
// pool entity keeps a stable identity for motion vectors.
// ---------------------------------------------------------------------------
static std::vector<uint32_t> g_slotToTree;       // pool slot -> pAllTrees index, UINT32_MAX = free
static std::vector<int32_t>  g_treeToSlot;       // pAllTrees index -> pool slot, -1 = unbound
static std::vector<uint32_t> g_treeDesiredEpoch; // per tree: frame epoch when it last made the nearest-N cut
static std::vector<uint8_t>  g_treeBand;         // per tree: LOD band, valid when epoch matches current
struct TreeSlotCache { float x, y, z, scale; uint8_t type, band; };
static std::vector<TreeSlotCache> g_slotCache;   // per slot: source-data snapshot at bind time
static uint32_t g_treeEpoch = 0;

static wi::ecs::Entity TreeMeshForBand( uint32_t type, uint8_t band )
{
	return ( band == 0 ) ? g_treeMeshEntity    [ type ] :
	       ( band == 1 ) ? g_treeMeshEntityLOD1[ type ] :
	                       g_treeMeshEntityLOD2[ type ];
}

// Bind pool slot -> tree: swap in the merged trunk+branches mesh for the LOD
// band and rebuild the slot transform. Wicked's per-object AABB derives from
// mesh AABB * transform, so culling picks the change up on the next scene
// update. Returns false (mappings untouched) if the tree can't be represented.
static bool BindTreeSlot( wi::scene::Scene& scene, uint32_t slot, uint32_t treeIdx, uint8_t band )
{
	InstanceTree* pTree = &pAllTrees[ treeIdx ];
	uint32_t type = (uint32_t)pTree->GetType();
	if ( type >= GG_TREE_TYPES ) return false;
	wi::ecs::Entity treeMesh = TreeMeshForBand( type, band );
	if ( treeMesh == wi::ecs::INVALID_ENTITY ) return false;

	wi::ecs::Entity e = g_treePoolEntities[ slot ];
	wi::scene::ObjectComponent*    obj   = scene.objects   .GetComponent( e );
	wi::scene::TransformComponent* xform = scene.transforms.GetComponent( e );
	if ( !obj || !xform ) return false;

	obj->meshID = treeMesh;
	obj->SetRenderable( true );
	// Exact DX11 recipe: detailed MESH shadows only within lod_dist_shadow
	// (2500 = our band 0); every tree beyond that gets its shadow from the
	// merged billboard proxies instead. Keeps the near-shadow crispness and
	// collapses the cascade-2 caster count to the handful of near trees.
	obj->SetCastShadow( ggtrees_global_params.draw_shadows != 0 && band == 0 );

	float scale = pTree->GetScaleFloat();  // ~0.5–1.5
	xform->ClearTransform();
	xform->Scale    ( XMFLOAT3( scale, scale, scale ) );
	xform->Translate( XMFLOAT3( pTree->x, pTree->y, pTree->z ) );
	xform->UpdateTransform();

	g_slotToTree[ slot ]    = treeIdx;
	g_treeToSlot[ treeIdx ] = (int32_t)slot;
	g_slotCache [ slot ]    = { pTree->x, pTree->y, pTree->z, scale, (uint8_t)type, band };
	return true;
}

// ---------------------------------------------------------------------------
// Far tree shadow proxies (DX11 shadow-distance parity, 2026-07-18).
//
// Production DX11 gets island-wide tree shadows cheaply: real tree meshes
// shadow only within lod_dist_shadow, and every tree beyond that renders a
// BILLBOARD QUAD into the far shadow cascades. Rendering our 20K real meshes
// into the far cascades instead cost ~11ms CPU + 7ms GPU (cascade 3 alone).
//
// So: per tree chunk (16x16 grid), ONE merged static mesh holding two crossed
// vertical quads per valid tree, textured with the type's authored billboard
// silhouette DDS (Files/treebank/billboards/*_BB_SF_*_color.dds — the exact
// same asset DX11 uses), alpha-tested. The objects are shadow-only: invisible
// to the main camera and reflections, no occlusion queries. Per frame they
// cost one object each (max 256); the real tree meshes keep cascadeMask=2 so
// they only shadow the near cascades where their detail shows.
//
// Rebuilt as one deferred batch ~0.5s after the last tree edit (same
// g_treeInstanceStamp signal the pool uses).
// ---------------------------------------------------------------------------
static wi::ecs::Entity g_shadowProxyMaterial[ GG_TREE_TYPES ] = { wi::ecs::INVALID_ENTITY };
static std::vector<wi::ecs::Entity> g_shadowProxyMeshes;
static std::vector<wi::ecs::Entity> g_shadowProxyObjects;

static wi::ecs::Entity BuildBillboardShadowMaterial( uint32_t type )
{
	auto& scene = wi::scene::GetScene();

	wi::ecs::Entity matEntity = wi::ecs::CreateEntity();
	wi::scene::MaterialComponent& mat = scene.materials.Create( matEntity );

	mat.SetBaseColor( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	mat.SetRoughness ( 1.0f );
	mat.SetMetalness ( 0.0f );
	mat.SetReflectance( 0.02f );

	char colorPath[ MAX_PATH ];
	sprintf_s( colorPath, "%s/Files/treebank/billboards/%s",
		g_treesExeDir.c_str(), g_GGTrees[ type ].billboardFilename );
	mat.textures[ wi::scene::MaterialComponent::BASECOLORMAP ].name = colorPath;

	// Wicked semantics: shader clips at (1 - alphaRef) — 0.85 keeps texels with
	// alpha >= 0.15, same cutoff as the leaf materials.
	mat.SetAlphaRef   ( 0.85f );
	mat.SetDoubleSided( true );
	mat.SetTextureStreamingDisabled( true );
	mat.CreateRenderData();
	return matEntity;
}

static void GGTrees_SetShadowProxiesVisible( wi::scene::Scene& scene, bool visible )
{
	for ( wi::ecs::Entity e : g_shadowProxyObjects )
	{
		wi::scene::ObjectComponent* obj = scene.objects.GetComponent( e );
		if ( obj ) obj->SetRenderable( visible );
	}
}

static void GGTrees_BuildShadowProxies()
{
	auto& scene = wi::scene::GetScene();

	// Tear down the previous build (materials are kept and reused).
	for ( wi::ecs::Entity e : g_shadowProxyObjects ) if ( e != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( e );
	for ( wi::ecs::Entity e : g_shadowProxyMeshes  ) if ( e != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( e );
	g_shadowProxyObjects.clear();
	g_shadowProxyMeshes.clear();

	if ( !ggtrees_global_params.draw_shadows ) return;

	// Per-type instance buckets, reused across chunks to keep allocations warm.
	static std::vector<InstanceTree*> byType[ GG_TREE_TYPES ];

	for ( uint32_t c = 0; c < numTreeChunks; c++ )
	{
		TreeChunk* pChunk = &pTreeChunks[ c ];
		const uint32_t numInst = pChunk->pInstances.NumItems();
		if ( numInst == 0 ) continue;

		for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ ) byType[ t ].clear();
		uint32_t validCount = 0;
		for ( uint32_t j = 0; j < numInst; j++ )
		{
			InstanceTree* p = pChunk->pInstances[ j ];
			if ( !p->IsVisible() || p->IsInvalid() || p->IsFlattened() ) continue;
			uint32_t type = (uint32_t)p->GetType();
			if ( type >= GG_TREE_TYPES ) continue;
			if ( g_treeMeshEntity[ type ] == wi::ecs::INVALID_ENTITY ) continue;
			byType[ type ].push_back( p );
			validCount++;
		}
		if ( validCount == 0 ) continue;

		wi::ecs::Entity meshEntity = wi::ecs::CreateEntity();
		wi::scene::MeshComponent& mesh = scene.meshes.Create( meshEntity );
		mesh.vertex_positions.reserve( validCount * 8 );
		mesh.vertex_normals  .reserve( validCount * 8 );
		mesh.vertex_uvset_0  .reserve( validCount * 8 );
		mesh.indices         .reserve( validCount * 12 );

		// One quad = 4 verts / 6 indices; uv v=0 at the top of the billboard.
		auto emitQuad = [&]( const XMFLOAT3& bl, const XMFLOAT3& br,
			const XMFLOAT3& tr, const XMFLOAT3& tl, const XMFLOAT3& n )
		{
			uint32_t base = (uint32_t)mesh.vertex_positions.size();
			mesh.vertex_positions.push_back( bl ); mesh.vertex_uvset_0.push_back( XMFLOAT2( 0, 1 ) );
			mesh.vertex_positions.push_back( br ); mesh.vertex_uvset_0.push_back( XMFLOAT2( 1, 1 ) );
			mesh.vertex_positions.push_back( tr ); mesh.vertex_uvset_0.push_back( XMFLOAT2( 1, 0 ) );
			mesh.vertex_positions.push_back( tl ); mesh.vertex_uvset_0.push_back( XMFLOAT2( 0, 0 ) );
			for ( int v = 0; v < 4; v++ ) mesh.vertex_normals.push_back( n );
			mesh.indices.push_back( base + 0 ); mesh.indices.push_back( base + 1 ); mesh.indices.push_back( base + 2 );
			mesh.indices.push_back( base + 0 ); mesh.indices.push_back( base + 2 ); mesh.indices.push_back( base + 3 );
		};

		for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
		{
			if ( byType[ t ].empty() ) continue;
			if ( g_shadowProxyMaterial[ t ] == wi::ecs::INVALID_ENTITY )
				g_shadowProxyMaterial[ t ] = BuildBillboardShadowMaterial( t );

			uint32_t indexOffset = (uint32_t)mesh.indices.size();
			for ( InstanceTree* p : byType[ t ] )
			{
				const float h = g_GGTrees[ t ].height * p->GetScaleFloat();
				const float halfW = 0.5f * g_GGTrees[ t ].billboardScaleX * h;
				const float x = p->x, y = p->y, z = p->z;
				emitQuad( XMFLOAT3( x - halfW, y, z ), XMFLOAT3( x + halfW, y, z ),
				          XMFLOAT3( x + halfW, y + h, z ), XMFLOAT3( x - halfW, y + h, z ),
				          XMFLOAT3( 0, 0, 1 ) );
				emitQuad( XMFLOAT3( x, y, z - halfW ), XMFLOAT3( x, y, z + halfW ),
				          XMFLOAT3( x, y + h, z + halfW ), XMFLOAT3( x, y + h, z - halfW ),
				          XMFLOAT3( 1, 0, 0 ) );
			}

			wi::scene::MeshComponent::MeshSubset subset;
			subset.materialID  = g_shadowProxyMaterial[ t ];
			subset.indexOffset = indexOffset;
			subset.indexCount  = (uint32_t)mesh.indices.size() - indexOffset;
			mesh.subsets.push_back( subset );
		}

		mesh.CreateRenderData();

		wi::ecs::Entity objEntity = wi::ecs::CreateEntity();
		wi::scene::ObjectComponent& obj = scene.objects.Create( objEntity );
		obj.meshID = meshEntity;
		obj.SetRenderable( true );
		obj.SetCastShadow( true );
		obj.SetNotVisibleInMainCamera( true );
		obj.SetNotVisibleInReflections( true );
		obj.SetOcclusionQueryDisabled( true );
		scene.transforms.Create( objEntity );  // identity — verts are world-space

		g_shadowProxyMeshes .push_back( meshEntity );
		g_shadowProxyObjects.push_back( objEntity );
	}
}

void GGTrees_WickedUpdate()
{
	if ( !g_wickedTreesSetup )
	{
		GGTrees_WickedSetup();
		if ( !g_wickedTreesSetup ) return;
	}

	auto& scene = wi::scene::GetScene();

	// Respect the GG-side visibility flags (parity fix: GGTrees_Update hides
	// trees during terrain regen via hide_until_update/draw_enabled, and the
	// pool used to ignore that and keep stale trees on screen). While hidden,
	// park the pool; on re-show force a full rebind because the instance data
	// (esp. Y heights) was regenerated while we were parked.
	static bool s_poolParked = false;
	bool forceRebind = false;
	if ( !ggtrees_global_params.draw_enabled )
	{
		if ( !s_poolParked )
		{
			for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
			{
				wi::scene::ObjectComponent* obj = scene.objects.GetComponent( g_treePoolEntities[ i ] );
				if ( obj ) obj->SetRenderable( false );
			}
			GGTrees_SetShadowProxiesVisible( scene, false );
			s_poolParked = true;
		}
		return;
	}
	if ( s_poolParked )
	{
		s_poolParked = false;
		forceRebind = true;
		GGTrees_SetShadowProxiesVisible( scene, true );
	}

	// Far-shadow proxy rebuild: deferred ~0.5s after the last tree-data change
	// so a paint stroke triggers one rebuild, not one per brush dab. Also
	// re-fires when the user toggles tree shadows (draw_shadows).
	{
		static uint32_t s_proxyStamp = ~0u;
		static int      s_proxyCountdown = -1;
		static int      s_lastDrawShadows = -1;
		if ( s_lastDrawShadows != ggtrees_global_params.draw_shadows )
		{
			s_lastDrawShadows = ggtrees_global_params.draw_shadows;
			forceRebind = true;   // refresh per-slot SetCastShadow via rebind
			s_proxyCountdown = 1;
		}
		if ( s_proxyStamp != g_treeInstanceStamp )
		{
			s_proxyStamp = g_treeInstanceStamp;
			s_proxyCountdown = 30;
		}
		if ( s_proxyCountdown >= 0 && --s_proxyCountdown < 0 )
		{
			GGTrees_BuildShadowProxies();
		}
	}

	// (Re)size per-tree state to the live instance array. A count change means
	// a level load / bulk tree edit — all bets on existing bindings are off.
	// Slot-vector size mismatch = first run or post-shutdown re-setup (fresh
	// pool entities, stale bindings).
	if ( g_treeToSlot.size() != (size_t)numTotalTrees ) forceRebind = true;
	if ( g_slotToTree.size() != (size_t)GG_TREE_POOL_SIZE ) forceRebind = true;
	if ( forceRebind )
	{
		g_treeToSlot      .assign( numTotalTrees, -1 );
		g_treeDesiredEpoch.assign( numTotalTrees, 0 );
		g_treeBand        .assign( numTotalTrees, 0 );
		g_slotToTree      .assign( GG_TREE_POOL_SIZE, UINT32_MAX );
		g_slotCache       .resize( GG_TREE_POOL_SIZE );
		for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
		{
			wi::scene::ObjectComponent* obj = scene.objects.GetComponent( g_treePoolEntities[ i ] );
			if ( obj ) obj->SetRenderable( false );
		}
	}

	// Camera position — used to pick the nearest N trees to spend the pool on.
	// XZ-only distance is sufficient: trees are all ~ground-level, and vertical
	// distance would distort priority when the camera flies above the terrain.
	const wi::scene::CameraComponent& camera = wi::scene::GetCamera();
	const float camX = camera.Eye.x;
	const float camZ = camera.Eye.z;

	// Selection throttle: the 400K candidate scan + nth_element + bind passes
	// only need to re-run when the answer can change — camera moved (>8 inches,
	// accumulated against the last SCANNED position so slow drift still lands),
	// tree data rebuilt (g_treeInstanceStamp bumps in GGTrees_UpdateInstances),
	// pool state reset, or a slow safety heartbeat for any mutation path that
	// doesn't announce itself. With a still camera this makes the whole pool
	// update near-free.
	static float    s_lastCamX = 1e30f, s_lastCamZ = 1e30f;  // far sentinel -> first frame always scans
	static uint32_t s_lastInstanceStamp = ~0u;
	static uint32_t s_heartbeat = 0;
	{
		const float mdx = camX - s_lastCamX;
		const float mdz = camZ - s_lastCamZ;
		const bool rescan = forceRebind
			|| s_lastInstanceStamp != g_treeInstanceStamp
			|| ( mdx * mdx + mdz * mdz ) > ( 8.0f * 8.0f )
			|| ++s_heartbeat >= 256;
		if ( !rescan ) return;
		s_heartbeat = 0;
		s_lastCamX = camX;
		s_lastCamZ = camZ;
		s_lastInstanceStamp = g_treeInstanceStamp;
	}

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

	// Mark this frame's desired set + LOD band per tree.
	constexpr float lod1Dist2 = GG_TREE_LOD1_DIST * GG_TREE_LOD1_DIST;
	constexpr float lod2Dist2 = GG_TREE_LOD2_DIST * GG_TREE_LOD2_DIST;
	g_treeEpoch++;
	for ( size_t k = 0; k < poolFill; k++ )
	{
		const uint32_t idx = candidates[ k ].idx;
		const float d2 = candidates[ k ].dist2;
		g_treeDesiredEpoch[ idx ] = g_treeEpoch;
		g_treeBand[ idx ] = ( d2 < lod1Dist2 ) ? 0 : ( d2 < lod2Dist2 ) ? 1 : 2;
	}

	// Pass A over slots: evict bindings that fell out of the nearest-N set,
	// keep (and verify) the rest. Kept slots cost a few array reads + float
	// compares — no component writes unless the tree's data or band changed.
	static std::vector<uint32_t> freeSlots;
	if ( freeSlots.capacity() < GG_TREE_POOL_SIZE ) freeSlots.reserve( GG_TREE_POOL_SIZE );
	freeSlots.clear();

	for ( uint32_t s = 0; s < GG_TREE_POOL_SIZE; s++ )
	{
		const uint32_t t = g_slotToTree[ s ];
		if ( t == UINT32_MAX ) { freeSlots.push_back( s ); continue; }

		bool evict = ( t >= numTotalTrees || g_treeDesiredEpoch[ t ] != g_treeEpoch );
		if ( !evict )
		{
			// Still desired — verify the source data hasn't changed under us
			// (editor tree move/retype/rescale mutates pAllTrees in place).
			InstanceTree* pTree = &pAllTrees[ t ];
			TreeSlotCache& c = g_slotCache[ s ];
			const uint8_t band = g_treeBand[ t ];
			const float scale = pTree->GetScaleFloat();
			if ( c.x != pTree->x || c.y != pTree->y || c.z != pTree->z ||
			     c.scale != scale || c.type != (uint8_t)pTree->GetType() )
			{
				evict = !BindTreeSlot( scene, s, t, band );  // full rebind in place
			}
			else if ( c.band != band )
			{
				// LOD boundary crossed — mesh swap only, transform unchanged.
				wi::scene::ObjectComponent* obj = scene.objects.GetComponent( g_treePoolEntities[ s ] );
				if ( obj )
				{
					obj->meshID = TreeMeshForBand( c.type, band );
					obj->SetCastShadow( ggtrees_global_params.draw_shadows != 0 && band == 0 );
					c.band = band;
				}
			}
		}
		if ( evict )
		{
			wi::scene::ObjectComponent* obj = scene.objects.GetComponent( g_treePoolEntities[ s ] );
			if ( obj ) obj->SetRenderable( false );
			if ( t < numTotalTrees && g_treeToSlot[ t ] == (int32_t)s ) g_treeToSlot[ t ] = -1;
			g_slotToTree[ s ] = UINT32_MAX;
			freeSlots.push_back( s );
		}
	}

	// Pass B over the desired set: bind whatever isn't already in a slot.
	for ( size_t k = 0; k < poolFill && !freeSlots.empty(); k++ )
	{
		const uint32_t idx = candidates[ k ].idx;
		if ( g_treeToSlot[ idx ] >= 0 ) continue;  // kept from a previous frame
		const uint32_t s = freeSlots.back();
		if ( BindTreeSlot( scene, s, idx, g_treeBand[ idx ] ) )
			freeSlots.pop_back();
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
	// Bindings refer to pool entities that no longer exist — drop them so the
	// next setup's update pass starts with a force-rebind.
	g_slotToTree.clear();
	g_treeToSlot.clear();
	g_treeDesiredEpoch.clear();
	g_treeBand.clear();

	// Shadow proxies + their materials go down with the pool.
	for ( wi::ecs::Entity e : g_shadowProxyObjects ) if ( e != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( e );
	for ( wi::ecs::Entity e : g_shadowProxyMeshes  ) if ( e != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( e );
	g_shadowProxyObjects.clear();
	g_shadowProxyMeshes.clear();
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		if ( g_shadowProxyMaterial[ t ] != wi::ecs::INVALID_ENTITY )
		{
			scene.Entity_Remove( g_shadowProxyMaterial[ t ] );
			g_shadowProxyMaterial[ t ] = wi::ecs::INVALID_ENTITY;
		}
	}
	for ( uint32_t t = 0; t < GG_TREE_TYPES; t++ )
	{
		// LOD1/LOD2 slots may ALIAS a higher LOD's entity (missing variant) —
		// remove only distinct entities to avoid double Entity_Remove.
		if ( g_treeMeshEntityLOD2[ t ] != wi::ecs::INVALID_ENTITY && g_treeMeshEntityLOD2[ t ] != g_treeMeshEntityLOD1[ t ] && g_treeMeshEntityLOD2[ t ] != g_treeMeshEntity[ t ] )
			scene.Entity_Remove( g_treeMeshEntityLOD2[ t ] );
		if ( g_treeMeshEntityLOD1[ t ] != wi::ecs::INVALID_ENTITY && g_treeMeshEntityLOD1[ t ] != g_treeMeshEntity[ t ] )
			scene.Entity_Remove( g_treeMeshEntityLOD1[ t ] );
		if ( g_treeMeshEntity            [ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeMeshEntity            [ t ] );
		if ( g_treeTrunkMaterialEntity   [ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeTrunkMaterialEntity   [ t ] );
		if ( g_treeBranchesMaterialEntity[ t ] != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeBranchesMaterialEntity[ t ] );
		g_treeMeshEntity            [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeMeshEntityLOD1        [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeMeshEntityLOD2        [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeTrunkMaterialEntity   [ t ] = wi::ecs::INVALID_ENTITY;
		g_treeBranchesMaterialEntity[ t ] = wi::ecs::INVALID_ENTITY;
	}
	g_wickedTreesSetup = false;
}

} // namespace GGTrees
