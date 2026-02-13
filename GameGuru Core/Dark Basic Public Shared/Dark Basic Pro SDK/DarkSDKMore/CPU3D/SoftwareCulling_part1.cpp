DB float OccluderCheck( int id )                           
{
#ifdef _TIME_TAKEN_
ResetTimer();
#endif

	pRendlist = &rend_list;
	GGVECTOR3 vecPosition;

	// clear renderlist so we can use it for occluders
	pRendlist->num_polys = 0;

	sObject* p = GetObjectData ( id );
	sObject* p2;

	if (!p ) return 0;

	if ( p->pInstanceOfObject )
		p2 = p->pInstanceOfObject;
	else
		p2 = p;

	VECTOR4D world_pos;
	world_pos.x = p->position.vecPosition.x;
	world_pos.y = p->position.vecPosition.y;
	world_pos.z = p->position.vecPosition.z;
	world_pos.w = 1.0f;

	float max_radius;
	float original_radius;
	max_radius = p2->collision.fScaledLargestRadius * 1.25f;
	if ( p2->collision.fScaledRadius > max_radius )
		max_radius = p2->collision.fScaledRadius * 1.25f;
	else if ( p->collision.fRadius > max_radius )
		max_radius = p->collision.fRadius * 1.25f;

	original_radius = max_radius;

	float dist = sqrt( ((world_pos.x - cam.pos.x)*(world_pos.x - cam.pos.x)) + ((world_pos.y - cam.pos.y)*(world_pos.y - cam.pos.y)) +((world_pos.z - cam.pos.z)*(world_pos.z - cam.pos.z)) );

#ifdef HIDE_SMALL_OBJECTS
	if ( max_radius < 5.0f && dist > 3000.0f )
	{
		return 0;
	}
	//if ( max_radius < 100.0f && dist > 5000.0f )
	if ( dist > 5000.0f )
	{
		return 0;
	}
#endif

	POINT4D sphere_pos; // hold result of transforming center of bounding sphere

	// transform point
	MatMulVector4D4X4(&world_pos, &cam.mcam, &sphere_pos);
	
	// step 2:  based on culling flags remove the object
	// cull only based on z clipping planes
	// test far plane

	if ( ((sphere_pos.z - max_radius) > cam.far_clip_z) || ((sphere_pos.z + max_radius) < cam.near_clip_z) )
	{
		return 0;
	}

	// cull only based on x clipping planes
	// test the the right and left clipping planes against the leftmost and rightmost
	// points of the bounding sphere
	float z_test = (0.5)*cam.viewplane_width*sphere_pos.z/cam.view_dist;

	if ( ((sphere_pos.x-max_radius) > z_test)  || ((sphere_pos.x+max_radius) < -z_test) )  
	{
		return 0;
	}

	// cull only based on y clipping planes
	// test the the top and bottom clipping planes against the bottommost and topmost
	// points of the bounding sphere
	z_test = (0.5)*cam.viewplane_height*sphere_pos.z/cam.view_dist;

	if ( ((sphere_pos.y-max_radius) > z_test)  || ((sphere_pos.y+max_radius) < -z_test) )
	{
		return 0;
	}

	//================================================================================
	float fWidth1  = p2->collision.vecMin.x;// / p->position.vecScale.x;
	float fHeight1 = p2->collision.vecMin.y;// / p->position.vecScale.y;
	float fDepth1  = p2->collision.vecMin.z;// / p->position.vecScale.z;
	float fWidth2  = p2->collision.vecMax.x;// * p->position.vecScale.x;
	float fHeight2 = p2->collision.vecMax.y;// * p->position.vecScale.y;
	float fDepth2  = p2->collision.vecMax.z;// * p->position.vecScale.z;

	//================================================================================				
	// create the box, unrolled for speed

	// POLY 1
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================	
	// POLY 2
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 3
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 4
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 5
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 6
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 7
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 8
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 9
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 10
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth2, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 11
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================
	// POLY 12
	// point pointer to polygon structure
	pRendlist->poly_ptrs[pRendlist->num_polys] = &pRendlist->poly_data[pRendlist->num_polys];

	// copy fields 
	pRendlist->poly_data[pRendlist->num_polys].state   = POLY4DV2_STATE_ACTIVE;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight1, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[0].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth2 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[1].w = 1;

	vecPosition = GGVECTOR3 ( fWidth1, fHeight2, fDepth1 );
	GGVec3TransformCoord ( &vecPosition, &vecPosition, &p->position.matWorld );

	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].x = vecPosition.x;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].y = vecPosition.y;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].z = vecPosition.z;
	pRendlist->poly_data[pRendlist->num_polys].vertexList[2].w = 1;

	// first set this node to point to previous node and next node (null)
	pRendlist->poly_data[pRendlist->num_polys].next = NULL;
	pRendlist->poly_data[pRendlist->num_polys].prev = &pRendlist->poly_data[pRendlist->num_polys-1];

	// now set previous node to point to this node
	pRendlist->poly_data[pRendlist->num_polys-1].next = &pRendlist->poly_data[pRendlist->num_polys];

	// increment number of polys in list
	pRendlist->num_polys++;
	//================================================================================

   if ( pRendlist->num_polys > 0 )
   {
		pRendlist->poly_data[0].next = NULL;
		pRendlist->poly_data[0].prev = NULL;
   }

	// apply world to camera transform
	RenderListWorldToCamera(&rend_list, &cam);


	RenderListClipPolys(&rend_list, &cam, CLIP_POLY_X_PLANE | CLIP_POLY_Y_PLANE | CLIP_POLY_Z_PLANE );

	// apply camera to perspective transformation
	RenderListCameraToPerspective(&rend_list, &cam);

	// apply screen transform
	RenderListPerspectiveToScreen(&rend_list, &cam);

	float tl_x = 10000; 
	float tl_y = 10000;
	float br_x = -10000;
	float br_y = -10000;
	UINT depth = 0xffffffff;

	for ( int c = 0 ; c < rend_list.num_polys; c++ )
	{
		for ( int v = 0 ; v < 3 ; v++ )
		{

			// is this polygon valid?
			if ((rend_list.poly_ptrs[c]==NULL) || !(rend_list.poly_ptrs[c]->state & POLY4DV2_STATE_ACTIVE) ||
				 (rend_list.poly_ptrs[c]->state & POLY4DV2_STATE_CLIPPED ) )
				   continue; // move onto next poly

			if ( rend_list.poly_ptrs[c]->vertexList[v].x < tl_x ) tl_x = rend_list.poly_ptrs[c]->vertexList[v].x;
			if ( rend_list.poly_ptrs[c]->vertexList[v].y < tl_y ) tl_y = rend_list.poly_ptrs[c]->vertexList[v].y;

			if ( rend_list.poly_ptrs[c]->vertexList[v].x > br_x ) br_x = rend_list.poly_ptrs[c]->vertexList[v].x;
			if ( rend_list.poly_ptrs[c]->vertexList[v].y > br_y ) br_y = rend_list.poly_ptrs[c]->vertexList[v].y;

			if ( (UINT)rend_list.poly_ptrs[c]->vertexList[v].z < depth ) depth = (UINT)rend_list.poly_ptrs[c]->vertexList[v].z;
		}
	}

	if ( tl_x == 10000 || tl_y == 10000 || br_x == -10000 || br_y == 10000 ) return 0;

	UINT zpixel;

	if ( tl_x > SCREEN_WIDTH-10 )
	{
		return 0;
	}
	if ( tl_y > cam.viewport_height-10 )
	{
		return 0;
	}

	if ( br_x < 10 ) 
	{
		return 0;
	}
	if ( br_y < 10 )
	{
		return 0;
	}

	if ( tl_x < 0 ) tl_x = 0;
	if ( tl_y < 0 ) tl_y = 0;

	if ( br_x >  SCREEN_WIDTH-1 ) br_x = SCREEN_WIDTH-1;
	if ( br_y > cam.viewport_height-1 ) br_y = cam.viewport_height-1;

	if ( tl_x > SCREEN_WIDTH * 0.7f ) return 0;
	if ( br_x < SCREEN_WIDTH * 0.3f ) return 0;

	if ( tl_y > cam.viewport_height * 0.7f ) return 0;
	if ( br_y < cam.viewport_height * 0.3f ) return 0;

	float W = (original_radius / dist) * 150.0f;

	return W;

