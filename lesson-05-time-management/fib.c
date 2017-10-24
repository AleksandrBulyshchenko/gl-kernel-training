
#include <linux/module.h>
#include <linux/kernel.h>

#include "fib.h"


void fib_get_str(const uint8_t *a, uint8_t *str, uint32_t length)
{
    uint16_t i=length-1;
    //char* sprf_ptr = str;
    
    if(length>FIB_BUFFSIZE) return;
    
    if(!(*(a+length-1)>>4))
    {
        //printk("%u", *(a+length-1));
        snprintf(str, FIB_BUFFSIZE, "%u", *(a+length-1));
        str++;
    }
    else
    {
        snprintf(str, FIB_BUFFSIZE, "%02x", *(a+length-1));
        str+=2;
    }

    for( ; i>0; i--)
    {
        snprintf(str, FIB_BUFFSIZE, "%02x", *(a+i-1));
        str+=2;
    }
    snprintf(str, FIB_BUFFSIZE, "\n");
    
    //printk("%s", out_buf);
    
    /*out_buf
    int snprintf(char *buf, size_t size, const char *fmt, ...);*/
}

bool sum_all(const uint8_t *a, const uint8_t *b, uint8_t *c, uint32_t length)
{
    bool overflow = false;
    int j = 0;
    for(;j<length; j++)
    {
        //*(c+j) = 0;
        int i=0;
        for(;i<2; i++)
        {
            uint8_t sm = ((*(a+j)>>4*i)&0xf) + ((*(b+j)>>4*i)&0xf) + (overflow == true);
            
            uint8_t add;
            
            if(sm>9)
            {
                add = sm-10;
                overflow = true;
            }
            else
            {
                add = sm;
                overflow = false;
            }
            
            //printf("add: %u %u sm: %u\n", ((*(a+j)>>4*i)&0xf) , ((*(b+j)>>4*i)&0xf),sm);
            
            if(i) *(c+j) &= 0x0F; else *(c+j) &= 0xF0;
            
            *(c+j) |= (add<<4*i);
        }
    }
    return overflow;
}

bool sum_short(const uint8_t *a, const uint8_t *b, uint8_t *c, uint16_t *length, uint16_t max_len)
{
    bool overflow = sum_all(a,b,c,*length);
    
    if(overflow)
    {
        if(*length >= max_len)
            return true;
        else
        {
            //printf("len=%u\n", *length);
            *(c+*length) = 0x01;
            (*length)++;
            //printf("len=%u\n", *length);
            
        }
    }
    return false;
}

bool fib_step(fb_t *fib)
{
    memcpy(fib->tmp, fib->fib_buf1, fib->fib_curr_len);
    fib->ovf = sum_short(fib->fib_buf0,fib->fib_buf1,fib->fib_buf1,&(fib->fib_curr_len),FIB_BUFFSIZE);
    memcpy(fib->fib_buf0, fib->tmp, fib->fib_curr_len);
    fib->fib_iter++;
    return fib->ovf;
}
