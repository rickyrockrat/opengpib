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
#include "open-gpib.h"

struct hp16500c_ctl {
	int addr; /**internal address, used to set address when != gpib.addr  */
	int debug;
	int cmd_wait;
	struct open_gpib_dev *dev; /**ip interface control  */
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int hp16500c_init( struct open_gpib *g)
{
	fprintf(stderr,"h156c-init %d\n",g->type_ctl);
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
int ctl_hp16500c_open( struct open_gpib_dev *ctl, char *ip)
{
	struct hp16500c_ctl *c;
	open_gpib_register ip_reg;
	if(NULL == ctl){
		printf("%s: ctl null\n",__func__);
		return -1;
	}
	/** c=(struct hp16500c_ctl *)ctl->dev;
	if(-1 ==check_calloc(sizeof(struct hp16500c_ctl), &c,__func__,&ctl->dev) ) return -1;*/
	open_gpib_list_interfaces();
	if(NULL == (ip_reg=open_gpib_find_interface("inet", OPEN_GPIB_REG_TYPE_TRANSPORT))){
		return -1;
	}
	c=(struct hp16500c_ctl *)ctl->dev;
	if(-1 ==check_calloc(sizeof(struct hp16500c_ctl), &c,__func__,&ctl->dev) ) return -1;
/*	printf("Calling ip_reg, var=%p,control=%p\n",&c->dev, &c->dev.control);  */
/*	c->dev.read=NULL; */
	printf("ip_rg\n");
	if(NULL == (c->dev=ip_reg())) /**load our ip function list  */
		goto err;
	printf("control\n");
/*	printf("Back\n"); */
/*	printf("ctl=%p\n",c->dev.control); */
	/**set the port - IMPORTANT! Be sure to do this before opening. */
	c->dev->funcs.og_control(c->dev,IP_CMD_SET_PORT,5025);
	/**set net timeout - Set this too, or you will timeout when opening */
	c->dev->funcs.og_control(c->dev,CMD_SET_CMD_TIMEOUT,50000); /**uS  */
	c->dev->funcs.og_control(c->dev,CMD_SET_DEBUG,c->debug); /**pass down debug option to interface level  */
	printf("open '%s'\n",ip);
	/**open ip address on port set above  */
	if(-1 == c->dev->funcs.og_open(c->dev,ip))
		goto err;
	printf("Exit hcipopen\n");
	return 0;
err:
	
	if(NULL != c->dev)
		free(c->dev);
	free(c);
	ctl->dev=NULL;
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
int ctl_hp16500c_write(struct open_gpib_dev *d, void *buf, int len)
{
	struct hp16500c_ctl *c;
	int i;
	char *m;
	
	c=(struct hp16500c_ctl *)d->dev;
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
	if((i=c->dev->funcs.og_write(c->dev,m,len)) <0) {/* error writing */
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
int ctl_hp16500c_read(struct open_gpib_dev *d, void *buf, int len)
{
	struct hp16500c_ctl *c;
	int i;
	char *m;
	m=(char *)buf;
	c=(struct hp16500c_ctl *)d->dev;
	/*fprintf(stderr,"Looking for %d bytes\n",len); */
	fprintf(stderr,"calling %s read\n",c->dev->if_name);
	i=c->dev->funcs.og_read(c->dev,buf,len);
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
int ctl_hp16500c_ctrl( struct open_gpib_dev *ctl, int cmd, uint32_t data)
{
	struct hp16500c_ctl *c; 
	int i;
	char buf[100];

	if(NULL == ctl){
		fprintf(stderr,"hp16500c: ctl null\n");
		return -1;
	}
	c=(struct hp16500c_ctl *)ctl->dev;
	if(-1 ==check_calloc(sizeof(struct hp16500c_ctl), &c,__func__,&ctl->dev) ) return -1;
		
	switch(cmd){
		case CTL_CLOSE:
			if(c->debug) fprintf(stderr,"Got Close ctl. Doing nothing hp INET\n");
			
			break;
		case CTL_SET_TIMEOUT:
			if(data >5000)
				ctl->wait=data;
			else
				ctl->wait=5000;
			
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
				ctl->debug=1;
			else
				ctl->debug=0;
			break;
		case CTL_SEND_CLR:
			i=sprintf(buf,"*CLS\n");
			ctl_hp16500c_write(ctl,buf,i);
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
int ctl_hp16500c_close(struct open_gpib_dev *ctl)
{
	int i;
	struct hp16500c_ctl *c;
	
	
	if(NULL == ctl->dev)
		return 0;
	c=(struct hp16500c_ctl *)ctl->dev;
	if(0!= (i=c->dev->funcs.og_close(c->dev)) ){
		fprintf(stderr,"Error closing interface\n");
	}
	fprintf(stderr,"hp16500C Close, free iface internal %s\n",c->dev->if_name);
	free (c->dev);
	c->dev=NULL;
	return i;
}


int ctl_hp16500c_init (struct open_gpib *g)
{
	return 0;
}

/***************************************************************************/
/** Allocate our internal data structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void *calloc_internal(void)
{
	void *p;
	if(-1 == check_calloc(sizeof(struct hp16500c_ctl), &p,__func__,NULL) ) 
		return NULL;
	return p;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int register_hp16500c( struct open_gpib *g)
{
	if(NULL == g){
		fprintf(stderr,"%s, incomming struct null\n",__func__);
		return -1;
	}
	if(-1 ==check_calloc(sizeof(struct open_gpib_dev), &g->ctl,__func__,NULL) ) return -1;
	g->ctl->funcs.og_control=ctl_hp16500c_ctrl;
	g->ctl->funcs.og_read=	ctl_hp16500c_read;
	g->ctl->funcs.og_write=	ctl_hp16500c_write;
	g->ctl->funcs.og_open=	ctl_hp16500c_open;
	g->ctl->funcs.og_close=	ctl_hp16500c_close;
	g->ctl->funcs.og_init=	ctl_hp16500c_init;
	g->ctl->dev=calloc_internal();
	return 0;
}
