/** \file ******************************************************************
\n\b File:        serial.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008  4:21 pm
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.1  2008/08/02 08:53:58  dfs
Initial working rev

*/

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */


#include "gpib.h"
#include "serial.h"
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> /* for select.... */
#include <stdio.h>         
#include <errno.h> /* for errno */
#include <stdlib.h> /*for malloc, free */
#include <string.h> /* for memset, strcpy,sprintf.... */

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
};
  
int init_port_settings( struct serial_port *p, char *name);
int _open_serial_port(struct serial_port *p);
void restore_serial_port(struct serial_port *p);
struct serial_port *open_serial_port(char *name, int baud, int data, int stop, int parity);
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
	
	printf("Checking baud %ld ",p->baud);
	  if ((baud = get_baud(p->baud)) == B4000000) {
	    printf("Can't handle baud=%ld\n",p->baud);
	    return(-1);
	  }
	
	
	printf("Checking Data Width %d ",p->data_bits);
	  if(-1 == (data_bits = get_data_bits(p->data_bits)) ) {
	    printf("Can't handle bits=%d. Must be 5,6,7,8\n",p->data_bits);
	    return(-1);
	  }
	
	printf("Checking Stop %d\n",p->stop);
	  if(-1 == (stop_bits = get_stop_bits(p->stop)) ) {
	    printf("Can't handle stop bits=%d. Must be 1,2\n",p->stop);
	    return(-1);
	  }
	
	printf("Checking Parity %c ",p->parity);
	  if ( -1 == (parity = get_parity(p->parity)) ) {
	    printf("Can't handle parity=%c. Must be N,E,O\n",p->parity);
	    return(-1);
	  }
	
	printf("Opening Port '%s'\n",p->phys_name);
		
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
	printf("\nFlushing Serial Line\n");
	tcflush(p->handle, TCIOFLUSH);
	printf("Activating Settings.\n");
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
struct serial_port *open_serial_port(char *name, int baud, int data, int stop, int parity)
{
	struct serial_port *p;
	if(NULL == (p=malloc(sizeof(struct serial_port))) ){
		printf("%s: out of memory\n",__func__);
		return NULL;
	}
	bzero(p,sizeof(struct serial_port));
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
	return p;
error:
	restore_serial_port(p);
	free(p);
	return NULL;
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
int _serial_read(void *dev, void *buf, int len)
{
	struct serial_port *p;
	int i,count;
	char *msg;
	fd_set rd;
	struct timeval tm;
	
	p=(struct serial_port *)dev;	
	msg=(char *)buf;
	
	tm.tv_sec=1;
	tm.tv_usec=0;
	FD_ZERO(&rd);
	FD_SET(p->handle, &rd);
	select (p->handle+1, &rd, NULL, NULL, &tm);

		for (i=0; i<len;++i){
		int x;
		for (count=0;count<2000;++count){
			if(0< (x=read(p->handle,&msg[i],len-i)) ){
				i+=x-1;
				break;
			}
				
			usleep(100);
		}
		if(0 == x){	
			if(0 == i || msg[i-1] != '\r' )	{
				printf("Timeout waiting on char\n");
				return -1;
				
			}else{
				--i;
				break;	
			}
		}	
		/*printf("%c",msg[i]); */
		if(msg[i]=='\n' )/* || msg[i] == '\r') */
			break;
	 }
	while(i && (msg[i]=='\r' || msg[i]=='\n'))
		--i;
	++i;
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
int _serial_write(void *dev, void *buf, int len)
{
	struct serial_port *p;
	int i;
	p=(struct serial_port *)dev;
	if((i=write(p->handle,buf,len))<0)
		printf("Error Writing to %s\n",p->phys_name);
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int _serial_close(struct gpib *g)
{
	return close_serial_port((struct serial_port *)g->dev);
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: 1 on failure, 0 on success
****************************************************************************/
int _serial_open(struct gpib *g, char *path)
{
	if(NULL ==g){
		printf("serial open: gpib null\n");
		return 1;
	}
	if(NULL == (g->dev=(void *)open_serial_port(path,115200,0,0,0))){
		printf("Can't open %s. Fatal\n",path);
		return 1;
	}
	printf("Serial port opened.\n");

	return 0; 
}

/***************************************************************************/
/** allocate and fill in our function structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int serial_register(struct gpib *g)
{
	g->read=	_serial_read;
	g->write=	_serial_write;
	g->open=	_serial_open;
	g->close=	_serial_close;
	return 0;
}
