-- Examine v11 by Necrym59
-- DESCRIPTION: Allows to examine an object. Must be collectable entity to collect.
-- DESCRIPTION: [PICKUP_RANGE=90(0,100)]
-- DESCRIPTION: [PICKUP_MESSAGE$="E to Examine object"]
-- DESCRIPTION: [EXAMINE_MESSAGE$="WASD or MB1+Move, MMW=Up/Dn, E=Collect, Q=Exit"]
-- DESCRIPTION: [EXAMINE_SPEED=50]
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [@ITEM_HIGHLIGHT=0(0=None,1=Shape,2=Outline,3=Icon)]
-- DESCRIPTION: [HIGHLIGHT_ICON_IMAGEFILE$="imagebank\\icons\\pickup.png"]
-- DESCRIPTION: [@PICKUP_TRIGGER=1(1=Off,2=On)]
-- DESCRIPTION: [@CAN_COLLECT=1(1=No,2=Yes)]
-- DESCRIPTION: [COLLECT_MESSAGE$="Item Collected"]
-- DESCRIPTION: <Sound0> for pickup sound
-- DESCRIPTION: <Sound1> for collection sound

local module_misclib = require "scriptbank\\module_misclib"
local U = require "scriptbank\\utillib"
local Q = require "scriptbank\\quatlib"
g_tEnt = {}

local rad = math.rad
local deg = math.deg
local abs = math.abs

local examine 				= {}
local pickup_range			= {}
local pickup_style			= {}
local pickup_message		= {}
local examine_message		= {}
local examine_speed			= {}
local prompt_display 		= {}
local item_highlight 		= {}
local highlight_icon 		= {}
local pickup_trigger		= {}
local can_collect			= {}
local collect_message		= {}

local exminetime		= {}
local status 			= {}
local tEnt 				= {}
local selectobj 		= {}
local startposx 		= {}
local startposy 		= {}
local startposz 		= {}
local startangx 		= {}
local startangy 		= {}
local startangz 		= {}
local new_y 			= {}
local prop_x 			= {}
local prop_y 			= {}
local prop_z 			= {}
local prop_h 			= {}
local hl_icon 			= {}
local hl_imgwidth 		= {}
local hl_imgheight 		= {}
local last_gun			= {}
local doonce			= {}
local keypause			= {}

function examine_properties(e, pickup_range, pickup_message, examine_message, examine_speed, prompt_display, item_highlight, highlight_icon_imagefile, pickup_trigger, can_collect, collect_message)
	examine[e].pickup_range = pickup_range
	examine[e].pickup_message =  pickup_message
	examine[e].examine_message = examine_message
	examine[e].examine_speed = examine_speed
	examine[e].prompt_display = prompt_display
	examine[e].item_highlight = item_highlight
	examine[e].highlight_icon = highlight_icon_imagefile
	examine[e].pickup_trigger = pickup_trigger or 1	
	examine[e].can_collect = can_collect or 1
	examine[e].collect_message = collect_message
end

function examine_init(e)
	examine[e] = {}
	examine[e].pickup_range = 100
	examine[e].pickup_message = "E to Examine object"
	examine[e].examine_message = "Hmmm..."
	examine[e].examine_speed = 50
	examine[e].prompt_display = 1
	examine[e].item_highlight = 0
	examine[e].highlight_icon = "imagebank\\icons\\pickup.png"
	examine[e].pickup_trigger = 1
	examine[e].can_collect = 1	
	examine[e].collect_message ="Item Collected"
	status[e] = "init"
	exminetime[e] = 0
	g_tEnt = 0
	tEnt[e] = 0
	selectobj[e] = 0
	hl_icon[e] = 0
	hl_imgwidth[e] = 0
	hl_imgheight[e] = 0
	doonce[e] = 0
	keypause[e] = math.huge
	last_gun[e] = g_PlayerGunName
end

