/** \file ******************************************************************
\n\b File:        hp1653x.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/15/2013 11:52 am
\n\b Description: Routines specific to hp1653x oscilliscope cards.
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
#include "hp16500.h"
#include "hp1653x.h"
/**#######################################################################
                     oscilliscope functionality
   #######################################################################*/

/***************************************************************************/
/** ":CHAN1:OFFS 0;:CHAN2:OFFS 0;:CHAN3:OFFS 0;:CHAN4:OFFS 0".
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_channel_offset(struct open_gpib_mstr *g, int channel, float offset)
{
	/**kill -Wextra warnings  */
	if(channel || offset || NULL ==g)
		return -1;
	return -1;
}


/***************************************************************************/
/** ":CHAN1:RANG 1.6;:CHAN2:RANG 1.6;:CHAN3:RANG 1.6;:CHAN4:RANG 1.6" .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_channel_range(struct open_gpib_mstr *g)
{
	if(NULL == g) /**kill -Wextra warning.  */
  	return -1;
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
int get_trigger_source (struct open_gpib_mstr *g)
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
double og_get_xinc_mult(double v, int *x)
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
int oscope_parse_preamble(struct open_gpib_mstr *g, struct hp_scope_preamble *h)
{
	h->fmt=og_get_value_col(0,g->buf);
	h->type=og_get_value_col(1,g->buf);
	h->points=og_get_value_col(2,g->buf);
  h->count=og_get_value_col(3,g->buf);
	h->xinc=og_get_value_col(4,g->buf);
	h->xorg=og_get_value_col(5,g->buf); /**first data point (in seconds) w/respect to trigger, i.e.  */
                                   /**trigger point is this value * -1, and data point number=
                                       xorg/xinc. */
	h->xref=og_get_value_col(6,g->buf);
	h->yinc=og_get_value_col(7,g->buf);
	h->yorg=og_get_value_col(8,g->buf);
	h->yref=og_get_value_col(9,g->buf);
  h->xincmult=og_get_xinc_mult(h->xinc,&h->xinc_thou);
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
int oscope_get_preamble(struct open_gpib_mstr *g,char *ch, struct hp_scope_preamble *h)
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
int get_oscope_data(struct open_gpib_mstr *g, char *ch, struct hp_scope_preamble *h)
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
int init_oscope_instrument(struct hp_common_options *o, struct open_gpib_mstr *g)	 
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

