#include "globals.h"
#include "data.h"
#include "resource_manager.h"
#include "scene_control.h"
#include "drawing_control.h"
#include "window_animation.h"
#include <neuro_routines.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../sfml_stub.h"
<<<<<<< HEAD
#include <SDL.h>
=======
>>>>>>> 485b19a5663aa85ba642d3ad6c4f5f1f148bc335
#include "sdl_audio.h"

/*
 * Video mode.
 */
static sfVideoMode g_mode = {
	640, 400, 32
};

/*
 * Main window.
 */
static sfRenderWindow *g_window = NULL;

/*
 * Texture to be mapped on screen;
 */
static sfTexture *g_texture = NULL;

/*
 * Texture scale factors.
 */
static float g_scale_x = 0;
static float g_scale_y = 0;

/*
 * Timer.
 */
sfClock *g_timer = NULL;

/*
 * Exit mark.
 */
uint8_t g_exit_game = 0;

/*
 * x86 emulator.
 */
cpu_t *g_cpu = NULL;

/*
 * Frame rate limiting.
 */
#define TARGET_FPS 30
#define FRAME_TIME_US (1000000 / TARGET_FPS)

/*
 * Cached display sprite (avoid creating/destroying every frame).
 */
static sfSprite *g_display_sprite = NULL;
static float g_last_scale_x = -1.0f;
static float g_last_scale_y = -1.0f;

/*
 * Cursor update tracking (only update when mouse actually moved).
 */
static int g_mouse_moved = 0;

void update_cursor()
{
	sfVector2i mouse_pos = sfMouse_getPositionRenderWindow(g_window);

	if (mouse_pos.x < 0)
	{
		mouse_pos.x = 0;
	}
	else if (mouse_pos.x >(int)g_mode.width)
	{
		mouse_pos.x = g_mode.width;
	}
	if (mouse_pos.y < 0)
	{
		mouse_pos.y = 0;
	}
	else if (mouse_pos.y >(int)g_mode.height)
	{
		mouse_pos.y = g_mode.height;
	}

	float mouse_pos_x = (mouse_pos.x / g_scale_x);
	float mouse_pos_y = (mouse_pos.y / g_scale_y);

	uint16_t new_left = (uint16_t)mouse_pos_x;
	uint16_t new_top = (uint16_t)mouse_pos_y;

	if (g_sprite_chain[SCI_CURSOR].left != new_left ||
	    g_sprite_chain[SCI_CURSOR].top != new_top) {
		g_sprite_chain[SCI_CURSOR].left = new_left;
		g_sprite_chain[SCI_CURSOR].top = new_top;
		g_mouse_moved = 1;
	}
}

sfKeyCode sfHandleTextInput(uint32_t u32_char,
	char *string, uint32_t size, int digits_only, int insert)
{
	size_t l = strlen(string);

	/* printable ascii */
	if ((digits_only && (u32_char >= 0x30 && u32_char <= 0x39)) ||
		(!digits_only && (u32_char >= 0x20 && u32_char <= 0x7e)))
	{
		if (insert)
		{
			if (l != 0 || l + 1 < size)
			{
				*string = (char)u32_char;
			}
			else
			{
				return sfKeyUnknown;
			}
		}
		else
		{
			if (l + 1 < size)
			{
				string[l] = (char)u32_char;
				string[l + 1] = 0;
			}
			else
			{
				return sfKeyUnknown;
			}
		}
	} /* backspace */
	else if (u32_char == 0x08)
	{
		if (insert)
		{
			/* should be handled by caller */
			return sfKeyBack;
		}
		if (l != 0)
		{
			string[l - 1] = 0;
		}
		return sfKeyBack;
	}
	else if (u32_char == 0x0d)
	{
		return sfKeyReturn;
	}
	else if (u32_char == 0x1b)
	{
		return sfKeyEscape;
	}

	return sfKeyCount;
}

