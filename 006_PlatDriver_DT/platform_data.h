#ifndef PLATFORM_DATA_H
#define PLATFORM_DATA_H

struct pcdev_platform_data
{
	int size;
	int perm;
	const char *serial_data;
};

#define RDWR	0x11
#define RDONLY	0x01
#define WRONLY	0x10

#endif
