/** \file ******************************************************************
\n\b File:        networkanalyzer.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        04/04/2009 11:40 am
\n\b Description: Use a signal generator and a spectrum analyzer to 
analyze a circuit over frequency. Initally calculate the gain.
MKTYPE PSN;SNGLS;RB 100 Hz;SP 1 KHz;ST 1 S
CF MHz;TS;MKPK HI;MKA?
*/ /************************************************************************
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
#include "common.h"
#include "gpib.h"
#include <math.h>

/** Modes...  */
#define CAL 1
#define GAIN 2
#define PEAK_SEARCH 0x8000 

#define PSG2400L_IDN_STR "Wayne Kerr PSG2400L"
#define HP8595E_IDN_STR "HP8595E"

struct freq_off {
	float frequency;
	float offset;
	float level;
};
/***************************************************************************/
/** CF? carrier frequency	Ghz, Mhz, KHz, Hz
		CF-SS (carrier step size)
		CFU Carrier Freq. Step up
		CFD CF step down
    CL? Carrier Level. dBm, dBuV, V, mV, uV
    CL-SS (Carrier level step size)
    CLU CL up
    CLD CL down
    M1F Modulation 1 frequency 10hZ -500kHz
    M1L Modulation level 0-2Mhz FM
    M1F-SS
    M1FU
    M1FD
    EXTL MHz,KHz,%AM, rad
    EXTL-SS
    RF-ON, RF-OFF, STEP-ON, STEP-OFF,M1-ON, M1-OFF, EXT-ON, EXT-OFF
    EXTLEV-ON, EXTLEV-OFF (monitor RMS @modulation input) 
    
		SWPT s,ms sweep time 1-999sec
		SWP-ON,SWP-OFF, STEP-ON, STEP-OFF
    
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct ginstrument *init_PSG2400L(char *path, int type, int addr, struct gpib *g)	 
{
	struct ginstrument *gi;
	
	if(NULL ==(gi=malloc(sizeof(struct ginstrument)))){
		printf("Out of mem on ginstrument\n");
		return NULL;
	}
	memset(gi,0,sizeof(struct ginstrument));
	if(NULL == g){
		if(NULL == (gi->g=open_gpib(type,addr,path,-1))){
			printf("Can't open %s. Fatal\n",path);
			free(gi);
			return NULL;
		}	
	}
	if(NULL == gi->g)
		gi->g=g;
	gi->addr=addr;
	printf("Initializing PSG2400L\n");
	write_string(gi->g,"*CLS");
	if(0 == write_wait_for_data("*IDN?",3,gi->g)){
		printf("No data for id for '%s'\n",PSG2400L_IDN_STR);
		return NULL;
	}
		
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(gi->g->buf,PSG2400L_IDN_STR)){
		printf("Unable to find '"PSG2400L_IDN_STR"' in id string '%s'\n",gi->g->buf);
		return NULL;
	}
	return gi;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct ginstrument *init_HP8595E(char *path, int type, int addr, struct gpib *g)	 
{
	struct ginstrument *gi;
	if(NULL ==(gi=malloc(sizeof(struct ginstrument)))){
		printf("Out of mem on ginstrument\n");
		return NULL;
	}
	memset(gi,0,sizeof(struct ginstrument));
	if(NULL == g){
		if(NULL == (gi->g=open_gpib(type,addr,path,-1))){
			printf("Can't open %s. Fatal\n",path);
			free(gi);
			return NULL;
		}	
	}
	if(NULL == gi->g)
		gi->g=g;
	gi->addr=addr;
	printf("Initializing %s\n",HP8595E_IDN_STR);
	if(0 == write_get_data(gi->g,"ID?")){
		printf("No data for write_get id\n");
		return NULL;
	}
		
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(gi->g->buf,HP8595E_IDN_STR)){
		printf("Unable to find '"HP8595E_IDN_STR"' in id string '%s'\n",gi->g->buf);
		return NULL;
	}
	return gi;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("networkanalyzer <options>\n"
	" -a saddr set signal generator addr. ex. -a s9 (9)\n"
	" -a aaddr set analyzer addr. ex. -a a18 (18)\n"
	" -b bw Set Analyzer bandwidth in Hz (100).\n"
	" -c fname set cal in/out file to cal\n"
	" -d spath set signal generator controller device name. ex. -d s/dev/ttyUSB0 (defaults to last device)\n"
	" -d apath set spectrum analyzer controller device name.(defaults to last device\n"
	"  at least one -d must be specified. If the second is not specified, it is assumed there\n"
	"  is only one controller, and the second defaults to the first\n"
	" -f efreq set end frequency to freq (in Mhz). ex. -f e2000 (2000)\n"
	" -f sfreq set start frequency to freq(in Mhz). ex -f s1 (1)\n"
	" -f f set follow mode. Uses last peak to increment from\n"
	" -i inc set frequency to inc. (in Khz)\n"
	" -l slevel set start dBm to level (-30dBm)\n"
	" -m mode set mode to mode. c=cal, g=gain.(g)\n"
	" -o fname put output to file called fname\n"
	" -p set mode to peak finding before cal, does +/- 10*span on either side of start freq.\n"
	" -s span set analyzer span in Hz (1000)\n"
	" -t time set sweep time in miliseconds (1000)\n"
	" -v set verbose mode (off)\n"
	"");
	
}

/***************************************************************************/
/** .       GPIB_CTL_PROLOGIXS
\n\b Arguments:
\n\b Returns:
HP8595E
****************************************************************************/
int main(int argc, char * argv[])
{
	struct ginstrument *signalgen_inst,*analyzer_inst;
	char *signalgen_dev, *analyzer_dev;
	int signalgen_addr,analyzer_addr, mode,c, verbose, follow;
	float start, stop, inc, iter,slevel, rbw, span, sweeptime;
 	float level,freq, last_l, last_f,delta, x;
	char *calname, *dname, lbuf[100];
	FILE *cal, *data;
	signalgen_inst=analyzer_inst=NULL;
	signalgen_dev=analyzer_dev=calname=dname=NULL;
	cal=data=NULL;
	signalgen_addr=9;
	analyzer_addr=18;
	mode=GAIN;
	start=stop=inc=0;
	slevel=-30;
	rbw=100;
	span=1000;
	sweeptime=1000;
	verbose=0;
	follow=0;
	
	/**Handle options  */
	
	while( -1 != (c = getopt(argc, argv, "a:b:c:d:f:hi:l:m:o:ps:t:v")) ) {
		switch(c){
			case 'a':
				switch(optarg[0]){
					case 's':
						signalgen_addr=atoi(&optarg[1]);
						break;
					case 'a':
						analyzer_addr=atoi(&optarg[1]);
						break;
					default:
						printf("Unknown -a option '%c' in %s\n",optarg[1], optarg);
						return 1;
				}
				break;
			case 'b':
				rbw=atoi(optarg);
				break;
			case 'c':
				calname=strdup(optarg);
				break;
			case 'd':
				switch(optarg[0]){
					case 's':
						signalgen_dev=strdup(&optarg[1]);
						break;
					case 'a':
						analyzer_dev=strdup(&optarg[1]);
						break;
					default:
						printf("Unknown -d option '%c' in %s\n",optarg[1], optarg);
						return 1;
				}
				break;
			case 'f':
				switch(optarg[0]){
					case 'e':
						sscanf(&optarg[1],"%f",&stop);
						break;
					case 'f':
						follow=1;
						break;
					case 's':
						sscanf(&optarg[1],"%f",&start);
						break;
					default:
						printf("Unknown -f option '%c' in %s\n",optarg[1], optarg);
						return 1;
				}
				break;			
			case 'h':
				usage();
				return 1;
			case 'i':
				sscanf(optarg,"%f",&inc);
				inc/=1000;
				break;
			case 'l':
				switch(optarg[0]){
					case 's':
						sscanf(&optarg[1],"%f",&slevel);
						break;
					default:
						printf("Unknown -l option '%c' in %s\n",optarg[1], optarg);
						return 1;
				}
				break;			
			case 'o':
				dname=strdup(optarg);
				break;
			case 'p':
				mode|=PEAK_SEARCH;
				break;
			case 'm':
				switch(optarg[0]){
					case 'c':
						mode=CAL;
						break;
					case 'g':
						mode=GAIN;
						break;
					default:
						printf("Unknown option to -m:'%s'\n",optarg);
						return 1;
				}
				break;
			case 's':
				span=atoi(optarg);
				break;
			case 't':
				sscanf(optarg,"%f",&sweeptime);
				break;
			case 'v':
				verbose=OPTION_DEBUG;
				break;
			default:
				printf("Unknown option %c\n",c);
				usage();
				return 1;
		}
	}
	
	/** Sanity checks  */
	
	if(slevel>10){
		printf("Not allowing level %f\n",slevel);
		return 1;
	}
	if(NULL ==analyzer_dev && NULL == signalgen_dev){
		printf("Must specify a controller device path (-p\n");
		usage();
		return 1;
	}
	/**set up device names  */
	if(NULL == signalgen_dev && NULL != analyzer_dev){
		printf("Using generator controller for spectrum analyzer\n");
		signalgen_dev=analyzer_dev;
	}
		
	if(NULL == analyzer_dev && NULL != signalgen_dev){
		printf("Using spectrum analyzer controller for generator\n");
		analyzer_dev=signalgen_dev;
	}
	if(CAL == mode){
		if(NULL ==calname){
			printf("Must specify -c when mode is CAL\n");
			return 1;
		}
		if(NULL ==(cal=fopen(calname,"w+"))){
			printf("Unable to open '%s'\n",calname);
			return 1;
		}
	}
/**INIT  */
	if(NULL == (analyzer_inst=init_HP8595E(analyzer_dev, GPIB_CTL_PROLOGIXS|verbose, analyzer_addr, NULL))) {
		return -1;
	}
	if(NULL == (signalgen_inst=init_PSG2400L(signalgen_dev, GPIB_CTL_PROLOGIXS|verbose, signalgen_addr, analyzer_dev==signalgen_dev?analyzer_inst->g:NULL))) {
		return -1;
	}
/**set up analyzer to base   */	
	sprintf(lbuf,"MKTYPE PSN;SNGLS;RB %f Hz;SP %f Hz;ST %f mS",rbw,span,sweeptime);
	write_string(analyzer_inst->g,lbuf);
/**set the signal generator level  */
	sprintf(lbuf,"CL %f dbM\n",slevel);
	write_string(signalgen_inst->g,lbuf);
/** PEAK search, if set  */
	printf("Resolution BW = %f Hz, span = %f Hz.\n",rbw,span);
	if( mode & PEAK_SEARCH){
	
		printf("Finding peak for start freq %f and level %f.\n",start,slevel);
		
		sprintf(lbuf,"CF %f Mhz",start);
		write_string(signalgen_inst->g,lbuf);
		x=((span-10)/1000000);
		
		printf("Span = %f Mhz, starting at %f\n",x,start-(x*10));
		
		for ( iter=start-(x*10),last_l=delta=1000,last_f=0; iter< start+(x*10); iter+=x){
			float y;
			if(verbose) printf("Looking at %f ",iter); 
			sprintf(lbuf,"CF %f MHz;TS;MKPK HI;MKA?",iter);
			write_wait_for_data(lbuf,10,analyzer_inst->g);
			sscanf(analyzer_inst->g->buf,"%f",&level);
			if(verbose) printf("%s ",analyzer_inst->g->buf); 
			sprintf(lbuf,"MKF?");
			write_wait_for_data(lbuf,10,analyzer_inst->g);
			sscanf(analyzer_inst->g->buf,"%f",&freq);
			if(verbose) printf(" Peak at %f",freq);
			y=fabs(fabs(level)-fabs(slevel/1000));
			/*printf("l=%f sl=%f y= %f d= %f ",level,slevel,y,delta); */
			if(y <delta){
				delta=y;
				last_f=freq;
				last_l=level;
				
				if(verbose) printf("*%f",last_f);
			}
			if(verbose) printf("\n");
		}
		last_f/=1000000;
	}	else{
		last_f=start;
	}
/**Start Main Loop  */		
	printf("Start = %f Mhz, Stop = %f Mhz, inc= %f Mhz. Offset=%f\n",start,stop,inc,last_f);
	for (iter=start; iter<=stop; iter+=inc){
		
		sprintf(lbuf,"CF %f Mhz",iter);
		
		write_string(signalgen_inst->g,lbuf);
		if(verbose) printf("Using %f for cf ",last_f);
		sprintf(lbuf,"CF %f MHz;TS;MKPK HI;MKA?",last_f);
		write_wait_for_data(lbuf,10,analyzer_inst->g);
		sscanf(analyzer_inst->g->buf,"%f",&level);
		if(verbose)printf("%s ",analyzer_inst->g->buf);
		sprintf(lbuf,"MKF?");
		write_wait_for_data(lbuf,10,analyzer_inst->g);
		sscanf(analyzer_inst->g->buf,"%f",&freq);
		freq/=1000000; /**convert to Mhz  */
		if( follow){
			/**use the last peak to estimate where the next one will be - 
			the signal generator may be off or not accurate enough...  */
			last_f=freq; /**convert to Mhz  */
			last_f +=inc;				/**set up for next measurement.  */			
		}else
			last_f+=inc;

		if(verbose)printf("%s (%f, %fMhz)\n",analyzer_inst->g->buf,freq,last_f);
		switch(mode){
			case CAL:
				fprintf(cal,"%f %f %f %f\n",iter,slevel,freq,level);
				printf("%f %f %f %f\n",iter,slevel,freq,level);
				break;
		}
	}
	if(NULL != cal)
		fclose(cal);
	return 0;
}
