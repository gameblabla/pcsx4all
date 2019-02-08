/* SDL Driver for P.E.Op.S Sound Plugin
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <SDL.h>
#include <stdint.h>
#include "out.h"
#include "spu_config.h"  // senquack - To get spu settings
#include "psxcommon.h"   // senquack - To get emu settings

#define SOUND_BUFFER_SIZE 22050        // Size in bytes
#define ROOM_IN_BUFFER (SOUND_BUFFER_SIZE - buffered_bytes)
static unsigned *sound_buffer = NULL;  // Sample ring buffer
static unsigned int buf_read_pos = 0;
static unsigned int buf_write_pos = 0;
static unsigned waiting_to_feed = 0;   // Set to 1 when emu is waiting for room in output buffer

#ifdef DEBUG_FEED_RATIO
static void update_feed_ratio(void);
float cur_feed_ratio = 1.0f;
static unsigned int new_ratio_val = 0;
static unsigned int total_bytes_consumed = 0;
static unsigned int total_bytes_fed = 0;
static unsigned int dropped_bytes = 0;
static unsigned int missed_bytes = 0;
#endif

//VARS THAT ARE SHARED BETWEEN MAIN THREAD AND AUDIO-CALLBACK THREAD (fence!):
static unsigned buffered_bytes = 0;    // How many bytes are in the ring buffer

// Semaphore for atomic versions of feed/callback functions:
static SDL_sem *sound_sem = NULL;

////////////////////////
// SDL AUDIO CALLBACK //
////////////////////////

// Newer version of SDL audio callback that uses GCC atomic var
static void SOUND_FillAudio(void *unused, Uint8 *stream, int len) {
	uint8_t *out_buf = (uint8_t *)stream;
	uint8_t *in_buf = (uint8_t *)sound_buffer;

	unsigned bytes_to_copy = (len > buffered_bytes) ? buffered_bytes : len;

#ifdef DEBUG_FEED_RATIO
	missed_bytes += len - bytes_to_copy;
	total_bytes_consumed += len;
	update_feed_ratio();
#endif

	if (bytes_to_copy > 0) {
		if (buf_read_pos + bytes_to_copy <= SOUND_BUFFER_SIZE ) {
			memcpy(out_buf, in_buf + buf_read_pos, bytes_to_copy);
		} else {
			unsigned tail = SOUND_BUFFER_SIZE - buf_read_pos;
			memcpy(out_buf, in_buf + buf_read_pos, tail);
			memcpy(out_buf + tail, in_buf, bytes_to_copy - tail);
		}

		buf_read_pos = (buf_read_pos + bytes_to_copy);
		if (buf_read_pos >= SOUND_BUFFER_SIZE)
			buf_read_pos -= SOUND_BUFFER_SIZE;

		// Atomically decrement 'buffered_bytes' by 'bytes_to_copy'
		// TODO: If ever ported to SDL2.0, its API offers portable atomics:
		__sync_fetch_and_sub(&buffered_bytes, bytes_to_copy);
	}

	// If the callback asked for more samples than we had, zero-fill remainder:
	if (len - bytes_to_copy > 0) {
		memset(out_buf + bytes_to_copy, 0, len - bytes_to_copy);
		//printf("SDL audio callback underrun by %d bytes\n", len - bytes_to_copy);
	}

	// Signal emu thread that room is available:
	if (waiting_to_feed)
		SDL_SemPost(sound_sem);
}


static void InitSDL() {
	if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
		SDL_InitSubSystem(SDL_INIT_AUDIO);
	} else {
		SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
	}
}

static void DestroySDL() {
	if (sound_sem)
		SDL_DestroySemaphore(sound_sem);

	if (SDL_WasInit(SDL_INIT_EVERYTHING & ~SDL_INIT_AUDIO)) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	} else {
		SDL_Quit();
	}
}

static int sdl_init(void) {
	if (sound_buffer != NULL) return -1;

	InitSDL();

	SDL_AudioSpec spec;

	if (Config.SyncAudio) {
		sound_sem = SDL_CreateSemaphore(0);
		if (sound_sem)
			printf("Created SDL audio output semaphore successfully.\n");
		else
			printf("Failed to create SDL audio output semaphore, audio will not be synced.\n");
	}
	spec.callback = SOUND_FillAudio;

	spec.freq = 44100;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	//senquack - TODO: Make SDL audio buffer size an adjustable setting
	//spec.samples = 512;
	spec.samples = 1024;	//Made 1024 for now, to match port.cpp's old
	                        // settings ..TODO: Makefile should define

	if (SDL_OpenAudio(&spec, NULL) < 0) {
		DestroySDL();
		return -1;
	}

	sound_buffer = (unsigned *)calloc(SOUND_BUFFER_SIZE,1);
	if (sound_buffer == NULL) {
		printf("-> ERROR: SPU plugin could not allocate %d-byte sound buffer\n", SOUND_BUFFER_SIZE);
		SDL_CloseAudio();
		return -1;
	}

	buf_read_pos = 0;
	buf_write_pos = 0;
	SDL_PauseAudio(0);
	return 0;
}

static void sdl_finish(void) {
	if (sound_buffer == NULL) return;

	SDL_CloseAudio();
	DestroySDL();

	if (sound_buffer) free(sound_buffer);
	sound_buffer = NULL;
}

//senquack - When spu_config.iTempo option is set, this is used to determine
//			 when spu.c decides to fake less samples having been written in
//			 order to force more to be generated (pcsxReARMed hack for slow
//		     devices)
//senquack - Original PCSX-Rearmed dfsound SPU code (for reference):
#if 0
static int sdl_busy(void) {
	int size;
	if (pSndBuffer == NULL) return 1;
	size = iReadPos - iWritePos;
	if (size <= 0) size += iBufSize;
	if (size < iBufSize / 2) return 1;
	return 0;
}
#endif //0
static int sdl_busy(void) {
	// We use the same buffer size as PCSX Rearmed, but don't keep it quite
	//  as full on average, to reduce sound lag. We instead offer flexibly
	//  scheduled SPU updates. (Rearmed tries to keep it 1/2 full)
	//if ((ROOM_IN_BUFFER < SOUND_BUFFER_SIZE/2) || sound_buffer == NULL)
	if ((ROOM_IN_BUFFER < SOUND_BUFFER_SIZE*5/8) || sound_buffer == NULL)
		return 1;

	return 0;
}

//////////////////////////////////////////////////
// EMU SPU -> INTERMEDIATE BUFFER FILL FUNCTION //
//////////////////////////////////////////////////

//Feed samples from emu into intermediate buffer
//Newer sdl_feed() function that uses a shared atomic var to
// coordinate with SDL audio callback function.
static void sdl_feed(void *pSound, int lBytes) {
#ifdef DEBUG_FEED_RATIO
	total_bytes_fed += lBytes;
#endif

	unsigned bytes_to_copy = lBytes;

	if (sound_sem) {
		while (ROOM_IN_BUFFER < lBytes) {
			// Wait until semaphore is posted by audio callback:
			waiting_to_feed = 1;
			SDL_SemWait(sound_sem);
		}
		waiting_to_feed = 0;
	} else {
		// Just drop the samples that cannot fit:
		if (ROOM_IN_BUFFER == 0) {
#ifdef DEBUG_FEED_RATIO
			dropped_bytes += bytes_to_copy;
#endif
			return;
		}

		if (bytes_to_copy > ROOM_IN_BUFFER) bytes_to_copy = ROOM_IN_BUFFER;

#ifdef DEBUG_FEED_RATIO
		dropped_bytes += lBytes - bytes_to_copy;
#endif
	}

	uint8_t *in_buf = (uint8_t *)pSound;
	uint8_t *out_buf = (uint8_t *)sound_buffer;

	if (buf_write_pos + bytes_to_copy <= SOUND_BUFFER_SIZE ) {
		memcpy(out_buf + buf_write_pos, in_buf, bytes_to_copy);
	} else {
		int tail = SOUND_BUFFER_SIZE - buf_write_pos;
		memcpy(out_buf + buf_write_pos, in_buf, tail);
		memcpy(out_buf, in_buf + tail, bytes_to_copy - tail);
	}

	buf_write_pos = (buf_write_pos + bytes_to_copy) % SOUND_BUFFER_SIZE;

	// Atomically increment 'buffered_bytes' by 'bytes_to_copy':
	// TODO: If ever ported to SDL2.0, its API offers portable atomics:
	__sync_fetch_and_add(&buffered_bytes, bytes_to_copy);
}

#ifdef DEBUG_FEED_RATIO
static void update_feed_ratio(void) {
	const int calls_between_new_ratio = 5;
	static int calls_until_new_ratio = calls_between_new_ratio;
	calls_until_new_ratio--;
	if (calls_until_new_ratio > 0)
		return;

	// Avoid possible div-by-zero:
	if (total_bytes_consumed == 0)
		total_bytes_consumed = 1;

	calls_until_new_ratio = calls_between_new_ratio;
	cur_feed_ratio = (float)total_bytes_fed / (float)total_bytes_consumed;
	new_ratio_val = 1;
	total_bytes_fed = total_bytes_consumed = 0;

	printf("fr: %f   buf: %d   drop: %d   miss: %d\n", cur_feed_ratio, buffered_bytes, dropped_bytes, missed_bytes);

	dropped_bytes = missed_bytes = 0;
}
#endif //DEBUG_FEED_RATIO

void out_register_sdl(struct out_driver *drv)
{
	drv->name = "sdl";
	drv->init = sdl_init;
	drv->finish = sdl_finish;
	drv->busy = sdl_busy;
	drv->feed = sdl_feed;
}
