/** \file ******************************************************************
\n\b File:        open-gpib.h
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

#ifdef _GLOBAL_ALLOC_
	#if __STDC_VERSION__ >= 199901L
		#define _XOPEN_SOURCE 600
		#warning "XOPENSRC 600"#__STDC_VERSION__
	#else
		#warning "XOPENSRC 501"#__STDC_VERSION__
		#define _XOPEN_SOURCE 501
	#endif 
#endif 

#ifdef HAVE_CONFIG_H
	#include <config.h>
	#ifdef HAVE_LIBLA2VCD2
		#include <libla2vcd2.h>
	#endif
#else
	#define PACKAGE_VERSION 0.12-unknown
#endif

/*#include <sys/time.h>*/
#include <stdint.h> /**for types uint32_t  */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> /**getopt, select, usleep  */
#include <time.h> /**timespec  */
#include <errno.h> /* for errno */
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>

#define MAX_CHANNELS 8

#ifdef _GLOBAL_TEK_2440_
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
/**control commands - supplied by interfaces.h
see build_interfaces to understand more.
enum {
	CMD_SET_DEBUG=0,
	CMD_SET_CMD_TIMEOUT,
	CMD_LAST,
};
*/
/**the defines below must correspond to the variable names
in the union 'val' below.  */
#define OG_PARAM_TYPE_UINT32 'u'
#define OG_PARAM_TYPE_INT32  's'
#define OG_PARAM_TYPE_STRING 'c'
#define OG_PARAM_TYPE_NAME   'n'

union param_val {
		uint32_t u;
		int32_t s;
		char *c;
};
/**these are for named parameters in each interface  */
struct open_gpib_param {
	char *name;
	union param_val val;
	int type; 
	struct open_gpib_param *next;
};

#define CONTROLLER_TYPEMASK 0xFF
#define OPTION_DEBUG 0xF00
/**  */
#define OPTION_EXTRACT_DEBUG(x) ((x&OPTION_DEBUG)>>8)
#define OPTION_SET_DEBUG(x) ((x<<8)&OPTION_DEBUG)
/**debug levels after the above operation  */
#define DBG_TRACE 4

#ifndef TOSTRING
	#define STRINGIFY(x) #x
	#define TOSTRING(x) STRINGIFY(x)
#endif

/**Auto-built interfaces.h (see build-interfaces)  */
#define IF_LIST open_gpib_interface_list

#define OPEN_GPIB_REG_TYPE_CONTROLLER 1
#define OPEN_GPIB_REG_TYPE_TRANSPORT  2
/**forward reference these two  */
struct open_gpib_dev;
struct open_gpib_mstr;
/** #define _INTERFACE_DEF_(name,type,register_func)*/
struct open_gpib_functions {
	int (*og_read)(struct open_gpib_dev *dev, void *buf, int len); 	/**Returns: -1 on failure, number of byte read otherwise */
	int (*og_write)(struct open_gpib_dev *dev, void *buf, int len);	/**Returns: -1 on failure, number of bytes written otherwise  */
	int (*og_open)(struct open_gpib_dev *d, char *path);			/** Returns: 1 on failure, 0 on success */
	int (*og_close)(struct open_gpib_dev *d);									/**closes interface  */
	int (*og_control)(struct open_gpib_dev *d, int cmd, uint32_t data); 			/**controller-interface control. Set addr, etc.  */	
	int (*og_init)(struct open_gpib_mstr *d); 									/**Initialize the controller  */		
};

/**structure to talk to transport mechanisms (serial, usb, inet, pci, etc.) and 
   controllers  */
struct open_gpib_dev {  
	struct open_gpib_dev *dev;
	struct open_gpib_functions funcs;
	void *internal;            												/**transport-specific structure  */
	
	int type_if;																	/**set type of interface (see GPIB_IF_*).  */	
	int debug;                                    /**debug level  */
	char *if_name;                                /**name of this interface  */
	char *dev_path;																/**physical device path.  */
	uint32_t wait;                                /**timeout for this interface  */
};
/**structure to talk to the host controller  */
struct open_gpib_mstr {
	struct open_gpib_dev *ctl;										/**controller-specific structure, if any  */	
	int addr; 																		/**instrument address. Check against inst addr  */
	int type_ctl;					 												/**controller type, options at top.  */
	char *buf;														/**GPIB buffer  */
	int buf_len;													/**buffer length  */
	void *inst;                                   /**instrument-specific structure, if any  */
	uint32_t mdebug;							    						/**value to set debug of other layers  */
	uint32_t cmd_timeout;                         /**value to set command timeout, in uS  */ 
};


/**structure to talk to the instrument  */
struct ginstrument {
	struct open_gpib_mstr *open_gpibp;
	int type;
	int addr;																							  /**instrument address.  */
	int (*init)(struct ginstrument *inst);									/**function to call to initialize instrument.  */
};

/**this function is created with the  GPIB_TRANSPORT_FUNCTION and 
   GPIB_CONTROLLER_FUNCTION macro. It allocates the memory for 
   itself and it's internal structure. Most internal structures
   have a struct open_gpib_mstr pointing to the next level.
   
*/
typedef struct open_gpib_dev *(*open_gpib_register)(void ); 	/**Returns: -1 on failure, 0 on success */

