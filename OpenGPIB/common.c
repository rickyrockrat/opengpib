/** \file ******************************************************************
\n\b File:        common.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/02/2008 11:48 pm
\n\b Description: Common functions.
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.2  2008/08/18 21:14:54  dfs
Fixed bug that truncated read

Revision 1.1  2008/08/03 06:18:42  dfs
Moved functions from tek2gplot.c

*/

#include "common.h"
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
	/*printf("Found %s->%s",f,tbuf);  */
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
/** Get the string at column number
\n\b Arguments:
\n\b Returns:
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
