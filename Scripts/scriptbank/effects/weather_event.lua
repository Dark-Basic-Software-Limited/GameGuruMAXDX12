-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Weather Event v9 by Necrym59 with special thanks to Bolt Action Gaming
-- DESCRIPTION: Apply to an object and can be activated by switch or zone.
-- DESCRIPTION: [#EVENT_PACE=0.01(0.01,60.0)] Higher = Slower
-- DESCRIPTION: [CLOUD_DENSITY=1(1,400)]
-- DESCRIPTION: [CLOUD_COVERAGE=1(1,200)]
-- DESCRIPTION: [CLOUD_HEIGHT=1(-100,3500)] Meters
-- DESCRIPTION: [CLOUD_THICKNESS=1(1,400)] Meters
-- DESCRIPTION: [CLOUD_SPEED=1(1,50)]
-- DESCRIPTION: [#WIND_SPEED=0.1(0.0,0.3)]
-- DESCRIPTION: [SUN_EXPOSURE=1(1,400)]
-- DESCRIPTION: [TRIGGER_STAGE=2(1,7)] Will activate an external entity at this event stage
-- DESCRIPTION: [WEATHER_WARNING$=""] Weather warning text sent to a User Global at stage before trigger stage
-- DESCRIPTION: [@@WARNING_USER_GLOBAL$=""(0=globallist)] eg: MyWeatherWarning
-- DESCRIPTION: [!EVENT_REPEAT=0] if set ON will repeat the event after delay
-- DESCRIPTION: [REPEAT_DELAY=60(1,300)] Seconds
-- DESCRIPTION: [DIAGNOSTIC!=0]

local weather_event			= {}
local event_pace 			= {}
local cloud_density 		= {}
local cloud_coverage 		= {}
local cloud_height 			= {}
local cloud_thickness 		= {}
local cloud_speed 			= {}
local wind_speed			= {}
local sun_exposure 			= {}
local trigger_stage			= {}
local weather_warning		= {}
local warning_user_global	= {}
local event_repeat			= {}
local repeat_delay			= {}
local diagnostic			= {}

local current_value	= {}
local doonce		= {}
local endval1 		= {}
local endval2 		= {}
local endval3 		= {}
local endval4 		= {}
local endval5 		= {}
local endval6		= {}
local endval7		= {}
local endvalue 		= {}
local pace			= {}
local status		= {}
local trigonce		= {}
local warnonce		= {}
local timedelay		= {}
local original_density	= {}
local original_coverage	= {}
local original_height 	= {}
local original_thickness= {}
local original_speed 	= {}
local original_wind 	= {}
local original_sun      = {}
local total_ms			= {}	

-- Unique velocities for each property
local v_den = {}
local v_cov = {}
local v_hei = {}
local v_thi = {}
local v_spd = {}
local v_win = {}
local v_sun = {}

function weather_event_properties(e, event_pace, cloud_density, cloud_coverage, cloud_height, cloud_thickness, cloud_speed, wind_speed, sun_exposure, trigger_stage, weather_warning, warning_user_global, event_repeat, repeat_delay, diagnostic)
	weather_event[e].event_pace = event_pace
	weather_event[e].cloud_density = cloud_density
	weather_event[e].cloud_coverage = cloud_coverage
	weather_event[e].cloud_height = cloud_height
	weather_event[e].cloud_thickness = cloud_thickness
	weather_event[e].cloud_speed = cloud_speed
	weather_event[e].wind_speed = wind_speed
	weather_event[e].sun_exposure = sun_exposure
	weather_event[e].trigger_stage = trigger_stage
	weather_event[e].weather_warning = weather_warning
	weather_event[e].warning_user_global = warning_user_global
	weather_event[e].event_repeat = event_repeat or 0
	weather_event[e].repeat_delay = (repeat_delay * 1000)
	weather_event[e].diagnostic	= diagnostic or 0
end

function weather_event_init(e)
	weather_event[e] = {}
	weather_event[e].event_pace = 1
	weather_event[e].cloud_density = 0
	weather_event[e].cloud_coverage = 0
	weather_event[e].cloud_height = 0
	weather_event[e].cloud_thickness = 0
	weather_event[e].cloud_speed = 0
	weather_event[e].wind_speed = 0
	weather_event[e].sun_exposure = 0
	weather_event[e].trigger_stage = 0
	weather_event[e].weather_warning = ""
	weather_event[e].warning_user_global = ""
	weather_event[e].event_repeat = 0
	weather_event[e].repeat_delay = 60
	weather_event[e].diagnostic	= 0

	SetEntityAlwaysActive(e,1)
	SetActivated(e,0)
	status[e] = "init"
end

function weather_event_main(e)

	if status[e] == "init" then
		original_density[e] = 0
		original_coverage[e] = 0
		original_height[e] = 0
		original_thickness[e] = 0
		original_speed[e] = 0
		original_wind[e] = 0
		original_sun[e] = 0
		v_den[e], v_cov[e], v_hei[e], v_thi[e], v_spd[e], v_win[e], v_sun[e] = 0,0,0,0,0,0,0
		doonce[e] = 0
		endval1[e], endval2[e], endval3[e], endval4[e], endval5[e], endval6[e], endval7[e] = 0,0,0,0,0,0,0
		endvalue[e] = 0
		trigonce[e] = 0
		warnonce[e] = 0
		pace[e] = 0
		total_ms[e] = 0
		timedelay[e] = 0		
		status[e] = "do_event"
	end

	if g_Entity[e]['activated'] == 1 then
		if doonce[e] == 0 then
			-- Capture original values
			original_density[e] = GetCloudDensity()
			original_coverage[e] = GetCloudCoverage()
			original_height[e] = GetCloudHeight()
			original_thickness[e] = GetCloudThickness()
			original_speed[e] = GetCloudSpeed()
			original_wind[e] = GetTreeWind()
			original_sun[e] = GetExposure() * 100

			-- Velocity is now (Target - Start) / Total Milliseconds
			total_ms[e] = (weather_event[e].event_pace * 2.5) * 100
			if total_ms[e] < 1 then total_ms[e] = 1 end

			v_den[e] = ((weather_event[e].cloud_density / 100) - original_density[e]) / total_ms[e]
			v_cov[e] = ((weather_event[e].cloud_coverage / 100) - original_coverage[e]) / total_ms[e]
			v_hei[e] = ((weather_event[e].cloud_height * 39.36) - original_height[e]) / total_ms[e]
			v_thi[e] = ((weather_event[e].cloud_thickness * 393.6) - original_thickness[e]) / total_ms[e]
			v_spd[e] = (weather_event[e].cloud_speed - original_speed[e]) / total_ms[e]
			v_win[e] = (weather_event[e].wind_speed - original_wind[e]) / total_ms[e]
			v_sun[e] = ((weather_event[e].sun_exposure / 100) - (original_sun[e]/100)) / total_ms[e]

			if weather_event[e].warning_user_global ~= "" then
				_G["g_UserGlobal['"..weather_event[e].warning_user_global.."']"] = ""
			end

			doonce[e] = 1
		end

		if status[e] == "do_event" then
			-- Frame-rate independent increments
			local delta = g_TimePassed or 1
			
			-- Density
			if endval1[e] == 0 then
				SetCloudDensity(GetCloudDensity() + (v_den[e] * delta))
				if (v_den[e] >= 0 and GetCloudDensity()*100 >= weather_event[e].cloud_density) or (v_den[e] < 0 and GetCloudDensity()*100 <= weather_event[e].cloud_density) or v_den[e] == 0 then
					SetCloudDensity(weather_event[e].cloud_density/100)
					endval1[e] = 1
				end
			end
			-- Coverage
			if endval2[e] == 0 then
				SetCloudCoverage(GetCloudCoverage() + (v_cov[e] * delta))
				if (v_cov[e] >= 0 and GetCloudCoverage()*100 >= weather_event[e].cloud_coverage) or (v_cov[e] < 0 and GetCloudCoverage()*100 <= weather_event[e].cloud_coverage) or v_cov[e] == 0 then
					SetCloudCoverage(weather_event[e].cloud_coverage/100)
					endval2[e] = 1
				end
			end
			-- Height
			if endval3[e] == 0 then
				SetCloudHeight(GetCloudHeight() + (v_hei[e] * delta))
				local target_h = weather_event[e].cloud_height * 39.36
				if (v_hei[e] >= 0 and GetCloudHeight() >= target_h) or (v_hei[e] < 0 and GetCloudHeight() <= target_h) or v_hei[e] == 0 then
					SetCloudHeight(target_h)
					endval3[e] = 1
				end
			end
			-- Thickness
			if endval4[e] == 0 then
				SetCloudThickness(GetCloudThickness() + (v_thi[e] * delta))
				local target_t = weather_event[e].cloud_thickness * 393.6
				if (v_thi[e] >= 0 and GetCloudThickness() >= target_t) or (v_thi[e] < 0 and GetCloudThickness() <= target_t) or v_thi[e] == 0 then
					SetCloudThickness(target_t)
					endval4[e] = 1
				end
			end
			-- Speed
			if endval5[e] == 0 then
				SetCloudSpeed(GetCloudSpeed() + (v_spd[e] * delta))
				if (v_spd[e] >= 0 and GetCloudSpeed() >= weather_event[e].cloud_speed) or (v_spd[e] < 0 and GetCloudSpeed() <= weather_event[e].cloud_speed) or v_spd[e] == 0 then
					SetCloudSpeed(weather_event[e].cloud_speed)
					endval5[e] = 1
				end
			end
			-- Exposure
			if endval6[e] == 0 then
				if weather_event[e].sun_exposure ~= 0 then
					SetExposure(GetExposure() + (v_sun[e] * delta))
					if (v_sun[e] >= 0 and GetExposure()*100 >= weather_event[e].sun_exposure) or (v_sun[e] < 0 and GetExposure()*100 <= weather_event[e].sun_exposure) then
						SetExposure(weather_event[e].sun_exposure/100)
						endval6[e] = 1
					end
				else
					endval6[e] = 1
				end
			end
			-- Wind
			if endval7[e] == 0 then
				SetTreeWind(GetTreeWind() + (v_win[e] * delta))
				if (v_win[e] >= 0 and GetTreeWind() >= weather_event[e].wind_speed) or (v_win[e] < 0 and GetTreeWind() <= weather_event[e].wind_speed) or v_win[e] == 0 then
					SetTreeWind(weather_event[e].wind_speed)
					endval7[e] = 1
				end
			end

			endvalue[e] = endval1[e]+endval2[e]+endval3[e]+endval4[e]+endval5[e]+endval6[e]+endval7[e]

			if endvalue[e] == (weather_event[e].trigger_stage - 1) and warnonce[e] == 0 then
				if weather_event[e].warning_user_global ~= "" then
					_G["g_UserGlobal['"..weather_event[e].warning_user_global.."']"] = weather_event[e].weather_warning
					warnonce[e] = 1
				end
			end
			if endvalue[e] == weather_event[e].trigger_stage and trigonce[e] == 0 then
				ActivateIfUsed(e)
				PerformLogicConnections(e)
				trigonce[e] = 1
			end

			if endvalue[e] >= 7 then
				status[e] = "start_fade"
				PerformLogicConnections(e)
			end
		end

		if status[e] == "start_fade" then
			total_ms[e] = (weather_event[e].event_pace * 2.5) * 100
			if total_ms[e] < 1 then total_ms[e] = 1 end
			
			v_den[e] = (original_density[e] - GetCloudDensity()) / total_ms[e]
			v_cov[e] = (original_coverage[e] - GetCloudCoverage()) / total_ms[e]
			v_hei[e] = (original_height[e] - GetCloudHeight()) / total_ms[e]
			v_thi[e] = (original_thickness[e] - GetCloudThickness()) / total_ms[e]
			v_spd[e] = (original_speed[e] - GetCloudSpeed()) / total_ms[e]
			v_win[e] = (original_wind[e] - GetTreeWind()) / total_ms[e]
			v_sun[e] = ((original_sun[e]/100) - GetExposure()) / total_ms[e]
			
			timedelay[e] = 0
			status[e] = "fade_back"
		end

		if status[e] == "fade_back" then
			local delta = g_TimePassed or 1
			SetCloudDensity(GetCloudDensity() + (v_den[e] * delta))
			SetCloudCoverage(GetCloudCoverage() + (v_cov[e] * delta))
			SetCloudHeight(GetCloudHeight() + (v_hei[e] * delta))
			SetCloudThickness(GetCloudThickness() + (v_thi[e] * delta))
			SetCloudSpeed(GetCloudSpeed() + (v_spd[e] * delta))
			SetTreeWind(GetTreeWind() + (v_win[e] * delta))
			SetExposure(GetExposure() + (v_sun[e] * delta))

			timedelay[e] = timedelay[e] + delta
			if timedelay[e] >= (weather_event[e].event_pace * 2.5 * 1000) then
				SetCloudDensity(original_density[e])
				SetCloudCoverage(original_coverage[e])
				SetCloudHeight(original_height[e])
				SetCloudThickness(original_thickness[e])
				SetCloudSpeed(original_speed[e])
				SetTreeWind(original_wind[e])
				SetExposure(original_sun[e]/100)
				status[e] = "end_delay"
				StartTimer(e)
			end
		end
		-- (Diagnostic Text blocks remain same)
		if weather_event[e].diagnostic == 1 then			
			Text(5,44,3,"Status: " ..status[e])
			Text(5,46,3,"Time: " ..g_Time)
			Text(5,48,3,"Event Pace: " ..weather_event[e].event_pace)
			Text(5,50,3,"Cloud Density: " .. GetCloudDensity()*100 .." Original : " .. original_density[e])
			Text(5,52,3,"Cloud Cover: " ..GetCloudCoverage()*100 .. " Original : " .. original_coverage[e])
			Text(5,54,3,"Cloud Height: " ..GetCloudHeight()/39.36 .. " Original : " .. original_height[e]/39.36)
			Text(5,56,3,"Cloud Thickness: " ..GetCloudThickness()/393.6 .. " Original : " .. original_thickness[e]/393.6)
			Text(5,58,3,"Cloud Speed: " ..GetCloudSpeed() .. " Original : " .. original_speed[e])
			Text(5,60,3,"Sun Exposure: " ..GetExposure()*100 .. " Original : " .. original_sun[e])
			Text(5,62,3,"Wind Speed: " ..GetTreeWind() .. " Original : " .. original_wind[e])
			Text(5,64,3,"Current Stage: " ..endvalue[e].. " of 7")
			Text(5,66,3,"Trigger Stage: " ..weather_event[e].trigger_stage)
			if weather_event[e].event_repeat == 0 then
				Text(5,68,3,"Event Repeat: OFF")
			end
			if weather_event[e].event_repeat == 1 then
				Text(5,68,3,"Event Repeat: ON  Delay: " .. weather_event[e].repeat_delay/1000 .. " seconds")
				if GetTimer(e)/1000 < weather_event[e].repeat_delay/1000 then Text(5,70,3,"Event Repeat In: " .. GetTimer(e)/1000 .. " seconds") end
			end
		end
	end
	
	if status[e] == "end_delay" and weather_event[e].event_repeat == 0 then
		status[e] = "init"		
		SetActivated(e,0)
	end
	if status[e] == "end_delay" and weather_event[e].event_repeat == 1 and GetTimer(e) > weather_event[e].repeat_delay then	
		status[e] = "init"  -- Total re-initialisation end
	end
end

function weather_event_exit(e)
end