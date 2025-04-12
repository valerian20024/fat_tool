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
    fprintf(stdout, "Parsing MBR partition table of \"%s\":\n", img_filename);

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
    For each partition, read from the buffer and write to the provided
    array of partitions.
    */
    for (int i = 0; i < PARTITION_COUNT; i++) {
        int offset = PARTITION_ENTRY_OFFSET + i * PARTITION_ENTRY_SIZE;
        memcpy(partitions[i], mbr + offset, sizeof(PartitionEntry));
    }

    fclose(fp);
}


int parse_fat32_info(char *filename, const PartitionEntry *partition, 
    FAT32Info *info_out) 
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening image file");
        return -1;
    }

    // Check System ID (from partition table)
    if (partition->system_id != 0x0B && partition->system_id != 0x0C) {
        fprintf(stderr, 
            "Partition is not FAT32 (System ID: 0x%02X)\n", 
            partition->system_id);
        fclose(fp);
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

    // Optional: check for 0x55AA signature at end of sector
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        fprintf(stderr, "Invalid boot sector signature (expected 0x55AA)\n");
        fclose(fp);
        return -1;
    }

    // Extract and populate FAT32Info. Little-endian so
    // we need to shift bits.
    info_out->bytes_per_sector = 
        sector[11] | (sector[12] << 8);

    info_out->sectors_per_cluster = 
        sector[13];

    // For BPB_Reserved
    //memcpy(info_out->reserved_sectors, &sector[52], sizeof(info_out->reserved_sectors));
    
    info_out->reserved_sector_count = 
        sector[14] | (sector[15] << 8);

    info_out->num_fats = 
        sector[16];

    info_out->fat_size_sectors = 
         sector[36]        | (sector[37] << 8) | 
        (sector[38] << 16) | (sector[39] << 24);

    info_out->root_cluster = 
         sector[44]        | (sector[45] << 8) | 
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

    fclose(fp);
    return 0;
}

int traverse_directory(const char *filename, const PartitionEntry *partition, const FAT32Info *info, uint32_t cluster, int depth) {
    
    FILE *fp = fopen(filename, "rb");

    // Calculate cluster size
    uint32_t cluster_size = info->bytes_per_sector * info->sectors_per_cluster;
    uint8_t *cluster_data = malloc(cluster_size);
    if (!cluster_data) {
        fprintf(stderr, "Failed to allocate memory for cluster\n");
        fclose(fp);
        return -1;
    }

    // Calculate data region start
    uint32_t data_start = partition->lba_start + info->reserved_sector_count + (info->num_fats * info->fat_size_sectors);

    // FAT start for reading cluster chains
    uint32_t fat_start = partition->lba_start + info->reserved_sector_count;

    // Process the root directory cluster(s)
    while (cluster < 0x0FFFFFF8) { // Continue until end-of-chain (0x0FFFFFF8 or higher)
        // Convert cluster to sector
        uint32_t first_sector = data_start + ((cluster - 2) * info->sectors_per_cluster);
        uint64_t offset = (uint64_t)first_sector * info->bytes_per_sector;

        // Seek to cluster data
        if (fseek(fp, offset, SEEK_SET) != 0) {
            fprintf(stderr, "Failed to seek to cluster %u\n", cluster);
            free(cluster_data);
            fclose(fp);
            return -1;
        }

        // Read cluster data
        if (fread(cluster_data, 1, cluster_size, fp) != cluster_size) {
            fprintf(stderr, "Failed to read cluster %u\n", cluster);
            free(cluster_data);
            fclose(fp);
            return -1;
        }

        // Parse directory entries (32 bytes each)
        for (uint32_t i = 0; i < cluster_size; i += sizeof(DirectoryEntry)) {
            DirectoryEntry *entry = (DirectoryEntry *)(cluster_data + i);

            // Check for end of directory
            if (entry->name[0] == 0x00) {
                break; // No more entries
            }

            // Skip deleted entries
            if (entry->name[0] == 0xE5) {
                continue;
            }

            // Skip LFN entries (attributes == 0x0F)
            if (entry->attr == 0x0F) {
                continue;
            }

            // Skip . and .. entries
            if (strncmp((char *)entry->name, ".          ", 11) == 0 ||
                strncmp((char *)entry->name, "..         ", 11) == 0) {
                continue;
            }

            // Extract short name (first 8 bytes, trim spaces, no extension)
            char name[9] = {0}; // 8 chars + null terminator
            int name_idx = 0;
            for (int j = 0; j < 8 && entry->name[j] != ' '; j++) {
                name[name_idx++] = entry->name[j];
            }
            name[name_idx] = '\0';

            // Print entry with indentation based on depth
            for (int d = 0; d < depth; d++) {
                printf("  ");
            }
            printf("%s (%s)\n", name, (entry->attr & 0x10) ? "Directory" : "File");
        }

        // Read next cluster from FAT
        uint64_t fat_offset = (uint64_t)(fat_start * info->bytes_per_sector) + (cluster * 4);
        if (fseek(fp, fat_offset, SEEK_SET) != 0) {
            fprintf(stderr, "Failed to seek to FAT entry for cluster %u\n", cluster);
            free(cluster_data);
            fclose(fp);
            return -1;
        }

        uint32_t next_cluster;
        if (fread(&next_cluster, sizeof(uint32_t), 1, fp) != 1) {
            fprintf(stderr, "Failed to read FAT entry for cluster %u\n", cluster);
            free(cluster_data);
            fclose(fp);
            return -1;
        }
        next_cluster &= 0x0FFFFFFF; // Mask reserved bits

        cluster = next_cluster; // Move to next cluster
    }

    free(cluster_data);
    fclose(fp);
    return 0;
}


