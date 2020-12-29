/*
 * BMP280.c
 *
 *  Created on: 15.10.2020
 *      Author: Piotr
 */

#include "BMP280.h"


TBMP bmp;
CONF conf_BMP280;


void check_boundaries (TBMP *bmp);			// check if read uncompensated values are in boundary MIN and MAX
void soft_reset (void);						// execute sensor reset by software
void get_status (TBMP *bmp);				// read statuses of sensor
uint8_t bmp280_compute_meas_time(void);		// measurement time in milliseconds for the active configuration
void pressure_at_sea_level(TBMP *bmp);		// calculating pressure reduced to sea level

void BMP280_read_data(uint8_t SLA, uint8_t register_addr,  uint8_t size, uint8_t *Data);			// read data from sensor
void BMP280_write_data(uint8_t SLA, uint8_t register_addr, uint8_t size, uint8_t *Data);			// write data to sensor
uint8_t read_compensation_parameter_write_configuration_and_check_it(CONF *sensor, TBMP *bmp);		// write configuration and check if saved configuration is equal to set

#if CALCULATION_AVERAGE_TEMP
	void calculation_average_temp(TBMP *bmp);	// calculate average temperature, No of samples to calculations is taken from No_OF_SAMPLES
#endif

/****************************************************************************/
/*      setting function configurations of sensor					        */
/****************************************************************************/
uint8_t BMP280_Conf (CONF *sensor, TBMP *bmp)
{
	uint8_t counter = 0;
	uint8_t  result_of_check = 5;
	static uint32_t current_time = 0;

	sensor->mode 		= BMP280_FORCEDMODE;
	sensor->osrs_p 		= BMP280_ULTRAHIGHRES;
	sensor->osrs_t		= BMP280_TEMPERATURE_20BIT;
	sensor->spi3w_en	= 0;
	sensor->reserved	= 0;
	sensor->filter		= BMP280_FILTER_OFF;
	sensor->t_sb		= BMP280_STANDBY_MS_0_5;

	if (current_time == 0)
		{
			soft_reset();				// make a reset to clear previous setting
			current_time = source_time; // get system time
		}

	if(source_time <= (current_time + 3) ) return 3;	//wait 3ms aster software reset

	do
	{
		result_of_check = read_compensation_parameter_write_configuration_and_check_it(sensor, bmp);
		counter++;
	}
	while((result_of_check) && (counter < 3));

	return result_of_check;	// if everything is OK return 0
}

