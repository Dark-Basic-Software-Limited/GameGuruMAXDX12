void Entity::UpdateMovement ( float fTimeDelta )
{
	// skip entirely if object no longer exists
	if ( CheckObjectExist ( dwObjectNumberRef ) == false )
		return;

	// cap time delta for any timeelapsed based calcs
	if ( fTimeDelta > 1.0f ) fTimeDelta = 1.0f;

	// manage avoid mode value
	int l_iAvoidMode = iEntityAvoidMode;
	if ( l_iAvoidMode < 0 ) l_iAvoidMode = iAvoidMode;

	// determine automated speed for full and crawl stance
	fCurrSpeed = fSpeed;
	if ( GetIsDucking ( ) ) fCurrSpeed = fSpeed / 2.0f;

	// if container does not exist, nowhere to move
	if ( !pWorld->ContainerExist ( pContainer ) ) pContainer = 0;
	if ( !pContainer || !pContainer->IsActive ( ) ) return;

	// entity-entity avoidance (modes 0,1,2,5)
	if ( (l_iAvoidMode < 3 || l_iAvoidMode == 5) )
	{
		// countdown any timers (each cycle)
		UpdateObstacleListCountDown ( fTimeDelta );

		// ensure expensive process done only occasionally
		if ( fExpensiveUpdateTimer <= 0.0f  )
		{
			// calculate obstacle list this cycle
			UpdateObstacleList ( fTimeDelta );

			// set random timer for avoidance mode
			fExpensiveUpdateTimer = ( 0.05f * rand() ) / RAND_MAX + 0.2f;
		}
	}

	// only when final destination changes do we re-assess final destination for point 'outside of obstacles'
	/* not using final dest
	float fDX = vecLastFinalDest.x - vecFinalDest.x;
	float fDZ = vecLastFinalDest.z - vecFinalDest.z;
	float fDD = sqrt(fabs(fDX*fDX)+abs(fDZ*fDZ));
	if ( fDD >= 0.0001f )
	{
		if ( pContainer->pPathFinder->InPolygons ( vecFinalDest.x, vecFinalDest.z ) > 0 )
		{
			pContainer->pPathFinder->FindClosestOutsidePoint ( &(vecFinalDest.x), &(vecFinalDest.z) );
		}
		vecLastFinalDest = vecFinalDest;
	}
	*/

	// deliberately commented out?
	/// AdjustDestination ( );

	// can show obstacle angles in debug (future feature?) - seems to stop AI bots
	//DebugHideObstacleAngles ( );
	//if ( bShowAvoidanceAngles )
	//{
	//	DebugDrawObstacleAngles ( );
	//}

	// work out usable per cycle speed based on time elapsed delta
	float fTimedSpeed = fCurrSpeed * fTimeDelta;

	// feedback position tells us whether we are walking on the spot - not using for now
	///if ( !bMoving ) ResetFeedbackPos ( );	
	///if ( fMoveFeedbackTimer <= 0.0f ) UpdateFeedbackPos ( );

	// calculate path as often as fPathTimer allows (intensive calc)
	bool bUpdateMovement = false;
	if ( g_LeeThread.GetWorkInProgress() == -1 )
	{
		if ( ListOfEntitiesToUpdateMovement.size() > 0 )
		{
			// go through all entities and work out who gets to update their path this cycle (only one per cycle in turn)
			//if ( ListOfEntitiesToUpdateMovementIndex > (int)ListOfEntitiesToUpdateMovement.size()-1 ) ListOfEntitiesToUpdateMovementIndex = 0;
			//if ( ListOfEntitiesToUpdateMovementIndex <= (int)ListOfEntitiesToUpdateMovement.size()-1 )
			int ListOfEntitiesToUpdateMovementIndex = 0; // always take first (oldest) one first (erase when done with it)
			{
				if ( ListOfEntitiesToUpdateMovement[ListOfEntitiesToUpdateMovementIndex] == this->iID )
				{
					if (((bRedoPath ) && !bChangingContainers && iAggressiveness != 2 && !bIsLeaping && !bIsDiving) ) 
					{
						// only allow if spacing time has elapsed
						CurrentAIWorkedPathTimer -= fTimeDelta;
						if ( CurrentAIWorkedPathTimer <= 0.0f )
						{
							CurrentAIWorkedPathTimer = 0.1f; // X second until next inc.
							bUpdateMovement = true;
						}
					}
				}
			}
		}
	}

	// when trigger path to be updated, start a thread task
	if ( bUpdateMovement == true )
	{
		// new path must start from here, and is also final pos if no path routed
		vecFinalDest.x = GetX( );
		vecFinalDest.y = GetY( );
		vecFinalDest.z = GetZ( );

		// if stored final dest in "pre_", restore it
		if ( pre_vecFinalDest.x != -999 )
		{
			vecFinalDest.x = pre_vecFinalDest.x;
			vecFinalDest.y = pre_vecFinalDest.y;
			vecFinalDest.z = pre_vecFinalDest.z;
			vecLastDest.x = pre_vecLastDest.x;
			vecLastDest.y = pre_vecLastDest.y;
			vecLastDest.z = pre_vecLastDest.z;
		}
		pre_vecFinalDest.x = -999;
		pre_vecLastDest.x = -999;

		// remove any old paths (about to create a new one)
		cMovePath.DebugHide ( );
		DebugHideDestination ( );
		ListOfEntitiesToUpdateMovement.erase (ListOfEntitiesToUpdateMovement.begin()+0);
		bRedoPath = false;

		// thread task issue
		g_LeeThread.BeginWork( this->iID, GetX(), GetZ(), vecFinalDest.x, vecFinalDest.y, vecFinalDest.z, pContainer, iDestContainer );
	}
	if ( g_LeeThread.GetWorkInProgress() == this->iID )
	{
		if ( g_LeeThread.GetWorkComplete() == 1 )
		{
			// ready for new path and set timers for next think-time
			fForceUpdatePathTimer = 0.0f;
			fPathTimer = 0.75f;

			// get new path from path calculation done in thread
			Path cNewPath = g_LeeThread.GetNewPath();
			g_LeeThread.GetNewFinalDest ( &vecFinalDest.x, &vecFinalDest.y, &vecFinalDest.z );
			g_LeeThread.EndWork();

			/*
			// prepare level obstacles for new path calculation
			Path cNewPath;
			pContainer->pPathFinder->ActivateAllWaypoints ( );
			pContainer->pPathFinder->CalculatePath ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z, &cNewPath, fPathStartCostLimit, iDestContainer );

			// if no path to finaldest, work from finaldest back to known reachable point
			if ( cNewPath.CountPoints() == 0 )
			{
				// okay, so final dest not pathable, and no direct line to barrier
				// so find closest node from container, and set that as finaldest
				int iThisContainer = GetContainer()->GetID(); 
				iDestContainer = iThisContainer; // container ID can be changed below
				float fBestDistance = 999999.0f;
				float fBestX = 0.0f;
				float fBestZ = 0.0f;
				float fDX = GetX( ) - vecFinalDest.x;
				float fDZ = GetZ( ) - vecFinalDest.z;
				float fDD = sqrt ( fabs(fDX*fDX) + fabs(fDZ*fDZ) );
				float fDDInc = fDD / 30.0f;
				int iDistCount = (int)fDDInc;
				for ( int iDist = 1; iDist <= 30; iDist++ )
				{
					for ( int iAng = 0; iAng < 360; iAng+=45 )
					{
						// this is where I want the AI to try to get to
						float fTryX = vecFinalDest.x + (sin(iAng*DEGTORAD)*(iDist*fDDInc));
						float fTryZ = vecFinalDest.z + (cos(iAng*DEGTORAD)*(iDist*fDDInc));
						int waypointmax = waypoint_getmax();
						for ( int waypointindex = 1; waypointindex <= waypointmax; waypointindex++ )
						{
							int tokay = waypoint_ispointinzoneex ( waypointindex, fTryX, vecFinalDest.y, fTryZ, 1 );
							if ( tokay == 1 ) 
							{
								iDestContainer = waypointindex;
								fBestDistance = 0;
								fBestX = fTryX;
								fBestZ = fTryZ;
								iDist = 31;
								iAng = 361;
								break;
							}
						}
					}
				}
				if ( fBestDistance != 999999.0f )
				{
					// best is within waypoint zone, but outside AI obstacle zone (margin added to AI waypoint system)
					// so project away from player to ensure we get inside
					float fDX = fBestX - vecFinalDest.x;
					float fDZ = fBestZ - vecFinalDest.z;
					float fDD = sqrt ( fabs(fDX*fDX)+fabs(fDZ*fDZ) );
					fDX = (fDX/fDD)*(fDD+30.0f);
					fDZ = (fDZ/fDD)*(fDD+30.0f);
					fBestX = vecFinalDest.x + fDX;
					fBestZ = vecFinalDest.z + fDZ;

					// now assign final pos
					if ( iDestContainer != iThisContainer )
					{
						// found closer position in another container, so just go there
						vecFinalDest.x = fBestX;
						vecFinalDest.z = fBestZ;
					}
					else
					{
						// same container, so dont get too close to edge when move nearer target
						float fPushX = fBestX - vecFinalDest.x;
						float fPushZ = fBestZ - vecFinalDest.z;
						float fPushDD = sqrt ( fabs(fPushX*fPushX) + fabs(fPushZ*fPushZ) );
						fPushX /= fPushDD;
						fPushZ /= fPushDD;
						vecFinalDest.x = fBestX + (fPushX*2.0f);
						vecFinalDest.z = fBestZ + (fPushZ*2.0f);
					}
				}
				else
				{
					if ( GetContainer()->GetID() == 0 )
					{
						float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z );
						if ( fResult >= 0.1f ) 
						{
							iDestContainer = GetContainer()->GetID(); 
							vecFinalDest.x = GetX( ) + ( vecFinalDest.x - GetX( ) )*fResult;
							vecFinalDest.z = GetZ( ) + ( vecFinalDest.z - GetZ( ) )*fResult;
						}
					}
				}

				// chart new path to this permimeter position
				cNewPath.Clear ( );
				pContainer->pPathFinder->ActivateAllWaypoints ( );
				pContainer->pPathFinder->CalculatePath ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z, &cNewPath, fPathStartCostLimit, iDestContainer );
				if ( cNewPath.CountPoints( ) == 2 )
				{
					// if path has zero length, reduce to a single node
					if ( fabs ( cNewPath.GetPoint( 0 ).x - cNewPath.GetPoint( 1 ).x ) < 5.0f && fabs ( cNewPath.GetPoint( 0 ).y - cNewPath.GetPoint( 1 ).y ) < 5.0f )
						cNewPath.RemoveLast();
				}
			}
			*/

			// create move path from above result
			cMovePath.Clear ( );
			if ( cNewPath.CountPoints( ) > 0 )
			{
				// make new path the move path
				cMovePath = cNewPath;
				iCurrentMovePoint = 1;
				bChangingContainers = false;
				if ( cNewPath.CountPoints( ) > 1 )
				{
					if ( cMovePath.GetPoint( 0 ).container != cMovePath.GetPoint( 1 ).container ) 
						bChangingContainers = true;
				}
			}
	
			// ensure new move path is from the new fresh one (and show if in debug)
			cMovePath.DebugHide ( );
			DebugHideDestination ( );
			if ( bShowPaths ) 
			{
				cMovePath.DebugDraw ( fRadius );
				DebugDrawDestination ( );
			}
		}
	}

	// and ensure this expensive task is only done once per fPathTimer
	/* new thread code
	if ( bUpdateMovement && fPathTimer <= 0.0f )
	{
		// new path must start from here, and is also final pos if no path routed
		vecFinalDest.x = GetX( );
		vecFinalDest.y = GetY( );
		vecFinalDest.z = GetZ( );

		// if stored final dest in "pre_", restore it
		if ( pre_vecFinalDest.x != -999 )
		{
			vecFinalDest.x = pre_vecFinalDest.x;
			vecFinalDest.y = pre_vecFinalDest.y;
			vecFinalDest.z = pre_vecFinalDest.z;
			vecLastDest.x = pre_vecLastDest.x;
			vecLastDest.y = pre_vecLastDest.y;
			vecLastDest.z = pre_vecLastDest.z;
		}
		pre_vecFinalDest.x = -999;
		pre_vecLastDest.x = -999;

		// remove any old paths (about to create a new one)
		cMovePath.DebugHide ( );
		DebugHideDestination ( );
		ListOfEntitiesToUpdateMovement.erase (ListOfEntitiesToUpdateMovement.begin()+0);

		// prepare level obstacles for new path calculation
		Path cNewPath;
		pContainer->pPathFinder->ActivateAllWaypoints ( );
		pContainer->pPathFinder->CalculatePath ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z, &cNewPath, fPathStartCostLimit, iDestContainer );
		bRedoPath = false;
		fForceUpdatePathTimer = 0.0f;
		fPathTimer = 0.75f; // 070317 - faster zombies require higher fidelity to prevent stop start

		// if no path to finaldest, work from finaldest back to known reachable point
		if ( cNewPath.CountPoints() == 0 )
		{
			// okay, so final dest not pathable, and no direct line to barrier
			// so find closest node from container, and set that as finaldest
			int iThisContainer = GetContainer()->GetID(); 
			iDestContainer = iThisContainer; // container ID can be changed below
			float fBestDistance = 999999.0f;
			float fBestX = 0.0f;
			float fBestZ = 0.0f;
			float fDX = GetX( ) - vecFinalDest.x;
			float fDZ = GetZ( ) - vecFinalDest.z;
			float fDD = sqrt ( fabs(fDX*fDX) + fabs(fDZ*fDZ) );
			float fDDInc = fDD / 30.0f;
			int iDistCount = (int)fDDInc;
			for ( int iDist = 1; iDist <= 30; iDist++ )
			{
				for ( int iAng = 0; iAng < 360; iAng+=45 )
				{
					// this is where I want the AI to try to get to
					float fTryX = vecFinalDest.x + (sin(iAng*DEGTORAD)*(iDist*fDDInc));
					float fTryZ = vecFinalDest.z + (cos(iAng*DEGTORAD)*(iDist*fDDInc));
					int waypointmax = waypoint_getmax();
					for ( int waypointindex = 1; waypointindex <= waypointmax; waypointindex++ )
					{
						int tokay = waypoint_ispointinzoneex ( waypointindex, fTryX, vecFinalDest.y, fTryZ, 1 );
						if ( tokay == 1 ) 
						{
							iDestContainer = waypointindex;
							fBestDistance = 0;
							fBestX = fTryX;
							fBestZ = fTryZ;
							iDist = 31;
							iAng = 361;
							break;
						}
					}
				}
			}
			if ( fBestDistance != 999999.0f )
			{
				// best is within waypoint zone, but outside AI obstacle zone (margin added to AI waypoint system)
				// so project away from player to ensure we get inside
				float fDX = fBestX - vecFinalDest.x;
				float fDZ = fBestZ - vecFinalDest.z;
				float fDD = sqrt ( fabs(fDX*fDX)+fabs(fDZ*fDZ) );
				fDX = (fDX/fDD)*(fDD+30.0f);
				fDZ = (fDZ/fDD)*(fDD+30.0f);
				fBestX = vecFinalDest.x + fDX;
				fBestZ = vecFinalDest.z + fDZ;

				// now assign final pos
				if ( iDestContainer != iThisContainer )
				{
					// found closer position in another container, so just go there
					vecFinalDest.x = fBestX;
					vecFinalDest.z = fBestZ;
				}
				else
				{
					// same container, so dont get too close to edge when move nearer target
					float fPushX = fBestX - vecFinalDest.x;
					float fPushZ = fBestZ - vecFinalDest.z;
					float fPushDD = sqrt ( fabs(fPushX*fPushX) + fabs(fPushZ*fPushZ) );
					fPushX /= fPushDD;
					fPushZ /= fPushDD;
					vecFinalDest.x = fBestX + (fPushX*2.0f);
					vecFinalDest.z = fBestZ + (fPushZ*2.0f);
				}
			}
			else
			{
				if ( GetContainer()->GetID() == 0 )
				{
					float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z );
					if ( fResult >= 0.1f ) 
					{
						iDestContainer = GetContainer()->GetID(); 
						vecFinalDest.x = GetX( ) + ( vecFinalDest.x - GetX( ) )*fResult;
						vecFinalDest.z = GetZ( ) + ( vecFinalDest.z - GetZ( ) )*fResult;
					}
				}
			}

			// chart new path to this permimeter position
			cNewPath.Clear ( );
			pContainer->pPathFinder->ActivateAllWaypoints ( );
			pContainer->pPathFinder->CalculatePath ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z, &cNewPath, fPathStartCostLimit, iDestContainer );
			if ( cNewPath.CountPoints( ) == 2 )
			{
				// if path has zero length, reduce to a single node
				if ( fabs ( cNewPath.GetPoint( 0 ).x - cNewPath.GetPoint( 1 ).x ) < 5.0f && fabs ( cNewPath.GetPoint( 0 ).y - cNewPath.GetPoint( 1 ).y ) < 5.0f )
					cNewPath.RemoveLast();
			}
		}

		// create move path from above result
		cMovePath.Clear ( );
		if ( cNewPath.CountPoints( ) > 0 )
		{
			// make new path the move path
			cMovePath = cNewPath;
			iCurrentMovePoint = 1;
			bChangingContainers = false;
			if ( cNewPath.CountPoints( ) > 1 )
			{
				if ( cMovePath.GetPoint( 0 ).container != cMovePath.GetPoint( 1 ).container ) 
					bChangingContainers = true;
			}
		}
	}
	IN A THREAD END*/
	
	// set intermediate destination based on path and avoidance data
	if ( bAvoiding && !bChangingContainers && !bIsLeaping && !bIsDiving )
	{
		// above code can create an avoidance position to get around something when stuck
		vecCurrDest.x = vecAvoidPos.x;
		vecCurrDest.y = vecAvoidPos.y;
		vecCurrDest.z = vecAvoidPos.z;
	}
	else
	{
		if ( cMovePath.CountPoints( ) > 2 ) 
		{
			// No longer using 'unit perfect' movement to get to node, we depend on LUA/Anim movement steps
			// so work on distance from start of path to end of path, and work the 'complete path' based on that
			bool bFurtherThanDistanceOfPath = false;
			float lastpointx = cMovePath.GetPoint( iCurrentMovePoint-1 ).x;
			float lastpointy = cMovePath.GetPoint( iCurrentMovePoint-1 ).y;
			float thispointx = cMovePath.GetPoint( iCurrentMovePoint ).x;
			float thispointy = cMovePath.GetPoint( iCurrentMovePoint ).y;
			float pathdistx = fabs(thispointx - lastpointx);
			float pathdisty = fabs(thispointy - lastpointy);
			if ( pathdistx > 5.0f ) pathdistx -= 5.0f;
			if ( pathdisty > 5.0f ) pathdisty -= 5.0f;
			float entitydistx = fabs(GetX() - lastpointx);
			float entitydisty = fabs(GetZ() - lastpointy);
			if ( entitydistx*entitydistx + entitydisty*entitydisty >= pathdistx*pathdistx + pathdisty*pathdisty )
			{
				// we have CROSSED the node and can move onto the next one, 
				// so no more searching for the node based on position!
				bFurtherThanDistanceOfPath = true;
			}

			// when get to end of path, stay in this container or consider moving to another one
			if ( bFurtherThanDistanceOfPath == true && iCurrentMovePoint == cMovePath.CountPoints( ) - 1 && bChangingContainers )
			{
				// stay in same container
				bChangingContainers = false;
			}
			else
			{
				// check if different container ahead
				bool bDiffContainers = (iCurrentMovePoint < cMovePath.CountPoints( ) - 1) && (cMovePath.GetPoint( iCurrentMovePoint ).container != cMovePath.GetPoint( iCurrentMovePoint + 1 ).container);
				if ( (bFurtherThanDistanceOfPath == true && iCurrentMovePoint < cMovePath.CountPoints( ) - 1) || (bDiffContainers && bFurtherThanDistanceOfPath == true) )
				{
					// work out new container to resume path
					Container *pNewCont = pWorld->GetContainer( cMovePath.GetPoint( iCurrentMovePoint+1 ).container );
					if ( !bDiffContainers || pNewCont->pPathFinder->GridGetPosition( cMovePath.GetPoint( iCurrentMovePoint+1 ).x, cMovePath.GetPoint( iCurrentMovePoint+1 ).y ) == 0 )
					{
						SetContainer( pNewCont );
						iCurrentMovePoint++;
						if ( cMovePath.GetPoint( iCurrentMovePoint - 1 ).container != cMovePath.GetPoint( iCurrentMovePoint ).container ) 
							bChangingContainers = true;
						else 
							bChangingContainers = false;
					}
				}
			}

			// always set the current destination based on end of path segment we are on
			vecCurrDest.x = cMovePath.GetPoint( iCurrentMovePoint ).x;
			vecCurrDest.z = cMovePath.GetPoint( iCurrentMovePoint ).y;
			vecCurrDest.y = GetY();
		}
		else
		{
			// straight path to end node
			if ( cMovePath.CountPoints( ) == 2 )
			{
				// two point path, straight to destination
				vecCurrDest.x = cMovePath.GetPoint( 1 ).x;
				vecCurrDest.z = cMovePath.GetPoint( 1 ).y;
				vecCurrDest.y = GetY();
				vecFinalDest = vecCurrDest;
			}
			else
			{
				// no possible path if only have one node
				if ( cMovePath.CountPoints( ) == 1 )
				{
					vecCurrDest.x = cMovePath.GetPoint( 0 ).x;
					vecCurrDest.z = cMovePath.GetPoint( 0 ).y;
					vecCurrDest.y = GetY();
				}
				else
				{
					// no nodes, stood still, so just look at final dest (goto pos)
					vecCurrDest = vecFinalDest;
				}
			}
		}
	}

	// determine if reached final destination
	float fDiffX = vecFinalDest.x - GetX( );
	float fDiffZ = vecFinalDest.z - GetZ( );
	float fDist = sqrt(fabs(fDiffX*fDiffX) + fabs(fDiffZ*fDiffZ));
	fDiffX = vecCurrDest.x - GetX( );
	fDiffZ = vecCurrDest.z - GetZ( );
	float fDist2 = sqrt(fabs(fDiffX*fDiffX) + fabs(fDiffZ*fDiffZ));
	if ( fDist2 > fDist ) fDist = fDist2;
	if ( cMovePath.CountPoints() > 1 && fDist > 4.0f )
	{
		if ( cMovePath.CountPoints() == 2 )
		{
			// last one, so if short and FAST, stop to avoid running in circles to find perfect landing spot
			if ( fDist < fSpeed/10.0f )
				bMoving = false;
			else		
				bMoving = true;
		}
		else
		{
			// longer way to go
			bMoving = true;
		}
	}
	else
	{
		bMoving = false;
	}

	// if avoid mode 4, and timer allows an event here
	if ( l_iAvoidMode == 4 && !bChangingContainers && !bIsLeaping && !bIsDiving && iAggressiveness != 2 )
	{
		if ( fExpensiveUpdateTimer <= 0.0f )
		{
			// clear so can create a new avoid path
			cAvoidPath.Clear( );
			RebuildDynamicPathFinder( );
			
			// dynamically caluclate a dynamic 'on the fly' avoidance path
			AvoidanceObject *pObject;
			if ( pDynamicPathFinder->InObject( GetX( ), GetZ( ), &pObject ) )
			{
				float x;
				float z;
				if ( pDynamicPathFinder->GetClosestPoint( pObject, GetX( ), GetZ( ), x, z ) )
				{
					cAvoidPath.AddPoint( GetX(), 0, GetZ() );
					cAvoidPath.AddPoint( x, 0, z );
					iCurrentAvoidPoint = 1;
				}
			}
			else
			{
				pDynamicPathFinder->CalculatePath( GetX( ), GetZ( ), vecCurrDest.x, vecCurrDest.z, &cAvoidPath, -1 );
				iCurrentAvoidPoint = 1;
			}

			// refresh timer to ensure better performance
			fExpensiveUpdateTimer = ( 0.05f * rand() ) / RAND_MAX + 0.2f;
		}
		
		// if avoid path was created
		if ( cAvoidPath.CountPoints( ) > 1 )
		{
			// if already near or on avoid path node, move to next one right away
			float diffx = cAvoidPath.GetPoint( iCurrentAvoidPoint ).x - GetX();
			float diffz = cAvoidPath.GetPoint( iCurrentAvoidPoint ).y - GetZ();
			if ( diffx*diffx + diffz*diffz < 0.01f && iCurrentAvoidPoint < cAvoidPath.CountPoints( ) - 1 ) 
			{
				iCurrentAvoidPoint++;
			}

			// set the current destination to this avoid path node instead (overrides one set above)
			vecCurrDest.x = cAvoidPath.GetPoint( iCurrentAvoidPoint ).x;
			vecCurrDest.z = cAvoidPath.GetPoint( iCurrentAvoidPoint ).y;
		}
	}

	// if avoidance mode (0,1,2,5) and not specific iAggressiveness mode (automated state)
	if ( (l_iAvoidMode < 3 || l_iAvoidMode == 5) && !bChangingContainers && !bIsLeaping && !bIsDiving && iAggressiveness != 2 )
	{
		// adjust intermediate destination based on nearby entities
		if ( bAvoiding == false )
		{
			float fDiffX = vecCurrDest.x - GetX( );
			float fDiffZ = vecCurrDest.z - GetZ( );
			float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;
			if ( fDist > 0.00001f )
			{
				// work out new current destination position to place in circle around current AI position
				float fDirAngle = acos ( fDiffZ / sqrt(fDist) ) * RADTODEG;
				if ( fDiffX < 0 ) fDirAngle = 360.0f - fDirAngle;
				bool bValid = AdjustDirection ( &fDirAngle );
				if ( bValid == true )
				{
					// set this AI bot to do an avoid phase
					float fAvoidByDistance = 100.0f;
					vecCurrDest.x = GetX( ) + (sin ( fDirAngle * DEGTORAD ) * fRadius * fAvoidByDistance);
					vecCurrDest.z = GetZ( ) + (cos ( fDirAngle * DEGTORAD ) * fRadius * fAvoidByDistance);
					vecAvoidPos.x = vecCurrDest.x;
					vecAvoidPos.y = vecCurrDest.y;
					vecAvoidPos.z = vecCurrDest.z;
					fAvoidTimer = 0.2f;
					bAvoiding = true;
				}
			}
		}
	}

	// handle avoid sequence (only uses vecAvoidPos above temporarily)
	if ( bAvoiding && fAvoidTimer <= 0 ) 
	{
		// ensure avoidance is switched off when time is up
		bAvoiding = false;
	}

	// double checks that a current destination was not embedded inside an obstacle
	/* seems only need this if bot start getting stuck INSIDE obstacles - from avoidance!
	if ( fObstacleTimer <= 0.0f && iAggressiveness != 2 )
	{
		if ( !bChangingContainers && !bIsLeaping && !bIsDiving )
		{
			// adjust intermediate destination based on obstacle data
			float fDX = vecLastCurrDest.x - vecCurrDest.x;
			float fDZ = vecLastCurrDest.z - vecCurrDest.z;
			float fDD = sqrt(fabs(fDX*fDX)+abs(fDZ*fDZ));
			if ( fDD >= 0.0001f )
			{
				if ( pContainer->pPathFinder->InPolygons ( vecCurrDest.x, vecCurrDest.z ) > 0 )
				{
					pContainer->pPathFinder->FindClosestOutsidePoint ( &(vecCurrDest.x), &(vecCurrDest.z) );
				}
				vecLastCurrDest = vecCurrDest;
			}

			// find closest polygon
			float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX( ), GetZ( ), vecCurrDest.x, vecCurrDest.z );
			if ( fResult >= 0.0f ) 
			{
				vecCurrDest.x = GetX( ) + ( vecCurrDest.x - GetX( ) )*fResult;
				vecCurrDest.z = GetZ( ) + ( vecCurrDest.z - GetZ( ) )*fResult;
			}
		}

		// intentionally commented out, diabling this?
		// fObstacleTimer = 0.2f + (rand()*0.05f / RAND_MAX);
	}
	*/
	
	/* 140217 - vecCurrLookAt (for movement) needed to be set when bot reached end of last path
	SUGGEST DELETING THIS - NOW LOOKAT IS ALSO MOVETO VECTOR!! (leave LOOKing to LUA script now)
	if ( stagger == 7 )
	{
		//handle entity look direction
		int iHeight = 1;
		if ( bIsDucking ) iHeight = 2;

		if ( !bLookAtPointSet )
		{		
			//if ( !pContainer->pPathFinder->QuickPolygonsCheckVisible ( GetX( ), GetZ( ), vecFinalDest.x, vecFinalDest.z, iHeight ) ) 
			if ( CouldSee( vecFinalDest.x, vecFinalDest.y, vecFinalDest.z, false ) && bCanStrafe )
			{
				vecCurrLookAt = vecFinalDest;
			}
			else 
			{
				if ( cMovePath.CountPoints ( ) > 2 )
				{
					vecCurrLookAt.x = cMovePath.GetPoint( iCurrentMovePoint ).x;				//set the look at to the current destination
					vecCurrLookAt.z = cMovePath.GetPoint( iCurrentMovePoint ).y;
					vecCurrLookAt.y = GetY( );
				}
				else
				{
					vecCurrLookAt = vecFinalDest;
				}
			}
		}
		else
		{
			//if ( !pContainer->pPathFinder->QuickPolygonsCheckVisible ( GetX( ), GetZ( ), vecLookAt.x, vecLookAt.z, iHeight ) )
			if ( CouldSee( vecLookAt.x, vecLookAt.y, vecLookAt.z, false ) && bCanStrafe )
			{
				vecCurrLookAt = vecLookAt;				//set the look at to the one defined by the state
			}
			else
			{
				if ( !bMoving )
				{
					vecCurrLookAt = vecLookAt;
				}
				else
				{
					if ( cMovePath.CountPoints ( ) > 2 )
					{
						vecCurrLookAt.x = cMovePath.GetPoint( iCurrentMovePoint ).x;				//set the look at to the current destination
						vecCurrLookAt.z = cMovePath.GetPoint( iCurrentMovePoint ).y;
						vecCurrLookAt.y = GetY( );
					}
					else
					{
						vecCurrLookAt = vecFinalDest;
					}
				}
			}		
		}
	}
	*/

	// 140217 - LOOK VECTOR (which controls entity angle) needs to be the destination 
	// for perfect turning while path finding at various speeds
	vecCurrLookAt = vecCurrDest;

	// show viewing arcs if debug mode active
	if ( bShowViewArcs ) DebugDrawViewArcs ( );
	else DebugHideViewArcs ( );

	// set the direction of movement 
	GGVECTOR3 vecDir;
	vecDir.x = vecCurrDest.x - GetX ( );
	vecDir.z = vecCurrDest.z - GetZ ( );
	vecDir.y = 0;

	// update last position of AI bot
	float fLength = vecDir.x*vecDir.x + vecDir.z*vecDir.z;
	vecLastPos.x = GetX( );
	vecLastPos.y = GetY( );
	vecLastPos.z = GetZ( );

	// not sure what this is?
	pvecDir[0] = pvecDir[0]*0.75f + vecDir*0.25f;

	// if no avoidance mode, affect the direction vector?
	if ( l_iAvoidMode == 0 )
	{
		GGVECTOR3 vecAvgDir( pvecDir[0] );
		float fDiffX = abs( vecAvgDir.x - vecDir.x );
		float fDiffZ = abs( vecAvgDir.z - vecDir.z );
		if ( fDiffX < 0.1f && fDiffZ < 0.1f ) vecDir = vecAvgDir;
		else { vecDir.x = 0; vecDir.y = 0; vecDir.z = 0; }
	}

	// 280217 - flag to restore legacy behaviour of AI moving itself based on AI speed (from below)
	if ( bLegacyForceMove == true )
	{
		// older GoTo call did not use containers
		if ( fLength > 0.01f && fLength > fTimedSpeed*fTimedSpeed )
		{	
			// adjust direction with AI bot speed
			vecDir = ( ( fTimedSpeed * vecDir ) / sqrt( fLength ) );
			// adjust direction vector for diving and leaping
			if ( bIsDiving ) vecDir *= fDiveMultiplier;
			else if ( bIsLeaping ) vecDir *= fLeapMultiplier;
			// flags to determine movement style
			if ( pObject ) pObject->position.vecPosition += vecDir;
			else
			{
				fPosX += vecDir.x;
				fPosY += vecDir.y;
				fPosZ += vecDir.z;
			}
			SetPosition ( fPosX, fPosY, fPosZ );
			bMoving = true;
		}
	}
	
	// if distance between current position and destination, use intermediate destination to move entity (automatic state)
	/* DIRECTLY MOVES the position of the AI bot - remove this as want LUA script control here
	if ( fLength > 0.01f && fLength > fTimedSpeed*fTimedSpeed )
	{	
		// adjust direction with AI bot speed
		vecDir = ( ( fTimedSpeed * vecDir ) / sqrt( fLength ) );

		// adjjust direction vector for diving and leaping
		if ( bIsDiving ) vecDir *= fDiveMultiplier;
		else if ( bIsLeaping ) vecDir *= fLeapMultiplier;

		// flags to determine movement style
		int iCanMove = 1;
		int iReserved = 0;

		// for avoidance mode 5, can reserve spaces
		if ( l_iAvoidMode == 5 )
		{
			int ignore = bMeleeMode ? GetTargetID(1) : 0;
			iCanMove = pContainer->pPathFinder->GridCheckDirection( GetID( ), GetX( ), GetZ( ), vecDir.x, vecDir.z, vecFinalDest.x, vecFinalDest.z, ignore, iReserved );
			if ( iCanMove == 2 )
			{
				pContainer->pPathFinder->GridClearEntityPosition( GetX(), GetZ() );
			}
			if ( iReserved == 1 ) bReservedSpace = true;
			if ( iReserved == 2 ) bReservedSpace = false;
		}

		if ( iCanMove && iAggressiveness != 2 )
		{
			if ( pObject ) pObject->position.vecPosition += vecDir;
			else
			{
				fPosX += vecDir.x;
				fPosY += vecDir.y;
				fPosZ += vecDir.z;
			}
		}

		//if ( !bMoving )	ResetFeedbackPos ( );
		//bMoving = true;
	}
	else
	{
		int iCanMove = 1;
		int iReserved = 0;
		if ( l_iAvoidMode == 5 )
		{
			int ignore = bMeleeMode ? GetTargetID(1) : 0;
			iCanMove = pContainer->pPathFinder->GridCheckDirection( GetID( ), GetX( ), GetZ( ), vecCurrDest.x - GetX(), vecCurrDest.z - GetZ(), vecFinalDest.x, vecFinalDest.z, ignore, iReserved );
			if ( iCanMove == 2 )
			{
				pContainer->pPathFinder->GridClearEntityPosition( GetX(), GetZ() );
			}
			if ( iReserved == 1 ) bReservedSpace = true;
			if ( iReserved == 2 ) bReservedSpace = false;
		}

		if ( iCanMove && iAggressiveness != 2 )
		{
			if ( pObject ) 
			{
				pObject->position.vecPosition.x = vecCurrDest.x;
				pObject->position.vecPosition.z = vecCurrDest.z;
			}
			else
			{
				fPosX = vecCurrDest.x;
				fPosZ = vecCurrDest.z;
			}
		}

		//bMoving = false;
		bAvoiding = false;
		if ( GetSqrDistToDest() > 0.0001f ) 
		{
			//Dave
			if ( bRedoPath == false )
			{
				ListOfEntitiesToUpdateMovement.push_back(this->iID);
				bRedoPath = true;
				//fPathTimer -= 0.35f;
			}
			//fPathTimer = 0;
		}
	}
	*/

	// if avoidance mode 5, set AI bot id to grid placement
	if ( l_iAvoidMode == 5 ) pContainer->pPathFinder->GridSetEntityPosition( GetX(), GetZ(), GetID( ) );

	// turn the entity to its look vector (now same as move position)
	TurnToAngle ( fTimeDelta, bMoving );

	// reset user defined avoidance data
	bHitSomething = false;
	bHitBySomething = false;
	pHitEntity = 0;
	pHitByEntity = 0;
}

