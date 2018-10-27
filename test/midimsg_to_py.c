#include "io_fifo.h"
#include "midimsg.h"
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv)
{
    if (argc < 2) { exit(EXIT_FAILURE); }
    int output_fifo_fd = open_output_fifo(argv[1]);
    if (output_fifo_fd < 0) {
        exit(EXIT_FAILURE);
    }
    midimsg mm = {
        {1,2,3,4,5},
        5,
        5678
    };
    write(output_fifo_fd,&mm,sizeof(mm));
    close(output_fifo_fd);
    exit(0);
}
