#include "display.h"

spi_interrupt_flag verify_spi = SPI_DATA_SENT;
Display_Data current_display;

// Numbers 0 to 9, colon, alarm dot
const uint16_t number_array[12] = {
		0b0000011111100000, 0b0000001100000000, 0b0000011011010000,
		0b0000011110010000, 0b0000001100110000, 0b0000010110110000,
		0b0000010111110000, 0b0000011100000000, 0b0000011111110000,
		0b0000011110110000, 0b0000011000000000, 0b0000011100000000
};

// Digits 1 - 4, dots
const uint16_t digit_array[5] = {
		0b1000000000000000, 0b0100000000000000, 0b0010000000000000,
		0b0001000000000000, 0b0000100000000000
};

const uint16_t alarm_mask 	= 0b0000000000001000;
const uint16_t pm_mask 		= 0b0000000100000000;

void Display_SPI_Init(void)
{
	// Create temporary initialization structs
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;

	// Enable peripheral clock for SPI2
	RCC_APB1PeriphClockCmd(SPI_PERIPH_CLK, ENABLE);

	// Setup SPI structure
	SPI_InitStruct.SPI_BaudRatePrescaler 	= SPI_BaudRatePrescaler_8;
	SPI_InitStruct.SPI_Direction 			= SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_Mode 				= SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize 			= SPI_DataSize_16b;
	SPI_InitStruct.SPI_NSS 					= SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
	SPI_InitStruct.SPI_FirstBit 			= SPI_FirstBit_LSB;
	SPI_InitStruct.SPI_CPOL 				= SPI_CPOL_High;
	SPI_InitStruct.SPI_CPHA 				= SPI_CPHA_2Edge;

	SPI_Init(SPI_PORT, &SPI_InitStruct);

	 // SCK = PB13
	 // MISO = PB14
	 // MOSI = PB15
	 // CS = PB12

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	// Setup SCK and MOSI
	GPIO_InitStruct.GPIO_Pin 	= SPI_MOSI | SPI_CLK;
	GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType 	= GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_DOWN;
	GPIO_Init(SPI_GPIO, &GPIO_InitStruct);

	// Setup CS
	GPIO_InitStruct.GPIO_Pin 	= SPI_CS;
	GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType 	= GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_UP;
	GPIO_Init(SPI_GPIO, &GPIO_InitStruct);

	// Configure alternate functions
	GPIO_PinAFConfig(SPI_GPIO, SPI_CLK_PS, SPI_AF);
	GPIO_PinAFConfig(SPI_GPIO, SPI_MOSI_PS, SPI_AF);

	// Initialize CS high
	GPIO_SetBits(SPI_GPIO, SPI_CS);

	// Enable SPI
	SPI_Cmd(SPI_PORT, ENABLE);
}

// Timer counter interrupt handler for TIM4
void TIM4_IRQHandler(void)
{
	// Setup static counter variable
	static uint8_t counter = 0;

	// Check that interrupt has occurred
	if(TIM_GetITStatus(DISP_TIM, TIM_IT_Update) != RESET)
	{
		// Clear pending timer counter interrupts
		TIM_ClearITPendingBit(DISP_TIM, TIM_IT_Update);


		switch(verify_spi)
		{
			case SPI_DATA_SENT:
				// Check if SPI is busy
				if(!SPI_I2S_GetFlagStatus(SPI_PORT, SPI_I2S_FLAG_BSY))
				{
					// Set latch high
					GPIO_SetBits(SPI_GPIO, SPI_CS);
					verify_spi = SPI_LATCH_HIGH;
				}
			case SPI_LATCH_HIGH:
				// Set latch low
				GPIO_ResetBits(SPI_GPIO, SPI_CS);

				// Reset counter if out of bounds
				if(counter >= 5) counter = 0;

				// Check if SPI Tx buffer is empty
				if(SPI_I2S_GetFlagStatus(SPI_PORT, SPI_I2S_FLAG_TXE))
				{
					// Send data over SPI
					SPI_I2S_SendData(SPI_PORT, current_display.data[counter] | digit_array[counter]);
					counter++;
					verify_spi = SPI_DATA_SENT;
				}
			break;

			default:
			break;
		}
	}
}

void Display_Timer_Init(void)
{
	// Setup timer interrupt
	DISP_NVIC_Struct.NVIC_IRQChannel = DISP_NVIC_IRQn;
	DISP_NVIC_Struct.NVIC_IRQChannelPreemptionPriority = DISP_NVIC_PREMP;
	DISP_NVIC_Struct.NVIC_IRQChannelSubPriority = DISP_NVIC_SUB;
	DISP_NVIC_Struct.NVIC_IRQChannelCmd = DISP_NVIC_CMD;
	NVIC_Init(&DISP_NVIC_Struct);

	// Configure timer for 500Hz
	RCC_APB1PeriphClockCmd(DISP_TIM_PERIPH_CLK, ENABLE);
	DISP_TIM_Struct.TIM_Period = DISP_TIM_PERIOD;
	DISP_TIM_Struct.TIM_Prescaler = DISP_TIM_PRESCALER;
	DISP_TIM_Struct.TIM_ClockDivision = DISP_TIM_CLKDIV;
	DISP_TIM_Struct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(DISP_TIM, &DISP_TIM_Struct);

	// Enable timer
	TIM_ITConfig(DISP_TIM, TIM_IT_Update, ENABLE);
	TIM_Cmd(DISP_TIM, ENABLE);
}
