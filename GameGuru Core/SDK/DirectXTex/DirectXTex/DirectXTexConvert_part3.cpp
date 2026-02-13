//-------------------------------------------------------------------------------------
// Convert image from planar to single plane (image)
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::ConvertToSinglePlane(const Image& srcImage, ScratchImage& image)
{
    if (!IsPlanar(srcImage.format))
        return E_INVALIDARG;

    if (!srcImage.pixels)
        return E_POINTER;

    DXGI_FORMAT format = _PlanarToSingle(srcImage.format);
    if (format == DXGI_FORMAT_UNKNOWN)
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

    hr = ConvertToSinglePlane_(srcImage, *rimage);
    if (FAILED(hr))
    {
        image.Release();
        return hr;
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
// Convert image from planar to single plane (complex)
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::ConvertToSinglePlane(
    const Image* srcImages,
    size_t nimages,
    const TexMetadata& metadata,
    ScratchImage& result)
{
    if (!srcImages || !nimages)
        return E_INVALIDARG;

    if (metadata.IsVolumemap())
    {
        // Direct3D does not support any planar formats for Texture3D
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    DXGI_FORMAT format = _PlanarToSingle(metadata.format);
    if (format == DXGI_FORMAT_UNKNOWN)
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

    for (size_t index = 0; index < nimages; ++index)
    {
        const Image& src = srcImages[index];
        if (src.format != metadata.format)
        {
            result.Release();
            return E_FAIL;
        }

        if ((src.width > UINT32_MAX) || (src.height > UINT32_MAX))
            return E_FAIL;

        const Image& dst = dest[index];
        assert(dst.format == format);

        if (src.width != dst.width || src.height != dst.height)
        {
            result.Release();
            return E_FAIL;
        }

        hr = ConvertToSinglePlane_(src, dst);
        if (FAILED(hr))
        {
            result.Release();
            return hr;
        }
    }

    return S_OK;
}
