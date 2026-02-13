DARKSDK unsigned long ReadObjectChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 if (ReadName ()==-1)
 {
  #ifdef __DEBUG__
  printf (">>>>* Dummy Object found\n");
  #endif
 }

 // CREATE Frame Start (delayed - only if required - 300606)
 bool bStartedFrameEntry = false;

 // frame contents
 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case OBJ_UNKNWN01:
						  if ( bStartedFrameEntry==false ) { StartFrameEntry ( temp_name); bStartedFrameEntry = true; }
						  tellertje+=ReadUnknownChunk (OBJ_UNKNWN01);
						  break;
        case OBJ_UNKNWN02:
						  if ( bStartedFrameEntry==false ) { StartFrameEntry ( temp_name); bStartedFrameEntry = true; }
						  tellertje+=ReadUnknownChunk (OBJ_UNKNWN02);
						  break;

        case OBJ_TRIMESH :
						  if ( bStartedFrameEntry==false ) { StartFrameEntry ( temp_name); bStartedFrameEntry = true; }
                          tellertje+=ReadObjChunk ();
                          break;
        case OBJ_FRAMES :
						  if ( bStartedFrameEntry==false ) { StartFrameEntry ( temp_name); bStartedFrameEntry = true; }
                          #ifdef __DEBUG__
                          printf (">>>> Found Obj/Frame chunk id of %0X\n",OBJ_FRAMES);
                          #endif
                          tellertje+=ReadFrameChunk();
                          break;
        case OBJ_LIGHT   :
                          #ifdef __DEBUG__
                          printf (">>>> Found Light chunk id of %0X\n",OBJ_LIGHT);
                          #endif
                          tellertje+=ReadLightChunk ();
                          break;
        case OBJ_CAMERA  :
                          #ifdef __DEBUG__
                          printf (">>>> Found Camera chunk id of %0X\n",OBJ_CAMERA);
                          #endif
                          tellertje+=ReadCameraChunk ();
                          break;
        default:         
			tellertje+=ReadUnknownChunk(0);
			break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 // Ensure frame hierarchy maintained
 // 300606 - u62 - ignore light and camera frames (X files do not like empty frames)
 if ( bStartedFrameEntry==true )
 {
	if(g_FindHierarchy==false)
	 {
		 // Advance indexes..
		 g_HierarchyFrameDepth++;
		 g_HierarchyIndex++;

		 int lastposition=-1;
		 if(g_HierarchyIndex>=0) lastposition = g_HierarchyList[g_HierarchyIndex];
		 int nextposition = g_HierarchyList[1+g_HierarchyIndex];
		 if(nextposition<=lastposition)
		 {
			 // Hierarchy Reverting to earlier tree position
			int returntolevel = g_HierarchyFrameList[1+nextposition];
			while(g_HierarchyFrameDepth>returntolevel && g_HierarchyFrameDepth>0)
			{
				pConvertBuffer->WriteFrameEnd();
				g_HierarchyFrameDepth--;
			}

			// Restore current matrix for this hierarhcy depth (store overwrites current trans frame when next frame created)
			g_HierarchyFrameMatrixStore = g_HierarchyFrameMatrices[g_HierarchyFrameDepth];
			if(g_HierarchyFrameDepth>0)
				g_HierarchyParentFrameMatrix = g_HierarchyFrameMatrices[g_HierarchyFrameDepth-1];
			else
				D3DUtil_SetIdentityMatrix(g_HierarchyParentFrameMatrix);
		 }
		 else
		 {
			 // Hierarchy goes deeper into tree
			 g_HierarchyParentFrameMatrix = g_HierarchyFrameMatrix;
		 }
	 }
	 else
	 {
		 pConvertBuffer->WriteFrameEnd();
	 }
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadBackgrChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {

   temp_int=ReadInt ();

       switch (temp_int)
       {
        case COL_RGB :
                      #ifdef __DEBUG__
                      printf (">> Found Color def (RGB) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadRGBColor ();
                      break;
        case COL_TRU :
                      #ifdef __DEBUG__
                      printf (">> Found Color def (24bit) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadTrueColor ();
                      break;
        default:      break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadAmbientChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case COL_RGB :
                      #ifdef __DEBUG__
                      printf (">>>> Found Color def (RGB) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadRGBColor ();
                      break;
        case COL_TRU :
                      #ifdef __DEBUG__
                      printf (">>>> Found Color def (24bit) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadTrueColor ();
                      break;
        default:      break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadColourChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case COL_RGB :
                      #ifdef __DEBUG__
                      printf (">>>> Found Color def (RGB) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadRGBColor ();
                      break;
        case COL_TRU :
                      #ifdef __DEBUG__
                      printf (">>>> Found Color def (24bit) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadTrueColor ();
                      break;
        default: break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadMapNameChunk(void)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	if(ReadLongName()==-1)
	{
		// No name
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadTexMapChunk (void)
{
	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long tellertje=6L; // 2 id + 4 pointer

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	while (end_found==FALSE)
	{
		temp_int=ReadInt();

		switch (temp_int)
		{
			case INT_PERCENTAGE :
				tellertje+=ReadIntPercentageChunk(); //returns g_float1
				break;

			case FLOAT_PERCENTAGE :
				tellertje+=ReadFloatPercentageChunk();			
				break;
			
			case MAT_MAPNAME:
				tellertje+=ReadMapNameChunk(); //returns temp_name
				break;

			default: 
				tellertje+=ReadUnknownChunk(0);
				break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long FindCameraChunk (void)
{
 long temp_pointer=0L;

 for (int i=0;i<12;i++)
  ReadInt ();

 temp_pointer=11L;
 temp_pointer=ReadName ();

 #ifdef __DEBUG__
 if (temp_pointer==-1)
   printf (">>>>* No Camera name found\n");
 #endif

 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadViewPortChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned int port,attribs;

 views_read++;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 attribs=ReadInt ();
 if (attribs==3)
 {
  #ifdef __DEBUG__
  printf ("<Snap> active in viewport\n");
  #endif
 }
 if (attribs==5)
 {
  #ifdef __DEBUG__
  printf ("<Grid> active in viewport\n");
  #endif
 }

 for (int i=1;i<6;i++) ReadInt (); // read 5 ints to get to the viewport

 port=ReadInt ();
 if ((port==0xFFFF) || (port==0))
 {
   FindCameraChunk ();
   port=CAMERA;
 }

 #ifdef __DEBUG__
 printf ("Reading [%s] information with id:%d\n",viewports [port],port);
 #endif

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadViewChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case EDIT_VIEW_P1 :
                           #ifdef __DEBUG__
                           printf (">>>> Found Viewport1 chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadViewPortChunk ();
                           break;
        case EDIT_VIEW_P2 :
                           #ifdef __DEBUG__
                           printf (">>>> Found Viewport2 (bogus) chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadUnknownChunk (EDIT_VIEW_P2);
                           break;
       case EDIT_VIEW_P3 :
                           #ifdef __DEBUG__
                           printf (">>>> Found Viewport chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadViewPortChunk ();
                           break;
        default           :break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;

   if (views_read>3)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadMatDefChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 if (ReadLongName ()==-1)
 {
   #ifdef __DEBUG__
   printf (">>>>* No Material name found\n");
   #endif
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadMaterialChunk (void)
{
	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long tellertje=6L;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	// Gather Data
	char		mat_name[256];
	float		mat_diffuse[3];
	float		mat_power;
	float		mat_specular[3];
	float		mat_TexturePercentage=-1;
	char		mat_TextureMapName[256];

	// lee - 040306 - u6rc5 - if material has no texture name, ensure it is a blank string
	strcpy ( mat_name, "" );
	strcpy ( mat_TextureMapName, "" );

	while (end_found==FALSE)
	{
		temp_int=ReadInt ();

		switch (temp_int)
		{
			case MAT_NAME01  :
				tellertje+=ReadMatDefChunk ();
				strcpy(mat_name, temp_name);
				break;

			case MAT_DIFFUSE  : 
				tellertje+=ReadColourChunk();
				mat_diffuse[0] = g_colorR;
				mat_diffuse[1] = g_colorG;
				mat_diffuse[2] = g_colorB;
				break;

			case MAT_SHININESS  : 
				tellertje+=ReadPercentageChunk();
				mat_power = g_float1;
				break;

			case MAT_SPECULAR  : 
				tellertje+=ReadColourChunk();
				mat_specular[0] = g_colorR;
				mat_specular[1] = g_colorG;
				mat_specular[2] = g_colorB;
				break;

			case MAT_TEXMAP :
				tellertje+=ReadTexMapChunk();
				mat_TexturePercentage = g_float1;
				strcpy(mat_TextureMapName, temp_name);
				break;

			default: tellertje+=ReadUnknownChunk(0); break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}


  
	
	// CREATE Material Item
	pConvertBuffer->WriteMaterialItem(	mat_name,
										mat_diffuse[0], mat_diffuse[1], mat_diffuse[2], 1.0f, 
										mat_power, 
										mat_specular[0], mat_specular[1], mat_specular[2],
										0.0f, 0.0f, 0.0f,
										mat_TexturePercentage, mat_TextureMapName);


	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadEditChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case EDIT_UNKNW01:tellertje+=ReadUnknownChunk (EDIT_UNKNW01);break;
        case EDIT_UNKNW02:tellertje+=ReadUnknownChunk (EDIT_UNKNW02);break;
        case EDIT_UNKNW03:tellertje+=ReadUnknownChunk (EDIT_UNKNW03);break;
        case EDIT_UNKNW04:tellertje+=ReadUnknownChunk (EDIT_UNKNW04);break;
        case EDIT_UNKNW05:tellertje+=ReadUnknownChunk (EDIT_UNKNW05);break;
        case EDIT_UNKNW06:tellertje+=ReadUnknownChunk (EDIT_UNKNW06);break;
        case EDIT_UNKNW07:tellertje+=ReadUnknownChunk (EDIT_UNKNW07);break;
        case EDIT_UNKNW08:tellertje+=ReadUnknownChunk (EDIT_UNKNW08);break;
        case EDIT_UNKNW09:tellertje+=ReadUnknownChunk (EDIT_UNKNW09);break;
        case EDIT_UNKNW10:tellertje+=ReadUnknownChunk (EDIT_UNKNW10);break;
        case EDIT_UNKNW11:tellertje+=ReadUnknownChunk (EDIT_UNKNW11);break;
        case EDIT_UNKNW12:tellertje+=ReadUnknownChunk (EDIT_UNKNW12);break;
        case EDIT_UNKNW13:tellertje+=ReadUnknownChunk (EDIT_UNKNW13);break;

        case EDIT_MATERIAL :
                            #ifdef __DEBUG__
                            printf (">>> Found Materials chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadMaterialChunk ();
                            break;
        case EDIT_VIEW1    :
                            #ifdef __DEBUG__
                            printf (">>> Found View main def chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadViewChunk ();
                            break;
        case EDIT_BACKGR   :
                            #ifdef __DEBUG__
                            printf (">>> Found Backgr chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadBackgrChunk ();
                            break;
        case EDIT_AMBIENT  :
                            #ifdef __DEBUG__
                            printf (">>> Found Ambient chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadAmbientChunk ();
                            break;
        case EDIT_OBJECT   :
                            #ifdef __DEBUG__
                            printf (">>> Found Object chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadObjectChunk ();
                            break;
        default:            break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 // When All Edit Objects done, close any remaining frame hierarchy brackets
 if(g_FindHierarchy==false)
 {
	 while(g_HierarchyFrameDepth>0)
	 {
		pConvertBuffer->WriteFrameEnd();
		g_HierarchyFrameDepth--;
	 }
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadFramesChunk(void)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	int StartFrame = ReadLong();
	int EndFrame = ReadLong();

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadNodeIdChunk(CAnimData* pAnimData)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	short NodeId = ReadInt();
	if(g_FindHierarchy==false)
		pAnimData->m_NodeId = NodeId;

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadNameAndHierarchyChunk(CAnimData* pAnimData)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	// Object name
	if(ReadLongName()==-1)
	{
		// No object name
	}
	else
	{
		if(g_FindHierarchy==false)
			strcpy(pAnimData->m_Name, temp_name);
	}

	if(strcmp(temp_name, "$$$DUMMY")==0)
	{
		// Object is dummy object
	}
	else
	{
		// Unknown Data
		ReadInt();
		ReadInt();

		short PositionNumber = ReadInt();

		// Add to Global Hierarchy
		if(g_FindHierarchy)
		{
			g_HierarchyList[g_HierarchyIndex] = PositionNumber;
			g_HierarchyFramePivots[g_HierarchyIndex] = GGVECTOR3(0,0,0);
			g_HierarchyIndex++;
		}
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadPivotChunk(void)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	// Pivot point
	D3DVECTOR Pivot;
	fread (&(Pivot.x),sizeof (float),1,bin3ds);
	fread (&(Pivot.z),sizeof (float),1,bin3ds);
	fread (&(Pivot.y),sizeof (float),1,bin3ds);
	
	// Add to Global Hierarchy
	if(g_FindHierarchy)
		g_HierarchyFramePivots[g_HierarchyIndex-1] = Pivot;

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadPosTagChunk(CAnimData* pAnimData)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	if(pAnimData)
	{
		// Unrequired Data
		ReadInt();
		ReadInt();
		ReadInt();
		ReadInt();
		ReadInt();
		short keys = ReadInt();
		ReadInt();
		float x, y, z;
		if(keys>0)
		{
			if(g_FindHierarchy==false)
			{
				pAnimData->m_KeyPosQuantity = keys;
			}
			for(int k=0; k<keys; k++)
			{
				short framenum = ReadInt();
				ReadLong();
				fread (&(x),sizeof (float),1,bin3ds);
				fread (&(y),sizeof (float),1,bin3ds);
				fread (&(z),sizeof (float),1,bin3ds);

				if(g_FindHierarchy==false)
				{
					CAnimDataFrame* pAnimDataFrame = new CAnimDataFrame;
					if(pAnimData->m_pFirstAnimPosFrame==NULL)
						pAnimData->m_pFirstAnimPosFrame=pAnimDataFrame;
					else
						pAnimData->m_pFirstAnimPosFrame->AddKeyFrame(pAnimDataFrame);

					pAnimDataFrame->m_frameid = framenum;
					pAnimDataFrame->m_x = x;
					pAnimDataFrame->m_y = z; // Z + Y transposed from 3ds format
					pAnimDataFrame->m_z = y;
				}
			}
		}
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadRotTagChunk(CAnimData* pAnimData)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	if(pAnimData)
	{
		// Unrequired Data
		ReadInt();
		ReadInt();
		ReadInt();
		ReadInt();
		ReadInt();
		short keys = ReadInt();
		ReadInt();

		GGQUATERNION absq;
		float rx, ry, rz, rangle;
		if(keys>0)
		{
			if(g_FindHierarchy==false)
			{
				pAnimData->m_KeyRotQuantity = keys;
			}
			for(int k=0; k<keys; k++)
			{
				short framenum = ReadInt();
				ReadLong();
				fread (&(rangle),sizeof (float),1,bin3ds);
				fread (&(rx),sizeof (float),1,bin3ds);
				fread (&(ry),sizeof (float),1,bin3ds);
				fread (&(rz),sizeof (float),1,bin3ds);
				GGVECTOR3 rvector = GGVECTOR3(-rx,-rz,-ry);

				if(g_FindHierarchy==false)
				{
					GGQUATERNION rq;
					
					D3DXQuaternionRotationAxis ( &rq, &rvector, rangle ); 

					CAnimDataFrame* pAnimDataFrame = new CAnimDataFrame;
					if(pAnimData->m_pFirstAnimRotFrame==NULL)
					{
						pAnimData->m_pFirstAnimRotFrame=pAnimDataFrame;
						absq=rq;
					}
					else
					{
						pAnimData->m_pFirstAnimRotFrame->AddKeyFrame(pAnimDataFrame);
						GGQUATERNION tempq = absq;
						
						D3DXQuaternionMultiply ( &absq, &tempq, &rq );
					}

					// COnvert Rotational Axis (from 3DS) to Quaternoin (for Xfile - absolute rotations)
					pAnimDataFrame->m_frameid = framenum;

					pAnimDataFrame->m_s = absq.w;
					pAnimDataFrame->m_x = absq.x;
					pAnimDataFrame->m_y = absq.y;
					pAnimDataFrame->m_z = absq.z;
				}
			}
		}
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadSclTagChunk(CAnimData* pAnimData)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	if(pAnimData)
	{
		// Unrequired Data
		ReadInt();
		ReadInt();
		ReadInt();
		ReadInt();
		ReadInt();
		short keys = ReadInt();
		ReadInt();
		float x, y, z;
		if(keys>0)
		{
			if(g_FindHierarchy==false)
			{
				pAnimData->m_KeySclQuantity = keys;
			}
			for(int k=0; k<keys; k++)
			{
				short framenum = ReadInt();
				ReadLong();
				fread (&(x),sizeof (float),1,bin3ds);
				fread (&(y),sizeof (float),1,bin3ds);
				fread (&(z),sizeof (float),1,bin3ds);
				
				if(g_FindHierarchy==false)
				{
					CAnimDataFrame* pAnimDataFrame = new CAnimDataFrame;
					if(pAnimData->m_pFirstAnimSclFrame==NULL)
						pAnimData->m_pFirstAnimSclFrame=pAnimDataFrame;
					else
						pAnimData->m_pFirstAnimSclFrame->AddKeyFrame(pAnimDataFrame);

					pAnimDataFrame->m_frameid = framenum;
					pAnimDataFrame->m_x = x;
					pAnimDataFrame->m_y = z;
					pAnimDataFrame->m_z = y;
				}
			}
		}
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK int ReadDummyLongName (void)
{
 unsigned int teller=0;
 unsigned char letter;

 strcpy (temp2_name,"Default name");

 letter=ReadChar ();
 if (letter==0) return (-1); // dummy object
 temp2_name [teller]=letter;
 teller++;

 do
 {
  letter=ReadChar ();
  temp2_name [teller]=letter;
  teller++;
 }
 while (letter!=0);

 temp2_name [teller-1]=0;

 return (0);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadDummyNameChunk(void)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	ReadDummyLongName();

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadObjDesChunk(void)
{
	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long tellertje=6L;
	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	CAnimData* pAnimData=NULL;
	if(g_FindHierarchy==false)
	{
		// Add x3ds_anim_ (node id, frame name, pos, rot, scl)
		pAnimData = new CAnimData;
	}

	while (end_found==FALSE)
	{
		temp_int=ReadInt ();

		switch (temp_int)
		{
			case NODE_ID:
				tellertje+=ReadNodeIdChunk(pAnimData);
				break;

			case OBJDES_NAH :
				tellertje+=ReadNameAndHierarchyChunk(pAnimData);
				break;

			case OBJDES_DUMMY :
				tellertje+=ReadDummyNameChunk();
				// No Dummy Objects
				if(pAnimData)
				{
					delete pAnimData;
					pAnimData=NULL;
				}
				break;

			case OBJDES_PIVOT :
				tellertje+=ReadPivotChunk();
				break;

			case OBJDES_POSTAG :
				tellertje+=ReadPosTagChunk(pAnimData);
				break;

			case OBJDES_ROTTAG :
				tellertje+=ReadRotTagChunk(pAnimData);
				break;

			case OBJDES_SCLTAG :
				tellertje+=ReadSclTagChunk(pAnimData);
				break;

			default:
				tellertje+=ReadUnknownChunk(0);
				break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}

	if(g_FindHierarchy==false)
	{
		if(pAnimData)
		{
			// Conclude by adding animdata to list
			if(g_pKeyframeAnimation==NULL)
				g_pKeyframeAnimation = pAnimData;
			else
				g_pKeyframeAnimation->AddAnimDataToList(pAnimData);
		}
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadKeyfChunk (void)
{
	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long tellertje=6L;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	while (end_found==FALSE)
	{
		temp_int=ReadInt ();

		switch (temp_int)
		{
			case KEYF_FRAMES   :
				tellertje+=ReadFramesChunk();
				break;

			case KEYF_OBJDES   :
				tellertje+=ReadObjDesChunk();
				break;

			case EDIT_VIEW1    :
				tellertje+=ReadViewChunk();
				break;

			default:
				tellertje+=ReadUnknownChunk(0);
				break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadMainChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
	temp_int=ReadInt ();

      switch (temp_int)
       {
        case KEYF3DS :
                      #ifdef __DEBUG__
                      printf (">> Found *Keyframer* chunk id of %0X\n",KEYF3DS);
                      #endif
                      tellertje+=ReadKeyfChunk ();
                      break;
        case EDIT3DS :
                      #ifdef __DEBUG__
                      printf (">> Found *Editor* chunk id of %0X\n",EDIT3DS);
                      #endif
                      tellertje+=ReadEditChunk ();
                      break;
        default:      break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
    end_found=TRUE;
}


 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position

 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK int ReadPrimaryChunk(void)
{
	unsigned char version = 0;

	if (ReadInt ()==MAIN3DS)
	{
		#ifdef __DEBUG__
		printf ("> Found Main chunk id of %0X\n",MAIN3DS);
		#endif

		//>---------- find version number
		fseek (bin3ds,28L,SEEK_SET);
		version=ReadChar ();

		fseek (bin3ds,2,SEEK_SET);
		ReadMainChunk ();

		// found so must end loop
		return 3;
	}
	else
		return (1);

	return (0);
}

DARKSDK void WriteAnimationDataToConvertBuffer(void)
{
	if(g_pKeyframeAnimation)
	{
		// Start AnimSet
		pConvertBuffer->WriteLine("AnimationSet x3ds_animset_0 {");

		// Anim Index Count
		int aicount=0;

		CAnimData* pAnimData = g_pKeyframeAnimation;
		while(pAnimData)
		{
			if(stricmp(pAnimData->m_Name, "$$$DUMMY")!=0)
			{
				// Start AnimFrame
				char string[256];
				wsprintf(string, "Animation x3ds_anim_%d {", aicount++);//pAnimData->m_NodeId);
		
				pConvertBuffer->WriteLine(string);

				// Frame Name
				//wsprintf(string, "{x3ds_%s}", pAnimData->m_Name);
				wsprintf(string, "{%s}", pAnimData->m_Name);
				
				pConvertBuffer->WriteLine(string);

				// ROTATION Keyframes
				if(pAnimData->m_KeyRotQuantity>0)
				{
					CAnimDataFrame* pAnimRotFrame = pAnimData->m_pFirstAnimRotFrame;

					
					pConvertBuffer->WriteLine("AnimationKey {");
					pConvertBuffer->WriteLine("0;");

					wsprintf(string, "%d;", pAnimData->m_KeyRotQuantity);

					pConvertBuffer->WriteLine(string);

					while(pAnimRotFrame)
					{
						// Keyframe DATA 
						sprintf(string, "%d; %d; %5.6f, %5.6f, %5.6f, %5.6f;;", pAnimRotFrame->m_frameid, 4, pAnimRotFrame->m_s, pAnimRotFrame->m_x, pAnimRotFrame->m_y, pAnimRotFrame->m_z);

						
						
						pConvertBuffer->WriteString(string);
						if(pAnimRotFrame->GetNext())
							pConvertBuffer->WriteString(",");
						else
							pConvertBuffer->WriteString(";");

						pConvertBuffer->WriteString("\n");
						//pConvertBuffer->WriteEOL();
						

						// Next Keyframe
						pAnimRotFrame = pAnimRotFrame->GetNext();
					}

					pConvertBuffer->WriteLine("}");
				}

				// SCALE Keyframes
				if(pAnimData->m_KeySclQuantity>0)
				{
					// Make sure first keyframe is not identity scale 1/1/1
					CAnimDataFrame* pAnimSclFrame = pAnimData->m_pFirstAnimSclFrame;
					if(pAnimData->m_KeySclQuantity==1 && pAnimSclFrame->m_x==1.0f && pAnimSclFrame->m_y==1.0f	&& pAnimSclFrame->m_z==1.0f)
					{
						// Basic Identity Scale found - no need to add to script
					}
					else
					{
						
						pConvertBuffer->WriteLine("AnimationKey {");
						pConvertBuffer->WriteLine("1;");

						wsprintf(string, "%d;", pAnimData->m_KeySclQuantity);

						
						 pConvertBuffer->WriteLine(string);

						while(pAnimSclFrame)
						{
							// Keyframe DATA 
							sprintf(string, "%d; %d; %5.6f, %5.6f, %5.6f;;", pAnimSclFrame->m_frameid, 3, pAnimSclFrame->m_x, pAnimSclFrame->m_y, pAnimSclFrame->m_z);

							
							
							pConvertBuffer->WriteString(string);
							if(pAnimSclFrame->GetNext())
								pConvertBuffer->WriteString(",");
							else
								pConvertBuffer->WriteString(";");
								
							pConvertBuffer->WriteString("\n");
							//pConvertBuffer->WriteEOL();
							

							// Next Keyframe
							pAnimSclFrame = pAnimSclFrame->GetNext();
						}

						
						pConvertBuffer->WriteLine("}");
					}
				}

				// POSITION Keyframes
				if(pAnimData->m_KeyPosQuantity>0)
				{
					CAnimDataFrame* pAnimPosFrame = pAnimData->m_pFirstAnimPosFrame;

					
					pConvertBuffer->WriteLine("AnimationKey {");
					pConvertBuffer->WriteLine("2;");

					wsprintf(string, "%d;", pAnimData->m_KeyPosQuantity);

					
					 pConvertBuffer->WriteLine(string);

					while(pAnimPosFrame)
					{
						// Keyframe DATA 
						sprintf(string, "%d; %d; %5.6f, %5.6f, %5.6f;;", pAnimPosFrame->m_frameid, 3, pAnimPosFrame->m_x, pAnimPosFrame->m_y, pAnimPosFrame->m_z);
						
						
						pConvertBuffer->WriteString(string);

						
						
						if(pAnimPosFrame->GetNext())
							pConvertBuffer->WriteString(",");
						else
							pConvertBuffer->WriteString(";");
						
						
						
						pConvertBuffer->WriteString("\n");
						//pConvertBuffer->WriteEOL();

						// Next Keyframe
						pAnimPosFrame = pAnimPosFrame->GetNext();
					}
					
					pConvertBuffer->WriteLine("}");
				}

				// End AnimFrame
				
				pConvertBuffer->WriteLine("}");
			}
		
			// Next Frame
			pAnimData = pAnimData->GetNext();
		}

		// End AnimSet
		
		pConvertBuffer->WriteLine("}");
	}
}

/*----------------------------------------------------------------------------*/
/*                      Test Main for the 3ds-bin lib                         */
/*----------------------------------------------------------------------------*/
DARKSDK int DB_Convert3DSBinary(char* Filename, char* DestFile)
{
	// Clear unititialised vars
	for(int n=0; n<5000; n++)
	{
		g_HierarchyList[n]=0;
		g_HierarchyFramePivots[n]=GGVECTOR3(0,0,0);
		g_HierarchyFrameList[n]=0;
		D3DUtil_SetIdentityMatrix(g_HierarchyFrameMatrices[n]);
	}

	// Pre-scan gets hierarchy data from file first
	
	pConvertBuffer = new CConvBuffer;
	bin3ds=fopen(Filename, "rb");
	if(bin3ds==NULL) return -4;

	g_HierarchyIndex = 0;
	g_FindHierarchy = true;
	int iResult=0;
	while((iResult=ReadPrimaryChunk())==0);
	g_HierarchyMax = g_HierarchyIndex;
	
	delete pConvertBuffer;
	pConvertBuffer=NULL;
	fclose(bin3ds);
	bin3ds=NULL;

//	// 3DS found to be less than Version 3.0
//	if(iResult==3)
//		return -3;

	// Open File
	bin3ds=fopen(Filename, "rb");
	if(bin3ds==NULL)
		return -4;

	// Create Converter Buffer
	pConvertBuffer = new CConvBuffer;
	pConvertBuffer->WriteHeader();
	
	// Clear animpointer to begin
	g_pKeyframeAnimation=NULL;

	// Read into Buffer
	g_HierarchyIndex = -1; // Use index as object counter for frame hierarchy construction
	g_HierarchyFrameDepth = 0;
	D3DUtil_SetIdentityMatrix(g_HierarchyFrameMatrix);
	D3DUtil_SetIdentityMatrix(g_HierarchyFrameMatrixStore);
	D3DUtil_SetIdentityMatrix(g_HierarchyParentFrameMatrix);
	g_FindHierarchy = false;
	while(ReadPrimaryChunk()==0);

	// Build animationset data
	WriteAnimationDataToConvertBuffer();

	// Delete animdata class
	if(g_pKeyframeAnimation)
	{
		delete g_pKeyframeAnimation;
		g_pKeyframeAnimation=NULL;
	}
	
	// Close File
	fclose(bin3ds);

	// Create X File
	DeleteFile(DestFile);
	HANDLE hfile = CreateFile(DestFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hfile!=INVALID_HANDLE_VALUE)
	{
		DWORD byteswritten;
		
		WriteFile(hfile, pConvertBuffer->GetMem(), pConvertBuffer->GetSize(), &byteswritten, NULL); 
		CloseHandle(hfile);
		hfile=NULL;
	}

	// Delete buffer object
	if(pConvertBuffer)
	{
		delete pConvertBuffer;
		pConvertBuffer = NULL;
	}
	

	return 0;
}


/*
//////////////////////////////////////////////////////////////////////////////////
// TO STOP LINKER ERRORS /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
DARKSDK float wrapangleoffset(float da)
{
	int breakout=100;
	while(da<0.0f || da>=360.0f)
	{
		if(da<0.0f) da=da+360.0f;
		if(da>=360.0f) da=da-360.0f;
		breakout--;
		if(breakout==0) break;
	}
	if(breakout==0) da=0.0f;
	return da;
}

DARKSDK void GetAngleFromPoint(float x1, float y1, float z1, float x2, float y2, float z2, float* ax, float* ay, float* az)
{
	GGVECTOR3 Vector;
	Vector.x = x2-x1;
	Vector.y = y2-y1;
	Vector.z = z2-z1;

	// Find Y and then X axis rotation
	double yangle=atan2(Vector.x, Vector.z);
	if(yangle<0.0) yangle+=D3DXToRadian(360.0);
	if(yangle>=D3DXToRadian(360.0)) yangle-=D3DXToRadian(360.0);

	GGMATRIX yrotate;
	D3DXMatrixRotationY ( &yrotate, (float)-yangle );
	D3DXVec3TransformCoord ( &Vector, &Vector, &yrotate );

	double xangle=-atan2(Vector.y, Vector.z);
	if(xangle<0.0) xangle+=D3DXToRadian(360.0);
	if(xangle>=D3DXToRadian(360.0)) xangle-=D3DXToRadian(360.0);

	*ax = wrapangleoffset(D3DXToDegree((float)xangle));
	*ay = wrapangleoffset(D3DXToDegree((float)yangle));
	*az = 0.0f;
}

DARKSDK void AnglesFromMatrix ( GGMATRIX* pmatMatrix, GGVECTOR3* pVecAngles )
{
	// Calculate angle vector based on matrix
	GGVECTOR3 pDirVec = GGVECTOR3 ( 0.0f, 0.0f, 1.0f );
	D3DXVec3TransformCoord ( &pDirVec, &pDirVec, pmatMatrix );
	GetAngleFromPoint ( 0.0f, 0.0f, 0.0f, pDirVec.x, pDirVec.y, pDirVec.z, &(pVecAngles->x), &(pVecAngles->y), &(pVecAngles->z));
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
*/

