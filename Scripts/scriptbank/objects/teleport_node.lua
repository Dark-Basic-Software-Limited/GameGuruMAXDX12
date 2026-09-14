-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Teleport_Node v2: by Necrym59
-- DESCRIPTION: This object will be treated as an externally activated teleport to a local connected point or to another level
-- DESCRIPTION: [@DESTINATION=1(1=Local, 2=Level)]
-- DESCRIPTION: [EXIT_ANGLE=1(1,360))] Player exit angle upon teleport
-- DESCRIPTION: [@@SPAWN_MARKER_USER_GLOBAL$=""(0=globallist)] user global required for using spawn markers (eg: MySpawnMarkers)
-- DESCRIPTION: [SPAWN_MARKER_NAME$=""] for optional spawning using spawn markers, can be assigned via other behaviors
-- DESCRIPTION: [@GoToLevelMode=1(1=Use Storyboard Logic,2=Go to Specific Level)] controls whether the next level in the Storyboard, or another level is loaded after entry to the zone.
-- DESCRIPTION: [ResetStates!=0] when entering the next level
-- DESCRIPTION: Play <Sound0> when activated.

local lower = string.lower
local teleport_node 			= {}
local destination 				= {}
local exit_angle 				= {}
local spawn_marker_user_global	= {}
local spawn_marker_name			= {}
local resetstates 				= {}

local dest_angle 		= {}
local current_spawn		= {}
local status 			= {}
local doonce			= {}

function teleport_node_properties(e, destination, exit_angle, spawn_marker_user_global, spawn_marker_name, resetstates)
	teleport_node[e].destination = destination or 1	
	teleport_node[e].exit_angle = exit_angle		
	teleport_node[e].spawn_marker_user_global = spawn_marker_user_global
	teleport_node[e].spawn_marker_name = lower(spawn_marker_name)
	teleport_node[e].resetstates = resetstates or 0
end

function teleport_node_init(e)
	teleport_node[e] = {}
	teleport_node[e].destination = 1	
	teleport_node[e].exit_angle = 1		
	teleport_node[e].spawn_marker_user_global = ""
	teleport_node[e].spawn_marker_name = ""
	teleport_node[e].resetstates = 0	
	
	dest_angle[e] = 0
	current_spawn[e] = ""
	doonce[e] = 0
	status = "init"
end

function teleport_node_main(e)

	if status == "init" then
		SetActivated(e,0)		
		dest_angle[e] = teleport_node[e].exit_angle
		status = "endinit"
	end
	
	if g_Entity[e]['activated'] == 1 then
		if _G["g_UserGlobal['"..teleport_node[e].spawn_marker_user_global.."']"] ~= nil then
			if teleport_node[e].spawn_marker_name == "" then current_spawn[e] = _G["g_UserGlobal['"..teleport_node[e].spawn_marker_user_global.."']"] end
			if teleport_node[e].spawn_marker_name ~= "" then current_spawn[e] = teleport_node[e].spawn_marker_name end
			_G["g_UserGlobal['"..teleport_node[e].spawn_marker_user_global.."']"] = current_spawn[e]
		end
		
		if teleport_node[e].destination == 1 and doonce[e] == 0 then
			PlaySound(e,0)				
			TransportToIfUsed(e)
			SetGamePlayerControlFinalCameraAngley(dest_angle[e])
			doonce[e] = 1
		end
		if teleport_node[e].destination == 2 and doonce[e] == 0 then
			PlaySound(e,0)
			SetGamePlayerControlFinalCameraAngley(dest_angle[e])
			JumpToLevelIfUsedEx(e,teleport_node[e].resetstates)
			doonce[e] = 1			
		end
		if doonce[e] == 1 then
			doonce[e] = 0
			status = "init"
		end
	end
end
