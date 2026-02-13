//--------------------------------------------------------------------------------------
// File: Texdiag.cpp
//
// DirectX Texture diagnostic tool
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
#include <vector>

#include <dxgiformat.h>

#include "directxtex.h"

//Uncomment to add support for OpenEXR (.exr)
//#define USE_OPENEXR

#ifdef USE_OPENEXR
// See <https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR> for details
#include "DirectXTexEXR.h"
#endif

using namespace DirectX;

enum COMMANDS
{
    CMD_INFO = 1,
    CMD_ANALYZE,
    CMD_COMPARE,
    CMD_DIFF,
    CMD_DUMPBC,
    CMD_MAX
};

enum OPTIONS
{
    OPT_RECURSIVE = 1,
    OPT_FORMAT,
    OPT_FILTER,
    OPT_DDS_DWORD_ALIGN,
    OPT_DDS_BAD_DXTN_TAILS,
    OPT_OUTPUTFILE,
    OPT_OVERWRITE,
    OPT_NOLOGO,
    OPT_TYPELESS_UNORM,
    OPT_TYPELESS_FLOAT,
    OPT_EXPAND_LUMINANCE,
    OPT_TARGET_PIXELX,
    OPT_TARGET_PIXELY,
    OPT_MAX
};

static_assert(OPT_MAX <= 32, "dwOptions is a DWORD bitfield");

struct SConversion
{
    wchar_t szSrc[MAX_PATH];
};

struct SValue
{
    LPCWSTR pName;
    DWORD dwValue;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const SValue g_pCommands[] =
{
    { L"info",      CMD_INFO },
    { L"analyze",   CMD_ANALYZE },
    { L"compare",   CMD_COMPARE },
    { L"diff",      CMD_DIFF },
    { L"dumpbc",    CMD_DUMPBC },
    { nullptr,      0 }
};

const SValue g_pOptions[] =
{
    { L"r",         OPT_RECURSIVE },
    { L"f",         OPT_FORMAT },
    { L"if",        OPT_FILTER },
    { L"dword",     OPT_DDS_DWORD_ALIGN },
    { L"badtails",  OPT_DDS_BAD_DXTN_TAILS },
    { L"nologo",    OPT_NOLOGO },
    { L"o",         OPT_OUTPUTFILE },
    { L"y",         OPT_OVERWRITE },
    { L"tu",        OPT_TYPELESS_UNORM },
    { L"tf",        OPT_TYPELESS_FLOAT },
    { L"xlum",      OPT_EXPAND_LUMINANCE },
    { L"targetx",   OPT_TARGET_PIXELX },
    { L"targety",   OPT_TARGET_PIXELY },
    { nullptr,      0 }
};

#define DEFFMT(fmt) { L#fmt, DXGI_FORMAT_ ## fmt }

const SValue g_pFormats[] =
{
    // List does not include _TYPELESS, depth/stencil, or BC formats
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
    DEFFMT(B5G6R5_UNORM),
    DEFFMT(B5G5R5A1_UNORM),

    // DXGI 1.1 formats
    DEFFMT(B8G8R8A8_UNORM),
    DEFFMT(B8G8R8X8_UNORM),
    DEFFMT(R10G10B10_XR_BIAS_A2_UNORM),
    DEFFMT(B8G8R8A8_UNORM_SRGB),
    DEFFMT(B8G8R8X8_UNORM_SRGB),

    // DXGI 1.2 formats
    DEFFMT(AYUV),
    DEFFMT(Y410),
    DEFFMT(Y416),
    DEFFMT(YUY2),
    DEFFMT(Y210),
    DEFFMT(Y216),
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
    DEFFMT(BC1_UNORM),
    DEFFMT(BC1_UNORM_SRGB),
    DEFFMT(BC2_TYPELESS),
    DEFFMT(BC2_UNORM),
    DEFFMT(BC2_UNORM_SRGB),
    DEFFMT(BC3_TYPELESS),
    DEFFMT(BC3_UNORM),
    DEFFMT(BC3_UNORM_SRGB),
    DEFFMT(BC4_TYPELESS),
    DEFFMT(BC4_UNORM),
    DEFFMT(BC4_SNORM),
    DEFFMT(BC5_TYPELESS),
    DEFFMT(BC5_UNORM),
    DEFFMT(BC5_SNORM),

    // DXGI 1.1 formats
    DEFFMT(B8G8R8A8_TYPELESS),
    DEFFMT(B8G8R8X8_TYPELESS),
    DEFFMT(BC6H_TYPELESS),
    DEFFMT(BC6H_UF16),
    DEFFMT(BC6H_SF16),
    DEFFMT(BC7_TYPELESS),
    DEFFMT(BC7_UNORM),
    DEFFMT(BC7_UNORM_SRGB),

    // DXGI 1.2 formats
    DEFFMT(AI44),
    DEFFMT(IA44),
    DEFFMT(P8),
    DEFFMT(A8P8),
    DEFFMT(NV12),
    DEFFMT(P010),
    DEFFMT(P016),
    DEFFMT(420_OPAQUE),
    DEFFMT(NV11),

    // DXGI 1.3 formats
    { L"P208", DXGI_FORMAT(130) },
    { L"V208", DXGI_FORMAT(131) },
    { L"V408", DXGI_FORMAT(132) },

    // Xbox-specific formats
    { L"R10G10B10_7E3_A2_FLOAT (Xbox)", DXGI_FORMAT(116) },
    { L"R10G10B10_6E4_A2_FLOAT (Xbox)", DXGI_FORMAT(117) },
    { L"D16_UNORM_S8_UINT (Xbox)", DXGI_FORMAT(118) },
    { L"R16_UNORM_X8_TYPELESS (Xbox)", DXGI_FORMAT(119) },
    { L"X16_TYPELESS_G8_UINT (Xbox)", DXGI_FORMAT(120) },
    { L"R10G10B10_SNORM_A2_UNORM (Xbox)", DXGI_FORMAT(189) },
    { L"R4G4_UNORM (Xbox)", DXGI_FORMAT(190) },

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
    { nullptr,                      TEX_FILTER_DEFAULT }
};

#define CODEC_DDS 0xFFFF0001 
#define CODEC_TGA 0xFFFF0002
#define CODEC_HDR 0xFFFF0005
#define CODEC_EXR 0xFFFF0006

const SValue g_pExtFileTypes[] =
{
    { L".BMP",  WIC_CODEC_BMP },
    { L".JPG",  WIC_CODEC_JPEG },
    { L".JPEG", WIC_CODEC_JPEG },
    { L".PNG",  WIC_CODEC_PNG },
    { L".DDS",  CODEC_DDS },
    { L".TGA",  CODEC_TGA },
    { L".HDR",  CODEC_HDR },
    { L".TIF",  WIC_CODEC_TIFF },
    { L".TIFF", WIC_CODEC_TIFF },
    { L".WDP",  WIC_CODEC_WMP },
    { L".HDP",  WIC_CODEC_WMP },
    { L".JXR",  WIC_CODEC_WMP },
#ifdef USE_OPENEXR
    { L"EXR",   CODEC_EXR },
#endif
    { nullptr,  CODEC_DDS }
};

