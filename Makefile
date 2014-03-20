obj-m = rtap.o
rtap-objs := rtap-ko.o filter.o rule.o ksocket.o

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean


