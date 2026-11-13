/* ============================================================
 * Implementazione FSM robot — RCJ Rescue Maze 2027
 * ============================================================ */

#include "robot_fsm.h"
#include "protocol.h"
#include "config.h"
#include "pins.h"

/* Driver */
#include "../drivers/motors.h"
#include "../drivers/tof_array.h"
#include "../drivers/imu.h"
#include "../drivers/color_apds9960.h"
#include "../drivers/rescue_kit.h"

#include <Arduino.h>

/* ================================================================
 *  Variabili di stato
 * ================================================================ */
static robot_state_t s_state      = STATE_IDLE;
static robot_state_t s_prev_state = STATE_IDLE;

/* Timestamp ingresso nello stato corrente */
static unsigned long s_state_entry_ms = 0;

/* Checkpoint: posizione logica salvata (tile contatore) */
static uint16_t s_checkpoint_tile = 0;
static uint16_t s_current_tile    = 0;

/* Vittima corrente da gestire */
static victim_type_t s_victim_type = VICTIM_NONE;

/* Flag LED identificazione */
static bool          s_led_on       = false;
static unsigned long s_led_toggle_ms = 0;

/* Target heading per le svolte */
static float s_turn_target_deg = 0.0f;

/* Contatore blink vittima */
static unsigned long s_blink_start_ms = 0;

/* ================================================================
 *  Helper interni
 * ================================================================ */

static unsigned long elapsed_ms(void)
{
    return millis() - s_state_entry_ms;
}

static void enter_state(robot_state_t new_state)
{
    s_prev_state     = s_state;
    s_state          = new_state;
    s_state_entry_ms = millis();

    /* Azioni di ingresso */
    switch (new_state) {
    case STATE_IDLE:
        motors_stop();
        digitalWrite(PIN_IDENT_LIGHT, LOW);
        protocol_log("→ IDLE");
        break;

    case STATE_NAVIGATE:
        protocol_log("→ NAVIGATE");
        break;

    case STATE_WALL_FOLLOW:
        protocol_log("→ WALL_FOLLOW");
        break;

    case STATE_TURN_LEFT:
        s_turn_target_deg = imu_get_heading_deg() - 90.0f;
        if (s_turn_target_deg < 0.0f) s_turn_target_deg += 360.0f;
        imu_set_target_deg(s_turn_target_deg);
        motors_turn_in_place(true, MOTOR_SPEED_TURN);
        protocol_log("→ TURN_LEFT");
        break;

    case STATE_TURN_RIGHT:
        s_turn_target_deg = imu_get_heading_deg() + 90.0f;
        if (s_turn_target_deg >= 360.0f) s_turn_target_deg -= 360.0f;
        imu_set_target_deg(s_turn_target_deg);
        motors_turn_in_place(false, MOTOR_SPEED_TURN);
        protocol_log("→ TURN_RIGHT");
        break;

    case STATE_TURN_AROUND:
        s_turn_target_deg = imu_get_heading_deg() + 180.0f;
        if (s_turn_target_deg >= 360.0f) s_turn_target_deg -= 360.0f;
        imu_set_target_deg(s_turn_target_deg);
        motors_turn_in_place(false, MOTOR_SPEED_TURN);
        protocol_log("→ TURN_AROUND");
        break;

    case STATE_PUDDLE:
        motors_stop();
        protocol_log("→ PUDDLE (attesa 5s)");
        break;

    case STATE_BLACK_HOLE:
        motors_stop();
        protocol_log("→ BLACK_HOLE (LoP)");
        break;

    case STATE_CHECKPOINT:
        motors_stop();
        s_checkpoint_tile = s_current_tile;
        protocol_log("→ CHECKPOINT salvato");
        break;

    case STATE_VICTIM_FOUND:
        motors_stop();
        s_blink_start_ms  = millis();
        s_led_on          = true;
        s_led_toggle_ms   = millis();
        digitalWrite(PIN_IDENT_LIGHT, HIGH);
        protocol_log("→ VICTIM_FOUND");
        break;

    case STATE_DROP_KIT:
        motors_stop();
        protocol_log("→ DROP_KIT");
        break;

    case STATE_EXIT_BONUS:
        motors_stop();
        digitalWrite(PIN_IDENT_LIGHT, HIGH);
        protocol_log("→ EXIT_BONUS");
        break;

    case STATE_EMERGENCY_STOP:
        motors_stop();
        digitalWrite(PIN_IDENT_LIGHT, LOW);
        protocol_log("→ EMERGENCY_STOP");
        break;

    default:
        break;
    }
}

/* ----------------------------------------------------------------
 * Gestione LED blink (non bloccante, 500ms ON / 500ms OFF)
 * ---------------------------------------------------------------- */
static void update_blink_led(void)
{
    unsigned long now = millis();
    if (now - s_led_toggle_ms >= (s_led_on ? VICTIM_BLINK_ON_MS : VICTIM_BLINK_OFF_MS)) {
        s_led_on = !s_led_on;
        digitalWrite(PIN_IDENT_LIGHT, s_led_on ? HIGH : LOW);
        s_led_toggle_ms = now;
    }
}

/* ----------------------------------------------------------------
 * Correzione heading durante la navigazione (P-controller base)
 * ---------------------------------------------------------------- */
static void navigate_with_heading_correction(void)
{
    uint16_t right_dist = tof_right_mm();

    /* Contributo 1: heading IMU (Kp = 1.2) */
    float    hdg_err   = imu_heading_error();
    /* Contributo 2: wall-follow destra (Kp = 0.3) */
    int16_t  wall_err  = (right_dist != TOF_INVALID)
                         ? (int16_t)right_dist - (int16_t)WALL_SAFE_DIST_MM
                         : 0;

    int16_t correction = (int16_t)(hdg_err * 1.2f + wall_err * 0.3f);
    correction = constrain(correction, -80, 80);

    motors_set(MOTOR_SPEED_DEFAULT - correction,
               MOTOR_SPEED_DEFAULT + correction);
}

/* ----------------------------------------------------------------
 * Algoritmo "mano destra": cerca prioritariamente la destra libera,
 * poi avanti, poi sinistra, poi torna indietro.
 * ---------------------------------------------------------------- */
static void decide_next_move(void)
{
    uint16_t front = tof_front_mm();
    uint16_t left  = tof_left_mm();
    uint16_t right = tof_right_mm();

    bool wall_front = (front != TOF_INVALID && front < WALL_PRESENT_DIST_MM);
    bool wall_left  = (left  != TOF_INVALID && left  < WALL_PRESENT_DIST_MM);
    bool wall_right = (right != TOF_INVALID && right < WALL_PRESENT_DIST_MM);

    /* Mano destra: se destra libera → gira a destra */
    if (!wall_right) {
        enter_state(STATE_TURN_RIGHT);
    } else if (!wall_front) {
        /* Avanti: segui muro a destra */
        enter_state(STATE_WALL_FOLLOW);
    } else if (!wall_left) {
        enter_state(STATE_TURN_LEFT);
    } else {
        /* Vicolo cieco: inverti */
        enter_state(STATE_TURN_AROUND);
    }
}

/* ================================================================
 *  Implementazione stati
 * ================================================================ */

static void state_idle(void)
{
    /* Attende il pulsante di start (gestito dall'interrupt in main.cpp) */
}

static void state_navigate(void)
{
    /* Controlla comandi Pi */
    pi_command_t cmd = protocol_get_command();
    if (cmd == PI_CMD_STOP)      { enter_state(STATE_EMERGENCY_STOP); return; }
    if (cmd == PI_CMD_VICTIM_H)  { s_victim_type = VICTIM_HARMED; enter_state(STATE_VICTIM_FOUND); return; }
    if (cmd == PI_CMD_VICTIM_S)  { s_victim_type = VICTIM_STABLE; enter_state(STATE_VICTIM_FOUND); return; }

    /* Leggi sensori */
    uint16_t front = tof_front_mm();

    /* ---- Verifica pavimento ---- */
    floor_color_t floor = color_read_floor_color();

    switch (floor) {
    case FLOOR_BLACK:
        enter_state(STATE_BLACK_HOLE);
        return;
    case FLOOR_BLUE:
        enter_state(STATE_PUDDLE);
        return;
    case FLOOR_SILVER:
        enter_state(STATE_CHECKPOINT);
        return;
    default:
        break;
    }

    /* ---- Vittima da distanza (senza Pi, fallback locale) ---- */
    if (front != TOF_INVALID && front <= VICTIM_DETECT_DIST_MM) {
        /* Aspettiamo il comando Pi; se non arriva, continuiamo */
    }

    /* ---- Decide direzione ---- */
    decide_next_move();
}

static void state_wall_follow(void)
{
    pi_command_t cmd = protocol_get_command();
    if (cmd == PI_CMD_STOP) { enter_state(STATE_EMERGENCY_STOP); return; }
    if (cmd == PI_CMD_VICTIM_H) { s_victim_type = VICTIM_HARMED; enter_state(STATE_VICTIM_FOUND); return; }
    if (cmd == PI_CMD_VICTIM_S) { s_victim_type = VICTIM_STABLE; enter_state(STATE_VICTIM_FOUND); return; }

    uint16_t front = tof_front_mm();

    /* Pavimento */
    floor_color_t floor = color_read_floor_color();
    if (floor == FLOOR_BLACK)  { enter_state(STATE_BLACK_HOLE);  return; }
    if (floor == FLOOR_BLUE)   { enter_state(STATE_PUDDLE);       return; }
    if (floor == FLOOR_SILVER) { enter_state(STATE_CHECKPOINT);   return; }

    /* Muro davanti → decide nuova direzione */
    if (front != TOF_INVALID && front < WALL_PRESENT_DIST_MM) {
        decide_next_move();
        return;
    }

    /* Navigazione in avanti: heading IMU + wall-follow destra */
    navigate_with_heading_correction();

    s_current_tile++;   /* avanzamento di tile (approssimativo) */
}

