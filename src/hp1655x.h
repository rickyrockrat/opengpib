/** \file ******************************************************************
\n\b File:        hp1655x.h
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        11/15/2013 11:52 am
\n\b Description: Header for Routines specific to hp1655x logic Analyzer cards.
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
#ifndef _HP1655X_H_
#define _HP1655X_H_ 1
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

#define CLK_START 0
#define POD_START 2
#define POD_ARRAYSIZE 10 /**don't change this - it's set by the hp config data  */

#define HEADER_SIZE 10 /** length of preamble before block starts.  */

struct section_hdr { /** 16 bytes*/
	char name[11]; /**the last byte is reserved, but usally 0  */
	uint8_t module_id; /**32 decimal for the HP16550A  */
	uint32_t block_len; /**lsb is most significant byte, i.e 0x100 is stored as 10 0, not 0 10  */
}__attribute__((__packed__));	

struct rtc_data {
	uint16_t year; /**RTC=year-1990  */
	uint8_t month; /**manual says 2 bytes, but it appears to be a misprint  */
	uint8_t day;
	uint8_t dow;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
}__attribute__((__packed__));	
struct analyzer_data_hp16550 { /**40 bytes   */
	uint8_t data_mode; /**According to HP16550A manual (Not HP16555) this is 1 byte and:
	-1=off,
	0=state data no tags 
	1=state data, each chip assigned to machine, 2k + tags
	2=state data, unassigned pod to store tag data, 4k
	8=state data at half channel 8k, no tags
	10=conventional timing full channel
	11=transitional timing full channel
	12=glitch timing
	13=conventional timing half channel
	14=transitional timing half channel*/
	uint8_t unused0;
	uint16_t pods; /**bit 21-clock pod, bit 1-12=pod 1-12, all other bits unused.  
	HP16550A manual: bit 15-13 unused,12-0 are pods 12-0, 1 indicated pod is assigned to this analyzer */
	uint8_t chipno; /**which chip is master when a pod is unassigned  */
	uint8_t masterchip;  /**Master chip for this analyzer 5=master-pod1&2,4=master-pod3&4,3=master-pod5&6,2-expansion-pod1&2,1=exp-pod3&4,0-ex-pod5&5,-1=none  */
	uint8_t unused1[6];	/**6 bytes unused  */
	uint64_t sampleperiod; /**in nano seconds  */
	uint64_t unused2;
	uint8_t tag_type; /**0-off,1=time, 2=state tags.  */
	uint8_t unused3;
	uint64_t trig_off; /**8 bytes - A decimal integer representing the time offset in picoseconds from
when this analyzer is triggered and when this analyzer provides an output
trigger to the IMB or port out. The value for one analyzer is always zero and
the value for the other analyzer is the time between the triggers of the two
analyzers.  */
	uint16_t unused4;
}__attribute__((__packed__));	

struct analyzer_data {
	uint32_t data_mode; /**According to HP16550A manual (Not HP16555) this is 1 byte and:
	-1=off,
	0=state data no tags 
	1=state data, each chip assigned to machine, 2k + tags
	2=state data, unassigned pod to store tag data, 4k
	8=state data at half channel 8k, no tags
	10=conventional timing full channel
	11=transitional timing full channel
	12=glitch timing
	13=conventional timing half channel
	14=transitional timing half channel*/
	uint32_t pods; /**bit 21-clock pod, bit 1-12=pod 1-12, all other bits unused.  
	HP16550A manual: bit 15-13 unused,12-0 are pods 12-0, 1 indicated pod is assigned to this analyzer */
	uint32_t masterchip; /**which chip is master when a pod is unassigned  */
	uint32_t maxmem;
	uint32_t unused1;
	uint64_t sampleperiod;
	uint32_t tag_type; /**0-off,1=time, 2=state tags.  */
	uint64_t trig_off;
	uint8_t unused2[30];
}__attribute__((__packed__));	

/**  
	uint32_t data_pod4_hi;
	uint32_t data_pod3_hi;
	uint32_t data_pod2_hi;
	uint32_t data_pod1_hi;
	uint32_t data_pod4_mid;
	uint32_t data_pod3_mid;
	uint32_t data_pod2_mid;
	uint32_t data_pod1_mid;
	uint32_t data_pod4_master;
	uint32_t data_pod3_master;
	uint32_t data_pod2_master;
	uint32_t data_pod1_master;
	uint8_t unused2[40];
	uint32_t trig_pod4_hi;
	uint32_t trig_pod3_hi;
	uint32_t trig_pod2_hi;
	uint32_t trig_pod1_hi;
	uint32_t trig_pod4_mid;
	uint32_t trig_pod3_mid;
	uint32_t trig_pod2_mid;
	uint32_t trig_pod1_mid;
	uint32_t trig_pod4_master;
	uint32_t trig_pod3_master;
	uint32_t trig_pod2_master;
	uint32_t trig_pod1_master;
*/
/** use same struct section_header... - this starts on byte 17.*/
struct data_preamble_hp16550 {
	uint16_t instid;   /**16550 says this is 2 bytes and is always 16500 decimal for HP16550A  */
	uint8_t rev_code; /**1 byte  */
	uint8_t chips;		 /** 1 byte  */
	struct analyzer_data_hp16550 a1; /**40 bytes  */
	struct analyzer_data_hp16550 a2;
	uint8_t unused1[2];/**offset 101  */
	
