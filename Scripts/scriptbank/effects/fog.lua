-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Fog v9 by Necrym with special thanks to Bolt Action Gaming
-- DESCRIPTION: Add to an object then link to a switch or multi-trigger zone to activate/de-activate.
-- DESCRIPTION: [PROMPT_TEXT$="Fog strength is changing"]
-- DESCRIPTION: [FOG_NEAREST=1(0,100)]
-- DESCRIPTION: [FOG_DISTANCE=5(0,100)]
-- DESCRIPTION: [FOG_SPEED=25(1,100)]
-- DESCRIPTION: [@FOG_COLOUR_CHANGE=1(1=Off, 2=On)]
-- DESCRIPTION: [FOG_RED=0(0,255)]
-- DESCRIPTION: [FOG_GREEN=0(0,255)]
-- DESCRIPTION: [FOG_BLUE=0(0,255)]
-- DESCRIPTION: [FOG_INTENSITY=0(0,100)]

	local fog 					= {}
	local prompt_text 			= {}
	local fog_nearest			= {}
	local fog_distance			= {}
	local fog_speed				= {}
	local fog_colour_change		= {}
	local fog_red				= {}
	local fog_green				= {}
	local fog_blue				= {}	
	local fog_intensity			= {}
	
	local status 				= {}
	local default_fogn			= {}
	local default_fogd			= {}	
	local default_fog_r			= {}
	local default_fog_g			= {}
	local default_fog_b			= {}
	local default_fog_intensity = {}
	local fognear				= {}
	local fogdist				= {}
	local fbspeed				= {}
	local fogswitch				= {}
	local doonce				= {}	
	local played				= {}
	local current_fog_r			= {}
	local current_fog_g			= {}
	local current_fog_b			= {}
	local current_fog_intensity = {}
	local fog_r_set				= {}
	local fog_g_set				= {}
	local fog_b_set				= {}
	local fog_i_set				= {} 
	local fog_rgbset			= {}

function fog_GetTargetFogUnit(percent_input)
    percent_input = math.max(0, math.min(100, percent_input)) -- Clamp input to 0-100
	local units = percent_input^2 * 100 
    return units
end

function fog_properties(e, prompt_text, fog_nearest, fog_distance, fog_speed, fog_colour_change, fog_red, fog_green, fog_blue, fog_intensity)
	fog[e].prompt_text = prompt_text
	fog[e].fog_nearest = fog_GetTargetFogUnit(fog_nearest)
	fog[e].fog_distance	= fog_GetTargetFogUnit(fog_distance)
	fog[e].fog_speed = fog_speed
	fog[e].fog_colour_change = fog_colour_change or 1
	fog[e].fog_red = fog_red
	fog[e].fog_green = fog_green
	fog[e].fog_blue = fog_blue	
	fog[e].fog_intensity = fog_intensity * .01
end

function fog_init(e)
	fog[e] = {}
	fog[e].prompt_text = "Fog strength is changing"
	fog[e].fog_nearest = 1
	fog[e].fog_distance	= 5
	fog[e].fog_speed = 10
	fog[e].fog_colour_change = 1
	
	fog[e].fog_red = GetFogRed()
	fog[e].fog_green = GetFogGreen()
	fog[e].fog_blue = GetFogBlue()
	fog[e].fog_intensity = GetFogIntensity() 
	
	fognear[e] = 0
	fogdist[e] = 0
	current_fog_r[e] = 0
	current_fog_g[e] = 0
	current_fog_b[e] = 0
	current_fog_intensity[e] = 0
	fog_r_set[e] = 0
	fog_g_set[e] = 0
	fog_b_set[e] = 0
	fog_i_set[e] = 0 
	fog_rgbset[e] = 0
	fbspeed[e] = fog[e].fog_speed
	fogswitch[e] = 0
	doonce[e] = 0
	status[e] = "init"
	played[e] = 0
end

