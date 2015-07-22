/*
 * gfx.h
 *
 *  Created on: 30 Jun 2015
 *      Author: Michael
 */

#ifndef GFX_GFX_H_
#define GFX_GFX_H_

#include "../WS2812_display/ws2812_display.h"



#define GFX_WIDTH WS2812_MATRIX_WIDTH
#define GFX_HEIGHT WS2812_MATRIX_HEIGHT


typedef struct {
	float r;       // 0-1.0
	float g;       // 0-1.0
	float b;       // 0-1.0
} rgb;

typedef struct {
	float h;       // 0-1.0 (or anything really)
	float s;       // 0-1.0
	float v;       // 0-1.0
} hsv;

hsv rgb2hsv(rgb in);
rgb hsv2rgb(hsv in);




void initDisplay();

void setDisplayCurrentLimit( float amps );

// this function will block if we've already swapped in the current refresh period
void normaliseAndSwapBuffer();

void clearDisplay();

void setColourRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void setColourRGBAf(float r, float g, float b, float a);

void setColourRGB(uint8_t r, uint8_t g, uint8_t b);
void setColourRGBf(float r, float g, float b);

void setColourHSV(uint16_t h, uint8_t s, uint8_t v);
void setColourHSVf(float h, float s, float v);




void drawPixel(int16_t x, int16_t y);

void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

int16_t drawChar(uint8_t c, int16_t x, int16_t y);

int16_t drawString(uint8_t * string, int16_t x, int16_t y);

/*

void drawRectangle(x1, y1, x2, y2);

void fillRectangle(x1, y1, x2, y2);

void drawTriangle(x1, y1, x2, y2, x3, y3);

void fillTriangle(x1, y1, x2, y2, x3, y3);

*/



#endif /* GFX_GFX_H_ */
