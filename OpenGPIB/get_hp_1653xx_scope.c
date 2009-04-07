/** \file ******************************************************************
\n\b File:        get_hp_16530_waveform.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  6:43 am
\n\b Description: Get waveforms from the HP 16530 timebase card.
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.2  2009-04-06 20:57:26  dfs
Major re-write for new gpib API

Revision 1.1  2008/10/06 12:44:11  dfs
Initial working revision

*/

#include "common.h"
#include "gpib.h"
#include <ctype.h>
/**for open...  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/** 
The channels correspond to Slot,channel number, so Slot D, channel 1 is D1.
The first channel is the lowest slot, so if it starts with C1, the channels start at C1
and go up. D1 is channel 3.

voltage=(dataval - yreference)*yincrement	+yorigin
time=(dataptno - xreference) *xicrement	+xorigin
Trigger is datapoint closest to time 0.

*idn?
++read
HEWLETT PACKARD,16500C,0,REV 01.00
The cardcage command returns what is in each slot, starting at A
:cardcage?
++read
34,-1,12,12,11,1,0,5,5,5
12 is the o-scope, 11 is the timebase.
select sets the module to talk to: 0=intermodule, 1-5 selects A-E
SELECT 5

channels are 1, 2, 3, etc.
:WAVeform:SOURce CHANnel1
FULL gives 4096 points, window gives just what is in window.
:WAVeform:RECord FULL|WINDow

Get the preable:
format,type,points,count,Xincrement,Xorigin,Xreference,Yincrement,Yorigin,Yreference
:WAVEFORM:SOURCE CHANNEL1;PRE?
++read
0,1,4096,1,+2.00000E-07,-8.25190E-05,0,+3.66211E-04,+2.50000E+00,8192

All scope commands should be preceeded by :WAVEFORM:SOURCE CHANNELx;CMD

This gives the trigger location...This is the xorigin as relates the trigger.
:WAVEFORM:XORIGIN?
++read
-8.25190E-05

:WAVEFORM:PREAMBLE 0,1,4096,1,  +4.00000E-10,-5.07444E-06,0,  +4.88281E-05,+2.60000E+00,8192
:WAVEFORM:XORIGIN -5.07444E-06

:WAVEFORM:RECORD FULL
:WAVEFORM:RECORD WINDOW
:WAVEFORM:FORMAT ASCII

:WAVEFORM:SOURCE CHANNEL3;PRE?
:WAVEFORM:PREAMBLE 0,1,500,1,+4.00000E-10,-1.01937E-07,0,+9.76562E-05,+1.10000E-01,8192

:WAVEFORM:SOURCE CHANNEL3;DATA?



:WAVEFORM:PREAMBLE 0,1,500,1,+1.00000E-09,-2.51937E-07,0,+9.76562E-05,+1.10000E-01,8192


:MEASURE:SOURCE CHANNEL3;VPP?
:WAVEFORM:SOURCE CHANNEL1;PRE?
:WAVEFORM:PREAMBLE 0,1,500,1,+1.00000E-09,-2.51802E-07,0,+4.88281E-04,+2.80000E+00,8192
*/

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_instrument(struct gpib *g)	 
{
	int i,s;
	int slots[5], osc;
	printf("Initializing Instrument\n");
	/*write_string(g,"*CLS"); */
	if(0 == write_get_data(g,"*IDN?"))
		return -1;
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(g->buf,"HEWLETT PACKARD,16500C")){
		printf("Unable to find 'HEWLETT PACKARD,16500C' in id string '%s'\n",g->buf);
		return -1;
	}
	if(0 == write_get_data(g,":CARDCAGE?"))
		return -1;
	for (s=0;g->buf[s];++s)
		if(isdigit(g->buf[s]))
			break;
	for (i=0,osc=-1;i<5;++i){
		slots[i]=(int)get_value_col(i,&g->buf[s]);
		printf("Slot %c=%d\n",'A'+i,slots[i]);
		if(11 == slots[i])
			osc=i;
	}
	if(-1 == osc){
		printf("Unable to find timebase card\n");
		return -1;
	}
	printf("Found Timbase in slot %c\n",'A'+osc);
	
	write_get_data(g,":SELECT?");
	/*sleep(5); */
	if(osc+1 == (g->buf[0]-0x30)){
		printf("Timebase %s already selected\n",g->buf);
	}	else{
		sprintf(g->buf,":SELECT %d",osc+1);
		write_string(g,g->buf);	
	}
	write_get_data(g,":SELECT?");
	printf("Selected Card %s to talk to.\n",g->buf);
	i=write_string(g,":WAVEFORM:FORMAT ASCII");
	return i;
/*34,-1,12,12,11,1,0,5,5,5 */
}

