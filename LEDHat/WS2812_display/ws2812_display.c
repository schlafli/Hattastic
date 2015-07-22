/*
 * ws2812_display.c
 *
 *  Created on: 29 Jun 2015
 *      Author: Michael
 */

#include "ws2812_display.h"
#include "msp.h"

#define BLANK_BYTES 100

//leds * channels * overhead
//#define LED_BUFFER_SIZE ((WS2812_LED_TOTAL_COUNT*3)+BLANK_BYTES)

//size is 2304 + blank bytes rounded up to nearest 256
#define LED_BUFFER_SIZE (2560)

uint8_t frameBuffer[2][LED_BUFFER_SIZE];

volatile uint8_t * displayShiftBuffer;
volatile uint8_t frameBufferIndex = 0;
volatile uint8_t swapQueued = 0;

uint16_t shiftPosition = 0;

//yay for tons of RAM :)
uint8_t ledLookupTableHighByte[256];
uint8_t ledLookupTableMiddleByte[256];
uint8_t ledLookupTableLowByte[256];

#define DMA_CH0_PRIMARY (0)
#define DMA_CH1_PRIMARY (1*4)
#define DMA_CH2_PRIMARY (2*4)
#define DMA_CH3_PRIMARY (3*4)
#define DMA_CH4_PRIMARY (4*4)
#define DMA_CH5_PRIMARY (5*4)
#define DMA_CH6_PRIMARY (6*4)
#define DMA_CH7_PRIMARY (7*4)

#define DMA_ALT_BASE (8*4)

#define DMA_CH0_ALT (DMA_ALT_BASE + DMA_CH0_PRIMARY)
#define DMA_CH1_ALT (DMA_ALT_BASE + DMA_CH1_PRIMARY)
#define DMA_CH2_ALT (DMA_ALT_BASE + DMA_CH2_PRIMARY)
#define DMA_CH3_ALT (DMA_ALT_BASE + DMA_CH3_PRIMARY)
#define DMA_CH4_ALT (DMA_ALT_BASE + DMA_CH4_PRIMARY)
#define DMA_CH5_ALT (DMA_ALT_BASE + DMA_CH5_PRIMARY)
#define DMA_CH6_ALT (DMA_ALT_BASE + DMA_CH6_PRIMARY)
#define DMA_CH7_ALT (DMA_ALT_BASE + DMA_CH7_PRIMARY)

#define DMA_OFS_SRC 0
#define DMA_OFS_DST 1
#define DMA_OFS_CFG 2

#define DMA_BLOCK_SIZE 256
#define DMA_BLOCK_COUNT 10

volatile uint8_t interruptDone = 0;

uint8_t usingAlt = 0;
uint32_t dma_control[64] __attribute__((aligned(1024)));

uint32_t dma_block_number = 0;

void setupDMATransfer(uint8_t primary) {
	uint16_t modAddress;
	if (primary) {
		modAddress = DMA_CH4_PRIMARY;
	} else {
		modAddress = DMA_CH4_ALT;
	}

	uint32_t dataEndAddress = (DMA_BLOCK_SIZE * (dma_block_number+1)) -1;


	dma_control[modAddress + DMA_OFS_SRC] = &(displayShiftBuffer[dataEndAddress]);
	dma_control[modAddress + DMA_OFS_DST] = &(EUSCI_A2->rTXBUF.r);
	dma_control[modAddress + DMA_OFS_CFG] =
	UDMA_CHCTL_DSTSIZE_8 | UDMA_CHCTL_DSTINC_NONE |
	UDMA_CHCTL_SRCSIZE_8 | UDMA_CHCTL_SRCINC_8 |
	UDMA_CHCTL_ARBSIZE_1 | ((DMA_BLOCK_SIZE-1) << 4) | UDMA_CHCTL_XFERMODE_PINGPONG;

	dma_block_number++;
	if(dma_block_number >= DMA_BLOCK_COUNT){
		dma_block_number = 0;
		if (swapQueued) {
			frameBufferIndex ^= 1;
			displayShiftBuffer = frameBuffer[frameBufferIndex];
			swapQueued = 0;
//			DIO->rPAOUT.b.bP2OUT ^= 0x02;
		}
	}


}

void DMA_err_interrupt() {
	while (1) {
		;
	}
}

void DMA_int0_interrupt() {
	while (1) {
		;
	}
}

void DMA_int1_interrupt() {

	interruptDone = 1;
	usingAlt ^= 1;
	setupDMATransfer(usingAlt); //because we toggled using alt, at this point it's the same flag as "primary"
	DMA->rINT0_CLRFLG.r = DMA->rINT0_SRCFLG.r;

}

void DMA_int2_interrupt() {
	while (1) {
		;
	}
}

void DMA_int3_interrupt() {
	while (1) {
		;
	}
}


void WS2812_initDMA() {
	DMA->rCFG.r = UDMA_STAT_MASTEN;
	DMA->rCTLBASE.r = dma_control;

	setupDMATransfer(1);
	setupDMATransfer(0);

	DMA->rINT1_SRCCFG.b.bINT_SRC = 4;
	DMA->rINT1_SRCCFG.b.bEN = 1;

	DMA->rENASET = 1 << 4; //enable channel 4

	DMA->rCH4_SRCCFG.b.bDMA_SRC = 1; //on MSP432 Lauchpad ch4 src 1 is EUCSI_A2_TX

	NVIC_EnableIRQ(DMA_INT1_IRQn);
	NVIC_EnableIRQ(DMA_ERR_IRQn);
	NVIC_EnableIRQ(DMA_INT0_IRQn);

}

