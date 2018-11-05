/*
midi_proc_jack.c

Receive midi events using JACK library.
Send midi events to a different process via message queue.
Receive events from a different process via a message queue.
Send these events using JACK library.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "heap.h"
#include "fastcache.h"
#include "midi_ev_filter.h"
#include "midimsg.h"
#include "io_fifo.h"

#ifdef __MINGW32__
#include <pthread.h>
#endif

#ifndef WIN32
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#endif

#ifndef MAX
#define MAX(a,b) ( (a) < (b) ? (b) : (a) )
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif 

#define CACHE_NOBJS 1024

static int debug;

static jack_port_t* port;
static jack_port_t* output_port;
static jack_ringbuffer_t *rb = NULL;

/* mutex for incoming midi events / outgoing messages */
static pthread_mutex_t msg_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_ready = PTHREAD_COND_INITIALIZER;

/* mutex for outgoing midi events / incoming messages */
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;

static int keeprunning = 1;
static uint64_t monotonic_cnt = 0;
Heap *outevheap;
static int passthrough = 0;

static int output_fifo_fd = 0;
static int input_fifo_fd = 0;

#define N_ARGS 3

/* Number of MIDI messages in the ring buffer */
#define RBSIZE 512

#define ERROR(s) fprintf(stderr,s)

static int
push_to_output_fifo (const midimsg* event)
{
    if (!output_fifo_fd) {
        ERROR("Output FIFO not open");
        return -1;
    }
    if (write(output_fifo_fd,event,sizeof(midimsg)) < 0) {
        perror("Sending MIDI message to FIFO");
        return -2;
    }
    return 0;
}

static int
read_from_input_fifo (midimsg *event)
{
    if (!input_fifo_fd) {
        ERROR("Input FIFO not open");
        return -1;
    }
    if (read(input_fifo_fd,event,sizeof(midimsg)) < 0) {
        perror("Receiving MIDI message from FIFO");
        return -2;
    }
    return 0;
}

static fastcache_t *out_midimsg_cache;

midi_ev_filter_t midi_ev_filt;

int
process (jack_nframes_t frames, void* arg)
{
    /* The count at the beginning of the frame, we need this to calculate the
       offsets into the frame of the outgoing MIDI messages. */
    uint64_t monotonic_cnt_beg_frame = monotonic_cnt;
    jack_nframes_t _frames;
	void *buffer, *midioutbuf;
	jack_nframes_t N;
	jack_nframes_t i;

	buffer = jack_port_get_buffer (port, frames);
    midioutbuf = jack_port_get_buffer(output_port, frames);
	jack_midi_clear_buffer(midioutbuf);
	assert (buffer);

    /* We assume events returned sorted by their order in time, which seems
       to be true if you check out the midi_dump.c example. */
	N = jack_midi_get_event_count (buffer);

    if (passthrough) {
        if (debug) { fprintf(stderr,"passing through\n"); }
        for (i = 0; i < N; ++i) {
            jack_midi_event_t event;
            int r;

            r = jack_midi_event_get (&event, buffer, i);

            unsigned char *midimsgbuf = 
                jack_midi_event_reserve(midioutbuf, event.time, 
                        event.size);
            if (r == 0 && midimsgbuf) {
                memcpy(midimsgbuf,event.buffer,event.size);
            } else {
                if (debug) { fprintf(stderr,"midi_proc_jack: MIDI msg dropped\n"); }
            }
        }
        return 0;
    }
    _frames = frames;
    if (N > 0) {
        fprintf(stderr,"received %d events\n",N);
    }
	for (i = 0; i < N; ++i) {
		jack_midi_event_t event;
		int r;

		r = jack_midi_event_get (&event, buffer, i);

		if (r == 0 && jack_ringbuffer_write_space (rb) >= sizeof(midimsg)) {
			midimsg m;
			m.tme_mon = monotonic_cnt;
			m.size    = event.size;
			memcpy (m.buffer, event.buffer, MIN(sizeof(m.buffer), event.size));
			if (jack_ringbuffer_write (rb, (void *) &m, sizeof(midimsg)) < sizeof(midimsg)) {
                fprintf(stderr,"%s: error writing to jack ringbuffer\n", __FILE__);
            }
            monotonic_cnt += event.time;
            _frames = event.time > _frames ? 0 : _frames - event.time;

		}
	}

	monotonic_cnt += _frames;

    /* Indicate to thread processing incoming events that data is there. */
	if (pthread_mutex_trylock (&msg_thread_lock) == 0) {
		pthread_cond_signal (&data_ready);
		pthread_mutex_unlock (&msg_thread_lock);
	}

    /* Get the mutex for the heap */
    if (pthread_mutex_trylock (&heap_lock) == 0) {
        midimsg *soonestmsg = NULL;
        if (debug) { fprintf(stderr,"Heap size: %zu\n",Heap_size(outevheap)); }
        if (debug) {
            fprintf(stderr,
            "current time, frame start: %lu frame end: %lu\n",
            monotonic_cnt_beg_frame,monotonic_cnt); 
        }
        /* While there are late or on-time messages in the heap, check if they
        should be sent, and if so, send them */
        while ((Heap_top(outevheap,(void**)&soonestmsg) == HEAP_ENONE)
                && (soonestmsg != NULL) 
                && (soonestmsg->tme_mon <= monotonic_cnt)) {
            if (debug) { fprintf(stderr,"message address: %p\n",(void*)soonestmsg); }
            unsigned char *midimsgbuf = NULL;
            jack_nframes_t currel;
            /* message can maybe get sent, it is time, first filter repeated note ons and note offs. */
            int shouldplay = 0;
            if ((shouldplay = midi_ev_filter_should_play(&midi_ev_filt,soonestmsg->buffer))) {
                currel = soonestmsg->tme_mon >= monotonic_cnt_beg_frame ?
                    soonestmsg->tme_mon - monotonic_cnt_beg_frame : 0;
                midimsgbuf = 
                    jack_midi_event_reserve(midioutbuf, currel, 
                            soonestmsg->size);
            }
            /* If it should be played, copy message to midimsgbuf */
            if (midimsgbuf) {
                if (1) {
                    fprintf(
                    stderr,"play MIDI msg, size %u time: %lu status: %#x ",
                    soonestmsg->size,
                    soonestmsg->tme_mon,
                    soonestmsg->buffer[0]);
                    int i;
                    for (i = 1; i < soonestmsg->size; i++) { 
                        fprintf(stderr,
                        "%d ",
                        soonestmsg->buffer[i]);
                    }
                    fprintf(stderr,"\n");
                }
                memcpy(midimsgbuf,soonestmsg->buffer,soonestmsg->size);
            } else {
                if (shouldplay) {
                    if (debug) {
                    fprintf(stderr,
                    "Returned NULL when requesting MIDI event of size %u at time %u, "
                    "MIDI msg not sent\n",
                    soonestmsg->size,currel);
                    }
                }
            }
            /* soonestmsg was sent, so remove it from the heap and free the
            space allocated to it. */
            Heap_pop(outevheap,(void**)&soonestmsg);
            fastcache_free(out_midimsg_cache,soonestmsg);
        }
        if (soonestmsg) {
            if (debug) {
                fprintf(stderr,
                "Soonest message at time %lu\n",
                soonestmsg->tme_mon);
            }
        }
        /* Give up the heap lock so the FIFO from
        the external app can fill it.  */
        pthread_mutex_unlock(&heap_lock);
    }

	return 0;
}

