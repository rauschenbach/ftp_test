/************************************************************************************
 *
 * 	FTP server lwip+freertos+ftp
 *  	Copyright (c) rauschenbach 
 *      sokareis@mail.ru
 *
 ***********************************************************************************/
/* команды: http://nsftools.com/tips/RawFTP.htm */

#include "ftpd.h"
#include "cmds.h"


static struct conn *conn_list = NULL;
static char ftp_buf[FTP_BUFFER_SIZE];	/* Буфер, чтобы не выделять память через malloc  */
static FATFS fatfs;

static int ftp_process_request(struct conn *session, char *buf);

static int conn_count = 0;

/**
 * Создать новое FTP соединение на каждую новую команду
 */
static struct conn *alloc_new_conn(void)
{
    struct conn *conn;
    conn = (struct conn *) malloc(sizeof(struct conn));
    conn->next = conn_list;
    conn->offset = 0;
    conn_list = conn;

/*    PRINTF("Alloc addr: %p\r\n", conn); */
    return conn;
}

/** 
 * Закрыть текущее FTP соединение
 */
static void destroy_conn(struct conn *conn)
{
    struct conn *list;

    if (conn_list == conn) {
	conn_list = conn_list->next;
	conn->next = NULL;
    } else {
	list = conn_list;
	while (list->next != conn)
	    list = list->next;

	list->next = conn->next;
	conn->next = NULL;
    }

    free(conn);
}

/**
 * Функция основного потока
 */
void ftpd_thread(void *par)
{
    int numbytes;
    int sockfd, maxfdp1;
    struct sockaddr_in local;
    fd_set readfds, tmpfds;
    struct conn *conn;
    u32 addr_len = sizeof(struct sockaddr);

#if 0
    char *ftp_buf = (char *) malloc(FTP_BUFFER_SIZE);	/* Буфер, чтобы не выделять память через malloc */
#endif

    /* Сначала подмонтируем ФС */
    if (f_mount(&fatfs, "0", 1) != FR_OK) {
	PRINTF("Mount ERROR\n\r");
	return;
    }
    PRINTF("Mount OK\n\r");

    /* Создаем сокет TCP для команд с этими настойками */
    local.sin_port = htons(FTP_CMD_PORT);
    local.sin_family = PF_INET;
    local.sin_addr.s_addr = INADDR_ANY;

    FD_ZERO(&readfds);
    FD_ZERO(&tmpfds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
	PRINTF("ERROR: Create socket\r\n");
	return;
    }
    PRINTF("SUCCESS: Create socket\r\n");

    /* Привяжем на 21 порт */
    if (bind(sockfd, (struct sockaddr *) &local, addr_len) < 0) {
	PRINTF("ERROR: Bind socket\r\n");
    }
    PRINTF("SUCCESS: Bind socket\r\n");

    if (listen(sockfd, FTP_MAX_CONNECTION) < 0) {
	PRINTF("ERROR: Listen %d socket connections\r\n", FTP_MAX_CONNECTION);
    }
    PRINTF("SUCCESS: Listen socket\r\n");

    /* Ждем входящего звонка */
    FD_SET(sockfd, &readfds);
    for (;;) {
              
	/* get maximum fd */
	maxfdp1 = sockfd + 1;
	conn = conn_list;

	while (conn != NULL) {
	    if (maxfdp1 < conn->sockfd + 1)
		maxfdp1 = conn->sockfd + 1;

	    FD_SET(conn->sockfd, &readfds);
	    conn = conn->next;
	}

	tmpfds = readfds;
	if (select(maxfdp1, &tmpfds, 0, 0, 0) == 0)
	    continue;

	if (FD_ISSET(sockfd, &tmpfds)) {
	    int com_socket;
	    struct sockaddr_in remote;

	    com_socket = accept(sockfd, (struct sockaddr *) &remote, (socklen_t *) & addr_len);
	    if (com_socket == -1) {
		PRINTF("Error on accept()\nContinuing...\r\n");
		continue;
	    } else {
		PRINTF("Got connection from %s\r\n", inet_ntoa(remote.sin_addr));
		send(com_socket, FTP_WELCOME_MSG, strlen(FTP_WELCOME_MSG), 0);
		FD_SET(com_socket, &readfds);

		/* new conn */
		conn = alloc_new_conn();
		if (conn != NULL) {
		    strcpy(conn->currentdir, FTP_SRV_ROOT);
		    conn->sockfd = com_socket;
		    conn->remote = remote;
		}
	    }
	}

	{
	    struct conn *next;

	    conn = conn_list;
	    while (conn != NULL) {

		next = conn->next;
		if (FD_ISSET(conn->sockfd, &tmpfds)) {

		    numbytes = recv(conn->sockfd, ftp_buf, FTP_BUFFER_SIZE, 0);
		    if (numbytes == 0 || numbytes == -1) {
			PRINTF("Client %s disconnected\r\n", inet_ntoa(conn->remote.sin_addr));
			FD_CLR(conn->sockfd, &readfds);
			close(conn->sockfd);
			destroy_conn(conn);
		    } else {
			ftp_buf[numbytes] = 0;

			if (ftp_process_request(conn, ftp_buf) == -1) {
			    PRINTF("Client %s disconnected\r\n", inet_ntoa(conn->remote.sin_addr));
			    close(conn->sockfd);
			    destroy_conn(conn);
			}
                        SLEEP(1);
		    }
		}
		conn = next;
	    }
	}
    }
}

