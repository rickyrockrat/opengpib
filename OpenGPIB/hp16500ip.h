/** \file ******************************************************************
\n\b File:        hp16500ip.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/19/2010 11:56 am
\n\b Description: Header file for HP the IP control
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
#ifndef _HP16500IP_H_
#define _HP16500IP_H_ 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct hp16500c_ctl {
	char *ipaddr;
	int port;
	int debug;
	struct sockaddr_in socket;
	int   sockfd;
	int cmd_wait;
};

int hp16500c_init(struct gpib *g);
int _hp16500c_open(struct gpib *g, char *ip);
int control_hp16500c(struct gpib *g, int cmd, int data);
int _hp16500c_write(void *d, void *buf, int len);
int _hp16500c_read(void *d, void *buf, int len);
int _hp16500c_close(struct gpib *g);
int register_hp16500c(struct gpib *g);

#endif

