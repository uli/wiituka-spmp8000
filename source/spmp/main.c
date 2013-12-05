/* main.c - system-specific routines; part of the Caprice32 SPMP port
 *
 * Copyright (C) 2013 Ulrich Hecht <ulrich.hecht@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgame.h>
#include <text.h>

#include "small_keyboard.h"
#include "cursor_point.h"

typedef unsigned int bool;

#include "../global.h"
#include "../port/gfx/pituka-arranque.h"
#include "../ymlib/StSoundLibrary.h"
#include "../port/ymplayer.h"

#include "ui.h"

/*** 2D Video Globals ***/
void *pix = NULL;

extern Bitu8 *pbSndBuffer;
extern Bitu8 *pbSndBufferEnd;
extern Bitu8 *pbSndStream;
extern t_CPC CPC;
extern int emuDone;

emu_graph_params_t gp;
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
char current_path [1024] = "./CPCROMS/";

char spool_cad[20];
bool spool;
bool spool_act;

bool sonido_ant;

/* CPC */
extern Bitu8 keyboard_matrix[16];
extern Bitu8 bit_values[8];
extern Bitu8 keyboard_translation_SDL[320];

extern Bitu8 * pbGPBuffer;

emu_keymap_t keymap;

uint16_t palette[256];

void update_palette(unsigned int *pal)
{
 int i;
  for (i = 0; i < 17; i++)
   palette[i] = pal[i];
}

static int init_linux (void)
{
    gp.pixels = malloc(WRES * HRES * 2);
    gp.width = WRES;
    gp.height = HRES;
    gp.has_palette = 0;//1;
    gp.palette = palette;
    gp.src_clip_x = 0;
    gp.src_clip_y = 0;
    gp.src_clip_w = WRES;
    gp.src_clip_h = HRES;
    emuIfGraphInit(&gp);

    gDisplayDev->setShadowBuffer(gDisplayDev->getFrameBuffer());
    emuIfKeyInit(&keymap);

    screen_base = (Bitu16 *)gp.pixels; // memory address of back buffer
    return 1;
}

inline void linux_draw_bit (Bitu16 *p, Bitu8 v)
{
    *p = pituka_pal[v];
}

//void SoundUpdate(void);
void update_sound(void);
void UpdateScreen (void) 
{
#if 0
 static uint64_t last = 0;
 uint64_t now = libgame_utime();
 //printf("update screen %d (%d us)\n", (unsigned int)now, now - last);
 last = now;
 //printf("update screen end %d\n", (unsigned int)libgame_utime());
#endif
}

#define CENTRADO_LOGO 32

static void bitblit(int x, int y, uint16_t *data, int w, int h)
{
    int fb_w = gp.width;
    uint16_t *fb = gp.pixels + x + y * fb_w;
    int i;
    if (x <= -w || x > fb_w)	/* off screen */
        return;
    int bytes = w * 2;
    if (x < 0) {
        fb -= x;
        data -= x;
        bytes += x * 2;
    }
    else if (x + w >= fb_w) {
        bytes -= (x + w - fb_w) * 2;
    }
    for (i = 0; i < h; i++) {
        memcpy(fb, data, bytes);
        fb += fb_w;
        data += w;
    }
}
static void bitblit_alpha(int x, int y, uint16_t *data, int w, int h, uint16_t key)
{
    int fb_w = gp.width;
    int fb_h = gp.height;
    uint16_t *fb = gp.pixels + x + y * fb_w;
    int i, j;
    if (x <= -w || x > fb_w)	/* off screen */
        return;
    if (y <= -h || y > fb_h)
        return;
    if (y + h > fb_h)
        h -= y + h - fb_h;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++)
            if (x + j >= 0 && x + j < fb_w && data[j] != key)
                fb[j] = data[j];
        fb += fb_w;
        data += w;
    }
}

int keyboard_x = (WRES - 320) / 2;
int keyboard_y = HRES - 92;
int cursor_x = 320 / 2;
int cursor_y = HRES - 92 / 2;

void draw_keyboard(void)
{
    bitblit(keyboard_x, keyboard_y, (uint16_t *)small_keyboard, 320, 92);
    bitblit_alpha(cursor_x - 5, cursor_y - 3, (uint16_t *)cursor_point, 20, 20, 0);
}

int show_keyboard = 0;
void FillScreen( int Updated )
{
 static uint64_t last = 0;
 //printf("fillscreen %d %d\n", Updated, (unsigned int)libgame_utime());
    if (!Updated)
        return;
    //printf("update\n");
    if (show_keyboard) {
        draw_keyboard();
    }
    emuIfGraphShow();
    uint64_t now = libgame_utime();
    //printf("fill %d after %d us\n", Updated, (unsigned int)(now - last));
    last = now;
 //printf("fillscreen end %d %d\n", Updated, (unsigned int)libgame_utime());
}

#if 0
Bitu8 event2key (SDL_Event event)
{
    if (event.key.keysym.sym>=320) {
        return 0xff;
    } else {
        return keyboard_translation_SDL[event.key.keysym.sym];
    }
}
#endif

