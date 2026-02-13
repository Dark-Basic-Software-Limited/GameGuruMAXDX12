//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Parameters and defaults
    size_t width = 0;
    size_t height = 0;
    size_t mipLevels = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    DWORD dwFilter = TEX_FILTER_DEFAULT;
    DWORD dwSRGB = 0;
    DWORD dwConvert = 0;
    DWORD dwCompress = TEX_COMPRESS_DEFAULT;
    DWORD dwFilterOpts = 0;
    DWORD FileType = CODEC_DDS;
    DWORD maxSize = 16384;
    int adapter = -1;
    float alphaWeight = 1.f;
    DWORD dwNormalMap = 0;
    float nmapAmplitude = 1.f;
    float wicQuality = -1.f;
    DWORD colorKey = 0;

    wchar_t szPrefix[MAX_PATH];
    wchar_t szSuffix[MAX_PATH];
    wchar_t szOutputDir[MAX_PATH];

    szPrefix[0] = 0;
    szSuffix[0] = 0;
    szOutputDir[0] = 0;

    // Initialize COM (needed for WIC)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to initialize COM (%08X)\n", hr);
        return 1;
    }

    // Process command line
    DWORD64 dwOptions = 0;
    std::list<SConversion> conversion;

    for (int iArg = 1; iArg < argc; iArg++)
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

            if (!dwOption || (dwOptions & (DWORD64(1) << dwOption)))
            {
                PrintUsage();
                return 1;
            }

            dwOptions |= (DWORD64(1) << dwOption);

            // Handle options with additional value parameter
            switch (dwOption)
            {
            case OPT_WIDTH:
            case OPT_HEIGHT:
            case OPT_MIPLEVELS:
            case OPT_FORMAT:
            case OPT_FILTER:
            case OPT_PREFIX:
            case OPT_SUFFIX:
            case OPT_OUTPUTDIR:
            case OPT_FILETYPE:
            case OPT_GPU:
            case OPT_FEATURE_LEVEL:
            case OPT_ALPHA_WEIGHT:
            case OPT_NORMAL_MAP:
            case OPT_NORMAL_MAP_AMPLITUDE:
            case OPT_WIC_QUALITY:
            case OPT_COLORKEY:
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
            case OPT_WIDTH:
                if (swscanf_s(pValue, L"%Iu", &width) != 1)
                {
                    wprintf(L"Invalid value specified with -w (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_HEIGHT:
                if (swscanf_s(pValue, L"%Iu", &height) != 1)
                {
                    wprintf(L"Invalid value specified with -h (%ls)\n", pValue);
                    printf("\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_MIPLEVELS:
                if (swscanf_s(pValue, L"%Iu", &mipLevels) != 1)
                {
                    wprintf(L"Invalid value specified with -m (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_FORMAT:
                format = (DXGI_FORMAT)LookupByName(pValue, g_pFormats);
                if (!format)
                {
                    wprintf(L"Invalid value specified with -f (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_FILTER:
                dwFilter = LookupByName(pValue, g_pFilters);
                if (!dwFilter)
                {
                    wprintf(L"Invalid value specified with -if (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_SRGBI:
                dwSRGB |= TEX_FILTER_SRGB_IN;
                break;

            case OPT_SRGBO:
                dwSRGB |= TEX_FILTER_SRGB_OUT;
                break;

            case OPT_SRGB:
                dwSRGB |= TEX_FILTER_SRGB;
                break;

            case OPT_SEPALPHA:
                dwFilterOpts |= TEX_FILTER_SEPARATE_ALPHA;
                break;

            case OPT_PREFIX:
                wcscpy_s(szPrefix, MAX_PATH, pValue);
                break;

            case OPT_SUFFIX:
                wcscpy_s(szSuffix, MAX_PATH, pValue);
                break;

            case OPT_OUTPUTDIR:
                wcscpy_s(szOutputDir, MAX_PATH, pValue);
                break;

            case OPT_FILETYPE:
                FileType = LookupByName(pValue, g_pSaveFileTypes);
                if (!FileType)
                {
                    wprintf(L"Invalid value specified with -ft (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_PREMUL_ALPHA:
                if (dwOptions & (DWORD64(1) << OPT_DEMUL_ALPHA))
                {
                    wprintf(L"Can't use -pmalpha and -alpha at same time\n\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_DEMUL_ALPHA:
                if (dwOptions & (DWORD64(1) << OPT_PREMUL_ALPHA))
                {
                    wprintf(L"Can't use -pmalpha and -alpha at same time\n\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_TA_WRAP:
                if (dwFilterOpts & TEX_FILTER_MIRROR)
                {
                    wprintf(L"Can't use -wrap and -mirror at same time\n\n");
                    PrintUsage();
                    return 1;
                }
                dwFilterOpts |= TEX_FILTER_WRAP;
                break;

            case OPT_TA_MIRROR:
                if (dwFilterOpts & TEX_FILTER_WRAP)
                {
                    wprintf(L"Can't use -wrap and -mirror at same time\n\n");
                    PrintUsage();
                    return 1;
                }
                dwFilterOpts |= TEX_FILTER_MIRROR;
                break;

            case OPT_NORMAL_MAP:
            {
                dwNormalMap = 0;

                if (wcschr(pValue, L'l'))
                {
                    dwNormalMap |= CNMAP_CHANNEL_LUMINANCE;
                }
                else if (wcschr(pValue, L'r'))
                {
                    dwNormalMap |= CNMAP_CHANNEL_RED;
                }
                else if (wcschr(pValue, L'g'))
                {
                    dwNormalMap |= CNMAP_CHANNEL_GREEN;
                }
                else if (wcschr(pValue, L'b'))
                {
                    dwNormalMap |= CNMAP_CHANNEL_BLUE;
                }
                else if (wcschr(pValue, L'a'))
                {
                    dwNormalMap |= CNMAP_CHANNEL_ALPHA;
                }
                else
                {
                    wprintf(L"Invalid value specified for -nmap (%ls), missing l, r, g, b, or a\n\n", pValue);
                    PrintUsage();
                    return 1;
                }

                if (wcschr(pValue, L'm'))
                {
                    dwNormalMap |= CNMAP_MIRROR;
                }
                else
                {
                    if (wcschr(pValue, L'u'))
                    {
                        dwNormalMap |= CNMAP_MIRROR_U;
                    }
                    if (wcschr(pValue, L'v'))
                    {
                        dwNormalMap |= CNMAP_MIRROR_V;
                    }
                }

                if (wcschr(pValue, L'i'))
                {
                    dwNormalMap |= CNMAP_INVERT_SIGN;
                }

                if (wcschr(pValue, L'o'))
                {
                    dwNormalMap |= CNMAP_COMPUTE_OCCLUSION;
                }
            }
            break;

            case OPT_NORMAL_MAP_AMPLITUDE:
                if (!dwNormalMap)
                {
                    wprintf(L"-nmapamp requires -nmap\n\n");
                    PrintUsage();
                    return 1;
                }
                else if (swscanf_s(pValue, L"%f", &nmapAmplitude) != 1)
                {
                    wprintf(L"Invalid value specified with -nmapamp (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                else if (nmapAmplitude < 0.f)
                {
                    wprintf(L"Normal map amplitude must be positive (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_GPU:
                if (swscanf_s(pValue, L"%d", &adapter) != 1)
                {
                    wprintf(L"Invalid value specified with -gpu (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                else if (adapter < 0)
                {
                    wprintf(L"Adapter index (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_FEATURE_LEVEL:
                maxSize = LookupByName(pValue, g_pFeatureLevels);
                if (!maxSize)
                {
                    wprintf(L"Invalid value specified with -fl (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_ALPHA_WEIGHT:
                if (swscanf_s(pValue, L"%f", &alphaWeight) != 1)
                {
                    wprintf(L"Invalid value specified with -aw (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                else if (alphaWeight < 0.f)
                {
                    wprintf(L"-aw (%ls) parameter must be positive\n", pValue);
                    wprintf(L"\n");
                    return 1;
                }
                break;

            case OPT_COMPRESS_UNIFORM:
                dwCompress |= TEX_COMPRESS_UNIFORM;
                break;

            case OPT_COMPRESS_MAX:
                if (dwCompress & TEX_COMPRESS_BC7_QUICK)
                {
                    wprintf(L"Can't use -bcmax and -bcquick at same time\n\n");
                    PrintUsage();
                    return 1;
                }
                dwCompress |= TEX_COMPRESS_BC7_USE_3SUBSETS;
                break;

            case OPT_COMPRESS_QUICK:
                if (dwCompress & TEX_COMPRESS_BC7_USE_3SUBSETS)
                {
                    wprintf(L"Can't use -bcmax and -bcquick at same time\n\n");
                    PrintUsage();
                    return 1;
                }
                dwCompress |= TEX_COMPRESS_BC7_QUICK;
                break;

            case OPT_COMPRESS_DITHER:
                dwCompress |= TEX_COMPRESS_DITHER;
                break;

            case OPT_WIC_QUALITY:
                if (swscanf_s(pValue, L"%f", &wicQuality) != 1
                    || (wicQuality < 0.f)
                    || (wicQuality > 1.f))
                {
                    wprintf(L"Invalid value specified with -wicq (%ls)\n", pValue);
                    printf("\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_COLORKEY:
                if (swscanf_s(pValue, L"%x", &colorKey) != 1)
                {
                    printf("Invalid value specified with -c (%ls)\n", pValue);
                    printf("\n");
                    PrintUsage();
                    return 1;
                }
                colorKey &= 0xFFFFFF;
                break;

            case OPT_X2_BIAS:
                dwConvert |= TEX_FILTER_FLOAT_X2BIAS;
                break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            size_t count = conversion.size();
            SearchForFiles(pArg, conversion, (dwOptions & (DWORD64(1) << OPT_RECURSIVE)) != 0);
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

            conv.szDest[0] = 0;

            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        PrintUsage();
        return 0;
    }

    if (~dwOptions & (DWORD64(1) << OPT_NOLOGO))
        PrintLogo();

    // Work out out filename prefix and suffix
    if (szOutputDir[0] && (L'\\' != szOutputDir[wcslen(szOutputDir) - 1]))
        wcscat_s(szOutputDir, MAX_PATH, L"\\");

    if (szPrefix[0])
        wcscat_s(szOutputDir, MAX_PATH, szPrefix);

    wcscpy_s(szPrefix, MAX_PATH, szOutputDir);

    const wchar_t* fileTypeName = LookupByValue(FileType, g_pSaveFileTypes);

    if (fileTypeName)
    {
        wcscat_s(szSuffix, MAX_PATH, L".");
        wcscat_s(szSuffix, MAX_PATH, fileTypeName);
    }
    else
    {
        wcscat_s(szSuffix, MAX_PATH, L".unknown");
    }

    if (FileType != CODEC_DDS)
    {
        mipLevels = 1;
    }

    LARGE_INTEGER qpcFreq;
    if (!QueryPerformanceFrequency(&qpcFreq))
    {
        qpcFreq.QuadPart = 0;
    }


    LARGE_INTEGER qpcStart;
    if (!QueryPerformanceCounter(&qpcStart))
    {
        qpcStart.QuadPart = 0;
    }

    // Convert images
    bool nonpow2warn = false;
    bool non4bc = false;
    ComPtr<ID3D11Device> pDevice;

    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv)
    {
        if (pConv != conversion.begin())
            wprintf(L"\n");

        // Load source image
        wprintf(L"reading %ls", pConv->szSrc);
        fflush(stdout);

        wchar_t ext[_MAX_EXT];
        wchar_t fname[_MAX_FNAME];
        _wsplitpath_s(pConv->szSrc, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

        TexMetadata info;
        std::unique_ptr<ScratchImage> image(new (std::nothrow) ScratchImage);

        if (!image)
        {
            wprintf(L"\nERROR: Memory allocation failed\n");
            return 1;
        }

        if (_wcsicmp(ext, L".dds") == 0)
        {
            DWORD ddsFlags = DDS_FLAGS_NONE;
            if (dwOptions & (DWORD64(1) << OPT_DDS_DWORD_ALIGN))
                ddsFlags |= DDS_FLAGS_LEGACY_DWORD;
            if (dwOptions & (DWORD64(1) << OPT_EXPAND_LUMINANCE))
                ddsFlags |= DDS_FLAGS_EXPAND_LUMINANCE;
            if (dwOptions & (DWORD64(1) << OPT_DDS_BAD_DXTN_TAILS))
                ddsFlags |= DDS_FLAGS_BAD_DXTN_TAILS;

            hr = LoadFromDDSFile(pConv->szSrc, ddsFlags, &info, *image);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                continue;
            }

            if (IsTypeless(info.format))
            {
                if (dwOptions & (DWORD64(1) << OPT_TYPELESS_UNORM))
                {
                    info.format = MakeTypelessUNORM(info.format);
                }
                else if (dwOptions & (DWORD64(1) << OPT_TYPELESS_FLOAT))
                {
                    info.format = MakeTypelessFLOAT(info.format);
                }

                if (IsTypeless(info.format))
                {
                    wprintf(L" FAILED due to Typeless format %d\n", info.format);
                    continue;
                }

                image->OverrideFormat(info.format);
            }
        }
        else if (_wcsicmp(ext, L".tga") == 0)
        {
            hr = LoadFromTGAFile(pConv->szSrc, &info, *image);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                continue;
            }
        }
        else if (_wcsicmp(ext, L".hdr") == 0)
        {
            hr = LoadFromHDRFile(pConv->szSrc, &info, *image);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                continue;
            }
        }
#ifdef USE_OPENEXR
        else if (_wcsicmp(ext, L".exr") == 0)
        {
            hr = LoadFromEXRFile(pConv->szSrc, &info, *image);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                continue;
            }
        }
#endif
        else
        {
            // WIC shares the same filter values for mode and dither
            static_assert(WIC_FLAGS_DITHER == TEX_FILTER_DITHER, "WIC_FLAGS_* & TEX_FILTER_* should match");
            static_assert(WIC_FLAGS_DITHER_DIFFUSION == TEX_FILTER_DITHER_DIFFUSION, "WIC_FLAGS_* & TEX_FILTER_* should match");
            static_assert(WIC_FLAGS_FILTER_POINT == TEX_FILTER_POINT, "WIC_FLAGS_* & TEX_FILTER_* should match");
            static_assert(WIC_FLAGS_FILTER_LINEAR == TEX_FILTER_LINEAR, "WIC_FLAGS_* & TEX_FILTER_* should match");
            static_assert(WIC_FLAGS_FILTER_CUBIC == TEX_FILTER_CUBIC, "WIC_FLAGS_* & TEX_FILTER_* should match");
            static_assert(WIC_FLAGS_FILTER_FANT == TEX_FILTER_FANT, "WIC_FLAGS_* & TEX_FILTER_* should match");

            DWORD wicFlags = dwFilter;
            if (FileType == CODEC_DDS)
                wicFlags |= WIC_FLAGS_ALL_FRAMES;

            hr = LoadFromWICFile(pConv->szSrc, wicFlags, &info, *image);
            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                continue;
            }
        }

        PrintInfo(info);

        size_t tMips = (!mipLevels && info.mipLevels > 1) ? info.mipLevels : mipLevels;

        bool sizewarn = false;

        size_t twidth = (!width) ? info.width : width;
        if (twidth > maxSize)
        {
            if (!width)
                twidth = maxSize;
            else
                sizewarn = true;
        }

        size_t theight = (!height) ? info.height : height;
        if (theight > maxSize)
        {
            if (!height)
                theight = maxSize;
            else
                sizewarn = true;
        }

        if (sizewarn)
        {
            wprintf(L"\nWARNING: Target size exceeds maximum size for feature level (%u)\n", maxSize);
        }

        if (dwOptions & (DWORD64(1) << OPT_FIT_POWEROF2))
        {
            FitPowerOf2(info.width, info.height, twidth, theight, maxSize);
        }

        // Convert texture
        wprintf(L" as");
        fflush(stdout);

        // --- Planar ------------------------------------------------------------------
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

        DXGI_FORMAT tformat = (format == DXGI_FORMAT_UNKNOWN) ? info.format : format;

        // --- Decompress --------------------------------------------------------------
        std::unique_ptr<ScratchImage> cimage;
        if (IsCompressed(info.format))
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

            hr = Decompress(img, nimg, info, DXGI_FORMAT_UNKNOWN /* picks good default */, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [decompress] (%x)\n", hr);
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

            if (FileType == CODEC_DDS)
            {
                // Keep the original compressed image in case we can reuse it
                cimage.reset(image.release());
                image.reset(timage.release());
            }
            else
            {
                image.swap(timage);
            }
        }

        // --- Undo Premultiplied Alpha (if requested) ---------------------------------
        if ((dwOptions & (DWORD64(1) << OPT_DEMUL_ALPHA))
            && HasAlpha(info.format)
            && info.format != DXGI_FORMAT_A8_UNORM)
        {
            if (info.GetAlphaMode() == TEX_ALPHA_MODE_STRAIGHT)
            {
                printf("\nWARNING: Image is already using straight alpha\n");
            }
            else if (!info.IsPMAlpha())
            {
                printf("\nWARNING: Image is not using premultipled alpha\n");
            }
            else
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

                hr = PremultiplyAlpha(img, nimg, info, TEX_PMALPHA_REVERSE | dwSRGB, *timage);
                if (FAILED(hr))
                {
                    wprintf(L" FAILED [demultiply alpha] (%x)\n", hr);
                    continue;
                }

                auto& tinfo = timage->GetMetadata();
                info.miscFlags2 = tinfo.miscFlags2;

                assert(info.width == tinfo.width);
                assert(info.height == tinfo.height);
                assert(info.depth == tinfo.depth);
                assert(info.arraySize == tinfo.arraySize);
                assert(info.mipLevels == tinfo.mipLevels);
                assert(info.miscFlags == tinfo.miscFlags);
                assert(info.dimension == tinfo.dimension);

                image.swap(timage);
                cimage.reset();
            }
        }

        // --- Flip/Rotate -------------------------------------------------------------
        if (dwOptions & ((DWORD64(1) << OPT_HFLIP) | (DWORD64(1) << OPT_VFLIP)))
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            DWORD dwFlags = 0;

            if (dwOptions & (DWORD64(1) << OPT_HFLIP))
                dwFlags |= TEX_FR_FLIP_HORIZONTAL;

            if (dwOptions & (DWORD64(1) << OPT_VFLIP))
                dwFlags |= TEX_FR_FLIP_VERTICAL;

            assert(dwFlags != 0);

            hr = FlipRotate(image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFlags, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [fliprotate] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert(tinfo.width == twidth && tinfo.height == theight);

            info.width = tinfo.width;
            info.height = tinfo.height;

            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.mipLevels == tinfo.mipLevels);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.format == tinfo.format);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }

        // --- Resize ------------------------------------------------------------------
        if (info.width != twidth || info.height != theight)
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            hr = Resize(image->GetImages(), image->GetImageCount(), image->GetMetadata(), twidth, theight, dwFilter | dwFilterOpts, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [resize] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert(tinfo.width == twidth && tinfo.height == theight && tinfo.mipLevels == 1);
            info.width = tinfo.width;
            info.height = tinfo.height;
            info.mipLevels = 1;

            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.format == tinfo.format);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }

        // --- Tonemap (if requested) --------------------------------------------------
        if (dwOptions & DWORD64(1) << OPT_TONEMAP)
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            // Compute max luminosity across all images
            XMVECTOR maxLum = XMVectorZero();
            hr = EvaluateImage(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
                [&](const XMVECTOR* pixels, size_t width, size_t y)
            {
                UNREFERENCED_PARAMETER(y);

                for (size_t j = 0; j < width; ++j)
                {
                    static const XMVECTORF32 s_luminance = { 0.3f, 0.59f, 0.11f, 0.f };

                    XMVECTOR v = *pixels++;

                    v = XMVector3Dot(v, s_luminance);

                    maxLum = XMVectorMax(v, maxLum);
                }
            });
            if (FAILED(hr))
            {
                wprintf(L" FAILED [tonemap maxlum] (%x)\n", hr);
                return 1;
            }

            // Reinhard et al, "Photographic Tone Reproduction for Digital Images" 
            // http://www.cs.utah.edu/~reinhard/cdrom/
            maxLum = XMVectorMultiply(maxLum, maxLum);

            hr = TransformImage(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
                [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
            {
                UNREFERENCED_PARAMETER(y);

                for (size_t j = 0; j < width; ++j)
                {
                    XMVECTOR value = inPixels[j];

                    XMVECTOR scale = XMVectorDivide(
                        XMVectorAdd(g_XMOne, XMVectorDivide(value, maxLum)),
                        XMVectorAdd(g_XMOne, value));
                    XMVECTOR nvalue = XMVectorMultiply(value, scale);

                    value = XMVectorSelect(value, nvalue, g_XMSelect1110);

                    outPixels[j] = value;
                }
            }, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [tonemap apply] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();
            tinfo;

            assert(info.width == tinfo.width);
            assert(info.height == tinfo.height);
            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.mipLevels == tinfo.mipLevels);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.format == tinfo.format);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }

        // --- Convert -----------------------------------------------------------------
        if (dwOptions & (DWORD64(1) << OPT_NORMAL_MAP))
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            DXGI_FORMAT nmfmt = tformat;
            if (IsCompressed(tformat))
            {
                nmfmt = (dwNormalMap & CNMAP_COMPUTE_OCCLUSION) ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R32G32B32_FLOAT;
            }

            hr = ComputeNormalMap(image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwNormalMap, nmapAmplitude, nmfmt, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [normalmap] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert(tinfo.format == nmfmt);
            info.format = tinfo.format;

            assert(info.width == tinfo.width);
            assert(info.height == tinfo.height);
            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.mipLevels == tinfo.mipLevels);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }
        else if (info.format != tformat && !IsCompressed(tformat))
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            hr = Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(), tformat,
                dwFilter | dwFilterOpts | dwSRGB | dwConvert, TEX_THRESHOLD_DEFAULT, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [convert] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert(tinfo.format == tformat);
            info.format = tinfo.format;

            assert(info.width == tinfo.width);
            assert(info.height == tinfo.height);
            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.mipLevels == tinfo.mipLevels);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }

        // --- ColorKey/ChromaKey ------------------------------------------------------
        if ((dwOptions & (DWORD64(1) << OPT_COLORKEY))
            && HasAlpha(info.format))
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            XMVECTOR colorKeyValue = XMLoadColor(reinterpret_cast<const XMCOLOR*>(&colorKey));

            hr = TransformImage(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
                [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
            {
                static const XMVECTORF32 s_tolerance = { 0.2f, 0.2f, 0.2f, 0.f };

                UNREFERENCED_PARAMETER(y);

                for (size_t j = 0; j < width; ++j)
                {
                    XMVECTOR value = inPixels[j];

                    if (XMVector3NearEqual(value, colorKeyValue, s_tolerance))
                    {
                        value = g_XMZero;
                    }
                    else
                    {
                        value = XMVectorSelect(g_XMOne, value, g_XMSelect1110);
                    }

                    outPixels[j] = value;
                }
            }, *timage);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [colorkey] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();
            tinfo;

            assert(info.width == tinfo.width);
            assert(info.height == tinfo.height);
            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.mipLevels == tinfo.mipLevels);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.format == tinfo.format);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }

        // --- Generate mips -----------------------------------------------------------
        if (!ispow2(info.width) || !ispow2(info.height) || !ispow2(info.depth))
        {
            if (info.dimension == TEX_DIMENSION_TEXTURE3D)
            {
                if (!tMips)
                {
                    tMips = 1;
                }
                else
                {
                    wprintf(L"\nERROR: Cannot generate mips for non-power-of-2 volume textures\n");
                    return 1;
                }
            }
            else if (!tMips || info.mipLevels != 1)
            {
                nonpow2warn = true;
            }
        }

        if ((!tMips || info.mipLevels != tMips) && (info.mipLevels != 1))
        {
            // Mips generation only works on a single base image, so strip off existing mip levels
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            TexMetadata mdata = info;
            mdata.mipLevels = 1;
            hr = timage->Initialize(mdata);
            if (FAILED(hr))
            {
                wprintf(L" FAILED [copy to single level] (%x)\n", hr);
                return 1;
            }

            if (info.dimension == TEX_DIMENSION_TEXTURE3D)
            {
                for (size_t d = 0; d < info.depth; ++d)
                {
                    hr = CopyRectangle(*image->GetImage(0, 0, d), Rect(0, 0, info.width, info.height),
                        *timage->GetImage(0, 0, d), TEX_FILTER_DEFAULT, 0, 0);
                    if (FAILED(hr))
                    {
                        wprintf(L" FAILED [copy to single level] (%x)\n", hr);
                        return 1;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < info.arraySize; ++i)
                {
                    hr = CopyRectangle(*image->GetImage(0, i, 0), Rect(0, 0, info.width, info.height),
                        *timage->GetImage(0, i, 0), TEX_FILTER_DEFAULT, 0, 0);
                    if (FAILED(hr))
                    {
                        wprintf(L" FAILED [copy to single level] (%x)\n", hr);
                        return 1;
                    }
                }
            }

            image.swap(timage);
            info.mipLevels = image->GetMetadata().mipLevels;

            if (cimage && (tMips == 1))
            {
                // Special case for trimming mips off compressed images and keeping the original compressed highest level mip
                mdata = cimage->GetMetadata();
                mdata.mipLevels = 1;
                hr = timage->Initialize(mdata);
                if (FAILED(hr))
                {
                    wprintf(L" FAILED [copy compressed to single level] (%x)\n", hr);
                    return 1;
                }

                if (mdata.dimension == TEX_DIMENSION_TEXTURE3D)
                {
                    for (size_t d = 0; d < mdata.depth; ++d)
                    {
                        auto simg = cimage->GetImage(0, 0, d);
                        auto dimg = timage->GetImage(0, 0, d);

                        memcpy_s(dimg->pixels, dimg->slicePitch, simg->pixels, simg->slicePitch);
                    }
                }
                else
                {
                    for (size_t i = 0; i < mdata.arraySize; ++i)
                    {
                        auto simg = cimage->GetImage(0, i, 0);
                        auto dimg = timage->GetImage(0, i, 0);

                        memcpy_s(dimg->pixels, dimg->slicePitch, simg->pixels, simg->slicePitch);
                    }
                }

                cimage.swap(timage);
            }
            else
            {
                cimage.reset();
            }
        }

        if ((!tMips || info.mipLevels != tMips) && (info.width > 1 || info.height > 1 || info.depth > 1))
        {
            std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
            if (!timage)
            {
                wprintf(L"\nERROR: Memory allocation failed\n");
                return 1;
            }

            if (info.dimension == TEX_DIMENSION_TEXTURE3D)
            {
                hr = GenerateMipMaps3D(image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFilter | dwFilterOpts, tMips, *timage);
            }
            else
            {
                hr = GenerateMipMaps(image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFilter | dwFilterOpts, tMips, *timage);
            }
            if (FAILED(hr))
            {
                wprintf(L" FAILED [mipmaps] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();
            info.mipLevels = tinfo.mipLevels;

            assert(info.width == tinfo.width);
            assert(info.height == tinfo.height);
            assert(info.depth == tinfo.depth);
            assert(info.arraySize == tinfo.arraySize);
            assert(info.miscFlags == tinfo.miscFlags);
            assert(info.format == tinfo.format);
            assert(info.dimension == tinfo.dimension);

            image.swap(timage);
            cimage.reset();
        }

        // --- Premultiplied alpha (if requested) --------------------------------------
        if ((dwOptions & (DWORD64(1) << OPT_PREMUL_ALPHA))
            && HasAlpha(info.format)
            && info.format != DXGI_FORMAT_A8_UNORM)
        {
            if (info.IsPMAlpha())
            {
                printf("\nWARNING: Image is already using premultiplied alpha\n");
            }
            else
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

                hr = PremultiplyAlpha(img, nimg, info, dwSRGB, *timage);
                if (FAILED(hr))
                {
                    wprintf(L" FAILED [premultiply alpha] (%x)\n", hr);
                    continue;
                }

                auto& tinfo = timage->GetMetadata();
                info.miscFlags2 = tinfo.miscFlags2;

                assert(info.width == tinfo.width);
                assert(info.height == tinfo.height);
                assert(info.depth == tinfo.depth);
                assert(info.arraySize == tinfo.arraySize);
                assert(info.mipLevels == tinfo.mipLevels);
                assert(info.miscFlags == tinfo.miscFlags);
                assert(info.dimension == tinfo.dimension);

                image.swap(timage);
                cimage.reset();
            }
        }

        // --- Compress ----------------------------------------------------------------
        if (IsCompressed(tformat) && (FileType == CODEC_DDS))
        {
            if (cimage && (cimage->GetMetadata().format == tformat))
            {
                // We never changed the image and it was already compressed in our desired format, use original data
                image.reset(cimage.release());

                auto& tinfo = image->GetMetadata();

                if ((tinfo.width % 4) != 0 || (tinfo.height % 4) != 0)
                {
                    non4bc = true;
                }

                info.format = tinfo.format;
                assert(info.width == tinfo.width);
                assert(info.height == tinfo.height);
                assert(info.depth == tinfo.depth);
                assert(info.arraySize == tinfo.arraySize);
                assert(info.mipLevels == tinfo.mipLevels);
                assert(info.miscFlags == tinfo.miscFlags);
                assert(info.dimension == tinfo.dimension);
            }
            else
            {
                cimage.reset();

                auto img = image->GetImage(0, 0, 0);
                assert(img);
                size_t nimg = image->GetImageCount();

                std::unique_ptr<ScratchImage> timage(new (std::nothrow) ScratchImage);
                if (!timage)
                {
                    wprintf(L"\nERROR: Memory allocation failed\n");
                    return 1;
                }

                bool bc6hbc7 = false;
                switch (tformat)
                {
                case DXGI_FORMAT_BC6H_TYPELESS:
                case DXGI_FORMAT_BC6H_UF16:
                case DXGI_FORMAT_BC6H_SF16:
                case DXGI_FORMAT_BC7_TYPELESS:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    bc6hbc7 = true;

                    {
                        static bool s_tryonce = false;

                        if (!s_tryonce)
                        {
                            s_tryonce = true;

                            if (!(dwOptions & (DWORD64(1) << OPT_NOGPU)))
                            {
                                if (!CreateDevice(adapter, pDevice.GetAddressOf()))
                                    wprintf(L"\nWARNING: DirectCompute is not available, using BC6H / BC7 CPU codec\n");
                            }
                            else
                            {
                                wprintf(L"\nWARNING: using BC6H / BC7 CPU codec\n");
                            }
                        }
                    }
                    break;
                }

                DWORD cflags = dwCompress;
#ifdef _OPENMP
                if (!(dwOptions & (DWORD64(1) << OPT_FORCE_SINGLEPROC)))
                {
                    cflags |= TEX_COMPRESS_PARALLEL;
                }
#endif

                if ((img->width % 4) != 0 || (img->height % 4) != 0)
                {
                    non4bc = true;
                }

                if (bc6hbc7 && pDevice)
                {
                    hr = Compress(pDevice.Get(), img, nimg, info, tformat, dwCompress | dwSRGB, alphaWeight, *timage);
                }
                else
                {
                    hr = Compress(img, nimg, info, tformat, cflags | dwSRGB, TEX_THRESHOLD_DEFAULT, *timage);
                }
                if (FAILED(hr))
                {
                    wprintf(L" FAILED [compress] (%x)\n", hr);
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
        }
        else
        {
            cimage.reset();
        }

        // --- Set alpha mode ----------------------------------------------------------
        if (HasAlpha(info.format)
            && info.format != DXGI_FORMAT_A8_UNORM)
        {
            if (image->IsAlphaAllOpaque())
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_OPAQUE);
            }
            else if (info.IsPMAlpha())
            {
                // Aleady set TEX_ALPHA_MODE_PREMULTIPLIED
            }
            else if (dwOptions & (DWORD64(1) << OPT_SEPALPHA))
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_CUSTOM);
            }
            else if (info.GetAlphaMode() == TEX_ALPHA_MODE_UNKNOWN)
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_STRAIGHT);
            }
        }
        else
        {
            info.SetAlphaMode(TEX_ALPHA_MODE_UNKNOWN);
        }

        // --- Save result -------------------------------------------------------------
        {
            auto img = image->GetImage(0, 0, 0);
            assert(img);
            size_t nimg = image->GetImageCount();

            PrintInfo(info);
            wprintf(L"\n");

            // Figure out dest filename
            wchar_t *pchSlash, *pchDot;

            wcscpy_s(pConv->szDest, MAX_PATH, szPrefix);

            pchSlash = wcsrchr(pConv->szSrc, L'\\');
            if (pchSlash != 0)
                wcscat_s(pConv->szDest, MAX_PATH, pchSlash + 1);
            else
                wcscat_s(pConv->szDest, MAX_PATH, pConv->szSrc);

            pchSlash = wcsrchr(pConv->szDest, '\\');
            pchDot = wcsrchr(pConv->szDest, '.');

            if (pchDot > pchSlash)
                *pchDot = 0;

            wcscat_s(pConv->szDest, MAX_PATH, szSuffix);

            // Write texture
            wprintf(L"writing %ls", pConv->szDest);
            fflush(stdout);

            if (~dwOptions & (DWORD64(1) << OPT_OVERWRITE))
            {
                if (GetFileAttributesW(pConv->szDest) != INVALID_FILE_ATTRIBUTES)
                {
                    wprintf(L"\nERROR: Output file already exists, use -y to overwrite:\n");
                    continue;
                }
            }

            switch (FileType)
            {
            case CODEC_DDS:
                hr = SaveToDDSFile(img, nimg, info,
                    (dwOptions & (DWORD64(1) << OPT_USE_DX10)) ? (DDS_FLAGS_FORCE_DX10_EXT | DDS_FLAGS_FORCE_DX10_EXT_MISC2) : DDS_FLAGS_NONE,
                    pConv->szDest);
                break;

            case CODEC_TGA:
                hr = SaveToTGAFile(img[0], pConv->szDest);
                break;

            case CODEC_HDR:
                hr = SaveToHDRFile(img[0], pConv->szDest);
                break;

#ifdef USE_OPENEXR
            case CODEC_EXR:
                hr = SaveToEXRFile(img[0], pConv->szDest);
                break;
#endif

            default:
            {
                WICCodecs codec = (FileType == CODEC_HDP || FileType == CODEC_JXR) ? WIC_CODEC_WMP : static_cast<WICCodecs>(FileType);
                size_t nimages = (dwOptions & (DWORD64(1) << OPT_WIC_MULTIFRAME)) ? nimg : 1;
                hr = SaveToWICFile(img, nimages, WIC_FLAGS_NONE, GetWICCodec(codec), pConv->szDest, nullptr,
                    [&](IPropertyBag2* props)
                {
                    bool wicLossless = (dwOptions & (DWORD64(1) << OPT_WIC_LOSSLESS)) != 0;

                    switch (FileType)
                    {
                    case WIC_CODEC_JPEG:
                        if (wicLossless || wicQuality >= 0.f)
                        {
                            PROPBAG2 options = {};
                            VARIANT varValues = {};
                            options.pstrName = const_cast<wchar_t*>(L"ImageQuality");
                            varValues.vt = VT_R4;
                            varValues.fltVal = (wicLossless) ? 1.f : wicQuality;
                            (void)props->Write(1, &options, &varValues);
                        }
                        break;

                    case WIC_CODEC_TIFF:
                    {
                        PROPBAG2 options = {};
                        VARIANT varValues = {};
                        if (wicLossless)
                        {
                            options.pstrName = const_cast<wchar_t*>(L"TiffCompressionMethod");
                            varValues.vt = VT_UI1;
                            varValues.bVal = WICTiffCompressionNone;
                        }
                        else if (wicQuality >= 0.f)
                        {
                            options.pstrName = const_cast<wchar_t*>(L"CompressionQuality");
                            varValues.vt = VT_R4;
                            varValues.fltVal = wicQuality;
                        }
                        (void)props->Write(1, &options, &varValues);
                    }
                    break;

                    case WIC_CODEC_WMP:
                    case CODEC_HDP:
                    case CODEC_JXR:
                    {
                        PROPBAG2 options = {};
                        VARIANT varValues = {};
                        if (wicLossless)
                        {
                            options.pstrName = const_cast<wchar_t*>(L"Lossless");
                            varValues.vt = VT_BOOL;
                            varValues.bVal = TRUE;
                        }
                        else if (wicQuality >= 0.f)
                        {
                            options.pstrName = const_cast<wchar_t*>(L"ImageQuality");
                            varValues.vt = VT_R4;
                            varValues.fltVal = wicQuality;
                        }
                        (void)props->Write(1, &options, &varValues);
                    }
                    break;
                    }
                });
            }
            break;
            }

            if (FAILED(hr))
            {
                wprintf(L" FAILED (%x)\n", hr);
                continue;
            }
            wprintf(L"\n");
        }
    }

    if (nonpow2warn && maxSize <= 4096)
    {
        // Only emit this warning if ran with -fl set to a 9.x feature level
        wprintf(L"\nWARNING: Not all feature levels support non-power-of-2 textures with mipmaps\n");
    }

    if (non4bc)
        wprintf(L"\nWARNING: Direct3D requires BC image to be multiple of 4 in width & height\n");

    if (dwOptions & (DWORD64(1) << OPT_TIMING))
    {
        LARGE_INTEGER qpcEnd;
        if (QueryPerformanceCounter(&qpcEnd))
        {
            LONGLONG delta = qpcEnd.QuadPart - qpcStart.QuadPart;
            wprintf(L"\n Processing time: %f seconds\n", double(delta) / double(qpcFreq.QuadPart));
        }
    }

    return 0;
}
