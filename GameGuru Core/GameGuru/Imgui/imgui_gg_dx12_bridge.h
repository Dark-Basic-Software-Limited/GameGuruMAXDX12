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
