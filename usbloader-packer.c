// USB Loader packer/unpacker for Balong chipset
// Unpacks and repacks usbloader.bin files
//
// (c) 2024

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#include "getopt.h"
#endif

#define MAX_BLOCKS 10
#define MAGIC_SIGNATURE 0x00020000
#define HEADER_SIZE 0x54  // 84 bytes - from start to first data block

// Block descriptor structure (16 bytes)
struct block_desc {
    uint32_t lmode;   // boot mode: 1=direct start, 2=via A-core restart
    uint32_t size;    // component size
    uint32_t adr;     // component loading address in memory
    uint32_t offset;  // offset to the component from the beginning of the file
};

// USB loader header structure
struct usbloader_header {
    uint32_t magic;           // offset 0: 0x00020000
    uint8_t reserved1[32];    // offset 4-35: zeros
    struct block_desc blocks[MAX_BLOCKS]; // offset 36+: block descriptors
};

//*************************************************
//* Print usage information
//*************************************************
void print_usage(char* progname) {
    printf("\n USB Loader packer/unpacker for Balong chipset\n\n");
    printf("Usage: %s [options]\n\n", progname);
    printf("Options:\n");
    printf("  -u <file>    Unpack USB loader file\n");
    printf("  -p <dir>     Pack USB loader from directory\n");
    printf("  -o <file>    Output file (for pack mode)\n");
    printf("  -d <dir>     Output directory (for unpack mode, default: <input>.unpacked)\n");
    printf("  -h           Show this help\n\n");
    printf("Examples:\n");
    printf("  %s -u usbloader.bin               # Unpack to usbloader.bin.unpacked/\n", progname);
    printf("  %s -u usbloader.bin -d mydir      # Unpack to mydir/\n", progname);
    printf("  %s -p mydir -o usbloader-new.bin  # Pack from mydir/\n\n", progname);
}

//*************************************************
//* Read entire file into buffer
//*************************************************
uint8_t* read_file(const char* filename, size_t* size) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("\n Error: Cannot open file %s: %s\n", filename, strerror(errno));
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(*size);
    if (!buffer) {
        printf("\n Error: Cannot allocate memory for file %s\n", filename);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t read_size = fread(buffer, 1, *size, f);
    fclose(f);
    
    if (read_size != *size) {
        printf("\n Error: Cannot read file %s\n", filename);
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

//*************************************************
//* Write buffer to file
//*************************************************
int write_file(const char* filename, const uint8_t* buffer, size_t size) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("\n Error: Cannot create file %s: %s\n", filename, strerror(errno));
        return 0;
    }
    
    size_t written = fwrite(buffer, 1, size, f);
    fclose(f);
    
    if (written != size) {
        printf("\n Error: Cannot write to file %s\n", filename);
        return 0;
    }
    
    return 1;
}

