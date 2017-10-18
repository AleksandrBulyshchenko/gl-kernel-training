#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>

#define LEN_MSG 160

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) 

#define IOFUNCS( name )                                                         \
static char buf_##name[ LEN_MSG + 1 ] = "не инициализировано "#name"\n";        \
static ssize_t SHOW_##name( struct class *class, struct class_attribute *attr,  \
                            char *buf ) {                                       \
   strcpy( buf, buf_##name );                                                   \
   printk( "read %ld\n", (long)strlen( buf ) );                                 \
   return strlen( buf );                                                        \
}                                                                               \
static ssize_t STORE_##name( struct class *class, struct class_attribute *attr, \
                             const char *buf, size_t count ) {                  \
   printk( "write %ld\n", (long)count );                                        \
   strncpy( buf_##name, buf, count );                                           \
   buf_##name[ count ] = '\0';                                                  \
   return count;                                                                \
}

#else 

#define IOFUNCS( name )                                                         \
static char buf_##name[ LEN_MSG + 1 ] = "не инициализировано "#name"\n";        \
static ssize_t SHOW_##name( struct class *class, char *buf ) {                  \
   strcpy( buf, buf_##name );                                                   \
   printk( "read %ld\n", (long)strlen( buf ) );                                 \
   return strlen( buf );                                                        \
}                                                                               \
static ssize_t STORE_##name( struct class *class, const char *buf,              \
                             size_t count ) {                                   \
   printk( "write %ld\n", (long)count );                                        \
   strncpy( buf_##name, buf, count );                                           \
   buf_##name[ count ] = '\0';                                                  \
   return count;                                                                \
}

#endif 

IOFUNCS( data1 );
IOFUNCS( data2 );
IOFUNCS( data3 );

#define OWN_CLASS_ATTR( name ) \
   struct class_attribute class_attr_##name = \
   __ATTR( name, ( S_IWUSR | S_IRUGO ), &SHOW_##name, &STORE_##name )
// ( S_IWUSR | S_IRUGO ),
//   __ATTR( name, 0666, &SHOW_##name, &STORE_##name )

static OWN_CLASS_ATTR( data1 );
static OWN_CLASS_ATTR( data2 );
static OWN_CLASS_ATTR( data3 );

static struct class *x_class;

int __init x_init(void) {
   int res;
   x_class = class_create( THIS_MODULE, "x-class" );
   if( IS_ERR( x_class ) ) printk( "bad class create\n" );
   res = class_create_file( x_class, &class_attr_data1 );
   res = class_create_file( x_class, &class_attr_data2 );
   res = class_create_file( x_class, &class_attr_data3 );
   printk("'yxxx' module initialized\n");
   return 0;
}

void x_cleanup(void) {
   class_remove_file( x_class, &class_attr_data1 );
   class_remove_file( x_class, &class_attr_data2 );
   class_remove_file( x_class, &class_attr_data3 );
   class_destroy( x_class );
   return;
}

module_init( x_init );
module_exit( x_cleanup );
MODULE_LICENSE( "GPL" );

