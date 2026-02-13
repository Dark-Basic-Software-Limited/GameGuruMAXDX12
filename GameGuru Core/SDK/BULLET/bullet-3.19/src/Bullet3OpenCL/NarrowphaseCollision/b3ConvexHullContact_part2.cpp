void GpuSatCollision::computeConvexConvexContactsGPUSAT(b3OpenCLArray<b3Int4>* pairs, int nPairs,
														const b3OpenCLArray<b3RigidBodyData>* bodyBuf,
														b3OpenCLArray<b3Contact4>* contactOut, int& nContacts,
														const b3OpenCLArray<b3Contact4>* oldContacts,
														int maxContactCapacity,
														int compoundPairCapacity,
														const b3OpenCLArray<b3ConvexPolyhedronData>& convexData,
														const b3OpenCLArray<b3Vector3>& gpuVertices,
														const b3OpenCLArray<b3Vector3>& gpuUniqueEdges,
														const b3OpenCLArray<b3GpuFace>& gpuFaces,
														const b3OpenCLArray<int>& gpuIndices,
														const b3OpenCLArray<b3Collidable>& gpuCollidables,
														const b3OpenCLArray<b3GpuChildShape>& gpuChildShapes,

														const b3OpenCLArray<b3Aabb>& clAabbsWorldSpace,
														const b3OpenCLArray<b3Aabb>& clAabbsLocalSpace,

														b3OpenCLArray<b3Vector3>& worldVertsB1GPU,
														b3OpenCLArray<b3Int4>& clippingFacesOutGPU,
														b3OpenCLArray<b3Vector3>& worldNormalsAGPU,
														b3OpenCLArray<b3Vector3>& worldVertsA1GPU,
														b3OpenCLArray<b3Vector3>& worldVertsB2GPU,
														b3AlignedObjectArray<class b3OptimizedBvh*>& bvhDataUnused,
														b3OpenCLArray<b3QuantizedBvhNode>* treeNodesGPU,
														b3OpenCLArray<b3BvhSubtreeInfo>* subTreesGPU,
														b3OpenCLArray<b3BvhInfo>* bvhInfo,

														int numObjects,
														int maxTriConvexPairCapacity,
														b3OpenCLArray<b3Int4>& triangleConvexPairsOut,
														int& numTriConvexPairsOut)
{
	myframecount++;

	if (!nPairs)
		return;

#ifdef CHECK_ON_HOST

	b3AlignedObjectArray<b3QuantizedBvhNode> treeNodesCPU;
	treeNodesGPU->copyToHost(treeNodesCPU);

	b3AlignedObjectArray<b3BvhSubtreeInfo> subTreesCPU;
	subTreesGPU->copyToHost(subTreesCPU);

	b3AlignedObjectArray<b3BvhInfo> bvhInfoCPU;
	bvhInfo->copyToHost(bvhInfoCPU);

	b3AlignedObjectArray<b3Aabb> hostAabbsWorldSpace;
	clAabbsWorldSpace.copyToHost(hostAabbsWorldSpace);

	b3AlignedObjectArray<b3Aabb> hostAabbsLocalSpace;
	clAabbsLocalSpace.copyToHost(hostAabbsLocalSpace);

	b3AlignedObjectArray<b3Int4> hostPairs;
	pairs->copyToHost(hostPairs);

	b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
	bodyBuf->copyToHost(hostBodyBuf);

	b3AlignedObjectArray<b3ConvexPolyhedronData> hostConvexData;
	convexData.copyToHost(hostConvexData);

	b3AlignedObjectArray<b3Vector3> hostVertices;
	gpuVertices.copyToHost(hostVertices);

	b3AlignedObjectArray<b3Vector3> hostUniqueEdges;
	gpuUniqueEdges.copyToHost(hostUniqueEdges);
	b3AlignedObjectArray<b3GpuFace> hostFaces;
	gpuFaces.copyToHost(hostFaces);
	b3AlignedObjectArray<int> hostIndices;
	gpuIndices.copyToHost(hostIndices);
	b3AlignedObjectArray<b3Collidable> hostCollidables;
	gpuCollidables.copyToHost(hostCollidables);

	b3AlignedObjectArray<b3GpuChildShape> cpuChildShapes;
	gpuChildShapes.copyToHost(cpuChildShapes);

	b3AlignedObjectArray<b3Int4> hostTriangleConvexPairs;

	b3AlignedObjectArray<b3Contact4> hostContacts;
	if (nContacts)
	{
		contactOut->copyToHost(hostContacts);
	}

	b3AlignedObjectArray<b3Contact4> oldHostContacts;

	if (oldContacts->size())
	{
		oldContacts->copyToHost(oldHostContacts);
	}

	hostContacts.resize(maxContactCapacity);

	for (int i = 0; i < nPairs; i++)
	{
		int bodyIndexA = hostPairs[i].x;
		int bodyIndexB = hostPairs[i].y;
		int collidableIndexA = hostBodyBuf[bodyIndexA].m_collidableIdx;
		int collidableIndexB = hostBodyBuf[bodyIndexB].m_collidableIdx;

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_SPHERE &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_CONVEX_HULL)
		{
			computeContactSphereConvex(i, bodyIndexA, bodyIndexB, collidableIndexA, collidableIndexB, &hostBodyBuf[0],
									   &hostCollidables[0], &hostConvexData[0], &hostVertices[0], &hostIndices[0], &hostFaces[0], &hostContacts[0], nContacts, maxContactCapacity);
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_CONVEX_HULL &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_SPHERE)
		{
			computeContactSphereConvex(i, bodyIndexB, bodyIndexA, collidableIndexB, collidableIndexA, &hostBodyBuf[0],
									   &hostCollidables[0], &hostConvexData[0], &hostVertices[0], &hostIndices[0], &hostFaces[0], &hostContacts[0], nContacts, maxContactCapacity);
			//printf("convex-sphere\n");
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_CONVEX_HULL &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_PLANE)
		{
			computeContactPlaneConvex(i, bodyIndexB, bodyIndexA, collidableIndexB, collidableIndexA, &hostBodyBuf[0],
									  &hostCollidables[0], &hostConvexData[0], &hostVertices[0], &hostIndices[0], &hostFaces[0], &hostContacts[0], nContacts, maxContactCapacity);
			//			printf("convex-plane\n");
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_PLANE &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_CONVEX_HULL)
		{
			computeContactPlaneConvex(i, bodyIndexA, bodyIndexB, collidableIndexA, collidableIndexB, &hostBodyBuf[0],
									  &hostCollidables[0], &hostConvexData[0], &hostVertices[0], &hostIndices[0], &hostFaces[0], &hostContacts[0], nContacts, maxContactCapacity);
			//			printf("plane-convex\n");
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_COMPOUND_OF_CONVEX_HULLS &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_COMPOUND_OF_CONVEX_HULLS)
		{
			computeContactCompoundCompound(i, bodyIndexB, bodyIndexA, collidableIndexB, collidableIndexA, &hostBodyBuf[0],
										   &hostCollidables[0], &hostConvexData[0], &cpuChildShapes[0], hostAabbsWorldSpace, hostAabbsLocalSpace, hostVertices, hostUniqueEdges, hostIndices, hostFaces, &hostContacts[0],
										   nContacts, maxContactCapacity, treeNodesCPU, subTreesCPU, bvhInfoCPU);
			//			printf("convex-plane\n");
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_COMPOUND_OF_CONVEX_HULLS &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_PLANE)
		{
			computeContactPlaneCompound(i, bodyIndexB, bodyIndexA, collidableIndexB, collidableIndexA, &hostBodyBuf[0],
										&hostCollidables[0], &hostConvexData[0], &cpuChildShapes[0], &hostVertices[0], &hostIndices[0], &hostFaces[0], &hostContacts[0], nContacts, maxContactCapacity);
			//			printf("convex-plane\n");
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_PLANE &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_COMPOUND_OF_CONVEX_HULLS)
		{
			computeContactPlaneCompound(i, bodyIndexA, bodyIndexB, collidableIndexA, collidableIndexB, &hostBodyBuf[0],
										&hostCollidables[0], &hostConvexData[0], &cpuChildShapes[0], &hostVertices[0], &hostIndices[0], &hostFaces[0], &hostContacts[0], nContacts, maxContactCapacity);
			//			printf("plane-convex\n");
		}

		if (hostCollidables[collidableIndexA].m_shapeType == SHAPE_CONVEX_HULL &&
			hostCollidables[collidableIndexB].m_shapeType == SHAPE_CONVEX_HULL)
		{
			//printf("hostPairs[i].z=%d\n",hostPairs[i].z);
			int contactIndex = computeContactConvexConvex2(i, bodyIndexA, bodyIndexB, collidableIndexA, collidableIndexB, hostBodyBuf, hostCollidables, hostConvexData, hostVertices, hostUniqueEdges, hostIndices, hostFaces, hostContacts, nContacts, maxContactCapacity, oldHostContacts);
			//int contactIndex = computeContactConvexConvex(hostPairs,i,bodyIndexA,bodyIndexB,collidableIndexA,collidableIndexB,hostBodyBuf,hostCollidables,hostConvexData,hostVertices,hostUniqueEdges,hostIndices,hostFaces,hostContacts,nContacts,maxContactCapacity,oldHostContacts);

			if (contactIndex >= 0)
			{
				//				printf("convex convex contactIndex = %d\n",contactIndex);
				hostPairs[i].z = contactIndex;
			}
			//			printf("plane-convex\n");
		}
	}

	if (hostPairs.size())
	{
		pairs->copyFromHost(hostPairs);
	}

	hostContacts.resize(nContacts);
	if (nContacts)
	{
		contactOut->copyFromHost(hostContacts);
	}
	else
	{
		contactOut->resize(0);
	}

	m_totalContactsOut.copyFromHostPointer(&nContacts, 1, 0, true);
	//printf("(HOST) nContacts = %d\n",nContacts);