#ifdef _TIME_TAKEN_
UpdateTimer();
#endif

}

void CPU3DAddOccluder ( int id )
{
	OccluderList.push_back ( id );
}

void CPU3DAddOccludee ( int id , bool isCharacter )
{
	OccludeeList.push_back ( id );
	OccludeeListNotVis.push_back ( 0 );
	OccludeeListIsVis.push_back ( 2 );
	OccludeeListIsCharacter.push_back ( isCharacter );
}

void CPU3DSetCameraIndex ( int id )
{
	CameraIndex = id;
}

void CPU3DClear()
{

	sObject* p;

	lightmappedterrainoffset = MAXINT32;
	lightmappedobjectoffset = MAXINT32;

	OccluderList.clear();
	OccludeeList.clear();
	OccludeeListNotVis.clear();
	OccludeeListIsVis.clear();
	OccludeeListIsCharacter.clear();

	OccluderListClosest.clear();
	OccluderListTemp.clear();
	OccluderListTempSize.clear();
	OccluderListDrawn.clear();

	smallDistanceMulti = 1;
	prevSmallDistanceMulti = 0;

	occluderBigVisualChange = false;

	OccluderQuickTimeDelay = 0;

	for ( int c = 0 ; c < MAX_CACHED_OBJECTS ; c++ )
	{
		if ( cachedPolys[c] != NULL )
		{
			delete cachedPolys[c];
			cachedPolys[c] = NULL;
		}
	}
}

