#include "st25dv.h"

#include "board.h"

#define ADDR_USER 0x53
#define ADDR_SYS  0x57

#define SYS_GPO1     0x0000
#define REG_I2C_PWD  0x0900
#define GPO1_EN      (1u << 0)
#define GPO1_RF_WRITE (1u << 7)

bool st25dv_open(void)
{
    nfc_power(1);
    delay_ms(1);        /* tBOOTDC ~0.6 ms */
    uint8_t probe[2] = {0, 0}, v;
    return i2c_write_read(ADDR_USER, probe, 2, &v, 1);
}

void st25dv_close(void) { nfc_power(0); }

bool st25dv_read(uint16_t addr, uint8_t *buf, size_t n)
{
    while (n) {
        size_t chunk = n > 255 ? 255 : n;
        uint8_t a[2] = {(uint8_t)(addr >> 8), (uint8_t)addr};
        if (!i2c_write_read(ADDR_USER, a, 2, buf, chunk))
            return false;
        addr += chunk;
        buf += chunk;
        n -= chunk;
    }
    return true;
}

static bool wait_write(uint8_t dev)
{
    for (int i = 0; i < 20; i++) {          /* ACK polling: 5 ms per 4-byte page */
        uint8_t a[2] = {0, 0}, v;
        if (i2c_write_read(dev, a, 2, &v, 1))
            return true;
        delay_ms(1);
    }
    return false;
}

static bool write_dev(uint8_t dev, uint16_t addr, const uint8_t *buf, size_t n)
{
    uint8_t b[2 + 16];
    while (n) {
        /* 16-byte bursts that never straddle a 4-byte page boundary midway */
        size_t chunk = 16 - (addr & 3);
        if (chunk > n)
            chunk = n;
        b[0] = (uint8_t)(addr >> 8);
        b[1] = (uint8_t)addr;
        for (size_t i = 0; i < chunk; i++)
            b[2 + i] = buf[i];
        if (!i2c_write(dev, b, 2 + chunk) || !wait_write(dev))
            return false;
        addr += chunk;
        buf += chunk;
        n -= chunk;
    }
    return true;
}

bool st25dv_write(uint16_t addr, const uint8_t *buf, size_t n)
{
    return write_dev(ADDR_USER, addr, buf, n);
}

bool st25dv_config_gpo(void)
{
    uint8_t gpo;
    uint8_t a[2] = {SYS_GPO1 >> 8, SYS_GPO1 & 0xFF};
    if (!i2c_write_read(ADDR_SYS, a, 2, &gpo, 1))
        return false;
    uint8_t want = GPO1_EN | GPO1_RF_WRITE;
    if (gpo == want)
        return true;
    /* present the default (all-zero) I2C password: pwd, 0x09, pwd */
    uint8_t p[2 + 17] = {REG_I2C_PWD >> 8, REG_I2C_PWD & 0xFF};
    p[2 + 8] = 0x09;
    if (!i2c_write(ADDR_SYS, p, sizeof p))
        return false;
    delay_ms(5);
    a[0] = SYS_GPO1 >> 8;
    a[1] = SYS_GPO1 & 0xFF;
    uint8_t w[3] = {a[0], a[1], want};
    return i2c_write(ADDR_SYS, w, 3) && wait_write(ADDR_SYS);
}
