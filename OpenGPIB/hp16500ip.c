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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gpib.h"
#include "hp16500ip.h"


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int hp16500c_init(struct gpib *g)
{
	struct hp16500c_ctl *c;
	c=(struct hp16500c_ctl *)g->ctl;
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
\n\b Returns: 1 on failure, 0 on success
****************************************************************************/
int _hp16500c_open(struct gpib *g, char *ip)
{
	struct hp16500c_ctl *c;
	if(NULL == g){
		fprintf(stderr,"%s: dev null\n",__func__);
		return 1;
	}
	
	if(NULL ==(c=malloc(sizeof(struct hp16500c_ctl))) ){
		fprintf(stderr,"Out of mem on hp16500c ctl alloc\n");
		return 1;
	}
	if(OPTION_DEBUG&g->type_ctl) 
		c->debug=1;
	else
		c->debug=0;
	c->ipaddr=strdup(ip);
	c->port=5025;
	c->cmd_wait=5000;/**uS  */
	c->socket.sin_family = AF_INET;
  c->socket.sin_addr.s_addr = inet_addr ( c->ipaddr );
  c->socket.sin_port = htons ( c->port );
	  /* Create an endpoint for communication */
  if(-1 ==(c->sockfd = socket( AF_INET, SOCK_STREAM, 0 )) ){
		fprintf(stderr,"Unable to create socket for '%s', port %d\n",c->ipaddr,c->port);
		return 1;
	}
	fprintf(stderr,"Socket created for '%s'\n",c->ipaddr);
  /* Initiate a connection on the created socket */
  if( 0 !=connect( c->sockfd, (struct sockaddr *)&c->socket, sizeof (struct sockaddr_in ) )){
		fprintf(stderr,"Unable to connect to '%s', port %d\n",c->ipaddr,c->port);
		return 1;
	}
	fprintf(stderr,"Connected to '%s'\n",c->ipaddr);
	g->ctl=c;
	if(-1 == hp16500c_init(g)){
		fprintf(stderr,"Controller init failed\n");
		free (g->ctl);
		g->ctl=NULL;
		/*fprintf(stderr,"_po rtn\n"); */
		return 1;
	}
	
	return 0;
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
	for (i=0;i<len;){
		int r;
		r= send ( c->sockfd, &m[i], len-i, 0 );
		if(c->debug)
			fprintf(stderr,"Sent %d bytes\n",r);
		if(-1 == r){
			fprintf(stderr,"Err Sending at byte %d\n",i);
			i=-1;
			goto end;
		}
		i+=r;
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
	int i,wait;
	char *m;
	m=(char *)buf;
	c=(struct hp16500c_ctl *)d;
	/*fprintf(stderr,"Looking for %d bytes\n",len); */
	for (i=wait=0; i<len;){
		int r;
		if(-1 == (r=recv ( c->sockfd, &m[i], len-i,MSG_DONTWAIT )) ){
			/*fprintf(stderr,"-1"); */
			++wait;
			if(wait > 5){
				/*fprintf(stderr,"Err sending\n"); */
				break;
			}
			usleep(c->cmd_wait);
		}else{
			i+=r;
			wait=0;
		}
		m[i]=0;	
		/** if(0 && -1 != r)
			fprintf(stderr,"Got %d bytes '%s'\n",r,&m[i-r]);*/
	}
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
int control_hp16500c(struct gpib *g, int cmd, int data)
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
		fprintf(stderr,"hp16500c_clt null\n");
		return 1;
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
	if(0!= (i=shutdown(c->sockfd,SHUT_RDWR)) ){
		fprintf(stderr,"Error closing interface\n");
	}
	close(c->sockfd);
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
