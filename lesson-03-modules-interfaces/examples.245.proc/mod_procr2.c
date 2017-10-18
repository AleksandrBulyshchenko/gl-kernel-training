#include "mod_proc.h"

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3, 9, 0))
#include "proc_node_read.c"
#else
#include "fops_rw.c"
static const struct file_operations node_fops = {
   .owner  = THIS_MODULE,
   .read   = node_read,
};
#endif

static int __init proc_init( void ) {
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0))
   struct proc_dir_entry *own_proc_node;
   own_proc_node = proc_create( NAME_NODE, S_IFREG | S_IRUGO | S_IWUGO, NULL, &node_fops );

   if( NULL == own_proc_node ) {
      ERR( "can't create /proc/%s\n", NAME_NODE );
      return-ENOENT;
   }
#else
   if( create_proc_read_entry( NAME_NODE, 0, NULL, proc_node_read, NULL ) == 0 ) {
      ERR( "can't create /proc/%s\n", NAME_NODE );
      return -ENOENT;
   }
#endif
   LOG( "/proc/%s installed", NAME_NODE );
   return 0;
}

static void __exit proc_exit( void ) {
   remove_proc_entry( NAME_NODE, NULL );
   LOG( "/proc/%s removed\n", NAME_NODE );

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0))
   /* put it somewhere to avoid warnings */
   (void)node_write;
#endif
}
