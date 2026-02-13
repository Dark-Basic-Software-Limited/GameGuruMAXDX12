// dear imgui, v1.65
// (main code and documentation)

// Call and read ImGui::ShowDemoWindow() in imgui_demo.cpp for demo code.
// Newcomers, read 'Programmer guide' below for notes on how to setup Dear ImGui in your codebase.
// Get latest version at https://github.com/ocornut/imgui
// Releases change-log at https://github.com/ocornut/imgui/releases
// Technical Support for Getting Started https://discourse.dearimgui.org/c/getting-started
// Gallery (please post your screenshots/video there!): https://github.com/ocornut/imgui/issues/1269
// Developed by Omar Cornut and every direct or indirect contributors to the GitHub.
// This library is free but I need your support to sustain development and maintenance.
// If you work for a company, please consider financial support, see README. For individuals: https://www.patreon.com/imgui

// It is recommended that you don't modify imgui.cpp! It will become difficult for you to update the library.
// Note that 'ImGui::' being a namespace, you can add functions into the namespace from your own source files, without
// modifying imgui.h or imgui.cpp. You may include imgui_internal.h to access internal data structures, but it doesn't
// come with any guarantee of forward compatibility. Discussing your changes on the GitHub Issue Tracker may lead you
// to a better solution or official support for them.

/*

Index of this file:

DOCUMENTATION

- MISSION STATEMENT
- END-USER GUIDE
- PROGRAMMER GUIDE (read me!)
  - Read first
  - How to update to a newer version of Dear ImGui
  - Getting started with integrating Dear ImGui in your code/engine
  - This is how a simple application may look like (2 variations)
  - This is how a simple rendering function may look like
  - Using gamepad/keyboard navigation controls
- API BREAKING CHANGES (read me when you update!)
- FREQUENTLY ASKED QUESTIONS (FAQ), TIPS
  - How can I tell whether to dispatch mouse/keyboard to imgui or to my application?
  - How can I display an image? What is ImTextureID, how does it works?
  - How can I have multiple widgets with the same label or without a label? A primer on labels and the ID Stack.
  - How can I use my own math types instead of ImVec2/ImVec4? 
  - How can I load a different font than the default?
  - How can I easily use icons in my application?
  - How can I load multiple fonts?
  - How can I display and input non-latin characters such as Chinese, Japanese, Korean, Cyrillic?
  - How can I use the drawing facilities without an ImGui window? (using ImDrawList API)
  - I integrated Dear ImGui in my engine and the text or lines are blurry..
  - I integrated Dear ImGui in my engine and some elements are clipping or disappearing when I move windows around..
  - How can I help?

CODE 
(search for "[SECTION]" in the code to find them)

// [SECTION] FORWARD DECLARATIONS
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
// [SECTION] MAIN USER FACING STRUCTURES (ImGuiStyle, ImGuiIO)
// [SECTION] MISC HELPER/UTILITIES (Maths, String, Format, Hash, File functions)
// [SECTION] MISC HELPER/UTILITIES (ImText* functions)
// [SECTION] MISC HELPER/UTILITIES (Color functions)
// [SECTION] ImGuiStorage
// [SECTION] ImGuiTextFilter
// [SECTION] ImGuiTextBuffer
// [SECTION] ImGuiListClipper
// [SECTION] RENDER HELPERS
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
// [SECTION] TOOLTIPS
// [SECTION] POPUPS
// [SECTION] KEYBOARD/GAMEPAD NAVIGATION
// [SECTION] COLUMNS
// [SECTION] DRAG AND DROP
// [SECTION] LOGGING/CAPTURING
// [SECTION] SETTINGS
// [SECTION] PLATFORM DEPENDENT HELPERS
// [SECTION] METRICS/DEBUG WINDOW

*/

//-----------------------------------------------------------------------------
// DOCUMENTATION
//-----------------------------------------------------------------------------

