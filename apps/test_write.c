#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <fs.h>

#define ASSERT(cond, func)                               \
do {                                                     \
	if (!(cond)) {                                       \
		fprintf(stderr, "Function '%s' failed\n", func); \
		exit(EXIT_FAILURE);                              \
	}                                                    \
} while (0)


void read_file(int fd, void** buf, int num_bytes)
{
	int ret;
	fs_lseek(fd, 0);
	ret = fs_read(fd, *buf, num_bytes);
	ASSERT(ret == num_bytes, "fs_read");
}

int main(int argc, char *argv[])
{
	int ret;
	char *diskname;
	int fd[8];
	int libc_fd;
	char data[26] = "abcdefghijklmnopqrstuvwxyz";
	char buffer[12288];
    char output[12288];

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
	ASSERT(!ret, "fs_create myfile");

	fd[0] = fs_open("myfile");
	ASSERT(fd >= 0, "fs_open");

	/* Test 1: Write <1 block's worth of data */
	ret = fs_write(fd[0], data, sizeof(data));
	ASSERT(ret == sizeof(data), "fs_write");

	fs_close(fd[0]);
	fd[0] = fs_open("myfile");
	fs_lseek(fd[0], fs_stat(fd[0]));

	/* Test 2: Write <1 block from offset (same thing as above, just from an offset */
	ret = fs_write(fd[0], data, sizeof(data));
	ASSERT(ret == sizeof(data), "fs_write");

	/* Check Test 1 & 2: Read the data to check it was written properly */
    fs_lseek(fd[0], 0);
    ret = fs_read(fd[0], output, sizeof(data) * 2);
    ASSERT(ret == (sizeof(data) * 2), "fs_read");
    // ASSERT(!ret, "fs_read");  // Should read nothing since offset at end - without lseek
    printf("%s\n", (char*)output);  // Expect: data, twice


	/* Test 3: Write exactly 1 block */
	// Read 1 block of local data file's contents into buffer
	libc_fd = open("data", O_RDONLY);  // Created by script_write.sh
	ret = read(libc_fd, buffer, 4096);
	ASSERT(ret == 4096, "open");
	close(libc_fd);
	
	ret = fs_create("data1");
	ASSERT(!ret, "fs_create data1");

	fd[1] = fs_open("data1");
	ASSERT(fd >= 0, "fs_open");

	// Write local data's contents into data1 file on disk
	ret = fs_write(fd[1], buffer, 4096);
	ASSERT(ret == 4096, "fs_write");

	/* Check Test 3: Read it out */
	fs_lseek(fd[1], 0);
    ret = fs_read(fd[1], output, 4096);
    ASSERT(ret == 4096, "fs_read");
	// printf("%s\n", (char*)output);

	/* Test 4: Write exactly 1 block from offset */
	ret = fs_create("data2");
	ASSERT(!ret, "fs_create data2");

	fd[2] = fs_open("data2");
	ret = fs_write(fd[2], data, sizeof(data));  // write something else first so there is an offset
	ASSERT(ret == sizeof(data), "fs_write");
	// Write 1 block from the offset
	ret = fs_write(fd[2], buffer, 4096);
	ASSERT(ret == 4096, "fs_write");

	/* Check Test 4: Read it out */
	// read_file(fd[2], (void**)&output, 4096);
	// fs_lseek(fd[2], 0);
	// ret = fs_read(fd[2], output, 4096);
	// ASSERT(ret == 4096, "fs_read");
	// printf("%s\n", (char*)output);  // Expect: data + 4096 bytes of buffer




	/* Test cases:
	 * Write less than 1 block
	 * Write less than 1 block from offset
	 * Write exactly 1 block
	 * Write exactly 1 block from offset (should take up 2 blocks)
	 * Write multiple blocks
	 * Write multiple blocks from offset
	 * Write entire disk
	 * Write entire disk + try more after
	 * Open 32 fds and try to open more
	 * 
	 * Notes:
	 * Read & print all of them to ensure we have correct outputs (part of script)
	 * Use lseek on some, close fd for others to ensure everything's working properly
	 */


    

	/* Close file and unmount */
	fs_close(fd[0]);
	fs_close(fd[1]);
	fs_umount();

	return 0;
}
