/** \file ******************************************************************
\n\b File:        tek2gplot.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/05/2007  1:12 am
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.6  2008/08/03 22:22:07  dfs
Added multiple plots, but gnuplot segfaults

Revision 1.5  2008/08/03 06:20:25  dfs
Moved get_string, get_value to common

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
#define MAX_INPUTFILES 10

#define TERM_X 		1
#define TERM_PNG 	2
#define TERM_DUMB 3

#define BUF_LEN 1024

#define WR_SINGLE_PLOT 0
#define WR_CLOSE			 -1
#define WR_TRIGGER     -2
#define WR_DATA				 -3
#define WR_SETFILENAME -4
#define WR_CURSORS     -5

struct extended {
	char *src;
	char *coupling;
	int vert_div;
	char *vert_units;
	double vert_mult;
	int time_div;
	char *time_units;
	char *line_color;
	double time_mult;
	double xtrig;
	double ytrig;
	double x;
	double y;
	int xrange;
	int yrange_minus;
	int yrange_plus;
	int terminal;
	int smoothing;
};

enum {
	_SRC,
	_COUPLE,
	_VERT,
	_HORIZ,
	_MODE
};

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

  else if( len == -1){
		printf("Unable to read file or EOF\n");
		return -1;
	}
    
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
      if( len == -1){
				printf("Unable to read file or EOF\n");		
				return -1;
			}
        
    }
  }
	/*printf("pos=%d ",pos); */
  dst[i]=0;
  /*printf("Got %d bytes\n",i); */
  return(i);	
}

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
	/*memset(x,0,sizeof(struct extended)); can't do this- we have persistent data*/
	x->src=x->coupling=x->vert_units=x->time_units=NULL;
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

