
//PE: Display all imgui DX11 errors.
//#define DEBUGDISPLAY

//PE: dear imgui: Renderer for DirectX11
// PE: Changed to be used in GameGuru.
// PE: Added additional dialog here.

// Includes 
#include "stdafx.h"
#include "commdlg.h"

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "imgui_gg_dx11.h"

#include "stdio.h"
#include <direct.h>

// DirectX
#include <stdio.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

#include "CFileC.h"
#include <Shlobj.h>

#include <algorithm>
#include <string>

#include "..\..\Guru-WickedMAX\wickedcalls.h"

#include "M-RPG.h"
#include "M-Workshop.h"
#include <deque>

char launchLoadOnStartup[260] = "\0";

// Names and types of 'readouts' - each one represents a variable somewhere in the engine, to be displayed in HUD
std::vector<std::string> readoutTitles;
std::vector<STORYBOARD_WIDGET_> readoutWidgetTypes;
std::vector<ReadoutLayers> readoutLayers;
std::vector<ReadoutTypes> readoutTypes;
std::vector<std::function<void()>> readoutCallbacks;

// Legacy WhiteList Globals
bool g_bCreateLegacyWhiteList = true;
std::vector<LPSTR> g_pLegacyWhiteList;

bool bImGuiFrameState = false;
bool bImGuiGotFocus = false;
bool bImGuiRenderTargetFocus = false;
bool bImGuiRenderTargetWantKeyboard = false;
bool bImGuiReadyToRender = false;
bool bImGuiInTestGame = false;
bool bBlockImGuiUntilNewFrame = false;
bool bImGuiRenderWithNoCustomTextures = false;
bool bBlockImGuiUntilFurtherNotice = false;
bool bImGuiInitDone = false;
int ImGuiStatusBar_Size = 0;

bool bPreviewWPE = false;
uint32_t PreviewWPERoot = 0;
float fPreviewYOffset = 0;

preferences pref;
bool g_bEnableAutoFlattenSystem = true;

ImVec2 vStartResolution = { 1280 , 800 }; // { 1024, 768 }; //1280x800
ImVec2 OldrenderTargetSize = { 0,0 };
ImVec2 OldrenderTargetPos = { 0,0 };
ImVec2 renderTargetAreaPos = { 0,0 };
ImVec2 renderTargetAreaSize = { 0,0 };
bool bCenterRenderView = false;

ImVec2 fImGuiScissorTopLeft = { 0, 0 };
ImVec2 fImGuiScissorBottomRight = { 0, 0 };

// DirectX data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGIFactory*            g_pFactory = NULL;
static ID3D11Buffer*            g_pVB = NULL;
static ID3D11Buffer*            g_pIB = NULL;
static ID3D10Blob*              g_pVertexShaderBlob = NULL;
static ID3D11VertexShader*      g_pVertexShader = NULL;
static ID3D11InputLayout*       g_pInputLayout = NULL;
static ID3D11Buffer*            g_pVertexConstantBuffer = NULL;
static ID3D10Blob*              g_pPixelShaderBlob = NULL;
static ID3D11PixelShader*       g_pPixelShader = NULL;
static ID3D10Blob*              g_pPixelShaderBlobBlur = NULL;
static ID3D11PixelShader*       g_pPixelShaderBlur = NULL;

static ID3D10Blob*              g_pPixelShaderNoWhiteBlob = NULL;
static ID3D11PixelShader*       g_pPixelShaderNoWhite = NULL;
static ID3D10Blob*              g_pPixelShaderNoAlphaBlob = NULL;
static ID3D11PixelShader*       g_pPixelShaderNoAlpha = NULL;

static ID3D10Blob*              g_ppixelShaderBoost25Blob = NULL;
static ID3D11PixelShader*       g_ppixelShaderBoost25 = NULL;

static ID3D11SamplerState*      g_pFontSampler = NULL;
static ID3D11ShaderResourceView*g_pFontTextureView = NULL;
static ID3D11RasterizerState*   g_pRasterizerState = NULL;
static ID3D11BlendState*        g_pBlendState = NULL;
static ID3D11DepthStencilState* g_pDepthStencilState = NULL;
static int                      g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

struct VERTEX_CONSTANT_BUFFER
{
    float   mvp[4][4];
};

std::vector<SliderData> g_SliderData; // For storing max values for sliders.
std::vector<std::vector<std::string>> luadropdownlabels; // Storage for dropdown labels extracted from lua script.
std::vector<std::string> g_DLuaVariableNames;

// Forward Declarations
static void ImGui_ImplDX11_InitPlatformInterface();
static void ImGui_ImplDX11_ShutdownPlatformInterface();