//*************************************************
//* Create directory
//*************************************************
int create_directory(const char* path) {
#ifndef WIN32
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
#else
    if (CreateDirectoryA(path, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
#endif
        printf("\n Error: Cannot create directory %s: %s\n", path, strerror(errno));
        return 0;
    }
    return 1;
}

//*************************************************
//* Check if file exists
//*************************************************
int file_exists(const char* filename) {
    struct stat st;
    return (stat(filename, &st) == 0);
}

//*************************************************
//* Unpack USB loader
//*************************************************
int unpack_loader(const char* input_file, const char* output_dir) {
    size_t file_size;
    uint8_t* buffer = read_file(input_file, &file_size);
    if (!buffer) return 0;
    
    // Check minimum size
    if (file_size < HEADER_SIZE) {
        printf("\n Error: File too small to be a valid USB loader\n");
        free(buffer);
        return 0;
    }
    
    // Parse header
    struct usbloader_header* header = (struct usbloader_header*)buffer;
    
    // Check magic signature
    if (header->magic != MAGIC_SIGNATURE) {
        printf("\n Error: Invalid USB loader signature (expected 0x%08x, got 0x%08x)\n", 
               MAGIC_SIGNATURE, header->magic);
        free(buffer);
        return 0;
    }
    
    printf("\n USB Loader: %s\n", input_file);
    printf(" Output directory: %s\n\n", output_dir);
    
    // Create output directory
    if (!create_directory(output_dir)) {
        free(buffer);
        return 0;
    }
    
    // Save header
    char header_path[512];
    snprintf(header_path, sizeof(header_path), "%s/header.bin", output_dir);
    if (!write_file(header_path, buffer, HEADER_SIZE)) {
        free(buffer);
        return 0;
    }
    printf(" [*] Saved header: %s (%d bytes)\n", header_path, HEADER_SIZE);
    
    // Create metadata file
    char meta_path[512];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.txt", output_dir);
    FILE* meta = fopen(meta_path, "w");
    if (!meta) {
        printf("\n Error: Cannot create metadata file\n");
        free(buffer);
        return 0;
    }
    
    fprintf(meta, "# USB Loader Metadata\n");
    fprintf(meta, "# Original file: %s\n", input_file);
    fprintf(meta, "# File size: %zu bytes\n\n", file_size);
    
    // Extract blocks - only process consecutive blocks starting from index 0
    int block_count = 0;
    uint32_t expected_offset = HEADER_SIZE;
    
    for (int i = 0; i < MAX_BLOCKS; i++) {
        struct block_desc* block = &header->blocks[i];
        
        // Check if this is a valid block (size > 0 and offset is reasonable)
        // A valid offset should be >= HEADER_SIZE and within the file
        if (block->size == 0 || block->offset < HEADER_SIZE) {
            // Stop processing blocks after first invalid one
            break;
        }
        
        // Verify block is within file
        if (block->offset + block->size > file_size) {
            printf("\n Warning: Block %d extends beyond file end (offset=0x%x, size=0x%x, file_size=0x%zx)\n",
                   i, block->offset, block->size, file_size);
            break;
        }
        
        // Determine block name
        const char* block_name;
        if (i == 0) block_name = "raminit";
        else if (i == 1) block_name = "usbldr";
        else block_name = "unknown";
        
        // Save block data
        char block_path[512];
        snprintf(block_path, sizeof(block_path), "%s/block%d_%s.bin", output_dir, i, block_name);
        if (!write_file(block_path, buffer + block->offset, block->size)) {
            fclose(meta);
            free(buffer);
            return 0;
        }
        
        printf(" [%d] Block: %s\n", i, block_name);
        printf("     - Mode: %d, Address: 0x%08x\n", block->lmode, block->adr);
        printf("     - Size: 0x%08x (%u bytes)\n", block->size, block->size);
        printf("     - Offset: 0x%08x\n", block->offset);
        printf("     - Saved to: %s\n\n", block_path);
        
        // Write metadata
        fprintf(meta, "[Block%d]\n", i);
        fprintf(meta, "name=%s\n", block_name);
        fprintf(meta, "lmode=%u\n", block->lmode);
        fprintf(meta, "address=0x%08x\n", block->adr);
        fprintf(meta, "size=0x%08x\n", block->size);
        fprintf(meta, "offset=0x%08x\n", block->offset);
        fprintf(meta, "file=block%d_%s.bin\n\n", i, block_name);
        
        block_count++;
    }
    
    fclose(meta);
    free(buffer);
    
    printf(" Total blocks extracted: %d\n", block_count);
    printf(" Metadata saved to: %s\n\n", meta_path);
    
    return 1;
}

//*************************************************
//* Parse metadata file
//*************************************************
int parse_metadata(const char* meta_path, struct block_desc blocks[MAX_BLOCKS], 
                   char block_files[MAX_BLOCKS][256], int* block_count) {
    FILE* f = fopen(meta_path, "r");
    if (!f) {
        printf("\n Error: Cannot open metadata file %s\n", meta_path);
        return 0;
    }
    
    char line[512];
    int current_block = -1;
    *block_count = 0;
    
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // Parse block header
        if (line[0] == '[') {
            if (sscanf(line, "[Block%d]", &current_block) == 1) {
                if (current_block >= 0 && current_block < MAX_BLOCKS) {
                    if (current_block >= *block_count) {
                        *block_count = current_block + 1;
                    }
                }
            }
            continue;
        }
        
        if (current_block < 0 || current_block >= MAX_BLOCKS) continue;
        
        // Parse fields
        char key[64], value[256];
        if (sscanf(line, "%63[^=]=%255[^\r\n]", key, value) == 2) {
            if (strcmp(key, "lmode") == 0) {
                blocks[current_block].lmode = strtoul(value, NULL, 0);
            } else if (strcmp(key, "address") == 0) {
                blocks[current_block].adr = strtoul(value, NULL, 0);
            } else if (strcmp(key, "size") == 0) {
                blocks[current_block].size = strtoul(value, NULL, 0);
            } else if (strcmp(key, "file") == 0) {
                strncpy(block_files[current_block], value, 255);
                block_files[current_block][255] = '\0';
            }
        }
    }
    
    fclose(f);
    return 1;
}

