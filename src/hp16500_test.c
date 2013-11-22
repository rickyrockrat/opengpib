/** \file ******************************************************************
\n\b File:        hp16500_test.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/15/2013 11:52 am
\n\b Description: Interface to test the HP16500C I/O
*/ /************************************************************************
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
#include "hp1653x.h"

enum {
	QID=0,
	QCARD,
	QCMD,
  QTEST,
};
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage(void)
{
	printf("hp16500, the HP16500 Query tool\n");
	show_common_usage(COM_USE_MODE_GENERIC);
	printf(" -c cmd execute command\n");
	printf(" -q type Set query type to: \n"
				 "   id - just print instrument ID\n"
	       "   cards - show cards in instrument\n"
	       " -v turn debug on.\n"
	       "\n"
	"");
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char * argv[])
{
	struct open_gpib_mstr *g;
	struct hp_common_options copt;
	int i, c, query,rtn,slot,debug;
	char *cmd;
	handle_common_opts(0,NULL,&copt);
	rtn=debug=0;
	i=-2;
	query=QTEST;
	cmd=NULL;
	while( -1 != (c = getopt(argc, argv, "c:q:hv"HP_COMMON_GETOPS)) ) {
		i=1;
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
				cmd=strdup(optarg);
				query=QCMD;
				break;
			case 'h':
				usage();
				return 1;
			case 'q':
				if( !strcmp(optarg,"id"))
					query=QID;
				else if(!strcmp(optarg,"cards"))
					query=QCARD;
				else{
					fprintf(stderr,"Unknown query '%s'\n",optarg);
					return 1;
				}
				break;
			case 'v':
				++debug;
				break;
			default:
				usage();
				return 1;
		}
	}
	if(-2 == i){
		usage();
		return 1;
	}
	copt.dtype|=OPTION_SET_DEBUG(debug);
	if(NULL ==copt.dev){
		fprintf(stderr,"Must specify device, or -a with ipaddress\n");
		return -1;
	}
	if(NULL == (g=open_gpib(copt.dtype,copt.iaddr,copt.dev,1048576))){
		fprintf(stderr,"Can't open/init controller at %s. Fatal\n",copt.dev);
		return 1;
	}
	/*open_gpib_list_interfaces(); */
	switch(query){
		case QID:
			if(0!=init_id(g,"*IDN?")){
				rtn=1;
				goto close;
			}
			fprintf(stderr," Addr %d, Method %d, device %s\n",copt.iaddr,copt.dtype,copt.dev);
			fprintf(stderr,"%s\n",g->buf);
			break;
		case QCARD:
			hp16500_find_card(copt.cardtype,copt.cardno,g);
			break;
		case QCMD:
			if(-1 == (slot=hp16500_find_card(copt.cardtype,copt.cardno,g)) ) {
				fprintf(stderr,"Unable to find timebase card\n");
				return -1;
			}
			select_hp_card(slot, g);
			sprintf(g->buf,"%s",cmd);
	    write_get_data(g,g->buf);	
      fprintf(stderr,"Sent '%s' Command. Reply: '%s'\n",cmd,g->buf);
      fprintf(stdout,"%s",g->buf);
			break;
    case QTEST:
      if(-1 == (slot=hp16500_find_card(copt.cardtype,copt.cardno,g)) ) {
				fprintf(stderr,"Unable to find timebase card\n");
				return -1;
			}
			select_hp_card(slot, g);
      fprintf(stderr,"Trigger source is %d\n",get_trigger_source(g));
      break;
	}
close:
	close_gpib(g);
	return rtn;
}

