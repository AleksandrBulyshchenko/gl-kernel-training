#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/spinlock.h>
/**
 * https://kernel.readthedocs.io/en/sphinx-samples/kernel-locking.html
 * Documentation/locking/spinlocks.txt
 */
MODULE_LICENSE( "GPL" );
MODULE_AUTHOR("Oleksii Kogutenko");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel training: 05 - Internal Kernel API (Time Management)");

static u64				last_sys_time = 0;			//jiff counter
static struct timeval	last_abs_time = { 0, 0 };	//abs time

#define FIBO_ARRAY_SIZE 3
struct t_fibo_struct {
	u64					data[FIBO_ARRAY_SIZE];	// Data used for fibo calculate
	int					cur;					// Current data pointer
	int					iteration;				// Number of current iteration of fibo calculation
	struct timer_list	timer;					// timer structure
	spinlock_t			lock;					// Spin lock to use in timer
};
#define FIBO_STRUCT_INIT {\
	.data = {0,1,1}, \
	.cur = 0, \
	.iteration = 0 \
}

static struct t_fibo_struct fibo_next  = FIBO_STRUCT_INIT;
static struct t_fibo_struct fibo_timer = FIBO_STRUCT_INIT;

#define TIMER_FIBO_PERIOD	(1 * HZ) /* 1 sec */

/* ----------------------------------------------------------------------------------------
 * Show jiff time (in seconds) passed since previous read of it.
 * /sys/class/tm/jiff
 * ----------------------------------------------------------------------------------------
 */
static ssize_t tm_jiff_show( struct class *class, struct class_attribute *attr, char *buf ) 
{
	size_t count = 0;

	u64 sys_time = get_jiffies_64();
	unsigned int seconds = 0;

	if (last_sys_time) {
		u64 diff = sys_time - last_sys_time;
		seconds = jiffies_to_msecs( diff ) / MSEC_PER_SEC;
	}

	sprintf( buf, "Time elapsed since last read: %u seconds\n", seconds );

	count = strlen( buf );
	last_sys_time = sys_time;

	pr_info( "read %ld\n", (long)count );

	return count;
}
/* ----------------------------------------------------------------------------------------
 * Show absolute time of previous reading
 * /sys/class/tm/abs
 * ----------------------------------------------------------------------------------------
 */
static ssize_t tm_abs_show( struct class *class, struct class_attribute *attr, char *buf ) 
{
	size_t count = 0;

	struct timeval tv = { 0, 0 };
	struct timeval tv_diff = { 0, 0 };

	do_gettimeofday( &tv );

	if (last_abs_time.tv_sec) {
		s64 ns_current = timeval_to_ns( &tv );
		s64 ns_last	  = timeval_to_ns( &last_abs_time );
		s64 diff = ns_current - ns_last;
		tv_diff = ns_to_timeval( diff );
	}

	sprintf( buf, "Time elapsed since last read: %llu.%u seconds \n",
		(unsigned long long)tv_diff.tv_sec, (unsigned)tv_diff.tv_usec );

	count = strlen( buf );
	last_abs_time = tv;

	pr_info( "read %ld\n", (long)count );

	return count;
}
/* ----------------------------------------------------------------------------------------
 * Update fibo structure
 * Unsafe function
 * ----------------------------------------------------------------------------------------
 */
static u64 update_fibo(struct t_fibo_struct *fibo)
{
	int fibo_i1 = (fibo->cur > 0) ? (fibo->cur - 1) : FIBO_ARRAY_SIZE - 1;
	int fibo_i2 = (fibo_i1 > 0) ? (fibo_i1 - 1) : FIBO_ARRAY_SIZE - 1;
	u64 fibo_val = (fibo->iteration < FIBO_ARRAY_SIZE) ? fibo->data[fibo->iteration] : fibo->data[fibo_i1] + fibo->data[fibo_i2];

	if (fibo->iteration >= FIBO_ARRAY_SIZE) fibo->data[fibo->cur] = fibo_val;

	fibo->cur = (fibo->cur + 1) % FIBO_ARRAY_SIZE;
	fibo->iteration++;

	return fibo_val;
}
/* ----------------------------------------------------------------------------------------
 * Get current fibo value
 * Unsafe function
 * ----------------------------------------------------------------------------------------
 */
static u64 get_fibo_current_value(struct t_fibo_struct *fibo)
{
	int fibo_i1 = (fibo->cur > 0) ? (fibo->cur - 1) : FIBO_ARRAY_SIZE - 1;
	u64 fibo_val = (fibo->iteration < FIBO_ARRAY_SIZE) ? fibo->data[fibo->iteration] : fibo->data[fibo_i1];

	return fibo_val;
}
/* ----------------------------------------------------------------------------------------
 * Show next fibo value since previous read of it
 * /sys/class/tm/fibo
 * ----------------------------------------------------------------------------------------
 */
static ssize_t tm_fibo_show( struct class *class, struct class_attribute *attr, char *buf ) 
{
	size_t count = 0;
	u64 fibo_val;

	fibo_val = update_fibo(&fibo_next);

	sprintf( buf, "Fibo value is %llu {cur %d, {%llu, %llu, %llu}}\n", fibo_val, fibo_next.cur, fibo_next.data[0], fibo_next.data[1], fibo_next.data[2] );

	count = strlen( buf );
	pr_info( "read %ld\n", (long)count );

	return count;
}
/* ----------------------------------------------------------------------------------------
 * Show fibo value from that increasing in the timer irq
 * /sys/class/tm/fibo_timer
 * ----------------------------------------------------------------------------------------
 */
static ssize_t tm_fibo_timer_show( struct class *class, struct class_attribute *attr, char *buf ) 
{
	size_t count = 0;
	unsigned long flags;
	u64 fibo_val;
	u64 iterations;

	spin_lock_irqsave(&fibo_timer.lock, flags);

	fibo_val = get_fibo_current_value(&fibo_timer);
	iterations = fibo_timer.iteration;

	spin_unlock_irqrestore(&fibo_timer.lock, flags);

	sprintf( buf, "Fibo value is %llu (iterations: %llu)\n", fibo_val, iterations);

	count = strlen( buf );
	pr_info( "read %ld\n", (long)count );

	return count;
}
/* ----------------------------------------------------------------------------------------
 * timer callback
 * ----------------------------------------------------------------------------------------
 */
void fibo_timer_callback(unsigned long data)
{
	struct t_fibo_struct *fibo = (struct t_fibo_struct*) data;
	unsigned long flags; 
	u64 jiff, diff;

	spin_lock_irqsave(&fibo->lock, flags);

	update_fibo(&fibo_timer);

	jiff = get_jiffies_64();
	diff = (jiff - fibo->timer.expires) + TIMER_FIBO_PERIOD + jiff;

	mod_timer(&fibo->timer, diff);

	spin_unlock_irqrestore(&fibo->lock, flags);
}
/* ----------------------------------------------------------------------------------------
 *
 * ----------------------------------------------------------------------------------------
 */
static int init_fibo_timer(struct t_fibo_struct *fibo)
{
	int ret = 0;
	spin_lock_init(&fibo->lock);

	init_timer(&fibo->timer);

	setup_timer(&fibo->timer, fibo_timer_callback, (unsigned long)fibo);

	ret = mod_timer(&fibo->timer, get_jiffies_64() + TIMER_FIBO_PERIOD);

	return ret;
}
/* ----------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------
 */
static void deinit_fibo_timer(struct t_fibo_struct *fibo)
{
	del_timer(&fibo->timer);
}
// ----------------------------------------------------------------------------------------
#define CLASS__ATTR_RO(_name, _data) {						\
	.attr	= { .name = __stringify(_data), .mode = S_IRUGO },	\
	.show	= _name##_##_data##_show,						\
}

#define CLASS(name) \
	static struct class* name##_class;

#define CLASS_DATA_RO(name, data) \
	struct class_attribute class_attr_##name##data = CLASS__ATTR_RO( name, data );

#define CREATE_CLASS(name, goto_err) \
	({ \
		name##_class = class_create( THIS_MODULE, #name ); \
		if( IS_ERR( name##_class ) ) { \
			pr_err( "Bad class create "#name"\n" ); \
			goto goto_err; \
		} \
	})

#define CREATE_FILE(name, data, goto_err) \
	({ \
		int res = 0; \
		res = class_create_file( name##_class, &class_attr_##name##data ); \
		if (res) goto goto_err; \
		res; \
	})

#define REMOVE_FILE(name, data) \
	class_remove_file( name##_class, &class_attr_##name##data );

#define REMOVE_CLASS(name) \
	class_destroy( name##_class );
// ----------------------------------------------------------------------------------------
CLASS(tm);
CLASS_DATA_RO(tm, jiff);
CLASS_DATA_RO(tm, abs);
CLASS_DATA_RO(tm, fibo);
CLASS_DATA_RO(tm, fibo_timer);
/* ----------------------------------------------------------------------------------------
 * Init module function
 * ----------------------------------------------------------------------------------------
 */
int __init tm_init(void) {
	int res;

	CREATE_CLASS(tm, create_class_err);
	res  = CREATE_FILE(tm, jiff, jiff_err);
	res |= CREATE_FILE(tm, abs,  abs_err);
	res |= CREATE_FILE(tm, fibo, fibo_err);
	res |= CREATE_FILE(tm, fibo_timer, fibo_timer_err);
	if (init_fibo_timer(&fibo_timer)) goto fibo_timer_init_error;
   
	pr_info( "'tm' module initialized %d\n", res );
	return res;
fibo_timer_init_error:
	pr_err("fibo timer init error!\n");
	deinit_fibo_timer(&fibo_timer);
fibo_timer_err:
	REMOVE_FILE(tm, fibo_timer);
fibo_err:
	REMOVE_FILE(tm, fibo);
abs_err:
	REMOVE_FILE(tm, abs);
jiff_err:
	REMOVE_FILE(tm, jiff);
create_class_err:
	REMOVE_CLASS(tm);
	return res;
}
/* ----------------------------------------------------------------------------------------
 * Exit module function
 * ----------------------------------------------------------------------------------------
 */
void tm_exit(void) {
	deinit_fibo_timer(&fibo_timer);

	REMOVE_FILE(tm, fibo_timer);
	REMOVE_FILE(tm, fibo);
	REMOVE_FILE(tm, abs);
	REMOVE_FILE(tm, jiff);
 
	REMOVE_CLASS(tm);
   return;
}
// ----------------------------------------------------------------------------------------
module_init( tm_init );
module_exit( tm_exit );

