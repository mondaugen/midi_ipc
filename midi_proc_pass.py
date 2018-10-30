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
input_fifo = os.open(input_fifo_path,os.O_RDONLY | os.O_NONBLOCK)
print("Opening FIFO %s for output" % (output_fifo_path,))
output_fifo = open(output_fifo_path,'wb',buffering=0)

done=False

def sig_handle(num,frame):
    done = True

signal.signal(signal.SIGINT,sig_handle)

while not done:
    # read in
    msg = os.read(input_fifo,common.midimsg_struct.size)
    print(len(msg))
    # write out
    output_fifo.write(msg)
    time.sleep(1)

print("Closing FIFOs")

input_fifo.close()
output_fifo.close()
