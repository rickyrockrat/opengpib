/** \file ******************************************************************
\n\b File:        open-gpib-ip.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/14/2013  2:18 pm
\n\b Description: Interface description file. It should just contain  minimal
functions of read, write, open, close, and control, and any defs specific to
this interface. It must also contain the ip_register function that fills
in the above function pointers.
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
#ifndef _IP_H_
#define _IP_H_ 1

struct ip_dev {
	int (*read)(struct ip_dev *d, void *buf, int len); 	/**Returns: -1 on failure, number of byte read otherwise */
	int (*write)(struct ip_dev *d, void *buf, int len);	/**Returns: -1 on failure, number of bytes written otherwise  */
	int (*open)(struct ip_dev *d, char *path);			/** Returns: 1 on failure, 0 on success */
	int (*close)(struct ip_dev *d);									/**closes interface  */
	int (*control)(struct ip_dev *d, int cmd,uint32_t data); 			/**controller-interface control. Set addr, etc.  */
	void *dev;            												/**interface-specific structure  */
	int type_if;																	/**set type of interface (see GPIB_IF_*).  */	
	int debug;
};

enum {
	IP_CMD_SET_CMD_TIMEOUT=0,
	IP_CMD_SET_DEBUG,
	IP_CMD_SET_PORT,		
	IP_CMD_NONE,
};
int ip_register(struct ip_dev *d);
#endif

