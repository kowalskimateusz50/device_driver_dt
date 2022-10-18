#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

/* Release function definition */

void pcdev_release (struct device *dev)
{
	pr_info("Device released\n");
}

/* 1. Create platform device's structures */

struct pcdev_platform_data pcdev_pdata[4] = {
	[0] = {.size = 1024, .permission = RDWR, .serial_number = "platform_device_driver_1"},
	[1] = {.size = 1024, .permission = RDWR, .serial_number = "platform_device_driver_2"},
	[2] = {.size = 1024, .permission = RDWR, .serial_number = "platform_device_driver_3"},
	[3] = {.size = 1024, .permission = RDWR, .serial_number = "platform_device_driver_4"}
};

struct platform_device platform_pcdev_1 = {
	.name = "pcdev-A1x",
	.id = 0,
	.dev = {
		.platform_data = &pcdev_pdata[0],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_2 = {
	.name = "pcdev-B1x",
	.id = 1,
	.dev = {
		.platform_data = &pcdev_pdata[1],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_3 = {
	.name = "pcdev-C1x",
	.id = 2,
	.dev = {
		.platform_data = &pcdev_pdata[2],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_4 = {
	.name = "pcdev-D1x",
	.id = 3,
	.dev = {
		.platform_data = &pcdev_pdata[3],
		.release = pcdev_release
	}
};

struct platform_device *platform_pdevs[]=
{
	&platform_pcdev_1,
	&platform_pcdev_2,
	&platform_pcdev_3,
	&platform_pcdev_4
};
/* 2. Initialize platform data of devices */

static int __init platform_device_init(void){
  /* 1. Register a platform devices */
	platform_add_devices(platform_pdevs, ARRAY_SIZE(platform_pdevs));

	pr_info("Device setup module inserted\n");


  return 0;
}

static void __exit platform_device_cleanup(void){

  /* 1. Unregister platform devices */

  platform_device_unregister(&platform_pcdev_1);
  platform_device_unregister(&platform_pcdev_2);
	platform_device_unregister(&platform_pcdev_3);
	platform_device_unregister(&platform_pcdev_4);

	pr_info("Device setup module removed\n");

}

module_init(platform_device_init);
module_exit(platform_device_cleanup);


/* Module description */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MKI");
MODULE_DESCRIPTION("Module which registers platform devices");

/* Hello github */
