stm32_ftp_server
Для FTP

Библиотеки использовал те, что уже были сделаны для предыдущего проекта т.е. не менял ни FreeRTOS.lib, ни FatFS, ни другие, поэтому необходимо проследить за стеком и остальной памятью

Добавил функцию ftp_send_reply() - отправка ответа FTP
Добавил функцию для определения входящей команды
Добавил пассивный режим, когда сервер FTP назначает порт данных, к какому присоединиться
Порт PASV 10000
Добавил в PASV посылку своего адреса
Разобраться с другими клиентами - они посылают 0 байт и сервер считает это разъединением
Добавил bind() на 20 порт (вернее FTP_PORT - 1) в команду PORT. FTP сервер можно запускать на любом порту, главное, чтобы порт данных был на единицу меньше порта команд
Дернул из lwip функцию inet_ntoa для вывода IP адреса. Ее нет в библиотеке для сс3200.
Заменил print_ip_addr() на inet_ntoa() в PRINTF
Все include включил в main.h
Убрал ошибку в сс3200: "[SOCK ERROR] - TX FAILED: socket NN, reason (NN)" при передаче могло передаться 0 байт
Заменил osi_Sleep() на SLEEP() который или объявлен или нет если нет-в CMD_RETR скорость повышается в 2 раза
Добавил функцию для перехода вниз по '..', еще нужно считать количество точек и слешей - доделать!
Каждый FTP клиент по разному соединяется, т.к. у них может быть различный формат. Хром не может пролистать директорию, так как посылает название со слешем
FileZilla при передаче файлов пытается открыть несколько соединений
Не хочет записывать длинные имена - нужно перекомпилировать fatfs с соответсвующей опцией
Добавил do_step_down() для перехода на каталог вниз. Разные клиенты по разному это задают или 'CDUP' или 'CWD ..'. Проверил в FAR и Total Comander
Некоторые команды от клиентов разрывают связь. На сс3200 сеть полностью вылетает, на stm32 - вылетает только сокет ftp. Поставить таймер, который проверяет наличие сети и перезапускать ее в случае падения.
Отключил полностью проверку login:passwd - разбираюсь почему вылетает сеть при работе с браузерами
Закоментировал PASV. Пока не знаю как избавиться от повторного вхождения
Добавил NOOP - посылает время от времени, чтобы сервер не заснул
Убрал селекты из главной задачи, закоментировал PASV, так как у меня нет опциии REUSEADDR.
Добавлены команды RENAME FROM и RENAME TO
Добавить таймер, который разрывает соединение при отсутсвии активности 1 минуту
