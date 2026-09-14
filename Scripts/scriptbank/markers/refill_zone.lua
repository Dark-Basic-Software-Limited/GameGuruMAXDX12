-- Copyright(c) 2025 GameGuruMAX
-- Refill Zone v4 by Necrym59
-- DESCRIPTION: Refills the carried designated weapon/tool ammunition and/or user_global when they enter the zone.
-- DESCRIPTION: [PROMPT_TEXT$="E to refill"]
-- DESCRIPTION: [@@WEAPON_NAME$=""(0=AnyWeaponList)] Carried Weapon/Tool - ("No Weapon"=Update Global Only)
-- DESCRIPTION: [AMOUNT=1(1,50)] to refill
-- DESCRIPTION: [ZONE_HEIGHT=100(1,500)] activation height
-- DESCRIPTION: [SPAWN_AT_START!=1] if unchecked use a switch or other trigger to spawn this zone
-- DESCRIPTION: [RESPAWN_DELAY=0(0,60)] seconds to respawn after use (0 = destroyed after use)
-- DESCRIPTION: [@@USER_GLOBAL_AFFECTED$=""(0=globallist)] eg; MyGlobal
-- DESCRIPTION: <Sound0> for when refilled

local refill 				= {}
local prompt_text			= {}
local weapon_name 			= {}
local amount 				= {}
local zone_height			= {}
local spawn_at_start		= {}
local respawn_delay			= {}
local user_global_affected	= {}

local currentvalue	= {}
local delaytime		= {}
local status		= {}

function refill_zone_properties(e, prompt_text, weapon_name, amount, zone_height, spawn_at_start, respawn_delay, user_global_affected)
	refill[e].prompt_text = prompt_text
	refill[e].weapon_name = weapon_name
	refill[e].amount = amount
	refill[e].zone_height = zone_height or 100
	refill[e].spawn_at_start = spawn_at_start or 1
	refill[e].respawn_delay = respawn_delay or 0
	refill[e].user_global_affected = user_global_affected
end

function refill_zone_init(e)
	refill[e] = {}
	refill[e].prompt_text = "E to refill"
	refill[e].weapon_name = ""
	refill[e].amount = 1
	refill[e].zone_height = 100
	refill[e].spawn_at_start = 1
	refill[e].respawn_delay = 0
	refill[e].user_global_affected = ""

	currentvalue[e] = 0
	delaytime[e] = math.huge
	SetEntityAlwaysActive(e,1)
	status[e] = "init"
end

function refill_zone_main(e)

	if status[e] == "init" then
		if refill[e].weapon_name == "" then refill[e].weapon_name = "No Weapon" end
		if refill[e].amount > 50 then refill[e].amount = 50 end
		if refill[e].spawn_at_start == 1 then SetActivated(e,1) end
		if refill[e].spawn_at_start == 0 then SetActivated(e,0) end
		status[e] = "endinit"
	end
	if g_Entity[e].activated == 1 then	
		if g_Entity[e].plrinzone == 1 and g_PlayerPosY > g_Entity[e].y and g_PlayerPosY < g_Entity[e].y + refill[e].zone_height then
			if refill[e].weapon_name ~= "" then				
				WeaponID = GetPlayerWeaponID()
				local WeaponNM = GetWeaponName(WeaponID)
				Prompt(refill[e].prompt_text)
				if g_KeyPressE == 1 then
					for index = 1, 10, 1 do														
						GetWeaponSlot(index)
						local poolindex = GetWeaponPoolAmmoIndex(index)
						local amqty = GetWeaponPoolAmmo(poolindex)
						if WeaponNM == refill[e].weapon_name then
							PlayNon3DSound(e,0)
							SetWeaponPoolAmmo(poolindex,amqty + refill[e].amount)
							SetGamePlayerStateFiringMode(2)
							if refill[e].user_global_affected ~= "" then
								if _G["g_UserGlobal['"..refill[e].user_global_affected.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..refill[e].user_global_affected.."']"] end
								_G["g_UserGlobal['"..refill[e].user_global_affected.."']"] = currentvalue[e] + refill[e].amount
							end	
						end
					end
					if refill[e].respawn_delay == 0 then Destroy(e) end
					if refill[e].respawn_delay > 0 then
						delaytime[e] = g_Time + (refill[e].respawn_delay*1000)
						SetActivated(e,0)					
					end
				end		
			end
			if refill[e].weapon_name == "No Weapon" or refill[e].weapon_name == "" then
				Prompt(refill[e].prompt_text)
				if g_KeyPressE == 1 then
					PlayNon3DSound(e,0)
					if refill[e].user_global_affected ~= "" then
						if _G["g_UserGlobal['"..refill[e].user_global_affected.."']"] ~= nil then currentvalue[e] = _G["g_UserGlobal['"..refill[e].user_global_affected.."']"] end
						_G["g_UserGlobal['"..refill[e].user_global_affected.."']"] = currentvalue[e] + refill[e].amount
					end
					if refill[e].respawn_delay == 0 then Destroy(e) end
					if refill[e].respawn_delay > 0 then
						delaytime[e] = g_Time + (refill[e].respawn_delay*1000)
						SetActivated(e,0)					
					end
				end		
			end
		end
	end
	if g_Time > delaytime[e] and g_Entity[e].plrinzone == 1 then
		delaytime[e] = g_Time + (refill[e].respawn_delay*1000)
	end	
	if g_Time > delaytime[e] then
		SetActivated(e,1)
		delaytime[e] = math.huge
	end
end