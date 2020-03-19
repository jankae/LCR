#ifndef IOX_AD5940_AD5940_H_
#define IOX_AD5940_AD5940_H_

#include <stdbool.h>
#include "stm.h"
#include "FreeRTOS.h"
#include "semphr.h"

// If the SPI interface is shared across multiple instances or other modules, this mutex is required
//#define AD5940_USE_SPI_MUTEX
#ifdef AD5940_USE_SPI_MUTEX
#define AD5940_SPI_MUTEX		spiMutexHandle
#endif

// If an instance of ad5940_t will be accessed by multiple tasks, this mutex is required
#define AD5940_USE_MUTEX

/***************************************
 * Don't change anything below this line
 **************************************/

typedef enum {
	// Configuration registers
	AD5940_REG_AFECON = 		0x2000,
	AD5940_REG_PMBW = 			0x22F0,
	// Identification registers
	AD5940_REG_ADIID = 			0x0400,
	AD5940_REG_CHIPID = 		0x0404,
	// Low power DAC circuit registers
	AD5940_REG_LPDACCON0 = 		0x2128,
	AD5940_REG_LPDACSW0 = 		0x2124,
	AD5940_REG_LPREFBUFCON = 	0x2050,
	AD5940_REG_SWMUX = 			0x235C,
	AD5940_REG_LPDACDAT0 = 		0x2120,
	// Low power TIA circuits registers
	AD5940_REG_LPTIASW0 = 		0x20E4,
	AD5940_REG_LPTIACON0 = 		0x20EC,
	// High speed DAC circuit registers
	AD5940_REG_HSDACCON = 		0x2010,
	AD5940_REG_HSDACDAT = 		0x2048,
	// High speed TIA circuit registers
	AD5940_REG_HSRTIACON = 		0x20F0,
	AD5940_REG_DE0RESCON = 		0x20F8,
	AD5940_REG_HSTIACON = 		0x20FC,
	// ADC circuit registers
	AD5940_REG_ADCFILTERCON = 	0x2044,
	AD5940_REG_ADCDAT = 		0x2074,
	AD5940_REG_DFTREAL = 		0x2078,
	AD5940_REG_DFTIMAG = 		0x207C,
	AD5940_REG_SINC2DAT = 		0x2080,
	AD5940_REG_TEMPSENSDAT = 	0x2084,
	AD5940_REG_DFTCON = 		0x20D0,
	AD5940_REG_TEMPSENS = 		0x2174,
	AD5940_REG_ADCCON = 		0x21A8,
	AD5940_REG_REPEATADCCNV = 	0x21F0,
	AD5940_REG_ADCBUFCON = 		0x238C,
	// ADC calibration registers
	AD5940_REG_CALDATLOCK = 	0x2230,
	AD5940_REG_ADCOFFSETLPTIA = 0x2288,
	AD5940_REG_ADCGNLPTIA = 	0x228C,
	AD5940_REG_ADCOFFSETHSTIA = 0x2234,
	AD5940_REG_ADCGAINHSTIA	= 	0x2284,
	AD5940_REG_ADCOFFSETGN1 = 	0x2244,
	AD5940_REG_ADCGAINGN1 = 	0x2240,
	AD5940_REG_ADCOFFSETGN1P5 = 0x22CC,
	AD5940_REG_ADCGAINGN1P5 = 	0x2270,
	AD5940_REG_ADCOFFSETGN2 = 	0x22C8,
	AD5940_REG_ADCGAINGN2 = 	0x2274,
	AD5940_REG_ADCOFFSETGN4 = 	0x22D4,
	AD5940_REG_ADCGAINGN4 = 	0x2278,
	AD5940_REG_ADCOFFSETGN9 = 	0x22D0,
	AD5940_REG_ADCGAINGN9 = 	0x2298,
	AD5940_REG_ADCOFFSETTEMPSENS = 0x223C,
	AD5940_REG_ADCGAINTEMPSENS = 0x2238,
	// ADC digital postprocessing registers
	AD5940_REG_ADCMIN = 		0x20A8,
	AD5940_REG_ADCMINSM = 		0x20AC,
	AD5940_REG_ADCMAX = 		0x20B0,
	AD5940_REG_ADCMAXSMEN = 	0x20B4,
	AD5940_REG_ADCDELTA = 		0x20B8,
	// ADC statistics registers
	AD5940_REG_STATSVAR = 		0x21C0,
	AD5940_REG_STATSCON = 		0x21C4,
	AD5940_REG_STATSMEAN = 		0x21C8,
	// Programmable switches registers
	AD5940_REG_SWCON = 			0x200C,
	AD5940_REG_DSWFULLCON = 	0x2150,
	AD5940_REG_NSWFULLCON = 	0x2154,
	AD5940_REG_PSWFULLCON = 	0x2158,
	AD5940_REG_TSWFULLCON = 	0x215C,
	AD5940_REG_DSWSTA = 		0x21B0,
	AD5940_REG_PSWSTA = 		0x21B4,
	AD5940_REG_NSWSTA = 		0x21B8,
	AD5940_REG_TSWSTA = 		0x21BC,
	// Sequence and fifo registers
	AD5940_REG_SEQCON = 		0x2004,
	AD5940_REG_FIFOCON = 		0x2008,
	AD5940_REG_SEQCRC = 		0x2060,
	AD5940_REG_SEQCNT = 		0x2064,
	AD5940_REG_SEQTIMEOUT = 	0x2068,
	AD5940_REG_DATAFIFORD = 	0x206C,
	AD5940_REG_CMDFIFOWRITE = 	0x2070,
	AD5940_REG_SEQSLPLOCK = 	0x2118,
	AD5940_REG_SEQTRGSLP = 		0x211C,
	AD5940_REG_SEQ0INFO = 		0x21CC,
	AD5940_REG_SEQ2INFO = 		0x21D0,
	AD5940_REG_CMDFIFOWADDR = 	0x21D4,
	AD5940_REG_CMDDATACON = 	0x21D8,
	AD5940_REG_DATAFIFOTHRES = 	0x21E0,
	AD5940_REG_SEQ3INFO = 		0x21E4,
	AD5940_REG_SEQ1INFO = 		0x21E8,
	AD5940_REG_FIFOCNTSTA = 	0x2200,
	AD5940_REG_SYNCEXTDEVICE = 	0x2054,
	AD5940_REG_TRIGSEQ = 		0x0430,
	// Waveform generator registers
	AD5940_REG_WGCON = 			0x2014,
	AD5940_REG_WGDCLEVEL1 = 	0x2018,
	AD5940_REG_WGDCLEVEL2 = 	0x201C,
	AD5940_REG_WGDELAY1 = 		0x2020,
	AD5940_REG_WGSLOPE1 = 		0x2024,
	AD5940_REG_WGDELAY2 = 		0x2028,
	AD5940_REG_WGSLOPE2 = 		0x202C,
	AD5940_REG_WGFCW = 			0x2030,
	AD5940_REG_WGPHASE = 		0x2034,
	AD5940_REG_WGOFFSET = 		0x2038,
	AD5940_REG_WGAMPLITUDE = 	0x203C,
	// Sleep and wakeup timer registers
	AD5940_REG_CON = 			0x0800,
	AD5940_REG_SEQORDER = 		0x0804,
	AD5940_REG_SEQ0WUPL = 		0x0808,
	AD5940_REG_SEQ0WUPH = 		0x080C,
	AD5940_REG_SEQ0SLEEPL = 	0x0810,
	AD5940_REG_SEQ0SLEEPH = 	0x0814,
	AD5940_REG_SEQ1WUPL = 		0x0818,
	AD5940_REG_SEQ1WUPH = 		0x081C,
	AD5940_REG_SEQ1SLEEPL = 	0x0820,
	AD5940_REG_SEQ1SLEEPH = 	0x0824,
	AD5940_REG_SEQ2WUPL = 		0x0828,
	AD5940_REG_SEQ2WUPH = 		0x082C,
	AD5940_REG_SEQ2SLEEPL = 	0x0830,
	AD5940_REG_SEQ2SLEEPH = 	0x0834,
	AD5940_REG_SEQ3WUPL = 		0x0838,
	AD5940_REG_SEQ3WUPH = 		0x083C,
	AD5940_REG_SEQ3SLEEPL = 	0x0840,
	AD5940_REG_SEQ3SLEEPH = 	0x0844,
	AD5940_REG_TMRCON = 		0x0A1C,
	// interrupt registers
	AD5940_REG_INTCPOL = 		0x3000,
	AD5940_REG_INTCCLR = 		0x3004,
	AD5940_REG_INTCSEL0 = 		0x3008,
	AD5940_REG_INTCSEL1 = 		0x300C,
	AD5940_REG_INTCFLAG0 = 		0x3010,
	AD5940_REG_INTCFLAG1 = 		0x3014,
	AD5940_REG_AFEGENINTSTA = 	0x209C,
	// External interrupt configuration registers
	AD5940_REG_EI0CON = 		0x0A20,
	AD5940_REG_EI1CON = 		0x0A24,
	AD5940_REG_EI2CON = 		0x0A28,
	AD5940_REG_EICLR = 			0x0A30,
	// GPIO registers
	AD5940_REG_GP0CON = 		0x0000,
	AD5940_REG_GP0OEN = 		0x0004,
	AD5940_REG_GP0PE = 			0x0008,
	AD5940_REG_GP0IEN = 		0x000C,
	AD5940_REG_GP0IN = 			0x0010,
	AD5940_REG_GP0OUT = 		0x0014,
	AD5940_REG_GP0SET = 		0x0018,
	AD5940_REG_GP0CLR = 		0x001C,
	AD5940_REG_GP0TGL = 		0x0020,
	// analog die reset registers
	AD5940_REG_RSTCONKEY = 		0x0A5C,
	AD5940_REG_SWRSTCON = 		0x0424,
	AD5940_REG_RSTSTA = 		0x0A40,
	// Power modes registers
	AD5940_REG_PWRMOD = 		0x0A00,
	AD5940_REG_PWRKEY = 		0x0A04,
	AD5940_REG_LPMODEKEY = 		0x210C,
	AD5940_REG_LPMODECLKSEL = 	0x2110,
	AD5940_REG_LPMODECON = 		0x2114,
	// Clock architecture registers
	AD5940_REG_CLKCON0KEY = 	0x0420,
	AD5940_REG_CLKCON0 = 		0x0408,
	AD5940_REG_CLKSEL = 		0x0414,
	AD5940_REG_CLKEN0 = 		0x0A70,
	AD5940_REG_CLKEN1 = 		0x0410,
	AD5940_REG_OSCKEY = 		0x0A0C,
	AD5940_REG_OSCCON = 		0x0A10,
	AD5940_REG_HSOSCCON = 		0x20BC,
	//AD5940_REG_RSTCONKEY = 		0x0A5C, // already listed in analog die reset registers
	AD5940_REG_LOSCTST =		0x0A6C,
} ad5940_reg_t;

