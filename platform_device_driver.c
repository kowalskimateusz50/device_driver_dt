#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h>
#include<platform.h>

#define RDONLY 0x1
#define WRONLY 0x10
#define RDWR   0x11

#define MAX_DEVICES 4

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/* Device private data structure */

struct pcdev_private_data
{
	/* Platform device data */
	struct pcdev_platform_data pdata;
	/* Device data buffer */
	char *buffer;
	/* Device number variable */
	dev_t dev_num;
	/* Cdev Variable */
	struct cdev cdev;
};

	int i;

/* Driver private data structure */

struct pcdrv_private_data
{
	int total_devices;

	/* This hold the device number */
	dev_t device_number_base;

	/* Declaration class structure pointer */
	struct class *pcd_class;

	/* Declaration of device structure pointer */
	struct device *pcd_device;
};

struct pcdrv_private_data pcdrv_data;

int check_permission(int device_permission, int access_permission)
{
	if(device_permission == RDWR)
	{
		return 0;
	}
	/* Ensures read-only access */
	if((device_permission == RDONLY) && (access_permission & FMODE_READ) && !(access_permission & FMODE_WRITE))
	{
		return 0;
	}
	/* Ensures write-only access */
	if((device_permission == WRONLY) && (access_permission & FMODE_WRITE) && !(access_permission & FMODE_READ))
	{
		return 0;
	}

	return -EPERM;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    return 0;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
  return 0;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count,  loff_t *f_pos)
{
  return -ENOMEM;
}

int pcd_open(struct inode *inode, struct file *flip)
{
	pr_info("Open was successful\n");
	return 0;
}

int pcd_release(struct inode *inode, struct file *flip)
{
	pr_info("Close was successful\n");
	return 0;
}

struct file_operations pcd_fops =
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

/* Function gets called when matched platform device was found */
int pcd_probe(struct platform_device *dev)
{
	/* 1. Get the device platform data */
	int ret;

	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data *pdata;

	pdata = (pcdev_platform_data*)dev_get_platdata(&dev->dev)

	/* 1.e Error handling */
	if(!pdata)
	{
		pr_info("No platform data available\n")
		return -EINVAL;
	}
	/* 2. Dynamically allocate memory for the device private data */

	dev_data = kzalloc(sizeof(pdev*), GFP_KERNEL);

	/* 2.e Erorr hangling for allocation */

	if(!dev_data)
	{
		pr_info("Cannot allocate memory\n");
		return -ENOMEM;
	}

	/* 2f. Copy platform data to device data */
	dev_data -> pdata.size = pdata -> size;
	dev_data -> pdata.perm = pdata -> permission;
	dev_data -> pdata.serial_number = pdata -> serial_number;
	/* Print before copied data */

	pr_info("Device size is: %d\n", dev_data -> pdata.size);
	pr_info("Device permission is: %d\n", dev_data -> pdata.permission);
	pr_info("Device serial number is: %s\n", dev_data -> pdata.serial_number);

	/* 3. Dynamically allocate device data buffer */
	dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL);

	/* 3.e Erorr hangling for allocation */

	if(!dev_data->buffer)
	{
		pr_info("Cannot allocate memory\n");
		kfree(pdata);
		return -ENOMEM;
	}
	/* 4. Get device number */
	dev_data -> pcdrv_data.device_number_base + dev->id;

	/* 5. Cdev init and Cdev add */

	/* 6. Create device file for the detected platform device */

	/* 7. Error handling */

  pr_info("Device is detected\n");

  return 0;
}

/* Function gets called when platform device was removed from system */
int pcd_remove(struct platform_device *dev)
{
  pr_info("Device is removed\n");
  return 0;
}

/* Instance of paltform driver representing structure */

struct platform_driver pcd_platform_driver = {
  .probe = pcd_probe,
  .remove = pcd_remove,
  .driver = {.name = "pchar-platform-device"}
};

/* Module initialization fucntion */

static int __init pcd_init(void)
{
		int ret;

	/* 1.	Dynamically allocate a device number for max devices*/
	ret = alloc_chrdev_region(&pcdrv_data.device_number_base,0,MAX_DEVICES,"plat-dev");
	/* 1.e Error handling */
	if(ret < 0)
	{
		pr_err("Chrdev allocation failed\n");
		return ret;
	}

	/* 2. Create device class under /sys/class/ */
	pcdrv_data.pcd_class = class_create(THIS_MODULE, "plat_class");
	/* 2.e Error handling of this section */
	if(IS_ERR(pcdrv_data.pcd_class))
	{
		/* Print some error info */
		pr_err("Class creation failed\n");
		/*Convert pointer to error code int */
		ret = PTR_ERR(pcdrv_data.pcd_class);
		unregister_chrdev_region(pcdrv_data.device_number_base,MAX_DEVICES);
		return ret;
	}

	/* 3. Register a platform Driver in sysfs*/
	platform_driver_register(&pcd_platform_driver);

	/* Confirmation of successfully ended initialization */
	pr_info("Module  was loaded\n");

	return 0;
}
static void __exit pcd_cleanup(void)
{
	/* 1. UNregister platform driver in sysfs */
	platform_driver_unregister(&pcd_platform_driver);

	/* 2. Class destroying */
	class_destroy(pcdrv_data.pcd_class);

	/* 3. Unregister dynamically allocated device number */
  unregister_chrdev_region(pcdrv_data.device_number_base,MAX_DEVICES);

  /* Some clean-up message */
  pr_info("Module was succesfully unloaded\n");
}

/* Adding function to init call */
module_init(pcd_init);

/* Adding function to exit call */
module_exit(pcd_cleanup);

/* Module description */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MKI");
MODULE_DESCRIPTION("A pseudo platform character device driver");
