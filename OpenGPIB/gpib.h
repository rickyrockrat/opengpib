/** \file ******************************************************************
\n\b File:        gpib.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
*/
#ifndef _GPIB_H_
#define _GPIB_H_ 1

#include "serial.h"
#include <sys/time.h>
#define BUF_SIZE 8096
int read_string(struct serial_port *p, char *msg, int len);
int write_string(struct serial_port *p, char *msg, int len);
void verify(struct serial_port *p, char *msg);
int init_prologix(struct serial_port *p, int inst_addr);
#endif
