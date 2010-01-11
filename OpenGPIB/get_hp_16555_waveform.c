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
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

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
#include "hp16500.h"

#include <ctype.h>
/**for open...  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/** 

*idn?
++read
HEWLETT PACKARD,16500C,0,REV 01.00
The cardcage command returns what is in each slot, starting at A
:cardcage?
++read
34,-1,12,12,11,1,0,5,5,5
12 is the o-scope, 11 is the timebase, 34 is the logic probe.
select sets the module to talk to: 0=intermodule, 1-5 selects A-E
SELECT 1


	The data part of the data is as follows:
	Cards       Clock Pod Bytes       Data Bytes          Total Bytes Per Row
	1           4 bytes               8 bytes             12 bytes
	2           4 bytes               16 bytes            20 bytes
	3           4 bytes               24 bytes            28 bytes
	The sequence of pod data within a row is the same as shown above for the
	number of valid rows per pod. The number of valid rows per pod can be
	determined by examining bytes 253 through 256 for pod pair 3/4 of the
	master card and bytes 257 through 260 for pod pair 1/2 of the master card.
	The number of valid rows for other pod pairs is contained in bytes 213
	through 252.
	
	A one-card configuration has the following data arrangement (per row):
	<clk pod 1> <pod 4> <pod 3> <pod 2> <pod 1>
	A two-card configuration has the following data arrangement (per row):
	       <-----expansion card ------><-------master card-------->
	<clk 1><pod 4><pod 3><pod 2><pod 1><pod 4><pod 3><pod 2><pod 1>
	A three-card configuration has the following data arrangement per row:
	       <- high expander -><-lower expander -><---master card--->
	<clk 1><pod 4>. . .<pod 1><pod 4>. . .<pod 1><pod 4>. . .<pod 1>
	
	Unused pods always have data, however it is invalid and should be ignored.


 The depth of the data array is equal to the pod with the greatest number of
 rows of valid data. If a pod has fewer rows of valid data than the data array,
 unused rows will contain invalid data that should be ignored.
 The clock pod contains data mapped according to the clock designator and
 the board (see below). Unused clock lines should be ignored.
                                     exp2 exp1 mstr
      Clock Pod 1           < xxxx MLKJ MLKJ MLKJ >
 Where x = not used, mstr = master card, exp# = expander card number.
Byte Position
          591 2 bytes - Not used (clock pod 2).
          593 1 byte - MSB of clock pod 1.
          594 1 byte - LSB of clock pod 1.
          595 1 byte - MSB of data pod 4, board x.
          596 1 byte - LSB of data pod 4, board x.
          597 1 byte - MSB of data pod 3, board x.
          598 1 byte - LSB of data pod 3, board x.
          599 1 byte - MSB of data pod 2, board x.
          600 1 byte - LSB of data pod 2, board x.
          601 1 byte - MSB of data pod 1, board x.
          602 1 byte - LSB of data pod 1, board x.
            .
            .
       Byte n where n = 591 + (bytes per row × maximum number of valid rows) - 1


Time Tag Data Description
If tags are enabled for one or both analyzers, the tag data follows the
acquisition data. The first byte of the tag data is determined as follows:
  591 + (bytes per row × maximum number of valid rows)
Each row of the tag data array consists of one (single analyzer with state
tags) or two (both analyzers with state tags) eight-byte tag values per row.
When both analyzers have state tags enabled, the first tag value in a row
belongs to Machine 1 and the second tag value belongs to Machine 2.
If the tag value is a time tag, the number is an integer representing time in
picoseconds. If the tag value is a state tag, the number is an integer state
count.
The total size of the tag array is 8 or 16 bytes per row times the greatest
number of valid rows.


*/

struct block_spec {
	uint8 blockstart[2]; /**should be #8  */
	uint8 blocklen[8];	 /**decimal len of block, stored in ascii  */
}__attribute__((__packed__));	

struct section_hdr {
	char name[11]; /**the last byte is reserved, but usally 0  */
	uint8 module_id;
	uint32 block_len; /**lsb is most significant byte, i.e 0x100 is stored as 10 0, not 0 10  */
}__attribute__((__packed__));	

struct analyzer_data {
	uint32 data_mode;
	uint32 pods; /**bit 21-clock pod, bit 1-12=pod 1-12, all other bits unused.  */
	uint32 masterchip;
	uint32 maxmem;
	uint32 unused1;
	uint64 sampleperiod;
	uint32 tag_type; /**0-off,1=time, 2=state tags.  */
	uint64 trig_off;
	uint8 unused2[30];
}__attribute__((__packed__));	

