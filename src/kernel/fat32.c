#include "fat32.h"
#include "ata.h"
#include "shell.h"
#include "memory.h"

uint32_t fat32_allocate_chain(uint32_t num_clusters);
uint32_t fat32_partition_lba = 0;
uint32_t fat32_data_start_lba = 0;
fat32_bpb_t bpb;

// --- MATH HELPERS ---

uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    return fat32_data_start_lba + ((cluster - 2) * bpb.sectors_per_cluster);
}

void format_fat32_name(char* input, char* output) {
    for (int k = 0; k < 11; k++) output[k] = ' ';
    int i = 0;
    for (i = 0; i < 8 && input[i] != '.' && input[i] != '\0'; i++) {
        output[i] = (input[i] >= 'a' && input[i] <= 'z') ? input[i] - 32 : input[i];
    }
    char* dot = 0;
    for (int k = 0; input[k] != '\0'; k++) if (input[k] == '.') { dot = &input[k + 1]; break; }
    if (dot) {
        for (int k = 0; k < 3 && dot[k] != '\0'; k++) {
            output[8 + k] = (dot[k] >= 'a' && dot[k] <= 'z') ? dot[k] - 32 : dot[k];
        }
    }
    output[11] = '\0';
}

// --- CORE FUNCTIONS ---

void init_fat32(uint32_t partition_lba) {
    fat32_partition_lba = partition_lba;
    uint8_t buffer[512];
    ata_read_sector(fat32_partition_lba, buffer);
    memcpy(&bpb, buffer, sizeof(fat32_bpb_t));

    uint32_t fat_start_lba = fat32_partition_lba + bpb.reserved_sectors;
    fat32_data_start_lba = fat_start_lba + (bpb.fat_count * bpb.sectors_per_fat_32);
    terminal_print("FAT32: Mounted successfully.\n");
}

void fat32_list_root() {
    uint32_t root_lba = fat32_cluster_to_lba(bpb.root_cluster);
    uint8_t buffer[512];
    ata_read_sector(root_lba, buffer);
    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)buffer;

    terminal_print("\n--- Root Directory ---\n");
    for (int i = 0; i < 16; i++) {
        if (dir[i].name[0] == 0x00) break;
        if (dir[i].name[0] == (char)0xE5 || dir[i].attributes == 0x0F) continue;

        char name[12];
        memcpy(name, dir[i].name, 11); name[11] = '\0';
        terminal_print(name); terminal_print(" | ");
        terminal_print_number(dir[i].size); terminal_print(" bytes\n");
    }
}

void fat32_read_file(char* filename) {
    char fat_name[12];
    format_fat32_name(filename, fat_name);

    uint8_t buffer[512];
    ata_read_sector(fat32_cluster_to_lba(bpb.root_cluster), buffer);
    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)buffer;

    for (int i = 0; i < 16; i++) {
        if (strncmp(dir[i].name, fat_name, 11) == 0) {
            uint32_t cluster = ((uint32_t)dir[i].cluster_high << 16) | dir[i].cluster_low;
            uint32_t bytes_left = dir[i].size;
            
            // Read FAT to follow chains
            uint32_t fat_table[128];
            ata_read_sector(fat32_partition_lba + bpb.reserved_sectors, (uint8_t*)fat_table);

            while (bytes_left > 0 && cluster < 0x0FFFFFF8) {
                uint8_t file_buf[512];
                ata_read_sector(fat32_cluster_to_lba(cluster), file_buf);
                uint32_t to_print = (bytes_left > 512) ? 512 : bytes_left;
                
                for (uint32_t b = 0; b < to_print; b++) {
                    char c[2] = { (char)file_buf[b], '\0' };
                    terminal_print(c);
                }
                bytes_left -= to_print;
                cluster = fat_table[cluster] & 0x0FFFFFFF;
            }
            terminal_print("\n");
            return;
        }
    }
    terminal_print("File not found.\n");
}



uint32_t fat32_allocate_chain(uint32_t num_clusters) {
    uint32_t fat_start_lba = fat32_partition_lba + bpb.reserved_sectors;
    uint32_t fat_table[128]; 
    ata_read_sector(fat_start_lba, (uint8_t*)fat_table);

    uint32_t first_cluster = 0;
    uint32_t prev_cluster = 0;
    uint32_t allocated = 0;

    for (int i = 2; i < 128 && allocated < num_clusters; i++) {
        if ((fat_table[i] & 0x0FFFFFFF) == 0x00000000) {
            if (first_cluster == 0) first_cluster = i;
            if (prev_cluster != 0) fat_table[prev_cluster] = i;
            
            fat_table[i] = 0x0FFFFFFF; 
            prev_cluster = i;
            allocated++;
        }
    }

    if (allocated == num_clusters) {
        ata_write_sector(fat_start_lba, (uint8_t*)fat_table);
        return first_cluster;
    }
    return 0; 
}


