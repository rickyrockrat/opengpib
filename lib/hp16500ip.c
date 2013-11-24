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
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init__hp16500cip( struct open_gpib_mstr *g)
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
int open_hp16500cip( struct open_gpib_dev *ctl, char *ip)
{
	open_gpib_register ip_reg;
	if(NULL == ctl){
		printf("%s: ctl null\n",__func__);
		return -1;
	}
	/** c=(struct hp16500c_ctl *)ctl->internal;
	if(-1 ==check_calloc(sizeof(struct hp16500c_ctl), &c,__func__,&ctl->internal) ) return -1;*/
	open_gpib_list_interfaces();
	if(NULL == (ip_reg=open_gpib_find_interface("inet", OPEN_GPIB_REG_TYPE_TRANSPORT))){
		return -1;
	}
	if(-1 ==check_calloc(sizeof(struct hp16500c_ctl), &ctl->internal,__func__,NULL) ) return -1;
/*	printf("Calling ip_reg, var=%p,control=%p\n",&c->internal, &c->internal.control);  */
/*	c->internal.read=NULL; */
	if(ctl->debug >=DBG_TRACE)	fprintf(stderr,"ip_rg\n");
	if(NULL == (ctl->dev=ip_reg())) /**load our ip function list  */
		goto err;
	if(ctl->debug >=DBG_TRACE) fprintf(stderr,"control\n");
/*	printf("Back\n"); */
/*	printf("ctl=%p\n",c->dev.control); */
	/**set the port - IMPORTANT! Be sure to do this before opening. */
	ctl->dev->funcs.og_control(ctl->dev,IP_CMD_SET_PORT,5025);
	/**set net timeout - Set this too, or you will timeout when opening */
	ctl->dev->funcs.og_control(ctl->dev,CMD_SET_CMD_TIMEOUT,50000); /**uS  */
	ctl->dev->funcs.og_control(ctl->dev,CMD_SET_DEBUG,ctl->debug); /**pass down debug option to interface level  */
	if(ctl->debug >=DBG_TRACE) fprintf(stderr,"open '%s'\n",ip);
	/**open ip address on port set above  */
	if(-1 == ctl->dev->funcs.og_open(ctl->dev,ip))
		goto err;
	if(ctl->debug >=DBG_TRACE) fprintf(stderr,"Exit hcipopen\n");
	return 0;
	
err:
	if(ctl->debug >=DBG_TRACE) fprintf(stderr,"%s Error exit\n",__func__);
	if(NULL != ctl->dev){
		ctl->dev->funcs.og_close(ctl->dev);
		free(ctl->dev);
		ctl->dev = NULL;
	}
	if( NULL != ctl->internal)
		free(ctl->internal);
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
int write_hp16500cip(struct open_gpib_dev *ctl, void *buf, int len)
{
	int i;
	char *m=(char *)buf;
	
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
	if(ctl->debug)
		fprintf(stderr,"Sending '%s'\n",m);
	if((i=ctl->dev->funcs.og_write(ctl->dev,m,len)) <0) {/* error writing */
		i=-1;
		fprintf(stderr,"%s: Error Writting\n",__func__);
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
int read_hp16500cip(struct open_gpib_dev *ctl, void *buf, int len)
{
	struct hp16500c_ctl *c;
	int i;
	char *m;
	m=(char *)buf;
	c=(struct hp16500c_ctl *)ctl->dev;
	/*fprintf(stderr,"Looking for %d bytes\n",len); */
	fprintf(stderr,"calling %s read\n",ctl->dev->if_name);
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
int control_hp16500cip( struct open_gpib_dev *ctl, int cmd, uint32_t data)
{
	int i;
	char buf[100];

	if(NULL == ctl){
		fprintf(stderr,"%s: ctl null\n",__func__);
		return -1;
	}
		
	switch(cmd){
		case CTL_CLOSE:
			if(ctl->debug) fprintf(stderr,"Got Close ctl. Doing nothing hp INET\n");
			
			break;
		case CTL_SET_TIMEOUT:
			if(data >5000)
				ctl->wait=data;
			else
				ctl->wait=5000;
			
			break;
		case CTL_SET_ADDR: /**send command, check result, then set gpib addr.  */
			fprintf(stderr, "%s: Ignoring CTL_SET_ADDR\n",__func__);
			/*i=sprintf(cmdbuf,"++addr %d\n",data); */
			/*if(c->serial.write(&c->serial,cmdbuf,i) == i){ */
			/*	g->addr=data; */
			/*	return 1; */
			/*} */
			break;
		case CTL_SET_DEBUG:
				ctl->debug=data;
			break;
		case CTL_SEND_CLR:
			i=sprintf(buf,"*CLS\n");
			write_hp16500cip(ctl,buf,i);
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
int close_hp16500cip(struct open_gpib_dev *ctl)
{
	int i;
	
	if(NULL == ctl->dev)
		return 0;
	if(0!= (i=ctl->dev->funcs.og_close(ctl->dev)) ){
		fprintf(stderr,"Error closing interface\n");
	}
	fprintf(stderr,"hp16500C Close, free iface internal %s\n",ctl->dev->if_name);
	free (ctl->dev);
	ctl->dev=NULL;
	return i;
}


int init_hp16500cip (struct open_gpib_mstr *g)
{
	if(NULL == g)
		return -1;
	return 0;
}

/***************************************************************************/
/** Allocate our internal data structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void *calloc_internal_hp16500cip(void)
{
	return calloc_internal(sizeof(struct hp16500c_ctl),__func__);
}

GPIB_CONTROLLER_FUNCTION(hp16500cip)

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
***************************************************************************
int register_hp16500c( struct open_gpib_mstr *g)
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
}*/