void Entity::RemoveFromThread ( )
{
	// when destroy entity, ensure leave any thread work
	if ( g_LeeThread.GetWorkInProgress() == this->iID )
	{
		g_LeeThread.EndWork();
	}
}

void Entity::UpdateState ( )
{
	vector <sHitPoint>::iterator hIter = sHitFrom_list.begin ( );

	while ( hIter < sHitFrom_list.end ( ) )
	{
		hIter->fLifeTime -= pWorld->GetTimeDelta ( );
		if ( hIter->fLifeTime <= 0.0f ) hIter = sHitFrom_list.erase ( hIter );
		else hIter++;
	}


	//Dave commented out
	/*if ( bDefending && !bManualControl )
	{
		float fDist = GetSqrDistTo ( vecDefendPos.x, vecDefendPos.y, vecDefendPos.z );
		if ( fDist > fDefendDist*fDefendDist )
		{
			ChangeState( pWorld->pEntityStates->pStateDefend );
		}
	}*/

	if ( !pCurrentState ) return;

	//pCurrentState->Execute ( this );

	bHit = false;
}

void Entity::ChangeState ( State *pNewState )
{
	if ( pCurrentState ) pCurrentState->Exit ( this );

	pLastState = pCurrentState;
	pCurrentState = pNewState;

	if ( pCurrentState ) pCurrentState->Enter ( this );
}

