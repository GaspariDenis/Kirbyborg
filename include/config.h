#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Costanti di gara — RoboCupJunior Rescue Maze 2027
 * ============================================================ */

/* Tile dimensions (mm) — usate per la navigazione odometrica */
#define TILE_SIZE_MM            300

/* Distanza massima dal muro per rilevare le vittime (mm) */
#define VICTIM_DETECT_DIST_MM   150    /* 15 cm */

/* Distanza sicura dai muri durante la navigazione (mm) */
#define WALL_SAFE_DIST_MM       80

/* Soglia muro presente: distanza sotto la quale c'è un muro (mm) */
#define WALL_PRESENT_DIST_MM    200

/* Timing regolamento */
#define PUDDLE_STOP_MS          5000UL  /* 5 s fermo su pozzanghera blu     */
#define VICTIM_BLINK_MS         5000UL  /* 5 s lampeggio LED vittima         */
#define VICTIM_BLINK_ON_MS      500UL   /* ON 500 ms                         */
#define VICTIM_BLINK_OFF_MS     500UL   /* OFF 500 ms                        */
#define EXIT_BONUS_MS           5000UL  /* 5 s LED fisso per exit bonus      */

/* Rescue kit: quanti kit rilasciare per tipo vittima */
#define KITS_FOR_HARMED         2
#define KITS_FOR_STABLE         1

/* Velocità motori (0-255) */
#define MOTOR_SPEED_DEFAULT     150
#define MOTOR_SPEED_SLOW        90
#define MOTOR_SPEED_TURN        120

/* Tolleranza heading IMU (gradi) per il mantenimento della rotta */
#define HEADING_TOL_DEG         3.0f

/* Comunicazione seriale con Raspberry Pi 5 */
#define COMMS_SERIAL            Serial   /* USB nativa del Mega */
#define COMMS_BAUD              115200