//*****************************************************************************
//
// MSP432 main.c template - Empty main
//
//****************************************************************************

#include "msp.h"
#include "WS2812_display/ws2812_display.h"
#include "gfx/gfx.h"
#include "ADC/adc.h"



#define USE_ARM_MATH

#define ARM_MATH_CM4
#include "arm_math.h"
#include "math.h"
#include "random.h"


void waitABit() {
	uint32_t i;
	for (i = 0; i < 500000; i++) {
		__no_operation();
	}
}

void initClock() {
	//TODO: There seems to be a bug with the external crystal on the msp432. Need to investigate with oscilloscope.
	//			After oscilloscpe investigation I still have no idea.

	//Up the core voltage
	PCM->rCTL0.r = 0x695A0001;

	while ( PCM->rCTL1.b.bPMR_BUSY) {
		;
	}

	DIO->rPJSEL0 |= 0x0c;
	DIO->rPJSEL1 &= ~0x0c;

	//unlock clock control registers
	CS->rKEY.b.bKEY = 0x695A;

	//set SMCLK to src(SMCLK)/2 in preparation of switching its source to 48MHz
	CS->rCTL1.b.bDIVS = 0x1;

	//set the oscillator to 48MHz
	CS->rCTL0.b.bDCORSEL = 0x5;

	/*	//disable fault clear counter
	 CS->rCTL3.b.bFCNTHF_EN = 0;

	 //switch high drive
	 CS->rCTL2.b.bHFXTDRIVE = 1;

	 //change frequency range of HFXT to 40-48MHz
	 CS->rCTL2.b.bHFXTFREQ = 6;

	 //enable the high frequency crystal
	 CS->rCTL2.b.bHFXT_EN = 1;

	 //clear the failt flag
	 CS->rCLRIFG.b.bCLR_HFXTIFG = 1;

	 waitABit();
	 */
	//switch MCLK to external 48MHz crystal
//	CS->rCTL1.b.bSELM = 5;
	//switch SMCLK to external 48MHz crystal
//	CS->rCTL1.b.bSELS = 5;
	//lock clock control registers (this can be anything except 0x695A)
	CS->rKEY.b.bKEY = 0xBEEF;
}

void initLFXT() {

	DIO->rPJSEL0 |= 0x0003;
	DIO->rPJSEL1 &= ~0x0003;

	//unlock clock control registers
	CS->rKEY.b.bKEY = 0x695A;

	waitABit();

	CS->rCTL3.b.bFCNTLF_EN = 0;
	CS->rCTL2.b.bLFXT_EN = 1;
	CS->rCLRIFG.b.bCLR_FCNTLFIFG = 1;
	CS->rCLRIFG.b.bCLR_LFXTIFG = 1;

	waitABit();

	CS->rCLRIFG.b.bCLR_LFXTIFG = 1;

	waitABit();

	if (CS->rIFG.b.bLFXTIFG) {
		while (1) {
			;
		}
	}

	CS->rCTL1.b.bSELA = 0;

	//lock clock control registers (this can be anything except 0x695A)
	CS->rKEY.b.bKEY = 0xBEEF;
}

uint32_t colourIndex = 0;

#define PALETTE_SIZE 64
rgb palette[PALETTE_SIZE];

//setColourRGBf(palette[index].r, palette[index].g, palette[index].b);
void calcPalette() {
	uint16_t i;
	float palette_scale = 1.0f / PALETTE_SIZE;
	for (i = 0; i < PALETTE_SIZE; i++) {
		hsv in;
		in.h = i * palette_scale;
		in.s = 1.0;
		in.v = 1.0;
		palette[i] = hsv2rgb(in);
	}
}

void drawFlatBackground(float offset, float saturation, float brightness,
		float scale) {
	offset -= floorf(offset);

	float xScale = 1.0f / (GFX_WIDTH * scale);

	uint32_t x, y;
	for (x = 0; x < GFX_WIDTH; x++) {
		float posInHue = offset + (x * xScale);
		setColourHSVf(posInHue, saturation, brightness);
		for (y = 0; y < GFX_HEIGHT; y++) {
			drawPixel(x, y);
		}
	}
}

