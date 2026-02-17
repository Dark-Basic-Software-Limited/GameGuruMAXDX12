#ifndef WI_SAMPLER_MAPPING_H
#define WI_SAMPLER_MAPPING_H

// Slot matchings:
////////////////////////////////////////////////////

// Persistent samplers - DX12 static samplers in root signature at s100-s109
// Old DX11 slots (s4-s14) remapped to match WICKED_ENGINE_DEFAULT_ROOTSIGNATURE
#define SSLOT_LINEAR_CLAMP		100
#define SSLOT_LINEAR_WRAP		101
#define SSLOT_LINEAR_MIRROR		102
#define SSLOT_POINT_CLAMP		103
#define SSLOT_POINT_WRAP		104
#define SSLOT_POINT_MIRROR		105
#define SSLOT_ANISO_CLAMP		106
#define SSLOT_ANISO_WRAP		107
#define SSLOT_ANISO_MIRROR		108
#define SSLOT_CMP_DEPTH			109
#define SSLOT_OBJECTSHADER		7
#define SSLOT_RESERVED			110
#define SSLOT_COUNT_PERSISTENT	10

// On demand samplers:
// These are bound on demand and alive until another is bound at the same slot
#define SSLOT_ONDEMAND0			0
#define SSLOT_ONDEMAND1			1
#define SSLOT_ONDEMAND2			2
#define SSLOT_ONDEMAND3			3
#define SSLOT_COUNT_ONDEMAND	(SSLOT_ONDEMAND3 + 1)

#define SSLOT_COUNT				(SSLOT_COUNT_PERSISTENT + SSLOT_COUNT_ONDEMAND)

#endif // WI_SAMPLER_MAPPING_H
