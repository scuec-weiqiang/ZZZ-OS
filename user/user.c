/**
 * @FilePath: /ZZZ/user/user.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-15 18:50:23
 * @LastEditTime: 2025-09-15 19:40:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

int main()
{
    *(volatile int*)0x20000 = 0xDEADBEEF;
    while (1)
    {
        /* code */
    }
    
    return 0;
}