#pragma once

#include "csingle.h"
#include "mpu6050_ioctl.h"
#include <string>

#define MPU6050_DEVICE_DATA_SWAPPED 1

void set(mpu6050_simple_values& r, const mpu6050_simple_values& v);
void set(mpu6050_values& r, const mpu6050_values& v);
void set(mpu6050_div_values& r, const mpu6050_div_values& v);

void div(mpu6050_simple_values& r, const int d);
void div(mpu6050_values& r, const mpu6050_div_values& d);

std::string toString(mpu6050_simple_values& v);
std::string toString(const char* name, mpu6050_simple_values& v);
std::string toString(mpu6050_values& v);
std::string toString(mpu6050_div_values& v);

class CMpu6050 : public CSingleTone<CMpu6050> {
    friend class CSingleTone<CMpu6050>;
private:
    static const char   *MPU6050_DEVICE;
    static int          m_mpu6050;

    mpu6050_values      m_val;
    mpu6050_div_values  m_div;
    mpu6050_values      m_coord;

    ~CMpu6050();
    CMpu6050();
public:
    int init();
    void deinit();

    int getAccelX();
    int getAccelY();
    int getAccelZ();
    int getGyroX();
    int getGyroY();
    int getGyroZ();
    int getAccel(mpu6050_simple_values& v);
    int getGyro(mpu6050_simple_values& v);
    int getTemperature();
    int getAll(mpu6050_values& v);
    int getDiv(mpu6050_div_values& v);
    int getAll();
    int getDiv();

    inline mpu6050_div_values& div() { return m_div; }
    inline mpu6050_values& val() { return m_val; }

    mpu6050_values& coord();

    std::string toString();
};
