#pragma once
// Phase 5: ImGui DX12 Bridge
// Provides DX12 initialization and rendering for ImGui, using WickedEngine's DX12 device.

#include <d3d12.h>

// Initialize the ImGui DX12 backend using WickedEngine's DX12 device and command queue.
// Returns true on success.
bool ImGui_DX12_InitBridge();

// Render ImGui draw data using the given DX12 graphics command list.
// Call this during WickedEngine's Compose step (within an active render pass).
void ImGui_DX12_RenderBridge(ID3D12GraphicsCommandList* cmdList);

// Shutdown and cleanup the ImGui DX12 backend.
void ImGui_DX12_ShutdownBridge();

// Returns true if the DX12 bridge was successfully initialized.
bool ImGui_DX12_IsInitialized();

// Call ImGui_DX12_NewFrame() once per frame (called from ImGui_ImplDX11_NewFrame redirect).
void ImGui_DX12_NewFrame();

// Force rebuild the DX12 font texture (called after ChangeGGFont clears the font atlas).
void ImGui_DX12_RebuildFontTexture();

// Load an image file (PNG/JPG/BMP/TGA) and register it as an ImGui texture.
// Returns the ImTextureID (GPU descriptor handle) or nullptr on failure.
// The texture is cached — subsequent calls with the same imageId return the cached handle.
void* ImGui_DX12_GetOrLoadTexture(int imageId, const char* filepath);

// Set width/height for an image entry (used during DX12 lazy loading).
void ImGui_DX12_SetImageSize(int imageId, int width, int height);

// Retrieve cached width/height for an image loaded via DX12.
bool ImGui_DX12_GetImageSize(int imageId, int* outWidth, int* outHeight);

// Remove a cached DX12 texture by image ID (must be called when an image is deleted/replaced).
void ImGui_DX12_RemoveTexture(int imageId);

// Query image file dimensions from disk without loading pixel data.
bool ImGui_DX12_GetFileDimensions(const char* filepath, int* outWidth, int* outHeight);
