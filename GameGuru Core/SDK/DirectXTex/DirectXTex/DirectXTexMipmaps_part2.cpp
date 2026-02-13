//=====================================================================================
// Entry-points
//=====================================================================================

//-------------------------------------------------------------------------------------
// Generate mipmap chain
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::GenerateMipMaps(
    const Image& baseImage,
    DWORD filter,
    size_t levels,
    ScratchImage& mipChain,
    bool allow1D)
{
    if (!IsValid(baseImage.format))
        return E_INVALIDARG;

    if (!baseImage.pixels)
        return E_POINTER;

    if (!_CalculateMipLevels(baseImage.width, baseImage.height, levels))
        return E_INVALIDARG;

    if (levels <= 1)
        return E_INVALIDARG;

    if (IsCompressed(baseImage.format) || IsTypeless(baseImage.format) || IsPlanar(baseImage.format) || IsPalettized(baseImage.format))
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    HRESULT hr;

    static_assert(TEX_FILTER_POINT == 0x100000, "TEX_FILTER_ flag values don't match TEX_FILTER_MASK");

    if (UseWICFiltering(baseImage.format, filter))
    {
        //--- Use WIC filtering to generate mipmaps -----------------------------------
        switch (filter & TEX_FILTER_MASK)
        {
        case 0:
        case TEX_FILTER_POINT:
        case TEX_FILTER_FANT: // Equivalent to Box filter
        case TEX_FILTER_LINEAR:
        case TEX_FILTER_CUBIC:
        {
            static_assert(TEX_FILTER_FANT == TEX_FILTER_BOX, "TEX_FILTER_ flag alias mismatch");

            WICPixelFormatGUID pfGUID;
            if (_DXGIToWIC(baseImage.format, pfGUID, true))
            {
                // Case 1: Base image format is supported by Windows Imaging Component
                hr = (baseImage.height > 1 || !allow1D)
                    ? mipChain.Initialize2D(baseImage.format, baseImage.width, baseImage.height, 1, levels)
                    : mipChain.Initialize1D(baseImage.format, baseImage.width, 1, levels);
                if (FAILED(hr))
                    return hr;

                return GenerateMipMapsUsingWIC(baseImage, filter, levels, pfGUID, mipChain, 0);
            }
            else
            {
                // Case 2: Base image format is not supported by WIC, so we have to convert, generate, and convert back
                assert(baseImage.format != DXGI_FORMAT_R32G32B32A32_FLOAT);
                ScratchImage temp;
                hr = _ConvertToR32G32B32A32(baseImage, temp);
                if (FAILED(hr))
                    return hr;

                const Image *timg = temp.GetImage(0, 0, 0);
                if (!timg)
                    return E_POINTER;

                ScratchImage tMipChain;
                hr = (baseImage.height > 1 || !allow1D)
                    ? tMipChain.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, baseImage.width, baseImage.height, 1, levels)
                    : tMipChain.Initialize1D(DXGI_FORMAT_R32G32B32A32_FLOAT, baseImage.width, 1, levels);
                if (FAILED(hr))
                    return hr;

                hr = GenerateMipMapsUsingWIC(*timg, filter, levels, GUID_WICPixelFormat128bppRGBAFloat, tMipChain, 0);
                if (FAILED(hr))
                    return hr;

                temp.Release();

                return _ConvertFromR32G32B32A32(tMipChain.GetImages(), tMipChain.GetImageCount(), tMipChain.GetMetadata(), baseImage.format, mipChain);
            }
        }
        break;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }
    else
    {
        //--- Use custom filters to generate mipmaps ----------------------------------
        TexMetadata mdata = {};
        mdata.width = baseImage.width;
        if (baseImage.height > 1 || !allow1D)
        {
            mdata.height = baseImage.height;
            mdata.dimension = TEX_DIMENSION_TEXTURE2D;
        }
        else
        {
            mdata.height = 1;
            mdata.dimension = TEX_DIMENSION_TEXTURE1D;
        }
        mdata.depth = mdata.arraySize = 1;
        mdata.mipLevels = levels;
        mdata.format = baseImage.format;

        DWORD filter_select = (filter & TEX_FILTER_MASK);
        if (!filter_select)
        {
            // Default filter choice
            filter_select = (ispow2(baseImage.width) && ispow2(baseImage.height)) ? TEX_FILTER_BOX : TEX_FILTER_LINEAR;
        }

        switch (filter_select)
        {
        case TEX_FILTER_BOX:
            hr = Setup2DMips(&baseImage, 1, mdata, mipChain);
            if (FAILED(hr))
                return hr;

            hr = Generate2DMipsBoxFilter(levels, filter, mipChain, 0);
            if (FAILED(hr))
                mipChain.Release();
            return hr;

        case TEX_FILTER_POINT:
            hr = Setup2DMips(&baseImage, 1, mdata, mipChain);
            if (FAILED(hr))
                return hr;

            hr = Generate2DMipsPointFilter(levels, mipChain, 0);
            if (FAILED(hr))
                mipChain.Release();
            return hr;

        case TEX_FILTER_LINEAR:
            hr = Setup2DMips(&baseImage, 1, mdata, mipChain);
            if (FAILED(hr))
                return hr;

            hr = Generate2DMipsLinearFilter(levels, filter, mipChain, 0);
            if (FAILED(hr))
                mipChain.Release();
            return hr;

        case TEX_FILTER_CUBIC:
            hr = Setup2DMips(&baseImage, 1, mdata, mipChain);
            if (FAILED(hr))
                return hr;

            hr = Generate2DMipsCubicFilter(levels, filter, mipChain, 0);
            if (FAILED(hr))
                mipChain.Release();
            return hr;

        case TEX_FILTER_TRIANGLE:
            hr = Setup2DMips(&baseImage, 1, mdata, mipChain);
            if (FAILED(hr))
                return hr;

            hr = Generate2DMipsTriangleFilter(levels, filter, mipChain, 0);
            if (FAILED(hr))
                mipChain.Release();
            return hr;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }
}

_Use_decl_annotations_
HRESULT DirectX::GenerateMipMaps(
    const Image* srcImages,
    size_t nimages,
    const TexMetadata& metadata,
    DWORD filter,
    size_t levels,
    ScratchImage& mipChain)
{
    if (!srcImages || !nimages || !IsValid(metadata.format))
        return E_INVALIDARG;

    if (metadata.IsVolumemap()
        || IsCompressed(metadata.format) || IsTypeless(metadata.format) || IsPlanar(metadata.format) || IsPalettized(metadata.format))
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (!_CalculateMipLevels(metadata.width, metadata.height, levels))
        return E_INVALIDARG;

    if (levels <= 1)
        return E_INVALIDARG;

    std::vector<Image> baseImages;
    baseImages.reserve(metadata.arraySize);
    for (size_t item = 0; item < metadata.arraySize; ++item)
    {
        size_t index = metadata.ComputeIndex(0, item, 0);
        if (index >= nimages)
            return E_FAIL;

        const Image& src = srcImages[index];
        if (!src.pixels)
            return E_POINTER;

        if (src.format != metadata.format || src.width != metadata.width || src.height != metadata.height)
        {
            // All base images must be the same format, width, and height
            return E_FAIL;
        }

        baseImages.push_back(src);
    }

    assert(baseImages.size() == metadata.arraySize);

    HRESULT hr;

    static_assert(TEX_FILTER_POINT == 0x100000, "TEX_FILTER_ flag values don't match TEX_FILTER_MASK");

    if (!metadata.IsPMAlpha() && UseWICFiltering(metadata.format, filter))
    {
        //--- Use WIC filtering to generate mipmaps -----------------------------------
        switch (filter & TEX_FILTER_MASK)
        {
        case 0:
        case TEX_FILTER_POINT:
        case TEX_FILTER_FANT: // Equivalent to Box filter
        case TEX_FILTER_LINEAR:
        case TEX_FILTER_CUBIC:
        {
            static_assert(TEX_FILTER_FANT == TEX_FILTER_BOX, "TEX_FILTER_ flag alias mismatch");

            WICPixelFormatGUID pfGUID;
            if (_DXGIToWIC(metadata.format, pfGUID, true))
            {
                // Case 1: Base image format is supported by Windows Imaging Component
                TexMetadata mdata2 = metadata;
                mdata2.mipLevels = levels;
                hr = mipChain.Initialize(mdata2);
                if (FAILED(hr))
                    return hr;

                for (size_t item = 0; item < metadata.arraySize; ++item)
                {
                    hr = GenerateMipMapsUsingWIC(baseImages[item], filter, levels, pfGUID, mipChain, item);
                    if (FAILED(hr))
                    {
                        mipChain.Release();
                        return hr;
                    }
                }

                return S_OK;
            }
            else
            {
                // Case 2: Base image format is not supported by WIC, so we have to convert, generate, and convert back
                assert(metadata.format != DXGI_FORMAT_R32G32B32A32_FLOAT);

                TexMetadata mdata2 = metadata;
                mdata2.mipLevels = levels;
                mdata2.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                ScratchImage tMipChain;
                hr = tMipChain.Initialize(mdata2);
                if (FAILED(hr))
                    return hr;

                for (size_t item = 0; item < metadata.arraySize; ++item)
                {
                    ScratchImage temp;
                    hr = _ConvertToR32G32B32A32(baseImages[item], temp);
                    if (FAILED(hr))
                        return hr;

                    const Image *timg = temp.GetImage(0, 0, 0);
                    if (!timg)
                        return E_POINTER;

                    hr = GenerateMipMapsUsingWIC(*timg, filter, levels, GUID_WICPixelFormat128bppRGBAFloat, tMipChain, item);
                    if (FAILED(hr))
                        return hr;
                }

                return _ConvertFromR32G32B32A32(tMipChain.GetImages(), tMipChain.GetImageCount(), tMipChain.GetMetadata(), metadata.format, mipChain);
            }
        }
        break;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }
    else
    {
        //--- Use custom filters to generate mipmaps ----------------------------------
        TexMetadata mdata2 = metadata;
        mdata2.mipLevels = levels;

        DWORD filter_select = (filter & TEX_FILTER_MASK);
        if (!filter_select)
        {
            // Default filter choice
            filter_select = (ispow2(metadata.width) && ispow2(metadata.height)) ? TEX_FILTER_BOX : TEX_FILTER_LINEAR;
        }

        switch (filter_select)
        {
        case TEX_FILTER_BOX:
            hr = Setup2DMips(&baseImages[0], metadata.arraySize, mdata2, mipChain);
            if (FAILED(hr))
                return hr;

            for (size_t item = 0; item < metadata.arraySize; ++item)
            {
                hr = Generate2DMipsBoxFilter(levels, filter, mipChain, item);
                if (FAILED(hr))
                    mipChain.Release();
            }
            return hr;

        case TEX_FILTER_POINT:
            hr = Setup2DMips(&baseImages[0], metadata.arraySize, mdata2, mipChain);
            if (FAILED(hr))
                return hr;

            for (size_t item = 0; item < metadata.arraySize; ++item)
            {
                hr = Generate2DMipsPointFilter(levels, mipChain, item);
                if (FAILED(hr))
                    mipChain.Release();
            }
            return hr;

        case TEX_FILTER_LINEAR:
            hr = Setup2DMips(&baseImages[0], metadata.arraySize, mdata2, mipChain);
            if (FAILED(hr))
                return hr;

            for (size_t item = 0; item < metadata.arraySize; ++item)
            {
                hr = Generate2DMipsLinearFilter(levels, filter, mipChain, item);
                if (FAILED(hr))
                    mipChain.Release();
            }
            return hr;

        case TEX_FILTER_CUBIC:
            hr = Setup2DMips(&baseImages[0], metadata.arraySize, mdata2, mipChain);
            if (FAILED(hr))
                return hr;

            for (size_t item = 0; item < metadata.arraySize; ++item)
            {
                hr = Generate2DMipsCubicFilter(levels, filter, mipChain, item);
                if (FAILED(hr))
                    mipChain.Release();
            }
            return hr;

        case TEX_FILTER_TRIANGLE:
            hr = Setup2DMips(&baseImages[0], metadata.arraySize, mdata2, mipChain);
            if (FAILED(hr))
                return hr;

            for (size_t item = 0; item < metadata.arraySize; ++item)
            {
                hr = Generate2DMipsTriangleFilter(levels, filter, mipChain, item);
                if (FAILED(hr))
                    mipChain.Release();
            }
            return hr;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }
}


//-------------------------------------------------------------------------------------
// Generate mipmap chain for volume texture
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::GenerateMipMaps3D(
    const Image* baseImages,
    size_t depth,
    DWORD filter,
    size_t levels,
    ScratchImage& mipChain)
{
    if (!baseImages || !depth)
        return E_INVALIDARG;

    if (filter & TEX_FILTER_FORCE_WIC)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    DXGI_FORMAT format = baseImages[0].format;
    size_t width = baseImages[0].width;
    size_t height = baseImages[0].height;

    if (!_CalculateMipLevels3D(width, height, depth, levels))
        return E_INVALIDARG;

    if (levels <= 1)
        return E_INVALIDARG;

    for (size_t slice = 0; slice < depth; ++slice)
    {
        if (!baseImages[slice].pixels)
            return E_POINTER;

        if (baseImages[slice].format != format || baseImages[slice].width != width || baseImages[slice].height != height)
        {
            // All base images must be the same format, width, and height
            return E_FAIL;
        }
    }

    if (IsCompressed(format) || IsTypeless(format) || IsPlanar(format) || IsPalettized(format))
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    static_assert(TEX_FILTER_POINT == 0x100000, "TEX_FILTER_ flag values don't match TEX_FILTER_MASK");

    HRESULT hr;

    DWORD filter_select = (filter & TEX_FILTER_MASK);
    if (!filter_select)
    {
        // Default filter choice
        filter_select = (ispow2(width) && ispow2(height) && ispow2(depth)) ? TEX_FILTER_BOX : TEX_FILTER_TRIANGLE;
    }

    switch (filter_select)
    {
    case TEX_FILTER_BOX:
        hr = Setup3DMips(baseImages, depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsBoxFilter(depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_POINT:
        hr = Setup3DMips(baseImages, depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsPointFilter(depth, levels, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_LINEAR:
        hr = Setup3DMips(baseImages, depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsLinearFilter(depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_CUBIC:
        hr = Setup3DMips(baseImages, depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsCubicFilter(depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_TRIANGLE:
        hr = Setup3DMips(baseImages, depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsTriangleFilter(depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    default:
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
}

_Use_decl_annotations_
HRESULT DirectX::GenerateMipMaps3D(
    const Image* srcImages,
    size_t nimages,
    const TexMetadata& metadata,
    DWORD filter,
    size_t levels,
    ScratchImage& mipChain)
{
    if (!srcImages || !nimages || !IsValid(metadata.format))
        return E_INVALIDARG;

    if (filter & TEX_FILTER_FORCE_WIC)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (!metadata.IsVolumemap()
        || IsCompressed(metadata.format) || IsTypeless(metadata.format) || IsPlanar(metadata.format) || IsPalettized(metadata.format))
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (!_CalculateMipLevels3D(metadata.width, metadata.height, metadata.depth, levels))
        return E_INVALIDARG;

    if (levels <= 1)
        return E_INVALIDARG;

    std::vector<Image> baseImages;
    baseImages.reserve(metadata.depth);
    for (size_t slice = 0; slice < metadata.depth; ++slice)
    {
        size_t index = metadata.ComputeIndex(0, 0, slice);
        if (index >= nimages)
            return E_FAIL;

        const Image& src = srcImages[index];
        if (!src.pixels)
            return E_POINTER;

        if (src.format != metadata.format || src.width != metadata.width || src.height != metadata.height)
        {
            // All base images must be the same format, width, and height
            return E_FAIL;
        }

        baseImages.push_back(src);
    }

    assert(baseImages.size() == metadata.depth);

    HRESULT hr;

    static_assert(TEX_FILTER_POINT == 0x100000, "TEX_FILTER_ flag values don't match TEX_FILTER_MASK");

    DWORD filter_select = (filter & TEX_FILTER_MASK);
    if (!filter_select)
    {
        // Default filter choice
        filter_select = (ispow2(metadata.width) && ispow2(metadata.height) && ispow2(metadata.depth)) ? TEX_FILTER_BOX : TEX_FILTER_TRIANGLE;
    }

    switch (filter_select)
    {
    case TEX_FILTER_BOX:
        hr = Setup3DMips(&baseImages[0], metadata.depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsBoxFilter(metadata.depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_POINT:
        hr = Setup3DMips(&baseImages[0], metadata.depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsPointFilter(metadata.depth, levels, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_LINEAR:
        hr = Setup3DMips(&baseImages[0], metadata.depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsLinearFilter(metadata.depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_CUBIC:
        hr = Setup3DMips(&baseImages[0], metadata.depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsCubicFilter(metadata.depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    case TEX_FILTER_TRIANGLE:
        hr = Setup3DMips(&baseImages[0], metadata.depth, levels, mipChain);
        if (FAILED(hr))
            return hr;

        hr = Generate3DMipsTriangleFilter(metadata.depth, levels, filter, mipChain);
        if (FAILED(hr))
            mipChain.Release();
        return hr;

    default:
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
}
