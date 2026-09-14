-- Navblockerer v5 by Necrym59
-- DESCRIPTION: A global behavior that make a navmesh blocker for any named object(s) in-game.
-- DESCRIPTION: Any object(s) to be nav-blocked must be set dynamic eg: Physics ON, IsImobile ON
-- DESCRIPTION: [OBJECT_NAME$=""]
-- DESCRIPTION: [HIDE_OBJECT!=0]

local lower = string.lower
local navblocker 		= {}
local object_name 		= {}
local hide_object		= {}

local status			= {}
local objEnt			= {}

function navblocker_properties(e, object_name, hide_object, collision_mode, navmesh_block)
	navblocker[e] = g_Entity[e]
	navblocker[e].object_name = lower(object_name)
	navblocker[e].hide_object = hide_object or 0
end 

function navblocker_init_name(e)
	navblocker[e] = {}
	navblocker[e].object_name = ""
	navblocker[e].hide_object = 0
	status[e] = "init"
end

function navblocker_main(e)
	
	if status[e] == "init" then
		for n = 1, g_EntityElementMax do
			if n ~= nil and g_Entity[n] ~= nil then
				if lower(GetEntityName(n)) == navblocker[e].object_name then					
					CollisionOn(n)
					local x,y,z = GetEntityPosAng(n)
					y = RDGetYFromMeshPosition(x,y,z)
					local xmin, ymin, zmin, xmax, ymax, zmax = GetObjectColBox(g_Entity[n]['obj'])
					local sx, sy, sz = GetObjectScales(g_Entity[n]['obj'])
					local w, h, l = (xmax - xmin) * sx, (ymax - ymin) * sy, (zmax - zmin) * sz
					local sizex = w
					local sizez = l
					local angle = GetEntityAngleY(n)
					local blockmode = 1					
					RDBlockNavMeshWithShape(x,y,z,w,1,l,angle)
					if navblocker[e].hide_object == 1 then Hide(n) end
				end
			end
		end
		status[e] = "endinit"
	end
end