typedef enum {
	AD5940_RES_OK = 0,
	AD5940_RES_ERROR = 1,
} ad5940_result_t;

typedef enum {
	AD5940_ELECTRODE_2WIRE,
	AD5940_ELECTRODE_3WIRE,
	AD5940_ELECTRODE_DISCONNECTED
} ad5940_electrode_mode_t;

typedef enum {
	AD5940_TIA_RL_0 = 0x0000,
	AD5940_TIA_RL_10 = 0x0400,
	AD5940_TIA_RL_30 = 0x0800,
	AD5940_TIA_RL_50 = 0x0C00,
	AD5940_TIA_RL_100 = 0x1000,
	AD5940_TIA_RL_1600 = 0x1400,
	AD5940_TIA_RL_3100 = 0x1800,
	AD5940_TIA_RL_3600 = 0x1C00,
} ad5940_tia_rl_t;

typedef enum {
	AD5940_TIA_RF_OPEN = 0x0000,
	AD5940_TIA_RF_0 = 0x2000,
	AD5940_TIA_RF_20K = 0x4000,
	AD5940_TIA_RF_100K = 0x6000,
	AD5940_TIA_RF_200K = 0x8000,
	AD5940_TIA_RF_400K = 0xA000,
	AD5940_TIA_RF_600K = 0xC000,
	AD5940_TIA_RF_1M = 0xE000,
} ad5940_tia_rf_t;

