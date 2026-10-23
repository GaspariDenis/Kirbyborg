#include "config.h"

typedef enum {
    FLOOR_UNKOWN,
    FLOOR_WHITE,
    FLOOR_SILVER,
    FLOOR_BLACK,
    FLOOR_BLUE,
    FLOOR_RED
} floor_color_t;

int color_init();

void color_read_rgb(u16* r, u16* g, u16* b, u16* c);

floor_color_t color_read_floor_color();
