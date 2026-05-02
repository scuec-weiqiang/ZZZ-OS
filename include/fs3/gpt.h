#ifndef GPT_H
#define GPT_H
#include <os/types.h>
struct gpt_header {
    char     signature[8];   // "EFI PART"
    u32      revision;
    u32      header_size;
    u32      header_crc32;
    u32      reserved;

    u64      current_lba;
    u64      backup_lba;
    u64      first_usable_lba;
    u64      last_usable_lba;

    u8       disk_guid[16];

    u64      part_entry_lba;   // 分区表起始 LBA
    u32      num_part_entries; // 分区项数量
    u32      part_entry_size;  // 单个分区项大小（通常 128）
    u32      part_array_crc32;
};

struct gpt_part_entry {
    u8   type_guid[16];
    u8   uniq_guid[16];
    u64  first_lba;   
    u64  last_lba;
    u64  attr;
    u16  name[36];
};

#endif