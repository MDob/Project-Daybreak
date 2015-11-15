#include "FSM.h"

FSM_return_code (*state[])(void) = {start_state, idle_state, set_minutes_state, set_hours_state, exit_state};

FSM_state_code FSM_state_transition[][NUM_RET_CODES] = {
	  // Transition Table Template:
	  //[state]					= {pass,			fail, 	repeat}
		[start] 				= {idle, 			exit, 	start},
		[idle] 					= {set_minutes, 	exit, 	idle},
		[set_minutes]			= {set_hours,		exit, 	set_minutes},
		[set_hours]				= {idle,			exit,	set_hours},
		[exit]					= {exit,			exit,	exit}
};

// Initialize Status Arrays
bool 	button_state[NUM_BUTTONS] 	= {false}; 	// Button States. 	Order: Time+, Time-, Set, Snooze
bool	flags[NUM_FLAGS] 			= {false};	// Flags. 			Order: See FSM.h

void update_display(void)
{
	// Get the current time only if not setting the time or alarm
	if(!flags[minutes] && !flags[hours]) RTC_GetTime(RTC_Format_BIN, displayed_time);

	/****************************************
	 * 		HOURS DIGITS (ONE AND TWO) 		*
	 * 	Always Lit: Normal Operation		*
	 * 	Blinking:	Time/Alarm Set			*
	 * 	Modifiers:	Alarm Indicator (One)	*
	 ****************************************/

	// Blink the hours digits when being set
	if(flags[hours] && flags[blink]){
		current_display.digit.one		= 0x00;
		current_display.digit.two 		= 0x00;

	// Ensure that the first digit is blank if 0
	}else{
		if((displayed_time->RTC_Hours)/10 != 0){
				current_display.digit.one 	= number_array[(displayed_time->RTC_Hours)/10];
		}else{
				current_display.digit.one 	= 0x00;
		}
		current_display.digit.two 		= number_array[(displayed_time->RTC_Hours)%10];
	}

	/****************************************
	 * 	  MINUTES DIGITS (TWO AND THREE) 	*
	 * 	Always Lit: Normal Operation		*
	 * 	Blinking:	Time/Alarm Set			*
	 * 	Modifiers:	None					*
	 ****************************************/

	// Blink the minutes digits when being set
	if(flags[minutes] && flags[blink]){
		current_display.digit.three = 0x00;
		current_display.digit.four  = 0x00;
	}else{
		current_display.digit.three 	= number_array[displayed_time->RTC_Minutes/10];
		current_display.digit.four 		= number_array[displayed_time->RTC_Minutes%10];
	}

	/****************************************
	 * 	 			  COLON					*
	 * 	Always Lit: Time/Alarm Set			*
	 * 	Blinking:	Normal Operation		*
	 * 	Modifiers:	AM/PM Indicator			*
	 ****************************************/

	// Keep colon lit if setting time/alarm
	if(flags[minutes] || flags[hours]){

		// Turn on alarm indicator light if needed
		current_display.digit.colon = number_array[10];

	// Otherwise, blink colon every second
	}else if(displayed_time->RTC_Seconds % 2 == 0){
		current_display.digit.colon = number_array[10];
	}else{
		current_display.digit.colon = 0x00;
	}

	// Mask with modifiers depending on alarm and AM/PM status
	if(flags[alarm_on] || flags[alarm_flag]) 	current_display.digit.one 	|= alarm_mask;
	if(flags[AM_PM]) 							current_display.digit.colon |= pm_mask;
}

/* 	State Name: 	start_state
 * 	Description:	Performs initialization and then switches to the first
 * 					main state
 */
FSM_return_code start_state(void)
{
	// Potentially perform initialization here, not yet implemented
	return pass;
}

/* 	State Name: 	idle_state
 * 	Description:	This state is active while the clock is not in time/alarm
 * 					setting mode. It waits for inputs to change the state and
 * 					updates the clock.
 */
FSM_return_code idle_state(void)
{
	static uint16_t counter = 0;

	displayed_time = &RTC_Global_Struct;
	flags[AM_PM] = (displayed_time->RTC_H12 == RTC_H12_PM) ? true: false;

	// Update the display with the current time
	update_display();

	// Check for button presses
	// Set 				- Sets the current time
	// Plus && Minus 	- Sets the alarm
	// Snooze			- Toggles the alarm
	if(flags[set]){
		// Set flags and continue to set state
		flags[time_set]  = 1;
		flags[set] 		 = 0;
		return pass;

	}else if(flags[plus] && flags[minus]){
		// Set flags and continue to set state
		flags[alarm_set] = 1;
		flags[plus] 	 = 0;
		flags[minus] 	 = 0;
		return pass;

	}else if(flags[snooze]){
		// Set flags but remain in display state
		flags[alarm_on] 	= !flags[alarm_on];
		flags[alarm_int] 	= 1;
		flags[snooze] 		= 0;
	}

	// Enable/disable Alarm Interrupt based on alarm_on state.
	// alarm_int is set by the snooze button and by the time_set
	// state when in alarm mode.
	if(flags[alarm_int]){
		if(flags[alarm_on]){
			// Enable alarm interrupt
			RTC_ITConfig(RTC_IT_ALRA, ENABLE);
			RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
			RTC_ClearFlag(RTC_FLAG_ALRAF);
			//exitMp3 = 0;
		}else{
			// Disable alarm interrupt
			RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
			RTC_ITConfig(RTC_IT_ALRA, DISABLE);
			RTC_ClearFlag(RTC_FLAG_ALRAF);
			interruptOccurred = 0;
			exitMp3 = 1;
		}

		// Reset flag
		flags[alarm_int] = 0;
	}

	return repeat;
}

