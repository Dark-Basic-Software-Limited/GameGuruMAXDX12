// Phase 5: ImGui DX12 Bridge Implementation
// Self-contained DX12 ImGui renderer compatible with ImGui 1.73.
// Does NOT use imgui_impl_dx12.cpp (which requires ImGui 1.92+).
// Based on the same rendering approach as the existing DX11 ImGuiHook_RenderCall_Direct.

#include "imgui.h"
#include "imgui_gg_dx12_bridge.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <unordered_map>
#include <cstdio>
#include <cstdarg>
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "d3d12")

// File logger for diagnosing DX12 texture upload issues.
// RUNAWAY LOG FIX 2026-07-29: this logged every ImGui draw call of every frame
// (~12K lines/sec at editor frame rates) through open-append-close I/O and had
// grown imgui_upload.log to 36 GB since February. Now OPT-IN: create an empty
// "imgui_upload_debug.txt" in the working directory to re-arm the diagnostics;
// without it every DX12Log call is a cheap early-out.
static UINT64 g_DX12LogFrameCount = 0;
static void DX12Log(const char* fmt, ...)
{
    static int s_enabled = -1;
    if (s_enabled < 0)
    {
        FILE* flag = nullptr;
        fopen_s(&flag, "imgui_upload_debug.txt", "rb");
        if (flag) { fclose(flag); s_enabled = 1; }
        else s_enabled = 0;
    }
    if (!s_enabled) return;
    FILE* f = nullptr;
    fopen_s(&f, "imgui_upload.log", "a");
    if (!f) return;
    // Timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fprintf(f, "\n");
    fclose(f);
}

// stb_image for loading PNG/JPG/BMP/TGA files (implementation in WickedEngine_Windows.lib)
#include "../Include/Utility/stb_image.h"

// DirectXTex for loading DDS files that stb_image cannot handle
#include "DirectXTex.h"

// WickedEngine headers for device access
#include "wiGraphicsDevice_DX12.h"
#include "wiGraphics.h"

using Microsoft::WRL::ComPtr;

// DX12 state for ImGui rendering
// GGMAX 2.49: dig-a-hole state shared with the DX11 backend and the game UI. bDigAHoleToHWND
// and rD3D11DigAHole are set by screens (Terrain Generator) that show the LIVE 3D scene through
// a hole in an otherwise opaque fullscreen window; bForceRenderEverywhere is toggled by draw
// callbacks 10/11 (defined in imgui_gg_dx11_part0.cpp, still compiled in this build).
// D3D11_RECT is `typedef RECT D3D11_RECT`, so the extern below is the same type.
extern bool bDigAHoleToHWND;
extern bool bForceRenderEverywhere;
extern RECT rD3D11DigAHole;

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
static bool g_DeviceRemoved = false;

// Deferred deletion queue — GPU resources must stay alive until all in-flight frames are done.
// Without this, deleting a texture that the GPU is still rendering causes DXGI_ERROR_DEVICE_REMOVED.
struct PendingTextureDelete
{
    DX12CachedTexture Texture;
    int FramesRemaining;  // count down each frame, free when 0
};
static std::vector<PendingTextureDelete> g_PendingDeletes;
static const int DEFERRED_DELETE_FRAMES = 4;  // wait 4 frames (safe margin over NUM_FRAMES_IN_FLIGHT=2)

// Private DIRECT queue for texture uploads (isolates from WickedEngine's graphics queue)
static ComPtr<ID3D12CommandQueue>        g_pUploadQueue;
static ComPtr<ID3D12CommandAllocator>    g_pUploadAllocator;
static ComPtr<ID3D12GraphicsCommandList> g_pUploadCommandList;
static ComPtr<ID3D12Fence>              g_pUploadFence;
static HANDLE                            g_hUploadFenceEvent = nullptr;
static UINT64                            g_UploadFenceValue = 0;

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

