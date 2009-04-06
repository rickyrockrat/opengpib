/** \file ******************************************************************
\n\b File:        gpib.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: common gpib functions...to talk to prologix.
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.2  2008/10/06 12:45:08  dfs
Added write_get_data, auto to init_prologix, added \r auto term in write_string

Revision 1.1  2008/10/06 07:54:16  dfs
moved from get_tek_waveform

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
int read_string(struct gpib *g)
{
	int i;
	if((i=g->read(g->dev,g->buf,g->buf_len))>=0){
		g->buf[i]=0;
	}
	
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_string(struct gpib *g, char *msg)
{
	int rtn,i;
	char *m;
	if(NULL == msg){
		printf("msg null\n");
		return -1;
	}
	i=strlen(msg);
	
	if('\r' != msg[i-1]){ /**Make sure we have terminator...  */
		if(NULL == (m=malloc(i+2)) ){
			printf("out of mem for %d\n",i+2);
			return -1;
		}
		/*printf("Alloc new msg for '%s'\n",g->buf); */
		i=sprintf(m,"%s\r",msg);
	}else
		m=msg;
	
	if((rtn=g->write(g->dev,m,i)) <0) {/* error writing */
		rtn=-1;
		goto end;
  }
	/*printf("wrote %d bytes\n%s\n",rtn,g->buf); */
	if(rtn != i)
		printf("Write mis-match %d != %d\n",rtn,i);
	usleep(500);
end:
	if(m!=msg)
		free(m);
	return rtn;	
}

/***************************************************************************/
/** Write a string, then print error message.	FIXME malloc xbuf.
\n\b Arguments:
\n\b Returns: 0 on fail
****************************************************************************/
int write_get_data (struct gpib *g, char *cmd)
{
	char xbuf[100];
	int i;
	sprintf(xbuf,"%s\r",cmd);
	write_string(g,xbuf);
	write_string(g,"++read");
	if(-1 == (i=read_string(g)) ){
		printf("%s:Unable to read from port for %s\n",__func__,cmd);
		return 0;
	}
	return i;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct gpib *open_gpib(int ctype, int addr, char *dev_path)
{
	struct gpib *g;
	if(NULL == (g=malloc(sizeof(struct gpib)) ) ){
		printf("Out of mem on serial register\n");
		return NULL;
	}		
	memset(g,0,sizeof(struct gpib));
	g->addr=addr;
	g->buf_len=BUF_SIZE;
	/**set up the controller and interface. FIXME Replace with config file.  */
	switch(ctype){
		case GPIB_CTL_PROLOGIXS:
			g->type_if=GPIB_IF_SERIAL;
			if(register_prologixs(g))
				goto err1;
			break;
		default:
			printf("Unknown controller %d\n",ctype);
			goto err1;
			break;
	}
	g->type_ctl=ctype;
	/**open our interface to the controller  */
	switch(g->type_if){
		case GPIB_IF_SERIAL:
			if(serial_register(g)) /**load our function list  */
				goto err1;
			if(g->open(g,dev_path))
				goto err;
			break;
		default:
			printf("Don't know how to handle interface type %d\n",g->type_if);
			goto err1;
			break;
	}
	/**open our controller  */
	switch(g->type_ctl){
		case GPIB_CTL_PROLOGIXS:
			if(g->control(g, CTL_OPEN))
				goto err;
			break;
	}
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
		g->control(g,CTL_CLOSE);
	if(NULL != g->close)
		g->close(g);
	return 0;
}
