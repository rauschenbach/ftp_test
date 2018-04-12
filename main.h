#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <lwip/sockets.h>
#include <lwip/arch.h>
#include <lwip/inet.h>
#include <lwip/opt.h>
#include <lwip/api.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <netconf.h>
#include <tcpip.h>
#include <usart1.h>
#include <ftpd.h>
#include <led.h>
#include <ff.h>


#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "globdefs.h"

/* MII and RMII mode selection, for STM324xG-EVAL Board(MB786) RevB ***********/
#define RMII_MODE


/* Exported macro ------------------------------------------------------------*/
#define TickGetDiff(a, b)       (s32)(a < b) ? (((s32)0xffffffff - b) + a) : (a - b)

/* Exported functions ------------------------------------------------------- */  
void Time_Update(void);

int TickGet(void);
void Delay(uint32_t nCount);
void vApplicationTickHook(void);
//u32 sys_now(void);


#define PRINTF printf

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


/*********** Portions COPYRIGHT 2012 Embest Tech. Co., Ltd.*****END OF FILE****/