/***************************************************************************/
/** returns the value of the digits at the end. 0 is invalid channel number.
\n\b Arguments:
\n\b Returns: 0 on error, channel no on success.
****************************************************************************/
int check_channel(char *ch)
{
	int i,b;
	if(NULL ==ch)
		return 0;
	for (b=-1,i=0;ch[i];++i){
		if(isdigit(ch[i])){
			b=i;
			break;
		}
	}
	if( -1 == b ){
		printf("Channel '%s' is invalid\n",ch);
		return 0;
	}
	return atoi(&ch[b]);
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_preamble(struct gpib *g,char *ch)
{
	int i;
	if(NULL == ch)
		return -1;
	if(0 == (i=check_channel(ch)))
		return -1;
/*	printf("GetPre %d\n",i);	 */
	i=sprintf(g->buf,":WAV:SOUR CHAN%d;PRE?",i);
	return write_get_data(g,g->buf);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_data(struct gpib *g, char *ch)
{
	int i;
	if(NULL == ch)
		return -1;
	if(0 == (i=check_channel(ch)))
		return -1;
/*	printf("GetData %d\n",i); */
	sprintf(g->buf,":WAV:SOUR CHAN%d;DATA?",i);
	i=write_get_data(g,g->buf);	
	return i;
	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("get_hp_16530_waveform <options>\n"
	" -a addr set instrument address to addr (7)\n"
	" -c n Use channel n for data (ch1). ch1-ch?\n"
	"    The -c option can be used multiple times. In this case, the fname\n"
	"    is a base name, and the filename will have a .ch1 or .ch2, etc appended\n"
	" -o fname put output to file called fname\n"
	" -p path set path to serial port name\n"
	"\n Channels are set up such that for C1 C2 D1 D2, C1= ch1, C2=ch2, D1=ch3, and D2=ch4\n"
	" So if you want C1 use ch1 as the -c option.\n"
	
	"");
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct gpib *g;
	FILE *ofd;
	char *name, *ofname, *channel[MAX_CHANNELS], *lbuf;
	int i, c,inst_addr, rtn, ch_idx;
	name="/dev/ttyUSB0";
	inst_addr=7;
	channel[0]="ch1";
	for (c=1;c<MAX_CHANNELS;++c){
		channel[c] = NULL;
	}
	ch_idx=0;
	rtn=1;
	ofname=NULL;
	ofd=NULL;
	while( -1 != (c = getopt(argc, argv, "a:c:ho:p:")) ) {
		switch(c){
			case 'a':
				inst_addr=atoi(optarg);
				break;
			case 'c':
				if(ch_idx>=MAX_CHANNELS){
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
		ofd=fdopen(1,"w+");
	}
		
	if(0 == ch_idx)	/**Use default, and one channel  */
		++ch_idx;	
	else {
		for (c=0;c<ch_idx;++c){
			if(0==check_channel(channel[c])){
				printf("Channel %s is invalid\n",channel[c]);
				return 1;
			}
		}
	}
	if(NULL == (g=open_gpib(GPIB_CTL_PROLOGIXS,inst_addr,name))){
		printf("Can't open/init controller at %s. Fatal\n",name);
		return 1;
	}
	
	if(-1 == init_instrument(g)){
		printf("Unable to initialize instrument.\n");
		printf("Did you forget to set 16500C controller 'Connected To:' HPIB?\n");
		goto closem;
	}
	printf("Sleep 5\n");
	/*sleep(5); */
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
		printf("f\n");
	}else
		lbuf=NULL;
	printf("Channel loop\n");
	/**channel loop. Open file, dump data, close file, repeat  */
	for (c=0;c<ch_idx && NULL != channel[c];++c){
		if(NULL != ofname && NULL != lbuf ){ /**we have valid filename & channel, open  */
			sprintf(lbuf,"%s.%s",ofname,channel[c]);
			if(NULL == (ofd=fopen(lbuf,"w+"))) {
				printf("Unable to open '%s' for writing\n",lbuf);
				goto closem;
			}
			printf("Reading Channel %s\n",channel[c]);
			if( 0 == (i=get_preamble(g,channel[c]))){
				printf("Preable failed on %s\n",channel[c]);
				goto closem;
			}
			fwrite(g->buf,1,i,ofd);	
			if(-1 == (i=get_data(g,channel[c])) ){
				printf("Unable to get waveform??\n");
				goto closem;
			}	
			while(i>0){	/**suck out all data from cmd above  */
				fwrite(g->buf,1,i,ofd);
				i=read_string(g);
			}
		}
		
		/*fwrite(g->buf,1,i,ofd);	 */
		if(NULL != ofd){
			fclose(ofd);
			ofd=NULL;
		}
	}
	rtn=0;
closem:
	if(NULL != ofd)
		fclose(ofd);
	close_gpib(g);
	return rtn;
}
