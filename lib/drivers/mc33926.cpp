/* Driver basso livello Pololu Dual MC33926 — implementazione. */

#include "mc33926.h"

#include <Arduino.h>

static mc33926_pins_t s_pins;

/* Costante di conversione current sense:
 * FB ~= 525 mV/A  ->  con ADC 10 bit e Vref 5 V:
 *   I [mA] = raw * (5000/1024) [mV] / 0.525 [mV/mA] = raw * 9.30
 * Misura indicativa: serve per stall detection, non per metrologia. */
#define FB_MA_PER_LSB   9.30f

void mc33926_init(const mc33926_pins_t *pins)
{
    s_pins = *pins;

    pinMode(s_pins.m1_dir, OUTPUT);
    pinMode(s_pins.m1_pwm, OUTPUT);
    pinMode(s_pins.m2_dir, OUTPUT);
    pinMode(s_pins.m2_pwm, OUTPUT);

    if (s_pins.en != 0xFF) {
        pinMode(s_pins.en, OUTPUT);
        digitalWrite(s_pins.en, LOW);    /* parti disabilitato */
    }
    if (s_pins.sf != 0xFF) {
        pinMode(s_pins.sf, INPUT);       /* SF e' open-drain: pull-up esterno
                                            gia' presente sul modulo */
    }
    /* FB sono ingressi analogici: nessun pinMode necessario */

    mc33926_stop_all();
}

void mc33926_enable(void)
{
    if (s_pins.en != 0xFF) 
        digitalWrite(s_pins.en, HIGH);
}

void mc33926_disable(void)
{
    if (s_pins.en != 0xFF) 
        digitalWrite(s_pins.en, LOW);
}

void mc33926_set(mc33926_motor_t m, int16_t speed)
{
    uint8_t dir_pin, pwm_pin;
    bool invert;

    if (m == M1) {
        dir_pin = s_pins.m1_dir; 
        pwm_pin = s_pins.m1_pwm; 
        invert = s_pins.invert_m1;
    } else {
        dir_pin = s_pins.m2_dir; 
        pwm_pin = s_pins.m2_pwm; 
        invert = s_pins.invert_m2;
    }

    bool forward = (speed >= 0);
    if (invert) 
        forward = !forward;
    digitalWrite(dir_pin, forward ? HIGH : LOW);

    int16_t duty = (speed >= 0) ? speed : -speed;
    if (duty > 255) 
        duty = 255;
    analogWrite(pwm_pin, (uint8_t)duty);
}

void mc33926_stop(mc33926_motor_t m)
{
    analogWrite((m == M1) ? s_pins.m1_pwm : s_pins.m2_pwm, 0);
}

void mc33926_stop_all(void)
{
    mc33926_stop(M1);
    mc33926_stop(M2);
}

/*
 * Pin FB: se 0xFF (non collegato), ritorna 0.
 * Altrimenti legge ADC e converte in mA.
 */
uint16_t mc33926_current_ma(mc33926_motor_t m)
{
    uint8_t fb = (m == M1) ? s_pins.m1_fb : s_pins.m2_fb;
    if (fb == 0xFF)
        return 0;
    uint16_t raw = (uint16_t)analogRead(fb);
    return (uint16_t)(raw * FB_MA_PER_LSB);
}

bool mc33926_fault(void)
{
    /* SF e' attivo BASSO: LOW = fault */
    if (s_pins.sf == 0xFF) 
        return false;
    return digitalRead(s_pins.sf) == LOW;
}
