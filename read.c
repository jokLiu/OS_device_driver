#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char **argv) {

    char *filename, *input; /* the name of the device */
    int fd; /* device file descriptor */
    int result;
    char hello[6]; 
    if (argc != 3) {
        fprintf (stderr, "Exactly two arguments required <device> and <size> , exiting!\n");
        exit (1);
    }

    /* ioctl  can be performed only on opened device */
    filename = argv[1];
    input = argv[2];
    fd = open (filename, O_RDWR);
    if (fd == -1) {
        fprintf (stderr, "Could not open file %s, exiting!\n", filename);
        exit (1);
    }

    for(int i=0; i<10; i++){
        result = write(fd, input, 6);
        if(result <= 0) printf("write failed");
    }

    for(int i=0; i<10; i++){
        result = read(fd, &hello, 6);
        if(result <= 0) {
            printf("write failed");
        }
        if(result==6)printf("%s\n", hello);
    }


   // printf ("The result of the ioctl is %d\n", result);

    close (fd);
    return 0;
}