void Entity::ReturnToPreviousState ( )
{
	if ( pCurrentState ) pCurrentState->Exit ( this );

	if ( pLastState ) pCurrentState = pLastState;
	else pCurrentState = pWorld->pEntityStates->pStateIdle;
	pLastState = 0;

	if ( pCurrentState ) pCurrentState->Enter ( this );
}

void Entity::DebugDrawDestination ( )
{
	if ( iCurrDestObject == 0 )
	{
		iCurrDestObject = dbFreeObject ( );
		MakeObjectSphere ( iCurrDestObject, 2.0f, 2, 4 );
		SetObjectMask ( iCurrDestObject, 1 );
		ScaleLimb( iCurrDestObject, 0, 40*fRadius,40*fRadius,40*fRadius );
		SetObjectCollisionOff( iCurrDestObject );
	}

	PositionObject ( iCurrDestObject, vecCurrDest.x, vecCurrDest.y, vecCurrDest.z );
	ColorObject ( iCurrDestObject, 0xff640000 );
	SetObjectEmissive ( iCurrDestObject, 0xff640000 );
	SetObjectEffect ( iCurrDestObject, g_GUIShaderEffectID );
	
	SetObjectCollisionOff ( iCurrDestObject );
}

void Entity::DebugHideDestination ( )
{
	if ( iCurrDestObject > 0 && ObjectExist ( iCurrDestObject ) == 1 )
	{
		DeleteObject ( iCurrDestObject );
	}

	iCurrDestObject = 0;
}

