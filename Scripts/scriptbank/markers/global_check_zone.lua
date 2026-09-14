-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Global Check Zone v3 by Necrym59
-- DESCRIPTION: Will compare/check two user globals to action a result to a third user global value when zone is entered by player or designated entity type.
-- DESCRIPTION: [@ENTITY_TYPE=5(1=Active Object Only, 2=Active Character Only, 3=Active Non-Character Only, 4=Non-Static Objects, 5=Player Character Only)]
-- DESCRIPTION: [@@CHECK_GLOBAL1$=""(0=globallist)] eg; MyValue
-- DESCRIPTION: [@@CHECK_GLOBAL2$=""(0=globallist)] eg; MyValue
-- DESCRIPTION: [@CHECK_TYPE=1(1=Global1 > Global2, 2=Global1 < Global2)]
-- DESCRIPTION: [@RESULT_ACTION=1(1=Add Value,2=Deduct Value)] result action
-- DESCRIPTION: [RESULT_VALUE=0(0,1000)] value to update result global
-- DESCRIPTION: [@@RESULT_GLOBAL$=""(0=globallist)] eg; MyValue
-- DESCRIPTION: [MESSAGE_TEXT$="Result Global Updated"]
-- DESCRIPTION: [ZoneHeight=100(1,1000)]
-- DESCRIPTION: [SpawnAtStart!=1] if unchecked use a switch or other trigger to spawn this zone
-- DESCRIPTION: [ActivateLogic!=0] if checked will trigger linked or IfUSed entities
-- DESCRIPTION: <Sound0> - When entering zone Sound
-- DESCRIPTION: <Sound1> - Action Sound

local glcheckzone 		= {}
local entity_type 		= {}
local check_global1 	= {}
local check_global2 	= {}
local check_type		= {}
local result_action		= {}
local result_value		= {}
local result_global 	= {}
local message_text		= {}
local zoneheight		= {}
local spawnatstart 		= {}
local activatelogic 	= {}

local status = {}
local result = {}
local played = {}
local doonce = {}
local wait = {}
local currentvalue	= {}
local EntityID = {}

function global_check_zone_properties(e, entity_type, check_global1, check_global2, check_type, result_action, result_value, result_global, message_text, zoneheight, spawnatstart, activatelogic)
	glcheckzone[e].entity_type = entity_type
	glcheckzone[e].check_global1 = check_global1
	glcheckzone[e].check_global2 = check_global2
	glcheckzone[e].check_type = check_type or 1	
	glcheckzone[e].result_action = result_action or 1
	glcheckzone[e].result_value = result_value
	glcheckzone[e].result_global = result_global
	glcheckzone[e].message_text = message_text
	glcheckzone[e].zoneheight = zoneheight or 100
	glcheckzone[e].spawnatstart = spawnatstart or 1
	glcheckzone[e].activatelogic = activatelogic or 0	
end 

function global_check_zone_init(e)
	glcheckzone[e] = {}
	glcheckzone[e].entity_type = 5
	glcheckzone[e].check_global1 = ""
	glcheckzone[e].check_global2 = ""
	glcheckzone[e].check_type = check_type or 1	
	glcheckzone[e].result_action = result_action or 1
	glcheckzone[e].result_value = 0
	glcheckzone[e].result_global = ""
	glcheckzone[e].message_text = "Result Global Updated"
	glcheckzone[e].zoneheight = zoneheight or 100
	glcheckzone[e].spawnatstart = spawnatstart or 1
	glcheckzone[e].activatelogic = activatelogic or 0	
	
	status[e] = "init"
	result[e] = 0
	played[e] = 0
	doonce[e] = 0
	currentvalue[e] = 0
	wait[e] = math.huge
	EntityID[e] = 0
end

