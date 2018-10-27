import struct

default_fifo_path="/tmp/midi_proc_jack_fifo"

# see midimsg.h for the layout of this struct
midimsg_format_str="16sIQ"
midimsg_struct=struct.Struct(midimsg_format_str)

