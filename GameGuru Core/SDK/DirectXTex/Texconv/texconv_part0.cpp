//--------------------------------------------------------------------------------------
// File: TexConv.cpp
//
// DirectX Texture Converter
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <memory>
#include <list>

#include <wrl\client.h>

#include <d3d11.h>
#include <dxgi.h>
#include <dxgiformat.h>

#include <wincodec.h>

#include "directxtex.h"

#include "DirectXPackedVector.h"

//Uncomment to add support for OpenEXR (.exr)
//#define USE_OPENEXR

#ifdef USE_OPENEXR
// See <https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR> for details
#include "DirectXTexEXR.h"
#endif

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

enum OPTIONS
{
    OPT_RECURSIVE = 1,
    OPT_WIDTH,
    OPT_HEIGHT,
    OPT_MIPLEVELS,
    OPT_FORMAT,
    OPT_FILTER,
    OPT_SRGBI,
    OPT_SRGBO,
    OPT_SRGB,
    OPT_PREFIX,
    OPT_SUFFIX,
    OPT_OUTPUTDIR,
    OPT_OVERWRITE,
    OPT_FILETYPE,
    OPT_HFLIP,
    OPT_VFLIP,
    OPT_DDS_DWORD_ALIGN,
    OPT_DDS_BAD_DXTN_TAILS,
    OPT_USE_DX10,
    OPT_NOLOGO,
    OPT_TIMING,
    OPT_SEPALPHA,
    OPT_TYPELESS_UNORM,
    OPT_TYPELESS_FLOAT,
    OPT_PREMUL_ALPHA,
    OPT_DEMUL_ALPHA,
    OPT_EXPAND_LUMINANCE,
    OPT_TA_WRAP,
    OPT_TA_MIRROR,
    OPT_FORCE_SINGLEPROC,
    OPT_GPU,
    OPT_NOGPU,
    OPT_FEATURE_LEVEL,
    OPT_FIT_POWEROF2,
    OPT_ALPHA_WEIGHT,
    OPT_NORMAL_MAP,
    OPT_NORMAL_MAP_AMPLITUDE,
    OPT_COMPRESS_UNIFORM,
    OPT_COMPRESS_MAX,
    OPT_COMPRESS_QUICK,
    OPT_COMPRESS_DITHER,
    OPT_WIC_QUALITY,
    OPT_WIC_LOSSLESS,
    OPT_WIC_MULTIFRAME,
    OPT_COLORKEY,
    OPT_TONEMAP,
    OPT_X2_BIAS,
    OPT_MAX
};

static_assert(OPT_MAX <= 64, "dwOptions is a DWORD64 bitfield");

struct SConversion
{
    wchar_t szSrc[MAX_PATH];
    wchar_t szDest[MAX_PATH];
};