void initTimeoutTimer() {


	TIMER_A1->rCTL.b.bSSEL = 1; //aclk
	TIMER_A1->rEX0.b.bIDEX = 3; // div_4
	TIMER_A1->rCTL.b.bID = 3; //div by 8
	TIMER_A1->rCTL.b.bCLR = 1; //clear the timer

}

void stopTimer(){

	TIMER_A1->rCTL.b.bMC = 0; //continuous;
	TIMER_A1->rR = 0;
}


void startTimer(){
	stopTimer();
	TIMER_A1->rCTL.b.bMC = 2; //continuous;
}

uint16_t getTimerValue(){
	return TIMER_A1->rR;
}

void initLookupTable() {
	uint16_t i, bitNumber;
	uint32_t result = 0;

	for (i = 0; i < 256; i++) {
		result = 0;
		for (bitNumber = 0; bitNumber < 8; bitNumber++) {
			if (i & (1 << bitNumber)) {
				result |= 6 << (bitNumber * 3);
			} else {
				result |= 4 << (bitNumber * 3);
			}
		}

		ledLookupTableHighByte[i] = 0xff & (result >> 16);
		ledLookupTableMiddleByte[i] = 0xff & (result >> 8);
		ledLookupTableLowByte[i] = 0xff & (result);

	}
}

void initSPI() {

	EUSCI_A2->rCTLW0.a.bSYNC = 1; //Turn on fucking SPI mode
	EUSCI_A2->rCTLW0.a.bMSB = 1; //msb
	EUSCI_A2->rCTLW0.a.bMST = 1; //master
	EUSCI_A2->rCTLW0.a.bSSEL = 3; //clock is SMCLK

	//	EUSCI_A2->rCTLW0.a.bMODE = 2;

	EUSCI_A2->rBRW = 12; //at 48 MHz this gives us 48/2/10 = 2.4 MHz bitrate
	EUSCI_A2->rCTLW0.a.bSWRST = 0;  //enable the SPI module

//	EUSCI_A2->rTXBUF.a.bTXBUF = 0;
//	EUSCI_A2->rIE.a.bTXIE = 1; //enable tx shift complete interrupt

//	NVIC_EnableIRQ(EUSCIA2_IRQn);
//	NVIC_SetPriority(EUSCIA2_IRQn, 0); //set the interrupt to the highest priority
}

void WS2812_stripSetLED(uint16_t ledNr, uint8_t r, uint8_t g, uint8_t b) {

	uint16_t baseIndex = (ledNr * 9);

	//Disable interrupts while writing led data. This is so that we don't get invalid data in the buffer
//	__disable_interrupts();
	uint8_t writeBuffer = frameBufferIndex ^ 1;

	frameBuffer[writeBuffer][baseIndex] = ledLookupTableHighByte[g];
	frameBuffer[writeBuffer][baseIndex + 1] = ledLookupTableMiddleByte[g];
	frameBuffer[writeBuffer][baseIndex + 2] = ledLookupTableLowByte[g];

	frameBuffer[writeBuffer][baseIndex + 3] = ledLookupTableHighByte[r];
	frameBuffer[writeBuffer][baseIndex + 4] = ledLookupTableMiddleByte[r];
	frameBuffer[writeBuffer][baseIndex + 5] = ledLookupTableLowByte[r];

	frameBuffer[writeBuffer][baseIndex + 6] = ledLookupTableHighByte[b];
	frameBuffer[writeBuffer][baseIndex + 7] = ledLookupTableMiddleByte[b];
	frameBuffer[writeBuffer][baseIndex + 8] = ledLookupTableLowByte[b];

//	__enable_interrupts();
}

void WS2812_matrixSetPixelUnsafe(uint16_t x, uint16_t y, uint8_t r, uint8_t g,
		uint8_t b) {
	if ((x % 2) == 1) {
		y = 7 - y;
	}
	WS2812_stripSetLED((x * 8) + y, r, g, b);
}

void WS2812_clearBuffer() {
	uint16_t i;
	for (i = 0; i < WS2812_LED_COUNT; i++) {
		WS2812_stripSetLED(i, 0, 0, 0);
	}

	for (i = WS2812_LED_COUNT * 9; i < LED_BUFFER_SIZE; i++) {
		frameBuffer[frameBufferIndex][i] = 0;
	}

}

volatile uint8_t WS2812_isWriteReady() {
	if(getTimerValue() > 30){
		swapQueued=0;
		stopTimer();
	}
	return !swapQueued;
}

void WS2812_swapBuffer() {
	swapQueued = 1;
	startTimer();
}

void WS2812_initDisplay() {
	initTimeoutTimer();
	frameBufferIndex = 0;
	shiftPosition = 0;

	displayShiftBuffer = frameBuffer[frameBufferIndex];
	initLookupTable();
	WS2812_clearBuffer();
	frameBufferIndex = 1;
	WS2812_clearBuffer();
	frameBufferIndex = 0;

	DIO->rPBSEL0.b.bP3SEL0 |= 0x08;
	DIO->rPBSEL1.b.bP3SEL1 &= ~0x08;

	initSPI();
	WS2812_initDMA();

	__enable_interrupts();

	WS2812_swapBuffer();
}



