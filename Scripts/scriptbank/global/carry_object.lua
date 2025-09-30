-- Carry Object V43 by Necrym59 and Lee
-- DESCRIPTION: A global behaviour for object handling.
-- DESCRIPTION: Weight: Must be between 1-99. 0=No Pickup.
-- DESCRIPTION: [PICKUP_TEXT$="E or LMB to pick-up, RMB to carry/throw"]
-- DESCRIPTION: [PICKUP_RANGE=80(1,500)]
-- DESCRIPTION: [MAX_PICKUP_WEIGHT=99(1,99)]
-- DESCRIPTION: [MAX_PICKUP_SIZE=40(1,100)]
-- DESCRIPTION: [RELEASE_TEXT$="Q or LMB to drop, MMW Up/Down, Z to Rotate"]
-- DESCRIPTION: [THROW_TEXT$="Shift to add force - Release RMB to throw"]
-- DESCRIPTION: [REARM_WEAPON!=0] Auto re-arm gun when object dropped
-- DESCRIPTION: [THROW_DAMAGE!=0] Damage entity hit with thrown object
-- DESCRIPTION: [CLONE_CHECK!=0] Check for cloned carryable objects
-- DESCRIPTION: [ITEM_OUTLINE!=0] Use carryable outline identification
-- DESCRIPTION: [USE_PICKUP_ICON!=0] Use carryable icon identification
-- DESCRIPTION: [ICON_IMAGEFILE$="imagebank\\icons\\pickup.png"]
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [DIAGNOSTICS!=0] Diagnostic information display
-- DESCRIPTION: <Sound0> when picking up object.

local U = require "scriptbank\\utillib"
local P = require "scriptbank\\physlib"
g_carrying				= {}
g_carryingweight		= {}

local carry_object		= {}
local pickup_text 		= {}
local pickup_range 		= {}
local pickup_weight		= {}
local pickup_size		= {}
local throw_text 		= {}
local release_text 		= {}
local rearm_weapon 		= {}
local throw_damage		= {}
local clone_check		= {}
local item_outline 		= {}
local use_pickup_icon	= {}
local pickup_icon		= {}
local prompt_display 	= {}
local diagnostics		= {}

local carry_mode 		= {}
local new_y 			= {}
local prop_x 			= {}
local prop_y 			= {}
local propang_y 		= {}
local prop_z 			= {}
local thrown 			= {}
local doonce 			= {}
local showonce 			= {}
local status 			= {}
local nearEnt			= {}
local allegiance		= {}
local objmass 			= {}
local objheight			= {}
local objweight			= {}
local objforce			= {}
local objwidth			= {}
local objlength			= {}
local carrydist			= {}
local nocarry			= {}
local last_gun			= {}
local kpressed			= {}
local colobj			= {}
local tEnt 				= {}
local selectobj 		= {}
local cox				= {}
local fgain				= {}
local hurtonce			= {}
local surface			= {}
local weightcheck		= {}
local objlookedat		= {}
local objectlist 		= {}
local checktimer		= {}
local throwtimer		= {}
local updatetimer		= {}
local pu_icon			= {}
local pu_imgwidth		= {}
local pu_imgheight		= {}
local emvalue			= {}
local ctrlpause			= {}

function carry_object_properties(e, pickup_text, pickup_range, max_pickup_weight, max_pickup_size, release_text, throw_text, rearm_weapon, throw_damage, clone_check, item_outline, use_pickup_icon, icon_imagefile, prompt_display, diagnostics)
	carry_object[e].pickup_text = pickup_text
	carry_object[e].pickup_range = pickup_range
	carry_object[e].pickup_weight = max_pickup_weight or 99
	carry_object[e].pickup_size = max_pickup_size
	carry_object[e].release_text = release_text
	carry_object[e].throw_text = throw_text
	carry_object[e].rearm_weapon = rearm_weapon or 0
	carry_object[e].throw_damage = throw_damage or 0
	carry_object[e].clone_check = clone_check or 0
	carry_object[e].item_outline = item_outline
	carry_object[e].use_pickup_icon	= use_pickup_icon
	carry_object[e].pickup_icon	= icon_imagefile
	carry_object[e].prompt_display = prompt_display
	carry_object[e].diagnostics = diagnostics or 0	
