// === END FUNCTION ===



// =====================================
// === BT INTERN GET SECTOR EXCLUSION ===
// ======================================
static bool BT_Intern_GetSectorExclusion(s_BT_terrain* Terrain,unsigned long LODLevel,unsigned long excludememblock,unsigned long row,unsigned long column,bool* buffer)
{
//Variables
	bool WholeSectorExcluded=true;
	unsigned long X;
	unsigned long Y;
	unsigned long Xb;
	unsigned long Yb;
	unsigned long StartX;
	unsigned long StartY;
	unsigned long BufferPos;
	unsigned long ExcludePos;
	bool Excluded;

//Set start positions
	StartX=column*Terrain->LODLevel[LODLevel].SectorDetail*Terrain->LODLevel[LODLevel].TileSpan;
	StartY=row*Terrain->LODLevel[LODLevel].SectorDetail*Terrain->LODLevel[LODLevel].TileSpan;

//Loop through points
	for(Y=0;Y<=Terrain->LODLevel[LODLevel].SectorDetail;Y++)
	{
		for(X=0;X<=Terrain->LODLevel[LODLevel].SectorDetail;X++)
		{

			BufferPos=(X+Y*(Terrain->LODLevel[LODLevel].SectorDetail+1));

			if(X*Terrain->LODLevel[LODLevel].TileSpan+StartX>unsigned(Terrain->Heightmapsize-1)) 
			{
				Xb=X*Terrain->LODLevel[LODLevel].TileSpan-1;
			}else{
				Xb=X*Terrain->LODLevel[LODLevel].TileSpan;
			}

			if(Y*Terrain->LODLevel[LODLevel].TileSpan+StartY>unsigned(Terrain->Heightmapsize-1))
			{
				Yb=Y*Terrain->LODLevel[LODLevel].TileSpan-1;
			}else{;
				Yb=Y*Terrain->LODLevel[LODLevel].TileSpan;
			}
			ExcludePos=12+((Xb+StartX)+(Yb+StartY)*Terrain->Heightmapsize)*4;
			#ifdef DX11
			Excluded=false;// no holes in terrain for DX11
			#else
			Excluded=Terrain->ExclusionThreshold>unsigned(GGCOLOR(ReadMemblockDWord(excludememblock,ExcludePos)).r*255);
			buffer[BufferPos]=Excluded;
			#endif
			if(Excluded==false)
				WholeSectorExcluded=false;
		}
	}

//Return
	return WholeSectorExcluded;
}
// === END FUNCTION ===



// ==============================
// === BT INTERN BUILD SECTOR ===
// ==============================
static void BT_Intern_BuildSector(s_BT_Sector* Sector)
{
//Generate meshdata
	Sector->QuadMap->GenerateMeshData();

//Create object
	Sector->DrawBuffer=Sector->QuadMap->GeneratePlain();

//Make world matrix
	GGMatrixTranslation(&Sector->WorldMatrix,Sector->Pos_x*Sector->Terrain->Scale/C_BT_INTERNALSCALE,Sector->Pos_y,Sector->Pos_z*Sector->Terrain->Scale/C_BT_INTERNALSCALE);
}
// === END FUNCTION ===



