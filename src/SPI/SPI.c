/*
 * SPI.c
 *
 *  Created on: 01.10.2020
 *      Author: Piotr
 */


#include "SPI.h"

void SPI_Conf(void)
{
  	GPIO_InitTypeDef  GPIO_InitStructure;
  	SPI_InitTypeDef   SPI_InitStructure;

	// Run clocks for peripherals
  	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA |
							RCC_APB2Periph_SPI1  |
							RCC_APB2Periph_AFIO, ENABLE);


  	// PB0 jako CS
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOA, &GPIO_InitStructure);

  	//SCK -> PA5, MISO -> PA6, MOSI -> PA7
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_Init(GPIOA, &GPIO_InitStructure);


  	// Configuration SPI
  	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  	//SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  	SPI_InitStructure.SPI_CRCPolynomial = 7;
  	SPI_Init(SPI1, &SPI_InitStructure);

  	// Wlacz SPI
  	SPI_Cmd(SPI1, ENABLE);

  	DESELECT();
}

void SELECT (void) 		// CS in low state
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);

}

void DESELECT (void) 	// CS in high state
{
	GPIO_SetBits(GPIOA, GPIO_Pin_1);

}

void SPI_SendData (uint8_t  address, uint8_t *Data, uint8_t size)  // sending a few data
{
	const uint8_t register_mask = 0x7F;
	uint8_t data;

	SELECT();

	for (uint8_t i = 0; i < size; i++)
	{
		data = Data[i];
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		SPI_I2S_SendData(SPI1, register_mask & address);

		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		SPI_I2S_SendData(SPI1, data);

		address++;
	}

	DESELECT();
}

void SPI_ReceiveData (uint8_t address, uint8_t *Data, uint8_t size)  		// Odebranie bajtu z SD
{
	SELECT();

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, address);

	for (uint8_t i = 0; i < size; i++)
	{
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
		Data[i] = SPI_I2S_ReceiveData(SPI1);
	}

	DESELECT();
}
