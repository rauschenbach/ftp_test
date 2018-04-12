#ifndef _GLOBDEFS_H
#define _GLOBDEFS_H


#include <signal.h>
#include <stdint.h>
#include <time.h>


/* Включения для 504 блекфина, не 506!!!!  */
#ifndef _WIN32			/* Embedded platform */
#include <float.h>
#else				/* windows   */
#include <windows.h>
#include <tchar.h>
#endif


#ifndef u8
#define u8 unsigned char
#endif

#ifndef s8
#define s8 char
#endif

#ifndef c8
#define c8 char
#endif

#ifndef u16
#define u16 unsigned short
#endif


#ifndef s16
#define s16 short
#endif

#ifndef i32
#define i32  int
#endif


#ifndef u32
#define u32 unsigned long
#endif


#ifndef s32
#define s32 long
#endif


#ifndef u64
#define u64 uint64_t
#endif


#ifndef s64
#define s64 int64_t
#endif


/* Длинное время */
#ifndef	time64
#define time64	int64_t
#endif


/* long double не поддержываеца  */
#ifndef f32
#define f32 float
#endif

#ifndef bool
#define bool u8
#endif


#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif


#ifndef IDEF
#define IDEF static inline
#endif


/* Делители */
#define 		SYSCLK          	80000000UL

/* Стек для задач. размер */
#define 		OSI_STACK_SIZE          1024

#define 		LOADER_TASK_PRIORITY		1
#define 		GPS_TASK_PRIORITY		1
#define 		PWM_TASK_PRIORITY		1
#define 		SENSOR_TASK_PRIORITY		1
#define 		CHECK_TIME_TASK_PRIORITY	1
#define 		WLAN_STATION_TASK_PRIORITY	1

#define 		TX_DATA_TASK_PRIORITY		5
#define 		TCP_RX_TASK_PRIORITY		5
#define 		UDP_RX_TASK_PRIORITY		5

#define 		SD_TASK_PRIORITY		1
#define 		CMD_TASK_PRIORITY		1
#define 		ADC_TASK_PRIORITY		1

/** 
 *  Передадим FTP серверу
 */ 
typedef union {
    u32 ip;
    u8  bIp[4];
} my_ip_addr;


#endif				/* my_defs.h */
