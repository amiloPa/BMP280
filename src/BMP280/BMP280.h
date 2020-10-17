/*
 * BMP280.h
 *
 *  Created on: 15.10.2020
 *      Author: Piotr
 */

#ifndef BMP280_BMP280_H_
#define BMP280_BMP280_H_

#include "stm32f10x.h"
#include "../I2C/I2C.h"

//select protocol
#define BMP280_SPI	0
#define BMP280_I2C	1

#define USE_STRING 1

#define BMP280_ADDR 		0xEC	// Sensor addres -> SDO pin is connected to GND
//#define BMP280_ADDR 		0xEE	// Sensor addres -> SDO pin is connected to VCC

// Pressure oversampling ->  osrs_p[2:0] -> addres register 0xF4 bits: 2,3,4
#define BMP280_SKIPPED			0	// Pressure oversampling skipped 	-> resolution 0
#define BMP280_ULTRALOWPOWER	1	// Pressure oversampling x1			-> resolution 16
#define BMP280_LOWPOWER			2	// Pressure oversampling x2			-> resolution 17
#define BMP280_STANDARD			3	// Pressure oversampling x4			-> resolution 18
#define BMP280_HIGHRES			4	// Pressure oversampling x8			-> resolution 19
#define BMP280_ULTRAHIGHRES		5	// Pressure oversampling x16		-> resolution 20

// Temperature resolution -> osrs_t[2:0] -> addres register 0xF4 bits: 5,6,7
#define BMP280_TEMPERATURE_16BIT 1	//Temperature oversampling x 1
#define BMP280_TEMPERATURE_17BIT 2	//Temperature oversampling x 2
#define BMP280_TEMPERATURE_18BIT 3	//Temperature oversampling x 4
#define BMP280_TEMPERATURE_19BIT 4	//Temperature oversampling x 8
#define BMP280_TEMPERATURE_20BIT 5	//Temperature oversampling x 16

// IIR filter ->  filter[2:0]  -> addres register 0xF5 bits: 2, 3, 4
#define BMP280_FILTER_OFF	0	// Filter OFF
#define BMP280_FILTER_X2 	1	// Filter coefficient 2
#define BMP280_FILTER_X4 	2	// Filter coefficient 4
#define BMP280_FILTER_X8	3	// Filter coefficient 8
#define BMP280_FILTER_X16 	4	// Filter coefficient 16

// Mode -> mode[1:0] -> addres register 0xF4 bits: 0,1
#define BMP280_SLEEPMODE		0
#define BMP280_FORCEDMODE		1
#define BMP280_NORMALMODE		3

// t_standby time ->  t_sb[2:0] -> addres register 0xF5 bits: 5, 6, 7
#define BMP280_STANDBY_MS_0_5	0
#define BMP280_STANDBY_MS_62_5	1
#define BMP280_STANDBY_MS_125	2
#define BMP280_STANDBY_MS_250 	3
#define BMP280_STANDBY_MS_500	4
#define BMP280_STANDBY_MS_1000	5
#define BMP280_STANDBY_MS_2000	6
#define BMP280_STANDBY_MS_4000	7


//soft reset ->  reset[7:0] ->	If the value 0xB6 is written to the register,
//								the device is reset using the complete power-on-reset procedure

#define BMP280_SOFT_RESET		0xB6



#if BMP280_SPI
// 3-wire SPI interface -> spi3w_en[0]  -> addres register 0xF5 bits: 0
#define BMP280_SPI_3_WIRE	1
#endif

typedef union {
	uint8_t bt[2];
	struct {
		uint8_t mode:2;		// Measurement modes (Sleep/Forced/Normal mode)
		uint8_t	osrs_p:3;	// Enabling/disabling the measurement and oversampling pressure
		uint8_t	osrs_t:3;	// Enabling/disabling the temperature measurement and oversampling temperature
		uint8_t	spi3w_en:1;	// Enables 3-wire SPI interface when set to ‘1’.
		uint8_t reserved:1;	// reserved bit
		uint8_t	filter:3;	// IIR filter
		uint8_t	t_sb:3;		// standby time
	};
}CONF;

extern CONF conf_BMP280;

typedef union {
	uint8_t bt[24];

	struct {
		uint16_t dig_T1;
		int16_t dig_T2;
		int16_t dig_T3;
		uint16_t dig_P1;
		int16_t dig_P2;
		int16_t dig_P3;
		int16_t dig_P4;
		int16_t dig_P5;
		int16_t dig_P6;
		int16_t dig_P7;
		int16_t dig_P8;
		int16_t dig_P9;
		//int16_t reserved;
	};
} TCOEF;


typedef struct {
	TCOEF coef;
	int32_t adc_T;	// raw value of temperature
	int32_t adc_P;	// raw value of presasure


	// ----- temperature -----
	int32_t temperature;		// x 0,1 degree C
	int8_t t1;				// before comma
	uint8_t t2;				// after comma

#if USE_STRING
	char temp2str[10];		// tepmerature as string
#endif

	// ----- pressure -----

	int preasure;			// calue of calculated pressure

#if USE_STRING
	char pressure2str[10];		// pressure as string
#endif
} TBMP;

//TCOEF BMP_CD;	// Calibration data
extern TBMP bmp;


void BMP280_Conf (void);
void BMP280_ReadTP(void);

#endif /* BMP280_BMP280_H_ */
