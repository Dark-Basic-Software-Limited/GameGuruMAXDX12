int storyboard_add_missing_nodex(int node,float area_width, float node_width, float node_height, bool bForce, bool bRestoring)
{
	// LB latest
	int orgnode = node;
	constexpr int allWidgets = ALLOW_BUTTON | ALLOW_TEXT | ALLOW_IMAGE | ALLOW_RADIOTYPE | ALLOW_SLIDER | ALLOW_TICKBOX | ALLOW_VIDEO | ALLOW_PROGRESS | ALLOW_TEXTAREA;
	constexpr int defaultWidgets = ALLOW_TEXT | ALLOW_IMAGE | ALLOW_VIDEO | ALLOW_BUTTON;
	bool bUpdateStoryboardToV2 = false;

	//General.
	if( strlen(Storyboard.Nodes[0].thumb) > 0 && pestrcasestr(Storyboard.Nodes[0].thumb,"loadingsplash.jpg"))
		strcpy(Storyboard.Nodes[0].thumb, "editors\\uiv3\\loadingsplash.jpg");

	if(orgnode == 0)
	{
		Storyboard.Nodes[node].used = true;
		Storyboard.Nodes[node].type = STORYBOARD_TYPE_SPLASH;
		Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5) - ((node_width + NODE_WIDTH_PADDING) * 4.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * node);
		Storyboard.Nodes[node].iEditEnable = true;
		strcpy(Storyboard.Nodes[node].title, "Splash Screen");
		strcpy(Storyboard.Nodes[node].thumb, "editors\\uiv3\\loadingsplash.jpg");
		strcpy(Storyboard.Nodes[node].lua_name, ""); //No script.
		strcpy(Storyboard.Nodes[node].output_title[0], " Connect to Scene ");
		strcpy(Storyboard.Nodes[node].output_action[0], "loadscene"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
	}

	if (orgnode == 1)
	{
		Storyboard.Nodes[node].used = true;
		Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
		Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5) - ((node_width + NODE_WIDTH_PADDING) * 2.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 0);
		Storyboard.Nodes[node].iEditEnable = true;
		strcpy(Storyboard.Nodes[node].title, "Title Screen");
		strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_title.lua.png");
		strcpy(Storyboard.Nodes[node].lua_name, "title.lua");
		strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\title.png");
		Storyboard.Nodes[node].widgets_available = allWidgets;
		strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
		strcpy(Storyboard.Nodes[node].output_title[0], " START GAME -> Connect to Level ");
		strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
		strcpy(Storyboard.Nodes[node].output_title[1], " LOAD GAME -> Connect to Scene ");
		strcpy(Storyboard.Nodes[node].output_action[1], "loadscene"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
		strcpy(Storyboard.Nodes[node].output_title[2], " ABOUT -> Connect to Scene ");
		strcpy(Storyboard.Nodes[node].output_action[2], "loadscene"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_SCREEN;
		int button = 0;
		strcpy(Storyboard.Nodes[node].widget_label[button], "START");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20.0 + 10.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_STARTGAME;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "start"); //Also add "-hover.png" ...
		button++;
		strcpy(Storyboard.Nodes[node].widget_label[button], "LOAD GAME");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20.0 + 20.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "load-game"); //Also add "-hover.png" ...
		button++;
		strcpy(Storyboard.Nodes[node].widget_label[button], "ABOUT");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20.0 + 30.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "about"); //Also add "-hover.png" ...
		button++;
		strcpy(Storyboard.Nodes[node].widget_label[button], "QUIT GAME");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20.0 + 40.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_EXITGAME;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "quit-game"); //Also add "-hover.png" ...
	}

	if (orgnode == 4)
	{
		Storyboard.Nodes[node].used = true;
		Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
		Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 2);
		Storyboard.Nodes[node].iEditEnable = true;
		strcpy(Storyboard.Nodes[node].title, "About Screen");
		strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_about.lua.png");
		strcpy(Storyboard.Nodes[node].lua_name, "about.lua");
		strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\about.png");
		Storyboard.Nodes[node].widgets_available = allWidgets;
		strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
		int button = 0;
		strcpy(Storyboard.Nodes[node].widget_label[button], "ABOUT GAME");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_name[button], "about-title"); //Also add "-hover.png" ...
		button = 1;
		strcpy(Storyboard.Nodes[node].widget_label[button], "");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXTAREA;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_name[button], "about-textarea"); //Also add "-hover.png" ...
		button = 2;
		strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 80); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "back"); //NOTE: DUP (back) - Also add "-hover.png" ...
	}

	if (orgnode == 5)
	{
		Storyboard.Nodes[node].used = true;
		Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
		Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5) + ((node_width + NODE_WIDTH_PADDING) * 4.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 0);
		Storyboard.Nodes[node].iEditEnable = true;
		strcpy(Storyboard.Nodes[node].title, "Game Won Screen");
		strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_win.lua.png");
		strcpy(Storyboard.Nodes[node].lua_name, "win.lua");
		strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\end.png");
		Storyboard.Nodes[node].widgets_available = allWidgets;
		strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
		int button = 0;
		strcpy(Storyboard.Nodes[node].widget_label[button], "CONTINUE");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 80); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_CONTINUE;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "continue"); //NOTE: DUP (continue) - Also add "-hover.png" ...
		button = 1;
		strcpy(Storyboard.Nodes[node].widget_label[button], "GAME COMPLETE");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_name[button], "gamecomplete"); //Also add "-hover.png" ...
	}

	if (orgnode == 6)
	{
		Storyboard.Nodes[node].used = true;
		Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
		Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5) + ((node_width + NODE_WIDTH_PADDING) * 4.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 1);
		Storyboard.Nodes[node].iEditEnable = true;
		strcpy(Storyboard.Nodes[node].title, "Game Over Screen");
		strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_lose.lua.png");
		strcpy(Storyboard.Nodes[node].lua_name, "lose.lua");
		strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\lost.png");
		Storyboard.Nodes[node].widgets_available = allWidgets;
		strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
		int button = 0;
		strcpy(Storyboard.Nodes[node].widget_label[button], "CONTINUE");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 80); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_CONTINUE;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
		strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
		strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
		strcpy(Storyboard.Nodes[node].widget_name[button], "continue"); //NOTE: DUP (continue) - Also add "-hover.png" ...
		button = 1;
		strcpy(Storyboard.Nodes[node].widget_label[button], "GAME OVER");
		Storyboard.Nodes[node].widget_used[button] = 1;
		Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
		Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
		Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8.0); //Pos in percent. using pivot center on X only.
		Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
		Storyboard.Nodes[node].widget_layer[button] = 0;
		Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
		strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
		strcpy(Storyboard.Nodes[node].widget_name[button], "gameover"); //Also add "-hover.png" ...
	}

	if (orgnode == 7 && bRestoring == false)
	{
		Storyboard.Nodes[node].used = true;
		Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
		Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5) + ((node_width + NODE_WIDTH_PADDING) * 2.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 0);
		Storyboard.Nodes[node].iEditEnable = true;
		strcpy(Storyboard.Nodes[node].title, "Level 1");
		strcpy(Storyboard.Nodes[node].levelnumber, "Level 1");
		strcpy(Storyboard.Nodes[node].thumb, "");
		strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
		strcpy(Storyboard.Nodes[node].output_title[0], " WIN LEVEL -> Connect to Scene ");
		strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
		Storyboard.Nodes[node].output_linkto[0] = Storyboard.Nodes[5].input_id[0];
		strcpy(Storyboard.Nodes[node].output_title[1], " GAME OVER -> Connect to Scene ");
		strcpy(Storyboard.Nodes[node].output_action[1], "loadlevel"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
		Storyboard.Nodes[node].output_linkto[1] = Storyboard.Nodes[6].input_id[0];
		strcpy(Storyboard.Nodes[node].output_title[2], " NEXT LEVEL -> Connect to Level ");
		strcpy(Storyboard.Nodes[node].output_action[2], "loadlevel"); //Not defined this yet.
		Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_LEVEL;
		Storyboard.Nodes[node].output_linkto[2] = 0;
	}

	if (orgnode == 8)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "gamemenu.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}

		//Change old node label.
		if (stricmp(Storyboard.Nodes[node].widget_label[5], "SOUND LEVELS") == 0)
		{
			strcpy(Storyboard.Nodes[node].widget_label[5], "SOUND SETTINGS");
		}

		//8 Default GAME PAUSED
		if( bValid && (Storyboard.Nodes[node].used == false || bForce) )
		{
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
			if (bRestoring==false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5) - ((node_width + NODE_WIDTH_PADDING)*4.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 2);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Game Paused");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_gamemenu.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "gamemenu.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, ""); //No backdrop transparent.
			Storyboard.Nodes[node].screen_backdrop_transparent = true;
			Storyboard.Nodes[node].widgets_available = allWidgets;
			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "GAME PAUSED");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_name[button], "about-title"); //Also add "-hover.png" ...
			button = 1;
			strcpy(Storyboard.Nodes[node].widget_label[button], "MAIN MENU");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_LEAVEGAME; //	//LeaveGame
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "main-menu"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[button], ""); //Empty no output pin.
			strcpy(Storyboard.Nodes[node].output_action[button], "title"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[button] = STORYBOARD_TYPE_SCREEN;
			button = 2;
			strcpy(Storyboard.Nodes[node].widget_label[button], "LOAD GAME");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 30.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN; //	//LeaveGame
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "load-game"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[0], " LOAD GAME -> Connect to Scene ");
			strcpy(Storyboard.Nodes[node].output_action[0], "loadscene"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
			button = 3;
			strcpy(Storyboard.Nodes[node].widget_label[button], "SAVE GAME");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 40.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN; //	//LeaveGame
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "save-game"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[1], " SAVE GAME -> Connect to Scene ");
			strcpy(Storyboard.Nodes[node].output_action[1], "savescene"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
			button = 4;
			strcpy(Storyboard.Nodes[node].widget_label[button], "GRAPHICS SETTINGS");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 50.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN; //	//LeaveGame
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "graphics-settings"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[2], " GRAPHICS SETTINGS -> Connect to Scene ");
			strcpy(Storyboard.Nodes[node].output_action[2], "graphicsscene"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_SCREEN;
			button = 5;
			strcpy(Storyboard.Nodes[node].widget_label[button], "SOUND SETTINGS"); //SOUND LEVELS
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 60.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN; //	//LeaveGame
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "sound-levels"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[3], " SOUND SETTINGS -> Connect to Scene ");
			strcpy(Storyboard.Nodes[node].output_action[3], "soundsscene"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[3] = STORYBOARD_TYPE_SCREEN;
			button = 6;
			strcpy(Storyboard.Nodes[node].widget_label[button], "RESUME GAME");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 90.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_RESUMEGAME; //ResumeGame
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "resume-game"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[button], ""); //Empty no output pin.
			strcpy(Storyboard.Nodes[node].output_action[button], ""); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[button] = STORYBOARD_ACTIONS_RESUMEGAME;
			button = 7;
			strcpy(Storyboard.Nodes[node].widget_label[button], "CONTROLS"); //SOUND LEVELS
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 70.0); // 80.0 if have current design.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_GOTOSCREEN; //
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls"); //Also add "-hover.png" ...
			strcpy(Storyboard.Nodes[node].output_title[7], " CONTROLS -> Connect to Scene ");
			strcpy(Storyboard.Nodes[node].output_action[7], "controls.lua"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[7] = STORYBOARD_TYPE_SCREEN;
		}
	}
	if (orgnode == 3)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "loadgame.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}
		//3 Default Load Game screen.
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].widgets_available = allWidgets;
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
			if (bRestoring == false)
			{
				if (bForce)
					Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 1);
				else
					Storyboard.Nodes[node].restore_position = ImVec2(area_width * 0.5 - (node_width * 0.5), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 2);
			}
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Load Game Screen");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_loadgame.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "loadgame.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\loading.png");
			//Input.
			strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
			//No Output.

			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "LOAD GAME");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "load-game-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			for (int l = 1; l < 9; l++)
			{
				button++;
				//These should be blank and filled out using: TextCenterOnX
				strcpy(Storyboard.Nodes[node].widget_label[button], "");
				Storyboard.Nodes[node].widget_used[button] = 1;
				Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
				Storyboard.Nodes[node].widget_read_only[button] = 1;
				Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
				Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (l * 7)); //Pos in percent. using pivot center on X only.
				Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_RETURNVALUETOLUA; //Just used for a title.
				Storyboard.Nodes[node].widget_layer[button] = 0;
				Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
				strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
				strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
				strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
				strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
				strcpy(Storyboard.Nodes[node].widget_name[button], "load-game-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...
				Storyboard.Nodes[node].widget_font_size[button] = 0.5;
			}

			button++;
			strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 90); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "back-load-game"); //NOTE: DUP (back) - Also add "-hover.png" ...
		}
	}

	//Save game.
	if (orgnode == 9)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "savegame.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}
		//9 Default Save Game screen.
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].widgets_available = allWidgets;
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
			if (bRestoring == false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5) - ((node_width + NODE_WIDTH_PADDING)*2.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 1);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Save Game Screen");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_savegame.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "savegame.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\loading.png");
			strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "SAVE GAME");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "save-game-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...
			for (int l = 1; l < 9; l++)
			{
				button++;
				//These should be blank and filled out using: TextCenterOnX
				strcpy(Storyboard.Nodes[node].widget_label[button], "");
				Storyboard.Nodes[node].widget_used[button] = 1;
				Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
				Storyboard.Nodes[node].widget_read_only[button] = 1;
				Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
				Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (l * 7)); //Pos in percent. using pivot center on X only.
				Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_RETURNVALUETOLUA; //Just used for a title.
				Storyboard.Nodes[node].widget_layer[button] = 0;
				Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
				strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
				strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
				strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
				strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
				strcpy(Storyboard.Nodes[node].widget_name[button], "save-game-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...
				Storyboard.Nodes[node].widget_font_size[button] = 0.5;
			}

			button++;
			strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 90); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "back-save-game"); //NOTE: DUP (back) - Also add "-hover.png" ...
		}
	}


	//Graphics
	if (orgnode == 10)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "graphics.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}


		//PE: Change old node label.
		if (stricmp(Storyboard.Nodes[node].widget_label[4], "FOV") == 0)
		{
			strcpy(Storyboard.Nodes[node].widget_label[4], "FIELD OF VIEW");
		}

		//10 Default graphics screen
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
			if (bRestoring == false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5) - ((node_width + NODE_WIDTH_PADDING)*2.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 2);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Graphics Settings Screen");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_graphics.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "graphics.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "");
			Storyboard.Nodes[node].screen_backdrop_transparent = true;
			Storyboard.Nodes[node].readouts_available = READOUT_GRAPHICS;
			Storyboard.Nodes[node].widgets_available = allWidgets;

			//Input.
			strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
			//No Output.

			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "GRAPHICS SETTINGS");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "graphics-game-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 1;
			strcpy(Storyboard.Nodes[node].widget_label[button], "LOW");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_RADIOTYPE;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button*10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "lowest"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 2;
			strcpy(Storyboard.Nodes[node].widget_label[button], "MEDIUM");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_RADIOTYPE;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "medium"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 3;
			strcpy(Storyboard.Nodes[node].widget_label[button], "HIGHEST");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_RADIOTYPE;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "highest"); //NOTE: DUP (load-game) - Also add "-hover.png" ...


			button = 4;
			strcpy(Storyboard.Nodes[node].widget_label[button], "FIELD OF VIEW");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "highest"); //NOTE: DUP (load-game) - Also add "-hover.png" ...


			button = 5;
			strcpy(Storyboard.Nodes[node].widget_label[button], "");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_SLIDER;

			int iPlayerFOVPerc = (((t.visuals.CameraFOV_f * t.visuals.CameraASPECT_f) - 20.0) / 180.0) * 114.0f;// *100.0;
			if (iPlayerFOVPerc < 0) iPlayerFOVPerc = 33; //default FOV
			if (iPlayerFOVPerc > 100) iPlayerFOVPerc = 33; //default FOV
			Storyboard.NodeSliderValues[node][button] = iPlayerFOVPerc;

			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "highest"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 6;
			strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "back-save-game"); //NOTE: DUP (back) - Also add "-hover.png" ...
		}
	}

	//Sounds
	if (orgnode == 11)
	{
		if (bUpdateStoryboardToV2)
		{
			strcpy(Storyboard.widget_readout[node][2], "Sound Effects Volume");
			strcpy(Storyboard.widget_readout[node][4], "Music Volume");
		}
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "sounds.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}

		if (stricmp(Storyboard.Nodes[node].widget_label[0], "SOUND LEVELS") == 0)
		{
			strcpy(Storyboard.Nodes[node].widget_label[0], "SOUND VOLUME SETTINGS");
		}

		//11 Default sounds screen
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
			if (bRestoring == false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5) - ((node_width + NODE_WIDTH_PADDING)*2.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 3);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Sound Settings Screen");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_sounds.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "sounds.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "");
			Storyboard.Nodes[node].screen_backdrop_transparent = true;
			Storyboard.Nodes[node].readouts_available = READOUT_SOUND;
			Storyboard.Nodes[node].widgets_available = allWidgets;

			//Input.
			strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
			//No Output.

			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "SOUND VOLUME SETTINGS"); //"SOUND LEVELS"
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "sounds-game-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 1;
			strcpy(Storyboard.Nodes[node].widget_label[button], "SOUND EFFECTS");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "sound-effects-label"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 2;
			strcpy(Storyboard.Nodes[node].widget_label[button], "");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_SLIDER;
			strcpy(Storyboard.widget_readout[node][button], "Sound Effects Volume");
			Storyboard.NodeSliderValues[node][button] = 100.0;

			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "slider-bar-empty"); //NOTE: DUP (load-game) - Also add "-hover.png" ...


			button = 3;
			strcpy(Storyboard.Nodes[node].widget_label[button], "MUSIC");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "music-label"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 4;
			strcpy(Storyboard.Nodes[node].widget_label[button], "");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_SLIDER;
			strcpy(Storyboard.widget_readout[node][button], "Music Volume");
			Storyboard.NodeSliderValues[node][button] = 100.0;

			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "slider-bar-empty"); //NOTE: DUP (load-game) - Also add "-hover.png" ...


			button = 5;
			strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "back-save-game"); //NOTE: DUP (back) - Also add "-hover.png" ...
		}
	}


	//Controls
	if (orgnode == 12)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "controls.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}

		//12 Default controls screen
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;
			if (bRestoring == false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5) - ((node_width + NODE_WIDTH_PADDING)*2.0), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 4);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Controls Screen");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_controls.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "controls.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "");
			Storyboard.Nodes[node].screen_backdrop_transparent = true;
			Storyboard.Nodes[node].readouts_available = READOUT_INPUT;
			Storyboard.Nodes[node].widgets_available = allWidgets;

			//Input.
			strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
			//No Output.

			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "CONTROLS"); //"SOUND LEVELS"
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 8); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-title"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 1;
			strcpy(Storyboard.Nodes[node].widget_label[button], "Movement");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(14.0, 20 ); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-movement"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 2;
			strcpy(Storyboard.Nodes[node].widget_label[button], "W - Forward\nA - Strafe Left\nS - Back\nD - Strafe Right\nMouse - Look Around\nSpace - Jump\nShift - Run");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXTAREA;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(20.0, 30.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-movement-textarea"); //Also add "-hover.png" ...


			button = 3;
			strcpy(Storyboard.Nodes[node].widget_label[button], "Weapons");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(47.0, 20); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-weapons"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 4;
			strcpy(Storyboard.Nodes[node].widget_label[button], "LMB - Shoot\nRMB - Aim\nAlt - Melee Attack\nR - Reload\n1 - 9 Weapons Slots");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXTAREA;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(53.0, 30.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-weapons-textarea"); //Also add "-hover.png" ...



			button = 5;
			strcpy(Storyboard.Nodes[node].widget_label[button], "Action");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(76.0, 20); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE; //Just used for a title.
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-actions"); //NOTE: DUP (load-game) - Also add "-hover.png" ...

			button = 6;
			strcpy(Storyboard.Nodes[node].widget_label[button], "F - Flash Light\nE - Use\nEsc - Menu");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXTAREA;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(80.0, 30.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_name[button], "controls-actions-textarea"); //Also add "-hover.png" ...



			button = 7;
			strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 20 + (button * 10)); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
			strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
			strcpy(Storyboard.Nodes[node].widget_name[button], "back-control-game"); //NOTE: DUP (back) - Also add "-hover.png" ...
		}
	}
	

	//Loading screen
	if (orgnode == 2)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true)
				{
					if (strcmp(Storyboard.Nodes[i].lua_name, "loading.lua") == 0)
					{
						//Found it and its active.
						bValid = false;
						node = i;
						//PE: Fix old bug where loading screen was set to wrong type.
						if (Storyboard.Nodes[node].type != STORYBOARD_TYPE_LEVEL)
							Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
						break;
					}
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}

		//PE: Check if current project is missing the new text and inject it.
		if (!bValid && Storyboard.Nodes[node].used == true && !bForce )
		{
			//PE: Inject new text into current projects, where its missing.
			int button = 2;
			if (Storyboard.Nodes[node].widget_used[button] != 1)
			{
				strcpy(Storyboard.Nodes[node].widget_label[button], "When in game, press the Escape key for controls and other settings.");
				Storyboard.Nodes[node].widget_used[button] = 1;
				Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
				Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
				Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 95.0); //Pos in percent. using pivot center on X only.
				Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
				Storyboard.Nodes[node].widget_layer[button] = 0;
				Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
				Storyboard.Nodes[node].widget_font_size[button] = 0.5;
				strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
				strcpy(Storyboard.Nodes[node].widget_name[button], "loading-text"); //Also add "-hover.png" ...
			}
		}

		//2 Default Loading screen.
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].widgets_available = ALLOW_TEXT | ALLOW_IMAGE | ALLOW_VIDEO;
			//iLoadingScreenNodeID;
			//2 Default Loading screen.
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
			if (bRestoring == false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 0);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "Loading Screen");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_loading.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "loading.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\loading.png");
			strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
			strcpy(Storyboard.Nodes[node].output_title[0], " LOAD LEVEL -> Connect to Level ");
			strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
			Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_LEVEL;
			int button = 0;
			strcpy(Storyboard.Nodes[node].widget_label[button], "LOADING LEVEL");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 80.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_name[button], "loading-text"); //Also add "-hover.png" ...
			button = 1;
			strcpy(Storyboard.Nodes[node].widget_label[button], ""); //Progressbar
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_PROGRESS;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 90.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
			strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");
			button = 2;
			strcpy(Storyboard.Nodes[node].widget_label[button], "When in game, press the Escape key for controls and other settings.");
			Storyboard.Nodes[node].widget_used[button] = 1;
			Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
			Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
			Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 95.0); //Pos in percent. using pivot center on X only.
			Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
			Storyboard.Nodes[node].widget_layer[button] = 0;
			Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
			Storyboard.Nodes[node].widget_font_size[button] = 0.5;
			strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
			strcpy(Storyboard.Nodes[node].widget_name[button], "loading-text"); //Also add "-hover.png" ...
		}
	}

	// In-Game HUD
	if (orgnode == 13)
	{
		bool bValid = true;
		if (!bForce)
		{
			//Do we have this node ?
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used == true && strcmp(Storyboard.Nodes[i].title, "In-Game HUD") == 0)
				{
					//Found it and its active.
					bValid = false;
					node = i;
					break;
				}
			}
			if (bValid)
			{
				//Find first free node and use that. start from 8
				for (int i = 8; i < STORYBOARD_MAXNODES; i++)
				{
					if (Storyboard.Nodes[i].used == false)
					{
						node = i;
						break;
					}
				}
			}
		}
		// 13 Default In-Game HUD screen.
		if (bValid && (Storyboard.Nodes[node].used == false || bForce))
		{
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_HUD;
			if (bRestoring == false) Storyboard.Nodes[node].restore_position = ImVec2(area_width*0.5 - (node_width*0.5), STORYBOARD_YSTART + (node_height + NODE_HEIGHT_PADDING) * 3);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "In-Game HUD");
			strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\hud.lua.png");
			strcpy(Storyboard.Nodes[node].lua_name, "hud0.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "");
			Storyboard.Nodes[node].screen_backdrop_transparent = true;
			Storyboard.Nodes[node].widgets_available = allWidgets;
			Storyboard.Nodes[node].readouts_available = READOUT_GAMEPLAY;

			// Storyboard does not yet have a HUD screen, so add one (copy from default template HUD in below filepath)
			// In future, any screen default states should be saved into this file
			const char* filepath = "editors\\templates\\ScreenEditor\\project.dat";
			bool load__storyboard_into_struct(const char*, StoryboardStruct&);
			if (load__storyboard_into_struct(filepath, tempProjectData))
			{
				StoryboardNodesStruct& thisNode = Storyboard.Nodes[node];
				StoryboardNodesStruct& source = tempProjectData.Nodes[orgnode];
				for (int j = 0; j < STORYBOARD_MAXWIDGETS; j++)
				{
					thisNode.widget_used[j] = source.widget_used[j];
					strcpy(thisNode.widget_label[j], source.widget_label[j]);
					thisNode.widget_size[j] = source.widget_size[j];
					thisNode.widget_pos[j] = source.widget_pos[j];
					strcpy(thisNode.widget_normal_thumb[j], source.widget_normal_thumb[j]);
					strcpy(thisNode.widget_highlight_thumb[j], source.widget_highlight_thumb[j]);
					strcpy(thisNode.widget_selected_thumb[j], source.widget_selected_thumb[j]);
					strcpy(thisNode.widget_click_sound[j], source.widget_click_sound[j]);
					thisNode.widget_action[j] = source.widget_action[j];
					strcpy(thisNode.widget_font[j],source.widget_font[j]);
					thisNode.widget_font_color[j] = source.widget_font_color[j];
					thisNode.widget_font_size[j] = source.widget_font_size[j];
					thisNode.widget_type[j] = source.widget_type[j];
					thisNode.widget_read_only[j] = source.widget_read_only[j];
					thisNode.widget_layer[j] = source.widget_layer[j];
					thisNode.widget_initial_value[j] = source.widget_initial_value[j];
					strcpy(thisNode.widget_name[j], source.widget_name[j]);
					Storyboard.widget_colors[node][j] = tempProjectData.widget_colors[orgnode][j];
					strcpy(Storyboard.widget_readout[node][j], tempProjectData.widget_readout[orgnode][j]);
					Storyboard.widget_textoffset[node][j] = tempProjectData.widget_textoffset[orgnode][j];
					Storyboard.widget_ingamehidden[node][j] = tempProjectData.widget_ingamehidden[orgnode][j];
					Storyboard.widget_drawordergroup[node][j] = tempProjectData.widget_drawordergroup[orgnode][j];
				}
			}
		}
	}

	if (node == iLoadingScreenNodeID)
		Storyboard.Nodes[node].widgets_available = ALLOW_TEXT | ALLOW_IMAGE | ALLOW_VIDEO;
	else
		Storyboard.Nodes[node].widgets_available = defaultWidgets;
	
	if (strcmp(Storyboard.Nodes[node].title, "In-Game HUD") == 0)
	{
		Storyboard.Nodes[node].readouts_available = READOUT_GAMEPLAY;
	}
	else
	{
		Storyboard.Nodes[node].readouts_available = 0;
	}
	
	return(node);
}

