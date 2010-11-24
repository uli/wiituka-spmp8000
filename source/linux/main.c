/*---------------------------------------------------------------------------------

    PITUKA TEST (LINUX BUILD)
    LINUX WINDOW ONLY - FOR DEBUG

---------------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>


typedef unsigned int bool;

#include "../global.h"
#include "../port/gfx/pituka-arranque.h"
#include "../ymlib/StSoundLibrary.h"
#include "../port/ymplayer.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

/*** 2D Video Globals ***/
void *pix = NULL;
SDL_AudioSpec *audio_spec = NULL;

extern Bitu8 pituka_arranque[];
extern Bitu16 pituka_pal[];

extern Bitu8 *pbSndBuffer;
extern Bitu8 *pbSndBufferEnd;
extern Bitu8 *pbSndStream;
extern t_CPC CPC;
extern int emuDone;

unsigned short pituka_surface[320*240];

SDL_Surface *screen;
int screen_bps;
Bitu16 *screen_base;

WiituKa_Status WiiStatus = {0,0,0,0,0,0,0,0,0,0};

/* FUNCIONES */
int init_buffer (void);
void close_buffer (void);


/* GLOBALES */
bool GPChangeDISK;
WiituKa_Status WiiStatus;
xmlWiiCFG WiitukaXML;
t_WiiRom globalRom;

Wii_gun gunstick;
char extension[5];
char file_state[13];
char rom_name[256];

Pituka_SpoolKeys spoolkeys;
unsigned char reiniciado;
bool pinta_menu;

char debugt [1024];
char current_path [1024] = "/CPCROMS/";

char spool_cad[20];
bool spool;
bool spool_act;

bool sonido_ant;

/* CPC */
extern Bitu8 keyboard_matrix[16];
extern Bitu8 bit_values[8];
extern Bitu8 keyboard_translation_SDL[320];

extern Bitu8 * pbGPBuffer;

static int init_linux (void)
{
    if(SDL_Init(SDL_INIT_VIDEO |SDL_INIT_AUDIO) < 0) //| SDL_INIT_JOYSTICK
        return 0;

    screen = SDL_SetVideoMode(WRES, HRES, 16, SDL_SWSURFACE);
    if (!screen) {
        fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
        return 0;
    }

    SDL_JoystickOpen(0);
    SDL_ShowCursor(SDL_DISABLE);

    if (SDL_MUSTLOCK(screen)) {
        SDL_LockSurface(screen);
    }

    screen_bps = screen->pitch / 4; // rendered screen line length (changing bytes to dwords)
    screen_base = (Bitu16 *)screen->pixels; // memory address of back buffer
    if (SDL_MUSTLOCK(screen)) {
        SDL_UnlockSurface(screen);
    }

    return 1;
}

inline void linux_draw_bit (Bitu16 *p, Bitu8 v)
{
    *p = pituka_pal[v];
}


void CleanScreen (void)
{
    SDL_FillRect(screen,NULL,SDL_MapRGB(screen->format,0,0,0));
    SDL_Flip(screen);
}

void UpdateScreen (void) 
{
    register int x;
    Bitu16 *dest = NULL, *src = NULL;

    if (SDL_MUSTLOCK(screen)) {
        if ( SDL_LockSurface(screen) < 0 ){ //Bloqueo el Surface
            return;
        }
    }

    dest = (Bitu16*) (screen_base + 0);
    src = (Bitu16*) (pix + 0);

    for (x=0; x < WRES*HRES ; x++)
    *(dest++) = (Bitu16) *(src++);


    if (SDL_MUSTLOCK(screen)) {
        SDL_UnlockSurface(screen);
    }

    SDL_Flip(screen);
}

#define CENTRADO_LOGO 32

void FillScreenPal ( unsigned char * surface )
{
    Bitu8 * src = surface;
    Bitu16 *dest = NULL;
    register int x, y;

    dest = (Bitu16*) (pix + WRES*CENTRADO_LOGO);

    for(y=0; y < 240; y++){ //tamaño del logo original 320x240
        dest+=CENTRADO_LOGO; 

        for (x=0; x < 320 ; x++)
            linux_draw_bit((Bitu16*)dest++, *(src++));

        dest+=CENTRADO_LOGO; 
    }
}

void FillScreenBuffer ( void * surface ) 
{
    Bitu8 * src = surface;
    Bitu16 *dest = NULL;
    register int x;

    dest = (Bitu16*) (pix + 0);

    for (x=0; x < HRES*WRES ; x++)
        *(dest++) = (Bitu16) *(src++);

}

void FillScreen( int Updated )
{
    //actualizado directamente en emulacion
}

Bitu8 event2key (SDL_Event event)
{
    if (event.key.keysym.sym>=320) {
        return 0xff;
    } else {
        return keyboard_translation_SDL[event.key.keysym.sym];
    }
}


int linux_input (void)
{
    SDL_Event event;

    while(SDL_PollEvent(&event)){

        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_JOYBUTTONDOWN:
            case SDL_QUIT:
                return 1;
                break;
        }
    }

    return 0;
}

