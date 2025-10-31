//----------------------------------------------------
//--- GAMEGURU - M-DAINew Header
//----------------------------------------------------

#include "cstr.h"

// used in GameGuru MAX (all new AI system)
void darkai_init (void);
void darkai_resetsmoothanims (void);
void darkai_free (void);
void darkai_createinternaldebugvisuals (void);
void darkai_destroyinternaldebugvisuals (void);
void darkai_updatedebugobjects_forcharacter(bool bCharIsActive);
void darkai_calcplrvisible (charanimstatetype& cas);
void darkai_loop (void);
void darkai_update (void);
void darkai_killai (void);
void char_init (void);
void char_loop (void);
void smoothanimtriggerrev (int obj, float  st, float  fn, int  speedoftransition, int  rev, int  playflag, float fStartFromPercentage);
void smoothanimtrigger (int obj, float  st, float  fn, int  speedoftransition);
void smoothanimupdate (int obj);
void darkai_makesound_byplayer (void);
void darkai_makesound (void);
void darkai_makeexplosionsound (void);
void darkai_managesound (void);
void darkai_init ( void );
void darkai_resetsmoothanims ( void );
void darkai_free ( void );
void darkai_setup_characters ( void );
void darkai_setupcharacter ( void );
void darkai_shootplayer (void);
void darkai_shoottarget ( int e );
void darkai_shooteffect ( void );
void darkai_shootcharacter ( void );
void darkai_ischaracterhit ( void );