static void state_turn(bool is_left_or_around)
{
    (void)is_left_or_around;

    float err = fabsf(imu_heading_error());
    if (err < (float)HEADING_TOL_DEG) {
        motors_stop();
        delay(100);   /* breve pausa per stabilizzazione meccanica */
        enter_state(STATE_WALL_FOLLOW);
    }
    /* I motori continuano a girare (impostati in enter_state) */
}

static void state_puddle(void)
{
    if (elapsed_ms() >= PUDDLE_STOP_MS) {
        protocol_log("PUDDLE: attesa completata");
        enter_state(STATE_NAVIGATE);
    }
    /* Il robot rimane fermo */
}

static void state_black_hole(void)
{
    /* LoP: il robot è fermo, attende reset fisico dell'arbitro.
     * Quando il pulsante viene premuto, l'interrupt in main.cpp
     * chiama fsm_force_state(STATE_NAVIGATE). */
}

static void state_checkpoint(void)
{
    /* Il pavimento argento è largo una tile: attraversiamola lentamente,
       poi torniamo a navigare. */
    if (elapsed_ms() < 500) {
        motors_forward(MOTOR_SPEED_SLOW);
    } else {
        motors_stop();
        enter_state(STATE_NAVIGATE);
    }
}

static void state_victim_found(void)
{
    /* Lampeggio LED per 5 secondi */
    update_blink_led();

    if (millis() - s_blink_start_ms >= VICTIM_BLINK_MS) {
        /* Fine lampeggio: vai a rilasciare il kit */
        digitalWrite(PIN_IDENT_LIGHT, LOW);
        enter_state(STATE_DROP_KIT);
    }
}

static void state_drop_kit(void)
{
    uint8_t count = (s_victim_type == VICTIM_HARMED) ? KITS_FOR_HARMED : KITS_FOR_STABLE;
    rescue_kit_drop(count);
    s_victim_type = VICTIM_NONE;
    /* Breve attesa, poi riprende la navigazione */
    delay(500);
    enter_state(STATE_NAVIGATE);
}

static void state_exit_bonus(void)
{
    /* LED fisso per EXIT_BONUS_MS */
    if (elapsed_ms() >= EXIT_BONUS_MS) {
        digitalWrite(PIN_IDENT_LIGHT, LOW);
        enter_state(STATE_IDLE);
        protocol_log("EXIT_BONUS completato!");
    }
}

static void state_emergency_stop(void)
{
    /* Attende CMD:RESUME dal Pi */
    pi_command_t cmd = protocol_get_command();
    if (cmd == PI_CMD_RESUME) {
        enter_state(STATE_NAVIGATE);
    }
}

/* ================================================================
 *  API pubblica
 * ================================================================ */

void fsm_init(void)
{
    s_state          = STATE_IDLE;
    s_state_entry_ms = millis();
    s_checkpoint_tile = 0;
    s_current_tile    = 0;
    s_victim_type     = VICTIM_NONE;

    pinMode(PIN_IDENT_LIGHT, OUTPUT);
    digitalWrite(PIN_IDENT_LIGHT, LOW);

    protocol_log("FSM inizializzata");
}

void fsm_update(void)
{
    /* Aggiorna sensori */
    imu_update();
    tof_update();

    switch (s_state) {
    case STATE_IDLE:            state_idle();                break;
    case STATE_NAVIGATE:        state_navigate();            break;
    case STATE_WALL_FOLLOW:     state_wall_follow();         break;
    case STATE_TURN_LEFT:       state_turn(true);            break;
    case STATE_TURN_RIGHT:      state_turn(false);           break;
    case STATE_TURN_AROUND:     state_turn(false);           break;
    case STATE_PUDDLE:          state_puddle();              break;
    case STATE_BLACK_HOLE:      state_black_hole();          break;
    case STATE_CHECKPOINT:      state_checkpoint();          break;
    case STATE_VICTIM_FOUND:    state_victim_found();        break;
    case STATE_DROP_KIT:        state_drop_kit();            break;
    case STATE_EXIT_BONUS:      state_exit_bonus();          break;
    case STATE_EMERGENCY_STOP:  state_emergency_stop();      break;
    default: break;
    }
}

const char *fsm_state_name(void)
{
    switch (s_state) {
    case STATE_IDLE:            return "IDLE";
    case STATE_NAVIGATE:        return "NAVIGATE";
    case STATE_WALL_FOLLOW:     return "WALL_FOLLOW";
    case STATE_TURN_LEFT:       return "TURN_L";
    case STATE_TURN_RIGHT:      return "TURN_R";
    case STATE_TURN_AROUND:     return "TURN_180";
    case STATE_PUDDLE:          return "PUDDLE";
    case STATE_BLACK_HOLE:      return "BLACK_HOLE";
    case STATE_CHECKPOINT:      return "CHECKPOINT";
    case STATE_VICTIM_FOUND:    return "VICTIM";
    case STATE_DROP_KIT:        return "DROP_KIT";
    case STATE_EXIT_BONUS:      return "EXIT_BONUS";
    case STATE_EMERGENCY_STOP:  return "E_STOP";
    default:                    return "UNKNOWN";
    }
}

void fsm_force_state(robot_state_t s)
{
    enter_state(s);
}