void sfSetKeyRepeat(int enabled)
{
	sfRenderWindow_setKeyRepeatEnabled(g_window, (sfBool)enabled);
}

static void render()
{
	drawing_control_draw_sprite_chain_to_vga();

	sfTexture_updateFromPixels(g_texture, g_vga, 320, 200, 0, 0);

	/* Cache display sprite to avoid allocation every frame */
	if (!g_display_sprite)
	{
		g_display_sprite = sfSprite_create();
	}
	sfVector2f scale = { g_scale_x, g_scale_y };

	/* Only update sprite scale if it changed */
	if (g_last_scale_x != g_scale_x || g_last_scale_y != g_scale_y)
	{
		sfSprite_setScale(g_display_sprite, scale);
		g_last_scale_x = g_scale_x;
		g_last_scale_y = g_scale_y;
	}

	sfRenderWindow_clear(g_window, sfBlack);

	sfSprite_setTexture(g_display_sprite, g_texture, 1);
	sfRenderWindow_drawSprite(g_window, g_display_sprite, NULL);

	if (g_window_animation_renderer_hook)
	{
		g_window_animation_renderer_hook(g_window, &scale);
	}

	sfRenderWindow_display(g_window);
}

static void reset()
{
	drawing_control_flush_vga();
	drawing_control_flush_sprite_chain();

	g_texture = sfTexture_create(320, 200);

	g_scale_x = (float)g_mode.width / 320;
	g_scale_y = (float)g_mode.height / 200;
}

int main(int argc, char *argv[])
{
	sfEvent event;

	g_window = sfRenderWindow_create(g_mode,
		"NeuromancerWin32", sfClose, NULL);
	if (!g_window)
	{
		return -1;
	}
	sfRenderWindow_setMouseCursorVisible(g_window, sfFalse);

	g_timer = sfClock_create();

	reset();
	g_4bae_init();
	resource_manager_init();
	g_cpu = cpu_new(neuro_cb);
	if (getenv("AUTO_START")) {
		strcpy(g_4bae.name + 2, "Case");
		scene_control_setup_scene(NSID_REAL_WORLD);
	} else {
		scene_control_setup_scene(NSID_MAIN_MENU);
	}

	/* Initialize audio */
	sdl_audio_init();
	sdl_audio_set_volume(0.3f);
	srand((uint32_t)time(NULL));

	while (sfRenderWindow_isOpen(g_window) && !g_exit_game)
	{
		unsigned int frame_start = SDL_GetTicks();
		unsigned int frame_elapsed;

		while (sfRenderWindow_pollEvent(g_window, &event))
		{
			if (event.type == sfEvtClosed)
			{
				sfRenderWindow_close(g_window);
			}
			else if (g_scene.handle_input)
			{
				if (event.ev.key.code == 77) /* M = toggle mute */
				{
					sdl_audio_toggle_mute();
				}
				g_scene.handle_input(&event);
			}
		}

		/* Reset cursor movement flag each frame */
		g_mouse_moved = 0;

		neuro_scene_id_t scene = g_scene.update();
		if (scene != g_scene.id)
		{
			scene_control_setup_scene(scene);
		}

		render();

		/* Frame rate limiting: sleep to maintain TARGET_FPS */
		frame_elapsed = SDL_GetTicks() - frame_start;
		{
			unsigned int frame_budget = 1000 / TARGET_FPS;
			if (frame_elapsed < frame_budget)
			{
				SDL_Delay(frame_budget - frame_elapsed);
			}
		}
	}

	g_scene.deinit();
	resource_manager_deinit();
	cpu_destroy(g_cpu);
	sdl_audio_shutdown();
<<<<<<< HEAD

	/* Free cached rendering objects */
	if (g_display_sprite) sfSprite_destroy(g_display_sprite);
=======
>>>>>>> 485b19a5663aa85ba642d3ad6c4f5f1f148bc335

	sfClock_destroy(g_timer);
	sfRenderWindow_destroy(g_window);
	sfTexture_destroy(g_texture);
	return 0;
}
