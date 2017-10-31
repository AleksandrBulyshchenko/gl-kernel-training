#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/delay.h>

// Show interval from prev read
static uint64_t last_time;

static char* print_interval( char* buf )
{
	uint64_t lt = last_time;
	last_time = get_jiffies_64();
	if (lt == 0)
		strcpy( buf, "first time interval\n" );
	else
		sprintf( buf, "time from last interval: %llu sec\n", div64_u64(last_time - lt , HZ ) );

	return buf;
}

static ssize_t interval_show( struct class *class, struct class_attribute *attr, char *buf )
{
	print_interval( buf );
	printk( "%s", buf );
	return strlen( buf );
}

// Show Absolute time for prev read
static time64_t prev_read_time;

char* print_abstime( char* buf )
{
  struct timespec64 ts = current_kernel_time64();
  struct tm tm = {0};
  time64_t prt = prev_read_time;
  prev_read_time = ts.tv_sec;
  if (!prt)
  {
    sprintf( buf, "this is first read!\n" );
  }
  else
  {
    time64_to_tm( prt, 0, &tm );
    sprintf( buf, "%04ld/%02d/%02d %02d:%02d:%02d\n", 1900 + tm.tm_year, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec );
  }
  return buf;
}

static ssize_t absolute_show( struct class *class, struct class_attribute *attr, char *buf )
{
	print_abstime( buf );
	printk( "read time %s\n", buf );
	return strlen( buf );
}

//Show last 3 digit from fibonacci calculated one per second
#define DEPTH 4
#define DMASK 3
static int64_t n1, n2;
static int64_t n[DEPTH];
static volatile int head, is_started;

static void fib_handler( unsigned long );
DEFINE_TIMER( mytimer, fib_handler, 0, 0 );

static void start_timer( void )
{
	mod_timer( &mytimer, jiffies + HZ );
}

void fib_handler( unsigned long data )
{
	if (is_started)
	{
		int t = n2;
		n2 = n1 + n2;
		if (n2 < 0) //is overflow, beginning
		{
			n1 = 0; n2 = 1;
		}
		else
			n1 = t;

		n[(head + 1) & DMASK] = n2;
		head += 1;

		start_timer();
	}
}

static ssize_t fibonacci_show( struct class *class, struct class_attribute *attr, char *buf )
{
	int i = head;
	sprintf( buf, "fibonacci: %lld, %lld, %lld\n", n[(i - 2) & DMASK], n[(i - 1) & DMASK], n[(i - 0) & DMASK] );
	printk( "read %s", buf );
	return strlen( buf );
}

static ssize_t fibonacci_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count )
{
	if (is_started)
	{   // simple sync
		is_started = 0;
		msleep( 1000 );
	}
	head = 0;
	memset(n, 0, DEPTH);
	n1 = 0;
	n2 = 1;
	is_started = 1;
	start_timer();
	return count;
}

CLASS_ATTR_RO( interval );
CLASS_ATTR_RO( absolute );
CLASS_ATTR_RW( fibonacci );

static struct class *tman_class;

int __init tman_init( void )
{
	int res;
	tman_class = class_create( THIS_MODULE, "tman-class" );
	if( IS_ERR( tman_class ) ) printk( "bad class create\n" );
	res = class_create_file( tman_class, &class_attr_interval );
	res |= class_create_file( tman_class, &class_attr_absolute );
	res |= class_create_file( tman_class, &class_attr_fibonacci );
	printk( "'tman' module initialized\n" );
	return res;
}

void tmam_cleanup( void ) {
	del_timer( &mytimer );
	class_remove_file( tman_class, &class_attr_fibonacci );
	class_remove_file( tman_class, &class_attr_absolute );
	class_remove_file( tman_class, &class_attr_interval );
	class_destroy( tman_class );
	return;
}

module_init( tman_init );
module_exit( tmam_cleanup );

MODULE_LICENSE( "GPL" );
