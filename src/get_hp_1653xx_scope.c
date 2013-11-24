/** \file ******************************************************************
\n\b File:        get_hp_165xx_scope.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  6:43 am
\n\b Description: Get waveforms from the HP 16530 timebase card.
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

#include <open-gpib.h>
#include "hp16500.h"
#include "hp1653x.h"

/**for open...  */
#include <sys/stat.h>
#include <fcntl.h>

/** 
The channels correspond to Slot,channel number, so Slot D, channel 1 is D1.
The first channel is the lowest slot, so if it starts with C1, the channels start at C1
and go up. D1 is channel 3.

voltage=(dataval - yreference)*yincrement	+yorigin
time=(dataptno - xreference) *xicrement	+xorigin
Trigger is datapoint closest to time 0.

*idn?
++read
HEWLETT PACKARD,16500C,0,REV 01.00
The cardcage command returns what is in each slot, starting at A
:cardcage?
++read
34,-1,12,12,11,1,0,5,5,5
12 is the o-scope, 11 is the timebase.
select sets the module to talk to: 0=intermodule, 1-5 selects A-E
SELECT 5

channels are 1, 2, 3, etc.
:WAVeform:SOURce CHANnel1
FULL gives 4096 points, window gives just what is in window.
:WAVeform:RECord FULL|WINDow

Get the preable:
format,type,points,count,Xincrement,Xorigin,Xreference,Yincrement,Yorigin,Yreference
:WAVEFORM:SOURCE CHANNEL1;PRE?
++read
0,1,4096,1,+2.00000E-07,-8.25190E-05,0,+3.66211E-04,+2.50000E+00,8192

All scope commands should be preceeded by :WAVEFORM:SOURCE CHANNELx;CMD

This gives the trigger location...This is the xorigin as relates the trigger.
:WAVEFORM:XORIGIN?
++read
-8.25190E-05

:WAVEFORM:PREAMBLE 0,1,4096,1,  +4.00000E-10,-5.07444E-06,0,  +4.88281E-05,+2.60000E+00,8192
:WAVEFORM:XORIGIN -5.07444E-06

:WAVEFORM:RECORD FULL
:WAV:REC FULL
:WAVEFORM:RECORD WINDOW
:WAVEFORM:FORMAT ASCII

:WAVEFORM:SOURCE CHANNEL3;PRE?
:WAVEFORM:PREAMBLE 0,1,500,1,+4.00000E-10,-1.01937E-07,0,+9.76562E-05,+1.10000E-01,8192

:WAVEFORM:SOURCE CHANNEL3;DATA?


									FMT,TYPE,Points,Count,Xinc,Xorigin,Xref,Yinc,Yorigin,Yref
:WAVEFORM:PREAMBLE 0,1,500,1,+1.00000E-09,-2.51937E-07,0,+9.76562E-05,+1.10000E-01,8192


:MEASURE:SOURCE CHANNEL3;VPP?
:WAVEFORM:SOURCE CHANNEL1;PRE?
:WAVEFORM:PREAMBLE 0,1,500,1,+1.00000E-09,-2.51802E-07,0,+4.88281E-04,+2.80000E+00,8192

The ASC data format is same as WORD, which is as follows: 
TYPE						MSB									LSB
NORMAL NU|OV|M5|M4|M3|M2|M1|M0 	 00|00|00|00|00|00|00|00
AVE		 NU|OV|M5|M4|M3|M2|M1|M0	 A7|A6|A5|A4|A3|A2|A1|A0
BYTE is 
	NU|OV|M5|M4|M3|M2|M1|M0
ASC is 0
BYTE is 1
WORD is 2
	
Data conversion:
Voltage=(Value-Yreference)*Yinc +Yorigin
Time= (point# -Xref)*Xinc + Xorigin

*/



/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	fprintf(stderr,"get_hp_16530_waveform <options>\n");
	show_common_usage(COM_USE_MODE_SCOPE);
	/**common options are -a, -d, -n, -m, and -t */
	fprintf(stderr,"\n -c n Use channel n for data (ch1). ch1-ch?\n"
	"    The -c option can be used multiple times. In this case, the fname\n"
	"    is a base name, and the filename will have a .ch1 or .ch2, etc appended\n"
  " -f set the format. 0=ASCII, 1=byte, and 2=word. If not set, then set according to instrument\n"
	" -g set gnuplot mode, with x,y (time volts), one point per line\n"
	" -o fname put output to file called fname\n"
  " -p hno Prefix file header to hno. 0=none,just x y. 1= title for x y, 2 = full header(2)\n"
	" -r set raw mode (don't convert data to volts)\n"
  " -u set unprocess mode, dump preamble at top and raw data after. -r,-g have no effect.\n"
#ifdef HAVE_LIBLA2VCD2
	" -v put vcd data to fname.vcd. Creates .chx also\n"
#endif	
	
	"\n Channels are set up such that for C1 C2 D1 D2, C1= ch1, C2=ch2, D1=ch3, D2=ch4, etc.\n"
	" So if you want C1 use ch1 as the -c option.\n"
	
	"");
}
#define HDR_NONE  0
#define HDR_TITLE 1
#define HDR_FULL  2
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct open_gpib_mstr *g;
	struct hp_scope_preamble h;
	struct hp_common_options copt;
	FILE *ofd;
  double miny, maxy, time, time_inc,toff;
	char *ofname, *channel[MAX_CHANNELS], *lbuf;
	int i, c, rtn, ch_idx, raw,xy,process,hdr;