void MakeTriangleSegment ( int iMeshNum, float fAngle, float fLength )
{
	if ( GetMeshExist ( iMeshNum ) == 1 ) DeleteMesh ( iMeshNum );
	float fSegAng = ( fAngle / 6.0f ) * DEGTORAD;
	
	int iObjNum = dbFreeObject ( );
	MakeObjectTriangle ( iObjNum, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, fLength, fLength * sin ( fSegAng ), 0.0f, fLength * cos ( fSegAng ) );
	SetObjectMask ( iObjNum, 1 );
	MakeMeshFromObject ( iMeshNum, iObjNum );

	fSegAng *= RADTODEG;

	for ( int i = 1; i < 6; i++ )
	{
		AddLimb ( iObjNum, i, iMeshNum );
		RotateLimb ( iObjNum, i, 0.0f, i * fSegAng, 0.0f );
	}

	dbCombineLimbs ( iObjNum );

	DeleteMesh ( iMeshNum );
	MakeMeshFromObject ( iMeshNum, iObjNum );
	DeleteObject ( iObjNum );
}

void Entity::DebugDrawObstacleAngles ( )
{
	DebugHideObstacleAngles ( );
	
	if ( sCombinedObstacleAngle_list.size( ) < 1 ) return;

	int iTempMesh = dbFreeMesh ( );
	iObstacleAnglesObject = dbFreeObject ( );
	
	float fAngDiff = sCombinedObstacleAngle_list [ 0 ].fAngEnd - sCombinedObstacleAngle_list [ 0 ].fAngBegin;
	float fLength = fRadius * 3.0f;
	
	MakeTriangleSegment ( iTempMesh, fAngDiff, fLength );
	MakeObject ( iObstacleAnglesObject, iTempMesh, 0 );
	SetObjectMask ( iObstacleAnglesObject, 1 );
	RotateLimb ( iObstacleAnglesObject, 0, 0.0f, sCombinedObstacleAngle_list [ 0 ].fAngBegin, 0.0f );
	
	for (int i = 1; i < (int) sCombinedObstacleAngle_list.size( ); i++ )
	{
		fAngDiff = sCombinedObstacleAngle_list [ i ].fAngEnd - sCombinedObstacleAngle_list [ i ].fAngBegin;
		
		MakeTriangleSegment ( iTempMesh, fAngDiff, fLength );

		AddLimb ( iObstacleAnglesObject, i, iTempMesh );
		RotateLimb ( iObstacleAnglesObject, i, 0.0f, sCombinedObstacleAngle_list [ i ].fAngBegin, 0.0f );
	}

	if ( GetMeshExist ( iTempMesh ) == 1 ) DeleteMesh ( iTempMesh );
	dbCombineLimbs ( iObstacleAnglesObject );

	PositionObject ( iObstacleAnglesObject, GetX( ), fDebugObstacleAnglesObjHeight, GetZ( ) );
	ColorObject ( iObstacleAnglesObject, 0xff00ff00 );
	SetObjectEmissive ( iObstacleAnglesObject, 0xff00ff00 );
	SetObjectEffect ( iObstacleAnglesObject, g_GUIShaderEffectID );

	SetObjectCollisionOff ( iObstacleAnglesObject );
}

