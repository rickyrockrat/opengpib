/** \file ******************************************************************
\n\b File:        hp16500ip.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/19/2010 11:58 am
\n\b Description: Implementation of the IP control interface on the hp 16500C
Logic analyzer.
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
#include "gpib.h"
#include "open-gpib-ip.h"
#include "hp16500ip.h"

struct hp16500c_ctl {
	int addr; /**internal address, used to set address when != gpib.addr  */
	int debug;
	int cmd_wait;
	struct ip_dev ip; /**ip interface control  */
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int hp16500c_init(struct gpib *g)
{
	
	if(g->type_ctl&OPTION_DEBUG)fprintf(stderr,"Init HP 16500C INET controller\n");
	if(0 == write_get_data(g,"*IDN?"))
		return -1;
	/*fprintf(stderr,"Got %d bytes\n",i); */
	if(NULL == strstr(g->buf,"HEWLETT PACKARD,16500C")){
		fprintf(stderr,"Unable to find 'HEWLETT PACKARD,16500C' in id string '%s'\n",g->buf);
		return -1;
	}	
	fprintf(stderr,"Init done\n");
	if(g->type_ctl&OPTION_DEBUG) fprintf(stderr,"Talking to Controller '%s'\n",g->buf);
	
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, 0 on success
****************************************************************************/
int _hp16500c_open(struct gpib *g, char *ip)
{
	struct hp16500c_ctl *c;
	if(NULL == g){
		printf("%s: dev null\n",__func__);
		return 1;
	}
	c=(struct hp16500c_ctl *)g->ctl;
	if(NULL == c){
		if(NULL ==(c=calloc(1,sizeof(struct hp16500c_ctl))) ){
			printf("Out of mem on hp16500c ctl alloc\n");
			return 1;
		}
	}	
	if(ip_register(&c->ip)) /**load our ip function list  */
		goto err;
	if(OPTION_DEBUG&g->type_ctl) 
		c->ip.debug=1;
	else
		c->ip.debug=0;
	/**set the port - IMPORTANT! Be sure to do this before opening. */
	c->ip.control(&c->ip,IP_CMD_SET_PORT,5025);
	/**set net timeout - Set this too, or you will timeout when opening */
	c->ip.control(&c->ip,IP_CMD_SET_CMD_TIMEOUT,50000); /**uS  */
	c->ip.control(&c->ip,IP_CMD_SET_DEBUG,c->debug); /**pass down debug option to interface level  */
	
	/**open ip address on port set above  */
	if(-1 == c->ip.open(&c->ip,ip))
		goto err;
	
	g->ctl=c;
	if(-1 == hp16500c_init(g)){
		fprintf(stderr,"Controller init failed\n");
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
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
int _hp16500c_write(void *d, void *buf, int len)
{
	struct hp16500c_ctl *c;
	int i;
	char *m;
	
	c=(struct hp16500c_ctl *)d;
	if(NULL == c){
		fprintf(stderr,"%s ctl struct NULL\n",__func__);
		return -1;
	}
	m=(char *)buf;
	
	if('\n' != m[len-1]){ /**Make sure we have terminator...  */
		if(NULL == (m=malloc(len+2)) ){
			fprintf(stderr,"out of mem for %d\n",len+2);
			return -1;
		}
		/*fprintf(stderr,"Alloc new msg for '%s'\n",g->buf); */
		memcpy(m,buf,len);
		m[len++]='\n';
		m[len]=0;
	}
	if(c->debug)
		fprintf(stderr,"Sending '%s'\n",m);
	if((i=c->ip.write(&c->ip,m,len)) <0) {/* error writing */
		i=-1;
		goto end;
  }
	/*fprintf(stderr,"wrote %d bytes\n%s\n",rtn,g->buf); */
	if(i != len)
		fprintf(stderr,"Write mis-match %d != %d\n",i,len);
	usleep(500);
end:
	if(m!=buf)
		free(m);
	return i;	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number of byte read otherwise
****************************************************************************/
int _hp16500c_read(void *d, void *buf, int len)
{
	struct hp16500c_ctl *c;
	int i;
	char *m;
	m=(char *)buf;
	c=(struct hp16500c_ctl *)d;
	/*fprintf(stderr,"Looking for %d bytes\n",len); */
	i=c->ip.read(&c->ip,buf,len);
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
int control_hp16500c(struct gpib *g, int cmd, uint32_t data)
{
	struct hp16500c_ctl *c; 
	int i;
	char buf[100];

	if(NULL == g){
		fprintf(stderr,"hp16500c: gpib null\n");
		return 0;
	}
	c=(struct hp16500c_ctl *)g->ctl;
	if(NULL == c){
		if(NULL ==(c=calloc(1,sizeof(struct hp16500c_ctl))) ){
			printf("Out of mem on hp16500c ctl alloc\n");
			return 1;
		}
		g->ctl=(void *)c;
	}
	switch(cmd){
		case CTL_CLOSE:
			if(c->debug) fprintf(stderr,"Closing hp INET\n");
			
			break;
		case CTL_SET_TIMEOUT:
			if(data >5000)
				c->cmd_wait=data;
			else
				c->cmd_wait=5000;
			
			break;
		case CTL_SET_ADDR: /**send command, check result, then set gpib addr.  */
			
			/*i=sprintf(cmdbuf,"++addr %d\n",data); */
			/*if(c->serial.write(&c->serial,cmdbuf,i) == i){ */
			/*	g->addr=data; */
			/*	return 1; */
			/*} */
			break;
		case CTL_SET_DEBUG:
			if(data)
				c->debug=1;
			else
				c->debug=0;
			break;
		case CTL_SEND_CLR:
			i=sprintf(buf,"*CLS\n");
			_hp16500c_write(c,buf,i);
			break;
		default:
			fprintf(stderr,"Unsupported cmd '%d'\n",cmd);
			break;
	}
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int _hp16500c_close(struct gpib *g)
{
	int i;
	struct hp16500c_ctl *c;
	if(NULL == g->ctl)
		return 0;
	c=(struct hp16500c_ctl *)g->ctl;
	if(0!= (i=c->ip.close(&c->ip)) ){
		fprintf(stderr,"Error closing interface\n");
	}
	free (g->ctl);
	g->ctl=NULL;
	return i;
}




/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int register_hp16500c(struct gpib *g)
{
	g->control=control_hp16500c;
	g->read=	_hp16500c_read;
	g->write=	_hp16500c_write;
	g->open=	_hp16500c_open;
	g->close=	_hp16500c_close;
	return 0;
}

