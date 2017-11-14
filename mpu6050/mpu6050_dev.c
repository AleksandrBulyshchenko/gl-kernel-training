#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/time.h>
//#include "../dev.h"
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "mpu6050-regs.h"

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Ruslan V. Sukhov <sukhov@karazin.ua>" );
MODULE_VERSION( "1.1" );

static struct i2c_client* mpu6050_i2c_client;

struct mpu6050_data
{
//	struct i2c_client *drv_client;
	int accel_values[3];
	int gyro_values[3];
	int temperature;
	
};

//static struct mpu6050_data g_mpu6050_data;
#define MPU6050_BUF_SIZE 10
struct mpu6050_buf_data
{
	unsigned int size;
	unsigned int read_index;
	unsigned int write_index;
	unsigned int count;
	struct mpu6050_data buf[MPU6050_BUF_SIZE];
};

//static struct mpu6050_buf_data* buf_data;

static struct mpu6050_buf_data buf_data = {
	.size = MPU6050_BUF_SIZE,
	.read_index = 0,
	.write_index = 0,
	.count = 0,
};
/*
static int mpu6050_buf_init(struct mpu6050_buf_data* bd)
{
	bd = kmalloc(sizeof(struct mpu6050_buf_data),GFP_KERNEL);
	bd->size = MPU6050_BUF_SIZE;
	bd->read_index = 0;
	bd->write_index = 0;
	bd->count = 0;
	bd->buf = kmalloc(sizeof(struct mpu6050_data) * (bd->size),GFP_KERNEL);
	return 0;
}

static int mpu6050_buf_deinit(struct mpu6050_buf_data* bd)
{
	kfree(bd->buf);
	kfree(bd);
	return 0;
}
*/
static int mpu6050_buf_add(struct mpu6050_buf_data* bd, struct mpu6050_data* d)
{
	int i;
	for (i = 0; i < 3; i++)	{
		(bd->buf[bd->write_index]).accel_values[i] = (d->accel_values)[i];
		(bd->buf[bd->write_index]).gyro_values[i] = (d->gyro_values)[i];	
	}
	(bd->buf[bd->write_index]).temperature = d->temperature;
	
	if (bd->count < bd->size) {
		(bd->write_index)++;
		if (bd->write_index == bd->size) bd->write_index = 0;
		(bd->count)++;	
	}
	else {
		(bd->write_index)++;
		(bd->read_index)++;
		if (bd->write_index == bd->size) bd->write_index = 0;
		if (bd->read_index == bd->size) bd->read_index = 0;
	}	
	return 0;
}