#define KEY_GFX_WIDTH 18
                            //X INIT
const int kc_pos [6][2] = {
		      { -6, 568 },
                      {  5, 599 },
                      {  9, 566 },
                      {  17, 539 },
                      {  17, 577 }
                    };       //X END
unsigned const char keyb_array [6][19] = 
		    {
		      { 0x82,0x80,0x81,0x71,0x70,0x61,0x60,0x51,0x50,0x41,0x40,0x31,0x30,0x97,0x97,0x12,0x13,0x03,0xff },
		      { 0x84,0x83,0x73,0x72,0x62,0x63,0x53,0x52,0x43,0x42,0x33,0x32,0x21,0x22,0xff,0x24,0x14,0x04,0xff },
		      { 0x86,0x85,0x74,0x75,0x65,0x64,0x54,0x55,0x45,0x44,0x35,0x23,0x22,0x22,0xff,0x15,0x16,0x05,0xff },
		      { 0x25,0x87,0x77,0x76,0x67,0x66,0x56,0x46,0x47,0x37,0x36,0x25,0x25,0x25,0xff,0x17,0x00,0x07,0xff },
		      { 0x27,0x11,0x11,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x06,0x06,0x06,0x06,0xff,0x10,0x02,0x01,0xff }
                    };

int poll_input (void) 
{
    static uint32_t oldkeys = 0xffffffffUL;
    uint32_t keys, diffkeys, downkeys, upkeys;
    if (oldkeys == 0xffffffffUL)
        oldkeys = emuIfKeyGetInput(&keymap);
    keys = emuIfKeyGetInput(&keymap);
    diffkeys = keys ^ oldkeys;
    downkeys = diffkeys & keys;
    upkeys = diffkeys & oldkeys;
    oldkeys = keys;
    ge_key_data_t ge_keys;
    if (diffkeys)
      NativeGE_getKeyInput4Ntv(&ge_keys);
    
    //printf("poll input keys %0x down %0x\n", keys, downkeys);
    if (downkeys & keymap.scancode[EMU_KEY_R]) {
        char *romname = 0;
        int res;
        if ((res = select_file(NULL, "dsk|zip", &romname, FONT_SIZE_16)) < 0) {
            printf("select_file %d\n", res);
        }
        if (romname) {
            printf("loading image %s\n", romname);
            loadFiled_rom(romname);
        }
    }
    if (downkeys & keymap.scancode[EMU_KEY_L]) {
        show_keyboard = !show_keyboard;
    }
    if (show_keyboard) {
        if (!(keys & keymap.scancode[EMU_KEY_O])) {
            if ((keys & keymap.scancode[EMU_KEY_LEFT]) && cursor_x > keyboard_x)
                cursor_x-=2;
            if ((keys & keymap.scancode[EMU_KEY_RIGHT]) && cursor_x < keyboard_x + 319)
                cursor_x+=2;
            if ((keys & keymap.scancode[EMU_KEY_UP]) && cursor_y > keyboard_y)
                cursor_y-=2;
            if ((keys & keymap.scancode[EMU_KEY_DOWN]) && cursor_y < keyboard_y + 91)
                cursor_y+=2;
        }
        if (diffkeys & keymap.scancode[EMU_KEY_O]) {
            int kc_py = (cursor_y - keyboard_y) / KEY_GFX_WIDTH;
            int kc_px = (cursor_x - 6 - kc_pos[kc_py][0] - keyboard_x) / KEY_GFX_WIDTH;
            if (kc_px < 0)
                kc_px = 0;
            int cpc_key = keyb_array[kc_py][kc_px];
            if (cpc_key != 0xff) {
                //printf("key event at %d/%d (%d/%d), key %0x\n", cursor_x, cursor_y, kc_px, kc_py, cpc_key);
                if (keys & keymap.scancode[EMU_KEY_O]) {
                    keyboard_matrix[cpc_key >> 4] &= ~bit_values[cpc_key & 7];
                    if (cpc_key != 0x25)
                        keyboard_matrix[0x25 >> 4] |= bit_values[0x25 & 7];
                    if (cpc_key != 0x27)
                        keyboard_matrix[0x27 >> 4] |= bit_values[0x27 & 7];
                }
                else {
                    if (cpc_key != 0x25 && cpc_key != 0x27) /* sticky modifiers */
                        keyboard_matrix[cpc_key >> 4] |= bit_values[cpc_key & 7];
                }
            }
        }
    }
#define setkey(x) keyboard_matrix[(x) >> 4] &= ~bit_values[(x) & 7]
#define relkey(x) keyboard_matrix[(x) >> 4] |= bit_values[(x) & 7]
    else {
            if (downkeys & keymap.scancode[EMU_KEY_DOWN])
                setkey(0x91);
            if (downkeys & keymap.scancode[EMU_KEY_UP])
                setkey(0x90);
            if (downkeys & keymap.scancode[EMU_KEY_LEFT])
                setkey(0x92);
            if (downkeys & keymap.scancode[EMU_KEY_RIGHT])
                setkey(0x93);
            if (upkeys & keymap.scancode[EMU_KEY_DOWN])
                relkey(0x91);
            if (upkeys & keymap.scancode[EMU_KEY_UP])
                relkey(0x90);
            if (upkeys & keymap.scancode[EMU_KEY_LEFT])
                relkey(0x92);
            if (upkeys & keymap.scancode[EMU_KEY_RIGHT])
                relkey(0x93);

            if (downkeys & keymap.scancode[EMU_KEY_O])
                setkey(0x95);
            if (upkeys & keymap.scancode[EMU_KEY_O])
                relkey(0x95);
            if (downkeys & keymap.scancode[EMU_KEY_X])
                setkey(0x94);
            if (upkeys & keymap.scancode[EMU_KEY_X])
                relkey(0x94);
    }
#if 0
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
#endif
    return 0;

}

