#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm-generic/param.h>

#define LEN_MSG 160
static char buf_msg[ LEN_MSG + 1 ] = "Hello from module!\n";
static long long ll_current_fib_num = 0;
static struct timer_list one_sec_timer;

static long long x_get_fibo(void)
{
   static long long fib_lo = 1;
   static long long fib_hi = 2;
   long long tmp = fib_lo;
   fib_lo = fib_hi;
   fib_hi += tmp;
   return fib_hi;
}

static long long get_current_fib_num(void)
{
   //TODO Add spin_lock()
   long long tmp = ll_current_fib_num;
   return tmp;
}

static void update_current_fib_num(unsigned long data)
{
   //TODO Add spin_lock()
   ll_current_fib_num = x_get_fibo();
   printk("Timer tick");
   one_sec_timer.expires = get_jiffies_64() + 1*HZ;
   add_timer(&one_sec_timer);
}

static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
   long long num = get_current_fib_num();
   memset(buf_msg, 0, LEN_MSG);
   sprintf(buf_msg, "current fibonacci number is %lld\n", num);
   strcpy( buf, buf_msg );
   printk( "%s", buf_msg );
   return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
   printk( "write %ld\n", (long)count );
   strncpy( buf_msg, buf, count );
   buf_msg[ count ] = '\0';
   return count;
}

CLASS_ATTR_RW( xxx );

static struct class *x_class;

int __init x_init(void) {
   int res;
   u64 jiff = 0;
   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( "bad class create\n" );
   res = class_create_file( x_class, &class_attr_xxx );
   printk( "'xxx' module initialized\n" );
   init_timer(&one_sec_timer);
   jiff = get_jiffies_64();
   one_sec_timer.expires = jiff + 1*HZ;
   one_sec_timer.data = 0;
   one_sec_timer.function = update_current_fib_num ;
   add_timer(&one_sec_timer);
   return res;
}

void x_cleanup(void) {
   del_timer(&one_sec_timer);
   class_remove_file( x_class, &class_attr_xxx );
   class_destroy( x_class );
   return;
}

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
