#ifndef _MESSAGE_SLOT_H_
#define _MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define DEVICE_NAME "message_slot"
#define MAJOR_NUMBER 235 // for prod
// #define MAJOR_NUMBER 8 // for local dubuging

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned int)
#define BUFFER 128
#define SUCCSESS 0
#define FALIURE -1


typedef struct channel {
	long channel_id;
	int message_size;
	char message[BUFFER];
	struct channel *next;
} channel;


typedef struct slot {
	struct channel *head;
} slot;

typedef struct device_file {
	int minor_number;
	struct channel *activeChannel;
} device_file;


#endif