// ================================
// === BT INTERN DELETE TERRAIN ===
// ================================
static void BT_Intern_DeleteTerrain(unsigned long TerrainID,bool DeleteObjectFlag)
{
//Check that the terrain is built
	if(BT_Main.Terrains[TerrainID].Built==true)
	{
	//Get terrain pointer
		s_BT_terrain* Terrain;
		Terrain=&BT_Main.Terrains[TerrainID];

	//Delete LODMap
		for(unsigned long i=0;i<Terrain->LODLevel[0].Split;i++)
			free(Terrain->LODMap[i]);
		free(Terrain->LODMap);

	//Delete LODLevels
		for(unsigned long LODLevel=0;LODLevel<Terrain->LODLevels;LODLevel++)
		{
		//Delete sectors
			unsigned long Sector=0;
			for(Sector=0;Sector<Terrain->LODLevel[LODLevel].Sectors;Sector++)
			{
			//Check if the sector is not excluded
				if(Terrain->LODLevel[LODLevel].Sector[Sector].Excluded==false)
				{
					#ifdef DX11
					// delete draw buffer in DX11
					#else
					//Delete drawbuffer
					Terrain->LODLevel[LODLevel].Sector[Sector].DrawBuffer->VertexBuffer->Release();
					Terrain->LODLevel[LODLevel].Sector[Sector].DrawBuffer->IndexBuffer->Release();
					Terrain->LODLevel[LODLevel].Sector[Sector].DrawBuffer->EdgeLineIndexBuffer->Release();
					#endif
					free(Terrain->LODLevel[LODLevel].Sector[Sector].DrawBuffer);

				//Delete Quadmap
					Terrain->LODLevel[LODLevel].Sector[Sector].QuadMap->DeleteInternalData();
					free(Terrain->LODLevel[LODLevel].Sector[Sector].QuadMap);

				//Delete RTTMS
					free(Terrain->LODLevel[LODLevel].Sector[Sector].VertexDataRTTMS);

				//Delete Info
					free(Terrain->LODLevel[LODLevel].Sector[Sector].Info);
				}
			}
			free(Terrain->LODLevel[LODLevel].Sector);

		//Delete Info
			free(Terrain->LODLevel[LODLevel].Info);
		}
		free(Terrain->LODLevel);

	//Delete Quadtree
		BT_Intern_DeAllocateQuadTree(Terrain->QuadTree);

	//Delete Environment map
		BT_Intern_DeleteEnvironmentMap(Terrain->EnvironmentMap);
		free(Terrain->EnvironmentMap);

	//Delete object
		if(DeleteObjectFlag==true)
			DeleteObject(Terrain->ObjectID);

	//Delete update handlers
		BT_RTTMS_DeleteUpdateHandlers(TerrainID);

	//Delete Info
		free(Terrain->Info);

	//Zero terrain array
		memset(Terrain,0,sizeof(s_BT_terrain));
	}else{
	//Zero terrain array
		memset(&BT_Main.Terrains[TerrainID],0,sizeof(s_BT_terrain));
	}
}
// === END FUNCTION ===

// ==================================
// === BT INTERN GET POINT HEIGHT ===
// ==================================
float BT_Intern_GetPointHeight(s_BT_terrain* Terrain,float Px,float Pz,char LODLevel,bool Round)
{
//Variables
	float Height;
	unsigned short SRow;
	unsigned short SCollumn;
	unsigned short SectorID;
	s_BT_Sector* Sector;
	BT_QuadMap* Quadmap;

//Clamp values
	if(Px>Terrain->TerrainSize)
		return 0.0;

	if(Pz>Terrain->TerrainSize)
		return 0.0;

	if(Px<0)
		return 0.0;

	if(Pz<0)
		return 0.0;

//Find the row and collumn of the sector
	SRow=(unsigned short)floor(Px/Terrain->LODLevel[LODLevel].SectorSize);
	SCollumn=(unsigned short)floor(Pz/Terrain->LODLevel[LODLevel].SectorSize);

//As the above clamping sometimes makes mistakes, we will correct them here
	if(SRow>unsigned(Terrain->LODLevel[LODLevel].Split-1))
		SRow=Terrain->LODLevel[LODLevel].Split-1;

	if(SCollumn>unsigned(Terrain->LODLevel[LODLevel].Split-1))
		SCollumn=Terrain->LODLevel[LODLevel].Split-1;

	SectorID=SCollumn+SRow*Terrain->LODLevel[LODLevel].Split;

//Find the sector pointer
	Sector=&Terrain->LODLevel[LODLevel].Sector[SectorID];

//Get point on sector
	Px=Px-Sector->Pos_x;
	Pz=Pz-Sector->Pos_z;

//Find the quadmap pointer
	Quadmap=Sector->QuadMap;

//Check that the sector exists
	if(Sector->Excluded==false)
	{
	//Get the height
		Height=Quadmap->GetPointHeight(Px,Pz,Round);

	//Return the height
		return Height;

	}else{
	//Return nothing
		return 0.0;
	}
}
// === END FUNCTION ===



