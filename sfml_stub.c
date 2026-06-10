/**
 * SFML 1.x -> SDL-1.2 compatibility implementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sfml_stub.h"

/* ---- Globals ---- */
static sfRenderWindow *g_sfwindow = NULL;
static Uint8 g_mouse_buttons[3] = {0, 0, 0};
static Uint8 g_keys[128] = {0};  /* Track keyboard state */

/* Queued text event: when SDL_KEYDOWN has unicode, we queue sfEvtTextEntered
 * to be returned on the next pollEvent call (SFML 1.x fires both
 * sfEvtKeyPressed and sfEvtTextEntered for printable keys). */
static int g_text_event_pending = 0;
static sfTextEvent g_text_event_buf;

/* ---- Clock ---- */


sfClock *sfClock_create(void)
{
    sfClock *c = malloc(sizeof(sfClock));
    c->start_time = SDL_GetTicks();
    return c;
}

void sfClock_destroy(sfClock *clock) { free(clock); }

sfTime sfClock_getElapsedTime(sfClock *clock)
{
    return (sfTime)(SDL_GetTicks() - clock->start_time);
}

sfTime sfTime_asMilliseconds(sfTime elapsed) { return elapsed; }

/* ---- Mouse ---- */

int sfMouse_isButtonPressed(sfMouseButton button)
{
    return g_mouse_buttons[button];
}

sfVector2i sfMouse_getPositionRenderWindow(void *window)
{
    sfVector2i pos;
    Uint8 state = SDL_GetMouseState(&pos.x, &pos.y);
    (void)state;
    (void)window;
    /* Return raw window coordinates; caller divides by scale factors. */
    return pos;
}

/* ---- Keyboard ---- */

int sfKeyboard_isKeyPressed(sfKeyCode code)
{
    if (code >= 0 && code < 128)
        return g_keys[code];
    return 0;
}

/* ---- Key code conversion ---- */

static sfKeyCode sdlToSfKey(SDLKey key)
{
    switch (key) {
        case SDLK_a: return sfKeyA;
        case SDLK_b: return sfKeyB;
        case SDLK_c: return sfKeyC;
        case SDLK_d: return sfKeyD;
        case SDLK_e: return sfKeyE;
        case SDLK_f: return sfKeyF;
        case SDLK_g: return sfKeyG;
        case SDLK_h: return sfKeyH;
        case SDLK_i: return sfKeyI;
        case SDLK_j: return sfKeyJ;
        case SDLK_k: return sfKeyK;
        case SDLK_l: return sfKeyL;
        case SDLK_m: return sfKeyM;
        case SDLK_n: return sfKeyN;
        case SDLK_o: return sfKeyO;
        case SDLK_p: return sfKeyP;
        case SDLK_q: return sfKeyQ;
        case SDLK_r: return sfKeyR;
        case SDLK_s: return sfKeyS;
        case SDLK_t: return sfKeyT;
        case SDLK_u: return sfKeyU;
        case SDLK_v: return sfKeyV;
        case SDLK_w: return sfKeyW;
        case SDLK_x: return sfKeyX;
        case SDLK_y: return sfKeyY;
        case SDLK_z: return sfKeyZ;
        case SDLK_0: return sfKeyNum0;
        case SDLK_1: return sfKeyNum1;
        case SDLK_2: return sfKeyNum2;
        case SDLK_3: return sfKeyNum3;
        case SDLK_4: return sfKeyNum4;
        case SDLK_5: return sfKeyNum5;
        case SDLK_6: return sfKeyNum6;
        case SDLK_7: return sfKeyNum7;
        case SDLK_8: return sfKeyNum8;
        case SDLK_9: return sfKeyNum9;
        case SDLK_ESCAPE: return sfKeyEscape;
        case SDLK_LCTRL: return sfKeyLControl;
        case SDLK_LSHIFT: return sfKeyLShift;
        case SDLK_LALT: return sfKeyLAlt;
        case SDLK_SPACE: return sfKeySpace;
        case SDLK_RETURN: return sfKeyReturn;
        case SDLK_BACKSPACE: return sfKeyBack;
        case SDLK_TAB: return sfKeyTab;
        case SDLK_UP: return sfKeyUp;
        case SDLK_DOWN: return sfKeyDown;
        case SDLK_LEFT: return sfKeyLeft;
        case SDLK_RIGHT: return sfKeyRight;
        case SDLK_HOME: return sfKeyHome;
        case SDLK_END: return sfKeyEnd;
        case SDLK_PAGEDOWN: return sfKeyPageDown;
        case SDLK_PAGEUP: return sfKeyPageUp;
        case SDLK_DELETE: return sfKeyDelete;
        case SDLK_INSERT: return sfKeyInsert;
        case SDLK_F1: return sfKeyF1;
        case SDLK_F2: return sfKeyF2;
        case SDLK_F3: return sfKeyF3;
        case SDLK_F4: return sfKeyF4;
        case SDLK_F5: return sfKeyF5;
        case SDLK_F6: return sfKeyF6;
        case SDLK_F7: return sfKeyF7;
        case SDLK_F8: return sfKeyF8;
        case SDLK_F9: return sfKeyF9;
        case SDLK_F10: return sfKeyF10;
        case SDLK_F11: return sfKeyF11;
        case SDLK_F12: return sfKeyF12;
        case SDLK_COMMA: return sfKeyComma;
        case SDLK_PERIOD: return sfKeyPeriod;
        default: return sfKeyUnknown;
    }
}

