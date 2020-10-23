/*
 * SPI.h
 *
 *  Created on: 01.10.2020
 *      Author: Piotr
 */

#ifndef SPI_H_
#define SPI_H_

#include "stm32f10x.h"

void SPI_Conf(void);
void SELECT (void);
void DESELECT (void);

void SPI_SendData (uint8_t  address, uint8_t *Data, uint8_t size);  // sending a few data

#endif /* SPI_H_ */
