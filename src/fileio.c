/** \file ******************************************************************
\n\b File:        fileio.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/11/2011 10:20 am
\n\b Description: file i/o driver for Open GPIB.
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
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "open-gpib.h"
#include "fileio.h"


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
off_t readlen(struct fileio_ctl *c, int len)
{
  off_t t;
  if(c->pos < c->readto){
    t=c->readto - c->pos;
    if(t < len){
      return t;
    }else
      return (off_t) len;
  }
  return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: -1  on failure, 0 on success
****************************************************************************/
int _fileio_open(struct gpib *g, char *name)
{
	struct fileio_ctl *c;
  struct stat finfo;
	if(NULL == g){
		fprintf(stderr,"%s: dev null\n",__func__);
		return 1;
	}
	c=(struct fileio_ctl *)g->ctl;
	if( check_calloc(sizeof(struct fileio_ctl), &c, __func__,NULL) == -1) return -1;
  c->last_cmd=NULL;
	if(OPTION_DEBUG&g->type_ctl) 
		c->debug=1;
	else
		c->debug=0;
	c->name=strdup(name);
	  /* Open the file for reading */
  if(NULL ==(c->f = fopen( c->name,"r")) ){
		fprintf(stderr,"Unable to open '%s'\n",c->name);
		goto err;
	}
  if(-1 == stat(name,&finfo)){
    fprintf(stderr,"Unable to Stat '%s'\n",c->name);
    fclose(c->f);
    free(c->name);
		goto err;
  }
  c->filelen=finfo.st_size;
  c->pos=0;
  c->readto=0;
	fprintf(stderr,"Opened '%s', %ld bytes\n",c->name,c->filelen);
	g->ctl=c;
	return 0;
	
err:
	free(c);
	g->ctl=NULL;
	return -1;
}

/***************************************************************************/
/** just save the command.
\n\b Arguments:
\n\b Returns: -1 on failure, number bytes written otherwise
****************************************************************************/
int _fileio_write(void *d, void *buf, int len)
{
	struct fileio_ctl *c;
	
	c=(struct fileio_ctl *)d;
  if(NULL == buf)
    return -1;
  c->last_cmd=strdup((char *)buf);
  c->last_cmd_len=len;
  return len;  
}

/***************************************************************************/
/** Guess which instrument we are supposed to be by looking at the command 
bytes, then do what we should..
\n\b Arguments:
\n\b Returns: -1 on failure, number of byte read otherwise
****************************************************************************/
int _fileio_read(void *d, void *buf, int len)
{
	struct fileio_ctl *c;
	int i,x;
	char *m;
  char s[20];
  
	m=(char *)buf;
	c=(struct fileio_ctl *)d;
  if(NULL == buf )
    return -1;
  if(NULL == c->last_cmd){/**just read to file position  */
    i=fread(m,1,readlen(c,len),c->f);
    c->pos +=i;
    return i;
  }
  if(!strncasecmp(":WAV:VAL?",c->last_cmd,c->last_cmd_len)){
    i=snprintf(m,len,"1");
    c->readto=c->pos; /**prevent any file reading.  */
  } else if(!strncasecmp(":WAV:SOUR",c->last_cmd,c->last_cmd_len)){/**handle hp scope preable and data here.  */
    if(NULL != strcasestr(c->last_cmd, "PRE?")){
      x=1;
      while(x!='#' && x != EOF && c->readto <c->filelen){
        x=fgetc(c->f);
        c->readto++;
      }
      if(x != '#'){
        fprintf(stderr,"Start of HP scope preamble not found!\n");
        c->readto=0;
        return -1;
      }  
      ungetc(x,c->f);
      c->readto--;
      i=fread(m,1,readlen(c,len),c->f);
    } else if(NULL != strcasestr(c->last_cmd, "DATA?")){ 
      c->readto=c->filelen;
      i=fread(m,1,readlen(c,len),c->f);
    }  
  }else if(NULL != strcasestr(":SYSTEM:",c->last_cmd)){ /**hplogic analyzer card  */
    if(NULL != strcasestr( ":SETUP?",c->last_cmd)){/**file should start with #8lenCONFIG  */
      
      if( '#' != fgetc(c->f) ) goto hplogicerr;
      if( '8'== fgetc(c->f) )goto hplogicerr;
      if(EOF == fscanf(c->f,"%ld",&c->readto) ) goto hplogicerr;
      fread(s,1,6,c->f);
      if(strncasecmp("CONFIG",s,6)) goto hplogicerr;
      fprintf(stderr,"Found Config. Readto is %ld\n",c->readto);
      c->readto +=10;
      fseek(c->f, 0L, SEEK_SET); /**rewind file  */
      i=fread(m,1,readlen(c,len),c->f);
      goto end;
hplogicerr:
      fprintf(stderr,"Unable to find start of CONFIG\n");
      return 0;
    }
    else if(NULL != strcasestr(":DATA?",c->last_cmd)){
      c->readto=c->filelen;
      i=fread(m,1,readlen(c,len),c->f);
    }
    /*WFMPRE WFID:"CH1 DC   5mV 500ms NORMAL",NR.PT:1024,PT.OFF:128,PT.FMT:Y,XUNIT:SEC,XINCR:1.000E-2,YMULT:2.000E-4,YOFF:-2.625E+1,YUNIT:V,BN.FMT:RI,ENCDG:ASCII;CURVE */
  }else if(!strncasecmp("WAV?",c->last_cmd,c->last_cmd_len) ) {/**tek waveform cmd  */
    c->readto=c->filelen;
    i=fread(m,1,len,c->f);
  }
end:
  c->pos +=i;  
  free(c->last_cmd);
  c->last_cmd=NULL;
  c->last_cmd_len=0;
	return i;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int control_fileio(struct gpib *g, int cmd, uint32_t data)
{
	struct fileio_ctl *c; 

	if(NULL == g){
		fprintf(stderr,"fileio: gpib null\n");
		return 0;
	}
	c=(struct fileio_ctl *)g->ctl;
	/**sets g->ctl if it allocates new memory  */
	if( check_calloc(sizeof(struct fileio_ctl), &c, __func__,&g->ctl) == -1) return -1;
		
	switch(cmd){
		case CTL_CLOSE:
			if(c->debug) fprintf(stderr,"Closing hp INET\n");
			break;
		case CTL_SET_TIMEOUT:
			break;
		case CTL_SET_ADDR: /**send command, check result, then set gpib addr.  */
			break;
		case CTL_SET_DEBUG:
			if(data)
				c->debug=1;
			else
				c->debug=0;
			break;
		case CTL_SEND_CLR:
			break;
		default:
			fprintf(stderr,"Unsupported cmd '%d'\n",cmd);
			break;
	}
	return 0;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int _fileio_close(struct gpib *g)
{
	struct fileio_ctl *c;
	if(NULL == g->ctl)
		return 0;
	c=(struct fileio_ctl *)g->ctl;
  if(c->last_cmd)
    free(c->last_cmd);
  c->last_cmd=NULL;
	fclose(c->f);
	free (g->ctl);
	g->ctl=NULL;
	return 0;
}




/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int register_fileio(struct gpib *g)
{
	g->control=control_fileio;
	g->read=	_fileio_read;
	g->write=	_fileio_write;
	g->open=	_fileio_open;
	g->close=	_fileio_close;
	return 0;
}
