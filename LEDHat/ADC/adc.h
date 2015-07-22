/*
 * adc.h
 *
 *  Created on: 4 May 2015
 *      Author: Michael
 */

#ifndef SRC_ADC_ADC_H_
#define SRC_ADC_ADC_H_

#include <stdint.h>

void ADC_interrupt(void);

void ADC_init(void);

uint16_t ADC_shittyBusyRead(uint8_t source);

uint16_t * ADC_busyRead();

void ADC_setNumberOfChannels(uint8_t count);

void ADC_setChannelSource(uint8_t channel, uint8_t source);



#endif /* SRC_ADC_ADC_H_ */
