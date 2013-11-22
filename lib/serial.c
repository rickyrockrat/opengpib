/** \file ******************************************************************
\n\b File:        serial.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008  4:21 pm
\n\b Description: Implementation of an interface over a serial port.
*/ /************************************************************************
Change Log: \n
 This file is part of OpenGPIB.
 For details, see http://opengpib.sourceforge.net/projects

 Copyright (C) 2008-2009 Doug Springer <gpib@rickyrockrat.net>
   
    OpenGPIB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 
    as published by the Free Software Foundation. Note that permission 
    is not granted to redistribute this program under the terms of any
    other version of the General Public License.

    OpenGPIB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenGPIB.  If not, see <http://www.gnu.org/licenses/>.
    
		The License should be in the file called COPYING.

*/

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */


#include "open-gpib.h"
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h> /* for errno */

#define FLAGS_SET 1
#define TERMIO_SET 2
#define SER_DEFAULT_PORT_NAME "/dev/ttyS"
struct serial_port {               
	int handle;                 /* handle used to talk to this interface. */
  int port;                   /* port number,  1-256. Linux: ttyS0=port1*/
  char *phys_name; 						/* physical device name of this interface. */	
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
	int timeout; /**inter char timeout in uS  */
	int debug; /**if set, turn on prints  */
};
  
int init_port_settings( struct serial_port *p, char *name);
int _open_serial_port(struct serial_port *p);
void restore_serial_port(struct serial_port *p);
int close_serial_port( struct serial_port *p);

/****************************************************************************
initialize the serial port information to default states.
****************************************************************************/
int init_port_settings( struct serial_port *p,char *name)
{
	
	if( p->port == 0) p->port =1;
	/* note: port_handle is moved to the thread struct */
	if(name == NULL){
		if(NULL == (p->phys_name=malloc(strlen(SER_DEFAULT_PORT_NAME)+10) )){
			printf("Out of Mem alloc serial port name!\n");
			return 1;
		}
		sprintf(p->phys_name,SER_DEFAULT_PORT_NAME"%d",p->port);
		
	} else {
		if(NULL == (p->phys_name=strdup(name))){
			printf("Out of Mem strdup serial port name!\n");
			return 1;
		}
	}
		
	
	p->lf=p->cr=1;
	p->addlinefeed=0;
	p->addcreturn=0;
	p->linewrap=80;
	/* only set these to default if they are not already set. */
	
	if( p->baud == 0)     p->baud =       9600;
	if( p->parity ==0)    p->parity =     'N';
	if( p->data_bits ==0) p->data_bits =  8;
	if( p->stop ==0)      p->stop =       1;
	if( p->flow ==0)      p->flow =       'N';
	return 0;
}

/****************************************************************************
validate the baud rate.
****************************************************************************/
tcflag_t  get_baud ( long baud)
{
switch(baud)
  {
  case 110: return(B110);
  case 300: return(B300);
  case 600: return(B600);
  case 1200: return(B1200);
  case 2400: return(B2400);
  case 4800: return(B4800);
  case 9600: return(B9600);
  case 19200: return(B19200);
  case 38400: return(B38400);
  case 57600: return(B57600);
  case 115200: return(B115200);
  case 230400: return(B230400);
  default: return(B4000000);
  }
}

/****************************************************************************
validate the data bits
****************************************************************************/
tcflag_t  get_data_bits ( int data_bits )
{
switch(data_bits)
  {
  case 5: return( CS5);
  case 6: return( CS6);
  case 7: return( CS7);
  case 8: return( CS8);
  default: return(-1);
  }
}

/****************************************************************************
****************************************************************************/
tcflag_t  get_stop_bits ( int stop)
{
switch(stop)
  {
  case 1: return(0);
  case 2: return(CSTOPB);
  default: return(-1);
  }
}


/****************************************************************************
****************************************************************************/
tcflag_t get_parity (int parity)
{
/* Warning!! this is not currently used.  Set to NoParity */
switch(parity)
  {
  case 'N': return(0); break;
  case 'E': return(PARENB); break;
  case 'O': return(PARENB|PARODD); break;
  default: return(-1);
  }
return(0);
}

