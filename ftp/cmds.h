#ifndef _CMDS_H
#define _CMDS_H

#include "main.h"
#include "ftpd.h"

int  do_full_list(char *, int);
int  do_simple_list(char *, int);
void do_send_reply(int, char *, ...);
int  do_get_filesize(char *);
int  do_parse_command(char *);
int  do_step_down(char *);
int  do_full_path(struct conn *, char *, char *, size_t);


#endif /* cmds.h */