typedef enum {
	AD5940_TIA_GAIN_OPEN = 0x0000,
	AD5940_TIA_GAIN_200 = 0x0020,
	AD5940_TIA_GAIN_1K = 0x0040,
	AD5940_TIA_GAIN_2K = 0x0060,
	AD5940_TIA_GAIN_3K = 0x0080,
	AD5940_TIA_GAIN_4K = 0x00A0,
	AD5940_TIA_GAIN_6K = 0x00C0,
	AD5940_TIA_GAIN_8K = 0x00E0,
	AD5940_TIA_GAIN_10K = 0x0100,
	AD5940_TIA_GAIN_12K = 0x0120,
	AD5940_TIA_GAIN_16K = 0x0140,
	AD5940_TIA_GAIN_20K = 0x0160,
	AD5940_TIA_GAIN_24K = 0x0180,
	AD5940_TIA_GAIN_30K = 0x01A0,
	AD5940_TIA_GAIN_32K = 0x01C0,
	AD5940_TIA_GAIN_40K = 0x01E0,
	AD5940_TIA_GAIN_48K = 0x0200,
	AD5940_TIA_GAIN_64K = 0x0220,
	AD5940_TIA_GAIN_85K = 0x0240,
	AD5940_TIA_GAIN_96K = 0x0260,
	AD5940_TIA_GAIN_100K = 0x0280,
	AD5940_TIA_GAIN_120K = 0x02A0,
	AD5940_TIA_GAIN_128K = 0x02C0,
	AD5940_TIA_GAIN_160K = 0x02E0,
	AD5940_TIA_GAIN_196K = 0x0300,
	AD5940_TIA_GAIN_256K = 0x0320,
	AD5940_TIA_GAIN_512K = 0x0340,
} ad5940_tia_gain_t;