// Shaders (compiled at init time via D3DCompile)
static const char* g_VertexShaderHLSL =
    "cbuffer vertexBuffer : register(b0) { float4x4 ProjectionMatrix; };\n"
    "struct VS_INPUT { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };\n"
    "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
    "    PS_INPUT output;\n"
    "    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
    "    output.col = input.col;\n"
    "    output.uv = input.uv;\n"
    "    return output;\n"
    "}\n";

static const char* g_PixelShaderHLSL =
    "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };\n"
    "SamplerState sampler0 : register(s0);\n"
    "Texture2D texture0 : register(t0);\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
    "    return input.col * texture0.Sample(sampler0, input.uv);\n"
    "}\n";

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

// Execute pending upload commands on the private DIRECT queue and wait for completion.
// After this returns, the upload command list is reset and ready to record new commands.
static bool ExecuteUploadAndWait()
{
    DX12Log("UPLOAD: ExecuteUploadAndWait() - closing cmdlist, fenceVal=%llu", g_UploadFenceValue + 1);
    HRESULT hr = g_pUploadCommandList->Close();
    if (FAILED(hr)) { DX12Log("UPLOAD: Close FAILED hr=0x%08X", hr); return false; }

    ID3D12CommandList* ppCmdLists[] = { g_pUploadCommandList.Get() };
    g_pUploadQueue->ExecuteCommandLists(1, ppCmdLists);

    g_UploadFenceValue++;
    hr = g_pUploadQueue->Signal(g_pUploadFence.Get(), g_UploadFenceValue);
    if (FAILED(hr)) return false;

    if (g_pUploadFence->GetCompletedValue() < g_UploadFenceValue)
    {
        g_pUploadFence->SetEventOnCompletion(g_UploadFenceValue, g_hUploadFenceEvent);
        WaitForSingleObject(g_hUploadFenceEvent, INFINITE);
    }

    // Reset allocator and command list back to recording state for next use
    hr = g_pUploadAllocator->Reset();
    if (FAILED(hr)) return false;
    hr = g_pUploadCommandList->Reset(g_pUploadAllocator.Get(), nullptr);
    if (FAILED(hr)) return false;

    return true;
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

    g_pUploadCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition to pixel shader resource on our private DIRECT queue
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = g_pFontTextureResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    g_pUploadCommandList->ResourceBarrier(1, &barrier);

    if (!ExecuteUploadAndWait()) return false;

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
    // Compile shaders at runtime
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(g_VertexShaderHLSL, strlen(g_VertexShaderHLSL), nullptr, nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob);
    if (FAILED(hr)) { OutputDebugStringA("ImGui DX12: VS D3DCompile FAILED\n"); return false; }

    hr = D3DCompile(g_PixelShaderHLSL, strlen(g_PixelShaderHLSL), nullptr, nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob);
    if (FAILED(hr)) { OutputDebugStringA("ImGui DX12: PS D3DCompile FAILED\n"); return false; }

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
    psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

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
    DX12Log("INIT: ImGui_DX12_InitBridge() START");

    // Get the WickedEngine DX12 device
    auto* graphicsDevice = wi::graphics::GetDevice();
    if (!graphicsDevice)
    {
        DX12Log("INIT: FAILED - wi::graphics::GetDevice() returned null");
        return false;
    }

    // static_cast: WickedEngine compiled with RTTI disabled (/GR-), dynamic_cast crashes
    auto* dx12Device = static_cast<wi::graphics::GraphicsDevice_DX12*>(graphicsDevice);
    if (!dx12Device)
    {
        DX12Log("INIT: FAILED - static_cast to GraphicsDevice_DX12 returned null");
        return false;
    }

    g_pd3dDevice = dx12Device->GetDX12Device();
    g_pd3dCommandQueue = dx12Device->GetGraphicsCommandQueue();
    DX12Log("INIT: Device=%p, CommandQueue=%p", g_pd3dDevice, g_pd3dCommandQueue);
    if (!g_pd3dDevice || !g_pd3dCommandQueue)
    {
        DX12Log("INIT: FAILED - null device or command queue");
        return false;
    }

    // Create shader-visible SRV descriptor heap for ImGui
    DX12Log("INIT: Creating SRV descriptor heap (CBV_SRV_UAV, %u descriptors, SHADER_VISIBLE)", IMGUI_SRV_HEAP_SIZE);
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = IMGUI_SRV_HEAP_SIZE;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = g_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pd3dSrvDescHeap));
    if (FAILED(hr))
    {
        DX12Log("INIT: FAILED - CreateDescriptorHeap hr=0x%08X", hr);
        return false;
    }
    DX12Log("INIT: SRV descriptor heap created OK, handle=%p", g_pd3dSrvDescHeap.Get());

    g_SrvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    DX12Log("INIT: Descriptor increment size = %u", g_SrvDescriptorSize);
    memset(g_SrvSlotUsed, 0, sizeof(g_SrvSlotUsed));

    // Create private DIRECT queue for texture uploads (isolates from WickedEngine's graphics queue)
    DX12Log("INIT: Creating private upload queue...");
    {
        D3D12_COMMAND_QUEUE_DESC uploadQueueDesc = {};
        uploadQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        uploadQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        uploadQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        uploadQueueDesc.NodeMask = 0;
        hr = g_pd3dDevice->CreateCommandQueue(&uploadQueueDesc, IID_PPV_ARGS(&g_pUploadQueue));
        if (FAILED(hr)) { DX12Log("INIT: FAILED - CreateCommandQueue(DIRECT) hr=0x%08X", hr); return false; }

        hr = g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pUploadAllocator));
        if (FAILED(hr)) { DX12Log("INIT: FAILED - CreateCommandAllocator(DIRECT) hr=0x%08X", hr); return false; }

        hr = g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pUploadAllocator.Get(), nullptr, IID_PPV_ARGS(&g_pUploadCommandList));
        if (FAILED(hr)) { DX12Log("INIT: FAILED - CreateCommandList(DIRECT) hr=0x%08X", hr); return false; }

        hr = g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pUploadFence));
        if (FAILED(hr)) { DX12Log("INIT: FAILED - CreateFence hr=0x%08X", hr); return false; }

        g_hUploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!g_hUploadFenceEvent) { DX12Log("INIT: FAILED - CreateEvent for upload fence"); return false; }

        g_UploadFenceValue = 0;
    }
    DX12Log("INIT: Upload queue created OK");

    // Always create font texture first — builds the ImGui font atlas.
    // If this isn't done, ImGui crashes in SetCurrentFont accessing null ContainerAtlas.
    DX12Log("INIT: Creating font texture...");
    if (!CreateFontTexture())
    {
        DX12Log("INIT: FAILED - CreateFontTexture returned false");
        return false;
    }
    DX12Log("INIT: Font texture created OK, GPU handle ptr=0x%llX", g_FontSrvGpuHandle.ptr);

    // Create pipeline state (root signature, shaders, PSO)
    DX12Log("INIT: Creating pipeline state...");
    if (!CreatePipelineState())
    {
        DX12Log("INIT: FAILED - CreatePipelineState returned false");
        return false;
    }
    DX12Log("INIT: Pipeline state created OK, RootSig=%p, PSO=%p", g_pRootSignature.Get(), g_pPipelineState.Get());

    g_ImGuiDX12Initialized = true;
    DX12Log("INIT: ImGui_DX12_InitBridge() SUCCESS");
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

