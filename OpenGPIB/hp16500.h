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
#include "gpib.h"
#ifdef LA2VCD_LIB
#include "la2vcd.h"
#else
  #warning "LA2VCD_LIB Not Defined! You loose vcd functionality"
#endif
#define SLOTNO_16500C    0 /**0 to talk to system  */
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

struct hp_cards{
	int type;
	char *model;
	char *desc;
};

struct card_info {
	struct hp_cards *info;
	int slot;
};

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
/*defines for mode2 below */
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
	int ena;  /**is signal enabled in menu  */
	int lsb;  /**lowest number bit where signal starts  */
	int msb;  /**highes number bit where signal ends  */
	int pol;  /**polarity of signal 1=normal (+), 0= inverted (-) */
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
#define COM_USE_MODE_GENERIC 0
#define COM_USE_MODE_SCOPE   1
#define COM_USE_MODE_LOGIC   2

/**generic functions  */
int get_hp_info_card(int id);
int print_card_model(int id);
int select_hp_card(int slot, struct gpib *g);
void show_known_hp_cards(void);
int hp16500_find_card(int cardtype, int no, struct gpib *g);
void show_hp_connection_info(void);
void show_common_usage (int mode);
int handle_common_opts(int c, char *optarg, struct hp_common_options *o);
/**scope functions  */
int get_trigger_source(struct gpib *g);
int oscope_parse_preamble(struct gpib *g, struct hp_scope_preamble *h);
int check_oscope_channel(char *ch);
int oscope_get_preamble(struct gpib *g,char *ch, struct hp_scope_preamble *h);
int get_oscope_data(struct gpib *g, char *ch, struct hp_scope_preamble *h);
int init_oscope_instrument(struct hp_common_options *o, struct gpib *g); 

/**logic analyzer functions  */
int validate_sampleperiod(uint32 p);

long int get_datsize(char *hdr);
uint32 swap32(uint32 in);
uint64 swap64(uint64 in);
uint16 swap16(uint16 in);
struct section *find_section(char *name, struct hp_block_hdr *blk );
void show_sections(struct hp_block_hdr *blk );
void config_show_label(char *a,struct labels *l, FILE *out);
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
