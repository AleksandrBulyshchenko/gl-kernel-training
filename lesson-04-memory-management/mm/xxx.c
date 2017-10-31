#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Chamara");
MODULE_DESCRIPTION("sysfc test module");
MODULE_VERSION("0.1");

//================================================================
#define MODULE_TAG      "xxx_module --> "
#define LEN_MSG 32
#define TIMER_PERIOD_MSEC 1000

//================================================================
static char *buf_msg;
static struct class *x_class;
static struct timer_list timer;
static unsigned int fib_val;

//================================================================
static void reload_timer(void)
{
	int err;
	err = mod_timer( &timer, jiffies + msecs_to_jiffies(TIMER_PERIOD_MSEC) );
	if (err)
		printk( MODULE_TAG "Timer modification error\n" );
}

static void timer_func (unsigned long parameter)
{
	static unsigned int iter;
	unsigned int i = 1;
	unsigned int x = 1;
	unsigned int y = 0;

	iter++;
	for (i = 1; i < iter; i++)
	{
		x += y;
		y = x - y;
	}
	fib_val = x;

	reload_timer();
}

static void timer_init(void)
{
	setup_timer( &timer, timer_func, 0 );
	reload_timer();
}

static int create_buffer(void)
{
	buf_msg = kmalloc( LEN_MSG, GFP_KERNEL );
    if ( NULL == buf_msg )
        return -ENOMEM;
    memset( buf_msg, 0, LEN_MSG );
    return 0;
}


static void cleanup_buffer(void)
{
    if ( buf_msg ) {
        kfree(buf_msg);
        buf_msg = NULL;
    }
}

static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
	static unsigned long long read_time_prev;
	static unsigned char first_call = true;
	unsigned long long read_time_curr = jiffies/1000;
	struct timeval absolute_time;

	do_gettimeofday(&absolute_time);

	snprintf(buf_msg, LEN_MSG, "%d\n", fib_val);
	strcpy( buf, buf_msg );
	if(first_call){
		printk( "%ld.%ld:" MODULE_TAG "read %ld\n", absolute_time.tv_sec,
													absolute_time.tv_usec,
													(long)strlen( buf ));
		first_call = false;
	}
	else{
		printk( "%ld.%ld:" MODULE_TAG "read %ld. Previous read was %llu sec ago\n", absolute_time.tv_sec,
																					absolute_time.tv_usec,
																					(long)strlen( buf ),
																					read_time_curr - read_time_prev);
	}

	read_time_prev = read_time_curr;
	return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {

	size_t byte_number = (count < LEN_MSG) ? count : LEN_MSG;
	struct timeval absolute_time;

	do_gettimeofday( &absolute_time );

	printk( "%ld.%ld:" MODULE_TAG "write %ld\n", absolute_time.tv_sec,
												 absolute_time.tv_usec,
												 (long)byte_number );
	strncpy( buf_msg, buf, byte_number );
	buf_msg[ byte_number ] = '\0';
	return byte_number;
}

CLASS_ATTR_RW( xxx );

//================================================================
int __init x_init(void) {
   int err;

   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ){
	   err = PTR_ERR( x_class );
	   printk( MODULE_TAG "File creation error\n");
	   goto class_create_err;
   }

   err = class_create_file( x_class, &class_attr_xxx );
   if( err ){
	   printk( MODULE_TAG "Class creation error\n");
	   goto file_create_err;
   }

    err = create_buffer();
	if( err ){
		printk( MODULE_TAG "Memory allocation error\n" );
		goto mem_aloc_err;
	}


	timer_init();
	printk( MODULE_TAG "Module initialized successfully\n" );
	return 0;

mem_aloc_err:
	// do nothing here

file_create_err:
	class_remove_file( x_class, &class_attr_xxx );

class_create_err:
	class_destroy( x_class );
    return err;
}

void x_cleanup(void) {
	cleanup_buffer();
	class_remove_file( x_class, &class_attr_xxx );
	class_destroy( x_class );
	del_timer( &timer );
	return;
}

//================================================================
module_init( x_init );
module_exit( x_cleanup );