static void ImGui_ImplDX11_SetupRenderState(ImDrawData* draw_data, ID3D11DeviceContext* ctx, bool nowhite = false )
{
    // Setup viewport
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D11_VIEWPORT));
    vp.Width = draw_data->DisplaySize.x;
    vp.Height = draw_data->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    ctx->RSSetViewports(1, &vp);

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    ctx->IASetInputLayout(g_pInputLayout);
    ctx->IASetVertexBuffers(0, 1, &g_pVB, &stride, &offset);
    ctx->IASetIndexBuffer(g_pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_pVertexShader, NULL, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_pVertexConstantBuffer);
	if(nowhite)
		ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
	else
		ctx->PSSetShader(g_pPixelShader, NULL, 0);
	
    ctx->PSSetSamplers(0, 1, &g_pFontSampler);
    ctx->GSSetShader(NULL, NULL, 0);
    ctx->HSSetShader(NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..
    ctx->DSSetShader(NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..
    ctx->CSSetShader(NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..

    // Setup blend state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendState(g_pBlendState, blend_factor, 0xffffffff);
    ctx->OMSetDepthStencilState(g_pDepthStencilState, 0);
    ctx->RSSetState(g_pRasterizerState);
}

extern bool bRenderTabTab;
extern bool bRenderNextFrame;

bool bForceRenderEverywhere = false;

std::vector<ID3D11ShaderResourceView*> lpBadTexture;
std::deque<std::vector<ID3D11ShaderResourceView*>> badTextureFrames;
const size_t MAX_FRAMES_TO_KEEP = 3;

void ImGuiHook_RenderCall_Direct(void* ctxptr, void* d3dptr)
{
	// TODO Phase 5: migrate to DX12
	// Phase 4: Guard against NULL device pointers during DX12 migration
	if (!ctxptr || !d3dptr)
		return;

	if (bBlockImGuiUntilNewFrame || bBlockImGuiUntilFurtherNotice)
		return;
	if (bRenderNextFrame)
	{
		//bRenderNextFrame = false;
	}
	else
	{
		if (bImGuiInTestGame && !bRenderTabTab)
			return;
		//	if (bRenderTabTab)
		//		bRenderTabTab = false;
	}

	// goes through the same sequence as 'ImGui_ImplDX11_RenderDrawData' but Wicked Friendly..
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)ctxptr;
	ID3D11Device* pd3dDevice = (ID3D11Device*) d3dptr;

	ImDrawData* draw_data = ImGui::GetDrawData();

	// Avoid rendering if no data
	if (draw_data == NULL)
		return;

	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	// Create and grow vertex/index buffers if needed
	if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount)
	{
		if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
		g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = g_VertexBufferSize * sizeof(ImDrawVert);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (pd3dDevice->CreateBuffer(&desc, NULL, &g_pVB) < 0)
			return;
	}
	if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount)
	{
		if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
		g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = g_IndexBufferSize * sizeof(ImDrawIdx);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (pd3dDevice->CreateBuffer(&desc, NULL, &g_pIB) < 0)
			return;
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	if (ctx->Map(g_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
		return;
	if (ctx->Map(g_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
		return;
	ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
	ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	ctx->Unmap(g_pVB, 0);
	ctx->Unmap(g_pIB, 0);

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		if (ctx->Map(g_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
			return;
		VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
		ctx->Unmap(g_pVertexConstantBuffer, 0);
	}

	// Setup desired DX state
	ImGui_ImplDX11_SetupRenderState(draw_data, ctx);

	badTextureFrames.push_back(std::move(lpBadTexture));

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_idx_offset = 0;
	int global_vtx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			
			if (!pcmd->TextureId)
				continue;

			bool bBadTextureAll = false;
			for (const auto& frameTextures : badTextureFrames)
			{
				//PE: Check all frames for bad texture_srv.
				for (int i = 0; i < frameTextures.size(); i++)
				{
					ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;
					if (texture_srv == frameTextures[i])
					{
						bBadTextureAll = true;
						break;
					}
				}
			}
			if (bBadTextureAll)
				continue;

			if (pcmd->UserCallback == (ImDrawCallback)10)
			{
				bForceRenderEverywhere = true;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)11)
			{
				bForceRenderEverywhere = false;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)1)
			{
				//PE: Change shaders.
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)2)
			{
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShader, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)3)
			{
				//NoAlpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShaderNoAlpha, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)4)
			{
				//Alpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShader, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)5)
			{
				//boost colors 25 percent.
				ctx->PSSetShader(g_ppixelShaderBoost25, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)6)
			{
				//Blur
				ctx->PSSetShader(g_pPixelShaderBlur, NULL, 0);
			}
			else if (pcmd->UserCallback != NULL)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				// for now we ignore shader changes mid-processing, put back when we see something!
				//if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
				//    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
				//else
				//    pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Apply scissor/clipping rectangle
				const D3D11_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
				ctx->RSSetScissorRects(1, &r);

				// Bind texture, Draw

				//ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iImageID);
				ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;

				//Locate bad textures.
				bool bBadTexture = false;
				//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
				if (lpBadTexture.size() > 0)
				{
					for (int i = 0; i < lpBadTexture.size(); i++)
					{
						if (texture_srv == lpBadTexture[i])
						{
							bBadTexture = true;
							break;
						}
					}
				}
				if (!bBadTexture)
				{
					ctx->PSSetShaderResources(0, 1, &texture_srv);
					ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
					ID3D11ShaderResourceView *const pSRV[1] = { NULL };
					ctx->PSSetShaderResources(0, 1, pSRV);
				}
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

	lpBadTexture.clear();

	if (badTextureFrames.size() > MAX_FRAMES_TO_KEEP) {
		badTextureFrames.pop_front();
	}

}

// New Render function which is now called from Wicked Engine LIB
extern bool bDigAHoleToHWND;
D3D11_RECT rD3D11DigAHole = { 300,300,600,600 };


bool IntersectsWith(D3D11_RECT rect , D3D11_RECT compare)
{

	if (rect.left <= compare.right && rect.right >= compare.left && rect.top <= compare.bottom)
	{
		return rect.bottom >= compare.top;
	}

	return false;
}

void ImGuiHook_RenderCall(void* ctxptr)
{
	// TODO Phase 5: migrate to DX12
	// Phase 4: Guard against NULL device during DX12 migration
	if (!g_pd3dDevice || !g_pd3dDeviceContext)
		return;

	extern bool g_bNoGGUntilGameGuruMainCalled;
	if (g_bNoGGUntilGameGuruMainCalled==false)
		return;
	extern bool g_bNo2DRender;
	if (g_bNo2DRender)
		return;
	bool bSpecialNoCustomTextureRender = false;
	if (bBlockImGuiUntilNewFrame || bBlockImGuiUntilFurtherNotice)
	{
		if (bBlockImGuiUntilNewFrame && bImGuiRenderWithNoCustomTextures)
		{
			bSpecialNoCustomTextureRender = true;
		}
		else
		{
			return;
		}
	}

	if (bRenderNextFrame)
	{
		//PE: If we have zero sprites and if (bImGuiInTestGame && !bRenderTabTab) , we need to return.
		
		//bRenderNextFrame = false;
	} 
	else
	{
		if (bImGuiInTestGame && !bRenderTabTab)
			return;
		//	if (bRenderTabTab)
		//		bRenderTabTab = false;
	}

	// goes through the same sequence as 'ImGui_ImplDX11_RenderDrawData' but Wicked Friendly..
    ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)ctxptr;

	ImDrawData* draw_data = ImGui::GetDrawData();

	// Avoid rendering if no data
	if (draw_data == NULL)
		return;

    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Create and grow vertex/index buffers if needed
    if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
        g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = g_VertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pVB) < 0)
            return;
    }
    if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
        g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = g_IndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pIB) < 0)
            return;
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
    if (ctx->Map(g_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
        return;
    if (ctx->Map(g_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
        return;
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    ctx->Unmap(g_pVB, 0);
    ctx->Unmap(g_pIB, 0);

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (ctx->Map(g_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
            return;
        VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
        ctx->Unmap(g_pVertexConstantBuffer, 0);
    }

    // Setup desired DX state
    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);

	/*
	if (bDigAHoleToHWND)
	{
		//PE: Cant use it without a geometry shaders.
		//PE: Need 4 viewports.
		D3D11_VIEWPORT vp[4];
		for (int i = 0; i < 4; i++)
		{
			memset(&vp[i], 0, sizeof(D3D11_VIEWPORT));
			vp[i].Width = draw_data->DisplaySize.x;
			vp[i].Height = draw_data->DisplaySize.y;
			vp[i].MinDepth = 0.0f;
			vp[i].MaxDepth = 1.0f;
			vp[i].TopLeftX = vp[i].TopLeftY = 0;
		}
		ctx->RSSetViewports(4, &vp[0]);
	}
	*/

	ImGuiContext& g = *GImGui;

	badTextureFrames.push_back(std::move(lpBadTexture));

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

			if (!pcmd->TextureId)
				continue;

			bool bBadTextureAll = false;
			for (const auto& frameTextures : badTextureFrames)
			{
				//PE: Check all frames for bad texture_srv.
				for (int i = 0; i < frameTextures.size(); i++)
				{
					ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;
					if (texture_srv == frameTextures[i])
					{
						bBadTextureAll = true;
						break;
					}
				}
			}
			if (bBadTextureAll)
				continue;

			if (pcmd->UserCallback == (ImDrawCallback)10)
			{
				bForceRenderEverywhere = true;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)11)
			{
				bForceRenderEverywhere = false;
			}
			else if (pcmd->UserCallback == (ImDrawCallback) 1)
			{
				//PE: Change shaders.
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)2)
			{
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShader, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)3)
			{
				//NoAlpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShaderNoAlpha, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)4)
			{
				//Alpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				ctx->PSSetShader(g_pPixelShader, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)5)
			{
				//boost colors 25 percent.
				ctx->PSSetShader(g_ppixelShaderBoost25, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)6)
			{
				//Blur
				ctx->PSSetShader(g_pPixelShaderBlur, NULL, 0);
			}
			else if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				// for now we ignore shader changes mid-processing, put back when we see something!
                //if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                //    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
                //else
                //    pcmd->UserCallback(cmd_list, pcmd);
            }
			else if(bSpecialNoCustomTextureRender)
			{
				//PE: Only render eveything but no special textures.
				if (pcmd->TextureId == g.Font->ContainerAtlas->TexID)
				{
					// Apply scissor/clipping rectangle
					const D3D11_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
					ctx->RSSetScissorRects(1, &r);

					// Bind texture, Draw
					ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;

					bool bBadTexture = false;
					//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
					if (lpBadTexture.size() > 0)
					{
						for (int i = 0; i < lpBadTexture.size(); i++)
						{
							if (texture_srv == lpBadTexture[i])
							{
								bBadTexture = true;
								break;
							}
						}
					}

					if (!bBadTexture)
					{
						//PE: Strange got one today, just protect it for now.
						try
						{
							// PREBEN, this is the line that crashes when exiting a blank test game
							ctx->PSSetShaderResources(0, 1, &texture_srv);
							// PREBEN, this is the line that crashes when exiting a blank test game
							ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
							ID3D11ShaderResourceView *const pSRV[1] = { NULL };
							ctx->PSSetShaderResources(0, 1, pSRV);
						}
						catch (...)
						{
							ID3D11ShaderResourceView *const pSRV[1] = { NULL };
							ctx->PSSetShaderResources(0, 1, pSRV);
							return;
						}
					}
				}
			}
            else
            {
                // Apply scissor/clipping rectangle

				if (bDigAHoleToHWND && !bForceRenderEverywhere)
				{
					const D3D11_RECT rAll = { (LONG)draw_data->DisplayPos.x - clip_off.x, draw_data->DisplayPos.y - clip_off.y, (LONG)draw_data->DisplayPos.x + draw_data->DisplaySize.x, (LONG)draw_data->DisplayPos.y + draw_data->DisplaySize.y };
					D3D11_RECT r[4];
					for (int i = 0; i < 4; i++)
					{
						r[i] = rAll;
					}
					r[0].bottom = rD3D11DigAHole.top; //Rect 1
					r[1].top = rD3D11DigAHole.top; //Rect 2
					r[1].right = rD3D11DigAHole.left; //Rect 2
					r[2].top = rD3D11DigAHole.bottom; //Rect 3
					r[2].left = rD3D11DigAHole.left; //Rect 3
					r[3].top = rD3D11DigAHole.top; //Rect 4
					r[3].left = rD3D11DigAHole.right; //Rect 4
					r[3].bottom = rD3D11DigAHole.bottom; //Rect 4
					ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;

					bool bBadTexture = false;
					//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
					if (lpBadTexture.size() > 0)
					{
						for (int i = 0; i < lpBadTexture.size(); i++)
						{
							if (texture_srv == lpBadTexture[i])
							{
								bBadTexture = true;
								break;
							}
						}
					}
					if (!bBadTexture)
					{
						for (int i = 0; i < 3; i++)
						{
							ctx->RSSetScissorRects(1, &r[i]);
							try
							{
								ctx->PSSetShaderResources(0, 1, &texture_srv);
								ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
								ID3D11ShaderResourceView *const pSRV[1] = { NULL };
								ctx->PSSetShaderResources(0, 1, pSRV);
							}
							catch (...)
							{
								ID3D11ShaderResourceView *const pSRV[1] = { NULL };
								ctx->PSSetShaderResources(0, 1, pSRV);
								return;
							}
						}
					}
					ctx->RSSetScissorRects(1, &r[3]);
				}
				else
				{
					const D3D11_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
					ctx->RSSetScissorRects(1, &r);
				}

                // Bind texture, Draw
	            ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;

				bool bBadTexture = false;
				//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
				if (lpBadTexture.size() > 0)
				{
					for (int i = 0; i < lpBadTexture.size(); i++)
					{
						if (texture_srv == lpBadTexture[i])
						{
							bBadTexture = true;
							break;
						}
					}
				}
				if (!bBadTexture)
				{

					//PE: Strange got one today, just protect it for now.
					try
					{
						// PREBEN, this is the line that crashes when exiting a blank test game
						ctx->PSSetShaderResources(0, 1, &texture_srv);
						// PREBEN, this is the line that crashes when exiting a blank test game
						ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
						ID3D11ShaderResourceView *const pSRV[1] = { NULL };
						ctx->PSSetShaderResources(0, 1, pSRV);
					}
					catch (...)
					{
						ID3D11ShaderResourceView *const pSRV[1] = { NULL };
						ctx->PSSetShaderResources(0, 1, pSRV);
						return;
					}

				}

            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

	lpBadTexture.clear();
	
	if (badTextureFrames.size() > MAX_FRAMES_TO_KEEP) {
		badTextureFrames.pop_front();
	}

}

bool ImGuiHook_GetScissorArea(float* pX1, float* pY1, float* pX2, float* pY2)
{
	extern bool m_bForceRender;
	if (m_bForceRender)
		return false;

	if (1)
	{
		if (bDigAHoleToHWND)
		{
			*pX1 = rD3D11DigAHole.left;
			*pY1 = rD3D11DigAHole.top;
			*pX2 = rD3D11DigAHole.right;
			*pY2 = rD3D11DigAHole.bottom;
		}
		else
		{
			*pX1 = fImGuiScissorTopLeft.x;
			*pY1 = fImGuiScissorTopLeft.y;
			*pX2 = fImGuiScissorBottomRight.x;
			*pY2 = fImGuiScissorBottomRight.y;
		}
		return true;
	}
	else
		return false;
}

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplDX11_RenderDrawData(ImDrawData* draw_data)
{
    // TODO Phase 5: Replace with ImGui_ImplDX12_RenderDrawData()
    // Phase 4: Skip DX11 rendering when no DX11 device is available (DX12 migration)
    if (!g_pd3dDevice || !g_pd3dDeviceContext)
        return;

    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

	HRESULT hr;

    ID3D11DeviceContext* ctx = g_pd3dDeviceContext;
	#ifdef DEBUGDISPLAY
	if (!ctx) MessageBoxA(NULL, "(RenderDrawData) ctx==NULL", "Debug", 0);
	#endif

    // Create and grow vertex/index buffers if needed
    if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
        g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = g_VertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
		if (hr=g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pVB) < 0)
		{
			#ifdef DEBUGDISPLAY
			if (FAILED(hr))
			{
				char tmp[255];
				sprintf(tmp, "(RenderDrawData) g_pd3dDevice->CreateBuffer failed (%ld)", hr);
				MessageBoxA(NULL, tmp, "Debug", 0);
			}
			#endif
			return;
		}
    }
    if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
        g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = g_IndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (hr=g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pIB) < 0)
		{
			#ifdef DEBUGDISPLAY
			if (FAILED(hr))
			{
				char tmp[255];
				sprintf(tmp, "(RenderDrawData2) g_pd3dDevice->CreateBuffer failed (%ld)", hr);
				MessageBoxA(NULL, tmp, "Debug", 0);
			}
			#endif

			return;
		}
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	if (hr=ctx->Map(g_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
	{
		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp, "(RenderDrawData) ctx->Map vtx_resource failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif
		return;
	}
	if (hr = ctx->Map(g_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
	{
		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp, "(RenderDrawData) ctx->Map idx_resource failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif
		return;
	}
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    ctx->Unmap(g_pVB, 0);
    ctx->Unmap(g_pIB, 0);

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource;
		if (hr = ctx->Map(g_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
		{
			#ifdef DEBUGDISPLAY
			if (FAILED(hr))
			{
				char tmp[255];
				sprintf(tmp, "(RenderDrawData) ctx->Map mapped_resource failed (%ld)", hr);
				MessageBoxA(NULL, tmp, "Debug", 0);
			}
			#endif
			return;
		}
        VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
        ctx->Unmap(g_pVertexConstantBuffer, 0);
    }

    // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
    struct BACKUP_DX11_STATE
    {
        UINT                        ScissorRectsCount, ViewportsCount;
        D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D11RasterizerState*      RS;
        ID3D11BlendState*           BlendState;
        FLOAT                       BlendFactor[4];
        UINT                        SampleMask;
        UINT                        StencilRef;
        ID3D11DepthStencilState*    DepthStencilState;
        ID3D11ShaderResourceView*   PSShaderResource;
        ID3D11SamplerState*         PSSampler;
        ID3D11PixelShader*          PS;
        ID3D11VertexShader*         VS;
        ID3D11GeometryShader*       GS;
        UINT                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
        ID3D11ClassInstance         *PSInstances[256], *VSInstances[256], *GSInstances[256];   // 256 is max according to PSSetShader documentation
        D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
        ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
        UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
        DXGI_FORMAT                 IndexBufferFormat;
        ID3D11InputLayout*          InputLayout;
    };
    BACKUP_DX11_STATE old;
    old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
    ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
    ctx->RSGetState(&old.RS);
    ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
    ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
    ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
    ctx->PSGetSamplers(0, 1, &old.PSSampler);
    old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
    ctx->PSGetShader(&old.PS, old.PSInstances, &old.PSInstancesCount);
    ctx->VSGetShader(&old.VS, old.VSInstances, &old.VSInstancesCount);
    ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
    ctx->GSGetShader(&old.GS, old.GSInstances, &old.GSInstancesCount);

    ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
    ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    ctx->IAGetInputLayout(&old.InputLayout);

    // Setup desired DX state
    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);

	badTextureFrames.push_back(std::move(lpBadTexture));

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

			if (!pcmd->TextureId)
				continue;

			bool bBadTextureAll = false;
			for (const auto& frameTextures : badTextureFrames)
			{
				//PE: Check all frames for bad texture_srv.
				for (int i = 0; i < frameTextures.size(); i++)
				{
					ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;
					if (texture_srv == frameTextures[i])
					{
						bBadTextureAll = true;
						break;
					}
				}
			}
			if (bBadTextureAll)
				continue;

			if (pcmd->UserCallback == (ImDrawCallback)10)
			{
				bForceRenderEverywhere = true;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)11)
			{
				bForceRenderEverywhere = false;
			}
			else if (pcmd->UserCallback == (ImDrawCallback) 1)
			{
				//PE: Change shaders.
				ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)2)
			{
				ctx->PSSetShader(g_pPixelShader, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)3)
			{
				//NoAlpha.
				ctx->PSSetShader(g_pPixelShaderNoAlpha, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)4)
			{
				//Alpha.
				ctx->PSSetShader(g_pPixelShader, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)5)
			{
				//boost colors 25 percent.
				ctx->PSSetShader(g_ppixelShaderBoost25, NULL, 0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)6)
			{
				//Blur
				ctx->PSSetShader(g_pPixelShaderBlur, NULL, 0);
			}
			else if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Apply scissor/clipping rectangle
                const D3D11_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                ctx->RSSetScissorRects(1, &r);

                // Bind texture, Draw
                ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;

				bool bBadTexture = false;
				//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
				if (lpBadTexture.size() > 0)
				{
					for (int i = 0; i < lpBadTexture.size(); i++)
					{
						if (texture_srv == lpBadTexture[i])
						{
							bBadTexture = true;
							break;
						}
					}
				}
				if (!bBadTexture)
				{
					try
					{
						ctx->PSSetShaderResources(0, 1, &texture_srv);
						ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
						ID3D11ShaderResourceView *const pSRV[1] = { NULL };
						ctx->PSSetShaderResources(0, 1, pSRV);
					}
					catch (...)
					{
						ID3D11ShaderResourceView *const pSRV[1] = { NULL };
						ctx->PSSetShaderResources(0, 1, pSRV);
						return;
					}
				}
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

	lpBadTexture.clear();

	if (badTextureFrames.size() > MAX_FRAMES_TO_KEEP) {
		badTextureFrames.pop_front();
	}

    // Restore modified DX state
    ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
    ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
    ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
    ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
    ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
    ctx->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
    ctx->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
    ctx->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount); if (old.PS) old.PS->Release();
    for (UINT i = 0; i < old.PSInstancesCount; i++) if (old.PSInstances[i]) old.PSInstances[i]->Release();
    ctx->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount); if (old.VS) old.VS->Release();
    ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
    ctx->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount); if (old.GS) old.GS->Release();
    for (UINT i = 0; i < old.VSInstancesCount; i++) if (old.VSInstances[i]) old.VSInstances[i]->Release();
    ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
    ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
    ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
    ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
}

