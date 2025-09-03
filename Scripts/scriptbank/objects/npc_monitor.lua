-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- NPC Monitor v16 by Necrym59
-- DESCRIPTION: A global behavior that allows a named npc to be health monitored and trigger event(s) or Lose/Win game or go to a specified level upon its death.
-- DESCRIPTION: Attach to an object set AlwaysActive=ON, and attach any logic links to this object and/or use ActivateIfUsed field.
-- DESCRIPTION: [NPC_NAME$=""] to monitor.
-- DESCRIPTION: [@DEATH_ACTION=1(1=Event Triggers, 2=Lose Game, 3=Win Game, 4=Go To Level, 5=Add to User Global)]
-- DESCRIPTION: [@HEALTH_DISPLAY=4(1=Health Bar, 2=Health Text, 3=Health Text+Bar, 4=None)] displayed above npc.
-- DESCRIPTION: [@MONITOR_ACTIVE=1(1=Yes, 2=No)] if No then use a zone or switch to activate this monitor.
-- DESCRIPTION: [ACTION_DELAY=2(0,100)] seconds delay before activating death action.
-- DESCRIPTION: [@@USER_GLOBAL$=""(0=globallist)] user global to apply value (eg: MyGlobal).
-- DESCRIPTION: [USER_GLOBAL_VALUE=100(1,100)] value to apply.
-- DESCRIPTION: [HEALTH_BAR_IMAGEFILE$="imagebank\\buttons\\slider-bar-full.png"] health bar image to use.
-- DESCRIPTION: [Y_ADJUSTMENT=10(0,50)] health display y adjustment
-- DESCRIPTION: [@GoToLevelMode=1(1=Use Storyboard Logic,2=Go to Specific Level)] controls whether the next level in the Storyboard, or another level is loaded.
-- DESCRIPTION: [ResetStates!=0] when entering the new level

local P = require "scriptbank\\physlib"
local U = require "scriptbank\\utillib"
local lower = string.lower
g_LegacyNPC = {}

local npc_monitor 		= {}
local npc_name			= {}
local death_action		= {}
local health_display	= {}
local monitor_active	= {}
local action_delay		= {}
local user_global 		= {}
local user_global_value	= {}
local health_bar		= {}
local y_adjustment		= {}
local resetstates		= {}

local hbarsize			= {}
local hbarsprite		= {}
local hreadout			= {}
local pEntn				= {}
local rotheight			= {}
local status			= {}
local wait				= {}
local actiondelay		= {}
local checktime			= {}
local currentvalue		= {}

function npc_monitor_properties(e, npc_name, death_action, health_display, monitor_active, action_delay, user_global, user_global_value, health_bar_imagefile, y_adjustment, resetstates)
	npc_monitor[e].npc_name = lower(npc_name) or ""
	npc_monitor[e].death_action = death_action
	npc_monitor[e].health_display = health_display
	npc_monitor[e].monitor_active = monitor_active or 1
	npc_monitor[e].action_delay = action_delay or 0	
	npc_monitor[e].user_global = user_global
	npc_monitor[e].user_global_value = user_global_value
	npc_monitor[e].health_bar = health_bar_imagefile
	npc_monitor[e].y_adjustment = y_adjustment
	npc_monitor[e].resetstates = resetstates
end

function npc_monitor_init(e)
	npc_monitor[e] = {}
	npc_monitor[e].npc_name = ""
	npc_monitor[e].death_action = 1	
	npc_monitor[e].health_display = 4
	npc_monitor[e].monitor_active = 1
	npc_monitor[e].action_delay = 3		
	npc_monitor[e].user_global = ""
	npc_monitor[e].user_global_value = 100
	npc_monitor[e].health_bar = "imagebank\\buttons\\slider-bar-full.png"
	npc_monitor[e].y_adjustment = 10	
	npc_monitor[e].resetstates = 0
	
	status[e] = "init"
	hbarsize[e] = 0
	hbarsprite[e] = 0
	hreadout[e] = 0
	wait[e] = math.huge
	actiondelay[e] = math.huge
	checktime[e] = 0
	rotheight[e] = 0
	currentvalue[e] = 0
	pEntn[e] = 0
	g_LegacyNPC = 0
end

