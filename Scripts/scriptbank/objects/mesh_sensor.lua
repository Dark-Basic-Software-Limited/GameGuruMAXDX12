-- Mesh Sensor v5 by Necrym59
-- DESCRIPTION: Allows to sense meshes within a multi-mesh object and perform the selected interaction with them
-- DESCRIPTION: [OBJECT_NAME$=""]
-- DESCRIPTION: [OBJECT_RANGE=100(1,500)]
-- DESCRIPTION: [SENSOR_RANGE=80(1,250)]
-- DESCRIPTION: [RAYCAST_X=50(0,100)]
-- DESCRIPTION: [RAYCAST_Y=50(0,100)]
-- DESCRIPTION: [MESH_NAME1$=""]
-- DESCRIPTION: [MESH_TEXT1$="E to use"]
-- DESCRIPTION: [@MESH_ACTION1=1(1=None, 2=Activate IfUsed, 3=Display Hud)]
-- DESCRIPTION: [MESH_IFUSED1$=""]
-- DESCRIPTION: [@@MESH_HUD1$=""(0=hudscreenlist)]
-- DESCRIPTION: [MESH_NAME2$=""]
-- DESCRIPTION: [MESH_TEXT2$="E to use"]
-- DESCRIPTION: [@MESH_ACTION2=1(1=None, 2=Activate IfUsed, 3=Display Hud))]
-- DESCRIPTION: [MESH_IFUSED2$=""]
-- DESCRIPTION: [@@MESH_HUD2$=""(0=hudscreenlist)]
-- DESCRIPTION: [MESH_NAME3$=""]
-- DESCRIPTION: [MESH_TEXT3$="E to use"]
-- DESCRIPTION: [@MESH_ACTION3=1(1=None, 2=Activate IfUsed, 3=Display Hud))]
-- DESCRIPTION: [MESH_IFUSED3$=""]
-- DESCRIPTION: [@@MESH_HUD3$=""(0=hudscreenlist)]
-- DESCRIPTION: [MESH_NAME4$=""]
-- DESCRIPTION: [MESH_TEXT4$="E to use"]
-- DESCRIPTION: [@MESH_ACTION4=1(1=None, 2=Activate IfUsed, 3=Display Hud))]
-- DESCRIPTION: [MESH_IFUSED4$=""]
-- DESCRIPTION: [@@MESH_HUD4$=""(0=hudscreenlist)]
-- DESCRIPTION: [MESH_NAME5$=""]
-- DESCRIPTION: [MESH_TEXT5$="E to use"]
-- DESCRIPTION: [@MESH_ACTION5=1(1=None, 2=Activate IfUsed, 3=Display Hud))]
-- DESCRIPTION: [MESH_IFUSED5$=""]
-- DESCRIPTION: [@@MESH_HUD5$=""(0=hudscreenlist)]
-- DESCRIPTION: [DIAGNOSTIC!=0] If on will show sensed mesh names

local lower = string.lower
local mesh_sensor 		= {}
local object_name 		= {}
local object_range 		= {}
local sensor_range		= {}
local raycast_x			= {}
local raycast_y			= {}
local mesh_name1		= {}
local mesh_text1 		= {}
local mesh_action1		= {}
local mesh_ifused1		= {}
local mesh_hud1			= {}
local mesh_name2		= {}
local mesh_text2 		= {}
local mesh_action2		= {}
local mesh_ifused2		= {}
local mesh_hud2			= {}
local mesh_name3		= {}
local mesh_text3 		= {}
local mesh_action3		= {}
local mesh_ifused3		= {}
local mesh_hud3			= {}
local mesh_name4		= {}
local mesh_text4 		= {}
local mesh_action4		= {}
local mesh_ifused4		= {}
local mesh_hud4			= {}
local mesh_name5		= {}
local mesh_text5 		= {}
local mesh_action5		= {}
local mesh_ifused5		= {}
local mesh_hud5			= {}
local diagnostic		= {}

local status			= {}
local keypressed		= {}
local raydist			= {}
local meshhit			= {}
local meshname			= {}
local object_no 		= {}


