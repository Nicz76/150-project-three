#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs.h>

#define ASSERT(cond, func)                               \
do {                                                     \
	if (!(cond)) {                                       \
		fprintf(stderr, "Function '%s' failed\n", func); \
		exit(EXIT_FAILURE);                              \
	}                                                    \
} while (0)

int main(int argc, char *argv[])
{
	int ret;
	char *diskname;
	int fd;
	// char data[26];

	if (argc < 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");
	printf("Mounted successfully\n");

    /* Print info about disk */
    ret = fs_info();
    ASSERT(!ret, "fs_info");
	printf("Info'd successfully\n");

	/* Open file */
	fd = fs_open("myfile");
	ASSERT(fd >= 0 && fd < 32, "fs_open");

    /* Check size of myfile */
    ret = fs_stat(fd);
    ASSERT(ret != -1 && ret >= 0, "fs_stat");

    /* Lseek */
    ret = fs_lseek(fd, fs_stat(fd));
    ASSERT(!ret, "fs_lseek");

	// /* Read some data */
	// fs_lseek(fd, 12);
	// ret = fs_read(fd, data, 10);
	// ASSERT(ret == 10, "fs_read");
	// ASSERT(!strncmp(data, "mnopqrstuv", 10), "fs_read");

	/* Close file */
	ret = fs_close(fd);
    ASSERT(!ret, "fs_close");

    /* Unmount disk */
	ret = fs_umount();
    ASSERT(!ret, "fs_umount");

	return 0;
}
