/* Livello alto motori, sopra il driver Pololu Dual MC33926. */

#include "motors.h"
#include "mc33926.h"
#include "pins.h"

#include <Arduino.h>

void motors_init(void)
{
    static const mc33926_pins_t pins = {
        .m1_dir = PIN_M1_DIR,
        .m1_pwm = PIN_M1_PWM,
        .m1_fb  = PIN_M1_FB,
        .m2_dir = PIN_M2_DIR,
        .m2_pwm = PIN_M2_PWM,
        .m2_fb  = PIN_M2_FB,
        .en     = PIN_MC33926_EN,
        .sf     = PIN_MC33926_SF,
        .invert_m1 = MOTOR_L_INVERTED,
        .invert_m2 = MOTOR_R_INVERTED,
    };
    mc33926_init(&pins);
    mc33926_enable();
}

void motors_set(int16_t left, int16_t right)
{
    mc33926_set(M1, left);
    mc33926_set(M2, right);
}

void motors_stop(void)
{
    mc33926_stop_all();
}

void motors_forward(int16_t speed)
{
    motors_set(speed, speed);
}

void motors_backward(int16_t speed)
{
    motors_set(-speed, -speed);
}

void motors_turn_in_place(bool left, int16_t speed)
{
    if (left) motors_set(-speed, speed);
    else      motors_set(speed, -speed);
}

void motors_enable(void)  { mc33926_enable(); }
void motors_disable(void) { mc33926_stop_all(); mc33926_disable(); }

bool motors_fault(void)
{
    return mc33926_fault();
}

uint16_t motors_current_ma(uint8_t side)
{
    return mc33926_current_ma(side == 0 ? M1 : M2);
}

bool motors_stalled(uint8_t side)
{
    return motors_current_ma(side) > MOTOR_STALL_MA;
}
