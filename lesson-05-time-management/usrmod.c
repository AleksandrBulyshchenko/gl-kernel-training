#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>

#include "fib.c"

static struct timer_list one_sec_tmr;

static char* out_buf;

fb_t fb_num;

static struct class *f_class;

static uint32_t lst_reading_tmr = 0;

static ssize_t feb_show( struct class *class, struct class_attribute *attr, char *buf ) {
   fib_get_str(fb_num.fib_buf1, out_buf, fb_num.fib_curr_len);
   strcpy( buf, out_buf );
   printk( "read %ld bytes [Fibonacci #%u]\n", (long)strlen( buf ), fb_num.fib_iter );
   return strlen( buf );
}

static ssize_t lst_r_tmr_show( struct class *class, struct class_attribute *attr, char *buf ) {
    if(!lst_reading_tmr)
    {
        const char out[] = "Newer\n";
        strcpy(buf, out);
        
    }
    else
    {
        char out[50];
        snprintf(out, 50, "last reading was %u secs ago\n", jiffies_to_msecs(jiffies)/1000-lst_reading_tmr);
        strcpy(buf, out);
    }
    lst_reading_tmr = jiffies_to_msecs(jiffies)/1000;
    return strlen( buf );
}


CLASS_ATTR_RO( feb );
CLASS_ATTR_RO( lst_r_tmr );

void timer_handler(unsigned long data)
{
    if(!fib_step(&fb_num))
        mod_timer( &one_sec_tmr, jiffies + msecs_to_jiffies(1000) );
    else
        printk("fib_overflow");
}

int __init mod_init(void)
{
    int res = 0;
    
    f_class = class_create( THIS_MODULE, "feb-class" );
    if( IS_ERR( f_class ) ) printk( "bad class create\n" );
    res |= class_create_file( f_class, &class_attr_feb );
    res |= class_create_file( f_class, &class_attr_lst_r_tmr );
    
    fb_num.fib_buf0 = (uint8_t*) kmalloc(FIB_BUFFSIZE, 0);
    fb_num.fib_buf1 = (uint8_t*) kmalloc(FIB_BUFFSIZE, 0);
    memset(fb_num.fib_buf0, 0, FIB_BUFFSIZE);
    memset(fb_num.fib_buf1, 0, FIB_BUFFSIZE);
    fb_num.fib_buf1[0]=0x01;
    fb_num.tmp = (uint8_t*) kmalloc(FIB_BUFFSIZE, 0);
    memset(fb_num.tmp, 0, FIB_BUFFSIZE);
    fb_num.fib_curr_len = 1;
    fb_num.fib_iter = 0;
    fb_num.ovf = false;
    out_buf = (char*) kmalloc(FIB_BUFFSIZE*2, 0);
    memset(out_buf, 0, FIB_BUFFSIZE*2);
    
    
    setup_timer(&one_sec_tmr, timer_handler, 0);
    
    printk( "timer start\n" );
    res |= mod_timer( &one_sec_tmr, jiffies + msecs_to_jiffies(1000) );
    if (res) printk("Error in mod_timer\n");
    
    return res;
}

void mod_cleanup(void) 
{
    del_timer( &one_sec_tmr );
    kfree(fb_num.fib_buf0);
    kfree(fb_num.fib_buf1);
    kfree(fb_num.tmp);
    kfree(out_buf);
    class_remove_file( f_class, &class_attr_feb );
    class_remove_file( f_class, &class_attr_lst_r_tmr );
    class_destroy( f_class );
    return;
}

module_init( mod_init );
module_exit( mod_cleanup );

MODULE_LICENSE( "GPL" );
