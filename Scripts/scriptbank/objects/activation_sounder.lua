-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Activation Sounder v6 by Necrym59
-- DESCRIPTION: Will play or loop a sound when this object is activated and optionally self destruct.
-- DESCRIPTION: Attach to an object.
-- DESCRIPTION: Set [@SOUND_STYLE=1(1=Play Sound0 , 2=Loop Sound0, 3=Sequence Play, 4=Random Play)] to play,loop (sound 0) or sequence or random play
-- DESCRIPTION: Set [SOUND_VOLUME=100[1,100] to adjust the volume
-- DESCRIPTION: Set [SOUND_DELAY=0[0,10000] to delay before playing sound
-- DESCRIPTION: [SELF_DESTRUCT!=0]
-- DESCRIPTION: <Sound0>
-- DESCRIPTION: <Sound1>
-- DESCRIPTION: <Sound2>
-- DESCRIPTION: <Sound3>
-- DESCRIPTION: <Sound4>

local lower = string.lower

local asobject 			= {}
local sound_style 		= {}
local sound_volume		= {}
local sound_delay		= {}
local self_destruct		= {}

local status		= {}
local delay			= {}
local doonce		= {}
local slot			= {}
local played		= {}
	
function activation_sounder_properties(e, sound_style, sound_volume, sound_delay, self_destruct)
	asobject[e].sound_style = sound_style
	asobject[e].sound_volume = sound_volume
	asobject[e].sound_delay = sound_delay
	asobject[e].self_destruct = self_destruct or 0
end
 
function activation_sounder_init(e)
	asobject[e] = {}
	asobject[e].sound_style = 1
	asobject[e].sound_volume = 100
	asobject[e].sound_delay = 0
	asobject[e].self_destruct = 0
	
	math.randomseed(os.time())
	math.random(); math.random(); math.random()	
	status[e] = "init"
	delay[e] = math.huge
	doonce[e] = 0
	slot[e] = 0
	played[e] = 0
end
 
function activation_sounder_main(e)
	if status[e] == "init" then
		status[e] = "process"
	end

	if status[e] == "process" then		
		if g_Entity[e]['activated'] == 0 then played[e] = 0	end
		if g_Entity[e]['activated'] == 1 then
			if doonce[e] == 0 then 
				delay[e] = (g_Time + asobject[e].sound_delay)
				doonce[e] = 1
			end	
			if g_Time > delay[e] then
				if played[e] == 0 then
					SetSound(e,slot[e])
					SetSoundVolume(asobject[e].sound_volume)				
					if asobject[e].sound_style == 1	then PlaySound(e,slot[e]) end
					if asobject[e].sound_style == 2	then LoopSound(e,slot[e]) end
					if asobject[e].sound_style == 3	then PlaySound(e,slot[e]) end
					if asobject[e].sound_style == 4	then PlaySound(e,slot[e]) end					
					played[e] = 1
				end			

				if played[e] == 1 and asobject[e].sound_style == 1 then	
					if asobject[e].self_destruct == 1 then						
						SwitchScript(e,"no_behavior_selected.lua")
						Destroy(e)
					else
						played[e] = 0
						doonce[e] = 0
						SetActivated(e,0)					
					end
				end
				if played[e] == 1 and asobject[e].sound_style == 2 then
					if g_Entity[e]['health'] <= 0 then
						StopSound(e,0)
						if asobject[e].self_destruct == 1 then
							SwitchScript(e,"no_behavior_selected.lua")
							Destroy(e)
						else
							played[e] = 0
							doonce[e] = 0
							SetActivated(e,0)
						end
					end	
				end
				if played[e] == 1 and asobject[e].sound_style == 3 then					
					slot[e] = slot[e] + 1
					if slot[e] == 5 then slot[e] = 0 end
					played[e] = 0
					doonce[e] = 0
					SetActivated(e,0)
				end
				if played[e] == 1 and asobject[e].sound_style == 4 then					
					slot[e] = math.random(0,4)
					played[e] = 0
					doonce[e] = 0
					SetActivated(e,0)
				end
			end	
		end
	end
end 
 
function activation_sounder_exit(e) 
end