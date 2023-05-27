#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC NULL

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

int rdir_free_entries;


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
	for (int i = 0; i < (int)sb.FAT_blocks; i++) {
		block_read(i + 1, &FAT[i]);
	}

	// Read root directory from virtual disk
	block_read(sb.root_dir_idx, &root_dir);
	
	// Initialize rdir_free_entries: number of free entries in the root directory
	rdir_free_entries = 0;
	// Count and update number of free entries
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (root_dir[i].filename[0] == '\0')
			rdir_free_entries++;
	}

	printf("Created virtual disk '%s' with '%d' data blocks\n", "diskname", (int)(sb.total_data_blks ));
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

	//free(FAT);
	return 0;
	//Close virtual disk(make sure the virtual disk is up to date)
	//* Return: -1 if no FS is currently mounted, or if the virtual disk cannot be
 //* closed, or if there are still open file descriptors. 0 otherwise.
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	// FIXME: need to handle no diskname ("Usage: __") and nonexistant diskname outputs?

	// Error checking: no underlying virtual disk was opened
	if (block_disk_count() == -1) {
		return -1;
	}

	//Show info about volume
	/* TODO: Phase 1 */
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", block_disk_count());
	printf("fat_blk_count=%d\n", sb.FAT_blocks);
	printf("rdir_blk=%d\n", sb.root_dir_idx);
	printf("data_blk=%d\n", sb.data_blks_idx);
	printf("data_blk_count=%d\n", sb.total_data_blks);
	// FIXME:
	// printf("fat_free_ratio=%d/%d\n", );
	printf("rdir_free_ratio=%d/%d\n", rdir_free_entries, FS_FILE_MAX_COUNT);
	
	return 0;
}

int fs_create(const char *filename)
{
	// Error checking: no FS mounted or @filename is invalid or string @filename is too long
	// or root directory already contains %FS_FILE_MAX_COUNT files (i.e. no free entries)
	if (block_disk_count == -1 || filename == NULL || (strlen(filename) + 1) > FS_FILENAME_LEN || rdir_free_entries == 0) {
		return -1;
	}

	// Error checking: filename already exists in root directory
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		//FIXME: Need to make sure they compare the names rather than address?
		if ((const char*)(root_dir[i].filename) == filename){
			return -1;
		}
	}

	// Find first available index in the root directory
	int first_avail_idx = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (root_dir[i].filename[0] == '\0') {
			first_avail_idx = i;
			break;
		}
	}

	// //The directory is already full
	// if (first_avail_idx == -1){
	// 	return -1;
	// }
	
	// Add filename entry to root directory
	strcpy(root_dir[first_avail_idx].filename, filename);  // FIXME: need memcpy instead?
	// root_dir[first_avail_idx].filename=(uint8_t)(filename);
	root_dir[first_avail_idx].size = 0;
	root_dir[first_avail_idx].data_start_idx = FAT_EOC;
	//int *ptr = FAT.entries;

	// Decrement number of free entries in root directory
	rdir_free_entries--;

	return 0;
	/* TODO: Phase 2*/ 

// 	 * Create a new and empty file named @filename in the root directory of the
//  * mounted file system. String @filename must be NULL-terminated and its total
//  * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
//  * character).
//  *
//  * Return: -1 if no FS is currently mounted, or if @filename is invalid, or if a
//  * file named @filename already exists, or if string @filename is too long, or
//  * if the root directory already contains %FS_FILE_MAX_COUNT files. 0 otherwise.
 }

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	// Error checking: no FS mounted or @filename is invalid
	if (block_disk_count == -1 || filename == NULL) {
		return -1;
	}
	
	// Find the entry corresponding to @filename in root directory
	int file_idx = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {  
		if (!strcmp(root_dir[i].filename, filename)) {
			file_idx = i;
			break;
		}
	}
	
	// Error checking: no file named @filename to delete
	if (file_idx == -1) {
		return -1;
	}
	
	// Error checking: @filename is currently open - FIXME: after Phase 3
	

	// Remove file's contents from FAT 
	int ptr = root_dir->data_start_idx;
	int ptr_content;

	while (ptr != FAT_EOC) {
		ptr_content = FAT->entries[ptr];
		FAT->entries[ptr] = 0;
		ptr = ptr_content;
	}
	
	// Remove file's entry from root directory
	root_dir[file_idx].size = NULL;
	root_dir[file_idx].filename[0] = '\0';

	// Increment number of free entries in root directory
	rdir_free_entries++;

	return 0;
}

int fs_ls(void)
{
// 	 * fs_ls - List files on file system
//  *
//  * List information about the files located in the root directory.
//  *
//  * Return: -1 if no FS is currently mounted. 0 otherwise.
	/* TODO: Phase 2 */
}

// int fs_open(const char *filename)
// {
// 	/* TODO: Phase 3 */
// }

// int fs_close(int fd)
// {
// 	/* TODO: Phase 3 */
// }

// int fs_stat(int fd)
// {
// 	block_disk_count();
// 	/* TODO: Phase 3 */
// }

// int fs_lseek(int fd, size_t offset)
// {
// 	/* TODO: Phase 3 */
// }

// int fs_write(int fd, void *buf, size_t count)
// {
// 	/* TODO: Phase 4 */
// 	//Modify and sycyhn data everytime there is
// 	//a change in the data blocks
// }

// int fs_read(int fd, void *buf, size_t count)
// {
// 	/* TODO: Phase 4 */
// }

// 