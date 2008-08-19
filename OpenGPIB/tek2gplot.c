/** \file ******************************************************************
\n\b File:        tek2gplot.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/05/2007  1:12 am
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.9  2008/08/18 22:09:26  dfs
Added time cursor handling, fixed vert scaling, fixed horiz printing

Revision 1.8  2008/08/12 23:04:00  dfs
Added diff function

Revision 1.7  2008/08/03 23:31:59  dfs
Added cursor plotting

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

#define FUNCTION_NONE 0
#define FUNCTION_DIFF 1
#define FUNCTION_

#define BUF_LEN 		1024
#define MAX_POINTS 	2048

#define WR_SINGLE_PLOT 0
#define WR_MULTI_PLOT  2
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
	double time_div;
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
struct _data_point {
	double x;
	double y;
};
struct data_point {
	struct _data_point data[MAX_POINTS];
	int idx;
};
struct data_point data_pt[MAX_INPUTFILES];

struct trace_data {
	struct extended info;
	struct data_point data_pt;
	double ymult;
	double xincr;
	double trigger;
	double yoff;
	char *fmt;
	char *enc;
	char *title;
	char *label;
	char *iname;
  char *oname;
};

struct plot_data {
	int max_yrange_plus;
	int max_yrange_minus;
	int idx;
	double maxx;
	double minx;
	double maxy;
	double miny;
	double xtrig;
	double ytrig;
	double yoff_max;
	char *trigname;
	char *cursorfile;
	int terminal;
	struct trace_data trace[MAX_INPUTFILES];
	
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
\n\b Returns:
****************************************************************************/
double get_multiplier (char *s)
{
	if(!strcmp(s,"ms"))
			return 1000;
	else if(!strcmp(s,"us"))
			return 1000000;
	else if(!strcmp(s,"ns"))
			return 1000000000;					
	else if(!strcmp(s,"ps"))
			return 1000000000000;
	return 1;					
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
					x->time_div=strtof(buf,NULL);
					for (k=0;buf[k];++k){
						if(buf[k] >'9' || buf[k] < '0')
							break;
					}
					x->time_units=strdup(&buf[k]);
					x->time_mult=get_multiplier(x->time_units);
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
\n\b Returns:
****************************************************************************/
int get_linecolor( char *rgb)
{
	unsigned long ilinecolor;
	char buf[256];
	if(NULL == rgb)
		return 2;
	sprintf(buf,"0%s",rgb);
	ilinecolor=strtol(buf,NULL,16);
	switch(ilinecolor){
		case 0xff0000: /**red  */
			return 1;
			break;
		case 0x00FF00: /*green */
			return 2;
			break;                           
		case 0x0000ff:  /*blue  */
			return 3;                  
			break;	                       
		case 0xFF00FF:  /*magenta */
			return 4;                  
			break;                         
		case 0x00FFFF:  /*cyan */
			return 5;
			break;
		case 0xA0522D: /*sienna */
			return 6;
			break;
		case 0xffa500: /**orange */
			return 7;
			break;	
		case 0xFF7F50: /*coral */
			return 8;
			break;				
	}
	return 2;
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
	int ilinecolor;

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
	if(WR_CURSORS == plotno){/**write cursors  */
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

	if(plotno <WR_MULTI_PLOT ){
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
		c=sprintf(buf,"set xtics 0,%f\n",ext->time_div);
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
			ilinecolor=get_linecolor(ext->line_color);
		}
		else ilinecolor=2;
		c=sprintf(buf,"set label 1 'T' at ");
		write(od,buf,c);
		offset=lseek(od,0,SEEK_CUR);
		/*printf("offset at %d\n",offset); */
		/**save space for rest of line.. and possible cursor title */
		c=sprintf(buf,"																					                  ");
		write(od,buf,c);
		c=sprintf(buf,"																					                 ");
		write(od,buf,c);
		c=sprintf(buf,"																					                 \n");
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
	printf("tek2gplot: $Revision: 1.10 $\n"
	" -c channelfname Set the channel no for the trigger file name. i.e. \n"
	"    which channel is trigger source. This must match an -i.\n"
	"    if this is not set, the first file is assumed to have the trigger\n"
	" -d cursorname Show the cursor info from the cursors file\n"
	" -f function Add another plot with the following functions: diff\n"
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
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int save_data_point(struct trace_data *t, double x, double y)
{
	if(t->data_pt.idx>=MAX_POINTS)
		return -1;
	t->data_pt.data[t->data_pt.idx].x=x;
	t->data_pt.data[t->data_pt.idx].y=y;
	++t->data_pt.idx;
	return 0;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int check_xrange (char *title, struct extended *e)
{
	int i;
	/**change range if over 3 digits  */
	i=strlen(title);
	if(e->time_div >100){
		int x,s,j;
		char b[12];
		/*printf("mult=%f Timeb=%.2f ",e->time_mult,e->time_div); */
		e->time_mult=e->time_mult/1000;
		x=round(e->time_mult);
		switch(x){
			case 1: strcpy(e->time_units,"s"); break;
  			case 1000: strcpy(e->time_units,"ms"); break;
  			case 1000000: strcpy(e->time_units,"us"); break;				
  			case 1000000000: strcpy(e->time_units,"ns"); break;
  			default: printf("Don'k know how to handle %f multiplier\n",e->time_mult); return 1;
		}
		
		e->time_div=e->time_div/1000;
		/*printf("TimeA=%.2f Title='%s'\n",e->time_div,title); */
		
		for (s=x=0; x<i && s<3;++x){
			if(' ' == title[x])
				++s;
		}
		s=x;/**first char we can write  */
		while(title[x] && ' ' != title[x])
			++x;
		/*printf("s=%d, x=%d, new units=%s\n",s,x,e->time_units); */
		/**FIXME add check here for size so we don't overwrite existing string.  */
		j=sprintf(b,"%2g%s",e->time_div,e->time_units);
		if(j>x-s){ /**realloc the string  */
			char *p;
			j+=i+5;
			if(NULL ==(p=malloc(j)) ){
				printf("OUt of mem on alloc of %d\n",j);
				return 1;
			}
			title[s]=0;
			sprintf(p,"%s %s %s",title,b,&title[x]);
			free (title);
			title=p;
		}	else {/**it fits, just use old string  */
			for (i=0;b[i] && s<x;++i,++s)
				title[s]=b[i];	
		}
		/*sprintf(&title[s],"%.2f%s",e->time_div,e->time_units); */
	}
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int build_oname(struct trace_data *t, char *oname)
{
	char *in,*bn;
	in=strdup(t->iname); /**strdup since basename can mod path  */
	bn=basename(in);
	
	if(NULL == (t->oname=malloc(strlen(bn)+strlen(oname)+10)) ){
		printf("Out of memory for bm&on\n");
		return 1;
	}
	sprintf(t->oname,"%s.%s",oname,bn);
	free (in);
	return 0;
}
/***************************************************************************/
/** When gnuplot is specified, there is just one file, the script file, so
write_script_file is used for all writing.
\n\b Arguments:
\n\b Returns:
****************************************************************************/

int main (int argc, char *argv[])
{
	char *oname, *preamble, buf[BUF_LEN], obuf[50];
	char  titles[BUF_LEN], xtitle[50];
	char *function;
	int id, od, ret=1, c, data, func;
	int sgn,gnuplot, datapoint, multiplot, trig, last_idx;
	double trigger, _xincr;
	struct plot_data plots;
	_xincr=0;
	memset(&plots,0,sizeof(struct plot_data));
	
	function=oname=NULL;
	multiplot=gnuplot=0;
	
	func=0;
	last_idx=0;
	while( -1 != (c = getopt(argc, argv, "c:d:f:gi:ml:o:s:t:")) ) {
		switch(c){
			case 'c':
				plots.trigname=strdup(optarg);
				break;
			case 'd':
				plots.cursorfile=strdup(optarg);
				break;			
			case 'f':
				function=strdup(optarg);
				if(!strcmp("diff",optarg))
					func=FUNCTION_DIFF;
				else{
					printf("Unknown function '%s'\n",optarg);
					return 1;
				}
				break;
			case 'g':
				gnuplot=1;
				break;
			case 'h':
				usage();
				return 0;
				break;
			case 'i':
				if(plots.idx>= MAX_INPUTFILES){
					printf("Exceeded max input files of %d\n",MAX_INPUTFILES);
					return 1;
				}
				plots.trace[plots.idx].info.smoothing=0;
				plots.trace[plots.idx].info.terminal=TERM_X;
				plots.trace[plots.idx].info.line_color="x00ff00";	
				last_idx=plots.idx;
				plots.trace[plots.idx++].iname=strdup(optarg);
				break;
			case 'l':
				plots.trace[last_idx].info.line_color=strdup(&optarg[1]);
				break;
			case 'm':
				multiplot=1;
				break;
			case 'o':
				oname=strdup(optarg);
				break;	
			case 's':
				plots.trace[last_idx].info.smoothing=strtol(optarg,NULL,10);
				break;
			case 't':
				if(!strcmp(optarg,"png"))
					plots.terminal=TERM_PNG;
				else if(!strcmp(optarg,"x11"))
					plots.terminal=TERM_X;
				else if(!strcmp(optarg,"ascii"))
					plots.terminal=TERM_DUMB;				
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
	if( 0 == plots.idx || NULL == oname){
		printf("Must supply at least -o, -i\n");
		usage();
		return 1;
	}
	/*find our max offset*/
	plots.yoff_max=0;
	for (c=0; c<plots.idx;++c){
		if(1>(id=open(plots.trace[c].iname,O_RDONLY)) ){
			printf("Unable to open %s\n",plots.trace[c].iname);
			goto closeod;
		}
		build_oname(&plots.trace[c],oname);
		if(NULL == (preamble=load_preamble(id, &data))){
			printf("Unable to load preamble for file %s\n",plots.trace[c].iname);
			goto closeid;
		}
		plots.trace[c].info.terminal=plots.terminal;
		plots.trace[c].yoff=get_value("YOFF", preamble);
		plots.trace[c].ymult=get_value("YMULT",preamble);
		plots.trace[c].title=get_string("WFID",preamble);
		break_extended(plots.trace[c].title,&plots.trace[c].info);
		plots.trace[c].yoff=plots.trace[c].yoff*plots.trace[c].ymult*plots.trace[c].info.vert_mult;
		if(plots.yoff_max > plots.trace[c].yoff)
			plots.yoff_max=plots.trace[c].yoff;
		close (id);
	}

	/**set the output script name  */
	write_script_file(NULL, oname, NULL, NULL,WR_SETFILENAME);
	
	xtitle[0]=titles[0]=0;
	/*printf("plots.idx=%d\n",plots.idx); */
	for (c=0; c<plots.idx;++c){
		int i, trigfile;
		if(gnuplot)
			trigfile=1;
		else if(NULL == plots.trigname){
			if(0 == c)	/**use first file  */
				trigfile=1;
			else
				trigfile=0;
		}
		else if(!strcmp(plots.trigname,plots.trace[c].iname))
			trigfile=1;
		else
			trigfile=0;
		if(0 == gnuplot){
			if(1>(od=open(plots.trace[c].oname,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
				printf("Unable to open %s\n",plots.trace[c].oname);
				goto end;
			}				
			printf("Writing %s\n",plots.trace[c].oname);
		}else 
			id=0;

		if(1>(id=open(plots.trace[c].iname,O_RDONLY)) ){
			printf("Unable to open %s\n",plots.trace[c].iname);
			goto closeod;
		}
		if(NULL == (preamble=load_preamble(id, &data))){
			printf("Unable to load preamble for file %s\n",plots.trace[c].iname);
			goto closeid;
		}
		/**PT.OFF  */
		plots.trace[c].yoff=plots.yoff_max;
		plots.trace[c].ymult=get_value("YMULT",preamble);
		plots.trace[c].xincr=get_value("XINCR", preamble);
		plots.trace[c].fmt=get_string("BN.FMT",preamble);
		plots.trace[c].enc=get_string("ENCDG",preamble);
		trigger=get_value("PT.OFF",preamble);
		plots.trace[c].title=get_string("WFID",preamble);
		strip_quotes(plots.trace[c].title);

		if(!strcmp("RI",plots.trace[c].fmt))
			sgn=1;
		else if(!strcmp("RP",plots.trace[c].fmt))
			sgn=0;
		else {
			printf("Unknown format %s\n",plots.trace[c].fmt);
			goto closeid;
		}
		
		if(strcmp(plots.trace[c].enc,"ASCII")){
			printf("Encoding %s not supportd\n",plots.trace[c].enc);
			goto closeid;
		}
		
		/*printf("off=%E mult=%E inc=%E\n",plots.trace[c].yoff,plots.trace[c].ymult,plots.trace[c].xincr); */
		break_extended(plots.trace[c].title,&plots.trace[c].info);
		if(0 == _xincr){
			_xincr=plots.trace[c].xincr;
		}
		/**change range if over 3 digits  */
		if(check_xrange(plots.trace[c].title,&plots.trace[c].info))
			return 1;
		
		i=strlen(titles);
		if(0 ==i)
			sprintf(&titles[i],"%s",plots.trace[c].title);
		else 
			sprintf(&titles[i],"\\n%s",plots.trace[c].title);
		/*printf("Total time = %f %f points per division, time_div %f time_mult %f\n",plots.trace[c].xincr*1024,round((plots.trace[c].info.time_div/(plots.trace[c].info.time_mult))/plots.trace[c].xincr),plots.trace[c].info.time_div,plots.trace[c].info.time_mult); */
		i=strlen(xtitle);
		sprintf(&xtitle[i],"%s ",plots.trace[c].info.src);
		plots.trace[c].label=strdup(plots.trace[c].info.src);
		
		/*yrange=( ((int)round(plots.trace[c].info.vert_div*8))+plots.trace[c].yoff<0?(int)(plots.trace[c].yoff*-1):(int)plots.trace[c].yoff)>>1; */
		plots.trace[c].info.yrange_plus=plots.trace[c].info.yrange_minus=((int)round(plots.trace[c].info.vert_div*8))>>1;
		if(plots.trace[c].yoff >=0)
			plots.trace[c].info.yrange_minus+=round(plots.trace[c].yoff);
		else
			plots.trace[c].info.yrange_plus+=round(-plots.trace[c].yoff);
		
		/**now take it to the closest volt.  */
		/** if(plots.trace[c].info.yrange_minus%1000)
			plots.trace[c].info.yrange_minus+=1000;
		if(plots.trace[c].info.yrange_plus%1000)
			plots.trace[c].info.yrange_plus+=1000; */
		/**now add a volt  
		plots.trace[c].info.yrange_plus+=1000;
		plots.trace[c].info.yrange_minus+=1000;*/
		if(plots.max_yrange_plus <plots.trace[c].info.yrange_plus)
			plots.max_yrange_plus =plots.trace[c].info.yrange_plus;
				if(plots.max_yrange_minus <plots.trace[c].info.yrange_minus)
			plots.max_yrange_minus =plots.trace[c].info.yrange_minus;
		/*printf("yrange=[-%d:%d] in %s ",plots.trace[c].info.yrange_minus,plots.trace[c].info.yrange_plus,plots.trace[c].info.vert_units); */
		plots.trace[c].info.xrange=(int)round(plots.trace[c].xincr*1024*plots.trace[c].info.time_mult);
		trig=(int)round(trigger);
		/*printf("Xrange=[0:%d] in %s\n",plots.trace[c].info.xrange,plots.trace[c].info.time_units); */
		/*printf("set xtics 0,%f\n",plots.trace[c].info.time_div); */
		plots.trace[c].info.x=0;
		if(gnuplot){
			if(-1 ==write_script_file(&plots.trace[c].info, "-", titles, xtitle,WR_SINGLE_PLOT)) {
				printf("Script file error\n");
				goto end;
			}
		}
		buf[0]=1;
		for (datapoint=0; datapoint<1024 && buf[0];++datapoint) {
			if(1> get_next_value(id,buf))	
				break;
			plots.trace[c].info.y=strtof(buf, NULL);
			if(0 == sgn)
				plots.trace[c].info.y=plots.trace[c].info.y-128;
			plots.trace[c].info.y=(plots.trace[c].info.y*plots.trace[c].ymult*plots.trace[c].info.vert_mult) - plots.trace[c].yoff;
			if(gnuplot)	/**write data  */
				write_script_file(&plots.trace[c].info, NULL, NULL, NULL,WR_DATA); 
			else {
				/*printf("%E %E (%s)\n",x,y,buf); */
				i=sprintf(obuf,"%E %E\n",plots.trace[c].info.time_mult*plots.trace[c].info.x,plots.trace[c].info.y);
				write(od,obuf,i);	
			}
			if(FUNCTION_NONE != func){
				save_data_point(&plots.trace[c],
					plots.trace[c].info.time_mult*plots.trace[c].info.x,
					plots.trace[c].info.y);
			}
			plots.trace[c].info.x+=plots.trace[c].xincr;
			/**we are on trigger channel, plot it, save it  */
			if(trigfile &&datapoint==trig){
				plots.ytrig=plots.trace[c].info.ytrig=plots.trace[c].info.y;
				plots.xtrig=plots.trace[c].info.xtrig=plots.trace[c].info.time_mult*plots.trace[c].info.x;
			}
		}
		if(gnuplot){ /**write trigger  */
			write_script_file(&plots.trace[c].info,NULL,NULL,0,WR_TRIGGER);
		}	else {
			/*write(od,buf,i);	 */
			close(od);
		}
		close(id);
		id=od=0;
	} /**end for loop  */	
	
	ret=0;
	if(0 == gnuplot)	{
		plots.trace[c].info.yrange_plus=plots.max_yrange_plus;
		plots.trace[c].info.yrange_minus=plots.max_yrange_minus;
		/**write the script file  */
		sprintf(buf,"\"%s\"",titles);
		/**the offset to the trigger is set when c=0  */
		for (c=0; c<plots.idx;++c){
			plots.trace[c].info.src=plots.trace[c].label;
			write_script_file(&plots.trace[c].info,plots.trace[c].oname,buf,xtitle,c+1);	
		}
		plots.trace[c].info.ytrig=plots.ytrig;
		plots.trace[c].info.xtrig=plots.xtrig;
		/**write the trigger.  */
		write_script_file(&plots.trace[c].info,NULL,NULL,0,WR_TRIGGER);	
	}	else
		--c;
	/**Handle functions here, before end of script file  */
	
	if(FUNCTION_NONE != func){
		/**FIX me! We are assuming the first two data files   */
		char *fn;
		int i;
		if(plots.idx<2){
			printf("Have to have two files to use func %d\n",func);
			goto closeid;
		}
		if(NULL == (fn=malloc(strlen(function)+strlen(oname)+10)) ){
			printf("Out of memory for %s&on\n",function);
			goto end;
		}
		sprintf(fn,"%s.%s",oname,function);
		if(1>(od=open(fn,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
			printf("Unable to open %s\n",fn);
			goto end;
		}				
		printf("Writing %s\n",fn);
		
		plots.trace[0].info.src=function;
		write_script_file(&plots.trace[0].info,fn,NULL,NULL,WR_MULTI_PLOT);
		switch(func){
			case FUNCTION_DIFF:
				for (c=0; c< MAX_POINTS && c<plots.trace[0].data_pt.idx && c<plots.trace[1].data_pt.idx;++c){
					double diff;
					diff=plots.trace[0].data_pt.data[c].y-plots.trace[1].data_pt.data[c].y;
					save_data_point(&plots.trace[plots.idx],plots.trace[0].data_pt.data[c].x,diff);
					i=sprintf(buf,"%E %E\n",plots.trace[0].data_pt.data[c].x,diff);
					write(od,buf,i);
				}
				break;
		}
	}	
	
	if(NULL != plots.cursorfile){
		double max,min,diff,center;
		char *func, *target;
		int targ=0;
		if(1>(id=open(plots.cursorfile,O_RDONLY)) ){
			printf("Unable to open %s\n",plots.cursorfile);
			goto closeod;
		}
		read(id,buf,BUF_LEN-1);
		func=get_string("CURSOR",buf);
		max=get_value("MAX",buf);
		min=get_value("MIN",buf);
		diff=get_value("DIFF",buf);
		/** FIXME - add this to get cursor info from: target=get_string("TARGET",buf);*/
		/*printf("Buf=%s\noff %F max %F,min %F,diff %F\n",buf,plots.yoff_max,max,min,diff); */
		memcpy(&plots.trace[plots.idx].info,&plots.trace[targ].info,sizeof(struct extended));
		targ=plots.idx;
		if(!strcmp(func,"VOLTS")){
			max=(max*plots.trace[targ].info.vert_mult)+(-1*plots.yoff_max);
			min=(min*plots.trace[targ].info.vert_mult)+(-1*plots.yoff_max);
			center=((max-min)/2)+min;
			plots.trace[targ].info.y=center;
			plots.trace[targ].info.x=-4;
			diff*=plots.trace[targ].info.vert_mult;
			sprintf(buf,"%d%s",(int)round(diff),plots.trace[targ].info.vert_units);
			plots.trace[targ].info.src=buf;
			write_script_file(&plots.trace[targ].info,NULL,NULL,0,WR_CURSORS);	
			/**now draw the cursors  */
			write_cursor(&plots.trace[targ].info,0,max);
			write_cursor(&plots.trace[targ].info,plots.trace[targ].info.xrange,max);
			write_cursor(&plots.trace[targ].info,plots.trace[targ].info.xrange,min);
			write_cursor(&plots.trace[targ].info,0,min);			
		}	else if(!strcmp(func,"TIME")){
			double len;
			len=(plots.max_yrange_plus+plots.max_yrange_minus)/50;
			plots.trace[targ].info.x=min*get_multiplier(plots.trace[targ].info.time_units);;
			plots.trace[targ].info.y=(plots.max_yrange_plus+len);
			/*printf("diff=%f\n",diff); */
			diff *=get_multiplier(plots.trace[targ].info.time_units);
			/*diff*=1000; */
			sprintf(buf,"%.3f%s",diff,plots.trace[targ].info.time_units);
			plots.trace[targ].info.src=buf;
			write_script_file(&plots.trace[targ].info,NULL,NULL,0,WR_CURSORS);	
			/**now draw the cursors  */   
			len*=2;
			write_cursor(&plots.trace[targ].info,max,plots.max_yrange_plus-len);
			write_cursor(&plots.trace[targ].info,max,plots.max_yrange_plus);
			write_cursor(&plots.trace[targ].info,min,plots.max_yrange_plus);
			write_cursor(&plots.trace[targ].info,min,plots.max_yrange_plus-len); /**-plots.max_yrange_minus  */
		}else{
			printf("Don't know how to handle cursor function '%s'\n",func);
		}
		
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

		printf("set xlabel \"%s\"\n",plots.trace[c].info.time_units);
		printf("set ylabel \"%s\"\n",plots.trace[c].info.vert_units);
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
	Then add this line (based on plots.trace[c].yoff from data).:
'-' title "" ls 4 with lines
0 2810
41 2810
0 2810 
0 2265
41 2265
  
  */

