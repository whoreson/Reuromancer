#include "window_animation.h"
#include "globals.h"
#include "data.h"
#include "drawing_control.h"
#include "neuro_window_control.h"
#include "resource_manager.h"
#include <neuro_routines.h>
#include <string.h>
#include <assert.h>

fn_window_animation_renderer_hook g_window_animation_renderer_hook = NULL;

typedef struct _page_turning_data_t {
	page_turning_data_t user_data;
	uint16_t l, t, w, h;
	int16_t height;
} _page_turning_data_t;

typedef struct _text_scrolling_data_t {
	text_scrolling_data_t user_data;
	char line[64];
	uint16_t line_length;
	uint16_t max_lines;
	uint16_t l, w, t, b;
	uint8_t *pixels;
	/* runtime state for skip_line support */
	int lines_on_screen;
	int lines_scrolled;
	int next_line;
	int elapsed;
} _text_scrolling_data_t;

typedef struct _screen_fading_data_t {
	screen_fading_data_t user_data;
	int32_t alpha;
} _screen_fading_data_t;

typedef struct window_animation_instance_t {
	window_animation_type_t type;
	union {
		window_folding_data_t folding;
		_screen_fading_data_t fading;
		_text_scrolling_data_t scrolling;
		_page_turning_data_t turning;
	} data;
} window_animation_instance_t;

static window_animation_instance_t g_animation = {
	.type = WA_TYPE_UNKNOWN,
};

static void page_turning_renderer_hook(sfRenderWindow *window, sfVector2f *scale)
{
	_page_turning_data_t *data = &g_animation.data.turning;

	sfRectangleShape *rect = sfRectangleShape_create();
	sfVector2f pos = { data->l, data->t };
	sfVector2f size = { data->w, data->height };
	sfColor color = { 255, 255, 255, 255 };

	sfRectangleShape_setSize(rect, size);
	sfRectangleShape_setPosition(rect, pos);
	sfRectangleShape_setFillColor(rect, color);
	sfRectangleShape_setScale(rect, *scale);

	sfRenderWindow_drawRectangleShape(window, (sfRectangleShape*)rect, NULL);
	sfRectangleShape_destroy(rect);
}

static window_animation_event_t update_page_turning(_page_turning_data_t *_data)
{
	static int elapsed = 0;
	static int dir = 0;

	page_turning_data_t *data = &_data->user_data;
	int passed = sfTime_asMilliseconds(sfClock_getElapsedTime(g_timer));

	if (passed - elapsed <= (int)data->frame_cap)
	{
		return WA_EVENT_NO_EVENT;
	}
	elapsed = passed;

	if (dir == 0)
	{
		_data->height += data->step;

		if (_data->height >= _data->h)
		{
			_data->height = _data->h;
			dir = 1;
			data->redraw();
		}
	}
	else
	{
		_data->height -= data->step;
		_data->t += data->step * 2;

		if (_data->height <= 0)
		{
			dir = 0;
			data->end();
			return WA_EVENT_COMPLETED;
		}
	}

	return WA_EVENT_NO_EVENT;
}

static window_animation_event_t update_text_scrolling(_text_scrolling_data_t *_data)
{
	text_scrolling_data_t *data = &_data->user_data;
	int passed = sfTime_asMilliseconds(sfClock_getElapsedTime(g_timer));

	if (passed - _data->elapsed <= (int)data->frame_cap)
	{
		return WA_EVENT_NO_EVENT;
	}
	_data->elapsed = passed;

	if (_data->next_line)
	{
		char *line = _data->line;
		int has_more = extract_line();

		neuro_window_draw_string(has_more ? line : " ", 0);
		_data->next_line = 0;

		if (!has_more)
		{
			_data->next_line = 1;
			_data->lines_on_screen = 0;
			return WA_EVENT_COMPLETED;
		}
	else if (++_data->lines_on_screen == _data->max_lines)
		{
			_data->lines_on_screen = 0;
			return WA_EVENT_WAIT_FOR_INPUT;
		}
	}
	else
	{
		uint8_t *pix = _data->pixels + sizeof(imh_hdr_t);

		for (int i = _data->t + 1, j = _data->t; i < _data->b; i++, j++)
		{
			memmove(&pix[160 * j + _data->l], &pix[160 * i + _data->l], _data->w);
		}

		if (++_data->lines_scrolled == 8)
		{
			_data->lines_scrolled = 0;
			_data->next_line = 1;
		}	
	}

	return WA_EVENT_NO_EVENT;
}

