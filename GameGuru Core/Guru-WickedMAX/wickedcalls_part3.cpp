#ifdef PICKBVHTHREADED
bool WickedCall_GetPick2_OLD(float fMouseX, float fMouseY, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask)
{
	// Use wicked mouse pointer to determine intersection with solid geometry (for terrain and entity detection)
	// PE: We got hits from hidden objects like "widgets" , now ignore hidden objects in WICKEDREPO.
	bool bHitSuccess = false;

	//PE: Do not do anything if mouse is not over level editor.
	//if (bImGuiGotFocus) return bHitSuccess;

	// do not wipe out hover object detection when widget plane pass happens
	if (iLayerMask & GGRENDERLAYERS_NORMAL)
	{
		g_hovered_pobject = NULL;
		g_hovered_entity = 0;
	}

	//PE: Wicked Mouse is relative to windows pos. ImGui is relative to screen.
	RAY pickRay = wiRenderer::GetPickRay((long)fMouseX, (long)fMouseY, master.masterrenderer);

	// check scene first if flagged, then terrain which is naturally underneath
	if ((iLayerMask & GGRENDERLAYERS_NORMAL) != 0 || (iLayerMask & GGRENDERLAYERS_CURSOROBJECT) != 0 || (iLayerMask & GGRENDERLAYERS_WIDGETPLANE) != 0)
	{
		wiScene::PickResult hovered = wiScene::Pick(pickRay, RENDERTYPE_ALL, iLayerMask);
		if (hovered.entity > 0)
		{
			if ((iLayerMask & GGRENDERLAYERS_NORMAL) != 0)
			{
				sObject* pHitObject = m_ObjectManager.FindObjectFromWickedObjectEntityID(hovered.entity);
				if (pHitObject)
				{
					bool ObjectIsEntity(void* pTestObject);

					//PE: Only highlight if this is a gg entity.
					bool bIsEntity = ObjectIsEntity(pHitObject);
					if (bIsEntity)
					{
						if (iLayerMask & GGRENDERLAYERS_NORMAL)
						{
							g_hovered_pobject = pHitObject;
							g_hovered_entity = hovered.entity;
							g_hovered_dot_pobject = NULL;
							g_hovered_dot_entity = 0;
						}
					}
					else
					{
						//#define DOTARCSOBJECTID 70001+40000+21001
						if (pHitObject->dwObjectNumber > 110000 && pHitObject->dwObjectNumber < 131002)
						{
							if (iLayerMask & GGRENDERLAYERS_NORMAL)
							{
								g_hovered_dot_pobject = pHitObject;
								g_hovered_dot_entity = hovered.entity;
								g_bhovered_dot = true;
							}
						}
					}
				}
				else
				{
					if (iLayerMask & GGRENDERLAYERS_NORMAL)
					{
						g_hovered_dot_pobject = NULL;
						g_hovered_dot_entity = 0;
					}
				}
			}

			// return hit position
			*pOutX = hovered.position.x;
			*pOutY = hovered.position.y;
			*pOutZ = hovered.position.z;

			if (iLayerMask & GGRENDERLAYERS_NORMAL)
			{
				fLastHitPosition[0] = hovered.position.x;
				fLastHitPosition[1] = hovered.position.y;
				fLastHitPosition[2] = hovered.position.z;
			}

			// if normals needed
			if (pNormX)
			{
				*pNormX = hovered.normal.x;
				*pNormY = hovered.normal.y;
				*pNormZ = hovered.normal.z;
			}

			// optionally return actual object the cursor hovered over
			if (pHitEntity) *pHitEntity = hovered.entity;

			// report a hit
			bHitSuccess = true;
		}
	}
	if ((iLayerMask & GGRENDERLAYERS_TERRAIN) != 0)
	{
		float fDistToObjectHit = -1;
		if (bHitSuccess == true)
		{
			float fDX = *pOutX - CameraPositionX(0);
			float fDY = *pOutY - CameraPositionY(0);
			float fDZ = *pOutZ - CameraPositionZ(0);
			fDistToObjectHit = sqrt(fabs(fDX * fDX) + fabs(fDY * fDY) + fabs(fDZ * fDZ));
		}
		float pTerrOutX, pTerrOutY, pTerrOutZ, pTerrNormX, pTerrNormY, pTerrNormZ;
		if (GGTerrain::GGTerrain_RayCast(pickRay, &pTerrOutX, &pTerrOutY, &pTerrOutZ, &pTerrNormX, &pTerrNormY, &pTerrNormZ, 0))
		{
			fLastTerrainHitX = pTerrOutX, fLastTerrainHitY = pTerrOutY, fLastTerrainHitZ = pTerrOutZ;

			float fDX = pTerrOutX - CameraPositionX(0);
			float fDY = pTerrOutY - CameraPositionY(0);
			float fDZ = pTerrOutZ - CameraPositionZ(0);
			float fDist = sqrt(fabs(fDX * fDX) + fabs(fDY * fDY) + fabs(fDZ * fDZ));
			if (fDist < fDistToObjectHit || fDistToObjectHit == -1)
			{
				// if terrain closer than object hit, we register a terrain detection instead
				if (pHitEntity) *pHitEntity = 0;
				*pOutX = pTerrOutX;
				*pOutY = pTerrOutY;
				*pOutZ = pTerrOutZ;
				if (pNormX)
				{
					*pNormX = pTerrNormX;
					*pNormY = pTerrNormY;
					*pNormZ = pTerrNormZ;
				}
				bHitSuccess = true;
			}
		}
		else
		{
			fLastTerrainHitX = 0, fLastTerrainHitY = 0, fLastTerrainHitZ = 0;
		}
	}

	// no entity hovering over, but still want to move the cursor line in visual logic system
	if (g_hovered_entity == 0)
	{
		if (iLayerMask & GGRENDERLAYERS_NORMAL)
		{
			fLastHitPosition[0] = *pOutX;
			fLastHitPosition[1] = *pOutY;
			fLastHitPosition[2] = *pOutZ;
		}
	}

	// return success flag
	return bHitSuccess;
}
#endif

// PERF P.3 diagnostics: how often the per-frame editor pick actually runs the scene raycast vs
// reuses the cached result (see cache below). Read via GET_PERF_DATA -> PICK_REAL_RUNS/PICK_CACHE_HITS.
int g_pickRealRuns = 0;
int g_pickCacheHits = 0;
int g_pickMissMask = 0; // miss because layer/output pattern differed from cached (multi-mask thrash)
int g_pickMissRay  = 0; // miss because the ray inputs (cursor/camera) differed (motion or instability)

bool WickedCall_GetPick(float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask)
{
	XMFLOAT4 currentMouse = wiInput::GetPointer();

	// PERF (Stage P.3): the level editor calls this pick EVERY FRAME to find the entity under the
	// cursor (findentitycursorobj -> gridedit_mapediting). The underlying wiScene raycast tests the
	// whole scene (~21K objects on the island) and is the single biggest editor-CPU cost measured
	// (~5ms, "P2-mapediting"). The hovered result can only change when the pick RAY changes, i.e. when
	// the cursor or the camera moves. Memoise on the RAW ray inputs - screen pointer + camera
	// Eye/At/Up + layer mask + which outputs were requested. (Deliberately NOT the derived pick ray:
	// CameraComponent::Projection bakes in the per-frame TAA jitter, so the ray oscillates sub-pixel
	// every frame even when parked and would defeat the cache.) When nothing moved we replay the
	// cached outputs and skip the raycast entirely; the pick's global hover state (g_hovered_*) is
	// produced inside GetPick2 which we skip, so it persists unchanged - exactly correct when nothing
	// moved. Any cursor/camera motion changes the key and re-runs the real pick, so behaviour (and
	// pixels) are identical to before. Sub-pixel-only differences from TAA jitter are intentionally
	// ignored (they never change which object is hovered, and dropping them removes edge hover flicker).
	// Several distinct pick calls happen each frame (different layer masks / requested outputs), so a
	// single-slot cache thrashes (measured: near-100% mask misses). Keep a small per-key slot table so
	// every distinct pick keeps its own cached result and hits frame-to-frame while nothing moves.
	const wiScene::CameraComponent& cam = wiScene::GetCamera();
	const int ptrMask = (pOutX ? 1 : 0) | (pNormX ? 2 : 0) | (pHitEntity ? 4 : 0);
	struct PickCacheSlot
	{
		bool valid;
		int  layer, ptr;
		float mx, my, ex, ey, ez, ax, ay, az, ux, uy, uz;
		bool res;
		float ox, oy, oz, nx, ny, nz;
		uint64_t ent;
	};
	static PickCacheSlot slots[8] = {};
	static int nextSlot = 0;

	// look for a slot that matches this exact pick (mask + requested outputs + ray inputs)
	for (int i = 0; i < 8; i++)
	{
		const PickCacheSlot& s = slots[i];
		if (!s.valid || s.layer != iLayerMask || s.ptr != ptrMask) continue;
		if (s.mx == currentMouse.x && s.my == currentMouse.y &&
			s.ex == cam.Eye.x && s.ey == cam.Eye.y && s.ez == cam.Eye.z &&
			s.ax == cam.At.x  && s.ay == cam.At.y  && s.az == cam.At.z  &&
			s.ux == cam.Up.x  && s.uy == cam.Up.y  && s.uz == cam.Up.z)
		{
			if (pOutX) *pOutX = s.ox; if (pOutY) *pOutY = s.oy; if (pOutZ) *pOutZ = s.oz;
			if (pNormX) *pNormX = s.nx; if (pNormY) *pNormY = s.ny; if (pNormZ) *pNormZ = s.nz;
			if (pHitEntity) *pHitEntity = s.ent;
			g_pickCacheHits++;
			return s.res;
		}
	}
	g_pickMissRay++; // reached only when no slot for this key matched the current ray

	g_pickRealRuns++;
	bool res = WickedCall_GetPick2(currentMouse.x, currentMouse.y, pOutX, pOutY, pOutZ, pNormX, pNormY, pNormZ, pHitEntity, iLayerMask);

	// store into the slot already owning this (mask, output-pattern) if present, else the next round-robin
	int useIdx = -1;
	for (int i = 0; i < 8; i++) { if (slots[i].valid && slots[i].layer == iLayerMask && slots[i].ptr == ptrMask) { useIdx = i; break; } }
	if (useIdx < 0) { useIdx = nextSlot; nextSlot = (nextSlot + 1) & 7; }
	PickCacheSlot& s = slots[useIdx];
	s.valid = true; s.layer = iLayerMask; s.ptr = ptrMask; s.res = res;
	s.mx = currentMouse.x; s.my = currentMouse.y;
	s.ex = cam.Eye.x; s.ey = cam.Eye.y; s.ez = cam.Eye.z;
	s.ax = cam.At.x;  s.ay = cam.At.y;  s.az = cam.At.z;
	s.ux = cam.Up.x;  s.uy = cam.Up.y;  s.uz = cam.Up.z;
	s.ox = pOutX ? *pOutX : 0; s.oy = pOutY ? *pOutY : 0; s.oz = pOutZ ? *pOutZ : 0;
	s.nx = pNormX ? *pNormX : 0; s.ny = pNormY ? *pNormY : 0; s.nz = pNormZ ? *pNormZ : 0;
	s.ent = pHitEntity ? *pHitEntity : 0;
	return res;
}

