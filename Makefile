obj-m = rtap.o
rtap-objs := rtap-ko.o filter.o rule.o ksocket.o device.o listener.o proc.o

SRC := $(shell pwd)
KVERSION := $(shell uname -r)
KERNEL_SRC ?= /lib/modules/$(KVERSION)/build


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean


