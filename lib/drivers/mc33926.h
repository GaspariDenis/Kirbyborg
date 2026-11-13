#pragma once

/* ============================================================
 * Driver basso livello per Pololu Dual MC33926 Motor Driver
 * (carrier o shield — i pin sono configurabili).
 *
 * Pin del modulo usati:
 *   MxDIR : direzione (HIGH = avanti, vedi invert_*)
 *   MxPWM : velocita' (PWM, duty 0..255)
 *   MxFB  : current sense analogico (~525 mV per Ampere)
 *   EN    : enable modulo (HIGH = attivo; sul carrier va pilotato,
 *           di default e' disattivo) -> mettere 0xFF se non collegato
 *   SF    : status flag (LOW = fault: sovracorrente, sovratemperatura,
 *           sottotensione, cortocircuito) -> 0xFF se non collegato
 *
 * Note:
 *  - Frequenza PWM Arduino di default (~490 Hz) OK; il chip regge
 *    fino a 20 kHz se serve silenziosita'.
 *  - Con PWM = 0 le uscite vanno in coast/brake a seconda del
 *    collegamento interno della scheda: verificare sul datasheet
 *    della versione (carrier vs shield) in uso.
 *  - Tensione motori: 5..28 V, 3 A continui per canale (picco 5 A).
 * ============================================================ */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { M1 = 0, M2 = 1 } mc33926_motor_t;

typedef struct {
    uint8_t m1_dir, m1_pwm, m1_fb;
    uint8_t m2_dir, m2_pwm, m2_fb;
    uint8_t en;              /* pin EN, 0xFF = non collegato */
    uint8_t sf;              /* pin SF, 0xFF = non collegato */
    bool    invert_m1;       /* true se il motore e' cablato al contrario */
    bool    invert_m2;
} mc33926_pins_t;

void     mc33926_init(const mc33926_pins_t *pins);

void     mc33926_enable(void);              /* EN = HIGH */
void     mc33926_disable(void);             /* EN = LOW: uscite in tri-state */

void     mc33926_set(mc33926_motor_t m, int16_t speed); /* -255 .. +255, segno = direzione */
void     mc33926_stop(mc33926_motor_t m);               /* duty 0 (coast/brake) */
void     mc33926_stop_all(void);

uint16_t mc33926_current_ma(mc33926_motor_t m);         /* corrente stimata da FB (mA) */
bool     mc33926_fault(void);                           /* true se SF segnala fault */

#ifdef __cplusplus
}
#endif