void ImGui_DX12_RebuildFontTexture()
{
    if (!g_ImGuiDX12Initialized) return;
    // Release old font texture resources and rebuild
    g_pFontTextureResource.Reset();
    g_FontTextureCreated = false;
    CreateFontTexture();
}

// Process deferred texture deletions — free GPU resources that are no longer in-flight.
static void ProcessPendingDeletes()
{
    for (auto it = g_PendingDeletes.begin(); it != g_PendingDeletes.end(); )
    {
        it->FramesRemaining--;
        if (it->FramesRemaining <= 0)
        {
            // Free the SRV descriptor slot so it can be reused
            if (it->Texture.SrvSlotIndex < IMGUI_SRV_HEAP_SIZE)
                g_SrvSlotUsed[it->Texture.SrvSlotIndex] = false;
            // ComPtr destructor releases the ID3D12Resource
            it = g_PendingDeletes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ImGui_DX12_RenderBridge(ID3D12GraphicsCommandList* cmdList)
{
    g_DX12LogFrameCount++;
    ProcessPendingDeletes();

    if (!g_ImGuiDX12Initialized || !cmdList || g_DeviceRemoved)
    {
        DX12Log("RENDER: early exit - initialized=%d cmdList=%p deviceRemoved=%d", g_ImGuiDX12Initialized, cmdList, g_DeviceRemoved);
        return;
    }

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->TotalVtxCount == 0)
        return; // normal empty frame, don't spam log

    // Avoid rendering when minimized
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        return;

    DX12Log("RENDER: START - vtx=%d idx=%d cmdLists=%d display=%.0fx%.0f",
        drawData->TotalVtxCount, drawData->TotalIdxCount, drawData->CmdListsCount,
        drawData->DisplaySize.x, drawData->DisplaySize.y);

    // Get frame resources (double-buffered)
    g_FrameIndex = (g_FrameIndex + 1) % NUM_FRAMES_IN_FLIGHT;
    FrameResources* fr = &g_FrameResources[g_FrameIndex];
    DX12Log("RENDER: frameIndex=%u", g_FrameIndex);

    // Create/grow vertex buffer
    if (!fr->VertexBuffer.Get() || fr->VertexBufferSize < drawData->TotalVtxCount)
    {
        fr->VertexBuffer.Reset();
        fr->VertexBufferSize = drawData->TotalVtxCount + 5000;
        DX12Log("RENDER: (re)creating VB, new size=%d verts (%llu bytes)",
            fr->VertexBufferSize, (UINT64)fr->VertexBufferSize * sizeof(ImDrawVert));

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
        HRESULT hr = g_pd3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->VertexBuffer));
        DX12Log("RENDER: VB create hr=0x%08X ptr=%p", hr, fr->VertexBuffer.Get());
    }

    // Create/grow index buffer
    if (!fr->IndexBuffer.Get() || fr->IndexBufferSize < drawData->TotalIdxCount)
    {
        fr->IndexBuffer.Reset();
        fr->IndexBufferSize = drawData->TotalIdxCount + 10000;
        DX12Log("RENDER: (re)creating IB, new size=%d indices (%llu bytes)",
            fr->IndexBufferSize, (UINT64)fr->IndexBufferSize * sizeof(ImDrawIdx));

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
        HRESULT hr = g_pd3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->IndexBuffer));
        DX12Log("RENDER: IB create hr=0x%08X ptr=%p", hr, fr->IndexBuffer.Get());
    }

    // Upload vertex/index data
    DX12Log("RENDER: mapping VB/IB for upload");
    void* vtxMapped = nullptr;
    void* idxMapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    HRESULT hrVB = fr->VertexBuffer->Map(0, &readRange, &vtxMapped);
    if (FAILED(hrVB))
    {
        HRESULT hrRemoved = g_pd3dDevice->GetDeviceRemovedReason();
        DX12Log("RENDER: FAILED - VB Map hr=0x%08X, DeviceRemovedReason=0x%08X", hrVB, hrRemoved);
        return;
    }
    HRESULT hrIB = fr->IndexBuffer->Map(0, &readRange, &idxMapped);
    if (FAILED(hrIB))
    {
        HRESULT hrRemoved = g_pd3dDevice->GetDeviceRemovedReason();
        DX12Log("RENDER: FAILED - IB Map hr=0x%08X, DeviceRemovedReason=0x%08X", hrIB, hrRemoved);
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
    DX12Log("RENDER: VB/IB upload done");

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
    DX12Log("RENDER: SetDescriptorHeaps - ImGui SRV heap=%p", g_pd3dSrvDescHeap.Get());
    ID3D12DescriptorHeap* heaps[] = { g_pd3dSrvDescHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Set pipeline state
    DX12Log("RENDER: SetPipelineState=%p, SetRootSignature=%p", g_pPipelineState.Get(), g_pRootSignature.Get());
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

    DX12Log("RENDER: VB GPU addr=0x%llX, IB GPU addr=0x%llX, viewport=%.0fx%.0f",
        vbv.BufferLocation, ibv.BufferLocation, vp.Width, vp.Height);

    // Render draw commands
    int globalIdxOffset = 0;
    int globalVtxOffset = 0;
    int totalDrawCalls = 0;
    ImVec2 clipOff = drawData->DisplayPos;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* imCmdList = drawData->CmdLists[n];
        for (int cmd_i = 0; cmd_i < imCmdList->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &imCmdList->CmdBuffer[cmd_i];

            if (pcmd->UserCallback != nullptr)
            {
                // GGMAX 2.49: callbacks 10/11 are the force-render toggles the DX11 backend uses
                // to let chosen draws (e.g. the Terrain Generator's yellow editable-area overlay)
                // punch through the dig-a-hole scissors below. They were being skipped with the
                // other callbacks, which held bForceRenderEverywhere permanently false.
                if (pcmd->UserCallback == (ImDrawCallback)10) { bForceRenderEverywhere = true;  continue; }
                if (pcmd->UserCallback == (ImDrawCallback)11) { bForceRenderEverywhere = false; continue; }

                // Phase 5: Custom shader callbacks (blur, nowhite, etc.) are not yet ported to DX12
                // TODO: Port custom pixel shader variants to DX12
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    DX12Log("RENDER: ResetRenderState callback at list=%d cmd=%d", n, cmd_i);
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

            // Bind texture
            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};
            textureHandle.ptr = (UINT64)pcmd->TextureId;

            // Validate texture handle is within our descriptor heap range
            D3D12_GPU_DESCRIPTOR_HANDLE heapStart = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
            UINT64 heapEnd = heapStart.ptr + (UINT64)IMGUI_SRV_HEAP_SIZE * g_SrvDescriptorSize;
            if (textureHandle.ptr < heapStart.ptr || textureHandle.ptr >= heapEnd)
            {
                DX12Log("RENDER: *** BAD TEXTURE HANDLE *** list=%d cmd=%d texID=0x%llX heapRange=[0x%llX..0x%llX) - SKIPPING",
                    n, cmd_i, textureHandle.ptr, heapStart.ptr, heapEnd);
                continue;
            }

            cmdList->SetGraphicsRootDescriptorTable(1, textureHandle);

            // GGMAX 2.49: dig-a-hole, ported from the DX11 backend (imgui_gg_dx11_part0.cpp:715-767).
            // The Terrain Generator "preview" is the LIVE 3D scene: the generator window is a
            // fullscreen OPAQUE ImGui window and the engine scene is composed underneath it —
            // the preview only exists because the UI draws around a HOLE (rD3D11DigAHole) in
            // four scissor rects (above / left-of / below / right-of the hole). This backend
            // used one ClipRect scissor for everything, so the opaque window covered the whole
            // viewport and the generator showed no terrain at all on DX12.
            // Faithful to DX11: while the hole is active, draws use the four display-sized rects
            // INSTEAD of their own ClipRect (panel widgets sit inside the side rects anyway),
            // and draws between callbacks 10/11 bypass the hole entirely.
            if (bDigAHoleToHWND && !bForceRenderEverywhere)
            {
                D3D12_RECT rAll;
                rAll.left = (LONG)(drawData->DisplayPos.x - clipOff.x);
                rAll.top = (LONG)(drawData->DisplayPos.y - clipOff.y);
                rAll.right = (LONG)(drawData->DisplayPos.x + drawData->DisplaySize.x);
                rAll.bottom = (LONG)(drawData->DisplayPos.y + drawData->DisplaySize.y);
                D3D12_RECT hr[4];
                for (int i = 0; i < 4; i++) hr[i] = rAll;
                hr[0].bottom = rD3D11DigAHole.top;     // strip above the hole
                hr[1].top = rD3D11DigAHole.top;        // strip left of the hole
                hr[1].right = rD3D11DigAHole.left;
                hr[1].bottom = rD3D11DigAHole.bottom;
                hr[2].top = rD3D11DigAHole.bottom;     // strip below the hole
                hr[3].top = rD3D11DigAHole.top;        // strip right of the hole
                hr[3].left = rD3D11DigAHole.right;
                hr[3].bottom = rD3D11DigAHole.bottom;
                for (int i = 0; i < 4; i++)
                {
                    if (hr[i].right <= hr[i].left || hr[i].bottom <= hr[i].top)
                        continue;
                    cmdList->RSSetScissorRects(1, &hr[i]);
                    cmdList->DrawIndexedInstanced(pcmd->ElemCount, 1,
                        pcmd->IdxOffset + globalIdxOffset,
                        pcmd->VtxOffset + globalVtxOffset, 0);
                    totalDrawCalls++;
                }
                continue;
            }

            // Apply scissor/clipping rectangle
            D3D12_RECT r;
            r.left = (LONG)(pcmd->ClipRect.x - clipOff.x);
            r.top = (LONG)(pcmd->ClipRect.y - clipOff.y);
            r.right = (LONG)(pcmd->ClipRect.z - clipOff.x);
            r.bottom = (LONG)(pcmd->ClipRect.w - clipOff.y);
            if (r.right <= r.left || r.bottom <= r.top)
                continue;
            cmdList->RSSetScissorRects(1, &r);

            // Log first few draw calls per frame, then just the count
            if (totalDrawCalls < 3)
            {
                DX12Log("RENDER: DrawIndexed #%d: elems=%u idxOff=%u vtxOff=%d tex=0x%llX scissor=(%ld,%ld,%ld,%ld)",
                    totalDrawCalls, pcmd->ElemCount,
                    pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset,
                    textureHandle.ptr, r.left, r.top, r.right, r.bottom);
            }

            // Draw
            cmdList->DrawIndexedInstanced(pcmd->ElemCount, 1,
                pcmd->IdxOffset + globalIdxOffset,
                pcmd->VtxOffset + globalVtxOffset, 0);
            totalDrawCalls++;
        }
        globalIdxOffset += imCmdList->IdxBuffer.Size;
        globalVtxOffset += imCmdList->VtxBuffer.Size;
    }

    DX12Log("RENDER: DONE - %d draw calls issued", totalDrawCalls);
}