function mesh_sensor_properties(e, object_name, object_range, sensor_range, raycast_x, raycast_y, mesh_name1, mesh_text1, mesh_action1, mesh_ifused1,  mesh_hud1, mesh_name2, mesh_text2, mesh_action2, mesh_ifused2, mesh_hud2, mesh_name3, mesh_text3, mesh_action3, mesh_ifused3, mesh_hud3, mesh_name4, mesh_text4, mesh_action4, mesh_ifused4, mesh_hud4, mesh_name5, mesh_text5, mesh_action5, mesh_ifused5, mesh_hud5, diagnostic)
	mesh_sensor[e].object_name = string.lower(object_name)
	mesh_sensor[e].object_range = object_range
	mesh_sensor[e].sensor_range = sensor_range
	mesh_sensor[e].raycast_x = raycast_x
	mesh_sensor[e].raycast_y = raycast_y	
	mesh_sensor[e].mesh_name1 = mesh_name1	
	mesh_sensor[e].mesh_text1 = mesh_text1
	mesh_sensor[e].mesh_action1 = mesh_action1 or 1
	mesh_sensor[e].mesh_ifused1	= mesh_ifused1
	mesh_sensor[e].mesh_hud1 = mesh_hud1
	mesh_sensor[e].mesh_name2 = mesh_name2	
	mesh_sensor[e].mesh_text2 = mesh_text2
	mesh_sensor[e].mesh_action2 = mesh_action2 or 1
	mesh_sensor[e].mesh_ifused2	= mesh_ifused2	
	mesh_sensor[e].mesh_hud2 = mesh_hud2	
	mesh_sensor[e].mesh_name3 = mesh_name3	
	mesh_sensor[e].mesh_text3 = mesh_text3
	mesh_sensor[e].mesh_action3 = mesh_action3 or 1
	mesh_sensor[e].mesh_ifused3	= mesh_ifused3	
	mesh_sensor[e].mesh_hud3 = mesh_hud3
	mesh_sensor[e].mesh_name4 = mesh_name4	
	mesh_sensor[e].mesh_text4 = mesh_text4
	mesh_sensor[e].mesh_action4 = mesh_action4 or 1
	mesh_sensor[e].mesh_ifused4	= mesh_ifused4	
	mesh_sensor[e].mesh_hud4 = mesh_hud4
	mesh_sensor[e].mesh_name5 = mesh_name5	
	mesh_sensor[e].mesh_text5 = mesh_text5
	mesh_sensor[e].mesh_action5 = mesh_action5 or 1	
	mesh_sensor[e].mesh_ifused5	= mesh_ifused5	
	mesh_sensor[e].mesh_hud5 = mesh_hud5
	mesh_sensor[e].diagnostic = diagnostic	
	mesh_sensor[e].object_no = 0
end

function mesh_sensor_init(e)
	mesh_sensor[e] = {}
	mesh_sensor[e].object_name = ""
	mesh_sensor[e].object_range = 100
	mesh_sensor[e].sensor_range = 80
	mesh_sensor[e].raycast_x = 50
	mesh_sensor[e].raycast_y = 50
	mesh_sensor[e].mesh_name1 = ""
	mesh_sensor[e].mesh_text1 = "E to use"
	mesh_sensor[e].mesh_action1 = mesh_action1 or 1
	mesh_sensor[e].mesh_ifused1	= ""	
	mesh_sensor[e].mesh_hud1 = ""	
	mesh_sensor[e].mesh_name2 = ""	
	mesh_sensor[e].mesh_text2 = "E to use"
	mesh_sensor[e].mesh_action2 = mesh_action2 or 1	
	mesh_sensor[e].mesh_ifused2	= ""	
	mesh_sensor[e].mesh_hud2 = ""	
	mesh_sensor[e].mesh_name3 = ""	
	mesh_sensor[e].mesh_text3 = "E to use"
	mesh_sensor[e].mesh_action3 = mesh_action3 or 1
	mesh_sensor[e].mesh_ifused3	= ""	
	mesh_sensor[e].mesh_hud3 = ""
	mesh_sensor[e].mesh_name4 = ""	
	mesh_sensor[e].mesh_text4 = "E to use"
	mesh_sensor[e].mesh_action4 = mesh_action4 or 1
	mesh_sensor[e].mesh_ifused4	= ""	
	mesh_sensor[e].mesh_hud4 = ""
	mesh_sensor[e].mesh_name5 = ""	
	mesh_sensor[e].mesh_text5 = "E to use"
	mesh_sensor[e].mesh_action5 = mesh_action5 or 1
	mesh_sensor[e].mesh_ifused5	= ""	
	mesh_sensor[e].mesh_hud5 = ""
	mesh_sensor[e].diagnostic = 0	
	mesh_sensor[e].object_no = 0	

	raydist[e] = 0
	meshhit[e] = 0
	meshname[e] = ""
	keypressed[e] = 0
	SetEntityAlwaysActive(e,1)
	status[e] = "init"	
end

