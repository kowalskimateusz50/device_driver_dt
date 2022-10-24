obj-m := platform_device_driver_dt.o

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR=/lib/modules/5.4.106+/build/
HOST_KERN_DIR=/lib/modules/$(shell uname -r)/build/

all:
		make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules
clean:
		make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
help:
		make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help
host:
		make -C $(HOST_KERN_DIR) M=$(PWD) modules
copy_dtb:
	  scp /home/mati/Desktop/LDDL/source/linux_bbb_5.10/arch/arm/boot/dts/am335x-boneblack.dtb debian@192.168.7.2:/home/debian/drivers
copy_drv:
		scp *.ko debian@192.168.7.2:/home/debian/drivers
