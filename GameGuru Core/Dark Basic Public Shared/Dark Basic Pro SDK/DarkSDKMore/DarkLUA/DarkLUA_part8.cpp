 DARKLUA_API void LuaReset ()
 {
	// Close all states, silently
	if ( ppLuaStates )
	{
		for ( int c = 0 ; c < maxLuaStates+1; c++ )
		{
			if ( ppLuaStates[c] != NULL )
			{
				CloseLuaSilent (c);
				ppLuaStates[c] = NULL;
			}
		}
		delete[] ppLuaStates;
		ppLuaStates = NULL;
		maxLuaStates = 0;
	}

	// Empty Message Queue
	if ( ppLuaMessages )
	{
		for ( int c = 0 ; c < maxLuaMessages; c++ )
		{
			if ( ppLuaMessages[c] != NULL )
			{
				delete ppLuaMessages[c];
				ppLuaMessages[c] = NULL;
			}
		}
	}

	// reset messaging
	strcpy ( currentMessage.msgDesc, "" );
	currentMessage.msgFloat = 0.0f;
	currentMessage.msgInt = 0;
	currentMessage.msgIndex = 0;
	strcpy ( currentMessage.msgString, "" );

	//Reset already loaded list
	ScriptsLoaded.clear();

	//Reset error list
	FunctionsWithErrors.clear();

	// 050416 - delete any sprites created inside LUA scripting
	for ( int c = g.LUASpriteoffset ; c <= g.LUASpriteoffsetMax ; c++ )
		if ( SpriteExist ( c ) == 1 )
			DeleteSprite ( c );

	// restore state default
	defaultState = 1;
 }

 DARKLUA_API int LuaExecute ( LPSTR pString , int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

    // run the Lua script string
	int a = 0;

	a = luaL_loadbuffer(lua2, pString, strlen(pString), pString) ||	lua_pcall(lua2, 0, 0, 0);
	if (a) 
	{
	  //MessageBox(NULL, lua_tostring(lua2, -1), "LUA ERROR", MB_TOPMOST | MB_OK);
	  lua_pop(lua2, 1);  /* pop error message from the stack */
	}

	// Return 1 for success, like dbpro styles
	if ( a == 0 )
		a = 1;
	else
		a = 0;

	return a;
 }

 DARKLUA_API int LuaExecute ( LPSTR pString )
 {
	if ( ppLuaStates == NULL ) return 1;

	int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

    // run the Lua script string
	int a = 0;

	a = luaL_loadbuffer(lua2, pString, strlen(pString), pString) ||	lua_pcall(lua2, 0, 0, 0);
	if (a) 
	{
	  //MessageBox(NULL, lua_tostring(lua2, -1), "LUA ERROR", MB_TOPMOST | MB_OK);
	  lua_pop(lua2, 1);  /* pop error message from the stack */
	}

	// Return 1 for success, like dbpro styles
	if ( a == 0 )
		a = 1;
	else
		a = 0;

	return a;
 }

 DARKLUA_API int LuaGetInt ( LPSTR pString , int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;
    lua_getglobal(lua2, pString);

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	int ret = (int)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);

    return ret;
 }

  DARKLUA_API int LuaGetInt ( LPSTR pString )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;
    lua_getglobal(lua2, pString);

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	int ret = (int)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);

    return ret;
 }

  DARKLUA_API int LuaReturnInt ( int id )
 {
	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( lua_gettop(lua2) == 0 )
	{
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	int ret = (int)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);

    return ret;
 }

 DARKLUA_API int LuaReturnInt ( void )
 {
	int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( lua_gettop(lua2) == 0 )
	{
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	int ret = (int)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);

    return ret;
 }

 DARKLUA_API float LuaGetFloat ( LPSTR pString , int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;
    lua_getglobal(lua2, pString);

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	float fValue = (float)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);
	return fValue;
 }

 DARKLUA_API float LuaGetFloat ( LPSTR pString )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;
    lua_getglobal(lua2, pString);

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	float fValue = (float)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);
	return fValue;
 }

 DARKLUA_API float LuaReturnFloat ( int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( lua_gettop(lua2) == 0 )
	{
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	float fValue = (float)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);
	return fValue;
 }

 DARKLUA_API float LuaReturnFloat ( void )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	if ( lua_gettop(lua2) == 0 )
	{
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	/*if (!lua_isnumber(L, -1))
    {
        MessageBox(NULL, "Variable is not a number", "LUA ERROR", MB_TOPMOST | MB_OK);
        return 0;
    }*/

	float fValue = (float)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);
	return fValue;
 }

 DARKLUA_API void LuaGetString(LPSTR pString, LPSTR pDestStr)
 {
	 int id = defaultState;
	 if (id > maxLuaStates + 1)
	 {
		 return;
	 }
	 if (ppLuaStates[id] == NULL)
	 {
		 return;
	 }

	 lua2 = ppLuaStates[id]->state;
	 lua_getglobal(lua2, pString);

	 const char* pValue = lua_tostring(lua2, -1);
	 if (pDestStr && pValue)
	 {
		 strcpy(pDestStr, pValue);
	 }
	 lua_pop(lua2, 1);
 }

 DARKLUA_API void LuaSetInt ( LPSTR pString , int value, int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );
	lua_setglobal( lua2, pString );

 }

  DARKLUA_API void LuaSetInt ( LPSTR pString , int value )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );
	lua_setglobal( lua2, pString );

 }

 DARKLUA_API void LuaPushInt ( int value, int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );

 }

 DARKLUA_API void LuaPushInt ( int value )
 {
	 int id = defaultState;

#ifdef LUA_DO_DEBUG
	WriteToDebugLog ( "-->LuaPushInt" , true );
	WriteToDebugLog ( "ID" , id );
	WriteToDebugLog ( "value" , value );
	WriteToDebugLog ( "===========" , true );
#endif

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );

 }

 DARKLUA_API void LuaSetFloat ( LPSTR pString , float value, int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );
	lua_setglobal( lua2, pString );
 }

 DARKLUA_API void LuaSetFloat ( LPSTR pString , float value )
 {
	int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );
	lua_setglobal( lua2, pString );
 }

 DARKLUA_API void LuaPushFloat ( float value, int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );

 }

  DARKLUA_API void LuaPushFloat ( float value )
 {
	 int id = defaultState;

#ifdef LUA_DO_DEBUG
	WriteToDebugLog ( "-->LuaPushFloat" , true );
	WriteToDebugLog ( "ID" , id );
	WriteToDebugLog ( "value" , value );
	WriteToDebugLog ( "===========" , true );
#endif

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", MB_TOPMOST | MB_OK);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushnumber( lua2, (lua_Number)value );

 }
	
 DARKLUA_API void LuaSetString ( LPSTR pString , LPSTR pStringValue, int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushstring( lua2, pStringValue );
	lua_setglobal( lua2, pString );
 }

 DARKLUA_API void LuaSetString ( LPSTR pString , LPSTR pStringValue )
 {
	int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushstring( lua2, pStringValue );
	lua_setglobal( lua2, pString );
 }


 DARKLUA_API void LuaPushString ( LPSTR pStringValue, int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushstring( lua2, pStringValue );

 }

 DARKLUA_API void LuaPushString ( LPSTR pStringValue )
 {
	 int id = defaultState;

#ifdef LUA_DO_DEBUG
	WriteToDebugLog ( "-->LuaPushString" , true );
	WriteToDebugLog ( "ID" , id );
	WriteToDebugLog ( "pStringValue" , pStringValue );
	WriteToDebugLog ( "===========" , true );
#endif

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	if ( ppLuaStates==NULL ) return;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return;
	}

	lua2 = ppLuaStates[id]->state;
	lua_pushstring( lua2, pStringValue );

 }

 DARKLUA_API int LuaArrayInt ( LPSTR pString , int id )
 {
	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	if ( ppLuaStates==NULL ) return 0;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	//
	char str[512];
	strcpy ( str , pString);
	char * pch;
	pch = strstr (str,"->");
	while (pch)
	{
		if (pch) strncpy (pch,"..",2);
		pch = strstr (str,"->");
	}
	//
	//char str[512];
	//strcpy ( str , pString);
	//char * pch;
	//MessageBox(NULL, str, "str =", NULL);
	bool foundFunction = false;	
	pch = NULL;
	int offset = -1;
	pch = strtok (str,".");
	char lastString[256];
	int lastNumber;
	int lastWas= 0;
	while (pch != NULL)
	{
		if (!foundFunction)
		{
			foundFunction = true;
			
			//sprintf ( errorString , "Array = %s" , pch );
			lua_getglobal(lua2, pch );
			strcpy ( lastString, pch );
			//MessageBox(NULL, errorString , "" , NULL);
			if ( !lua_istable(lua2, offset) )
			{
				lua_pop(lua2,-offset);
				sprintf ( errorString , "%s is not a Lua Table" , pch );
				MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
				return 0;
			}
		}
		else
		{
			if (lastWas == 1)
			{

				lua_getfield ( lua2 , offset , lastString );

				if ( !lua_istable(lua2, offset) )
				{
					lua_pop(lua2,-offset);
					sprintf ( errorString , "previous field to %s does not exist in table" , pch );
					MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
					return 0;
				}
			}
			else if ( lastWas == 2 )
			{
				lua_rawgeti ( lua2, offset , lastNumber );
			}

			lastWas = 1;

			if ( pch[0] == '#' )
			{
				char szString[256];
				strcpy ( szString , pch );
				memcpy ( &szString [ 0 ], &szString [ 1 ], strlen ( szString ) - 1 );
				szString [ strlen ( szString ) - 1 ] = 0;

				int num = atoi(szString);
				lastNumber = num;
				lastWas = 2;
			}
			else
				strcpy ( lastString, pch );
			
		}

		pch = strtok (NULL, ".");
	}

	if ( lastWas != 2 )
	{
		lua_pushstring(lua2, lastString );
		offset--;
		lua_gettable(lua2, offset);
	}
	else
	{
		lua_rawgeti ( lua2, offset , lastNumber );
	}

	int ret = (int)lua_tonumber(lua2, -1);
	lua_pop(lua2,-(++offset));
	return ret;

 }

  DARKLUA_API int LuaArrayInt ( LPSTR pString )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	if ( ppLuaStates==NULL ) return 0;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	//
	char str[512];
	strcpy ( str , pString);
	char * pch;
	pch = strstr (str,"->");
	while (pch)
	{
		if (pch) strncpy (pch,"..",2);
		pch = strstr (str,"->");
	}
	//
	//char str[512];
	//strcpy ( str , pString);
	//char * pch;
	//MessageBox(NULL, str, "str =", NULL);
	bool foundFunction = false;	
	pch = NULL;
	int offset = -1;
	pch = strtok (str,".");
	char lastString[256];
	int lastNumber;
	int lastWas= 0;
	while (pch != NULL)
	{
		if (!foundFunction)
		{
			foundFunction = true;
			
			//sprintf ( errorString , "Array = %s" , pch );
			lua_getglobal(lua2, pch );
			strcpy ( lastString, pch );
			//MessageBox(NULL, errorString , "" , NULL);
			if ( !lua_istable(lua2, offset) )
			{
				lua_pop(lua2,-offset);
				sprintf ( errorString , "%s is not a Lua Table" , pch );
				MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
				return 0;
			}
		}
		else
		{
			if (lastWas == 1)
			{

				lua_getfield ( lua2 , offset , lastString );

				if ( !lua_istable(lua2, offset) )
				{
					lua_pop(lua2,-offset);
					sprintf ( errorString , "previous field to %s does not exist in table" , pch );
					MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
					return 0;
				}
			}
			else if ( lastWas == 2 )
			{
				lua_rawgeti ( lua2, offset , lastNumber );
			}

			lastWas = 1;

			if ( pch[0] == '#' )
			{
				char szString[256];
				strcpy ( szString , pch );
				memcpy ( &szString [ 0 ], &szString [ 1 ], strlen ( szString ) - 1 );
				szString [ strlen ( szString ) - 1 ] = 0;

				int num = atoi(szString);
				lastNumber = num;
				lastWas = 2;
			}
			else
				strcpy ( lastString, pch );
			
		}

		pch = strtok (NULL, ".");
	}

	if ( lastWas != 2 )
	{
		lua_pushstring(lua2, lastString );
		offset--;
		lua_gettable(lua2, offset);
	}
	else
	{
		lua_rawgeti ( lua2, offset , lastNumber );
	}

	int ret = (int)lua_tonumber(lua2, -1);
	lua_pop(lua2,-(++offset));
	return ret;

 }

 DARKLUA_API float LuaArrayFloat ( LPSTR pString , int id )
 {

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	if ( ppLuaStates==NULL ) return 0;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	//
	char str[512];
	strcpy ( str , pString);
	char * pch;
	pch = strstr (str,"->");
	while (pch)
	{
		if (pch) strncpy (pch,"..",2);
		pch = strstr (str,"->");
	}
	//
	//char str[512];
	//strcpy ( str , pString);
	//char * pch;
	//MessageBox(NULL, str, "str =", NULL);
	bool foundFunction = false;	
	pch = NULL;
	int offset = -1;
	pch = strtok (str,".");
	char lastString[256];
	int lastNumber;
	int lastWas= 0;
	while (pch != NULL)
	{
		if (!foundFunction)
		{
			foundFunction = true;
			
			//sprintf ( errorString , "Array = %s" , pch );
			lua_getglobal(lua2, pch );
			strcpy ( lastString, pch );
			//MessageBox(NULL, errorString , "" , NULL);
			if ( !lua_istable(lua2, offset) )
			{
				lua_pop(lua2,-offset);
				sprintf ( errorString , "%s is not a Lua Table" , pch );
				MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
				return 0;
			}
		}
		else
		{
			if (lastWas == 1)
			{

				lua_getfield ( lua2 , offset , lastString );

				if ( !lua_istable(lua2, offset) )
				{
					lua_pop(lua2,-offset);
					sprintf ( errorString , "previous field to %s does not exist in table" , pch );
					MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
					return 0;
				}
			}
			else if ( lastWas == 2 )
			{
				lua_rawgeti ( lua2, offset , lastNumber );
			}

			lastWas = 1;

			if ( pch[0] == '#' )
			{
				char szString[256];
				strcpy ( szString , pch );
				memcpy ( &szString [ 0 ], &szString [ 1 ], strlen ( szString ) - 1 );
				szString [ strlen ( szString ) - 1 ] = 0;

				int num = atoi(szString);
				lastNumber = num;
				lastWas = 2;
			}
			else
				strcpy ( lastString, pch );
			
		}

		pch = strtok (NULL, ".");
	}

	if ( lastWas != 2 )
	{
		lua_pushstring(lua2, lastString );
		offset--;
		lua_gettable(lua2, offset);
	}
	else
	{
		lua_rawgeti ( lua2, offset , lastNumber );
	}

	float fValue = (float)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);
	return fValue;

 }

  DARKLUA_API float LuaArrayFloat ( LPSTR pString )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	if ( ppLuaStates==NULL ) return 0;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	//
	char str[512];
	strcpy ( str , pString);
	char * pch;
	pch = strstr (str,"->");
	while (pch)
	{
		if (pch) strncpy (pch,"..",2);
		pch = strstr (str,"->");
	}
	//
	//char str[512];
	//strcpy ( str , pString);
	//char * pch;
	//MessageBox(NULL, str, "str =", NULL);
	bool foundFunction = false;	
	pch = NULL;
	int offset = -1;
	pch = strtok (str,".");
	char lastString[256];
	int lastNumber;
	int lastWas= 0;
	while (pch != NULL)
	{
		if (!foundFunction)
		{
			foundFunction = true;
			
			//sprintf ( errorString , "Array = %s" , pch );
			lua_getglobal(lua2, pch );
			strcpy ( lastString, pch );
			//MessageBox(NULL, errorString , "" , NULL);
			if ( !lua_istable(lua2, offset) )
			{
				lua_pop(lua2,-offset);
				sprintf ( errorString , "%s is not a Lua Table" , pch );
				MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
				return 0;
			}
		}
		else
		{
			if (lastWas == 1)
			{

				lua_getfield ( lua2 , offset , lastString );

				if ( !lua_istable(lua2, offset) )
				{
					lua_pop(lua2,-offset);
					sprintf ( errorString , "previous field to %s does not exist in table" , pch );
					MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
					return 0;
				}
			}
			else if ( lastWas == 2 )
			{
				lua_rawgeti ( lua2, offset , lastNumber );
			}

			lastWas = 1;

			if ( pch[0] == '#' )
			{
				char szString[256];
				strcpy ( szString , pch );
				memcpy ( &szString [ 0 ], &szString [ 1 ], strlen ( szString ) - 1 );
				szString [ strlen ( szString ) - 1 ] = 0;

				int num = atoi(szString);
				lastNumber = num;
				lastWas = 2;
			}
			else
				strcpy ( lastString, pch );
			
		}

		pch = strtok (NULL, ".");
	}

	if ( lastWas != 2 )
	{
		lua_pushstring(lua2, lastString );
		offset--;
		lua_gettable(lua2, offset);
	}
	else
	{
		lua_rawgeti ( lua2, offset , lastNumber );
	}

	float fValue = (float)lua_tonumber(lua2, -1);
	lua_pop(lua2,1);
	return fValue;

 }

 DARKLUA_API LPSTR LuaArrayString ( LPSTR pString , int id )
 {
	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	if ( ppLuaStates==NULL ) return 0;
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	//
	char str[512];
	strcpy ( str , pString);
	char * pch;
	pch = strstr (str,"->");
	while (pch)
	{
		if (pch) strncpy (pch,"..",2);
		pch = strstr (str,"->");
	}
	//
	//char str[512];
	//strcpy ( str , pString);
	//char * pch;
	//MessageBox(NULL, str, "str =", NULL);
	bool foundFunction = false;	
	pch = NULL;
	int offset = -1;
	pch = strtok (str,".");
	char lastString[256];
	int lastNumber;
	int lastWas= 0;
	while (pch != NULL)
	{
		if (!foundFunction)
		{
			foundFunction = true;
			
			//sprintf ( errorString , "Array = %s" , pch );
			lua_getglobal(lua2, pch );
			strcpy ( lastString, pch );
			//MessageBox(NULL, errorString , "" , NULL);
			if ( !lua_istable(lua2, offset) )
			{
				lua_pop(lua2,-offset);
				sprintf ( errorString , "%s is not a Lua Table" , pch );
				MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
				return 0;
			}
		}
		else
		{
			if (lastWas == 1)
			{

				lua_getfield ( lua2 , offset , lastString );

				if ( !lua_istable(lua2, offset) )
				{
					lua_pop(lua2,-offset);
					sprintf ( errorString , "previous field to %s does not exist in table" , pch );
					MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
					return 0;
				}
			}
			else if ( lastWas == 2 )
			{
				lua_rawgeti ( lua2, offset , lastNumber );
			}

			lastWas = 1;

			if ( pch[0] == '#' )
			{
				char szString[256];
				strcpy ( szString , pch );
				memcpy ( &szString [ 0 ], &szString [ 1 ], strlen ( szString ) - 1 );
				szString [ strlen ( szString ) - 1 ] = 0;

				int num = atoi(szString);
				lastNumber = num;
				lastWas = 2;
			}
			else
				strcpy ( lastString, pch );
			
		}

		pch = strtok (NULL, ".");
	}

	if ( lastWas != 2 )
	{
		lua_pushstring(lua2, lastString );
		offset--;
		lua_gettable(lua2, offset);
	}
	else
	{
		lua_rawgeti ( lua2, offset , lastNumber );
	}

  	// Return string pointer
	LPSTR pReturnString=NULL;
	const char *s = lua_tostring(lua2, -1);
	lua_pop(lua2,1);

	// If input string valid
	if(s)
	{
		// Create a new string and copy input string to it
		DWORD dwSize=strlen( s );
		g_pGlob->CreateDeleteString ( (char**)&pReturnString, dwSize+1 );
		strcpy(pReturnString, s);
	}
	else
	{
		return NULL;
	}

	return pReturnString;

 }

 DARKLUA_API LPSTR LuaArrayString ( LPSTR pString )
 {
	 int id = defaultState;

	if ( id > maxLuaStates+1 )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	if ( ppLuaStates==NULL ) return 0; 
	if ( ppLuaStates[id] == NULL )
	{
		//MessageBox(NULL, "Lua ID not in use", "LUA ERROR", NULL);
		return 0;
	}

	lua2 = ppLuaStates[id]->state;

	//
	char str[512];
	strcpy ( str , pString);
	char * pch;
	pch = strstr (str,"->");
	while (pch)
	{
		if (pch) strncpy (pch,"..",2);
		pch = strstr (str,"->");
	}
	//
	//char str[512];
	//strcpy ( str , pString);
	//char * pch;
	//MessageBox(NULL, str, "str =", NULL);
	bool foundFunction = false;	
	pch = NULL;
	int offset = -1;
	pch = strtok (str,".");
	char lastString[256];
	int lastNumber;
	int lastWas= 0;
	while (pch != NULL)
	{
		if (!foundFunction)
		{
			foundFunction = true;
			
			//sprintf ( errorString , "Array = %s" , pch );
			lua_getglobal(lua2, pch );
			strcpy ( lastString, pch );
			//MessageBox(NULL, errorString , "" , NULL);
			if ( !lua_istable(lua2, offset) )
			{
				lua_pop(lua2,-offset);
				sprintf ( errorString , "%s is not a Lua Table" , pch );
				MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
				return 0;
			}
		}
		else
		{
			if (lastWas == 1)
			{

				lua_getfield ( lua2 , offset , lastString );

				if ( !lua_istable(lua2, offset) )
				{
					lua_pop(lua2,-offset);
					sprintf ( errorString , "previous field to %s does not exist in table" , pch );
					MessageBox(NULL, errorString , "" , MB_TOPMOST | MB_OK);
					return 0;
				}
			}
			else if ( lastWas == 2 )
			{
				lua_rawgeti ( lua2, offset , lastNumber );
			}

			lastWas = 1;

			if ( pch[0] == '#' )
			{
				char szString[256];
				strcpy ( szString , pch );
				memcpy ( &szString [ 0 ], &szString [ 1 ], strlen ( szString ) - 1 );
				szString [ strlen ( szString ) - 1 ] = 0;

				int num = atoi(szString);
				lastNumber = num;
				lastWas = 2;
			}
			else
				strcpy ( lastString, pch );
			
		}

		pch = strtok (NULL, ".");
	}

	if ( lastWas != 2 )
	{
		lua_pushstring(lua2, lastString );
		offset--;
		lua_gettable(lua2, offset);
	}
	else
	{
		lua_rawgeti ( lua2, offset , lastNumber );
	}

  	// Return string pointer
	LPSTR pReturnString=NULL;
	const char *s = lua_tostring(lua2, -1);
	lua_pop(lua2,1);

	// If input string valid
	if(s)
	{
		// Create a new string and copy input string to it
		DWORD dwSize=strlen( s );
		g_pGlob->CreateDeleteString ( (char**)&pReturnString, dwSize+1 );
		strcpy(pReturnString, s);
	}
	else
	{
		return NULL;
	}

	return pReturnString;
 }