static void ImGui_ImplDX11_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D *pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
		if (g_pFontTextureView) { g_pFontTextureView->Release(); g_pFontTextureView = NULL; ImGui::GetIO().Fonts->TexID = NULL; }
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &g_pFontTextureView);
        pTexture->Release();
    }

    // Store our identifier
    io.Fonts->TexID = (ImTextureID)g_pFontTextureView;

    // Create texture sampler
    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
		if (g_pFontSampler) { g_pFontSampler->Release(); g_pFontSampler = NULL; }
        g_pd3dDevice->CreateSamplerState(&desc, &g_pFontSampler);
    }
}

bool    ImGui_ImplDX11_CreateDeviceObjects()
{
    // Phase 5: DX12 path — rebuild font texture via bridge instead of DX11
    extern bool ImGui_DX12_IsInitialized();
    if (ImGui_DX12_IsInitialized())
    {
        extern void ImGui_DX12_RebuildFontTexture();
        ImGui_DX12_RebuildFontTexture();
        return true;
    }

    if (!g_pd3dDevice)
    {
        // No DX11 device and no DX12 bridge — at minimum build the font atlas
        // to prevent crash in SetCurrentFont when accessing null ContainerAtlas
        ImGuiIO& io = ImGui::GetIO();
        if (!io.Fonts->IsBuilt())
            io.Fonts->Build();
        return false;
    }
    if (g_pFontSampler)
        ImGui_ImplDX11_InvalidateDeviceObjects();

    // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
    // If you would like to use this DX11 sample code but remove this dependency you can:
    //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
    //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
    // See https://github.com/ocornut/imgui/pull/638 for sources and details.

    // Create the vertex shader
    {
        static const char* vertexShader =
            "cbuffer vertexBuffer : register(b0) \
            {\
            float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
            float2 pos : POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
            PS_INPUT output;\
            output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
            output.col = input.col;\
            output.uv  = input.uv;\
            return output;\
            }";

        //D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &g_pVertexShaderBlob, NULL);
		D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &g_pVertexShaderBlob, NULL);
		if (g_pVertexShaderBlob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return false;
        if (g_pd3dDevice->CreateVertexShader((DWORD*)g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize(), NULL, &g_pVertexShader) != S_OK)
            return false;

        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC local_layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        if (g_pd3dDevice->CreateInputLayout(local_layout, 3, g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize(), &g_pInputLayout) != S_OK)
            return false;

        // Create the constant buffer
        {
            D3D11_BUFFER_DESC desc;
            desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pVertexConstantBuffer);
        }
    }

    // Create the pixel shader
    {
        static const char* pixelShader =
            "struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
            return out_col; \
            }";
		//PE: This will normally be used to display large images as small icons, so texelsize is set higher and fixed (512).
		static const char* pixelShaderBlur =
			"struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
			float texelSize = 1.0f / 512.0f;\
            float4 out_col = texture0.Sample(sampler0, input.uv); \
            out_col += texture0.Sample(sampler0, input.uv + float2(0.0f,texelSize) ); \
            out_col += texture0.Sample(sampler0, input.uv + float2(texelSize,texelSize) ); \
            out_col += texture0.Sample(sampler0, input.uv + float2(texelSize,0.0f) ); \
            out_col += texture0.Sample(sampler0, input.uv + float2(0.0f,-texelSize) ); \
            out_col += texture0.Sample(sampler0, input.uv + float2(-texelSize,-texelSize) ); \
            out_col += texture0.Sample(sampler0, input.uv + float2(-texelSize,0.0f) ); \
            out_col += texture0.Sample(sampler0, input.uv + float2(texelSize,-texelSize) ); \
			out_col *= 0.125f;\
            out_col = input.col * out_col; \
            return out_col; \
            }";
		static const char* pixelShaderNoWhite =
			"struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 img_col = texture0.Sample(sampler0, input.uv); \
            float4 out_col = input.col * img_col; \
			if( img_col.r >= 0.9 && img_col.g >= 0.9 && img_col.b >= 0.9) { \
				out_col.a = 0.0; \
			} \
			return out_col; \
            }";

		static const char* pixelShaderBoost25 =
			"struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 img_col = texture0.Sample(sampler0, input.uv); \
			img_col.rgb = (img_col.rgb * 0.75) + 0.35; \
            float4 out_col = input.col * img_col; \
			return out_col; \
            }";

		static const char* pixelShaderNoAlpha =
			"struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
            out_col.w = 1.0; \
            return out_col; \
            }";

        //D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_pPixelShaderBlob, NULL);
		D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderBlob, NULL);
		if (g_pPixelShaderBlob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return false;
        if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pPixelShaderBlob->GetBufferPointer(), g_pPixelShaderBlob->GetBufferSize(), NULL, &g_pPixelShader) != S_OK)
            return false;

		//D3DCompile(pixelShaderNoWhite, strlen(pixelShaderNoWhite), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_pPixelShaderNoWhiteBlob, NULL);
		D3DCompile(pixelShaderNoWhite, strlen(pixelShaderNoWhite), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderNoWhiteBlob, NULL);
		if (g_pPixelShaderNoWhiteBlob == NULL)
			return false;
		if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pPixelShaderNoWhiteBlob->GetBufferPointer(), g_pPixelShaderNoWhiteBlob->GetBufferSize(), NULL, &g_pPixelShaderNoWhite) != S_OK)
			return false;

		//D3DCompile(pixelShaderNoAlpha, strlen(pixelShaderNoAlpha), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_pPixelShaderNoAlphaBlob, NULL);
		D3DCompile(pixelShaderNoAlpha, strlen(pixelShaderNoAlpha), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderNoAlphaBlob, NULL);
		if (g_pPixelShaderNoAlphaBlob == NULL)
			return false;
		if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pPixelShaderNoAlphaBlob->GetBufferPointer(), g_pPixelShaderNoAlphaBlob->GetBufferSize(), NULL, &g_pPixelShaderNoAlpha) != S_OK)
			return false;

		//D3DCompile(pixelShaderBoost25, strlen(pixelShaderBoost25), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_ppixelShaderBoost25Blob, NULL);
		D3DCompile(pixelShaderBoost25, strlen(pixelShaderBoost25), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_ppixelShaderBoost25Blob, NULL);
		if (g_ppixelShaderBoost25Blob == NULL)
			return false;
		if (g_pd3dDevice->CreatePixelShader((DWORD*)g_ppixelShaderBoost25Blob->GetBufferPointer(), g_ppixelShaderBoost25Blob->GetBufferSize(), NULL, &g_ppixelShaderBoost25) != S_OK)
			return false;

		//D3DCompile(pixelShaderBlur, strlen(pixelShaderBlur), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_pPixelShaderBlobBlur, NULL);
		D3DCompile(pixelShaderBlur, strlen(pixelShaderBlur), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderBlobBlur, NULL);
		if (g_pPixelShaderBlobBlur == NULL)
			return false;
		if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pPixelShaderBlobBlur->GetBufferPointer(), g_pPixelShaderBlobBlur->GetBufferSize(), NULL, &g_pPixelShaderBlur) != S_OK)
			return false;
		
    }

    // Create the blending setup
    {
        D3D11_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_pd3dDevice->CreateBlendState(&desc, &g_pBlendState);
    }

    // Create the rasterizer state
    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
        g_pd3dDevice->CreateRasterizerState(&desc, &g_pRasterizerState);
    }

    // Create depth-stencil State
    {
        D3D11_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        g_pd3dDevice->CreateDepthStencilState(&desc, &g_pDepthStencilState);
    }

    ImGui_ImplDX11_CreateFontsTexture();

    return true;
}

