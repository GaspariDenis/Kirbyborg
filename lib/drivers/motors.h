#pragma once

/* Livello ALTO per il controllo dei 2 motori del robot.
 * Basato sul driver basso livello mc33926 (Pololu Dual MC33926).
 *
 * Convenzione: M1 = motore SINISTRO, M2 = motore DESTRO.
 * Speed: -255 .. +255 (segno = direzione). */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void motors_init(void);

void motors_set(int16_t left, int16_t right);
void motors_stop(void);
void motors_forward(int16_t speed);
void motors_backward(int16_t speed);
void motors_turn_in_place(bool left, int16_t speed);

void motors_enable(void);          /* EN = HIGH (chiamare all'avvio del run) */
void motors_disable(void);         /* EN = LOW: motori liberi (es. LoP) */

/* Diagnostica */
bool     motors_fault(void);                          /* fault dal pin SF */
uint16_t motors_current_ma(uint8_t side);             /* side: 0=SX, 1=DX */
bool     motors_stalled(uint8_t side);                /* corrente > MOTOR_STALL_MA */

#ifdef __cplusplus
}
#endif