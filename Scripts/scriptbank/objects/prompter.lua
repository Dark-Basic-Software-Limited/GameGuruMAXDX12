-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Prompter v1: by Necrym59
-- DESCRIPTION: Will display a prompt message to the screen or user global when activated by linked switch or zone.
-- DESCRIPTION: [PROMPT_TEXT$="This the message text"]
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Screen,2=User Global)]
-- DESCRIPTION: [PROMPT_DURATION=1(1,60)] in second(s)
-- DESCRIPTION: [@@USER_GLOBAL$=""(0=globallist)] eg: MyTextGlobal
-- DESCRIPTION: <Sound0> will play when activated

local prompter 			= {}
local prompt_text 		= {}
local prompt_display 	= {}
local prompt_duration	= {}
local user_global 		= {}

local doonce			= {}
local displaytimer		= {}
local status			= {}
	
function prompter_properties(e, prompt_text, prompt_display, prompt_duration, user_global)
	prompter[e].prompt_text = prompt_text
	prompter[e].prompt_display = prompt_display
	prompter[e].prompt_duration = prompt_duration
	prompter[e].user_global = user_global 
end 	
	
function prompter_init(e)
	prompter[e] = {}
	prompter[e].prompt_text = "E to Use"
	prompter[e].prompt_display = 1
	prompter[e].prompt_duration = 1
	prompter[e].user_global = ""
	
	doonce[e] = 0
	displaytimer[e] = math.huge
	status[e] = "init"	
end
 
function prompter_main(e)

	if status[e] == "init" then		
		status[e] = "endinit"
	end

	if g_Entity[e]['activated'] == 1 then
		if doonce[e] == 0 then
			if prompter[e].prompt_display == 1 then PromptDuration(prompter[e].prompt_text,(prompter[e].prompt_duration*1000)) end
			if prompter[e].prompt_display == 2 then 
				if prompter[e].user_global > "" then _G["g_UserGlobal['"..prompter[e].user_global.."']"] = prompter[e].prompt_text end				
			end				
			PlaySound(e,0)
			displaytimer[e] = g_Time + (prompter[e].prompt_duration*1000)
			doonce[e] = 1
		end
		if doonce[e] == 1 then
			if g_Time > displaytimer[e] then
				_G["g_UserGlobal['"..prompter[e].user_global.."']"] = ""
				doonce[e] = 0
				displaytimer[e] = math.huge
				SetActivated(e,0)
			end
		end	
	end	
end
