#ifdef GGTERRAIN_UNDOREDO
void GGTerrain_PerformUndoRedoAction(int type, void* pEventData, int eList)
{
	if (!pEventData)
		return;

	uint8_t* pAddressOfEventData = nullptr;

	switch (type)
	{
		case eUndoSys_Terrain_Sculpt:
		{
			sUndoSysEventTerrainSculpt* pEvent = (sUndoSysEventTerrainSculpt*)pEventData;

			GGTerrain::GGTerrain_SetSculptData(0, 0, pEvent);
			
			pAddressOfEventData = pEvent->typeData;

			delete pEvent;
			pEvent = 0;

			break;
		}
		case eUndoSys_Terrain_Paint:
		{
			sUndoSysEventTerrainPaint* pEvent = (sUndoSysEventTerrainPaint*)pEventData;

			GGTerrain::GGTerrain_SetPaintData(0, 0, pEvent);
		
			pAddressOfEventData = pEvent->materialData;

			delete pEvent;
			pEvent = 0;

			break;
		}
	}

	//Mark the sculpt data memory for this event as unused (0 = undo, 1 = redo)
	if (eList == eUndoSys_UndoList)
		undosys_terrain_setmemoryunusedfrom(pAddressOfEventData, g_TerrainUndoMem);
	else
		undosys_terrain_setmemoryunusedfrom(pAddressOfEventData, g_TerrainRedoMem);
}

void GGTerrain_CreateUndoRedoAction(int type, int eList, bool bUserAction, void* pEventData)
{
	// count any sculpts as will need to reverse them all when undo-ing a delayed object repositioning
	extern int g_iDelayActualObjectAdjustmentSculptCount;
	if (type == eUndoSys_Terrain_Sculpt && bUserAction == true )
	{
		g_iDelayActualObjectAdjustmentSculptCount++;
	}
	
	// User performed this undo action, so clear the redo stack since it now contains outdated events.
	if (bUserAction == true)
	{
		undosys_clearredostack();
		undosys_terrain_cleardata(g_TerrainRedoMem);
	}

	if (type == eUndoSys_Terrain_Sculpt || type == eUndoSys_Terrain_Paint)
	{
		// If we are undoing a redo or vice versa then the terrain snapshot will be out of date.
		if (!bUserAction)
		{
			if (type == eUndoSys_Terrain_Sculpt)
				GGTerrain::GGTerrain_GetSculptData(g_pTerrainSnapshot);
			else if(type == eUndoSys_Terrain_Paint)
				GGTerrain::GGTerrain_GetPaintData(g_pTerrainSnapshot);
		}

		TerrainEditsBB bb;
		if (bUserAction)
		{	// Use the bounding box from the sculpt/paint that the user performed.
			bb = g_EditBounds;
		}
		else
		{
			// Use the bounding box from the event that we are storing the undo/redo data for.
			if (type == eUndoSys_Terrain_Sculpt)
			{
				sUndoSysEventTerrainSculpt* pImpendingUndoRedoEvent = (sUndoSysEventTerrainSculpt*)pEventData;
				bb.maxX = pImpendingUndoRedoEvent->maxX;
				bb.maxY = pImpendingUndoRedoEvent->maxY;
				bb.minX = pImpendingUndoRedoEvent->minX;
				bb.minY = pImpendingUndoRedoEvent->minY;
			}
			else if (type == eUndoSys_Terrain_Paint)
			{
				sUndoSysEventTerrainPaint* pImpendingUndoRedoEvent = (sUndoSysEventTerrainPaint*)pEventData;
				bb.maxX = pImpendingUndoRedoEvent->maxX;
				bb.maxY = pImpendingUndoRedoEvent->maxY;
				bb.minX = pImpendingUndoRedoEvent->minX;
				bb.minY = pImpendingUndoRedoEvent->minY;
			}
		}

		if (type == eUndoSys_Terrain_Sculpt)
		{
			// After sculpting, any objects that were on the terrain are moved up in line with the sculpt, so need to collect multiple events so both actions can be undone in one press.
			// though cannot use undosys_multiplevents_start(); we will use glue system instead (added to repos obj code)
			undosys_terrain_sculpt(bb, g_pTerrainSnapshot, eList);
		}
		else if (type == eUndoSys_Terrain_Paint)
		{
			undosys_terrain_paint(bb, g_pTerrainSnapshot, eList);
		}			
	}
}


#endif // GGTERRAIN_UNDOREDO
