#pragma once

/* ============================================================
 * Driver array 6x VL53L1X gestiti tramite TCA9548A I2C mux.
 *
 * Layout fisico (canali del TCA9548A):
 *   CH0 → Frontale SINISTRO
 *   CH1 → Frontale DESTRO
 *   CH2 → Laterale SINISTRO ANTERIORE
 *   CH3 → Laterale SINISTRO POSTERIORE
 *   CH4 → Laterale DESTRO ANTERIORE
 *   CH5 → Laterale DESTRO POSTERIORE
 *
 * Tutti i VL53L1X condividono l'indirizzo di default 0x29;
 * vengono attivati uno alla volta tramite il mux.
 * ============================================================ */

#include <stdint.h>
#include <stdbool.h>

/* Indici logici dei sensori */
typedef enum {
    TOF_FRONT_LEFT  = 0,
    TOF_FRONT_RIGHT = 1,
    TOF_LEFT_FRONT  = 2,
    TOF_LEFT_REAR   = 3,
    TOF_RIGHT_FRONT = 4,
    TOF_RIGHT_REAR  = 5,
    TOF_COUNT       = 6
} tof_sensor_t;

/* Distanza invalida / fuori range */
#define TOF_INVALID   0xFFFF

/**
 * Inizializza il mux TCA9548A e tutti e 6 i VL53L1X.
 * Deve essere chiamata una sola volta in setup().
 * @return true se tutti i sensori sono stati inizializzati, false altrimenti.
 */
bool tof_init(void);

/**
 * Aggiorna le letture di tutti i sensori in sequenza.
 * Chiamare nel loop() ad ogni iterazione.
 * Usa il timing non-bloccante: se il sensore non ha dati pronti,
 * mantiene l'ultimo valore valido.
 */
void tof_update(void);

/**
 * Ritorna l'ultima distanza letta per il sensore indicato (mm).
 * Ritorna TOF_INVALID se il sensore non è disponibile o fuori range.
 */
uint16_t tof_get_mm(tof_sensor_t sensor);

/**
 * Distanza frontale media (media dei due sensori frontali, mm).
 */
uint16_t tof_front_mm(void);

/**
 * Distanza laterale sinistra media (mm).
 */
uint16_t tof_left_mm(void);

/**
 * Distanza laterale destra media (mm).
 */
uint16_t tof_right_mm(void);

/**
 * Stampa diagnostica su Serial (debug).
 */
void tof_print_debug(void);
