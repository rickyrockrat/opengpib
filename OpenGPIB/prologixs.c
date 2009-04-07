/** \file ******************************************************************
\n\b File:        prologix.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        04/06/2009 10:53 am
\n\b Description: Prologix interface routines
*/ /************************************************************************
Change Log: \n
*/

#include "gpib.h"
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
	printf("Init Prologix controller\n");
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
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(g->buf,"Prologix")){
		printf("%s:Unable to find correct ver in\n%s\n",__func__,g->buf);
		return -1;
	}
	printf("Talking to Controller '%s'\n",g->buf);
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
	verify(g,"++addr");
	verify(g,"++eoi");
	verify(g,"++eos");
	verify(g,"++auto");
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int control_prologixs(struct gpib *g, int cmd, int data)
{
	int action;
	struct prologixs_ctl *c; 
	if(NULL == g){
		printf("prologixs: gpib null\n");
		return 1;
	}
	c=(struct prologixs_ctl *)g->ctl;
	switch(cmd){
		case CTL_CLOSE:
			printf("Closing gpib\n");
			break;
		case CTL_SET_TIMEOUT:
			return c->serial.control(&c->serial,SERIAL_CMD_SET_CHAR_TIMEOUT,data);
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
\n\b Returns: 1 on failure, 0 on success
****************************************************************************/
int _prologixs_open(struct gpib *g, char *path)
{
	struct prologixs_ctl *c;
	if(NULL == g){
		printf("%s: dev null\n",__func__);
		return 1;
	}
	
	if(NULL ==(c=malloc(sizeof(struct prologixs_ctl))) ){
		printf("Out of mem on prologix ctl alloc\n");
		return 1;
	}
	if(serial_register(&c->serial)) /**load our function list  */
		return 1;
	if(c->serial.open(&c->serial,path))
		return 1;
	c->serial.control(&c->serial,SERIAL_CMD_SET_CHAR_TIMEOUT,50000);
	c->autor=1;
	g->ctl=c;
	if(-1 == prologixs_init(g)){
		printf("Controller init failed\n");
		c->serial.close(&c->serial);
		free (g->ctl);
		g->ctl=NULL;
		/*printf("_po rtn\n"); */
		return 1;
	}
	
	return 0;
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