static const ad5940_tia_gain_t ad5940_tia_gain_default = AD5940_TIA_GAIN_10K;

typedef enum {
	AD5940_ADC_MUXN_FLOATING = 0x0000,
	AD5940_ADC_MUXN_HSTIAN = 0x0100,
	AD5940_ADC_MUXN_LPTIAN = 0x0200,
	AD5940_ADC_MUXN_AIN0 = 0x0400,
	AD5940_ADC_MUXN_AIN1 = 0x0500,
	AD5940_ADC_MUXN_AIN2 = 0x0600,
	AD5940_ADC_MUXN_AIN3 = 0x0700,
	AD5940_ADC_MUXN_VBIAS_CAP = 0x0800,
	AD5940_ADC_MUXN_TEMPSENN = 0x0B00,
	AD5940_ADC_MUXN_AIN4 = 0x0C00,
	AD5940_ADC_MUXN_AIN6 = 0x0E00,
	AD5940_ADC_MUXN_VZERO_PIN = 0x1000,
	AD5940_ADC_MUXN_VBIAS_PIN = 0x1001,
	AD5940_ADC_MUXN_EXAMPN = 0x1400,
} ad5940_adc_muxn_t;

typedef enum {
	AD5940_ADC_MUXP_FLOATING = 0x0000,
	AD5940_ADC_MUXP_HSTIAP = 0x0001,
	AD5940_ADC_MUXP_LPTIAPLP = 0x0002,
	AD5940_ADC_MUXP_AIN0 = 0x0004,
	AD5940_ADC_MUXP_AIN1 = 0x0005,
	AD5940_ADC_MUXP_AIN2 = 0x0006,
	AD5940_ADC_MUXP_AIN3 = 0x0007,
	AD5940_ADC_MUXP_AVDD_2 = 0x0008,
	AD5940_ADC_MUXP_DVDD_2 = 0x0009,
	AD5940_ADC_MUXP_AVDD_REG_2 = 0x000A,
	AD5940_ADC_MUXP_TEMPSENP = 0x000B,
	AD5940_ADC_MUXP_VBIAS_CAP = 0x000C,
	AD5940_ADC_MUXP_DE0_PIN = 0x000D,
	AD5940_ADC_MUXP_SE0_PIN = 0x000E,
	AD5940_ADC_MUXP_VREF_2V5_2 = 0x0010,
	AD5940_ADC_MUXP_VREF_1V82 = 0x0012,
	AD5940_ADC_MUXP_TEMPSENN = 0x0013,
	AD5940_ADC_MUXP_AIN4 = 0x0014,
	AD5940_ADC_MUXP_AIN6 = 0x0016,
	AD5940_ADC_MUXP_VZERO_PIN = 0x0017,
	AD5940_ADC_MUXP_VBIAS_PIN = 0x0018,
	AD5940_ADC_MUXP_CE0_PIN = 0x0019,
	AD5940_ADC_MUXP_RE0_PIN = 0x001A,
	AD5940_ADC_MUXP_VCE0_2 = 0x001F,
	AD5940_ADC_MUXP_LPTIAP = 0x0021,
	AD5940_ADC_MUXP_AGND_REF = 0x0023,
	AD5940_ADC_MUXP_EXAMPP = 0x0024,
} ad5940_adc_muxp_t;

