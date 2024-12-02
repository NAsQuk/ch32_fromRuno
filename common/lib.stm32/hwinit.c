/*
 * hwinit.c
 *
 *  Created on: Mar 24, 2011
 *      Author: baron
 */
#include "board.h"

//#ifdef ID_GD
#define HSEStartUp_TimeOut      ((uint16_t)0xFFFF)
//#else
//   #define HSEStartUp_TimeOut   ((uint16_t)0x0500) /*!< Time out for HSE start up */
//#endif

ErrorStatus HSEStartUpStatus;
extern void USART3_IRQHandler(void);
/*******************************************************************************
 * Function Name  : RCC_Configuration
 * Description    : Configures the different system clocks.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void RCC_Configuration(void) {
	/* RCC system reset(for debug purpose) */
	RCC_DeInit();

	/* Enable HSE */
	RCC_HSEConfig(RCC_HSE_ON);
	RCC_LSEConfig(RCC_LSE_OFF); /////проверить как работает с gd
	/* Wait till HSE is ready */
	HSEStartUpStatus = RCC_WaitForHSEStartUp();

	if (HSEStartUpStatus == SUCCESS) {
		/* Enable Prefetch Buffer */
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		/* Flash 2 wait state */
		FLASH_SetLatency(FLASH_Latency_2);

		/* HCLK = SYSCLK */

		if (initMK == 1) {
			RCC_HCLKConfig(RCC_SYSCLK_Div1);
		} else {
			RCC_HCLKConfig(RCC_SYSCLK_Div2);
		}

		/* PCLK2 = HCLK */
		RCC_PCLK2Config(RCC_HCLK_Div2);

		/* PCLK1 = HCLK/2 */
		RCC_PCLK1Config(RCC_HCLK_Div2);

		/* PLLCLK = 16MHz * 3 = 48 MHz */
		//RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_3);
		/* PLLCLK = (16MHz * 9) /2= 72 MHz */
		RCC_PLLConfig(RCC_PLLSource_HSE_Div2, RCC_PLLMul_9);
		//RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_2);

		/* Enable PLL */
		RCC_PLLCmd(ENABLE);

		/* Wait till PLL is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {
		}

		/* Select PLL as system clock source */
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		/* Wait till PLL is used as system clock source */
		while (RCC_GetSYSCLKSource() != 0x08) {
		}



	}
}
/*******************************************************************************
 * Function Name  : NVIC_Configuration
 * Description    : Configures Vector Table base location.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
#ifdef VECT_TAB_RAM
/* vector-offset (TBLOFF) from bottom of SRAM. defined in linker script */
extern uint32_t _isr_vectorsram_offs;
void NVIC_Configuration(void)
{
	/* Set the Vector Table base location at 0x20000000+_isr_vectorsram_offs */
	NVIC_SetVectorTable(NVIC_VectTab_RAM, (uint32_t) &_isr_vectorsram_offs);
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure one bit for preemption priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = -3;
    	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;;
    	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    	NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = -3;
    	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    	NVIC_Init(&NVIC_InitStructure);


    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//5//2
    	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
	   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
	   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	   NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel5_IRQn;
	   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
	   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	   NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel3_IRQn;
	   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	   NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	   NVIC_Init(&NVIC_InitStructure);
    //----------------------------------------------------
}
#else
extern uint32_t _isr_vectorsflash_offs;
void NVIC_Configuration(void) {
	/* Set the Vector Table base location at 0x08000000+_isr_vectorsflash_offs */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH,
			(uint32_t) & _isr_vectorsflash_offs);
}
#endif /* VECT_TAB_RAM */

#ifdef USE_FULL_ASSERT

#include "term_io.h"

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param file: pointer to thvoid hwInit()
{

}
 * e source file name
 * @param line: assert_param error line source number
 * @retval : None
 */
void assert_failed(const uint8_t* file, const uint8_t* function, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	xprintf("\n !!! Wrong parameter value detected\n");
	xprintf(" - file %s\n", file);
	xprintf(" - function %s\n", function);
	xprintf(" - line %lu\n", line);

#if 0
	/* Infinite loop */
	while (1)
	{
	}
#endif
}
#endif

void hwInit() {
	GPIO_SetBits(GPIO_LEDWORK, LEDWORK);
	/* System Clocks Configuration */
	RCC_Configuration();
	/* NVIC configuration */
	NVIC_Configuration();
}

