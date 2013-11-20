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
#ifndef OPEN_GPIB_IP_H_
#define OPEN_GPIB_IP_H_ 1


#include <open-gpib.h>
/**only add commands here we don't already have  */
enum {
	IP_CMD_SET_PORT=CMD_LAST,
	IP_CMD_NONE,
};
#endif