bool WickedCall_SentRay(float originx, float originy, float originz, float directionx, float directiony, float directionz,float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask)
{
	//PE: Sent ray using wicked.
	XMFLOAT4 currentMouse = wiInput::GetPointer();
	RAY pickRay;
	XMFLOAT3 direction_inverse;
	pickRay.origin.x = originx;
	pickRay.origin.y = originy;
	pickRay.origin.z = originz;
	pickRay.direction.x = directionx;
	pickRay.direction.y = directiony;
	pickRay.direction.z = directionz;
	XMStoreFloat3(&direction_inverse, XMVectorDivide(XMVectorReplicate(1.0f), XMVectorSet(directionx, directiony, directionz,1.0f)));
	pickRay.direction_inverse = direction_inverse;
#ifdef PICKBVHTHREADED
	wiScene::PickResult hovered = wiScene::Pick(pickRay, RENDERTYPE_ALL, iLayerMask);
#else
	wiScene::PickResult hovered = wiScene::Pick(pickRay, RENDERTYPE_ALL, iLayerMask);
#endif
	if (hovered.entity > 0)
	{
		sObject* pHitObject = m_ObjectManager.FindObjectFromWickedObjectEntityID(hovered.entity);

		if (pHitObject)
		{
			bool ObjectIsEntity(void* pTestObject);

			//PE: Only highlight if this is a gg entity.
			bool bIsEntity = ObjectIsEntity(pHitObject);
			if (bIsEntity) {
				g_ray_pobject = pHitObject;
			}
		}
		else {
			g_ray_pobject = NULL;
		}

		// return hit position
		*pOutX = hovered.position.x;
		*pOutY = hovered.position.y;
		*pOutZ = hovered.position.z;

		fLastHitPosition[0] = hovered.position.x;
		fLastHitPosition[1] = hovered.position.y;
		fLastHitPosition[2] = hovered.position.z;

		// if normals needed
		if (pNormX)
		{
			*pNormX = hovered.normal.x;
			*pNormY = hovered.normal.y;
			*pNormZ = hovered.normal.z;
		}

		// optionally return actual object the cursor hovered over
		if (pHitEntity) *pHitEntity = hovered.entity;

		// report a hit
		return true;
	}
	else
	{
		// report a miss
		g_ray_pobject = NULL;
		return false;
	}
}

bool WickedCall_SentRay2(float originx, float originy, float originz, float directionx, float directiony, float directionz, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask)
{
	if (iLayerMask & GGRENDERLAYERS_NORMAL)
	{
		g_hovered_pobject = NULL;
		g_hovered_entity = 0;
	}

	RAY pickRay;
	XMFLOAT3 direction_inverse;
	pickRay.origin.x = originx;
	pickRay.origin.y = originy;
	pickRay.origin.z = originz;
	pickRay.direction.x = directionx;
	pickRay.direction.y = directiony;
	pickRay.direction.z = directionz;
	XMStoreFloat3(&direction_inverse, XMVectorDivide(XMVectorReplicate(1.0f), XMVectorSet(directionx, directiony, directionz, 1.0f)));
	pickRay.direction_inverse = direction_inverse;

	// check scene first if flagged, then terrain which is naturally underneath
	if ((iLayerMask & GGRENDERLAYERS_NORMAL) != 0 || (iLayerMask & GGRENDERLAYERS_CURSOROBJECT) != 0 || (iLayerMask & GGRENDERLAYERS_WIDGETPLANE) != 0)
	{
#ifdef PICKBVHTHREADED
		wiScene::PickResult hovered = wiScene::Pick(pickRay, RENDERTYPE_ALL, iLayerMask);
#else
		wiScene::PickResult hovered = wiScene::Pick(pickRay, RENDERTYPE_ALL, iLayerMask);
#endif
		if (hovered.entity > 0)
		{
			if ((iLayerMask & GGRENDERLAYERS_NORMAL) != 0)
			{
				sObject* pHitObject = m_ObjectManager.FindObjectFromWickedObjectEntityID(hovered.entity);
				if (pHitObject)
				{
					bool ObjectIsEntity(void* pTestObject);

					//PE: Only highlight if this is a gg entity.
					bool bIsEntity = ObjectIsEntity(pHitObject);
					if (bIsEntity)
					{
						if (iLayerMask & GGRENDERLAYERS_NORMAL)
						{
							g_hovered_pobject = pHitObject;
							g_hovered_entity = hovered.entity;
							g_hovered_dot_pobject = NULL;
							g_hovered_dot_entity = 0;
						}
					}
					else
					{
						//#define DOTARCSOBJECTID 70001+40000+21001
						if (pHitObject->dwObjectNumber > 110000 && pHitObject->dwObjectNumber < 131002)
						{
							if (iLayerMask & GGRENDERLAYERS_NORMAL)
							{
								g_hovered_dot_pobject = pHitObject;
								g_hovered_dot_entity = hovered.entity;
							}
						}
					}
				}
				else
				{
					if (iLayerMask & GGRENDERLAYERS_NORMAL)
					{
						g_hovered_dot_pobject = NULL;
						g_hovered_dot_entity = 0;
					}
				}
			}

			// return hit position
			*pOutX = hovered.position.x;
			*pOutY = hovered.position.y;
			*pOutZ = hovered.position.z;

			if (iLayerMask & GGRENDERLAYERS_NORMAL)
			{
				fLastHitPosition[0] = hovered.position.x;
				fLastHitPosition[1] = hovered.position.y;
				fLastHitPosition[2] = hovered.position.z;
			}

			// if normals needed
			if (pNormX)
			{
				*pNormX = hovered.normal.x;
				*pNormY = hovered.normal.y;
				*pNormZ = hovered.normal.z;
			}

			// optionally return actual object the cursor hovered over
			if (pHitEntity) *pHitEntity = hovered.entity;

			// report a hit
			return true;
		}
	}
	if ((iLayerMask & GGRENDERLAYERS_TERRAIN) != 0)
	{
		if (GGTerrain::GGTerrain_RayCast(pickRay, pOutX, pOutY, pOutZ, pNormX, pNormY, pNormZ, 0))
		{
			fLastTerrainHitX = *pOutX, fLastTerrainHitY = *pOutY, fLastTerrainHitZ = *pOutZ;
			return true;
		}
		else
		{
			fLastTerrainHitX = 0, fLastTerrainHitY = 0, fLastTerrainHitZ = 0;
		}

	}
	return false;
}

bool WickedCall_SentRay4(float originx, float originy, float originz, float directionx, float directiony, float directionz, float fDistanceOfRay, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, DWORD* pdwObjectNumberHit, bool bOpaqueOnly)
{
	// ray cast specifically used by game loop to find accurate position of animating objects (performant?)
	RAY pickRay;
	XMFLOAT3 direction_inverse;
	pickRay.origin.x = originx;
	pickRay.origin.y = originy;
	pickRay.origin.z = originz;
	pickRay.direction.x = directionx;
	pickRay.direction.y = directiony;
	pickRay.direction.z = directionz;
	XMStoreFloat3(&direction_inverse, XMVectorDivide(XMVectorReplicate(1.0f), XMVectorSet(directionx, directiony, directionz, 1.0f)));
	pickRay.direction_inverse = direction_inverse;
	uint32_t checkType = RENDERTYPE_ALL;
	//PE: @Lee we have no checks on transparent objects, we cant shoot glass, no impact effects , no killing pradator ...
	if (bOpaqueOnly == true) checkType = RENDERTYPE_OPAQUE | RENDERTYPE_TRANSPARENT;
#ifdef PICKBVHTHREADED
	wiScene::PickResult hit = wiScene::Pick(pickRay, checkType, GGRENDERLAYERS_NORMAL);
#else
	wiScene::PickResult hit = wiScene::Pick(pickRay, checkType, GGRENDERLAYERS_NORMAL);
#endif
	if (hit.entity > 0)
	{
		float fDX = hit.position.x - originx;
		float fDY = hit.position.y - originy;
		float fDZ = hit.position.z - originz;
		float fDistOfHit = sqrt(fabs(fDX*fDX)+fabs(fDY*fDY)+fabs(fDZ*fDZ));
		if (fDistOfHit <= fDistanceOfRay)
		{
			sObject* pHitObject = m_ObjectManager.FindObjectFromWickedObjectEntityID(hit.entity);
			if (pHitObject) *pdwObjectNumberHit = pHitObject->dwObjectNumber;
			*pOutX = hit.position.x;
			*pOutY = hit.position.y;
			*pOutZ = hit.position.z;
			if (pNormX)
			{
				*pNormX = hit.normal.x;
				*pNormY = hit.normal.y;
				*pNormZ = hit.normal.z;
			}
			return true;
		}
	}
	return false;
}

#ifdef PICKBVHTHREADED
bool WickedCall_SentRay4_ThreadSafe(float originx, float originy, float originz, float directionx, float directiony, float directionz, float fDistanceOfRay, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, DWORD* pdwObjectNumberHit, bool bOpaqueOnly)
{
	// ray cast specifically used by game loop to find accurate position of animating objects (performant?)
	RAY pickRay;
	XMFLOAT3 direction_inverse;
	pickRay.origin.x = originx;
	pickRay.origin.y = originy;
	pickRay.origin.z = originz;
	pickRay.direction.x = directionx;
	pickRay.direction.y = directiony;
	pickRay.direction.z = directionz;
	XMStoreFloat3(&direction_inverse, XMVectorDivide(XMVectorReplicate(1.0f), XMVectorSet(directionx, directiony, directionz, 1.0f)));
	pickRay.direction_inverse = direction_inverse;
	uint32_t checkType = RENDERTYPE_ALL;
	//PE: @Lee we have no checks on transparent objects, we cant shoot glass, no impact effects , no killing pradator ...
	if (bOpaqueOnly == true) checkType = RENDERTYPE_OPAQUE | RENDERTYPE_TRANSPARENT;
	wiScene::PickResult hit = wiScene::Pick(pickRay, checkType, GGRENDERLAYERS_NORMAL);
	if (hit.entity > 0)
	{
		float fDX = hit.position.x - originx;
		float fDY = hit.position.y - originy;
		float fDZ = hit.position.z - originz;
		float fDistOfHit = sqrt(fabs(fDX * fDX) + fabs(fDY * fDY) + fabs(fDZ * fDZ));
		if (fDistOfHit <= fDistanceOfRay)
		{
			sObject* pHitObject = m_ObjectManager.FindObjectFromWickedObjectEntityID(hit.entity);
			if (pHitObject) *pdwObjectNumberHit = pHitObject->dwObjectNumber;
			*pOutX = hit.position.x;
			*pOutY = hit.position.y;
			*pOutZ = hit.position.z;
			if (pNormX)
			{
				*pNormX = hit.normal.x;
				*pNormY = hit.normal.y;
				*pNormZ = hit.normal.z;
			}
			return true;
		}
	}
	return false;
}
#endif

bool WickedCall_SentRay3(float originx, float originy, float originz, float directionx, float directiony, float directionz, float fDistanceOfRay, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, DWORD* pdwObjectNumberHit)
{
	return WickedCall_SentRay4(originx, originy, originz, directionx, directiony, directionz, fDistanceOfRay, pOutX, pOutY, pOutZ, pNormX, pNormY, pNormZ, pdwObjectNumberHit, false);
}

void WickedCall_GetMouseDeltas(float* pfX, float* pfY)
{
	XMFLOAT4 currentMouse = wiInput::GetPointer();
	*pfX = currentMouse.x - g_lastMousePos.x;
	*pfY = currentMouse.y - g_lastMousePos.y;
	g_lastMousePos = currentMouse;
}

uint32_t WickedCall_GetTextureWidth(void* ptex)
{
	Texture* tex = (Texture*)ptex;
	return( tex->GetDesc().width);
}
uint32_t WickedCall_GetTextureHeight(void* ptex)
{
	Texture* tex = (Texture*)ptex;
	return(tex->GetDesc().height);
}

uint64_t WickedCall_GetFirstRootEntityID(sObject* pObject)
{
	//PE: Each frame has its own master object id.
	for (int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++)
	{
		if (pObject->ppFrameList && pObject->ppFrameList[iFrameIndex])
		{
			sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
			if (pFrame && pFrame->wickedobjindex > 0)
			{
				ObjectComponent* object = wiScene::GetScene().objects.GetComponent(pFrame->wickedobjindex);
				if (object) {
					return(pFrame->wickedobjindex);
				}
			}
		}
	}
	return(0);
}

int WickedCall_GetNumberOfRootEntityIDs(sObject* pObject)
{
	int iRootNumber = 0;
	for (int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++)
	{
		sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
		if (pFrame && pFrame->wickedobjindex > 0)
		{
			ObjectComponent* object = wiScene::GetScene().objects.GetComponent(pFrame->wickedobjindex);
			if (object) {
				iRootNumber++;
			}
		}
	}
	return(iRootNumber);
}

void WickedCall_SetSelectedObject(sObject* pObject)
{
	g_selected_entity = 0;
	g_selected_pobject = pObject;

	if (!pObject) {
		return;
	}
	uint64_t rootEntity = WickedCall_GetFirstRootEntityID(pObject);
	if (rootEntity > 0)
	{
		g_selected_entity = rootEntity;
		g_selected_pobject = pObject;
	}
	return;
}

void WickedCall_SetObjectHighlightColor(sObject* pObject, bool bHighlight, int highlightColorType)
{
	uint64_t rootEntity = WickedCall_GetFirstRootEntityID(pObject);
	if (rootEntity > 0) 
	{
		wiScene::ObjectComponent* pWickedObject = wiScene::GetScene().objects.GetComponent(rootEntity);
		if (bHighlight) 
		{
			if (pWickedObject) pWickedObject->SetUserStencilRef((EDITORSTENCILREF)highlightColorType);
		}
		else 
		{
			if (pWickedObject) pWickedObject->SetUserStencilRef(EDITORSTENCILREF_CLEAR);
		}
	}
	for (int iF = 0; iF < pObject->iFrameCount; iF++)
	{
		if (pObject->ppFrameList && pObject->ppFrameList[iF])
		{
			uint64_t objectEntity = pObject->ppFrameList[iF]->wickedobjindex;
			if (objectEntity > 0 && rootEntity != objectEntity)
			{
				wiScene::ObjectComponent* pWickedObject = wiScene::GetScene().objects.GetComponent(objectEntity);
				if (bHighlight) 
				{
					if (pWickedObject) pWickedObject->SetUserStencilRef((EDITORSTENCILREF)highlightColorType);
				}
				else 
				{
					if (pWickedObject) pWickedObject->SetUserStencilRef(EDITORSTENCILREF_CLEAR);
				}
			}
		}
	}
}

void WickedCall_SetObjectHighlightRed(sObject* pObject, bool bHighlight)
{
	return WickedCall_SetObjectHighlightColor (pObject, bHighlight, (int)EDITORSTENCILREF_HIGHLIGHT_OBJECT_RED);
}

void WickedCall_SetObjectHighlightBlue(sObject* pObject, bool bHighlight)
{
	return WickedCall_SetObjectHighlightColor (pObject, bHighlight, (int)EDITORSTENCILREF_HIGHLIGHT_OBJECT_BLUE);
}

void WickedCall_SetObjectHighlight(sObject* pObject, bool bHighlight)
{
	uint64_t rootEntity = WickedCall_GetFirstRootEntityID(pObject);
	if (rootEntity > 0) 
	{
		wiScene::ObjectComponent* pWickedObject = wiScene::GetScene().objects.GetComponent(rootEntity);
		if (bHighlight) 
		{
			if (pWickedObject) pWickedObject->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_OBJECT);
		}
		else 
		{
			if (pWickedObject)
				pWickedObject->SetUserStencilRef(EDITORSTENCILREF_CLEAR);
		}
	}
	
	for (int iF = 0; iF < pObject->iFrameCount; iF++)
	{
		if (pObject->ppFrameList && pObject->ppFrameList[iF])
		{
			uint64_t objectEntity = pObject->ppFrameList[iF]->wickedobjindex;
			if (objectEntity > 0 && rootEntity != objectEntity)
			{
				wiScene::ObjectComponent* pWickedObject = wiScene::GetScene().objects.GetComponent(objectEntity);
				if (bHighlight) {
					if (pWickedObject) pWickedObject->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_OBJECT);
				}
				else {
					if (pWickedObject) pWickedObject->SetUserStencilRef(EDITORSTENCILREF_CLEAR);
				}
			}
		}
	}

}


void WickedCall_DrawObjctBox_CHECK_IF_WE_HAVE_A_GG_COLLISION_PROBLEM(sObject* pObject, XMFLOAT4 color)
{
	if (!pObject) return;

	GGMATRIX matARotation;
	GGVECTOR3 box1;
	GGMATRIX matRotateX, matRotateY, matRotateZ;
	if (pObject->position.bFreeFlightRotation)
	{
		matARotation = pObject->position.matFreeFlightRotate;
	}
	else
	{
		GGMatrixRotationX(&matRotateX, GGToRadian(pObject->position.vecRotate.x));	// x rotation
		GGMatrixRotationY(&matRotateY, GGToRadian(pObject->position.vecRotate.y));	// y rotation
		GGMatrixRotationZ(&matRotateZ, GGToRadian(pObject->position.vecRotate.z));	// z rotation
		matARotation = matRotateX * matRotateY * matRotateZ;
	}
	if (pObject->position.bApplyPivot)
	{
		matARotation *= pObject->position.matPivot;
	}

	AABB aabb;
	aabb._min.x = pObject->collision.vecMin.x;
	aabb._min.y = pObject->collision.vecMin.y;
	aabb._min.z = pObject->collision.vecMin.z;
	aabb._max.x = pObject->collision.vecMax.x;
	aabb._max.y = pObject->collision.vecMax.y;
	aabb._max.z = pObject->collision.vecMax.z;

	box1.x = aabb._min.x; box1.y = aabb._min.y; box1.z = aabb._min.z;
	GGVec3TransformCoord(&box1, &box1, &matARotation);
	aabb._min.x = box1.x; aabb._min.y = box1.y; aabb._min.z = box1.z;
	box1.x = aabb._max.x; box1.y = aabb._max.y; box1.z = aabb._max.z;
	GGVec3TransformCoord(&box1, &box1, &matARotation);
	aabb._max.x = box1.x; aabb._max.y = box1.y; aabb._max.z = box1.z;
	
	aabb._min.x = (aabb._min.x * pObject->position.vecScale.x) + pObject->position.vecPosition.x;
	aabb._min.y = (aabb._min.y * pObject->position.vecScale.y) + pObject->position.vecPosition.y;
	aabb._min.z = (aabb._min.z * pObject->position.vecScale.z) + pObject->position.vecPosition.z;
	aabb._max.x = (aabb._max.x * pObject->position.vecScale.x) + pObject->position.vecPosition.x;
	aabb._max.y = (aabb._max.y * pObject->position.vecScale.y) + pObject->position.vecPosition.y;
	aabb._max.z = (aabb._max.z * pObject->position.vecScale.z) + pObject->position.vecPosition.z;

	XMFLOAT4X4 hoverBox;
	XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
	wiRenderer::DrawBox(hoverBox, color);
}


void WickedCall_DrawObjctBox(sObject* pObject, XMFLOAT4 color, bool bThickLine, bool ForceBox)
{
	if (!pObject) return;

	if(!bUseEditorOutlineSelection())
		ForceBox = true;
	if (!ForceBox || bImGuiInTestGame)
	{
		if (ObjectExist(pObject->dwObjectNumber))
		{
			g_ObjectHighlightList.push_back(pObject->dwObjectNumber);
			if(color.x==1.0 && color.y==0.0 && color.z==0.0)
				WickedCall_SetObjectHighlightRed(pObject, true);
			else
				WickedCall_SetObjectHighlight(pObject, true);
		}
		return;
	}
	
	if (color.w == 0.0) return; //PE: Disable box if no color.

	AABB aabb;
	aabb._min.x = pObject->collision.vecMin.x;
	aabb._min.y = pObject->collision.vecMin.y;
	aabb._min.z = pObject->collision.vecMin.z;
	aabb._max.x = pObject->collision.vecMax.x;
	aabb._max.y = pObject->collision.vecMax.y;
	aabb._max.z = pObject->collision.vecMax.z;

	XMFLOAT4X4 hoverBox;

	XMMATRIX rot;
#ifndef MATCHCLASSICROTATION
	//PE: For some reaon we need ZXY rotation ?
	rot = XMMatrixRotationZ(GGToRadian(pObject->position.vecRotate.z));
	rot = rot * XMMatrixRotationX(GGToRadian(pObject->position.vecRotate.x));
	rot = rot * XMMatrixRotationY(GGToRadian(pObject->position.vecRotate.y));
#else
	rot = XMMatrixRotationX(GGToRadian(pObject->position.vecRotate.x));
	rot = rot * XMMatrixRotationY(GGToRadian(pObject->position.vecRotate.y));
	rot = rot * XMMatrixRotationZ(GGToRadian(pObject->position.vecRotate.z));
#endif

	GGVECTOR3 vObjectCenter = pObject->collision.vecCentre;

	XMFLOAT3 ext = aabb.getHalfWidth();
	XMMATRIX sca = XMMatrixScaling(ext.x*pObject->position.vecScale.x, ext.y*pObject->position.vecScale.y, ext.z*pObject->position.vecScale.z);
	XMMATRIX tra = XMMatrixTranslation(pObject->collision.vecCentre.x*pObject->position.vecScale.x, pObject->collision.vecCentre.y*pObject->position.vecScale.y, pObject->collision.vecCentre.z*pObject->position.vecScale.z) * rot;

	XMStoreFloat4x4(&hoverBox, sca * tra);

	hoverBox._41 += pObject->position.vecPosition.x;
	hoverBox._42 += pObject->position.vecPosition.y;
	hoverBox._43 += pObject->position.vecPosition.z;

	wiRenderer::DrawBox(hoverBox, color);

	if (bThickLine)
	{
		float seperate_line = 0.05; //0.1;
		hoverBox._41 += seperate_line;
		hoverBox._42 += seperate_line;
		hoverBox._43 += seperate_line;
		wiRenderer::DrawBox(hoverBox, color);
		hoverBox._41 -= seperate_line * 2.0;
		hoverBox._42 -= seperate_line * 2.0;
		hoverBox._43 -= seperate_line * 2.0;
		wiRenderer::DrawBox(hoverBox, color);
		hoverBox._42 += seperate_line;
		wiRenderer::DrawBox(hoverBox, color);
		hoverBox._41 += seperate_line * 2.0;
		hoverBox._43 += seperate_line * 2.0;
		wiRenderer::DrawBox(hoverBox, color);
	}

}


void WickedCall_DrawPoint(float fx, float fy, float fz, float size, XMFLOAT4 color, bool bThickLine)
{

	AABB aabb;
	aabb._min.x = fx- size;
	aabb._min.y = fy- size;
	aabb._min.z = fz- size;
	aabb._max.x = fx+ size;
	aabb._max.y = fy+ size;
	aabb._max.z = fz+ size;

	XMFLOAT4X4 hoverBox;

	XMFLOAT3 ext = aabb.getHalfWidth();
	XMMATRIX sca = XMMatrixScaling(ext.x*1.0, ext.y*1.0, ext.z*1.0);
	XMMATRIX tra = XMMatrixTranslation(0.0, 0.0, 0.0);

	XMStoreFloat4x4(&hoverBox, sca * tra);

	hoverBox._41 += fx;
	hoverBox._42 += fy;
	hoverBox._43 += fz;

	wiRenderer::DrawBox(hoverBox, color);

	if (bThickLine)
	{
		float seperate_line = 0.1;
		hoverBox._41 += seperate_line;
		hoverBox._42 += seperate_line;
		hoverBox._43 += seperate_line;
		wiRenderer::DrawBox(hoverBox, color);
		hoverBox._41 -= seperate_line * 2.0;
		hoverBox._42 -= seperate_line * 2.0;
		hoverBox._43 -= seperate_line * 2.0;
		wiRenderer::DrawBox(hoverBox, color);
		hoverBox._42 += seperate_line;
		wiRenderer::DrawBox(hoverBox, color);
		hoverBox._41 += seperate_line * 2.0;
		hoverBox._43 += seperate_line * 2.0;
		wiRenderer::DrawBox(hoverBox, color);
	}

}

void WickedCall_DrawObjctBox_Color(sObject* pObject, float r, float g, float b, float a)
{
	XMFLOAT4 color = { r,g,b,a };
	WickedCall_DrawObjctBox(pObject, color);
}

float WickedCall_GetVRAMUsageMB( void )
{
	// real usage of THIS process on the device Wicked actually created (D3D12MA
	// budget) — the legacy DXGI EnumAdapters(0) query can watch the wrong GPU
	wiGraphics::GraphicsDevice* device = wiGraphics::GetDevice();
	if ( device == nullptr ) return 0.0f;
	wiGraphics::GraphicsDevice::MemoryUsage mem = device->GetMemoryUsage();
	return (float)( mem.usage / (1024.0 * 1024.0) );
}

void WickedCall_RenderEditorFunctions( void )
{
	// if shooter genre mode active, show all logic objects
	extern bool Shooter_Tools_Window;
	if (Shooter_Tools_Window == true)
	{
		// only logic highlights
		Wicked_Highlight_ClearAllObjects();
		Wicked_Highlight_AllLogicObjects();
		return;
	}

	// Remove all highlights
	if (g_ObjectHighlightList.size() > 0)
	{
		for (int i = 0; i < (int)g_ObjectHighlightList.size(); i++)
		{
			int obj = g_ObjectHighlightList[i];
			if (obj > 0)
			{
				if (ObjectExist(obj))
				{
					sObject* pObject = (sObject*)GetObjectsInternalData(obj);
					if (pObject)
						WickedCall_SetObjectHighlight(pObject, false);
				}
			}
		}
		g_ObjectHighlightList.clear();
	}

	// exit early if not in gridedit selection mode, in procedural level creation mode or in test level mode
	if (iGetgrideditselect() != 5)
		return;
	if (bProceduralLevel)
		return;
	if (bImGuiInTestGame)
		return;

	// highlight selected object too
	if (g_selected_pobject) 
	{
		WickedCall_DrawObjctBox(g_selected_pobject, XMFLOAT4(0.25f, 1.0f, 0.25f, 0.5f));
	}

	// highlight editor selection too
	if (g_selected_editor_object) 
	{
		try
		{
			if (ObjectExist(g_selected_editor_objectID))
			{
				int iObjNo = g_selected_editor_object->dwObjectNumber;
				if (iObjNo > 0 && iObjNo <= 300000)
				{
					WickedCall_DrawObjctBox(g_selected_editor_object, g_selected_editor_color);
					WickedCall_DrawObjctBox(g_selected_editor_object, g_selected_editor_color, true, true);
				}
			}
		}
		catch(...)
		{
			// error trapping in case object is deleted elsewhere!
			g_selected_editor_object = NULL;
			g_selected_editor_objectID = 0;
		}
	}

	// highlight highlighted object too
	if (g_highlight_pobject) 
	{
		#ifdef ONLY_USE_OUTLINE_HIGHLIGHT
		WickedCall_DrawObjctBox(g_highlight_pobject, XMFLOAT4(1.0f, 0.75f, 0.0f, 0.85f), true, false);
		#else
		WickedCall_DrawObjctBox(g_highlight_pobject, XMFLOAT4(1.0f, 0.75f, 0.0f, 0.85f), true, true);
		#endif
	}

	// process highlights for rubber banded objects, selections and locked objects
	Wicked_Highlight_Rubberband();
	Wicked_Highlight_Selection();
	Wicked_Highlight_LockedList();
}

void Wicked_Update_Shadows(void *voidvisual);
uint64_t WickedCall_AddLight(int iLightType)
{
	Entity light = wiScene::GetScene ( ).Entity_CreateLight ( "light", XMFLOAT3 ( 0, 0, 0 ), XMFLOAT3 ( 1, 1, 1 ), 1, 500 );
	LightComponent* lightComponent = wiScene::GetScene ( ).lights.GetComponent ( light );
	lightComponent->_flags = 0;
	lightComponent->SetType ( (wiScene::LightComponent::LightType)iLightType );
	lightComponent->BackCompatSetEnergy(60);
	Wicked_Update_Shadows(NULL);
	return light;
}

void WickedCall_DeleteLight(uint64_t wickedlightindex)
{
	wiScene::GetScene().Entity_Remove(wickedlightindex);
	//PE: When creating light also update probes.
	WickedCall_UpdateProbes();

	
}

//PE: Calculate needed light textures.
int WickedCall_Get2DShadowLights(void)
{
	wiScene::Scene* pScene = &wiScene::GetScene();
	int lights = pScene->lights.GetCount();
	int shadows = 0;
	for (int i = 0; i < lights; i++)
	{
		if (pScene->lights[i].GetType() == ENTITY_TYPE_SPOTLIGHT)
		{
			if (pScene->lights[i].IsCastingShadow()) shadows++;
		}
	}
	return(shadows);
}

int WickedCall_GetCubeShadowLights(bool bDebug)
{
	wiScene::Scene* pScene = &wiScene::GetScene();
	int lights = pScene->lights.GetCount();
	int shadows = 0;
	for (int i = 0; i < lights; i++)
	{
		if (pScene->lights[i].GetType() == ENTITY_TYPE_POINTLIGHT)
		{
			if (pScene->lights[i].IsCastingShadow())
			{
				shadows++;
				if (bDebug)
				{
					XMFLOAT4X4 hoverBox;
					AABB aabb = pScene->aabb_lights[i];
					XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
					XMFLOAT4 color = { 1,0,0,1 };
					wiRenderer::DrawBox(hoverBox, color);
				}
			}
		}
	}
	return(shadows);
}

int WickedCall_GetSpotShadowLights(bool bDebug)
{
	wiScene::Scene* pScene = &wiScene::GetScene();
	int lights = pScene->lights.GetCount();
	int shadows = 0;
	for (int i = 0; i < lights; i++)
	{
		if (pScene->lights[i].GetType() == ENTITY_TYPE_SPOTLIGHT)
		{
			if (pScene->lights[i].IsCastingShadow())
			{
				shadows++;
				if (bDebug)
				{
					XMFLOAT4X4 hoverBox;
					AABB aabb = pScene->aabb_lights[i];
					XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
					XMFLOAT4 color = { 1,0,0,1 };
					wiRenderer::DrawBox(hoverBox, color);
				}
			}
		}
	}
	return(shadows);
}


void WickedCall_UpdateLight(uint64_t wickedlightindex, float fX, float fY, float fZ, float fAX, float fAY, float fAZ, float fRange, float fSpotRadius, int iColR, int iColG, int iColB, bool bCastShadow)
{
	bool bLightScapeHasChangedEnoughForEnvProbeUpdate = false;

	LightComponent* lightComponent = wiScene::GetScene ( ).lights.GetComponent ( wickedlightindex );
	lightComponent->SetCastShadow ( bCastShadow );
	lightComponent->outerConeAngle = GGToRadian(fSpotRadius);
	lightComponent->color = XMFLOAT3((float)iColR / 255.0f, (float)iColG / 255.0f, (float)iColB / 255.0f);

	// DX12 PBR has two major changes from DX11 that reduce perceived brightness:
	// 1) Inverse-square attenuation (1/d²) — DX11 used simple (1-d²/r²)² with no 1/d²
	// 2) Lambertian diffuse normalization (1/PI) — DX11 omitted this
	// Scale intensity with range²×PI/4 to compensate for both factors, targeting
	// equivalent DX11 brightness at half the light's range.
	if (fRange > 0.1f)
	{
		float fIntensity = fRange * fRange * 0.785f; // PI/4 ≈ 0.785
		if (fIntensity < 600.0f) fIntensity = 600.0f;
		if (fIntensity > 60000.0f) fIntensity = 60000.0f;
		if (lightComponent->GetType() == LightComponent::SPOT)
		{
			fIntensity = fRange * fRange * 7.85f; // spots need ~10x more (cone focus)
			if (fIntensity < 6000.0f) fIntensity = 6000.0f;
			if (fIntensity > 60000.0f) fIntensity = 60000.0f;
		}
		lightComponent->intensity = fIntensity;
	}

	// for now, only change env probe if light on/off
	TransformComponent* transformLight = wiScene::GetScene ().transforms.GetComponent (wickedlightindex);
	bool bDetectOnOffNotRangeChange = false;
	if (lightComponent->range != fRange)
	{
		if (lightComponent->range > 1 && fRange <= 1) bDetectOnOffNotRangeChange = true;
		if (lightComponent->range <= 1 && fRange > 1) bDetectOnOffNotRangeChange = true;
		lightComponent->range = fRange;
	}
	float fCurrentX = transformLight->GetPosition().x;
	if (fCurrentX <= -999999.0 && fX > -999999.0) bDetectOnOffNotRangeChange = true;
	if (fCurrentX > -999999.0 && fX <= -999999.0) bDetectOnOffNotRangeChange = true;
	if (bDetectOnOffNotRangeChange == true)
	{
		bLightScapeHasChangedEnoughForEnvProbeUpdate = true;
	}

	transformLight->ClearTransform();
	transformLight->Translate(XMFLOAT3(fX, fY, fZ));

	if (lightComponent->GetType() == LightComponent::SPOT)
	{
		// fAX, fAY, fAZ is a directional normal pointing in direction of spot light
		XMMATRIX rot;
		rot = XMMatrixRotationX(GGToRadian(fAX-90.0)); //Match the spot light object.
		rot = rot * XMMatrixRotationY(GGToRadian(fAY));
		rot = rot * XMMatrixRotationZ(GGToRadian(fAZ));
		XMVECTOR S;
		XMVECTOR R;
		XMVECTOR T;
		XMMatrixDecompose(&S, &R, &T, rot);
		transformLight->Rotate(R);
	}
	transformLight->SetDirty();

	// when light-scape changes, refresh env probes (performance hit here)
	if (bLightScapeHasChangedEnoughForEnvProbeUpdate == true)
	{
		extern bool g_bLightProbeInstantChange;
		g_bLightProbeInstantChange = true;
	}
}

void WickedCall_UpdateProbes(void)
{
	GGTerrain::ggterrain_extra_params.bUpdateProbes = true;
}

void WickedCall_EnableCameraLight(bool On)
{
	if (On)
	{
		if (!g_entityCameraLight)
		{
			g_entityCameraLight = WickedCall_AddLight(2);
			g_entityCameraLight = wiScene::GetScene().Entity_CreateLight("cameraLight", XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), 8, 2900);
			LightComponent* lightCamera = wiScene::GetScene().lights.GetComponent(g_entityCameraLight);
			lightCamera->SetCastShadow(false); //PE: No need for shadows its a editor light.
			lightCamera->color = XMFLOAT3(0.65, 0.65, 0.65); //PE: Dont make it to bright.
		}
	}
	else {
		if (g_entityCameraLight)
		{
			WickedCall_DeleteLight(g_entityCameraLight);
			g_entityCameraLight = NULL;
		}
	}
}

void WickedCall_EnableThumbLight(bool On)
{
	if (On)
	{
		if (!g_entityThumbLight)
		{
			//g_entityThumbLight = WickedCall_AddLight(2);
			g_entityThumbLight = wiScene::GetScene().Entity_CreateLight("thumbLight", XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), 8, 2900);
			LightComponent* lightCamera = wiScene::GetScene().lights.GetComponent(g_entityThumbLight);
			lightCamera->SetCastShadow(false); //PE: No need for shadows its a editor light.
			lightCamera->color = XMFLOAT3(0.65, 0.65, 0.65); //PE: Dont make it to bright.
			g_bLightShaftState = GetLightShaftState();
			g_bLensFlareState = GetLensFlareState();
		}
		if (!g_entityThumbLight2)
		{
			g_entityThumbLight2 = wiScene::GetScene().Entity_CreateLight("thumbLight2", XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), 8, 2900);
			LightComponent* lightCamera = wiScene::GetScene().lights.GetComponent(g_entityThumbLight2);
			lightCamera->SetCastShadow(false); //PE: No need for shadows its a editor light.
			lightCamera->color = XMFLOAT3(0.65, 0.65, 0.65); //PE: Dont make it to bright.
			g_bLightShaftState = GetLightShaftState();
			g_bLensFlareState = GetLensFlareState();
		}
		SetLightShaftState(false);
		SetLensFlareState(false);
	}
	else 
	{
		if (g_entityThumbLight)
		{
			WickedCall_DeleteLight(g_entityThumbLight);
			g_entityThumbLight = NULL;
		}
		if (g_entityThumbLight2)
		{
			WickedCall_DeleteLight(g_entityThumbLight2);
			g_entityThumbLight2 = NULL;
		}
		SetLightShaftState(g_bLightShaftState);
		SetLensFlareState(g_bLensFlareState);
	}
}

void WickedCall_SetEditorCameraLight(bool bSwitchOn)
{
	LightComponent* lightCamera = wiScene::GetScene ( ).lights.GetComponent ( g_entityCameraLight );
	if (lightCamera)
	{
		if (bSwitchOn == true)
			lightCamera->color = XMFLOAT3(1, 1, 1);
		else
			lightCamera->color = XMFLOAT3(0, 0, 0);
	}
}

void WickedCall_SetSpriteBoundBox(bool bShow,float fX1, float fY1,float fX2, float fY2)
{
	//PE: Wicked sprite do not support DPI and placement is wrong. like if you set windows DPI to 150%.
	//PE: Use imgui to draw it.
	void DrawRubberBand(float fx, float fy, float fx2, float fy2);
	if (bShow == true)
	{
		float fMouseCenterOffset = 6.0f;
		fX1 -= fMouseCenterOffset;
		fY1 -= fMouseCenterOffset;
		fX2 -= fMouseCenterOffset;
		fY2 -= fMouseCenterOffset;
		DrawRubberBand(fX1, fY1, fX2, fY2);
	}
}

void WickedCall_SetSunDirection(float fAx, float fAy, float fAz)
{
	TransformComponent* transformSunLight = wiScene::GetScene().transforms.GetComponent(g_entitySunLight);
	transformSunLight->ClearTransform();
	XMFLOAT3 rotationinrads;
	rotationinrads.x = fAx * (3.141592654f / 180.0f);
	rotationinrads.y = fAy * (3.141592654f / 180.0f);
	rotationinrads.z = fAz * (3.141592654f / 180.0f);
	transformSunLight->RotateRollPitchYaw(rotationinrads);

	// a moved sun makes frozen staggered-cascade contents stale; change-latched
	// because this is called on every visuals apply
	static float s_lastSunAx = -99999.0f, s_lastSunAy = -99999.0f, s_lastSunAz = -99999.0f;
	if (fAx != s_lastSunAx || fAy != s_lastSunAy || fAz != s_lastSunAz)
	{
		s_lastSunAx = fAx; s_lastSunAy = fAy; s_lastSunAz = fAz;
		wi::renderer::InvalidateDelayedShadowCascades();
	}
}

void WickedCall_SetSunColors(float fRed, float fGreen, float fBlue,float fEnergy,float fFov, float fShadowBias)
{
	LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	lightSun->color.x = fRed;
	lightSun->color.y = fGreen;
	lightSun->color.z = fBlue;
	lightSun->intensity = fEnergy;
	lightSun->outerConeAngle = fFov;
}
#include "master.h"

void WickedCall_SetVisualizerEnabled(bool bVisualizer)
{
	LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	lightSun->SetVisualizerEnabled(bVisualizer);

}

void WickedCall_SunSetVolumetricsEnabled(bool bVolumetrics)
{
	LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	lightSun->SetVolumetricsEnabled(bVolumetrics);
}

void WickedCall_SunSetSetStatic(bool bSetStatic)
{
	LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	lightSun->SetStatic(bSetStatic);
}
void WickedCall_SunSetRange(float fRange)
{
	LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	lightSun->range = fRange;
}

