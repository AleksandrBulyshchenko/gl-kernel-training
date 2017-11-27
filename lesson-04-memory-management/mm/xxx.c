#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/sysfs.h>
#include <linux/list.h>

struct MemoryManager
{
   void (*fp_initialize_memory)(void);
   void (*fp_finalize_memory)(void);
   void* (*fp_allocate_memory)(size_t count);
   void (*fp_free_memory)(void** buffer);
   size_t (*fp_store_to_buffer)(char const* buffer_from, size_t count);
   size_t (*fp_dump_to_buffer)(char* buffer_to);
};

static struct MemoryManager g_mm;
static size_t g_buf_size = 0;
static char* g_buf_msg   = 0;

#define MEM_CONFIG_KMALLOC 0
#define MEM_CONFIG_KMCACHE 1

#define CACHE_NAME "xxx_cache"
static struct kmem_cache* g_cache = NULL;
struct cache_list
{
   struct list_head list;
};
struct cache_list cache_list;
#define CACHE_BUFFER_OFFSET sizeof(struct cache_list)
#define CACHE_BUFFER_DATA_SIZE 244
#define CACHE_SIZE CACHE_BUFFER_OFFSET + CACHE_BUFFER_DATA_SIZE

static void cache_constructor(void* p)
{
   pr_info("%s constructs %p(%u)", THIS_MODULE->name, p, (p ? *(char*)p : 0));
}

static int g_mem_config = MEM_CONFIG_KMALLOC;
module_param_named(mem_config, g_mem_config, int, 0);


static size_t construct_buffer(size_t new_count)
{
   size_t result = new_count;
   if (!new_count)
      return result;

   if (new_count >= g_buf_size)
   {
      if (g_buf_msg)
         g_mm.fp_free_memory((void**)&g_buf_msg);
      g_buf_size = new_count + 1;
      g_buf_msg  = g_mm.fp_allocate_memory(g_buf_size);
      if (!g_buf_msg)
      {
         g_buf_size = 0;
         result     = 0;
      }
      else
      {
         result = g_buf_size - 1;
      }
   }
   printk("%s %s: %p/%d", THIS_MODULE->name, __FUNCTION__, g_buf_msg, result);
   return result;
}

//Memory management for kmem_cache
static void initialize_memory_kmcache(void)
{
   g_cache = kmem_cache_create(CACHE_NAME, CACHE_SIZE, 0/*SLAB_HWCACHE_ALIGN*/,
      0/*SLAB_DEBUG_INITIAL*/, &cache_constructor);
   INIT_LIST_HEAD(&cache_list.list);
   pr_info("%s %s cache: %p", THIS_MODULE->name, __FUNCTION__, g_cache);
}

static void finalize_memory_kmcache(void)
{
   if (g_cache)
      kmem_cache_destroy(g_cache);
   g_cache = NULL;
}

static void* allocate_memory_kmcache(size_t count)
{
   void* result = NULL;
   if (!count)
      return result;

   if (g_cache)
   {
      do
      {
         struct cache_list* node;
         result = kmem_cache_alloc(g_cache, GFP_KERNEL);
         if (!result)
            break;
         node = (struct cache_list*)result;
         INIT_LIST_HEAD(&node->list);
         list_add_tail(&(node->list), &(cache_list.list));
         count -= count > CACHE_BUFFER_DATA_SIZE - 1 // to allow '\0'
            ? CACHE_BUFFER_DATA_SIZE - 1 : count;
         pr_info("%s %s: %p/%d", THIS_MODULE->name, __FUNCTION__, result, count);
      } while (count > 0);
      return &cache_list;
   }
   else
   {
      count = 0;
   }
   pr_info("%s %s: %p/%d", THIS_MODULE->name, __FUNCTION__, result, count);
   return result;
}

static void free_memory_kmcache(void** buffer)
{
   if (!buffer || !(*buffer))
      return;

   if (g_cache)
   {
      struct cache_list *node, *tmp;
      list_for_each_entry_safe(node, tmp, &cache_list.list, list)
      {
         pr_info("%s %s: freeing node %p", THIS_MODULE->name, __FUNCTION__, node);
         list_del(&node->list);
         kmem_cache_free(g_cache, node);
      }
   }
   *buffer = NULL;
}

static size_t store_to_buffer_kmcache(char const* buffer_from, size_t count)
{
   size_t size = construct_buffer(count);
   if (size)
   {
      struct cache_list* node;
      size_t counter = 0;
      list_for_each_entry(node, &cache_list.list, list)
      {
         char* buffer = ((char*)node) + CACHE_BUFFER_OFFSET;
         size_t to_copy = (size - counter) > (CACHE_BUFFER_DATA_SIZE - 1)
            ? CACHE_BUFFER_DATA_SIZE - 1 : size - counter;
         strncpy(buffer, buffer_from + counter, to_copy);
         buffer[to_copy] = '\0';
         counter += to_copy;
         if(counter >= size)
            break;
      }
   }
   return size;
}

