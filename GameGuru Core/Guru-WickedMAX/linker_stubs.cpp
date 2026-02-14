// linker_stubs.cpp - Stub implementations for missing symbols
// These are needed to satisfy the linker but may not be fully functional at runtime

#include <cstdint>
#include <cstdlib>
#include <windows.h>

// ============================================================================
// 1. g_iActiveAdapterNumber - referenced by DBDLLCore but never defined
// ============================================================================
uint32_t g_iActiveAdapterNumber = 0;

// ============================================================================
// 2. wi::backlog::Scroll(int) - header declares int, but .cpp defines float
// ============================================================================
namespace wi {
namespace backlog {
    // The implementation in WickedEngine uses float, but the header declares int
    // Forward-declare the float version and wrap it
    extern void Scroll(float direction);
}
}

// Provide the int overload that the header promises
namespace wi {
namespace backlog {
    void Scroll(int direction) {
        Scroll(static_cast<float>(direction));
    }
}
}

// ============================================================================
// 3. D3DX11 stub functions - from legacy DirectX SDK, not available
// ============================================================================

// Forward declarations for D3DX11 types
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Resource;
struct ID3DX11ThreadPump;

// Minimal D3DX11_IMAGE_INFO struct (from D3DX11tex.h)
typedef struct _D3DX11_IMAGE_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT ArraySize;
    UINT MipLevels;
    UINT MiscFlags;
    UINT Format;
    UINT ResourceDimension;
    UINT ImageFileFormat;
} D3DX11_IMAGE_INFO;

// D3DX11_IMAGE_LOAD_INFO
typedef struct _D3DX11_IMAGE_LOAD_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT FirstMipLevel;
    UINT MipLevels;
    UINT Usage;
    UINT BindFlags;
    UINT CpuAccessFlags;
    UINT MiscFlags;
    UINT Format;
    UINT Filter;
    UINT MipFilter;
    void* pSrcInfo;
} D3DX11_IMAGE_LOAD_INFO;

typedef UINT D3DX11_IMAGE_FILE_FORMAT;

extern "C" {

HRESULT WINAPI D3DX11CreateTextureFromFileA(
    ID3D11Device* pDevice,
    LPCSTR pSrcFile,
    D3DX11_IMAGE_LOAD_INFO* pLoadInfo,
    ID3DX11ThreadPump* pPump,
    ID3D11Resource** ppTexture,
    HRESULT* pHResult)
{
    if (ppTexture) *ppTexture = nullptr;
    if (pHResult) *pHResult = E_NOTIMPL;
    return E_NOTIMPL;
}

HRESULT WINAPI D3DX11GetImageInfoFromFileA(
    LPCSTR pSrcFile,
    ID3DX11ThreadPump* pPump,
    D3DX11_IMAGE_INFO* pSrcInfo,
    HRESULT* pHResult)
{
    if (pSrcInfo) memset(pSrcInfo, 0, sizeof(D3DX11_IMAGE_INFO));
    if (pHResult) *pHResult = E_NOTIMPL;
    return E_NOTIMPL;
}

HRESULT WINAPI D3DX11GetImageInfoFromMemory(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    ID3DX11ThreadPump* pPump,
    D3DX11_IMAGE_INFO* pSrcInfo,
    HRESULT* pHResult)
{
    if (pSrcInfo) memset(pSrcInfo, 0, sizeof(D3DX11_IMAGE_INFO));
    if (pHResult) *pHResult = E_NOTIMPL;
    return E_NOTIMPL;
}

HRESULT WINAPI D3DX11SaveTextureToFileA(
    ID3D11DeviceContext* pContext,
    ID3D11Resource* pSrcTexture,
    D3DX11_IMAGE_FILE_FORMAT DestFormat,
    LPCSTR pDestFile)
{
    return E_NOTIMPL;
}

} // extern "C"
