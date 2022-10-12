#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h>

#define RDONLY 0x1
#define WRONLY 0x10
#define RDWR   0x11

#define MAX_DEVICES 4

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/* Device private data structure */

struct pcdev_private_data
{
	char *buffer;
	int size;
	const char *serial_number;
	/* Device permission */
	int perm;
	/* Cdev Variable */
	struct cdev cdev;
};

	int i;
/* Driver private data structure */

struct pcdrv_private_data
{
	int total_devices;

	struct pcdev_private_data pcdev_data[MAX_DEVICES];
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
int pcd_probe(struct platform_device *pdev)
{
  pr_info("Device is detected\n");

  return 0;
}

/* Function gets called when platform device was removed from system */
int pcd_remove(struct platform_device *pdev)
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
