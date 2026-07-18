#ifdef GGTREES_UNDOREDO
void GGTrees_CreateUndoRedoAction(int type, int eList, bool bUserAction, void* pEventData)
{
	if (!ggtrees_initialised) return;
	// User performed this undo action, so clear the redo stack since it now contains outdated events.
	if (bUserAction == true)
	{
		undosys_clearredostack();
		undosys_terrain_cleardata(g_TerrainRedoMem);
	}

	if (type == eUndoSys_Terrain_MoveTree)
	{
		// Event data needed.
		TreeMoveData moveData;

		if (bUserAction)
		{
			TreeMoveData* pTreeMoveData = (TreeMoveData*)pEventData;
			moveData.x = pTreeMoveData->x;
			moveData.z = pTreeMoveData->z;
			moveData.treeIndex = pTreeMoveData->treeIndex;
			moveData.userMoved = pTreeMoveData->userMoved;
		}
		else
		{
			// Undoing a redo or vice versa - get up-to-date data needed for a TreeMove event.
			sUndoSysEventTreeSingle* pImpendingUndoRedoEvent = (sUndoSysEventTreeSingle*)pEventData;
			moveData.treeIndex = undosys_terrain_treemove_gettreeindex(pImpendingUndoRedoEvent);

			// Use the tree index to get the current state of the tree to create the undo or redo action.
			GGTrees::InstanceTree* pInstance = &GGTrees::pAllTrees[moveData.treeIndex];
			moveData.x = pInstance->x;
			moveData.z = pInstance->z;
			moveData.userMoved = pInstance->IsUserMoved();
		}

		undosys_terrain_treemove(moveData, eList);
	}
	else if (type == eUndoSys_Terrain_ScaleTree)
	{
		TreeScaleData scaleData;
	
		if (bUserAction)
		{
			TreeScaleData* pTreeScaleData = (TreeScaleData*)pEventData;
			scaleData.treeIndex = pTreeScaleData->treeIndex;
			scaleData.userMoved = pTreeScaleData->userMoved;
			scaleData.scale = pTreeScaleData->scale;
		}
		else
		{
			// Undoing a redo or vice versa - get up-to-date data needed for a TreeMove event.
			sUndoSysEventTreeSingle* pImpendingUndoRedoEvent = (sUndoSysEventTreeSingle*)pEventData;
			scaleData.treeIndex = undosys_terrain_treemove_gettreeindex(pImpendingUndoRedoEvent);
	
			// Use the tree index to get the current state of the tree to create the undo or redo action.
			GGTrees::InstanceTree* pInstance = &GGTrees::pAllTrees[scaleData.treeIndex];
			scaleData.userMoved = pInstance->IsUserMoved();
			scaleData.scale = pInstance->GetScale();
		}
	
		undosys_terrain_treescale(scaleData, eList);
	}
	else if (type == eUndoSys_Terrain_AddTree)
	{
		// To undo an AddTree event, we must make a RemoveTree event.
		TreeRemoveData removeData;

		if (bUserAction)
		{
			TreeAddData* pTreeAddData = (TreeAddData*)pEventData;
			removeData.treeIndex = pTreeAddData->treeIndex;
			removeData.x = pTreeAddData->x;
			removeData.z = pTreeAddData->z;
			removeData.scale = pTreeAddData->scale;
			removeData.userMoved = pTreeAddData->userMoved;
		}
		else
		{
			// Undoing a redo or vice versa - get up-to-date data needed for a TreeMove event.
			sUndoSysEventTreeSingle* pImpendingUndoRedoEvent = (sUndoSysEventTreeSingle*)pEventData;
			removeData.treeIndex = undosys_terrain_treemove_gettreeindex(pImpendingUndoRedoEvent);

			// Use the tree index to get the current state of the tree to create the undo or redo action.
			GGTrees::InstanceTree* pInstance = &GGTrees::pAllTrees[removeData.treeIndex];
			removeData.scale = pInstance->GetScale();
			removeData.userMoved = pInstance->IsUserMoved();
			removeData.x = pInstance->x;
			removeData.z = pInstance->z;
		}

		undosys_terrain_treeremove(removeData, eList);
	}
	else if (type == eUndoSys_Terrain_RemoveTree)
	{
		// To undo a RemoveTree event, we must make an AddTree event.
		TreeAddData addData;

		if (bUserAction)
		{
			TreeRemoveData* pTreeRemoveData = (TreeRemoveData*)pEventData;
			addData.treeIndex = pTreeRemoveData->treeIndex;
			addData.scale = pTreeRemoveData->scale;
			addData.x = pTreeRemoveData->x;
			addData.z = pTreeRemoveData->z;
			addData.userMoved = pTreeRemoveData->userMoved;
			addData.type = pTreeRemoveData->type;
		}
		else
		{
			// Undoing a redo or vice versa - get up-to-date data needed for a TreeMove event.
			sUndoSysEventTreeSingle* pImpendingUndoRedoEvent = (sUndoSysEventTreeSingle*)pEventData;
			addData.treeIndex = undosys_terrain_treemove_gettreeindex(pImpendingUndoRedoEvent);
			
			// Use the tree index to get the current state of the tree to create the undo or redo action.
			GGTrees::InstanceTree* pInstance = &GGTrees::pAllTrees[addData.treeIndex];
			addData.x = pInstance->x;
			addData.z = pInstance->z;
			addData.userMoved = pInstance->IsUserMoved();
			addData.scale = pInstance->GetScale();
			addData.type = pInstance->GetType();
		}

		//undosys_terrain_treeremove(removeData, eList);
		undosys_terrain_treeadd(addData, eList);
	}
	else if (type == eUndoSys_Terrain_PaintTree)
	{
		// If we are undoing a redo or vice versa then the terrain snapshot will be out of date.
		if (!bUserAction)
		{
			// If we are undoing a redo or vice versa then the snapshot will be out of date.
			GGTrees::GGTrees_GetSnapshot(g_pTerrainSnapshot);
		}

		undosys_terrain_trees(g_pTerrainSnapshot, eList);
	}
}