struct SValue
{
    LPCWSTR pName;
    DWORD dwValue;
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const SValue g_pOptions[] =
{
    { L"r",             OPT_RECURSIVE },
    { L"w",             OPT_WIDTH },
    { L"h",             OPT_HEIGHT },
    { L"m",             OPT_MIPLEVELS },
    { L"f",             OPT_FORMAT },
    { L"if",            OPT_FILTER },
    { L"srgbi",         OPT_SRGBI },
    { L"srgbo",         OPT_SRGBO },
    { L"srgb",          OPT_SRGB },
    { L"px",            OPT_PREFIX },
    { L"sx",            OPT_SUFFIX },
    { L"o",             OPT_OUTPUTDIR },
    { L"y",             OPT_OVERWRITE },
    { L"ft",            OPT_FILETYPE },
    { L"hflip",         OPT_HFLIP },
    { L"vflip",         OPT_VFLIP },
    { L"dword",         OPT_DDS_DWORD_ALIGN },
    { L"badtails",      OPT_DDS_BAD_DXTN_TAILS },
    { L"dx10",          OPT_USE_DX10 },
    { L"nologo",        OPT_NOLOGO },
    { L"timing",        OPT_TIMING },
    { L"sepalpha",      OPT_SEPALPHA },
    { L"tu",            OPT_TYPELESS_UNORM },
    { L"tf",            OPT_TYPELESS_FLOAT },
    { L"pmalpha",       OPT_PREMUL_ALPHA },
    { L"alpha",         OPT_DEMUL_ALPHA },
    { L"xlum",          OPT_EXPAND_LUMINANCE },
    { L"wrap",          OPT_TA_WRAP },
    { L"mirror",        OPT_TA_MIRROR },
    { L"singleproc",    OPT_FORCE_SINGLEPROC },
    { L"gpu",           OPT_GPU },
    { L"nogpu",         OPT_NOGPU },
    { L"fl",            OPT_FEATURE_LEVEL },
    { L"pow2",          OPT_FIT_POWEROF2 },
    { L"aw",            OPT_ALPHA_WEIGHT },
    { L"nmap",          OPT_NORMAL_MAP },
    { L"nmapamp",       OPT_NORMAL_MAP_AMPLITUDE },
    { L"bcuniform",     OPT_COMPRESS_UNIFORM },
    { L"bcmax",         OPT_COMPRESS_MAX },
    { L"bcquick",       OPT_COMPRESS_QUICK },
    { L"bcdither",      OPT_COMPRESS_DITHER },
    { L"wicq",          OPT_WIC_QUALITY },
    { L"wiclossless",   OPT_WIC_LOSSLESS },
    { L"wicmulti",      OPT_WIC_MULTIFRAME },
    { L"c",             OPT_COLORKEY },
    { L"tonemap",       OPT_TONEMAP },
    { L"x2bias",        OPT_X2_BIAS },
    { nullptr,          0 }
};

#define DEFFMT(fmt) { L#fmt, DXGI_FORMAT_ ## fmt }

const SValue g_pFormats[] =
{
    // List does not include _TYPELESS or depth/stencil formats
    DEFFMT(R32G32B32A32_FLOAT),
    DEFFMT(R32G32B32A32_UINT),
    DEFFMT(R32G32B32A32_SINT),
    DEFFMT(R32G32B32_FLOAT),
    DEFFMT(R32G32B32_UINT),
    DEFFMT(R32G32B32_SINT),
    DEFFMT(R16G16B16A16_FLOAT),
    DEFFMT(R16G16B16A16_UNORM),
    DEFFMT(R16G16B16A16_UINT),
    DEFFMT(R16G16B16A16_SNORM),
    DEFFMT(R16G16B16A16_SINT),
    DEFFMT(R32G32_FLOAT),
    DEFFMT(R32G32_UINT),
    DEFFMT(R32G32_SINT),
    DEFFMT(R10G10B10A2_UNORM),
    DEFFMT(R10G10B10A2_UINT),
    DEFFMT(R11G11B10_FLOAT),
    DEFFMT(R8G8B8A8_UNORM),
    DEFFMT(R8G8B8A8_UNORM_SRGB),
    DEFFMT(R8G8B8A8_UINT),
    DEFFMT(R8G8B8A8_SNORM),
    DEFFMT(R8G8B8A8_SINT),
    DEFFMT(R16G16_FLOAT),
    DEFFMT(R16G16_UNORM),
    DEFFMT(R16G16_UINT),
    DEFFMT(R16G16_SNORM),
    DEFFMT(R16G16_SINT),
    DEFFMT(R32_FLOAT),
    DEFFMT(R32_UINT),
    DEFFMT(R32_SINT),
    DEFFMT(R8G8_UNORM),
    DEFFMT(R8G8_UINT),
    DEFFMT(R8G8_SNORM),
    DEFFMT(R8G8_SINT),
    DEFFMT(R16_FLOAT),
    DEFFMT(R16_UNORM),
    DEFFMT(R16_UINT),
    DEFFMT(R16_SNORM),
    DEFFMT(R16_SINT),
    DEFFMT(R8_UNORM),
    DEFFMT(R8_UINT),
    DEFFMT(R8_SNORM),
    DEFFMT(R8_SINT),
    DEFFMT(A8_UNORM),
    DEFFMT(R9G9B9E5_SHAREDEXP),
    DEFFMT(R8G8_B8G8_UNORM),
    DEFFMT(G8R8_G8B8_UNORM),
    DEFFMT(BC1_UNORM),
    DEFFMT(BC1_UNORM_SRGB),
    DEFFMT(BC2_UNORM),
    DEFFMT(BC2_UNORM_SRGB),
    DEFFMT(BC3_UNORM),
    DEFFMT(BC3_UNORM_SRGB),
    DEFFMT(BC4_UNORM),
    DEFFMT(BC4_SNORM),
    DEFFMT(BC5_UNORM),
    DEFFMT(BC5_SNORM),
    DEFFMT(B5G6R5_UNORM),
    DEFFMT(B5G5R5A1_UNORM),

    // DXGI 1.1 formats
    DEFFMT(B8G8R8A8_UNORM),
    DEFFMT(B8G8R8X8_UNORM),
    DEFFMT(R10G10B10_XR_BIAS_A2_UNORM),
    DEFFMT(B8G8R8A8_UNORM_SRGB),
    DEFFMT(B8G8R8X8_UNORM_SRGB),
    DEFFMT(BC6H_UF16),
    DEFFMT(BC6H_SF16),
    DEFFMT(BC7_UNORM),
    DEFFMT(BC7_UNORM_SRGB),

    // DXGI 1.2 formats
    DEFFMT(AYUV),
    DEFFMT(Y410),
    DEFFMT(Y416),
    DEFFMT(YUY2),
    DEFFMT(Y210),
    DEFFMT(Y216),
    // No support for legacy paletted video formats (AI44, IA44, P8, A8P8)
    DEFFMT(B4G4R4A4_UNORM),

    { nullptr, DXGI_FORMAT_UNKNOWN }
};

const SValue g_pReadOnlyFormats[] =
{
    DEFFMT(R32G32B32A32_TYPELESS),
    DEFFMT(R32G32B32_TYPELESS),
    DEFFMT(R16G16B16A16_TYPELESS),
    DEFFMT(R32G32_TYPELESS),
    DEFFMT(R32G8X24_TYPELESS),
    DEFFMT(D32_FLOAT_S8X24_UINT),
    DEFFMT(R32_FLOAT_X8X24_TYPELESS),
    DEFFMT(X32_TYPELESS_G8X24_UINT),
    DEFFMT(R10G10B10A2_TYPELESS),
    DEFFMT(R8G8B8A8_TYPELESS),
    DEFFMT(R16G16_TYPELESS),
    DEFFMT(R32_TYPELESS),
    DEFFMT(D32_FLOAT),
    DEFFMT(R24G8_TYPELESS),
    DEFFMT(D24_UNORM_S8_UINT),
    DEFFMT(R24_UNORM_X8_TYPELESS),
    DEFFMT(X24_TYPELESS_G8_UINT),
    DEFFMT(R8G8_TYPELESS),
    DEFFMT(R16_TYPELESS),
    DEFFMT(R8_TYPELESS),
    DEFFMT(BC1_TYPELESS),
    DEFFMT(BC2_TYPELESS),
    DEFFMT(BC3_TYPELESS),
    DEFFMT(BC4_TYPELESS),
    DEFFMT(BC5_TYPELESS),

    // DXGI 1.1 formats
    DEFFMT(B8G8R8A8_TYPELESS),
    DEFFMT(B8G8R8X8_TYPELESS),
    DEFFMT(BC6H_TYPELESS),
    DEFFMT(BC7_TYPELESS),

    // DXGI 1.2 formats
    DEFFMT(NV12),
    DEFFMT(P010),
    DEFFMT(P016),
    DEFFMT(420_OPAQUE),
    DEFFMT(NV11),

    // DXGI 1.3 formats
    { L"P208", DXGI_FORMAT(130) },
    { L"V208", DXGI_FORMAT(131) },
    { L"V408", DXGI_FORMAT(132) },

    { nullptr, DXGI_FORMAT_UNKNOWN }
};

const SValue g_pFilters[] =
{
    { L"POINT",                     TEX_FILTER_POINT },
    { L"LINEAR",                    TEX_FILTER_LINEAR },
    { L"CUBIC",                     TEX_FILTER_CUBIC },
    { L"FANT",                      TEX_FILTER_FANT },
    { L"BOX",                       TEX_FILTER_BOX },
    { L"TRIANGLE",                  TEX_FILTER_TRIANGLE },
    { L"POINT_DITHER",              TEX_FILTER_POINT | TEX_FILTER_DITHER },
    { L"LINEAR_DITHER",             TEX_FILTER_LINEAR | TEX_FILTER_DITHER },
    { L"CUBIC_DITHER",              TEX_FILTER_CUBIC | TEX_FILTER_DITHER },
    { L"FANT_DITHER",               TEX_FILTER_FANT | TEX_FILTER_DITHER },
    { L"BOX_DITHER",                TEX_FILTER_BOX | TEX_FILTER_DITHER },
    { L"TRIANGLE_DITHER",           TEX_FILTER_TRIANGLE | TEX_FILTER_DITHER },
    { L"POINT_DITHER_DIFFUSION",    TEX_FILTER_POINT | TEX_FILTER_DITHER_DIFFUSION },
    { L"LINEAR_DITHER_DIFFUSION",   TEX_FILTER_LINEAR | TEX_FILTER_DITHER_DIFFUSION },
    { L"CUBIC_DITHER_DIFFUSION",    TEX_FILTER_CUBIC | TEX_FILTER_DITHER_DIFFUSION },
    { L"FANT_DITHER_DIFFUSION",     TEX_FILTER_FANT | TEX_FILTER_DITHER_DIFFUSION },
    { L"BOX_DITHER_DIFFUSION",      TEX_FILTER_BOX | TEX_FILTER_DITHER_DIFFUSION },
    { L"TRIANGLE_DITHER_DIFFUSION", TEX_FILTER_TRIANGLE | TEX_FILTER_DITHER_DIFFUSION },
    { nullptr,                      TEX_FILTER_DEFAULT                              }
};

#define CODEC_DDS 0xFFFF0001 
#define CODEC_TGA 0xFFFF0002
#define CODEC_HDP 0xFFFF0003
#define CODEC_JXR 0xFFFF0004
#define CODEC_HDR 0xFFFF0005
#define CODEC_EXR 0xFFFF0006

const SValue g_pSaveFileTypes[] =   // valid formats to write to
{
    { L"BMP",   WIC_CODEC_BMP  },
    { L"JPG",   WIC_CODEC_JPEG },
    { L"JPEG",  WIC_CODEC_JPEG },
    { L"PNG",   WIC_CODEC_PNG  },
    { L"DDS",   CODEC_DDS      },
    { L"TGA",   CODEC_TGA      },
    { L"HDR",   CODEC_HDR      },
    { L"TIF",   WIC_CODEC_TIFF },
    { L"TIFF",  WIC_CODEC_TIFF },
    { L"WDP",   WIC_CODEC_WMP  },
    { L"HDP",   CODEC_HDP      },
    { L"JXR",   CODEC_JXR      },
#ifdef USE_OPENEXR
    { L"EXR",   CODEC_EXR      },
#endif
    { nullptr,  CODEC_DDS      }
};

const SValue g_pFeatureLevels[] =   // valid feature levels for -fl for maximimum size
{
    { L"9.1",  2048 },
    { L"9.2",  2048 },
    { L"9.3",  4096 },
    { L"10.0", 8192 },
    { L"10.1", 8192 },
    { L"11.0", 16384 },
    { L"11.1", 16384 },
    { L"12.0", 16384 },
    { L"12.1", 16384 },
    { nullptr, 0 },
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4616 6211 )

namespace
{
    inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }

