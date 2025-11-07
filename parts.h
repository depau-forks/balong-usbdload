
// Partition descriptor structure
struct ptable_line{
    char name[16];
    unsigned start;
    unsigned lsize;
    unsigned length;
    unsigned loadaddr;   
    unsigned entry;      
    unsigned type;  
    unsigned nproperty;  // partition flags
    unsigned count;
};

// Full structure of the partition table page
struct ptable_t {
  uint8_t head[16];
  uint8_t version[16];
  uint8_t product[16];
  struct ptable_line part[41];
  uint8_t tail[32];
};

// table header signature
extern const uint8_t headmagic[16];

uint32_t find_ptable(FILE* ldr);
uint32_t find_ptable_ram(char* buf, uint32_t size);
void show_map(struct ptable_t ptable);