/**
 * Сессия для каждого соединения - здесь нужен большой стек!!!
 */
static int ftp_process_request(struct conn *conn, char *buf)
{
    FRESULT rc;
    FIL file;
    ftp_cmd_user cmd;
    int num_bytes;
    char spare_buf[256];	/* И для имени файла и для обработки других команд */
    struct timeval tv;
    fd_set readfds;
    c8 *sbuf;
    char *parameter_ptr, *ptr;
    struct sockaddr_in pasvremote, local;
    u32 addr_len = sizeof(struct sockaddr_in);
    int ret = 0;		/* Результат выполнения этой функции */
    int led_r = 0, led_w = 0;	/* Для моргания ламп при чтении и записи */

    sbuf = (c8 *) malloc(FTP_BUFFER_SIZE);
    if (sbuf == NULL) {
	PRINTF("ERROR: malloc for conn\r\n");
	return -1;
    }
    /* PRINTF("SUCCESS: malloc for conn\r\n"); */


    tv.tv_sec = 3;
    tv.tv_usec = 0;

    /* remove \r\n */
    ptr = buf;
    while (*ptr) {
	if (*ptr == '\r' || *ptr == '\n')
	    *ptr = 0;
	ptr++;
    }

    /* Какой параметр после команды */
    parameter_ptr = strchr(buf, ' ');
    if (parameter_ptr != NULL)
	parameter_ptr++;

    /* debug: */
    PRINTF("%s requested \"%s\"\r\n", inet_ntoa(conn->remote.sin_addr), buf);

    /* Разбор входящих команд */
    cmd = (ftp_cmd_user) do_parse_command(buf);
#if 0
    /* Выкидывать пользователя, если он не залогинен - на всякий! */
    if (cmd > CMD_PASS && conn->status != LOGGED_STAT) {
	do_send_reply(conn->sockfd, "550 Permission denied.\r\n");
	free(sbuf);
	return 0;
    }
#endif
    switch (cmd) {

	/* Посылка логина */
    case CMD_USER:
#if 0
	PRINTF("%s sent login \"%s\"\r\n", inet_ntoa(conn->remote.sin_addr), parameter_ptr);
	/* login correct */
	if (strcmp(parameter_ptr, FTP_USER) == 0) {
	    do_send_reply(conn->sockfd, "331 Password required for \"%s\"\r\n", parameter_ptr);
	    conn->status = USER_STAT;
	} else {
	    /* incorrect login */
	    do_send_reply(conn->sockfd, "530 Login incorrect. Bye.\r\n");
	    ret = -1;
	}
#endif
	do_send_reply(conn->sockfd, "331 Password required for \"%s\"\r\n", parameter_ptr);
	break;

	/* Посылка пароля */
    case CMD_PASS:
#if 0
	PRINTF("%s sent password \"%s\"\r\n", inet_ntoa(conn->remote.sin_addr), parameter_ptr);

	/* password correct */
	if (strcmp(parameter_ptr, FTP_PASSWORD) == 0) {
	    do_send_reply(conn->sockfd, "230 User logged in\r\n");
	    conn->status = LOGGED_STAT;
	    PRINTF("%s Password correct\r\n", inet_ntoa(conn->remote.sin_addr));
	} else {
	    /* incorrect password */
	    do_send_reply(conn->sockfd, "530 Login or Password incorrect. Bye!\r\n");
	    conn->status = ANON_STAT;
	    ret = -1;
	}
#endif
	do_send_reply(conn->sockfd, "230 Password OK. Current directory is %s\r\n", FTP_SRV_ROOT);
	break;

	/* Расширенный листинг */
    case CMD_LIST:
	do_send_reply(conn->sockfd, "150 Opening Binary mode connection for file list.\r\n");
	do_full_list(conn->currentdir, conn->pasv_sockfd);
	close(conn->pasv_sockfd);
	conn->pasv_active = 0;
	do_send_reply(conn->sockfd, "226 Transfer complete.\r\n");
	break;

	/* Простой листинг */
    case CMD_NLST:
	do_send_reply(conn->sockfd, "150 Opening Binary mode connection for file list.\r\n");
	do_simple_list(conn->currentdir, conn->pasv_sockfd);
	close(conn->pasv_sockfd);
	conn->pasv_active = 0;
	do_send_reply(conn->sockfd, "226 Transfer complete.\r\n");
	break;

	/* Ничего не делаем - это опция клиента */
    case CMD_TYPE:
	if (strcmp(parameter_ptr, "I") == 0) {
	    do_send_reply(conn->sockfd, "200 Type set to binary.\r\n");
	} else {
	    do_send_reply(conn->sockfd, "200 Type set to ascii.\r\n");
	}
	break;

	/* Чтение выбранного файла */
    case CMD_RETR:
	strcpy(spare_buf, buf + 5);
	do_full_path(conn, parameter_ptr, spare_buf, 256);
	num_bytes = do_get_filesize(spare_buf);

	if (num_bytes == -1) {
	    do_send_reply(conn->sockfd, "550 \"%s\" : not a regular file\r\n", spare_buf);
	    conn->offset = 0;
	    break;
	}

	/* Открыли файл на чтение */
	rc = f_open(&file, spare_buf, FA_READ);
	if (rc != FR_OK) {
	    PRINTF("ERROR: open file %s for reading\r\n", spare_buf);
	    break;
	}

	/* Двигаемся по файлу с начала */
	if (conn->offset > 0 && conn->offset < num_bytes) {
	    f_lseek(&file, conn->offset);
	    do_send_reply(conn->sockfd, "150 Opening binary mode data connection for partial \"%s\" (%d/%d bytes).\r\n",
			  spare_buf, num_bytes - conn->offset, num_bytes);
	} else {
	    do_send_reply(conn->sockfd, "150 Opening binary mode data connection for \"%s\" (%d bytes).\r\n", spare_buf, num_bytes);
	}
	/* Передаем данные файла. Перед посылков в сокет необходимо проверять num_bytes
	 * иначе можно послать 0 байт и будет ошибка передачи  */
	led_r = 0;
	do {
	    f_read(&file, sbuf, FTP_BUFFER_SIZE, (unsigned *) &num_bytes);
	    if (num_bytes > 0) {
		send(conn->pasv_sockfd, sbuf, num_bytes, 0);
		if (led_r++ % 50 == 0) {
		    //led_toggle(LED2);
		}
	    }
	    SLEEP(1);
	} while (num_bytes > 0);

	do_send_reply(conn->sockfd, "226 Finished.\r\n");
	f_close(&file);
	close(conn->pasv_sockfd);
	break;


	/* Запись файла в каталог FTP */
    case CMD_STOR:
	do_full_path(conn, parameter_ptr, spare_buf, 256);

	/* Открываем файло на запись */
	rc = f_open(&file, spare_buf, FA_WRITE | FA_OPEN_ALWAYS);
	if (rc != FR_OK) {
	    do_send_reply(conn->sockfd, "550 Cannot open \"%s\" for writing.\r\n", spare_buf);
	    break;
	}
	do_send_reply(conn->sockfd, "150 Opening binary mode data connection for \"%s\".\r\n", spare_buf);

	FD_ZERO(&readfds);
	FD_SET(conn->pasv_sockfd, &readfds);
	PRINTF("Waiting %d seconds for data...\r\n", tv.tv_sec);
	led_w = 0;

	while (select(conn->pasv_sockfd + 1, &readfds, 0, 0, &tv) > 0) {
	    if ((num_bytes = recv(conn->pasv_sockfd, sbuf, FTP_BUFFER_SIZE, 0)) > 0) {
		unsigned bw;
		f_write(&file, sbuf, num_bytes, &bw);
		if (led_w++ % 50 == 0) {
		    //led_toggle(LED1);
		}
	    } else if (num_bytes == 0) {
		f_close(&file);
		do_send_reply(conn->sockfd, "226 Finished.\r\n");
		break;
	    } else if (num_bytes == -1) {
		f_close(&file);
		ret = -1;
		break;
	    }
	}
	close(conn->pasv_sockfd);
	break;

	/* Размер файла */
    case CMD_SIZE:
	do_full_path(conn, parameter_ptr, spare_buf, 256);
	num_bytes = do_get_filesize(spare_buf);
	if (num_bytes == -1) {
	    do_send_reply(conn->sockfd, "550 \"%s\" : not a regular file\r\n", spare_buf);
	} else {
	    do_send_reply(conn->sockfd, "213 %d\r\n", num_bytes);
	}
	break;

	/* Вернуть время модификации файла */
    case CMD_MDTM:
	do_send_reply(conn->sockfd, "550 \"/\" : not a regular file\r\n");
	break;

	/* Система, на чем работает */
    case CMD_SYST:
	do_send_reply(conn->sockfd, "215 UNIX Type: L8\r\n");
	break;

	/* Текущий рабочий каталог */
    case CMD_PWD:
	do_send_reply(conn->sockfd, "257 \"%s\" is current directory.\r\n", conn->currentdir);
	break;

	/* Сменить директорию */
    case CMD_CWD:
	do_full_path(conn, parameter_ptr, spare_buf, 256);
	do_send_reply(conn->sockfd, "250 Changed to directory \"%s\"\r\n", spare_buf);
	strcpy(conn->currentdir, spare_buf);
	PRINTF("CWD: Changed to directory %s\r\n", spare_buf);
	break;

	/* Сменить директорию вниз */
    case CMD_CDUP:
	//sprintf(spare_buf, "%s/%s", conn->currentdir, "..");
	sprintf(spare_buf, "%s", conn->currentdir);
	do_step_down(spare_buf);
	strcpy(conn->currentdir, spare_buf);

	PRINTF("path: %s\r\n", conn->currentdir);
	do_send_reply(conn->sockfd, "250 Changed to directory \"%s\"\r\n", spare_buf);

	PRINTF("CDUP: Changed to directory %s\r\n", spare_buf);
	break;

	/* Активный режим - сервер открывает соединение. параметры порта передаются в строке 
	 * сделать СВОЙ порт 20 иначе некоторые клиенты не работают */
    case CMD_PORT:
	{
	    int portcom[6];
	    num_bytes = 0;
	    portcom[num_bytes++] = atoi(strtok(parameter_ptr, ".,;()"));
	    for (; num_bytes < 6; num_bytes++) {
		portcom[num_bytes] = atoi(strtok(0, ".,;()"));
	    }
	    sprintf(spare_buf, "%d.%d.%d.%d", portcom[0], portcom[1], portcom[2], portcom[3]);

	    FD_ZERO(&readfds);
	    if ((conn->pasv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		do_send_reply(conn->sockfd, "425 Can't open data connection.\r\n");
		close(conn->pasv_sockfd);
		conn->pasv_active = 0;
		break;
	    }


	    /* Получим адрес как число long и удаленный порт 
	     * Свой порт нужно обязательно поставить на 20!
	     * попробую сделать bind
	     */
	    local.sin_port = htons(FTP_DATA_PORT);
	    local.sin_addr.s_addr = INADDR_ANY;
	    local.sin_family = PF_INET;
	    if (bind(conn->pasv_sockfd, (struct sockaddr *) &local, addr_len) < 0) {
		PRINTF("ERROR: Bind socket\r\n");
	    }
	    PRINTF("SUCCESS: Bind socket to %d port\r\n", FTP_DATA_PORT);

	    pasvremote.sin_addr.s_addr = ((u32) portcom[3] << 24) | ((u32) portcom[2] << 16) | ((u32) portcom[1] << 8) | ((u32) portcom[0]);
	    pasvremote.sin_port = htons(portcom[4] * 256 + portcom[5]);
	    pasvremote.sin_family = PF_INET;
	    if (connect(conn->pasv_sockfd, (struct sockaddr *) &pasvremote, addr_len) == -1) {
		pasvremote.sin_addr = conn->remote.sin_addr;
		if (connect(conn->pasv_sockfd, (struct sockaddr *) &pasvremote, addr_len) == -1) {
		    do_send_reply(conn->sockfd, "425 Can't open data connection.\r\n");
		    close(conn->pasv_sockfd);
		    break;
		}
	    }
	    conn->pasv_active = 1;
	    conn->pasv_port = portcom[4] * 256 + portcom[5];
	    PRINTF("Connected to Data(PORT) %s @ %d\r\n", spare_buf, portcom[4] * 256 + portcom[5]);
	    do_send_reply(conn->sockfd, "200 Port Command Successful.\r\n");
	}
	break;

	/* Установить позицию в файле с какого читать */
    case CMD_REST:
	if (atoi(parameter_ptr) >= 0) {
	    conn->offset = atoi(parameter_ptr);
	    do_send_reply(conn->sockfd, "350 Send RETR or STOR to start transfert.\r\n");
	}
	break;

	/* Создать директорию */
    case CMD_MKD:
	do_full_path(conn, parameter_ptr, spare_buf, 256);
	if (f_mkdir(spare_buf)) {
	    do_send_reply(conn->sockfd, "550 File \"%s\" exists.\r\n", spare_buf);
	} else {
	    do_send_reply(conn->sockfd, "257 directory \"%s\" was successfully created.\r\n", spare_buf);
	}
	break;

	/* Удалить файл */
    case CMD_DELE:
	do_full_path(conn, parameter_ptr, spare_buf, 256);
	if (f_unlink(spare_buf) == FR_OK)
	    do_send_reply(conn->sockfd, "250 file \"%s\" was successfully deleted.\r\n", spare_buf);
	else {
	    do_send_reply(conn->sockfd, "550 Not such file or directory: %s.\r\n", spare_buf);
	}
	break;

	/* Удаляем директорию */
    case CMD_RMD:
	do_full_path(conn, parameter_ptr, spare_buf, 256);
	if (f_unlink(spare_buf)) {
	    do_send_reply(conn->sockfd, "550 Directory \"%s\" doesn't exist.\r\n", spare_buf);
	} else {
	    do_send_reply(conn->sockfd, "257 directory \"%s\" was successfully deleted.\r\n", spare_buf);
	}
	break;

	/* Пассивный режим - мы говорим к какому порту к нам подцепиться чтобы передать данные */
#if 1
    case CMD_PASV:
	do {
	    int dig1, dig2;
	    int sockfd;
	    extern struct ip_addr my_ipaddr;

	    conn->pasv_port = FTP_PASSV_PORT;
	    conn->pasv_active = 1;
	    local.sin_port = htons(conn->pasv_port);
	    local.sin_addr.s_addr = INADDR_ANY;
	    local.sin_family = PF_INET;

	    dig1 = (int) (conn->pasv_port / 256);
	    dig2 = conn->pasv_port % 256;

	    FD_ZERO(&readfds);
	    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		do_send_reply(conn->sockfd, "425 Can't open data connection.\r\n");
		PRINTF("Can't make passive TCP socket\r\n");
		ret = 1;
		break;
	    }

	    if (bind(sockfd, (struct sockaddr *) &local, addr_len) == -1) {
		do_send_reply(conn->sockfd, "425 Can't open data connection.\r\n");
		PRINTF("Can't bind passive socket\r\n");
		ret = 3;
	    }
	    if (listen(sockfd, 1) == -1) {
		do_send_reply(conn->sockfd, "425 Can't open data connection.\r\n");
		PRINTF("Can't listen passive socket\r\n");
		ret = 4;
		break;
	    }
	    /* PRINTF("Listening %d seconds @ port %d\r\n", tv.tv_sec, conn->pasv_port); */

	    /* Послать свой IP адрес и свой порт к которому нужно прицепиться */
	    do_send_reply(conn->sockfd, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n",
			  (u8)(my_ipaddr.addr >> 0),
                          (u8)(my_ipaddr.addr >> 8),
                          (u8)(my_ipaddr.addr >> 16),
                          (u8)(my_ipaddr.addr >> 24),
                          dig1, dig2);

	    FD_SET(sockfd, &readfds);
	    select(sockfd + 1, &readfds, 0, 0, &tv);
	    if (FD_ISSET(sockfd, &readfds)) {
		conn->pasv_sockfd = accept(sockfd, (struct sockaddr *) &pasvremote, (socklen_t *) & addr_len);
		if (conn->pasv_sockfd < 0) {
		    PRINTF("Can't accept socket\r\n");
		    do_send_reply(conn->sockfd, "425 Can't open data connection.\r\n");
		    ret = 5;
		    break;
		} else {
		    PRINTF("Got Data(PASV) connection from %s\r\n", inet_ntoa(pasvremote.sin_addr));
		    conn->pasv_active = 1;
		    close(sockfd);
		}
	    } else {
		ret = 6;
		break;
	    }
	} while (0);
	/* Если ошибка, ставим ret = 0 и пробуем команду PORT */
	if (ret) {
	    PRINTF("Select err\r\n");
	    close(conn->pasv_sockfd);
	    conn->pasv_active = 0;
	    ret = 0;
	}
	break;
#endif

	/* Никаких фич у нас нет */
    case CMD_FEAT:
	do_send_reply(conn->sockfd, "211 No-features\r\n");
	break;

	/* Завершение работы */
    case CMD_QUIT:
	do_send_reply(conn->sockfd, "221 Adios!\r\n");
	ret = -1;		/* В этом случае также пишем  */
	break;

	/* НЕИЗВЕСТНАЯ КОМАНДА */
    default:
	do_send_reply(conn->sockfd, "502 Not Implemented yet.\r\n");
	break;
    }

    if (sbuf) {
	free(sbuf);
    }
    return ret;
}