// ==================================
// === BT INTERN GET POINT NORMAL ===
// ==================================
BTVector3 BT_Intern_GetPointNormal(s_BT_terrain* Terrain,float Px,float Pz)
{
//Variables
	BTVector3 Normal;
	float Tilesize;
	float Top;
	float Left;
	float Bottom;
	float Right;
	float Dx;
	float Dy;
	float Dz;

//Work out tilesize (halved to make this code a little faster)
	Tilesize=C_BT_INTERNALSCALE/2.0;

//Find point heights
	Top=BT_Intern_GetPointHeight(Terrain,Px,float(Pz+Tilesize),0,0);
	Left=BT_Intern_GetPointHeight(Terrain,float(Px-Tilesize),Pz,0,0);
	Bottom=BT_Intern_GetPointHeight(Terrain,Px,float(Pz-Tilesize),0,0);
	Right=BT_Intern_GetPointHeight(Terrain,float(Px+Tilesize),Pz,0,0);

//If Top or Right heights are 0 then set height to middle (bug fix)
	if(Top==0.0f || Left==0.0f || Bottom==0.0f || Right==0.0f)
	{
		float Middle=BT_Intern_GetPointHeight(Terrain,Px,Pz,0,0);
		if(Top==0.0f)
			Top=Middle*2-Bottom;
		if(Left==0.0f)
			Left=Middle*2-Right;
		if(Bottom==0.0f)
			Bottom=Middle*2-Top;
		if(Right==0.0f)
			Right=Middle*2-Left;
	}

//Get distances
	Dx=(Bottom-Top)/Tilesize;
	Dz=(Right-Left)/Tilesize;
	Dx=Dx/Terrain->Scale*C_BT_INTERNALSCALE;
	Dz=Dz/Terrain->Scale*C_BT_INTERNALSCALE;
	Dy=float(1.0/sqrt(1.0+Dx*Dx+Dz*Dz));

//Work out normal
	Normal.x=-Dz*Dy;
	Normal.y=Dy;
	Normal.z=Dx*Dy;

	return Normal;
}
// === END FUNCTION ===



// ====================================
// === BT INTERN GET POINT EXCLUDED ===
// ====================================
static bool BT_Intern_GetPointExcluded(s_BT_terrain* Terrain,float Px,float Pz)
{
//Variables
	unsigned short SRow;
	unsigned short SCollumn;
	unsigned short SectorID;
	s_BT_Sector* Sector;
	BT_QuadMap* Quadmap;

//Clamp values
	if(Px>Terrain->TerrainSize-C_BT_INTERNALSCALE)
		return true;

	if(Pz>Terrain->TerrainSize-C_BT_INTERNALSCALE)
		return true;

	if(Px<0)
		return true;

	if(Pz<0)
		return true;

//Find the row and collumn of the sector
	SRow=(unsigned short) floor(Px/Terrain->LODLevel[0].SectorSize);
	SCollumn=(unsigned short) floor(Pz/Terrain->LODLevel[0].SectorSize);

//As the above clamping sometimes makes mistakes, we will correct them here
	if(SRow>unsigned(Terrain->LODLevel[0].Split-1))
		SRow=Terrain->LODLevel[0].Split-1;

	if(SCollumn>unsigned(Terrain->LODLevel[0].Split-1))
		SCollumn=Terrain->LODLevel[0].Split-1;

	SectorID=SCollumn+SRow*Terrain->LODLevel[0].Split;

//Find the sector pointer
	Sector=&Terrain->LODLevel[0].Sector[SectorID];

//Get point on sector
	Px=Px-Sector->Pos_x;
	Pz=Pz-Sector->Pos_z;

//Find the quadmap pointer
	Quadmap=Sector->QuadMap;

//Check that the sector exists
	if(Sector->Excluded==false)
	{
	//Get the excluded
		return Quadmap->GetPointExcluded(Px,Pz);
	}else{
	//Return nothing
		return false;
	}
}
// === END FUNCTION ===



// ===================================
// === BT INTERN ALLOCATE QUADTREE ===
// ===================================
s_BT_QuadTree* BT_Intern_AllocateQuadTree(s_BT_terrain* Terrain)
{
//Variables
	s_BT_QuadTree* QuadTree;
	unsigned char Levels;

//Calculate levels
	unsigned char i=1;
	while(((Terrain->LODLevel[0].Split>>i)&0x1)==NULL)
		i++;

	Levels=i;
	Terrain->QuadTreeLevels=Levels;

//Allocate quadtree
	QuadTree=BT_Intern_AllocateQuadTreeRec(Terrain,Levels,NULL,0,0);

	return QuadTree;
}
// === END FUNCTION ===