    struct find_closer { void operator()(HANDLE h) { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

    typedef public std::unique_ptr<void, find_closer> ScopedFindHandle;

    inline static bool ispow2(size_t x)
    {
        return ((x != 0) && !(x & (x - 1)));
    }

#pragma prefast(disable : 26018, "Only used with static internal arrays")

    DWORD LookupByName(const wchar_t *pName, const SValue *pArray)
    {
        while (pArray->pName)
        {
            if (!_wcsicmp(pName, pArray->pName))
                return pArray->dwValue;

            pArray++;
        }

        return 0;
    }


    const wchar_t* LookupByValue(DWORD pValue, const SValue *pArray)
    {
        while (pArray->pName)
        {
            if (pValue == pArray->dwValue)
                return pArray->pName;

            pArray++;
        }

        return L"";
    }


    void SearchForFiles(const wchar_t* path, std::list<SConversion>& files, bool recursive)
    {
        // Process files
        WIN32_FIND_DATA findData = {};
        ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path,
            FindExInfoBasic, &findData,
            FindExSearchNameMatch, nullptr,
            FIND_FIRST_EX_LARGE_FETCH)));
        if (hFile)
        {
            for (;;)
            {
                if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
                {
                    wchar_t drive[_MAX_DRIVE] = {};
                    wchar_t dir[_MAX_DIR] = {};
                    _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);

                    SConversion conv;
                    _wmakepath_s(conv.szSrc, drive, dir, findData.cFileName, nullptr);
                    files.push_back(conv);
                }

                if (!FindNextFile(hFile.get(), &findData))
                    break;
            }
        }

        // Process directories
        if (recursive)
        {
            wchar_t searchDir[MAX_PATH] = {};
            {
                wchar_t drive[_MAX_DRIVE] = {};
                wchar_t dir[_MAX_DIR] = {};
                _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
                _wmakepath_s(searchDir, drive, dir, L"*", nullptr);
            }

            hFile.reset(safe_handle(FindFirstFileExW(searchDir,
                FindExInfoBasic, &findData,
                FindExSearchLimitToDirectories, nullptr,
                FIND_FIRST_EX_LARGE_FETCH)));
            if (!hFile)
                return;

            for (;;)
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (findData.cFileName[0] != L'.')
                    {
                        wchar_t subdir[MAX_PATH] = {};

                        {
                            wchar_t drive[_MAX_DRIVE] = {};
                            wchar_t dir[_MAX_DIR] = {};
                            wchar_t fname[_MAX_FNAME] = {};
                            wchar_t ext[_MAX_FNAME] = {};
                            _wsplitpath_s(path, drive, dir, fname, ext);
                            wcscat_s(dir, findData.cFileName);
                            _wmakepath_s(subdir, drive, dir, fname, ext);
                        }

                        SearchForFiles(subdir, files, recursive);
                    }
                }

                if (!FindNextFile(hFile.get(), &findData))
                    break;
            }
        }
    }


    void PrintFormat(DXGI_FORMAT Format)
    {
        for (const SValue *pFormat = g_pFormats; pFormat->pName; pFormat++)
        {
            if ((DXGI_FORMAT)pFormat->dwValue == Format)
            {
                wprintf(pFormat->pName);
                return;
            }
        }

        for (const SValue *pFormat = g_pReadOnlyFormats; pFormat->pName; pFormat++)
        {
            if ((DXGI_FORMAT)pFormat->dwValue == Format)
            {
                wprintf(pFormat->pName);
                return;
            }
        }

        wprintf(L"*UNKNOWN*");
    }


    void PrintInfo(const TexMetadata& info)
    {
        wprintf(L" (%Iux%Iu", info.width, info.height);

        if (TEX_DIMENSION_TEXTURE3D == info.dimension)
            wprintf(L"x%Iu", info.depth);

        if (info.mipLevels > 1)
            wprintf(L",%Iu", info.mipLevels);

        if (info.arraySize > 1)
            wprintf(L",%Iu", info.arraySize);

        wprintf(L" ");
        PrintFormat(info.format);

        switch (info.dimension)
        {
        case TEX_DIMENSION_TEXTURE1D:
            wprintf((info.arraySize > 1) ? L" 1DArray" : L" 1D");
            break;

        case TEX_DIMENSION_TEXTURE2D:
            if (info.IsCubemap())
            {
                wprintf((info.arraySize > 6) ? L" CubeArray" : L" Cube");
            }
            else
            {
                wprintf((info.arraySize > 1) ? L" 2DArray" : L" 2D");
            }
            break;

        case TEX_DIMENSION_TEXTURE3D:
            wprintf(L" 3D");
            break;
        }

        switch (info.GetAlphaMode())
        {
        case TEX_ALPHA_MODE_OPAQUE:
            wprintf(L" \x0e0:Opaque");
            break;
        case TEX_ALPHA_MODE_PREMULTIPLIED:
            wprintf(L" \x0e0:PM");
            break;
        case TEX_ALPHA_MODE_STRAIGHT:
            wprintf(L" \x0e0:NonPM");
            break;
        }

        wprintf(L")");
    }


    void PrintList(size_t cch, const SValue *pValue)
    {
        while (pValue->pName)
        {
            size_t cchName = wcslen(pValue->pName);

            if (cch + cchName + 2 >= 80)
            {
                wprintf(L"\n      ");
                cch = 6;
            }

            wprintf(L"%ls ", pValue->pName);
            cch += cchName + 2;
            pValue++;
        }

        wprintf(L"\n");
    }


    void PrintLogo()
    {
        wprintf(L"Microsoft (R) DirectX Texture Converter (DirectXTex version)\n");
        wprintf(L"Copyright (C) Microsoft Corp. All rights reserved.\n");
#ifdef _DEBUG
        wprintf(L"*** Debug build ***\n");
#endif
        wprintf(L"\n");
    }


    _Success_(return != false)
        bool GetDXGIFactory(_Outptr_ IDXGIFactory1** pFactory)
    {
        if (!pFactory)
            return false;

        *pFactory = nullptr;

        typedef HRESULT(WINAPI* pfn_CreateDXGIFactory1)(REFIID riid, _Out_ void **ppFactory);

        static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

        if (!s_CreateDXGIFactory1)
        {
            HMODULE hModDXGI = LoadLibrary(L"dxgi.dll");
            if (!hModDXGI)
                return false;

            s_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));
            if (!s_CreateDXGIFactory1)
                return false;
        }

        return SUCCEEDED(s_CreateDXGIFactory1(IID_PPV_ARGS(pFactory)));
    }


    void PrintUsage()
    {
        PrintLogo();

        wprintf(L"Usage: texconv <options> <files>\n\n");
        wprintf(L"   -r                  wildcard filename search is recursive\n");
        wprintf(L"   -w <n>              width\n");
        wprintf(L"   -h <n>              height\n");
        wprintf(L"   -m <n>              miplevels\n");
        wprintf(L"   -f <format>         format\n");
        wprintf(L"   -if <filter>        image filtering\n");
        wprintf(L"   -srgb{i|o}          sRGB {input, output}\n");
        wprintf(L"   -px <string>        name prefix\n");
        wprintf(L"   -sx <string>        name suffix\n");
        wprintf(L"   -o <directory>      output directory\n");
        wprintf(L"   -y                  overwrite existing output file (if any)\n");
        wprintf(L"   -ft <filetype>      output file type\n");
        wprintf(L"   -hflip              horizonal flip of source image\n");
        wprintf(L"   -vflip              vertical flip of source image\n");
        wprintf(L"   -sepalpha           resize/generate mips alpha channel separately\n");
        wprintf(L"                       from color channels\n");
        wprintf(L"   -wrap, -mirror      texture addressing mode (wrap, mirror, or clamp)\n");
        wprintf(L"   -pmalpha            convert final texture to use premultiplied alpha\n");
        wprintf(L"   -alpha              convert premultiplied alpha to straight alpha\n");
        wprintf(L"   -pow2               resize to fit a power-of-2, respecting aspect ratio\n");
        wprintf(
            L"   -nmap <options>     converts height-map to normal-map\n"
            L"                       options must be one or more of\n"
            L"                          r, g, b, a, l, m, u, v, i, o\n");
        wprintf(L"   -nmapamp <weight>   normal map amplitude (defaults to 1.0)\n");
        wprintf(L"   -fl <feature-level> Set maximum feature level target (defaults to 11.0)\n");
        wprintf(L"\n                       (DDS input only)\n");
        wprintf(L"   -t{u|f}             TYPELESS format is treated as UNORM or FLOAT\n");
        wprintf(L"   -dword              Use DWORD instead of BYTE alignment\n");
        wprintf(L"   -badtails           Fix for older DXTn with bad mipchain tails\n");
        wprintf(L"   -xlum               expand legacy L8, L16, and A8P8 formats\n");
        wprintf(L"\n                       (DDS output only)\n");
        wprintf(L"   -dx10               Force use of 'DX10' extended header\n");
        wprintf(L"\n   -nologo             suppress copyright message\n");
        wprintf(L"   -timing             Display elapsed processing time\n\n");
#ifdef _OPENMP
        wprintf(L"   -singleproc         Do not use multi-threaded compression\n");
#endif
        wprintf(L"   -gpu <adapter>      Select GPU for DirectCompute-based codecs (0 is default)\n");
        wprintf(L"   -nogpu              Do not use DirectCompute-based codecs\n");
        wprintf(L"   -bcuniform          Use uniform rather than perceptual weighting for BC1-3\n");
        wprintf(L"   -bcdither           Use dithering for BC1-3\n");
        wprintf(L"   -bcmax              Use exhaustive compression (BC7 only)\n");
        wprintf(L"   -bcquick            Use quick compression (BC7 only)\n");
        wprintf(L"   -wicq <quality>     When writing images with WIC use quality (0.0 to 1.0)\n");
        wprintf(L"   -wiclossless        When writing images with WIC use lossless mode\n");
        wprintf(L"   -wicmulti           When writing images with WIC encode multiframe images\n");
        wprintf(
            L"   -aw <weight>        BC7 GPU compressor weighting for alpha error metric\n"
            L"                       (defaults to 1.0)\n");
        wprintf(L"   -c <hex-RGB>        colorkey (a.k.a. chromakey) transparency\n");
        wprintf(L"   -tonemap            Apply a tonemap operator based on maximum luminance\n");
        wprintf(L"   -x2bias             Enable *2 - 1 conversion cases for unorm/pos-only-float\n");

        wprintf(L"\n   <format>: ");
        PrintList(13, g_pFormats);

        wprintf(L"\n   <filter>: ");
        PrintList(13, g_pFilters);

        wprintf(L"\n   <filetype>: ");
        PrintList(15, g_pSaveFileTypes);

        wprintf(L"\n   <feature-level>: ");
        PrintList(13, g_pFeatureLevels);

        ComPtr<IDXGIFactory1> dxgiFactory;
        if (GetDXGIFactory(dxgiFactory.GetAddressOf()))
        {
            wprintf(L"\n   <adapter>:\n");

            ComPtr<IDXGIAdapter> adapter;
            for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters(adapterIndex, adapter.ReleaseAndGetAddressOf()); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                {
                    wprintf(L"      %u: VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                }
            }
        }
    }


    _Success_(return != false)
        bool CreateDevice(int adapter, _Outptr_ ID3D11Device** pDevice)
    {
        if (!pDevice)
            return false;

        *pDevice = nullptr;

        static PFN_D3D11_CREATE_DEVICE s_DynamicD3D11CreateDevice = nullptr;

        if (!s_DynamicD3D11CreateDevice)
        {
            HMODULE hModD3D11 = LoadLibrary(L"d3d11.dll");
            if (!hModD3D11)
                return false;

            s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D11, "D3D11CreateDevice")));
            if (!s_DynamicD3D11CreateDevice)
                return false;
        }

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        ComPtr<IDXGIAdapter> pAdapter;
        if (adapter >= 0)
        {
            ComPtr<IDXGIFactory1> dxgiFactory;
            if (GetDXGIFactory(dxgiFactory.GetAddressOf()))
            {
                if (FAILED(dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf())))
                {
                    wprintf(L"\nERROR: Invalid GPU adapter index (%d)!\n", adapter);
                    return false;
                }
            }
        }

        D3D_FEATURE_LEVEL fl;
        HRESULT hr = s_DynamicD3D11CreateDevice(pAdapter.Get(),
            (pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
            nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
            D3D11_SDK_VERSION, pDevice, &fl, nullptr);
        if (SUCCEEDED(hr))
        {
            if (fl < D3D_FEATURE_LEVEL_11_0)
            {
                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
                hr = (*pDevice)->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
                if (FAILED(hr))
                    memset(&hwopts, 0, sizeof(hwopts));

                if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
                {
                    if (*pDevice)
                    {
                        (*pDevice)->Release();
                        *pDevice = nullptr;
                    }
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            hr = (*pDevice)->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                hr = dxgiDevice->GetAdapter(pAdapter.ReleaseAndGetAddressOf());
                if (SUCCEEDED(hr))
                {
                    DXGI_ADAPTER_DESC desc;
                    hr = pAdapter->GetDesc(&desc);
                    if (SUCCEEDED(hr))
                    {
                        wprintf(L"\n[Using DirectCompute on \"%ls\"]\n", desc.Description);
                    }
                }
            }

            return true;
        }
        else
            return false;
    }


    void FitPowerOf2(size_t origx, size_t origy, size_t& targetx, size_t& targety, size_t maxsize)
    {
        float origAR = float(origx) / float(origy);

        if (origx > origy)
        {
            size_t x;
            for (x = maxsize; x > 1; x >>= 1) { if (x <= targetx) break; };
            targetx = x;

            float bestScore = FLT_MAX;
            for (size_t y = maxsize; y > 0; y >>= 1)
            {
                float score = fabs((float(x) / float(y)) - origAR);
                if (score < bestScore)
                {
                    bestScore = score;
                    targety = y;
                }
            }
        }
        else
        {
            size_t y;
            for (y = maxsize; y > 1; y >>= 1) { if (y <= targety) break; };
            targety = y;

            float bestScore = FLT_MAX;
            for (size_t x = maxsize; x > 0; x >>= 1)
            {
                float score = fabs((float(x) / float(y)) - origAR);
                if (score < bestScore)
                {
                    bestScore = score;
                    targetx = x;
                }
            }
        }
    }
}


