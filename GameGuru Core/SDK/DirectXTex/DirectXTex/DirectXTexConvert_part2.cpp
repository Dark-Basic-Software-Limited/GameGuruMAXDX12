_Use_decl_annotations_
void DirectX::_ConvertScanline(
    XMVECTOR* pBuffer,
    size_t count,
    DXGI_FORMAT outFormat,
    DXGI_FORMAT inFormat,
    DWORD flags)
{
    assert(pBuffer && count > 0 && (((uintptr_t)pBuffer & 0xF) == 0));
    assert(IsValid(outFormat) && !IsTypeless(outFormat) && !IsPlanar(outFormat) && !IsPalettized(outFormat));
    assert(IsValid(inFormat) && !IsTypeless(inFormat) && !IsPlanar(inFormat) && !IsPalettized(inFormat));

    if (!pBuffer)
        return;

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

    // Determine conversion details about source and dest formats
    ConvertData key = { inFormat, 0 };
    const ConvertData* in = (const ConvertData*)bsearch_s(&key, g_ConvertTable, _countof(g_ConvertTable), sizeof(ConvertData),
        _ConvertCompare, nullptr);
    key.format = outFormat;
    const ConvertData* out = (const ConvertData*)bsearch_s(&key, g_ConvertTable, _countof(g_ConvertTable), sizeof(ConvertData),
        _ConvertCompare, nullptr);
    if (!in || !out)
    {
        assert(false);
        return;
    }

    assert(_GetConvertFlags(inFormat) == in->flags);
    assert(_GetConvertFlags(outFormat) == out->flags);

    // Handle SRGB filtering modes
    switch (inFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        flags |= TEX_FILTER_SRGB_IN;
        break;

    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        flags &= ~TEX_FILTER_SRGB_IN;
        break;

    default:
        break;
    }

    switch (outFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        flags |= TEX_FILTER_SRGB_OUT;
        break;

    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        flags &= ~TEX_FILTER_SRGB_OUT;
        break;

    default:
        break;
    }

    if ((flags & (TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT)) == (TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT))
    {
        flags &= ~(TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT);
    }

    // sRGB input processing (sRGB -> Linear RGB)
    if (flags & TEX_FILTER_SRGB_IN)
    {
        if (!(in->flags & CONVF_DEPTH) && ((in->flags & CONVF_FLOAT) || (in->flags & CONVF_UNORM)))
        {
            XMVECTOR* ptr = pBuffer;
            for (size_t i = 0; i < count; ++i, ++ptr)
            {
                *ptr = XMColorSRGBToRGB(*ptr);
            }
        }
    }

    // Handle conversion special cases
    DWORD diffFlags = in->flags ^ out->flags;
    if (diffFlags != 0)
    {
        if (diffFlags & CONVF_DEPTH)
        {
            //--- Depth conversions ---
            if (in->flags & CONVF_DEPTH)
            {
                // CONVF_DEPTH -> !CONVF_DEPTH
                if (in->flags & CONVF_STENCIL)
                {
                    // Stencil -> Alpha
                    static const XMVECTORF32 S = { { { 1.f, 1.f, 1.f, 255.f } } };

                    if (out->flags & CONVF_UNORM)
                    {
                        // UINT -> UNORM
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorSplatY(v);
                            v1 = XMVectorClamp(v1, g_XMZero, S);
                            v1 = XMVectorDivide(v1, S);
                            *ptr++ = XMVectorSelect(v1, v, g_XMSelect1110);
                        }
                    }
                    else if (out->flags & CONVF_SNORM)
                    {
                        // UINT -> SNORM
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorSplatY(v);
                            v1 = XMVectorClamp(v1, g_XMZero, S);
                            v1 = XMVectorDivide(v1, S);
                            v1 = XMVectorMultiplyAdd(v1, g_XMTwo, g_XMNegativeOne);
                            *ptr++ = XMVectorSelect(v1, v, g_XMSelect1110);
                        }
                    }
                    else
                    {
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorSplatY(v);
                            *ptr++ = XMVectorSelect(v1, v, g_XMSelect1110);
                        }
                    }
                }

                // Depth -> RGB
                if ((out->flags & CONVF_UNORM) && (in->flags & CONVF_FLOAT))
                {
                    // Depth FLOAT -> UNORM
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSaturate(v);
                        v1 = XMVectorSplatX(v1);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                    }
                }
                else if (out->flags & CONVF_SNORM)
                {
                    if (in->flags & CONVF_UNORM)
                    {
                        // Depth UNORM -> SNORM
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorMultiplyAdd(v, g_XMTwo, g_XMNegativeOne);
                            v1 = XMVectorSplatX(v1);
                            *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                        }
                    }
                    else
                    {
                        // Depth FLOAT -> SNORM
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorClamp(v, g_XMNegativeOne, g_XMOne);
                            v1 = XMVectorSplatX(v1);
                            *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                        }
                    }
                }
                else
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSplatX(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                    }
                }
            }
            else
            {
                // !CONVF_DEPTH -> CONVF_DEPTH

                // RGB -> Depth (red channel)
                switch (flags & (TEX_FILTER_RGB_COPY_RED | TEX_FILTER_RGB_COPY_GREEN | TEX_FILTER_RGB_COPY_BLUE))
                {
                case TEX_FILTER_RGB_COPY_GREEN:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSplatY(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                    }
                }
                break;

                case TEX_FILTER_RGB_COPY_BLUE:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSplatZ(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                    }
                }
                break;

                default:
                    if ((in->flags & CONVF_UNORM) && ((in->flags & CONVF_RGB_MASK) == (CONVF_R | CONVF_G | CONVF_B)))
                    {
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVector3Dot(v, g_Grayscale);
                            *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                        }
                        break;
                    }

                    __fallthrough;

                case TEX_FILTER_RGB_COPY_RED:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSplatX(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                    }
                }
                break;
                }

                // Finialize type conversion for depth (red channel)
                if (out->flags & CONVF_UNORM)
                {
                    if (in->flags & CONVF_SNORM)
                    {
                        // SNORM -> UNORM
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
                            *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                        }
                    }
                    else if (in->flags & CONVF_FLOAT)
                    {
                        // FLOAT -> UNORM
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorSaturate(v);
                            *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                        }
                    }
                }

                if (out->flags & CONVF_STENCIL)
                {
                    // Alpha -> Stencil (green channel)
                    static const XMVECTORU32 select0100 = { { { XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0 } } };
                    static const XMVECTORF32 S = { { { 255.f, 255.f, 255.f, 255.f } } };

                    if (in->flags & CONVF_UNORM)
                    {
                        // UNORM -> UINT
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorMultiply(v, S);
                            v1 = XMVectorSplatW(v1);
                            *ptr++ = XMVectorSelect(v, v1, select0100);
                        }
                    }
                    else if (in->flags & CONVF_SNORM)
                    {
                        // SNORM -> UINT
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
                            v1 = XMVectorMultiply(v1, S);
                            v1 = XMVectorSplatW(v1);
                            *ptr++ = XMVectorSelect(v, v1, select0100);
                        }
                    }
                    else
                    {
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVectorSplatW(v);
                            *ptr++ = XMVectorSelect(v, v1, select0100);
                        }
                    }
                }
            }
        }
        else if (out->flags & CONVF_DEPTH)
        {
            // CONVF_DEPTH -> CONVF_DEPTH
            if (diffFlags & CONVF_FLOAT)
            {
                if (in->flags & CONVF_FLOAT)
                {
                    // FLOAT -> UNORM depth, preserve stencil
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSaturate(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1000);
                    }
                }
            }
        }
        else if (out->flags & CONVF_UNORM)
        {
            //--- Converting to a UNORM ---
            if (in->flags & CONVF_SNORM)
            {
                // SNORM -> UNORM
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    *ptr++ = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
                }
            }
            else if (in->flags & CONVF_FLOAT)
            {
                XMVECTOR* ptr = pBuffer;
                if (!(in->flags & CONVF_POS_ONLY) && (flags & TEX_FILTER_FLOAT_X2BIAS))
                {
                    // FLOAT -> UNORM (x2 bias)
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        v = XMVectorClamp(v, g_XMNegativeOne, g_XMOne);
                        *ptr++ = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
                    }
                }
                else
                {
                    // FLOAT -> UNORM
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        *ptr++ = XMVectorSaturate(v);
                    }
                }
            }
        }
        else if (out->flags & CONVF_SNORM)
        {
            //--- Converting to a SNORM ---
            if (in->flags & CONVF_UNORM)
            {
                // UNORM -> SNORM
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    *ptr++ = XMVectorMultiplyAdd(v, g_XMTwo, g_XMNegativeOne);
                }
            }
            else if (in->flags & CONVF_FLOAT)
            {
                XMVECTOR* ptr = pBuffer;
                if ((in->flags & CONVF_POS_ONLY) && (flags & TEX_FILTER_FLOAT_X2BIAS))
                {
                    // FLOAT (positive only, x2 bias) -> SNORM
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        v = XMVectorSaturate(v);
                        *ptr++ = XMVectorMultiplyAdd(v, g_XMTwo, g_XMNegativeOne);
                    }
                }
                else
                {
                    // FLOAT -> SNORM
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        *ptr++ = XMVectorClamp(v, g_XMNegativeOne, g_XMOne);
                    }
                }
            }
        }
        else if (diffFlags & CONVF_UNORM)
        {
            //--- Converting from a UNORM ---
            assert(in->flags & CONVF_UNORM);
            if (out->flags & CONVF_FLOAT)
            {
                if (!(out->flags & CONVF_POS_ONLY) && (flags & TEX_FILTER_FLOAT_X2BIAS))
                {
                    // UNORM (x2 bias) -> FLOAT
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        *ptr++ = XMVectorMultiplyAdd(v, g_XMTwo, g_XMNegativeOne);
                    }
                }
            }
        }
        else if (diffFlags & CONVF_POS_ONLY)
        {
            if (flags & TEX_FILTER_FLOAT_X2BIAS)
            {
                if (in->flags & CONVF_POS_ONLY)
                {
                    if (out->flags & CONVF_FLOAT)
                    {
                        // FLOAT (positive only, x2 bias) -> FLOAT
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            v = XMVectorSaturate(v);
                            *ptr++ = XMVectorMultiplyAdd(v, g_XMTwo, g_XMNegativeOne);
                        }
                    }
                }
                else if (out->flags & CONVF_POS_ONLY)
                {
                    if (in->flags & CONVF_FLOAT)
                    {
                        // FLOAT -> FLOAT (positive only, x2 bias)
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            v = XMVectorClamp(v, g_XMNegativeOne, g_XMOne);
                            *ptr++ = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
                        }
                    }
                    else if (in->flags & CONVF_SNORM)
                    {
                        // SNORM -> FLOAT (positive only, x2 bias)
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            *ptr++ = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
                        }
                    }
                }
            }
        }

        // !CONVF_A -> CONVF_A is handled because LoadScanline ensures alpha defaults to 1.0 for no-alpha formats

        // CONVF_PACKED cases are handled because LoadScanline/StoreScanline handles packing/unpacking

        if (((out->flags & CONVF_RGBA_MASK) == CONVF_A) && !(in->flags & CONVF_A))
        {
            // !CONVF_A -> A format
            switch (flags & (TEX_FILTER_RGB_COPY_RED | TEX_FILTER_RGB_COPY_GREEN | TEX_FILTER_RGB_COPY_BLUE))
            {
            case TEX_FILTER_RGB_COPY_GREEN:
            {
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    *ptr++ = XMVectorSplatY(v);
                }
            }
            break;

            case TEX_FILTER_RGB_COPY_BLUE:
            {
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    *ptr++ = XMVectorSplatZ(v);
                }
            }
            break;

            default:
                if ((in->flags & CONVF_UNORM) && ((in->flags & CONVF_RGB_MASK) == (CONVF_R | CONVF_G | CONVF_B)))
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        *ptr++ = XMVector3Dot(v, g_Grayscale);
                    }
                    break;
                }

                __fallthrough;

            case TEX_FILTER_RGB_COPY_RED:
            {
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    *ptr++ = XMVectorSplatX(v);
                }
            }
            break;
            }
        }
        else if (((in->flags & CONVF_RGBA_MASK) == CONVF_A) && !(out->flags & CONVF_A))
        {
            // A format -> !CONVF_A
            XMVECTOR* ptr = pBuffer;
            for (size_t i = 0; i < count; ++i)
            {
                XMVECTOR v = *ptr;
                *ptr++ = XMVectorSplatW(v);
            }
        }
        else if ((in->flags & CONVF_RGB_MASK) == CONVF_R)
        {
            if ((out->flags & CONVF_RGB_MASK) == (CONVF_R | CONVF_G | CONVF_B))
            {
                // R format -> RGB format
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    XMVECTOR v1 = XMVectorSplatX(v);
                    *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                }
            }
            else if ((out->flags & CONVF_RGB_MASK) == (CONVF_R | CONVF_G))
            {
                // R format -> RG format
                XMVECTOR* ptr = pBuffer;
                for (size_t i = 0; i < count; ++i)
                {
                    XMVECTOR v = *ptr;
                    XMVECTOR v1 = XMVectorSplatX(v);
                    *ptr++ = XMVectorSelect(v, v1, g_XMSelect1100);
                }
            }
        }
        else if ((in->flags & CONVF_RGB_MASK) == (CONVF_R | CONVF_G | CONVF_B))
        {
            if ((out->flags & CONVF_RGB_MASK) == CONVF_R)
            {
                // RGB format -> R format
                switch (flags & (TEX_FILTER_RGB_COPY_RED | TEX_FILTER_RGB_COPY_GREEN | TEX_FILTER_RGB_COPY_BLUE))
                {
                case TEX_FILTER_RGB_COPY_GREEN:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSplatY(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                    }
                }
                break;

                case TEX_FILTER_RGB_COPY_BLUE:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSplatZ(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                    }
                }
                break;

                default:
                    if (in->flags & CONVF_UNORM)
                    {
                        XMVECTOR* ptr = pBuffer;
                        for (size_t i = 0; i < count; ++i)
                        {
                            XMVECTOR v = *ptr;
                            XMVECTOR v1 = XMVector3Dot(v, g_Grayscale);
                            *ptr++ = XMVectorSelect(v, v1, g_XMSelect1110);
                        }
                        break;
                    }

                    __fallthrough;

                case TEX_FILTER_RGB_COPY_RED:
                    // Leave data unchanged and the store will handle this...
                    break;
                }
            }
            else if ((out->flags & CONVF_RGB_MASK) == (CONVF_R | CONVF_G))
            {
                // RGB format -> RG format
                switch (flags & (TEX_FILTER_RGB_COPY_RED | TEX_FILTER_RGB_COPY_GREEN | TEX_FILTER_RGB_COPY_BLUE))
                {
                case TEX_FILTER_RGB_COPY_RED | TEX_FILTER_RGB_COPY_BLUE:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSwizzle<0, 2, 0, 2>(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1100);
                    }
                }
                break;

                case TEX_FILTER_RGB_COPY_GREEN | TEX_FILTER_RGB_COPY_BLUE:
                {
                    XMVECTOR* ptr = pBuffer;
                    for (size_t i = 0; i < count; ++i)
                    {
                        XMVECTOR v = *ptr;
                        XMVECTOR v1 = XMVectorSwizzle<1, 2, 3, 0>(v);
                        *ptr++ = XMVectorSelect(v, v1, g_XMSelect1100);
                    }
                }
                break;

                case TEX_FILTER_RGB_COPY_RED | TEX_FILTER_RGB_COPY_GREEN:
                default:
                    // Leave data unchanged and the store will handle this...
                    break;
                }
            }
        }
    }

    // sRGB output processing (Linear RGB -> sRGB)
    if (flags & TEX_FILTER_SRGB_OUT)
    {
        if (!(out->flags & CONVF_DEPTH) && ((out->flags & CONVF_FLOAT) || (out->flags & CONVF_UNORM)))
        {
            XMVECTOR* ptr = pBuffer;
            for (size_t i = 0; i < count; ++i, ++ptr)
            {
                *ptr = XMColorRGBToSRGB(*ptr);
            }
        }
    }
}


