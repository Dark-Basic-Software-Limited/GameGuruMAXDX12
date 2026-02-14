#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"

// DX12: customShaderParam1-4 replaced with uint4 userdata (asfloat to read as float)
#define GRID_LINE_THICKNESS_BASE     asfloat(GetMaterial().userdata.x)
#define MAX_FADE_DISTANCE            asfloat(GetMaterial().userdata.y)
#define GRID_POWER_EXPONENT          asfloat(GetMaterial().userdata.z)
#define GRID_MIN_ALPHA               asfloat(GetMaterial().userdata.w)
#define GRID_COLOR 0.6f
#define DARK_GRID_COLOR 0.2f


float GetLineIntensity(float frac_coord, float world_line_spacing, float base_thickness_factor, float3 worldPos)
{
    float dx = fwidth(worldPos.x);
    float dz = fwidth(worldPos.z);

    float screen_space_line_thickness_x = dx * (1.0f / world_line_spacing);
    float screen_space_line_thickness_z = dz * (1.0f / world_line_spacing);

    float effective_line_thickness = max(screen_space_line_thickness_x, screen_space_line_thickness_z);

    effective_line_thickness *= base_thickness_factor;

    float half_width = effective_line_thickness * 0.5f;

    float dist_to_nearest_line_center = min(frac_coord, 1.0f - frac_coord);

    float intensity = smoothstep(half_width, 0.0f, dist_to_nearest_line_center);

    return intensity;
}


float4 main(PixelInput input) : SV_TARGET
{
    float3 worldPos = input.GetPos3D();
    float dist_to_camera = distance(worldPos, GetCamera().position.xyz);

    //PE: Fading
    float fade_t = saturate(dist_to_camera / MAX_FADE_DISTANCE);
    float overall_alpha_fade = saturate(1.0f - pow(fade_t, GRID_POWER_EXPONENT));

    float final_alpha = GRID_MIN_ALPHA * overall_alpha_fade;

    if (final_alpha < 0.001f)
    {
        discard;
    }

    float3 grid_pixel_color = float3(0.0f, 0.0f, 0.0f);

    //PE: Grid 100
    float3 color100 = float3(GRID_COLOR, GRID_COLOR, GRID_COLOR);

    float fracX100 = frac(worldPos.x / 100.0f);
    float fracZ100 = frac(worldPos.z / 100.0f);

    float lineX100_intensity = GetLineIntensity(fracX100, 100.0f, GRID_LINE_THICKNESS_BASE, worldPos);
    float lineZ100_intensity = GetLineIntensity(fracZ100, 100.0f, GRID_LINE_THICKNESS_BASE, worldPos);

    grid_pixel_color = max(grid_pixel_color, lineX100_intensity * color100);
    grid_pixel_color = max(grid_pixel_color, lineZ100_intensity * color100);


    //PE: Grid 50
    float fine_grid_alpha_blend = 1.0f;
    float3 color50 = float3(DARK_GRID_COLOR, DARK_GRID_COLOR, DARK_GRID_COLOR);

    float fracX50 = frac(worldPos.x / 50.0f);
    float fracZ50 = frac(worldPos.z / 50.0f);

    float lineX50_intensity = GetLineIntensity(fracX50, 50.0f, GRID_LINE_THICKNESS_BASE, worldPos);
    float lineZ50_intensity = GetLineIntensity(fracZ50, 50.0f, GRID_LINE_THICKNESS_BASE, worldPos);

    float not_on_100_line_X = 1.0f - lineX100_intensity;
    float not_on_100_line_Z = 1.0f - lineZ100_intensity;

    grid_pixel_color = max(grid_pixel_color, lineX50_intensity * color50 * fine_grid_alpha_blend * not_on_100_line_X);
    grid_pixel_color = max(grid_pixel_color, lineZ50_intensity * color50 * fine_grid_alpha_blend * not_on_100_line_Z);

    return float4(grid_pixel_color, final_alpha);
}
