/** \file ******************************************************************
\n\b File:        gpib.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: common gpib functions...
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
#include <open-gpib.h>

struct supported_dev {
	int type;
	char *option;
	char *name;
};
static struct supported_dev s_dev[]={\
	{GPIB_CTL_PROLOGIXS,"prologixs","Prologix Serial GPIB Controller"},														
	{GPIB_CTL_HP16500C,"hpip","HP 16500C LAN Controller"},
  {GPIB_CTL_FILEIO,"file","File Reader"},
	{-1,NULL,NULL},
};

struct open_gpib_dev *find_and_register_if(const char *if_name, int type, uint32_t debug,char *buf, int blen);

/***************************************************************************/
/** Just print out all interfaces auto-registered.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_gpib_list_interfaces(void)
{
	int i;
	char buf[100];
	
	struct open_gpib_dev *d;
	fprintf(stderr,"Open GPIB List Interfaces\n         'Name', type\n");
	
	for (i=0;NULL != IF_LIST[i].name; ++i){
		fprintf(stderr,"Interface '%s', %d\n",IF_LIST[i].name, IF_LIST[i].type);
		if(NULL != (d=find_and_register_if(IF_LIST[i].name, IF_LIST[i].type, 0,buf, 90))) {
			if(NULL != d->params) {
				open_gpib_show_param(d->params);
				open_gpib_param_free(d->params);
			}
			d->funcs.og_close(d);
		}
	}
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments: 
name is the name of the interface, like ip/usb/serial.
type is either controller or transport
                
\n\b Returns:	function found or null;
****************************************************************************/
open_gpib_register open_gpib_find_interface(const char *name, int type)
{
	int i;
	char *tname=NULL;
	if( NULL == name)
		return NULL;
	switch (type){
		case OPEN_GPIB_REG_TYPE_CONTROLLER:
			tname="controller";
			break;
		case OPEN_GPIB_REG_TYPE_TRANSPORT:
			tname="transport";
			break;
		default:
			fprintf(stderr,"Unknown type %d\n",type);
			return NULL;
	}
	for (i=0;NULL != IF_LIST[i].name; ++i){
		if(!strcmp(name,IF_LIST[i].name) &&	type == IF_LIST[i].type) {
			/*fprintf(stderr,"Found '%s', type %d, @%p\n",IF_LIST[i].name, IF_LIST[i].type, IF_LIST[i].func); */
			return IF_LIST[i].func;		
		}
	}
	fprintf(stderr,"Canot find %s interface '%s', type %d\n",tname, name,type);
	return NULL;
}

/***************************************************************************/
/** Finds the interface, then calls the register function which should
allocate all the data structures. Set our buffer and length and return.
\n\b Arguments:
\n\b Returns: Structure or NULL on error.
****************************************************************************/
struct open_gpib_dev *find_and_register_if(const char *if_name, int type, uint32_t debug,char *buf, int blen)
{
	open_gpib_register reg_func; 
	struct open_gpib_dev *open_gpibp=NULL;
	
	if(NULL == (reg_func=open_gpib_find_interface(if_name, type)))
		return NULL;
	
	if(DBG_TRACE<=debug)fprintf(stderr,"reg %s\n",if_name);
	/**this call allocates the interface's open_gpib_dev  structure
		and allocates the internal structure. It also sets if_name.
		see the macro in _open_gpib.h GPIB_TRANSPORT_FUNCTION
	   */
	if(NULL == (open_gpibp=reg_func()) )
		return NULL; 
	open_gpibp->buf=buf;
	open_gpibp->buf_len=blen;	
	open_gpibp->debug=debug;
	return open_gpibp;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: 1 on error, 0 on success.
****************************************************************************/
int setup_interface (struct open_gpib_dev *open_gpibp, uint32_t wait, uint32_t addr, char *dev_path)
{
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"Call %s set_debug\n",open_gpibp->if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_DEBUG,open_gpibp->debug);
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"Call %s set timeout\n",open_gpibp->if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_TIMEOUT,wait);
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"Call %s set addr\n",open_gpibp->if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_ADDR,addr);
	
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"Call %s open\n",open_gpibp->if_name);
	if(-1==open_gpibp->funcs.og_open(open_gpibp,dev_path))
		goto err;
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"Call %s init\n",open_gpibp->if_name);
	if(-1==open_gpibp->funcs.og_init(open_gpibp))
		goto err;
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"%s Ready\n",open_gpibp->if_name);
	open_gpibp->dev_path=strdup(dev_path);
	return 0;