void Entity::DebugHideObstacleAngles ( )
{
	if ( iObstacleAnglesObject > 0 && ObjectExist ( iObstacleAnglesObject ) == 1 )
	{
		DeleteObject ( iObstacleAnglesObject );
	}

	iObstacleAnglesObject = 0;
}

void Entity::DebugDrawViewArcs ( )
{
	int iTempMesh = dbFreeMesh ( );

	if ( iOuterViewObject == 0 ) 
	{
		iOuterViewObject = dbFreeObject ( );
		
		float fSegAngle = ( fOuterViewArc / 20.0f ) * DEGTORAD;

		MakeObjectTriangle ( iOuterViewObject, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, fViewRange, fViewRange * sin ( fSegAngle ), 0.0f, fViewRange * cos ( fSegAngle ) );
		SetObjectMask ( iOuterViewObject, 1 );
		MakeMeshFromObject ( iTempMesh, iOuterViewObject );

		fSegAngle *= RADTODEG;

		for ( int i = 1; i < 20; i++ )
		{
			AddLimb ( iOuterViewObject, i, iTempMesh );
			RotateLimb ( iOuterViewObject, i, 0.0f, i * fSegAngle, 0.0f );
		}

		DeleteMesh ( iTempMesh );
		MakeMeshFromObject ( iTempMesh, iOuterViewObject );
		AddLimb ( iOuterViewObject, 20, iTempMesh );
		RotateLimb ( iOuterViewObject, 20, 0.0f, -fOuterViewArc, 0.0f );
		
		dbCombineLimbs ( iOuterViewObject );
		DeleteMesh ( iTempMesh );

		ColorObject ( iOuterViewObject, 0xffff0000 );
		SetObjectEmissive ( iOuterViewObject, 0xffff0000 );
		SetObjectEffect ( iOuterViewObject, g_GUIShaderEffectID );

		SetAlphaMappingOn ( iOuterViewObject, 25.0f );
		DisableObjectZWrite ( iOuterViewObject );
	}

	if ( iHearingRangeObject == 0 ) 
	{
		iHearingRangeObject = dbFreeObject ( );

		float fSegAngle = ( 360.0f / 40.0f ) * DEGTORAD;

		MakeObjectTriangle ( iHearingRangeObject, 0.0f, 0.0f, fHearingRange - 1.0f, 0.0f, 0.0f, fHearingRange, fHearingRange * sin ( fSegAngle ), 0.0f, fHearingRange * cos ( fSegAngle ) );
		MakeMeshFromObject ( iTempMesh, iHearingRangeObject );
		DeleteObject ( iHearingRangeObject );
		MakeObjectTriangle ( iHearingRangeObject, 0.0f, 0.0f, fHearingRange - 1.0f, fHearingRange * sin ( fSegAngle ), 0.0f, fHearingRange * cos ( fSegAngle ), (fHearingRange - 1.0f) * sin ( fSegAngle ), 0.0f, (fHearingRange - 1.0f) * cos ( fSegAngle ) );
		AddLimb ( iHearingRangeObject, 1, iTempMesh );
		dbCombineLimbs ( iHearingRangeObject );
		DeleteMesh ( iTempMesh );
		MakeMeshFromObject ( iTempMesh, iHearingRangeObject );
		SetObjectMask ( iHearingRangeObject, 1 );

		fSegAngle *= RADTODEG;

		for ( int i = 1; i < 40; i++ )
		{
			AddLimb ( iHearingRangeObject, i, iTempMesh );
			RotateLimb ( iHearingRangeObject, i, 0.0f, i * fSegAngle, 0.0f );
		}

		dbCombineLimbs ( iHearingRangeObject );
		DeleteMesh ( iTempMesh );

		ColorObject ( iHearingRangeObject, 0xffffff00 );
		SetObjectEmissive ( iHearingRangeObject, 0xffffff00 );
		SetObjectEffect ( iHearingRangeObject, g_GUIShaderEffectID );
	}

	PositionObject ( iOuterViewObject, GetX ( ), fDebugViewObjHeight, GetZ ( ) );
	YRotateObject ( iOuterViewObject, GetAngleY ( ) );

	PositionObject ( iHearingRangeObject, GetX ( ), fDebugViewObjHeight, GetZ ( ) );

	SetObjectCollisionOff ( iOuterViewObject );
	SetObjectCollisionOff ( iHearingRangeObject );
}