end

function carry_object_init(e)
	carry_object[e] = {}
	carry_object[e].pickup_text = "E or LMB to pick-up, RMB to carry/throw"
	carry_object[e].pickup_range = 50
	carry_object[e].pickup_weight = 99
	carry_object[e].pickup_size = 40
	carry_object[e].release_text = "Q or LMB to drop, MMW Up/Down, Z to Rotate"
	carry_object[e].throw_text = "Shift to add force - Release RMB to throw"
	carry_object[e].rearm_weapon = 0
	carry_object[e].throw_damage = 0
	carry_object[e].clone_check = 0
	carry_object[e].item_outline = 0
	carry_object[e].use_pickup_icon	= 0
	carry_object[e].pickup_icon	= "imagebank\\icons\\pickup.png"
	carry_object[e].prompt_display = 1
	carry_object[e].diagnostics = 0	

	carry_mode[e] = 0
	new_y[e] = 0
	prop_x[e] = 0
	prop_z[e] = 0
	propang_y[e] = 0
	doonce[e] = 0
	thrown[e] = 0
	kpressed[e] = 0
	carrydist[e] = 0
	nocarry[e] = 2
	nearEnt[e] = 0
	g_carrying = 0
	g_carryingweight = 0
	status[e] = 'init'
	tEnt[e] = 0
	cox[e] = 0
	fgain[e] = 0
	hurtonce[e] = 0
	allegiance[e] = 0
	last_gun[e] = g_PlayerGunName
	colobj[e] = 0
	surface[e] = 0
	weightcheck[e] = 0
	objlookedat[e] = 0
	checktimer[e] = math.huge
	throwtimer[e] = math.huge
	updatetimer[e] = math.huge
	ctrlpause[e] = math.huge
	pu_icon[e] = ""
	pu_imgwidth[e] = 0
	pu_imgheight[e] = 0
	emvalue[e] = 0
end

