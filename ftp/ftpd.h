#ifndef _FTP_H
#define _FTP_H

#include "main.h"



#define FTP_CMD_PORT		21
#define FTP_DATA_PORT		(FTP_CMD_PORT - 1) /* Меньше на единицу всегда */
#define	FTP_PASSV_PORT		10000
#define FTP_SRV_ROOT		"/"
#define FTP_MAX_CONNECTION	2
#define FTP_USER		"erik"
#define FTP_PASSWORD		"erik"
#define FTP_WELCOME_MSG		"220 Test FTP server for CC3200 ready.\r\n"
#define FTP_BUFFER_SIZE		 (256 * 2)	/* если 256 байт - не рушица буфер при передаче на stm32 */

/* Задержка во время чтения или записи */ 
#if 0
#define SLEEP(x) 		    osi_Sleep(x) /* или vTaskDelay(x) */
#else
#define SLEEP(x)		    vTaskDelay(x)	
#endif



/**
 * Состояние FTP, чтобы не было возможности войти без пароля
 */
typedef enum {
    ANON_STAT,
    USER_STAT,
    LOGGED_STAT,
    QUIT_STAT,
} ftp_state_en;


/**
 * Команды FTP
 */
typedef enum {
    CMD_USER,
    CMD_PASS,
    CMD_LIST,
    CMD_NLST,
    CMD_PWD,
    CMD_TYPE,
    CMD_PASV,
    CMD_RETR,
    CMD_STOR,
    CMD_SIZE,
    CMD_MDTM,
    CMD_SYST,
    CMD_CWD,
    CMD_CDUP,
    CMD_PORT,
    CMD_REST,
    CMD_MKD,
    CMD_DELE,
    CMD_RMD,
    CMD_FEAT,
    CMD_QUIT,
    CMD_UKNWN = -1,
} ftp_cmd_user;


/**
 * Связный список 
 */
struct conn {
    int sockfd;
    struct sockaddr_in remote;
    ftp_state_en status;	/* Вошел с паролем */
    char pasv_active;		/* pasv data */
    int pasv_sockfd;
    u16 pasv_port;
    size_t offset;
    char currentdir[FTP_BUFFER_SIZE];	/* current directory */
    struct conn *next;
};


void ftpd_thread(void *);

#endif				/* ftp.h */