/* ---- Window ---- */


sfRenderWindow *sfRenderWindow_create(sfVideoMode mode, const char *title,
                                      unsigned long style, void *settings)
{
    sfRenderWindow *win = malloc(sizeof(sfRenderWindow));
    memset(win, 0, sizeof(sfRenderWindow));
    (void)style; (void)settings;

    win->surface = SDL_SetVideoMode(mode.width, mode.height, mode.bitsPerPixel,
                                    SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!win->surface)
        win->surface = SDL_SetVideoMode(mode.width, mode.height,
                                        mode.bitsPerPixel, SDL_SWSURFACE);
    if (!win->surface) { free(win); return NULL; }

    win->vga_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 32, 0, 0, 0, 0);
    win->open = 1;
    g_sfwindow = win;
    SDL_WM_SetCaption(title, NULL);
    SDL_EnableUNICODE(1);
    return win;
}

void sfRenderWindow_destroy(sfRenderWindow *window)
{
    if (window) {
        if (window->vga_surface) SDL_FreeSurface(window->vga_surface);
        free(window);
    }
    g_sfwindow = NULL;
}

int sfRenderWindow_isOpen(sfRenderWindow *w) { return w && w->open; }
void sfRenderWindow_close(sfRenderWindow *w) { if (w) w->open = 0; }

