import common
import time
import sys

if len(sys.argv) < 2:
    raise Exception("Specify fifo path stub")

fifo_path_stub = sys.argv[1]

input_fifo_path = fifo_path_stub + '_input'
output_fifo_path = fifo_path_stub + '_output'

input_fifo = open(input_fifo_path,'rb',buffering=0)
output_fifo = open(output_fifo_path,'wb',buffering=0)

while not done:
    # read in
    msg = input_fifo.read(common.midimsg_struct.size)
    # write out
    output_fifo.write(msg)
    time.sleep(0.001)

input_fifo.close()
output_fifo.close()
