#ifndef _SDL_AUDIO_H
#define _SDL_AUDIO_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SDL audio subsystem (PC speaker tone synthesis) */
int sdl_audio_init(void);
void sdl_audio_shutdown(void);
void sdl_audio_play_track(int track_num);
void sdl_audio_stop(void);
void sdl_audio_set_volume(float volume);
void sdl_audio_toggle_mute(void);
int sdl_audio_is_playing(void);
int sdl_audio_get_track(void);

#ifdef __cplusplus
}
#endif

#endif /* _SDL_AUDIO_H */
