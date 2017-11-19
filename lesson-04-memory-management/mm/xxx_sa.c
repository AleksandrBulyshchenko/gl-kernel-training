#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>

#define LEN_MSG 160
static char *buf_msg;
static const char init_buf_msg[] = "Hello from module!\n";

struct kmem_cache* user_cache = NULL;

static ssize_t xxx_show( struct class *class, struct class_attribute *attr, char *buf ) {
   strcpy( buf, buf_msg );
   printk( "read %ld\n", (long)strlen( buf ) );
   return strlen( buf );
}

static ssize_t xxx_store( struct class *class, struct class_attribute *attr, const char *buf, size_t count ) {
   printk( "write %ld\n", (long)count );
   strncpy( buf_msg, buf, count );
   buf_msg[ count ] = '\0';
   return count;
}

CLASS_ATTR_RW( xxx );

static struct class *x_class;

int __init x_init(void) {
   int res;
   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( "bad class create\n" );
   res = class_create_file( x_class, &class_attr_xxx );

   user_cache = kmem_cache_create("user_cache", LEN_MSG, 0, 0, NULL); 

   if(user_cache == NULL)
   {
   	   return -ENOMEM;
   }

   buf_msg = (char*) kmem_cache_alloc(user_cache, 0);
   strcpy(buf_msg, init_buf_msg);

   printk( "'xxx' module initialized\n" );
   return res;
}

void x_cleanup(void) {
   class_remove_file( x_class, &class_attr_xxx );
   class_destroy( x_class );
   kmem_cache_free(user_cache, buf_msg);
   kmem_cache_destroy(user_cache);
   return;
}

module_init( x_init );
module_exit( x_cleanup );

MODULE_LICENSE( "GPL" );