function fog_main(e)

	if status[e] == "init" then
		--fogswitch[e] = 0
		default_fogn[e]	= GetFogNearest()
		default_fogd[e]	= GetFogDistance()
		default_fog_r[e] = GetFogRed()
		default_fog_g[e] = GetFogGreen()
		default_fog_b[e] = GetFogBlue()
		default_fog_intensity[e] = GetFogIntensity() -- Added default intensity
		current_fog_r[e] = default_fog_r[e]
		current_fog_g[e] = default_fog_g[e]
		current_fog_b[e] = default_fog_b[e]	
		current_fog_intensity[e] = default_fog_intensity[e]
		fognear[e] = (default_fogn[e])
		fogdist[e] = (default_fogd[e])
		fbspeed[e] = fog[e].fog_speed
		if fogdist[e] > fog[e].fog_distance then fogswitch[e] = 0 end
		if fogdist[e] < default_fogd[e] then fogswitch[e] = 1 end
		SetActivated(e,0)
		status[e] = "endinit"
	end

	if g_Entity[e]['activated'] == 1 then

		if fogswitch[e] == 0 then
			Prompt(fog[e].prompt_text)	
			local near_step = fbspeed[e] / 5000
			if math.abs(fognear[e] - fog[e].fog_nearest) > 10 then 
				local neardistance_to_go = fog[e].fog_nearest - fognear[e]
				local step_size = neardistance_to_go * near_step
				if step_size < 100 and step_size > 0 then step_size = 100 end 
				if step_size > -100 and step_size < 0then step_size = -100 end 
				fognear[e] = fognear[e] + step_size
				SetFogNearest(fognear[e])
				if (neardistance_to_go > 0 and fognear[e] >= fog[e].fog_nearest) or (neardistance_to_go < 0 and fognear[e] <= fog[e].fog_nearest) then
					fognear[e] = fog[e].fog_nearest
				end
			else
				fognear[e] = fog[e].fog_nearest
			end
			
			local dist_step = fbspeed[e] / 5000
			if math.abs(fogdist[e] - fog[e].fog_distance) > 10 then 
				local distance_to_go = fog[e].fog_distance - fogdist[e]
				local step_size = distance_to_go * dist_step 
				if step_size < 100 and step_size > 0 then step_size = 100 end 
				if step_size > -100 and step_size < 0 then step_size = -100 end 
				fogdist[e] = fogdist[e] + step_size 
				SetFogDistance(fogdist[e])
				if (distance_to_go > 0 and fogdist[e] >= fog[e].fog_distance) or (distance_to_go < 0 and fogdist[e] <= fog[e].fog_distance) then
					fogdist[e] = fog[e].fog_distance
				end
			else
				fogdist[e] = fog[e].fog_distance
			end 
			
			
			if fog[e].fog_colour_change == 1 then fog_rgbset[e] = 3 end
			if fog[e].fog_colour_change == 2 then 				
				if current_fog_r[e] < fog[e].fog_red and fog_r_set[e] == 0 then
					SetFogRed(current_fog_r[e])
					current_fog_r[e] = current_fog_r[e] + fbspeed[e]/100
					if fog[e].fog_red <= current_fog_r[e] then
						current_fog_r[e] = fog[e].fog_red
						fog_r_set[e] = 1
					end
				end
				if current_fog_r[e] > fog[e].fog_red and fog_r_set[e] == 0 then
					SetFogRed(current_fog_r[e])
					current_fog_r[e] = current_fog_r[e] - fbspeed[e]/100
					if fog[e].fog_red >= current_fog_r[e] then
						fog_r_set[e] = 1
						current_fog_r[e] = fog[e].fog_red
					end
				end
				
				if current_fog_g[e] < fog[e].fog_green and fog_g_set[e] == 0 then
					SetFogGreen(current_fog_g[e])
					current_fog_g[e] = current_fog_g[e] + fbspeed[e]/100
					if fog[e].fog_green <= current_fog_g[e] then
						fog_g_set[e] = 1
						current_fog_g[e] = fog[e].fog_green
					end
				end
				if current_fog_g[e] > fog[e].fog_green and fog_g_set[e] == 0 then
					SetFogGreen(current_fog_g[e])
					current_fog_g[e] = current_fog_g[e] - fbspeed[e]/100
					if fog[e].fog_green >= current_fog_g[e] then
						fog_g_set[e] = 1
						current_fog_g[e] = fog[e].fog_green
					end
				end
				
				if current_fog_b[e] < fog[e].fog_blue and fog_b_set[e] == 0 then
					SetFogBlue(current_fog_b[e])
					current_fog_b[e] = current_fog_b[e] + fbspeed[e]/100
					if fog[e].fog_blue <= current_fog_b[e] then
						fog_b_set[e] = 1
						current_fog_b[e] = fog[e].fog_blue
					end
				end
				if current_fog_b[e] > fog[e].fog_blue and fog_b_set[e] == 0 then
					SetFogBlue(current_fog_b[e])
					current_fog_b[e] = current_fog_b[e] - fbspeed[e]/100
					if fog[e].fog_blue >= current_fog_b[e] then
						fog_b_set[e] = 1
						current_fog_b[e] = fog[e].fog_blue
					end
				end

				local f_step = fbspeed[e]/10000
				if current_fog_intensity[e] < fog[e].fog_intensity and fog_i_set[e] == 0 then
					SetFogIntensity(current_fog_intensity[e])
					current_fog_intensity[e] = current_fog_intensity[e] + f_step
					if fog[e].fog_intensity <= current_fog_intensity[e] then
						fog_i_set[e] = 1
						current_fog_intensity[e] = fog[e].fog_intensity
					end
				end
				if current_fog_intensity[e] > fog[e].fog_intensity and fog_i_set[e] == 0 then
					SetFogIntensity(current_fog_intensity[e])
					current_fog_intensity[e] = current_fog_intensity[e] - f_step
					if fog[e].fog_intensity >= current_fog_intensity[e] then
						fog_i_set[e] = 1
						current_fog_intensity[e] = fog[e].fog_intensity
					end
				end
						
				if current_fog_r[e] == fog[e].fog_red then fog_r_set[e] = 1 end
				if current_fog_g[e] == fog[e].fog_green then fog_g_set[e] = 1 end
				if current_fog_b[e] == fog[e].fog_blue then fog_b_set[e] = 1 end
				if current_fog_intensity[e] == fog[e].fog_intensity then fog_i_set[e] = 1 end
				fog_rgbset[e] = fog_r_set[e] + fog_g_set[e] + fog_b_set[e]
			end		
						
			if fognear[e] == fog[e].fog_nearest and fogdist[e] == fog[e].fog_distance and fog_rgbset[e] == 3 and fog_i_set[e] == 1 then
				fogswitch[e] = 1				
				fog_r_set[e] = 0
				fog_g_set[e] = 0
				fog_b_set[e] = 0
				fog_i_set[e] = 0 
				fog_rgbset[e] = 0
				SetActivated(e,0)
				return				
			end
		end

		if fogswitch[e] == 1 then			
			Prompt(fog[e].prompt_text)
			local near_step = fbspeed[e] / 5000
			if math.abs(fognear[e] - default_fogn[e]) > 10 then 
				local neardistance_to_go = default_fogn[e] - fognear[e]
				local step_size = (neardistance_to_go * near_step)
				if step_size < 100 and step_size > 0 then step_size = 100 end
				if step_size > -100 and step_size < 0 then step_size = -100 end
				fognear[e] = fognear[e] + step_size
				SetFogNearest(fognear[e])
				if (neardistance_to_go > 0 and fognear[e] >= default_fogn[e]) or (neardistance_to_go < 0 and fognear[e] <= default_fogn[e]) then
					fognear[e] = default_fogn[e]
				end
			else
				fognear[e] = default_fogn[e]
			end
			
			local dist_step = fbspeed[e] / 5000
			if math.abs(fogdist[e] - default_fogd[e]) > 10 then 
				local distance_to_go = default_fogd[e] - fogdist[e]
				local step_size = distance_to_go * dist_step
				if step_size < 100 and step_size > 0 then step_size = 100 end
				if step_size > -100 and step_size < 0 then step_size = -100 end

				fogdist[e] = fogdist[e] + step_size
				SetFogDistance(fogdist[e])
				if (distance_to_go > 0 and fogdist[e] >= default_fogd[e]) or (distance_to_go < 0 and fogdist[e] <= default_fogd[e]) then
					fogdist[e] = default_fogd[e]
				end
			else
				fogdist[e] = default_fogd[e]
			end 
			
			if fog[e].fog_colour_change == 1 then fog_rgbset[e] = 3 end
			if fog[e].fog_colour_change == 2 then
			
				if current_fog_r[e] < default_fog_r[e] and fog_r_set[e] == 0 then
					SetFogRed(current_fog_r[e])
					current_fog_r[e] = current_fog_r[e] + 0.1
					if default_fog_r[e] <= current_fog_r[e] then					
						fog_r_set[e] = 1
						current_fog_r[e] = default_fog_r[e]
					end
				end
				if current_fog_r[e] > default_fog_r[e] and fog_r_set[e] == 0 then
					SetFogRed(current_fog_r[e])
					current_fog_r[e] = current_fog_r[e] - 0.1
					if default_fog_r[e] >= current_fog_r[e] then
						fog_r_set[e] = 1
						current_fog_r[e] = default_fog_r[e]
					end
				end
				
				if current_fog_g[e] < default_fog_g[e] and fog_g_set[e] == 0 then
					SetFogGreen(current_fog_g[e])
					current_fog_g[e] = current_fog_g[e] + 0.1
					if default_fog_g[e] <= current_fog_g[e] then
						fog_g_set[e] = 1
						current_fog_g[e] = default_fog_g[e]
					end
				end
				if current_fog_g[e] > default_fog_g[e] and fog_g_set[e] == 0 then
					SetFogGreen(current_fog_g[e])
					current_fog_g[e] = current_fog_g[e] - 0.2
					if default_fog_g[e] >= current_fog_g[e] then
						fog_g_set[e] = 1
						current_fog_g[e] = default_fog_g[e]
					end
				end
				
				if current_fog_b[e] < default_fog_b[e] and fog_b_set[e] == 0 then
					SetFogBlue(current_fog_b[e])
					current_fog_b[e] = current_fog_b[e] + 0.2
					if default_fog_b[e] <= current_fog_b[e] then
						fog_b_set[e] = 1
						current_fog_b[e] = default_fog_b[e]
					end
				end
				if current_fog_b[e] > default_fog_b[e] and fog_b_set[e] == 0 then
					SetFogBlue(current_fog_b[e])
					current_fog_b[e] = current_fog_b[e] - 0.2
					if default_fog_b[e] >= current_fog_b[e] then
						fog_b_set[e] = 1
						current_fog_b[e] = default_fog_b[e]
					end
				end
				local f_step = fbspeed[e]/10000
				if current_fog_intensity[e] < default_fog_intensity[e] and fog_i_set[e] == 0 then
					SetFogIntensity(current_fog_intensity[e])
					current_fog_intensity[e] = current_fog_intensity[e] + f_step
					if default_fog_intensity[e] <= current_fog_intensity[e] then
						fog_i_set[e] = 1
						current_fog_intensity[e] = default_fog_intensity[e]
					end
				end
				if current_fog_intensity[e] > default_fog_intensity[e] and fog_i_set[e] == 0 then
					SetFogIntensity(current_fog_intensity[e])
					current_fog_intensity[e] = current_fog_intensity[e] - f_step
					if default_fog_intensity[e] >= current_fog_intensity[e] then
						fog_i_set[e] = 1
						current_fog_intensity[e] = default_fog_intensity[e]
					end
				end 			
				if current_fog_r[e] == default_fog_r[e] then fog_r_set[e] = 1 end
				if current_fog_g[e] == default_fog_g[e] then fog_g_set[e] = 1 end
				if current_fog_b[e] == default_fog_b[e] then fog_b_set[e] = 1 end			
				if current_fog_intensity[e] == default_fog_intensity[e] then fog_i_set[e] = 1 end
				fog_rgbset[e] = fog_r_set[e] + fog_g_set[e] + fog_b_set[e]
			end
						
			if fognear[e] == default_fogn[e] and fogdist[e] == default_fogd[e] and fog_rgbset[e] == 3 and fog_i_set[e] == 1 then
				fogswitch[e] = 0
				fog_r_set[e] = 0
				fog_g_set[e] = 0
				fog_b_set[e] = 0
				fog_i_set[e] = 0
				fog_rgbset[e] = 0				
				SetActivated(e,0)
				return
			end
		end
	end
end
