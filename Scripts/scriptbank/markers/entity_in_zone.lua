-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Entity In Zone v11 by Necrym59 and Lee
-- DESCRIPTION: If entity of selected type and allegiance enters the zone, will activate linked or IfUsed entities then can destroy zone.
-- DESCRIPTION: [@ENTITY_TYPE=1(1=Active Object Only, 2=Active Character Only, 3=Active Non-Character Only, 4=Non-Static Objects)]
-- DESCRIPTION: [@ALLEGIANCE=3(0=Enemy, 1=Ally, 2=Neutral, 3=None, 4=Any)]
-- DESCRIPTION: [NOTIFICATION$="Entity in zone"]
-- DESCRIPTION: [ZONEHEIGHT=100(0,1000)]
-- DESCRIPTION: [SpawnAtStart!=1] if unchecked use a switch or other trigger to spawn this zone
-- DESCRIPTION: [SelfDestroy!=1] if checked will destroy zone after activation/detection
-- DESCRIPTION: Plays <Sound0> when triggered.

local entityinzone 		= {}
local entity_type 		= {}
local allegiance 		= {}
local zoneheight		= {}
local spawnatstart		= {}
local selfdestroy		= {}

local EntityID			= {}
local EntityAL			= {}
local wait				= {}
local played 			= {}
local doonce			= {}
local status			= {}
	
function entity_in_zone_properties(e, entity_type, allegiance, notification, zoneheight, spawnatstart, selfdestroy)
	entityinzone[e].entity_type = entity_type
	entityinzone[e].allegiance = allegiance
	entityinzone[e].notification = notification
	entityinzone[e].zoneheight = zoneheight
	entityinzone[e].spawnatstart = spawnatstart
	entityinzone[e].selfdestroy = selfdestroy
end
 
function entity_in_zone_init(e)
	entityinzone[e] = {}
	entityinzone[e].entity_type = 0
	entityinzone[e].allegiance = 3	
	entityinzone[e].notification = "Entity in zone"
	entityinzone[e].zoneheight = 100
	entityinzone[e].spawnatstart = 1
	entityinzone[e].selfdestroy = 1
	
	status[e] = "init"
	played[e] = 0
	doonce[e] = 0
	wait[e] = math.huge
	EntityID[e] = 0
	EntityAL[e] = 0
end
 
function entity_in_zone_main(e)	
	if status[e] == "init" then
		if entityinzone[e].spawnatstart == 1 then SetActivated(e,1) end
		if entityinzone[e].spawnatstart == 0 then SetActivated(e,0) end
		status[e] = "endinit"
	end
	if g_Entity[e]['activated'] == 1 then		
		GetEntityInZoneWithFilter(e,entityinzone[e].entity_type)
		EntityID[e] = g_Entity[e]['entityinzone']
		EntityAL[e] = GetEntityAllegiance(EntityID[e])		
		if g_Entity[e]['entityinzone'] > 0 and EntityID[e] > 0 and g_Entity[EntityID[e]] ~= nil and g_Entity[EntityID[e]]['y'] > g_Entity[e]['y']-1 and g_Entity[EntityID[e]]['y'] < g_Entity[e]['y']+entityinzone[e].zoneheight then
		
			if entityinzone[e].entity_type == 1 then --Active Object Only
				if EntityAL[e] == entityinzone[e].allegiance or entityinzone[e].allegiance == 4 then 
					Prompt(entityinzone[e].notification)
					if played[e] == 0 then
						PlaySound(e,0)
						played[e] = 1
					end	
					if doonce[e] == 0 then
						ActivateIfUsed(e)
						PerformLogicConnections(e)
						doonce[e] = 1
						wait[e] = g_Time + 2000
					end
				end
			end
			if entityinzone[e].entity_type == 2 then --Active Character Only
				if EntityAL[e] == entityinzone[e].allegiance or entityinzone[e].allegiance == 4 then
					Prompt(entityinzone[e].notification)
					if played[e] == 0 then
						PlaySound(e,0)
						played[e] = 1
					end	
					if doonce[e] == 0 then
						ActivateIfUsed(e)
						PerformLogicConnections(e)
						doonce[e] = 1
						wait[e] = g_Time + 2000
					end
				end
			end
			if entityinzone[e].entity_type == 3 then --Active Non-Character Only
				if EntityAL[e] == entityinzone[e].allegiance or entityinzone[e].allegiance == 4 then
					Prompt(entityinzone[e].notification)
					if played[e] == 0 then
						PlaySound(e,0)
						played[e] = 1
					end	
					if doonce[e] == 0 then
						ActivateIfUsed(e)
						PerformLogicConnections(e)
						doonce[e] = 1
						wait[e] = g_Time + 2000
					end
				end
			end
			if entityinzone[e].entity_type == 4 then --Non-Static Objects
				if EntityAL[e] == entityinzone[e].allegiance or entityinzone[e].allegiance == 4 then
					Prompt(entityinzone[e].notification)
					if played[e] == 0 then
						PlaySound(e,0)
						played[e] = 1
					end	
					if doonce[e] == 0 then
						ActivateIfUsed(e)
						PerformLogicConnections(e)
						doonce[e] = 1
						wait[e] = g_Time + 2000
					end
				end
			end
		end	
	end
	if g_Time > wait[e] then
		if entityinzone[e].selfdestroy ==  1 then Destroy(e) end
		played[e] = 0
		doonce[e] = 0
	end
end