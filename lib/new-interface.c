/** \file ******************************************************************
\n\b File:        new-interface.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/22/2013 
\n\b Description: New driver for Open GPIB.
Instructions for adding a new driver.
1) Copy this file to new file name in lib.
2) search and replace new_interface with whatever your interface name is.
	it must be a unique name (grep -r FUNCTION). The macro at the bottom sets
	your function names.
3) Implement the details of your function. Your private data resides in internal.
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

Change Log: \n
*/
#include "open-gpib.h"
struct new_interface_ctl {
	int my_internal_data;
	char *name;
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1  on failure, 0 on success
****************************************************************************/
static int open_( struct open_gpib_dev *ctl, char *name)
{
	struct new_interface_ctl *c;
	if(NULL == ctl){
		fprintf(stderr,"%s: dev null\n",__func__);
		return 1;
	}
	c=(struct new_interface_ctl *)ctl->internal;
	if( -1 ==check_calloc(sizeof(struct new_interface_ctl), &c, __func__,NULL) == -1) return -1;
	c->name=strdup(name);
	return 0;
	
err:
	if(NULL != c){
		free(c->name);
		free(c);
	}
	ctl->internal=NULL;
	return -1;
}

/***************************************************************************/
/** just save the command.
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
static int write_new_interface(struct open_gpib_dev *ctl, void *buf, int len)
{
	struct new_interface_ctl *c;
	
	c=(struct new_interface_ctl *)ctl->internal;
  if(NULL == buf)
    return -1;
  return len;  
}

/***************************************************************************/
/** Guess which instrument we are supposed to be by looking at the command 
bytes, then do what we should..
\n\b Arguments:
\n\b Returns: -1 on failure, number of byte read otherwise
****************************************************************************/
static int read_new_interface(struct open_gpib_dev *ctl, void *buf, int len)
{
	struct new_interface_ctl *c;
	int i=0;
	char *m;
  
	m=(char *)buf;
	c=(struct new_interface_ctl *)ctl->internal;
  if(NULL == buf )
    return -1;
	i=len;
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int control_new_interface( struct open_gpib_dev *ctl, int cmd, uint32_t data)
{
	struct new_interface_ctl *c; 

	if(NULL == ctl){
		fprintf(stderr,"%s: gpib null\n",__func__);
		return 0;
	}
	c=(struct new_interface_ctl *)ctl->internal;
	/**sets g->ctl if it allocates new memory  */
	if( -1 ==check_calloc(sizeof(struct new_interface_ctl), &c, __func__,&ctl->internal) == -1) return -1;
		
	switch(cmd){
		case CTL_CLOSE:
			if(ctl->debug) fprintf(stderr,"Closing new_interface\n");
			break;
		case CTL_SET_TIMEOUT:
			break;
		case CTL_SET_ADDR: /**send command, check result, then set gpib addr.  */
			break;
		case CTL_SET_DEBUG:
			c->debug=data;
			break;
		case CTL_SEND_CLR:
			break;
		default:
			fprintf(stderr,"Unsupported cmd '%d'\n",cmd);
			break;
	}
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static int close_new_interface(struct open_gpib_dev *ctl)
{
	struct new_interface_ctl *c;
	if(NULL == ctl)
		return 0;
	c=(struct new_interface_ctl *)ctl->internal;
	if(NULL != c){
		if(NULL != c->name)
			free(c->name);
		free(c);
	}
	ctl->internal=NULL;
	return 0;
}

static int init_new_interface(struct open_gpib_mstr *g)
{
	
}
/***************************************************************************/
/** Allocate our internal data structure.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void *calloc_internal_new_interface(void)
{
	return calloc_internal(sizeof(struct new_interface_ctl),__func__);
}

GPIB_CONTROLLER_FUNCTION(new_interface)


