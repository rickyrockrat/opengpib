/** \file ******************************************************************
\n\b File:        serial.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        08/01/2008  4:57 pm
\n\b Description: Interface description file. It should just contain 
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
Revision 1.4  2009-04-07 18:15:06  dfs
Changed control proto, added SERIAL_CMD enum

Revision 1.3  2009-04-07 07:05:42  dfs
Added serial interface functions

Revision 1.2  2009-04-06 20:57:26  dfs
Major re-write for new gpib API

Revision 1.1  2008/08/02 08:53:58  dfs
Initial working rev

*/
#ifndef _SERIAL_H_
#define _SERIAL_H_ 1
struct serial_dev {
	int (*read)(struct serial_dev *d, void *buf, int len); 	/**Returns: -1 on failure, number of byte read otherwise */
	int (*write)(struct serial_dev *d, void *buf, int len);	/**Returns: -1 on failure, number of bytes written otherwise  */
	int (*open)(struct serial_dev *d, char *path);			/** Returns: 1 on failure, 0 on success */
	int (*close)(struct serial_dev *d);									/**closes interface  */
	int (*control)(struct serial_dev *d, int cmd,int data); 			/**controller-interface control. Set addr, etc.  */
	void *dev;            												/**interface-specific structure  */
	int type_if;																	/**set type of interface (see GPIB_IF_*).  */	
	int debug;
};

enum {
	SERIAL_CMD_SET_CHAR_TIMEOUT=0,
	SERIAL_CMD_SET_DEBUG,
	SERIAL_CMD_NONE,
};
int serial_register(struct serial_dev *d);
#endif

