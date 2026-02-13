//-------------------------------------------------------------------------------------
// DirectXTexConvert.cpp
//  
// DirectX Texture Library - Image pixel format conversion 
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//-------------------------------------------------------------------------------------

#include "directxtexp.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

namespace
{
    inline uint32_t FloatTo7e3(float Value)
    {
        uint32_t IValue = reinterpret_cast<uint32_t *>(&Value)[0];

        if (IValue & 0x80000000U)
        {
            // Positive only
            return 0;
        }
        else if (IValue > 0x41FF73FFU)
        {
            // The number is too large to be represented as a 7e3. Saturate.
            return 0x3FFU;
        }
        else
        {
            if (IValue < 0x3E800000U)
            {
                // The number is too small to be represented as a normalized 7e3.
                // Convert it to a denormalized value.
                uint32_t Shift = 125U - (IValue >> 23U);
                IValue = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
            }
            else
            {
                // Rebias the exponent to represent the value as a normalized 7e3.
                IValue += 0xC2000000U;
            }

            return ((IValue + 0x7FFFU + ((IValue >> 16U) & 1U)) >> 16U) & 0x3FFU;
        }
    }

    inline float FloatFrom7e3(uint32_t Value)
    {
        uint32_t Mantissa = (uint32_t)(Value & 0x7F);

        uint32_t Exponent = (Value & 0x380);
        if (Exponent != 0)  // The value is normalized
        {
            Exponent = (uint32_t)((Value >> 7) & 0x7);
        }
        else if (Mantissa != 0)     // The value is denormalized
        {
            // Normalize the value in the resulting float
            Exponent = 1;

            do
            {
                Exponent--;
                Mantissa <<= 1;
            } while ((Mantissa & 0x80) == 0);

            Mantissa &= 0x7F;
        }
        else                        // The value is zero
        {
            Exponent = (uint32_t)-124;
        }

        uint32_t Result = ((Exponent + 124) << 23) | // Exponent
            (Mantissa << 16);          // Mantissa

        return reinterpret_cast<float*>(&Result)[0];
    }

    inline uint32_t FloatTo6e4(float Value)
    {
        uint32_t IValue = reinterpret_cast<uint32_t *>(&Value)[0];

        if (IValue & 0x80000000U)
        {
            // Positive only
            return 0;
        }
        else if (IValue > 0x43FEFFFFU)
        {
            // The number is too large to be represented as a 6e4. Saturate.
            return 0x3FFU;
        }
        else
        {
            if (IValue < 0x3C800000U)
            {
                // The number is too small to be represented as a normalized 6e4.
                // Convert it to a denormalized value.
                uint32_t Shift = 121U - (IValue >> 23U);
                IValue = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
            }
            else
            {
                // Rebias the exponent to represent the value as a normalized 6e4.
                IValue += 0xC4000000U;
            }

            return ((IValue + 0xFFFFU + ((IValue >> 17U) & 1U)) >> 17U) & 0x3FFU;
        }
    }

    inline float FloatFrom6e4(uint32_t Value)
    {
        uint32_t Mantissa = (uint32_t)(Value & 0x3F);

        uint32_t Exponent = (Value & 0x3C0);
        if (Exponent != 0)  // The value is normalized
        {
            Exponent = (uint32_t)((Value >> 6) & 0xF);
        }
        else if (Mantissa != 0)     // The value is denormalized
        {
            // Normalize the value in the resulting float
            Exponent = 1;

            do
            {
                Exponent--;
                Mantissa <<= 1;
            } while ((Mantissa & 0x40) == 0);

            Mantissa &= 0x3F;
        }
        else                        // The value is zero
        {
            Exponent = (uint32_t)-120;
        }

        uint32_t Result = ((Exponent + 120) << 23) | // Exponent
            (Mantissa << 17);          // Mantissa

        return reinterpret_cast<float*>(&Result)[0];
    }

#if DIRECTX_MATH_VERSION >= 310
#define StoreFloat3SE XMStoreFloat3SE
#else
    inline void XM_CALLCONV StoreFloat3SE(_Out_ XMFLOAT3SE* pDestination, DirectX::FXMVECTOR V)
    {
        assert(pDestination);

        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, V);

        static const float maxf9 = float(0x1FF << 7);
        static const float minf9 = float(1.f / (1 << 16));

        float x = (tmp.x >= 0.f) ? ((tmp.x > maxf9) ? maxf9 : tmp.x) : 0.f;
        float y = (tmp.y >= 0.f) ? ((tmp.y > maxf9) ? maxf9 : tmp.y) : 0.f;
        float z = (tmp.z >= 0.f) ? ((tmp.z > maxf9) ? maxf9 : tmp.z) : 0.f;

        const float max_xy = (x > y) ? x : y;
        const float max_xyz = (max_xy > z) ? max_xy : z;

        const float maxColor = (max_xyz > minf9) ? max_xyz : minf9;

        union { float f; int32_t i; } fi;
        fi.f = maxColor;
        fi.i += 0x00004000; // round up leaving 9 bits in fraction (including assumed 1)

        // Fix applied from DirectXMath 3.10
        uint32_t exp = fi.i >> 23;
        pDestination->e = exp - 0x6f;

        fi.i = 0x83000000 - (exp << 23);
        float ScaleR = fi.f;

        pDestination->xm = static_cast<uint32_t>(lroundf(x * ScaleR));
        pDestination->ym = static_cast<uint32_t>(lroundf(y * ScaleR));
        pDestination->zm = static_cast<uint32_t>(lroundf(z * ScaleR));
    }
#endif

    const XMVECTORF32 g_Grayscale = { { { 0.2125f, 0.7154f, 0.0721f, 0.0f } } };
    const XMVECTORF32 g_HalfMin   = { { { -65504.f, -65504.f, -65504.f, -65504.f } } };
    const XMVECTORF32 g_HalfMax   = { { { 65504.f, 65504.f, 65504.f, 65504.f } } };
    const XMVECTORF32 g_8BitBias  = { { { 0.5f / 255.f, 0.5f / 255.f, 0.5f / 255.f, 0.5f / 255.f } } };
}

