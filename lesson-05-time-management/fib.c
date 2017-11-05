
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "fib.h"


/************************************************************************************************
 *                  Calculation sum of two binary-coded decimal arrays
 *
 *  retval: boolean overflow
 ***********************************************************************************************/
bool sum_all(const uint8_t *addend1, const uint8_t *addend2, uint8_t *res, uint32_t length)
{
    bool overflow = false;
    int j = 0;
    for(;j<length; j++)
    {
        int i=0;
        for(;i<2; i++)
        {
            // Sum of two binary-coded digits:
            uint8_t sm = ((*(addend1+j)>>4*i)&0xf) + ((*(addend2+j)>>4*i)&0xf) + ((overflow == true)?1:0);
            
            uint8_t add;
            
            if(sm>9) // Need add next order of magnitude
            {
                add = sm-10;
                overflow = true;
            }
            else
            {
                add = sm; // Is enough current size of magnitude
                overflow = false;
            }

            // Clear the binary-coded digit for writing:
            if(i) *(res+j) &= 0x0F; else *(res+j) &= 0xF0;
            
            // Writing the binary-coded digit of sum:
            *(res+j) |= (add<<4*i);
        }
    }
    return overflow;
}

/************************************************************************************************
 *        Calculation sum of first significant bytes of two binary-coded decimal arrays
 *           If res buffer has free space his length will increase in overflow case
 *
 *  retval: boolean overflow
 ***********************************************************************************************/
bool sum_short(const uint8_t *addend1, const uint8_t *addend2, uint8_t *res, uint16_t *length, uint16_t max_len)
{
    bool overflow = sum_all(addend1, addend2, res, *length);
    
    if(overflow)
    {
        if(*length >= max_len)
            return true; // Buffer hasn't any free space
        else
        {
            // Buffer has free space; add one element
            *(res+*length) = 0x01;
            (*length)++;
        }
    }
    return false;
}

/************************************************************************************************
 *                           PUBLIC FUNCTIONS IMPLEMENTATIONS
 ***********************************************************************************************/

void fib_init(fb_t *f)
{
    // Create buffers for fib numbers:
    f->fib_buf0 = (uint8_t*) kmalloc(FIB_BUFFSIZE, 0);
    f->fib_buf1 = (uint8_t*) kmalloc(FIB_BUFFSIZE, 0);
    memset(f->fib_buf0, 0, FIB_BUFFSIZE);
    memset(f->fib_buf1, 0, FIB_BUFFSIZE);

    // Init first value
    f->fib_buf1[0]=0x01;
    f->fib_curr_len = 1;
    f->fib_iter = 0;
    f->ovf = false;
}

void fib_uninit(fb_t *f)
{
    kfree(f->fib_buf0);
    kfree(f->fib_buf1);
}

bool fib_step(fb_t *f)
{
    uint16_t tmpvar_len = f->fib_curr_len;
    uint8_t* tmpvar = (uint8_t*) kmalloc(tmpvar_len, 0);
    // Save the second Fibonacci number:
    memcpy(tmpvar, f->fib_buf1, f->fib_curr_len);
    // Calculate next Fibonacci number:
    f->ovf = sum_short(f->fib_buf0,f->fib_buf1,f->fib_buf1,&(f->fib_curr_len),FIB_BUFFSIZE);
    // Restore to the first Fibonacci number:
    memcpy(f->fib_buf0, tmpvar, tmpvar_len);
    kfree(tmpvar);
    f->fib_iter++;

    return f->ovf;
}

void fib_get_str(fb_t *f, char *buf)
{
    uint16_t i = f->fib_curr_len-1;

    // Copy the highest digit
    if(!(*(f->fib_buf1+f->fib_curr_len-1)>>4))
    {
        // 0X case
        snprintf(buf, FIB_BUFFSIZE, "%u", *(f->fib_buf1+f->fib_curr_len-1));
        buf++;
    }
    else
    {
        // XX case
        snprintf(buf, FIB_BUFFSIZE, "%02x", *(f->fib_buf1+f->fib_curr_len-1));
        buf+=2;
    }

    for( ; i>0; i--)
    {
        snprintf(buf, FIB_BUFFSIZE, "%02x", *(f->fib_buf1+i-1));
        buf+=2;
    }
    snprintf(buf, FIB_BUFFSIZE, "\n");
}

MODULE_LICENSE( "GPL" );
