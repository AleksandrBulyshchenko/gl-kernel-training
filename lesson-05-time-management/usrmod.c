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

#include "fib.h"
#warning Problem with building multiple file modules, please see Makefile
#include "fib.c"

static struct class *f_class;
static struct timer_list one_sec_tmr;
static fb_t fb_num;

static ssize_t feb_show( struct class *class, struct class_attribute *attr, char *buf )
{
    fib_get_str(&fb_num, buf);
    printk( "read %ld bytes [Fibonacci #%u]\n", (long)strlen( buf ), fb_num.fib_iter );
    return strlen( buf );
}

static ssize_t lst_r_tmr_show( struct class *class, struct class_attribute *attr, char *buf )
{
    static uint32_t lst_reading_tmr = 0;

    if(lst_reading_tmr)
        snprintf(buf, 50, "last reading was %u secs ago\n", jiffies_to_msecs(jiffies)/1000-lst_reading_tmr);
    else
       strcpy(buf, "Newer\n");

    lst_reading_tmr = jiffies_to_msecs(jiffies)/1000;
    return strlen( buf );
}


CLASS_ATTR_RO( feb );
CLASS_ATTR_RO( lst_r_tmr );

void timer_handler(unsigned long data)
{
    if(!fib_step(&fb_num))
    {
        if(mod_timer( &one_sec_tmr, jiffies + msecs_to_jiffies(1000)))
                printk("usrmod: Timer starting fault/n");
    }
    else
        printk("usrmod: Fibonacci buffer overflow\n");
}

int __init mod_init(void)
{
    printk("usrmod start...\n");

    f_class = class_create( THIS_MODULE, "feb-class" );
    if( IS_ERR( f_class ) )
        goto err_cc;
    printk("usrmod: feb-class created\n");

    if(class_create_file( f_class, &class_attr_feb ))
        goto err_cf_feb;
    printk("usrmod: file feb created\n");


    if(class_create_file( f_class, &class_attr_lst_r_tmr ))
        goto err_cf_tmr;
    printk("usrmod: file lst_r_tmr created\n");

    fib_init(&fb_num);
    
    setup_timer(&one_sec_tmr, timer_handler, 0);

    if(mod_timer( &one_sec_tmr, jiffies + msecs_to_jiffies(1000)))
        goto err_tmr;
    printk( "usrmod: timer started\n" );

    printk( "usrmod initialized -----------\n" );
    return 0;

err_tmr:
    fib_uninit(&fb_num);
    class_remove_file( f_class, &class_attr_lst_r_tmr );
err_cf_tmr:
    class_remove_file( f_class, &class_attr_feb );
err_cf_feb:
    class_destroy( f_class );
err_cc:
    printk( "usrmod initialization fault\n" );
    return -1;
}

void mod_cleanup(void) 
{
    del_timer( &one_sec_tmr );
    fib_uninit(&fb_num);
    class_remove_file( f_class, &class_attr_feb );
    class_remove_file( f_class, &class_attr_lst_r_tmr );
    class_destroy( f_class );
    printk( "usrmod removed ---------------\n" );
    return;
}

module_init( mod_init );
module_exit( mod_cleanup );

MODULE_LICENSE( "GPL" );
