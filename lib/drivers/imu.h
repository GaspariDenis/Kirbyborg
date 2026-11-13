#pragma once

/* ============================================================
 * Driver IMU — Adafruit MPU6050
 *
 * Fornisce:
 *  - heading (yaw integrato, in gradi 0-359)
 *  - rilevamento collisione / stall tramite accelerometro
 * ============================================================ */

#include <stdbool.h>

/**
 * Inizializza l'MPU6050.
 * @return true se trovato e configurato correttamente.
 */
bool imu_init(void);

/**
 * Aggiorna lo stato IMU (integra giroscopio).
 * Chiamare nel loop() ad ogni iterazione.
 */
void imu_update(void);

/**
 * Ritorna l'heading corrente in gradi [0.0, 360.0).
 * Cresce in senso orario (destra = +).
 */
float imu_get_heading_deg(void);

/**
 * Azzera l'heading (imposta il target corrente come 0°).
 */
void imu_reset_heading(void);

/**
 * Imposta il target di heading (es. dopo una svolta a 90°).
 */
void imu_set_target_deg(float deg);

/**
 * Ritorna il target di heading attuale.
 */
float imu_get_target_deg(void);

/**
 * Errore di heading rispetto al target (-180..+180).
 * Positivo = devo girare a destra.
 */
float imu_heading_error(void);

/**
 * Stampa diagnostica.
 */
void imu_print_debug(void);
