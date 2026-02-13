char * imgui_setpropertylist2c(int group, int controlindex, char* data_s, char* field_s, char* desc_s, int listtype)
{
	cstr ldata_s = data_s, ldesc_s = desc_s, lfields_s = field_s;

	if (cstr(data_s) == "" || !data_s)  ldata_s = "";
	if (cstr(desc_s) == "" || !desc_s)  ldesc_s = "";
	if (cstr(field_s) == "" || !field_s)  lfields_s = "";

	int current_selection = atoi(ldata_s.Get());

	int listmax = 0;
	listmax = 0;
	if (listtype == 0)
	{
		listmax = 1;
		t.list_s[0] = t.strarr_s[471];
		t.list_s[1] = t.strarr_s[470];
	}
	if (listtype == 1)
	{
		listmax = fillgloballistwithweaponsQuick(false, true, true, false);
		for (int n = 0; n <= listmax; n++)
		{
			if (ldata_s == t.list_s[n]) 
			{
				current_selection = n;
				break;
			}
		}
	}
	if (listtype == 11)
	{
		listmax = fillgloballistwithbehaviours();
		for (int n = 0; n <= listmax; n++)
		{
			if (ldata_s == t.list_s[n]) 
			{
				current_selection = n;
				break;
			}
		}
	}
	if (listtype == 21)
	{
		listmax = fillgloballistwithcollectables();
		for (int n = 0; n <= listmax; n++)
		{
			if (ldata_s == t.list_s[n])
			{
				current_selection = n;
				break;
			}
		}
	}

	const char* current_item = t.list_s[current_selection].Get();

	std::string uniquiField = ""; //lfields_s.Get()
	uniquiField = uniquiField + "##" + lfields_s.Get();
	uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);


	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
	ImGui::Text(lfields_s.Get());
	ImGui::SameLine();
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
	ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));

	ImGui::PushItemWidth(-10);

	if (ImGui::BeginCombo(uniquiField.c_str() , current_item)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n <= listmax; n++)
		{
			bool is_selected = (current_item == t.list_s[n].Get()); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(t.list_s[n].Get(), is_selected)) {
				current_selection = n;
				current_item = t.list_s[n].Get();
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	
	ImGui::PopItemWidth();
	return t.list_s[current_selection].Get();
}

int imgui_setpropertylist2(int group, int controlindex, char* data_s, char* field_s, char* desc_s, int listtype)
{
	cstr ldata_s = data_s, ldesc_s = desc_s, lfields_s = field_s;

	if (cstr(data_s) == "" || !data_s)  ldata_s = "";
	if (cstr(desc_s) == "" || !desc_s)  ldesc_s = "";
	if (cstr(field_s) == "" || !field_s)  lfields_s = "";

	int current_selection = atoi(ldata_s.Get());


	int listmax = 0;

	listmax = 0;
	if (listtype == 0)
	{
		listmax = 1;
		t.list_s[0] = t.strarr_s[471];
		t.list_s[1] = t.strarr_s[470];
	}
	if (listtype == 1)
	{
		listmax = fillgloballistwithweapons();
	}
	if (listtype == 11)
	{
		listmax = fillgloballistwithbehaviours();
	}
	if (listtype == 21)
	{
		listmax = fillgloballistwithcollectables();
	}

	const char* current_item = t.list_s[current_selection].Get();
	
	std::string uniquiField = ""; //lfields_s.Get()
	uniquiField = uniquiField + "##" + lfields_s.Get();
	uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);

	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
	ImGui::Text(lfields_s.Get());
	ImGui::SameLine();
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
	ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));

	ImGui::PushItemWidth(-10);

	if (ImGui::BeginCombo(uniquiField.c_str() , current_item)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n <= listmax; n++)
		{
			bool is_selected = (current_item == t.list_s[n].Get() ); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(t.list_s[n].Get(), is_selected)) {
				current_selection = n;
				current_item = t.list_s[n].Get();
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", ldesc_s.Get() );

	ImGui::PopItemWidth();
	return current_selection;
}