/* Thread that manages sending messages via FIFO to the other application */

typedef struct {
    pthread_mutex_t *msg_thread_lock;
    int *keeprunning;
    jack_ringbuffer_t *rb;
    pthread_cond_t *data_ready;
} outthread_data;

static void *
output_thread(void *aux)
{
    outthread_data *thread_data = aux;
    /* Wait for the lock */
    pthread_mutex_lock (thread_data->msg_thread_lock);

    if (debug) {
    fprintf(stderr,
    "output thread running\n"); 
    }
    while (*thread_data->keeprunning) {
        /* Wait for data to be ready in the ring buffer */
        pthread_cond_wait (thread_data->data_ready, thread_data->msg_thread_lock);
        const int mqlen = jack_ringbuffer_read_space (thread_data->rb) / sizeof(midimsg);
        int i;
        for (i=0; i < mqlen; ++i) {
            midimsg m;
            jack_ringbuffer_read(thread_data->rb, (char*) &m, sizeof(midimsg));

            if (push_to_output_fifo(&m)) { continue; /* On error just keep going. */ }
            if (debug) {
                fprintf(stdout,"sent message at time %lu\n", m.tme_mon);
            }
        }
        // fflush (stdout); // ?
        /* TODO Pretty sure this shouldn't go here */
/*         pthread_cond_wait (thread_data->data_ready, thread_data->msg_thread_lock); */
    }
    pthread_mutex_unlock (thread_data->msg_thread_lock);
    if (debug) { fprintf(stderr,"output thread stopping\n"); }
    return thread_data;
}

/* Thread that manages receiving messages from other application */

typedef struct {
    int *keeprunning;
    pthread_mutex_t *heap_lock;
    Heap *outevheap;
} inthread_data;

