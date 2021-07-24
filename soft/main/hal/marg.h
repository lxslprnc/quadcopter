#pragma once

#include <periph/i2c_master.h>
#include <drv/MPU6050.h>
#include <drv/HMC5883L.h>
#include <os/mutex.h>

using namespace std;

class Marg
{

  private: 

    /* Attributes */

    I2cMaster  * _i2c;      ///< i2c device
    MPU6050    * _mpu;
    HMC5883L   * _hmc;
    Mutex      * _mutex;

  public:

    /* Constructors */

    Marg(I2cMaster * i2c);

    /* Accessors*/

    /* Other methods */

    esp_err_t init(void);

    esp_err_t read_acc(float * acc_x, float * acc_y, float * acc_z);

    esp_err_t read_gyro(float * gyro_x, float * gyro_y, float * gyro_z);

    esp_err_t read_acc_gyro(float *acc_x, float *acc_y, float *acc_z,
                            float *gyro_x, float *gyro_y, float *gyro_z);

    esp_err_t read_mag(float * mag_x, float * mag_y, float * mag_z);
};