void    ImGui_ImplDX11_InvalidateDeviceObjects()
{
    if (!g_pd3dDevice)
        return;

    if (g_pFontSampler) { g_pFontSampler->Release(); g_pFontSampler = NULL; }
    if (g_pFontTextureView) { g_pFontTextureView->Release(); g_pFontTextureView = NULL; ImGui::GetIO().Fonts->TexID = NULL; } // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
    if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
    if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }

    if (g_pBlendState) { g_pBlendState->Release(); g_pBlendState = NULL; }
    if (g_pDepthStencilState) { g_pDepthStencilState->Release(); g_pDepthStencilState = NULL; }
    if (g_pRasterizerState) { g_pRasterizerState->Release(); g_pRasterizerState = NULL; }
    if (g_pPixelShader) { g_pPixelShader->Release(); g_pPixelShader = NULL; }
    if (g_pPixelShaderBlob) { g_pPixelShaderBlob->Release(); g_pPixelShaderBlob = NULL; }

	if (g_pPixelShaderBlur) { g_pPixelShaderBlur->Release(); g_pPixelShaderBlur = NULL; }
	if (g_pPixelShaderBlobBlur) { g_pPixelShaderBlobBlur->Release(); g_pPixelShaderBlobBlur = NULL; }

    if (g_pVertexConstantBuffer) { g_pVertexConstantBuffer->Release(); g_pVertexConstantBuffer = NULL; }
    if (g_pInputLayout) { g_pInputLayout->Release(); g_pInputLayout = NULL; }
    if (g_pVertexShader) { g_pVertexShader->Release(); g_pVertexShader = NULL; }
    if (g_pVertexShaderBlob) { g_pVertexShaderBlob->Release(); g_pVertexShaderBlob = NULL; }
}

