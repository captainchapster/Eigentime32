/* Vector table and reset handler for the STM32L031. */
#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
int main(void);

void Reset_Handler(void)
{
    uint32_t *s = &_sidata, *d = &_sdata;
    while (d < &_edata)
        *d++ = *s++;
    for (d = &_sbss; d < &_ebss;)
        *d++ = 0;
    main();
    for (;;)
        ;
}

static void Default_Handler(void)
{
    for (;;)
        ;
}

#define WEAK __attribute__((weak, alias("Default_Handler")))
void NMI_Handler(void) WEAK;
void HardFault_Handler(void) WEAK;
void SVC_Handler(void) WEAK;
void PendSV_Handler(void) WEAK;
void SysTick_Handler(void) WEAK;
void EXTI0_1_IRQHandler(void) WEAK;
void EXTI2_3_IRQHandler(void) WEAK;
void EXTI4_15_IRQHandler(void) WEAK;

__attribute__((section(".isr_vector"), used)) static void (*const vectors[16 + 32])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    0, 0, 0, 0, 0, 0, 0,
    SVC_Handler,
    0, 0,
    PendSV_Handler,
    SysTick_Handler,
    /* IRQ 0..31 */
    [16 + 5] = EXTI0_1_IRQHandler,
    [16 + 6] = EXTI2_3_IRQHandler,
    [16 + 7] = EXTI4_15_IRQHandler,
};
