-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Communicator v3 by Necrym59
-- DESCRIPTION: Creates an audible communicator device that can be answered by the player.
-- DESCRIPTION: Attach to an object and activate from a switch or zone.
-- DESCRIPTION: [USE_RANGE=80(1,200)] range player can interact with
-- DESCRIPTION: [USE_PROMPT$="Press E to answer and to end-call"]
-- DESCRIPTION: [@USE_TRIGGER=1(1=Off, 2=Call Answer, 3=Call End, 4=Answer+End)]
-- DESCRIPTION: [TONE_VOLUME=100(0,100)] call tone sound volume
-- DESCRIPTION: [CALL_VOLUME=100(0,100)] call sound volume
-- DESCRIPTION: [END_VOLUME=100(0,100)] end sound volume
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [@ITEM_HIGHLIGHT=0(0=None,1=Shape,2=Outline,3=Icon)] Use emmisive color for shape option
-- DESCRIPTION: [HIGHLIGHT_ICON_IMAGEFILE$="imagebank\\icons\\pickup.png"]
-- DESCRIPTION: <Sound0> Tone loop
-- DESCRIPTION: <Sound1> Call sound
-- DESCRIPTION: <Sound2> End sound

local module_misclib = require "scriptbank\\module_misclib"
g_tEnt = {}

local communicator		= {}
local use_range			= {}
local use_prompt		= {}
local use_trigger		= {}
local tone_volume		= {}
local call_volume		= {}
local end_volume		= {}
local prompt_display 	= {}
local item_highlight 	= {}
local highlight_icon 	= {}

local hl_icon 			= {}
local hl_imgwidth 		= {}
local hl_imgheight 		= {}
local status			= {}
local tEnt				= {}
local keypressed		= {}
local doonce			= {}
local keypausetimer		= {}


function communicator_properties(e, use_range, use_prompt, use_trigger, tone_volume, call_volume, end_volume, prompt_display, item_highlight, highlight_icon_imagefile)
	communicator[e].use_range = use_range
	communicator[e].use_prompt = use_prompt
	communicator[e].use_trigger = use_trigger
	communicator[e].tone_volume = tone_volume
	communicator[e].call_volume = call_volume
	communicator[e].end_volume = end_volume	
	communicator[e].prompt_display = prompt_display
	communicator[e].item_highlight = item_highlight
	communicator[e].highlight_icon = highlight_icon_imagefile
end

function communicator_init(e)
	communicator[e] = {}
	communicator[e].use_range = 80
	communicator[e].use_prompt = "Press E to answer and to end-call"
	communicator[e].use_trigger = 1	
	communicator[e].tone_volume = 100
	communicator[e].call_volume = 100
	communicator[e].end_volume = 100
	communicator[e].prompt_display = 1
	communicator[e].item_highlight = 0
	communicator[e].highlight_icon = "imagebank\\icons\\pickup.png"
	
	hl_icon[e] = 0
	hl_imgwidth[e] = 0
	hl_imgheight[e] = 0
	tEnt[e] = 0
	g_tEnt = 0
	keypressed[e] = 0
	reset_timer[e] = 0
	doonce[e] = 0
	keypausetimer[e] = math.huge
	status[e] = "init"	
end

function communicator_main(e)
	
	if status[e] == "init" then
		if communicator[e].item_highlight == 3 and communicator[e].highlight_icon ~= "" then
			hl_icon[e] = CreateSprite(LoadImage(communicator[e].highlight_icon))
			hl_imgwidth[e] = GetImageWidth(LoadImage(communicator[e].highlight_icon))
			hl_imgheight[e] = GetImageHeight(LoadImage(communicator[e].highlight_icon))
			SetSpriteSize(hl_icon[e],-1,-1)
			SetSpriteDepth(hl_icon[e],100)
			SetSpriteOffset(hl_icon[e],hl_imgwidth[e]/2.0, hl_imgheight[e]/2.0)
			SetSpritePosition(hl_icon[e],500,500)
		end				
		status[e] = "endinit"
		SetActivated(e,0)
	end
	
	if g_Entity[e]['activated'] == 1 then
		local PlayerDist = GetPlayerDistance(e)
		
		if doonce[e] == 0 then
			keypausetimer[e] = math.huge
			doonce[e] = 1
		end
		if keypressed[e] == 0 then
			SetSound(e,0)
			SetSoundVolume(communicator[e].tone_volume)
			LoopSound(e,0)
		else
			StopSound(e,0)
		end				
				
		if PlayerDist < communicator[e].use_range then
			--pinpoint select object--
			module_misclib.pinpoint(e,communicator[e].use_range,communicator[e].item_highlight,hl_icon[e])
			tEnt[e] = g_tEnt
			--end pinpoint select object--
		end			
		if PlayerDist < communicator[e].use_range and tEnt[e] == e then
			if keypressed[e] == 0 then
				if communicator[e].prompt_display == 1 then PromptLocal(e,communicator[e].use_prompt) end
				if communicator[e].prompt_display == 2 then Prompt(communicator[e].use_prompt) end
			end	
			if g_KeyPressE == 1 and keypressed[e] == 0 then
				keypausetimer[e] = g_Time + 1000
				keypressed[e] = 1				
				StopSound(e,0)
				SetSound(e,1)
				SetSoundVolume(communicator[e].call_volume)
				PlaySound(e,1)
				if communicator[e].use_trigger == 2 or communicator[e].use_trigger == 4 then 
					PerformLogicConnections(e)
					ActivateIfUsed(e)
				end
			end
			if keypressed[e] == 1 and GetSoundPlaying(e,1) == 0 then g_KeyPressE = 1 end
			if g_KeyPressE == 1 and keypressed[e] == 1 and g_Time > keypausetimer[e] then
				StopSound(e,0)
				StopSound(e,1)
				SetSound(e,2)
				SetSoundVolume(communicator[e].end_volume)
				PlaySound(e,2)
				if communicator[e].use_trigger == 3 or communicator[e].use_trigger == 4 then
					PerformLogicConnections(e)
					ActivateIfUsed(e)
				end		
				keypausetimer[e] = math.huge
				keypressed[e] = 0
				doonce[e] = 0
				SetActivated(e,0)				
			end
		end
		if PlayerDist > communicator[e].use_range * 10 then
			StopSound(e,0)
			SetActivated(e,0)		
		end	
	end
end

function communicator_exit(e)
end