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


/** Helper functions **/

/**
  * Finds the minimum of two integers
  * @param a: first integer
  * @param b: second integer
  * 
  * Return: minimum of a and b
  */
static int min(int a, int b) 
{
    return a < b ? a : b;
}

/**
  * Finds the maximum of two integers
  * @param a: first integer
  * @param b: second integer
  * 
  * Return: maximum of a and b
  */
static int max(int a, int b)
{
    return a > b ? a : b;
}

/**
  * Finds the index of the first free entry in the FAT
  *
  * Return: -1 if no entries available, otherwise returns the index of the first free entry of the FAT
  */
// Return: 
int get_free_FAT_idx()
{
	// Loop through FAT to find the first entry that is == 0
    for (int i = 0; i < sb.total_data_blks; i++) {
        if (FAT[i] == 0) {
            return i;
        }
    }

    // No free entries
    return -1;
}


int fs_mount(const char *diskname)
{	
	// uint64_t expected_signature = 0x4543533135304653;  // "ECS150FS"
	
	// Error checking: Virtual file disk @diskname cannot be opened
	if (block_disk_open(diskname) == -1) {
		// perror("Block disk open");
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
		// perror("Wrong signature");
		return -1;
	}

	// printf("Superblock signature matches\n");

    // Error checking: total number of blocks is same as block_disk_count() of total
	int total_number_of_block = 1 + (int)(sb.FAT_blocks)+ 1 + (int)(sb.total_data_blks);
	if (total_number_of_block != block_disk_count()) {
		// // perror("Number of blocks do not match\n");
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
{	// Error checking: no FS mounted or still have open file descriptors
 	if (block_disk_count() == -1 || total_file_open != 0){
		// printf("There are still open file descriptors\n");
		return -1;
	}

    // Write root directory & FAT back to the disk
    if (block_write(sb.root_dir_idx, root_dir) == -1) {
        // perror("Block write");
        // printf("Error when writing to root dir at unmount\n");
        return -1;
    }

    for (int i = 0; i < sb.FAT_blocks; i++) {
        if (block_write(i + 1, FAT + (BLOCK_SIZE / 2) * i) == -1) {
            // perror("Block write");
            return -1;
        }
    }

    // Close disk
	if (block_disk_close() == -1){		
		return -1;
	} 

	// free(FAT);
	return 0;
}

int fs_info(void)
{
	// Error checking: no underlying virtual disk was opened
	if (block_disk_count() == -1) {
		return -1;
	}

    // For FAT ratio by Professor
    // for loop to loop all the entries of the FAT table
	int fat_free = 0;
	int total_fat_entries = sb.total_data_blks;

	for (int i = 0; i < total_fat_entries; i++){
		if (FAT[i] == 0){
			fat_free++;
		}
	}
    
	//Show info about volume

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", block_disk_count());
	printf("fat_blk_count=%d\n", sb.FAT_blocks);
	printf("rdir_blk=%d\n", sb.root_dir_idx);
	printf("data_blk=%d\n", sb.data_blks_idx);
	printf("data_blk_count=%d\n", sb.total_data_blks);
	printf("fat_free_ratio=%d/%d\n", fat_free, total_fat_entries);
	printf("rdir_free_ratio=%d/%d\n", rdir_free_entries, FS_FILE_MAX_COUNT);
	
	return 0;
}

int fs_create(const char *filename)
{
	// Error checking: no FS mounted or @filename is invalid or string @filename is too long
	//                 or root directory already contains %FS_FILE_MAX_COUNT files (i.e. no free entries)
	if (block_disk_count() == -1 || filename == NULL || (strlen(filename) + 1) > FS_FILENAME_LEN || rdir_free_entries == 0) {
		return -1;
	}

	// Error checking: filename already exists in root directory
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp((char*)root_dir[i].filename, filename)) {
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
	memcpy(root_dir[first_avail_idx].filename, filename, FS_FILENAME_LEN);  
	// root_dir[first_avail_idx].filename=(uint8_t)(filename);
	root_dir[first_avail_idx].size = 0;
	root_dir[first_avail_idx].data_start_idx = FAT_EOC;

	// Decrement number of free entries in root directory
	rdir_free_entries--;

	return 0;
 }

int fs_delete(const char *filename)
{
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
	
	// Error checking: @filename is currently open
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if (!strcmp((char*)FDT[i].filename, filename)) {
			// @filename exists in FDT as an open file
			return -1;
		}
	}
	

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
}

int fs_open(const char *filename)
{
	// Error checking: no FS mounted or @filename is invalid 
	// or there are already %FS_OPEN_MAX_COUNT files currently open
	if (block_disk_count() == -1 || filename == NULL || total_file_open >= FS_OPEN_MAX_COUNT) {
        // printf("FS open = %d\n", block_disk_count());
        // printf("Filename = %s\n", filename);
        // printf("Total files open = %d\n", total_file_open);
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
}

int fs_close(int fd)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open)
	if (block_disk_count() == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0') {
		return -1;
	}

    // Reset fields for the fd entry in FDT
	FDT[fd].filename[0] = '\0';
	FDT[fd].data_start_idx = 0;
	FDT[fd].offset = 0;

    // Closed the fd
	total_file_open--;

	return 0;
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
}


