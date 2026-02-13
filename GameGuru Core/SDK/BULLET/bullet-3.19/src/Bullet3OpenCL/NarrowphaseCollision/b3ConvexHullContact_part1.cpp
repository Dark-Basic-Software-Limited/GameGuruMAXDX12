__kernel void clipCompoundsHullHullKernel(__global const b3Int4* gpuCompoundPairs,
										  __global const b3RigidBodyData* rigidBodies,
										  __global const b3Collidable* collidables,
										  __global const b3ConvexPolyhedronData* convexShapes,
										  __global const b3AlignedObjectArray<b3Float4>& vertices,
										  __global const b3AlignedObjectArray<b3Float4>& uniqueEdges,
										  __global const b3AlignedObjectArray<b3GpuFace>& faces,
										  __global const b3AlignedObjectArray<int>& indices,
										  __global const b3GpuChildShape* gpuChildShapes,
										  __global const b3AlignedObjectArray<b3Float4>& gpuCompoundSepNormalsOut,
										  __global const b3AlignedObjectArray<int>& gpuHasCompoundSepNormalsOut,
										  __global struct b3Contact4Data* globalContactsOut,
										  int* nGlobalContactsOut,
										  int numCompoundPairs, int maxContactCapacity, int i)
{
	//	int i = get_global_id(0);
	int pairIndex = i;

	float4 worldVertsB1[64];
	float4 worldVertsB2[64];
	int capacityWorldVerts = 64;

	float4 localContactsOut[64];
	int localContactCapacity = 64;

	float minDist = -1e30f;
	float maxDist = 0.0f;

	if (i < numCompoundPairs)
	{
		if (gpuHasCompoundSepNormalsOut[i])
		{
			int bodyIndexA = gpuCompoundPairs[i].x;
			int bodyIndexB = gpuCompoundPairs[i].y;

			int childShapeIndexA = gpuCompoundPairs[i].z;
			int childShapeIndexB = gpuCompoundPairs[i].w;

			int collidableIndexA = -1;
			int collidableIndexB = -1;

			b3Quat ornA = rigidBodies[bodyIndexA].m_quat;
			float4 posA = rigidBodies[bodyIndexA].m_pos;

			b3Quat ornB = rigidBodies[bodyIndexB].m_quat;
			float4 posB = rigidBodies[bodyIndexB].m_pos;

			if (childShapeIndexA >= 0)
			{
				collidableIndexA = gpuChildShapes[childShapeIndexA].m_shapeIndex;
				float4 childPosA = gpuChildShapes[childShapeIndexA].m_childPosition;
				b3Quat childOrnA = gpuChildShapes[childShapeIndexA].m_childOrientation;
				float4 newPosA = b3QuatRotate(ornA, childPosA) + posA;
				b3Quat newOrnA = b3QuatMul(ornA, childOrnA);
				posA = newPosA;
				ornA = newOrnA;
			}
			else
			{
				collidableIndexA = rigidBodies[bodyIndexA].m_collidableIdx;
			}

			if (childShapeIndexB >= 0)
			{
				collidableIndexB = gpuChildShapes[childShapeIndexB].m_shapeIndex;
				float4 childPosB = gpuChildShapes[childShapeIndexB].m_childPosition;
				b3Quat childOrnB = gpuChildShapes[childShapeIndexB].m_childOrientation;
				float4 newPosB = b3QuatRotate(ornB, childPosB) + posB;
				b3Quat newOrnB = b3QuatMul(ornB, childOrnB);
				posB = newPosB;
				ornB = newOrnB;
			}
			else
			{
				collidableIndexB = rigidBodies[bodyIndexB].m_collidableIdx;
			}

			int shapeIndexA = collidables[collidableIndexA].m_shapeIndex;
			int shapeIndexB = collidables[collidableIndexB].m_shapeIndex;

			int numLocalContactsOut = clipHullAgainstHull(gpuCompoundSepNormalsOut[i],
														  convexShapes[shapeIndexA], convexShapes[shapeIndexB],
														  posA, ornA,
														  posB, ornB,
														  worldVertsB1, worldVertsB2, capacityWorldVerts,
														  minDist, maxDist,
														  vertices, faces, indices,
														  vertices, faces, indices,
														  localContactsOut, localContactCapacity);

			if (numLocalContactsOut > 0)
			{
				float4 normal = -gpuCompoundSepNormalsOut[i];
				int nPoints = numLocalContactsOut;
				float4* pointsIn = localContactsOut;
				b3Int4 contactIdx;  // = {-1,-1,-1,-1};

				contactIdx.s[0] = 0;
				contactIdx.s[1] = 1;
				contactIdx.s[2] = 2;
				contactIdx.s[3] = 3;

				int nReducedContacts = extractManifoldSequentialGlobal(pointsIn, nPoints, normal, &contactIdx);

				int dstIdx;
				dstIdx = b3AtomicInc(nGlobalContactsOut);
				if ((dstIdx + nReducedContacts) < maxContactCapacity)
				{
					__global struct b3Contact4Data* c = globalContactsOut + dstIdx;
					c->m_worldNormalOnB = -normal;
					c->m_restituitionCoeffCmp = (0.f * 0xffff);
					c->m_frictionCoeffCmp = (0.7f * 0xffff);
					c->m_batchIdx = pairIndex;
					int bodyA = gpuCompoundPairs[pairIndex].x;
					int bodyB = gpuCompoundPairs[pairIndex].y;
					c->m_bodyAPtrAndSignBit = rigidBodies[bodyA].m_invMass == 0 ? -bodyA : bodyA;
					c->m_bodyBPtrAndSignBit = rigidBodies[bodyB].m_invMass == 0 ? -bodyB : bodyB;
					c->m_childIndexA = childShapeIndexA;
					c->m_childIndexB = childShapeIndexB;
					for (int i = 0; i < nReducedContacts; i++)
					{
						c->m_worldPosB[i] = pointsIn[contactIdx.s[i]];
					}
					b3Contact4Data_setNumPoints(c, nReducedContacts);
				}

			}  //		if (numContactsOut>0)
		}      //		if (gpuHasCompoundSepNormalsOut[i])
	}          //	if (i<numCompoundPairs)
}

