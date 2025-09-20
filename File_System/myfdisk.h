/**********************************************************
 * File: myfdisk.h
 * Author: Mohamed Khaled Ahmed
 * Date: 2025-09-16
 * Description: This is the header file for the myfdisk utility
 **********************************************************/

#ifndef MYFDISK_H
#define MYFDISK_H

/***************** Include files ******************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/***************** Definitions ******************/

#define SECTOR_SIZE 512

/***************** Structures ******************/


typedef struct {
    uint8_t status;             /**< Status of the partition (e.g., active or inactive) */
    uint8_t first_CHS[3];       /**< First CHS address */
    uint8_t type;               /**< Type of the partition (e.g., FAT32, NTFS, etc.) */
    uint8_t last_CHS[3];        /**< Last CHS address */
    uint32_t lba;               /**< Logical Block Address */
    uint32_t sector_count;      /**< Sector count */
} PartitionEntry;   

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32_header;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t partition_entries_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t crc32_partition_array;
} GptHeader; // Gpt header act as the table of contents for the entire disk it's main purpose to tell the os everything it needs to find and use the partitions

typedef struct {
    uint8_t partition_type_guid[16];
    uint8_t unique_partition_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t partition_name[36]; // UTF-16LE
} GptPartitionEntry;

/***************** Functions Prototypes ******************/

/**
 * @brief Read the partition table from the device
 * @param device The device to read the partition table from
 */
void read_partition_table(char *device);

/**
 * @brief Get the human-readable name for a partition type
 * @param type The partition type code
 * @return String representation of the partition type
 */
char* get_partition_type_name(uint8_t type);


/**
 * @brief Process the partition table
 * @param device The name of the device 
 * @param partition_number The partition number
 * @param partition_entry Pointer to the partition entry
 */     
void process_partition_table(char *device , uint8_t partition_number , PartitionEntry *table_entry_ptr);


/**
 * @brief Read the MBR partition table from the device
 * @param device The name of the device
 * @param table_entry_ptr Pointer to the partition entry
 */

 void read_mbr_partition_table(char *device , PartitionEntry *table_entry_ptr);


 /**
 * @brief Read the EBR from the device
 * @param device The name of the device
 * @param current_ebr_sector The sector address we need right now 
 * @param extended_partition_start The starting address of the extended partition
 * @param logical_num A counter to keep track of partition number 
 */

void read_ebr_partition_table(char *device , uint32_t current_ebr_sector , uint32_t extended_partition_start , int logical_num);

/**
 * @brief Process GPT partition entry
 * @param device The device name
 * @param partition_number The partition number
 * @param gpt_entry Pointer to the GPT partition entry
 */
 void process_gpt_partition(char *device, uint8_t partition_number, GptPartitionEntry *gpt_entry);

/**
 * @brief Read the GPT header and partition entries from the device
 * @param device The name of the device
 */
void read_gpt_partition_table(char *device);

#endif // MYFDISK_H