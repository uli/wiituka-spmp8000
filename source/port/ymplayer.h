#include "../ymlib/StSoundLibrary.h"

extern int ym_playing;
int ymload_file( char* filename );
int ymload_buffer( char* buffer, int size );
void ymclose(void);
extern volatile YMMUSIC * s_pMusic;
