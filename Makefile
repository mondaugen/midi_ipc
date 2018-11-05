CFLAGS=-Wall -g -I.
LDLIBS=-ljack -pthread

midi_proc_jack : heap.o midi_proc_jack.o fastcache.o midi_ev_filter.o io_fifo.o

c_to_py_msg : c_to_py_msg.o

test_fastcache : test_fastcache.o fastcache.o

test/midimsg_to_py : test/midimsg_to_py.o io_fifo.o

test/midimsg_size : test/midimsg_size.o
