/** \file ******************************************************************
\n\b File:        common.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/02/2008 11:49 pm
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.1  2008/08/03 06:18:42  dfs
Moved functions from tek2gplot.c

*/
#ifndef _COMMON_H_
#define _COMMON_H_ 1
#ifdef _COMMON_
#define _XOPEN_SOURCE 501
#endif 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
double get_value( char *f, char *buf);
char * get_string( char *f, char *buf);
#endif 

