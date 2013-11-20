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
#include "open-gpib.h"
#include "serial.h"
#include "prologixs.h"
#include "hp16500ip.h"
#include "fileio.h"
#include "interfaces.h" 

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
\n\b Returns:
****************************************************************************/
int open_gpib_list_interfaces(void)
{
	int i;
	for (i=0;NULL != IF_LIST[i].name; ++i){
		fprintf(stderr,"%s, %d, %p\n",IF_LIST[i].name, IF_LIST[i].type, IF_LIST[i].func);
	}
	
}

/***************************************************************************/
/** .
\n\b Arguments: 
name is the name of the interface, like ip/usb/serial.
type is either controller or transport
                
\n\b Returns:	function found or null;
****************************************************************************/
open_gpib_register open_gpib_find_interface(char *name, int type)
{
	int i;
	if( NULL == name)
		return;
	switch (type){
		case OPEN_GPIB_REG_TYPE_CONTROLLER:
		case OPEN_GPIB_REG_TYPE_TRANSPORT:
			break;
		default:
			fprintf(stderr,"Unknown type %d\n",type);
			return NULL;
	}
	for (i=0;NULL != IF_LIST[i].name; ++i){
		if(!strcmp(name,IF_LIST[i].name) &&	type == IF_LIST[i].type) {
			fprintf(stderr,"Found '%s', type %d, @%p\n",IF_LIST[i].name, IF_LIST[i].type, IF_LIST[i].func);
			return IF_LIST[i].func;		
		}
			
		
	}
	fprintf(stderr,"Canot find interface '%s', type %d\n", IF_LIST[i].name,IF_LIST[i].type);
	return NULL;
}
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
	fprintf(stderr,"OpenGPIB Version %s\n",PACKAGE_VERSION);  
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
size is size in bytes to allocate
x is a pointer to a pointer, and *x will be allocated.
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
			fprintf(stderr,"Out of mem on %s alloc\n",f);
			return -1;
		}	
	/*	else fprintf(stderr,"Allocated %d: %p->%p, set %p->%p\n",size,*p,p,*set,set); */
		if(NULL != set)
			*set=*p;
		return 1;
	}
	return 0;
}
/*from common.c*/

/***************************************************************************/
/** Tests to see if entire string is made of digits.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int is_string_number(char *s)
{
	while (*s){
		if(!isdigit(*s))
			return 0;
		++s;
	}
	return 1;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int og_next_col(struct c_opts *o)
{
	int i;
	for (i=0;i!=EOF && i!= o->dlm;){
		i=fgetc(o->fd);
	}
	if(EOF == i){
		printf("Got EOF while looking for '%c'\n",o->dlm);
		return 1;
	}
		
	while(i==o->dlm)
		i=fgetc(o->fd);
	if(EOF ==i){
		printf("Got EOF while removing delims\n");
		return 1;
	}
		
	if(i == '\n'){
		printf("Hit \n looking for col %d\n",o->col);
		return 1;
	}
	ungetc(i,o->fd);
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int og_get_col(struct c_opts *o, float *f)
{
	int i;
	if(o->col >1){
		for (i=1;i<o->col;++i)	{
			if(og_next_col(o))
				return -1;
		}
			
	}
	if( EOF ==fscanf(o->fd,"%f",f))
		return -1;
	for (i=0;i!=EOF && i!='\n';)
		i=fgetc(o->fd);
	return 0;
}
/***************************************************************************/
/** find the range, divide val by range, and set m.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
double format_eng_units(double val, int *m)
{
	double x;
	int i;
	if(val >=0){/**positive  */
		for (i=0,x=1000;val>x;){
			++i;
			x*=1000;
		}
		x/=1000;
		printf("val=%f x=%f i=%d\n",val,x,i);
		x=val/x;
	}	else{
		for (i=0,x=.001;val<x;){
			--i;
			x/=1000;
		}
		x*=1000;
		x=val*x;
	}
	switch(i){
		case 0:	break;
		case 1: *m='K'; break;
		case 2: *m='M'; break;
		case 3: *m='G'; break;
		case -1: *m='m'; break;
		case -2: *m='u'; break;
		case -3: *m='n'; break;
		case -4: *m='p'; break;	
			default:
				printf("Unknown unit %d (power of 1000)\n",i);
				
	}
	return x;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
double og_get_value( char *f, char *buf)
{
	char tbuf[50], *find;
	int i,k;
	double x; 
	
	find=strstr(buf,f);
	if(NULL==find)
		return -1;
	for (k=0,i=strlen(f)+1; k<49 && find[i] != ',' && find[i] != ';';++i,++k)
		tbuf[k]=find[i];
	tbuf[k]=0;
	/*printf("Found %s->%s",f,tbuf);   */
	x=strtof(tbuf, NULL);
	/*printf(" : %f\n",x); */
	
	return(x );	 
}

/***************************************************************************/
/** Get the string following the title.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char * og_get_string( char *f, char *buf)
{
	char tbuf[50], *find;
	int i,k;
	
	find=strstr(buf,f);
	if(NULL==find)
		return "NOTFOUND";
	for (k=0,i=strlen(f)+1; k<30 && find[i] != ',' && find[i] != ';';++i,++k)
		tbuf[k]=find[i];
	tbuf[k]=0;
	/*printf("Found %s->%s\n",f,tbuf); */
	
	return(strdup(tbuf));	 
}
/***************************************************************************/
/** Get the string at column number. 
\n\b Arguments:
col - zero-based column number
buf - buffer to parse
\n\b Returns: allocated string for column value
****************************************************************************/
char * og_get_string_col( int col, char *buf)
{
	char tbuf[50];
	int i,k,c;
	for (c=i=k=0; buf[k] ;++k){
		if(',' == buf[k])
			++c;
		else if(col==c){
			tbuf[i++]=buf[k];	
		}else if(c>col)
			break;
	}
	if(c<col)
		return strdup("NOTFOUND");
	tbuf[i]=0;
	/*printf("Found %d->%s",col,tbuf);   */
	return(strdup(tbuf));	 
}
/***************************************************************************/
/** Get the value at the column. Count commas.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
double og_get_value_col( int col, char *buf)
{
	double x; 
	char *v;
	
	v=og_get_string_col(col,buf);
	
	if(!strcmp("NOTFOUND",v) ){ 
		x=-1;
	}else
		x=strtod(v, NULL);
	
	/*printf("'%s' : %f\n",v,x);  */
	free(v);
	return(x );	 
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void og_usleep(int usec)
{
	struct timespec t,r;
	r.tv_sec=usec/1000;
	usec-=r.tv_sec*1000;
	r.tv_nsec=usec*1000;
	nanosleep(&t,&r);	
	/** do{
		memcpy(&t,&r, sizeof(struct timespec));
	
	}while(r.tv_sec || r.tv_nsec);*/
	
}
