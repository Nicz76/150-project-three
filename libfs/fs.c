#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF

struct __attribute__((__packed__)) superblock {
	// uint64_t signature;
	uint8_t signature[8];
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
	// uint64_t expected_signature = 0x4543533135304653;  // "ECS150FS"
	
	// Error checking: Virtual file disk @diskname cannot be opened
	if (block_disk_open(diskname) == -1) {
		perror("Block disk open");
		return -1;
	}

	// Read superblock from virtual disk
	if (block_read(0, &sb) == -1) {
        return -1;
    }

	// printf("Read superblock\n");
	// printf("Signature: %lX\n", sb.signature);

	// Error checking: file system has expected format
	if (memcmp(sb.signature, "ECS150FS", 8)) {
		perror("Wrong signature");
		return -1;
	}

	// printf("Superblock signature matches\n");

    // Error checking: total number of blocks is same as block_disk_count() of total
	int total_number_of_block = 1 + (int)(sb.FAT_blocks)+ 1 + (int)(sb.total_data_blks);
	if (total_number_of_block != block_disk_count()) {
		perror("Number of blocks do not match\n");
		return -1;
	}

	// printf("Number of blocks matches block_disk_count()\n");

    // Allocate FAT blocks
    // FAT = malloc((int)sb.FAT_blocks * sizeof(struct FAT_block));
	int entries_per_FAT_block = BLOCK_SIZE / 2;
	// FAT = malloc((int)sb.FAT_blocks * entries_per_FAT_block * sizeof(uint16_t));
    FAT = calloc((int)sb.FAT_blocks * BLOCK_SIZE, sizeof(uint16_t));

	// printf("Allocated FAT blocks\n");

    // Read FAT from virtual disk
	for (int i = 0; i < (int)sb.FAT_blocks; i++) {
		if (block_read(i + 1, &FAT[i * entries_per_FAT_block]) == -1) {
            // Block read failed
            return -1;
        }
	}

	// printf("Read FAT from disk\n");

	// Read root directory from virtual disk
	if (block_read(sb.root_dir_idx, &root_dir) == -1) {
        return -1;
    }
	
	// printf("Read root directory from disk\n");
		
	// Initialize rdir_free_entries: number of free entries in the root directory
	rdir_free_entries = 0;
	// Count and update number of free entries
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (root_dir[i].filename[0] == '\0')
			rdir_free_entries++;
	}

	// printf("Initialized root directory entries\n");
	
	// Initialize file descriptor table (FDT)
	// set each filedes to "empty" by initializing first character of filename to null char
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		FDT[i].filename[0] = '\0';
		FDT[i].offset = 0;
	}

	// printf("Created virtual disk '%s' with '%d' data blocks\n", "diskname", (int)(sb.total_data_blks ));
	//find the diskname (checked)
	//Open the virtual disk (Checked)
	//Read the metadata(superbloc, fat, root directory) (Unsured)
	/* TODO: Phase 1 */

	return 0;
}

