#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

// FAT32 BIOS Parameter Block (BPB)
typedef struct {
    uint8_t  jump_code[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // FAT32 Extended Information
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;     //usually 0x28 or 0x29
    uint32_t volume_id;
    char     volume_label[11];   // "KALSANG_OS "
    char     fs_type[8];         // "FAT32   "
} __attribute__((packed)) fat32_bpb_t;

// FAT32 32-byte Directory Entry
typedef struct {
    char     name[11];               //8.3 filename (e.g., "TEST    TXT")
    uint8_t  attributes;             //read-only, hidden, directory and all
    uint8_t  reserved;
    uint8_t  creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t cluster_high;           //high 16 bits of the starting cluster
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t cluster_low;            //low 16 bits of the starting cluster
    uint32_t size;                   //file size in bytes
} __attribute__((packed)) fat32_dir_entry_t;

void fat32_list_root();

void fat32_read_file(char* filename);

void init_fat32(uint32_t partition_lba);

#endif