#else

	{
		if (nPairs)
		{
			m_totalContactsOut.copyFromHostPointer(&nContacts, 1, 0, true);

			B3_PROFILE("primitiveContactsKernel");
			b3BufferInfoCL bInfo[] = {
				b3BufferInfoCL(pairs->getBufferCL(), true),
				b3BufferInfoCL(bodyBuf->getBufferCL(), true),
				b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
				b3BufferInfoCL(convexData.getBufferCL(), true),
				b3BufferInfoCL(gpuVertices.getBufferCL(), true),
				b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
				b3BufferInfoCL(gpuFaces.getBufferCL(), true),
				b3BufferInfoCL(gpuIndices.getBufferCL(), true),
				b3BufferInfoCL(contactOut->getBufferCL()),
				b3BufferInfoCL(m_totalContactsOut.getBufferCL())};

			b3LauncherCL launcher(m_queue, m_primitiveContactsKernel, "m_primitiveContactsKernel");
			launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
			launcher.setConst(nPairs);
			launcher.setConst(maxContactCapacity);
			int num = nPairs;
			launcher.launch1D(num);
			clFinish(m_queue);

			nContacts = m_totalContactsOut.at(0);
			contactOut->resize(nContacts);
		}
	}

#endif  //CHECK_ON_HOST

	B3_PROFILE("computeConvexConvexContactsGPUSAT");
	// printf("nContacts = %d\n",nContacts);

	m_sepNormals.resize(nPairs);
	m_hasSeparatingNormals.resize(nPairs);

	int concaveCapacity = maxTriConvexPairCapacity;
	m_concaveSepNormals.resize(concaveCapacity);
	m_concaveHasSeparatingNormals.resize(concaveCapacity);
	m_numConcavePairsOut.resize(0);
	m_numConcavePairsOut.push_back(0);

	m_gpuCompoundPairs.resize(compoundPairCapacity);

	m_gpuCompoundSepNormals.resize(compoundPairCapacity);

	m_gpuHasCompoundSepNormals.resize(compoundPairCapacity);

	m_numCompoundPairsOut.resize(0);
	m_numCompoundPairsOut.push_back(0);

	int numCompoundPairs = 0;

	int numConcavePairs = 0;

	{
		clFinish(m_queue);
		if (findSeparatingAxisOnGpu)
		{
			m_dmins.resize(nPairs);
			if (splitSearchSepAxisConvex)
			{
				if (useMprGpu)
				{
					nContacts = m_totalContactsOut.at(0);
					{
						B3_PROFILE("mprPenetrationKernel");
						b3BufferInfoCL bInfo[] = {
							b3BufferInfoCL(pairs->getBufferCL(), true),
							b3BufferInfoCL(bodyBuf->getBufferCL(), true),
							b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
							b3BufferInfoCL(convexData.getBufferCL(), true),
							b3BufferInfoCL(gpuVertices.getBufferCL(), true),
							b3BufferInfoCL(m_sepNormals.getBufferCL()),
							b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
							b3BufferInfoCL(contactOut->getBufferCL()),
							b3BufferInfoCL(m_totalContactsOut.getBufferCL())};

						b3LauncherCL launcher(m_queue, m_mprPenetrationKernel, "mprPenetrationKernel");
						launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));

						launcher.setConst(maxContactCapacity);
						launcher.setConst(nPairs);

						int num = nPairs;
						launcher.launch1D(num);
						clFinish(m_queue);
						/*
						b3AlignedObjectArray<int>hostHasSepAxis;
						m_hasSeparatingNormals.copyToHost(hostHasSepAxis);
						b3AlignedObjectArray<b3Vector3>hostSepAxis;
						m_sepNormals.copyToHost(hostSepAxis);
						*/
						nContacts = m_totalContactsOut.at(0);
						contactOut->resize(nContacts);
						//	printf("nContacts (after mprPenetrationKernel) = %d\n",nContacts);
						if (nContacts > maxContactCapacity)
						{
							b3Error("Error: contacts exceeds capacity (%d/%d)\n", nContacts, maxContactCapacity);
							nContacts = maxContactCapacity;
						}
					}
				}

				if (1)
				{
					if (1)
					{
						{
							B3_PROFILE("findSeparatingAxisVertexFaceKernel");
							b3BufferInfoCL bInfo[] = {
								b3BufferInfoCL(pairs->getBufferCL(), true),
								b3BufferInfoCL(bodyBuf->getBufferCL(), true),
								b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
								b3BufferInfoCL(convexData.getBufferCL(), true),
								b3BufferInfoCL(gpuVertices.getBufferCL(), true),
								b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
								b3BufferInfoCL(gpuFaces.getBufferCL(), true),
								b3BufferInfoCL(gpuIndices.getBufferCL(), true),
								b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
								b3BufferInfoCL(m_sepNormals.getBufferCL()),
								b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
								b3BufferInfoCL(m_dmins.getBufferCL())};

							b3LauncherCL launcher(m_queue, m_findSeparatingAxisVertexFaceKernel, "findSeparatingAxisVertexFaceKernel");
							launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
							launcher.setConst(nPairs);

							int num = nPairs;
							launcher.launch1D(num);
							clFinish(m_queue);
						}

						int numDirections = sizeof(unitSphere162) / sizeof(b3Vector3);

						{
							B3_PROFILE("findSeparatingAxisEdgeEdgeKernel");
							b3BufferInfoCL bInfo[] = {
								b3BufferInfoCL(pairs->getBufferCL(), true),
								b3BufferInfoCL(bodyBuf->getBufferCL(), true),
								b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
								b3BufferInfoCL(convexData.getBufferCL(), true),
								b3BufferInfoCL(gpuVertices.getBufferCL(), true),
								b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
								b3BufferInfoCL(gpuFaces.getBufferCL(), true),
								b3BufferInfoCL(gpuIndices.getBufferCL(), true),
								b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
								b3BufferInfoCL(m_sepNormals.getBufferCL()),
								b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
								b3BufferInfoCL(m_dmins.getBufferCL()),
								b3BufferInfoCL(m_unitSphereDirections.getBufferCL(), true)

							};

							b3LauncherCL launcher(m_queue, m_findSeparatingAxisEdgeEdgeKernel, "findSeparatingAxisEdgeEdgeKernel");
							launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
							launcher.setConst(numDirections);
							launcher.setConst(nPairs);
							int num = nPairs;
							launcher.launch1D(num);
							clFinish(m_queue);
						}
					}
					if (useMprGpu)
					{
						B3_PROFILE("findSeparatingAxisUnitSphereKernel");
						b3BufferInfoCL bInfo[] = {
							b3BufferInfoCL(pairs->getBufferCL(), true),
							b3BufferInfoCL(bodyBuf->getBufferCL(), true),
							b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
							b3BufferInfoCL(convexData.getBufferCL(), true),
							b3BufferInfoCL(gpuVertices.getBufferCL(), true),
							b3BufferInfoCL(m_unitSphereDirections.getBufferCL(), true),
							b3BufferInfoCL(m_sepNormals.getBufferCL()),
							b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
							b3BufferInfoCL(m_dmins.getBufferCL())};

						b3LauncherCL launcher(m_queue, m_findSeparatingAxisUnitSphereKernel, "findSeparatingAxisUnitSphereKernel");
						launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
						int numDirections = sizeof(unitSphere162) / sizeof(b3Vector3);
						launcher.setConst(numDirections);

						launcher.setConst(nPairs);

						int num = nPairs;
						launcher.launch1D(num);
						clFinish(m_queue);
					}
				}
			}
			else
			{
				B3_PROFILE("findSeparatingAxisKernel");
				b3BufferInfoCL bInfo[] = {
					b3BufferInfoCL(pairs->getBufferCL(), true),
					b3BufferInfoCL(bodyBuf->getBufferCL(), true),
					b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
					b3BufferInfoCL(convexData.getBufferCL(), true),
					b3BufferInfoCL(gpuVertices.getBufferCL(), true),
					b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
					b3BufferInfoCL(gpuFaces.getBufferCL(), true),
					b3BufferInfoCL(gpuIndices.getBufferCL(), true),
					b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
					b3BufferInfoCL(m_sepNormals.getBufferCL()),
					b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL())};

				b3LauncherCL launcher(m_queue, m_findSeparatingAxisKernel, "m_findSeparatingAxisKernel");
				launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
				launcher.setConst(nPairs);

				int num = nPairs;
				launcher.launch1D(num);
				clFinish(m_queue);
			}
		}
		else
		{
			B3_PROFILE("findSeparatingAxisKernel CPU");

			b3AlignedObjectArray<b3Int4> hostPairs;
			pairs->copyToHost(hostPairs);
			b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
			bodyBuf->copyToHost(hostBodyBuf);

			b3AlignedObjectArray<b3Collidable> hostCollidables;
			gpuCollidables.copyToHost(hostCollidables);

			b3AlignedObjectArray<b3GpuChildShape> cpuChildShapes;
			gpuChildShapes.copyToHost(cpuChildShapes);

			b3AlignedObjectArray<b3ConvexPolyhedronData> hostConvexShapeData;
			convexData.copyToHost(hostConvexShapeData);

			b3AlignedObjectArray<b3Vector3> hostVertices;
			gpuVertices.copyToHost(hostVertices);

			b3AlignedObjectArray<int> hostHasSepAxis;
			hostHasSepAxis.resize(nPairs);
			b3AlignedObjectArray<b3Vector3> hostSepAxis;
			hostSepAxis.resize(nPairs);

			b3AlignedObjectArray<b3Vector3> hostUniqueEdges;
			gpuUniqueEdges.copyToHost(hostUniqueEdges);
			b3AlignedObjectArray<b3GpuFace> hostFaces;
			gpuFaces.copyToHost(hostFaces);

			b3AlignedObjectArray<int> hostIndices;
			gpuIndices.copyToHost(hostIndices);

			b3AlignedObjectArray<b3Contact4> hostContacts;
			if (nContacts)
			{
				contactOut->copyToHost(hostContacts);
			}
			hostContacts.resize(maxContactCapacity);
			int nGlobalContactsOut = nContacts;

			for (int i = 0; i < nPairs; i++)
			{
				int bodyIndexA = hostPairs[i].x;
				int bodyIndexB = hostPairs[i].y;
				int collidableIndexA = hostBodyBuf[bodyIndexA].m_collidableIdx;
				int collidableIndexB = hostBodyBuf[bodyIndexB].m_collidableIdx;

				int shapeIndexA = hostCollidables[collidableIndexA].m_shapeIndex;
				int shapeIndexB = hostCollidables[collidableIndexB].m_shapeIndex;

				hostHasSepAxis[i] = 0;

				//once the broadphase avoids static-static pairs, we can remove this test
				if ((hostBodyBuf[bodyIndexA].m_invMass == 0) && (hostBodyBuf[bodyIndexB].m_invMass == 0))
				{
					continue;
				}

				if ((hostCollidables[collidableIndexA].m_shapeType != SHAPE_CONVEX_HULL) || (hostCollidables[collidableIndexB].m_shapeType != SHAPE_CONVEX_HULL))
				{
					continue;
				}

				float dmin = FLT_MAX;

				b3ConvexPolyhedronData* convexShapeA = &hostConvexShapeData[shapeIndexA];
				b3ConvexPolyhedronData* convexShapeB = &hostConvexShapeData[shapeIndexB];
				b3Vector3 posA = hostBodyBuf[bodyIndexA].m_pos;
				b3Vector3 posB = hostBodyBuf[bodyIndexB].m_pos;
				b3Quaternion ornA = hostBodyBuf[bodyIndexA].m_quat;
				b3Quaternion ornB = hostBodyBuf[bodyIndexB].m_quat;

				if (useGjk)
				{
					//first approximate the separating axis, to 'fail-proof' GJK+EPA or MPR
					{
						b3Vector3 c0local = hostConvexShapeData[shapeIndexA].m_localCenter;
						b3Vector3 c0 = b3TransformPoint(c0local, posA, ornA);
						b3Vector3 c1local = hostConvexShapeData[shapeIndexB].m_localCenter;
						b3Vector3 c1 = b3TransformPoint(c1local, posB, ornB);
						b3Vector3 DeltaC2 = c0 - c1;

						b3Vector3 sepAxis;

						bool hasSepAxisA = b3FindSeparatingAxis(convexShapeA, convexShapeB, posA, ornA, posB, ornB, DeltaC2,
																&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																&sepAxis, &dmin);

						if (hasSepAxisA)
						{
							bool hasSepAxisB = b3FindSeparatingAxis(convexShapeB, convexShapeA, posB, ornB, posA, ornA, DeltaC2,
																	&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																	&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																	&sepAxis, &dmin);
							if (hasSepAxisB)
							{
								bool hasEdgeEdge = b3FindSeparatingAxisEdgeEdge(convexShapeA, convexShapeB, posA, ornA, posB, ornB, DeltaC2,
																				&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																				&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																				&sepAxis, &dmin, false);

								if (hasEdgeEdge)
								{
									hostHasSepAxis[i] = 1;
									hostSepAxis[i] = sepAxis;
									hostSepAxis[i].w = dmin;
								}
							}
						}
					}

					if (hostHasSepAxis[i])
					{
						int pairIndex = i;

						bool useMpr = true;
						if (useMpr)
						{
							int res = 0;
							float depth = 0.f;
							b3Vector3 sepAxis2 = b3MakeVector3(1, 0, 0);
							b3Vector3 resultPointOnBWorld = b3MakeVector3(0, 0, 0);

							float depthOut;
							b3Vector3 dirOut;
							b3Vector3 posOut;

							//res = b3MprPenetration(bodyIndexA,bodyIndexB,hostBodyBuf,hostConvexShapeData,hostCollidables,hostVertices,&mprConfig,&depthOut,&dirOut,&posOut);
							res = b3MprPenetration(pairIndex, bodyIndexA, bodyIndexB, &hostBodyBuf[0], &hostConvexShapeData[0], &hostCollidables[0], &hostVertices[0], &hostSepAxis[0], &hostHasSepAxis[0], &depthOut, &dirOut, &posOut);
							depth = depthOut;
							sepAxis2 = b3MakeVector3(-dirOut.x, -dirOut.y, -dirOut.z);
							resultPointOnBWorld = posOut;
							//hostHasSepAxis[i] = 0;

							if (res == 0)
							{
								//add point?
								//printf("depth = %f\n",depth);
								//printf("normal = %f,%f,%f\n",dir.v[0],dir.v[1],dir.v[2]);
								//qprintf("pos = %f,%f,%f\n",pos.v[0],pos.v[1],pos.v[2]);

								float dist = 0.f;

								const b3ConvexPolyhedronData& hullA = hostConvexShapeData[hostCollidables[hostBodyBuf[bodyIndexA].m_collidableIdx].m_shapeIndex];
								const b3ConvexPolyhedronData& hullB = hostConvexShapeData[hostCollidables[hostBodyBuf[bodyIndexB].m_collidableIdx].m_shapeIndex];

								if (b3TestSepAxis(&hullA, &hullB, posA, ornA, posB, ornB, &sepAxis2, &hostVertices[0], &hostVertices[0], &dist))
								{
									if (depth > dist)
									{
										float diff = depth - dist;

										static float maxdiff = 0.f;
										if (maxdiff < diff)
										{
											maxdiff = diff;
											printf("maxdiff = %20.10f\n", maxdiff);
										}
									}
								}
								if (depth > dmin)
								{
									b3Vector3 oldAxis = hostSepAxis[i];
									depth = dmin;
									sepAxis2 = oldAxis;
								}

								if (b3TestSepAxis(&hullA, &hullB, posA, ornA, posB, ornB, &sepAxis2, &hostVertices[0], &hostVertices[0], &dist))
								{
									if (depth > dist)
									{
										float diff = depth - dist;
										//printf("?diff  = %f\n",diff );
										static float maxdiff = 0.f;
										if (maxdiff < diff)
										{
											maxdiff = diff;
											printf("maxdiff = %20.10f\n", maxdiff);
										}
									}
									//this is used for SAT
									//hostHasSepAxis[i] = 1;
									//hostSepAxis[i] = sepAxis2;

									//add contact point

									//int contactIndex = nGlobalContactsOut;
									b3Contact4& newContact = hostContacts.at(nGlobalContactsOut);
									nGlobalContactsOut++;
									newContact.m_batchIdx = 0;  //i;
									newContact.m_bodyAPtrAndSignBit = (hostBodyBuf.at(bodyIndexA).m_invMass == 0) ? -bodyIndexA : bodyIndexA;
									newContact.m_bodyBPtrAndSignBit = (hostBodyBuf.at(bodyIndexB).m_invMass == 0) ? -bodyIndexB : bodyIndexB;

									newContact.m_frictionCoeffCmp = 45874;
									newContact.m_restituitionCoeffCmp = 0;

									static float maxDepth = 0.f;

									if (depth > maxDepth)
									{
										maxDepth = depth;
										printf("MPR maxdepth = %f\n", maxDepth);
									}

									resultPointOnBWorld.w = -depth;
									newContact.m_worldPosB[0] = resultPointOnBWorld;
									//b3Vector3 resultPointOnAWorld = resultPointOnBWorld+depth*sepAxis2;
									newContact.m_worldNormalOnB = sepAxis2;
									newContact.m_worldNormalOnB.w = (b3Scalar)1;
								}
								else
								{
									printf("rejected\n");
								}
							}
						}
						else
						{
							//int contactIndex = computeContactConvexConvex2(           i,bodyIndexA,bodyIndexB,collidableIndexA,collidableIndexB,hostBodyBuf, hostCollidables,hostConvexData,hostVertices,hostUniqueEdges,hostIndices,hostFaces,hostContacts,nContacts,maxContactCapacity,oldHostContacts);
							b3AlignedObjectArray<b3Contact4> oldHostContacts;
							int result;
							result = computeContactConvexConvex2(  //hostPairs,
								pairIndex,
								bodyIndexA, bodyIndexB,
								collidableIndexA, collidableIndexB,
								hostBodyBuf,
								hostCollidables,
								hostConvexShapeData,
								hostVertices,
								hostUniqueEdges,
								hostIndices,
								hostFaces,
								hostContacts,
								nGlobalContactsOut,
								maxContactCapacity,
								oldHostContacts
								//hostHasSepAxis,
								//hostSepAxis

							);
						}  //mpr
					}      //hostHasSepAxis[i] = 1;
				}
				else
				{
					b3Vector3 c0local = hostConvexShapeData[shapeIndexA].m_localCenter;
					b3Vector3 c0 = b3TransformPoint(c0local, posA, ornA);
					b3Vector3 c1local = hostConvexShapeData[shapeIndexB].m_localCenter;
					b3Vector3 c1 = b3TransformPoint(c1local, posB, ornB);
					b3Vector3 DeltaC2 = c0 - c1;

					b3Vector3 sepAxis;

					bool hasSepAxisA = b3FindSeparatingAxis(convexShapeA, convexShapeB, posA, ornA, posB, ornB, DeltaC2,
															&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
															&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
															&sepAxis, &dmin);

					if (hasSepAxisA)
					{
						bool hasSepAxisB = b3FindSeparatingAxis(convexShapeB, convexShapeA, posB, ornB, posA, ornA, DeltaC2,
																&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																&sepAxis, &dmin);
						if (hasSepAxisB)
						{
							bool hasEdgeEdge = b3FindSeparatingAxisEdgeEdge(convexShapeA, convexShapeB, posA, ornA, posB, ornB, DeltaC2,
																			&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																			&hostVertices.at(0), &hostUniqueEdges.at(0), &hostFaces.at(0), &hostIndices.at(0),
																			&sepAxis, &dmin, true);

							if (hasEdgeEdge)
							{
								hostHasSepAxis[i] = 1;
								hostSepAxis[i] = sepAxis;
							}
						}
					}
				}
			}

			if (useGjkContacts)  //nGlobalContactsOut>0)
			{
				//printf("nGlobalContactsOut=%d\n",nGlobalContactsOut);
				nContacts = nGlobalContactsOut;
				contactOut->copyFromHost(hostContacts);

				m_totalContactsOut.copyFromHostPointer(&nContacts, 1, 0, true);
			}

			m_hasSeparatingNormals.copyFromHost(hostHasSepAxis);
			m_sepNormals.copyFromHost(hostSepAxis);

			/*
             //double-check results from GPU (comment-out the 'else' so both paths are executed
            b3AlignedObjectArray<int> checkHasSepAxis;
            m_hasSeparatingNormals.copyToHost(checkHasSepAxis);
            static int frameCount = 0;
            frameCount++;
            for (int i=0;i<nPairs;i++)
            {
                if (hostHasSepAxis[i] != checkHasSepAxis[i])
                {
                    printf("at frameCount %d hostHasSepAxis[%d] = %d but checkHasSepAxis[i] = %d\n",
                           frameCount,i,hostHasSepAxis[i],checkHasSepAxis[i]);
                }
            }
            //m_hasSeparatingNormals.copyFromHost(hostHasSepAxis);
            //    m_sepNormals.copyFromHost(hostSepAxis);
            */
		}

		numCompoundPairs = m_numCompoundPairsOut.at(0);
		bool useGpuFindCompoundPairs = true;
		if (useGpuFindCompoundPairs)
		{
			B3_PROFILE("findCompoundPairsKernel");
			b3BufferInfoCL bInfo[] =
				{
					b3BufferInfoCL(pairs->getBufferCL(), true),
					b3BufferInfoCL(bodyBuf->getBufferCL(), true),
					b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
					b3BufferInfoCL(convexData.getBufferCL(), true),
					b3BufferInfoCL(gpuVertices.getBufferCL(), true),
					b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
					b3BufferInfoCL(gpuFaces.getBufferCL(), true),
					b3BufferInfoCL(gpuIndices.getBufferCL(), true),
					b3BufferInfoCL(clAabbsLocalSpace.getBufferCL(), true),
					b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
					b3BufferInfoCL(m_gpuCompoundPairs.getBufferCL()),
					b3BufferInfoCL(m_numCompoundPairsOut.getBufferCL()),
					b3BufferInfoCL(subTreesGPU->getBufferCL()),
					b3BufferInfoCL(treeNodesGPU->getBufferCL()),
					b3BufferInfoCL(bvhInfo->getBufferCL())};

			b3LauncherCL launcher(m_queue, m_findCompoundPairsKernel, "m_findCompoundPairsKernel");
			launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
			launcher.setConst(nPairs);
			launcher.setConst(compoundPairCapacity);

			int num = nPairs;
			launcher.launch1D(num);
			clFinish(m_queue);

			numCompoundPairs = m_numCompoundPairsOut.at(0);
			//printf("numCompoundPairs =%d\n",numCompoundPairs );
			if (numCompoundPairs)
			{
				//printf("numCompoundPairs=%d\n",numCompoundPairs);
			}
		}
		else
		{
			b3AlignedObjectArray<b3QuantizedBvhNode> treeNodesCPU;
			treeNodesGPU->copyToHost(treeNodesCPU);

			b3AlignedObjectArray<b3BvhSubtreeInfo> subTreesCPU;
			subTreesGPU->copyToHost(subTreesCPU);

			b3AlignedObjectArray<b3BvhInfo> bvhInfoCPU;
			bvhInfo->copyToHost(bvhInfoCPU);

			b3AlignedObjectArray<b3Aabb> hostAabbsWorldSpace;
			clAabbsWorldSpace.copyToHost(hostAabbsWorldSpace);

			b3AlignedObjectArray<b3Aabb> hostAabbsLocalSpace;
			clAabbsLocalSpace.copyToHost(hostAabbsLocalSpace);

			b3AlignedObjectArray<b3Int4> hostPairs;
			pairs->copyToHost(hostPairs);

			b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
			bodyBuf->copyToHost(hostBodyBuf);

			b3AlignedObjectArray<b3Int4> cpuCompoundPairsOut;
			cpuCompoundPairsOut.resize(compoundPairCapacity);

			b3AlignedObjectArray<b3Collidable> hostCollidables;
			gpuCollidables.copyToHost(hostCollidables);

			b3AlignedObjectArray<b3GpuChildShape> cpuChildShapes;
			gpuChildShapes.copyToHost(cpuChildShapes);

			b3AlignedObjectArray<b3ConvexPolyhedronData> hostConvexData;
			convexData.copyToHost(hostConvexData);

			b3AlignedObjectArray<b3Vector3> hostVertices;
			gpuVertices.copyToHost(hostVertices);

			for (int pairIndex = 0; pairIndex < nPairs; pairIndex++)
			{
				int bodyIndexA = hostPairs[pairIndex].x;
				int bodyIndexB = hostPairs[pairIndex].y;
				int collidableIndexA = hostBodyBuf[bodyIndexA].m_collidableIdx;
				int collidableIndexB = hostBodyBuf[bodyIndexB].m_collidableIdx;
				if (cpuChildShapes.size())
				{
					findCompoundPairsKernel(
						pairIndex,
						bodyIndexA,
						bodyIndexB,
						collidableIndexA,
						collidableIndexB,
						&hostBodyBuf[0],
						&hostCollidables[0],
						&hostConvexData[0],
						hostVertices,
						hostAabbsWorldSpace,
						hostAabbsLocalSpace,
						&cpuChildShapes[0],
						&cpuCompoundPairsOut[0],
						&numCompoundPairs,
						compoundPairCapacity,
						treeNodesCPU,
						subTreesCPU,
						bvhInfoCPU);
				}
			}

			m_numCompoundPairsOut.copyFromHostPointer(&numCompoundPairs, 1, 0, true);
			if (numCompoundPairs)
			{
				b3CompoundOverlappingPair* ptr = (b3CompoundOverlappingPair*)&cpuCompoundPairsOut[0];
				m_gpuCompoundPairs.copyFromHostPointer(ptr, numCompoundPairs, 0, true);
			}
			//cpuCompoundPairsOut
		}
		if (numCompoundPairs)
		{
			printf("numCompoundPairs=%d\n", numCompoundPairs);
		}

		if (numCompoundPairs > compoundPairCapacity)
		{
			b3Error("Exceeded compound pair capacity (%d/%d)\n", numCompoundPairs, compoundPairCapacity);
			numCompoundPairs = compoundPairCapacity;
		}

		m_gpuCompoundPairs.resize(numCompoundPairs);
		m_gpuHasCompoundSepNormals.resize(numCompoundPairs);
		m_gpuCompoundSepNormals.resize(numCompoundPairs);

		if (numCompoundPairs)
		{
			B3_PROFILE("processCompoundPairsPrimitivesKernel");
			b3BufferInfoCL bInfo[] =
				{
					b3BufferInfoCL(m_gpuCompoundPairs.getBufferCL(), true),
					b3BufferInfoCL(bodyBuf->getBufferCL(), true),
					b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
					b3BufferInfoCL(convexData.getBufferCL(), true),
					b3BufferInfoCL(gpuVertices.getBufferCL(), true),
					b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
					b3BufferInfoCL(gpuFaces.getBufferCL(), true),
					b3BufferInfoCL(gpuIndices.getBufferCL(), true),
					b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
					b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
					b3BufferInfoCL(contactOut->getBufferCL()),
					b3BufferInfoCL(m_totalContactsOut.getBufferCL())};

			b3LauncherCL launcher(m_queue, m_processCompoundPairsPrimitivesKernel, "m_processCompoundPairsPrimitivesKernel");
			launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
			launcher.setConst(numCompoundPairs);
			launcher.setConst(maxContactCapacity);

			int num = numCompoundPairs;
			launcher.launch1D(num);
			clFinish(m_queue);
			nContacts = m_totalContactsOut.at(0);
			//printf("nContacts (after processCompoundPairsPrimitivesKernel) = %d\n",nContacts);
			if (nContacts > maxContactCapacity)
			{
				b3Error("Error: contacts exceeds capacity (%d/%d)\n", nContacts, maxContactCapacity);
				nContacts = maxContactCapacity;
			}
		}

		if (numCompoundPairs)
		{
			B3_PROFILE("processCompoundPairsKernel");
			b3BufferInfoCL bInfo[] =
				{
					b3BufferInfoCL(m_gpuCompoundPairs.getBufferCL(), true),
					b3BufferInfoCL(bodyBuf->getBufferCL(), true),
					b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
					b3BufferInfoCL(convexData.getBufferCL(), true),
					b3BufferInfoCL(gpuVertices.getBufferCL(), true),
					b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
					b3BufferInfoCL(gpuFaces.getBufferCL(), true),
					b3BufferInfoCL(gpuIndices.getBufferCL(), true),
					b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
					b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
					b3BufferInfoCL(m_gpuCompoundSepNormals.getBufferCL()),
					b3BufferInfoCL(m_gpuHasCompoundSepNormals.getBufferCL())};

			b3LauncherCL launcher(m_queue, m_processCompoundPairsKernel, "m_processCompoundPairsKernel");
			launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
			launcher.setConst(numCompoundPairs);

			int num = numCompoundPairs;
			launcher.launch1D(num);
			clFinish(m_queue);
		}

		//printf("numConcave  = %d\n",numConcave);

		//		printf("hostNormals.size()=%d\n",hostNormals.size());
		//int numPairs = pairCount.at(0);
	}
	int vertexFaceCapacity = 64;

	{
		//now perform the tree query on GPU

		if (treeNodesGPU->size() && treeNodesGPU->size())
		{
			if (bvhTraversalKernelGPU)
			{
				B3_PROFILE("m_bvhTraversalKernel");

				numConcavePairs = m_numConcavePairsOut.at(0);

				b3LauncherCL launcher(m_queue, m_bvhTraversalKernel, "m_bvhTraversalKernel");
				launcher.setBuffer(pairs->getBufferCL());
				launcher.setBuffer(bodyBuf->getBufferCL());
				launcher.setBuffer(gpuCollidables.getBufferCL());
				launcher.setBuffer(clAabbsWorldSpace.getBufferCL());
				launcher.setBuffer(triangleConvexPairsOut.getBufferCL());
				launcher.setBuffer(m_numConcavePairsOut.getBufferCL());
				launcher.setBuffer(subTreesGPU->getBufferCL());
				launcher.setBuffer(treeNodesGPU->getBufferCL());
				launcher.setBuffer(bvhInfo->getBufferCL());

				launcher.setConst(nPairs);
				launcher.setConst(maxTriConvexPairCapacity);
				int num = nPairs;
				launcher.launch1D(num);
				clFinish(m_queue);
				numConcavePairs = m_numConcavePairsOut.at(0);
			}
			else
			{
				b3AlignedObjectArray<b3Int4> hostPairs;
				pairs->copyToHost(hostPairs);
				b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
				bodyBuf->copyToHost(hostBodyBuf);
				b3AlignedObjectArray<b3Collidable> hostCollidables;
				gpuCollidables.copyToHost(hostCollidables);
				b3AlignedObjectArray<b3Aabb> hostAabbsWorldSpace;
				clAabbsWorldSpace.copyToHost(hostAabbsWorldSpace);

				//int maxTriConvexPairCapacity,
				b3AlignedObjectArray<b3Int4> triangleConvexPairsOutHost;
				triangleConvexPairsOutHost.resize(maxTriConvexPairCapacity);

				//int numTriConvexPairsOutHost=0;
				numConcavePairs = 0;
				//m_numConcavePairsOut

				b3AlignedObjectArray<b3QuantizedBvhNode> treeNodesCPU;
				treeNodesGPU->copyToHost(treeNodesCPU);
				b3AlignedObjectArray<b3BvhSubtreeInfo> subTreesCPU;
				subTreesGPU->copyToHost(subTreesCPU);
				b3AlignedObjectArray<b3BvhInfo> bvhInfoCPU;
				bvhInfo->copyToHost(bvhInfoCPU);
				//compute it...

				volatile int hostNumConcavePairsOut = 0;

				//
				for (int i = 0; i < nPairs; i++)
				{
					b3BvhTraversal(&hostPairs.at(0),
								   &hostBodyBuf.at(0),
								   &hostCollidables.at(0),
								   &hostAabbsWorldSpace.at(0),
								   &triangleConvexPairsOutHost.at(0),
								   &hostNumConcavePairsOut,
								   &subTreesCPU.at(0),
								   &treeNodesCPU.at(0),
								   &bvhInfoCPU.at(0),
								   nPairs,
								   maxTriConvexPairCapacity,
								   i);
				}
				numConcavePairs = hostNumConcavePairsOut;

				if (hostNumConcavePairsOut)
				{
					triangleConvexPairsOutHost.resize(hostNumConcavePairsOut);
					triangleConvexPairsOut.copyFromHost(triangleConvexPairsOutHost);
				}
				//

				m_numConcavePairsOut.resize(0);
				m_numConcavePairsOut.push_back(numConcavePairs);
			}

			//printf("numConcavePairs=%d (max = %d\n",numConcavePairs,maxTriConvexPairCapacity);

			if (numConcavePairs > maxTriConvexPairCapacity)
			{
				static int exceeded_maxTriConvexPairCapacity_count = 0;
				b3Error("Exceeded the maxTriConvexPairCapacity (found %d but max is %d, it happened %d times)\n",
						numConcavePairs, maxTriConvexPairCapacity, exceeded_maxTriConvexPairCapacity_count++);
				numConcavePairs = maxTriConvexPairCapacity;
			}
			triangleConvexPairsOut.resize(numConcavePairs);

			if (numConcavePairs)
			{
				clippingFacesOutGPU.resize(numConcavePairs);
				worldNormalsAGPU.resize(numConcavePairs);
				worldVertsA1GPU.resize(vertexFaceCapacity * (numConcavePairs));
				worldVertsB1GPU.resize(vertexFaceCapacity * (numConcavePairs));

				if (findConcaveSeparatingAxisKernelGPU)
				{
					/*
					m_concaveHasSeparatingNormals.copyFromHost(concaveHasSeparatingNormalsCPU);
						clippingFacesOutGPU.copyFromHost(clippingFacesOutCPU);
						worldVertsA1GPU.copyFromHost(worldVertsA1CPU);
						worldNormalsAGPU.copyFromHost(worldNormalsACPU);
						worldVertsB1GPU.copyFromHost(worldVertsB1CPU);
					*/

					//now perform a SAT test for each triangle-convex element (stored in triangleConvexPairsOut)
					if (splitSearchSepAxisConcave)
					{
						//printf("numConcavePairs = %d\n",numConcavePairs);
						m_dmins.resize(numConcavePairs);
						{
							B3_PROFILE("findConcaveSeparatingAxisVertexFaceKernel");
							b3BufferInfoCL bInfo[] = {
								b3BufferInfoCL(triangleConvexPairsOut.getBufferCL()),
								b3BufferInfoCL(bodyBuf->getBufferCL(), true),
								b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
								b3BufferInfoCL(convexData.getBufferCL(), true),
								b3BufferInfoCL(gpuVertices.getBufferCL(), true),
								b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
								b3BufferInfoCL(gpuFaces.getBufferCL(), true),
								b3BufferInfoCL(gpuIndices.getBufferCL(), true),
								b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
								b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
								b3BufferInfoCL(m_concaveSepNormals.getBufferCL()),
								b3BufferInfoCL(m_concaveHasSeparatingNormals.getBufferCL()),
								b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
								b3BufferInfoCL(worldVertsA1GPU.getBufferCL()),
								b3BufferInfoCL(worldNormalsAGPU.getBufferCL()),
								b3BufferInfoCL(worldVertsB1GPU.getBufferCL()),
								b3BufferInfoCL(m_dmins.getBufferCL())};

							b3LauncherCL launcher(m_queue, m_findConcaveSeparatingAxisVertexFaceKernel, "m_findConcaveSeparatingAxisVertexFaceKernel");
							launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
							launcher.setConst(vertexFaceCapacity);
							launcher.setConst(numConcavePairs);

							int num = numConcavePairs;
							launcher.launch1D(num);
							clFinish(m_queue);
						}
						//                        numConcavePairs = 0;
						if (1)
						{
							B3_PROFILE("findConcaveSeparatingAxisEdgeEdgeKernel");
							b3BufferInfoCL bInfo[] = {
								b3BufferInfoCL(triangleConvexPairsOut.getBufferCL()),
								b3BufferInfoCL(bodyBuf->getBufferCL(), true),
								b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
								b3BufferInfoCL(convexData.getBufferCL(), true),
								b3BufferInfoCL(gpuVertices.getBufferCL(), true),
								b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
								b3BufferInfoCL(gpuFaces.getBufferCL(), true),
								b3BufferInfoCL(gpuIndices.getBufferCL(), true),
								b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
								b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
								b3BufferInfoCL(m_concaveSepNormals.getBufferCL()),
								b3BufferInfoCL(m_concaveHasSeparatingNormals.getBufferCL()),
								b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
								b3BufferInfoCL(worldVertsA1GPU.getBufferCL()),
								b3BufferInfoCL(worldNormalsAGPU.getBufferCL()),
								b3BufferInfoCL(worldVertsB1GPU.getBufferCL()),
								b3BufferInfoCL(m_dmins.getBufferCL())};

							b3LauncherCL launcher(m_queue, m_findConcaveSeparatingAxisEdgeEdgeKernel, "m_findConcaveSeparatingAxisEdgeEdgeKernel");
							launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
							launcher.setConst(vertexFaceCapacity);
							launcher.setConst(numConcavePairs);

							int num = numConcavePairs;
							launcher.launch1D(num);
							clFinish(m_queue);
						}

						// numConcavePairs = 0;
					}
					else
					{
						B3_PROFILE("findConcaveSeparatingAxisKernel");
						b3BufferInfoCL bInfo[] = {
							b3BufferInfoCL(triangleConvexPairsOut.getBufferCL()),
							b3BufferInfoCL(bodyBuf->getBufferCL(), true),
							b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
							b3BufferInfoCL(convexData.getBufferCL(), true),
							b3BufferInfoCL(gpuVertices.getBufferCL(), true),
							b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
							b3BufferInfoCL(gpuFaces.getBufferCL(), true),
							b3BufferInfoCL(gpuIndices.getBufferCL(), true),
							b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
							b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
							b3BufferInfoCL(m_concaveSepNormals.getBufferCL()),
							b3BufferInfoCL(m_concaveHasSeparatingNormals.getBufferCL()),
							b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
							b3BufferInfoCL(worldVertsA1GPU.getBufferCL()),
							b3BufferInfoCL(worldNormalsAGPU.getBufferCL()),
							b3BufferInfoCL(worldVertsB1GPU.getBufferCL())};

						b3LauncherCL launcher(m_queue, m_findConcaveSeparatingAxisKernel, "m_findConcaveSeparatingAxisKernel");
						launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
						launcher.setConst(vertexFaceCapacity);
						launcher.setConst(numConcavePairs);

						int num = numConcavePairs;
						launcher.launch1D(num);
						clFinish(m_queue);
					}
				}
				else
				{
					b3AlignedObjectArray<b3Int4> clippingFacesOutCPU;
					b3AlignedObjectArray<b3Vector3> worldVertsA1CPU;
					b3AlignedObjectArray<b3Vector3> worldNormalsACPU;
					b3AlignedObjectArray<b3Vector3> worldVertsB1CPU;
					b3AlignedObjectArray<int> concaveHasSeparatingNormalsCPU;

					b3AlignedObjectArray<b3Int4> triangleConvexPairsOutHost;
					triangleConvexPairsOut.copyToHost(triangleConvexPairsOutHost);
					//triangleConvexPairsOutHost.resize(maxTriConvexPairCapacity);
					b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
					bodyBuf->copyToHost(hostBodyBuf);
					b3AlignedObjectArray<b3Collidable> hostCollidables;
					gpuCollidables.copyToHost(hostCollidables);
					b3AlignedObjectArray<b3Aabb> hostAabbsWorldSpace;
					clAabbsWorldSpace.copyToHost(hostAabbsWorldSpace);

					b3AlignedObjectArray<b3ConvexPolyhedronData> hostConvexData;
					convexData.copyToHost(hostConvexData);

					b3AlignedObjectArray<b3Vector3> hostVertices;
					gpuVertices.copyToHost(hostVertices);

					b3AlignedObjectArray<b3Vector3> hostUniqueEdges;
					gpuUniqueEdges.copyToHost(hostUniqueEdges);
					b3AlignedObjectArray<b3GpuFace> hostFaces;
					gpuFaces.copyToHost(hostFaces);
					b3AlignedObjectArray<int> hostIndices;
					gpuIndices.copyToHost(hostIndices);
					b3AlignedObjectArray<b3GpuChildShape> cpuChildShapes;
					gpuChildShapes.copyToHost(cpuChildShapes);

					b3AlignedObjectArray<b3Vector3> concaveSepNormalsHost;
					m_concaveSepNormals.copyToHost(concaveSepNormalsHost);
					concaveHasSeparatingNormalsCPU.resize(concaveSepNormalsHost.size());

					b3GpuChildShape* childShapePointerCPU = 0;
					if (cpuChildShapes.size())
						childShapePointerCPU = &cpuChildShapes.at(0);

					clippingFacesOutCPU.resize(clippingFacesOutGPU.size());
					worldVertsA1CPU.resize(worldVertsA1GPU.size());
					worldNormalsACPU.resize(worldNormalsAGPU.size());
					worldVertsB1CPU.resize(worldVertsB1GPU.size());

					for (int i = 0; i < numConcavePairs; i++)
					{
						b3FindConcaveSeparatingAxisKernel(&triangleConvexPairsOutHost.at(0),
														  &hostBodyBuf.at(0),
														  &hostCollidables.at(0),
														  &hostConvexData.at(0), &hostVertices.at(0), &hostUniqueEdges.at(0),
														  &hostFaces.at(0), &hostIndices.at(0), childShapePointerCPU,
														  &hostAabbsWorldSpace.at(0),
														  &concaveSepNormalsHost.at(0),
														  &clippingFacesOutCPU.at(0),
														  &worldVertsA1CPU.at(0),
														  &worldNormalsACPU.at(0),
														  &worldVertsB1CPU.at(0),
														  &concaveHasSeparatingNormalsCPU.at(0),
														  vertexFaceCapacity,
														  numConcavePairs, i);
					};

					m_concaveSepNormals.copyFromHost(concaveSepNormalsHost);
					m_concaveHasSeparatingNormals.copyFromHost(concaveHasSeparatingNormalsCPU);
					clippingFacesOutGPU.copyFromHost(clippingFacesOutCPU);
					worldVertsA1GPU.copyFromHost(worldVertsA1CPU);
					worldNormalsAGPU.copyFromHost(worldNormalsACPU);
					worldVertsB1GPU.copyFromHost(worldVertsB1CPU);
				}
				//							b3AlignedObjectArray<b3Vector3> cpuCompoundSepNormals;
				//						m_concaveSepNormals.copyToHost(cpuCompoundSepNormals);
				//					b3AlignedObjectArray<b3Int4> cpuConcavePairs;
				//				triangleConvexPairsOut.copyToHost(cpuConcavePairs);
			}
		}
	}

	if (numConcavePairs)
	{
		if (numConcavePairs)
		{
			B3_PROFILE("findConcaveSphereContactsKernel");
			nContacts = m_totalContactsOut.at(0);
			//				printf("nContacts1 = %d\n",nContacts);
			b3BufferInfoCL bInfo[] = {
				b3BufferInfoCL(triangleConvexPairsOut.getBufferCL()),
				b3BufferInfoCL(bodyBuf->getBufferCL(), true),
				b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
				b3BufferInfoCL(convexData.getBufferCL(), true),
				b3BufferInfoCL(gpuVertices.getBufferCL(), true),
				b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
				b3BufferInfoCL(gpuFaces.getBufferCL(), true),
				b3BufferInfoCL(gpuIndices.getBufferCL(), true),
				b3BufferInfoCL(clAabbsWorldSpace.getBufferCL(), true),
				b3BufferInfoCL(contactOut->getBufferCL()),
				b3BufferInfoCL(m_totalContactsOut.getBufferCL())};

			b3LauncherCL launcher(m_queue, m_findConcaveSphereContactsKernel, "m_findConcaveSphereContactsKernel");
			launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));

			launcher.setConst(numConcavePairs);
			launcher.setConst(maxContactCapacity);

			int num = numConcavePairs;
			launcher.launch1D(num);
			clFinish(m_queue);
			nContacts = m_totalContactsOut.at(0);
			//printf("nContacts (after findConcaveSphereContactsKernel) = %d\n",nContacts);

			//printf("nContacts2 = %d\n",nContacts);

			if (nContacts >= maxContactCapacity)
			{
				b3Error("Error: contacts exceeds capacity (%d/%d)\n", nContacts, maxContactCapacity);
				nContacts = maxContactCapacity;
			}
		}
	}

