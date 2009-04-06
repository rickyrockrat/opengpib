/** \file ******************************************************************
\n\b File:        gpib.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.2  2008/10/06 12:45:23  dfs
Added write_get_data, auto to init_prologix

Revision 1.1  2008/10/06 07:54:16  dfs
moved from get_tek_waveform

Interface defines for various interfaces. An interface
is the physical access device for control.  Proligix uses serial and 
ethernet. There are also ISA, USB, PCI, and presumably firewire cards for 
other controllers.

*/
#ifndef _GPIB_H_
#define _GPIB_H_ 1

#include <sys/time.h>
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
	CTL_NONE,
};


struct gpib {
	int addr; 																		/**instrument address  */
	int type_if;																	/**set type of interface (see GPIB_IF_*).  */	
	int type_ctl;					 												/**controller type  */
	void *ctl;																		/**controller-specific structure, if any  */
	void *dev;            												/**interface-specific structure  */
	int (*read)(void *dev, void *buf, int len); 	/**Returns: -1 on failure, number of byte read otherwise */
	int (*write)(void *dev, void *buf, int len);	/**Returns: -1 on failure, number of bytes written otherwise  */
	int (*open)(struct gpib *g, char *path);			/** Returns: 1 on failure, 0 on success */
	int (*close)(struct gpib *g);									/**closes interface  */
	int (*control)(struct gpib *g, int cmd); 			/**controller-interface control. Set addr, etc.  */
	char buf[BUF_SIZE];														/**GPIB buffer  */
	int buf_len;													/**buffer length  */
};


int read_string(struct gpib *g);
int write_string(struct gpib *g, char *msg);
int write_get_data (struct gpib *g, char *cmd);
struct gpib *open_gpib(int ctype, int addr, char *dev_path); /**opens and inits GPIB interface, interface,and controller  */
int close_gpib (struct gpib *g);
#endif
