#ifndef _SFML_STUB_H
#define _SFML_STUB_H

#include <SDL.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t sfUint8;
typedef uint16_t sfUint16;
typedef uint32_t sfUint32;
typedef int8_t sfInt8;
typedef int16_t sfInt16;
typedef int32_t sfInt32;
typedef int sfBool;

#define sfTrue 1
#define sfFalse 0

typedef sfUint32 sfTime;

typedef struct { sfUint8 r, g, b, a; } sfColor;
typedef struct { sfInt32 x, y; } sfVector2i;
typedef struct { sfUint32 x, y; } sfVector2u;
typedef struct { float x, y; } sfVector2f;
typedef struct { sfInt32 left, top, width, height; } sfIntRect;
typedef struct { sfUint32 width, height, bitsPerPixel; } sfVideoMode;
typedef struct { sfUint32 depthBits, stencilBits, antiAliasingLevel, majorVersion, minorVersion; } sfContextSettings;
typedef struct { sfUint32 value; } sfRenderStates;

typedef enum {
    sfEvtNone = 0, sfEvtResized, sfEvtClosed, sfEvtKeyPressed, sfEvtKeyReleased,
    sfEvtTextEntered, sfEvtMouseWheelMoved, sfEvtMouseButtonPressed,
    sfEvtMouseButtonReleased, sfEvtMouseMoved, sfEvtLostFocus, sfEvtGainedFocus,
    sfEvtMouseLeft, sfEvtMouseEntered
} sfEventType;

typedef enum {
    sfKeyUnknown = 0, sfKeyA, sfKeyB, sfKeyC, sfKeyD, sfKeyE, sfKeyF, sfKeyG,
    sfKeyH, sfKeyI, sfKeyJ, sfKeyK, sfKeyL, sfKeyM, sfKeyN, sfKeyO, sfKeyP,
    sfKeyQ, sfKeyR, sfKeyS, sfKeyT, sfKeyU, sfKeyV, sfKeyW, sfKeyX, sfKeyY,
    sfKeyZ, sfKeyNum0, sfKeyNum1, sfKeyNum2, sfKeyNum3, sfKeyNum4, sfKeyNum5,
    sfKeyNum6, sfKeyNum7, sfKeyNum8, sfKeyNum9, sfKeyLShift, sfKeyRShift,
    sfKeyLControl, sfKeyRControl, sfKeyLAlt, sfKeyRAlt, sfKeyLSystem,
    sfKeyRSystem, sfKeyMenu, sfKeyReturn = 66, sfKeyEscape = 67,
    sfKeyBackspace = 68, sfKeyBack = 68, sfKeyTab = 69, sfKeySpace = 70,
    sfKeyDelete = 71, sfKeyUp = 72, sfKeyDown = 73, sfKeyLeft = 74,
    sfKeyRight = 75, sfKeyPageUp = 76, sfKeyPageDown = 77, sfKeyHome = 78,
    sfKeyEnd = 79, sfKeyInsert = 80, sfKeyF1 = 81, sfKeyF2, sfKeyF3,
    sfKeyF4, sfKeyF5, sfKeyF6, sfKeyF7, sfKeyF8, sfKeyF9, sfKeyF10,
    sfKeyF11, sfKeyF12, sfKeyComma = 82, sfKeyPeriod = 83, sfKeyCount = 84
} sfKeyCode;

typedef enum { sfMouseLeft = 0, sfMouseRight, sfMouseMiddle } sfMouseButton;

typedef struct { sfKeyCode code; sfUint8 alt, control, shift, system; } sfKeyEvent;
typedef struct { sfUint8 code; uint32_t unicode; } sfTextEvent;

typedef struct {
    sfEventType type;
    union {
        struct { sfUint32 width, height; } resized;
        sfKeyEvent key;
        sfTextEvent text;
        struct { int x, y; sfUint8 button, alt, control, shift; } mouseButton;
        struct { int x, y; sfUint8 alt, control, shift; } mouseMove;
        struct { sfUint8 x, y, delta, alt, control, shift; } mouseWheelMoved;
    } ev;
} sfEvent;

/* Forward declarations */
struct sfTexture;
struct sfImage;
struct sfClock;