err:
	if(DBG_TRACE<=open_gpibp->debug)fprintf(stderr,"%s Error on %s\n",__func__,open_gpibp->if_name);
	open_gpibp->funcs.og_close(open_gpibp);
	free(open_gpibp);
	return 1;
}
/***************************************************************************/
/** .
\n\b Arguments:
name is the name of the interface (see GPIB_TRANSPORT_FUNCTION, GPIB_CONTROLLER_FUNCTION)
type is the type, either OPEN_GPIB_REG_TYPE_CONTROLLER or OPEN_GPIB_REG_TYPE_TRANSPORT
debug is the debug level for this interface
wait is the wait in uS for this interface (inter-character, inter-command, inter-xxx)
dev_path is the path to the device. It can be a device name or IP address.
\n\b Returns: allocated structure on success, NULL on fail
****************************************************************************/
struct open_gpib_dev *find_and_open(char *if_name, int type, uint32_t debug, uint32_t wait, uint32_t addr, char *dev_path, char *buf, int blen)
{
  struct open_gpib_dev *open_gpibp=NULL;
	if(NULL ==(open_gpibp=find_and_register_if(if_name,type,debug, buf,blen)) )
		return NULL;
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s set_debug\n",if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_DEBUG,debug);
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s set timeout\n",if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_TIMEOUT,wait);
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s set addr\n",if_name);
	open_gpibp->funcs.og_control(open_gpibp,CTL_SET_ADDR,addr);
	
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s open\n",if_name);
	if(-1==open_gpibp->funcs.og_open(open_gpibp,dev_path))
		goto err;
	if(DBG_TRACE<=debug)fprintf(stderr,"Call %s init\n",if_name);
	if(-1==open_gpibp->funcs.og_init(open_gpibp))
		goto err;
	if(DBG_TRACE<=debug)fprintf(stderr,"%s Ready\n",if_name);
	open_gpibp->dev_path=strdup(dev_path);
	return open_gpibp;
	
err:
	if(DBG_TRACE<=debug)fprintf(stderr,"%s Error on %s\n",__func__,if_name);
	open_gpibp->funcs.og_close(open_gpibp);
	free(open_gpibp);
	return NULL;
}

/***************************************************************************/
/** allocate data and initialize structures.
\n\b Arguments:
debug sets the debug level,
cmd_timeout is in uS (microseconds)
buf_size will set the size of the common buffer for communications.
\n\b Returns:
****************************************************************************/
struct open_gpib_mstr *open_gpib_new(uint32_t debug, uint32_t cmd_timeout, uint32_t buf_size)
{
	struct open_gpib_mstr *open_gpibp;
	
	fprintf(stderr,"OpenGPIB Version %s\n",PACKAGE_VERSION);  
	if(NULL == (open_gpibp=calloc(1,sizeof( struct open_gpib_mstr)) ) ){
		fprintf(stderr,"Out of mem on gpib alloc\n");
		return NULL;
	}
	if(0>= buf_size)
		buf_size=8096;
  /**make sure it's a 8-byte multiple, so we stop on largest data boundry  */
  buf_size=((buf_size+7)/8)*8;
	if(NULL == (open_gpibp->buf=malloc(buf_size) ) ){
		fprintf(stderr,"Out of mem on buf size of %d\n",buf_size);
		free(open_gpibp);
		return NULL;
	}
	open_gpibp->buf_len=buf_size;	
	open_gpibp->mdebug=debug;
	open_gpibp->cmd_timeout=cmd_timeout;
	return open_gpibp;
}

