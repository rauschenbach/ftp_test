#include "led.h"

/* Инициализация LED  */
void led_init(void)
{
    STM_EVAL_LEDInit(0);
    STM_EVAL_LEDInit(1);
    STM_EVAL_LEDInit(2);
    STM_EVAL_LEDInit(3);
}


int led_on(u8 led)
{
    STM_EVAL_LEDOn(led);
    return 0;
}


int led_off(u8 led)
{
    STM_EVAL_LEDOff(led);
    return 0;
}

int led_toggle(u8 led)
{
    STM_EVAL_LEDToggle(led);
    return 0;
}
