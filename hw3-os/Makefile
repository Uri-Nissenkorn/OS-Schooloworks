# obj-m := message_slot.o
# KDIR := /lib/modules/$(shell uname -r)/build
# PWD := $(shell pwd)

# all:
# 	$(MAKE) -C $(KDIR) M=$(PWD) modules
 
# clean:
# 	$(MAKE) -C $(KDIR) M=$(PWD) clean

obj-m := message_slot.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean