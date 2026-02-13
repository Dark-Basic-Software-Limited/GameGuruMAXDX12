/// @par
/// 
/// This method peforms what is often called 'string pulling'.
///
/// The start position is clamped to the first polygon in the path, and the 
/// end position is clamped to the last. So the start and end positions should 
/// normally be within or very near the first and last polygons respectively.
///
/// The returned polygon references represent the reference id of the polygon 
/// that is entered at the associated path position. The reference id associated 
/// with the end point will always be zero.  This allows, for example, matching 
/// off-mesh link points to their representative polygons.
///
/// If the provided result buffers are too small for the entire result set, 
/// they will be filled as far as possible from the start toward the end 
/// position.
///
dtStatus dtNavMeshQuery::findStraightPath(const float* startPos, const float* endPos,
										  const dtPolyRef* path, const int pathSize,
										  float* straightPath, unsigned char* straightPathFlags, dtPolyRef* straightPathRefs,
										  int* straightPathCount, const int maxStraightPath, const int options) const
{
	dtAssert(m_nav);

	if (!straightPathCount)
		return DT_FAILURE | DT_INVALID_PARAM;

	*straightPathCount = 0;

	if (!startPos || !dtVisfinite(startPos) ||
		!endPos || !dtVisfinite(endPos) ||
		!path || pathSize <= 0 || !path[0] ||
		maxStraightPath <= 0)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	dtStatus stat = 0;
	
	// TODO: Should this be callers responsibility?
	float closestStartPos[3];
	if (dtStatusFailed(closestPointOnPolyBoundary(path[0], startPos, closestStartPos)))
		return DT_FAILURE | DT_INVALID_PARAM;

	float closestEndPos[3];
	if (dtStatusFailed(closestPointOnPolyBoundary(path[pathSize-1], endPos, closestEndPos)))
		return DT_FAILURE | DT_INVALID_PARAM;
	
	// Add start point.
	stat = appendVertex(closestStartPos, DT_STRAIGHTPATH_START, path[0],
						straightPath, straightPathFlags, straightPathRefs,
						straightPathCount, maxStraightPath);
	if (stat != DT_IN_PROGRESS)
		return stat;
	
	if (pathSize > 1)
	{
		float portalApex[3], portalLeft[3], portalRight[3];
		dtVcopy(portalApex, closestStartPos);
		dtVcopy(portalLeft, portalApex);
		dtVcopy(portalRight, portalApex);
		int apexIndex = 0;
		int leftIndex = 0;
		int rightIndex = 0;
		
		unsigned char leftPolyType = 0;
		unsigned char rightPolyType = 0;
		
		dtPolyRef leftPolyRef = path[0];
		dtPolyRef rightPolyRef = path[0];
		
		for (int i = 0; i < pathSize; ++i)
		{
			float left[3], right[3];
			unsigned char toType;
			
			if (i+1 < pathSize)
			{
				unsigned char fromType; // fromType is ignored.

				// Next portal.
				if (dtStatusFailed(getPortalPoints(path[i], path[i+1], left, right, fromType, toType)))
				{
					// Failed to get portal points, in practice this means that path[i+1] is invalid polygon.
					// Clamp the end point to path[i], and return the path so far.
					
					if (dtStatusFailed(closestPointOnPolyBoundary(path[i], endPos, closestEndPos)))
					{
						// This should only happen when the first polygon is invalid.
						return DT_FAILURE | DT_INVALID_PARAM;
					}

					// Apeend portals along the current straight path segment.
					if (options & (DT_STRAIGHTPATH_AREA_CROSSINGS | DT_STRAIGHTPATH_ALL_CROSSINGS))
					{
						// Ignore status return value as we're just about to return anyway.
						appendPortals(apexIndex, i, closestEndPos, path,
											 straightPath, straightPathFlags, straightPathRefs,
											 straightPathCount, maxStraightPath, options);
					}

					// Ignore status return value as we're just about to return anyway.
					appendVertex(closestEndPos, 0, path[i],
										straightPath, straightPathFlags, straightPathRefs,
										straightPathCount, maxStraightPath);
					
					return DT_SUCCESS | DT_PARTIAL_RESULT | ((*straightPathCount >= maxStraightPath) ? DT_BUFFER_TOO_SMALL : 0);
				}
				
				// If starting really close the portal, advance.
				if (i == 0)
				{
					float t;
					if (dtDistancePtSegSqr2D(portalApex, left, right, t) < dtSqr(0.001f))
						continue;
				}
			}
			else
			{
				// End of the path.
				dtVcopy(left, closestEndPos);
				dtVcopy(right, closestEndPos);
				
				toType = DT_POLYTYPE_GROUND;
			}
			
			// Right vertex.
			if (dtTriArea2D(portalApex, portalRight, right) <= 0.0f)
			{
				if (dtVequal(portalApex, portalRight) || dtTriArea2D(portalApex, portalLeft, right) > 0.0f)
				{
					dtVcopy(portalRight, right);
					rightPolyRef = (i+1 < pathSize) ? path[i+1] : 0;
					rightPolyType = toType;
					rightIndex = i;
				}
				else
				{
					// Append portals along the current straight path segment.
					if (options & (DT_STRAIGHTPATH_AREA_CROSSINGS | DT_STRAIGHTPATH_ALL_CROSSINGS))
					{
						stat = appendPortals(apexIndex, leftIndex, portalLeft, path,
											 straightPath, straightPathFlags, straightPathRefs,
											 straightPathCount, maxStraightPath, options);
						if (stat != DT_IN_PROGRESS)
							return stat;					
					}
				
					dtVcopy(portalApex, portalLeft);
					apexIndex = leftIndex;
					
					unsigned char flags = 0;
					if (!leftPolyRef)
						flags = DT_STRAIGHTPATH_END;
					else if (leftPolyType == DT_POLYTYPE_OFFMESH_CONNECTION)
						flags = DT_STRAIGHTPATH_OFFMESH_CONNECTION;
					dtPolyRef ref = leftPolyRef;
					
					// Append or update vertex
					stat = appendVertex(portalApex, flags, ref,
										straightPath, straightPathFlags, straightPathRefs,
										straightPathCount, maxStraightPath);
					if (stat != DT_IN_PROGRESS)
						return stat;
					
					dtVcopy(portalLeft, portalApex);
					dtVcopy(portalRight, portalApex);
					leftIndex = apexIndex;
					rightIndex = apexIndex;
					
					// Restart
					i = apexIndex;
					
					continue;
				}
			}
			
			// Left vertex.
			if (dtTriArea2D(portalApex, portalLeft, left) >= 0.0f)
			{
				if (dtVequal(portalApex, portalLeft) || dtTriArea2D(portalApex, portalRight, left) < 0.0f)
				{
					dtVcopy(portalLeft, left);
					leftPolyRef = (i+1 < pathSize) ? path[i+1] : 0;
					leftPolyType = toType;
					leftIndex = i;
				}
				else
				{
					// Append portals along the current straight path segment.
					if (options & (DT_STRAIGHTPATH_AREA_CROSSINGS | DT_STRAIGHTPATH_ALL_CROSSINGS))
					{
						stat = appendPortals(apexIndex, rightIndex, portalRight, path,
											 straightPath, straightPathFlags, straightPathRefs,
											 straightPathCount, maxStraightPath, options);
						if (stat != DT_IN_PROGRESS)
							return stat;
					}

					dtVcopy(portalApex, portalRight);
					apexIndex = rightIndex;
					
					unsigned char flags = 0;
					if (!rightPolyRef)
						flags = DT_STRAIGHTPATH_END;
					else if (rightPolyType == DT_POLYTYPE_OFFMESH_CONNECTION)
						flags = DT_STRAIGHTPATH_OFFMESH_CONNECTION;
					dtPolyRef ref = rightPolyRef;

					// Append or update vertex
					stat = appendVertex(portalApex, flags, ref,
										straightPath, straightPathFlags, straightPathRefs,
										straightPathCount, maxStraightPath);
					if (stat != DT_IN_PROGRESS)
						return stat;
					
					dtVcopy(portalLeft, portalApex);
					dtVcopy(portalRight, portalApex);
					leftIndex = apexIndex;
					rightIndex = apexIndex;
					
					// Restart
					i = apexIndex;
					
					continue;
				}
			}
		}

		// Append portals along the current straight path segment.
		if (options & (DT_STRAIGHTPATH_AREA_CROSSINGS | DT_STRAIGHTPATH_ALL_CROSSINGS))
		{
			stat = appendPortals(apexIndex, pathSize-1, closestEndPos, path,
								 straightPath, straightPathFlags, straightPathRefs,
								 straightPathCount, maxStraightPath, options);
			if (stat != DT_IN_PROGRESS)
				return stat;
		}
	}

	// Ignore status return value as we're just about to return anyway.
	appendVertex(closestEndPos, DT_STRAIGHTPATH_END, 0,
						straightPath, straightPathFlags, straightPathRefs,
						straightPathCount, maxStraightPath);
	
	return DT_SUCCESS | ((*straightPathCount >= maxStraightPath) ? DT_BUFFER_TOO_SMALL : 0);
}