//-------------------------------------------------------------------------------------
// Dithering
//-------------------------------------------------------------------------------------
namespace
{
    // 4X4X4 ordered dithering matrix
    const float g_Dither[] =
    {
        // (z & 3) + ( (y & 3) * 8) + (x & 3)
        0.468750f,  -0.031250f, 0.343750f, -0.156250f, 0.468750f, -0.031250f, 0.343750f, -0.156250f,
        -0.281250f,  0.218750f, -0.406250f, 0.093750f, -0.281250f, 0.218750f, -0.406250f, 0.093750f,
        0.281250f,  -0.218750f, 0.406250f, -0.093750f, 0.281250f, -0.218750f, 0.406250f, -0.093750f,
        -0.468750f,  0.031250f, -0.343750f, 0.156250f, -0.468750f, 0.031250f, -0.343750f, 0.156250f,
    };

    const XMVECTORF32 g_Scale16pc   = { { { 65535.f, 65535.f, 65535.f, 65535.f } } };
    const XMVECTORF32 g_Scale15pc   = { { { 32767.f, 32767.f, 32767.f, 32767.f } } };
    const XMVECTORF32 g_Scale10pc   = { { {  1023.f,  1023.f,  1023.f,     3.f } } };
    const XMVECTORF32 g_Scale9pc    = { { {   511.f,   511.f,   511.f,     3.f } } };
    const XMVECTORF32 g_Scale8pc    = { { {   255.f,   255.f,   255.f,   255.f } } };
    const XMVECTORF32 g_Scale7pc    = { { {   127.f,   127.f,   127.f,   127.f } } };
    const XMVECTORF32 g_Scale565pc  = { { {    31.f,    63.f,    31.f,     1.f } } };
    const XMVECTORF32 g_Scale5551pc = { { {    31.f,    31.f,    31.f,     1.f } } };
    const XMVECTORF32 g_Scale4pc    = { { {    15.f,    15.f,    15.f,    15.f } } };

