import struct

default_fifo_path="/tmp/midi_proc_jack_fifo"

# see midimsg.h for the layout of this struct
midimsg_buffer_size=16
midimsg_format_str=("%dsIQ" % (midimsg_buffer_size,))
midimsg_struct=struct.Struct(midimsg_format_str)
