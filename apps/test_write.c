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
	char data[27] = "abcdefghijklmnopqrstuvwxyz";
    char output[27];

	if (argc < 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	/* Create file and open */
	ret = fs_create("myfile");
	ASSERT(!ret, "fs_create");

	fd = fs_open("myfile");
	ASSERT(fd >= 0, "fs_open");

	/* Write some data */
	ret = fs_write(fd, data, sizeof(data));
	ASSERT(ret == sizeof(data), "fs_write");


    /* Read the data to check it was written properly */
    fs_lseek(fd, 0);
    ret = fs_read(fd, output, sizeof(data));
    ASSERT(ret == sizeof(data), "fs_read");
    // ASSERT(!ret, "fs_read");  // Should read nothing since offset at end - without lseek
    printf("%s\n", (char*)output);

	/* Close file and unmount */
	fs_close(fd);
	fs_umount();

	return 0;
}