void setUpUART() {
	EUSCI_A1->rCTLW0.b.bSWRST = 1;

	//switch pin 2.2/2.3 to Rx/Tx
	DIO->rPASEL0.b.bP2SEL0 |= 0x0c;
	DIO->rPASEL1.b.bP2SEL1 &= ~0x0c;

	EUSCI_A1->rCTLW0.b.bSSEL = 2;

	//calc from datasheet to give 9600 baud from 24MHz smclk
	EUSCI_A1->rBRW = 156;
	EUSCI_A1->rMCTLW.b.bOS16 = 1;
	EUSCI_A1->rMCTLW.b.bBRS = 0x44;
	EUSCI_A1->rMCTLW.b.bBRF = 4;

	EUSCI_A1->rCTLW0.b.bSWRST = 0;
}
#ifdef USE_ARM_MATH
float getValueForCoord(float x, float y, float time) {
	//	float value = arm_sin_f32(x * 10 + time);

	float value =0.0;
	value += arm_sin_f32(x * 10 + time);
	value += arm_sin_f32(10*(x*arm_sin_f32(time/2) + y*arm_cos_f32(time/3))+time);

	float cx = x+0.5*arm_sin_f32(time/5);
	float cy = y+0.5*arm_sin_f32(time/3);

	value += arm_sin_f32( sqrtf(100 * ((cx*cx)+(cy*cy))+1)+time);

	return value;
}

#else

float getValueForCoord(float x, float y, float time) {
//	float value = sinf(x * 10 + time);

	float value = 0.0;
	value += sinf(x * 10 + time);
	value += sinf(10 * (x * sinf(time / 2) + y * cosf(time / 3)) + time);

	float cx = x + 0.5 * sinf(time / 5);
	float cy = y + 0.5 * sinf(time / 3);

	value += sinf(sqrtf(100 * ((cx * cx) + (cy * cy)) + 1) + time);

	return value;
}
#endif

void drawPlasma(float time, float saturation, float value) {
	uint8_t x, y;

	for (x = 0; x < GFX_WIDTH; x++) {
		float xValue = (x / (GFX_WIDTH * 1.0f)) - 0.5;
		for (y = 0; y < GFX_HEIGHT; y++) {
			float yValue = ((y + 12) / (GFX_WIDTH * 1.0f)) - 0.5;

			float val = getValueForCoord(xValue, yValue, time) * 0.5;
			val += 0.5;

			setColourHSVf(val, saturation, value);

			drawPixel(x, y);
		}
	}
}

uint16_t lastTime;


uint8_t random(){

	uint8_t i;
	uint8_t result = 0;

	uint16_t * adcSamples = ADC_busyRead();

	for(i=0;i<4;i++){
		result |= (adcSamples[i] & 0x01) << i;
	}

	adcSamples = ADC_busyRead();

	for(i=0;i<4;i++){
		result |= (adcSamples[i] & 0x01) << (i+4);
	}
	return result;
}

void initRandom(){

	uint32_t seed = 0;

	while(seed==0){
		seed = 0;
			seed |= random() << 24;
			seed |= random() << 16;
			seed |= random() << 8;
			seed |= random() << 0;
	}

	srand( seed );

}

void initTimer() {
	TIMER_A0->rCTL.b.bSSEL = 1; //aclk
	TIMER_A0->rEX0.b.bIDEX = 3; // div_4
	TIMER_A0->rCTL.b.bID = 3; //div by 8
	TIMER_A0->rCTL.b.bCLR = 1; //clear the timer
	TIMER_A0->rR = 0;
	lastTime = 0;

	TIMER_A0->rCTL.b.bMC = 2; //continuous;
}

#define FLOAT_DIV_1024 0.0009765625f

float getTimeDelta() {
	float result = 0;

	uint16_t currentTime = TIMER_A0->rR;
	int32_t delta = currentTime - lastTime;

	if (delta < 0) {
		//something went wrong. Reset everything
		TIMER_A0->rCTL.b.bMC = 0; //stop timer;
		lastTime = 0;
		TIMER_A0->rR = 0;
		TIMER_A0->rCTL.b.bMC = 2; //resume timer;
	} else {
		result = delta * FLOAT_DIV_1024;
	}

	lastTime = currentTime;

	if (currentTime > 30000) {
		TIMER_A0->rCTL.b.bMC = 0; //stop timer;
		TIMER_A0->rR -= 30000;
		TIMER_A0->rCTL.b.bMC = 2; //resume timer;
		lastTime -= 30000;
	}

	return result;

}




