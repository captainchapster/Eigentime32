#include "board.h"

#include <string.h>

static volatile uint32_t events;
static volatile uint32_t ticks;

/* ---------------------------------------------------------------- GPIO */
static void pin_mode(GPIO_TypeDef *g, int pin, uint32_t mode)
{
    g->MODER = (g->MODER & ~(3u << (pin * 2))) | (mode << (pin * 2));
}

static void pin_af(GPIO_TypeDef *g, int pin, uint32_t af)
{
    volatile uint32_t *r = &g->AFR[pin >> 3];
    *r = (*r & ~(0xFu << ((pin & 7) * 4))) | (af << ((pin & 7) * 4));
    pin_mode(g, pin, 2);
}

static void pin_set(GPIO_TypeDef *g, int pin, bool v)
{
    g->BSRR = v ? (1u << pin) : (1u << (pin + 16));
}

static bool pin_get(GPIO_TypeDef *g, int pin) { return (g->IDR >> pin) & 1u; }

/* ---------------------------------------------------------------- init */
void board_init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

    /* outputs, all off: LCD power, LCD lines, NFC supply */
    pin_set(GPIOA, PIN_LCD_PWR_EN, 0);
    pin_mode(GPIOA, PIN_LCD_PWR_EN, 1);
    lcd_pins_idle();
    pin_set(GPIOB, PIN_NFC_VCC, 0);
    pin_mode(GPIOB, PIN_NFC_VCC, 1);

    /* inputs with external pull-ups (or push-pull drivers): no internal pulls */
    pin_mode(GPIOA, PIN_RTC_INT, 0);
    pin_mode(GPIOA, PIN_LID_CLOSED, 0);
    pin_mode(GPIOA, PIN_BTN1, 0);
    pin_mode(GPIOA, PIN_NFC_GPO, 0);
    pin_mode(GPIOB, PIN_BTN2, 0);

    /* EXTI: PA0 (RTC, falling), PB1 (BTN2, falling), PA2 (lid, both), PA3 (BTN1, falling), PA8 (NFC, falling) */
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~(0xFu << 4)) | (1u << 4); /* EXTI1 <- port B */
    uint32_t lines = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 8);
    EXTI->FTSR |= lines;
    EXTI->RTSR |= (1u << 2);
    EXTI->PR = lines;
    EXTI->IMR |= lines;
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
    NVIC_EnableIRQ(EXTI4_15_IRQn);

    /* 1 ms tick while running */
    SysTick_Config(CORE_HZ / 1000);

    /* Stop mode: ultra-low-power regulator, VREFINT off in stop, fast wake-up, wake on MSI */
    PWR->CR |= PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU;
    PWR->CR &= ~PWR_CR_PDDS;
    RCC->CFGR &= ~RCC_CFGR_STOPWUCK;
}

/* ---------------------------------------------------------------- interrupts */
void SysTick_Handler(void) { ticks++; }

void EXTI0_1_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & 3u;
    EXTI->PR = pr;
    if (pr & 1u)
        events |= EV_RTC;
    if (pr & 2u)
        events |= EV_BTN2;
}

void EXTI2_3_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & (3u << 2);
    EXTI->PR = pr;
    if (pr & (1u << 2))
        events |= EV_LID;
    if (pr & (1u << 3))
        events |= EV_BTN1;
}

void EXTI4_15_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & 0xFFF0u;
    EXTI->PR = pr;
    if (pr & (1u << 8))
        events |= EV_NFC;
}

uint32_t board_take_events(void)
{
    __disable_irq();
    uint32_t e = events;
    events = 0;
    __enable_irq();
    return e;
}

void board_drop_events(uint32_t mask)
{
    __disable_irq();
    events &= ~mask;
    __enable_irq();
}

void board_sleep(void)
{
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __disable_irq();
    if (!events) {
        __DSB();
        __WFI();
    }
    __enable_irq();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

uint32_t millis(void) { return ticks; }

void delay_ms(uint32_t ms)
{
    uint32_t start = ticks;
    while ((uint32_t)(ticks - start) < ms)
        __WFI(); /* plain sleep: SysTick keeps running */
}

/* ---------------------------------------------------------------- inputs */
bool lid_open(void) { return pin_get(GPIOA, PIN_LID_CLOSED); }
bool btn1_down(void) { return !pin_get(GPIOA, PIN_BTN1); }
bool btn2_down(void) { return !pin_get(GPIOB, PIN_BTN2); }

/* ---------------------------------------------------------------- LCD */
void lcd_power(bool on) { pin_set(GPIOA, PIN_LCD_PWR_EN, on); }

void lcd_pins_idle(void)
{
    static const int pins[] = {PIN_LCD_SCS, PIN_LCD_SCLK, PIN_LCD_DISP, PIN_LCD_SI};
    for (unsigned i = 0; i < sizeof pins / sizeof pins[0]; i++) {
        pin_set(GPIOA, pins[i], 0);
        pin_mode(GPIOA, pins[i], 1);
    }
}

void lcd_disp(bool on) { pin_set(GPIOA, PIN_LCD_DISP, on); }
void lcd_scs(bool on) { pin_set(GPIOA, PIN_LCD_SCS, on); }

void spi_open(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    pin_af(GPIOA, PIN_LCD_SCLK, 0);
    pin_af(GPIOA, PIN_LCD_SI, 0);
    /* master, mode 0, LSB first, fPCLK/4 = 524 kHz (LCD max 1 MHz), software NSS */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_LSBFIRST | SPI_CR1_BR_0 | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi_write(const uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        while (!(SPI1->SR & SPI_SR_TXE))
            ;
        *(volatile uint8_t *)&SPI1->DR = buf[i];
    }
    while (!(SPI1->SR & SPI_SR_TXE))
        ;
    while (SPI1->SR & SPI_SR_BSY)
        ;
    (void)SPI1->DR;
    (void)SPI1->SR;
}

void spi_close(void)
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
    lcd_pins_idle();
}