/****************************************************************************/
/*      read, check, calculate and prepare string for measured values       */
/****************************************************************************/
uint8_t BMP280_ReadTP(TBMP *bmp)
{
	uint8_t temp[6];
	uint8_t divisor;
	int32_t var1, var2, t_fine;
	uint32_t p;


	// ----- if occured some error during parameterization sensor don't do any measure -----
	if(bmp->err_conf) return 1;

#if BMP280_INCLUDE_STATUS
	// ----- get status of sensor -----
	get_status();

	// ----- if sensor is in measuring or im_update status, wait up to the finishing and after that read measures -----
	if( (bmp.measuring_staus)  || (bmp.im_update_staus)) return 2;

#endif

	BMP280_read_data(BMP280_ADDR, 0xF7, 6, (uint8_t *)&temp);	// read data register

	bmp->adc_T = (temp[3] << 12) | (temp[4] << 4) | (temp[5] >> 4);
	bmp->adc_P = (temp[0] << 12) | (temp[1] << 4) | (temp[2] >> 4);


//	// ----- check boundaries -----
	check_boundaries(bmp);

	// ----- if raw values are lower or over the limits, function is intermittent and returning 3  -----
	if ((bmp->err_boundaries_T != 0) || ( bmp->err_boundaries_P != 0)) return 3;

	// ----- calculate temperature -----
	var1 =  ((((bmp->adc_T >> 3) - ((int32_t)bmp->coef.dig_T1 << 1))) * ((int32_t)bmp->coef.dig_T2)) >> 11;
	var2 = (((((bmp->adc_T >> 4) - ((int32_t)bmp->coef.dig_T1)) * ((bmp->adc_T >> 4) - ((int32_t)bmp->coef.dig_T1))) >> 12) * ((int32_t)bmp->coef.dig_T3)) >> 14;

	t_fine = var1 + var2;

	bmp->temperature  = (t_fine * 5 + 128) >> 8;

	if(my_abs(bmp->temperature) > 9) 	divisor = 100;
	else 								divisor = 10;


	bmp->t1 = (int32_t)bmp->temperature / (int8_t)divisor;
	bmp->t2 = my_abs((uint32_t)bmp->temperature % (uint8_t)divisor);

	// ----- prepare string with value of temperature -----
#if USE_STRING
	uint8_t len;

	#if CALCULATION_AVERAGE_TEMP


		// ----- calculation of average temperature -----
		calculation_average_temp(bmp);

		itoa(bmp->avearage_cel, &bmp->temp2str[0], 10);
		len = strlen(bmp->temp2str);
		bmp->temp2str[len++] = ',';

		if( bmp->avearage_fract < 10) bmp->temp2str[len++] = '0';
		itoa(bmp->avearage_fract, &bmp->temp2str[len++],10);

	#else

		itoa(bmp->t1, &bmp->temp2str[0], 10);
		len = strlen(bmp->temp2str);
		bmp->temp2str[len++] = ',';

		if( bmp->t2 < 10) bmp->temp2str[len++] = '0';
		itoa(bmp->t2, &bmp->temp2str[len++],10);
	#endif

#endif

	// ----- calculate pressure -----
	bmp->compensate_status = 0;
	var1 = 0;
	var2 = 0;

	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11 ) * ((int32_t)bmp->coef.dig_P6);
	var2 = var2 + ((var1 * ((int32_t)bmp->coef.dig_P5)) << 1);
	var2 = (var2 >> 2) + (((int32_t)bmp->coef.dig_P4) << 16);
	var1 = (((bmp->coef.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13 )) >> 3) + ((((int32_t)bmp->coef.dig_P2) * var1) >> 1)) >> 18;
	var1 =((((32768 + var1)) * ((int32_t)bmp->coef.dig_P1)) >> 15);

	if (var1 == 0) //if dividing by 0, function is intermittent and returning 4
	{
		bmp->compensate_status = 1;
		return 4;
	}

	p = (((uint32_t)(((int32_t)1048576) - bmp->adc_P) - (var2 >> 12))) * 3125;

	if (p < 0x80000000) p = (p << 1) / ((uint32_t)var1);
	else				p = (p / (uint32_t)var1) * 2;


	var1 = (((int32_t)bmp->coef.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp->coef.dig_P8)) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp->coef.dig_P7) >> 4));


	bmp->preasure = (uint32_t)p;
	bmp->p1 =  (uint32_t)bmp->preasure / (uint8_t)100;
	//bmp.p2 =  (uint32_t)bmp.preasure % (uint8_t)100;


	// ----- prepare string with value of pressure -----
#if USE_STRING
	itoa(bmp->p1, &bmp->pressure2str[0],10);
#endif

	// ----- measure and prepare values for the next reading -----
	if(conf_BMP280.mode == BMP280_FORCEDMODE)
	{
		BMP280_write_data(BMP280_ADDR, 0xF4, 1, conf_BMP280.bt);		// write configurations bytes
	}

	// ----- calculate a preasure sea level -----
	pressure_at_sea_level(bmp);

	return 0;	// if everything is OK return 0
}

/****************************************************************************/
/*      check if read uncompensated values are in boundary MIN and MAX      */
/****************************************************************************/
void check_boundaries (TBMP *bmp)
{
	bmp->err_boundaries_T = 0;
	bmp->err_boundaries_P = 0;

	if		(bmp->adc_T <= BMP280_ST_ADC_T_MIN) bmp->err_boundaries_T = T_lower_limit;
	else if (bmp->adc_T >= BMP280_ST_ADC_T_MAX) bmp->err_boundaries_T = T_over_limit;

	if		(bmp->adc_P <= BMP280_ST_ADC_P_MIN) bmp->err_boundaries_P = P_lower_limit;
	else if (bmp->adc_P >= BMP280_ST_ADC_P_MAX) bmp->err_boundaries_P = P_over_limit;
}

/****************************************************************************/
/*      execute sensor reset by software							        */
/****************************************************************************/
void soft_reset (void)
{
	BMP280_write_data(BMP280_ADDR, 0xE0, 1, (uint8_t*)BMP280_SOFTWARE_RESET);		// write configurations bytes
}

/****************************************************************************/
/*      read statuses of sensor      										*/
/****************************************************************************/
void get_status (TBMP *bmp)
{
	uint8_t *status = 0;

	BMP280_read_data(BMP280_ADDR, 0xF3, 1, status);	// read set register

	bmp->measuring_staus =  *status & BMP280_MEASURING_STATUS;
	bmp->im_update_staus =  *status & BMP280_IM_UPDATE_STATUS;
}

