//This program uses the RTC to display time on a 4 digit 7 segment display
//When the alarm triggers, it plays mp3 files through a USB connected on the
//micro USB port

#include "stm32f4xx_rtc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_syscfg.h"
#include "misc.h"
#include "stm32f4xx_exti.h"
#include "audioMP3.h"
#include "main.h"
#include "display.h"
#include "FSM.h"

//function prototypes
void configuration(void);

//global variables
int interruptOccurred = 0;
volatile int exitMp3 = 0;
extern volatile int mp3PlayingFlag = 0;
extern volatile int snoozeMemory = 0;

/*for testing
uint32_t *ptr;
uint32_t *ptr2;*/

int main(void)
{
	configuration();

	while ( 1 )
	{
		mp3PlayingFlag = 1;
		audioToMp3();
	}
}

// Performs de-bouncing on the buttons and updates blink flag
void TIM3_IRQHandler(void)
{
	static uint32_t plus_counter 	= 0;
	static uint32_t minus_counter 	= 0;
	static uint32_t set_counter 	= 0;
	static uint32_t snooze_counter 	= 0;
	static uint32_t blink_counter 	= 0;

	//double checks that interrupt has occurred
	if( TIM_GetITStatus( TIM3, TIM_IT_Update ) != RESET )
	{
		//clears interrupt flag
		TIM3->SR 		= (uint16_t)~TIM_IT_Update;

		// Poll buttons for current state
		button_state[TIME_SET] 			= GPIO_ReadInputDataBit(TIME_SET_GPIO, TIME_SET_PIN);
		button_state[TIME_PLUS] 		= GPIO_ReadInputDataBit(TIME_PLUS_GPIO, TIME_PLUS_PIN);
		button_state[TIME_MINUS] 		= GPIO_ReadInputDataBit(TIME_MINUS_GPIO, TIME_MINUS_PIN);
		button_state[SNOOZE_BUTTON] 	= GPIO_ReadInputDataBit(SNOOZE_GPIO, SNOOZE_PIN);

		// Increment blink_counter
		blink_counter++;

		// Change blink state every 0.5s
		if((blink_counter >= 250) || flags[time_set] || flags[alarm_set]){
			flags[blink] = !flags[blink];
			blink_counter = 0;
		}

		// Increment button-specific counters when button is pressed
		// Reset counter if button is released

		// Time+ Case
		// Flag is set true on first press and set to true every 1s after that
		if(button_state[TIME_PLUS] && !flags[plus_check]){
			flags[plus] = 1;
			flags[plus_check] = 1;
			return;
		}else if(button_state[TIME_PLUS] && flags[plus_check]){
			plus_counter++;
			if(plus_counter >= 500){
				flags[plus] 		= 1;
				flags[plus_check] 	= 0;
				plus_counter = 0;
			}

		// flag is not reset until the button has been un-pressed for 0.2s
		// Note: Other functions can reset this flag independently
		}else{
			plus_counter++;
			if(plus_counter >= 100)
			{
				plus_counter = 0;
				flags[plus] = 0;
				flags[plus_check] = 0;
			}
		}

		// Time- Case
		// Flag is set true on first press and set to true every 1s after that
		if(button_state[TIME_MINUS] && !flags[minus_check]){
			flags[minus] = 1;
			flags[minus_check] = 1;
			return;
		}else if(button_state[TIME_MINUS] && flags[minus_check]){
			minus_counter++;
			if(minus_counter >= 500){
				flags[minus] 		= 1;
				flags[minus_check] 	= 0;
				minus_counter = 0;
			}

		// flag is not reset until the button has been un-pressed for 0.2s
		// Note: Other functions can reset this flag independently
		}else{
			minus_counter++;
			if(minus_counter >= 100)
			{
				minus_counter = 0;
				flags[minus] = 0;
				flags[minus_check] = 0;
			}
		}

		// Time Set Case
		// Flag is set after holding Time Set for 0.5s
		if(button_state[TIME_SET] && !flags[set]){
			set_counter++;
		}else{
			if(set_counter >= 100) flags[set] = 1;
			set_counter = 0;
		}

		// Snooze Case
		// Flag is set true on first press and set to true every 1s after that
		if(button_state[SNOOZE_BUTTON] && !flags[snooze_check]){
			flags[snooze] = 1;
			flags[snooze_check] = 1;
			return;
		}else if(button_state[SNOOZE_BUTTON] && flags[snooze_check]){
			snooze_counter++;
			if(minus_counter >= 500){
				flags[snooze] 		= 1;
				flags[snooze_check] 	= 0;
				snooze_counter = 0;
			}

		// flag is not reset until the button has been un-pressed for 0.2s
		// Note: Other functions can reset this flag independently
		}else{
			snooze_counter++;
			if(snooze_counter >= 100)
			{
				snooze_counter = 0;
				flags[snooze] = 0;
				flags[snooze_check] = 0;
			}
		}
	}
}

