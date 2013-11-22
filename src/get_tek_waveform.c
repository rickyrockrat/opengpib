/** \file ******************************************************************
\n\b File:        get_tek_waveform.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008 12:45 pm
\n\b Description: open port to GPIB device, init it, grab initial data from 
scope, then dump that into a file.
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
 

#include "open-gpib.h"

#define FUNC_VOLT 1
#define FUNC_TIME 2
#define FUNC_FREQ 3

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
int set_channel(struct open_gpib *g,char *ch)
{
	int i;
	char *c;
	if(NULL == ch)
		return -1;
	c=strdup(ch);
	for (i=0;0 != c[i];++i)
		c[i]=toupper(c[i]);
	sprintf(g->buf,"DATA SOURCE:%s\r",ch);
	return write_string(g,g->buf);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_instrument(struct open_gpib *g)	 
{
	int i;
	printf("Initializing Instrument\n");
	g->ctl->funcs.og_control(g->ctl,CTL_SET_TIMEOUT,1000);
	while(read_string(g));
	g->ctl->funcs.og_control(g->ctl,CTL_SET_TIMEOUT,500000);
	write_string(g,"id?");
	if(-1 == (i=read_string(g)) ){
		printf("%s:Unable to read from port on id\n",__func__);
		return -1;
	}
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(g->buf,"TEK/2440")){
		printf("Unable to find 'TEK/2440' in id string '%s'\n",g->buf);
		return -1;
	}
	printf("Talking to addr %d: '%s'\n",g->addr,g->buf);
	return write_string(g,"DATA ENCDG:ASCI");
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_data(struct open_gpib *g)
{
	int i;
	write_string(g,"WAV?");
	i=read_string(g);
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

int read_cursors(struct open_gpib *g)
{
	char *target,lbuf[100], *function, *trigsrc;
	float min,max,volts, diff,xinc;
	int i, f;
	
	printf("Reading Cursor\n");
	write_string(g,"CURSOR?");
	read_string(g);
	
	function=og_get_string("FUNCTION",g->buf);
	if( !strcmp(function,"TIME"))
		f=FUNC_TIME;
	else if (!strcmp(function,"ONE/TIME") )
		f=FUNC_FREQ;
	else if(!strcmp(function,"VOLTS") ){
		f=FUNC_VOLT;
		min=og_get_value("YPOS:ONE",g->buf);
		max=og_get_value("YPOS:TWO",g->buf);	
		target=og_get_string("TARGET",g->buf); /**reference to  */
		
		/**get channel for cursor ref  */
	  i=sprintf(lbuf,"%s?\r",target);
		write_string(g,lbuf);
		read_string(g);
		/**get the volts/div  */
		volts=og_get_value(function,g->buf);
		min *=volts;
		max *=volts;		
	}else{
		printf("Don't know how to handle cursor function '%s' in\n%s\n",function, g->buf);
		diff=min=max=1;
		return 0;
	}
	if(FUNC_TIME ==f || FUNC_FREQ == f){
		min=og_get_value("TPOS:ONE",g->buf);
		max=og_get_value("TPOS:TWO",g->buf);
		i=sprintf(lbuf,"WFM?\r");
		write_string(g,lbuf);
		read_string(g);
		xinc=og_get_value("XINCR",g->buf);
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
		write_string(g,lbuf);
		read_string(g);
		trigsrc=og_get_string("SOURCE",g->buf);
	
	if(FUNC_VOLT ==f)
		printf("volts");
	else if(FUNC_TIME == f)
		printf("time");
	printf("=%E, diff=%E\n",volts,diff);
	i=sprintf(g->buf,"CURSOR:%s,MAX:%E,MIN:%E,DIFF:%E,TARGET:%s,TRIGGER:%s\n",function,max,min,diff,target,trigsrc);
	
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
	" -d path set path to device name\n"
	" -o fname put output to file called fname\n"
	
	"");
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct open_gpib *g;
	char *name, *ofname, *channel[MAX_CHANNELS], *lbuf;
	FILE *ofd;
	int i, c,inst_addr, rtn, ch_idx;
	name="/dev/ttyUSB0";
	inst_addr=2;
	channel[0]="ch1";
	for (c=1;c<MAX_CHANNELS;++c){
		channel[c] = NULL;
	}
	ch_idx=0;
	rtn=1;
	ofname=NULL;
	ofd=NULL;
	while( -1 != (c = getopt(argc, argv, "a:c:d:ho:")) ) {
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
			case 'd':
				name=strdup(optarg);
				break;
			case 'h':
				usage();
				return 1;
			case 'o':
				ofname=strdup(optarg);
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
			if(check_channel(channel[c])){
				printf("Channel %s is invalid\n",channel[c]);
				return 1;
			}
		}
	}
	if(NULL == (g=open_gpib(GPIB_CTL_PROLOGIXS,inst_addr,name,-1))){
		printf("Can't open %s. Fatal\n",name);
		return 1;
	}
	
	if(-1 == init_instrument(g)){
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
			if(NULL == (ofd=fopen(lbuf,"w+"))) {
				printf("Unable to open '%s' for writing\n",lbuf);
				goto closem;
			}
		}	
		if(!strcmp(CH_LIST[CURSORS],channel[c])) {
			if(-1 == (i=read_cursors(g)) )
				goto closem;
		} else {
			set_channel(g,channel[c]);
			printf("Reading Channel %s\n",channel[c]);
			if(-1 == (i=get_data(g)) ){
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