void CPU3DSetPolyCount( int amount )
{
	if ( amount < 0 ) amount = 0;
	if ( amount > 100 ) amount = 100;

	AggresiveMode = amount / 100.0f;

	//Changed to use max polys always	
	if (amount == 0) {
		cpu3dMaxPolys = 0;
		g_enabeleverything = true;
	}
	else
		cpu3dMaxPolys = CPU_3D_MAX_POLY_SETTING;

	//Recent small distance multi to force a change
	smallDistanceMulti = 1.0f;
}

void CPU3DShow( int show )
{
	ShowZbuffer = show;
}

bool doneOcclude = false;

void Sync ( void );

float camax, camay, camaz;

void CPU3DOcclude ()
{
	if ( cpu3dMaxPolys <= 0 )
	{
		camOldFar = 0;
		return;
	}

	begin();

	tagCameraData* camData = (tagCameraData*)GetCameraInternalData ( CameraIndex );
	if ( camData==NULL ) return;
	camax = CameraAngleX(0); camay = CameraAngleY(0); camaz = CameraAngleZ(0);

	#define SHOWZBUFFER
	#ifdef SHOWZBUFFER
	if ( ShowZbuffer )
	{
		UINT   *zb_ptr    =  (UINT *)zbuffer.zbuffer;
		int max = 0;
		int min = 100000;

		#ifdef DRAW_ZBUFFER
		LockPixels();

		for (int y = 0; y < SCREEN_HEIGHT; y+=8)
		{
			for (int x = 0; x < SCREEN_WIDTH; x+=8)
			{
				UINT zpixel = zb_ptr[x + y*SCREEN_WIDTH];

				zpixel = UINT(zpixel / 66191.36f);

				#ifdef ZBUFFER_DRAW_MONO
				zpixel *= 0.1f;
				zpixel = 256 - zpixel;
				if ( zpixel > 0 )
					pix (x,y,255);
				#else
				zpixel *= 0.1f;
				zpixel = 256 - zpixel;
				if ( zpixel < min ) min = zpixel;
				if ( zpixel < 0 ) zpixel = 0;
				if ( zpixel > 256) zpixel = 256;
				if ( zpixel > max && zpixel < 59392 ) max = zpixel;
				if ( zpixel > 0 )
					OccluderPix ( x/8 , y/8 , (int)zpixel);
				#endif
			}
		}

		#ifndef METHOD_TWO
			// Draw Mip Maps
			int size = 128;
			int yOffset = 144;
			for ( int a = 1 ; a < 9 ; a++ )
			{
				for (int y = 0; y < size; y++)
				{
					for (int x = 0; x < size; x++)
					{
						UINT zpixel = mipMaps[a][x + y*size];
			#ifdef ZBUFFER_DRAW_MONO
						zpixel *= 0.1f;
						zpixel = 256 - zpixel;
						if ( zpixel > 0 )
							pix (x,y,255);
			#else
						zpixel *= 0.1f;
						zpixel = 256 - zpixel;
						if ( zpixel < min ) min = zpixel;
						if ( zpixel < 0 ) zpixel = 0;
						if ( zpixel > 256) zpixel = 256;
						if ( zpixel > max && zpixel < 59392 ) max = zpixel;
						if ( zpixel > 0 )
							pix ( x , y+yOffset , (int)zpixel);
			#endif
					}
				}

				yOffset += size+1;
				size /= 2;
			}
		#endif

		UnlockPixels();

		#endif
	}
	#endif
}

