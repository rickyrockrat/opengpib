/** \file ******************************************************************
\n\b File:        get_tek_waveform.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008 12:45 pm
\n\b Description: open port to GPIB device, init it, grab initial data from 
scope, then dump that into a file.
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
*/
 
#include "serial.h"
#include <time.h>
#if 0
struct prologix_port {
	char phys_name[MAX_PORT_NAME];  /* physical device name of this interface, i.e the /dev/xxx. */
	
};
struct gpib_handle {
	struct gpib_port port;	
	
};

/***************************************************************************/
/** Open the GPIB port.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct gpib_handle *open_gpib(struct gpib_port *port)
{
	
}

#endif
#define BUF_SIZE 8096
char buf[BUF_SIZE];

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void _usleep(int usec)
{
	struct timespec t,r;
	r.tv_sec=usec/1000;
	usec/=1000;
	r.tv_nsec=usec*1000;
	nanosleep(&t,&r);	
	/** do{
		memcpy(&t,&r, sizeof(struct timespec));
	
	}while(r.tv_sec || r.tv_nsec);*/
	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char read_port (struct serial_port *p)
{
	int res;
	char ch;
	res = read(p->handle,&ch,1);
  /*    printf("1"); */
  if( res <1 ) {
    if( errno == EAGAIN || res == 0) /* EAGAIN - no data avail */
      usleep(60); /**for 15200 baud  */
    else  {
      printf("We had a Read Error on %s.\n",p->phys_name);
			return -1;
    }
  }
	return ch;
}

