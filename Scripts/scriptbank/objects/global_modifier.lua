-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Global Modifier v10 by Necrym59
-- DESCRIPTION: The attached object when activated will add/deduct a value to up to three user globals for use with other behaviors, such as countdowns or monitors.
-- DESCRIPTION: [PROMPT_TEXT$="E to collect"]
-- DESCRIPTION: [COLLECTED_TEXT$="Modifier collected"]
-- DESCRIPTION: [PICKUP_RANGE=90(1,100)]
-- DESCRIPTION: [@ACTIVATION_STYLE=1(1=Automatic Pickup, 2=Manual Pickup, 3=External Triggered)]
-- DESCRIPTION: [@@USER_GLOBAL_AFFECTED1$=""(0=globallist)] eg: MyGlobal
-- DESCRIPTION: [AFFECTED_AMOUNT1=10(1,30)]
-- DESCRIPTION: [@@USER_GLOBAL_AFFECTED2$=""(0=globallist)] eg: MyGlobal
-- DESCRIPTION: [AFFECTED_AMOUNT2=10(1,30)]
-- DESCRIPTION: [@@USER_GLOBAL_AFFECTED3$=""(0=globallist)] eg: MyGlobal
-- DESCRIPTION: [AFFECTED_AMOUNT3=10(1,30)]
-- DESCRIPTION: [@EFFECT=1(1=Add, 2=Deduct)]
-- DESCRIPTION: [HEALTH_MODIFIER=0(0,100)]
-- DESCRIPTION: [@WHEN_ACTIVATED=1(1=Do Nothing, 2=Hide, 3=Show, 4=Destroy)]
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [@ITEM_HIGHLIGHT=0(0=None,1=Shape,2=Outline,3=Icon)]
-- DESCRIPTION: [HIGHLIGHT_ICON_IMAGEFILE$="imagebank\\icons\\pickup.png"]
-- DESCRIPTION: Play the audio <Sound0> when picked up

local module_misclib = require "scriptbank\\module_misclib"
local U = require "scriptbank\\utillib"
g_tEnt = {}

local modifier 				= {}
local prompt_text 			= {}
local collected_text 		= {}
local pickup_range 			= {}
local activation_style 		= {}
local user_global_affected1	= {}
local affected_amount1		= {}
local user_global_affected2	= {}
local affected_amount2		= {}
local user_global_affected3	= {}
local affected_amount3		= {}
local effect				= {}
local health_modifier		= {}
local when_activated		= {}
local prompt_display 		= {}
local item_highlight 		= {}
local highlight_icon 		= {}

local currentvalue1 	= {}
local currentvalue2 	= {}
local currentvalue3 	= {}
local pressed 			= {}
local played 			= {}
local status			= {}
local hl_icon			= {}
local hl_imgwidth		= {}
local hl_imgheight		= {}
local tEnt				= {}
local selectobj			= {}

function global_modifier_properties(e, prompt_text, collected_text, pickup_range, activation_style, user_global_affected1, affected_amount1, user_global_affected2, affected_amount2, user_global_affected3, affected_amount3, effect, health_modifier, when_activated, prompt_display, item_highlight, highlight_icon_imagefile)
	modifier[e].prompt_text = prompt_text
	modifier[e].collected_text = collected_text
	modifier[e].pickup_range = pickup_range
	modifier[e].activation_style = activation_style
	modifier[e].user_global_affected1 = user_global_affected1
	modifier[e].affected_amount1 = affected_amount1
	modifier[e].user_global_affected2 = user_global_affected2
	modifier[e].affected_amount2 = affected_amount2	
	modifier[e].user_global_affected3 = user_global_affected3
	modifier[e].affected_amount3 = affected_amount3	
	modifier[e].effect = effect
	modifier[e].health_modifier = health_modifier
	modifier[e].when_activated = when_activated
	modifier[e].prompt_display = prompt_display
	modifier[e].prompt_display = prompt_display
	modifier[e].item_highlight = item_highlight
	modifier[e].highlight_icon = highlight_icon_imagefile	
end

