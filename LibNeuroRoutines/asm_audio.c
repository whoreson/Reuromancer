/**
 * Pure C port of asm_audio.asm / asm_seg7.asm
 * PC speaker tone generator for Neuromancer.
 */

#include <stdint.h>
#include <string.h>

extern const unsigned char seg7_data[];
extern const int seg7_data_size;

static unsigned char g_seg[7214];
static int g_init_done = 0;

static void seg7_init(void)
{
    if (!g_init_done) {
        memcpy(g_seg, seg7_data, seg7_data_size);
        g_init_done = 1;
    }
}

#define DB(off)   g_seg[(off)]
#define DW(off)   ((uint16_t)g_seg[(off)] | ((uint16_t)g_seg[(off)+1] << 8))
#define WDW(off,v) do { g_seg[(off)]=(unsigned char)(v); g_seg[(off)+1]=(unsigned char)((v)>>8); } while(0)

/* Data segment labels */
#define byte_26F20  0x00
#define byte_26F21  0x01
#define byte_26F22  0x02
#define mark_0003   0x03
#define track_num   0x04
#define mark_0006   0x06
#define mark_0008   0x08
#define mark_000A   0x0A
#define mark_000C   0x0C

/* Channel state offsets (within 48-byte entry) */
#define CH_COUNT    0x00
#define CH_CMD_OFF  0x02
#define CH_ACC1     0x04
#define CH_STEP1    0x06
#define CH_DIVISOR  0x08
#define CH_ACC2     0x0A
#define CH_STEP2    0x0C
#define CH_CONFIG   0x0E

/* ============================================================ */
/* sub_20513 - Command stream parser                            */
/* ============================================================ */
static void sub_20513(uint16_t rsi)
{
    uint16_t rdi = DW(rsi + CH_CMD_OFF);
    uint8_t bl;
    uint16_t ax;

    if (rdi == 0) return;

    WDW(mark_0006, rsi);

    while (1) {
        bl = DB(rdi++);

        if (bl < 0xFA) {
            /* Note byte */
            uint8_t al = bl;
            uint8_t pitch = al >> 5;
            uint8_t note_idx = bl & 0x1F;
            uint16_t di2;

            di2 = pitch * DB(mark_0003) + DW(mark_0008);
            WDW(rsi + di2, note_idx ? note_idx + 1 : 0);

            {
                uint16_t ebx = bl;
                uint8_t al2 = DB(ebx) & 0x7F;

                if (al2 == 0x7F) {
                    rdi++;
                    uint8_t al3 = DB(rdi++);
                    if (al3 & 0x80) {
                        continue;
                    }
                    /* end */
                    uint16_t si = DW(mark_0006);
                    WDW(si + CH_CMD_OFF, (DW(si) != 0) ? rdi : 0);
                    return;
                }

                {
                    uint16_t cx = DW(rsi);
                    WDW(rsi + di2, cx);
                    WDW(rsi + di2 + 0x14, (uint16_t)((int16_t)(cx - DW(rsi + di2 + 0x10))));
                    WDW(rsi + di2 + 0x18, 0);
                    WDW(rsi + di2 + 0x1A, 1);

                    uint8_t cl2 = 1;
                    uint16_t val = al2 + DW(rsi + di2 + 0x12);
                    while (val >= 0x0C) { val -= 0x0C; cl2++; }
                    val += 0x0C;
                    uint16_t bx2 = val * 2 + DW(mark_000A);
                    ax = DW(rsi + bx2);
                    ax >>= cl2;
                    WDW(rsi + CH_ACC1, ax);
                    WDW(rsi + CH_DIVISOR, ax);
                }
            }

            rdi++;
            ax = DB(rdi++);
            if (ax & 0x80) {
                continue;
            }
            /* end */
            ax = DW(mark_0006);
            WDW(ax + CH_CMD_OFF, (DW(ax) != 0) ? rdi : 0);
            return;
        }

        /* bl >= 0xFA: control command */
        bl -= 0xFA;
        bl *= 2;

        ax = DW(rdi + rsi + 0x21);

        if (ax == 0x1DD) {
            /* silence - zero channel entry */
            uint16_t si = DW(rdi);
            rdi += 2;
            uint16_t target = si + DW(mark_0008);
            int k;
            for (k = 0; k < 0x30; k += 2) {
                WDW(target + k, 0);
            }
            continue;
        }

        /* parameter command */
        bl = DB(rdi++);
        ax = DW(rdi);
        rdi += 2;
        WDW(rsi + bl, ax);

        if (bl == 0) {
            break;
        }
    }

    /* end */
    ax = DW(mark_0006);
    WDW(ax + CH_CMD_OFF, (DW(ax) != 0) ? rdi : 0);
}