int fs_umount(void)
{	//Check if there are open file descriptors
 	if (total_file_open != 0){
		printf("There are still open file descriptors\n");
		return -1;
	}

	//Checks if the virtual disk file is open
	if (block_disk_close() == -1){		
		return -1;
	} 

	// free(FAT);
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
	int fat_free = 0;
	int total_fat_entries = sb.total_data_blks;

	for (int i = 0; i < total_fat_entries; i++){
		if (FAT[i] == 0){
			fat_free++;
		}
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
	printf("fat_free_ratio=%d/%d\n", fat_free, total_fat_entries);
	printf("rdir_free_ratio=%d/%d\n", rdir_free_entries, FS_FILE_MAX_COUNT);
	
	return 0;
}

int fs_create(const char *filename)
{
	// Error checking: no FS mounted or @filename is invalid or string @filename is too long
	// or root directory already contains %FS_FILE_MAX_COUNT files (i.e. no free entries)
	if (block_disk_count() == -1 || filename == NULL || (strlen(filename) + 1) > FS_FILENAME_LEN || rdir_free_entries == 0) {
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
	memcpy(root_dir[first_avail_idx].filename, filename, strlen(filename) + 1);  
	// root_dir[first_avail_idx].filename=(uint8_t)(filename);
	root_dir[first_avail_idx].size = 0;
	root_dir[first_avail_idx].data_start_idx = FAT_EOC;
	// root_dir[first_avail_idx].data_start_idx = 0; 

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
	if (block_disk_count() == -1 || filename == NULL) {
		return -1;
	}
	
	// Find the entry corresponding to @filename in root directory
	int file_idx = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {  
		if (!strcmp((char*)root_dir[i].filename, filename)) {
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
	root_dir[file_idx].size = 0;
	root_dir[file_idx].filename[0] = '\0';

	// Increment number of free entries in root directory
	rdir_free_entries++;

	return 0;
}

int fs_ls(void)
{
	// Error checking: no underlying virtual disk was opened
	if (block_disk_count() == -1){
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
	if (block_disk_count() == -1 || filename == NULL || total_file_open >= FS_OPEN_MAX_COUNT) {
		return -1;
	}
	
	//Change the global variable to an array to keep track of the file
	//that are open and need find a way to set the offset if the filename
	//are the same
	// Find @filename in root_dir
	int rdir_file_idx = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if (!strcmp((char*)root_dir[i].filename, filename)) {
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
	memcpy(FDT[fd].filename, root_dir[rdir_file_idx].filename, FS_FILENAME_LEN);
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
	if (block_disk_count() == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0') {
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
	if (block_disk_count() == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0') {
		return -1;
	}

	// Find file in root directory to grab its size
	uint32_t file_size;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp((char*)FDT[fd].filename, (char*)root_dir[i].filename)) {
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
	if (block_disk_count() == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || offset > (size_t)fs_stat(fd)) {
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
/*
int fs_write(int fd, void *buf, size_t count)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open) or @buf is NULL
	if (block_disk_count == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || buf == NULL) {
		return -1;
	}

	 TODO: Phase 4 
	//Modify and sycyhn data everytime there is
	//a change in the data blocks
	*
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 *
 * Attempt to write @count bytes of data from buffer pointer by @buf into the
 * file referenced by file descriptor @fd. It is assumed that @buf holds at
 * least @count bytes.
 *
 * When the function attempts to write past the end of the file, the file is
 * automatically extended to hold the additional bytes. If the underlying disk
 * runs out of space while performing a write operation, fs_write() should write
 * as many bytes as possible. The number of written bytes can therefore be
 * smaller than @count (it can even be 0 if there is no more space on disk).
 *
 * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
 * invalid (out of bounds or not currently open), or if @buf is NULL. Otherwise
 * return the number of bytes actually written.
 
}

int fs_read(int fd, void *buf, size_t count)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open) or @buf is NULL
	if (block_disk_count == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || buf == NULL) {
		return -1;
	}




	// Need a bounce buffer for reading and writing for partial read or write
	int idx = get_data_block_idx(fd);
	char bounce[BLOCK_SIZE];
	int bytes_left = count;
	int bytes_to_copy = 0;  // number of bytes to copy from bounce into buf
	int bytes_read = 0;
	int offset = FDT[fd].offset;
	// Calculate number of blocks to read (ceiling division)
	int blocks_to_read = (offset + count) / BLOCK_SIZE + ((offset + count) % BLOCK_SIZE != 0);

	// Part of one or exactly one data block to be read
	if (blocks_to_read == 1) {
		block_read(idx, bounce);
		memcpy(buf, bounce + offset, count);
		fs_lseek(fd, offset + count);
	} else { // More than one data block to be read
		for (int i = 0; i < blocks_to_read && idx != -1; i++) {
			block_read(idx, bounce);
			// First data block: offset possibly not aligned with beginning of block
			if (i == 0) {
				bytes_to_copy = offset % BLOCK_SIZE;
				memcpy(buf, offset % BLOCK_SIZE + bounce, bytes_to_copy);  // FIXME: bounce + offset correct?
			} else if (i == (blocks_to_read - 1)) {  // Last data block
				bytes_to_copy = bytes_left;
				memcpy(buf + bytes_read, bounce, bytes_to_copy);
			} else {  // Middle data blocks
				bytes_to_copy = BLOCK_SIZE;
				memcpy(buf + bytes_read, bounce, bytes_to_copy);
			}
			// Copy appropriate data into buffer
			// memcpy((buf + bytes_read), ((offset % BLOCK_SIZE) + bounce), bytes_to_copy);
			// Update variables
			bytes_read += bytes_to_copy;
			bytes_left -= bytes_to_copy;
			bytes_to_copy = 0;
			// Move the offset after the read
			fs_lseek(fd, offset + bytes_read);
			offset = FDT[fd].offset;
			// Get the index of the next data block to be read
			idx = get_data_block_idx(fd);
		}
	}

	for (int i = 0; bytes_read < count && idx != -1; i += bytes_read) {
		// bytes_left = count - bytes_read;
		// Read entire data block at idx into bounce
		block_read(idx, bounce);


		// if ((FDT[fd].offset + bytes_left) % BLOCK_SIZE > 0) {
		// 	// more block accesses needed
		// 	// find nearest next block beginning index
		// 	int next_block = ((FDT[fd].offset + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
		// 	bytes_to_copy = next_block - FDT[fd].offset;
		// }
		// Case: offset aligned exactly with the beginning of a block
		// if (FDT[fd].offset % BLOCK_SIZE == 0) {
		// 	// Number of bytes left to read is at least BLOCK_SIZE many bytes
		// 	if (bytes_left / BLOCK_SIZE >= 1) {
		// 		bytes_to_copy = BLOCK_SIZE;
		// 	} else {  // Number of bytes left to read is < BLOCK_SIZE
		// 		bytes_to_copy = bytes_left;
		// 	}
		// } else { // Case: offset not aligned with beginning of block
		// 	// Number of bytes left to read is at least BLOCK_SIZE many bytes
		// 	if (bytes_left / BLOCK_SIZE >= 1) {
		// 		bytes_to_copy = BLOCK_SIZE - (FDT[fd].offset % BLOCK_SIZE);
		// 	} else {  // Number of bytes left to read is < BLOCK_SIZE
		// 		bytes_to_copy = bytes_left;
		// 	}
		// }

		// memcpy((buf + i), bounce, bytes_to_copy);
		// fs_lseek(fd, FDT[fd].offset + bytes_to_copy);
		// bytes_read += bytes_to_copy;
		// bytes_left -= bytes_to_copy;
		// bytes_to_copy = 0;

		// Case: reading 
		if (FDT[fd].offset % BLOCK_SIZE > 0) {
			//Current position of the datablock
			int cur_position_datablock = 1;
			for (int i = 1; FDT[fd].offset > (i * BLOCK_SIZE); i++){
				cur_position_datablock++;
			}
			
			//Checks if there are multiple blocks to read
			//int	traverse_datablocks = ((FDT[fd].offset - (cur_position_datablock * BLOCK_SIZE)) + count) / (BLOCK_SIZE + 1);
			
			//Keep track of the data block idx
			int data_block_idx = FDT[fd].data_start_idx;
			int obtained_bytes = 0;
			//To store the bytes that are acutally read to the buf			
			char bounce_copy[count];
			for (int i =0; i < count; i ++){

				//Checks if the offset reachs the end of the datablock
				if (FDT[fd].offset > (cur_position_datablock * BLOCK_SIZE)){
					//Check if there is more datablocks in the file
					//If not then return the read bytes
					if(FAT[data_block_idx] == FAT_EOC){
						memcpy(buf, bounce_copy, sizeof(bounce_copy));
						return i;
					}
					else{
						block_read(get_data_block_idx(fd), bounce);
						//traverse_datablocks--;
						cur_position_datablock++;
						data_block_idx = FAT[data_block_idx];
					}
				}
			bounce_copy[i] = bounce[(FDT[fd].offset)];
			fs_lseek(fd,(FDT[fd].offset + 1));
			obtained_bytes++;
			}

			memcpy(buf, bounce_copy, sizeof(bounce_copy));
			return obtained_bytes;
		}
	}

	//For all cases:
	//buf needs to be the one to have all the data
	//FIXME: Updating the offset to the number of byets that were acutally read
	FDT[fd].offset = FDT[fd].offset + bytes_read;




	//After the function is done reading
	//Need to set the buffer to the next available data pointer??
	


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
//}


/**
 * Helper function: Gets the index of the data block corresponding to the file's offset
 * @fd: File descriptor
 * 
 * Return: index of the data block corresponding to the file's offset
 *
static int get_data_block_idx(int fd)
{
	// File size is 0 - does not occupy any data blocks
	if (FDT[fd].data_start_idx == FAT_EOC) {
		return -1;
	}
	
	// Find the size of the file
	// int size;
	// for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
	// 	if (!strcmp(FDT[fd].filename, root_dir[i].filename)) {
	// 		size = root_dir[i].size;
	// 		break;
	// 	}
	// }

	// Find the number of data blocks that the file spans
	// (size / BLOCK_SIZE): number of fully occupied data blocks
	// (size % BLOCK_SIZE > 0 ? 1 : 0): if a blocks is partially occupied, add 1 - otherwise add 0
	// This accounts both cases where the file occupies exactly a multiple of a block (size % BLOCK_SIZE = 0)
	// and when a file occupies part of a block (size % BLOCK_SIZE > 0)
	// int num_blocks = (size / BLOCK_SIZE) + (size % BLOCK_SIZE > 0 ? 1 : 0);

	// Start at the first data block index
	int idx = FDT[fd].data_start_idx;
	int i = 1;  // block size multiplier
	// While the offset is larger than some multiple of BLOCK_SIZE, keep going through the FAT
	// entries to find the index of the next data block
	// When the offset is <= some multiple of BLOCK_SIZE, then we have found the data block
	// corresponding to offset
	while (FDT[fd].offset / (BLOCK_SIZE * i) > 0) {
		idx = FAT[idx];
		i++;
	}

	return idx;
}

//Helper Function for fs_write() to extend file size
// int extend_file_size(int fd, int bytes_left, int cur_offset)
// {
	int required_datablocks = (((cur_offset % BLOCK_SIZE) + bytes_left) / BLOCK_SIZE);
	
	//If there is a need to allocate a new datablock
	if (requried_datablocks != 0){
// 	int ptr;
// 	//get the current EOC
// 	ptr = FDT[fd].data_start_idx;
//	//Check if the file size is not zero
//	if (ptr != FAT_EOC){
// 		while (FAT[ptr] != FAT_EOC){
// 			ptr = FAT[ptr];
// 		}

// 	//Travse the FAT array to find space to allocate new blocks of
// 	//datablocks
// 	int out_of_datablocks = -1;
//	int total_datablocks_ = required_datablocks;
// 	for (int i = 0; i <(sb.FAT_blocks * BLOCK_SIZE / 2); i++){		//Fixme: Need the size of the FAT array
// 		if (FAT[i] == 0){
//			//If the file size is zero
//			if (ptr == FAT_EOC){
//			FDT[fd].data_start_idx = i;
//			ptr = i;
//			//It does run the other code which is unnessary for this case
//			// but its better to leave it as it is as does this once rather than using
//			//if or else statements at least a thousand times
//			}
//			out_of_datablocks = i;
// 			FAT[ptr] = i;
// 			FAT[i] = FAT_EOC;
// 			ptr = i;
// 			required_datablocks--;
// 		}
//		if (required_datablocks == 0){
//		break;
//		}
//
// 	}
//	if (total_datablocks == requried_datablocks && out_of_datablocks == -1)
//	printf("No free datablocks available");
//	return -1;
// 	}

// 	//Find the size of the file
// 	int size;
// 	int root_dir_file;
// 	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
// 		if (!strcmp(FDT[fd].filename, root_dir[i].filename)) {
// 			size = root_dir[i].size;
// 			root_dir_file = i;
// 			break;
// 		}
// 	}
//
//	//Calculate the amount of bytes acutally allocated to the file size
//	int bytes_given = bytes_left - (requried_datablocks * BLOCK_SIZE);
//
// 	//Increase current file size
// 	size = size + bytes_given;
// 	root_dir[i].size = size;
// }
*/
