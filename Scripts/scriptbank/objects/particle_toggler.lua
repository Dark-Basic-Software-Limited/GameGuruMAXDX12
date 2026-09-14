-- PARTICLE_TOGGLER v4 by Necrym59
-- DESCRIPTION: When this entity is triggered it will toggle a named particle on or off.
-- DESCRIPTION: Attach to an object and activate from a link, switch or zone or proximity.
-- DESCRIPTION: [PARTICLE_NAME$=""] particle name
-- DESCRIPTION: [@TRIGGER_TYPE=1(1=External, 2=Proximity)]
-- DESCRIPTION: [PROXIMITY_RANGE=1000]

local lower = string.lower
local particle_toggler		= {}
local particle_name			= {}
local trigger_type			= {}
local proximity_range		= {}
local particle_no			= {}
local status				= {}
	
function particle_toggler_properties(e, particle_name, trigger_type, proximity_range)
	particle_toggler[e].particle_name = string.lower(particle_name)
	particle_toggler[e].trigger_type = trigger_type
	particle_toggler[e].proximity_range = proximity_range
end
 
function particle_toggler_init(e)
	particle_toggler[e] = {}
	particle_toggler[e].particle_name = ""
	particle_toggler[e].trigger_type = 1
	particle_toggler[e].proximity_range = 1000
	particle_toggler[e].particle_no = 0
	
	SetEntityAlwaysActive(e,1)
	status[e] = "init"
end
 
function particle_toggler_main(e)	
	
	if status[e] == "init" then
		if particle_toggler[e].particle_name ~= "" then
			for p = 1, g_EntityElementMax do
				if p ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == particle_toggler[e].particle_name then
						particle_toggler[e].particle_no = p
					end
				end
			end
		end
		Hide(particle_toggler[e].particle_no)
		status[e] = "Off"
	end
	
	if particle_toggler[e].trigger_type == 1 then
		if g_Entity[e]['activated'] == 1 and status[e] == "Off" then			
			EffectStart(particle_toggler[e].particle_no)
			Show(particle_toggler[e].particle_no)
			status[e] = "On"
			SetActivated(e,0)
		end
		if g_Entity[e]['activated'] == 1 and status[e] == "On"  then
			EffectStop(particle_toggler[e].particle_no)
			Hide(particle_toggler[e].particle_no)
			status[e] = "Off"
			SetActivated(e,0)
		end
	end	
	
	if particle_toggler[e].trigger_type == 2 then
		if GetPlayerDistance(e) < particle_toggler[e].proximity_range then
			if status[e] == "Off" then
				status[e] = "On"
				EffectStart(particle_toggler[e].particle_no)
				Show(particle_toggler[e].particle_no)
			end
		else
			if status[e] == "On" then
				status[e] = "Off"
				EffectStop(particle_toggler[e].particle_no)
				Hide(particle_toggler[e].particle_no)
			end
		end
	end	
end