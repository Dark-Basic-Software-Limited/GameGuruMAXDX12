-- View Activator v3 by Necrym59
-- DESCRIPTION: Will allow activation of logic linked or IfUsed entities by player viewing the trigger entity then deactivate.
-- DESCRIPTION: [VIEW_DISTANCE=150(1,1000)] Maximum view distance
-- DESCRIPTION: [ACTIVATION_DELAY=0(0,10000)] milliseconds (1000 = 1 second)
-- DESCRIPTION: [ACTIVATION_MESSAGE$="Activation in progress"]
-- DESCRIPTION: <Sound0> - Activation Sound

local U = require "scriptbank\\utillib"

local view_activator		= {}
local view_distance			= {}
local activation_delay		= {}
local activation_message	= {}

local is_triggered		= {}
local time_delay		= {}
local doonce			= {}
local status			= {}

function view_activator_properties(e, view_distance, activation_delay, activation_message)
	view_activator[e].view_distance = view_distance
	view_activator[e].activation_delay = activation_delay
	view_activator[e].activation_message = activation_message
end

function view_activator_init(e)
	view_activator[e] = {}
	view_activator[e].view_distance = 150
	view_activator[e].activation_delay = 0
	view_activator[e].activation_message = "Activation in progress"
	
	is_triggered[e] = 0
	doonce[e] = 0
	time_delay[e] = math.huge	
	status[e] = "init"
end

function view_activator_main(e)

	if status[e] == "init" then
		if g_Entity[e].activated > 0 then is_triggered[e] = true end
		time_delay[e] = (g_Time + view_activator[e].activation_delay)
		status[e] = "viewing"
	end	
	
	if status[e] == "viewing" then		
		if is_triggered[e] == 0 and U.PlayerCloserThan(e,view_activator[e].view_distance) and U.PlayerLookingAt(e,view_activator[e].view_distance) then
			PlaySound(e,0)
			PromptDuration(view_activator[e].activation_message,2000)			
			time_delay[e] = (g_Time + view_activator[e].activation_delay)
			status[e] = "activated"
		end
	end	
	if status[e] == "activated" then
		if g_Time > time_delay[e] then
			ActivateIfUsed(e)
			PerformLogicConnections(e)						
			SetActivated(e,1)			
			is_triggered[e] = 1
			status[e] = "endview"
		end	
	end	
	if status[e] == "endview" then		
		SwitchScript(e,"no_behavior_selected.lua")			
	end
end

function view_activator_exit(e)
end