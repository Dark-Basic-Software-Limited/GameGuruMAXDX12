-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Deployable Named v5 by Necrym59
-- DESCRIPTION: This behavior allows one named object to be monitored to be collected, carried and deployed.
-- DESCRIPTION: The named object may have its own behavior attached?
-- DESCRIPTION: Apply to an object set AlwaysActive=ON
-- DESCRIPTION: (Named object being deployed must be set Physics ON, IsImobile=YES)
-- DESCRIPTION: [OBJECT_NAME$=""] (eg:Tent)
-- DESCRIPTION: [USE_RANGE=120]
-- DESCRIPTION: [PICKUP_TEXT$="Press HOME to pick up the object"]
-- DESCRIPTION: [DEPLOY_TEXT$="Press END to deploy the object"]
-- DESCRIPTION: [IS_DEPLOYED!=1] if object is deployed or carried at start
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [@ITEM_HIGHLIGHT=0(0=None,1=Shape,2=Outline,3=Icon)]
-- DESCRIPTION: [HIGHLIGHT_ICON_IMAGEFILE$="imagebank\\icons\\pickup.png"]
-- DESCRIPTION: <Sound0> for collection sound.
-- DESCRIPTION: <Sound1> for deloyment sound.

local module_misclib = require "scriptbank\\module_misclib"
local U = require "scriptbank\\utillib"
g_tEnt = {}

local lower = string.lower
local deployable			= {}
local object_name			= {}
local use_range				= {}
local pickup_text			= {}
local deploy_text			= {}
local is_deployed			= {}
local prompt_display 		= {}
local item_highlight 		= {}
local highlight_icon 		= {}

local status 				= {}
local tEnt 					= {}
local selectobj 			= {}
local deployable_no			= {}	
local deployable_deployed 	= {}
local height_difference		= {}
local deployed_x			= {}
local deployed_z			= {}
local doonce				= {}
local salpha				= {}
local pressed				= {}
local hl_icon 				= {}
local hl_imgwidth			= {}
local hl_imgheight			= {}
	 
function deployable_named_properties(e, object_name, use_range, pickup_text, deploy_text, is_deployed, prompt_display, item_highlight, highlight_icon_imagefile)
	deployable[e].object_name = string.lower(object_name)
	deployable[e].use_range = use_range
	deployable[e].pickup_text = pickup_text
	deployable[e].deploy_text = deploy_text
	deployable[e].is_deployed = is_deployed	
	deployable[e].prompt_display = prompt_display
	deployable[e].item_highlight = item_highlight
	deployable[e].highlight_icon = highlight_icon_imagefile	
end
 
 
function deployable_named_init(e)
	deployable[e] = {}
	deployable[e].object_name = ""
	deployable[e].use_range = 120
	deployable[e].pickup_text = "Press HOME to pack up the object"
	deployable[e].deploy_text = "Press END to deploy the object"
	deployable[e].is_deployed = 1		
	deployable[e].prompt_display = 1
	deployable[e].item_highlight = 1
	deployable[e].highlight_icon = "imagebank\\icons\\pickup.png"
	deployable[e].deployable_no = 0

	status[e] = "init"
	tEnt[e] = 0
	g_tEnt = 0
	height_difference[e] = 0
	deployable_deployed[e] = 0
	deployed_x[e] = 0
	deployed_z[e] = 0
	objcenter_x[e] = 0
	objcenter_z[e] = 0
	doonce[e] = 0
	pressed[e] = 0
	hl_icon[e] = 0
	hl_imgwidth[e] = 0
	hl_imgheight[e] = 0
	SetEntityAlwaysActive(e,1)
end
 
