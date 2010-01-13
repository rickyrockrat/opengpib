/** \file ******************************************************************
\n\b File:        get_hp_16530_waveform.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  6:43 am
\n\b Description: Get waveforms from the HP 16530 timebase card.
*/ /************************************************************************
Change Log: \n
 This file is part of OpenGPIB.

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

#include "common.h"
#include "gpib.h"

#include <ctype.h>

/**for open...  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HP16500_COM_SYMBOLS 1
#include "hp16500.h"
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
:WAV:REC FULL
:WAVEFORM:RECORD WINDOW
:WAVEFORM:FORMAT ASCII

:WAVEFORM:SOURCE CHANNEL3;PRE?
:WAVEFORM:PREAMBLE 0,1,500,1,+4.00000E-10,-1.01937E-07,0,+9.76562E-05,+1.10000E-01,8192

:WAVEFORM:SOURCE CHANNEL3;DATA?


									FMT,TYPE,Points,Count,Xinc,Xorigin,Xref,Yinc,Yorigin,Yref
:WAVEFORM:PREAMBLE 0,1,500,1,+1.00000E-09,-2.51937E-07,0,+9.76562E-05,+1.10000E-01,8192


:MEASURE:SOURCE CHANNEL3;VPP?
:WAVEFORM:SOURCE CHANNEL1;PRE?
:WAVEFORM:PREAMBLE 0,1,500,1,+1.00000E-09,-2.51802E-07,0,+4.88281E-04,+2.80000E+00,8192

The ASC data format is same as WORD, which is as follows: 
TYPE						MSB									LSB
NORMAL NU|OV|M5|M4|M3|M2|M1|M0 	 00|00|00|00|00|00|00|00
AVE		 NU|OV|M5|M4|M3|M2|M1|M0	 A7|A6|A5|A4|A3|A2|A1|A0
BYTE is 
	NU|OV|M5|M4|M3|M2|M1|M0
ASC is 0
BYTE is 1
WORD is 2
	
Data conversion:
Voltage=(Value-Yreference)*Yinc +Yorigin
Time= (point# -Xref)*Xinc + Xorigin

*/

/***************************************************************************/
/** FMT,TYPE,Points,Count,Xinc,Xorigin,Xref,Yinc,Yorigin,Yref.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int parse_preamble(struct gpib *g, struct hp_scope_preamble *h)
{
	h->fmt=get_value_col(0,g->buf);
	h->points=get_value_col(2,g->buf);
	h->xinc=get_value_col(4,g->buf);
	h->xorg=get_value_col(5,g->buf);
	h->xref=get_value_col(6,g->buf);
	h->yinc=get_value_col(7,g->buf);
	h->yorg=get_value_col(8,g->buf);
	h->yref=get_value_col(9,g->buf);
	return 0;
}
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
	g->control(g,CTL_SET_TIMEOUT,500);
	while(read_string(g));
	g->control(g,CTL_SET_TIMEOUT,50000);
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
		if(CARDTYPE_16530A == slots[i])
			osc=i;
	}
	if(-1 == osc){
		printf("Unable to find timebase card\n");
		return -1;
	}
	printf("Found Timbase in slot %c\n",'A'+osc);
	
	write_get_data(g,":SELECT?");

	if(osc+1 == (g->buf[0]-0x30)){
		printf("Timebase %s already selected\n",g->buf);
	}	else{
		sprintf(g->buf,":SELECT %d",osc+1);
		write_string(g,g->buf);	
	}
	
	i=write_get_data(g,":WAV:REC FULL;:WAV:FORM BYTE;:SELECT?");
	printf("Selected Card %s to talk to.\n",g->buf);

	return i;
/*34,-1,12,12,11,1,0,5,5,5 */
	/** :WAV:REC FULL;:WAV:FORM ASC;:SELECT? */
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
int get_preamble(struct gpib *g,char *ch, struct hp_scope_preamble *h)
{
	int i;
	if(NULL == ch)
		return -1;
	if(0 == (i=check_channel(ch)))
		return -1;
	/*printf("GetPre %d\n",i);	  */
	sprintf(g->buf,":WAV:SOUR CHAN%d;PRE?",i);
	if( write_get_data(g,g->buf) <=0){
		printf("No data from preamble (%d)\n",i);
		return 0; 
	}
		
	parse_preamble(g,h);
	return 1;
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
	" -d dev set path to device name\n"
	" -g set gnuplot mode, with x,y (time volts), one point per line\n"
	" -o fname put output to file called fname\n"

	" -r set raw mode (don't convert data to volts)\n"
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
	struct hp_scope_preamble h;
	FILE *ofd;
	char *name, *ofname, *channel[MAX_CHANNELS], *lbuf;
	int i, c,inst_addr, rtn, ch_idx, raw,xy;
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
	raw=xy=0;
	while( -1 != (c = getopt(argc, argv, "a:d:gc:ho:r")) ) {
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
			case 'd':
				name=strdup(optarg);
				break;
			case 'h':
				usage();
				return 1;
			case 'g':
				xy=1;
				break;
			case 'o':
				ofname=strdup(optarg);
				break;

			case 'r':
				raw=1;
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
	if(NULL == (g=open_gpib(GPIB_CTL_PROLOGIXS,inst_addr,name,-1))){
		printf("Can't open/init controller at %s. Fatal\n",name);
		return 1;
	}
	
	if(-1 == init_instrument(g)){
		printf("Unable to initialize instrument.\n");
		printf("Did you forget to set 16500C controller 'Connected To:' HPIB?\n");
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
		printf("f\n");
	}else
		lbuf=NULL;
	printf("Channel loop\n");
	/**channel loop. Open file, dump data, close file, repeat  */
	for (c=0;c<ch_idx && NULL != channel[c];++c){
		int point;
		if(NULL != ofname && NULL != lbuf ){ /**we have valid filename & channel, open  */
			sprintf(lbuf,"%s.%s",ofname,channel[c]);
			if(NULL == (ofd=fopen(lbuf,"w+"))) {
				printf("Unable to open '%s' for writing\n",lbuf);
				goto closem;
			}
			printf("Reading Channel %s\n",channel[c]);
			if( 0 >= (i=get_preamble(g,channel[c],&h))){
				printf("Preable failed on %s\n",channel[c]);
				goto closem;
			}
	/** Voltage=(Value-Yreference)*Yinc +Yorigin
			Time= (point# -Xref)*Xinc + Xorigin */
			fwrite(g->buf,1,i,ofd);	
			if(-1 == (i=get_data(g,channel[c])) ){
				printf("Unable to get waveform??\n");
				goto closem;
			}	
			point=0;
			h.xinc*=1000000000;
			while(i>0){	/**suck out all data from cmd above  */
				int x;
				for (x=0;x<i;++x){
					float volts;
					float time;
					time=h.xinc*(float)point;
					if(0==point)
						printf("%d %f %f \n",point,h.xinc,time); 
					volts=(g->buf[x]-h.yref)*h.yinc + h.yorg;
					if(xy){
						if(raw)
							fprintf(ofd,"%f %d\n",time,g->buf[x]);
						else
							fprintf(ofd,"%f %f\n",time,volts);	
					}	else {
						if(raw)
							fprintf(ofd,"%d,",g->buf[x]);
						else
							fprintf(ofd,"%f,",volts);	
					}
					++point;
				}
				/*fwrite(g->buf,1,i,ofd); */
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
