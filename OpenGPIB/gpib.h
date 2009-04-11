/** \file ******************************************************************
\n\b File:        gpib.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: Header for common gpib functions and structures.
*/ /************************************************************************
Change Log: \n
 This file is part of OpenGPIB.

    OpenGPIB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenGPIB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenGPIB.  If not, see <http://www.gnu.org/licenses/>.
    
		The License should be in the file called COPYING.


Interface defines for various interfaces. An interface
is the physical access device for control.  Proligix uses serial and 
ethernet. There are also ISA, USB, PCI, and presumably firewire cards for 
other controllers.

*/
#ifndef _GPIB_H_
#define _GPIB_H_ 1

#include <sys/time.h>
#include <unistd.h> /**for usleep  */

#define BUF_SIZE 8096
/**interface types  */
enum {
	GPIB_IF_SERIAL=0,
	GPIB_IF_NONE,
};
/**controller types  */
enum {
	GPIB_CTL_PROLOGIXS=0,
	GPIB_CTL_NONE,
};
/**controller commands  */
enum {
	CTL_OPEN=0,
	CTL_CLOSE,
	CTL_SET_ADDR,
	CTL_SET_TIMEOUT,
	CTL_NONE,
};


struct gpib {
	int addr; 																		/**instrument address  */
	void *ctl;																		/**controller-specific structure, if any  */
	int type_ctl;					 												/**controller type  */
	int (*read)(void *dev, void *buf, int len); 	/**Returns: -1 on failure, number of byte read otherwise */
	int (*write)(void *dev, void *buf, int len);	/**Returns: -1 on failure, number of bytes written otherwise  */
	int (*open)(struct gpib *g, char *path);			/** Returns: 1 on failure, 0 on success */
	int (*close)(struct gpib *g);									/**closes interface  */
	int (*control)(struct gpib *g, int cmd, int data); 			/**controller-interface control. Set addr, etc.  */
	char buf[BUF_SIZE];														/**GPIB buffer  */
	int buf_len;													/**buffer length  */
};


int read_string(struct gpib *g);
int write_string(struct gpib *g, char *msg);
int write_get_data (struct gpib *g, char *cmd);
struct gpib *open_gpib(int ctype, int addr, char *dev_path); /**opens and inits GPIB interface, interface,and controller  */
int close_gpib (struct gpib *g);
#endif
