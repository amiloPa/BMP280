/*
 * BMP280.c
 *
 *  Created on: 15.10.2020
 *      Author: Piotr
 */

#include "BMP280.h"


TBMP bmp;
CONF conf_BMP280;
uint32_t current_time;

void check_boundaries (void);
void soft_reset (void);
int my_abs(int x);

//void BMP280_Conf (TCOEF *bmp)
uint8_t BMP280_Conf (void)
{
	uint8_t buf[2];
	uint8_t i;

	conf_BMP280.mode 		= BMP280_FORCEDMODE;
	conf_BMP280.osrs_p 		= BMP280_ULTRAHIGHRES;
	conf_BMP280.osrs_t		= BMP280_TEMPERATURE_20BIT;
	conf_BMP280.spi3w_en	= 0;
	conf_BMP280.reserved	= 0;
	conf_BMP280.filter		= BMP280_FILTER_OFF;
	conf_BMP280.t_sb		= BMP280_STANDBY_MS_0_5;

	soft_reset();	// make a reset to clear previous setting
	if (current_time == 0)	current_time = source_time; // get system time

	if(source_time <= (current_time + 2) ) return 0;	//wait 2ms aster software reset

#if BMP280_I2C
	I2C_WRITE(BMP280_ADDR, 0xF4, 2, conf_BMP280.bt);	// write configurations bytes
	I2C_READ(BMP280_ADDR, 0xF4, 2, buf);				// read seted register
	I2C_READ(BMP280_ADDR, 0x88, 24, bmp.coef.bt);		// read compensation parameters
#endif


#if BMP280_SPI

#endif


	// ----- check if all calibration coefficients are diferent from 0 -----
	for( i = 0; i < (SIZE_OF_CONF_UNION/2); i++)
	{
		if(bmp.coef.bt2[i] == 0)
		{
			bmp.err_conf = calib_reg;
			return 2;
		}
	}

	// ----- check if set configuration registers are that same as readed -----
	for(i = 0; i < 2; i++)
	{
		if(buf[i] != conf_BMP280.bt[i])
		{
			if(!bmp.err_conf) 	bmp.err_conf = both;
			else 				bmp.err_conf = config_reg;
			return 2;
		}
	}

	return 1;
}

uint8_t BMP280_ReadTP(void)
{
	uint8_t temp[6];
	uint8_t divisor;
	int32_t var1, var2, t_fine;
	uint32_t p;

	// ----- if occured some error during parameterization sensor don't do any measure -----
	if(bmp.err_conf) return 1;


	I2C_READ(BMP280_ADDR, 0xF7, 6, temp);

	bmp.adc_T = (temp[3] << 12) | (temp[4] << 4) | (temp[5] >> 4);;
	bmp.adc_P = (temp[0] << 12) | (temp[1] << 4) | (temp[2] >> 4);;


	// ----- check boundaries -----
	check_boundaries();

	// ----- if raw values are lower or over the limits, function is intermittent and returning 1  -----
	if ((bmp.err_boundaries_T != 0) || ( bmp.err_boundaries_P != 0)) return 1;


	// ----- calculate temperature -----
	var1 = ((((bmp.adc_T >> 3) - ((int32_t)bmp.coef.dig_T1 << 1))) * ((int32_t)bmp.coef.dig_T2)) >> 11;
	var2 = (((((bmp.adc_T >> 4) - ((int32_t)bmp.coef.dig_T1)) * ((bmp.adc_T >> 4) - ((int32_t)bmp.coef.dig_T1))) >> 12) * ((int32_t)bmp.coef.dig_T3)) >> 14;

	t_fine = var1 + var2;

	bmp.temperature  = (t_fine * 5 + 128) >> 8;

	if(my_abs(bmp.temperature) > 999) 	divisor = 100;
	else 								divisor = 10;

	bmp.t1 = (int32_t)bmp.temperature / (int8_t)divisor;
	bmp.t2 = my_abs((uint32_t)bmp.temperature % (uint8_t)divisor);


	// ----- prepare string with value of temperature -----
#if USE_STRING
	uint8_t len;
	if(bmp.t1 >= 0) bmp.temp2str[0] = ' ';
	else			bmp.temp2str[0] = '-';

	itoa(bmp.t1, &bmp.temp2str[1],10);
	len = strlen(bmp.temp2str);
	bmp.temp2str[len++] = ',';
	if( bmp.t2 < 10) bmp.temp2str[len++] = '0';
	itoa(bmp.t2, &bmp.temp2str[len++],10);
#endif

	// ----- calculate pressure -----
	bmp.compensate_status = 0;
	var1 = 0;
	var2 = 0;

	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11 ) * ((int32_t)bmp.coef.dig_P6);
	var2 = var2 + ((var1 * ((int32_t)bmp.coef.dig_P5)) << 1);
	var2 = (var2 >> 2) + (((int32_t)bmp.coef.dig_P4) << 16);
	var1 = (((bmp.coef.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13 )) >> 3) + ((((int32_t)bmp.coef.dig_P2) * var1) >> 1)) >> 18;
	var1 =((((32768 + var1)) * ((int32_t)bmp.coef.dig_P1)) >> 15);

	if (var1 == 0) //if dividing by 0, function is intermittent and and returning 1
	{
		bmp.compensate_status = 1;
		return 1;
	}

	p = (((uint32_t)(((int32_t)1048576) - bmp.adc_P) - (var2 >> 12))) * 3125;

	if (p < 0x80000000) p = (p << 1) / ((uint32_t)var1);
	else				p = (p / (uint32_t)var1) * 2;


	var1 = (((int32_t)bmp.coef.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp.coef.dig_P8)) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp.coef.dig_P7) >> 4));


	bmp.preasure = (uint32_t)p;
	bmp.p1 =  (uint32_t)bmp.preasure / (uint8_t)100;
	//bmp.p2 =  (uint32_t)bmp.preasure % (uint8_t)100;


	// ----- prepare string with value of pressure -----
#if USE_STRING
	itoa(bmp.p1, &bmp.pressure2str[0],10);
#endif


	// ----- measure and prepare values for the next reading -----
	if(conf_BMP280.mode == BMP280_FORCEDMODE)
	{
		#if BMP280_I2C
			I2C_WRITE(BMP280_ADDR, 0xF4, 1, conf_BMP280.bt);	// start force mode
		#endif
	}

	return 0;// if everything is OK return 0
}

int my_abs(int x)
{
    return x < 0 ? -x : x;
}


void check_boundaries (void)
{
	bmp.err_boundaries_T = 0;
	bmp.err_boundaries_P = 0;

	if		(bmp.adc_T <= BMP280_ST_ADC_T_MIN) bmp.err_boundaries_T = T_lower_limit;
	else if (bmp.adc_T >= BMP280_ST_ADC_T_MAX) bmp.err_boundaries_T = T_over_limit;

	if		(bmp.adc_P <= BMP280_ST_ADC_P_MIN) 	bmp.err_boundaries_P = P_lower_limit;
	else if (bmp.adc_P >= BMP280_ST_ADC_P_MAX) 	bmp.err_boundaries_P = P_over_limit;
}

void soft_reset (void)
{
#ifdef BMP280_I2C
	I2C_WRITE(BMP280_ADDR, 0xE0, 1, (const void*)BMP280_SOFTWARE_RESET);
#endif

#ifdef BMP280_SPI
#endif
}