int ImGui_GetWindowOrder(ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	for (int i = g.WindowsFocusOrder.Size - 1; i >= 0; i--)
		if (g.WindowsFocusOrder[i] == window)
			return i;
	return(-1);
}

int iZOrderIsSorting = 0;
void CheckWindowsOnTop(ImGuiWindow* storyboard_window)
{
	//PE: We need special attension on window that need to be placed on top of storyboard as its fullscreen.
	if (iZOrderIsSorting > 0)
	{
		iZOrderIsSorting--;
		return;
	}

	int storyboard_idx = ImGui_GetWindowOrder(storyboard_window);
	if (storyboard_idx < 0) return;
	int secondscreen_idx = -1;

	//PE: From Small Tutorials Videos, check this directly as there can be many, but only one at the same time.
	if (bLastSmallVideoPlayerMaximized)
	{
		ImGuiWindow* win = ImGui::FindWindowByName("Tutorial Video##VideosMaxSize");
		if (win)
		{
			bool bSwitchOrder = false;
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx) bSwitchOrder = true;
			if (bSwitchOrder)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "Tutorial Video##VideosMaxSize");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
		}
	}


	if (bAbout_Window)
	{

		
		ImGuiWindow* win = ImGui::FindWindowByName("##Credits##AboutWindow");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "##Credits##AboutWindow");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
			secondscreen_idx = settings_idx;
		}

		win = ImGui::FindWindowByName("##About##AboutWindow");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "##About##AboutWindow");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
			secondscreen_idx = settings_idx;
		}
	}

	if (bPreferences_Window)
	{
		ImGuiWindow* win = ImGui::FindWindowByName("Settings");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "Settings");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
			secondscreen_idx = settings_idx;
		}
	}

	if (bEditGameSettings)
	{
		ImGuiWindow* win = ImGui::FindWindowByName("Edit Game Settings##Storyboard");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "Edit Game Settings##Storyboard");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
			secondscreen_idx = settings_idx;
		}
	}
	//"Edit Game Settings##Storyboard"

	if (bExternal_Entities_Window)
	{
		ImGuiWindow* win = ImGui::FindWindowByName("##Object Library ExternalWindow");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "##Object Library ExternalWindow");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
			secondscreen_idx = settings_idx;
		}
	}

	if (bInfo_Window)
	{
		ImGuiWindow* win = ImGui::FindWindowByName("Information##InformationWindow");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "Information##InformationWindow");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
			secondscreen_idx = settings_idx;
		}
	}

	//PE: Tutorial video.
	if (bVideoPlayerMaximized)
	{
		ImGuiWindow* win = ImGui::FindWindowByName("Tutorial Video##Videos2MaxSize");
		if (win)
		{
			int settings_idx = ImGui_GetWindowOrder(win);
			if (settings_idx < storyboard_idx)
			{
				//Focus settings
				strcpy(cNextWindowFocus, "Tutorial Video##Videos2MaxSize");
				iSkibFramesBeforeLaunch = 2;
				iZOrderIsSorting = iSkibFramesBeforeLaunch + 2;
				iLaunchAfterSync = 81; //Delayed window focus.
			}
		}
	}
}

