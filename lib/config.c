 /** \file ******************************************************************
 \n\b File:        config.c
 \n\b Author:      Doug Springer
 \n\b Company:     DNK Designs Inc.
 \n\b Date:        11/28/2013  8:47 am
 \n\b Description: libconfig interface file
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
 */
#include <open-gpib.h>
#include <libconfig.h>

struct og_conf_int {
	config_setting_t *set;
	struct config_t cfg;
	char *setname;
};

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:	config handle on success, NULL on error
****************************************************************************/
struct og_conf *og_conf_open(char *path)
{
	struct og_conf *ogc=NULL;
	struct og_conf_int *i;
	if(NULL == path){
		fprintf(stderr,"%s: Path Null\n",__func__);
		goto err;
	}
	if(access(path, F_OK) ){
		fprintf(stderr,"%s: Unable to access '%s'\n",__func__,path);
		goto err;
	}
	if(NULL ==(ogc=calloc(1,sizeof(struct og_conf)) )){
		fprintf(stderr,"%s: memory alloc erro\n",__func__);
		goto err;
	}
	
	if(NULL ==(i=calloc(1,sizeof(struct og_conf_int)) )){
		fprintf(stderr,"%s: memory alloc erro\n",__func__);
		goto err;
	}
	config_init(&i->cfg);
	if(!config_read_file(&i->cfg, path)){
		fprintf(stderr,"%s: Error in file '%s' on line %d: %s\n",__func__, path, config_error_line(&i->cfg), config_error_text(&i->cfg));
		goto err;
	}
	ogc->fname=strdup(path);
	ogc->dat=i;
	return ogc;
err:
	if(NULL != ogc)
		free(ogc);
	return NULL;
}

/***************************************************************************/
/** de-allocate our struct.
config_write_file(&cfg, file);
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void og_conf_close(struct og_conf *ogc)
{
	struct og_conf_int *i;
	if(NULL != ogc){
		if(NULL != ogc->fname)
			free(ogc->fname);
		ogc->fname=NULL;
		if(NULL != ogc->dat){
			i=(struct og_conf_int *)ogc->dat;
			if(NULL != i->setname)
				free(i->setname);
			config_destroy(&i->cfg);
			free(ogc->dat);
		}
			
		ogc->dat=NULL;
		free(ogc);
	}
}

/***************************************************************************/
/** get a setting, or group.
\n\b Arguments:
\n\b Returns: 0 on success, 1 on failure
****************************************************************************/
int og_conf_get_group(struct og_conf *ogc, char *name)
{
	struct og_conf_int *i;
	if(NULL == ogc || NULL == ogc->dat ){
		fprintf(stderr,"%s: config is NULL\n",__func__);
		return 1;
	}
	i=(struct og_conf_int *)ogc->dat;
	if(NULL == (i->set=config_lookup(&i->cfg, name)) ){
		fprintf(stderr,"%s: Can't find group '%s'\n",__func__,name);
		return 1;
	}
	return 0;
}

/***************************************************************************/
/** Get an int.
\n\b Arguments:
name is name of parameter, val is the value of same.
\n\b Returns:	0 on success 1 on error
****************************************************************************/
int og_conf_get_uint32(struct og_conf *ogc, char *name, uint32_t *val)
{
	int v;
	struct og_conf_int *i;
	if(NULL == ogc || NULL == ogc->dat){
		fprintf(stderr,"%s: config is NULL\n",__func__);
		return 1;
	}
	i=(struct og_conf_int *)ogc->dat;
	if(NULL ==i || NULL == i->set){
		if(CONFIG_FALSE == config_lookup_int(&i->cfg, name, &v) ){
			fprintf(stderr,"%s: Can't find '%s'\n",__func__,name);
			return 1;
		}	
	}else{
		if(CONFIG_FALSE == config_setting_lookup_int(i->set, name, &v) ){
			fprintf(stderr,"%s: Can't find '%s' in setting '%s'\n",__func__,name,i->setname);
			return 1;
		}	
	}
	
	/**config_setting_set_int(setting, x);  */
	*val = (uint32_t)v;
	return 0;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: string on success, NULL on failure
****************************************************************************/
const char *og_conf_get_string(struct og_conf *ogc, char *name)
{
	const char *str=NULL;
	struct og_conf_int *i;
	if(NULL == ogc || NULL == ogc->dat ){
		fprintf(stderr,"%s: config is NULL\n",__func__);
		return NULL;
	}
	i=(struct og_conf_int *)ogc->dat;
	if(NULL == i->set){
		if(CONFIG_FALSE == config_lookup_string(&i->cfg, name, &str) ){
			fprintf(stderr,"%s: Can't find '%s'\n",__func__,name);
			return NULL;
		}
	}	else {
		if(CONFIG_FALSE == config_setting_lookup_string(i->set, name, &str) ){
			fprintf(stderr,"%s: Can't find '%s' in setting '%s'\n",__func__,name,i->setname);
			return NULL;
		}	
	}
	return str;
}



