-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Object Monitor v15 by Necrym59
-- DESCRIPTION: A behavior that allows a named object/s health to be monitored and trigger event(s), Lose/Win Game, Change Level or Destroy a named entity upon reaching zero health.
-- DESCRIPTION: Attach to an object set AlwaysActive=ON, and attach any logic links to this object and/or use ActivateIfUsed field.
-- DESCRIPTION: [OBJECT_NAME$=""] to monitor
-- DESCRIPTION: [@DESTROYED_ACTION=1(1=Event Triggers, 2=Lose Game, 3=Win Game, 4=Go To Level, 5=Add to User Global, 6=Deduct from User Global, 7=Destroy Named Entity)]
-- DESCRIPTION: [ENTITY_NAME$=""] to destroy
-- DESCRIPTION: [@MONITOR_ACTIVE=1(1=Yes, 2=No)] if No then use a zone or switch to activate this monitor.
-- DESCRIPTION: [ACTION_DELAY=2(0,100)] seconds delay before activating destroyed_action.
-- DESCRIPTION: [@@USER_GLOBAL$=""(0=globallist)] user global to apply value (eg: MyGlobal).
-- DESCRIPTION: [USER_GLOBAL_VALUE=100(1,100)] value to apply.
-- DESCRIPTION: [@GoToLevelMode=1(1=Use Storyboard Logic,2=Go to Specific Level)] controls whether the next level in the Storyboard, or another level is loaded.
-- DESCRIPTION: [ResetStates!=0] when entering the new level

local lower = string.lower

local object_monitor 	= {}
local object_name		= {}
local destroyed_action	= {}
local entity_name		= {}
local monitor_active	= {}
local action_delay		= {}
local user_global 		= {}
local user_global_value	= {}
local resetstates		= {}

local dEntn				= {}
local mEnts				= {}
local status			= {}
local wait				= {}
local actiondelay		= {}
local checktime			= {}
local currentvalue		= {}
local tableName 		= {}


function object_monitor_properties(e, object_name, destroyed_action, entity_name, monitor_active, action_delay, user_global, user_global_value, resetstates)
	object_monitor[e].object_name = lower(object_name) or ""
	object_monitor[e].destroyed_action = destroyed_action
	object_monitor[e].entity_name = lower(entity_name) or ""
	object_monitor[e].monitor_active = monitor_active or 1
	object_monitor[e].action_delay = action_delay or 0	
	object_monitor[e].user_global = user_global
	object_monitor[e].user_global_value = user_global_value
	object_monitor[e].resetstates = resetstates
end

function object_monitor_init(e)
	object_monitor[e] = {}
	object_monitor[e].object_name = ""
	object_monitor[e].destroyed_action = 1
	object_monitor[e].entity_name = ""	
	object_monitor[e].monitor_active = 1
	object_monitor[e].action_delay = 3		
	object_monitor[e].user_global = ""
	object_monitor[e].user_global_value = 100
	object_monitor[e].resetstates = 0
	
	wait[e] = math.huge
	actiondelay[e] = math.huge
	checktime[e] = 0
	currentvalue[e] = 0
	dEntn[e] = 0
	mEnts[e] = 0
	tableName[e] = "monitorlist" ..tostring(e)
	_G[tableName[e]] = {}	
	status[e] = "init"	
end

function object_monitor_main(e)

	if status[e] == "init" then
		checktime[e] = g_Time + 500
		if object_monitor[e].monitor_active == 1 then SetEntityActivated(e,1) end
		if object_monitor[e].monitor_active == 2 then SetEntityActivated(e,0) end		

		if object_monitor[e].object_name ~= "" then
			for n = 1, g_EntityElementMax do
				if n ~= nil and g_Entity[n] ~= nil then
					if lower(GetEntityName(n)) == object_monitor[e].object_name then 
						table.insert(_G[tableName[e]],n)
						mEnts[e] = mEnts[e]+1						
					end
				end
			end
		end
		if dEntn[e] == 0 then
			for m = 1, g_EntityElementMax do
				if m ~= nil and g_Entity[m] ~= nil then
					if lower(GetEntityName(m)) == object_monitor[e].entity_name then 
						dEntn[e] = m
						break
					end
				end
			end
		end
		status[e] = "monitor"
	end			

	if g_Entity[e]['activated'] == 1 then
		if status[e] == "monitor" and g_Time > checktime[e] then
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 1 then
					wait[e] = g_Time + (object_monitor[e].action_delay*1000)				
					status[e] = "alarm"
			end
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 2 then
				wait[e] = g_Time + (object_monitor[e].action_delay*1000)
				status[e] = "winorlose"
			end
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 3 then
				wait[e] = g_Time + (object_monitor[e].action_delay*1000)
				status[e] = "winorlose"
			end
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 4 then
				if mEnts[e] == 0 then
					wait[e] = g_Time + (object_monitor[e].action_delay*1000)
					status[e] = "winorlose"
				end	
			end
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 5 then
				if _G["g_UserGlobal['"..object_monitor[e].user_global.."']"] ~= nil then
					currentvalue[e] = _G["g_UserGlobal['"..object_monitor[e].user_global.."']"]
					_G["g_UserGlobal['"..object_monitor[e].user_global.."']"] = currentvalue[e] + object_monitor[e].user_global_value
				end
				status[e] = "end"
				SwitchScript(e,"no_behavior_selected.lua")
			end
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 6 then
				if _G["g_UserGlobal['"..object_monitor[e].user_global.."']"] ~= nil then
					currentvalue[e] = _G["g_UserGlobal['"..object_monitor[e].user_global.."']"]
					_G["g_UserGlobal['"..object_monitor[e].user_global.."']"] = currentvalue[e] - object_monitor[e].user_global_value
				end
				status[e] = "end"
				SwitchScript(e,"no_behavior_selected.lua")
			end
			if mEnts[e] == 0 and object_monitor[e].destroyed_action == 7 then
				wait[e] = g_Time + (object_monitor[e].action_delay*1000)				
				status[e] = "destroy"
			end			
			checktime[e] = g_Time + 500
		end

		if status[e] == "alarm" then			
			if g_Time < wait[e] then MakeAISound(g_PlayerPosX,g_PlayerPosY,g_PlayerPosZ,3000,1,-1) end
			if g_Time > wait[e] then
				ActivateIfUsed(e)
				PerformLogicConnections(e)
				status[e] = "end"
				SwitchScript(e,"no_behavior_selected.lua")
			end
		end
		
		if status[e] == "winorlose" then
			if g_Time > wait[e] then
				if object_monitor[e].destroyed_action == 2 then LoseGame() end
				if object_monitor[e].destroyed_action == 3 then WinGame() end
				if object_monitor[e].destroyed_action == 4 then JumpToLevelIfUsedEx(e,object_monitor[e].resetstates) end
				status[e] = "end"
			end
		end
		
		if status[e] == "destroy" then			
			if g_Time > wait[e] then
				Destroy(dEntn[e])
				status[e] = "end"
				SwitchScript(e,"no_behavior_selected.lua")
			end
		end
		
		--Destroyed Monitored Objects check--
		for _,a in pairs (_G[tableName[e]]) do
			if g_Entity[a] ~= nil then
				if g_Entity[a]['health'] <= 0 then						
					table.remove(_G[tableName[e]], tableFind(_G[tableName[e]],a))
					mEnts[e] = mEnts[e] -1
				end
			end
		end		
	end	
end

function tableFind(tbl, value)
    for key, val in pairs(tbl) do
        if val == value then
            return key
        end
    end
    return nil
end