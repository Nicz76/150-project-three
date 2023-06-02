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
	// int fd;
	// char data[26];

	if (argc < 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

    /* Print info about disk */
    ret = fs_info();
    ASSERT(!ret, "fs_info");

    /* Create file */
    ret = fs_create("myfile.txt");
    ASSERT(!ret, "fs_create");
    printf("Created myfile.txt\n");

    /* Check info after file creation */
    printf("LS after creating myfile.txt:\n\n");
    ret = fs_info();
    ASSERT(!ret, "fs_info");

    /* List files on disk - should see myfile.txt */
    ret = fs_ls();
    ASSERT(!ret, "fs_ls");

    /* Delete file */
    ret = fs_delete("myfile.txt");
    ASSERT(!ret, "fs_delete");

    /* Check info after file deletion */
    ret = fs_info();
    ASSERT(!ret, "fs_info");

    /* List files on disk - should see nothing */
    ret = fs_ls();
    ASSERT(!ret, "fs_ls");

    /* Unmount disk */
	ret = fs_umount();
    ASSERT(!ret, "fs_umount");

	return 0;
}