#ifdef __APPLE__
	bool contactClippingOnGpu = true;
#else
	bool contactClippingOnGpu = true;
#endif

	if (contactClippingOnGpu)
	{
		m_totalContactsOut.copyFromHostPointer(&nContacts, 1, 0, true);
		//		printf("nContacts3 = %d\n",nContacts);

		//B3_PROFILE("clipHullHullKernel");

		bool breakupConcaveConvexKernel = true;

#ifdef __APPLE__
		//actually, some Apple OpenCL platform/device combinations work fine...
		breakupConcaveConvexKernel = true;
#endif
		//concave-convex contact clipping
		if (numConcavePairs)
		{
			//			printf("numConcavePairs = %d\n", numConcavePairs);
			//		nContacts = m_totalContactsOut.at(0);
			//	printf("nContacts before = %d\n", nContacts);

			if (breakupConcaveConvexKernel)
			{
				worldVertsB2GPU.resize(vertexFaceCapacity * numConcavePairs);

				//clipFacesAndFindContacts

				if (clipConcaveFacesAndFindContactsCPU)
				{
					b3AlignedObjectArray<b3Int4> clippingFacesOutCPU;
					b3AlignedObjectArray<b3Vector3> worldVertsA1CPU;
					b3AlignedObjectArray<b3Vector3> worldNormalsACPU;
					b3AlignedObjectArray<b3Vector3> worldVertsB1CPU;

					clippingFacesOutGPU.copyToHost(clippingFacesOutCPU);
					worldVertsA1GPU.copyToHost(worldVertsA1CPU);
					worldNormalsAGPU.copyToHost(worldNormalsACPU);
					worldVertsB1GPU.copyToHost(worldVertsB1CPU);

					b3AlignedObjectArray<int> concaveHasSeparatingNormalsCPU;
					m_concaveHasSeparatingNormals.copyToHost(concaveHasSeparatingNormalsCPU);

					b3AlignedObjectArray<b3Vector3> concaveSepNormalsHost;
					m_concaveSepNormals.copyToHost(concaveSepNormalsHost);

					b3AlignedObjectArray<b3Vector3> worldVertsB2CPU;
					worldVertsB2CPU.resize(worldVertsB2GPU.size());

					for (int i = 0; i < numConcavePairs; i++)
					{
						clipFacesAndFindContactsKernel(&concaveSepNormalsHost.at(0),
													   &concaveHasSeparatingNormalsCPU.at(0),
													   &clippingFacesOutCPU.at(0),
													   &worldVertsA1CPU.at(0),
													   &worldNormalsACPU.at(0),
													   &worldVertsB1CPU.at(0),
													   &worldVertsB2CPU.at(0),
													   vertexFaceCapacity,
													   i);
					}

					clippingFacesOutGPU.copyFromHost(clippingFacesOutCPU);
					worldVertsB2GPU.copyFromHost(worldVertsB2CPU);
				}
				else
				{
					if (1)
					{
						B3_PROFILE("clipFacesAndFindContacts");
						//nContacts = m_totalContactsOut.at(0);
						//int h = m_hasSeparatingNormals.at(0);
						//int4 p = clippingFacesOutGPU.at(0);
						b3BufferInfoCL bInfo[] = {
							b3BufferInfoCL(m_concaveSepNormals.getBufferCL()),
							b3BufferInfoCL(m_concaveHasSeparatingNormals.getBufferCL()),
							b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
							b3BufferInfoCL(worldVertsA1GPU.getBufferCL()),
							b3BufferInfoCL(worldNormalsAGPU.getBufferCL()),
							b3BufferInfoCL(worldVertsB1GPU.getBufferCL()),
							b3BufferInfoCL(worldVertsB2GPU.getBufferCL())};
						b3LauncherCL launcher(m_queue, m_clipFacesAndFindContacts, "m_clipFacesAndFindContacts");
						launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
						launcher.setConst(vertexFaceCapacity);

						launcher.setConst(numConcavePairs);
						int debugMode = 0;
						launcher.setConst(debugMode);
						int num = numConcavePairs;
						launcher.launch1D(num);
						clFinish(m_queue);
						//int bla = m_totalContactsOut.at(0);
					}
				}
				//contactReduction
				{
					int newContactCapacity = nContacts + numConcavePairs;
					contactOut->reserve(newContactCapacity);
					if (reduceConcaveContactsOnGPU)
					{
						//						printf("newReservation = %d\n",newReservation);
						{
							B3_PROFILE("newContactReductionKernel");
							b3BufferInfoCL bInfo[] =
								{
									b3BufferInfoCL(triangleConvexPairsOut.getBufferCL(), true),
									b3BufferInfoCL(bodyBuf->getBufferCL(), true),
									b3BufferInfoCL(m_concaveSepNormals.getBufferCL()),
									b3BufferInfoCL(m_concaveHasSeparatingNormals.getBufferCL()),
									b3BufferInfoCL(contactOut->getBufferCL()),
									b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
									b3BufferInfoCL(worldVertsB2GPU.getBufferCL()),
									b3BufferInfoCL(m_totalContactsOut.getBufferCL())};

							b3LauncherCL launcher(m_queue, m_newContactReductionKernel, "m_newContactReductionKernel");
							launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
							launcher.setConst(vertexFaceCapacity);
							launcher.setConst(newContactCapacity);
							launcher.setConst(numConcavePairs);
							int num = numConcavePairs;

							launcher.launch1D(num);
						}
						nContacts = m_totalContactsOut.at(0);
						contactOut->resize(nContacts);

						//printf("contactOut4 (after newContactReductionKernel) = %d\n",nContacts);
					}
					else
					{
						volatile int nGlobalContactsOut = nContacts;
						b3AlignedObjectArray<b3Int4> triangleConvexPairsOutHost;
						triangleConvexPairsOut.copyToHost(triangleConvexPairsOutHost);
						b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
						bodyBuf->copyToHost(hostBodyBuf);

						b3AlignedObjectArray<int> concaveHasSeparatingNormalsCPU;
						m_concaveHasSeparatingNormals.copyToHost(concaveHasSeparatingNormalsCPU);

						b3AlignedObjectArray<b3Vector3> concaveSepNormalsHost;
						m_concaveSepNormals.copyToHost(concaveSepNormalsHost);

						b3AlignedObjectArray<b3Contact4> hostContacts;
						if (nContacts)
						{
							contactOut->copyToHost(hostContacts);
						}
						hostContacts.resize(newContactCapacity);

						b3AlignedObjectArray<b3Int4> clippingFacesOutCPU;
						b3AlignedObjectArray<b3Vector3> worldVertsB2CPU;

						clippingFacesOutGPU.copyToHost(clippingFacesOutCPU);
						worldVertsB2GPU.copyToHost(worldVertsB2CPU);

						for (int i = 0; i < numConcavePairs; i++)
						{
							b3NewContactReductionKernel(&triangleConvexPairsOutHost.at(0),
														&hostBodyBuf.at(0),
														&concaveSepNormalsHost.at(0),
														&concaveHasSeparatingNormalsCPU.at(0),
														&hostContacts.at(0),
														&clippingFacesOutCPU.at(0),
														&worldVertsB2CPU.at(0),
														&nGlobalContactsOut,
														vertexFaceCapacity,
														newContactCapacity,
														numConcavePairs,
														i);
						}

						nContacts = nGlobalContactsOut;
						m_totalContactsOut.copyFromHostPointer(&nContacts, 1, 0, true);
						//						nContacts = m_totalContactsOut.at(0);
						//contactOut->resize(nContacts);
						hostContacts.resize(nContacts);
						//printf("contactOut4 (after newContactReductionKernel) = %d\n",nContacts);
						contactOut->copyFromHost(hostContacts);
					}
				}
				//re-use?
			}
			else
			{
				B3_PROFILE("clipHullHullConcaveConvexKernel");
				nContacts = m_totalContactsOut.at(0);
				int newContactCapacity = contactOut->capacity();

				//printf("contactOut5 = %d\n",nContacts);
				b3BufferInfoCL bInfo[] = {
					b3BufferInfoCL(triangleConvexPairsOut.getBufferCL(), true),
					b3BufferInfoCL(bodyBuf->getBufferCL(), true),
					b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
					b3BufferInfoCL(convexData.getBufferCL(), true),
					b3BufferInfoCL(gpuVertices.getBufferCL(), true),
					b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
					b3BufferInfoCL(gpuFaces.getBufferCL(), true),
					b3BufferInfoCL(gpuIndices.getBufferCL(), true),
					b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
					b3BufferInfoCL(m_concaveSepNormals.getBufferCL()),
					b3BufferInfoCL(contactOut->getBufferCL()),
					b3BufferInfoCL(m_totalContactsOut.getBufferCL())};
				b3LauncherCL launcher(m_queue, m_clipHullHullConcaveConvexKernel, "m_clipHullHullConcaveConvexKernel");
				launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
				launcher.setConst(newContactCapacity);
				launcher.setConst(numConcavePairs);
				int num = numConcavePairs;
				launcher.launch1D(num);
				clFinish(m_queue);
				nContacts = m_totalContactsOut.at(0);
				contactOut->resize(nContacts);
				//printf("contactOut6 = %d\n",nContacts);
				b3AlignedObjectArray<b3Contact4> cpuContacts;
				contactOut->copyToHost(cpuContacts);
			}
			//			printf("nContacts after = %d\n", nContacts);
		}  //numConcavePairs

		//convex-convex contact clipping

		bool breakupKernel = false;

#ifdef __APPLE__
		breakupKernel = true;
#endif

#ifdef CHECK_ON_HOST
		bool computeConvexConvex = false;
#else
		bool computeConvexConvex = true;
#endif  //CHECK_ON_HOST
		if (computeConvexConvex)
		{
			B3_PROFILE("clipHullHullKernel");
			if (breakupKernel)
			{
				worldVertsB1GPU.resize(vertexFaceCapacity * nPairs);
				clippingFacesOutGPU.resize(nPairs);
				worldNormalsAGPU.resize(nPairs);
				worldVertsA1GPU.resize(vertexFaceCapacity * nPairs);
				worldVertsB2GPU.resize(vertexFaceCapacity * nPairs);

				if (findConvexClippingFacesGPU)
				{
					B3_PROFILE("findClippingFacesKernel");
					b3BufferInfoCL bInfo[] = {
						b3BufferInfoCL(pairs->getBufferCL(), true),
						b3BufferInfoCL(bodyBuf->getBufferCL(), true),
						b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
						b3BufferInfoCL(convexData.getBufferCL(), true),
						b3BufferInfoCL(gpuVertices.getBufferCL(), true),
						b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
						b3BufferInfoCL(gpuFaces.getBufferCL(), true),
						b3BufferInfoCL(gpuIndices.getBufferCL(), true),
						b3BufferInfoCL(m_sepNormals.getBufferCL()),
						b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
						b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
						b3BufferInfoCL(worldVertsA1GPU.getBufferCL()),
						b3BufferInfoCL(worldNormalsAGPU.getBufferCL()),
						b3BufferInfoCL(worldVertsB1GPU.getBufferCL())};

					b3LauncherCL launcher(m_queue, m_findClippingFacesKernel, "m_findClippingFacesKernel");
					launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
					launcher.setConst(vertexFaceCapacity);
					launcher.setConst(nPairs);
					int num = nPairs;
					launcher.launch1D(num);
					clFinish(m_queue);
				}
				else
				{
					float minDist = -1e30f;
					float maxDist = 0.02f;

					b3AlignedObjectArray<b3ConvexPolyhedronData> hostConvexData;
					convexData.copyToHost(hostConvexData);
					b3AlignedObjectArray<b3Collidable> hostCollidables;
					gpuCollidables.copyToHost(hostCollidables);

					b3AlignedObjectArray<int> hostHasSepNormals;
					m_hasSeparatingNormals.copyToHost(hostHasSepNormals);
					b3AlignedObjectArray<b3Vector3> cpuSepNormals;
					m_sepNormals.copyToHost(cpuSepNormals);

					b3AlignedObjectArray<b3Int4> hostPairs;
					pairs->copyToHost(hostPairs);
					b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
					bodyBuf->copyToHost(hostBodyBuf);

					//worldVertsB1GPU.resize(vertexFaceCapacity*nPairs);
					b3AlignedObjectArray<b3Vector3> worldVertsB1CPU;
					worldVertsB1GPU.copyToHost(worldVertsB1CPU);

					b3AlignedObjectArray<b3Int4> clippingFacesOutCPU;
					clippingFacesOutGPU.copyToHost(clippingFacesOutCPU);

					b3AlignedObjectArray<b3Vector3> worldNormalsACPU;
					worldNormalsACPU.resize(nPairs);

					b3AlignedObjectArray<b3Vector3> worldVertsA1CPU;
					worldVertsA1CPU.resize(worldVertsA1GPU.size());

					b3AlignedObjectArray<b3Vector3> hostVertices;
					gpuVertices.copyToHost(hostVertices);
					b3AlignedObjectArray<b3GpuFace> hostFaces;
					gpuFaces.copyToHost(hostFaces);
					b3AlignedObjectArray<int> hostIndices;
					gpuIndices.copyToHost(hostIndices);

					for (int i = 0; i < nPairs; i++)
					{
						int bodyIndexA = hostPairs[i].x;
						int bodyIndexB = hostPairs[i].y;

						int collidableIndexA = hostBodyBuf[bodyIndexA].m_collidableIdx;
						int collidableIndexB = hostBodyBuf[bodyIndexB].m_collidableIdx;

						int shapeIndexA = hostCollidables[collidableIndexA].m_shapeIndex;
						int shapeIndexB = hostCollidables[collidableIndexB].m_shapeIndex;

						if (hostHasSepNormals[i])
						{
							b3FindClippingFaces(cpuSepNormals[i],
												&hostConvexData[shapeIndexA],
												&hostConvexData[shapeIndexB],
												hostBodyBuf[bodyIndexA].m_pos, hostBodyBuf[bodyIndexA].m_quat,
												hostBodyBuf[bodyIndexB].m_pos, hostBodyBuf[bodyIndexB].m_quat,
												&worldVertsA1CPU.at(0), &worldNormalsACPU.at(0),
												&worldVertsB1CPU.at(0),
												vertexFaceCapacity, minDist, maxDist,
												&hostVertices.at(0), &hostFaces.at(0),
												&hostIndices.at(0),
												&hostVertices.at(0), &hostFaces.at(0),
												&hostIndices.at(0), &clippingFacesOutCPU.at(0), i);
						}
					}

					clippingFacesOutGPU.copyFromHost(clippingFacesOutCPU);
					worldVertsA1GPU.copyFromHost(worldVertsA1CPU);
					worldNormalsAGPU.copyFromHost(worldNormalsACPU);
					worldVertsB1GPU.copyFromHost(worldVertsB1CPU);
				}

				///clip face B against face A, reduce contacts and append them to a global contact array
				if (1)
				{
					if (clipConvexFacesAndFindContactsCPU)
					{
						//b3AlignedObjectArray<b3Int4> hostPairs;
						//pairs->copyToHost(hostPairs);

						b3AlignedObjectArray<b3Vector3> hostSepNormals;
						m_sepNormals.copyToHost(hostSepNormals);
						b3AlignedObjectArray<int> hostHasSepAxis;
						m_hasSeparatingNormals.copyToHost(hostHasSepAxis);

						b3AlignedObjectArray<b3Int4> hostClippingFaces;
						clippingFacesOutGPU.copyToHost(hostClippingFaces);
						b3AlignedObjectArray<b3Vector3> worldVertsB2CPU;
						worldVertsB2CPU.resize(vertexFaceCapacity * nPairs);

						b3AlignedObjectArray<b3Vector3> worldVertsA1CPU;
						worldVertsA1GPU.copyToHost(worldVertsA1CPU);
						b3AlignedObjectArray<b3Vector3> worldNormalsACPU;
						worldNormalsAGPU.copyToHost(worldNormalsACPU);

						b3AlignedObjectArray<b3Vector3> worldVertsB1CPU;
						worldVertsB1GPU.copyToHost(worldVertsB1CPU);

						/*
					  __global const b3Float4* separatingNormals,
                                                   __global const int* hasSeparatingAxis,
                                                   __global b3Int4* clippingFacesOut,
                                                   __global b3Float4* worldVertsA1,
                                                   __global b3Float4* worldNormalsA1,
                                                   __global b3Float4* worldVertsB1,
                                                   __global b3Float4* worldVertsB2,
                                                    int vertexFaceCapacity,
															int pairIndex
					*/
						for (int i = 0; i < nPairs; i++)
						{
							clipFacesAndFindContactsKernel(
								&hostSepNormals.at(0),
								&hostHasSepAxis.at(0),
								&hostClippingFaces.at(0),
								&worldVertsA1CPU.at(0),
								&worldNormalsACPU.at(0),
								&worldVertsB1CPU.at(0),
								&worldVertsB2CPU.at(0),

								vertexFaceCapacity,
								i);
						}

						clippingFacesOutGPU.copyFromHost(hostClippingFaces);
						worldVertsB2GPU.copyFromHost(worldVertsB2CPU);
					}
					else
					{
						B3_PROFILE("clipFacesAndFindContacts");
						//nContacts = m_totalContactsOut.at(0);
						//int h = m_hasSeparatingNormals.at(0);
						//int4 p = clippingFacesOutGPU.at(0);
						b3BufferInfoCL bInfo[] = {
							b3BufferInfoCL(m_sepNormals.getBufferCL()),
							b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
							b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
							b3BufferInfoCL(worldVertsA1GPU.getBufferCL()),
							b3BufferInfoCL(worldNormalsAGPU.getBufferCL()),
							b3BufferInfoCL(worldVertsB1GPU.getBufferCL()),
							b3BufferInfoCL(worldVertsB2GPU.getBufferCL())};

						b3LauncherCL launcher(m_queue, m_clipFacesAndFindContacts, "m_clipFacesAndFindContacts");
						launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
						launcher.setConst(vertexFaceCapacity);

						launcher.setConst(nPairs);
						int debugMode = 0;
						launcher.setConst(debugMode);
						int num = nPairs;
						launcher.launch1D(num);
						clFinish(m_queue);
					}

					{
						nContacts = m_totalContactsOut.at(0);
						//printf("nContacts = %d\n",nContacts);

						int newContactCapacity = nContacts + nPairs;
						contactOut->reserve(newContactCapacity);

						if (reduceConvexContactsOnGPU)
						{
							{
								B3_PROFILE("newContactReductionKernel");
								b3BufferInfoCL bInfo[] =
									{
										b3BufferInfoCL(pairs->getBufferCL(), true),
										b3BufferInfoCL(bodyBuf->getBufferCL(), true),
										b3BufferInfoCL(m_sepNormals.getBufferCL()),
										b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
										b3BufferInfoCL(contactOut->getBufferCL()),
										b3BufferInfoCL(clippingFacesOutGPU.getBufferCL()),
										b3BufferInfoCL(worldVertsB2GPU.getBufferCL()),
										b3BufferInfoCL(m_totalContactsOut.getBufferCL())};

								b3LauncherCL launcher(m_queue, m_newContactReductionKernel, "m_newContactReductionKernel");
								launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
								launcher.setConst(vertexFaceCapacity);
								launcher.setConst(newContactCapacity);
								launcher.setConst(nPairs);
								int num = nPairs;

								launcher.launch1D(num);
							}
							nContacts = m_totalContactsOut.at(0);
							contactOut->resize(nContacts);
						}
						else
						{
							volatile int nGlobalContactsOut = nContacts;
							b3AlignedObjectArray<b3Int4> hostPairs;
							pairs->copyToHost(hostPairs);
							b3AlignedObjectArray<b3RigidBodyData> hostBodyBuf;
							bodyBuf->copyToHost(hostBodyBuf);
							b3AlignedObjectArray<b3Vector3> hostSepNormals;
							m_sepNormals.copyToHost(hostSepNormals);
							b3AlignedObjectArray<int> hostHasSepAxis;
							m_hasSeparatingNormals.copyToHost(hostHasSepAxis);
							b3AlignedObjectArray<b3Contact4> hostContactsOut;
							contactOut->copyToHost(hostContactsOut);
							hostContactsOut.resize(newContactCapacity);

							b3AlignedObjectArray<b3Int4> hostClippingFaces;
							clippingFacesOutGPU.copyToHost(hostClippingFaces);
							b3AlignedObjectArray<b3Vector3> worldVertsB2CPU;
							worldVertsB2GPU.copyToHost(worldVertsB2CPU);

							for (int i = 0; i < nPairs; i++)
							{
								b3NewContactReductionKernel(&hostPairs.at(0),
															&hostBodyBuf.at(0),
															&hostSepNormals.at(0),
															&hostHasSepAxis.at(0),
															&hostContactsOut.at(0),
															&hostClippingFaces.at(0),
															&worldVertsB2CPU.at(0),
															&nGlobalContactsOut,
															vertexFaceCapacity,
															newContactCapacity,
															nPairs,
															i);
							}

							nContacts = nGlobalContactsOut;
							m_totalContactsOut.copyFromHostPointer(&nContacts, 1, 0, true);
							hostContactsOut.resize(nContacts);
							//printf("contactOut4 (after newContactReductionKernel) = %d\n",nContacts);
							contactOut->copyFromHost(hostContactsOut);
						}
						//                    b3Contact4 pt = contactOut->at(0);
						//                  printf("nContacts = %d\n",nContacts);
					}
				}
			}
			else  //breakupKernel
			{
				if (nPairs)
				{
					b3BufferInfoCL bInfo[] = {
						b3BufferInfoCL(pairs->getBufferCL(), true),
						b3BufferInfoCL(bodyBuf->getBufferCL(), true),
						b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
						b3BufferInfoCL(convexData.getBufferCL(), true),
						b3BufferInfoCL(gpuVertices.getBufferCL(), true),
						b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
						b3BufferInfoCL(gpuFaces.getBufferCL(), true),
						b3BufferInfoCL(gpuIndices.getBufferCL(), true),
						b3BufferInfoCL(m_sepNormals.getBufferCL()),
						b3BufferInfoCL(m_hasSeparatingNormals.getBufferCL()),
						b3BufferInfoCL(contactOut->getBufferCL()),
						b3BufferInfoCL(m_totalContactsOut.getBufferCL())};
					b3LauncherCL launcher(m_queue, m_clipHullHullKernel, "m_clipHullHullKernel");
					launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
					launcher.setConst(nPairs);
					launcher.setConst(maxContactCapacity);

					int num = nPairs;
					launcher.launch1D(num);
					clFinish(m_queue);

					nContacts = m_totalContactsOut.at(0);
					if (nContacts >= maxContactCapacity)
					{
						b3Error("Exceeded contact capacity (%d/%d)\n", nContacts, maxContactCapacity);
						nContacts = maxContactCapacity;
					}
					contactOut->resize(nContacts);
				}
			}

			int nCompoundsPairs = m_gpuCompoundPairs.size();

			if (nCompoundsPairs)
			{
				b3BufferInfoCL bInfo[] = {
					b3BufferInfoCL(m_gpuCompoundPairs.getBufferCL(), true),
					b3BufferInfoCL(bodyBuf->getBufferCL(), true),
					b3BufferInfoCL(gpuCollidables.getBufferCL(), true),
					b3BufferInfoCL(convexData.getBufferCL(), true),
					b3BufferInfoCL(gpuVertices.getBufferCL(), true),
					b3BufferInfoCL(gpuUniqueEdges.getBufferCL(), true),
					b3BufferInfoCL(gpuFaces.getBufferCL(), true),
					b3BufferInfoCL(gpuIndices.getBufferCL(), true),
					b3BufferInfoCL(gpuChildShapes.getBufferCL(), true),
					b3BufferInfoCL(m_gpuCompoundSepNormals.getBufferCL(), true),
					b3BufferInfoCL(m_gpuHasCompoundSepNormals.getBufferCL(), true),
					b3BufferInfoCL(contactOut->getBufferCL()),
					b3BufferInfoCL(m_totalContactsOut.getBufferCL())};
				b3LauncherCL launcher(m_queue, m_clipCompoundsHullHullKernel, "m_clipCompoundsHullHullKernel");
				launcher.setBuffers(bInfo, sizeof(bInfo) / sizeof(b3BufferInfoCL));
				launcher.setConst(nCompoundsPairs);
				launcher.setConst(maxContactCapacity);

				int num = nCompoundsPairs;
				launcher.launch1D(num);
				clFinish(m_queue);

				nContacts = m_totalContactsOut.at(0);
				if (nContacts > maxContactCapacity)
				{
					b3Error("Error: contacts exceeds capacity (%d/%d)\n", nContacts, maxContactCapacity);
					nContacts = maxContactCapacity;
				}
				contactOut->resize(nContacts);
			}  //if nCompoundsPairs
		}
	}  //contactClippingOnGpu

	//printf("nContacts end = %d\n",nContacts);

	//printf("frameCount = %d\n",frameCount++);
}
