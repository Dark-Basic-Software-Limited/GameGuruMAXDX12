_Use_decl_annotations_
bool DirectX::_StoreScanline(
    void* pDestination,
    size_t size,
    DXGI_FORMAT format,
    const XMVECTOR* pSource,
    size_t count,
    float threshold)
{
    assert(pDestination && size > 0);
    assert(pSource && count > 0 && (((uintptr_t)pSource & 0xF) == 0));
    assert(IsValid(format) && !IsTypeless(format) && !IsCompressed(format) && !IsPlanar(format) && !IsPalettized(format));

    const XMVECTOR* __restrict sPtr = pSource;
    if (!sPtr)
        return false;

    const XMVECTOR* ePtr = pSource + count;

    switch (static_cast<int>(format))
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        STORE_SCANLINE(XMFLOAT4, XMStoreFloat4)

    case DXGI_FORMAT_R32G32B32A32_UINT:
        STORE_SCANLINE(XMUINT4, XMStoreUInt4)

    case DXGI_FORMAT_R32G32B32A32_SINT:
        STORE_SCANLINE(XMINT4, XMStoreSInt4)

    case DXGI_FORMAT_R32G32B32_FLOAT:
        STORE_SCANLINE(XMFLOAT3, XMStoreFloat3)

    case DXGI_FORMAT_R32G32B32_UINT:
        STORE_SCANLINE(XMUINT3, XMStoreUInt3)

    case DXGI_FORMAT_R32G32B32_SINT:
        STORE_SCANLINE(XMINT3, XMStoreSInt3)

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        if (size >= sizeof(XMHALF4))
        {
            XMHALF4* __restrict dPtr = reinterpret_cast<XMHALF4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMHALF4) + 1); icount += sizeof(XMHALF4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = *sPtr++;
                v = XMVectorClamp(v, g_HalfMin, g_HalfMax);
                XMStoreHalf4(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16G16B16A16_UNORM:
        STORE_SCANLINE(XMUSHORTN4, XMStoreUShortN4)

    case DXGI_FORMAT_R16G16B16A16_UINT:
        STORE_SCANLINE(XMUSHORT4, XMStoreUShort4)

    case DXGI_FORMAT_R16G16B16A16_SNORM:
        STORE_SCANLINE(XMSHORTN4, XMStoreShortN4)

    case DXGI_FORMAT_R16G16B16A16_SINT:
        STORE_SCANLINE(XMSHORT4, XMStoreShort4)

    case DXGI_FORMAT_R32G32_FLOAT:
        STORE_SCANLINE(XMFLOAT2, XMStoreFloat2)

    case DXGI_FORMAT_R32G32_UINT:
        STORE_SCANLINE(XMUINT2, XMStoreUInt2)

    case DXGI_FORMAT_R32G32_SINT:
        STORE_SCANLINE(XMINT2, XMStoreSInt2)

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            const size_t psize = sizeof(float) + sizeof(uint32_t);
            if (size >= psize)
            {
                float *dPtr = reinterpret_cast<float*>(pDestination);
                for (size_t icount = 0; icount < (size - psize + 1); icount += psize)
                {
                    if (sPtr >= ePtr) break;
                    XMFLOAT4 f;
                    XMStoreFloat4(&f, *sPtr++);
                    dPtr[0] = f.x;
                    uint8_t* ps8 = reinterpret_cast<uint8_t*>(&dPtr[1]);
                    ps8[0] = static_cast<uint8_t>(std::min<float>(255.f, std::max<float>(0.f, f.y)));
                    ps8[1] = ps8[2] = ps8[3] = 0;
                    dPtr += 2;
                }
                return true;
            }
        }
        return false;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        STORE_SCANLINE(XMUDECN4, XMStoreUDecN4);

    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        STORE_SCANLINE(XMUDECN4, XMStoreUDecN4_XR);

    case DXGI_FORMAT_R10G10B10A2_UINT:
        STORE_SCANLINE(XMUDEC4, XMStoreUDec4);

    case DXGI_FORMAT_R11G11B10_FLOAT:
        STORE_SCANLINE(XMFLOAT3PK, XMStoreFloat3PK);

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorAdd(*sPtr++, g_8BitBias);
                XMStoreUByteN4(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8G8B8A8_UINT:
        STORE_SCANLINE(XMUBYTE4, XMStoreUByte4)

    case DXGI_FORMAT_R8G8B8A8_SNORM:
        STORE_SCANLINE(XMBYTEN4, XMStoreByteN4)

    case DXGI_FORMAT_R8G8B8A8_SINT:
        STORE_SCANLINE(XMBYTE4, XMStoreByte4)

    case DXGI_FORMAT_R16G16_FLOAT:
        if (size >= sizeof(XMHALF2))
        {
            XMHALF2* __restrict dPtr = reinterpret_cast<XMHALF2*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMHALF2) + 1); icount += sizeof(XMHALF2))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = *sPtr++;
                v = XMVectorClamp(v, g_HalfMin, g_HalfMax);
                XMStoreHalf2(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16G16_UNORM:
        STORE_SCANLINE(XMUSHORTN2, XMStoreUShortN2)

    case DXGI_FORMAT_R16G16_UINT:
        STORE_SCANLINE(XMUSHORT2, XMStoreUShort2)

    case DXGI_FORMAT_R16G16_SNORM:
        STORE_SCANLINE(XMSHORTN2, XMStoreShortN2)

    case DXGI_FORMAT_R16G16_SINT:
        STORE_SCANLINE(XMSHORT2, XMStoreShort2)

    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        if (size >= sizeof(float))
        {
            float * __restrict dPtr = reinterpret_cast<float*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(float) + 1); icount += sizeof(float))
            {
                if (sPtr >= ePtr) break;
                XMStoreFloat(dPtr++, *(sPtr++));
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R32_UINT:
        if (size >= sizeof(uint32_t))
        {
            uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(uint32_t) + 1); icount += sizeof(uint32_t))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMConvertVectorFloatToUInt(*(sPtr++), 0);
                XMStoreInt(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R32_SINT:
        if (size >= sizeof(int32_t))
        {
            uint32_t * __restrict dPtr = reinterpret_cast<uint32_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(int32_t) + 1); icount += sizeof(int32_t))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMConvertVectorFloatToInt(*(sPtr++), 0);
                XMStoreInt(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        if (size >= sizeof(uint32_t))
        {
            static const XMVECTORF32 clamp = { { { 1.f, 255.f, 0.f, 0.f } } };
            XMVECTOR zero = XMVectorZero();
            uint32_t *dPtr = reinterpret_cast<uint32_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(uint32_t) + 1); icount += sizeof(uint32_t))
            {
                if (sPtr >= ePtr) break;
                XMFLOAT4 f;
                XMStoreFloat4(&f, XMVectorClamp(*sPtr++, zero, clamp));
                *dPtr++ = (static_cast<uint32_t>(f.x * 16777215.f) & 0xFFFFFF)
                    | ((static_cast<uint32_t>(f.y) & 0xFF) << 24);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8G8_UNORM:
        STORE_SCANLINE(XMUBYTEN2, XMStoreUByteN2)

    case DXGI_FORMAT_R8G8_UINT:
        STORE_SCANLINE(XMUBYTE2, XMStoreUByte2)

    case DXGI_FORMAT_R8G8_SNORM:
        STORE_SCANLINE(XMBYTEN2, XMStoreByteN2)

    case DXGI_FORMAT_R8G8_SINT:
        STORE_SCANLINE(XMBYTE2, XMStoreByte2)

    case DXGI_FORMAT_R16_FLOAT:
        if (size >= sizeof(HALF))
        {
            HALF * __restrict dPtr = reinterpret_cast<HALF*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(HALF) + 1); icount += sizeof(HALF))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 65504.f), -65504.f);
                *(dPtr++) = XMConvertFloatToHalf(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        if (size >= sizeof(uint16_t))
        {
            uint16_t * __restrict dPtr = reinterpret_cast<uint16_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(uint16_t) + 1); icount += sizeof(uint16_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 1.f), 0.f);
                *(dPtr++) = static_cast<uint16_t>(v*65535.f + 0.5f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16_UINT:
        if (size >= sizeof(uint16_t))
        {
            uint16_t * __restrict dPtr = reinterpret_cast<uint16_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(uint16_t) + 1); icount += sizeof(uint16_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 65535.f), 0.f);
                *(dPtr++) = static_cast<uint16_t>(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16_SNORM:
        if (size >= sizeof(int16_t))
        {
            int16_t * __restrict dPtr = reinterpret_cast<int16_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(int16_t) + 1); icount += sizeof(int16_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 1.f), -1.f);
                *(dPtr++) = static_cast<int16_t>(v * 32767.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R16_SINT:
        if (size >= sizeof(int16_t))
        {
            int16_t * __restrict dPtr = reinterpret_cast<int16_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(int16_t) + 1); icount += sizeof(int16_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 32767.f), -32767.f);
                *(dPtr++) = static_cast<int16_t>(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_UNORM:
        if (size >= sizeof(uint8_t))
        {
            uint8_t * __restrict dPtr = reinterpret_cast<uint8_t*>(pDestination);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 1.f), 0.f);
                *(dPtr++) = static_cast<uint8_t>(v * 255.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_UINT:
        if (size >= sizeof(uint8_t))
        {
            uint8_t * __restrict dPtr = reinterpret_cast<uint8_t*>(pDestination);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 255.f), 0.f);
                *(dPtr++) = static_cast<uint8_t>(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_SNORM:
        if (size >= sizeof(int8_t))
        {
            int8_t * __restrict dPtr = reinterpret_cast<int8_t*>(pDestination);
            for (size_t icount = 0; icount < size; icount += sizeof(int8_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 1.f), -1.f);
                *(dPtr++) = static_cast<int8_t>(v * 127.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8_SINT:
        if (size >= sizeof(int8_t))
        {
            int8_t * __restrict dPtr = reinterpret_cast<int8_t*>(pDestination);
            for (size_t icount = 0; icount < size; icount += sizeof(int8_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetX(*sPtr++);
                v = std::max<float>(std::min<float>(v, 127.f), -127.f);
                *(dPtr++) = static_cast<int8_t>(v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_A8_UNORM:
        if (size >= sizeof(uint8_t))
        {
            uint8_t * __restrict dPtr = reinterpret_cast<uint8_t*>(pDestination);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                if (sPtr >= ePtr) break;
                float v = XMVectorGetW(*sPtr++);
                v = std::max<float>(std::min<float>(v, 1.f), 0.f);
                *(dPtr++) = static_cast<uint8_t>(v * 255.f);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R1_UNORM:
        if (size >= sizeof(uint8_t))
        {
            uint8_t * __restrict dPtr = reinterpret_cast<uint8_t*>(pDestination);
            for (size_t icount = 0; icount < size; icount += sizeof(uint8_t))
            {
                uint8_t pixels = 0;
                for (size_t bcount = 8; bcount > 0; --bcount)
                {
                    if (sPtr >= ePtr) break;
                    float v = XMVectorGetX(*sPtr++);

                    // Absolute thresholding generally doesn't give good results for all images
                    // Picking the 'right' threshold automatically requires whole-image analysis

                    if (v > 0.25f)
                        pixels |= 1 << (bcount - 1);
                }
                *(dPtr++) = pixels;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        STORE_SCANLINE(XMFLOAT3SE, StoreFloat3SE)

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v0 = *sPtr++;
                XMVECTOR v1 = (sPtr < ePtr) ? XMVectorSplatY(*sPtr++) : XMVectorZero();
                XMVECTOR v = XMVectorSelect(v1, v0, g_XMSelect1110);
                v = XMVectorAdd(v, g_8BitBias);
                XMStoreUByteN4(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        if (size >= sizeof(XMUBYTEN4))
        {
            static XMVECTORU32 select1101 = { { { XM_SELECT_1, XM_SELECT_1, XM_SELECT_0, XM_SELECT_1 } } };

            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v0 = XMVectorSwizzle<1, 0, 3, 2>(*sPtr++);
                XMVECTOR v1 = (sPtr < ePtr) ? XMVectorSplatY(*sPtr++) : XMVectorZero();
                XMVECTOR v = XMVectorSelect(v1, v0, select1101);
                v = XMVectorAdd(v, g_8BitBias);
                XMStoreUByteN4(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B5G6R5_UNORM:
        if (size >= sizeof(XMU565))
        {
            static const XMVECTORF32 s_Scale = { { { 31.f, 63.f, 31.f, 1.f } } };
            XMU565 * __restrict dPtr = reinterpret_cast<XMU565*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMU565) + 1); icount += sizeof(XMU565))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(*sPtr++);
                v = XMVectorMultiply(v, s_Scale);
                XMStoreU565(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B5G5R5A1_UNORM:
        if (size >= sizeof(XMU555))
        {
            static const XMVECTORF32 s_Scale = { { { 31.f, 31.f, 31.f, 1.f } } };
            XMU555 * __restrict dPtr = reinterpret_cast<XMU555*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMU555) + 1); icount += sizeof(XMU555))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(*sPtr++);
                v = XMVectorMultiply(v, s_Scale);
                XMStoreU555(dPtr, v);
                dPtr->w = (XMVectorGetW(v) > threshold) ? 1 : 0;
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(*sPtr++);
                v = XMVectorAdd(v, g_8BitBias);
                XMStoreUByteN4(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorPermute<2, 1, 0, 7>(*sPtr++, g_XMIdentityR3);
                v = XMVectorAdd(v, g_8BitBias);
                XMStoreUByteN4(dPtr++, v);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_AYUV:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;

                XMUBYTEN4 rgba;
                XMStoreUByteN4(&rgba, *sPtr++);

                // http://msdn.microsoft.com/en-us/library/windows/desktop/dd206750.aspx

                // Y  =  0.2568R + 0.5041G + 0.1001B + 16
                // Cb = -0.1482R - 0.2910G + 0.4392B + 128
                // Cr =  0.4392R - 0.3678G - 0.0714B + 128

                int y = ((66 * rgba.x + 129 * rgba.y + 25 * rgba.z + 128) >> 8) + 16;
                int u = ((-38 * rgba.x - 74 * rgba.y + 112 * rgba.z + 128) >> 8) + 128;
                int v = ((112 * rgba.x - 94 * rgba.y - 18 * rgba.z + 128) >> 8) + 128;

                dPtr->x = static_cast<uint8_t>(std::min<int>(std::max<int>(v, 0), 255));
                dPtr->y = static_cast<uint8_t>(std::min<int>(std::max<int>(u, 0), 255));
                dPtr->z = static_cast<uint8_t>(std::min<int>(std::max<int>(y, 0), 255));
                dPtr->w = rgba.w;
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y410:
        if (size >= sizeof(XMUDECN4))
        {
            XMUDECN4 * __restrict dPtr = reinterpret_cast<XMUDECN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUDECN4) + 1); icount += sizeof(XMUDECN4))
            {
                if (sPtr >= ePtr) break;

                XMUDECN4 rgba;
                XMStoreUDecN4(&rgba, *sPtr++);

                // http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578.aspx

                // Y  =  0.2560R + 0.5027G + 0.0998B + 64
                // Cb = -0.1478R - 0.2902G + 0.4379B + 512
                // Cr =  0.4379R - 0.3667G - 0.0712B + 512

                int64_t r = rgba.x;
                int64_t g = rgba.y;
                int64_t b = rgba.z;

                int y = static_cast<int>((16780 * r + 32942 * g + 6544 * b + 32768) >> 16) + 64;
                int u = static_cast<int>((-9683 * r - 19017 * g + 28700 * b + 32768) >> 16) + 512;
                int v = static_cast<int>((28700 * r - 24033 * g - 4667 * b + 32768) >> 16) + 512;

                dPtr->x = static_cast<uint32_t>(std::min<int>(std::max<int>(u, 0), 1023));
                dPtr->y = static_cast<uint32_t>(std::min<int>(std::max<int>(y, 0), 1023));
                dPtr->z = static_cast<uint32_t>(std::min<int>(std::max<int>(v, 0), 1023));
                dPtr->w = rgba.w;
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y416:
        if (size >= sizeof(XMUSHORTN4))
        {
            XMUSHORTN4 * __restrict dPtr = reinterpret_cast<XMUSHORTN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUSHORTN4) + 1); icount += sizeof(XMUSHORTN4))
            {
                if (sPtr >= ePtr) break;

                XMUSHORTN4 rgba;
                XMStoreUShortN4(&rgba, *sPtr++);

                // http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578.aspx

                // Y  =  0.2558R + 0.5022G + 0.0998B + 4096
                // Cb = -0.1476R - 0.2899G + 0.4375B + 32768
                // Cr =  0.4375R - 0.3664G - 0.0711B + 32768

                int64_t r = int64_t(rgba.x);
                int64_t g = int64_t(rgba.y);
                int64_t b = int64_t(rgba.z);

                int y = static_cast<int>((16763 * r + 32910 * g + 6537 * b + 32768) >> 16) + 4096;
                int u = static_cast<int>((-9674 * r - 18998 * g + 28672 * b + 32768) >> 16) + 32768;
                int v = static_cast<int>((28672 * r - 24010 * g - 4662 * b + 32768) >> 16) + 32768;

                dPtr->x = static_cast<uint16_t>(std::min<int>(std::max<int>(u, 0), 65535));
                dPtr->y = static_cast<uint16_t>(std::min<int>(std::max<int>(y, 0), 65535));
                dPtr->z = static_cast<uint16_t>(std::min<int>(std::max<int>(v, 0), 65535));
                dPtr->w = rgba.w;
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_YUY2:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUBYTEN4) + 1); icount += sizeof(XMUBYTEN4))
            {
                if (sPtr >= ePtr) break;

                XMUBYTEN4 rgb1;
                XMStoreUByteN4(&rgb1, *sPtr++);

                // See AYUV
                int y0 = ((66 * rgb1.x + 129 * rgb1.y + 25 * rgb1.z + 128) >> 8) + 16;
                int u0 = ((-38 * rgb1.x - 74 * rgb1.y + 112 * rgb1.z + 128) >> 8) + 128;
                int v0 = ((112 * rgb1.x - 94 * rgb1.y - 18 * rgb1.z + 128) >> 8) + 128;

                XMUBYTEN4 rgb2;
                if (sPtr < ePtr)
                {
                    XMStoreUByteN4(&rgb2, *sPtr++);
                }
                else
                {
                    rgb2.x = rgb2.y = rgb2.z = rgb2.w = 0;
                }

                int y1 = ((66 * rgb2.x + 129 * rgb2.y + 25 * rgb2.z + 128) >> 8) + 16;
                int u1 = ((-38 * rgb2.x - 74 * rgb2.y + 112 * rgb2.z + 128) >> 8) + 128;
                int v1 = ((112 * rgb2.x - 94 * rgb2.y - 18 * rgb2.z + 128) >> 8) + 128;

                dPtr->x = static_cast<uint8_t>(std::min<int>(std::max<int>(y0, 0), 255));
                dPtr->y = static_cast<uint8_t>(std::min<int>(std::max<int>((u0 + u1) >> 1, 0), 255));
                dPtr->z = static_cast<uint8_t>(std::min<int>(std::max<int>(y1, 0), 255));
                dPtr->w = static_cast<uint8_t>(std::min<int>(std::max<int>((v0 + v1) >> 1, 0), 255));
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y210:
        // Same as Y216 with least significant 6 bits set to zero
        if (size >= sizeof(XMUSHORTN4))
        {
            XMUSHORTN4 * __restrict dPtr = reinterpret_cast<XMUSHORTN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUSHORTN4) + 1); icount += sizeof(XMUSHORTN4))
            {
                if (sPtr >= ePtr) break;

                XMUDECN4 rgb1;
                XMStoreUDecN4(&rgb1, *sPtr++);

                // See Y410
                int64_t r = rgb1.x;
                int64_t g = rgb1.y;
                int64_t b = rgb1.z;

                int y0 = static_cast<int>((16780 * r + 32942 * g + 6544 * b + 32768) >> 16) + 64;
                int u0 = static_cast<int>((-9683 * r - 19017 * g + 28700 * b + 32768) >> 16) + 512;
                int v0 = static_cast<int>((28700 * r - 24033 * g - 4667 * b + 32768) >> 16) + 512;

                XMUDECN4 rgb2;
                if (sPtr < ePtr)
                {
                    XMStoreUDecN4(&rgb2, *sPtr++);
                }
                else
                {
                    rgb2.x = rgb2.y = rgb2.z = rgb2.w = 0;
                }

                r = rgb2.x;
                g = rgb2.y;
                b = rgb2.z;

                int y1 = static_cast<int>((16780 * r + 32942 * g + 6544 * b + 32768) >> 16) + 64;
                int u1 = static_cast<int>((-9683 * r - 19017 * g + 28700 * b + 32768) >> 16) + 512;
                int v1 = static_cast<int>((28700 * r - 24033 * g - 4667 * b + 32768) >> 16) + 512;

                dPtr->x = static_cast<uint16_t>(std::min<int>(std::max<int>(y0, 0), 1023) << 6);
                dPtr->y = static_cast<uint16_t>(std::min<int>(std::max<int>((u0 + u1) >> 1, 0), 1023) << 6);
                dPtr->z = static_cast<uint16_t>(std::min<int>(std::max<int>(y1, 0), 1023) << 6);
                dPtr->w = static_cast<uint16_t>(std::min<int>(std::max<int>((v0 + v1) >> 1, 0), 1023) << 6);
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_Y216:
        if (size >= sizeof(XMUSHORTN4))
        {
            XMUSHORTN4 * __restrict dPtr = reinterpret_cast<XMUSHORTN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUSHORTN4) + 1); icount += sizeof(XMUSHORTN4))
            {
                if (sPtr >= ePtr) break;

                XMUSHORTN4 rgb1;
                XMStoreUShortN4(&rgb1, *sPtr++);

                // See Y416
                int64_t r = int64_t(rgb1.x);
                int64_t g = int64_t(rgb1.y);
                int64_t b = int64_t(rgb1.z);

                int y0 = static_cast<int>((16763 * r + 32910 * g + 6537 * b + 32768) >> 16) + 4096;
                int u0 = static_cast<int>((-9674 * r - 18998 * g + 28672 * b + 32768) >> 16) + 32768;
                int v0 = static_cast<int>((28672 * r - 24010 * g - 4662 * b + 32768) >> 16) + 32768;

                XMUSHORTN4 rgb2;
                if (sPtr < ePtr)
                {
                    XMStoreUShortN4(&rgb2, *sPtr++);
                }
                else
                {
                    rgb2.x = rgb2.y = rgb2.z = rgb2.w = 0;
                }

                r = int64_t(rgb2.x);
                g = int64_t(rgb2.y);
                b = int64_t(rgb2.z);

                int y1 = static_cast<int>((16763 * r + 32910 * g + 6537 * b + 32768) >> 16) + 4096;
                int u1 = static_cast<int>((-9674 * r - 18998 * g + 28672 * b + 32768) >> 16) + 32768;
                int v1 = static_cast<int>((28672 * r - 24010 * g - 4662 * b + 32768) >> 16) + 32768;

                dPtr->x = static_cast<uint16_t>(std::min<int>(std::max<int>(y0, 0), 65535));
                dPtr->y = static_cast<uint16_t>(std::min<int>(std::max<int>((u0 + u1) >> 1, 0), 65535));
                dPtr->z = static_cast<uint16_t>(std::min<int>(std::max<int>(y1, 0), 65535));
                dPtr->w = static_cast<uint16_t>(std::min<int>(std::max<int>((v0 + v1) >> 1, 0), 65535));
                ++dPtr;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B4G4R4A4_UNORM:
        if (size >= sizeof(XMUNIBBLE4))
        {
            static const XMVECTORF32 s_Scale = { { { 15.f, 15.f, 15.f, 15.f } } };
            XMUNIBBLE4 * __restrict dPtr = reinterpret_cast<XMUNIBBLE4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUNIBBLE4) + 1); icount += sizeof(XMUNIBBLE4))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(*sPtr++);
                v = XMVectorMultiply(v, s_Scale);
                XMStoreUNibble4(dPtr++, v);
            }
            return true;
        }
        return false;

    case XBOX_DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
        // Xbox One specific 7e3 format with alpha
        if (size >= sizeof(XMUDECN4))
        {
            static const XMVECTORF32  Scale = { { { 1.0f, 1.0f, 1.0f, 3.0f } } };
            static const XMVECTORF32  C = { { { 31.875f, 31.875f, 31.875f, 3.f } } };

            XMUDECN4 * __restrict dPtr = reinterpret_cast<XMUDECN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUDECN4) + 1); icount += sizeof(XMUDECN4))
            {
                if (sPtr >= ePtr) break;

                XMVECTOR V = XMVectorMultiply(*sPtr++, Scale);
                V = XMVectorClamp(V, g_XMZero, C);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, V);

                dPtr->x = FloatTo7e3(tmp.x);
                dPtr->y = FloatTo7e3(tmp.y);
                dPtr->z = FloatTo7e3(tmp.z);
                dPtr->w = (uint32_t)tmp.w;
                ++dPtr;
            }
            return true;
        }
        return false;

    case XBOX_DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
        // Xbox One specific 6e4 format with alpha
        if (size >= sizeof(XMUDECN4))
        {
            static const XMVECTORF32  Scale = { { { 1.0f, 1.0f, 1.0f, 3.0f } } };
            static const XMVECTORF32  C = { { { 508.f, 508.f, 508.f, 3.f } } };

            XMUDECN4 * __restrict dPtr = reinterpret_cast<XMUDECN4*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(XMUDECN4) + 1); icount += sizeof(XMUDECN4))
            {
                if (sPtr >= ePtr) break;

                XMVECTOR V = XMVectorMultiply(*sPtr++, Scale);
                V = XMVectorClamp(V, g_XMZero, C);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, V);

                dPtr->x = FloatTo6e4(tmp.x);
                dPtr->y = FloatTo6e4(tmp.y);
                dPtr->z = FloatTo6e4(tmp.z);
                dPtr->w = (uint32_t)tmp.w;
                ++dPtr;
            }
            return true;
        }
        return false;

    case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
        // Xbox One specific format
        STORE_SCANLINE(XMXDECN4, XMStoreXDecN4);

    case XBOX_DXGI_FORMAT_R4G4_UNORM:
        // Xbox One specific format
        if (size >= sizeof(uint8_t))
        {
            static const XMVECTORF32 s_Scale = { { { 15.f, 15.f, 0.f, 0.f } } };
            uint8_t * __restrict dPtr = reinterpret_cast<uint8_t*>(pDestination);
            for (size_t icount = 0; icount < (size - sizeof(uint8_t) + 1); icount += sizeof(uint8_t))
            {
                if (sPtr >= ePtr) break;
                XMVECTOR v = XMVectorMultiply(*sPtr++, s_Scale);

                XMUNIBBLE4 nibble;
                XMStoreUNibble4(&nibble, v);
                *dPtr = static_cast<uint8_t>(nibble.v);
                ++dPtr;
            }
            return true;
        }
        return false;

        // We don't support the planar or palettized formats

    default:
        return false;
    }
}

