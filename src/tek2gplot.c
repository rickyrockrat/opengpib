/** \file ******************************************************************
\n\b File:        tek2gplot.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/05/2007  1:12 am
\n\b Description: 
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
#define _GNU_SOURCE
/*#define _ISOC99_SOURCE  */
/*#define _XOPEN_SOURCE=600   */
#include <fcntl.h>
#include <sys/stat.h>


#include <math.h>
#include <inttypes.h>
#include "open-gpib.h"

#define CURSOR_XPERCENT 17
#define RMARGIN 19
#define LMARGIN 13
#define BMARGIN	5

#define MAX_INPUTFILES 10

#define TERM_X 		1
#define TERM_PNG 	2
#define TERM_DUMB 3

#define FUNCTION_BAD 	 -1
#define FUNCTION_NONE 	0
#define FUNCTION_DIFF 	1
#define FUNCTION_MAXY 	2
#define FUNCTION_MINY 	3
#define FUNCTION_PK_PK 	4

struct func_list {
	int min_files;
	int func;
	int datafile; /**1 have data file, 0 don't  */
	char *name;
};

struct func_list func_names[]={
	{.name="diff",.func=FUNCTION_DIFF,.min_files=2,.datafile=1},
	{.name="max-y",.func=FUNCTION_MAXY,.min_files=1,.datafile=0},
	{.name="min-y",.func=FUNCTION_MINY,.min_files=1,.datafile=0},
	{.name="pk-pk",.func=FUNCTION_PK_PK,.min_files=1,.datafile=0},
/*	{.name=,.func=FUNCTION_,.min_files=,.datafile=}, */
	{.name=NULL,.func=0,.min_files=0},
};

#define BUF_LEN 		1024
#define MAX_POINTS 	20000

#define WR_SINGLE_PLOT 0
#define WR_MULTI_PLOT  2
#define WR_CLOSE			 -1
#define WR_TRIGGER     -2
#define WR_DATA				 -3
#define WR_SETFILENAME -4
#define WR_CURSORS     -5
#define WR_LABEL  		 -6
#define WR_RMARGIN     -7

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
	double points;
	double trigger;
	double yoff;
	double yzero; /**adders to vertical  */
	double xzero; /**adders to horizontal  */
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
	int datafile;	/**1 - creates data file for function 0 -creats label  */
	struct trace_data trace;
	char *input[MAX_FUNC_INPUT+1]; /* filename to operate on. Must match input filename */
	int main_idx[MAX_FUNC_INPUT+1]; /**numerical offset to trace data corresponding to input, above  */
};