#ifdef HAVE_LIBLA2VCD2	 
  int vcd=0;
  struct la2vcd *l=NULL;
#endif
	miny=maxy=0; /**kill -Wextra warnings  */
  process=1;
	channel[0]="ch1";
	
	handle_common_opts(0,NULL,&copt);
	copt.cardtype=CARDTYPE_16534A;
  copt.fmt=FMT_WORD;
	for (c=1;c<MAX_CHANNELS;++c){
		channel[c] = NULL;
	}
	ch_idx=0;
	rtn=1;
	ofname=NULL;
	ofd=NULL;
	raw=xy=0;
  hdr=HDR_FULL;
	while( -1 != (c = getopt(argc, argv, "f:gc:hp:o:ruv"HP_COMMON_GETOPS)) ) {
		switch(c){
			case 'a':
			case 'd':
			case 'm':
			case 'n':
			case 't':
				if(handle_common_opts(c,optarg,&copt))
					return -1;
				break;
			case 'c':
				if(ch_idx>=MAX_CHANNELS){
					fprintf(stderr,"Too many -c options. Max of %d allowed.\n",MAX_CHANNELS);
				}
				channel[ch_idx++]=strdup(optarg);
				break;

      case 'f':
        copt.fmt=atoi(optarg);
        break;
			case 'h':
				usage();
				return 1;
			case 'g':
				xy=1;
				break;
			
			case 'o':
				ofname=strdup(optarg);
				break;
      case 'p':
        hdr=atoi(optarg);
        if(hdr< HDR_NONE || hdr>HDR_FULL){
          fprintf(stderr,"Invalid -p option (%d)\n",hdr);
        }
        break;
			case 'r':
				raw=1;
				break;
      case 'u':
        process=0;
        break;
#ifdef HAVE_LIBLA2VCD2	  			
      case 'v':
				++vcd;
				break;
#endif        
			default:
				usage();
				return 1;
		}
	}
  h.fmt=copt.fmt;
  if(FMT_WORD == copt.fmt)
    h.data_size=2;
  else if(FMT_BYTE == copt.fmt)
    h.data_size=1;
  else {
    printf("Can't handle data format %d\n",copt.fmt);
    return -1;
  }
	if(NULL == ofname){
		ofd=fdopen(1,"w+");
	}
		
	if(0 == ch_idx)	/**Use default, and one channel  */
		++ch_idx;	
	else {
		for (c=0;c<ch_idx;++c){
			if(0==check_oscope_channel(channel[c])){
				fprintf(stderr,"Channel %s is invalid\n",channel[c]);
				return 1;
			}
		}
	}
	if(NULL == (g=open_gpib(copt.dtype,copt.iaddr,copt.dev,-1))){
		fprintf(stderr,"Can't open/init controller at %s. Fatal\n",copt.dev);
		return 1;
	}
	
	if(-1 == init_oscope_instrument(&copt,g)){
		fprintf(stderr,"Unable to initialize instrument.\n");
		fprintf(stderr,"Did you forget to set 16500C controller 'Connected To:' HPIB?\n");
		goto closem;
	}
	if(NULL != ofname){
		/**find max strlen of channel name.  */
		for (i=c=0;c<ch_idx;++c){
			int x;
			x=0;
			if(NULL != channel[c])
				x=strlen(channel[c]);
			if(x>i)
				i=x;
		}
		i+=10;
		/**allocate buffer for all fnames.  */
		if(NULL == (lbuf=malloc(strlen(ofname)+i)) ){
			fprintf(stderr,"Out of mem for lbuf alloc\n");
			goto closem;
		}
	}else{
    lbuf=NULL;
#ifdef HAVE_LIBLA2VCD2	
    fprintf(stderr,"Can't output .vcd to stdout\n");
    vcd=0;
  }

  if(vcd){
    sprintf(lbuf,"%s.vcd",ofname);
    if(NULL==(l=open_la2vcd(lbuf,NULL,1,0,"Oscilliscope"))){ 
			fprintf(stderr,"Unable to open la2vcd lib\n");
      vcd=0;
		}	  
    if(vcd){
      if(vcd_add_file(l,NULL,2,64,V_REAL)){ /**set the sample size up  */
    	 fprintf(stderr,"Failed to add input file\n");
       vcd=0;
      }   
      if(vcd) {
        for (c=0;c<ch_idx && NULL != channel[c];++c)
          vcd_add_signal (l,V_REAL, 0, 0,channel[c]);
        if(-1 == write_vcd_header (l)){
    			fprintf(stderr,"VCD Header write failed\n");
    			vcd=0;
    		}
    		fprintf(stderr,"Wrote VCD Hdr\n");  
        fflush(NULL);
      }
    }
  }
