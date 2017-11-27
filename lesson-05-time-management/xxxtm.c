#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>

static u64 g_last_sys_time = 0;
static struct timeval g_last_abs_time = { 0, 0 };

static ssize_t tm_jif_show( struct class *class, struct class_attribute *attr, char *buf ) {
   size_t count = 0;
   u64 sys_time = get_jiffies_64();
   unsigned int seconds = 0;
   if (g_last_sys_time)
   {
      u64 diff = sys_time - g_last_sys_time;
      seconds = jiffies_to_msecs( diff ) / MSEC_PER_SEC;
   }
   sprintf( buf, "Time elapsed since last read: %u seconds\n", seconds );
   count = strlen( buf );
   g_last_sys_time = sys_time;
   printk( "read %ld\n", (long)count );
   return count;
}

static ssize_t tm_absk_show( struct class *class, struct class_attribute *attr, char *buf ) {
   size_t count = 0;
   struct timeval tv = { 0, 0 };
   struct timeval tv_diff = { 0, 0 };
   do_gettimeofday( &tv );
   if (g_last_abs_time.tv_sec)
   {
      s64 ns_current = timeval_to_ns( &tv );
      s64 ns_last    = timeval_to_ns( &g_last_abs_time );
      s64 diff = ns_current - ns_last;
      tv_diff = ns_to_timeval( diff );
   }
   sprintf( buf, "Time elapsed since last read: %llu.%u seconds \n",
      (long long)tv_diff.tv_sec, (unsigned)tv_diff.tv_usec );
   count = strlen( buf );
   g_last_abs_time = tv;
   printk( "read %ld\n", (long)count );
   return count;
}

//CLASS_ATTR_RO( xxxtm );
struct class_attribute class_attr_tm_jif  = __ATTR_RO( tm_jif );
struct class_attribute class_attr_tm_absk = __ATTR_RO( tm_absk );

static struct class* tm_jif_class;
static struct class* tm_absk_class;

int __init x_init(void) {
   int res;
   tm_jif_class = class_create( THIS_MODULE, "tm_jif-class" );
   if( IS_ERR( tm_jif_class ) ) printk( "bad class create\n" );
   res = class_create_file( tm_jif_class, &class_attr_tm_jif );

   tm_absk_class = class_create( THIS_MODULE, "tm_absk-class" );
   if( IS_ERR( tm_absk_class ) ) printk( "bad class create\n" );
   res |= class_create_file( tm_absk_class, &class_attr_tm_absk );

   printk( "'xxxtm' module initialized %d\n", res );
   return res;
}

void x_cleanup(void) {
   class_remove_file( tm_absk_class, &class_attr_tm_absk );
   class_destroy( tm_absk_class );

   class_remove_file( tm_jif_class, &class_attr_tm_jif );
   class_destroy( tm_jif_class );
   return;
}

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
