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
#include "prologixs.h"
#include "serial.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct prologixs_ctl {
	int addr; /**internal address, used to set address when != gpib.addr  */
	int autor; /**auto read setting  */
	struct serial_dev serial; /**serial interface control  */
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int verify(struct gpib *g, char *msg)
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
int prologixs_set_addr()
{
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int prologixs_init(struct gpib *g)
{
	int i;
	struct prologixs_ctl *c;
	c=(struct prologixs_ctl *)g->ctl;
	if(g->type_ctl&OPTION_DEBUG)printf("Init Prologix controller\n");
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
		return -1;
	}
	/*printf("Got %d bytes\n%s\n",i,g->buf);  */
	if(NULL == strstr(g->buf,"Prologix")){
		printf("%s:Unable to find correct ver in\n%s\n",__func__,g->buf);
		return -1;
	}
	if(g->type_ctl&OPTION_DEBUG) printf("Talking to Controller '%s'\n",g->buf);
	/*Then set to Controller mode */
	write_string(g,"++mode 1");
	
	/*Set the address to talk to */
	sprintf(g->buf,"++addr %d",g->addr);
	c->addr=g->addr;
	write_string(g,g->buf);
	/*Set the eoi mode (EOI after cmd) */
	write_string(g,"++eoi 1");
	/*Set the eos mode (LF) */
	write_string(g,"++eos 2");
	if(g->type_ctl&OPTION_DEBUG){
		verify(g,"++addr");
		verify(g,"++eoi");
		verify(g,"++eos");
		verify(g,"++auto");	
	}
	
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int control_prologixs(struct gpib *g, int cmd, uint32_t data)
{
	int i;
	struct prologixs_ctl *c; 
	char cmdbuf[100];
	if(NULL == g){
		printf("prologixs: gpib null\n");
		return 0;
	}
	c=(struct prologixs_ctl *)g->ctl;
	if( check_calloc(sizeof(struct prologixs_ctl), &c, __func__,&g->ctl) == -1) return -1;
		
	switch(cmd){
		case CTL_CLOSE:
			if(c->serial.debug) printf("Closing gpib\n");
			
			break;
		case CTL_SET_TIMEOUT:
			return c->serial.control(&c->serial,SERIAL_CMD_SET_CHAR_TIMEOUT,data);
			break;
		case CTL_SET_ADDR: /**send command, check result, then set gpib addr.  */
			i=sprintf(cmdbuf,"++addr %d\n",data);
			if(c->serial.write(&c->serial,cmdbuf,i) == i){
				g->addr=data;
				return 1;
			}
			break;
		case CTL_SEND_CLR:
			i=sprintf(cmdbuf,"++clr\n");
			if(c->serial.write(&c->serial,cmdbuf,i) == i){
				printf("Sent Device Clear\n");
				return 1;
			}
			break;
		case CTL_SET_DEBUG:
			if(data)
				c->serial.debug=1;
			else
				c->serial.debug=0;
			break;
	}
	return 0;
}
	


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
int _prologixs_write(void *d, void *buf, int len)
{
	struct prologixs_ctl *c;
	int rtn;
	char *m;
	
	c=(struct prologixs_ctl *)d;
	m=(char *)buf;
	
	if('\r' != m[len-1]){ /**Make sure we have terminator...  */
		if(NULL == (m=malloc(len+2)) ){
			printf("out of mem for %d\n",len+2);
			return -1;
		}
		/*printf("Alloc new msg for '%s'\n",g->buf); */
		memcpy(m,buf,len);
		m[len++]='\r';
		m[len]=0;
	}
	/*printf("Swrite.."); */
	if((rtn=c->serial.write(&c->serial,m,len)) <0) {/* error writing */
		rtn=-1;
		goto end;
  }
	/*printf("wrote %d bytes\n%s\n",rtn,g->buf); */
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
int _prologixs_read(void *d, void *buf, int len)
{
	struct prologixs_ctl *c;
	int i;
	char *m;
	m=(char *)buf;
	c=(struct prologixs_ctl *)d;
	if(0 == c->autor){
		char cmd[20];
		i=sprintf(cmd,"++read\r");
		_prologixs_write(d,cmd,i);
	}
	i=c->serial.read(&c->serial,buf,len);
	
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
int _prologixs_close(struct gpib *g)
{
	int i;
	struct prologixs_ctl *c;
	if(NULL == g->ctl)
		return 0;
	c=(struct prologixs_ctl *)g->ctl;
	if((i=c->serial.close(&c->serial)) ){
		printf("Error closing interface\n");
	}
	free (g->ctl);
	g->ctl=NULL;
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, 0 on success
****************************************************************************/
int _prologixs_open(struct gpib *g, char *path)
{
	struct prologixs_ctl *c;
	if(NULL == g){
		printf("%s: dev null\n",__func__);
		return 1;
	}
	c=(struct prologixs_ctl *)g->ctl;
	if( check_calloc(sizeof(struct prologixs_ctl), &c, __func__,NULL) == -1) return -1;
	
	if(serial_register(&c->serial)) /**load our function list  */
		goto err;
	
	if(OPTION_DEBUG&g->type_ctl) 
		c->serial.debug=1;
	else
		c->serial.debug=0;
	if(-1==c->serial.open(&c->serial,path))
		goto err;
	
	c->serial.control(&c->serial,SERIAL_CMD_SET_CHAR_TIMEOUT,50000);
	c->autor=1;
	g->ctl=c;
	if(-1 == prologixs_init(g)){
		printf("Controller init failed\n");
		c->serial.close(&c->serial);
		goto err;
	}
	
	return 0;
err:
	free(c);
	g->ctl=NULL;
	return -1;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int register_prologixs(struct gpib *g)
{
	g->control=control_prologixs;
	g->read=	_prologixs_read;
	g->write=	_prologixs_write;
	g->open=	_prologixs_open;
	g->close=	_prologixs_close;
	return 0;
}

