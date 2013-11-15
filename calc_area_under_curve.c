/** \file ******************************************************************
\n\b File:        calc_area_under_curve.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        04/04/2009 11:04 am
\n\b Description: Calucalte area under curve, mostly for gnu plot data files.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common.h"


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void usage( void)
{
	printf("calc_area_under_curve <options>\n"
	" -c col select column col for operation. (2)\n"
	" -d dlm use delimiter dlm for column spacing (space)\n"
	" -i fname unput file name. - is stdin. This is required.\n"
	" -m dx/mx set the divider or multiplier for result (1/1).\n"
	" The file data is assumed to be one or more columns of floating \n"
	" point numbers.  Use -d to specify the single-character delimiter\n"
	" between columns.\n"
	
	
	"");
}



/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int calc_area_under_curve(struct c_opts *o)
{
	float c,off,sum,ave,last;
	int count;
	off=99999;
	sum=0;
	count=0;
	while( 0 == get_col(o,&c)){
		++count;
		if(count>1){
			ave=(last+c)/2;
			if(ave>0){
				sum+=ave;
			}	else
				sum+=(0-ave);
		}
			
		if(off>c)
			off=c;
		last=c;
	}
	sum=sum/count;
	
	sum*=o->mul;
	off*=o->mul;
	sum/=o->div;
	off/=o->div;
	
	c=sum-(off);
	
	
	printf("Area under curve (%d values) is %f without DC offset of %f it is %f\n",
		count,sum,off,c);
	return 0;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int main(int argc, char *argv[])
{
	struct c_opts opts;
	int c;
	char *fname;
	opts.dlm=' ';
	opts.col=2;
	opts.fd=NULL;
	opts.div=opts.mul=1;
	if(argc <2){
		usage();
		return 1;
	}
	while( -1 != (c = getopt(argc, argv, "c:d:i:m:")) ) {
		switch(c){
			case 'c':
				opts.col=atoi(optarg);
				if(opts.col<1){
					printf("Invalid column %d\n",opts.col);
					return 1;
				}
				break;
			case 'd':
				opts.dlm=optarg[0];
				break;
			case 'i':
				if(!strcmp("-",optarg))
					opts.fd=fdopen(0,"r");
				fname=strdup(optarg);
				break;
			case 'm':
				if('d'==optarg[0]){
					sscanf(&optarg[1],"%f",&opts.div);
					if(0 == opts.div){
						printf("Refusing to divide by zero\n");
						return 1;
					}
				}
					
				if( 'm'==optarg[0]){
					sscanf(&optarg[1],"%f",&opts.mul);
					
				}
					
				break;
		}
	}
	if(NULL == opts.fd){
		if(NULL ==(opts.fd=fopen(fname,"r")) ){
			printf("Unable to open %s\n",fname);
			return 1;
		}
	}
	return calc_area_under_curve(&opts);
}
