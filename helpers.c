#ifndef __HELPERS_C__
#define __HELPERS_C__

#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"

/*
This function prints a PartitionEntry to the terminal, showing
all its fields, as well as computing the number of sectors.

(field) indicates 'field' has been computed and is not present
in the PartitionEntry 
*/
void printPartitionEntry(PartitionEntry *entry) {
    uint32_t start_sector = entry->lba_start;
    uint32_t sector_count = entry->sector_count;
    uint32_t end_sector = start_sector + sector_count - 1;

    printf("  Boot Flag     : 0x%02X\n", entry->boot_flag);
    printf("  CHS Start     : %02X %02X %02X\n", entry->chs_start[0], entry->chs_start[1], entry->chs_start[2]);
    printf("  System ID     : 0x%02X\n", entry->system_id);
    printf("  CHS End       : %02X %02X %02X\n", entry->chs_end[0], entry->chs_end[1], entry->chs_end[2]);
    printf("  Start LBA     : 0x%08X (%u)\n", start_sector, start_sector);
    printf("  End LBA       : 0x%08X (%u)\n", end_sector, end_sector);
    printf("  (Sector Count): 0x%08X (%u)\n\n", sector_count, sector_count);
}

/*
This function frees an array of PartitionEntry of size partition_count.
*/
int freePartitions(PartitionEntry *partitions[], size_t partition_count) {
    for (size_t i = 0; i < partition_count; i++) {
        free(partitions[i]);
    }
    return 0;
}

void printFAT32Info(FAT32Info *info) {
    printf("FAT32 Volume Information:\n");
    printf("  Sectors per cluster  : 0x%02X (%u)\n", info->sectors_per_cluster, info->sectors_per_cluster);
    printf("  Bytes per sector     : 0x%04X (%u)\n", info->bytes_per_sector, info->bytes_per_sector);
    printf("  Number of FATs       : 0x%02X (%u)\n", info->num_fats, info->num_fats);
    printf("  Reserved sectors     : 0x%08X (%u)\n", info->reserved_sectors, info->reserved_sectors);
    printf("  FAT size (sectors)   : 0x%08X (%u)\n", info->fat_size_sectors, info->fat_size_sectors);
    printf("  Root cluster         : 0x%08X (%u)\n", info->root_cluster, info->root_cluster);
}

int freeFAT32Info(FAT32Info *info[], size_t partition_count) {
    for (int i = 0; i < partition_count; i++) {
        free(info[i]);
    }
    return 0;
}

#endif