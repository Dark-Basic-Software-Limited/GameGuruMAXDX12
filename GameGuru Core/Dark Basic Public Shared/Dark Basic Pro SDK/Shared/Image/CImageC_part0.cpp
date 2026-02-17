// INCLUDES / LIBS ///////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include <DDSTextureLoader.h>
#include "DirectXTex.h"
#include "wincodec.h"

#include "cimagec.h"
#include ".\..\error\cerror.h"
#include "globstruct.h"
#include <stdio.h>
#include <direct.h>
#include <vector>
#include <string>
#include <map>
#include ".\..\Core\SteamCheckForWorkshop.h"

#include "CSpritesC.h"
#include "CMemblocks.h"
#include "CObjectsC.h"
#include "DarkLUA.h"
#include <thread>
#include ".\..\Core\DBDLLCore.h"

extern "C" HANDLE GG_CreateFile( LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
extern "C" int GG_GetRealPath( char* fullPath, int create, bool bIgnoreAdditional = false);

#define PETESTIMAGEUSAGE

// Forward declclaration of timestampactivity since it is housed elsewhere
#ifndef NOSTEAMORVIDEO
void timestampactivity(int i, char* desc_s);
#endif

// Externs
extern GlobStruct*				g_pGlob;
extern LPGG						m_pDX;
extern LPGGDEVICE				m_pD3D;

// globals for single thread (THREAD-SAFE)
std::thread* g_pT1 = NULL;
bool g_bT1 = false;
bool g_bRequestCleanInteruptionT1 = false;
std::vector<sPreLoadedTexture> g_image_list;
std::vector<sPreLoadedTexture> g_image_outputv;
int iResizeLoadImageX = 0, iResizeLoadImageY = 0;

bool g_bCriticalSectionCreated = false;
CRITICAL_SECTION CriticalSection; 

#ifdef WICKEDENGINE
// used to fool image system into thinking it has successfully loaded an image
LPVOID g_pDummyImage = NULL;
GGIMAGE_INFO g_pDummyInfo;
#endif
bool g_bAllowLegacyImageLoadingForUI = false;
void image_setlegacyimageloading(bool bEnable) { g_bAllowLegacyImageLoadingForUI = bEnable; }
bool g_bUseRGBAFormat = false;
bool g_bDontUseImageAlpha = false;
//PE: Silent 502 , log instead.
int iMaxPasteImageLogs = 10;

// function to execute thread code
void image_thread_function(const std::vector<sPreLoadedTexture> &v)
{
	// in this thread, load each DX11 texture in turn
	g_bT1 = true;
	g_image_outputv.clear();
	for ( int n = 0; n < v.size(); n++ )
	{
		// Request ownership of the critical section.
		EnterCriticalSection(&CriticalSection); 
		sPreLoadedTexture item = v[n];
		if ( item.pTexture == NULL ) item.pTexture = PreloadThreadSafeImage ( item.pFilename , item.iMipMaps );
		g_image_outputv.push_back(item);
		if (g_bRequestCleanInteruptionT1 == true)
		{
			// can interupt this load thread if required
			LeaveCriticalSection(&CriticalSection);
			break;
		}
		// Release ownership of the critical section.
		LeaveCriticalSection(&CriticalSection);
	}
	g_bT1 = false;
}

void image_preload_files_start ( void )
{
	// Initialize the critical section one time only.
	if (g_bCriticalSectionCreated == false)
	{
		if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x00000400) ) 
			return;

		// we can use this to ensure only one thing done at once
		g_bCriticalSectionCreated = true;
	}

	// ensure previous thread has ended
	image_preload_files_strictwaittoend();

	// get number of CPUs we can run threads on, in case we need it
	unsigned int iCPUCores = std::thread::hardware_concurrency();
	char CPUnium[32];
	sprintf ( CPUnium, "%d", iCPUCores );

	// load texture file into member well ahead of needing it (see below for getting the preloaded textures)
	g_image_list.clear();
}

void image_preload_files_add ( LPSTR pFilename , int iMipMaps)
{
	//Moved here so we can check if its already in the list.
	char *cUseFilename;
	char cResolvePath[MAX_PATH];
	//Resolve path , if some function later in code change current dir.
	if (GetFullPathNameA(pFilename, MAX_PATH, &cResolvePath[0], NULL) > 0) {
		cUseFilename = &cResolvePath[0];
	}
	else {
		cUseFilename = pFilename;
	}

	// check to make sure we don't add something we already have in the list
	for ( int n = 0; n < g_image_list.size(); n++ )
		if ( stricmp ( g_image_list[n].pFilename, cUseFilename) == NULL )
			return;

	// add item to list of work
	sPreLoadedTexture item;
	item.pTexture = NULL;
	item.iMipMaps = iMipMaps;
	strcpy(item.pFilename, cUseFilename);
	g_image_list.push_back(item);
}

void image_preload_files_finish ( void )
{
	// before send list to thread, load up list with previous preloaded file 'data' still in memory
	for ( int n = 0; n < g_image_list.size(); n++ )
	{
		if ( n < g_image_outputv.size() )
		{
			if ( stricmp ( g_image_list[n].pFilename, g_image_outputv[n].pFilename ) == NULL )
			{
				// this ensures we avoid reloading something we 'might need' that we have already preloaded previously
				g_image_list[n].pTexture = g_image_outputv[n].pTexture;
			}
		}
	}

	// start preloading
	g_bRequestCleanInteruptionT1 = false; //Start fresh.
	g_pT1 = new std::thread(image_thread_function, std::ref(g_image_list));
}

void image_preload_files_strictwaittoend ( void )
{
	// wait for all work to finish
	if ( g_pT1 )
	{
		g_pT1->join();
		delete g_pT1;
		g_pT1 = NULL;
		g_bRequestCleanInteruptionT1 = false;
	}
}

void image_preload_files_wait ( void )
{
	g_bRequestCleanInteruptionT1 = true;
	image_preload_files_strictwaittoend();
}

bool image_preload_files_in_progress ( void )
{
	// wait for all work to finish
	return g_bT1;
}

void image_preload_files_reset ( void )
{
	// clear finished list for next batch of work
	for ( int n = 0; n < g_image_outputv.size(); n++ )
	{
		if ( g_image_outputv[n].pTexture ) 
		{
			g_image_outputv[n].pTexture->Release();
			g_image_outputv[n].pTexture = NULL;
		}
	}
	g_image_outputv.clear();
}

#ifdef WICKEDENGINE
extern std::vector<ID3D11ShaderResourceView*> lpBadTexture;
#endif

// Forward declaration for DX12 texture cache eviction (defined in imgui_gg_dx12_bridge.cpp)
extern void ImGui_DX12_RemoveTexture(int imageId);

namespace
{
    typedef std::map<int, tagImgData*>		ImageList_t;
    typedef ImageList_t::iterator			ImagePtr;

    // Image Block Globals

    bool								g_bImageBlockActive = false;
    LPSTR								g_iImageBlockFilename = NULL;
    LPSTR								g_iImageBlockRootPath = NULL;
    char								g_pImageBlockExcludePath[512];
    int									g_iImageBlockMode = -1;
    DWORD								g_dwImageBlockSize = 0;
    LPSTR								g_pImageBlockPtr = NULL;
    std::vector<LPSTR>					g_ImageBlockListFile;
    std::vector<DWORD>					g_ImageBlockListOffset;
    std::vector<DWORD>					g_ImageBlockListSize;

    ImageList_t							m_List;
    int									m_iWidth		= 0;				// width of current texture
    int									m_iHeight		= 0;				// height of current texture
    int									m_iMipMapNum	= -1;				// default number of mipmaps
    int									m_iMemory		= 0;				// default memory pool
    bool								m_bSharing		= true;				// sharing flag
    bool								m_bMipMap		= true;				// mipmap on / off
    GGCOLOR								m_Color         = GGCOLOR_ARGB ( 255, 0, 0, 0 );// default transparent color
    tagImgData*							m_imgptr = NULL;
    int									m_CurrentId = 0;
	#ifdef WICKEDENGINE
    GGFORMAT							g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	#else
    GGFORMAT							g_DefaultGGFORMAT;
	#endif
	DWORD								g_dwMipMapGenMode;

