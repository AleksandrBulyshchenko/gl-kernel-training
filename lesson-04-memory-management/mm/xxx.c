#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/sysfs.h>

static size_t g_buf_size = 0;
static char* g_buf_msg   = 0;

#define MEM_CONFIG_KMALLOC 0
#define MEM_CONFIG_KMCACHE 1

#define CACHE_NAME "xxx_cache"
#define CACHE_SIZE 160
static struct kmem_cache* g_cache = NULL;

static void cache_constructor( void* p )
{
   printk( "%s constructs %p(%u)", THIS_MODULE->name, p, (p ? *(char*)p : 0) );
}

static int g_mem_config = MEM_CONFIG_KMALLOC;
module_param_named( mem_config, g_mem_config, int, 0 );


static void initialize_memory( void )
{
   if (g_mem_config != MEM_CONFIG_KMCACHE)
      return;

   g_cache = kmem_cache_create( CACHE_NAME, CACHE_SIZE, 0/*SLAB_HWCACHE_ALIGN*/,
      0/*SLAB_DEBUG_INITIAL*/, &cache_constructor );
   printk( "%s %s cache: %p", THIS_MODULE->name, __FUNCTION__, g_cache );
}

static void finalize_memory( void )
{
   if (g_mem_config != MEM_CONFIG_KMCACHE)
      return;

   if (g_cache)
      kmem_cache_destroy( g_cache );
   g_cache = NULL;
}

static void* allocate_memory( size_t* count )
{
   void* result = NULL;
   if (!*count)
      return result;

   if (g_mem_config == MEM_CONFIG_KMALLOC)
   {
      result = kmalloc( *count, GFP_ATOMIC );
   }
   else if (g_mem_config == MEM_CONFIG_KMCACHE)
   {
      if (g_cache)
      {
         *count = CACHE_SIZE;
         result = kmem_cache_alloc( g_cache, GFP_KERNEL );
      }
      else
      {
         *count = 0;
      }
   }
   printk( "%s %s: %p/%d", THIS_MODULE->name, __FUNCTION__, result, *count );
   return result;
}

static void free_memory( void** buffer )
{
   if (!buffer || !(*buffer))
      return;

   if (g_mem_config == MEM_CONFIG_KMALLOC)
   {
      kfree( *buffer );
      *buffer = NULL;
   }
   else if (g_mem_config == MEM_CONFIG_KMCACHE)
   {
      if (g_cache)
         kmem_cache_free( g_cache, *buffer );
      *buffer = NULL;
   }
}

static size_t construct_buffer( size_t* new_count )
{
   size_t result = *new_count;
   if (!*new_count)
      return result;

   if (*new_count >= g_buf_size)
   {
      if (g_buf_msg)
         free_memory( (void**)&g_buf_msg );
      g_buf_size = *new_count + 1;
      g_buf_msg  = allocate_memory( &g_buf_size );
      if (!g_buf_msg)
      {
         g_buf_size = 0;
         result   = 0;
      }
      else
      {
         result = g_buf_size - 1;
      }
   }
   *new_count = result;
   printk( "%s %s: %p/%d", THIS_MODULE->name, __FUNCTION__, g_buf_msg, *new_count );
   return result;
}

static size_t store_to_buffer( char const* buffer_from, size_t count )
{
   if (construct_buffer( &count ))
   {
      strncpy( g_buf_msg, buffer_from, count );
      g_buf_msg[ count ] = '\0';
   }
   return count;
}

static ssize_t xxx_show(struct class *class, struct class_attribute *attr,
                  char *buf)
{
   size_t count = 0;
   if (g_buf_msg)
   {
      strcpy( buf, g_buf_msg );
      count = strlen( buf );
   }
   printk( "read %ld\n", (long)count );
   return count;
}

static ssize_t xxx_store(struct class *class, struct class_attribute *attr,
                   const char *buf, size_t count)
{
   printk( "write %ld\n", (long)count );
   count = store_to_buffer( buf, count );
   return count;
}

CLASS_ATTR_RW(xxx);

static struct class *x_class;

int __init x_init(void) {
   int res;
   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( "bad class create\n" );
   res = class_create_file( x_class, &class_attr_xxx );
   if (res)
      goto error;

   initialize_memory();
   {
      char const* const initial_buffer = "Hi!\n";
      store_to_buffer( initial_buffer, strlen( initial_buffer ) );
   }

error:
   printk( "'xxx' module initialized %d\n", res );
   return res;
}

void x_cleanup(void) {
   free_memory( (void**)&g_buf_msg );
   finalize_memory();
   class_remove_file( x_class, &class_attr_xxx );
   class_destroy( x_class );
   return;
}

module_init(x_init);
module_exit(x_cleanup);

MODULE_LICENSE("GPL");
