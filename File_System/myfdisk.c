/**********************************************************
 * File: myfdisk.c
 * Author: Mohamed Khaled Ahmed
 * Date: 2025-09-16
 * Description: This is the source file for the myfdisk utility
 **********************************************************/

#include "myfdisk.h"
#include <stdint.h>
#include <stdio.h>



/***************** Main Function ******************/


int main(int argc , char **argv){

    /**< Check if the number of arguments is correct */
    if(argc != 2){
        printf("Usage: %s <device>\n" , argv[0]);
        return EXIT_FAILURE;
    }

    /**< Get the device name */
    char *device = argv[1];

    /**< Read the partition table */
    read_partition_table(device);

    return EXIT_SUCCESS;
}


void read_partition_table(char *device){

    char buffer[SECTOR_SIZE];

    int fd = open(device , O_RDONLY); // Open the device in read only mode

    // Check if the device is opened successfully
    if(fd == -1){
        printf("Error: Failed to open the device\n");
        exit(EXIT_FAILURE);
    }

    

    if(read(fd , buffer , SECTOR_SIZE) != SECTOR_SIZE){
        printf("Error: Failed to read the partition table\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Cast the buffer to a PartitionEntry pointer
    PartitionEntry *table_entry_ptr = (PartitionEntry *) & buffer[446];

    /**< Check if the partition table is MBR or GPT */
    if(table_entry_ptr[0].type == 0xEE){
        read_gpt_partition_table(device);
    }
    else{
        read_mbr_partition_table(device , table_entry_ptr);
    }

    /**< Close the device */
    close(fd);
}

char* get_partition_type_name(uint8_t type) {
    switch(type) {
        case 0x00: return "Empty";
        case 0x01: return "FAT12";
        case 0x04: return "FAT16";
        case 0x05: return "Extended";
        case 0x06: return "FAT16B";
        case 0x07: return "NTFS/exFAT";
        case 0x0B: return "FAT32";
        case 0x0C: return "FAT32";
        case 0x0E: return "FAT16";
        case 0x0F: return "Extended";
        case 0x82: return "Linux swap";
        case 0x83: return "Linux";
        case 0x85: return "Linux extended";
        case 0xEE: return "GPT";
        default: return "Unknown";
    }
}


void read_mbr_partition_table(char *device , PartitionEntry *table_entry_ptr){
    /**< Print the header for the partition table information with bold text */ 
    printf("\033[1m%-10s %5s   %-10s %-10s %-10s %-10s %-5s %-5s\033[0m\n", "Device",
           "Boot", "Start", "End", "Sectors", "Size", "Id", "Type");

    
        /**< Print the partition table information */
    for(int i = 0; i < 4; i++){
        if(table_entry_ptr[i].sector_count == 0 || table_entry_ptr[i].type == 0){
            continue;
        }
        process_partition_table(device , i , &table_entry_ptr[i]);
        if(table_entry_ptr[i].type == 0x05 && table_entry_ptr[i].sector_count > 0){
            read_ebr_partition_table(device , table_entry_ptr[i].lba , table_entry_ptr[i].lba , 0);
        }
    }

}




void read_ebr_partition_table(char *device , uint32_t current_ebr_lba , uint32_t extended_partition_start , int logical_num){

    char buffer[SECTOR_SIZE];

    // Open the device in read only mode
    uint32_t fd = open(device , O_RDONLY);

    // Check if the device is opened successfully
    if(fd == -1){
        printf("Error: Failed to open the device\n");
        exit(EXIT_FAILURE);
    }
    
    /**< Calculate the offset of the EBR sector */
    off_t offset = current_ebr_lba * SECTOR_SIZE;

    /**< Seek to the EBR sector */
    off_t curr_offset = lseek(fd , offset , SEEK_SET);

    if(curr_offset != offset){
        printf("Error: Failed to seek to the EBR sector\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(read(fd , buffer , SECTOR_SIZE) != SECTOR_SIZE){
        printf("Error: Failed to read the EBR sector\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /**< Cast the buffer to a PartitionEntry pointer */
    PartitionEntry *table_entry_ptr = ((PartitionEntry *)&buffer[446]);

    /**< Calculate the real LBA of the extended partition */
    table_entry_ptr[0].lba = current_ebr_lba + table_entry_ptr[0].lba;

    /**< Print the details of the extended partition */
    process_partition_table(device , logical_num , &table_entry_ptr[0]);

    close(fd);

    if(table_entry_ptr[1].sector_count > 0){
        int next_ebr_lba = extended_partition_start + table_entry_ptr[1].lba;
        read_ebr_partition_table(device , next_ebr_lba , extended_partition_start , logical_num + 1);
    }
}



void process_gpt_partition(char *device, uint8_t partition_number, GptPartitionEntry *gpt_entry) {
    /**< Print the details of each GPT partition entry */ 
    printf("%-12s   %-10llu %-10llu %-10llu %6.2f %-5s %-15s\n",
           device,                                                       /**< Device name */ 
           (unsigned long long)gpt_entry->starting_lba,                  /**< Start sector */ 
           (unsigned long long)gpt_entry->ending_lba,                   /**< End sector */ 
           (unsigned long long)(gpt_entry->ending_lba - gpt_entry->starting_lba + 1), /**< Number of sectors */ 
           (double)(gpt_entry->ending_lba - gpt_entry->starting_lba + 1) * SECTOR_SIZE / (1024 * 1024 * 1024), /**< Size in GB */
           "M",                                                        /**< Partition ID */
           "GPT Partition");                                             /**< Partition type name */
}



void read_gpt_partition_table(char *device){
    char buffer[SECTOR_SIZE];

    int fd = open(device , O_RDONLY);

    if(fd == -1){
        printf("Error: Failed to open the device\n");
        exit(EXIT_FAILURE);
    }

    // lseek to sector number 1
    off_t cur_offset = lseek(fd , 1 * SECTOR_SIZE , SEEK_SET);

    if(read(fd , buffer , SECTOR_SIZE) != SECTOR_SIZE){
        printf("Error: Failed to read the GPT header\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Cast the buffer to a GptHeader pointer
    GptHeader *gpt_header = (GptHeader *)buffer;

    
    // Check if the GPT header is valid
    /* The Signature is a "magic number" a specific sequence of 8 bytes the identifies the data 
    as valid GPT header*/
    if(gpt_header->signature != 0x5452415020494645){
        printf("Error: Invalid GPT header\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /**< Print the header for the GPT partition table information with bold text */ 
    printf("\033[1m%-12s   %-10s %-10s %-10s %-10s %-5s %-15s\033[0m\n", "Device",
           "Start", "End", "Sectors", "Size", "Id", "Type");

    // Calculate partition entries location and read them
    cur_offset = gpt_header->partition_entries_lba * SECTOR_SIZE;
    
    if(lseek(fd, cur_offset, SEEK_SET) != cur_offset){
        printf("Error: Failed to seek to partition entries\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Read partition entries (allocate buffer for entries)
    uint8_t partition_buffer[512 * 128]; // Buffer for partition entries
    uint32_t entries_size = gpt_header->num_partition_entries * gpt_header->partition_entry_size;
    
    if(read(fd, partition_buffer, entries_size) != entries_size){
        printf("Error: Failed to read partition entries\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Process each partition entry
    GptPartitionEntry *gpt_entries = (GptPartitionEntry *)partition_buffer;
    
    for(uint32_t i = 0; i < gpt_header->num_partition_entries; i++){
        // Check if partition is valid (has a starting LBA)
        if(gpt_entries[i].starting_lba != 0){
            process_gpt_partition(device, i, &gpt_entries[i]);
        }
    }

    close(fd);

}




void process_partition_table(char *device, uint8_t partition_number, PartitionEntry *table_entry_ptr) {
    /**< Print the details of each partition entry */ 
    printf("%-8s%-4d  %-4c %-10u %-10u %-10u %6.2f %5X %10s\n",
           device,                                                       /**< Device name */ 
           partition_number + 1,                                         /**< Partition number */ 
           table_entry_ptr->status == 0x80 ? '*' : ' ',                  /**< Boot flag */ 
           table_entry_ptr->lba,                                         /**< Start sector */ 
           table_entry_ptr->lba + table_entry_ptr->sector_count - 1,     /**< End sector */ 
           table_entry_ptr->sector_count,                             /**< Number of sectors */ 
           (double)table_entry_ptr->sector_count * SECTOR_SIZE / (1024 * 1024 * 1024), /**< Size in GB */
           table_entry_ptr->type,                                        /**< Partition ID */
           get_partition_type_name(table_entry_ptr->type));              /**< Partition type name */
}

