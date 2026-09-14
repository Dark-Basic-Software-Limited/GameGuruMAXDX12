-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Wicked Particle Emmitter Activator v4 
-- DESCRIPTION: Attach to an object and activate by a linked switch or zone or set IsActive to ON.
-- DESCRIPTION: [WPEFILE$="particlesbank//wpe//firearea.pe"] Name of WPE Particle file
-- DESCRIPTION: [Y_ADJUSTMENT=0] Y adjustment of the effect.
-- DESCRIPTION: [IsActive!=0] if unticked will need to be activated by switch or zone
-- DESCRIPTION: [@SOUND_STYLE=1(1=Play Once, 2=Loop Sound)] Effect sound
-- DESCRIPTION: <Sound0> Effect sound

local wpeactivator 		= {}
local wpefile			= {}
local effectid 			= {}
local y_adjustment 		= {}
local isactive			= {}
local sound_style		= {}
local wpestate			= {}
local status			= {}

function wpe_activator_properties(e, wpefile, y_adjustment, isactive, sound_style)
	wpeactivator[e].wpefile = wpefile
	wpeactivator[e].effectid = WParticleEffectLoad(wpefile)
	wpeactivator[e].y_adjustment = y_adjustment
	wpeactivator[e].isactive = isactive or 0
	wpeactivator[e].sound_style = sound_style or 1
	WParticleEffectVisible(wpeactivator[e].effectid,0)
	WParticleEffectAction(wpeactivator[e].effectid,3)	
end

function wpe_activator_init(e)
	wpeactivator[e] = {}
	wpeactivator[e].wpefile = ""
	wpeactivator[e].y_adjustment = 0
	wpeactivator[e].isactive = 0
	wpeactivator[e].sound_style = 1	
	wpeactivator[e].effectid = ""
	
	SetEntityAlwaysActive(e,1)
	wpestate[e] = -1
	status[e] = "init"
end

function wpe_activator_main(e)

	if status[e] == "init" then
		if wpeactivator[e].isactive == 1 then
			SetActivated(e,1)
		else
			SetActivated(e,0)
		end	
		status[e] = "process"
	end
	
	if status[e] == "process" then 
		if wpeactivator[e].effectid > 0 then
			if g_Entity[e]['activated'] == 1 then
				WParticleEffectPosition(wpeactivator[e].effectid, g_Entity[e]['x'], g_Entity[e]['y'] + wpeactivator[e].y_adjustment, g_Entity[e]['z'])				
				if wpestate[e] ~= 1 then
					WParticleEffectVisible(wpeactivator[e].effectid,1)
					WParticleEffectAction(wpeactivator[e].effectid,3)
					WParticleEffectAction(wpeactivator[e].effectid,1)
					if wpeactivator[e].sound_style == 1 then PlaySound(e,0) end
					if wpeactivator[e].sound_style == 2 then LoopSound(e,0) end					
					wpestate[e] = 1
				end
			else	
				if wpestate[e] ~= 0 then
					WParticleEffectVisible(wpeactivator[e].effectid,0)
					WParticleEffectAction(wpeactivator[e].effectid,2)
					wpestate[e] = 0
				end
			end
			if g_Entity[e]['activated'] == 0 then
				StopSound(e,0)
				WParticleEffectVisible(wpeactivator[e].effectid,0)
				WParticleEffectAction(wpeactivator[e].effectid,3)
			end
			if g_Entity[e]['health'] <= 0 then
				SetPreExitValue(e,1)
			end	
		end	
	end	
end
-- WParticleEffectAction(effectid,Action) - Action =  1=Burst all 2=Pause 3=Resume 4=Restart
function wpe_activator_preexit(e)
	WParticleEffectVisible(wpeactivator[e].effectid,0)
	WParticleEffectAction(wpeactivator[e].effectid,3)
end