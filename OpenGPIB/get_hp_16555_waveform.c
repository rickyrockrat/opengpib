/** \file ******************************************************************
\n\b File:        get_hp_16555_waveform.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  6:43 am
\n\b Description: Get waveforms from the HP 1655x logic analizer cards.
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
#include "hp16500.h"
#ifdef LA2VCD_LIB
#include "la2vcd.h"
#endif
#include <ctype.h>
/**for open...  */
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

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_instrument(struct gpib *g)	 
{
	int i,s;
	int slots[5], osc;
	fprintf(stderr,"Initializing Instrument\n");
	g->control(g,CTL_SET_TIMEOUT,500);
	while(read_string(g));
	g->control(g,CTL_SET_TIMEOUT,50000);
	/*write_string(g,"*CLS"); */
	if(0 == write_get_data(g,"*IDN?"))
		return -1;
	/*fprintf(stderr,"Got %d bytes\n",i); */
	if(NULL == strstr(g->buf,"HEWLETT PACKARD,16500C")){
		fprintf(stderr,"Unable to find 'HEWLETT PACKARD,16500C' in id string '%s'\n",g->buf);
		g->control(g,CTL_SEND_CLR,0);
		return -1;
	}
	if(0 == write_get_data(g,":CARDCAGE?"))
		return -1;
	for (s=0;g->buf[s];++s)
		if(isdigit(g->buf[s]))
			break;
	for (i=0,osc=-1;i<5;++i){
		slots[i]=(int)get_value_col(i,&g->buf[s]);
		fprintf(stderr,"Slot %c=%d\n",'A'+i,slots[i]);
		if(CARDTYPE_16554M == slots[i])
			osc=i;
	}
	if(-1 == osc){
		fprintf(stderr,"Unable to find Sampling card\n");
		return -1;
	}
	fprintf(stderr,"Found Sampling in slot %c\n",'A'+osc);
	
	write_get_data(g,":SELECT?");

	if(osc+1 == (g->buf[0]-0x30)){
		fprintf(stderr,"Timebase %s already selected\n",g->buf);
	}	else{
		sprintf(g->buf,":SELECT %d",osc+1);
		write_string(g,g->buf);	
	}
	write_get_data(g,":SELECT?");
/*	i=write_get_data(g,":?"); */
	fprintf(stderr,"Selected Card %s to talk to.\n",g->buf);

	return i;
/*34,-1,12,12,11,1,0,5,5,5 */
	/** :WAV:REC FULL;:WAV:FORM ASC;:SELECT? */
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
		fprintf(stderr,"unable to find data len\n");
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
	fprintf(stderr,"Version %s: get_hp_16555_waveform <options>\n"
	" -a addr set instrument address to addr (7)\n"
	" -c fname put config data to fname\n"
	" -d dev set path to device name\n"
	" -m meth set method to meth (hpip)\n"
	" -o fname put config lfDATAlf data to file called fname\n"
	" -p set packed mode to packed for config only (unpacked)\n"
	" -t fname put trace(data) to fname\n"
#ifdef LA2VCD_LIB
	" -v basename put vcd data to basename.vcd. Creates .dat and .cfg\n"
#endif	
	
	"Make sure the 16550C Controller is connected to LAN for ip \nor to HPIB for prologix\n"
	"\nHere are the supported controllers\n\n"
	"",TOSTRING(VERSION));
	show_gpib_supported_controllers();
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct gpib *g;
	FILE *ofd,*cfd,*tfd,*vfd;
	char *name, *ofname;
	char *tname,*cname,*vname;
	int i, c,inst_addr, rtn, raw,dtype;
	long int total,sz;
	inst_addr=7;
	rtn=1;
	name=ofname=tname=cname=vname=NULL;
	vfd=tfd=cfd=ofd=NULL;
	raw=0;
	dtype=GPIB_CTL_HP16500C;
	while( -1 != (c = getopt(argc, argv, "a:c:d:hmo:pt:v:")) ) {
		switch(c){
			case 'a':
				inst_addr=atoi(optarg);
				break;
			case 'c':
				if(NULL != vname){
					fprintf(stderr,"-v and -c are mutually exclusive.\n");
					usage();
					return 1;
				}
				cname=strdup(optarg);
				break;
			case 'd':
				name=strdup(optarg);
				break;
			case 'h':
				usage();
				return 1;
			case 'm':
				if(-1 == (dtype=gpib_option_to_type(optarg)))	{
					fprintf(stderr,"'%s' method not supported\n",optarg);
					return -1;
				}
				break;
			case 'o':
				ofname=strdup(optarg);
				break;
			case 'p':
				raw=1;
				break;
			case 't':
				if(NULL != vname){
					fprintf(stderr,"-v and -t are mutually exclusive.\n");
					usage();
					return 1;
				}
				tname=strdup(optarg);
				break;
			case 'v':
#ifdef LA2VCD_LIB 
				if(NULL != tname || NULL != cname){
					fprintf(stderr,"-v and -c/-tare mutually exclusive.\n");
					usage();
					return 1;
				}				
				if(NULL ==(vname=malloc( (strlen(optarg)+5)*3 ))){
					fprintf(stderr,"Unable to malloc filename space for '%s'\n",optarg);
					return 1;
				}
				i=1+sprintf(vname,"%s.vcd",optarg);
				
				cname=&vname[i];
				i+=1+sprintf(cname,"%s.cfg",optarg);
				tname=&vname[i];
				sprintf(tname,"%s.dat",optarg);
#else
				fprintf(stderr,"-v not supported. Re-build with LA2VCD_LIB=/path/to/lib\n");
				return 1;
#endif
				break;
			default:
				usage();
				return 1;
		}
	}
	if(NULL != ofname && (NULL != tname || NULL != cname)){
		fprintf(stderr,"-o and -t/-c are mutually exclusive\n");
		usage();
		return 1;
	}
	if(NULL == ofname && NULL == tname && NULL == cname){
		fprintf(stderr,"must specify -t/-c or -o\n");
		usage();
		return 1;
	} 
	if(NULL != vname && (NULL == cname || NULL == tname)){
		fprintf(stderr,"Must specify -c and -t with -v\n");
		usage();
		return 1;
	}
	if(NULL == (g=open_gpib(dtype,inst_addr,name,1048576))){
		fprintf(stderr,"Can't open/init controller at %s. Fatal\n",name);
		return 1;
	}
	
	if(-1 == init_instrument(g)){
		fprintf(stderr,"Unable to initialize instrument.\n");
		fprintf(stderr,"Did you forget to set 16500C controller 'Connected To:' HPIB?\n");
		goto closem;
	}
	if(NULL != ofname){
			if(NULL == (ofd=fopen(ofname,"w+"))) {
				fprintf(stderr,"Unable to open '%s' for writing\n",ofname);
				goto closem;
			}
	}	else{
		if(NULL !=cname){
			if(NULL == (cfd=fopen(cname,"w+"))) {
				fprintf(stderr,"Unable to open '%s' for writing\n",cname);
				goto closem;
			}
		}
		if(NULL !=tname){
			if(NULL == (tfd=fopen(tname,"w+"))) {
				fprintf(stderr,"Unable to open '%s' for writing\n",tname);
				goto closem;
			}
		}
	}
	if(cfd>0 ||ofd>0){ /**grab config  */
		fprintf(stderr,"Retreiving Setup\n");
		sprintf(g->buf,":SYSTEM:SETUP?");
		i=write_string(g,g->buf);	
		usleep(100000);
		total=0;
		while(i){
			i=read_raw(g);
			if(0 == total){
				sz=get_datsize(g->buf);
				fprintf(stderr,"Len %ld Got %d ",sz,i);
			}else
				fprintf(stderr,"%ld ",sz-total);
			fflush(NULL);
			fwrite(g->buf,1,i,ofd?ofd:cfd);	
			total+=i;
		}	
		if(ofd>0){
			i=sprintf(g->buf,"\nDATA\n");
			fwrite(g->buf,1,i,ofd);		
		}
		fprintf(stderr,"\nWrote %ld config + %d datahdr.\n",total,i);
	}
	usleep(1000000);
	
	if(tfd>0 ||ofd>0){ /**grab data  */
		int nodata=0;
		fprintf(stderr,"Retreiving Data\n");
	/*	goto closem; */
		sprintf(g->buf,":DBLOCK %s;:SYSTEM:DATA?",raw?"PACKED":"UNPACKED");
		i=write_string(g,g->buf);	
		usleep(1000000);
		total=0;
		while(i){
			uint32 off=0;
			i=read_raw(g);
			if(0 == total){
				if(g->buf[0]!='#'){
					for (off=0;off<i && g->buf[off] != '#';++off);
					if(i == off){
						fprintf(stderr,"discarding first buffer of %d\n",i);
						i=0;
						++nodata;
					}else if(off){
						fprintf(stderr,"Discarding first %d byte(s)\n",off);
						i-=off;
					}
				}
				if(i)	{
					sz=get_datsize(g->buf);
					fprintf(stderr,"Len %ld Got %d ",sz,i);	
				}
				
			}else
				fprintf(stderr,"%ld ",sz-total);
			fflush(NULL);
			fwrite(g->buf,1,i,ofd?ofd:tfd);	
			total+=i;
			if(0 == i){
				++nodata;
				if(nodata>10){
					fprintf(stderr,"No Data for 10 tries\n");
					break;
				}else
					nodata=0;
					
			}
				
		}
		fprintf(stderr,"Done. Wrote %ld bytes\n",total);
		if(total!=sz+10){	/**add header size of 10  */
			fprintf(stderr,"Total does not match size. Sending CLR\n");
			g->control(g,CTL_SEND_CLR,0);
		}
			
	}	
	
	
	rtn=0;
closem:
	if(NULL != ofd)
		fclose(ofd);
	if(NULL != cfd)
		fclose(cfd);
	if(NULL != tfd)
		fclose(tfd);
	close_gpib(g);
#ifdef LA2VCD_LIB	
	if(total != sz)
		goto endvcd;
	if(NULL !=vname){ /**generate a vcd file from the config and data  */
		struct section *s;
		struct data_preamble *p;
		struct la2vcd *l;
		struct signal_data *d,*x;
		char buf[500];
		if(NULL ==(s=parse_config(cname,"CONFIG    ",JUST_LOAD) ) ){
			fprintf(stderr,"Unable to re-open '%s'\n",cname);
			goto endvcd;
		}	
		if(NULL == (p=parse_data(tname,NULL,JUST_LOAD))){
			fprintf(stderr,"Unable to re-open data file '%s'\n",tname);
			goto endvcd;
		}	
		if(NULL ==(d=show_la2vcd(p,s,JUST_LOAD))){
			fprintf(stderr,"Error loading labels\n");
			goto endvcd;
		}	
		if(NULL==(l=open_la2vcd(vname,NULL,p->a1.sampleperiod*1e-12,0,NULL==s?NULL:s->data))){ 
			fprintf(stderr,"Unable to open la2vcd lib\n");
			goto endvcd;
		}	
    if(vcd_add_file(l,NULL,16,d->bits)){
			fprintf(stderr,"Failed to add input file\n");
			goto endvcd;
		}
		fprintf(stderr,"Bits=%d\n",d->bits);
		/**Add our signal descriptions in  */
/*		vcd_add_signal (&l->first_signal,&l->last_signal, l->last_input_file,"zilch", 0, 0); */
		for (x=d;x;x=x->next){
			vcd_add_signal (&l->first_signal,&l->last_signal, l->last_input_file,x->name, x->lsb, x->msb);
			/*fprintf(stderr,"Added '%s' %d %d\n",x->name,x->msb,x->lsb); */
		}
			if(-1 == write_vcd_header (l)){
			fprintf(stderr,"VCD Header write failed\n");
			goto endvcd;
		}
		fprintf(stderr,"Wrote VCD Hdr\n");
		/**rewind our data  */
    get_next_datarow(NULL,NULL);
		l->first_input_file->buf=buf;
		fprintf(stderr,"r,bit=%d %d\n",l->first_input_file->radix,l->first_input_file->bit_count);
		while (1) {
			if(get_next_datarow(p,buf)){
				vcd_read_sample(l); 
				write_vcd_data (l);
				advance_time (l);
			} else
				break;
		}

		write_vcd_trailer (l);
		close_la2vcd(l);
	}	
	
endvcd:
#endif	/**la2vcd lib def  */
				
	return rtn;
}
