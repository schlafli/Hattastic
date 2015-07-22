/*
 * random.h
 *
 *  Created on: 21 Jul 2015
 *      Author: Michael
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include "stdint.h"

#define RAND_MAX ((1U << 31) - 1)

void srand(uint32_t x);

uint32_t rand();

float randf();

#endif /* RANDOM_H_ */