/// @par
///
/// This method is optimized for small delta movement and a small number of 
/// polygons. If used for too great a distance, the result set will form an 
/// incomplete path.
///
/// @p resultPos will equal the @p endPos if the end is reached. 
/// Otherwise the closest reachable position will be returned.
/// 
/// @p resultPos is not projected onto the surface of the navigation 
/// mesh. Use #getPolyHeight if this is needed.
///
/// This method treats the end position in the same manner as 
/// the #raycast method. (As a 2D point.) See that method's documentation 
/// for details.
/// 
/// If the @p visited array is too small to hold the entire result set, it will 
/// be filled as far as possible from the start position toward the end 
/// position.
///
dtStatus dtNavMeshQuery::moveAlongSurface(dtPolyRef startRef, const float* startPos, const float* endPos,
										  const dtQueryFilter* filter,
										  float* resultPos, dtPolyRef* visited, int* visitedCount, const int maxVisitedSize) const
{
	dtAssert(m_nav);
	dtAssert(m_tinyNodePool);

	if (!visitedCount)
		return DT_FAILURE | DT_INVALID_PARAM;

	*visitedCount = 0;

	if (!m_nav->isValidPolyRef(startRef) ||
		!startPos || !dtVisfinite(startPos) ||
		!endPos || !dtVisfinite(endPos) ||
		!filter || !resultPos || !visited ||
		maxVisitedSize <= 0)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	dtStatus status = DT_SUCCESS;
	
	static const int MAX_STACK = 48;
	dtNode* stack[MAX_STACK];
	int nstack = 0;
	
	m_tinyNodePool->clear();
	
	dtNode* startNode = m_tinyNodePool->getNode(startRef);
	startNode->pidx = 0;
	startNode->cost = 0;
	startNode->total = 0;
	startNode->id = startRef;
	startNode->flags = DT_NODE_CLOSED;
	stack[nstack++] = startNode;
	
	float bestPos[3];
	float bestDist = FLT_MAX;
	dtNode* bestNode = 0;
	dtVcopy(bestPos, startPos);
	
	// Search constraints
	float searchPos[3], searchRadSqr;
	dtVlerp(searchPos, startPos, endPos, 0.5f);
	searchRadSqr = dtSqr(dtVdist(startPos, endPos)/2.0f + 0.001f);
	
	float verts[DT_VERTS_PER_POLYGON*3];
	
	while (nstack)
	{
		// Pop front.
		dtNode* curNode = stack[0];
		for (int i = 0; i < nstack-1; ++i)
			stack[i] = stack[i+1];
		nstack--;
		
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtPolyRef curRef = curNode->id;
		const dtMeshTile* curTile = 0;
		const dtPoly* curPoly = 0;
		m_nav->getTileAndPolyByRefUnsafe(curRef, &curTile, &curPoly);			
		
		// Collect vertices.
		const int nverts = curPoly->vertCount;
		for (int i = 0; i < nverts; ++i)
			dtVcopy(&verts[i*3], &curTile->verts[curPoly->verts[i]*3]);
		
		// If target is inside the poly, stop search.
		if (dtPointInPolygon(endPos, verts, nverts))
		{
			bestNode = curNode;
			dtVcopy(bestPos, endPos);
			break;
		}
		
		// Find wall edges and find nearest point inside the walls.
		for (int i = 0, j = (int)curPoly->vertCount-1; i < (int)curPoly->vertCount; j = i++)
		{
			// Find links to neighbours.
			static const int MAX_NEIS = 8;
			int nneis = 0;
			dtPolyRef neis[MAX_NEIS];
			
			if (curPoly->neis[j] & DT_EXT_LINK)
			{
				// Tile border.
				for (unsigned int k = curPoly->firstLink; k != DT_NULL_LINK; k = curTile->links[k].next)
				{
					const dtLink* link = &curTile->links[k];
					if (link->edge == j)
					{
						if (link->ref != 0)
						{
							const dtMeshTile* neiTile = 0;
							const dtPoly* neiPoly = 0;
							m_nav->getTileAndPolyByRefUnsafe(link->ref, &neiTile, &neiPoly);
							if (filter->passFilter(link->ref, neiTile, neiPoly))
							{
								if (nneis < MAX_NEIS)
									neis[nneis++] = link->ref;
							}
						}
					}
				}
			}
			else if (curPoly->neis[j])
			{
				const unsigned int idx = (unsigned int)(curPoly->neis[j]-1);
				const dtPolyRef ref = m_nav->getPolyRefBase(curTile) | idx;
				if (filter->passFilter(ref, curTile, &curTile->polys[idx]))
				{
					// Internal edge, encode id.
					neis[nneis++] = ref;
				}
			}
			
			if (!nneis)
			{
				// Wall edge, calc distance.
				const float* vj = &verts[j*3];
				const float* vi = &verts[i*3];
				float tseg;
				const float distSqr = dtDistancePtSegSqr2D(endPos, vj, vi, tseg);
				if (distSqr < bestDist)
				{
                    // Update nearest distance.
					dtVlerp(bestPos, vj,vi, tseg);
					bestDist = distSqr;
					bestNode = curNode;
				}
			}
			else
			{
				for (int k = 0; k < nneis; ++k)
				{
					// Skip if no node can be allocated.
					dtNode* neighbourNode = m_tinyNodePool->getNode(neis[k]);
					if (!neighbourNode)
						continue;
					// Skip if already visited.
					if (neighbourNode->flags & DT_NODE_CLOSED)
						continue;
					
					// Skip the link if it is too far from search constraint.
					// TODO: Maybe should use getPortalPoints(), but this one is way faster.
					const float* vj = &verts[j*3];
					const float* vi = &verts[i*3];
					float tseg;
					float distSqr = dtDistancePtSegSqr2D(searchPos, vj, vi, tseg);
					if (distSqr > searchRadSqr)
						continue;
					
					// Mark as the node as visited and push to queue.
					if (nstack < MAX_STACK)
					{
						neighbourNode->pidx = m_tinyNodePool->getNodeIdx(curNode);
						neighbourNode->flags |= DT_NODE_CLOSED;
						stack[nstack++] = neighbourNode;
					}
				}
			}
		}
	}
	
	int n = 0;
	if (bestNode)
	{
		// Reverse the path.
		dtNode* prev = 0;
		dtNode* node = bestNode;
		do
		{
			dtNode* next = m_tinyNodePool->getNodeAtIdx(node->pidx);
			node->pidx = m_tinyNodePool->getNodeIdx(prev);
			prev = node;
			node = next;
		}
		while (node);
		
		// Store result
		node = prev;
		do
		{
			visited[n++] = node->id;
			if (n >= maxVisitedSize)
			{
				status |= DT_BUFFER_TOO_SMALL;
				break;
			}
			node = m_tinyNodePool->getNodeAtIdx(node->pidx);
		}
		while (node);
	}
	
	dtVcopy(resultPos, bestPos);
	
	*visitedCount = n;
	
	return status;
}


