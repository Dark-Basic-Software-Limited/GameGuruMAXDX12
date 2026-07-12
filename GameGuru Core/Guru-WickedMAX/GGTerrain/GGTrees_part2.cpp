// GGTrees_part2.cpp — Phase 5: Colored cylinder tree placeholders on the new
// Wicked Engine terrain. Grown from `pAllTrees[]` (positions/type/scale set by
// the DX11-era GGTrees data pipeline) via a shared cylinder MeshComponent + a
// small fixed pool of ObjectComponents. Real LOD tree meshes come post-Phase-6.
//
// Colors match the tree reference overlay (`referenceColors[32]` in
// GGTerrainVirtualPBR_PS.hlsl) so each cylinder lines up in hue with the
// 3-pixel dot the overlay stamps at the same tree position — one glance at
// the terrain confirms placement + type correspondence.
//
// Included by GGTrees.cpp unity build; lives inside `namespace GGTrees` so
// pAllTrees / numTotalTrees / InstanceTree resolve without qualification.

namespace GGTrees
{

// Pool size caps the number of visible-tree cylinders per frame. pAllTrees[]
// is 400000 but visible trees in a typical level are well under this cap.
static constexpr uint32_t GG_TREE_POOL_SIZE  = 10000;
static constexpr uint32_t GG_TREE_CYL_SIDES  = 12;

// Trunk dimensions in world inches (GG uses 1 unit = 1 inch, so 30" ≈ 0.75 m
// radius and 400" ≈ 10 m height at scale 1.0). Multiplied by pTree->GetScaleFloat()
// per tree (typically 0.5–1.5).
static constexpr float GG_TREE_TRUNK_RADIUS  = 30.0f;
static constexpr float GG_TREE_TRUNK_HEIGHT  = 400.0f;

// Same 32-color palette used by the reference-overlay pixel shader
// (GGTerrainVirtualPBR_PS.hlsl:referenceColors). Type index masked with 31
// matches the shader — types 32..37 alias back to 0..5, which is fine as long
// as the cylinders line up in hue with the overlay dots.
static const XMFLOAT4 g_treeReferenceColors[ 32 ] =
{
	{ 0.18f, 0.55f, 0.15f, 1.0f },  //  0
	{ 0.95f, 0.85f, 0.25f, 1.0f },  //  1
	{ 0.76f, 0.70f, 0.50f, 1.0f },  //  2
	{ 0.10f, 0.40f, 0.10f, 1.0f },  //  3
	{ 0.55f, 0.55f, 0.55f, 1.0f },  //  4
	{ 0.90f, 0.30f, 0.10f, 1.0f },  //  5
	{ 0.20f, 0.60f, 0.85f, 1.0f },  //  6
	{ 0.80f, 0.20f, 0.60f, 1.0f },  //  7
	{ 0.95f, 0.60f, 0.10f, 1.0f },  //  8
	{ 0.30f, 0.80f, 0.70f, 1.0f },  //  9
	{ 0.70f, 0.10f, 0.10f, 1.0f },  // 10
	{ 0.50f, 0.85f, 0.20f, 1.0f },  // 11
	{ 0.35f, 0.20f, 0.70f, 1.0f },  // 12
	{ 0.90f, 0.75f, 0.55f, 1.0f },  // 13
	{ 0.10f, 0.35f, 0.55f, 1.0f },  // 14
	{ 0.85f, 0.85f, 0.10f, 1.0f },  // 15
	{ 0.60f, 0.30f, 0.15f, 1.0f },  // 16
	{ 0.40f, 0.80f, 0.40f, 1.0f },  // 17
	{ 0.75f, 0.45f, 0.75f, 1.0f },  // 18
	{ 0.25f, 0.65f, 0.45f, 1.0f },  // 19
	{ 0.90f, 0.45f, 0.45f, 1.0f },  // 20
	{ 0.45f, 0.45f, 0.80f, 1.0f },  // 21
	{ 0.80f, 0.65f, 0.20f, 1.0f },  // 22
	{ 0.55f, 0.85f, 0.85f, 1.0f },  // 23
	{ 0.85f, 0.35f, 0.35f, 1.0f },  // 24
	{ 0.35f, 0.70f, 0.20f, 1.0f },  // 25
	{ 0.65f, 0.20f, 0.45f, 1.0f },  // 26
	{ 0.95f, 0.70f, 0.40f, 1.0f },  // 27
	{ 0.20f, 0.50f, 0.70f, 1.0f },  // 28
	{ 0.70f, 0.70f, 0.30f, 1.0f },  // 29
	{ 0.50f, 0.30f, 0.50f, 1.0f },  // 30
	{ 0.80f, 0.80f, 0.80f, 1.0f },  // 31
};

static bool             g_wickedTreesSetup    = false;
static wi::ecs::Entity  g_treeMeshEntity      = wi::ecs::INVALID_ENTITY;
static wi::ecs::Entity  g_treeMaterialEntity  = wi::ecs::INVALID_ENTITY;
static wi::ecs::Entity  g_treePoolEntities[ GG_TREE_POOL_SIZE ] = { wi::ecs::INVALID_ENTITY };

static void GGTrees_WickedSetup()
{
	auto& scene = wi::scene::GetScene();

	// --- (1) Shared cylinder mesh --------------------------------------------------
	// Unit cylinder: base ring at Y=0, top ring at Y=1, radius=1. Per-tree
	// TransformComponent supplies the real world dimensions via non-uniform scale.
	// Side walls only (no caps) — cheaper, and trees are only ever viewed from the
	// side. Radial per-vertex normals so lighting reads round even at 12 segments.
	g_treeMeshEntity = wi::ecs::CreateEntity();
	wi::scene::MeshComponent& mesh = scene.meshes.Create( g_treeMeshEntity );

	mesh.vertex_positions.reserve( GG_TREE_CYL_SIDES * 2 );
	mesh.vertex_normals  .reserve( GG_TREE_CYL_SIDES * 2 );
	for ( uint32_t i = 0; i < GG_TREE_CYL_SIDES; i++ )
	{
		float theta = ( (float)i / (float)GG_TREE_CYL_SIDES ) * 6.28318530718f;
		float cx = cosf( theta );
		float sz = sinf( theta );
		// Bottom ring, then top ring — interleaved so a side quad is 4
		// consecutive verts around the cylinder.
		mesh.vertex_positions.push_back( XMFLOAT3( cx, 0.0f, sz ) );
		mesh.vertex_normals  .push_back( XMFLOAT3( cx, 0.0f, sz ) );
		mesh.vertex_positions.push_back( XMFLOAT3( cx, 1.0f, sz ) );
		mesh.vertex_normals  .push_back( XMFLOAT3( cx, 0.0f, sz ) );
	}

	mesh.indices.reserve( GG_TREE_CYL_SIDES * 6 );
	for ( uint32_t i = 0; i < GG_TREE_CYL_SIDES; i++ )
	{
		uint32_t i0 = ( i * 2 );
		uint32_t i1 = ( i * 2 ) + 1;
		uint32_t i2 = ( ( ( i + 1 ) % GG_TREE_CYL_SIDES ) * 2 );
		uint32_t i3 = ( ( ( i + 1 ) % GG_TREE_CYL_SIDES ) * 2 ) + 1;
		// Two triangles per side quad. CCW winding when viewed from outside.
		mesh.indices.push_back( i0 ); mesh.indices.push_back( i1 ); mesh.indices.push_back( i3 );
		mesh.indices.push_back( i0 ); mesh.indices.push_back( i3 ); mesh.indices.push_back( i2 );
	}

	// --- (2) Shared material -------------------------------------------------------
	// Plain white PBR base — per-tree tint comes from ObjectComponent.color, which
	// multiplies the baseColor in the fragment shader. This keeps one material for
	// the whole pool while still giving each cylinder its own type-coded color.
	g_treeMaterialEntity = wi::ecs::CreateEntity();
	wi::scene::MaterialComponent& mat = scene.materials.Create( g_treeMaterialEntity );
	mat.SetBaseColor( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	mat.SetRoughness( 1.0f );
	mat.SetMetalness( 0.0f );
	mat.SetReflectance( 0.02f );
	mat.CreateRenderData();

	// --- (3) One subset covering the whole cylinder --------------------------------
	wi::scene::MeshComponent::MeshSubset subset;
	subset.materialID  = g_treeMaterialEntity;
	subset.indexOffset = 0;
	subset.indexCount  = (uint32_t)mesh.indices.size();
	mesh.subsets.push_back( subset );
	mesh.CreateRenderData();

	// --- (4) Entity pool ----------------------------------------------------------
	// Each slot has its own ObjectComponent (points at the shared mesh) and its own
	// TransformComponent (per-frame position/scale). Starts hidden until the update
	// loop assigns it to a tree this frame.
	for ( uint32_t i = 0; i < GG_TREE_POOL_SIZE; i++ )
	{
		wi::ecs::Entity e = wi::ecs::CreateEntity();
		wi::scene::ObjectComponent& obj = scene.objects.Create( e );
		obj.meshID = g_treeMeshEntity;
		obj.SetRenderable( false );
		scene.transforms.Create( e );
		g_treePoolEntities[ i ] = e;
	}

	g_wickedTreesSetup = true;
	wi::backlog::post( ( "GGTrees: wicked cylinder pool ready ("
		+ std::to_string( GG_TREE_POOL_SIZE ) + " slots, " + std::to_string( GG_TREE_CYL_SIDES )
		+ "-sided cylinder)" ).c_str() );
}

void GGTrees_WickedInit()
{
	// Nothing eager here — the setup path needs the scene to be live and MSAA-free
	// (same phase requirement as SetupWickedGrass / SetupWickedTerrainMaterials).
	// Lazy first-touch in GGTrees_WickedUpdate keeps ordering safe.
	g_wickedTreesSetup   = false;
	g_treeMeshEntity     = wi::ecs::INVALID_ENTITY;
	g_treeMaterialEntity = wi::ecs::INVALID_ENTITY;
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

		wi::ecs::Entity e = g_treePoolEntities[ poolIndex ];
		wi::scene::ObjectComponent*     obj   = scene.objects   .GetComponent( e );
		wi::scene::TransformComponent*  xform = scene.transforms.GetComponent( e );
		if ( !obj || !xform ) { poolIndex++; continue; }

		float treeScale = pTree->GetScaleFloat();  // ~0.5–1.5
		xform->ClearTransform();
		xform->Scale( XMFLOAT3(
			GG_TREE_TRUNK_RADIUS * treeScale,
			GG_TREE_TRUNK_HEIGHT * treeScale,
			GG_TREE_TRUNK_RADIUS * treeScale ) );
		xform->Translate( XMFLOAT3( pTree->x, pTree->y, pTree->z ) );
		xform->UpdateTransform();

		int type = pTree->GetType();
		obj->color = g_treeReferenceColors[ type & 31 ];
		obj->SetRenderable( true );

		poolIndex++;
	}

	// Hide any pool slots we didn't fill this frame — leftover from previous
	// frame's larger tree count, or from a level with fewer trees than the cap.
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
	if ( g_treeMeshEntity     != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeMeshEntity );
	if ( g_treeMaterialEntity != wi::ecs::INVALID_ENTITY ) scene.Entity_Remove( g_treeMaterialEntity );
	g_treeMeshEntity     = wi::ecs::INVALID_ENTITY;
	g_treeMaterialEntity = wi::ecs::INVALID_ENTITY;
	g_wickedTreesSetup   = false;
}

} // namespace GGTrees
