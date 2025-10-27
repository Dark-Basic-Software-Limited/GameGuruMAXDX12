//----------------------------------------------------
//--- GAMEGURU - M-Postprocess
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

#include "openxr.h"

//PE: GameGuru IMGUI.
#include "..\Imgui\imgui.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

// 
//  POST PROCESSING (0-main cam,1-reserved,2-reflect cam,3-finalrender cam)
//  4-sunlight ray camera
// 

// Some convenient globals for VR controller shader and textures
int g_iCShaderID = 0;
int g_iCTextureID0 = 0;
int g_iCTextureID1 = 0;
int g_iCTextureID2 = 0;
int g_iCTextureID3 = 0;
int g_iCTextureID4 = 0;
int g_iCTextureID5 = 0;
int g_iCTextureID6 = 0;

void postprocess_init ( void )
{
	// full screen shaders
	g.gpostprocessing = 1;

	// no need for post processing, but need to init VR (copied from below)
	char pErrorStr[1024];
	sprintf ( pErrorStr, "check if VR required with codes %d and %d", g.vrglobals.GGVREnabled, g.vrglobals.GGVRUsingVRSystem );
	timestampactivity(0,pErrorStr);
	if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
	{
		// Set camera IDs and initialise GGVR
		t.glefteyecameraid = 6;
		t.grighteyecameraid = 7;
		g.vrglobals.GGVRInitialized = 0;
		sprintf ( pErrorStr, "initialise VR System Mode %d", g.vrglobals.GGVREnabled );
		timestampactivity(0,pErrorStr);
		if ( g_iCShaderID == 0 ) g_iCShaderID = g.controllerpbreffect;
		if ( g_iCTextureID0 == 0 ) g_iCTextureID0 = loadinternaltextureex("gamecore\\vrcontroller\\vrcontroller_color.png", 1, t.tfullorhalfdivide);
		if ( g_iCTextureID1 == 0 ) g_iCTextureID1 = loadinternaltextureex("effectbank\\reloaded\\media\\blank_O.dds", 1, t.tfullorhalfdivide);
		if ( g_iCTextureID2 == 0 ) g_iCTextureID2 = loadinternaltextureex("gamecore\\vrcontroller\\vrcontroller_normal.png", 1, t.tfullorhalfdivide);
		if ( g_iCTextureID3 == 0 ) g_iCTextureID3 = loadinternaltextureex("gamecore\\vrcontroller\\vrcontroller_metalness.png", 1, t.tfullorhalfdivide);
		if ( g_iCTextureID4 == 0 ) g_iCTextureID4 = loadinternaltextureex("gamecore\\vrcontroller\\vrcontroller_gloss.png", 1, t.tfullorhalfdivide);
		if ( g_iCTextureID5 == 0 ) g_iCTextureID5 = g.postprocessimageoffset+5;
		if ( g_iCTextureID6 == 0 ) g_iCTextureID6 = t.terrain.imagestartindex+31;//loadinternaltextureex("effectbank\\reloaded\\media\\blank_I.dds", 1, t.tfullorhalfdivide);
		int oculusTex0 = loadinternaltextureex("gamecore\\vrcontroller\\oculus\\controller_bc.png", 1, t.tfullorhalfdivide);
		sprintf ( pErrorStr, "controller asset %d %d %d %d %d %d %d %d", g_iCShaderID, g_iCTextureID0, g_iCTextureID1, g_iCTextureID2, g_iCTextureID3, g_iCTextureID4, g_iCTextureID5, g_iCTextureID6 );
		timestampactivity(0,pErrorStr);
		int iErrorCode = GGVR_Init ( g.rootdir_s.Get(), g.postprocessimageoffset + 4, g.postprocessimageoffset + 3, t.grighteyecameraid, t.glefteyecameraid, 10000, 10001, 10002, 10003, 10004, 10005, 10099, g.guishadereffectindex, g.editorimagesoffset+14, g_iCShaderID, g_iCTextureID0, g_iCTextureID1, g_iCTextureID2, g_iCTextureID3, g_iCTextureID4, g_iCTextureID5, g_iCTextureID6, oculusTex0);
		if ( iErrorCode > 0 )
		{
			sprintf ( pErrorStr, "Error starting VR : Code %d", iErrorCode );
			timestampactivity(0,pErrorStr);
			t.visuals.generalpromptstatetimer = Timer()+1000;
			t.visuals.generalprompt_s = "No OpenXR runtime found";
		}
		GGVR_SetGenericOffsetAngX( g.gvroffsetangx );
		GGVR_SetWMROffsetAngX( g.gvrwmroffsetangx );

		// used to mark VR initiated (so can free resources when done)
		t.gpostprocessmode = 1;
	}
}

