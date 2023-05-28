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

struct __attribute__((__packed__)) filedes {
	uint8_t filename[FS_FILENAME_LEN];
	uint16_t data_start_idx;
	int offset;
};

// struct __attribute__((__packed__)) root_directory {
//     struct root_dir_entry files[FS_FILE_MAX_COUNT];
// };

//Old way of creating a struct for FAT
// struct __attribute__((__packed__)) FAT_block {
//     uint16_t entries[BLOCK_SIZE / 2];
// };


struct superblock sb;
// struct root_directory root_dir;
struct root_dir_entry root_dir[FS_FILE_MAX_COUNT];
// struct FAT_block* FAT;
uint16_t* FAT;

int rdir_free_entries;
int total_file_open;
struct filedes FDT[FS_OPEN_MAX_COUNT];

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
    // FAT = malloc((int)sb.FAT_blocks * sizeof(struct FAT_block));
	int entries_per_FAT_block = BLOCK_SIZE / sizeof(uint16_t);
	FAT = malloc((int)sb.FAT_blocks * entries_per_FAT_block);

    // Read FAT from virtual disk
	for (int i = 0; i < (int)sb.FAT_blocks; i++) {
		block_read(i + 1, &FAT[i * entries_per_FAT_block]);
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

	// Initialize file descriptor table (FDT)
	// set each filedes to "empty" by initializing first character of filename to null char
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		FDT[i].filename[0] = '\0';
		FDT[i].offset = 0;
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

	free(FAT);
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

//For FAT ratio by Professor
//for loop to loop all the entries of the FAT table
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
	int FAT_file_idx = root_dir[file_idx].data_start_idx;
	int next_FAT_file_idx;

	while (FAT_file_idx != FAT_EOC) {
		next_FAT_file_idx = FAT[FAT_file_idx];
		FAT[FAT_file_idx] = 0;
		FAT_file_idx = next_FAT_file_idx;
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
	// Error checking: no underlying virtual disk was opened
	if (block_disk_count == -1){
		return -1;
	}

	printf("FS Ls:\n");

	// Print info for each file in root directory
	for (int i = 0 ; i < FS_FILE_MAX_COUNT; i++) {
		if (root_dir[i].filename[0] != '\0') {
			printf("file: %s, size: %d, data_blk: %d\n", root_dir[i].filename, root_dir[i].size, root_dir[i].data_start_idx);
		}
	}

	return 0;
// 	 * fs_ls - List files on file system
//  *
//  * List information about the files located in the root directory.
//  *
//  * Return: -1 if no FS is currently mounted. 0 otherwise.
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	// Error checking: no FS mounted or @filename is invalid 
	// or there are already %FS_OPEN_MAX_COUNT files currently open
	if (block_disk_count == -1 || filename == NULL || total_file_open >= FS_OPEN_MAX_COUNT) {
		return -1;
	}
	
	//Change the global variable to an array to keep track of the file
	//that are open and need find a way to set the offset if the filename
	//are the same
	// Find @filename in root_dir
	int rdir_file_idx = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if (root_dir[i].filename == filename) {
			rdir_file_idx = i;
			break;
		}
	}
	// Error checking: there is no file named @filename to open, return -1
	if (rdir_file_idx == -1) {
		return -1;
	}
	
	//If the filename is the same need to change the offset
	//Use lseek() function for this problem

	// Find first available file descriptor index in FDT
	int fd;
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if (FDT[i].filename[0] == '\0') {
			fd = i;
			break;
		}
	}

	// Populate the filedes struct for this file (??)
	strcpy(FDT[fd].filename, root_dir[rdir_file_idx].filename);
	FDT[fd].data_start_idx = root_dir[rdir_file_idx].data_start_idx;
	FDT[fd].offset = 0;

	// for (int i = 0; i < total_file_open; i++){
	// 	if (FDT[i] == fd) {
	// 		lseek(fd);
	// 	}
	// }

	//This adds the fd number to the global array containing 
	//all current open files descriptors
	// FDT[total_file_open] = fd;
	total_file_open++;

	return fd;
	/* TODO: Phase 3 */
