/** \file ******************************************************************
\n\b File:        tek2gplot.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/05/2007  1:12 am
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.4  2008/08/02 08:54:43  dfs
Attempt to add yrange adjustment

Revision 1.3  2007/10/05 20:35:44  dfs
Added -l and -t options

Revision 1.2  2007/10/05 14:45:56  dfs
Added trigger, plot title, and fixed y-axis label

Revision 1.1.1.1  2007/10/05 13:59:11  dfs
Initial Creation

*/
#define _GNU_SOURCE
/*#define _ISOC99_SOURCE  */
/*#define _XOPEN_SOURCE=600   */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>


#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include "common.h"
/**build with 
gcc -Wall -lm -o tek2plot tek2gplot.c
  */
/**
WFMPRE WFID:"CH1 DC   5mV 500ms NORMAL",NR.PT:1024,PT.OFF:128,PT.FMT:Y,XUNIT:SEC
,XINCR:1.000E-2,YMULT:2.000E-4,YOFF:-2.625E+1,YUNIT:V,BN.FMT:RI,ENCDG:ASCII;CURV
E  */

/***************************************************************************/
/** Load everything before curve into a buffer and set offset to first data
point.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char *load_preamble(int fd, int *offset)
{
	char buf[256], *end;
	int i,x,k;
	i=read(fd,buf,200);
	for (x=k=0; k<i;++k){
		while(buf[x]== '\n' || buf[x] == '\r'){
			++x;
			
		}
		buf[k]=buf[x++];
	}
	
	end=strstr(buf,"CURVE");
	if(NULL == end)	{
		*offset=-1;
		buf[i]=0;
		printf("preamble=%s\n",buf);
		return NULL;
	}
	i=end-buf;
	buf[i]=0;
	*offset=i+6+(x-k);
	lseek(fd,*offset,SEEK_SET);
	return (strdup(buf));
	
}
#define BUF_LEN 1024
/***************************************************************************/
/** use internal buffered file I/O to read next value.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_next_value (int fd, char *dst)
{
	static char buf[BUF_LEN+1];
  static int pos, len=0;
  int i, dbg=3;
  if( NULL ==dst)
    return -1;

  /**read in to fill our buffer.  */
  if( 0 == len || pos >= len) {
    len=read(fd,buf,BUF_LEN);
    pos=0;
  }
  if( len == 0 ){
    if(dbg>2)printf("get_nex_value: len is 0\n");
    return 0;
  }

  else if( len == -1)
    return -1;
  else if( len > BUF_LEN) {
    if(dbg)printf("Read returned more than buf size!\n");
    return(0);
  }
  /*printf("Looking at line (buf %d long).\n",len-pos); */
  for (i=0;buf[pos] != ',' && len;){
		if(buf[pos] != '\n' && buf[pos] != '\r')
    	dst[i++]=buf[pos++];
		else ++pos;
    if(pos >= len) {
      /*printf("Ran out of input, reading again.\n"); */
      len=read(fd,buf,BUF_LEN);
      pos=0;
    }

  }
  /**get rid of \n or \r  */
  while( ('\n' == buf[pos] || buf[pos] == '\r' ||',' == buf[pos]) && len) {
    ++pos;
    if(pos >= len) {
      /*printf("Ran out of input looking at term, reading again.\n"); */
      len=read(fd,buf,BUF_LEN);
      pos=0;
      if( len == -1)
        return -1;
    }
  }
	/*printf("pos=%d ",pos); */
  dst[i]=0;
  /*printf("Got %d bytes\n",i); */
  return(i);	
}


struct extended {
	char *src;
	char *coupling;
	int vert_div;
	char *vert_units;
	double vert_mult;
	int time_div;
	char *time_units;
	double time_mult;
};

