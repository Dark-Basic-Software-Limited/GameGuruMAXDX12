// Phase 5: ImGui DX12 Bridge Implementation
// Self-contained DX12 ImGui renderer compatible with ImGui 1.73.
// Does NOT use imgui_impl_dx12.cpp (which requires ImGui 1.92+).
// Based on the same rendering approach as the existing DX11 ImGuiHook_RenderCall_Direct.

#include "imgui.h"
#include "imgui_gg_dx12_bridge.h"

#include <d3d12.h>
// d3dcompiler no longer needed - shaders are pre-compiled
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <unordered_map>

// d3dcompiler lib no longer needed - shaders are pre-compiled
#pragma comment(lib, "d3d12")

// stb_image for loading PNG/JPG/BMP/TGA files (implementation in WickedEngine_Windows.lib)
#include "../Include/Utility/stb_image.h"

// WickedEngine headers for device access
#include "wiGraphicsDevice_DX12.h"
#include "wiGraphics.h"

using Microsoft::WRL::ComPtr;

// DX12 state for ImGui rendering
static bool g_ImGuiDX12Initialized = false;
static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;

// Pipeline state
static ComPtr<ID3D12RootSignature> g_pRootSignature;
static ComPtr<ID3D12PipelineState> g_pPipelineState;

// Descriptor heap for ImGui SRV (font texture + UI images)
static const UINT IMGUI_SRV_HEAP_SIZE = 512;
static ComPtr<ID3D12DescriptorHeap> g_pd3dSrvDescHeap;
static UINT g_SrvDescriptorSize = 0;

// Simple free-list for SRV descriptors
static bool g_SrvSlotUsed[IMGUI_SRV_HEAP_SIZE] = {};

// Font texture
static ComPtr<ID3D12Resource> g_pFontTextureResource;
static D3D12_GPU_DESCRIPTOR_HANDLE g_FontSrvGpuHandle = {};
static bool g_FontTextureCreated = false;

// Texture cache for UI images (image ID → DX12 texture data)
struct DX12CachedTexture
{
    ComPtr<ID3D12Resource> Resource;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = {};
    UINT SrvSlotIndex = 0;
    int Width = 0;
    int Height = 0;
};
static std::unordered_map<int, DX12CachedTexture> g_TextureCache;

// Per-frame resources (double-buffered)
static const int NUM_FRAMES_IN_FLIGHT = 2;
struct FrameResources
{
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;
    int VertexBufferSize = 0;
    int IndexBufferSize = 0;
};
static FrameResources g_FrameResources[NUM_FRAMES_IN_FLIGHT];
static UINT g_FrameIndex = 0;

// Vertex constant buffer layout
struct VERTEX_CONSTANT_BUFFER_DX12
{
    float mvp[4][4];
};

