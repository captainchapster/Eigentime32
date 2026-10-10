/* Eigentime32 pin map (docs/power-pins-accuracy.md section 5) and low-level helpers. */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32l0xx.h"

/* Port A */
#define PIN_RTC_INT     0   /* EXTI0, active low */
#define PIN_LCD_PWR_EN  1   /* load switch ON + boost EN (RC delayed) */
#define PIN_LID_CLOSED  2   /* EXTI2, low while the lid magnet is near */
#define PIN_BTN1        3   /* EXTI3, 10 o'clock, active low */
#define PIN_LCD_SCS     4
#define PIN_LCD_SCLK    5   /* SPI1_SCK AF0 */
#define PIN_LCD_DISP    6
#define PIN_LCD_SI      7   /* SPI1_MOSI AF0 */
#define PIN_NFC_GPO     8   /* EXTI8, open drain, low on an RF write */
#define PIN_SCL         9   /* I2C1 AF1 */
#define PIN_SDA         10  /* I2C1 AF1 */
/* Port B */
#define PIN_NFC_VCC     0
#define PIN_BTN2        1   /* EXTI1, 2 o'clock, active low */

#define CORE_HZ 2097152u    /* MSI range 5, the reset default */

/* wake/event flags set from interrupts */
#define EV_RTC   (1u << 0)
#define EV_LID   (1u << 1)
#define EV_BTN1  (1u << 2)
#define EV_BTN2  (1u << 3)
#define EV_NFC   (1u << 4)

void board_init(void);
uint32_t board_take_events(void);
void board_drop_events(uint32_t mask);
void board_sleep(void);                /* Stop mode until an EXTI wake */
void delay_ms(uint32_t ms);
uint32_t millis(void);                 /* only counts while the core runs */

bool lid_open(void);
bool btn1_down(void);
bool btn2_down(void);

void lcd_power(bool on);               /* load switch; boost follows after ~0.3 s */
void lcd_pins_idle(void);              /* hold SCS/SCLK/SI/DISP low while VLCD is off */
void lcd_disp(bool on);
void lcd_scs(bool on);
void spi_open(void);
void spi_close(void);
void spi_write(const uint8_t *buf, size_t n);

void nfc_power(bool on);

bool i2c_write(uint8_t addr7, const uint8_t *buf, size_t n);
bool i2c_write_read(uint8_t addr7, const uint8_t *wbuf, size_t wn, uint8_t *rbuf, size_t rn);

uint16_t battery_mv(void);             /* VDD from VREFINT */

/* STM32 data EEPROM (1 KB at DATA_EEPROM_BASE) */
void eeprom_write_words(uint32_t offset, const uint32_t *src, size_t words);
void eeprom_read(uint32_t offset, void *dst, size_t n);

#endif