int main(int argc, char *argv[]) {

    // Creating array of informations structures
    PartitionEntry *partitions[PARTITION_COUNT];
    FAT32Info *fat32_info[PARTITION_COUNT];

    for (int i = 0; i < PARTITION_COUNT; i++) {
        partitions[i] = malloc(sizeof(PartitionEntry));
        fat32_info[i] = malloc(sizeof(FAT32Info));

        if (!partitions[i]) {
            fprintf(stderr, 
                "main: Failed to allocate memory for partition entry\n");
            return EXIT_FAILURE;
        }
        if (!fat32_info[i]) {
            fprintf(stderr, 
                "main: Failed to allocate memory for FAT32 info\n");
            return EXIT_FAILURE;
        }
    }

    // Parsing arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <disk_image_file> <mode>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    char *mode = argv[2];

    if (strcmp(mode, "--mbr") == 0) {
        parse_mbr(filename, partitions);
        for (int i = 0; i < PARTITION_COUNT; i++) {
            printPartitionEntry(partitions[i]);
        }

    } else if (strcmp(mode, "--fat") == 0) {
        parse_mbr(filename, partitions);
        for (int i = 0; i < PARTITION_COUNT; i++) {            
            // Only if it is FAT32 shall we print its informations
            if (!parse_fat32_info(filename, partitions[i], fat32_info[i])) {
                printFAT32Info(fat32_info[i]); 
            }
        }
    } else if (strcmp(mode, "--tree") == 0) {
        parse_mbr(filename, partitions);
        for (int i = 0; i < PARTITION_COUNT; i++) {            
            // Only if it is FAT32 shall we print its informations
            if (!parse_fat32_info(filename, partitions[i], fat32_info[i])) {
                
            }
        }
        traverse_directory(
            filename, 
            partitions[0], 
            fat32_info[0], 
            fat32_info[0]->root_cluster, 1);
    } else {
        fprintf(stderr, "This mode doesn't exist. Use '--mbr' or '--fat' or '--tree'.\n");
    }

    freePartitions(partitions, PARTITION_COUNT);
    freeFAT32Info(fat32_info, PARTITION_COUNT);
    return 0;
}
