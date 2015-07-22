/*
 * gfx.c
 *
 *  Created on: 30 Jun 2015
 *      Author: Michael
 */

#include "gfx.h"
#include "math.h"
#include "msp.h"
#include "charset.h"

#define SUB_PIXEL_TOTAL_COUNT (GFX_WIDTH * GFX_HEIGHT * 3)

#define sgn(x) ((x<0)?-1:((x>0)?1:0))
#define abs(x) ((x<0)?-x:x)

//(1/360)
#define FLOAT_HUE_SCALE 0.0027777778f


float drawBuffer[SUB_PIXEL_TOTAL_COUNT];

float _red = 0.0f;
float _green = 0.0f;
float _blue = 0.0f;
float _alpha = 0.0f;

//(1/255)
#define FLOAT_PIXEL_SCALE  0.003921569f

float maxCurrentAmps = 1.0;

#define MAX_STRING_LENGTH 255

uint16_t mystrlen(uint8_t * string){
	uint16_t i=0;
	while(string[i]!='\0' && i<MAX_STRING_LENGTH){
		i++;
	}
	return i;
}

void setDisplayCurrentLimit(float amps) {

	float standbyCurrent = WS2812_STANDBY_CURRENT_CONSUMPTION * GFX_WIDTH * GFX_HEIGHT;

	//0.2 amps minimum (number totally pulled out of ass)
	if ((amps - standbyCurrent) < 0.2) {
		amps = standbyCurrent + 0.2;
	}

	maxCurrentAmps = amps - standbyCurrent;

}


hsv rgb2hsv(rgb in) {
	hsv out;
	float min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max > in.b ? max : in.b;

	out.v = max; // v
	delta = max - min;
	if (max > 0.0) {
		out.s = (delta / max); // s
	} else {
		// r = g = b = 0                        // s = 0, v is undefined
		out.s = 0.0;
		out.h = NAN; // its now undefined
		return out;
	}
	if (in.r >= max) // > is bogus, just keeps compilor happy
		out.h = (in.g - in.b) / delta; // between yellow & magenta
	else if (in.g >= max)
		out.h = 2.0 + (in.b - in.r) / delta; // between cyan & yellow
	else
		out.h = 4.0 + (in.r - in.g) / delta; // between magenta & cyan

	out.h *= 60.0; // degrees

	if (out.h < 0.0)
		out.h += 360.0;

	out.h *= FLOAT_HUE_SCALE;

	return out;
}

rgb hsv2rgb(hsv in) {
	float hh, p, q, t, ff;
	long i;
	rgb out;

	if (in.s <= 0.0) { // < is bogus, just shuts up warnings
		if (isnan(in.h)) { // in.h == NAN
			out.r = in.v;
			out.g = in.v;
			out.b = in.v;
			return out;
		}
		// error - should never happen
		out.r = 0.0;
		out.g = 0.0;
		out.b = 0.0;
		return out;
	}
	hh = in.h;

	hh = (hh - floorf(in.h)) * 360;

	while (hh >= 360) {
		hh -= 360.0;
	}

	hh /= 60.0;
	i = (long) hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}



void _drawPixelUnsafe(int16_t x, int16_t y) {

	x += y * GFX_WIDTH;
	x *= 3;

	drawBuffer[x] = _red;
	drawBuffer[x + 1] = _green;
	drawBuffer[x + 2] = _blue;

}

void clearDisplay() {
	uint16_t i;
	for (i = 0; i < SUB_PIXEL_TOTAL_COUNT; i++) {
		drawBuffer[i] = 0.0f;
	}
}

void normaliseAndSwapBuffer() {
	while (!WS2812_isWriteReady()) {
		__no_operation();
	}

	float total = 0;
	uint16_t i, x, y, baseIndex;
	for (i = 0; i < SUB_PIXEL_TOTAL_COUNT; i++) {
		total += drawBuffer[i];
	}

	//each LED is 20ma
	total *= 0.02;

	if (total > maxCurrentAmps) {
		float normalise = maxCurrentAmps / total;
		for (i = 0; i < SUB_PIXEL_TOTAL_COUNT; i++) {
			drawBuffer[i] *= normalise;
		}
	}

	for (y = 0; y < GFX_HEIGHT; y++) {
		if(y>=WS2812_MATRIX_HEIGHT){
			break;
		}

		i = y * GFX_WIDTH;
		for (x = 0; x < GFX_WIDTH ; x++) {
			if(x >= WS2812_MATRIX_WIDTH){
				break;
			}

			baseIndex = (i + x) * 3;

			uint8_t r = roundf(drawBuffer[baseIndex] * 255);
			uint8_t g = roundf(drawBuffer[baseIndex + 1] * 255);
			uint8_t b = roundf(drawBuffer[baseIndex + 2] * 255);

			WS2812_matrixSetPixelUnsafe(x, y, r, g, b);
		}

	}

	WS2812_swapBuffer();
}