    const XMVECTORF32 g_ErrorWeight3 = { { { 3.f / 16.f, 3.f / 16.f, 3.f / 16.f, 3.f / 16.f } } };
    const XMVECTORF32 g_ErrorWeight5 = { { { 5.f / 16.f, 5.f / 16.f, 5.f / 16.f, 5.f / 16.f } } };
    const XMVECTORF32 g_ErrorWeight1 = { { { 1.f / 16.f, 1.f / 16.f, 1.f / 16.f, 1.f / 16.f } } };
    const XMVECTORF32 g_ErrorWeight7 = { { { 7.f / 16.f, 7.f / 16.f, 7.f / 16.f, 7.f / 16.f } } };

#define STORE_SCANLINE( type, scalev, clampzero, norm, itype, mask, row, bgr ) \
        if ( size >= sizeof(type) ) \
        { \
            type * __restrict dest = reinterpret_cast<type*>(pDestination); \
            for( size_t i = 0; i < count; ++i ) \
            { \
                ptrdiff_t index = static_cast<ptrdiff_t>( ( row & 1 ) ? ( count - i - 1 ) : i ); \
                ptrdiff_t delta = ( row & 1 ) ? -2 : 0; \
                \
                XMVECTOR v = sPtr[ index ]; \
                if ( bgr ) { v = XMVectorSwizzle<2, 1, 0, 3>( v ); } \
                if ( norm && clampzero ) v = XMVectorSaturate( v ) ; \
                else if ( clampzero ) v = XMVectorClamp( v, g_XMZero, scalev ); \
                else if ( norm ) v = XMVectorClamp( v, g_XMNegativeOne, g_XMOne ); \
                else v = XMVectorClamp( v, -scalev + g_XMOne, scalev ); \
                v = XMVectorAdd( v, vError ); \
                if ( norm ) v = XMVectorMultiply( v, scalev ); \
                \
                XMVECTOR target; \
                if ( pDiffusionErrors ) \
                { \
                    target = XMVectorRound( v ); \
                    vError = XMVectorSubtract( v, target ); \
                    if (norm) vError = XMVectorDivide( vError, scalev ); \
                    \
                    /* Distribute error to next scanline and next pixel */ \
                    pDiffusionErrors[ index-delta ]   += XMVectorMultiply( g_ErrorWeight3, vError ); \
                    pDiffusionErrors[ index+1 ]       += XMVectorMultiply( g_ErrorWeight5, vError ); \
                    pDiffusionErrors[ index+2+delta ] += XMVectorMultiply( g_ErrorWeight1, vError ); \
                    vError = XMVectorMultiply( vError, g_ErrorWeight7 ); \
                } \
                else \
                { \
                    /* Applied ordered dither */ \
                    target = XMVectorAdd( v, ordered[ index & 3 ] ); \
                    target = XMVectorRound( target ); \
                } \
                \
                target = XMVectorMin( scalev, target ); \
                target = XMVectorMax( (clampzero) ? g_XMZero : ( -scalev + g_XMOne ), target ); \
                \
                XMFLOAT4A tmp; \
                XMStoreFloat4A( &tmp, target ); \
                \
                auto dPtr = &dest[ index ]; \
                if (dPtr >= ePtr) break; \
                dPtr->x = static_cast<itype>( tmp.x ) & mask; \
                dPtr->y = static_cast<itype>( tmp.y ) & mask; \
                dPtr->z = static_cast<itype>( tmp.z ) & mask; \
                dPtr->w = static_cast<itype>( tmp.w ) & mask; \
            } \
            return true; \
        } \
        return false;

#define STORE_SCANLINE2( type, scalev, clampzero, norm, itype, mask, row ) \
        /* The 2 component cases are always bgr=false */ \
        if ( size >= sizeof(type) ) \
        { \
            type * __restrict dest = reinterpret_cast<type*>(pDestination); \
            for( size_t i = 0; i < count; ++i ) \
            { \
                ptrdiff_t index = static_cast<ptrdiff_t>( ( row & 1 ) ? ( count - i - 1 ) : i ); \
                ptrdiff_t delta = ( row & 1 ) ? -2 : 0; \
                \
                XMVECTOR v = sPtr[ index ]; \
                if ( norm && clampzero ) v = XMVectorSaturate( v ) ; \
                else if ( clampzero ) v = XMVectorClamp( v, g_XMZero, scalev ); \
                else if ( norm ) v = XMVectorClamp( v, g_XMNegativeOne, g_XMOne ); \
                else v = XMVectorClamp( v, -scalev + g_XMOne, scalev ); \
                v = XMVectorAdd( v, vError ); \
                if ( norm ) v = XMVectorMultiply( v, scalev ); \
                \
                XMVECTOR target; \
                if ( pDiffusionErrors ) \
                { \
                    target = XMVectorRound( v ); \
                    vError = XMVectorSubtract( v, target ); \
                    if (norm) vError = XMVectorDivide( vError, scalev ); \
                    \
                    /* Distribute error to next scanline and next pixel */ \
                    pDiffusionErrors[ index-delta ]   += XMVectorMultiply( g_ErrorWeight3, vError ); \
                    pDiffusionErrors[ index+1 ]       += XMVectorMultiply( g_ErrorWeight5, vError ); \
                    pDiffusionErrors[ index+2+delta ] += XMVectorMultiply( g_ErrorWeight1, vError ); \
                    vError = XMVectorMultiply( vError, g_ErrorWeight7 ); \
                } \
                else \
                { \
                    /* Applied ordered dither */ \
                    target = XMVectorAdd( v, ordered[ index & 3 ] ); \
                    target = XMVectorRound( target ); \
                } \
                \
                target = XMVectorMin( scalev, target ); \
                target = XMVectorMax( (clampzero) ? g_XMZero : ( -scalev + g_XMOne ), target ); \
                \
                XMFLOAT4A tmp; \
                XMStoreFloat4A( &tmp, target ); \
                \
                auto dPtr = &dest[ index ]; \
                if (dPtr >= ePtr) break; \
                dPtr->x = static_cast<itype>( tmp.x ) & mask; \
                dPtr->y = static_cast<itype>( tmp.y ) & mask; \
            } \
            return true; \
        } \
        return false;

#define STORE_SCANLINE1( type, scalev, clampzero, norm, mask, row, selectw ) \
        /* The 1 component cases are always bgr=false */ \
        if ( size >= sizeof(type) ) \
        { \
            type * __restrict dest = reinterpret_cast<type*>(pDestination); \
            for( size_t i = 0; i < count; ++i ) \
            { \
                ptrdiff_t index = static_cast<ptrdiff_t>( ( row & 1 ) ? ( count - i - 1 ) : i ); \
                ptrdiff_t delta = ( row & 1 ) ? -2 : 0; \
                \
                XMVECTOR v = sPtr[ index ]; \
                if ( norm && clampzero ) v = XMVectorSaturate( v ) ; \
                else if ( clampzero ) v = XMVectorClamp( v, g_XMZero, scalev ); \
                else if ( norm ) v = XMVectorClamp( v, g_XMNegativeOne, g_XMOne ); \
                else v = XMVectorClamp( v, -scalev + g_XMOne, scalev ); \
                v = XMVectorAdd( v, vError ); \
                if ( norm ) v = XMVectorMultiply( v, scalev ); \
                \
                XMVECTOR target; \
                if ( pDiffusionErrors ) \
                { \
                    target = XMVectorRound( v ); \
                    vError = XMVectorSubtract( v, target ); \
                    if (norm) vError = XMVectorDivide( vError, scalev ); \
                    \
                    /* Distribute error to next scanline and next pixel */ \
                    pDiffusionErrors[ index-delta ]   += XMVectorMultiply( g_ErrorWeight3, vError ); \
                    pDiffusionErrors[ index+1 ]       += XMVectorMultiply( g_ErrorWeight5, vError ); \
                    pDiffusionErrors[ index+2+delta ] += XMVectorMultiply( g_ErrorWeight1, vError ); \
                    vError = XMVectorMultiply( vError, g_ErrorWeight7 ); \
                } \
                else \
                { \
                    /* Applied ordered dither */ \
                    target = XMVectorAdd( v, ordered[ index & 3 ] ); \
                    target = XMVectorRound( target ); \
                } \
                \
                target = XMVectorMin( scalev, target ); \
                target = XMVectorMax( (clampzero) ? g_XMZero : ( -scalev + g_XMOne ), target ); \
                \
                auto dPtr = &dest[ index ]; \
                if (dPtr >= ePtr) break; \
                *dPtr = static_cast<type>( (selectw) ? XMVectorGetW( target ) : XMVectorGetX( target ) ) & mask; \
            } \
            return true; \
        } \
        return false;
}