typedef enum {
	AD5940_PGA_GAIN_1 = 0x0000,
	AD5940_PGA_GAIN_1_5 = 0x10000,
	AD5940_PGA_GAIN_2 = 0x20000,
	AD5940_PGA_GAIN_4 = 0x30000,
	AD5940_PGA_GAIN_9 = 0x40000,
} ad5940_pga_gain_t;

typedef enum {
	AD5940_DFTSRC_DISABLED,
	AD5940_DFTSRC_SINC2,
	AD5940_DFTSRC_SINC3,
	AD5940_DFTSRC_RAW,
	AD5940_DFTSRC_AVG
} ad5940_dftsource_t;

typedef struct {
	// any power of 2 from 4 to 16384
	uint16_t points;
	bool hanning;
	ad5940_dftsource_t source;
} ad5940_dftconfig_t;

typedef enum {
	AD5940_WAVE_SINE,
	AD5940_WAVE_TRAPEZOID,
	AD5940_WAVE_DC,
} ad5940_wavetype_t;

typedef struct {
	ad5940_wavetype_t type;
	union {
		// TODO support other options beside sine
		struct{
			uint32_t frequency; // in mHz
			int32_t phaseoffset; // in mdegrees
			int32_t offset; // in uV
			uint32_t amplitude; // in uV
		} sine;
	};
} ad5940_waveinfo_t;

// enum values correspond to the bitmask for the D switch in SWCON register
typedef enum {
	AD5940_EXAMP_DSW_NONE = 0x00,
	AD5940_EXAMP_DSW_AIN1 = 0x02, // Switch D2
	AD5940_EXAMP_DSW_AIN2 = 0x03, // Switch D3
	AD5940_EXAMP_DSW_AIN3 = 0x04, // Switch D4
	AD5940_EXAMP_DSW_CE0 = 0x05, // Switch D5
	AD5940_EXAMP_DSW_AFE1 = 0x06, // Switch D6
	AD5940_EXAMP_DSW_SE0 = 0x07, // Switch D7
	AD5940_EXAMP_DSW_AFE3 = 0x08, // Switch D8
	AD5940_EXAMP_DSW_RCAL0 = 0x01, // Switch DR0
} ad5940_ex_amp_dsw_t;