// =======================================
// === BT INTERN ALLOCATE QUADTREE REC ===
// =======================================
static s_BT_QuadTree* BT_Intern_AllocateQuadTreeRec(s_BT_terrain* Terrain,unsigned char Levels,s_BT_QuadTree* Parent,unsigned char row,unsigned char collumn)
{
//Variables
	s_BT_QuadTree* Quadtree;
	bool QHasSector=false;

//Allocate Quadtree
	Quadtree=(s_BT_QuadTree*)malloc(sizeof(s_BT_QuadTree));
	if(Quadtree==nullptr)
		BT_Intern_Error(C_BT_ERROR_MEMORYERROR);
	memset ( Quadtree, 0, sizeof(s_BT_QuadTree) );

//Set values
	Quadtree->Level=Levels;
	Quadtree->row=row;
	Quadtree->collumn=collumn;
	Quadtree->Parent=Parent;
	Quadtree->Excluded=false;
	Quadtree->Culled=false;

//Allocate cullbox
	Quadtree->CullBox=(s_BT_CullBox*)malloc(sizeof(s_BT_CullBox));
	if(Quadtree->CullBox==nullptr)
		BT_Intern_Error(C_BT_ERROR_MEMORYERROR);

//Check if this LOD level exists
	if(Levels<Terrain->LODLevels)
	{
	//Find sector ID
		Quadtree->Sector=&Terrain->LODLevel[Levels].Sector[row+collumn*Terrain->LODLevel[Levels].Split];
		Quadtree->Sector->QuadTree=Quadtree;

	//Set cullbox values
		if(Quadtree->Sector->Excluded==false)
		{
			Quadtree->CullBox->Left=float(Quadtree->Sector->Pos_x-Terrain->LODLevel[Levels].SectorSize/2.0);
			Quadtree->CullBox->Right=float(Quadtree->Sector->Pos_x+Terrain->LODLevel[Levels].SectorSize/2.0);
			Quadtree->CullBox->Front=float(Quadtree->Sector->Pos_z-Terrain->LODLevel[Levels].SectorSize/2.0);
			Quadtree->CullBox->Back=float(Quadtree->Sector->Pos_z+Terrain->LODLevel[Levels].SectorSize/2.0);
			Quadtree->CullBox->Top=Quadtree->Sector->Pos_y+Quadtree->Sector->QuadMap->GetHighestPoint();
			Quadtree->CullBox->Bottom=Quadtree->Sector->Pos_y+Quadtree->Sector->QuadMap->GetLowestPoint();
		}
		QHasSector=true;

		Quadtree->Excluded=Quadtree->Sector->Excluded;
	}else{
		Quadtree->Sector=NULL;
	}

//Allocate children (if any)
	if(Levels>0)
	{
		Levels--;
		Quadtree->n1=BT_Intern_AllocateQuadTreeRec(Terrain,Levels,Quadtree,row*2,collumn*2);
		Quadtree->n2=BT_Intern_AllocateQuadTreeRec(Terrain,Levels,Quadtree,row*2,collumn*2+1);
		Quadtree->n3=BT_Intern_AllocateQuadTreeRec(Terrain,Levels,Quadtree,row*2+1,collumn*2);
		Quadtree->n4=BT_Intern_AllocateQuadTreeRec(Terrain,Levels,Quadtree,row*2+1,collumn*2+1);

		Quadtree->Excluded=Quadtree->n1->Excluded && Quadtree->n2->Excluded && Quadtree->n3->Excluded && Quadtree->n4->Excluded;

		if(QHasSector==false)
		{
			Quadtree->CullBox->Left=Quadtree->n1->CullBox->Left;
			Quadtree->CullBox->Right=Quadtree->n4->CullBox->Right;
			Quadtree->CullBox->Front=Quadtree->n1->CullBox->Front;
			Quadtree->CullBox->Back=Quadtree->n4->CullBox->Back;

			Quadtree->CullBox->Top=Quadtree->n1->CullBox->Top;
			if(Quadtree->n2->CullBox->Top>Quadtree->CullBox->Top)
				Quadtree->CullBox->Top=Quadtree->n2->CullBox->Top;
			if(Quadtree->n3->CullBox->Top>Quadtree->CullBox->Top)
				Quadtree->CullBox->Top=Quadtree->n3->CullBox->Top;
			if(Quadtree->n4->CullBox->Top>Quadtree->CullBox->Top)
				Quadtree->CullBox->Top=Quadtree->n4->CullBox->Top;

			Quadtree->CullBox->Bottom=Quadtree->n1->CullBox->Bottom;
			if(Quadtree->n2->CullBox->Bottom<Quadtree->CullBox->Bottom)
				Quadtree->CullBox->Bottom=Quadtree->n2->CullBox->Bottom;
			if(Quadtree->n3->CullBox->Bottom<Quadtree->CullBox->Bottom)
				Quadtree->CullBox->Bottom=Quadtree->n3->CullBox->Bottom;
			if(Quadtree->n4->CullBox->Bottom<Quadtree->CullBox->Bottom)
				Quadtree->CullBox->Bottom=Quadtree->n4->CullBox->Bottom;
		}
	}else{
		Quadtree->n1=NULL;
		Quadtree->n2=NULL;
		Quadtree->n3=NULL;
		Quadtree->n4=NULL;
	}

//Find position
	Quadtree->PosX=(Quadtree->CullBox->Left+Quadtree->CullBox->Right)/2.0f;
	Quadtree->PosY=(Quadtree->CullBox->Top+Quadtree->CullBox->Bottom)/2.0f;
	Quadtree->PosZ=(Quadtree->CullBox->Front+Quadtree->CullBox->Back)/2.0f;

//Return
	return Quadtree;
}
// === END FUNCTION ===

