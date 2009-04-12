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
Change Log: \n
$Log: not supported by cvs2svn $
*/
#include "common.h"
#include "gpib.h"

#define PSG2400L_IDN_STR "Wayne Kerr PSG2400L"
#define HP8595E_IDN_STR "HP8595E"
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
		if(NULL == (gi->g=open_gpib(type,addr,path))){
			printf("Can't open %s. Fatal\n",path);
			free(gi);
			return NULL;
		}	
	}
	if(NULL == gi->g)
		gi->g=g;
	gi->addr=addr;
	printf("Initializing Instrument\n");
	write_string(gi->g,"*CLS");
	if(0 == write_get_data(gi->g,"*IDN?")){
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
		if(NULL == (gi->g=open_gpib(type,addr,path))){
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
	" -a gaddr set signal generator addr.(9)\n"
	" -a aaddr set analyzer addr.(18)\n"
	" -c fname set cal in/out file to cal\n"
	" -d gpath set signal generator controller device name.(defaults to last device)\n"
	" -d apath set spectrum analyzer controller device name.(defaults to last device\n"
	"  at least one -d must be specified. If the second is not specified, it is assumed there\n"
	"  is only one controller, and the second defaults to the first\n"
	" -f efreq set end frequency to freq (in Mhz)\n"
	" -f sfreq set start frequency to freq(in Mhz)\n"
	" -i inc set frequency to inc. (in Khz)\n"
	" -l slevel set start dBm to level (-30dBm)\n"
	" -m mode set mode to mode. c=cal, g=gain.(g)\n"
	" -o fname put output to file called fname\n"
	
	"");
	
}
#define CAL 1
#define GAIN 2


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_wait_for_data(char *msg, int sec, struct gpib *g)
{
	int i,rtn;
	rtn =0;
	write_string(g,msg);
	for (i=0; i<sec;++i){
		if((rtn=read_string(g)))
			break;
		sleep(1);
	}
	return rtn;
}
/***************************************************************************/
/** .       GPIB_CTL_PROLOGIXS
\n\b Arguments:
\n\b Returns:
HP8595E
****************************************************************************/
int main(int argc, char * argv[])
{
	struct ginstrument *gi,*si;
	char *gdev, *sdev;
	int gaddr,saddr, mode,c;
	float start, stop, inc, iter,slevel;
	char *calname, *dname, lbuf[100];
	FILE *cal, *data;
	gi=si=NULL;
	gdev=sdev=calname=dname=NULL;
	cal=data=NULL;
	gaddr=9;
	saddr=18;
	mode=GAIN;
	start=stop=inc=0;
	slevel=-30;
	while( -1 != (c = getopt(argc, argv, "a:c:d:f:hi:l:m:o:")) ) {
		switch(c){
			case 'a':
				switch(optarg[0]){
					case 'g':
						gaddr=atoi(&optarg[1]);
						break;
					case 's':
						saddr=atoi(&optarg[1]);
						break;
					default:
						printf("Unknown -a option '%c' in %s\n",optarg[1], optarg);
						return 1;
				}
				break;
			case 'c':
				calname=strdup(optarg);
				break;
			case 'd':
				switch(optarg[0]){
					case 'g':
						gdev=strdup(&optarg[1]);
						break;
					case 's':
						sdev=strdup(&optarg[1]);
						break;
					default:
						printf("Unknown -p option '%c' in %s\n",optarg[1], optarg);
						return 1;
				}
				break;
			case 'f':
				switch(optarg[0]){
					case 'e':
						sscanf(&optarg[1],"%f",&stop);
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
			default:
				printf("Unknown option %c\n",c);
				usage();
				return 1;
		}
	}
	if(slevel>10){
		printf("Not allowing level %f\n",slevel);
		return 1;
	}
	if(NULL ==sdev && NULL == gdev){
		printf("Must specify a controller device path (-p\n");
		usage();
		return 1;
	}
	/**set up device names  */
	if(NULL == gdev && NULL != sdev){
		printf("Using generator controller for spectrum analyzer\n");
		gdev=sdev;
	}
		
	if(NULL == sdev && NULL != gdev){
		printf("Using spectrum analyzer controller for generator\n");
		sdev=gdev;
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
	if(NULL == (si=init_HP8595E(sdev, GPIB_CTL_PROLOGIXS, saddr, NULL))) {
		return -1;
	}
	if(NULL == (gi=init_PSG2400L(gdev, GPIB_CTL_PROLOGIXS, gaddr, sdev==gdev?si->g:NULL))) {
		return -1;
	}
	
	sprintf(lbuf,"MKTYPE PSN;SNGLS;RB 100 Hz;SP 1 KHz;ST 1 S");
	write_string(si->g,lbuf);
	sprintf(lbuf,"CL %f dbM\n",slevel);
	write_string(gi->g,lbuf);
	printf("Start = %f Mhz, Stop = %f Mhz, inc= %f Mhz\n",start,stop,inc);
	for (iter=start; iter<=stop; iter+=inc){
		char lbuf[100];
		float level,freq;
		sprintf(lbuf,"CF %f Mhz",iter);

		write_string(gi->g,lbuf);
		sprintf(lbuf,"CF %f MHz;TS;MKPK HI;MKA?",iter);
		write_wait_for_data(lbuf,10,si->g);
		sscanf(si->g->buf,"%f",&level);
		printf("%s ",si->g->buf);
		sprintf(lbuf,"MKF?");
		write_wait_for_data(lbuf,10,si->g);
		sscanf(si->g->buf,"%f",&freq);
		printf("%s\n",si->g->buf);
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
