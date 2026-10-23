#include "color_apds9960.h"
#include <Adafruit_APDS9960.h>

static Adafruit_APDS9960 apds; 

// Inizializza il sensore APDS9960
int color_init(){
    return apds.begin();
}

// Legge i valori RGB e Clear dal sensore APDS9960
void color_read_rgb(u16 *r, u16 *g, u16 *b, u16*c){

    while(!apds.colorDataReady()) {
        delay(5);
    };

    apds.getColorData(r, g, b, c);
}

// Determina il colore del pavimento in base ai valori RGB letti dal sensore
floor_color_t color_read_floor_color(){
}