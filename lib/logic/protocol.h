#pragma once

/* ============================================================
 * Protocollo di comunicazione seriale Arduino Mega ↔ Raspberry Pi 5
 *
 * Formato messaggi (ASCII, terminatore '\n'):
 *
 * ARDUINO → PI (telemetria):
 *   T:<stato>,<floor>,<front_mm>,<left_mm>,<right_mm>,<hdg_deg>,<kits>\n
 *
 * PI → ARDUINO (comandi):
 *   CMD:VICTIM_H\n   → vittima harmed rilevata (rilascia 2 kit)
 *   CMD:VICTIM_S\n   → vittima stable rilevata (rilascia 1 kit)
 *   CMD:STOP\n       → stop di emergenza
 *   CMD:RESUME\n     → riprendi navigazione
 *
 * Il Pi usa la visione artificiale per classificare le vittime.
 * L'Arduino gestisce autonomamente sensori e navigazione.
 * ============================================================ */

#include <stdint.h>
#include <stdbool.h>

/* Tipo vittima comunicato dal Pi */
typedef enum {
    VICTIM_NONE   = 0,
    VICTIM_HARMED = 1,   /* cerchio rosso  — 2 kit */
    VICTIM_STABLE = 2    /* cerchio verde  — 1 kit */
} victim_type_t;

/* Comandi ricevuti dal Pi */
typedef enum {
    PI_CMD_NONE      = 0,
    PI_CMD_VICTIM_H  = 1,
    PI_CMD_VICTIM_S  = 2,
    PI_CMD_STOP      = 3,
    PI_CMD_RESUME    = 4
} pi_command_t;

/**
 * Inizializza la porta seriale di comunicazione.
 */
void protocol_init(void);

/**
 * Legge i byte in arrivo dal Pi e aggiorna il comando pendente.
 * Chiamare nel loop() ad ogni iterazione. NON blocca.
 */
void protocol_update(void);

/**
 * Ritorna e consuma il prossimo comando ricevuto dal Pi.
 */
pi_command_t protocol_get_command(void);

/**
 * Invia la telemetria corrente al Pi.
 */
void protocol_send_telemetry(const char *state_name,
                             uint8_t     floor,
                             uint16_t    front_mm,
                             uint16_t    left_mm,
                             uint16_t    right_mm,
                             float       heading_deg,
                             uint8_t     kits);

/**
 * Invia un messaggio di log al Pi (prefisso "LOG:").
 */
void protocol_log(const char *msg);
