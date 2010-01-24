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
#include "common.h"
/** Id Number Card*/
#define CARDTYPE_16515A  1         /** HP 16515A 1GHz Timing Master Card*/
#define CARDTYPE_16516A  2         /** HP 16516A 1GHz Timing Expansion Card*/
#define CARDTYPE_16517A  4         /** HP 16517A 4GHz Timing/1GHz State Analyzer Master Card*/
#define CARDTYPE_16518A  5         /** HP 16518A 4GHz Timing/1GHz State Analyzer Expansion Card*/
#define CARDTYPE_16530A  11        /** HP 16530A 400 MSa/s Oscilloscope Timebase Card*/
#define CARDTYPE_16531A  12        /** HP 16531A Oscilloscope Acquisition Card*/
#define CARDTYPE_16532A  13        /** HP 16532A 1GSa/s Oscilloscope Card*/
#define CARDTYPE_16533A  14        /** HP 16533A or HP 16534A 32K GSa/s Oscilloscope Card*/
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

#define DESC_16515A "HP 16515A 1GHz Timing Master Card"
#define DESC_16516A "HP 16516A 1GHz Timing Expansion Card"
#define DESC_16517A "HP 16517A 4GHz Timing/1GHz State Analyzer Master Card"
#define DESC_16518A "HP 16518A 4GHz Timing/1GHz State Analyzer Expansion Card"
#define DESC_16530A "HP 16530A 400 MSa/s Oscilloscope Timebase Card"
#define DESC_16531A "HP 16531A Oscilloscope Acquisition Card"
#define DESC_16532A "HP 16532A 1GSa/s Oscilloscope Card"
#define DESC_16533A "HP 16533A or HP 16534A 32K GSa/s Oscilloscope Card"
#define DESC_16535A "HP 16535A MultiProbe 2-Output Module"
#define DESC_16520A "HP 16520A Pattern Generator Master Card"
#define DESC_16521A "HP 16521A Pattern Generator Expansion Card"
#define DESC_16522AE "HP 16522A 200MHz Pattern Generator Expansion Card"
#define DESC_16522AM "HP 16522A 200MHz Pattern Generator Master Card"
#define DESC_16511B "HP 16511B Logic Analyzer Card"
#define DESC_16510A "HP 16510A or B Logic Analyzer Card"
#define DESC_16550AM "HP 16550A 100/500 MHz Logic Analyzer Master Card"
#define DESC_16550AE "HP 16550A 100/500 MHz Logic Analyzer Expansion Card"
#define DESC_16554M "HP 16554, 16555, or 16556 Logic Analyzer Master Card"
#define DESC_16554E "HP 16554, 16555, or 16556 Logic Analyzer Expansion Card"
#define DESC_16540A "HP 16540A 100/100 MHz Logic Analyzer Master Card"
#define DESC_16541A "HP 16541A 100/100 MHz Logic Analyzer Expansion Card"
#define DESC_16542A "HP 16542A 2 MB Acquisition Logic Analyzer Master Card"
struct hp_cards{
	int type;
	char *desc;
};
#ifdef HP16500_COM_SYMBOLS
struct hp_cards hp_cardlist[]=\
{
	{CARDTYPE_16515A,  DESC_16515A},
	{CARDTYPE_16516A,  DESC_16516A},
	{CARDTYPE_16517A,  DESC_16517A},
	{CARDTYPE_16518A,  DESC_16518A},
	{CARDTYPE_16530A,  DESC_16530A},
	{CARDTYPE_16531A,  DESC_16531A},
	{CARDTYPE_16532A,  DESC_16532A},
	{CARDTYPE_16533A,  DESC_16533A},
	{CARDTYPE_16535A,  DESC_16535A},
	{CARDTYPE_16520A,  DESC_16520A},
	{CARDTYPE_16521A,  DESC_16521A},
	{CARDTYPE_16522AE,  DESC_16522AE},
	{CARDTYPE_16522AM,  DESC_16522AM},
	{CARDTYPE_16511B,  DESC_16511B},
	{CARDTYPE_16510A,  DESC_16510A},
	{CARDTYPE_16550AE,  DESC_16550AE},
	{CARDTYPE_16550AM,  DESC_16550AM},
	{CARDTYPE_16554M,  DESC_16554M },
	{CARDTYPE_16554E,  DESC_16554E },
	{CARDTYPE_16540A,  DESC_16540A},
	{CARDTYPE_16541A,  DESC_16541A},
	{CARDTYPE_16542A,  DESC_16542A},
	{CARDTYPE_INVALID,NULL},
};
#endif

struct hp_scope_preamble {
	float xinc;
	float xorg;
	float xref;
	float yinc;
	float yorg;
	float yref;
	int points;
	int fmt;
};

#define HP_FMT_ASC 	0
#define HP_FMT_BYTE 1
#define HP_FMT_WORD 2

#define CLK_START 0
#define POD_START 2
#define POD_ARRAYSIZE 10 /**don't change this - it's set by the hp config data  */

