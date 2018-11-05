#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
open_output_fifo (const char *path)
{
    char output_path[strlen(path)+strlen("_output")+1];
    strcat(strcpy(output_path,path),"_output");
    printf("Opening output FIFO at path %s\n",output_path);
    int ret = open(output_path, O_WRONLY | O_SYNC);
    if (ret < 0) {
        char buf[strlen("Opening output FIFO at path ")+strlen(output_path)+1];
        sprintf(buf,"Opening output FIFO at path %s", output_path);
        perror(buf);
        return -1;
    }
    return ret;
}

int
open_input_fifo (const char *path)
{
    char input_path[strlen(path)+strlen("_input")+1];
    strcat(strcpy(input_path,path),"_input");
    printf("Opening input FIFO at path %s\n",input_path);
    int ret = open(input_path, O_RDONLY);
    if (ret < 0) {
        char buf[strlen("Opening input FIFO at path ")+strlen(input_path)+1];
        sprintf(buf,"Opening input FIFO at path %s", input_path);
        perror(buf);
        return -1;
    }
    return ret;
} 



