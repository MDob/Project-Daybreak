#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_spi.h"
#include "misc.h"

#define SPI_PORT 			SPI2
#define SPI_MOSI			GPIO_Pin_15
#define SPI_MOSI_PS			GPIO_PinSource15
#define SPI_CLK 			GPIO_Pin_13
#define SPI_CLK_PS			GPIO_PinSource13
#define SPI_CS 				GPIO_Pin_12
#define SPI_GPIO 			GPIOB
#define SPI_AF				GPIO_AF_SPI2
#define SPI_PERIPH_CLK 		RCC_APB1Periph_SPI2

#define DISP_TIM TIM4
#define DISP_TIM_PERIPH_CLK	RCC_APB1Periph_TIM4
#define DISP_TIM_PERIOD 	8000
#define DISP_TIM_PRESCALER 	20
#define DISP_TIM_CLKDIV 	0
#define DISP_TIM_CTRMODE 	TIM_CounterMode_Up

#define DISP_NVIC_IRQn 		TIM4_IRQn
#define DISP_NVIC_PREMP 	0
#define DISP_NVIC_SUB 		1
#define DISP_NVIC_CMD 		ENABLE

TIM_TimeBaseInitTypeDef DISP_TIM_Struct;
NVIC_InitTypeDef 		DISP_NVIC_Struct;

typedef enum{SPI_DATA_SENT, SPI_LATCH_HIGH, COND_3} spi_interrupt_flag;

typedef union SPI_Display_Union{
	uint16_t data[5];

	struct{
		uint16_t one;
		uint16_t two;
		uint16_t three;
		uint16_t four;
		uint16_t colon;
	}digit;
}Display_Data;

extern Display_Data current_display;
extern const uint16_t number_array[12];
extern const uint16_t digit_array[5];
extern const uint16_t alarm_mask;
extern const uint16_t pm_mask;

void Display_SPI_Init(void);

void Display_Timer_Init(void);


#endif /* DISPLAY_H */