    bool RemoveImage( int iID )
    {
        // Clear the cached value if the image being deleted is the current cached image.
        if (m_CurrentId == iID)
        {
            m_CurrentId = 0;
            m_imgptr = NULL;
        }

        // Locate the image, and if found, release all of it's resources.
        ImagePtr pImage = m_List.find( iID );
        if (pImage != m_List.end())
        {
			#ifdef DX11
			#ifdef WICKEDENGINE
			//PE: Mark bad textures that have been deleted.
			lpBadTexture.push_back(pImage->second->lpTextureView);

			// release all 'except' any dummy view if active
			bool bIsDummy = false;

			if (g_pDummyImage)
			{
				if (g_pDummyImage == pImage->second->lpTexture)
				{
					// do not attempt to delete this, we need it to remain alive for all other images that use this dummy
					bIsDummy = true;
				}
				else
		            SAFE_RELEASE( pImage->second->lpTextureView );
			}
			else
	            SAFE_RELEASE( pImage->second->lpTextureView );

			//PE: @Lee we are leaking memory, not sure why DX11 dont free the texture ?
			//PE: @Lee I dont know the reason so im using the g_bAllowLegacyImageLoadingForUI so i can control if texture should be released.
			//PE: @Lee this could also be a problem for Classic not releasing the texture, should always release it ? test must wait until we do classic fixes :)
			//PE: Mainly you see this if you quickly hover over thumbs that use backdrops, you can use many many gb in less then a minute.
			//PE: Also dynamic loading of new thumbs and the old icons also leaked.
			//PE: Use same legacy flag to see if we can delete the image (for now).
			if (!bIsDummy && g_bAllowLegacyImageLoadingForUI)
			{
				SAFE_RELEASE(pImage->second->lpTexture);
			}

			#else
#ifdef WICKEDENGINE
			SAFE_RELEASE( pImage->second->lpTextureView );
#endif
			#endif
			#else
            SAFE_RELEASE( pImage->second->lpTexture );
			#endif
            SAFE_DELETE( pImage->second->lpName );
            delete pImage->second;

            // Evict from DX12 texture cache so replacement images at the same ID get loaded
            ::ImGui_DX12_RemoveTexture(iID);

            m_List.erase(pImage);

            return true;
        }

        return false;
    }

    bool UpdatePtrImage ( int iID )
    {
        // If the image required is not already cached, refresh the cached value
        if (!m_imgptr || iID != m_CurrentId)
        {
            m_CurrentId = iID;

            ImagePtr p = m_List.find( iID );
            if (p == m_List.end())
            {
                m_imgptr = NULL;
            }
            else
            {
                m_imgptr = p->second;
            }
        }

        return m_imgptr != NULL;
    }
}

DARKSDK int GetPowerSquareOfSize( int Size );

DARKSDK void ImageConstructorD3D ( void )
{
	// setup the image library
	//m_iMipMapNum			= 9; //Default was used in thread loader should be -1.
	m_iMipMapNum = -1;
}

DARKSDK void ImageConstructor ( void )
{
	ImageConstructorD3D ( );
	ImagePassCoreData ( NULL );
}


int getBitsPerPixel(int fmt);
std::string getImageformat(int fmt);

int DumpImageListCount = 1;

//Clear all "entitybank" memory used.


