
#include "stm32f4xx_rtc.h"

#ifndef MAIN_H_
#define MAIN_H_


#define TIMEBUTTON 0xB20		//(GPIOC->IDR & GPIO_Pin_4)
#define TIMEHOURBUTTON 0xA20	//(GPIOC->IDR & GPIO_Pin_8)
#define TIMEMINBUTTON 0x320		//(GPIOC->IDR & GPIO_Pin_11)
#define ALARMBUTTON 0x930		//(GPIOC->IDR & GPIO_Pin_9)
#define MODEBUTTON 0xA30//0xB32		//(GPIOC->IDR & GPIO_Pin_6)
#define BUTTON		0xB10		//(GPIOC->IDR & GPIO_Pin_5)
#define SNOOZEBUTTON 0x330//0xB70		//(GPIOC->IDR & GPIO_Pin_1)
#define ALARMHOURBUTTON 0x830
#define ALARMMINBUTTON 0x130
#define NOBUTTON 0xB30

#define TIME_PLUS_GPIO 	GPIOA
#define TIME_PLUS_PIN 	GPIO_Pin_8
#define TIME_MINUS_GPIO GPIOC
#define TIME_MINUS_PIN 	GPIO_Pin_6
#define TIME_SET_GPIO 	GPIOC
#define TIME_SET_PIN 	GPIO_Pin_8
#define SNOOZE_GPIO 	GPIOC
#define SNOOZE_PIN 		GPIO_Pin_9

//RTC structures
RTC_InitTypeDef		myclockInitTypeStruct;
RTC_TimeTypeDef		RTC_Global_Struct;
RTC_AlarmTypeDef	AlarmStruct;
RTC_AlarmTypeDef 	alarmMemory;

//Other structures
RCC_ClocksTypeDef 		RCC_Clocks;
GPIO_InitTypeDef		GPIOInitStruct;
TIM_TimeBaseInitTypeDef TIM_InitStruct;
NVIC_InitTypeDef 		NVIC_InitStructure;
EXTI_InitTypeDef 		EXTI_InitStructure;

extern int interruptOccurred;


#endif /* MAIN_H_ */
