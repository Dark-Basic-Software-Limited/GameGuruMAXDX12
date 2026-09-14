-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Prefill Zone v3 by Necrym59
-- DESCRIPTION: Will allow to prefill a players inventory with up to five designated entities when entering or starting on this zone and then will delete the zone.
-- DESCRIPTION: Entities must be set as 'collectable' and or 'resource' and in the 'Collection List'.
-- DESCRIPTION: [PROMPT_TEXT$="Prefilling Items"]
-- DESCRIPTION: [ENTITY1_NAME$=""]
-- DESCRIPTION: [ENTITY2_NAME$=""]
-- DESCRIPTION: [ENTITY3_NAME$=""]
-- DESCRIPTION: [ENTITY4_NAME$=""]
-- DESCRIPTION: [ENTITY5_NAME$=""]
-- DESCRIPTION: [ZONEHEIGHT=100(0,1000)]
-- DESCRIPTION: [SpawnAtStart!=1] if unchecked use a switch or other trigger to spawn this zone
-- DESCRIPTION: [ActivateLogic!=0]
-- DESCRIPTION: <Sound0> - Activation Sound

local lower = string.lower
local prefill_zone 			= {}
local prompt_text 			= {}
local prompt_display		= {}
local entity1_name			= {}
local entity2_name			= {}
local entity3_name			= {}
local entity4_name			= {}
local entity5_name			= {}
local zoneheight			= {}
local spawnatstart			= {}
local activatelogic			= {}

local status		= {}
local entity1_no	= {}
local entity2_no	= {}
local entity3_no	= {}
local entity4_no	= {}
local entity5_no	= {}

function prefill_zone_properties(e, prompt_text, entity1_name, entity2_name, entity3_name, entity4_name, entity5_name, zoneheight, spawnatstart, activatelogic)
	prefill_zone[e].prompt_text = prompt_text
	prefill_zone[e].entity1_name = string.lower(entity1_name)
	prefill_zone[e].entity2_name = string.lower(entity2_name)
	prefill_zone[e].entity3_name = string.lower(entity3_name)
	prefill_zone[e].entity4_name = string.lower(entity4_name)
	prefill_zone[e].entity5_name = string.lower(entity5_name)	
	prefill_zone[e].zoneheight = zoneheight or 100
	prefill_zone[e].spawnatstart = spawnatstart or 1
	prefill_zone[e].activatelogic = activatelogic or 0
end

function prefill_zone_init(e)
	prefill_zone[e] = {}
	prefill_zone[e].prompt_text = ""
	prefill_zone[e].entity1_name = ""
	prefill_zone[e].entity2_name = ""
	prefill_zone[e].entity3_name = ""
	prefill_zone[e].entity4_name = ""
	prefill_zone[e].entity5_name = ""
	prefill_zone[e].zoneheight = 100
	prefill_zone[e].spawnatstart = 1
	prefill_zone[e].activatelogic = 0
	
	status[e] = "init"
	entity1_no[e] = 0
	entity2_no[e] = 0
	entity3_no[e] = 0
	entity4_no[e] = 0
	entity5_no[e] = 0
end

function prefill_zone_main(e)

	if status[e] == "init" then
		if prefill_zone[e].spawnatstart == 1 then SetActivated(e,1) end
		if prefill_zone[e].spawnatstart == 0 then SetActivated(e,0) end
		if prefill_zone[e].entity1_name ~= "" and entity1_no[e] == 0 then
			for p = 1, g_EntityElementMax do
				if p ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == prefill_zone[e].entity1_name then
						entity1_no[e] = p
						SetEntityAlwaysActive(p,1)
					end
				end
			end
		end
		if prefill_zone[e].entity2_name ~= "" and entity2_no[e] == 0 then
			for p = 1, g_EntityElementMax do
				if p ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == prefill_zone[e].entity2_name then
						entity2_no[e] = p
						SetEntityAlwaysActive(p,1)
					end
				end
			end
		end
		if prefill_zone[e].entity3_name ~= "" and entity3_no[e] == 0 then
			for p = 1, g_EntityElementMax do
				if r ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == prefill_zone[e].entity3_name then
						entity3_no[e] = p
						SetEntityAlwaysActive(p,1)
					end
				end
			end
		end
		if prefill_zone[e].entity4_name ~= "" and entity4_no[e] == 0 then
			for p = 1, g_EntityElementMax do
				if r ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == prefill_zone[e].entity4_name then
						entity4_no[e] = p
						SetEntityAlwaysActive(p,1)
					end
				end
			end
		end
		if prefill_zone[e].entity5_name ~= "" and entity5_no[e] == 0 then
			for p = 1, g_EntityElementMax do
				if r ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == prefill_zone[e].entity5_name then
						entity5_no[e] = p
						SetEntityAlwaysActive(p,1)
					end
				end
			end
		end		
		status[e] = "endinit"
	end

	if g_Entity[e]['activated'] == 1 then
		if g_Entity[e]['plrinzone'] == 1 and g_PlayerHealth > 0 and g_PlayerPosY < g_Entity[e]['y']+prefill_zone[e].zoneheight then
			PromptDuration(prefill_zone[e].prompt_text,2000)
			PlayNon3DSound(e,0)
			if prefill_zone[e].activatelogic == 1 then 
				ActivateIfUsed(e)
				PerformLogicConnections(e)
			end	
			if entity1_no[e] ~= 0 and GetEntityCollectable(entity1_no[e]) == 1 or GetEntityCollectable(entity1_no[e]) == 2 then SetEntityCollected(entity1_no[e],1) end			
			if entity2_no[e] ~= 0 and GetEntityCollectable(entity2_no[e]) == 1 or GetEntityCollectable(entity2_no[e]) == 2 then SetEntityCollected(entity2_no[e],1) end
			if entity3_no[e] ~= 0 and GetEntityCollectable(entity3_no[e]) == 1 or GetEntityCollectable(entity3_no[e]) == 2 then SetEntityCollected(entity3_no[e],1) end
			if entity4_no[e] ~= 0 and GetEntityCollectable(entity4_no[e]) == 1 or GetEntityCollectable(entity4_no[e]) == 2 then SetEntityCollected(entity4_no[e],1) end
			if entity5_no[e] ~= 0 and GetEntityCollectable(entity5_no[e]) == 1 or GetEntityCollectable(entity5_no[e]) == 2 then SetEntityCollected(entity5_no[e],1) end
			SetActivated(e,0)
			Destroy(e)			
		end
	end
end

function prefill_zone_exit(e)
end