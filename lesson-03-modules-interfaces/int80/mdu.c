#include "syscall.h"

static int __init x80_init( void ) {
   do_write();
   do_mknod();
   do_getpid();
   return -1;
}

module_init( x80_init );

MODULE_LICENSE( "GPL" );