// Pre-compiled shader bytecode (vs_5_0 and ps_5_0, compiled with fxc.exe)
// This eliminates the runtime D3DCompile dependency.
static const BYTE g_ImGuiVS[] =
{
     68,  88,  66,  67,  20, 173,   7,  97,  75,  46,  94,  62,
     52, 248, 140,  61, 105, 223,  61,  15,   1,   0,   0,   0,
    220,   3,   0,   0,   5,   0,   0,   0,  52,   0,   0,   0,
     80,   1,   0,   0, 192,   1,   0,   0,  52,   2,   0,   0,
     64,   3,   0,   0,  82,  68,  69,  70,  20,   1,   0,   0,
      1,   0,   0,   0, 108,   0,   0,   0,   1,   0,   0,   0,
     60,   0,   0,   0,   0,   5, 254, 255,   0,   1,   0,   0,
    236,   0,   0,   0,  82,  68,  49,  49,  60,   0,   0,   0,
     24,   0,   0,   0,  32,   0,   0,   0,  40,   0,   0,   0,
     36,   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,
     92,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      1,   0,   0,   0,   1,   0,   0,   0, 118, 101, 114, 116,
    101, 120,  66, 117, 102, 102, 101, 114,   0, 171, 171, 171,
     92,   0,   0,   0,   1,   0,   0,   0, 132,   0,   0,   0,
     64,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    172,   0,   0,   0,   0,   0,   0,   0,  64,   0,   0,   0,
      2,   0,   0,   0, 200,   0,   0,   0,   0,   0,   0,   0,
    255, 255, 255, 255,   0,   0,   0,   0, 255, 255, 255, 255,
      0,   0,   0,   0,  80, 114, 111, 106, 101,  99, 116, 105,
    111, 110,  77,  97, 116, 114, 105, 120,   0, 102, 108, 111,
     97, 116,  52, 120,  52,   0, 171, 171,   3,   0,   3,   0,
      4,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0, 189,   0,   0,   0,  77, 105,  99, 114,
    111, 115, 111, 102, 116,  32,  40,  82,  41,  32,  72,  76,
     83,  76,  32,  83, 104,  97, 100, 101, 114,  32,  67, 111,
    109, 112, 105, 108, 101, 114,  32,  49,  48,  46,  49,   0,
     73,  83,  71,  78, 104,   0,   0,   0,   3,   0,   0,   0,
      8,   0,   0,   0,  80,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,
      3,   3,   0,   0,  89,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,   0,   0,   1,   0,   0,   0,
      3,   3,   0,   0,  98,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,   0,   0,   2,   0,   0,   0,
     15,  15,   0,   0,  80,  79,  83,  73,  84,  73,  79,  78,
      0,  84,  69,  88,  67,  79,  79,  82,  68,   0,  67,  79,
     76,  79,  82,   0,  79,  83,  71,  78, 108,   0,   0,   0,
      3,   0,   0,   0,   8,   0,   0,   0,  80,   0,   0,   0,
      0,   0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,
      0,   0,   0,   0,  15,   0,   0,   0,  92,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,
      1,   0,   0,   0,  15,   0,   0,   0,  98,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,
      2,   0,   0,   0,   3,  12,   0,   0,  83,  86,  95,  80,
     79,  83,  73,  84,  73,  79,  78,   0,  67,  79,  76,  79,
     82,   0,  84,  69,  88,  67,  79,  79,  82,  68,   0, 171,
     83,  72,  69,  88,   4,   1,   0,   0,  80,   0,   1,   0,
     65,   0,   0,   0, 106,   8,   0,   1,  89,   0,   0,   4,
     70, 142,  32,   0,   0,   0,   0,   0,   4,   0,   0,   0,
     95,   0,   0,   3,  50,  16,  16,   0,   0,   0,   0,   0,
     95,   0,   0,   3,  50,  16,  16,   0,   1,   0,   0,   0,
     95,   0,   0,   3, 242,  16,  16,   0,   2,   0,   0,   0,
    103,   0,   0,   4, 242,  32,  16,   0,   0,   0,   0,   0,
      1,   0,   0,   0, 101,   0,   0,   3, 242,  32,  16,   0,
      1,   0,   0,   0, 101,   0,   0,   3,  50,  32,  16,   0,
      2,   0,   0,   0, 104,   0,   0,   2,   1,   0,   0,   0,
     56,   0,   0,   8, 242,   0,  16,   0,   0,   0,   0,   0,
     86,  21,  16,   0,   0,   0,   0,   0,  70, 142,  32,   0,
      0,   0,   0,   0,   1,   0,   0,   0,  50,   0,   0,  10,
    242,   0,  16,   0,   0,   0,   0,   0,  70, 142,  32,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   6,  16,  16,   0,
      0,   0,   0,   0,  70,  14,  16,   0,   0,   0,   0,   0,
      0,   0,   0,   8, 242,  32,  16,   0,   0,   0,   0,   0,
     70,  14,  16,   0,   0,   0,   0,   0,  70, 142,  32,   0,
      0,   0,   0,   0,   3,   0,   0,   0,  54,   0,   0,   5,
    242,  32,  16,   0,   1,   0,   0,   0,  70,  30,  16,   0,
      2,   0,   0,   0,  54,   0,   0,   5,  50,  32,  16,   0,
      2,   0,   0,   0,  70,  16,  16,   0,   1,   0,   0,   0,
     62,   0,   0,   1,  83,  84,  65,  84, 148,   0,   0,   0,
      6,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
      6,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0
};