void startgroup ( char* s_s )
{
	if (  cstr(s_s) == ""  )  s_s = "";
	SetFileMapDWORD (  3,g.g_filemapoffset,2  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(s_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,s_s  ); g.g_filemapoffset += ((Len(s_s)+3)/4 )*4;
}

void endgroup ( void )
{
	SetFileMapDWORD (  3,g.g_filemapoffset,0  ); g.g_filemapoffset += 4;
}

void setpropertystring2 ( int group, char* data_s, char* field_s, char* desc_s )
{
	if (  cstr(data_s) == ""  )  data_s = "";
	SetFileMapDWORD (  3,g.g_filemapoffset,3  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,group  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(field_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,field_s  ); g.g_filemapoffset += ((Len(field_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(data_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,data_s  ); g.g_filemapoffset += ((Len(data_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(desc_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,desc_s  ); g.g_filemapoffset += ((Len(desc_s)+3)/4 )*4;
}

void setpropertycolor2 ( int group, int dataval, char* field_s, char* desc_s )
{
	cstr data_s =  "";
	data_s=data_s+Str(RgbR(dataval))+" "+Str(RgbG(dataval))+" "+Str(RgbB(dataval));
	SetFileMapDWORD (  3,g.g_filemapoffset,4  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,group  );  g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(field_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,field_s ); g.g_filemapoffset += ((Len(field_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(data_s.Get())  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,data_s.Get()); g.g_filemapoffset += ((Len(data_s.Get())+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(desc_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,desc_s  ); g.g_filemapoffset += ((Len(desc_s)+3)/4 )*4;
//endfunction

}

void setpropertyfile2 ( int group, char* data_s, char* field_s, char* desc_s, char* within_s )
{
	cstr s_s =  "";
	if (  cstr(data_s) == ""  )  data_s = "";
	SetFileMapDWORD (  3,g.g_filemapoffset,5  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,group  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(field_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,field_s  ); g.g_filemapoffset += ((Len(field_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(data_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,data_s  ); g.g_filemapoffset += ((Len(data_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(desc_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,desc_s ); g.g_filemapoffset += ((Len(desc_s)+3)/4 )*4;
	s_s = g.rootdir_s+within_s;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(s_s.Get())  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,s_s.Get()  ); g.g_filemapoffset += ((Len(s_s.Get())+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(t.strarr_s[321].Get())  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,t.strarr_s[321].Get() ); g.g_filemapoffset += ((Len(t.strarr_s[321].Get())+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(t.strarr_s[322].Get()) ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,t.strarr_s[322].Get() ); g.g_filemapoffset += ((Len(t.strarr_s[322].Get())+3)/4 )*4;
//endfunction

}

void setpropertylist2 ( int group, int controlindex, char* data_s, char* field_s, char* desc_s, int listtype )
{
	int listmax = 0;
	if ( strcmp ( data_s , "" ) == 0  )  strcpy ( data_s , "" );
	if (  listtype == 0 ) 
	{
		//  yesno
		if (  strcmp ( data_s , "0" ) == 0  )  strcpy ( data_s , t.strarr_s[471].Get() );
		if (  strcmp ( data_s , "1" ) == 0  )  strcpy ( data_s , t.strarr_s[470].Get() );
	}
	if (  listtype == 11 ) 
	{
		//  behaviours (trim scriptbank behaviours and .fpi)
		strcpy ( data_s , Right(data_s,Len(data_s)-Len("behavioursx")) );
		strcpy ( data_s , Left(data_s,Len(data_s)-4) );
		t.strwork = "" ; t.strwork = t.strwork + Upper(Left(data_s,1))+Lower(Right(data_s,Len(data_s)-1));
		strcpy ( data_s , t.strwork.Get() );
	}
	SetFileMapDWORD (  3,g.g_filemapoffset,6  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,group  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,controlindex  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(field_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,field_s  ); g.g_filemapoffset += ((Len(field_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(data_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,data_s  ); g.g_filemapoffset += ((Len(data_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(desc_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,desc_s  ); g.g_filemapoffset += ((Len(desc_s)+3)/4 )*4;
	listmax=0;
	if (  listtype == 0 ) 
	{
		listmax=1;
		t.list_s[0]=t.strarr_s[471];
		t.list_s[1]=t.strarr_s[470];
	}
	if (  listtype == 1 ) 
	{
		listmax=fillgloballistwithweapons();
	}
	if (  listtype == 11 ) 
	{
		listmax=fillgloballistwithbehaviours();
	}
	SetFileMapDWORD (  3,g.g_filemapoffset,listmax  ); g.g_filemapoffset += 4;
	for ( int i = 0 ; i<=  listmax; i++ )
	{
		SetFileMapDWORD (  3,g.g_filemapoffset,Len(t.list_s[ i ].Get())  ); g.g_filemapoffset += 4;
		SetFileMapString (  3,g.g_filemapoffset,t.list_s[ i ].Get()  ); g.g_filemapoffset += ((Len(t.list_s[ i ].Get())+3)/4 )*4;
	}
//endfunction

}

void setpropertylist3 ( int group, int controlindex, char* data_s, char* field_s, char* desc_s, int listtype )
{
	int listmax = 0;
	if (  strcmp ( data_s , "" ) == 0  )  strcpy ( data_s , "" );
	if (  strcmp ( data_s , "0" ) == 0  )  strcpy ( data_s , "No" );
	if (  strcmp ( data_s , "1" ) == 0  )  strcpy ( data_s , "A" );
	if (  strcmp ( data_s , "2" ) == 0  )  strcpy ( data_s , "B" );
	SetFileMapDWORD (  3,g.g_filemapoffset,6  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,group  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,controlindex  ); g.g_filemapoffset += 4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(field_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,field_s  ); g.g_filemapoffset += ((Len(field_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(data_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,data_s  ); g.g_filemapoffset += ((Len(data_s)+3)/4 )*4;
	SetFileMapDWORD (  3,g.g_filemapoffset,Len(desc_s)  ); g.g_filemapoffset += 4;
	SetFileMapString (  3,g.g_filemapoffset,desc_s  ); g.g_filemapoffset += ((Len(desc_s)+3)/4 )*4;
	listmax=2;
	Dim (  t.list_s,2  );
	t.list_s[0]="No";
	t.list_s[1]="A";
	t.list_s[2]="B";
	SetFileMapDWORD (  3,g.g_filemapoffset,listmax  ); g.g_filemapoffset += 4;
	for ( int i = 0 ; i<=  listmax; i++ )
	{
		SetFileMapDWORD (  3,g.g_filemapoffset,Len(t.list_s[ i ].Get())  ); g.g_filemapoffset += 4;
		SetFileMapString (  3,g.g_filemapoffset,t.list_s[ i ].Get()  ); g.g_filemapoffset += ((Len(t.list_s[ i ].Get())+3)/4 )*4;
	}
//endfunction

}

void setpropertybase ( int code, char*  s_s )
{
	if ( strcmp ( s_s , "" ) == 0  )  strcpy ( s_s , "" );
	SetFileMapString (  2, STRING_A, s_s );
	SetFileMapString (  2, STRING_B, "" );
	SetFileMapString (  2, STRING_C, "" );
	SetFileMapDWORD (  2, code, 1 );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, code )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
//endfunction

}

void setpropertystring ( int group, char* data_s, char* field_s, char* desc_s )
{
	if (  strcmp ( data_s , "" )  )  strcpy ( data_s , "" );
	SetFileMapString (  2, STRING_A, field_s );
	SetFileMapString (  2, STRING_B, data_s );
	SetFileMapString (  2, STRING_C, desc_s );
	SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
	SetFileMapDWORD (  2, ENTITY_ADD_EDIT_BOX, 1 );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, ENTITY_ADD_EDIT_BOX )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
//endfunction

}

void setpropertycolor ( int group, int dataval, char* field_s, char* desc_s )
{
	cstr data_s =  "";
	data_s=data_s+Str(RgbR(dataval))+" "+Str(RgbG(dataval))+" "+Str(RgbB(dataval));
	SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
	SetFileMapString (  2, STRING_A, field_s );
	SetFileMapString (  2, STRING_B, data_s.Get() );
	SetFileMapString (  2, STRING_C, desc_s );
	SetFileMapDWORD (  2, ENTITY_ADD_COLOR_PICKER, 1 );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, ENTITY_ADD_COLOR_PICKER )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
//endfunction

}

void setpropertyfile ( int group, char* data_s, char* field_s, char* desc_s, char* within_s )
{
	if ( strcmp ( data_s , "" ) == 0 )  strcpy ( data_s , "" );
	SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
	SetFileMapString (  2, STRING_A, field_s );
	SetFileMapString (  2, STRING_B, data_s );
	SetFileMapString (  2, STRING_C, desc_s );
	t.strwork = "" ; t.strwork = t.strwork + g.rootdir_s+within_s;
	SetFileMapString (  2, 2024, t.strwork.Get() );
	SetFileMapString (  2, 2280, t.strarr_s[321].Get() );
	SetFileMapString (  2, 2536, t.strarr_s[322].Get() );
	SetFileMapDWORD (  2, ENTITY_ADD_FILE_PICKER, 1 );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, ENTITY_ADD_FILE_PICKER )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
}

int FillWeaponList(std::vector<std::string>& labels, char *filter)
{
	int listmax = fillgloballistwithweaponsQuick(true, true, true, false);
	int iWeaponListIndex = 0;
	if (filter)
	{
		std::string label = "No " + std::string(filter);
		labels.push_back(label.c_str());
	}
	else
	{
		labels.push_back("No Weapon");
	}
	iWeaponListIndex++;
	for (int gunid = 1; gunid <= g.gunmax; gunid++)
	{
		cstr thisLabel = t.gun[gunid].name_s;
		bool bAdd = true;
		if (filter && !pestrcasestr(thisLabel.Get(), filter))
			bAdd = false;
		if (bAdd)
		{
			if (strlen(thisLabel.Get()) == 0) thisLabel = "No Weapon";
			labels.push_back(thisLabel.Get());
			iWeaponListIndex++;
		}
	}

	return(iWeaponListIndex);
}

//PE: Cache available weapons , this list is really killing the fps.
std::vector <cstr> cached_weapon_list[2][2][2][2][2];
int cached_weapon_list_size[2][2][2][2][2];

int fillgloballistwithweaponsQuick(bool forcharacters, bool bForShooting, bool bForMelee, bool bIncludeSlotNotUsedChoice)
{
	if (cached_weapon_list[forcharacters][bForShooting][bForMelee][bIncludeSlotNotUsedChoice][g_bCharacterCreatorPlusActivated].size() > 0)
	{
		t.list_s = cached_weapon_list[forcharacters][bForShooting][bForMelee][bIncludeSlotNotUsedChoice][g_bCharacterCreatorPlusActivated];
		return cached_weapon_list_size[forcharacters][bForShooting][bForMelee][bIncludeSlotNotUsedChoice][g_bCharacterCreatorPlusActivated];
	}

	int retvalue = 0;
	int gunid = 0;
	Dim(t.list_s, 1 + g.gunmax);

	// when shooting and melee both true, ALL weapons can be chosen (or none)
	bool bForAll = false;
	if (bForShooting == true && bForMelee == true)
		bForAll = true;

	// For drop down , so quick with no file checks.
	if (bForAll==false && forcharacters==true && bForShooting == true && g_bCharacterCreatorPlusActivated == false)
	{
		// when using shooting weapon cannot have No Weapon (use different behavior if want that)
		t.list_s[0] = "enhanced\\Mk19T"; // standard issue pistol
	}
	else
	{
		t.list_s[0] = "";
	}
	int iListCount = 0;
	for (gunid = 1; gunid <= g.gunmax; gunid++)
	{
		bool bIncludeThisWeapon = false;
		if (bForAll == true)
		{
			// all weapons should be listed
			bIncludeThisWeapon = true;
		}
		else
		{
			if (bForShooting == true)
			{
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\AK") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\AR") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\M29S") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\Mk18") == NULL) bIncludeThisWeapon = true;
				if (forcharacters == true && bForShooting == true && g_bCharacterCreatorPlusActivated == false)
				{
					// already included above as the default
				}
				else
				{
					if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\Mk19T") == NULL) bIncludeThisWeapon = true;
				}
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\R870") == NULL) bIncludeThisWeapon = true;
			}
			if (bForMelee == true)
			{
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\B810") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\Gloves_Unarmed") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\SledgeHammer") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "aztec\\AztecAxe") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "aztec\\AztecDagger") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "aztec\\AztecSpear") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "tools\\Hammer") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "tools\\Handsaw") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "tools\\Shovel") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "wasteland\\tools\\Crowbar") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "wasteland\\tools\\LargeScrewdriver") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "wasteland\\tools\\LargeSpanner") == NULL) bIncludeThisWeapon = true;
				if (stricmp(t.gun[gunid].name_s.Get(), "wasteland\\tools\\LumpHammer") == NULL) bIncludeThisWeapon = true;
				if (strnicmp(t.gun[gunid].name_s.Get(), "wasteland\\weapons\\", 18) == NULL) bIncludeThisWeapon = true;
			}
			if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\M67") == NULL) bIncludeThisWeapon = false;
			if (forcharacters == true)
			{
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\B810") == NULL) bIncludeThisWeapon = false;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\Gloves_Unarmed") == NULL) bIncludeThisWeapon = false;
				if (stricmp(t.gun[gunid].name_s.Get(), "enhanced\\SledgeHammer") == NULL) bIncludeThisWeapon = false;
			}
			// any another category should be revealed for all cases (restrict later as new items come in - via gunspec!)
			if (strnicmp(t.gun[gunid].name_s.Get(), "enhanced\\", 9) != NULL && strnicmp(t.gun[gunid].name_s.Get(), "aztec\\", 6) != NULL
				&& strnicmp(t.gun[gunid].name_s.Get(), "wasteland\\", 10) != NULL && strnicmp(t.gun[gunid].name_s.Get(), "tools\\", 6) != NULL)
			{
				bIncludeThisWeapon = true;
			}
		}
		if (bIncludeThisWeapon == true)
		{
			if ( stricmp ( t.gun[gunid].name_s.Get(), "Slot Not Used" ) == NULL )
			{
				if (bIncludeSlotNotUsedChoice == true)
				{
					iListCount++;
					t.list_s[iListCount] = "00000Slot Not Used";
				}
			}
			else
			{
				iListCount++;
				t.list_s[iListCount] = t.gun[gunid].name_s;
			}
		}
	}

	// 190416 - sort weapons into alpha order
	for (int iSortA = 0; iSortA <= iListCount; iSortA++)
	{
		for (int iSortB = 0; iSortB <= iListCount; iSortB++)
		{
			if (iSortA != iSortB && strcmp(t.list_s[iSortA].Get(), t.list_s[iSortB].Get()) < 0)
			{
				// swap over for bubble sort
				cstr pStoreA = t.list_s[iSortA];
				t.list_s[iSortA] = t.list_s[iSortB];
				t.list_s[iSortB] = pStoreA;
			}
		}
	}

	// rename any  "Slot Not Used"
	for (int iFindA = 0; iFindA <= iListCount; iFindA++)
	{
		if(stricmp(t.list_s[iFindA].Get(), "00000Slot Not Used") == NULL)
		{
			t.list_s[iFindA] = "Slot Not Used";
			break;
		}
	}

	cached_weapon_list[forcharacters][bForShooting][bForMelee][bIncludeSlotNotUsedChoice][g_bCharacterCreatorPlusActivated] = t.list_s;
	cached_weapon_list_size[forcharacters][bForShooting][bForMelee][bIncludeSlotNotUsedChoice][g_bCharacterCreatorPlusActivated] = iListCount;
	// return valid gun name count
	return iListCount;
}

std::vector<cstr> g_HandsList_s;

int fillgloballistwithHands(void)
{
	// initially populate from Hands folder
	if (g_HandsList_s.size() == 0)
	{
		cstr pOld = GetDir();
		SetDir(cstr(g.fpscrootdir_s+cstr("\\Files\\gamecore\\Hands\\")).Get());
		ChecklistForFiles();
		for (int c = 1; c <= ChecklistQuantity(); c++)
		{
			if (ChecklistValueA(c) != 0)
			{
				cstr folder = ChecklistString(c);
				if (folder != "." && folder != ".." && folder != "Animations")
				{
					g_HandsList_s.push_back(folder.Get());
				}
			}
		}
		SetDir(pOld.Get());
	}
	int iHandsCount = g_HandsList_s.size();
	Dim(t.list_s, iHandsCount);
	int iIndex = 0;
	t.list_s[iIndex++] = "No Preference";
	for (int i = 0; i < iHandsCount; i++)
	{
		t.list_s[iIndex++] = g_HandsList_s[i];
	}
	return iIndex-1;
}

bool g_bCheckedBoosterAnims = false;
bool g_bDoWeHaveBoosterAnims = false;

int fillgloballistwithCharAnimSetsQuick(int iSpecialValue)
{
	// one time check to see if booster anims available
	if (g_bCheckedBoosterAnims == false)
	{
		if (FileExist("charactercreatorplus\\animations\\sets\\adult male\\default animations-pistol-lowered.dbo"))
		{
			g_bDoWeHaveBoosterAnims = true;
		}
		g_bCheckedBoosterAnims = true;
	}

	// create list of available animations for standard user dropdown
	Dim(t.list_s, 10);
	int iIndex = 0;
	t.list_s[iIndex] = "Default Animation";
	iIndex++; t.list_s[iIndex] = "Original Animation";
	if (iSpecialValue == 1)
	{
		// adult male soldier
		iIndex++; t.list_s[iIndex] = "Adult Male Pistol";
		iIndex++; t.list_s[iIndex] = "Adult Male Rifle";
		if (g_bDoWeHaveBoosterAnims)
		{
			iIndex++; t.list_s[iIndex] = "Adult Male Pistol Lowered";
			iIndex++; t.list_s[iIndex] = "Adult Male Rifle Lowered";
			iIndex++; t.list_s[iIndex] = "Adult Male Shotgun Lowered";
		}
	}
	if (iSpecialValue == 2)
	{
		// adult male melee
		iIndex++; t.list_s[iIndex] = "Adult Male Melee";
		iIndex++; t.list_s[iIndex] = "Adult Male Axe";
		iIndex++; t.list_s[iIndex] = "Adult Male Spear";
	}
	if (iSpecialValue == 3)
	{
		// adult female soldier
		iIndex++; t.list_s[iIndex] = "Adult Female Pistol";
		iIndex++; t.list_s[iIndex] = "Adult Female Rifle";
		if (g_bDoWeHaveBoosterAnims)
		{
			iIndex++; t.list_s[iIndex] = "Adult Female Pistol Lowered";
			iIndex++; t.list_s[iIndex] = "Adult Female Rifle Lowered";
			iIndex++; t.list_s[iIndex] = "Adult Female Shotgun Lowered";
		}
	}
	if (iSpecialValue == 4)
	{
		// adult female melee
		iIndex++; t.list_s[iIndex] = "Adult Female Melee";
		iIndex++; t.list_s[iIndex] = "Adult Female Axe";
		iIndex++; t.list_s[iIndex] = "Adult Female Spear";
	}
	if (iSpecialValue == 5)
	{
		// zombie male
		iIndex++; t.list_s[iIndex] = "Zombie Male";
	}
	if (iSpecialValue == 6)
	{
		// zombie female
		iIndex++; t.list_s[iIndex] = "Zombie Female";
	}
	if (iSpecialValue == 7)
	{
		// low poly melee
		iIndex++; t.list_s[iIndex] = "Low Poly Melee";
		iIndex++; t.list_s[iIndex] = "Low Poly Axe";
		iIndex++; t.list_s[iIndex] = "Low Poly Spear";
	}
	// return count
	return iIndex;
}

int fillgloballistwithweapons ( void )
{
	int retvalue = 0;
	int gunid = 0;
	Dim (  t.list_s,1+g.gunmax  );
	t.list_s[0] = "";
	int iListCount = 0; // 020316 - v1.13b1 - list can exclude player weapons with no HUDs
	for ( gunid = 1; gunid <= g.gunmax; gunid++ )
	{
		// 020316 - v1.13b1 - quickly check the existence of the HUD.X file to see if we exclude (later change when weapon changes for characters)
		t.tfile_s = cstr("gamecore\\guns\\") + t.gun[gunid].name_s + cstr("\\HUD.X");
		if ( FileExist(t.tfile_s.Get()) == 1 ) 
		{
			iListCount++;
			t.list_s[iListCount] = t.gun[gunid].name_s;
		}
	}

	// 190416 - sort weapons into alpha order
	for ( int iSortA = 0; iSortA <= iListCount; iSortA++ )
	{
		for ( int iSortB = 0; iSortB <= iListCount; iSortB++ )
		{
			if ( iSortA != iSortB && strcmp ( t.list_s[iSortA].Get(), t.list_s[iSortB].Get() ) < 0 )
			{
				// swap over for bubble sort
				cstr pStoreA = t.list_s[iSortA];
				t.list_s[iSortA] = t.list_s[iSortB];
				t.list_s[iSortB] = pStoreA;
			}
		}
	}

	// return valid gun name count
	return iListCount;
}

int fillgloballistwithbehaviours_init ( void )
{
	cstr storedir_s =  "";
	int retvalue = 0;
	cstr file_s =  "";
	int c = 0;
	retvalue=0;
	t.strwork = "" ; t.strwork = t.strwork + g.rootdir_s+"scriptbank\\behaviours";
	if (  PathExist( t.strwork.Get() ) == 1 ) 
	{
		storedir_s=GetDir();
		SetDir (  t.strwork.Get() );
		ChecklistForFiles ();
		Dim (  t.behaviourlist_s,ChecklistQuantity( ) );
		for ( c = 1 ; c<=  ChecklistQuantity(); c++ )
		{
			file_s=ChecklistString(c);
			if (  strcmp ( Lower(Right(file_s.Get(),4)) , ".fpi" ) == 0 ) 
			{
				++retvalue;
				t.strwork = "" ; t.strwork=t.strwork+Left(file_s.Get(),Len(file_s.Get())-4);
				file_s = t.strwork;
				t.behaviourlist_s[retvalue] = ""; t.behaviourlist_s[retvalue]=t.behaviourlist_s[retvalue]+Upper(Left(file_s.Get(),1))+Lower(Right(file_s.Get(),Len(file_s.Get())-1));
			}
		}
		SetDir (  storedir_s.Get() );
	}
	return retvalue;
}

int fillgloballistwithbehaviours ( void )
{
	int behaviourlistmax = 0;
	int retvalue = 0;
	int n;
	retvalue=behaviourlistmax;
	Dim (  t.list_s,retvalue  );
	t.list_s[0]="";
	if (  retvalue>0 ) 
	{
		for ( n = 1 ; n<=  retvalue; n++ )
		{
			t.list_s[n-1]=t.behaviourlist_s[n];
		}
		retvalue=retvalue-1;
	}
	return retvalue;
}

int fillgloballistwithcollectables (void)
{
	Dim (t.list_s, g.entityelementlist);
	t.list_s[0] = "(Choose Collectible)";
	int retvalue = 1;
	for (int e = 1; e <= g.entityelementlist; e++)
	{
		if (t.entityelement[e].bankindex > 0)
		{
			if (t.entityelement[e].eleprof.iscollectable > 0)
			{
				t.list_s[retvalue] = t.entityelement[e].eleprof.name_s;
				retvalue++;
			}
		}
	}
	return retvalue;
}

int fillgloballistwithdecals(std::vector <cstr> & list_s)
{
	Dim(list_s, g.decalmax);
	list_s[0] = "None";
	int retvalue = 1;
	for (int i = 1; i <= g.decalmax; i++)
	{
		if (t.decal[i].name_s.Len() > 0)
		{
			list_s[retvalue] = t.decal[i].name_s;
			retvalue++;
		}
	}
	return retvalue;
}

int fillgloballistwithvoices(std::vector <cstr>& list_s)
{
	Dim(list_s, 4);
	list_s[0] = "player";
	int retvalue = 1;
	list_s[retvalue] = "male"; retvalue++;
	list_s[retvalue] = "female"; retvalue++;
	list_s[retvalue] = "custom"; retvalue++;
	return retvalue - 1;
}

void setpropertylist ( int group, int controlindex, char* data_s, char* field_s, char* desc_s, int listtype )
{
	int listmax = 0;
	if ( strcmp ( data_s , "" ) == 0  )  strcpy ( data_s , "" );
	if (  listtype == 0 ) 
	{
		if (  strcmp ( data_s , "0" ) == 0  )  strcpy ( data_s , t.strarr_s[471].Get() );
		if (  strcmp ( data_s , "1" ) == 0  )  strcpy ( data_s , t.strarr_s[470].Get() );
	}
	SetFileMapString (  2, STRING_A, field_s );
	SetFileMapString (  2, STRING_B, data_s );
	SetFileMapString (  2, STRING_C, desc_s );
	SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
	SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, controlindex );
	SetFileMapDWORD (  2, ENTITY_ADD_LIST_BOX, 1 );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, ENTITY_ADD_LIST_BOX )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
	listmax=0;
	if (  listtype == 0 ) 
	{
		t.list_s[0]=t.strarr_s[471];
		t.list_s[1]=t.strarr_s[470];
		listmax=1;
	}
	if (  listtype == 1 ) 
	{
		listmax=fillgloballistwithweapons();
	}
	if (  listtype == 11 ) 
	{
		listmax=fillgloballistwithbehaviours();
	}
	for ( int i = 0 ; i<=  listmax; i++ )
	{
		SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
		SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, controlindex );
		SetFileMapString (  2, STRING_A, t.list_s[ i ].Get() );
		SetFileMapDWORD (  2, ENTITY_ADD_ITEM_TO_LIST_BOX, 1 );
		SetEventAndWait (  2 );
		while (  GetFileMapDWORD( 2, ENTITY_ADD_ITEM_TO_LIST_BOX )  ==  1 ) 
		{
			SetEventAndWait (  2 );
		}
	}
//endfunction

}

// 
//  Interface Properties Expressions
// 

char* getpropertyfield ( int group, int iControl )
{
	cstr field_s =  "";
	SetFileMapDWORD (  2, ENTITY_GET_CONTROL_NAME, 1 );
	SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
	SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, iControl );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, ENTITY_GET_CONTROL_NAME )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
	field_s = GetFileMapString( 2, STRING_A );
//endfunction field$
	strcpy ( t.szreturn , field_s.Get() );
	return t.szreturn;
}

char* getpropertydata ( int group, int iControl )
{
	cstr data_s =  "";
	SetFileMapDWORD (  2, ENTITY_GET_CONTROL_CONTENTS, 1 );
	SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
	SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, iControl );
	SetEventAndWait (  2 );
	while (  GetFileMapDWORD( 2, ENTITY_GET_CONTROL_CONTENTS )  ==  1 ) 
	{
		SetEventAndWait (  2 );
	}
	data_s = GetFileMapString( 2, STRING_B );
//endfunction data$
	strcpy ( t.szreturn , data_s.Get() );
	return t.szreturn;
;
}


//COMMON INTERFACE FUNCTIONS


void set_progress_position ( int  item, int  position )
{
			SetFileMapDWORD (  1, SET_PROGRESS_ITEM, item );
			SetFileMapDWORD (  1, SET_PROGRESS_POSITION, position );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, SET_PROGRESS_ITEM ) ==  1 ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

char* get_list_box ( int  item, int  index )
{
	cstr contents_s =  "";
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, SET_LIST_INDEX, index );
			SetFileMapDWORD (  1, GET_LIST_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, GET_LIST_ITEM )  ==  1 )
			{
				SetEventAndWait (  1 );
			}
			contents_s = GetFileMapString (  1, STRING_A );
//endfunction contents$
	strcpy ( t.szreturn , contents_s.Get() );
	return t.szreturn;
}

void set_radio_state ( int  item, int  state )
{
			if (  state == 0  )  state = 2;
			SetFileMapDWORD (  1, SET_RADIO_ITEM, item );
			SetFileMapDWORD (  1, SET_RADIO_STATE, state );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, SET_RADIO_STATE ) > 0 )  
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

int get_radio_state ( int  item )
{
			int state = 0;
			SetFileMapDWORD (  1, SET_RADIO_ITEM, item );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, SET_RADIO_ITEM ) > 0 ) 
			{
				SetEventAndWait (  1 );
			}
			state = GetFileMapDWORD ( 1, GET_RADIO_ITEM );
//endfunction state
	return state;
}

void set_edit_item ( int  item, char*  text_s )
{
			SetFileMapDWORD (  1, SET_EDIT_ITEM, item );
			SetFileMapString (  1, STRING_A, text_s );
			SetFileMapDWORD (  1, SET_EDIT_TEXT, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, SET_EDIT_TEXT )  ==  1 ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

char* get_edit_item ( int  item )
{
	cstr text_s =  "";
			SetFileMapDWORD (  1, SET_EDIT_ITEM, item );
			SetFileMapDWORD (  1, GET_EDIT_TEXT, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, GET_EDIT_TEXT )  ==  1 )
			{
				SetEventAndWait (  1 );
			}
			text_s = GetFileMapString ( 1, STRING_A );
//endfunction text$
	strcpy ( t.szreturn , text_s.Get() );
	return t.szreturn;
}

void browse ( char*  title_s, char*  directory_s, char*  filter_s )
{
			SetFileMapString (  1, STRING_A, title_s );
			SetFileMapString (  1, STRING_B, directory_s );
			SetFileMapString (  1, STRING_C, filter_s );
			SetFileMapDWORD (  1, BROWSE_DISPLAY, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, BROWSE_FILE_SELECTED ) ==  0 )
			{
				SetEventAndWait (  1 );
			}
			SetFileMapDWORD (  1, BROWSE_FILE_SELECTED, 0 );
			SetFileMapDWORD (  1, BUTTON_CLICKED, 0 );
//endfunction

}

char* browse_for_folder ( char*  directory_s )
{
			cstr text_s =  "";
			SetFileMapString (  1, STRING_A, directory_s );
			SetFileMapDWORD (  1, 200, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD (  1, 204 )  ==  0 )
			{
				SetEventAndWait (  1 );
			}
			SetFileMapDWORD (  1, 204, 0 );
			SetEventAndWait (  1 );
			text_s = GetFileMapString ( 1, STRING_A );
			SetFileMapDWORD (  1, BUTTON_CLICKED, 0 );
			SetEventAndWait (  1 );
//endfunction text$
	strcpy ( t.szreturn , text_s.Get() );
	return t.szreturn;
}

void add_list_item ( int  item, char*  text_s )
{
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapString (  1, STRING_A, text_s );
			SetFileMapDWORD (  1, ADD_LIST_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, ADD_LIST_ITEM )  ==  1 )
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

char* get_list_item ( int  item, int  index )
{
	cstr text_s =  "";
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, SET_LIST_INDEX, index );
			SetFileMapDWORD (  1, GET_LIST_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, GET_LIST_ITEM )  ==  1 ) 
			{
				SetEventAndWait (  1 );
			}
			text_s = GetFileMapString ( 1, STRING_A );
//endfunction text$
	strcpy ( t.szreturn , text_s.Get() );
	return t.szreturn;
}

void delete_list_item ( int  item, int  index )
{
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, SET_LIST_INDEX, index );
			SetFileMapDWORD (  1, DELETE_LIST_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD (   1, DELETE_LIST_ITEM )  ==  1  ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

void clear_list ( int  item )
{
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, LIST_CLEAR, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD (  1, LIST_CLEAR )  ==  1  ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

void select_list_item ( int  item, int  selectionindex )
{
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, LIST_SELECT_ITEM_INDEX, selectionindex );
			SetFileMapDWORD (  1, LIST_SELECT_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD (  1, LIST_SELECT_ITEM )  ==  1  ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

int get_list_item_selection ( int  item )
{
			int selection = -1;
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, GET_LIST_SELECTION, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD (  1, GET_LIST_SELECTION )  ==  1 ) 
			{
				SetEventAndWait (  1 );
			}
			selection = GetFileMapDWORD( 1, LIST_SELECTION );
//endfunction selection
	return selection
;
}

void insert_list_item ( int  item, int  position, char*  text_s )
{
			SetFileMapDWORD (  1, SET_LIST_ITEM, item );
			SetFileMapDWORD (  1, LIST_INSERT_POSITION, position );
			SetFileMapString (  1, STRING_A, text_s );
			SetFileMapDWORD (  1, LIST_INSERT_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD (  1, LIST_INSERT_ITEM )  ==  1 ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}

void add_combo_box ( int  item, char*  text_s )
{
			SetFileMapDWORD (  1, SET_COMBO_ITEM, item );
			SetFileMapString (  1, STRING_A, text_s );
			SetFileMapDWORD (  1, ADD_COMBO_ITEM, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD ( 1, ADD_COMBO_ITEM )  ==  1 ) 
			{
				SetEventAndWait (  1 );
			}
//endfunction

}


//Property Functions


void add_group ( char*  name_s )
{
			SetFileMapString (  2, STRING_A, name_s );
			SetFileMapDWORD (  2, ENTITY_ADD_GROUP, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_GROUP )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

void add_edit_box ( int  group, char*  name_s, char*  contents_s, char*  description_s )
{
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapString (  2, STRING_A, name_s );
			SetFileMapString (  2, STRING_B, contents_s );
			SetFileMapString (  2, STRING_C, description_s );
			SetFileMapDWORD (  2, ENTITY_ADD_EDIT_BOX, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_EDIT_BOX )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

void add_color_picker ( int  group, char*  name_s, char*  contents_s, char*  description_s )
{
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapString (  2, STRING_A, name_s );
			SetFileMapString (  2, STRING_B, contents_s );
			SetFileMapString (  2, STRING_C, description_s );
			SetFileMapDWORD (  2, ENTITY_ADD_COLOR_PICKER, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_COLOR_PICKER )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

void add_file_picker_ex ( int  group, char*  name_s, char*  contents_s, char*  description_s, char*  dir_s, char*  filter_s, char*  title_s )
{
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapString (  2, STRING_A, name_s );
			SetFileMapString (  2, STRING_B, contents_s );
			SetFileMapString (  2, STRING_C, description_s );
			SetFileMapString (  2, 2024, dir_s );
			if (  filter_s != "" ) 
			{
				SetFileMapString (  2, 2280, filter_s );
			}
			else
			{
				SetFileMapString (  2, 2280, t.strarr_s[323].Get() );
			}
			if (  cstr(title_s) != "" ) 
			{
				SetFileMapString (  2, 2536, title_s );
			}
			else
			{
				SetFileMapString (  2, 2536, t.strarr_s[324].Get() );
			}
			SetFileMapDWORD (  2, ENTITY_ADD_FILE_PICKER, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_FILE_PICKER )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

void add_file_picker ( int  group, char*  name_s, char*  contents_s, char*  description_s, char*  dir_s )
{
	add_file_picker_ex( group, name_s, contents_s, description_s, dir_s, "", "" );
//endfunction

}

void add_font_picker ( int  group, char*  name_s, char*  contents_s, char*  description_s )
{
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapString (  2, STRING_A, name_s );
			SetFileMapString (  2, STRING_B, contents_s );
			SetFileMapString (  2, STRING_C, description_s );
			SetFileMapDWORD (  2, ENTITY_ADD_FONT_PICKER, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_FONT_PICKER )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

void add_list_box ( int  group, char*  name_s, char*  contents_s, char*  description_s )
{
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapString (  2, STRING_A, name_s );
			SetFileMapString (  2, STRING_B, contents_s );
			SetFileMapString (  2, STRING_C, description_s );
			SetFileMapDWORD (  2, ENTITY_ADD_LIST_BOX, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_LIST_BOX )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

void add_item_to_list_box ( int  group, int  control, char*  item_s )
{
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, control );
			SetFileMapString (  2, STRING_A, item_s );
			SetFileMapDWORD (  2, ENTITY_ADD_ITEM_TO_LIST_BOX, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_ADD_ITEM_TO_LIST_BOX )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
//endfunction

}

char* get_control_name ( int  group, int  control )
{
	cstr name_s =  "";
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, control );
			SetFileMapDWORD (  2, ENTITY_GET_CONTROL_NAME, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_GET_CONTROL_NAME )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
			name_s =  GetFileMapString( 2, STRING_A );
//endfunction name$
	strcpy ( t.szreturn , name_s.Get() );
	return t.szreturn;
}

char* get_control_contents ( int  group, int  control )
{
	cstr contents_s =  "";
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, control );
			SetFileMapDWORD (  2, ENTITY_GET_CONTROL_CONTENTS, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_GET_CONTROL_CONTENTS )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
			contents_s = GetFileMapString( 2, STRING_B );
//endfunction contents$
	strcpy ( t.szreturn , contents_s.Get() );
	return t.szreturn;
}

char* get_control_description ( int  group, int  control )
{
	cstr description_s =  "";
			SetFileMapDWORD (  2, ENTITY_SET_GROUP_INDEX, group );
			SetFileMapDWORD (  2, ENTITY_SET_CONTROL_INDEX, control );
			SetFileMapDWORD (  2, ENTITY_GET_CONTROL_DESCRIPTION, 1 );
			SetEventAndWait (  2 );
			while (  GetFileMapDWORD( 2, ENTITY_GET_CONTROL_DESCRIPTION )  ==  1 ) 
			{
				SetEventAndWait (  2 );
			}
			description_s = GetFileMapString( 2, STRING_C );
//endfunction description$
	strcpy ( t.szreturn , description_s.Get() );
	return t.szreturn;
}

//Memory check behaviour for MAP EDITOR / TEST GAME

void checkmemoryforgracefulexit ( void )
{
	int recoverdonotuseany3dreferences = 0;
	int ttogglebannertimer = 0;
	int tredscreencount = 0;
	int tsmemavailable = 0;
	int ttogglebanner = 0;
	int tokay = 0;

	//  if cannot create 100MB of contiguous memory, we're nearing the max fragmentation level
	if (  t.game.gameisexe == 0 && g.globals.memorydetector == 1 ) 
	{
	tsmemavailable=SMEMAvailable(1);
	if (  tsmemavailable>1600000 ) 
	{

		//  The Red Screen of Resurrection
		t.strwork = ""; t.strwork = t.strwork + "checkmemoryforgracefulexit - memory detector "+Str(tsmemavailable)+" Kb";
		timestampactivity(0, t.strwork.Get() );
		tredscreencount= MAXTimer()+2000;
		ttogglebannertimer= MAXTimer()+450;
		while (MAXTimer()<tredscreencount )
		{
			CLS (  Rgb(128,0,0) );
			if (MAXTimer()>ttogglebannertimer )
			{
				ttogglebannertimer= MAXTimer()+450;
				ttogglebanner=1-ttogglebanner;
			}
			PasteImage (  g.editorimagesoffset+5+ttogglebanner,(GetDisplayWidth()-ImageWidth(g.editorimagesoffset+5))/2,(GetDisplayHeight()-ImageHeight(g.editorimagesoffset+5))/2 );
			Sync (  );
		}

		//  close conmunication with editor
		OpenFileMap (  1, "FPSEXCHANGE" );
		SetFileMapDWORD (  1,974,2 );

		//  Before we 'BIN OUT', signal IDE that we wish to return to the IDE editor state
		OpenFileMap (  1, "FPSEXCHANGE" );
		SetFileMapDWORD (  1, 970, 1 );
		SetEventAndWait (  1 );

		//  message Box (  - resolution has been changed - must restart - save changes? )
		OpenFileMap (  1, "FPSEXCHANGE" );
		SetFileMapDWORD (  1, 900, 1 );
		SetFileMapString (  1, 1256, t.strarr_s[622].Get() );
		SetFileMapString (  1, 1000, t.strarr_s[623].Get() );
		SetEventAndWait (  1 );
		while (  GetFileMapDWORD(1, 900) == 1 ) 
		{
			SetEventAndWait (  1 );
		}
		tokay=GetFileMapDWORD(1, 904);

		if (  tokay == 1 ) 
		{
			//  no references to 3D objects
			recoverdonotuseany3dreferences=1;
			//  save as now
			gridedit_saveas_map ( );
		}

		//  call a new map editor
		OpenFileMap (  2, "FPSEXCHANGE" );
		SetFileMapString (  2, 1000, "Guru-MapEditor.exe" );
		SetFileMapString (  2, 1256, "-r" );
		SetFileMapDWORD (  2, 994, 0 );
		SetFileMapDWORD (  2, 924, 1 );
		SetEventAndWait (  2 );

		//  Terminate fragmented EXE
		common_justbeforeend();
		ExitProcess ( 0 );
	}
	}
}

int get_cursor_scale_for_obj ( int tObj )
{
	t.tSizeX_f = ObjectSizeX(tObj,1);
	t.tSizeZ_f = ObjectSizeZ(tObj,1);
	t.tscale_f= Sqrt(t.tSizeX_f*t.tSizeX_f + t.tSizeZ_f*t.tSizeZ_f)*3.0;
	return t.tscale_f;
}

void AddPayLoad(ImGuiPayload* payload, bool addtocursor)
{
	//PE: For this to work in wicked we need to shutdown ALL objects created in new enitylib.
	//PE: or g.entidmaster will get reset. and not display anything.
	bool bSetEntIDMaster = false;
	if ( g_TempimageList.size() > 0)
	{
		if(iRestoreEntidMaster >= 0)
			bSetEntIDMaster = true;
		FreeTempImageList();
	}

	extern cFolderItem::sFolderFiles *pDragDropFile;
	if (pDragDropFile) {

		IM_ASSERT(payload->DataSize == sizeof(cFolderItem::sFolderFiles *));
		cFolderItem::sFolderFiles * payload_n = (cFolderItem::sFolderFiles *) payload->Data;
		payload_n = payload_n->m_dropptr;
		if (payload_n) 
		{
			//Add the item.
			CloseDownEditorProperties();
			t.inputsys.constructselection = 0;
			iLastEntityOnCursor = 0;

			t.addentityfile_s = payload_n->m_sFolder.Get();
			if (t.addentityfile_s != "")
			{
				entity_adduniqueentity(false);
				t.tasset = t.entid;
				if (t.talreadyloaded == 0)
				{
					editor_filllibrary();
				}
			}
			if (addtocursor) 
			{
				bool bNormalMasterAdd = true;
				if (payload_n->iAnimationFrom >= 200000)
				{
					//Special multiply object drag drop.
					g.entityrubberbandlist.clear();
					float centerx = GGORIGIN_X;
					float centery = GGORIGIN_Y;
					float centerz = GGORIGIN_Z;
					int iAnchorEntityIndex = -1;
					float higesty = -999999.0f, lowesty = 999999.0;
					int gridcol = sqrt(selected_library_fpe.size());
					if (gridcol < 1) gridcol = 1;
					int gridcolcount = 0;
					for (std::map<std::string, int>::iterator it = selected_library_fpe.begin(); it != selected_library_fpe.end(); ++it)
					{
						if (it->first.length() > 0)
						{
							//Add
							t.addentityfile_s = it->first.c_str();
							if (t.addentityfile_s != "")
							{
								entity_adduniqueentity(false);
								t.tasset = t.entid;
								if (t.talreadyloaded == 0)
								{
									editor_filllibrary();
								}
							}

							int masterobj = g.entitybankoffset + t.entid;

							// duplicate new entity as clone of relevant original clipboard entity
							bool bLowestFound = false;
							t.gridentity = t.entid;

							#define MINGRIDSIZE 20
							float gsx = ObjectSizeX(masterobj) / 2.0f;
							if (gsx < MINGRIDSIZE) gsx = MINGRIDSIZE;
							centerx += gsx * 1.05f;

							//PE: all t.gridentity... need to be set for this to work correctly.
							entity_fillgrideleproffromprofile();  // t.entid
							t.gridentitystaticmode = t.entityprofile[t.entid].defaultstatic;
							t.gridentityposx_f = centerx;
							t.gridentityposy_f = centery;
							t.gridentityposz_f = centerz;
							t.gridentityrotatex_f = ObjectAngleX(masterobj);
							t.gridentityrotatey_f = ObjectAngleY(masterobj);
							t.gridentityrotatez_f = ObjectAngleZ(masterobj);
							t.gridentityrotatequatmode = 0;
							t.gridentityrotatequatx_f = 0;
							t.gridentityrotatequaty_f = 0;
							t.gridentityrotatequatz_f = 0;
							t.gridentityrotatequatw_f = 1;
							t.gridentityscalex_f = ObjectScaleX(masterobj);
							t.gridentityscaley_f = ObjectScaleY(masterobj);
							t.gridentityscalez_f = ObjectScaleZ(masterobj);

							if (higesty < t.gridentityposy_f) higesty = t.gridentityposy_f;
							if (lowesty > t.gridentityposy_f)
							{
								lowesty = t.gridentityposy_f;
								bLowestFound = true;
							}

							//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
							extern bool bNextObjectMustBeClone;
							bNextObjectMustBeClone = true;

							gridedit_addentitytomap(); //Add it to map set t.e

							bNextObjectMustBeClone = false;

							if (iAnchorEntityIndex == -1 || bLowestFound) iAnchorEntityIndex = t.e;

							// and add to new rubber band group
							sRubberBandType rubberbandItem;
							rubberbandItem.e = t.e;
							rubberbandItem.x = t.entityelement[t.e].x;
							rubberbandItem.y = t.entityelement[t.e].y;
							rubberbandItem.z = t.entityelement[t.e].z;
							rubberbandItem.px = t.entityelement[t.e].x;
							rubberbandItem.py = t.entityelement[t.e].y;
							rubberbandItem.pz = t.entityelement[t.e].z;
							rubberbandItem.rx = t.entityelement[t.e].rx;
							rubberbandItem.ry = t.entityelement[t.e].ry;
							rubberbandItem.rz = t.entityelement[t.e].rz;						
							rubberbandItem.quatmode = t.entityelement[t.e].quatmode;
							rubberbandItem.quatx = t.entityelement[t.e].quatx;
							rubberbandItem.quaty = t.entityelement[t.e].quaty;
							rubberbandItem.quatz = t.entityelement[t.e].quatz;
							rubberbandItem.quatw = t.entityelement[t.e].quatw;
							rubberbandItem.scalex = t.entityelement[t.e].scalex;
							rubberbandItem.scaley = t.entityelement[t.e].scaley;
							rubberbandItem.scalez = t.entityelement[t.e].scalez;
							g.entityrubberbandlist.push_back(rubberbandItem);
							centerx += gsx * 1.05f;
						}
					}
					if (iAnchorEntityIndex != -1)
					{
						//Select and add first entity to cursor, along with the rubberband.
						AddEntityToCursor(iAnchorEntityIndex, false);

						//Change to just place under cursor.
						t.inputsys.dragoffsetx_f = 0;
						t.inputsys.dragoffsety_f = 0;
						fHitPointX = 0;
						fHitPointY = HITPOINTYSTARTPOS;
						fHitPointZ = 0;
						fHitOffsetX = 0;
						fHitOffsetY = 0;
						fHitOffsetZ = 0;

						g_bHoldGridEntityPosWhenManaged = true;
						g_fHoldGridEntityPosX = t.gridentityposx_f;
						g_fHoldGridEntityPosY = t.gridentityposy_f;
						g_fHoldGridEntityPosZ = t.gridentityposz_f;
					}
					bNormalMasterAdd = false;
					t.onetimeentitypickup = 0;

					//When dragging in many objects, window is in the way , close it down.
					if (bExternal_Entities_Window && selected_library_fpe.size() > 2 )
						bCheckForClosing = true;
				}
				else if (payload_n->iAnimationFrom >= 100000)
				{
					int l = payload_n->iAnimationFrom - 100000;
					DuplicateFromListToCursor(vEntityGroupList[l]);
					bNormalMasterAdd = false;
					t.onetimeentitypickup = 0;
				}
				else if (payload_n->iAnimationFrom > 0 )
				{
					AddEntityToCursor(payload_n->iAnimationFrom,true);
					bNormalMasterAdd = false;
					t.onetimeentitypickup = 0;
				}
				if (bNormalMasterAdd)
				{
					//PE: TODO check if t.entid is valid here.
					//Make sure we are in entty mode.
					bForceKey = true;
					csForceKey = "e";
					csForceKey = "o";
					iExtractMode = 0; //PE: Always start in find floor mode.
					t.inputsys.dragoffsetx_f = 0;
					t.inputsys.dragoffsety_f = 0;
					fHitPointX = 0;
					fHitPointY = HITPOINTYSTARTPOS;
					fHitPointZ = 0;
					fHitOffsetX = 0;
					fHitOffsetY = 0;
					fHitOffsetZ = 0;

					g_bHoldGridEntityPosWhenManaged = true;
					g_fHoldGridEntityPosX = t.gridentityposx_f;
					g_fHoldGridEntityPosY = t.gridentityposy_f;
					g_fHoldGridEntityPosZ = t.gridentityposz_f;

					t.inputsys.constructselection = t.tasset;
					t.gridentity = t.entid;
					t.inputsys.constructselection = t.entid;
					t.inputsys.domodeentity = 1;
					t.grideditselect = 5;
					//Make sure we use a fresh t.grideleprof
					entity_fillgrideleproffromprofile();
					editor_refresheditmarkers();
				}
				//PE: Removed in design "Keep window open when dragging in objects to level".
				//PE: Now always close window.
				if(bExternal_Entities_Window)
					bCheckForClosing = true;
			}
		}
		pDragDropFile = NULL;
	}

	if (bSetEntIDMaster)
	{
		iRestoreEntidMaster = g.entidmaster;
	}
}

bool TutorialNextAction(void)
{
	tut.iCurrent_Step++;
	return true;
}


//Check if we need to point in the game scene.
bool bReadyForMouseRelease = false;
bool CheckTutorialPlaceit(void)
{
	if (tut.bActive && t.inputsys.mclick == 0 && t.gridentity != 0 ) //
		return CheckTutorialAction("PLACEIT"); //Tutorial: check if we are waiting for this action
	if (tut.bActive && t.gridentity == 0) //terrain
	{
		if (CheckTutorialAction("PLACEIT")) {
			if (t.inputsys.mclick == 0) {
				if (bReadyForMouseRelease) {
					//released.
					TutorialNextAction();
					bReadyForMouseRelease = false;
				}
			}
			else {
				bReadyForMouseRelease = true;
			}
		}
		else bReadyForMouseRelease = false;
		return false;
	}
	else {
		bReadyForMouseRelease = false;
	}
	return false;
}

bool CheckTutorialAction(const char * action, float x_adder)
{
	g_bInTutorialMode = false;
	if (bHelp_Window && tutorial_files.size() >= selected_tutorial) 
	{
		if (tut.bActive) 
		{
			//Tutorial Active.
			g_bInTutorialMode = true;
			if (tut.iCurrent_Step >= 0 && tut.iCurrent_Step < TUTORIALMAXSTEPS) {
				if (pestrcasestr(tut.cStepAction[tut.iCurrent_Step], action)) {

					if (!bTutorialRendered && bImGuiFrameState && !bImGuiReadyToRender) {

						ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
						ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;

						//Display the pointer here.
						bTutorialRendered = true; //Make sure we only render one window.
		
						ImVec2 oldpos = ImGui::GetCursorPos();

						ImGuiStyle &st = ImGui::GetStyle();
						float oldborder = st.PopupBorderSize;

						static float sincounter = 0.0f;
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						ImVec2 pos = window->DC.CursorPos;

						float icon_additional_size = 32.0f;
						st.PopupBorderSize = 0;
						pos.x += x_adder + 4;
						pos.x -= (icon_additional_size*0.60);
						pos.y += 80.0f; // pointer points up now (could make this a toggle mode)
						pos.y -= icon_additional_size;
						pos.y += sin(sincounter) * 22.0f;
						sincounter = sincounter + (5.5f*t.ElapsedTime_f);
						if (sincounter >= 360.0f) sincounter -= 360.0f;

						if (strcmp(tut.cStepAction[tut.iCurrent_Step], "PLACEIT") == 0) {
							pos = OldrenderTargetPos + ImVec2((OldrenderTargetSize.x*0.5f) - 64.0f, 60 + (sin(sincounter) * 22.0f));
							pos += tut.vOffsetPointer[tut.iCurrent_Step];
						}

						ID3D11ShaderResourceView* lpTexture = GetImagePointerView(TUTORIAL_POINTERUP);
						if (lpTexture) 
						{
							ImGuiWindow* window = ImGui::GetCurrentWindow();
							ImGui::GetForegroundDrawList()->AddImage((ImTextureID)lpTexture, pos , pos + ImVec2(64+icon_additional_size, 64+icon_additional_size), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
						}

						st.PopupBorderSize = oldborder;
						ImGui::SetCursorPos(oldpos);
					}
					bTutorialCheckAction = true;
					return true;
				}
			}
		}
	}
	//selected_tutorial
	bTutorialCheckAction = false;
	return false;
}


float ApplyPivot(sObject* pObject, int iMode, GGVECTOR3 vecValue, float fValue)
{
	if (pObject->position.bApplyPivot)
	{
		GGVec3TransformCoord(&vecValue, &vecValue, &pObject->position.matPivot);

		if (iMode == 0) return vecValue.x;
		if (iMode == 1) return vecValue.y;
		if (iMode == 2) return vecValue.z;
	}

	return fValue;
}


void RenderToPreview(int displayobj)
{
	float oldx_f, oldy_f, oldz_f, oldangx_f, oldangy_f;
	int entid = displayobj - g.entitybankoffset;

	// prepare for thumb , set camera.
	oldx_f = t.editorfreeflight.c.x_f;
	oldy_f = t.editorfreeflight.c.y_f;
	oldz_f = t.editorfreeflight.c.z_f;
	oldangx_f = t.editorfreeflight.c.angx_f;
	oldangy_f = t.editorfreeflight.c.angy_f;

	float fLargestY = ObjectSizeY(displayobj,1); // Also add scale (,1)
	float fLargestX = ObjectSizeX(displayobj,1);
	float fLargestZ = ObjectSizeZ(displayobj,1);

	float fOffsetX = 0.0f,fOffsetZ = 0.0f;

	float terrain_height = BT_GetGroundHeight(t.terrain.TerrainID, GGORIGIN_X, GGORIGIN_Z, 1);

	sObject* pObject = g_ObjectList[displayobj];
	if (pObject && t.entityprofile[entid].ischaracter != 1) {
		float fAdjustScaleX = 1.0, fAdjustScaleZ = 1.0;
		if (pObject->pInstanceOfObject)
		{
			fAdjustScaleX = pObject->position.vecScale[0];
			fAdjustScaleZ = pObject->position.vecScale[2];
			pObject = pObject->pInstanceOfObject;
		}
		float fValue = (pObject->collision.vecMax[0] + pObject->collision.vecMin[0]);
		fValue = ApplyPivot(pObject, 0, GGVECTOR3(pObject->collision.vecMax - pObject->collision.vecMin), fValue);
		fValue = fValue * pObject->position.vecScale[0] * fAdjustScaleX;
		fOffsetX = fValue * 0.5f;

		fValue = (pObject->collision.vecMax[2] + pObject->collision.vecMin[2]);
		fValue = ApplyPivot(pObject, 2, GGVECTOR3(pObject->collision.vecMax - pObject->collision.vecMin), fValue);
		fValue = fValue * pObject->position.vecScale[2] * fAdjustScaleZ;
		fOffsetZ = fValue * 0.5f;
	}
	float fLargest = fLargestX;

	if (fLargestZ > fLargest)
		fLargest = fLargestZ;
	
	fLargest += (fLargestY * 0.2);
	//Prevent camera for getting to far away.
	if (fLargest >= 1500) fLargest = 1500;
	if (fLargestY >= 1500) fLargestY = 1500;

	t.editorfreeflight.c.x_f = 25650 + (fLargest*2.4);
	if(fLargestY < 10.0f)
		t.editorfreeflight.c.y_f = terrain_height + 80 + (fLargestY*1.85);
	else
		t.editorfreeflight.c.y_f = terrain_height + 50 + (fLargestY*1.85);
	t.editorfreeflight.c.z_f = 25550 - (fLargest*2.4);
	t.editorfreeflight.c.angx_f = 13;
	t.editorfreeflight.c.angy_f = -180;
	t.editorfreeflight.s = t.editorfreeflight.c;

	//Preview object could be reused so store old pos.
	float fOldObjPosX = ObjectPositionX(displayobj), fOldObjPosY = ObjectPositionY(displayobj), fOldObjPosZ = ObjectPositionZ(displayobj);
	bool bDisplayObjVisible = false;
	bool bWaterVisible = false;

	if (g_ObjectList[t.terrain.objectstartindex + 2] && g_ObjectList[t.terrain.objectstartindex + 2]->bVisible)
		bWaterVisible = true;

	if (g_ObjectList[displayobj] && g_ObjectList[displayobj]->bVisible)
		bDisplayObjVisible = true;

	//custom clear color.
	custom_back_color[0] = 119.0f/255.0f; custom_back_color[1] = 154.0f / 255.0f; custom_back_color[2] = 181.0f / 255.0f; custom_back_color[3] = 1.0f;

	//Hide everything.
	widget_hide();
	ebe_hide();
	terrain_paintselector_hide();
	t.geditorhighlightingtentityID = 0;

	editor_restoreentityhighlightobj();
	gridedit_clearentityrubberbandlist();
	waypoint_hideall();

	//  "hide" all entities in map by moving them out the way
	for (t.tcce = 1; t.tcce <= g.entityelementlist; t.tcce++)
	{
		t.tccentid = t.entityelement[t.tcce].bankindex;
		if (t.tccentid > 0)
		{
			t.tccsourceobj = t.entityelement[t.tcce].obj;
			if (ObjectExist(t.tccsourceobj) == 1)
			{
				PositionObject(t.tccsourceobj, 0, 0, 0);
			}
		}
	}


	PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
	RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);
	PointCamera(GGORIGIN_X + fOffsetX, terrain_height, GGORIGIN_Z + fOffsetZ);
	
	if(fLargestY > 1000.0f)
		t.editorfreeflight.c.angx_f = CameraAngleX() - 9.0f; //6.0
	else if (fLargestY > 500.0f)
		t.editorfreeflight.c.angx_f = CameraAngleX() - 8.0f; //6.0
	else if (fLargestY > 150.0f)
		t.editorfreeflight.c.angx_f = CameraAngleX() - 7.0f; //6.0
	else if (fLargestY > 100.0f)
		t.editorfreeflight.c.angx_f = CameraAngleX() - 6.0f; //6.0
	else if (fLargestY > 50.0f)
		t.editorfreeflight.c.angx_f = CameraAngleX() - 5.0f; //6.0
	else
		t.editorfreeflight.c.angx_f = CameraAngleX() - 4.0f; //6.0
	t.editorfreeflight.c.angy_f = CameraAngleY();
	RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);

	PositionObject(displayobj, GGORIGIN_X, terrain_height, GGORIGIN_Z);

	HideObject(t.terrain.objectstartindex + 2); //hide water

	if (t.entityprofile[entid].ismarker != 0 || t.entityprofile[entid].zdepth == 0)
	{
		SetObjectMask(displayobj, 1);
	}
	else
	{
		SetObjectMask(displayobj, 1 + (1 << 31));
	}

	ShowObject(displayobj);
	
	visuals_justshaderupdate();

	if (t.terrain.TerrainID > 0)
	{
		if (g.globals.riftmode > 0)
		{
		}
		else
		{
			terrain_renderonly();
		}
	}

	bImGuiInTestGame = true; //just reuse this to prevent imgui rendering.
	FastSync();
	bImGuiInTestGame = false;

	if(bWaterVisible)
		ShowObject(t.terrain.objectstartindex + 2);
	if( bDisplayObjVisible )
		ShowObject(displayobj);
	else
		HideObject(displayobj);



	PositionObject(displayobj, fOldObjPosX, fOldObjPosY, fOldObjPosZ);
	
	// delete previous thumbnail
	if (GetImageExistEx(g.importermenuimageoffset + 50))
	{
		image_setlegacyimageloading(true);
		DeleteImage(g.importermenuimageoffset + 50);
		image_setlegacyimageloading(false);
	}

	// we can't grab from the backbuffer when we use a camera image.
	extern DBPRO_GLOBAL CCameraManager m_CameraManager;
	DBPRO_GLOBAL tagCameraData* m_mycam;
	m_mycam = m_CameraManager.GetData(0);
	float thumbnail_dimension = 512;
	if (m_mycam)
	{
		extern GlobStruct* g_pGlob;
		LPGGSURFACE	pTmpSurface = g_pGlob->pCurrentBitmapSurface;
		g_pGlob->pCurrentBitmapSurface = m_mycam->pCameraToImageSurface;

		ImVec2 grab = ImVec2((m_mycam->viewPort3D.Width*0.5) - (thumbnail_dimension*0.5) , (m_mycam->viewPort3D.Height*0.5) - (thumbnail_dimension*0.5) );

		GrabImage(g.importermenuimageoffset + 50, grab.x, grab.y, grab.x+thumbnail_dimension, grab.y + thumbnail_dimension, 0);
		g_pGlob->pCurrentBitmapSurface = pTmpSurface;
	}


	//Restore camera.
	t.editorfreeflight.c.x_f = oldx_f;
	t.editorfreeflight.c.y_f = oldy_f;
	t.editorfreeflight.c.z_f = oldz_f;
	t.editorfreeflight.c.angx_f = oldangx_f;
	t.editorfreeflight.c.angy_f = oldangy_f;
	PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
	RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);


	//Display everything again.
	t.inputsys.dowaypointview = 0;
	
	//Restore.
	waypoint_restore();
	t.gridentityhidemarkers = 0;
	editor_updatemarkervisibility();
	editor_refresheditmarkers();

	//  put all entities back where they were
	for (t.tcce = 1; t.tcce <= g.entityelementlist; t.tcce++)
	{
		t.tccentid = t.entityelement[t.tcce].bankindex;
		if (t.tccentid > 0)
		{
			t.tccsourceobj = t.entityelement[t.tcce].obj;
			if (ObjectExist(t.tccsourceobj) == 1)
			{
				PositionObject(t.tccsourceobj, t.entityelement[t.tcce].x, t.entityelement[t.tcce].y, t.entityelement[t.tcce].z);
			}
		}
	}

	//Turn off custom clear color.
	custom_back_color[0] = 0.0f; custom_back_color[1] = 0.0f; custom_back_color[2] = 0.0f; custom_back_color[3] = 0.0f;

	bImGuiInTestGame = true;
	FastSync();
	bImGuiInTestGame = false;

}


void CheckTooltipObjectDelete(void)
{
	if (!iTooltipAlreadyLoaded) {

		t.tentitytoselect = iTooltipLastObjectId;
		t.entobj = g.entitybankoffset + iTooltipLastObjectId;
		if (ObjectExist(g.entitybankoffset + iTooltipLastObjectId)) {
			DeleteObject(g.entitybankoffset + iTooltipLastObjectId);
		}
		iTooltipLastObjectId = 0;
	}
}