int fs_write(int fd, void *buf, size_t count)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open) 
    //                 or @buf is NULL
	if (block_disk_count() == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || buf == NULL) {
		return -1;
	}

    // Grab offset into local var - for readability
    int offset = FDT[fd].offset;

    // Find index of current data block corresponding to offset
	int idx = FDT[fd].data_start_idx;  // Index of FAT entry corresponding to offset
    int data_idx;  // Index of data block corresponding to offset
    int prev_idx = FAT_EOC;  // One FAT idx "back" from offset idx (corresponding to fd) - used for updating FAT

    // If idx == FAT_EOC, then file size is 0 - does not occupy any data blocks
	if (idx == FAT_EOC) {
		idx = get_free_FAT_idx();  // get first free FAT index
		if (idx == -1) {
			// File size is 0 and there is no space left in the FAT - no bytes can be written
			return 0;
		}
	} else {  // file size > 0 - find idx corresponding to this offset
        int i = 1;
        while (FDT[fd].offset / (BLOCK_SIZE * i) > 0) {
            prev_idx = idx;
            idx = FAT[idx];
            i++;
        }
    }
    // Add number of data blocks (data blocks offset) to idx to get data_idx (from superblock & FAT blocks before start of data blocks)
    data_idx = idx + sb.data_blks_idx;

    char bounce[BLOCK_SIZE];  // Bounce buffer to read from/write to disk
    int bytes_left = count;  // Number of remaining bytes to be written
    int bytes_to_copy = 0;  // Number of bytes to copy from buf into bounce
    int bytes_written = 0;  // Number of bytes written already
    
    // Calculate number of blocks needed to write all of buf into disk
    int blocks_to_write = (offset + count) / BLOCK_SIZE + ((offset + count) % BLOCK_SIZE != 0);

    // Find fd's entry in root_dir to grab its size and index
    int size = 0;
    int root_dir_idx = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (!strcmp((char*)FDT[fd].filename, (char*)root_dir[i].filename)) {
            size = root_dir[i].size;
            root_dir_idx = i;
            break;
        }
    }


    // Write buf into disk
    for (int i = 0; i < blocks_to_write && (size_t) bytes_written < count; i++) {
        // Offset is at end of current file and needs another block allocated
        if (offset == size && offset % BLOCK_SIZE == 0) {
            // printf("Loop i = %d, offset = %d\n", i, offset);
            // Find next available FAT index (if not already found - i.e. first loop)
            if (idx == FAT_EOC) {
                idx = get_free_FAT_idx();  // get first free FAT index
                data_idx = idx + sb.data_blks_idx;
            }

            // Copy the next block (or remaining bytes to be written) of buf into bounce for writing into disk
            bytes_to_copy = min(BLOCK_SIZE, bytes_left);
            memcpy(bounce, buf + bytes_written, bytes_to_copy);

            // Write to disk
            block_write(data_idx, bounce);

            // Update FAT
            if (prev_idx != FAT_EOC) {
                FAT[prev_idx] = idx;
            }
            FAT[idx] = FAT_EOC;

            // File originally had no data - set its starting data block index
            if (size == 0) {
                root_dir[root_dir_idx].data_start_idx = idx;
                FDT[fd].data_start_idx = idx;
            }

            // Calculate new size
            size += bytes_to_copy;
        } else {  // Offset is somewhere in the middle of a block
            // printf("Loop i = %d, idx = %d, data_idx = %d\n", i, idx, data_idx);
            // Read block (including existing data) into bounce
            block_read(data_idx, bounce);

			// printf("Bounce looks like: %s\n", (char*)bounce);
			// printf("File size: %d\n", fs_stat(fd));
			// printf("offset = %d\n", offset);

            // Copy from buf into bounce
            bytes_to_copy = min(BLOCK_SIZE - offset % BLOCK_SIZE, bytes_left);  // min of remaining space in block and the remaining bytes to be written
            memcpy(bounce + offset, buf + bytes_written, bytes_to_copy);

            // Write to disk
            block_write(data_idx, bounce);

            // Calculate new size - we may have written over data but did not increase the size, or increased the size (whether data was written over or not)
            size = max(offset + bytes_to_copy, size);
            // printf("new size = %d\n", size);
        }
        
        // Update size and offset
        // fs_lseek(fd, offset + bytes_to_copy);
        offset += bytes_to_copy;
        FDT[fd].offset = offset;
        root_dir[root_dir_idx].size = size;

        // printf("offset = %d\n", offset);

        // Update bytes_left and bytes_written and reset bytes_to_copy
        bytes_left -= bytes_to_copy;
        bytes_written += bytes_to_copy;
        bytes_to_copy = 0;

        // Get next FAT and data block indices
        prev_idx = idx;
        idx = FAT[idx];
        data_idx = idx + sb.data_blks_idx;
    }

    return bytes_written;
}