void computeContactCompoundCompound(int pairIndex,
									int bodyIndexA, int bodyIndexB,
									int collidableIndexA, int collidableIndexB,
									const b3RigidBodyData* rigidBodies,
									const b3Collidable* collidables,
									const b3ConvexPolyhedronData* convexShapes,
									const b3GpuChildShape* cpuChildShapes,
									const b3AlignedObjectArray<b3Aabb>& hostAabbsWorldSpace,
									const b3AlignedObjectArray<b3Aabb>& hostAabbsLocalSpace,

									const b3AlignedObjectArray<b3Vector3>& convexVertices,
									const b3AlignedObjectArray<b3Vector3>& hostUniqueEdges,
									const b3AlignedObjectArray<int>& convexIndices,
									const b3AlignedObjectArray<b3GpuFace>& faces,

									b3Contact4* globalContactsOut,
									int& nGlobalContactsOut,
									int maxContactCapacity,
									b3AlignedObjectArray<b3QuantizedBvhNode>& treeNodesCPU,
									b3AlignedObjectArray<b3BvhSubtreeInfo>& subTreesCPU,
									b3AlignedObjectArray<b3BvhInfo>& bvhInfoCPU)
{
	int shapeTypeB = collidables[collidableIndexB].m_shapeType;
	b3Assert(shapeTypeB == SHAPE_COMPOUND_OF_CONVEX_HULLS);

	b3AlignedObjectArray<b3Int4> cpuCompoundPairsOut;
	int numCompoundPairsOut = 0;
	int maxNumCompoundPairsCapacity = 8192;  //1024;
	cpuCompoundPairsOut.resize(maxNumCompoundPairsCapacity);

	// work-in-progress
	findCompoundPairsKernel(
		pairIndex,
		bodyIndexA, bodyIndexB,
		collidableIndexA, collidableIndexB,
		rigidBodies,
		collidables,
		convexShapes,
		convexVertices,
		hostAabbsWorldSpace,
		hostAabbsLocalSpace,
		cpuChildShapes,
		&cpuCompoundPairsOut[0],
		&numCompoundPairsOut,
		maxNumCompoundPairsCapacity,
		treeNodesCPU,
		subTreesCPU,
		bvhInfoCPU);

	printf("maxNumAabbChecks=%d\n", maxNumAabbChecks);
	if (numCompoundPairsOut > maxNumCompoundPairsCapacity)
	{
		b3Error("numCompoundPairsOut exceeded maxNumCompoundPairsCapacity (%d)\n", maxNumCompoundPairsCapacity);
		numCompoundPairsOut = maxNumCompoundPairsCapacity;
	}
	b3AlignedObjectArray<b3Float4> cpuCompoundSepNormalsOut;
	b3AlignedObjectArray<int> cpuHasCompoundSepNormalsOut;
	cpuCompoundSepNormalsOut.resize(numCompoundPairsOut);
	cpuHasCompoundSepNormalsOut.resize(numCompoundPairsOut);

	for (int i = 0; i < numCompoundPairsOut; i++)
	{
		processCompoundPairsKernel(&cpuCompoundPairsOut[0], rigidBodies, collidables, convexShapes, convexVertices, hostUniqueEdges, faces, convexIndices, 0, cpuChildShapes,
								   cpuCompoundSepNormalsOut, cpuHasCompoundSepNormalsOut, numCompoundPairsOut, i);
	}

	for (int i = 0; i < numCompoundPairsOut; i++)
	{
		clipCompoundsHullHullKernel(&cpuCompoundPairsOut[0], rigidBodies, collidables, convexShapes, convexVertices, hostUniqueEdges, faces, convexIndices, cpuChildShapes,
									cpuCompoundSepNormalsOut, cpuHasCompoundSepNormalsOut, globalContactsOut, &nGlobalContactsOut, numCompoundPairsOut, maxContactCapacity, i);
	}
	/*
		int childColIndexA = gpuChildShapes[childShapeIndexA].m_shapeIndex;

					float4 posA = rigidBodies[bodyIndexA].m_pos;
					b3Quat ornA = rigidBodies[bodyIndexA].m_quat;
					float4 childPosA = gpuChildShapes[childShapeIndexA].m_childPosition;
					b3Quat childOrnA = gpuChildShapes[childShapeIndexA].m_childOrientation;
					float4 newPosA = b3QuatRotate(ornA,childPosA)+posA;
					b3Quat newOrnA = b3QuatMul(ornA,childOrnA);

					int shapeIndexA = collidables[childColIndexA].m_shapeIndex;


			bool foundSepAxis = findSeparatingAxis(hullA,hullB,
							posA,
							ornA,
							posB,
							ornB,

							convexVertices,uniqueEdges,faces,convexIndices,
							convexVertices,uniqueEdges,faces,convexIndices,
							
							sepNormalWorldSpace
							);
							*/

	/*
	if (foundSepAxis)
	{
		
		
		contactIndex = clipHullHullSingle(
			bodyIndexA, bodyIndexB,
						   posA,ornA,
						   posB,ornB,
			collidableIndexA, collidableIndexB,
			&rigidBodies, 
			&globalContactsOut,
			nGlobalContactsOut,
			
			convexShapes,
			convexShapes,
	
			convexVertices, 
			uniqueEdges, 
			faces,
			convexIndices,
	
			convexVertices,
			uniqueEdges,
			faces,
			convexIndices,

			collidables,
			collidables,
			sepNormalWorldSpace,
			maxContactCapacity);
			
	}
	*/

	//	return contactIndex;

	/*

	int numChildrenB = collidables[collidableIndexB].m_numChildShapes;
	for (int c=0;c<numChildrenB;c++)
	{
		int childShapeIndexB = collidables[collidableIndexB].m_shapeIndex+c;
		int childColIndexB = cpuChildShapes[childShapeIndexB].m_shapeIndex;

		float4 rootPosB = rigidBodies[bodyIndexB].m_pos;
		b3Quaternion rootOrnB = rigidBodies[bodyIndexB].m_quat;
		b3Vector3 childPosB = cpuChildShapes[childShapeIndexB].m_childPosition;
		b3Quaternion childOrnB = cpuChildShapes[childShapeIndexB].m_childOrientation;
		float4  posB = b3QuatRotate(rootOrnB,childPosB)+rootPosB;
		b3Quaternion ornB = b3QuatMul(rootOrnB,childOrnB);//b3QuatMul(ornB,childOrnB);

		int shapeIndexB = collidables[childColIndexB].m_shapeIndex;

		const b3ConvexPolyhedronData* hullB = &convexShapes[shapeIndexB];

	}
	*/
}

