#pragma once
// GGMAX 3.25 - WATER BAKE. See GGWaterBake.cpp for the rationale.
// Replaces the 2.94 "Water Off" switch: still removes the ocean and its planar reflection pass,
// but leaves a flat semi-transparent plane behind so the water does not simply disappear.

#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"

extern bool  gg_water_bake;         // panel "Water Bake", setup.ini waterbake
extern float gg_water_bake_alpha;   // setup.ini waterbakealpha, 0..1
extern int   gg_water_bake_drawn;   // diagnostic: 1 if the plane was drawn last frame

void GGWaterBake_Init();
void GGWaterBake_Update();          // main thread
void GGWaterBake_Draw( const wi::primitive::Frustum* frustum, wi::graphics::CommandList cmd );
const char* GGWaterBake_Report();
