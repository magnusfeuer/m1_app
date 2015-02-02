#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
main(int argc, char *argv[])
{
    int des;
    struct input_event ev;
    if (argc != 2) {
	printf("Usage: %s event_device (such as /dev/input/event0)\n", argv[0]);
	exit(255);
    }

    if ((des = open(argv[1], O_RDONLY)) == -1) {
	perror(argv[1]);
	exit(255);
    }

    while(1) {
	read(des, (void *) &ev, sizeof(ev));
	printf("Type[%d/%X] Code[%d/%X], Val[%d/%X]\n", 
	       ev.type,
	       ev.type,
	       ev.code,
	       ev.code,
	       ev.value,
	       ev.value);
    }
}
