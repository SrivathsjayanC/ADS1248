/*
 * delay_us.h
 *
 *  Created on: Oct 10, 2026
 *      Author: SRIVATHSJAYAN
 */

#ifndef INC_DELAY_US_H_
#define INC_DELAY_US_H_

#include "main.h"

extern TIM_HandleTypeDef htim10;

void delay_us(uint16_t us);

#endif /* INC_DELAY_US_H_ */