int sfRenderWindow_pollEvent(sfRenderWindow *window, sfEvent *event)
{
    /* Return pending text event first (queued from previous SDL_KEYDOWN).
     * SFML 1.x fires both sfEvtKeyPressed and sfEvtTextEntered for each
     * printable keypress; we emulate this by deferring the text event. */
    if (g_text_event_pending) {
        g_text_event_pending = 0;
        event->type = sfEvtTextEntered;
        event->ev.text.code = 0;
        event->ev.text.unicode = g_text_event_buf.unicode;
        return 1;
    }

    SDL_Event sdl;
    while (SDL_PollEvent(&sdl)) {
        switch (sdl.type) {
            case SDL_QUIT:
                event->type = sfEvtClosed;
                return 1;
            case SDL_KEYDOWN: {
                sfKeyCode k = sdlToSfKey(sdl.key.keysym.sym);
                if (k >= 0 && k < 128) g_keys[k] = 1;
                /* Queue sfEvtTextEntered if unicode char is printable. */
                if (sdl.key.keysym.unicode != 0 &&
                    ((sdl.key.keysym.unicode >= 0x20 && sdl.key.keysym.unicode <= 0x7e) ||
                     sdl.key.keysym.unicode == 0x08 ||   /* backspace */
                     sdl.key.keysym.unicode == 0x0d ||   /* enter */
                     sdl.key.keysym.unicode == 0x1b))    /* escape */
                {
                    g_text_event_buf.code = k;
                    g_text_event_buf.unicode = sdl.key.keysym.unicode;
                    g_text_event_pending = 1;
                }
                event->type = sfEvtKeyPressed;
                event->ev.key.code = k;
                event->ev.key.alt = (sdl.key.keysym.mod & KMOD_ALT) ? 1 : 0;
                event->ev.key.control = (sdl.key.keysym.mod & KMOD_CTRL) ? 1 : 0;
                event->ev.key.shift = (sdl.key.keysym.mod & KMOD_SHIFT) ? 1 : 0;
                event->ev.key.system = (sdl.key.keysym.mod & KMOD_MODE) ? 1 : 0;
                return 1;
            }
            case SDL_KEYUP: {
                sfKeyCode k = sdlToSfKey(sdl.key.keysym.sym);
                if (k >= 0 && k < 128) g_keys[k] = 0;
                event->type = sfEvtKeyReleased;
                event->ev.key.code = k;
                event->ev.key.alt = (sdl.key.keysym.mod & KMOD_ALT) ? 1 : 0;
                event->ev.key.control = (sdl.key.keysym.mod & KMOD_CTRL) ? 1 : 0;
                event->ev.key.shift = (sdl.key.keysym.mod & KMOD_SHIFT) ? 1 : 0;
                event->ev.key.system = (sdl.key.keysym.mod & KMOD_MODE) ? 1 : 0;
                return 1;
            }
            case SDL_MOUSEBUTTONDOWN:
                g_mouse_buttons[sdl.button.button - 1] = 1;
                event->type = sfEvtMouseButtonPressed;
                event->ev.mouseButton.x = sdl.button.x;
                event->ev.mouseButton.y = sdl.button.y;
                event->ev.mouseButton.button = (sfUint8)(sdl.button.button - 1);
                event->ev.mouseButton.alt = 0;
                event->ev.mouseButton.control = 0;
                event->ev.mouseButton.shift = 0;
                return 1;
            case SDL_MOUSEBUTTONUP:
                g_mouse_buttons[sdl.button.button - 1] = 0;
                event->type = sfEvtMouseButtonReleased;
                event->ev.mouseButton.x = sdl.button.x;
                event->ev.mouseButton.y = sdl.button.y;
                event->ev.mouseButton.button = (sfUint8)(sdl.button.button - 1);
                event->ev.mouseButton.alt = 0;
                event->ev.mouseButton.control = 0;
                event->ev.mouseButton.shift = 0;
                return 1;
            case SDL_MOUSEMOTION:
                event->type = sfEvtMouseMoved;
                event->ev.mouseMove.x = sdl.motion.x;
                event->ev.mouseMove.y = sdl.motion.y;
                event->ev.mouseMove.alt = 0;
                event->ev.mouseMove.control = 0;
                event->ev.mouseMove.shift = 0;
                return 1;
            default: break;
        }
    }
    return 0;
}

void sfRenderWindow_setKeyRepeatEnabled(sfRenderWindow *w, sfBool enabled){
    (void)w;
    SDL_EnableKeyRepeat(enabled ? SDL_DEFAULT_REPEAT_DELAY : 0,
                        SDL_DEFAULT_REPEAT_INTERVAL);
}

void sfRenderWindow_setMouseCursorVisible(sfRenderWindow *w, sfBool visible)
{
    (void)w;
    SDL_ShowCursor(visible);
}

void sfRenderWindow_clear(sfRenderWindow *w, sfColor color)
{
    SDL_FillRect(w->surface, NULL, SDL_MapRGB(w->surface->format,
                  color.r, color.g, color.b));
}

static void blit_2x(SDL_Surface *src, SDL_Surface *dst, int x, int y)
{
    int sx, sy;
    for (sy = 0; sy < src->h; sy++)
        for (sx = 0; sx < src->w; sx++) {
            uint32_t *sp = (uint32_t *)src->pixels;
            uint32_t *dp = (uint32_t *)dst->pixels;
            uint32_t p = sp[sy * src->w + sx];
            int dx = x + sx * 2, dy = y + sy * 2;
            int ii, jj;
            for (ii = 0; ii < 2 && dy + ii < dst->h; ii++)
                for (jj = 0; jj < 2 && dx + jj < dst->w; jj++)
                    dp[(dy + ii) * dst->w + (dx + jj)] = p;
        }
}