static int mpu6050_buf_get(struct mpu6050_buf_data* bd, struct mpu6050_data* d)
{
	if (bd->count > 0) {
		int i;
		for (i = 0; i < 3; i++)	{
			(d->accel_values)[i] = (bd->buf[bd->read_index]).accel_values[i];
			(d->gyro_values)[i] = (bd->buf[bd->read_index]).gyro_values[i];	
		}
		d->temperature = (bd->buf[bd->read_index]).temperature;
		(bd->read_index)++;
		if (bd->read_index == bd->size) bd->read_index = 0;
		(bd->count)--;
	}
	return 0;
}
static int mpu6050_read_data(struct mpu6050_data* p_mpu6050_data)
{
	int temp;

	if (mpu6050_i2c_client == 0)
		return -ENODEV;

	/* accel */
	p_mpu6050_data->accel_values[0] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_ACCEL_XOUT_H));
	p_mpu6050_data->accel_values[1] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_ACCEL_YOUT_H));
	p_mpu6050_data->accel_values[2] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	p_mpu6050_data->gyro_values[0] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_GYRO_XOUT_H));
	p_mpu6050_data->gyro_values[1] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_GYRO_YOUT_H));
	p_mpu6050_data->gyro_values[2] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_GYRO_ZOUT_H));
	/* temp */
	/* Temperature in degrees C = (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_i2c_client, REG_TEMP_OUT_H));
	p_mpu6050_data->temperature = (temp + 12420 + 170) / 340;

	printk(KERN_INFO "mpu6050: sensor data read:\n");
	printk(KERN_INFO "mpu6050: ACCEL[X,Y,Z] = [%d, %d, %d]\n",
		p_mpu6050_data->accel_values[0],
		p_mpu6050_data->accel_values[1],
		p_mpu6050_data->accel_values[2]);
	printk(KERN_INFO "mpu6050: GYRO[X,Y,Z] = [%d, %d, %d]\n",
		p_mpu6050_data->gyro_values[0],
		p_mpu6050_data->gyro_values[1],
		p_mpu6050_data->gyro_values[2]);
	printk(KERN_INFO "mpu6050: TEMP = %d\n", p_mpu6050_data->temperature);
	return 0;
}

static int mpu6050_probe(struct i2c_client *drv_client, const struct i2c_device_id *id)
{
	int ret;

	printk(KERN_INFO "mpu6050: i2c client address is 0x%X\n", drv_client->addr);

	/* Read who_am_i register */
	ret = i2c_smbus_read_byte_data(drv_client, REG_WHO_AM_I);
	if (IS_ERR_VALUE(ret)) {
		printk(KERN_ERR "mpu6050: i2c_smbus_read_byte_data() failed with error: %d\n", ret);
		return ret;
	}
	if (ret != MPU6050_WHO_AM_I) {
		printk(KERN_ERR "mpu6050: wrong i2c device found: expected 0x%X, found 0x%X\n", MPU6050_WHO_AM_I, ret);
		return -1;
	}
	printk(KERN_INFO "mpu6050: i2c mpu6050 device found, WHO_AM_I register value = 0x%X\n", ret);

	/* Setup the device */
	/* No error handling here! */
	i2c_smbus_write_byte_data(drv_client, REG_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_GYRO_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_ACCEL_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_FIFO_EN, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_PIN_CFG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_ENABLE, 0);
	i2c_smbus_write_byte_data(drv_client, REG_USER_CTRL, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_1, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_2, 0);

	mpu6050_i2c_client = drv_client;

	printk(KERN_INFO "mpu6050: i2c driver probed\n");
	return 0;
}

static int mpu6050_remove(struct i2c_client *drv_client)
{
	mpu6050_i2c_client = 0;

	printk(KERN_INFO "mpu6050: i2c driver removed\n");
	return 0;
}

