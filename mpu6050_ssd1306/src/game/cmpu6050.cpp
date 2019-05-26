#include "cmpu6050.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sstream>
#include <iostream>

const char * CMpu6050::MPU6050_DEVICE = "/dev/mpu6050_x";
int CMpu6050::m_mpu6050 = -1;

void set(mpu6050_simple_values& r, const mpu6050_simple_values& v)
{
    r.x = v.x;
    r.y = v.y;
    r.z = v.z;
}
void set(mpu6050_values& r, const mpu6050_values& v)
{
    set(r.accel, v.accel);
    set(r.gyro, v.gyro);
    r.temperature = v.temperature;
}
void set(mpu6050_div_values& r, const mpu6050_div_values& v)
{
    r.accel = v.accel;
    r.gyro = v.gyro;
}

void div(mpu6050_simple_values& r, const int d)
{
    if (!d) {
        std::cerr << "Devide to zero";
        return;
    }
    r.x = 10 * r.x / d;
    r.y = 10 * r.y / d;
    r.z = 10 * r.z / d;
}

void div(mpu6050_values& r, const mpu6050_div_values& d)
{
    r.accel.x = 10 * r.accel.x / d.accel;
    r.accel.y = 10 * r.accel.y / d.accel;
    r.accel.z = 10 * r.accel.z / d.accel;

    r.gyro.x = 10 * r.gyro.x / d.gyro;
    r.gyro.y = 10 * r.gyro.y / d.gyro;
    r.gyro.z = 10 * r.gyro.z / d.gyro;
}

std::string toString(mpu6050_simple_values& v)
{
    std::ostringstream str;
    str << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return str.str();
}

std::string toString(const char* name, mpu6050_simple_values& v)
{
    return std::string(name) + toString(v);
}

std::string toString(mpu6050_values& v)
{
    std::ostringstream str;
    str << toString("accel", v.accel) << " " << toString("gyro", v.gyro) << " t:" << v.temperature;
    return str.str();
}

std::string toString(mpu6050_div_values& v)
{
    std::ostringstream str;
    str << "div [" << v.accel << ", " << v.gyro << "]";
    return str.str();
}

CMpu6050::CMpu6050()
{
}

CMpu6050::~CMpu6050()
{
}

int CMpu6050::init()
{
    //fprintf(stdout, "CMpu6050::CMpu6050().\n");
    m_mpu6050 = open(MPU6050_DEVICE, O_RDONLY);
    if (m_mpu6050 < 0) throw "Cannot open device for mpu6050";

    fprintf(stdout, " m_mpu6050: %d.\n", m_mpu6050);
    getDiv();
    return 0;
}

void CMpu6050::deinit()
{
    if (m_mpu6050 >= 0) close(m_mpu6050);
}

int CMpu6050::getAccelX()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_ACCEL_X, &m_val.accel.x)) {
        fprintf(stderr, "Ioctl MPU6050_GET_ACCEL_X error.\n");
        return 0;
    }
    return m_val.accel.x;
}

int CMpu6050::getAccelY()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_ACCEL_Y, &m_val.accel.y)) {
        fprintf(stderr, "Ioctl MPU6050_GET_ACCEL_Y error.\n");
        return 0;
    }
    return m_val.accel.y;
}

int CMpu6050::getAccelZ()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_ACCEL_Z, &m_val.accel.z)) {
        fprintf(stderr, "Ioctl MPU6050_GET_ACCEL_Z error.\n");
        return 0;
    }
    return m_val.accel.z;
}

int CMpu6050::getGyroX()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_GYRO_X, &m_val.gyro.x)) {
        fprintf(stderr, "Ioctl MPU6050_GET_GYRO_X error.\n");
        return 0;
    }
    return m_val.gyro.x;
}

int CMpu6050::getGyroY()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_GYRO_Y, &m_val.gyro.y)) {
        fprintf(stderr, "Ioctl MPU6050_GET_GYRO_Y error.\n");
        return 0;
    }
    return m_val.gyro.y;
}

int CMpu6050::getGyroZ()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_GYRO_Z, &m_val.gyro.z)) {
        fprintf(stderr, "Ioctl MPU6050_GET_GYRO_Z error.\n");
        return 0;
    }
    return m_val.gyro.z;
}

int CMpu6050::getAccel(mpu6050_simple_values& v)
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_ACCEL_VAL, &m_val.accel)) {
        fprintf(stderr, "Ioctl MPU6050_GET_ACCEL_VAL error.\n");
        return -1;
    }
    set(v, m_val.accel);
    return 0;
}

int CMpu6050::getGyro(mpu6050_simple_values& v)
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_ACCEL_VAL, &m_val.gyro)) {
        fprintf(stderr, "Ioctl MPU6050_GET_ACCEL_VAL error.\n");
        return -1;
    }
    set(v, m_val.gyro);
    return 0;
}

int CMpu6050::getTemperature()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_TEMPERATURE, &m_val.temperature)) {
        fprintf(stderr, "Ioctl MPU6050_GET_TEMPERATURE error.\n");
        return 0;
    }
    return m_val.temperature;
}

int CMpu6050::getAll(mpu6050_values &v)
{
    if (getAll() < 0) return -1;
    set(v, m_val);
    return 0;
}

int CMpu6050::getDiv(mpu6050_div_values& v)
{
    if (getDiv() < 0) return -1;
    set(v, m_div);
    return 0;
}

int CMpu6050::getAll()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_ALL_VAL, &m_val)) {
        fprintf(stderr, "Ioctl MPU6050_GET_ALL_VAL error.\n");
        return -1;
    }
    return 0;
}

int CMpu6050::getDiv()
{
    if (-1 == ioctl(m_mpu6050, MPU6050_GET_DIV_VAL, &m_div)) {
        fprintf(stderr, "Ioctl MPU6050_GET_DIV_VAL error.\n");
        return -1;
    }
    return 0;
}

mpu6050_values& CMpu6050::coord()
{
    set(m_coord, m_val);
#ifndef MPU6050_DEVICE_DATA_SWAPPED
    /**
      * for normal MPU6050 devices you can use call ::div(m_coord, m_div);
      */
    ::div(m_coord, m_div);
#else
    /**
      * for swap coord device:
      */
    ::div(m_coord.accel, m_div.gyro);
    ::div(m_coord.gyro, m_div.accel);
#endif
    return m_coord;
}

std::string CMpu6050::toString()
{
    return ::toString(m_val) + ::toString(m_div);
}
