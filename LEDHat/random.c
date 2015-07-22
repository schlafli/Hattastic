/*
 * random.c
 *
 *  Created on: 21 Jul 2015
 *      Author: Michael
 */

#include "random.h"

uint32_t rand();
uint32_t rseed = 0;

void srand(uint32_t x)
{
	rseed = x;
}

uint32_t rand()
{
	return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}

float randf()
{
	return (rand() % 65535) / 65535.0f;
}




