/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f10x.h"
#include "UART/UART.h"
#include "I2C/I2C.h"
#include "BMP280/BMP280.h"
#include "COMMON/common_var.h"
#include "SPI/SPI.h"
			
ErrorStatus HSEStartUpStatus;
#define F_PCLK2  72000000

void RCC_Conf(void);
void GPIO_Conf(void);
void NVIC_Conf(void);
void SysTick_Conf(void);

volatile uint8_t flag;
uint32_t start_measure;
uint32_t allow_for_measure;
uint16_t result_time;
char measure_time[15];

int main(void)
{

	uint8_t result;
	uint8_t result_BMP_conf;

	RCC_Conf();
	SysTick_Conf();
	GPIO_Conf();

#if BMP280_I2C
	I2C_Conf(400);
#endif

#if BMP280_SPI
	SPI_Conf();
#endif

	UART_Conf(UART_BAUD);

	NVIC_Conf();



	do
	{
		result_BMP_conf = BMP280_Conf(&conf_BMP280, &bmp);
	}
	while(result_BMP_conf == 3);



	while (1)
	{
		if(flag)
		{
			flag = 0;

			if(result_BMP_conf)
			{
				uart_puts("Sensor configuration error:");
				if(bmp.err_conf == calib_reg)  uart_puts(" calibration coefficients includes zero value,");
				if(bmp.err_conf == config_reg) uart_puts(" configuration registers error,");
				if(bmp.err_conf == both) uart_puts(" calibration coefficients includes zero value and configuration registers error,");
				uart_puts("\n\r");
			}
			else
			{
				start_measure = source_time;
				result = BMP280_ReadTP(&bmp);

				switch(result)
				{
				case 1:
					uart_puts("Sensor error. Initialization phase didn't go properly.");
					uart_puts("\n\r");
					flag = 0;
					break;

				case 2:
					break;

				case 3:
					switch(bmp.err_boundaries_T)
					{
					case T_lower_limit:
						uart_puts(" Measured raw value of temperature is lower than minimum value (0x00000),");
						break;
					case T_over_limit:
						uart_puts(" Measured raw value of temperature is over than maximum value (0xFFFF0),");
						break;
					}

					switch(bmp.err_boundaries_T)
					{
					case P_lower_limit:
						uart_puts(" Measured raw value of pressure is lower than minimum value (0x00000),");
						break;
					case P_over_limit:
						uart_puts(" Measured raw value of pressure is over than maximum value (0xFFFF0),");
						break;
					}
					break;
				case 4:
					uart_puts(" Try to divide by 0 (measuring is intermittent.)");
					break;

				default:
					result_time = source_time - start_measure;

					uart_puts(bmp.temp2str);
					uart_puts("  ");
					uart_puts(bmp.pressure2str);

					uart_puts("  ");
					itoa(result_time, measure_time,10);
					uart_puts("measure take = ");
					uart_puts(measure_time);
					uart_puts("ms");
	//				uart_puts("    ");
	//				uart_puts("measure = ");
	//				uart_puts(source_time_tab);
	//				uart_puts("ms");
					uart_puts("\n\r");
				}
			}
		}

	}
}


void SysTick_Conf (void)
{

#define SysTick_Frequency 9000000 // 9MHz

//	SysTick_Config(F_PCLK2/8/1000);
//	SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;
	SysTick_Config(8000000/800);

}

void RCC_Conf(void)
{
  // Reset ustawien RCC
  RCC_DeInit();

  // Wlacz HSE
  RCC_HSEConfig(RCC_HSE_ON);

  // Czekaj az HSE bedzie gotowy
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if(HSEStartUpStatus == SUCCESS)
  {
	  /*
	   * wprowadzenie opoznien jest (waitstate) dla wy¿szych czêstotliwoœci taktowania
	   * jest spowodowane tym, ¿e maksymalna czêstotliwoœæ z jak¹ przeprowadzana
	   * jest komunikacja z pamiecia Flash moze wynosic 24 MHz
	   */
	  FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

	  // zwloka dla pamieci Flash
	  FLASH_SetLatency(FLASH_Latency_2);

	  // HCLK = SYSCLK
	  RCC_HCLKConfig(RCC_SYSCLK_Div1);

	  // PCLK2 = HCLK
	  RCC_PCLK2Config(RCC_HCLK_Div1);

//	  // PCLK1 = HCLK/2
//	  RCC_PCLK1Config(RCC_HCLK_Div2);
//
//	  // PLLCLK = 8MHz * 9 = 72 MHz
//	  RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
//
//	  // Wlacz PLL
//	  RCC_PLLCmd(ENABLE);
//
//	  // Czekaj az PLL poprawnie sie uruchomi
//	  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
//
//	  // PLL bedzie zrodlem sygnalu zegarowego
//	  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
//
//	  // Czekaj az PLL bedzie sygnalem zegarowym systemu
//	  while(RCC_GetSYSCLKSource() != 0x08);

	  //W³¹czenie systemu nadzoru sygna³u taktuj¹cego
	  //RCC_ClockSecuritySystemCmd(ENABLE);

	  RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
  }

//  Wlacz taktowanie GPIOB
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
}


void NVIC_Conf(void)
{
  // Tablica wektorow przerwan
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
}


void GPIO_Conf(void)
{
	GPIO_InitTypeDef GPIOInit;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
//	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	GPIO_StructInit(&GPIOInit);

	//Definicja pinów dla I2C GPIOB_PIN6 - SCL, GPIOB_PIN7 - SDA
	GPIOInit.GPIO_Pin = GPIO_Pin_13;
	GPIOInit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIOInit.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOC, &GPIOInit);



}



__attribute__((interrupt)) void SysTick_Handler(void)
{
	static uint16_t counter = 0;
	static uint8_t status = 0;

	if(counter == 1000)
	{
		allow_for_measure = source_time;
		flag = 1;
		counter = 0;
	}
	else
	{
		counter++;
	}
	source_time++;

	if(0 == status)
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		status = 1;
	}
	else
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_13);;
		status = 0;
	}


}