typedef struct { SDL_Surface *surface, *vga_surface; sfBool open; } sfRenderWindow;
typedef struct { uint32_t *pixels; int width, height; } sfTexture;
typedef struct { SDL_Surface *srf; } sfImage;
typedef struct { sfVector2f position; sfColor fillColor, outlineColor; float outlineThickness; } sfShape;
typedef struct { sfVector2f pos, size, scale; sfColor color; } sfRectangleShape;
typedef struct { sfShape shape; float radius; sfUint32 pointCount; } sfCircleShape;
typedef struct { sfShape shape; sfUint32 pointCount; } sfConvexShape;
typedef struct { sfUint32 vertexCount; } sfVertexArray;
typedef struct { sfTexture *texture; sfVector2f position, scale; sfColor color; } sfSprite;
typedef struct { } sfSoundBuffer;
typedef struct { } sfSound;
typedef struct sfClock { sfUint32 start_time; } sfClock;

/* Window styles */
#define sfClose 1
#define sfResize 2
#define sfTitle 4

/* Colors */
#define sfBlack       ((sfColor){0,0,0,255})
#define sfWhite       ((sfColor){255,255,255,255})
#define sfRed         ((sfColor){255,0,0,255})
#define sfGreen       ((sfColor){0,255,0,255})
#define sfBlue        ((sfColor){0,0,255,255})
#define sfYellow      ((sfColor){255,255,0,255})
#define sfMagenta     ((sfColor){255,0,255,255})
#define sfCyan        ((sfColor){0,255,255,255})
#define sfTransparent ((sfColor){0,0,0,0})

/* Clock */
sfClock *sfClock_create(void);
void sfClock_destroy(sfClock *);
sfTime sfClock_getElapsedTime(sfClock *);
sfTime sfTime_asMilliseconds(sfTime);

/* Input */
int sfMouse_isButtonPressed(sfMouseButton);
sfVector2i sfMouse_getPositionRenderWindow(void *);
int sfKeyboard_isKeyPressed(sfKeyCode);
void sfSleep(sfTime);

/* Window */
sfRenderWindow *sfRenderWindow_create(sfVideoMode, const char *, unsigned long, void *);
void sfRenderWindow_destroy(sfRenderWindow *);
sfBool sfRenderWindow_isOpen(sfRenderWindow *);
void sfRenderWindow_close(sfRenderWindow *);
sfBool sfRenderWindow_pollEvent(sfRenderWindow *, sfEvent *);
void sfRenderWindow_setKeyRepeatEnabled(sfRenderWindow *, sfBool);
void sfRenderWindow_setMouseCursorVisible(sfRenderWindow *, sfBool);
void sfRenderWindow_setTitle(sfRenderWindow *, const sfUint8 *);
void sfRenderWindow_clear(sfRenderWindow *, sfColor);
void sfRenderWindow_setFramerateLimit(sfRenderWindow *, sfUint32);
void sfRenderWindow_setVerticalSyncEnabled(sfRenderWindow *, sfBool);
void sfRenderWindow_display(sfRenderWindow *);

/* Drawing */
void sfRenderWindow_drawSprite(sfRenderWindow *, sfSprite *, void *);
void sfRenderWindow_drawShape(sfRenderWindow *, sfShape *, sfRenderStates *);
void sfRenderWindow_drawCircleShape(sfRenderWindow *, sfCircleShape *, sfRenderStates *);
void sfRenderWindow_drawConvexShape(sfRenderWindow *, sfConvexShape *, sfRenderStates *);
void sfRenderWindow_drawVertexArray(sfRenderWindow *, sfVertexArray *, sfRenderStates *);
void sfRenderWindow_drawRectangleShape(sfRenderWindow *, sfRectangleShape *, void *);

/* Texture */
sfTexture *sfTexture_createFromFile(const char *, sfVector2i);
sfTexture *sfTexture_create(unsigned int, unsigned int);
void sfTexture_destroy(sfTexture *);
void sfTexture_setSmooth(sfTexture *, sfBool);
sfVector2u sfTexture_getSize(sfTexture *);
void sfTexture_updateFromPixels(sfTexture *, const void *, unsigned int, unsigned int, unsigned int, unsigned int);

/* Sprite */
sfSprite *sfSprite_create(void);
void sfSprite_destroy(sfSprite *);
void sfSprite_setTexture(sfSprite *, sfTexture *, sfBool);
void sfSprite_setPosition(sfSprite *, sfVector2f);
void sfSprite_setScale(sfSprite *, sfVector2f);
void sfSprite_setColor(sfSprite *, sfColor);
void sfSprite_setRotation(sfSprite *, float);
void sfSprite_setOrigin(sfSprite *, sfVector2f);
void sfSprite_setLocalOrigin(sfSprite *, sfVector2f);
void sfSprite_setCenter(sfSprite *, sfVector2f);
float sfSprite_getRotation(sfSprite *);

