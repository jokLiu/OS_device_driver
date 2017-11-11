#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char **argv) {

    char *filename; /* the name of the device */
    int fd; /* device file descriptor */
    int result;

    if (argc != 3) {
        fprintf (stderr, "Exactly two arguments required <device> and <size> , exiting!\n");
        exit (1);
    }

    /* ioctl  can be performed only on opened device */
    filename = argv[1];
    fd = open (filename, O_RDONLY);
    if (fd == -1) {
        fprintf (stderr, "Could not open file %s, exiting!\n", filename);
        exit (1);
    }

    result = ioctl (fd, 0, atoi(argv[2]));
    printf ("The result of the ioctl is %d\n", result);

    close (fd);
    return 0;
}



//tests:
//1. when message is not available, then -EAGAIN should be returned;
//2. during the write when the message is too big, -EINVAL is returned
//3. during the write if the limit of the size of all messages was surpassed, -EAGAIN is returned
//4. both cases of ioctl
//5. test if the current size is handled correctly
//6. blocking reads and writes
