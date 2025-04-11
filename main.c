#ifndef __MAIN_C__
#define __MAIN_C__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "helpers.h"

#define MBR_SIZE 512
#define PARTITION_COUNT 4
#define PARTITION_ENTRY_SIZE 16
#define PARTITION_ENTRY_OFFSET 446

#define SECTOR_SIZE 512





/*
This function takes as input a filename of a disk image and parses the MBR to extract every partition information.
*/
void parse_mbr(const char *img_filename, PartitionEntry *partitions[]) {
    FILE *fp = fopen(img_filename, "rb");
    if (!fp) {
        fprintf(stderr, "parse_mbr: Failed to open image file\n");
        exit(EXIT_FAILURE);
    }

    // Create a MBR buffer and read from file
    uint8_t mbr[MBR_SIZE];
    if (fread(mbr, 1, MBR_SIZE, fp) != MBR_SIZE) {
        perror("Failed to read MBR");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    /* 
    For each partition, read from the buffer and create a dedicated PartitionEntry struct.
    Parse every fiel of the struct and print them.
    */
    fprintf(stdout, "Parsing MBR partition table of \"%s\":\n", img_filename);
    for (int i = 0; i < PARTITION_COUNT; i++) {
        PartitionEntry entry;
        int offset = PARTITION_ENTRY_OFFSET + i * PARTITION_ENTRY_SIZE;
        memcpy(&entry, mbr + offset, sizeof(PartitionEntry));
        
        printPartitionEntry(&entry);
    }

    fclose(fp);
}


int parse_fat32_info(char *filename, const PartitionEntry *partition, FAT32Info *info_out) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening image file");
        return -1;
    }

    printf("Parsing FAT32 info...\n");

    // Seek to the partition's first sector (boot sector)
    if (fseek(fp, partition->lba_start * SECTOR_SIZE, SEEK_SET) != 0) {
        perror("Failed to seek to partition's boot sector");
        fclose(fp);
        return -1;
    }

    uint8_t sector[SECTOR_SIZE];
    if (fread(sector, 1, SECTOR_SIZE, fp) != SECTOR_SIZE) {
        perror("Failed to read boot sector");
        fclose(fp);
        return -1;
    }

    // Check System ID (from partition table)
    if (partition->system_id != 0x0B && partition->system_id != 0x0C) {
        fprintf(stderr, "Partition is not FAT32 (System ID: 0x%02X)\n", partition->system_id);
        fclose(fp);
        return -1;
    }

    // Optional: check for 0x55AA signature at end of sector
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        fprintf(stderr, "Invalid boot sector signature (expected 0x55AA)\n");
        fclose(fp);
        return -1;
    }

    // Extract and populate FAT32Info
    info_out->bytes_per_sector     = sector[11] | (sector[12] << 8);
    info_out->sectors_per_cluster  = sector[13];
    info_out->reserved_sectors     = sector[14] | (sector[15] << 8);
    info_out->num_fats             = sector[16];
    info_out->fat_size_sectors     = sector[36] | (sector[37] << 8) |
                                     (sector[38] << 16) | (sector[39] << 24);
    info_out->root_cluster         = sector[44] | (sector[45] << 8) |
                                     (sector[46] << 16) | (sector[47] << 24);

    // Basic sanity checks
    if (info_out->bytes_per_sector == 0 ||
        info_out->sectors_per_cluster == 0 ||
        info_out->fat_size_sectors == 0 ||
        info_out->root_cluster < 2) {
        fprintf(stderr, "Invalid FAT32 boot sector fields.\n");
        fclose(fp);
        return -1;
    }

    // Print the extracted info
    printf("FAT32 Volume Information:\n");
    printf("  Bytes per sector     : %u\n", info_out->bytes_per_sector);
    printf("  Sectors per cluster  : %u\n", info_out->sectors_per_cluster);
    printf("  Reserved sectors     : %u\n", info_out->reserved_sectors);
    printf("  Number of FATs       : %u\n", info_out->num_fats);
    printf("  FAT size (sectors)   : %u\n", info_out->fat_size_sectors);
    printf("  Root cluster         : %u\n", info_out->root_cluster);

    fclose(fp);
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <disk_image_file> <mode>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    char *mode = argv[2];

    // Creating array of PartitionEntry pointers
    PartitionEntry *partitions[PARTITION_COUNT];
    for (int i = 0; i < PARTITION_COUNT; i++) {
        partitions[i] = malloc(sizeof(PartitionEntry));
        if (!partitions[i]) {
            fprintf(stderr, "main: Failed to allocate memory for partition entry\n");
            return EXIT_FAILURE;
        }
    }

    // Parsing arguments
    if (strcmp(mode, "--mbr") == 0) {
        parse_mbr(filename, partitions);

    } else if (strcmp(mode, "--fat") == 0) {
        for (int i = 0; i < PARTITION_COUNT; i++) {
            fprintf(stdout, "Parsing partition number %d\n", i + 1);  
            FAT32Info *fat32_info = malloc(sizeof(FAT32Info));
            parse_fat32_info(filename, partitions[i], fat32_info);
            
            free(fat32_info);
        }
    } else {
        fprintf(stderr, "This mode doesn't exist. Use '--mbr' or '--fat'.\n");

        freePartitions(partitions, PARTITION_COUNT);
        exit(EXIT_FAILURE);
    }

    freePartitions(partitions, PARTITION_COUNT);
    return 0;
}

#endif