//-------------------------------------------------------------------------------------
// Copies an image row with optional clearing of alpha value to 1.0
// (can be used in place as well) otherwise copies the image row unmodified.
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void DirectX::_CopyScanline(
    void* pDestination,
    size_t outSize,
    const void* pSource,
    size_t inSize,
    DXGI_FORMAT format,
    DWORD flags)
{
    assert(pDestination && outSize > 0);
    assert(pSource && inSize > 0);
    assert(IsValid(format) && !IsPalettized(format));

    if (flags & TEXP_SCANLINE_SETALPHA)
    {
        switch (static_cast<int>(format))
        {
            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            if (inSize >= 16 && outSize >= 16)
            {
                uint32_t alpha;
                if (format == DXGI_FORMAT_R32G32B32A32_FLOAT)
                    alpha = 0x3f800000;
                else if (format == DXGI_FORMAT_R32G32B32A32_SINT)
                    alpha = 0x7fffffff;
                else
                    alpha = 0xffffffff;

                if (pDestination == pSource)
                {
                    uint32_t *dPtr = reinterpret_cast<uint32_t*> (pDestination);
                    for (size_t count = 0; count < (outSize - 15); count += 16)
                    {
                        dPtr += 3;
                        *(dPtr++) = alpha;
                    }
                }
                else
                {
                    const uint32_t * __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
                    uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 15); count += 16)
                    {
                        *(dPtr++) = *(sPtr++);
                        *(dPtr++) = *(sPtr++);
                        *(dPtr++) = *(sPtr++);
                        *(dPtr++) = alpha;
                        ++sPtr;
                    }
                }
            }
            return;

            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_Y416:
            if (inSize >= 8 && outSize >= 8)
            {
                uint16_t alpha;
                if (format == DXGI_FORMAT_R16G16B16A16_FLOAT)
                    alpha = 0x3c00;
                else if (format == DXGI_FORMAT_R16G16B16A16_SNORM || format == DXGI_FORMAT_R16G16B16A16_SINT)
                    alpha = 0x7fff;
                else
                    alpha = 0xffff;

                if (pDestination == pSource)
                {
                    uint16_t *dPtr = reinterpret_cast<uint16_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 7); count += 8)
                    {
                        dPtr += 3;
                        *(dPtr++) = alpha;
                    }
                }
                else
                {
                    const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
                    uint16_t * __restrict dPtr = reinterpret_cast<uint16_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 7); count += 8)
                    {
                        *(dPtr++) = *(sPtr++);
                        *(dPtr++) = *(sPtr++);
                        *(dPtr++) = *(sPtr++);
                        *(dPtr++) = alpha;
                        ++sPtr;
                    }
                }
            }
            return;

            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_Y410:
        case XBOX_DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
        case XBOX_DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
        case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
            if (inSize >= 4 && outSize >= 4)
            {
                if (pDestination == pSource)
                {
                    uint32_t *dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 3); count += 4)
                    {
                        *dPtr |= 0xC0000000;
                        ++dPtr;
                    }
                }
                else
                {
                    const uint32_t * __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
                    uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 3); count += 4)
                    {
                        *(dPtr++) = *(sPtr++) | 0xC0000000;
                    }
                }
            }
            return;

            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
            if (inSize >= 4 && outSize >= 4)
            {
                const uint32_t alpha = (format == DXGI_FORMAT_R8G8B8A8_SNORM || format == DXGI_FORMAT_R8G8B8A8_SINT) ? 0x7f000000 : 0xff000000;

                if (pDestination == pSource)
                {
                    uint32_t *dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 3); count += 4)
                    {
                        uint32_t t = *dPtr & 0xFFFFFF;
                        t |= alpha;
                        *(dPtr++) = t;
                    }
                }
                else
                {
                    const uint32_t * __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
                    uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 3); count += 4)
                    {
                        uint32_t t = *(sPtr++) & 0xFFFFFF;
                        t |= alpha;
                        *(dPtr++) = t;
                    }
                }
            }
            return;

            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            if (inSize >= 2 && outSize >= 2)
            {
                if (pDestination == pSource)
                {
                    uint16_t *dPtr = reinterpret_cast<uint16_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 1); count += 2)
                    {
                        *(dPtr++) |= 0x8000;
                    }
                }
                else
                {
                    const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
                    uint16_t * __restrict dPtr = reinterpret_cast<uint16_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 1); count += 2)
                    {
                        *(dPtr++) = *(sPtr++) | 0x8000;
                    }
                }
            }
            return;

            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_A8_UNORM:
            memset(pDestination, 0xff, outSize);
            return;

            //-----------------------------------------------------------------------------
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            if (inSize >= 2 && outSize >= 2)
            {
                if (pDestination == pSource)
                {
                    uint16_t *dPtr = reinterpret_cast<uint16_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 1); count += 2)
                    {
                        *(dPtr++) |= 0xF000;
                    }
                }
                else
                {
                    const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
                    uint16_t * __restrict dPtr = reinterpret_cast<uint16_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 1); count += 2)
                    {
                        *(dPtr++) = *(sPtr++) | 0xF000;
                    }
                }
            }
            return;
        }
    }

    // Fall-through case is to just use memcpy (assuming this is not an in-place operation)
    if (pDestination == pSource)
        return;

    size_t size = std::min<size_t>(outSize, inSize);
    memcpy_s(pDestination, outSize, pSource, size);
}


