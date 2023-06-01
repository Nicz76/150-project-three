#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs.h>
#include <disk.h>

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
    char *filename = "heart.csv";
    // char *filename = "myfile";
	int fd[3];
	char data[BLOCK_SIZE * 3];

	if (argc < 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	/* Open file */
	fd[0] = fs_open(filename);
	ASSERT(fd[0] >= 0, "fs_open");
    
    fd[1] = fs_open(filename);
    ASSERT(fd[1] >= 0, "fs_open");

    fd[2] = fs_open(filename);
    ASSERT(fd[2] >= 0, "fs_open");

	/* Read some data */
    /* Read 1 block from start */
    ret = fs_read(fd[0], data, BLOCK_SIZE);
    printf("%s", (char*)data);
    // printf("ret = %d\n", ret);
    // ASSERT(ret == BLOCK_SIZE, "fs_read");
    ASSERT(ret == BLOCK_SIZE, "fs_read");

    /* Read 3 blocks from offset */
    // fs_lseek(fd[1], )
    // ret = fs_read(fd[1],)

	// fs_lseek(fd[0], 12);
	// ret = fs_read(fd, data, 10);
	// ASSERT(ret == 10, "fs_read");
	// ASSERT(!strncmp(data, "mnopqrstuv", 10), "fs_read");

	/* Close file and unmount */
	fs_close(fd[0]);
    fs_close(fd[1]);
    fs_close(fd[2]);
	fs_umount();

	return 0;
}