/***************************************************************************/
/** .
\n\b Arguments:
len is max_len for msg.
\n\b Returns: number of chars read.
****************************************************************************/
int read_string(struct serial_port *p, char *msg, int len)
{
	int i,count;

	fd_set rd;
	struct timeval tm;
	tm.tv_sec=1;
	tm.tv_usec=0;
	FD_ZERO(&rd);
	FD_SET(p->handle, &rd);
	select (p->handle+1, &rd, NULL, NULL, &tm);
#if 0		
	/*printf("RD\n"); */
	
	for (i=0; i<len;++i){
		int x;

		for (count=0;count<20;++count){
			/*printf("Sl\n"); */
			if(select (p->handle+1, &rd, NULL, NULL, &tm) < 0)	{
				if ((errno != EINTR) && (errno != EAGAIN)
				    && (errno != EINVAL)) {
					printf("Key input err\n");
					return (-1);
				} 
			}	
			if(0 != FD_ISSET(p->handle, &rd)){
				break;
			}
		}
		if(1 != (x=read(p->handle,&msg[i],1)) )
			perror("");
			/*usleep(100); */
		
		if(x != 1 ){
			
			printf("Timeout waiting on char\n");
			return -1;
		}
		printf("%c",msg[i]);
		if(msg[i]=='\n' || msg[i] == '\r')/* || msg[i] == '\r') */
			break;
	}
#else
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
#endif
		
#if 0	
	if( (rtn=read(p->handle,msg,len)) <1 ) {
    if( errno == EAGAIN || rtn == 0) /* EAGAIN - no data avail */
      usleep(60); /**for 15200 baud  */
    else  {
      printf("We had a Read Error on %s.\n",p->phys_name);
			return -1;
    }
  }
#endif
	while(i && (buf[i]=='\r' || buf[i]=='\n'))
		--i;
	++i;
	buf[i]=0;
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_port(struct serial_port *p, char b)
{
	if(write(p->handle,&b,1) <0) {/* error writing */
    printf("Error Writing to %s\n",p->phys_name);
		return -1;
  }
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_string(struct serial_port *p, char *msg, int len)
{
	int rtn,i;
	if(NULL ==msg){
		printf("msg null\n");
		return -1;
	}
	if(0 == len)
		i=strlen(msg);
	else
		i=len;
	if((rtn=write(p->handle,msg,i)) <0) {/* error writing */
    printf("Error Writing to %s\n",p->phys_name);
		return -1;
  }
	printf("wrote %d bytes\n%s\n",rtn,msg);
	if(rtn != i)
		printf("Write mis-match %d != %d\n",rtn,i);
	usleep(500); 
	return rtn;	
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void verify(struct serial_port *p, char *msg)
{
	int i;
	i=sprintf(buf,"%s\r",msg);
	write_string(p,buf,i);
	read_string(p,buf,BUF_SIZE);
	printf("%s = %s\n",msg,buf);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_prologix(struct serial_port *p, int inst_addr)
{
	int i;
	printf("Init Prologix controller\n");
	read_string(p,buf,BUF_SIZE);
	/*make sure auto reply is on*/
	write_string(p,"++auto 1\r",0);
	write_string(p,"++ver\r",0);
	/*write_string(p,"++read\r",0); */
	
	if(-1 == (i=read_string(p,buf,BUF_SIZE)) ){
		printf("%s:Unable to read from port on ver\n",__func__);
		return -1;
	}
	printf("Got %d bytes\n",i);
	if(NULL == strstr(buf,"Prologix")){
		printf("%s:Unable to find correct ver in\n%s\n",__func__,buf);
		return -1;
	}
	printf("Talking to Controller '%s'\n",buf);
	/*Then set to Controller mode */
	write_string(p,"++mode 1\r",0);
	
	/*Set the address to talk to (2, usually) */
	sprintf(buf,"++addr %d\r",inst_addr);
	write_string(p,buf,0);
	/*Set the eoi mode (EOI after cmd) */
	write_string(p,"++eoi 1\r",0);
	/*Set the eos mode (LF) */
	write_string(p,"++eos 2\r",0);
	verify(p,"++addr");
	verify(p,"++eoi");
	verify(p,"++eos");
	verify(p,"++auto");
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_instrument(struct serial_port *p, int channel)	 
{
	int i;
	printf("Initializing Instrument\n");
	write_string(p,"id?\r",0);
	if(-1 == (i=read_string(p,buf,BUF_SIZE)) ){
		printf("%s:Unable to read from port on id\n",__func__);
		return -1;
	}
	printf("Got %d bytes\n",i);
	if(NULL == strstr(buf,"TEK/2440")){
		printf("Unable to find 'TEK/2440' in id string '%s'\n",buf);
		return -1;
	}
	write_string(p,"DATA ENCDG:ASCI\r",0);
	i=sprintf(buf,"DATA SOURCE:CH%d\r",channel);
	write_string(p,buf,i);
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_data(struct serial_port *p)
{
	int i;
	write_string(p,"WAV?\r",0);
	i=read_string(p,buf,BUF_SIZE);
	return i;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("get_tek_waveform <options>\n"
	" -a addr set instrument address to addr (2)\n"
	" -c n Use channel n for data (1)\n"
	" -o fname put output to file called fname\n"
	" -p path set path to serial port name\n"
	
	"");
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct serial_port *p;
	char *name, *ofname;
	int c,inst_addr,channel, rtn, ofd;
	name="/dev/ttyUSB0";
	c=1;
	inst_addr=2;
	channel=1;
	rtn=1;
	ofname=NULL;
	while( -1 != (c = getopt(argc, argv, "a:c:o:p:")) ) {
		switch(c){
			case 'a':
				inst_addr=atoi(optarg);
				break;
			case 'c':
				channel=atoi(optarg);
				break;
			case 'o':
				ofname=strdup(optarg);
				break;
			case 'p':
				name=strdup(optarg);
				break;
		}
	}
	if(NULL == ofname)
		ofd=1;
	else	if(0>(ofd=open(ofname,O_RDWR|O_CREAT,S_IROTH|S_IRGRP|S_IWUSR|S_IRUSR))) {
		printf("Unable to open '%s' for writing\n",ofname);
		return 1;
	}
	if(NULL == (p=open_serial_port(name,115200,0,0,0))){
		printf("Can't open %s. Fatal\n",name);
		return 1;
	}
	printf("Serial port opened.\n");
	if(-1 == init_prologix(p,inst_addr)){
		printf("Controller init failed\n");
		goto closem;
	}
	
	if(-1 == init_instrument(p,channel)){
		printf("Unable to initialize instrument\n");
		goto closem;
	}
	if(-1 == (c=get_data(p)) ){
		printf("Unable to get waveform??\n");
		goto closem;
	}
	write(ofd,buf,c);
	rtn=0;
closem:
	if(ofd>1)
		close(ofd);
	close_serial_port(p);
	return rtn;
}