//-------------------------------------------------------------------------------------
// Swizzles (RGB <-> BGR) an image row with optional clearing of alpha value to 1.0
// (can be used in place as well) otherwise copies the image row unmodified.
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void DirectX::_SwizzleScanline(
    void* pDestination,
    size_t outSize,
    const void* pSource,
    size_t inSize,
    DXGI_FORMAT format,
    DWORD flags)
{
    assert(pDestination && outSize > 0);
    assert(pSource && inSize > 0);
    assert(IsValid(format) && !IsPlanar(format) && !IsPalettized(format));

    switch (static_cast<int>(format))
    {
        //---------------------------------------------------------------------------------
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
        if (inSize >= 4 && outSize >= 4)
        {
            if (flags & TEXP_SCANLINE_LEGACY)
            {
                // Swap Red (R) and Blue (B) channel (used for D3DFMT_A2R10G10B10 legacy sources)
                if (pDestination == pSource)
                {
                    uint32_t *dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 3); count += 4)
                    {
                        uint32_t t = *dPtr;

                        uint32_t t1 = (t & 0x3ff00000) >> 20;
                        uint32_t t2 = (t & 0x000003ff) << 20;
                        uint32_t t3 = (t & 0x000ffc00);
                        uint32_t ta = (flags & TEXP_SCANLINE_SETALPHA) ? 0xC0000000 : (t & 0xC0000000);

                        *(dPtr++) = t1 | t2 | t3 | ta;
                    }
                }
                else
                {
                    const uint32_t * __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
                    uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 3); count += 4)
                    {
                        uint32_t t = *(sPtr++);

                        uint32_t t1 = (t & 0x3ff00000) >> 20;
                        uint32_t t2 = (t & 0x000003ff) << 20;
                        uint32_t t3 = (t & 0x000ffc00);
                        uint32_t ta = (flags & TEXP_SCANLINE_SETALPHA) ? 0xC0000000 : (t & 0xC0000000);

                        *(dPtr++) = t1 | t2 | t3 | ta;
                    }
                }
                return;
            }
        }
        break;

        //---------------------------------------------------------------------------------
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        if (inSize >= 4 && outSize >= 4)
        {
            // Swap Red (R) and Blue (B) channels (used to convert from DXGI 1.1 BGR formats to DXGI 1.0 RGB)
            if (pDestination == pSource)
            {
                uint32_t *dPtr = reinterpret_cast<uint32_t*>(pDestination);
                for (size_t count = 0; count < (outSize - 3); count += 4)
                {
                    uint32_t t = *dPtr;

                    uint32_t t1 = (t & 0x00ff0000) >> 16;
                    uint32_t t2 = (t & 0x000000ff) << 16;
                    uint32_t t3 = (t & 0x0000ff00);
                    uint32_t ta = (flags & TEXP_SCANLINE_SETALPHA) ? 0xff000000 : (t & 0xFF000000);

                    *(dPtr++) = t1 | t2 | t3 | ta;
                }
            }
            else
            {
                const uint32_t * __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
                uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
                size_t size = std::min<size_t>(outSize, inSize);
                for (size_t count = 0; count < (size - 3); count += 4)
                {
                    uint32_t t = *(sPtr++);

                    uint32_t t1 = (t & 0x00ff0000) >> 16;
                    uint32_t t2 = (t & 0x000000ff) << 16;
                    uint32_t t3 = (t & 0x0000ff00);
                    uint32_t ta = (flags & TEXP_SCANLINE_SETALPHA) ? 0xff000000 : (t & 0xFF000000);

                    *(dPtr++) = t1 | t2 | t3 | ta;
                }
            }
            return;
        }
        break;

        //---------------------------------------------------------------------------------
    case DXGI_FORMAT_YUY2:
        if (inSize >= 4 && outSize >= 4)
        {
            if (flags & TEXP_SCANLINE_LEGACY)
            {
                // Reorder YUV components (used to convert legacy UYVY -> YUY2)
                if (pDestination == pSource)
                {
                    uint32_t *dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    for (size_t count = 0; count < (outSize - 3); count += 4)
                    {
                        uint32_t t = *dPtr;

                        uint32_t t1 = (t & 0x000000ff) << 8;
                        uint32_t t2 = (t & 0x0000ff00) >> 8;
                        uint32_t t3 = (t & 0x00ff0000) << 8;
                        uint32_t t4 = (t & 0xff000000) >> 8;

                        *(dPtr++) = t1 | t2 | t3 | t4;
                    }
                }
                else
                {
                    const uint32_t * __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
                    uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
                    size_t size = std::min<size_t>(outSize, inSize);
                    for (size_t count = 0; count < (size - 3); count += 4)
                    {
                        uint32_t t = *(sPtr++);

                        uint32_t t1 = (t & 0x000000ff) << 8;
                        uint32_t t2 = (t & 0x0000ff00) >> 8;
                        uint32_t t3 = (t & 0x00ff0000) << 8;
                        uint32_t t4 = (t & 0xff000000) >> 8;

                        *(dPtr++) = t1 | t2 | t3 | t4;
                    }
                }
                return;
            }
        }
        break;
    }

    // Fall-through case is to just use memcpy (assuming this is not an in-place operation)
    if (pDestination == pSource)
        return;

    size_t size = std::min<size_t>(outSize, inSize);
    memcpy_s(pDestination, outSize, pSource, size);
}


