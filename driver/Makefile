obj-m += sysmon_driver.o

# Detect kernel build directory or allow override
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod sysmon_driver.ko
	sudo chmod 666 /dev/sysmon_driver
	dmesg | tail -n 10

unload:
	sudo rmmod sysmon_driver
	dmesg | tail -n 10