void WickedCall_SetTextureName(int obj, char *texturename)
{
	sObject* GetObjectData(int iID);
	sObject* pObject = GetObjectData(obj);
	if (pObject)
	{
		for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
		{
			sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
			if (pMesh && pMesh->pTextures)
			{
				strcpy(pMesh->pTextures[0].pName, texturename);
			}
		}
	}
}


void WickedCall_DrawImguiNow(void)
{
	// Phase 5: ImGui rendering is now done in MasterRenderer::Compose() via the DX12 backend.
	// This function is kept for compatibility but is a no-op — the actual rendering happens
	// through the WickedEngine render pipeline using ImGui_DX12_RenderBridge().

	// TODO: removed DX11 ImGui path
	// void ImGuiHook_RenderCall_Direct(void* ctxptr, void* d3dptr);
	// extern LPGGDEVICE            m_pD3D;
	// extern LPGGIMMEDIATECONTEXT  m_pImmediateContext;
	// ImGuiHook_RenderCall_Direct((void*)m_pImmediateContext, (void*)m_pD3D);
}

//PE: Changing the far plane in indoor levels can really boost the FPS.
void WickedCall_SetCameraFarPlanes(float farplane)
{
	float fNear = wiScene::GetCamera().zNearP;
	float fFar = farplane;
	float fCameraFov = wiScene::GetCamera().fov;
	wiScene::GetCamera().CreatePerspective((float)master.masterrenderer.GetLogicalWidth(), (float)master.masterrenderer.GetLogicalHeight(), fNear, fFar, fCameraFov);
}

void WickedCall_SetCameraFOV ( float fFOV )
{
	// from wicked camera
	float fNear = wiScene::GetCamera().zNearP;
	float fFar = wiScene::GetCamera().zFarP;

	// directly set camera FOV (ingame weapon FOV changes)
	float fCameraFov;
	extern bool bImGuiInTestGame;
	float fUsedFOV = fFOV;
	if (bImGuiInTestGame == false) fUsedFOV = 45;
	if (bImGuiInTestGame == true)
	{
		// when in game, weapon FOV correction
		fCameraFov = GGToRadian(fUsedFOV); // Oops - backwards logic, lower FOV needs lower angle passed in
	}
	else
	{
		// keep consistency with editor, even though reversed
		fCameraFov = XM_PI / (fUsedFOV / 15.0f); //Fit GG settings.
	}
	wiScene::GetCamera().CreatePerspective((float)master.masterrenderer.GetLogicalWidth(), (float)master.masterrenderer.GetLogicalHeight(), fNear, fFar, fCameraFov);
}

int WickedCall_GetSkinable(void)
{
	int iSkinable = 0;
	wiScene::Scene* pScene = &wiScene::GetScene();
	for (size_t i = 0; i < pScene->meshes.GetCount(); ++i)
	{
		Entity entity = pScene->meshes.GetEntity(i);
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		//MeshComponent * mesh = pScene->meshes[i];
		if (mesh)
		{
			if (mesh->IsSkinned() && pScene->armatures.Contains(mesh->armatureID))
			{
				iSkinable++;
			}
		}
	}
	return(iSkinable);
}

int WickedCall_GetSkinableVisible(void)
{
	int iSkinable = 0;
	wiScene::Scene* pScene = &wiScene::GetScene();
	for (size_t i = 0; i < pScene->meshes.GetCount(); ++i)
	{
		Entity entity = pScene->meshes.GetEntity(i);
		wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		//MeshComponent * mesh = pScene->meshes[i];
		if (mesh)
		{
			if (mesh->IsRenderable() && mesh->IsSkinned() && pScene->armatures.Contains(mesh->armatureID))
			{
				iSkinable++;
			}
		}
	}
	return(iSkinable);
}

void WickedCall_SetShadowRange(float ShadowFar)
{
	fWickedCallShadowFarPlane = ShadowFar;

	// DX11 parity (2026-07-18): the old engine consumed fWickedCallShadowFarPlane
	// inside CreateDirLightShadowCams with hand-tuned GGREDUCED cascade splits
	// (WickedRepo wiRenderer.cpp: 0/380/950/7500/30000/farPlane, 5 cascades).
	// The new engine ignores that global entirely — cascades come from each
	// light's cascade_distances, whose stock default {8,80,800} is ~20 metres
	// in GG's inch-scaled world. That left DX12 shadows stopping just past the
	// nearest trees. Recreate the production splits on the sun light here —
	// this is the same choke point production used (visuals apply/level load).
	wiScene::LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	if (lightSun)
	{
		float farCascade = ShadowFar;
		if (farCascade < 30001.0f) farCascade = 30001.0f;
		lightSun->cascade_distances = { 380.0f, 950.0f, 7500.0f, 30000.0f, farCascade };

		// changed splits make frozen staggered-cascade contents stale;
		// change-latched because this is called on every visuals apply
		static float s_lastFarCascade = -1.0f;
		if (farCascade != s_lastFarCascade)
		{
			s_lastFarCascade = farCascade;
			wi::renderer::InvalidateDelayedShadowCascades();
		}
	}
}


bool bCubesVisible = false; //PE: Default off.
void WickedCall_DisplayCubes(bool Visible)
{
	if (Visible) {
		bCubesVisible = Visible;
		return;
	}
	if (bCubesVisible && !Visible)
	{
		bCubesVisible = Visible;
		void set_terrain_sculpt_mode(int mode);
		void set_terrain_edit_mode(int mode);
		void clear_highlighted_tree(void);
		int get_terrain_sculpt_mode(void);
		extern int iLastTerrainSculptMode;
		if(iLastTerrainSculptMode == -1)
			iLastTerrainSculptMode = get_terrain_sculpt_mode();
		set_terrain_sculpt_mode(0); // GGTERRAIN_SCULPT_NONE; Disable terrain sculpt circle.
		set_terrain_edit_mode(0); // GGTERRAIN_EDIT_NONE; Disable terrain paint circle.
		clear_highlighted_tree();

		//PE: Not sure if we are going to use the below for grass ? so keep it here.

		Scene& scene = wiScene::GetScene();
		for (unsigned int i = 0; i < 32; i++)
		{
			Entity entity;
			TransformComponent* transform = NULL;

			entity = scene.Entity_FindByName("CubeDisplay_" + std::to_string(i));
			if (entity)
			{
				transform = scene.transforms.GetComponent(entity);
				if (transform)
				{
					transform->ClearTransform();
					transform->Translate(XMFLOAT3(-100000, -100000, -100000));
				}
			}
		}
	}
}

void WickedCall_CreateReflectionProbe(float x, float y, float z,char *name,float size)
{
	// no code was here, can we delete all calls to this function ?
}

void WickedCall_MoveReflectionProbe(float x, float y, float z, char *name, float size)
{
	// no code was here, can we delete all calls to this function ?
}

void WickedCall_DeleteReflectionProbe(char *name)
{
	// no code was here, can we delete all calls to this function ?
}

//PE: Capsule need more work. just some test code.
void WickedCall_DrawObjctCapsule(sObject* pObject, XMFLOAT4 color)
{
	if (!pObject) return;

	AABB aabb;
	aabb._min.x = pObject->collision.vecMin.x;
	aabb._min.y = pObject->collision.vecMin.y;
	aabb._min.z = pObject->collision.vecMin.z;
	aabb._max.x = pObject->collision.vecMax.x;
	aabb._max.y = pObject->collision.vecMax.y;
	aabb._max.z = pObject->collision.vecMax.z;

	XMFLOAT4X4 hoverBox;

	XMMATRIX rot;
#ifndef MATCHCLASSICROTATION
	//PE: For some reaon we need ZXY rotation ?
	rot = XMMatrixRotationZ(GGToRadian(pObject->position.vecRotate.z));
	rot = rot * XMMatrixRotationX(GGToRadian(pObject->position.vecRotate.x));
	rot = rot * XMMatrixRotationY(GGToRadian(pObject->position.vecRotate.y));
#else
	rot = XMMatrixRotationX(GGToRadian(pObject->position.vecRotate.x));
	rot = rot * XMMatrixRotationY(GGToRadian(pObject->position.vecRotate.y));
	rot = rot * XMMatrixRotationZ(GGToRadian(pObject->position.vecRotate.z));
#endif
	GGVECTOR3 vObjectCenter = pObject->collision.vecCentre;

	XMFLOAT3 ext = aabb.getHalfWidth();
	XMMATRIX sca = XMMatrixScaling(ext.x*pObject->position.vecScale.x, ext.y*pObject->position.vecScale.y, ext.z*pObject->position.vecScale.z);
	XMMATRIX tra = XMMatrixTranslation(pObject->collision.vecCentre.x*pObject->position.vecScale.x, pObject->collision.vecCentre.y*pObject->position.vecScale.y, pObject->collision.vecCentre.z*pObject->position.vecScale.z) * rot;

	XMStoreFloat4x4(&hoverBox, sca * tra);
	XMFLOAT3 pos = XMFLOAT3(pObject->position.vecPosition.x, pObject->position.vecPosition.y, pObject->position.vecPosition.z);
	XMFLOAT3 tip = pos;
	//tip.x += 10.0f;
	tip.y += 1.0f;
	//tip.z += 10.0f;

	CAPSULE capsule = CAPSULE(pos, tip, 40.0f);
	wiRenderer::DrawCapsule(capsule, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) );
}
void CapsuleTest(void)
{
	if (g_selected_editor_object)
	{
		if (ObjectExist(g_selected_editor_objectID) == 1)
		{
			WickedCall_DrawObjctCapsule(g_selected_editor_object, g_selected_editor_color);
		}
	}
}

void WickedCall_SetLDSSkinningEnabled(bool enabled)
{
	//wiRenderer::SetLDSSkinningEnabled(enabled); // removed from wi::renderer
}

void WickedCall_SetLightShaftParameters(float density, float weight, float decay, float exposure)
{
	//LB: master.masterrenderer.setLightShaftValues(density, weight, decay, exposure);
}

void WickedCall_GetLightShaftParameters(float& density, float& weight, float& decay, float& exposure)
{
	//LB: density = master.masterrenderer.GetLightShaftDensity();
	//LB: weight = master.masterrenderer.GetLightShaftWeight();
	//LB: decay = master.masterrenderer.GetLightShaftDecay();
	//LB: exposure = master.masterrenderer.GetLightShaftExposure();
}