//LEFT = x-size
//RIGHT = x+size
//TOP = y+size
//BOTTOM = y-size
//FRONT = z-size
//BACK = z+size

// =====================================
// === BT INTERN DEALLOCATE QUADTREE ===
// =====================================
static void BT_Intern_DeAllocateQuadTree(s_BT_QuadTree* Quadtree)
{
//DeAllocate quadtree
	BT_Intern_DeAllocateQuadTreeRec(Quadtree);
	free(Quadtree);
}
// === END FUNCTION ===



// =========================================
// === BT INTERN DEALLOCATE QUADTREE REC ===
// =========================================
static void BT_Intern_DeAllocateQuadTreeRec(s_BT_QuadTree* Quadtree)
{
//Deallocate children
	if(Quadtree->n1!=NULL){
		BT_Intern_DeAllocateQuadTreeRec(Quadtree->n1);
		free(Quadtree->n1);
	}

	if(Quadtree->n2!=NULL){
		BT_Intern_DeAllocateQuadTreeRec(Quadtree->n2);
		free(Quadtree->n2);
	}

	if(Quadtree->n3!=NULL){
		BT_Intern_DeAllocateQuadTreeRec(Quadtree->n3);
		free(Quadtree->n3);
	}

	if(Quadtree->n4!=NULL){
		BT_Intern_DeAllocateQuadTreeRec(Quadtree->n4);
#pragma message("TEMPORARY")
		//free(Quadtree->n4);
	}

	if(Quadtree->CullBox!=NULL)
		free(Quadtree->CullBox);
}
// === END FUNCTION ===



void BT_Intern_RefreshVB(s_BT_DrawBuffer* DrawBuffer,unsigned long FirstVertex,unsigned long LastVertex,BT_Meshdata_Vertex* Vertex)
{
//Variables

	//FVF size
	unsigned long FVFSize;

//Get variables from drawbuffer
	FVFSize=DrawBuffer->FVF_Size;

//Lock vertexdata
#ifdef DX11
	// lock and write to vertex buffer in DX11
	if ( 0 ) 
	{
		// if VB is dynamic
		D3D11_MAPPED_SUBRESOURCE resource;
		std::memset ( &resource, 0, sizeof ( resource ) );
		if ( FAILED ( m_pImmediateContext->Map ( DrawBuffer->VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource ) ) ) return;
		BT_Meshdata_Vertex* LockedVertex = (BT_Meshdata_Vertex*)resource.pData;
		std::memcpy(LockedVertex,Vertex+FirstVertex*FVFSize,(LastVertex-FirstVertex)*FVFSize);
		m_pImmediateContext->Unmap ( DrawBuffer->VertexBuffer, 0 );
	}
	else
	{
		// if VB is default (faster)
		D3D11_BOX box;
		box.left = FirstVertex*FVFSize;
		box.right = (FirstVertex*FVFSize) + ( (LastVertex-FirstVertex)*FVFSize );
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		m_pImmediateContext->UpdateSubresource ( DrawBuffer->VertexBuffer, 0, &box, Vertex, 0, 0 );
	}
#else
	//Locked vertex list
	BT_Meshdata_Vertex* LockedVertex;
	DrawBuffer->VertexBuffer->Lock(FirstVertex*FVFSize,(LastVertex-FirstVertex)*FVFSize,(void**)&LockedVertex,NULL);

//Copy vertexdata
	memcpy(LockedVertex,Vertex+FirstVertex*FVFSize,(LastVertex-FirstVertex)*FVFSize);

//Unlock vertexdata
	DrawBuffer->VertexBuffer->Unlock();
#endif
}

