/**
 * Copyright (c) 2018 Henadzi Matuts
 */

#ifndef _NEURO_ROUTINES_H
#define _NEURO_ROUTINES_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Non-Windows: LIBNEUROAPI is a no-op */

#ifdef _MSC_VER
#ifdef LIBNEURO
#define LIBNEUROAPI __declspec(dllexport)
#else
#define LIBNEUROAPI __declspec(dllimport)
#endif
#else
#define LIBNEUROAPI
#endif

#pragma pack(push, 2)

typedef struct imh_hdr_t {
	uint16_t dx;
	uint16_t dy;
	uint16_t width;
	uint16_t height;
} imh_hdr_t;

typedef struct resource_t {
	int file;
	char *name;
	long offset;
	size_t size;
} resource_t;

typedef enum spite_chain_index_t {
	SCI_CURSOR = 0,
	SCI_NEURO_MENU = 2,
	SCI_DIALOG_BUBBLE = 3,
	SCI_CHARACTER = 4,
	SCI_LEVEL_BG = 9,
	SCI_BACKGRND = 10,
	SCI_TOTAL = 11
} spite_chain_index_t;

typedef struct sprite_layer_t {
	uint8_t flags; /* 1st bit - active, 5th bit - opaque */
	uint8_t update; /* =1 - update; =2 - update, then delete */
	uint16_t left;
	uint16_t top;
	uint16_t new_left;
	uint16_t new_top;
	imh_hdr_t sprite_hdr;
	uint16_t pixels_segt;
	uint16_t pixels_offt;
	imh_hdr_t _sprite_hdr;
	uint16_t _pixels_segt;
	uint16_t _pixels_offt;
} sprite_layer_t;

typedef struct neuro_button_t {
	uint16_t left;
	uint16_t top;
	uint16_t right;
	uint16_t bottom;
	uint16_t code;
	char label;
	uint8_t padding;
} neuro_button_t;

typedef struct neuro_menu_t {
	uint16_t left;   // 65FA
	uint16_t top;    // 65FC
	uint16_t right;  // 65FE
	uint16_t bottom; // 6600

	uint16_t inner_left;   // 6602
	uint16_t inner_top;    // 6604
	uint16_t inner_right;  // 6606
	uint16_t inner_bottom; // 6608

	uint16_t _inner_left;   // 660A
	uint16_t _inner_top;    // 660C
	uint16_t _inner_right;  // 660E
	uint16_t _inner_bottom; // 6610

	uint16_t mode; // 6612

	uint16_t items_count; // 6614
	neuro_button_t items[16];

	uint16_t width;
	uint16_t pixels_segt;
	uint16_t pixels_offt;
} neuro_menu_t;

typedef struct bih_hdr_t {
	uint16_t cb_offt;                 // a8e8
	uint16_t cb_segt;                 // a8ea
	uint16_t ctrl_struct_addr;        // a8ec
	uint16_t text_offset;             // a8ee
	uint16_t bytecode_array_offt[3];  // a8f0
	uint16_t init_obj_code_offt[3];   // a8f6
	uint16_t unknown[10];             // a8fc
	/* the rest of bih file */
} bih_hdr_t;

typedef struct bg_animation_control_table_t {
	uint16_t total_frames;
	uint8_t *first_frame_data;
	uint8_t *first_frame_bytes;
	uint16_t sleep;
	uint16_t curr_frame;
} bg_animation_control_table_t;

typedef struct anh_hdr_t {
	uint16_t anh_entries;
	/* first entry hdr */
} anh_hdr_t;

typedef struct anh_entry_hdr_t {
	uint16_t entry_size;
	uint16_t total_frames;
	/* anh_frame_data_t first_frame_data */
	/* another frames data */
	/* anh_frame_hdr first_frame_hdr */
	/* another frames */
} anh_entry_hdr_t;

typedef struct anh_frame_data_t {
	uint16_t frame_sleep;
	uint16_t frame_offset;
} anh_frame_data_t;

typedef struct anh_frame_hdr {
	uint8_t bg_x_offt;
	uint8_t bg_y_offt;
	uint8_t frame_width;
	uint8_t frame_height;
	/* rle encoded frame bytes */
} anh_frame_hdr;

typedef struct roompos_level_t {
	uint8_t roompos[5][4];
} roompos_level_t;

typedef struct roompos_t {
	roompos_level_t roompos_level[58];
} roompos_t;

typedef enum ui_panel_mode_t {
	UI_PM_CASH = 0,
	UI_PM_CON,
	UI_PM_TIME,
	UI_PM_DATE
} ui_panel_mode_t;

/*
 * Resource tables.
 */
