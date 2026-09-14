-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Freefall Object v4 - by Necrym59
-- DESCRIPTION: A object that will damage a player or npc upon freefall contact. Always Active=ON Physics=ON, Gravity=ON
-- DESCRIPTION: [HIT_DAMAGE=10(1,500)]
-- DESCRIPTION: [HIT_RADIUS=50(1,500)]
-- DESCRIPTION: <Sound0> Hit sound

local P = require "scriptbank\\physlib"
local U = require "scriptbank\\utillib"

local freefall_object 	= {}
local hit_damage		= {}
local hit_radius 		= {}

local status 			= {}
local plr_hit 			= {}
local checktimer		= {}
local eventtimer		= {}
local event				= {}
local starty			= {}
local surface			= {}
local surfacecheck		= {}

function freefall_object_properties(e, hit_damage, hit_radius)
	freefall_object[e] = g_Entity[e]
	freefall_object[e].hit_damage	= hit_damage
	freefall_object[e].hit_radius = hit_radius
end

function freefall_object_init(e)
	freefall_object[e] = g_Entity[e]
	freefall_object[e].hit_damage = 10
	freefall_object[e].hit_radius = 50

	status[e] = "init"
	plr_hit[e] = 0
	starty[e] = 0
	event[e] = 0
	surface[e] = 0
	surfacecheck[e] = 0
	checktimer[e] = math.huge
	eventtimer[e] = math.huge
	GravityOn(e)
	CollisionOn(e)
end

function freefall_object_main(e)

	if status[e] == "init" then
		starty[e] = g_Entity[e]['y']		
		status[e] = "start_event"
	end

	if g_Entity[e]['y'] < starty[e]-2 then SetActivated(e,1) end
	
	if g_Entity[e]['activated'] == 1 then
		if status[e] == "start_event" then
			PushObject(g_Entity[e]['obj'],math.random(0,1),math.random(0,10),math.random(0,1),math.random(10,45),math.random(20,45),math.random(10,45))
			checktimer[e] = g_Time + 10
			eventtimer[e] = g_Time + 15000
			event[e] = 1
			status[e] = "freefall_event"
		end
		if status[e] == "freefall_event" then
			if GetPlayerDistance(e) < freefall_object[e].hit_radius and plr_hit[e] == 0 then
				PlaySound(e,0)
				HurtPlayer(-1,freefall_object[e].hit_damage)
				SetEntityHealth(e,g_Entity[e]['health']-freefall_object[e].hit_damage)
				ForcePlayer(0,3)
				plr_hit[e] = 1
			end
			if g_Time > checktimer[e] then
				for _, v in pairs(U.ClosestEntities(freefall_object[e].hit_radius,math.huge,g_Entity[e]['x'],g_Entity[e]['z'])) do
					if GetEntityAllegiance(v) > -1 then
						if g_Entity[v]['health'] > 0 then
							PlaySound(e,0)
							SetEntityHealth(v,g_Entity[v]['health']-freefall_object[e].hit_damage)
							SetEntityHealth(e,g_Entity[e]['health']-freefall_object[e].hit_damage)
						end
						checktimer[e] = g_Time + 1
					end
				end
			end
			surface[e] = GetSurfaceHeight(g_Entity[e]['x'],g_Entity[e]['y'],g_Entity[e]['z'])
			surfacecheck[e] = (g_Entity[e]['y'] - surface[e])
			if g_Entity[e]['y'] == surfacecheck[e] then
				SetEntityHealth(e,g_Entity[e]['health']-freefall_object[e].hit_damage)
				plr_hit[e] = 1
			end
		end
		if g_Time > eventtimer[e] and event[e] == 1 then
			SetActivated(e,0)			
			SwitchScript(e,"no_behavior_selected.lua")
		end
	end
end