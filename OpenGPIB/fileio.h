/** \file ******************************************************************
\n\b File:        hp16500ip.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/11/2011 10:23 am
\n\b Description: Header file for HP the fileio OpenGPIB driver.
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
#ifndef _FILEIO_H_
#define _FILEIO_H_ 1
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct fileio_ctl {
	FILE *f;        /**file pointer for open file  */
	int   fd;       /**file descriptor, not used, but available  */
	char *name;     /**file name  */
  char *last_cmd; /**string of write command  */
  int last_cmd_len; /**length of string above  */
  int state; /**for command states  */
  int debug;
  off_t readto; /**place to read to in the file  */
  off_t filelen;        /**length of file  */
  off_t pos;        /**current place in file  */
};

int _fileio_open(struct gpib *g, char *name);
int control_fileio(struct gpib *g, int cmd, int data);
int _fileio_write(void *d, void *buf, int len);
int _fileio_read(void *d, void *buf, int len);
int _fileio_close(struct gpib *g);
int register_fileio(struct gpib *g);

#endif

