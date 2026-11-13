/* ============================================================
 * Implementazione protocollo seriale Arduino Mega ↔ Raspberry Pi 5.
 * ============================================================ */

#include "protocol.h"
#include "config.h"

#include <Arduino.h>

/* Buffer per la ricezione riga dal Pi */
#define RX_BUF_LEN  64
static char     s_rx_buf[RX_BUF_LEN];
static uint8_t  s_rx_idx = 0;

/* Ultimo comando ricevuto */
static pi_command_t s_pending_cmd = PI_CMD_NONE;

void protocol_init(void)
{
    COMMS_SERIAL.begin(COMMS_BAUD);
    Serial.println(F("[PROTO] Seriale Pi5 OK"));
}

/* Parsing del comando ricevuto */
static void parse_line(const char *line)
{
    if (strncmp(line, "CMD:", 4) != 0) return;

    const char *cmd = line + 4;

    if (strcmp(cmd, "VICTIM_H") == 0) {
        s_pending_cmd = PI_CMD_VICTIM_H;
    } else if (strcmp(cmd, "VICTIM_S") == 0) {
        s_pending_cmd = PI_CMD_VICTIM_S;
    } else if (strcmp(cmd, "STOP") == 0) {
        s_pending_cmd = PI_CMD_STOP;
    } else if (strcmp(cmd, "RESUME") == 0) {
        s_pending_cmd = PI_CMD_RESUME;
    }
}

void protocol_update(void)
{
    while (COMMS_SERIAL.available()) {
        char c = (char)COMMS_SERIAL.read();

        if (c == '\n' || c == '\r') {
            if (s_rx_idx > 0) {
                s_rx_buf[s_rx_idx] = '\0';
                parse_line(s_rx_buf);
                s_rx_idx = 0;
            }
        } else if (s_rx_idx < RX_BUF_LEN - 1) {
            s_rx_buf[s_rx_idx++] = c;
        } else {
            /* buffer overflow: reset */
            s_rx_idx = 0;
        }
    }
}

pi_command_t protocol_get_command(void)
{
    pi_command_t cmd = s_pending_cmd;
    s_pending_cmd = PI_CMD_NONE;
    return cmd;
}

void protocol_send_telemetry(const char *state_name,
                             uint8_t     floor,
                             uint16_t    front_mm,
                             uint16_t    left_mm,
                             uint16_t    right_mm,
                             float       heading_deg,
                             uint8_t     kits)
{
    COMMS_SERIAL.print(F("T:"));
    COMMS_SERIAL.print(state_name);
    COMMS_SERIAL.print(',');
    COMMS_SERIAL.print(floor);
    COMMS_SERIAL.print(',');
    COMMS_SERIAL.print(front_mm);
    COMMS_SERIAL.print(',');
    COMMS_SERIAL.print(left_mm);
    COMMS_SERIAL.print(',');
    COMMS_SERIAL.print(right_mm);
    COMMS_SERIAL.print(',');
    COMMS_SERIAL.print(heading_deg, 1);
    COMMS_SERIAL.print(',');
    COMMS_SERIAL.print(kits);
    COMMS_SERIAL.println();
}

void protocol_log(const char *msg)
{
    COMMS_SERIAL.print(F("LOG:"));
    COMMS_SERIAL.println(msg);
}
