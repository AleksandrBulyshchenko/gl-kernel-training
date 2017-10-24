
#define FIB_BUFFSIZE 1000

typedef struct
{
    uint8_t *fib_buf0;
    uint8_t *fib_buf1;
    uint8_t *tmp;
    uint16_t fib_curr_len;
    uint32_t fib_iter;
    bool ovf;
}fb_t; 


void fib_get_str(const uint8_t *a, uint8_t *str, uint32_t length);

bool fib_step(fb_t *fib);
