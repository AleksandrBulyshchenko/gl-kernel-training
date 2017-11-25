#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>

#define BUF_SIZE_MAX 256

static enum { ALLOC_TYPE_KMALLOC = 0, ALLOC_TYPE_KVMALLOC, ALLOC_TYPE_KCACHE} alloc_type = ALLOC_TYPE_KMALLOC;
static struct kmem_cache* pmem_cache = 0;
static size_t buf_size = 0;
struct KCACHE_STRUCT {
    char buf[BUF_SIZE_MAX];
};

static char* buf_msg = 0;

const char* get_alloc_type(int type)
{
    switch (type) {
        case ALLOC_TYPE_KMALLOC:
            return "ALLOC_TYPE_KMALLOC";
        case ALLOC_TYPE_KVMALLOC:
            return "ALLOC_TYPE_KVMALLOC";
        case ALLOC_TYPE_KCACHE:
            return "ALLOC_TYPE_KCACHE";
    }
    return "ALLOC_TYPE_UNKNOWN";
}

static void* mem_alloc(int type, size_t size)
{
    void* buf = 0;
    switch (type) {
        case ALLOC_TYPE_KMALLOC:
            buf = kmalloc(size, GFP_KERNEL);
            break;
        case ALLOC_TYPE_KVMALLOC:
            buf = vmalloc(size);
            break;
        case ALLOC_TYPE_KCACHE:
            buf = kmem_cache_alloc( pmem_cache, GFP_KERNEL );
            break;
    }

    if (buf) buf_size = size;

    printk(KERN_INFO "[%s:%s] {%s} %zu bytes --> %p\n", THIS_MODULE->name, __FUNCTION__, get_alloc_type(type), size, buf);
    return buf;
}

static void mem_free(int type, void* buf)
{
    if (!buf) {
        printk(KERN_INFO "[%s:%s] Wrong parameter %p\n", THIS_MODULE->name, __FUNCTION__, buf);
        return;
    }
    switch (type) {
       case ALLOC_TYPE_KMALLOC:
            kfree(buf);
            break;
        case ALLOC_TYPE_KVMALLOC:
            vfree(buf);
            break;
        case ALLOC_TYPE_KCACHE:
            kmem_cache_free( pmem_cache, buf );
            break;
   }
    printk(KERN_INFO "[%s:%s] {%s} bytes --> %p\n", THIS_MODULE->name, __FUNCTION__, get_alloc_type(type), buf);
}

void init_kcache(void) 
{
    if (alloc_type != ALLOC_TYPE_KCACHE) return;
    pmem_cache = KMEM_CACHE(KCACHE_STRUCT, 0);
    printk(KERN_INFO "[%s:%s] pmem_cache: %p\n", THIS_MODULE->name, __FUNCTION__, pmem_cache);
}

void deinit_kcache(void)
{
    if (alloc_type != ALLOC_TYPE_KCACHE) {
        mem_free(alloc_type, buf_msg);
        return;
    }
    kmem_cache_destroy( pmem_cache );
}

static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
	if (!buf_msg) {
		printk(KERN_INFO "[%s:%s] Buffer is empty\n", THIS_MODULE->name, __FUNCTION__);
		return 0; 
	}
	strcpy( buf, buf_msg );
	printk(KERN_INFO "[%s:%s] read %ld\n", THIS_MODULE->name, __FUNCTION__, (long)strlen( buf ) );
	return strlen( buf );	
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
	if (buf_msg) {
		mem_free(alloc_type, buf_msg);
		buf_msg = 0;
	}

	if (count >= BUF_SIZE_MAX) {
		printk(KERN_INFO "[%s:%s] Buffer size exceeded! \n", THIS_MODULE->name, __FUNCTION__);
		return 0;
	}
	buf_msg = mem_alloc(alloc_type, count + 1);
	if (!buf_msg) {
		printk(KERN_INFO "[%s:%s] Memory allocation error! \n", THIS_MODULE->name, __FUNCTION__);
		return 0;
	}	
	else {
		printk(KERN_INFO "[%s:%s] write %ld\n", THIS_MODULE->name, __FUNCTION__, (long)count );
		strncpy( buf_msg, buf, count );
		buf_msg[ count ] = '\0';
		return count;
	}
}

CLASS_ATTR_RW( xxx );

static struct class *x_class;

int __init x_init(void) {
	int res;
	printk(KERN_INFO "alloc_type is %s\n", get_alloc_type(alloc_type));
	x_class = class_create( THIS_MODULE, "x-class" );
	if( IS_ERR( x_class ) ) printk(KERN_INFO "bad class create\n" );
	res = class_create_file( x_class, &class_attr_xxx );
	init_kcache();
	printk(KERN_INFO "'xxx' module initialized\n" );
	return res;
}

void x_cleanup(void) {
	class_remove_file( x_class, &class_attr_xxx );
	class_destroy( x_class );
	deinit_kcache();
	printk(KERN_INFO "'xxx' module deinitialized\n" );
	return;
}

module_param( alloc_type, int, 0444 );
MODULE_PARM_DESC(alloc_type, "Type of memory allocation: 0 - kmalloc; 1 - vmalloc; 2 - cache");

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