#pragma warning(push)
#pragma warning( disable : 4127 )

_Use_decl_annotations_
bool DirectX::_StoreScanlineDither(
    void* pDestination,
    size_t size,
    DXGI_FORMAT format,
    XMVECTOR* pSource,
    size_t count,
    float threshold,
    size_t y,
    size_t z,
    XMVECTOR* pDiffusionErrors)
{
    assert(pDestination && size > 0);
    assert(pSource && count > 0 && (((uintptr_t)pSource & 0xF) == 0));
    assert(IsValid(format) && !IsTypeless(format) && !IsCompressed(format) && !IsPlanar(format) && !IsPalettized(format));

    XMVECTOR ordered[4];
    if (pDiffusionErrors)
    {
        // If pDiffusionErrors != 0, then this function performs error diffusion dithering (aka Floyd-Steinberg dithering)

        // To avoid the need for another temporary scanline buffer, we allow this function to overwrite the source buffer in-place
        // Given the intended usage in the conversion routines, this is not a problem.

        XMVECTOR* ptr = pSource;
        const XMVECTOR* err = pDiffusionErrors + 1;
        for (size_t i = 0; i < count; ++i)
        {
            // Add contribution from previous scanline
            XMVECTOR v = XMVectorAdd(*ptr, *err++);
            *ptr++ = v;
        }

        // Reset errors for next scanline
        memset(pDiffusionErrors, 0, sizeof(XMVECTOR)*(count + 2));
    }
    else
    {
        // If pDiffusionErrors == 0, then this function performs ordered dithering

        XMVECTOR dither = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(g_Dither + (z & 3) + ((y & 3) * 8)));

        ordered[0] = XMVectorSplatX(dither);
        ordered[1] = XMVectorSplatY(dither);
        ordered[2] = XMVectorSplatZ(dither);
        ordered[3] = XMVectorSplatW(dither);
    }

    const XMVECTOR* __restrict sPtr = pSource;
    if (!sPtr)
        return false;

    const void* ePtr = reinterpret_cast<const uint8_t*>(pDestination) + size;

    XMVECTOR vError = XMVectorZero();

    switch (static_cast<int>(format))
    {
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        STORE_SCANLINE(XMUSHORTN4, g_Scale16pc, true, true, uint16_t, 0xFFFF, y, false)

    case DXGI_FORMAT_R16G16B16A16_UINT:
        STORE_SCANLINE(XMUSHORT4, g_Scale16pc, true, false, uint16_t, 0xFFFF, y, false)

    case DXGI_FORMAT_R16G16B16A16_SNORM:
        STORE_SCANLINE(XMSHORTN4, g_Scale15pc, false, true, int16_t, 0xFFFF, y, false)

    case DXGI_FORMAT_R16G16B16A16_SINT:
        STORE_SCANLINE(XMSHORT4, g_Scale15pc, false, false, int16_t, 0xFFFF, y, false)

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        STORE_SCANLINE(XMUDECN4, g_Scale10pc, true, true, uint16_t, 0x3FF, y, false)

    case DXGI_FORMAT_R10G10B10A2_UINT:
        STORE_SCANLINE(XMUDEC4, g_Scale10pc, true, false, uint16_t, 0x3FF, y, false)

    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        if (size >= sizeof(XMUDEC4))
        {
            static const XMVECTORF32  Scale = { { { 510.0f, 510.0f, 510.0f, 3.0f } } };
            static const XMVECTORF32  Bias  = { { { 384.0f, 384.0f, 384.0f, 0.0f } } };
            static const XMVECTORF32  MinXR = { { { -0.7529f, -0.7529f, -0.7529f, 0.f } } };
            static const XMVECTORF32  MaxXR = { { { 1.2529f, 1.2529f, 1.2529f, 1.0f } } };

            XMUDEC4 * __restrict dest = reinterpret_cast<XMUDEC4*>(pDestination);
            for (size_t i = 0; i < count; ++i)
            {
                ptrdiff_t index = static_cast<ptrdiff_t>((y & 1) ? (count - i - 1) : i);
                ptrdiff_t delta = (y & 1) ? -2 : 0;

                XMVECTOR v = XMVectorClamp(sPtr[index], MinXR, MaxXR);
                v = XMVectorMultiplyAdd(v, Scale, vError);

                XMVECTOR target;
                if (pDiffusionErrors)
                {
                    target = XMVectorRound(v);
                    vError = XMVectorSubtract(v, target);
                    vError = XMVectorDivide(vError, Scale);

                    // Distribute error to next scanline and next pixel
                    pDiffusionErrors[index - delta] += XMVectorMultiply(g_ErrorWeight3, vError);
                    pDiffusionErrors[index + 1] += XMVectorMultiply(g_ErrorWeight5, vError);
                    pDiffusionErrors[index + 2 + delta] += XMVectorMultiply(g_ErrorWeight1, vError);
                    vError = XMVectorMultiply(vError, g_ErrorWeight7);
                }
                else
                {
                    // Applied ordered dither
                    target = XMVectorAdd(v, ordered[index & 3]);
                    target = XMVectorRound(target);
                }

                target = XMVectorAdd(target, Bias);
                target = XMVectorClamp(target, g_XMZero, g_Scale10pc);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, target);

                auto dPtr = &dest[index];
                if (dPtr >= ePtr) break;
                dPtr->x = static_cast<uint16_t>(tmp.x) & 0x3FF;
                dPtr->y = static_cast<uint16_t>(tmp.y) & 0x3FF;
                dPtr->z = static_cast<uint16_t>(tmp.z) & 0x3FF;
                dPtr->w = static_cast<uint16_t>(tmp.w);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        STORE_SCANLINE(XMUBYTEN4, g_Scale8pc, true, true, uint8_t, 0xFF, y, false)

    case DXGI_FORMAT_R8G8B8A8_UINT:
        STORE_SCANLINE(XMUBYTE4, g_Scale8pc, true, false, uint8_t, 0xFF, y, false)

    case DXGI_FORMAT_R8G8B8A8_SNORM:
        STORE_SCANLINE(XMBYTEN4, g_Scale7pc, false, true, int8_t, 0xFF, y, false)

    case DXGI_FORMAT_R8G8B8A8_SINT:
        STORE_SCANLINE(XMBYTE4, g_Scale7pc, false, false, int8_t, 0xFF, y, false)

    case DXGI_FORMAT_R16G16_UNORM:
        STORE_SCANLINE2(XMUSHORTN2, g_Scale16pc, true, true, uint16_t, 0xFFFF, y)

    case DXGI_FORMAT_R16G16_UINT:
        STORE_SCANLINE2(XMUSHORT2, g_Scale16pc, true, false, uint16_t, 0xFFFF, y)

    case DXGI_FORMAT_R16G16_SNORM:
        STORE_SCANLINE2(XMSHORTN2, g_Scale15pc, false, true, int16_t, 0xFFFF, y)

    case DXGI_FORMAT_R16G16_SINT:
        STORE_SCANLINE2(XMSHORT2, g_Scale15pc, false, false, int16_t, 0xFFFF, y)

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        if (size >= sizeof(uint32_t))
        {
            static const XMVECTORF32 Clamp  = { { { 1.f,  255.f, 0.f, 0.f } } };
            static const XMVECTORF32 Scale  = { { { 16777215.f,   1.f, 0.f, 0.f } } };
            static const XMVECTORF32 Scale2 = { { { 16777215.f, 255.f, 0.f, 0.f } } };

            uint32_t * __restrict dest = reinterpret_cast<uint32_t*>(pDestination);
            for (size_t i = 0; i < count; ++i)
            {
                ptrdiff_t index = static_cast<ptrdiff_t>((y & 1) ? (count - i - 1) : i);
                ptrdiff_t delta = (y & 1) ? -2 : 0;

                XMVECTOR v = XMVectorClamp(sPtr[index], g_XMZero, Clamp);
                v = XMVectorAdd(v, vError);
                v = XMVectorMultiply(v, Scale);

                XMVECTOR target;
                if (pDiffusionErrors)
                {
                    target = XMVectorRound(v);
                    vError = XMVectorSubtract(v, target);
                    vError = XMVectorDivide(vError, Scale);

                    // Distribute error to next scanline and next pixel
                    pDiffusionErrors[index - delta] += XMVectorMultiply(g_ErrorWeight3, vError);
                    pDiffusionErrors[index + 1] += XMVectorMultiply(g_ErrorWeight5, vError);
                    pDiffusionErrors[index + 2 + delta] += XMVectorMultiply(g_ErrorWeight1, vError);
                    vError = XMVectorMultiply(vError, g_ErrorWeight7);
                }
                else
                {
                    // Applied ordered dither
                    target = XMVectorAdd(v, ordered[index & 3]);
                    target = XMVectorRound(target);
                }

                target = XMVectorClamp(target, g_XMZero, Scale2);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, target);

                auto dPtr = &dest[index];
                if (dPtr >= ePtr) break;
                *dPtr = (static_cast<uint32_t>(tmp.x) & 0xFFFFFF)
                    | ((static_cast<uint32_t>(tmp.y) & 0xFF) << 24);
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_R8G8_UNORM:
        STORE_SCANLINE2(XMUBYTEN2, g_Scale8pc, true, true, uint8_t, 0xFF, y)

    case DXGI_FORMAT_R8G8_UINT:
        STORE_SCANLINE2(XMUBYTE2, g_Scale8pc, true, false, uint8_t, 0xFF, y)

    case DXGI_FORMAT_R8G8_SNORM:
        STORE_SCANLINE2(XMBYTEN2, g_Scale7pc, false, true, int8_t, 0xFF, y)

    case DXGI_FORMAT_R8G8_SINT:
        STORE_SCANLINE2(XMBYTE2, g_Scale7pc, false, false, int8_t, 0xFF, y)

    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        STORE_SCANLINE1(uint16_t, g_Scale16pc, true, true, 0xFFFF, y, false)

    case DXGI_FORMAT_R16_UINT:
        STORE_SCANLINE1(uint16_t, g_Scale16pc, true, false, 0xFFFF, y, false)

    case DXGI_FORMAT_R16_SNORM:
        STORE_SCANLINE1(int16_t, g_Scale15pc, false, true, 0xFFFF, y, false)

    case DXGI_FORMAT_R16_SINT:
        STORE_SCANLINE1(int16_t, g_Scale15pc, false, false, 0xFFFF, y, false)

    case DXGI_FORMAT_R8_UNORM:
        STORE_SCANLINE1(uint8_t, g_Scale8pc, true, true, 0xFF, y, false)

    case DXGI_FORMAT_R8_UINT:
        STORE_SCANLINE1(uint8_t, g_Scale8pc, true, false, 0xFF, y, false)

    case DXGI_FORMAT_R8_SNORM:
        STORE_SCANLINE1(int8_t, g_Scale7pc, false, true, 0xFF, y, false)

    case DXGI_FORMAT_R8_SINT:
        STORE_SCANLINE1(int8_t, g_Scale7pc, false, false, 0xFF, y, false)

    case DXGI_FORMAT_A8_UNORM:
        STORE_SCANLINE1(uint8_t, g_Scale8pc, true, true, 0xFF, y, true)

    case DXGI_FORMAT_B5G6R5_UNORM:
        if (size >= sizeof(XMU565))
        {
            XMU565 * __restrict dest = reinterpret_cast<XMU565*>(pDestination);
            for (size_t i = 0; i < count; ++i)
            {
                ptrdiff_t index = static_cast<ptrdiff_t>((y & 1) ? (count - i - 1) : i);
                ptrdiff_t delta = (y & 1) ? -2 : 0;

                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(sPtr[index]);
                v = XMVectorSaturate(v);
                v = XMVectorAdd(v, vError);
                v = XMVectorMultiply(v, g_Scale565pc);

                XMVECTOR target;
                if (pDiffusionErrors)
                {
                    target = XMVectorRound(v);
                    vError = XMVectorSubtract(v, target);
                    vError = XMVectorDivide(vError, g_Scale565pc);

                    // Distribute error to next scanline and next pixel
                    pDiffusionErrors[index - delta] += XMVectorMultiply(g_ErrorWeight3, vError);
                    pDiffusionErrors[index + 1] += XMVectorMultiply(g_ErrorWeight5, vError);
                    pDiffusionErrors[index + 2 + delta] += XMVectorMultiply(g_ErrorWeight1, vError);
                    vError = XMVectorMultiply(vError, g_ErrorWeight7);
                }
                else
                {
                    // Applied ordered dither
                    target = XMVectorAdd(v, ordered[index & 3]);
                    target = XMVectorRound(target);
                }

                target = XMVectorClamp(target, g_XMZero, g_Scale565pc);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, target);

                auto dPtr = &dest[index];
                if (dPtr >= ePtr) break;
                dPtr->x = static_cast<uint16_t>(tmp.x) & 0x1F;
                dPtr->y = static_cast<uint16_t>(tmp.y) & 0x3F;
                dPtr->z = static_cast<uint16_t>(tmp.z) & 0x1F;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B5G5R5A1_UNORM:
        if (size >= sizeof(XMU555))
        {
            XMU555 * __restrict dest = reinterpret_cast<XMU555*>(pDestination);
            for (size_t i = 0; i < count; ++i)
            {
                ptrdiff_t index = static_cast<ptrdiff_t>((y & 1) ? (count - i - 1) : i);
                ptrdiff_t delta = (y & 1) ? -2 : 0;

                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(sPtr[index]);
                v = XMVectorSaturate(v);
                v = XMVectorAdd(v, vError);
                v = XMVectorMultiply(v, g_Scale5551pc);

                XMVECTOR target;
                if (pDiffusionErrors)
                {
                    target = XMVectorRound(v);
                    vError = XMVectorSubtract(v, target);
                    vError = XMVectorDivide(vError, g_Scale5551pc);

                    // Distribute error to next scanline and next pixel
                    pDiffusionErrors[index - delta] += XMVectorMultiply(g_ErrorWeight3, vError);
                    pDiffusionErrors[index + 1] += XMVectorMultiply(g_ErrorWeight5, vError);
                    pDiffusionErrors[index + 2 + delta] += XMVectorMultiply(g_ErrorWeight1, vError);
                    vError = XMVectorMultiply(vError, g_ErrorWeight7);
                }
                else
                {
                    // Applied ordered dither
                    target = XMVectorAdd(v, ordered[index & 3]);
                    target = XMVectorRound(target);
                }

                target = XMVectorClamp(target, g_XMZero, g_Scale5551pc);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, target);

                auto dPtr = &dest[index];
                if (dPtr >= ePtr) break;
                dPtr->x = static_cast<uint16_t>(tmp.x) & 0x1F;
                dPtr->y = static_cast<uint16_t>(tmp.y) & 0x1F;
                dPtr->z = static_cast<uint16_t>(tmp.z) & 0x1F;
                dPtr->w = (XMVectorGetW(target) > threshold) ? 1 : 0;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        STORE_SCANLINE(XMUBYTEN4, g_Scale8pc, true, true, uint8_t, 0xFF, y, true)

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        if (size >= sizeof(XMUBYTEN4))
        {
            XMUBYTEN4 * __restrict dest = reinterpret_cast<XMUBYTEN4*>(pDestination);
            for (size_t i = 0; i < count; ++i)
            {
                ptrdiff_t index = static_cast<ptrdiff_t>((y & 1) ? (count - i - 1) : i);
                ptrdiff_t delta = (y & 1) ? -2 : 0;

                XMVECTOR v = XMVectorSwizzle<2, 1, 0, 3>(sPtr[index]);
                v = XMVectorSaturate(v);
                v = XMVectorAdd(v, vError);
                v = XMVectorMultiply(v, g_Scale8pc);

                XMVECTOR target;
                if (pDiffusionErrors)
                {
                    target = XMVectorRound(v);
                    vError = XMVectorSubtract(v, target);
                    vError = XMVectorDivide(vError, g_Scale8pc);

                    // Distribute error to next scanline and next pixel
                    pDiffusionErrors[index - delta] += XMVectorMultiply(g_ErrorWeight3, vError);
                    pDiffusionErrors[index + 1] += XMVectorMultiply(g_ErrorWeight5, vError);
                    pDiffusionErrors[index + 2 + delta] += XMVectorMultiply(g_ErrorWeight1, vError);
                    vError = XMVectorMultiply(vError, g_ErrorWeight7);
                }
                else
                {
                    // Applied ordered dither
                    target = XMVectorAdd(v, ordered[index & 3]);
                    target = XMVectorRound(target);
                }

                target = XMVectorClamp(target, g_XMZero, g_Scale8pc);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, target);

                auto dPtr = &dest[index];
                if (dPtr >= ePtr) break;
                dPtr->x = static_cast<uint8_t>(tmp.x) & 0xFF;
                dPtr->y = static_cast<uint8_t>(tmp.y) & 0xFF;
                dPtr->z = static_cast<uint8_t>(tmp.z) & 0xFF;
                dPtr->w = 0;
            }
            return true;
        }
        return false;

    case DXGI_FORMAT_B4G4R4A4_UNORM:
        STORE_SCANLINE(XMUNIBBLE4, g_Scale4pc, true, true, uint8_t, 0xF, y, true)

    case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
        STORE_SCANLINE(XMXDECN4, g_Scale9pc, false, true, uint16_t, 0x3FF, y, false)

    case XBOX_DXGI_FORMAT_R4G4_UNORM:
        if (size >= sizeof(uint8_t))
        {
            uint8_t * __restrict dest = reinterpret_cast<uint8_t*>(pDestination);
            for (size_t i = 0; i < count; ++i)
            {
                ptrdiff_t index = static_cast<ptrdiff_t>((y & 1) ? (count - i - 1) : i);
                ptrdiff_t delta = (y & 1) ? -2 : 0;

                XMVECTOR v = XMVectorSaturate(sPtr[index]);
                v = XMVectorAdd(v, vError);
                v = XMVectorMultiply(v, g_Scale4pc);

                XMVECTOR target;
                if (pDiffusionErrors)
                {
                    target = XMVectorRound(v);
                    vError = XMVectorSubtract(v, target);
                    vError = XMVectorDivide(vError, g_Scale4pc);

                    // Distribute error to next scanline and next pixel
                    pDiffusionErrors[index - delta] += XMVectorMultiply(g_ErrorWeight3, vError);
                    pDiffusionErrors[index + 1] += XMVectorMultiply(g_ErrorWeight5, vError);
                    pDiffusionErrors[index + 2 + delta] += XMVectorMultiply(g_ErrorWeight1, vError);
                    vError = XMVectorMultiply(vError, g_ErrorWeight7);
                }
                else
                {
                    // Applied ordered dither
                    target = XMVectorAdd(v, ordered[index & 3]);
                    target = XMVectorRound(target);
                }

                target = XMVectorClamp(target, g_XMZero, g_Scale4pc);

                XMFLOAT4A tmp;
                XMStoreFloat4A(&tmp, target);

                auto dPtr = &dest[index];
                if (dPtr >= ePtr) break;
                *dPtr = (static_cast<uint8_t>(tmp.x) & 0xF)
                    | ((static_cast<uint8_t>(tmp.y) & 0xF) << 4);
            }
            return true;
        }
        return false;

    default:
        return _StoreScanline(pDestination, size, format, pSource, count, threshold);
    }
}

#pragma warning(pop)

#undef STORE_SCANLINE
#undef STORE_SCANLINE2
#undef STORE_SCANLINE1

namespace
{
    //-------------------------------------------------------------------------------------
    // Selection logic for using WIC vs. our own routines
    //-------------------------------------------------------------------------------------
    inline bool UseWICConversion(
        _In_ DWORD filter,
        _In_ DXGI_FORMAT sformat,
        _In_ DXGI_FORMAT tformat,
        _Out_ WICPixelFormatGUID& pfGUID,
        _Out_ WICPixelFormatGUID& targetGUID)
    {
        memcpy(&pfGUID, &GUID_NULL, sizeof(GUID));
        memcpy(&targetGUID, &GUID_NULL, sizeof(GUID));

        if (filter & TEX_FILTER_FORCE_NON_WIC)
        {
            // Explicit flag indicates use of non-WIC code paths
            return false;
        }

        if (!_DXGIToWIC(sformat, pfGUID) || !_DXGIToWIC(tformat, targetGUID))
        {
            // Source or target format are not WIC supported native pixel formats
            return false;
        }

        if (filter & TEX_FILTER_FORCE_WIC)
        {
            // Explicit flag to use WIC code paths, skips all the case checks below
            return true;
        }

        if (filter & TEX_FILTER_SEPARATE_ALPHA)
        {
            // Alpha is not premultiplied, so use non-WIC code paths
            return false;
        }

        if (filter & TEX_FILTER_FLOAT_X2BIAS)
        {
            // X2 Scale & Bias conversions not supported by WIC code paths
            return false;
        }

#if defined(_XBOX_ONE) && defined(_TITLE)
        if (sformat == DXGI_FORMAT_R16G16B16A16_FLOAT
            || sformat == DXGI_FORMAT_R16_FLOAT
            || tformat == DXGI_FORMAT_R16G16B16A16_FLOAT
            || tformat == DXGI_FORMAT_R16_FLOAT)
        {
            // Use non-WIC code paths as these conversions are not supported by Xbox One XDK
            return false;
        }
#endif

        // Check for special cases
        switch (sformat)
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            switch (tformat)
            {
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_D32_FLOAT:
                // WIC converts via UNORM formats and ends up converting colorspaces for these cases
            case DXGI_FORMAT_A8_UNORM:
                // Conversion logic for these kinds of textures is unintuitive for WIC code paths
                return false;

            default:
                break;
            }
            break;

        case DXGI_FORMAT_R16_FLOAT:
            switch (tformat)
            {
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_D32_FLOAT:
                // WIC converts via UNORM formats and ends up converting colorspaces for these cases
            case DXGI_FORMAT_A8_UNORM:
                // Conversion logic for these kinds of textures is unintuitive for WIC code paths
                return false;

            default:
                break;
            }
            break;

        case DXGI_FORMAT_A8_UNORM:
            // Conversion logic for these kinds of textures is unintuitive for WIC code paths
            return false;

        default:
            switch (tformat)
            {
            case DXGI_FORMAT_A8_UNORM:
                // Conversion logic for these kinds of textures is unintuitive for WIC code paths
                return false;

            default:
                break;
            }
        }

        // Check for implicit color space changes
        if (IsSRGB(sformat))
            filter |= TEX_FILTER_SRGB_IN;

        if (IsSRGB(tformat))
            filter |= TEX_FILTER_SRGB_OUT;

        if ((filter & (TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT)) == (TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT))
        {
            filter &= ~(TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT);
        }

        DWORD wicsrgb = _CheckWICColorSpace(pfGUID, targetGUID);

        if (wicsrgb != (filter & (TEX_FILTER_SRGB_IN | TEX_FILTER_SRGB_OUT)))
        {
            // WIC will perform a colorspace conversion we didn't request
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------------------
    // Convert the source image using WIC
    //-------------------------------------------------------------------------------------
    HRESULT ConvertUsingWIC(
        _In_ const Image& srcImage,
        _In_ const WICPixelFormatGUID& pfGUID,
        _In_ const WICPixelFormatGUID& targetGUID,
        _In_ DWORD filter,
        _In_ float threshold,
        _In_ const Image& destImage)
    {
        assert(srcImage.width == destImage.width);
        assert(srcImage.height == destImage.height);

        bool iswic2 = false;
        IWICImagingFactory* pWIC = GetWICFactory(iswic2);
        if (!pWIC)
            return E_NOINTERFACE;

        ComPtr<IWICFormatConverter> FC;
        HRESULT hr = pWIC->CreateFormatConverter(FC.GetAddressOf());
        if (FAILED(hr))
            return hr;

        // Note that WIC conversion ignores the TEX_FILTER_SRGB_IN and TEX_FILTER_SRGB_OUT flags,
        // but also always assumes UNORM <-> FLOAT conversions are changing color spaces sRGB <-> scRGB

        BOOL canConvert = FALSE;
        hr = FC->CanConvert(pfGUID, targetGUID, &canConvert);
        if (FAILED(hr) || !canConvert)
        {
            // This case is not an issue for the subset of WIC formats that map directly to DXGI
            return E_UNEXPECTED;
        }

        ComPtr<IWICBitmap> source;
        hr = pWIC->CreateBitmapFromMemory(static_cast<UINT>(srcImage.width), static_cast<UINT>(srcImage.height), pfGUID,
            static_cast<UINT>(srcImage.rowPitch), static_cast<UINT>(srcImage.slicePitch),
            srcImage.pixels, source.GetAddressOf());
        if (FAILED(hr))
            return hr;

        hr = FC->Initialize(source.Get(), targetGUID, _GetWICDither(filter), nullptr, threshold * 100.f, WICBitmapPaletteTypeMedianCut);
        if (FAILED(hr))
            return hr;

        hr = FC->CopyPixels(0, static_cast<UINT>(destImage.rowPitch), static_cast<UINT>(destImage.slicePitch), destImage.pixels);
        if (FAILED(hr))
            return hr;

        return S_OK;
    }


    //-------------------------------------------------------------------------------------
    // Convert the source image (not using WIC)
    //-------------------------------------------------------------------------------------
    HRESULT ConvertCustom(
        _In_ const Image& srcImage,
        _In_ DWORD filter,
        _In_ const Image& destImage,
        _In_ float threshold,
        size_t z)
    {
        assert(srcImage.width == destImage.width);
        assert(srcImage.height == destImage.height);

        const uint8_t *pSrc = srcImage.pixels;
        uint8_t *pDest = destImage.pixels;
        if (!pSrc || !pDest)
            return E_POINTER;

        size_t width = srcImage.width;

        if (filter & TEX_FILTER_DITHER_DIFFUSION)
        {
            // Error diffusion dithering (aka Floyd-Steinberg dithering)
            ScopedAlignedArrayXMVECTOR scanline(reinterpret_cast<XMVECTOR*>(_aligned_malloc((sizeof(XMVECTOR)*(width * 2 + 2)), 16)));
            if (!scanline)
                return E_OUTOFMEMORY;

            XMVECTOR* pDiffusionErrors = scanline.get() + width;
            memset(pDiffusionErrors, 0, sizeof(XMVECTOR)*(width + 2));

            for (size_t h = 0; h < srcImage.height; ++h)
            {
                if (!_LoadScanline(scanline.get(), width, pSrc, srcImage.rowPitch, srcImage.format))
                    return E_FAIL;

                _ConvertScanline(scanline.get(), width, destImage.format, srcImage.format, filter);

                if (!_StoreScanlineDither(pDest, destImage.rowPitch, destImage.format, scanline.get(), width, threshold, h, z, pDiffusionErrors))
                    return E_FAIL;

                pSrc += srcImage.rowPitch;
                pDest += destImage.rowPitch;
            }
        }
        else
        {
            ScopedAlignedArrayXMVECTOR scanline(reinterpret_cast<XMVECTOR*>(_aligned_malloc((sizeof(XMVECTOR)*width), 16)));
            if (!scanline)
                return E_OUTOFMEMORY;

            if (filter & TEX_FILTER_DITHER)
            {
                // Ordered dithering
                for (size_t h = 0; h < srcImage.height; ++h)
                {
                    if (!_LoadScanline(scanline.get(), width, pSrc, srcImage.rowPitch, srcImage.format))
                        return E_FAIL;

                    _ConvertScanline(scanline.get(), width, destImage.format, srcImage.format, filter);

                    if (!_StoreScanlineDither(pDest, destImage.rowPitch, destImage.format, scanline.get(), width, threshold, h, z, nullptr))
                        return E_FAIL;

                    pSrc += srcImage.rowPitch;
                    pDest += destImage.rowPitch;
                }
            }
            else
            {
                // No dithering
                for (size_t h = 0; h < srcImage.height; ++h)
                {
                    if (!_LoadScanline(scanline.get(), width, pSrc, srcImage.rowPitch, srcImage.format))
                        return E_FAIL;

                    _ConvertScanline(scanline.get(), width, destImage.format, srcImage.format, filter);

                    if (!_StoreScanline(pDest, destImage.rowPitch, destImage.format, scanline.get(), width, threshold))
                        return E_FAIL;

                    pSrc += srcImage.rowPitch;
                    pDest += destImage.rowPitch;
                }
            }
        }

        return S_OK;
    }

    //-------------------------------------------------------------------------------------
    DXGI_FORMAT _PlanarToSingle(_In_ DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_NV11:
            return DXGI_FORMAT_YUY2;

        case DXGI_FORMAT_P010:
            return DXGI_FORMAT_Y210;

        case DXGI_FORMAT_P016:
            return DXGI_FORMAT_Y216;

            // We currently do not support conversion for Xbox One specific 16-bit depth formats

            // We can't do anything with DXGI_FORMAT_420_OPAQUE because it's an opaque blob of bits

            // We don't support conversion of JPEG Hardware decode formats

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    //-------------------------------------------------------------------------------------
    // Convert the image from a planar to non-planar image
    //-------------------------------------------------------------------------------------
#define CONVERT_420_TO_422( srcType, destType )\
        {\
            size_t rowPitch = srcImage.rowPitch;\
            \
            auto sourceE = reinterpret_cast<const srcType*>( pSrc + srcImage.slicePitch );\
            auto pSrcUV = pSrc + ( srcImage.height * rowPitch );\
            \
            for( size_t y = 0; y < srcImage.height; y+= 2 )\
            {\
                auto sPtrY0 = reinterpret_cast<const srcType*>( pSrc );\
                auto sPtrY2 = reinterpret_cast<const srcType*>( pSrc + rowPitch );\
                auto sPtrUV = reinterpret_cast<const srcType*>( pSrcUV );\
                \
                destType * __restrict dPtr0 = reinterpret_cast<destType*>(pDest);\
                destType * __restrict dPtr1 = reinterpret_cast<destType*>(pDest + destImage.rowPitch);\
                \
                for( size_t x = 0; x < srcImage.width; x+= 2 )\
                {\
                    if ( (sPtrUV+1) >= sourceE ) break;\
                    \
                    srcType u = *(sPtrUV++);\
                    srcType v = *(sPtrUV++);\
                    \
                    dPtr0->x = *(sPtrY0++);\
                    dPtr0->y = u;\
                    dPtr0->z = *(sPtrY0++);\
                    dPtr0->w = v;\
                    ++dPtr0;\
                    \
                    dPtr1->x = *(sPtrY2++);\
                    dPtr1->y = u;\
                    dPtr1->z = *(sPtrY2++);\
                    dPtr1->w = v;\
                    ++dPtr1;\
                }\
                \
                pSrc += rowPitch * 2;\
                pSrcUV += rowPitch;\
                \
                pDest += destImage.rowPitch * 2;\
            }\
        }

    HRESULT ConvertToSinglePlane_(_In_ const Image& srcImage, _In_ const Image& destImage)
    {
        assert(srcImage.width == destImage.width);
        assert(srcImage.height == destImage.height);

        const uint8_t *pSrc = srcImage.pixels;
        uint8_t *pDest = destImage.pixels;
        if (!pSrc || !pDest)
            return E_POINTER;

        switch (srcImage.format)
        {
        case DXGI_FORMAT_NV12:
            assert(destImage.format == DXGI_FORMAT_YUY2);
            CONVERT_420_TO_422(uint8_t, XMUBYTEN4);
            return S_OK;

        case DXGI_FORMAT_P010:
            assert(destImage.format == DXGI_FORMAT_Y210);
            CONVERT_420_TO_422(uint16_t, XMUSHORTN4);
            return S_OK;

        case DXGI_FORMAT_P016:
            assert(destImage.format == DXGI_FORMAT_Y216);
            CONVERT_420_TO_422(uint16_t, XMUSHORTN4);
            return S_OK;

        case DXGI_FORMAT_NV11:
            assert(destImage.format == DXGI_FORMAT_YUY2);
            // Convert 4:1:1 to 4:2:2
            {
                size_t rowPitch = srcImage.rowPitch;

                const uint8_t* sourceE = pSrc + srcImage.slicePitch;
                const uint8_t* pSrcUV = pSrc + (srcImage.height * rowPitch);

                for (size_t y = 0; y < srcImage.height; ++y)
                {
                    const uint8_t* sPtrY = pSrc;
                    const uint8_t* sPtrUV = pSrcUV;

                    XMUBYTEN4 * __restrict dPtr = reinterpret_cast<XMUBYTEN4*>(pDest);

                    for (size_t x = 0; x < srcImage.width; x += 4)
                    {
                        if ((sPtrUV + 1) >= sourceE) break;

                        uint8_t u = *(sPtrUV++);
                        uint8_t v = *(sPtrUV++);

                        dPtr->x = *(sPtrY++);
                        dPtr->y = u;
                        dPtr->z = *(sPtrY++);
                        dPtr->w = v;
                        ++dPtr;

                        dPtr->x = *(sPtrY++);
                        dPtr->y = u;
                        dPtr->z = *(sPtrY++);
                        dPtr->w = v;
                        ++dPtr;
                    }

                    pSrc += rowPitch;
                    pSrcUV += (rowPitch >> 1);

                    pDest += destImage.rowPitch;
                }
            }
            return S_OK;

        default:
            return E_UNEXPECTED;
        }
    }

#undef CONVERT_420_TO_422
}


//=====================================================================================
// Entry-points
//=====================================================================================

//-------------------------------------------------------------------------------------
// Convert image
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::Convert(
    const Image& srcImage,
    DXGI_FORMAT format,
    DWORD filter,
    float threshold,
    ScratchImage& image)
{
    if ((srcImage.format == format) || !IsValid(format))
        return E_INVALIDARG;

    if (!srcImage.pixels)
        return E_POINTER;

    if (IsCompressed(srcImage.format) || IsCompressed(format)
        || IsPlanar(srcImage.format) || IsPlanar(format)
        || IsPalettized(srcImage.format) || IsPalettized(format)
        || IsTypeless(srcImage.format) || IsTypeless(format))
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if ((srcImage.width > UINT32_MAX) || (srcImage.height > UINT32_MAX))
        return E_INVALIDARG;

    HRESULT hr = image.Initialize2D(format, srcImage.width, srcImage.height, 1, 1);
    if (FAILED(hr))
        return hr;

    const Image *rimage = image.GetImage(0, 0, 0);
    if (!rimage)
    {
        image.Release();
        return E_POINTER;
    }

    WICPixelFormatGUID pfGUID, targetGUID;
    if (UseWICConversion(filter, srcImage.format, format, pfGUID, targetGUID))
    {
        hr = ConvertUsingWIC(srcImage, pfGUID, targetGUID, filter, threshold, *rimage);
    }
    else
    {
        hr = ConvertCustom(srcImage, filter, *rimage, threshold, 0);
    }

    if (FAILED(hr))
    {
        image.Release();
        return hr;
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
// Convert image (complex)
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::Convert(
    const Image* srcImages,
    size_t nimages,
    const TexMetadata& metadata,
    DXGI_FORMAT format,
    DWORD filter,
    float threshold,
    ScratchImage& result)
{
    if (!srcImages || !nimages || (metadata.format == format) || !IsValid(format))
        return E_INVALIDARG;

    if (IsCompressed(metadata.format) || IsCompressed(format)
        || IsPlanar(metadata.format) || IsPlanar(format)
        || IsPalettized(metadata.format) || IsPalettized(format)
        || IsTypeless(metadata.format) || IsTypeless(format))
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if ((metadata.width > UINT32_MAX) || (metadata.height > UINT32_MAX))
        return E_INVALIDARG;

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

    WICPixelFormatGUID pfGUID, targetGUID;
    bool usewic = !metadata.IsPMAlpha() && UseWICConversion(filter, metadata.format, format, pfGUID, targetGUID);

    switch (metadata.dimension)
    {
    case TEX_DIMENSION_TEXTURE1D:
    case TEX_DIMENSION_TEXTURE2D:
        for (size_t index = 0; index < nimages; ++index)
        {
            const Image& src = srcImages[index];
            if (src.format != metadata.format)
            {
                result.Release();
                return E_FAIL;
            }

            if ((src.width > UINT32_MAX) || (src.height > UINT32_MAX))
            {
                result.Release();
                return E_FAIL;
            }

            const Image& dst = dest[index];
            assert(dst.format == format);

            if (src.width != dst.width || src.height != dst.height)
            {
                result.Release();
                return E_FAIL;
            }

            if (usewic)
            {
                hr = ConvertUsingWIC(src, pfGUID, targetGUID, filter, threshold, dst);
            }
            else
            {
                hr = ConvertCustom(src, filter, dst, threshold, 0);
            }

            if (FAILED(hr))
            {
                result.Release();
                return hr;
            }
        }
        break;

    case TEX_DIMENSION_TEXTURE3D:
    {
        size_t index = 0;
        size_t d = metadata.depth;
        for (size_t level = 0; level < metadata.mipLevels; ++level)
        {
            for (size_t slice = 0; slice < d; ++slice, ++index)
            {
                if (index >= nimages)
                {
                    result.Release();
                    return E_FAIL;
                }

                const Image& src = srcImages[index];
                if (src.format != metadata.format)
                {
                    result.Release();
                    return E_FAIL;
                }

                if ((src.width > UINT32_MAX) || (src.height > UINT32_MAX))
                {
                    result.Release();
                    return E_FAIL;
                }

                const Image& dst = dest[index];
                assert(dst.format == format);

                if (src.width != dst.width || src.height != dst.height)
                {
                    result.Release();
                    return E_FAIL;
                }

                if (usewic)
                {
                    hr = ConvertUsingWIC(src, pfGUID, targetGUID, filter, threshold, dst);
                }
                else
                {
                    hr = ConvertCustom(src, filter, dst, threshold, slice);
                }

                if (FAILED(hr))
                {
                    result.Release();
                    return hr;
                }
            }

            if (d > 1)
                d >>= 1;
        }
    }
    break;

    default:
        result.Release();
        return E_FAIL;
    }

    return S_OK;
}


