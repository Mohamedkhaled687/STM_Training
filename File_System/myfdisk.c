/**********************************************************
 * File: myfdisk.c
 * Author: Mohamed Khaled Ahmed
 * Date: 2025-09-16
 * Description: This is the source file for the myfdisk utility
 **********************************************************/

#include "myfdisk.h"



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
    PartitionEntry *partition_entry = ((PartitionEntry *)&buffer[446]);

     /**< Print the header for the partition table information with bold text */ 
    printf("\033[1m%-10s %5s   %-10s %-10s %-10s %-10s %-5s %-5s\033[0m\n", "Device",
           "Boot", "Start", "End", "Sectors", "Size", "Id", "Type");

    /**< Print the partition table information */
    for(int i = 0; i < 4; i++){
        process_partition_table(device , i , &partition_entry[i]);
        if(partition_entry[i].type == 0x05 && partition_entry[i].sector_count > 0){
            read_ebr(device , partition_entry[i].lba , partition_entry[i].lba , 1);
        }
    }
    

    close(fd);
}

void process_partition_table(char *device , uint8_t partition_number , PartitionEntry *partition_entry){
    /**< Print the details of each partition entry */
    printf("%-8s%-4d %-4c %-10u %-10u %-10u %06.2fG %5X\n",
        device,         /**< Device name */
        partition_number + 1, /**< Partition number */
        partition_entry->status == 0x80 ? '*' : ' ' , /**< Status */
        partition_entry->lba ,  /**< Logical Block Address */
        partition_entry->lba + partition_entry->sector_count - 1 , /**< End Logical Block Address */
        partition_entry->sector_count , /**< Sector count */
        (double)partition_entry->sector_count * SECTOR_SIZE / (1024 * 1024 * 1024) , /**< Size */
        partition_entry->type /**< Partition type */
    );
}



void read_ebr(char *device , uint32_t lba , uint32_t original_ebr_lba , int ebr_number){
    int fd = open(device , O_RDONLY);
    if(fd == -1){
        printf("Error: Failed to open the device\n");
        exit(EXIT_FAILURE);
    }
}

