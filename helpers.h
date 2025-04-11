#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <stdint.h>
#include <stdlib.h>

/*
Structure definitions.
*/

typedef struct {
    uint8_t  boot_flag;       // 0x80 = bootable
    uint8_t  chs_start[3];    // CHS address (not reliable)
    uint8_t  system_id;       // Filesystem type (e.g., 0x0B for FAT32 CHS, 0x0C for FAT32 LBA)
    uint8_t  chs_end[3];      // CHS address
    uint32_t lba_start;       // Starting LBA (sector)
    uint32_t sector_count;    // Number of sectors
} __attribute__((packed)) PartitionEntry;  // compiler directive to ensure struct element are tightly packed

typedef struct {
    uint8_t  sectors_per_cluster;
    uint16_t bytes_per_sector;
    uint8_t  num_fats;
    uint16_t reserved_sectors;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;
} FAT32Info;


/*
Prototypes declaration.
*/
void printPartitionEntry(PartitionEntry *entry);

int freePartitions(PartitionEntry *partitions[], size_t partition_count);


#endif