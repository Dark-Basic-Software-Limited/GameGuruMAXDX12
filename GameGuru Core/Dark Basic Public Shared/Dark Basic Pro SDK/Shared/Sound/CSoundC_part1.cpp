DARKSDK void Load3DSound ( LPSTR szFilename, int iID, int iSilentFail, int iGlobalSound )
{
	// leeadd - 120108 - U&1 - Sound is 3D
	LoadRawSound ( szFilename, iID, true, iSilentFail, iGlobalSound );
}

DARKSDK void PositionSound ( int iID, float fX, float fY, float fZ )
{
#ifdef WICKEDAUDIO
#else

	//return;

	// if no sound card, leave now
	if (!g_pSoundManager) return;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return;
	}
#ifdef WICKEDAUDIO
	m_ptr->vecPosition = GGVECTOR3(fX / 100.0f, fY / 100.0f, fZ / 100.0f);
	SoundComponent* sound = GetScene().sounds.GetComponent(m_ptr->wickedEntity);
	if (sound != nullptr)
	{
		Entity entity = m_ptr->wickedEntity;
		TransformComponent* transform = GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local = XMFLOAT3(1, 1, 1);
			transform->rotation_local = XMFLOAT4(0, 0, 0, 0);
			transform->translation_local = XMFLOAT3(fX, fY, fZ);
			transform->SetDirty();
			transform->UpdateTransform();
		}
	}
#else
	if ( m_ptr->pDSBuffer3D==NULL )
		return;

	if (!m_ptr->bPlaying)
		return;

	// Adjust distance
	fX/=100.0f;
	fY/=100.0f;
	fZ/=100.0f;

	// positions a sound in 3D space
	m_ptr->vecPosition = GGVECTOR3 ( fX, fY, fZ );
	if ( fabs ( GGVec3Length ( &(m_ptr->vecLast - m_ptr->vecPosition) ) ) > 0.09f )
	{
		#ifndef _DEBUG
		m_ptr->pDSBuffer3D->SetPosition ( fX, fY, fZ, DS3D_DEFERRED );
		#endif
		m_ptr->vecLast = m_ptr->vecPosition;
	}
#endif
}

// listener is a hog - only update if changes
float g_fListenQuickScale = 0.0f;
GGVECTOR3 g_vListenQuickPos = GGVECTOR3(-1, -1, -1);
GGVECTOR3 g_vListenQuickAngle = GGVECTOR3(-1, -1, -1);

DARKSDK void ResetListener ( void )
{
	g_fListenQuickScale = 0.0f;
	g_vListenQuickPos = GGVECTOR3(-1, -1, -1);
	g_vListenQuickAngle = GGVECTOR3(-1, -1, -1);
}

DARKSDK void PositionListener ( float fX, float fY, float fZ )
{
#ifdef WICKEDAUDIO
	//PE: Always camera for now.
	const wiScene::CameraComponent& camera = wiScene::GetCamera();
	vecListenerPosition = GGVECTOR3(fX / 100.0f, fY / 100.0f, fZ / 100.0f);
#else
	//return;
	if ( pDSListener )
	{
		// Adjust distance
		fX/=100.0f;
		fY/=100.0f;
		fZ/=100.0f;

		// store the position and set it
		vecListenerPosition = GGVECTOR3 ( fX, fY, fZ );
		//PE: xyz / 100 so 5.0 = 500 units, think we need to update faster :)
		if (GGVec3Length(&(vecListenerPosition - g_vListenQuickPos)) > 0.5f) //5.0f
		{
			g_vListenQuickPos = vecListenerPosition;
			#ifndef _DEBUG
			pDSListener->SetPosition (fX, fY, fZ, DS3D_DEFERRED);
			#endif
		}
	}
#endif
}

