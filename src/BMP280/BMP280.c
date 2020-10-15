/*
 * BMP280.c
 *
 *  Created on: 15.10.2020
 *      Author: Piotr
 */

#include "BMP280.h"

#define BMP280_ADDR 		0xEC	// Sensor addres -> SDO pin is connected to GND
//#define BMP280_ADDR 		0xEE	// Sensor addres -> SDO pin is connected to VCC
#define BMP280_SLEEP_MODE	0		// Sleep mode
#define BMP280_FORCED_MODE	1		// Forced mode
#define BMP280_NORMAL_MODE	3		// Normal mode

