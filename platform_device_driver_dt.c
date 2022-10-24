#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include<linux/of_device.h>
#include "platform.h"

#define RDONLY 0x1
#define WRONLY 0x10
#define RDWR   0x11

#define MAX_DEVICES 10

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__


int pcd_probe(struct platform_device *pdev);
int pcd_remove(struct platform_device *pdev);

/* Passing configs parameters into driver */
struct device_config
{
	int config_item1;
	int config_item2;
};

/* Enumerators for simplify drivers identyfication */
enum pcdev_names
{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X
};

struct device_config pdevs_config[] =
{
	[PCDEVA1X] = {.config_item1 = 60, .config_item2 = 21},
	[PCDEVB1X] = {.config_item1 = 40, .config_item2 = 11},
	[PCDEVC1X] = {.config_item1 = 66, .config_item2 = 51},
	[PCDEVD1X] = {.config_item1 = 16, .config_item2 = 22},
};

/* Platform devices naming array */
struct platform_device_id pcdevs_ids[]=
{
	{.name = "pcdev-A1x", .driver_data = PCDEVA1X},
	{.name = "pcdev-B1x", .driver_data = PCDEVB1X},
	{.name = "pcdev-C1x", .driver_data = PCDEVC1X},
	{.name = "pcdev-D1x", .driver_data = PCDEVD1X},

	{ } /* NULL terminated entry */
};
struct of_device_id org_pcdev_dt_match[] = {

	{.compatible = "pcdev-A1X", .data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1X", .data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1X", .data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1X", .data = (void*)PCDEVD1X},

	{ } /* NULL terminated end */
};

/* Instance of paltform driver representing structure */
struct platform_driver pcd_platform_driver = {
  .probe = pcd_probe,
  .remove = pcd_remove,
	.id_table = pcdevs_ids,
  .driver = {
		.name = "pchar-platform-device",
		.of_match_table = org_pcdev_dt_match
	}
};

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

  dev_info(&pdev->dev, "Device was removed\n");

  return 0;
}

/* This function is returning data from device tree */

struct pcdev_platform_data* pcdev_get_platdata_from_dt(struct device *dev)
{
		struct device_node *dev_node = dev->of_node;
		struct pcdev_platform_data *pdata;
		if(!dev_node)
		{
			/* This probe not happen because of device tree node */
			return NULL;
		}

			pdata = devm_kzalloc(dev, sizeof(*pdata),GFP_KERNEL);
			/* Error if something goes wrong with memory allocation */

			if(!pdata)
			{
				dev_info(dev, "Cannot allocate memory\n");
				return ERR_PTR(-ENOMEM);
			}

			/* Read serial_number from of_noda data */
			if(of_property_read_string(dev_node, "org,device-serial-num", &pdata->serial_number))
			{
				dev_info(dev, "Missing serial number property\n");
				return ERR_PTR(-EINVAL);
			}

			/* Read buffer size from of_node data */
			if(of_property_read_u32(dev_node, "org,size", &pdata->size))
			{
				dev_info(dev, "Missing size property\n");
				return ERR_PTR(-EINVAL);
			}

			/* Read permission from of_node data */

			if(of_property_read_u32(dev_node, "org,perm", &pdata->permission))
			{
				dev_info(dev, "Missing size property\n");
				return ERR_PTR(-EINVAL);
			}

		return pdata;
}

/* Function gets called when matched platform device was found */
int pcd_probe(struct platform_device *pdev)
{


	/* 1. Get the device platform data */
	struct pcdev_private_data *dev_data;
	struct pcdev_platform_data *pdata;
	struct device *dev = &pdev -> dev;
	int driver_data;
	int ret;

	dev_info(dev, "Device detected\n");

	/* Extract data from device tree */
	pdata = pcdev_get_platdata_from_dt(dev);

	/* Check if returned pointer have error code and return this error code */
	if(IS_ERR(pdata))
	{
		return PTR_ERR(pdata);
	}

	if(!pdata)
	{
		pdata = (struct pcdev_platform_data*)dev_get_platdata(dev);

		/* 1.e Error handling */
		if(!pdata)
		{
			pr_info("No platform data available\n");
			return -EINVAL;
		}
		driver_data = pdev -> id_entry -> driver_data;
	}
	else
	{
		/* Extract data from match table of device */
		driver_data = *((int*)of_device_get_match_data(dev));

	}
	/* 2. Dynamically allocate memory for the device private data */

	dev_data = devm_kzalloc(&pdev -> dev, sizeof(*pdev), GFP_KERNEL);

	/* 2.e Erorr hangling for allocation */

	if(!dev_data)
	{
		pr_info("Cannot allocate memory\n");
		return -ENOMEM;
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

	/* Printing config info */

	pr_info("Device config item 1 is: %d\n", pdevs_config[driver_data].config_item1);
	pr_info("Device config item 2 is: %d\n", pdevs_config[driver_data].config_item2);

	/* 3. Dynamically allocate device data buffer */
	dev_data->buffer = devm_kzalloc(&pdev -> dev, dev_data->pdata.size, GFP_KERNEL);

	/* 3.e Erorr hangling for allocation */
	if(!dev_data->buffer)
	{
		pr_info("Cannot allocate memory\n");
		return -ENOMEM;
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
		return ret;
	}
	/* 7. Populate the sysfs (/sys/class) with device information */
	pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, dev_data-> dev_num, NULL,"pcdev-%d", pdev-> id);
	/*7.e Erorr handling */
	if(IS_ERR(pcdrv_data.pcd_device))
	{
		pr_err("Device create failed\n");
		ret = PTR_ERR(pcdrv_data.pcd_device);
		cdev_del(&dev_data -> cdev);
		return ret;
	}
	/*Increment device counter */
	pcdrv_data.total_devices++;

	/* Return 0 if execution ends without any errors */
	pr_info("Module probe function execution successfull");


	return 0;
}

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