/***************************************************************************/
/** .
\n\b Arguments:
if plotno is 1 or above, we do a multi-plot.  If 0, we do a single plot.
if plotno is -1, we close the file
\n\b Returns:
****************************************************************************/
int write_script_file(struct extended *ext, char *dfile, char *title,char *xtitle, int plotno)
{
	char buf[BUF_LEN];
	int c,data;
	static int od=0, first=1;
	static int offset;
	static char *fname;
	uint32_t ilinecolor;

	if( WR_SETFILENAME == plotno){
		sprintf(buf,"%s.plot",dfile);
		fname=strdup(buf);
		return 0;
	}
	if(od<3){	
		if(NULL == fname){
			printf("o filename is NULL\n");
			return -1;
		}
		if(1>(od=open(fname,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
			printf("Unable to open %s\n",fname);
			return -1;
		}	
	}			
	if( WR_CLOSE == plotno){
		if(od>2)
			close(od);
		return 0;
	}
	if(WR_DATA == plotno){	/**write data  */
		c=sprintf(buf,"%E %E\n",ext->time_mult*ext->x,ext->y);
		write(od,buf,c);
		return 0;
	}
	if(WR_TRIGGER == plotno){/**write trigger  */
		off_t o;
		o=lseek(od,0,SEEK_CUR);
		lseek(od,offset,SEEK_SET);
		c=sprintf(buf,"%E,%E front\n",ext->xtrig,ext->ytrig);			
		write(od,buf,c);
		offset=lseek(od,0,SEEK_CUR);
		lseek(od,o,SEEK_SET);
		return 0;
	}
	if(WR_CURSORS == plotno){/**write trigger  */
		off_t o;
		o=lseek(od,0,SEEK_CUR);
		lseek(od,offset,SEEK_SET);
		c=sprintf(buf,"set label 2 '%s' at %E,%E front",ext->src,ext->x,ext->y);			
		write(od,buf,c);
		offset=lseek(od,0,SEEK_CUR);
		lseek(od,o,SEEK_SET);
		c=sprintf(buf,",\\\n'-' title \"\" ls %d with lines\n",ilinecolor);
		write(od,buf,c);			
		++ilinecolor;
		return 0;
	}

	if(plotno <2 ){
		c=sprintf(buf,"set xlabel \"%s\"\n",ext->time_units);
		write(od,buf,c);
		c=sprintf(buf,"set lmargin 10\n");
		write(od,buf,c);
		c=sprintf(buf,"set label 2 \"%s\" at graph -0.06, graph .5\n",ext->vert_units);
		write(od,buf,c);
		c=sprintf(buf,"set yrange[%d:%d]\n",-ext->yrange_minus,ext->yrange_plus);
		write(od,buf,c);
		c=sprintf(buf,"set xrange[0:%d]\n",ext->xrange);
		write(od,buf,c);
		c=sprintf(buf,"set xtics 0,%d\n",(int)round(ext->time_div));
		write(od,buf,c);
		c=sprintf(buf,"set title %s\n",title);
		write(od,buf,c);
		for (data=1; data<9;++data){
			c=sprintf(buf,"set style line %d lt %d lw 2.000 pointtype 4\n",data,data);
			write(od,buf,c);	
		}
		
		switch(ext->terminal){
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
	                     "xadd8e6 x0000ff xdda0dd x9500d3\n",ext->line_color );
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
				c=sprintf(buf,"set terminal x11 title \"%s\" persist\n",xtitle);
				write(od,buf,c);
				break;
			case TERM_DUMB:
				c=sprintf(buf,"set terminal dumb\n");
				write(od,buf,c);
				break;
		}
		if(TERM_X == ext->terminal ){
			sprintf(buf,"0%s",ext->line_color);
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
		c=sprintf(buf,"set label 1 'T' at ");
		write(od,buf,c);
		offset=lseek(od,0,SEEK_CUR);
		printf("offset at %d\n",offset);
		/**save space for rest of line.. and possible cursor title */
		c=sprintf(buf,"																					                  ");
		write(od,buf,c);
		c=sprintf(buf,"																					                 ");
		write(od,buf,c);
		c=sprintf(buf,"																					                 ");
		write(od,buf,c);
		
		if(ext->smoothing){
			c=sprintf(buf,"set samples %d\n",ext->smoothing);
			write(od,buf,c);		
		}		
	}	
	
	if(ext->smoothing){
		c=sprintf(buf,"%s'%s' title \"%s\" ls %d smooth csplines with lines%s",plotno>0?first?"\nplot ":",\\\n":"\nplot ",dfile,ext->src,ilinecolor,plotno>0?"":"\n");
		write(od,buf,c);			
	}
	else{
		c=sprintf(buf,"%s'%s' title \"%s\" ls %d with lines%s",plotno>0?first?"\nplot ":",\\\n":"\nplot ",dfile,ext->src,ilinecolor,plotno>0?"":"\n");
		write(od,buf,c);			
	}
	++ilinecolor;
	first=0;
	return 0;
}
		

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void write_cursor(struct extended *e,double x, double y)
{
	e->x=x;
	e->y=y;
	write_script_file(e,NULL,NULL,0,WR_DATA);	
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
	printf("tek2gplot: $Revision: 1.7 $\n"
	" -c channelfname Set the channel no for the trigger file name. i.e. \n"
	"    which channel is trigger source. This must match an -i.\n"
	"    if this is not set, the first file is assumed to have the trigger\n"
	" -d cursorname Show the cursor info from the cursors file\n"
	" -h This display\n"
	" -g generate a gnuplot script file with data (default just data).\n"	
	"    -g can only be used for one input file.\n"
	" -i fname use fname for input file(s)\n"
	" -l color set line color in 0xRRGGBB format \n"
	" -m Set multi-plot mode. -o will have .plot appended to it\n"
	" -o fname use fname for output file. if multiple -i are used, this name\n"
	"    becomes a base filename with the -i names appended like ofname.ifname\n"
	"    In all cases the script file becomes fname.plot\n"
	" -s val Use smoothing with value val. \n"
	
	" -t term, set terminal to png, ascii, or x11 (default is x11)\n"
	"if you use -t png, gnuplot usage is:\n"
	" gnuplot fname > image.png\n" \
	);
	
	
}

/***************************************************************************/
/** strip double-quotes out of filename.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void strip_quotes(char *n)
{
	int i,len, s, e;
	len=strlen(n);
	s=e=-1;
	for (i=0;i<len;++i){
		if('"' ==n[i]){
			if(-1 ==s)
				s=i;
			else 
				e=i;
		}	
	}
	if(-1 != s && -1 != e)	{
		++s;
		for (i=0;s<len;++i,++s)
			n[i]=n[s];
		n[e-1]=0;
	}else{
		printf("Didn't find full quotes in '%s'\n",n);
	}
		
		
}
/***************************************************************************/
/** When gnuplot is specified, there is just one file, the script file, so
write_script_file is used for all writing.
\n\b Arguments:
\n\b Returns:
****************************************************************************/

int main (int argc, char *argv[])
{
	char *iname[MAX_INPUTFILES],*oname, *preamble, buf[BUF_LEN], obuf[50];
	char *fmt, *enc, *title, titles[BUF_LEN], xtitle[50], *labels[MAX_INPUTFILES];
	char *ofnames[MAX_INPUTFILES], *trigname, *cursorfile;
	int id, od, ret=1, c, data, idx;
	int sgn,gnuplot, datapoint, multiplot, max_yrange_plus, max_yrange_minus;
	double yoff, ymult, xincr, trigger, xtrig, ytrig, yoff_max;
	struct extended ext;
	int trig;
	
	cursorfile=trigname=iname[0]=oname=NULL;
	multiplot=gnuplot=0;
	ext.smoothing=0;
	ext.terminal=TERM_X;
	ext.line_color="x00ff00";
	idx=0;
	while( -1 != (c = getopt(argc, argv, "c:d:gi:ml:o:s:t:")) ) {
		switch(c){
			case 'c':
				trigname=strdup(optarg);
				break;
			case 'd':
				cursorfile=strdup(optarg);
				break;			
			case 'g':
				gnuplot=1;
				break;
			case 'h':
				usage();
				return 0;
				break;
			case 'i':
				if(idx>= MAX_INPUTFILES){
					printf("Exceeded max input files of %d\n",MAX_INPUTFILES);
					return 1;
				}
					
				iname[idx++]=strdup(optarg);
				break;
			case 'l':
				ext.line_color=strdup(&optarg[1]);
				break;
			case 'm':
				multiplot=1;
				break;
			case 'o':
				oname=strdup(optarg);
				break;	
			case 's':
				ext.smoothing=strtol(optarg,NULL,10);
				break;
			case 't':
				if(!strcmp(optarg,"png"))
					ext.terminal=TERM_PNG;
				else if(!strcmp(optarg,"x11"))
					ext.terminal=TERM_X;
				else if(!strcmp(optarg,"ascii"))
					ext.terminal=TERM_DUMB;				
				else {
					printf("Must specify X or png for -t option\n");
					usage();
					return 1;
				}
				break;
		}
	}
	if( multiplot && gnuplot){
		printf("Cannot specify both -m and -g\n");
		return 1;
	}
	if( 0 == idx || NULL == oname){
		printf("Must supply at least -o, -i\n");
		usage();
		return 1;
	}
	/*find our max offset*/
	yoff_max=0;
	for (c=0; c<idx;++c){
		if(1>(id=open(iname[c],O_RDONLY)) ){
			printf("Unable to open %s\n",iname[c]);
			goto closeod;
		}
		if(NULL == (preamble=load_preamble(id, &data))){
			printf("Unable to load preamble for file %s\n",iname[c]);
			goto closeid;
		}
		yoff=get_value("YOFF", preamble);
		ymult=get_value("YMULT",preamble);
		title=get_string("WFID",preamble);
		break_extended(title,&ext);
		yoff=yoff*ymult*ext.vert_mult;
		if(yoff_max > yoff)
			yoff_max=yoff;
		close (id);
	}

	/**set the output script name  */
	write_script_file(NULL, oname, NULL, NULL,WR_SETFILENAME);
	
	xtitle[0]=titles[0]=0;
	max_yrange_minus=max_yrange_plus=0;
	printf("idx=%d\n",idx);
	for (c=0; c<idx;++c){
		char *in,*bn;/**output name, input name, and base name of inputname  */
		int i, trigfile;
		if(gnuplot)
			trigfile=1;
		else if(NULL == trigname){
			if(0 == c)	/**use first file  */
				trigfile=1;
			else
				trigfile=0;
		}
		else if(!strcmp(trigname,iname[c]))
			trigfile=1;
		else
			trigfile=0;
		if(0 == gnuplot){
			in=strdup(iname[c]);
			bn=basename(in);
			
			if(NULL == (ofnames[c]=malloc(strlen(bn)+strlen(oname)+10)) ){
				printf("Out of memory for bm&on\n");
				goto end;
			}
			sprintf(ofnames[c],"%s.%s",oname,bn);
			free (in);
			if(1>(od=open(ofnames[c],O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
				printf("Unable to open %s\n",ofnames[c]);
				goto end;
			}				
			printf("Writing %s\n",ofnames[c]);
		}else 
			id=0;

		if(1>(id=open(iname[c],O_RDONLY)) ){
			printf("Unable to open %s\n",iname[c]);
			goto closeod;
		}
		if(NULL == (preamble=load_preamble(id, &data))){
			printf("Unable to load preamble for file %s\n",iname[c]);
			goto closeid;
		}
		/**PT.OFF  */
		yoff=yoff_max;
		ymult=get_value("YMULT",preamble);
		xincr=get_value("XINCR", preamble);
		fmt=get_string("BN.FMT",preamble);
		enc=get_string("ENCDG",preamble);
		trigger=get_value("PT.OFF",preamble);
		title=get_string("WFID",preamble);
		strip_quotes(title);
		i=strlen(titles);
		if(0 ==i)
			sprintf(&titles[i],"%s",title);
		else 
			sprintf(&titles[i],"\\n%s",title);
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
		i=strlen(xtitle);
		sprintf(&xtitle[i],"%s ",ext.src);
		labels[c]=strdup(ext.src);
		printf("Total time = %f %f points per division\n",xincr*1024,round((ext.time_div/(ext.time_mult))/xincr));
		/*yrange=( ((int)round(ext.vert_div*8))+yoff<0?(int)(yoff*-1):(int)yoff)>>1; */
		ext.yrange_plus=ext.yrange_minus=((int)round(ext.vert_div*8))>>1;
		if(yoff >=0)
			ext.yrange_minus+=round(yoff);
		else
			ext.yrange_plus+=round(-yoff);
		
		/**now take it to the closest volt.  */
		if(ext.yrange_minus%1000)
			ext.yrange_minus+=1000;
		if(ext.yrange_plus%1000)
			ext.yrange_plus+=1000;
		/**now add a volt  
		ext.yrange_plus+=1000;
		ext.yrange_minus+=1000;*/
		if(max_yrange_plus <ext.yrange_plus)
			max_yrange_plus =ext.yrange_plus;
				if(max_yrange_minus <ext.yrange_minus)
			max_yrange_minus =ext.yrange_minus;
		printf("yrange=[-%d:%d] in %s ",ext.yrange_minus,ext.yrange_plus,ext.vert_units);
		ext.xrange=(int)round(xincr*1024*ext.time_mult);
		trig=(int)round(trigger);
		printf("Xrange=[0:%d] in %s\n",ext.xrange,ext.time_units);
		printf("set xtics 0,%d\n",(int)round(ext.time_div));
		ext.x=0;
		if(gnuplot){
			if(-1 ==write_script_file(&ext, "-", titles, xtitle,WR_SINGLE_PLOT)) {
				printf("Script file error\n");
				goto end;
			}
		}
		for (datapoint=0; datapoint<1024 && buf[0];++datapoint) {
			if(1> get_next_value(id,buf))	
				break;
			ext.y=strtof(buf, NULL);
			if(0 == sgn)
				ext.y=ext.y-128;
			ext.y=(ext.y*ymult*ext.vert_mult) - yoff;
			if(gnuplot)	/**write data  */
				write_script_file(&ext, NULL, NULL, NULL,WR_DATA); 
			else {
				/*printf("%E %E (%s)\n",x,y,buf); */
				i=sprintf(obuf,"%E %E\n",ext.time_mult*ext.x,ext.y);
				write(od,obuf,i);	
			}
			ext.x+=xincr;
			/**we are on trigger channel, plot it, save it  */
			if(trigfile &&datapoint==trig){
				ytrig=ext.ytrig=ext.y;
				xtrig=ext.xtrig=ext.time_mult*ext.x;
			}
		}
		if(gnuplot){ /**write trigger  */
			write_script_file(&ext,NULL,NULL,0,WR_TRIGGER);
		}	else {
			/*write(od,buf,i);	 */
			close(od);
		}
		close(id);
		id=od=0;
	} /**end for loop  */	
	ret=0;
	if(0 == gnuplot)	{
		ext.yrange_plus=max_yrange_plus;
		ext.yrange_minus=max_yrange_minus;
		/**write the script file  */
		sprintf(buf,"\"%s\"",titles);
		/**the offset to the trigger is set when c=0  */
		for (c=0; c<idx;++c){
			ext.src=labels[c];
			write_script_file(&ext,ofnames[c],buf,xtitle,c+1);	
		}
		ext.ytrig=ytrig;
		ext.xtrig=xtrig;
		/**write the trigger.  */
		write_script_file(&ext,NULL,NULL,0,WR_TRIGGER);	
	}
	if(NULL != cursorfile){
		double max,min,diff,center;
		char *func;
		if(1>(id=open(cursorfile,O_RDONLY)) ){
			printf("Unable to open %s\n",cursorfile);
			goto closeod;
		}
		read(id,buf,BUF_LEN-1);
		func=get_string("CURSOR",buf);
		max=(get_value("MAX",buf)*1000)+(-1*yoff_max);
		min=(get_value("MIN",buf)*1000)+(-1*yoff_max);
		diff=get_value("DIFF",buf);
		printf("Buf=%s\noff %F max %F,min %F,diff %F\n",buf,yoff_max,max,min,diff);
		center=((max-min)/2)+min;
		ext.y=center;
		ext.x=-4;
		diff*=1000;
		sprintf(buf,"%dmv",(int)round(diff));
		ext.src=buf;
		write_script_file(&ext,NULL,NULL,0,WR_CURSORS);	
		/**now draw the cursors  */
		write_cursor(&ext,0,max);
		write_cursor(&ext,ext.xrange,max);
		write_cursor(&ext,ext.xrange,min);
		write_cursor(&ext,0,min);
	}
	/**close output file  */
	write_script_file(NULL,NULL,NULL,0,WR_CLOSE);
closeid:
	if(id>2)
		close(id);
closeod:
	if(od>2)
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

Todo:
fix gnuplog segfault.

Add cursor input from get_tek_waveform:
	Add 
	set label 2 '545mV' at -4,2525 front
	right after placing trigger.
	Then add this line (based on yoff from data).:
'-' title "" ls 4 with lines
0 2810
41 2810
0 2810 
0 2265
41 2265
  
  */

