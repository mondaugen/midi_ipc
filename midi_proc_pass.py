import common
import time
import sys
import signal
import os

if len(sys.argv) < 2:
    raise Exception("Specify fifo path stub")

fifo_path_stub = sys.argv[1]

# output / input reversed for slave
input_fifo_path = fifo_path_stub + '_output'
output_fifo_path = fifo_path_stub + '_input'

print("Opening FIFO %s for input" % (input_fifo_path,))
#input_fifo = os.fdopen(os.open(input_fifo_path,os.O_RDONLY | os.O_NONBLOCK),mode='rb')
input_fifo = os.open(input_fifo_path,os.O_RDONLY | os.O_NONBLOCK)
print("Opening FIFO %s for output" % (output_fifo_path,))
output_fifo = open(output_fifo_path,'wb',buffering=0)

done=False

def sig_handle(num,frame):
    done = True

signal.signal(signal.SIGINT,sig_handle)

msg = b''
while not done:
    # read in
    try:
        msg += os.read(input_fifo,common.midimsg_struct.size)
        #msg = input_fifo.read(common.midimsg_struct.size)
    except BlockingIOError:
        continue # Don't send message, we don't have one to send
    while len(msg) > common.midimsg_struct.size:
        print("midi_proc_pass.py: message length: %d" % (len(msg),))
        parsed_msg = common.midimsg_struct.unpack(msg[:common.midimsg_struct.size])
        print(parsed_msg)
        #print("midi_proc_pass.py: message")
        #print(parsed_msg[:common.midimsg_buffer_size])
        #print("size")
        #print(parsed_msg[common.midimsg_buffer_size])
        #print("time")
        #print(parsed_msg[common.midimsg_buffer_size+1])
        # write out
        output_fifo.write(msg)
        msg = msg[common.midimsg_struct.size:]
    time.sleep(0.001)

print("Closing FIFOs")

input_fifo.close()
output_fifo.close()
