/*
 * BMP280.c
 *
 *  Created on: 15.10.2020
 *      Author: Piotr
 */

#include "BMP280.h"


TBMP bmp;
CONF conf_BMP280;

//void BMP280_Conf (TCOEF *bmp)
void BMP280_Conf (void)
{
	conf_BMP280.mode 		= BMP280_FORCEDMODE;
	conf_BMP280.osrs_p 		= BMP280_ULTRAHIGHRES;
	conf_BMP280.osrs_t		= BMP280_TEMPERATURE_20BIT;
	conf_BMP280.spi3w_en	= 0;
	conf_BMP280.reserved	= 0;
	conf_BMP280.filter		= BMP280_FILTER_OFF;
	conf_BMP280.t_sb		= BMP280_STANDBY_MS_0_5;

#if BMP280_I2C
	I2C_WRITE(BMP280_ADDR, 0xF4, 2, conf_BMP280.bt);	// write configurations bytes
	I2C_READ(BMP280_ADDR, 0x88, 24, bmp.coef.bt);		// read compensation parameters
#endif

//    uint8_t tmp;
//    for( uint8_t i=0; i<26; i+=2 ) {
//    	tmp = bmp.coef.bt[i];
//    	bmp.coef.bt[i] = bmp.coef.bt[i+1];
//    	bmp.coef.bt[i+1] = tmp;
//    }
}

void BMP280_ReadTP(void)
{
	uint8_t temp[6];
	uint8_t divisor;
	int32_t var1, var2, t_fine;
	uint32_t p;

	I2C_READ(BMP280_ADDR, 0xF7, 6, temp);

	// ----- calculate temperature -----
	switch(conf_BMP280.osrs_t)
	{
	case(BMP280_TEMPERATURE_16BIT):
			bmp.adc_T = (temp[3] << 8) | temp[4];
		break;
	case(BMP280_TEMPERATURE_17BIT):
			bmp.adc_T = (temp[3] << 9)  | (temp[4] << 1) | (temp[5] >> 7);
		break;
	case(BMP280_TEMPERATURE_18BIT):
			bmp.adc_T = (temp[3] << 10) | (temp[4] << 2) | (temp[5] >> 6);
		break;
	case(BMP280_TEMPERATURE_19BIT):
			bmp.adc_T = (temp[3] << 11) | (temp[4] << 3) | (temp[5] >> 5);
		break;
	case(BMP280_TEMPERATURE_20BIT):
			bmp.adc_T = (temp[3] << 12) | (temp[4] << 4) | (temp[5] >> 4);;
		break;
	}

	var1 = ((((bmp.adc_T >> 3) - ((int32_t)bmp.coef.dig_T1 << 1))) * ((int32_t)bmp.coef.dig_T2)) >> 11;
	var2 = (((((bmp.adc_T >> 4) - ((int32_t)bmp.coef.dig_T1)) * ((bmp.adc_T >> 4) - ((int32_t)bmp.coef.dig_T1))) >> 12) * ((int32_t)bmp.coef.dig_T3)) >> 14;

	t_fine = var1 + var2;

	bmp.temperature  = (t_fine * 5 + 128) >> 8;

	if(my_abs(bmp.temperature) > 999) 	divisor = 100;
	else 								divisor = 10;

	bmp.t1 = (int32_t)bmp.temperature / (int8_t)divisor;
	bmp.t2 = my_abs((uint32_t)bmp.temperature % (uint8_t)divisor);

#if USE_STRING
	uint8_t len;
	itoa(bmp.t1, &bmp.temp2str[0],10);
	len = strlen(bmp.temp2str);
	bmp.temp2str[len++] = ',';
	itoa(bmp.t2, &bmp.temp2str[len++],10);
#endif

	// ----- calculate pressure -----
	switch(conf_BMP280.osrs_p)
	{
	case(BMP280_ULTRALOWPOWER):
			bmp.adc_P = (temp[0] << 8) | temp[1];
		break;
	case(BMP280_LOWPOWER):
			bmp.adc_P = (temp[0] << 9)  | (temp[1] << 1) | (temp[2] >> 7);
		break;
	case(BMP280_STANDARD):
			bmp.adc_P = (temp[0] << 10) | (temp[1] << 2) | (temp[2] >> 6);
		break;
	case(BMP280_HIGHRES):
			bmp.adc_P = (temp[0] << 11) | (temp[1] << 3) | (temp[2] >> 5);
		break;
	case(BMP280_ULTRAHIGHRES):
			bmp.adc_P = (temp[0] << 12) | (temp[1] << 4) | (temp[2] >> 4);;
		break;
	}


	bmp.compensate_status = 0;
	divisor = 0;
	var1 = 0;
	var2 = 0;


	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11 ) * ((int32_t)bmp.coef.dig_P6);
	var2 = var2 + ((var1 * ((int32_t)bmp.coef.dig_P5)) << 1);
	var2 = (var2 >> 2) + (((int32_t)bmp.coef.dig_P4) << 16);
	var1 = (((bmp.coef.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13 )) >> 3) + ((((int32_t)bmp.coef.dig_P2) * var1) >> 1)) >> 18;
	var1 =((((32768 + var1)) * ((int32_t)bmp.coef.dig_P1)) >> 15);

	if (var1 == 0) bmp.compensate_status = 1;

	p = (((uint32_t)(((int32_t)1048576) - bmp.adc_P) - (var2 >> 12))) * 3125;

	if (p < 0x80000000) p = (p << 1) / ((uint32_t)var1);
	else				p = (p / (uint32_t)var1) * 2;


	var1 = (((int32_t)bmp.coef.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp.coef.dig_P8)) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp.coef.dig_P7) >> 4));


	bmp.preasure = (uint32_t)p;
	bmp.p1 =  (uint32_t)bmp.preasure / (uint8_t)100;
	bmp.p2 =  (uint32_t)bmp.preasure % (uint8_t)100;


#if USE_STRING
	itoa(bmp.p1, &bmp.pressure2str[0],10);
#endif


	if(conf_BMP280.mode == BMP280_FORCEDMODE)				// prepare values for next measure
	{
		#if BMP280_I2C
			I2C_WRITE(BMP280_ADDR, 0xF4, 1, conf_BMP280.bt);	// start force mode
		#endif
	}

}

int my_abs(int x)
{
    return x < 0 ? -x : x;
}
