/**
 * @FilePath     : /ZZZ-OS/include/os/bitops.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-13 15:48:22
 * @LastEditTime : 2026-03-13 17:40:14
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __OS_BITOPS_H
#define __OS_BITOPS_H

#define BIT(n)   (1U << (n))

#define GENMASK(h,l) \
    (((~0U) >> (31-(h))) & (~0U << (l)))
    
#define FIELD_PREP(mask,val) \
    (((val) << __builtin_ctz(mask)) & (mask))

#define FIELD_GET(mask,val) \
    (((val) & (mask)) >> __builtin_ctz(mask))

#define SET_VAL(dest,mask,val)  ((dest) = (((dest) & ~(mask)) | FIELD_PREP(mask,val)))
#define GET_VAL(src,mask)       (FIELD_GET(mask,src))

#endif