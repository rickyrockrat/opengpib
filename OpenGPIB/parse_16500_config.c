/** \file ******************************************************************
\n\b File:        parse_16500_config.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/11/2010  3:48 am
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

#include "hp16500.h"

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{ fprintf(stderr,"parse_16500_config Version %s\n"
	"Usage: parse_16500_config <options>\n"
	" -c filename set name of config file\n"
	" -d filename set name of data file\n"
	" -f file set name of output file. Send valid data to file\n"
	" -m file set name of output file. Send label map to file\n"
#ifdef LA2VCD_LIB
	" -l file set name of output file. Send vcd data to file\n"
#endif
  " -v set verbose mode\n"
	"",TOSTRING(VERSION));
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char *argv[])
{
	char *cfname, *dfname, *mfname,*outfname,*vname;
	struct section *s;
	int c,v;
	v=SHOW_PRINT;
	s=NULL;
	outfname=dfname=mfname=cfname=vname=NULL;
	while( -1 != (c = getopt(argc, argv, "c:d:f:hl:m:v")) ) {
		switch(c){
			case 'c':
				cfname=strdup(optarg);
				break;
			case 'd':
				dfname=strdup(optarg);
				break;
			case 'f':
				outfname=strdup(optarg);
				break;
			case 'm':
				mfname=strdup(optarg);
				break;
			case 'l':
#ifdef LA2VCD_LIB
				vname=strdup(optarg);
				v=JUST_LOAD;
#else
				fprintf(stderr,"-l not supported. Re-build with LA2VCD_LIB=/path/to/lib\n");
				return 1;
#endif
				break;
        case 'v':
          v=SHOW_PRINT;
          break;
			default:
				usage();
				return 1;
		}
	}
	if(NULL == cfname && NULL == dfname){
		usage();
		goto closem;
	}	
	if(NULL != mfname && NULL != cfname ){
		
		if(NULL !=(s=parse_config(cfname,"CONFIG    ",JUST_LOAD) ) ){
			FILE *cfd=NULL;
	
			if(NULL == (cfd=fopen(mfname,"w+"))) {
				fprintf(stderr,"Unable to open '%s' for writing\n",mfname);
				goto closem;
			}
			
			config_show_labelmaps(s, cfd);
			if(NULL != cfd)
				fclose(cfd);
		}
	}	else	if(NULL != cfname ){
		s=parse_config(cfname,"CONFIG    ",v);
	}
	if(NULL != dfname ){
		struct data_preamble *p;
		printf("Parse Data\n");
		if(NULL != (p=parse_data(dfname,outfname,v))){
			printf("ParseData Done\n");
			if(JUST_LOAD==v && NULL !=s){
				if(NULL == vname){
					show_la2vcd(p,s,SHOW_PRINT);
					fprintf(stderr,"-td %ld ns\n",p->a1.sampleperiod/1000);
				}
#ifdef LA2VCD_LIB										
				else{
					struct la2vcd *l;
					struct signal_data *d,*x;
					char buf[500];
					if(NULL !=(d=show_la2vcd(p,s,JUST_LOAD))){
						/** first machine name=sec->data,second=(sec->data+0x40));*/
						if(NULL!=(l=open_la2vcd(vname,NULL,p->a1.sampleperiod*1e-12,0,NULL==s?NULL:s->data))){ 
							fprintf(stderr,"Opened la2vcd lib\n");
							
							if(vcd_add_file(l,NULL,16,d->bits,V_WIRE)){
								fprintf(stderr,"Failed to add input file\n");
								goto closevcd;
							}
							printf("Bits=%d\n",d->bits);
							/**Add our signal descriptions in  */
/*							vcd_add_signal (&l->first_signal,&l->last_signal, l->last_input_file,"zilch", 0, 0); */
							for (x=d;x;x=x->next){
								vcd_add_signal (l,V_WIRE, x->msb, x->lsb,x->name);
								/*printf("Added '%s' %d %d\n",x->name,x->msb,x->lsb); */
							}
	  					if(-1 == write_vcd_header (l)){
								fprintf(stderr,"VCD Header write failed\n");
								goto closevcd;
							}
							fprintf(stderr,"Wrote VCD Hdr\n");
              get_next_datarow(NULL,NULL);
							l->first_input_file->buf=buf;
							printf("r,bit=%d %d\n",l->first_input_file->radix,l->first_input_file->bit_count);
							while (1) {
								
								if(get_next_datarow(p,buf)){
									vcd_read_sample(l); 
									write_vcd_data (l);
									advance_time (l);
								} else
									break;
								
    					}

closevcd:
						 close_la2vcd(l);
						}	
					}

				}
#endif
			}
		}
	}
closem:
	return 0;
}
