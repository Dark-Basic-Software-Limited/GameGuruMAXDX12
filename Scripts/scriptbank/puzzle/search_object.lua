-- LUA Script - precede every function and global member with lowercase name of script + '_main'
-- Search Object v14 by Necrym59
-- DESCRIPTION: Searching this object will give the player the selected contents?
-- DESCRIPTION: [USE_RANGE=90(0,100)]
-- DESCRIPTION: [PROMPT_TEXT$="E to Search"]
-- DESCRIPTION: [@CONTENT=1(1=Ammo, 2=Health, 3=Unique Named Item, 4=Cloned Named Item, 5=Nothing)]
-- DESCRIPTION: [NAMED_ITEM$=""] Entity name (will be auto hidden)
-- DESCRIPTION: [QUANTITY=1(1,50)] Health/Ammo only
-- DESCRIPTION: [SEARCH_TIME=8(1,30)]
-- DESCRIPTION: [SEARCH_TEXT$="Searching..."]
-- DESCRIPTION: [RESULT_TEXT$="Found.."]
-- DESCRIPTION: [NOISE_RANGE=500(1,5000)]
-- DESCRIPTION: [@SEARCH_TRIGGER=1(1=Off, 2=On)]
-- DESCRIPTION: [SEARCHBAR_IMAGEFILE$="imagebank\\misc\\testimages\\search-bar.png"]
-- DESCRIPTION: [@PROMPT_DISPLAY=1(1=Local,2=Screen)]
-- DESCRIPTION: [@ITEM_HIGHLIGHT=0(0=None,1=Shape,2=Outline,3=Icon)]
-- DESCRIPTION: [HIGHLIGHT_ICON_IMAGEFILE$="imagebank\\icons\\hand.png"]
-- DESCRIPTION: <Sound0> Searching sound
-- DESCRIPTION: <Sound1> Found item sound

local module_misclib = require "scriptbank\\module_misclib"
local U = require "scriptbank\\utillib"
g_tEnt = {}

local lower = string.lower
local use_range			= {}
local searchobject 		= {}
local prompt_text 		= {}
local content			= {}
local named_item 		= {}
local quantity			= {}
local search_time 		= {}
local search_text 		= {}
local result_text 		= {}
local noise_range		= {}
local search_trigger	= {}
local searchbar_image	= {}
local prompt_display	= {}
local item_highlight	= {}
local highlight_icon	= {}

local searchbar		= {}
local status		= {}
local hl_icon		= {}
local hl_imgwidth	= {}
local hl_imgheight	= {}
local stime 		= {}
local item_entity	= {}
local doonce		= {}
local playonce		= {}
local keypressed	= {}
local tEnt 			= {}
local selectobj 	= {}
local resulttimer	= {}
local itemtoclone	= {}
local newEntn		= {}
local terrainheight = {}
local surfaceheight = {}
local newposx		= {}
local newposy		= {}
local newposz		= {}

function search_object_properties(e, use_range, prompt_text, content, named_item, quantity, search_time, search_text, result_text, noise_range, search_trigger, searchbar_image, prompt_display, item_highlight, highlight_icon_imagefile)
	searchobject[e].prompt_text = prompt_text
	searchobject[e].content = content
	searchobject[e].named_item = lower(named_item)
	searchobject[e].quantity = quantity
	searchobject[e].search_time = search_time
	searchobject[e].search_text = search_text
	searchobject[e].result_text = result_text
	searchobject[e].noise_range = noise_range
	searchobject[e].search_trigger = search_trigger
	searchobject[e].use_range = use_range
	searchobject[e].searchbar_image = searchbar_image or searchbar_imagefile
	searchobject[e].prompt_display = prompt_display
	searchobject[e].item_highlight = item_highlight
	searchobject[e].highlight_icon = highlight_icon_imagefile
end

function search_object_init(e)
	searchobject[e] = {}
	searchobject[e].prompt_text = "E to Search"
	searchobject[e].content = 2
	searchobject[e].named_item = ""
	searchobject[e].quantity = 8
	searchobject[e].search_time = 8
	searchobject[e].search_text = ""
	searchobject[e].result_text = ""
	searchobject[e].noise_range = 500
	searchobject[e].search_trigger = 1
	searchobject[e].use_range = 90
	searchobject[e].searchbar_image = "imagebank\\misc\\testimages\\search-bar.png"
	searchobject[e].prompt_display = 1
	searchobject[e].item_highlight = 0
	searchobject[e].highlight_icon = "imagebank\\icons\\hand.png"

	status[e] = "init"
	doonce[e] = 0
	playonce[e] = 0
	tEnt[e] = 0
	keypressed[e] = 0
	g_tEnt = 0
	selectobj[e] = 0
	itemtoclone[e] = nil
	newEntn[e] = nil
	terrainheight[e] = 0
	surfaceheight[e] = 0
