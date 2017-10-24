#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/timer.h>


#define LEN_MSG 160
#define DELAY HZ
static unsigned int fib_n;
static char* buf_msg;
static struct timer_list irq_timer;
//static unsigned long time_interval;

unsigned long long int fib(unsigned int n)
{
   unsigned long long int first = 0, second = 1, next = 0;
   unsigned int i;
   for ( i = 0 ; i <= n ; i++ )
   {
      if ( i <= 1 )
         next = i;
      else
      {
         next = first + second;
         first = second;
         second = next;
      }
   }
   return next;
}

static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
   
   strcpy( buf, buf_msg );  
//   printk("timeout: %ld", time_interval);
   printk( "read %ld\n", (long)strlen( buf ) );   
   return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
//   printk( "write %ld\n", (long)count );
   strncpy( buf_msg, buf, count );
   buf_msg[ count ] = '\0';
   return count;
}

CLASS_ATTR_RW( xxx );

static struct class *x_class;

void IRQ_TimerHandler(unsigned long data)
{
	char buf[100];
//	time_interval++;
	sprintf(buf,"%u %llu\n",fib_n,fib(fib_n));
//   	printk( KERN_INFO "%s\n", buf );
   	xxx_store(x_class, &class_attr_xxx,buf,strlen(buf)+1);
	if (fib_n < 93) fib_n++;
   	else fib_n = 0;
	irq_timer.expires = get_jiffies_64() + DELAY;
	add_timer(&irq_timer);
}


int __init x_init(void) {
   int res;   
   buf_msg = kmalloc(LEN_MSG+1,GFP_KERNEL | __GFP_ZERO);   
   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( "bad class create\n" );
   res = class_create_file( x_class, &class_attr_xxx );
   
   fib_n = 0;

//   time_interval = 0;
//init timer
   init_timer(&irq_timer);
//inicialize struct
   irq_timer.expires = get_jiffies_64() + DELAY;
   irq_timer.data = 0;
   irq_timer.function = IRQ_TimerHandler;
//add timer
   add_timer(&irq_timer);

   printk( "'xxx' module initialized\n" );
   return res;
}

void x_cleanup(void) {
   del_timer(&irq_timer);
   class_remove_file( x_class, &class_attr_xxx );
   class_destroy( x_class );
   kfree(buf_msg);   
   
   return;
}

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
