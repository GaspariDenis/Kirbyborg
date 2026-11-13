/* ============================================================
 * Implementazione driver IMU MPU6050.
 * ============================================================ */

#include "imu.h"

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

static Adafruit_MPU6050 s_mpu;

static float s_heading_deg = 0.0f;   /* heading integrato */
static float s_target_deg  = 0.0f;   /* heading desiderato */
static unsigned long s_last_us = 0;  /* timestamp ultimo aggiornamento (µs) */

bool imu_init(void)
{
    if (!s_mpu.begin()) {
        Serial.println(F("[IMU] ERRORE: MPU6050 non trovato!"));
        return false;
    }

    /* Range giroscopio ±250°/s, sufficiente per le manovre in gara */
    s_mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    s_mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    s_heading_deg = 0.0f;
    s_target_deg  = 0.0f;
    s_last_us     = micros();

    Serial.println(F("[IMU] MPU6050 OK"));
    return true;
}

void imu_update(void)
{
    sensors_event_t accel, gyro, temp;
    s_mpu.getEvent(&accel, &gyro, &temp);

    unsigned long now_us = micros();
    float dt_s = (float)(now_us - s_last_us) / 1e6f;
    s_last_us  = now_us;

    /* Integra velocità angolare Z (asse verticale).
       MPU6050 restituisce rad/s — convertiamo in deg/s */
    float gz_dps = gyro.gyro.z * (180.0f / (float)M_PI);

    s_heading_deg += gz_dps * dt_s;

    /* Normalizza in [0, 360) */
    while (s_heading_deg <   0.0f) s_heading_deg += 360.0f;
    while (s_heading_deg >= 360.0f) s_heading_deg -= 360.0f;
}

float imu_get_heading_deg(void)
{
    return s_heading_deg;
}

void imu_reset_heading(void)
{
    s_heading_deg = 0.0f;
    s_target_deg  = 0.0f;
}

void imu_set_target_deg(float deg)
{
    while (deg <   0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    s_target_deg = deg;
}

float imu_get_target_deg(void)
{
    return s_target_deg;
}

float imu_heading_error(void)
{
    float err = s_heading_deg - s_target_deg;
    /* Normalizza a -180 .. +180 */
    while (err >  180.0f) err -= 360.0f;
    while (err < -180.0f) err += 360.0f;
    return err;
}

void imu_print_debug(void)
{
    Serial.print(F("IMU[hdg="));
    Serial.print(s_heading_deg, 1);
    Serial.print(F(" tgt="));
    Serial.print(s_target_deg, 1);
    Serial.print(F(" err="));
    Serial.print(imu_heading_error(), 1);
    Serial.println(F("]"));
}