/****************************************************************************/
/*      measurement time in milliseconds for the active configuration       */
/****************************************************************************/
uint8_t bmp280_compute_meas_time(void)
{
    uint32_t period = 0;
    uint32_t t_dur = 0, p_dur = 0, p_startup = 0;
    const uint32_t startup = 1000, period_per_osrs = 2000; // Typical timings in us. Maximum is +15% each

        t_dur = period_per_osrs * ((UINT32_C(1) << conf_BMP280.osrs_t) >> 1);
        p_dur = period_per_osrs * ((UINT32_C(1) << conf_BMP280.osrs_p) >> 1);
        p_startup = (conf_BMP280.osrs_p) ? 500 : 0;

        period = startup + t_dur + p_startup + p_dur + 500; // Increment the value to next highest integer if greater than 0.5
        period /= 1000; 									// Convert to milliseconds


    return (uint8_t)period;
}

/****************************************************************************/
/*      calculating pressure reduced to sea level            				*/
/****************************************************************************/
void pressure_at_sea_level(TBMP *bmp){

        uint16_t st_baryczny,tpm,t_sr;
        uint32_t p0,p_sr;

        st_baryczny = (800000 * (1000 + 4 * bmp->temperature) / (bmp->preasure));          		// calculation of the baric degree acc. to the pattern of the Babineta
        p0 = bmp->preasure + (100000 * BMP280_ALTITUDE / (st_baryczny));              			// calculation of approximate sea level pressure
        p_sr = (bmp->preasure + p0) / 2;                     									// calculation of average pressure for layers between sea level and sensor
        tpm = bmp->temperature + ((6 * BMP280_ALTITUDE) / 1000);                           		// calculation of average temperature for layer of air
        t_sr = (bmp->temperature + tpm) / 2;                                              		// calculation of average temperature for layer between sea level and sensor
        st_baryczny = (800000 * (1000 + 4 * t_sr) / (p_sr));              						// calculation of more accurate value of baric degree
        bmp->sea_pressure_redu = bmp->preasure + (100000 * BMP280_ALTITUDE / (st_baryczny)); 	// calculation of more accurate value of pressure for sea level
}

/****************************************************************************/
/*     in depend on selected protocol data are saved to sensor		        */
/****************************************************************************/
void BMP280_write_data(uint8_t SLA, uint8_t register_addr, uint8_t size, uint8_t *Data)
{
#if BMP280_I2C

	int i;
	  const uint8_t* buffer = (uint8_t*)Data;

	  //select device
	  I2C_ADDRES(SLA, register_addr);

	  //sending data from whole buffer
	  for (i = 0; i < size; i++)
	  {
	   I2C_SendData(I2C1, buffer[i]);
	   while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS);
	  }
	  //STOP signal generating
	  I2C_GenerateSTOP(I2C1, ENABLE);

#endif

#if BMP280_SPI
//	SPI_SendData(register_addr, Data, size);

	const uint8_t register_mask = 0x7F;
	uint8_t data;

	SELECT();

	for (uint8_t i = 0; i < size; i++)
	{
		data = Data[i];
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		SPI_I2S_SendData(SPI1, register_mask & register_addr);

		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		SPI_I2S_SendData(SPI1, data);

		register_addr++;
	}
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

	DESELECT();
#endif

}

/****************************************************************************/
/*      in depend on selected protocol data are readed from sensor	        */
/****************************************************************************/
void BMP280_read_data(uint8_t SLA, uint8_t register_addr,  uint8_t size, uint8_t *Data)
{
#if BMP280_I2C

	  int i;
	  uint8_t* buffer = (uint8_t*)Data;

	  //select device
	  I2C_ADDRES(SLA, register_addr);

	  //START signal sending and waiting for response
	  I2C_GenerateSTART(I2C1, ENABLE);
	  while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS);

	  //Receiving only one data byte and turn off ack signal
	  I2C_AcknowledgeConfig(I2C1, ENABLE);
	  //Device address sending, setting microcontroller as master in receive mode
	  I2C_Send7bitAddress(I2C1, SLA, I2C_Direction_Receiver);
	  //waitnig for EV6
	  while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS);

	  for (i = 0; i < size - 1; i++)
	  {
	   while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS);
	   buffer[i] = I2C_ReceiveData(I2C1);
	  }
	  //Receiving only one byte
	  I2C_AcknowledgeConfig(I2C1, DISABLE);

	  //STOP signal generating
	  I2C_GenerateSTOP(I2C1, ENABLE);
	  //waiting for signal
	  while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS);
	  //reading data from receiving register
	  buffer[i] = I2C_ReceiveData(I2C1);
