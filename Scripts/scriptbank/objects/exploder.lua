-- Exploder v6 by Necrym59
-- DESCRIPTION: The attached object will destroy itself when player is in proximity range or externally triggered.
-- DESCRIPTION: Set this object to Explodable and set the Explosion Damage Amount.
-- DESCRIPTION: [@ACTIVATION_TYPE=1(1=Proximity Ranged, 2=Externally Triggered)]
-- DESCRIPTION: [PROXIMITY_RANGE=500(1,2000)]
-- DESCRIPTION: [PROXIMITY_DAMAGE=50(0,500)]
-- DESCRIPTION: [#EXPLOSION_DELAY=0.0(0.0,30.0)]
-- DESCRIPTION: [@EXPLOSION_BUILDUP=1(1=Emissive Solid,2=Emissive Ramped,3=Emissive Pulsed,4=Emissive Flicker)]
-- DESCRIPTION: [@EXPLOSION_SHAKE=2(1=On,2=Off)]
-- DESCRIPTION: <Sound0> activation sound
-- DESCRIPTION: <Sound1> buildup sound loop
-- DESCRIPTION: <Sound2> exploding sound

local U = require "scriptbank\\utillib"
local lower = string.lower
local exploder 			= {}
local activation_type	= {}
local proximity_range 			= {}
local proximity_damage	= {}
local explosion_delay	= {}
local explosion_buildup	= {}
local explosion_shake	= {}

local closestent		= {}
local doonce			= {}
local tminus0			= {}
local countdown			= {}
local currentlvl		= {}
local emeffect			= {}
local empulse			= {}
local status			= {}

function exploder_properties(e, activation_type, proximity_range, proximity_damage, explosion_delay, explosion_buildup, explosion_shake)
	exploder[e].activation_type = activation_type
	exploder[e].proximity_range = proximity_range
	exploder[e].proximity_damage = proximity_damage
	exploder[e].explosion_delay = explosion_delay
	exploder[e].explosion_buildup = explosion_buildup
	exploder[e].explosion_shake	= explosion_shake
	exploder[e].in_vehicle = in_vehicle or 0
end

function exploder_init_name(e)
	exploder[e] = {}
	exploder[e].activation_type = 1
	exploder[e].proximity_range = 500
	exploder[e].proximity_damage = 50
	exploder[e].explosion_delay = 0
	exploder[e].explosion_buildup = 1
	exploder[e].explosion_shake	= 2

	tminus0[e] = math.huge
	countdown[e] = 0
	currentlvl[e] = 0
	emeffect[e] = 0
	empulse[e] = 0
	SetEntityAlwaysActive(e,1)
	SetEntityEmissiveStrength(e,0)
	status[e] = "init"
end

function exploder_main(e)

	if status[e] == "init" then
		if exploder[e].activation_type == 1 then SetActivated(e,1) end
		if exploder[e].activation_type == 2 then SetActivated(e,0) end
		status[e] = "detect"
	end

	local PlayerDist = GetPlayerDistance(e)

	if g_Entity[e]['activated'] == 1 then

		if status[e] == "detect" then
			if exploder[e].activation_type == 1 then
				if PlayerDist < exploder[e].proximity_range then
					tminus0[e] = g_Time + (exploder[e].explosion_delay*1000)
					if exploder[e].explosion_buildup == 1 then emeffect[e] = 3000 end
					if exploder[e].explosion_buildup == 2 then emeffect[e] = 10/exploder[e].explosion_delay end
					if exploder[e].explosion_buildup == 3 then emeffect[e] = 10/exploder[e].explosion_delay end
					if exploder[e].explosion_buildup == 4 then emeffect[e] = 10/exploder[e].explosion_delay end
					status[e] = "explode"
					PlaySound(e,0)
					SetActivated(e,1)
				end
			end
			if exploder[e].activation_type == 2 then
				tminus0[e] = g_Time + (exploder[e].explosion_delay*1000)
				if exploder[e].explosion_buildup == 1 then emeffect[e] = 3000 end
				if exploder[e].explosion_buildup == 2 then emeffect[e] = 10/exploder[e].explosion_delay end
				if exploder[e].explosion_buildup == 3 then emeffect[e] = 10/exploder[e].explosion_delay end
				if exploder[e].explosion_buildup == 4 then emeffect[e] = 10/exploder[e].explosion_delay end				
				status[e] = "explode"
				PlaySound(e,0)
			end
		end

		if status[e] == "explode" then
			if g_Time < tminus0[e] then
				LoopSound(e,1)
				countdown[e] = tminus0[e] - g_Time

				if exploder[e].explosion_buildup == 1 then
					SetEntityEmissiveStrength(e,emeffect[e])
				end
				if exploder[e].explosion_buildup == 2 then
					SetEntityEmissiveStrength(e,currentlvl[e])
					currentlvl[e] = currentlvl[e] + emeffect[e]
				end
				if exploder[e].explosion_buildup == 3 then
					empulse[e] = (math.sin(g_Time * 0.01) + (emeffect[e]/2)) / 2
					SetEntityEmissiveStrength(e,currentlvl[e] * empulse[e])
					currentlvl[e] = currentlvl[e] + emeffect[e]
					emeffect[e] = emeffect[e]+exploder[e].explosion_delay/10000
				end
				if exploder[e].explosion_buildup == 4 then
					if g_Time % 5 == 0 then
						SetEntityEmissiveStrength(e,currentlvl[e])
						SetEntityEmissiveStrength(e,math.random(0,currentlvl[e]*10))
						currentlvl[e] = currentlvl[e] + emeffect[e]
					end
				end
			end

			if g_Time > tminus0[e] then
				SetEntityHealth(e,0)
				StopSound(e,1)
				PlaySound(e,2)
				if exploder[e].explosion_shake == 1 and GamePlayerControlAddShakeTrauma ~= nil then
					GamePlayerControlAddShakeTrauma(65.0)
					GamePlayerControlAddShakePeriod(30.0)
					GamePlayerControlAddShakeFade(2.0)
					GamePlayerControlSetShakeTrauma(0.0)
					GamePlayerControlSetShakePeriod(0.0)
				end
				if PlayerDist < exploder[e].proximity_range then HurtPlayer(-1,exploder[e].proximity_damage) end
				closestent[e] = U.ClosestEntToPlayer(90)
				SetEntityHealth(closestent[e],g_Entity[closestent[e]]['health']-exploder[e].proximity_damage)
				Hide(e)
				CollisionOff(e)
				Destroy(e)
			end
		end
	end
end