void sfRenderWindow_drawSprite(sfRenderWindow *w, sfSprite *sprite, void *states)
{
    (void)states;
    /* sprite is sfSprite*, extract position and scale */
    sfSprite *sp = (sfSprite *)sprite;
    blit_2x(w->vga_surface, w->surface,
            (int)(sp->position.x), (int)(sp->position.y));
}

void sfRenderWindow_drawRectangleShape(sfRenderWindow *w, sfRectangleShape *shape, void *states)
{
    (void)states;
    sfRectangleShape *sh = (sfRectangleShape *)shape;
    int x = (int)(sh->pos.x * 2);
    int y = (int)(sh->pos.y * 2);
    int rw = (int)(sh->size.x * 2 * sh->scale.x);
    int rh = (int)(sh->size.y * 2 * sh->scale.y);
    
    SDL_Rect rect = {x, y, rw, rh};
    if (rect.x < w->surface->w && rect.y < w->surface->h) {
        SDL_FillRect(w->surface, &rect,
                     SDL_MapRGBA(w->surface->format,
                                 sh->color.r, sh->color.g,
                                 sh->color.b, sh->color.a));
    }
}

void sfRenderWindow_display(sfRenderWindow *w) { SDL_Flip(w->surface); }

/* ---- Texture ---- */


sfTexture *sfTexture_create(unsigned int width, unsigned int height)
{
    sfTexture *tex = malloc(sizeof(sfTexture));
    tex->width = width;
    tex->height = height;
    tex->pixels = malloc(width * height * 4);
    memset(tex->pixels, 0, width * height * 4);
    return tex;
}

void sfTexture_destroy(sfTexture *tex) {
    if (tex) { free(tex->pixels); free(tex); }
}

void sfTexture_updateFromPixels(sfTexture *tex, const void *pixels,
                                unsigned int width, unsigned int height,
                                unsigned int xOffset, unsigned int yOffset)
{
    (void)xOffset; (void)yOffset;
    memcpy(tex->pixels, pixels, width * height * 4);
    
    /* Copy to vga_surface */
    if (g_sfwindow && g_sfwindow->vga_surface) {
        SDL_PixelFormat *fmt = g_sfwindow->vga_surface->format;
        const uint8_t *src = (const uint8_t *)tex->pixels;
        uint32_t *dst = (uint32_t *)g_sfwindow->vga_surface->pixels;
        int i, j;
        for (i = 0; i < height && i < 200; i++)
            for (j = 0; j < width && j < 320; j++) {
                const uint8_t *bp = src + (i * width + j) * 4;
                uint8_t r = bp[0], g = bp[1], b = bp[2], a = bp[3];
                dst[i * 320 + j] = SDL_MapRGBA(fmt, r, g, b, a);
            }
    }
}

/* ---- Sprite ---- */


sfSprite *sfSprite_create(void)
{
    sfSprite *s = calloc(1, sizeof(sfSprite));
    s->scale = (sfVector2f){1, 1};
    return s;
}

void sfSprite_destroy(sfSprite *s) { free(s); }

void sfSprite_setTexture(sfSprite *s, sfTexture *tex, sfBool smooth)
{ (void)smooth; s->texture = tex; }

void sfSprite_setScale(sfSprite *s, sfVector2f sc) { s->scale = sc; }

/* ---- RectangleShape ---- */


sfRectangleShape *sfRectangleShape_create(void)
{
    sfRectangleShape *s = calloc(1, sizeof(sfRectangleShape));
    s->scale = (sfVector2f){1, 1};
    s->color = sfWhite;
    return s;
}

void sfRectangleShape_destroy(sfRectangleShape *s) { free(s); }

void sfRectangleShape_setSize(sfRectangleShape *s, sfVector2f sz) { s->size = sz; }
void sfRectangleShape_setPosition(sfRectangleShape *s, sfVector2f p) { s->pos = p; }
void sfRectangleShape_setFillColor(sfRectangleShape *s, sfColor c) { s->color = c; }
void sfRectangleShape_setScale(sfRectangleShape *s, sfVector2f sc) { s->scale = sc; }

/* ---- Utility ---- */

void sfSleep(sfTime seconds) { SDL_Delay(seconds); }
