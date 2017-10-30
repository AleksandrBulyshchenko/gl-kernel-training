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
#define DELAY 1000
static char* buf_msg;
static struct timer_list irq_timer;
static unsigned long time_interval;
static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
   
   strcpy( buf, buf_msg );  
   printk("timeout: %ld", time_interval);
   printk( "read %ld\n", (long)strlen( buf ) );   
   return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
   //printk( "write %ld\n", (long)count );
   strncpy( buf_msg, buf, count );
   buf_msg[ count ] = '\0';
   return count;
}

static unsigned long get_fibonachi(unsigned long place)
{
       unsigned long old_value = 0;
	unsigned long value = 1;
	unsigned long hold;
	unsigned long n;
	if(value < 1) return (0);
	for (n = 1; n < place; n++)
	{
		hold = value;
		value += old_value;
		old_value = hold;
	}
	return (value);
}


CLASS_ATTR_RW( xxx );

static struct class *x_class;

void IRQ_TimerHandler(unsigned long data)
{
	char buf[20];
	time_interval ++;
	sprintf(buf,"%u %llu\n", time_interval, get_fibonachi(time_interval));
	xxx_store(x_class, &class_attr_xxx, buf, strlen(buf)+1);
	irq_timer.expires += DELAY;
	add_timer(&irq_timer);
}


int __init x_init(void) {
   int res;   
   buf_msg = kmalloc(LEN_MSG+1,GFP_KERNEL | __GFP_ZERO);   
   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( "bad class create\n" );
   res = class_create_file( x_class, &class_attr_xxx );
   time_interval = 0;
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
