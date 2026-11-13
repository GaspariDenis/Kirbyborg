#pragma once

#include "config.h"

/* ============================================================
 * Colori del pavimento riconosciuti dall'APDS9960.
 * ============================================================ */
typedef enum {
    FLOOR_UNKOWN,    /* non classificato */
    FLOOR_WHITE,     /* pavimento normale */
    FLOOR_SILVER,    /* checkpoint — salvare posizione */
    FLOOR_BLACK,     /* buco / assenza pavimento → LoP immediato */
    FLOOR_BLUE,      /* pozzanghera → stop 5 s */
    FLOOR_RED        /* zona pericolosa → rallentare */
} floor_color_t;

/**
 * Inizializza il sensore APDS9960.
 * @return 1 se OK, 0 se errore.
 */
int color_init(void);

/**
 * Legge i canali RGBC raw.
 */
void color_read_rgb(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);

/**
 * Classifica il colore del pavimento sotto il robot.
 */
floor_color_t color_read_floor_color(void);
