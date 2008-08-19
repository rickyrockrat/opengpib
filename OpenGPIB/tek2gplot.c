/** \file ******************************************************************
\n\b File:        tek2gplot.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/05/2007  1:12 am
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.10  2008/08/19 00:41:22  dfs
Moved data to main data struct, functionality same

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

#define CURSOR_XPERCENT 17

#define MAX_INPUTFILES 10

#define TERM_X 		1
#define TERM_PNG 	2
#define TERM_DUMB 3


#define FUNCTION_NONE 0
#define FUNCTION_DIFF 1
#define FUNCTION_BAD -1

struct func_list {
	int min_files;
	int func;
	char *name;
};

struct func_list func_names[]={
	{.name="diff",.func=FUNCTION_DIFF,.min_files=2},
	{.name=NULL,.func=0,.min_files=0},
};

#define BUF_LEN 		1024
#define MAX_POINTS 	2048

#define WR_SINGLE_PLOT 0
#define WR_MULTI_PLOT  2
#define WR_CLOSE			 -1
#define WR_TRIGGER     -2
#define WR_DATA				 -3
#define WR_SETFILENAME -4
#define WR_CURSORS     -5

#define FLAGS_NONE      0
#define FLAGS_GNUPLOT 	1
#define FLAGS_MULTIPLOT 2
#define FLAGS_FUNCTIONS 4

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
	struct _data_point data[MAX_POINTS+1];
	int idx;
};

struct trace_data {
	struct extended info;
	struct data_point data_pt;
	double ymult;
	double xincr;
	double trigger;
	double yoff;
	int sgn; /**signed or unsigned  */
	char *fmt;
	char *enc;
	char *title;
	char *label;
	char *iname;
  char *oname;
};

#define MAX_FUNC_INPUT 4
struct func {
	char *name;	/**name of function  */
	int func;				/**type of function, for convenience  */
	int idx;				/**idx of arrays below  */
	int min_files; /**minimun number of files for function  */
	struct trace_data trace;
	char *input[MAX_FUNC_INPUT+1]; /* filename to operate on. Must match input filename */
	int main_idx[MAX_FUNC_INPUT+1]; /**numerical offset to trace data corresponding to input, above  */
};

struct plot_data {
	int max_yrange_plus;
	int max_yrange_minus;
	int idx;		/**location of data idx.  */
	int ch_idx; /**value of idx before any functions are added.  */
	int f_idx; /**function index, number of functions.  */
	int flags;
	int graphx;	/**size of graph in pixels, for some terminals  */
	int graphy;
	double maxx;
	double minx;
	double maxy;
	double miny;
	double xtrig;
	double ytrig;
	double yoff_max;
	char *trigname;
	char *cursorfile;
	char *basename;
	int terminal;
	struct trace_data trace[MAX_INPUTFILES+1];
	char titles[BUF_LEN+1];
	char xtitle[50];
	struct func function[MAX_INPUTFILES+1];
};

struct cursors {
	double max;
	double min;
	double diff;
	char *target;
	char *trigger;
	char *func;
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
	static int od=0, first=1,labelno=1;
	static int offset, x, y;
	static char *fname;
	int ilinecolor;

	if( WR_SETFILENAME == plotno){
		sprintf(buf,"%s.plot",dfile);
		fname=strdup(buf);
		if(NULL != ext){
			x=round(ext->x);
			y=round(ext->y);	
		}else{
			x=640;
			y=480;
		}
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
		c=sprintf(buf,"set label %d '%s' at %E,%E front",labelno++,ext->src,ext->x,ext->y);			
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
		c=sprintf(buf,"set lmargin 13\nset bmargin 5\n");
		write(od,buf,c);
		c=sprintf(buf,"set label %d \"%s\" at graph -0.15, graph .5\n",labelno++,ext->vert_units);
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
				c=sprintf(buf,"set terminal png medium size %d,%d \\\n"
	                     "xffffff x000000 x404040 \\\n"
	                     "xff0000 %s x66cdaa xcdb5cd \\\n"
	                     "xadd8e6 x0000ff xdda0dd x9500d3\n",x, y,ext->line_color );
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
		c=sprintf(buf,"set label %d 'T' at ",labelno++);
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
	printf("tek2gplot: $Revision: 1.11 $\n"
	" -c channelfname Set the channel no for the trigger file name. i.e. \n"
	"    which channel is trigger source. This must match an -i.\n"
	"    if this is not set, the first file is assumed to have the trigger\n"
	" -d cursorname Show the cursor info from the cursors file\n"
	" -f function Add another plot with the following functions: diff\n"
	"		 This MUST be after all input files. Additional -i will define what\n"
	"    input files the function will operate on. Assumes first two if missing.\n"
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
	"    if you use -t png, gnuplot usage is:\n"
	"    gnuplot fname > image.png\n" \
	" -x pixels Set out (png for now) X size in pixels\n"
	" -y pixels Set out (png for now) Y size in pixels\n"
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
/** look for trace text in loaded data. The label will match the the channel
name in the data.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int find_trace(struct plot_data *p, char *name)
{
	int i;
	for (i=0;i<p->idx;++i){
		if(!strcasecmp(name,p->trace[i].label)){
			/**found it  */
			return i;
		}
	}
		
