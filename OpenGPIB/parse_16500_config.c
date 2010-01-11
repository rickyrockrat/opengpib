/** \file ******************************************************************
\n\b File:        parse_16500_config.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/11/2010  3:48 am
\n\b Description: 
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

#include <ctype.h>
/**for open...  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"
#include "hp16500.h"

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint32 swap32(uint32 in)
{
	uint32 out;
	out=in<<24;
	out|=(in&0x0000FF00)<<8;
	out|=in>>24;
	out|=(in&0x00FF0000)>>8;
	/*printf("in %d out %d\n",in,out); */
	return out;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint64 swap64(uint64 in)
{
	uint64 out;
	out=in<<56;
	out|=in>>56;
	
	out|=(in&0x000000000000FF00)<<40;
	out|=(in&0x00FF000000000000)>>40;
	out|=(in&0x0000000000FF0000)<<24;
	out|=(in&0x0000FF0000000000)>>24;
	out|=(in&0x00000000FF000000)<<8;
	out|=(in&0x000000FF00000000)>>8;
	/*printf("in %016lx out %016lx\n",in,out);  */
	return out;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint16 swap16(uint16 in)
{
	uint16 out;
	out=in<<8;
	out|=in>>8;
	/*printf("in %d out %d\n",in,out); */
	return out;
}
/***************************************************************************/
/** find location of section using name. Allocate and load the data into the 
structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct section *find_section(char *name, struct hp_block_hdr *blk )
{
	uint32 total;
  struct section *sec;
	if( NULL == (sec=malloc(sizeof(struct section))))	{
		printf("Unable to alloc for section\n");
		return NULL;
	}
	for (total=0,sec->sz=1; total<blk->bs.blocklen && sec->sz>0;){
		memcpy(&sec->hdr,(char *)(blk->data+total),sizeof(struct section_hdr));
		sec->hdr.name[10]=0;
		sec->sz=swap32(sec->hdr.block_len);
		printf("Looking at Name %s module_id %d len %d\n",sec->hdr.name, sec->hdr.module_id,sec->sz);	
		
		if(!strcmp(name,sec->hdr.name)){
			if( NULL == (sec->data=malloc(sec->sz)) )	{
				printf("Unable to alloc for section\n");
				free(sec);
				return NULL;
			}		
			memcpy(sec->data,(char *)(blk->data+total+16),sec->sz);
			sec->off=total; /**save location in block header where section starts  */
			return sec;
		}
			
		total+=sec->sz+16; /**16 is size of section info  */
	}	
	return NULL;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_sections(struct hp_block_hdr *blk )
{
	uint32 total, sz;
	struct section_hdr hdr;
	for (total=0,sz=1; total<blk->bs.blocklen && sz>0;){
		memcpy(&hdr,(char *)(blk->data+total),sizeof(struct section_hdr));
		hdr.name[10]=0;
		sz=swap32(hdr.block_len);
		printf("Name '%s' module_id %d len %d\n",hdr.name, hdr.module_id,sz);	
		total+=sz+16; /**16 is size of section info  */
	}
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_labelmaps(struct section *sec)
{
	struct labels l;
	int i,k;
	char x[10];
	/**0x27A is 0x294-1A. 1A is start of machine name 1.  */
	
	for (i=0,k=0x27A; i<0xFF; ++i){
		memcpy(&l,sec->data+k,LABEL_RECORD_LEN);
		l.actual_offset=swap16(l.strange_offsetlo);
		//printf("Actual map offset=0x%x ",l.actual_offset); 
		l.actual_offset-=0x6E90;
		/*printf("Adjusted = 0x%x ",l.actual_offset); */
		memcpy(&l.map,sec->data+0x27A+l.actual_offset,LABEL_MAP_LEN);
		printf("%s map is clk 0x%02x, P1 0x%02x %02x P2 0x%02x %02x #bits %d ena %d seq %d\n",l.name,l.map.clk,
							                    l.map.pod1_hi,l.map.pod1_lo,l.map.pod2_hi,l.map.pod2_lo,
							                    l.bits,l.enable, l.sequence);	
		k+=LABEL_RECORD_LEN;
		snprintf(x,7,"Lab%d  ",i+1);
		if(!strcmp(l.name,x))
			break;
	}
	
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void parse_config( char *cfname)
{
	FILE *cfd;
	struct hp_block_hdr blk;
	struct section *sec;
	if(NULL == (cfd=fopen(cfname,"r"))) {
		printf("Unable to open '%s' for writing\n",cfname);
		goto closem;
	}
	/**read in header  */
	fread(&blk,1,10,cfd);
	sscanf(blk.bs.blocklen_data,"%d",&blk.bs.blocklen);
	printf("Blocksize is %d\n",blk.bs.blocklen);
	/**allocate room for data  */
	if(NULL == (blk.data=malloc(blk.bs.blocklen))){
		printf("Unable to malloc %d bytes\n",blk.bs.blocklen);
		goto closem;
	}
	/**read data in  */
	fread(blk.data,1,blk.bs.blocklen,cfd);
	show_sections(&blk);
	if(NULL == (sec=find_section("CONFIG    ",&blk))){
		printf("Unable to find section 'CONFIG    '\n");
		goto closem;
	}
	printf("First Machine is '%s', second is '%s'\n",sec->data,(char *)(sec->data+0x40));
	/** printf("First Label is '%s'\n",(char *)(sec->data+0x27A));
	memcpy(&l,sec->data+0x27A,LABEL_RECORD_LEN);
	l.actual_offset=swap16(l.strange_offsetlo);
	printf("Actual map offset=0x%x ",l.actual_offset);
	l.actual_offset-=0x6C16;
	printf("Adjusted = 0x%x ",l.actual_offset);
	memcpy(&l.map,sec->data+l.actual_offset,LABEL_MAP_LEN);
	printf("%s map is clk 0x%02x, P1 0x%02x %02x P2 0x%02x %02x\n",l.name,l.map.clk,
						                    l.map.pod1_hi,l.map.pod1_lo,l.map.pod2_hi,l.map.pod2_lo);*/
	show_labelmaps(sec);
	
closem:
	if(NULL != cfd)
		fclose(cfd);
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void swap_analyzer_bytes(struct analyzer_data *a)
{
	a->data_mode=swap32(a->data_mode);
	a->pods=swap32(a->pods);
	a->masterchip=swap32(a->masterchip);
	a->maxmem=swap32(a->maxmem);
	a->sampleperiod=swap64(a->sampleperiod);
	a->tag_type=swap32(a->tag_type);
	a->trig_off=swap64(a->trig_off);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void swapbytes(struct data_preamble *p)
{
	p->instid=swap32(p->instid);
	p->rev_code=swap32(p->rev_code);
	p->chips=swap32(p->chips);
	p->analyzer_id=swap32(p->analyzer_id);
	swap_analyzer_bytes(&p->a1);
  swap_analyzer_bytes(&p->a2);
	
	p->data_pod4_hi=swap32(p->data_pod4_hi);
	p->data_pod3_hi=swap32(p->data_pod3_hi);
	p->data_pod2_hi=swap32(p->data_pod2_hi);
	p->data_pod1_hi=swap32(p->data_pod1_hi);
	p->data_pod4_mid=swap32(p->data_pod4_mid);
	p->data_pod3_mid=swap32(p->data_pod3_mid);
	p->data_pod2_mid=swap32(p->data_pod2_mid);
	p->data_pod1_mid=swap32(p->data_pod1_mid);
	p->data_pod4_master=swap32(p->data_pod4_master);
	p->data_pod3_master=swap32(p->data_pod3_master);
	p->data_pod2_master=swap32(p->data_pod2_master);
	p->data_pod1_master=swap32(p->data_pod1_master);
	p->trig_pod4_hi=swap32(p->trig_pod4_hi);
	p->trig_pod3_hi=swap32(p->trig_pod3_hi);
	p->trig_pod2_hi=swap32(p->trig_pod2_hi);
	p->trig_pod1_hi=swap32(p->trig_pod1_hi);
	p->trig_pod4_mid=swap32(p->trig_pod4_mid);
	p->trig_pod3_mid=swap32(p->trig_pod3_mid);
	p->trig_pod2_mid=swap32(p->trig_pod2_mid);
	p->trig_pod1_mid=swap32(p->trig_pod1_mid);
	p->trig_pod4_master=swap32(p->trig_pod4_master);
	p->trig_pod3_master=swap32(p->trig_pod3_master);
	p->trig_pod2_master=swap32(p->trig_pod2_master);
	p->trig_pod1_master=swap32(p->trig_pod1_master);
	p->rtc.year=swap16(p->rtc.year);
	/*p->rtc.month=swap16(p->rtc.month); */
	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_analyzer(struct analyzer_data *a)
{
	printf("datam=%d pods=%08x master=%04x maxmem=%04x samp=%ldps tagtype=%04x trig_off=%ldps\n",
	a->data_mode, a->pods, a->masterchip, a->maxmem, a->sampleperiod,a->tag_type,a->trig_off);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_pre(struct data_preamble *p)
{
	struct rtc_data *r;
	printf("ID=%d REV=%04x Chips=%04x AID=%04x\n",p->instid,p->rev_code, p->chips, p->analyzer_id);
	printf("Analyzer 1\n");
	show_analyzer(&p->a1);
	printf("Analyzer 2\n");
	show_analyzer(&p->a2);
	printf("Valid Data Rows\n");
	printf("4h=%d 3h=%d 2h=%d 1h=%d 4m=%d 3m=%d 2m=%d 1m=%d M4=%d M3=%d M2=%d M1=%d\n",
	p->data_pod4_hi,p->data_pod3_hi,p->data_pod2_hi,p->data_pod1_hi,
	p->data_pod4_mid,p->data_pod3_mid,p->data_pod2_mid,p->data_pod1_mid,
	p->data_pod4_master,p->data_pod3_master,p->data_pod2_master,p->data_pod1_master);
	printf("Trigger Point\n");
	printf("4h=%d 3h=%d 2h=%d 1h=%d 4m=%d 3m=%d 2m=%d 1m=%d M4=%d M3=%d M2=%d M1=%d\n",
	p->trig_pod4_hi,p->trig_pod3_hi,p->trig_pod2_hi,p->trig_pod1_hi,
	p->trig_pod4_mid,p->trig_pod3_mid,p->trig_pod2_mid,p->trig_pod1_mid,
	p->trig_pod4_master,p->trig_pod3_master,p->trig_pod2_master,p->trig_pod1_master);
	r=&p->rtc;
	printf("Acq time %d/%d/%d dow %d %d:%d:%d\n",
	r->year+1990,r->month,r->day,r->dow,r->hour,r->min,r->sec);
	/**our trace data should start right after r->sec.  */
	/*print_data( */
}


/***************************************************************************/
/** Find the start of the trace.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char *get_trace_start(struct section *sec)
{
	char *s;
	struct data_preamble *lp;
	
	lp=(struct data_preamble *)sec->data;
	
	s=((char *)&lp->rtc.sec - sec->data)+sec->data+1; 
	printf("start %p last %p %ld (0x%lx) @%p\n",sec->data,&lp->rtc.sec,(long int)(&lp->rtc.sec)-(long int)sec->data,(long int)(&lp->rtc.sec)-(long int)sec->data,s); 
	return s;
}
/***************************************************************************/
/** Given a pod and a bit, find all states & times.
\n\b Arguments:
pod - 1-4
state  bit fields to look for that match this state
mask -  bits to look at, 1 means look for it
\n\b Returns:
****************************************************************************/
void search_state(int pod, uint16 clk, uint16 clkmask, uint16 state, uint16 mask,struct data_preamble *pre, struct section *sec)
{
	struct one_card_data *d;
	char *s;
	long int  ps;
	int inc,c,p;
	int cmatch, pmatch;
	ps=0;
	c=0;
	cmatch=pmatch=0;
	if(0 == clkmask)
		cmatch=-1;
	if(0==mask)
		pmatch=-1;
	if(-1 == cmatch && -1== pmatch){
		printf("Must have either clock or pod mask set\n");
		return;
	}
	if(pod>=4 || pod<1){
		printf("Valid pod numbers are 1-4, not %d\n",pod);
		return;
	}
	p=((pod-1)*2);
	p=6-p;
	s=get_trace_start(sec);
	inc = ONE_CARD_ROWSIZE;
	while(s<sec->data + sec->sz){
		uint16 val,_clk,x;
		
		d=(struct one_card_data *)s;
		_clk=d->clklo|(d->clkhi<<8);
		if(clkmask){
			x=_clk&clkmask;
			if(x==clk)
				cmatch=1;
			else cmatch=0;
		}
		val=d->pdata[p+1]|d->pdata[p]<<8;
		if(mask){
			x=val&mask;
			if(x == state)
				pmatch=1;
			else 
				pmatch=0;
		}
		if(1 == pmatch || 1 == cmatch) {
			printf("c%04x pod %04x %ldns\n",_clk,val,ps/1000);
			++c;
			if(c>10 )
				break;
		}
		
		ps+=pre->a1.sampleperiod;
		
		s+=inc;
		
	}	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void print_data(struct data_preamble *p, struct section *sec)
{
	struct one_card_data *d;
	char *dstart;
	long int ps;
	int inc,c;
	
	c=0;
	ps=0;
	inc = ONE_CARD_ROWSIZE;
	
	dstart=get_trace_start(sec);
/*	dstart = (void *)((char *)(&lp->rtc.sec) - (char *)sec->data);  */
	
	while (dstart < sec->data + sec->sz){
		int i;
		d=(struct one_card_data *)dstart;
		printf("CH%02x CL%02x ",d->clkhi, d->clklo);
		for (i=0; i<8; ++i){
			char x,y;
			if(!(i%2))
				x='M';
			else 
				x='L';
			y='0'+(9-i)/2;
			printf("%c%c%02x ",x,y,d->pdata[i]);
			
		}
		
		printf(" %ldns\n",ps/1000);
		ps+=p->a1.sampleperiod;
		dstart+=inc;
		++c;
		if(c>20)
			break;
	}
	
}
/***************************************************************************/
/** .\n\b Arguments:
\n\b Returns:
****************************************************************************/
void parse_data( char *cfname)
{
	FILE *cfd;
	struct hp_block_hdr blk;
	struct section *sec;
	struct data_preamble pre;
	if(NULL == (cfd=fopen(cfname,"r"))) {
		printf("Unable to open '%s' for writing\n",cfname);
		goto closem;
	}
	/**read in header  */
	fread(&blk,1,10,cfd);
	sscanf(blk.bs.blocklen_data,"%d",&blk.bs.blocklen);
	printf("Blocksize is %d\n",blk.bs.blocklen);
	/**allocate room for data  */
	if(NULL == (blk.data=malloc(blk.bs.blocklen))){
		printf("Unable to malloc %d bytes\n",blk.bs.blocklen);
		goto closem;
	}
	/**read data in  */
	fread(blk.data,1,blk.bs.blocklen,cfd);
	show_sections(&blk);
	if(NULL == (sec=find_section("DATA      ",&blk))){
		printf("Unable to find section 'DATA      '\n");
		goto closem;
	}
	memcpy(&pre,sec->data,sizeof(struct data_preamble));
	swapbytes(&pre);
	show_pre(&pre);
	
	print_data(&pre,sec);
	search_state(1,0,0,0x800,0x800,&pre,sec);
	
closem:
	if(NULL != cfd)
		fclose(cfd);
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("Usage: parse_16500_config <options>\n"
	" -c filename set name of config file\n"
	" -d filename set name of data file\n"
	"");
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char *argv[])
{
	char *cfname, *dfname;
	int c;
	dfname=cfname=NULL;
	while( -1 != (c = getopt(argc, argv, "c:d:h")) ) {
		switch(c){
			case 'c':
				cfname=strdup(optarg);
				break;
			case 'd':
				dfname=strdup(optarg);
				break;
			default:
				usage();
				return 1;
		}
	}
	if(NULL == cfname && NULL == dfname){
		usage();
		goto closem;
	}	
	if(NULL != cfname ){
		parse_config(cfname);
	}
	if(NULL != dfname ){
		parse_data(dfname);
	}
closem:
	return 0;
}
