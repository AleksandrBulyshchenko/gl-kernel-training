#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>


#define LEN_MSG 160
static unsigned int fib_n;
static char* buf_msg;
static struct timeval* tv;
static ktime_t tout;
static struct kt_data 
{
	struct hrtimer timer;
	ktime_t        period;
} *data;


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
	printk( "read %ld\n", (long)strlen( buf ) );
	return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
	strncpy( buf_msg, buf, count );
	buf_msg[ count ] = '\0';
	return count;
}

CLASS_ATTR_RW( xxx );

static struct class *x_class;


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static int ktfun( struct hrtimer *var ) 
{
#else
static enum hrtimer_restart ktfun( struct hrtimer *var ) 
{
#endif
	char buf[100];
	ktime_t now = var->base->get_time();      // текущее время в типе ktime_t
	sprintf(buf,"%u %llu\n",fib_n,fib(fib_n));
	xxx_store(x_class, &class_attr_xxx,buf,strlen(buf)+1);
	if (fib_n < 93) fib_n++;
	else fib_n = 0;
	hrtimer_forward( var, now, tout );
	return HRTIMER_RESTART;
}
	

int __init x_init(void) 
{
	fib_n = 0;
	enum hrtimer_mode mode;
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
	mode = HRTIMER_REL;
	#else
	mode = HRTIMER_MODE_REL;
	#endif
	tout = ktime_set( 1, 0 );      /* 1 sec. + 0 nsec. */
	data = kmalloc( sizeof(*data), GFP_KERNEL );
	data->period = tout;
	hrtimer_init( &data->timer, CLOCK_REALTIME, mode );
	data->timer.function = ktfun;
	printk( KERN_INFO "timer start at jiffies=%ld\n", jiffies );
	hrtimer_start( &data->timer, data->period, mode );
	int res;
	tv = kmalloc(sizeof(*tv),GFP_KERNEL);
	buf_msg = kmalloc(LEN_MSG+1,GFP_KERNEL | __GFP_ZERO);
	tv->tv_sec = 0;
	tv->tv_usec = 0;
	x_class = class_create( THIS_MODULE, "x-class" );
	if( IS_ERR( x_class ) ) printk( "bad class create\n" );
	res = class_create_file( x_class, &class_attr_xxx );
	printk( "'xxx' module initialized\n" );
	return res;
}

void x_cleanup(void) {
	class_remove_file( x_class, &class_attr_xxx );
	class_destroy( x_class );
	hrtimer_cancel( &data->timer );
	kfree( data );
	kfree(buf_msg);
	kfree(tv);
	return;
}

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
