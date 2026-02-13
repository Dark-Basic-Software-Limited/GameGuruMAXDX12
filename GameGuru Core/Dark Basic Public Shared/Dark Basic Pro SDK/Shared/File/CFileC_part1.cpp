DARKSDK int ReadByte( int f )
{
	int iResult=0;
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Read from file
			unsigned char data;
			if(ReadFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			if(bytes==0) FileEOF[f]=TRUE;

			iResult = data;
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

	return iResult;
}

DARKSDK int ReadWord( int f )
{
	int iResult=0;
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Read from file
			WORD data;
			if(ReadFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			if(bytes==0) FileEOF[f]=TRUE;

			iResult = data;
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

	return iResult;
}

DARKSDK int ReadLong( int f )
{
	int iResult=0;
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Read from file
			DWORD data;
			if(ReadFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			if(bytes==0) FileEOF[f]=TRUE;

			iResult = data;
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

	return iResult;
}

DARKSDK float ReadFloat( int f )
{
	float fResult=0.0f;
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Read from file
			float data;
			if(ReadFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			if(bytes==0) FileEOF[f]=TRUE;

			fResult = data;
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

	return fResult;
}

//PE: Used a lot but GetReturnStringFromWorkString never freed.
//PE: Most calls use cstr so pointer to free is lost.
int ringReadString = 0;
char cReadString[20][1025];

DARKSDK LPSTR ReadString( int f )
{
    /*
        20091129 v75 - IRM - http://forum.thegamecreators.com/?m=forum_view&t=81894&b=15
        Use of fixed size buffers (1024 bytes for m_pWorkString and 2048 for internal
        buffer in this function gave two chances for buffer overruns to crash the program.
        
        Replaced the use of these buffers with a single buffer implemented using an
        std::vector, and changed the routine GetReturnStringFromWorkString to allow an
        optional buffer to be provided (defaults to m_pWorkString).
    */

	LPSTR pReturnString = 0;
	ringReadString = ringReadString + 1;
	if (ringReadString > 19)
		ringReadString = 0;


	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			unsigned char c=0;
            DWORD bytes;
            std::vector<char> WorkString;

			bool eof=false;
			do
			{
				if(ReadFile(File[f], &c, 1, &bytes, NULL)==0)
				{
					RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
					goto fileerror;
				}
				if(bytes==0)
                {
                    FileEOF[f]=TRUE;
					eof=true;
                }
				else if(c>=32 || c==9)
                {
                    WorkString.push_back(c);
                }
			} while((c>=32 || c==9) && !eof);

            WorkString.push_back(0);

			if(c==13)
			{
				if(ReadFile(File[f], &c, 1, &bytes, NULL)==0)
				{
					RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
					goto fileerror;
				}
				if(bytes==0) FileEOF[f]=TRUE;
			}

	        // Create and return string
			if (WorkString.size() < 1024)
			{
				strcpy(&cReadString[ringReadString][0], &WorkString[0]);
				pReturnString = &cReadString[ringReadString][0];
			}
			else
			{
				//PE: Just in case we have some larger string.
				pReturnString = GetReturnStringFromWorkString(&WorkString[0]);
			}
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

fileerror:

    return pReturnString;
}

//PE: We can have 0x0a in soundset4 (entered text) so always use 13 to stop.
DARKSDK LPSTR ReadStringIncl0xA(int f)
{
	/*
		20091129 v75 - IRM - http://forum.thegamecreators.com/?m=forum_view&t=81894&b=15
		Use of fixed size buffers (1024 bytes for m_pWorkString and 2048 for internal
		buffer in this function gave two chances for buffer overruns to crash the program.

		Replaced the use of these buffers with a single buffer implemented using an
		std::vector, and changed the routine GetReturnStringFromWorkString to allow an
		optional buffer to be provided (defaults to m_pWorkString).
	*/

	LPSTR pReturnString = 0;

	if (f >= 1 && f <= MAX_FILES)
	{
		if (File[f] != NULL)
		{
			unsigned char c = 0;
			DWORD bytes;
			std::vector<char> WorkString;

			bool eof = false;
			do
			{
				if (ReadFile(File[f], &c, 1, &bytes, NULL) == 0)
				{
					RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
					goto fileerror;
				}
				if (bytes == 0)
				{
					FileEOF[f] = TRUE;
					eof = true;
				}
				else if (c >= 32 || c == 9 || c == 10 )
				{
					WorkString.push_back(c);
				}
			} while ((c >= 32 || c == 9 || c == 10) && !eof);

			WorkString.push_back(0);

			if (c == 13)
			{
				if (ReadFile(File[f], &c, 1, &bytes, NULL) == 0)
				{
					RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
					goto fileerror;
				}
				if (bytes == 0) FileEOF[f] = TRUE;
			}

			// Create and return string
			pReturnString = GetReturnStringFromWorkString(&WorkString[0]);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

fileerror:

	return pReturnString;
}

DARKSDK void ReadFileBlockCore(char* FilenameString, int f )
{
	// Get Size of fileblock
	DWORD bytes;
	DWORD nSize;
	ReadFile(File[f], &nSize, 4, &bytes, NULL);

	// Create mem for it
	char* pBuffer = (char*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, nSize);
	if(pBuffer)
	{
		// Read it in
		ReadFile(File[f], pBuffer, nSize, &bytes, NULL);
		if(bytes==0) FileEOF[f]=TRUE;

		// Write it to file
		DWORD byteswritten;
		HANDLE hwritefile = GG_CreateFile(FilenameString, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hwritefile!=INVALID_HANDLE_VALUE)
		{
			WriteFile(hwritefile, pBuffer, nSize, &byteswritten, NULL);
			CloseHandle(hwritefile);		
		}

		// Free buffer
		GlobalFree(pBuffer);
		pBuffer=NULL;
	}
}

DARKSDK void MakePathToThisFolder(char* thepathiwant)
{
	// get directory desired
	char file[_MAX_PATH];
	strcpy(file, thepathiwant);

	// Get path from filename (upto 8 nests)
	char filepath[8][256];
	int filepathindex=0;
	for(unsigned int n=0; n<8; n++)
		strcpy(filepath[n],"");

	// mike - 020206 - addition for vs8
	//for(n=0; n<strlen(file); n++)
	for(unsigned int n=0; n<strlen(file); n++)
	{
		if(file[n]=='\\')
		{
			// mike - 020206 - addition for vs8
			unsigned int o = 0;

			// Get folder name
			char folder[256];
			
			//for(unsigned int o=0; o<n; o++)
			for(o=0; o<n; o++)
				folder[o]=file[o];
			folder[o]=0;

			// Copy and store it
			strcpy(filepath[filepathindex], folder);
			if(filepathindex<7) filepathindex++;

			// Truncate and continue
			unsigned int q=0;
			for(unsigned int p=n+1; p<=strlen(file); p++)
				file[q++]=file[p];
			file[q]=0;
			n=0;
		}
	}

	// Store current directory
	char olddir[256];
	_getcwd(olddir, 256);

	// If filename has a path, and it doesnt exist, make it
	for(int m=0; m<filepathindex; m++)
	{
		if(strcmp(filepath[m],"")!=0)
		{
			if(_chdir(filepath[m])==-1)
			{
				mkdir(filepath[m]);
				_chdir(filepath[m]);
			}
		}
	}

	// Restore directory
	_chdir(olddir);
}

DARKSDK void ReadFileBlock( int f, DWORD pFilename )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			char* FilenameString = (LPSTR)pFilename;

			// Store current directory
			char olddir[256];
			getcwd(olddir, 256);

			// If directory doesn't exist, create one
			char DirString[256];
			strcpy(DirString, FilenameString);

			// mike - 020206 - addition for vs8
			int n = 0;
			//for(int n=strlen(DirString)-1; n>0 ; n--)
			for(n=strlen(DirString)-1; n>0 ; n--)
				if(DirString[n]=='\\') break;
			DirString[n+1]=0;
			MakePathToThisFolder(DirString);

			// Create file (nicely into dir if created earlier)
			ReadFileBlockCore(FilenameString, f);

			// Restore directory
			_chdir(olddir);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void SkipBytes( int f, int iSkipValue )
{
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			DWORD nSize = (int)iSkipValue;
			char* pBuffer = (char*)GlobalAlloc(GMEM_FIXED, nSize);
			if(pBuffer)
			{
				// Read from file skippable bytes
				if(ReadFile(File[f], pBuffer, nSize, &bytes, NULL)==0)
					RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
				if(bytes<nSize) FileEOF[f]=TRUE;
				GlobalFree(pBuffer);
				pBuffer=NULL;
			}
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void ReadDirBlock( int f, DWORD pFilename )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Find Directory to write
			char* DirString = (LPSTR)pFilename;

			// Store current directory
			char olddir[256];
			getcwd(olddir, 256);

			// If directory doesn't exist, create one
			char pNewDir[_MAX_PATH];
			strcpy(pNewDir, DirString);
			DWORD dwLength=strlen(pNewDir);
			if(pNewDir[dwLength-1]!='\\') { pNewDir[dwLength]='\\'; pNewDir[dwLength+1]=0; } 
			MakePathToThisFolder(pNewDir);
			_chdir(pNewDir);

			// Read number of files in dirblock
			DWORD bytes;
			DWORD NumberOfFiles=0;
			if(ReadFile(File[f], &NumberOfFiles, sizeof(NumberOfFiles), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			if(bytes==0) FileEOF[f]=TRUE;

			// Load all files in
			for(unsigned int n=0; n<NumberOfFiles; n++)
			{
				// Read size of filename
				int stringlength=0;
				if(ReadFile(File[f], &stringlength, sizeof(stringlength), &bytes, NULL)==0)
					RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
				if(bytes==0) FileEOF[f]=TRUE;
				if(stringlength>0)
				{
					// Read filename
					char* FilenameString = (char*)GlobalAlloc(GMEM_FIXED, stringlength);
					if(FilenameString)
					{
						if(ReadFile(File[f], FilenameString, stringlength, &bytes, NULL)==0)
							RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

						// Get path from filename (upto 8 nests)
						MakePathToThisFolder(FilenameString);

						// Read fileblock
						ReadFileBlockCore(FilenameString, f);

						// Release string
						GlobalFree(FilenameString);
						FilenameString=NULL;
					}
					else
					{
						RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
					}
				}
			}

			// Restore directory
			_chdir(olddir);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteByte( int f, int iValue )
{
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Write to file
			unsigned char data = (unsigned char)iValue;
			if(WriteFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteWord( int f, int iValue )
{
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Write to file
			WORD data = (WORD)iValue;
			if(WriteFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteLong( int f, int iValue )
{
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Write to file
			DWORD data = (DWORD)iValue;
			if(WriteFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteFloat( int f, float fValue )
{
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Write to file
			float data = fValue;
			if(WriteFile(File[f], &data, sizeof(data), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteString( int f, LPSTR pString )
{
    /*
        20091129 v75 - IRM - http://forum.thegamecreators.com/?m=forum_view&t=108603&b=15
        Copy of the input string to an internal buffer of fixed size (2k) caused buffer
        overruns into the stack and program crashes.

        Elimitated the buffer by writing from the original string (if set).
    */

    char carriage[3];
	carriage[0]=13;
	carriage[1]=10;
	carriage[2]=0;

	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
            // 20091129 v75 - IRM - Only write the string if it has been set
            if (pString)
            {
                LPSTR string = (char*)pString;
                DWORD stringlength=strlen(string);

                // 20091129 v75 - IRM - Only write the string if >0 bytes
                if (stringlength)
                {
                    // 20091129 v75 - IRM - Write directly from the input data, not using a secondary buffer
    			    if(WriteFile(File[f], string, stringlength, &bytes, NULL)==0)
	    			    RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
                }
            }

			// Write Carriage-Return to file
			if(WriteFile(File[f], carriage, 2, &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteData( int f, unsigned char* pData, unsigned int length )
{
    if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
            if (pData && length)
            {
				DWORD bytes;
                if( WriteFile(File[f], pData, length, &bytes, NULL ) == 0 )
	    		    RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);
            }
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteFileBlockCore( char* FilenameString, int f, int mode )
{
	DWORD bytes;
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Read fileblock from file
			DWORD bytesread;
			HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hreadfile!=INVALID_HANDLE_VALUE)
			{
				// Get Size of fileblock
				DWORD nSize = GetFileSize(hreadfile, NULL);

				// Create mem for it
				char* pBuffer = (char*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, nSize);
				if(pBuffer)
				{
					// Read the data into buffer
					ReadFile(hreadfile, pBuffer, nSize, &bytesread, NULL);

					// Close handle to file
					CloseHandle(hreadfile);		
					hreadfile=NULL;

					// Write it out to my file
					if(mode==1)
					{
						// Don't write size, cannot read back!
					}
					else
						WriteFile(File[f], &nSize, 4, &bytes, NULL);

					WriteFile(File[f], pBuffer, nSize, &bytes, NULL);

					// Free buffer
					GlobalFree(pBuffer);
					pBuffer=NULL;
				}
			}
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteFileBlock( int f, DWORD pFilename )
{
	char* FilenameString = (LPSTR)pFilename;
	WriteFileBlockCore(FilenameString, f, 0);
}

DARKSDK void WriteFileBlock( int f, DWORD pFilename, int iFlag )
{
	char* FilenameString = (LPSTR)pFilename;
	WriteFileBlockCore(FilenameString, f, 1);
}

DARKSDK void WriteDirContents(int f, char* newdir, bool bMode, DWORD* pCount, char* relativedir)
{
	// Remember current dir
	char olddir[256];
	getcwd(olddir,256);

	// Switch to new dir
	_chdir(newdir);

	// Go through dir and write out all files
	int res=-1;
	struct _finddata_t filedata;
	long hLocalFile = _findfirst("*.*", &filedata);
	if(strcmp(filedata.name,".")==0) res=_findnext(hLocalFile, &filedata);
	if(strcmp(filedata.name,"..")==0) res=_findnext(hLocalFile, &filedata);
	while(res!=-1L)
	{
		if(FGetActualTypeValue(filedata.attrib)==1)
		{
			char thisrelativedir[256];
			strcpy(thisrelativedir, relativedir);
			strcat(thisrelativedir, filedata.name);
			strcat(thisrelativedir, "\\");
			WriteDirContents(f, filedata.name, bMode, pCount, thisrelativedir);
		}
		else
		{
			if(bMode)
			{
				// Get size of filename
				DWORD bytes;
				char string[256];
				strcpy(string, relativedir);
				strcat(string, filedata.name);
				DWORD stringlength=strlen(string)+1;

				// Write size of filename first
				if(WriteFile(File[f], &stringlength, sizeof(stringlength), &bytes, NULL)==0)
					RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);

				// Write filename string second
				if(WriteFile(File[f], string, stringlength, &bytes, NULL)==0)
					RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);

				// Write actual fileblock
				WriteFileBlockCore(filedata.name, f, 0);
			}
			else
			{
				int inc = *(pCount);
				*(pCount)=inc+1;
			}
		}
		res=_findnext(hLocalFile, &filedata);
	}
	_findclose(hLocalFile);

	// Restore old dir
	_chdir(olddir);
}

DARKSDK void WriteDirBlock( int f, DWORD pFilename )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			// Find Directory to write
			char* DirString = (LPSTR)pFilename;

			// Count files in dirblock
			DWORD Count=0;
			char RelativeDir[256];
			strcpy(RelativeDir,"");
			WriteDirContents(f, DirString, false, &Count, RelativeDir);

			// Write Header to DirBlock
			DWORD bytes;
			if(WriteFile(File[f], &Count, sizeof(Count), &bytes, NULL)==0)
				RunTimeWarning(RUNTIMEERROR_CANNOTWRITETOFILE);

			// Write all files in dir
			strcpy(RelativeDir,"");
			WriteDirContents(f, DirString, true, &Count, RelativeDir);
		}
		else
			RunTimeWarning(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void ReadMemblock( int f, int mbi )
{
	// mike - 011005 - quit if invalid pointer
	/*if ( !ExtMakeMemblock )
		return;*/

	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			if(mbi>=1 && mbi<=255)
			{
				// Get Size of MEMBLOCK
				DWORD bytes, size;
				if(ReadFile(File[f], &size, 4, &bytes, NULL)!=0)
				{
					// Create memblock memory
					LPSTR pMem = ExtMakeMemblock ( mbi, size );
					if(pMem)
					{
						// Gey Data of MEMBLOCK from FILE
						if(ReadFile(File[f], pMem, size, &bytes, NULL)==0)
							RunTimeError(RUNTIMEERROR_CANNOTREADFROMFILE);
					}
					else
						RunTimeError(RUNTIMEERROR_MEMBLOCKCREATIONFAILED);
				}
				else
					RunTimeError(RUNTIMEERROR_CANNOTREADFROMFILE);
			}
			else
				RunTimeError(RUNTIMEERROR_MEMBLOCKRANGEILLEGAL);
		}
		else
			RunTimeError(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeError(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void MakeMemblockFromFile( int mbi, int f )
{
	// mike - 011005 - quit if invalid pointer
/*	if ( !ExtMakeMemblock )
		return;*/
		
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			if(mbi>=1 && mbi<=255)
			{
				// Get file size
				DWORD dwSize = GetFileSize(File[f], NULL);
				if(dwSize>0)
				{
					// Get Size of MEMBLOCK
					DWORD bytes;

					// Create memblock memory
					LPSTR pMem = ExtMakeMemblock ( mbi, dwSize );
					if(pMem)
					{
						// Gey Data of MEMBLOCK from FILE
						if(ReadFile(File[f], pMem, dwSize, &bytes, NULL)==0)
							RunTimeError(RUNTIMEERROR_CANNOTREADFROMFILE);
					}
					else
						RunTimeError(RUNTIMEERROR_MEMBLOCKCREATIONFAILED);
				}
				else
					RunTimeError(RUNTIMEERROR_CANNOTREADFROMFILE);
			}
			else
				RunTimeError(RUNTIMEERROR_MEMBLOCKRANGEILLEGAL);
		}
		else
			RunTimeError(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeError(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void MakeFileFromMemblock( int f, int mbi )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			if(mbi>=1 && mbi<=255)
			{
				LPSTR pPtr = ExtGetMemblockPtr ( mbi );
				if(pPtr)
				{
					// Write it to file (difference from WRIITE MEMBLOCK is that no size DWORD is written = pure file)
					DWORD bytes;
					DWORD dwSize = ExtGetMemblockSize ( mbi );

					// Write MEMBLOCK to FILE
					if(WriteFile(File[f], pPtr, dwSize, &bytes, NULL)==0)
						RunTimeError(RUNTIMEERROR_CANNOTWRITETOFILE);
				}
				else
					RunTimeError(RUNTIMEERROR_MEMBLOCKNOTEXIST);
			}
			else
				RunTimeError(RUNTIMEERROR_MEMBLOCKRANGEILLEGAL);
		}
		else
			RunTimeError(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeError(RUNTIMEERROR_FILENUMBERINVALID);
}

DARKSDK void WriteMemblock( int f, int mbi )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
		{
			if(mbi>=1 && mbi<=255)
			{
				LPSTR pPtr = ExtGetMemblockPtr ( mbi );
				if(pPtr)
				{
					// Write MEMBLOCK Size to FILE
					DWORD dwSize = ExtGetMemblockSize ( mbi );
					DWORD bytes;
					if(WriteFile(File[f], &dwSize, 4, &bytes, NULL)!=0)
					{
						// Write MEMBLOCK to FILE
						if(WriteFile(File[f], pPtr, dwSize, &bytes, NULL)==0)
							RunTimeError(RUNTIMEERROR_CANNOTWRITETOFILE);
					}
					else
						RunTimeError(RUNTIMEERROR_CANNOTWRITETOFILE);
				}
				else
					RunTimeError(RUNTIMEERROR_MEMBLOCKNOTEXIST);
			}
			else
				RunTimeError(RUNTIMEERROR_MEMBLOCKRANGEILLEGAL);
		}
		else
			RunTimeError(RUNTIMEERROR_FILENOTOPEN);
	}
	else
		RunTimeError(RUNTIMEERROR_FILENUMBERINVALID);
}

//		
// Command Expression Functions
//
//PE: GetDir calls never free memory, we use it alot.
//PE: We nearly newer use LPSTR pThisDir = GetDir(); , but more cstr and std::string where the pointer is lost.
//PE: So make a small ring buffer.

int currentRingDir = 0;
char cRingDir[20][1024];
DARKSDK LPSTR GetDir(void)
{
	// Create and return string
	currentRingDir = currentRingDir + 1;
	if (currentRingDir > 19)
		currentRingDir = 0;
	getcwd(cRingDir[currentRingDir], 1024);
	LPSTR pReturnString = &cRingDir[currentRingDir][0];
	return pReturnString;
}

DARKSDK LPSTR GetDirOLD( void )
{
	// Create and return string
	getcwd(m_pWorkString, 1024);
	LPSTR pReturnString=GetReturnStringFromWorkString();
	return pReturnString;
}

//PE: Mem never freed normally use cstr tmp = GetFileName();
int ringCounter = 0;
char FilenameRingBuffer[20][MAX_PATH];
DARKSDK LPSTR GetFileName( void )
{
	ringCounter = ringCounter + 1;
	if (ringCounter > 19)
		ringCounter = 0;
	if(hInternalFile)
		strcpy(&FilenameRingBuffer[ringCounter][0], filedata.name);
	else
		strcpy(&FilenameRingBuffer[ringCounter][0], "");

	LPSTR pReturnString = &FilenameRingBuffer[ringCounter][0];
	return pReturnString;
}

DARKSDK LPSTR GetFileNameOLD(void)
{
	if (hInternalFile)
		strcpy(m_pWorkString, filedata.name);
	else
		strcpy(m_pWorkString, "");

	LPSTR pReturnString = GetReturnStringFromWorkString();
	return pReturnString;
}

DARKSDK int GetFileType( void )
{
	if(FGetFileReturnValue()==-1L || hInternalFile==NULL)
		return -1;
	else
		return FGetActualTypeValue(filedata.attrib);
}


DARKSDK LPSTR GetFileDate(void)
{
	if (hInternalFile)
		wsprintf(m_pWorkString, "%.24s", ctime(&(filedata.time_write)));
	else
		strcpy(m_pWorkString, "");

	LPSTR pReturnString = GetReturnStringFromWorkString();
	return pReturnString;
}

DARKSDK long GetFileDateLong( void )
{
	if(hInternalFile)
		return( (long) filedata.time_write);
	else
		return(0);
}

DARKSDK LPSTR GetFileCreation( void )
{
	if(hInternalFile)
		wsprintf(m_pWorkString, "%.24s", ctime( &( filedata.time_create)));
	else
		strcpy(m_pWorkString, "");

	LPSTR pReturnString=GetReturnStringFromWorkString();
	return pReturnString;
}

DARKSDK int FileExist( LPSTR pFilename )
{
	if(DB_FileExist(pFilename))
		return 1;
	else
		return 0;
}

DARKSDK int FileExistPrefDDS(LPSTR pFilename)
{
	if (DB_FileExist(pFilename))
		return 1;
	else
	{
		// additional check for DDS alternative (used in standalone post-optimization code)
		int iLen = strlen(pFilename);
		if (iLen > 4)
		{
			char pDDSCheck[MAX_PATH];
			strcpy(pDDSCheck, pFilename);
			pDDSCheck[iLen - 3] = 'd';
			pDDSCheck[iLen - 2] = 'd';
			pDDSCheck[iLen - 1] = 's';
			if (DB_FileExist(pDDSCheck))
				return 1;
		}
		return 0;
	}
}

DARKSDK int FileSize( LPSTR pFilename )
{
	return DB_FileSize(pFilename);
}

DARKSDK int PathExist( LPSTR pFilename )
{
	if(DB_PathExist(pFilename))
		return 1;
	else
		return 0;
}

DARKSDK int FileOpen( int f )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(File[f]!=NULL)
			return 1;
		else
			return 0;
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

	return 0;
}

DARKSDK int FileEnd( int f )
{
	if(f>=1 && f<=MAX_FILES)
	{
		if(FileEOF[f]==TRUE)
			return 1;
		else
			return 0;
	}
	else
		RunTimeWarning(RUNTIMEERROR_FILENUMBERINVALID);

	return 0;
}

//PE: g_pGlob->CreateDeleteString never freed, always use same string
char cAppName[1025];

DARKSDK LPSTR Appname( void )
{
	#ifdef PRODUCTCONVERTER
	#else
	extern bool bSpecialStandalone;
	extern char cSpecialStandaloneProject[MAX_PATH];
	if (bSpecialStandalone)
	{
		//PE: Emulate normal standalone name.
		strcpy(cAppName, cSpecialStandaloneProject);
		strcat(cAppName, ".exe");
		LPSTR pReturnString = &cAppName[0];
		return pReturnString;
	}
	#endif
	// Create and return string
	GetModuleFileName(g_pGlob->hInstance, cAppName, 1024);
	LPSTR pReturnString = &cAppName[0];
	return pReturnString;
}

DARKSDK LPSTR AppnameOLD(void)
{
#ifdef PRODUCTCONVERTER
#else
	extern bool bSpecialStandalone;
	extern char cSpecialStandaloneProject[MAX_PATH];
	if (bSpecialStandalone)
	{
		//PE: Emulate normal standalone name.
		strcpy(m_pWorkString, cSpecialStandaloneProject);
		strcat(m_pWorkString, ".exe");
		LPSTR pReturnString = GetReturnStringFromWorkString();
		return pReturnString;
	}
#endif
	// Create and return string
	GetModuleFileName(g_pGlob->hInstance, m_pWorkString, 1024);
	LPSTR pReturnString = GetReturnStringFromWorkString();
	return pReturnString;
}

DARKSDK char* Windir( char* pDestStr )
{
	// Create and return string
	GetWindowsDirectory(m_pWorkString, 1024);	
	LPSTR pReturnString=GetReturnStringFromWorkString();
	return pReturnString;
}

//PE: Never freed.
char docdir[MAX_PATH];
DARKSDK LPSTR Mydocdir( void )
{
	// lee - 040407 - return the My Documents folder in full
	SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, 0, docdir);
	LPSTR pReturnString = &docdir[0];
	return pReturnString;
}

DARKSDK int CollectFilesWithExtension(char* extension, char* directory, std::vector<std::string>* result)
{
	if (result == nullptr)
	{
		return 0;
	}

	std::string oldDir = GetDir();
	int extLength = strlen(extension);
	if (PathExist(directory) == 1)
	{
		SetDir(directory);
		FindFirst();
		int lastFile = 0;
		while (GetFileType() != -1)
		{
			if (GetFileType() == 1 && strlen(GetFileName()) > 2)
			{
				// Folder
				char newDir[MAX_PATH];
				strcpy(newDir, directory);
				char lastChar = newDir[strlen(newDir) - 1];
				if (lastChar != '\\' && lastChar != '/')
				{
					strcat(newDir, "\\");
				}
				strcat(newDir, GetFileName());
				CollectFilesWithExtension(extension, newDir, result);
				// Since directory was changed, now need to get to the last file before the subfolder
				FindFirst();
				if (lastFile > 0)
				{
					for (int i = 1; i <= lastFile; i++)
					{
						if (GetFileType() > -1)
						{
							FindNext();
						}
					}
				}
			}
			else
			{
				// File
				LPSTR pFilename = GetFileName();
				if (strlen(pFilename) >= extLength && strnicmp(pFilename + strlen(pFilename) - extLength, extension, extLength) == NULL)
				{
					std::string name = directory;
					char lastChar = directory[strlen(directory) - 1];
					if (lastChar != '\\' && lastChar != '/')
					{
						name.append("\\");
					}
					name.append(pFilename);
					result->push_back(name);
				}
			}
			FindNext();
			lastFile++;
		}
	}
	SetDir((char*)oldDir.c_str());
	return 1;
}