// This is the constructor of a class that has been exported.
// see DarkLUA.h for the class definition
CDarkLUA::CDarkLUA()
{
	return;
}

///////////////////////////////////////////////////////////

bool LuaCheckForWorkshopFile ( LPSTR VirtualFilename)
{
	if ( !VirtualFilename ) return false;
	if ( strlen ( VirtualFilename ) < 3 ) return false;

	char* tempCharPointerCheck = NULL;
	tempCharPointerCheck = strrchr( VirtualFilename, '\\' );
	if ( tempCharPointerCheck == VirtualFilename+strlen(VirtualFilename)-1 ) return false;
	if ( VirtualFilename[0] == '.' ) return false;
	if ( strstr ( VirtualFilename , ".fpm" ) ) return false;

	// encrypted file check
	char szEncryptedFilename[_MAX_PATH];
	char szEncryptedFilenameFolder[_MAX_PATH];
	char* tempCharPointer = NULL;
	strcpy ( szEncryptedFilenameFolder, VirtualFilename );

	// replace and forward slashes with backslash
	for ( int c = 0 ; c < strlen(szEncryptedFilenameFolder); c++ )
	{
		if ( szEncryptedFilenameFolder[c] == '/' ) 
			szEncryptedFilenameFolder[c] = '\\';
	}

	tempCharPointer = strrchr( szEncryptedFilenameFolder, '\\' );
	if ( tempCharPointer && tempCharPointer != szEncryptedFilenameFolder+strlen(szEncryptedFilenameFolder)-1 )
	{
		tempCharPointer[0] = 0;
		sprintf ( szEncryptedFilename , "%s\\_e_%s" , szEncryptedFilenameFolder , tempCharPointer+1 );
	}
	else
	{
		sprintf ( szEncryptedFilename , "_e_%s" , szEncryptedFilenameFolder );
	}
	
	if ( GG_FileExists(szEncryptedFilename) )
	{
		strcpy ( VirtualFilename , szEncryptedFilename );
		return true;
	}
	// end of encrypted file check

	// Workshop handling
	#ifdef PHOTONMP
	#else
		char szWorkshopFilename[_MAX_PATH];
		char szWorkshopFilenameFolder[_MAX_PATH];
		char szWorkShopItemPath[_MAX_PATH];
		SteamGetWorkshopItemPathDLL(szWorkShopItemPath);
		//strcpy ( szWorkShopItemPath,"D:\\Games\\Steam\\steamapps\\workshop\\content\\266310\\378822626");
		// If the string is empty then there is no active workshop item, so we can return
		if ( strcmp ( szWorkShopItemPath , "" ) == 0 ) return false;
		tempCharPointer = NULL;
		strcpy ( szWorkshopFilenameFolder, VirtualFilename );

		// only check if the workshop item path isnt blank
		if ( strcmp ( szWorkShopItemPath , "" ) )
		{
			// replace and forward slashes with backslash
			for ( unsigned int c = 0 ; c < strlen(szWorkshopFilenameFolder); c++ )
			{
				if ( szWorkshopFilenameFolder[c] == '/' )
					szWorkshopFilenameFolder[c] = '\\';
			}

			// strip off any path to files folder
			bool found = true;
			while ( found )
			{
			char* stripped = strstr ( szWorkshopFilenameFolder , "Files\\" );
			if ( !stripped )
				stripped = strstr ( szWorkshopFilenameFolder , "files\\" );

			if ( stripped )
				strcpy ( szWorkshopFilenameFolder , stripped+6 );
			else
				found = false;
			}

			bool last = false;
			char tempstring[MAX_PATH];
			strcpy ( tempstring, szWorkshopFilenameFolder);
			strcpy ( szWorkshopFilenameFolder , "" );
			// replace and forward slashes with backslash
			for ( unsigned int c = 0 ; c < strlen(tempstring); c++ )
			{
				if ( tempstring[c] == '/' || tempstring[c] == '\\' ) 
				{
					if ( last == false )
					{
						strcat ( szWorkshopFilenameFolder , "_" );
						last = true;
					}
				}
				else
				{
					strcat ( szWorkshopFilenameFolder , " " );
					szWorkshopFilenameFolder[strlen(szWorkshopFilenameFolder)-1] = tempstring[c];
					last = false;
				}
			}

			//NEED TO CHECK IF THE FILE EXISTS FIRST, IF IT DOES WE COPY IT
			char szTempName[_MAX_PATH];
			strcpy ( szTempName , szWorkShopItemPath );
			strcat ( szTempName , "\\" );
			strcat ( szTempName , szWorkshopFilenameFolder );

			if ( GG_FileExists(szTempName) )
			{
				int szTempNamelength = strlen(szTempName);
				int virtualfilelength = strlen(VirtualFilename);				
				strcpy ( VirtualFilename , szTempName );
				return true;
			}
			else // check for encrypted version
			{
				char* tempCharPointer = NULL;

				tempCharPointer = strrchr( szTempName, '\\' );
				if ( tempCharPointer && tempCharPointer != szTempName+strlen(szTempName)-1 )
				{
					tempCharPointer[0] = 0;
					sprintf ( szWorkshopFilename , "%s\\_w_%s" , szTempName , tempCharPointer+1 );
				}
				else
				{
					sprintf ( szWorkshopFilename , "_w_%s" , szTempName );
				}
				
				if ( GG_FileExists(szWorkshopFilename) )
				{
					strcpy ( VirtualFilename , szWorkshopFilename );
					return true;
				}
			}
		}
	#endif
	return false;
}


