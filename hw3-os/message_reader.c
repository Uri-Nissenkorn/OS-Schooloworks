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
 	char  buffer[BUFFER];
	int file;
	int val;

	if (argc != 3) {
		perror("Error: ");
		exit(1);
	}

	channel_id = atoi(argv[2]);
	
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

	val = read(file, buffer, BUFFER);
	if (val <= 0) {
		perror("Error: ");
		exit(1);
	}

    if (write(1, buffer, val) != val) {
		perror("Error: ");
		exit(1);
	}

	close(file);

	exit(0);
}