/** \file ******************************************************************
\n\b File:        open-gpib-param.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/25/2013  6:58 pm
\n\b Description: 
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
*/
#include <open-gpib.h>
/***************************************************************************/
/** .
\n\b Arguments:
ptr is the single element, fmt contains u,s, or c.
dynamically allocates/deallocates strings.
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
		case OG_PARAM_TYPE_STRING:  
			if(NULL != ptr->val.c)
				free(ptr->val.c);
			ptr->val.c=strdup(va_arg(ap, char *));	
			break;
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
	fprintf(stderr,"%s: Unable to find parameter '%s'\n",__func__,name);
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
uint32_t open_gpib_get_uint32_t(struct open_gpib_param *head,char *name)
{
	struct open_gpib_param *p=open_gpib_get_param_ptr(head,name);
	if(NULL == p)
		return 1;
	return p->val.u;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
char *open_gpib_get_string(struct open_gpib_param *head,char *name)
{
	struct open_gpib_param *p=open_gpib_get_param_ptr(head,name);
	if(NULL == p)
		return NULL;
	
	return p->val.c;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int32_t open_gpib_get_int32_t(struct open_gpib_param *head,char *name)
{
	struct open_gpib_param *p=open_gpib_get_param_ptr(head,name);
	if(NULL == p)
		return 1;
	return p->val.s;
}
/***************************************************************************/
/** Print the argument names and their values.
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
\n\b Returns: new head of list
****************************************************************************/
struct open_gpib_param *open_gpib_del_param(struct open_gpib_param *head,char *name)
{
	struct open_gpib_param *i, *last;
	if(NULL == name){
		fprintf(stderr,"%s: Failed to supply name, '%c'\n",__func__,OG_PARAM_TYPE_NAME );
		return head;
	}
	
	if(NULL == head || NULL == name)
		return NULL;
	for (last=i=head;NULL != i; i=i->next){
		if(!strcmp(name,i->name)){
			if(OG_PARAM_TYPE_STRING==i->type && NULL != i->val.c)
				free(i->val.c);
			if(head == i){/**use last as return value.  */
				last=i->next;
			}else{
				last->next=i->next;
				last=head;
			}
			free(i);
			return last;
		}
		last=i;	
	}	
	fprintf(stderr,"%s: Unable to find parameter '%s'\n",__func__,name);
	return NULL;
}
/***************************************************************************/
/** .
\n\b Arguments:
head is the top of the list. 
name is the name of the parameter.

fmt is the type of the paramters, currently:

OG_PARAM_TYPE_UINT32
OG_PARAM_TYPE_INT32
OG_PARAM_TYPE_STRING

\n\b Returns: New top of the list
****************************************************************************/
struct open_gpib_param *open_gpib_new_param(struct open_gpib_param *head,char *name, char *fmt, ...)
{
	va_list ap;
	struct open_gpib_param *p;
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