static const BYTE g_ImGuiPS[] =
{
     68,  88,  66,  67, 111, 184, 221, 249, 217, 154,  13, 200,
     70, 122,  34,  91, 210,  73,  87,  63,   1,   0,   0,   0,
    224,   2,   0,   0,   5,   0,   0,   0,  52,   0,   0,   0,
    244,   0,   0,   0, 104,   1,   0,   0, 156,   1,   0,   0,
     68,   2,   0,   0,  82,  68,  69,  70, 184,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,
     60,   0,   0,   0,   0,   5, 255, 255,   0,   1,   0,   0,
    142,   0,   0,   0,  82,  68,  49,  49,  60,   0,   0,   0,
     24,   0,   0,   0,  32,   0,   0,   0,  40,   0,   0,   0,
     36,   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,
    124,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      1,   0,   0,   0,   1,   0,   0,   0, 133,   0,   0,   0,
      2,   0,   0,   0,   5,   0,   0,   0,   4,   0,   0,   0,
    255, 255, 255, 255,   0,   0,   0,   0,   1,   0,   0,   0,
     13,   0,   0,   0, 115,  97, 109, 112, 108, 101, 114,  48,
      0, 116, 101, 120, 116, 117, 114, 101,  48,   0,  77, 105,
     99, 114, 111, 115, 111, 102, 116,  32,  40,  82,  41,  32,
     72,  76,  83,  76,  32,  83, 104,  97, 100, 101, 114,  32,
     67, 111, 109, 112, 105, 108, 101, 114,  32,  49,  48,  46,
     49,   0, 171, 171,  73,  83,  71,  78, 108,   0,   0,   0,
      3,   0,   0,   0,   8,   0,   0,   0,  80,   0,   0,   0,
      0,   0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,
      0,   0,   0,   0,  15,   0,   0,   0,  92,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,
      1,   0,   0,   0,  15,  15,   0,   0,  98,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,
      2,   0,   0,   0,   3,   3,   0,   0,  83,  86,  95,  80,
     79,  83,  73,  84,  73,  79,  78,   0,  67,  79,  76,  79,
     82,   0,  84,  69,  88,  67,  79,  79,  82,  68,   0, 171,
     79,  83,  71,  78,  44,   0,   0,   0,   1,   0,   0,   0,
      8,   0,   0,   0,  32,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,
     15,   0,   0,   0,  83,  86,  95,  84,  97, 114, 103, 101,
    116,   0, 171, 171,  83,  72,  69,  88, 160,   0,   0,   0,
     80,   0,   0,   0,  40,   0,   0,   0, 106,   8,   0,   1,
     90,   0,   0,   3,   0,  96,  16,   0,   0,   0,   0,   0,
     88,  24,   0,   4,   0, 112,  16,   0,   0,   0,   0,   0,
     85,  85,   0,   0,  98,  16,   0,   3, 242,  16,  16,   0,
      1,   0,   0,   0,  98,  16,   0,   3,  50,  16,  16,   0,
      2,   0,   0,   0, 101,   0,   0,   3, 242,  32,  16,   0,
      0,   0,   0,   0, 104,   0,   0,   2,   1,   0,   0,   0,
     69,   0,   0, 139, 194,   0,   0, 128,  67,  85,  21,   0,
    242,   0,  16,   0,   0,   0,   0,   0,  70,  16,  16,   0,
      2,   0,   0,   0,  70, 126,  16,   0,   0,   0,   0,   0,
      0,  96,  16,   0,   0,   0,   0,   0,  56,   0,   0,   7,
    242,  32,  16,   0,   0,   0,   0,   0,  70,  14,  16,   0,
      0,   0,   0,   0,  70,  30,  16,   0,   1,   0,   0,   0,
     62,   0,   0,   1,  83,  84,  65,  84, 148,   0,   0,   0,
      3,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0
};

// Allocate an SRV slot from the ImGui descriptor heap
static bool AllocSrvSlot(UINT& outIndex, D3D12_CPU_DESCRIPTOR_HANDLE& outCpu, D3D12_GPU_DESCRIPTOR_HANDLE& outGpu)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < IMGUI_SRV_HEAP_SIZE; i++)
    {
        if (!g_SrvSlotUsed[i])
        {
            g_SrvSlotUsed[i] = true;
            outIndex = i;
            outCpu.ptr = cpuStart.ptr + i * g_SrvDescriptorSize;
            outGpu.ptr = gpuStart.ptr + i * g_SrvDescriptorSize;
            return true;
        }
    }
    return false;
}

