/** \file ******************************************************************
\n\b File:        gpib.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: common gpib functions...to talk to prologix.
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.3  2009-04-06 20:57:26  dfs
Major re-write for new gpib API

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
	int i,j;
	i=0;
	while( (j=g->read(g->ctl,&g->buf[i],g->buf_len-i)) >0){
		i+=j;
	}
	g->buf[i]=0;
	/*printf("Got %d bytes\n'%s'\n",i,g->buf); */
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
	g->type_ctl=ctype;
	/**set up the controller and interface. FIXME Replace with config file.  */
	switch(ctype){
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
	return 0;
}

