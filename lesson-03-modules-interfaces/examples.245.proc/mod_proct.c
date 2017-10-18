#include "mod_proc.h"
#include "fops_rw.c" // чтение-запись для /proc/mod_dir/mod_node

static const struct file_operations node_fops = {
   .owner  = THIS_MODULE,
   .read   = node_read,
   .write  = node_write
};

static struct proc_dir_entry *own_proc_dir; //, *own_proc_node;

static int __init proc_init( void ) {
   int ret;
   struct proc_dir_entry *own_proc_node;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
   own_proc_dir = create_proc_entry( NAME_DIR, S_IFDIR | S_IRWXUGO, NULL );
   if( NULL == own_proc_dir ) {
      ret = -ENOENT;
      ERR( "can't create directory /proc/%s\n", NAME_DIR );
      goto err_dir;
   }
   own_proc_dir->uid = own_proc_dir->gid = 0;
   own_proc_node = create_proc_entry( NAME_NODE, S_IFREG | S_IRUGO | S_IWUGO, own_proc_dir );
   if( NULL == own_proc_node ) {
      ret = -ENOENT;
      ERR( "can't create node /proc/%s/%s\n", NAME_DIR, NAME_NODE );
      goto err_node;
   }
   own_proc_node->uid = own_proc_node->gid = 0;
   own_proc_node->proc_fops = &node_fops;
#else
   own_proc_dir = proc_mkdir( NAME_DIR, NULL );
   if( NULL == own_proc_dir ) {
      ret = -ENOENT;
      ERR( "can't create directory /proc/%s\n", NAME_NODE );
      goto err_dir;
   }
   own_proc_node = proc_create( NAME_NODE, S_IFREG | S_IRUGO | S_IWUGO, own_proc_dir, &node_fops );
   if( NULL == own_proc_node ) {
      ret = -ENOENT;
      ERR( "can't create node /proc/%s/%s\n", NAME_DIR, NAME_NODE );
      goto err_node;
   }
#endif
   LOG( "/proc/%s/%s installed\n", NAME_DIR, NAME_NODE );
   return 0;
err_node:
   remove_proc_entry( NAME_DIR, NULL );
err_dir:
   return ret;
}

static void __exit proc_exit( void ) {
   remove_proc_entry( NAME_NODE, own_proc_dir );
   remove_proc_entry( NAME_DIR, NULL );
   LOG( "/proc/%s/%s removed\n", NAME_DIR, NAME_NODE );
} 