#define HEADER_SIZE 10 /** length of preamble before block starts.  */

struct section_hdr {
	char name[11]; /**the last byte is reserved, but usally 0  */
	uint8 module_id;
	uint32 block_len; /**lsb is most significant byte, i.e 0x100 is stored as 10 0, not 0 10  */
}__attribute__((__packed__));	

struct rtc_data {
	uint16 year; /**RTC=year-1990  */
	uint8 month; /**manual says 2 bytes, but it appears to be a misprint  */
	uint8 day;
	uint8 dow;
	uint8 hour;
	uint8 min;
	uint8 sec;
}__attribute__((__packed__));	
struct analyzer_data {
	uint32 data_mode;
	uint32 pods; /**bit 21-clock pod, bit 1-12=pod 1-12, all other bits unused.  */
	uint32 masterchip;
	uint32 maxmem;
	uint32 unused1;
	uint64 sampleperiod;
	uint32 tag_type; /**0-off,1=time, 2=state tags.  */
	uint64 trig_off;
	uint8 unused2[30];
}__attribute__((__packed__));	

/**  
	uint32 data_pod4_hi;
	uint32 data_pod3_hi;
	uint32 data_pod2_hi;
	uint32 data_pod1_hi;
	uint32 data_pod4_mid;
	uint32 data_pod3_mid;
	uint32 data_pod2_mid;
	uint32 data_pod1_mid;
	uint32 data_pod4_master;
	uint32 data_pod3_master;
	uint32 data_pod2_master;
	uint32 data_pod1_master;
	uint8 unused2[40];
	uint32 trig_pod4_hi;
	uint32 trig_pod3_hi;
	uint32 trig_pod2_hi;
	uint32 trig_pod1_hi;
	uint32 trig_pod4_mid;
	uint32 trig_pod3_mid;
	uint32 trig_pod2_mid;
	uint32 trig_pod1_mid;
	uint32 trig_pod4_master;
	uint32 trig_pod3_master;
	uint32 trig_pod2_master;
	uint32 trig_pod1_master;
*/
struct data_preamble {
	uint32 instid;
	uint32 rev_code;
	uint32 chips;
	uint32 analyzer_id;
	struct analyzer_data a1;
	struct analyzer_data a2;
	uint8 unused1[40];
	/**number of valid rows of data... offset 173 */
	uint32 data_hi[4];/**0=pod4,3=pod1  */
	uint32 data_mid[4];
	uint32 data_master[4];
	/**trigger position?? offset 261 */
	uint8 unused2[40];
	uint32 trig_hi[4];
	uint32 trig_mid[4];
	uint32 trig_master[4];
	/** offset 349  */
	uint8 unused3[234];
	/**  acquision time*/
	
	struct rtc_data rtc;
	char *data;	 /**origial section data  */
	uint32 data_sz;	/**and section data size  */
}__attribute__((__packed__));	

struct block_spec {
	uint8 blockstart[2]; /**should be #8  */
	char blocklen_data[8];	 /**decimal len of block, stored in ascii  */
	uint32 blocklen;
}__attribute__((__packed__));	

struct hp_data_block {
	struct block_spec bs;
	struct section_hdr shdr;
	struct data_preamble preamble;
}__attribute__((__packed__));	

struct hp_block_hdr {
	struct block_spec bs;
	char *data;
}__attribute__((__packed__));	

struct section {
	struct section_hdr hdr;
	uint32 sz;
	uint32 off;
	char *data;
}__attribute__((__packed__));	

/**these are guesses, since I haven't figured out how to separate A1 from A2 or A3 from A4  */
/**for the pod_info* bytes  */
#define POD_INFO_A1 0x02
#define POD_INFO_A2 0x04
#define POD_INFO_A3 0x08
#define POD_INFO_A4 0x10
/**10 bytes  */
struct pod_assignment {
	uint8 unknown1[3];
	uint8 pod_info0; /**pod_info0 and 1 always seem to be equal to each other No pods=0 */
	uint8 unknown2[3];
	uint8 pod_info1;
	uint8 unknown3;
	uint8 pod_info2; /**this is 0x06 with no pods assigned to either analyzer, 0x66 if pod1/2, 0x78 if pod 3/4, 0x1E if pod1,2,3,4.  */
}__attribute__((__packed__));	
/**defines for the mode byte below  */
#define MACHINE_MODE_OFF 		0
#define MACHINE_MODE_TIMING 1
#define MACHINE_MODE_STATE	2
#define MACHINE_MODE_CMP_SPA 3
#define MACHINE_MODE2_FULL 0
#define MACHINE_MODE2_HALF 1
/*64 bytes per machine config*/

struct machine_config {
	char name[11];
	uint8 unknown3[4];
	uint8 mode;	/**0=off,1=timing,2=state,3=statecompare or SPA  */
	uint8 unknown4[7];
	uint8 mode2;	 /*0=fullchannel, 1=half channel*/
	uint8 unknown5[8];
	struct pod_assignment assign;
	uint8	un[22];
}__attribute__((__packed__));	