// enum values correspond to the bitmask for the p switch in SWCON register
typedef enum {
	AD5940_EXAMP_PSW_NONE = 0x00F0, // All switches open
	AD5940_EXAMP_PSW_INT_FEEDBACK = 0x0000, // Switch PL
	AD5940_EXAMP_PSW_RCAL0 = 0x0010, // Switch PR0
	AD5940_EXAMP_PSW_AIN1 = 0x0020, // Switch P2
	AD5940_EXAMP_PSW_AIN2 = 0x0030, // Switch P3
	AD5940_EXAMP_PSW_AIN3 = 0x0040, // Switch P4
	AD5940_EXAMP_PSW_RE0 = 0x0050, // Switch P5
	AD5940_EXAMP_PSW_AFE2 = 0x0060, // Switch P6
	AD5940_EXAMP_PSW_SE0 = 0x0070, // Switch P7
	AD5940_EXAMP_PSW_DE0 = 0x0080, // Switch P8
	AD5940_EXAMP_PSW_AFE3 = 0x0090, // Switch P9
	AD5940_EXAMP_PSW_CE0 = 0x00B0, // Switch P11
	/* The bit for switch P12 is reserved according to datasheet. Might be a typo in datasheet?
	 * Bit meaning is otherwise pretty consistent to switch number and datasheet does not say how
	 * to close P12 otherwise.
	 */
	AD5940_EXAMP_PSW_AFE1 = 0x00C0, // Switch P12
	AD5940_EXAMP_PSW_DVDD = 0x00D0, // Switch PL2
} ad5940_ex_amp_psw_t;

// enum values correspond to the bitmask for the n switch in SWCON register
typedef enum {
	AD5940_EXAMP_NSW_NONE = 0x0F00, // All switches open
	AD5940_EXAMP_NSW_INT_FEEDBACK = 0x0000, // Switch NL
	AD5940_EXAMP_NSW_AIN0 = 0x0100, // Switch N1
	AD5940_EXAMP_NSW_AIN1 = 0x0200, // Switch N2
	AD5940_EXAMP_NSW_AIN2 = 0x0300, // Switch N3
	AD5940_EXAMP_NSW_AIN3 = 0x0400, // Switch N4
	AD5940_EXAMP_NSW_SE0_RLOAD = 0x0500, // Switch N5
	AD5940_EXAMP_NSW_DE0 = 0x0600, // Switch N6
	AD5940_EXAMP_NSW_AFE3 = 0x0700, // Switch N7
	AD5940_EXAMP_NSW_SE0_DIRECT = 0x0900, // Switch N9
	AD5940_EXAMP_NSW_RCAL1 = 0x0A00, // Switch NR1
	AD5940_EXAMP_NSW_DVDD = 0x0B00, // Switch NL2
} ad5940_ex_amp_nsw_t;

typedef enum {
	AD5940_HSRTIA_200 = 0x00,
	AD5940_HSRTIA_1K = 0x01,
	AD5940_HSRTIA_5K = 0x02,
	AD5940_HSRTIA_10K = 0x03,
	AD5940_HSRTIA_20K = 0x04,
	AD5940_HSRTIA_40K = 0x05,
	AD5940_HSRTIA_80K = 0x06,
	AD5940_HSRTIA_160K = 0x07,
	AD5940_HSRTIA_OPEN = 0x08
} ad5940_hsrtia_t;

typedef enum {
	AD5940_HSTIA_VBIAS_1V1 = 0x00,
	AD5940_HSTIA_VBIAS_VZERO = 0x01,
} ad5940_HSTIA_VBIAS_t;

// enum values correspond to the bitmask for the T switch in SWCON
typedef enum {
	AD5940_HSTSW_NONE = 0x0000, // All switches open
	AD5940_HSTSW_SE0 = 0x5000, // Switch T5
	AD5940_HSTSW_AIN0 = 0x1000, // Switch T1
	AD5940_HSTSW_AIN1 = 0x2000, // Switch T2
	AD5940_HSTSW_AIN2 = 0x3000, // Switch T3
	AD5940_HSTSW_AIN3 = 0x4000,  // Switch T4, could actually be AIN4, datasheet is inconsistent
	AD5940_HSTSW_RCAL1 = 0x8000,	// Switch TR1
	AD5940_HSTSW_DE0_DIRECT = 0x6000, // Switch T6
	AD5940_HSTSW_AFE3 = 0x7000, // Switch T7
} ad5940_hstsw_t;

typedef struct {
	float mag, phase;
} ad5940_dftresult_t;

