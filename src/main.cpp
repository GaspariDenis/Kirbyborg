/* ============================================================
 * Kirbyborg — RoboCupJunior Rescue Maze 2027
 * main.cpp — entry point PlatformIO / Arduino Mega 2560
 *
 * Architettura:
 *   - Tutti i driver sono in lib/drivers/
 *   - La logica di gara è in lib/logic/robot_fsm.cpp
 *   - La comunicazione Pi5 è in lib/logic/protocol.cpp
 *   - Questo file: setup(), loop() e ISR pulsante start/LoP
 * ============================================================ */

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "pins.h"

/* Driver */
#include "tof_array.h"
#include "imu.h"
#include "color_apds9960.h"
#include "rescue_kit.h"
#include "motors.h"

/* Logica */
#include "robot_fsm.h"
#include "protocol.h"

/* ----------------------------------------------------------------
 * Timing telemetria verso il Pi (ogni 100 ms, non bloccante)
 * ---------------------------------------------------------------- */
#define TELEM_PERIOD_MS  100UL
static unsigned long s_last_telem_ms = 0;

/* ----------------------------------------------------------------
 * ISR pulsante START / LoP
 * Il pulsante su PIN_START_BUTTON (interrupt-capable) gestisce:
 *   - prima pressione → avvio gara (IDLE → NAVIGATE)
 *   - durante la gara → LoP manuale → riposizionamento arbitro
 * ---------------------------------------------------------------- */
static volatile bool s_start_pressed = false;

static void on_start_button_isr(void)
{
    s_start_pressed = true;
}

/* ----------------------------------------------------------------
 * setup()
 * ---------------------------------------------------------------- */
void setup()
{
    /* Debug seriale (monitor PlatformIO) */
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {}   /* attende apertura serial (max 3 s) */
    Serial.println(F("=== Kirbyborg v1.0 — RCJ Rescue Maze 2027 ==="));

    /* Pulsante start/LoP */
    pinMode(PIN_START_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_START_BUTTON),
                    on_start_button_isr, FALLING);

    /* LED identificazione */
    pinMode(PIN_IDENT_LIGHT, OUTPUT);
    digitalWrite(PIN_IDENT_LIGHT, LOW);

    /* Inizializzazione driver (ordine importante: Wire prima) */
    Wire.begin();

    Serial.println(F("[INIT] Motori..."));
    motors_init();

    Serial.println(F("[INIT] Sensore colore APDS9960..."));
    if (!color_init()) {
        Serial.println(F("[WARN] APDS9960 non inizializzato!"));
    }

    Serial.println(F("[INIT] IMU MPU6050..."));
    if (!imu_init()) {
        Serial.println(F("[WARN] MPU6050 non inizializzato!"));
    }

    Serial.println(F("[INIT] Array ToF VL53L1X..."));
    if (!tof_init()) {
        Serial.println(F("[WARN] Uno o più VL53L1X non trovati!"));
    }

    Serial.println(F("[INIT] Servo kit di soccorso..."));
    rescue_kit_init();

    Serial.println(F("[INIT] Protocollo Pi5..."));
    protocol_init();

    Serial.println(F("[INIT] FSM..."));
    fsm_init();

    Serial.println(F("[INIT] Pronto! Premi START per iniziare."));
}

/* ----------------------------------------------------------------
 * loop()
 * ---------------------------------------------------------------- */
void loop()
{
    /* ---- Gestione pulsante start / LoP ---- */
    if (s_start_pressed) {
        s_start_pressed = false;

        /* Anti-bounce software: ignora pressioni < 300 ms */
        static unsigned long last_btn_ms = 0;
        unsigned long now = millis();
        if (now - last_btn_ms > 300) {
            last_btn_ms = now;

            robot_state_t cur_state_hack = STATE_NAVIGATE;  /* per confronto */
            (void)cur_state_hack;

            const char *name = fsm_state_name();

            if (strcmp(name, "IDLE") == 0) {
                /* Prima pressione: avvio gara */
                Serial.println(F("[BTN] START → NAVIGATE"));
                fsm_force_state(STATE_NAVIGATE);
            } else if (strcmp(name, "BLACK_HOLE") == 0 || strcmp(name, "E_STOP") == 0) {
                /* LoP confermato: riprende dalla posizione del checkpoint */
                Serial.println(F("[BTN] LoP reset → NAVIGATE"));
                fsm_force_state(STATE_NAVIGATE);
            } else {
                /* LoP manuale durante la navigazione */
                Serial.println(F("[BTN] LoP manuale → BLACK_HOLE"));
                fsm_force_state(STATE_BLACK_HOLE);
            }
        }
    }

    /* ---- Leggi comandi Pi ---- */
    protocol_update();

    /* ---- Aggiorna FSM ---- */
    fsm_update();

    /* ---- Telemetria verso Pi (ogni TELEM_PERIOD_MS) ---- */
    unsigned long now = millis();
    if (now - s_last_telem_ms >= TELEM_PERIOD_MS) {
        s_last_telem_ms = now;

        protocol_send_telemetry(
            fsm_state_name(),
            (uint8_t)color_read_floor_color(),
            tof_front_mm(),
            tof_left_mm(),
            tof_right_mm(),
            imu_get_heading_deg(),
            rescue_kit_remaining()
        );
    }
}