inline void _setRGBA(float r, float g, float b, float a) {
	_red = r;
	_green = g;
	_blue = b;
	_alpha = a;
}

void setColourRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	_setRGBA(r*FLOAT_PIXEL_SCALE, g*FLOAT_PIXEL_SCALE, b*FLOAT_PIXEL_SCALE, a*FLOAT_PIXEL_SCALE);
}

void setColourRGBAf(float r, float g, float b, float a) {
	_setRGBA(r,g,b,a);
}

void setColourRGB(uint8_t r, uint8_t g, uint8_t b) {
	_setRGBA(r*FLOAT_PIXEL_SCALE, g*FLOAT_PIXEL_SCALE, b*FLOAT_PIXEL_SCALE, 1.0f);
}

void setColourRGBf(float r, float g, float b) {
	_setRGBA(r, g, b, 1.0f);
}

void setColourHSV(uint16_t h, uint8_t s, uint8_t v) {
	setColourHSVf(h * FLOAT_HUE_SCALE, s * FLOAT_PIXEL_SCALE, v * FLOAT_PIXEL_SCALE);
}

void setColourHSVf(float h, float s, float v) {
	hsv tmp;
	tmp.h = h;
	tmp.s = s;
	tmp.v = v;

	rgb result = hsv2rgb(tmp);
	_setRGBA(result.r, result.g, result.b, 1.0);
}



void drawPixel(int16_t x, int16_t y) {
	if(x<0 || x>= GFX_WIDTH || y<0 || y>=GFX_HEIGHT){
		return;
	}

	x += y * GFX_WIDTH;
	x *= 3;

	drawBuffer[x] = _red;
	drawBuffer[x + 1] = _green;
	drawBuffer[x + 2] = _blue;

}


void initDisplay(){
	WS2812_initDisplay();
	clearDisplay();
	normaliseAndSwapBuffer();
}



void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	drawPixel(x1,y1);
	drawPixel(x2,y2);

	int16_t dxabs,dyabs,px,py,x,y,sdx,sdy,i,dx,dy;

	dx=x2-x1;      /* the horizontal distance of the line */
	dy=y2-y1;      /* the vertical distance of the line */
	dxabs=abs(dx);
	dyabs=abs(dy);
	sdx=sgn(dx);
	sdy=sgn(dy);
	x=dyabs>>1;
	y=dxabs>>1;
	px=x1;
	py=y1;

	if (dxabs>=dyabs){ /* the line is more horizontal than vertical */
		for(i=0;i<dxabs;i++){
			y+=dyabs;
			if (y>=dxabs){
				y-=dxabs;
				py+=sdy;
			}
			px+=sdx;
			drawPixel(px,py);
		}
	}else{ /* the line is more vertical than horizontal */
		for(i=0;i<dyabs;i++){
			x+=dxabs;
			if (x>=dyabs){
				x-=dyabs;
				px+=sdx;
			}
			py+=sdy;
			drawPixel(px,py);
		}
	}
}


int16_t drawChar(uint8_t c, int16_t x, int16_t y){
	if(c<32 || (c-32) > charcount){
		c=127;
	}

	c-=32;

	uint16_t xc, yc;

	uint8_t fontSize = 7;

	if(x<-fontSize || x> GFX_WIDTH || y<-8 || y>GFX_HEIGHT){
		return x+fontSize;
	}

	for(xc=0;xc<fontSize;xc++){
//		uint8_t slice = (big)?font7x8[c][xc]:font5x7[c][xc];
		uint8_t slice = font7x8[c][xc];
		for(yc=0;yc<8;yc++){
			if(slice & (1<<yc)){
				drawPixel(x+xc, y+yc);
			}
		}
	}

	return x+fontSize;
}

int16_t drawString(uint8_t * string, int16_t x, int16_t y){

	int16_t length = mystrlen(string);

	uint8_t fontSize = 7;

	if(y>GFX_HEIGHT || y<-8 || x>GFX_WIDTH){
		return (length * fontSize) + x;
	}

	int16_t end = (length * fontSize) + x;

	if(end<0){
		return end;
	}

	uint16_t i;
	for(i=0;i<length;i++){
		drawChar(string[i], (i*fontSize)+x, y);
	}

	return end;
}
