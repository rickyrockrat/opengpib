/** \file ******************************************************************
\n\b File:        hp16500.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/17/2010 10:44 am
\n\b Description: Routines specific to hp16500, or helper routines created
to support the hp16500.
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
Change Log: \n

*/
#include "common.h"
#include "hp16500.h"
#include "gpib.h"
#include <ctype.h>
#define SIZEOF_ACTIVE_ARRAY 13

struct signal_data *show_vcd_label(char *a, struct labels *l);

struct valid_bits {
	uint8 pod[2*SIZEOF_ACTIVE_ARRAY];
	char active[SIZEOF_ACTIVE_ARRAY];
	int bytes;
	int bits;
};

struct hp_cards hp_cardlist[]=\
{
	{CARDTYPE_16500C,   MODEL_16500C,  DESC_16500C},
  {CARDTYPE_16515A,   MODEL_16515A,  DESC_16515A},
	{CARDTYPE_16516A,   MODEL_16516A,  DESC_16516A},
	{CARDTYPE_16517A,   MODEL_16517A,  DESC_16517A},
	{CARDTYPE_16518A,   MODEL_16518A,  DESC_16518A},
	{CARDTYPE_16530A,   MODEL_16530A,  DESC_16530A},
	{CARDTYPE_16531A,   MODEL_16531A,  DESC_16531A},
	{CARDTYPE_16532A,   MODEL_16532A,  DESC_16532A},
	{CARDTYPE_16534A,   MODEL_16534A,  DESC_16534A},
	{CARDTYPE_16535A,   MODEL_16535A,  DESC_16535A},
	{CARDTYPE_16520A,   MODEL_16520A,  DESC_16520A},
	{CARDTYPE_16521A,   MODEL_16521A,  DESC_16521A},
	{CARDTYPE_16522AE,  MODEL_16522AE,  DESC_16522AE},
	{CARDTYPE_16522AM,  MODEL_16522AM,  DESC_16522AM},
	{CARDTYPE_16511B ,  MODEL_16511B ,  DESC_16511B },
	{CARDTYPE_16510A ,  MODEL_16510A ,  DESC_16510A },
	{CARDTYPE_16550AE,  MODEL_16550AE,  DESC_16550AE},
	{CARDTYPE_16550AM,  MODEL_16550AM,  DESC_16550AM},
	{CARDTYPE_16554M ,  MODEL_16554M ,  DESC_16554M },
	{CARDTYPE_16554E ,  MODEL_16554E ,  DESC_16554E },
	{CARDTYPE_16540A ,  MODEL_16540A ,  DESC_16540A },
	{CARDTYPE_16541A ,  MODEL_16541A ,  DESC_16541A },
	{CARDTYPE_16542A ,  MODEL_16542A ,  DESC_16542A },
	{CARDTYPE_INVALID,NULL},
};

/** Generic functions for accessing the HP16500C go here.  */




