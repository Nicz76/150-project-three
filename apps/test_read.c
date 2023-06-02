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
    //char *filename = "heart.csv";
	char *filenames = "smallfile";
    // char *filename = "myfile";
	int fd[36];
	char data[BLOCK_SIZE * 3];
	char smalldata[BLOCK_SIZE];

	if (argc < 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	/* Open file */
	/*fd[0] = fs_open(filename);
	ASSERT(fd[0] >= 0, "fs_open");
    
    fd[1] = fs_open(filename);
    ASSERT(fd[1] >= 0, "fs_open");

    fd[2] = fs_open(filename);
    ASSERT(fd[2] >= 0, "fs_open");
	*/
	fd[3] = fs_open(filenames);
	ASSERT(fd[3] >= 0, "fs_open");
	
	// /* Read some data */
    // /* Read 1 block from start */
    // ret = fs_read(fd[0], data, BLOCK_SIZE);
    // printf("Read one datablock");
    // // printf("ret = %d\n", ret);
    // // ASSERT(ret == BLOCK_SIZE, "fs_read");
    // ASSERT(ret == BLOCK_SIZE, "fs_read");

	// /*Read multiple blocks of data*/
	// ret = fs_read(fd[1], data, (BLOCK_SIZE * 2));
	// printf("%s", (char*)data);
	// ASSERT(ret == (BLOCK_SIZE * 2), "fs_read multiple datablocks");

	// /*Read from an offset*/
	// ret = fs_lseek(fd[2], 10);
	// ASSERT(!ret, "fs_lseek");
	// ret = fs_read(fd[2], data, 90);
	// ASSERT(ret == 90, "fs_read offset");

	// /*Read multiple blocks from an offset*/
	// ret = fs_read(fd[2], data, BLOCK_SIZE);
	// ASSERT(ret == BLOCK_SIZE, "fs_read offset multiple bytes");

	/*Read a small file with larger count than file size*/
	ret = fs_read(fd[3], data, BLOCK_SIZE);
	ASSERT(ret == 35, "fs_read max data read");

	/*TESTING FOR ERRORS: invalid fd*/
	ret = fs_read(-1, data, BLOCK_SIZE);
	ASSERT(ret == -1, "Error of reading an invaild fd");

	// ret = fs_read(fd[2], NULL , BLOCK_SIZE);
	// ASSERT(ret == -1, "Error of buf is NULL");
    /* Read 3 blocks from offset */
    // fs_lseek(fd[1], )
    // ret = fs_read(fd[1],)

	// fs_lseek(fd[0], 12);
	// ret = fs_read(fd, data, 10);
	// ASSERT(ret == 10, "fs_read");
	// ASSERT(!strncmp(data, "mnopqrstuv", 10), "fs_read");

	/* Close file and unmount */
	// fs_close(fd[0]);
    // fs_close(fd[1]);
    // fs_close(fd[2]);
	fs_close(fd[3]);
	fs_umount();

	return 0;
}
