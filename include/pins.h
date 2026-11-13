#pragma once

/* ============================================================
 * Mappatura pin — Arduino Mega 2560 (adattare se cambia board)
 * ============================================================ */

// Fake pin 
#define PIN_FAKE            0xFF

/* Pulsante di start / procedura LoP.
 * §4.2.8: UN unico pulsante fisico ben visibile all'arbitro. */
#define PIN_START_BUTTON    2    /* interrupt-capable */

/* Identification Light.
 * §4.2.11: LED dedicato SOLO a identificazione vittime ed exit bonus,
 * ben visibile all'arbitro. */
#define PIN_IDENT_LIGHT     13

/* Servo rilascio medikit */
#define PIN_SERVO_KIT       9
#define SERVO_POS_IDLE      10   /* gradi: posizione di riposo  */
#define SERVO_POS_DROP      120  /* gradi: posizione di rilascio */

/* ---- Driver motori: Pololu Dual MC33926 ----
 * M1 = motore sinistro, M2 = motore destro. */
#define PIN_M1_DIR          4
#define PIN_M1_PWM          5
#define PIN_M1_FB           PIN_FAKE   /* current sense motore 1 */
#define PIN_M2_DIR          7
#define PIN_M2_PWM          6
#define PIN_M2_FB           PIN_FAKE   /* current sense motore 2 */
#define PIN_MC33926_EN      PIN_FAKE    /* enable (HIGH = attivo) */
#define PIN_MC33926_SF      PIN_FAKE   /* status flag (LOW = fault) */

/* Veri se i motori sono cablati con polarita' invertita */
#define MOTOR_L_INVERTED    false
#define MOTOR_R_INVERTED    false

/* Stall detection: corrente oltre la quale il motore si
 * considera bloccato (da tarare con il carico reale) */
#define MOTOR_STALL_MA      2500

/* ---- Bus I2C (SDA=20, SCL=21 sul Mega) ----
 *  - TCA9548A @ 0x70  -> 6x VL53L1X sui canali 0..5
 *  - MPU6050  @ 0x68  -> bus principale
 *  - APDS9960 @ 0x39  -> bus principale
 *  Indirizzi diversi -> nessun conflitto fuori dal mux. */
#define I2C_ADDR_MUX        0x70