void fat32_write_file(char* filename, char* data, uint32_t size) {
    char fat_name[12];
    format_fat32_name(filename, fat_name);

    uint32_t root_lba = fat32_cluster_to_lba(bpb.root_cluster);
    uint8_t dir_buf[512];
    ata_read_sector(root_lba, dir_buf);
    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)dir_buf;

    // 1. Check if file exists to overwrite
    int slot = -1;
    for (int i = 0; i < 16; i++) {
        if (strncmp(dir[i].name, fat_name, 11) == 0) { slot = i; break; }
        if (slot == -1 && (dir[i].name[0] == 0x00 || dir[i].name[0] == (char)0xE5)) slot = i;
    }

    if (slot == -1) { terminal_print("Dir full\n"); return; }

    // 2. Allocate chain
    uint32_t num_clusters = (size + 511) / 512;
    uint32_t cluster = fat32_allocate_chain(num_clusters);

    // 3. Write data blocks
    uint32_t current = cluster;
    uint32_t bytes_left = size;
    uint32_t offset = 0;
    
    uint32_t fat_table[128];
    ata_read_sector(fat32_partition_lba + bpb.reserved_sectors, (uint8_t*)fat_table);

    while (bytes_left > 0) {
        uint8_t block[512] = {0};
        uint32_t to_write = (bytes_left > 512) ? 512 : bytes_left;
        memcpy(block, data + offset, to_write);
        ata_write_sector(fat32_cluster_to_lba(current), block);
        bytes_left -= to_write;
        offset += to_write;
        if (bytes_left > 0) current = fat_table[current] & 0x0FFFFFFF;
    }

    // 4. Update Directory Entry
    memcpy(dir[slot].name, fat_name, 11);
    dir[slot].size = size;
    dir[slot].cluster_low = cluster & 0xFFFF;
    dir[slot].cluster_high = (cluster >> 16) & 0xFFFF;
    dir[slot].attributes = 0x20;
    ata_write_sector(root_lba, dir_buf);
    terminal_print("Saved.\n");
}

void fat32_delete_file(char* filename) {
    char fat_name[12];
    format_fat32_name(filename, fat_name);

    uint8_t dir_buf[512];
    ata_read_sector(fat32_cluster_to_lba(bpb.root_cluster), dir_buf);
    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)dir_buf;

    for (int i = 0; i < 16; i++) {
        if (strncmp(dir[i].name, fat_name, 11) == 0) {
            // Mark deleted
            dir[i].name[0] = (char)0xE5;
            ata_write_sector(fat32_cluster_to_lba(bpb.root_cluster), dir_buf);
            terminal_print("Deleted.\n");
            return;
        }
    }
}


// Returns the number of bytes read
uint32_t fat32_get_file_data(char* filename, char* out_buffer) {
    char fat_name[12];
    format_fat32_name(filename, fat_name);

    uint8_t buffer[512];
    ata_read_sector(fat32_cluster_to_lba(bpb.root_cluster), buffer);
    fat32_dir_entry_t* dir = (fat32_dir_entry_t*)buffer;

    for (int i = 0; i < 16; i++) {
        if (strncmp(dir[i].name, fat_name, 11) == 0) {
            uint32_t cluster = ((uint32_t)dir[i].cluster_high << 16) | dir[i].cluster_low;
            uint32_t bytes_to_read = dir[i].size;
            uint32_t total_read = 0;
            
            uint32_t fat_table[128];
            ata_read_sector(fat32_partition_lba + bpb.reserved_sectors, (uint8_t*)fat_table);

            while (bytes_to_read > 0 && cluster < 0x0FFFFFF8) {
                uint8_t file_buf[512];
                ata_read_sector(fat32_cluster_to_lba(cluster), file_buf);
                uint32_t to_copy = (bytes_to_read > 512) ? 512 : bytes_to_read;
                
                memcpy(out_buffer + total_read, file_buf, to_copy);
                
                total_read += to_copy;
                bytes_to_read -= to_copy;
                cluster = fat_table[cluster] & 0x0FFFFFFF;
            }
            return total_read;
        }
    }
    return 0;
}