constexpr DXGI_FORMAT _ConvertFormat(wiGraphics::Format value)
{
	switch (value)
	{
	case wiGraphics::Format::UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
		break;
	case wiGraphics::Format::R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case wiGraphics::Format::R32G32B32A32_UINT:
		return DXGI_FORMAT_R32G32B32A32_UINT;
		break;
	case wiGraphics::Format::R32G32B32A32_SINT:
		return DXGI_FORMAT_R32G32B32A32_SINT;
		break;
	case wiGraphics::Format::R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case wiGraphics::Format::R32G32B32_UINT:
		return DXGI_FORMAT_R32G32B32_UINT;
		break;
	case wiGraphics::Format::R32G32B32_SINT:
		return DXGI_FORMAT_R32G32B32_SINT;
		break;
	case wiGraphics::Format::R16G16B16A16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case wiGraphics::Format::R16G16B16A16_UNORM:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case wiGraphics::Format::R16G16B16A16_UINT:
		return DXGI_FORMAT_R16G16B16A16_UINT;
		break;
	case wiGraphics::Format::R16G16B16A16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_SNORM;
		break;
	case wiGraphics::Format::R16G16B16A16_SINT:
		return DXGI_FORMAT_R16G16B16A16_SINT;
		break;
	case wiGraphics::Format::R32G32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
		break;
	case wiGraphics::Format::R32G32_UINT:
		return DXGI_FORMAT_R32G32_UINT;
		break;
	case wiGraphics::Format::R32G32_SINT:
		return DXGI_FORMAT_R32G32_SINT;
		break;
	//case wiGraphics::Format::R32G8X24_TYPELESS: // not in WickedEngine Format enum
	//	return DXGI_FORMAT_R32G8X24_TYPELESS;
	//	break;
	case wiGraphics::Format::D32_FLOAT_S8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		break;
	case wiGraphics::Format::R10G10B10A2_UNORM:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
		break;
	case wiGraphics::Format::R10G10B10A2_UINT:
		return DXGI_FORMAT_R10G10B10A2_UINT;
		break;
	case wiGraphics::Format::R11G11B10_FLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT;
		break;
	case wiGraphics::Format::R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case wiGraphics::Format::R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case wiGraphics::Format::R8G8B8A8_UINT:
		return DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case wiGraphics::Format::R8G8B8A8_SNORM:
		return DXGI_FORMAT_R8G8B8A8_SNORM;
		break;
	case wiGraphics::Format::R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_SINT;
		break;
	case wiGraphics::Format::R16G16_FLOAT:
		return DXGI_FORMAT_R16G16_FLOAT;
		break;
	case wiGraphics::Format::R16G16_UNORM:
		return DXGI_FORMAT_R16G16_UNORM;
		break;
	case wiGraphics::Format::R16G16_UINT:
		return DXGI_FORMAT_R16G16_UINT;
		break;
	case wiGraphics::Format::R16G16_SNORM:
		return DXGI_FORMAT_R16G16_SNORM;
		break;
	case wiGraphics::Format::R16G16_SINT:
		return DXGI_FORMAT_R16G16_SINT;
		break;
	//case wiGraphics::Format::R32_TYPELESS: // not in WickedEngine Format enum
	//	return DXGI_FORMAT_R32_TYPELESS;
	//	break;
	case wiGraphics::Format::D32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;
		break;
	case wiGraphics::Format::R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
		break;
	case wiGraphics::Format::R32_UINT:
		return DXGI_FORMAT_R32_UINT;
		break;
	case wiGraphics::Format::R32_SINT:
		return DXGI_FORMAT_R32_SINT;
		break;
	//case wiGraphics::Format::R24G8_TYPELESS: // not in WickedEngine Format enum
	//	return DXGI_FORMAT_R24G8_TYPELESS;
	//	break;
	case wiGraphics::Format::D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	case wiGraphics::Format::R8G8_UNORM:
		return DXGI_FORMAT_R8G8_UNORM;
		break;
	case wiGraphics::Format::R8G8_UINT:
		return DXGI_FORMAT_R8G8_UINT;
		break;
	case wiGraphics::Format::R8G8_SNORM:
		return DXGI_FORMAT_R8G8_SNORM;
		break;
	case wiGraphics::Format::R8G8_SINT:
		return DXGI_FORMAT_R8G8_SINT;
		break;
	//case wiGraphics::Format::R16_TYPELESS: // not in WickedEngine Format enum
	//	return DXGI_FORMAT_R16_TYPELESS;
	//	break;
	case wiGraphics::Format::R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
		break;
	case wiGraphics::Format::D16_UNORM:
		return DXGI_FORMAT_D16_UNORM;
		break;
	case wiGraphics::Format::R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
		break;
	case wiGraphics::Format::R16_UINT:
		return DXGI_FORMAT_R16_UINT;
		break;
	case wiGraphics::Format::R16_SNORM:
		return DXGI_FORMAT_R16_SNORM;
		break;
	case wiGraphics::Format::R16_SINT:
		return DXGI_FORMAT_R16_SINT;
		break;
	case wiGraphics::Format::R8_UNORM:
		return DXGI_FORMAT_R8_UNORM;
		break;
	case wiGraphics::Format::R8_UINT:
		return DXGI_FORMAT_R8_UINT;
		break;
	case wiGraphics::Format::R8_SNORM:
		return DXGI_FORMAT_R8_SNORM;
		break;
	case wiGraphics::Format::R8_SINT:
		return DXGI_FORMAT_R8_SINT;
		break;
	case wiGraphics::Format::BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM;
		break;
	case wiGraphics::Format::BC1_UNORM_SRGB:
		return DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case wiGraphics::Format::BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM;
		break;
	case wiGraphics::Format::BC2_UNORM_SRGB:
		return DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case wiGraphics::Format::BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM;
		break;
	case wiGraphics::Format::BC3_UNORM_SRGB:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case wiGraphics::Format::BC4_UNORM:
		return DXGI_FORMAT_BC4_UNORM;
		break;
	case wiGraphics::Format::BC4_SNORM:
		return DXGI_FORMAT_BC4_SNORM;
		break;
	case wiGraphics::Format::BC5_UNORM:
		return DXGI_FORMAT_BC5_UNORM;
		break;
	case wiGraphics::Format::BC5_SNORM:
		return DXGI_FORMAT_BC5_SNORM;
		break;
	case wiGraphics::Format::B8G8R8A8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case wiGraphics::Format::B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case wiGraphics::Format::BC6H_UF16:
		return DXGI_FORMAT_BC6H_UF16;
		break;
	case wiGraphics::Format::BC6H_SF16:
		return DXGI_FORMAT_BC6H_SF16;
		break;
	case wiGraphics::Format::BC7_UNORM:
		return DXGI_FORMAT_BC7_UNORM;
		break;
	case wiGraphics::Format::BC7_UNORM_SRGB:
		return DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	}
	return DXGI_FORMAT_UNKNOWN;
}

void Wicked_Memory_Use_Textures(void)
{
	void timestampactivity(int i, char* desc_s);
	char timestampMsg[1024];
	sprintf(timestampMsg, "WICKED TEXTURES:");
	timestampactivity(0, timestampMsg);

	//PE: Displaying everything directly is way better.
	wiScene::Scene* pScene = &wiScene::GetScene();
	long usedsize = 0;
	long usedFilesize = 0;
	auto size = pScene->materials.GetCount();
	std::vector<void*> already_registred; //PE: We reuse textures, so ignore when reused.
	for (int i = 0; i < size; i++)
	{
		for (int a = 0; a < 13; a++) //TEXTURESLOT_COUNT = 13
		{
			if (!pScene->materials[i].textures[a].name.empty())
			{
				wiResource image = pScene->materials[i].textures[a].resource;
				if (image.IsValid())
				{
					auto filedata = image.GetFileData().data();
					auto filesize = image.GetFileData().size();

					void *pmat = (void *)pScene->materials[i].textures[a].GetGPUResource();
						// TODO: MaterialGetSRV removed from GraphicsDevice
					ID3D11ShaderResourceView* lpTexture = (ID3D11ShaderResourceView*)nullptr; //wiGraphics::GetDevice()->MaterialGetSRV((void*)pmat);
					bool bAlreadyDisplayed = false;
					for(int b=0;b< already_registred.size();b++)
						if (already_registred[b] == (void*)lpTexture) { bAlreadyDisplayed = true; break; }
					if (!bAlreadyDisplayed)
					{
						already_registred.push_back((void*)lpTexture);

						wiGraphics::Texture texture = image.GetTexture();
						auto Height = texture.desc.height;
						auto Width = texture.desc.width;
						auto Format = texture.desc.format;
						TextureDesc imgdesc = texture.GetDesc();
							// _ConvertFormat was DX11-specific, cast format directly (values match DXGI_FORMAT)
						DXGI_FORMAT DxgiFormat = static_cast<DXGI_FORMAT>(Format);

						int getBitsPerPixel(int fmt);
						float bperpixel = 32;
						if (DxgiFormat != DXGI_FORMAT_UNKNOWN)
						{
							bperpixel = (float)getBitsPerPixel(DxgiFormat) / 8;
							if (bperpixel == 0.5) {
								bperpixel -= 0.125; // remove alpha count from BC1
							}
						}
						int addmipmapssize;

						if (imgdesc.mip_levels > 1) {
							addmipmapssize = (int)((float)(Width*Height) * bperpixel) / 1024 * imgdesc.array_size - 1; // Full mipmaps always give size -1.
							if (addmipmapssize <= 0) addmipmapssize = 0;
						}
						else {
							addmipmapssize = 0;
						}

						usedFilesize += filesize;
						usedsize += ((int)(((float)(Width*Height) * bperpixel) / 1024) * imgdesc.array_size) + addmipmapssize;

						std::string getImageformat(int fmt);
						sprintf(timestampMsg, "WList%d: (%ld,%ld) (%ld kb.+ mipm %ld kb.) filesize %ldkb mipmaps %d array %d format %s \"%s\" (*%ld)", i, Width, Height, (int)((float)(Width*Height) * bperpixel) / 1024 * imgdesc.array_size, addmipmapssize, (long)filesize / 1024.0, imgdesc.mip_levels, imgdesc.array_size, getImageformat(DxgiFormat).c_str(), pScene->materials[i].textures[a].name.c_str(), lpTexture);
						timestampactivity(0, timestampMsg);
					}
				}
			}
		}
	}
	sprintf(timestampMsg, "Total WICKED texture mem used: %ld (%.2fmb) (%.2fgb)", usedsize, (float)usedsize / 1024.0, (float)usedsize / 1024.0 / 1024.0);
	timestampactivity(0, timestampMsg);
	sprintf(timestampMsg, "Total WICKED filedata allocated used: %ld (%.2fmb) (%.2fgb)", usedFilesize, (float)usedFilesize / 1024.0, (float)usedFilesize / 1024.0 / 1024.0);
	timestampactivity(0, timestampMsg);
	sprintf(timestampMsg, "Total: %ld (%.2fmb) (%.2fgb)", usedsize+usedFilesize, (float)(usedsize+usedFilesize) / 1024.0, (float)(usedsize+usedFilesize) / 1024.0 / 1024.0);
	timestampactivity(0, timestampMsg);


}

