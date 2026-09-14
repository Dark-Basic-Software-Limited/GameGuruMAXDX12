-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Variable Light v5
-- DESCRIPTION: A light for use with a variable switch. Attach to a light.
-- DESCRIPTION: Enter the name of the [@@VARIABLE_SWITCH_USER_GLOBAL$=""(0=globallist)]  User Global used to monitor (eg; Variable_Switch1)
-- DESCRIPTION: [#VARIABLE_MULTIPLIER=0.0(0.0,1000.0)]
-- DESCRIPTION: [LIGHT_OBJECT_NAME$="Light Object"]

module_lightcontrol = require "scriptbank\\markers\\module_lightcontrol"
local rad = math.rad
local lower = string.lower
local varlight 						= {}
local variable_switch_user_global	= {}

local status 						= {}
local current_level					= {}
local currentvalue					= {}
local variable_multiplier			= {}
local light_object_name				= {}
local light_object_no				= {}
local lightNum = GetEntityLightNumber(e)

function variable_light_properties(e,variable_switch_user_global, variable_multiplier, light_object_name)	
	module_lightcontrol.init(e,1)
	varlight[e].variable_switch_user_global = variable_switch_user_global
	varlight[e].variable_multiplier = variable_multiplier
	varlight[e].light_object_name = lower(light_object_name)
end

function variable_light_init(e)
	varlight[e] = {}
	varlight[e].variable_switch_user_global = ""
	varlight[e].variable_multiplier = 0
	varlight[e].light_object_name = ""
	varlight[e].light_object_no = 0
	
	lightNum = GetEntityLightNumber(e)
	SetActivated(e,1)
	currentvalue[e] = 0
	status[e] = "init"
end
	
function variable_light_main(e)	

	if status[e] == "init" then
		if varlight[e].light_object_no == 0 and varlight[e].light_object_name > "" then
			for a = 1, g_EntityElementMax do
				if a ~= nil and g_Entity[a] ~= nil then
					if lower(GetEntityName(a)) == varlight[e].light_object_name then
						varlight[e].light_object_no = a
						SetEntityEmissiveStrength(a,0)
						SetActivated(a,0)
						break
					end
				end
			end	
		end	
		status[e] = "endinit"
	end
	
	current_level[e] = g_vswitchvalue
	
	if g_Entity[e]['activated'] == 1 then		
		lightNum = GetEntityLightNumber(e)
		if varlight[e].variable_switch_user_global ~= "" then
			if _G["g_UserGlobal['"..varlight[e].variable_switch_user_global.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..varlight[e].variable_switch_user_global.."']"] end
			current_level[e] = _G["g_UserGlobal['"..varlight[e].variable_switch_user_global.."']"] * varlight[e].variable_multiplier
			if varlight[e].light_object_no > 0 then SetEntityEmissiveStrength(varlight[e].light_object_no,current_level[e]*(varlight[e].variable_multiplier/20)) end			
		end
		SetLightRange(lightNum,current_level[e])
	end
end