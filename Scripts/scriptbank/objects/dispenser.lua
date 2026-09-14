-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Dispenser v4 by Necrym59
-- DESCRIPTION: Attach to an object to act as a dispenser for required attribute.
-- DESCRIPTION: [USE_RANGE=80(1,200)]
-- DESCRIPTION: [USE_PROMPT$="Hold E to use"]
-- DESCRIPTION: [DISPENSE_RATE=1(1,100)] Value per dispense
-- DESCRIPTION: [DISPENSE_MAX=50(1,1000)] Maximum dispenser amount
-- DESCRIPTION: [@DISPENSE_TO=1(1=Health, 2=User Global)]
-- DESCRIPTION: [@@USER_GLOBAL$=""(0=globallist)] User Global affected (eg; MyCustomValue)
-- DESCRIPTION: [@WHEN_EMPTY=1(1=Deactivate, 2=Destroy, 3=Trigger)]
-- DESCRIPTION: [@DISPENSING_ANIMATION$=-1(0=AnimSetList)]
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [@ITEM_HIGHLIGHT=0(0=None,1=Shape,2=Outline,3=Icon)]
-- DESCRIPTION: [HIGHLIGHT_ICON_IMAGEFILE$="imagebank\\icons\\hand.png"]
-- DESCRIPTION: <Sound0> dispense loop sound
-- DESCRIPTION: <Sound1> dispense empty sound

local module_misclib = require "scriptbank\\module_misclib"
g_tEnt = {}

local dispenser 			= {}
local use_range 			= {}
local use_prompt 			= {}
local dispense_rate 		= {}
local dispense_max 			= {}
local dispense_to			= {}
local user_global			= {}
local when_empty			= {}
local dispensing_animation	= {}
local prompt_display		= {}
local item_highlight		= {}
local highlight_icon		= {}

local hl_icon 		= {}
local hl_imgwidth	= {}
local hl_imgheight	= {}
local currentvalue	= {}
local status		= {}
local tEnt			= {}

local dispensed_level = {}
local dispensed_total = {}
local dispense_timer = {}
local playonce	= {}


function dispenser_properties(e, use_range, use_prompt, dispense_rate, dispense_max, dispense_to, user_global, when_empty, dispensing_animation, prompt_display, item_highlight, highlight_icon_imagefile)
    dispenser[e].use_range = use_range
    dispenser[e].use_prompt = use_prompt
    dispenser[e].dispense_rate = dispense_rate
    dispenser[e].dispense_max = dispense_max
    dispenser[e].dispense_to = dispense_to
    dispenser[e].user_global = user_global
    dispenser[e].when_empty = when_empty
    dispenser[e].dispensing_animation = "=" .. tostring(dispensing_animation)
    dispenser[e].prompt_display = prompt_display
    dispenser[e].item_highlight = item_highlight
    dispenser[e].highlight_icon = highlight_icon_imagefile
end

function dispenser_init(e)
    dispenser[e] = {}
    dispenser[e].use_range = 80
    dispenser[e].use_prompt = "Hold E to use"
    dispenser[e].dispense_rate = 1
    dispenser[e].dispense_max = 50
    dispenser[e].dispense_to = dispense_to
    dispenser[e].user_global = user_global
    dispenser[e].when_empty = 1
	dispenser[e].dispensing_animation = ""
    dispenser[e].prompt_display = prompt_display or 1
    dispenser[e].item_highlight = 0
    dispenser[e].highlight_icon = "imagebank\\icons\\hand.png"

    dispensed_total[e] = 0
	dispensed_level[e] = 0
	dispense_timer[e] = math.huge
	hl_icon[e] = 0
	hl_imgwidth[e] = 0
	hl_imgheight[e] = 0
	currentvalue[e] = 0
	g_tEnt = 0
	tEnt[e] = 0
	playonce[e] =0
	status[e] = "init"
end

function dispenser_main(e)

	if status[e] == "init" then
		if dispenser[e].item_highlight == 3 and dispenser[e].highlight_icon ~= "" then
			hl_icon[e] = CreateSprite(LoadImage(dispenser[e].highlight_icon))
			hl_imgwidth[e] = GetImageWidth(LoadImage(dispenser[e].highlight_icon))
			hl_imgheight[e] = GetImageHeight(LoadImage(dispenser[e].highlight_icon))
			SetSpriteSize(hl_icon[e],-1,-1)
			SetSpriteDepth(hl_icon[e],100)
			SetSpriteOffset(hl_icon[e],hl_imgwidth[e]/2.0, hl_imgheight[e]/2.0)
			SetSpritePosition(hl_icon[e],500,500)
		end
		if dispenser[e].user_global ~= "" then
			if _G["g_UserGlobal['"..dispenser[e].user_global.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..dispenser[e].user_global.."']"] end
		end
		dispense_timer[e] = g_Time + 100
		status[e] = "endinit"
	end

	local PlayerDist = GetPlayerDistance(e)
	if PlayerDist < dispenser[e].use_range then
		--pinpoint select object--
		module_misclib.pinpoint(e,dispenser[e].use_range,dispenser[e].item_highlight,hl_icon[e])
		tEnt[e] = g_tEnt
		--end pinpoint select object--
	end

	if PlayerDist < dispenser[e].use_range and tEnt[e] == e then
		if dispenser[e].prompt_display == 1 then TextCenterOnX(50,53,1, dispenser[e].use_prompt) end
		if dispenser[e].prompt_display == 2 then Prompt(dispenser[e].use_prompt) end
		if dispenser[e].user_global ~= "" then
			if _G["g_UserGlobal['"..dispenser[e].user_global.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..dispenser[e].user_global.."']"] end
		end
		if g_KeyPressE == 1 and dispensed_total[e] < dispenser[e].dispense_max and g_Time > dispense_timer[e] then
			if dispenser[e].dispensing_animation ~= "" then
				SetAnimationName(e,dispenser[e].dispensing_animation)
				LoopAnimation(e)
			end
			LoopSound(e,0)
			dispensed_level[e] = dispensed_level[e] + dispenser[e].dispense_rate
			dispensed_total[e] = dispensed_total[e] + dispensed_level[e]
			currentvalue[e] = currentvalue[e] + dispenser[e].dispense_rate
			if dispenser[e].dispense_to == 1 then -- Health Attribute
				SetPlayerHealth(g_PlayerHealth + dispensed_level[e])
				if g_PlayerHealth > g_PlayerStartStrength then g_PlayerHealth = g_PlayerStartStrength end
				SetPlayerHealthCore(g_PlayerHealth)
			end
			if dispenser[e].dispense_to == 2 then -- User Global Attribute
				if dispenser[e].user_global ~= "" then
					_G["g_UserGlobal['"..dispenser[e].user_global.."']"] = currentvalue[e]
				end
			end
			dispense_timer[e] = g_Time + 100
		else
			StopAnimation(e)
			dispensed_level[e] = 0
			StopSound(e,0)
		end
	end

	if dispensed_total[e] >= dispenser[e].dispense_max then
		if playonce[e] == 0 then
			StopSound(e,0)
			PlaySound(e,1)
			playonce[e] = 1
		end
		if dispenser[e].when_empty == 1 then
			SetEntityEmissiveStrength(e,0)
			SwitchScript(e,"no_behavior_selected.lua")
		end
		if dispenser[e].when_empty == 2 then
			Destroy(e)
		end
		if dispenser[e].when_empty == 3 then
			ActivateIfUsed(e)
			PerformLogicConnections(e)
			SwitchScript(e,"no_behavior_selected.lua")
		end
	end
end