	return 0;/**default to first file  */
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int read_cursor_file(char *name, struct cursors *c)
{
	int id;
	char buf[BUF_LEN];
	if(1>(id=open(name,O_RDONLY)) ){
		printf("Unable to open %s\n",name);
		return 1;
	}
	read(id,buf,BUF_LEN-1);
	c->func=get_string("CURSOR",buf);
	c->max=get_value("MAX",buf);
	c->min=get_value("MIN",buf);
	c->diff=get_value("DIFF",buf);
	c->target=get_string("TARGET",buf);
	c->trigger=get_string("TRIGGER", buf);
	close (id);
	return 0;
}

/***************************************************************************/
/** See if our current file is the trigger file. Return 1 if it is.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int check_for_trigger_file(struct plot_data *p, int c)
{
	if(p->flags & FLAGS_GNUPLOT)
		return 1;
	else if(NULL == p->trigname){
		if(0 == c)	/**use first file  */
			return 1;
		else
			return 0;
	}
	else if(!strcmp(p->trigname,p->trace[c].iname))
		return 1;
	return 0;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int load_data(struct plot_data *p)
{
	int c,id,i,data;
	double _xincr;
	char buf[BUF_LEN], *preamble;
	p->yoff_max=0;
	for (c=0; c<p->idx;++c){
		int trigfile, trig, dp;
		trigfile=check_for_trigger_file(p,c);
		if(1>(id=open(p->trace[c].iname,O_RDONLY)) ){
			printf("Unable to open %s\n",p->trace[c].iname);
			return 1;
		}
		build_oname(&p->trace[c],p->basename);
		if(NULL == (preamble=load_preamble(id, &data))){
			printf("Unable to load preamble for file %s\n",p->trace[c].iname);
			goto closeid;
		}
		p->trace[c].info.terminal=p->terminal;
		p->trace[c].yoff=get_value("YOFF", preamble);
		/**PT.OFF  */
		p->trace[c].ymult=get_value("YMULT",preamble);
		p->trace[c].xincr=get_value("XINCR", preamble);
		p->trace[c].fmt=get_string("BN.FMT",preamble);
		p->trace[c].enc=get_string("ENCDG",preamble);
		p->trace[c].trigger=get_value("PT.OFF",preamble);
		p->trace[c].title=get_string("WFID",preamble);
		strip_quotes(p->trace[c].title);

		if(!strcmp("RI",p->trace[c].fmt))
			p->trace[c].sgn=1;
		else if(!strcmp("RP",p->trace[c].fmt))
			p->trace[c].sgn=0;
		else {
			printf("Unknown format %s\n",p->trace[c].fmt);
			goto closeid;
		}
		
		if(strcmp(p->trace[c].enc,"ASCII")){
			printf("Encoding %s not supportd\n",p->trace[c].enc);
			goto closeid;
		}
		
		break_extended(p->trace[c].title,&p->trace[c].info);
		p->trace[c].label=strdup(p->trace[c].info.src);
		p->trace[c].yoff=p->trace[c].yoff*p->trace[c].ymult*p->trace[c].info.vert_mult;
		if(p->yoff_max > p->trace[c].yoff)
			p->yoff_max=p->trace[c].yoff;
		if(0 == _xincr){
			_xincr=p->trace[c].xincr;
		}
		/**change range if over 3 digits  */
		if(check_xrange(p->trace[c].title,&p->trace[c].info))
			goto closeid;
		
		i=strlen(p->titles);
		if(0 ==i)
			sprintf(&p->titles[i],"%s",p->trace[c].title);
		else 
			sprintf(&p->titles[i],"\\n%s",p->trace[c].title);
		/*printf("Total time = %f %f points per division, time_div %f time_mult %f\n",p->trace[c].xincr*1024,round((p->trace[c].info.time_div/(p->trace[c].info.time_mult))/p->trace[c].xincr),p->trace[c].info.time_div,p->trace[c].info.time_mult); */
		i=strlen(p->xtitle);
		sprintf(&p->xtitle[i],"%s ",p->trace[c].label);
		
		
		/*yrange=( ((int)round(p->trace[c].info.vert_div*8))+p->trace[c].yoff<0?(int)(p->trace[c].yoff*-1):(int)p->trace[c].yoff)>>1; */
		p->trace[c].info.yrange_plus=p->trace[c].info.yrange_minus=((int)round(p->trace[c].info.vert_div*8))>>1;
		if(p->trace[c].yoff >=0)
			p->trace[c].info.yrange_minus+=round(p->trace[c].yoff);
		else
			p->trace[c].info.yrange_plus+=round(-p->trace[c].yoff);
		
		/**now take it to the closest volt.  */
		/** if(p->trace[c].info.yrange_minus%1000)
			p->trace[c].info.yrange_minus+=1000;
		if(p->trace[c].info.yrange_plus%1000)
			p->trace[c].info.yrange_plus+=1000; */
		/**now add a volt  
		p->trace[c].info.yrange_plus+=1000;
		p->trace[c].info.yrange_minus+=1000;*/
		if(p->max_yrange_plus <p->trace[c].info.yrange_plus)
			p->max_yrange_plus =p->trace[c].info.yrange_plus;
		if(p->max_yrange_minus <p->trace[c].info.yrange_minus)
			p->max_yrange_minus =p->trace[c].info.yrange_minus;
		/*printf("yrange=[-%d:%d] in %s ",p->trace[c].info.yrange_minus,p->trace[c].info.yrange_plus,p->trace[c].info.vert_units); */
		p->trace[c].info.xrange=(int)round(p->trace[c].xincr*1024*p->trace[c].info.time_mult);
		trig=(int)round(p->trace[c].trigger);
		/*printf("Xrange=[0:%d] in %s\n",p->trace[c].info.xrange,p->trace[c].info.time_units); */
		/*printf("set xtics 0,%f\n",p->trace[c].info.time_div); */
		p->trace[c].info.x=0;
		buf[0]=1;
		for (dp=0; dp<1024 && buf[0];++dp) {
			if(1> get_next_value(id,buf))	
				break;
			p->trace[c].info.y=strtof(buf, NULL);
			if(0 == p->trace[c].sgn)
				p->trace[c].info.y=p->trace[c].info.y-128;
			p->trace[c].info.y=(p->trace[c].info.y*p->trace[c].ymult*p->trace[c].info.vert_mult) - p->trace[c].yoff;
			save_data_point(&p->trace[c],
					p->trace[c].info.time_mult*p->trace[c].info.x,
					p->trace[c].info.y);
			p->trace[c].info.x+=p->trace[c].xincr;
			/**we are on trigger channel, plot it, save it  */
			if(trigfile &&dp==trig){
				p->ytrig=p->trace[c].info.ytrig=p->trace[c].info.y;
				p->xtrig=p->trace[c].info.xtrig=p->trace[c].info.time_mult*p->trace[c].info.x;
			}
		}	/**end data points  */
		close(id);
		id=0;
	}	 /**END of data load  */
	return 0;
closeid:
	if(id>2)
		close(id);
return 1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int find_file(struct plot_data *p, char *name)
{
	static int x=0;
	int i;
	if(NULL == name){
		return x++;
	}
	for (i=0;i<p->idx;++i){
		if(NULL != p->trace[i].iname){
			if(!strcmp(name,p->trace[i].iname))
				return i;
		}
	}
	return x++;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void update_yrange(struct plot_data *p, double y)
{
	if(y>p->max_yrange_plus)
		p->max_yrange_plus=y;
	if(y < -(p->max_yrange_minus))
		p->max_yrange_minus= -y;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int handle_functions(struct plot_data *p)
{
	int i,c,od;
	char buf[50];
	/**first, set up our indexes to the main files (if any)  */
	for (i=0;i<p->f_idx;++i){
		struct func *f;
		f=&p->function[i];
		if(FUNCTION_NONE != f->func){
			/**set up the index into the main array to find parameter info.  */
			for (c=0;c<f->idx;++c){
				f->main_idx[c]=find_file(p,f->input[c]);
				memcpy(&f->trace,&p->trace[f->main_idx[c]],sizeof(struct trace_data));
			}
		
			if(f->idx<f->min_files){
				printf("Have to have %d files to use func %s\n",
						f->min_files,f->name);	
				return 1;
			}
			
			if(NULL == (f->trace.oname=malloc(strlen(f->name)+strlen(p->basename)+10)) ){
				printf("Out of memory for %s&on\n",f->name);
				return 1;
			}
			sprintf(f->trace.oname,"%s.%s",p->basename,f->name);
			if(1>(od=open(f->trace.oname,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
				printf("Unable to open %s\n",f->trace.oname);
				return 1;
			}				
			printf("Writing %s\n",f->trace.oname);
			
			f->trace.info.src=f->name;
			/*write_script_file(&f->trace.info,f->trace.oname,NULL,NULL,WR_MULTI_PLOT); */
			switch(f->func){
				case FUNCTION_DIFF:
					for (c=0; c< MAX_POINTS && c< p->trace[f->main_idx[0]].data_pt.idx && c<p->trace[f->main_idx[1]].data_pt.idx;++c){
						double diff;
						diff=p->trace[f->main_idx[0]].data_pt.data[c].y-p->trace[f->main_idx[1]].data_pt.data[c].y;
						update_yrange(p,diff);
						save_data_point(&f->trace,p->trace[f->main_idx[0]].data_pt.data[c].x,diff);
						i=sprintf(buf,"%E %E\n",p->trace[f->main_idx[0]].data_pt.data[c].x,diff);
						write(od,buf,i);
					}
					break;
			}
			close(od);
		}		
	}
	return 0;
}


/***************************************************************************/
/** Write the cursor out to the plot file.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int plot_cursor(struct plot_data *p)
{
  struct cursors cursor;
	double center;
	int targ;
	char buf[50];
	
	if(NULL != p->cursorfile){
		if(read_cursor_file(p->cursorfile,&cursor)) {
			return 1;
		}
		if(NULL == cursor.target)
			targ=0; /**default to first file  */
		else {
			targ=find_trace(p,cursor.target);
			/*printf("Using %d for target %s\n",targ,cursor.target); */
		}
		/*printf("Buf=%s\noff %F max %F,min %F,diff %F\n",buf,p->yoff_max,max,min,diff); */
		memcpy(&p->trace[p->idx].info,&p->trace[targ].info,sizeof(struct extended));
		targ=p->idx;
		if(!strcmp(cursor.func,"VOLTS")){
			cursor.max=(cursor.max*p->trace[targ].info.vert_mult)+(-1*p->yoff_max);
			cursor.min=(cursor.min*p->trace[targ].info.vert_mult)+(-1*p->yoff_max);
			center=((cursor.max-cursor.min)/2)+cursor.min;
			p->trace[targ].info.y=center;
			p->trace[targ].info.x=-(p->trace[targ].info.xrange*CURSOR_XPERCENT)/100;
			cursor.diff*=p->trace[targ].info.vert_mult;
			sprintf(buf,"%d%s",(int)round(cursor.diff),p->trace[targ].info.vert_units);
			p->trace[targ].info.src=buf;
			write_script_file(&p->trace[targ].info,NULL,NULL,0,WR_CURSORS);	
			/**now draw the cursors  */
			write_cursor(&p->trace[targ].info,0,cursor.max);
			write_cursor(&p->trace[targ].info,p->trace[targ].info.xrange,cursor.max);
			write_cursor(&p->trace[targ].info,p->trace[targ].info.xrange,cursor.min);
			write_cursor(&p->trace[targ].info,0,cursor.min);			
		}	else if(!strcmp(cursor.func,"TIME")){
			double len;
			len=(p->max_yrange_plus+p->max_yrange_minus)/50;
			p->trace[targ].info.x=cursor.min*get_multiplier(p->trace[targ].info.time_units);;
			p->trace[targ].info.y=(p->max_yrange_plus+len);
			/*printf("diff=%f\n",diff); */
			cursor.diff *=get_multiplier(p->trace[targ].info.time_units);
			/*diff*=1000; */
			sprintf(buf,"%.3f%s",cursor.diff,p->trace[targ].info.time_units);
			p->trace[targ].info.src=buf;
			write_script_file(&p->trace[targ].info,NULL,NULL,0,WR_CURSORS);	
			/**now draw the cursors  */   
			len*=2;
			write_cursor(&p->trace[targ].info,cursor.max,p->max_yrange_plus-len);
			write_cursor(&p->trace[targ].info,cursor.max,p->max_yrange_plus);
			write_cursor(&p->trace[targ].info,cursor.min,p->max_yrange_plus);
			write_cursor(&p->trace[targ].info,cursor.min,p->max_yrange_plus-len); /**-p->max_yrange_minus  */
		}else{
			printf("Don't know how to handle cursor function '%s'. Ignoring.\n",cursor.func);
			return 0;
		}
	}
	return 0;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_datafiles(struct plot_data *p)
{
	int c,dp,od,i;
	for (c=0; c<p->idx;++c){
		p->trace[c].yoff=p->yoff_max;
		
		if(p->flags & FLAGS_GNUPLOT){
			if(-1 ==write_script_file(&p->trace[0].info, "-", p->titles, p->xtitle,WR_SINGLE_PLOT)) {
				printf("Script file error\n");
				return 1;
			}
		}else{
			if(1>(od=open(p->trace[c].oname,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
				printf("Unable to open %s\n",p->trace[c].oname);
				return 1;
			}	
			printf("Writing %s\n",p->trace[c].oname);
		}
		
		for (dp=0; dp<1024;++dp) {
			char buf[50];
			double x,y;
			x=p->trace[c].data_pt.data[dp].x;
			y=p->trace[c].data_pt.data[dp].y;
			if(p->flags & FLAGS_GNUPLOT){/**write data  */
				p->trace[c].info.x=x;
				p->trace[c].info.y=y;
				write_script_file(&p->trace[c].info, NULL, NULL, NULL,WR_DATA); 
			}	
			else {
				/*printf("%E %E (%s)\n",x,y,buf); */
				i=sprintf(buf,"%E %E\n",x,y);
				write(od,buf,i);	
			}
		}
		if(p->flags & FLAGS_GNUPLOT){ /**write trigger  */
			write_script_file(&p->trace[0].info,NULL,NULL,0,WR_TRIGGER);
		}	else {
			close(od);
		}
		od=0;
	} /**data write  */	
	return 0;
}
/***************************************************************************/
/** Set the function number and minimum files.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_function(struct func *f, char *name)
{
	struct func_list *l;
	f->func=FUNCTION_BAD;
	f->min_files=0;
	for (l=func_names;NULL != l->name; ++l){
		if(!strcmp(l->name,name)){
			f->func=l->func;
			f->min_files=l->min_files;
			return 0;
		}		
	}
	printf("Unknown function '%s'\n",name);
	return 1;
}
/***************************************************************************/
/** When gnuplot is specified, there is just one file, the script file, so
write_script_file is used for all writing.
\n\b Arguments:
\n\b Returns:
****************************************************************************/

int main (int argc, char *argv[])
{
	char buf[BUF_LEN+1];
	int ret=1, c;
	int  f_load, last_idx;
	struct plot_data plot;
	memset(&plot,0,sizeof(struct plot_data));
	
	plot.graphx=640;
	plot.graphy=480;
	f_load=0;
	last_idx=0;
	while( -1 != (c = getopt(argc, argv, "c:d:f:gi:ml:o:s:t:x:y:")) ) {
		switch(c){
			case 'c':
				plot.trigname=strdup(optarg);
				break;
			case 'd':
				plot.cursorfile=strdup(optarg);
				break;			
			case 'f':
				++f_load;
				if(plot.f_idx>= MAX_INPUTFILES){
					printf("Exceeded max functions of %d\n",MAX_INPUTFILES);
					return 1;
				}
				plot.function[plot.f_idx].name=strdup(optarg);
				if(set_function(&plot.function[plot.f_idx++],optarg) ) 
					return 1;
				break;
			case 'g':
				plot.flags|=FLAGS_GNUPLOT;
				break;
			case 'h':
				usage();
				return 0;
				break;
			case 'i':
				if(f_load){
					struct func *p;
					if(0 == plot.f_idx){
						printf("No -f, but loading -f with -i ??\n");
						return 1;
					}
					p=&plot.function[plot.f_idx-1];
					if(p->idx >= MAX_FUNC_INPUT){
						printf("Exceeded function input limit of %d\n",MAX_FUNC_INPUT);
						return 1;
					}
					p->input[p->idx++]=strdup(optarg);
				}	else {
					if(plot.idx>= MAX_INPUTFILES){
						printf("Exceeded max input files of %d\n",MAX_INPUTFILES);
						return 1;
					}
					plot.trace[plot.idx].info.smoothing=0;
					plot.trace[plot.idx].info.terminal=TERM_X;
					plot.trace[plot.idx].info.line_color="x00ff00";	
					last_idx=plot.idx;
					plot.trace[plot.idx++].iname=strdup(optarg);	
				}
				
				break;
			case 'l':
				plot.trace[last_idx].info.line_color=strdup(&optarg[1]);
				break;
			case 'm':
				plot.flags|=FLAGS_MULTIPLOT;
				break;
			case 'o':
				plot.basename=strdup(optarg);
				break;	
			case 's':
				plot.trace[last_idx].info.smoothing=strtol(optarg,NULL,10);
				break;
			case 't':
				if(!strcmp(optarg,"png"))
					plot.terminal=TERM_PNG;
				else if(!strcmp(optarg,"x11"))
					plot.terminal=TERM_X;
				else if(!strcmp(optarg,"ascii"))
					plot.terminal=TERM_DUMB;				
				else {
					printf("Must specify X or png for -t option\n");
					usage();
					return 1;
				}
				break;
			case 'x':
				plot.graphx=atoi(optarg);
				break;
			case 'y':
				plot.graphy=atoi(optarg);
				break;			
		}
	}
	if(plot.flags &FLAGS_GNUPLOT){
		if( plot.flags &FLAGS_MULTIPLOT ){
			printf("Cannot specify both -m and -g\n");
			return 1;
		}
		if(plot.idx>1){
			printf("Cannot plot more than one file with -g option\n");
			return 1;
		}
	}	
	if( 0 == plot.idx || NULL == plot.basename){
		printf("Must supply at least -o, -i\n");
		usage();
		return 1;
	}
	/*preload data, find our max offset*/
	if(load_data(&plot))
		return 1;
	/**build function data & write the data file.  */
	if(handle_functions(&plot))
		return 1;
	/**now adjust 2% per trace  */
	c=(plot.max_yrange_plus+plot.max_yrange_minus)*4/100;
	c*=(plot.idx+plot.f_idx);
	plot.max_yrange_plus+=c;
	printf("ymin %d , ymax %d\n",plot.max_yrange_plus, plot.max_yrange_minus);
	
	/**set the output script name  &size*/
	plot.trace[plot.idx].info.x=plot.graphx;
	plot.trace[plot.idx].info.y=plot.graphy;
	write_script_file(&plot.trace[plot.idx].info, plot.basename, NULL, NULL,WR_SETFILENAME);
	
	/*printf("plot.idx=%d\n",plot.idx); */
	if(plot.flags &FLAGS_GNUPLOT){
		plot.trace[0].info.yrange_plus=plot.max_yrange_plus;
		plot.trace[0].info.yrange_minus=plot.max_yrange_minus;
	}	
	/**write the data  */
	if(write_datafiles(&plot))
		goto end;
	
	ret=0;
	
	/**write the script file  */
	sprintf(buf,"\"%s\"",plot.titles);
	/**the offset to the trigger is set when c=0  */
	for (c=0; c<plot.idx;++c){
		plot.trace[c].info.yrange_plus=plot.max_yrange_plus;
		plot.trace[c].info.yrange_minus=plot.max_yrange_minus;
		plot.trace[c].info.src=plot.trace[c].label;
		write_script_file(&plot.trace[c].info,plot.trace[c].oname,buf,plot.xtitle,c+1);	
	}
	plot.trace[c].info.ytrig=plot.ytrig;
	plot.trace[c].info.xtrig=plot.xtrig;
	/**write the trigger.  */
	write_script_file(&plot.trace[c].info,NULL,NULL,0,WR_TRIGGER);	
	
	/**Handle functions - write the plot lines */
	for (c=0;c<plot.f_idx;++c){
		plot.function[c].trace.info.src=plot.function[c].name;
		write_script_file(&plot.function[c].trace.info,plot.function[c].trace.oname,NULL,NULL,WR_MULTI_PLOT);
	}
	/**now handle the cursor stuff.  */
	plot_cursor(&plot);
	
	/**close output file  */
	write_script_file(NULL,NULL,NULL,0,WR_CLOSE);

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

		printf("set xlabel \"%s\"\n",plot.trace[c].info.time_units);
		printf("set ylabel \"%s\"\n",plot.trace[c].info.vert_units);
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
	Then add this line (based on plot.trace[c].yoff from data).:
'-' title "" ls 4 with lines
0 2810
41 2810
0 2810 
0 2265
41 2265
  
  */

