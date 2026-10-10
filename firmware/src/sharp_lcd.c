#include "sharp_lcd.h"

#include <string.h>

#include "board.h"

#define CMD_WRITE 0x01  /* M0, sent LSB first */
#define CMD_CLEAR 0x04  /* M2 */

static gfx_fb_t shown;  /* what the panel holds now (1 = black) */
static bool shown_valid;

static void frame_begin(void)
{
    spi_open();
    lcd_scs(1);
    delay_ms(1);        /* tsSCS >= 6 us */
}

static void frame_end(void)
{
    delay_ms(1);        /* thSCS >= 2 us */
    lcd_scs(0);
    spi_close();
}

void sharp_on(void)
{
    lcd_power(1);
    /* the boost starts after the RC delay on its EN pin (~0.24-0.37 s), then VLCD settles */
    delay_ms(450);
    static const uint8_t clear[2] = {CMD_CLEAR, 0x00};
    frame_begin();
    spi_write(clear, sizeof clear);
    frame_end();
    lcd_disp(1);
    shown_valid = false;
}

void sharp_off(void)
{
    lcd_disp(0);
    lcd_pins_idle();
    lcd_power(0);
    shown_valid = false;
}

void sharp_flush(const gfx_fb_t *fb)
{
    uint8_t line[2 + GFX_STRIDE];
    bool started = false;
    for (int y = 0; y < GFX_H; y++) {
        if (shown_valid && memcmp(fb->px[y], shown.px[y], GFX_STRIDE) == 0)
            continue;
        if (!started) {
            frame_begin();
            static const uint8_t cmd = CMD_WRITE;
            spi_write(&cmd, 1);
            started = true;
        }
        line[0] = (uint8_t)(y + 1);                 /* gate line 1..128, LSB first */
        for (int i = 0; i < GFX_STRIDE; i++)
            line[1 + i] = (uint8_t)~fb->px[y][i];   /* panel: 1 = white */
        line[1 + GFX_STRIDE] = 0x00;                /* line trailer */
        spi_write(line, sizeof line);
    }
    if (started) {
        static const uint8_t trailer = 0x00;
        spi_write(&trailer, 1);
        frame_end();
    }
    memcpy(&shown, fb, sizeof shown);
    shown_valid = true;
}