static size_t dump_to_buffer_kmcache(char* buffer_to)
{
   size_t counter = 0;
   if (g_buf_msg)
   {
      struct cache_list* node;
      list_for_each_entry(node, &cache_list.list, list)
      {
         char const* buffer = ((char*)node) + CACHE_BUFFER_OFFSET;
         size_t to_copy = strlen(buffer);
         strncpy(buffer_to + counter, buffer, to_copy);
         counter += to_copy;
         if (counter >= g_buf_size)
            break;
      }
      buffer_to[counter] = '\0';
   }
   return counter;
}
///////////////////////

//Memory management for kmalloc
static void initialize_memory_kmalloc(void)
{
}

static void finalize_memory_kmalloc(void)
{
}

static void* allocate_memory_kmalloc(size_t count)
{
   void* result = NULL;
   if (!count)
      return result;

   result = kmalloc(count, GFP_ATOMIC);
   pr_info("%s %s: %p/%d", THIS_MODULE->name, __FUNCTION__, result, count);
   return result;
}

static void free_memory_kmalloc(void** buffer)
{
   if (!buffer || !(*buffer))
      return;

   kfree(*buffer);
   *buffer = NULL;
}

static size_t store_to_buffer_kmalloc(char const* buffer_from, size_t count)
{
   size_t size = construct_buffer(count);
   if (size)
   {
      strncpy(g_buf_msg, buffer_from, size);
      g_buf_msg[size] = '\0';
   }
   return size;
}

static size_t dump_to_buffer_kmalloc(char* buffer_to)
{
   size_t count = 0;
   if (g_buf_msg)
   {
      strcpy(buffer_to, g_buf_msg);
      count = strlen(buffer_to);
   }
   return count;
}
//////////////////////

static ssize_t xxx_show(struct class *class, struct class_attribute *attr,
                  char *buf)
{
   size_t const count = g_mm.fp_dump_to_buffer(buf);
   printk("read %ld\n", (long)count);
   return count;
}

static ssize_t xxx_store(struct class *class, struct class_attribute *attr,
                   const char *buf, size_t count)
{
   printk("write %ld\n", (long)count);
   count = g_mm.fp_store_to_buffer(buf, count);
   return count;
}

CLASS_ATTR_RW(xxx);

static struct class *x_class = NULL;

static bool initialize_mm(void)
{
   bool result = true;
   if (g_mem_config == MEM_CONFIG_KMALLOC)
   {
      g_mm.fp_initialize_memory = initialize_memory_kmalloc;
      g_mm.fp_finalize_memory = finalize_memory_kmalloc;
      g_mm.fp_allocate_memory = allocate_memory_kmalloc;
      g_mm.fp_free_memory = free_memory_kmalloc;
      g_mm.fp_store_to_buffer = store_to_buffer_kmalloc;
      g_mm.fp_dump_to_buffer = dump_to_buffer_kmalloc;
   }
   else if (g_mem_config == MEM_CONFIG_KMCACHE)
   {
      g_mm.fp_initialize_memory = initialize_memory_kmcache;
      g_mm.fp_finalize_memory = finalize_memory_kmcache;
      g_mm.fp_allocate_memory = allocate_memory_kmcache;
      g_mm.fp_free_memory = free_memory_kmcache;
      g_mm.fp_store_to_buffer = store_to_buffer_kmcache;
      g_mm.fp_dump_to_buffer = dump_to_buffer_kmcache;
   }
   else
      result = false;
   return result;
}

void x_cleanup(void)
{
   g_mm.fp_free_memory((void**)&g_buf_msg);
   g_mm.fp_finalize_memory();
   if (x_class)
   {
      class_remove_file(x_class, &class_attr_xxx);
      class_destroy(x_class);
   }
}

int __init x_init(void)
{
   int res = -EINVAL;
   x_class = class_create(THIS_MODULE, "x-class");
   if(IS_ERR(x_class))
   {
      pr_info("bad class create\n");
      goto error;
   }
   res = class_create_file(x_class, &class_attr_xxx);
   res = !res
      ? !initialize_mm() ? -EINVAL : 0
      : res;
   if (res)
      goto error;

   g_mm.fp_initialize_memory();
   {
      char const* const initial_buffer = "Hi!\n";
      g_mm.fp_store_to_buffer(initial_buffer, strlen(initial_buffer));
   }
   printk("'xxx' module initialized %d\n", res);
   return res;

error:
   x_cleanup();
   return res;
}

module_init(x_init);
module_exit(x_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergii Romantsov");
MODULE_DESCRIPTION("Module shows usage of memory management by different sub-systems");
