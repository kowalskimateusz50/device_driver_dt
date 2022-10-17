#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "platform.h"

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

struct pcdrv_private_data pcdrv_data = {
	.total_devices = 0
};


loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{

	/* Define current cursor position */
	loff_t curr_f_pos;

	/* Extracting private data file structure from filp */
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	/* Defining device memory size */
	unsigned int dev_mem_size = pcdev_data -> pdata.size;

	pr_info("lseek requested \n");
	pr_info("Current value of file position = %lld", filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:

			/* Check if offset is greater than buffer array size */

			if(offset > dev_mem_size  || offset < 0)
			{
				return -EINVAL;
			}

			filp-> f_pos = offset;
			break;

		case SEEK_CUR:

			/*Check if sum of current file position and offset is not greater than buffer size and less than 0 */
			curr_f_pos = filp-> f_pos + offset;
			if(curr_f_pos > dev_mem_size || curr_f_pos <0)
			{
				return -EINVAL;
			}

			filp-> f_pos = curr_f_pos;

			break;

		case SEEK_END:

			/*Check if sum of current file position and offset is not greater than buffer size and less than 0 */
			curr_f_pos = dev_mem_size + offset;
			if(curr_f_pos > dev_mem_size || curr_f_pos <0)
			{
				return -EINVAL;
			}
			filp-> f_pos = curr_f_pos;
			break;
		default:
			/* Invalid whence argument has been received */
			return -EINVAL;

		pr_info("New value of file pointer file position = %lld\n", filp->f_pos);
	}

	return filp->f_pos;

}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	/* Extracting private data file structure from filp */
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	/* Defining device memory size */
	unsigned int dev_mem_size = pcdev_data -> pdata.size;

	/* 0. Print read request amount of data bytes and actuall position of data before read */

	pr_info("\nread requested for %zu bytes \n",count);
	pr_info("\nCurrent position of data before reading process = %lld\n", *f_pos);

	/* 1. Check if value of count data is not greater than buffer size, and if is trim count value */

	if((*f_pos + count) > dev_mem_size)
	{
		count = dev_mem_size - *f_pos;
	}

	/* 2. Copy kernel buffer data to user space */

	if(copy_to_user(buff, pcdev_data->buffer+(*f_pos), count))
	{
		return -EFAULT;
	}

	/* 4. Update (increment) the current file position */

	*f_pos += count;

	/* 5. Print amount of data successfully read and updated file position */

	pr_info("\nNumber of bytes successfully read = %zu\n", count);
	pr_info("\nUpdated position of data = %lld\n", *f_pos);

	/* 6. Return amount of data bytes if data was successfully read */

	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count,  loff_t *f_pos)
{

	/* Extracting private data file structure from filp */
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	/* Defining device memory size */
	unsigned int dev_mem_size = pcdev_data -> pdata.size;

	/* 0. Print read request amount of data bytes and actuall position of data before read */

	pr_info("Write requested for %zu bytes\n",count);
	pr_info("\nCurrent position of data before writing process = %lld\n", *f_pos);

	/* 1. Check if value of count data is not greater than buffer size, and if is trim count value */

	if((*f_pos + count) > dev_mem_size)
	{
		count = dev_mem_size - *f_pos;
	}

	/* If on input is zero value of data return error */

	if(!count)
	{
		pr_err("No memory left on device\n");
		return -ENOMEM;
	}

	/* 2. Copy user space data to kernel space */

	if(copy_from_user(pcdev_data->buffer+(*f_pos), buff, count))
	{
		return -EFAULT;
	}

	/* 4. Update (increment) the current file position */

	*f_pos += count;

	/* 5. Print amount of data successfully written and updated file position */

	pr_info("\nNumber of bytes successfully written = %zu\n", count);
	pr_info("\nUpdated position of data = %lld\n", *f_pos);

	/* 6. Return count of data bytes if data was successfully written */

	return count;

}
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

