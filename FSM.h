#ifndef FSM_H
#define FSM_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rtc.h"
#include "stdbool.h"
#include "display.h"
#include "main.h"
#include "audioMP3.h"
#include "misc.h"

#define EXIT_STATE 		exit
#define START_STATE 	start

#define NUM_BUTTONS 	4
#define NUM_FLAGS		18
#define NUM_RET_CODES 	3

#define TIME_PLUS		0
#define TIME_MINUS		1
#define TIME_SET		2
#define SNOOZE_BUTTON	3

#define BLINK_PERIOD	250

// State Return Codes
typedef enum FSM_return_code_enum	{pass, 		fail, 		repeat}		FSM_return_code;

// State Codes
typedef enum state_code 		{start, idle, set_minutes, set_hours, exit} FSM_state_code;

// Flag codes
typedef enum flag_code_enum	{ 	plus, 		plus_check,	minus, 		minus_check,
								set, 		set_check,	snooze, 	snooze_check,
								alarm_on, 	format_24,	time_set,	alarm_set,
								minutes,	hours,		AM_PM,		blink,
								alarm_flag,	alarm_int
} flag_code;

/* FSM Functions */
FSM_return_code start_state(void);		 // State 1
FSM_return_code idle_state(void);		 // State 2
FSM_return_code set_minutes_state(void); // State 3
FSM_return_code set_hours_state(void);	 // State 4
FSM_return_code exit_state(void);		 // State 5

// Array of state function pointers
FSM_return_code (*state[])(void);

// A transition table containing states and their associated pass/fail/repeat conditions
FSM_state_code 	FSM_state_transition[][NUM_RET_CODES];

// RTC Time Struct Pointer
RTC_TimeTypeDef			*displayed_time;				

// Current state of buttons. 0 = Un-pressed, 1 = Pressed
extern bool				button_state[NUM_BUTTONS];		

// Flags indicating whether the button has been pressed AND released once
extern bool 			flags[NUM_FLAGS];				


#endif /* FSM_H */
