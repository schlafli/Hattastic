/*
 * adc.c
 *
 *  Created on: 4 May 2015
 *      Author: Michael
 */


#include "adc.h"
#include "msp.h"


uint16_t ADC_readMem(uint8_t channel);

volatile uint8_t adcInterrupted = 0;
volatile uint16_t adcInternalBuffer[32];
uint16_t adcSamples[32];
volatile uint8_t sampleIndex;

void ADC_interrupt(void){
	//for now treat any interrupt as EOC
	uint8_t i;
	for(i=0;i<32;i++){
		if(ADC14->rIFGR0.r & (1<<i)){
			adcInternalBuffer[i] = ADC_readMem(i);
		}
	}

	adcInterrupted=1;

	//	ADC14->rIV =0; //clear pending ADC interrupts
}

void ADC_busyWaitTillFinished(){
	while(ADC14->rCTL0.b.bBUSY){
		__no_operation();
	}
}


void ADC_init(){

	//wait till the ADC is no longer busy
	ADC_busyWaitTillFinished();

	//disable ENC
	ADC14->rCTL0.b.bENC = 0;

	//disable ADC
	ADC14->rCTL0.b.bON = 1;

	//set ADC clock source to SMCLK (24MHz)
	ADC14->rCTL0.b.bSSEL = 4;

	//set the divisor to 2
	ADC14->rCTL0.b.bDIV = 1;

	//switch to pulse sampling
	ADC14->rCTL0.b.bSHP = 1;

	//set 14bit resolution
	ADC14->rCTL1.b.bRES = 3;

	//enable ADC interrupts
	NVIC_EnableIRQ(ADC14_IRQn);
}

void ADC_stopConversion(){
	ADC14->rCTL0.r &= ~(ADC14ENC | ADC14SC);
}


void ADC_startConversion(){
	ADC14->rCTL0.r |= ADC14ENC | ADC14SC;
}

void ADC_setChannels(uint8_t start, uint8_t end){
	start &=0x001f;
	end &=0x001f;

	uint8_t i;

	volatile uint32_t * ADCMCTL_BASE = &(ADC14->rMCTL0.r);


	for(i=0;i<end-start;i++){
		ADCMCTL_BASE[start+i] &= ~ADC14EOS;
	}
	ADCMCTL_BASE[end-1] |= ADC14EOS;

	ADC14->rCTL1.b.bCSTARTADD = start;
}



void ADC_setNumberOfChannels(uint8_t count){
	if(count>31){
		return;
	}
	ADC_setChannels(0, count);

	if(count == 1){
		//set single conversion;
		ADC14->rCTL0.b.bCONSEQ = 0;
		ADC14->rCTL0.b.bMSC = 0;
	}else{
		//set to measure sequence of channels mode (single shot)
		ADC14->rCTL0.b.bCONSEQ = 1;
		ADC14->rCTL0.b.bMSC = 1;
	}

	ADC14->rIER0.r =  1<<(count-1); //set the interrupt on EOC
}

void ADC_setChannelSource(uint8_t channel, uint8_t source){

	channel &=0x1f;
	channel &=0x1f;

	volatile uint32_t * ADCMCTL_BASE = &(ADC14->rMCTL0.r);

	ADCMCTL_BASE[channel] &= ~0x1f;
	ADCMCTL_BASE[channel] |= source;

}


uint16_t ADC_readMem(uint8_t channel){
	channel &=0x1f;
	volatile uint32_t * ADCMEM_BASE = &(ADC14->rMEM0.r);
	return (uint16_t) (ADCMEM_BASE[channel] & 0x0000ffff);
}


uint16_t ADC_shittyBusyRead(uint8_t source){
	source &= 0x1f;

	ADC_stopConversion();

	uint16_t i=0;
	for(i=0;i<1000;i++){
		__no_operation();
	}
	adcInterrupted = 0;

	ADC_setNumberOfChannels(2);
	ADC_setChannelSource(0, source);
	ADC_setChannelSource(1, source);

	//enable ADC complete interrupt and change code to interrupt based ADC
	ADC14->rCLRIFGR0.r = 0; //clear all pending ADC interrupts

	ADC_startConversion();

	while(!adcInterrupted){
		__no_operation();
	}

	//	ADC_busyWaitTillFinished();

	ADC_stopConversion();

	return ADC_readMem(0);
}

uint16_t * ADC_busyRead(){
	adcInterrupted = 0;
	ADC_startConversion();
	while(!adcInterrupted){
		__no_operation();
	}

	uint8_t i=0;
	for(i=0;i<32;i++){
		adcSamples[i] = adcInternalBuffer[i];
	}

	return adcSamples;
}

