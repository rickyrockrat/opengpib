#ifndef _HP16500_H_
#define _HP16500_H_ 1
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
#endif