bool    ImGui_ImplDX11_Init(ID3D11Device* device, ID3D11DeviceContext* device_context)
{
    // TODO Phase 5: Replace DX11 ImGui backend with DX12 backend (imgui_impl_dx12.h/cpp from D:\max\imgui\backends)
    // Phase 4: Guard against NULL device during DX12 migration — DX11 device is no longer available
    if (!device || !device_context)
    {
        bImGuiInitDone = false;
        return false;
    }

    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx11";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    // Get factory from device
    IDXGIDevice* pDXGIDevice = NULL;
    IDXGIAdapter* pDXGIAdapter = NULL;
    IDXGIFactory* pFactory = NULL;

    if (device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)) == S_OK)
        if (pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter)) == S_OK)
            if (pDXGIAdapter->GetParent(IID_PPV_ARGS(&pFactory)) == S_OK)
            {
                g_pd3dDevice = device;
                g_pd3dDeviceContext = device_context;
                g_pFactory = pFactory;
            }
    if (pDXGIDevice) pDXGIDevice->Release();
    if (pDXGIAdapter) pDXGIAdapter->Release();
    g_pd3dDevice->AddRef();
    g_pd3dDeviceContext->AddRef();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplDX11_InitPlatformInterface();

    return true;
}

int ImGui_Get_Allow_Undocking()
{
	return pref.iAllowUndocking;
}
int ImGui_Get_Multi_Viewports_Disabled()
{
	return pref.iDisableObjectLibraryViewport;
}