function npc_monitor_main(e)

	if status[e] == "init" then
		checktime[e] = g_Time + 500
		if npc_monitor[e].monitor_active == 1 then SetEntityActivated(e,1) end
		if npc_monitor[e].monitor_active == 2 then SetEntityActivated(e,0) end
		
		if npc_monitor[e].health_bar ~= "" then
			hbarsprite[e] = CreateSprite(LoadImage(npc_monitor[e].health_bar))
			SetSpriteSize(hbarsprite[e],-1,-1)
			SetSpritePosition(hbarsprite[e],200,200)
		end

		if pEntn[e] == 0 then
			for n = 1, g_EntityElementMax do
				if n ~= nil and g_Entity[n] ~= nil then
					if lower(GetEntityName(n)) == npc_monitor[e].npc_name then 
						pEntn[e] = n
						Ent = g_Entity[n]
						local dims = P.GetObjectDimensions(Ent.obj)
						rotheight[e] = (dims.h + npc_monitor[e].y_adjustment)						
						status[e] = "monitor"
						break
					end
				end
			end
		end
	end	

	if g_Entity[e]['activated'] == 1 then
	
		if status[e] == "monitor" then
			--3dto2d check--
			ScreenPosX = -1
			ScreenPosX,ScreenPosY = Convert3DTo2D(g_Entity[pEntn[e]]['x'],g_Entity[pEntn[e]]['y']+rotheight[e],g_Entity[pEntn[e]]['z'])
			if ScreenPosX < 0 then
				ScreenPosX = 0
				ScreenPosY = 0
			else
				percentx,percenty = ScreenCoordsToPercent(ScreenPosX,ScreenPosY)
			end
			--Health and Healthbar check--
			if g_LegacyNPC == 0 then hreadout[e] = g_Entity[pEntn[e]]['health'] end
			if g_LegacyNPC == 1 then hreadout[e] = g_Entity[pEntn[e]]['health']-1000 end			
			if hreadout[e] < 9000 then
				hbarsize[e] = hreadout[e]/200
				SetSpriteSize(hbarsprite[e],hbarsize[e],3)
				if g_Entity[pEntn[e]]['health'] > 100 then SetSpriteColor(hbarsprite[e],0,255,0,255) end
				if g_Entity[pEntn[e]]['health'] < 100 then SetSpriteColor(hbarsprite[e],255,0,0,255) end				
			end				
			if npc_monitor[e].health_display == 1 then
				PasteSpritePosition(hbarsprite[e],percentx-(hbarsize[e]/2),percenty)
			end			
			if npc_monitor[e].health_display == 2 then
				TextCenterOnXColor(percentx,percenty,1,"Health: " ..hreadout[e],255,255,255)
			end
			if npc_monitor[e].health_display == 3 then
				PasteSpritePosition(hbarsprite[e],percentx-(hbarsize[e]/2),percenty)
				TextCenterOnXColor(percentx,percenty,1,"Health: " ..hreadout[e],255,255,255)
			end
			if npc_monitor[e].health_display == 4 then
				--No Display--
			end	
			
			if g_Time > checktime[e] then			
				if g_Entity[pEntn[e]].health <= 0 and npc_monitor[e].death_action == 1 then
					wait[e] = g_Time + (npc_monitor[e].action_delay*1000)				
					status[e] = "alarm"
				end
				if g_Entity[pEntn[e]].health <= 0 and npc_monitor[e].death_action == 2 then
					wait[e] = g_Time + (npc_monitor[e].action_delay*1000)
					status[e] = "winorlose"
				end
				if g_Entity[pEntn[e]].health <= 0 and npc_monitor[e].death_action == 3 then			
					wait[e] = g_Time + (npc_monitor[e].action_delay*1000)
					status[e] = "winorlose"
				end
				if g_Entity[pEntn[e]].health <= 0 and npc_monitor[e].death_action == 4 then			
					wait[e] = g_Time + (npc_monitor[e].action_delay*1000)
					status[e] = "winorlose"
				end
				if g_Entity[pEntn[e]].health <= 0 and npc_monitor[e].death_action == 5 then			
					if _G["g_UserGlobal['"..npc_monitor[e].user_global.."']"] ~= nil then
						currentvalue[e] = _G["g_UserGlobal['"..npc_monitor[e].user_global.."']"]
						_G["g_UserGlobal['"..npc_monitor[e].user_global.."']"] = currentvalue[e] + npc_monitor[e].user_global_value
					end
					status[e] = "end"
					SwitchScript(e,"no_behavior_selected.lua")
				end
				if g_LegacyNPC == 1 and g_Entity[pEntn[e]].health < 1000 then
					g_LegacyNPC = 0
				end
				checktime[e] = g_Time + 50
			end	
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
				if npc_monitor[e].death_action == 2 then LoseGame() end
				if npc_monitor[e].death_action == 3 then WinGame() end
				if npc_monitor[e].death_action == 4 then
					JumpToLevelIfUsedEx(e,npc_monitor[e].resetstates)
				end
				status[e] = "end"
			end
		end
	end
end