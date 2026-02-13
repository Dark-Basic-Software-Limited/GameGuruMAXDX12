void charactercreatorplus_initcameratransitions()
{		
	g_DefaultCamPosition.x = t.editorfreeflight.c.x_f;
	g_DefaultCamPosition.y = t.editorfreeflight.c.y_f;
	g_DefaultCamPosition.z = t.editorfreeflight.c.z_f;
	g_DefaultCamAngle.x = t.editorfreeflight.c.angx_f;
	g_DefaultCamAngle.y = t.editorfreeflight.c.angy_f;
	g_DefaultCamAngle.z = 0;
	g_CurrentCamPosition = g_DefaultCamPosition;
	g_CurrentCamAngle = g_DefaultCamAngle;

	// Set initial conditions for the transition when the ccp is initialised.
	g_pCurrentTransition = &g_UpperBodyTransition;
	g_pLastKnownTransition = &g_UpperBodyTransition;
	g_pCurrentTransition->from[0] = g_DefaultCamPosition.x;
	g_pCurrentTransition->from[1] = g_DefaultCamPosition.y;
	g_pCurrentTransition->from[2] = g_DefaultCamPosition.z;
	g_pCurrentTransition->angFrom[0] = g_DefaultCamAngle.x;
	g_pCurrentTransition->angFrom[1] = g_DefaultCamAngle.y;

	GGVECTOR3 characterPos(ObjectPositionX(iCharObj), ObjectPositionY(iCharObj), ObjectPositionZ(iCharObj));
	float fAngleX = g_DefaultCamAngle.x;
	float fAngleY = g_DefaultCamAngle.y;

	g_HeadTransition.t = 0.0f;
	g_HeadTransition.to[0] = characterPos.x + (g_DefaultCamPosition.x - characterPos.x) * 0.15f;
	g_HeadTransition.to[1] = characterPos.y + 56.5f + (g_DefaultCamPosition.y - characterPos.y) * 0.15f;
	g_HeadTransition.to[2] = characterPos.z + (g_DefaultCamPosition.z - characterPos.z) * 0.15f;
	g_HeadTransition.angTo[0] = fAngleX + 0.0f;
	g_HeadTransition.angTo[1] = fAngleY;

	g_ZombieHeadTransition.t = 0.0f;
	g_ZombieHeadTransition.to[0] = characterPos.x + (g_DefaultCamPosition.x - characterPos.x) * 0.2f + 5.0f;
	g_ZombieHeadTransition.to[1] = characterPos.y + 53.0f + (g_DefaultCamPosition.y - characterPos.y) * 0.1f;
	g_ZombieHeadTransition.to[2] = characterPos.z + (g_DefaultCamPosition.z - characterPos.z) * 0.15f;
	g_ZombieHeadTransition.angTo[0] = fAngleX - 3.0f;
	g_ZombieHeadTransition.angTo[1] = fAngleY;

	g_UpperBodyTransition.t = 0.0f;
	g_UpperBodyTransition.to[0] = characterPos.x + (g_DefaultCamPosition.x - characterPos.x) * 0.2f;
	g_UpperBodyTransition.to[1] = characterPos.y + 45.0f + (g_DefaultCamPosition.y - characterPos.y) * 0.2f;
	g_UpperBodyTransition.to[2] = characterPos.z + (g_DefaultCamPosition.z - characterPos.z) * 0.2f;
	g_UpperBodyTransition.angTo[0] = fAngleX + 10.0f;
	g_UpperBodyTransition.angTo[1] = fAngleY;

	g_ZombieBodyTransition.t = 0.0f;
	g_ZombieBodyTransition.to[0] = characterPos.x + (g_DefaultCamPosition.x - characterPos.x) * 0.2f;
	g_ZombieBodyTransition.to[1] = characterPos.y + 42.0f + (g_DefaultCamPosition.y - characterPos.y) * 0.15f;
	g_ZombieBodyTransition.to[2] = characterPos.z + (g_DefaultCamPosition.z - characterPos.z) * 0.2f;
	g_ZombieBodyTransition.angTo[0] = fAngleX + 10.0f;
	g_ZombieBodyTransition.angTo[1] = fAngleY;

	g_LowerBodyTransition.t = 0.0f;
	g_LowerBodyTransition.to[0] = characterPos.x + (g_DefaultCamPosition.x - characterPos.x) * 0.3f;
	g_LowerBodyTransition.to[1] = characterPos.y + 37.0f + (g_DefaultCamPosition.y - characterPos.y) * 0.3f;
	g_LowerBodyTransition.to[2] = characterPos.z + (g_DefaultCamPosition.z - characterPos.z) * 0.3f;
	g_LowerBodyTransition.angTo[0] = fAngleX + 40.0f;
	g_LowerBodyTransition.angTo[1] = fAngleY;
};

