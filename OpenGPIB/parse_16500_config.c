#include <ctype.h>
/**for open...  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"

struct block_spec {
	uint8 blockstart[2]; /**should be #8  */
	char blocklen_data[8];	 /**decimal len of block, stored in ascii  */
	uint32 blocklen;
}__attribute__((__packed__));	

struct section_hdr {
	char name[11]; /**the last byte is reserved, but usally 0  */
	uint8 module_id;
	uint32 block_len; /**lsb is most significant byte, i.e 0x100 is stored as 10 0, not 0 10  */
}__attribute__((__packed__));	

struct hp_config_block {
	struct block_spec bs;
	char *data;
}__attribute__((__packed__));	

struct section {
	struct section_hdr hdr;
	uint32 sz;
	uint32 off;
	char *data;
}__attribute__((__packed__));	
#define LABEL_RECORD_LEN 22
#define LABEL_MAP_LEN 12
struct label_map {
	uint8 unknown1;
	uint8 clk;
	uint8 unknown2[4];
	uint8 pod2_hi;
	uint8 pod2_lo;
	uint8 pod1_hi;
	uint8 pod1_lo; 
	uint8 unknown3;
}__attribute__((__packed__));	

struct labels {
	char name[6];
	uint8 unknown1[3];
	uint8 polarity;
	uint8 unknown2;
	uint8 strange_offset;
	uint16 strange_offsetlo;
	uint8 unknown3[4];
	uint8 bits;
	uint8 enable;
	uint8 sequence;
	uint32 actual_offset;
	struct label_map map;
}__attribute__((__packed__));	
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint32 swapbytes(uint32 in)
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
uint16 swap16(uint16 in)
{
	uint16 out;
	out=in<<8;
	out|=in>>8;
	/*printf("in %d out %d\n",in,out); */
	return out;
}
/***************************************************************************/
/** find location of section using name. Return the offset to start of header.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct section *find_section(char *name, struct hp_config_block *blk )
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
		sec->sz=swapbytes(sec->hdr.block_len);
		printf("Name %s module_id %d len %d\n",sec->hdr.name, sec->hdr.module_id,sec->sz);	
		
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
void show_sections(struct hp_config_block *blk )
{
	uint32 total, sz;
	struct section_hdr hdr;
	for (total=0,sz=1; total<blk->bs.blocklen && sz>0;){
		memcpy(&hdr,(char *)(blk->data+total),sizeof(struct section_hdr));
		hdr.name[10]=0;
		sz=swapbytes(hdr.block_len);
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
void usage(void)
{
	printf("Usage: parse_16500_config -i file\n");
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char *argv[])
{
	FILE *ofd;
	char *ofname;
	struct hp_config_block blk;
	struct section *sec;
	int c;
	ofname=NULL;
	ofd=NULL;
	while( -1 != (c = getopt(argc, argv, "i:h")) ) {
		switch(c){
			case 'i':
				ofname=strdup(optarg);
				break;
			default:
				usage();
				return 1;
		}
	}
	if(NULL == ofname){
		usage();
		goto closem;
	}	
	if(NULL == (ofd=fopen(ofname,"r"))) {
		printf("Unable to open '%s' for writing\n",ofname);
		goto closem;
	}
	/**read in header  */
	fread(&blk,1,10,ofd);
	sscanf(blk.bs.blocklen_data,"%d",&blk.bs.blocklen);
	printf("Blocksize is %d\n",blk.bs.blocklen);
	/**allocate room for data  */
	if(NULL == (blk.data=malloc(blk.bs.blocklen))){
		printf("Unable to malloc %d bytes\n",blk.bs.blocklen);
		goto closem;
	}
	/**read data in  */
	fread(blk.data,1,blk.bs.blocklen,ofd);
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
	if(NULL != ofd)
		fclose(ofd);
	return 0;
}