function deployable_named_main(e)

	if status[e] == "init" then
		if deployable[e].item_highlight == 3 and deployable[e].highlight_icon ~= "" then
			hl_icon[e] = CreateSprite(LoadImage(deployable[e].highlight_icon))
			hl_imgwidth[e] = GetImageWidth(LoadImage(deployable[e].highlight_icon))
			hl_imgheight[e] = GetImageHeight(LoadImage(deployable[e].highlight_icon))
			SetSpriteSize(hl_icon[e],-1,-1)
			SetSpriteDepth(hl_icon[e],100)
			SetSpriteOffset(hl_icon[e],hl_imgwidth[e]/2.0, hl_imgheight[e]/2.0)
			SetSpritePosition(hl_icon[e],500,500)
		end
		--Find Named Object --
		if deployable[e].object_name > "" then
			for p = 1, g_EntityElementMax do
				if p ~= nil and g_Entity[p] ~= nil then
					if string.lower(GetEntityName(p)) == deployable[e].object_name then					
						deployable[e].deployable_no = p	
						salpha[e] = GetEntityBaseAlpha(p)
						SetEntityBaseAlpha(p,salpha[e])
						break
					end
				end
			end
		else
			return
		end
		if deployable[e].is_deployed == 0 then deployable_deployed[e] = 0 end
		if deployable[e].is_deployed == 1 then deployable_deployed[e] = 1 end
		pressed[e] = 0
		status[e] = "endinit"
	end
	
	local PlayerDist = GetPlayerDistance(deployable[e].deployable_no)
	
	if deployable_deployed[e] == 1 then
		Show(deployable[e].deployable_no)
		if PlayerDist < deployable[e].use_range then
			--pinpoint select object--
			module_misclib.pinpoint(deployable[e].deployable_no,deployable[e].use_range,deployable[e].item_highlight,hl_icon[e])
			tEnt[e] = g_tEnt
			--end pinpoint select object--				
			if PlayerDist < deployable[e].use_range and tEnt[e] == deployable[e].deployable_no and GetEntityVisibility(e) == 1 then
				height_difference[e] = g_PlayerPosY - g_Entity[deployable[e].deployable_no]['y']
				if deployable[e].prompt_display == 1 then TextCenterOnX(50,55,1,deployable[e].pickup_text) end
				if deployable[e].prompt_display == 2 then PromptDuration(deployable[e].pickup_text,2000) end
				if g_Scancode == 199 then --Pickup (HomeKey)
					deployable_deployed[e] = 0
					SetEntityTransparency(deployable[e].deployable_no,1)
					PlaySound(e,0)
					if deployable[e].prompt_display == 1 then PromptDuration(deployable[e].deploy_text,2000) end	
					if deployable[e].prompt_display == 2 then PromptDuration(deployable[e].deploy_text,2000) end
				end
			end			
		end		
	end
	
	
	if deployable_deployed[e] == 0 then
		local new_y = math.rad(g_PlayerAngY)
		deployed_x[e] = g_PlayerPosX + (math.sin(new_y) * 100)
		deployed_z[e] = g_PlayerPosZ + (math.cos(new_y) * 100)
		Hide(deployable[e].deployable_no)
		CollisionOff(deployable[e].deployable_no)
		ResetPosition(deployable[e].deployable_no, deployed_x[e], g_PlayerPosY-height_difference[e]+500, deployed_z[e])

		if g_Scancode == 207 and pressed[e] == 0 then --Deploy (EndKey)
			pressed[e] = 1
		end
		if pressed[e] == 1 then	
			ResetPosition(deployable[e].deployable_no, deployed_x[e], g_PlayerPosY-height_difference[e], deployed_z[e])
			SetEntityBaseAlpha(deployable[e].deployable_no,50)
			Show(deployable[e].deployable_no)
			Prompt("Mouse Wheel to rotate, Mouse Button 2 to Deploy")
			if g_MouseWheel < 0 then
				SetRotation(deployable[e].deployable_no,0,g_Entity[deployable[e].deployable_no]['angley']-2,g_PlayerAngZ)				
			end
			if g_MouseWheel > 0 then
				SetRotation(deployable[e].deployable_no,0,g_Entity[deployable[e].deployable_no]['angley']+2,g_PlayerAngZ)				
			end
			if g_MouseClick == 2 then pressed[e] = 2 end			
		end
		if pressed[e] == 2 then
			PlaySound(e,1)
			SetEntityBaseAlpha(deployable[e].deployable_no,salpha[e])
			SetEntityTransparency(deployable[e].deployable_no,0)			
			Show(deployable[e].deployable_no)
			CollisionOn(deployable[e].deployable_no)			
			deployable_deployed[e] = 1
			pressed[e] = 0
		end
	end
end