/* 	State Name: 	set_minutes_state
 * 	Description:	While the clock is in time/alarm set mode, this state reads
 * 					the button inputs and adjusts the minutes. This state
 * 					repeats until time_set is pressed.
 */
FSM_return_code set_minutes_state(void)
{
	static uint8_t 	*set_mins;
	static uint16_t counter	 = 0;

	// Get time and set flags
	if(flags[time_set]){
		set_mins = &displayed_time->RTC_Minutes;

		flags[minutes] 		= 1;
		flags[time_set] 	= 0;
		return repeat;

	// Get alarm time and set flags
	}else if(flags[alarm_set]){
		RTC_GetAlarm(RTC_Format_BIN, RTC_Alarm_A, &AlarmStruct);
		displayed_time = &AlarmStruct.RTC_AlarmTime;

		set_mins = &displayed_time->RTC_Minutes;

		// Get new AM_PM flag as it may differ from time AM/PM state
		flags[AM_PM] = (displayed_time->RTC_H12 == RTC_H12_PM) ? true: false;

		flags[minutes] 		= 1;
		flags[alarm_flag]	= 1;
		flags[alarm_set]	= 0;
		return repeat;
	}

	// Check button inputs
	if(flags[plus]){

		// Bound checking
		if(*set_mins == 59) *set_mins = 0;
		else (*set_mins)++;
		flags[plus] = 0;

	}else if(flags[minus]){

		// Bound checking
		if(*set_mins == 0) *set_mins = 59;
		else (*set_mins)--;
		flags[minus] = 0;

	}else if(flags[set]){

		flags[set] = 0;
		return pass;
	}

	update_display();
	return repeat;
}

/* 	State Name: 	set_hours_state
 * 	Description:	While the clock is in time/alarm set mode, this state reads
 * 					the button inputs and adjusts the hours. This state
 * 					repeats until time_set is pressed.
 */
FSM_return_code set_hours_state(void)
{
	static uint8_t 	*set_hrs;
	static uint16_t counter = 0;

	// Get updated time and reset flags
	if(flags[alarm_flag] && flags[minutes]){
		set_hrs = &displayed_time->RTC_Hours;

		flags[hours] 	= 1;
		flags[minutes]	= 0;
		return repeat;

	}else if(flags[minutes]){
		set_hrs = &displayed_time->RTC_Hours;

		flags[hours] 	= 1;
		flags[minutes] 	= 0;
		return repeat;
	}

	// Check button inputs
	if(flags[plus]){

		// Bound checking (12/24 Hours)
		if(flags[format_24] && (*set_hrs == 23)){
			*set_hrs = 0;

		}else if(!flags[format_24] && (*set_hrs == 12)){
			*set_hrs = 1;

		}else{
			if(*set_hrs == 11) flags[AM_PM] = !flags[AM_PM];
			(*set_hrs)++;
		}

		flags[plus] = 0;

	}else if(flags[minus]){

		// Bound checking (12/24 Hours)
		if(flags[format_24] && (*set_hrs == 0)){
			*set_hrs = 23;

		}else if(!flags[format_24] && (*set_hrs == 1)){
			*set_hrs = 12;

		}else{
			if(*set_hrs == 12) flags[AM_PM] = !flags[AM_PM];
			(*set_hrs)--;
		}

		flags[minus] = 0;

	}else if(flags[set]){

		// Set AM/PM flag
		displayed_time->RTC_H12 = flags[AM_PM] ? RTC_H12_PM : RTC_H12_AM;

		// Set alarm if alarm_flag is true
		if(flags[alarm_flag]){
			RTC_AlarmCmd(RTC_Alarm_A,DISABLE);
			RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &AlarmStruct);
			flags[alarm_flag] 	= 0;
			flags[alarm_on] 	= 1;
			flags[alarm_int] 	= 1;

		// Set time if alarm_flag is false
		}else{
			RTC_SetTime(RTC_Format_BIN, displayed_time);
		}

		// Reset flags
		flags[hours] = 0;
		flags[set] = 0;
		return pass;
	}

	update_display();
	return repeat;
}

/* 	State Name: 	exit_state
 * 	Description:	Control should never read this state at any point. If it
 * 					does then something has gone wrong and will loop infinitely.
 * 					Fault recovery is not currently implemented.
 */
FSM_return_code exit_state(void)
{
	for(;;)
	{
		// Things have gone horribly wrong (likely out of bounds pointer writing)
	}
	return fail;
}
