/* ============================================================
 * Implementazione driver rescue kit (servo).
 * ============================================================ */

#include "rescue_kit.h"
#include "pins.h"

#include <Arduino.h>
#include <Servo.h>

static Servo   s_servo;
static uint8_t s_kits_remaining = 4;   /* caricati manualmente prima della gara */

void rescue_kit_init(void)
{
    s_servo.attach(PIN_SERVO_KIT);
    s_servo.write(SERVO_POS_IDLE);
    delay(300);
    Serial.println(F("[KIT] Servo kit inizializzato"));
}

void rescue_kit_drop_one(void)
{
    if (s_kits_remaining == 0) {
        Serial.println(F("[KIT] ATTENZIONE: nessun kit disponibile!"));
        return;
    }

    /* Vai in posizione di rilascio */
    s_servo.write(SERVO_POS_DROP);
    delay(500);

    /* Ritorna in posizione di riposo */
    s_servo.write(SERVO_POS_IDLE);
    delay(300);

    s_kits_remaining--;

    Serial.print(F("[KIT] Rilasciato. Rimasti: "));
    Serial.println(s_kits_remaining);
}

void rescue_kit_drop(uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        rescue_kit_drop_one();
        if (i < count - 1) delay(400); /* pausa tra kit multipli */
    }
}

uint8_t rescue_kit_remaining(void)
{
    return s_kits_remaining;
}
