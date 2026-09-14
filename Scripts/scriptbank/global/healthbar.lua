-- Healthbar v14 - by Necrym,59
-- DESCRIPTION: A global behavior that will display a viewed enemys health in a bar or text.
-- DESCRIPTION: [DISPLAY_RANGE=200(100,1000)]
-- DESCRIPTION: [@DISPLAY_MODE=1(1=Health Bar, 2=Health Text, 3=Health Text+Bar, 4=Identity Text, 5=Identity Text+Bar)]
-- DESCRIPTION: [Y_ADJUSTMENT=10(0,50)]
-- DESCRIPTION: [FIXED_Y!=0]
-- DESCRIPTION: [HEALTH_TEXT$="Health:"]
-- DESCRIPTION: [HEALTH_BAR_IMAGEFILE$="imagebank\\buttons\\slider-bar-full.png"]
-- DESCRIPTION: [HEALTH_COLOR_CHANGE=150(1,1000)]

local P = require "scriptbank\\physlib"
local U = require "scriptbank\\utillib"

g_LegacyNPC = {}
local healthbar = {}
local display_range = {}
local display_mode = {}
local y_adjustment = {}
local fixed_y = {}
local health_text = {}
local health_bar = {}
local health_color_change = {}

local rotheight	= {}
local status = {}
local hbarsize = {}
local hreadout = {}
local hbarsprite = {}
local sprwidth = {}
local tagreadout = {}
local tableName = {}
local checktimer = {}
local entrange = {}
local enemies = {}
local doonce = {}

function healthbar_properties(e, display_range, display_mode, y_adjustment, fixed_y, health_text,health_bar, health_color_change)
	healthbar[e].display_range = display_range or 500
	healthbar[e].display_mode = display_mode or 1
	healthbar[e].y_adjustment = y_adjustment
	healthbar[e].fixed_y = fixed_y or 0
	healthbar[e].health_text = health_text
	healthbar[e].health_bar = health_bar
	healthbar[e].health_color_change = health_color_change
end

function healthbar_init(e)
	healthbar[e] = {}
	healthbar[e].display_range = 500
	healthbar[e].display_mode = 1
	healthbar[e].y_adjustment = 0
	healthbar[e].fixed_y = 0
	healthbar[e].health_text = "Health:"
	healthbar[e].health_bar = "imagebank\\buttons\\slider-bar-full.png"
	healthbar[e].health_color_change = 150

	status[e] = "init"
	rotheight[e] = 0
	hbarsize[e] = 0
	hreadout[e] = 0
	hbarsprite[e] = 0
	sprwidth[e] = 0
	tagreadout[e] = ""
	g_LegacyNPC = 0
	doonce[e] = 0
	enemies[e] = 0
	checktimer[e] =	math.huge
	tableName[e] = "hbenemies" ..tostring(e)
	_G[tableName[e]] = {}
	entrange[e] = 0
	SetEntityAlwaysActive(e,1)
end