void GGTrees_PerformUndoRedoAction(int type, void* pEventData, int eList)
{
	if (!ggtrees_initialised) return;
	if (!pEventData)
		return;

	uint8_t* pAddressOfEventData = nullptr;

	switch (type)
	{
	case eUndoSys_Terrain_MoveTree:
	{
		sUndoSysEventTreeSingle* pEvent = (sUndoSysEventTreeSingle*)pEventData;

		TreeMoveData moveData;
		memcpy(&moveData, pEvent->data, sizeof(TreeMoveData));
		GGTrees::InstanceTree* pInstance = &GGTrees::pAllTrees[moveData.treeIndex];
		GGTrees::GGTrees_SetTreePosition(moveData.treeIndex, moveData.x, moveData.z);
		pInstance->SetUserMoved(moveData.userMoved);

		pAddressOfEventData = pEvent->data;

		delete pEvent;
		pEvent = 0;
		
		break;
	}
	case eUndoSys_Terrain_ScaleTree:
	{
		sUndoSysEventTreeSingle* pEvent = (sUndoSysEventTreeSingle*)pEventData;

		TreeScaleData scaleData;
		memcpy(&scaleData, pEvent->data, sizeof(TreeScaleData));
		GGTrees::InstanceTree* pInstance = &GGTrees::pAllTrees[scaleData.treeIndex];
		pInstance->SetScale(scaleData.scale);
		pInstance->SetUserMoved(scaleData.userMoved);

		pAddressOfEventData = pEvent->data;

		delete pEvent;
		pEvent = 0;

		break;
	}
	case eUndoSys_Terrain_AddTree:
	{
		sUndoSysEventTreeSingle* pEvent = (sUndoSysEventTreeSingle*)pEventData;

		TreeAddData addData;
		memcpy(&addData, pEvent->data, sizeof(TreeAddData));

		if (GGTrees::pInvisibleTrees.NumItems() > 0)
		{
			uint32_t index = GGTrees::pInvisibleTrees.PopItem();

			GGTrees::pAllTrees[index].SetVisible(1);
			GGTrees::pAllTrees[index].SetUserMoved(1);
			GGTrees::pAllTrees[index].SetInvalid(0);
			GGTrees::pAllTrees[index].SetType(addData.type);
			GGTrees::pAllTrees[index].SetScale(addData.scale);
			GGTrees::GGTrees_SetTreePosition(index, addData.x, addData.z);
		}

		pAddressOfEventData = pEvent->data;

		delete pEvent;
		pEvent = 0;

		break;
	}
	case eUndoSys_Terrain_RemoveTree:
	{
		sUndoSysEventTreeSingle* pEvent = (sUndoSysEventTreeSingle*)pEventData; 
		TreeRemoveData removeData;
		memcpy(&removeData, pEvent->data, sizeof(TreeRemoveData));

		if ( GGTrees::pAllTrees[removeData.treeIndex].IsVisible() ) GGTrees::pInvisibleTrees.AddItem(removeData.treeIndex);
		GGTrees::pAllTrees[removeData.treeIndex].SetVisible(0);
		GGTrees::pAllTrees[removeData.treeIndex].SetUserMoved(0);
		GGTrees::TreeChunk* pChunk = GGTrees::GGTrees_GetChunk(GGTrees::pAllTrees[removeData.treeIndex].x, GGTrees::pAllTrees[removeData.treeIndex].z);
		if (pChunk) pChunk->Update();

		pAddressOfEventData = pEvent->data;

		delete pEvent;
		pEvent = 0;

		break;
	}
	case eUndoSys_Terrain_PaintTree:
	{
		sUndoSysEventTrees* pEvent = (sUndoSysEventTrees*)pEventData;

		memcpy(&GGTrees::pAllTrees, pEvent->treeData, sizeof(GGTrees::InstanceTree) * GGTrees::numTotalTrees);
		//memcpy(&GGTrees::pTreeChunks, pEvent->chunkData, sizeof(GGTrees::TreeChunk) * GGTrees::numTreeChunks);
		// the raw memcpy bypasses every setter — announce the bulk change so the
		// Wicked pool + spatial grid rebuild instead of waiting for the heartbeat
		GGTrees::g_treeInstanceStamp++;

		GGTrees::pInvisibleTrees.Clear();
		for (int i = 0; i < GGTrees::numTotalTrees; i++)
		{
			if ( !GGTrees::pAllTrees[i].IsVisible() )
				GGTrees::pInvisibleTrees.AddItem(i);
		}

		pAddressOfEventData = pEvent->treeData;

		delete pEvent;
		pEvent = 0;
	}
	}

	//Mark the memory for this event as unused (0 = undo, 1 = redo)
	if (eList == eUndoSys_UndoList)
		undosys_terrain_setmemoryunusedfrom(pAddressOfEventData, g_TerrainUndoMem);
	else
		undosys_terrain_setmemoryunusedfrom(pAddressOfEventData, g_TerrainRedoMem);
}
#endif
