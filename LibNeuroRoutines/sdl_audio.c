/**
 * sdl_audio.c - SDL audio subsystem for Neuromancer PC Speaker tones.
 *
 * Provides real-time square wave synthesis from asm_get_sample()
 * through SDL's audio callback. Supports track switching, volume control,
 * and mute.
 *
 * Architecture:
 *   The original DOS game used IRQ8 (PIT channel 0 at ~236.7Hz) to call
 *   a handler that fetched the next frequency divisor from asm_get_sample()
 *   and wrote it to PIT channel 2 (PC speaker).
 *
 *   Here we use SDL_AudioSpec at 44100Hz, generating 8-bit mono PCM
 *   square waves at whatever frequency asm_get_sample() produces.
 */

#include "sdl_audio.h"
#include <SDL.h>
#include <string.h>

/* Extern declarations for asm_audio.c functions */
extern void asm_set_track_on_playback(int track_num);
extern uint16_t asm_get_sample(void);

/* Audio state */
static int g_audio_initialized = 0;
static int g_audio_muted = 0;
static int g_current_track = 0;
static int g_track_playing = 0;
static float g_volume = 0.5f;

/* Square wave state */
static int g_speaker_on = 0;
static int g_half_samples_left = 0;

/*
 * Generate one sample of PCM audio.
 * asm_get_sample() returns a 16-bit divisor for PIT channel 2.
 * freq = 1193180 / divisor
 * Half-period in samples = (1 / freq) / 2 * sample_rate
 *                        = (divisor * 44100) / (2 * 1193180)
 *                        = divisor * 44100 / 2386360
 */
static unsigned char generate_pc_sample(void)
{
    if (g_audio_muted || !g_track_playing) {
        return 128;
    }

    if (g_half_samples_left <= 0) {
        uint16_t divisor = asm_get_sample();
        if (divisor == 0) {
            g_speaker_on = 0;
            g_half_samples_left = 0;
            return 128;
        }
        g_speaker_on = !g_speaker_on;
        g_half_samples_left = (int)((divisor * 44100.0) / 2386360.0);
        if (g_half_samples_left < 1) g_half_samples_left = 1;
    }

    g_half_samples_left--;

    if (g_speaker_on) {
        int val = (int)(128 + 127.0f * g_volume);
        return (val > 255) ? 255 : (unsigned char)val;
    } else {
        int val = (int)(128 - 127.0f * g_volume);
        return (val < 0) ? 0 : (unsigned char)val;
    }
}

static void sdl_audio_callback(void *userdata, Uint8 *audio_buf, int audio_len)
{
    (void)userdata;
    unsigned char *p = (unsigned char *)audio_buf;
    int i;

    if (g_audio_muted || !g_track_playing) {
        memset(audio_buf, 128, audio_len);
        return;
    }

    for (i = 0; i < audio_len; i++) {
        *p++ = generate_pc_sample();
    }
}

int sdl_audio_init(void)
{
    if (g_audio_initialized) return 0;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        return -1;
    }

    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 4096;
    want.callback = sdl_audio_callback;
    want.userdata = NULL;

    if (SDL_OpenAudio(&want, &have) < 0) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return -1;
    }

    g_audio_initialized = 1;
    g_audio_muted = 0;
    g_current_track = 0;
    g_track_playing = 0;
    g_volume = 0.5f;
    g_speaker_on = 0;
    g_half_samples_left = 0;

    return 0;
}

void sdl_audio_shutdown(void)
{
    if (g_audio_initialized) {
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        g_audio_initialized = 0;
    }
}

void sdl_audio_play_track(int track_num)
{
    if (!g_audio_initialized) return;
    if (track_num == g_current_track && g_track_playing) return;

    g_track_playing = 0;
    SDL_PauseAudio(1);

    asm_set_track_on_playback(track_num);
    g_current_track = track_num;
    g_speaker_on = 0;
    g_half_samples_left = 0;

    /* Reset track to beginning */
    asm_set_track_on_playback(track_num);
    g_speaker_on = 0;
    g_half_samples_left = 0;
    g_track_playing = 1;

    SDL_PauseAudio(0);
}

void sdl_audio_stop(void)
{
    if (!g_audio_initialized) return;
    g_track_playing = 0;
    g_current_track = 0;
    SDL_PauseAudio(1);
}

void sdl_audio_set_volume(float volume)
{
    g_volume = (volume < 0.0f) ? 0.0f : ((volume > 1.0f) ? 1.0f : volume);
}

void sdl_audio_toggle_mute(void)
{
    g_audio_muted = !g_audio_muted;
}

int sdl_audio_is_playing(void)
{
    return g_track_playing && !g_audio_muted;
}

int sdl_audio_get_track(void)
{
    return g_current_track;
}
