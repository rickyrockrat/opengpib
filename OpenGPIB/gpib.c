/** \file ******************************************************************
\n\b File:        gpib.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: common gpib functions...
*/ /************************************************************************
Change Log: \n
 This file is part of OpenGPIB.

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
	/*printf("Got %d bytes\n'%s'\n",i,g->buf); */
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
	/*printf("Got %d bytes\n'%s'\n",i,g->buf);  */
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
		printf("msg null\n");
		return -1;
	}
	/*printf("WS:%s",msg); */
	i=strlen(msg);
	return g->write(g->ctl,msg,i);
}

/***************************************************************************/
/** Write a string, then print error message.	FIXME malloc xbuf.
\n\b Arguments:
\n\b Returns: 0 on fail
****************************************************************************/
int write_get_data (struct gpib *g, char *cmd)
{
	int i;
	/*printf("Write... "); */
	write_string(g,cmd);
	/*printf("Read..."); */
	if(-1 == (i=read_string(g)) ){
		printf("%s:Unable to read from port for %s\n",__func__,cmd);
		return 0;
	}
	/*printf("WGD rtn\n"); */
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_wait_for_data(char *msg, int sec, struct gpib *g)
{
	int i,rtn;
	rtn =0;
	write_string(g,msg);
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
				printf("Unable to set instrument address %d\n",gi->addr);
				return -1;
			}
			
		}else{
			printf("Unable to set instrument address (%d!=%d). device control not set\n",gi->addr,gi->g->addr);
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
	printf("OpenGPIB Version ""VERSION""\n");
	if(NULL == (g=malloc(sizeof(struct gpib)) ) ){
		printf("Out of mem on serial register\n");
		return NULL;
	}
	memset(g,0,sizeof(struct gpib));
	g->addr=addr;
	if(0>= buf_size)
		buf_size=8096;
	if(NULL == (g->buf=malloc(buf_size) ) ){
		printf("Out of mem on buf size of %d\n",buf_size);
		free(g);
		return NULL;
	}
	g->buf_len=buf_size;
	/*printf("Using %d buf size\n",g->buf_len); */
	g->type_ctl=ctype;
	/**set up the controller and interface. FIXME Replace with config file.  */
	switch(ctype&CONTROLLER_TYPEMASK){
		case GPIB_CTL_PROLOGIXS:
			if(register_prologixs(g))
				goto err1;
			if(g->open(g,dev_path))
				goto err;
			break;
		default:
			printf("Unknown controller %d\n",ctype);
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

