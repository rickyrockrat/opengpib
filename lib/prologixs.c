/** \file ******************************************************************
\n\b File:        prologix.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        04/06/2009 10:53 am
\n\b Description: Prologix interface routines
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

#include "open-gpib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct prologixs_ctl {
	int addr; /**internal address, used to set address when != gpib.addr  */
	int autor; /**auto read setting  */
	struct open_gpib_dev *serial; /**serial interface control  */
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int verify( struct open_gpib_dev *g, char *msg)
{
	sprintf(g->buf,"%s\r",msg);
	write_string(g,g->buf);
	read_string(g);
	printf("'%s' = '%s'\n",msg,g->buf);	 
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int init_prologixs( struct open_gpib_dev *g)
{
	int i;
	struct prologixs_ctl *c;
	c=(struct prologixs_ctl *)g->internal;
	if(g->debug)printf("Init Prologix controller, dbg=%d\n",g->debug);
	write_string(g,"++clr");
	read_string(g);
	/*make sure auto reply is on*/
	if(c->autor)
		write_string(g,"++auto 1");
	else
		write_string(g,"++auto 0");
	write_string(g,"++ver");
	/*write_string(p,"++read"); */
	
	if(-1 == (i=read_string(g)) ){
		printf("%s:Unable to read from port on ver\n",__func__);
		goto err;
	}
	/*printf("Got %d bytes\n%s\n",i,g->buf);  */
	if(NULL == strstr(g->buf,"Prologix")){
		printf("%s:Unable to find correct ver in\n%s\n",__func__,g->buf);
		goto err;
	}
	if(g->debug) printf("Talking to Controller '%s'\n",g->buf);
	if(g->debug >= DBG_TRACE)	fprintf(stderr,"write mode\n");
	/*Then set to Controller mode */
	write_string(g,"++mode 1");
	if(g->debug >= DBG_TRACE)	fprintf(stderr,"write addr\n");
	/*Set the address to talk to */
	sprintf(g->buf,"++addr %d",c->addr);
	write_string(g,g->buf);
	/*Set the eoi mode (EOI after cmd) */
	write_string(g,"++eoi 1");
	/*Set the eos mode (LF) */
	write_string(g,"++eos 2");
	if(g->debug){
		verify(g,"++addr");
		verify(g,"++eoi");
		verify(g,"++eos");
		verify(g,"++auto");	
	}
	if(g->debug >= DBG_TRACE)	fprintf(stderr,"%s: return\n",__func__);
	return 0;
err:
	fprintf(stderr,"Controller init failed\n");
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int control_prologixs( struct open_gpib_dev *ctl, int cmd, uint32_t data)
{
	int i;
	struct prologixs_ctl *c; 
	char cmdbuf[100];
	if(NULL == ctl){
		fprintf(stderr,"prologixs: gpib null\n");
		return 0;
	}
	c=(struct prologixs_ctl *)ctl->internal;
	if( -1== check_calloc(sizeof(struct prologixs_ctl), &c, __func__,&ctl->internal) ) return -1;
		
	switch(cmd){
		case CTL_CLOSE:
			if(ctl->debug) fprintf(stderr,"Closing gpib\n");
			
			break;
		case CTL_SET_TIMEOUT:
			return ctl->dev->funcs.og_control(ctl->dev,SERIAL_CMD_SET_CHAR_TIMEOUT,data);
			break;
		case CTL_SET_ADDR: /**send command, check result, then set gpib addr.  */
			i=sprintf(cmdbuf,"++addr %d\n",data);
			if(ctl->dev->funcs.og_write(ctl->dev,cmdbuf,i) == i){
				c->addr=data;
				return 1;
			}
			break;
		case CTL_SEND_CLR:
			i=sprintf(cmdbuf,"++clr\n");
			if(ctl->dev->funcs.og_write(ctl->dev,cmdbuf,i) == i){
				fprintf(stderr,"Sent Device Clear\n");
				return 1;
			}
			break;
		case CTL_SET_DEBUG:
			ctl->debug=data;
			fprintf(stderr,"%s: Setting debug to %d\n",__func__,ctl->debug);
			
			break;
	}
	return 0;
}
	


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
static int write_prologixs(struct open_gpib_dev *ctl, void *buf, int len)
{
	int rtn;
	char *m=(char *)buf;
	
	if('\r' != m[len-1]){ /**Make sure we have terminator...  */
		if(NULL == (m=malloc(len+2)) ){
			printf("out of mem for %d\n",len+2);
			return -1;
		}
		/*printf("Alloc new msg for '%s'\n",ctl->buf); */
		memcpy(m,buf,len);
		m[len++]='\r';
		m[len]=0;
	}
	/*printf("Swrite.."); */
	if((rtn=ctl->dev->funcs.og_write(ctl->dev,m,len)) <0) {/* error writing */
		rtn=-1;
		goto end;
  }
	/*printf("wrote %d bytes\n%s\n",rtn,ctl->buf); */
	if(rtn != len)
		printf("Write mis-match %d != %d\n",rtn,len);
	usleep(500);
end:
	if(m!=buf)
		free(m);
	return rtn;	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number of byte read otherwise
****************************************************************************/
static int read_prologixs(struct open_gpib_dev *ctl, void *buf, int len)
{
	struct prologixs_ctl *c;
	int i;
	char *m;
	if(NULL == ctl || NULL == buf){
		fprintf(stderr,"%s: Null buffer or dev\n",__func__);
		return -1;
	}
	m=(char *)buf;
	c=(struct prologixs_ctl *)ctl->internal;
	if(0 == c->autor){
		char cmd[20];
		i=sprintf(cmd,"++read\r");
		write_prologixs(ctl,cmd,i);
	}
	i=ctl->dev->funcs.og_read(ctl->dev,buf,len);
	
	if(i){
		--i;
		while(i>=0 && (m[i]=='\r' || m[i]=='\n'))
			--i;
		++i;	
	}
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int close_prologixs( struct open_gpib_dev *ctl)
{
	int i=0;
	if(NULL == ctl || NULL == ctl->dev)
		return 0;
	if(NULL != ctl->dev->funcs.og_close ){
		if((i=ctl->dev->funcs.og_close(ctl->dev)) ){
			printf("Error closing interface\n");
		}	
	}
	
	free(ctl->internal);
	if(NULL != ctl->dev)
		free (ctl->dev);
	ctl->dev = ctl->internal=NULL;
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, 0 on success
****************************************************************************/
static int open_prologixs(struct open_gpib_dev *ctl, char *path)
{
	struct prologixs_ctl *c;
	open_gpib_register reg_func;
	if(NULL == ctl){
		printf("%s: dev null\n",__func__);
		return 1;
	}
	c=(struct prologixs_ctl *)ctl->internal;
	if(-1 == check_calloc(sizeof(struct prologixs_ctl), &c, __func__,&ctl->internal) ) 
		return -1;
		
	if(NULL == (reg_func=open_gpib_find_interface("serial", OPEN_GPIB_REG_TYPE_TRANSPORT))){
		goto err;
	}	
	if(NULL == (ctl->dev=reg_func())) /**load our function list  */
		goto err;
	/**pass our debug option down  */
	ctl->dev->funcs.og_control(ctl->dev,CMD_SET_DEBUG,ctl->debug);
	if(-1==ctl->dev->funcs.og_open(ctl->dev,path))
		goto err;
	
	ctl->dev->funcs.og_control(ctl->dev,SERIAL_CMD_SET_CHAR_TIMEOUT,50000);
	c->autor=1;
	return 0;
err:
	free(ctl->dev);
	ctl->dev=NULL;
	return -1;
}

/***************************************************************************/
/** Allocate our internal data structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void *calloc_internal_prologixs(void)
{
	void *p=NULL;
	if(-1 == check_calloc(sizeof(struct prologixs_ctl), &p,__func__,NULL) ) 
		return NULL;
	return p;
}

GPIB_CONTROLLER_FUNCTION(prologixs,"Prologix Serial GPIB Controller")