struct plot_data {
	int max_yrange_plus;	/**positive value, represents max value above 0  */
	int max_yrange_minus;	/**positive value, represents max value below 0  */
	int idx;		/**location of data idx.  */
	int f_idx; /**function index, number of functions.  */
	int flags; /**see FLAG_xxx  */
	int graphx;	/**x size of graph in pixels, for some terminals  */
	int graphy; /**y size of graph in pixels, for some terminals  */
	double xtrig;	 /**x location of the trigger  */
	double ytrig;	 /**y location of the trigger  */
	double yoff_max;
	char *trigname; /**file name that tigger is on  */
	char *cursorfile;	/**cursor file name  */
	char *basename;	/**name to base output names on  */
	int terminal;
	struct trace_data trace[MAX_INPUTFILES+1];  /**data for each input file  */
	char titles[BUF_LEN+1];											/**titles at top of graph  */
	char xtitle[50];														/**Title for X11 window  */
	struct func function[MAX_INPUTFILES+1];			/**data for each function  */
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

struct personality {
	char *yzero;  /**  */
	char *xzero;
	int pre_yoff; /**take the offset off the value before using multipliers  */
	int title_delim; /**Delimiter when parsing the title (stuff between "")  */
	char *pre_delim;	/**list of delimiters (in a string) for parsing the preamble  */
	int value_delim;	/**delimiter for the value delimiter  */
	char *name;	/**instrument name  */
	char *yoff;	/** these are strings for the names to search the preamble for  */
	char *ymult;
	char *xincr;
	char *points;
	char *fmt;
	char *enc;
	char *trigger;
	char *title;
	char *curve;
	char *delim_list;
	char *enc_supported;
};

static struct personality inst[]={
	{.yzero=NULL,.xzero=NULL,.pre_yoff=0,.title_delim=' ',.pre_delim=":",.value_delim=',',.name="2440",.yoff="YOFF",.ymult="YMULT",.xincr="XINCR",.points="NR.PT",.fmt="BN.FMT",.enc="ENCDG",.trigger="PT.OFF",.title="WFID",.curve="CURVE",.delim_list=",;",.enc_supported="ASCII"},
	{.yzero="YZE",.xzero="XZE",.pre_yoff=1,.title_delim=',',.pre_delim=",",.value_delim=',',.name="TDS",.yoff="YOF",.ymult="YMU",.xincr="XIN",.points="NR_P",.fmt="BN_F",.enc="ENC",.trigger="PT_O",.title="WFI",.curve="CURV",.delim_list=";",.enc_supported="ASC"},
	{.yzero=NULL,.xzero=NULL,.pre_yoff=0,.title_delim=0,.pre_delim="",.value_delim=',',.name=NULL,.yoff="",.ymult="",.xincr="",.points="",.fmt="",.enc="",.trigger="",.title="",.curve="",.delim_list="",.enc_supported=""},
};
static int inst_no=0;



/***************************************************************************/
/** \details Find the instrument, so we know how to parse. Defaults to 2440.
\param 
\returns 
****************************************************************************/
int find_instrument(char *name, int mode)
{
	int i;
	if( mode )
		printf("Known instruments:\n");
	for (i=0; NULL != inst[i].name; ++i){
		if(mode)
			printf("%s\n",inst[i].name);
		else{	
			if(!strcmp(name,inst[i].name))
				return i;
		}	
	}
	return -1;
}
/**build with 
gcc -Wall -lm -o tek2plot tek2gplot.c
  */
/**
2440 = 
WFMPRE WFID:"CH1 DC   5mV 500ms NORMAL",
NR.PT:1024,PT.OFF:128,PT.FMT:Y,XUNIT:SEC,XINCR:1.000E-2,YMULT:2.000E-4,YOFF:-2.625E+1,YUNIT:V,BN.FMT:RI,ENCDG:ASCII;CURVE  
TDS 684 =
:WFMP:BYT_N 1;BIT_N 8;ENC ASC;BN_F RP;BYT_O MSB;CH1:WFI 
"Ch1, DC coupling, 2.000 Volts/div, 5.000us/div, 15000 points, Sample mode";

NR_P 500;PT_F Y;XUN "s";XIN 100.0E-9;XZE 46.7E-9;PT_O 1500;YUN "Volts";YMU 80.000E-3;YOF 53.50E+0;YZE 0.0E+0;:CURV 
From the TDS programmers manual:
Xn = XZEro + XINcr * (n­PT_Off)
Yn = YZEro + YMUlt * (Yn - YOFf)

*/

/***************************************************************************/
/** Load everything before curve into a buffer and set offset to first data
point.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char *load_tek_preamble(int fd, int *offset)
{
	char buf[256], *end;
	int i,x,k,l;
	i=read(fd,buf,250);
	for (x=k=0; k<i;++k){
		while(buf[x]== '\n' || buf[x] == '\r'){
			++x;
			
		}
		buf[k]=buf[x++];
	}
	
	end=strstr(buf,inst[inst_no].curve);
	if(NULL == end)	{
		*offset=-1;
		buf[i]=0;
		printf("preamble=%s\n",buf);
		return NULL;
	}
	l=strlen(inst[inst_no].curve)+1;
	i=end-buf;
	buf[i]=0;
	*offset=i+l+(x-k);
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
  for (i=0;buf[pos] != inst[inst_no].value_delim && len;){
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
  while( ('\n' == buf[pos] || buf[pos] == '\r' ||inst[inst_no].value_delim == buf[pos]) && len) {
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
/** Process the title "CH1 DC   5mV 500ms NORMAL"
\n\b Arguments:
\n\b Returns:   
****************************************************************************/
int break_extended(char *data, struct extended *x)
{
	int k,i,state,j;
	char buf[10];
	state=_SRC;
	/*printf("%s\n",data); */
	/*memset(x,0,sizeof(struct extended)); can't do this- we have persistent data*/
	x->src=x->coupling=x->vert_units=x->time_units=NULL;
	for (i=k=0;data[i];){
		if(data[i] != '"' && !og_is_delim(data[i], inst[inst_no].pre_delim))
			buf[k++]=data[i++];
		else if(og_is_delim(data[i],inst[inst_no].pre_delim)){
			buf[k]=0;
			while (og_is_delim(data[i],inst[inst_no].pre_delim)) ++i;
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
						if('.'  != buf[k] && ' ' != buf[k] && (buf[k] >'9' || buf[k] < '0') )
							break;
					}
					for (j=k;buf[j];++j){
						if('/' == buf[j])
							break;
					}
					buf[j]=0;
					x->vert_units=strdup(&buf[k]);
					if(!strcmp(x->vert_units,"mV"))
							x->vert_mult=1000;
					else 
						x->vert_mult=1;
					printf("Vert:%d'%s' %E\n",x->vert_div,x->vert_units,x->vert_mult); 
					++state;
					break;
				case _HORIZ:
					/*printf("buf='%s'\n",buf); */
					for (k=0;buf[k];++k){
						if(' ' != buf[k])
							break;
					}
						
					x->time_div=strtod(buf,NULL);
					for (;buf[k];++k){
						if('.'  != buf[k] && (buf[k] >'9' || buf[k] < '0') )
							break;
					}
					for (j=k;buf[j];++j){
						if('/' == buf[j])
							break;
					}
					buf[j]=0;
					x->time_units=strdup(&buf[k]);
					x->time_mult=get_multiplier(x->time_units);
					printf("time:%g'%s' %E\n",x->time_div,x->time_units,x->time_mult); 
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
int write_file ( int fd, char *buf, int len, char *fname)
{
	int i;
	if ( (i=write(fd,buf,len)) != len)
			fprintf(stderr,"Failed to write all bytes (%d of %d) to %s\n",i,len,fname);
	return i;
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
	static int offset, x, y, rmargin=RMARGIN;
	static char *fname;
	int ilinecolor=0;

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
	if(WR_LABEL==plotno){
		char *s;
		if(NULL == ext){
			printf("ext NULL!!\n");
			return 1;
		}
		if(NULL == dfile)
			s="graph";
		else
			s=dfile;
		c=sprintf(buf,"set label %d \"%s\" at %s %f, %s %f\n",labelno++,title,s,ext->x,s, ext->y);
		write_file(od,buf,c,fname);
		return 0;
	}
	if(WR_RMARGIN == plotno){
		rmargin=round(ext->x);
		return 0;
	}
	if(WR_DATA == plotno){	/**write data  */
		c=sprintf(buf,"%E %E\n",ext->time_mult*ext->x,ext->y);
		write_file(od,buf,c,fname);
		return 0;
	}
	if(WR_TRIGGER == plotno){/**write trigger  */
		off_t o;
		o=lseek(od,0,SEEK_CUR);
		lseek(od,offset,SEEK_SET);
		c=sprintf(buf,"%E,%E front\n",ext->xtrig,ext->ytrig);			
		write_file(od,buf,c,fname);
		offset=lseek(od,0,SEEK_CUR);
		lseek(od,o,SEEK_SET);
		return 0;
	}
	if(WR_CURSORS == plotno){/**write cursors  */
		off_t o;
		o=lseek(od,0,SEEK_CUR);
		lseek(od,offset,SEEK_SET);
		c=sprintf(buf,"set label %d '%s' at %E,%E front",labelno++,ext->src,ext->x,ext->y);			
		write_file(od,buf,c,fname);
		offset=lseek(od,0,SEEK_CUR);
		lseek(od,o,SEEK_SET);
		c=sprintf(buf,",\\\n'-' title \"\" ls %d with lines\n",ilinecolor);
		write_file(od,buf,c,fname);			
		++ilinecolor;
		return 0;
	}

	if(plotno <WR_MULTI_PLOT ){
		c=sprintf(buf,"set xlabel \"%s\"\n",ext->time_units);
		write_file(od,buf,c,fname);
		c=sprintf(buf,"set lmargin %d\nset bmargin %d\nset rmargin %d\n",LMARGIN,BMARGIN,rmargin);
		write_file(od,buf,c,fname);
		c=sprintf(buf,"set label %d \"%s\" at graph -0.15, graph .5\n",labelno++,ext->vert_units);
		write_file(od,buf,c,fname);
		c=sprintf(buf,"set yrange[%d:%d]\n",-ext->yrange_minus,ext->yrange_plus);
		write_file(od,buf,c,fname);
		c=sprintf(buf,"set xrange[0:%d]\n",ext->xrange);
		write_file(od,buf,c,fname);
		c=sprintf(buf,"set xtics 0,%f\n",ext->time_div);
		write_file(od,buf,c,fname);
		c=sprintf(buf,"set title %s\n",title);
		write_file(od,buf,c,fname);
		for (data=1; data<9;++data){
			c=sprintf(buf,"set style line %d lt %d lw 2.000 pointtype 4\n",data,data);
			write_file(od,buf,c,fname);	
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
				write_file(od,buf,c,fname);
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
				write_file(od,buf,c,fname);
				break;
			case TERM_DUMB:
				c=sprintf(buf,"set terminal dumb\n");
				write_file(od,buf,c,fname);
				break;
		}
		if(TERM_X == ext->terminal ){
			ilinecolor=get_linecolor(ext->line_color);
		}
		else ilinecolor=2;
		c=sprintf(buf,"set label %d 'T' at ",labelno++);
		write_file(od,buf,c,fname);
		offset=lseek(od,0,SEEK_CUR);
		/*printf("offset at %d\n",offset); */
		/**save space for rest of line.. and possible cursor title */
		c=sprintf(buf,"																					                  ");
		write_file(od,buf,c,fname);
		c=sprintf(buf,"																					                 ");
		write_file(od,buf,c,fname);
		c=sprintf(buf,"																					                 \n");
		write_file(od,buf,c,fname);
		
		if(ext->smoothing){
			c=sprintf(buf,"set samples %d\n",ext->smoothing);
			write_file(od,buf,c,fname);		
		}		
	}	
	
	if(ext->smoothing){
		c=sprintf(buf,"%s'%s' title \"%s\" ls %d smooth csplines with lines%s",plotno>0?first?"\nplot ":",\\\n":"\nplot ",dfile,ext->src,ilinecolor,plotno>0?"":"\n");
		write_file(od,buf,c,fname);			
	}
	else{
		c=sprintf(buf,"%s'%s' title \"%s\" ls %d with lines%s",plotno>0?first?"\nplot ":",\\\n":"\nplot ",dfile,ext->src,ilinecolor,plotno>0?"":"\n");
		write_file(od,buf,c,fname);			
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
	printf("tek2gplot: $Revision: 1.22 $\n"
	" -a text Append descriptive text to graph at bottom left.\n"
	" -c channelfname Set the channel no for the trigger file name. i.e. \n"
	"    which channel is trigger source. This must match an -i.\n"
	"    if this is not set, the first file is assumed to have the trigger\n"
	" -d cursorname Show the cursor info from the cursors file\n"
	" -f function Add another plot with the following functions: diff\n"
	"    This MUST be after all input files. Additional -i will define what\n"
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
	" -p name Set personality to name (2440)\n"
	" -s val Use smoothing with value val. \n"
	
	" -t term, set terminal to png, ascii, or x11 (default is x11)\n"
	"    if you use -t png, gnuplot usage is:\n"
	"    gnuplot fname > image.png\n" \
	" -x pixels Set out (png for now) X size in pixels\n"
	" -y pixels Set out (png for now) Y size in pixels\n"
	);
	find_instrument(NULL,1);
	
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
	e-=s;	/**account for leading spaces: ' "'  */
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
	if(e->time_div >=100){
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
	if( 1>read(id,buf,BUF_LEN-1)) {
		fprintf(stderr,"Error reading or 0 bytes read from %s\n",name);
		return 1;
	}
	c->func=og_get_string("CURSOR",buf,inst[inst_no].delim_list);
	c->max=og_get_value("MAX",buf);
	c->min=og_get_value("MIN",buf);
	c->diff=og_get_value("DIFF",buf);
	c->target=og_get_string("TARGET",buf,inst[inst_no].delim_list);
	c->trigger=og_get_string("TRIGGER", buf,inst[inst_no].delim_list);
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
		if(NULL == (preamble=load_tek_preamble(id, &data))){
			printf("Unable to load preamble for file %s\n",p->trace[c].iname);
			goto closeid;
		}
		p->trace[c].info.terminal=p->terminal;
		p->trace[c].yoff=og_get_value(inst[inst_no].yoff, preamble);
		/**PT.OFF  */
		if(NULL != inst[inst_no].xzero)
			p->trace[c].xzero=og_get_value(inst[inst_no].xzero,preamble);
		else
			p->trace[c].xzero=0;
		if(NULL != inst[inst_no].yzero)
			p->trace[c].yzero=og_get_value(inst[inst_no].yzero,preamble);
		else
			p->trace[c].yzero=0;
		printf("Xzero=%g Yzero= %g ",p->trace[c].xzero,p->trace[c].yzero);
		p->trace[c].ymult=og_get_value(inst[inst_no].ymult,preamble);
		p->trace[c].xincr=og_get_value(inst[inst_no].xincr, preamble);
		p->trace[c].points=og_get_value(inst[inst_no].points, preamble);
		if(p->trace[c].points> MAX_POINTS){
			printf("Points from waveform = %f. Max is %d. We will truncate.\n",p->trace[c].points,MAX_POINTS);
			p->trace[c].points=(double)MAX_POINTS;
		}
		p->trace[c].fmt=og_get_string(inst[inst_no].fmt,preamble,inst[inst_no].delim_list);
		p->trace[c].enc=og_get_string(inst[inst_no].enc,preamble,inst[inst_no].delim_list);
		p->trace[c].trigger=og_get_value(inst[inst_no].trigger,preamble);
		p->trace[c].title=og_get_string(inst[inst_no].title,preamble,inst[inst_no].delim_list);
		strip_quotes(p->trace[c].title);
		printf("yoff=%g ymult=%g xincr=%g points=%f\n",p->trace[c].yoff,p->trace[c].ymult,p->trace[c].xincr,p->trace[c].points);
		if(!strcmp("RI",p->trace[c].fmt))
			p->trace[c].sgn=1;
		else if(!strcmp("RP",p->trace[c].fmt))
			p->trace[c].sgn=0;
		else {
			printf("Unknown format %s\n",p->trace[c].fmt);
			goto closeid;
		}
		
		if(strcmp(p->trace[c].enc,inst[inst_no].enc_supported)){
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
		/*printf("Total time = %f %f points per division, time_div %f time_mult %f\n",p->trace[c].xincr*p->trace[c].points,round((p->trace[c].info.time_div/(p->trace[c].info.time_mult))/p->trace[c].xincr),p->trace[c].info.time_div,p->trace[c].info.time_mult); */
		i=strlen(p->xtitle);
		sprintf(&p->xtitle[i],"%s ",p->trace[c].label);
		
		
		/*yrange=( ((int)round(p->trace[c].info.vert_div*8))+p->trace[c].yoff<0?(int)(p->trace[c].yoff*-1):(int)p->trace[c].yoff)>>1; */
		if(0 == inst_no)/**what is this????  */
			p->trace[c].info.yrange_plus=p->trace[c].info.yrange_minus=((int)round(p->trace[c].info.vert_div*8))>>1;
		else 
			p->trace[c].info.yrange_plus=p->trace[c].info.yrange_minus=((int)round(p->trace[c].info.vert_div*4));
		if( inst[inst_no].pre_yoff ){
			if(p->trace[c].yoff >=0)
				p->trace[c].info.yrange_minus+=round(p->trace[c].yoff*p->trace[c].ymult*p->trace[c].info.vert_mult);
			else
				p->trace[c].info.yrange_plus+=round(-p->trace[c].yoff*p->trace[c].ymult*p->trace[c].info.vert_mult);
		}else {
			if(p->trace[c].yoff >=0)
				p->trace[c].info.yrange_minus+=round(p->trace[c].yoff);
			else
				p->trace[c].info.yrange_plus+=round(-p->trace[c].yoff);
		}
		
		
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
		printf("yrange=[-%d:%d] in %s ",p->trace[c].info.yrange_minus,p->trace[c].info.yrange_plus,p->trace[c].info.vert_units); 
		p->trace[c].info.xrange=(int)round(p->trace[c].xincr*p->trace[c].points*p->trace[c].info.time_mult);
		trig=(int)round(p->trace[c].trigger);
		printf("Xrange=[0:%d] in %s\n",p->trace[c].info.xrange,p->trace[c].info.time_units); 
		/*printf("set xtics 0,%f\n",p->trace[c].info.time_div); */
		p->trace[c].info.x=0;
		buf[0]=1;
		for (dp=0; dp<p->trace[c].points && buf[0];++dp) {
			if(1> get_next_value(id,buf))	
				break;
			p->trace[c].info.y=strtof(buf, NULL);
			
			if( inst[inst_no].pre_yoff ){
				p->trace[c].info.y=(p->trace[c].yzero + (p->trace[c].info.y-p->trace[c].yoff)*(p->trace[c].ymult*p->trace[c].info.vert_mult));
				if(  0 == p->trace[c].sgn)
					p->trace[c].info.y-=p->trace[c].ymult*p->trace[c].yoff;
			}else{
				if(  0 == p->trace[c].sgn)
					p->trace[c].info.y=p->trace[c].info.y-128;
				p->trace[c].info.y=p->trace[c].yzero + (p->trace[c].info.y*p->trace[c].ymult*p->trace[c].info.vert_mult) - p->trace[c].yoff;
			}
				
			save_data_point(&p->trace[c],
					p->trace[c].xzero + p->trace[c].info.time_mult*p->trace[c].info.x,
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
/** PK-PK, min, max, etc show up on Rt side, so space is limited.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int handle_functions(struct plot_data *p)
{
	int i,c,od, len;
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
			if(0 == f->datafile) /**write a label  */
				len=strlen(f->name)+20;
			else
				len=10;	
			if(NULL == (f->trace.oname=malloc(strlen(f->name)+strlen(p->basename)+len)) ){
				printf("Out of memory for %s&on\n",f->name);
				return 1;
			}
			if(f->datafile){
				sprintf(f->trace.oname,"%s.%s",p->basename,f->name);
				if(1>(od=open(f->trace.oname,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) ){
					printf("Unable to open %s\n",f->trace.oname);
					return 1;
				}				
				printf("Writing %s\n",f->trace.oname);	
			}else 
				od=0;
			
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
						write_file(od,buf,i,f->trace.oname);
					}
					break;
				
				case FUNCTION_PK_PK: 
				case FUNCTION_MAXY:
				case FUNCTION_MINY:
				{
					double min,max,diff;
					struct data_point *d;
					min=max=0;
					d=&p->trace[f->main_idx[0]].data_pt;
					for (c=0; c< MAX_POINTS && c< d->idx; ++c){
						if(d->data[c].y >max)
							max=d->data[c].y;
						if(d->data[c].y <min)
							min=d->data[c].y;
					}	
					diff=max-min;
					switch(f->func){
						case FUNCTION_PK_PK: diff=diff; break;
						case FUNCTION_MAXY: diff=max; break;
						case FUNCTION_MINY: diff=min; break;
				}
					sprintf(f->trace.oname,"%s %s %.1f%s",p->trace[f->main_idx[0]].label,f->name,diff,p->trace[f->main_idx[0]].info.vert_units);
					printf("%d:%s\n",i,f->trace.oname);	
				}
					break;
			}	/**end switch  */
			if(od>2)
				close(od);
		}		
	}
	return 0;
}

#define CURSOR_VOLTS 1
#define CURSOR_TIME  2
#define CURSOR_ONE_TIME 3

/***************************************************************************/
/** Write the cursor out to the plot file.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int plot_cursor(struct plot_data *p)
{
  struct cursors cursor;
	double center;
	int targ, cursor_type;
	char buf[50];
	if(NULL == p ||NULL == p->cursorfile){
		printf("plot_cursor: arg or fname NULL\n");
		return 1;
	}
	
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
	if(!strcmp(cursor.func,"VOLTS"))
		cursor_type=CURSOR_VOLTS;
	else if(!strcmp(cursor.func,"TIME"))
		cursor_type=CURSOR_TIME;
	else if(!strcmp(cursor.func,"ONE/TIME") )
		cursor_type=CURSOR_ONE_TIME;
	else{
		printf("Don't know how to handle cursor function '%s'. Ignoring.\n",cursor.func);
		return 0;
	}
	if(CURSOR_VOLTS==cursor_type){
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
	}	else if(CURSOR_TIME ==cursor_type|| CURSOR_ONE_TIME==cursor_type) {
		double len;
		len=(p->max_yrange_plus+p->max_yrange_minus)/50;
		p->trace[targ].info.x=cursor.min*get_multiplier(p->trace[targ].info.time_units);;
		p->trace[targ].info.y=(p->max_yrange_plus+len);
		/*printf("diff=%f\n",diff); */
		
		/*diff*=1000; */
		if(CURSOR_TIME ==cursor_type){
			cursor.diff *=get_multiplier(p->trace[targ].info.time_units);
			sprintf(buf,"%.3f%s",cursor.diff,p->trace[targ].info.time_units);
		}
		if(CURSOR_ONE_TIME==cursor_type){
			double x;
			int m;
			x=format_eng_units(cursor.diff, &m);
			sprintf(buf,"%.3f%cHz",x,m);
		}
			
		p->trace[targ].info.src=buf;
		write_script_file(&p->trace[targ].info,NULL,NULL,0,WR_CURSORS);	
		/**now draw the cursors  */   
		len*=2;
		write_cursor(&p->trace[targ].info,cursor.max,p->max_yrange_plus-len);
		write_cursor(&p->trace[targ].info,cursor.max,p->max_yrange_plus);
		write_cursor(&p->trace[targ].info,cursor.min,p->max_yrange_plus);
		write_cursor(&p->trace[targ].info,cursor.min,p->max_yrange_plus-len); /**-p->max_yrange_minus  */
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
		
		for (dp=0; dp<p->trace[c].points;++dp) {
			char buf[50];
			double x,y;
			x=p->trace[c].data_pt.data[dp].x;
			y=p->trace[c].data_pt.data[dp].y;
/*			printf("%d: %E %E\n",dp,x,y); */
			if(p->flags & FLAGS_GNUPLOT){/**write data  */
				p->trace[c].info.x=x;
				p->trace[c].info.y=y;
				write_script_file(&p->trace[c].info, NULL, NULL, NULL,WR_DATA); 
			}	
			else {
				/*printf("%E %E (%s)\n",x,y,buf); */
				i=sprintf(buf,"%E %E\n",x,y);
				write_file(od,buf,i,p->trace[c].oname);	
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
			f->datafile=l->datafile;
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
	char buf[BUF_LEN+1], *desc;
	int ret=1, c;
	int  f_load, last_idx;
	struct plot_data plot;
	struct extended e;
	memset(&plot,0,sizeof(struct plot_data));
	
	plot.graphx=640;
	plot.graphy=480;
	f_load=0;
	last_idx=0;
	desc=NULL;
	while( -1 != (c = getopt(argc, argv, "a:c:d:f:ghi:ml:o:p:s:t:x:y:")) ) {
		switch(c){
			case 'a':
				desc=strdup(optarg);
				/*printf("Got '%s'\n",desc); */
				strip_quotes(desc);
				break;
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
			case 'p':
				inst_no=find_instrument(optarg,0);
				if(-1 == inst_no){
					printf("Unable to find instrument '%s'\n",optarg);
					return -1;
				}
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
	/**check functions..use first two files if none were specified.  */
	if(plot.f_idx){
		for (c=0; c<plot.f_idx;++c){
			if(0 == plot.function[c].idx){
				int i;
				for (i=0;i<2;++i)
					plot.function[c].input[i]=plot.trace[i].iname;
				plot.function[c].idx+=i;
			}
		}
	}
	/*preload data, find our max offset*/
	if(load_data(&plot))
		return 1;
	/**build function data & write the data file.  */
	if(handle_functions(&plot))
		return 1;
	/**now adjust 5% per trace  */
	c=(plot.max_yrange_plus+plot.max_yrange_minus)*5/100;
	c*=(plot.idx+plot.f_idx);
	plot.max_yrange_plus+=c;
	/*printf("ymin %d , ymax %d\n",plot.max_yrange_plus, plot.max_yrange_minus); */
	
	/**set the output script name  &size*/
	plot.trace[plot.idx].info.x=plot.graphx;
	plot.trace[plot.idx].info.y=plot.graphy;
	write_script_file(&plot.trace[plot.idx].info, plot.basename, NULL, NULL,WR_SETFILENAME);
	if(NULL != desc){
		e.x=-.1;
		e.y=-.12;
		/*printf("Setting desc to '%s'\n",desc); */
		write_script_file(&e, "graph", desc , NULL,WR_LABEL);
	}
	/**now do functions that do not have data files  */
	e.x=1 - ((double)RMARGIN/100 +.01);
	e.y=.87;
	for (f_load=0,c=0;c<plot.f_idx;++c){
		if(0 == plot.function[c].datafile){
			write_script_file(&e,"screen",plot.function[c].trace.oname,NULL,WR_LABEL);	
			e.y-=.03;
			++f_load;
		}
		
	}	
	if(f_load){ /**adjust right margin  */
		e.x=(double)RMARGIN;
		write_script_file(&e,NULL,NULL,NULL,WR_RMARGIN);	
	} else{
		e.x=(double)2;
		write_script_file(&e,NULL,NULL,NULL,WR_RMARGIN);	
	}
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
		memcpy(&e,&plot.trace[c].info, sizeof(struct extended));
		e.yrange_plus=plot.max_yrange_plus;
		e.yrange_minus=plot.max_yrange_minus;
		e.src=plot.trace[c].label;
		if(0 == c && (e.xrange/e.time_div)>10){
			int r;
			r=e.xrange;
			if(r>10)
				r=(r/10)*10;
			printf("r=%d div=%f ",e.xrange,e.time_div);
			e.time_div=(double)(r)/10;
			printf("ndiv=%f\n",e.time_div);
		}
		write_script_file(&e,plot.trace[c].oname,buf,plot.xtitle,c+1);	
	}
	plot.trace[c].info.ytrig=plot.ytrig;
	plot.trace[c].info.xtrig=plot.xtrig;
	/**write the trigger.  */
	write_script_file(&plot.trace[c].info,NULL,NULL,0,WR_TRIGGER);	
	
	/**Handle functions with data files - write the plot lines */
	for (c=0;c<plot.f_idx;++c){
		
		if(FUNCTION_DIFF == plot.function[c].func){
			plot.function[c].trace.info.src=plot.function[c].name;
			write_script_file(&plot.function[c].trace.info,plot.function[c].trace.oname,NULL,NULL,WR_MULTI_PLOT);	
		}
		
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

