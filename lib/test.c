/** \file ******************************************************************
\n\b File:        test.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/25/2013 12:54 pm
\n\b Description: file to test various library functions
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

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: 0 on success.
****************************************************************************/
int test_parameter_lists(void)
{
	struct open_gpib_param *head;
	uint32_t x;
	int32_t s;
	
	head=open_gpib_new_param(NULL,"uint32_1", "u",NULL);
	open_gpib_show_param(head);
	head=open_gpib_new_param(head, "uint32_2","u", 201);
	head=open_gpib_new_param(head, "int32_","s", 201);
	open_gpib_show_param(head);
	head=open_gpib_new_param(head, "uint32_2","u", 222);
	head=open_gpib_new_param(head, "str","c", "ThisisMyStr");
	open_gpib_show_param(head);
	printf("\n");
	open_gpib_set_uint32_t(head, "uint32_2", 200);
	open_gpib_show_param(head);
	open_gpib_set_param(head, "uint32_1","u", 100);
	open_gpib_show_param(head);
	s=-30;
	x=(uint32_t)s;
	open_gpib_set_param(head, "uint32_1","u", x);
	open_gpib_show_param(head);
	open_gpib_set_param(head, "int32_","s", x);
	open_gpib_show_param(head);
	open_gpib_set_param(head, "int32","s", x);
	printf("\n");
	head=open_gpib_del_param(head,"int32_");
	open_gpib_show_param(head);
	head=open_gpib_del_param(head,"str");
	open_gpib_show_param(head);
	return 0;
}
int test_parametr_lists2 ( void )
{
	struct open_gpib_param *head=NULL;
	head=open_gpib_param_init(og_static_settings,NULL);
	open_gpib_show_param(head);
	open_gpib_param_free(head);
	head=NULL;
	head=open_gpib_param_init(og_static_settings,"serial");
	open_gpib_show_param(head);
	return 0;
}

int test_show_if (void)
{
	return open_gpib_list_interfaces();
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: 0=OK, 1=error
****************************************************************************/
int test_config(void)
{
	struct og_conf *c=og_conf_open("test.cfg");
	uint32_t v;
	if(og_conf_get_group(c,"mygroup")) {
		printf("Err getting mygroup\n");
		return 1;
	}
	printf("var1='%s'\n",og_conf_get_string(c,"var1"));
	if(og_conf_get_uint32(c,"var2",&v)){
		printf("Err getting var2\n");
		return 1;
	}
		
	else
		printf("var2=%d\n",v);
	return 0;
}
int main (int argc, char *argv[])
{
	int x;
	if(argc || argv)
		x=10; /**prevent unused warning  */
	/*test_parameter_lists(); */
	/*test_config(); */
	/*test_parametr_lists2(); */
	test_show_if();
	return 0;
}