#else
  }  
#endif        
    
	fprintf(stderr,"Channel loop\n");
	/**channel loop. Open file, dump data, close file, repeat  */
	for (c=0;c<ch_idx && NULL != channel[c];++c){
		int point;
    if(NULL != ofname && NULL != lbuf ){ /**we have valid filename & channel, open  */
      sprintf(lbuf,"%s.%s",ofname,channel[c]);
      if(NULL == (ofd=fopen(lbuf,"w+"))) {
        fprintf(stderr,"Unable to open '%s' for writing\n",lbuf);
        goto closem;
      }
    }  
  	fprintf(stderr,"Reading Channel %s\n",channel[c]);
  	if( 0 >= (i=oscope_get_preamble(g,channel[c],&h))){
  		fprintf(stderr,"Preable failed on %s\n",channel[c]);
  		goto closem;
  	}
#ifdef HAVE_LIBLA2VCD2	
    if(vcd && NULL !=l)/**marvelous hack...  */
      l->time_delta=h.xinc;
#endif
      /*fprintf(stderr,"Got %d bytes of preamble '%s'\n",i,g->buf); */
  /** Voltage=(Value-Yreference)*Yinc +Yorigin
  	Time= (point# -Xref)*Xinc + Xorigin */
    if(!process)
  	  fwrite(g->buf,1,i,ofd);	
  	if(-1 == (i=get_oscope_data(g,channel[c],&h)) ){
  		fprintf(stderr,"Unable to get waveform??\n");
  		goto closem;
  	}	
    /*fprintf(stderr,"data_len=%ld, Start of Data %d (%02x %02x)\n",h.point_len, h.point_start,g->buf[h.point_start-1],g->buf[h.point_start]); */
      
  	point=0;
    time=0;
  	time_inc=(h.xinc*h.xincmult);
    toff= (h.xref*h.xincmult)+(h.xorg*h.xincmult);
    if(xy){
		/*save room for our min/max y*/
    if(HDR_FULL == hdr)
	     fprintf(ofd,"                                                         ");
    if(HDR_FULL == hdr||HDR_TITLE == hdr)  
	    fprintf(ofd,"%s Volts\n",h.xunits);
	  miny=maxy=0;
    }
  	while(i>0){	/**process the first block of data received  */
      if(process){
				int x;
        
				for (x=h.point_start;x<i;x+=h.data_size){
					double volts;
          unsigned int rawdat;
          fflush(NULL);
          rawdat=g->buf[x];
          if(2 == h.data_size){
            rawdat<<=8;
            rawdat+=g->buf[x+1];
          }
          time+=time_inc;  
					/*time=((time_inc*(double)(point))); */
          volts=((double)(rawdat)-h.yref)*h.yinc + h.yorg;
					if(0==point){
            fprintf(stderr,"%e %e %e %e v%e\n",h.xinc,time, time_inc,h.xorg,volts); 
          }
						
					
          /**skip values that are out of range.  */
					if(401 > volts && -401 < volts) {
  					if(xy){
  						if(volts>maxy) maxy=volts;
  						if(volts<miny) miny=volts;
  						if(raw)
  							fprintf(ofd,"%f %d\n",time,rawdat);
  						else
  							fprintf(ofd,"%e %e\n",time,volts);	
  					}	else {
  						if(raw)
  							fprintf(ofd,"%d,",rawdat);
  						else
  							fprintf(ofd,"%e,",volts);	
  					}
          
#ifdef HAVE_LIBLA2VCD2
          
            if(vcd ){
  				    vcd_read_sample_real(l,volts); 
  	          write_vcd_data (l);
  				    advance_time (l);
            }
          }else if(vcd) {
            fprintf(stderr,"Discarding %f %f\n",time,volts);
            advance_time(l);
          }
            
#else
          }
#endif
          
					++point;
				} /**end x loop  */
      } else { /**! process  */
        fwrite(g->buf,1,i,ofd);
      }
		  i=read_string(g);
      h.point_start=0;
  	} /**end read block loop  */
		
		/*fwrite(g->buf,1,i,ofd);	 */
		if(NULL != ofname && NULL != ofd){
      if(HDR_FULL == hdr){
        fseek(ofd,0,SEEK_SET);
  			fprintf(ofd,"%e %e %e %e\n",h.xorg,time,miny,maxy);  
      }
			
			fclose(ofd);
			ofd=NULL;
		}
	} /**end channel loop  */
  
	rtn=0;
  /*fprintf(stderr,"Done!\n"); */
  fflush(NULL);
closem:
#ifdef HAVE_LIBLA2VCD2
  if(vcd)
    close_la2vcd(l);  
#endif
	if(NULL != ofd)
		fclose(ofd);
	close_gpib(g);
	return rtn;
}