DARKSDK void RotateListener ( float fX, float fY, float fZ )
{
#ifdef WICKEDAUDIO
	//PE: Always camera for now.
	vecListenerAngle.x = fX;
	vecListenerAngle.y = fY;
	vecListenerAngle.z = fZ;
#else
	//return;
	if ( pDSListener )
	{
		// for reference
		vecListenerAngle.x = fX;
		vecListenerAngle.y = fY;
		vecListenerAngle.z = fZ;

		// quicker method
		float fOldAngle = fabs(vecListenerAngle.x) + fabs(vecListenerAngle.y) + fabs(vecListenerAngle.z);
		float fNewAngle = fabs(g_vListenQuickAngle.x) + fabs(g_vListenQuickAngle.y) + fabs(g_vListenQuickAngle.z);
		if (fabs(fabs(fNewAngle) - fabs(fOldAngle)) > 5.0f || g_vListenQuickAngle.x == -1)
		{
			// record new angle
			g_vListenQuickAngle = vecListenerAngle;

			// Set Angle Rotation
			GGMATRIX matFront, matTop, matTemp, matRot;

			// Convert angle formats
			fX = GGToRadian (fX);
			fY = GGToRadian (fY);
			fZ = GGToRadian (fZ);

			// Set Front and Top
			ZeroMemory(&matFront, sizeof(GGMATRIX));
			matFront._41 = 0.0f;
			matFront._42 = 0.0f;
			matFront._43 = 1.0f;
			ZeroMemory(&matTop, sizeof(GGMATRIX));
			matTop._41 = 0.0f;
			matTop._42 = 1.0f;
			matTop._43 = 0.0f;

			// Produce and combine the rotation matrices.
			GGMatrixIdentity(&matRot);
			GGMatrixRotationX(&matTemp, fX);
			GGMatrixMultiply(&matRot, &matTemp, &matRot);
			GGMatrixRotationY(&matTemp, fY);
			GGMatrixMultiply(&matRot, &matTemp, &matRot);
			GGMatrixRotationZ(&matTemp, fZ);
			GGMatrixMultiply(&matRot, &matTemp, &matRot);

			// Apply the rotation matrices to complete the matrix.
			GGMatrixMultiply(&matFront, &matFront, &matRot);
			GGMatrixMultiply(&matTop, &matTop, &matRot);

			// rotate the listener data
			#ifndef _DEBUG
			pDSListener->SetOrientation(matFront._41, matFront._42, matFront._43, matTop._41, matTop._42, matTop._43, DS3D_DEFERRED);
			#endif
		}
	}
#endif
}

DARKSDK void ScaleListener ( float fScale )
{
#ifdef WICKEDAUDIO
	//PE: Always camera for now.
#else

	if ( pDSListener )
	{
		if (fScale != g_fListenQuickScale )
		{
			// check all sounds and alter min/max defaults
			g_fListenQuickScale = fScale;
			link* check = m_SDKSoundManager.GetList();
			while (check)
			{
				sSoundData* ptr = NULL;
				ptr = m_SDKSoundManager.GetData (check->id);
				if (ptr == NULL) continue;

				// scale min and max (3000 is the default scene camera range)
				if (ptr->pDSBuffer3D)
				{
					#ifndef _DEBUG
					ptr->pDSBuffer3D->SetMinDistance(DS3D_DEFAULTMINDISTANCE * fScale, DS3D_DEFERRED);
					ptr->pDSBuffer3D->SetMaxDistance(3000.0f * fScale, DS3D_DEFERRED);
					#endif
				}

				// Next sound
				check = check->next;
			}
		}
	}
#endif
}

DARKSDK void SetEAX ( int iEffect ) 
{
	// Not Implemented in DBPRO V1 RELEASE
	#ifndef IGNOREALLSOUNDERRORS
	RunTimeError(RUNTIMEERROR_COMMANDNOWOBSOLETE);
	#endif
}

DARKSDK int SoundExist ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}

	// returns true if the sound exists

	// get data
	if ( !UpdateSoundPtr ( iID ) )
		return 0;
	
	return 1;
}

DARKSDK int SoundType ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get the sound type
	
	// check the 3D flag
	if ( m_ptr->b3D )
		return 1;

	return 0;
}