void computeContactPlaneCompound(int pairIndex,
								 int bodyIndexA, int bodyIndexB,
								 int collidableIndexA, int collidableIndexB,
								 const b3RigidBodyData* rigidBodies,
								 const b3Collidable* collidables,
								 const b3ConvexPolyhedronData* convexShapes,
								 const b3GpuChildShape* cpuChildShapes,
								 const b3Vector3* convexVertices,
								 const int* convexIndices,
								 const b3GpuFace* faces,

								 b3Contact4* globalContactsOut,
								 int& nGlobalContactsOut,
								 int maxContactCapacity)
{
	int shapeTypeB = collidables[collidableIndexB].m_shapeType;
	b3Assert(shapeTypeB == SHAPE_COMPOUND_OF_CONVEX_HULLS);

	int numChildrenB = collidables[collidableIndexB].m_numChildShapes;
	for (int c = 0; c < numChildrenB; c++)
	{
		int childShapeIndexB = collidables[collidableIndexB].m_shapeIndex + c;
		int childColIndexB = cpuChildShapes[childShapeIndexB].m_shapeIndex;

		float4 rootPosB = rigidBodies[bodyIndexB].m_pos;
		b3Quaternion rootOrnB = rigidBodies[bodyIndexB].m_quat;
		b3Vector3 childPosB = cpuChildShapes[childShapeIndexB].m_childPosition;
		b3Quaternion childOrnB = cpuChildShapes[childShapeIndexB].m_childOrientation;
		float4 posB = b3QuatRotate(rootOrnB, childPosB) + rootPosB;
		b3Quaternion ornB = rootOrnB * childOrnB;  //b3QuatMul(ornB,childOrnB);

		int shapeIndexB = collidables[childColIndexB].m_shapeIndex;

		const b3ConvexPolyhedronData* hullB = &convexShapes[shapeIndexB];

		b3Vector3 posA = rigidBodies[bodyIndexA].m_pos;
		b3Quaternion ornA = rigidBodies[bodyIndexA].m_quat;

		//	int numContactsOut = 0;
		//	int numWorldVertsB1= 0;

		b3Vector3 planeEq = faces[collidables[collidableIndexA].m_shapeIndex].m_plane;
		b3Vector3 planeNormal = b3MakeVector3(planeEq.x, planeEq.y, planeEq.z);
		b3Vector3 planeNormalWorld = b3QuatRotate(ornA, planeNormal);
		float planeConstant = planeEq.w;
		b3Transform convexWorldTransform;
		convexWorldTransform.setIdentity();
		convexWorldTransform.setOrigin(posB);
		convexWorldTransform.setRotation(ornB);
		b3Transform planeTransform;
		planeTransform.setIdentity();
		planeTransform.setOrigin(posA);
		planeTransform.setRotation(ornA);

		b3Transform planeInConvex;
		planeInConvex = convexWorldTransform.inverse() * planeTransform;
		b3Transform convexInPlane;
		convexInPlane = planeTransform.inverse() * convexWorldTransform;

		b3Vector3 planeNormalInConvex = planeInConvex.getBasis() * -planeNormal;
		float maxDot = -1e30;
		int hitVertex = -1;
		b3Vector3 hitVtx;

#define MAX_PLANE_CONVEX_POINTS 64

		b3Vector3 contactPoints[MAX_PLANE_CONVEX_POINTS];
		int numPoints = 0;

		b3Int4 contactIdx;
		contactIdx.s[0] = 0;
		contactIdx.s[1] = 1;
		contactIdx.s[2] = 2;
		contactIdx.s[3] = 3;

		for (int i = 0; i < hullB->m_numVertices; i++)
		{
			b3Vector3 vtx = convexVertices[hullB->m_vertexOffset + i];
			float curDot = vtx.dot(planeNormalInConvex);

			if (curDot > maxDot)
			{
				hitVertex = i;
				maxDot = curDot;
				hitVtx = vtx;
				//make sure the deepest points is always included
				if (numPoints == MAX_PLANE_CONVEX_POINTS)
					numPoints--;
			}

			if (numPoints < MAX_PLANE_CONVEX_POINTS)
			{
				b3Vector3 vtxWorld = convexWorldTransform * vtx;
				b3Vector3 vtxInPlane = planeTransform.inverse() * vtxWorld;
				float dist = planeNormal.dot(vtxInPlane) - planeConstant;
				if (dist < 0.f)
				{
					vtxWorld.w = dist;
					contactPoints[numPoints] = vtxWorld;
					numPoints++;
				}
			}
		}

		int numReducedPoints = 0;

		numReducedPoints = numPoints;

		if (numPoints > 4)
		{
			numReducedPoints = extractManifoldSequentialGlobal(contactPoints, numPoints, planeNormalInConvex, &contactIdx);
		}
		int dstIdx;
		//    dstIdx = nGlobalContactsOut++;//AppendInc( nGlobalContactsOut, dstIdx );

		if (numReducedPoints > 0)
		{
			if (nGlobalContactsOut < maxContactCapacity)
			{
				dstIdx = nGlobalContactsOut;
				nGlobalContactsOut++;

				b3Contact4* c = &globalContactsOut[dstIdx];
				c->m_worldNormalOnB = -planeNormalWorld;
				c->setFrictionCoeff(0.7);
				c->setRestituitionCoeff(0.f);

				c->m_batchIdx = pairIndex;
				c->m_bodyAPtrAndSignBit = rigidBodies[bodyIndexA].m_invMass == 0 ? -bodyIndexA : bodyIndexA;
				c->m_bodyBPtrAndSignBit = rigidBodies[bodyIndexB].m_invMass == 0 ? -bodyIndexB : bodyIndexB;
				for (int i = 0; i < numReducedPoints; i++)
				{
					b3Vector3 pOnB1 = contactPoints[contactIdx.s[i]];
					c->m_worldPosB[i] = pOnB1;
				}
				c->m_worldNormalOnB.w = (b3Scalar)numReducedPoints;
			}  //if (dstIdx < numPairs)
		}
	}
}