/**
 * Helper function: Gets the index of the data block corresponding to the file's offset
 * @fd: File descriptor
 * 
 * Return: index of the data block corresponding to the file's offset
 */
// static int get_data_block_idx(int fd)
// {
// 	// File size is 0 - does not occupy any data blocks
// 	if (FDT[fd].data_start_idx == FAT_EOC) {
// 		return -1;
// 	}
	
// 	// Find the size of the file
// 	// int size;
// 	// for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
// 	// 	if (!strcmp(FDT[fd].filename, root_dir[i].filename)) {
// 	// 		size = root_dir[i].size;
// 	// 		break;
// 	// 	}
// 	// }

// 	// Find the number of data blocks that the file spans
// 	// (size / BLOCK_SIZE): number of fully occupied data blocks
// 	// (size % BLOCK_SIZE > 0 ? 1 : 0): if a blocks is partially occupied, add 1 - otherwise add 0
// 	// This accounts both cases where the file occupies exactly a multiple of a block (size % BLOCK_SIZE = 0)
// 	// and when a file occupies part of a block (size % BLOCK_SIZE > 0)
// 	// int num_blocks = (size / BLOCK_SIZE) + (size % BLOCK_SIZE > 0 ? 1 : 0);

// 	// Start at the first data block index
// 	int idx = FDT[fd].data_start_idx;
// 	int i = 1;  // block size multiplier
// 	// While the offset is larger than some multiple of BLOCK_SIZE, keep going through the FAT
// 	// entries to find the index of the next data block
// 	// When the offset is <= some multiple of BLOCK_SIZE, then we have found the data block
// 	// corresponding to offset
// 	while (FDT[fd].offset / (BLOCK_SIZE * i) > 0) {
// 		idx = FAT[idx];
//         // Reached end of file
//         if (idx == FAT_EOC) {
//             return -1;
//         }
// 		i++;
// 	}

// 	return idx + sb.data_blks_idx;
// }