//timer interrupt handler that is called at a rate of 500Hz
//this function gets the time and displays it on the 7 segment display
//it also checks for button presses, debounces, and handles each case
void TIM5_IRQHandler(void) // aka why the hell didn't I just use FreeRTOS
{
	static FSM_state_code 	current_state = START_STATE;
	static FSM_return_code 	return_code;
	static FSM_return_code (*state_function)(void);


	//double checks that interrupt has occurred
	if( TIM_GetITStatus( TIM5, TIM_IT_Update ) != RESET )
	{
	     //clears interrupt flag
	     TIM5->SR 		= (uint16_t)~TIM_IT_Update;

	     // Get state to execute
	     state_function	= state[current_state];

	     // Execute the state and save return code
	     return_code	= state_function();

	     // Stop execution if in exit state
	     if(current_state == EXIT_STATE) return;

	     // Transition states
	     current_state = FSM_state_transition[current_state][return_code];
	}
}

//alarm A interrupt handler
//when alarm occurs, clear all the interrupt bits and flags
//then set the flag to play mp3
void RTC_Alarm_IRQHandler(void)
{
	//resets alarm flags and sets flag to play mp3
	if(RTC_GetITStatus(RTC_IT_ALRA) != RESET)
	{
		RTC_ClearFlag(RTC_FLAG_ALRAF);
		RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
		RTC_ITConfig(RTC_IT_ALRA, DISABLE);

		interruptOccurred = 1;
		exitMp3 = 0;

		RTC_ClearITPendingBit(RTC_IT_ALRA);
		EXTI_ClearITPendingBit(EXTI_Line17);
	}
}