/*

 MISSION STATEMENT
 =================

 - Easy to use to create code-driven and data-driven tools
 - Easy to use to create ad hoc short-lived tools and long-lived, more elaborate tools
 - Easy to hack and improve
 - Minimize screen real-estate usage
 - Minimize setup and maintenance
 - Minimize state storage on user side
 - Portable, minimize dependencies, run on target (consoles, phones, etc.)
 - Efficient runtime and memory consumption (NB- we do allocate when "growing" content e.g. creating a window,
   opening a tree node for the first time, etc. but a typical frame should not allocate anything)

 Designed for developers and content-creators, not the typical end-user! Some of the weaknesses includes:
 - Doesn't look fancy, doesn't animate
 - Limited layout features, intricate layouts are typically crafted in code


 END-USER GUIDE
 ==============

 - Double-click on title bar to collapse window.
 - Click upper right corner to close a window, available when 'bool* p_open' is passed to ImGui::Begin().
 - Click and drag on lower right corner to resize window (double-click to auto fit window to its contents).
 - Click and drag on any empty space to move window.
 - TAB/SHIFT+TAB to cycle through keyboard editable fields.
 - CTRL+Click on a slider or drag box to input value as text.
 - Use mouse wheel to scroll.
 - Text editor:
   - Hold SHIFT or use mouse to select text.
   - CTRL+Left/Right to word jump.
   - CTRL+Shift+Left/Right to select words.
   - CTRL+A our Double-Click to select all.
   - CTRL+X,CTRL+C,CTRL+V to use OS clipboard/
   - CTRL+Z,CTRL+Y to undo/redo.
   - ESCAPE to revert text to its original value.
   - You can apply arithmetic operators +,*,/ on numerical values. Use +- to subtract (because - would set a negative value!)
   - Controls are automatically adjusted for OSX to match standard OSX text editing operations.
 - General Keyboard controls: enable with ImGuiConfigFlags_NavEnableKeyboard.
 - General Gamepad controls: enable with ImGuiConfigFlags_NavEnableGamepad. See suggested mappings in imgui.h ImGuiNavInput_ + download PNG/PSD at http://goo.gl/9LgVZW


 PROGRAMMER GUIDE
 ================

 READ FIRST

 - Read the FAQ below this section!
 - Your code creates the UI, if your code doesn't run the UI is gone! The UI can be highly dynamic, there are no construction
   or destruction steps, less data retention on your side, less state duplication, less state synchronization, less bugs.
 - Call and read ImGui::ShowDemoWindow() for demo code demonstrating most features.
 - You can learn about immediate-mode gui principles at http://www.johno.se/book/imgui.html or watch http://mollyrocket.com/861

 HOW TO UPDATE TO A NEWER VERSION OF DEAR IMGUI

 - Overwrite all the sources files except for imconfig.h (if you have made modification to your copy of imconfig.h)
 - Read the "API BREAKING CHANGES" section (below). This is where we list occasional API breaking changes.
   If a function/type has been renamed / or marked obsolete, try to fix the name in your code before it is permanently removed
   from the public API. If you have a problem with a missing function/symbols, search for its name in the code, there will
   likely be a comment about it. Please report any issue to the GitHub page!
 - Try to keep your copy of dear imgui reasonably up to date.

 GETTING STARTED WITH INTEGRATING DEAR IMGUI IN YOUR CODE/ENGINE

 - Run and study the examples and demo to get acquainted with the library.
 - Add the Dear ImGui source files to your projects or using your preferred build system.
   It is recommended you build the .cpp files as part of your project and not as a library.
 - You can later customize the imconfig.h file to tweak some compilation time behavior, such as integrating imgui types with your own maths types.
 - You may be able to grab and copy a ready made imgui_impl_*** file from the examples/ folder.
 - When using Dear ImGui, your programming IDE is your friend: follow the declaration of variables, functions and types to find comments about them.
 - Dear ImGui never touches or knows about your GPU state. The only function that knows about GPU is the draw function that you provide.
   Effectively it means you can create widgets at any time in your code, regardless of considerations of being in "update" vs "render"
   phases of your own application. All rendering informatioe are stored into command-lists that you will retrieve after calling ImGui::Render().
 - Refer to the bindings and demo applications in the examples/ folder for instruction on how to setup your code.

 THIS IS HOW A SIMPLE APPLICATION MAY LOOK LIKE
 EXHIBIT 1: USING THE EXAMPLE BINDINGS (imgui_impl_XXX.cpp files from the examples/ folder)

     // Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Initialize helper Platform and Renderer bindings (here we are using imgui_impl_win32 and imgui_impl_dx11)
     ImGui_ImplWin32_Init(hwnd);
     ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

     // Application main loop
     while (true)
     {
         // Feed inputs to dear imgui, start new frame
         ImGui_ImplDX11_NewFrame();
         ImGui_ImplWin32_NewFrame();
         ImGui::NewFrame();

         // Any application code here
         ImGui::Text("Hello, world!");

         // Render dear imgui into screen
         ImGui::Render();
         ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
         g_pSwapChain->Present(1, 0);
     }
     
     // Shutdown
     ImGui_ImplDX11_Shutdown();
     ImGui_ImplWin32_Shutdown();
     ImGui::DestroyContext();

 THIS IS HOW A SIMPLE APPLICATION MAY LOOK LIKE
 EXHIBIT 2: IMPLEMENTING CUSTOM BINDING / CUSTOM ENGINE

     // Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Build and load the texture atlas into a texture
     // (In the examples/ app this is usually done within the ImGui_ImplXXX_Init() function from one of the demo Renderer)
     int width, height;
     unsigned char* pixels = NULL;
     io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

     // At this point you've got the texture data and you need to upload that your your graphic system:
     // After we have created the texture, store its pointer/identifier (_in whichever format your engine uses_) in 'io.Fonts->TexID'.
     // This will be passed back to your via the renderer. Basically ImTextureID == void*. Read FAQ below for details about ImTextureID.
     MyTexture* texture = MyEngine::CreateTextureFromMemoryPixels(pixels, width, height, TEXTURE_TYPE_RGBA32)
     io.Fonts->TexID = (void*)texture;

     // Application main loop
     while (true)
     {
        // Setup low-level inputs, e.g. on Win32: calling GetKeyboardState(), or write to those fields from your Windows message handlers, etc.
        // (In the examples/ app this is usually done within the ImGui_ImplXXX_NewFrame() function from one of the demo Platform bindings)
        io.DeltaTime = 1.0f/60.0f;              // set the time elapsed since the previous frame (in seconds)
        io.DisplaySize.x = 1920.0f;             // set the current display width
        io.DisplaySize.y = 1280.0f;             // set the current display height here
        io.MousePos = my_mouse_pos;             // set the mouse position
        io.MouseDown[0] = my_mouse_buttons[0];  // set the mouse button states
        io.MouseDown[1] = my_mouse_buttons[1];

        // Call NewFrame(), after this point you can use ImGui::* functions anytime
        // (So you want to try calling NewFrame() as early as you can in your mainloop to be able to use imgui everywhere)
        ImGui::NewFrame();

        // Most of your application code here
        ImGui::Text("Hello, world!");
        MyGameUpdate(); // may use any ImGui functions, e.g. ImGui::Begin("My window"); ImGui::Text("Hello, world!"); ImGui::End();
        MyGameRender(); // may use any ImGui functions as well!

        // Render imgui, swap buffers
        // (You want to try calling EndFrame/Render as late as you can, to be able to use imgui in your own game rendering code)
        ImGui::EndFrame();
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        MyImGuiRenderFunction(draw_data);
        SwapBuffers();
     }

     // Shutdown
     ImGui::DestroyContext();

 THIS HOW A SIMPLE RENDERING FUNCTION MAY LOOK LIKE

    void void MyImGuiRenderFunction(ImDrawData* draw_data)
    {
       // TODO: Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
       // TODO: Setup viewport using draw_data->DisplaySize
       // TODO: Setup orthographic projection matrix cover draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize
       // TODO: Setup shader: vertex { float2 pos, float2 uv, u32 color }, fragment shader sample color from 1 texture, multiply by vertex color.
       for (int n = 0; n < draw_data->CmdListsCount; n++)
       {
          const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;  // vertex buffer generated by ImGui
          const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;   // index buffer generated by ImGui
          for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
          {
             const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
             if (pcmd->UserCallback)
             {
                 pcmd->UserCallback(cmd_list, pcmd);
             }
             else
             {
                 // The texture for the draw call is specified by pcmd->TextureId.
                 // The vast majority of draw calls with use the imgui texture atlas, which value you have set yourself during initialization.
                 MyEngineBindTexture(pcmd->TextureId);

                 // We are using scissoring to clip some objects. All low-level graphics API should supports it.
                 // - If your engine doesn't support scissoring yet, you may ignore this at first. You will get some small glitches
                 //   (some elements visible outside their bounds) but you can fix that once everywhere else works!
                 // - Clipping coordinates are provided in imgui coordinates space (from draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize)
                 //   In a single viewport application, draw_data->DisplayPos will always be (0,0) and draw_data->DisplaySize will always be == io.DisplaySize.
                 //   However, in the interest of supporting multi-viewport applications in the future, always subtract draw_data->DisplayPos from
                 //   clipping bounds to convert them to your viewport space.
                 // - Note that pcmd->ClipRect contains Min+Max bounds. Some graphics API may use Min+Max, other may use Min+Size (size being Max-Min)
                 ImVec2 pos = draw_data->DisplayPos;
                 MyEngineScissor((int)(pcmd->ClipRect.x - pos.x), (int)(pcmd->ClipRect.y - pos.y), (int)(pcmd->ClipRect.z - pos.x), (int)(pcmd->ClipRect.w - pos.y));

                 // Render 'pcmd->ElemCount/3' indexed triangles.
                 // By default the indices ImDrawIdx are 16-bits, you can change them to 32-bits if your engine doesn't support 16-bits indices.
                 MyEngineDrawIndexedTriangles(pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer, vtx_buffer);
             }
             idx_buffer += pcmd->ElemCount;
          }
       }
    }

 - The examples/ folders contains many functional implementation of the pseudo-code above.
 - When calling NewFrame(), the 'io.WantCaptureMouse', 'io.WantCaptureKeyboard' and 'io.WantTextInput' flags are updated.
   They tell you if ImGui intends to use your inputs. When a flag is set you want to hide the corresponding inputs from the rest of your application.
   In both cases you need to pass on the inputs to imgui. Read the FAQ below for more information about those flags.
 - Please read the FAQ above. Amusingly, it is called a FAQ because people frequently have the same issues!

 USING GAMEPAD/KEYBOARD NAVIGATION CONTROLS

 - The gamepad/keyboard navigation is fairly functional and keeps being improved. 
 - You can ask questions and report issues at https://github.com/ocornut/imgui/issues/787
 - The initial focus was to support game controllers, but keyboard is becoming increasingly and decently usable.
 - Gamepad:
    - Set io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad to enable.
    - Backend: Set io.BackendFlags |= ImGuiBackendFlags_HasGamepad + fill the io.NavInputs[] fields before calling NewFrame().
      Note that io.NavInputs[] is cleared by EndFrame().
    - See 'enum ImGuiNavInput_' in imgui.h for a description of inputs. For each entry of io.NavInputs[], set the following values:
         0.0f= not held. 1.0f= fully held. Pass intermediate 0.0f..1.0f values for analog triggers/sticks.
    - We uses a simple >0.0f test for activation testing, and won't attempt to test for a dead-zone.
      Your code will probably need to transform your raw inputs (such as e.g. remapping your 0.2..0.9 raw input range to 0.0..1.0 imgui range, etc.).
    - You can download PNG/PSD files depicting the gamepad controls for common controllers at: http://goo.gl/9LgVZW.
    - If you need to share inputs between your game and the imgui parts, the easiest approach is to go all-or-nothing, with a buttons combo
      to toggle the target. Please reach out if you think the game vs navigation input sharing could be improved.
 - Keyboard:
    - Set io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard to enable.
      NewFrame() will automatically fill io.NavInputs[] based on your io.KeysDown[] + io.KeyMap[] arrays.
    - When keyboard navigation is active (io.NavActive + ImGuiConfigFlags_NavEnableKeyboard), the io.WantCaptureKeyboard flag
      will be set. For more advanced uses, you may want to read from:
       - io.NavActive: true when a window is focused and it doesn't have the ImGuiWindowFlags_NoNavInputs flag set.
       - io.NavVisible: true when the navigation cursor is visible (and usually goes false when mouse is used).
       - or query focus information with e.g. IsWindowFocused(ImGuiFocusedFlags_AnyWindow), IsItemFocused() etc. functions.
      Please reach out if you think the game vs navigation input sharing could be improved.
 - Mouse:
    - PS4 users: Consider emulating a mouse cursor with DualShock4 touch pad or a spare analog stick as a mouse-emulation fallback.
    - Consoles/Tablet/Phone users: Consider using a Synergy 1.x server (on your PC) + uSynergy.c (on your console/tablet/phone app) to share your PC mouse/keyboard.
    - On a TV/console system where readability may be lower or mouse inputs may be awkward, you may want to set the ImGuiConfigFlags_NavEnableSetMousePos flag.
      Enabling ImGuiConfigFlags_NavEnableSetMousePos + ImGuiBackendFlags_HasSetMousePos instructs dear imgui to move your mouse cursor along with navigation movements.
      When enabled, the NewFrame() function may alter 'io.MousePos' and set 'io.WantSetMousePos' to notify you that it wants the mouse cursor to be moved.
      When that happens your back-end NEEDS to move the OS or underlying mouse cursor on the next frame. Some of the binding in examples/ do that.
      (If you set the NavEnableSetMousePos flag but don't honor 'io.WantSetMousePos' properly, imgui will misbehave as it will see your mouse as moving back and forth!)
      (In a setup when you may not have easy control over the mouse cursor, e.g. uSynergy.c doesn't expose moving remote mouse cursor, you may want
       to set a boolean to ignore your other external mouse positions until the external source is moved again.)


 API BREAKING CHANGES
 ====================

 Occasionally introducing changes that are breaking the API. We try to make the breakage minor and easy to fix.
 Below is a change-log of API breaking changes only. If you are using one of the functions listed, expect to have to fix some code.
 When you are not sure about a old symbol or function name, try using the Search/Find function of your IDE to look for comments or references in all imgui files.
 You can read releases logs https://github.com/ocornut/imgui/releases for more details.

 - 2018/09/06 (1.65) - renamed stb_truetype.h to imstb_truetype.h, stb_textedit.h to imstb_textedit.h, and stb_rect_pack.h to imstb_rectpack.h. 
                       If you were conveniently using the imgui copy of those STB headers in your project you will have to update your include paths.
 - 2018/09/05 (1.65) - renamed io.OptCursorBlink/io.ConfigCursorBlink to io.ConfigInputTextCursorBlink. (#1427)
 - 2018/08/31 (1.64) - added imgui_widgets.cpp file, extracted and moved widgets code out of imgui.cpp into imgui_widgets.cpp. Re-ordered some of the code remaining in imgui.cpp.
                       NONE OF THE FUNCTIONS HAVE CHANGED. THE CODE IS SEMANTICALLY 100% IDENTICAL, BUT _EVERY_ FUNCTION HAS BEEN MOVED.
                       Because of this, any local modifications to imgui.cpp will likely conflict when you update. Read docs/CHANGELOG.txt for suggestions.
 - 2018/08/22 (1.63) - renamed IsItemDeactivatedAfterChange() to IsItemDeactivatedAfterEdit() for consistency with new IsItemEdited() API. Kept redirection function (will obsolete soonish as IsItemDeactivatedAfterChange() is very recent).
 - 2018/08/21 (1.63) - renamed ImGuiTextEditCallback to ImGuiInputTextCallback, ImGuiTextEditCallbackData to ImGuiInputTextCallbackData for consistency. Kept redirection types (will obsolete).
 - 2018/08/21 (1.63) - removed ImGuiInputTextCallbackData::ReadOnly since it is a duplication of (ImGuiInputTextCallbackData::Flags & ImGuiInputTextFlags_ReadOnly).
 - 2018/08/01 (1.63) - removed per-window ImGuiWindowFlags_ResizeFromAnySide beta flag in favor of a global io.ConfigResizeWindowsFromEdges to enable the feature.
 - 2018/08/01 (1.63) - renamed io.OptCursorBlink to io.ConfigCursorBlink [-> io.ConfigInputTextCursorBlink in 1.65], io.OptMacOSXBehaviors to ConfigMacOSXBehaviors for consistency.
 - 2018/07/22 (1.63) - changed ImGui::GetTime() return value from float to double to avoid accumulating floating point imprecisions over time.
 - 2018/07/08 (1.63) - style: renamed ImGuiCol_ModalWindowDarkening to ImGuiCol_ModalWindowDimBg for consistency with other features. Kept redirection enum (will obsolete).
 - 2018/06/06 (1.62) - renamed GetGlyphRangesChinese() to GetGlyphRangesChineseFull() to distinguish other variants and discourage using the full set.
 - 2018/06/06 (1.62) - TreeNodeEx()/TreeNodeBehavior(): the ImGuiTreeNodeFlags_CollapsingHeader helper now include the ImGuiTreeNodeFlags_NoTreePushOnOpen flag. See Changelog for details. 
 - 2018/05/03 (1.61) - DragInt(): the default compile-time format string has been changed from "%.0f" to "%d", as we are not using integers internally any more.
                       If you used DragInt() with custom format strings, make sure you change them to use %d or an integer-compatible format.
                       To honor backward-compatibility, the DragInt() code will currently parse and modify format strings to replace %*f with %d, giving time to users to upgrade their code.
                       If you have IMGUI_DISABLE_OBSOLETE_FUNCTIONS enabled, the code will instead assert! You may run a reg-exp search on your codebase for e.g. "DragInt.*%f" to help you find them.
 - 2018/04/28 (1.61) - obsoleted InputFloat() functions taking an optional "int decimal_precision" in favor of an equivalent and more flexible "const char* format",
                       consistent with other functions. Kept redirection functions (will obsolete).
 - 2018/04/09 (1.61) - IM_DELETE() helper function added in 1.60 doesn't clear the input _pointer_ reference, more consistent with expectation and allows passing r-value.
 - 2018/03/20 (1.60) - renamed io.WantMoveMouse to io.WantSetMousePos for consistency and ease of understanding (was added in 1.52, _not_ used by core and only honored by some binding ahead of merging the Nav branch).
 - 2018/03/12 (1.60) - removed ImGuiCol_CloseButton, ImGuiCol_CloseButtonActive, ImGuiCol_CloseButtonHovered as the closing cross uses regular button colors now.
 - 2018/03/08 (1.60) - changed ImFont::DisplayOffset.y to default to 0 instead of +1. Fixed rounding of Ascent/Descent to match TrueType renderer. If you were adding or subtracting to ImFont::DisplayOffset check if your fonts are correctly aligned vertically.
 - 2018/03/03 (1.60) - renamed ImGuiStyleVar_Count_ to ImGuiStyleVar_COUNT and ImGuiMouseCursor_Count_ to ImGuiMouseCursor_COUNT for consistency with other public enums.
 - 2018/02/18 (1.60) - BeginDragDropSource(): temporarily removed the optional mouse_button=0 parameter because it is not really usable in many situations at the moment.
 - 2018/02/16 (1.60) - obsoleted the io.RenderDrawListsFn callback, you can call your graphics engine render function after ImGui::Render(). Use ImGui::GetDrawData() to retrieve the ImDrawData* to display.
 - 2018/02/07 (1.60) - reorganized context handling to be more explicit,
                       - YOU NOW NEED TO CALL ImGui::CreateContext() AT THE BEGINNING OF YOUR APP, AND CALL ImGui::DestroyContext() AT THE END.
                       - removed Shutdown() function, as DestroyContext() serve this purpose.
                       - you may pass a ImFontAtlas* pointer to CreateContext() to share a font atlas between contexts. Otherwise CreateContext() will create its own font atlas instance.
                       - removed allocator parameters from CreateContext(), they are now setup with SetAllocatorFunctions(), and shared by all contexts.
                       - removed the default global context and font atlas instance, which were confusing for users of DLL reloading and users of multiple contexts.
 - 2018/01/31 (1.60) - moved sample TTF files from extra_fonts/ to misc/fonts/. If you loaded files directly from the imgui repo you may need to update your paths.
 - 2018/01/11 (1.60) - obsoleted IsAnyWindowHovered() in favor of IsWindowHovered(ImGuiHoveredFlags_AnyWindow). Kept redirection function (will obsolete).
 - 2018/01/11 (1.60) - obsoleted IsAnyWindowFocused() in favor of IsWindowFocused(ImGuiFocusedFlags_AnyWindow). Kept redirection function (will obsolete).
 - 2018/01/03 (1.60) - renamed ImGuiSizeConstraintCallback to ImGuiSizeCallback, ImGuiSizeConstraintCallbackData to ImGuiSizeCallbackData.
 - 2017/12/29 (1.60) - removed CalcItemRectClosestPoint() which was weird and not really used by anyone except demo code. If you need it it's easy to replicate on your side.
 - 2017/12/24 (1.53) - renamed the emblematic ShowTestWindow() function to ShowDemoWindow(). Kept redirection function (will obsolete).
 - 2017/12/21 (1.53) - ImDrawList: renamed style.AntiAliasedShapes to style.AntiAliasedFill for consistency and as a way to explicitly break code that manipulate those flag at runtime. You can now manipulate ImDrawList::Flags
 - 2017/12/21 (1.53) - ImDrawList: removed 'bool anti_aliased = true' final parameter of ImDrawList::AddPolyline() and ImDrawList::AddConvexPolyFilled(). Prefer manipulating ImDrawList::Flags if you need to toggle them during the frame.
 - 2017/12/14 (1.53) - using the ImGuiWindowFlags_NoScrollWithMouse flag on a child window forwards the mouse wheel event to the parent window, unless either ImGuiWindowFlags_NoInputs or ImGuiWindowFlags_NoScrollbar are also set.
 - 2017/12/13 (1.53) - renamed GetItemsLineHeightWithSpacing() to GetFrameHeightWithSpacing(). Kept redirection function (will obsolete).
 - 2017/12/13 (1.53) - obsoleted IsRootWindowFocused() in favor of using IsWindowFocused(ImGuiFocusedFlags_RootWindow). Kept redirection function (will obsolete).
                     - obsoleted IsRootWindowOrAnyChildFocused() in favor of using IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows). Kept redirection function (will obsolete).
 - 2017/12/12 (1.53) - renamed ImGuiTreeNodeFlags_AllowOverlapMode to ImGuiTreeNodeFlags_AllowItemOverlap. Kept redirection enum (will obsolete).
 - 2017/12/10 (1.53) - removed SetNextWindowContentWidth(), prefer using SetNextWindowContentSize(). Kept redirection function (will obsolete).
 - 2017/11/27 (1.53) - renamed ImGuiTextBuffer::append() helper to appendf(), appendv() to appendfv(). If you copied the 'Log' demo in your code, it uses appendv() so that needs to be renamed.
 - 2017/11/18 (1.53) - Style, Begin: removed ImGuiWindowFlags_ShowBorders window flag. Borders are now fully set up in the ImGuiStyle structure (see e.g. style.FrameBorderSize, style.WindowBorderSize). Use ImGui::ShowStyleEditor() to look them up.
                       Please note that the style system will keep evolving (hopefully stabilizing in Q1 2018), and so custom styles will probably subtly break over time. It is recommended you use the StyleColorsClassic(), StyleColorsDark(), StyleColorsLight() functions.
 - 2017/11/18 (1.53) - Style: removed ImGuiCol_ComboBg in favor of combo boxes using ImGuiCol_PopupBg for consistency.
 - 2017/11/18 (1.53) - Style: renamed ImGuiCol_ChildWindowBg to ImGuiCol_ChildBg.
 - 2017/11/18 (1.53) - Style: renamed style.ChildWindowRounding to style.ChildRounding, ImGuiStyleVar_ChildWindowRounding to ImGuiStyleVar_ChildRounding.
 - 2017/11/02 (1.53) - obsoleted IsRootWindowOrAnyChildHovered() in favor of using IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
 - 2017/10/24 (1.52) - renamed IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS/IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS to IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS/IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS for consistency.
 - 2017/10/20 (1.52) - changed IsWindowHovered() default parameters behavior to return false if an item is active in another window (e.g. click-dragging item from another window to this window). You can use the newly introduced IsWindowHovered() flags to requests this specific behavior if you need it.
 - 2017/10/20 (1.52) - marked IsItemHoveredRect()/IsMouseHoveringWindow() as obsolete, in favor of using the newly introduced flags for IsItemHovered() and IsWindowHovered(). See https://github.com/ocornut/imgui/issues/1382 for details.
                       removed the IsItemRectHovered()/IsWindowRectHovered() names introduced in 1.51 since they were merely more consistent names for the two functions we are now obsoleting.
 - 2017/10/17 (1.52) - marked the old 5-parameters version of Begin() as obsolete (still available). Use SetNextWindowSize()+Begin() instead!
 - 2017/10/11 (1.52) - renamed AlignFirstTextHeightToWidgets() to AlignTextToFramePadding(). Kept inline redirection function (will obsolete).
 - 2017/09/25 (1.52) - removed SetNextWindowPosCenter() because SetNextWindowPos() now has the optional pivot information to do the same and more. Kept redirection function (will obsolete).
 - 2017/08/25 (1.52) - io.MousePos needs to be set to ImVec2(-FLT_MAX,-FLT_MAX) when mouse is unavailable/missing. Previously ImVec2(-1,-1) was enough but we now accept negative mouse coordinates. In your binding if you need to support unavailable mouse, make sure to replace "io.MousePos = ImVec2(-1,-1)" with "io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX)".
 - 2017/08/22 (1.51) - renamed IsItemHoveredRect() to IsItemRectHovered(). Kept inline redirection function (will obsolete). -> (1.52) use IsItemHovered(ImGuiHoveredFlags_RectOnly)!
                     - renamed IsMouseHoveringAnyWindow() to IsAnyWindowHovered() for consistency. Kept inline redirection function (will obsolete).
                     - renamed IsMouseHoveringWindow() to IsWindowRectHovered() for consistency. Kept inline redirection function (will obsolete).
 - 2017/08/20 (1.51) - renamed GetStyleColName() to GetStyleColorName() for consistency.
 - 2017/08/20 (1.51) - added PushStyleColor(ImGuiCol idx, ImU32 col) overload, which _might_ cause an "ambiguous call" compilation error if you are using ImColor() with implicit cast. Cast to ImU32 or ImVec4 explicily to fix.
 - 2017/08/15 (1.51) - marked the weird IMGUI_ONCE_UPON_A_FRAME helper macro as obsolete. prefer using the more explicit ImGuiOnceUponAFrame.
 - 2017/08/15 (1.51) - changed parameter order for BeginPopupContextWindow() from (const char*,int buttons,bool also_over_items) to (const char*,int buttons,bool also_over_items). Note that most calls relied on default parameters completely.
 - 2017/08/13 (1.51) - renamed ImGuiCol_Columns*** to ImGuiCol_Separator***. Kept redirection enums (will obsolete).
 - 2017/08/11 (1.51) - renamed ImGuiSetCond_*** types and flags to ImGuiCond_***. Kept redirection enums (will obsolete).
 - 2017/08/09 (1.51) - removed ValueColor() helpers, they are equivalent to calling Text(label) + SameLine() + ColorButton().
 - 2017/08/08 (1.51) - removed ColorEditMode() and ImGuiColorEditMode in favor of ImGuiColorEditFlags and parameters to the various Color*() functions. The SetColorEditOptions() allows to initialize default but the user can still change them with right-click context menu.
                     - changed prototype of 'ColorEdit4(const char* label, float col[4], bool show_alpha = true)' to 'ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0)', where passing flags = 0x01 is a safe no-op (hello dodgy backward compatibility!). - check and run the demo window, under "Color/Picker Widgets", to understand the various new options.
                     - changed prototype of rarely used 'ColorButton(ImVec4 col, bool small_height = false, bool outline_border = true)' to 'ColorButton(const char* desc_id, ImVec4 col, ImGuiColorEditFlags flags = 0, ImVec2 size = ImVec2(0,0))'
 - 2017/07/20 (1.51) - removed IsPosHoveringAnyWindow(ImVec2), which was partly broken and misleading. ASSERT + redirect user to io.WantCaptureMouse
 - 2017/05/26 (1.50) - removed ImFontConfig::MergeGlyphCenterV in favor of a more multipurpose ImFontConfig::GlyphOffset.
 - 2017/05/01 (1.50) - renamed ImDrawList::PathFill() (rarely used directly) to ImDrawList::PathFillConvex() for clarity.
 - 2016/11/06 (1.50) - BeginChild(const char*) now applies the stack id to the provided label, consistently with other functions as it should always have been. It shouldn't affect you unless (extremely unlikely) you were appending multiple times to a same child from different locations of the stack id. If that's the case, generate an id with GetId() and use it instead of passing string to BeginChild().
 - 2016/10/15 (1.50) - avoid 'void* user_data' parameter to io.SetClipboardTextFn/io.GetClipboardTextFn pointers. We pass io.ClipboardUserData to it.
 - 2016/09/25 (1.50) - style.WindowTitleAlign is now a ImVec2 (ImGuiAlign enum was removed). set to (0.5f,0.5f) for horizontal+vertical centering, (0.0f,0.0f) for upper-left, etc.
 - 2016/07/30 (1.50) - SameLine(x) with x>0.0f is now relative to left of column/group if any, and not always to left of window. This was sort of always the intent and hopefully breakage should be minimal.
 - 2016/05/12 (1.49) - title bar (using ImGuiCol_TitleBg/ImGuiCol_TitleBgActive colors) isn't rendered over a window background (ImGuiCol_WindowBg color) anymore.
                       If your TitleBg/TitleBgActive alpha was 1.0f or you are using the default theme it will not affect you.
                       If your TitleBg/TitleBgActive alpha was <1.0f you need to tweak your custom theme to readjust for the fact that we don't draw a WindowBg background behind the title bar.
                       This helper function will convert an old TitleBg/TitleBgActive color into a new one with the same visual output, given the OLD color and the OLD WindowBg color.
                           ImVec4 ConvertTitleBgCol(const ImVec4& win_bg_col, const ImVec4& title_bg_col)
                           {
                               float new_a = 1.0f - ((1.0f - win_bg_col.w) * (1.0f - title_bg_col.w)), k = title_bg_col.w / new_a;
                               return ImVec4((win_bg_col.x * win_bg_col.w + title_bg_col.x) * k, (win_bg_col.y * win_bg_col.w + title_bg_col.y) * k, (win_bg_col.z * win_bg_col.w + title_bg_col.z) * k, new_a);
                           }
                       If this is confusing, pick the RGB value from title bar from an old screenshot and apply this as TitleBg/TitleBgActive. Or you may just create TitleBgActive from a tweaked TitleBg color.
 - 2016/05/07 (1.49) - removed confusing set of GetInternalState(), GetInternalStateSize(), SetInternalState() functions. Now using CreateContext(), DestroyContext(), GetCurrentContext(), SetCurrentContext().
 - 2016/05/02 (1.49) - renamed SetNextTreeNodeOpened() to SetNextTreeNodeOpen(), no redirection.
 - 2016/05/01 (1.49) - obsoleted old signature of CollapsingHeader(const char* label, const char* str_id = NULL, bool display_frame = true, bool default_open = false) as extra parameters were badly designed and rarely used. You can replace the "default_open = true" flag in new API with CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen).
 - 2016/04/26 (1.49) - changed ImDrawList::PushClipRect(ImVec4 rect) to ImDrawList::PushClipRect(Imvec2 min,ImVec2 max,bool intersect_with_current_clip_rect=false). Note that higher-level ImGui::PushClipRect() is preferable because it will clip at logic/widget level, whereas ImDrawList::PushClipRect() only affect your renderer.
 - 2016/04/03 (1.48) - removed style.WindowFillAlphaDefault setting which was redundant. Bake default BG alpha inside style.Colors[ImGuiCol_WindowBg] and all other Bg color values. (ref github issue #337).
 - 2016/04/03 (1.48) - renamed ImGuiCol_TooltipBg to ImGuiCol_PopupBg, used by popups/menus and tooltips. popups/menus were previously using ImGuiCol_WindowBg. (ref github issue #337)
 - 2016/03/21 (1.48) - renamed GetWindowFont() to GetFont(), GetWindowFontSize() to GetFontSize(). Kept inline redirection function (will obsolete).
 - 2016/03/02 (1.48) - InputText() completion/history/always callbacks: if you modify the text buffer manually (without using DeleteChars()/InsertChars() helper) you need to maintain the BufTextLen field. added an assert.
 - 2016/01/23 (1.48) - fixed not honoring exact width passed to PushItemWidth(), previously it would add extra FramePadding.x*2 over that width. if you had manual pixel-perfect alignment in place it might affect you.
 - 2015/12/27 (1.48) - fixed ImDrawList::AddRect() which used to render a rectangle 1 px too large on each axis.
 - 2015/12/04 (1.47) - renamed Color() helpers to ValueColor() - dangerously named, rarely used and probably to be made obsolete.
 - 2015/08/29 (1.45) - with the addition of horizontal scrollbar we made various fixes to inconsistencies with dealing with cursor position.
                       GetCursorPos()/SetCursorPos() functions now include the scrolled amount. It shouldn't affect the majority of users, but take note that SetCursorPosX(100.0f) puts you at +100 from the starting x position which may include scrolling, not at +100 from the window left side.
                       GetContentRegionMax()/GetWindowContentRegionMin()/GetWindowContentRegionMax() functions allow include the scrolled amount. Typically those were used in cases where no scrolling would happen so it may not be a problem, but watch out!
 - 2015/08/29 (1.45) - renamed style.ScrollbarWidth to style.ScrollbarSize
 - 2015/08/05 (1.44) - split imgui.cpp into extra files: imgui_demo.cpp imgui_draw.cpp imgui_internal.h that you need to add to your project.
 - 2015/07/18 (1.44) - fixed angles in ImDrawList::PathArcTo(), PathArcToFast() (introduced in 1.43) being off by an extra PI for no justifiable reason
 - 2015/07/14 (1.43) - add new ImFontAtlas::AddFont() API. For the old AddFont***, moved the 'font_no' parameter of ImFontAtlas::AddFont** functions to the ImFontConfig structure.
                       you need to render your textured triangles with bilinear filtering to benefit from sub-pixel positioning of text.
 - 2015/07/08 (1.43) - switched rendering data to use indexed rendering. this is saving a fair amount of CPU/GPU and enables us to get anti-aliasing for a marginal cost.
                       this necessary change will break your rendering function! the fix should be very easy. sorry for that :(
                     - if you are using a vanilla copy of one of the imgui_impl_XXXX.cpp provided in the example, you just need to update your copy and you can ignore the rest.
                     - the signature of the io.RenderDrawListsFn handler has changed!
                       old: ImGui_XXXX_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
                       new: ImGui_XXXX_RenderDrawLists(ImDrawData* draw_data).
                         argument:   'cmd_lists' becomes 'draw_data->CmdLists', 'cmd_lists_count' becomes 'draw_data->CmdListsCount'
                         ImDrawList: 'commands' becomes 'CmdBuffer', 'vtx_buffer' becomes 'VtxBuffer', 'IdxBuffer' is new.
                         ImDrawCmd:  'vtx_count' becomes 'ElemCount', 'clip_rect' becomes 'ClipRect', 'user_callback' becomes 'UserCallback', 'texture_id' becomes 'TextureId'.
                     - each ImDrawList now contains both a vertex buffer and an index buffer. For each command, render ElemCount/3 triangles using indices from the index buffer.
                     - if you REALLY cannot render indexed primitives, you can call the draw_data->DeIndexAllBuffers() method to de-index the buffers. This is slow and a waste of CPU/GPU. Prefer using indexed rendering!
                     - refer to code in the examples/ folder or ask on the GitHub if you are unsure of how to upgrade. please upgrade!
 - 2015/07/10 (1.43) - changed SameLine() parameters from int to float.
 - 2015/07/02 (1.42) - renamed SetScrollPosHere() to SetScrollFromCursorPos(). Kept inline redirection function (will obsolete).
 - 2015/07/02 (1.42) - renamed GetScrollPosY() to GetScrollY(). Necessary to reduce confusion along with other scrolling functions, because positions (e.g. cursor position) are not equivalent to scrolling amount.
 - 2015/06/14 (1.41) - changed ImageButton() default bg_col parameter from (0,0,0,1) (black) to (0,0,0,0) (transparent) - makes a difference when texture have transparence
 - 2015/06/14 (1.41) - changed Selectable() API from (label, selected, size) to (label, selected, flags, size). Size override should have been rarely be used. Sorry!
 - 2015/05/31 (1.40) - renamed GetWindowCollapsed() to IsWindowCollapsed() for consistency. Kept inline redirection function (will obsolete).
 - 2015/05/31 (1.40) - renamed IsRectClipped() to IsRectVisible() for consistency. Note that return value is opposite! Kept inline redirection function (will obsolete).
 - 2015/05/27 (1.40) - removed the third 'repeat_if_held' parameter from Button() - sorry! it was rarely used and inconsistent. Use PushButtonRepeat(true) / PopButtonRepeat() to enable repeat on desired buttons.
 - 2015/05/11 (1.40) - changed BeginPopup() API, takes a string identifier instead of a bool. ImGui needs to manage the open/closed state of popups. Call OpenPopup() to actually set the "open" state of a popup. BeginPopup() returns true if the popup is opened.
 - 2015/05/03 (1.40) - removed style.AutoFitPadding, using style.WindowPadding makes more sense (the default values were already the same).
 - 2015/04/13 (1.38) - renamed IsClipped() to IsRectClipped(). Kept inline redirection function until 1.50.
 - 2015/04/09 (1.38) - renamed ImDrawList::AddArc() to ImDrawList::AddArcFast() for compatibility with future API
 - 2015/04/03 (1.38) - removed ImGuiCol_CheckHovered, ImGuiCol_CheckActive, replaced with the more general ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive.
 - 2014/04/03 (1.38) - removed support for passing -FLT_MAX..+FLT_MAX as the range for a SliderFloat(). Use DragFloat() or Inputfloat() instead.
 - 2015/03/17 (1.36) - renamed GetItemBoxMin()/GetItemBoxMax()/IsMouseHoveringBox() to GetItemRectMin()/GetItemRectMax()/IsMouseHoveringRect(). Kept inline redirection function until 1.50.
 - 2015/03/15 (1.36) - renamed style.TreeNodeSpacing to style.IndentSpacing, ImGuiStyleVar_TreeNodeSpacing to ImGuiStyleVar_IndentSpacing
 - 2015/03/13 (1.36) - renamed GetWindowIsFocused() to IsWindowFocused(). Kept inline redirection function until 1.50.
 - 2015/03/08 (1.35) - renamed style.ScrollBarWidth to style.ScrollbarWidth (casing)
 - 2015/02/27 (1.34) - renamed OpenNextNode(bool) to SetNextTreeNodeOpened(bool, ImGuiSetCond). Kept inline redirection function until 1.50.
 - 2015/02/27 (1.34) - renamed ImGuiSetCondition_*** to ImGuiSetCond_***, and _FirstUseThisSession becomes _Once.
 - 2015/02/11 (1.32) - changed text input callback ImGuiTextEditCallback return type from void-->int. reserved for future use, return 0 for now.
 - 2015/02/10 (1.32) - renamed GetItemWidth() to CalcItemWidth() to clarify its evolving behavior
 - 2015/02/08 (1.31) - renamed GetTextLineSpacing() to GetTextLineHeightWithSpacing()
 - 2015/02/01 (1.31) - removed IO.MemReallocFn (unused)
 - 2015/01/19 (1.30) - renamed ImGuiStorage::GetIntPtr()/GetFloatPtr() to GetIntRef()/GetIntRef() because Ptr was conflicting with actual pointer storage functions.
 - 2015/01/11 (1.30) - big font/image API change! now loads TTF file. allow for multiple fonts. no need for a PNG loader.
              (1.30) - removed GetDefaultFontData(). uses io.Fonts->GetTextureData*() API to retrieve uncompressed pixels.
                       font init:  const void* png_data; unsigned int png_size; ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size); <..Upload texture to GPU..>
                       became:     unsigned char* pixels; int width, height; io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height); <..Upload texture to GPU>; io.Fonts->TexId = YourTextureIdentifier;
                       you now more flexibility to load multiple TTF fonts and manage the texture buffer for internal needs.
                       it is now recommended that you sample the font texture with bilinear interpolation.
              (1.30) - added texture identifier in ImDrawCmd passed to your render function (we can now render images). make sure to set io.Fonts->TexID.
              (1.30) - removed IO.PixelCenterOffset (unnecessary, can be handled in user projection matrix)
              (1.30) - removed ImGui::IsItemFocused() in favor of ImGui::IsItemActive() which handles all widgets
 - 2014/12/10 (1.18) - removed SetNewWindowDefaultPos() in favor of new generic API SetNextWindowPos(pos, ImGuiSetCondition_FirstUseEver)
 - 2014/11/28 (1.17) - moved IO.Font*** options to inside the IO.Font-> structure (FontYOffset, FontTexUvForWhite, FontBaseScale, FontFallbackGlyph)
 - 2014/11/26 (1.17) - reworked syntax of IMGUI_ONCE_UPON_A_FRAME helper macro to increase compiler compatibility
 - 2014/11/07 (1.15) - renamed IsHovered() to IsItemHovered()
 - 2014/10/02 (1.14) - renamed IMGUI_INCLUDE_IMGUI_USER_CPP to IMGUI_INCLUDE_IMGUI_USER_INL and imgui_user.cpp to imgui_user.inl (more IDE friendly)
 - 2014/09/25 (1.13) - removed 'text_end' parameter from IO.SetClipboardTextFn (the string is now always zero-terminated for simplicity)
 - 2014/09/24 (1.12) - renamed SetFontScale() to SetWindowFontScale()
 - 2014/09/24 (1.12) - moved IM_MALLOC/IM_REALLOC/IM_FREE preprocessor defines to IO.MemAllocFn/IO.MemReallocFn/IO.MemFreeFn
 - 2014/08/30 (1.09) - removed IO.FontHeight (now computed automatically)
 - 2014/08/30 (1.09) - moved IMGUI_FONT_TEX_UV_FOR_WHITE preprocessor define to IO.FontTexUvForWhite
 - 2014/08/28 (1.09) - changed the behavior of IO.PixelCenterOffset following various rendering fixes


 FREQUENTLY ASKED QUESTIONS (FAQ), TIPS
 ======================================

 Q: How can I tell whether to dispatch mouse/keyboard to imgui or to my application?
 A: You can read the 'io.WantCaptureMouse', 'io.WantCaptureKeyboard' and 'io.WantTextInput' flags from the ImGuiIO structure (e.g. if (ImGui::GetIO().WantCaptureMouse) { ... } )
    - When 'io.WantCaptureMouse' is set, imgui wants to use your mouse state, and you may want to discard/hide the inputs from the rest of your application.
    - When 'io.WantCaptureKeyboard' is set, imgui wants to use your keyboard state, and you may want to discard/hide the inputs from the rest of your application.
    - When 'io.WantTextInput' is set to may want to notify your OS to popup an on-screen keyboard, if available (e.g. on a mobile phone, or console OS).
    Note: you should always pass your mouse/keyboard inputs to imgui, even when the io.WantCaptureXXX flag are set false.
     This is because imgui needs to detect that you clicked in the void to unfocus its windows.
    Note: The 'io.WantCaptureMouse' is more accurate that any attempt to "check if the mouse is hovering a window" (don't do that!).
     It handle mouse dragging correctly (both dragging that started over your application or over an imgui window) and handle e.g. modal windows blocking inputs.
     Those flags are updated by ImGui::NewFrame(). Preferably read the flags after calling NewFrame() if you can afford it, but reading them before is also
     perfectly fine, as the bool toggle fairly rarely. If you have on a touch device, you might find use for an early call to UpdateHoveredWindowAndCaptureFlags().
    Note: Text input widget releases focus on "Return KeyDown", so the subsequent "Return KeyUp" event that your application receive will typically
     have 'io.WantCaptureKeyboard=false'. Depending on your application logic it may or not be inconvenient. You might want to track which key-downs
     were targeted for Dear ImGui, e.g. with an array of bool, and filter out the corresponding key-ups.)

 Q: How can I display an image? What is ImTextureID, how does it works?
 A: Short explanation:
    - You may use functions such as ImGui::Image(), ImGui::ImageButton() or lower-level ImDrawList::AddImage() to emit draw calls that will use your own textures.
    - Actual textures are identified in a way that is up to the user/engine.
    - Loading image files from the disk and turning them into a texture is not within the scope of Dear ImGui (for a good reason). 
      Please read documentations or tutorials on your graphics API to understand how to display textures on the screen before moving onward.

    Long explanation:
    - Dear ImGui's job is to create "meshes", defined in a renderer-agnostic format made of draw commands and vertices.
      At the end of the frame those meshes (ImDrawList) will be displayed by your rendering function. They are made up of textured polygons and the code
      to render them is generally fairly short (a few dozen lines). In the examples/ folder we provide functions for popular graphics API (OpenGL, DirectX, etc.).
    - Each rendering function decides on a data type to represent "textures". The concept of what is a "texture" is entirely tied to your underlying engine/graphics API.
      We carry the information to identify a "texture" in the ImTextureID type. 
      ImTextureID is nothing more that a void*, aka 4/8 bytes worth of data: just enough to store 1 pointer or 1 integer of your choice.
      Dear ImGui doesn't know or understand what you are storing in ImTextureID, it merely pass ImTextureID values until they reach your rendering function.
    - In the examples/ bindings, for each graphics API binding we decided on a type that is likely to be a good representation for specifying 
      an image from the end-user perspective. This is what the _examples_ rendering functions are using:

         OpenGL:     ImTextureID = GLuint                       (see ImGui_ImplGlfwGL3_RenderDrawData() function in imgui_impl_glfw_gl3.cpp)
         DirectX9:   ImTextureID = LPDIRECT3DTEXTURE9           (see ImGui_ImplDX9_RenderDrawData()     function in imgui_impl_dx9.cpp)
         DirectX11:  ImTextureID = ID3D11ShaderResourceView*    (see ImGui_ImplDX11_RenderDrawData()    function in imgui_impl_dx11.cpp)
         DirectX12:  ImTextureID = D3D12_GPU_DESCRIPTOR_HANDLE  (see ImGui_ImplDX12_RenderDrawData()    function in imgui_impl_dx12.cpp)

      For example, in the OpenGL example binding we store raw OpenGL texture identifier (GLuint) inside ImTextureID. 
      Whereas in the DirectX11 example binding we store a pointer to ID3D11ShaderResourceView inside ImTextureID, which is a higher-level structure 
      tying together both the texture and information about its format and how to read it.
    - If you have a custom engine built over e.g. OpenGL, instead of passing GLuint around you may decide to use a high-level data type to carry information about
      the texture as well as how to display it (shaders, etc.). The decision of what to use as ImTextureID can always be made better knowing how your codebase
      is designed. If your engine has high-level data types for "textures" and "material" then you may want to use them.
      If you are starting with OpenGL or DirectX or Vulkan and haven't built much of a rendering engine over them, keeping the default ImTextureID 
      representation suggested by the example bindings is probably the best choice.
      (Advanced users may also decide to keep a low-level type in ImTextureID, and use ImDrawList callback and pass information to their renderer)

    User code may do:

        // Cast our texture type to ImTextureID / void*
        MyTexture* texture = g_CoffeeTableTexture;
        ImGui::Image((void*)texture, ImVec2(texture->Width, texture->Height)); 

    The renderer function called after ImGui::Render() will receive that same value that the user code passed:

        // Cast ImTextureID / void* stored in the draw command as our texture type
        MyTexture* texture = (MyTexture*)pcmd->TextureId;
        MyEngineBindTexture2D(texture);

    Once you understand this design you will understand that loading image files and turning them into displayable textures is not within the scope of Dear ImGui.
    This is by design and is actually a good thing, because it means your code has full control over your data types and how you display them.
    If you want to display an image file (e.g. PNG file) into the screen, please refer to documentation and tutorials for the graphics API you are using.

    Here's a simplified OpenGL example using stb_image.h:

        // Use stb_image.h to load a PNG from disk and turn it into raw RGBA pixel data:
        #define STB_IMAGE_IMPLEMENTATION
        #include <stb_image.h>
        [...]
        int my_image_width, my_image_height;
        unsigned char* my_image_data = stbi_load("my_image.png", &my_image_width, &my_image_height, NULL, 4);

        // Turn the RGBA pixel data into an OpenGL texture:
        GLuint my_opengl_texture;
        glGenTextures(1, &my_opengl_texture);
        glBindTexture(GL_TEXTURE_2D, my_opengl_texture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

        // Now that we have an OpenGL texture, assuming our imgui rendering function (imgui_impl_xxx.cpp file) takes GLuint as ImTextureID, we can display it:
        ImGui::Image((void*)(intptr_t)my_opengl_texture, ImVec2(my_image_width, my_image_height));

    C/C++ tip: a void* is pointer-sized storage. You may safely store any pointer or integer into it by casting your value to ImTexture / void*, and vice-versa. 
    Because both end-points (user code and rendering function) are under your control, you know exactly what is stored inside the ImTexture / void*.
    Examples:

        GLuint my_tex = XXX;
        void* my_void_ptr;
        my_void_ptr = (void*)(intptr_t)my_tex;                  // cast a GLuint into a void* (we don't take its address! we literally store the value inside the pointer)
        my_tex = (GLuint)(intptr_t)my_void_ptr;                 // cast a void* into a GLuint

        ID3D11ShaderResourceView* my_dx11_srv = XXX;
        void* my_void_ptr;
        my_void_ptr = (void*)my_dx11_srv;                       // cast a ID3D11ShaderResourceView* into an opaque void*
        my_dx11_srv = (ID3D11ShaderResourceView*)my_void_ptr;   // cast a void* into a ID3D11ShaderResourceView*

    Finally, you may call ImGui::ShowMetricsWindow() to explore/visualize/understand how the ImDrawList are generated.

 Q: How can I have multiple widgets with the same label or without a label?
 Q: I have multiple widgets with the same label, and only the first one works. Why is that?
 A: A primer on labels and the ID Stack...

    Dear ImGui internally need to uniquely identify UI elements.
    Elements that are typically not clickable (such as calls to the Text functions) don't need an ID.
    Interactive widgets (such as calls to Button buttons) need a unique ID. 
    Unique ID are used internally to track active widgets and occasionally associate state to widgets.
    Unique ID are implicitly built from the hash of multiple elements that identify the "path" to the UI element.

   - Unique ID are often derived from a string label:

       Button("OK");          // Label = "OK",     ID = hash of (..., "OK")
       Button("Cancel");      // Label = "Cancel", ID = hash of (..., "Cancel")

   - ID are uniquely scoped within windows, tree nodes, etc. which all pushes to the ID stack. Having
     two buttons labeled "OK" in different windows or different tree locations is fine.
     We used "..." above to signify whatever was already pushed to the ID stack previously:

       Begin("MyWindow");
       Button("OK");          // Label = "OK",     ID = hash of ("MyWindow", "OK")
       End();

   - If you have a same ID twice in the same location, you'll have a conflict:

       Button("OK");
       Button("OK");          // ID collision! Interacting with either button will trigger the first one.

     Fear not! this is easy to solve and there are many ways to solve it!

   - Solving ID conflict in a simple/local context:
     When passing a label you can optionally specify extra ID information within string itself.
     Use "##" to pass a complement to the ID that won't be visible to the end-user.
     This helps solving the simple collision cases when you know e.g. at compilation time which items
     are going to be created:

       Begin("MyWindow");
       Button("Play");        // Label = "Play",   ID = hash of ("MyWindow", "Play")
       Button("Play##foo1");  // Label = "Play",   ID = hash of ("MyWindow", "Play##foo1")  // Different from above
       Button("Play##foo2");  // Label = "Play",   ID = hash of ("MyWindow", "Play##foo2")  // Different from above
       End();

   - If you want to completely hide the label, but still need an ID:

       Checkbox("##On", &b);  // Label = "",       ID = hash of (..., "##On")   // No visible label!

   - Occasionally/rarely you might want change a label while preserving a constant ID. This allows
     you to animate labels. For example you may want to include varying information in a window title bar,
     but windows are uniquely identified by their ID. Use "###" to pass a label that isn't part of ID:

       Button("Hello###ID";   // Label = "Hello",  ID = hash of (..., "ID")
       Button("World###ID";   // Label = "World",  ID = hash of (..., "ID")     // Same as above, even though the label looks different

       sprintf(buf, "My game (%f FPS)###MyGame", fps);
       Begin(buf);            // Variable label,   ID = hash of "MyGame"

   - Solving ID conflict in a more general manner:
     Use PushID() / PopID() to create scopes and manipulate the ID stack, as to avoid ID conflicts
     within the same window. This is the most convenient way of distinguishing ID when iterating and
     creating many UI elements programmatically.
     You can push a pointer, a string or an integer value into the ID stack.
     Remember that ID are formed from the concatenation of _everything_ in the ID stack!

       Begin("Window");
       for (int i = 0; i < 100; i++)
       {
         PushID(i);         // Push i to the id tack
         Button("Click");   // Label = "Click",  ID = Hash of ("Window", i, "Click")
         PopID();
       }
       for (int i = 0; i < 100; i++)
       {
         MyObject* obj = Objects[i];
         PushID(obj);
         Button("Click");   // Label = "Click",  ID = Hash of ("Window", obj pointer, "Click")
         PopID();
       }
       for (int i = 0; i < 100; i++)
       {
         MyObject* obj = Objects[i];
         PushID(obj->Name);
         Button("Click");   // Label = "Click",  ID = Hash of ("Window", obj->Name, "Click")
         PopID();
       }
       End();

   - More example showing that you can stack multiple prefixes into the ID stack:

       Button("Click");     // Label = "Click",  ID = hash of (..., "Click")
       PushID("node");
       Button("Click");     // Label = "Click",  ID = hash of (..., "node", "Click")
         PushID(my_ptr);
           Button("Click"); // Label = "Click",  ID = hash of (..., "node", my_ptr, "Click")
         PopID();
       PopID();

   - Tree nodes implicitly creates a scope for you by calling PushID().

       Button("Click");     // Label = "Click",  ID = hash of (..., "Click")
       if (TreeNode("node"))
       {
         Button("Click");   // Label = "Click",  ID = hash of (..., "node", "Click")
         TreePop();
       }

   - When working with trees, ID are used to preserve the open/close state of each tree node.
     Depending on your use cases you may want to use strings, indices or pointers as ID.
      e.g. when following a single pointer that may change over time, using a static string as ID
       will preserve your node open/closed state when the targeted object change.
      e.g. when displaying a list of objects, using indices or pointers as ID will preserve the
       node open/closed state differently. See what makes more sense in your situation!

 Q: How can I use my own math types instead of ImVec2/ImVec4? 
 A: You can edit imconfig.h and setup the IM_VEC2_CLASS_EXTRA/IM_VEC4_CLASS_EXTRA macros to add implicit type conversions.
    This way you'll be able to use your own types everywhere, e.g. passsing glm::vec2 to ImGui functions instead of ImVec2.

 Q: How can I load a different font than the default?
 A: Use the font atlas to load the TTF/OTF file you want:
      ImGuiIO& io = ImGui::GetIO();
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels);
      io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()
    (default is ProggyClean.ttf, rendered at size 13, embedded in dear imgui's source code)

    New programmers: remember that in C/C++ and most programming languages if you want to use a
    backslash \ within a string literal, you need to write it double backslash "\\":
      io.Fonts->AddFontFromFileTTF("MyDataFolder\MyFontFile.ttf", size_in_pixels);   // WRONG (you are escape the M here!)
      io.Fonts->AddFontFromFileTTF("MyDataFolder\\MyFontFile.ttf", size_in_pixels);  // CORRECT
      io.Fonts->AddFontFromFileTTF("MyDataFolder/MyFontFile.ttf", size_in_pixels);   // ALSO CORRECT

 Q: How can I easily use icons in my application?
 A: The most convenient and practical way is to merge an icon font such as FontAwesome inside you
    main font. Then you can refer to icons within your strings. Read 'How can I load multiple fonts?'
    and the file 'misc/fonts/README.txt' for instructions and useful header files.

 Q: How can I load multiple fonts?
 A: Use the font atlas to pack them into a single texture:
    (Read misc/fonts/README.txt and the code in ImFontAtlas for more details.)

      ImGuiIO& io = ImGui::GetIO();
      ImFont* font0 = io.Fonts->AddFontDefault();
      ImFont* font1 = io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels);
      ImFont* font2 = io.Fonts->AddFontFromFileTTF("myfontfile2.ttf", size_in_pixels);
      io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()
      // the first loaded font gets used by default
      // use ImGui::PushFont()/ImGui::PopFont() to change the font at runtime

      // Options
      ImFontConfig config;
      config.OversampleH = 3;
      config.OversampleV = 1;
      config.GlyphOffset.y -= 2.0f;      // Move everything by 2 pixels up
      config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
      io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, &config);

      // Combine multiple fonts into one (e.g. for icon fonts)
      ImWchar ranges[] = { 0xf000, 0xf3ff, 0 };
      ImFontConfig config;
      config.MergeMode = true;
      io.Fonts->AddFontDefault();
      io.Fonts->LoadFromFileTTF("fontawesome-webfont.ttf", 16.0f, &config, ranges); // Merge icon font
      io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, NULL, &config, io.Fonts->GetGlyphRangesJapanese()); // Merge japanese glyphs

 Q: How can I display and input non-Latin characters such as Chinese, Japanese, Korean, Cyrillic?
 A: When loading a font, pass custom Unicode ranges to specify the glyphs to load.

      // Add default Japanese ranges
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, io.Fonts->GetGlyphRangesJapanese());

      // Or create your own custom ranges (e.g. for a game you can feed your entire game script and only build the characters the game need)
      ImVector<ImWchar> ranges;
      ImFontAtlas::GlyphRangesBuilder builder;
      builder.AddText("Hello world");                        // Add a string (here "Hello world" contains 7 unique characters)
      builder.AddChar(0x7262);                               // Add a specific character
      builder.AddRanges(io.Fonts->GetGlyphRangesJapanese()); // Add one of the default ranges
      builder.BuildRanges(&ranges);                          // Build the final result (ordered ranges with all the unique characters submitted)
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, ranges.Data);

    All your strings needs to use UTF-8 encoding. In C++11 you can encode a string literal in UTF-8
    by using the u8"hello" syntax. Specifying literal in your source code using a local code page
    (such as CP-923 for Japanese or CP-1251 for Cyrillic) will NOT work!
    Otherwise you can convert yourself to UTF-8 or load text data from file already saved as UTF-8.

    Text input: it is up to your application to pass the right character code by calling io.AddInputCharacter(). 
    The applications in examples/ are doing that. 
    Windows: you can use the WM_CHAR or WM_UNICHAR or WM_IME_CHAR message (depending if your app is built using Unicode or MultiByte mode).
    You may also use MultiByteToWideChar() or ToUnicode() to retrieve Unicode codepoints from MultiByte characters or keyboard state.
    Windows: if your language is relying on an Input Method Editor (IME), you copy the HWND of your window to io.ImeWindowHandle in order for 
    the default implementation of io.ImeSetInputScreenPosFn() to set your Microsoft IME position correctly.

 Q: How can I use the drawing facilities without an ImGui window? (using ImDrawList API)
 A: - You can create a dummy window. Call SetNextWindowBgAlpha(0.0f), call Begin() with NoTitleBar|NoResize|NoMove|NoScrollbar|NoSavedSettings|NoInputs flags.
      Then you can retrieve the ImDrawList* via GetWindowDrawList() and draw to it in any way you like.
    - You can call ImGui::GetOverlayDrawList() and use this draw list to display contents over every other imgui windows.
    - You can create your own ImDrawList instance. You'll need to initialize them ImGui::GetDrawListSharedData(), or create your own ImDrawListSharedData.

 Q: I integrated Dear ImGui in my engine and the text or lines are blurry..
 A: In your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f).
    Also make sure your orthographic projection matrix and io.DisplaySize matches your actual framebuffer dimension.

 Q: I integrated Dear ImGui in my engine and some elements are clipping or disappearing when I move windows around..
 A: You are probably mishandling the clipping rectangles in your render function.
    Rectangles provided by ImGui are defined as (x1=left,y1=top,x2=right,y2=bottom) and NOT as (x1,y1,width,height).

 Q: How can I help?
 A: - If you are experienced with Dear ImGui and C++, look at the github issues, or docs/TODO.txt and see how you want/can help!
    - Convince your company to fund development time! Individual users: you can also become a Patron (patreon.com/imgui) or donate on PayPal! See README.
    - Disclose your usage of dear imgui via a dev blog post, a tweet, a screenshot, a mention somewhere etc.
      You may post screenshot or links in the gallery threads (github.com/ocornut/imgui/issues/1269). Visuals are ideal as they inspire other programmers.
      But even without visuals, disclosing your use of dear imgui help the library grow credibility, and help other teams and programmers with taking decisions.
    - If you have issues or if you need to hack into the library, even if you don't expect any support it is useful that you share your issues (on github or privately).

 - tip: you can call Begin() multiple times with the same name during the same frame, it will keep appending to the same window.
        this is also useful to set yourself in the context of another window (to get/set other settings)
 - tip: you can create widgets without a Begin()/End() block, they will go in an implicit window called "Debug".
 - tip: the ImGuiOnceUponAFrame helper will allow run the block of code only once a frame. You can use it to quickly add custom UI in the middle
        of a deep nested inner loop in your code.
 - tip: you can call Render() multiple times (e.g for VR renders).
 - tip: call and read the ShowDemoWindow() code in imgui_demo.cpp for more example of how to use ImGui!

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <ctype.h>      // toupper, isprint
#include <stdio.h>      // vsnprintf, sscanf, printf
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

#define IMGUI_DEBUG_NAV_SCORING     0
#define IMGUI_DEBUG_NAV_RECTS       0

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127) // condition expression is constant
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-pragmas"        // warning : unknown warning group '-Wformat-pedantic *'        // not all warnings are known by all clang versions.. so ignoring warnings triggers new warnings on some configuration. great!
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // warning : declaration requires an exit-time destructor       // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference it.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#pragma clang diagnostic ignored "-Wformat-pedantic"        // warning : format specifies type 'void *' but the argument has type 'xxxx *' // unreasonable, would lead to casting every %p arg to void*. probably enabled by -pedantic.
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast" // warning : cast to 'void *' from smaller integer type 'int'
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat"                   // warning: format '%p' expects argument of type 'void*', but argument 6 has type 'ImGuiWindow*'
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wstrict-overflow"          // warning: assuming signed overflow does not occur when assuming that (X - c) > X is always false
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif
#endif

// When using CTRL+TAB (or Gamepad Square+L/R) we delay the visual a little in order to reduce visual noise doing a fast switch.
static const float NAV_WINDOWING_HIGHLIGHT_DELAY   = 0.20f; // Time before the highlight and screen dimming starts fading in
static const float NAV_WINDOWING_LIST_APPEAR_DELAY = 0.15f; // Time before the window list starts to appear

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------

static void             SetCurrentWindow(ImGuiWindow* window);
static void             SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiCond cond);
static void             SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiCond cond);
static void             SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiCond cond);
static void             FindHoveredWindow();
static ImGuiWindow*     CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags);
static void             CheckStacksSize(ImGuiWindow* window, bool write);
static ImVec2           CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window, bool snap_on_edges);

static void             AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list);
static void             AddWindowToDrawData(ImVector<ImDrawList*>* out_list, ImGuiWindow* window);
static void             AddWindowToSortedBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window);

static ImRect           GetViewportRect();

// Settings
static void*            SettingsHandlerWindow_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void             SettingsHandlerWindow_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void             SettingsHandlerWindow_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);

// Platform Dependents default implementation for IO functions
static const char*      GetClipboardTextFn_DefaultImpl(void* user_data);
static void             SetClipboardTextFn_DefaultImpl(void* user_data, const char* text);
static void             ImeSetInputScreenPosFn_DefaultImpl(int x, int y);

namespace ImGui
{
static bool             BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags);

// Navigation
static void             NavUpdate();
static void             NavUpdateWindowing();
static void             NavUpdateWindowingList();
static void             NavUpdateMoveResult();
static float            NavUpdatePageUpPageDown(int allowed_dir_flags);
static inline void      NavUpdateAnyRequestFlag();
static void             NavProcessItem(ImGuiWindow* window, const ImRect& nav_bb, const ImGuiID id);
static ImVec2           NavCalcPreferredRefPos();
static void             NavSaveLastChildNavWindow(ImGuiWindow* nav_window);
static ImGuiWindow*     NavRestoreLastChildNavWindow(ImGuiWindow* window);

// Misc
static void             UpdateMouseInputs();
static void             UpdateMouseWheel();
static void             UpdateManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4]);
}

//-----------------------------------------------------------------------------
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
//-----------------------------------------------------------------------------

// Current context pointer. Implicitly used by all ImGui functions. Always assumed to be != NULL.
// CreateContext() will automatically set this pointer if it is NULL. Change to a different context by calling ImGui::SetCurrentContext().
// If you use DLL hotreloading you might need to call SetCurrentContext() after reloading code from this file.
// ImGui functions are not thread-safe because of this pointer. If you want thread-safety to allow N threads to access N different contexts, you can:
// - Change this variable to use thread local storage. You may #define GImGui in imconfig.h for that purpose. Future development aim to make this context pointer explicit to all calls. Also read https://github.com/ocornut/imgui/issues/586
// - Having multiple instances of the ImGui code compiled inside different namespace (easiest/safest, if you have a finite number of contexts)
#ifndef GImGui
ImGuiContext*   GImGui = NULL;
#endif

// Memory Allocator functions. Use SetAllocatorFunctions() to change them.
// If you use DLL hotreloading you might need to call SetAllocatorFunctions() after reloading code from this file.
// Otherwise, you probably don't want to modify them mid-program, and if you use global/static e.g. ImVector<> instances you may need to keep them accessible during program destruction.
#ifndef IMGUI_DISABLE_DEFAULT_ALLOCATORS
static void*   MallocWrapper(size_t size, void* user_data)    { (void)user_data; return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { (void)user_data; free(ptr); }
#else
static void*   MallocWrapper(size_t size, void* user_data)    { (void)user_data; (void)size; IM_ASSERT(0); return NULL; }
static void    FreeWrapper(void* ptr, void* user_data)        { (void)user_data; (void)ptr; IM_ASSERT(0); }
#endif

static void*  (*GImAllocatorAllocFunc)(size_t size, void* user_data) = MallocWrapper;
static void   (*GImAllocatorFreeFunc)(void* ptr, void* user_data) = FreeWrapper;
static void*    GImAllocatorUserData = NULL;

//-----------------------------------------------------------------------------
// [SECTION] MAIN USER FACING STRUCTURES (ImGuiStyle, ImGuiIO)
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;             // Global alpha applies to everything in ImGui
    WindowPadding           = ImVec2(8,8);      // Padding within a window
    WindowRounding          = 7.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    WindowBorderSize        = 1.0f;             // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    WindowMinSize           = ImVec2(32,32);    // Minimum window size
    WindowTitleAlign        = ImVec2(0.0f,0.5f);// Alignment for title bar text
    ChildRounding           = 0.0f;             // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
    ChildBorderSize         = 1.0f;             // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    PopupRounding           = 0.0f;             // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
    PopupBorderSize         = 1.0f;             // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    FramePadding            = ImVec2(4,3);      // Padding within a framed rectangle (used by most widgets)
    FrameRounding           = 0.0f;             // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
    FrameBorderSize         = 0.0f;             // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
    ItemSpacing             = ImVec2(8,4);      // Horizontal and vertical spacing between widgets/lines
    ItemInnerSpacing        = ImVec2(4,4);      // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    TouchExtraPadding       = ImVec2(0,0);      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
    IndentSpacing           = 21.0f;            // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
    ColumnsMinSpacing       = 6.0f;             // Minimum horizontal spacing between two columns
    ScrollbarSize           = 16.0f;            // Width of the vertical scrollbar, Height of the horizontal scrollbar
    ScrollbarRounding       = 9.0f;             // Radius of grab corners rounding for scrollbar
    GrabMinSize             = 10.0f;            // Minimum width/height of a grab box for slider/scrollbar
    GrabRounding            = 0.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    ButtonTextAlign         = ImVec2(0.5f,0.5f);// Alignment of button text when button is larger than text.
    DisplayWindowPadding    = ImVec2(20,20);    // Window position are clamped to be visible within the display area by at least this amount. Only applies to regular windows.
    DisplaySafeAreaPadding  = ImVec2(3,3);      // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
    MouseCursorScale        = 1.0f;             // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
    AntiAliasedLines        = true;             // Enable anti-aliasing on lines/borders. Disable if you are really short on CPU/GPU.
    AntiAliasedFill         = true;             // Enable anti-aliasing on filled shapes (rounded rectangles, circles, etc.)
    CurveTessellationTol    = 1.25f;            // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.

    // Default theme
    ImGui::StyleColorsDark(this);
}

// To scale your entire UI (e.g. if you want your app to use High DPI or generally be DPI aware) you may use this helper function. Scaling the fonts is done separately and is up to you.
// Important: This operation is lossy because we round all sizes to integer. If you need to change your scale multiples, call this over a freshly initialized ImGuiStyle structure rather than scaling multiple times.
void ImGuiStyle::ScaleAllSizes(float scale_factor)
{
    WindowPadding = ImFloor(WindowPadding * scale_factor);
    WindowRounding = ImFloor(WindowRounding * scale_factor);
    WindowMinSize = ImFloor(WindowMinSize * scale_factor);
    ChildRounding = ImFloor(ChildRounding * scale_factor);
    PopupRounding = ImFloor(PopupRounding * scale_factor);
    FramePadding = ImFloor(FramePadding * scale_factor);
    FrameRounding = ImFloor(FrameRounding * scale_factor);
    ItemSpacing = ImFloor(ItemSpacing * scale_factor);
    ItemInnerSpacing = ImFloor(ItemInnerSpacing * scale_factor);
    TouchExtraPadding = ImFloor(TouchExtraPadding * scale_factor);
    IndentSpacing = ImFloor(IndentSpacing * scale_factor);
    ColumnsMinSpacing = ImFloor(ColumnsMinSpacing * scale_factor);
    ScrollbarSize = ImFloor(ScrollbarSize * scale_factor);
    ScrollbarRounding = ImFloor(ScrollbarRounding * scale_factor);
    GrabMinSize = ImFloor(GrabMinSize * scale_factor);
    GrabRounding = ImFloor(GrabRounding * scale_factor);
    DisplayWindowPadding = ImFloor(DisplayWindowPadding * scale_factor);
    DisplaySafeAreaPadding = ImFloor(DisplaySafeAreaPadding * scale_factor);
    MouseCursorScale = ImFloor(MouseCursorScale * scale_factor);
}

ImGuiIO::ImGuiIO()
{
    // Most fields are initialized with zero
    memset(this, 0, sizeof(*this));

    // Settings
    ConfigFlags = 0x00;
    BackendFlags = 0x00;
    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f/60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini";
    LogFilename = "imgui_log.txt";
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    for (int i = 0; i < ImGuiKey_COUNT; i++)
        KeyMap[i] = -1;
    KeyRepeatDelay = 0.250f;
    KeyRepeatRate = 0.050f;
    UserData = NULL;

    Fonts = NULL;
    FontGlobalScale = 1.0f;
    FontDefault = NULL;
    FontAllowUserScaling = false;
    DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    DisplayVisibleMin = DisplayVisibleMax = ImVec2(0.0f, 0.0f);

    // Miscellaneous configuration options
#ifdef __APPLE__
    ConfigMacOSXBehaviors = true;  // Set Mac OS X style defaults based on __APPLE__ compile time flag
#else
    ConfigMacOSXBehaviors = false;
#endif
    ConfigInputTextCursorBlink = true;
    ConfigResizeWindowsFromEdges = false;

    // Settings (User Functions)
    GetClipboardTextFn = GetClipboardTextFn_DefaultImpl;   // Platform dependent default implementations
    SetClipboardTextFn = SetClipboardTextFn_DefaultImpl;
    ClipboardUserData = NULL;
    ImeSetInputScreenPosFn = ImeSetInputScreenPosFn_DefaultImpl;
    ImeWindowHandle = NULL;

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    RenderDrawListsFn = NULL;
#endif

    // Input (NB: we already have memset zero the entire structure)
    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    MousePosPrev = ImVec2(-FLT_MAX, -FLT_MAX);
    MouseDragThreshold = 6.0f;
    for (int i = 0; i < IM_ARRAYSIZE(MouseDownDuration); i++) MouseDownDuration[i] = MouseDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(KeysDownDuration); i++) KeysDownDuration[i]  = KeysDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(NavInputsDownDuration); i++) NavInputsDownDuration[i] = -1.0f;
}

// Pass in translated ASCII characters for text input.
// - with glfw you can get those from the callback set in glfwSetCharCallback()
// - on Windows you can get those using ToAscii+keyboard state, or via the WM_CHAR message
void ImGuiIO::AddInputCharacter(ImWchar c)
{
    const int n = ImStrlenW(InputCharacters);
    if (n + 1 < IM_ARRAYSIZE(InputCharacters))
    {
        InputCharacters[n] = c;
        InputCharacters[n+1] = '\0';
    }
}

void ImGuiIO::AddInputCharactersUTF8(const char* utf8_chars)
{
    // We can't pass more wchars than ImGuiIO::InputCharacters[] can hold so don't convert more
    const int wchars_buf_len = sizeof(ImGuiIO::InputCharacters) / sizeof(ImWchar);
    ImWchar wchars[wchars_buf_len];
    ImTextStrFromUtf8(wchars, wchars_buf_len, utf8_chars, NULL);
    for (int i = 0; i < wchars_buf_len && wchars[i] != 0; i++)
        AddInputCharacter(wchars[i]);
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPER/UTILITIES (Maths, String, Format, Hash, File functions)
//-----------------------------------------------------------------------------

ImVec2 ImLineClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& p)
{
    ImVec2 ap = p - a;
    ImVec2 ab_dir = b - a;
    float dot = ap.x * ab_dir.x + ap.y * ab_dir.y;
    if (dot < 0.0f)
        return a;
    float ab_len_sqr = ab_dir.x * ab_dir.x + ab_dir.y * ab_dir.y;
    if (dot > ab_len_sqr)
        return b;
    return a + ab_dir * dot / ab_len_sqr;
}

bool ImTriangleContainsPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    bool b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) < 0.0f;
    bool b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) < 0.0f;
    bool b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) < 0.0f;
    return ((b1 == b2) && (b2 == b3));
}

void ImTriangleBarycentricCoords(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p, float& out_u, float& out_v, float& out_w)
{
    ImVec2 v0 = b - a;
    ImVec2 v1 = c - a;
    ImVec2 v2 = p - a;
    const float denom = v0.x * v1.y - v1.x * v0.y;
    out_v = (v2.x * v1.y - v1.x * v2.y) / denom;
    out_w = (v0.x * v2.y - v2.x * v0.y) / denom;
    out_u = 1.0f - out_v - out_w;
}

ImVec2 ImTriangleClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    ImVec2 proj_ab = ImLineClosestPoint(a, b, p);
    ImVec2 proj_bc = ImLineClosestPoint(b, c, p);
    ImVec2 proj_ca = ImLineClosestPoint(c, a, p);
    float dist2_ab = ImLengthSqr(p - proj_ab);
    float dist2_bc = ImLengthSqr(p - proj_bc);
    float dist2_ca = ImLengthSqr(p - proj_ca);
    float m = ImMin(dist2_ab, ImMin(dist2_bc, dist2_ca));
    if (m == dist2_ab)
        return proj_ab;
    if (m == dist2_bc)
        return proj_bc;
    return proj_ca;
}

int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

int ImStrnicmp(const char* str1, const char* str2, size_t count)
{
    int d = 0;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
}

void ImStrncpy(char* dst, const char* src, size_t count)
{
    if (count < 1) return;
    strncpy(dst, src, count);
    dst[count-1] = 0;
}

char* ImStrdup(const char *str)
{
    size_t len = strlen(str) + 1;
    void* buf = ImGui::MemAlloc(len);
    return (char*)memcpy(buf, (const void*)str, len);
}

const char* ImStrchrRange(const char* str, const char* str_end, char c)
{
    for ( ; str < str_end; str++)
        if (*str == c)
            return str;
    return NULL;
}

int ImStrlenW(const ImWchar* str)
{
    int n = 0;
    while (*str++) n++;
    return n;
}

const ImWchar* ImStrbolW(const ImWchar* buf_mid_line, const ImWchar* buf_begin) // find beginning-of-line
{
    while (buf_mid_line > buf_begin && buf_mid_line[-1] != '\n')
        buf_mid_line--;
    return buf_mid_line;
}

const char* ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)toupper(*needle);
    while ((!haystack_end && *haystack) || (haystack_end && haystack < haystack_end))
    {
        if (toupper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (toupper(*a) != toupper(*b))
                    break;
            if (b == needle_end)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

// Trim str by offsetting contents when there's leading data + writing a \0 at the trailing position. We use this in situation where the cost is negligible.
void ImStrTrimBlanks(char* buf)
{
    char* p = buf;
    while (p[0] == ' ' || p[0] == '\t')     // Leading blanks
        p++;
    char* p_start = p;
    while (*p != 0)                         // Find end of string
        p++;
    while (p > p_start && (p[-1] == ' ' || p[-1] == '\t'))  // Trailing blanks
        p--;
    if (p_start != buf)                     // Copy memory if we had leading blanks
        memmove(buf, p_start, p - p_start);
    buf[p - p_start] = 0;                   // Zero terminate
}

// A) MSVC version appears to return -1 on overflow, whereas glibc appears to return total count (which may be >= buf_size).
// Ideally we would test for only one of those limits at runtime depending on the behavior the vsnprintf(), but trying to deduct it at compile time sounds like a pandora can of worm.
// B) When buf==NULL vsnprintf() will return the output size.
#ifndef IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf _vsnprintf
#endif

int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}

int ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
    int w = vsnprintf(buf, buf_size, fmt, args);
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}
#endif // #ifdef IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

// Pass data_size==0 for zero-terminated strings
// FIXME-OPT: Replace with e.g. FNV1a hash? CRC32 pretty much randomly access 1KB. Need to do proper measurements.
ImU32 ImHash(const void* data, int data_size, ImU32 seed)
{
    static ImU32 crc32_lut[256] = { 0 };
    if (!crc32_lut[1])
    {
        const ImU32 polynomial = 0xEDB88320;
        for (ImU32 i = 0; i < 256; i++)
        {
            ImU32 crc = i;
            for (ImU32 j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (ImU32(-int(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    seed = ~seed;
    ImU32 crc = seed;
    const unsigned char* current = (const unsigned char*)data;

    if (data_size > 0)
    {
        // Known size
        while (data_size--)
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *current++];
    }
    else
    {
        // Zero-terminated string
        while (unsigned char c = *current++)
        {
            // We support a syntax of "label###id" where only "###id" is included in the hash, and only "label" gets displayed.
            // Because this syntax is rarely used we are optimizing for the common case.
            // - If we reach ### in the string we discard the hash so far and reset to the seed.
            // - We don't do 'current += 2; continue;' after handling ### to keep the code smaller.
            if (c == '#' && current[0] == '#' && current[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    return ~crc;
}

FILE* ImFileOpen(const char* filename, const char* mode)
{
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__GNUC__)
    // We need a fopen() wrapper because MSVC/Windows fopen doesn't handle UTF-8 filenames. Converting both strings from UTF-8 to wchar format (using a single allocation, because we can)
    const int filename_wsize = ImTextCountCharsFromUtf8(filename, NULL) + 1;
    const int mode_wsize = ImTextCountCharsFromUtf8(mode, NULL) + 1;
    ImVector<ImWchar> buf;
    buf.resize(filename_wsize + mode_wsize);
    ImTextStrFromUtf8(&buf[0], filename_wsize, filename, NULL);
    ImTextStrFromUtf8(&buf[filename_wsize], mode_wsize, mode, NULL);
    return _wfopen((wchar_t*)&buf[0], (wchar_t*)&buf[filename_wsize]);
#else
    return fopen(filename, mode);
#endif
}

// Load file content into memory
// Memory allocated with ImGui::MemAlloc(), must be freed by user using ImGui::MemFree()
void* ImFileLoadToMemory(const char* filename, const char* file_open_mode, size_t* out_file_size, int padding_bytes)
{
    IM_ASSERT(filename && file_open_mode);
    if (out_file_size)
        *out_file_size = 0;

    FILE* f;
    if ((f = ImFileOpen(filename, file_open_mode)) == NULL)
        return NULL;

    long file_size_signed;
    if (fseek(f, 0, SEEK_END) || (file_size_signed = ftell(f)) == -1 || fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        return NULL;
    }

    size_t file_size = (size_t)file_size_signed;
    void* file_data = ImGui::MemAlloc(file_size + padding_bytes);
    if (file_data == NULL)
    {
        fclose(f);
        return NULL;
    }
    if (fread(file_data, 1, file_size, f) != file_size)
    {
        fclose(f);
        ImGui::MemFree(file_data);
        return NULL;
    }
    if (padding_bytes > 0)
        memset((void*)(((char*)file_data) + file_size), 0, (size_t)padding_bytes);

    fclose(f);
    if (out_file_size)
        *out_file_size = file_size;

    return file_data;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (ImText* functions)
//-----------------------------------------------------------------------------

// Convert UTF-8 to 32-bits character, process single character input.
// Based on stb_from_utf8() from github.com/nothings/stb/
// We handle UTF-8 decoding error by skipping forward.
int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
{
    unsigned int c = (unsigned int)-1;
    const unsigned char* str = (const unsigned char*)in_text;
    if (!(*str & 0x80))
    {
        c = (unsigned int)(*str++);
        *out_char = c;
        return 1;
    }
    if ((*str & 0xe0) == 0xc0)
    {
        *out_char = 0xFFFD; // will be invalid but not end of string
        if (in_text_end && in_text_end - (const char*)str < 2) return 1;
        if (*str < 0xc2) return 2;
        c = (unsigned int)((*str++ & 0x1f) << 6);
        if ((*str & 0xc0) != 0x80) return 2;
        c += (*str++ & 0x3f);
        *out_char = c;
        return 2;
    }
    if ((*str & 0xf0) == 0xe0)
    {
        *out_char = 0xFFFD; // will be invalid but not end of string
        if (in_text_end && in_text_end - (const char*)str < 3) return 1;
        if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf)) return 3;
        if (*str == 0xed && str[1] > 0x9f) return 3; // str[1] < 0x80 is checked below
        c = (unsigned int)((*str++ & 0x0f) << 12);
        if ((*str & 0xc0) != 0x80) return 3;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80) return 3;
        c += (*str++ & 0x3f);
        *out_char = c;
        return 3;
    }
    if ((*str & 0xf8) == 0xf0)
    {
        *out_char = 0xFFFD; // will be invalid but not end of string
        if (in_text_end && in_text_end - (const char*)str < 4) return 1;
        if (*str > 0xf4) return 4;
        if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf)) return 4;
        if (*str == 0xf4 && str[1] > 0x8f) return 4; // str[1] < 0x80 is checked below
        c = (unsigned int)((*str++ & 0x07) << 18);
        if ((*str & 0xc0) != 0x80) return 4;
        c += (unsigned int)((*str++ & 0x3f) << 12);
        if ((*str & 0xc0) != 0x80) return 4;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80) return 4;
        c += (*str++ & 0x3f);
        // utf-8 encodings of values used in surrogate pairs are invalid
        if ((c & 0xFFFFF800) == 0xD800) return 4;
        *out_char = c;
        return 4;
    }
    *out_char = 0;
    return 0;
}

int ImTextStrFromUtf8(ImWchar* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        if (c < 0x10000)    // FIXME: Losing characters that don't fit in 2 bytes
            *buf_out++ = (ImWchar)c;
    }
    *buf_out = 0;
    if (in_text_remaining)
        *in_text_remaining = in_text;
    return (int)(buf_out - buf);
}

int ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end)
{
    int char_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        if (c < 0x10000)
            char_count++;
    }
    return char_count;
}

// Based on stb_to_utf8() from github.com/nothings/stb/
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80)
    {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800)
    {
        if (buf_size < 2) return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));
        return 2;
    }
    if (c >= 0xdc00 && c < 0xe000)
    {
        return 0;
    }
    if (c >= 0xd800 && c < 0xdc00)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c ) & 0x3f));
        return 4;
    }
    //else if (c < 0x10000)
    {
        if (buf_size < 3) return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c>> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c ) & 0x3f));
        return 3;
    }
}

// Not optimal but we very rarely use this function.
int ImTextCountUtf8BytesFromChar(const char* in_text, const char* in_text_end)
{
    unsigned int dummy = 0;
    return ImTextCharFromUtf8(&dummy, in_text, in_text_end);
}

static inline int ImTextCountUtf8BytesFromChar(unsigned int c)
{
    if (c < 0x80) return 1;
    if (c < 0x800) return 2;
    if (c >= 0xdc00 && c < 0xe000) return 0;
    if (c >= 0xd800 && c < 0xdc00) return 4;
    return 3;
}

int ImTextStrToUtf8(char* buf, int buf_size, const ImWchar* in_text, const ImWchar* in_text_end)
{
    char* buf_out = buf;
    const char* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            *buf_out++ = (char)c;
        else
            buf_out += ImTextCharToUtf8(buf_out, (int)(buf_end-buf_out-1), c);
    }
    *buf_out = 0;
    return (int)(buf_out - buf);
}

int ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end)
{
    int bytes_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            bytes_count++;
        else
            bytes_count += ImTextCountUtf8BytesFromChar(c);
    }
    return bytes_count;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPER/UTILTIES (Color functions)
// Note: The Convert functions are early design which are not consistent with other API.
//-----------------------------------------------------------------------------

ImVec4 ImGui::ColorConvertU32ToFloat4(ImU32 in)
{
    float s = 1.0f/255.0f;
    return ImVec4(
        ((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

ImU32 ImGui::ColorConvertFloat4ToU32(const ImVec4& in)
{
    ImU32 out;
    out  = ((ImU32)IM_F32_TO_INT8_SAT(in.x)) << IM_COL32_R_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.y)) << IM_COL32_G_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.z)) << IM_COL32_B_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.w)) << IM_COL32_A_SHIFT;
    return out;
}

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
void ImGui::ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        ImSwap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        ImSwap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = ImFabs(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
void ImGui::ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;
        return;
    }

    h = ImFmod(h, 1.0f) / (60.0f/360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

ImU32 ImGui::GetColorU32(ImGuiCol idx, float alpha_mul)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = style.Colors[idx];
    c.w *= style.Alpha * alpha_mul;
    return ColorConvertFloat4ToU32(c);
}

ImU32 ImGui::GetColorU32(const ImVec4& col)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = col;
    c.w *= style.Alpha;
    return ColorConvertFloat4ToU32(c);
}

const ImVec4& ImGui::GetStyleColorVec4(ImGuiCol idx)
{
    ImGuiStyle& style = GImGui->Style;
    return style.Colors[idx];
}

ImU32 ImGui::GetColorU32(ImU32 col)
{
    float style_alpha = GImGui->Style.Alpha;
    if (style_alpha >= 1.0f)
        return col;
    ImU32 a = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
    a = (ImU32)(a * style_alpha); // We don't need to clamp 0..255 because Style.Alpha is in 0..1 range.
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiStorage
// Helper: Key->value storage
//-----------------------------------------------------------------------------

// std::lower_bound but without the bullshit
static ImVector<ImGuiStorage::Pair>::iterator LowerBound(ImVector<ImGuiStorage::Pair>& data, ImGuiID key)
{
    ImVector<ImGuiStorage::Pair>::iterator first = data.begin();
    ImVector<ImGuiStorage::Pair>::iterator last = data.end();
    size_t count = (size_t)(last - first);
    while (count > 0)
    {
        size_t count2 = count >> 1;
        ImVector<ImGuiStorage::Pair>::iterator mid = first + count2;
        if (mid->key < key)
        {
            first = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return first;
}

// For quicker full rebuild of a storage (instead of an incremental one), you may add all your contents and then sort once.
void ImGuiStorage::BuildSortByKey()
{
    struct StaticFunc
    {
        static int IMGUI_CDECL PairCompareByID(const void* lhs, const void* rhs)
        {
            // We can't just do a subtraction because qsort uses signed integers and subtracting our ID doesn't play well with that.
            if (((const Pair*)lhs)->key > ((const Pair*)rhs)->key) return +1;
            if (((const Pair*)lhs)->key < ((const Pair*)rhs)->key) return -1;
            return 0;
        }
    };
    if (Data.Size > 1)
        ImQsort(Data.Data, (size_t)Data.Size, sizeof(Pair), StaticFunc::PairCompareByID);
}

int ImGuiStorage::GetInt(ImGuiID key, int default_val) const
{
    ImVector<Pair>::iterator it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_i;
}

bool ImGuiStorage::GetBool(ImGuiID key, bool default_val) const
{
    return GetInt(key, default_val ? 1 : 0) != 0;
}

float ImGuiStorage::GetFloat(ImGuiID key, float default_val) const
{
    ImVector<Pair>::iterator it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImVector<Pair>::iterator it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return NULL;
    return it->val_p;
}

// References are only valid until a new value is added to the storage. Calling a Set***() function or a Get***Ref() function invalidates the pointer.
int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_i;
}

bool* ImGuiStorage::GetBoolRef(ImGuiID key, bool default_val)
{
    return (bool*)GetIntRef(key, default_val ? 1 : 0);
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_f;
}

void** ImGuiStorage::GetVoidPtrRef(ImGuiID key, void* default_val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_p;
}

// FIXME-OPT: Need a way to reuse the result of lower_bound when doing GetInt()/SetInt() - not too bad because it only happens on explicit interaction (maximum one a frame)
void ImGuiStorage::SetInt(ImGuiID key, int val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_i = val;
}

void ImGuiStorage::SetBool(ImGuiID key, bool val)
{
    SetInt(key, val ? 1 : 0);
}

void ImGuiStorage::SetFloat(ImGuiID key, float val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImGuiID key, void* val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (int i = 0; i < Data.Size; i++)
        Data[i].val_i = v;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextFilter
//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter(const char* default_filter)
{
    if (default_filter)
    {
        ImStrncpy(InputBuf, default_filter, IM_ARRAYSIZE(InputBuf));
        Build();
    }
    else
    {
        InputBuf[0] = 0;
        CountGrep = 0;
    }
}

bool ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width != 0.0f)
        ImGui::PushItemWidth(width);
    bool value_changed = ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (width != 0.0f)
        ImGui::PopItemWidth();
    if (value_changed)
        Build();
    return value_changed;
}

void ImGuiTextFilter::TextRange::split(char separator, ImVector<TextRange>* out) const
{
    out->resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(TextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out->push_back(TextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    TextRange input_range(InputBuf, InputBuf+strlen(InputBuf));
    input_range.split(',', &Filters);

    CountGrep = 0;
    for (int i = 0; i != Filters.Size; i++)
    {
        TextRange& f = Filters[i];
        while (f.b < f.e && ImCharIsBlankA(f.b[0]))
            f.b++;
        while (f.e > f.b && ImCharIsBlankA(f.e[-1]))
            f.e--;
        if (f.empty())
            continue;
        if (Filters[i].b[0] != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (Filters.empty())
        return true;

    if (text == NULL)
        text = "";

    for (int i = 0; i != Filters.Size; i++)
    {
        const TextRange& f = Filters[i];
        if (f.empty())
            continue;
        if (f.b[0] == '-')
        {
            // Subtract
            if (ImStristr(text, text_end, f.begin()+1, f.end()) != NULL)
                return false;
        }
        else
        {
            // Grep
            if (ImStristr(text, text_end, f.begin(), f.end()) != NULL)
                return true;
        }
    }

    // Implicit * grep
    if (CountGrep == 0)
        return true;

    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextBuffer
//-----------------------------------------------------------------------------

// On some platform vsnprintf() takes va_list by reference and modifies it.
// va_copy is the 'correct' way to copy a va_list but Visual Studio prior to 2013 doesn't have it.
#ifndef va_copy
#if defined(__GNUC__) || defined(__clang__)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#else
#define va_copy(dest, src) (dest = src)
#endif
#endif

// Helper: Text buffer for logging/accumulating text
void ImGuiTextBuffer::appendfv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = ImFormatStringV(NULL, 0, fmt, args);         // FIXME-OPT: could do a first pass write attempt, likely successful on first pass.
    if (len <= 0)
    {
        va_end(args_copy);
        return;
    }

    const int write_off = Buf.Size;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int double_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > double_capacity ? needed_sz : double_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off - 1], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

void ImGuiTextBuffer::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

