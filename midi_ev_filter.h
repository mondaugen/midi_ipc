#ifndef MIDI_EV_FILTER_H
#define MIDI_EV_FILTER_H 

#include <stdint.h>
#include <string.h>

#define MIDI_HW_IF_CHAN_MAX 16
#define MIDI_HW_IF_PITCH_MAX 128

typedef enum {
    /* Filter out repeated note ons */
    midi_filter_flag_NOTEONS = (1 << 0),
    /* Filter out repeated note offs */
    midi_filter_flag_NOTEOFFS = (1 << 1),
} midi_filter_flag_t;

typedef struct {
    uint32_t          counts[MIDI_HW_IF_CHAN_MAX][MIDI_HW_IF_PITCH_MAX];
    midi_filter_flag_t flags;
} midi_ev_filter_t;

int
midi_ev_filter_should_play(midi_ev_filter_t *ef,
                           char *bytes);
void
midi_ev_filter_init(midi_ev_filter_t *ef, midi_filter_flag_t flags);

#endif /* MIDI_EV_FILTER_H */
