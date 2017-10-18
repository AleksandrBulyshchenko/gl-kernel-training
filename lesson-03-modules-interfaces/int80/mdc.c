#include <linux/uaccess.h>
#include "syscall.h"

static int __init x80_init( void ) {
   mm_segment_t fs = get_fs();
   set_fs( get_ds() );
   do_write();
   do_mknod();
   do_getpid();
   set_fs(fs);
   return -1;
}

module_init( x80_init );

MODULE_LICENSE( "GPL" );