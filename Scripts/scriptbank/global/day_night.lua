-- Day_Night v22 by Necrym59, Lee and Bolt Action Gaming
-- DESCRIPTION: A global behavior to allow for a Day/Night time cycler
-- DESCRIPTION: [#START_ANGLE=-95(-180,180)]
-- DESCRIPTION: [TIME_DILATION=1(1,1000)]
-- DESCRIPTION: [MIN_AMBIENCE_R=10(1,255)]
-- DESCRIPTION: [MIN_AMBIENCE_G=25(1,255)]
-- DESCRIPTION: [MIN_AMBIENCE_B=75(1,255)]
-- DESCRIPTION: [MIN_EXPOSURE#=0.20(0.01,1.00)]
-- DESCRIPTION: [#SUN_ROLL=-95(-180,180)]
-- DESCRIPTION: [#SUN_PITCH=70(-180,180)]
-- DESCRIPTION: [#SUN_YAW=0(-180,180)]
-- DESCRIPTION: [MIN_INTENSITY#=3.45(0.01,50.00)]
-- DESCRIPTION: [MAX_AMBIENCE_R=255(1,255)]
-- DESCRIPTION: [MAX_AMBIENCE_G=255(1,255)]
-- DESCRIPTION: [MAX_AMBIENCE_B=255(1,255)]
-- DESCRIPTION: [MAX_EXPOSURE#=1.00(0.01,1.00)]
-- DESCRIPTION: [MAX_INTENSITY#=7.40(0.01,50.00)]
-- DESCRIPTION: [@TRIGGER_EVENT_A=27(1=1am,2=2am,3=3am,4=4am,5=5am,6=6am,7=7am,8=8am,9=9am,10=10am,11=11am,12=12pm,13=1pm,14=2pm,15=3pm,16=4pm,17=5pm,18=6pm,19=7pm,20=8pm,21=9pm,22=10pm,23=11pm,24=12am,25=7 Days,26=28 Days, 27=None)]
-- DESCRIPTION: [@TRIGGER_EVENT_B=27(1=1am,2=2am,3=3am,4=4am,5=5am,6=6am,7=7am,8=8am,9=9am,10=10am,11=11am,12=12pm,13=1pm,14=2pm,15=3pm,16=4pm,17=5pm,18=6pm,19=7pm,20=8pm,21=9pm,22=10pm,23=11pm,24=12am,25=7 Days,26=28 Days, 27=None)]
-- DESCRIPTION: [@START_DAY=1(1=Sunday, 2=Monday, 3=Tuesday, 4=Wednesday, 5=Thursday, 6=Friday, 7=Saturday)]
-- DESCRIPTION: [@@READOUT_USER_GLOBAL$=""(0=globallist)] User Global for displaying day and time (eg: MyUserGlobal)
-- DESCRIPTION: [LIGHT_CONTROL!=0] Control day/night lights
-- DESCRIPTION: [LIGHT_NAME$="Light"] Name of Light(s) to turn on/off
-- DESCRIPTION: [LIGHT_RANGE=300(1,5000)] Strength of light
-- DESCRIPTION: [DIAGNOSTICS!=0]

local lower = string.lower
g_sunrollposition = {}
g_updatedposition = {}

local day_night = {}
local start_angle = {}
local sun_roll = {}
local sun_pitch = {}
local sun_yaw = {}
local time_dilation = {}
local diagnostics = {}
local min_ambience_r = {}
local min_ambience_g = {}
local min_ambience_b = {}
local min_exposure = {}
local min_intensity = {}
local max_ambience_r = {}
local max_ambience_g = {}
local max_ambience_b = {}
local max_exposure = {}
local max_intensity = {}
local trigger_event_a = {}
local trigger_event_b = {}
local start_day = {}
local readout_user_global = {}

local suntimer = {}
local sunmoonroll = {}
local sunmoonpitch = {}
local sunmoonyaw = {}
local ambrvalue = {}
local ambgvalue = {}
local ambbvalue = {}
local expovalue = {}
local sintvalue = {}
local ambrvaluem = {}
local ambgvaluem = {}
local ambbvaluem = {}
local expovaluem = {}
local sintvaluem = {}
local sunrvalue = {}
local sunbvalue = {}
local sungvalue = {}
local sintvalue = {}
local expovalue = {}

local light_control = {}
local light_name = {}
local light_range = {}
local dolightsoff = {}
local dolightson = {}

local status = {}
local lightlist = {}
local lightNum = {}
local state = {}
local tod = {}
local currenttod = {}
local currentdaytime = {}
local changeday = {}
local daycount = {}
local weekcount = {}
local mode = {}
local event_trig = {}
local a_wastriggered = {}
local b_wastriggered = {}
local runonce = {}


local function math_lerp(a, b, t)
    return a + (b - a) * t
end

function day_night_properties(e, start_angle, time_dilation, min_ambience_r, min_ambience_g, min_ambience_b, min_exposure, sun_roll, sun_pitch, sun_yaw, min_intensity, max_ambience_r, max_ambience_g, max_ambience_b, max_exposure, max_intensity, trigger_event_a, trigger_event_b, start_day, readout_user_global, light_control, light_name, light_range, diagnostics)
	-- start_angle in legacy version now replaced with RPY below but retained for compatability
	day_night[e].start_angle = start_angle
	day_night[e].time_dilation = time_dilation
	day_night[e].min_ambience_r = min_ambience_r
	day_night[e].min_ambience_g = min_ambience_g
	day_night[e].min_ambience_b = min_ambience_b
	day_night[e].min_exposure = min_exposure
	if sun_roll == nil then sun_roll = start_angle end
	day_night[e].sun_roll = sun_roll
	if sun_pitch == nil then sun_pitch = 75 end
	day_night[e].sun_pitch = sun_pitch
	if sun_yaw == nil then sun_yaw = 0 end
	day_night[e].sun_yaw = sun_yaw
	if min_intensity == nil then min_intensity = 3.4 end
	day_night[e].min_intensity = min_intensity
	if max_ambience_r == nil then max_ambience_r = 255 end
	day_night[e].max_ambience_r = max_ambience_r
	if max_ambience_g == nil then max_ambience_g = 255 end
	day_night[e].max_ambience_g = max_ambience_g
	if max_ambience_b == nil then max_ambience_b = 255 end
	day_night[e].max_ambience_b = max_ambience_b
	if max_exposure == nil then max_exposure = 1.00 end
	day_night[e].max_exposure = max_exposure
	if max_intensity == nil then max_intensity = 7.4 end
	day_night[e].max_intensity = max_intensity
	if trigger_event_a == nil then trigger_event_a = 25 end
	day_night[e].trigger_event_a = trigger_event_a
	day_night[e].trigger_event_b = trigger_event_b	
	day_night[e].start_day = start_day
	day_night[e].readout_user_global = readout_user_global
	day_night[e].light_control = light_control or 0
	day_night[e].light_name = light_name
	day_night[e].light_range = light_range
	day_night[e].diagnostics = diagnostics	
end

function day_night_init(e)
	day_night[e] = {}
	day_night[e].start_angle = -95
	day_night[e].sun_roll = -95
	day_night[e].sun_pitch = 75
	day_night[e].sun_yaw = 0
	day_night[e].time_dilation = 1
	day_night[e].min_ambience_r = 0
	day_night[e].min_ambience_g = 0
	day_night[e].min_ambience_b = 0
	day_night[e].min_exposure = 0.00
	day_night[e].min_intensity = 3.4
	day_night[e].max_ambience_r = 255
	day_night[e].max_ambience_g = 255
	day_night[e].max_ambience_b = 255
	day_night[e].max_exposure = 1.00
	day_night[e].max_intensity = 7.4
	day_night[e].trigger_event_a = 27
	day_night[e].trigger_event_b = 27
	day_night[e].start_day = 1
	day_night[e].readout_user_global = ""
	day_night[e].light_control = 1
	day_night[e].light_name = ""
	day_night[e].light_range = 300
	day_night[e].diagnostics = 1	

	status[e] = "init"
	sunrvalue[e] = 255
	sungvalue[e] = 255
	sunbvalue[e] = 255
	sintvalue[e] = day_night[e].min_intensity
	expovalue[e] = day_night[e].min_exposure
	state[e] = ""
	g_sunrollposition = 0
	g_updatedposition = 0
	sunmoonroll[e] = 0
	sunmoonpitch[e] = 0
	sunmoonyaw[e] = 0
	suntimer[e] = math.huge
	sintvalue[e] = 0
	event_trig[e] = 0
	currentdaytime[e] = 0
	mode[e] = ""
	tod[e] = ""
	currenttod[e] = "" 
	a_wastriggered = 0
	b_wastriggered = 0
	runonce = 0 
	changeday[e] = 1
	daycount[e] = 0
	weekcount[e] = 0
	dolightsoff[e] = 0
	dolightson[e] = 0
	lightNum[e] = 0
	Hide(e)
end

function day_night_main(e)
	event_trig[e] = 0
	
	-- INITIALIZATION
	if status[e] == "init" then
		event_trig[e] = 0
		a_wastriggered = 0
		b_wastriggered = 0
		runonce = 0
		ambrvalue[e] = day_night[e].min_ambience_r
		ambgvalue[e] = day_night[e].min_ambience_g
		ambbvalue[e] = day_night[e].min_ambience_b
		expovalue[e] = day_night[e].min_exposure
		sintvalue[e] = day_night[e].min_intensity
		
		-- Logic check: If starting in day, max out values immediately
		if day_night[e].start_angle > -90 and day_night[e].start_angle < 90 then
			ambrvalue[e] = day_night[e].max_ambience_r
			ambgvalue[e] = day_night[e].max_ambience_g
			ambbvalue[e] = day_night[e].max_ambience_b
			expovalue[e] = day_night[e].max_exposure
			sintvalue[e] = day_night[e].max_intensity
		end

		if day_night[e].time_dilation >= 1000 then day_night[e].time_dilation = 1000 end
		sunmoonroll[e] = day_night[e].start_angle
		
		sunmoonpitch[e] = day_night[e].sun_pitch
		sunmoonyaw[e] = day_night[e].sun_yaw
		suntimer[e] = g_Time + 1000
		SetSunDirection(sunmoonroll[e],sunmoonpitch[e],sunmoonyaw[e])
		
		--Check for Lights --
		if day_night[e].light_control == 1 and day_night[e].light_name > "" then
			for n = 1, g_EntityElementMax do
				if n ~= nil and g_Entity[n] ~= nil then				
					if lower(GetEntityName(n)) == lower(day_night[e].light_name) then
						table.insert(lightlist,n)
					end
				end
			end
		end
		status[e] = "endinit"
	end
	
	-- TIME PROGRESSION
	if g_Time > suntimer[e] then
		sunmoonroll[e] = (sunmoonroll[e] + 0.0042) --1 Sec = 0.0042 deg
		SetSunDirection(sunmoonroll[e],sunmoonpitch[e],sunmoonyaw[e])				
		g_sunrollposition = sunmoonroll[e]		
		if g_updatedposition > 0 then			
			sunmoonroll[e] = (g_sunrollposition + g_updatedposition)
			g_updatedposition = 0
		end		
		suntimer[e] = g_Time + 1000 / day_night[e].time_dilation
	end
	
	local target_sun_r = 255
	local target_sun_g = 255
	local target_sun_b = 255
	-- local smoothing_speed = 0.002 -- Lower = Slower/Smoother, Higher = Faster .002 = 1000 dilation  .000002 = 1 dilation. 
	local smoothing_speed = .002 * (day_night[e].time_dilation / 1000)						  --
	local target_intensity = day_night[e].max_intensity
	local target_exposure = day_night[e].max_exposure
	local target_amb_r = day_night[e].max_ambience_r
	local target_amb_g = day_night[e].max_ambience_g
	local target_amb_b = day_night[e].max_ambience_b
	
	-- DETERMINE STATES
	if sunmoonroll[e] < -90 then 
		state[e] = "Night"
	elseif sunmoonroll[e] >= -90 and sunmoonroll[e] < -60 then
		state[e] = "Dawn"
	elseif sunmoonroll[e] >= -60 and sunmoonroll[e] < 60 then
		state[e] = "Day"
	elseif sunmoonroll[e] >= 60 and sunmoonroll[e] < 90 then
		state[e] = "Dusk"
	elseif sunmoonroll[e] >= 90 then
		state[e] = "Night"
	end

	-- APPLY LIGHTING
	if state[e] == "Night" then
		target_sun_r, target_sun_g, target_sun_b = 0, 3, 10
		SetSunIntensity(day_night[e].min_intensity)
		target_intensity = day_night[e].min_intensity
		target_exposure = day_night[e].min_exposure
		target_amb_r = day_night[e].min_ambience_r
		target_amb_g = day_night[e].min_ambience_g
		target_amb_b = day_night[e].min_ambience_b	
		--Loop Moon
		if sunmoonroll[e] > 165 then sunmoonroll[e] = -165 end
	end

	if state[e] == "Dawn" then
		-- Colors: Peach/Pink
		target_sun_r, target_sun_g, target_sun_b = 224, 173, 166
		-- Values: Ramping UP to Max
		target_intensity = day_night[e].max_intensity
		target_exposure = day_night[e].max_exposure
		target_amb_r = day_night[e].max_ambience_r
		target_amb_g = day_night[e].max_ambience_g
		target_amb_b = day_night[e].max_ambience_b
	end

	if state[e] == "Day" then
		-- Max Settings
		target_sun_r, target_sun_g, target_sun_b = 255, 255, 255
		target_intensity = day_night[e].max_intensity
		target_exposure = day_night[e].max_exposure
		target_amb_r = day_night[e].max_ambience_r
		target_amb_g = day_night[e].max_ambience_g
		target_amb_b = day_night[e].max_ambience_b
	end

	if state[e] == "Dusk" then
		-- Colors: Orange/Red
		target_sun_r, target_sun_g, target_sun_b = 255, 100, 50
		-- Values: Ramping DOWN to Min
		target_intensity = day_night[e].min_intensity
		target_exposure = day_night[e].min_exposure
		target_amb_r = day_night[e].min_ambience_r
		target_amb_g = day_night[e].min_ambience_g
		target_amb_b = day_night[e].min_ambience_b
	end
	
	-- SUN COLOR SMOOTHING
	sunrvalue[e] = math_lerp(sunrvalue[e], target_sun_r, smoothing_speed)
	sungvalue[e] = math_lerp(sungvalue[e], target_sun_g, smoothing_speed)
	sunbvalue[e] = math_lerp(sunbvalue[e], target_sun_b, smoothing_speed)
	ambrvalue[e] = math_lerp(ambrvalue[e], target_amb_r, smoothing_speed)
	ambgvalue[e] = math_lerp(ambgvalue[e], target_amb_g, smoothing_speed)
	ambbvalue[e] = math_lerp(ambbvalue[e], target_amb_b, smoothing_speed)
	expovalue[e] = math_lerp(expovalue[e], target_exposure, smoothing_speed/2)
	sintvalue[e] = math_lerp(sintvalue[e], target_intensity, smoothing_speed/2) 
	
	-- APPLY TO ENGINE
	SetSunLightingColor(math.floor(sunrvalue[e]), math.floor(sungvalue[e]), math.floor(sunbvalue[e]))
	SetSunIntensity(sintvalue[e])
	SetExposure(expovalue[e])
	SetAmbienceRed(math.floor(ambrvalue[e]))
	SetAmbienceGreen(math.floor(ambgvalue[e]))
	SetAmbienceBlue(math.floor(ambbvalue[e]))
	SetAmbienceIntensity(120) -- Keep this constant or lerp it if you have a variable for it

	-- DAY CHANGING LOGIC
	if sunmoonroll[e] >= 160 and changeday[e] == 1 then
		changeday[e] = 0
	end

	local trigger_a_val = day_night[e].trigger_event_a 
	local trigger_b_val = day_night[e].trigger_event_b 

	-- TIME OF DAY CALCULATIONS
	if sunmoonroll[e] >= 165.5 then tod[e] = "12am"; if trigger_a_val == 24 or trigger_b_val == 24 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 150.0 then tod[e] = "11pm"; if trigger_a_val == 23 or trigger_b_val == 23 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 135.0 then tod[e] = "10pm"; if trigger_a_val == 22 or trigger_b_val == 22 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 120.0 then tod[e] = "9pm"; if trigger_a_val == 21 or trigger_b_val == 21 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 105.0 then tod[e] = "8pm"; if trigger_a_val == 20 or trigger_b_val == 20 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 90.0 then tod[e] = "7pm"; if trigger_a_val == 19 or trigger_b_val == 19 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 75.0 then tod[e] = "6pm"; if trigger_a_val == 18 or trigger_b_val == 18 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 60.0 then tod[e] = "5pm"; if trigger_a_val == 17 or trigger_b_val == 17 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 50.0 then tod[e] = "4pm"; if trigger_a_val == 16 or trigger_b_val == 16 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 45.0 then tod[e] = "3pm"; if trigger_a_val == 15 or trigger_b_val == 15 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 30.0 then tod[e] = "2pm"; if trigger_a_val == 14 or trigger_b_val == 14 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 15.0 then tod[e] = "1pm"; if trigger_a_val == 13 or trigger_b_val == 13 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > 0 then tod[e] = "12pm"; if trigger_a_val == 12 or trigger_b_val == 12 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -15.0 then tod[e] = "11am"; if trigger_a_val == 11 or trigger_b_val == 11 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -30.0 then tod[e] = "10am"; if trigger_a_val == 10 or trigger_b_val == 10 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -45.0 then tod[e] = "9am"; if trigger_a_val == 9 or trigger_b_val == 9 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -50.0 then tod[e] = "8am"; if trigger_a_val == 8 or trigger_b_val == 8 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -60.0 then tod[e] = "7am"; if trigger_a_val == 7 or trigger_b_val == 7 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -75.0 then tod[e] = "6am"; if trigger_a_val == 6 or trigger_b_val == 6 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -90.0 then tod[e] = "5am"; if trigger_a_val == 5 or trigger_b_val == 5 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -105.0 then tod[e] = "4am"; if trigger_a_val == 4 or trigger_b_val == 4 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -120.0 then tod[e] = "3am"; if trigger_a_val == 3 or trigger_b_val == 3 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -135.5 then tod[e] = "2am"; if trigger_a_val == 2 or trigger_b_val == 2 then event_trig[e] = 1 end
	elseif sunmoonroll[e] > -150.0 then tod[e] = "1am"; if trigger_a_val == 1 or trigger_b_val == 1 then event_trig[e] = 1 end
	elseif sunmoonroll[e] >= -165.5 then
		tod[e] = "12am"
		if trigger_a_val == 24 or trigger_b_val == 24 then event_trig[e] = 1 end
		runonce = 0
		if sunmoonroll[e] >= -165.5 and changeday[e] == 0 then
			day_night[e].start_day = day_night[e].start_day + 1
			if day_night[e].start_day > 7 then day_night[e].start_day = 1 end
			if day_night[e].start_day > 7 then weekcount[e] = 1 end
			daycount[e] = daycount[e] + 1
			changeday[e] = 1
			a_wastriggered = 0
			b_wastriggered = 0 
			runonce = 1
			currenttod[e] = ""
		end
	else
		tod[e] = "error"
	end

	-- LOGIC TRIGGERING
	if day_night[e].trigger_event_a == 25 or day_night[e].trigger_event_b == 25 and weekcount[e] == 1 then event_trig[e] = 1 end
	if day_night[e].trigger_event_a == 26 or day_night[e].trigger_event_b == 26 and daycount[e] == 28 then event_trig[e] = 1 end
	
	if day_night[e].start_day == 1 then currentdaytime[e] = ("Sunday  " ..tod[e]) end
	if day_night[e].start_day == 2 then currentdaytime[e] = ("Monday  " ..tod[e]) end
	if day_night[e].start_day == 3 then currentdaytime[e] = ("Tuesday  " ..tod[e]) end
	if day_night[e].start_day == 4 then currentdaytime[e] = ("Wednesday  " ..tod[e]) end
	if day_night[e].start_day == 5 then currentdaytime[e] = ("Thursday  " ..tod[e]) end
	if day_night[e].start_day == 6 then currentdaytime[e] = ("Friday  " ..tod[e]) end
	if day_night[e].start_day == 7 then currentdaytime[e] = ("Saturday  " ..tod[e]) end

	if _G["g_UserGlobal['"..day_night[e].readout_user_global.."']"] ~= nil or day_night[e].readout_user_global ~= "" then
		_G["g_UserGlobal['"..day_night[e].readout_user_global.."']"] = currentdaytime[e]
	end
	
	--EVENTS
	if event_trig[e] == 1 and tod[e] ~= currenttod[e] then
		if daycount[e] == 28 then daycount[e] = 0 end
		if weekcount[e] == 1 then weekcount[e] = 0 end
		event_trig[e] = 0
		currenttod[e] = tod[e]
		if a_wastriggered == 0 then 
			a_wastriggered = 1
			ActivateIfUsed(e)
			PerformLogicConnections(e)
		else 
			b_wastriggered = 1
			ActivateIfUsed(e)
			PerformLogicConnections(e)
		end 
	end

	--LIGHTS
	if day_night[e].light_control == 1 then
		if state[e] == "Day" or state[e] == "Dawn" then -- Lights OFF
			if dolightsoff[e] == 0 then
				for a,b in pairs (lightlist) do
					SetLightRange(b,0)
					SetActivated(b,0)
				end 
			end
			dolightsoff[e] = 1
			dolightson[e] = 0
		else -- Lights ON (Dusk and Night)
			SetActivated(e,1)
			if dolightson[e] == 0 then
				for a,b in pairs (lightlist) do
					SetLightRange(b,day_night[e].light_range)
					SetActivated(b,1)
				end
			end	
			dolightson[e] = 1
			dolightsoff[e] = 0
		end
	end
	
	--DIAGNOSTICS
	if day_night[e].diagnostics == 1 then
		Text(1,22,3,"Day/Time: " ..currentdaytime[e])
		Text(1,24,3,"Sun/Moon Angle: " ..math.floor(sunmoonroll[e]))
		Text(1,28,3,"Dialation: " ..day_night[e].time_dilation)
		Text(1,30,3,"State: " ..state[e])
		Text(1,32,3,"Ambience R: " ..math.floor(ambrvalue[e]))
		Text(1,34,3,"Ambience G: " ..math.floor(ambgvalue[e]))
		Text(1,36,3,"Ambience B: " ..math.floor(ambbvalue[e]))
		Text(1,38,3,"Exposure: " ..math.floor(expovalue[e]*100)/100)
		Text(1,40,3,"Intensity: " ..math.floor(sintvalue[e]*100)/100)
		Text(1,42,3,"TrigA/B: " .. day_night[e].trigger_event_a .. " " .. day_night[e].trigger_event_b .. " " .. event_trig[e])
		Text(1,44,3,"A/B: " .. a_wastriggered .. " " .. b_wastriggered)
	end
end

function day_night_exit(e)
end