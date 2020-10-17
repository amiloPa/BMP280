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
	conf_BMP280.osrs_p 		= BMP280_ULTRALOWPOWER;
	conf_BMP280.osrs_t		= BMP280_TEMPERATURE_16BIT;
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
	int32_t var1, var2, t_fine;

	I2C_READ(BMP280_ADDR, 0xF7, 6, temp);


//	switch(conf_BMP280.osrs_t)
//	{
//	case(BMP280_TEMPERATURE_16BIT):
//			bmp.adc_T = (temp[4] << 8) | temp[3];
//		break;
//	case(BMP280_TEMPERATURE_17BIT):
//			bmp.adc_T = (temp[4] << 9) | (temp[3] << 1) | (temp[5] >> 4);
//		break;
//	case(BMP280_TEMPERATURE_18BIT):
//			bmp.adc_T = (temp[4] << 10) | (temp[3] << 2) | (temp[5] >> 4);
//		break;
//	case(BMP280_TEMPERATURE_19BIT):
//			bmp.adc_T = (temp[4] << 11) | (temp[3] << 3) | (temp[5] >> 4);
//		break;
//	case(BMP280_TEMPERATURE_20BIT):
//			bmp.adc_T = (temp[4] << 12) | (temp[3] << 4) | (temp[5] >> 4);
//		break;
//	}


	switch(conf_BMP280.osrs_t)
	{
	case(BMP280_TEMPERATURE_16BIT):
			bmp.adc_T = (temp[4] << 8) | temp[3];
//			bmp.adc_T = (temp[3] << 8) | temp[4];
		break;
	case(BMP280_TEMPERATURE_17BIT):
			bmp.adc_T =  (temp[5] << 25) | (temp[4] << 9) | (temp[3] << 1);
		break;
	case(BMP280_TEMPERATURE_18BIT):
			bmp.adc_T = (temp[4] << 10) | (temp[3] << 2) | (temp[5] >> 4);
		break;
	case(BMP280_TEMPERATURE_19BIT):
			bmp.adc_T = (temp[4] << 11) | (temp[3] << 3) | (temp[5] >> 4);
		break;
	case(BMP280_TEMPERATURE_20BIT):
			bmp.adc_T = (temp[4] << 12) | (temp[3] << 4) | (temp[5] >> 4);
		break;
	}


	var1 = ((((bmp.adc_T >> 3) - ((int32_t)bmp.coef.dig_T1 << 1))) * ((int32_t)bmp.coef.dig_T2)) >> 11;
	var2 = (((((bmp.adc_T >> 4) - ((int32_t)bmp.coef.dig_T1)) * ((bmp.adc_T >> 4) - ((int32_t)bmp.coef.dig_T1))) >> 12) * ((int32_t)bmp.coef.dig_T3)) >> 14;

	t_fine = var1 + var2;

	bmp.temperature  = (t_fine * 5 + 128) >> 8;


	if(conf_BMP280.mode == BMP280_FORCEDMODE)				// prepare values for next measure
	{
		#if BMP280_I2C
			I2C_WRITE(BMP280_ADDR, 0xF4, 1, conf_BMP280.bt);	// start force mode
		#endif
	}

}
