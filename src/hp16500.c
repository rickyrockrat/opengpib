/** \file ******************************************************************
\n\b File:        hp16500.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/17/2010 10:44 am
\n\b Description: Helper routines created to support the hp16500.
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
#include "hp16500.h"
#include <open-gpib.h>

struct hp_cards{
	int type;
	char *model;
	char *desc;
};

struct card_info {
	struct hp_cards *info;
	int slot;
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
int select_hp_card(int slot, struct open_gpib *g)
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
int hp16500_find_card(int cardtype, int no, struct open_gpib *g)
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
  		slots[i]=(int)og_get_value_col(i,&g->buf[s]);
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
  char *emsg="";
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
	struct open_gpib *g;
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
#ifdef HAVE_LIBLA2VCD2 
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
				fprintf(stderr,"-v not supported. Re-build with HAVE_LIBLA2VCD2=/path/to/lib\n");
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



