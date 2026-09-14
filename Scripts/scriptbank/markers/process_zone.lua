-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Process Zone v3
-- DESCRIPTION: Will allow to activate/decativate/destroy all of a named entity when entering in this zone then destroy the zone.
-- DESCRIPTION: Attach to a trigger zone
-- DESCRIPTION: [ENTITY_NAME$=""]
-- DESCRIPTION: [@PROCESS_MODE=1(1=Activate,2=Deactivate,3=Destroy)]
-- DESCRIPTION: [PROCESS_TEXT$="All Entities Activated/Deactivated"]
-- DESCRIPTION: [ZONEHEIGHT=100(0,1000)]
-- DESCRIPTION: [SpawnAtStart!=1] if unchecked use a switch or other trigger to spawn this zone
-- DESCRIPTION: <Sound0> for when entering the zone

local lower = string.lower
local process_zone 			= {}
local entity_name			= {}
local process_mode			= {}
local process_text 			= {}
local zoneheight			= {}
local spawnatstart			= {}

local tableName 			= {}
local tableEnts 			= {}
local status 				= {}
local played				= {}

function process_zone_properties(e, entity_name, process_mode, process_text, zoneheight, spawnatstart)
	process_zone[e].entity_name = lower(entity_name)
	process_zone[e].process_mode = process_mode
	process_zone[e].process_text = process_text
	process_zone[e].zoneheight = zoneheight or 100
	process_zone[e].spawnatstart = spawnatstart
end

function process_zone_init(e)
	process_zone[e] = {}
	process_zone[e].entity_name = ""
	process_zone[e].process_mode = 1
	process_zone[e].process_text = "All Entities Activated/Deactivated/Destroyed"
	process_zone[e].zoneheight = 100
	process_zone[e].spawnatstart = 1

	tableName[e] = "processlist" ..tostring(e)
	_G[tableName[e]] = {}
	tableEnts[e] = 0
	status[e] = "init"
	played[e] = 0
end

function process_zone_main(e)

	if status[e] == "init" then
		if process_zone[e].entity_name > "" then
			for n = 1, g_EntityElementMax do
				if n ~= nil and g_Entity[n] ~= nil then				
					if lower(GetEntityName(n)) == process_zone[e].entity_name then
						table.insert(_G[tableName[e]],n)
						tableEnts[e] = tableEnts[e] + 1
					end
				end
			end
		end	
		if process_zone[e].spawnatstart == 1 then SetActivated(e,1) end
		if process_zone[e].spawnatstart == 0 then SetActivated(e,0) end
		status[e] = "endinit"
	end

	if g_Entity[e]['activated'] == 1 then
		if g_Entity[e]['plrinzone'] == 1 and g_PlayerPosY > g_Entity[e]['y'] and g_PlayerPosY < g_Entity[e]['y']+process_zone[e].zoneheight then
			if played[e] == 0 then
				PlaySound(e,0)
				played[e] = 1
			end
			------------------------
			if tableEnts[e] > 0 then
				if process_zone[e].process_mode == 1 then --Activate
					for a,b in pairs(_G[tableName[e]]) do						
						SetActivated(b,1)
						SetEntityIfUsed(e,process_zone[e].entity_name)
						ActivateIfUsed(e)
					end
				end
				if process_zone[e].process_mode == 2 then --DeActivate
					for a,b in pairs(_G[tableName[e]]) do						
						SetActivated(b,0)
					end
				end
				if process_zone[e].process_mode == 3 then --Destroy
					for a,b in pairs(_G[tableName[e]]) do						
						SetActivated(b,0)
						Destroy(b)
					end
				end
			end
			PerformLogicConnections(e)
			PromptDuration(process_zone[e].process_text,3000)
			Destroy(e)			
		end
	end
end

function process_zone_exit(e)
end