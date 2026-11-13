#include "color_apds9960.h"
#include <Adafruit_APDS9960.h>
#include <Arduino.h>

static Adafruit_APDS9960 apds;

/* ============================================================
 * Soglie colore pavimento — calibrare con il robot reale!
 *
 * Metodo: leggere i valori r,g,b,c su ogni pavimento e
 * calcolare le proporzioni normalizzate (r/c, g/c, b/c).
 *
 * Pavimenti riconosciuti (RCJ Rescue Maze 2027):
 *   BIANCO  — pavimento neutro
 *   ARGENTO — checkpoint (salvare posizione)
 *   NERO    — buco / assenza pavimento (LoP)
 *   BLU     — pozzanghera (stop 5 s)
 *   ROSSO   — zona pericolosa (rallentare)
 * ============================================================ */

/* Clear (luce totale) sotto il quale il pavimento è considerato NERO */
#define THRESH_BLACK_CLEAR      300

/* Clear sopra il quale il pavimento è probabilmente ARGENTO/BIANCO */
#define THRESH_SILVER_CLEAR     3000

/* Rapporti normalizzati per ARGENTO: molto alto e uniforme su r,g,b */
#define SILVER_MIN_RATIO        0.28f   /* ogni canale > 28% del clear */

/* Soglie per BLU: B/C > BLU_MIN e B > G e B > R */
#define BLUE_RATIO_MIN          0.35f

/* Soglie per ROSSO: R/C > RED_MIN e R > G*1.5 */
#define RED_RATIO_MIN           0.40f

// ----------------------------------------------------------------

int color_init()
{
    if (!apds.begin()) {
        Serial.println(F("[COLOR] ERRORE: APDS9960 non trovato!"));
        return 0;
    }
    apds.enableColor(true);
    Serial.println(F("[COLOR] APDS9960 OK"));
    return 1;
}

void color_read_rgb(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
    uint32_t timeout = millis() + 100;
    while (!apds.colorDataReady()) {
        delay(5);
        if (millis() > timeout) break;   /* evita blocco infinito */
    }
    apds.getColorData(r, g, b, c);
}

floor_color_t color_read_floor_color()
{
    uint16_t r, g, b, c;
    color_read_rgb(&r, &g, &b, &c);

    /* ---- NERO: poca luce riflessa → buco o ostacolo ---- */
    if (c < THRESH_BLACK_CLEAR) {
        return FLOOR_BLACK;
    }

    float fc = (float)c;
    float nr  = (float)r / fc;   /* rapporto normalizzato rosso  */
    float ng  = (float)g / fc;   /* rapporto normalizzato verde  */
    float nb  = (float)b / fc;   /* rapporto normalizzato blu    */

    /* ---- ARGENTO: elevata luminosità uniforme su tutti i canali ---- */
    if (c > THRESH_SILVER_CLEAR
        && nr > SILVER_MIN_RATIO
        && ng > SILVER_MIN_RATIO
        && nb > SILVER_MIN_RATIO) {
        return FLOOR_SILVER;
    }

    /* ---- BLU: canale blu dominante ---- */
    if (nb > BLUE_RATIO_MIN && nb > nr * 1.4f && nb > ng * 1.2f) {
        return FLOOR_BLUE;
    }

    /* ---- ROSSO: canale rosso dominante ---- */
    if (nr > RED_RATIO_MIN && nr > ng * 1.5f && nr > nb * 1.5f) {
        return FLOOR_RED;
    }

    /* ---- BIANCO: tutto il resto ---- */
    return FLOOR_WHITE;
}