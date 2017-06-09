obj-m = rtap.o
rtap-objs := ksocket.o device.o listener.o rule.o filter.o proc.o rtap-ko.o stats.o

SRC := $(shell pwd)
KVERSION := $(shell uname -r)
KERNEL_SRC ?= /lib/modules/$(KVERSION)/build


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean


