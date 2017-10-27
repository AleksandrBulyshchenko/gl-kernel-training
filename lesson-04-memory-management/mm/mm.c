#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>
//#include <linux/gfp.h>
//#include <linux/sysfs.h>

#define MEM_TYPE_KMALLOC    0
#define MEM_TYPE_KCACHE     1
#define MEM_TYPE_KVMALLOC   2

#define BUFFER_SIZE         160

static size_t   buf_size = 0;
static char     *pbuf_msg = 0;
static int      mem_type = MEM_TYPE_KMALLOC;
struct KCACHE_STRUCT {
    char buf[BUFFER_SIZE];
};

static struct kmem_cache* pmem_cache = NULL;

const char* get_mem_type(int type)
{
    switch (type) {
        case MEM_TYPE_KMALLOC:
            return "MEM_TYPE_KMALLOC";
        case MEM_TYPE_KCACHE:
            return "MEM_TYPE_KCACHE";
        case MEM_TYPE_KVMALLOC:
            return "MEM_TYPE_KVMALLOC";
    }
    return "UNKNOWN";
}

static void* mem_alloc(int type, size_t size)
{
    void* buf = NULL;
    switch (type) {
        case MEM_TYPE_KMALLOC:
            buf = kmalloc(size, GFP_KERNEL);
            break;
        case MEM_TYPE_KCACHE:
            buf = kmem_cache_alloc( pmem_cache, GFP_KERNEL );
            break;
        case MEM_TYPE_KVMALLOC:
            buf = vmalloc(size);
            break;
    }

    if (buf) buf_size = size;

    printk("[%s:%s] {%s} %zu bytes --> %p\n", THIS_MODULE->name, __FUNCTION__, get_mem_type(type), size, buf);
    return buf;
}

static void mem_free(int type, void* buf)
{
    if (!buf) {
        printk("[%s:%s] Wrong parameter %p\n", THIS_MODULE->name, __FUNCTION__, buf);
        return;
    }
    switch (type) {
        case MEM_TYPE_KMALLOC:
            kfree(buf);
            break;
        case MEM_TYPE_KCACHE:
            kmem_cache_free( pmem_cache, buf );
            buf = kmem_cache_alloc( pmem_cache, GFP_KERNEL );
            break;
        case MEM_TYPE_KVMALLOC:
            vfree(buf);
            break;
    }
    printk("[%s:%s] {%s} bytes --> %p\n", THIS_MODULE->name, __FUNCTION__, get_mem_type(type), buf);
}

void init_kcache(void) 
{
    if (mem_type != MEM_TYPE_KCACHE) return;
/*
 * 140 * Please use this macro to create slab caches. Simply specify the
 * 141 * name of the structure and maybe some flags that are listed above.
 * 142 *
 * 143 * The alignment of the struct determines object alignment. If you
 * 144 * f.e. add ____cacheline_aligned_in_smp to the struct declaration
 * 145 * then the objects will be properly aligned in SMP configurations.
 * 146
 * #define KMEM_CACHE(__struct, __flags) kmem_cache_create(#__struct,\
 * 148                sizeof(struct __struct), __alignof__(struct __struct),\
 * 149                (__flags), NULL)
 */
    pmem_cache = KMEM_CACHE(KCACHE_STRUCT, 0);
    printk("[%s:%s] pmem_cache: %p\n", THIS_MODULE->name, __FUNCTION__, pmem_cache);
}

void deinit_kcache(void)
{
    if (mem_type != MEM_TYPE_KCACHE) {
        mem_free(mem_type, pbuf_msg);
        return;
    }
    kmem_cache_destroy( pmem_cache );
}

static ssize_t mm_show( struct class *class, struct class_attribute *attr, char *buf ) 
{
    if (!pbuf_msg) {
        printk( "[%s:%s] nothing to read\n", THIS_MODULE->name, __FUNCTION__);
        return 0; 
    }
    strcpy( buf, pbuf_msg );
    printk( "[%s:%s] read %ld\n", THIS_MODULE->name, __FUNCTION__, (long)strlen( buf ) );
    return strlen( buf );
}

static ssize_t mm_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) 
{
    if (!pbuf_msg) {
        pbuf_msg = mem_alloc(mem_type, BUFFER_SIZE);
        if (!pbuf_msg) {
           printk( "[%s:%s] Wrong alloc buffer! \n", THIS_MODULE->name, __FUNCTION__);
           return 0;
        }
    }
    if (count >= BUFFER_SIZE) {
        printk( "[%s:%s]  Wrong buffer size %lu\n", THIS_MODULE->name, __FUNCTION__, (long)count );
        return 0;
    }
    printk( "[%s:%s] write %ld\n", THIS_MODULE->name, __FUNCTION__, (long)count );
    strncpy( pbuf_msg, buf, count );
    pbuf_msg[ count ] = '\0';
    return count;
}

CLASS_ATTR_RW( mm );

static struct class *mm_class;


int __init mm_init(void) {
   int res;
   printk("mem_type is %d\n", mem_type);
   mm_class = class_create( THIS_MODULE, "mm-class" );
   if( IS_ERR( mm_class ) ) printk( "bad class create\n" );
   res = class_create_file( mm_class, &class_attr_mm );
   printk( "'mm' module initialized\n" );
   init_kcache();
   return res;
}

void mm_cleanup(void) {
   class_remove_file( mm_class, &class_attr_mm );
   class_destroy( mm_class );
   deinit_kcache();
   return;
}

/**
 * 137 * module_param_named - typesafe helper for a renamed module/cmdline parameter
 * 138 * @name: a valid C identifier which is the parameter name.
 * 139 * @value: the actual lvalue to alter.
 * 140 * @type: the type of the parameter
 * 141 * @perm: visibility in sysfs.
 * 142 *
 * 143 * Usually it's a good idea to have variable names and user-exposed names the
 * 144 * same, but that's harder if the variable must be non-static or is inside a
 * 145 * structure.  This allows exposure under a different name.
 * 146 */
module_param( mem_type, int, 0444 );
MODULE_PARM_DESC(mem_type, "Type of memory allocation: 0 - kmalloc; 1 - cache; 2 - vmalloc");

module_init( mm_init );
module_exit( mm_cleanup );

MODULE_LICENSE( "GPL" );
