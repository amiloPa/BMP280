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
	//char temp[2];
//	char source_time_tab[12];

	RCC_Conf();
	GPIO_Conf();
	I2C_Conf(400);
	UART_Conf(UART_BAUD);
	SysTick_Conf();
	NVIC_Conf();

	while (!BMP280_Conf());

	BMP280_ReadTP();

	while (1)
	{
		if(flag)
		{
			start_measure = source_time;
			result = BMP280_ReadTP();
			if( 1 == result )
			{
				uart_puts("sensor error");
				uart_puts("\n\r");
			}
			else if ( 2 == result)
			{
			}
			else
			{
				flag = 0;
				result_time = source_time - start_measure;

//				itoa(my_abs_uint(measure_time_time -allow_for_measure), source_time_tab,10);

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


void SysTick_Conf (void)
{

#define SysTick_Frequency 9000000 // 9MHz

	SysTick_Config(F_PCLK2/8/1000);
	SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;

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

	  // PCLK1 = HCLK/2
	  RCC_PCLK1Config(RCC_HCLK_Div2);

	  // PLLCLK = 8MHz * 9 = 72 MHz
	  RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

	  // Wlacz PLL
	  RCC_PLLCmd(ENABLE);

	  // Czekaj az PLL poprawnie sie uruchomi
	  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

	  // PLL bedzie zrodlem sygnalu zegarowego
	  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

	  // Czekaj az PLL bedzie sygnalem zegarowym systemu
	  while(RCC_GetSYSCLKSource() != 0x08);

	  //W³¹czenie systemu nadzoru sygna³u taktuj¹cego
	  //RCC_ClockSecuritySystemCmd(ENABLE);
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
//	GPIO_InitTypeDef GPIOInit;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
//
//	GPIO_StructInit(&GPIOInit);
//
//	//Definicja pinów dla I2C GPIOB_PIN6 - SCL, GPIOB_PIN7 - SDA
//	GPIOInit.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
//	GPIOInit.GPIO_Mode = GPIO_Mode_AF_OD;
//	GPIOInit.GPIO_Speed = GPIO_Speed_50MHz;
//	 GPIO_Init(GPIOB, &GPIOInit);



}



__attribute__((interrupt)) void SysTick_Handler(void)
{
	static uint16_t counter = 0;

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

//	BB(GPIOC->ODR, PC13) ^= 1;

}