void ImGui_DX12_ShutdownBridge()
{
    DX12Log("SHUTDOWN: ImGui_DX12_ShutdownBridge() - clean exit after %llu frames", g_DX12LogFrameCount);
    g_PendingDeletes.clear();
    g_TextureCache.clear();
    g_pFontTextureResource.Reset();
    g_pPipelineState.Reset();
    g_pRootSignature.Reset();
    g_pd3dSrvDescHeap.Reset();

    // Clean up private upload queue resources (reverse creation order)
    g_pUploadCommandList.Reset();
    g_pUploadAllocator.Reset();
    g_pUploadFence.Reset();
    g_pUploadQueue.Reset();
    if (g_hUploadFenceEvent) { CloseHandle(g_hUploadFenceEvent); g_hUploadFenceEvent = nullptr; }
    g_UploadFenceValue = 0;

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
    g_DeviceRemoved = false;
}

bool ImGui_DX12_IsInitialized()
{
    return g_ImGuiDX12Initialized;
}

// --- Texture loading for UI images ---

static bool CreateDX12TextureFromPixels(unsigned char* pixels, int width, int height,
    ComPtr<ID3D12Resource>& outResource, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle, UINT& outSrvSlot)
{
    DX12Log("UPLOAD: CreateDX12TextureFromPixels(%dx%d) ENTER", width, height);
    if (!g_pd3dDevice || !g_pd3dCommandQueue || !pixels || width <= 0 || height <= 0)
    {
        DX12Log("UPLOAD: CreateDX12TextureFromPixels - FAILED null check");
        return false;
    }

    // Check if device has been removed — no point continuing
    if (g_DeviceRemoved)
    {
        DX12Log("UPLOAD: CreateDX12TextureFromPixels - SKIPPED (device already removed)");
        return false;
    }
    HRESULT hrCheck = g_pd3dDevice->GetDeviceRemovedReason();
    if (FAILED(hrCheck))
    {
        DX12Log("UPLOAD: CreateDX12TextureFromPixels - DEVICE REMOVED hr=0x%08X", hrCheck);
        g_DeviceRemoved = true;
        return false;
    }

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

    g_pUploadCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition to pixel shader resource on our private DIRECT queue
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = outResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    g_pUploadCommandList->ResourceBarrier(1, &barrier);

    DX12Log("UPLOAD: CreateDX12TextureFromPixels - executing on private DIRECT queue...");
    if (!ExecuteUploadAndWait())
    {
        DX12Log("UPLOAD: CreateDX12TextureFromPixels - ExecuteUploadAndWait FAILED, device removed");
        g_DeviceRemoved = true;
        return false;
    }

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

    DX12Log("UPLOAD: CreateDX12TextureFromPixels(%dx%d) SUCCESS - srvSlot=%u", width, height, outSrvSlot);
    return true;
}