void BT_Intern_RefreshIB(s_BT_DrawBuffer* DrawBuffer,unsigned long FirstIndex,unsigned long LastIndex,unsigned short* Index)
{
#ifdef DX11
	// lock index buffer and write to it
	if ( 0 )
	{
		// if IB is dynamic
		D3D11_MAPPED_SUBRESOURCE resource;
		std::memset ( &resource, 0, sizeof ( resource ) );
		if ( FAILED ( m_pImmediateContext->Map ( DrawBuffer->IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource ) ) ) return;
		unsigned short* LockedIndex = (unsigned short*)resource.pData;
		std::memcpy(LockedIndex,Index+FirstIndex*sizeof(unsigned short),(LastIndex-FirstIndex)*sizeof(unsigned short));
		m_pImmediateContext->Unmap ( DrawBuffer->IndexBuffer, 0 );
	}
	else
	{
		// if IB is default (faster)
		D3D11_BOX box;
		box.left = FirstIndex*sizeof(unsigned short);
		box.right = (FirstIndex*sizeof(unsigned short)) + ( (LastIndex-FirstIndex)*sizeof(unsigned short) );
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		m_pImmediateContext->UpdateSubresource ( DrawBuffer->IndexBuffer, 0, &box, Index, 0, 0 );
	}
#else
	//Locked index list
	unsigned short* LockedIndex;
//Lock indexdata
	DrawBuffer->IndexBuffer->Lock(FirstIndex*sizeof(unsigned short),(LastIndex-FirstIndex)*sizeof(unsigned short),(void**)&LockedIndex,NULL);

//Copy indexdata
	memcpy(LockedIndex,Index+FirstIndex*sizeof(unsigned short),(LastIndex-FirstIndex)*sizeof(unsigned short));

//Unlock indexdata
	DrawBuffer->IndexBuffer->Unlock();
#endif
}



static void BT_Intern_ExtractFrustum()
{
//Variables
	GGMATRIX ClipMatrix;

//Multiply view and projection matrices together
	GGMatrixMultiply(&ClipMatrix,&BT_Main.CurrentUpdateCamera->matView,&BT_Main.CurrentUpdateCamera->matProjection);

//Get right plane
	BT_Main.Frustum[0][0]=ClipMatrix._14-ClipMatrix._11;
	BT_Main.Frustum[0][1]=ClipMatrix._24-ClipMatrix._21;
	BT_Main.Frustum[0][2]=ClipMatrix._34-ClipMatrix._31;
	BT_Main.Frustum[0][3]=ClipMatrix._44-ClipMatrix._41;

//Get left plane
	BT_Main.Frustum[1][0]=ClipMatrix._14+ClipMatrix._11;
	BT_Main.Frustum[1][1]=ClipMatrix._24+ClipMatrix._21;
	BT_Main.Frustum[1][2]=ClipMatrix._34+ClipMatrix._31;
	BT_Main.Frustum[1][3]=ClipMatrix._44+ClipMatrix._41;

//Get bottom plane
	BT_Main.Frustum[2][0]=ClipMatrix._14+ClipMatrix._12;
	BT_Main.Frustum[2][1]=ClipMatrix._24+ClipMatrix._22;
	BT_Main.Frustum[2][2]=ClipMatrix._34+ClipMatrix._32;
	BT_Main.Frustum[2][3]=ClipMatrix._44+ClipMatrix._42;

//Get top plane
	BT_Main.Frustum[3][0]=ClipMatrix._14-ClipMatrix._12;
	BT_Main.Frustum[3][1]=ClipMatrix._24-ClipMatrix._22;
	BT_Main.Frustum[3][2]=ClipMatrix._34-ClipMatrix._32;
	BT_Main.Frustum[3][3]=ClipMatrix._44-ClipMatrix._42;

//Get far plane
	BT_Main.Frustum[4][0]=ClipMatrix._14-ClipMatrix._13;
	BT_Main.Frustum[4][1]=ClipMatrix._24-ClipMatrix._23;
	BT_Main.Frustum[4][2]=ClipMatrix._34-ClipMatrix._33;
	BT_Main.Frustum[4][3]=ClipMatrix._44-ClipMatrix._43;

}


