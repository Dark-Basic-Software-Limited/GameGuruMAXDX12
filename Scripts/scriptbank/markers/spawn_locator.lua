-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Spawn Locator v8 by Necrym59
-- DESCRIPTION: Will relocate the player to a designated spawn marker. Place under the Player Start Marker or use as a zone.
-- DESCRIPTION: [@@SPAWN_MARKER_USER_GLOBAL$=""(0=globallist)] user global required for using spawn markers (eg: MySpawnMarkers)
-- DESCRIPTION: [SPAWN_MARKER_NAME$=""] for optional specific spawning, leave blank if assigned via other behaviors.
-- DESCRIPTION: [SPAWN_X_ADJUST=0(-500,500)] Allows adjustment of spawn locator x position
-- DESCRIPTION: [SPAWN_Y_ADJUST=0(-500,500)] Allows adjustment of spawn locator y position
-- DESCRIPTION: [SPAWN_Z_ADJUST=0(-500,500)] Allows adjustment of spawn locator z position
-- DESCRIPTION: [@MARKER_HIDDEN=1(1=No,2=Yes)] Hides the named spawn marker
-- DESCRIPTION: [@MARKER_COLLISION=1(1=Off,2=On)] Enable/Disable marker_collision on the named spawn marker
-- DESCRIPTION: [@ACTIVATION=1(1=Always On,2=Deactivate after use)] Activation/Deactivation for this zone
-- DESCRIPTION: [ZONEHEIGHT=100(0,1000)]
-- DESCRIPTION: [SpawnAtStart!=1] if unchecked use a switch or other trigger to spawn this zone

local lower = string.lower
local spawn_locator = {}
local spawn_marker_user_global = {}
local spawn_marker_name = {}
local spawn_x_adjust = {}
local spawn_y_adjust = {}
local spawn_z_adjust = {}
local marker_hidden = {}
local marker_collision = {}
local activation = {}
local zoneheight = {}
local spawnatstart = {}

local spawn_marker_number = {}
local pos_x	= {}
local pos_y	= {}
local pos_z	= {}
local ang_y	= {}
local status = {}

function spawn_locator_properties(e, spawn_marker_user_global, spawn_marker_name, spawn_x_adjust, spawn_y_adjust, spawn_z_adjust, marker_hidden, marker_collision, activation, zoneheight, spawnatstart)
	spawn_locator[e].spawn_marker_user_global = spawn_marker_user_global
	spawn_locator[e].spawn_marker_name = lower(spawn_marker_name) or nil
	spawn_locator[e].spawn_x_adjust = spawn_x_adjust or 0
	spawn_locator[e].spawn_y_adjust = spawn_y_adjust or 0
	spawn_locator[e].spawn_z_adjust = spawn_z_adjust or 0
	spawn_locator[e].marker_hidden = marker_hidden or 1
	spawn_locator[e].marker_collision = marker_collision or 1
	spawn_locator[e].activation = activation or 1
	spawn_locator[e].zoneheight = zoneheight
end

function spawn_locator_init(e)
	spawn_locator[e] = {}
	spawn_locator[e].spawn_marker_user_global = ""
	spawn_locator[e].spawn_marker_name = ""
	spawn_locator[e].spawn_x_adjust = 0
	spawn_locator[e].spawn_y_adjust = 0
	spawn_locator[e].spawn_z_adjust = 0
	spawn_locator[e].marker_hidden = 1
	spawn_locator[e].marker_collision = 1
	spawn_locator[e].activation = 1
	spawn_locator[e].zoneheight = 100

	if spawn_locator[e].spawnatstart == 1 then SetActivated(e,1) end
	if spawn_locator[e].spawnatstart == 0 then SetActivated(e,0) end
	spawn_locator[e].spawn_marker_number = 0
	status[e] = "init"
end

function spawn_locator_main(e)
	if status[e] == "init" then
		spawn_locator[e].spawn_marker_number = 0
		status[e] = "endinit"
	end

	if g_Entity[e]['activated'] == 1 then
		if _G["g_UserGlobal['"..spawn_locator[e].spawn_marker_user_global.."']"] ~= nil then
			if spawn_locator[e].spawn_marker_name == "" then
				spawn_locator[e].spawn_marker_name = _G["g_UserGlobal['"..spawn_locator[e].spawn_marker_user_global.."']"]
				spawn_locator[e].spawn_marker_number = 0
			end
			if spawn_locator[e].spawn_marker_name ~= "" then
				_G["g_UserGlobal['"..spawn_locator[e].spawn_marker_user_global.."']"] = spawn_locator[e].spawn_marker_name
				spawn_locator[e].spawn_marker_name = _G["g_UserGlobal['"..spawn_locator[e].spawn_marker_user_global.."']"]
				spawn_locator[e].spawn_marker_number = 0
			end
		end

		if spawn_locator[e].spawn_marker_number == 0 and spawn_locator[e].spawn_marker_name ~= "" then
			for ee = 1, g_EntityElementMax do
				if ee ~= nil and g_Entity[ee] ~= nil then
					if lower(GetEntityName(ee)) == lower(spawn_locator[e].spawn_marker_name) then
						spawn_locator[e].spawn_marker_number = ee
						pos_x[e] = g_Entity[ee]['x'] + spawn_locator[e].spawn_x_adjust
						pos_y[e] = g_Entity[ee]['y'] + spawn_locator[e].spawn_y_adjust
						pos_z[e] = g_Entity[ee]['z'] + spawn_locator[e].spawn_z_adjust
						ang_y[e] = g_Entity[ee]['angley']
						GravityOff(ee)
						if spawn_locator[e].marker_hidden == 2 then Hide(ee) end
						if spawn_locator[e].marker_collision == 2 then	CollisionOn(ee) end
						status[e] = "spawnplayer"
						break
					end
				end
			end
		end

		if status[e] == "spawnplayer" then
			if g_Entity[e]['plrinzone'] == 1 and g_PlayerPosY > g_Entity[e]['y'] and g_PlayerPosY < g_Entity[e]['y']+spawn_locator[e].zoneheight then
				if spawn_locator[e].marker_number ~= 0 then
					SetFreezePosition(pos_x[e],pos_y[e]+35,pos_z[e])
					TransportToFreezePositionOnly()
					SetGamePlayerControlFinalCameraAngley(ang_y[e])
					if spawn_locator[e].activation == 1 then SetActivated(e,1) end
					if spawn_locator[e].activation == 2 then SetActivated(e,0) end
					status[e] = "init"
				end
			end
		end
	end
end