extern int iGridObjectStart, iGridObjectEnd;
extern int Timer(void);
extern float CameraPositionX ( int iID );
extern float CameraPositionY ( int iID );
extern float CameraPositionZ ( int iID );

extern float CameraAngleX ( int iID );
extern float CameraAngleY ( int iID );
extern float CameraAngleZ ( int iID );

extern bool OccluderCheckingForMultiplayer();

int occluderLastTime = 0;
float oldcamx, oldcamy, oldcamz, oldcamrx, oldcamry, oldcamrz;
int oldPolyCount = 0;
float multiPosX, multiPosY, multiPosZ;

float oldAggresiveMode = 0;

void CPU3DDoOcclude()
{
	rend_list.num_polys = 0;

	if ( !g_occluderOn )
	{
		sObject* p;
		for ( int c = 0 ; c < OccluderList.size() ; c++ )
		{
			p = GetObjectData ( OccluderList[c] );
			if ( p ) p->bUniverseVisible = true;
		}
		for ( int c = 0 ; c < OccludeeList.size() ; c++ )
		{
			p = GetObjectData ( OccludeeList[c] );
			if ( p ) p->bUniverseVisible = true;
		}

		doneOcclude = false;

		CPU3DClear();

		smallDistanceMulti = 1;
		prevSmallDistanceMulti = 0;

		smallDistanceMulti = 1;
		oldcamx = -9999;
		g_occluderf9Mode = false;

		if ( g_hOccluderEnd ) SetEvent ( g_hOccluderEnd );

		return;
	}

	//If f9 mode is active, show everything.
	if ( g_occluderf9Mode )
	{
		sObject* p;

		for ( int c = 0 ; c < OccluderList.size() ; c++ )
		{
			p = GetObjectData ( OccluderList[c] );
			if ( p ) p->bUniverseVisible = true;
		}

		for ( int c = 0 ; c < OccludeeList.size() ; c++ )
		{
			p = GetObjectData ( OccludeeList[c] );
			if ( p ) p->bUniverseVisible = true;
		}

		// 121115 - also need to show all GRASS if we ar ein F9 mode
		for ( int c = iGridObjectStart; c < iGridObjectEnd ; c++ )
		{
			if ( c > 0 )
			{
				p = GetObjectData ( c );
				if ( p ) p->bUniverseVisible = true;
			}
		}

		doneOcclude = false;
		oldcamx = -9999;
		smallDistanceMulti = 1;
		return;
	}

	if (cpu3dMaxPolys <= 0)
	{
		//force updated next time occluder is on
		oldcamx = -9999;

		if (doneOcclude)
		{
			sObject* p;

			for (int c = 0; c < OccludeeList.size(); c++)
			{
				p = GetObjectData(OccludeeList[c]);
				if (p) p->bUniverseVisible = true;
			}

			doneOcclude = false;
			smallDistanceMulti = 1;
		}

		if (g_enabeleverything)
		{
			//PE: If they do stuff in tab tab, we need to display all object again, if occluder is moved to 0.
			sObject* p;
			for (int c = 0; c < OccluderList.size(); c++)
			{
				p = GetObjectData(OccluderList[c]);
				if (p) p->bUniverseVisible = true;
			}
			for (int c = 0; c < OccludeeList.size(); c++)
			{
				p = GetObjectData(OccludeeList[c]);
				if (p) p->bUniverseVisible = true;
			}
			for (int c = iGridObjectStart; c < iGridObjectEnd; c++)
			{
				if (c > 0)
				{
					p = GetObjectData(c);
					if (p) p->bUniverseVisible = true;
				}
			}
			g_enabeleverything = false;
		}
		return;
	}

	// If the camera has been spun quite a bit, reset everything
	// This is to fix fast spinning around
	if ( abs(camay - oldCameraAngleY ) > 10 )
	{

		sObject* p;

		for ( int c = 0 ; c < OccludeeList.size() ; c++ )
		{
			OccludeeListNotVis[c] = 0;
		}
		OccluderQuickTimeDelay = MAXTimer();
		occluderLastTime = 0;
	}

	if ( MAXTimer() - OccluderQuickTimeDelay < 500 ) 
	{
		sObject* p;
		for ( int c = 0 ; c < OccludeeList.size() ; c++ )
		{
			OccludeeListNotVis[c] = 0;
		}
	}
	else
		OccluderQuickTimeDelay = 0;

	oldCameraAngleY = camay;
	

	if ( oldPolyCount != cpu3dMaxPolys ) oldcamx = -9999;
	oldPolyCount = cpu3dMaxPolys;

	//If the aggresive mode has changed, ensure we re calc visibility
	if ( AggresiveMode != oldAggresiveMode )
	{
		oldcamx = -9999;
	}
	oldAggresiveMode = AggresiveMode;

	howManyOccluders = OccluderList.size();
	howManyOccludersDrawn = 0;
	howManyOccludees = OccludeeList.size();
	howManyOccludeesHidden = 0;

	oldcamx = cam.pos.x; oldcamy = cam.pos.y; oldcamz = cam.pos.z;
	oldcamrx = camax; oldcamry = camay; oldcamrz = camaz;

	doneOcclude = true;

	OccluderListClosest.clear();
	OccluderListTemp.clear();
	OccluderListTempSize.clear();
	OccluderListDrawn.clear();

	float size;
	float biggest = 0;
	float biggestIndex;
	float dist;
	float closest;
	float closestIndex;
	sObject* p;
	VECTOR4D world_pos;	

	for ( int c = 0 ; c < OccluderList.size() ; c++ )
	{
		OccluderListTemp.push_back ( OccluderList[c] );
		OccluderListTempSize.push_back ( 99999 );
	}

	while ( OccluderListTemp.size() > 0 )
	{
		closest = 0xffffffff;
		biggest = -1;

		for ( int c = 0 ; c < OccluderListTemp.size() ; c++ )
		{
#ifndef USE_SCREEN_SPACE_OCCLUDER
			p = GetObjectData ( OccluderListTemp[c] );

			world_pos.x = p->position.vecPosition.x;
			world_pos.y = p->position.vecPosition.y;
			world_pos.z = p->position.vecPosition.z;

			dist = sqrt( ((world_pos.x - cam.pos.x)*(world_pos.x - cam.pos.x)) + ((world_pos.y - cam.pos.y)*(world_pos.y - cam.pos.y)) +((world_pos.z - cam.pos.z)*(world_pos.z - cam.pos.z)) );			

			if ( dist < closest )
			{
				closest = dist;
				closestIndex = c;
			}
#else
			if ( OccluderListTempSize[c] == 99999 )
				OccluderListTempSize[c] = OccluderCheck ( OccluderListTemp[c] );

			pRendlist->num_polys = 0;

			if (  OccluderListTemp[c] == 70002 ) trackingSize = OccluderListTempSize[c];

			size = OccluderListTempSize[c];

			if ( size > biggest )
			{
				biggest = size;
				biggestIndex = c;
			}
#endif

		}

#ifndef USE_SCREEN_SPACE_OCCLUDER
		OccluderListClosest.push_back ( OccluderListTemp[closestIndex] );

		p = GetObjectData ( OccluderListTemp[closestIndex] );
		if ( p ) p->bUniverseVisible = true;

		OccluderListTemp.erase (OccluderListTemp.begin()+closestIndex);
#else
		OccluderListClosest.push_back ( OccluderListTemp[biggestIndex] );

		p = GetObjectData ( OccluderListTemp[biggestIndex] );

		OccluderListTemp.erase (OccluderListTemp.begin()+biggestIndex);
#endif

	}

	for ( int c = 0 ; c < OccluderListClosest.size() ; c++ )
	{
		if ( DrawOccluder ( OccluderListClosest[c]) ) 
		{
			OccluderListDrawn.push_back ( OccluderListClosest[c]);
			howManyOccludersDrawn++;
		}
	}		

	Draw();	

	bool found = false;

	for ( int c = 0 ; c < OccludeeList.size() ; c++ )
	{
		found = false;
		if ( !found )
		{
			OccludeeCheck ( OccludeeList[c] , OccludeeListIsCharacter[c] );		

			// to stop flickering where something might go in and out of vis, objects must be not visible for 2 frames before they will get set to not visible
			p = GetObjectData ( OccludeeList[c] );
			if ( p )
			{
				// either the ocl is invisible but slowly ready to be visible thanks to the delayed OccludeeListNotVis action
				// OR the ocl is triggered for eventual hiding so should not fail this condition otherwise OccludeeListNotVis=0 would make it appear again
				if (  p->bUniverseVisible == false || p->dwCountdownToUniverseVisOff > 0 )
				{
					if ( OccludeeListNotVis[c] < 2 )//&& !forceIsVisCheck )
					{
						OccludeeListNotVis[c]++;
						p->bUniverseVisible = true;
					}
					else
					{
						howManyOccludeesHidden++;
					}
				}
				else // if the are visible, we set the counter back to 0
				{
					OccludeeListNotVis[c] = 0;
					if ( OccludeeListIsVis[c] < 1 )
					{
						OccludeeListIsVis[c]++;
						p->bUniverseVisible = false;
						howManyOccludeesHidden++;
					}
					else
					{						
						OccludeeListNotVis[c] = 0;
					}
				}
			}
		}
		else
		{
			p = GetObjectData ( OccludeeList[c] );
			if ( p )
			{
				OccludeeListNotVis[c] = 0;
				p->bUniverseVisible = true;
			}
		}
	}

	// 051016 - A special occludee system added which delays hiding ocludees until a countdown has completed
	for ( int c = 0 ; c < OccludeeList.size() ; c++ )
	{
		p = GetObjectData ( OccludeeList[c] );
		if ( p )
		{
			if ( p->dwCountdownToUniverseVisOff > 0 ) 
			{
				p->dwCountdownToUniverseVisOff = p->dwCountdownToUniverseVisOff - 1;
				if ( p->dwCountdownToUniverseVisOff == 0 )
				{
					p->bUniverseVisible = false;
				}
			}
		}
	}

	// Veg Check too
	for ( int c = iGridObjectStart; c < iGridObjectEnd ; c++ )
	{
		if ( c > 0 )
		{
			if ( ObjectExist ( c ) == 1 ) OccludeeCheck ( c , false );
		}
	}

	//If too many occludees drawn, cut the range down they will be drawn within
#ifdef SHOW_LESS_OCCLUDEES_ON_HIGH_PRIM_CALLS
	if ( g_pGlob->dwNumberOfPrimCalls > 500 && smallDistanceMulti != prevSmallDistanceMulti ) // 500
	{
		if  ( AggresiveMode == 0 ) 
			smallDistanceMulti = 0.0f;
		else
			smallDistanceMulti = (2.0f * AggresiveMode) + 1.0f;

		prevSmallDistanceMulti = smallDistanceMulti;
		oldcamx = -9999;
		//smallDistanceMulti = 3;
		multiPosX = cam.pos.x;
		multiPosY = cam.pos.y;
		multiPosZ = cam.pos.z;
	}
	// Once we have moved from the original position the draw calls dropped and the draw calls have dropped down
	// We can switch back to showing again
	else if ( g_pGlob->dwNumberOfPrimCalls < 200 && smallDistanceMulti != 1 ) //200
	{
		float dx2 = multiPosX - cam.pos.x;
		float dy2 = multiPosY - cam.pos.y;
		float dz2 = multiPosZ - cam.pos.z;
		float dist2 = sqrt( (dx2*dx2) + (dy2*dy2) + (dz2*dz2) );

		if ( dist2 > 3000 )
		{
			smallDistanceMulti = 1;
			prevSmallDistanceMulti = 0;
			oldcamx = -9999;
		}		
	}
#endif
}