#endif


#if BMP280_SPI
//	SPI_ReceiveData(register_addr, Data, size );

	SELECT();

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, register_addr);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	SPI_I2S_ReceiveData(SPI1);

	for (uint8_t i = 0; i < size; i++)
	{
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		SPI_I2S_SendData(SPI1, 0);
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
		Data[i] = SPI_I2S_ReceiveData(SPI1);
	}

	DESELECT();
#endif
}

/****************************************************************************/
/*     write configuration and check if saved configuration is equal to set */
/****************************************************************************/
uint8_t read_compensation_parameter_write_configuration_and_check_it (CONF *sensor, TBMP *bmp)
{
	uint8_t buf[2];
	uint8_t i;
	uint8_t bt_temp[2];

	BMP280_write_data(BMP280_ADDR, 0xF4, 2, sensor->bt);		// write configurations bytes
	BMP280_read_data(BMP280_ADDR, 0xF4,  2, buf);				// read set register
	BMP280_read_data(BMP280_ADDR, 0x88, 24, bmp->coef.bt);		// read compensation parameters

	// ----- check if all calibration coefficients are diferent from 0 -----
	for( i = 0; i < (SIZE_OF_CONF_UNION/2); i++)
	{
		if(bmp->coef.bt2[i] == 0)
		{
			bmp->err_conf = calib_reg;
			return 1;
		}
	}

	// ----- check if set configuration registers are that same as readed -----
	if(sensor->mode == BMP280_FORCEDMODE)   /*sometimes hapend, that sensor go to sleep mode after single measurement was finished
	 	 	 	 	 	 	 	 	 	 	 * (power mode is equal to forced mode and this situation is decribed in datasheet,
	 	 	 	 	 	 	 	 	 	 	 * chapter: 3.6.2 Forced mode). In this case bytes responsible for select mode
	 	 	 	 	 	 	 	 	 	 	 * in control register 0xF4 are come back to 0 value (sensor go to sleep mode)*/
	{
		for(i = 0; i < 2; i++)
		{
			bt_temp[ i] = sensor->bt[i];
			if (i == 0)
			{
				buf[i] &= 0xFE;
				bt_temp[i] &= 0xFE;
			}

			if(buf[i] != bt_temp[i])
			{
				if(bmp->err_conf == calib_reg) 	bmp->err_conf = both;
				else 							bmp->err_conf = config_reg;
				return 2;
			}
		}
	}
	else
	{
		for(i = 0; i < 2; i++)
		{
			if(buf[i] != sensor->bt[i])
			{
				if(bmp->err_conf == calib_reg) 	bmp->err_conf = both;
				else 							bmp->err_conf = config_reg;
				return 2;
			}
		}
	}

	return 0;
}

/****************************************************************************/
/*     Calculate average temperature,										*/
/* 	   No of samples to calculations is taken from No_OF_SAMPLES 			*/
/****************************************************************************/
#if CALCULATION_AVERAGE_TEMP
	void calculation_average_temp(TBMP *bmp)
	{
		int32_t avearage_temp_value = 0;
		static uint8_t i = 1;
		uint8_t k;
		uint8_t avearage_fract_temp;

		if(i <= No_OF_SAMPLES)
		{
			if(bmp->t1 >= 0) bmp->smaples_of_temp[i-1] = 100 * bmp->t1 + bmp->t2;
			else 			 bmp->smaples_of_temp[i-1] = -1 * (100 * my_abs(bmp->t1) + bmp->t2);
		}
		else
		{
			for (k = 1; k < i - 1; k++)
			{
				bmp->smaples_of_temp[k-1] = bmp->smaples_of_temp[k];
			}

			if(bmp->t1 >= 0) bmp->smaples_of_temp[--k] = 100 * bmp->t1 + bmp->t2;
			else 			 bmp->smaples_of_temp[--k] = -1 * (100 * my_abs(bmp->t1) + bmp->t2);
		}

		for (k = 0; k < i - 1; k++)
		{
			avearage_temp_value += bmp->smaples_of_temp[k];
		}

		avearage_temp_value /= (i - 1);

		bmp->avearage_cel = avearage_temp_value / 100 ;
		avearage_fract_temp = (my_abs(avearage_temp_value)) % 100;
		bmp->avearage_fract = avearage_fract_temp;

		if( i <= No_OF_SAMPLES) i++;
	}
#endif

