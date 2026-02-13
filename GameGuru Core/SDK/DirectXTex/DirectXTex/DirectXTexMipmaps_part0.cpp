//-------------------------------------------------------------------------------------
// DirectXTexMipMaps.cpp
//  
// DirectX Texture Library - Mip-map generation
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

#include "filters.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    inline bool ispow2(_In_ size_t x)
    {
        return ((x != 0) && !(x & (x - 1)));
    }


    size_t _CountMips(_In_ size_t width, _In_ size_t height)
    {
        size_t mipLevels = 1;

        while (height > 1 || width > 1)
        {
            if (height > 1)
                height >>= 1;

            if (width > 1)
                width >>= 1;

            ++mipLevels;
        }

        return mipLevels;
    }


    size_t _CountMips3D(_In_ size_t width, _In_ size_t height, _In_ size_t depth)
    {
        size_t mipLevels = 1;

        while (height > 1 || width > 1 || depth > 1)
        {
            if (height > 1)
                height >>= 1;

            if (width > 1)
                width >>= 1;

            if (depth > 1)
                depth >>= 1;

            ++mipLevels;
        }

        return mipLevels;
    }


    HRESULT EnsureWicBitmapPixelFormat(
        _In_ IWICImagingFactory* pWIC,
        _In_ IWICBitmap* src,
        _In_ DWORD filter,
        _In_ const WICPixelFormatGUID& desiredPixelFormat,
        _Deref_out_ IWICBitmap** dest)
    {
        if (!pWIC || !src || !dest)
            return E_POINTER;

        *dest = nullptr;

        WICPixelFormatGUID actualPixelFormat;
        HRESULT hr = src->GetPixelFormat(&actualPixelFormat);

        if (SUCCEEDED(hr))
        {
            if (memcmp(&actualPixelFormat, &desiredPixelFormat, sizeof(WICPixelFormatGUID)) == 0)
            {
                src->AddRef();
                *dest = src;
            }
            else
            {
                ComPtr<IWICFormatConverter> converter;
                hr = pWIC->CreateFormatConverter(converter.GetAddressOf());

                if (SUCCEEDED(hr))
                {
                    BOOL canConvert = FALSE;
                    hr = converter->CanConvert(actualPixelFormat, desiredPixelFormat, &canConvert);
                    if (FAILED(hr) || !canConvert)
                    {
                        return E_UNEXPECTED;
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = converter->Initialize(src, desiredPixelFormat, _GetWICDither(filter), nullptr, 0, WICBitmapPaletteTypeMedianCut);
                }

                if (SUCCEEDED(hr))
                {
                    hr = pWIC->CreateBitmapFromSource(converter.Get(), WICBitmapCacheOnDemand, dest);
                }
            }
        }

        return hr;
    }
}


namespace DirectX
{
    bool _CalculateMipLevels(_In_ size_t width, _In_ size_t height, _Inout_ size_t& mipLevels)
    {
        if (mipLevels > 1)
        {
            size_t maxMips = _CountMips(width, height);
            if (mipLevels > maxMips)
                return false;
        }
        else if (mipLevels == 0)
        {
            mipLevels = _CountMips(width, height);
        }
        else
        {
            mipLevels = 1;
        }
        return true;
    }

    bool _CalculateMipLevels3D(_In_ size_t width, _In_ size_t height, _In_ size_t depth, _Inout_ size_t& mipLevels)
    {
        if (mipLevels > 1)
        {
            size_t maxMips = _CountMips3D(width, height, depth);
            if (mipLevels > maxMips)
                return false;
        }
        else if (mipLevels == 0)
        {
            mipLevels = _CountMips3D(width, height, depth);
        }
        else
        {
            mipLevels = 1;
        }
        return true;
    }