static bool CreateFontTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture resource
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = g_pd3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&g_pFontTextureResource));
    if (FAILED(hr)) return false;

    // Create upload buffer
    UINT64 uploadSize = 0;
    g_pd3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width = uploadSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = g_pd3dDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(hr)) return false;

    // Create command allocator and command list for the upload
    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12Fence> fence;

    hr = g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    if (FAILED(hr)) return false;
    hr = g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    if (FAILED(hr)) return false;
    hr = g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) return false;

    // Copy texture data to upload buffer
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
    UINT numRows;
    UINT64 rowSizeInBytes;
    g_pd3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, &layout, &numRows, &rowSizeInBytes, &uploadSize);

    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    hr = uploadBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr)) return false;

    for (UINT y = 0; y < numRows; y++)
    {
        memcpy((char*)mapped + layout.Offset + y * layout.Footprint.RowPitch,
               pixels + y * width * 4,
               width * 4);
    }
    uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = uploadBuffer.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint = layout;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = g_pFontTextureResource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition to pixel shader resource
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = g_pFontTextureResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    hr = cmdList->Close();
    if (FAILED(hr)) return false;

    // Execute and wait
    ID3D12CommandList* ppCmdLists[] = { cmdList.Get() };
    g_pd3dCommandQueue->ExecuteCommandLists(1, ppCmdLists);
    g_pd3dCommandQueue->Signal(fence.Get(), 1);

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    // Create SRV
    UINT srvIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpu;
    if (!AllocSrvSlot(srvIndex, srvCpu, g_FontSrvGpuHandle))
        return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    g_pd3dDevice->CreateShaderResourceView(g_pFontTextureResource.Get(), &srvDesc, srvCpu);

    // Store font texture ID for ImGui (as GPU descriptor handle)
    io.Fonts->TexID = (ImTextureID)g_FontSrvGpuHandle.ptr;

    g_FontTextureCreated = true;
    return true;
}

static bool CreatePipelineState()
{
    // Shaders are pre-compiled - no runtime D3DCompile needed
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr;

    // Create root signature
    // Root parameter 0: 32-bit constants (MVP matrix, 16 floats)
    // Root parameter 1: Descriptor table (1 SRV for texture)
    D3D12_DESCRIPTOR_RANGE descRange = {};
    descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRange.NumDescriptors = 1;
    descRange.BaseShaderRegister = 0;
    descRange.RegisterSpace = 0;
    descRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER rootParams[2] = {};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].Constants.ShaderRegister = 0;
    rootParams[0].Constants.RegisterSpace = 0;
    rootParams[0].Constants.Num32BitValues = 16; // 4x4 matrix
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &descRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static sampler (linear clamp)
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.ShaderRegister = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> rootSigBlob;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
    if (FAILED(hr)) { OutputDebugStringA("ImGui DX12: D3D12SerializeRootSignature FAILED\n"); return false; }

    hr = g_pd3dDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&g_pRootSignature));
    if (FAILED(hr)) { OutputDebugStringA("ImGui DX12: CreateRootSignature FAILED\n"); return false; }

    // Create PSO
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, 3 };
    psoDesc.pRootSignature = g_pRootSignature.Get();
    psoDesc.VS = { g_ImGuiVS, sizeof(g_ImGuiVS) };
    psoDesc.PS = { g_ImGuiPS, sizeof(g_ImGuiPS) };

    // Rasterizer state
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // Blend state (alpha blending)
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // Depth stencil state (disabled)
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    // Must match WickedEngine's swapchain format (SwapChainDesc default: R10G10B10A2_UNORM)
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = g_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pPipelineState));
    if (FAILED(hr)) { OutputDebugStringA("ImGui DX12: CreateGraphicsPipelineState FAILED\n"); return false; }

    return true;
}

