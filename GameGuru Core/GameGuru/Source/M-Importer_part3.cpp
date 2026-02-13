void importer_check_for_physics_changes ( void )
{
	if (  t.importer.collisionShapeCount  ==  0  )  return;
	int tCount = 0;
	for ( tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
	{
			if (  t.snapPosX_f[tCount][1]  ==  -99999 ) 
			{
				t.importer.selectedCollisionObject = tCount;
				t.slidersmenuvalue[t.importer.properties2Index][7].value = t.importerCollision[t.importer.selectedCollisionObject].rotx;
				t.slidersmenuvalue[t.importer.properties2Index][8].value = t.importerCollision[t.importer.selectedCollisionObject].roty;
				t.slidersmenuvalue[t.importer.properties2Index][9].value = t.importerCollision[t.importer.selectedCollisionObject].rotz;
				importer_update_selection_markers ( );
			}
	}

	// coordinate system for importer grabbing collision box corners
	t.tadjustedtoareax_f = ((float)t.inputsys.xmouse / (float)GetDisplayWidth()) / ((float)GetDisplayWidth() / (float)GetChildWindowWidth(-1));
	t.tadjustedtoareay_f = ((float)t.inputsys.ymouse / (float)GetDisplayHeight()) / ((float)GetDisplayHeight() / (float)GetChildWindowHeight(-1));
	t.tadjustedtoareax_f = t.tadjustedtoareax_f*(GetDisplayWidth() + 0.0);
	t.tadjustedtoareay_f = t.tadjustedtoareay_f*(GetDisplayHeight() + 0.0);

	//  No marker selected yet
	if (  t.importer.collisionObjectMode  ==  0 ) 
	{
		if (  t.inputsys.mclick  ==  1 ) 
		{
			//  Have we clicked on a marker?
			t.importer.selectedMarker = 0;
			for ( tCount = 1 ; tCount<=  9; tCount++ )
			{
				t.picked = PickScreenObjectEx(t.tadjustedtoareax_f, t.tadjustedtoareay_f, t.selectedObjectMarkers[tCount], t.selectedObjectMarkers[tCount],1,0);
				ScaleObject (  t.selectedObjectMarkers[tCount],100,100,100 );
				SetObjectAmbience (  t.selectedObjectMarkers[tCount],0 );
				if (  tCount  !=  9 ) 
				{
					SetObjectEmissive (  t.selectedObjectMarkers[tCount], Rgb(255,255,0) );
				}
				else
				{
					SetObjectEmissive (  t.selectedObjectMarkers[tCount], Rgb(0,255,255) );
				}
				if (  t.picked  !=  0 ) 
				{
					//  Enlarge and recolor the selected corner marker
					t.importer.selectedMarker = tCount;
					t.tScaleX_f = t.inputsys.xmousemove ; t.tScaleY_f = t.inputsys.ymousemove;
					ScaleObject (  t.selectedObjectMarkers[tCount],100,100,100 );
					SetObjectAmbience (  t.selectedObjectMarkers[tCount],0 );
					if (  tCount  !=  9 ) 
					{
						SetObjectEmissive (  t.selectedObjectMarkers[tCount], Rgb(255,100,0) );
					}
					else
					{
						SetObjectEmissive (  t.selectedObjectMarkers[tCount], Rgb(0,100,255) );
					}

					float xpick = ((float)t.inputsys.xmouse / (float)GetDisplayWidth()) / ((float)GetDisplayWidth() / (float)GetChildWindowWidth(-1));
					float ypick = ((float)t.inputsys.ymouse / (float)GetDisplayHeight()) / ((float)GetDisplayHeight() / (float)GetChildWindowHeight(-1));

					//  then provide in a format for the pick-from-screen command
					xpick=xpick*(GetChildWindowWidth()+0.0);
					ypick=ypick*(GetChildWindowHeight()+0.0);
					PickScreen2D23D (  xpick , ypick , IMPORTERZPOSITION );
					t.timporterpickdepth_f =  LimbPositionZ(t.selectedObjectMarkers[t.importer.selectedMarker],0);
					t.timporterpickdepth_f = t.timporterpickdepth_f * t.tadjustedtoimporterxbase_f;
					PickScreen2D23D (  xpick, ypick, t.timporterpickdepth_f );
					t.timporteroldmousex = GetPickVectorX();
					t.timporteroldmousey = GetPickVectorZ();
				}
			}

			//  If we have not clicked on a marker, then perhaps another collision box?
			if (  t.importer.selectedMarker  ==  0 ) 
			{
				for ( tCount = 0 ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
				{
					if (  PickScreenObjectEx(t.tadjustedtoareax_f, t.tadjustedtoareay_f, t.importerCollision[tCount].object2, t.importerCollision[tCount].object2,1,0) ) 
					{
						t.importer.selectedCollisionObject = tCount;
						t.slidersmenuvalue[t.importer.properties2Index][7].value = ObjectAngleX(t.importerCollision[tCount].object2);
						t.slidersmenuvalue[t.importer.properties2Index][8].value = ObjectAngleY(t.importerCollision[tCount].object2);
						t.slidersmenuvalue[t.importer.properties2Index][9].value = ObjectAngleZ(t.importerCollision[tCount].object2);
						break;
					}
				}
			}
			else
			{
				t.importer.collisionObjectMode = 1;
			}
		}
	}
	else
	{
		//  Marker is selected, lets deal with it
		if (  t.inputsys.mclick  ==  1 ) 
		{
			for ( tCount = 1 ; tCount<=  9; tCount++ )
			{
				if (  t.importer.selectedMarker  !=  tCount  )  
				{
					ScaleObject (  t.selectedObjectMarkers[tCount],50,50,50 );
				}
			}

			importer_hide_mouse ( );

			t.tXOnly = 1;
			t.tYOnly = 1;
			t.tZOnly = 1;
			t.importer.message = "Hold X,C or V to lock an axis";
			if (  t.inputsys.kscancode  ==  88 || t.inputsys.kscancode  ==  67 || t.inputsys.kscancode  ==  86 ) 
			{
				t.tXOnly = 0 ; t.tYOnly = 0 ; t.tZOnly = 0;
			}
			if (  t.inputsys.kscancode  ==  88  )  t.tXOnly  =  1;
			if (  t.inputsys.kscancode  ==  67  )  t.tYOnly  =  1;
			if (  t.inputsys.kscancode  ==  86  )  t.tZOnly  =  1;

			t.tScaleX_f = 0.0;
			t.tScaleY_f = 0.0;
			t.tScaleZ_f = 0.0;
			t.tMultiX_f = 1.0;
			t.tMultiY_f = 1.0;
			t.tMultiZ_f = 1.0;

			t.tr1_f = t.slidersmenuvalue[t.importer.properties2Index][7].value;
			t.tr2_f = t.slidersmenuvalue[t.importer.properties2Index][8].value;
			t.tr3_f = t.slidersmenuvalue[t.importer.properties2Index][9].value;

			RotateObject (  t.selectedObjectMarkers[1], t.tr1x_f+360,t.tr2_f+360,t.tr3_f+360 );

			if (  t.importer.selectedMarker  ==  1 ) 
			{

				t.tSnapObject = t.selectedObjectMarkers[1];

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[1]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[1]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[1]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[2]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[3]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[5]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[1],90 );
					MoveObject (  t.selectedObjectMarkers[1], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[1], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapLeft ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[1],90 );
					MoveObject (  t.selectedObjectMarkers[1], -t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[1],-90 );
					MoveObject (  t.selectedObjectMarkers[1], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[1],90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[1],180 );
					MoveObject (  t.selectedObjectMarkers[1], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[1], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[1],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[1],180 );
					MoveObject (  t.selectedObjectMarkers[1], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[1], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[1],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[1]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[1]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[1]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.oldz2_f > t.oldz1_f ) 
				{
					t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
				}
				else
				{
					t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
				}
			}

			if (  t.importer.selectedMarker  ==  2 ) 
			{
				t.tSnapObject = t.selectedObjectMarkers[2];
				t.tMultiX_f = -1;
				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[2]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[2]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[2]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[1]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[4]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[6]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[2],90 );
					MoveObject (  t.selectedObjectMarkers[2], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[2], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[2],180 );
						importer_snapLeft ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[2],90 );
					MoveObject (  t.selectedObjectMarkers[2], t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[2],-90 );
					MoveObject (  t.selectedObjectMarkers[2], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[2],180 );
						TurnObjectLeft (  t.selectedObjectMarkers[2],90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[2],180 );
					MoveObject (  t.selectedObjectMarkers[2], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[2], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[2],180 );
					MoveObject (  t.selectedObjectMarkers[2], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[2], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[2]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[2]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[2]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.importer.viewMessage  !=  "Top" ) 
				{
					if (  t.oldz2_f > t.oldz1_f ) 
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
				}
				else
				{
					if (  t.oldz2_f > t.oldz1_f ) 
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
				}
			}

			if (  t.importer.selectedMarker  ==  3 ) 
			{
				t.tSnapObject = t.selectedObjectMarkers[3];

				t.tMultiY_f = -1.0;

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[3]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[3]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[3]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[4]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[1]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[8]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[3],90 );
					MoveObject (  t.selectedObjectMarkers[3], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[3], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapLeft ( );
						RollObjectRight (  t.selectedObjectMarkers[3],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[3],90 );
					MoveObject (  t.selectedObjectMarkers[3], -t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[3],-90 );
					MoveObject (  t.selectedObjectMarkers[3], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[3],90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[3],180 );
					MoveObject (  t.selectedObjectMarkers[3], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[3], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[3],180 );
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[3],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[3],180 );
					MoveObject (  t.selectedObjectMarkers[3], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[3], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectRight (  t.selectedObjectMarkers[3],180 );
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[3],180 );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[3]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[3]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[3]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.oldz2_f > t.oldz1_f ) 
				{
					t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
				}
				else
				{
					t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
				}

			}

			if (  t.importer.selectedMarker  ==  4 ) 
			{

				t.tSnapObject = t.selectedObjectMarkers[4];

				t.tMultiX_f = -1;
				t.tMultiY_f = -1;

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[4]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[4]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[4]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[3]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[2]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[5]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[4],90 );
					MoveObject (  t.selectedObjectMarkers[4], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[4], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[4],180 );
						importer_snapLeft ( );
						RollObjectRight (  t.selectedObjectMarkers[4],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[4],90 );
					MoveObject (  t.selectedObjectMarkers[4], t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[4],-90 );
					MoveObject (  t.selectedObjectMarkers[4], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[4],-90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[4],180 );
					MoveObject (  t.selectedObjectMarkers[4], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[4], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[4],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[4],180 );
					MoveObject (  t.selectedObjectMarkers[4], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[4], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[4],180 );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[4]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[4]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[4]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.importer.viewMessage  !=  "Top" ) 
				{
					if (  t.oldz2_f > t.oldz1_f ) 
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
				}
				else
				{
					if (  t.oldz2_f > t.oldz1_f ) 
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
				}
			}

			if (  t.importer.selectedMarker  ==  5 ) 
			{

				t.tSnapObject = t.selectedObjectMarkers[5];

				t.tMultiZ_f = -1;

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[5]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[5]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[5]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[6]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[7]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[1]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[5],90 );
					MoveObject (  t.selectedObjectMarkers[5], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[5], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapLeft ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[5],90 );
					MoveObject (  t.selectedObjectMarkers[5], -t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[5],-90 );
					MoveObject (  t.selectedObjectMarkers[5], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[5],90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[5],180 );
					MoveObject (  t.selectedObjectMarkers[5], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[5], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[5],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[5],180 );
					MoveObject (  t.selectedObjectMarkers[5], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[5], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[5],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[5]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[5]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[5]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.oldz2_f < t.oldz1_f ) 
				{
					t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
				}
				else
				{
					t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
				}

			}


			if (  t.importer.selectedMarker  ==  6 ) 
			{

				t.tSnapObject = t.selectedObjectMarkers[6];

				t.tMultiX_f = -1;
				t.tMultiZ_f = -1;

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[6]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[6]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[6]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[5]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[8]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[2]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[6],90 );
					MoveObject (  t.selectedObjectMarkers[6], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[6], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[6],180 );
						importer_snapLeft ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[6],90 );
					MoveObject (  t.selectedObjectMarkers[6], t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[6],-90 );
					MoveObject (  t.selectedObjectMarkers[6], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[6],180 );
						TurnObjectLeft (  t.selectedObjectMarkers[6],90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[6],180 );
					MoveObject (  t.selectedObjectMarkers[6], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[6], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[6],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[6],180 );
					MoveObject (  t.selectedObjectMarkers[6], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[6], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[6],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[6]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[6]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[6]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.importer.viewMessage  !=  "Top" ) 
				{
					if (  t.oldz2_f > t.oldz1_f ) 
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
				}
				else
				{
					if (  t.oldz2_f < t.oldz1_f ) 
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
				}

			}

			if (  t.importer.selectedMarker  ==  7 ) 
			{

				t.tSnapObject = t.selectedObjectMarkers[7];

				t.tMultiZ_f = -1;
				t.tMultiY_f = -1;

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[7]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[7]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[7]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[4]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[2]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[1]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[7],90 );
					MoveObject (  t.selectedObjectMarkers[7], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[7], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[7],180 );
						importer_snapLeft ( );
						RollObjectRight (  t.selectedObjectMarkers[7],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[7],90 );
					MoveObject (  t.selectedObjectMarkers[7], t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[7],-90 );
					MoveObject (  t.selectedObjectMarkers[7], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[7],180+90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[7],180 );
					MoveObject (  t.selectedObjectMarkers[7], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[7], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[7],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[7],180 );
					MoveObject (  t.selectedObjectMarkers[7], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[7], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[7],180 );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[7]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[7]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[7]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f < t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.importer.viewMessage  !=  "Top" ) 
				{
					if (  t.oldz2_f < t.oldz1_f ) 
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
				}
				else
				{
					if (  t.oldz2_f < t.oldz1_f ) 
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
				}
			}

			if (  t.importer.selectedMarker  ==  8 ) 
			{

				t.tSnapObject = t.selectedObjectMarkers[8];

				t.tMultiX_f = -1;
				t.tMultiZ_f = -1;
				t.tMultiY_f = -1;

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[8]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[8]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[8]);
				t.oldx2_f = ObjectPositionX(t.selectedObjectMarkers[7]);
				t.oldy2_f = ObjectPositionY(t.selectedObjectMarkers[6]);
				t.oldz2_f = ObjectPositionZ(t.selectedObjectMarkers[1]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[8],90 );
					MoveObject (  t.selectedObjectMarkers[8], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[8], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[8],180 );
						importer_snapLeft ( );
						RollObjectRight (  t.selectedObjectMarkers[8],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[8],90 );
					MoveObject (  t.selectedObjectMarkers[8], t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[8],-90 );
					MoveObject (  t.selectedObjectMarkers[8], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[8],-90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					TurnObjectLeft (  t.selectedObjectMarkers[8],180 );
					MoveObject (  t.selectedObjectMarkers[8], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[8], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[8],180 );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					TurnObjectRight (  t.selectedObjectMarkers[8],180 );
					MoveObject (  t.selectedObjectMarkers[8], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[8], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						RollObjectRight (  t.selectedObjectMarkers[8],180 );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[8]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[8]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[8]);

				t.dist1x_f = GetDistance( t.oldx1_f,0,0,t.oldx2_f,0,0);
				t.dist2x_f = GetDistance( t.newx1_f,0,0,t.oldx2_f,0,0);

				//  X plane
				if (  t.oldx2_f > t.oldx1_f ) 
				{
					t.tScaleX_f = -(t.dist2x_f - t.dist1x_f);
				}
				else
				{
					t.tScaleX_f = (t.dist2x_f - t.dist1x_f);
				}

				//  Y plane
				t.dist1y_f = GetDistance( t.oldy1_f,0,0,t.oldy2_f,0,0);
				t.dist2y_f = GetDistance( t.newy1_f,0,0,t.oldy2_f,0,0);

				if (  t.oldy2_f > t.oldy1_f ) 
				{
					t.tScaleY_f = -(t.dist2y_f - t.dist1y_f);
				}
				else
				{
					t.tScaleY_f = (t.dist2y_f - t.dist1y_f);
				}

				//  Z plane
				t.dist1z_f = GetDistance( t.oldz1_f,0,0,t.oldz2_f,0,0);
				t.dist2z_f = GetDistance( t.newz1_f,0,0,t.oldz2_f,0,0);

				if (  t.importer.viewMessage  !=  "Top" ) 
				{
					if (  t.oldz2_f < t.oldz1_f ) 
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
				}
				else
				{
					if (  t.oldz2_f < t.oldz1_f ) 
					{
						t.tScaleZ_f = -(t.dist2z_f - t.dist1z_f);
					}
					else
					{
						t.tScaleZ_f = (t.dist2z_f - t.dist1z_f);
					}
				}
			}

			if (  t.importer.selectedMarker  ==  9 ) 
			{
				t.tSnapObject = t.selectedObjectMarkers[1];

				t.oldx1_f = ObjectPositionX(t.selectedObjectMarkers[9]);
				t.oldy1_f = ObjectPositionY(t.selectedObjectMarkers[9]);
				t.oldz1_f = ObjectPositionZ(t.selectedObjectMarkers[9]);

				if (  t.importer.viewMessage  ==  "Front" ) 
				{
					RotateObject (  t.selectedObjectMarkers[9],0,0,0 );
					if (t.importer.selectedCollisionObject >= 0) {
						TurnObjectLeft(t.selectedObjectMarkers[9], ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object));
						PitchObjectUp(t.selectedObjectMarkers[9], -ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object));
						RollObjectLeft(t.selectedObjectMarkers[9], ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object));
					}
					TurnObjectLeft (  t.selectedObjectMarkers[9],90 );
					MoveObject (  t.selectedObjectMarkers[9], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[9], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapLeft ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Top" ) 
				{
					if (t.importer.selectedCollisionObject >= 0) {
						RotateObject(t.selectedObjectMarkers[9], -ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object), ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object), -ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object));
					}
					RotateObject (  t.selectedObjectMarkers[9],0,0,0 );
					if (t.importer.selectedCollisionObject >= 0) {
						TurnObjectLeft(t.selectedObjectMarkers[9], ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object));
						PitchObjectUp(t.selectedObjectMarkers[9], -ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object));
						RollObjectLeft(t.selectedObjectMarkers[9], ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object));
					}
					TurnObjectLeft (  t.selectedObjectMarkers[9],90 );
					MoveObject (  t.selectedObjectMarkers[9], -t.inputsys.xmousemove );
					TurnObjectLeft (  t.selectedObjectMarkers[9],-90 );
					MoveObject (  t.selectedObjectMarkers[9], -t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						importer_snapforward ( );
						TurnObjectLeft (  t.selectedObjectMarkers[9],90 );
						importer_snapLeft ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Left" ) 
				{
					RotateObject (  t.selectedObjectMarkers[9],0,0,0 );
					if (t.importer.selectedCollisionObject >= 0) {
						TurnObjectLeft(t.selectedObjectMarkers[9], ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object));
						PitchObjectUp(t.selectedObjectMarkers[9], -ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object));
						RollObjectLeft(t.selectedObjectMarkers[9], ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object));
					}
					TurnObjectLeft (  t.selectedObjectMarkers[9],180 );
					MoveObject (  t.selectedObjectMarkers[9], t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[9], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[9],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}
				if (  t.importer.viewMessage  ==  "Right" ) 
				{
					RotateObject (  t.selectedObjectMarkers[9],0,0,0 );
					if (t.importer.selectedCollisionObject >= 0) {
						TurnObjectLeft(t.selectedObjectMarkers[9], ObjectAngleY(t.importerCollision[t.importer.selectedCollisionObject].object));
						PitchObjectUp(t.selectedObjectMarkers[9], -ObjectAngleX(t.importerCollision[t.importer.selectedCollisionObject].object));
						RollObjectLeft(t.selectedObjectMarkers[9], ObjectAngleZ(t.importerCollision[t.importer.selectedCollisionObject].object));
					}
					TurnObjectRight (  t.selectedObjectMarkers[9],180 );
					MoveObject (  t.selectedObjectMarkers[9], -t.inputsys.xmousemove );
					MoveObjectUp (  t.selectedObjectMarkers[9], t.inputsys.ymousemove );
					if (  t.inputsys.keyshift ) 
					{
						TurnObjectLeft (  t.selectedObjectMarkers[9],180 );
						importer_snapforward ( );
						importer_snapUp ( );
					}
				}

				t.newx1_f = ObjectPositionX(t.selectedObjectMarkers[9]);
				t.newy1_f = ObjectPositionY(t.selectedObjectMarkers[9]);
				t.newz1_f = ObjectPositionZ(t.selectedObjectMarkers[9]);

				t.tmoveX_f = t.newx1_f - t.oldx1_f;
				t.tmoveY_f = t.newy1_f - t.oldy1_f;
				t.tmoveZ_f = t.newz1_f - t.oldz1_f;
	
				if (  t.tXOnly  ==  0  )  t.tmoveX_f  =  0;
				if (  t.tYOnly  ==  0  )  t.tmoveY_f  =  0;
				if (  t.tZOnly  ==  0  )  t.tmoveZ_f  =  0;
	
				if (t.importer.selectedCollisionObject >= 0) {
					MoveObjectRight(t.importerCollision[t.importer.selectedCollisionObject].object, t.tmoveX_f);
					MoveObjectDown(t.importerCollision[t.importer.selectedCollisionObject].object, t.tmoveY_f);
					MoveObject(t.importerCollision[t.importer.selectedCollisionObject].object, t.tmoveZ_f);
					ShowObject(t.importerCollision[t.importer.selectedCollisionObject].object);
					SetObjectLight(t.importerCollision[t.importer.selectedCollisionObject].object, 1);
					ColorObject(t.importerCollision[t.importer.selectedCollisionObject].object, Rgb(0, 0, 0));
					SetObjectAmbience(t.importerCollision[t.importer.selectedCollisionObject].object, 0);
					SetObjectEmissive(t.importerCollision[t.importer.selectedCollisionObject].object, Rgb(100, 100, 0));

					MoveObjectRight(t.importerCollision[t.importer.selectedCollisionObject].object2, t.tmoveX_f);
					MoveObjectDown(t.importerCollision[t.importer.selectedCollisionObject].object2, t.tmoveY_f);
					MoveObject(t.importerCollision[t.importer.selectedCollisionObject].object2, t.tmoveZ_f);
				}
				return;
			}

			//  If any axis are not locked allow movement
			if (  t.tXOnly  ==  0  )  t.tScaleX_f  =  0;
			if (  t.tYOnly  ==  0  )  t.tScaleY_f  =  0;
			if (  t.tZOnly  ==  0  )  t.tScaleZ_f  =  0;

			//  stop the boxes becomes too small or inverted
			if (t.importer.selectedCollisionObject >= 0) {
				if (LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) - t.tScaleX_f < 5)
				{
					t.tScaleX_f = -(5.0 - LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object2, 0));
				}
				if (LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) - t.tScaleY_f < 5)
				{
					t.tScaleY_f = -(5.0 - LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object2, 0));
				}
				if (LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) + t.tScaleZ_f < 5)
				{
					t.tScaleZ_f = (5.0 - LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object2, 0));
				}

				ScaleLimb(t.importerCollision[t.importer.selectedCollisionObject].object, 0, LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) - t.tScaleX_f, LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) - t.tScaleY_f, LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) + t.tScaleZ_f);
				t.importerCollision[t.importer.selectedCollisionObject].sizex = LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object, 0) - t.tScaleX_f;
				t.importerCollision[t.importer.selectedCollisionObject].sizey = LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object, 0) - t.tScaleY_f;
				t.importerCollision[t.importer.selectedCollisionObject].sizez = LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object, 0) + t.tScaleZ_f;
				MoveObjectRight(t.importerCollision[t.importer.selectedCollisionObject].object, t.tScaleX_f * t.tMultiX_f / 2.0);
				MoveObjectDown(t.importerCollision[t.importer.selectedCollisionObject].object, t.tScaleY_f * t.tMultiY_f / 2.0);
				MoveObject(t.importerCollision[t.importer.selectedCollisionObject].object, -t.tScaleZ_f * t.tMultiZ_f / 2.0);
				ShowObject(t.importerCollision[t.importer.selectedCollisionObject].object);

				SetObjectLight(t.importerCollision[t.importer.selectedCollisionObject].object, 1);
				ColorObject(t.importerCollision[t.importer.selectedCollisionObject].object, Rgb(0, 0, 0));
				SetObjectAmbience(t.importerCollision[t.importer.selectedCollisionObject].object, 0);
				SetObjectEmissive(t.importerCollision[t.importer.selectedCollisionObject].object, Rgb(100, 100, 0));

				ScaleLimb(t.importerCollision[t.importer.selectedCollisionObject].object2, 0, LimbScaleX(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) - t.tScaleX_f, LimbScaleY(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) - t.tScaleY_f, LimbScaleZ(t.importerCollision[t.importer.selectedCollisionObject].object2, 0) + t.tScaleZ_f);
				MoveObjectRight(t.importerCollision[t.importer.selectedCollisionObject].object2, t.tScaleX_f * t.tMultiX_f / 2.0);
				MoveObjectDown(t.importerCollision[t.importer.selectedCollisionObject].object2, t.tScaleY_f * t.tMultiY_f / 2.0);
				MoveObject(t.importerCollision[t.importer.selectedCollisionObject].object2, -t.tScaleZ_f * t.tMultiZ_f / 2.0);
			}
		}
		else
		{

			//  Mouse released, come out of editing
			importer_show_mouse ( );
			t.timporterpickdepth_f = 1250;
			if (t.importer.selectedCollisionObject >= 0) {
				HideObject(t.importerCollision[t.importer.selectedCollisionObject].object);
			}
			for ( tCount = 1 ; tCount<=  9; tCount++ )
			{
				t.picked = PickScreenObjectEx(t.importer.MouseX, t.importer.MouseY, t.selectedObjectMarkers[tCount], t.selectedObjectMarkers[tCount],1,0);
				ScaleObject (  t.selectedObjectMarkers[tCount],100,100,100 );
				SetObjectAmbience (  t.selectedObjectMarkers[tCount],0 );
				if (  tCount  !=  9 ) 
				{
					SetObjectEmissive (  t.selectedObjectMarkers[tCount], Rgb(255,255,0) );
				}
				else
				{
					SetObjectEmissive (  t.selectedObjectMarkers[tCount], Rgb(0,255,255) );
				}

			}
			t.importer.selectedMarker = 0;
			t.importer.collisionObjectMode = 0;
		}
	}
}

