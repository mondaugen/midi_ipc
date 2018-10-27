#include <stdio.h>
#include "midi_ev_filter.h"

int debug = 0;

void
midi_ev_filter_init(midi_ev_filter_t *ef, midi_filter_flag_t flags)
{
    memset(ef, 0, sizeof(midi_ev_filter_t));
    ef->flags |= flags;
}

/* Event type */
typedef enum {
    midi_ev_type_NOTEON = 0x90,
    midi_ev_type_NOTEOFF = 0x80,
} midi_ev_type_t;

/* Bytes must at least be of length 3 */
int
midi_ev_filter_should_play(midi_ev_filter_t *ef,
                                 char *bytes)
{
    int chan = bytes[0]&0x0f, pitch = bytes[1], vel = bytes[2];
    switch (bytes[0]&0xf0) {
        case midi_ev_type_NOTEON:
            if (vel > 0) {
                if ((chan >= MIDI_HW_IF_CHAN_MAX) ||
                    (pitch >= MIDI_HW_IF_PITCH_MAX) ||
                    ef->counts[chan][pitch] == UINT32_MAX) {
                    return 0;
                }
                int ret = 0;
                ef->counts[chan][pitch] += 1;
                if (debug) { fprintf(stderr,"noteon, counts: %u\n",ef->counts[chan][pitch]); }
                if (ef->flags & midi_filter_flag_NOTEONS) {
                    if (debug) { fprintf(stderr,"checking noteons\n"); }
                    ret = ef->counts[chan][pitch] == 1 ? 1
                        : 0;
                } else {
                    ret = 1;
                }
                if (ret) { 
                    if (debug) { fprintf(stderr,"sending noteon\n"); }
                }
                return ret;
            }
            /* Otherwise interpreted as note off */
        case midi_ev_type_NOTEOFF:
            if ((chan >= MIDI_HW_IF_CHAN_MAX) ||
                (pitch >= MIDI_HW_IF_PITCH_MAX) ||
                ef->counts[chan][pitch] == 0) {
                return 0;
            }
            int ret = 0;
            ef->counts[chan][pitch] -= 1;
            if (debug) { fprintf(stderr,"noteoff, counts: %u\n",ef->counts[chan][pitch]); }
            if (ef->flags & midi_filter_flag_NOTEOFFS) {
                if (debug) { fprintf(stderr,"checking noteoff\n"); }
                ret = ef->counts[chan][pitch] == 0 ? 1
                    : 0;
            } else {
                ret = 1;
            }
            if (ret) { 
                if (debug) { fprintf(stderr,"sending noteoff\n"); }
            }
            return ret;
        default: return 1;
    }
}

