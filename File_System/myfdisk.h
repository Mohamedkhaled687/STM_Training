/**********************************************************
 * File: myfdisk.h
 * Author: Mohamed Khaled Ahmed
 * Date: 2025-09-16
 * Description: This is the header file for the myfdisk utility
 **********************************************************/

#ifndef MYFDISK_H
#define MYFDISK_H

#define _LARGEFILE64_SOURCE

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
    uint8_t sector_count;       /**< Sector count */
} PartitionEntry;   


/***************** Functions Prototypes ******************/

/**
 * @brief Read the partition table from the device
 * @param device The device to read the partition table from
 */
void read_partition_table(char *device);


/**
 * @brief Process the partition table
 * @param device The name of the device 
 * @param partition_number The partition number
 * @param partition_entry Pointer to the partition entry
 */
void process_partition_table(char *device , uint8_t partition_number , PartitionEntry *partition_entry);


/**
 * @brief Read the EBR from the device
 * @param device The name of the device
 * @param current_ebr_sector The sector address we need right now 
 * @param extended_partition_start The starting address of the extended partition
 * @param logical_num A counter to keep track of partition number 
 */

void read_ebr(char *device , uint32_t current_ebr_sector , uint32_t extended_partition_start , int logical_num);

#endif // MYFDISK_H