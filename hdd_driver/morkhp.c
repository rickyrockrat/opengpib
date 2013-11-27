/** \file ******************************************************************
\n\b File:        morkhp.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        12/13/2010  8:45 am
\n\b Description:  Read the first 512 bytes of a partition and dump contents
Fat16 for now. Also modify the first 512 bytes to boot on HP16500C.
*/ /************************************************************************
Change Log: \n
Working 1.6G disk

Jump eb 3e 90 OEM name 'HP16500B'
sector size 32768 sectors per cluser 1 reserved sectors 1
fat copies 2 maxdirs 1024 media_desc 0xFA sectors per fat 4 sectors 49266
sectors per track 63 heads 16 hidden sectors 0 sectors in parition 0
Logical drive no 0, extended sig 0x29 serial 595099512(0x23787f78)
volname 'B16500     ' fatname 'FAT16   '

fdisk -l produces:

Disk /dev/loop1: 1614 MB, 1614348288 bytes
255 heads, 63 sectors/track, 196 cylinders
Units = cylinders of 16065 * 512 = 8225280 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x00000000
This drive is mounted by doing this in the fat driver:
		u32 mult;
		u8 x8;
		u16 x16;
		printk(KERN_INFO "FAT: found HP16500. Set blk to 512:");
		x16=get_unaligned_le16(&b->sector_size);
		mult=x16/512;
		put_unaligned_le16(512,&b->sector_size);
		now adjust all sector vars by mult.  
		x8=x16=mult;
		
		b->sec_per_clus*=x8;
		put_unaligned_le16(x16*get_unaligned_le16(&b->reserved),&b->reserved);
		if(0!=b->total_sect){
			printk(KERN_INFO "FAT: total_sect %d. Changing!\n",b->total_sect);
		}
		put_unaligned_le32(mult*((u32)(get_unaligned_le16(&b->sectors))),&b->total_sect);
		*(u16 *)b->sectors=0;
		put_unaligned_le16(x16*get_unaligned_le16(&b->fat_length),&b->fat_length);
		put_unaligned_le16(x16*get_unaligned_le16(&b->secs_track),&b->secs_track);
		put_unaligned_le32(mult*get_unaligned_le32(&b->hidden),&b->hidden);


mkdosfs -S 8192 -s 4 -f 2 -R 4 -F 16 -h 0 -I -C -n "B16500     " x 197064
gives you this  (197064/24633=8)

Jump eb 3c 90 OEM name 'mkdosfs'
sector size 8192 sectors per cluser 4 reserved sectors 4
fat copies 2 maxdirs 512 media_desc 0xF8 sectors per fat 2 sectors 24633
sectors per track 32 heads 64 hidden sectors 0 sectors in parition 0
Logical drive no 0, extended sig 0x29 serial 541616493(0x2048696d)
volname 'B16500     ' fatname 'FAT16   ' exe:

*/

#define _XOPEN_SOURCE	500
/*#define __USE_XOPEN_EXTENDED 1 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BYTES_TO_READ 512

/**first sector of every partition  */
struct fat16_boot{
	unsigned char ignored[3];
	char system_id[8];
	__uint16_t sector_size;
	__uint8_t sec_per_clus;
	__uint16_t reserved;
	__uint8_t fats;
	__uint16_t dir_entries;
	__uint16_t sectors;
	__uint8_t media;
	__uint16_t fat_length;
	__uint16_t secs_track;
	__uint16_t heads;
	__uint32_t hidden;
	__uint32_t total_sect;
/**fat32 use only??  */	
	__uint16_t logicaldrive;
	__uint8_t	extended_sig;
	__uint32_t serial;
	char volname[11];
	char fatname[8];
	__uint8_t exe[448];
	unsigned char marker[2];
	
}__attribute__((__packed__));
typedef __uint8_t __u8;
typedef __uint16_t __le16;
typedef __uint32_t __le32;
/**from linux/msdos_fs.h  */
struct fat_boot_sector {
	__u8	ignored[3];	/* Boot strap short or near jump */
	__u8	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	__u8	sector_size[2];	/* bytes per logical sector */
	__u8	sec_per_clus;	/* sectors/cluster */
	__le16	reserved;	/* reserved sectors */
	__u8	fats;		/* number of FATs */
	__u8	dir_entries[2];	/* root directory entries */
	__u8	sectors[2];	/* number of sectors */
	__u8	media;		/* media code */
	__le16	fat_length;	/* sectors/FAT */
	__le16	secs_track;	/* sectors per track */
	__le16	heads;		/* number of heads */
	__le32	hidden;		/* hidden sectors (unused) */
	__le32	total_sect;	/* number of sectors (if sectors == 0) */