//-------------------------------------------------------------------------------------
// Converts an image row with optional clearing of alpha value to 1.0
// Returns true if supported, false if expansion case not supported
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
bool DirectX::_ExpandScanline(
    void* pDestination,
    size_t outSize,
    DXGI_FORMAT outFormat,
    const void* pSource,
    size_t inSize,
    DXGI_FORMAT inFormat,
    DWORD flags)
{
    assert(pDestination && outSize > 0);
    assert(pSource && inSize > 0);
    assert(IsValid(outFormat) && !IsPlanar(outFormat) && !IsPalettized(outFormat));
    assert(IsValid(inFormat) && !IsPlanar(inFormat) && !IsPalettized(inFormat));

    switch (inFormat)
    {
    case DXGI_FORMAT_B5G6R5_UNORM:
        if (outFormat != DXGI_FORMAT_R8G8B8A8_UNORM)
            return false;

        // DXGI_FORMAT_B5G6R5_UNORM -> DXGI_FORMAT_R8G8B8A8_UNORM
        if (inSize >= 2 && outSize >= 4)
        {
            const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
            uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);

            for (size_t ocount = 0, icount = 0; ((icount < (inSize - 1)) && (ocount < (outSize - 3))); icount += 2, ocount += 4)
            {
                uint16_t t = *(sPtr++);

                uint32_t t1 = ((t & 0xf800) >> 8) | ((t & 0xe000) >> 13);
                uint32_t t2 = ((t & 0x07e0) << 5) | ((t & 0x0600) >> 5);
                uint32_t t3 = ((t & 0x001f) << 19) | ((t & 0x001c) << 14);

                *(dPtr++) = t1 | t2 | t3 | 0xff000000;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B5G5R5A1_UNORM:
        if (outFormat != DXGI_FORMAT_R8G8B8A8_UNORM)
            return false;

        // DXGI_FORMAT_B5G5R5A1_UNORM -> DXGI_FORMAT_R8G8B8A8_UNORM
        if (inSize >= 2 && outSize >= 4)
        {
            const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
            uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);

            for (size_t ocount = 0, icount = 0; ((icount < (inSize - 1)) && (ocount < (outSize - 3))); icount += 2, ocount += 4)
            {
                uint16_t t = *(sPtr++);

                uint32_t t1 = ((t & 0x7c00) >> 7) | ((t & 0x7000) >> 12);
                uint32_t t2 = ((t & 0x03e0) << 6) | ((t & 0x0380) << 1);
                uint32_t t3 = ((t & 0x001f) << 19) | ((t & 0x001c) << 14);
                uint32_t ta = (flags & TEXP_SCANLINE_SETALPHA) ? 0xff000000 : ((t & 0x8000) ? 0xff000000 : 0);

                *(dPtr++) = t1 | t2 | t3 | ta;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B4G4R4A4_UNORM:
        if (outFormat != DXGI_FORMAT_R8G8B8A8_UNORM)
            return false;

        // DXGI_FORMAT_B4G4R4A4_UNORM -> DXGI_FORMAT_R8G8B8A8_UNORM
        if (inSize >= 2 && outSize >= 4)
        {
            const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
            uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);

            for (size_t ocount = 0, icount = 0; ((icount < (inSize - 1)) && (ocount < (outSize - 3))); icount += 2, ocount += 4)
            {
                uint16_t t = *(sPtr++);

                uint32_t t1 = ((t & 0x0f00) >> 4) | ((t & 0x0f00) >> 8);
                uint32_t t2 = ((t & 0x00f0) << 8) | ((t & 0x00f0) << 4);
                uint32_t t3 = ((t & 0x000f) << 20) | ((t & 0x000f) << 16);
                uint32_t ta = (flags & TEXP_SCANLINE_SETALPHA) ? 0xff000000 : (((t & 0xf000) << 16) | ((t & 0xf000) << 12));

                *(dPtr++) = t1 | t2 | t3 | ta;
            }
            return true;
        }
        return false;

    default:
        return false;
    }
}


//-------------------------------------------------------------------------------------
// Loads an image row into standard RGBA XMVECTOR (aligned) array
//-------------------------------------------------------------------------------------
#define LOAD_SCANLINE( type, func )\
        if ( size >= sizeof(type) )\
        {\
            const type * __restrict sPtr = reinterpret_cast<const type*>(pSource);\
            for( size_t icount = 0; icount < ( size - sizeof(type) + 1 ); icount += sizeof(type) )\
            {\
                if ( dPtr >= ePtr ) break;\
                *(dPtr++) = func( sPtr++ );\
            }\
            return true;\
        }\
        return false;

#define LOAD_SCANLINE3( type, func, defvec )\
        if ( size >= sizeof(type) )\
        {\
            const type * __restrict sPtr = reinterpret_cast<const type*>(pSource);\
            for( size_t icount = 0; icount < ( size - sizeof(type) + 1 ); icount += sizeof(type) )\
            {\
                XMVECTOR v = func( sPtr++ );\
                if ( dPtr >= ePtr ) break;\
                *(dPtr++) = XMVectorSelect( defvec, v, g_XMSelect1110 );\
            }\
            return true;\
        }\
        return false;

#define LOAD_SCANLINE2( type, func, defvec )\
        if ( size >= sizeof(type) )\
        {\
            const type * __restrict sPtr = reinterpret_cast<const type*>(pSource);\
            for( size_t icount = 0; icount < ( size - sizeof(type) + 1 ); icount += sizeof(type) )\
            {\
                XMVECTOR v = func( sPtr++ );\
                if ( dPtr >= ePtr ) break;\
                *(dPtr++) = XMVectorSelect( defvec, v, g_XMSelect1100 );\
            }\
            return true;\
        }\
        return false;

#pragma warning(suppress: 6101)
_Use_decl_annotations_ bool DirectX::_LoadScanline(
    XMVECTOR* pDestination,
    size_t count,
    const void* pSource,
    size_t size,
    DXGI_FORMAT format)
{
    assert(pDestination && count > 0 && (((uintptr_t)pDestination & 0xF) == 0));
    assert(pSource && size > 0);
    assert(IsValid(format) && !IsTypeless(format, false) && !IsCompressed(format) && !IsPlanar(format) && !IsPalettized(format));

    XMVECTOR* __restrict dPtr = pDestination;
    if (!dPtr)
        return false;

    const XMVECTOR* ePtr = pDestination + count;

    switch (static_cast<int>(format))
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    {
        size_t msize = (size > (sizeof(XMVECTOR)*count)) ? (sizeof(XMVECTOR)*count) : size;
        memcpy_s(dPtr, sizeof(XMVECTOR)*count, pSource, msize);
    }
    return true;

    case DXGI_FORMAT_R32G32B32A32_UINT:
        LOAD_SCANLINE(XMUINT4, XMLoadUInt4)

    case DXGI_FORMAT_R32G32B32A32_SINT:
        LOAD_SCANLINE(XMINT4, XMLoadSInt4)

    case DXGI_FORMAT_R32G32B32_FLOAT:
        LOAD_SCANLINE3(XMFLOAT3, XMLoadFloat3, g_XMIdentityR3)

    case DXGI_FORMAT_R32G32B32_UINT:
        LOAD_SCANLINE3(XMUINT3, XMLoadUInt3, g_XMIdentityR3)

    case DXGI_FORMAT_R32G32B32_SINT:
        LOAD_SCANLINE3(XMINT3, XMLoadSInt3, g_XMIdentityR3)

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        LOAD_SCANLINE(XMHALF4, XMLoadHalf4)

    case DXGI_FORMAT_R16G16B16A16_UNORM:
        LOAD_SCANLINE(XMUSHORTN4, XMLoadUShortN4)

    case DXGI_FORMAT_R16G16B16A16_UINT:
        LOAD_SCANLINE(XMUSHORT4, XMLoadUShort4)

    case DXGI_FORMAT_R16G16B16A16_SNORM:
        LOAD_SCANLINE(XMSHORTN4, XMLoadShortN4)

    case DXGI_FORMAT_R16G16B16A16_SINT:
        LOAD_SCANLINE(XMSHORT4, XMLoadShort4)

    case DXGI_FORMAT_R32G32_FLOAT:
        LOAD_SCANLINE2(XMFLOAT2, XMLoadFloat2, g_XMIdentityR3)

    case DXGI_FORMAT_R32G32_UINT:
        LOAD_SCANLINE2(XMUINT2, XMLoadUInt2, g_XMIdentityR3)

    case DXGI_FORMAT_R32G32_SINT:
        LOAD_SCANLINE2(XMINT2, XMLoadSInt2, g_XMIdentityR3)

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            const size_t psize = sizeof(float) + sizeof(uint32_t);
            if (size >= psize)
            {
                const float * sPtr = reinterpret_cast<const float*>(pSource);
                for (size_t icount = 0; icount < (size - psize + 1); icount += psize)
                {
                    const uint8_t* ps8 = reinterpret_cast<const uint8_t*>(&sPtr[1]);
                    if (dPtr >= ePtr) break;
                    *(dPtr++) = XMVectorSet(sPtr[0], static_cast<float>(*ps8), 0.f, 1.f);
                    sPtr += 2;
                }
                return true;
            }
        }
        return false;

    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    {
        const size_t psize = sizeof(float) + sizeof(uint32_t);
        if (size >= psize)
        {
            const float * sPtr = reinterpret_cast<const float*>(pSource);
            for (size_t icount = 0; icount < (size - psize + 1); icount += psize)
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(sPtr[0], 0.f /* typeless component assumed zero */, 0.f, 1.f);
                sPtr += 2;
            }
            return true;
        }
    }
    return false;

    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    {
        const size_t psize = sizeof(float) + sizeof(uint32_t);
        if (size >= psize)
        {
            const float * sPtr = reinterpret_cast<const float*>(pSource);
            for (size_t icount = 0; icount < (size - psize + 1); icount += psize)
            {
                const uint8_t* pg8 = reinterpret_cast<const uint8_t*>(&sPtr[1]);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(0.f /* typeless component assumed zero */, static_cast<float>(*pg8), 0.f, 1.f);
                sPtr += 2;
            }
            return true;
        }
    }
    return false;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        LOAD_SCANLINE(XMUDECN4, XMLoadUDecN4);

    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        LOAD_SCANLINE(XMUDECN4, XMLoadUDecN4_XR);

    case DXGI_FORMAT_R10G10B10A2_UINT:
        LOAD_SCANLINE(XMUDEC4, XMLoadUDec4);

    case DXGI_FORMAT_R11G11B10_FLOAT:
        LOAD_SCANLINE3(XMFLOAT3PK, XMLoadFloat3PK, g_XMIdentityR3);

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        LOAD_SCANLINE(XMUBYTEN4, XMLoadUByteN4)

    case DXGI_FORMAT_R8G8B8A8_UINT:
        LOAD_SCANLINE(XMUBYTE4, XMLoadUByte4)

    case DXGI_FORMAT_R8G8B8A8_SNORM:
        LOAD_SCANLINE(XMBYTEN4, XMLoadByteN4)

    case DXGI_FORMAT_R8G8B8A8_SINT:
        LOAD_SCANLINE(XMBYTE4, XMLoadByte4)

    case DXGI_FORMAT_R16G16_FLOAT:
        LOAD_SCANLINE2(XMHALF2, XMLoadHalf2, g_XMIdentityR3)

    case DXGI_FORMAT_R16G16_UNORM:
        LOAD_SCANLINE2(XMUSHORTN2, XMLoadUShortN2, g_XMIdentityR3)

    case DXGI_FORMAT_R16G16_UINT:
        LOAD_SCANLINE2(XMUSHORT2, XMLoadUShort2, g_XMIdentityR3)

    case DXGI_FORMAT_R16G16_SNORM:
        LOAD_SCANLINE2(XMSHORTN2, XMLoadShortN2, g_XMIdentityR3)

    case DXGI_FORMAT_R16G16_SINT:
        LOAD_SCANLINE2(XMSHORT2, XMLoadShort2, g_XMIdentityR3)

    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        if (size >= sizeof(float))
        {
            const float* __restrict sPtr = reinterpret_cast<const float*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(float) + 1); icount += sizeof(float))
            {
                XMVECTOR v = XMLoadFloat(sPtr++);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1000);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R32_UINT:
        if (size >= sizeof(uint32_t))
        {
            const uint32_t* __restrict sPtr = reinterpret_cast<const uint32_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint32_t) + 1); icount += sizeof(uint32_t))
            {
                XMVECTOR v = XMLoadInt(sPtr++);
                v = XMConvertVectorUIntToFloat(v, 0);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1000);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R32_SINT:
        if (size >= sizeof(int32_t))
        {
            const int32_t * __restrict sPtr = reinterpret_cast<const int32_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(int32_t) + 1); icount += sizeof(int32_t))
            {
                XMVECTOR v = XMLoadInt(reinterpret_cast<const uint32_t*> (sPtr++));
                v = XMConvertVectorIntToFloat(v, 0);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1000);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        if (size >= sizeof(uint32_t))
        {
            const uint32_t * sPtr = reinterpret_cast<const uint32_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint32_t) + 1); icount += sizeof(uint32_t))
            {
                float d = static_cast<float>(*sPtr & 0xFFFFFF) / 16777215.f;
                float s = static_cast<float>((*sPtr & 0xFF000000) >> 24);
                ++sPtr;
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(d, s, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        if (size >= sizeof(uint32_t))
        {
            const uint32_t * sPtr = reinterpret_cast<const uint32_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint32_t) + 1); icount += sizeof(uint32_t))
            {
                float r = static_cast<float>(*sPtr & 0xFFFFFF) / 16777215.f;
                ++sPtr;
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(r, 0.f /* typeless component assumed zero */, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        if (size >= sizeof(uint32_t))
        {
            const uint32_t * sPtr = reinterpret_cast<const uint32_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint32_t) + 1); icount += sizeof(uint32_t))
            {
                float g = static_cast<float>((*sPtr & 0xFF000000) >> 24);
                ++sPtr;
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(0.f /* typeless component assumed zero */, g, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8G8_UNORM:
        LOAD_SCANLINE2(XMUBYTEN2, XMLoadUByteN2, g_XMIdentityR3)

    case DXGI_FORMAT_R8G8_UINT:
        LOAD_SCANLINE2(XMUBYTE2, XMLoadUByte2, g_XMIdentityR3)

    case DXGI_FORMAT_R8G8_SNORM:
        LOAD_SCANLINE2(XMBYTEN2, XMLoadByteN2, g_XMIdentityR3)

    case DXGI_FORMAT_R8G8_SINT:
        LOAD_SCANLINE2(XMBYTE2, XMLoadByte2, g_XMIdentityR3)

    case DXGI_FORMAT_R16_FLOAT:
        if (size >= sizeof(HALF))
        {
            const HALF * __restrict sPtr = reinterpret_cast<const HALF*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(HALF) + 1); icount += sizeof(HALF))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(XMConvertHalfToFloat(*sPtr++), 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        if (size >= sizeof(uint16_t))
        {
            const uint16_t* __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint16_t) + 1); icount += sizeof(uint16_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++) / 65535.f, 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16_UINT:
        if (size >= sizeof(uint16_t))
        {
            const uint16_t * __restrict sPtr = reinterpret_cast<const uint16_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint16_t) + 1); icount += sizeof(uint16_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++), 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16_SNORM:
        if (size >= sizeof(int16_t))
        {
            const int16_t * __restrict sPtr = reinterpret_cast<const int16_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(int16_t) + 1); icount += sizeof(int16_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++) / 32767.f, 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16_SINT:
        if (size >= sizeof(int16_t))
        {
            const int16_t * __restrict sPtr = reinterpret_cast<const int16_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(int16_t) + 1); icount += sizeof(int16_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++), 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_UNORM:
        if (size >= sizeof(uint8_t))
        {
            const uint8_t * __restrict sPtr = reinterpret_cast<const uint8_t*>(pSource);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++) / 255.f, 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_UINT:
        if (size >= sizeof(uint8_t))
        {
            const uint8_t * __restrict sPtr = reinterpret_cast<const uint8_t*>(pSource);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++), 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_SNORM:
        if (size >= sizeof(int8_t))
        {
            const int8_t * __restrict sPtr = reinterpret_cast<const int8_t*>(pSource);
            for (size_t icount = 0; icount < size; icount += sizeof(int8_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++) / 127.f, 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_SINT:
        if (size >= sizeof(int8_t))
        {
            const int8_t * __restrict sPtr = reinterpret_cast<const int8_t*>(pSource);
            for (size_t icount = 0; icount < size; icount += sizeof(int8_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(static_cast<float>(*sPtr++), 0.f, 0.f, 1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_A8_UNORM:
        if (size >= sizeof(uint8_t))
        {
            const uint8_t * __restrict sPtr = reinterpret_cast<const uint8_t*>(pSource);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(0.f, 0.f, 0.f, static_cast<float>(*sPtr++) / 255.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R1_UNORM:
        if (size >= sizeof(uint8_t))
        {
            const uint8_t * __restrict sPtr = reinterpret_cast<const uint8_t*>(pSource);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                for (size_t bcount = 8; bcount > 0; --bcount)
                {
                    if (dPtr >= ePtr) break;
                    *(dPtr++) = XMVectorSet((((*sPtr >> (bcount - 1)) & 0x1) ? 1.f : 0.f), 0.f, 0.f, 1.f);
                }

                ++sPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        LOAD_SCANLINE3(XMFLOAT3SE, XMLoadFloat3SE, g_XMIdentityR3)

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
        if (size >= sizeof(XMUBYTEN4))
        {
            const XMUBYTEN4 * __restrict sPtr = reinterpret_cast<const XMUBYTEN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                XMVECTOR v = XMLoadUByteN4(sPtr++);
                XMVECTOR v1 = XMVectorSwizzle<0, 3, 2, 1>(v);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1110);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v1, g_XMSelect1110);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        if (size >= sizeof(XMUBYTEN4))
        {
            const XMUBYTEN4 * __restrict sPtr = reinterpret_cast<const XMUBYTEN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                XMVECTOR v = XMLoadUByteN4(sPtr++);
                XMVECTOR v0 = XMVectorSwizzle<1, 0, 3, 2>(v);
                XMVECTOR v1 = XMVectorSwizzle<1, 2, 3, 0>(v);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v0, g_XMSelect1110);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v1, g_XMSelect1110);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B5G6R5_UNORM:
        if (size >= sizeof(XMU565))
        {
            static const XMVECTORF32 s_Scale = { { { 1.f / 31.f, 1.f / 63.f, 1.f / 31.f, 1.f } } };
            const XMU565 * __restrict sPtr = reinterpret_cast<const XMU565*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMU565) + 1); icount += sizeof(XMU565))
            {
                XMVECTOR v = XMLoadU565(sPtr++);
                v = XMVectorMultiply(v, s_Scale);
                v = XMVectorSwizzle<2, 1, 0, 3>(v);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1110);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B5G5R5A1_UNORM:
        if (size >= sizeof(XMU555))
        {
            static const XMVECTORF32 s_Scale = { { { 1.f / 31.f, 1.f / 31.f, 1.f / 31.f, 1.f } } };
            const XMU555 * __restrict sPtr = reinterpret_cast<const XMU555*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMU555) + 1); icount += sizeof(XMU555))
            {
                XMVECTOR v = XMLoadU555(sPtr++);
                v = XMVectorMultiply(v, s_Scale);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSwizzle<2, 1, 0, 3>(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        if (size >= sizeof(XMUBYTEN4))
        {
            const XMUBYTEN4 * __restrict sPtr = reinterpret_cast<const XMUBYTEN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                XMVECTOR v = XMLoadUByteN4(sPtr++);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSwizzle<2, 1, 0, 3>(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        if (size >= sizeof(XMUBYTEN4))
        {
            const XMUBYTEN4 * __restrict sPtr = reinterpret_cast<const XMUBYTEN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                XMVECTOR v = XMLoadUByteN4(sPtr++);
                v = XMVectorSwizzle<2, 1, 0, 3>(v);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1110);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_AYUV:
        if (size >= sizeof(XMUBYTEN4))
        {
            const XMUBYTEN4 * __restrict sPtr = reinterpret_cast<const XMUBYTEN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                int v = int(sPtr->x) - 128;
                int u = int(sPtr->y) - 128;
                int y = int(sPtr->z) - 16;
                unsigned int a = sPtr->w;
                ++sPtr;

                // http://msdn.microsoft.com/en-us/library/windows/desktop/dd206750.aspx

                // Y’  = Y - 16
                // Cb’ = Cb - 128
                // Cr’ = Cr - 128

                // R = 1.1644Y’ + 1.5960Cr’
                // G = 1.1644Y’ - 0.3917Cb’ - 0.8128Cr’
                // B = 1.1644Y’ + 2.0172Cb’

                int r = (298 * y + 409 * v + 128) >> 8;
                int g = (298 * y - 100 * u - 208 * v + 128) >> 8;
                int b = (298 * y + 516 * u + 128) >> 8;

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 255)) / 255.f,
                    float(std::min<int>(std::max<int>(g, 0), 255)) / 255.f,
                    float(std::min<int>(std::max<int>(b, 0), 255)) / 255.f,
                    float(a / 255.f));
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y410:
        if (size >= sizeof(XMUDECN4))
        {
            const XMUDECN4 * __restrict sPtr = reinterpret_cast<const XMUDECN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUDECN4) + 1); icount += sizeof(XMUDECN4))
            {
                int64_t u = int(sPtr->x) - 512;
                int64_t y = int(sPtr->y) - 64;
                int64_t v = int(sPtr->z) - 512;
                unsigned int a = sPtr->w;
                ++sPtr;

                // http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578.aspx

                // Y’  = Y - 64
                // Cb’ = Cb - 512
                // Cr’ = Cr - 512

                // R = 1.1678Y’ + 1.6007Cr’
                // G = 1.1678Y’ - 0.3929Cb’ - 0.8152Cr’
                // B = 1.1678Y’ + 2.0232Cb’

                int r = static_cast<int>((76533 * y + 104905 * v + 32768) >> 16);
                int g = static_cast<int>((76533 * y - 25747 * u - 53425 * v + 32768) >> 16);
                int b = static_cast<int>((76533 * y + 132590 * u + 32768) >> 16);

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 1023)) / 1023.f,
                    float(std::min<int>(std::max<int>(g, 0), 1023)) / 1023.f,
                    float(std::min<int>(std::max<int>(b, 0), 1023)) / 1023.f,
                    float(a / 3.f));
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y416:
        if (size >= sizeof(XMUSHORTN4))
        {
            const XMUSHORTN4 * __restrict sPtr = reinterpret_cast<const XMUSHORTN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUSHORTN4) + 1); icount += sizeof(XMUSHORTN4))
            {
                int64_t u = int64_t(sPtr->x) - 32768;
                int64_t y = int64_t(sPtr->y) - 4096;
                int64_t v = int64_t(sPtr->z) - 32768;
                unsigned int a = sPtr->w;
                ++sPtr;

                // http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578.aspx

                // Y’  = Y - 4096
                // Cb’ = Cb - 32768
                // Cr’ = Cr - 32768

                // R = 1.1689Y’ + 1.6023Cr’
                // G = 1.1689Y’ - 0.3933Cb’ - 0.8160Cr’
                // B = 1.1689Y’+ 2.0251Cb’

                int r = static_cast<int>((76607 * y + 105006 * v + 32768) >> 16);
                int g = static_cast<int>((76607 * y - 25772 * u - 53477 * v + 32768) >> 16);
                int b = static_cast<int>((76607 * y + 132718 * u + 32768) >> 16);

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(g, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(b, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(a, 0), 65535)) / 65535.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_YUY2:
        if (size >= sizeof(XMUBYTEN4))
        {
            const XMUBYTEN4 * __restrict sPtr = reinterpret_cast<const XMUBYTEN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                int y0 = int(sPtr->x) - 16;
                int u = int(sPtr->y) - 128;
                int y1 = int(sPtr->z) - 16;
                int v = int(sPtr->w) - 128;
                ++sPtr;

                // See AYUV
                int r = (298 * y0 + 409 * v + 128) >> 8;
                int g = (298 * y0 - 100 * u - 208 * v + 128) >> 8;
                int b = (298 * y0 + 516 * u + 128) >> 8;

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 255)) / 255.f,
                    float(std::min<int>(std::max<int>(g, 0), 255)) / 255.f,
                    float(std::min<int>(std::max<int>(b, 0), 255)) / 255.f,
                    1.f);

                r = (298 * y1 + 409 * v + 128) >> 8;
                g = (298 * y1 - 100 * u - 208 * v + 128) >> 8;
                b = (298 * y1 + 516 * u + 128) >> 8;

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 255)) / 255.f,
                    float(std::min<int>(std::max<int>(g, 0), 255)) / 255.f,
                    float(std::min<int>(std::max<int>(b, 0), 255)) / 255.f,
                    1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y210:
        // Same as Y216 with least significant 6 bits set to zero
        if (size >= sizeof(XMUSHORTN4))
        {
            const XMUSHORTN4 * __restrict sPtr = reinterpret_cast<const XMUSHORTN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUSHORTN4) + 1); icount += sizeof(XMUSHORTN4))
            {
                int64_t y0 = int64_t(sPtr->x >> 6) - 64;
                int64_t u = int64_t(sPtr->y >> 6) - 512;
                int64_t y1 = int64_t(sPtr->z >> 6) - 64;
                int64_t v = int64_t(sPtr->w >> 6) - 512;
                ++sPtr;

                // See Y410
                int r = static_cast<int>((76533 * y0 + 104905 * v + 32768) >> 16);
                int g = static_cast<int>((76533 * y0 - 25747 * u - 53425 * v + 32768) >> 16);
                int b = static_cast<int>((76533 * y0 + 132590 * u + 32768) >> 16);

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 1023)) / 1023.f,
                    float(std::min<int>(std::max<int>(g, 0), 1023)) / 1023.f,
                    float(std::min<int>(std::max<int>(b, 0), 1023)) / 1023.f,
                    1.f);

                r = static_cast<int>((76533 * y1 + 104905 * v + 32768) >> 16);
                g = static_cast<int>((76533 * y1 - 25747 * u - 53425 * v + 32768) >> 16);
                b = static_cast<int>((76533 * y1 + 132590 * u + 32768) >> 16);

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 1023)) / 1023.f,
                    float(std::min<int>(std::max<int>(g, 0), 1023)) / 1023.f,
                    float(std::min<int>(std::max<int>(b, 0), 1023)) / 1023.f,
                    1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y216:
        if (size >= sizeof(XMUSHORTN4))
        {
            const XMUSHORTN4 * __restrict sPtr = reinterpret_cast<const XMUSHORTN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUSHORTN4) + 1); icount += sizeof(XMUSHORTN4))
            {
                int64_t y0 = int64_t(sPtr->x) - 4096;
                int64_t u = int64_t(sPtr->y) - 32768;
                int64_t y1 = int64_t(sPtr->z) - 4096;
                int64_t v = int64_t(sPtr->w) - 32768;
                ++sPtr;

                // See Y416
                int r = static_cast<int>((76607 * y0 + 105006 * v + 32768) >> 16);
                int g = static_cast<int>((76607 * y0 - 25772 * u - 53477 * v + 32768) >> 16);
                int b = static_cast<int>((76607 * y0 + 132718 * u + 32768) >> 16);

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(g, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(b, 0), 65535)) / 65535.f,
                    1.f);

                r = static_cast<int>((76607 * y1 + 105006 * v + 32768) >> 16);
                g = static_cast<int>((76607 * y1 - 25772 * u - 53477 * v + 32768) >> 16);
                b = static_cast<int>((76607 * y1 + 132718 * u + 32768) >> 16);

                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSet(float(std::min<int>(std::max<int>(r, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(g, 0), 65535)) / 65535.f,
                    float(std::min<int>(std::max<int>(b, 0), 65535)) / 65535.f,
                    1.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B4G4R4A4_UNORM:
        if (size >= sizeof(XMUNIBBLE4))
        {
            static const XMVECTORF32 s_Scale = { { { 1.f / 15.f, 1.f / 15.f, 1.f / 15.f, 1.f / 15.f } } };
            const XMUNIBBLE4 * __restrict sPtr = reinterpret_cast<const XMUNIBBLE4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUNIBBLE4) + 1); icount += sizeof(XMUNIBBLE4))
            {
                XMVECTOR v = XMLoadUNibble4(sPtr++);
                v = XMVectorMultiply(v, s_Scale);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSwizzle<2, 1, 0, 3>(v);
            }
            return true;
        }
        return false;

    case XBOX_DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
        // Xbox One specific 7e3 format
        if (size >= sizeof(XMUDECN4))
        {
            const XMUDECN4 * __restrict sPtr = reinterpret_cast<const XMUDECN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUDECN4) + 1); icount += sizeof(XMUDECN4))
            {
                if (dPtr >= ePtr) break;

                XMVECTORF32 vResult = { { {
                    FloatFrom7e3(sPtr->x),
                    FloatFrom7e3(sPtr->y),
                    FloatFrom7e3(sPtr->z),
                    (float)(sPtr->v >> 30) / 3.0f
                } } };

                ++sPtr;

                *(dPtr++) = vResult.v;
            }
            return true;
        }
        return false;

    case XBOX_DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
        // Xbox One specific 6e4 format
        if (size >= sizeof(XMUDECN4))
        {
            const XMUDECN4 * __restrict sPtr = reinterpret_cast<const XMUDECN4*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(XMUDECN4) + 1); icount += sizeof(XMUDECN4))
            {
                if (dPtr >= ePtr) break;

                XMVECTORF32 vResult = { { {
                    FloatFrom6e4(sPtr->x),
                    FloatFrom6e4(sPtr->y),
                    FloatFrom6e4(sPtr->z),
                    (float)(sPtr->v >> 30) / 3.0f
                } } };

                ++sPtr;

                *(dPtr++) = vResult.v;
            }
            return true;
        }
        return false;

    case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
        // Xbox One specific format
        LOAD_SCANLINE(XMXDECN4, XMLoadXDecN4);

    case XBOX_DXGI_FORMAT_R4G4_UNORM:
        // Xbox One specific format
        if (size >= sizeof(uint8_t))
        {
            static const XMVECTORF32 s_Scale = { { { 1.f / 15.f, 1.f / 15.f, 0.f, 0.f } } };
            const uint8_t * __restrict sPtr = reinterpret_cast<const uint8_t*>(pSource);
            for (size_t icount = 0; icount < (size - sizeof(uint8_t) + 1); icount += sizeof(uint8_t))
            {
                XMUNIBBLE4 nibble;
                nibble.v = static_cast<uint16_t>(*sPtr++);
                XMVECTOR v = XMLoadUNibble4(&nibble);
                v = XMVectorMultiply(v, s_Scale);
                if (dPtr >= ePtr) break;
                *(dPtr++) = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1100);
            }
            return true;
        }
        return false;

        // We don't support the planar or palettized formats

    default:
        return false;
    }
}

#undef LOAD_SCANLINE
#undef LOAD_SCANLINE3
#undef LOAD_SCANLINE2


//-------------------------------------------------------------------------------------
// Stores an image row from standard RGBA XMVECTOR (aligned) array
//-------------------------------------------------------------------------------------
#define STORE_SCANLINE( type, func )\
        if ( size >= sizeof(type) )\
        {\
            type * __restrict dPtr = reinterpret_cast<type*>(pDestination);\
            for( size_t icount = 0; icount < ( size - sizeof(type) + 1 ); icount += sizeof(type) )\
            {\
                if ( sPtr >= ePtr ) break;\
                func( dPtr++, *sPtr++ );\
            }\
            return true; \
        }\