int pcd_open(struct inode *inode, struct file *filp)
{
	int minor_n;
	int ret;

	struct pcdev_private_data *pcdev_data;

	/* Find out on which device numer open was attempted by user space */
	minor_n = MINOR(inode->i_rdev);
	pr_info("Device number %d was attempted\n",minor_n);

	/* Extract device private data structure from *cdev */

	pcdev_data = container_of(inode -> i_cdev, struct pcdev_private_data, cdev);

	/* Supply other methods with device private data */
	filp->private_data = pcdev_data;

	ret = check_permission(pcdev_data->pdata.permission, filp->f_mode);

	if(!ret)
	{
		pr_info("Open was succesfull\n");
	}
	else
	{
		pr_info("Open was unsuccessfull\n");
		return -EPERM;
	}

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
/* Function gets called when platform device was removed from system */
int pcd_remove(struct platform_device *pdev)
{
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev -> dev);

	/* 1. Remove a device that was created with device_create() */
	device_destroy(pcdrv_data.pcd_class, dev_data -> dev_num);

	/* 2. Remove a cdev entry from the system */
	cdev_del(&dev_data -> cdev);

	/*3. Decrement devices counter */
	pcdrv_data.total_devices++;

  pr_info("Device was removed\n");

  return 0;
}

/* Function gets called when matched platform device was found */
int pcd_probe(struct platform_device *pdev)
{
	/* 1. Get the device platform data */
	int ret;

	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data *pdata;

	pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);

	/* 1.e Error handling */
	if(!pdata)
	{
		pr_info("No platform data available\n");
		ret = EINVAL;
		goto out;
	}
	/* 2. Dynamically allocate memory for the device private data */

	dev_data = devm_kzalloc(&pdev -> dev, sizeof(*pdev), GFP_KERNEL);

	/* 2.e Erorr hangling for allocation */

	if(!dev_data)
	{
		pr_info("Cannot allocate memory\n");
		ret = ENOMEM;
		goto out;
	}
	/* 2f. Passing data to remove function by dev_set_drvdata and dev_get_drvdata */
	dev_set_drvdata(&pdev-> dev, dev_data);

	/* 2g. Copy platform data to device data */
	dev_data -> pdata.size = pdata -> size;
	dev_data -> pdata.permission = pdata -> permission;
	dev_data -> pdata.serial_number = pdata -> serial_number;

	/* Print previously copied data */
	pr_info("Device size is: %d\n", dev_data -> pdata.size);
	pr_info("Device permission is: %d\n", dev_data -> pdata.permission);
	pr_info("Device serial number is: %s\n", dev_data -> pdata.serial_number);

	/* 3. Dynamically allocate device data buffer */
	dev_data->buffer = devm_kzalloc(&pdev -> dev, dev_data->pdata.size, GFP_KERNEL);

	/* 3.e Erorr hangling for allocation */
	if(!dev_data->buffer)
	{
		pr_info("Cannot allocate memory\n");
		ret = ENOMEM;
		goto dev_data_free;
	}
	/* 4. Get device number */
	dev_data -> dev_num = pcdrv_data.device_number_base + pdev->id;

	/* 5. Initialization the cdev structure with fops */
	cdev_init(&dev_data->cdev, &pcd_fops);

	/* 6. Register a device (cdev structure) with VFS */
	dev_data -> cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data -> cdev,dev_data -> dev_num,1);
	if(ret <0)
	{
		pr_err("Cdev add failed \n");
		goto buffer_free;
	}
	/* 7. Populate the sysfs (/sys/class) with device information */
	pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, dev_data-> dev_num, NULL,"pcdev-%d", pdev-> id);
	/*7.e Erorr handling */
	if(IS_ERR(pcdrv_data.pcd_device))
	{
		pr_err("Device create failed\n");
		ret = PTR_ERR(pcdrv_data.pcd_device);
		goto cdev_del;
	}
	/*Increment device counter */
	pcdrv_data.total_devices++;

	/* Return 0 if execution ends without any errors */
	pr_info("Module probe function execution successfull");

	return 0;

	/* 8. Error handling */
	cdev_del:
		cdev_del(&dev_data -> cdev);

	buffer_free:
		devm_kfree(&pdev-> dev, dev_data-> buffer);

	dev_data_free:
		devm_kfree(&pdev-> dev, dev_data);

	out:
		pr_info("Device probe failed \n");
		return ret;

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
