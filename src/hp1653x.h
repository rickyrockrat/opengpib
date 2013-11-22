/** \file ******************************************************************
\n\b File:        hp1653x.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/15/2013 11:52 am
\n\b Description: Header for Routines specific to hp1653x oscilliscope cards.
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
#ifndef _HP1653X_H_
#define _HP1653X_H_ 1
#include <hp16500.h>
struct hp_scope_preamble {
	double xinc;  /**time value between consecutive data points. Time between samples in FULL mode, the only mode we run in.  */
	double xorg;  /**Time of first data point in memory with respect to the trigger.  */
	double xref;  /** X value of first data point in memory always zero  */
	double yinc;  /**Voltage difference between consecutive data values  */
	double yorg;  /**value of voltage at center screen  */
	double yref;  /**The value at center screen where the Y-origin occurs  */
  int xinc_thou;  /**number of powers of 1000  to get xinc to >=1*/
  double xincmult;  /**multiplier to remove -e exponet, i.e. 2e-6*1000000=2  */
  char xunits[3]; /**units, i.e. ms,us,ns,ps  */
	int points;
	int type;
	int fmt;    /**ascii, byte, word  */
  int data_size; /**related to fmt, above.  */
  int count;
  long point_len; /**number of bytes that follow on data stream  */
  int point_start; /**location in buffer where points start.  */
};
/**scope functions  */
int get_trigger_source(struct open_gpib_mstr *g);
int oscope_parse_preamble(struct open_gpib_mstr *g, struct hp_scope_preamble *h);
int check_oscope_channel(char *ch);
int oscope_get_preamble(struct open_gpib_mstr *g,char *ch, struct hp_scope_preamble *h);
int get_oscope_data(struct open_gpib_mstr *g, char *ch, struct hp_scope_preamble *h);
int init_oscope_instrument(struct hp_common_options *o, struct open_gpib_mstr *g); 
#endif

