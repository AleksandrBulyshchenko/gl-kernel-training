#pragma once

#include <linux/ioctl.h>

/* ioctl()'s for the mpu6050 values read */

/* Get the entropy count. */
#define MPU6050_GET_ACCEL_X		_IOR( 'R', 0x01, int )
#define MPU6050_GET_ACCEL_Y		_IOR( 'R', 0x02, int )
#define MPU6050_GET_ACCEL_Z		_IOR( 'R', 0x03, int )
#define MPU6050_GET_GYRO_X		_IOR( 'R', 0x04, int )
#define MPU6050_GET_GYRO_Y		_IOR( 'R', 0x05, int )
#define MPU6050_GET_GYRO_Z		_IOR( 'R', 0x06, int )

struct mpu6050_simple_values {
	union {
		struct {
			int x, y, z;
		};
		int data[3];
	};
};

#define MPU6050_GET_ACCEL_VAL	_IOR( 'R', 0x07, unsigned long )
#define MPU6050_GET_GYRO_VAL	_IOR( 'R', 0x08, unsigned long )
#define MPU6050_GET_TEMPERATURE	_IOR( 'R', 0x09, int )


struct mpu6050_values {
	union {
		struct {
            struct mpu6050_simple_values accel;
            struct mpu6050_simple_values gyro;
			int temperature;
		};
		int data[7];
	};
};

#define MPU6050_GET_ALL_VAL		_IOR( 'R', 0x0a, unsigned long )

struct mpu6050_div_values {
	union {
		struct {
			int accel, gyro;
		};
		int data[2];
	};

};
#define MPU6050_GET_DIV_VAL		_IOR( 'R', 0x0b, unsigned long )