//LEFT = x-size
//RIGHT = x+size
//TOP = y+size
//BOTTOM = y-size
//FRONT = z-size
//BACK = z+size


//Returns 0 if box isnt in frustum, 1 if intersecting and 2 if completely in frustum
static int BT_Intern_CullBox(s_BT_CullBox* pCullBox)
{
//Increase cull checks
	BT_Main.CullChecks++;

//Subtract Cull offset
	s_BT_CullBox CullBox;
	CullBox.Left=pCullBox->Left*BT_Main.CullScale.x-BT_Main.CullOffset.x;
	CullBox.Right=pCullBox->Right*BT_Main.CullScale.x-BT_Main.CullOffset.x;
	CullBox.Top=pCullBox->Top*BT_Main.CullScale.y-BT_Main.CullOffset.y;
	CullBox.Bottom=pCullBox->Bottom*BT_Main.CullScale.y-BT_Main.CullOffset.y;
	CullBox.Front=pCullBox->Front*BT_Main.CullScale.z-BT_Main.CullOffset.z;
	CullBox.Back=pCullBox->Back*BT_Main.CullScale.z-BT_Main.CullOffset.z;

//Variables
	int p;
	int c;
	int c2 = 0;

//Loop
	for( p = 0; p < 5; p++ )
	{
		c = 0;
		if( BT_Main.Frustum[p][0] * CullBox.Left + BT_Main.Frustum[p][1] * CullBox.Bottom + BT_Main.Frustum[p][2] * CullBox.Front + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Right + BT_Main.Frustum[p][1] * CullBox.Bottom + BT_Main.Frustum[p][2] * CullBox.Front + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Left + BT_Main.Frustum[p][1] * CullBox.Top + BT_Main.Frustum[p][2] * CullBox.Front + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Right + BT_Main.Frustum[p][1] * CullBox.Top + BT_Main.Frustum[p][2] * CullBox.Front + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Left + BT_Main.Frustum[p][1] * CullBox.Bottom + BT_Main.Frustum[p][2] * CullBox.Back + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Right + BT_Main.Frustum[p][1] * CullBox.Bottom + BT_Main.Frustum[p][2] * CullBox.Back + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Left + BT_Main.Frustum[p][1] * CullBox.Top + BT_Main.Frustum[p][2] * CullBox.Back + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( BT_Main.Frustum[p][0] * CullBox.Right + BT_Main.Frustum[p][1] * CullBox.Top + BT_Main.Frustum[p][2] * CullBox.Back + BT_Main.Frustum[p][3] > 0 )
			c++;
		if( c == 0 )
			return 0;
		if( c == 8 )
			c2++;
	}
	return (c2 == 4) ? 2 : 1;
}



// =========================================
// === BT INTERN LOCK SECTOR VERTEX DATA ===
// =========================================
static BT_RTTMS_STRUCT* BT_Intern_LockSectorVertexData(s_BT_Sector* Sector)
{
//Lock sector
	return (BT_RTTMS_STRUCT*)BT_RTTMS_LockSectorVertexData(Sector->Terrain->ID,Sector->LODLevel->ID,Sector->ID);
}
// === END FUNCTION ===



// ===========================================
// === BT INTERN UNLOCK SECTOR VERTEX DATA ===
// ===========================================
static void BT_Intern_UnlockSectorVertexData(s_BT_Sector* Sector)
{
//Unlock vertexdata
	if(Sector->VertexDataLocked==true)
	{
		BT_RTTMS_UnlockSectorVertexData((void*)Sector->VertexDataRTTMS);
	}
}
// === END FUNCTION ===


// ======================================
// === BT INTERN RTTMS UPDATE HANDLER ===
// ======================================
void BT_Intern_RTTMSUpdateHandler(unsigned long TerrainID,unsigned long LODLevelID,unsigned long SectorID,unsigned short StartVertex,unsigned short EndVertex,float* VerticesPtr)
{
#ifdef C_BT_FULLVERSION
//Cast vertices
	float* Vertices=(float*)VerticesPtr;

//Get sector
	s_BT_Sector* Sector=&BT_Main.Terrains[TerrainID].LODLevel[LODLevelID].Sector[SectorID];

//Change mesh data
	if(EndVertex>0)
	{
	//Update meshdata
		Sector->QuadMap->ChangeMeshData(StartVertex,EndVertex,Vertices);
		Sector->UpdateMesh=true;

	//Force update of edges
		Sector->LeftSideNeedsUpdate=true;
		Sector->RightSideNeedsUpdate=true;
		Sector->TopSideNeedsUpdate=true;
		Sector->BottomSideNeedsUpdate=true;

	//Update collision
		if(Sector->DBPObject!=0){
			Sector->QuadMap->UpdateDBPMesh(Sector->DBPObject->pFrame->pMesh);
		}
		if(Sector->LODLevel->DBPObject!=0){
		//Find the sector
			if ( Sector->LODLevelObjectFrame )
				Sector->QuadMap->UpdateDBPMesh(Sector->LODLevelObjectFrame->pMesh);
		}

	//Say that the cull box has changed
		s_BT_QuadTree* QuadTree=Sector->QuadTree;
		do{
			QuadTree->CullboxChanged=true;
			QuadTree=QuadTree->Parent;
		}while(QuadTree!=NULL);
	}
#endif
}
// === END FUNCTION ===



// ========================================
// === BT INTERN GET HEIGHT FROM COLOUR ===
// ========================================
static float BT_Intern_GetHeightFromColor(unsigned long Colour)
{
	GGCOLOR D3DColour = GGCOLOR(Colour);
#ifdef C_BT_FULLVERSION
	return D3DColour.r * 256.0f + D3DColour.g + D3DColour.b / 256.0f;
#else
	return D3DColour.r * 256.0f;
#endif
}
// === END FUNCTION ===



// =========================
// === BT SMOOTH TERRAIN ===
// =========================
static void BT_Intern_SmoothTerrain(s_BT_terrain* Terrain)
{
//Loop through smooth levels
	for(unsigned char SmoothLevel=0;SmoothLevel<Terrain->Smoothing;SmoothLevel++){
		for(unsigned short y=1;y<Terrain->Heightmapsize-1;y++){
			for(unsigned short x=1;x<Terrain->Heightmapsize-1;x++){
				float CornA=Terrain->HeightPoint[(x-1)+(y-1)*Terrain->Heightmapsize];
				float CornB=Terrain->HeightPoint[(x+1)+(y-1)*Terrain->Heightmapsize];
				float CornC=Terrain->HeightPoint[(x-1)+(y+1)*Terrain->Heightmapsize];
				float CornD=Terrain->HeightPoint[(x+1)+(y+1)*Terrain->Heightmapsize];
				float NexA=Terrain->HeightPoint[(x-1)+y*Terrain->Heightmapsize];
				float NexB=Terrain->HeightPoint[(x+1)+y*Terrain->Heightmapsize];
				float NexC=Terrain->HeightPoint[x+(y-1)*Terrain->Heightmapsize];
				float NexD=Terrain->HeightPoint[x+(y+1)*Terrain->Heightmapsize];
				float Middle=Terrain->HeightPoint[x+y*Terrain->Heightmapsize];
				float CornerAverage=(CornA+CornB+CornC+CornD)/4.0f;
				float NeighborAverage=(CornerAverage+NexA+NexB+NexC+NexD)/5.0f;
				Terrain->HeightPoint[x+y*Terrain->Heightmapsize]=(Middle+NeighborAverage)/2.0f;
			}
		}
	}
}
// === END FUNCTION ===