/* Rectangle */
sfRectangleShape *sfRectangleShape_create(void);
void sfRectangleShape_destroy(sfRectangleShape *);
void sfRectangleShape_setSize(sfRectangleShape *, sfVector2f);
void sfRectangleShape_setScale(sfRectangleShape *, sfVector2f);
void sfRectangleShape_setPosition(sfRectangleShape *, sfVector2f);
void sfRectangleShape_setFillColor(sfRectangleShape *, sfColor);
void sfRectangleShape_setOutlineColor(sfRectangleShape *, sfColor);
void sfRectangleShape_setOutlineThickness(sfRectangleShape *, float);

/* Shape */
sfShape *sfShape_create(void);
void sfShape_destroy(sfShape *);
void sfShape_setPosition(sfShape *, sfVector2f);
void sfShape_setScale(sfShape *, sfVector2f);
void sfShape_setFillColor(sfShape *, sfColor);
void sfShape_setOutlineColor(sfShape *, sfColor);
void sfShape_setOutlineThickness(sfShape *, float);

/* Circle */
void sfCircleShape_create(sfCircleShape *, float, sfUint32);
void sfCircleShape_destroy(sfCircleShape *);
void sfCircleShape_setPosition(sfCircleShape *, sfVector2f);
void sfCircleShape_setFillColor(sfCircleShape *, sfColor);
void sfCircleShape_setOutlineColor(sfCircleShape *, sfColor);
void sfCircleShape_setRadius(sfCircleShape *, float);
void sfCircleShape_setPointCount(sfCircleShape *, sfUint32);
void sfCircleShape_setScale(sfCircleShape *, sfVector2f);
void sfCircleShape_setRotation(sfCircleShape *, float);
void sfCircleShape_setOrigin(sfCircleShape *, sfVector2f);
void sfCircleShape_setLocalOrigin(sfCircleShape *, sfVector2f);
void sfCircleShape_setCenter(sfCircleShape *, sfVector2f);
float sfCircleShape_getRotation(sfCircleShape *);

/* Convex */
void sfConvexShape_create(sfConvexShape *, sfUint32);
void sfConvexShape_destroy(sfConvexShape *);
void sfConvexShape_setFillColor(sfConvexShape *, sfColor);
void sfConvexShape_setOutlineColor(sfConvexShape *, sfColor);
void sfConvexShape_setPoint(sfConvexShape *, sfUint32, sfVector2f);
void sfConvexShape_setPointCount(sfConvexShape *, sfUint32);
sfUint32 sfConvexShape_getPointCount(sfConvexShape *);
void sfConvexShape_setScale(sfConvexShape *, sfVector2f);
void sfConvexShape_setRotation(sfConvexShape *, float);
void sfConvexShape_setOrigin(sfConvexShape *, sfVector2f);
void sfConvexShape_setLocalOrigin(sfConvexShape *, sfVector2f);
void sfConvexShape_setCenter(sfConvexShape *, sfVector2f);
float sfConvexShape_getRotation(sfConvexShape *);

/* Vertex Array */
void sfVertexArray_create(sfVertexArray *);
void sfVertexArray_destroy(sfVertexArray *);
void sfVertexArray_setVertexCount(sfVertexArray *, sfUint32);
void sfVertexArray_setVertex(sfVertexArray *, sfUint32, sfVector2f, sfColor);
sfUint32 sfVertexArray_getVertexCount(sfVertexArray *);
void sfVertexArray_clear(sfVertexArray *);
void sfVertexArray_append(sfVertexArray *, sfVector2f, sfColor);

/* Sound */
sfSoundBuffer *sfSoundBuffer_createFromFile(const sfUint8 *);
void sfSoundBuffer_destroy(sfSoundBuffer *);
sfTime sfSoundBuffer_getDuration(sfSoundBuffer *);
sfSound *sfSound_create(void);
void sfSound_destroy(sfSound *);
void sfSound_setBuffer(sfSound *, sfSoundBuffer *);
void sfSound_play(sfSound *);
void sfSound_stop(sfSound *);
void sfSound_setVolume(sfSound *, sfUint32);
void sfSound_setPlayingOffset(sfSound *, sfTime);

/* Image */
sfImage *sfImage_createFromFile(const char *);
void sfImage_destroy(sfImage *);
sfVector2u sfImage_getSize(sfImage *);
sfUint8 *sfImage_getPixelsPtr(sfImage *);
void sfImage_copy(sfImage *, const sfImage *, sfInt32, sfInt32, sfIntRect, sfBool);

#endif /* _SFML_STUB_H */
