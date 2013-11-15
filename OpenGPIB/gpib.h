/** \file ******************************************************************
\n\b File:        gpib.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: Header for common gpib functions and structures.
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


Interface defines for various interfaces. An interface
is the physical access device for control.  Proligix uses serial and 
ethernet. There are also ISA, USB, PCI, and presumably firewire cards for 
other controllers.

*/
#ifndef _GPIB_H_
#define _GPIB_H_ 1

#include <sys/time.h>
#include <unistd.h> /**for usleep  */
#include <stdint.h> /**for types uint32_t  */

/**interface types  */
enum {
	GPIB_IF_SERIAL=0,
	GPIB_IF_IP,
	GPIB_IF_NONE,
};
/**controller types  */
enum {
	GPIB_CTL_PROLOGIXS=0,
	GPIB_CTL_HP16500C,
  GPIB_CTL_FILEIO,
	GPIB_CTL_NONE,
};
/**controller commands  */
enum {
	CTL_OPEN=0,
	CTL_CLOSE,
	CTL_SET_ADDR,
	CTL_SET_TIMEOUT,
	CTL_SET_DEBUG,
	CTL_SEND_CLR,
	CTL_NONE,
};

#define CONTROLLER_TYPEMASK 0xFF
#define OPTION_DEBUG 0x100
#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif
/**structure to talk to the host controller  */
struct gpib {
	int addr; 																		/**instrument address. Check against inst addr  */
	void *ctl;																		/**controller-specific structure, if any  */
	int type_ctl;					 												/**controller type, options at top.  */
	int (*read)(void *dev, void *buf, int len); 	/**Returns: -1 on failure, number of byte read otherwise */
	int (*write)(void *dev, void *buf, int len);	/**Returns: -1 on failure, number of bytes written otherwise  */
	int (*open)(struct gpib *g, char *path);			/** Returns: 1 on failure, 0 on success */
	int (*close)(struct gpib *g);									/**closes interface  */
	int (*control)(struct gpib *g, int cmd, uint32_t data); 			/**controller-interface control. Set addr, etc.  */
	char *buf;														/**GPIB buffer  */
	int buf_len;													/**buffer length  */
	char *dev_path;																/**physical device path.  */
	void *inst;                                   /**instrument-specific structure, if any  */
};

/**structure to talk to the instrument  */
struct ginstrument {
	struct gpib *g;
	int type;
	int addr;																							  /**instrument address.  */
	int (*init)(struct ginstrument *inst);									/**function to call to initialize instrument.  */
};

int read_raw(struct gpib *g);
int read_string(struct gpib *g);
int write_string(struct gpib *g, char *msg);
int write_get_data (struct gpib *g, char *cmd);
int write_wait_for_data(struct gpib *g, char *cmd, int sec);
struct gpib *open_gpib(int ctype, int addr, char *dev_path,int buf_size); /**opens and inits GPIB interface, interface,and controller  */
int close_gpib (struct gpib *g);
int gpib_option_to_type(char *op);
void show_gpib_supported_controllers(void);
int init_id(struct gpib *g, char *idstr);
#endif

