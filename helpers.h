#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <stdint.h>
#include <stdlib.h>

/*
Structure definitions.
*/

/*
A partition entry in the MBR is 16 contiguous bytes.
*/
typedef struct {
    uint8_t  boot_flag;       // 0x80 = bootable, 0x00 = non-bootable
    uint8_t  chs_start[3];    // CHS address (Cylinder-Head-Sector)
    uint8_t  system_id;       // FS type (0x0B = FAT32 CHS, 0x0C = FAT32 LBA)
    uint8_t  chs_end[3];      // CHS address
    uint32_t lba_start;       // Starting LBA (Logical Block Addressing)
    uint32_t sector_count;

    // compiler directive to ensure struct element are tightly packed.
    // Required for a byte per byte representation of the struct.
} __attribute__((packed)) PartitionEntry; 

/*
This structure gathers all the required informations from 
the statement. It doesn't take all the fields in a FAT BPB.
*/
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