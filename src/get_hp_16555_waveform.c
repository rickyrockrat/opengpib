/** \file ******************************************************************
\n\b File:        get_hp_16555_waveform.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  6:43 am
\n\b Description: Get waveforms from the HP 1655x logic analizer cards.
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
#define HP16500_COM_SYMBOLS
#include "open-gpib.h"
#include "hp16500.h"
#include "hp1655x.h"

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
\n\b Returns: slot number of card.
****************************************************************************/
int init_instrument(struct hp_common_options *o,struct gpib *g)	 
{
	int slot;
	if(-1 == (slot=hp16500_find_card(o->cardtype,o->cardno,g)) )
		return -1;
  if(SLOTNO_16500C != slot)
	  select_hp_card(slot, g);
	
	sprintf(g->buf,"*OPC\n");
	write_string(g,g->buf);	
	sprintf(g->buf,":menu %d,0\n",slot);
	write_string(g,g->buf);	
	return slot;
/*34,-1,12,12,11,1,0,5,5,5 */
	/** :WAV:REC FULL;:WAV:FORM ASC;:SELECT? */
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int wait_for_data(struct gpib *g)
{
	int i;
	/**make sure it is enabled  */
	usleep(10000);
	sprintf(g->buf,"*OPC\n");
	write_string(g,g->buf);	
	while(1){
		sprintf(g->buf,"*OPC?\n");
		i=write_get_data(g,g->buf);	
		if( i ){
			if(g->buf[0] == '1')
				break;
		}
		usleep(10000);
	}
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int message_avail(struct gpib *g)
{
	int i;
	/**make sure it is enabled  */
	usleep(10000);
	sprintf(g->buf,"*STB?\n");
	i=write_get_data(g,g->buf);	
	if( i ){
		if(g->buf[0] & 0x10)
			return 1;
	}
	usleep(10000);
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	fprintf(stderr,"Version %s: get_hp_16555_waveform <options>\n",TOSTRING(VERSION));
	show_common_usage(COM_USE_MODE_LOGIC);
	/**common options are -a, -d, -n, -m, and -t */
	fprintf(stderr,"\n"
	" -c fname put config data to fname\n"
	" -o fname put config lfDATAlf data to file called fname\n"
	" -p set packed mode to packed for config only (unpacked)\n"
	" -r sample if !0, set sample period to sample ns and send run before getting data\n"
	" -s fname put signal(data) to fname\n"
#ifdef LA2VCD_LIB
	" -v basename put vcd data to basename.vcd. Creates .dat and .cfg\n"
#endif	
	"");
	show_hp_connection_info();
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
long int write_data_to_file(struct gpib *g, FILE *fd)
{
	
	long int  sz,total;
	int nodata;
	uint32_t i, off=0;
	/*wait_for_data(g); */
	usleep(1000000); 
	sz=total=0;
	
	while(1){
		
		i=read_raw(g);
		if(i && 0 == total){
			/**clean out beginning of buffer, if needed.  */
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
			
			if(i)	{	/**fixme: What if we don't have enough data?  */
				sz=get_datsize(g->buf);
				fprintf(stderr,"Len %ld Got %d ",sz,i);	
			}
			
		}else{
			fprintf(stderr,"%ld ",sz-total);
			off=0;
		}
			
		fflush(NULL);
		fwrite(&g->buf[off],1,i,fd);	
		total+=i;
		if(sz && total>= sz+HEADER_SIZE)
			break;
		if(0 == i){
			++nodata;
			if(nodata>10){
				fprintf(stderr,"No Data for 10 tries\n");
				break;
			}
				
		}else
			nodata=0;
	}
	fprintf(stderr,"Wrote %ld bytes\n",total);
	if(total!=sz+HEADER_SIZE){	/**add header size of 10  */
		fprintf(stderr,"Total does not match size. Sending CLR\n");
		g->control(g,CTL_SEND_CLR,0);
	}
	return total;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct gpib *g;
	struct hp_common_options copt;
	
	FILE *ofd,*cfd,*tfd,*vfd;
	char *ofname;
	char *sname,*cname,*vname;
	int i, c, rtn, raw,trigger,slot, speriod;
	long int total;
	rtn=1;
	trigger=0;
	ofname=sname=cname=vname=NULL;
	vfd=tfd=cfd=ofd=NULL;
	raw=0;
	handle_common_opts(0,NULL,&copt);
	copt.cardtype=CARDTYPE_16554M;
	while( -1 != (c = getopt(argc, argv, "c:ho:pr:s:v:"HP_COMMON_GETOPS)) ) {
		switch(c){
			case 'a':
			case 'd':
			case 'm':
			case 'n':
			case 't':
				if(handle_common_opts(c,optarg,&copt))
					return -1;
				break;
			case 'c':
				if(NULL != vname){
					fprintf(stderr,"-v and -c are mutually exclusive.\n");
					usage();
					return 1;
				}
				cname=strdup(optarg);
				break;
			case 'h':
				usage();
				return 1;
			case 'o':
				ofname=strdup(optarg);
				break;
			case 'p':
				raw=1;
				break;
			case 'r':
				speriod=atoi(optarg);
				if(0!=speriod){
					if(-1 ==validate_sampleperiod(speriod)){
						fprintf(stderr,"Invalid period %dns (must be 2,4,8 or 8ns increments)\n",speriod);
						return -1;
					}	
				}
				
				trigger=1;
				break;
			case 's':
				if(NULL != vname){
					fprintf(stderr,"-v and -s are mutually exclusive.\n");
					usage();
					return 1;
				}
				sname=strdup(optarg);
				break;
			case 'v':
#ifdef LA2VCD_LIB 
				if(NULL != sname || NULL != cname){
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
				sname=&vname[i];
				sprintf(sname,"%s.dat",optarg);
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
	if(NULL != ofname && (NULL != sname || NULL != cname)){
		fprintf(stderr,"-o and -t/-c are mutually exclusive\n");
		usage();
		return 1;
	}
	if(NULL == ofname && NULL == sname && NULL == cname){
		fprintf(stderr,"must specify -t/-c or -o\n");
		usage();
		return 1;
	} 
	if(NULL != vname && (NULL == cname || NULL == sname)){
		fprintf(stderr,"Must specify -c and -t with -v\n");
		usage();
		return 1;
	}
	if(NULL == (g=open_gpib(copt.dtype,copt.iaddr,copt.dev,1048576))){
		fprintf(stderr,"Can't open/init controller at %s. Fatal\n",copt.dev);
		return 1;
	}
	
	if(-1 == (slot=init_instrument(&copt,g)) ){
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
		if(NULL !=sname){
			if(NULL == (tfd=fopen(sname,"w+"))) {
				fprintf(stderr,"Unable to open '%s' for writing\n",sname);
				goto closem;
			}
		}
	}
	if(trigger){
		char range[100], period[100];
		range[0]=period[0]=0;
		/**set waveform to minimal waveform display, so it doesn't take forever to draw  */
		sprintf(g->buf,":mach1:TWAV:RANGE?\n");
		i=write_wait_for_data(g,g->buf,5);
		if(i && i<100)
			sprintf(range,"%s",g->buf);
		if(0 != speriod){
				/**set the sample period*/
			sprintf(g->buf,":mach1:TTR:SPER?\n");
			i=write_wait_for_data(g,g->buf,5);
			if(i && i<100)
				sprintf(period,"%s",g->buf);
			sprintf(g->buf,":mach1:TTR:SPER %dE-9\n",speriod);
			i=write_string(g,g->buf);	
		}
		
		sprintf(g->buf,":mach1:TWAV:RANGE 10E-9\n");
		i=write_string(g,g->buf);	
		fprintf(stderr,"Starting Aquisition. Waiting for Data\n");
		sprintf(g->buf,":RMODE SINGLE");
		i=write_string(g,g->buf);	
		sprintf(g->buf,":START");
		i=write_string(g,g->buf);	
		wait_for_data(g);
		/**move menu to config so it won't paint the screen...  */
		sprintf(g->buf,":menu %d,0\n",slot+1);
		write_string(g,g->buf);	
		if(range[0]){
			sprintf(g->buf,":mach1:TWAV:RANGE %s\n",range);
			write_string(g,g->buf);
			usleep(10000);
		}
		if(period[0]){
			sprintf(g->buf,":mach1:TTR:SPER %s\n",period);
			write_string(g,g->buf);
			usleep(10000);
		}		
	}
	if(cfd>0 ||ofd>0){ /**grab config  */
		fprintf(stderr,"Retreiving Setup\n");
		sprintf(g->buf,":SYSTEM:SETUP?");
		i=write_string(g,g->buf);	
		total=write_data_to_file(g,ofd?ofd:cfd);	

		if(ofd>0){
			i=sprintf(g->buf,"\nDATA\n");
			fwrite(g->buf,1,i,ofd);		
		}
	}
	usleep(100000);
	
	if(tfd>0 ||ofd>0){ /**grab data  */
		
		fprintf(stderr,"Retreiving Data\n");
	/*	goto closem; */
		sprintf(g->buf,":DBLOCK %s;:SYSTEM:DATA?",raw?"PACKED":"UNPACKED");
		i=write_string(g,g->buf);	
		total=write_data_to_file(g,ofd?ofd:tfd);	
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
/*	if(total != sz+10) */
/*		goto endvcd; */
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
		if(NULL == (p=parse_data(sname,NULL,JUST_LOAD))){
			fprintf(stderr,"Unable to re-open data file '%s'\n",sname);
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
    if(vcd_add_file(l,NULL,16,d->bits, V_WIRE)){
			fprintf(stderr,"Failed to add input file\n");
			goto endvcd;
		}
		fprintf(stderr,"samp=%dns Bits=%d\n",(int)(p->a1.sampleperiod/1000),d->bits);
		/**Add our signal descriptions in  */
/*		vcd_add_signal (&l->first_signal,&l->last_signal, l->last_input_file,"zilch", 0, 0); */
		for (x=d;x;x=x->next){
			vcd_add_signal (l,V_WIRE, x->msb, x->lsb,x->name);
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
		close_la2vcd(l);
	}	
	
endvcd:
#endif	/**la2vcd lib def  */
				
	return rtn;
}
#if 0
/**  		while (!message_avail(g))
			usleep(10000);*/
/*		wait_for_data(g); */
		usleep(100000); 
		sz=total=0;
		
		while(1){
			i=read_raw(g);
			if(i && 0 == total){
				sz=get_datsize(g->buf);
				fprintf(stderr,"Len %ld Got %d ",sz,i);
			}else
				fprintf(stderr,"%ld ",sz-total);
			fflush(NULL);
			fwrite(g->buf,1,i,ofd?ofd:cfd);	
			total+=i;
			if(sz && total>= sz+HEADER_SIZE)
				break;
			if(0 == i){
				++nodata;
				if(nodata>10){
					fprintf(stderr,"No Data for 10 tries\n");
					break;
				}
			}else
				nodata=0;
				
		}	
		
/**DATA  */
		/*wait_for_data(g); */
		usleep(1000000); 
		sz=total=0;
		
		while(1){
			uint32_t off=0;
			i=read_raw(g);
			if(i && 0 == total){
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
				
			}else{
				fprintf(stderr,"%ld ",sz-total);
				off=0;
			}
				
			fflush(NULL);
			fwrite(&g->buf[off],1,i,ofd?ofd:tfd);	
			total+=i;
			if(sz && total> sz)
				break;
			if(0 == i){
				++nodata;
				if(nodata>10){
					fprintf(stderr,"No Data for 10 tries\n");
					break;
				}
					
			}else
				nodata=0;
				
		}
#endif
