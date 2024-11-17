#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>

#include "commands.h"

int main(int argc, char *argv[])
{
	int fd;
	char *value = malloc(100 * sizeof(char));
	// char *value;
	printf("\n\n****Jäbän ioctl systeemit!!!****\n");

	printf("\nOpening Driver\n");
	char *filename = "/dev/taskmonitor";
	printf("name: %s\n", filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Cannot open device file...\n");
		return 0;
	}

	printf("Reading Value from Driver\n");
	int res = ioctl(fd, TM_GET, value);
	if (res != 0) {
		printf("Error, ioctl return value %d\n", res);
	} else {
		printf("Value is:\n\n%s\n\n", value);
	}

	printf("Tapa se threadi!\n");
	res = ioctl(fd, TM_STOP);
	if (res != 0) {
		printf("Error, ioctl return value %d\n", res);
	} else {
		printf("Lobetettu\n");
	}
	// sleep(12);

	printf("UUS threadi!\n");
	res = ioctl(fd, TM_START);
	if (res != 0) {
		printf("Error, ioctl return value %d\n", res);
	} else {
		printf("Aloideddu\n");
	}

	// printf("Toine threadi??\n");
	//        res = ioctl(fd, TM_START);
	// if (res != 0) {
	// 	printf("Error, ioctl return value %d\n", res);
	// } else {
	// 	printf("Aloideddu\n");
	// }

	printf("Testataas vaihtaa pidiä lennosta!\n");
	int *param = malloc(sizeof(int));
	*param = 2;
	res = ioctl(fd, TM_PID, param);
	if (res != 0) {
		printf("Error, ioctl return value %d\n", res);
	} else {
		printf("Pid 2 seurantaan!\n");
	}

	printf("Closing Driver\n");
	close(fd);
	free(value);
	free(param);
	return 0;
}
