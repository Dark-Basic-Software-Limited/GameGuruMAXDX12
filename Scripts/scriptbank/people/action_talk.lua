-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Action Talk v3: by Necrym59
-- DESCRIPTION: Will play animation when key is pressed and associated speech or optional randomised speech
-- DESCRIPTION: [USE_RANGE=150(1,500)]
-- DESCRIPTION: [USE_TEXT$="E to Start Conversation, Q to Exit Conversation"]
-- DESCRIPTION: [@START_ANIMATION=-1(0=AnimSetList)] Default start animation
-- DESCRIPTION: [@TALK_ANIMATION0=-1(0=AnimSetList)] Animation while talking 
-- DESCRIPTION: [@TALK_ANIMATION1=-1(0=AnimSetList)] Animation while talking 
-- DESCRIPTION: [@TALK_ANIMATION2=-1(0=AnimSetList)] Animation while talking 
-- DESCRIPTION: [@TALK_ANIMATION3=-1(0=AnimSetList)] Animation while talking 
-- DESCRIPTION: [SPEECH0$=""] Speech file 0
-- DESCRIPTION: [SPEECH1$=""] Speech file 1
-- DESCRIPTION: [SPEECH2$=""] Speech file 2
-- DESCRIPTION: [SPEECH3$=""] Speech file 3
-- DESCRIPTION: [RANDOM_SPEECH!=0] Randomise Speech

math.randomseed(os.time())
math.random(); math.random(); math.random()

local lower = string.lower
local action_talk 		= {}
local use_range 		= {}
local use_text 			= {}
local start_animation	= {}
local talk_animation0	= {}
local talk_animation1	= {}
local talk_animation2	= {}
local talk_animation3	= {}

local talk_channel		= {}
local doonce			= {}
local staanim			= {}
local finanim			= {}
local keypause 			= {}
local status			= {}

function action_talk_properties(e, use_range, use_text, start_animation, talk_animation0, talk_animation1, talk_animation2, talk_animation3, speech0, speech1, speech2, speech3, random_speech)
	action_talk[e].use_range = use_range
	action_talk[e].use_range = use_range
	action_talk[e].start_animation = "=" .. tostring(start_animation)
	action_talk[e].talk_animation0 = "=" .. tostring(talk_animation0)
	action_talk[e].talk_animation1 = "=" .. tostring(talk_animation1)
	action_talk[e].talk_animation2 = "=" .. tostring(talk_animation2)
	action_talk[e].talk_animation3 = "=" .. tostring(talk_animation3)	
	action_talk[e].speech0 = speech0
	action_talk[e].speech1 = speech1	
	action_talk[e].speech2 = speech2
	action_talk[e].speech3 = speech3
	action_talk[e].random_speech = random_speech or 0	
end

function action_talk_init(e)
	action_talk[e] = {}
	action_talk[e].use_range = 90
	action_talk[e].use_text = "E to Start Conversation, Q to Exit Conversation"
	action_talk[e].start_animation = ""
	action_talk[e].talk_animation0 = ""
	action_talk[e].talk_animation1 = ""
	action_talk[e].talk_animation2 = ""
	action_talk[e].talk_animation3 = ""
	action_talk[e].speech0 = ""
	action_talk[e].speech1 = ""	
	action_talk[e].speech2 = ""
	action_talk[e].speech3 = ""
	action_talk[e].random_speech = 0	

	doonce[e] = 0
	staanim[e] = nil
	finanim[e] = nil
	keypause[e] = math.huge
	status[e] = "init"
end

function action_talk_main(e)

	if status[e] == "init" then
		SetAnimationName(e,action_talk[e].start_animation)
		LoopAnimation(e)
		talk_channel[e] = 0
		status[e] = "endinit"
	end

	if GetPlayerDistance(e) < action_talk[e].use_range then
		LookAtPlayer(e,10)
		Prompt(action_talk[e].use_text)
		if g_KeyPressE == 1 and doonce[e] == 0 then
			keypause[e] = g_Time + 1000
			if talk_channel[e] >= 0 and talk_channel[e] < 4 and doonce[e] == 0 then
				if talk_channel[e] == 0 then
					SetAnimationName(e,action_talk[e].talk_animation0)
					staanim[e],finanim[e] = GetEntityAnimationStartFinish(e,action_talk[e].talk_animation0)
					PlayAnimation(e)
				end
				if talk_channel[e] == 1 then
					SetAnimationName(e,action_talk[e].talk_animation1)
					staanim[e],finanim[e] = GetEntityAnimationStartFinish(e,action_talk[e].talk_animation1)
					PlayAnimation(e)
				end
				if talk_channel[e] == 2 then
					SetAnimationName(e,action_talk[e].talk_animation2)
					staanim[e],finanim[e] = GetEntityAnimationStartFinish(e,action_talk[e].talk_animation2)
					PlayAnimation(e)
				end
				if talk_channel[e] == 3 then
					SetAnimationName(e,action_talk[e].talk_animation3)
					staanim[e],finanim[e] = GetEntityAnimationStartFinish(e,action_talk[e].talk_animation3)
					PlayAnimation(e)
				end
				if action_talk[e].random_speech == 0 and GetSpeech(e) == 0 then
					PlaySpeech(e,talk_channel[e])
				end
				if action_talk[e].random_speech == 1 and GetSpeech(e) == 0 then
					talk_channel[e] = math.random(0,3)
					PlaySpeech(e,talk_channel[e])
				end								
				doonce[e] = 1
			end	
		end
		if g_KeyPressQ == 1 and doonce[e] == 1 then				
			SetAnimationName(e,action_talk[e].start_animation)
			LoopAnimation(e)
			StopSpeech(e,talk_channel[e])
			talk_channel[e] = 0
			finanim[e] = nil
			doonce[e] = 0
		end
		if g_Entity[e]['frame'] == finanim[e] and doonce[e] == 1 then
			SetAnimationName(e,action_talk[e].start_animation)
			LoopAnimation(e)		
			StopSpeech(e,talk_channel[e])
			talk_channel[e] = talk_channel[e] + 1
			if talk_channel[e] == 4 then talk_channel[e] = 0 end
			finanim[e] = nil
			doonce[e] = 0
		end
		if g_KeyPressE == 1 and g_Time > keypause[e] then
			talk_channel[e] = talk_channel[e] + 1
			if talk_channel[e] == 4 then talk_channel[e] = 0 end
			SetAnimationName(e,action_talk[e].start_animation)
			LoopAnimation(e)		
			StopSpeech(e,talk_channel[e])
			finanim[e] = nil
			doonce[e] = 0
		end
	end
end