#undef STORE_SCANLINE


//-------------------------------------------------------------------------------------
// Convert DXGI image to/from GUID_WICPixelFormat128bppRGBAFloat (no range conversions)
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::_ConvertToR32G32B32A32(const Image& srcImage, ScratchImage& image)
{
    if (!srcImage.pixels)
        return E_POINTER;

    HRESULT hr = image.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, srcImage.width, srcImage.height, 1, 1);
    if (FAILED(hr))
        return hr;

    const Image *img = image.GetImage(0, 0, 0);
    if (!img)
    {
        image.Release();
        return E_POINTER;
    }

    uint8_t* pDest = img->pixels;
    if (!pDest)
    {
        image.Release();
        return E_POINTER;
    }

    const uint8_t *pSrc = srcImage.pixels;
    for (size_t h = 0; h < srcImage.height; ++h)
    {
        if (!_LoadScanline(reinterpret_cast<XMVECTOR*>(pDest), srcImage.width, pSrc, srcImage.rowPitch, srcImage.format))
        {
            image.Release();
            return E_FAIL;
        }

        pSrc += srcImage.rowPitch;
        pDest += img->rowPitch;
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT DirectX::_ConvertFromR32G32B32A32(const Image& srcImage, const Image& destImage)
{
    assert(srcImage.format == DXGI_FORMAT_R32G32B32A32_FLOAT);

    if (!srcImage.pixels || !destImage.pixels)
        return E_POINTER;

    if (srcImage.width != destImage.width || srcImage.height != destImage.height)
        return E_FAIL;

    const uint8_t *pSrc = srcImage.pixels;
    uint8_t* pDest = destImage.pixels;

    for (size_t h = 0; h < srcImage.height; ++h)
    {
        if (!_StoreScanline(pDest, destImage.rowPitch, destImage.format, reinterpret_cast<const XMVECTOR*>(pSrc), srcImage.width))
            return E_FAIL;

        pSrc += srcImage.rowPitch;
        pDest += destImage.rowPitch;
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT DirectX::_ConvertFromR32G32B32A32(const Image& srcImage, DXGI_FORMAT format, ScratchImage& image)
{
    if (!srcImage.pixels)
        return E_POINTER;

    HRESULT hr = image.Initialize2D(format, srcImage.width, srcImage.height, 1, 1);
    if (FAILED(hr))
        return hr;

    const Image *img = image.GetImage(0, 0, 0);
    if (!img)
    {
        image.Release();
        return E_POINTER;
    }

    hr = _ConvertFromR32G32B32A32(srcImage, *img);
    if (FAILED(hr))
    {
        image.Release();
        return hr;
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT DirectX::_ConvertFromR32G32B32A32(
    const Image* srcImages,
    size_t nimages,
    const TexMetadata& metadata,
    DXGI_FORMAT format,
    ScratchImage& result)
{
    if (!srcImages)
        return E_POINTER;

    result.Release();

    assert(metadata.format == DXGI_FORMAT_R32G32B32A32_FLOAT);

    TexMetadata mdata2 = metadata;
    mdata2.format = format;
    HRESULT hr = result.Initialize(mdata2);
    if (FAILED(hr))
        return hr;

    if (nimages != result.GetImageCount())
    {
        result.Release();
        return E_FAIL;
    }

    const Image* dest = result.GetImages();
    if (!dest)
    {
        result.Release();
        return E_POINTER;
    }

    for (size_t index = 0; index < nimages; ++index)
    {
        const Image& src = srcImages[index];
        const Image& dst = dest[index];

        assert(src.format == DXGI_FORMAT_R32G32B32A32_FLOAT);
        assert(dst.format == format);

        if (src.width != dst.width || src.height != dst.height)
        {
            result.Release();
            return E_FAIL;
        }

        const uint8_t* pSrc = src.pixels;
        uint8_t* pDest = dst.pixels;
        if (!pSrc || !pDest)
        {
            result.Release();
            return E_POINTER;
        }

        for (size_t h = 0; h < src.height; ++h)
        {
            if (!_StoreScanline(pDest, dst.rowPitch, format, reinterpret_cast<const XMVECTOR*>(pSrc), src.width))
            {
                result.Release();
                return E_FAIL;
            }

            pSrc += src.rowPitch;
            pDest += dst.rowPitch;
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
// Convert from Linear RGB to sRGB
//
// if C_linear <= 0.0031308 -> C_srgb = 12.92 * C_linear
// if C_linear >  0.0031308 -> C_srgb = ( 1 + a ) * pow( C_Linear, 1 / 2.4 ) - a
//                             where a = 0.055
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
bool DirectX::_StoreScanlineLinear(
    void* pDestination,
    size_t size,
    DXGI_FORMAT format,
    XMVECTOR* pSource,
    size_t count,
    DWORD flags,
    float threshold)
{
    assert(pDestination && size > 0);
    assert(pSource && count > 0 && (((uintptr_t)pSource & 0xF) == 0));
    assert(IsValid(format) && !IsTypeless(format) && !IsCompressed(format) && !IsPlanar(format) && !IsPalettized(format));

    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        flags |= TEX_FILTER_SRGB;
        break;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        break;

    default:
        // can't treat A8, XR, Depth, SNORM, UINT, or SINT as sRGB
        flags &= ~TEX_FILTER_SRGB;
        break;
    }

    // sRGB output processing (Linear RGB -> sRGB)
    if (flags & TEX_FILTER_SRGB_OUT)
    {
        // To avoid the need for another temporary scanline buffer, we allow this function to overwrite the source buffer in-place
        // Given the intended usage in the filtering routines, this is not a problem.
        XMVECTOR* ptr = pSource;
        for (size_t i = 0; i < count; ++i, ++ptr)
        {
            *ptr = XMColorRGBToSRGB(*ptr);
        }
    }

    return _StoreScanline(pDestination, size, format, pSource, count, threshold);
}


//-------------------------------------------------------------------------------------
// Convert from sRGB to Linear RGB
//
// if C_srgb <= 0.04045 -> C_linear = C_srgb / 12.92
// if C_srgb >  0.04045 -> C_linear = pow( ( C_srgb + a ) / ( 1 + a ), 2.4 )
//                         where a = 0.055
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
bool DirectX::_LoadScanlineLinear(
    XMVECTOR* pDestination,
    size_t count,
    const void* pSource,
    size_t size,
    DXGI_FORMAT format,
    DWORD flags)
{
    assert(pDestination && count > 0 && (((uintptr_t)pDestination & 0xF) == 0));
    assert(pSource && size > 0);
    assert(IsValid(format) && !IsTypeless(format, false) && !IsCompressed(format) && !IsPlanar(format) && !IsPalettized(format));

    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        flags |= TEX_FILTER_SRGB;
        break;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        break;

    default:
        // can't treat A8, XR, Depth, SNORM, UINT, or SINT as sRGB
        flags &= ~TEX_FILTER_SRGB;
        break;
    }

    if (_LoadScanline(pDestination, count, pSource, size, format))
    {
        // sRGB input processing (sRGB -> Linear RGB)
        if (flags & TEX_FILTER_SRGB_IN)
        {
            XMVECTOR* ptr = pDestination;
            for (size_t i = 0; i < count; ++i, ++ptr)
            {
                *ptr = XMColorSRGBToRGB(*ptr);
            }
        }

        return true;
    }

    return false;
}


//-------------------------------------------------------------------------------------
// Convert scanline based on source/target formats
//-------------------------------------------------------------------------------------
namespace
{
    struct ConvertData
    {
        DXGI_FORMAT format;
        size_t datasize;
        DWORD flags;
    };

    const ConvertData g_ConvertTable[] =
    {
        { DXGI_FORMAT_R32G32B32A32_FLOAT,           32, CONVF_FLOAT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R32G32B32A32_UINT,            32, CONVF_UINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R32G32B32A32_SINT,            32, CONVF_SINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R32G32B32_FLOAT,              32, CONVF_FLOAT | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_R32G32B32_UINT,               32, CONVF_UINT | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_R32G32B32_SINT,               32, CONVF_SINT | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_R16G16B16A16_FLOAT,           16, CONVF_FLOAT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R16G16B16A16_UNORM,           16, CONVF_UNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R16G16B16A16_UINT,            16, CONVF_UINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R16G16B16A16_SNORM,           16, CONVF_SNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R16G16B16A16_SINT,            16, CONVF_SINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R32G32_FLOAT,                 32, CONVF_FLOAT | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R32G32_UINT,                  32, CONVF_UINT | CONVF_R | CONVF_G  },
        { DXGI_FORMAT_R32G32_SINT,                  32, CONVF_SINT | CONVF_R | CONVF_G  },
        { DXGI_FORMAT_D32_FLOAT_S8X24_UINT,         32, CONVF_FLOAT | CONVF_DEPTH | CONVF_STENCIL },
        { DXGI_FORMAT_R10G10B10A2_UNORM,            10, CONVF_UNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R10G10B10A2_UINT,             10, CONVF_UINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R11G11B10_FLOAT,              10, CONVF_FLOAT | CONVF_POS_ONLY | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_R8G8B8A8_UNORM,               8, CONVF_UNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,          8, CONVF_UNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R8G8B8A8_UINT,                8, CONVF_UINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R8G8B8A8_SNORM,               8, CONVF_SNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R8G8B8A8_SINT,                8, CONVF_SINT | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_R16G16_FLOAT,                 16, CONVF_FLOAT | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R16G16_UNORM,                 16, CONVF_UNORM | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R16G16_UINT,                  16, CONVF_UINT | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R16G16_SNORM,                 16, CONVF_SNORM | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R16G16_SINT,                  16, CONVF_SINT | CONVF_R | CONVF_G },
        { DXGI_FORMAT_D32_FLOAT,                    32, CONVF_FLOAT | CONVF_DEPTH },
        { DXGI_FORMAT_R32_FLOAT,                    32, CONVF_FLOAT | CONVF_R },
        { DXGI_FORMAT_R32_UINT,                     32, CONVF_UINT | CONVF_R },
        { DXGI_FORMAT_R32_SINT,                     32, CONVF_SINT | CONVF_R },
        { DXGI_FORMAT_D24_UNORM_S8_UINT,            32, CONVF_UNORM | CONVF_DEPTH | CONVF_STENCIL },
        { DXGI_FORMAT_R8G8_UNORM,                   8, CONVF_UNORM | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R8G8_UINT,                    8, CONVF_UINT | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R8G8_SNORM,                   8, CONVF_SNORM | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R8G8_SINT,                    8, CONVF_SINT | CONVF_R | CONVF_G },
        { DXGI_FORMAT_R16_FLOAT,                    16, CONVF_FLOAT | CONVF_R },
        { DXGI_FORMAT_D16_UNORM,                    16, CONVF_UNORM | CONVF_DEPTH },
        { DXGI_FORMAT_R16_UNORM,                    16, CONVF_UNORM | CONVF_R },
        { DXGI_FORMAT_R16_UINT,                     16, CONVF_UINT | CONVF_R },
        { DXGI_FORMAT_R16_SNORM,                    16, CONVF_SNORM | CONVF_R },
        { DXGI_FORMAT_R16_SINT,                     16, CONVF_SINT | CONVF_R },
        { DXGI_FORMAT_R8_UNORM,                     8, CONVF_UNORM | CONVF_R },
        { DXGI_FORMAT_R8_UINT,                      8, CONVF_UINT | CONVF_R },
        { DXGI_FORMAT_R8_SNORM,                     8, CONVF_SNORM | CONVF_R },
        { DXGI_FORMAT_R8_SINT,                      8, CONVF_SINT | CONVF_R },
        { DXGI_FORMAT_A8_UNORM,                     8, CONVF_UNORM | CONVF_A },
        { DXGI_FORMAT_R1_UNORM,                     1, CONVF_UNORM | CONVF_R },
        { DXGI_FORMAT_R9G9B9E5_SHAREDEXP,           9, CONVF_FLOAT | CONVF_SHAREDEXP | CONVF_POS_ONLY | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_R8G8_B8G8_UNORM,              8, CONVF_UNORM | CONVF_PACKED | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_G8R8_G8B8_UNORM,              8, CONVF_UNORM | CONVF_PACKED | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_BC1_UNORM,                    8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC1_UNORM_SRGB,               8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC2_UNORM,                    8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC2_UNORM_SRGB,               8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC3_UNORM,                    8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC3_UNORM_SRGB,               8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC4_UNORM,                    8, CONVF_UNORM | CONVF_BC | CONVF_R },
        { DXGI_FORMAT_BC4_SNORM,                    8, CONVF_SNORM | CONVF_BC | CONVF_R },
        { DXGI_FORMAT_BC5_UNORM,                    8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G },
        { DXGI_FORMAT_BC5_SNORM,                    8, CONVF_SNORM | CONVF_BC | CONVF_R | CONVF_G },
        { DXGI_FORMAT_B5G6R5_UNORM,                 5, CONVF_UNORM | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_B5G5R5A1_UNORM,               5, CONVF_UNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_B8G8R8A8_UNORM,               8, CONVF_UNORM | CONVF_BGR | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_B8G8R8X8_UNORM,               8, CONVF_UNORM | CONVF_BGR | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,   10, CONVF_UNORM | CONVF_XR | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,          8, CONVF_UNORM | CONVF_BGR | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,          8, CONVF_UNORM | CONVF_BGR | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_BC6H_UF16,                    16, CONVF_FLOAT | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC6H_SF16,                    16, CONVF_FLOAT | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC7_UNORM,                    8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_BC7_UNORM_SRGB,               8, CONVF_UNORM | CONVF_BC | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_AYUV,                         8, CONVF_UNORM | CONVF_YUV | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_Y410,                         10, CONVF_UNORM | CONVF_YUV | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_Y416,                         16, CONVF_UNORM | CONVF_YUV | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { DXGI_FORMAT_YUY2,                         8, CONVF_UNORM | CONVF_YUV | CONVF_PACKED | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_Y210,                         10, CONVF_UNORM | CONVF_YUV | CONVF_PACKED | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_Y216,                         16, CONVF_UNORM | CONVF_YUV | CONVF_PACKED | CONVF_R | CONVF_G | CONVF_B },
        { DXGI_FORMAT_B4G4R4A4_UNORM,               4, CONVF_UNORM | CONVF_BGR | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { XBOX_DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT,  10, CONVF_FLOAT | CONVF_POS_ONLY | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { XBOX_DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT,  10, CONVF_FLOAT | CONVF_POS_ONLY | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM,10, CONVF_SNORM | CONVF_R | CONVF_G | CONVF_B | CONVF_A },
        { XBOX_DXGI_FORMAT_R4G4_UNORM,               4, CONVF_UNORM | CONVF_R | CONVF_G },
    };

#pragma prefast( suppress : 25004, "Signature must match bsearch_s" );
    int __cdecl _ConvertCompare(void *context, const void* ptr1, const void *ptr2)
    {
        UNREFERENCED_PARAMETER(context);
        const ConvertData *p1 = reinterpret_cast<const ConvertData*>(ptr1);
        const ConvertData *p2 = reinterpret_cast<const ConvertData*>(ptr2);
        if (p1->format == p2->format) return 0;
        else return (p1->format < p2->format) ? -1 : 1;
    }
}

_Use_decl_annotations_
DWORD DirectX::_GetConvertFlags(DXGI_FORMAT format)
{
#ifdef _DEBUG
    // Ensure conversion table is in ascending order
    assert(_countof(g_ConvertTable) > 0);
    DXGI_FORMAT lastvalue = g_ConvertTable[0].format;
    for (size_t index = 1; index < _countof(g_ConvertTable); ++index)
    {
        assert(g_ConvertTable[index].format > lastvalue);
        lastvalue = g_ConvertTable[index].format;
    }
#endif

    ConvertData key = { format, 0 };
    const ConvertData* in = (const ConvertData*)bsearch_s(&key, g_ConvertTable, _countof(g_ConvertTable), sizeof(ConvertData),
        _ConvertCompare, nullptr);
    return (in) ? in->flags : 0;
}