function global_check_zone_main(e)
	if status[e] == "init" then
		if glcheckzone[e].spawnatstart == 1 then SetActivated(e,1) end
		if glcheckzone[e].spawnatstart == 0 then SetActivated(e,0) end
		status[e] = "endinit"
	end
	
	if g_Entity[e]['activated'] == 1 then
		if _G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] end
		if glcheckzone[e].entity_type < 5 then
			GetEntityInZoneWithFilter(e,glcheckzone[e].entity_type)
			EntityID[e] = g_Entity[e]['entityinzone']		
			if g_Entity[e]['entityinzone'] > 0 and EntityID[e] > 0 and g_Entity[EntityID[e]] ~= nil and g_Entity[EntityID[e]]['y'] > g_Entity[e]['y']-1 and g_Entity[EntityID[e]]['y'] < g_Entity[e]['y']+ glcheckzone[e].zoneheight then
				if played[e] == 0 then
					PlaySound(e,0)
					played[e] = 1
				end
				if doonce[e] == 0 then
					Prompt(glcheckzone[e].message_text)
					if glcheckzone[e].check_type == 1 then 
						if _G["g_UserGlobal['"..glcheckzone[e].check_global1.."']"] > _G["g_UserGlobal['"..glcheckzone[e].check_global2.."']"] then result[e] = 1 end						
					end
					if glcheckzone[e].check_type == 2 then
						if _G["g_UserGlobal['"..glcheckzone[e].check_global1.."']"] < _G["g_UserGlobal['"..glcheckzone[e].check_global2.."']"] then result[e] = 1 end						
					end
					
					if result[e] == 1 then
						if glcheckzone[e].result_action == 1 then --Add
							_G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] = currentvalue[e] + glcheckzone[e].result_value
						end
						if glcheckzone[e].result_action == 2 then --Deduct
							_G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] = currentvalue[e] - glcheckzone[e].result_value
						end						
						if glcheckzone[e].activatelogic == 1 then
							ActivateIfUsed(e)
							PerformLogicConnections(e)
						end
						PlaySound(e,1)
					end
					doonce[e] = 1
				end	
			end
			if g_Entity[e]['entityinzone'] == 0 then
				played[e] = 0
				doonce[e] = 0
				result[e] = 0
				currentvalue[e] = 0
			end			
		end
		
		if glcheckzone[e].entity_type == 5 then
			if _G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] end
			if g_Entity[e]['plrinzone'] == 1 and g_PlayerPosY > g_Entity[e]['y'] and g_PlayerPosY < g_Entity[e]['y'] + glcheckzone[e].zoneheight then
				if played[e] == 0 then
					PlaySound(e,0)
					played[e] = 1
				end
				if doonce[e] == 0 then
					Prompt(glcheckzone[e].message_text)
					if glcheckzone[e].check_type == 1 then 
						if _G["g_UserGlobal['"..glcheckzone[e].check_global1.."']"] > _G["g_UserGlobal['"..glcheckzone[e].check_global2.."']"] then result[e] = 1 end						
					end
					if glcheckzone[e].check_type == 2 then
						if _G["g_UserGlobal['"..glcheckzone[e].check_global1.."']"] < _G["g_UserGlobal['"..glcheckzone[e].check_global2.."']"] then result[e] = 1 end						
					end
					
					if result[e] == 1 then
						if glcheckzone[e].result_action == 1 then --Add
							_G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] = currentvalue[e] + glcheckzone[e].result_value
						end
						if glcheckzone[e].result_action == 2 then --Deduct
							_G["g_UserGlobal['"..glcheckzone[e].result_global.."']"] = currentvalue[e] - glcheckzone[e].result_value
						end						
						if glcheckzone[e].activatelogic == 1 then
							ActivateIfUsed(e)
							PerformLogicConnections(e)
						end
						PlaySound(e,1)
					end
					doonce[e] = 1
				end	
			end
			if g_Entity[e]['plrinzone'] == 0 then
				played[e] = 0
				doonce[e] = 0
				result[e] = 0
				currentvalue[e] = 0
			end
		end		
	end
end