DARKSDK int SoundPlaying ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

#ifdef WICKEDAUDIO
	SoundComponent* sound = GetScene().sounds.GetComponent(m_ptr->wickedEntity);
	if (sound != nullptr)
	{ 
		//PE: This is not correct need to check if it has finish.
		//PE: OnStreamEnd fixed it :)
		//uint32_t cf = wiAudio::GetCallBackF(&sound->soundinstance);
		//uint32_t cs = wiAudio::GetCallBackS(&sound->soundinstance);
		bool bPlaying = wiAudio::bIsReallyPlaying(&sound->soundinstance); //Flush so restart.
		if (!m_ptr->bPlaying)
			bPlaying = false;
		return(bPlaying);
	}
#else
	// is the sound playing
	if (m_ptr->bPlaying)
		return 1;
	else
		return 0;
#endif
}

DARKSDK int SoundLooping ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// is the sound looping

	// return loop flag
	return m_ptr->bLoop;
}

DARKSDK int SoundPaused ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// is the sound paused

	// return pause flag
	return m_ptr->bPause;
}

DARKSDK int SoundPan ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get the pan of a sound
	if ( m_ptr->b3D )
		return 0;

#ifdef WICKEDAUDIO
	return 0;
#else

	long pan;
	
	// get the pan
	m_ptr->pSound->GetBuffer(0)->GetPan ( &pan );

	// return
	return ( int ) pan;
#endif
}

DARKSDK int SoundSpeed ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get speed of a sound
	DWORD dwSpeed;
#ifdef WICKEDAUDIO
	return(0);
#else
	// get the speed
	m_ptr->pSound->GetBuffer(0)->GetFrequency ( &dwSpeed );

	// return
	return ( int ) dwSpeed;
#endif
}

DARKSDK int SoundVolume ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get sound volume

	// return volume
	return m_ptr->iVolume;
}

DARKSDK DWORD SoundPositionXEx ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get sound x position

	// return x position
	float fResult = m_ptr->vecPosition.x*100.0f;
	return *(DWORD*)&fResult;
}

DARKSDK DWORD SoundPositionYEx ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get sound y position

	// return y position
	float fResult = m_ptr->vecPosition.y*100.0f;
	return *(DWORD*)&fResult;
}

DARKSDK DWORD SoundPositionZEx ( int iID )
{
#ifdef WICKEDAUDIO
#else
	// if no sound card, leave now
	if (!g_pSoundManager) return 0;

	// mike - 010904 - 5.7 - silent return if sound card does not exist
	if ( !g_pSoundManager->GetExists ( ) )
		return 0;
#endif

	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	// get sound z position

	// return z position
	float fResult = m_ptr->vecPosition.z*100.0f;
	return *(DWORD*)&fResult;
}

DARKSDK DWORD ListenerPositionXEx ( void )
{
	// get listener x position
	float fResult = vecListenerPosition.x*100.0f;
	return *(DWORD*)&fResult;
}

DARKSDK DWORD ListenerPositionYEx ( void )
{
	// get listener y position
	float fResult = vecListenerPosition.y*100.0f;
	return *(DWORD*)&fResult;
}

DARKSDK DWORD ListenerPositionZEx ( void )
{
	// get listener z position
	float fResult = vecListenerPosition.z*100.0f;
	return *(DWORD*)&fResult;
}

DARKSDK DWORD ListenerAngleXEx ( void )
{
	// get angle x
	return *(DWORD*)&vecListenerAngle.x;
}

DARKSDK DWORD ListenerAngleYEx ( void )
{
	// get angle y
	return *(DWORD*)&vecListenerAngle.y;
}

DARKSDK DWORD ListenerAngleZEx ( void )
{
	// get angle z
	return *(DWORD*)&vecListenerAngle.z;
}

//
// Data Access Functions
//

