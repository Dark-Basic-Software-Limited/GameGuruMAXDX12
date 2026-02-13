//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Parameters and defaults
    DWORD dwFilter = TEX_FILTER_DEFAULT;
    int pixelx = -1;
    int pixely = -1;
    DXGI_FORMAT diffFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DWORD diffFileType = WIC_CODEC_BMP;
    wchar_t szOutputFile[MAX_PATH] = { 0 };

    // Initialize COM (needed for WIC)
    HRESULT hr = hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to initialize COM (%08X)\n", hr);
        return 1;
    }

    // Process command line
    if (argc < 2)
    {
        PrintUsage();
        return 0;
    }

    DWORD dwCommand = LookupByName(argv[1], g_pCommands);
    switch (dwCommand)
    {
    case CMD_INFO:
    case CMD_ANALYZE:
    case CMD_COMPARE:
    case CMD_DIFF:
    case CMD_DUMPBC:
        break;

    default:
        wprintf(L"Must use one of: info, analyze, compare, diff, or dumpbc\n\n");
        return 1;
    }

    DWORD dwOptions = 0;
    std::list<SConversion> conversion;

    for (int iArg = 2; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if (('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            PWSTR pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            DWORD dwOption = LookupByName(pArg, g_pOptions);

            if (!dwOption || (dwOptions & (1 << dwOption)))
            {
                PrintUsage();
                return 1;
            }

            dwOptions |= 1 << dwOption;

            // Handle options with additional value parameter
            switch (dwOption)
            {
            case OPT_FILTER:
            case OPT_FORMAT:
            case OPT_OUTPUTFILE:
            case OPT_TARGET_PIXELX:
            case OPT_TARGET_PIXELY:
                if (!*pValue)
                {
                    if ((iArg + 1 >= argc))
                    {
                        PrintUsage();
                        return 1;
                    }

                    iArg++;
                    pValue = argv[iArg];
                }
                break;
            }

            switch (dwOption)
            {
            case OPT_FORMAT:
                if (dwCommand != CMD_DIFF)
                {
                    wprintf(L"-f only valid for use with diff command\n");
                    return 1;
                }
                else
                {
                    diffFormat = (DXGI_FORMAT)LookupByName(pValue, g_pFormats);
                    if (!diffFormat)
                    {
                        wprintf(L"Invalid value specified with -f (%ls)\n", pValue);
                        return 1;
                    }
                }
                break;

            case OPT_FILTER:
                dwFilter = LookupByName(pValue, g_pFilters);
                if (!dwFilter)
                {
                    wprintf(L"Invalid value specified with -if (%ls)\n", pValue);
                    return 1;
                }
                break;

            case OPT_OUTPUTFILE:
                if (dwCommand != CMD_DIFF)
                {
                    wprintf(L"-o only valid for use with diff command\n");
                    return 1;
                }
                else
                {
                    wcscpy_s(szOutputFile, MAX_PATH, pValue);

                    wchar_t ext[_MAX_EXT];
                    _wsplitpath_s(szOutputFile, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

                    diffFileType = LookupByName(ext, g_pExtFileTypes);
                }
                break;

            case OPT_TARGET_PIXELX:
                if (dwCommand != CMD_DUMPBC)
                {
                    wprintf(L"-targetx only valid with dumpbc command\n");
                    return 1;
                }
                else if (swscanf_s(pValue, L"%d", &pixelx) != 1)
                {
                    wprintf(L"Invalid value for pixel x location (%ls)\n", pValue);
                    return 1;
                }
                break;

            case OPT_TARGET_PIXELY:
                if (dwCommand != CMD_DUMPBC)
                {
                    wprintf(L"-targety only valid with dumpbc command\n");
                    return 1;
                }
                else if (swscanf_s(pValue, L"%d", &pixely) != 1)
                {
                    wprintf(L"Invalid value for pixel y location (%ls)\n", pValue);
                    return 1;
                }
                break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            size_t count = conversion.size();
            SearchForFiles(pArg, conversion, (dwOptions & (1 << OPT_RECURSIVE)) != 0);
            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\n", pArg);
                return 1;
            }
        }
        else
        {
            SConversion conv;
            wcscpy_s(conv.szSrc, MAX_PATH, pArg);

            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        PrintUsage();
        return 0;
    }

    if (~dwOptions & (1 << OPT_NOLOGO))
        PrintLogo();

    switch (dwCommand)
    {
    case CMD_COMPARE:
    case CMD_DIFF:
        // --- Compare/Diff ------------------------------------------------------------
        if (conversion.size() != 2)
        {
            wprintf(L"ERROR: compare/diff needs exactly two images\n");
            return 1;
        }
        else
        {
            auto pImage1 = conversion.cbegin();

            wprintf(L"1: %ls", pImage1->szSrc);
            fflush(stdout);

            TexMetadata info1;
            std::unique_ptr<ScratchImage> image1;
            hr = LoadImage(pImage1->szSrc, dwOptions, dwFilter, info1, image1);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                return 1;
            }

            auto pImage2 = conversion.cbegin();
            std::advance(pImage2, 1);

            wprintf(L"\n2: %ls", pImage2->szSrc);
            fflush(stdout);

            TexMetadata info2;
            std::unique_ptr<ScratchImage> image2;
            hr = LoadImage(pImage2->szSrc, dwOptions, dwFilter, info2, image2);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                return 1;
            }

            wprintf(L"\n");
            fflush(stdout);

            if (info1.height != info2.height
                || info1.width != info2.width)
            {
                wprintf(L"ERROR: Can only compare/diff images of the same width & height\n");
                return 1;
            }

            if (dwCommand == CMD_DIFF)
            {
                if (!*szOutputFile)
                {
                    wchar_t ext[_MAX_EXT];
                    wchar_t fname[_MAX_FNAME];
                    _wsplitpath_s(pImage1->szSrc, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
                    if (_wcsicmp(ext, L".bmp") == 0)
                    {
                        wprintf(L"ERROR: Need to specify output file via -o\n");
                        return 1;
                    }

                    _wmakepath_s(szOutputFile, nullptr, nullptr, fname, L".bmp");
                }

                if (image1->GetImageCount() > 1 || image2->GetImageCount() > 1)
                    wprintf(L"WARNING: ignoring all images but first one in each file\n");

                ScratchImage diffImage;
                hr = Difference(*image1->GetImage(0, 0, 0), *image2->GetImage(0, 0, 0), dwFilter, diffFormat, diffImage);
                if (FAILED(hr))
                {
                    wprintf(L"Failed diffing images (%08X)\n", hr);
                    return 1;
                }

                if (~dwOptions & (1 << OPT_OVERWRITE))
                {
                    if (GetFileAttributesW(szOutputFile) != INVALID_FILE_ATTRIBUTES)
                    {
                        wprintf(L"\nERROR: Output file already exists, use -y to overwrite\n");
                        return 1;
                    }
                }

                hr = SaveImage(diffImage.GetImage(0, 0, 0), szOutputFile, diffFileType);
                if (FAILED(hr))
                {
                    wprintf(L" FAILED (%x)\n", hr);
                    return 1;
                }

                wprintf(L"Difference %ls\n", szOutputFile);
            }
            else if ((info1.depth == 1
                && info1.arraySize == 1
                && info1.mipLevels == 1)
                || info1.depth != info2.depth
                || info1.arraySize != info2.arraySize
                || info1.mipLevels != info2.mipLevels
                || image1->GetImageCount() != image2->GetImageCount())
            {
                // Compare single image
                if (image1->GetImageCount() > 1 || image2->GetImageCount() > 1)
                    wprintf(L"WARNING: ignoring all images but first one in each file\n");

                float mse, mseV[4];
                hr = ComputeMSE(*image1->GetImage(0, 0, 0), *image2->GetImage(0, 0, 0), mse, mseV);
                if (FAILED(hr))
                {
                    wprintf(L"Failed comparing images (%08X)\n", hr);
                    return 1;
                }

                wprintf(L"Result: %f (%f %f %f %f) PSNR %f dB\n", mse, mseV[0], mseV[1], mseV[2], mseV[3],
                    10.f * log10f(3.f / (mseV[0] + mseV[1] + mseV[2])));
            }
            else
            {
                // Compare all images
                float min_mse = FLT_MAX;
                float min_mseV[4] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };

                float max_mse = -FLT_MAX;
                float max_mseV[4] = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };

                double sum_mse = 0;
                double sum_mseV[4] = { 0, 0, 0, 0 };

                size_t total_images = 0;

                if (info1.depth > 1)
                {
                    wprintf(L"Results by mip (%3Iu) and slice (%3Iu)\n\n", info1.mipLevels, info1.depth);

                    size_t depth = info1.depth;
                    for (size_t mip = 0; mip < info1.mipLevels; ++mip)
                    {
                        for (size_t slice = 0; slice < depth; ++slice)
                        {
                            const Image* img1 = image1->GetImage(mip, 0, slice);
                            const Image* img2 = image2->GetImage(mip, 0, slice);

                            if (!img1
                                || !img2
                                || img1->height != img2->height
                                || img1->width != img2->width)
                            {
                                wprintf(L"ERROR: Unexpected mismatch at slice %3Iu, mip %3Iu\n", slice, mip);
                                return 1;
                            }
                            else
                            {
                                float mse, mseV[4];
                                hr = ComputeMSE(*img1, *img2, mse, mseV);
                                if (FAILED(hr))
                                {
                                    wprintf(L"Failed comparing images at slice %3Iu, mip %3Iu (%08X)\n", slice, mip, hr);
                                    return 1;
                                }

                                min_mse = std::min(min_mse, mse);
                                max_mse = std::max(max_mse, mse);
                                sum_mse += mse;

                                for (size_t j = 0; j < 4; ++j)
                                {
                                    min_mseV[j] = std::min(min_mseV[j], mseV[j]);
                                    max_mseV[j] = std::max(max_mseV[j], mseV[j]);
                                    sum_mseV[j] += mseV[j];
                                }

                                ++total_images;

                                wprintf(L"[%3Iu,%3Iu]: %f (%f %f %f %f) PSNR %f dB\n", mip, slice, mse, mseV[0], mseV[1], mseV[2], mseV[3],
                                    10.f * log10f(3.f / (mseV[0] + mseV[1] + mseV[2])));
                            }
                        }

                        if (depth > 1)
                            depth >>= 1;
                    }
                }
                else
                {
                    wprintf(L"Results by item (%3Iu) and mip (%3Iu)\n\n", info1.arraySize, info1.mipLevels);

                    for (size_t item = 0; item < info1.arraySize; ++item)
                    {
                        for (size_t mip = 0; mip < info1.mipLevels; ++mip)
                        {
                            const Image* img1 = image1->GetImage(mip, item, 0);
                            const Image* img2 = image2->GetImage(mip, item, 0);

                            if (!img1
                                || !img2
                                || img1->height != img2->height
                                || img1->width != img2->width)
                            {
                                wprintf(L"ERROR: Unexpected mismatch at item %3Iu, mip %3Iu\n", item, mip);
                                return 1;
                            }
                            else
                            {
                                float mse, mseV[4];
                                hr = ComputeMSE(*img1, *img2, mse, mseV);
                                if (FAILED(hr))
                                {
                                    wprintf(L"Failed comparing images at item %3Iu, mip %3Iu (%08X)\n", item, mip, hr);
                                    return 1;
                                }

                                min_mse = std::min(min_mse, mse);
                                max_mse = std::max(max_mse, mse);
                                sum_mse += mse;

                                for (size_t j = 0; j < 4; ++j)
                                {
                                    min_mseV[j] = std::min(min_mseV[j], mseV[j]);
                                    max_mseV[j] = std::max(max_mseV[j], mseV[j]);
                                    sum_mseV[j] += mseV[j];
                                }

                                ++total_images;

                                wprintf(L"[%3Iu,%3Iu]: %f (%f %f %f %f) PSNR %f dB\n", item, mip, mse, mseV[0], mseV[1], mseV[2], mseV[3],
                                    10.f * log10f(3.f / (mseV[0] + mseV[1] + mseV[2])));
                            }
                        }
                    }
                }

                // Output multi-image stats
                if (total_images > 1)
                {
                    wprintf(L"\n    Minimum MSE: %f (%f %f %f %f) PSNR %f dB\n", min_mse, min_mseV[0], min_mseV[1], min_mseV[2], min_mseV[3],
                        10.f * log10f(3.f / (min_mseV[0] + min_mseV[1] + min_mseV[2])));
                    double total_mseV0 = sum_mseV[0] / double(total_images);
                    double total_mseV1 = sum_mseV[1] / double(total_images);
                    double total_mseV2 = max_mseV[2] / double(total_images);
                    wprintf(L"    Average MSE: %f (%f %f %f %f) PSNR %f dB\n", sum_mse / double(total_images),
                        total_mseV0,
                        total_mseV1,
                        total_mseV2,
                        sum_mseV[3] / double(total_images),
                        10.0 * log10(3.0 / (total_mseV0 + total_mseV1 + total_mseV2)));
                    wprintf(L"    Maximum MSE: %f (%f %f %f %f) PSNR %f dB\n", max_mse, max_mseV[0], max_mseV[1], max_mseV[2], max_mseV[3],
                        10.f * log10f(3.f / (max_mseV[0] + max_mseV[1] + max_mseV[2])));
                }
            }
        }
        break;
    
    default:
        for (auto pConv = conversion.cbegin(); pConv != conversion.cend(); ++pConv)
        {
            // Load source image
            if (pConv != conversion.begin())
                wprintf(L"\n");

            wprintf(L"%ls", pConv->szSrc);
            fflush(stdout);

            TexMetadata info;
            std::unique_ptr<ScratchImage> image;
            hr = LoadImage(pConv->szSrc, dwOptions, dwFilter, info, image);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                return 1;
            }

            wprintf(L"\n");
            fflush(stdout);

            if (dwCommand == CMD_INFO)
            {
                // --- Info ----------------------------------------------------------------
                wprintf(L"        width = %Iu\n", info.width);
                wprintf(L"       height = %Iu\n", info.height);
                wprintf(L"        depth = %Iu\n", info.depth);
                wprintf(L"    mipLevels = %Iu\n", info.mipLevels);
                wprintf(L"    arraySize = %Iu\n", info.arraySize);
                wprintf(L"       format = ");
                PrintFormat(info.format);
                wprintf(L"\n    dimension = ");
                switch (info.dimension)
                {
                case TEX_DIMENSION_TEXTURE1D:
                    wprintf((info.arraySize > 1) ? L"1DArray\n" : L"1D\n");
                    break;

                case TEX_DIMENSION_TEXTURE2D:
                    if (info.IsCubemap())
                    {
                        wprintf((info.arraySize > 6) ? L"CubeArray\n" : L"Cube\n");
                    }
                    else
                    {
                        wprintf((info.arraySize > 1) ? L"2DArray\n" : L"2D\n");
                    }
                    break;

                case TEX_DIMENSION_TEXTURE3D:
                    wprintf(L" 3D");
                    break;
                }

                wprintf(L"   alpha mode = ");
                switch (info.GetAlphaMode())
                {
                case TEX_ALPHA_MODE_OPAQUE:
                    wprintf(L"Opaque");
                    break;
                case TEX_ALPHA_MODE_PREMULTIPLIED:
                    wprintf(L"Premultiplied");
                    break;
                case TEX_ALPHA_MODE_STRAIGHT:
                    wprintf(L"Straight");
                    break;
                default:
                    wprintf(L"Unknown");
                    break;
                }

                wprintf(L"\n       images = %Iu\n", image->GetImageCount());

                auto sizeInKb = static_cast<uint32_t>(image->GetPixelsSize() / 1024);

                wprintf(L"   pixel size = %u (KB)\n\n", sizeInKb);
            }
            else if (dwCommand == CMD_DUMPBC)
            {
                // --- Dump BC -------------------------------------------------------------
                if (!IsCompressed(info.format))
                {
                    wprintf(L"ERROR: dumpbc only operates on BC format DDS files\n");
                    return 1;
                }

                if (pixelx >= (int)info.width
                    || pixely >= (int)info.height)
                {
                    wprintf(L"WARNING: Specified pixel location (%d x %d) is out of range for image (%Iu x %Iu)\n", pixelx, pixely, info.width, info.height);
                    continue;
                }

                wprintf(L"Compression: ");
                PrintFormat(info.format);
                wprintf(L"\n");

                if (info.depth > 1)
                {
                    wprintf(L"Results by mip (%3Iu) and slice (%3Iu)\n", info.mipLevels, info.depth);

                    size_t depth = info.depth;
                    for (size_t mip = 0; mip < info.mipLevels; ++mip)
                    {
                        for (size_t slice = 0; slice < depth; ++slice)
                        {
                            const Image* img = image->GetImage(mip, 0, slice);

                            if (!img)
                            {
                                wprintf(L"ERROR: Unexpected error at slice %3Iu, mip %3Iu\n", slice, mip);
                                return 1;
                            }
                            else
                            {
                                wprintf(L"\n[%3Iu, %3Iu]:\n", mip, slice);

                                hr = DumpBCImage(*img, pixelx, pixely);
                                if (FAILED(hr))
                                {
                                    wprintf(L"ERROR: Failed dumping image at slice %3Iu, mip %3Iu (%08X)\n", slice, mip, hr);
                                    return 1;
                                }
                            }
                        }

                        if (depth > 1)
                            depth >>= 1;

                        if (pixelx > 0)
                            pixelx >>= 1;

                        if (pixely > 0)
                            pixely >>= 1;
                    }
                }
                else
                {
                    wprintf(L"Results by item (%3Iu) and mip (%3Iu)\n", info.arraySize, info.mipLevels);

                    for (size_t item = 0; item < info.arraySize; ++item)
                    {
                        int tpixelx = pixelx;
                        int tpixely = pixely;

                        for (size_t mip = 0; mip < info.mipLevels; ++mip)
                        {
                            const Image* img = image->GetImage(mip, item, 0);

                            if (!img)
                            {
                                wprintf(L"ERROR: Unexpected error at item %3Iu, mip %3Iu\n", item, mip);
                                return 1;
                            }
                            else
                            {
                                if (image->GetImageCount() > 1)
                                {
                                    wprintf(L"\n[%3Iu, %3Iu]:\n", item, mip);
                                }
                                hr = DumpBCImage(*img, tpixelx, tpixely);
                                if (FAILED(hr))
                                {
                                    wprintf(L"ERROR: Failed dumping image at item %3Iu, mip %3Iu (%08X)\n", item, mip, hr);
                                    return 1;
                                }
                            }

                            if (tpixelx > 0)
                                tpixelx >>= 1;

                            if (tpixely > 0)
                                tpixely >>= 1;
                        }
                    }
                }
            }
            else
            {
                // --- Analyze -------------------------------------------------------------
                if (IsPlanar(info.format))
                {
                    auto img = image->GetImage(0, 0, 0);
                    assert(img);
                    size_t nimg = image->GetImageCount();

                    std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
                    if (!timage)
                    {
                        wprintf(L"\nERROR: Memory allocation failed\n");
                        return 1;
                    }

                    hr = ConvertToSinglePlane(img, nimg, info, *timage);
                    if (FAILED(hr))
                    {
                        wprintf(L" FAILED [converttosingleplane] (%x)\n", hr);
                        continue;
                    }

                    auto& tinfo = timage->GetMetadata();

                    info.format = tinfo.format;

                    assert(info.width == tinfo.width);
                    assert(info.height == tinfo.height);
                    assert(info.depth == tinfo.depth);
                    assert(info.arraySize == tinfo.arraySize);
                    assert(info.mipLevels == tinfo.mipLevels);
                    assert(info.miscFlags == tinfo.miscFlags);
                    assert(info.dimension == tinfo.dimension);

                    image.swap(timage);
                }

                if (info.depth > 1)
                {
                    wprintf(L"Results by mip (%3Iu) and slice (%3Iu)\n\n", info.mipLevels, info.depth);

                    size_t depth = info.depth;
                    for (size_t mip = 0; mip < info.mipLevels; ++mip)
                    {
                        for (size_t slice = 0; slice < depth; ++slice)
                        {
                            const Image* img = image->GetImage(mip, 0, slice);

                            if (!img)
                            {
                                wprintf(L"ERROR: Unexpected error at slice %3Iu, mip %3Iu\n", slice, mip);
                                return 1;
                            }
                            else
                            {
                                AnalyzeData data;
                                hr = Analyze(*img, data);
                                if (FAILED(hr))
                                {
                                    wprintf(L"ERROR: Failed analyzing image at slice %3Iu, mip %3Iu (%08X)\n", slice, mip, hr);
                                    return 1;
                                }

                                wprintf(L"Result slice %3Iu, mip %3Iu:\n", slice, mip);
                                data.Print();
                            }

                            if (IsCompressed(info.format))
                            {
                                AnalyzeBCData data;
                                hr = AnalyzeBC(*img, data);
                                if (FAILED(hr))
                                {
                                    wprintf(L"ERROR: Failed analyzing BC image at slice %3Iu, mip %3Iu (%08X)\n", slice, mip, hr);
                                    return 1;
                                }

                                data.Print(img->format);
                            }
                            wprintf(L"\n");
                        }

                        if (depth > 1)
                            depth >>= 1;
                    }
                }
                else
                {
                    wprintf(L"Results by item (%3Iu) and mip (%3Iu)\n\n", info.arraySize, info.mipLevels);

                    for (size_t item = 0; item < info.arraySize; ++item)
                    {
                        for (size_t mip = 0; mip < info.mipLevels; ++mip)
                        {
                            const Image* img = image->GetImage(mip, item, 0);

                            if (!img)
                            {
                                wprintf(L"ERROR: Unexpected error at item %3Iu, mip %3Iu\n", item, mip);
                                return 1;
                            }
                            else
                            {
                                AnalyzeData data;
                                hr = Analyze(*img, data);
                                if (FAILED(hr))
                                {
                                    wprintf(L"ERROR: Failed analyzing image at item %3Iu, mip %3Iu (%08X)\n", item, mip, hr);
                                    return 1;
                                }

                                if (image->GetImageCount() > 1)
                                {
                                    wprintf(L"Result item %3Iu, mip %3Iu:\n", item, mip);
                                }
                                data.Print();
                            }

                            if (IsCompressed(info.format))
                            {
                                AnalyzeBCData data;
                                hr = AnalyzeBC(*img, data);
                                if (FAILED(hr))
                                {
                                    wprintf(L"ERROR: Failed analyzing BC image at item %3Iu, mip %3Iu (%08X)\n", item, mip, hr);
                                    return 1;
                                }

                                data.Print(img->format);
                            }
                            wprintf(L"\n");
                        }
                    }
                }
            }
        }
        break;
    }

    return 0;
}