/***************************************************************************/													
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct open_gpib_mstr *open_gpib(uint32_t ctype, int gpib_addr, char *dev_path, int buf_size)
{
	struct open_gpib_mstr *open_gpibm;
	char *buf;
	uint32_t debug=OPTION_EXTRACT_DEBUG(ctype);
	
	fprintf(stderr,"OpenGPIB Version %s\n",PACKAGE_VERSION);  
	if(NULL == dev_path){
		fprintf(stderr,"Device name is NULL. Must specify device.\n");
		return NULL;
	}
	if(NULL == (open_gpibm=malloc(sizeof( struct open_gpib_mstr)) ) ){
		fprintf(stderr,"Out of mem on gpib alloc\n");
		return NULL;
	}
	memset(open_gpibm,0,sizeof( struct open_gpib_mstr));
	open_gpibm->addr=gpib_addr;
	if(0>= buf_size)
		buf_size=8096;
  /**make sure it's a 8-byte multiple, so we stop on largest data boundry  */
  buf_size=((buf_size+7)/8)*8;
	if(NULL == (buf=malloc(buf_size) ) ){
		fprintf(stderr,"Out of mem on buf size of %d\n",buf_size);
		free(open_gpibm);
		return NULL;
	}
	open_gpibm->buf=buf;
	open_gpibm->buf_len=buf_size;
	/*fprintf(stderr,"Using %d buf size\n",open_gpibp->buf_len); */
	open_gpibm->type_ctl=ctype;
	/**set up the controller and interface. FIXME Replace with config file.  */
	switch(ctype&CONTROLLER_TYPEMASK){
		case GPIB_CTL_PROLOGIXS:
			if(NULL == (open_gpibm->ctl=find_and_register_if("prologixs",OPEN_GPIB_REG_TYPE_CONTROLLER,debug,buf,buf_size)) )
				goto err;
			if(NULL == (open_gpibm->ctl->dev=find_and_register_if("serial",OPEN_GPIB_REG_TYPE_TRANSPORT,debug,buf,buf_size)))
				goto err;
			if(setup_interface(open_gpibm->ctl,5000, gpib_addr,dev_path))
				return NULL;
			break;
		case GPIB_CTL_HP16500C:
			if(NULL == (open_gpibm->ctl=find_and_open("hp16500cip",OPEN_GPIB_REG_TYPE_CONTROLLER, debug, 5000, gpib_addr,dev_path,open_gpibm->buf,buf_size)))
				goto err;
			break;
    case GPIB_CTL_FILEIO:
    	if(NULL == (open_gpibm->ctl=find_and_open("fileio",OPEN_GPIB_REG_TYPE_CONTROLLER, debug, 5000, gpib_addr,dev_path,open_gpibm->buf,buf_size)))
				goto err1;
			break;
		default:
			fprintf(stderr,"Unknown controller %d\n",ctype&CONTROLLER_TYPEMASK);
			goto err1;
			break;
	}
	
	return open_gpibm;
err:
	if(NULL != open_gpibm->ctl)
		open_gpibm->ctl->funcs.og_close(open_gpibm->ctl);
err1:
	free(open_gpibm);
	return NULL;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int close_gpib ( struct open_gpib_mstr *open_gpibp)
{
	if(NULL != open_gpibp->ctl->funcs.og_control)
		open_gpibp->ctl->funcs.og_control(open_gpibp->ctl,CTL_CLOSE,0);
	if(NULL != open_gpibp->ctl->funcs.og_close)
		open_gpibp->ctl->funcs.og_close(open_gpibp->ctl);
	
	fprintf(stderr,"Free %s\n",open_gpibp->ctl->dev_path);
	if(NULL != open_gpibp->ctl->dev_path)
		free(open_gpibp->ctl->dev_path);
	fprintf(stderr,"Now freeing ctl\n");
	free(open_gpibp->ctl);
	open_gpibp->ctl=NULL;
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int gpib_option_to_type(char *op)
{
	int i;
	for (i=0; NULL != s_dev[i].name;++i){
		if(!strcmp(s_dev[i].option,op))
			return s_dev[i].type;
	}	
	return -1;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void show_gpib_supported_controllers(void)
{
	int i;
	fprintf(stderr,"Type 'Option' Name\n");
	for (i=0; NULL != s_dev[i].name;++i){
		fprintf(stderr,"%d    '%s' %s\n",gpib_option_to_type(s_dev[i].option),s_dev[i].option, s_dev[i].name);
	}
}

/***************************************************************************/
/** Set up timeouts and send the id string.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_id( struct open_gpib_mstr *open_gpibp, char *idstr)
{
	open_gpibp->ctl->funcs.og_control(open_gpibp->ctl,CTL_SET_TIMEOUT,500);
	while(read_string(open_gpibp->ctl));
	open_gpibp->ctl->funcs.og_control(open_gpibp->ctl,CTL_SET_TIMEOUT,50000);
	/*write_string(open_gpibp,"*CLS"); */
	if(0 == write_get_data(open_gpibp->ctl,idstr))
		return -1;
	return 0;
}


