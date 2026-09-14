-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Fog v10 by Necrym59 with special thanks to Bolt Action Gaming
-- DESCRIPTION: Add to an object then link to a switch or multi-trigger zone to activate/de-activate.
-- DESCRIPTION: [PROMPT_TEXT$="Fog strength is changing"]
-- DESCRIPTION: [FOG_NEAREST=1(0,100)]
-- DESCRIPTION: [FOG_DISTANCE=5(0,100)]
-- DESCRIPTION: [FOG_SPEED=25(1,100)]
-- DESCRIPTION: [@FOG_COLOUR_CHANGE=1(1=Off, 2=On)]
-- DESCRIPTION: [FOG_RED=0(0,255)]
-- DESCRIPTION: [FOG_GREEN=0(0,255)]
-- DESCRIPTION: [FOG_BLUE=0(0,255)]
-- DESCRIPTION: [FOG_INTENSITY=0(0,100)]

local fog 				= {}
local prompt_text 		= {}
local fog_nearest		= {}
local fog_distance		= {}
local fog_speed			= {}
local fog_colour_change	= {}
local fog_red			= {}
local fog_green			= {}
local fog_blue			= {}	
local fog_intensity		= {}

local status 			= {}
local fogswitch			= {}
local fog_inprogress	= {}

-- "Default" values
local default_fogn		= {}
local default_fogd		= {}	
local default_fog_r		= {}
local default_fog_g		= {}
local default_fog_b		= {}
local default_fog_intensity = {}

-- "Start" values
local start_fogn		= {}
local start_fogd		= {}
local start_fog_r		= {}
local start_fog_g		= {}
local start_fog_b		= {}
local start_fog_intensity = {}

-- The Interpolation Timer (0.0 to 1.0)
local fog_t             = {}

-- Linear Interpolation
local function math_lerp(a, b, t)
    return a + (b - a) * t
end

local function math_smoothstep(t)
    return t * t * (3 - 2 * t)
end

function fog_GetTargetFogUnit(percent_input)
    percent_input = math.max(0, math.min(100, percent_input))
    local units = percent_input^2 * 100 
    return units
end

function fog_properties(e, prompt_text, fog_nearest, fog_distance, fog_speed, fog_colour_change, fog_red, fog_green, fog_blue, fog_intensity)
    fog[e] = fog[e] or {}
    fog[e].prompt_text = prompt_text
    fog[e].fog_nearest = fog_GetTargetFogUnit(fog_nearest)
    fog[e].fog_distance	= fog_GetTargetFogUnit(fog_distance)
    fog[e].fog_speed = fog_speed
    fog[e].fog_colour_change = fog_colour_change or 1
    fog[e].fog_red = fog_red
    fog[e].fog_green = fog_green
    fog[e].fog_blue = fog_blue	
    fog[e].fog_intensity = fog_intensity * .01 
end

function fog_init(e)
    fog[e] = fog[e] or {}
    fog[e].prompt_text = "Fog strength is changing"
    fog[e].fog_nearest = 1
    fog[e].fog_distance	= 5
    fog[e].fog_speed = 10
    fog[e].fog_colour_change = 1
    
    -- Initialize defaults
    fog[e].fog_red = GetFogRed()
    fog[e].fog_green = GetFogGreen()
    fog[e].fog_blue = GetFogBlue()
    fog[e].fog_intensity = GetFogIntensity() 
    
    fogswitch[e] = 0
    fog_inprogress[e] = 0
    fog_t[e] = 0
    status[e] = "init"
end


function fog_main(e)

    if status[e] == "init" then
        -- Capture curren levels
        default_fogn[e]	= GetFogNearest()
        default_fogd[e]	= GetFogDistance()
        default_fog_r[e] = GetFogRed()
        default_fog_g[e] = GetFogGreen()
        default_fog_b[e] = GetFogBlue()
        default_fog_intensity[e] = GetFogIntensity()
        if default_fogd[e] > fog[e].fog_distance then fogswitch[e] = 0 end
        if default_fogd[e] < default_fogd[e] then fogswitch[e] = 1 end
        
        SetActivated(e,0)
        status[e] = "endinit"
    end

    if g_Entity[e]['activated'] == 1 then
        -- PHASE 1: INITIALIZATION
        if fog_inprogress[e] == 0 then
            fog_inprogress[e] = 1
            fog_t[e] = 0 -- Reset timer
            start_fogn[e] = GetFogNearest()
            start_fogd[e] = GetFogDistance()
            start_fog_r[e] = GetFogRed()
            start_fog_g[e] = GetFogGreen()
            start_fog_b[e] = GetFogBlue()
            start_fog_intensity[e] = GetFogIntensity()
        end

        -- PHASE 2: CALCULATE
        local target_n, target_d, target_r, target_g, target_b, target_i
        
        if fogswitch[e] == 0 then
            -- To new fog settings
            target_n = fog[e].fog_nearest
            target_d = fog[e].fog_distance
            target_r = fog[e].fog_red
            target_g = fog[e].fog_green
            target_b = fog[e].fog_blue
            target_i = fog[e].fog_intensity
        else
            -- Back to default settings
            target_n = default_fogn[e]
            target_d = default_fogd[e]
            target_r = default_fog_r[e]
            target_g = default_fog_g[e]
            target_b = default_fog_b[e]
            target_i = default_fog_intensity[e]
        end

        -- PHASE 3: INTERPOLATION
        local speed_factor = (fog[e].fog_speed / 10000.0) * 0.05 
        fog_t[e] = fog_t[e] + speed_factor
        -- Clamp timer at 1.0
        if fog_t[e] > 1.0 then fog_t[e] = 1.0 end        
        local t_linear = fog_t[e]
        local t_curved = math_smoothstep(t_linear) 
        -- Apply Distance Fogs using the curved time
        SetFogNearest(math_lerp(start_fogn[e], target_n, t_curved))
        SetFogDistance(math_lerp(start_fogd[e], target_d, t_curved))
        -- Apply Color Changes (If enabled)
        if fog[e].fog_colour_change == 2 then
            SetFogRed(math_lerp(start_fog_r[e], target_r, t_curved))
            SetFogGreen(math_lerp(start_fog_g[e], target_g, t_curved))
            SetFogBlue(math_lerp(start_fog_b[e], target_b, t_curved))
            SetFogIntensity(math_lerp(start_fog_intensity[e], target_i, t_curved))
        end
 
        -- PHASE 4: COMPLETION
		if t_linear >= 1.0 then
            fog_inprogress[e] = 0            
            if fogswitch[e] == 0 then
                fogswitch[e] = 1 
				PromptDuration(fog[e].prompt_text, 3000)
            else
                fogswitch[e] = 0
				PromptDuration(fog[e].prompt_text, 3000)
            end            
            SetActivated(e, 0)
        end
    end
end