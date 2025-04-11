#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MBR_SIZE 512
#define PARTITION_COUNT 4
#define PARTITION_ENTRY_SIZE 16
#define PARTITION_ENTRY_OFFSET 446


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
        
        // Performing calculation of the send sector
        uint32_t start_sector = entry.lba_start;
        uint32_t sector_count = entry.sector_count;
        uint32_t end_sector = start_sector + sector_count - 1;

        printf("Partition %d:\n", i + 1);
        printf("  Boot Flag     : 0x%02X\n", entry.boot_flag);
        printf("  CHS Start     : %02X %02X %02X\n", entry.chs_start[0], entry.chs_start[1], entry.chs_start[2]);
        printf("  System ID     : 0x%02X\n", entry.system_id);
        printf("  CHS End       : %02X %02X %02X\n", entry.chs_end[0], entry.chs_end[1], entry.chs_end[2]);
        printf("  Start LBA     : 0x%08X (%u)\n", start_sector, start_sector);
        printf("  End LBA       : 0x%08X (%u)\n", end_sector, end_sector);
        printf("  (Sector Count): 0x%08X (%u)\n\n", sector_count, sector_count);
    }

    fclose(fp);
}


int parse_fat32_info(char *filename, const PartitionEntry *partition, FAT32Info *info_out) {
    FILE *fp = fopen(filename, "rb");

    fprintf(stdout, "Parsing FAT32 info \n");

    fclose(fp);
    return 0;
}

int freePartitions(PartitionEntry *partitions[]) {
    for (int i = 0; i < PARTITION_COUNT; i++) {
        free(partitions[i]);
    }
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
            fprintf(stderr, "Failed to allocate memory for partition entry\n");
            return EXIT_FAILURE;
        }
    }

    // Parsing arguments
    if (strcmp(mode, "--mbr") == 0) {
        parse_mbr(filename, partitions);

    } else if (strcmp(mode, "--fat") == 0) {
        FAT32Info *fat32_info = malloc(sizeof(FAT32Info));
        parse_fat32_info(filename, partitions[0], fat32_info);
        
        free(fat32_info);

    } else {
        fprintf(stderr, "This mode doesn't exist. Use '--mbr' or '--fat'.\n");

        freePartitions(partitions);
        exit(EXIT_FAILURE);
    }

    freePartitions(partitions);
    return 0;
}