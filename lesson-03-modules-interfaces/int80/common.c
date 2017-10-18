#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/kdev_t.h>
#include <sys/syscall.h>

void do_write( void );
void do_mknod( void );
void do_getpid( void );

int main( int argc, char *argv[] ) {
   do_getpid();
   do_write();
   do_mknod();
   return EXIT_SUCCESS;
};