enum {
	_SRC,
	_COUPLE,
	_VERT,
	_HORIZ,
	_MODE
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:   CH1 DC   5mV 500ms NORMAL"
****************************************************************************/
int break_extended(char *data, struct extended *x)
{
	int k,i,state;
	char buf[10];
	state=_SRC;
	/*printf("%s\n",data); */
	memset(x,0,sizeof(struct extended));
	for (i=k=0;data[i];){
		if(data[i] != '"' && data[i] != ' ')
			buf[k++]=data[i++];
		else if(data[i] == ' '){
			buf[k]=0;
			while (data[i] == ' ') ++i;
			/*printf("%s\n",buf); */
			switch(state){
				case _SRC:
					x->src=strdup(buf);
					++state;
					break;
				case _COUPLE:
					x->coupling=strdup(buf);
					++state;
					break;
				case _VERT:
					x->vert_div=strtol(buf,NULL,10);
					for (k=0;buf[k];++k){
						if(buf[k] >'9' || buf[k] < '0')
							break;
					}
					x->vert_units=strdup(&buf[k]);
					if(!strcmp(x->vert_units,"mV"))
							x->vert_mult=1000;
					else 
						x->vert_mult=1;
					/*printf("Vert:%d%s %E\n",x->vert_div,x->vert_units,x->vert_mult); */
					++state;
					break;
				case _HORIZ:
					x->time_div=strtol(buf,NULL,10);
					for (k=0;buf[k];++k){
						if(buf[k] >'9' || buf[k] < '0')
							break;
					}
					x->time_units=strdup(&buf[k]);
					if(!strcmp(x->time_units,"ms"))
							x->time_mult=1000;
					else if(!strcmp(x->time_units,"us"))
							x->time_mult=1000000;
					else if(!strcmp(x->time_units,"ns"))
							x->time_mult=1000000000;					
					else if(!strcmp(x->time_units,"ps"))
							x->time_mult=1000000000000;
					else 
						x->time_mult=1;					
					/*printf("time:%d%s %E\n",x->time_div,x->time_units,x->time_mult); */
					++state;
					break;
				default:
					return 0;
			}
			k=0;
		}
		else
			++i;
		
	}
	return 1;
}

/*encoding formats:
ASCII, (Signed integer)	 RI
RIBINARY,  (Signed integer)	RI
RPBINARY,  (Positive integer)	RP
RIPARTIAL, (Signed integer - partial) 
RPPARTIAL. (Positive integer -partial)

*/

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage( void)
{
	printf("tek2gplot: $Revision: 1.5 $\n"
	" -h This display\n"
	" -g generate a gnuplot script file with data (default just data)\n"	
	" -i fname use fname for input file\n"
	" -l color set line color in 0xRRGGBB format \n"
	" -o fname use fname for output file\n"
	" -s val Use smoothing with value val\n"
	" -t term, set terminal to png, ascii, or x11 (default is x11)\n"
	"if you use -t png, gnuplot usage is:\n"
	" gnuplot fname > image.png\n" \
	);
	
	
}
#define TERM_X 		1
#define TERM_PNG 	2
#define TERM_DUMB 3
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/

int main (int argc, char *argv[])
{
	char *iname,*oname, *preamble, buf[256], obuf[50], *line_color;
	char *fmt, *enc, *title;
	int id, od, ret=1, c, data;
	int sgn,gnuplot,smoothing, datapoint, terminal;
	double yoff, ymult, xincr, x,y, trigger,ytrig,xtrig;
	struct extended ext;
	int yrange_plus, yrange_minus, xrange, trig,offset;
	uint32_t ilinecolor;
	iname=oname=NULL;
	gnuplot=0;
	smoothing=0;
	terminal=TERM_X;
	line_color="x00ff00";
	while( -1 != (c = getopt(argc, argv, "i:l:o:gs:t:")) ) {
		switch(c){
			case 'g':
				gnuplot=1;
				break;
			case 'h':
				usage();
				return 0;
				break;
			case 'i':
				iname=strdup(optarg);
				break;
			case 'l':
				line_color=strdup(&optarg[1]);
				break;
			case 'o':
				oname=strdup(optarg);
				break;	
			case 's':
				smoothing=strtol(optarg,NULL,10);
				break;
			case 't':
				if(!strcmp(optarg,"png"))
					terminal=TERM_PNG;
				else if(!strcmp(optarg,"x11"))
					terminal=TERM_X;
				else if(!strcmp(optarg,"ascii"))
					terminal=TERM_DUMB;				
				else {
					printf("Must specify X or png for -t option\n");
					usage();
					return 1;
				}
				break;
		}
	}
	if(TERM_X == terminal ){
		sprintf(buf,"0%s",line_color);
		ilinecolor=strtol(buf,NULL,16);
		switch(ilinecolor){
			case 0xff0000: /**red  */
				ilinecolor=1;
				break;
			case 0x00FF00: /*green */
				ilinecolor=2;
				break;                           
			case 0x0000ff:  /*blue  */
				ilinecolor=3;                  
				break;	                       
			case 0xFF00FF:  /*magenta */
				ilinecolor=4;                  
				break;                         
			case 0x00FFFF:  /*cyan */
				ilinecolor=5;
				break;
			case 0xA0522D: /*sienna */
				ilinecolor=6;
				break;
			case 0xffa500: /**orange */
				ilinecolor=7;
				break;	
			case 0xFF7F50: /*coral */
				ilinecolor=8;
				break;				
			default:
				ilinecolor=2;
				break;
		}
	}
	else ilinecolor=2;
	if( NULL == iname || NULL == oname){
		printf("Must supply at least -o, -i\n");
		usage();
		return 1;
	}
	if(1>(od=open(oname,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
		printf("Unable to open %s\n",oname);
		goto end;
	}	
	if(1>(id=open(iname,O_RDONLY)) ){
		printf("Unable to open %s\n",argv[1]);
		goto closeod;
	}
	if(NULL == (preamble=load_preamble(id, &data))){
		printf("Unable to load preamble\n");
		goto closeid;
	}
	/**PT.OFF  */
	yoff=get_value("YOFF", preamble);
	ymult=get_value("YMULT",preamble);
	xincr=get_value("XINCR", preamble);
	fmt=get_string("BN.FMT",preamble);
	enc=get_string("ENCDG",preamble);
	trigger=get_value("PT.OFF",preamble);
	title=get_string("WFID",preamble);
	if(!strcmp("RI",fmt))
		sgn=1;
	else if(!strcmp("RP",fmt))
		sgn=0;
	else {
		printf("Unknown format %s\n",fmt);
		goto closeid;
	}
	
	if(strcmp(enc,"ASCII")){
		printf("Encoding %s not supportd\n",enc);
		goto closeid;
	}
	
	printf("off=%E mult=%E inc=%E\n",yoff,ymult,xincr);
	break_extended(title,&ext);
	printf("Total time = %f %f points per division\n",xincr*1024,round((ext.time_div/(ext.time_mult))/xincr));
	/*yrange=( ((int)round(ext.vert_div*8))+yoff<0?(int)(yoff*-1):(int)yoff)>>1; */
	yrange_plus=yrange_minus=((int)round(ext.vert_div*8))>>1;
	if(yoff >=0)
		yrange_minus+=round(yoff);
	else
		yrange_plus+=round(-yoff);
	
	/**now take it to the closest volt.  */
	if(yrange_minus%1000)
		yrange_minus+=1000;
	if(yrange_plus%1000)
		yrange_plus+=1000;
	printf("yrange=[-%d:%d] in %s ",yrange_minus,yrange_plus,ext.vert_units);
	xrange=(int)round(xincr*1024*ext.time_mult);
	trig=(int)round(trigger);
	printf("Xrange=[0:%d] in %s\n",xrange,ext.time_units);
	printf("set xtics 0,%d\n",(int)round(ext.time_div));
	x=0;
	if(gnuplot){
		c=sprintf(buf,"set xlabel \"%s\"\n",ext.time_units);
		write(od,buf,c);
		c=sprintf(buf,"set lmargin 10\n");
		write(od,buf,c);
		c=sprintf(buf,"set label 2 \"%s\" at graph -0.06, graph .5\n",ext.vert_units);
		write(od,buf,c);
		c=sprintf(buf,"set yrange[%d:%d]\n",-yrange_minus,yrange_plus);
		write(od,buf,c);
		c=sprintf(buf,"set xrange[0:%d]\n",xrange);
		write(od,buf,c);
		c=sprintf(buf,"set xtics 0,%d\n",(int)round(ext.time_div));
		write(od,buf,c);
		c=sprintf(buf,"set title %s\n",title);
		write(od,buf,c);
		for (data=1; data<9;++data){
			c=sprintf(buf,"set style line %d lt %d lw 2.000 pointtype 4\n",data,data);
			write(od,buf,c);	
		}
		
		switch(terminal){
			/**
			set terminal png medium size 640,480 \
                      xffffff x000000 x404040 \
                      xff0000 xffa500 x66cdaa xcdb5cd \
                      xadd8e6 x0000ff xdda0dd x9500d3 
			which uses white for the non-transparent background, black for borders, 
			gray for the axes, and red, orange, medium aquamarine, thistle 3, 
			light blue, blue, plum and dark violet for eight plotting colors
			*/
			case TERM_PNG:
				c=sprintf(buf,"set terminal png medium size 640,480 \\\n"
                      "xffffff x000000 x404040 \\\n"
                      "xff0000 %s x66cdaa xcdb5cd \\\n"
                      "xadd8e6 x0000ff xdda0dd x9500d3\n",line_color );
				write(od,buf,c);
				break;
			/**  
			gnuplot*background:  white
		  gnuplot*textColor:   black
		  gnuplot*borderColor: black
		  gnuplot*axisColor:   black
		  gnuplot*line1Color:  red
		  gnuplot*line2Color:  green
		  gnuplot*line3Color:  blue
		  gnuplot*line4Color:  magenta
		  gnuplot*line5Color:  cyan
		  gnuplot*line6Color:  sienna
		  gnuplot*line7Color:  orange
		  gnuplot*line8Color:  coral
				*/
			case TERM_X:
				c=sprintf(buf,"set terminal x11 title \"%s\" persist\n",ext.src);
				write(od,buf,c);
				break;
			case TERM_DUMB:
				c=sprintf(buf,"set terminal dumb\n");
				write(od,buf,c);
				break;
		}
		c=sprintf(buf,"set label 1 'T' at ");
		write(od,buf,c);
		offset=lseek(od,0,SEEK_CUR);
		printf("offset at %d\n",offset);
		/**save space for rest of line..  */
		c=sprintf(buf,"																					");
		write(od,buf,c);
		c=sprintf(buf,"																					\n");
		write(od,buf,c);
		
		if(smoothing){
			c=sprintf(buf,"set samples %d\n",smoothing);
			write(od,buf,c);		
			c=sprintf(buf,"plot '-' title \"%s\" ls %d smooth csplines with lines\n",ext.src,ilinecolor);
			write(od,buf,c);			
		}
		else{
			c=sprintf(buf,"plot '-' title \"%s\" ls %d with lines\n",ext.src,ilinecolor);
			write(od,buf,c);			
		}

		
	}
	for (datapoint=0; datapoint<1024 && buf[0];++datapoint) {
		if(1> get_next_value(id,buf))	
			goto closeid;
		y=strtof(buf, NULL);
		if(0 == sgn)
			y=y-128;
		y=(y - yoff)*ymult*ext.vert_mult;
		/*printf("%E %E (%s)\n",x,y,buf); */
		c=sprintf(obuf,"%E %E\n",ext.time_mult*x,y);
		write(od,obuf,c);
		x+=xincr;
		if(datapoint==trig){
			ytrig=y;
			xtrig=ext.time_mult*x;
		}
			
	}
	lseek(od,offset,SEEK_SET);
	c=sprintf(buf,"%E,%E front",xtrig,ytrig);
	write(od,buf,c);
	ret=0;
	
	
closeid:
	close(id);
closeod:
	close(od);
end:
	return (ret);
	
}

/**gnuplot notes:
use -persist to leave plot up when gnuplot exits
8 Vert divs 10 horz divs
  set yrange [-20:20] (for 5V/div)
  set xtics 0,20e-9
  set samples 300
  plot 't5' ls 4 smooth csplines with lines
  set style line 4 linetype 2 linewidth 2.000 pointtype 4 pointsize default
  set style line 1 lt 2 lw 2.000 pt 4 ps default
  set style line 4 lt 2 lc rgb "green" lw 2
  plot cos(x)     ls 4 title 'ls 4'
  plot "-"

		printf("set xlabel \"%s\"\n",ext.time_units);
		printf("set ylabel \"%s\"\n",ext.vert_units);
  set label 1 'Y-AXIS' at graph -0.2, graph 0.5


2440/ProLogix notes:
DATA SOURCE: REF1-4 CH1-2
DATA ENCDG:ASCI
WAV?
++read

  
  */