#define MESSGAE_COUNT 40


const char messages[MESSGAE_COUNT][150] = {
		"Lunch?",
		"",
		"",
		"",
		"",

		"",
		"",
		"",
		"",
		"",

//		"Thanks Jay for the great story!",
//		"Welcome to the Skeptics Guide to the Universe!",
//		"Go SGU!",
//		"Just a normal hat.",
//		"I don't think the cat is a fan of this hat...",
//
//
//		"Hope to see you all at TAM 2017!",


		"",
		"",
		"",
		"",
		"",

		"",
		"",
		"",
		"",
		"",

		"",
		"",
		"",
		"",
		"",

		"",
		"",
		"",
		"",
		"",

		"",
		"",
		"",
		"",
		"",

		"",
		"",
		"",
		"",
		""
};

/*

const char messages[MESSGAE_COUNT][150] = {
//---------------------------------------------------------------------------------------------------------------------------------- Message end ->|
"Nõo",
"Terviseks!",
"Completely ordinary hat.",
"This is a fascinator. Nothing to see here, move along...",
"HAT HAT HAT HAT HAT (hat)",

"[ERROR IN LEXICALISATION]",
"Hufflepuff",
"Much wedding, such wow",
"Good luck Schlakeith",
"Should have gone with Twitter",

"My other hats a...",
"01101101 01110101 01100011 01101000 00100000 01110111 01100101 01100100 01100100 01101001 01101110 01100111 (much wedding)",
"One does not simply wear a normal hat to get married in.",
"It's hard life being a cool hat!",
"Huzzah! Sheldon Cooper would be proud",

"Paintball: Everyone vs Michael?",
"Congratulations [NO LEX RULES WHICH COULD BE SATISFIED WERE FOUND FOR ValueLexRule name]!",
"Do you like Huey Lewis and The News?",
"Keep Calm and Get Married!",
"Feed me a stray cat.",

"Hello!",
"Laura & Michael 2015",
"Help! I'm trapped in a hat!",
"Tere Tere!",
"void waitABit() { uint32_t i; for (i = 0; i < 500000; i++) { __no_operation(); } }",

"Happy birthd--uhh--- wedding!",
"Ada",
"Bash",
"Clojure",
"Dart",

"Erlang",
"Forth",
"Groovy",
"Haskell",
"Idris",

"",
"",
"",
"",
""



};
*/


hsv randomColour;


void getRandomColour(){

	float textBrightness = 0.35;
	if(randf() < 0.075){
		randomColour.h = 0;
		randomColour.s = 0;
		randomColour.v = 0;
	}else{

		randomColour.h = randf();
		randomColour.s = (1.0f - (randf() / 4));
		randomColour.v = textBrightness;

	}

}

uint16_t getRandomMessageNumber(){
	return  rand() % MESSGAE_COUNT;
}


#define MESSAGE_COLOR_COUNT 6
hsv messageColours[MESSAGE_COLOR_COUNT];

uint8_t readButton(){
	return ! (DIO->rPAIN.b.bP1IN & 0x02);
}