bool bPopModalStoryboard = false;
bool bStoryboardWindowLast = false;
int iLastPrefStyle = -1;
int iWaitForNewLevel = 0;
int iWaitForNewScreenshot = 0;
int iWaitFor2DEditor = 0;
int iWaitFor2DEditorNode = -1;
int iNewLevelNode = -1;
int iScreenshotNode = -1;
bool bDuplicateLevel = false;
int iDuplicateNode = false;
bool bRenameLevel = false;
int iRenameNode = false;

bool bBlockNextMouseCheck = false;
int iFakeLoadGameTest = 0;
bool bStartLoadingGame = false;
int iExecuteMenuCommand = 0;
static int iCurrentSelectedWidget = -1;
static bool bTestStandalone = false;

#define INCLUDE_GAME_SETTINGS

int process_createanewhudscreen(int iStartAt)
{
	int iLastKnownNode = -1;
	int hudScreenCount = iStartAt;// 2;
	if (hudScreenCount < 2) hudScreenCount = 2;
	while (hudScreenCount < 2 + STORYBOARD_MAXNODES)
	{
		char pTryHUDScreenName[256];
		sprintf(pTryHUDScreenName, "HUD Screen %d", hudScreenCount);
		bool bHUDScreenExists = false;
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used && stricmp(Storyboard.Nodes[i].title, pTryHUDScreenName) == NULL)
			{
				bHUDScreenExists = true;
				iLastKnownNode = i;
				break;
			}
		}
		if (bHUDScreenExists == false)
		{
			// found next available HUD Screen number
			break;
		}
		else
		{
			// try next one
			hudScreenCount++;
		}
	}

	// the HUD ID
	char cHudCount[8];
	sprintf_s(cHudCount, "%d", hudScreenCount);

	// Find first free storyboard node that we can use for the new screen.
	int node = -1;
	int iUniqueIdsAdd = 1000;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (i == 100 || i == 200)
			iUniqueIdsAdd += 100000;

		if (Storyboard.Nodes[i].used == 0)
		{
			// Reset node to default state, in case any old data remains.
			node = i;
			reset_single_node(node);

			//PE: Setup new unique id's
			int iUniqueId = STORYBOARD_THUMBS + node;
			Storyboard.Nodes[node].id = iUniqueId;
			Storyboard.Nodes[node].thumb_id = iUniqueId;
			for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
			{
				//PE: input_id,output_id ID's broken in checkproject.
				Storyboard.Nodes[node].widget_normal_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 600;
				Storyboard.Nodes[node].widget_highlight_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 700;
				Storyboard.Nodes[node].widget_selected_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 800;
			}
			for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
			{
				Storyboard.Nodes[node].input_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l);
				Storyboard.Nodes[node].output_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 500;
			}

			Storyboard.Nodes[node].screen_backdrop_id = iUniqueId + 500;

			// New node defaults to a HUD screen
			Storyboard.Nodes[node].used = true;
			Storyboard.Nodes[node].type = STORYBOARD_TYPE_HUD;

			// locate new screen next to last known HUD screen
			if (iLastKnownNode >= 0)
			{
				Storyboard.Nodes[node].restore_position = ImVec2(Storyboard.Nodes[iLastKnownNode].restore_position.x + 20, Storyboard.Nodes[iLastKnownNode].restore_position.y + 20);
			}
			else
			{
				Storyboard.Nodes[node].restore_position = ImVec2(Storyboard.Nodes[iHUDScreenNodeID].restore_position.x + 200 * hudScreenCount, Storyboard.Nodes[iHUDScreenNodeID].restore_position.y);
			}
			ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);
			Storyboard.Nodes[node].iEditEnable = true;
			strcpy(Storyboard.Nodes[node].title, "HUD Screen ");
			strcat(Storyboard.Nodes[node].title, cHudCount);
			strcpy(Storyboard.Nodes[node].lua_name, "hud.lua");
			strcpy(Storyboard.Nodes[node].screen_backdrop, "");
			Storyboard.Nodes[node].screen_backdrop_transparent = true;
			Storyboard.Nodes[node].widgets_available = ALLOW_TEXT | ALLOW_TEXTAREA | ALLOW_IMAGE | ALLOW_BUTTON;
			Storyboard.Nodes[node].readouts_available = READOUT_GAMEPLAY | READOUT_GRAPHICS | READOUT_INPUT | READOUT_SOUND;
			break;
		}
	}
	return node;
}