function global_modifier_init(e)
	modifier[e] = {}
	modifier[e].prompt_text = "E to collect"
	modifier[e].collected_text = "Collected modifier"
	modifier[e].modifier_level = 0
	modifier[e].pickup_range = 50
	modifier[e].activation_style = 1
	modifier[e].user_global_affected1 = ""
	modifier[e].affected_amount1 = 0	
	modifier[e].user_global_affected2 = ""
	modifier[e].affected_amount2 = 0		
	modifier[e].user_global_affected3 = ""
	modifier[e].affected_amount3 = 0		
	modifier[e].effect = 1
	modifier[e].health_modifier = 0
	modifier[e].when_activated = 1
	modifier[e].prompt_display = 1
	modifier[e].item_highlight = 0
	modifier[e].highlight_icon = "imagebank\\icons\\pickup.png"

	played[e] = 0
	pressed[e] = 0
	currentvalue1[e] = 0
	currentvalue2[e] = 0
	currentvalue3[e] = 0	
	tEnt[e] = 0
	g_tEnt = 0
	selectobj[e] = 0
	status[e] = "init"
	hl_icon[e] = 0
	hl_imgwidth[e] = 0
	hl_imgheight[e] = 0	
end

function global_modifier_main(e)

	if status[e] == "init" then
		if modifier[e].item_highlight == 3 and modifier[e].highlight_icon ~= "" then
			hl_icon[e] = CreateSprite(LoadImage(modifier[e].highlight_icon))
			hl_imgwidth[e] = GetImageWidth(LoadImage(modifier[e].highlight_icon))
			hl_imgheight[e] = GetImageHeight(LoadImage(modifier[e].highlight_icon))
			SetSpriteSize(hl_icon[e],-1,-1)
			SetSpriteDepth(hl_icon[e],100)
			SetSpriteOffset(hl_icon[e],hl_imgwidth[e]/2.0, hl_imgheight[e]/2.0)
			SetSpritePosition(hl_icon[e],500,500)
		end	
		status[e] = "endinit"
	end

	local PlayerDist = GetPlayerDistance(e)

	if modifier[e].activation_style == 1 and PlayerDist < modifier[e].pickup_range then
		if modifier[e].prompt_display == 1 then PromptLocal(e,modifier[e].collected_text) end
		if modifier[e].prompt_display == 2 then Prompt(modifier[e].collected_text) end	
		if played[e] == 0 then
			PlaySound(e,0)
			played[e] = 1
		end
		PerformLogicConnections(e)
		if modifier[e].user_global_affected1 > "" and modifier[e].effect == 1 then
			if _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] ~= nil then currentvalue1[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] end			
			_G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] = currentvalue1[e] + modifier[e].affected_amount1
			SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier)
		end
		if modifier[e].user_global_affected2 > "" and modifier[e].effect == 1 then
			if _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] ~= nil then currentvalue2[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] end
			_G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] = currentvalue2[e] + modifier[e].affected_amount2
			SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier)
		end
		if modifier[e].user_global_affected3 > "" and modifier[e].effect == 1 then		
			if _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] ~= nil then currentvalue3[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] end
			_G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] = currentvalue3[e] + modifier[e].affected_amount3						
		end		
		if modifier[e].user_global_affected1 > "" and modifier[e].effect == 2 then
			if _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] ~= nil then currentvalue1[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] end
			_G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] = currentvalue1[e] - modifier[e].affected_amount1
		end
		if modifier[e].user_global_affected2 > "" and modifier[e].effect == 2 then
			if _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] ~= nil then currentvalue2[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] end
			_G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] = currentvalue2[e] - modifier[e].affected_amount2
		end	
		if modifier[e].user_global_affected3 > "" and modifier[e].effect == 2 then
			if _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] ~= nil then currentvalue3[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] end
			_G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] = currentvalue3[e] - modifier[e].affected_amount3					
		end
		if modifier[e].health_modifier ~= 0 and modifier[e].effect == 1 then SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier) end
		if modifier[e].health_modifier ~= 0 and modifier[e].effect == 2 then SetPlayerHealth(g_PlayerHealth - modifier[e].health_modifier) end
		Hide(e)
		CollisionOff(e)
		Destroy(e)
		pressed[e] = 1
		tEnt[e] = 0
	end

	if modifier[e].activation_style == 2 and PlayerDist < modifier[e].pickup_range then
		--pinpoint select object--
		module_misclib.pinpoint(e,modifier[e].pickup_range,modifier[e].item_highlight,hl_icon[e])
		tEnt[e] = g_tEnt
		--end pinpoint select object--

		if PlayerDist < modifier[e].pickup_range and tEnt[e] == e then
			if modifier[e].prompt_display == 1 then PromptLocal(e,modifier[e].prompt_text) end
			if modifier[e].prompt_display == 2 then Prompt(modifier[e].prompt_text) end	
			if g_KeyPressE == 1 and pressed[e] == 0 then
				if modifier[e].prompt_display == 1 then PromptLocal(e,modifier[e].collected_text) end
				if modifier[e].prompt_display == 2 then Prompt(modifier[e].collected_text) end			
				PlaySound(e,0)
				PerformLogicConnections(e)
				if modifier[e].user_global_affected1 > "" and modifier[e].effect == 1 then
					if _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] ~= nil then currentvalue1[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] end			
					_G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] = currentvalue1[e] + modifier[e].affected_amount1
					SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier)
				end
				if modifier[e].user_global_affected2 > "" and modifier[e].effect == 1 then
					if _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] ~= nil then currentvalue2[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] end
					_G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] = currentvalue2[e] + modifier[e].affected_amount2
					SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier)
				end
				if modifier[e].user_global_affected3 > "" and modifier[e].effect == 1 then		
					if _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] ~= nil then currentvalue3[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] end
					_G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] = currentvalue3[e] + modifier[e].affected_amount3						
				end		
				if modifier[e].user_global_affected1 > "" and modifier[e].effect == 2 then
					if _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] ~= nil then currentvalue1[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] end
					_G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] = currentvalue1[e] - modifier[e].affected_amount1
				end
				if modifier[e].user_global_affected2 > "" and modifier[e].effect == 2 then
					if _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] ~= nil then currentvalue2[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] end
					_G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] = currentvalue2[e] - modifier[e].affected_amount2
				end	
				if modifier[e].user_global_affected3 > "" and modifier[e].effect == 2 then
					if _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] ~= nil then currentvalue3[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] end
					_G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] = currentvalue3[e] - modifier[e].affected_amount3					
				end
				if modifier[e].health_modifier ~= 0 and modifier[e].effect == 1 then SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier) end
				if modifier[e].health_modifier ~= 0 and modifier[e].effect == 2 then SetPlayerHealth(g_PlayerHealth - modifier[e].health_modifier) end
				Hide(e)
				CollisionOff(e)
				Destroy(e)
				pressed[e] = 1
			end
		end
	end
	
	if modifier[e].activation_style == 3 then
		if g_Entity[e].activated == 1 then
			if modifier[e].prompt_display == 1 then PromptLocal(e,modifier[e].collected_text) end
			if modifier[e].prompt_display == 2 then Prompt(modifier[e].collected_text) end
			if played[e] == 0 then
				PlaySound(e,0)
				played[e] = 1
			end
			PerformLogicConnections(e)
			if modifier[e].user_global_affected1 > "" and modifier[e].effect == 1 then
				if _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] ~= nil then currentvalue1[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] end			
				_G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] = currentvalue1[e] + modifier[e].affected_amount1
				SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier)
			end
			if modifier[e].user_global_affected2 > "" and modifier[e].effect == 1 then
				if _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] ~= nil then currentvalue2[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] end
				_G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] = currentvalue2[e] + modifier[e].affected_amount2
				SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier)
			end
			if modifier[e].user_global_affected3 > "" and modifier[e].effect == 1 then		
				if _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] ~= nil then currentvalue3[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] end
				_G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] = currentvalue3[e] + modifier[e].affected_amount3						
			end		
			if modifier[e].user_global_affected1 > "" and modifier[e].effect == 2 then
				if _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] ~= nil then currentvalue1[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] end
				_G["g_UserGlobal['"..modifier[e].user_global_affected1.."']"] = currentvalue1[e] - modifier[e].affected_amount1
			end
			if modifier[e].user_global_affected2 > "" and modifier[e].effect == 2 then
				if _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] ~= nil then currentvalue2[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] end
				_G["g_UserGlobal['"..modifier[e].user_global_affected2.."']"] = currentvalue2[e] - modifier[e].affected_amount2
			end	
			if modifier[e].user_global_affected3 > "" and modifier[e].effect == 2 then
				if _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] ~= nil then currentvalue3[e] = _G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] end
				_G["g_UserGlobal['"..modifier[e].user_global_affected3.."']"] = currentvalue3[e] - modifier[e].affected_amount3					
			end
			if modifier[e].health_modifier ~= 0 and modifier[e].effect == 1 then SetPlayerHealth(g_PlayerHealth + modifier[e].health_modifier) end
			if modifier[e].health_modifier ~= 0 and modifier[e].effect == 2 then SetPlayerHealth(g_PlayerHealth - modifier[e].health_modifier) end
			if modifier[e].when_activated == 1 then end	--do nothing
			if modifier[e].when_activated == 2 then		--hide
				CollisionOff(e)
				Hide(e)
			end
			if modifier[e].when_activated == 3 then 	--show
				Show(e)
			end
			if modifier[e].when_activated == 4 then 	--destroy	
				CollisionOff(e)
				Hide(e)				
				Destroy(e)
			end
			pressed[e] = 1
			tEnt[e] = 0
		end
	end
end