#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>

enum {
  MODE_DEFAULT,
  MODE_STATIC = MODE_DEFAULT,
  MODE_KMALLOC,
  MODE_KMEM_CACHE,
  MODE_MAX
};

static int mode = 0; 
module_param( mode, int, S_IRUGO );
MODULE_PARM_DESC( mode, "mode of memory allocation: 0 (default) = static, 1 = kmalloc, 2 = kmem_cache" );
static int persistent_mode = 0;

#define LEN_MSG (160 + 1)
static char static_buf_msg[ LEN_MSG ] = "Hello from module!\n";
static char *buf_msg;

#define SLABNAME "xxx_slab"
static struct kmem_cache *cache = NULL;


static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
   strcpy( buf, buf_msg );
   printk( KERN_INFO "%s: read %ld\n", THIS_MODULE->name, (long)strlen( buf ) );
   return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
   printk( KERN_INFO "%s: write %ld\n", THIS_MODULE->name, (long)count );
   strncpy( buf_msg, buf, count );
   buf_msg[ count ] = '\0';
   return count;
}

CLASS_ATTR_RW( xxx );

static struct class *x_class;

static void print_mode( int mode )
{
  switch ( mode ) {
    case MODE_STATIC:     printk( KERN_INFO "%s: mode: static\n", THIS_MODULE->name );      break;
    case MODE_KMALLOC:    printk( KERN_INFO "%s: mode: kmalloc\n", THIS_MODULE->name );     break;
    case MODE_KMEM_CACHE: printk( KERN_INFO "%s: mode: kmem_cache\n", THIS_MODULE->name );  break;
    default:              printk( KERN_ERR  "%s: mode: unsupported\n", THIS_MODULE->name ); break;
  }
}

static char *alloc_buf_msg( void )
{
  char *ret = NULL;

  switch ( persistent_mode ) {
    case MODE_STATIC: {
      ret = static_buf_msg;
      printk( KERN_INFO "%s: providing static buffer (ret = %p)\n", THIS_MODULE->name, ret );
      break;
    }
    case MODE_KMALLOC: {
      ret = kmalloc( LEN_MSG, GFP_KERNEL );
      printk( KERN_INFO "%s: providing kmalloc() buffer (ret = %p)\n", THIS_MODULE->name, ret );
      break;
    }
    case MODE_KMEM_CACHE: {
      cache = kmem_cache_create( SLABNAME, LEN_MSG, 0, 0, NULL );
      if ( NULL == cache ) {
        printk( KERN_ERR "%s: kmem_cache_create failed", THIS_MODULE->name );
        break;
      }
      ret = kmem_cache_alloc( cache, GFP_KERNEL );
      printk( KERN_INFO "%s: providing kmem_cache buffer (ret = %p)\n", THIS_MODULE->name, ret );
      break;
    }
    default: {
      printk( KERN_ERR  "%s: mode: unsupported\n", THIS_MODULE->name );
      break;
    }
  }

  return ret;
}

static void free_buf_msg( char *buf )
{
  switch ( persistent_mode ) {
    case MODE_STATIC: {
      /* do nothing */
      printk( KERN_INFO "%s: releasing static buffer (buf = %p)\n", THIS_MODULE->name, buf );
      break;
    }
    case MODE_KMALLOC: {
      printk( KERN_INFO "%s: releasing kmalloc() buffer (buf = %p)\n", THIS_MODULE->name, buf );
      kfree( buf );
      break;
    }
    case MODE_KMEM_CACHE: {
      BUG_ON( NULL == cache );

      printk( KERN_INFO "%s: releasing kmem_cache() buffer (buf = %p)\n", THIS_MODULE->name, buf );
      kmem_cache_free( cache, buf );
      kmem_cache_destroy( cache );
      break;
    }
    default: {
      printk( KERN_ERR  "%s: mode: unsupported\n", THIS_MODULE->name );
      break;
    }
  }
}

int __init x_init(void) {
   int res;

   persistent_mode = (( mode >= MODE_DEFAULT ) && ( mode < MODE_MAX )) ? mode : MODE_DEFAULT; /* don't want to handle run-time changes of 'mode' in /sys/module/THIS_MODULE/parmeters */
   print_mode( persistent_mode );
   buf_msg = alloc_buf_msg();
   if ( NULL == buf_msg ) {
     res = -ENOMEM;
     goto err;
   }

   strcpy( buf_msg, static_buf_msg );

   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( KERN_ERR "%s: bad class create\n", THIS_MODULE->name );
   res = class_create_file( x_class, &class_attr_xxx );
   printk( KERN_INFO "%s: module initialized\n", THIS_MODULE->name );

err:
   return res;
}

void x_cleanup(void) {
   class_remove_file( x_class, &class_attr_xxx );
   class_destroy( x_class );
   free_buf_msg ( buf_msg );
   printk( KERN_INFO "%s: module done\n", THIS_MODULE->name );
   return;
}

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