//*************************************************
//* Pack USB loader
//*************************************************
int pack_loader(const char* input_dir, const char* output_file) {
    char meta_path[512];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.txt", input_dir);
    
    // Parse metadata
    struct block_desc blocks[MAX_BLOCKS];
    char block_files[MAX_BLOCKS][256];
    int block_count = 0;
    
    memset(blocks, 0, sizeof(blocks));
    memset(block_files, 0, sizeof(block_files));
    
    if (!parse_metadata(meta_path, blocks, block_files, &block_count)) {
        return 0;
    }
    
    if (block_count == 0) {
        printf("\n Error: No blocks found in metadata\n");
        return 0;
    }
    
    printf("\n Packing USB Loader\n");
    printf(" Input directory: %s\n", input_dir);
    printf(" Output file: %s\n", output_file);
    printf(" Blocks to pack: %d\n\n", block_count);
    
    // Calculate total size needed
    uint32_t current_offset = HEADER_SIZE;
    size_t total_size = HEADER_SIZE;
    
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].size > 0) {
            total_size += blocks[i].size;
        }
    }
    
    // Allocate buffer for packed file
    uint8_t* buffer = (uint8_t*)calloc(total_size, 1);
    if (!buffer) {
        printf("\n Error: Cannot allocate memory for packed file\n");
        return 0;
    }
    
    // Read header if exists
    char header_path[512];
    snprintf(header_path, sizeof(header_path), "%s/header.bin", input_dir);
    if (file_exists(header_path)) {
        size_t header_size;
        uint8_t* header_data = read_file(header_path, &header_size);
        if (header_data) {
            if (header_size > HEADER_SIZE) header_size = HEADER_SIZE;
            memcpy(buffer, header_data, header_size);
            free(header_data);
        }
    }
    
    // Set magic signature
    struct usbloader_header* header = (struct usbloader_header*)buffer;
    header->magic = MAGIC_SIGNATURE;
    
    // Pack blocks
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].size == 0) continue;
        
        // Read block data
        char block_path[512];
        snprintf(block_path, sizeof(block_path), "%s/%s", input_dir, block_files[i]);
        
        size_t block_size;
        uint8_t* block_data = read_file(block_path, &block_size);
        if (!block_data) {
            free(buffer);
            return 0;
        }
        
        // Verify size matches
        if (block_size != blocks[i].size) {
            printf("\n Warning: Block %d size mismatch (metadata: %u, file: %zu)\n",
                   i, blocks[i].size, block_size);
            blocks[i].size = block_size;
        }
        
        // Set block offset
        blocks[i].offset = current_offset;
        
        // Copy block descriptor to header
        memcpy(&header->blocks[i], &blocks[i], sizeof(struct block_desc));
        
        // Copy block data
        memcpy(buffer + current_offset, block_data, block_size);
        
        printf(" [%d] Packed block: %s\n", i, block_files[i]);
        printf("     - Mode: %d, Address: 0x%08x\n", blocks[i].lmode, blocks[i].adr);
        printf("     - Size: 0x%08x (%u bytes)\n", blocks[i].size, blocks[i].size);
        printf("     - Offset: 0x%08x\n\n", blocks[i].offset);
        
        current_offset += block_size;
        free(block_data);
    }
    
    // Write packed file
    int result = write_file(output_file, buffer, total_size);
    free(buffer);
    
    if (result) {
        printf(" Successfully packed to: %s (%zu bytes)\n\n", output_file, total_size);
    }
    
    return result;
}

//*************************************************
//* Main
//*************************************************
int main(int argc, char* argv[]) {
    int opt;
    char* unpack_file = NULL;
    char* pack_dir = NULL;
    char* output_file = NULL;
    char* output_dir = NULL;
    
    printf("\n USB Loader Packer/Unpacker v1.0\n");
    printf(" For Balong chipset USB loaders\n");
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    while ((opt = getopt(argc, argv, "hu:p:o:d:")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'u':
                unpack_file = optarg;
                break;
            case 'p':
                pack_dir = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'd':
                output_dir = optarg;
                break;
            case '?':
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Unpack mode
    if (unpack_file) {
        if (!output_dir) {
            // Create default output directory name
            static char default_dir[512];
            snprintf(default_dir, sizeof(default_dir), "%s.unpacked", unpack_file);
            output_dir = default_dir;
        }
        
        if (!unpack_loader(unpack_file, output_dir)) {
            printf("\n Unpacking failed!\n\n");
            return 1;
        }
        
        printf(" Unpacking completed successfully!\n\n");
        return 0;
    }
    
    // Pack mode
    if (pack_dir) {
        if (!output_file) {
            printf("\n Error: Output file (-o) is required for pack mode\n");
            print_usage(argv[0]);
            return 1;
        }
        
        if (!pack_loader(pack_dir, output_file)) {
            printf("\n Packing failed!\n\n");
            return 1;
        }
        
        printf(" Packing completed successfully!\n\n");
        return 0;
    }
    
    // No mode specified
    printf("\n Error: Either -u (unpack) or -p (pack) must be specified\n");
    print_usage(argv[0]);
    return 1;
}
