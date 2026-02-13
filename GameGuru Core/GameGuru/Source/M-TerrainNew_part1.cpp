int iLastTerrainSculptMode = -1;
bool bTreeGlobalInit = false;
float fTreeRandomMin = (ggtrees_global_params.paint_scale_random_low); 
float fTreeRandomMax = (ggtrees_global_params.paint_scale_random_high);

void imgui_terrain_loop_v3(void)
{
	if (!imgui_is_running)
		return;

	// terrain editing causes grass to fully update (hills raise grass)
	if (iForceUpdateVegetation == 2)
	{
		iForceUpdateVegetation = 0;
		extern bool bFullVegUpdate;
		bFullVegUpdate = true;
		bUpdateVeg = true;
	}
	if (!bTreeGlobalInit)
	{
		fTreeRandomMin = (ggtrees_global_params.paint_scale_random_low); 
		fTreeRandomMax = (ggtrees_global_params.paint_scale_random_high); 
	
		uint64_t values = gggrass_global_params.paint_type;
		for (int iL = 0; iL < GGGRASS_NUM_SELECTABLE_TYPES; iL++)
		{
			uint64_t mask = 1ULL << iL;
			if (values & mask)
				bCurrentGrassTextureForPaint[iL] = true;
			else
				bCurrentGrassTextureForPaint[iL] = false;
		}
		bTreeGlobalInit = true;
	}
	if (t.grideditselect == 0 && t.terrain.terrainpaintermode >= 0 && t.terrain.terrainpaintermode <= 12)
	{
		if (skib_terrain_frames_execute > 0)
			skib_terrain_frames_execute--;

		if (bTerrain_Tools_Window)
		{
			if (iLastTerrainSculptMode >= 0)
			{
				ggterrain_extra_params.sculpt_mode = iLastTerrainSculptMode;
				iLastTerrainSculptMode = -1;
			}
			ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_NONE;

			float media_icon_size = 40.0f;
			float plate_width = (media_icon_size + 6.0) * 4.0f;
			grideleprof_uniqui_id = 16000;
			int icon_size = 60;
			ImVec2 iToolbarIconSize = { (float)icon_size, (float)icon_size };
			ImVec2 tool_selected_padding = { 1.0, 1.0 };
			tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
			if (pref.current_style == 3)
				tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_Button];

			current_mode = -1;
			int top_current_mode = TOOL_SHAPE;
			if (t.terrain.terrainpaintermode >= 6)
			{
				if (t.terrain.terrainpaintermode == 11)
				{
					current_mode = TOOL_PAINTTREE;
					top_current_mode = TOOL_PAINTTREE;
					ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_TREES;
					ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;

					if (ggtrees_global_params.paint_mode == GGTREES_PAINT_ADD)
						current_mode = TOOL_TREE_ADD;
					if (ggtrees_global_params.paint_mode == GGTREES_PAINT_MOVE)
						current_mode = TOOL_TREE_MOVE;
					if (ggtrees_global_params.paint_mode == GGTREES_PAINT_REMOVE)
						current_mode = TOOL_TREE_DELETE;
					if (ggtrees_global_params.paint_mode == GGTREES_PAINT_SPRAY)
						current_mode = TOOL_TREES_ADD;
					if (ggtrees_global_params.paint_mode == GGTREES_PAINT_SPRAY_REMOVE)
						current_mode = TOOL_TREES_DELETE;
					if (ggtrees_global_params.paint_mode == GGTREES_PAINT_SCALE)
						current_mode = TOOL_TREE_SCALE;
				}
				else if (t.terrain.terrainpaintermode == 10)
				{
					current_mode = TOOL_PAINTGRASS;
					top_current_mode = TOOL_PAINTGRASS;
					ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_GRASS;
					ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
				}
				else
				{
					current_mode = TOOL_PAINTTEXTURE;
					top_current_mode = TOOL_PAINTTEXTURE;
					ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_PAINT;
					ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
				}
			}
			else
			{
				ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_SCULPT;
				ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;

				if(ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RAISE || ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_LOWER)
					current_mode = TOOL_SHAPE;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_LEVEL)
					current_mode = TOOL_LEVELMODE;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_BLEND)
					current_mode = TOOL_BLENDMODE;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RAMP)
					current_mode = TOOL_RAMPMODE;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_PICK)
					current_mode = TOOL_PICK;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_WRITE)
					current_mode = TOOL_WRITE;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RANDOM)
					current_mode = TOOL_RANDOM;
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RESTORE)
					current_mode = TOOL_RESTORE;
			}

			if (ggterrain_extra_params.sculpt_mode != GGTERRAIN_SCULPT_NONE)
			{
				//PE: We need to delay raise/lower object until terrain is done working on it.
				static std::vector<int> adjustedObjects;
				
				if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RAMP && !ImGui::IsMouseDown(0) && (vLastRampTerrainPickPosition.z != 0 || vLastRampTerrainPickPosition.w != 0) )
				{
					//React on mouse release.
					float fx = vLastRampTerrainPickPosition.x;
					float fz = vLastRampTerrainPickPosition.y;
					float fx2 = vLastRampTerrainPickPosition.z;
					float fz2 = vLastRampTerrainPickPosition.w;

					//PE: Move to center of line.
					float radiusx = fx2 - fx;
					float radiusz = fz2 - fz;
					fx += radiusx * 0.5;
					fz += radiusz * 0.5;

					radiusx = abs(radiusx);
					radiusz = abs(radiusz);
					float radius = radiusx;
					if (radiusz > radius) radius = radiusz;
					radius *= 1.5;

					for (t.e = 1; t.e <= g.entityelementlist; t.e++)
					{
						t.obj = t.entityelement[t.e].obj;
						if (t.obj > 0)
						{
							t.ttdx_f = t.entityelement[t.e].x - fx;
							t.ttdz_f = t.entityelement[t.e].z - fz;
							t.ttdd_f = Sqrt(abs(t.ttdx_f*t.ttdx_f) + abs(t.ttdz_f*t.ttdz_f));
							if (t.ttdd_f <= radius)
							{
								//PE: Delay adjusting so just save for now.
								//LB: remember the distance from the old floor so can keep items on tables
								if (t.entityelement[t.e].delay_floorposy == -90000.0f)
								{
									t.entityelement[t.e].delay_floorposy = BT_GetGroundHeight (t.terrain.TerrainID, t.entityelement[t.e].x, t.entityelement[t.e].z);
									adjustedObjects.push_back(t.e);
								}
								g_iDelayActualObjectAdjustment = 40;
							}
						}
					}

					vLastRampTerrainPickPosition.z = 0;
					vLastRampTerrainPickPosition.w = 0;
					
				}
				else if (ImGui::IsMouseDown(0) && ggterrain_extra_params.sculpt_mode != GGTERRAIN_SCULPT_RAMP)
				{
					float fRadius = ggterrain_global_render_params2.brushSize;
					for (t.e = 1; t.e <= g.entityelementlist; t.e++)
					{
						t.obj = t.entityelement[t.e].obj;
						if (t.obj > 0)
						{
							t.ttdx_f = t.entityelement[t.e].x - vLastTerrainPickPosition.x;
							t.ttdz_f = t.entityelement[t.e].z - vLastTerrainPickPosition.z;
							t.ttdd_f = Sqrt(abs(t.ttdx_f*t.ttdx_f) + abs(t.ttdz_f*t.ttdz_f));
							if (t.ttdd_f <= fRadius)
							{
								//PE: Delay adjusting so just save for now.
								//LB: remember the distance from the old floor so can keep items on tables
								if (t.entityelement[t.e].delay_floorposy == -90000.0f)
								{
									t.entityelement[t.e].delay_floorposy = BT_GetGroundHeight (t.terrain.TerrainID, t.entityelement[t.e].x, t.entityelement[t.e].z);
									adjustedObjects.push_back(t.e);
								}
								g_iDelayActualObjectAdjustment = 40;
							}
						}
					}
				}
				else
				{
					if (g_iDelayActualObjectAdjustment > 0)
					{
						if (g_iDelayActualObjectAdjustment == 1)
						{
							// Execute actual object adjustments.
							undosys_multiplevents_start();
							for (t.e = 1; t.e <= g.entityelementlist; t.e++)
							{
								t.obj = t.entityelement[t.e].obj;
								if (t.obj > 0)
								{
									for (int i = 0; i < adjustedObjects.size(); i++)
									{
										if (adjustedObjects[i] == t.e)
										{
											// Can't mix undo master stack items, so move the objects using the terrain undo system.
											entity_createundoaction(eUndoSys_Object_ChangePosRotScl, t.e);
											break;
										}
									}
									t.entityelement[t.e].floorposy = t.entityelement[t.e].delay_floorposy;
									t.entityelement[t.e].delay_floorposy = -90000.0f;
								}
							}
							undosys_multiplevents_finish();
							if (g_iDelayActualObjectAdjustmentSculptCount > 0)
							{
								// connects to earlier terrain sculpts
								undosys_glue(eUndoSys_UndoList, g_iDelayActualObjectAdjustmentSculptCount); 
							}
							adjustedObjects.clear();
							g_iDelayActualObjectAdjustmentSculptCount = 0;
						}
						g_iDelayActualObjectAdjustment--;
					}
				}
			}
			cstr sWindowLabel = "Terrain Tools##Sculpt Terrain##TerrainToolsWindow";
			if (current_mode == TOOL_PAINTGRASS)
				sWindowLabel = "Terrain Tools##Add Vegetation##TerrainToolsWindow";
			if (current_mode == TOOL_PAINTTEXTURE)
				sWindowLabel = "Terrain Tools##Paint Terrain##TerrainToolsWindow";
			if (top_current_mode == TOOL_PAINTTREE)
				sWindowLabel = "Terrain Tools##Add Trees##TerrainToolsWindow";

			extern int iGenralWindowsFlags;
			ImGui::Begin(sWindowLabel.Get(), &bTerrain_Tools_Window, iGenralWindowsFlags);

			float w = ImGui::GetWindowContentRegionWidth();
			ImGuiWindow* window = ImGui::GetCurrentWindow();

			if (ImGui::StyleCollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				// center icons
				float center_icons_numbers = 4.3; //4.0f; //5
				float icon_spacer = 6.0; //10.0f;
				int max_icon_size = 56;
				int control_image_size = 26; //PE: 34 - This is now the lowest possible icon size.
				float control_width = (control_image_size + 3.0) * center_icons_numbers + 6.0;
				control_width += (icon_spacer * center_icons_numbers);
				int indent = 10;
				if (w > control_width)
				{
					//PE: fit perfectly with window width.
					control_image_size = (w - 20.0) / center_icons_numbers;
					control_image_size -= 4.0; //Padding.
					if (control_image_size > max_icon_size) control_image_size = max_icon_size;
					control_width = (control_image_size + 3.0) * center_icons_numbers + 6.0;
					control_width += (icon_spacer * (center_icons_numbers - 1.0f));
					if (control_image_size == max_icon_size)
					{
						indent = (w*0.5) - (control_width*0.5);
						if (indent < 10)
							indent = 10;
					}
				}
				else
				{
					indent = (w*0.5) - (control_width*0.5);
					if (indent < 10)
						indent = 10;
				}
				ImGui::Indent(indent);

				//PE: This is the same as current toolbar background.
				ImVec2 padding = { 3.0, 3.0 };
				extern ImVec4 drawCol_toogle; 
				ImVec4 vIconBackground = ImVec4(0,0,0,0);

				//## Terrain Tool Selection Icons Start ##	
				ImGuiWindow* window = ImGui::GetCurrentWindow();

				ImVec2 cursorRestore = ImGui::GetCursorPos() + ImVec2(0.0f, 1.0f);
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 1.0f - window->Scroll.y));

				if (window)
				{
					ImVec4 background = drawCol_toogle;
					if (pref.current_style == 25)
					{
						background = ImVec4(0.12f, 0.26f, 0.35f, 1.00f);
					}
					ImVec2 cpos = ImGui::GetCursorPos();
					window->DrawList->AddRectFilled(window->Pos+ImVec2(3.0f, cpos.y - padding.y - 3.0) , window->Pos + ImVec2(0, cpos.y+ padding.y) + ImVec2(window->Size.x, control_image_size + padding.x), ImGui::GetColorU32(background),2.0, ImDrawCornerFlags_All);
				}

				ImGui::SetCursorPos(cursorRestore);

				if (top_current_mode == TOOL_SHAPE)	window->DrawList->AddRect((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size), ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);

				if (ImGui::ImgBtn(TOOL_TERRAIN_TOOLBAR, ImVec2(control_image_size, control_image_size), vIconBackground, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					bForceKey = true;
					csForceKey = "t";
					bForceKey2 = true;
					csForceKey2 = "1";
					ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_SCULPT;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Sculpt the editable area of the terrain");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

				if (current_mode == TOOL_PAINTTEXTURE) window->DrawList->AddRect((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size), ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				if (ImGui::ImgBtn(TOOL_PAINTTEXTURE, ImVec2(control_image_size, control_image_size), vIconBackground, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					bForceKey = true;
					csForceKey = "t";
					bForceKey2 = true;
					csForceKey2 = "6";
					bTerrain_Tools_Window = true;
					ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_PAINT;
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Paint into the editable area of the terrain");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

				if (top_current_mode == TOOL_PAINTTREE) window->DrawList->AddRect((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size), ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				if (ImGui::ImgBtn(TOOL_PAINTTREE, ImVec2(control_image_size, control_image_size), vIconBackground, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					t.terrain.terrainpaintermode = 11;
					bTerrain_Tools_Window = true;
					ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_TREES;
					csForceKey = "t";
					csForceKey2 = "11";
					bForceKey = true;
					bForceKey2 = true;
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Paint into the editable area of the terrain");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

				if (current_mode == TOOL_PAINTGRASS) window->DrawList->AddRect((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size), ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				if (ImGui::ImgBtn(TOOL_PAINTGRASS, ImVec2(control_image_size, control_image_size), vIconBackground, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					bForceKey = true;
					csForceKey = "t";
					bForceKey2 = true;
					csForceKey2 = "0";
					bTerrain_Tools_Window = true;
				}
				if (ImGui::windowTabVisible() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Add vegetation into the editable area of the terrain");
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
				
				//## Terrain Tool Selection Icons End ##

				//PE: Now fit to 3 icons.
				ImGui::Indent(-indent);

				//PE: Size to fit trees and bushes.
				center_icons_numbers = 5.0f; //Added SCALE

				icon_spacer = 10.0f;
				max_icon_size = 56;
				control_image_size = 26; //PE: 34 - This is now the lowest possible icon size.
				control_width = (control_image_size + 3.0) * center_icons_numbers + 6.0;
				control_width += (icon_spacer * center_icons_numbers);
				indent = 10;
				if (w > control_width)
				{
					//PE: fit perfectly with window width.
					control_image_size = (w - 20.0) / center_icons_numbers;
					control_image_size -= 4.0; //Padding.
					if (control_image_size > max_icon_size) control_image_size = max_icon_size;
					control_width = (control_image_size + 3.0) * center_icons_numbers + 6.0;
					control_width += (icon_spacer * (center_icons_numbers - 1.0f));
					if (control_image_size == max_icon_size)
					{
						indent = (w*0.5) - (control_width*0.5);
						if (indent < 10)
							indent = 10;
					}
				}
				else
				{
					indent = (w*0.5) - (control_width*0.5);
					if (indent < 10)
						indent = 10;
				}
				int control_image_size_2 = control_image_size;
				ImGui::Indent(indent);

				// LB: Use TAB instead of shift or ctrl, and trigger the toggle as though clicking the button
				bool bSwitchModeToOne = false;
				bool bSwitchModeToZero = false;
				static bool bReadyToChange = true;
				bool bPressTAB = t.inputsys.keytab == 1;
				if (!bPressTAB) bReadyToChange = true;
				if (bReadyToChange && bPressTAB)
				{
					if (current_mode != TOOL_PAINTTEXTURE && current_mode != TOOL_PAINTGRASS)
					{
						if (iTerrainRaiseMode == 0) bSwitchModeToOne = true;
						if (iTerrainRaiseMode == 1) bSwitchModeToZero = true;
					}
					else if (current_mode == TOOL_PAINTGRASS)
					{
						if (iTerrainGrassPaintMode == 0) bSwitchModeToOne = true;
						if (iTerrainGrassPaintMode == 1) bSwitchModeToZero = true;
					}
					else
					{
						if (iTerrainPaintMode == 0) bSwitchModeToOne = true;
						if (iTerrainPaintMode == 1) bSwitchModeToZero = true;
					}
					bReadyToChange = false; //toggle, wait until tab is released again.
				}

				LPSTR pEditTitle = "Sculpting Terrain";
				if (current_mode == TOOL_PAINTTEXTURE) pEditTitle = "Painting Terrain";
				if (current_mode == TOOL_PAINTGRASS) pEditTitle = "Adding Vegetation";
				if (top_current_mode == TOOL_PAINTTREE) pEditTitle = "Painting Trees";

				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f)); //Give header a little space.
				ImGui::TextCenter(pEditTitle);
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));

				if (top_current_mode == TOOL_PAINTTREE && ggtrees_global_params.draw_enabled )
				{
					//#############################
					//## PE: Display Tree tools. ##
					//#############################

					ImVec4 back_color = ImColor(255, 255, 255, 0);
					if (current_mode == TOOL_TREE_ADD)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_TREE_ADD, ImVec2(control_image_size, control_image_size), back_color, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						ggtrees_global_params.paint_mode = GGTREES_PAINT_ADD;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Tree");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					if (current_mode == TOOL_TREE_MOVE)//(iTerrainRaiseMode != 1 || bHoldShift ) )
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_TREE_MOVE, ImVec2(control_image_size, control_image_size), back_color, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						ggtrees_global_params.paint_mode = GGTREES_PAINT_MOVE;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move Tree");

					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					if (current_mode == TOOL_TREE_DELETE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_TREE_DELETE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {
						ggtrees_global_params.paint_mode = GGTREES_PAINT_REMOVE;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Remove Tree");

					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					if (current_mode == TOOL_TREE_SCALE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_TREE_SCALE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {
						ggtrees_global_params.paint_mode = GGTREES_PAINT_SCALE;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Scale Tree");

					//New line.

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((control_image_size + icon_spacer + icon_spacer), 0.0f));
					if (current_mode == TOOL_TREES_ADD)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_TREES_ADD, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {
						ggtrees_global_params.paint_mode = GGTREES_PAINT_SPRAY;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Spray Trees");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));
					//ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((control_image_size + icon_spacer)*0.5, 0.0f));

					if (current_mode == TOOL_TREES_DELETE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_TREES_DELETE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {

						ggtrees_global_params.paint_mode = GGTREES_PAINT_SPRAY_REMOVE;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Clear Trees");

				}

				ImGui::Indent(-indent);

				//PE: Size to fit rest.
				center_icons_numbers = 3.0f; //Added SCALE
				icon_spacer = 10.0f;
				max_icon_size = 56;
				control_image_size = 26; //PE: 34 - This is now the lowest possible icon size.
				control_width = (control_image_size + 3.0) * center_icons_numbers + 6.0;
				control_width += (icon_spacer * center_icons_numbers);
				indent = 10;
				if (w > control_width)
				{
					//PE: fit perfectly with window width.
					control_image_size = (w - 20.0) / center_icons_numbers;
					control_image_size -= 4.0; //Padding.
					if (control_image_size > max_icon_size) control_image_size = max_icon_size;
					control_width = (control_image_size + 3.0) * center_icons_numbers + 6.0;
					control_width += (icon_spacer * (center_icons_numbers - 1.0f));
					if (control_image_size == max_icon_size)
					{
						indent = (w*0.5) - (control_width*0.5);
						if (indent < 10)
							indent = 10;
					}
				}
				else
				{
					indent = (w*0.5) - (control_width*0.5);
					if (indent < 10)
						indent = 10;
				}
				control_image_size_2 = control_image_size;
				ImGui::Indent(indent);

				if (current_mode != TOOL_PAINTTEXTURE && current_mode != TOOL_PAINTGRASS && top_current_mode != TOOL_PAINTTREE && top_current_mode != TOOL_PAINTBUSH)
				{
					//###############################
					//## PE: Display Sculpt tools. ##
					//###############################

					ImVec4 toggle_color = ImColor(255, 255, 255, 0);
					if (current_mode == TOOL_SHAPE && iTerrainRaiseMode == 1)//(iTerrainRaiseMode == 1 && !bHoldShift) )
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (bSwitchModeToOne == true || ImGui::ImgBtn(TOOL_SHAPE_UP, ImVec2(control_image_size, control_image_size), toggle_color, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						//Paint mode.
						iTerrainRaiseMode = 1;
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "1";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_RAISE;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raise Terrain");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					toggle_color = ImColor(255, 255, 255, 0);
					if (current_mode == TOOL_SHAPE && iTerrainRaiseMode != 1)//(iTerrainRaiseMode != 1 || bHoldShift ) )
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (bSwitchModeToZero == true || ImGui::ImgBtn(TOOL_SHAPE_DOWN, ImVec2(control_image_size, control_image_size), toggle_color, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						//Remove mode.
						iTerrainRaiseMode = 0;
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "1";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_LOWER;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lower Terrain");

					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					CheckTutorialAction("TOOL_LEVELMODE", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_LEVELMODE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_LEVELMODE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "2";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_LEVEL;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Level Mode");

					//New line.
					
					CheckTutorialAction("TOOL_BLENDMODE", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_BLENDMODE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_BLENDMODE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {

						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "4";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_BLEND;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Blend Mode");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					CheckTutorialAction("TOOL_RAMPMODE", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_RAMPMODE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_RAMPMODE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) 
					{
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "5";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_RAMP;

						// Ensure that the ramp brush size is set only once, so that after the user changes it, they don't need to keep changing it
						static bool bOnlyOnce = true;
						if (bOnlyOnce)
						{
							// If people don't like this way, we could store a brush size for ramp mode only and use that instead
							ggterrain_global_render_params2.brushSize = 75;
							bOnlyOnce = false;
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Ramp Mode");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					CheckTutorialAction("TOOL_RANDOM", -10.0f);
					if (current_mode == TOOL_RANDOM)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_RANDOM, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) 
					{
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "5";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_RANDOM;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Random Mode");

					//New line.
					
					CheckTutorialAction("TOOL_PICK", -10.0f);
					if (current_mode == TOOL_PICK)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_PICK, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {

						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "4";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_PICK;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Pick A Height");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					CheckTutorialAction("TOOL_WRITE", -10.0f);
					if (current_mode == TOOL_WRITE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_WRITE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {

						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "5";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_WRITE;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Paint With Chosen Height");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(icon_spacer, 0.0f));

					CheckTutorialAction("TOOL_RESTORE", -10.0f);
					if (current_mode == TOOL_RESTORE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_RESTORE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) {

						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "5";
						ggterrain_extra_params.sculpt_mode = GGTERRAIN_SCULPT_RESTORE;
						GGTerrain::GGTerrain_CancelRamp();
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Restore To Original Height");

					//#########################
				}
				ImGui::Indent(-indent);

				#define GRASSRESTOREWASREMOVED

				if (t.terrain.terrainpaintermode == 10)
				{
					control_width = (control_image_size_2 + 3.0f) * 2.0f + 6.0f;
					indent = (w*0.5f) - (control_width*0.5f);
					if (indent < 10)
						indent = 10;
					ImGui::Indent(indent);

					//Grass
					if (!(top_current_mode == TOOL_PAINTGRASS && !gggrass_global_params.draw_enabled))
					{
						if (iTerrainGrassPaintMode == 1)// && !bHoldShift)
						{
							ImVec2 padding = { 3.0, 3.0 };
							const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size_2, control_image_size_2));
							window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
						}

						if (bSwitchModeToOne == true || ImGui::ImgBtn(EBE_CONTROL1, ImVec2(control_image_size_2, control_image_size_2), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) 
						{
							//Paint mode.
							iTerrainGrassPaintMode = 1;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paint Mode");
						ImGui::SameLine();

						if (iTerrainGrassPaintMode == 0)
						{
							ImVec2 padding = { 3.0, 3.0 };
							const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size_2, control_image_size_2));
							window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
						}
						if (bSwitchModeToZero == true || ImGui::ImgBtn(EBE_CONTROL2, ImVec2(control_image_size_2, control_image_size_2), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) 
						{
							//Remove mode.
							iTerrainGrassPaintMode = 0;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Mode");
					}
					ImGui::Indent(-indent);
				}

				//Setup indent for 2 icons.
				control_width = (control_image_size_2 + 3.0f) * 2.0f + 6.0f;
				indent = (w*0.5f) - (control_width*0.5f);
				if (indent < 10)
					indent = 10;

				ImGui::Indent(indent);
				if (t.terrain.terrainpaintermode >= 6 && t.terrain.terrainpaintermode < 10)
				{
					if (iTerrainPaintMode == 1)// && !bHoldShift)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size_2, control_image_size_2));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					if (bSwitchModeToOne == true || ImGui::ImgBtn(EBE_CONTROL1, ImVec2(control_image_size_2, control_image_size_2), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) 
					{
						//Paint mode.
						iTerrainPaintMode = 1;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paint Mode");
					ImGui::SameLine();

					if (iTerrainPaintMode != 1)// || bHoldShift )
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size_2, control_image_size_2));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (bSwitchModeToZero == true || ImGui::ImgBtn(EBE_CONTROL2, ImVec2(control_image_size_2, control_image_size_2), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors)) 
					{
						//Remove mode.
						iTerrainPaintMode = 0;
						ggterrain_extra_params.paint_material = 0;
					}
					if (iTerrainPaintMode == 1)
					{
						ggterrain_extra_params.paint_material = iCurrentTextureForPaint + 1;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restore Mode");
				}

				if (current_mode == TOOL_PAINTGRASS)
				{
					if(iTerrainGrassPaintMode == 0)
						gggrass_global_params.paint_mode = 1; //PE: Delete
					if (iTerrainGrassPaintMode == 1)
						gggrass_global_params.paint_mode = 0; //PE: Paint.
				}

				ImGui::Indent(-indent);

				ImGui::Indent(10);

				//Brush Size.
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::PushItemWidth(-10);

				if (current_mode != TOOL_PAINTTEXTURE && top_current_mode != TOOL_PAINTTREE && top_current_mode != TOOL_PAINTBUSH)
				{
					//PE: Hidden in new design but pickplane always on.
					ggterrain_extra_params.edit_pick_mode = 0; //(1) PE: Now default to pick mode 0.
				}

				//ggtrees_global_params.draw_enabled
				bool bBrushSizeEnable = true;
				if (top_current_mode == TOOL_PAINTTREE)
				{
					if (!ggtrees_global_params.draw_enabled) bBrushSizeEnable = false;
				}
				else if (top_current_mode == TOOL_PAINTBUSH)
				{
					if (!ggtrees_global_params.draw_enabled) bBrushSizeEnable = false;
				}
				else if (top_current_mode == TOOL_PAINTGRASS)
				{
					if (!gggrass_global_params.draw_enabled) bBrushSizeEnable = false;
				}

				if (bBrushSizeEnable)
				{
					ImGui::TextCenter("Brush Size");
					ImGui::MaxSliderInputFloatPower("##BrushSize", &ggterrain_global_render_params2.brushSize, 15.0f, 7000.0f, 0, 15.0f, 7500, 30, 2.0f);

					// Alter brush size with input.
					if (t.inputsys.k_s == "-" && ggterrain_global_render_params2.brushSize > 15.0f)
					{
						ggterrain_global_render_params2.brushSize -= 500 * ImGui::GetIO().DeltaTime;
					}
					if (t.inputsys.k_s == "=" && ggterrain_global_render_params2.brushSize < 7000.0f)
					{
						ggterrain_global_render_params2.brushSize += 500 * ImGui::GetIO().DeltaTime;
					}

					ImGuiIO& io = ImGui::GetIO();
					if (io.KeyCtrl && ImGui::GetIO().MouseWheel != 0)
					{
						float speed = 50.0;
						ggterrain_global_render_params2.brushSize += ImGui::GetIO().MouseWheel*speed;
						if (ggterrain_global_render_params2.brushSize > 7000.0f) ggterrain_global_render_params2.brushSize = 7000.0f;
						if (ggterrain_global_render_params2.brushSize < 15.0) ggterrain_global_render_params2.brushSize = 15.0f;
					}
				}

				static bool bGrassMatchTerrain = 0;
				if (current_mode == TOOL_PAINTTEXTURE)
				{
					bool bTmp = 1 - bGrassMatchTerrain;
					if (ImGui::Checkbox("Match Painted Grass", &bTmp))
					{
						bGrassMatchTerrain = 1 - bTmp;
						gggrass_global_params.paint_material = bGrassMatchTerrain;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("When painting, any grass will change to ensure it blends best. This option is synced with the grass tick tickbox");
				}

				if (current_mode != TOOL_PAINTTEXTURE && top_current_mode != TOOL_PAINTTREE && top_current_mode != TOOL_PAINTBUSH && top_current_mode != TOOL_PAINTGRASS)
				{

					ImGui::TextCenter("Scuplt Speed");
					ImGui::MaxSliderInputFloatPower("##Sculpt Speed", &ggterrain_extra_params.sculpt_speed, 1.0f, 200.0f, 0, 1.0f, 200.0f, 30, 2.0f);

					if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_RANDOM)
					{
						ImGui::TextCenter("Randomness Frequency");
						ImGui::SliderFloat("##RandomnessFreq", &ggterrain_extra_params.sculpt_randomness_frequency, 3.0f, 50.0f, "%.1f", 2.0f);
					}

					if (ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_PICK || ggterrain_extra_params.sculpt_mode == GGTERRAIN_SCULPT_WRITE)
					{
						ImGui::TextCenter("Chosen Sculpt Height");
						float meterValue = GGTerrain_UnitsToMeters(ggterrain_extra_params.sculpt_chosen_height);
						if (ImGui::SliderFloat("##ChosenSculptHeight", &meterValue, -2000.0f, 5000.0f))
						{
							ggterrain_extra_params.sculpt_chosen_height = GGTerrain_MetersToUnits(meterValue);
						}
					}
				}

				if (top_current_mode == TOOL_PAINTGRASS && gggrass_global_params.draw_enabled)
				{
					ImGui::TextCenter("Grass Density");
					ImGui::SliderInt("##GrassDensity", &gggrass_global_params.paint_density, 0, 100);

					ImGui::TextCenter("Grass Draw Distance");
					int fTmp = gggrass_global_params.lod_dist;
					if (ImGui::SliderInt("##GrassDrawDistance", &fTmp, 750, 7000)) //PE: min distance changed to 750 so there are a range to fade, or chunks will pop in.
					{
						g.projectmodified = 1;
						gggrass_global_params.lod_dist = fTmp;
					}

					if (pref.iEnableAdvancedGrass)
					{
						int iTmp;

						if (gggrass_global_params.min_height < g.gdefaultwaterheight) gggrass_global_params.min_height = g.gdefaultwaterheight;
						if (gggrass_global_params.max_height < g.gdefaultwaterheight) gggrass_global_params.max_height = g.gdefaultwaterheight;
						if (gggrass_global_params.min_height_underwater > g.gdefaultwaterheight) gggrass_global_params.min_height_underwater = g.gdefaultwaterheight;
						if (gggrass_global_params.max_height_underwater > g.gdefaultwaterheight) gggrass_global_params.max_height_underwater = g.gdefaultwaterheight;
						if (gggrass_global_params.min_height_underwater < g.gdefaultwaterheight - 2000.0) gggrass_global_params.min_height_underwater = g.gdefaultwaterheight - 2000.0;
						if (gggrass_global_params.max_height_underwater < g.gdefaultwaterheight - 2000.0) gggrass_global_params.max_height_underwater = g.gdefaultwaterheight - 2000.0;

						ImGui::TextCenter("Grass Scale");
						if (ImGui::SliderFloat("##Grassgrass_scale", &gggrass_global_params.grass_scale, 1.0f, 200.0f, "%.2f", 1.0f))
						{
							//ggterrain_extra_params.iUpdateGrass = 2;
						}

						ImGui::TextCenter("Grass Start/End Altitude");

						//ImGui::TextCenter("Grass Min Height");
						if (ImGui::MaxSliderInputFloatPower("##GrassMinHeight", &gggrass_global_params.min_height, g.gdefaultwaterheight, 30000.0, "Sets the global altitude at which grass should be rendered, zero overlaps underwater depths", 0, 100, 30, 3.0f))
						{
							ggterrain_extra_params.iUpdateGrass = 2;
							g.projectmodified = 1;
						}

						if (ImGui::MaxSliderInputFloatPower("##GrassMaxHeight", &gggrass_global_params.max_height, g.gdefaultwaterheight, 30000.0, "Sets the global altitude at which the grass no longer draws", 0, 100, 30, 3.0f))
						{
							ggterrain_extra_params.iUpdateGrass = 2;
							g.projectmodified = 1;
						}
						if (gggrass_global_params.max_height < gggrass_global_params.min_height)
						{
							float fTmp = gggrass_global_params.max_height;
							gggrass_global_params.max_height = gggrass_global_params.min_height;
							gggrass_global_params.min_height = fTmp;
						}

						ImGui::TextCenter("Grass Start/End Altitude Underwater");
						if (ImGui::MaxSliderInputFloatPower("##GrassMinHeightWater", &gggrass_global_params.min_height_underwater, g.gdefaultwaterheight-2000.0, g.gdefaultwaterheight, "Sets the global altitude at which underwater grass should be rendered", 0, 100, 30, 1.0f))
						{
							ggterrain_extra_params.iUpdateGrass = 2;
							g.projectmodified = 1;
						}

						if (ImGui::MaxSliderInputFloatPower("##GrassMaxHeightWater", &gggrass_global_params.max_height_underwater, g.gdefaultwaterheight-2000.0, g.gdefaultwaterheight, "Sets the global altitude at which the underwater grass no longer draws", 0, 100, 30, 1.0f))
						{
							ggterrain_extra_params.iUpdateGrass = 2;
							g.projectmodified = 1;
						}
						if (gggrass_global_params.max_height_underwater < gggrass_global_params.min_height_underwater)
						{
							float fTmp = gggrass_global_params.max_height_underwater;
							gggrass_global_params.max_height_underwater = gggrass_global_params.min_height_underwater;
							gggrass_global_params.min_height_underwater = fTmp;
						}
					}

					bool bTmp = 1 - bGrassMatchTerrain;
					if (ImGui::Checkbox("Match Terrain Color", &bTmp))
					{
						bGrassMatchTerrain = 1 - bTmp;
						gggrass_global_params.paint_material = bGrassMatchTerrain;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("When painting grass, the style will be automatically chosen to blend with the terrain. This option is synced with the paint texture tickbox");

					extern void ControlAdvancedSetting(int&, const char*, bool* = nullptr);
					ControlAdvancedSetting(pref.iEnableAdvancedGrass, "Advanced Grass Settings");
				}

				if (top_current_mode == TOOL_PAINTTREE && ggtrees_global_params.draw_enabled)
				{

					ImGui::TextCenter("Tree Density");
					ImGui::SliderInt("##TreeDensity", &ggtrees_global_params.paint_density, 0, 100);
					ImGui::TextCenter("Tree Height");
					ImGui::PushItemWidth(-10);
					if (ImGui::MaxSliderInputRangeFloatDirect("##TreeRandomHeight", &fTreeRandomMin, &fTreeRandomMax, 0.0, 255.0, "Set Tree Random Height Interval"))
					{
						fTreeRandomMin = (int)fTreeRandomMin;
						fTreeRandomMax = (int)fTreeRandomMax;
						if (fTreeRandomMin > fTreeRandomMax)
						{
							int iTmp = fTreeRandomMax;
							fTreeRandomMax = fTreeRandomMin;
							fTreeRandomMin = iTmp;
						}
						if (fTreeRandomMin < 0) fTreeRandomMin = 0;
						if (fTreeRandomMax > 255) fTreeRandomMax = 255;
						ggtrees_global_params.paint_scale_random_low = (fTreeRandomMin); //*2.55) + 1;
						ggtrees_global_params.paint_scale_random_high = (fTreeRandomMax); //*2.55) + 1;
						if (ggtrees_global_params.paint_scale_random_high > 255) ggtrees_global_params.paint_scale_random_high = 255;
					}
					ImGui::PopItemWidth();

					ImGui::TextCenter("Tree Water Distance");
					if (ImGui::SliderFloat("##TreeWaterDist", &ggtrees_global_params.water_dist, -1000.0f, 5000.0f, "%.0f", 2.0f))
					{
						ggterrain_extra_params.iUpdateTrees = 1;
					}

					ImGui::TextCenter("Tree Wind");
					if (ImGui::SliderFloat("##TreeWind", &t.visuals.tree_wind, 0.0f, 1.0f, "%.2f", 1.0f))
					{
						t.gamevisuals.tree_wind = t.visuals.tree_wind;
						extern wiECS::Entity g_weatherEntityID;
						wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
						if (weather)
						{
							//weather->tree_wind = t.visuals.tree_wind; // REMOVED
						}
					}

					ImGui::TextCenter("Subsurface Scattering");
					if (ImGui::SliderFloat("##TreeSSS", &t.visuals.tree_sss, 0.0f, 1.0f, "%.2f", 1.0f))
					{
						t.gamevisuals.tree_sss = t.visuals.tree_sss;
						extern wiECS::Entity g_weatherEntityID;
						wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
						if (weather)
						{
							//weather->tree_sss = t.visuals.tree_sss; // REMOVED
						}
					}

					float but_gadget_size = ImGui::GetFontSize()*10.0;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

					if (ImGui::StyleButton("Lock Tree Visibility##TerrainTrees", ImVec2(but_gadget_size, 0)))
					{
						int iAction = askBoxCancel("This will lock the current visibility of trees so that terrain changes will no longer show or hide them, this can only be undone by clicking Randomize All Trees.\n\nAre you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
						if (iAction == 1)
						{
							GGTrees::GGTrees_LockVisibility();
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Locks the current visibility of trees so that terrain changes no longer affect them, this can only be undone by clicking Randomize All Trees");
				}

				ImGui::PopItemWidth();

				//PE: Process brush size keyboard-shortcuts.
				if (ImGui::IsKeyDown(17) && (ImGui::IsKeyDown(187) || ImGui::IsKeyDown(107))) //[CTRL] + [+]
				{
					t.terrain.RADIUS_f += g.timeelapsed_f*3.0;
					if (t.terrain.RADIUS_f < t.tmin) t.terrain.RADIUS_f = t.tmin;
					if (t.terrain.RADIUS_f > g.fTerrainBrushSizeMax) t.terrain.RADIUS_f = g.fTerrainBrushSizeMax;
				}
				if (ImGui::IsKeyDown(17) && (ImGui::IsKeyDown(189) || ImGui::IsKeyDown(109))) //[CTRL] + [-]
				{
					t.terrain.RADIUS_f -= g.timeelapsed_f*3.0;
					if (t.terrain.RADIUS_f < t.tmin) t.terrain.RADIUS_f = t.tmin;
					if (t.terrain.RADIUS_f > g.fTerrainBrushSizeMax) t.terrain.RADIUS_f = g.fTerrainBrushSizeMax;
				}

				ImGui::Indent(-10);
			}

			if (current_mode == TOOL_PAINTTEXTURE)
				imgui_Customize_Terrain_v3(0);
			if (current_mode == TOOL_PAINTGRASS && gggrass_global_params.draw_enabled)
				imgui_Customize_Vegetation_v3(0);
			if (top_current_mode == TOOL_PAINTTREE && ggtrees_global_params.draw_enabled)
				imgui_Customize_Tree_v3(0);

			if (top_current_mode == TOOL_PAINTTREE && t.showeditortrees == 0)
			{
				bool bShow = t.showeditortrees;
				ImGui::Indent(10.0f);
				if (ImGui::Checkbox("Enable Trees##terraintooltrees", &bShow))
				{
					t.showeditortrees = bShow;
					ggtrees_global_params.draw_enabled = bShow;
					t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = t.showeditortrees; //PE: Also set test level.
				}
				ImGui::Indent(-10.0f);
			}
			if (top_current_mode == TOOL_PAINTGRASS && t.showeditorveg == 0)
			{
				bool bShow = t.showeditorveg;
				ImGui::Indent(10.0f);
				if (ImGui::Checkbox("Enable Grass##terraintoolgrass", &bShow))
				{
					t.showeditorveg = bShow;
					gggrass_global_params.draw_enabled = bShow;
					t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = t.showeditorveg; //PE: Need to also update test level.
				}
				ImGui::Indent(-10.0f);
			}

			if (current_mode != TOOL_PAINTTEXTURE && current_mode != TOOL_PAINTGRASS && top_current_mode != TOOL_PAINTTREE && top_current_mode != TOOL_PAINTBUSH)
			{
				imgui_Customize_Water_V2(4);
			}

			if (top_current_mode == TOOL_PAINTGRASS && gggrass_global_params.draw_enabled)
			{
				if (ImGui::StyleCollapsingHeader("Fill Whole Terrain", ImGuiTreeNodeFlags_DefaultOpen))
				{
					float but_gadget_size = ImGui::GetFontSize()*14.0;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
					if (ImGui::StyleButton("Populate Vegetation Everywhere", ImVec2(but_gadget_size, 0)))
					{
						//PE: Crash if empty, div by zero.
						const char* msg = "This will populate vegetation everywhere with your chosen settings, are you sure?\n\n (To populate grass that matches the terrain instead, check the Match Terrain Color checkbox)";
						if( gggrass_global_params.paint_material == 0 ) msg = "This will reset grass everywhere to match the terrain, are you sure?\n\n (To populate with your chosen grass instead, uncheck the Match Terrain Color checkbox)";
						int iAction = askBoxCancel(msg, "Confirmation"); //1==Yes 2=Cancel 0=No
						if (iAction == 1)
						{
							if (gggrass_global_params.paint_type == 0)
								gggrass_global_params.paint_type = 1;

							GGGrass::GGGrass_AddAll();
						}
					}
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
					if (ImGui::StyleButton("Clear All Vegetation", ImVec2(but_gadget_size, 0)))
					{
						int iAction = askBoxCancel("This will clear all vegetation, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
						if (iAction == 1)
						{
							GGGrass::GGGrass_RemoveAll();
						}
					}
				}
			}

			if (!pref.bHideTutorials)
			{
				if (ImGui::StyleCollapsingHeader("Tutorial", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent(10);
					char* my_combo_itemsp[] = { NULL,NULL,NULL };
					int my_combo_items = 0;
					int iVideoSection = 0;
					cstr cShowTutorial = "0601 - Terrain Editing";
					my_combo_itemsp[0] = "0601 - Terrain Editing";
					my_combo_items = 1;
					cShowTutorial = "0601 - Terrain Editing";
					iVideoSection = SECTION_SCULPT_TERRAIN;

					SmallTutorialVideo(cShowTutorial.Get(), my_combo_itemsp, my_combo_items, iVideoSection);
					float but_gadget_size = ImGui::GetFontSize()*12.0;
					float w = ImGui::GetWindowContentRegionWidth() - 10.0;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
					#ifdef INCLUDESTEPBYSTEP
					if (ImGui::StyleButton("View Step by Step Tutorial", ImVec2(but_gadget_size, 0)))
					{
						bHelp_Window = true;
						bHelpVideo_Window = true;
						extern bool bSetTutorialSectionLeft;
						bSetTutorialSectionLeft = false;
						strcpy(cForceTutorialName, cShowTutorial.Get());
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start Step by Step Tutorial");
					#endif
					ImGui::Indent(-10);
				}
			}

			// insert a keyboard shortcut component into panel
			eKeyboardShortcutType KST = eKST_Sculpt;
			if (current_mode == TOOL_PAINTTEXTURE)
				KST = eKST_Paint;
			else if (current_mode == TOOL_PAINTGRASS)
				KST = eKST_AddVeg;
			UniversalKeyboardShortcut(KST);

			ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
			if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
			{
				bImGuiGotFocus = true;
			}

			void CheckMinimumDockSpaceSize(float minsize);
			CheckMinimumDockSpaceSize(250.0f);

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
			{
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}

			ImGui::End();
		}
	}
}

void imgui_terrain_loop(void)
{
	if (!imgui_is_running)
		return;

	// terrain editing causes grass to fully update (hills raise grass)
	if (iForceUpdateVegetation == 2)
	{
		iForceUpdateVegetation = 0;
		extern bool bFullVegUpdate;
		bFullVegUpdate = true;
		bUpdateVeg = true;
	}
	
	if (bUpdateVeg) 
	{
		if (bEnableVeg) 
		{
			t.visuals.VegQuantity_f = t.gamevisuals.VegQuantity_f;
			t.visuals.VegWidth_f = t.gamevisuals.VegWidth_f;
			t.visuals.VegHeight_f = t.gamevisuals.VegHeight_f;

			extern bool bResourcesSet, bGridMade;

			bool bOldGridMade = bGridMade;
			int iTrimUsingGrassMemblock = 0;
			if (t.game.gameisexe == 1) iTrimUsingGrassMemblock = t.terrain.grassmemblock;
			if (g.usegrassbelowwater > 0)
				MakeVegetationGridQuick(4.0f*t.visuals.VegQuantity_f, t.visuals.VegWidth_f, t.visuals.VegHeight_f, terrain_veg_areawidth, t.terrain.vegetationgridsize, t.tTerrainID, iTrimUsingGrassMemblock, true);
			else
				MakeVegetationGridQuick(4.0f*t.visuals.VegQuantity_f, t.visuals.VegWidth_f, t.visuals.VegHeight_f, terrain_veg_areawidth, t.terrain.vegetationgridsize, t.tTerrainID, iTrimUsingGrassMemblock, false);

			// small lookup for memblock painting circles
			static bool bCurveDataSet = false;
			if (!bCurveDataSet) {
				Dim(t.curve_f, 100);
				for (t.r = 0; t.r <= 180; t.r++)
				{
					t.trx_f = Cos(t.r - 90)*100.0;
					t.trz_f = Sin(t.r - 90)*100.0;
					t.curve_f[int((100 + t.trz_f) / 2)] = t.trx_f / 100.0;
				}
				bCurveDataSet = true;
			}
			t.terrain.grassregionupdate = 0; //PE: Make sure we update.
			t.terrain.grassupdateafterterrain = 1;
			t.terrain.lastgrassupdatex1 = -1; //PE: Force update.
			t.terrain.grassupdateafterterrain = 0;
			ShowVegetationGrid();
			visuals_justshaderupdate();
			iLastUpdateVeg = MAXTimer();
		}
		else 
		{
			HideVegetationGrid();
			iLastUpdateVeg = MAXTimer();
		}
		bUpdateVeg = false;
	}
	else 
	{
		bool bReadyToUpdateVeg = false;
		if (bVegHasChanged)
			bReadyToUpdateVeg = true;

		if (bEnableVeg && iTerrainVegLoopUpdate++ > 10) 
		{
			iTerrainVegLoopUpdate = 0;
		}

		//Continue cheking if we need to update terrain.
		if (bReadyToUpdateVeg && bEnableVeg ) 
		{
			t.visuals.VegQuantity_f = t.gamevisuals.VegQuantity_f;
			t.visuals.VegWidth_f = t.gamevisuals.VegWidth_f;
			t.visuals.VegHeight_f = t.gamevisuals.VegHeight_f;

			t.terrain.grassupdateafterterrain = 1;
			t.terrain.grassupdateafterterrain = 0;
			ShowVegetationGrid();

			bReadyToUpdateVeg = false;
			iLastUpdateVeg = MAXTimer();
			bVegHasChanged = false;
		}
	}

	if (t.grideditselect == 0 && t.terrain.terrainpaintermode >= 0 && t.terrain.terrainpaintermode <= 10)
	{
		if (skib_terrain_frames_execute > 0)
			skib_terrain_frames_execute--;

		if (bTerrain_Tools_Window) {

			
			float media_icon_size = 40.0f;
			float plate_width = (media_icon_size + 6.0) * 4.0f;
			grideleprof_uniqui_id = 16000;
			int icon_size = 60;
			ImVec2 iToolbarIconSize = { (float)icon_size, (float)icon_size };
			ImVec2 tool_selected_padding = { 1.0, 1.0 };
			tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
			if (pref.current_style == 3)
				tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_Button];

			current_mode = -1;
			if (t.terrain.terrainpaintermode >= 6) 
			{
				if (t.terrain.terrainpaintermode == 10) 
				{
					current_mode = TOOL_PAINTGRASS;
				}
				else 
				{
					current_mode = TOOL_PAINTTEXTURE;
				}
			}
			else 
			{
				if (t.terrain.terrainpaintermode == 1)
					current_mode = TOOL_SHAPE;
				if (t.terrain.terrainpaintermode == 2)
					current_mode = TOOL_LEVELMODE;
				if (t.terrain.terrainpaintermode == 3)
					current_mode = TOOL_STOREDLEVEL;
				if (t.terrain.terrainpaintermode == 4)
					current_mode = TOOL_BLENDMODE;
				if (t.terrain.terrainpaintermode == 5)
					current_mode = TOOL_RAMPMODE;
			}

			cstr sWindowLabel = "Sculpt Terrain##TerrainToolsWindow";
			if(current_mode == TOOL_PAINTGRASS)
				sWindowLabel = "Add Vegetation##TerrainToolsWindow";
			if (current_mode == TOOL_PAINTTEXTURE)
				sWindowLabel = "Paint Terrain##TerrainToolsWindow";

			extern int iGenralWindowsFlags;
			ImGui::Begin(sWindowLabel.Get(), &bTerrain_Tools_Window, iGenralWindowsFlags);

			float w = ImGui::GetWindowContentRegionWidth();
			ImGuiWindow* window = ImGui::GetCurrentWindow();

			if (ImGui::StyleCollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen)) 
			{
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				int control_image_size = 26; //PE: 34 - This is now the lowest possible icon size.
				float control_width = (control_image_size + 3.0) * 5.0f + 6.0;
				int indent = 10;

				if (w > control_width) 
				{
					//PE: fit perfectly with window width.
					control_image_size = (w - 20.0) / 5.0;
					control_image_size -= 4.0; //Padding.
					if (control_image_size > 64) control_image_size = 64;
					control_width = (control_image_size + 3.0) * 5.0f + 6.0;
					if (control_image_size == 64)
					{
						indent = (w*0.5) - (control_width*0.5);
						if (indent < 10)
							indent = 10;
					}
					//iSliderAdjustY = control_image_size - 34
				}
				else {
					indent = (w*0.5) - (control_width*0.5);
					if (indent < 10)
						indent = 10;
				}

				ImGui::Indent(indent);

				if (current_mode != TOOL_PAINTTEXTURE && current_mode != TOOL_PAINTGRASS)
				{
					//#########################
					//PE: Display Sculpt tools.
					//#########################

					ImVec2 padding = { 3.0, 3.0 };

					//CheckTutorialAction("TOOL_SHAPE", -10.0f); //This goes to the main toolbar not here.
					if (current_mode == TOOL_SHAPE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_SHAPE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,true, bBoostIconColors)) {
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "1";
					}
					if (ImGui::IsItemHovered() ) ImGui::SetTooltip("%s", "Shape Mode");
					ImGui::SameLine();

					CheckTutorialAction("TOOL_LEVELMODE", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_LEVELMODE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_LEVELMODE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "2";
					}
					if (ImGui::IsItemHovered() ) ImGui::SetTooltip("%s", "Level Mode");
					ImGui::SameLine();

					CheckTutorialAction("TOOL_STOREDLEVEL", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_STOREDLEVEL)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_STOREDLEVEL, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "3";
					}
					if (ImGui::IsItemHovered() ) ImGui::SetTooltip("%s", "Stored Level Mode");
					ImGui::SameLine();

					CheckTutorialAction("TOOL_BLENDMODE", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_BLENDMODE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_BLENDMODE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {

						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "4";
					}
					if (ImGui::IsItemHovered() ) ImGui::SetTooltip("%s", "Blend Mode");
					ImGui::SameLine();

					CheckTutorialAction("TOOL_RAMPMODE", -10.0f); //Tutorial: check if we are waiting for this action
					if (current_mode == TOOL_RAMPMODE)
					{
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(TOOL_RAMPMODE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {

						bForceKey = true;
						csForceKey = "t";
						bForceKey2 = true;
						csForceKey2 = "5";
					}
					if (ImGui::IsItemHovered() ) ImGui::SetTooltip("%s", "Ramp Mode");

					//#########################
				}

				if (t.terrain.terrainpaintermode >= 6) 
				{
					if (iTerrainPaintMode == 1)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					if (ImGui::ImgBtn(EBE_CONTROL1, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
						//Paint mode.
						iTerrainPaintMode = 1;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paint Mode");
					ImGui::SameLine();

					if (iTerrainPaintMode != 1)
					{
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(EBE_CONTROL2, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
						//Remove mode.
						iTerrainPaintMode = 0;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Mode");

					ImGui::SameLine();
				}

				if (current_mode != TOOL_PAINTTEXTURE && current_mode != TOOL_PAINTGRASS) { //Was: current_mode == TOOL_SHAPE

					ImVec4 toggle_color = ImColor(255, 255, 255, 0);
					if (iTerrainRaiseMode == 1)
					{
						//PE: Toggle looks better when  we have the tool icons above.
						toggle_color = ImColor(128, 128, 128, 128);
					}

					if (ImGui::ImgBtn(TOOL_SHAPE_UP, ImVec2(control_image_size, control_image_size), toggle_color, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
						//Paint mode.
						iTerrainRaiseMode = 1;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raise Terrain");
					ImGui::SameLine();

					toggle_color = ImColor(255, 255, 255, 0);
					if (iTerrainRaiseMode != 1)
					{
						toggle_color = ImColor(128, 128, 128, 128);
					}
					if (ImGui::ImgBtn(TOOL_SHAPE_DOWN, ImVec2(control_image_size, control_image_size), toggle_color, ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) 
					{
						//Remove mode.
						iTerrainRaiseMode = 0;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lower Terrain");

					ImGui::SameLine();
				}

				ImVec2 cp = ImGui::GetCursorPos();
				ImGui::SetItemAllowOverlap();
				ImGui::SameLine();
				float fAdjY = control_image_size - 34 * 0.5;
				fAdjY -= 20;

				ImGui::SetCursorPos(ImVec2(cp.x, cp.y + (ImGui::GetFontSize() * 1.5) + fAdjY ));
				//				ImGui::PushItemWidth(-10);
				ImGui::PushItemWidth((control_image_size + 6.0) * 3.0);
				ImGui::SetWindowFontScale(0.5);

				//ImGuiCol_FrameBg
				ImVec4 oldFrameBg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
				ImVec4 oldBorder = ImGui::GetStyle().Colors[ImGuiCol_Border];

				ImGui::GetStyle().Colors[ImGuiCol_FrameBg].w *= 0.25;
				ImGui::GetStyle().Colors[ImGuiCol_Border].w *= 0.25;

				if (ImGui::SliderFloat("##Brushsize", &t.terrain.RADIUS_f, 70.0f, 500.0f, "")) { //g.fTerrainBrushSizeMax
					if (t.terrain.RADIUS_f < t.tmin) t.terrain.RADIUS_f = t.tmin;
					if (t.terrain.RADIUS_f > g.fTerrainBrushSizeMax) t.terrain.RADIUS_f = g.fTerrainBrushSizeMax;
				}
				ImGui::GetStyle().Colors[ImGuiCol_FrameBg].w = oldFrameBg.w;
				ImGui::GetStyle().Colors[ImGuiCol_Border].w = oldBorder.w;
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius %.2f", t.terrain.RADIUS_f);
				ImGui::PopItemWidth();
				ImGui::SetWindowFontScale(1.0);

				ImGui::SetCursorPos(cp);

				if (0 && t.terrain.RADIUS_f == 110.0f)
				{
					ImVec2 padding = { 3.0, 3.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				ImGui::SetItemAllowOverlap();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 5));
				if (ImGui::ImgBtn(TOOL_DOTCIRCLE_S, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
					t.terrain.RADIUS_f = 110.0f;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius Small");

				ImGui::SameLine();

				if (0 && t.terrain.RADIUS_f == 280.0f)
				{
					ImVec2 padding = { 3.0, 3.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOL_DOTCIRCLE_M, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
					t.terrain.RADIUS_f = 280.0f;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius Medium");

				ImGui::SameLine();

				if (0 && t.terrain.RADIUS_f == 450.0f)
				{
					ImVec2 padding = { 3.0, 3.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(control_image_size, control_image_size));
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOL_DOTCIRCLE, ImVec2(control_image_size, control_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors)) {
					t.terrain.RADIUS_f = 450.0f;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw Radius Large");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 5));
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 8));
				ImGui::Indent(-indent);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				if (current_mode == TOOL_PAINTGRASS)
				{
					int iTextRightPos = 116; //136

					ImGui::PushItemWidth(-10);
					static float fval1 = 20.0, fval2 = 80.0;
					ImVec2 vOldPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::Text("Height:");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
					ImGui::SetCursorPos(ImVec2(iTextRightPos, ImGui::GetCursorPosY()));
					ImGui::RangeSlider("##VegHeight", g_fvegRandomMin, g_fvegRandomMax, 100.0f);
					ImGui::SetCursorPos(ImVec2(vOldPos.x, ImGui::GetCursorPosY() + 9));
					ImGui::PopItemWidth();
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Vegetation Draw Height Range");

					//PE: These is still Overall and not when spraying, so keep them as is for now.
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::Text("Overall Quantity:");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
					ImGui::SetCursorPos(ImVec2(iTextRightPos, ImGui::GetCursorPosY()));
					ImGui::PushItemWidth(-10);
					if (ImGui::SliderFloat("##VegOverallQuantity", &t.gamevisuals.VegQuantity_f, 0.0, 100.0,"%.0f"))
					{
						iLastUpdateVeg = MAXTimer();
						bUpdateVeg = true;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Overall Vegetation Quantity");
					ImGui::PopItemWidth();

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::Text("Overall Width:");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
					ImGui::SetCursorPos(ImVec2(iTextRightPos, ImGui::GetCursorPosY()));
					ImGui::PushItemWidth(-10);
					if (ImGui::SliderFloat("##VegOverallWidth", &t.gamevisuals.VegWidth_f, 0.0, 100.0, "%.0f"))
					{
						iLastUpdateVeg = MAXTimer();
						bUpdateVeg = true;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Overall Vegetation Width");
					ImGui::PopItemWidth();
				}
			}

			if (current_mode == TOOL_PAINTTEXTURE)
				imgui_Customize_Terrain(0);
			if (current_mode == TOOL_PAINTGRASS)
				imgui_Customize_Vegetation(0);

			if (ImGui::StyleCollapsingHeader("Keyboard Shortcuts ???", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));

				// context help button
				float button_gadget_size = ImGui::GetFontSize()*10.0;
				float w = ImGui::GetWindowContentRegionWidth();
				ImGui::Text("Left Mouse Button to Paint.");
				ImGui::Text("Shift + Left Mouse Button to Remove.");
				ImGui::Text("+ Increase Draw Radius.");
				ImGui::Text("- Decrease Draw Radius.");
				ImGui::Indent(-10);
			}

			ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
			if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
			{
				bImGuiGotFocus = true;
			}

			void CheckMinimumDockSpaceSize(float minsize);
			CheckMinimumDockSpaceSize(250.0f);

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}

			ImGui::End();
		}
	}
}


void terrain_createactualterrain(void)
{
	t.terrain.terrainobjectindex = t.terrain.objectstartindex + 3;
	return;
}

//PE: We need these functions for skyspec ...
void terrain_parsed_getstring(void)
{
	for (t.n = 1; t.n <= Len(t.line_s.Get()); t.n++)
	{
		if (cstr(Mid(t.line_s.Get(), t.n)) == "=")
		{
			if (cstr(Mid(t.line_s.Get(), t.n + 1)) == " ")
				t.rest_s = Right(t.line_s.Get(), (Len(t.line_s.Get()) - t.n) - 1);
			else
				t.rest_s = Right(t.line_s.Get(), Len(t.line_s.Get()) - t.n);

			t.n = Len(t.line_s.Get());
		}
	}
}

void terrain_parsed_getvalues(void)
{
	for (t.n = 1; t.n <= Len(t.line_s.Get()); t.n++)
	{
		if (cstr(Mid(t.line_s.Get(), t.n)) == "=")
		{
			t.rest_s = Right(t.line_s.Get(), Len(t.line_s.Get()) - t.n);
			t.n = Len(t.line_s.Get());
		}
	}
	t.valuei = 0;
	for (t.n = 1; t.n <= Len(t.rest_s.Get()); t.n++)
	{
		if (cstr(Mid(t.rest_s.Get(), t.n)) == "," || t.n == Len(t.rest_s.Get()))
		{
			if (t.n == Len(t.rest_s.Get()))
			{
				t.value_s = Left(t.rest_s.Get(), t.n);
			}
			else
			{
				t.value_s = Left(t.rest_s.Get(), t.n - 1);
			}
			t.value_f[t.valuei] = ValF(t.value_s.Get()); ++t.valuei;
			t.rest_s = Right(t.rest_s.Get(), Len(t.rest_s.Get()) - t.n);
			t.n = 0;
		}
	}
}


std::vector<cstr> terrain_selections;
std::vector<cstr> terrain_selections_text;
void init_terrain_selections()
{
	terrain_selections.clear();

	terrain_selections.push_back("texturebank\\terrain grassy gravel_color.dds");
	terrain_selections.push_back("texturebank\\terrain rock_color.dds");
	terrain_selections.push_back("texturebank\\terrain stony grass_color.dds");
	terrain_selections.push_back("texturebank\\terrain grass_color.dds");
	terrain_selections.push_back("texturebank\\terrain cracked mud_color.dds");
	terrain_selections.push_back("texturebank\\terrain dirt_color.dds");

	for (int i = 0; i < terrain_selections.size(); i++)
	{
		char pFileOnly[MAX_PATH];
		strcpy(pFileOnly, terrain_selections[i].Get());
		for (int n = strlen(pFileOnly) - 1; n > 0; n--)
		{
			if (pFileOnly[n] == '\\' || pFileOnly[n] == '/')
			{
				strcpy(pFileOnly, pFileOnly + n + 1);
				break;
			}
		}
		char *remove_ext = (char *)pestrcasestr(pFileOnly, "_color.dds");
		if (remove_ext)
			*remove_ext = 0;
		terrain_selections_text.push_back(pFileOnly);
	}
}

int imgui_get_selections(std::vector<cstr> selections, std::vector<cstr> selection_text, int uniqid, int imagestart, bool *winopen)
{
	if (!*winopen)
		return(-1);

	int retval = -1;
	int iPreviewIconSize = 64;

	ImGui::SetNextWindowSize(ImVec2(35 * ImGui::GetFontSize(), 33 * ImGui::GetFontSize()), ImGuiCond_Once); //ImGuiCond_FirstUseEver
	ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
	ImGui::Begin("Select Texture##imgui_get_selections", winopen, 0);

	ImGui::Indent(10);
	cstr label = cstr("imgui_my_selections") + cstr(uniqid);
	ImGui::Columns(4, label.Get(), false);  //false no border
	for (int i = 0; i < selections.size(); i++)
	{
		if (!ImageExist(t.terrain.imagestartindex + imagestart + i)) {
			image_setlegacyimageloading(true);
			LoadImage(selections[i].Get(), t.terrain.imagestartindex + imagestart + i, 0, g.gdividetexturesize);
			image_setlegacyimageloading(false);
		}

		if (ImageExist(t.terrain.imagestartindex + imagestart + i)) {

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (iPreviewIconSize*0.5), 0.0f));
			if (ImGui::ImgBtn(t.terrain.imagestartindex + imagestart + i, ImVec2(iPreviewIconSize, iPreviewIconSize), ImColor(0, 0, 0, 196), ImColor(255, 255, 255, 255), ImColor(220, 220, 220, 220), ImColor(180, 180, 160, 255), 0, 0, 0, 0, false, false, true))
			{
				winopen = false;
				retval = i;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::ImgBtn(t.terrain.imagestartindex + imagestart + i, ImVec2(300, 300), ImColor(0, 0, 0, 255));
				ImGui::EndTooltip();
			}
			ImGui::TextCenter(selection_text[i].Get());
			ImGui::NextColumn();
		}
	}

	if (ImageExist(TOOL_LOADLEVEL)) {

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (iPreviewIconSize*0.5), 0.0f));
		if (ImGui::ImgBtn(TOOL_LOADLEVEL, ImVec2(iPreviewIconSize, iPreviewIconSize), ImColor(0, 0, 0, 196), ImColor(255, 255, 255, 255), ImColor(220, 220, 220, 220), ImColor(180, 180, 160, 255), 0, 0, 0, 0, false, false, true,false,false, bBoostIconColors))
		{
			winopen = false;
			retval = 999;
		}

		ImGui::TextCenter("Custom Texture");
		ImGui::NextColumn();
	}

	ImGui::Indent(-10);

	ImGui::Columns(1);
	ImGui::End();

	return(retval);
}

