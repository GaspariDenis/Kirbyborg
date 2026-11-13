#pragma once

/* ============================================================
 * Gestione rilascio kit di soccorso tramite servo.
 *
 * Il servo è montato su un meccanismo che sgancia fisicamente
 * i medikit. Ogni chiamata a rescue_kit_drop() esegue un
 * ciclo: IDLE → DROP → IDLE con un delay controllato.
 * ============================================================ */

#include <stdint.h>

/**
 * Inizializza il servo nella posizione di riposo.
 */
void rescue_kit_init(void);

/**
 * Rilascia un singolo kit (un ciclo servo).
 * Blocca per ~600 ms (tempo meccanico di sgancio).
 */
void rescue_kit_drop_one(void);

/**
 * Rilascia N kit in sequenza.
 * @param count numero di kit da rilasciare (1 o 2).
 */
void rescue_kit_drop(uint8_t count);

/**
 * Ritorna il numero totale di kit ancora disponibili.
 */
uint8_t rescue_kit_remaining(void);