//configures the clocks, gpio, alarm, interrupts etc.
void configuration(void)
{
	  //lets the system clocks be viewed
	  RCC_GetClocksFreq(&RCC_Clocks);

	  //enable peripheral clocks
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, 	ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, 	ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, 	ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, 	ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, 	ENABLE);
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, 	ENABLE);
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, 	ENABLE);

	  //enable the RTC
	  PWR_BackupAccessCmd(DISABLE);
	  PWR_BackupAccessCmd(ENABLE);
	  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	  RCC_RTCCLKCmd(ENABLE);
	  RTC_AlarmCmd(RTC_Alarm_A,DISABLE);

	  //Enable the LSI OSC
	  RCC_LSICmd(ENABLE);

	  //Wait till LSI is ready
	  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

	  //enable the external interrupt for the RTC to use the Alarm
	  /* EXTI configuration */
	  EXTI_ClearITPendingBit(EXTI_Line17);
	  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	  EXTI_Init(&EXTI_InitStructure);

	  //set timer 5 to interrupt at a rate of 500Hz
	  TIM_TimeBaseStructInit(&TIM_InitStruct);
	  TIM_InitStruct.TIM_Period	=  8000;	// 500Hz
	  TIM_InitStruct.TIM_Prescaler = 20;
	  TIM_TimeBaseInit(TIM5, &TIM_InitStruct);

	  // Enable the TIM5 global Interrupt
	  NVIC_Init( &NVIC_InitStructure );
	  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init( &NVIC_InitStructure );

	  //set timer 3 to interrupt at a rate of 500Hz
	  TIM_TimeBaseStructInit(&TIM_InitStruct);
	  TIM_InitStruct.TIM_Period	=  8000;	// 500Hz
	  TIM_InitStruct.TIM_Prescaler = 20;
	  TIM_TimeBaseInit(TIM3, &TIM_InitStruct);

      // Enable the TIM3 global Interrupt
      NVIC_Init( &NVIC_InitStructure );
      NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init( &NVIC_InitStructure );

      //setup the RTC for 12 hour format
	  myclockInitTypeStruct.RTC_HourFormat = RTC_HourFormat_12;
	  myclockInitTypeStruct.RTC_AsynchPrediv = 127;
	  myclockInitTypeStruct.RTC_SynchPrediv = 0x00FF;
	  RTC_Init(&myclockInitTypeStruct);

	  //set the time displayed on power up to 12PM
	  RTC_Global_Struct.RTC_H12 = RTC_H12_PM;
	  RTC_Global_Struct.RTC_Hours = 12;
	  RTC_Global_Struct.RTC_Minutes = 0;
	  RTC_Global_Struct.RTC_Seconds = 0;
	  RTC_SetTime(RTC_Format_BIN, &RTC_Global_Struct);

	  //sets alarmA for 12:00AM, date doesn't matter
	  AlarmStruct.RTC_AlarmTime.RTC_H12 = RTC_H12_AM;
	  AlarmStruct.RTC_AlarmTime.RTC_Hours = 12;
	  AlarmStruct.RTC_AlarmTime.RTC_Minutes = 0;
	  AlarmStruct.RTC_AlarmTime.RTC_Seconds = 0;
	  AlarmStruct.RTC_AlarmDateWeekDay = 0x31;
	  AlarmStruct.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
	  AlarmStruct.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;
	  RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &AlarmStruct);

	  // Enable the Alarm global Interrupt
	  NVIC_Init( &NVIC_InitStructure );
	  NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	  NVIC_Init( &NVIC_InitStructure );

	  // TIME_PLUS GPIO Config
	  GPIO_StructInit( &GPIOInitStruct );
	  GPIOInitStruct.GPIO_Pin = TIME_PLUS_PIN;
	  GPIOInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIOInitStruct.GPIO_Mode = GPIO_Mode_IN;
	  GPIOInitStruct.GPIO_OType = GPIO_OType_PP;
	  GPIOInitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	  GPIO_Init(GPIOA, &GPIOInitStruct);

	  // TIME_MINUS, TIME_SET, and SNOOZE GPIO Config
	  GPIO_StructInit( &GPIOInitStruct );
	  GPIOInitStruct.GPIO_Pin = TIME_MINUS_PIN | TIME_SET_PIN | SNOOZE_PIN;
	  GPIOInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIOInitStruct.GPIO_Mode = GPIO_Mode_IN;
	  GPIOInitStruct.GPIO_OType = GPIO_OType_PP;
	  GPIOInitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	  GPIO_Init(GPIOC, &GPIOInitStruct);

	  //enables RTC alarm A interrupt
	  RTC_ITConfig(RTC_IT_ALRA, ENABLE);

	  //enables timer interrupts
	  TIM5->DIER |= TIM_IT_Update;
	  TIM3->DIER |= TIM_IT_Update;

	  //enables timers
	  TIM5->CR1 |= TIM_CR1_CEN;
	  TIM3->CR1 |= TIM_CR1_CEN;

	  // Setup SPI and the update timer
	  Display_SPI_Init();
	  Display_Timer_Init();
}