bool ImGui_DX12_InitBridge()
{
    // Get the WickedEngine DX12 device
    auto* graphicsDevice = wi::graphics::GetDevice();
    if (!graphicsDevice)
        return false;

    // static_cast: WickedEngine compiled with RTTI disabled (/GR-), dynamic_cast crashes
    auto* dx12Device = static_cast<wi::graphics::GraphicsDevice_DX12*>(graphicsDevice);
    if (!dx12Device)
        return false;

    g_pd3dDevice = dx12Device->GetDX12Device();
    g_pd3dCommandQueue = dx12Device->GetGraphicsCommandQueue();
    if (!g_pd3dDevice || !g_pd3dCommandQueue)
        return false;

    // Create shader-visible SRV descriptor heap for ImGui
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = IMGUI_SRV_HEAP_SIZE;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = g_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pd3dSrvDescHeap));
    if (FAILED(hr)) return false;

    g_SrvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    memset(g_SrvSlotUsed, 0, sizeof(g_SrvSlotUsed));

    // Always create font texture first — builds the ImGui font atlas.
    // If this isn't done, ImGui crashes in SetCurrentFont accessing null ContainerAtlas.
    if (!CreateFontTexture())
        return false;

    // Create pipeline state (root signature, shaders, PSO)
    if (!CreatePipelineState())
        return false;

    g_ImGuiDX12Initialized = true;
    return true;
}

void ImGui_DX12_NewFrame()
{
    if (!g_ImGuiDX12Initialized) return;

    // Check if font atlas needs rebuilding (e.g. after ChangeGGFont calls io.Fonts->Clear())
    ImGuiIO& io = ImGui::GetIO();
    if (!io.Fonts->IsBuilt() || !g_FontTextureCreated)
    {
        // Release old font texture resources
        g_pFontTextureResource.Reset();
        g_FontTextureCreated = false;
        CreateFontTexture();
    }
}

