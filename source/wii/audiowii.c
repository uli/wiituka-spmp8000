/*! \file audiowii.c
 *  \brief Audio Driver.
 *         Wii Library.
 *
 *  \version 0.4
 *
 *  Audio para 48Khz/16bit/Stereo
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asndlib.h>
#include <mp3player.h>

#include "../global.h"

#define SFX_THREAD_STACKSIZE 1024*128 
#define SFX_THREAD_PRIO 80
#define SFX_THREAD_FRAG_SIZE 1024

static lwpq_t sfx_queue;
static lwp_t sfx_thread;

static u8 sfx_stack[SFX_THREAD_STACKSIZE]
  __attribute__ ((__aligned__ (32)));

static bool sfx_thread_running = false;
static bool sfx_thread_quit = false;
static bool sfx_thread_paused = true;

static bool sb = 0;

static u8 sound_buffer[2][SFX_THREAD_FRAG_SIZE]
  __attribute__ ((__aligned__ (32)));


//punteros al sistema de sonido del pituka
extern unsigned char *pbSndBuffer;
extern unsigned char *pbSndBufferEnd;
extern unsigned char *pbSndStream;

/*! \fn static void audio_switch_buffers()
    \brief Envia el buffer de Wii al DMA.
    \todo hacer esta funcion solo interna.
*/
static void audio_switch_buffers() {

        AUDIO_StopDMA(); 
	AUDIO_InitDMA((u32) sound_buffer[sb], SFX_THREAD_FRAG_SIZE);
	AUDIO_StartDMA();

	LWP_ThreadSignal(sfx_queue);
}

/*! \fn static void * sfx_thread_func(void *arg)
    \brief El hilo que copia el buffer del CPC al buffer que sera enviado a la Wii.

    \param arg No usado.
    \return No usado, siempre NULL.
    \todo hacer esta funcion solo interna.
*/
static void * sfx_thread_func(void *arg) {

	bool next_sb;


	while (true) {
		LWP_ThreadSleep(sfx_queue);



		if (sfx_thread_quit)
			break;

		next_sb = sb ^ 1; //cambia el puntero

                if (!sfx_thread_paused)
                        //memset (sound_buffer[next_sb], 0, SFX_THREAD_FRAG_SIZE);
                //else
                {
			if((pbSndStream + SFX_THREAD_FRAG_SIZE) >= pbSndBufferEnd){
                		memcpy(sound_buffer[next_sb], pbSndStream, (pbSndBufferEnd-pbSndStream)); //copia del cpc al buffer
                    		pbSndStream = pbSndBuffer;           // vuelve al comienzo
			}else{
                		memcpy(sound_buffer[next_sb], pbSndStream, SFX_THREAD_FRAG_SIZE); //copia del cpc al buffer
                		pbSndStream += (SFX_THREAD_FRAG_SIZE); // se prepara para el próximo copiado
			}
		}


		DCFlushRange(sound_buffer[next_sb], SFX_THREAD_FRAG_SIZE); //vacia la cache del envio realizado 
                                                                           //y espera al ok de la CPU
		sb = next_sb; //cambia el puntero para el envio por DMA
	}

	return NULL;
}

/*! \fn void Init_SoundSystem( void )
    \brief Inicia el sistema de sonido crea los hilos y configura el callback.
    \todo hacer esta funcion solo interna.
*/
void Init_SoundSystem( void ) {
	sfx_thread_running = false;
	sfx_thread_quit = false;

	//Init in aSndlib
	//AUDIO_Init(0);

	memset(sfx_stack, 0, SFX_THREAD_STACKSIZE);

	LWP_InitQueue(&sfx_queue);

	s32 res = LWP_CreateThread(&sfx_thread, sfx_thread_func, NULL, sfx_stack,
								SFX_THREAD_STACKSIZE, SFX_THREAD_PRIO);

	if (res) {
		printf("ERROR creating sfx thread: %d\n", res);
		LWP_CloseQueue(sfx_queue);
		return;
	}

	sfx_thread_running = true;

	memset(sound_buffer[0], 0, SFX_THREAD_FRAG_SIZE);
	memset(sound_buffer[1], 0, SFX_THREAD_FRAG_SIZE);

	DCFlushRange(sound_buffer[0], SFX_THREAD_FRAG_SIZE);
	DCFlushRange(sound_buffer[1], SFX_THREAD_FRAG_SIZE);

	//Init in aSndlib
	//AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	AUDIO_RegisterDMACallback(audio_switch_buffers);

	audio_switch_buffers();
}

/*! \fn void Close_SoundSystem( void )
    \brief Para el dma, cierra los hilos y libera memoria.
    \todo hacer esta funcion solo interna.
*/
void Close_SoundSystem( void ) {

	AUDIO_StopDMA();
        usleep(100);
	AUDIO_RegisterDMACallback(NULL);

	if (sfx_thread_running) {
		sfx_thread_quit = true;
		LWP_ThreadBroadcast(sfx_queue);

		LWP_JoinThread(sfx_thread, NULL);
		LWP_CloseQueue(sfx_queue);

		sfx_thread_running = false;

	}

}

/*! \fn void StopSound ( int val )
    \param val inicia el sistema de sonido cuando es 0
    \brief Pausa y reanuda el sonido.
    \note Habria que buscar una forma mejor de hacer esto...
*/
void StopSound ( int val ) {

	sfx_thread_paused = val;

        //clean
        memset (sound_buffer[0], 0, SFX_THREAD_FRAG_SIZE);
        memset (sound_buffer[1], 0, SFX_THREAD_FRAG_SIZE);

	switch(val)
        {
		case 1:
			SoundClose();
	     	        ASND_Init();
			//needed by mp3player
			SND_Pause(0);
 			SND_StopVoice(0);

			break;	

		case 0:
		        if (MP3Player_IsPlaying()) { 
			    MP3Player_Stop(); 
                        }
		        ASND_End();

			SoundInit();
			break;
	}

}

/*! \fn int SoundInit (void)
    \brief Funcion global de inicializacion de sonido.

*/
void SoundInit (void)
{

  Init_SoundSystem();

}
/*! \fn int SoundSetup (void)
    \brief Funcion global de configuracion del sonido.
    \note Indica a la emulacion el tamaño del buffer (x4)
*/
int SoundSetup (void)
{
  return SFX_THREAD_FRAG_SIZE << 2;
}

/*! \fn int SoundInit (void)
    \brief Funcion global de finalizacion del sistema de sonido.
*/
void SoundClose(void) 
{
  Close_SoundSystem();

}