struct data_preamble {
		uint32 instid;
	uint32 rev_code;
	uint32 chips;
	uint32 analyzer_id;
	struct analyzer_data a1;
	struct analyzer_data a2;
	uint8 unused1[40];
	/**number of valid rows of data... offset 173 */
	uint32 data_pod4_hi;
	uint32 data_pod3_hi;
	uint32 data_pod2_hi;
	uint32 data_pod1_hi;
	uint32 data_pod4_mid;
	uint32 data_pod3_mid;
	uint32 data_pod2_mid;
	uint32 data_pod1_mid;
	uint32 data_pod4_master;
	uint32 data_pod3_master;
	uint32 data_pod2_master;
	uint32 data_pod1_master;
	/**trigger position?? offset 261 */
	uint8 unused2[40];
	uint32 trig_pod4_hi;
	uint32 trig_pod3_hi;
	uint32 trig_pod2_hi;
	uint32 trig_pod1_hi;
	uint32 trig_pod4_mid;
	uint32 trig_pod3_mid;
	uint32 trig_pod2_mid;
	uint32 trig_pod1_mid;
	uint32 trig_pod4_master;
	uint32 trig_pod3_master;
	uint32 trig_pod2_master;
	uint32 trig_pod1_master;
	/** offset 349  */
	uint8 unused3[234];
	/**  acquision time*/
	uint16 rtc_year; /**RTC=year-1990  */
	uint16 rtc_month;
	uint8 rtc_day;
	uint8 rtc_dow;
	uint8 rtc_hour;
	uint8 rtc_min;
	uint8 rtc_sec;
}__attribute__((__packed__));	

struct hp_data_block {
	struct block_spec bs;
	struct section_hdr shdr;
	struct data_preamble preamble;
}__attribute__((__packed__));	

struct hp_config_block {
	struct block_spec bs;
	char *data;
}__attribute__((__packed__));	

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
		if(CARDTYPE_16554M == slots[i])
			osc=i;
	}
	if(-1 == osc){
		printf("Unable to find Sampling card\n");
		return -1;
	}
	printf("Found Sampling in slot %c\n",'A'+osc);
	
	write_get_data(g,":SELECT?");

	if(osc+1 == (g->buf[0]-0x30)){
		printf("Timebase %s already selected\n",g->buf);
	}	else{
		sprintf(g->buf,":SELECT %d",osc+1);
		write_string(g,g->buf);	
	}
	
/*	i=write_get_data(g,":?"); */
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
long int get_datsize(char *hdr)
{
	long int len;
	char *s;
	
	if(NULL==(s=strstr(hdr,"#8")) ){
		printf("unable to find data len\n");
		return 0;
	}
	s+=2;
	sscanf(s,"%ld",&len);
	return len;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("get_hp_16555_waveform <options>\n"
	" -a addr set instrument address to addr (7)\n"
	" -d dev set path to device name\n"
	" -g set gnuplot mode, with x,y (time volts), one point per line\n"
	" -o fname put output to file called fname\n"
	" -p set packed mode to packed (unpacked)\n"
	
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
	char *name, *ofname;
	int i, c,inst_addr, rtn, raw,xy;
	long int total,sz;
	name="/dev/ttyUSB0";
	inst_addr=7;
	rtn=1;
	ofname=NULL;
	ofd=NULL;
	raw=0;
	xy=0;
	while( -1 != (c = getopt(argc, argv, "a:d:gho:p")) ) {
		switch(c){
			case 'a':
				inst_addr=atoi(optarg);
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

			case 'p':
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
			if(NULL == (ofd=fopen(ofname,"w+"))) {
				printf("Unable to open '%s' for writing\n",ofname);
				goto closem;
			}
	}	
	printf("Retreiving Setup\n");
	sprintf(g->buf,":SYSTEM:SETUP?");
	i=write_string(g,g->buf);	
	total=0;
	while(i){
		i=read_raw(g);
		if(0 == total){
			sz=get_datsize(g->buf);
			printf("Len %ld Got %d ",sz,i);
		}else
			printf("%ld ",sz-total);
		fflush(NULL);
		fwrite(g->buf,1,i,ofd);	
		total+=i;
	}
	i=sprintf(g->buf,"\nDATA\n");
	fwrite(g->buf,1,i,ofd);	
	printf("\nWrote %ld config + %d datahdr. Retreiving Data\n",total,i);
	goto closem;
	sprintf(g->buf,":DBLOCK %s;:SYSTEM:DATA?",raw?"PACKED":"UNPACKED");
	i=write_string(g,g->buf);	
	usleep(10000000);
	total=0;
	while(i){
		i=read_raw(g);
		if(0 == total){
			sz=get_datsize(g->buf);
			printf("Len %ld Got %d ",sz,i);
		}else
			printf("%ld ",sz-total);
		fflush(NULL);
		fwrite(g->buf,1,i,ofd);	
		total+=i;
	}
	printf("Done. Wrote %ld bytes\n",total);
		
	rtn=0;
closem:
	if(NULL != ofd)
		fclose(ofd);
	close_gpib(g);
	return rtn;
}
