#ifndef GPT_H
#define GPT_H
#include <os/types.h>
struct gpt_header {
    char     signature[8];   // "EFI PART"
    uint32_t      revision;
    uint32_t      header_size;
    uint32_t      header_crc32;
    uint32_t      reserved;

    uint64_t      current_lba;
    uint64_t      backup_lba;
    uint64_t      first_usable_lba;
    uint64_t      last_usable_lba;

    uint8_t       disk_guid[16];

    uint64_t      part_entry_lba;   // 分区表起始 LBA
    uint32_t      num_part_entries; // 分区项数量
    uint32_t      part_entry_size;  // 单个分区项大小（通常 128）
    uint32_t      part_array_crc32;
};

struct gpt_part_entry {
    uint8_t   type_guid[16];
    uint8_t   uniq_guid[16];
    uint64_t  first_lba;   
    uint64_t  last_lba;
    uint64_t  attr;
    uint16_t  name[36];
};

#endif