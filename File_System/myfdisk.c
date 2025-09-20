/**********************************************************
 * File: myfdisk.c
 * Author: Mohamed Khaled Ahmed
 * Date: 2025-09-16
 * Description: This is the source file for the myfdisk utility
 **********************************************************/

#include "myfdisk.h"
#include <stdio.h>



/***************** Main Function ******************/


int main(int argc , char **argv){
    if(argc != 2){
        printf("Usage: %s <device>\n" , argv[0]);
        return EXIT_FAILURE;
    }

    char *device = argv[1];

    read_partition_table(device);

    return EXIT_SUCCESS;
}


void read_partition_table(char *device){

    int fd = open(device , O_RDONLY); // Open the device in read only mode

    // Check if the device is opened successfully
    if(fd == -1){
        printf("Error: Failed to open the device\n");
        exit(EXIT_FAILURE);
    }

    
    char buffer[SECTOR_SIZE];

    if(read(fd , buffer , SECTOR_SIZE) != SECTOR_SIZE){
        printf("Error: Failed to read the partition table\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Cast the buffer to a PartitionEntry pointer
    PartitionEntry *table_entry_ptr = ((PartitionEntry *)&buffer[446]);

     /**< Print the header for the partition table information with bold text */ 
    printf("\033[1m%-10s %5s   %-10s %-10s %-10s %-10s %-5s %-5s\033[0m\n", "Device",
           "Boot", "Start", "End", "Sectors", "Size", "Id", "Type");

    /**< Print the partition table information */
    for(int i = 0; i < 4; i++){
        process_partition_table(device , i , &table_entry_ptr[i]);
        if(table_entry_ptr[i].type == 0x05 && table_entry_ptr[i].sector_count > 0){
            read_ebr(device , table_entry_ptr[i].lba , table_entry_ptr[i].lba , 0);
        }
    }
    

    close(fd);
}

void process_partition_table(char *device , uint8_t partition_number , PartitionEntry *table_entry_ptr){
    /**< Print the details of each partition entry */
    printf("%-8s%-4d %-4c %-10u %-10u %-10u %06.2fG %5X\n",
        device,         /**< Device name */
        partition_number + 1, /**< Partition number */
        table_entry_ptr->status == 0x80 ? '*' : ' ' , /**< Status */
        table_entry_ptr->lba ,  /**< Logical Block Address */
        table_entry_ptr->lba + table_entry_ptr->sector_count - 1 , /**< End Logical Block Address */
        table_entry_ptr->sector_count , /**< Sector count */
        (double)table_entry_ptr->sector_count * SECTOR_SIZE / (1024 * 1024 * 1024) , /**< Size */
        table_entry_ptr->type /**< Partition type */
    );
}



void read_ebr(char *device , uint32_t current_ebr_sector , uint32_t extended_partition_start , int logical_num){

    char buffer[SECTOR_SIZE];

    // Open the device in read only mode
    uint32_t fd = open(device , O_RDONLY);

    // Check if the device is opened successfully
    if(fd == -1){
        printf("Error: Failed to open the device\n");
        exit(EXIT_FAILURE);
    }
    
    
    off64_t offset = current_ebr_sector * SECTOR_SIZE;

    off64_t curr_offset = lseek64(fd , offset , SEEK_SET);

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
    table_entry_ptr[0].lba = current_ebr_sector + table_entry_ptr[0].lba;

    /**< Print the details of the extended partition */
    process_partition_table(device , logical_num , &table_entry_ptr[0]);

    close(fd);

    if(table_entry_ptr[1].sector_count > 0){
        int next_ebr_sector = extended_partition_start + table_entry_ptr[1].lba;
        read_ebr(device , next_ebr_sector , extended_partition_start , logical_num + 1);
    }
}



