-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Countdown v21 by Necrym59
-- DESCRIPTION: A Countdown timer to count down to an end action. 
-- DESCRIPTION: Attach to an object. Set Always Active ON. Trigger from a zone or switch.
-- DESCRIPTION: [#MAXIMUM_TIME=1.00(0.01,600.00)] minutes
-- DESCRIPTION: [@TIME_DISPLAY=1(1=None, 2=Text Display, 3=Time Display Global)]
-- DESCRIPTION: [DISPLAY_TEXT$="Time Left:"]
-- DESCRIPTION: [DISPLAY_X=10]
-- DESCRIPTION: [DISPLAY_Y=10]
-- DESCRIPTION: [DISPLAY_SIZE=3(1,5)]
-- DESCRIPTION: [@END_ACTION=1(1=Terminate Player, 2=Hurt Player, 3=Activate Entities, 4=Lose Game, 5=Win Game, 6=Display Hud Screen, 7=Go to Level)]
-- DESCRIPTION: [@@BONUS_PENALTY_GLOBAL$=""(0=globallist)] (eg: MyBonusPenaltyGlobal) - Numeric User Global 
-- DESCRIPTION: [@@TIME_DISPLAY_GLOBAL$=""(0=globallist)] (eg: MyTimeDisplay) - Text User Global  
-- DESCRIPTION: [@@LAUNCH_DISPLAY_GLOBAL$=""(0=globallist)] (eg: MyLaunchDisplay)- Text User Global 
-- DESCRIPTION: [@LAUNCH_WARNING=1(1=None, 2=Text Display, 3=Launch Display Global)]
-- DESCRIPTION: [@@END_HUD_SCREEN$=""(0=hudscreenlist)] Eg: HUD Screen 9
-- DESCRIPTION: [START_DISARMED!=1]
-- DESCRIPTION: [START_FREEZE!=1]
-- DESCRIPTION: [@GoToLevelMode=1(1=Use Storyboard Logic,2=Go to Specific Level)] controls whether the next level in the Storyboard, or another level is loaded after the switch is turned on.
-- DESCRIPTION: <Sound0> for launch "Ready" sound
-- DESCRIPTION: <Sound1> for launch "Set" sound
-- DESCRIPTION: <Sound2> for launch "Go" sound
-- DESCRIPTION: <Sound3> for end warning sound

g_countdown = {}

local countdown 			= {}
local maximum_time 			= {}
local time_display 			= {}
local display_text 			= {}
local display_x 			= {}
local display_y 			= {}
local display_size 			= {}
local end_action 			= {}
local bonus_penalty_global	= {}
local time_display_global	= {}
local launch_display_global	= {}
local launch_warning		= {}
local end_hud_screen		= {}
local start_disarmed		= {}
local start_freeze			= {}

local launch_size			= {}
local launch_warning_x		= {}
local launch_warning_y		= {}
local lastgun     			= {}

local secondsleft 	= {}
local minutesleft 	= {}
local timeleftsec	= {}
local startcount 	= {}
local maxcount 		= {}
local played		= {}
local status		= {}
local wait			= {}
local launch_stage	= {}
local launch_count	= {}
local currentvalue	= {}
local doonce		= {}
local rearm			= {}
local freezex		= {}
local freezey		= {}
local freezez		= {}
local rearmtimer	= {}

function countdown_properties(e,maximum_time, time_display, display_text, display_x, display_y, display_size, end_action, bonus_penalty_global, time_display_global, launch_display_global, launch_warning, end_hud_screen, start_disarmed, start_freeze)
	countdown[e] = g_Entity[e]
	countdown[e].maximum_time = maximum_time
	countdown[e].time_display = time_display
	countdown[e].display_text = display_text	
	countdown[e].display_x = display_x
	countdown[e].display_y = display_y
	countdown[e].display_size = display_size
	countdown[e].end_action = end_action
	countdown[e].bonus_penalty_global = bonus_penalty_global
	countdown[e].time_display_global = time_display_global
	countdown[e].launch_display_global = launch_display_global
	
	countdown[e].launch_warning = launch_warning
	countdown[e].end_hud_screen = end_hud_screen
	countdown[e].start_disarmed = start_disarmed or 1
	countdown[e].start_freeze = start_freeze
end

function countdown_init(e)
	countdown[e] = {}
	countdown[e].maximum_time = 10
	countdown[e].time_display = 1
	countdown[e].display_text = "Time Left:"
	countdown[e].display_x = 10
	countdown[e].display_y = 10
	countdown[e].display_size = 3
	countdown[e].end_action = 1
	countdown[e].bonus_penalty_global = ""
	countdown[e].time_display_global = ""
	countdown[e].launch_display_global = ""
	countdown[e].launch_warning = 1
	countdown[e].end_hud_screen = ""
	countdown[e].start_disarmed = 1
	countdown[e].start_freeze = 1
	
	startcount[e] = 0
	maxcount[e] = 0
	secondsleft[e] = 0
	minutesleft[e] = 0
	timeleftsec[e] = 0
	played[e] = 0
	doonce[e] = 0
	rearm[e] = 0
	launch_stage[e] = 0
	launch_count[e] = 600	
	currentvalue[e] = 0
	lastgun[e] = 0
	wait[e] = math.huge	
	rearmtimer[e] = math.huge
	g_countdown	= 0	
	status[e] = "init"
end
 
function countdown_main(e)
	if status[e] == "init" then
		if countdown[e].start_disarmed == 1 then
			lastgun[e] = g_PlayerGunName
			SetPlayerWeapons(0)
		end	
		SetGamePlayerStatePlrKeyForceKeystate(0)
		launch_stage[e] = 0
		if countdown[e].launch_warning == 1 then launch_stage[e] = 1 end
		startcount[e] = 0
		maxcount[e] = (countdown[e].maximum_time * 1000) * 60
		freezex[e] = g_PlayerPosX
		freezey[e] = g_PlayerPosY
		freezez[e] = g_PlayerPosZ		
		status[e] = "endinit"
	end

	if g_Entity[e]['activated'] == 1 then
	
		if countdown[e].launch_warning == 2 and launch_stage[e] == 0 then
			if countdown[e].start_freeze == 1 and launch_stage[e] == 0 then				
				SetFreezePosition(freezex[e],freezey[e],freezez[e])	
				TransportToFreezePositionOnly()
			end	
			if launch_stage[e] == 0 then 
				if launch_count[e] > 400 then
					TextCenterOnX(50,50,5,"READY")
					if played[e] == 0 then
						PlaySound(e,0)
						played[e] = 1
					end					
				end
				if launch_count[e] > 200 and launch_count[e] <= 400 then
					TextCenterOnX(50,50,5,"SET")
					if played[e] == 1 then
						PlaySound(e,1)
						played[e] = 2
					end						
				end
				if launch_count[e] <= 200 then
					TextCenterOnX(50,50,5,"GO")
					if played[e] == 2 then
						PlaySound(e,2)
						played[e] = 3
					end						
				end
				if launch_count[e] <= 100 then
					launch_stage[e] = 1					
					PerformLogicConnections(e)
				end			
				launch_count[e] = launch_count[e] - 1
				g_countdown = launch_count[e]
				if g_countdown <= 100 then g_countdown = 0 end
			end
		end
		if countdown[e].launch_warning == 3 and launch_stage[e] == 0 then
			if countdown[e].start_freeze == 1 and launch_stage[e] == 0 then				
				SetFreezePosition(freezex[e],freezey[e],freezez[e])	
				TransportToFreezePositionOnly()
			end	
			if launch_stage[e] == 0 then 
				if _G["g_UserGlobal['"..countdown[e].launch_display_global.."']"] ~= nil then
					if launch_count[e] > 400 then
						_G["g_UserGlobal['"..countdown[e].launch_display_global.."']"] = "READY"
						if played[e] == 0 then
							PlaySound(e,0)
							played[e] = 1
						end
					end	
					if launch_count[e] > 200 and launch_count[e] <= 400 then
						_G["g_UserGlobal['"..countdown[e].launch_display_global.."']"] = "SET"
						if played[e] == 1 then
							PlaySound(e,1)
							played[e] = 2
						end
					end
					if launch_count[e] <= 200 then
						_G["g_UserGlobal['"..countdown[e].launch_display_global.."']"] = "GO"
						if played[e] == 2 then
							PlaySound(e,2)
							played[e] = 3
						end	
					end
				end	
				if launch_count[e] <= 100 then
					launch_stage[e] = 1					
					PerformLogicConnections(e)
				end
				launch_count[e] = launch_count[e] - 1
				g_countdown = launch_count[e]
				if g_countdown <= 100 then g_countdown = 0 end
			end
		end		
		if startcount[e] == 0 and launch_stage[e] == 1 then
			_G["g_UserGlobal['"..countdown[e].launch_display_global.."']"] = ""		
			if countdown[e].start_disarmed == 1 then
				if rearm[e] == 0 then
					SetPlayerWeapons(1)
					ChangePlayerWeapon(lastgun[e])
					SetGamePlayerStatePlrKeyForceKeystate(2)					
					rearm[e] = 1
					rearmtimer[e] = g_Time + 500
				end
			end
			played[e] = 0
			StartTimer(e)
			startcount[e] = 1			
		end
		if startcount[e] == 1 then
			if g_Time > rearmtimer[e] then SetGamePlayerStatePlrKeyForceKeystate(0) end
			if countdown[e].bonus_penalty_global > "" then
				if _G["g_UserGlobal['"..countdown[e].bonus_penalty_global.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..countdown[e].bonus_penalty_global.."']"] end
				if currentvalue[e] ~= nil then
					maxcount[e] = (maxcount[e] + currentvalue[e] * 1000)
					StartTimer(e)
					startcount[e] = 1
					_G["g_UserGlobal['"..countdown[e].bonus_penalty_global.."']"] = 0
				end
			end
			secondsleft[e] = math.floor((maxcount[e]/1000)-GetTimer(e)/1000)
			minutesleft[e] = math.floor(((maxcount[e]/1000)/60)-((GetTimer(e)/1000)/60))
			timeleftsec[e] = secondsleft[e] - (minutesleft[e]*60)			
			if countdown[e].time_display == 1 then end
			if countdown[e].time_display == 2 then
				if countdown[e].display_x < 50 or countdown[e].display_x > 50 then
					if minutesleft[e] > 0 and secondsleft[e] > 9 then Text(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size, countdown[e].display_text.. " " ..minutesleft[e].. " : " ..timeleftsec[e]) end
					if minutesleft[e] == 0 and secondsleft[e] > 9 then Text(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size,countdown[e].display_text.. " 0 : " ..timeleftsec[e]) end
					if minutesleft[e] > 0 and secondsleft[e] < 10 then Text(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size, countdown[e].display_text.. " " ..minutesleft[e].. " : 0" ..timeleftsec[e]) end
					if minutesleft[e] == 0 and secondsleft[e] < 10 then Text(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size,countdown[e].display_text.. " 0 : 0" ..timeleftsec[e]) end					
				end
				if countdown[e].display_x == 50 then
					if minutesleft[e] > 0 and secondsleft[e] > 9  then TextCenterOnX(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size, countdown[e].display_text.. " " ..minutesleft[e].. " : " ..timeleftsec[e]) end
					if minutesleft[e] == 0 and secondsleft[e] > 9 then TextCenterOnX(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size,countdown[e].display_text.. " 0 : " ..timeleftsec[e]) end
					if minutesleft[e] > 0 and secondsleft[e] < 10 then TextCenterOnX(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size, countdown[e].display_text.. " " ..minutesleft[e].. " : 0" ..timeleftsec[e]) end
					if minutesleft[e] == 0 and secondsleft[e] < 10 then TextCenterOnX(countdown[e].display_x, countdown[e].display_y, countdown[e].display_size,countdown[e].display_text.. " 0 : 0" ..timeleftsec[e]) end
				end
			end		
			if countdown[e].time_display == 3 then
				if _G["g_UserGlobal['"..countdown[e].time_display_global.."']"] ~= nil then
					if minutesleft[e] > 0 and secondsleft[e] > 9 then _G["g_UserGlobal['"..countdown[e].time_display_global.."']"] = (countdown[e].display_text.. " " ..minutesleft[e].. " : " ..timeleftsec[e]) end
					if minutesleft[e] == 0 and secondsleft[e] > 9 then _G["g_UserGlobal['"..countdown[e].time_display_global.."']"] = (countdown[e].display_text.. " 0 : " ..timeleftsec[e]) end
					if minutesleft[e] > 0 and secondsleft[e] < 10 then _G["g_UserGlobal['"..countdown[e].time_display_global.."']"] = (countdown[e].display_text.. " " ..minutesleft[e].. " : 0" ..timeleftsec[e]) end
					if minutesleft[e] == 0 and secondsleft[e] < 10 then _G["g_UserGlobal['"..countdown[e].time_display_global.."']"] = (countdown[e].display_text.. " 0 : 0" ..timeleftsec[e]) end					
				end			
			end
			if secondsleft[e] <= 10 then
				if played[e] == 0 then
					PlaySound(e,3)
					played[e] = 1
				end
			end		
			if secondsleft[e] == 0 then
				if countdown[e].end_action == 1 then
					HurtPlayer(e,g_PlayerHealth)
				end
				if countdown[e].end_action == 2 then
					HurtPlayer(e,g_PlayerHealth/3)
				end
				if countdown[e].end_action == 3 then
					if doonce[e] == 0 then
						ActivateIfUsed(e)						
						doonce[e] = 1
					end
				end
				if countdown[e].end_action == 4 then
					LoseGame()
				end
				if countdown[e].end_action == 5 then
					WinGame()					
				end
				if countdown[e].end_action == 6 then
					ScreenToggle(countdown[e].end_hud_screen)
					status[e] = "interface"
				end
				if countdown[e].end_action == 7 then
					JumpToLevelIfUsedEx(e,0)
				end
			end
		end		
	end

	if g_Entity[e]['activated'] == 0 then		
		status[e] = "init"
	end
	
	if status[e] == "interface" then
		local buttonElementID = DisplayCurrentScreen()
		local buttonElementName = GetScreenElementName(1+buttonElementID)
		if string.len(buttonElementName) > 0 then				
			if buttonElementName == "LEAVE" then				
				status[e] = "interface2"
			end
		end	
	end
	if status[e] == "interface2" then
		-- CLOSE HUD
		ScreenToggle("")
		JumpToLevelIfUsedEx(e,0)
	end
end

function level_countdown_exit(e)
end