void importer_update_textures ( void )
{
	// count how many images are specified in image list
	int iImageCount = 0;
	for ( int tCount = 1 ; tCount <= IMPORTERTEXTURESMAX; tCount++ )
		if ( strlen ( t.importerTextures[tCount].fileName.Get() ) > 0 )
			iImageCount++;

	// show all textures associated with this model import (so user can change textures used)
	strcpy ( g_pShowFilenameHoveringOver, "" );
	for ( int tCount = 1 ; tCount <= IMPORTERTEXTURESMAX; tCount++ )
	{
		// skip image slots that have no filename
		if ( strlen ( t.importerTextures[tCount].fileName.Get() ) == 0 )
			continue;

		// determine what texture is shown in UI (if any)
		int iTexSlotImage = t.importerTextures[tCount].imageID;

		// work out texture panel position
		t.tOffsetX = 10;
		if (  tCount > 5  )  t.tOffsetX  =  10+128;
		if (  tCount > 10  )  t.tOffsetX  =  10+(128*2);
		if (  tCount > 15  )  t.tOffsetX  =  10+(128*3);
		if (  tCount > 20  )  t.tOffsetX  =  10+(128*4);
		//if (  t.importer.scaleMulti  !=  1.0  )  t.tOffsetX  =  0;
		t.tOffsetY = tCount;
		if (  tCount > 5  )  t.tOffsetY  =  tCount-5;
		if (  tCount > 10  )  t.tOffsetY  =  tCount-10;
		if (  tCount > 15  )  t.tOffsetY  =  tCount-15;
		if (  tCount > 20  )  t.tOffsetY  =  tCount-20;
		int iVertical = (t.tOffsetY-1);
		t.tOffsetY = 10 + (iVertical * 128);

		if (!bRemoveSprites) {
			if (t.importer.scaleMulti != 1.0)
				MAXSprite(t.importerTextures[tCount].spriteID2, t.tOffsetX, (GetChildWindowHeight() / 2) - 400 + t.tOffsetY - 19 + 20, g.importermenuimageoffset + 7);
			else
				MAXSprite(t.importerTextures[tCount].spriteID2, (GetChildWindowWidth() / 2) - 430 - t.tOffsetX - 19 - 20, (GetChildWindowHeight() / 2) - 400 + t.tOffsetY - 19 + 20, g.importermenuimageoffset + 7);
			SizeSprite(t.importerTextures[tCount].spriteID2, 128, 128);

			if (iTexSlotImage > 0)
			{
				if (t.importer.scaleMulti != 1.0)
					MAXSprite(t.importerTextures[tCount].spriteID, t.tOffsetX + 20, (GetChildWindowHeight() / 2) - 400 + t.tOffsetY + 20, iTexSlotImage);
				else
					MAXSprite(t.importerTextures[tCount].spriteID, (GetChildWindowWidth() / 2) - 430 - t.tOffsetX - 20, (GetChildWindowHeight() / 2) - 400 + t.tOffsetY + 20, iTexSlotImage);
				SizeSprite(t.importerTextures[tCount].spriteID, 90, 90);
				SetSpritePriority(t.importerTextures[tCount].spriteID, 1);
			}
			else
			{
				MAXSprite(t.importerTextures[tCount].spriteID, -10000, -10000, g.importermenuimageoffset + 7);
			}

			if (t.importer.MouseX >= SpriteX(t.importerTextures[tCount].spriteID2) - 5 && t.importer.MouseY >= (GetChildWindowHeight() / 2) - 400 + (iVertical * 128) + 5)
			{
				if (t.importer.MouseX <= SpriteX(t.importerTextures[tCount].spriteID2) + 128 - 5 && t.importer.MouseY <= 90 + (GetChildWindowHeight() / 2) - 400 + (iVertical * 128) + 30)
				{
					strcpy(g_pShowFilenameHoveringOver, t.importerTextures[tCount].fileName.Get());
					if (t.inputsys.mclick == 0)
					{
						if (t.importer.scaleMulti != 1.0)
							MAXSprite(t.importerTextures[tCount].spriteID2, t.tOffsetX, (GetChildWindowHeight() / 2) - 400 + t.tOffsetY - 19 + 20, g.importermenuimageoffset + 7);
						else
							MAXSprite(t.importerTextures[tCount].spriteID2, (GetChildWindowWidth() / 2) - 430 - t.tOffsetX - 19 - 20, (GetChildWindowHeight() / 2) - 400 + t.tOffsetY - 19 + 20, g.importermenuimageoffset + 7);
						SizeSprite(t.importerTextures[tCount].spriteID2, 128, 128);
						SizeSprite(t.importerTextures[tCount].spriteID, 106, 106);
						if (t.importer.scaleMulti == 1.0)
						{
							MAXSprite(t.importerTextures[tCount].spriteID, (GetChildWindowWidth() / 2) - 430 - 8 - 20, (GetChildWindowHeight() / 2) - 400 + tCount * 128 - 8 + 20, iTexSlotImage);
						}
						else
						{
							if (iTexSlotImage > 0)
							{
								MAXSprite(t.importerTextures[tCount].spriteID, t.tOffsetX + 10, (GetChildWindowHeight() / 2) - 400 + (iVertical * 128) - 8 + 20, iTexSlotImage);
							}
						}
						SetSpritePriority(t.importerTextures[tCount].spriteID, 1);
					}
					else
					{
						if (t.importer.oldMouseClick == 0)
						{
							t.tFileName_s = openFileBox("PNG|*.png|DDS|*.dds|JPEG|*.jpg|BMP|*.bmp|All Files|*.*|", "", "Open Texture", ".dds", IMPORTEROPENFILE);
							if (t.tFileName_s == "Error")  return;
							if (FileExist(t.tFileName_s.Get()) == 1)
							{
								// prompt as this may take some seconds
								LPSTR pDelayPrompt = "Loading chosen texture and associated files";
								for (int iSyncPass = 0; iSyncPass < 2; iSyncPass++)
								{
									pastebitmapfont(pDelayPrompt, (GetChildWindowWidth() / 2) - (getbitmapfontwidth(pDelayPrompt, 1) / 2), 860, 1, 255);
									Sync();
								}

								// find free image
								t.tImageID = t.importerTextures[tCount].imageID;
								if (t.tImageID == 0)
								{
									t.tImageID = g.importermenuimageoffset + 15;
									while (ImageExist(t.tImageID) == 1) ++t.tImageID;
								}

								// can expand out a color texture once (to add normal/gloss/etc)
								bool bExpandOutPBRTextureSet = false;

								// replace image details
								if (ImageExist(t.tImageID) == 1) DeleteImage(t.tImageID);
								LoadImage(t.tFileName_s.Get(), t.tImageID);
								if (ImageExist(t.tImageID) == 1)
								{
									// remove any previous references to associated files for the old filename
									if (t.importerTextures[tCount].iExpandedThisSlot == 0)
									{
										// but only if its a base color texutre
										char pIsItTexColor[2048];
										strcpy(pIsItTexColor, t.importerTextures[tCount].fileName.Get());
										if (strlen(pIsItTexColor) > 1 + 8 + 4)
										{
											pIsItTexColor[strlen(pIsItTexColor) - 4] = 0;
											if (strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 2, "_d", 2) == NULL
												|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 6, "_color", 6) == NULL
												|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 8, "_diffuse", 8) == NULL
												|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 7, "_albedo", 7) == NULL
												|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 8, "blankTex", 8) == NULL)
											{
												// for both!
												strcpy(pIsItTexColor, t.tFileName_s.Get());
												if (strlen(pIsItTexColor) > 1 + 8 + 4)
												{
													pIsItTexColor[strlen(pIsItTexColor) - 4] = 0;
													if (strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 2, "_d", 2) == NULL
														|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 6, "_color", 6) == NULL
														|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 8, "_diffuse", 8) == NULL
														|| strnicmp(pIsItTexColor + strlen(pIsItTexColor) - 7, "_albedo", 7) == NULL)
													{
														importer_removeentryandassociatesof(tCount);
														t.importerTextures[tCount].iExpandedThisSlot = 1;
														bExpandOutPBRTextureSet = true;
													}
												}
											}
										}
									}

									// update image list data
									t.importerTextures[tCount].fileName = t.tFileName_s;
									t.importerTextures[tCount].imageID = t.tImageID;
								}

								// ensure single texture is specified in FPE
								if (iImageCount == 1)
								{
									t.importer.objectFPE.textured = t.tFileName_s;
								}

								// reapply texture to model
								importer_applyimagelisttextures(false, tCount, bExpandOutPBRTextureSet);
								importer_recreate_texturesprites();
							}
						}
					}
				}
			}
		}
	}
	SetDir (  t.importer.startDir.Get() );
}

