//----------------------------------------------------
//--- GAMEGURU - M-Grass
//----------------------------------------------------
#include "stdafx.h"
#include "gameguru.h"

#ifdef WICKEDENGINE
#include ".\..\..\Guru-WickedMAX\wickedcalls.h"
#endif

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

// Global
int g_iSuperTriggerFullGrassReveal = 0;

// Externs

#ifndef PRODUCTCLASSIC
extern bool bUpdateVeg;
#endif

// 
//  GRASS SYSTEM
// 

void grass_init ( void )
{
}

void grass_initstyles ( void )
{
}

void grass_initstyles_reset ( void )
{
}

void grass_assignnewshader ( void )
{
}

void grass_applyshader ( void )
{
}

void grass_setgrassimage ( void )
{
}

void grass_setgrassgridandfade ( void )
{
}

void grass_loop ( void )
{
}

void grass_clearregion ( void )
{
}

void grass_updatedirtyregionfast ( void )
{
}

void grass_clamptomemblockres ( void )
{
}

void grass_updategrassfrombitmap ( void )
{
}

void grass_loadgrass ( void )
{
}

void grass_savegrass ( void )
{
}

void grass_buildblankgrass ( void )
{
}

void grass_buildblankgrass_fornew ( void )
{
}

void grass_free ( void )
{
}

void grass_changevegstyle(void)
{
}

//
// Separated out grass editing part from terrain
//

void grass_editcontrol(void)
{
}

void grass_paint(void)
{
}

void grass_resetchoices ( void )
{
}