DARKSDK void DumpImageList(void)
{
#ifdef PETESTIMAGEUSAGE
	char timestampMsg[1024];
	long imageuse = 0;
	long notused = 0;
	long notusedsize = 0;
	long usedsize = 0;
	long used = 0;
	long totalmipmaps = 0;
	long savedbybaking = 0;
	long minusmem = 0;
	long GPUcachedimagesSize = 0;

	int TotalLoadTime = 0;

	for (ImagePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagImgData* ptr = pCheck->second;


		GGSURFACE_DESC imgdesc;

		LPGGSURFACE pTextureInterface = NULL;
		ptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);

		pTextureInterface->GetDesc(&imgdesc);
		SAFE_RELEASE ( pTextureInterface );

		//BC1: 5:6 : 5 color(5 bits red, 6 bits green, 5 bits blue).This is true even if the data also contains 1 - bit alpha.
		//Assuming a 4×4 texture using the largest data format possible, the BC1 format reduces the memory required from 48 bytes(16 colors × 3 components / color × 1 byte / component) to 8 bytes of memory.
		//So to calculate all 4 bits per pixels should be devided by 8.
		//BC1: 48 bytes (16 colors × 3 components/color × 1 byte/component) to 8 bytes of memory.
		//BC2: 64 bytes (16 colors × 4 components/color × 1 byte/component) to 16 bytes of memory.
		//BC3: 64 bytes (16 colors × 4 components/color × 1 byte/component) to 16 bytes of memory.
		//BC1 / 8.0 = * 0,125 (-alpha 4bit) (4 bits per pixel )
		//BC2 / 4.0 = * 0.25 (8 bits per pixel )
		//BC3 / 4.0 = * 0.25 (8 bits per pixel )
		//So: Width*Height*4(RGBA)*(getBitsPerPixel/32.0)
		//Or: Width*Height*(getBitsPerPixel/8.0)
		//Substract BC1 alpha. = 0.5/4 = (-0.125)

		float bperpixel = (float)getBitsPerPixel(imgdesc.Format) / 8;
		if (bperpixel == 0.5 ) {
			bperpixel -= 0.125; // remove alpha count from BC1
		}

		//in kb , so dont need to be so precise just use int.
		int addmipmapssize;
		
		if (imgdesc.MipLevels > 1) {
			addmipmapssize = (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize - 1; // Full mipmaps always give size -1.
			if (addmipmapssize <= 0) addmipmapssize = 0;
		}
		else {
			addmipmapssize = 0;
		}
		totalmipmaps += addmipmapssize;


		if( strlen(ptr->szShortFilename) > 0 )
			sprintf(timestampMsg, "DumpImgList%d(%d,%d): %s (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int) pCheck->first , ptr->iImageLoadTime, ptr->szShortFilename, ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int) ( (float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize , imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format).c_str());
		else
			sprintf(timestampMsg, "DumpImgList%d(%d): unknown (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int)pCheck->first , ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize, imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format).c_str());

		imageuse += ((int) (((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;  //PE: Real mem use.

		if ((int)pCheck->first < 0) {
			minusmem += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize; 
		}

		//if (ptr->AccessCountCPU <= 1 && ptr->AccessCountGPU <= 1) {
		if (ptr->AccessCountCPU == 0 && ptr->AccessCountGPU == 0 ) { //
			notused++;
			notusedsize += ((int) (((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
		}
		else {
			used++;
			usedsize += ((int) (((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
		}

		if (ptr->AccessCountCPU >= 1 && ptr->AccessCountGPU >= 1000) {
			//PE: Estimate that 1000 accesses will make the GPU cache the image. ( not releasing it ).
			//PE: If this get larger then 2 gb ( depends on GPU ) we will get stuttering.
			//PE: todo - reset GPU access between levels for standalone.
			GPUcachedimagesSize += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize; //PE: Real mem use.
		}
		TotalLoadTime += ptr->iImageLoadTime;
		#ifndef NOSTEAMORVIDEO
		timestampactivity(0, timestampMsg);
		#endif
	}
	//PE: until GPU access is reset , dont display GPUcachedimagesSize as this will only work for the first level you load and can confuse, now that you know this you can enable this line :)
	//sprintf(timestampMsg, "DumpImageList: Total mem=%ld kb. mipmaps=%ld kb. Used Images: %ld mem %ld kb. - Notused Images: %ld mem %ld kb. - MinusMem: %ld kb. (GPUcachedimagesSize : %ld)", imageuse, totalmipmaps, used, usedsize, notused, notusedsize, minusmem, GPUcachedimagesSize);
	sprintf(timestampMsg, "DumpImageList: LoadTime: %d Total mem=%ld kb. mipmaps=%ld kb. Used Images: %ld mem %ld kb. - Notused Images: %ld mem %ld kb. - MinusMem: %ld kb.", TotalLoadTime , imageuse, totalmipmaps, used, usedsize, notused, notusedsize, minusmem);
	#ifndef NOSTEAMORVIDEO
	timestampactivity(0, timestampMsg);
	#ifndef WICKEDENGINE
	//PE: not used not for wicked, we cant see GPU accesses.
	timestampactivity(0, "NOTUSED");
	#endif
	#endif
	#ifndef WICKEDENGINE
	int totalNUtime = 0;
	for (ImagePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagImgData* ptr = pCheck->second;

		GGSURFACE_DESC imgdesc;

		LPGGSURFACE pTextureInterface = NULL;
		ptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);

		pTextureInterface->GetDesc(&imgdesc);
		SAFE_RELEASE(pTextureInterface);

		float bperpixel = (float)getBitsPerPixel(imgdesc.Format) / 8;
		if (bperpixel == 0.5) {
			bperpixel -= 0.125; // remove alpha count from BC1
		}

		//in kb , so dont need to be so precise just use int.
		int addmipmapssize;

		if (imgdesc.MipLevels > 1) {
			addmipmapssize = (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize - 1; // Full mipmaps always give size -1.
			if (addmipmapssize <= 0) addmipmapssize = 0;
		}
		else {
			addmipmapssize = 0;
		}
		totalmipmaps += addmipmapssize;


//		if (strlen(ptr->szShortFilename) > 0)
//			sprintf(timestampMsg, "NU%d(%d): %s (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int)pCheck->first, ptr->szShortFilename, ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize, imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format));
//		else
//			sprintf(timestampMsg, "NU%d(%d): unknown (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int)pCheck->first, ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize, imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format));
		if (strlen(ptr->szShortFilename) > 0) {
			sprintf(timestampMsg, "NU: %s (%d)", ptr->szShortFilename, ptr->iImageLoadTime);
		}

		imageuse += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;  //PE: Real mem use.

		if ((int)pCheck->first < 0) {
			minusmem += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
		}

		//if (ptr->AccessCountCPU <= 1 && ptr->AccessCountGPU <= 1) {
		if (ptr->AccessCountCPU == 0 && ptr->AccessCountGPU == 0 && strlen(ptr->szShortFilename) > 0 ) { //
			notused++;
			notusedsize += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
			totalNUtime += ptr->iImageLoadTime;
			#ifndef NOSTEAMORVIDEO
			timestampactivity(0, timestampMsg);
			#endif
		}

	}

	#ifndef NOSTEAMORVIDEO
	sprintf(timestampMsg, "NUTIME: %d", totalNUtime);
	timestampactivity(0, timestampMsg);
	#endif

	#endif

	#ifdef WICKEDENGINE
	void Wicked_Memory_Use_Textures(void);
	Wicked_Memory_Use_Textures();
	#endif

#endif
}


std::string getImageformat(int fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return std::string("DXGI_FORMAT_R32G32B32A32_128B");

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return std::string("DXGI_FORMAT_R32G32B32_96B");

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return std::string("DXGI_FORMAT_64B");

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		return std::string("DXGI_FORMAT_R10G10B10A2_TYPELESS_32B");
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return std::string("DXGI_FORMAT_R10G10B10A2_UNORM_32B");
	case DXGI_FORMAT_R10G10B10A2_UINT:
		return std::string("DXGI_FORMAT_R10G10B10A2_UINT_32B");
	case DXGI_FORMAT_R11G11B10_FLOAT:
		return std::string("DXGI_FORMAT_R11G11B10_FLOAT_32B");
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return std::string("DXGI_FORMAT_R8G8B8A8_TYPELESS_32B");
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return std::string("DXGI_FORMAT_R8G8B8A8_UNORM_32B");
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return std::string("DXGI_FORMAT_R8G8B8A8_UNORM_SRGB_32B");
	case DXGI_FORMAT_R8G8B8A8_UINT:
		return std::string("DXGI_FORMAT_R8G8B8A8_UINT_32B");
	case DXGI_FORMAT_R8G8B8A8_SNORM:
		return std::string("DXGI_FORMAT_R8G8B8A8_SNORM_32B");
	case DXGI_FORMAT_R8G8B8A8_SINT:
		return std::string("DXGI_FORMAT_R8G8B8A8_SINT_32B");
	case DXGI_FORMAT_R16G16_TYPELESS:
		return std::string("DXGI_FORMAT_R16G16_TYPELESS_32B");
	case DXGI_FORMAT_R16G16_FLOAT:
		return std::string("DXGI_FORMAT_R16G16_FLOAT_32B");
	case DXGI_FORMAT_R16G16_UNORM:
		return std::string("DXGI_FORMAT_R16G16_UNORM_32B");
	case DXGI_FORMAT_R16G16_UINT:
		return std::string("DXGI_FORMAT_R16G16_UINT_32B");
	case DXGI_FORMAT_R16G16_SNORM:
		return std::string("DXGI_FORMAT_R16G16_SNORM_32B");
	case DXGI_FORMAT_R16G16_SINT:
		return std::string("DXGI_FORMAT_R16G16_SINT_32B");
	case DXGI_FORMAT_R32_TYPELESS:
		return std::string("DXGI_FORMAT_R32_TYPELESS_32B");
	case DXGI_FORMAT_D32_FLOAT:
		return std::string("DXGI_FORMAT_D32_FLOAT_32B");
	case DXGI_FORMAT_R32_FLOAT:
		return std::string("DXGI_FORMAT_R32_FLOAT_32B");
	case DXGI_FORMAT_R32_UINT:
		return std::string("DXGI_FORMAT_R32_UINT_32B");
	case DXGI_FORMAT_R32_SINT:
		return std::string("DXGI_FORMAT_R32_SINT_32B");
	case DXGI_FORMAT_R24G8_TYPELESS:
		return std::string("DXGI_FORMAT_R24G8_TYPELESS_32B");
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return std::string("DXGI_FORMAT_D24_UNORM_S8_UINT_32B");
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		return std::string("DXGI_FORMAT_R24_UNORM_X8_TYPELESS_32B");
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return std::string("DXGI_FORMAT_X24_TYPELESS_G8_UINT_32B");
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		return std::string("DXGI_FORMAT_R9G9B9E5_SHAREDEXP_32B");
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
		return std::string("DXGI_FORMAT_R8G8_B8G8_UNORM_32B");
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
		return std::string("DXGI_FORMAT_G8R8_G8B8_UNORM_32B");
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return std::string("DXGI_FORMAT_B8G8R8A8_UNORM_32B");
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return std::string("DXGI_FORMAT_B8G8R8X8_UNORM_32B");
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		return std::string("DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM_32B");
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return std::string("DXGI_FORMAT_B8G8R8A8_TYPELESS_32B");
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return std::string("DXGI_FORMAT_B8G8R8A8_UNORM_SRGB_32B");
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		return std::string("DXGI_FORMAT_B8G8R8X8_TYPELESS_32B");
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return std::string("DXGI_FORMAT_B8G8R8X8_UNORM_SRGB_32B");
	case DXGI_FORMAT_AYUV:
		return std::string("DXGI_FORMAT_AYUV_32B");
	case DXGI_FORMAT_Y410:
		return std::string("DXGI_FORMAT_Y410_32B");
	case DXGI_FORMAT_YUY2:
		return std::string("DXGI_FORMAT_YUY2_32B");

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		return std::string("DXGI_FORMAT_P0_24B");

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return std::string("DXGI_FORMAT_R8G8_16B");

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
		return std::string("DXGI_FORMAT_NV_12B");

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
		return std::string("DXGI_FORMAT_R8_8B");

	case DXGI_FORMAT_R1_UNORM:
		return std::string("DXGI_FORMAT_R1_UNORM_1B");

	case DXGI_FORMAT_BC1_TYPELESS:
		return std::string("DXGI_FORMAT_BC1_TYPELESS_4B");
	case DXGI_FORMAT_BC1_UNORM:
		return std::string("DXGI_FORMAT_BC1_UNORM_4B");
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return std::string("DXGI_FORMAT_BC1_UNORM_SRGB_4B");
	case DXGI_FORMAT_BC4_TYPELESS:
		return std::string("DXGI_FORMAT_BC4_TYPELESS_4B");
	case DXGI_FORMAT_BC4_UNORM:
		return std::string("DXGI_FORMAT_BC4_UNORM_4B");
	case DXGI_FORMAT_BC4_SNORM:
		return std::string("DXGI_FORMAT_BC4_SNORM_4B");

	case DXGI_FORMAT_BC2_TYPELESS:
		return std::string("DXGI_FORMAT_BC2_TYPELESS_8B");
	case DXGI_FORMAT_BC2_UNORM:
		return std::string("DXGI_FORMAT_BC2_UNORM_8B");
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return std::string("DXGI_FORMAT_BC2_UNORM_SRGB_8B");
	case DXGI_FORMAT_BC3_TYPELESS:
		return std::string("DXGI_FORMAT_BC3_TYPELESS_8B");
	case DXGI_FORMAT_BC3_UNORM:
		return std::string("DXGI_FORMAT_BC3_UNORM_8B");
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return std::string("DXGI_FORMAT_BC3_UNORM_SRGB_8B");
	case DXGI_FORMAT_BC5_TYPELESS:
		return std::string("DXGI_FORMAT_BC5_TYPELESS_8B");
	case DXGI_FORMAT_BC5_UNORM:
		return std::string("DXGI_FORMAT_BC5_UNORM_8B");
	case DXGI_FORMAT_BC5_SNORM:
		return std::string("DXGI_FORMAT_BC5_SNORM_8B");
	case DXGI_FORMAT_BC6H_TYPELESS:
		return std::string("DXGI_FORMAT_BC6H_TYPELESS_8B");
	case DXGI_FORMAT_BC6H_UF16:
		return std::string("DXGI_FORMAT_BC6H_UF16_8B");
	case DXGI_FORMAT_BC6H_SF16:
		return std::string("DXGI_FORMAT_BC6H_SF16_8B");
	case DXGI_FORMAT_BC7_TYPELESS:
		return std::string("DXGI_FORMAT_BC7_TYPELESS_8B");
	case DXGI_FORMAT_BC7_UNORM:
		return std::string("DXGI_FORMAT_BC7_UNORM_8B");
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return std::string("DXGI_FORMAT_BC7_UNORM_SRGB_8B");

	default:
		return std::string("DXGI_?_0B");
	}
}


int getBitsPerPixel(int fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
		return 32;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		return 24;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 16;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
		return 12;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;

	default:
		return 0;
	}
}



DARKSDK void ImageDestructorD3D ( void )
{
    m_CurrentId = 0;
    m_imgptr = NULL;

    for (ImagePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
    {
        // Release the texture and texture name
        tagImgData* ptr = pCheck->second;
		#ifdef DX11
        SAFE_RELEASE( ptr->lpTextureView );
        SAFE_RELEASE( ptr->lpTexture );
		#else
        SAFE_RELEASE( ptr->lpTexture );
		#endif
        SAFE_DELETE( ptr->lpName );

        // Release the rest of the image storage
        delete ptr;

        // NOTE: Not removing from m_List at this point:
        // 1 - it makes moving to the next item harder
        // 2 - it's less efficient - we'll clear the entire list at the end
    }

    // Now clear the list
    m_List.clear();

}

DARKSDK void ImageDestructor ( void )
{
	ImageDestructorD3D();
}

DARKSDK void ImageSetErrorHandler ( LPVOID pErrorHandlerPtr )
{
	// Update error handler pointer
	g_pErrorHandler = (CRuntimeErrorHandler*)pErrorHandlerPtr;
}

DARKSDK void PassSpriteInstance (  )
{
}

#ifdef DX11
DARKSDK void CreateShaderResourceViewFor ( tagImgData* pImgPtr, int iTextureFlag, GGFORMAT format )
{
	pImgPtr->lpTextureView = NULL;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;
	if ( iTextureFlag == 2 ) 
	{
		// cube needs a shader cube view
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	}
	HRESULT hr = m_pD3D->CreateShaderResourceView(pImgPtr->lpTexture, &shaderResourceViewDesc, &pImgPtr->lpTextureView);
	if ( FAILED ( hr ) )
	{
		Error1 ( "Failed to create resource view for image" );
		return;
	}
}
#endif

DARKSDK int ImageGetBitDepthFromFormat(GGFORMAT Format)
{
	#ifdef DX11
	switch(Format)
	{
		case DXGI_FORMAT_UNKNOWN : 
			return 0;

		case DXGI_FORMAT_R32G32B32A32_TYPELESS : 
		case DXGI_FORMAT_R32G32B32A32_FLOAT : 
		case DXGI_FORMAT_R32G32B32A32_UINT : 
		case DXGI_FORMAT_R32G32B32A32_SINT : 
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS : 
		case DXGI_FORMAT_R32G32B32_FLOAT : 
		case DXGI_FORMAT_R32G32B32_UINT : 
		case DXGI_FORMAT_R32G32B32_SINT : 
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS : 
		case DXGI_FORMAT_R16G16B16A16_FLOAT : 
		case DXGI_FORMAT_R16G16B16A16_UNORM : 
		case DXGI_FORMAT_R16G16B16A16_UINT : 
		case DXGI_FORMAT_R16G16B16A16_SNORM : 
		case DXGI_FORMAT_R16G16B16A16_SINT : 
		case DXGI_FORMAT_R32G32_TYPELESS : 
		case DXGI_FORMAT_R32G32_FLOAT : 
		case DXGI_FORMAT_R32G32_UINT : 
		case DXGI_FORMAT_R32G32_SINT : 
		case DXGI_FORMAT_R32G8X24_TYPELESS : 
			return 64;

		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT : 
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS : 
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT : 
		case DXGI_FORMAT_R10G10B10A2_TYPELESS : 
		case DXGI_FORMAT_R10G10B10A2_UNORM : 
		case DXGI_FORMAT_R10G10B10A2_UINT : 
		case DXGI_FORMAT_R11G11B10_FLOAT : 
		case DXGI_FORMAT_R8G8B8A8_TYPELESS : 
		case DXGI_FORMAT_B8G8R8A8_UNORM : 
		case DXGI_FORMAT_R8G8B8A8_UNORM : 
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : 
		case DXGI_FORMAT_R8G8B8A8_UINT : 
		case DXGI_FORMAT_R8G8B8A8_SNORM : 
		case DXGI_FORMAT_R8G8B8A8_SINT : 
		case DXGI_FORMAT_R16G16_TYPELESS : 
		case DXGI_FORMAT_R16G16_FLOAT : 
		case DXGI_FORMAT_R16G16_UNORM : 
		case DXGI_FORMAT_R16G16_UINT : 
		case DXGI_FORMAT_R16G16_SNORM : 
		case DXGI_FORMAT_R16G16_SINT : 
		case DXGI_FORMAT_R32_TYPELESS : 
		case DXGI_FORMAT_D32_FLOAT : 
		case DXGI_FORMAT_R32_FLOAT : 
		case DXGI_FORMAT_R32_UINT : 
		case DXGI_FORMAT_R32_SINT : 
		case DXGI_FORMAT_R24G8_TYPELESS : 
		case DXGI_FORMAT_D24_UNORM_S8_UINT : 
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS : 
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT : 
			return 32;

		case DXGI_FORMAT_R8G8_TYPELESS : 
		case DXGI_FORMAT_R8G8_UNORM : 
		case DXGI_FORMAT_R8G8_UINT : 
		case DXGI_FORMAT_R8G8_SNORM : 
		case DXGI_FORMAT_R8G8_SINT : 
		case DXGI_FORMAT_R16_TYPELESS : 
		case DXGI_FORMAT_R16_FLOAT : 
		case DXGI_FORMAT_D16_UNORM : 
		case DXGI_FORMAT_R16_UNORM : 
		case DXGI_FORMAT_R16_UINT : 
		case DXGI_FORMAT_R16_SNORM : 
		case DXGI_FORMAT_R16_SINT : 
			return 16;

		case DXGI_FORMAT_R8_TYPELESS : 
		case DXGI_FORMAT_R8_UNORM : 
		case DXGI_FORMAT_R8_UINT : 
		case DXGI_FORMAT_R8_SNORM : 
		case DXGI_FORMAT_R8_SINT : 
		case DXGI_FORMAT_A8_UNORM : 
			return 8;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;
	}
	return 0;
	#else
	switch(Format)
	{
		case GGFMT_R8G8B8 :		return 24;	break;
		case GGFMT_A8R8G8B8 :		return 32;	break;
		case GGFMT_X8R8G8B8 :		return 32;	break;
		case GGFMT_R5G6B5 :		return 16;	break;
		case GGFMT_X1R5G5B5 :		return 16;	break;
		case GGFMT_A1R5G5B5 :		return 16;	break;
		case GGFMT_A4R4G4B4 :		return 16;	break;
		case GGFMT_A8	:			return 8;	break;
		case GGFMT_R3G3B2 :		return 8;	break;
		case GGFMT_A8R3G3B2 :		return 16;	break;
		case GGFMT_X4R4G4B4 :		return 16;	break;
		case GGFMT_A2B10G10R10 :	return 32;	break;
		case GGFMT_G16R16 :		return 32;	break;
		case GGFMT_A8P8 :			return 8;	break;
		case GGFMT_P8 :			return 8;	break;
		case GGFMT_L8 :			return 8;	break;
		case GGFMT_A8L8 :			return 16;	break;
		case GGFMT_A4L4 :			return 8;	break;
	}
	return 0;
	#endif
}

DARKSDK void ImagePassCoreDataDX9 ( LPVOID pGlobPtr )
{
	#ifndef DX11
	// only if have display
	LPGGSURFACE pBackBuffer = g_pGlob->pCurrentBitmapSurface;
	if ( m_pDX == NULL || pBackBuffer == NULL )
		return;

	// Get default GGFORMAT from backbuffer
	D3DSURFACE_DESC backdesc;
	if(pBackBuffer)
	{
		HRESULT hRes = pBackBuffer->GetDesc(&backdesc);
		DWORD dwDepth=ImageGetBitDepthFromFormat(backdesc.Format);
		if(dwDepth==16) g_DefaultGGFORMAT = GGFMT_A1R5G5B5;
		if(dwDepth==32) g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	}
	else
	{
		g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	}

	// Ensure textureformat is valid, else choose next valid..
	HRESULT hRes = m_pDX->CheckDeviceFormat(	GGADAPTER_DEFAULT,
												D3DDEVTYPE_HAL,
												backdesc.Format,
												0, D3DRTYPE_TEXTURE,
												g_DefaultGGFORMAT);
	if ( FAILED( hRes ) )
	{
		// Need another texture format with an alpha
		for(DWORD t=0; t<12; t++)
		{
			switch(t)
			{
				case 0  : g_DefaultGGFORMAT = GGFMT_A8R8G8B8;		break;
				case 1  : g_DefaultGGFORMAT = GGFMT_X8R8G8B8;		break;
				case 2  : g_DefaultGGFORMAT = GGFMT_A1R5G5B5;		break;
				case 3  : g_DefaultGGFORMAT = GGFMT_A2B10G10R10;	break;
				case 4  : g_DefaultGGFORMAT = GGFMT_A4R4G4B4;		break;
				case 5  : g_DefaultGGFORMAT = GGFMT_A8R3G3B2;		break;
				case 6  : g_DefaultGGFORMAT = GGFMT_R8G8B8;		break;
				case 7  : g_DefaultGGFORMAT = GGFMT_R5G6B5;		break;
				case 8  : g_DefaultGGFORMAT = GGFMT_X1R5G5B5;		break;
				case 9  : g_DefaultGGFORMAT = GGFMT_R3G3B2;		break;
				case 10 : g_DefaultGGFORMAT = GGFMT_X4R4G4B4;		break;
				case 11 : g_DefaultGGFORMAT = GGFMT_G16R16;		break;
			}
			HRESULT hRes = m_pDX->CheckDeviceFormat(	GGADAPTER_DEFAULT,
														D3DDEVTYPE_HAL,
														backdesc.Format,
														0, D3DRTYPE_TEXTURE,
														g_DefaultGGFORMAT);
			if ( SUCCEEDED( hRes ) )
			{
				// Found a texture we can use
				return;
			}
		}
	}
	#endif
}

DARKSDK void ImagePassCoreDataDX11 ( LPVOID pGlobPtr )
{
	//  work out default GGFORMAT for current device
	#ifdef DX11
	g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	#endif
}

DARKSDK void ImagePassCoreData( LPVOID pGlobPtr )
{
	#ifdef DX11
	ImagePassCoreDataDX11(pGlobPtr);
	#else
	ImagePassCoreDataDX9(pGlobPtr);
	#endif
}

DARKSDK void ImageRefreshGRAFIX ( int iMode )
{
	if(iMode==0)
	{
		// Remove all traces of old D3D usage
		ImageDestructorD3D();
	}
	if(iMode==1)
	{
		// Get new D3D and recreate everything D3D related
		ImageConstructorD3D ( );
		ImagePassCoreData ( g_pGlob );
		PassSpriteInstance ( );
	}
}

void GetFileInMemory ( LPSTR szFilename, LPVOID* ppFileInMemoryData, DWORD* pdwFileInMemorySize, LPSTR pFinalRelPathAndFileRef )
{
	*pdwFileInMemorySize = 0;
	*ppFileInMemoryData = NULL;
	if ( g_bImageBlockActive )
	{
		// final storage string of path and file resolver (makes the filename and path uniform for imageblock retrieval)
		char pFinalRelPathAndFile[512];

		// store old directory
		char pOldDir [ 512 ];
		_getcwd ( pOldDir, 512 );

		// get combined path only
		char pPath[1024];
		char pFile[1024];
		strcpy ( pPath, pOldDir );
		strcat ( pPath, "\\" );
		strcat ( pPath, szFilename );
		strcat ( pFile, "" );
		for ( int n=strlen(pPath); n>0; n-- )
		{
			if ( pPath[n]=='\\' || pPath[n]=='/' )
			{
				// split file and path
				strcpy ( pFile, pPath + n + 1 );
				pPath [ n + 1 ] = 0;
				break;
			}
		}

		// Combine current working folder and filename to get a resolved path (removes ..\..\ stuff)
		char pResolvedDir[512];
		strcpy ( pResolvedDir, pPath );

		// Remove the part which represents the root location of the Image Block (g_iImageBlockRootPath)
		if ( strlen ( pResolvedDir ) <= strlen(g_iImageBlockRootPath) )
			strcpy ( pFinalRelPathAndFile, "" );
		else
			strcpy ( pFinalRelPathAndFile, pResolvedDir + strlen(g_iImageBlockRootPath) );

		// Ensure a \ is added
		if ( strlen ( pFinalRelPathAndFile ) > 0 )
		{
			if ( pFinalRelPathAndFile [ strlen(pFinalRelPathAndFile)-1 ]!='\\' )
			{
				// add folder divide at end of path string
				int iLen = strlen(pFinalRelPathAndFile);
				pFinalRelPathAndFile [ iLen+0 ] = '\\';
				pFinalRelPathAndFile [ iLen+1 ] = 0;
			}
		}

		// Add the filename back in
		strcat ( pFinalRelPathAndFile, pFile );

		// Restore folder
		_chdir ( pOldDir );

		// Retrieve file in memory
		if ( g_iImageBlockMode==1 )
		{
			*ppFileInMemoryData = RetrieveFromImageBlock ( pFinalRelPathAndFile, pdwFileInMemorySize );
		}

		// copy final rel path and file
		strcpy ( pFinalRelPathAndFileRef, pFinalRelPathAndFile );
	}
	else
	{
		strcpy ( pFinalRelPathAndFileRef, "" );
	}
}

DARKSDK LPGGTEXTURE GetTextureCore ( char* szFilename, GGIMAGE_INFO* info, int iOneToOnePixels, int iFullTexturePlateMode, int iDivideTextureSize )
{
	// new feature IMAGEBLOCK
	DWORD dwFileInMemorySize = 0;
	LPVOID pFileInMemoryData = NULL;
	if ( g_bImageBlockActive )
	{
		// final storage string of path and file resolver (makes the filename and path uniform for imageblock retrieval)
		// and work out true file and path, then look for it in imageblock
		char pFinalRelPathAndFile[512];
		GetFileInMemory ( szFilename, &pFileInMemoryData, &dwFileInMemorySize, pFinalRelPathAndFile );

		// Add relative path and file to image block
		if ( g_iImageBlockMode==0 )
		{
			_chdir ( g_iImageBlockRootPath );
			AddToImageBlock ( pFinalRelPathAndFile );
		}
	}

	// loads a texture and returns a pointer to it make sure that the device is valid
	if ( !m_pD3D )
		return NULL;

	// variable declarations
	LPGGTEXTURE	lpTexture = NULL;	// set texture to null

	DARKSDK int Timer(void);
	int ts = MAXTimer();

	// get file image info
	HRESULT hRes = 0;
	if ( g_iImageBlockMode==1 && pFileInMemoryData )
	{
		#ifdef DX11
		hRes = D3DX11GetImageInfoFromMemory( pFileInMemoryData, dwFileInMemorySize, NULL, info, NULL );
		if (FAILED(hRes)) 
		{
			char szRealFilename[ MAX_PATH ];
			strcpy_s( szRealFilename, MAX_PATH, szFilename );
			GG_GetRealPath( szRealFilename, 0 );
			hRes = D3DX11GetImageInfoFromFile( szRealFilename, NULL, info, NULL );
		}
		#else
		hRes = D3DXGetImageInfoFromFileInMemory( pFileInMemoryData, dwFileInMemorySize, info );
		if (FAILED(hRes)) hRes = D3DXGetImageInfoFromFile( szFilename, info );
		#endif
	}
	else
	{
		#ifdef DX11
		char szRealFilename[ MAX_PATH ];
		strcpy_s( szRealFilename, MAX_PATH, szFilename );
		GG_GetRealPath( szRealFilename, 0 );
		hRes = D3DX11GetImageInfoFromFile( szRealFilename, NULL, info, NULL );
		#else
		hRes = D3DXGetImageInfoFromFile( szFilename, info );
		#endif
	}

	// If failed to get image information, then can't be any image there either
	if (FAILED(hRes)) {
		return NULL;
	}

	// if texture size needs diviing, do so now
	if ( iDivideTextureSize>0 )
	{
		if ( iDivideTextureSize==16384 )
		{
			// notextureloadmode
			(*info).Width = 1;
			(*info).Height = 1;
		}
		else if (iDivideTextureSize == 8192)
		{
			(*info).Width = iResizeLoadImageX;
			(*info).Height = iResizeLoadImageY;
		}
		else
		{
			// divide by specified value (reduce texture consumption)
			(*info).Width /= iDivideTextureSize;
			(*info).Height /= iDivideTextureSize;
			if ( (*info).Width < 4 ) (*info).Width = 4;
			if ( (*info).Height < 4 ) (*info).Height = 4;
		}
	}

	// if mode is CUBE(2) or VOLUME(3), direct cube loader
	if ( iFullTexturePlateMode==2 || iFullTexturePlateMode==3 || iDivideTextureSize==16384 )
	{
		if ( iDivideTextureSize==16384 )
		{
			// support for quick-fake-texture-load (apply texture to scene without loading it)
			LPGGTEXTURE pFakeTex = NULL;
			#ifdef DX11
			#else
			hRes = D3DXCreateTexture ( m_pD3D,
									   (*info).Width,
									   (*info).Height,
									   1,//one mipmap only for one-to-one pixels
									   0,
									   g_DefaultGGFORMAT,
									   D3DPOOL_MANAGED,
									   &pFakeTex );
			#endif
			lpTexture = pFakeTex;
		}
		else
		{
			if ( iFullTexturePlateMode==2 ) 
			{
				// support for cube textures when specify texture flag of two (2)
				HRESULT hRes = 0;
				LPGGCUBETEXTURE pCubeTex = NULL;
				#ifdef DX11
				D3DX11_IMAGE_LOAD_INFO loadinfo;
				loadinfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
				char szRealFilename[ MAX_PATH ];
				strcpy_s( szRealFilename, MAX_PATH, szFilename );
				GG_GetRealPath( szRealFilename, 0 );
				hRes = D3DX11CreateTextureFromFile(	m_pD3D, szRealFilename, &loadinfo, NULL, &pCubeTex, NULL );
				#else
				if ( g_iImageBlockMode==1 && pFileInMemoryData )
					hRes = D3DXCreateCubeTextureFromFileInMemoryEx( m_pD3D, pFileInMemoryData, dwFileInMemorySize, (*info).Width, D3DX_DEFAULT, 0, GGFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pCubeTex );
				else
					hRes = D3DXCreateCubeTextureFromFileEx( m_pD3D, szFilename, (*info).Width, D3DX_DEFAULT, 0, GGFMT_UNKNOWN, 
					D3DPOOL_DEFAULT,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
					D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pCubeTex );
				#endif
				if ( SUCCEEDED( hRes ) ) 
				{
					// cube loaded fine
					lpTexture = (LPGGTEXTURE)pCubeTex;
				}
				else
					return NULL;
			}
			if ( iFullTexturePlateMode==3 ) 
			{
				// support for volume textures when specify 3
				HRESULT hRes = 0;
				#ifdef DX11
				#else
				LPDIRECT3DVOLUMETEXTURE9 pVolumeTex = NULL;
				if ( g_iImageBlockMode==1 && pFileInMemoryData )
					hRes = D3DXCreateVolumeTextureFromFileInMemoryEx( m_pD3D, pFileInMemoryData, dwFileInMemorySize,
								(*info).Width, (*info).Height, (*info).Depth, 1, 0, GGFMT_UNKNOWN,
								D3DPOOL_DEFAULT,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM! 
								D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
								&pVolumeTex );
				else
					hRes = D3DXCreateVolumeTextureFromFileEx ( m_pD3D, szFilename, 
								(*info).Width, (*info).Height, (*info).Depth, 3, 0, GGFMT_UNKNOWN,
								D3DPOOL_DEFAULT,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
								D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
								&pVolumeTex );
				if ( SUCCEEDED( hRes ) ) 
				{
					// volume loaded fine
					lpTexture = (LPGGTEXTURE)pVolumeTex;
				}
				else
					return NULL;
				#endif
			}
		}
	}
	else
	{
		// texture flag can control if image is GPU only or MANAGED (SYS memory copy)
		#ifdef DX11

		// just as about to load texture file, check if we already have done in preload thread

		ID3D11Resource* pPreCreatedTexture = NULL;
		for ( int n = 0; n < g_image_outputv.size(); n++ )
		{
			int iSearchStrLen = strlen(szFilename);
			if (!(iSearchStrLen > strlen(g_image_outputv[n].pFilename))) { //PE: Got exception iSearchStrLen > strlen(g_image_outputv[n].pFilename)
				if (strnicmp(g_image_outputv[n].pFilename + strlen(g_image_outputv[n].pFilename) - iSearchStrLen, szFilename, iSearchStrLen) == NULL)
				{
					if (g_image_outputv[n].pTexture == NULL) {
						char mdebug[512];
						sprintf(mdebug, "image_preload_files_wait(): %s ", szFilename);
						#ifndef NOSTEAMORVIDEO
						timestampactivity(0, mdebug);
						#endif
						image_preload_files_wait();
					}
					if (g_image_outputv[n].pTexture)
					{
						pPreCreatedTexture = g_image_outputv[n].pTexture;
						g_image_outputv[n].pTexture = NULL;
						break;
					}
				}
			}
		}

		// not unknown, use file format
		GGFORMAT newImageFormat = (*info).Format;
					
		// if DDS, load directly with original mipmap data intact
		if ( g_iImageBlockMode==1 && pFileInMemoryData )
		{
			//hRes = D3DX11CreateTextureFromFileInMemoryEx(	m_pD3D,	pFileInMemoryData, dwFileInMemorySize, info.Width,info.Height,D3DX_DEFAULT,0,GGFMT_UNKNOWN,
			//dwPoolType,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
			//D3DX_DEFAULT,D3DX_DEFAULT,0,&info,NULL,&lpTexture );
		}
		else
		{
			bool bFormatDDSLoadFriendly = true;
			if ( (*info).Format >= DXGI_FORMAT_R8G8B8A8_TYPELESS && (*info).Format <= DXGI_FORMAT_R8G8B8A8_SINT ) bFormatDDSLoadFriendly = false;
			if ( m_iMipMapNum == -1 && (*info).MipLevels == 1 && (*info).Format >= DXGI_FORMAT_BC1_TYPELESS && (*info).Format <= DXGI_FORMAT_BC5_SNORM ) bFormatDDSLoadFriendly = false;

			#ifdef WIP_PROLOADLEVELTEXTURES
			//PE: record all level textures that can be preloaded.
			extern std::vector<std::string> preload_setup;
			//Only record mipmap -1.
			if (m_iMipMapNum == -1)
				preload_setup.push_back(szFilename);
			#endif

			//PE: Used to see what files can be thread preloaded.
			//if (pPreCreatedTexture == NULL) { // && strnicmp(szFilename + strlen(szFilename) - 4, ".dds", 4) == NULL) {
			//	char mdebug[2048];
			//	sprintf(mdebug, "DLOAD POSSIBLE:(%d) %s ", m_iMipMapNum, szFilename);
			//	timestampactivity(0, mdebug);
			//}

			if (pPreCreatedTexture != NULL && m_iMipMapNum == -1 && strnicmp(szFilename + strlen(szFilename) - 4, ".dds", 4) == NULL) 
			{
				//PE: Already loaded with mipmaps generated.
#ifdef WICKEDENGINE
				lpTexture = pPreCreatedTexture;
#endif
				pPreCreatedTexture = NULL;
			}
			else if ( strnicmp ( szFilename + strlen(szFilename) - 4, ".dds", 4 ) == NULL && bFormatDDSLoadFriendly == true )
			{

				if ( iDivideTextureSize <= 1 )
				{
					if (pPreCreatedTexture != NULL ) //PE: Should always be correct when here.
					{
#ifdef WICKEDENGINE
						lpTexture = pPreCreatedTexture;
#endif
						pPreCreatedTexture = NULL;
					}
					else
					{
						char szRealFilename[ MAX_PATH ];
						strcpy_s( szRealFilename, MAX_PATH, szFilename );
						GG_GetRealPath( szRealFilename, 0 );

						// effort to speed up loading of DDS texture files (above took 0.1s per 512K texture, .4s per 2K texture)
						// as above, you cannot auto gen mipmaps if the texture is compressed (do this as part of file)
						size_t origsize = strlen(szRealFilename) + 1;
						const size_t newsize = 1024;
						size_t convertedChars = 0;
						wchar_t wcstringTextureFilename[newsize];
						mbstowcs_s(&convertedChars, wcstringTextureFilename, origsize, szRealFilename, _TRUNCATE);
						hRes = DirectX::CreateDDSTextureFromFile(m_pD3D, m_pImmediateContext, wcstringTextureFilename, &lpTexture, NULL);
					}
				}
				else
				{
					//PE: We need support for resize inside thread loader.

					// meets all fast load requirements, but need to reduce texture size when loading
					wchar_t wTexFilename[512];
					MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wTexFilename, sizeof(wTexFilename));
					DirectX::TexMetadata imageData;
					DirectX::ScratchImage imageTexture;
					hRes = GetMetadataFromDDSFile( wTexFilename, DirectX::DDS_FLAGS_NONE, imageData );			
					hRes = LoadFromDDSFile( wTexFilename, DirectX::DDS_FLAGS_NONE, &imageData, imageTexture );
					DirectX::ScratchImage* pWrkImage = &imageTexture;
					DirectX::ScratchImage convertedTexture;
					bool bWasCompressed = false;
					DXGI_FORMAT storeFormat = imageTexture.GetMetadata().format;
					if ( storeFormat >= DXGI_FORMAT_BC1_TYPELESS && storeFormat <= DXGI_FORMAT_BC5_SNORM )
					{
						hRes = DirectX::Decompress( imageTexture.GetImages(), imageTexture.GetImageCount(), imageTexture.GetMetadata(), DXGI_FORMAT_B8G8R8A8_UNORM, convertedTexture );
						pWrkImage = &convertedTexture;
						imageTexture.Release();
						bWasCompressed = true;
					}
					DirectX::ScratchImage resizedTexture;
					hRes = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), (*info).Width, (*info).Height, DirectX::TEX_FILTER_SEPARATE_ALPHA, resizedTexture );
					pWrkImage->Release();
					pWrkImage = &resizedTexture;

					DirectX::ScratchImage resizedCompressedTexture;
					if ( bWasCompressed == true )
					{
						hRes = DirectX::Compress(pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(),
						storeFormat, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, resizedCompressedTexture );
						//PE: MEM bug resizedTexture was never released.
						pWrkImage->Release();
						pWrkImage = &resizedCompressedTexture;
					}

					hRes = DirectX::CreateTexture ( m_pD3D, pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), &lpTexture );
					pWrkImage->Release();
				}
			}
			else
			{
				//PE: Make sure we keep current compression, if we just need to generate mipmaps.
				if ( m_iMipMapNum == -1 && (*info).MipLevels == 1 && strnicmp(szFilename + strlen(szFilename) - 4, ".dds", 4) == NULL ) 
				{

					//Only need to generate mipmaps so keep original compression.
					D3DX11_IMAGE_LOAD_INFO loadinfo;
					loadinfo.Format = (*info).Format;
					loadinfo.MipLevels = m_iMipMapNum; // set using SetMipmapNum when want no mipmaps (i.e. vegmask)
					loadinfo.Width = (*info).Width;
					loadinfo.Height = (*info).Height;

					// special THREAD-SAFE way to preload resources, then use it instead of doing it in the main thread
					if ( pPreCreatedTexture != NULL )
					{
#ifdef WICKEDENGINE
						lpTexture = pPreCreatedTexture;
#endif
						pPreCreatedTexture = NULL;
					}
					else
					{
						char szRealFilename[ MAX_PATH ];
						strcpy_s( szRealFilename, MAX_PATH, szFilename );
						GG_GetRealPath( szRealFilename, 0 );
						hRes = D3DX11CreateTextureFromFile(m_pD3D, szRealFilename, &loadinfo, NULL, &lpTexture, NULL);
					}
				}
				else 
				{
					//PE: Normally only png and bmp goes here.
					if (pPreCreatedTexture != NULL) //PE: Should always be correct when here.
					{
#ifdef WICKEDENGINE
						lpTexture = pPreCreatedTexture;
#endif
						pPreCreatedTexture = NULL;
					}
					else
					{
						char szRealFilename[ MAX_PATH ];
						strcpy_s( szRealFilename, MAX_PATH, szFilename );
						GG_GetRealPath( szRealFilename, 0 );

						// to conform to internal BGRA format (DX9 to DX11 nonesense)
						D3DX11_IMAGE_LOAD_INFO loadinfo;
						#ifdef WICKEDENGINE
						//PE: In Wicked we need  DXGI_FORMAT_R8G8B8A8_UNORM (28)
						loadinfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						(*info).Format = loadinfo.Format;
						#else
						loadinfo.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
						(*info).Format = loadinfo.Format;
						#endif
						loadinfo.MipLevels = m_iMipMapNum; // set using SetMipmapNum when want no mipmaps (i.e. vegmask)
						loadinfo.Width = (*info).Width;
						loadinfo.Height = (*info).Height;
						hRes = D3DX11CreateTextureFromFile(m_pD3D, szRealFilename, &loadinfo, NULL, &lpTexture, NULL);
					}
				}
			}
		}

		// 010205 - default mem can run out
		if (lpTexture == NULL) {
			return NULL;
		}

		// adjust to actual size if texture smaller
		//D3DSURFACE_DESC desc;
		//lpTexture->GetLevelDesc(0,&desc);
		//if(desc.Width<info.Width) info.Width=desc.Width;
		//if(desc.Height<info.Height) info.Height=desc.Height;

		#else
		D3DPOOL dwPoolType = D3DPOOL_DEFAULT;
		if ( iFullTexturePlateMode==10 )
			dwPoolType = D3DPOOL_MANAGED;

		// perfect texture is one-to-one pixels and no mipmaps or alpha
		if ( iOneToOnePixels==1 )
		{
			// Keep Alpha from file
			DWORD dwUseAlphaCode = 0;
			if(_strnicmp( szFilename + (strlen(szFilename)-4), ".dds", 4)==NULL
			|| _strnicmp( szFilename + (strlen(szFilename)-4), ".png", 4)==NULL
			|| _strnicmp( szFilename + (strlen(szFilename)-4), ".tga", 4)==NULL)
				dwUseAlphaCode = 0;
			else
				dwUseAlphaCode = m_Color;

			// leeadd - 300305 - format selection (replaced info.Format or g_DefaultGGFORMAT in createtexture as we need SET IMAGE COLORKEY to retain alpha!)
			GGFORMAT newImageFormat = (*info).Format;
			if ( dwUseAlphaCode!=0 ) newImageFormat = g_DefaultGGFORMAT;

			// create a new texture/image
			hRes = D3DXCreateTexture ( m_pD3D,
									   (*info).Width,
									   (*info).Height,
									   1,//one mipmap only for one-to-one pixels
									   0,
									   newImageFormat,
									   dwPoolType,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
									   &lpTexture );

			// 010205 - default mem can run out
			if ( lpTexture==NULL )
				return NULL;

			// adjust to actual size if texture smaller
			D3DSURFACE_DESC desc;
			lpTexture->GetLevelDesc(0,&desc);
			if(desc.Width<(*info).Width) (*info).Width=desc.Width;
			if(desc.Height<(*info).Height) (*info).Height=desc.Height;
			if( SUCCEEDED ( hRes ))
			{
				LPGGSURFACE pSurface=NULL;
				hRes = lpTexture->GetSurfaceLevel(0, &pSurface);
				if( SUCCEEDED ( hRes ))
				{
					// load surface data into it
					RECT destrc = { 0, 0, (LONG)(*info).Width, (LONG)(*info).Height };
					if ( g_iImageBlockMode==1 && pFileInMemoryData )
						hRes = D3DXLoadSurfaceFromFileInMemory( pSurface, NULL, &destrc, pFileInMemoryData, dwFileInMemorySize, NULL, D3DX_FILTER_POINT, dwUseAlphaCode, info );
					else
						hRes = D3DXLoadSurfaceFromFile( pSurface, NULL, &destrc, szFilename, NULL, D3DX_FILTER_POINT, dwUseAlphaCode, info );
					pSurface->Release();
				}
			}
            // If any of the previous steps failed, release the target texture
            // to signal a failure to load.
            if ( FAILED ( hRes ) )
                SAFE_RELEASE( lpTexture );
		}
		else
		{
			// DDS or Other File Format (leefix - 220303 - added TGA to keep alpha load)
			if(_strnicmp( szFilename + (strlen(szFilename)-4), ".dds", 4)==NULL
			|| _strnicmp( szFilename + (strlen(szFilename)-4), ".png", 4)==NULL
			|| _strnicmp( szFilename + (strlen(szFilename)-4), ".tga", 4)==NULL)
			{
				// lee - 180406 - u6rc10 - not unknown, use file format
				GGFORMAT newImageFormat = (*info).Format;
					
				// if DDS, load directly with original mipmap data intact
				if ( g_iImageBlockMode==1 && pFileInMemoryData )
					hRes = D3DXCreateTextureFromFileInMemoryEx(	m_pD3D,	pFileInMemoryData, dwFileInMemorySize, (*info).Width,(*info).Height,D3DX_DEFAULT,0,GGFMT_UNKNOWN,
					dwPoolType,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
					D3DX_DEFAULT,D3DX_DEFAULT,0,info,NULL,&lpTexture );
				else
					hRes = D3DXCreateTextureFromFileEx(	m_pD3D,
													szFilename,
													(*info).Width,
													(*info).Height,
													g_dwMipMapGenMode,//D3DX_FROM_FILE,//D3DX_DEFAULT, // 031014 - take mipmap as stored in file, DO NOT generate a whole chain!!
													0,
													newImageFormat,// 180406 - u6rc10 - GGFMT_UNKNOWN,//g_DefaultGGFORMAT,
													dwPoolType,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
													D3DX_DEFAULT,
													D3DX_DEFAULT,
													0, //m_Color, LEEFIX - DDS/TGA has its own Alphamap!
													info,
													NULL,
													&lpTexture );

				// 010205 - default mem can run out
				if ( lpTexture==NULL )
					return NULL;

				// adjust to actual size if texture smaller
				D3DSURFACE_DESC desc;
				lpTexture->GetLevelDesc(0,&desc);
				if(desc.Width<(*info).Width) (*info).Width=desc.Width;
				if(desc.Height<(*info).Height) (*info).Height=desc.Height;
			}
			else
			{
				// create a new texture/image
				hRes = D3DXCreateTexture ( 
										  m_pD3D,
										  (*info).Width,
										  (*info).Height,
										  D3DX_DEFAULT,
										  0,
										  g_DefaultGGFORMAT,
										  dwPoolType,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
										  &lpTexture	       );

				// 010205 - default mem can run out
				if ( lpTexture==NULL )
					return NULL;

				// adjust to actual size if texture smaller
				D3DSURFACE_DESC desc;
				lpTexture->GetLevelDesc(0,&desc);
				if(desc.Width<(*info).Width) (*info).Width=desc.Width;
				if(desc.Height<(*info).Height) (*info).Height=desc.Height;
				if( SUCCEEDED ( hRes ))
				{
					// get surface of texture (as many mipmap levels as it has)
					for ( DWORD level=0; level<lpTexture->GetLevelCount(); level++ )
					{
						LPGGSURFACE pSurface=NULL;
						hRes = lpTexture->GetSurfaceLevel(level, &pSurface);
						if( SUCCEEDED ( hRes ))
						{
							// load surface data into it
							// leefix-260603-ditheris aweful!
							// leefix-220703-changed again so level 0 is clean mip and rest are dithered for good distance textures
							if ( g_iImageBlockMode==1 && pFileInMemoryData )
							{
								if ( level==0 )
									hRes = D3DXLoadSurfaceFromFileInMemory( pSurface, NULL, NULL, pFileInMemoryData, dwFileInMemorySize, NULL, D3DX_FILTER_POINT, m_Color, info );
								else
									hRes = D3DXLoadSurfaceFromFileInMemory( pSurface, NULL, NULL, pFileInMemoryData, dwFileInMemorySize, NULL, D3DX_DEFAULT, m_Color, info );
							}
							else
							{
								if ( level==0 )
									hRes = D3DXLoadSurfaceFromFile( pSurface, NULL, NULL, szFilename, NULL, D3DX_FILTER_POINT, m_Color, info );
								else
									hRes = D3DXLoadSurfaceFromFile( pSurface, NULL, NULL, szFilename, NULL, D3DX_DEFAULT, m_Color, info );
							}

							pSurface->Release();
						}
						else
							break;
					}
				}
			}
		}
		#endif
	}

	// check the texture loaded in ok
	if ( !lpTexture )
		Error1 ( "Failed to load texture for image library" );

	// needed image info
	m_iWidth  = (*info).Width;		// file width
	m_iHeight = (*info).Height;	// file height

	// finally return the pointer
	return lpTexture;
}

DARKSDK ID3D11Resource* PreloadThreadSafeImage(LPSTR szFilename, int iMipMaps)
{
	// this is called from a thread, needs to be THREAD-SAFE! (pretty sure it is now)

	// can replace this and transport format, size, etc via list!
	GGIMAGE_INFO info;

	// LB: Seems the 'GG_GetRealPath' call is not thread safe (crashes on create_file) so preload of images
	// will not only be applicable to files in the Program Files Root Area
	//LPSTR szRealFilename = szFilename;
	// LB: But since then added the CRITICAL SECTION which protects this so only one use at a time on same thread :)
	char szRealFilename[ MAX_PATH ];
	strcpy_s( szRealFilename, MAX_PATH, szFilename );
	GG_GetRealPath( szRealFilename, 0 );

	// Load in image info from file (thread safe)
	HRESULT hRes = D3DX11GetImageInfoFromFile(szRealFilename, NULL, &info, NULL);

	// can we call a DX11 create texture function from a thread (DX11 supposed to be threadsafe and thread pump undocumented!!)
	ID3D11Resource* pPreCreatedTexture = NULL;
	D3DX11_IMAGE_LOAD_INFO loadinfo;
	loadinfo.Format = (info).Format;
	loadinfo.MipLevels = iMipMaps; //PE: Enable us to set per file mipmaps, so we can load more in the background.
	loadinfo.Width = (info).Width;
	loadinfo.Height = (info).Height;

	// Create texture (thread safe as m_pD3D can be used by multiple threads)
	hRes = D3DX11CreateTextureFromFile(m_pD3D, szRealFilename, &loadinfo, NULL, &pPreCreatedTexture, NULL);

	// return
	if (hRes == 0)
		return pPreCreatedTexture;
	else
		return NULL;
}

DARKSDK void SetImageAutoMipMap ( int iGenerateMipMaps )
{
	#ifdef DX11
	#else
	if ( iGenerateMipMaps==1 )
		g_dwMipMapGenMode = D3DX_DEFAULT;
	else
		g_dwMipMapGenMode = D3DX_FROM_FILE;
	#endif
}

DARKSDK bool LoadImageCoreFullTex ( char* szFilename, LPGGTEXTURE* pImage, GGIMAGE_INFO* info, int iFullTexturePlateMode, int iDivideTextureSize )
{
	bool bUseLegacyImageLoader = true;
	#ifdef WICKEDENGINE
	// wicked uses its own resource image loader, this now only required for IMGUI UI image loading
	// though a global flag determines if we load/re-use a dummy 32x32 texture, or allow the real image to load
	bUseLegacyImageLoader = g_bAllowLegacyImageLoadingForUI;
	#endif

	// load in a real image using the legacy system
	bool bRes = false;
	if (bUseLegacyImageLoader == true)
	{
		// Uses actual or virtual file..
		char VirtualFilename[_MAX_PATH];
		strcpy(VirtualFilename, szFilename);
		CheckForWorkshopFile(VirtualFilename);

		// Decrypt and use media
		g_pGlob->Decrypt(VirtualFilename);

		// load the media
		*pImage = GetTextureCore(VirtualFilename, info, 0, iFullTexturePlateMode, iDivideTextureSize);
		if (*pImage) bRes = true;

		// get media info
		HRESULT hRes = 0;
		if (g_bImageBlockActive && g_iImageBlockMode == 1)
		{
			DWORD dwFileInMemorySize = 0;
			LPVOID pFileInMemoryData = NULL;
			char pFinalRelPathAndFile[512];
			GetFileInMemory(VirtualFilename, &pFileInMemoryData, &dwFileInMemorySize, pFinalRelPathAndFile);
			#ifdef DX11
			hRes = D3DX11GetImageInfoFromMemory(pFileInMemoryData, dwFileInMemorySize, NULL, info, NULL);
			#else
			hRes = D3DXGetImageInfoFromFileInMemory(pFileInMemoryData, dwFileInMemorySize, info);
			#endif
		}

		// get info from physical file if not in image block
		#ifdef DX11
		/// 230817 - so do not overwrite info data thats been changed
		/// if ( hRes==0 || hRes!=GG_OK ) hRes = D3DX11GetImageInfoFromFile( VirtualFilename, NULL, info, NULL );
		#else
		if (hRes == 0 || hRes != GG_OK) hRes = D3DXGetImageInfoFromFile(VirtualFilename, info);
		#endif

		// re-encrypt if applicable
		g_pGlob->Encrypt(VirtualFilename);
	}
	#ifdef WICKEDENGINE
	else
	{
		// load or use the dummy image (preserved logical flow by allowing images to think they have been loaded in)
		// can clean up this for the wicked engine down the line once Classic is obsolete
		if (g_pDummyImage == NULL)
		{
			if (stricmp(szFilename, "Files\\editors\\gfx\\dummy.png") == NULL)
			{
				*pImage = GetTextureCore(szFilename, info, 0, iFullTexturePlateMode, iDivideTextureSize);
				if (*pImage)
				{
					g_pDummyImage = (LPVOID)*pImage;
					g_pDummyInfo = *info;
					bRes = true;
				}
			}
		}
		else
		{
			*pImage = (ID3D11Resource*)g_pDummyImage;
			*info = g_pDummyInfo;
			bRes = true;
		}
	}
	#endif

	// success or no
	return bRes;
}

DARKSDK bool LoadImageCore ( char* szFilename, LPGGTEXTURE* pImage, GGIMAGE_INFO* info )
{
	// default is full texture plate mode zero = simple surface
	return LoadImageCoreFullTex ( szFilename, pImage, info, 0, 0 );
}

