/** \file ******************************************************************
\n\b File:        hp16500.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/13/2010 11:42 am
\n\b Description: 
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

#ifndef _HP16500_H_
#define _HP16500_H_ 1
#include <open-gpib.h>

#define SLOTNO_16500C    0 /**0 to talk to system (i.e. controller)  */
/** Id Number Card*/
#define CARDTYPE_16500C  0         /** HP 16500C system  */
#define CARDTYPE_16515A  1         /** HP 16515A 1GHz Timing Master Card*/
#define CARDTYPE_16516A  2         /** HP 16516A 1GHz Timing Expansion Card*/
#define CARDTYPE_16517A  4         /** HP 16517A 4GHz Timing/1GHz State Analyzer Master Card*/
#define CARDTYPE_16518A  5         /** HP 16518A 4GHz Timing/1GHz State Analyzer Expansion Card*/
#define CARDTYPE_16530A  11        /** HP 16530A 400 MSa/s Oscilloscope Timebase Card*/
#define CARDTYPE_16531A  12        /** HP 16531A Oscilloscope Acquisition Card*/
#define CARDTYPE_16532A  13        /** HP 16532A 1GSa/s Oscilloscope Card*/
#define CARDTYPE_16534A  14        /** HP 16533A or HP 16534A 32K GSa/s Oscilloscope Card*/
#define CARDTYPE_16535A  15        /** HP 16535A MultiProbe 2-Output Module*/
#define CARDTYPE_16520A  21        /** HP 16520A Pattern Generator Master Card*/
#define CARDTYPE_16521A  22        /** HP 16521A Pattern Generator Expansion Card*/
#define CARDTYPE_16522AE 24        /** HP 16522A 200MHz Pattern Generator Expansion Card*/
#define CARDTYPE_16522AM 25        /** HP 16522A 200MHz Pattern Generator Master Card*/
#define CARDTYPE_16511B  30        /** HP 16511B Logic Analyzer Card*/
#define CARDTYPE_16510A  31        /** HP 16510A or B Logic Analyzer Card*/
#define CARDTYPE_16550AM 32        /** HP 16550A 100/500 MHz Logic Analyzer Master Card*/
#define CARDTYPE_16550AE 33        /** HP 16550A 100/500 MHz Logic Analyzer Expansion Card*/
#define CARDTYPE_16554M  34        /** HP 16554, 16555, or 16556 Logic Analyzer Master Card*/
#define CARDTYPE_16554E  35        /** HP 16554, 16555, or 16556 Logic Analyzer Expansion Card*/
#define CARDTYPE_16540A  40        /** HP 16540A 100/100 MHz Logic Analyzer Master Card*/
#define CARDTYPE_16541A  41        /** HP 16541A 100/100 MHz Logic Analyzer Expansion Card*/
#define CARDTYPE_16542A  42        /** HP 16542A 2 MB Acquisition Logic Analyzer Master Card*/
#define CARDTYPE_INVALID -1

#define MODEL_16500C  "16500C"
#define MODEL_16515A  "16515A"
#define MODEL_16516A  "16516A"
#define MODEL_16517A  "16517A"
#define MODEL_16518A  "16518A"
#define MODEL_16530A  "16530A"
#define MODEL_16531A  "16531A"
#define MODEL_16532A  "16532A"
#define MODEL_16534A  "16533/34A"
#define MODEL_16535A  "16535A"
#define MODEL_16520A  "16520A"
#define MODEL_16521A  "16521A"
#define MODEL_16522AE "16522A"
#define MODEL_16522AM "16522A"
#define MODEL_16511B  "16511B"
#define MODEL_16510A  "16510A/B"
#define MODEL_16550AM "16550A"
#define MODEL_16550AE "16550A"
#define MODEL_16554M  "16554/16555/16556"
#define MODEL_16554E  "16554/16555/16556"
#define MODEL_16540A  "16540A"
#define MODEL_16541A  "16541A"
#define MODEL_16542A  "16542A"

#define DESC_16500C  "16500C System"
#define DESC_16515A  "1GHz Timing Master Card"
#define DESC_16516A  "1GHz Timing Expansion Card"
#define DESC_16517A  "4GHz Timing/1GHz State Analyzer Master Card"
#define DESC_16518A  "4GHz Timing/1GHz State Analyzer Expansion Card"
#define DESC_16530A  "400 MSa/s Oscilloscope Timebase Card"
#define DESC_16531A  "Oscilloscope Acquisition Card"
#define DESC_16532A  "1GSa/s Oscilloscope Card"
#define DESC_16534A  "32K GSa/s Oscilloscope Card"
#define DESC_16535A  "MultiProbe 2-Output Module"
#define DESC_16520A  "Pattern Generator Master Card"
#define DESC_16521A  "Pattern Generator Expansion Card"
#define DESC_16522AE "200MHz Pattern Generator Expansion Card"
#define DESC_16522AM "200MHz Pattern Generator Master Card"
#define DESC_16511B  "Logic Analyzer Card"
#define DESC_16510A  "Logic Analyzer Card"
#define DESC_16550AM "100/500 MHz Logic Analyzer Master Card"
#define DESC_16550AE "100/500 MHz Logic Analyzer Expansion Card"
#define DESC_16554M  "Logic Analyzer Master Card"
#define DESC_16554E  "Logic Analyzer Expansion Card"
#define DESC_16540A  "100/100 MHz Logic Analyzer Master Card"
#define DESC_16541A  "100/100 MHz Logic Analyzer Expansion Card"
#define DESC_16542A  "2 MB Acquisition Logic Analyzer Master Card"


struct hp_common_options {
	char *dev;    /**Either device name, i.e. /dev/ttyxxx or the IP address  */
	int iaddr;    /**instrument address for GPIB  */
	int cardtype; /**Type of card to talk to  */
	int cardno;   /**which card, if multiple cards  */
	int dtype;    /**transport type, i.e. LAN, GPIB, etc.  */
  int fmt;      /**data format. 0=ascii 1=byte,2=word  */
};
#define FMT_ASCII 0
#define FMT_BYTE  1
#define FMT_WORD  2
#define FMT_NONE  -1


#define HP_COMMON_GETOPS "a:d:m:n:t:"

#ifdef HP16500_COM_SYMBOLS

#endif

#define SHOW_PRINT 1
#define JUST_LOAD 2

#define COM_USE_MODE_GENERIC 0
#define COM_USE_MODE_SCOPE   1
#define COM_USE_MODE_LOGIC   2

/**generic functions  */
int get_hp_info_card(int id);
int print_card_model(int id);
int select_hp_card(int slot, struct open_gpib_mstr *g);
void show_known_hp_cards(void);
int hp16500_find_card(int cardtype, int no, struct open_gpib_mstr *g);
void show_hp_connection_info(void);
void show_common_usage (int mode);
int handle_common_opts(int c, char *optarg, struct hp_common_options *o);

#endif