int fs_read(int fd, void *buf, size_t count)
{
	// Error checking: no FS mounted or @fd is invalid (out of bounds or not currently open) or @buf is NULL
	if (block_disk_count() == -1 || fd < 0 || fd >= FS_OPEN_MAX_COUNT || FDT[fd].filename[0] == '\0' || buf == NULL) {
		return -1;
	}

    // Find index of current data block corresponding to offset
	int idx = FDT[fd].data_start_idx;
	if (idx == FAT_EOC) {
        // File size is 0 - does not occupy any data blocks, nothing to read
		return 0;
	}
	// Otherwise, find the index in FAT corresponding to offset
    int i = 1;
    while (FDT[fd].offset / (BLOCK_SIZE * i) > 0) {
        idx = FAT[idx];
        i++;
    }
    // Add offset to idx (from superblock & FAT blocks before start of data blocks)
    idx += sb.data_blks_idx;

    
	char bounce[BLOCK_SIZE];  // Bounce buffer to read entire blocks from disk
	int bytes_left = count;  // Numbr of bytes remaining to be read
	int bytes_to_copy = 0;  // Number of bytes to copy from bounce into buf
	int bytes_read = 0;    // Number of bytes read already
	int offset = FDT[fd].offset;  // Current offset for the fd (grabbing into another variable for more readability)

	// Calculate number of blocks to read (ceiling division)
	int blocks_to_read = (offset + count) / BLOCK_SIZE + ((offset + count) % BLOCK_SIZE != 0);
    
    // Find fd's entry in root_dir to grab its size
    int size = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (!strcmp((char*)FDT[fd].filename, (char*)root_dir[i].filename)) {
            size = root_dir[i].size;
            break;
        }
    }

	// Part of one or exactly one data block to be read
	if (blocks_to_read == 1) {
        // printf("idx = %d\n", idx);
		if (block_read(idx, bounce) == -1) {
            // perror("Block read");
            return -1;
        }
        bytes_to_copy = min(count, size);
        // printf("Bounce: %s\n", (char*)bounce);
        // printf("Offset = %d\n", offset);
		memcpy(buf, bounce + offset, bytes_to_copy);
        // printf("Buf: %s\n", (char*)buf);
        bytes_read = bytes_to_copy;
        offset += bytes_to_copy;
        FDT[fd].offset = offset;
	} else { // More than one data block to be read
		for (int i = 0; i < blocks_to_read && (size_t)bytes_read < count; i++) {
			if (block_read(idx, bounce) == -1) {
                // perror("Block read");
                return -1;
            }
			// First data block: offset possibly not aligned with beginning of block
			if (i == 0) {
				bytes_to_copy = min(BLOCK_SIZE - offset % BLOCK_SIZE, size - bytes_read);
                // printf("Offset = %d\n", offset);
                // printf("bytes_to_copy = %d\n", bytes_to_copy);
				memcpy(buf, bounce + offset % BLOCK_SIZE, bytes_to_copy);
			} else if (i == (blocks_to_read - 1)) {  // Last data block
				bytes_to_copy = min(bytes_left, size - bytes_read);
				memcpy(buf + bytes_read, bounce, bytes_to_copy);
			} else {  // Middle data blocks
				bytes_to_copy = min(BLOCK_SIZE, size - bytes_read);
				memcpy(buf + bytes_read, bounce, bytes_to_copy);
			}
			// Copy appropriate data into buffer
			// memcpy((buf + bytes_read), ((offset % BLOCK_SIZE) + bounce), bytes_to_copy);
			// Update bytes_read and bytes_left and reset bytes_to_copy

            // Move the offset after the read
            offset += bytes_to_copy;
            FDT[fd].offset = offset;

			bytes_read += bytes_to_copy;
			bytes_left -= bytes_to_copy;
			bytes_to_copy = 0;
			
			// Get the index of the next data block to be read
			idx = FAT[idx - sb.data_blks_idx];  
            if (idx == FAT_EOC) {
                break;
            }
            idx += sb.data_blks_idx;
		}
	}


    return bytes_read;
}




//Helper Function for fs_write() to extend file size
// int extend_file_size(int fd, int bytes_left, int cur_offset)
// {
	// int required_datablocks = (((cur_offset % BLOCK_SIZE) + bytes_left) / BLOCK_SIZE);
	
	//If there is a need to allocate a new datablock
	// if (requried_datablocks != 0){
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
//		
//
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