void ImGui_ImplDX11_Shutdown()
{
    // TODO Phase 5: Replace with ImGui_ImplDX12_Shutdown()
    // Phase 4: Safe to call even when DX11 was never initialized
    ImGui_ImplDX11_ShutdownPlatformInterface();
    ImGui_ImplDX11_InvalidateDeviceObjects();
    if (g_pFactory) { g_pFactory->Release(); g_pFactory = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
}

void ImGui_ImplDX11_NewFrame()
{
    // Phase 5: Redirect to DX12 backend NewFrame
    // All 9+ call sites continue to call ImGui_ImplDX11_NewFrame() — we forward to DX12 here.
    extern bool ImGui_DX12_IsInitialized();
    if (ImGui_DX12_IsInitialized())
    {
        extern void ImGui_DX12_NewFrame();
        ImGui_DX12_NewFrame();
        return;
    }

    // TODO: removed DX11 ImGui path — original DX11 backend NewFrame
    // if (!g_pd3dDevice)
    //     return;
    // if (!g_pFontSampler)
    //     ImGui_ImplDX11_CreateDeviceObjects();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the back-end to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

struct ImGuiViewportDataDx11
{
    IDXGISwapChain*             SwapChain;
    ID3D11RenderTargetView*     RTView;

    ImGuiViewportDataDx11()     { SwapChain = NULL; RTView = NULL; }
    ~ImGuiViewportDataDx11()    { IM_ASSERT(SwapChain == NULL && RTView == NULL); }
};


static void ImGui_ImplDX11_CreateWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataDx11* data = IM_NEW(ImGuiViewportDataDx11)();
    viewport->RendererUserData = data;

    // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some back-end will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
    HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
    IM_ASSERT(hwnd != 0);

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferDesc.Width = (UINT)viewport->Size.x;
    sd.BufferDesc.Height = (UINT)viewport->Size.y;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = hwnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

	HRESULT hr;

    IM_ASSERT(data->SwapChain == NULL && data->RTView == NULL);

	#ifdef DEBUGDISPLAY
	if (!g_pFactory)
	{
		MessageBoxA(NULL, "g_pFactory==NULL", "Debug", 0);
	}
	#endif

    hr = g_pFactory->CreateSwapChain(g_pd3dDevice, &sd, &data->SwapChain);

	#ifdef DEBUGDISPLAY
	if (FAILED(hr))
	{
		if (hr == DXGI_STATUS_OCCLUDED)
		{
			MessageBoxA(NULL, "DXGI_STATUS_OCCLUDED", "Debug", 0);
		}
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			MessageBoxA(NULL, "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE", "Debug", 0);
		}
		if (hr == E_OUTOFMEMORY)
		{
			MessageBoxA(NULL, "E_OUTOFMEMORY", "Debug", 0);
		}
		if (!data->SwapChain)
		{
			MessageBoxA(NULL, "data->SwapChain==NULL", "Debug", 0);
		}
	}
	#endif
    // Create the render target
    if (data->SwapChain)
    {
        ID3D11Texture2D* pBackBuffer;
        hr = data->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp,"data->SwapChain->GetBuffer failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif

		hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &data->RTView);

		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp, "g_pd3dDevice->CreateRenderTargetView failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif
		pBackBuffer->Release();
    }
}

