#include "cmds.h"

/* Месяцы для вывода директорий */
const char months[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "Ukn"
};


/**
 * Разбор входящей строки
 */
static int str_begin_with(const char *src, char *match)
{
    while (*match) {
	/* check source */
	if (*src == 0) {
	    return -1;
	}

	if (*match != *src) {
	    return -1;
	}
	match++;
	src++;
    }
    return 0;
}


/**
 * Посылка ответа FTP в сокет
 */
void do_send_reply(int sock, char *fmt, ...)
{
    static char str[256];
    va_list p_args;
    int r;

    va_start(p_args, fmt);
    r = vsnprintf(str, sizeof(str), fmt, p_args);
    va_end(p_args);

    /* printf(buf); */
    if (r > 0) {
	send(sock, str, r, 0);
    }
}

/**
 * Список текущей директории. Сделаем как это средствами CHAN FAT
 * сделать статическими параметры если будет выпадать
 * при нехватке стека
 */
int do_full_list(char *name, int sockfd)
{
    DIR dir;
    FILINFO fno;
    FRESULT rc;
    u16 day, mon, year;		/* дата */
    int res = -1;

/*    PRINTF("do full list of %s", name); */

    /* Хром ставит слеш в последнюю позицию */
    int len = strlen(name);
    if (len > 1 && name[len - 1] == '/') {
	name[len - 1] = 0;
    }


    do {
	/* Открываем директорию */
	rc = f_opendir(&dir, (TCHAR *) name);
	if (rc) {
	    do_send_reply(sockfd, "500 Internal Server Error\r\n");
	    break;
	} else {

	    for (;;) {
		/* Читаем директорию */
		rc = f_readdir(&dir, &fno);
		if (rc || !fno.fname[0]) {
		    break;	/* Конец директории */
		}

		day = fno.fdate & 0x1f;	// День 
		mon = ((fno.fdate >> 5) & 0x0f) - 1;	//месец
		year = ((fno.fdate >> 9) & 0x3f) + 1980;

		/* Если какая то ошибка */
		if (mon > 12) {
		    mon = 0;
		    year = 2000;
		}

		/* Если директория */
		if (fno.fattrib & AM_DIR) {
		    do_send_reply(sockfd, "drw-r--r-- 1 root root %9d %3s %2u %4u %s\r\n", 0, months[mon], day, year, fno.fname);
		} else {
		    /* Если файл */
		    do_send_reply(sockfd, "-rw-r--r-- 1 root root %9d %3s %2u %4u %s\r\n", fno.fsize, months[mon], day, year, fno.fname);
		}
		res = 0;
	    }
	}
    } while (0);
    
/*    PRINTF("...OK\r\n"); */
    
    return res;
}


/**
 * Это вариант листинга на команду NLST
 */
int do_simple_list(char *name, int sockfd)
{
    FILINFO fno;
    FRESULT rc;
    DIR dir;

/*    PRINTF("do simple list of %s", name);  */
    
    /* Открываем директорию */
    rc = f_opendir(&dir, (TCHAR *) name);
    if (rc != FR_OK) {
	do_send_reply(sockfd, "500 Internal Error\r\n");
	return -1;
    }
    
    /* Если открыли - пролистаем */
    while (1) {
	rc = f_readdir(&dir, &fno);
	if (rc || !fno.fname[0]) {
	    break;		/* Конец директории */
	}
	do_send_reply(sockfd, "%s\r\n", fno.fname);
    }
    
/*    PRINTF("...OK\r\n"); */
    return 0;
}

/**
 * Вернуть размер файла. Файл еще не открыт. Закрывается после использования
 */
int do_get_filesize(char *filename)
{
    FIL file;
    int res = -1;

    if (f_open(&file, (TCHAR*)filename, FA_READ) == FR_OK) {
	res = f_size(&file);
	f_close(&file);
    }

    return res;
}

/**
  * Возвращает номер входящей команды по enum
  * Эти 2 функции можно как то оптимизировать,
  * чтобы не вызывать каждый раз
  */
int do_parse_command(char *buf)
{
    if (str_begin_with(buf, "USER") == 0) {
	return CMD_USER;
    } else if (str_begin_with(buf, "PASS") == 0) {
	return CMD_PASS;
    } else if (str_begin_with(buf, "LIST") == 0) {
	return CMD_LIST;
    } else if (str_begin_with(buf, "NLST") == 0) {
	return CMD_NLST;
    } else if (str_begin_with(buf, "PWD") == 0 || str_begin_with(buf, "XPWD") == 0) {
	return CMD_PWD;
    } else if (str_begin_with(buf, "TYPE") == 0) {
	return CMD_TYPE;
    } else if (str_begin_with(buf, "PASV") == 0) {
	return CMD_PASV;
    } else if (str_begin_with(buf, "RETR") == 0) {
	return CMD_RETR;
    } else if (str_begin_with(buf, "STOR") == 0) {
	return CMD_STOR;
    } else if (str_begin_with(buf, "SIZE") == 0) {
	return CMD_SIZE;
    } else if (str_begin_with(buf, "MDTM") == 0) {
	return CMD_MDTM;
    } else if (str_begin_with(buf, "SYST") == 0) {
	return CMD_SYST;
    } else if (str_begin_with(buf, "CWD") == 0) {
	return CMD_CWD;
    } else if (str_begin_with(buf, "CDUP") == 0) {
	return CMD_CDUP;
    } else if (str_begin_with(buf, "PORT") == 0) {
	return CMD_PORT;
    } else if (str_begin_with(buf, "REST") == 0) {
	return CMD_REST;
    } else if (str_begin_with(buf, "MKD") == 0) {
	return CMD_MKD;
    } else if (str_begin_with(buf, "DELE") == 0) {
	return CMD_DELE;
    } else if (str_begin_with(buf, "RMD") == 0) {
	return CMD_RMD;
    } else if (str_begin_with(buf, "FEAT") == 0) {
	return CMD_FEAT;
    } else if (str_begin_with(buf, "QUIT") == 0) {
	return CMD_QUIT;
    } else {
	return CMD_UKNWN;
    }
}

/**
 * Спуститься на каталог вниз, если приходил команда перейти на директорию вниз
 * если несколько слешей - подсчитать их и спуститься на это количество!!!
 * доделать завтра
 */
int do_step_down(char *path)
{
    int len, i;
    int res = -1;

    len = strlen(path);

    /* находим первый слеш */
    for (i = len - 1; i > 0; i--) {
	if (path[i] == '/') {
	    path[i] = 0;
	    res = 0;
	    break;
	}
    }

    /* Если слеша не нашли - нужно спуститься в корень */
    if (res != 0) {
	path[0] = '/';
	path[1] = 0;
    }

    return 0;
}

/**
 * Сделать правильный путь, убрать двойные //
 * и сделать правильно при переходе на директроию вниз ../..
 */
int do_full_path(struct conn *conn, char *path, char *new_path, size_t size)
{
    if (path[0] == '/') {
	strcpy(new_path, path);
    } else if (strcmp("/", conn->currentdir) == 0) {
	sprintf(new_path, "%s", path);
    } else if (path[0] == '.' && path[1] == '.') {
	/* Нужно спуститься на ступень ниже */
	PRINTF("Suspicius path\r\n");
	do_step_down(conn->currentdir);
	sprintf(new_path, "%s", conn->currentdir);
    } else {
	sprintf(new_path, "%s/%s", conn->currentdir, path);
    }

    return 0;
}
