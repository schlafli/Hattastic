/*
 * ws2812_display.h
 *
 *  Created on: 28 Jun 2015
 *      Author: Michael
 */

#ifndef WS2812_DISPLAY_WS2812_DISPLAY_H_
#define WS2812_DISPLAY_WS2812_DISPLAY_H_

#include "inttypes.h"


#define WS2812_MATRIX_WIDTH 32
#define WS2812_MATRIX_HEIGHT 8

#define WS2812_LED_COUNT (WS2812_MATRIX_WIDTH * WS2812_MATRIX_HEIGHT)
#define WS2812_STANDBY_CURRENT_CONSUMPTION 0.0007f
#define WS2812_LED_TOTAL_COUNT (WS2812_LED_COUNT * 3)

void WS2812_initDisplay();

volatile uint8_t WS2812_isWriteReady();

void WS2812_swapBuffer();

void WS2812_matrixSetPixelUnsafe(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);

#endif /* WS2812_DISPLAY_WS2812_DISPLAY_H_ */