int poll_input (void) 
{
    SDL_Event event;
    Bitu8 cpc_key;

    while(SDL_PollEvent(&event)){

        switch (event.type) {
            case SDL_MOUSEBUTTONUP:
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    if(ym_playing == 1)
                    {
                        ymclose();
                        emu_paused (0);
                    }
                    else
                    {
                        emu_paused (1);
                        ymload_file(rom_name);
                    }
                }
                else
                {
                
                WiiStatus.LoadDISK = 2;
                strcpy(rom_name, "../../shinobi.zip");
                printf("ROM: %s\n", rom_name);
                emu_paused (1);
                emulator_reset(true);
                
                }
                break;

            case SDL_KEYDOWN:
                cpc_key = event2key(event); // translate the PC key to a CPC key
                if (cpc_key != 0xff) {
                    keyboard_matrix[cpc_key >> 4] &= ~bit_values[cpc_key & 7]; // key is being held down
                }
                break;
            case SDL_KEYUP:
                cpc_key = event2key(event); // translate the PC key to a CPC key
                if (cpc_key != 0xff) {
                    keyboard_matrix[cpc_key >> 4] |= bit_values[cpc_key & 7]; // key has been released
                }
                break;

            case SDL_QUIT:
                emuDone = 1;
                break;
        }
    }
    return 0;

}


int main(int argc, char *argv[]) {
    int i,length;

    if(!init_linux())
        return 1;

    SoundInit();

    if(!init_buffer())
        return 2;

    for (i = 1; i < argc; i++) { // loop for all command line arguments
        length = strlen(argv[i]);
        if (length > 5 && length < 256) // minumum for a valid filename
        {
            if (argv[i][0] == '"') { // argument passed with quotes?
                length -= 2;
                strncpy(rom_name, &argv[i][1], length); // strip the quotes
            }
            else
                strncpy(rom_name, &argv[i][0], sizeof(rom_name)-1); // take it as is

            printf("ROM: %s\n", rom_name);
            //WiiStatus.LoadDISK = 2;
        }
    }
    
    cpc_main();

    close_buffer();

    if(ym_playing == 1)
       ymclose();

    SDL_Quit();

    return 0;
}

int load_rom(t_WiiRom * romfs)
{
    int fileSize = 0;
    void * fbuffer = NULL;
    FILE *pfile;

    strcpy(romfs->filename, rom_name);

    if(!(pfile = fopen(romfs->filename, "rb")))
    {
        spool = 1;
        sprintf(spool_cad, " LOAD ROM: ERROR (%s)", romfs->filename);
        return false;
    }

    fseek(pfile, 0, SEEK_END);
    fileSize = ftell(pfile);
    rewind(pfile);

    fbuffer = (void *) malloc(fileSize); 

    if(fbuffer == NULL)
    {
        spool = 1;
        sprintf(spool_cad, " LOAD ROM: NO MEMORY (%i)", fileSize);
        fclose(pfile);
        return false;
    }

    if(!fread(fbuffer, 1, fileSize, pfile))
    {
        free(fbuffer);
        fclose(pfile);
        spool = 1;
        sprintf(spool_cad, " LOAD ROM: NO READED? (%s)", romfs->filename);
        return false;
    }

    fclose(pfile);

    bool result = false;

    if(!(result = loadBuffered_rom (fbuffer, fileSize)))
    {
        spool = 1;
        sprintf(spool_cad, " LOAD BUFFER ROM: BAD? (%s)", romfs->filename);
    }

    free(fbuffer);

    return result;
}

unsigned int GetTicks(void)
{
    return (unsigned int) SDL_GetTicks();
}

int init_buffer (void)
{
    pbGPBuffer = (Bitu8*) malloc(128*1024); //need by unzip lib

    pix = (void *)malloc(4 * WRES * HRES);
    if(pix == NULL)
        return 0;

    return 1;
}

void close_buffer (void)
{
    if(pix != NULL)
    {
        free(pix);
        pix = NULL;
    }

    free(pbGPBuffer);
}

/*
   SND DEBUG CODE
*/

void SoundUpdate (void *userdata, Bitu8 *stream, int len)
{

    if(ym_playing)
    {
		int nbSample = len / sizeof(ymsample);
		ymMusicCompute((void*)s_pMusic,(ymsample*)stream,nbSample);
    }
    else
    {
        memcpy(stream, pbSndStream, len);
        pbSndStream += len;

        if (pbSndStream >= pbSndBufferEnd) 
            pbSndStream = pbSndBuffer;
    }
}

int audio_align_samples (int given)
{
    int actual = 1;

    while (actual < given) {
        actual <<= 2;
    }

    return actual; // return the closest match as 2^n
}

void StopSound ( int val )
{
    SDL_PauseAudio(val);  
}

int SoundSetup (void)
{
    return audio_spec->size; // size is samples * channels * bytes per sample (1 or 2)
}

void SoundInit (void)
{
    SDL_AudioSpec *desired, *obtained;

    desired = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
    obtained = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));

    desired->freq = 48000;
    desired->format = AUDIO_S16LSB;
    desired->channels = 2;
    desired->samples = audio_align_samples(desired->freq / 50); // desired is 20ms at the given frequency

    desired->callback = SoundUpdate;
    desired->userdata = NULL;

    if (SDL_OpenAudio(desired, obtained) < 0) 
    {
        fprintf(stderr, "Could not open audio: %s\n", SDL_GetError());
        return;
    }

    free(desired);
    audio_spec = obtained;

    printf("AUDIO - S: %i\n",audio_spec->size);

}

void SoundClose (void)
{
    SDL_CloseAudio();
    if (audio_spec) 
    {
        free(audio_spec);
    }

}

void ShowMenu (int nMenu) 
{ 
    emu_paused (0);
}

void main_process_pause (int val) {}
void ShowLed(int val) {}
void ShowEmuCommon (void) {}

inline unsigned char Wiimote_CheckGun (void)
{
    return 0xff; //nada
}