/* Advances the text scroller to the next line immediately,
 * skipping the remaining pixel-scroll steps. Returns 1 if
 * there is more text to scroll, 0 otherwise. */
int window_animation_skip_line(void)
{
	_text_scrolling_data_t *_data;
	uint8_t *pix;
	int remaining;

	if (g_animation.type != WA_TYPE_TEXT_SCROLLING)
		return 0;

	_data = &g_animation.data.scrolling;

	/* If we're in the "draw next line" phase already, nothing to skip */
	if (_data->next_line)
		return 1;

	/* Force-complete the pixel scroll phase by doing the remaining
	 * memmove shifts so the display doesn't look wrong. */
	pix = _data->pixels + sizeof(imh_hdr_t);
	remaining = 8 - _data->lines_scrolled;
	while (remaining > 0)
	{
		for (int i = _data->t + 1, j = _data->t; i < _data->b; i++, j++)
		{
			memmove(&pix[160 * j + _data->l], &pix[160 * i + _data->l], _data->w);
		}
		remaining--;
	}

	/* Reset scroll state and advance to next line */
	_data->lines_scrolled = 0;
	_data->next_line = 1;
	_data->elapsed = 0;

	/* Immediately draw the next line (same logic as update_text_scrolling) */
	{
		char *line = _data->line;
		int has_more = extract_line();

		neuro_window_draw_string(has_more ? line : " ", 0);
		_data->next_line = 0;

		if (!has_more)
		{
			_data->next_line = 1;
			_data->lines_on_screen = 0;
			g_animation.type = WA_TYPE_UNKNOWN;
			g_window_animation_renderer_hook = NULL;
			return 0;
		}
		else if (++_data->lines_on_screen == _data->max_lines)
		{
			_data->lines_on_screen = 0;
			/* Will return WAIT_FOR_INPUT on next update_text_scrolling call */
		}
	}

	return 1;
}

static void screen_fade_renderer_hook(sfRenderWindow *window, sfVector2f *scale)
{
	sfRectangleShape *fader = sfRectangleShape_create();
	sfVector2f size = { 320, 240 };
	sfColor color = { 0, 0, 0, g_animation.data.fading.alpha };

	sfRectangleShape_setFillColor(fader, color);
	sfRectangleShape_setSize(fader, size);
	sfRectangleShape_setScale(fader, *scale);

	sfRenderWindow_drawRectangleShape(window, (sfRectangleShape*)fader, NULL);
	sfRectangleShape_destroy(fader);
}

static window_animation_event_t update_screen_fading(_screen_fading_data_t *_data)
{
	static int elapsed = 0;

	screen_fading_data_t *data = &_data->user_data;
	int passed = sfTime_asMilliseconds(sfClock_getElapsedTime(g_timer));

	if (passed - elapsed <= (int)data->frame_cap)
	{
		return WA_EVENT_NO_EVENT;
	}
	elapsed = passed;

	if (data->direction == FADE_IN)
	{
		_data->alpha -= data->step;
		if (_data->alpha <= 0)
		{
			_data->alpha = 0;
			return WA_EVENT_COMPLETED;
		}
	}
	else
	{
		_data->alpha += data->step;
		if (_data->alpha >= 255)
		{
			_data->alpha = 255;
			return WA_EVENT_COMPLETED;
		}
	}

	return WA_EVENT_NO_EVENT;
}

static window_animation_event_t update_window_folding(window_folding_data_t *data)
{
	static int frame = 0;
	static int elapsed = 0;

	int passed = sfTime_asMilliseconds(sfClock_getElapsedTime(g_timer));

	if (passed - elapsed <= (int)data->frame_cap)
	{
		return WA_EVENT_NO_EVENT;
	}
	elapsed = passed;

	if (frame == data->total_frames)
	{
		frame = 0;
		return WA_EVENT_COMPLETED;
	}

	window_folding_frame_data_t *frame_data = &data->frame_data[frame++];

	build_text_frame(frame_data->h, frame_data->w, (imh_hdr_t*)data->pixels);
	drawing_control_add_sprite_to_chain(data->sprite_chain_index,
		frame_data->l, frame_data->t, data->pixels, 1);

	return WA_EVENT_NO_EVENT;
}