static const struct i2c_device_id mpu6050_idtable [] = {
	{ "gl_mpu6050", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);

static struct i2c_driver mpu6050_i2c_driver = {
	.driver = {
		.name = "gl_mpu6050",
	},

	.probe = mpu6050_probe ,
	.remove = mpu6050_remove,
	.id_table = mpu6050_idtable,
};

static char mpu6050_str[100];   // buffer!

static int major_current = 0;
static int major_all = 0;
module_param( major_current, int, S_IRUGO );
module_param( major_all, int, S_IRUGO );

static ssize_t dev_current_read( struct file * file, char * buf,size_t count, loff_t *ppos ) {
	struct mpu6050_data tmp_data;
	int len;
	static int execute = 0;
	if (execute == 0) {
		mpu6050_read_data(&tmp_data);
		mpu6050_buf_add(&buf_data, &tmp_data);
		sprintf(mpu6050_str, "ACC[X,Y,Z]=[%d, %d, %d]\nGYRO[X,Y,Z]=[%d, %d, %d]\nT=%d\n\0", 				tmp_data.accel_values[0], tmp_data.accel_values[1], tmp_data.accel_values[2], 
			tmp_data.gyro_values[0], tmp_data.gyro_values[1], tmp_data.gyro_values[2],
			tmp_data.temperature);
				
		len = strlen( mpu6050_str );		
		execute = 1;
	}		
	else {
		len = 0;
		execute = 0;
	}
	
	if( count < len ) return -EINVAL;
	if( *ppos != 0 ) return 0;
	if( copy_to_user( buf, mpu6050_str, len ) ) return -EINVAL;
	*ppos = len;
	return len;
}

static const struct file_operations dev_current_fops = {
   .owner = THIS_MODULE,
   .read  = dev_current_read,
};

static ssize_t dev_all_read( struct file * file, char * buf,size_t count, loff_t *ppos ) {
	struct mpu6050_data tmp_data;
	int offset = 0, len = 0, i, tmp;
	static int execute = 0;
	if (execute == 0) {
		sprintf(mpu6050_str,"Buffer contains %d samples:\n",buf_data.count);
		len = strlen( mpu6050_str );
		if( count < len ) return -EINVAL;
		if( *ppos != 0 ) return 0;
		offset += len;
		if( copy_to_user( buf, mpu6050_str, len ) ) return -EINVAL;
		tmp = buf_data.count;		
		for(i = 0; i < tmp; i++) {
			mpu6050_buf_get(&buf_data, &tmp_data);
			sprintf(mpu6050_str, "ACC[X,Y,Z]=[%d, %d, %d]\nGYRO[X,Y,Z]=[%d, %d, %d]\nT=%d\n%d\0", 					tmp_data.accel_values[0], tmp_data.accel_values[1], tmp_data.accel_values[2], 
				tmp_data.gyro_values[0], tmp_data.gyro_values[1], tmp_data.gyro_values[2],
				tmp_data.temperature, buf_data.count);
			len = strlen( mpu6050_str );
			if( copy_to_user( buf + offset, mpu6050_str, len ) ) return -EINVAL;
			offset += len;
		}
		execute = 1;
	}
	else {
		offset = 0;
		execute = 0;
	}
	*ppos = offset;
	return offset;
}

static const struct file_operations dev_all_fops = {
   .owner = THIS_MODULE,
   .read  = dev_all_read,
};

#define DEVICE_CURRENT_FIRST  0
#define DEVICE_ALL_FIRST  0
#define DEVICE_COUNT  1
#define MOD_CURRENT_NAME "mpu6050_current_mod"
#define MOD_ALL_NAME "mpu6050_all_mod"

static struct cdev h_current_cdev;
static struct cdev h_all_cdev;
static struct class *devclass;

static int __init dev_init( void ) {
	int ret, i;
	dev_t dev;
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret < 0) {
		printk(KERN_ERR "mpu6050: failed to add new i2c driver: %d\n", ret);
		goto err;
	}
//	printk(KERN_INFO "mpu6050: i2c driver created\n");	
//current
	if( major_current != 0 ) {
		dev = MKDEV( major_current, DEVICE_CURRENT_FIRST );
		ret = register_chrdev_region( dev, DEVICE_COUNT, MOD_CURRENT_NAME );
	}
	else {
		ret = alloc_chrdev_region( &dev, DEVICE_CURRENT_FIRST, DEVICE_COUNT, MOD_CURRENT_NAME );
		major_current = MAJOR( dev );  // не забыть зафиксировать!
	}
	if( ret < 0 ) {
		printk( KERN_ERR "Can not register char device region\n" );
		goto err;
	}
	cdev_init( &h_current_cdev, &dev_current_fops );
	h_current_cdev.owner = THIS_MODULE;
	h_current_cdev.ops = &dev_current_fops;   // обязательно! - cdev_init() недостаточно?
	ret = cdev_add( &h_current_cdev, dev, DEVICE_COUNT );
	if( ret < 0 ) {
		unregister_chrdev_region( MKDEV( major_current, DEVICE_CURRENT_FIRST ), DEVICE_COUNT );
		printk( KERN_ERR "Can not add char device\n" );
		goto err;
	}
	devclass = class_create( THIS_MODULE, "mpu6050" );
	for (i = 0; i < DEVICE_COUNT; i++) {
	#define DEV_CURRENT_NAME "mpu6050current"
		dev = MKDEV( major_current, DEVICE_CURRENT_FIRST + i);		
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26) 
	/* struct device *device_create( struct class *cls, struct device *parent, 
                                 dev_t devt, const char *fmt, ...); */ 
      		device_create( devclass, NULL, dev, "%s_%d", DEV_CURRENT_NAME, i ); 
	#else           // прототип device_create() изменился! 
	/* struct device *device_create( struct class *cls, struct device *parent, 
                                 dev_t devt, void *drvdata, const char *fmt, ...); */ 
		device_create( devclass, NULL, dev, NULL, "%s_%d", DEV_CURRENT_NAME, i ); 
	#endif
	}
	printk( KERN_INFO "=========== module installed %d:%d ==============\n",
	MAJOR( dev ), MINOR( dev ) );