void ImGui_DX12_RenderBridge(ID3D12GraphicsCommandList* cmdList)
{
    if (!g_ImGuiDX12Initialized || !cmdList)
        return;

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->TotalVtxCount == 0)
        return;

    // Avoid rendering when minimized
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        return;

    // Get frame resources (double-buffered)
    g_FrameIndex = (g_FrameIndex + 1) % NUM_FRAMES_IN_FLIGHT;
    FrameResources* fr = &g_FrameResources[g_FrameIndex];

    // Create/grow vertex buffer
    if (!fr->VertexBuffer.Get() || fr->VertexBufferSize < drawData->TotalVtxCount)
    {
        fr->VertexBuffer.Reset();
        fr->VertexBufferSize = drawData->TotalVtxCount + 5000;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->VertexBufferSize * sizeof(ImDrawVert);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        g_pd3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->VertexBuffer));
    }

    // Create/grow index buffer
    if (!fr->IndexBuffer.Get() || fr->IndexBufferSize < drawData->TotalIdxCount)
    {
        fr->IndexBuffer.Reset();
        fr->IndexBufferSize = drawData->TotalIdxCount + 10000;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->IndexBufferSize * sizeof(ImDrawIdx);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        g_pd3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->IndexBuffer));
    }

    // Upload vertex/index data
    void* vtxMapped = nullptr;
    void* idxMapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(fr->VertexBuffer->Map(0, &readRange, &vtxMapped))) return;
    if (FAILED(fr->IndexBuffer->Map(0, &readRange, &idxMapped)))
    {
        fr->VertexBuffer->Unmap(0, nullptr);
        return;
    }

    ImDrawVert* vtxDst = (ImDrawVert*)vtxMapped;
    ImDrawIdx* idxDst = (ImDrawIdx*)idxMapped;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList_ = drawData->CmdLists[n];
        memcpy(vtxDst, cmdList_->VtxBuffer.Data, cmdList_->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmdList_->IdxBuffer.Data, cmdList_->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmdList_->VtxBuffer.Size;
        idxDst += cmdList_->IdxBuffer.Size;
    }

    D3D12_RANGE vtxWriteRange = { 0, (SIZE_T)(drawData->TotalVtxCount * sizeof(ImDrawVert)) };
    D3D12_RANGE idxWriteRange = { 0, (SIZE_T)(drawData->TotalIdxCount * sizeof(ImDrawIdx)) };
    fr->VertexBuffer->Unmap(0, &vtxWriteRange);
    fr->IndexBuffer->Unmap(0, &idxWriteRange);

    // Setup orthographic projection matrix
    VERTEX_CONSTANT_BUFFER_DX12 vertexConstantBuffer;
    {
        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
            { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
            { 0.0f,              0.0f,              0.5f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
        };
        memcpy(&vertexConstantBuffer.mvp, mvp, sizeof(mvp));
    }

    // Setup viewport
    D3D12_VIEWPORT vp = {};
    vp.Width = drawData->DisplaySize.x;
    vp.Height = drawData->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { g_pd3dSrvDescHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Set pipeline state
    cmdList->SetPipelineState(g_pPipelineState.Get());
    cmdList->SetGraphicsRootSignature(g_pRootSignature.Get());
    cmdList->SetGraphicsRoot32BitConstants(0, 16, &vertexConstantBuffer, 0);

    // Set vertex/index buffers
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = fr->VertexBuffer->GetGPUVirtualAddress();
    vbv.SizeInBytes = fr->VertexBufferSize * sizeof(ImDrawVert);
    vbv.StrideInBytes = sizeof(ImDrawVert);
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = fr->IndexBuffer->GetGPUVirtualAddress();
    ibv.SizeInBytes = fr->IndexBufferSize * sizeof(ImDrawIdx);
    ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    cmdList->IASetIndexBuffer(&ibv);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->RSSetViewports(1, &vp);

    // Set blend factor
    const float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    cmdList->OMSetBlendFactor(blendFactor);

    // Render draw commands
    int globalIdxOffset = 0;
    int globalVtxOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* imCmdList = drawData->CmdLists[n];
        for (int cmd_i = 0; cmd_i < imCmdList->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &imCmdList->CmdBuffer[cmd_i];

            if (pcmd->UserCallback != nullptr)
            {
                // Phase 5: Custom shader callbacks (blur, nowhite, etc.) are not yet ported to DX12
                // TODO: Port custom pixel shader variants to DX12
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    // Reset render state
                    cmdList->SetPipelineState(g_pPipelineState.Get());
                    cmdList->SetGraphicsRootSignature(g_pRootSignature.Get());
                    cmdList->SetGraphicsRoot32BitConstants(0, 16, &vertexConstantBuffer, 0);
                }
                continue;
            }

            // Skip if no texture
            if (!pcmd->TextureId)
                continue;

            // Apply scissor/clipping rectangle
            D3D12_RECT r;
            r.left = (LONG)(pcmd->ClipRect.x - clipOff.x);
            r.top = (LONG)(pcmd->ClipRect.y - clipOff.y);
            r.right = (LONG)(pcmd->ClipRect.z - clipOff.x);
            r.bottom = (LONG)(pcmd->ClipRect.w - clipOff.y);
            if (r.right <= r.left || r.bottom <= r.top)
                continue;
            cmdList->RSSetScissorRects(1, &r);

            // Bind texture
            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};
            textureHandle.ptr = (UINT64)pcmd->TextureId;
            cmdList->SetGraphicsRootDescriptorTable(1, textureHandle);

            // Draw
            cmdList->DrawIndexedInstanced(pcmd->ElemCount, 1,
                pcmd->IdxOffset + globalIdxOffset,
                pcmd->VtxOffset + globalVtxOffset, 0);
        }
        globalIdxOffset += imCmdList->IdxBuffer.Size;
        globalVtxOffset += imCmdList->VtxBuffer.Size;
    }
}

