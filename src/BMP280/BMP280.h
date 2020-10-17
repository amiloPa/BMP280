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


// Pressure oversampling ->  osrs_p[2:0] -> addres register 0xF4 bits: 2,3,4
#define BMP280_SKIPPED			0	// Pressure oversampling skipped 	-> resolution 0
#define BMP280_ULTRALOWPOWER	1	// Pressure oversampling x1			-> resolution 16
#define BMP280_LOWPOWER			2	// Pressure oversampling x2			-> resolution 17
#define BMP280_STANDARD			3	// Pressure oversampling x4			-> resolution 18
#define BMP280_HIGHRES			4	// Pressure oversampling x8			-> resolution 19
#define BMP280_ULTRAHIGHRES		5	// Pressure oversampling x16		-> resolution 20

// Temperature resolution -> osrs_t[2:0] -> addres register 0xF4 bits: 5,6,7
#define BMP280_TEMPERATURE_16BIT 1
#define BMP280_TEMPERATURE_17BIT 2
#define BMP280_TEMPERATURE_18BIT 3
#define BMP280_TEMPERATURE_19BIT 4
#define BMP280_TEMPERATURE_20BIT 5

// IIR filter ->  filter[2:0]  -> addres register 0xF5 bits: 2, 3, 4
#define BMP280_FILTER_OFF	0
#define BMP280_FILTER_X2 	1
#define BMP280_FILTER_X4 	2
#define BMP280_FILTER_X8	3
#define BMP280_FILTER_X16 	4

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

// 3-wire SPI interface -> spi3w_en[0]  -> addres register 0xF5 bits: 0
#define BMP280_SPI_3_WIRE 		1

typedef union{
	uint8_t bt[26];

	struct{
		uint16_t dig_T1;
		int dig_T2;
		int dig_T3;
		uint16_t dig_P1;
		int dig_P2;
		int dig_P3;
		int dig_P4;
		int dig_P5;
		int dig_P6;
		int dig_P7;
		int dig_P8;
		int dig_P9;
		int reserved;
	};
}TCOEF;

#endif /* BMP280_BMP280_H_ */