/* ---------------------------------------------------------------- NFC supply */
void nfc_power(bool on) { pin_set(GPIOB, PIN_NFC_VCC, on); }

/* ---------------------------------------------------------------- I2C1 */
static void i2c_open(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    GPIOA->OTYPER |= (1u << PIN_SCL) | (1u << PIN_SDA);
    pin_af(GPIOA, PIN_SCL, 1);
    pin_af(GPIOA, PIN_SDA, 1);
    I2C1->CR1 = 0;
    /* ~95 kHz from 2.097 MHz: SCLL 10, SCLH 9, SDADEL 0, SCLDEL 1 */
    I2C1->TIMINGR = (1u << 20) | (9u << 8) | 10u;
    I2C1->CR1 = I2C_CR1_PE;
}

static void i2c_close(void)
{
    I2C1->CR1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;
    pin_mode(GPIOA, PIN_SCL, 3); /* analog: the pull-ups sit on VBAT */
    pin_mode(GPIOA, PIN_SDA, 3);
}

static bool i2c_wait(uint32_t flag)
{
    uint32_t start = ticks;
    while (!(I2C1->ISR & flag)) {
        if (I2C1->ISR & I2C_ISR_NACKF)
            return false;
        if ((uint32_t)(ticks - start) > 20)
            return false;
    }
    return true;
}

static bool i2c_xfer(uint8_t addr7, const uint8_t *wbuf, size_t wn, uint8_t *rbuf, size_t rn)
{
    bool ok = true;
    i2c_open();
    if (wn) {
        I2C1->CR2 = ((uint32_t)addr7 << 1) | (wn << I2C_CR2_NBYTES_Pos) | (rn ? 0 : I2C_CR2_AUTOEND) |
                    I2C_CR2_START;
        for (size_t i = 0; i < wn && ok; i++) {
            ok = i2c_wait(I2C_ISR_TXIS);
            if (ok)
                I2C1->TXDR = wbuf[i];
        }
        if (ok && rn)
            ok = i2c_wait(I2C_ISR_TC);
    }
    if (ok && rn) {
        I2C1->CR2 = ((uint32_t)addr7 << 1) | (rn << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN |
                    I2C_CR2_AUTOEND | I2C_CR2_START;
        for (size_t i = 0; i < rn && ok; i++) {
            ok = i2c_wait(I2C_ISR_RXNE);
            if (ok)
                rbuf[i] = (uint8_t)I2C1->RXDR;
        }
    }
    if (ok)
        ok = i2c_wait(I2C_ISR_STOPF);
    else if (!(I2C1->ISR & I2C_ISR_STOPF))
        I2C1->CR2 |= I2C_CR2_STOP;
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    i2c_close();
    return ok;
}

bool i2c_write(uint8_t addr7, const uint8_t *buf, size_t n)
{
    return n <= 255 && i2c_xfer(addr7, buf, n, 0, 0);
}

bool i2c_write_read(uint8_t addr7, const uint8_t *wbuf, size_t wn, uint8_t *rbuf, size_t rn)
{
    return wn <= 255 && rn <= 255 && i2c_xfer(addr7, wbuf, wn, rbuf, rn);
}

/* ---------------------------------------------------------------- battery */
uint16_t battery_mv(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
    ADC1->CFGR2 = ADC_CFGR2_CKMODE_0;           /* PCLK/2, synchronous */
    ADC->CCR |= ADC_CCR_VREFEN;
    ADC1->CR |= ADC_CR_ADVREGEN;
    delay_ms(1);                                /* regulator start-up (20 us max) */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL)
        ;
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
        ;
    ADC1->SMPR = 7;                             /* longest sample time for VREFINT */
    ADC1->CHSELR = ADC_CHSELR_CHSEL17;
    delay_ms(1);                                /* VREFINT start-up */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC))
        ;
    uint32_t raw = ADC1->DR;
    ADC1->CR |= ADC_CR_ADDIS;
    while (ADC1->CR & ADC_CR_ADEN)
        ;
    ADC1->CR &= ~ADC_CR_ADVREGEN;
    ADC->CCR &= ~ADC_CCR_VREFEN;
    RCC->APB2ENR &= ~RCC_APB2ENR_ADCEN;
    uint32_t cal = *(const uint16_t *)0x1FF80078; /* VREFINT_CAL at 3.0 V */
    return raw ? (uint16_t)(3000u * cal / raw) : 0;
}

/* ---------------------------------------------------------------- data EEPROM */
void eeprom_write_words(uint32_t offset, const uint32_t *src, size_t words)
{
    volatile uint32_t *dst = (volatile uint32_t *)(DATA_EEPROM_BASE + offset);
    if (FLASH->PECR & FLASH_PECR_PELOCK) {
        FLASH->PEKEYR = 0x89ABCDEFu;
        FLASH->PEKEYR = 0x02030405u;
    }
    for (size_t i = 0; i < words; i++) {
        if (dst[i] == src[i])
            continue;
        dst[i] = src[i];
        while (FLASH->SR & FLASH_SR_BSY)
            ;
    }
    FLASH->PECR |= FLASH_PECR_PELOCK;
}

void eeprom_read(uint32_t offset, void *dst, size_t n)
{
    memcpy(dst, (const void *)(DATA_EEPROM_BASE + offset), n);
}