// Load a DDS file into RGBA pixel data using DirectXTex.
// If the file has a .jpg extension and contains DDS data, rename it to .dds first.
// Returns malloc'd pixel buffer (caller must free()), or nullptr on failure.
static unsigned char* TryLoadDDSWithRename(const char* filepath, int* outWidth, int* outHeight)
{
    // Check if file starts with DDS magic bytes
    FILE* f = nullptr;
    fopen_s(&f, filepath, "rb");
    if (!f)
        return nullptr;
    unsigned char magic[4] = {};
    size_t rd = fread(magic, 1, 4, f);
    fclose(f);
    if (rd < 4 || magic[0] != 'D' || magic[1] != 'D' || magic[2] != 'S' || magic[3] != ' ')
        return nullptr;

    // It's a DDS file - rename .jpg to .dds for cleanup
    const char* loadPath = filepath;
    char ddsPath[512] = {};
    int pathLen = (int)strlen(filepath);
    if (pathLen > 4 && _stricmp(filepath + pathLen - 4, ".jpg") == 0)
    {
        strcpy_s(ddsPath, filepath);
        strcpy_s(ddsPath + pathLen - 4, 5, ".dds");
        if (rename(filepath, ddsPath) == 0)
            loadPath = ddsPath;
        // If rename fails (read-only etc), load from original path
    }

    // Convert path to wide string for DirectXTex
    wchar_t wPath[512];
    MultiByteToWideChar(CP_ACP, 0, loadPath, -1, wPath, 512);

    // Load DDS via DirectXTex
    DirectX::ScratchImage image;
    DirectX::TexMetadata meta;
    HRESULT hr = DirectX::LoadFromDDSFile(wPath, DirectX::DDS_FLAGS_NONE, &meta, image);
    if (FAILED(hr))
        return nullptr;

    // Decompress if block-compressed format
    if (DirectX::IsCompressed(meta.format))
    {
        DirectX::ScratchImage decompressed;
        hr = DirectX::Decompress(*image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
        if (FAILED(hr))
            return nullptr;
        image = std::move(decompressed);
        meta = image.GetMetadata();
    }

    // Convert to RGBA8 if not already
    if (meta.format != DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        DirectX::ScratchImage converted;
        hr = DirectX::Convert(*image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM,
            DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
        if (FAILED(hr))
            return nullptr;
        image = std::move(converted);
    }

    const DirectX::Image* img = image.GetImage(0, 0, 0);
    if (!img || img->width == 0 || img->height == 0)
        return nullptr;

    *outWidth = (int)img->width;
    *outHeight = (int)img->height;

    // Copy pixels to malloc'd buffer (row-by-row in case of pitch padding)
    size_t rowBytes = img->width * 4;
    unsigned char* pixels = (unsigned char*)malloc(rowBytes * img->height);
    if (!pixels)
        return nullptr;
    for (size_t y = 0; y < img->height; y++)
        memcpy(pixels + y * rowBytes, img->pixels + y * img->rowPitch, rowBytes);

    return pixels;
}

void* ImGui_DX12_GetOrLoadTexture(int imageId, const char* filepath)
{
    if (!g_ImGuiDX12Initialized || !filepath || !filepath[0] || g_DeviceRemoved)
        return nullptr;

    // Check cache first
    auto it = g_TextureCache.find(imageId);
    if (it != g_TextureCache.end())
        return (void*)it->second.GpuHandle.ptr;

    DX12Log("UPLOAD: GetOrLoadTexture(id=%d) LOADING: %s", imageId, filepath);

    // Load image from file using stb_image
    int width, height, channels;
    unsigned char* pixels = stbi_load(filepath, &width, &height, &channels, 4); // Force RGBA
    bool pixelsFromDDS = false;

    if (!pixels)
    {
        // stbi_load failed - check if file is actually DDS format (e.g. .jpg containing DDS data)
        // If so, rename .jpg to .dds for cleanup and load via DirectXTex
        pixels = TryLoadDDSWithRename(filepath, &width, &height);
        if (!pixels)
            return nullptr;
        pixelsFromDDS = true;
    }

    DX12CachedTexture cached;
    cached.Width = width;
    cached.Height = height;

    bool ok = CreateDX12TextureFromPixels(pixels, width, height,
        cached.Resource, cached.GpuHandle, cached.SrvSlotIndex);

    if (pixelsFromDDS)
        free(pixels);
    else
        stbi_image_free(pixels);

    if (!ok)
    {
        DX12Log("UPLOAD: GetOrLoadTexture(id=%d) FAILED - CreateDX12TextureFromPixels returned false", imageId);
        return nullptr;
    }

    void* result = (void*)cached.GpuHandle.ptr;
    g_TextureCache[imageId] = std::move(cached);
    DX12Log("UPLOAD: GetOrLoadTexture(id=%d) OK - %dx%d cached, total=%d", imageId, width, height, (int)g_TextureCache.size());
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

void ImGui_DX12_RemoveTexture(int imageId)
{
    auto it = g_TextureCache.find(imageId);
    if (it != g_TextureCache.end())
    {
        // Defer deletion — the GPU may still be rendering with this texture from a previous frame.
        // Immediate release of the ID3D12Resource causes DXGI_ERROR_DEVICE_REMOVED.
        PendingTextureDelete pd;
        pd.Texture = std::move(it->second);
        pd.FramesRemaining = DEFERRED_DELETE_FRAMES;
        g_PendingDeletes.push_back(std::move(pd));
        g_TextureCache.erase(it);
    }
}

bool ImGui_DX12_GetFileDimensions(const char* filepath, int* outWidth, int* outHeight)
{
    if (!filepath || !filepath[0])
        return false;
    int comp = 0;
    if (stbi_info(filepath, outWidth, outHeight, &comp))
        return true;
    // stbi_info failed - check if it's a DDS file and read dimensions from header
    FILE* f = nullptr;
    fopen_s(&f, filepath, "rb");
    if (f)
    {
        unsigned char header[20] = {};
        size_t rd = fread(header, 1, 20, f);
        fclose(f);
        if (rd == 20 && header[0] == 'D' && header[1] == 'D' && header[2] == 'S' && header[3] == ' ')
        {
            // DDS header: bytes 12-15 = height, bytes 16-19 = width (little-endian uint32)
            *outHeight = (int)(header[12] | (header[13] << 8) | (header[14] << 16) | (header[15] << 24));
            *outWidth  = (int)(header[16] | (header[17] << 8) | (header[18] << 16) | (header[19] << 24));
            if (*outWidth > 0 && *outHeight > 0)
                return true;
        }
    }
    return false;
}