	/** 103 */
	uint16_t valid_erows[6];/**5=pod6,0=pod1 number of valid rows of data expansion card data */
	uint16_t valid_mrows[6];/**5=pod6,0=pod1 number of valid rows of data master card data */
	/**this is the offset into the data where the triger point is. Data prior to this point is pre-trigger data.  */
	uint16_t valid_trig_erows[6];/**5=pod6,0=pod1 number of valid rows of trigger point data expansion card data */
	uint16_t valid_trig_mrows[6];/**5=pod6,0=pod1 number of valid rows of trigger point data master card data */
	uint8_t unused2[24];
	/**data grouped in 14-byte rows (single) or 26-byte rows for 2-card analyzer.  */
	/**clock data is xxxx xxxx xxPN MLKJ  */
	uint8_t row1[14]; /**clock top two bytes pod6 next 2 bytes...pod1  */
	uint8_t row2[26]; /**clock top two bytes pod12 next 2...pod1  */
	char *data;	 /**origial section data  */
	uint32_t data_sz;	/**and section data size  */
}__attribute__((__packed__));	
struct data_preamble {
	uint32_t instid;   /**16550 says this is 2 bytes and is always 16500 decimal for HP16550A  */
	uint32_t rev_code; /**1 byte  */
	uint32_t chips;		 /** 1 byte  */
	uint32_t analyzer_id;
	struct analyzer_data a1;
	struct analyzer_data a2;
	uint8_t unused1[40];
	/**number of valid rows of data... offset 173 */
	uint32_t data_hi[4];/**0=pod4,3=pod1  */
	uint32_t data_mid[4];
	uint32_t data_master[4];
	/**trigger position?? offset 261 */
	uint8_t unused2[40];
	uint32_t trig_hi[4];
	uint32_t trig_mid[4];
	uint32_t trig_master[4];
	/** offset 349  */
	uint8_t unused3[234];
	/**  acquision time*/
	
	struct rtc_data rtc;
	char *data;	 /**origial section data  */
	uint32_t data_sz;	/**and section data size  */
}__attribute__((__packed__));	

struct block_spec {
	uint8_t blockstart[2]; /**should be #8  */
	char blocklen_data[8];	 /**decimal len of block, stored in ascii  */
	uint32_t blocklen;
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
	uint32_t sz;
	uint32_t off;
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
	uint8_t unknown1[3];
	uint8_t pod_info0; /**pod_info0 and 1 always seem to be equal to each other No pods=0 */
	uint8_t unknown2[3];
	uint8_t pod_info1;
	uint8_t unknown3;
	uint8_t pod_info2; /**this is 0x06 with no pods assigned to either analyzer, 0x66 if pod1/2, 0x78 if pod 3/4, 0x1E if pod1,2,3,4.  */
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
	uint8_t unknown3[4];
	uint8_t mode;	/**0=off,1=timing,2=state,3=statecompare or SPA  */
	uint8_t unknown4[7];
	uint8_t mode2;	 /*0=fullchannel, 1=half channel*/
	uint8_t unknown5[8];
	struct pod_assignment assign;
	uint8_t un[22];
}__attribute__((__packed__));	

struct config_data {
	struct machine_config m[2];
}__attribute__((__packed__));	

#define LABEL_RECORD_LEN 22
#define LABEL_MAP_LEN 12

struct label_map {
	/**offset 0= hi byte.  */
	uint8_t clk_pods[POD_ARRAYSIZE]; /**0 and 1 index are clk, 2 idx is hi byte of pod4  */
	uint8_t unknown[2];
}__attribute__((__packed__));	

struct labels {
	char name[6];
	uint8_t unknown1[3];
	uint8_t polarity;
	uint8_t unknown2;
	uint8_t strange_offset;
	uint16_t strange_offsetlo;
	uint8_t unknown3[4];
	uint8_t bits;
	uint8_t enable;
	uint8_t sequence;
	uint32_t actual_offset;
	struct label_map map;
}__attribute__((__packed__));	

struct one_card_data {
	uint16_t clkunused;
	uint8_t pdata[POD_ARRAYSIZE]; /**clkhi is 0, clklo is 1, pod 4 is 2  */
}__attribute__((__packed__));	
struct two_card_data {
	uint32_t clk;
	uint16_t epods[4];
	uint16_t mpods[4];
}__attribute__((__packed__));	

struct three_card_data {
	uint32_t clk;
	uint16_t eh_pods[4];
	uint16_t el_pods[4];
	uint16_t mpods[4];
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

/**logic analyzer functions  */
int validate_sampleperiod(uint32_t p);

long int get_datsize(char *hdr);
uint32_t swap32(uint32_t in);
uint64_t swap64(uint64_t in);
uint16_t swap16(uint16_t in);
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
void search_state(int pod, uint16_t clk, uint16_t clkmask, uint16_t state, uint16_t mask, int mode,struct data_preamble *pre);
int number_of_pods_assigned(uint32_t pods);
uint32_t valid_rows(int pod_no, uint32_t pods, uint32_t *rows);
int get_next_datarow(struct data_preamble *p, char *buf);
uint32_t put_data_to_file(struct data_preamble *p, char *fname);
void print_data(struct data_preamble *p);
struct data_preamble *parse_data( char *cfname, char *out, int mode);
struct signal_data *add_signal(struct signal_data *d, int pol, int ena, int bits, int lsb, int msb, char *name);
struct signal_data *show_vcd_label(char *a, struct labels *l);
struct signal_data *show_la2vcd(struct data_preamble *pre, struct section *sec, int mode);

#endif

