#include <stdarg.h>
#include "types.h"
#include "uart.h"

#define __PBUFF_SIZE 1024

static char printf_buff[__PBUFF_SIZE];//输出缓冲区1k字节

/***************************************************************
 * @description: 将一个无符号整型数转换为字符串
 * @param {char*} str [out]:  字符串
 * @param {unsigned int} pos [in]:  从<str>的哪个位置开始写入字符串
 * @param {int} num [in]:  需要转化的数字
 * @param {int} decimal [in]:  进制,可选2，10，16
 * @return {int} 返回转化后字符串长度
***************************************************************/
int num2char(char* str,unsigned int pos,unsigned int num,int decimal)
{
    unsigned int digit = 1;//进制下位数

    for(int temp = num;temp/=decimal;digit++);//记录在decimal进制下有多少位
    if(NULL_PTR != str)
    {
        switch (decimal)
        {
            case 16:
                str[pos] = '0';pos++;str[pos] = 'x';pos++;
                break;
            case 2:
                str[pos] = '0';pos++;str[pos] = 'b';pos++;
                break;   
            case 10:
                break;
            default:
                return -1;
        }
        for(int i=digit-1;i>=0;i--)//从第一位开始将需要格式化的地方替换为字符数字
        {   
            unsigned int temp = 0;
            temp = num % decimal;
            if(temp>=10) //考虑16进制中出现字母
            {
                temp -= 10; 
                temp +='a';
                str[pos+i] = temp;
            }
            else
            {
                str[pos+i] = '0' + temp;//替换为数字字符
            }
            num /= decimal;
        }
    }

    if(10 == decimal) return digit;
    else              return digit+2;//这里返回的是数字转化为字符串之后的长度，16进制与2进制因为前面有0x或0b所以要多加两位

    
}

/***************************************************************
 * @description: 
 * @param {char*} out_buff [out]:  输出缓冲区
 * @param {char} *str [in]:  源字符串
 * @param {va_list} valist [in]:  可变参数列表
 * @return {*}
***************************************************************/
int _vsprintf(char* out_buff,const char *str,va_list vl)
{
    uint8_t format = 0;//置一表明遍历到了需要格式化输出的位置，比如%d
	size_t pos = 0;//这是输出缓冲区的下标
    uint8_t longarg = 0;//long型标志位
    uint8_t decimal = 0;//进制标志位

    for(;(*str);str++)//遍历整个字符串
    {
        if(1 == format)//遍历到需要格式化输出的部分
        {
            switch( (*str) )
            {   
                case 'l':longarg = 1;goto DEC;
                case 'x':decimal = 16;goto DEC;
                case 'b':decimal = 2;goto DEC;
                case 'd':
                    DEC://整数输出
                    long num = longarg?va_arg(vl,long):va_arg(vl,int);
                    if(0 == decimal)
                    {
                        decimal = 10;
                        if(num<0 && NULL_PTR != out_buff)
                        {
                            num = -num; 
                            out_buff[pos] = '-';
                            pos++;
                        }
                    }
                    pos += num2char(out_buff,pos,num,decimal); //将数字转化为字符串
                    //更新输出缓冲区的下标,指向下一个空白位置

                    longarg = 0;//清除标志位
                    format = 0;
                    decimal = 0;
                break;

                case 'c':
                    int c = va_arg(vl,int);
                    if(NULL_PTR != out_buff)
                    {
                        out_buff[pos] = (char)c;
                    }
                    pos++;
                    format = 0;
                break;

                case 's':
                    addr_t addr = va_arg(vl,int);
                    while(*(char*)addr)
                    {
                        char c = *(char*)addr;
                        if(NULL_PTR != out_buff)
                        {
                            out_buff[pos] = (char)c;
                        }
                        pos++;
                        addr++;
                    }
                    format = 0;
                break;
                
                default:
                    format = 0;
                break;
            }
        }
        else if ( '%' == (*str) )//遇到了%,代表后面需要格式化输出
        {   
            format = 1;
        }
        else//遍历到普通字符
        {
            if(NULL_PTR != out_buff)
            {
                out_buff[pos] = (*str);//不用处理直接写入到输出缓冲区
            }
            pos++;//指向输出缓冲区的下一个位置
        }
    }
    if(NULL_PTR != out_buff)//在结尾加上结束符
    {
        out_buff[pos] = 0;
    }
    return pos;
}


/***************************************************************
 * @description: 
 * @param {char*} str [in/out]:  
 * @param {va_list} vl [in/out]:  
 * @return {*}
***************************************************************/
int _vprintf(const char* str,va_list vl)
{
    int n =  _vsprintf(NULL_PTR,str,vl);//统计一下格式化字符串
    if(n>__PBUFF_SIZE)
    {
        uart_puts("Error: Output string size overflow!\n");
        while (1)
        {
           
        }
    }
    _vsprintf(printf_buff,str,vl);

    /*重定向只需要改下面这个输出*/
    uart_puts(printf_buff);
    
    return n;
}


/***************************************************************
 * @description: 格式化输出
 * @param {char*} str [in]:  
 * @return {*}
***************************************************************/
int printf(const char *str, ...)
{
    va_list vl;
    va_start(vl,str);
    int n = _vprintf(str,vl);
    va_end(vl);
    return n;
}

int panic(const char *str)
{
    printf("panic:%s\n",str);
    while(1){}
}