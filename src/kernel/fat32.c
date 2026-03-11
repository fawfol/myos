#include "fat32.h"
#include "ata.h"
#include "shell.h"
#include "memory.h"

uint32_t fat32_partition_lba = 0;
fat32_bpb_t bpb;

void init_fat32(uint32_t partition_lba) {
    fat32_partition_lba = partition_lba;
    uint8_t buffer[512];

    terminal_print("FAT32: Mounting partition at LBA ");
    terminal_print_number(partition_lba);
    terminal_print("...\n");

    //read Sector 0 of the partition (The Boot Sector / BPB)
    ata_read_sector(fat32_partition_lba, buffer);

    //cast the raw bytes into our struct
    fat32_bpb_t* parsed_bpb = (fat32_bpb_t*)buffer;

    //verify its actually a FAT32 drive
    // 0x28 and 0x29 standard extended boot signatures for FAT32
    if (parsed_bpb->boot_signature != 0x28 && parsed_bpb->boot_signature != 0x29) {
        terminal_print("FAT32 Error: Invalid Boot Signature (Expected 0x28/0x29, got ");
        terminal_print_hex(parsed_bpb->boot_signature);
        terminal_print(")\n");
        return;
    }

    //copy bpb into our global variable so we can use it later for math
    memcpy(&bpb, parsed_bpb, sizeof(fat32_bpb_t));

    //volume label is exactly 11 characters. It is NOT null-terminated on the disk.
    //need to build a safe, null-terminated string to pass to terminal_print.
    char vol_label[12];
    for (int i = 0; i < 11; i++) {
        vol_label[i] = bpb.volume_label[i];
    }
    vol_label[11] = '\0';

    terminal_print("FAT32: Volume Label is [");
    terminal_print(vol_label);
    terminal_print("]\n");
    
    //print some vital stats that will help us calculate file locations later
    terminal_print("FAT32: Bytes per sector: ");
    terminal_print_number(bpb.bytes_per_sector);
    terminal_print("\nFAT32: Sectors per cluster: ");
    terminal_print_number(bpb.sectors_per_cluster);
    terminal_print("\nFAT32: Root Cluster: ");
    terminal_print_number(bpb.root_cluster);
    terminal_print("\nFAT32: Ready.\n\n");
}
