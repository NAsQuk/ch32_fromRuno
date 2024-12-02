/*
 * board.h
 *
 *  Created on: Jul 31, 2010
 *      Author: albert
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "stm32f10x_conf.h"

#define RUNO

#define GPIO_LEDWORK GPIOA
#define GPIO_LED	GPIOC

#define GPIO_RELE	GPIOA
#define GPIO_D1	GPIOA
#define GPIO_D2toD5	GPIOC
#define GPIO_D6toD9	GPIOB
#define GPIO_D10toD11	GPIOC

#define LEDWORK		GPIO_Pin_7
#define LEDGREEN	GPIO_Pin_2
#define LEDRED		GPIO_Pin_3

#define P1           GPIO_Pin_4
#define P2           GPIO_Pin_5
#define P3           GPIO_Pin_6

/*#define P1           GPIO_Pin_6
 #define P2           GPIO_Pin_5
 #define P3           GPIO_Pin_4*/

// Leds
#define GPIO_Led     GPIOE
#define Led1_1         GPIO_Pin_2
#define Led1_2         GPIO_Pin_3
#define Led2_1         GPIO_Pin_4
#define Led2_2         GPIO_Pin_5
#define Led3_2         GPIO_Pin_6

#define GPIO_Led3     GPIOD
#define Led3_1         GPIO_Pin_7


#define D11         GPIO_Pin_4
#define D10         GPIO_Pin_5
#define D9          GPIO_Pin_1
#define D8          GPIO_Pin_2
#define D7          GPIO_Pin_12
#define D6          GPIO_Pin_15
#define D5          GPIO_Pin_6
#define D4          GPIO_Pin_7
#define D3          GPIO_Pin_8
#define D2          GPIO_Pin_9
#define D1          GPIO_Pin_10
extern int initMK;
extern int manualwork;
#endif /* BOARD_H_ */