struct config_data {
	struct machine_config m[2];
}__attribute__((__packed__));	

#define LABEL_RECORD_LEN 22
#define LABEL_MAP_LEN 12

struct label_map {
	/**offset 0= hi byte.  */
	uint8 clk_pods[POD_ARRAYSIZE]; /**0 and 1 index are clk, 2 idx is hi byte of pod4  */
	uint8 unknown[2];
}__attribute__((__packed__));	

struct labels {
	char name[6];
	uint8 unknown1[3];
	uint8 polarity;
	uint8 unknown2;
	uint8 strange_offset;
	uint16 strange_offsetlo;
	uint8 unknown3[4];
	uint8 bits;
	uint8 enable;
	uint8 sequence;
	uint32 actual_offset;
	struct label_map map;
}__attribute__((__packed__));	

struct one_card_data {
	uint16 clkunused;
	uint8 pdata[POD_ARRAYSIZE]; /**clkhi is 0, clklo is 1, pod 4 is 2  */
}__attribute__((__packed__));	
struct two_card_data {
	uint32 clk;
	uint16 epods[4];
	uint16 mpods[4];
}__attribute__((__packed__));	

struct three_card_data {
	uint32 clk;
	uint16 eh_pods[4];
	uint16 el_pods[4];
	uint16 mpods[4];
}__attribute__((__packed__));	

struct signal_data {
	int bits; /**width of signal in bits  */
	int ena; /**is signal enabled in menu  */
	int lsb; /**lowest number bit where signal starts  */
	int msb; /**highes number bit where signal ends  */
	int pol; /**polarity of signal 1=normal (+), 0= inverted (-) */
	char *name;
	struct signal_data *next;
};

#define ONE_CARD_ROWSIZE 12
#define TWO_CARD_ROWSIZE 20
#define THREE_CARD_ROWSIZE 28



#define SHOW_PRINT 1
#define JUST_LOAD 2
/** 
Cards Clock Pod Bytes Data Bytes Total Bytes Per Row
1     4 bytes         8 bytes    12 bytes
2     4 bytes         16 bytes   20 bytes
3     4 bytes         24 bytes   28 bytes


The sequence of pod data within a row is the same as shown above for th
number of valid rows per pod. The number of valid rows per pod can be
determined by examining bytes 253 through 256 for pod pair 3/4 of the
master card and bytes 257 through 260 for pod pair 1/2 of the master car
The number of valid rows for other pod pairs is contained in bytes 213
through 252.
A one-card configuration has the following data arrangement (per row):
<clk pod 1> <pod 4> <pod 3> <pod 2> <pod 1>
A two-card configuration has the following data arrangement (per row):
       <-----expansion card ------><-------master card-------->
<clk 1><pod 4><pod 3><pod 2><pod 1><pod 4><pod 3><pod 2><pod 1>
A three-card configuration has the following data arrangement per row:
       <- high expander -><-lower expander -><---master card--->
<clk 1><pod 4>. . .<pod 1><pod 4>. . .<pod 1><pod 4>. . .<pod 1>

                   exp2 exp1 mstr
Clock Pod 1 < xxxx MLKJ MLKJ MLKJ >

*/

int validate_sampleperiod(uint32 p);
int print_card_model(int id, struct hp_cards *h);
long int get_datsize(char *hdr);
uint32 swap32(uint32 in);
uint64 swap64(uint64 in);
uint16 swap16(uint16 in);
struct section *find_section(char *name, struct hp_block_hdr *blk );
void show_sections(struct hp_block_hdr *blk );
void config_show_label(struct labels *l, FILE *out);
void config_show_labelmaps(struct section *sec, FILE *out);
struct hp_block_hdr *read_block(char *cfname);
struct section *parse_config( char *cfname, char *name, int mode);
void swap_analyzer_bytes(struct analyzer_data *a);
void swapbytes(struct data_preamble *p);
void show_analyzer(struct analyzer_data *a);
void show_pre(struct data_preamble *p);
char *get_trace_start(struct data_preamble *p);
void search_state(int pod, uint16 clk, uint16 clkmask, uint16 state, uint16 mask, int mode,struct data_preamble *pre);
int number_of_pods_assigned(uint32 pods);
uint32 valid_rows(int pod_no, uint32 pods, uint32 *rows);
int get_next_datarow(struct data_preamble *p, char *buf);
uint32 put_data_to_file(struct data_preamble *p, char *fname);
void print_data(struct data_preamble *p);
struct data_preamble *parse_data( char *cfname, char *out, int mode);
struct signal_data *add_signal(struct signal_data *d, int pol, int ena, int bits, int lsb, int msb, char *name);
struct signal_data *show_vcd_label(char *a, struct labels *l);
struct signal_data *show_la2vcd(struct data_preamble *pre, struct section *sec, int mode);

#endif
