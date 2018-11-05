#ifndef MIDIMSG_H
#define MIDIMSG_H 

#include <stdint.h>

#define MIDI_HW_IF_CHAN_MAX 16
#define MIDI_HW_IF_PITCH_MAX 128
#define MIDIMSGBUFSIZE 16

typedef struct {
    /* MIDI message data */
	char  buffer[MIDIMSGBUFSIZE];
    /* number of data in message */
	uint32_t size;
    /* time since application started, also used by scheduler to determine when
    events should be output i.e., the heap sorts by this value so that the event
    that should happen soonest is always at the top. */
	uint64_t tme_mon;
} midimsg;

#endif /* MIDIMSG_H */