#ifdef LUA_DO_DEBUG
FILE* g_fpDebug = NULL;

void OpenDebugLog ( char* szFile )
{
 char szFileOpen [ 256 ] = "";
 sprintf ( szFileOpen, "f:\\%s", szFile );
 g_fpDebug = GG_fopen ( szFileOpen, "wt" );
}

void WriteToDebugLog ( char* szMessage, bool bNewLine )
{
 if ( !g_fpDebug )
  OpenDebugLog ( "log.txt" );
 
 if ( !g_fpDebug )
  return;

 char szOut [ 256 ] = "";

 if ( bNewLine )
  sprintf ( szOut, "%s\n", szMessage );
 else
  sprintf ( szOut, "%s", szMessage );

 fwrite ( szOut, strlen ( szOut ) * sizeof ( char ), 1, g_fpDebug );
 fflush ( g_fpDebug );
}

void WriteToDebugLog ( char* szMessage, int i )
{
 if ( !g_fpDebug )
  OpenDebugLog ( "log.txt" );
 
 if ( !g_fpDebug )
  return;

 char szOut [ 256 ] = "";

 if ( 1 )
  sprintf ( szOut, "%s = %i\n", szMessage , i );


 fwrite ( szOut, strlen ( szOut ) * sizeof ( char ), 1, g_fpDebug );
 fflush ( g_fpDebug );
}

void WriteToDebugLog ( char* szMessage, float f )
{
 if ( !g_fpDebug )
  OpenDebugLog ( "log.txt" );
 
 if ( !g_fpDebug )
  return;

 char szOut [ 256 ] = "";

 if ( 1 )
  sprintf ( szOut, "%s = %f\n", szMessage , f );


 fwrite ( szOut, strlen ( szOut ) * sizeof ( char ), 1, g_fpDebug );
 fflush ( g_fpDebug );
}

void WriteToDebugLog ( char* szMessage, char* s )
{
 if ( !g_fpDebug )
  OpenDebugLog ( "log.txt" );
 
 if ( !g_fpDebug )
  return;

 char szOut [ 256 ] = "";

 if ( 1 )
  sprintf ( szOut, "%s = %s\n", s );


 fwrite ( szOut, strlen ( szOut ) * sizeof ( char ), 1, g_fpDebug );
 fflush ( g_fpDebug );
}

void CloseDebugLog ( void )
{
 fclose ( g_fpDebug );
}
#endif
