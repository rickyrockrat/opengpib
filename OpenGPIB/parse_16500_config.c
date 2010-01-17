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
If a the file pointer is not null, output the data in this format:
Label  Ch Cl P4h l P3h l P2h l P1h l
POL    00 00 00 00 00 00 00 00 00 08

Where h= hi, l= lo, C=CLK and P=POD. In the above example, POL is bit 3, on
Pod 1
\n\b Returns:
****************************************************************************/
void show_label(struct labels *l, FILE *out)
{
	int m;
	if(NULL != out){
		fprintf(out,"%s ",l->name);
		for (m=0;m<10;++m){
			fprintf(out,"%02x ",l->map.clk_pods[m]);
		}
		fprintf(out,"\n");
	}else{
		printf("%s map is clk ",l->name);
		for (m=0;m<10;++m){
			if(m>1 && !(m%2))
				printf("P%d ",5-((m+2)/2));
			printf("%02x ",l->map.clk_pods[m]);
		}
			
		printf(" #bits %d ena %d seq %d ",l->bits,l->enable, l->sequence);	
		/** for (m=0;m<3;++m)
			printf("%02x ",l->unknown1[m]);
		printf("*3* ");
		for (m=0;m<4;++m)
			printf("%02x ",l->unknown3[m]);*/
		printf("\n");
	}
		
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_labelmaps(struct section *sec, FILE *out)
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
		show_label(&l, out);
			
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
struct hp_block_hdr *read_block(char *cfname)
{
	struct hp_block_hdr *blk=NULL;
	FILE *cfd=NULL;
	
	if(NULL == (cfd=fopen(cfname,"r"))) {
		printf("Unable to open '%s' for writing\n",cfname);
		goto closem;
	}
	if(NULL == (blk=malloc(sizeof(struct hp_block_hdr))) ){
		printf("Unable to malloc for hdr\n");
		goto closem;
	}
	/**read in header  */
	fread(blk,1,10,cfd);
	sscanf(blk->bs.blocklen_data,"%d",&blk->bs.blocklen);
	printf("Blocksize is %d\n",blk->bs.blocklen);
	/**allocate room for data  */
	if(NULL == (blk->data=malloc(blk->bs.blocklen))){
		printf("Unable to malloc %d bytes\n",blk->bs.blocklen);
		goto closem;
	}
	/**read data in  */
	fread(blk->data,1,blk->bs.blocklen,cfd);
closem:
	if(NULL != cfd)
		fclose(cfd);
	return blk;
}

#define SHOW_PRINT 1
#define JUST_LOAD 2
/***************************************************************************/
/** .
\n\b Arguments:
cfname is config file input name, name is name of section to return, mode 
indicates if we want info or quiet.
\n\b Returns:
****************************************************************************/
struct section *parse_config( char *cfname, char *name, int mode)
{
	struct hp_block_hdr *blk;
	struct section *sec;
	
	if(NULL == (blk=read_block(cfname))) {
		printf("Unable to read block from '%s'\n",cfname);
		return NULL;
	}
	
	if(NULL == (sec=find_section(name,blk))){
		printf("Unable to find section '%s'\n",name);
		return NULL;
	}
	if(SHOW_PRINT == mode){
		show_sections(blk);
		printf("First Machine is '%s', second is '%s'\n",sec->data,(char *)(sec->data+0x40));
		show_labelmaps(sec, NULL);
	}
	return sec;
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
	int i;
	p->instid=swap32(p->instid);
	p->rev_code=swap32(p->rev_code);
	p->chips=swap32(p->chips);
	p->analyzer_id=swap32(p->analyzer_id);
	swap_analyzer_bytes(&p->a1);
  swap_analyzer_bytes(&p->a2);
	for (i=0; i<4; ++i){
		p->data_hi[i]=swap32(p->data_hi[i]);
		p->data_mid[i]=swap32(p->data_mid[i]);
		p->data_master[i]=swap32(p->data_master[i]);
		p->trig_hi[i]=swap32(p->trig_hi[i]);
		p->trig_mid[i]=swap32(p->trig_mid[i]);
		p->trig_master[i]=swap32(p->trig_master[i]);
	}
	
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
	int i;
	printf("ID=%d REV=%04x Chips=%04x AID=%04x\n",p->instid,p->rev_code, p->chips, p->analyzer_id);
	printf("Analyzer 1\n");
	show_analyzer(&p->a1);
	printf("Analyzer 2\n");
	show_analyzer(&p->a2);
	printf("Valid Data Rows\n");
	for (i=0; i<4; ++i){
		printf("%dh=%d ",(3-i)+1,p->data_hi[i]);
	}
	for (i=0; i<4; ++i){
		printf("%dm=%d ",(3-i)+1,p->data_mid[i]);
	}
	for (i=0; i<4; ++i){
		printf("%dM=%d ",(3-i)+1,p->data_master[i]);
	}
	printf("\nTrigger Point\n");
	for (i=0; i<4; ++i){
		printf("%dh=%d ",(3-i)+1,p->trig_hi[i]);
	}
	for (i=0; i<4; ++i){
		printf("%dm=%d ",(3-i)+1,p->trig_mid[i]);
	}
	for (i=0; i<4; ++i){
		printf("%dM=%d ",(3-i)+1,p->trig_master[i]);
	}
	r=&p->rtc;
	printf("\nAcq time %d/%d/%d dow %d %d:%d:%d\n",
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
#define MODE_EDGES 1
#define MODE_LEVELS 0
/***************************************************************************/
/** Given a pod and a bit, find all states & times.
\n\b Arguments:
pod - 1-4
state  bit fields to look for that match this state
mask -  bits to look at, 1 means look for it
\n\b Returns:
****************************************************************************/
void search_state(int pod, uint16 clk, uint16 clkmask, uint16 state, uint16 mask, int mode,struct data_preamble *pre, struct section *sec)
{
	struct one_card_data *d;
	char *s,*b;
	uint32 count;
	long int  ps;
	int inc,c,p;
	int cmatch, pmatch, lc, lp;
	ps=0;
	c=0;
	cmatch=pmatch=0;
	lc=lp=1;
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
	b=s=get_trace_start(sec);
	inc = ONE_CARD_ROWSIZE;
	count=0;
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
		if(mode&MODE_LEVELS){
			if(1 == pmatch || 1 == cmatch) {
			
				printf("c%04x pod %04x %ldns\n",_clk,val,ps/1000);
				++c;
				if(c>10 )
					break;	
			}
		}else if(mode&MODE_EDGES){
			x=0;
			if(pmatch != lp){
				x=1;
				lp=pmatch;
			}
			if(cmatch != lc){
				x=1;
				lc=cmatch;
			}
			if(x){
				printf("%08d %08lx c%04x pod %04x %ldns\n",count,(s-b)+0x258,_clk,val,ps/1000);
				++c;
				if(c>10 )
					break;	
			}
		}
		
		ps+=pre->a1.sampleperiod;
		
		s+=inc;
		++count;
		if(count>=pre->data_master[3])
			break;
	}	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int number_of_pods_assigned(uint32 pods)
{
	int i, p;
	for (p=0,i=1;i;i<<=1)
		if(i&pods)
			++p;
	printf("pods_assigned %d\n",p);
	return p;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint32 valid_rows(int pod_no, uint32 pods, uint32 *rows)
{
	uint32 i;
	if(pod_no <1 || pod_no>12){
		printf("valid_rows: invalid pod %d\n",pod_no);
		return 0;
	}
	i=1<<pod_no;
	if(i&pods)
		return rows[4-pod_no];
	return 0;
}
/***************************************************************************/
/** 
Look at the pods assigned to the analyzer, then look at the valid rows of 
that pod to know if the data is good.  Also, if the mode of the analyzer 
is 13, it is half-channel so you *know* the second pod data is not good 
reguardless of the what the data_xxx vars say. 
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint32 put_data_to_file(struct data_preamble *p, struct section *sec, char *fname)
{
	struct one_card_data *d;
	char *dstart;
	long int ps;
	int inc,c,fields,i;
	FILE *out;
	uint32 count;
	count=0;
	
	ps=0;
	
	if(NULL == (out=fopen(fname,"w+"))) {
		printf("Unable to open '%s' for writing\n",fname);
		goto closem;
	}
	printf("Writing file '%s'\n",fname);
	/*if(13 == p->a1.data_mode) */
	fields=number_of_pods_assigned(p->a1.pods);
	
	printf("Valid rows: ");
	for (c=1; c<5; ++c){
		printf("%d=%d ",c,valid_rows(c,p->a1.pods,p->data_master));
	}
	printf("\n");
	dstart=get_trace_start(sec);
/*	dstart = (void *)((char *)(&lp->rtc.sec) - (char *)sec->data);  */
	c=0;
	inc = ONE_CARD_ROWSIZE;
	while (dstart < sec->data + sec->sz){
		d=(struct one_card_data *)dstart;
		fprintf(out,"%02x%02x",d->clkhi, d->clklo);
		if(fields>1){/**if 1, just clock??  */
			
			for (i=4;i>0;--i){
				int x;
				if(valid_rows(i,p->a1.pods,p->data_master))	{
					x=8-(i*2);
					fprintf(out,"%02x%02x",d->pdata[x],d->pdata[x+1]);
				}
			}
		}	
		fprintf(out,"\n");
		/**just print out clk, pod 1 & 2 data  */
/*		fprintf(out,"%02x%02x%02x%02x%02x%02x\n",d->clkhi, d->clklo,d->pdata[4],d->pdata[5],d->pdata[6],d->pdata[7]); */
		ps+=p->a1.sampleperiod;
		dstart+=inc;
		++count;
	}
closem:
	if(NULL != out)
		fclose(out);
	return count;
	
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
	uint32 count;
	count=0;
	c=0;
	ps=0;
	inc = ONE_CARD_ROWSIZE;
	printf("Valid rows= %d\n",p->data_master[3]);
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
struct data_preamble *parse_data( char *cfname, char *out, int mode)
{
	struct hp_block_hdr *blk;
	struct section *sec;
	struct data_preamble *pre;
	
	if(NULL == (pre=malloc(sizeof(struct data_preamble)))){
		printf("Unable to allocate for data_preamble struct\n");
		return NULL;
	}
	
	if(NULL == (blk=read_block(cfname))) {
		printf("Unable to read '%s' for writing\n",cfname);
		goto err;
	}
	if(mode != JUST_LOAD)
		show_sections(blk);
	if(NULL == (sec=find_section("DATA      ",blk))){
		printf("Unable to find section 'DATA      '\n");
		goto err;
	}
	
	memcpy(pre,sec->data,sizeof(struct data_preamble));
	swapbytes(pre);
	if(mode == JUST_LOAD)
		return pre;
	show_pre(pre);
	
	print_data(pre,sec);
	search_state(1,0,0,0x800,0x800,MODE_EDGES,pre,sec);
	if(NULL != out)
		put_data_to_file(pre,sec,out);
	return pre;
err:
	free(pre);
	return NULL;
}

/***************************************************************************/
/** pods[0] is pod4, hi byte..
active[0] is pod1
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int show_vcd_label(uint8 *a, struct labels *l )
{
	int i,bits, bs, be,p,bytes;
	uint8 pod[26];
	uint16 x;
	/*printf("       "); */
	for (bits=i=0;i<13;++i){
		/*printf(" %d ",a[12-i]); */
		if(a[i])
			bits+=16;/**each pod is 16 lines, and we register the clock as 16 too  */
	}
	bytes=bits/8;
	/*printf("bits=%d, bytes=%d\n",bits,bytes);	 */
	/**now load the data array  */
	p=bytes-1;
	for (i=0;i<10;++i){
		if(a[i/2]){
			if(p<0){
				printf("Fatal Err.p -1");
				return 1;
			}
			pod[p--]=l->map.clk_pods[i];	
			/*printf("%02x ",l->map.clk_pods[i]); */
		}
	}
	/*printf("\n"); */
	/**now pod has clk top two indexes and lowest pod in 0,1, lsb in 0  */	
	/**we start out off-by one on bit position (p)  */
	bs=be=0;
	for (p=i=0;i<bytes;++i){
		
		for (x=1;x<0x100;x<<=1){
			++p;
			if(x&pod[i]){
				if(bs && be ){
					printf("Can't handle discontinous bits at bit %d for label %s\n",p-1,l->name);
					return 1;
				}
				if(!bs)
					bs=p;	/**this is off by one.  */
			}else if(bs && !be){
				be=p-1;
			}
				
		}
	}	
	if (bs==be)
	 printf("-sf %d %s ",bs-1,l->name);
	else
		printf("-sf %d %d %s ",be-1, bs-1,l->name);
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
pre is the preable from the data. sec is the config section from the config.
\n\b Returns:
****************************************************************************/
void show_la2vcd(struct data_preamble *pre, struct section *sec)
{
	int fields,i,k;
	char x[10];
	struct labels l;
	uint8 active[13]; /**max 12 pods +clock can be assigned to one machine.  */
										/**but only 4 pod logic is implemented here...  */
	printf("-td %ld ",pre->a1.sampleperiod);
	fields=number_of_pods_assigned(pre->a1.pods);
	memset(active,0,13);
	active[4]=1;
	/**set this array so that pod 1 is high bit, so we match data struct in label  */
	if(fields>1){/**if 1, just clock??  */
		for (i=4;i>0;--i){
			if(valid_rows(i,pre->a1.pods,pre->data_master))	{
				active[i-1]=1;
			}
		}	
	}	
			
	/**0x27A is 0x294-1A. 1A is start of machine name 1.  */
	for (i=0,k=0x27A; i<0xFF; ++i){
		memcpy(&l,sec->data+k,LABEL_RECORD_LEN);
		l.actual_offset=swap16(l.strange_offsetlo);
		//printf("Actual map offset=0x%x ",l.actual_offset); 
		l.actual_offset-=0x6E90;
		/*printf("Adjusted = 0x%x ",l.actual_offset); */
		memcpy(&l.map,sec->data+0x27A+l.actual_offset,LABEL_MAP_LEN);
		show_vcd_label(active,&l);
			
		k+=LABEL_RECORD_LEN;
		snprintf(x,7,"Lab%d  ",i+1);
		if(!strcmp(l.name,x))
			break;
	}
	printf("\n");
			
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
	" -f file set name of output file. Send valid data to file\n"
	" -l file set name of output file. Send label map to file\n"
	" -v set la2vcd mode\n"
	"");
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char *argv[])
{
	char *cfname, *dfname, *lfname,*outfname;
	struct section *s;
	int c,v;
	s=NULL;
	v=SHOW_PRINT;
	outfname=dfname=lfname=cfname=NULL;
	while( -1 != (c = getopt(argc, argv, "c:d:f:hl:v")) ) {
		switch(c){
			case 'c':
				cfname=strdup(optarg);
				break;
			case 'd':
				dfname=strdup(optarg);
				break;
			case 'f':
				outfname=strdup(optarg);
				break;
			case 'l':
				lfname=strdup(optarg);
				break;
			case 'v':
				v=JUST_LOAD; 
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
	if(NULL != lfname && NULL != cfname ){
		
		if(NULL !=(s=parse_config(cfname,"CONFIG    ",JUST_LOAD) ) ){
			FILE *cfd=NULL;
	
			if(NULL == (cfd=fopen(lfname,"w+"))) {
				printf("Unable to open '%s' for writing\n",lfname);
				goto closem;
			}
			
			show_labelmaps(s, cfd);
			if(NULL != cfd)
				fclose(cfd);
		}
	}	else	if(NULL != cfname ){
		s=parse_config(cfname,"CONFIG    ",v);
	}
	printf("************************\n");
	if(NULL != dfname ){
		struct data_preamble *p;
		if(NULL != (p=parse_data(dfname,outfname,v))){
			printf("EEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
			if(JUST_LOAD==v && NULL !=s){
				printf("Showing stuff\n");
				show_la2vcd(p,s);
			}
				
		}
		
	}
closem:
	return 0;
}