void importer_load_textures_finish ( int tCount, bool bCubeMapOnly )
{
	// final image load count, load textures and sort txture button sprite
	t.tcounttextures = tCount;

	// Load textures
	for ( tCount = 1; tCount <= t.tcounttextures; tCount++ )
	{
		t.tImageID = g.importermenuimageoffset+15;
		while ( ImageExist(t.tImageID) == 1 ) ++t.tImageID;
		LoadImage ( t.importerTextures[tCount].fileName.Get(), t.tImageID );
		if ( ImageExist ( t.tImageID ) == 1 )
			t.importerTextures[tCount].imageID = t.tImageID;
		else
			t.importerTextures[tCount].imageID = 0;
	}
	
	// Apply textures to model
	importer_applyimagelisttextures ( bCubeMapOnly, -1, true );
	importer_recreate_texturesprites();
}

void importer_load_textures ( void )
{
	// Clean importer Textures List
	int tCount = 0;
	for ( tCount = 1 ; tCount <= IMPORTERTEXTURESMAX; tCount++ )
	{
		t.importerTextures[tCount].imageID = 0;
		t.importerTextures[tCount].fileName = "";
		t.importerTextures[tCount].iExpandedThisSlot = 0;		
		t.importerTextures[tCount].iOptionalStage = 0;
		t.importerTextures[tCount].iAssociatedBaseImage = 0;
	}

	// Byte scan of file - good for TXT or FBX based model files
	t.filesize = FileSize(t.timporterfile_s.Get());
	t.mbi=255;
	OpenToRead (  11,t.timporterfile_s.Get() );
	if (FileOpen(11) == 1)
	{
		MakeMemblockFromFile(t.mbi, 11);
		CloseFile(11);
		tCount = 0;
		t.leavetime = MAXTimer();
		for (t.b = 0; t.b <= t.filesize - 5; t.b++)
		{
			//  JPG, PNG, DDS, BMP, TGA
			t.tokay = 0;
			if (ReadMemblockByte(t.mbi, t.b + 0) == Asc("."))
			{
				if (ReadMemblockByte(t.mbi, t.b + 1) == Asc("j"))
				{
					if (ReadMemblockByte(t.mbi, t.b + 2) == Asc("p"))
					{
						if (ReadMemblockByte(t.mbi, t.b + 3) == Asc("g"))
						{
							t.tokay = 1;
						}
					}
				}
				if (ReadMemblockByte(t.mbi, t.b + 1) == Asc("p"))
				{
					if (ReadMemblockByte(t.mbi, t.b + 2) == Asc("n"))
					{
						if (ReadMemblockByte(t.mbi, t.b + 3) == Asc("g"))
						{
							t.tokay = 1;
						}
					}
				}
				if (ReadMemblockByte(t.mbi, t.b + 1) == Asc("d"))
				{
					if (ReadMemblockByte(t.mbi, t.b + 2) == Asc("d"))
					{
						if (ReadMemblockByte(t.mbi, t.b + 3) == Asc("s"))
						{
							t.tokay = 1;
						}
					}
				}
				if (ReadMemblockByte(t.mbi, t.b + 1) == Asc("t"))
				{
					if (ReadMemblockByte(t.mbi, t.b + 2) == Asc("g"))
					{
						if (ReadMemblockByte(t.mbi, t.b + 3) == Asc("a"))
						{
							t.tokay = 1;
						}
					}
				}
				if (ReadMemblockByte(t.mbi, t.b + 1) == Asc("b"))
				{
					if (ReadMemblockByte(t.mbi, t.b + 2) == Asc("m"))
					{
						if (ReadMemblockByte(t.mbi, t.b + 3) == Asc("p"))
						{
							t.tokay = 1;
						}
					}
				}
			}
			if (t.tokay == 1)
			{
				// determine name of external texture file
				t.c = t.b;
				while (t.c > 0)
				{
					--t.c; if (t.c <= 0 || ReadMemblockByte(t.mbi, t.c) < 32 || ReadMemblockByte(t.mbi, t.c) > Asc("z"))  break;
				}
				++t.c;
				t.tthisfile_s = "";
				while (t.c <= t.b + 3)
				{
					t.tthisfile_s = t.tthisfile_s + Chr(ReadMemblockByte(t.mbi, t.c)); ++t.c;
				}
				t.b = t.b + 3;

				//  found texture specified in imported model
				t.tSourceName_s = t.tthisfile_s;

				//  check if texture file exists alonside model file
				char pFindFile[512];
				t.tFileName_s = t.importer.objectFileOriginalPath + t.tSourceName_s;
				strcpy(pFindFile, t.tFileName_s.Get());
				if (FileExist(pFindFile) == 0)
				{
					// if not, try just the filename itself (no path)
					int iFoundFilename = -1;
					for (int n = strlen(pFindFile) - 1; n > 0; n--)
					{
						if (pFindFile[n] == '\\' || pFindFile[n] == '/')
						{
							iFoundFilename = n;
							break;
						}
					}
					if (iFoundFilename != -1)
					{
						char pFilenameOnly[512];
						strcpy(pFilenameOnly, pFindFile + iFoundFilename + 1);
						strcpy(pFindFile, t.importer.objectFileOriginalPath.Get());
						strcat(pFindFile, pFilenameOnly);
					}
				}
				t.tFileName_s = pFindFile;

				//  ensure file is not a path or contains invalid characters
				if (strcmp(Right(t.tFileName_s.Get(), 1), "/") == 0 || strcmp(Right(t.tFileName_s.Get(), 1), "\\") == 0)  t.tFileName_s = "";
				for (t.fc = 1; t.fc <= Len(t.tFileName_s.Get()); t.fc++)
				{
					if (Asc(Mid(t.tFileName_s.Get(), t.fc)) < 32 || Asc(Mid(t.tFileName_s.Get(), t.fc)) > 126)
					{
						t.tFileName_s = "";
						break;
					}
				}

				// Add found texture to the importer texture list (if exists)
				if (FileExist(t.tFileName_s.Get()) == 1)
				{
					importer_addtexturefiletolist(t.tFileName_s, t.tSourceName_s, &tCount);
				}
			}
		}
		DeleteMemblock(t.mbi);
	}

	// If nothing in file byte scan, lets see if the FPE had a texture
	t.tFoundInFPE = 0;
	if ( tCount == 0 ) 
	{
		// get texture from FPE file
		if (  t.importer.objectFPE.textured  !=  "" ) 
		{
			t.tSourceName_s = t.importer.objectFPE.textured;
			if (  FileExist ( t.tSourceName_s.Get() ) == 1 )  
			{
				t.tFoundInFPE = 1;
				t.tFileName_s = t.tSourceName_s;
			}
			if (  t.tFoundInFPE  ==  0 ) 
			{
				t.tFileName_s = t.importer.objectFileOriginalPath + t.tSourceName_s;
				if (  FileExist ( t.tFileName_s.Get() ) == 1 )  
				{
					t.tFoundInFPE = 1;
				}
			}
		}

		// see if the FPE texture has a relative path
		if ( t.tFoundInFPE == 0 ) 
		{
			t.strwork = "" ; t.strwork = t.strwork + g.fpscrootdir_s + "\\Files\\"+t.importer.objectFPE.textured;
			if (  FileExist( t.strwork.Get() ) == 1 ) 
			{
				t.tSourceName_s=""; t.tSourceName_s = t.tSourceName_s+g.fpscrootdir_s + "\\Files\\"+t.importer.objectFPE.textured;
				t.tFoundInFPE = 1;
				t.tFileName_s = ""; t.tFileName_s = t.tFileName_s + g.fpscrootdir_s + "\\Files\\"+t.importer.objectFPE.textured;
			}
		}

		// if FPE did have valid texture, add to list
		if ( t.tFoundInFPE == 1 ) 
		{
			importer_addtexturefiletolist ( t.tFileName_s, t.tSourceName_s, &tCount );
		}
	}

	// if above scans/fpe found no texture file(s), go through limb texture names (in case we're dealing with binary X file)
	if ( tCount == 0 ) 
	{
		SetObjectEffect (  t.importer.objectnumber,0 );
		PerformCheckListForLimbs (  t.importer.objectnumber );
		for ( t.tCount9 = 1 ; t.tCount9 <= ChecklistQuantity()-1; t.tCount9++ )
		{
			t.tlimbname_s = "";
			if (LimbExist(t.importer.objectnumber, t.tCount9))
			{
				LPSTR sTmp = LimbTextureName(t.importer.objectnumber, t.tCount9);
				t.tlimbname_s = sTmp;
				if (sTmp) delete[] sTmp;
			}
			if (  t.tlimbname_s != "" ) 
			{
				t.tSourceName_s=t.tlimbname_s;
				t.tFileName_s=t.importer.objectFileOriginalPath+t.tSourceName_s;
				if (  FileExist(t.tFileName_s.Get()) == 0 ) 
				{
					t.tSourceName_s = ""; t.tSourceName_s=t.tSourceName_s+Left(t.tlimbname_s.Get(),Len(t.tlimbname_s.Get())-4)+".dds";
					t.tFileName_s=t.importer.objectFileOriginalPath+t.tSourceName_s;
				}
				if (  FileExist(t.tFileName_s.Get()) == 1 ) 
				{
					importer_addtexturefiletolist ( t.tFileName_s, t.tSourceName_s, &tCount );
				}
			}
		}
	}

	// if still no images, revert to default blank texture placeholder
	if ( tCount == 0 ) 
	{
		// if no actual texture file or FPE texture valid, use blank texture
		importer_addtexturefiletolist ( t.tSourceName_s, t.tSourceName_s, &tCount );
	}

	// final image load count, load textures and sort txture button sprite
	importer_load_textures_finish ( tCount, false );
}