// 	* Open file named @filename for reading and writing, and return the
//  * corresponding file descriptor. The file descriptor is a non-negative integer
//  * that is used subsequently to access the contents of the file. The file offset
//  * of the file descriptor is set to 0 initially (beginning of the file). If the
//  * same file is opened multiple files, fs_open() must return distinct file
//  * descriptors. A maximum of %FS_OPEN_MAX_COUNT files can be open
//  * simultaneously.
//  *
//  * Return: -1 if no FS is currently mounted, or if @filename is invalid, or if
//  * there is no file named @filename to open, or if there are already
//  * %FS_OPEN_MAX_COUNT files currently open. Otherwise, return the file
//  * descriptor.
}

int fs_close(int fd)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open)
	if (block_disk_count == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0') {
		return -1;
	}


	// int fd_file_idx = -1;
	// for (int i = 0; i < FS_OPEN_MAX_COUNT; i++){
	// 	if (FDT[i] == fd) {
	// 		fd_file_idx = i;
	// 		break;
	// 	}
	// }
	
	// //Error Checking : invalid fd
	// if (fd_file_idx == -1){
	// 	return -1;
	// }



	FDT[fd].filename[0] = '\0';
	FDT[fd].data_start_idx = 0;
	FDT[fd].offset = 0;

	total_file_open--;
	// close(fd);
	return 0;
	
	/* TODO: Phase 3 */
// 	 * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
//  * invalid (out of bounds or not currently open). 0 otherwise.
}

int fs_stat(int fd)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open)
	if (block_disk_count == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0') {
		return -1;
	}

	// Find file in root directory to grab its size
	uint32_t file_size;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(FDT[fd].filename, root_dir[i].filename)) {
			file_size = root_dir[i].size;
			break;
		}
	}
	
	return file_size;
	/* TODO: Phase 3 */
// 	 * fs_stat - Get file status
//  * @fd: File descriptor
//  *
//  * Get the current size of the file pointed by file descriptor @fd.
//  *
//  * Return: -1 if no FS is currently mounted, of if file descriptor @fd is
//  * invalid (out of bounds or not currently open). Otherwise return the current
//  * size of file.
}

int fs_lseek(int fd, size_t offset)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open)
	//                 or @offset is larger than current file size
	if (block_disk_count == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || offset > fs_stat(fd)) {
		return -1;
	}

	FDT[fd].offset = offset;

	return 0;

	/* TODO: Phase 3 */
// 	* fs_lseek - Set file offset
//  * @fd: File descriptor
//  * @offset: File offset
//  *
//  * Set the file offset (used for read and write operations) associated with file
//  * descriptor @fd to the argument @offset. To append to a file, one can call
//  * fs_lseek(fd, fs_stat(fd));
//  *
//  * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
//  * invalid (i.e., out of bounds, or not currently open), or if @offset is larger
//  * than the current file size. 0 otherwise.
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	//Modify and sycyhn data everytime there is
	//a change in the data blocks
}

int fs_read(int fd, void *buf, size_t count)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open) or @buf is NULL
	if (block_disk_count == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || buf == NULL) {
		return -1;
	}
	
	// Need a bounce buffer for reading and writing
	char * bounce;

// 	 * Attempt to read @count bytes of data from the file referenced by file
//  * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
//  * enough to hold at least @count bytes.
//  *
//  * The number of bytes read can be smaller than @count if there are less than
//  * @count bytes until the end of the file (it can even be 0 if the file offset
//  * is at the end of the file). The file offset of the file descriptor is
//  * implicitly incremented by the number of bytes that were actually read.
//  *
//  * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
//  * invalid (out of bounds or not currently open), or if @buf is NULL. Otherwise
//  * return the number of bytes actually read.
//  */
	/* TODO: Phase 4 */
}