void WickedCall_SetRenderTargetMouseFocus(bool focus)
{
	bRenderTargetHasFocus = focus;
}



void WickedCall_UpdateWaterColor(float red, float green, float blue)
{
	wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
	if (weather)
	{
		XMFLOAT4 oldColor = weather->oceanParameters.waterColor;
		XMFLOAT4 waterColor = XMFLOAT4(red / 255.0f, green / 255.0f, blue / 255.0f, oldColor.w);
		weather->oceanParameters.waterColor = waterColor;
	}
}


void WickedCall_UpdateTreeWind(float wind)
{
	wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
	if (weather)
	{
		//weather->tree_wind = wind; // removed from WeatherComponent
	}
}

void WickedCall_UpdateWaterHeight(float height)
{
	wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
	if (weather)
	{
		weather->oceanParameters.waterHeight = height;
	}
}

void WickedCall_RemoveObjectTextures(sObject* pObject)
{
	for (int i = 0; i < pObject->iMeshCount; i++)
	{
		sMesh* pMesh = pObject->ppMeshList[i];
		if (pMesh)
		{
			// Remove mesh texures
			for (int slot = 0; slot < pMesh->dwTextureCount; slot++)
			{
				pMesh->pTextures[slot].pName[0] = 0;
			}

			// Remove Wicked material textures
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
			if (mesh)
			{
				// get material from mesh
				uint64_t materialEntity = mesh->subsets[0].materialID;
				wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				if (pObjectMaterial)
				{
					for (int slot = 0; slot < wiScene::MaterialComponent::TEXTURESLOT_COUNT; slot++)
					{
						pObjectMaterial->textures[slot].name = "";
						pObjectMaterial->textures[slot].resource = {};
						pObjectMaterial->SetDirty();
					}
				}
			}
		}
	}
}

void WickedCall_SetExposure(float exposure)
{
	master.masterrenderer.setExposure(exposure);
}

#ifdef WICKEDPARTICLESYSTEM
uint32_t WickedCall_LoadWiSceneDirect(Scene& scene2,char* filename, bool attached, char* changename, char* changenameto)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	Entity root = 0;

	XMMATRIX& transformMatrix = XMMatrixIdentity();

	// Validate file is a genuine Wicked Engine archive before opening
	// (old .pe particle files from the DX11 engine have incompatible binary format)
	{
		FILE* fcheck = fopen(filename, "rb");
		if (fcheck)
		{
			uint64_t fileVersion = 0;
			size_t bytesRead = fread(&fileVersion, 1, sizeof(fileVersion), fcheck);
			fclose(fcheck);
			if (bytesRead < sizeof(fileVersion) || fileVersion < 22 || fileVersion > 200)
			{
				char msg[1024];
				sprintf(msg, "Skipped non-Wicked archive: %s (version=%llu, expected 22-93)", filename, (unsigned long long)fileVersion);
				void timestampactivity(int i, char* desc_s);
				timestampactivity(0, msg);
				return 0;
			}
		}
	}
	wiArchive archive(filename, true);
	if (archive.IsOpen())
	{
		if (archive.IsReadMode())
		{
			uint32_t reserved;
			archive >> reserved;
		}
		else
		{
			uint32_t reserved = 0;
			archive << reserved;
		}

		//PE: Keeping this alive to keep serialized resources alive until entity serialization ends:
		//wiResourceManager::Serialize removed from API
		//wiResourceManager::ResourceSerializer resource_seri;
		//if (archive.GetVersion() >= 63)
		//{
		//	wiResourceManager::Serialize(archive, resource_seri);
		//}

		EntitySerializer seri;

		scene2.names.Serialize(archive, seri);
		scene2.layers.Serialize(archive, seri);
		scene2.transforms.Serialize(archive, seri);
		//scene2.prev_transforms.Serialize(archive, seri); // removed, replaced by matrix_objects_prev
		scene2.hierarchy.Serialize(archive, seri);
		scene2.materials.Serialize(archive, seri);
		scene2.meshes.Serialize(archive, seri);
		scene2.impostors.Serialize(archive, seri);
		scene2.objects.Serialize(archive, seri);
		//scene2.aabb_objects.Serialize(archive, seri); // aabb vectors no longer have Serialize
		scene2.rigidbodies.Serialize(archive, seri);
		scene2.softbodies.Serialize(archive, seri);
		scene2.armatures.Serialize(archive, seri);
		scene2.lights.Serialize(archive, seri);
		//scene2.aabb_lights.Serialize(archive, seri); // aabb vectors no longer have Serialize
		scene2.cameras.Serialize(archive, seri);
		scene2.probes.Serialize(archive, seri);
		//scene2.aabb_probes.Serialize(archive, seri); // aabb vectors no longer have Serialize
		scene2.forces.Serialize(archive, seri);
		scene2.decals.Serialize(archive, seri);
		//scene2.aabb_decals.Serialize(archive, seri); // aabb vectors no longer have Serialize
		scene2.animations.Serialize(archive, seri);
		scene2.emitters.Serialize(archive, seri);
		scene2.hairs.Serialize(archive, seri);
		scene2.weathers.Serialize(archive, seri);
		if (archive.GetVersion() >= 30)
		{
			scene2.sounds.Serialize(archive, seri);
		}
		if (archive.GetVersion() >= 37)
		{
			scene2.inverse_kinematics.Serialize(archive, seri);
		}
		if (archive.GetVersion() >= 38)
		{
			scene2.springs.Serialize(archive, seri);
		}
		if (archive.GetVersion() >= 46)
		{
			scene2.animation_datas.Serialize(archive, seri);
		}

		//PE: create new root:
		root = CreateEntity();
		scene2.transforms.Create(root);
		scene2.layers.Create(root).layerMask = ~0;

		//PE: Parent all unparented transforms to new root entity
		for (size_t i = 0; i < scene2.transforms.GetCount() - 1; ++i) // GetCount() - 1 because the last added was the "root"
		{
			Entity entity = scene2.transforms.GetEntity(i);
			if (!scene2.hierarchy.Contains(entity))
			{
				scene2.Component_Attach(entity, root);
			}
		}
		//PE: The root component is transformed, scene is updated:
		scene2.transforms.GetComponent(root)->MatrixTransform(transformMatrix);
		scene2.Update(0);

		if (!attached)
		{
			//PE: In this case, we don't care about the root anymore, so delete it. This will simplify overall hierarchy
			scene2.Component_DetachChildren(root);
			scene2.Entity_Remove(root);
			root = INVALID_ENTITY;
		}

		//PE: Support _e_ here for all materials.
		for (int i = 0; i < scene2.materials.GetCount(); i++)
		{
			for (int a = 0; a < MaterialComponent::EMISSIVEMAP; a++)
			{
				if (scene2.materials[i].textures[a].name.size() > 0)
				{
					if (!scene2.materials[i].textures[a].resource.IsValid())
					{
						scene2.materials[i].textures[a].resource = WickedCall_LoadImage(scene2.materials[i].textures[a].name);
					}
				}
			}
		}
	}

	//PE: Fix for GG moved enums
	if (pestrcasestr(filename, ".wiscene"))
	{
		for (int i = 0; i < scene2.materials.GetCount(); i++)
		{
			if (scene2.materials[i].userBlendMode == BLENDMODE_PREMULTIPLIED)
				scene2.materials[i].userBlendMode = BLENDMODE_MULTIPLY;
			if (scene2.materials[i].userBlendMode == BLENDMODE_ALPHA)
				scene2.materials[i].userBlendMode = BLENDMODE_ADDITIVE;
			if (scene2.materials[i].userBlendMode == BLENDMODE_OPAQUE) // was BLENDMODE_FORCEDEPTH, removed
				scene2.materials[i].userBlendMode = BLENDMODE_PREMULTIPLIED;
			if (scene2.materials[i].userBlendMode == BLENDMODE_ALPHA) // was BLENDMODE_ALPHANOZ, removed
				scene2.materials[i].userBlendMode = BLENDMODE_ALPHA;
			scene2.materials[i].SetDirty();
		}
	}

	if (changename && changenameto)
	{
		for (int i = 0; i < scene2.names.GetCount(); i++)
		{
			if (stricmp(scene2.names[i].name.c_str(), changename) == 0)
			{
				scene2.names[i].name = changenameto;
			}
		}
	}
	
	return root;
}
uint32_t WickedCall_LoadWiScene(char* filename, bool attached, char* changename, char* changenameto)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	Scene scene2;
	Entity root = WickedCall_LoadWiSceneDirect(scene2, filename, attached, changename, changenameto);
	GetScene().Merge(scene2);
	return root;
}

uint32_t WickedCall_LoadWPE(char* filename)
{
	Scene& scene = wiScene::GetScene();
	uint32_t root = 0;
	uint32_t count_before = scene.emitters.GetCount();

	char path[MAX_PATH];
	strcpy(path, filename);
	GG_GetRealPath(path, 0);

	WickedCall_LoadWiScene(path, false, NULL, NULL);
	uint32_t count_after = scene.emitters.GetCount();
	if (count_before != count_after)
	{
		Entity emitter = scene.emitters.GetEntity(scene.emitters.GetCount() - 1);
		if (scene.emitters.GetCount() > 0)
		{
			HierarchyComponent* hier = scene.hierarchy.GetComponent(emitter);
			if (hier)
			{
				root = hier->parentID;
			}
		}
		wiEmittedParticle* ec = scene.emitters.GetComponent(emitter);
		if (ec)
		{
			ec->Restart();
			//ec->SetVisible(false); // SetVisible removed from EmittedParticleSystem
		}
	}
	return root;
}
#endif // WICKEDPARTICLESYSTEM (continued in wickedcalls_part4.cpp)
