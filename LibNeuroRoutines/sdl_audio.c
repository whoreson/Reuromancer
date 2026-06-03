/**
 * sdl_audio.c - SDL audio subsystem for Neuromancer PC Speaker tones.
 *
 * The original DOS game used IRQ8 (PIT channel 0 at ~236.7Hz) to call
 * a handler that fetched the next frequency divisor from get_audio_sample()
 * and wrote it to PIT channel 2 (PC speaker).
 *
 * Here we use SDL_AudioSpec at 44100Hz, 8-bit mono PCM.
 * We call get_audio_sample() every ~186 samples (44100/236.7) to match
 * the original timing, then generate square wave PCM at that frequency.
 *
 * Frequency calculation:
 *   freq = 1193180 / divisor  (PC speaker frequency from PIT divisor)
 *   half_period_samples = 44100 / (2 * freq)
 *                        = (44100 * divisor) / (2 * 1193180)
 *                        = divisor / 54.14
 */

#include "sdl_audio.h"
#include <SDL.h>
#include <string.h>
#include <math.h>

/* Extern declarations for asm_audio.c functions */
extern void set_audio_track(int track_num);
extern uint16_t get_audio_sample(void);

/* Audio state */
static int g_audio_initialized = 0;
static int g_audio_muted = 0;
static int g_current_track = 0;
static int g_track_playing = 0;
static float g_volume = 0.5f;

/* Square wave synthesis state */
static int g_speaker_on = 0;        /* Current speaker state (high/low) */
static int g_half_samples_left = 0; /* Samples remaining in current half-period */
static uint16_t g_current_divisor = 0;

/* IRQ8 timing: call get_audio_sample() every 186 samples (~236.7 Hz) */
#define IRQ8_INTERVAL 186  /* 44100 / 236.7 */
static int g_irq8_counter = 0;

/* Silence counter for track end detection */
static int g_silence_count = 0;
#define SILENCE_LIMIT (IRQ8_INTERVAL * 60)  /* 1 second of silence */

static void sdl_tick(void)
{
    /* Call get_audio_sample() at IRQ8 rate (~236.7 Hz) */
    uint16_t divisor = get_audio_sample();
    
    if (divisor == 0) {
        /* Silence - speaker off */
        g_speaker_on = 0;
        g_current_divisor = 0;
        g_half_samples_left = 0;
        g_silence_count++;
        return;
    }
    
    g_silence_count = 0;
    g_current_divisor = divisor;
    
    /* Calculate half-period in samples */
    /* freq = 1193180 / divisor */
    /* half_period = 44100 / (2 * freq) = (44100 * divisor) / 2386360 */
    g_half_samples_left = (int)((divisor * 44100.0) / 2386360.0);
    if (g_half_samples_left < 1) g_half_samples_left = 1;
    
    /* Toggle speaker state */
    g_speaker_on = !g_speaker_on;
}

static unsigned char generate_pc_sample(void)
{
    if (g_audio_muted || !g_track_playing) {
        return 128;
    }
    
    /* Check for track end (1 second of silence) */
    if (g_silence_count >= SILENCE_LIMIT) {
        g_track_playing = 0;
        g_current_track = 0;
        return 128;
    }
    
    /* IRQ8 tick: fetch new frequency at ~236.7 Hz */
    if (g_irq8_counter >= IRQ8_INTERVAL) {
        g_irq8_counter = 0;
        sdl_tick();
    }
    g_irq8_counter++;
    
    if (g_current_divisor == 0 || g_half_samples_left <= 0) {
        return 128;  /* Silence */
    }
    
    if (g_half_samples_left <= 0) {
        /* Toggle speaker state at period boundary */
        g_speaker_on = !g_speaker_on;
        g_half_samples_left = (int)((g_current_divisor * 44100.0) / 2386360.0);
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
    g_current_divisor = 0;
    g_irq8_counter = 0;
    g_silence_count = 0;
    
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
    if (track_num == 0) {
        sdl_audio_stop();
        return;
    }
    if (track_num == g_current_track && g_track_playing) return;
    
    g_track_playing = 0;
    SDL_PauseAudio(1);
    
    set_audio_track(track_num);
    g_current_track = track_num;
    g_speaker_on = 0;
    g_half_samples_left = 0;
    g_current_divisor = 0;
    g_irq8_counter = 0;
    g_silence_count = 0;
    g_track_playing = 1;
    
    SDL_PauseAudio(0);
}

void sdl_audio_stop(void)
{
    if (!g_audio_initialized) return;
    g_track_playing = 0;
    g_current_track = 0;
    g_silence_count = 0;
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
