#include "common.c"

void do_write( void ) {
   char *str = "эталонная строка для вывода!\n";
   int len = strlen( str ) + 1, n;
   printf( "string for write length = %d\n", len );
   n = syscall( __NR_write, 1, str, len );  
   printf( "write return : %d\n", n );
}

void do_mknod( void ) {
   char *nam = "ZZZ";
   int n = syscall( __NR_mknod, nam, S_IFCHR | S_IRUSR | S_IWUSR, MKDEV( 247, 0 ) );
   printf( "mknod return : %d\n", n );
}

void do_getpid( void ) {
   int n = syscall( __NR_getpid );
   printf( "getpid return : %d\n", n );
}
