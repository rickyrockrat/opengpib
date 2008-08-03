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
Revision 1.2  2008/08/03 06:19:59  dfs
Added multiple channel reads and cursor reads

Revision 1.1  2008/08/02 08:53:58  dfs
Initial working rev

*/
 
#include "serial.h"
#include "common.h"
#include <time.h>

#define MAX_CHANNELS 7
#define CURSORS      6 /**offset into CH_LIST where cursors keyword is */
#define BUF_SIZE 8096
char buf[BUF_SIZE];

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

char *CH_LIST[MAX_CHANNELS]=\
{
	"CH1",
	"CH2",
	"REF1",
	"REF2",
	"REF3",
	"REF4",
	"CURSORS",
};
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int check_channel(char *ch)
{
	int i;
	if(NULL ==ch)
		return 1;
	for (i=0;i<MAX_CHANNELS;++i){
		if(!strcmp(ch,CH_LIST[i]))
			return 0;
	}
	return 1;
}
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
	/*printf("wrote %d bytes\n%s\n",rtn,msg); */
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
	/*printf("%s = %s\n",msg,buf);	 */
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
	/*printf("Got %d bytes\n",i); */
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
int set_channel(struct serial_port *p,char *ch)
{
	int i;
	if(NULL == ch)
		return -1;
	i=sprintf(buf,"DATA SOURCE:%s\r",ch);
	return write_string(p,buf,i);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_instrument(struct serial_port *p)	 
{
	int i;
	printf("Initializing Instrument\n");
	write_string(p,"id?\r",0);
	if(-1 == (i=read_string(p,buf,BUF_SIZE)) ){
		printf("%s:Unable to read from port on id\n",__func__);
		return -1;
	}
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(buf,"TEK/2440")){
		printf("Unable to find 'TEK/2440' in id string '%s'\n",buf);
		return -1;
	}
	return write_string(p,"DATA ENCDG:ASCI\r",0);
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
/** read the cursors data.
To get the cursor info, just type
cursor?
The return string is something like:
CURSOR FUNCTION:VOLTS,TARGET:CH1,UNITS:TIME:BASE,UNITS:SLOPE:BASE,UNITS:VOLTS:BA
SE,REFVOLTS:UNITS:V,REFVOLTS:VALUE:1.0000,REFSLOPE:XUNIT:SEC,REFSLOPE:YUNIT:V,RE
FSLOPE:VALUE:1.0000,REFTIME:UNITS:SEC,REFTIME:VALUE:1.0000,XPOS:ONE:3.00,XPOS:TW
O:-3.00,YPOS:ONE:2.47,YPOS:TWO:1.38,TPOS:ONE:1.24000E+2,TPOS:TWO:1.95500E+2,MODE
:DELTA,SELECT:TWO
Then look for function (VOLTS or?) target (CH1)
Send the command to read target: 
ch1?
Data looks like:
CH1 VOLTS:5E-1,VARIABLE:0,POSITION:-3.15,COUPLING:DC,FIFTY:OFF,INVERT:OFF

Then you take the YPOS:ONE and YPOS:TWO and subtract, then multiply by the CH1 VOLTS: value.
This example gives you 545 mV.

ATRIGGER data:
ATRIGGER MODE:SGLSEQ,SOURCE:CH1,COUPLING:DC,LOGSRC:OFF,LEVEL:2.04,SLOPE:PLUS,POSITION:8,HOLDOFF:0,ABSELECT:A
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int read_cursors(struct serial_port *p,int fd)
{
	char *t,lbuf[100], *function, *trigsrc;
	float one,two,volts, diff;
	int i;
	
	printf("Reading Cursor\n");
	write_string(p,"CURSOR?\r",0);
	read_string(p,buf,BUF_SIZE);
	t=get_string("TARGET",buf);
	printf("Target=%s\n",t);
	one=get_value("YPOS:ONE",buf);
	two=get_value("YPOS:TWO",buf);
	printf("Target=%s, one=%E, two=%E\n",t,one,two);
	function=get_string("FUNCTION",buf);
	
	/*find out trigger channel*/
	i=sprintf(lbuf,"ATRIGGER?\r");
	write_string(p,lbuf,i);
	read_string(p,buf,BUF_SIZE);
	trigsrc=get_string("SOURCE",buf);
	/**get channel for cursor ref  */
  i=sprintf(lbuf,"%s?\r",t);
	write_string(p,lbuf,i);
	read_string(p,buf,BUF_SIZE);
	volts=get_value(function,buf);
	diff=one-two;
	if(diff <0){
		diff *=-1;
		i=1; /**swap one-two  */
	}else
		i=0;
		
	diff*=volts;
	one *=volts;
	two *=volts;
	printf("volts=%E, diff=%E\n",volts,diff);
	i=sprintf(buf,"CURSOR:%s %F %F %F\nTRIGGER:%s\n",function,i?two:one,i?one:two,diff,trigsrc);
	/*write(fd,buf,i); written out by loop*/
	
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
	" -c n Use channel n for data (CH1). CH1, CH2, REF1-4, and CURSORS\n"
	"    The -c option can be used multiple times. In this case, the fname\n"
	"    is a base name, and the filename will have a .1 or .2, etc appended\n"
	" -o fname put output to file called fname\n"
	" -p path set path to serial port name\n"
	" -r read cursors and place in ofname.cursor\n"
	
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
	char *name, *ofname, *channel[MAX_CHANNELS], *lbuf;
	int i, c,inst_addr, rtn, ofd, ch_idx;
	name="/dev/ttyUSB0";
	inst_addr=2;
	channel[0]="CH1";
	for (c=1;c<MAX_CHANNELS;++c){
		channel[c] = NULL;
	}
	ch_idx=0;
	rtn=1;
	ofname=NULL;
	while( -1 != (c = getopt(argc, argv, "a:c:ho:p:")) ) {
		switch(c){
			case 'a':
				inst_addr=atoi(optarg);
				break;
			case 'c':
				if(ch_idx>=5){
					printf("Too many -c options. Max of %d allowed.\n",MAX_CHANNELS);
				}
				channel[ch_idx++]=strdup(optarg);
				break;
			case 'h':
				usage();
				return 1;
			case 'o':
				ofname=strdup(optarg);
				break;
			case 'p':
				name=strdup(optarg);
				break;
			default:
				usage();
				return 1;
		}
	}
	if(NULL == ofname){
		ofd=1;
	}
		
	if(0 == ch_idx)	/**Use default, and one channel  */
		++ch_idx;	
	else {
		for (c=0;c<ch_idx;++c){
			if(check_channel(channel[c])){
				printf("Channel %s is invalid\n",channel[c]);
				return 1;
			}
		}
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
	
	if(-1 == init_instrument(p)){
		printf("Unable to initialize instrument\n");
		goto closem;
	}
	if(NULL != ofname){
		/**find max strlen of channel name.  */
		for (i=c=0;c<ch_idx;++c){
			int x;
			x=0;
			if(NULL != channel[c])
				x=strlen(channel[c]);
			if(x>i)
				i=x;
		}
		i+=5;
		/**allocate buffer for all fnames.  */
		if(NULL == (lbuf=malloc(strlen(ofname)+i)) ){
			printf("Out of mem for lbuf alloc\n");
			goto closem;
		}
	}else
		lbuf=NULL;

	/**channel loop. Open file, dump data, close file, repeat  */
	for (c=0;c<ch_idx && NULL != channel[c];++c){
		if(NULL != ofname && NULL != lbuf ){ /**we have valid filename & channel, open  */
			sprintf(lbuf,"%s.%s",ofname,channel[c]);
			if(0>(ofd=open(lbuf,O_RDWR|O_CREAT,S_IROTH|S_IRGRP|S_IWUSR|S_IRUSR))) {
				printf("Unable to open '%s' for writing\n",lbuf);
				goto closem;
			}
		}	
		if(!strcmp(CH_LIST[CURSORS],channel[c])) {
			if(-1 == (i=read_cursors(p,ofd)) )
				goto closem;
		} else {
			set_channel(p,channel[c]);
			printf("Reading Channel %s\n",channel[c]);
			if(-1 == (i=get_data(p)) ){
				printf("Unable to get waveform??\n");
				goto closem;
			}	
		}
		
		write(ofd,buf,i);	
		if(ofd>2){
			close(ofd);
			ofd=1;
		}
	}
	rtn=0;
closem:
	if(ofd>1)
		close(ofd);
	close_serial_port(p);
	return rtn;
}
