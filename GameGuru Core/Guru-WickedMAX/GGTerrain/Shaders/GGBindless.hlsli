// GGBindless.hlsli - DEPRECATED: Use GGFrameCompat.hlsli instead.
//
// This header originally contained the compatibility bridge between old DX11
// slot-based resource model and new DX12 bindless model. With the decision to
// use C2 (expanded SRV range to 64) rather than full bindless, the "bindless"
// name is misleading. All functionality has been moved to GGFrameCompat.hlsli.
//
// This header is kept as a forwarding include for any code that may reference it.

#ifndef GG_BINDLESS_HLSLI
#define GG_BINDLESS_HLSLI

#include "GGFrameCompat.hlsli"

#endif // GG_BINDLESS_HLSLI