void postprocess_reset_fade ( void )
{
}

void postprocess_general_init ( void )
{
}

void postprocess_free ( void )
{
	// only free if enagaged
	if ( t.gpostprocessmode > 0 )
	{
		// free GGVR if used
		if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
		{
			GGVR_Shutdown();
		}

		// Wicked has post processing covered!

		// and reset flag
		t.gpostprocessmode=0;
	}
}

void postprocess_off ( void )
{
}

void postprocess_on ( void )
{
}

void postprocess_preterrain ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	// Most rendering done in master for wicked, but some code from below remains for alignment of controls
	if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
	{
		if ( !GGVR_IsRuntimeFound() ) GGVR_ReInit();
		if ( !GGVR_IsRuntimeFound() )
		{
			t.visuals.generalpromptstatetimer = Timer()+1000;
			t.visuals.generalprompt_s = "OpenXR runtime not found";
		}
		else
		{
			// position VR player at location of main camera
			GGVR_SetPlayerPosition(t.tFinalCamX_f, t.tFinalCamY_f, t.tFinalCamZ_f);

			// this sets the origin based on the current camera zero (ARG!)
			// should only set based on player angle (minus HMD influence) as HMD added later at right time for smooth headset viewing!
			GGVR_SetPlayerAngleY(t.camangy_f);

			// update seated/standing flag
			g.vrglobals.GGVRStandingMode = GGVR_GetTrackingSpace();

			// handle teleport
			bool bAllowPlayerTeleport = false;
			if (bAllowPlayerTeleport == true)
			{
				float fTelePortDestX = 0.0f;
				float fTelePortDestY = 0.0f;
				float fTelePortDestZ = 0.0f;
				float fTelePortDestAngleY = 0.0f;
				bool VRteleport = GGVR_HandlePlayerTeleport (&fTelePortDestX, &fTelePortDestY, &fTelePortDestZ, &fTelePortDestAngleY);
				if (VRteleport)
				{
					physics_disableplayer ();
					t.terrain.playerx_f = fTelePortDestX;
					t.terrain.playery_f = fTelePortDestY + 30;
					t.terrain.playerz_f = fTelePortDestZ;
					physics_setupplayer ();
				}
			}

			// update HMD position and controller feedback
			bool bPlayerDucking = false;
			if ( t.aisystem.playerducking != 0 ) bPlayerDucking = true;
			int iBatchStart = g.batchobjectoffset;
			int iBatchEnd = g.batchobjectoffset + g.merged_new_objects + 1;
			GGVR_UpdatePlayer(bPlayerDucking,t.terrain.TerrainID,g.lightmappedobjectoffset,g.lightmappedobjectoffsetfinish,g.entityviewstartobj,g.entityviewendobj,iBatchStart,iBatchEnd);
						
			// set some values the player control script needs!
			GGVR_SetOpenXRValuesForMAX();
		}
	}
}

void postprocess_setscreencolor ( void )
{
	float fade = GetXVector4(t.tColorVector);
	if (fade > 0.0)
	{
		extern bool bImGuiInTestGame;
		extern bool bRenderTabTab;
		extern bool bBlockImGuiUntilNewFrame;
		extern bool bImGuiRenderWithNoCustomTextures;
		extern bool g_bNoGGUntilGameGuruMainCalled;
		extern bool bImGuiFrameState;

		if ((bImGuiInTestGame) && !bRenderTabTab && !bImGuiFrameState)
		{
			//We need a new frame.
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			bRenderTabTab = true;
			bBlockImGuiUntilNewFrame = false;
			bImGuiRenderWithNoCustomTextures = false;
			extern bool bSpriteWinVisible;
			bSpriteWinVisible = false;
		}

		ImGuiViewport* mainviewport = ImGui::GetMainViewport();
		if (mainviewport)
		{
			ImDrawList* drawlist = ImGui::GetForegroundDrawList(mainviewport);
			if (drawlist)
			{
				ImVec4 monitor_col = ImVec4(0.0, 0.0, 0.0, fade); //Fade in.
				drawlist->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size, ImGui::GetColorU32(monitor_col));
			}
		}
	}
}
