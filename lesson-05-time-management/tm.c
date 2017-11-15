#include <linux/module.h>
 #include <linux/fs.h>
 #include <linux/cdev.h>
 #include <asm/uaccess.h>
 #include <linux/pci.h>
 #include <linux/version.h>
 #include <linux/init.h>
 #include <linux/sched.h>
 #include <linux/time.h>
 
 static u64 last_sys_time = 0;
 static struct timeval last_abs_time = { 0, 0 };
 
 static ssize_t tm_jiff_show( struct class *class, struct class_attribute *attr, char *buf ) 
{
    size_t count = 0;

    u64 sys_time = get_jiffies_64();
    unsigned int seconds = 0;

    if (last_sys_time)
    {
       u64 diff = sys_time - last_sys_time;
       seconds = jiffies_to_msecs( diff ) / MSEC_PER_SEC;
    }

    sprintf( buf, "Time elapsed since last read: %u seconds\n", seconds );

    count = strlen( buf );
    last_sys_time = sys_time;

    printk( "read %ld\n", (long)count );

    return count;
 }
 
 static ssize_t tm_abs_show( struct class *class, struct class_attribute *attr, char *buf ) {
    size_t count = 0;

    struct timeval tv = { 0, 0 };
    struct timeval tv_diff = { 0, 0 };

    do_gettimeofday( &tv );

    if (last_abs_time.tv_sec)
    {
       s64 ns_current = timeval_to_ns( &tv );
       s64 ns_last    = timeval_to_ns( &last_abs_time );
       s64 diff = ns_current - ns_last;
       tv_diff = ns_to_timeval( diff );
    }

    sprintf( buf, "Time elapsed since last read: %llu.%u seconds \n",
       (long long)tv_diff.tv_sec, (unsigned)tv_diff.tv_usec );

    count = strlen( buf );
    last_abs_time = tv;

    printk( "read %ld\n", (long)count );

    return count;
 }
 
 struct class_attribute class_attr_tm_jiff  = __ATTR_RO( tm_jiff );
 struct class_attribute class_attr_tm_abs = __ATTR_RO( tm_abs );
 
 static struct class* tm_jiff_class;
 static struct class* tm_abs_class;
 
 int __init tm_init(void) {
    int res;

    tm_jiff_class = class_create( THIS_MODULE, "tm_jiff-class" );

    if( IS_ERR( tm_jiff_class ) ) printk( "bad class create\n" );

    res = class_create_file( tm_jiff_class, &class_attr_tm_jiff );
 
    tm_abs_class = class_create( THIS_MODULE, "tm_abs-class" );

    if( IS_ERR( tm_abs_class ) ) printk( "bad class create\n" );

    res |= class_create_file( tm_abs_class, &class_attr_tm_abs );
 
    printk( "'tm' module initialized %d\n", res );
    return res;
 }
 
 void tm_cleanup(void) {
    class_remove_file( tm_abs_class, &class_attr_tm_abs );
    class_destroy( tm_abs_class );
 
    class_remove_file( tm_jiff_class, &class_attr_tm_jiff );
    class_destroy( tm_jiff_class );
    return;
 }
 
 module_init( tm_init );
 module_exit( tm_cleanup );
 
 MODULE_LICENSE( "GPL" );