window_animation_event_t window_animation_update()
{
	window_animation_event_t evt;

	switch (g_animation.type) {
	case WA_TYPE_WINDOW_FOLDING:
		evt = update_window_folding(&g_animation.data.folding);
		break;

	case WA_TYPE_SCREEN_FADING:
		evt = update_screen_fading(&g_animation.data.fading);
		break;

	case WA_TYPE_TEXT_SCROLLING:
		evt = update_text_scrolling(&g_animation.data.scrolling);
		break;

	case WA_TYPE_PAGE_TURNING:
		evt = update_page_turning(&g_animation.data.turning);
		break;

	default:
		return WA_EVENT_COMPLETED;
	}

	if (evt == WA_EVENT_COMPLETED)
	{
		g_animation.type = WA_TYPE_UNKNOWN;
		g_window_animation_renderer_hook = NULL;
	}

	return evt;
}

static void prepare_page_turning()
{
	switch (g_neuro_window.mode) {
	case NWM_PAX:
		g_animation.data.turning.l = 8;
		g_animation.data.turning.t = 12;
		g_animation.data.turning.w = 304;
		g_animation.data.turning.h = 94;
		g_animation.data.turning.height = 0;
		break;

	default:
		{break;}
	}
}

static void prepare_text_scrolling()
{
	switch (g_neuro_window.mode) {
	case NWM_NEURO_UI:
		g_animation.data.scrolling.line_length = 17;
		g_animation.data.scrolling.max_lines = 7;
		g_animation.data.scrolling.l = 88;
		g_animation.data.scrolling.w = 68;
		g_animation.data.scrolling.t = 134;
		g_animation.data.scrolling.b = 191;
		g_animation.data.scrolling.pixels = g_seg010.background;
		memset(g_animation.data.scrolling.line, 0, 64);
		extract_line_prepare(g_animation.data.scrolling.user_data.text,
			g_animation.data.scrolling.line, 17);
		break;

	case NWM_PAX:
		g_animation.data.scrolling.line_length = 38;
		g_animation.data.scrolling.max_lines = 9;
		g_animation.data.scrolling.l = 8;
		g_animation.data.scrolling.w = 304;
		g_animation.data.scrolling.t = 16;
		g_animation.data.scrolling.b = 97;
		g_animation.data.scrolling.pixels = g_seg011.data;
		memset(g_animation.data.scrolling.line, 0, 64);
		extract_line_prepare(g_animation.data.scrolling.user_data.text,
			g_animation.data.scrolling.line, 38);
		break;

	default:
		{break;}
	}
}

void window_animation_setup(window_animation_type_t type, void *data)
{
	g_animation.type = type;

	switch (type) {
	case WA_TYPE_WINDOW_FOLDING:
		memmove(&g_animation.data.folding, data, sizeof(window_folding_data_t));
		break;

	case WA_TYPE_SCREEN_FADING:
		memmove(&g_animation.data.fading.user_data, data, sizeof(screen_fading_data_t));
		g_animation.data.fading.alpha =
			(g_animation.data.fading.user_data.direction == FADE_IN) ? 255 : 0;
		g_window_animation_renderer_hook = screen_fade_renderer_hook;
		break;

	case WA_TYPE_TEXT_SCROLLING:
	 memmove(&g_animation.data.scrolling.user_data, data, sizeof(text_scrolling_data_t));
	 g_animation.data.scrolling.lines_on_screen = 0;
	 g_animation.data.scrolling.lines_scrolled = 0;
	 g_animation.data.scrolling.next_line = 1;
	 g_animation.data.scrolling.elapsed = 0;
	 prepare_text_scrolling();
	 break;

	case WA_TYPE_PAGE_TURNING:
		memmove(&g_animation.data.turning.user_data, data, sizeof(page_turning_data_t));
		prepare_page_turning();
		g_window_animation_renderer_hook = page_turning_renderer_hook;
		break;

	default:
		g_animation.type = WA_TYPE_UNKNOWN;
		break;
	}
}
