#pragma once

/* ============================================================
 * Macchina a stati finiti (FSM) del robot — RCJ Rescue Maze 2027
 *
 * Stati:
 *   IDLE          → robot spento, attende il pulsante di start
 *   NAVIGATE      → navigazione autonoma nel labirinto
 *   WALL_FOLLOW   → segue il muro (algoritmo mano destra)
 *   TURN_LEFT     → svolta sinistra 90°
 *   TURN_RIGHT    → svolta destra 90°
 *   TURN_AROUND   → inversione 180°
 *   PUDDLE        → fermo su pozzanghera blu (5 s)
 *   BLACK_HOLE    → buco rilevato, arresto e LoP
 *   CHECKPOINT    → salva posizione su pavimento argento
 *   VICTIM_FOUND  → vittima a <= 15 cm, lampeggio LED 5 s
 *   DROP_KIT      → rilascio kit di soccorso
 *   EXIT_BONUS    → ritorno alla partenza, LED fisso 5 s
 *   EMERGENCY_STOP→ stop da comando Pi o fault
 * ============================================================ */

typedef enum {
    STATE_IDLE,
    STATE_NAVIGATE,
    STATE_WALL_FOLLOW,
    STATE_TURN_LEFT,
    STATE_TURN_RIGHT,
    STATE_TURN_AROUND,
    STATE_PUDDLE,
    STATE_BLACK_HOLE,
    STATE_CHECKPOINT,
    STATE_VICTIM_FOUND,
    STATE_DROP_KIT,
    STATE_EXIT_BONUS,
    STATE_EMERGENCY_STOP
} robot_state_t;

/**
 * Inizializza la FSM (da chiamare dopo tutti i driver).
 */
void fsm_init(void);

/**
 * Aggiorna la FSM — chiamare nel loop().
 */
void fsm_update(void);

/**
 * Ritorna il nome testuale dello stato corrente (per telemetria).
 */
const char *fsm_state_name(void);

/**
 * Forza uno stato esterno (es. da interrupt pulsante LoP).
 */
void fsm_force_state(robot_state_t s);