static void *
input_thread(void *aux)
{
    inthread_data *thread_data = aux;
    midimsg just_recvd;
    if (debug) { fprintf(stderr,"input thread running\n"); }
    while (*thread_data->keeprunning) {
        //sleep(1);
        /* blocks while waiting for messages on FIFO */
        if (read_from_input_fifo(&just_recvd) < 0) {
            fprintf(stderr,"%s: input thread stopping...\n",__FILE__);
            *thread_data->keeprunning = 0;
            break;
        }
        //fprintf(stderr,"message received\n");
        /* once one is obtained, push to heap */
        pthread_mutex_lock(thread_data->heap_lock);
        midimsg *mmsg_topush = fastcache_alloc(out_midimsg_cache);
        if (!mmsg_topush) {
            fprintf(stderr,"cache full, incoming msg dropped\n");
            pthread_mutex_unlock(thread_data->heap_lock);
            continue;
        }
        *mmsg_topush = just_recvd;
        if (Heap_push(thread_data->outevheap,mmsg_topush) 
                != HEAP_ENONE) {
            fprintf(stderr,"heap push failed, incoming msg dropped, heap full?\n");
            fastcache_free(out_midimsg_cache,mmsg_topush);
        }
        if (debug) { fprintf(stderr,"Pushed message to heap.\n"); }
        pthread_mutex_unlock(thread_data->heap_lock);
    }
    if (debug) { fprintf(stderr,"input thread stopping\n"); }
    return thread_data;
}

/* Comparison function for sorting the midi message heap */
static int
mmsg_cmp(void *_a, void *_b)
{
    midimsg *a = _a;
    midimsg *b = _b;
    return a->tme_mon >= b->tme_mon;
}

/* heap doesn't need to know the index */
static void
set_idx_ignore(void *a, size_t idx)
{
    return;
}

static void
stopsig(int sn)
{
    keeprunning = 0;
}

/* TODO: Make better exit on error that cleans up. */
int
main (int argc, char* argv[])
{
#ifdef DEBUG
    debug = 1;
#else
    debug = 0;
#endif
	jack_client_t* client;
	char * client_name;
    char * fifo_path_stub;
	int r;

    if (argc < N_ARGS) {
        fprintf(stderr,"Arguments are client_name fifo_path_stub [pass_through]");
        exit (EXIT_FAILURE);
    }
    client_name = argv[1];
    fifo_path_stub = argv[2];

	if (argc >= 4) {
		if (!strcmp (argv[3], "-p")) {
            passthrough = 1;
        }
    }

	client = jack_client_open (client_name, JackNullOption, NULL);
	if (client == NULL) {
		fprintf (stderr, "Could not create JACK client.\n");
		exit (EXIT_FAILURE);
	}

	rb = jack_ringbuffer_create (RBSIZE * sizeof(midimsg));

	jack_set_process_callback (client, process, 0);

	port = jack_port_register (client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	output_port = jack_port_register (client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

	if (port == NULL) {
		fprintf (stderr, "Could not register port.\n");
		exit (EXIT_FAILURE);
	}

    if ((input_fifo_fd = open_input_fifo(fifo_path_stub)) < 0) {
        exit (EXIT_FAILURE);
    }

    if ((output_fifo_fd = open_output_fifo(fifo_path_stub)) < 0) {
        exit (EXIT_FAILURE);
    }

    outevheap = Heap_new(CACHE_NOBJS,
            mmsg_cmp,
            set_idx_ignore);

    if (!outevheap) {
        fprintf(stderr, "Could not allocate heap\n");
        exit (EXIT_FAILURE);
    }

    out_midimsg_cache = fastcache_new(sizeof(midimsg),CACHE_NOBJS);

    if (!out_midimsg_cache) {
        fprintf(stderr, "Could not allocate midimsg cache\n");
        exit(EXIT_FAILURE);
    }

    outthread_data ot_data = {
        .msg_thread_lock = &msg_thread_lock,
        .keeprunning = &keeprunning,
        .rb = rb,
        .data_ready = &data_ready,
    };

    inthread_data it_data = {
        .keeprunning = &keeprunning,
        .heap_lock = &heap_lock,
        .outevheap = outevheap,
    };

    /* I would reckon only filtering note offs is the most common configuration */
    midi_ev_filter_init(&midi_ev_filt,midi_filter_flag_NOTEOFFS);

	r = jack_activate (client);
	if (r != 0) {
		fprintf (stderr, "Could not activate client.\n");
		exit (EXIT_FAILURE);
	} else {
        printf("Client %s now active", client_name);
    }

    pthread_t midi_to_msgq;
    pthread_t msgq_to_midi;
    
    signal(SIGINT,stopsig);

    if (pthread_create(&midi_to_msgq,NULL,output_thread,&ot_data)) {
        perror("pthread create midi_to_msgq");
        exit (EXIT_FAILURE);
    }

    if (pthread_create(&msgq_to_midi,NULL,input_thread,&it_data)) {
        perror("pthread create msgq_to_midi");
        exit (EXIT_FAILURE);
    }
    
    pthread_join(midi_to_msgq,NULL);
    pthread_join(msgq_to_midi,NULL);

	jack_deactivate (client);
	jack_client_close (client);
	jack_ringbuffer_free (rb);

	return 0;
}