end

function search_object_main(e)

	if status[e] == "init" then
		if searchobject[e].searchbar_image ~= "" then
			searchbar[e] = CreateSprite(LoadImage(searchobject[e].searchbar_image))
			SetSpriteSize(searchbar[e],5,-1)
			SetSpriteColor(searchbar[e],255,255,255,255)
			SetSpritePosition(searchbar[e],200,200)
		end	
		if searchobject[e].item_highlight == 3 and searchobject[e].highlight_icon ~= "" then
			hl_icon[e] = CreateSprite(LoadImage(searchobject[e].highlight_icon))
			hl_imgwidth[e] = GetImageWidth(LoadImage(searchobject[e].highlight_icon))
			hl_imgheight[e] = GetImageHeight(LoadImage(searchobject[e].highlight_icon))
			SetSpriteSize(hl_icon[e],-1,-1)
			SetSpriteDepth(hl_icon[e],100)
			SetSpriteOffset(hl_icon[e],hl_imgwidth[e]/2.0, hl_imgheight[e]/2.0)
			SetSpritePosition(hl_icon[e],500,500)
		end		
	
		if searchobject[e].content == 3 or searchobject[e].content == 4 and searchobject[e].named_item ~= "" then
			item_entity[e] = nil
			for a = 1, g_EntityElementMax do
				if a ~= nil and g_Entity[a] ~= nil then
					if lower(GetEntityName(a)) == searchobject[e].named_item then
						item_entity[e] = a
						CollisionOff(a)
						Hide(a)
						itemtoclone[e] = a
						break
					end
				end
			end
		end
		stime[e] = searchobject[e].search_time * 5
		status[e] = "sealed"
	end
	
	local PlayerDist = GetPlayerDistance(e)
	if PlayerDist < searchobject[e].use_range and status[e] == "sealed" then
		--pinpoint select object--
		module_misclib.pinpoint(e,searchobject[e].use_range,searchobject[e].item_highlight,hl_icon[e])
		tEnt[e] = g_tEnt
		--end pinpoint select object--
		if PlayerDist < searchobject[e].use_range and tEnt[e] == e and GetEntityVisibility(e) == 1 then
			if status[e] == "sealed" then  --Sealed
				if searchobject[e].prompt_display == 1 and keypressed[e] == 0 then TextCenterOnX(50,55,1,searchobject[e].prompt_text) end				
				if searchobject[e].prompt_display == 2 and keypressed[e] == 0 then Prompt(searchobject[e].prompt_text) end
				if g_KeyPressE == 1 then
					keypressed[e] = 1
					if stime[e] > 0 then
						if playonce[e] == 0 then
							PlaySound(e,0)
							playonce[e] = 1
						end
						if searchobject[e].prompt_display == 1 then
							TextCenterOnX(50,55,1,searchobject[e].search_text)
							PasteSpritePosition(searchbar[e],50-(stime[e]/16),56)						
							SetSpriteSize(searchbar[e],stime[e]/8,1)							
						end
						if searchobject[e].prompt_display == 2 then
							Prompt(searchobject[e].search_text)
							PasteSpritePosition(searchbar[e],50-(stime[e]/16),95)						
							SetSpriteSize(searchbar[e],stime[e]/8,1)
						end						
						if searchobject[e].noise_range > 0 then MakeAISound(g_PlayerPosX,g_PlayerPosY,g_PlayerPosZ,searchobject[e].noise_range,1,-1) end
						stime[e] = stime[e]-0.1
						if stime[e] < 0 then stime[e] = 0 end
					end
					if stime[e] == 0 then						
						SetAnimationName(e,"open")
						PlayAnimation(e)
						status[e] = "opened"
						resulttimer[e] = g_Time + 2000
					end
				end
				if g_KeyPressE == 0 then StopSound(e,0) end
			end
		end
		----------------------------------------------------------------------------------------------------------------------------------------------------
		if status[e] == "opened" then  --Opened
			if searchobject[e].content == 1 then	--Ammo
				if searchobject[e].prompt_display == 1 then TextCenterOnX(50,55,1,searchobject[e].result_text.. " " ..searchobject[e].quantity..  " Ammo") end
				if searchobject[e].prompt_display == 2 then PromptDuration(searchobject[e].result_text.. " " ..searchobject[e].quantity..  " Ammo",2000) end				
				if doonce[e] == 0 then
					for index = 1, 10, 1 do
						WeaponID = GetPlayerWeaponID()
						GetWeaponSlot (index, WeaponID, WeaponID)
						local amqty = GetWeaponPoolAmmo(index)
						SetWeaponPoolAmmo(index,amqty + searchobject[e].quantity)
					end
					StopSound(e,0)
					PlaySound(e,1)
					doonce[e] = 1
				end
				status[e] = "searched"
			end

			if searchobject[e].content == 2 then	--Health
				if searchobject[e].prompt_display == 1 then	TextCenterOnX(50,55,1,searchobject[e].result_text.. " " ..searchobject[e].quantity..  " Health") end
				if searchobject[e].prompt_display == 2 then PromptDuration(searchobject[e].result_text.. " " ..searchobject[e].quantity..  " Health",2000) end
				if doonce[e] == 0 then
					StopSound(e,0)
					PlaySound(e,1)
					if g_PlayerHealth < g_gameloop_StartHealth then
						local healthAmount = g_PlayerHealth + searchobject[e].quantity
						if healthAmount > g_gameloop_StartHealth then
							healthAmount = g_gameloop_StartHealth
						end
						SetPlayerHealth(healthAmount)
					end
					doonce[e] = 1					
				end				
				status[e] = "searched"
			else
				StopSound(e,0)
			end

			if searchobject[e].content == 3 then -- Unique Named Item
				if doonce[e] == 0 then
					StopSound(e, 0)
					PlaySound(e, 1)
					if item_entity[e] ~= nil then
						CollisionOn(item_entity[e])
						Show(item_entity[e])
						if searchobject[e].prompt_display == 1 then PromptLocal(e,searchobject[e].result_text.. " " ..searchobject[e].quantity.. " "..searchobject[e].named_item) end							
						if searchobject[e].prompt_display == 2 then PromptDuration(searchobject[e].result_text.. " " ..searchobject[e].quantity.. " "..searchobject[e].named_item,2000) end
					end
					doonce[e] = 1
				end
				status[e] = "searched"
			end
			
			if searchobject[e].content == 4 then -- Cloned Named Item
				if doonce[e] == 0 then
					StopSound(e, 0)
					PlaySound(e, 1)
					if itemtoclone[e] ~= nil then
						local ox,oy,oz = U.Rotate3D( 0, 0, 50, math.rad( g_PlayerAngX ), math.rad( g_PlayerAngY ), math.rad( g_PlayerAngZ ) )
						newposx[e], newposy[e], newposz[e] = g_PlayerPosX + ox, g_PlayerPosY + oy, g_PlayerPosZ + oz
						terrainheight[e] = GetTerrainHeight(newposx[e],newposz[e])
						surfaceheight[e] = GetSurfaceHeight(newposx[e],newposy[e],newposz[e])
						if surfaceheight[e] > terrainheight[e] then
							newposy[e] = surfaceheight[e] + math.random(5,10)
						else
							newposy[e] = terrainheight[e] + math.random(5,10)
						end
						newEntn[e] = SpawnNewEntity(itemtoclone[e])
						ResetPosition(newEntn[e],newposx[e],newposy[e],newposz[e])
						Show(newEntn[e])
						CollisionOn(newEntn[e])
						GravityOn(newEntn[e])						
						if searchobject[e].prompt_display == 1 then PromptLocal(e,searchobject[e].result_text.. " " ..searchobject[e].quantity.. " "..searchobject[e].named_item) end							
						if searchobject[e].prompt_display == 2 then PromptDuration(searchobject[e].result_text.. " " ..searchobject[e].quantity.. " "..searchobject[e].named_item,2000) end
					end
					doonce[e] = 1
				end
				status[e] = "searched"
			end		

			if searchobject[e].content == 5 then --Nothing
				if searchobject[e].prompt_display == 1 then TextCenterOnX(50,55,1,"Nothing found") end
				if searchobject[e].prompt_display == 2 then PromptDuration("Nothing found",2000) end
				StopSound(e,0)
				status[e] = "searched"
			end
		end

		if status[e] == "searched" then --Finished
			if searchobject[e].search_trigger == 2 then
				PerformLogicConnections(e)
				ActivateIfUsed(e)
				status[e] = "finish"				
				SwitchScript(e,"no_behavior_selected.lua")
			end
		end
	end
end