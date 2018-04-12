/************************************************************************************
 *
 * 	FTP server lwip+freertos+ftp
 *  	Copyright (c) rauschenbach 
 *      sokareis@mail.ru
 *
 ***********************************************************************************/
#include "stm32f4_discovery.h"
#include "main.h"



#define DHCP_TASK_PRIO   ( tskIDLE_PRIORITY + 2 )
#define LED_TASK_PRIO    ( tskIDLE_PRIORITY + 1 )


static void led_task(void *pvParameters);
static void prvSetupHardware(void);

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
    prvSetupHardware();


#ifdef USE_DHCP
    /* Start DHCPClient */
    xTaskCreate(lwip_dhcp_task, "DHCP task", configMINIMAL_STACK_SIZE * 2, NULL, DHCP_TASK_PRIO, NULL);
#endif    
    
    
    xTaskCreate(ftpd_thread, "FTP task", configMINIMAL_STACK_SIZE * 8, NULL, DHCP_TASK_PRIO, NULL);
    
       /* Start toogleLed4 task : Toggle LED4  every 250ms */
  //  xTaskCreate(led_task, "led task", configMINIMAL_STACK_SIZE, NULL, LED_TASK_PRIO, NULL);

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);
}

/**
 * Настройка часов, уартов и пр.
 */
static void prvSetupHardware(void)
{
    STM_EVAL_LEDInit(LED3);
    STM_EVAL_LEDInit(LED4);
    STM_EVAL_LEDInit(LED5);
    STM_EVAL_LEDInit(LED6);
    usart1_init();
    ETH_BSP_Config();
    lwip_init_all();
}


/**
  * @brief  Toggle Led4 task
  * @param  pvParameters not used
  * @retval None
  */
void led_task(void *p)
{
    for (;;) {
	/* toggle LED4 each 250ms */
	STM_EVAL_LEDToggle(LED4);
	vTaskDelay(250);
    }
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t * file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1) {
    }
}
#endif