static void ImGui_ImplDX11_DestroyWindow(ImGuiViewport* viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
    if (ImGuiViewportDataDx11* data = (ImGuiViewportDataDx11*)viewport->RendererUserData)
    {
        if (data->SwapChain)
            data->SwapChain->Release();
        data->SwapChain = NULL;
        if (data->RTView)
            data->RTView->Release();
        data->RTView = NULL;
        IM_DELETE(data);
    }
    viewport->RendererUserData = NULL;
}

static void ImGui_ImplDX11_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiViewportDataDx11* data = (ImGuiViewportDataDx11*)viewport->RendererUserData;
    if (data->RTView)
    {
        data->RTView->Release();
        data->RTView = NULL;
    }
	HRESULT hr;
    if (data->SwapChain)
    {
        ID3D11Texture2D* pBackBuffer = NULL;
        hr = data->SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp, "(SetWindowSize) data->SwapChain->ResizeBuffers failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif

		hr = data->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp, "(SetWindowSize) data->SwapChain->GetBuffer failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif

        if (pBackBuffer == NULL) { fprintf(stderr, "ImGui_ImplDX11_SetWindowSize() failed creating buffers.\n"); return; }
        hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &data->RTView);
		#ifdef DEBUGDISPLAY
		if (FAILED(hr))
		{
			char tmp[255];
			sprintf(tmp, "(SetWindowSize) g_pd3dDevice->CreateRenderTargetView failed (%ld)", hr);
			MessageBoxA(NULL, tmp, "Debug", 0);
		}
		#endif
		pBackBuffer->Release();
    }
}

