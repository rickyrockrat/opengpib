/** \file ******************************************************************
\n\b File:        common.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/02/2008 11:49 pm
\n\b Description: Common header file for OpenGPIB.
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
#ifndef _COMMON_H_
#define _COMMON_H_ 1
#ifdef _COMMON_
#define _XOPEN_SOURCE 501
#endif 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> /**getopt  */
#include <time.h> /**timespec  */
#include <stdint.h> // int types are defined here
#include <sys/types.h>
typedef  int8_t   int8;
typedef uint8_t  uint8;
typedef  int16_t  int16;
typedef uint16_t uint16;
typedef  int32_t  int32;
typedef uint32_t uint32;
typedef  int64_t  int64;
typedef uint64_t uint64;



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
void _usleep(int usec);
#endif 

