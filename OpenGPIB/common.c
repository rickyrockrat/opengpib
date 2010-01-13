/** \file ******************************************************************
\n\b File:        common.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/02/2008 11:48 pm
\n\b Description: Common functions.
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

#include "common.h"
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int next_col(struct c_opts *o)
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
int get_col(struct c_opts *o, float *f)
{
	int i;
	if(o->col >1){
		for (i=1;i<o->col;++i)	{
			if(next_col(o))
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
double get_value( char *f, char *buf)
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
char * get_string( char *f, char *buf)
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
char * get_string_col( int col, char *buf)
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
		return "NOTFOUND";
	tbuf[i]=0;
	/*printf("Found %d->%s",col,tbuf);   */
	return(strdup(tbuf));	 
}
/***************************************************************************/
/** Get the value at the column. Count commas.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
double get_value_col( int col, char *buf)
{
	double x; 
	char *v;
	
	v=get_string_col(col,buf);
	
	if(!strcmp("NOTFOUND",v) ){ 
		x=-1;
	}else
		x=strtof(v, NULL);
	
	/*printf(" : %f\n",x); */
	free(v);
	return(x );	 
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void _usleep(int usec)
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
