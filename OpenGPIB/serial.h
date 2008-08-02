/** \file ******************************************************************
\n\b File:        serial.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008  4:57 pm
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
*/
#ifndef _SERIAL_H_
#define _SERIAL_H_ 1
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> /* for select.... */
#include <stdio.h>         
#include <errno.h> /* for errno */
#include <stdlib.h> /*for malloc, free */
#include <string.h> /* for memset, strcpy,sprintf.... */

#define MAX_PORT_NAME 150
#if 0
typedef char (*CHAR_FUNCTION_PTR)(void);
typedef char (*CHAR_UCHAR_FUNCTION_PTR)(char);
typedef U16 (*GEN_IO)(void *handle, void *data);
typedef void *(*GEN_OPEN)(void *name);
typedef U16 (*GEN_CLOSE)(void *handle);
  GEN_OPEN open;                   /* pointer to function to open this interface */
  GEN_IO rd;                       /* pointer to function to get data from this interface */
  GEN_IO wr;                       /* pointer to function to put data into this interface */
  GEN_CLOSE close;                 /* pointer to function to close this interface */
#endif
struct serial_port {               
  int handle;                   /* handle used to talk to this interface. */
  int port;                        /* port number,  1-256. Linux: ttyS0=port1*/
  char    phys_name[MAX_PORT_NAME];  /* physical device name of this interface. */
  int    data_bits;                  /* 5,6,7,8 */
  int   parity;                     /* 'N', 'E', 'O' */
  int   stop;                       /* 1, 2 */
  char   flow;                       /* N, H, S = None, Hardware, Software (Xon/Xoff) */
  long   baud;                       /* 110 - 115200, standard baud rates */
  int    cr;                         /** if this is set, send a cr to terminal  */
  int   addlinefeed;                /** if this is set, send a linefeed for each cr  */
  int   lf;                         /** if this is set, send a lf to terminal  */
  int   addcreturn;                 /** if this is set, send a carrige return for each lf */
  int   linewrap;                   /** if this is set, we use this for num cols  */
/**this is for the terminal  */	
  struct termios old_io;
  struct termios new_io;
	int oldflags;
	int newflags;	
	int termset;										 /**1 indicates the terminal has been messed with.  */
/**this is for the serial port  */
  struct termios old_pio;
  struct termios new_pio;	
	int oldpflags;
	int newpflags;	
	int ptermset;
  };
  
void init_port_settings( struct serial_port *p, char *name);
int _open_serial_port(struct serial_port *p);
void restore_serial_port(struct serial_port *p);
struct serial_port *open_serial_port(char *name, int baud, int data, int stop, int parity);
int close_serial_port( struct serial_port *p);
#endif

