/* Micro Crystal RV-3032-C7 driver (register map per the RV-3032-C7 Application Manual). */
#ifndef RV3032_H
#define RV3032_H

#include <stdbool.h>
#include <stdint.h>

#include "timeutil.h"

typedef struct {
    bool power_on_reset;   /* PORF: time lost (cell swap) */
    bool voltage_low;      /* VLF: data may be corrupt */
} rv3032_status_t;

bool rv3032_init(rv3032_status_t *st);       /* config + status; clears PORF/VLF */
bool rv3032_read(dt_t *utc, uint8_t *hundredths);
bool rv3032_write(const dt_t *utc);
bool rv3032_clkout(bool on);                 /* 1 Hz CLKOUT to the LCD's EXTCOMIN */
bool rv3032_tick(bool on, bool every_second);/* periodic update interrupt on INT */
bool rv3032_ack_tick(void);
bool rv3032_get_offset(int8_t *steps);
bool rv3032_set_offset(int8_t steps);        /* also stored in the RTC's EEPROM */

#endif