DARKSDK void GetSoundData( int iID, DWORD* dwBitsPerSecond, DWORD* Frequency, DWORD* Duration, LPSTR* pData, DWORD* dwDataSize, bool bLockData, WAVEFORMATEX* wfx )
{
#ifdef WICKEDAUDIO
	//PE: Only used by memblocks that we do not use in MAX.
#else

	// mike - 300305 - new param for waveformat

	// Read Data
	if(bLockData==true)
	{
		// get ptr to sound
		if ( !UpdateSoundPtr ( iID ) )
			return;

		// may need to extract data from a clones real sound

		// Sound Buffer Data
		DWORD dwSourceSize = m_ptr->pSound->GetBufferSize();
		LPSTR pSourcePtr = m_ptr->pSound->GetBufferData();

		// data
	    WAVEFORMATEX* pwfx = m_ptr->pSound->m_pWaveFile->GetFormat();

		// mike - 300305 - need to store extra wave data
		memcpy ( wfx, pwfx, sizeof ( WAVEFORMATEX ) );

		*Frequency = pwfx->nAvgBytesPerSec/(pwfx->wBitsPerSample/8);
		*dwBitsPerSecond = pwfx->nAvgBytesPerSec*8;
		*Duration = dwSourceSize/pwfx->nAvgBytesPerSec;

		// create memory
		*pData = new char[dwSourceSize];
		*dwDataSize = dwSourceSize;

		// copy sound data
		LPSTR pPtr = *pData;
		memcpy(pPtr, pSourcePtr, dwSourceSize);
	}
	else
	{
		// free memory
		delete *pData;
	}
#endif
}

DARKSDK void SetSoundData( int iID, DWORD dwBitsPerSecond, DWORD Frequency, DWORD Duration, LPSTR pData, DWORD dwDataSize, WAVEFORMATEX wfx )
{
#ifdef WICKEDAUDIO
	//PE: Only used by memblocks that we do not use in MAX.
#else

	// mike - 300305 - new param for waveformat

	// Delete if exists
	if ( UpdateSoundPtr ( iID ) ) DeleteSound ( iID );

	// add structure to sound manager
	sSoundData m_Data;	
	memset ( &m_Data, 0, sizeof ( m_Data ) );
	m_SDKSoundManager.Add ( &m_Data, iID );
	if ( !UpdateSoundPtr ( iID ) ) return;

	// fill with sound data
	m_ptr->iVolume=100;
	m_ptr->iPan=100;

	// temo sound file
	char path[_MAX_PATH];
	DB_GetWinTemp(path);
	char szFilename[_MAX_PATH];

	// U69 - 050608 - add folder seperator only if required
	strcpy ( szFilename, path );
	if ( strncmp ( path + strlen ( path ) - 1, "\\", 1 )!=NULL ) strcat ( szFilename, "\\" );
	strcat ( szFilename, "tempfile.wav" );

	// sample quality
	DWORD nSamplesPerSec=Frequency;

	// save sound
	DeleteFile(szFilename);
	DB_SaveWAVFile(szFilename, pData, dwDataSize, nSamplesPerSec, wfx );

    // Load the wave file into a DirectSound buffer
    HRESULT hr = g_pSoundManager->Create( &m_ptr->pSound, szFilename, DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY, SOUND_GUID_NULL );  
    if( FAILED( hr ) )
	{
		// no sound buffer
		m_SDKSoundManager.Delete ( iID );
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDLOADFAILED,szFilename);
		#endif
        return;
	}

	// Delete temo sound file
	DeleteFile(szFilename);
#endif
}

// MIKE - 090104
//PE: Not used.
/*
DARKSDK LPDIRECTSOUND8 GetSoundInterface ( void )
{
	// if no sound card, leave now
	if (!g_pSoundManager) return NULL;

	return g_pSoundManager->m_pDS;
}
*/
// MIKE - 100204
//PE: Not used.
/*
DARKSDK IDirectSound3DBuffer8* GetSoundBuffer ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNUMBERILLEGAL);
		#endif
		return 0;
	}
	if ( !UpdateSoundPtr ( iID ) )
	{
		#ifndef IGNOREALLSOUNDERRORS
		RunTimeError(RUNTIMEERROR_SOUNDNOTEXIST);
		#endif
		return 0;
	}

	return m_ptr->pDSBuffer3D;
}
*/

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////////
// DARK SDK SECTION //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

