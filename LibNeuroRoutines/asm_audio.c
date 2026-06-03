/**
 * Neuromancer PC Speaker Audio Engine
 * Clean C port of the original DOS ASM audio engine (asm_audio.asm / asm_seg7.asm)
 *
 * 4-channel sequencer/synthesizer using a frequency table and 8-bit wrapping arithmetic.
 * Returns PIT divisor values; 0 = silence. Frequency = 1193180 / divisor Hz.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern const unsigned char seg7_data[];
extern const int seg7_data_size;

#define NUM_CHANNELS 4
#define FREQ_TABLE_BASE 0x1AD
#define TRACK_TABLE_BASE 0x1D0

typedef struct {
    uint16_t duration;      /* 0x00: ticks remaining */
    uint16_t script_ptr;    /* 0x02: command stream pointer */
    uint16_t freq_current;  /* 0x04: base frequency accumulator */
    uint16_t freq_base;     /* 0x06: vibrato/modulation param from chain */
    uint16_t divisor;       /* 0x08: PIT divisor (OUTPUT) */
    uint16_t freq_step;     /* 0x0A: vibrato/slide accumulator */
    uint16_t freq_target;   /* 0x0C: vibrato/slide target */
    uint16_t vibrato_spd;   /* 0x12: modulation parameter */
    uint16_t vibrato_stg;   /* 0x1A: vibrato stage */
} Voice;

static unsigned char g_seg[7214];
static Voice voices[NUM_CHANNELS];
static uint16_t g_track_num = 0;
static int g_init = 0;

#define DW(off) ((uint16_t)g_seg[(off)] | ((uint16_t)g_seg[(off)+1] << 8))

/* Exact simulation of the 8086 frequency calculation loop */
static uint16_t compute_frequency(uint8_t dur_entry)
{
    uint8_t ax = dur_entry;
    uint8_t cl = 0xFF;
    
    while (1) {
        cl++;
        ax -= 0x0C;
        if (ax & 0x80) break;  /* Loop until CF=1 (8-bit borrow) */
    }
    ax += 0x0C;  /* Restore remainder with 8-bit wrap */
    
    uint16_t table_off = FREQ_TABLE_BASE + (uint16_t)ax * 2;
    if (table_off + 1 >= 7214) return 0;
    
    uint16_t raw_freq = DW(table_off);
    return raw_freq >> (cl > 15 ? 15 : cl);
}

static void process_script(int ch)
{
    Voice *v = &voices[ch];
    if (v->script_ptr == 0) return;

    while (1) {
        if (v->script_ptr >= 7214) { v->script_ptr = 0; return; }
        
        uint8_t cmd = g_seg[v->script_ptr++];
        
        if (cmd < 0xFA) {
            /* Note Event */
            uint8_t note_idx = cmd & 0x1F;
            uint8_t dur_entry = g_seg[cmd] & 0x7F;
            
            /* Duration: (note_idx or 1) * table[0x0E + note_idx] */
            uint8_t ni = (note_idx == 0) ? 1 : note_idx;
            v->duration = (uint16_t)ni * g_seg[0x0E + note_idx];
            
            /* Frequency */
            uint16_t freq = compute_frequency(dur_entry);
            v->freq_current = freq;
            v->divisor = freq;
            v->freq_step = (dur_entry == 0) ? 0 : 1;  /* Active if not rest */
            
            /* Read param byte */
            if (v->script_ptr >= 7214) return;
            uint8_t param = g_seg[v->script_ptr++];
            
            /* Store param in freq_base for vibrato/modulation */
            v->freq_base = param;
            
            /* If param bit 7 is clear, end of event chain - stop */
            if (!(param & 0x80)) return;
            
            /* If bit 7 is set, the next byte is another note in the chain.
             * In the original MASM, this next note is loaded on the NEXT
             * call to process_script (when current duration reaches 0).
             * The script_ptr already points to the next note byte.
             * So we return here and let the caller handle it. */
            return;
            
        } else if (cmd == 0xFD) {
            v->script_ptr = 0;
            return;
        } else if (cmd == 0xFF) {
            continue;  /* Separator - skip and read next */
        } else if (cmd == 0xFA) {
            /* Jump */
            v->script_ptr = DW(v->script_ptr);
            return;
        }
    }
}

void set_audio_track(int track_num)
{
    if (!g_init) {
        memcpy(g_seg, seg7_data, 7214);
        g_init = 1;
    }
    g_track_num = (uint16_t)track_num;
    memset(voices, 0, sizeof(voices));
    if (track_num == 0) return;

    for (int i = 0; i < NUM_CHANNELS; i++) {
        uint16_t off = DW(TRACK_TABLE_BASE + (track_num * 8) + (i * 2));
        if (off) {
            voices[i].script_ptr = off;
            process_script(i);
        }
    }
}

uint16_t get_audio_sample(void)
{
    if (g_track_num == 0) return 0;

    int active_ch = -1;
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (voices[i].duration > 0) {
            voices[i].duration--;
            if (voices[i].duration == 0) process_script(i);
        } else if (voices[i].script_ptr != 0) {
            /* Duration is 0 but script has more data - load next note */
            process_script(i);
        }
        
        /* Prioritize melody channels: prefer higher channel index with freq_step!=0 */
        /* Fallback: any channel with duration > 0 and divisor > 0 */
        if (voices[i].duration > 0 && voices[i].divisor != 0) {
            if (voices[i].freq_step != 0) {
                active_ch = i;  /* Override: prefer channels with active modulation */
            } else if (active_ch == -1) {
                active_ch = i;  /* Fallback: use this channel if no active one found */
            }
        }
    }

    return (active_ch != -1) ? voices[active_ch].divisor : 0;
}

void audio_debug(void)
{
    printf("track_num: %d\n", g_track_num);
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        printf("ch%d: dur=%d script=0x%04X div=0x%04X freq_step=%d freq_base=0x%04X\n",
               ch, voices[ch].duration, voices[ch].script_ptr,
               voices[ch].divisor, voices[ch].freq_step, voices[ch].freq_base);
    }
}
