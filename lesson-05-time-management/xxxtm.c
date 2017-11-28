#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>

static u64 g_last_sys_time = 0;
static struct timeval g_last_abs_time = {0, 0};

#define FIBO1_INIT 0
#define FIBO2_INIT 1
static unsigned int g_fibo1 = FIBO1_INIT;
static unsigned int g_fibo2 = FIBO2_INIT;
#define FIBO_NUMBER_BUFFER 10 + 10 + 2

static void compute_fibo_next(unsigned int *fibo1, unsigned int *fibo2)
{
   unsigned int fibo_temp = *fibo1 + *fibo2;
   *fibo1 = *fibo2;
   *fibo2 = fibo_temp;
}

static void dump_fibo_sequence(char *buf)
{
   unsigned int fibo_max = g_fibo2;
   unsigned int fibo1 = FIBO1_INIT;
   unsigned int fibo2 = FIBO2_INIT;
   while (fibo2 <= fibo_max) {
      char tem_buf[FIBO_NUMBER_BUFFER] = {0};
      int count = 0;
      if (fibo1 == FIBO1_INIT)
         count = sprintf(tem_buf, "%u %u ", fibo1, fibo2);
      else
         count = sprintf(tem_buf, "%u ", fibo2);
      //pr_info("%s: count %d, buf %p", __FUNCTION__, count, buf);
      if (count > 0)
      {
         strncpy(buf, tem_buf, count);
         buf += count;
         buf[0] = '\0';
         //pr_info("%s: count %d, buf %p/%d", __FUNCTION__, count, buf, buf[0]);
      }
      compute_fibo_next(&fibo1, &fibo2);
   }
   buf[0] = '\n';
}

static ssize_t tm_jif_show(struct class *class, struct class_attribute *attr,
   char *buf)
{
   size_t count = 0;
   u64 sys_time = get_jiffies_64();
   unsigned int seconds = 0;
   if (g_last_sys_time) {
      u64 diff = sys_time - g_last_sys_time;
      seconds = jiffies_to_msecs(diff) / MSEC_PER_SEC;
   }
   sprintf(buf, "%u\n", seconds);
   count = strlen(buf);
   g_last_sys_time = sys_time;
   return count;
}

static ssize_t tm_absk_show(struct class *class, struct class_attribute *attr,
   char *buf)
{
   size_t count = 0;
   struct timeval tv;
   unsigned long time;
   if (!buf)
      return count;

   do_gettimeofday(&tv);
   time = g_last_abs_time.tv_sec
      ? g_last_abs_time.tv_sec : tv.tv_sec;
   if (time) {
      struct tm tm_val;
      time_to_tm(time, 0, &tm_val);
      sprintf(buf, "%d-%d-%ld %02d:%02d:%02d\n",
         tm_val.tm_mday, tm_val.tm_mon + 1, 1900 + tm_val.tm_year,
         tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
   }
   else {
      buf[0] = '\0';
   }

   count = strlen(buf);
   g_last_abs_time = tv;
   return count;
}

static void schedule_timer(void);
#define TIMER_DELAY_MSEC 1000

static void fibo_timer(unsigned long data)
{
   compute_fibo_next(&g_fibo1, &g_fibo2);
   schedule_timer();
}

struct timer_list g_fibo_timer = __TIMER_INITIALIZER(fibo_timer, 0, 0, TIMER_IRQSAFE);

static void schedule_timer(void)
{
   u64 time;
   int ret;

   time = get_jiffies_64() + msecs_to_jiffies(TIMER_DELAY_MSEC);
   ret = mod_timer(&g_fibo_timer, time);
   //pr_info("%s: timer rescheduled: %d\n", __FUNCTION__, ret);
}

static ssize_t tm_fibo_show(struct class *class, struct class_attribute *attr,
   char *buf)
{
   size_t count = 0;
   if (!buf)
      return count;

   dump_fibo_sequence(buf);

   count = strlen(buf);
   return count;
}

struct class_attribute class_attr_tm_jif  = __ATTR_RO(tm_jif);
struct class_attribute class_attr_tm_absk = __ATTR_RO(tm_absk);
struct class_attribute class_attr_tm_fibo = __ATTR_RO(tm_fibo);

static struct class *tm_jif_class = NULL;
static struct class *tm_absk_class = NULL;
static struct class *tm_fibo_class = NULL;

void x_cleanup(void)
{
   if (tm_fibo_class) {
      class_remove_file(tm_fibo_class, &class_attr_tm_fibo);
      class_destroy(tm_fibo_class);
   }

   if (tm_absk_class) {
      class_remove_file(tm_absk_class, &class_attr_tm_absk);
      class_destroy(tm_absk_class);
   }

   if (tm_jif_class) {
      class_remove_file(tm_jif_class, &class_attr_tm_jif);
      class_destroy(tm_jif_class);
   }

   del_timer_sync(&g_fibo_timer);
}

int __init x_init(void)
{
   int res = 0;
   tm_jif_class = class_create(THIS_MODULE, "tm_jif-class");
   if(IS_ERR(tm_jif_class)) {
      pr_info("bad class create\n");
      goto error1;
   }
   res = class_create_file(tm_jif_class, &class_attr_tm_jif);
   if (res != 0)
      goto error1;

   tm_absk_class = class_create(THIS_MODULE, "tm_absk-class");
   if(IS_ERR(tm_absk_class)) {
      pr_info("bad class create\n");
      goto error1;
   }
   res = class_create_file(tm_absk_class, &class_attr_tm_absk);
   if (res != 0)
      goto error1;

   tm_fibo_class = class_create(THIS_MODULE, "tm_fibo-class");
   if(IS_ERR(tm_fibo_class)) {
      pr_info("bad class create\n");
      goto error1;
   }
   res = class_create_file(tm_fibo_class, &class_attr_tm_fibo);
   if (res != 0)
      goto error1;

   schedule_timer();

   pr_info("'xxxtm' module initialized %d\n", res);
   return res;

error1:
   pr_err("initialization error\n");
   x_cleanup();
   return res;
}

module_init(x_init);
module_exit(x_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergii.Romantsov serg.cramser@gmail.com");
