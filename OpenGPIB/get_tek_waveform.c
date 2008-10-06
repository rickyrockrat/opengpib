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
Revision 1.10  2008/08/19 06:42:26  dfs
Moved def of CH_LIST to common.h

Revision 1.9  2008/08/19 00:49:48  dfs
Added target to dump

Revision 1.8  2008/08/18 21:18:33  dfs
Fixed min,max read of cursor

Revision 1.7  2008/08/17 04:55:43  dfs
Removed invalid -r line in usage

Revision 1.6  2008/08/12 23:03:04  dfs
Added time/period cursors, changed to lower case

Revision 1.5  2008/08/04 09:46:39  dfs
Added handling of time cursors

Revision 1.4  2008/08/03 23:31:39  dfs
Changed format of cursors file to match waveform preable

Revision 1.3  2008/08/03 22:21:40  dfs
Added trigger to cursors info

Revision 1.2  2008/08/03 06:19:59  dfs
Added multiple channel reads and cursor reads

Revision 1.1  2008/08/02 08:53:58  dfs
Initial working rev

*/
 

#include "common.h"
#include "gpib.h"
#include <time.h>
#include <ctype.h>


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
int set_channel(struct serial_port *p,char *ch)
{
	int i;
	char *c;
	if(NULL == ch)
		return -1;
	c=strdup(ch);
	for (i=0;0 != c[i];++i)
		c[i]=toupper(c[i]);
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

TIME:
CURSOR FUNCTION:TIME,TARGET:CH1,UNITS:TIME:BASE,UNITS:SLOPE:BASE,UNITS:VOLTS:BAS
E,REFVOLTS:UNITS:V,REFVOLTS:VALUE:1.0000,REFSLOPE:XUNIT:SEC,REFSLOPE:YUNIT:V,REF
SLOPE:VALUE:1.0000,REFTIME:UNITS:SEC,REFTIME:VALUE:1.0000,XPOS:ONE:3.00,XPOS:TWO
:-3.00,YPOS:ONE:2.39,YPOS:TWO:1.03,TPOS:ONE:2.56500E+2,TPOS:TWO:2.93000E+2,MODE:
DELTA,SELECT:TWO

Then look for function (VOLTS or?) target (CH1)
Send the command to read target: 
ch1?
Data looks like:
CH1 VOLTS:5E-1,VARIABLE:0,POSITION:-3.15,COUPLING:DC,FIFTY:OFF,INVERT:OFF

Then you take the YPOS:ONE and YPOS:TWO and subtract, then multiply by the CH1 VOLTS: value.
This example gives you 545 mV.

ATRIGGER data:
ATRIGGER MODE:SGLSEQ,SOURCE:CH1,COUPLING:DC,LOGSRC:OFF,LEVEL:2.04,SLOPE:PLUS,POSITION:8,HOLDOFF:0,ABSELECT:A
Todo:
	Handle 'V.T'
	Handle 'SLOPE'
	Handle 'ONE/TIME'
\n\b Arguments:
\n\b Returns:
****************************************************************************/
#define FUNC_VOLT 1
#define FUNC_TIME 2
#define FUNC_FREQ 3
int read_cursors(struct serial_port *p,int fd)
{
	char *target,lbuf[100], *function, *trigsrc;
	float min,max,volts, diff,xinc;
	int i, f;
	
	printf("Reading Cursor\n");
	write_string(p,"CURSOR?\r",0);
	read_string(p,buf,BUF_SIZE);
	
	function=get_string("FUNCTION",buf);
	if( !strcmp(function,"TIME"))
		f=FUNC_TIME;
	else if (!strcmp(function,"ONE/TIME") )
		f=FUNC_FREQ;
	else if(!strcmp(function,"VOLTS") ){
		f=FUNC_VOLT;
		min=get_value("YPOS:ONE",buf);
		max=get_value("YPOS:TWO",buf);	
		target=get_string("TARGET",buf); /**reference to  */
		
		/**get channel for cursor ref  */
	  i=sprintf(lbuf,"%s?\r",target);
		write_string(p,lbuf,i);
		read_string(p,buf,BUF_SIZE);
		/**get the volts/div  */
		volts=get_value(function,buf);
		min *=volts;
		max *=volts;		
	}else{
		printf("Don't know how to handle cursor function '%s' in\n%s\n",function, buf);
		diff=min=max=1;
		return 0;
	}
	if(FUNC_TIME ==f || FUNC_FREQ == f){
		min=get_value("TPOS:ONE",buf);
		max=get_value("TPOS:TWO",buf);
		i=sprintf(lbuf,"WFM?\r");
		write_string(p,lbuf,i);
		read_string(p,buf,BUF_SIZE);
		xinc=get_value("XINCR",buf);
		min*=xinc;
		max*=xinc;
	}
	if(min>max){
		diff=min;
		min=max;
		max=diff;
	}
	diff=max-min;
	if(FUNC_FREQ == f ){
		diff=1/diff;
	}
	
	/*find out trigger channel*/
		i=sprintf(lbuf,"ATRIGGER?\r");
		write_string(p,lbuf,i);
		read_string(p,buf,BUF_SIZE);
		trigsrc=get_string("SOURCE",buf);
	
	if(FUNC_VOLT ==f)
		printf("volts");
	else if(FUNC_TIME == f)
		printf("time");
	printf("=%E, diff=%E\n",volts,diff);
	i=sprintf(buf,"CURSOR:%s,MAX:%E,MIN:%E,DIFF:%E,TARGET:%s,TRIGGER:%s\n",function,max,min,diff,target,trigsrc);
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
	" -c n Use channel n for data (ch1). ch1, ch2, ref1-4, and cursors\n"
	"    The -c option can be used multiple times. In this case, the fname\n"
	"    is a base name, and the filename will have a .1 or .2, etc appended\n"
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
	char *name, *ofname, *channel[MAX_CHANNELS], *lbuf;
	int i, c,inst_addr, rtn, ofd, ch_idx;
	name="/dev/ttyUSB0";
	inst_addr=2;
	channel[0]="ch1";
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