static void map_buttons(void)
{
    libgame_buttonmap_t bm[] = {
        {"Start", EMU_KEY_START},
        {"Select", EMU_KEY_SELECT},	/* aka SELECT */
        {"O", EMU_KEY_O},		/* aka B */
        {"X", EMU_KEY_X},		/* aka X */
        {"Square", EMU_KEY_SQUARE},	/* aka A */
        {"Triangle", EMU_KEY_TRIANGLE},	/* aka Y */
        {"L", EMU_KEY_L},
        {"R", EMU_KEY_R},
        {0, 0}
    };
    libgame_map_buttons("caprice/caprice.map", &keymap, bm);
}

int main(int argc, char *argv[]) {
    int i,length;

#ifdef SPMP_USB
    libgame_set_debug(0);
    usbdbg_init();
    usbdbg_redirect_stdio(1);
#endif

    printf("ARM frequency %d\n", GetArmCoreFreq());
    changeARMFreq(333);
    printf("ARM frequency %d\n", GetArmCoreFreq());
    
    libgame_chdir_game();
    _ecos_mkdir("caprice", 0666);
    
    if(!init_linux())
        return 1;

    if (libgame_load_buttons("caprice/caprice.map", &keymap) < 0) {
        map_buttons();
    }


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

#define AUDIO_BUF_SIZE (48000/50)
emu_sound_params_t sp;
uint64_t last_sound = 0;
unsigned int GetTicks(void)
{
    uint64_t ticks = libgame_utime();
    return (unsigned int) ticks / 1000;
}

int init_buffer (void)
{
    pbGPBuffer = (Bitu8*) malloc(128*1024); //need by unzip lib

    pix = screen_base;//(void *)malloc(2 * WRES * (HRES+1));
    if(pix == NULL)
        return 0;

    return 1;
}

void close_buffer (void)
{
#if 1
    if(pix != NULL)
    {
        free(pix);
        pix = NULL;
    }
#endif

    free(pbGPBuffer);
}

/*
   SND DEBUG CODE
*/
#define PBUFSIZE 1024
uint16_t playbuf[2][PBUFSIZE * 2];

void SoundUpdate(int count)
{
  static int sample_count = 0;
  static int cur_buf = 0;
  sample_count += count;
  if (sample_count >= PBUFSIZE *2 * 2) {
        if(ym_playing)
        {
                    int nbSample = PBUFSIZE * 2 * 2 / sizeof(ymsample);
                    ymMusicCompute((void*)s_pMusic,(ymsample*)playbuf[cur_buf],nbSample);
                    //memset(playbuf[cur_buf], 0,  PBUFSIZE * 2 * 2);
        }
        else
        {
            memcpy(playbuf[cur_buf], pbSndStream, PBUFSIZE * 2 * 2);
            //memset(playbuf[cur_buf], 0, PBUFSIZE * 2 * 2);
            pbSndStream += PBUFSIZE * 2 * 2;

            if (pbSndStream >= pbSndBufferEnd) 
                pbSndStream = pbSndBuffer;
        }
        sp.buf = (uint8_t *)playbuf[cur_buf];
        sp.buf_size = PBUFSIZE * 2 * 2;
        //uint64_t now = libgame_utime();
        emuIfSoundPlay(&sp);
        //next = libgame_utime() + (PBUFSIZE * 1000000 / 48000) - 200;
        //printf("play %d %d next %d\n", (unsigned int)now, (unsigned int)(libgame_utime() - now), (unsigned int)next);
        cur_buf = !cur_buf;
        sample_count = 0;
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
//    SDL_PauseAudio(val);  
}


int SoundSetup (void)
{
    return PBUFSIZE * 2 * 2;//audio_spec->size; // size is samples * channels * bytes per sample (1 or 2)
}


void SoundInit (void)
{
    sp.rate = 44100;//48000;
    sp.channels = 2;
    sp.depth = 0;
    sp.callback = 0;
    //sp.buf_size = AUDIO_BUF_SIZE * 2 * 2;
    emuIfSoundInit(&sp);
}

void SoundClose (void)
{
    emuIfSoundCleanup();
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