void importer_load_fpe ( void )
{
	//  reset physics shape count
	t.importer.isProtected = 0;
	t.importer.collisionShapeCount = 0;

	//  Split the filename into tokens to grab the path, object name and create fpe name
	Dim (  t.tArray,300 );
	Dim (  t.tUnknown,300 );

	t.importer.unknownFPELineCount = 0;

	//  Reset FPE data to default before attempting to load
	t.importer.objectFPE.desc = Left(t.importer.objectFilenameFPE.Get(),Len(t.importer.objectFilenameFPE.Get())-4);

	//  ;visualinfo
	t.importer.objectFPE.textured = "";
	if (LimbExist(t.importer.objectnumber, 1) == 1)
	{
		LPSTR sTmp = LimbTextureName(t.importer.objectnumber, 1);
		t.importer.objectFPE.textured = sTmp; //PE: Never freed LimbTextureName(t.importer.objectnumber, 1);
		if (sTmp) delete[] sTmp;
	}
	t.importer.objectFPE.effect = "effectbank\\reloaded\\entity_basic.fx";
	t.importer.objectFPE.castshadow = "0";
	t.importer.objectFPE.transparency = "0";
	//  ;orientation
	t.importer.objectFPE.model = t.importer.objectFilename;
	t.importer.objectFPE.offx = "0";
	t.importer.objectFPE.offy = "0";
	t.importer.objectFPE.offz = "0";
	t.importer.objectFPE.rotx = "0";
	t.importer.objectFPE.roty = "0";
	t.importer.objectFPE.rotz = "0";
	t.importer.objectFPE.scale = "100";
	t.importer.objectFPE.collisionmode = "0";
	t.importer.objectFPE.defaultstatic = "1";
	t.importer.objectFPE.materialindex = "0";
	t.importer.objectFPE.matrixmode = "0";
	t.importer.objectFPE.cullmode = "0";
	//  ;identity details
	t.importer.objectFPE.ischaracter = "0";
	t.importer.objectFPE.hasweapon = "";
	t.importer.objectFPE.isobjective = "0";
	t.importer.objectFPE.cantakeweapon = "0";
	//  ;statistics
	t.importer.objectFPE.strength = "25";
	t.importer.objectFPE.explodable= "0";
	t.importer.objectFPE.debrisshape = "0";
	//  ;ai
	t.importer.objectFPE.aimain = "default.lua";
	//  ;spawn
	t.importer.objectFPE.spawnmax = "0";
	t.importer.objectFPE.spawndelay = "0";
	t.importer.objectFPE.spawnqty = "0";
	//  ;anim
	t.importer.objectFPE.animmax = "0";
	t.importer.objectFPE.animspeed = "100";
	t.tTotalFrames_s = "0";
	t.importer.objectFPE.anim0 = ""; t.importer.objectFPE.anim0 = t.importer.objectFPE.anim0+"0," + t.tTotalFrames_s;
	t.importer.objectFPE.playanimineditor = "0";
	t.importer.objectFPE.ignorecsirefs = "1";

	//  Check if an FPE exists, if so load it in
	if (  FileOpen(1) ) CloseFile (1);
	t.strwork = ""; t.strwork = t.strwork + t.importer.objectFileOriginalPath + t.importer.objectFilenameFPE;
	if (  FileExist ( t.strwork.Get() ) )  
	{
		t.strwork = ""; t.strwork = t.strwork +t.importer.objectFileOriginalPath + t.importer.objectFilenameFPE;
		OpenToRead (  1 , t.strwork.Get() );
		while (  FileEnd(1)  ==  0 ) 
		{
			t.tstring_s = ReadString (  1 );
			t.tempLine_s = t.tstring_s;

			t.tArrayMarker = 0;
			t.tToken_s=FirstToken(t.tstring_s.Get()," ");
			if (  t.tToken_s  !=  "" ) 
			{
				t.tArray[t.tArrayMarker] = t.tToken_s;
				++t.tArrayMarker;
			}
			do
			{
				t.tToken_s=NextToken(" ");
				if (  t.tToken_s  !=  "" ) 
				{
					t.tArray[t.tArrayMarker] = t.tToken_s;
					++t.tArrayMarker;
				}
			} while ( !(  t.tToken_s == "" ) );
			t.tStippedString_s = "";
			int tCount = 0;
			for ( tCount = 0 ; tCount<=  t.tArrayMarker-1; tCount++ )
			{
				if (  tCount < 3 ) 
				{
					t.tStippedString_s = t.tStippedString_s + t.tArray[tCount];
				}
				else
				{
					t.tStippedString_s = t.tStippedString_s + " " + t.tArray[tCount];
				}
			}
			if (  t.tStippedString_s  != "" && strcmp ( Left(t.tStippedString_s.Get(),1) , ";" ) != 0 ) 
			{
				t.tToken_s=FirstToken(t.tStippedString_s.Get(),"=");
				t.tToken2_s=NextToken("=");

				//  Get rid of any tabs that exist and replace with nothing (some files have tabs in sometimes)
				t.tstring_s = t.tToken_s ; t.tToken_s = "";
				for ( tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
				{
					if (  cstr(Mid(t.tstring_s.Get(),tCount))  !=  Chr(9)  )  t.tToken_s  =  t.tToken_s + Mid(t.tstring_s.Get(),tCount);
				}

				t.tstring_s = t.tToken2_s ; t.tToken2_s = "";
				for ( tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
				{
					if ( cstr( Mid(t.tstring_s.Get(),tCount))  !=  Chr(9)  )  t.tToken2_s  =  t.tToken2_s + Mid(t.tstring_s.Get(),tCount);
				}

				//  Header
				if ( t.tToken_s == "protected" ) { t.importer.isProtected  =  ValF(t.tToken2_s.Get()) ; }
				else if ( t.tToken_s == "desc" ) { t.importer.objectFPE.desc  =  t.tToken2_s ; }
				//  Visual Info
				else if ( t.tToken_s == "textured" ) { t.importer.objectFPE.textured  =  t.tToken2_s ; }
				else if ( t.tToken_s == "effect" ) { t.importer.objectFPE.effect  =  t.tToken2_s ; }
				else if ( t.tToken_s == "castshadow" ) { t.importer.objectFPE.castshadow  =  t.tToken2_s ; }
				else if ( t.tToken_s == "transparency" ) { t.importer.objectFPE.transparency  =  t.tToken2_s ; } 
				//  Orientation
				else if ( t.tToken_s == "model" ) { t.importer.objectFPE.model  =  t.tToken2_s ; }
				else if ( t.tToken_s == "offx" ) { t.importer.objectFPE.offx  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "offy" ) { t.importer.objectFPE.offy  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "offz" ) { t.importer.objectFPE.offz  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "rotx" ) { t.importer.objectFPE.rotx  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "roty" ) { t.importer.objectFPE.roty  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "rotz" ) { t.importer.objectFPE.rotz  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "scale" ) { t.importer.objectFPE.scale  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "collisionmode" ) { t.importer.objectFPE.collisionmode  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "defaultstatic" )
				{
					t.importer.objectFPE.defaultstatic  =  t.tToken2_s ;
				} 
				else if ( t.tToken_s == "materialindex" ) { t.importer.objectFPE.materialindex  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "matrixmode" ) { t.importer.objectFPE.matrixmode  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "cullmode" ) { t.importer.objectFPE.cullmode  =  t.tToken2_s ; } 
					
				//  Identity details
				else if ( t.tToken_s == "ischaracter" ) { t.importer.objectFPE.ischaracter  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "hasweapon" ) { t.importer.objectFPE.hasweapon  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "isobjective" ) { t.importer.objectFPE.isobjective  =  t.tToken2_s ; } 

				//  Statistics
				else if ( t.tToken_s == "strength" ) { t.importer.objectFPE.strength  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "explodable" ) { t.importer.objectFPE.explodable  =  t.tToken2_s ; } 
				else if ( t.tToken_s == "debrisshape" ) { t.importer.objectFPE.debrisshape  =  t.tToken2_s ; }
				//  AI
				else if ( t.tToken_s == "aimain" ) { importer_check_script_token_exists() ; t.importer.objectFPE.aimain  =  t.tToken2_s ; }
				//  Spawn
				else if ( t.tToken_s == "spawnmax" ) { t.importer.objectFPE.spawnmax  =  t.tToken2_s ; }
				else if ( t.tToken_s == "spawndelay" ) { t.importer.objectFPE.spawndelay  =  t.tToken2_s ; }
				else if ( t.tToken_s == "spawnqty" ) { t.importer.objectFPE.spawnqty  =  t.tToken2_s ; }
				//  Physics
				else if ( t.tToken_s == "physicscount" ) {
					t.tPhyscount = ValF(t.tToken2_s.Get());
					t.importer.collisionShapeCount = 0;
					while (  t.importer.collisionShapeCount < t.tPhyscount ) 
					{
						t.tstring_s = ReadString (  1 );

						t.tArrayMarker = 0;
						t.ttToken_s=FirstToken(t.tstring_s.Get()," ");
						if (  t.ttToken_s  !=  "" ) 
						{
							t.tArray[t.tArrayMarker] = t.ttToken_s;
							++t.tArrayMarker;
						}
						do
						{
							t.ttToken_s=NextToken(" ");
							if (  t.ttToken_s  !=  "" ) 
							{
								t.tArray[t.tArrayMarker] = t.ttToken_s;
								++t.tArrayMarker;
							}
						} while ( !(  t.ttToken_s == "" ) );
						t.tStippedString_s = "";
						for ( int tCount = 0 ; tCount<=  t.tArrayMarker-1; tCount++ )
						{
							if (  tCount < 3 ) 
							{
								t.tStippedString_s = t.tStippedString_s + t.tArray[tCount];
							}
							else
							{
								t.tStippedString_s = t.tStippedString_s + " " + t.tArray[tCount];
							}
						}
						if (  t.tStippedString_s  !=  "" && t.tStippedString_s.Get()[0]  !=  ';' ) 
						{
							t.ttToken_s=FirstToken(t.tStippedString_s.Get(),"=");
							t.ttToken2_s=NextToken("=");

							//  Get rid of any tabs that exist and replace with nothing (some files have tabs in sometimes)
							t.tstring_s = t.ttToken_s ; t.ttToken_s = "";
							for ( int tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
							{
								if (  cstr(Mid(t.tstring_s.Get(),tCount))  !=  Chr(9)  )  t.ttToken_s  =  t.ttToken_s + Mid(t.tstring_s.Get(),tCount);
							}

							t.tstring_s = t.ttToken2_s ; t.ttToken2_s = "";
							for ( int tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
							{
								if (  cstr(Mid(t.tstring_s.Get(),tCount))  !=  Chr(9)  )  t.ttToken2_s  =  t.ttToken2_s + Mid(t.tstring_s.Get(),tCount);
							}

							//  get rid of the quotation marks
							t.tStrip_s = t.ttToken2_s;
							t.tStrip_s = Left(t.tStrip_s.Get(), Len(t.tStrip_s.Get())-1);
							t.tStrip_s = Right(t.tStrip_s.Get(), Len(t.tStrip_s.Get())-1);

							t.tArrayMarker = 0;
							t.ttToken_s=FirstToken(t.tStrip_s.Get(),",");
							if (  t.ttToken_s  !=  "" ) 
							{
								t.tArray[t.tArrayMarker] = t.ttToken_s;
								++t.tArrayMarker;
							}
							do
							{
								t.ttToken_s=NextToken(",");
								if (  t.ttToken_s  !=  "" ) 
								{
									t.tArray[t.tArrayMarker] = t.ttToken_s;
									++t.tArrayMarker;
								}
							} while ( !(  t.ttToken_s == "" ) );

							//  Format; shapetype, sizex, sizey, sizez, offx, offy, offz, rotx, roty, rotz
							t.tPShapeType = ValF(t.tArray[0].Get());
							t.tPSizeX_f = ValF(t.tArray[1].Get());
							t.tPSizeY_f = ValF(t.tArray[2].Get());
							t.tPSizeZ_f = ValF(t.tArray[3].Get());
							t.tPOffX_f = ValF(t.tArray[4].Get());
							t.tPOffY_f = ValF(t.tArray[5].Get());
							t.tPOffZ_f = ValF(t.tArray[6].Get());
							t.tPRotX_f = ValF(t.tArray[7].Get());
							t.tPRotY_f = ValF(t.tArray[8].Get());
							t.tPRotZ_f = ValF(t.tArray[9].Get());
							importer_add_collision_box_loaded ( );
						}
					}
				}
				else
				{
					//  Store any unknown lines
					t.tUnknown[t.importer.unknownFPELineCount] = t.tempLine_s;
					++t.importer.unknownFPELineCount;
				}
			}
		}

	}

	CloseFile (  1 );

	//  Cleanup
	UnDim (  t.tArray );

	importer_apply_fpe ( );
}