void Entity::DebugHideViewArcs ( )
{
	if ( iInnerViewObject > 0 && ObjectExist ( iInnerViewObject ) )
	{
		DeleteObject ( iInnerViewObject );
	}

	iInnerViewObject = 0;

	if ( iOuterViewObject > 0 && ObjectExist ( iOuterViewObject ) )
	{
		DeleteObject ( iOuterViewObject );
	}

	iOuterViewObject = 0;

	if ( iHearingRangeObject > 0 && ObjectExist ( iHearingRangeObject ) )
	{
		DeleteObject ( iHearingRangeObject );
	}

	iHearingRangeObject = 0;
}

void Entity::DebugOutputInternalData ( HANDLE hOut )
{
	if ( !hOut ) return;

	DWORD dwWritten = 0;
	DWORD dwWrite = 0;
	char strOut [ 256 ];
	
	dwWrite = sprintf_s ( strOut, 255, "\n" );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, "Entity: %d\n", iID );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, "Container: %d\n", pContainer->GetID( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " -Active: %d,\n", bActive ? 1 : 0 );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );
	
	dwWrite = sprintf_s ( strOut, 255, " -Object Num: %d, Object Pointer %p\n", iID, pObject );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " -Radius: %f, Min Distance: %f\n", fRadius, fMinimumDist );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " -Height: %f\n", fHeight );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );
	
	dwWrite = sprintf_s ( strOut, 255, " -Movement Data " );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Destination Angle Y: %3.3f, Current Angle Y: %3.3f\n", fDestAngY, GetAngleY ( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );
	
	dwWrite = sprintf_s ( strOut, 255, " --Max Speed: %3.3f, Current Speed: %3.3f, Turn Speed: %3.3f\n", fSpeed, fCurrSpeed, fTurnSpeed );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Original Position: %3.3f, %3.3f, %3.3f\n", vecOrigPos.x, vecOrigPos.y, vecOrigPos.z );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Defend Position: %3.3f, %3.3f, %3.3f\n", vecDefendPos.x, vecDefendPos.y, vecDefendPos.z );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Current Position: %3.3f, %3.3f, %3.3f\n", GetX ( ), GetY ( ), GetZ ( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Current Destination: %3.3f, %3.3f, %3.3f\n", vecCurrDest.x, vecCurrDest.y, vecCurrDest.z );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Final Destination: %3.3f, %3.3f, %3.3f\n", vecFinalDest.x, vecFinalDest.y, vecFinalDest.z );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Avoid Position: %3.3f, %3.3f, %3.3f\n", vecAvoidPos.x, vecAvoidPos.y, vecAvoidPos.z );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Interest Position: %3.3f, %3.3f, %3.3f\n", vecInterestPos.x, vecInterestPos.y, vecInterestPos.z );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " -View Data\n" );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Inner View Arc: %3.3f  Outer View Arc: %3.3f  Fire Arc: %3.3f\n", fInnerViewArc, fOuterViewArc, fFireArc );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --View Range: %3.3f  Hearing Range: %3.3f\n", fViewRange, fHearingRange );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " -State Data\n" );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Current State: %s\n", pCurrentState->GetName ( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --State Timer: %f\n", fStateTimer );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );
	
	dwWrite = sprintf_s ( strOut, 255, " -Other\n" );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Can Fire: %d  Is Hit: %d  Heard Sound: %d\n", bFireWeapon, bHit, bHeardSound );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Num Hit Points: %d\n", CountHitPoints( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Aggressiveness: %d\n", iAggressiveness );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Container: %d\n", pContainer ? pContainer->GetID() : -1 );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Changing Containers: %d\n", bChangingContainers ? 1 : 0 );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Number of Targets: %d\n", CountTargets( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Hit Something: %d  Hit By Something: %d\n", bHitSomething, bHitBySomething );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Hit Something: %p  Hit By Something: %p\n", pHitEntity, pHitByEntity );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Avoiding: %d  Making Progress: %d  Stuck: %d\n", bAvoiding, bMakingProgress, bStuck );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Moving: %d  In A Polygon: %d\n", bMoving, pContainer ? pContainer->pPathFinder->InPolygons ( GetX( ), GetZ( ) ) : 0 );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Following: %d  Defending: %d  Defend Dist: %3.3f\n", bFollowingLeader, bDefending, fDefendDist );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Manual Control: %d  Look At Point Set: %d\n", bManualControl, bLookAtPointSet );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Patrol Path ID: %d  Pointer: %p\n", iPatrolPathID, pPatrolPath );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Current Patrol Point: %d  Num Points: %d\n", iCurrentPatrolPoint, CountPatrolPoints( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Current Avoid Point: %d  Num Points: %d\n", iCurrentAvoidPoint, cAvoidPath.CountPoints( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Current Move Point: %d  Move Path Points: %d\n", iCurrentMovePoint, cMovePath.CountPoints ( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	dwWrite = sprintf_s ( strOut, 255, " --Obstacle Angles: %d\n", sCombinedObstacleAngle_list.size( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	for ( int i = 0; i < (int) sCombinedObstacleAngle_list.size( ); i++ )
	{
		dwWrite = sprintf_s ( strOut, 255, " ---Angle Begin: %f  Angle End: %f\n", sCombinedObstacleAngle_list [ i ].fAngBegin, sCombinedObstacleAngle_list [ i ].fAngEnd );
		WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );
	}

	dwWrite = sprintf_s ( strOut, 255, " --Targets: %d  Valid Target: %d\n", CountTargets( ), ValidTarget( ) );
	WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );

	for ( int i = 0; i < (int) CountTargets( ); i++ )
	{
		dwWrite = sprintf_s ( strOut, 255, " ---ID: %d  Threat: %f  Time: %f  Zone: %d Container: %d\n", sTarget_list [ i ].iTargetID, sTarget_list [ i ].fTargetThreat, sTarget_list [ i ].fTargetTimer, sTarget_list [ i ].bZoneTarget, sTarget_list [ i ].iContainer );
		WriteConsole ( hOut, strOut, dwWrite, &dwWritten, NULL );
	}

}

void Entity::SetCoverPoint( sCoverPoint* pPoint ) 
{ 
	pCoverPoint = pPoint; 
	if ( pCoverPoint ) pCoverPoint->InUse();
}

void Entity::ClearCoverPoint() 
{ 
	Container* pCont = cWorld.GetContainer(0);
	if ( !pCont ) return;

	PathFinderAdvanced* pCoverList = NULL;
	if ( !pCont->pPathFinder ) return;

	sCoverPoint* pItem = pCont->pPathFinder->pCoverPointList;
	while ( pItem )
	{
		if ( pCoverPoint==pItem ) { pCoverPoint->Unused(); break; }
		pItem = pItem->pNextPoint;
	}
	pCoverPoint = 0;
}

sCoverPoint* Entity::GetCoverPoint()
{
	return pCoverPoint;
}

void Entity::MoveToCoverPoint()
{
	if ( !pCoverPoint ) return;

	if ( prevCoverX.size() > 10 )
	{
		prevCoverX.clear();
		prevCoverZ.clear();
	}

	prevCoverX.push_back(pCoverPoint->fX);
	prevCoverZ.push_back (pCoverPoint->fY);
	SetDestination( pCoverPoint->fX, GetY(), pCoverPoint->fY, pCoverPoint->iContainer );
}

float Entity::GetLeapRange()
{
	return fLeapRange;
}

float Entity::GetDiveRange()
{
	return fDiveRange;
}

int Entity::GetMovePoints()
{
	return cMovePath.CountPoints();
}

void Entity::SetDiveMultiplier( float value )
{
	fDiveMultiplier = value;
}

void Entity::SetLeapMultiplier( float value )
{
	fLeapMultiplier = value;
}
