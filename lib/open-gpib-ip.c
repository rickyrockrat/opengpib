/** \file ******************************************************************
\n\b File:        ip.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008  4:21 pm
\n\b Description: Implementation of an interface over a IP socket.
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
#include <sys/stat.h>
#include <fcntl.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct ip_ctl {
	char *ipaddr;
	int port;
	int debug;
	struct sockaddr_in socket;
	int   sockfd;
	int cmd_timeout;
};


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, 0 on success
****************************************************************************/
static int open_if(struct open_gpib_dev *d, char *ip)
{
	struct ip_ctl *c;
	int i;
	if(NULL == d){
		fprintf(stderr,"%s: dev null\n",__func__);
		return 1;
	}
	c=(struct ip_ctl *)d->dev;
 	if( (i=check_calloc(sizeof(struct ip_ctl), &c, __func__,NULL)) == -1) return -1;
	/**We shouldn't ever come here, since we won't have a port or timeout to set...  
	   Instead, use control to set port and delay.
	*/
  if(1 == i)
		fprintf(stderr,"%s Port and delay not set! Opening anyway, but this should fail.\n",__func__);
	c->ipaddr=strdup(ip);
	c->socket.sin_family = AF_INET;
  c->socket.sin_addr.s_addr = inet_addr ( c->ipaddr );
  c->socket.sin_port = htons ( c->port );
	  /* Create an endpoint for communication */
  if(-1 ==(c->sockfd = socket( AF_INET, SOCK_STREAM, 0 )) ){
		fprintf(stderr,"Unable to create socket for '%s', port %d\n",c->ipaddr,c->port);
		goto err;
	}
	fprintf(stderr,"Socket %d created for '%s'\n",c->sockfd, c->ipaddr);
  /* Initiate a connection on the created socket */
  if( 0 !=connect( c->sockfd, (struct sockaddr *)&c->socket, sizeof (struct sockaddr_in ) )){
		fprintf(stderr,"Unable to connect to '%s', port %d\n",c->ipaddr,c->port);
		goto err;
	}
	fprintf(stderr,"Connected to '%s'\n",c->ipaddr);
	d->dev=(void *)c;
	
	return 0;
err:
	free (c);
	d->dev=NULL;
	return -1;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: number of bytes read 
****************************************************************************/
static int read_if(struct open_gpib_dev *d, void *buf, int len)
{
	struct ip_ctl *c; 				 
	int i,wait;
	char *m;
	m=(char *)buf;
	c=(struct ip_ctl *)d->dev;
	fprintf(stderr,"%s read\n",d->if_name);
	/*fprintf(stderr,"Looking for %d bytes\n",len); */
	for (i=wait=0; i<len;){
		int r;
		if(-1 == (r=recv ( c->sockfd, &m[i], len-i,MSG_DONTWAIT )) ){
			/*fprintf(stderr,"-1"); */
			++wait;
			if(wait > 10){
				/*fprintf(stderr,"Err sending\n"); */
				break;
			}
			usleep(c->cmd_timeout);
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
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
static int write_if(struct open_gpib_dev *d, void *buf, int len)
{
	struct ip_ctl *c;
	int i;
	char *m;
	printf("write_if %s\n",d->if_name);
	c=(struct ip_ctl *)d->dev;
	m=(char *)buf;
	
	if('\n' != m[len-1]){ /**Make sure we have terminator...  */
		if(NULL == (m=calloc(1,len+2)) ){
			fprintf(stderr,"%s out of mem for %d\n",__func__,len+2);
			return -1;
		}
		/*fprintf(stderr,"Alloc new msg for '%s'\n",g->buf); */
		memcpy(m,buf,len);
		m[len++]='\n';
		m[len]=0;
	}
	
	for (i=0;i<len;){
		int r;
		if(c->debug)
			fprintf(stderr,"Sending '%s' i=%d, len=%d\n",m,i,len);	
		r= send ( c->sockfd, &m[i], len-i, 0 );
		if(c->debug)
			fprintf(stderr,"Sent %d bytes\n",r);
		if(-1 == r){
			fprintf(stderr,"Err Sending at byte %d\n",i);
			perror("send");
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
\n\b Returns: -1 on error or 0 all OK
****************************************************************************/
static int close_if(struct open_gpib_dev *d)
{
	int i;
	struct ip_ctl *c;
	c=(struct ip_ctl *)d->dev;
	if(0!= (i=shutdown(c->sockfd,SHUT_RDWR)) ){
		fprintf(stderr,"Error closing interface\n");
		return -1;
	}
	close(c->sockfd);
	free (d->dev);
	d->dev=NULL;
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int control_if(struct open_gpib_dev *d, int cmd, uint32_t data)
{
	struct ip_ctl *p;
	p=(struct ip_ctl *)d->dev;
	/**auto-allocate so we can use the control structure before we open.  */
/*	fprintf(stderr,"p=%p &p=%p, &dev=%p\n",p,&p,&d->dev); */
	if(-1==check_calloc(sizeof(struct ip_ctl), &p, __func__,(void *)&d->dev) == -1) return -1;
		
	switch(cmd){
		case CMD_SET_CMD_TIMEOUT:
			p->cmd_timeout=data;
			printf("ip cmdtimeout=%ld\n",(long)data);
			break;
		case CMD_SET_DEBUG:
			if(data)
				p->debug=1;
			else
				p->debug=0;
			printf("ip dbg=%d\n",p->debug);
			break;
		case IP_CMD_SET_PORT:
			p->port=data;
			printf("%s: ip port=%ld\n",d->if_name,(long)data);
			break;
	}
	return 0;
}
/***************************************************************************/
/** Allocate our internal data structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void *calloc_internal(void)
{
	void *p=NULL;
	printf("inet:calloc_internal\n");
	if(-1 == check_calloc(sizeof(struct ip_ctl), &p,__func__,NULL) ) 
		return NULL;
	printf("rtncalloc\n");
	return p;
}
/***************************************************************************/
/** Nothing to do - open does it.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int init_if( struct open_gpib *d)
{
	
}
GPIB_TRANSPORT_FUNCTION(inet)
OPEN_GPIB_ADD_CMD(IP_CMD_SET_PORT)