/**structure to use for controller and transport registration  */
struct open_gpib_interfaces {
	const char *name;
	int type;
	open_gpib_register func; 	/**Returns: -1 on failure, 0 otherwise */
};

/** Set up our controller/transport functions.  
static int read_if(struct open_gpib_dev *d, void *buf, int len); 	// **Returns: -1 on failure, number of byte read otherwise 
static int write_if(struct open_gpib_dev *d, void *buf, int len);	// **Returns: -1 on failure, number of bytes written otherwise  
static int open_if(struct open_gpib_dev *d, char *path);			// ** Returns: 1 on failure, 0 on success 
static int close_if(struct open_gpib_dev *d);									// **closes interface  
static int init_if( struct open_gpib_mstr *d);									// **initializes interface - usually for the controller setup.  
static int control_if(struct open_gpib_dev *d, int cmd,uint32_t data); 			//controller-interface control. Set addr, etc.  
static void *calloc_internal(void); // **allocate internal structures.  
*/

/** 	
	void **x=p;	\
if( -1 == check_calloc(sizeof(struct transport_dev), p, __func__,NULL) == -1) return -1;\
		fprintf(stderr,"register_func passed %p read= %p\n",d,&d->funcs.og_read);\
	fprintf(stderr,"ctl=%p\n",d->funcs.og_control); \
	*/
/**these macros are read with build-interfaces, and fills in the
   open_gpib_interface_list structure, which is 
   struct open_gpib_interfaces.
   see build_interfaces to understand more
  */

#define GPIB_TRANSPORT_FUNCTION(x) \
struct open_gpib_dev *register_##x(void) {\
	struct open_gpib_dev *d=NULL;\
	if(-1 == check_calloc(sizeof(struct open_gpib_dev), &d,__func__,NULL) ) return NULL;\
	d->funcs.og_read=read_##x;\
	d->funcs.og_write=write_##x;\
	d->funcs.og_open=open_##x;\
	d->funcs.og_close=close_##x;\
	d->funcs.og_control=control_##x;\
	d->funcs.og_init=init_##x;\
	d->dev=calloc_internal_##x();\
	d->if_name=strdup(#x);\
	d->wait=1;\
	printf("rtn from %s\n",__func__);\
	return d; \
}

#define GPIB_CONTROLLER_FUNCTION GPIB_TRANSPORT_FUNCTION
/** #define GPIB_CONTROLLER_FUNCTION (x) \
int register_##x ( void *p) {\
	 struct open_gpib_mstr *d=( struct open_gpib_mstr *)p;\
	d->funcs.og_read=read_ctl;\
	d->funcs.og_write=write_ctl;\
	d->funcs.og_open=open_ctl;\
	d->funcs.og_close=close_ctl;\
	d->funcs.og_control=control_ctl;\
	d->init=init_ctl;\
	d->dev=NULL;\
	return 0;\
}		*/

/**This becomes part of the global command set. Appended to  CMD_SET_, see build-interfaces.*/
#define OPEN_GPIB_ADD_CMD(x)



int open_gpib_list_interfaces(void);
open_gpib_register open_gpib_find_interface(char *name, int type);
int read_raw( struct open_gpib_mstr *g);
int read_string( struct open_gpib_mstr *g);
int write_string( struct open_gpib_mstr *g, char *msg);
int write_get_data ( struct open_gpib_mstr *g, char *cmd);
int write_wait_for_data( struct open_gpib_mstr *g, char *cmd, int sec);
struct open_gpib_mstr *open_gpib(uint32_t ctype, int gpib_addr, char *dev_path,int buf_size); /**opens and inits GPIB interface, interface,and controller  */
int close_gpib ( struct open_gpib_mstr *g);
int gpib_option_to_type(char *op);
void show_gpib_supported_controllers(void);
int init_id( struct open_gpib_mstr *g, char *idstr);
int check_calloc(size_t size, void *p, const char *func, void *set);
void *calloc_internal( size_t size, const char *func);

int is_string_number(char *s);
int og_next_col(struct c_opts *o);
int og_get_col(struct c_opts *o, float *f);
double format_eng_units(double val, int *m);
double og_get_value( char *f, char *buf);
double og_get_value_col( int col, char *buf);
char * og_get_string( char *f, char *buf);
char * og_get_string_col( int col, char *buf); /**make sure resulting char * is has free() called on it  */
void _usleep(int usec);
/*it is critial the line below stays here - it is used by build-interfaces */

/**in open-gpib-param.c  */
int open_gpib_set_param(struct open_gpib_param *ptr,char *name, char *fmt, ...);
int open_gpib_set_uint32_t(struct open_gpib_param *,char *name,uint32_t val);
int open_gpib_set_int32_t(struct open_gpib_param *,char *name,int32_t val);
int open_gpib_set_string(struct open_gpib_param *,char *name,char *str);
char *open_gpib_get_string(struct open_gpib_param *head,char *name);
uint32_t open_gpib_get_uint32_t(struct open_gpib_param *head,char *name);
int open_gpib_show_param(struct open_gpib_param *head);
int32_t open_gpib_get_int32_t(struct open_gpib_param *head,char *name);
struct open_gpib_param *open_gpib_new_param(struct open_gpib_param *head,char *name, char *fmt, ...);
struct open_gpib_param *open_gpib_new_param_old(struct open_gpib_param *head, int type, char *name, void *val);
#endif /*REMOVE_ME/