    //--- Resizing color and alpha channels separately using WIC ---
    HRESULT _ResizeSeparateColorAndAlpha(
        _In_ IWICImagingFactory* pWIC,
        _In_ bool iswic2,
        _In_ IWICBitmap* original,
        _In_ size_t newWidth,
        _In_ size_t newHeight,
        _In_ DWORD filter,
        _Inout_ const Image* img)
    {
        if (!pWIC || !original || !img)
            return E_POINTER;

        const WICBitmapInterpolationMode interpolationMode = _GetWICInterp(filter);

        WICPixelFormatGUID desiredPixelFormat = GUID_WICPixelFormatUndefined;
        HRESULT hr = original->GetPixelFormat(&desiredPixelFormat);

        size_t colorBytesInPixel = 0;
        size_t colorBytesPerPixel = 0;
        size_t colorWithAlphaBytesPerPixel = 0;
        WICPixelFormatGUID colorPixelFormat = GUID_WICPixelFormatUndefined;
        WICPixelFormatGUID colorWithAlphaPixelFormat = GUID_WICPixelFormatUndefined;

        if (SUCCEEDED(hr))
        {
            ComPtr<IWICComponentInfo> componentInfo;
            hr = pWIC->CreateComponentInfo(desiredPixelFormat, componentInfo.GetAddressOf());

            ComPtr<IWICPixelFormatInfo> pixelFormatInfo;
            if (SUCCEEDED(hr))
            {
                hr = componentInfo.As(&pixelFormatInfo);
            }

            UINT bitsPerPixel = 0;
            if (SUCCEEDED(hr))
            {
                hr = pixelFormatInfo->GetBitsPerPixel(&bitsPerPixel);
            }

            if (SUCCEEDED(hr))
            {
                if (bitsPerPixel <= 32)
                {
                    colorBytesInPixel = colorBytesPerPixel = 3;
                    colorPixelFormat = GUID_WICPixelFormat24bppBGR;

                    colorWithAlphaBytesPerPixel = 4;
                    colorWithAlphaPixelFormat = GUID_WICPixelFormat32bppBGRA;
                }
                else
                {
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
                    if (iswic2)
                    {
                        colorBytesInPixel = colorBytesPerPixel = 12;
                        colorPixelFormat = GUID_WICPixelFormat96bppRGBFloat;
                    }
                    else
#else
                    UNREFERENCED_PARAMETER(iswic2);
#endif
                    {
                        colorBytesInPixel = 12;
                        colorBytesPerPixel = 16;
                        colorPixelFormat = GUID_WICPixelFormat128bppRGBFloat;
                    }

                    colorWithAlphaBytesPerPixel = 16;
                    colorWithAlphaPixelFormat = GUID_WICPixelFormat128bppRGBAFloat;
                }
            }
        }

        // Resize color only image (no alpha channel)
        ComPtr<IWICBitmap> resizedColor;
        if (SUCCEEDED(hr))
        {
            ComPtr<IWICBitmapScaler> colorScaler;
            hr = pWIC->CreateBitmapScaler(colorScaler.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                ComPtr<IWICBitmap> converted;
                hr = EnsureWicBitmapPixelFormat(pWIC, original, filter, colorPixelFormat, converted.GetAddressOf());
                if (SUCCEEDED(hr))
                {
                    hr = colorScaler->Initialize(converted.Get(), static_cast<UINT>(newWidth), static_cast<UINT>(newHeight), interpolationMode);
                }
            }

            if (SUCCEEDED(hr))
            {
                ComPtr<IWICBitmap> resized;
                hr = pWIC->CreateBitmapFromSource(colorScaler.Get(), WICBitmapCacheOnDemand, resized.GetAddressOf());
                if (SUCCEEDED(hr))
                {
                    hr = EnsureWicBitmapPixelFormat(pWIC, resized.Get(), filter, colorPixelFormat, resizedColor.GetAddressOf());
                }
            }
        }

        // Resize color+alpha image
        ComPtr<IWICBitmap> resizedColorWithAlpha;
        if (SUCCEEDED(hr))
        {
            ComPtr<IWICBitmapScaler> colorWithAlphaScaler;
            hr = pWIC->CreateBitmapScaler(colorWithAlphaScaler.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                ComPtr<IWICBitmap> converted;
                hr = EnsureWicBitmapPixelFormat(pWIC, original, filter, colorWithAlphaPixelFormat, converted.GetAddressOf());
                if (SUCCEEDED(hr))
                {
                    hr = colorWithAlphaScaler->Initialize(converted.Get(), static_cast<UINT>(newWidth), static_cast<UINT>(newHeight), interpolationMode);
                }
            }

            if (SUCCEEDED(hr))
            {
                ComPtr<IWICBitmap> resized;
                hr = pWIC->CreateBitmapFromSource(colorWithAlphaScaler.Get(), WICBitmapCacheOnDemand, resized.GetAddressOf());
                if (SUCCEEDED(hr))
                {
                    hr = EnsureWicBitmapPixelFormat(pWIC, resized.Get(), filter, colorWithAlphaPixelFormat, resizedColorWithAlpha.GetAddressOf());
                }
            }
        }

        // Merge pixels (copying color channels from color only image to color+alpha image)
        if (SUCCEEDED(hr))
        {
            ComPtr<IWICBitmapLock> colorLock;
            ComPtr<IWICBitmapLock> colorWithAlphaLock;
            hr = resizedColor->Lock(nullptr, WICBitmapLockRead, colorLock.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = resizedColorWithAlpha->Lock(nullptr, WICBitmapLockWrite, colorWithAlphaLock.GetAddressOf());
            }

            if (SUCCEEDED(hr))
            {
                WICInProcPointer colorWithAlphaData = nullptr;
                UINT colorWithAlphaSizeInBytes = 0;
                UINT colorWithAlphaStride = 0;

                hr = colorWithAlphaLock->GetDataPointer(&colorWithAlphaSizeInBytes, &colorWithAlphaData);
                if (SUCCEEDED(hr))
                {
                    if (!colorWithAlphaData)
                    {
                        hr = E_POINTER;
                    }
                    else
                    {
                        hr = colorWithAlphaLock->GetStride(&colorWithAlphaStride);
                    }
                }

                WICInProcPointer colorData = nullptr;
                UINT colorSizeInBytes = 0;
                UINT colorStride = 0;
                if (SUCCEEDED(hr))
                {
                    hr = colorLock->GetDataPointer(&colorSizeInBytes, &colorData);
                    if (SUCCEEDED(hr))
                    {
                        if (!colorData)
                        {
                            hr = E_POINTER;
                        }
                        else
                        {
                            hr = colorLock->GetStride(&colorStride);
                        }
                    }
                }

                for (size_t j = 0; SUCCEEDED(hr) && j < newHeight; j++)
                {
                    for (size_t i = 0; SUCCEEDED(hr) && i < newWidth; i++)
                    {
                        size_t colorWithAlphaIndex = (j * colorWithAlphaStride) + (i * colorWithAlphaBytesPerPixel);
                        size_t colorIndex = (j * colorStride) + (i * colorBytesPerPixel);

                        if (((colorWithAlphaIndex + colorBytesInPixel) > colorWithAlphaSizeInBytes)
                            || ((colorIndex + colorBytesPerPixel) > colorSizeInBytes))
                        {
                            hr = E_INVALIDARG;
                        }
                        else
                        {
#pragma warning( suppress : 26014 6386 ) // No overflow possible here
                            memcpy_s(colorWithAlphaData + colorWithAlphaIndex, colorWithAlphaBytesPerPixel, colorData + colorIndex, colorBytesInPixel);
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            ComPtr<IWICBitmap> wicBitmap;
            hr = EnsureWicBitmapPixelFormat(pWIC, resizedColorWithAlpha.Get(), filter, desiredPixelFormat, wicBitmap.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = wicBitmap->CopyPixels(nullptr, static_cast<UINT>(img->rowPitch), static_cast<UINT>(img->slicePitch), img->pixels);
            }
        }

        return hr;
    }
}

