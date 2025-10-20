/**
 * @FilePath: /ZZZ-OS/drivers/of/fdt.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-20 20:25:59
 * @LastEditTime: 2025-10-20 23:01:26
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "fdt.h"
#include "printk.h"
#include "string.h"

#define be32_to_cpu(x) __builtin_bswap32(x)
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define GET_NEXT_TOKEN(x,len) __PROTECT((x) = (u32*)((uintptr_t)x + ALIGN_UP(len, 4));)

static struct fdt_header *fdt;
static const char *struct_blk;
static const char *strings;

int fdt_init(void *dtb)
{
    fdt = (struct fdt_header*)dtb;
    if(be32_to_cpu(fdt->magic) != FDT_MAGIC)
    {
        printk("Bad FDT magic!\n");
        return -1;
    }

    struct_blk = (char*)dtb + (size_t)be32_to_cpu(fdt->off_dt_struct);
    strings = (char*)dtb + (size_t)be32_to_cpu(fdt->off_dt_strings);
    printf("[fdt] loaded, struct=%xu, strings=%xu\n", struct_blk, strings);
}
/* /soc
BEGIN_NODE "soc"
  PROP "compatible" = "simple-bus"
  BEGIN_NODE "uart@2020000"
    PROP "compatible" = "fsl,imx6ull-uart"
    PROP "reg" = <0x02020000 0x4000>
  END_NODE
END_NODE
END
*/
void *fdt_find_node(void* parent, const char* child_name)
{
    u32 *p = (u32*)parent+1; // 跳过FDT_BEGIN_NODE
    int depth = 0;

    while (1)
    {
        u32 token = be32_to_cpu(*p);p++;
        switch (token)
        {
            case FDT_BEGIN_NODE:
            {
                const char* name = (const char*)p;
                printk("in node: %s\n", name);
                int len = strlen(name);
                depth++;
                if(strcmp(name, child_name) == 0 && depth == 1)
                {
                    printk("find node: %s\n", name);
                    return (void*)(p - 1 );
                }
                break;
            }
            case FDT_PROP:
            {
                u32 len = be32_to_cpu(*p); p++;
                u32 nameoff = be32_to_cpu(*p); p++;
                p += (len + 3) / 4; // 跳过属性值
                break;
            }
            case FDT_NOP:
                break;
            case FDT_END_NODE:
                depth--;
                break;
            case FDT_END:
                return NULL;
            default:
                break; // 其他
        }
       
    }
    return NULL;
}