/***************************************************************************/
/** Returns index to info about the card type.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_hp_info_card(int id)
{
	int i;
	for (i=0;NULL != hp_cardlist[i].desc;++i){
	  //fprintf(stderr,"%d - %s\n",hp_cardlist[i].type, hp_cardlist[i].desc);
		if(id==hp_cardlist[i].type){
			return i;
		}
	}
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: index into instrument array
****************************************************************************/
int print_card_model(int id)
{
	int i;
	if(-1 != (i=get_hp_info_card(id)))
		fprintf(stderr,"HP %s %s",hp_cardlist[i].model,hp_cardlist[i].desc);
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments: slot is 1-5
\n\b Returns:
****************************************************************************/
int select_hp_card(int slot, struct gpib *g)
{
	char *model;
	if(NULL == g->inst )
		model="Unknown";
	else 
		model=((struct card_info *)g->inst)->info->model;
	write_get_data(g,":SELECT?");

	if(slot == (g->buf[0]-0x30)){
		fprintf(stderr,"Slot %c (%s) %s already selected\n",0x30!=g->buf[0]?g->buf[0]+'A'-'0'-1:g->buf[0],model,g->buf);
	}	else{
		sprintf(g->buf,":SELECT %d",slot);
		write_string(g,g->buf);	
	}
	write_get_data(g,":SELECT?");
/*	i=write_get_data(g,":?"); */
	fprintf(stderr,"Selected Card %c to talk to.\n",0x30!=g->buf[0]?g->buf[0]+'A'-'0'-1:g->buf[0]);
	return 0;

}
/***************************************************************************/
/** Print list of all known cards.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_known_hp_cards( void )
{
	int i;
	fprintf(stderr,"List of Known HP16500 Mainframe Cards\n"
	"Type - Model Description\n");
	for (i=0;NULL != hp_cardlist[i].desc;++i){
	  fprintf(stderr,"%2d - HP %s %s\n",hp_cardlist[i].type, hp_cardlist[i].model, hp_cardlist[i].desc);
	}
}

/***************************************************************************/
/** Make sure we are talking to the right piece of test equipment and 
 the correct card is found.

\n\b Arguments: if card type is -1, assume we just want to print out the slots.
\n\b      If no is 0, we use the last card found.
\n\b Returns: card number found (1-5). 0 is system.
****************************************************************************/
int hp16500_find_card(int cardtype, int no, struct gpib *g)
{
	int i,s, info,ccount;
	int slots[5], slot=-1;
	if(-1 != cardtype){
		if(-1 == (info=get_hp_info_card(cardtype)) ){
			fprintf(stderr,"Unknown card type %d\n",cardtype);
			show_known_hp_cards();
			return -1;
		}	
		fprintf(stderr,"Looking for card %s\n",hp_cardlist[info].model);
		if(NULL == (g->inst=calloc(1,sizeof(struct card_info))) ){
			fprintf(stderr,"Out of memory allocating card_info structure\n");
			return -1;
		}
		((struct card_info *)g->inst)->info=&hp_cardlist[info];
	}

	fprintf(stderr,"Initializing Instrument\n");
	if(0!=init_id(g,"*IDN?"))
		return -1;
	/*fprintf(stderr,"Got %d bytes\n",i); */
	if(NULL == strstr(g->buf,"HEWLETT PACKARD,16500C")){
		fprintf(stderr,"Unable to find 'HEWLETT PACKARD,16500C' in id string '%s'\n",g->buf);
		return -1;
	}
  if(CARDTYPE_16500C == cardtype){ /**talking to system  */
    return SLOTNO_16500C;
  }else{
    if(0 == write_get_data(g,":CARDCAGE?")){
  		fprintf(stderr,"Error sending Cardcage command\n");
  		return -1;
  	}
  	
  	for (s=0;g->buf[s];++s)
  		if(isdigit(g->buf[s]))
  			break;
  	for (ccount=i=0;i<5;++i){
  		slots[i]=(int)get_value_col(i,&g->buf[s]);
  		fprintf(stderr,"Slot %c = %d - ",'A'+i,slots[i]);
  		print_card_model(slots[i]);
  		fprintf(stderr,"\n");
  		if(cardtype == slots[i]){
  			++ccount;
  			if((-1 ==slot && 0== no) || ccount == no){
  				slot=i;
  				if(-1 != cardtype) ((struct card_info *)g->inst)->slot=i;	
  			}
  		}
  	}
  	if( -1 != cardtype){
  		if(-1 == slot){
  			fprintf(stderr,"Unable to find %s card\n",hp_cardlist[info].model);
  		}	else{
  			fprintf(stderr,"Found ");
  			print_card_model(slots[slot]);
  			fprintf(stderr," in slot %c\n",'A'+slot);	
  		}		
  		return slot+1;
  	}
  	return 0;  
  }
	
	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_hp_connection_info(void)
{
	printf("\nMake sure the 16550C Controller is connected to:\n"
	" LAN for  'hpip' or\n"
	" HPIB for 'prologixs'\n"
	"Supported controllers\n\n"
	"");
	show_gpib_supported_controllers();	
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_common_usage (int mode)
{
  char *emsg=NULL;
  int type;
  switch(mode){
    case COM_USE_MODE_SCOPE: 
      type=CARDTYPE_16534A;  
      emsg="   Current types are 11=16530, 13=16532, 14=16534\n";
    break;
    case COM_USE_MODE_LOGIC: type=CARDTYPE_16554M; break;
    default: type=0; break;
  }
	printf("Common Options\n"
	" -a addr set GPIB instrument address to addr (ip address automatically sets -d, -m)\n"
	" -d dev set path to device name (use ipaddress for hpip)\n"
	" -m meth set access method to meth, currently hpip or prologixs\n"
	" -n num Set card number, use for multiple cards in system. 0 selects first card found.\n"
	" -t type Set card type (%d)\n%s"
	"",type,emsg);
	show_known_hp_cards();
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int handle_common_opts(int c, char *optarg, struct hp_common_options *o)
{
  int i;
	if(0 == c && NULL == optarg){
		o->iaddr=7;
		o->cardno=0;
		o->cardtype=-1;
		o->iaddr=7;
	  o->dev=NULL;
	  o->dtype=GPIB_CTL_HP16500C;
		return 0;
	}
	switch(c){
    case 'a':
      if(is_string_number(optarg)){
				o->iaddr=atoi(optarg);
				o->dtype=gpib_option_to_type("prologixs");
			} else{ /**Assume it's an IP address  */
				o->dev=strdup(optarg);
				o->dtype=gpib_option_to_type("hpip");
			}
			break;
		case 'd':
			o->dev=strdup(optarg);
			break;
		case 'm':
			if(-1 == (o->dtype=gpib_option_to_type(optarg)))	{
				fprintf(stderr,"'%s' method not supported\n",optarg);
				return 1;
			}
			break;
		
		case 'n':
			o->cardno=atoi(optarg);
			if(o->cardno>5 || o->cardno <0){
				fprintf(stderr,"-n: '%s' Invalid card number, must be 0-5\n",optarg);
				return 1;
			}
			break;
		
		case 't':
			o->cardtype=atoi(optarg);
      for (i=0;CARDTYPE_INVALID != hp_cardlist[i].type;++i){
        if(hp_cardlist[i].type == o->cardtype )
          break;
      }
        
			if(CARDTYPE_INVALID == hp_cardlist[i].type){
			  fprintf(stderr,"'-t: %s' Invalid card type\n",optarg);							
			  return 1;
			}
      if(FMT_NONE == o->fmt){
        if(CARDTYPE_16530A == o->cardtype )
          o->fmt=FMT_BYTE;
        else
          o->fmt=FMT_WORD;
      }  
			break;
		}
	return 0;
}

#if defined (MAIN_QUERY)

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
	struct gpib *g;
	struct hp_common_options copt;
	int i, c, query,rtn,slot;
	char *cmd;
	handle_common_opts(0,NULL,&copt);
	rtn=0;
	i=-2;
	query=QTEST;
	cmd=NULL;
	while( -1 != (c = getopt(argc, argv, "c:q:h"HP_COMMON_GETOPS)) ) {
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
			default:
				usage();
				return 1;
		}
	}
	if(-2 == i){
		usage();
		return 1;
	}
	if(NULL ==copt.dev){
		fprintf(stderr,"Must specify device, or -a with ipaddress\n");
		return -1;
	}
	if(NULL == (g=open_gpib(copt.dtype,copt.iaddr,copt.dev,1048576))){
		fprintf(stderr,"Can't open/init controller at %s. Fatal\n",copt.dev);
		return 1;
	}
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
#endif

#if 0
enum {
		  ACCESS=1,	 /**Access method GPIB, IP, etc.  */
			IADDR,		 /**Instrument Address 0-30, assume GPIB, if dotted, assume net. */
			DEVICE,		 /**device - Access method   */
}

struct gpib_options {
	int type; 				/**an ENUM from above  */
	int optval; 			/** option value, i.e. -'a'  */
	char description;	/**descriptive text to use with usage prints  */
	char *val; 				/**value of option  */
};

struct gpib_optons common[]={
	{ACCESS,'m',NULL},
	{IADDR,'a',NULL},
	{DEVICE,'d',NULL},
};


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void parse_options(struct options *o)
{
	struct gpib *g;
	FILE *ofd,*cfd,*tfd,*vfd;
	char *name, *ofname;
	char *tname,*cname,*vname;
	int i, c,inst_addr, rtn, raw,dtype,trigger,slot, speriod;
	long int total;
	inst_addr=7;
	rtn=1;
	trigger=0;
	name=ofname=tname=cname=vname=NULL;
	vfd=tfd=cfd=ofd=NULL;
	raw=0;
	dtype=GPIB_CTL_HP16500C;
	while( -1 != (c = getopt(argc, argv, "a:c:d:hmo:pr:t:v:")) ) {
		switch(c){
			case 'a':
				inst_addr=atoi(optarg);
				break;
			case 'c':
				if(NULL != vname){
					fprintf(stderr,"-v and -c are mutually exclusive.\n");
					usage();
					return 1;
				}
				cname=strdup(optarg);
				break;
			case 'd':
				name=strdup(optarg);
				break;
			case 'h':
				usage();
				return 1;
			case 'm':
				if(-1 == (dtype=gpib_option_to_type(optarg)))	{
					fprintf(stderr,"'%s' method not supported\n",optarg);
					return -1;
				}
				break;
			case 'o':
				ofname=strdup(optarg);
				break;
			case 'p':
				raw=1;
				break;
			case 'r':
				speriod=atoi(optarg);
				if(0!=speriod){
					if(-1 ==validate_sampleperiod(speriod)){
						fprintf(stderr,"Invalid period %dns (must be 2,4,8 or 8ns increments)\n",speriod);
						return -1;
					}	
				}
				
				trigger=1;
				break;
			case 't':
				if(NULL != vname){
					fprintf(stderr,"-v and -t are mutually exclusive.\n");
					usage();
					return 1;
				}
				tname=strdup(optarg);
				break;
			case 'v':
#ifdef LA2VCD_LIB 
				if(NULL != tname || NULL != cname){
					fprintf(stderr,"-v and -c/-tare mutually exclusive.\n");
					usage();
					return 1;
				}				
				if(NULL ==(vname=malloc( (strlen(optarg)+5)*3 ))){
					fprintf(stderr,"Unable to malloc filename space for '%s'\n",optarg);
					return 1;
				}
				i=1+sprintf(vname,"%s.vcd",optarg);
				
				cname=&vname[i];
				i+=1+sprintf(cname,"%s.cfg",optarg);
				tname=&vname[i];
				sprintf(tname,"%s.dat",optarg);
#else
				fprintf(stderr,"-v not supported. Re-build with LA2VCD_LIB=/path/to/lib\n");
				return 1;
#endif
				break;
			default:
				usage();
				return 1;
		}
	}	
}
#endif

/**#######################################################################
                     oscilliscope functionality
   #######################################################################*/



/***************************************************************************/
/** ":CHAN1:OFFS 0;:CHAN2:OFFS 0;:CHAN3:OFFS 0;:CHAN4:OFFS 0".
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_channel_offset(struct gpib *g, int channel, float offset)
{
  return -1;
}


/***************************************************************************/
/** ":CHAN1:RANG 1.6;:CHAN2:RANG 1.6;:CHAN3:RANG 1.6;:CHAN4:RANG 1.6" .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_channel_range(struct gpib *g)
{
  return -1;
}

/***************************************************************************/
/** Look for the trigger source, comes back as:
CHANNEL1
CHANNEL2
CHANNELx
EXTERNAL
\n\b Arguments:
\n\b Returns: 0 for external, or 1-x for channel, or -1 on error
****************************************************************************/
int get_trigger_source (struct gpib *g)
{
  sprintf(g->buf,":TRIG:SOUR?");
  if(0 == write_get_data(g,g->buf))
    return -1;
  if(!strncmp(g->buf,"EXTERNAL",8) )
    return 0;
  return(atoi(&g->buf[7]) );
}

/***************************************************************************/
/** We want the multiplier that takes our xinc value from xe-y to xe0.
we also want to use ms,us, etc, so do it by powers of 1000.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
double get_xinc_mult(double v, int *x)
{
  double i;
  int z;
  /**for some reason, if you use 1 here, apparently 1e-6 *1e6 != 1   */
  for (z=0,i=1;(double)(v*i)<(double)(.0001);++z,i*=1000);
  if(0 &&z){/**allow .1-.9  */
    --z;
    i/=1000;
  }  
/*    printf("%d %e %e -> %e (%d)\n",z,i,v,v*i,(int)(v*i)); */
  *x=z;
  return i;
}
/***************************************************************************/
/** FMT,TYPE,Points,Count,Xinc,Xorigin,Xref,Yinc,Yorigin,Yref.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int oscope_parse_preamble(struct gpib *g, struct hp_scope_preamble *h)
{
	h->fmt=get_value_col(0,g->buf);
	h->type=get_value_col(1,g->buf);
	h->points=get_value_col(2,g->buf);
  h->count=get_value_col(3,g->buf);
	h->xinc=get_value_col(4,g->buf);
	h->xorg=get_value_col(5,g->buf); /**first data point (in seconds) w/respect to trigger, i.e.  */
                                   /**trigger point is this value * -1, and data point number=
                                       xorg/xinc. */
	h->xref=get_value_col(6,g->buf);
	h->yinc=get_value_col(7,g->buf);
	h->yorg=get_value_col(8,g->buf);
	h->yref=get_value_col(9,g->buf);
  h->xincmult=get_xinc_mult(h->xinc,&h->xinc_thou);
  switch(h->xinc_thou){
    case 0: snprintf(h->xunits,3,"S"); break;
    case 1: snprintf(h->xunits,3,"mS"); break;
    case 2: snprintf(h->xunits,3,"uS"); break;
    case 3: snprintf(h->xunits,3,"nS"); break;
    case 4: snprintf(h->xunits,3,"pS"); break;
    default: snprintf(h->xunits,3,"?S"); break;  
  }
  fprintf(stderr,"%s\n",g->buf);
  fprintf(stderr,"fmt %d type %d points %d count %d xinc %e xorg %e xref %e yinc %e yorg %e yref %e xmult %e inc_thou %d '%s'\n",
  h->fmt, h->type,h->points, h->count,h->xinc,h->xorg,h->xref,h->yinc,h->yorg,h->yref,h->xincmult,h->xinc_thou,h->xunits);
	return 0;
}


/***************************************************************************/
/** returns the value of the digits at the end. 0 is invalid channel number.
\n\b Arguments:
\n\b Returns: 0 on error, channel no on success.
****************************************************************************/
int check_oscope_channel(char *ch)
{
	int i,b;
	if(NULL ==ch)
		return 0;
	for (b=-1,i=0;ch[i];++i){
		if(isdigit(ch[i])){
			b=i;
			break;
		}
	}
	if( -1 == b ){
		fprintf(stderr,"Channel '%s' is invalid\n",ch);
		return 0;
	}
	return atoi(&ch[b]);
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int oscope_get_preamble(struct gpib *g,char *ch, struct hp_scope_preamble *h)
{
	int i;
	if(NULL == ch)
		return -1;
	if(0 == (i=check_oscope_channel(ch)))
		return -1;
  /**check to see if there is valid data  */
  sprintf(g->buf,":WAV:VAL?");
  if( write_get_data(g,g->buf) <=0){
		fprintf(stderr,"Unable to check valid data\n");
		return 0; 
	}
  if('0' ==  g->buf[0]){
    fprintf(stderr,"There is no valid data. Have you triggered the scope?\n");
    return 0;
  }
	/*fprintf(stderr,"GetPre %d\n",i);	  */
	sprintf(g->buf,":WAV:SOUR CHAN%d;PRE?",i);
	if( (i=write_get_data(g,g->buf)) <=0){
		fprintf(stderr,"No data from preamble (%d)\n",i);
		return 0; 
	}
		
	oscope_parse_preamble(g,h);
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_oscope_data(struct gpib *g, char *ch, struct hp_scope_preamble *h)
{
	int i,x;
	if(NULL == ch)
		return -1;
	if(0 == (i=check_oscope_channel(ch)))
		return -1;
/*	fprintf(stderr,"GetData %d\n",i); */
	sprintf(g->buf,":WAV:SOUR CHAN%d;DATA?",i);
	i=write_get_data(g,g->buf);	
  h->point_len=strtol(&g->buf[2],NULL,10);
  for (x=2;g->buf[x] && isdigit(g->buf[x]); ++x);
  h->point_start=x; /**point to start of data  */
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_oscope_instrument(struct hp_common_options *o, struct gpib *g)	 
{
	int slot,i;
  char buf[20];
	
	if(-1 == (slot=hp16500_find_card(o->cardtype, o->cardno,g)) ){
		fprintf(stderr,"Unable to find timebase card\n");
		return -1;
	}
	select_hp_card(slot, g);
  switch(o->fmt){
    case FMT_ASCII: sprintf(buf,"ASCII");  break;
    
    case FMT_WORD: sprintf(buf,"WORD");  break;
    case FMT_BYTE: 
    default:
     sprintf(buf,"BYTE");  break;
  }
	
  sprintf(g->buf,":WAV:REC FULL;:WAV:FORM %s;:SELECT?",buf);
	i=write_get_data(g,g->buf);
	fprintf(stderr,"Selected Card %s to talk to.\n",g->buf);

	return i;
/*34,-1,12,12,11,1,0,5,5,5 */
	/** :WAV:REC FULL;:WAV:FORM ASC;:SELECT? */
}
/**#######################################################################
                     logic analyzer functionality
   #######################################################################*/


/***************************************************************************/
/** Make sure the sample period is valid.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int validate_sampleperiod(uint32 p)
{
	if( p<8){
		if( 2 != p && 4 != p)
			return -1;
		return 0;
	}
	if(p%8)
		return -1;
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
long int get_datsize(char *hdr)
{
	long int len;
	char *s;
	
	if(NULL==(s=strstr(hdr,"#8")) ){
		fprintf(stderr,"unable to find data len\n");
		return 0;
	}
	s+=2;
	sscanf(s,"%ld",&len);
	return len;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint32 swap32(uint32 in)
{
	uint32 out;
	out=in<<24;
	out|=(in&0x0000FF00)<<8;
	out|=in>>24;
	out|=(in&0x00FF0000)>>8;
	/*fprintf(stderr,"in %d out %d\n",in,out); */
	return out;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint64 swap64(uint64 in)
{
	uint64 out;
	out=in<<56;
	out|=in>>56;
	
	out|=(in&0x000000000000FF00)<<40;
	out|=(in&0x00FF000000000000)>>40;
	out|=(in&0x0000000000FF0000)<<24;
	out|=(in&0x0000FF0000000000)>>24;
	out|=(in&0x00000000FF000000)<<8;
	out|=(in&0x000000FF00000000)>>8;
	/*fprintf(stderr,"in %016lx out %016lx\n",in,out);  */
	return out;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint16 swap16(uint16 in)
{
	uint16 out;
	out=in<<8;
	out|=in>>8;
	/*fprintf(stderr,"in %d out %d\n",in,out); */
	return out;
}
/***************************************************************************/
/** find location of section using name. Allocate and load the data into the 
structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct section *find_section(char *name, struct hp_block_hdr *blk )
{
	uint32 total;
  struct section *sec;
	if( NULL == (sec=malloc(sizeof(struct section))))	{
		fprintf(stderr,"Unable to alloc for section\n");
		return NULL;
	}
	for (total=0,sec->sz=1; total<blk->bs.blocklen && sec->sz>0;){
		memcpy(&sec->hdr,(char *)(blk->data+total),sizeof(struct section_hdr));
		sec->hdr.name[10]=0;
		sec->sz=swap32(sec->hdr.block_len);
		fprintf(stderr,"Looking at Name '%s' module_id %d len %d\n",sec->hdr.name, sec->hdr.module_id,sec->sz);	
		
		if(!strcmp(name,sec->hdr.name)){
			if( NULL == (sec->data=malloc(sec->sz)) )	{
				fprintf(stderr,"Unable to alloc for section\n");
				free(sec);
				return NULL;
			}		
			memcpy(sec->data,(char *)(blk->data+total+16),sec->sz);
			sec->off=total; /**save location in block header where section starts  */
			return sec;
		}
			
		total+=sec->sz+16; /**16 is size of section info  */
	}	
	return NULL;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_sections(struct hp_block_hdr *blk )
{
	uint32 total, sz;
	struct section_hdr hdr;
	for (total=0,sz=1; total<blk->bs.blocklen && sz>0;){
		memcpy(&hdr,(char *)(blk->data+total),sizeof(struct section_hdr));
		hdr.name[10]=0;
		sz=swap32(hdr.block_len);
		fprintf(stderr,"Name '%s' module_id %d len %d\n",hdr.name, hdr.module_id,sz);	
		total+=sz+16; /**16 is size of section info  */
	}
}

/***************************************************************************/
/** .
\n\b Arguments:
If a the file pointer is not null, output the data in this format:
Label  Ch Cl P4h l P3h l P2h l P1h l
POL    00 00 00 00 00 00 00 00 00 08

Where h= hi, l= lo, C=CLK and P=POD. In the above example, POL is bit 3, on
Pod 1
a[0] is clk, a[1] is pod 4
\n\b Returns:
****************************************************************************/
void config_show_label(char *a, struct labels *l, FILE *out)
{
	int m;
	if(NULL != out){
		fprintf(out,"%s ",l->name);
		for (m=0;m<10;++m){
			if(a[m/2])
				fprintf(out,"%02x ",l->map.clk_pods[m]);
		}
		fprintf(out,"\n");
	}else{
		fprintf(stderr,"%s map is clk ",l->name);
		for (m=0;m<10;++m){
			if(a[m/2]){
				if(m>1 && !(m%2))
					fprintf(stderr,"P%d ",6-((m+2)/2));
				fprintf(stderr,"%02x ",l->map.clk_pods[m]);	
			}
		}
			
		fprintf(stderr," pol %d #bits %d ena %d seq %d ",l->polarity,l->bits,l->enable, l->sequence);	
		/** for (m=0;m<3;++m)
			fprintf(stderr,"%02x ",l->unknown1[m]);
		fprintf(stderr,"*3* ");
		for (m=0;m<4;++m)
			fprintf(stderr,"%02x ",l->unknown3[m]);*/
		fprintf(stderr,"\n");
	}
		
}
/***************************************************************************/
/** FIXME: This does not handle having both analyzers on.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void config_show_labelmaps(struct section *sec, FILE *out)
{
	struct labels l;
	int i,k;
	char x[10], active[SIZEOF_ACTIVE_ARRAY];
	struct config_data d;
	memcpy(&d,sec->data,sizeof(struct config_data));
	
	for (i=0;i<2;++i){
		d.m[i].name[10]=0;
		fprintf(stderr,"Machine %d is '%s'\n",i,d.m[i].name); 
	}
		
	/**just for display purposes, set our bits to clock and 4 pods  */
	memset(active,0,SIZEOF_ACTIVE_ARRAY);
	active[4]=1; /**clk  */ 
	for (i=0;i<2;++i){
		int pod;
		pod=0;
		if(MACHINE_MODE_OFF == d.m[i].mode)
			fprintf(stderr,"Machine %d is off\n",i+1);
		else{
			int x;
			if(d.m[i].assign.pod_info0&(POD_INFO_A1|POD_INFO_A2))
				pod=1;
			if(d.m[i].assign.pod_info0&(POD_INFO_A3|POD_INFO_A4))
				pod|=2;
		  for (x=0;x<2;++x){
				if(pod&(1<<x)){
					active[x<<1]=1;
					if(MACHINE_MODE2_FULL == d.m[i].mode2) /**full channel, both pods are used  */
						active[(x<<1)+1]=1;
				}
			}	
		}
	}
	fprintf(stderr,"Pods Active ");	
	for (i=0;i<5;++i)
		fprintf(stderr,"%d ",active[i]);	
	fprintf(stderr,"\n");
	/**0x27A is 0x294-1A. 1A is start of machine name 1.  */	
	for (i=0,k=0x27A; i<0xFF; ++i){
		memcpy(&l,sec->data+k,LABEL_RECORD_LEN);
		l.actual_offset=swap16(l.strange_offsetlo);
		//fprintf(stderr,"Actual map offset=0x%x ",l.actual_offset); 
		l.actual_offset-=0x6E90;
		/*fprintf(stderr,"Adjusted = 0x%x ",l.actual_offset); */
		memcpy(&l.map,sec->data+0x27A+l.actual_offset,LABEL_MAP_LEN);
		config_show_label(active,&l, out);
		show_vcd_label(active,&l);	
		k+=LABEL_RECORD_LEN;
		snprintf(x,7,"Lab%d  ",i+1);
		if(!strcmp(l.name,x))
			break;
	}
	show_vcd_label(active,NULL);	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct hp_block_hdr *read_block(char *cfname)
{
	struct hp_block_hdr *blk=NULL;
	FILE *cfd=NULL;
	
	if(NULL == (cfd=fopen(cfname,"r"))) {
		fprintf(stderr,"Unable to open '%s' for writing\n",cfname);
		goto closem;
	}
	if(NULL == (blk=malloc(sizeof(struct hp_block_hdr))) ){
		fprintf(stderr,"Unable to malloc for hdr\n");
		goto closem;
	}
	/**read in header  */
	fread(blk,1,10,cfd);
	sscanf(blk->bs.blocklen_data,"%d",&blk->bs.blocklen);
	fprintf(stderr,"Blocksize is %d\n",blk->bs.blocklen);
	/**allocate room for data  */
	if(NULL == (blk->data=malloc(blk->bs.blocklen))){
		fprintf(stderr,"Unable to malloc %d bytes\n",blk->bs.blocklen);
		goto closem;
	}
	/**read data in  */
	fread(blk->data,1,blk->bs.blocklen,cfd);
closem:
	if(NULL != cfd)
		fclose(cfd);
	return blk;
}


/***************************************************************************/
/** .
\n\b Arguments:
cfname is config file input name, name is name of section to return, mode 
indicates if we want info or quiet.
\n\b Returns:
****************************************************************************/
struct section *parse_config( char *cfname, char *name, int mode)
{
	struct hp_block_hdr *blk;
	struct section *sec;
	
	if(NULL == (blk=read_block(cfname))) {
		fprintf(stderr,"Unable to read block from '%s'\n",cfname);
		return NULL;
	}
	
	if(NULL == (sec=find_section(name,blk))){
		fprintf(stderr,"Unable to find section '%s'\n",name);
		return NULL;
	}
	if(SHOW_PRINT == mode){
		show_sections(blk);
		/*fprintf(stderr,"First Machine is '%s', second is '%s'\n",sec->data,(char *)(sec->data+0x40)); */
		config_show_labelmaps(sec, NULL);
	}
	return sec;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void swap_analyzer_bytes(struct analyzer_data *a)
{
	a->data_mode=swap32(a->data_mode);
	a->pods=swap32(a->pods);
	a->masterchip=swap32(a->masterchip);
	a->maxmem=swap32(a->maxmem);
	a->sampleperiod=swap64(a->sampleperiod);
	a->tag_type=swap32(a->tag_type);
	a->trig_off=swap64(a->trig_off);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void swapbytes(struct data_preamble *p)
{
	int i;
	p->instid=swap32(p->instid);
	p->rev_code=swap32(p->rev_code);
	p->chips=swap32(p->chips);
	p->analyzer_id=swap32(p->analyzer_id);
	swap_analyzer_bytes(&p->a1);
  swap_analyzer_bytes(&p->a2);
	for (i=0; i<4; ++i){
		p->data_hi[i]=swap32(p->data_hi[i]);
		p->data_mid[i]=swap32(p->data_mid[i]);
		p->data_master[i]=swap32(p->data_master[i]);
		p->trig_hi[i]=swap32(p->trig_hi[i]);
		p->trig_mid[i]=swap32(p->trig_mid[i]);
		p->trig_master[i]=swap32(p->trig_master[i]);
	}
	
	p->rtc.year=swap16(p->rtc.year);
	/*p->rtc.month=swap16(p->rtc.month); */
	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_analyzer(struct analyzer_data *a)
{
	fprintf(stderr,"datam=%d pods=%08x master=%04x maxmem=%04x samp=%ldps tagtype=%04x trig_off=%ldps\n",
	a->data_mode, a->pods, a->masterchip, a->maxmem, a->sampleperiod,a->tag_type,a->trig_off);	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_pre(struct data_preamble *p)
{
	struct rtc_data *r;
	int i;
	fprintf(stderr,"ID=%d REV=%04x Chips=%04x AID=%04x\n",p->instid,p->rev_code, p->chips, p->analyzer_id);
	fprintf(stderr,"Analyzer 1\n");
	show_analyzer(&p->a1);
	fprintf(stderr,"Analyzer 2\n");
	show_analyzer(&p->a2);
	fprintf(stderr,"Valid Data Rows\n");
	for (i=0; i<4; ++i){
		fprintf(stderr,"%dh=%d ",(3-i)+1,p->data_hi[i]);
	}
	for (i=0; i<4; ++i){
		fprintf(stderr,"%dm=%d ",(3-i)+1,p->data_mid[i]);
	}
	for (i=0; i<4; ++i){
		fprintf(stderr,"%dM=%d ",(3-i)+1,p->data_master[i]);
	}
	fprintf(stderr,"\nTrigger Point\n");
	for (i=0; i<4; ++i){
		fprintf(stderr,"%dh=%d ",(3-i)+1,p->trig_hi[i]);
	}
	for (i=0; i<4; ++i){
		fprintf(stderr,"%dm=%d ",(3-i)+1,p->trig_mid[i]);
	}
	for (i=0; i<4; ++i){
		fprintf(stderr,"%dM=%d ",(3-i)+1,p->trig_master[i]);
	}
	r=&p->rtc;
	fprintf(stderr,"\nAcq time %d/%d/%d dow %d %d:%d:%d\n",
	r->year+1990,r->month,r->day,r->dow,r->hour,r->min,r->sec);
	/**our trace data should start right after r->sec.  */
	/*print_data( */
}


/***************************************************************************/
/** Find the start of the trace.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char *get_trace_start(struct data_preamble *l)
{
	char *s;
	struct data_preamble *lp;
	lp=(struct data_preamble *)l->data;
	
	s=((char *)&lp->rtc.sec - l->data)+l->data+1; 
	fprintf(stderr,"start %p last %p %ld (0x%lx) @%p\n",
	l->data,&lp->rtc.sec,(long int)(&lp->rtc.sec)-(long int)l->data,(long int)(&lp->rtc.sec)-(long int)l->data,s); 
	return s;
}
#define MODE_EDGES 1
#define MODE_LEVELS 0
/***************************************************************************/
/** Given a pod and a bit, find all states & times.
\n\b Arguments:
pod - 1-4
state  bit fields to look for that match this state
mask -  bits to look at, 1 means look for it
\n\b Returns:
****************************************************************************/
void search_state(int pod, uint16 clk, uint16 clkmask, uint16 state, uint16 mask, int mode,struct data_preamble *pre)
{
	struct one_card_data *d;
	char *s,*b;
	uint32 count;
	long int  ps;
	int inc,c,p;
	int cmatch, pmatch, lc, lp;
	ps=0;
	c=0;
	cmatch=pmatch=0;
	lc=lp=1;
	if(0 == clkmask)
		cmatch=-1;
	if(0==mask)
		pmatch=-1;
	if(-1 == cmatch && -1== pmatch){
		fprintf(stderr,"Must have either clock or pod mask set\n");
		return;
	}
	if(pod>=4 || pod<1){
		fprintf(stderr,"Valid pod numbers are 1-4, not %d\n",pod);
		return;
	}
	p=((pod-1)*2);
	p=6-p;
	p+=2; /**move the offset past clk  */
	b=s=get_trace_start(pre);
	inc = ONE_CARD_ROWSIZE;
	count=0;
	while(s<pre->data + pre->data_sz){
		uint16 val,_clk,x;
		
		d=(struct one_card_data *)s;
		val=d->pdata[p+1]|d->pdata[p]<<8;
		
		if(clkmask){
			_clk=d->pdata[1]|(d->pdata[0]<<8);
			x=_clk&clkmask;
			if(x==clk)
				cmatch=1;
			else cmatch=0;
		}
		
		if(mask){
			x=val&mask;
			if(x == state)
				pmatch=1;
			else 
				pmatch=0;
		}
		if(mode&MODE_LEVELS){
			if(1 == pmatch || 1 == cmatch) {
			
				fprintf(stderr,"c%04x pod %04x %ldns\n",_clk,val,ps/1000);
				++c;
				if(c>10 )
					break;	
			}
		}else if(mode&MODE_EDGES){
			x=0;
			if(pmatch != lp){
				x=1;
				lp=pmatch;
			}
			if(cmatch != lc){
				x=1;
				lc=cmatch;
			}
			if(x){
				fprintf(stderr,"%08d %08lx c%04x pod %04x %ldns\n",count,(s-b)+0x258,_clk,val,ps/1000);
				++c;
				if(c>10 )
					break;	
			}
		}
		
		ps+=pre->a1.sampleperiod;
		
		s+=inc;
		++count;
		if(count>=pre->data_master[3])
			break;
	}	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int number_of_pods_assigned(uint32 pods)
{
	int p;
	uint32 i;
	for (p=0,i=1;i;i<<=1)
		if(i&pods)
			++p;
	fprintf(stderr,"pods_assigned %d\n",p);
	return p;
}

/***************************************************************************/
/** .
\n\b Arguments:
pod_no is the pod number, 1-12
pods is the pod map for the analyzer (shows what is assigned with a 1)
rows is the array that tells how many valid rows this pod has.
Note that bit 2 = pod 1 in map, but rows[3]=pod 1.
\n\b Returns:
****************************************************************************/
uint32 valid_rows(int pod_no, uint32 pods, uint32 *rows)
{
	uint32 i;
	if(pod_no <1 || pod_no>12){
		fprintf(stderr,"valid_rows: invalid pod %d\n",pod_no);
		return 0;
	}

	i=1<<pod_no;
	if(i&pods){
		if(pod_no>4)	/**pods are split into 3 groups of 4, so hopefully they passed   */
			pod_no/=4;	/** the right row array in...  */
		return rows[4-pod_no];
	}
		
	return 0;
}


/***************************************************************************/
/** FIXME. We do not compensate for inverted polarity here.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int get_next_datarow(struct data_preamble *p, char *buf)
{
	static char *dstart;
	static int state=0;
	struct one_card_data *d;
	static unsigned long off;
	int i,k;
	if(NULL == p || NULL ==buf){
		state=0;
		return 0;
	}
		
	if(0 == state){
		dstart=get_trace_start(p);
		state=1;
		off=dstart-p->data;
	}
	if(dstart < p->data + p->data_sz){
		d=(struct one_card_data *)dstart;
		/**clk is 0...pod 4 is 1  */
		k=sprintf(buf,"%02x%02x",d->pdata[0], d->pdata[1]);
		for (i=1;i<POD_ARRAYSIZE/2;++i){
				if(valid_rows(POD_ARRAYSIZE/2-i,p->a1.pods,p->data_master))	{
					k+=sprintf(&buf[k],"%02x%02x",d->pdata[i*2],d->pdata[1+(i*2)]);
				}	
		}
		/** if( strncmp("000000000000",buf,12))
			printf("%s",buf);*/
		sprintf(&buf[k],"\n");
#if 0 /**for debug, prints raw data +offset out  */		
		off +=  ONE_CARD_ROWSIZE;
		printf("%06lx",off);
		for (i=0;i<12;++i)
			printf(" %02x",((unsigned char *)dstart)[i]);
		printf("\n");
#endif
		dstart+=ONE_CARD_ROWSIZE;
		return 1;
	}	
	return 0;
}
/***************************************************************************/
/** 
Look at the pods assigned to the analyzer, then look at the valid rows of 
that pod to know if the data is good.  Also, if the mode of the analyzer 
is 13, it is half-channel so you *know* the second pod data is not good 
reguardless of the what the data_xxx vars say. 
\n\b Arguments:
\n\b Returns:
****************************************************************************/
uint32 put_data_to_file(struct data_preamble *p, char *fname)
{
	long int ps;
	int c;
	char buf[500];
	FILE *out;
	uint32 count;
	count=0;
	
	ps=0;
	
	if(NULL == (out=fopen(fname,"w+"))) {
		fprintf(stderr,"Unable to open '%s' for writing\n",fname);
		goto closem;
	}
	fprintf(stderr,"Writing file '%s'\n",fname);
	/*if(13 == p->a1.data_mode) */
	
	
	fprintf(stderr,"Valid rows: ");
	for (c=1; c<5; ++c){
		fprintf(stderr,"%d=%d ",c,valid_rows(c,p->a1.pods,p->data_master));
	}
	fprintf(stderr,"\n");
	/**make sure we are reset  */
	get_next_datarow(NULL,NULL);
	while(get_next_datarow(p,buf) ){
		fprintf(out,"%s",buf);
		ps+=p->a1.sampleperiod;
		++count;
	}
#if 0
	dstart=get_trace_start(sec);
/*	dstart = (void *)((char *)(&lp->rtc.sec) - (char *)sec->data);  */
	c=0;
	inc = ONE_CARD_ROWSIZE;
	fields=number_of_pods_assigned(p->a1.pods);
	while (dstart < sec->data + sec->sz){
		d=(struct one_card_data *)dstart;
		fprintf(out,"%02x%02x",d->clkhi, d->clklo);
		if(fields>1){/**if 1, just clock??  */
			
			for (i=4;i>0;--i){
				int x;
				if(valid_rows(i,p->a1.pods,p->data_master))	{
					x=8-(i*2);
					fprintf(out,"%02x%02x",d->pdata[x],d->pdata[x+1]);
				}
			}
		}	
		fprintf(out,"\n");
		/**just print out clk, pod 1 & 2 data  */
/*		fprintf(out,"%02x%02x%02x%02x%02x%02x\n",d->clkhi, d->clklo,d->pdata[4],d->pdata[5],d->pdata[6],d->pdata[7]); */
		ps+=p->a1.sampleperiod;
		dstart+=inc;
		++count;
	}
#endif
closem:
	if(NULL != out)
		fclose(out);
	return count;
	
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void print_data(struct data_preamble *p)
{
	struct one_card_data *d;
	char *dstart;
	long int ps;
	int inc,c;
	uint32 count;
	count=0;
	c=0;
	ps=0;
	inc = ONE_CARD_ROWSIZE;
	fprintf(stderr,"Valid rows= %d\n",p->data_master[3]);
	dstart=get_trace_start(p);
/*	dstart = (void *)((char *)(&lp->rtc.sec) - (char *)sec->data);  */
	
	while (dstart < p->data + p->data_sz){
		int i;
		d=(struct one_card_data *)dstart;
		fprintf(stderr,"CH%02x CL%02x ",d->pdata[0], d->pdata[1]);
		for (i=2; i<10; ++i){
			char x,y;
			if(!(i%2))
				x='M';
			else 
				x='L';
			y='0'+(9-i)/2;
			fprintf(stderr,"%c%c%02x ",x,y,d->pdata[i]);
			
		}
		
		fprintf(stderr," %ldns\n",ps/1000);
		ps+=p->a1.sampleperiod;
		dstart+=inc;
		++c;
		if(c>20)
			break;
	}
	
}


/***************************************************************************/
/** .\n\b Arguments:
mode=JUST_LOAD, loads & swaps bytes in pre, then returns.
\n\b Returns:
****************************************************************************/
struct data_preamble *parse_data( char *cfname, char *out, int mode)
{
	struct hp_block_hdr *blk;
	struct section *sec;
	struct data_preamble *pre;
	
	if(NULL == (pre=malloc(sizeof(struct data_preamble)))){
		fprintf(stderr,"Unable to allocate for data_preamble struct\n");
		return NULL;
	}
	
	if(NULL == (blk=read_block(cfname))) {
		fprintf(stderr,"Unable to read '%s' for writing\n",cfname);
		goto err;
	}
	if(mode != JUST_LOAD)
		show_sections(blk);
	if(NULL == (sec=find_section("DATA      ",blk))){
		fprintf(stderr,"Unable to find section 'DATA      '\n");
		goto err;
	}
	
	memcpy(pre,sec->data,sizeof(struct data_preamble));
	swapbytes(pre);
	pre->data=sec->data;
	pre->data_sz=sec->sz;
	if(mode == JUST_LOAD)
		return pre;
	show_pre(pre);
	
	print_data(pre);
	search_state(1,0,0,0x800,0x800,MODE_EDGES,pre);
	if(NULL != out)
		put_data_to_file(pre,out);
	return pre;
err:
	free(pre);
	return NULL;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct signal_data *add_signal(struct signal_data *d,int pol,int ena, int bits, int lsb, int msb, char *name)
{
	struct signal_data *s, *x;
	if(NULL != d){
		s=d;
		while(s->next)
			s=s->next;
	}else s=NULL;	
		
	if(NULL ==(x=malloc(sizeof(struct signal_data )))){
		fprintf(stderr,"Out of Mem for signal_data on '%s'\n",name);
		return NULL;
	}
	memset(x,0,sizeof(struct signal_data ));
	if(NULL!=s){
		s->next=x;
	}
	x->name=strdup(name);
	x->lsb=lsb;
	x->msb=msb;
	x->bits=bits;
	x->ena=ena;
	x->pol=pol;
	/*fprintf(stderr,"%s %d %d %d %d\n",x->name, x->msb, x->lsb, x->bits, x->ena); */
	return x;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
/***************************************************************************/
/** pods[0] is pod4, hi byte..
active[0] is clk, pod 4 is [1]
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct signal_data *show_vcd_label(char *a, struct labels *l)
{
	int i,bits, bs, be,p,bytes, bsset,beset;
	uint8 pod[26];
	uint16 x;
	static struct signal_data *d=NULL;
	if(NULL ==a && NULL ==l) /**just return pointer to data struct  */
		return d;
	if(NULL ==l && NULL !=d){	/**print out our signal list  */
		struct signal_data *s=d;
		while(s){
			if(s->lsb==s->msb)
				fprintf(stderr,"%s %d ",s->name,s->lsb);
			else
				fprintf(stderr,"%s %d-%d ",s->name,s->msb, s->lsb);
			fprintf(stderr,"\n");
			s=s->next;
		}
	  return d;	
	}
	/*fprintf(stderr,"       "); */
	for (bits=i=0;i<SIZEOF_ACTIVE_ARRAY;++i){
		/*fprintf(stderr," %d ",a[i]);   */
		if(a[i])
			bits+=16;/**each pod is 16 lines, and we register the clock as 16 too  */
	}
	bytes=bits/8;
	/*fprintf(stderr,"bits=%d, bytes=%d\n",bits,bytes);	   */
	/**now load the data array  */
	p=bytes-1;
	for (i=0;i<POD_ARRAYSIZE;++i){
		int x;
		if(a[i/2]){
			if(p<0){
				fprintf(stderr,"Fatal Err.p -1");
				return NULL;
			}
			if(i>=POD_START)
				x=POD_ARRAYSIZE-i;
			else
				x=i;
			pod[p--]=l->map.clk_pods[i];	
			/*fprintf(stderr,"%02x ",l->map.clk_pods[i]);   */
		}/*	else
			fprintf(stderr,"!%02x ",l->map.clk_pods[i]);   */
	}
	/*fprintf(stderr,"p=%d\n",p);   */
	/**now pod has clk top two indexes and lowest pod in 0,1, lsb in 0  */	
	/**we start out off-by one on bit position (p)  */
	bs=be=0;
	beset=bsset=0;
	for (p=i=0;i<bytes;++i){
		fprintf(stderr,"P0x%02x:",pod[i]);
		for (x=1;x<0x100;x<<=1,++p){
			
			if(x&pod[i]){
				/*fprintf(stderr,"p%d s%d e%d ",p,bs,be);  */
				if(bs && be ){
					fprintf(stderr,"Can't handle discontinous bits at bit %d for label %s\n",p,l->name);
					return NULL;
				}
				if(!bsset){
					/*fprintf(stderr,"U%d ",p);  */
					bs=p;	/**this is not off by one.  */
					bsset=1;
				}
					
			}else if(bsset && !beset) {
        beset=1;
				be=p;
				if(p)
					--be;
				/*fprintf(stderr,"D%d ",p);  */
			}
		}
	}
  printf("%s:%d-%d\n",l->name,bs,be);
	if(NULL ==d)
		d=add_signal(NULL, l->polarity,l->enable, bits, bs,be,l->name);
	else
		add_signal(d, l->polarity,l->enable, bits, bs,be,l->name);

	return d;
}

/***************************************************************************/
/** .
\n\b Arguments:
active has pod 4 at [0], pod 1 at [0], and clk at [4].
\n\b Returns:
****************************************************************************/
void calculate_active_bits(struct data_preamble *pre, char *active)
{
	int fields,i;
	fields=number_of_pods_assigned(pre->a1.pods);
	memset(active,0,SIZEOF_ACTIVE_ARRAY);
	active[0]=1;/**clock  */
	/**set this array so that pod 1 is high bit, so we match data struct in label  */
	if(fields>1){/**if 1, just clock??  */
		for (i=1;i<5;++i){
			if(valid_rows(i,pre->a1.pods,pre->data_master))	{
				active[5-i]=1;
			}
		}	
	}	
}
/***************************************************************************/
/** .
\n\b Arguments:
pre is the preable from the data. sec is the config section from the config.
\n\b Returns:
****************************************************************************/
struct signal_data *show_la2vcd(struct data_preamble *pre, struct section *sec, int mode)
{
	int i,k;
	char x[10];
	struct labels l;
	char active[SIZEOF_ACTIVE_ARRAY]; /**max 12 pods +clock can be assigned to one machine.  */
										/**but only 4 pod logic is implemented here...  */
	
	calculate_active_bits(pre,active);
			
	/**0x27A is 0x294-1A. 1A is start of machine name 1.  */
	for (i=0,k=0x27A; i<0xFF; ++i){
		memcpy(&l,sec->data+k,LABEL_RECORD_LEN);
		l.actual_offset=swap16(l.strange_offsetlo);
		//fprintf(stderr,"Actual map offset=0x%x ",l.actual_offset); 
		l.actual_offset-=0x6E90;
		/*fprintf(stderr,"Adjusted = 0x%x ",l.actual_offset); */
		memcpy(&l.map,sec->data+0x27A+l.actual_offset,LABEL_MAP_LEN);
			
		k+=LABEL_RECORD_LEN;
		snprintf(x,7,"Lab%d  ",i+1);
		if(!strcmp(l.name,x))
			break;
		show_vcd_label(active,&l);
	}
	if(SHOW_PRINT == mode){
		show_vcd_label(active,NULL);
		fprintf(stderr,"\n");	
	}
	
	return show_vcd_label(NULL,NULL);
}