void ImGui_DX12_ShutdownBridge()
{
    g_TextureCache.clear();
    g_pFontTextureResource.Reset();
    g_pPipelineState.Reset();
    g_pRootSignature.Reset();
    g_pd3dSrvDescHeap.Reset();

    for (int i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
    {
        g_FrameResources[i].VertexBuffer.Reset();
        g_FrameResources[i].IndexBuffer.Reset();
        g_FrameResources[i].VertexBufferSize = 0;
        g_FrameResources[i].IndexBufferSize = 0;
    }

    g_pd3dDevice = nullptr;
    g_pd3dCommandQueue = nullptr;
    g_FontTextureCreated = false;
    g_ImGuiDX12Initialized = false;
}

bool ImGui_DX12_IsInitialized()
{
    return g_ImGuiDX12Initialized;
}

// --- Texture loading for UI images ---

static bool CreateDX12TextureFromPixels(unsigned char* pixels, int width, int height,
    ComPtr<ID3D12Resource>& outResource, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle, UINT& outSrvSlot)
{
    if (!g_pd3dDevice || !g_pd3dCommandQueue || !pixels || width <= 0 || height <= 0)
        return false;

    // Create texture resource
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = g_pd3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&outResource));
    if (FAILED(hr)) return false;

    // Create upload buffer
    UINT64 uploadSize = 0;
    g_pd3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width = uploadSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = g_pd3dDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(hr)) return false;

    // Create temp command list for upload
    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12Fence> fence;

    hr = g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    if (FAILED(hr)) return false;
    hr = g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    if (FAILED(hr)) return false;
    hr = g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) return false;

    // Copy pixel data to upload buffer
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
    UINT numRows;
    UINT64 rowSizeInBytes;
    g_pd3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, &layout, &numRows, &rowSizeInBytes, &uploadSize);

    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    hr = uploadBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr)) return false;

    for (UINT y = 0; y < numRows; y++)
    {
        memcpy((char*)mapped + layout.Offset + y * layout.Footprint.RowPitch,
               pixels + y * width * 4,
               width * 4);
    }
    uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = uploadBuffer.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint = layout;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = outResource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition to pixel shader resource
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = outResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    hr = cmdList->Close();
    if (FAILED(hr)) return false;

    // Execute and wait
    ID3D12CommandList* ppCmdLists[] = { cmdList.Get() };
    g_pd3dCommandQueue->ExecuteCommandLists(1, ppCmdLists);
    g_pd3dCommandQueue->Signal(fence.Get(), 1);

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    // Allocate SRV slot
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpu;
    if (!AllocSrvSlot(outSrvSlot, srvCpu, outGpuHandle))
        return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    g_pd3dDevice->CreateShaderResourceView(outResource.Get(), &srvDesc, srvCpu);

    return true;
}

void* ImGui_DX12_GetOrLoadTexture(int imageId, const char* filepath)
{
    if (!g_ImGuiDX12Initialized || !filepath || !filepath[0])
        return nullptr;

    // Check cache first
    auto it = g_TextureCache.find(imageId);
    if (it != g_TextureCache.end())
        return (void*)it->second.GpuHandle.ptr;

    // Load image from file using stb_image
    int width, height, channels;
    unsigned char* pixels = stbi_load(filepath, &width, &height, &channels, 4); // Force RGBA
    if (!pixels)
        return nullptr;

    DX12CachedTexture cached;
    cached.Width = width;
    cached.Height = height;

    bool ok = CreateDX12TextureFromPixels(pixels, width, height,
        cached.Resource, cached.GpuHandle, cached.SrvSlotIndex);
    stbi_image_free(pixels);

    if (!ok)
        return nullptr;

    void* result = (void*)cached.GpuHandle.ptr;
    g_TextureCache[imageId] = std::move(cached);
    return result;
}

void ImGui_DX12_SetImageSize(int imageId, int width, int height)
{
    auto it = g_TextureCache.find(imageId);
    if (it != g_TextureCache.end())
    {
        it->second.Width = width;
        it->second.Height = height;
    }
}

bool ImGui_DX12_GetImageSize(int imageId, int* outWidth, int* outHeight)
{
    auto it = g_TextureCache.find(imageId);
    if (it != g_TextureCache.end() && it->second.Width > 0)
    {
        if (outWidth) *outWidth = it->second.Width;
        if (outHeight) *outHeight = it->second.Height;
        return true;
    }
    return false;
}

bool ImGui_DX12_GetFileDimensions(const char* filepath, int* outWidth, int* outHeight)
{
    if (!filepath || !filepath[0])
        return false;
    int comp = 0;
    if (stbi_info(filepath, outWidth, outHeight, &comp))
        return true;
    return false;
}