LIBNEUROAPI extern resource_t g_res_imh[29];
LIBNEUROAPI extern resource_t g_res_pic[56];
LIBNEUROAPI extern resource_t g_res_bih[61];
LIBNEUROAPI extern resource_t g_res_anh[22];
LIBNEUROAPI extern resource_t g_res_txh[2];
LIBNEUROAPI extern resource_t g_res_savegame;

/*
 * Font table.
 */
LIBNEUROAPI extern uint8_t cp437_font[1024];

/*
 * Decompression routines.
 */
LIBNEUROAPI int decompress_imh(uint8_t *src, uint8_t *dst);
LIBNEUROAPI int decompress_pic(uint8_t *src, uint8_t *dst);
LIBNEUROAPI int decompress_bih(uint8_t *src, uint8_t *dst);
LIBNEUROAPI int decompress_anh(uint8_t *src, uint8_t *dst);
LIBNEUROAPI int decompress_txh(uint8_t *src, uint8_t *dst);

LIBNEUROAPI int huffman_decompress(uint8_t *src, uint8_t *dst);
LIBNEUROAPI int decode_rle(uint8_t *src, uint32_t len, uint8_t *dst);

/*
 * Text routines.
 */
LIBNEUROAPI void build_character(char c, uint8_t *dst);
LIBNEUROAPI void build_string(char *string, uint32_t w, uint32_t h,
		uint32_t l, uint32_t t, uint8_t *dst);


LIBNEUROAPI void build_text_frame(uint32_t h, uint32_t w, imh_hdr_t *dst);

/*
 * Sound stuff.
 */
LIBNEUROAPI int build_track_waveform(int track_num, uint8_t *waveform, int len);
/*
 * SDL audio subsystem (PC speaker tone synthesis).
 */
LIBNEUROAPI int sdl_audio_init(void);
LIBNEUROAPI void sdl_audio_shutdown(void);
LIBNEUROAPI void sdl_audio_play_track(int track_num);
LIBNEUROAPI void sdl_audio_stop(void);
LIBNEUROAPI void sdl_audio_set_volume(float volume);
LIBNEUROAPI void sdl_audio_toggle_mute(void);
LIBNEUROAPI int sdl_audio_is_playing(void);
LIBNEUROAPI int sdl_audio_get_track(void);

/*
 * Background animation.
 */
LIBNEUROAPI int bg_animation_init_tables(bg_animation_control_table_t *tables,
					uint8_t *decompd_anh);
LIBNEUROAPI void bg_animation_update(bg_animation_control_table_t *tables,
					uint16_t animations, uint8_t *working_area, uint8_t *bg_pixels);

#ifdef __cplusplus
}
#endif

/* Non-Windows: LIBNEUROAPI is a no-op */
#ifndef _MSC_VER
#undef LIBNEUROAPI
#define LIBNEUROAPI
#endif

#pragma pack(pop)

/* Endianness helpers for LE binary data from DOS game resources */
#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__ppc__) || defined(__powerpc__) || defined(__PPC__) || defined(__sparc__)
static inline uint16_t le16(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static inline uint32_t le32(uint32_t v) { uint16_t lo = le16((uint16_t)v); uint16_t hi = le16((uint16_t)(v >> 16)); return (uint32_t)(lo | ((uint32_t)hi << 16)); }
static inline void write_le16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)(v); p[1]=(uint8_t)(v>>8); }
static inline void write_le32(uint8_t *p, uint32_t v) { write_le16(p,(uint16_t)v); write_le16(p+2,(uint16_t)(v>>16)); }
#else
#define le16(v)  ((uint16_t)(v))
#define le32(v)  ((uint32_t)(v))
#define write_le16(p,v) do { (p)[0]=(uint8_t)(v); (p)[1]=(uint8_t)((v)>>8); } while(0)
#define write_le32(p,v) do { (p)[0]=(uint8_t)(v); (p)[1]=(uint8_t)((v)>>8); (p)[2]=(uint8_t)((v)>>16); (p)[3]=(uint8_t)((v)>>24); } while(0)
#endif

/* Read LE uint16 from raw byte pointer (works on all platforms) */
static inline uint16_t read_le16_bytes(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }

/* Read imh_hdr_t fields from raw LE byte data (works on all platforms) */
#define imh_dx(p)   (((uint8_t*)(p))[0] | (((uint8_t*)(p))[1] << 8))
#define imh_dy(p)   (((uint8_t*)(p))[2] | (((uint8_t*)(p))[3] << 8))
#define imh_w(p)    (((uint8_t*)(p))[4] | (((uint8_t*)(p))[5] << 8))
#define imh_h(p)    (((uint8_t*)(p))[6] | (((uint8_t*)(p))[7] << 8))
#endif
