#ifndef FIB_H
#define FIB_H


#define FIB_BUFFSIZE 1000

typedef struct
{
    uint32_t fib_iter;
    uint16_t fib_curr_len;
    uint8_t *fib_buf0;
    uint8_t *fib_buf1;  
    bool ovf;
}fb_t; 


void fib_init(fb_t *f);
void fib_uninit(fb_t *f);
bool fib_step(fb_t *f);
void fib_get_str(fb_t *f, char *buf);



#endif //FIB_H
