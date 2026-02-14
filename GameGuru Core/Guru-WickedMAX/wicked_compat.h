#pragma once
// Compatibility shims for WickedEngine API changes
// This header maps old API names to their new equivalents

#include "../../../WickedEngineDX12/WickedEngine/WickedEngine.h"

// GetDevice() moved from wi::renderer to wi::graphics
// Provide it in wi::renderer for backwards compatibility
namespace wi { namespace renderer {
    inline wi::graphics::GraphicsDevice* GetDevice() {
        return wi::graphics::GetDevice();
    }
}}

// Profiler statistics functions removed - provide stubs returning 0
namespace wi { namespace profiler {
    inline int GetDrawCalls() { return 0; }
    inline int GetDrawCallsShadows() { return 0; }
    inline int GetDrawCallsTransparent() { return 0; }
    inline int GetDrawCallsShadowsCube() { return 0; }
    inline int GetPolygons() { return 0; }
    inline int GetPolygonsShadows() { return 0; }
    inline int GetPolygonsTransparent() { return 0; }
    inline std::string GetProfilerData() { return ""; }
    inline std::string GetProfilerDataFilter() { return ""; }
}}

// wi::renderer::SetGamma removed - provide no-op stub
namespace wi { namespace renderer {
    inline void SetGamma(float) {}
}}

// Viewport member name changes: Width/Height -> width/height
// TextureDesc member name changes: Width/Height -> width/height
// These must be fixed in-place (cannot shim struct members)

// wi::Resource changes: no longer supports nullptr assignment or boolean conversion
// Use resource.IsValid() instead of if(resource) or !resource
// Use resource = wi::Resource() instead of resource = nullptr
