/** \file ******************************************************************
\n\b File:        gpib.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: common gpib functions...
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gpib.h"
#include "serial.h"
#include "prologixs.h"
#include "hp16500ip.h"
#include "fileio.h"
struct supported_dev {
	int type;
	char *option;
	char *name;
};
static struct supported_dev s_dev[]={\
	{GPIB_CTL_PROLOGIXS,"prologixs","Prologix Serial GPIB Controller"},														
	{GPIB_CTL_HP16500C,"hpip","HP 16500C LAN Controller"},
  {GPIB_CTL_FILEIO,"file","File Reader"},
	{-1,NULL,NULL},
};

/***************************************************************************/
/** .
\n\b Arguments:
len is max_len for msg.
\n\b Returns: number of chars read.
****************************************************************************/
int read_raw(struct gpib *g)
{
	int i,j;
	i=0;
	while( (j=g->read(g->ctl,&g->buf[i],g->buf_len-i)) >0){
		i+=j;
		usleep(1000);
	}
	/*fprintf(stderr,"Got %d bytes\n'%s'\n",i,g->buf); */
	return i;
}


/***************************************************************************/
/** .
\n\b Arguments:
len is max_len for msg.
\n\b Returns: number of chars read.
****************************************************************************/
int read_string(struct gpib *g)
{
	int i,j;
	i=0;
	while( (j=g->read(g->ctl,&g->buf[i],g->buf_len-i)) >0){
		i+=j;
		usleep(1000);
	}
	g->buf[i]=0;
	/*fprintf(stderr,"Got %d bytes\n'%s'\n",i,g->buf);  */
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_string(struct gpib *g, char *msg)
{
	int i;
	if(NULL == msg){
		fprintf(stderr,"msg null\n");
		return -1;
	}
	/*fprintf(stderr,"WS:%s",msg); */
	i=strlen(msg);
	return g->write(g->ctl,msg,i);
}

/***************************************************************************/
/** Write a string, then print error message.	FIXME malloc xbuf.
\n\b Arguments:
\n\b Returns: 0 on fail or number of chars read on success.
****************************************************************************/
int write_get_data (struct gpib *g, char *cmd)
{
	int i;
	/*fprintf(stderr,"Write... "); */
	write_string(g,cmd);
	/*fprintf(stderr,"Read..."); */
	if(-1 == (i=read_string(g)) ){
		fprintf(stderr,"%s:Unable to read from port for %s\n",__func__,cmd);
		return 0;
	}
	/*fprintf(stderr,"WGD rtn\n"); */
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_wait_for_data(struct gpib *g, char *cmd, int sec)
{
	int i,rtn;
	rtn =0;
	write_string(g,cmd);
	for (i=0; i<sec*2;++i){
		if((rtn=read_string(g)))
			break;
		usleep(500000);
	}
	return rtn;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1 on error or bytes written on success.
****************************************************************************/
int write_cmd(struct ginstrument *gi, char *cmd)
{
	if(gi->addr != gi->g->addr){
		if(NULL !=gi->g->control){
			if(0 != gi->g->control(gi->g,CTL_SET_ADDR,gi->addr)){
				fprintf(stderr,"Unable to set instrument address %d\n",gi->addr);
				return -1;
			}
			
		}else{
			fprintf(stderr,"Unable to set instrument address (%d!=%d). device control not set\n",gi->addr,gi->g->addr);
		}
	}
	return write_string(gi->g,cmd);
		
}
/***************************************************************************/													
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct gpib *open_gpib(int ctype, int addr, char *dev_path, int buf_size)
{
	struct gpib *g;
	fprintf(stderr,"OpenGPIB Version %s\n",TOSTRING(VERSION));  
	if(NULL == dev_path){
		fprintf(stderr,"Device name is NULL. Must specify device.\n");
		return NULL;
	}
	if(NULL == (g=malloc(sizeof(struct gpib)) ) ){
		fprintf(stderr,"Out of mem on gpib alloc\n");
		return NULL;
	}
	memset(g,0,sizeof(struct gpib));
	g->addr=addr;
	if(0>= buf_size)
		buf_size=8096;
  /**make sure it's a 8-byte multiple, so we stop on largest data boundry  */
  buf_size=((buf_size+7)/8)*8;
	if(NULL == (g->buf=malloc(buf_size) ) ){
		fprintf(stderr,"Out of mem on buf size of %d\n",buf_size);
		free(g);
		return NULL;
	}
	g->buf_len=buf_size;
	/*fprintf(stderr,"Using %d buf size\n",g->buf_len); */
	g->type_ctl=ctype;
	/**set up the controller and interface. FIXME Replace with config file.  */
	switch(ctype&CONTROLLER_TYPEMASK){
		case GPIB_CTL_PROLOGIXS:
			if(register_prologixs(g))
				goto err1;
			if(ctype&OPTION_DEBUG)
				g->control(g,CTL_SET_DEBUG,1);
			if(-1==g->open(g,dev_path))
				goto err;
			break;
		case GPIB_CTL_HP16500C:
			if(-1==register_hp16500c(g))
				goto err1;
			if(ctype&OPTION_DEBUG)
				g->control(g,CTL_SET_DEBUG,1);
			if(-1==g->open(g,dev_path))
				goto err;
			break;
    case GPIB_CTL_FILEIO:
			if(-1==register_fileio(g))
				goto err1;
			if(ctype&OPTION_DEBUG)
				g->control(g,CTL_SET_DEBUG,1);
			if(-1==g->open(g,dev_path))
				goto err;
			break;
		default:
			fprintf(stderr,"Unknown controller %d\n",ctype&CONTROLLER_TYPEMASK);
			goto err1;
			break;
	}
	g->dev_path=strdup(dev_path);
	return g;
err:
	g->close(g);
err1:
	free(g);
	return NULL;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int close_gpib (struct gpib *g)
{
	if(NULL != g->control)
		g->control(g,CTL_CLOSE,0);
	if(NULL != g->close)
		g->close(g);
	if(NULL != g->dev_path)
		free(g->dev_path);
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int gpib_option_to_type(char *op)
{
	int i;
	for (i=0; NULL != s_dev[i].name;++i){
		if(!strcmp(s_dev[i].option,op))
			return s_dev[i].type;
	}	
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_gpib_supported_controllers(void)
{
	int i;
	fprintf(stderr,"Type 'Option' Name\n");
	for (i=0; NULL != s_dev[i].name;++i){
		fprintf(stderr,"%d    '%s' %s\n",gpib_option_to_type(s_dev[i].option),s_dev[i].option, s_dev[i].name);
	}
}

/***************************************************************************/
/** Set up timeouts and send the id string.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_id(struct gpib *g, char *idstr)
{
	g->control(g,CTL_SET_TIMEOUT,500);
	while(read_string(g));
	g->control(g,CTL_SET_TIMEOUT,50000);
	/*write_string(g,"*CLS"); */
	if(0 == write_get_data(g,idstr))
		return -1;
	return 0;
}

/***************************************************************************/
/** Check a pointer, if it is null, allocate to specified size.
\n\b Arguments:
\n\b Returns: 0 on OK, 1 on allocated memory, -1 if out of mem or other error
****************************************************************************/
int check_calloc(size_t size, void *x, const char *func, void *s)
{
	const char *f;
	void **p=x;
	void **set=s;
	/**sanity checks  */
	if(NULL != func)	f=func;
	else f="";
	if(NULL == p) return -1;
		
	if( NULL == *p){
		if(NULL ==(*p=calloc(1,size)) ){
			printf("Out of mem on %s alloc\n",f);
			return -1;
		}
		if(NULL != set)
			*set=*p;
		return 1;
	}
	return 0;
}
