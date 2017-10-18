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
   int ret;
   struct proc_dir_entry *own_proc_node;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0))
   own_proc_node = proc_create( NAME_NODE, S_IFREG | S_IRUGO | S_IWUGO, NULL, &node_fops );
#else
   own_proc_node = create_proc_entry( NAME_NODE, S_IFREG | S_IRUGO | S_IWUGO, NULL );
#endif
   if( NULL == own_proc_node ) {
      ret = -ENOENT;
      ERR( "can't create /proc/%s\n", NAME_NODE );
      goto err_node;
   }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3, 9, 0))
   own_proc_node->uid = own_proc_node->gid = 0;
   own_proc_node->read_proc = proc_node_read;
#endif
   LOG( "/proc/%s installed\n", NAME_NODE );
   return 0;
err_node:
   return ret;
}

static void __exit proc_exit( void ) {
   remove_proc_entry( NAME_NODE, NULL );
   LOG( "/proc/%s removed\n", NAME_NODE );

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0))
   /* put it somewhere to avoid warnings */
   (void)node_write;
#endif
}
