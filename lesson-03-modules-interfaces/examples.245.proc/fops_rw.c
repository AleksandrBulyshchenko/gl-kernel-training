#include <linux/compiler.h>

static char *get_rw_buf( void ) {
   static char buf_msg[ LEN_MSG + 1 ] =
          ".........1.........2.........3.........4.........5\n";
   return buf_msg;
}

// чтение из /proc/mod_proc :
static ssize_t node_read( struct file *file, char *buf,
                          size_t count, loff_t *ppos ) {
   char *buf_msg = get_rw_buf();
   int res;
   LOG( "read: %ld bytes (ppos=%lld)\n", (long)count, *ppos );
   if( *ppos >= strlen( buf_msg ) ) {     // EOF
      *ppos = 0;
      LOG( "EOF" );
      return 0;
   }
   if( count > strlen( buf_msg ) - *ppos ) 
      count = strlen( buf_msg ) - *ppos;  // это копия
   res = copy_to_user( (void*)buf, buf_msg + *ppos, count );
   *ppos += count;
   LOG( "return %ld bytes\n", (long)count );
   return count;
}

// запись в /proc/mod_proc :
static ssize_t __maybe_unused node_write( struct file *file, const char *buf,
                           size_t count, loff_t *ppos ) {
   char *buf_msg = get_rw_buf();
   int res;
   uint len = count < LEN_MSG ? count : LEN_MSG;
   LOG( "write: %ld bytes\n", (long)count );
   res = copy_from_user( buf_msg, (void*)buf, len );
   buf_msg[ len ] = '\0';
   LOG( "put %d bytes\n", len );
   return len;
}