function examine_main(e)

	local PlayerDist = GetPlayerDistance(e)
	if status[e] == "init" then
		if examine[e].item_highlight == 3 and examine[e].highlight_icon ~= "" then
			hl_icon[e] = CreateSprite(LoadImage(examine[e].highlight_icon))
			hl_imgwidth[e] = GetImageWidth(LoadImage(examine[e].highlight_icon))
			hl_imgheight[e] = GetImageHeight(LoadImage(examine[e].highlight_icon))
			SetSpriteSize(hl_icon[e],-1,-1)
			SetSpriteDepth(hl_icon[e],100)
			SetSpriteOffset(hl_icon[e],hl_imgwidth[e]/2.0, hl_imgheight[e]/2.0)
			SetSpritePosition(hl_icon[e],500,500)
		end
		local ex,ey,ez,eax,eay,eaz = GetEntityPosAng(e)
		startposx[e] = ex
		startposy[e] = ey
		startposz[e] = ez
		startangx[e] = eax
		startangy[e] = eay
		startangz[e] = eaz
		last_gun[e] = g_PlayerGunName
		keypause[e] = g_Time + 1500
		status[e] = "pick up"
	end

	if status[e] == "pick up" then
		if PlayerDist < examine[e].pickup_range then
			--pinpoint select object--
			module_misclib.pinpoint(e,examine[e].pickup_range,examine[e].item_highlight,hl_icon[e])
			tEnt[e] = g_tEnt
			--end pinpoint select object--
		end
		if PlayerDist < examine[e].pickup_range and tEnt[e] == e and GetEntityVisibility(e) == 1 then
			if examine[e].prompt_display == 1 then PromptLocal(e,examine[e].pickup_message) end
			if examine[e].prompt_display == 2 then Prompt(examine[e].pickup_message) end
			if g_KeyPressE == 1 then
				PlaySound(e,0)
				if examine[e].pickup_trigger == 2 and doonce[e] == 0 then
					ActivateIfUsed(e)
					PerformLogicConnections(e)
					doonce[e] = 1
				end
				GravityOff(e)
				CollisionOff(e)
				local xmin, ymin, zmin, xmax, ymax, zmax = GetObjectColBox(g_Entity[tEnt[e]]['obj'])
				local sx, sy, sz = GetObjectScales(g_Entity[tEnt[e]]['obj'])
				local w, h, l = (xmax - xmin) * sx, (ymax - ymin) * sy, (zmax - zmin) * sz
				prop_h[e] = h
				new_y[tEnt[e]] = math.rad(g_PlayerAngY)
				prop_x[tEnt[e]] = g_PlayerPosX + (math.sin(new_y[tEnt[e]]) * 30)
				prop_y[tEnt[e]] = g_PlayerPosY - (math.sin(math.rad(GetCameraAngleX(0)))* 30)+30
				prop_z[tEnt[e]] = g_PlayerPosZ + (math.cos(new_y[tEnt[e]]) * 30)
				PositionObject(g_Entity[tEnt[e]]['obj'],prop_x[tEnt[e]],prop_y[tEnt[e]]-h/2,prop_z[tEnt[e]])
				RotateObject(g_Entity[tEnt[e]]['obj'],0,g_Entity[tEnt[e]]['angley'],g_PlayerAngZ)
				SetCameraOverride(3)
				exminetime[e] = 0
				last_gun[e] = g_PlayerGunName
				if g_PlayerGunID > 0 then
					CurrentlyHeldWeaponID = GetPlayerWeaponID()
					SetPlayerWeapons(0)
				end
				keypause[e] = g_Time + 1500
				status[e] = "examining"
			end
		end
	end

	if status[e] == "examining" then
		ActivateMouse()
		if examine[e].prompt_display == 1 then PromptLocal(e,examine[e].examine_message) end
		if examine[e].prompt_display == 2 then Prompt(examine[e].examine_message) end
		exminetime[e] = GetElapsedTime() * 100
		PositionObject(g_Entity[tEnt[e]]['obj'],prop_x[tEnt[e]],prop_y[tEnt[e]]-prop_h[e]/2,prop_z[tEnt[e]])
		RotateObject(g_Entity[tEnt[e]]['obj'],g_Entity[tEnt[e]]['anglex'],g_Entity[tEnt[e]]['angley'],g_Entity[tEnt[e]]['anglez'])

		if g_KeyPressW == 1 then
			RotateX(e, examine[e].examine_speed * exminetime[e])
		end
		if g_KeyPressS == 1 then
			RotateX(e, -examine[e].examine_speed * exminetime[e])
		end
		if g_KeyPressA == 1 then
			RotateY(e, -examine[e].examine_speed * exminetime[e])
		end
		if g_KeyPressD == 1 then
			RotateY(e, examine[e].examine_speed * exminetime[e])
		end

		if g_MouseClick == 1 or g_MouseClick == 2 then
			omx, omy = (g_MouseX-50)/50, (g_MouseY-50)/50
			if abs( omx ) < 0.05 then omx = 0 end
			if abs( omy ) < 0.05 then omy = 0 end
			if omx < 0 then
				RotateY(e,(omx*100)*exminetime[e])
			end
			if omx > 0 then
				RotateY(e,(omx*100)*exminetime[e])
			end
			if omy < 0 then
				RotateX(e,(omy*100)*exminetime[e])
			end
			if omy > 0 then
				RotateX(e,(omy*100)*exminetime[e])
			end
		end

		if g_MouseWheel < 0 then
			prop_y[tEnt[e]] = prop_y[tEnt[e]] + 1
		elseif g_MouseWheel > 0 then
			prop_y[tEnt[e]] = prop_y[tEnt[e]] - 1
		end
		if examine[e].can_collect == 2 then -- is collectable
			if g_KeyPressE == 1 and g_Time > keypause[e] then
				if GetEntityCollectable(e) == 1 or GetEntityCollectable(e) == 2 then
					if GetEntityCollected(e) == 0 then
						CollisionOn(e)
						GravityOn(e)
						SetEntityCollected(e,1)
						PromptDuration(examine[e].collect_message,1000)
						PlaySound(e,1)						
						Hide(e)
						g_KeyPressQ = 1
					end
				end
			end
		end	
		if g_KeyPressQ == 1 then
			exminetime[e] = 0
			SetCameraOverride(0)
			DeactivateMouse()
			ResetPosition(e, startposx[e], startposy[e], startposz[e])
			ResetRotation(e, startangx[e], startangy[e], startangz[e])
			CollisionOn(e)
			GravityOn(e)			
			ChangePlayerWeapon(last_gun[e])
			SetPlayerWeapons(1)
			ChangePlayerWeaponID(CurrentlyHeldWeaponID)
			status[e] = "pick up"
		end
	end
end