function mesh_sensor_main(e)
	if status[e] == "init" then
		--Find named object with multi meshes--
		if mesh_sensor[e].object_name ~= "" and mesh_sensor[e].object_no == 0 then
			for p = 1, g_EntityElementMax do
				if p ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == mesh_sensor[e].object_name then					
						mesh_sensor[e].object_no = p	
						break
					end
				end
			end
		else
			return
		end	
		status[e] = "endinit"
	end
	
	local PlayerDist = GetPlayerDistance(mesh_sensor[e].object_no)
	
	if PlayerDist < mesh_sensor[e].object_range then
		if meshhit[e] == 0 then TextCenterOnXColor(50,50-0.4,3,".",180,180,180) end  -- Dot pointer		
		px,py,pz,dx,dy,dz = Convert2DTo3D(mesh_sensor[e].raycast_x,mesh_sensor[e].raycast_y)
		raydist[e] = mesh_sensor[e].sensor_range
		meshhit[e] = IntersectAll(px,py,pz,px+(dx*raydist[e]),py+(dy*raydist[e]),pz+(dz*raydist[e]),0)
		if meshhit[e] > 0 then
			if mesh_sensor[e].diagnostic == 1 then TextCenterOnX(50,60,3,"Mesh: " ..meshname[e]) end
			meshname[e] = IntersectGetLastHitFrame(meshhit[e])
			if meshname[e] == mesh_sensor[e].mesh_name1 then
				if keypressed[e] == 0 then TextCenterOnX(50,50,3,mesh_sensor[e].mesh_text1) end
				if g_KeyPressE == 1 then
					keypressed[e] = 1
					if mesh_sensor[e].mesh_action1 == 1 then end
					if mesh_sensor[e].mesh_action1 == 2 then
						SetEntityIfUsed(e,mesh_sensor[e].mesh_ifused1)
						ActivateIfUsed(e)
					end
					if mesh_sensor[e].mesh_action1 == 3 then ScreenToggle(mesh_sensor[e].mesh_hud1) end
				end
			end
			if meshname[e] == mesh_sensor[e].mesh_name2 then
				if keypressed[e] == 0 then TextCenterOnX(50,50,3,mesh_sensor[e].mesh_text2) end
				if g_KeyPressE == 1 then
					keypressed[e] = 1
					if mesh_sensor[e].mesh_action2 == 1 then end
					if mesh_sensor[e].mesh_action2 == 2 then
						SetEntityIfUsed(e,mesh_sensor[e].mesh_ifused2)
						ActivateIfUsed(e)
					end
					if mesh_sensor[e].mesh_action2 == 3 then ScreenToggle(mesh_sensor[e].mesh_hud2) end
				end				
			end
			if meshname[e] == mesh_sensor[e].mesh_name3 then
				if keypressed[e] == 0 then TextCenterOnX(50,50,3,mesh_sensor[e].mesh_text3) end
				if g_KeyPressE == 1 then
					keypressed[e] = 1
					if mesh_sensor[e].mesh_action3 == 1 then end
					if mesh_sensor[e].mesh_action3 == 2 then
						SetEntityIfUsed(e,mesh_sensor[e].mesh_ifused3)
						ActivateIfUsed(e)
					end
					if mesh_sensor[e].mesh_action3 == 3 then ScreenToggle(mesh_sensor[e].mesh_hud3) end
				end
			end
			if meshname[e] == mesh_sensor[e].mesh_name4 then
				if keypressed[e] == 0 then TextCenterOnX(50,50,3,mesh_sensor[e].mesh_text4) end
				if g_KeyPressE == 1 then
					keypressed[e] = 1
					if mesh_sensor[e].mesh_action4 == 1 then end
					if mesh_sensor[e].mesh_action4 == 2 then
						SetEntityIfUsed(e,mesh_sensor[e].mesh_ifused4)
						ActivateIfUsed(e)
					end
					if mesh_sensor[e].mesh_action4 == 3 then ScreenToggle(mesh_sensor[e].mesh_hud4) end
				end
			end
			if meshname[e] == mesh_sensor[e].mesh_name5 then
				if keypressed[e] == 0 then TextCenterOnX(50,50,3,mesh_sensor[e].mesh_text5) end
				if g_KeyPressE == 1 then
					keypressed[e] = 1
					if mesh_sensor[e].mesh_action5 == 1 then end
					if mesh_sensor[e].mesh_action5 == 2 then
						SetEntityIfUsed(e,mesh_sensor[e].mesh_ifused5)
						ActivateIfUsed(e)
					end					
					if mesh_sensor[e].mesh_action5 == 3 then ScreenToggle(mesh_sensor[e].mesh_hud5) end
				end
			end
		end
		if GetCurrentScreen() == -1 and keypressed[e] == 1 then
			keypressed[e] = 0
		end	
	end	
end