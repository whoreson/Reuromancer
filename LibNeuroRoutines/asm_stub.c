/**
 * Stub implementation of asm_audio.asm / asm_seg7.asm functions.
 * Replaces MASM audio playback with pure C stubs.
 * Audio is not critical - just produces silence.
 */

/* Stub: always returns 0 (silence) */
int asm_set_track_on_playback(int track_num)
{
    (void)track_num;
    return 0;
}

int asm_get_sample(void)
{
    /* Return silence (no frequency) */
    return 0;
}