/***************************************************************************/
/**U16 open_setup_port (PORT_INFO *p, tcflag_t baud, tcflag_t data, tcflag_t stop, tcflag_t parity)
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int _open_serial_port(struct serial_port *p)
{

	tcflag_t  baud;
	tcflag_t  data_bits;
	tcflag_t  stop_bits;
	tcflag_t  parity;
	
	if(p->debug) printf("Checking baud %ld ",p->baud);
  if ((baud = get_baud(p->baud)) == B4000000) {
    printf("Can't handle baud=%ld\n",p->baud);
    return(-1);
  }
	
	
	if(p->debug) printf("Checking Data Width %d ",p->data_bits);
	if(-1 == (data_bits = get_data_bits(p->data_bits)) ) {
	  printf("Can't handle bits=%d. Must be 5,6,7,8\n",p->data_bits);
	  return(-1);
	}
	
	if(p->debug) printf("Checking Stop %d\n",p->stop);
	  if(-1 == (stop_bits = get_stop_bits(p->stop)) ) {
	    printf("Can't handle stop bits=%d. Must be 1,2\n",p->stop);
	    return(-1);
	  }
	
	if(p->debug) printf("Checking Parity %c ",p->parity);
	  if ( -1 == (parity = get_parity(p->parity)) ) {
	    printf("Can't handle parity=%c. Must be N,E,O\n",p->parity);
	    return(-1);
	  }
	
	if(p->debug) printf("Opening Port '%s'\n",p->phys_name);
		
	/* FIXME:  add software, hardware flow... */
	
	/*
	Open modem device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
	*/
	p->handle = open(p->phys_name, O_RDWR |O_EXCL| O_NOCTTY |O_NONBLOCK);
	
	if(p->handle == -1)
	  {
	  printf("Open Failed on %s.\n",p->phys_name);
	  return(-1);
	  }
 	tcgetattr(p->handle,&(p->old_pio)); /* save current serial port settings */
	p->ptermset|=TERMIO_SET;
 	bzero(&(p->new_pio), sizeof(p->new_pio)); /* clear struct for new port settings */
	/*
	  c_iflag - input modes   IXON, IXOFF, IXANY, INPCK
	  c_oflag - output modes    CSx CSTOPB PARENB PARENODD
	  c_cflag - control modes
	  c_lflag - local modes
	  c_cc    - control chars*/
	
	/*BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
	  CRTSCTS : output hardware flow control (only used if the cable has
	            all necessary lines. See sect. 7 of Serial-HOWTO)
	  CS8     : 8n1 (8bit,no parity,1 stopbit)
	  CLOCAL  : local connection, no modem contol
	  CREAD   : enable receiving characters
	*/
	/* p->new_io.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD; */
	
	p->new_pio.c_cflag = baud | data_bits |stop_bits| CLOCAL | CREAD;
	
	/*FIXME: ??p->new_io.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD; */
	/*
	  IGNPAR  : ignore bytes with parity errors
	  ICRNL   : map CR to NL (otherwise a CR input on the other computer
	            will not terminate input)
	  otherwise make device raw (no other input processing)
	*/
	/* FIXME: Fixed??  which one of these flags work? */
	/*p->new_io.c_iflag = 0 ; */
	
	/*p->new_io.c_iflag = IGNPAR | ICRNL ; */
	p->new_pio.c_iflag = IGNBRK ;
	
	/*p->new_io.c_iflag = IGNBRK ; */
	/*p->new_io.c_iflag &= ~(IXON|IXOFF|IXANY); */
	
	/* Raw output.*/
	p->new_pio.c_oflag = 0;
	
	/*  ICANON  : enable canonical input
	  disable all echo functionality, and don't send signals to calling program*/
	/* (p->new_io).c_lflag = ICANON; */
	 /* set input mode (non-canonical, no echo,...) */
	
	p->new_pio.c_lflag = 0;
	
	/*initialize all control characters
	  default values can be found in /usr/include/termios.h, and are given
	  in the comments, but we don't need them here */
	p->new_pio.c_cc[VINTR]    = 0;     /* Ctrl-c */
	p->new_pio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	p->new_pio.c_cc[VERASE]   = 0;     /* del */
	p->new_pio.c_cc[VKILL]    = 0;     /* @ */
	p->new_pio.c_cc[VEOF]     = 0;     /* Ctrl-d */
	p->new_pio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	p->new_pio.c_cc[VMIN]     = 0;     /* blocking read until 1 character arrives */
	p->new_pio.c_cc[VSWTC]    = 0;     /* '\0' */
	p->new_pio.c_cc[VSTART]   = 0;     /* Ctrl-q */
	p->new_pio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	p->new_pio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	p->new_pio.c_cc[VEOL]     = 0;     /* '\0' */
	p->new_pio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	p->new_pio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	p->new_pio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	p->new_pio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	p->new_pio.c_cc[VEOL2]    = 0;     /* '\0' */
	
	/*
	  now clean the modem line and activate the settings for the port
	*/
	if(p->debug) printf("\nFlushing Serial Line\n");
	tcflush(p->handle, TCIOFLUSH);
	if(p->debug) printf("Activating Settings.\n");
	tcsetattr(p->handle,TCSANOW,&(p->new_pio));
	p->ptermset|=TERMIO_SET;
	/* set overall connect flag */
	/*p->connected=TRUE; */
	
	return( 0 );
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void restore_serial_port(struct serial_port *p)
{
	if(p->termset&FLAGS_SET)
		fcntl(0,F_SETFL,p->oldflags);
	if(p->termset&TERMIO_SET)
		tcsetattr(0,TCSANOW,&p->old_io);  /* restore.. */	
	if(p->ptermset&TERMIO_SET && -1 != p->handle)
		tcsetattr(p->handle,TCSANOW,&p->old_pio);  /* restore.. */	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_serial_port(char *name, int baud, int data, int stop, int parity, struct serial_port *p)
{
	p->handle=-1;
	p->baud=baud;
	p->stop=stop;
	p->data_bits=data;
	p->parity=parity;
	if(init_port_settings(p,name))
		goto error;

  if( -1 ==  _open_serial_port(p) ) {
		goto error;
	}	
	p->timeout=50000;
	return 0;
error:
	restore_serial_port(p);
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int close_serial_port( struct serial_port *p)
{
	if(NULL == p){
		printf("%s: struct ptr is NULL\n",__func__);
		return -1;
	}
	restore_serial_port(p);	
	if(-1 != p->handle){
		close(p->handle);
		p->handle=-1;
	}
	free (p);
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number of byte read otherwise
****************************************************************************/
static int read_if(struct open_gpib_dev *d, void *buf, int len)
{
	struct serial_port *p; 				 
	int i,count;
	char *msg;
	fd_set rd;
	struct timeval tm;
	
	p=(struct serial_port *)d->dev;	 
	msg=(char *)buf;
	
	FD_ZERO(&rd);
	FD_SET(p->handle, &rd);

	for (i=0; i<len;){
		int x;
		tm.tv_usec=p->timeout;
		tm.tv_sec=0;
		/*printf("!"); */
		if(select (p->handle+1, &rd, NULL, NULL, &tm)<1){
			/**no chars avail.  */
			break;
		}
		for (count=0;count<100;++count){
			if( (x=read(p->handle,&msg[i],len-i)) >0 ){
		/*		printf("%02X ",msg[i]); */
				i+=x;
				break;
			}
		}
	}
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
static int write_if(struct open_gpib_dev *d, void *buf, int len)
{
	int i;
	struct serial_port *p;
	p=(struct serial_port *)d->dev;
	if((i=write(p->handle,buf,len))<0)
		printf("Error Writing to %s\n",p->phys_name);
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int close_if(struct open_gpib_dev  *d)
{
	return close_serial_port((struct serial_port *)d->dev);
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, 0 on success
****************************************************************************/
static int open_if(struct open_gpib_dev  *d, char *path)
{
	struct serial_port *p;
	if(NULL == d){
		printf("serial open: dev null\n");
		return -1;
	}
	p=(struct serial_port *)d->dev;
	if(check_calloc(sizeof(struct serial_port), &p,__func__,NULL) ) return -1;
		
	p->debug=d->debug;
	if(-1 == open_serial_port(path,115200,0,0,0,p)) {
		printf("Can't open %s. Fatal\n",path);
		free(p);
		return -1;
	}
	d->dev=(void *)p;
	if(d->debug) printf("Serial port opened.\n");

	return 0; 
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int control_if(struct open_gpib_dev  *d, int cmd, uint32_t data)
{
	struct serial_port *p;
	p=(struct serial_port *)d;
	if(-1 == check_calloc(sizeof(struct serial_port), &p,__func__,d->dev) ) return -1;
	switch(cmd){
		case SERIAL_CMD_SET_CHAR_TIMEOUT:
			p->timeout=data;
			break;
		case SERIAL_CMD_SET_BAUD:
			p->baud=data;
			break;
		case SERIAL_CMD_SET_PARITY:
			p->parity=data;
			break;
		case SERIAL_CMD_SET_DATA_BITS:
			p->data_bits=data;
			break;
		case SERIAL_CMD_SET_STOP:
			p->stop=data;
			break;
		case SERIAL_CMD_SET_FLOW:
			p->flow=data;
			break;
		case CMD_SET_DEBUG:
			if(data)
				p->debug=1;
			else
				p->debug=0;
		  break;
		default:
			fprintf(stderr,"%s unknown cmd %d\n",__func__,cmd);
			return -1;
			break;

	}
	return 0;
}

/***************************************************************************/
/** Allocate our internal data structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void *calloc_internal(void)
{
	void *p;
	if(-1 == check_calloc(sizeof(struct serial_port), &p,__func__,NULL) ) 
		return NULL;
	return p;
}
/***************************************************************************/
/** Nothing to do. Done in open.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int init_if(struct open_gpib *d)
{
	
}

/**set up auto-generation of commands and the register function via build-interfaces	*/
GPIB_TRANSPORT_FUNCTION(serial)
OPEN_GPIB_ADD_CMD(SERIAL_CMD_SET_CHAR_TIMEOUT)
OPEN_GPIB_ADD_CMD(SERIAL_CMD_SET_BAUD)
OPEN_GPIB_ADD_CMD(SERIAL_CMD_SET_PARITY)
OPEN_GPIB_ADD_CMD(SERIAL_CMD_SET_DATA_BITS)
OPEN_GPIB_ADD_CMD(SERIAL_CMD_SET_STOP)
OPEN_GPIB_ADD_CMD(SERIAL_CMD_SET_FLOW)