function healthbar_main(e)

	if status[e] == "init" then
		if healthbar[e].health_bar ~= "" then
			hbarsprite[e] = CreateSprite(LoadImage(healthbar[e].health_bar))
			SetSpriteSize(hbarsprite[e],-1,-1)
			SetSpritePosition(hbarsprite[e],200,200)
		end
		for n = 1, g_EntityElementMax do
			if n ~= nil and g_Entity[n] ~= nil then
				if GetEntityAllegiance(n) == 0 then
					table.insert(_G[tableName[e]],n)
					enemies[e] = enemies[e]+1
				end
			end
		end
		if healthbar[e].hide_this_entity == 1 then
			CollisionOff(e)
			Hide(e)
		end
		sprwidth[e] = 100 + (GetDesktopWidth()/GetDesktopHeight())
		checktimer[e] = g_Time + 2
		status[e] = "active"
	end

	if status[e] == "active" then
		if g_Time > checktimer[e] then
			for _,a in pairs (_G[tableName[e]]) do
				if g_Entity[a] ~= nil then
					entrange[e] = math.ceil(GetFlatDistanceToPlayer(a))
					GetEntityPlayerVisibility(a)
					if U.PlayerLookingNear(a,healthbar[e].display_range,120) == true and GetEntityVisibility(a) == 1 and g_Entity[a]['health'] ~= 999 then
						if g_Entity[a]["health"] > 0 and entrange[e] < healthbar[e].display_range then
							tagreadout[e] = GetEntityName(a)
							--Entity dimensions check--
							Ent = g_Entity[a]
							local dims = P.GetObjectDimensions(Ent.obj)
							if healthbar[e].fixed_y == 0 then rotheight[e] = (dims.h + healthbar[e].y_adjustment) end
							if healthbar[e].fixed_y == 1 then rotheight[e] = healthbar[e].y_adjustment end
							--3dto2d check--
							ScreenPosX = -1
							ScreenPosX,ScreenPosY = Convert3DTo2D(g_Entity[a]['x'],g_Entity[a]['y']+rotheight[e],g_Entity[a]['z'])
							if ScreenPosX < 0 then
								ScreenPosX = 0
								ScreenPosY = 0
							else
								percentx,percenty = ScreenCoordsToPercent(ScreenPosX,ScreenPosY)
							end
							--Health and Healthbar check--
							if g_Entity[a]['health'] > 1000 then
								g_LegacyNPC = 1
							else
								g_LegacyNPC = 0
							end
							if g_LegacyNPC == 0 then hreadout[e] = g_Entity[a]['health'] end
							if g_LegacyNPC == 1 then hreadout[e] = (g_Entity[a]['health']-1000) end
							if g_Entity[a]['health'] < 9000 then
								hbarsize[e] = (hreadout[e]/sprwidth[e])
								SetSpriteSize(hbarsprite[e],hbarsize[e],3)
								if hreadout[e] > healthbar[e].health_color_change then SetSpriteColor(hbarsprite[e],0,255,0,255) end
								if hreadout[e] < healthbar[e].health_color_change then SetSpriteColor(hbarsprite[e],255,0,0,255) end
							end
							--Display Healthbar and Health--
							if healthbar[e].display_mode == 1 and hreadout[e] > 0 then
								PasteSpritePosition(hbarsprite[e],percentx-(hbarsize[e]/2),percenty)
							end
							if healthbar[e].display_mode == 2 and hreadout[e] > 0 then
								if hreadout[e] > healthbar[e].health_color_change then TextCenterOnXColor(percentx,percenty,1,healthbar[e].health_text.. " " ..hreadout[e],0,255,0) end
								if hreadout[e] < healthbar[e].health_color_change then TextCenterOnXColor(percentx,percenty,1,healthbar[e].health_text.. " " ..hreadout[e],255,0,0) end
							end
							if healthbar[e].display_mode == 3 and hreadout[e] > 0 then
								PasteSpritePosition(hbarsprite[e],percentx-(hbarsize[e]/2),percenty)
								if hreadout[e] > healthbar[e].health_color_change then TextCenterOnXColor(percentx,percenty,1,healthbar[e].health_text.. " " ..hreadout[e],0,255,0) end
								if hreadout[e] < healthbar[e].health_color_change then TextCenterOnXColor(percentx,percenty,1,healthbar[e].health_text.. " " ..hreadout[e],255,0,0) end
							end
							if healthbar[e].display_mode == 4 and hreadout[e] > 0 then
								if g_MouseClick == 0 then
									TextCenterOnXColor(percentx,percenty,1,tagreadout[e],255,255,255)
								end
								if g_MouseClick == 2 then
									if g_Entity[a]['health'] > g_PlayerHealth then TextCenterOnXColor(percentx,percenty,1,tagreadout[e],255,0,0) end
									if g_Entity[a]['health'] == g_PlayerHealth then TextCenterOnXColor(percentx,percenty,1,tagreadout[e],255,255,0) end
									if g_Entity[a]['health'] < g_PlayerHealth then TextCenterOnXColor(percentx,percenty,1,tagreadout[e],0,255,0) end
								end
							end
							if healthbar[e].display_mode == 5 and hreadout[e] > 0 then
								PasteSpritePosition(hbarsprite[e],percentx-(hbarsize[e]/2),percenty)
								if g_MouseClick == 0 then
									TextCenterOnXColor(percentx,percenty,1,tagreadout[e],255,255,255)
								end
								if g_MouseClick == 2 then
									if g_Entity[a]['health'] > g_PlayerHealth then TextCenterOnXColor(percentx,percenty,1,tagreadout[e],255,0,0) end
									if g_Entity[a]['health'] == g_PlayerHealth then TextCenterOnXColor(percentx,percenty,1,tagreadout[e],255,255,0) end
									if g_Entity[a]['health'] < g_PlayerHealth then TextCenterOnXColor(percentx,percenty,1,tagreadout[e],0,255,0) end
								end
							end
							if g_LegacyNPC == 1 and g_Entity[a]['health'] < 1000 then
								g_LegacyNPC = 0
							end
						end
					end
				end
			end
			--Destroy Dead Entities check--
			for _,a in pairs (_G[tableName[e]]) do
				if g_Entity[a] ~= nil then
					if g_Entity[a]['health'] <= 0 then
						SetEntityHealth(a,0)
						table.remove(_G[tableName[e]], tableFind(_G[tableName[e]],a))
					end
				end
			end
			checktimer[e] = g_Time + 0.5
		end
	end
end

function tableFind(tbl, value)
    for key, val in pairs(tbl) do
        if val == value then
            return key
        end
    end
    return nil
end

function GetFlatDistanceToPlayer(v)
	if g_Entity[v] ~= nil then
		local distDX = g_PlayerPosX - g_Entity[v]['x']
		local distDZ = g_PlayerPosZ - g_Entity[v]['z']
		return math.sqrt((distDX*distDX)+(distDZ*distDZ));
	end
end