typedef struct {
	SPI_HandleTypeDef *spi;
	GPIO_TypeDef *CSport;
	uint16_t CSpin;
	uint8_t vzero_code;
	bool autoranging;
	struct {
		ad5940_pga_gain_t gain_current;
		ad5940_pga_gain_t gain_voltage;
		ad5940_hsrtia_t rtia;
	} impedance;
#ifdef AD5940_USE_MUTEX
	StaticSemaphore_t xMutHardwareAccess;
	xSemaphoreHandle access_mutex;
#endif
} ad5940_t;

typedef enum {
	AD5940_FIFOSRC_ADC = 0x00,
	AD5940_FIFOSRC_DFT = 0x02,
	AD5940_FIFOSRC_SINC2 = 0x03,
	AD5940_FIFOSRC_VARIANCE = 0x04,
	AD5940_FIFOSRC_MEAN = 0x05,
} ad5940_fifosrc_t;

typedef enum {
	AD5940_SINC2OSR_22 = 0,
	AD5940_SINC2OSR_44 = 1,
	AD5940_SINC2OSR_89 = 2,
	AD5940_SINC2OSR_178 = 3,
	AD5940_SINC2OSR_267 = 4,
	AD5940_SINC2OSR_533 = 5,
	AD5940_SINC2OSR_640 = 6,
	AD5940_SINC2OSR_667 = 7,
	AD5940_SINC2OSR_800 = 8,
	AD5940_SINC2OSR_889 = 9,
	AD5940_SINC2OSR_1067 = 10,
	AD5940_SINC2OSR_1333 = 11,
} ad5940_sinc2_osr_t;

ad5940_result_t ad5940_init(ad5940_t *a);

/*
 * Functions (mostly) related to electrode measurements
 */
ad5940_result_t ad5940_set_vzero(ad5940_t *a, uint16_t mV);
ad5940_result_t ad5940_set_electrode(ad5940_t *a, ad5940_electrode_mode_t e);
ad5940_result_t ad5940_set_vbias(ad5940_t *a, int16_t mV);
ad5940_result_t ad5940_set_TIA_lowpass(ad5940_t *a, ad5940_tia_rf_t rf);
ad5940_result_t ad5940_set_TIA_load(ad5940_t *a, ad5940_tia_rl_t rl);
ad5940_result_t ad5940_set_TIA_gain(ad5940_t *a, ad5940_tia_gain_t g);
ad5940_result_t ad5940_get_TIA_gain(ad5940_t *a, ad5940_tia_gain_t *g);
uint32_t ad5940_LPTIA_gain_to_value(ad5940_tia_gain_t g);
ad5940_tia_gain_t ad5940_value_to_LPTIA_gain(uint32_t value);
ad5940_result_t ad5940_set_ADC_mux(ad5940_t *a, ad5940_adc_muxp_t p, ad5940_adc_muxn_t n);
ad5940_result_t ad5940_set_PGA_gain(ad5940_t *a, ad5940_pga_gain_t gain);
ad5940_result_t ad5940_get_PGA_gain(ad5940_t *a, ad5940_pga_gain_t *gain);
uint8_t ad5940_PGA_gain_to_value10(ad5940_pga_gain_t g);
uint16_t ad5940_get_raw_ADC(ad5940_t *a);
ad5940_result_t ad5940_zero_ADC(ad5940_t *a);
ad5940_result_t ad5940_ADC_start(ad5940_t *a);
ad5940_result_t ad5940_set_SINC2_OSR(ad5940_t *a, ad5940_sinc2_osr_t osr);
ad5940_result_t ad5940_ADC_stop(ad5940_t *a);
//ad5940_result_t ad5940_find_optimal_gain(ad5940_t *a);
int32_t ad5940_raw_ADC_to_current(ad5940_t *a, int16_t raw);
ad5940_result_t ad5940_measure_current(ad5940_t *a, int32_t *nA);

/*
 * FIFO functions
 */
ad5940_result_t ad5940_enable_FIFO(ad5940_t *a, ad5940_fifosrc_t src);
ad5940_result_t ad5940_disable_FIFO(ad5940_t *a);
uint16_t ad5940_get_FIFO_level(ad5940_t *a);
void ad5940_FIFO_read(ad5940_t *a, uint16_t *dest, uint16_t num);


