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

struct pcdev_platform_data pcdev_pdata[2] = {
	[0] = {.size = 1024, .permission = RDWR, .serial_number = "platform_device_driver_1"},
	[1] = {.size = 1024, .permission = RDWR, .serial_number = "platform_device_driver_2"}
};

struct platform_device platform_pcdev_1 = {
	.name = "pchar-platform-device",
	.id = 0,
	.dev = {
		.platform_data = &pcdev_pdata[0],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_2 = {
	.name = "pchar-platform-device",
	.id = 1,
	.dev = {
		.platform_data = &pcdev_pdata[1],
		.release = pcdev_release
	}
};

/* 2. Initialize platform data of devices */

static int __init platform_device_init(void){
  /* 1. Register a platform devices */
  platform_device_register(&platform_pcdev_1);
  platform_device_register(&platform_pcdev_2);

	pr_info("Device setup module inserted\n");


  return 0;
}

static void __exit platform_device_cleanup(void){

  /* 1. Unregister platform devices */

  platform_device_unregister(&platform_pcdev_1);
  platform_device_unregister(&platform_pcdev_2);

	pr_info("Device setup module removed\n");

}

module_init(platform_device_init);
module_exit(platform_device_cleanup);


/* Module description */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MKI");
MODULE_DESCRIPTION("Module which registers platform devices");

/* Hello github */