static void ImGui_ImplDX11_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataDx11* data = (ImGuiViewportDataDx11*)viewport->RendererUserData;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

	HRESULT hr;

	#ifdef DEBUGDISPLAY
	if(!g_pd3dDeviceContext) MessageBoxA(NULL, "(RenderWindow) g_pd3dDeviceContext==NULL", "Debug", 0);
	if(!data->RTView) MessageBoxA(NULL, "(RenderWindow) data->RTView==NULL", "Debug", 0);
	#endif

    g_pd3dDeviceContext->OMSetRenderTargets(1, &data->RTView, NULL);

    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
        g_pd3dDeviceContext->ClearRenderTargetView(data->RTView, (float*)&clear_color);
    ImGui_ImplDX11_RenderDrawData(viewport->DrawData);
}


static void ImGui_ImplDX11_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataDx11* data = (ImGuiViewportDataDx11*)viewport->RendererUserData;
	if (data->SwapChain) //PE: need to check if still alive.
	{
		//PE: We need to update the directx include files in the repo.
		#ifndef DXGI_PRESENT_DO_NOT_WAIT
		#define DXGI_PRESENT_DO_NOT_WAIT               0x00000008UL
		#endif
		//PE: Sometimes Present never return.
		//PE: This did not help.
		//PE: This only happen in tab tab when you move a window to another monitor, so disable this for now.
		//PE: Until the reason is found. DXGI_PRESENT_DO_NOT_WAIT dont seam to fix it.
		//wiGraphics::GetDevice()->WaitForGPU(); //This did not help.
		//DXGI_SWAP_EFFECT_DISCARD 
		//PE: Think the problem is that we use a VERY old gxgi.h and we really should update this and start using a FLIP model.

		HRESULT hr = data->SwapChain->Present(0, DXGI_PRESENT_DO_NOT_WAIT); // Present without vsync

		if (FAILED(hr))
		{
			#ifdef DEBUGDISPLAY
			if (FAILED(hr))
			{
				//Graphix got this:
				//"-2005270518" Windows error 0x887A000A, -2005270518 (The GPU was busy at the moment when the call was made, and the call was neither executed nor scheduled.)
				//char tmp[255];
				//sprintf(tmp, "(SwapBuffers) data->SwapChain->Present failed (%ld)", hr);
				//MessageBoxA(NULL, tmp, "Debug", 0);
			}
			#endif

			if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
			{
				//PE: Need to test this more.
				//hr = 0x887a000a : The GPU was busy at the moment when the call was made, and the call was neither executed nor scheduled. 
				//printf("tmp");
			}
		}
	}
}

static void ImGui_ImplDX11_InitPlatformInterface()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ImGui_ImplDX11_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_ImplDX11_DestroyWindow;
    platform_io.Renderer_SetWindowSize = ImGui_ImplDX11_SetWindowSize;
    platform_io.Renderer_RenderWindow = ImGui_ImplDX11_RenderWindow;
    platform_io.Renderer_SwapBuffers = ImGui_ImplDX11_SwapBuffers;
}

static void ImGui_ImplDX11_ShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}

int ImGui_GetActiveViewPorts(void)
{
	ImGuiContext& g = *GImGui;
	if (GImGui != NULL)
	{
		if (g.Initialized)
		{
			ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
			return platform_io.Viewports.Size;
		}
	}
	return 0;
}

//#################################
//PE: Additional dialogs code here.
//#################################
#define GG_WINDOWS
#define AGK_WINDOWS

#include "globstruct.h"

extern GlobStruct*	g_pGlob;
#define g_agkhWnd g_pGlob->hWnd

#include "boxer.h"

//Noc File dialog cross platform.
//const char *noc_file_dialog_open(int flags,
//	const char *filters,
//	const char *default_path,
//	const char *default_name);

#include <stdlib.h>
#include <string.h>

static char *g_noc_file_dialog_ret = NULL;


#ifdef GG_WINDOWS

#include "windows.h"
#include "winuser.h"
#include <shlobj.h>
#include <conio.h>

static int __stdcall BrowseCallbackProcW(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
#ifdef UNICODE
		SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pData);
#else
		SendMessageA(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pData);
#endif
	}
	return 0;
}

#endif // GG_WINDOWS - continued in part1
// callback function
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
}