dtStatus dtNavMeshQuery::getPortalPoints(dtPolyRef from, dtPolyRef to, float* left, float* right,
										 unsigned char& fromType, unsigned char& toType) const
{
	dtAssert(m_nav);
	
	const dtMeshTile* fromTile = 0;
	const dtPoly* fromPoly = 0;
	if (dtStatusFailed(m_nav->getTileAndPolyByRef(from, &fromTile, &fromPoly)))
		return DT_FAILURE | DT_INVALID_PARAM;
	fromType = fromPoly->getType();

	const dtMeshTile* toTile = 0;
	const dtPoly* toPoly = 0;
	if (dtStatusFailed(m_nav->getTileAndPolyByRef(to, &toTile, &toPoly)))
		return DT_FAILURE | DT_INVALID_PARAM;
	toType = toPoly->getType();
		
	return getPortalPoints(from, fromPoly, fromTile, to, toPoly, toTile, left, right);
}

// Returns portal points between two polygons.
dtStatus dtNavMeshQuery::getPortalPoints(dtPolyRef from, const dtPoly* fromPoly, const dtMeshTile* fromTile,
										 dtPolyRef to, const dtPoly* toPoly, const dtMeshTile* toTile,
										 float* left, float* right) const
{
	// Find the link that points to the 'to' polygon.
	const dtLink* link = 0;
	for (unsigned int i = fromPoly->firstLink; i != DT_NULL_LINK; i = fromTile->links[i].next)
	{
		if (fromTile->links[i].ref == to)
		{
			link = &fromTile->links[i];
			break;
		}
	}
	if (!link)
		return DT_FAILURE | DT_INVALID_PARAM;
	
	// Handle off-mesh connections.
	if (fromPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
	{
		// Find link that points to first vertex.
		for (unsigned int i = fromPoly->firstLink; i != DT_NULL_LINK; i = fromTile->links[i].next)
		{
			if (fromTile->links[i].ref == to)
			{
				const int v = fromTile->links[i].edge;
				dtVcopy(left, &fromTile->verts[fromPoly->verts[v]*3]);
				dtVcopy(right, &fromTile->verts[fromPoly->verts[v]*3]);
				return DT_SUCCESS;
			}
		}
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	if (toPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
	{
		for (unsigned int i = toPoly->firstLink; i != DT_NULL_LINK; i = toTile->links[i].next)
		{
			if (toTile->links[i].ref == from)
			{
				const int v = toTile->links[i].edge;
				dtVcopy(left, &toTile->verts[toPoly->verts[v]*3]);
				dtVcopy(right, &toTile->verts[toPoly->verts[v]*3]);
				return DT_SUCCESS;
			}
		}
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	// Find portal vertices.
	const int v0 = fromPoly->verts[link->edge];
	const int v1 = fromPoly->verts[(link->edge+1) % (int)fromPoly->vertCount];
	dtVcopy(left, &fromTile->verts[v0*3]);
	dtVcopy(right, &fromTile->verts[v1*3]);
	
	// If the link is at tile boundary, dtClamp the vertices to
	// the link width.
	if (link->side != 0xff)
	{
		// Unpack portal limits.
		if (link->bmin != 0 || link->bmax != 255)
		{
			const float s = 1.0f/255.0f;
			const float tmin = link->bmin*s;
			const float tmax = link->bmax*s;
			dtVlerp(left, &fromTile->verts[v0*3], &fromTile->verts[v1*3], tmin);
			dtVlerp(right, &fromTile->verts[v0*3], &fromTile->verts[v1*3], tmax);
		}
	}
	
	return DT_SUCCESS;
}

// Returns edge mid point between two polygons.
dtStatus dtNavMeshQuery::getEdgeMidPoint(dtPolyRef from, dtPolyRef to, float* mid) const
{
	float left[3], right[3];
	unsigned char fromType, toType;
	if (dtStatusFailed(getPortalPoints(from, to, left,right, fromType, toType)))
		return DT_FAILURE | DT_INVALID_PARAM;
	mid[0] = (left[0]+right[0])*0.5f;
	mid[1] = (left[1]+right[1])*0.5f;
	mid[2] = (left[2]+right[2])*0.5f;
	return DT_SUCCESS;
}

dtStatus dtNavMeshQuery::getEdgeMidPoint(dtPolyRef from, const dtPoly* fromPoly, const dtMeshTile* fromTile,
										 dtPolyRef to, const dtPoly* toPoly, const dtMeshTile* toTile,
										 float* mid) const
{
	float left[3], right[3];
	if (dtStatusFailed(getPortalPoints(from, fromPoly, fromTile, to, toPoly, toTile, left, right)))
		return DT_FAILURE | DT_INVALID_PARAM;
	mid[0] = (left[0]+right[0])*0.5f;
	mid[1] = (left[1]+right[1])*0.5f;
	mid[2] = (left[2]+right[2])*0.5f;
	return DT_SUCCESS;
}



/// @par
///
/// This method is meant to be used for quick, short distance checks.
///
/// If the path array is too small to hold the result, it will be filled as 
/// far as possible from the start postion toward the end position.
///
/// <b>Using the Hit Parameter (t)</b>
/// 
/// If the hit parameter is a very high value (FLT_MAX), then the ray has hit 
/// the end position. In this case the path represents a valid corridor to the 
/// end position and the value of @p hitNormal is undefined.
///
/// If the hit parameter is zero, then the start position is on the wall that 
/// was hit and the value of @p hitNormal is undefined.
///
/// If 0 < t < 1.0 then the following applies:
///
/// @code
/// distanceToHitBorder = distanceToEndPosition * t
/// hitPoint = startPos + (endPos - startPos) * t
/// @endcode
///
/// <b>Use Case Restriction</b>
///
/// The raycast ignores the y-value of the end position. (2D check.) This 
/// places significant limits on how it can be used. For example:
///
/// Consider a scene where there is a main floor with a second floor balcony 
/// that hangs over the main floor. So the first floor mesh extends below the 
/// balcony mesh. The start position is somewhere on the first floor. The end 
/// position is on the balcony.
///
/// The raycast will search toward the end position along the first floor mesh. 
/// If it reaches the end position's xz-coordinates it will indicate FLT_MAX
/// (no wall hit), meaning it reached the end position. This is one example of why
/// this method is meant for short distance checks.
///
dtStatus dtNavMeshQuery::raycast(dtPolyRef startRef, const float* startPos, const float* endPos,
								 const dtQueryFilter* filter,
								 float* t, float* hitNormal, dtPolyRef* path, int* pathCount, const int maxPath) const
{
	dtRaycastHit hit;
	hit.path = path;
	hit.maxPath = maxPath;

	dtStatus status = raycast(startRef, startPos, endPos, filter, 0, &hit);
	
	*t = hit.t;
	if (hitNormal)
		dtVcopy(hitNormal, hit.hitNormal);
	if (pathCount)
		*pathCount = hit.pathCount;

	return status;
}


/// @par
///
/// This method is meant to be used for quick, short distance checks.
///
/// If the path array is too small to hold the result, it will be filled as 
/// far as possible from the start postion toward the end position.
///
/// <b>Using the Hit Parameter t of RaycastHit</b>
/// 
/// If the hit parameter is a very high value (FLT_MAX), then the ray has hit 
/// the end position. In this case the path represents a valid corridor to the 
/// end position and the value of @p hitNormal is undefined.
///
/// If the hit parameter is zero, then the start position is on the wall that 
/// was hit and the value of @p hitNormal is undefined.
///
/// If 0 < t < 1.0 then the following applies:
///
/// @code
/// distanceToHitBorder = distanceToEndPosition * t
/// hitPoint = startPos + (endPos - startPos) * t
/// @endcode
///
/// <b>Use Case Restriction</b>
///
/// The raycast ignores the y-value of the end position. (2D check.) This 
/// places significant limits on how it can be used. For example:
///
/// Consider a scene where there is a main floor with a second floor balcony 
/// that hangs over the main floor. So the first floor mesh extends below the 
/// balcony mesh. The start position is somewhere on the first floor. The end 
/// position is on the balcony.
///
/// The raycast will search toward the end position along the first floor mesh. 
/// If it reaches the end position's xz-coordinates it will indicate FLT_MAX
/// (no wall hit), meaning it reached the end position. This is one example of why
/// this method is meant for short distance checks.
///
dtStatus dtNavMeshQuery::raycast(dtPolyRef startRef, const float* startPos, const float* endPos,
								 const dtQueryFilter* filter, const unsigned int options,
								 dtRaycastHit* hit, dtPolyRef prevRef) const
{
	dtAssert(m_nav);

	if (!hit)
		return DT_FAILURE | DT_INVALID_PARAM;

	hit->t = 0;
	hit->pathCount = 0;
	hit->pathCost = 0;

	// Validate input
	if (!m_nav->isValidPolyRef(startRef) ||
		!startPos || !dtVisfinite(startPos) ||
		!endPos || !dtVisfinite(endPos) ||
		!filter ||
		(prevRef && !m_nav->isValidPolyRef(prevRef)))
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	float dir[3], curPos[3], lastPos[3];
	float verts[DT_VERTS_PER_POLYGON*3+3];	
	int n = 0;

	dtVcopy(curPos, startPos);
	dtVsub(dir, endPos, startPos);
	dtVset(hit->hitNormal, 0, 0, 0);

	dtStatus status = DT_SUCCESS;

	const dtMeshTile* prevTile, *tile, *nextTile;
	const dtPoly* prevPoly, *poly, *nextPoly;
	dtPolyRef curRef;

	// The API input has been checked already, skip checking internal data.
	curRef = startRef;
	tile = 0;
	poly = 0;
	m_nav->getTileAndPolyByRefUnsafe(curRef, &tile, &poly);
	nextTile = prevTile = tile;
	nextPoly = prevPoly = poly;
	if (prevRef)
		m_nav->getTileAndPolyByRefUnsafe(prevRef, &prevTile, &prevPoly);

	while (curRef)
	{
		// Cast ray against current polygon.
		
		// Collect vertices.
		int nv = 0;
		for (int i = 0; i < (int)poly->vertCount; ++i)
		{
			dtVcopy(&verts[nv*3], &tile->verts[poly->verts[i]*3]);
			nv++;
		}
		
		float tmin, tmax;
		int segMin, segMax;
		if (!dtIntersectSegmentPoly2D(startPos, endPos, verts, nv, tmin, tmax, segMin, segMax))
		{
			// Could not hit the polygon, keep the old t and report hit.
			hit->pathCount = n;
			return status;
		}

		hit->hitEdgeIndex = segMax;

		// Keep track of furthest t so far.
		if (tmax > hit->t)
			hit->t = tmax;
		
		// Store visited polygons.
		if (n < hit->maxPath)
			hit->path[n++] = curRef;
		else
			status |= DT_BUFFER_TOO_SMALL;

		// Ray end is completely inside the polygon.
		if (segMax == -1)
		{
			hit->t = FLT_MAX;
			hit->pathCount = n;
			
			// add the cost
			if (options & DT_RAYCAST_USE_COSTS)
				hit->pathCost += filter->getCost(curPos, endPos, prevRef, prevTile, prevPoly, curRef, tile, poly, curRef, tile, poly);
			return status;
		}

		// Follow neighbours.
		dtPolyRef nextRef = 0;
		
		for (unsigned int i = poly->firstLink; i != DT_NULL_LINK; i = tile->links[i].next)
		{
			const dtLink* link = &tile->links[i];
			
			// Find link which contains this edge.
			if ((int)link->edge != segMax)
				continue;
			
			// Get pointer to the next polygon.
			nextTile = 0;
			nextPoly = 0;
			m_nav->getTileAndPolyByRefUnsafe(link->ref, &nextTile, &nextPoly);
			
			// Skip off-mesh connections.
			if (nextPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
				continue;
			
			// Skip links based on filter.
			if (!filter->passFilter(link->ref, nextTile, nextPoly))
				continue;
			
			// If the link is internal, just return the ref.
			if (link->side == 0xff)
			{
				nextRef = link->ref;
				break;
			}
			
			// If the link is at tile boundary,
			
			// Check if the link spans the whole edge, and accept.
			if (link->bmin == 0 && link->bmax == 255)
			{
				nextRef = link->ref;
				break;
			}
			
			// Check for partial edge links.
			const int v0 = poly->verts[link->edge];
			const int v1 = poly->verts[(link->edge+1) % poly->vertCount];
			const float* left = &tile->verts[v0*3];
			const float* right = &tile->verts[v1*3];
			
			// Check that the intersection lies inside the link portal.
			if (link->side == 0 || link->side == 4)
			{
				// Calculate link size.
				const float s = 1.0f/255.0f;
				float lmin = left[2] + (right[2] - left[2])*(link->bmin*s);
				float lmax = left[2] + (right[2] - left[2])*(link->bmax*s);
				if (lmin > lmax) dtSwap(lmin, lmax);
				
				// Find Z intersection.
				float z = startPos[2] + (endPos[2]-startPos[2])*tmax;
				if (z >= lmin && z <= lmax)
				{
					nextRef = link->ref;
					break;
				}
			}
			else if (link->side == 2 || link->side == 6)
			{
				// Calculate link size.
				const float s = 1.0f/255.0f;
				float lmin = left[0] + (right[0] - left[0])*(link->bmin*s);
				float lmax = left[0] + (right[0] - left[0])*(link->bmax*s);
				if (lmin > lmax) dtSwap(lmin, lmax);
				
				// Find X intersection.
				float x = startPos[0] + (endPos[0]-startPos[0])*tmax;
				if (x >= lmin && x <= lmax)
				{
					nextRef = link->ref;
					break;
				}
			}
		}
		
		// add the cost
		if (options & DT_RAYCAST_USE_COSTS)
		{
			// compute the intersection point at the furthest end of the polygon
			// and correct the height (since the raycast moves in 2d)
			dtVcopy(lastPos, curPos);
			dtVmad(curPos, startPos, dir, hit->t);
			float* e1 = &verts[segMax*3];
			float* e2 = &verts[((segMax+1)%nv)*3];
			float eDir[3], diff[3];
			dtVsub(eDir, e2, e1);
			dtVsub(diff, curPos, e1);
			float s = dtSqr(eDir[0]) > dtSqr(eDir[2]) ? diff[0] / eDir[0] : diff[2] / eDir[2];
			curPos[1] = e1[1] + eDir[1] * s;

			hit->pathCost += filter->getCost(lastPos, curPos, prevRef, prevTile, prevPoly, curRef, tile, poly, nextRef, nextTile, nextPoly);
		}

		if (!nextRef)
		{
			// No neighbour, we hit a wall.
			
			// Calculate hit normal.
			const int a = segMax;
			const int b = segMax+1 < nv ? segMax+1 : 0;
			const float* va = &verts[a*3];
			const float* vb = &verts[b*3];
			const float dx = vb[0] - va[0];
			const float dz = vb[2] - va[2];
			hit->hitNormal[0] = dz;
			hit->hitNormal[1] = 0;
			hit->hitNormal[2] = -dx;
			dtVnormalize(hit->hitNormal);
			
			hit->pathCount = n;
			return status;
		}

		// No hit, advance to neighbour polygon.
		prevRef = curRef;
		curRef = nextRef;
		prevTile = tile;
		tile = nextTile;
		prevPoly = poly;
		poly = nextPoly;
	}
	
	hit->pathCount = n;
	
	return status;
}

/// @par
///
/// At least one result array must be provided.
///
/// The order of the result set is from least to highest cost to reach the polygon.
///
/// A common use case for this method is to perform Dijkstra searches. 
/// Candidate polygons are found by searching the graph beginning at the start polygon.
///
/// If a polygon is not found via the graph search, even if it intersects the 
/// search circle, it will not be included in the result set. For example:
///
/// polyA is the start polygon.
/// polyB shares an edge with polyA. (Is adjacent.)
/// polyC shares an edge with polyB, but not with polyA
/// Even if the search circle overlaps polyC, it will not be included in the 
/// result set unless polyB is also in the set.
/// 
/// The value of the center point is used as the start position for cost 
/// calculations. It is not projected onto the surface of the mesh, so its 
/// y-value will effect the costs.
///
/// Intersection tests occur in 2D. All polygons and the search circle are 
/// projected onto the xz-plane. So the y-value of the center point does not 
/// effect intersection tests.
///
/// If the result arrays are to small to hold the entire result set, they will be 
/// filled to capacity.
/// 
dtStatus dtNavMeshQuery::findPolysAroundCircle(dtPolyRef startRef, const float* centerPos, const float radius,
											   const dtQueryFilter* filter,
											   dtPolyRef* resultRef, dtPolyRef* resultParent, float* resultCost,
											   int* resultCount, const int maxResult) const
{
	dtAssert(m_nav);
	dtAssert(m_nodePool);
	dtAssert(m_openList);

	if (!resultCount)
		return DT_FAILURE | DT_INVALID_PARAM;

	*resultCount = 0;

	if (!m_nav->isValidPolyRef(startRef) ||
		!centerPos || !dtVisfinite(centerPos) ||
		radius < 0 || !dtMathIsfinite(radius) ||
		!filter || maxResult < 0)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	m_nodePool->clear();
	m_openList->clear();
	
	dtNode* startNode = m_nodePool->getNode(startRef);
	dtVcopy(startNode->pos, centerPos);
	startNode->pidx = 0;
	startNode->cost = 0;
	startNode->total = 0;
	startNode->id = startRef;
	startNode->flags = DT_NODE_OPEN;
	m_openList->push(startNode);
	
	dtStatus status = DT_SUCCESS;
	
	int n = 0;
	
	const float radiusSqr = dtSqr(radius);
	
	while (!m_openList->empty())
	{
		dtNode* bestNode = m_openList->pop();
		bestNode->flags &= ~DT_NODE_OPEN;
		bestNode->flags |= DT_NODE_CLOSED;
		
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtPolyRef bestRef = bestNode->id;
		const dtMeshTile* bestTile = 0;
		const dtPoly* bestPoly = 0;
		m_nav->getTileAndPolyByRefUnsafe(bestRef, &bestTile, &bestPoly);
		
		// Get parent poly and tile.
		dtPolyRef parentRef = 0;
		const dtMeshTile* parentTile = 0;
		const dtPoly* parentPoly = 0;
		if (bestNode->pidx)
			parentRef = m_nodePool->getNodeAtIdx(bestNode->pidx)->id;
		if (parentRef)
			m_nav->getTileAndPolyByRefUnsafe(parentRef, &parentTile, &parentPoly);

		if (n < maxResult)
		{
			if (resultRef)
				resultRef[n] = bestRef;
			if (resultParent)
				resultParent[n] = parentRef;
			if (resultCost)
				resultCost[n] = bestNode->total;
			++n;
		}
		else
		{
			status |= DT_BUFFER_TOO_SMALL;
		}
		
		for (unsigned int i = bestPoly->firstLink; i != DT_NULL_LINK; i = bestTile->links[i].next)
		{
			const dtLink* link = &bestTile->links[i];
			dtPolyRef neighbourRef = link->ref;
			// Skip invalid neighbours and do not follow back to parent.
			if (!neighbourRef || neighbourRef == parentRef)
				continue;
			
			// Expand to neighbour
			const dtMeshTile* neighbourTile = 0;
			const dtPoly* neighbourPoly = 0;
			m_nav->getTileAndPolyByRefUnsafe(neighbourRef, &neighbourTile, &neighbourPoly);
		
			// Do not advance if the polygon is excluded by the filter.
			if (!filter->passFilter(neighbourRef, neighbourTile, neighbourPoly))
				continue;
			
			// Find edge and calc distance to the edge.
			float va[3], vb[3];
			if (!getPortalPoints(bestRef, bestPoly, bestTile, neighbourRef, neighbourPoly, neighbourTile, va, vb))
				continue;
			
			// If the circle is not touching the next polygon, skip it.
			float tseg;
			float distSqr = dtDistancePtSegSqr2D(centerPos, va, vb, tseg);
			if (distSqr > radiusSqr)
				continue;
			
			dtNode* neighbourNode = m_nodePool->getNode(neighbourRef);
			if (!neighbourNode)
			{
				status |= DT_OUT_OF_NODES;
				continue;
			}
				
			if (neighbourNode->flags & DT_NODE_CLOSED)
				continue;
			
			// Cost
			if (neighbourNode->flags == 0)
				dtVlerp(neighbourNode->pos, va, vb, 0.5f);
			
			float cost = filter->getCost(
				bestNode->pos, neighbourNode->pos,
				parentRef, parentTile, parentPoly,
				bestRef, bestTile, bestPoly,
				neighbourRef, neighbourTile, neighbourPoly);

			const float total = bestNode->total + cost;
			
			// The node is already in open list and the new result is worse, skip.
			if ((neighbourNode->flags & DT_NODE_OPEN) && total >= neighbourNode->total)
				continue;
			
			neighbourNode->id = neighbourRef;
			neighbourNode->pidx = m_nodePool->getNodeIdx(bestNode);
			neighbourNode->total = total;
			
			if (neighbourNode->flags & DT_NODE_OPEN)
			{
				m_openList->modify(neighbourNode);
			}
			else
			{
				neighbourNode->flags = DT_NODE_OPEN;
				m_openList->push(neighbourNode);
			}
		}
	}
	
	*resultCount = n;
	
	return status;
}

/// @par
///
/// The order of the result set is from least to highest cost.
/// 
/// At least one result array must be provided.
///
/// A common use case for this method is to perform Dijkstra searches. 
/// Candidate polygons are found by searching the graph beginning at the start 
/// polygon.
/// 
/// The same intersection test restrictions that apply to findPolysAroundCircle()
/// method apply to this method.
/// 
/// The 3D centroid of the search polygon is used as the start position for cost 
/// calculations.
/// 
/// Intersection tests occur in 2D. All polygons are projected onto the 
/// xz-plane. So the y-values of the vertices do not effect intersection tests.
/// 
/// If the result arrays are is too small to hold the entire result set, they will 
/// be filled to capacity.
///
dtStatus dtNavMeshQuery::findPolysAroundShape(dtPolyRef startRef, const float* verts, const int nverts,
											  const dtQueryFilter* filter,
											  dtPolyRef* resultRef, dtPolyRef* resultParent, float* resultCost,
											  int* resultCount, const int maxResult) const
{
	dtAssert(m_nav);
	dtAssert(m_nodePool);
	dtAssert(m_openList);

	if (!resultCount)
		return DT_FAILURE | DT_INVALID_PARAM;

	*resultCount = 0;

	if (!m_nav->isValidPolyRef(startRef) ||
		!verts || nverts < 3 ||
		!filter || maxResult < 0)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	// Validate input
	if (!startRef || !m_nav->isValidPolyRef(startRef))
		return DT_FAILURE | DT_INVALID_PARAM;
	
	m_nodePool->clear();
	m_openList->clear();
	
	float centerPos[3] = {0,0,0};
	for (int i = 0; i < nverts; ++i)
		dtVadd(centerPos,centerPos,&verts[i*3]);
	dtVscale(centerPos,centerPos,1.0f/nverts);

	dtNode* startNode = m_nodePool->getNode(startRef);
	dtVcopy(startNode->pos, centerPos);
	startNode->pidx = 0;
	startNode->cost = 0;
	startNode->total = 0;
	startNode->id = startRef;
	startNode->flags = DT_NODE_OPEN;
	m_openList->push(startNode);
	
	dtStatus status = DT_SUCCESS;

	int n = 0;
	
	while (!m_openList->empty())
	{
		dtNode* bestNode = m_openList->pop();
		bestNode->flags &= ~DT_NODE_OPEN;
		bestNode->flags |= DT_NODE_CLOSED;
		
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtPolyRef bestRef = bestNode->id;
		const dtMeshTile* bestTile = 0;
		const dtPoly* bestPoly = 0;
		m_nav->getTileAndPolyByRefUnsafe(bestRef, &bestTile, &bestPoly);
		
		// Get parent poly and tile.
		dtPolyRef parentRef = 0;
		const dtMeshTile* parentTile = 0;
		const dtPoly* parentPoly = 0;
		if (bestNode->pidx)
			parentRef = m_nodePool->getNodeAtIdx(bestNode->pidx)->id;
		if (parentRef)
			m_nav->getTileAndPolyByRefUnsafe(parentRef, &parentTile, &parentPoly);

		if (n < maxResult)
		{
			if (resultRef)
				resultRef[n] = bestRef;
			if (resultParent)
				resultParent[n] = parentRef;
			if (resultCost)
				resultCost[n] = bestNode->total;

			++n;
		}
		else
		{
			status |= DT_BUFFER_TOO_SMALL;
		}
		
		for (unsigned int i = bestPoly->firstLink; i != DT_NULL_LINK; i = bestTile->links[i].next)
		{
			const dtLink* link = &bestTile->links[i];
			dtPolyRef neighbourRef = link->ref;
			// Skip invalid neighbours and do not follow back to parent.
			if (!neighbourRef || neighbourRef == parentRef)
				continue;
			
			// Expand to neighbour
			const dtMeshTile* neighbourTile = 0;
			const dtPoly* neighbourPoly = 0;
			m_nav->getTileAndPolyByRefUnsafe(neighbourRef, &neighbourTile, &neighbourPoly);
			
			// Do not advance if the polygon is excluded by the filter.
			if (!filter->passFilter(neighbourRef, neighbourTile, neighbourPoly))
				continue;
			
			// Find edge and calc distance to the edge.
			float va[3], vb[3];
			if (!getPortalPoints(bestRef, bestPoly, bestTile, neighbourRef, neighbourPoly, neighbourTile, va, vb))
				continue;
			
			// If the poly is not touching the edge to the next polygon, skip the connection it.
			float tmin, tmax;
			int segMin, segMax;
			if (!dtIntersectSegmentPoly2D(va, vb, verts, nverts, tmin, tmax, segMin, segMax))
				continue;
			if (tmin > 1.0f || tmax < 0.0f)
				continue;
			
			dtNode* neighbourNode = m_nodePool->getNode(neighbourRef);
			if (!neighbourNode)
			{
				status |= DT_OUT_OF_NODES;
				continue;
			}
			
			if (neighbourNode->flags & DT_NODE_CLOSED)
				continue;
			
			// Cost
			if (neighbourNode->flags == 0)
				dtVlerp(neighbourNode->pos, va, vb, 0.5f);
			
			float cost = filter->getCost(
				bestNode->pos, neighbourNode->pos,
				parentRef, parentTile, parentPoly,
				bestRef, bestTile, bestPoly,
				neighbourRef, neighbourTile, neighbourPoly);

			const float total = bestNode->total + cost;
			
			// The node is already in open list and the new result is worse, skip.
			if ((neighbourNode->flags & DT_NODE_OPEN) && total >= neighbourNode->total)
				continue;
			
			neighbourNode->id = neighbourRef;
			neighbourNode->pidx = m_nodePool->getNodeIdx(bestNode);
			neighbourNode->total = total;
			
			if (neighbourNode->flags & DT_NODE_OPEN)
			{
				m_openList->modify(neighbourNode);
			}
			else
			{
				neighbourNode->flags = DT_NODE_OPEN;
				m_openList->push(neighbourNode);
			}
		}
	}
	
	*resultCount = n;
	
	return status;
}

dtStatus dtNavMeshQuery::getPathFromDijkstraSearch(dtPolyRef endRef, dtPolyRef* path, int* pathCount, int maxPath) const
{
	if (!m_nav->isValidPolyRef(endRef) || !path || !pathCount || maxPath < 0)
		return DT_FAILURE | DT_INVALID_PARAM;

	*pathCount = 0;

	dtNode* endNode;
	if (m_nodePool->findNodes(endRef, &endNode, 1) != 1 ||
		(endNode->flags & DT_NODE_CLOSED) == 0)
		return DT_FAILURE | DT_INVALID_PARAM;

	return getPathToNode(endNode, path, pathCount, maxPath);
}

/// @par
///
/// This method is optimized for a small search radius and small number of result 
/// polygons.
///
/// Candidate polygons are found by searching the navigation graph beginning at 
/// the start polygon.
///
/// The same intersection test restrictions that apply to the findPolysAroundCircle 
/// mehtod applies to this method.
///
/// The value of the center point is used as the start point for cost calculations. 
/// It is not projected onto the surface of the mesh, so its y-value will effect 
/// the costs.
/// 
/// Intersection tests occur in 2D. All polygons and the search circle are 
/// projected onto the xz-plane. So the y-value of the center point does not 
/// effect intersection tests.
/// 
/// If the result arrays are is too small to hold the entire result set, they will 
/// be filled to capacity.
/// 
dtStatus dtNavMeshQuery::findLocalNeighbourhood(dtPolyRef startRef, const float* centerPos, const float radius,
												const dtQueryFilter* filter,
												dtPolyRef* resultRef, dtPolyRef* resultParent,
												int* resultCount, const int maxResult) const
{
	dtAssert(m_nav);
	dtAssert(m_tinyNodePool);

	if (!resultCount)
		return DT_FAILURE | DT_INVALID_PARAM;

	*resultCount = 0;

	if (!m_nav->isValidPolyRef(startRef) ||
		!centerPos || !dtVisfinite(centerPos) ||
		radius < 0 || !dtMathIsfinite(radius) ||
		!filter || maxResult < 0)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}

	static const int MAX_STACK = 48;
	dtNode* stack[MAX_STACK];
	int nstack = 0;
	
	m_tinyNodePool->clear();
	
	dtNode* startNode = m_tinyNodePool->getNode(startRef);
	startNode->pidx = 0;
	startNode->id = startRef;
	startNode->flags = DT_NODE_CLOSED;
	stack[nstack++] = startNode;
	
	const float radiusSqr = dtSqr(radius);
	
	float pa[DT_VERTS_PER_POLYGON*3];
	float pb[DT_VERTS_PER_POLYGON*3];
	
	dtStatus status = DT_SUCCESS;
	
	int n = 0;
	if (n < maxResult)
	{
		resultRef[n] = startNode->id;
		if (resultParent)
			resultParent[n] = 0;
		++n;
	}
	else
	{
		status |= DT_BUFFER_TOO_SMALL;
	}
	
	while (nstack)
	{
		// Pop front.
		dtNode* curNode = stack[0];
		for (int i = 0; i < nstack-1; ++i)
			stack[i] = stack[i+1];
		nstack--;
		
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtPolyRef curRef = curNode->id;
		const dtMeshTile* curTile = 0;
		const dtPoly* curPoly = 0;
		m_nav->getTileAndPolyByRefUnsafe(curRef, &curTile, &curPoly);
		
		for (unsigned int i = curPoly->firstLink; i != DT_NULL_LINK; i = curTile->links[i].next)
		{
			const dtLink* link = &curTile->links[i];
			dtPolyRef neighbourRef = link->ref;
			// Skip invalid neighbours.
			if (!neighbourRef)
				continue;
			
			// Skip if cannot alloca more nodes.
			dtNode* neighbourNode = m_tinyNodePool->getNode(neighbourRef);
			if (!neighbourNode)
				continue;
			// Skip visited.
			if (neighbourNode->flags & DT_NODE_CLOSED)
				continue;
			
			// Expand to neighbour
			const dtMeshTile* neighbourTile = 0;
			const dtPoly* neighbourPoly = 0;
			m_nav->getTileAndPolyByRefUnsafe(neighbourRef, &neighbourTile, &neighbourPoly);
			
			// Skip off-mesh connections.
			if (neighbourPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
				continue;
			
			// Do not advance if the polygon is excluded by the filter.
			if (!filter->passFilter(neighbourRef, neighbourTile, neighbourPoly))
				continue;
			
			// Find edge and calc distance to the edge.
			float va[3], vb[3];
			if (!getPortalPoints(curRef, curPoly, curTile, neighbourRef, neighbourPoly, neighbourTile, va, vb))
				continue;
			
			// If the circle is not touching the next polygon, skip it.
			float tseg;
			float distSqr = dtDistancePtSegSqr2D(centerPos, va, vb, tseg);
			if (distSqr > radiusSqr)
				continue;
			
			// Mark node visited, this is done before the overlap test so that
			// we will not visit the poly again if the test fails.
			neighbourNode->flags |= DT_NODE_CLOSED;
			neighbourNode->pidx = m_tinyNodePool->getNodeIdx(curNode);
			
			// Check that the polygon does not collide with existing polygons.
			
			// Collect vertices of the neighbour poly.
			const int npa = neighbourPoly->vertCount;
			for (int k = 0; k < npa; ++k)
				dtVcopy(&pa[k*3], &neighbourTile->verts[neighbourPoly->verts[k]*3]);
			
			bool overlap = false;
			for (int j = 0; j < n; ++j)
			{
				dtPolyRef pastRef = resultRef[j];
				
				// Connected polys do not overlap.
				bool connected = false;
				for (unsigned int k = curPoly->firstLink; k != DT_NULL_LINK; k = curTile->links[k].next)
				{
					if (curTile->links[k].ref == pastRef)
					{
						connected = true;
						break;
					}
				}
				if (connected)
					continue;
				
				// Potentially overlapping.
				const dtMeshTile* pastTile = 0;
				const dtPoly* pastPoly = 0;
				m_nav->getTileAndPolyByRefUnsafe(pastRef, &pastTile, &pastPoly);
				
				// Get vertices and test overlap
				const int npb = pastPoly->vertCount;
				for (int k = 0; k < npb; ++k)
					dtVcopy(&pb[k*3], &pastTile->verts[pastPoly->verts[k]*3]);
				
				if (dtOverlapPolyPoly2D(pa,npa, pb,npb))
				{
					overlap = true;
					break;
				}
			}
			if (overlap)
				continue;
			
			// This poly is fine, store and advance to the poly.
			if (n < maxResult)
			{
				resultRef[n] = neighbourRef;
				if (resultParent)
					resultParent[n] = curRef;
				++n;
			}
			else
			{
				status |= DT_BUFFER_TOO_SMALL;
			}
			
			if (nstack < MAX_STACK)
			{
				stack[nstack++] = neighbourNode;
			}
		}
	}
	
	*resultCount = n;
	
	return status;
}


struct dtSegInterval
{
	dtPolyRef ref;
	short tmin, tmax;
};

static void insertInterval(dtSegInterval* ints, int& nints, const int maxInts,
						   const short tmin, const short tmax, const dtPolyRef ref)
{
	if (nints+1 > maxInts) return;
	// Find insertion point.
	int idx = 0;
	while (idx < nints)
	{
		if (tmax <= ints[idx].tmin)
			break;
		idx++;
	}
	// Move current results.
	if (nints-idx)
		memmove(ints+idx+1, ints+idx, sizeof(dtSegInterval)*(nints-idx));
	// Store
	ints[idx].ref = ref;
	ints[idx].tmin = tmin;
	ints[idx].tmax = tmax;
	nints++;
}

/// @par
///
/// If the @p segmentRefs parameter is provided, then all polygon segments will be returned. 
/// Otherwise only the wall segments are returned.
/// 
/// A segment that is normally a portal will be included in the result set as a 
/// wall if the @p filter results in the neighbor polygon becoomming impassable.
/// 
/// The @p segmentVerts and @p segmentRefs buffers should normally be sized for the 
/// maximum segments per polygon of the source navigation mesh.
/// 
dtStatus dtNavMeshQuery::getPolyWallSegments(dtPolyRef ref, const dtQueryFilter* filter,
											 float* segmentVerts, dtPolyRef* segmentRefs, int* segmentCount,
											 const int maxSegments) const
{
	dtAssert(m_nav);

	if (!segmentCount)
		return DT_FAILURE | DT_INVALID_PARAM;
	
	*segmentCount = 0;

	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if (dtStatusFailed(m_nav->getTileAndPolyByRef(ref, &tile, &poly)))
		return DT_FAILURE | DT_INVALID_PARAM;

	if (!filter || !segmentVerts || maxSegments < 0)
		return DT_FAILURE | DT_INVALID_PARAM;
	
	int n = 0;
	static const int MAX_INTERVAL = 16;
	dtSegInterval ints[MAX_INTERVAL];
	int nints;
	
	const bool storePortals = segmentRefs != 0;
	
	dtStatus status = DT_SUCCESS;
	
	for (int i = 0, j = (int)poly->vertCount-1; i < (int)poly->vertCount; j = i++)
	{
		// Skip non-solid edges.
		nints = 0;
		if (poly->neis[j] & DT_EXT_LINK)
		{
			// Tile border.
			for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
			{
				const dtLink* link = &tile->links[k];
				if (link->edge == j)
				{
					if (link->ref != 0)
					{
						const dtMeshTile* neiTile = 0;
						const dtPoly* neiPoly = 0;
						m_nav->getTileAndPolyByRefUnsafe(link->ref, &neiTile, &neiPoly);
						if (filter->passFilter(link->ref, neiTile, neiPoly))
						{
							insertInterval(ints, nints, MAX_INTERVAL, link->bmin, link->bmax, link->ref);
						}
					}
				}
			}
		}
		else
		{
			// Internal edge
			dtPolyRef neiRef = 0;
			if (poly->neis[j])
			{
				const unsigned int idx = (unsigned int)(poly->neis[j]-1);
				neiRef = m_nav->getPolyRefBase(tile) | idx;
				if (!filter->passFilter(neiRef, tile, &tile->polys[idx]))
					neiRef = 0;
			}

			// If the edge leads to another polygon and portals are not stored, skip.
			if (neiRef != 0 && !storePortals)
				continue;
			
			if (n < maxSegments)
			{
				const float* vj = &tile->verts[poly->verts[j]*3];
				const float* vi = &tile->verts[poly->verts[i]*3];
				float* seg = &segmentVerts[n*6];
				dtVcopy(seg+0, vj);
				dtVcopy(seg+3, vi);
				if (segmentRefs)
					segmentRefs[n] = neiRef;
				n++;
			}
			else
			{
				status |= DT_BUFFER_TOO_SMALL;
			}
			
			continue;
		}
		
		// Add sentinels
		insertInterval(ints, nints, MAX_INTERVAL, -1, 0, 0);
		insertInterval(ints, nints, MAX_INTERVAL, 255, 256, 0);
		
		// Store segments.
		const float* vj = &tile->verts[poly->verts[j]*3];
		const float* vi = &tile->verts[poly->verts[i]*3];
		for (int k = 1; k < nints; ++k)
		{
			// Portal segment.
			if (storePortals && ints[k].ref)
			{
				const float tmin = ints[k].tmin/255.0f; 
				const float tmax = ints[k].tmax/255.0f; 
				if (n < maxSegments)
				{
					float* seg = &segmentVerts[n*6];
					dtVlerp(seg+0, vj,vi, tmin);
					dtVlerp(seg+3, vj,vi, tmax);
					if (segmentRefs)
						segmentRefs[n] = ints[k].ref;
					n++;
				}
				else
				{
					status |= DT_BUFFER_TOO_SMALL;
				}
			}

			// Wall segment.
			const int imin = ints[k-1].tmax;
			const int imax = ints[k].tmin;
			if (imin != imax)
			{
				const float tmin = imin/255.0f; 
				const float tmax = imax/255.0f; 
				if (n < maxSegments)
				{
					float* seg = &segmentVerts[n*6];
					dtVlerp(seg+0, vj,vi, tmin);
					dtVlerp(seg+3, vj,vi, tmax);
					if (segmentRefs)
						segmentRefs[n] = 0;
					n++;
				}
				else
				{
					status |= DT_BUFFER_TOO_SMALL;
				}
			}
		}
	}
	
	*segmentCount = n;
	
	return status;
}

/// @par
///
/// @p hitPos is not adjusted using the height detail data.
///
/// @p hitDist will equal the search radius if there is no wall within the 
/// radius. In this case the values of @p hitPos and @p hitNormal are
/// undefined.
///
/// The normal will become unpredicable if @p hitDist is a very small number.
///
dtStatus dtNavMeshQuery::findDistanceToWall(dtPolyRef startRef, const float* centerPos, const float maxRadius,
											const dtQueryFilter* filter,
											float* hitDist, float* hitPos, float* hitNormal) const
{
	dtAssert(m_nav);
	dtAssert(m_nodePool);
	dtAssert(m_openList);
	
	// Validate input
	if (!m_nav->isValidPolyRef(startRef) ||
		!centerPos || !dtVisfinite(centerPos) ||
		maxRadius < 0 || !dtMathIsfinite(maxRadius) ||
		!filter || !hitDist || !hitPos || !hitNormal)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}
	
	m_nodePool->clear();
	m_openList->clear();
	
	dtNode* startNode = m_nodePool->getNode(startRef);
	dtVcopy(startNode->pos, centerPos);
	startNode->pidx = 0;
	startNode->cost = 0;
	startNode->total = 0;
	startNode->id = startRef;
	startNode->flags = DT_NODE_OPEN;
	m_openList->push(startNode);
	
	float radiusSqr = dtSqr(maxRadius);
	
	dtStatus status = DT_SUCCESS;
	
	while (!m_openList->empty())
	{
		dtNode* bestNode = m_openList->pop();
		bestNode->flags &= ~DT_NODE_OPEN;
		bestNode->flags |= DT_NODE_CLOSED;
		
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtPolyRef bestRef = bestNode->id;
		const dtMeshTile* bestTile = 0;
		const dtPoly* bestPoly = 0;
		m_nav->getTileAndPolyByRefUnsafe(bestRef, &bestTile, &bestPoly);
		
		// Get parent poly and tile.
		dtPolyRef parentRef = 0;
		const dtMeshTile* parentTile = 0;
		const dtPoly* parentPoly = 0;
		if (bestNode->pidx)
			parentRef = m_nodePool->getNodeAtIdx(bestNode->pidx)->id;
		if (parentRef)
			m_nav->getTileAndPolyByRefUnsafe(parentRef, &parentTile, &parentPoly);
		
		// Hit test walls.
		for (int i = 0, j = (int)bestPoly->vertCount-1; i < (int)bestPoly->vertCount; j = i++)
		{
			// Skip non-solid edges.
			if (bestPoly->neis[j] & DT_EXT_LINK)
			{
				// Tile border.
				bool solid = true;
				for (unsigned int k = bestPoly->firstLink; k != DT_NULL_LINK; k = bestTile->links[k].next)
				{
					const dtLink* link = &bestTile->links[k];
					if (link->edge == j)
					{
						if (link->ref != 0)
						{
							const dtMeshTile* neiTile = 0;
							const dtPoly* neiPoly = 0;
							m_nav->getTileAndPolyByRefUnsafe(link->ref, &neiTile, &neiPoly);
							if (filter->passFilter(link->ref, neiTile, neiPoly))
								solid = false;
						}
						break;
					}
				}
				if (!solid) continue;
			}
			else if (bestPoly->neis[j])
			{
				// Internal edge
				const unsigned int idx = (unsigned int)(bestPoly->neis[j]-1);
				const dtPolyRef ref = m_nav->getPolyRefBase(bestTile) | idx;
				if (filter->passFilter(ref, bestTile, &bestTile->polys[idx]))
					continue;
			}
			
			// Calc distance to the edge.
			const float* vj = &bestTile->verts[bestPoly->verts[j]*3];
			const float* vi = &bestTile->verts[bestPoly->verts[i]*3];
			float tseg;
			float distSqr = dtDistancePtSegSqr2D(centerPos, vj, vi, tseg);
			
			// Edge is too far, skip.
			if (distSqr > radiusSqr)
				continue;
			
			// Hit wall, update radius.
			radiusSqr = distSqr;
			// Calculate hit pos.
			hitPos[0] = vj[0] + (vi[0] - vj[0])*tseg;
			hitPos[1] = vj[1] + (vi[1] - vj[1])*tseg;
			hitPos[2] = vj[2] + (vi[2] - vj[2])*tseg;
		}
		
		for (unsigned int i = bestPoly->firstLink; i != DT_NULL_LINK; i = bestTile->links[i].next)
		{
			const dtLink* link = &bestTile->links[i];
			dtPolyRef neighbourRef = link->ref;
			// Skip invalid neighbours and do not follow back to parent.
			if (!neighbourRef || neighbourRef == parentRef)
				continue;
			
			// Expand to neighbour.
			const dtMeshTile* neighbourTile = 0;
			const dtPoly* neighbourPoly = 0;
			m_nav->getTileAndPolyByRefUnsafe(neighbourRef, &neighbourTile, &neighbourPoly);
			
			// Skip off-mesh connections.
			if (neighbourPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
				continue;
			
			// Calc distance to the edge.
			const float* va = &bestTile->verts[bestPoly->verts[link->edge]*3];
			const float* vb = &bestTile->verts[bestPoly->verts[(link->edge+1) % bestPoly->vertCount]*3];
			float tseg;
			float distSqr = dtDistancePtSegSqr2D(centerPos, va, vb, tseg);
			
			// If the circle is not touching the next polygon, skip it.
			if (distSqr > radiusSqr)
				continue;
			
			if (!filter->passFilter(neighbourRef, neighbourTile, neighbourPoly))
				continue;

			dtNode* neighbourNode = m_nodePool->getNode(neighbourRef);
			if (!neighbourNode)
			{
				status |= DT_OUT_OF_NODES;
				continue;
			}
			
			if (neighbourNode->flags & DT_NODE_CLOSED)
				continue;
			
			// Cost
			if (neighbourNode->flags == 0)
			{
				getEdgeMidPoint(bestRef, bestPoly, bestTile,
								neighbourRef, neighbourPoly, neighbourTile, neighbourNode->pos);
			}
			
			const float total = bestNode->total + dtVdist(bestNode->pos, neighbourNode->pos);
			
			// The node is already in open list and the new result is worse, skip.
			if ((neighbourNode->flags & DT_NODE_OPEN) && total >= neighbourNode->total)
				continue;
			
			neighbourNode->id = neighbourRef;
			neighbourNode->flags = (neighbourNode->flags & ~DT_NODE_CLOSED);
			neighbourNode->pidx = m_nodePool->getNodeIdx(bestNode);
			neighbourNode->total = total;
				
			if (neighbourNode->flags & DT_NODE_OPEN)
			{
				m_openList->modify(neighbourNode);
			}
			else
			{
				neighbourNode->flags |= DT_NODE_OPEN;
				m_openList->push(neighbourNode);
			}
		}
	}
	
	// Calc hit normal.
	dtVsub(hitNormal, centerPos, hitPos);
	dtVnormalize(hitNormal);
	
	*hitDist = dtMathSqrtf(radiusSqr);
	
	return status;
}

bool dtNavMeshQuery::isValidPolyRef(dtPolyRef ref, const dtQueryFilter* filter) const
{
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	dtStatus status = m_nav->getTileAndPolyByRef(ref, &tile, &poly);
	// If cannot get polygon, assume it does not exists and boundary is invalid.
	if (dtStatusFailed(status))
		return false;
	// If cannot pass filter, assume flags has changed and boundary is invalid.
	if (!filter->passFilter(ref, tile, poly))
		return false;
	return true;
}

/// @par
///
/// The closed list is the list of polygons that were fully evaluated during 
/// the last navigation graph search. (A* or Dijkstra)
/// 
bool dtNavMeshQuery::isInClosedList(dtPolyRef ref) const
{
	if (!m_nodePool) return false;
	
	dtNode* nodes[DT_MAX_STATES_PER_NODE];
	int n= m_nodePool->findNodes(ref, nodes, DT_MAX_STATES_PER_NODE);

	for (int i=0; i<n; i++)
	{
		if (nodes[i]->flags & DT_NODE_CLOSED)
			return true;
	}		

	return false;
}
