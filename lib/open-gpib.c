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
ptr is the single element, fmt contains u,s, or c.
\n\b Returns: 1 on error 0 on success.
****************************************************************************/
int _open_gpib_set_param(struct open_gpib_param *ptr,char *fmt, ...)
{
	va_list ap;
	if(NULL == ptr)
		return 1;
	
	va_start(ap, fmt);
	switch (*fmt) {
		case OG_PARAM_TYPE_UINT32:  ptr->val.u=va_arg(ap, int32_t); break;
		case OG_PARAM_TYPE_INT32:   ptr->val.s=va_arg(ap, int32_t); break;
		case OG_PARAM_TYPE_STRING:  ptr->val.c=va_arg(ap, char *);	break;
		default: fprintf(stderr,"%s: Unknown type '%c'\n",__func__,*fmt);
			goto err;
	}	
	va_end(ap);	
	ptr->type=*fmt;
	return 0;
err:
	return 1;
}

/***************************************************************************/
/** .
\n\b Arguments:
head is head of list
name is name of var
fmt is one of "u" "s" "c"
last is value
\n\b Returns: 1 on error 0 on success.
****************************************************************************/
int open_gpib_set_param(struct open_gpib_param *head,char *name, char *fmt, ...)
{
	va_list ap;
	struct open_gpib_param *i;
	
	if(NULL == head || NULL == name)
		return 1;
	for (i=head;NULL != i; i=i->next){
		if(!strcmp(name,i->name)){
			void *ptr;
			va_start(ap, fmt);
			/**this trick only works because we have a limited number types, 
			and a pointer should be large enough to pass all types we use */
			ptr=va_arg(ap,void *); 
			va_end(ap);
			return(_open_gpib_set_param(i,fmt,ptr)); /**function only takes one arg.  */
		}
	}	
	fprintf(stderr,"%s: not found %s\n",__func__,name);
	return 1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct open_gpib_param *open_gpib_get_param_ptr(struct open_gpib_param *head,char *name)
{
	struct open_gpib_param *i;
	if(NULL == head || NULL == name)
		return NULL;
	for (i=head;NULL != i; i=i->next){
		if(!strcmp(name,i->name))
			return i;
	}	
	return NULL;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_gpib_set_uint32_t(struct open_gpib_param *head,char *name,uint32_t val)
{
	struct open_gpib_param *p=open_gpib_get_param_ptr(head,name);
	if(NULL == p)
		return 1;
	p->val.u=val;
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_gpib_set_string(struct open_gpib_param *head,char *name,char *str)
{
	struct open_gpib_param *p=open_gpib_get_param_ptr(head,name);
	if(NULL == p || NULL ==str)
		return 1;
	if(NULL != p->val.c)
		free(p->val.c);
	
	p->val.c=strdup(str);
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_gpib_set_int32_t(struct open_gpib_param *head,char *name,int32_t val)
{
	struct open_gpib_param *p=open_gpib_get_param_ptr(head,name);
	if(NULL == p)
		return 1;
	p->val.s=val;
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_gpib_show_param(struct open_gpib_param *head)
{
	int n;
	char *fmt;
	struct open_gpib_param *i;
	if(NULL == head)
		return 0;
	printf("paramlist: \n");
	for (n=0,i=head;NULL != i; i=i->next,n=n+1){
		fmt=NULL;
		printf("%s=",i->name);
		switch(i->type){
			case OG_PARAM_TYPE_UINT32: printf("%d\n",i->val.u);	break;
			case OG_PARAM_TYPE_INT32:  printf("%d\n",i->val.s); break;
			case OG_PARAM_TYPE_STRING: printf("%s\n",i->val.c);	break;
		}
	}
	return n;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct open_gpib_param *open_gpib_new_param(struct open_gpib_param *head,char *name, char *fmt, ...)
{
	va_list ap;
	struct open_gpib_param *i,*p;
	char buf[3];
	void *ptr=NULL;
	int type=0;
	buf[0]=0;
	if(NULL == name){
		fprintf(stderr,"%s: Failed to supply name, '%c'\n",__func__,OG_PARAM_TYPE_NAME );
		return head;
	}
	if(NULL == fmt){
		fprintf(stderr,"%s: Failed to supply fmt\n",__func__);
		return head;
	}
	va_start(ap, fmt);
/*	while (*fmt){ */
		switch (*fmt) {
			case OG_PARAM_TYPE_UINT32: 
			case OG_PARAM_TYPE_INT32:  
			case OG_PARAM_TYPE_STRING: 
				type=*fmt;
				sprintf(buf,"%c",*fmt);
				ptr=va_arg(ap,void *); 
				break;
			/** case OG_PARAM_TYPE_NAME:   
				name=va_arg(ap, char *);	
				break;*/
			default: 
				fprintf(stderr,"%s: Unknown type '%c'\n",__func__,*fmt);
		}	
/**  		++fmt;
	}*/
	
	va_end(ap);
	
	if(0==type){
		fprintf(stderr,"%s: Failed to supply type\n",__func__);
		return head;
	}
	if(NULL !=head ){/**elements already in list  */
		if(NULL != (p=open_gpib_get_param_ptr(head, name)) ){
			if(NULL != ptr && p->type == type){
				sprintf(buf,"%c",type);
				_open_gpib_set_param(p,buf,ptr);
				return head;
			}
		}
	}		
	
	/**allocate new element  */
	if(NULL == (p=calloc(1,sizeof(struct open_gpib_param)) ) ){
		fprintf(stderr,"%s:out of mem allocating %s\n",__func__,name);
		return NULL;
	}	
	
	if(NULL !=head){
		p->next=head;
	}
	p->name=strdup(name);
	p->type=type;
	_open_gpib_set_param(p,buf,ptr);
	return p;
}
/***************************************************************************/
/** Create and allocate memory for a new parameter.
\n\b Arguments:
type is the type of the paramters, currently:

OG_PARAM_TYPE_UINT32
OG_PARAM_TYPE_INT32
OG_PARAM_TYPE_STRING
name is the name of the parameter.
\n\b Returns:
****************************************************************************/
struct open_gpib_param *open_gpib_new_param_old(struct open_gpib_param *head, int type, char *name, void *val)
{
	struct open_gpib_param *i,*p;
/*	int vsize=sizeof(char *) > sizeof(uint32_t)?sizeof(char *):sizeof(uint32_t); */
	if(NULL == name){
		fprintf(stderr,"%s:NULL name\n",__func__);
		return NULL;
	}
	
	if(NULL !=head ){/**elements already in list  */
		if(NULL != (p=open_gpib_get_param_ptr(head, name)) )
			return NULL;
		for (i=head;NULL != i->next; i=i->next);
	}else 
		i=NULL;
	/**allocate new element  */
	if(NULL == (p=calloc(1,sizeof(struct open_gpib_param)) ) ){
		fprintf(stderr,"%s:out of mem allocating %s\n",__func__,name);
		return NULL;
	}		
	p->name=strdup(name);
	p->type=type;
	switch(type){
		case OG_PARAM_TYPE_UINT32: p->val.u=NULL==val?0:*(uint32_t *)val;	break;
		case OG_PARAM_TYPE_INT32: p->val.s=NULL==val?0:*(int32_t *)val;	break;
		case OG_PARAM_TYPE_STRING: p->val.c=NULL==val?NULL:strdup((char *)val);	break;
		default:
			fprintf(stderr,"%s: Unknown type %c\n",__func__,type);
			free(p);
			return NULL;
	}
	
	if(NULL !=i){
		i->next=p;
		return head;
	}
		
	return p;
}

/***************************************************************************/
/** Just print out all interfaces auto-registered.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_gpib_list_interfaces(void)
{
	int i;
	fprintf(stderr,"listif, %s\n",IF_LIST[0].name);
	for (i=0;NULL != IF_LIST[i].name; ++i){
		fprintf(stderr,"%s, %d, %p\n",IF_LIST[i].name, IF_LIST[i].type, IF_LIST[i].func);
	}
	return 0;
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
	char *tname=NULL;
	if( NULL == name)
		return NULL;
	switch (type){
		case OPEN_GPIB_REG_TYPE_CONTROLLER:
			tname="controller";
			break;
		case OPEN_GPIB_REG_TYPE_TRANSPORT:
			tname="transport";
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
	fprintf(stderr,"Canot find %s interface '%s', type %d\n",tname, name,type);
	return NULL;
}
/***************************************************************************/
/** .
\n\b Arguments:
len is max_len for msg.
\n\b Returns: number of chars read.
****************************************************************************/
int read_raw( struct open_gpib_mstr *open_gpibp)
{
	int i,j;
	i=0;
	while( (j=open_gpibp->ctl->funcs.og_read(open_gpibp->ctl,&open_gpibp->buf[i],open_gpibp->buf_len-i)) >0){
		i+=j;
		usleep(1000);
	}
	/*fprintf(stderr,"Got %d bytes\n'%s'\n",i,open_gpibp->buf); */
	return i;
}


/***************************************************************************/
/** .
\n\b Arguments:
len is max_len for msg.
\n\b Returns: number of chars read.
****************************************************************************/
int read_string( struct open_gpib_mstr *open_gpibp)
{
	int i,j;
	i=0;
	while( (j=open_gpibp->ctl->funcs.og_read(open_gpibp->ctl,&open_gpibp->buf[i],open_gpibp->buf_len-i)) >0){
		i+=j;
		usleep(1000);
	}
	open_gpibp->buf[i]=0;
	/*fprintf(stderr,"Got %d bytes\n'%s'\n",i,open_gpibp->buf);  */
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_string( struct open_gpib_mstr *open_gpibp, char *msg)
{
	int i;
	if(NULL == msg){
		fprintf(stderr,"msg null\n");
		return -1;
	}
	/*fprintf(stderr,"WS:%s",msg); */
	i=strlen(msg);
	return open_gpibp->ctl->funcs.og_write(open_gpibp->ctl,msg,i);
}

/***************************************************************************/
/** Write a string, then print error message.	FIXME malloc xbuf.
\n\b Arguments:
\n\b Returns: 0 on fail or number of chars read on success.
****************************************************************************/
int write_get_data ( struct open_gpib_mstr *open_gpibp, char *cmd)
{
	int i;
	/*fprintf(stderr,"Write... "); */
	write_string(open_gpibp,cmd);
	/*fprintf(stderr,"Read..."); */
	if(-1 == (i=read_string(open_gpibp)) ){
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
int write_wait_for_data( struct open_gpib_mstr *open_gpibp, char *cmd, int sec)
{
	int i,rtn;
	rtn =0;
	write_string(open_gpibp,cmd);
	for (i=0; i<sec*2;++i){
		if((rtn=read_string(open_gpibp)))
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
	if(gi->addr != gi->open_gpibp->addr){
		if(NULL !=gi->open_gpibp->ctl->funcs.og_control){
			if(0 != gi->open_gpibp->ctl->funcs.og_control(gi->open_gpibp->ctl,CTL_SET_ADDR,gi->addr)){
				fprintf(stderr,"Unable to set instrument address %d\n",gi->addr);
				return -1;
			}
			
		}else{
			fprintf(stderr,"Unable to set instrument address (%d!=%d). device control not set\n",gi->addr,gi->open_gpibp->addr);
		}
	}
	return write_string(gi->open_gpibp,cmd);
		
}

/***************************************************************************/
/** .
\n\b Arguments:
name is the name of the interface (see GPIB_TRANSPORT_FUNCTION, GPIB_CONTROLLER_FUNCTION)
type is the type, either OPEN_GPIB_REG_TYPE_CONTROLLER or OPEN_GPIB_REG_TYPE_TRANSPORT
debug is the debug level for this interface
wait is the wait in uS for this interface (inter-character, inter-command, inter-xxx)
dev_path is the path to the device. It can be a device name or IP address.
\n\b Returns: allocated structure on success, NULL on fail
****************************************************************************/
struct open_gpib_dev *find_and_open(char *if_name, int type, uint32_t debug, uint32_t wait, char *dev_path)
{
	open_gpib_register reg_func; 
	struct open_gpib_dev *open_gpibp=NULL;
	
	if(NULL == (reg_func=open_gpib_find_interface(if_name, type)))
		goto err;
	
	if(DBG_TRACE<=debug)fprintf(stderr,"reg %s\n",if_name);
	/**this call allocates the interface's open_gpib_dev  structure
		and allocates the internal structure. It also sets if_name.
		see the macro in _open_gpib.h GPIB_TRANSPORT_FUNCTION
	   */
	if(NULL == (open_gpibp=reg_func()) )
		goto err; 
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s set_debug\n",if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_DEBUG,debug);
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s open\n",if_name);
	if(-1==open_gpibp->funcs.og_open(open_gpibp,dev_path))
		goto err;
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s init\n",if_name);
	if(-1==open_gpibp->funcs.og_init(open_gpibp))
		goto err;
	if(DBG_TRACE<=debug)fprintf(stderr,"%s Ready\n",if_name);
	open_gpibp->dev_path=strdup(dev_path);
	return open_gpibp;
	
err:
	if(DBG_TRACE<=debug)fprintf(stderr,"%s Error on %s\n",__func__,if_name);
	open_gpibp->funcs.og_close(open_gpibp);
	free(open_gpibp);
	return NULL;
}

/***************************************************************************/
/** allocate data and initialize structures.
\n\b Arguments:
debug sets the debug level,
cmd_timeout is in uS (microseconds)
buf_size will set the size of the common buffer for communications.
\n\b Returns:
****************************************************************************/
struct open_gpib_mstr *open_gpib_new(uint32_t debug, uint32_t cmd_timeout, uint32_t buf_size)
{
	struct open_gpib_mstr *open_gpibp;
	
	fprintf(stderr,"OpenGPIB Version %s\n",PACKAGE_VERSION);  
	if(NULL == (open_gpibp=malloc(sizeof( struct open_gpib_mstr)) ) ){
		fprintf(stderr,"Out of mem on gpib alloc\n");
		return NULL;
	}
	memset(open_gpibp,0,sizeof( struct open_gpib_mstr));
	if(0>= buf_size)
		buf_size=8096;
  /**make sure it's a 8-byte multiple, so we stop on largest data boundry  */
  buf_size=((buf_size+7)/8)*8;
	if(NULL == (open_gpibp->buf=malloc(buf_size) ) ){
		fprintf(stderr,"Out of mem on buf size of %d\n",buf_size);
		free(open_gpibp);
		return NULL;
	}
	open_gpibp->buf_len=buf_size;	
	open_gpibp->mdebug=debug;
	open_gpibp->cmd_timeout=cmd_timeout;
	return open_gpibp;
}

/***************************************************************************/													
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct open_gpib_mstr *open_gpib(uint32_t ctype, int gpib_addr, char *dev_path, int buf_size)
{
	struct open_gpib_mstr *open_gpibm;
	uint32_t debug=OPTION_EXTRACT_DEBUG(ctype);
	
	fprintf(stderr,"OpenGPIB Version %s\n",PACKAGE_VERSION);  
	if(NULL == dev_path){
		fprintf(stderr,"Device name is NULL. Must specify device.\n");
		return NULL;
	}
	if(NULL == (open_gpibm=malloc(sizeof( struct open_gpib_mstr)) ) ){
		fprintf(stderr,"Out of mem on gpib alloc\n");
		return NULL;
	}
	memset(open_gpibm,0,sizeof( struct open_gpib_mstr));
	open_gpibm->addr=gpib_addr;
	if(0>= buf_size)
		buf_size=8096;
  /**make sure it's a 8-byte multiple, so we stop on largest data boundry  */
  buf_size=((buf_size+7)/8)*8;
	if(NULL == (open_gpibm->buf=malloc(buf_size) ) ){
		fprintf(stderr,"Out of mem on buf size of %d\n",buf_size);
		free(open_gpibm);
		return NULL;
	}
	open_gpibm->buf_len=buf_size;
	/*fprintf(stderr,"Using %d buf size\n",open_gpibp->buf_len); */
	open_gpibm->type_ctl=ctype;
	/**set up the controller and interface. FIXME Replace with config file.  */
	switch(ctype&CONTROLLER_TYPEMASK){
		case GPIB_CTL_PROLOGIXS:
			if(NULL == (open_gpibm->ctl=find_and_open("prologixs",OPEN_GPIB_REG_TYPE_CONTROLLER, debug, 5000, dev_path)))
				goto err;
			break;
		case GPIB_CTL_HP16500C:
			if(NULL == (open_gpibm->ctl=find_and_open("hp16500cip",OPEN_GPIB_REG_TYPE_CONTROLLER, debug, 5000,dev_path)))
				goto err;
			break;
    case GPIB_CTL_FILEIO:
    	if(NULL == (open_gpibm->ctl=find_and_open("fileio",OPEN_GPIB_REG_TYPE_CONTROLLER, debug, 5000, dev_path)))
				goto err1;
			break;
		default:
			fprintf(stderr,"Unknown controller %d\n",ctype&CONTROLLER_TYPEMASK);
			goto err1;
			break;
	}
	
	return open_gpibm;
err:
	if(NULL != open_gpibm->ctl)
		open_gpibm->ctl->funcs.og_close(open_gpibm->ctl);
err1:
	free(open_gpibm);
	return NULL;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int close_gpib ( struct open_gpib_mstr *open_gpibp)
{
	if(NULL != open_gpibp->ctl->funcs.og_control)
		open_gpibp->ctl->funcs.og_control(open_gpibp->ctl,CTL_CLOSE,0);
	if(NULL != open_gpibp->ctl->funcs.og_close)
		open_gpibp->ctl->funcs.og_close(open_gpibp->ctl);
	
	fprintf(stderr,"Free %s\n",open_gpibp->ctl->dev_path);
	if(NULL != open_gpibp->ctl->dev_path)
		free(open_gpibp->ctl->dev_path);
	fprintf(stderr,"Now freeing ctl\n");
	free(open_gpibp->ctl);
	open_gpibp->ctl=NULL;
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
int init_id( struct open_gpib_mstr *open_gpibp, char *idstr)
{
	open_gpibp->ctl->funcs.og_control(open_gpibp->ctl,CTL_SET_TIMEOUT,500);
	while(read_string(open_gpibp));
	open_gpibp->ctl->funcs.og_control(open_gpibp->ctl,CTL_SET_TIMEOUT,50000);
	/*write_string(open_gpibp,"*CLS"); */
	if(0 == write_get_data(open_gpibp,idstr))
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

/***************************************************************************/
/** Wapper for check_alloc. Just allocate the memory
\n\b Arguments:
size is size in bytes to allocate
\n\b Returns: NULL on error or memory allocated
****************************************************************************/
void *calloc_internal( size_t size, const char *func)
{
	void *p=NULL;
	if(-1 == check_calloc(size,&p,func,NULL))
		return NULL;
	return p;
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