	/* The following fields are only used by FAT32 */
	__le32	fat32_length;	/* sectors/FAT */
	__le16	flags;		/* bit 8: fat mirroring, low 4: active fat */
	__u8	version[2];	/* major, minor filesystem version */
	__le32	root_cluster;	/* first cluster in root directory */
	__le16	info_sector;	/* filesystem info sector */
	__le16	backup_boot;	/* backup boot sector */
	__le16	reserved2[6];	/* Unused */
}__attribute__((__packed__));

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int swap_buffer(char *buf, int no)
{
	char *b2;
	if(NULL ==(b2=malloc(no))){
		printf("Out of mem for %d\n",no);
		return 1;
	}
	memcpy(b2,buf,no);
	swab(b2,buf,no);
	free(b2);
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int cp_name( char *s, char *d, int len)
{
	int i;
	for (i=0;i<len; ++i)
		d[i]=s[i];
	d[i]=0;
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int display_fat16(char *buf, int no)
{
	struct fat16_boot *f;
	char nbuf[20];
	int i;
	i= sizeof(struct fat16_boot);
	if(no <i){
		printf("buf size (%d) not bigenough for struct (%d)",no,i);
		return 1;
	}
	f=(struct fat16_boot *)buf;
	printf("\nJump %02x %02x %02x ",f->ignored[0],f->ignored[1],f->ignored[2]);
	cp_name(f->system_id,nbuf,8);
	printf("OEM name '%s'\n",nbuf);
	printf("sector size %d sectors per cluser %d reserved sectors %d\n",
	f->sector_size, f->sec_per_clus, f->reserved);
	printf("fat copies %d maxdirs %d media_desc 0x%02X sectors per fat %d sectors %d\n",
	f->fats, f->dir_entries, f->media,	f->fat_length,f->sectors);
	printf("sectors per track %d heads %d hidden sectors %d sectors in parition %d\n",
		f->secs_track,f->heads,f->hidden,f->total_sect);
	printf("Logical drive no %d, extended sig 0x%x serial %d(0x%x)\n",
	f->logicaldrive,f->extended_sig,f->serial,f->serial);
	cp_name(f->volname,nbuf,11);
	printf("volname '%s' ",nbuf);
	cp_name(f->fatname,nbuf,8);
	printf("fatname '%s' exe:\n",nbuf);
/** 	for (i=0;i<448;++i){
		if(i && 0==(i%16))
			printf("\n");
		printf("%02x ",f->exe[i]);
	}	*/
		
	printf("\nMarker %02x %02x\n",f->marker[0],f->marker[1]);
	printf("\n");
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
__uint32_t check_div(char *name,__uint32_t val, __uint32_t div)
{
	if(val < div || 0 == val || (val%div)){
		printf("New Param %s (%d) zero, not large enough, or not evenly divisible by %d\n",name,val,div);
		return 0;
	}
	return val/div;
}
/***************************************************************************/
/** buf should contain existing boot. Modify for HP16500C.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_new_boot(char * buf, int no, int fd, int swap)
{
	int i;
	struct fat16_boot *f=(struct fat16_boot *)buf;
	__uint32_t tsec,div;
	__uint8_t exe[]={0,0x90,
		0xfa,0xb8,0x30,0x00,0x8e,0xd0,0xbc,0xfc,0x00,0xfb,0x0e,0x1f,0xbb,0x07,0x00,0xbe,
		0x76,0x7c,0x90,0x8a,0x04,0x46,0x3c,0x00,0x74,0x08,0xb4,0x0e,0x56,0xcd,0x10,0x5e,
		0xeb,0xf1,0xb4,0x01,0xcd,0x16,0x74,0x06,0xb4,0x00,0xcd,0x16,0xeb,0xf4,0xb4,0x00,
		0xcd,0x16,0x33,0xd2,0xcd,0x19,0x0d,0x0a,0x54,0x68,0x69,0x73,0x20,0x64,0x69,0x73,
		0x6b,0x20,0x63,0x61,0x6e,0x27,0x74,0x20,0x62,0x6f,0x6f,0x74,0x3a,0x20,0x69,0x74,
		0x20,0x77,0x61,0x73,0x0d,0x0a,0x66,0x6f,0x72,0x6d,0x61,0x74,0x74,0x65,0x64,0x20,
		0x77,0x69,0x74,0x68,0x6f,0x75,0x74,0x20,0x74,0x68,0x65,0x20,0x2f,0x53,0x20,0x28,
		0x73,0x79,0x73,0x74,0x65,0x6d,0x29,0x20,0x6f,0x70,0x74,0x69,0x6f,0x6e,0x2e,0x0d,
		0x0a,0x0a,0x54,0x6f,0x20,0x6d,0x61,0x6b,0x65,0x20,0x69,0x74,0x20,0x62,0x6f,0x6f,
		0x74,0x61,0x62,0x6c,0x65,0x2c,0x20,0x75,0x73,0x65,0x20,0x74,0x68,0x65,0x20,0x44,
		0x4f,0x53,0x20,0x75,0x74,0x69,0x6c,0x69,0x74,0x79,0x20,0x53,0x59,0x53,0x20,0x78,
		0x3a,0x0d,0x0a,0x0a,0x43,0x68,0x61,0x6e,0x67,0x65,0x20,0x64,0x69,0x73,0x6b,0x73,
		0x20,0x26,0x20,0x70,0x72,0x65,0x73,0x73,0x20,0x61,0x20,0x6b,0x65,0x79,0x2e,0x0d,
		0x0a,0x0a};
	struct fat16_boot n={
		.ignored={0xeb,0x3e,0x90},
		.system_id="HP16500B",
		//.sector_size=32768,
		.media=0xFA,
		/*.heads=16,16  */ 
		/*.hidden=0, */
		/*.total_sect=0,49266 */
		.logicaldrive=0,/**0  */
		.extended_sig=0x29,
		//.serial=0x23787f78,
		.volname="B16500     ",
		.fatname="FAT16   ",
		.marker={0x55,0xaa},	};
	/**sanity checks  */
	if( strncmp((const char *)n.marker,(const char *)f->marker,2) || fd<3){
		printf("Sanity Checks failed.\n");
		return 1;
	}
	
	n.hidden=f->hidden;
	n.heads=f->heads;
	n.fats=f->fats;
	n.dir_entries=f->dir_entries;
	n.sector_size=f->sector_size;
	/**init new struct  */	
	memset(n.exe,0,sizeof(n.exe));
	for (i=0; i<sizeof(exe);++i)
		n.exe[i]=exe[i];
	
	div=f->sector_size/512;
  tsec=f->sectors;
	if(0 == tsec){
		tsec=f->total_sect;
	}
	
	if(f->sector_size != 32768){
		div=32769/f->sector_size;
		n.sector_size=32768;
		if(0 == (n.sec_per_clus=check_div("sec_per_clus",f->sec_per_clus,div)) )
			goto err_num;
		
		if(0 == (n.sectors=check_div("Sectors",tsec,div)) )
			goto err_num;
		
		if(0 == (n.reserved=check_div("Reserved",f->reserved,div)) )
			goto err_num;
		
		if(0 == (n.secs_track=check_div("Sectors per Track",f->secs_track,div)) )
			goto err_num;
		
		if(f->fat_length){
			if(0 == (n.fat_length=check_div("Fat Length",f->fat_length,div)) )
				goto err_num;
			}	else
				n.fat_length=f->fat_length;
		printf("Modified the following:\n"
		"Val       Old      New\n"
		"Sectors    %d      %d\n"
		"sect/clust %d      %d\n"
		"sect/track %d      %d\n"
		"reserved   %d      %d\n"
		"fat length %d      %d\n",
		f->sectors,n.sectors,
		f->sec_per_clus,n.sec_per_clus,
		f->secs_track,n.secs_track,
		f->reserved,n.reserved,
		f->fat_length,n.fat_length);
		
		goto good;
	} else
		printf("Sector size already 32768. Not modifying numbers.\n");
err_num: /**don't modify the values or it'll hose. Instead, do everything else.  */		
	printf("Not modifying block\n");
	n.sector_size=f->sector_size;
	n.sec_per_clus=f->sec_per_clus;
	n.sectors=f->sectors;
	n.total_sect=f->total_sect;
	n.reserved=f->reserved;
	n.fat_length=f->fat_length;
	n.secs_track=f->secs_track;
	n.hidden=f->hidden;
	return 1;
good:	/**number crunching done, write static  */
	lseek(fd,0,SEEK_SET);
	if(swap){
		if(swap_buffer((char *)&n,512)){
			printf("Swap buffer failed just before write. Abort write.\n");
			return 1;
		}
	}	
	write(fd,(void *)&n,512); 
	return 0;
	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("readpartition <options>\n"
	
	" -d /path/to/dev set device to read/write\n"
	" -h this screen\n"
	" -s set swap bytes on\n"
	" -w After reading drive, write the modified boot block back.\n"
	"\n");
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char *argv[])
{
	int c,devf,swap, bytes_to_read,rw,mode;	
	char *dev;
	char *buf;
	rw=swap=0;
	dev=NULL;
	bytes_to_read=BYTES_TO_READ;
	
	mode=O_RDONLY;
		
	while( -1 != (c = getopt(argc, argv, "d:hsw")) ) {
		switch(c){
			case 'h':
				usage();
				return 1;
			case 'd':
				dev=strdup(optarg);
				break;
			case 's':
				swap=1;
				break;
			case 'w':
				rw=1;
				mode=O_RDWR;
				break;
		}
	}
	if(NULL == dev){
		usage();
		return 1;
	}
	if( swap && (bytes_to_read%2)){
		printf("With swap, even number of bytes must be used (%d).",bytes_to_read);
		return 1;
	}
		
	if(NULL ==(buf=malloc(bytes_to_read+2))){
		printf("Unable to allocate for %d bytes\n",bytes_to_read);
		return 1;
	}
	if((devf=open(dev,mode))<0){
		if( EPERM == devf)
			printf("You lack sufficient permissions to open '%s'. Perhaps use sudo?\n",dev);
		printf("Unable to open '%s'\n",dev);
		return 1;
	}
	if((c=read(devf,buf,bytes_to_read))!= bytes_to_read){
		printf("Unable to read %d bytes",bytes_to_read);
		return 1;
	}
	if( swap){
		if(swap_buffer(buf,bytes_to_read)){
			printf("swap buffer failed.\n");
			return 1;
		}
	}
	display_fat16(buf,bytes_to_read);
	if(rw)	{
		write_new_boot(buf,bytes_to_read,devf,swap);
	}
	close(devf);
	return 0;
}