void computeContactSphereConvex(int pairIndex,
								int bodyIndexA, int bodyIndexB,
								int collidableIndexA, int collidableIndexB,
								const b3RigidBodyData* rigidBodies,
								const b3Collidable* collidables,
								const b3ConvexPolyhedronData* convexShapes,
								const b3Vector3* convexVertices,
								const int* convexIndices,
								const b3GpuFace* faces,
								b3Contact4* globalContactsOut,
								int& nGlobalContactsOut,
								int maxContactCapacity)
{
	float radius = collidables[collidableIndexA].m_radius;
	float4 spherePos1 = rigidBodies[bodyIndexA].m_pos;
	b3Quaternion sphereOrn = rigidBodies[bodyIndexA].m_quat;

	float4 pos = rigidBodies[bodyIndexB].m_pos;

	b3Quaternion quat = rigidBodies[bodyIndexB].m_quat;

	b3Transform tr;
	tr.setIdentity();
	tr.setOrigin(pos);
	tr.setRotation(quat);
	b3Transform trInv = tr.inverse();

	float4 spherePos = trInv(spherePos1);

	int collidableIndex = rigidBodies[bodyIndexB].m_collidableIdx;
	int shapeIndex = collidables[collidableIndex].m_shapeIndex;
	int numFaces = convexShapes[shapeIndex].m_numFaces;
	float4 closestPnt = b3MakeVector3(0, 0, 0, 0);
	//	float4 hitNormalWorld = b3MakeVector3(0, 0, 0, 0);
	float minDist = -1000000.f;  // TODO: What is the largest/smallest float?
	bool bCollide = true;
	int region = -1;
	float4 localHitNormal;
	for (int f = 0; f < numFaces; f++)
	{
		b3GpuFace face = faces[convexShapes[shapeIndex].m_faceOffset + f];
		float4 planeEqn;
		float4 localPlaneNormal = b3MakeVector3(face.m_plane.x, face.m_plane.y, face.m_plane.z, 0.f);
		float4 n1 = localPlaneNormal;  //quatRotate(quat,localPlaneNormal);
		planeEqn = n1;
		planeEqn[3] = face.m_plane.w;

		float4 pntReturn;
		float dist = signedDistanceFromPointToPlane(spherePos, planeEqn, &pntReturn);

		if (dist > radius)
		{
			bCollide = false;
			break;
		}

		if (dist > 0)
		{
			//might hit an edge or vertex
			b3Vector3 out;

			bool isInPoly = IsPointInPolygon(spherePos,
											 &face,
											 &convexVertices[convexShapes[shapeIndex].m_vertexOffset],
											 convexIndices,
											 &out);
			if (isInPoly)
			{
				if (dist > minDist)
				{
					minDist = dist;
					closestPnt = pntReturn;
					localHitNormal = planeEqn;
					region = 1;
				}
			}
			else
			{
				b3Vector3 tmp = spherePos - out;
				b3Scalar l2 = tmp.length2();
				if (l2 < radius * radius)
				{
					dist = b3Sqrt(l2);
					if (dist > minDist)
					{
						minDist = dist;
						closestPnt = out;
						localHitNormal = tmp / dist;
						region = 2;
					}
				}
				else
				{
					bCollide = false;
					break;
				}
			}
		}
		else
		{
			if (dist > minDist)
			{
				minDist = dist;
				closestPnt = pntReturn;
				localHitNormal = planeEqn;
				region = 3;
			}
		}
	}
	static int numChecks = 0;
	numChecks++;

	if (bCollide && minDist > -10000)
	{
		float4 normalOnSurfaceB1 = tr.getBasis() * localHitNormal;  //-hitNormalWorld;
		float4 pOnB1 = tr(closestPnt);
		//printf("dist ,%f,",minDist);
		float actualDepth = minDist - radius;
		if (actualDepth < 0)
		{
			//printf("actualDepth = ,%f,", actualDepth);
			//printf("normalOnSurfaceB1 = ,%f,%f,%f,", normalOnSurfaceB1.x,normalOnSurfaceB1.y,normalOnSurfaceB1.z);
			//printf("region=,%d,\n", region);
			pOnB1[3] = actualDepth;

			int dstIdx;
			//    dstIdx = nGlobalContactsOut++;//AppendInc( nGlobalContactsOut, dstIdx );

			if (nGlobalContactsOut < maxContactCapacity)
			{
				dstIdx = nGlobalContactsOut;
				nGlobalContactsOut++;

				b3Contact4* c = &globalContactsOut[dstIdx];
				c->m_worldNormalOnB = normalOnSurfaceB1;
				c->setFrictionCoeff(0.7);
				c->setRestituitionCoeff(0.f);

				c->m_batchIdx = pairIndex;
				c->m_bodyAPtrAndSignBit = rigidBodies[bodyIndexA].m_invMass == 0 ? -bodyIndexA : bodyIndexA;
				c->m_bodyBPtrAndSignBit = rigidBodies[bodyIndexB].m_invMass == 0 ? -bodyIndexB : bodyIndexB;
				c->m_worldPosB[0] = pOnB1;
				int numPoints = 1;
				c->m_worldNormalOnB.w = (b3Scalar)numPoints;
			}  //if (dstIdx < numPairs)
		}
	}  //if (hasCollision)
}

