/* ============================================================
 * Implementazione driver array 6x VL53L1X + TCA9548A.
 * ============================================================ */

#include "tof_array.h"
#include "pins.h"

#include <Arduino.h>
#include <Wire.h>
#include <TCA9548.h>
#include <Adafruit_VL53L1X.h>

/* Il TCA9548A è sull'indirizzo I2C_ADDR_MUX (0x70 di default) */
static TCA9548 s_mux(I2C_ADDR_MUX);

/* Un'unica istanza VL53L1X: viene reinizializzata ogni volta che
   si cambia canale del mux, poiché tutti i chip hanno stesso addr */
static Adafruit_VL53L1X s_tof[TOF_COUNT];

/* Stato inizializzazione per ogni sensore */
static bool s_ok[TOF_COUNT] = {false};

/* Buffer distanze (mm) */
static uint16_t s_dist_mm[TOF_COUNT];

/* Mappa canale mux → indice logico sensore */
static const uint8_t CH_MAP[TOF_COUNT] = {0, 1, 2, 3, 4, 5};

/* ---- helper interno ---- */
static void select_channel(uint8_t ch)
{
    s_mux.selectChannel(ch);
    delayMicroseconds(100);   /* piccola pausa per stabilizzare il bus */
}

bool tof_init(void)
{
    Wire.begin();

    /* Inizializza il mux */
    if (!s_mux.begin()) {
        Serial.println(F("[TOF] ERRORE: TCA9548A non trovato!"));
        return false;
    }
    s_mux.disableAllChannels();

    bool all_ok = true;

    for (uint8_t i = 0; i < TOF_COUNT; i++) {
        uint8_t ch = CH_MAP[i];
        select_channel(ch);

        if (!s_tof[i].begin(0x29, &Wire)) {
            Serial.print(F("[TOF] ERRORE: VL53L1X CH"));
            Serial.print(ch);
            Serial.println(F(" non trovato"));
            s_ok[i]      = false;
            s_dist_mm[i] = TOF_INVALID;
            all_ok       = false;
        } else {
            /* Timing budget 50 ms, buon compromesso velocità/precisione */
            s_tof[i].setTimingBudget(50);
            s_tof[i].startRanging();
            s_ok[i]      = true;
            s_dist_mm[i] = TOF_INVALID;
            Serial.print(F("[TOF] CH"));
            Serial.print(ch);
            Serial.println(F(" OK"));
        }

        s_mux.disableAllChannels();
    }

    return all_ok;
}

void tof_update(void)
{
    for (uint8_t i = 0; i < TOF_COUNT; i++) {
        if (!s_ok[i]) continue;

        select_channel(CH_MAP[i]);

        if (s_tof[i].dataReady()) {
            int16_t d = s_tof[i].distance();
            s_tof[i].clearInterrupt();

            if (d > 0 && d < 4000) {
                s_dist_mm[i] = (uint16_t)d;
            } else {
                s_dist_mm[i] = TOF_INVALID;
            }
        }
        /* Se non ci sono dati pronti, manteniamo l'ultimo valore */

        s_mux.disableAllChannels();
    }
}

uint16_t tof_get_mm(tof_sensor_t sensor)
{
    if (sensor >= TOF_COUNT) return TOF_INVALID;
    return s_dist_mm[sensor];
}

/* Media di due valori, esclude TOF_INVALID */
static uint16_t avg2(uint16_t a, uint16_t b)
{
    if (a == TOF_INVALID && b == TOF_INVALID) return TOF_INVALID;
    if (a == TOF_INVALID) return b;
    if (b == TOF_INVALID) return a;
    return (uint16_t)((uint32_t)(a + b) / 2);
}

uint16_t tof_front_mm(void)
{
    return avg2(s_dist_mm[TOF_FRONT_LEFT], s_dist_mm[TOF_FRONT_RIGHT]);
}

uint16_t tof_left_mm(void)
{
    return avg2(s_dist_mm[TOF_LEFT_FRONT], s_dist_mm[TOF_LEFT_REAR]);
}

uint16_t tof_right_mm(void)
{
    return avg2(s_dist_mm[TOF_RIGHT_FRONT], s_dist_mm[TOF_RIGHT_REAR]);
}

void tof_print_debug(void)
{
    Serial.print(F("TOF[F_L="));
    Serial.print(s_dist_mm[TOF_FRONT_LEFT]);
    Serial.print(F(" F_R="));
    Serial.print(s_dist_mm[TOF_FRONT_RIGHT]);
    Serial.print(F(" L_F="));
    Serial.print(s_dist_mm[TOF_LEFT_FRONT]);
    Serial.print(F(" L_R="));
    Serial.print(s_dist_mm[TOF_LEFT_REAR]);
    Serial.print(F(" R_F="));
    Serial.print(s_dist_mm[TOF_RIGHT_FRONT]);
    Serial.print(F(" R_R="));
    Serial.print(s_dist_mm[TOF_RIGHT_REAR]);
    Serial.println(F("]"));
}
