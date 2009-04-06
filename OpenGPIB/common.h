/** \file ******************************************************************
\n\b File:        common.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/02/2008 11:49 pm
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.6  2009-04-04 18:09:20  dfs
Added get_col,next_col,c_opts struct

Revision 1.5  2009-03-18 22:52:06  dfs
Added format_eng_units

Revision 1.4  2008/10/06 12:42:58  dfs
Added get_string_col, get_value_col

Revision 1.3  2008/08/19 06:42:18  dfs
Moved def of CH_LIST to common.h

Revision 1.2  2008/08/18 21:14:34  dfs
Added _XOPEN_SOURCE for strdup with C99

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
#include <unistd.h> /**getopt  */

#ifndef _COMMON_
#define MAX_CHANNELS 7
/**We use the below for figuring out which channel the target is on.  */
#define CURSORS      6 /**offset into CH_LIST where cursors keyword is */
char *CH_LIST[MAX_CHANNELS]=\
{
	"ch1",
	"ch2",
	"ref1",
	"ref2",
	"ref3",
	"ref4",
	"cursors", /**make sure all REAL channels go before this.  */
};

#endif
struct c_opts {
	FILE *fd;
	int col;
	int dlm;
	float div;
	float mul;
};
int next_col(struct c_opts *o);
int get_col(struct c_opts *o, float *f);
double format_eng_units(double val, int *m);
double get_value( char *f, char *buf);
double get_value_col( int col, char *buf);
char * get_string( char *f, char *buf);
char * get_string_col( int col, char *buf);
#endif 