void main(void) {

	//	WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer

	initClock();
	initLFXT();

	SCB_CPACR = 0xf << 20; //enable the fpu

	DIO->rPADIR.b.bP2DIR = 0x07; //set onboard rgb led pins to output
	DIO->rPAOUT.b.bP2OUT = 0x00; //set onboard rgb led pins to off

	DIO->rPADIR.b.bP1DIR &= ~0x02; //button
	DIO->rPAREN.b.bP1REN |= 0x02; //button
	DIO->rPAOUT.b.bP1OUT |= 0x02; //button


	DIO->rPAOUT.b.bP2OUT |= 0x01;

	setUpUART();
//	testInitDMA();

//	PMAP->rKEYID = 0x2D52;
//	PMAP->rP2MAP23 = PM_UCA1SIMO;
//	PMAP->rP3MAP23 |= PM_UCA2SIMO;
//	PMAP->rKEYID = 0x00;

	initDisplay();
	setDisplayCurrentLimit(1.6f);



	ADC_init();

	ADC_setNumberOfChannels(4);
	ADC_setChannelSource(0, 15);
	ADC_setChannelSource(1, 14);
	ADC_setChannelSource(2, 13);
	ADC_setChannelSource(3, 8);

	initRandom();

//	uint16_t testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();
//	testRandom = getRandomMessageNumber();


	uint8_t lastButtonState = 0;

	uint16_t increment = 0;

	calcPalette();

	initTimer();
	getTimeDelta();
	float time = 0;

	float scrollRate = 25;
	float scrollStart = 0.0;

	int messageNumber = getRandomMessageNumber();
	int scrollTick = 0;
	int scrollDiv = 1;
	int scrollPosition = GFX_WIDTH;

	int staticHatIndex = 0;

	float textBrightness = 0.3;

	messageColours[staticHatIndex].h = 0.0;
	messageColours[staticHatIndex].s = 1.0;
	messageColours[staticHatIndex++].v = textBrightness;

	messageColours[staticHatIndex].h = 0.2;
	messageColours[staticHatIndex].s = 1.0;
	messageColours[staticHatIndex++].v = textBrightness;

	messageColours[staticHatIndex].h = 0.4;
	messageColours[staticHatIndex].s = 1.0;
	messageColours[staticHatIndex++].v = textBrightness;

	messageColours[staticHatIndex].h = 0.6;
	messageColours[staticHatIndex].s = 1.0;
	messageColours[staticHatIndex++].v = textBrightness;

	messageColours[staticHatIndex].h = 0.8;
	messageColours[staticHatIndex].s = 1.0;
	messageColours[staticHatIndex++].v = textBrightness;

	messageColours[staticHatIndex].h = 0.5;
	messageColours[staticHatIndex].s = 1.0;
	messageColours[staticHatIndex++].v = 0.0;

	getRandomColour();

	int showMessage = 0;

	int currentMessageColour = rand() % MESSAGE_COLOR_COUNT;

	while (1) {

		time += getTimeDelta();
		clearDisplay();
//		drawFlatBackground(0.5, 1.0, 0.1, 2.0);
		drawPlasma(time, 1.0, 0.05);

//		int16_t fontX = 0;

		if (showMessage) {
			setColourHSVf(randomColour.h, randomColour.s, randomColour.v);

			scrollTick++;
			if (scrollTick % scrollDiv == 0) {
				scrollPosition--;
				scrollTick = 0;
			}
			int textXPos = scrollPosition;

			textXPos = drawString(messages[messageNumber], textXPos, 0);

			if (textXPos < 0) {
//			scrollStart = time;
				scrollPosition = GFX_WIDTH;
				scrollTick = 0;
			messageNumber = (messageNumber + 1) % MESSGAE_COUNT;
//				messageNumber = getRandomMessageNumber();

				//skip empty ones
				while (messages[messageNumber][0] == '\0') {
//					messageNumber = getRandomMessageNumber();

					messageNumber = (messageNumber + 1) % MESSGAE_COUNT;
				}

				currentMessageColour = rand() % MESSAGE_COLOR_COUNT;
				getRandomColour();
			}

		}
		normaliseAndSwapBuffer();
//		EUSCI_A1->rTXBUF.a.bTXBUF = 'f';

		if (time > 600) {
			time = 0;
//			scrollStart = 0;
		}
//		increment = (increment + 1) % 2;
//
//		if (increment == 0) {
//			;
//		}
//		colourIndex = (colourIndex + 1) & 0x3f;

//		DIO->rPAOUT.b.bP2OUT ^= 0x01;

		uint8_t button  = readButton();
		if( button && !lastButtonState){
			//transition on
			DIO->rPAOUT.b.bP2OUT ^= 0x01;
			DIO->rPAOUT.b.bP2OUT ^= 0x02;
			showMessage ^= 0x01;

		}else if( !button && lastButtonState){
			//transition off;
		}

		lastButtonState = button;

	}

}
