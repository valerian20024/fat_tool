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

/*
This function takes as input a filename of a disk image and parses the MBR to extract every partition information.
*/
void parse_mbr(const char *img_filename) {
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
        /*
        if (entry.sector_count == 0)
            continue;  // Skip empty partition
        */
        
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

int main(int argc, char *argv[]) {

    char * filename = "mock.img";
    parse_mbr(filename);


    return 0;
}