#ifdef DARKSDK_COMPILE

void ConstructorSound ( HWND hWnd )
{
	Constructor ( hWnd );
}

void DestructorSound ( void )
{
	Destructor ( );
}

void SetErrorHandlerSound	( LPVOID pErrorHandlerPtr )
{
	SetErrorHandler ( pErrorHandlerPtr );
}

void PassCoreDataSound ( LPVOID pGlobPtr )
{
	PassCoreData ( pGlobPtr );
}

void RefreshGRAFIXSound ( int iMode )
{
	RefreshGRAFIX ( iMode );
}

void UpdateSound ( void )
{
	Update ( );
}

void dbLoadSound ( LPSTR szFilename, int iID )
{
	LoadSound ( szFilename, iID );
}

void dbLoadSound ( LPSTR szFilename, int iID, int iFlag )
{
	LoadSound ( szFilename, iID, iFlag );
}

void dbCloneSound ( int iDestination, int iSource )
{
	CloneSound ( iDestination, iSource );
}

void dbDeleteSound ( int iID )
{
	DeleteSound ( iID );
}

void dbPlaySound ( int iID )
{
	PlaySound ( iID );
}

void dbPlaySound ( int iID, int iOffset )
{
	PlaySoundOffset ( iID, iOffset );
}

void dbLoopSound ( int iID )
{
	LoopSound ( iID );
}

void dbLoopSound ( int iID, int iStart )
{
	LoopSound ( iID, iStart );
}

void dbLoopSound ( int iID, int iStart, int iEnd )
{
	LoopSound ( iID, iStart, iEnd );
}

void dbLoopSound ( int iID, int iStart, int iEnd, int iInitialPos )
{
	LoopSound ( iID, iStart, iEnd, iInitialPos );
}

void dbStopSound ( int iID )
{
	StopSound ( iID );
}

void dbResumeSound ( int iID )
{
	ResumeSound ( iID );
}

void dbPauseSound ( int iID )
{
	PauseSound ( iID );
}

void dbSetSoundPan ( int iID, int iPan )
{
	SetSoundPan ( iID, iPan );
}

void dbSetSoundSpeed ( int iID, int iFrequency )
{
	SetSoundSpeed ( iID, iFrequency );
}

void dbSetSoundVolume ( int iID, int iVolume )
{
	SetSoundVolume ( iID, iVolume );
}

void dbRecordSound ( int iID )
{
	RecordSound ( iID );
}

void dbRecordSound ( int iID, int iCaptureDuration )
{
	RecordSound ( iID, iCaptureDuration );
}

void dbStopRecordingSound ( void )
{
	StopRecordingSound ( );
}

void dbSaveSound ( LPSTR szFilename, int iID )
{
	SaveSound ( szFilename, iID );
}

void dbLoad3DSound ( LPSTR szFilename, int iID )
{
	Load3DSound ( szFilename, iID );
}

void dbLoad3DSound ( LPSTR szFilename, int iID, int iSilentFail )
{
    Load3DSound ( szFilename, iID, iSilentFail );
}

void dbLoad3DSound ( LPSTR szFilename, int iID, int iSilentFail, int iGlobalSound )
{
    Load3DSound ( szFilename, iID, iSilentFail, iGlobalSound );
}

void dbPositionSound ( int iID, float fX, float fY, float fZ )
{
	PositionSound (  iID,  fX,  fY,  fZ );
}

void dbPositionListener ( float fX, float fY, float fZ )
{
	PositionListener (  fX,  fY,  fZ );
}

void dbRotateListener ( float fX, float fY, float fZ )
{
	RotateListener (  fX,  fY,  fZ );
}

void dbScaleListener ( float fScale )
{
	ScaleListener (  fScale );
}

void dbSetEAX ( int iEffect )
{
	SetEAX ( iEffect );
}

