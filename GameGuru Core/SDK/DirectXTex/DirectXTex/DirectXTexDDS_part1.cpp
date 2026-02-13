//-------------------------------------------------------------------------------------
// Save a DDS file to disk
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::SaveToDDSFile(
    const Image* images,
    size_t nimages,
    const TexMetadata& metadata,
    DWORD flags,
    const wchar_t* szFile)
{
    if (!szFile)
        return E_INVALIDARG;

    // Create DDS Header
    const size_t MAX_HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
    uint8_t header[MAX_HEADER_SIZE];
    size_t required;
    HRESULT hr = _EncodeDDSHeader(metadata, flags, header, MAX_HEADER_SIZE, required);
    if (FAILED(hr))
        return hr;

    // Create file and write header
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile(safe_handle(CreateFile2(szFile, GENERIC_WRITE | DELETE, 0, CREATE_ALWAYS, nullptr)));
#else
    ScopedHandle hFile(safe_handle(CreateFileW(szFile, GENERIC_WRITE | DELETE, 0, nullptr, CREATE_ALWAYS, 0, nullptr)));
#endif
    if (!hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    auto_delete_file delonfail(hFile.get());

    DWORD bytesWritten;
    if (!WriteFile(hFile.get(), header, static_cast<DWORD>(required), &bytesWritten, nullptr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (bytesWritten != required)
    {
        return E_FAIL;
    }

    // Write images
    switch (metadata.dimension)
    {
    case DDS_DIMENSION_TEXTURE1D:
    case DDS_DIMENSION_TEXTURE2D:
    {
        size_t index = 0;
        for (size_t item = 0; item < metadata.arraySize; ++item)
        {
            for (size_t level = 0; level < metadata.mipLevels; ++level, ++index)
            {
                if (index >= nimages)
                    return E_FAIL;

                if (!images[index].pixels)
                    return E_POINTER;

                assert(images[index].rowPitch > 0);
                assert(images[index].slicePitch > 0);

                size_t ddsRowPitch, ddsSlicePitch;
                ComputePitch(metadata.format, images[index].width, images[index].height, ddsRowPitch, ddsSlicePitch, CP_FLAGS_NONE);

                if (images[index].slicePitch == ddsSlicePitch)
                {
                    if (!WriteFile(hFile.get(), images[index].pixels, static_cast<DWORD>(ddsSlicePitch), &bytesWritten, nullptr))
                    {
                        return HRESULT_FROM_WIN32(GetLastError());
                    }

                    if (bytesWritten != ddsSlicePitch)
                    {
                        return E_FAIL;
                    }
                }
                else
                {
                    size_t rowPitch = images[index].rowPitch;
                    if (rowPitch < ddsRowPitch)
                    {
                        // DDS uses 1-byte alignment, so if this is happening then the input pitch isn't actually a full line of data
                        return E_FAIL;
                    }

                    const uint8_t * __restrict sPtr = images[index].pixels;

                    size_t lines = ComputeScanlines(metadata.format, images[index].height);
                    for (size_t j = 0; j < lines; ++j)
                    {
                        if (!WriteFile(hFile.get(), sPtr, static_cast<DWORD>(ddsRowPitch), &bytesWritten, nullptr))
                        {
                            return HRESULT_FROM_WIN32(GetLastError());
                        }

                        if (bytesWritten != ddsRowPitch)
                        {
                            return E_FAIL;
                        }

                        sPtr += rowPitch;
                    }
                }
            }
        }
    }
    break;

    case DDS_DIMENSION_TEXTURE3D:
    {
        if (metadata.arraySize != 1)
            return E_FAIL;

        size_t d = metadata.depth;

        size_t index = 0;
        for (size_t level = 0; level < metadata.mipLevels; ++level)
        {
            for (size_t slice = 0; slice < d; ++slice, ++index)
            {
                if (index >= nimages)
                    return E_FAIL;

                if (!images[index].pixels)
                    return E_POINTER;

                assert(images[index].rowPitch > 0);
                assert(images[index].slicePitch > 0);

                size_t ddsRowPitch, ddsSlicePitch;
                ComputePitch(metadata.format, images[index].width, images[index].height, ddsRowPitch, ddsSlicePitch, CP_FLAGS_NONE);

                if (images[index].slicePitch == ddsSlicePitch)
                {
                    if (!WriteFile(hFile.get(), images[index].pixels, static_cast<DWORD>(ddsSlicePitch), &bytesWritten, nullptr))
                    {
                        return HRESULT_FROM_WIN32(GetLastError());
                    }

                    if (bytesWritten != ddsSlicePitch)
                    {
                        return E_FAIL;
                    }
                }
                else
                {
                    size_t rowPitch = images[index].rowPitch;
                    if (rowPitch < ddsRowPitch)
                    {
                        // DDS uses 1-byte alignment, so if this is happening then the input pitch isn't actually a full line of data
                        return E_FAIL;
                    }

                    const uint8_t * __restrict sPtr = images[index].pixels;

                    size_t lines = ComputeScanlines(metadata.format, images[index].height);
                    for (size_t j = 0; j < lines; ++j)
                    {
                        if (!WriteFile(hFile.get(), sPtr, static_cast<DWORD>(ddsRowPitch), &bytesWritten, nullptr))
                        {
                            return HRESULT_FROM_WIN32(GetLastError());
                        }

                        if (bytesWritten != ddsRowPitch)
                        {
                            return E_FAIL;
                        }

                        sPtr += rowPitch;
                    }
                }
            }

            if (d > 1)
                d >>= 1;
        }
    }
    break;

    default:
        return E_FAIL;
    }

    delonfail.clear();

    return S_OK;
}
