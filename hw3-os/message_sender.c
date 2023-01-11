#include "message_slot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int main(int argc, char** argv)
{
	unsigned long channel_id;
	char * message;
	int size;
	int file;
	int val;

	if (argc != 4) {
		perror("1Error: ");
		exit(1);
	}

	channel_id = atoi(argv[2]);
    message = argv[3];
	size = strlen(message);

	
    file = open(argv[1], O_RDWR);
    
	if (file < 0) {
		perror("Error: ");
		exit(1);
	}


    val = ioctl(file, MSG_SLOT_CHANNEL, channel_id);
	if (val != 0) {
		perror("Error: ");
		exit(1);
	}
	
	val = write(file, message, size);
	if (val != size) {
		perror("Error: ");
		exit(1);
	}

	close(file);

	exit(0);
}