void charactercreatorplus_changecameratransition(int part)
{
	int ccp_part_order[] = { 2       ,1        ,4         ,0       ,3           ,5        ,6        ,7 };

	g_iPreviousCategorySelection = part;

	// Choose which camera transition to use.
	if (ccp_part_order[part] <= 4)
	{
		if(!strstr(CCP_Type, "Zombie"))
			g_pCurrentTransition = &g_HeadTransition;
		else
			g_pCurrentTransition = &g_ZombieHeadTransition;
	}
	else if (ccp_part_order[part] == 5)
	{
		if (!strstr(CCP_Type, "Zombie"))
			g_pCurrentTransition = &g_UpperBodyTransition;
		else
			g_pCurrentTransition = &g_ZombieBodyTransition;
	}
	else if (ccp_part_order[part] <= 7)
		g_pCurrentTransition = &g_LowerBodyTransition;
	else
		g_pCurrentTransition = nullptr;

	// Set the intial conditions for the camera transition.
	if (g_pCurrentTransition)
	{
		g_pCurrentTransition->from[0] = g_CurrentCamPosition.x;
		g_pCurrentTransition->from[1] = g_CurrentCamPosition.y;
		g_pCurrentTransition->from[2] = g_CurrentCamPosition.z;
		g_pCurrentTransition->angFrom[0] = g_CurrentCamAngle.x;
		g_pCurrentTransition->angFrom[1] = g_CurrentCamAngle.y;
		g_pCurrentTransition->t = 0.0f;
	}
}

void charactercreatorplus_performcameratransition(bool bIsZooming)
{
	if (g_pCurrentTransition)
	{
		g_pLastKnownTransition = g_pCurrentTransition;

		// Offset the target away from the character based on the zoom level.
		float fZoomLevel = 1.0f - g_fCCPZoom * 0.01f;
		GGVECTOR3 target(g_pCurrentTransition->to[0] + (g_DefaultCamPosition.x - g_pCurrentTransition->to[0]) * fZoomLevel, g_pCurrentTransition->to[1] + (g_DefaultCamPosition.y - g_pCurrentTransition->to[1]) *fZoomLevel, g_pCurrentTransition->to[2] + (g_DefaultCamPosition.z - g_pCurrentTransition->to[2]) * fZoomLevel);

		// Lerp between the starting and target position to get the desired camera position;
		GGVECTOR3 from(g_pCurrentTransition->from[0], g_pCurrentTransition->from[1], g_pCurrentTransition->from[2]);
		g_CurrentCamPosition = from + ((target - from) * g_pCurrentTransition->t);

		// Lerp between the starting and target rotation to get the desired camera rotation.
		GGVECTOR3 targetRotation(g_pCurrentTransition->angTo[0] + (g_DefaultCamAngle.x - g_pCurrentTransition->angTo[0]) * fZoomLevel, g_pCurrentTransition->angTo[1] + (g_DefaultCamAngle.y - g_pCurrentTransition->angTo[1]) * fZoomLevel, 0);
		GGVECTOR3 fromRotation(g_pCurrentTransition->angFrom[0], g_pCurrentTransition->angFrom[1], 0);
		g_CurrentCamAngle = fromRotation + (targetRotation - fromRotation) * g_pCurrentTransition->t;

		// If the user is zooming, increase the move speed so it feels responsive.
		float fMoveSpeed = 2.0f;
		if (bIsZooming)
			fMoveSpeed = 4.0f;

		// Increment t ready for the next frame.
		float dt = ImGui::GetIO().DeltaTime;
		if (dt > 0.05f) dt = 0.05f;
		g_pCurrentTransition->t += fMoveSpeed * dt;

		// Finished camera transition.
		if (g_pCurrentTransition->t >= 1.0f)
		{
			g_pCurrentTransition->t = 0.0f;
			g_pCurrentTransition = nullptr;
		}

		// Apply the camera changes.
		PositionCamera(g_CurrentCamPosition.x, g_CurrentCamPosition.y, g_CurrentCamPosition.z);
		RotateCamera(g_CurrentCamAngle.x, g_CurrentCamAngle.y, 0.0);
		t.editorfreeflight.c.x_f = g_CurrentCamPosition.x;
		t.editorfreeflight.c.y_f = g_CurrentCamPosition.y;
		t.editorfreeflight.c.z_f = g_CurrentCamPosition.z;
		t.editorfreeflight.c.angx_f = g_CurrentCamAngle.x;
		t.editorfreeflight.c.angy_f = g_CurrentCamAngle.y;
	}
}

void charactercreatorplus_dozoom()
{
	g_pCurrentTransition = g_pLastKnownTransition;
	if (g_pCurrentTransition)
	{
		g_pCurrentTransition->from[0] = g_CurrentCamPosition.x;
		g_pCurrentTransition->from[1] = g_CurrentCamPosition.y;
		g_pCurrentTransition->from[2] = g_CurrentCamPosition.z;
		g_pCurrentTransition->angFrom[0] = g_CurrentCamAngle.x;
		g_pCurrentTransition->angFrom[1] = g_CurrentCamAngle.y;
		charactercreatorplus_performcameratransition(true);
	}
}