/* ============================================================ */
/* sub_20482 - Channel tick processor                           */
/* ============================================================ */
static void sub_20482(uint16_t rsi)
{
    uint16_t ax, sum;

    /* Update acc2 += step2 */
    ax = DW(rsi + CH_STEP2);
    WDW(rsi + CH_ACC2, DW(rsi + CH_ACC2) + ax);

    /* Update acc1 += step1 */
    ax = DW(rsi + CH_STEP1);
    WDW(rsi + CH_ACC1, DW(rsi + CH_ACC1) + ax);

    /* Calculate divisor */
    {
        uint16_t dx = 0;
        ax = DW(rsi + 0x1E);
        sum = ax + DW(rsi + 0x20);

        if (sum != 0) {
            if (sum >= DW(rsi + 0x24)) {
                sum -= DW(rsi + 0x24);
            }
            WDW(rsi + 0x1E, sum);
            sum = (sum >> 4) + DW(rsi + 0x1C);
            ax = DB(sum) * DW(rsi + 0x22);
        }

        dx += DW(rsi + CH_ACC1);
        WDW(rsi + CH_DIVISOR, dx);
    }

    /* Duration counter */
    if (DW(rsi + 0x14) != 0) {
        WDW(rsi + 0x14, DW(rsi + 0x14) - 1);
        if (DW(rsi + 0x14) != 0) return;
        WDW(rsi + 0x18, 0x10);
        WDW(rsi + 0x1A, 1);
    }

    /* Decrement note count */
    WDW(rsi, DW(rsi) - 1);
    if (DW(rsi) != 0) return;

    /* Note ended, parse command stream */
    sub_20513(rsi);
}

/* ============================================================ */
/* asm_get_sample - Main sample generator                       */
/* ============================================================ */
uint16_t asm_get_sample(void)
{
    uint16_t bx, rsi;

    seg7_init();

    if (DB(byte_26F21) != 0) return 0;
    DB(byte_26F21) = 1;

    bx = DW(track_num);
    if (bx == 0) {
        DB(byte_26F21) = 0;
        return 0;
    }

    WDW(mark_0006, 0);
    DB(byte_26F22) = 4;
    rsi = 0x1AD;
    WDW(mark_000A, rsi);
    WDW(mark_0008, 0x2D);

    while (DB(byte_26F22) != 0) {
        if (DW(rsi) != 0) {
            sub_20482(rsi);
            if (DW(mark_0006) != 0) {
                rsi += 0x30;
                DB(byte_26F22)--;
                continue;
            }
            if (DW(rsi + CH_ACC2) == 0 || DW(rsi) == 0) {
                rsi += 0x30;
                DB(byte_26F22)--;
                continue;
            }
            WDW(mark_0006, rsi);
        }

        rsi += 0x30;
        DB(byte_26F22)--;
    }

    bx = DW(mark_0006);
    if (bx == 0) {
        DB(byte_26F21) = 0;
        return 0;
    }

    DB(byte_26F21) = 0;
    return DW(bx + CH_DIVISOR);
}

/* ============================================================ */
/* asm_set_track_on_playback - Initialize channels for a track  */
/* ============================================================ */
void asm_set_track_on_playback(int track_num_arg)
{
    uint16_t rsi, rdi;

    seg7_init();

    DB(byte_26F21) = 1;
    DB(byte_26F22) = 4;
    WDW(track_num, (uint16_t)track_num_arg);

    rsi = (uint16_t)(track_num_arg * 2);
    rdi = 0x2D;

    while (DB(byte_26F22) != 0) {
        uint16_t ax = DW(0x1D0 + rsi);

        if (ax != 0) {
            uint16_t bx = 0x2E;
            do {
                WDW(rdi + bx, 0);
                if (bx == 0) break;
                bx -= 2;
            } while (1);
            WDW(rdi + CH_CMD_OFF, ax);
            WDW(rdi + CH_COUNT, 1);
        }

        rdi += 0x30;
        rsi += 2;
        DB(byte_26F22)--;
    }

    DB(byte_26F21) = 0;
}