/*
 * Functions (mostly) related to impedance measurements
 */
ad5940_result_t ad5940_generate_waveform(ad5940_t *a, ad5940_waveinfo_t *w);
ad5940_result_t ad5940_set_excitation_amplifier(ad5940_t *a,
		ad5940_ex_amp_dsw_t dsw, ad5940_ex_amp_psw_t psw,
		ad5940_ex_amp_nsw_t nsw, bool dc_bias);
ad5940_result_t ad5940_disable_excitation_amplifier(ad5940_t *a);

/**
 * \brief Configures the high speed TIA
 *
 * \param rtia Selects the feedback resistor, setting the gain of the amplifier
 * \param ctia sets the feedback capacitor in pF, providing stability. Available values are 1-32pF
 * \param diode Enables the diode in the feedback path
 * \param sw selects the connected analog input pin
 */
ad5940_result_t ad5940_set_HSTIA(ad5940_t *a, ad5940_hsrtia_t rtia,
		uint8_t ctia, ad5940_HSTIA_VBIAS_t bias, bool diode, ad5940_hstsw_t sw);

ad5940_result_t ad5940_disable_HSTIA(ad5940_t *a);

uint32_t ad5940_HSTIA_gain_to_value(ad5940_hsrtia_t g);
ad5940_hsrtia_t ad5940_value_to_HSTIA_gain(uint32_t value);

ad5940_result_t ad5940_set_dft(ad5940_t *a, ad5940_dftconfig_t *dft);
ad5940_result_t ad5940_get_dft_result(ad5940_t *a, uint8_t avg, ad5940_dftresult_t *data);
ad5940_result_t ad5940_setup_four_wire(ad5940_t *a, uint32_t frequency,
		uint32_t nS_min, uint32_t nS_max, ad5940_ex_amp_dsw_t amp,
		ad5940_hstsw_t hstsw, uint16_t rseries);
ad5940_result_t ad5940_measure_four_wire(ad5940_t *a, ad5940_adc_muxp_t p_sense,
		ad5940_adc_muxn_t n_sense, uint8_t avg, ad5940_dftresult_t *data);

/*
 * \brief Disables all peripherals used in the high speed loop
 */
ad5940_result_t ad5940_disable_HS_loop(ad5940_t *a);

/*
 * Functions for using GPIOs
 */
typedef enum {
	AD5940_GPIO_DISABLED,
	AD5940_GPIO_INPUT,
	AD5940_GPIO_OUTPUT,
} ad5940_gpio_config_t;

#define AD5940_GPIO0			0x01
#define AD5940_GPIO1			0x02
#define AD5940_GPIO2			0x04
#define AD5940_GPIO3			0x08
#define AD5940_GPIO4			0x10
#define AD5940_GPIO5			0x20
#define AD5940_GPIO6			0x40
#define AD5940_GPIO7			0x80

ad5940_result_t ad5940_gpio_configure(ad5940_t *a, uint8_t gpio, ad5940_gpio_config_t config);
ad5940_result_t ad5940_gpio_set(ad5940_t *a, uint8_t gpio);
ad5940_result_t ad5940_gpio_clear(ad5940_t *a, uint8_t gpio);
uint8_t ad5940_gpio_get(ad5940_t *a, uint8_t gpio);

/* Functions for direct register access. Only use if necessary (e.i. for speed improvements if only one setting changed).
 * Enclose every register access block with ad5940_take_mutex and ad5940_release_mutex to make access thread safe.
 */
void ad5940_take_mutex(ad5940_t *a);
void ad5940_release_mutex(ad5940_t *a);
void ad5940_write_reg(ad5940_t *a, ad5940_reg_t reg, uint32_t val);
uint32_t ad5940_read_reg(ad5940_t *a, ad5940_reg_t reg);
void ad5940_set_bits(ad5940_t *a, ad5940_reg_t reg, uint32_t bits);
void ad5940_clear_bits(ad5940_t *a, ad5940_reg_t reg, uint32_t bits);
void ad5940_modify_reg(ad5940_t *a, ad5940_reg_t reg, uint32_t val,
		uint32_t mask);

#endif