function carry_object_main(e)

	if status[e] == 'init' then
		if carry_object[e].use_pickup_icon == 1 then
			if carry_object[e].pickup_icon > "" then
				pu_icon[e] = CreateSprite(LoadImage(carry_object[e].pickup_icon))
				pu_imgwidth[e] = GetImageWidth(LoadImage(carry_object[e].pickup_icon))
				pu_imgheight[e] = GetImageHeight(LoadImage(carry_object[e].pickup_icon))
				SetSpriteSize(pu_icon[e],-1,-1)
				SetSpriteDepth(pu_icon[e],100)
				SetSpritePosition(pu_icon[e],500,500)
				SetSpriteOffset(pu_icon[e],pu_imgwidth[e]/2.0, pu_imgheight[e]/2.0)
			end
		end
		if carry_object[e].pickup_weight > 99 then carry_object[e].pickup_weight = 99 end
		if carry_object[e].pickup_size > 100 then carry_object[e].pickup_size = 100 end
		checktimer[e] = g_Time + 500
		throwtimer[e] = g_Time + 500
		updatetimer[e] = g_Time + 1000
		for n = 1, g_EntityElementMax do
			if n ~= nil and g_Entity[n] ~= nil and GetEntityAllegiance(n) == -1 then
				if GetEntityWeight(n) < 100 and g_Entity[n]['health'] > 0 then
					table.insert(objectlist,n)
				end
			end
		end
		status[e] = 'pickup'
	end

	if status[e] == 'pickup' then
	
		if carry_object[e].clone_check == 1 then
			if g_Time > updatetimer[e] then
				objectlist = {}
				for n = 1, g_EntityElementMax do
					if n ~= nil and g_Entity[n] ~= nil and GetEntityAllegiance(n) == -1 then
						if GetEntityWeight(n) < 100 and g_Entity[n]['health'] > 0 then
							table.insert(objectlist,n)
						end
					end
				end
				updatetimer[e] = g_Time + 1000
			end
		end	

		if g_Time > checktimer[e] then
			objlookedat[e] = U.ObjectPlayerLookingAt(carry_object[e].pickup_range)
			if objlookedat[e] > 0 then
				for a,b in pairs (objectlist) do
					if g_Entity[b]['obj'] == objlookedat[e] and g_Entity[b]['health'] > 0 then
						status[e] = "pickup2"
						break
					end
				end
			else
				nearEnt[e] = 0
				selectobj[e] = 0
				tEnt[e] = 0
				objlookedat[e] = 0
				checktimer[e] = g_Time + 350
			end
		end
	end

	if status[e] == 'pickup2' then
		if objlookedat[e] > 0 then
			nearEnt[e] = U.ClosestEntToPos(g_PlayerPosX, g_PlayerPosZ,carry_object[e].pickup_range)
			weightcheck[e] = GetEntityWeight(nearEnt[e])
			if nearEnt[e] ~= 0 or nearEnt[e] ~= 1 and nearEnt[e] > 1 and weightcheck[e] < 100 then
				nocarry[e] = 2
				-- pinpoint select object--
				selectobj[e] = U.ObjectPlayerLookingAt(carry_object[e].pickup_range)
				if selectobj[e] ~= 0 then
					tEnt[e] = P.ObjectToEntity(selectobj[e])
					allegiance[e] = GetEntityAllegiance(tEnt[e])
					if allegiance[e] == -1 then
						local xmin, ymin, zmin, xmax, ymax, zmax = GetObjectColBox(g_Entity[tEnt[e]]['obj'])
						local sx, sy, sz = GetObjectScales(g_Entity[tEnt[e]]['obj'])
						local w, h, l = (xmax - xmin) * sx, (ymax - ymin) * sy, (zmax - zmin) * sz
						local massmod = GetEntityWeight(tEnt[e])/100
						local weight = GetEntityWeight(tEnt[e])
						objmass[tEnt[e]] = (w*h*l)/50*massmod
						objheight[tEnt[e]] = 5
						objweight[tEnt[e]] = weight
						objforce[tEnt[e]] = math.min(weight,objmass[tEnt[e]])/1.5
						objwidth[tEnt[e]] = w
						objlength[tEnt[e]] = l
						local pd = GetPlayerDistance(tEnt[e])
						carrydist[tEnt[e]] = GetPlayerDistance(tEnt[e])
						nocarry[e] = 0
						if w > carrydist[tEnt[e]] then carrydist[tEnt[e]] = carrydist[tEnt[e]] + 15 end
						if l > carrydist[tEnt[e]] then carrydist[tEnt[e]] = carrydist[tEnt[e]] + 15 end
						if l > carry_object[e].pickup_size and w > carry_object[e].pickup_size then nocarry[e] = 1 end
						if objweight[tEnt[e]] == 0 then nocarry[e] = 1 end
					end
					if allegiance[e] == -1 then
						if objweight[tEnt[e]] <= carry_object[e].pickup_weight and nocarry[e] == 0 or nocarry[e] == nil then
							if carry_object[e].use_pickup_icon == 1 and carry_object[e].pickup_icon ~= "" then
								if carry_object[e].item_outline == 1 then SetEntityOutline(tEnt[e],1) end
								PasteSpritePosition(pu_icon[e],50,50)
								if carry_object[e].prompt_display == 1 then TextCenterOnX(50,54,1,carry_object[e].pickup_text) end
								if carry_object[e].prompt_display == 2 then Prompt(carry_object[e].pickup_text) end
							else
								if carry_object[e].item_outline == 0 then TextCenterOnXColor(50-0.01,50,3,"+",255,255,255) end
								if carry_object[e].item_outline == 1 then SetEntityOutline(tEnt[e],1) end
								if carry_object[e].prompt_display == 1 then TextCenterOnX(50,54,1,carry_object[e].pickup_text) end
								if carry_object[e].prompt_display == 2 then Prompt(carry_object[e].pickup_text) end
							end
						end
						if objweight[tEnt[e]] > carry_object[e].pickup_weight or nocarry[e] == 1 then
							tEnt[e] = 0
						end
					end
					if allegiance[e] ~= -1 then
						tEnt[e] = 0
					end
				end
				--end pinpoint select object--
				if selectobj[e] == 0 then
					tEnt[e] = 0
					objlookedat[e] = 0
					status[e] = 'pickup'
				end
				if nearEnt[e] ~= tEnt[e] then
					selectobj[e] = 0
				end
			else
				nearEnt[e] = 0
				objlookedat[e] = 0
				selectobj[e] = 0
				tEnt[e] = 0
				status[e] = 'pickup'
			end

			if tEnt[e] ~= 0 then
				if g_KeyPressE == 1 and g_carrying == 0 then
					kpressed[e] = 1
					status[e] = 'carry'
					g_carrying = 1
					last_gun[e] = g_PlayerGunName
					if g_PlayerGunID > 0 then
						CurrentlyHeldWeaponID = GetPlayerWeaponID()
						SetPlayerWeapons(0)
					end
					PlaySound(e,0)
				end
				if g_MouseClick == 1 and g_carrying == 0 then
					g_KeyPressE = 1
					kpressed[e] = 1
					status[e] = 'carry'
					g_carrying = 1
					last_gun[e] = g_PlayerGunName
					if g_PlayerGunID > 0 then
						CurrentlyHeldWeaponID = GetPlayerWeaponID()
						SetPlayerWeapons(0)
					end
					PlaySound(e,0)
					g_MouseClick = 0
					ctrlpause[e] = g_Time + 1000
				end
				if g_MouseClick == 2 and g_carrying == 0 then
					g_KeyPressE = 1
					kpressed[e] = 1
					status[e] = 'carry'
					g_carrying = 1
					last_gun[e] = g_PlayerGunName
					if g_PlayerGunID > 0 then
						CurrentlyHeldWeaponID = GetPlayerWeaponID()
						SetPlayerWeapons(0)
					end
					PlaySound(e,0)
					thrown[e] = 1
				end
			end
		end
	end

	if status[e] == 'carry' then
		g_carryingweight = GetEntityWeight(tEnt[e])
		if doonce[e] == 0 then
			new_y[tEnt[e]] = math.rad(g_PlayerAngY)
			prop_x[tEnt[e]] = g_PlayerPosX + (math.sin(new_y[tEnt[e]]) * carrydist[tEnt[e]])
			prop_y[tEnt[e]] = g_PlayerPosY - math.sin(math.rad(g_PlayerAngX))*carrydist[tEnt[e]]
			prop_z[tEnt[e]] = g_PlayerPosZ + (math.cos(new_y[tEnt[e]]) * carrydist[tEnt[e]])
			SetEntityZDepthMode(tEnt[e],2)
			doonce[e] = 1
			GravityOff(tEnt[e])
			CollisionOff(tEnt[e])
		end
		if g_MouseWheel < 0 then
			SetPlayerWeapons(0)
			objheight[tEnt[e]] = objheight[tEnt[e]] - 1
			if objheight[tEnt[e]] < -10 then objheight[tEnt[e]] = -10 end
		elseif g_MouseWheel > 0 then
			SetPlayerWeapons(0)
			objheight[tEnt[e]] = objheight[tEnt[e]] + 1
			if objheight[tEnt[e]] > 25 then objheight[tEnt[e]] = 25 end
		end
		new_y[tEnt[e]] = math.rad(g_PlayerAngY)
		prop_x[tEnt[e]] = g_PlayerPosX + (math.sin(new_y[tEnt[e]]) * carrydist[tEnt[e]])
		prop_y[tEnt[e]] = g_PlayerPosY - math.sin(math.rad(g_PlayerAngX))* carrydist[tEnt[e]]
		prop_z[tEnt[e]] = g_PlayerPosZ + (math.cos(new_y[tEnt[e]]) * carrydist[tEnt[e]])
		if g_InKey == "z" or g_InKey == "Z" then SetRotation(tEnt[e],0,g_Entity[tEnt[e]]['angley']+1,g_PlayerAngZ) end

		local px, py, pz = prop_x[tEnt[e]], prop_y[tEnt[e]], prop_z[tEnt[e]]
		local rayX, rayY, rayZ = 5,0,10
		local paX, paY, paZ = math.rad(g_PlayerAngX), math.rad(g_PlayerAngY), math.rad(g_PlayerAngZ)
		rayX, rayY, rayZ = U.Rotate3D(rayX, rayY, rayZ, paX, paY, paZ)
		colobj[tEnt[e]]=IntersectAll(px,py,pz, px+rayX, py, pz+rayZ,g_Entity[tEnt[e]]['obj']) --avoids pushing carryobj through wall!
		--colobj[tEnt[e]]=IntersectAll(g_PlayerPosX,g_PlayerPosY,g_PlayerPosZ, px+rayX, py, pz+rayZ,g_Entity[tEnt[e]]['obj'])
		if colobj[tEnt[e]] > 0 then
			ForcePlayer(g_PlayerAngY + 180,0.3)
		end
		if tEnt[e] > 0 and g_carrying == 1 then
			PositionObject(g_Entity[tEnt[e]]['obj'],prop_x[tEnt[e]],prop_y[tEnt[e]]+objheight[tEnt[e]],prop_z[tEnt[e]])
			RotateObject(g_Entity[tEnt[e]]['obj'],0,g_Entity[tEnt[e]]['angley'],g_PlayerAngZ)
		end	
		if g_MouseClick == 2 and g_carrying == 1 then
			if carry_object[e].prompt_display == 1 then TextCenterOnX(50,54,1,carry_object[e].throw_text) end
			if carry_object[e].prompt_display == 2 then Prompt(carry_object[e].throw_text) end
			if g_KeyPressSHIFT == 1 then
				fgain[e] = fgain[e] + 0.02
				if carry_object[e].prompt_display == 1 then TextCenterOnX(50,56,1,"Increasing throw force by " ..math.ceil(fgain[e])) end
				if carry_object[e].prompt_display == 2 then Prompt("Increasing throw force by " ..math.ceil(fgain[e])) end
			end
			if fgain[e] >= 20 then fgain[e] = 20 end
		else
			if carry_object[e].prompt_display == 1 then TextCenterOnX(50,54,1,carry_object[e].release_text) end
			if carry_object[e].prompt_display == 2 then Prompt(carry_object[e].release_text) end
		end

		if ( GetGamePlayerStateCamAngleX()<-35) then SetGamePlayerStateCamAngleX(-35) end
		if ( GetGamePlayerStateCamAngleX()>35) then SetGamePlayerStateCamAngleX(35) end

		if g_MouseClick == 2 and g_carrying == 1 then thrown[e] = 1 end
		if g_carrying == 1 then
			if g_KeyPressQ == 1 then kpressed[e] = 0 end
			if g_MouseClick == 1 and g_Time > ctrlpause[e] then kpressed[e] = 0 end
			if g_MouseClick == 2 then kpressed[e] = 0 end
		end
		if kpressed[e] == 0 and g_MouseClick == 0 and colobj[e] == 0 then
			SetEntityZDepthMode(tEnt[e],1)
			surface[e] = GetSurfaceHeight(g_Entity[tEnt[e]]['x'],g_Entity[tEnt[e]]['y'],g_Entity[tEnt[e]]['z'])
			if prop_y[tEnt[e]] < surface[e] then prop_y[tEnt[e]] = surface[e] end
			CollisionOff(tEnt[e])
			PositionObject(g_Entity[tEnt[e]]['obj'],prop_x[tEnt[e]],prop_y[tEnt[e]],prop_z[tEnt[e]])
			CollisionOn(tEnt[e])
			objheight[tEnt[e]] = 5
			doonce[e] = 0
			status[e] = 'pickup'
			g_carrying = 0
			g_carryingweight = 0
			checktimer[e] = g_Time + 250
			objlookedat[e] = 0
			if carry_object[e].rearm_weapon == 1 then
				ChangePlayerWeapon(last_gun[e])
				SetPlayerWeapons(1)
				ChangePlayerWeaponID(CurrentlyHeldWeaponID)
			else
				SetPlayerWeapons(1)
			end
		end
		if thrown[e] == 1 and g_MouseClick == 0 and g_KeyPressQ == 0 and GetEntityVisibility(tEnt[e]) == 1 then
			SetEntityZDepthMode(tEnt[e],1)
			local paX, paY, paZ = math.rad( g_PlayerAngX ), math.rad( g_PlayerAngY ),math.rad( g_PlayerAngZ )
			local vx, vy, vz = U.Rotate3D( 0, 0, 1, paX, paY, paZ)
			objforce[tEnt[e]] = objforce[tEnt[e]] + (fgain[e]*10)
			PushObject(g_Entity[tEnt[e]]['obj'],vx*objforce[tEnt[e]], vy*objforce[tEnt[e]], vz*objforce[tEnt[e]], math.random()/100, math.random()/100, math.random()/100 )
			thrown[e] = 2
			g_carrying = 0
			g_carryingweight = 0
			throwtimer[e] = g_Time + 500
			status[e] = 'thrown'
		end
	end

	if status[e] == 'thrown' then
		for _, v in pairs(U.ClosestEntities(80,math.huge,g_Entity[tEnt[e]]['x'],g_Entity[tEnt[e]]['z'])) do
			if GetEntityAllegiance(v) > -1 then
				if g_Entity[v]['health'] > 0 then
					SetEntityHealth(v,g_Entity[v]['health']-(objforce[tEnt[e]]))
					if carry_object[e].throw_damage == 1 then
						SetEntityHealth(tEnt[e],g_Entity[tEnt[e]]['health']-(objforce[tEnt[e]]))
					else
						SetEntityHealth(tEnt[e],g_Entity[tEnt[e]]['health']-0)
					end
					throwtimer[e] = g_Time + 500
					fgain[e] = 0
					hurtonce[e] = 1
					nearEnt[e] = 0
					selectobj[e] = 0
					tEnt[e] = 0
					status[e] = 'pickup'
					checktimer[e] = g_Time + 250
					objlookedat[e] = 0
				end
			end
			if g_Time > throwtimer[e] and GetEntityAllegiance(v) < 0 then
				if g_Entity[v]['health'] > 0 then
					if carry_object[e].throw_damage == 1 then
						SetEntityHealth(tEnt[e],g_Entity[tEnt[e]]['health']-(objforce[tEnt[e]]))
					else
						SetEntityHealth(tEnt[e],g_Entity[tEnt[e]]['health']-0)
					end
					throwtimer[e] = g_Time + 500
					fgain[e] = 0
					hurtonce[e] = 0
					nearEnt[e] = 0
					selectobj[e] = 0
					tEnt[e] = 0
					status[e] = 'pickup'
					checktimer[e] = g_Time + 250
					objlookedat[e] = 0
				end
			end
		end
	end

	if carry_object[e].diagnostics == 1 then
		if tEnt[e] > 0 then
			Text(10,54,1,"Entity #: " ..tEnt[e])
			Text(10,56,1,"Obj Weight: " ..objweight[tEnt[e]])
			Text(10,58,1,"Obj Width: " ..objwidth[tEnt[e]])
			Text(10,60,1,"Obj Lngth: " ..objlength[tEnt[e]])
			Text(10,62,1,"Obj Mass: " ..objmass[tEnt[e]])
			Text(10,64,1,"Obj Health: " ..g_Entity[tEnt[e]]['health'])
			if nocarry[e] == 0 then	Text(10,66,1,"Carryable: Yes") end
			if nocarry[e] == 1 then	Text(10,66,1,"Carryable: No") end
		end
	end
end
