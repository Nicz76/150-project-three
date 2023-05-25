#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

struct __attribute__((__packed__)) superblock {
	uint64_t signature;
	uint16_t total_blocks;
	uint16_t root_dir_idx;
	uint16_t data_blks_idx;
	uint16_t total_data_blks;
	uint8_t FAT_blocks;
	uint8_t padding[4079];
};

struct __attribute__((__packed__)) root_dir_entry {
    uint8_t filename[FS_FILENAME_LEN];
    uint32_t size;
    uint16_t data_start_idx;
    uint8_t padding[10];
};

// struct __attribute__((__packed__)) root_directory {
//     struct root_dir_entry files[FS_FILE_MAX_COUNT];
// };

struct __attribute__((__packed__)) FAT_block {
    uint16_t entries[BLOCK_SIZE / 2];
};


struct superblock sb;
// struct root_directory root_dir;
struct root_dir_entry root_dir[FS_FILE_MAX_COUNT];
struct FAT_block* FAT;



/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{	
	uint64_t expected_signature = 0x4543533135304653;  // "ECS150FS"
	
	// Error checking: Virtual file disk @diskname cannot be opened
	if (block_disk_open(diskname) == -1) {
		return -1;
	}

	// Read superblock from virtual disk
	block_read(0, &sb);  // FIXME: correct usage?

	// Error checking: file system has expected format
	if (sb.signature != expected_signature) {
		return -1;
	}

    // Error checking: total number of blocks is same as block_disk_count() of total
	int total_number_of_block = 1 + (int)(sb.FAT_blocks)+ 1 + (int)(sb.total_data_blks);
	if (total_number_of_block != block_disk_count()) {
		printf("Number of blocks do not match\n");
		return -1;
	}

    // Allocate FAT blocks
    FAT = malloc((int)sb.FAT_blocks * sizeof(struct FAT_block));

    // Read FAT from virtual disk



	//find the diskname (checked)
	//Open the virtual disk (Checked)
	//Read the metadata(superbloc, fat, root directory) (Unsured)
	/* TODO: Phase 1 */

	return 0;
}

int fs_umount(void)
{
	if (block_disk_close() == -1){
		//Checks if the virtual disk file is open
		return -1;
	} 

	return 0;
	//Close virtual disk(make sure the virtual disk is up to date)
	//* Return: -1 if no FS is currently mounted, or if the virtual disk cannot be
 //* closed, or if there are still open file descriptors. 0 otherwise.
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	//Show info about volume
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	block_disk_count();
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