//all
	if( major_all != 0 ) {
		dev = MKDEV( major_all, DEVICE_ALL_FIRST );
		ret = register_chrdev_region( dev, DEVICE_COUNT, MOD_ALL_NAME );
	}
	else {
		ret = alloc_chrdev_region( &dev, DEVICE_ALL_FIRST, DEVICE_COUNT, MOD_ALL_NAME );
		major_all = MAJOR( dev );  // не забыть зафиксировать!
	}
	if( ret < 0 ) {
		printk( KERN_ERR "Can not register char device region\n" );
		goto err;
	}
	cdev_init( &h_all_cdev, &dev_all_fops );
	h_all_cdev.owner = THIS_MODULE;
	h_all_cdev.ops = &dev_all_fops;   // обязательно! - cdev_init() недостаточно?
	ret = cdev_add( &h_all_cdev, dev, DEVICE_COUNT );
	if( ret < 0 ) {
		unregister_chrdev_region( MKDEV( major_all, DEVICE_ALL_FIRST ), DEVICE_COUNT );
		printk( KERN_ERR "Can not add char device\n" );
		goto err;
	}
	for (i = 0; i < DEVICE_COUNT; i++) {
	#define DEV_ALL_NAME "mpu6050all"
		dev = MKDEV( major_all, DEVICE_ALL_FIRST + i);		
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26) 
	/* struct device *device_create( struct class *cls, struct device *parent, 
                                 dev_t devt, const char *fmt, ...); */ 
      		device_create( devclass, NULL, dev, "%s_%d", DEV_ALL_NAME, i ); 
	#else           // прототип device_create() изменился! 
	/* struct device *device_create( struct class *cls, struct device *parent, 
                                 dev_t devt, void *drvdata, const char *fmt, ...); */ 
		device_create( devclass, NULL, dev, NULL, "%s_%d", DEV_ALL_NAME, i ); 
	#endif
	}
	printk( KERN_INFO "=========== module installed %d:%d ==============\n",
	MAJOR( dev ), MINOR( dev ) );

//	mpu6050_buf_init(buf_data);	

	err:
   		return ret;
}

static void __exit dev_exit( void ) {
	dev_t dev;
	int i;
//	mpu6050_buf_deinit(buf_data);
	i2c_del_driver(&mpu6050_i2c_driver);
//current
	for (i = 0; i < DEVICE_COUNT; i++) {
		dev = MKDEV( major_current, DEVICE_CURRENT_FIRST + i);		
		device_destroy( devclass, dev );
	}
//all
	for (i = 0; i < DEVICE_COUNT; i++) {
		dev = MKDEV( major_all, DEVICE_CURRENT_FIRST + i);		
		device_destroy( devclass, dev );
	}

	class_destroy( devclass );
//currnet
	cdev_del( &h_current_cdev );
	unregister_chrdev_region( MKDEV( major_current, DEVICE_CURRENT_FIRST ), DEVICE_COUNT );
	printk( KERN_INFO "=============== module removed ==================\n" );
//all
	cdev_del( &h_all_cdev );
	unregister_chrdev_region( MKDEV( major_all, DEVICE_ALL_FIRST ), DEVICE_COUNT );
	printk( KERN_INFO "=============== module removed ==================\n" );
}

module_init( dev_init );
module_exit( dev_exit );
