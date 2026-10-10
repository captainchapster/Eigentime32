/* ST25DV04KC dynamic NFC tag: our status page out, the phone's time sync in. */
#ifndef ST25DV_H
#define ST25DV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ST25DV_USER_BYTES 512

bool st25dv_open(void);                     /* powers the tag's VCC and waits for boot */
void st25dv_close(void);
bool st25dv_read(uint16_t addr, uint8_t *buf, size_t n);
bool st25dv_write(uint16_t addr, const uint8_t *buf, size_t n);
/* Enables the GPO pulse on RF writes (needs the I2C security session, default password). */
bool st25dv_config_gpo(void);

#endif
