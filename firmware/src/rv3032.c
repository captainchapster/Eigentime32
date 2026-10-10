#include "rv3032.h"

#include "board.h"

#define ADDR 0x51

#define REG_100TH    0x00
#define REG_STATUS   0x0D
#define REG_TEMP_LSB 0x0E
#define REG_CTRL1    0x10
#define REG_CTRL2    0x11
#define REG_EEADDR   0x3D
#define REG_EEDATA   0x3E
#define REG_EECMD    0x3F
#define REG_PMU      0xC0
#define REG_OFFSET   0xC1
#define REG_CLKOUT2  0xC3

#define STATUS_UF   (1u << 5)
#define STATUS_PORF (1u << 1)
#define STATUS_VLF  (1u << 0)
#define CTRL1_USEL  (1u << 4)
#define CTRL1_EERD  (1u << 2)
#define CTRL2_UIE   (1u << 5)
#define PMU_NCLKE   (1u << 6)
#define TEMP_EEBUSY (1u << 2)
#define TEMP_EEF    (1u << 3)
#define EECMD_UPDATE 0x11   /* copy all configuration RAM to EEPROM */
#define EECMD_WRITE1 0x21   /* write one EEPROM byte from EEADDR/EEDATA */

static bool rd(uint8_t reg, uint8_t *buf, size_t n) { return i2c_write_read(ADDR, &reg, 1, buf, n); }

static bool wr(uint8_t reg, const uint8_t *buf, size_t n)
{
    uint8_t b[9];
    if (n > 8)
        return false;
    b[0] = reg;
    for (size_t i = 0; i < n; i++)
        b[i + 1] = buf[i];
    return i2c_write(ADDR, b, n + 1);
}

static bool wr1(uint8_t reg, uint8_t v) { return wr(reg, &v, 1); }

static bool modify(uint8_t reg, uint8_t clear, uint8_t set)
{
    uint8_t v;
    return rd(reg, &v, 1) && wr1(reg, (uint8_t)((v & ~clear) | set));
}

static bool ee_wait(void)
{
    for (int i = 0; i < 50; i++) {
        uint8_t t;
        if (!rd(REG_TEMP_LSB, &t, 1))
            return false;
        if (!(t & TEMP_EEBUSY))
            return !(t & TEMP_EEF);
        delay_ms(2);
    }
    return false;
}

/* Writes one configuration byte to RAM and, if it differs, to EEPROM too.
 * EERD must be set so the RTC doesn't refresh RAM from EEPROM meanwhile. */
static bool cfg_set(uint8_t reg, uint8_t mask, uint8_t value, bool persist)
{
    uint8_t v;
    if (!rd(reg, &v, 1))
        return false;
    uint8_t nv = (uint8_t)((v & ~mask) | (value & mask));
    if (nv == v)
        return true;
    if (!wr1(reg, nv))
        return false;
    if (!persist)
        return true;
    return wr1(REG_EEADDR, reg) && wr1(REG_EEDATA, nv) && wr1(REG_EECMD, EECMD_WRITE1) && ee_wait();
}

static uint8_t bcd(uint8_t v) { return (uint8_t)((v / 10) << 4 | (v % 10)); }
static uint8_t unbcd(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }

bool rv3032_init(rv3032_status_t *st)
{
    uint8_t s;
    if (!modify(REG_CTRL1, 0, CTRL1_EERD) || !rd(REG_STATUS, &s, 1))
        return false;
    st->power_on_reset = s & STATUS_PORF;
    st->voltage_low = s & STATUS_VLF;
    /* CLKOUT: 1 Hz from the crystal (FD = 11, OS = 0), kept off (NCLKE) until the lid opens.
     * Both are stored in EEPROM so a cell swap comes back up quiet (default is 32.768 kHz on). */
    if (!cfg_set(REG_CLKOUT2, 0xE0, 0x60, true) || !cfg_set(REG_PMU, PMU_NCLKE, PMU_NCLKE, true))
        return false;
    return wr1(REG_STATUS, (uint8_t)(s & ~(STATUS_PORF | STATUS_VLF)));
}

bool rv3032_read(dt_t *t, uint8_t *hundredths)
{
    uint8_t b[8];
    if (!rd(REG_100TH, b, 8))
        return false;
    if (hundredths)
        *hundredths = unbcd(b[0]);
    t->sec = unbcd(b[1] & 0x7F);
    t->min = unbcd(b[2] & 0x7F);
    t->hour = unbcd(b[3] & 0x3F);
    t->wday = b[4] & 0x07;
    t->day = unbcd(b[5] & 0x3F);
    t->month = unbcd(b[6] & 0x1F);
    t->year = (uint16_t)(2000 + unbcd(b[7]));
    return dt_valid(t);
}

bool rv3032_write(const dt_t *t)
{
    /* writing the seconds register also clears the 100th-seconds and the prescaler */
    uint8_t b[7] = {bcd(t->sec), bcd(t->min), bcd(t->hour), t->wday,
                    bcd(t->day), bcd(t->month), bcd((uint8_t)(t->year - 2000))};
    return wr(REG_100TH + 1, b, 7);
}

bool rv3032_clkout(bool on) { return cfg_set(REG_PMU, PMU_NCLKE, on ? 0 : PMU_NCLKE, false); }

bool rv3032_tick(bool on, bool every_second)
{
    if (!modify(REG_CTRL1, CTRL1_USEL, every_second ? 0 : CTRL1_USEL))
        return false;
    return modify(REG_CTRL2, CTRL2_UIE, on ? CTRL2_UIE : 0) && rv3032_ack_tick();
}

bool rv3032_ack_tick(void) { return modify(REG_STATUS, STATUS_UF, 0); }

bool rv3032_get_offset(int8_t *steps)
{
    uint8_t v;
    if (!rd(REG_OFFSET, &v, 1))
        return false;
    v &= 0x3F;
    *steps = (int8_t)(v & 0x20 ? v - 64 : v);
    return true;
}

bool rv3032_set_offset(int8_t steps)
{
    return cfg_set(REG_OFFSET, 0x3F, (uint8_t)(steps & 0x3F), true);
}
