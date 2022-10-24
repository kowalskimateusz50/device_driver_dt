/* Definition of macros */

#define RDONLY 0x1
#define WRONLY 0x10
#define RDWR   0x11

/* Definition of platform data structure */
struct pcdev_platform_data
{
	int size;
	const char* serial_number;
	int permission;
};