int computeContactConvexConvex2(
	int pairIndex,
	int bodyIndexA, int bodyIndexB,
	int collidableIndexA, int collidableIndexB,
	const b3AlignedObjectArray<b3RigidBodyData>& rigidBodies,
	const b3AlignedObjectArray<b3Collidable>& collidables,
	const b3AlignedObjectArray<b3ConvexPolyhedronData>& convexShapes,
	const b3AlignedObjectArray<b3Vector3>& convexVertices,
	const b3AlignedObjectArray<b3Vector3>& uniqueEdges,
	const b3AlignedObjectArray<int>& convexIndices,
	const b3AlignedObjectArray<b3GpuFace>& faces,
	b3AlignedObjectArray<b3Contact4>& globalContactsOut,
	int& nGlobalContactsOut,
	int maxContactCapacity,
	const b3AlignedObjectArray<b3Contact4>& oldContacts)
{
	int contactIndex = -1;
	b3Vector3 posA = rigidBodies[bodyIndexA].m_pos;
	b3Quaternion ornA = rigidBodies[bodyIndexA].m_quat;
	b3Vector3 posB = rigidBodies[bodyIndexB].m_pos;
	b3Quaternion ornB = rigidBodies[bodyIndexB].m_quat;

	b3ConvexPolyhedronData hullA, hullB;

	b3Vector3 sepNormalWorldSpace;

	b3Collidable colA = collidables[collidableIndexA];
	hullA = convexShapes[colA.m_shapeIndex];
	//printf("numvertsA = %d\n",hullA.m_numVertices);

	b3Collidable colB = collidables[collidableIndexB];
	hullB = convexShapes[colB.m_shapeIndex];
	//printf("numvertsB = %d\n",hullB.m_numVertices);

	//	int contactCapacity = MAX_VERTS;
	//int numContactsOut=0;

#ifdef _WIN32
	b3Assert(_finite(rigidBodies[bodyIndexA].m_pos.x));
	b3Assert(_finite(rigidBodies[bodyIndexB].m_pos.x));
#endif

	bool foundSepAxis = findSeparatingAxis(hullA, hullB,
										   posA,
										   ornA,
										   posB,
										   ornB,

										   convexVertices, uniqueEdges, faces, convexIndices,
										   convexVertices, uniqueEdges, faces, convexIndices,

										   sepNormalWorldSpace);

	if (foundSepAxis)
	{
		contactIndex = clipHullHullSingle(
			bodyIndexA, bodyIndexB,
			posA, ornA,
			posB, ornB,
			collidableIndexA, collidableIndexB,
			&rigidBodies,
			&globalContactsOut,
			nGlobalContactsOut,

			convexShapes,
			convexShapes,

			convexVertices,
			uniqueEdges,
			faces,
			convexIndices,

			convexVertices,
			uniqueEdges,
			faces,
			convexIndices,

			collidables,
			collidables,
			sepNormalWorldSpace,
			maxContactCapacity);
	}

	return contactIndex;
}

