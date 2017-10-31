#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR("Oleksii Kogutenko");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel training: 05 - Internal Kernel API (Time Management)");

static u64 last_sys_time = 0;
static struct timeval last_abs_time = { 0, 0 };

#define FIBO_ARRAY_SIZE 3
struct t_fibo_struct {
    u64 data[FIBO_ARRAY_SIZE];
    int cur, iteration;
};

static struct t_fibo_struct fibo_next = {
    .data = {0,1,1},
    .cur = 0,
    .iteration = 0
};

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
       (unsigned long long)tv_diff.tv_sec, (unsigned)tv_diff.tv_usec );

    count = strlen( buf );
    last_abs_time = tv;

    printk( "read %ld\n", (long)count );

    return count;
}

static ssize_t tm_fibo_show( struct class *class, struct class_attribute *attr, char *buf ) {
    size_t count = 0;

    int fibo_i1 = (fibo_next.cur > 0) ? (fibo_next.cur - 1) : FIBO_ARRAY_SIZE - 1;
    int fibo_i2 = (fibo_i1 > 0) ? (fibo_i1 - 1) : FIBO_ARRAY_SIZE - 1;
    u64 fibo_val = (fibo_next.iteration < FIBO_ARRAY_SIZE) ? fibo_next.data[fibo_next.iteration] : fibo_next.data[fibo_i1] + fibo_next.data[fibo_i2];
    
    if (fibo_next.iteration >= FIBO_ARRAY_SIZE) fibo_next.data[fibo_next.cur] = fibo_val;
    
    fibo_next.cur = (fibo_next.cur + 1) % FIBO_ARRAY_SIZE;
    fibo_next.iteration++;

    sprintf( buf, "Fibo value is %llu {cur %d, {%llu, %llu, %llu}}\n", fibo_val, fibo_next.cur, fibo_next.data[0], fibo_next.data[1], fibo_next.data[2] );

    count = strlen( buf );
    printk( "read %ld\n", (long)count );

    return count;
}


#define CLASS__ATTR_RO(_name, _data) {						\
	.attr	= { .name = __stringify(_data), .mode = S_IRUGO },	\
	.show	= _name##_##_data##_show,						\
}

#define CLASS(name) \
    static struct class* name##_class;

#define CLASS_DATA_RO(name, data) \
    struct class_attribute class_attr_##name##data = CLASS__ATTR_RO( name, data ); \

#define CREATE_CLASS(name, goto_err) \
    ({ \
        name##_class = class_create( THIS_MODULE, #name ); \
        if( IS_ERR( name##_class ) ) { \
            printk( "Bad class create "#name"\n" ); \
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
    class_remove_file( name##_class, &class_attr_##name##data ); \

#define REMOVE_CLASS(name) \
    class_destroy( name##_class );

CLASS(tm);
CLASS_DATA_RO(tm, jiff);
CLASS_DATA_RO(tm, abs);
CLASS_DATA_RO(tm, fibo);

int __init tm_init(void) {
    int res;

    CREATE_CLASS(tm, create_class_err);
    res  = CREATE_FILE(tm, jiff, jiff_err);
    res |= CREATE_FILE(tm, abs,  abs_err);
    res |= CREATE_FILE(tm, fibo, fibo_err);
   
    printk( "'tm' module initialized %d\n", res );
    return res;
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
 
void tm_cleanup(void) {
    REMOVE_FILE(tm, fibo);
    REMOVE_FILE(tm, abs);
    REMOVE_FILE(tm, jiff);
 
    REMOVE_CLASS(tm);
   return;
}
 
module_init( tm_init );
module_exit( tm_cleanup );
 

