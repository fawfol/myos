#include "fat32.h"
#include "ata.h"
#include "shell.h"
#include "memory.h"

uint32_t fat32_partition_lba = 0;
uint32_t fat32_data_start_lba = 0;
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

//math to convert a FAT32 Cluster number to an ATA LBA Sector number
uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    // Clusters 0 and 1 are reserved, so cluster 2 is actually the first data cluster
    return fat32_data_start_lba + ((cluster - 2) * bpb.sectors_per_cluster);
}

void fat32_list_root() {
    if (fat32_partition_lba == 0) {
        terminal_print("Error: FAT32 not mounted.\n");
        return;
    }

    // 1. calculate exactly where the data region starts
    uint32_t fat_start_lba = fat32_partition_lba + bpb.reserved_sectors;
    fat32_data_start_lba = fat_start_lba + (bpb.fat_count * bpb.sectors_per_fat_32);

    // 2. get the LBA for the Root Directory (Cluster 2)
    uint32_t root_lba = fat32_cluster_to_lba(bpb.root_cluster);
    
    // 3. read the sector
    uint8_t buffer[512];
    ata_read_sector(root_lba, buffer);

    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)buffer;
    
    terminal_print("\n--- FAT32 Root Directory ('/') ---\n");
    
    //a 512-byte sector holds exactly 16 of these 32-byte entries
    for (int i = 0; i < 16; i++) {
        if (dir[i].name[0] == 0x00) break;              // 0x00 means end of directory
        if (dir[i].name[0] == (char)0xE5) continue;     // 0xE5 means deleted file
        if (dir[i].attributes == 0x0F) continue;        // 0x0F means Long File Name (LFN) part, skip for now
        if (dir[i].attributes & 0x08) continue;         // 0x08 means Volume Label (like KALSANG_OS), skip

        //etract the name safely
        char filename[12];
        for (int j = 0; j < 11; j++) {
            filename[j] = dir[i].name[j];
        }
        filename[11] = '\0';

        terminal_print("File: [");
        terminal_print(filename);
        terminal_print("] | Size: ");
        terminal_print_number(dir[i].size);
        terminal_print(" bytes | Cluster: ");
        
        //cluster number is split across two 16-bit variables
        uint32_t start_cluster = ((uint32_t)dir[i].cluster_high << 16) | dir[i].cluster_low;
        terminal_print_number(start_cluster);
        terminal_print("\n");
    }
    terminal_print("----------------------------------\n");
}


//heler to convert standard "TEST.TXT" into FAT32's "TEST    TXT" format
void format_fat32_name(char* input, char* output) {
    int i = 0, j = 0;
    //fill with spaces by default
    for (int k = 0; k < 11; k++) output[k] = ' ';
    output[11] = '\0';

    //cp filename up to the dot
    while (input[i] != '.' && input[i] != '\0' && i < 8) {
        //convert to uppercase
        if (input[i] >= 'a' && input[i] <= 'z') {
            output[j++] = input[i] - 32;
        } else {
            output[j++] = input[i];
        }
        i++;
    }

    //skip the dot
    while (input[i] != '.' && input[i] != '\0') i++;
    if (input[i] == '.') i++;

    //copy extension
    j = 8;
    while (input[i] != '\0' && j < 11) {
        if (input[i] >= 'a' && input[i] <= 'z') {
            output[j++] = input[i] - 32;
        } else {
            output[j++] = input[i];
        }
        i++;
    }
}

void fat32_read_file(char* filename) {
    if (fat32_partition_lba == 0) {
        terminal_print("Error: FAT32 not mounted.\n");
        return;
    }

    char fat_name[12];
    format_fat32_name(filename, fat_name);

    uint32_t root_lba = fat32_cluster_to_lba(bpb.root_cluster);
    uint8_t buffer[512];
    ata_read_sector(root_lba, buffer);

    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)buffer;
    
    for (int i = 0; i < 16; i++) {
        if (dir[i].name[0] == 0x00) break;
        if (dir[i].name[0] == (char)0xE5 || dir[i].attributes == 0x0F || (dir[i].attributes & 0x08)) continue;

        //check fi names match
        bool match = true;
        for (int j = 0; j < 11; j++) {
            if (dir[i].name[j] != fat_name[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            uint32_t file_cluster = ((uint32_t)dir[i].cluster_high << 16) | dir[i].cluster_low;
            uint32_t file_lba = fat32_cluster_to_lba(file_cluster);
            uint32_t file_size = dir[i].size;

            terminal_print("Reading ");
            terminal_print(filename);
            terminal_print("...\n\n");

            //read the files data sector
            uint8_t file_buffer[512];
            ata_read_sector(file_lba, file_buffer);

            //print the characters up to the file size
            for (uint32_t b = 0; b < file_size && b < 512; b++) {
                char char_buf[2] = { file_buffer[b], '\0' };
                terminal_print(char_buf);
            }
            terminal_print("\n");
            return;
        }
    }
    
    terminal_print("Error: File not found.\n");
}