int dbSoundExist ( int iID )
{
	return GetSoundExist ( iID );
}

int dbSoundType ( int iID )
{
	return GetSoundType ( iID );
}

int dbSoundPlaying ( int iID )
{
	return GetSoundPlaying ( iID );
}

int dbSoundLooping ( int iID )
{
	return GetSoundLooping ( iID );
}

int dbSoundPaused ( int iID )
{
	return GetSoundPaused ( iID );
}

int dbSoundPan ( int iID )
{
	return GetSoundPan ( iID );
}

int dbSoundSpeed ( int iID )
{
	return GetSoundSpeed ( iID );
}

int dbSoundVolume ( int iID )
{
	return GetSoundVolume ( iID );
}

float dbSoundPositionX ( int iID )
{
	DWORD dwReturn = GetSoundPositionXEx ( iID );
	
	return *( float* ) &dwReturn;
}

float dbSoundPositionY ( int iID )
{
	DWORD dwReturn = GetSoundPositionYEx ( iID );
	
	return *( float* ) &dwReturn;
}

float dbSoundPositionZ ( int iID )
{
	DWORD dwReturn = GetSoundPositionZEx ( iID );
	
	return *( float* ) &dwReturn;
}

float dbListenerPositionX ( void )
{
	DWORD dwReturn = GetListenerPositionXEx ( );
	
	return *( float* ) &dwReturn;
}

float dbListenerPositionY ( void )
{
	DWORD dwReturn = GetListenerPositionYEx ( );
	
	return *( float* ) &dwReturn;
}

float dbListenerPositionZ ( void )
{
	DWORD dwReturn = GetListenerPositionZEx ( );
	
	return *( float* ) &dwReturn;
}

float dbListenerAngleX ( void )
{
	DWORD dwReturn = GetListenerAngleXEx ( );
	
	return *( float* ) &dwReturn;
}

float dbListenerAngleY ( void )
{
	DWORD dwReturn = GetListenerAngleYEx ( );
	
	return *( float* ) &dwReturn;
}

float dbListenerAngleZ ( void )
{
	DWORD dwReturn = GetListenerAngleZEx ( );
	
	return *( float* ) &dwReturn;
}

float dbGetSoundPositionX ( int iID )
{
	return GetSoundPositionX ( iID );
}

float dbGetSoundPositionY ( int iID )
{
	return GetSoundPositionY ( iID );
}

float dbGetSoundPositionZ ( int iID )
{
	return GetSoundPositionZ ( iID );
}

float dbGetListenerPositionX ( int iID )
{
	return 0.0f;

	//return GetListenerPositionX ( iID );
}

float dbGetListenerPositionY ( int iID )
{
	return 0.0f;

	//return GetListenerPositionY ( iID );
}

float dbGetListenerPositionZ ( int iID )
{
	return 0.0f;

	//return GetListenerPositionZ ( iID );
}

float dbGetListenerAngleX ( int iID )
{
	return 0.0f;
	//return GetListenerAngleX ( iID );
}

float dbGetListenerAngleY ( int iID )
{
	return 0.0f;
	//return GetListenerAngleY ( iID );
}

float dbGetListenerAngleZ ( int iID )
{
	return 0.0f;
	//return GetListenerAngleZ ( iID );
}

void dbGetSoundData	( int iID, DWORD* dwBitsPerSecond, DWORD* Frequency, DWORD* Duration, LPSTR* pData, DWORD* dwDataSize, bool bLockData, WAVEFORMATEX* wfx )
{
	GetSoundData ( iID, dwBitsPerSecond, Frequency, Duration, pData, dwDataSize, bLockData, wfx );
}

void dbSetSoundData	( int iID, DWORD dwBitsPerSecond, DWORD Frequency, DWORD Duration, LPSTR pData, DWORD dwDataSize, WAVEFORMATEX wfx )
{
	SetSoundData ( iID, dwBitsPerSecond, Frequency, Duration, pData, dwDataSize, wfx );
}

#endif

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
