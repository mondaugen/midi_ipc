import common
import sys

if len(sys.argv) < 2:
    raise Exception('This never happens to me :O 8===D')

input_fifo = open(sys.argv[1]+"_output",'rb')

msg = input_fifo.read(common.midimsg_struct.size)

mess,size,time = common.midimsg_struct.unpack(msg)

print(mess)
print(size)
print(time)

input_fifo.close()
