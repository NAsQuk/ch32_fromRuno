/*
 *  uart.c
 *  Created on: Mar 24, 2011
 *  Author: baron
 */


#include "usart2.h"

UART_FIFO_STR u2Fifo;
struct ReceivedMsgStr ReceivedMsg;
void USART2TIMConfigure(uint16_t aTIMtime)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 100000;
	TIM_TimeBaseStructure.TIM_Prescaler = 1000;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	TIM_ARRPreloadConfig(TIM5, ENABLE);

	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;

	//TIM5->ARR = aTIMtime;
	//TIM_ARRPreloadConfig(TIM5,ENABLE);
//	TIM_ICInit(TIM5, &TIM_ICInitStructure);

	//TIM_SelectInputTrigger(TIM5, TIM_TS_TI2FP2);
//	TIM_SelectSlaveMode(TIM5, TIM_SlaveMode_Reset);
//	TIM_SelectMasterSlaveMode(TIM5, TIM_MasterSlaveMode_Enable);
    TIM_ClearFlag(TIM5,TIM_FLAG_Update);//
//	TIM_Cmd(TIM5, ENABLE);

	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);

//	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
    TIM_Cmd(TIM5, ENABLE);//


/*
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 10000;
	TIM_TimeBaseStructure.TIM_Prescaler = 1000;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	TIM_ARRPreloadConfig(TIM5, ENABLE);


	TIM_Cmd(TIM5, ENABLE);
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE);

	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
*/
}

void TIM5_IRQHandler(void)
{
	//ReceivedMsgStr ReceivedMsg;
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);

    int asize = uart2Read(ReceivedMsg.buffer,260);
	if(asize>0)
    {
	if(ReceivedMsg.size == 0)
        {
		ReceivedMsg.flag = true;
		ReceivedMsg.Readflag = false;
		ReceivedMsg.size = asize;
		}
		//TIM_Cmd(TIM5, DISABLE);//
	}
}


void uart2Init(uint32_t boudrate)
{
	GPIOforUSART3cfg();
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	USART_DeInit(USART3);

	USART_InitTypeDef USART_InitStructure;

	/* Enable AFIO,  clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	/* Enable GPIO clocks */
	RCC_APB2PeriphClockCmd(/*RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOA|*/RCC_APB2Periph_GPIOB, ENABLE);


	/* Enable USART3,  clocks */
     RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	//GPIO_PinRemapConfig(GPIO_PartialRemap_USART3,ENABLE);



	USART_InitStructure.USART_BaudRate = boudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode =  USART_Mode_Tx | USART_Mode_Rx;
	/* Configure USART3 */
	USART_Init(USART3, &USART_InitStructure);

	USART_ClearFlag(USART3, USART_FLAG_CTS | USART_FLAG_LBD  |
						USART_FLAG_TC  | USART_FLAG_RXNE );

	uartRxDMAConfiguration(USART3, DMA1_Channel3, u2Fifo.rxBuf,UARTRX_FIFO_SIZE);
	uartTxDMAConfiguration(USART3, DMA1_Channel2, u2Fifo.txBuf, UARTTX_FIFO_SIZE);

		//USART3TIMConfigure(50);

	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	/* Enable USART_Rx DMA Receive request */
    USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);
	//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	  /* Enable USART_Rx Receive interrupt */
	  //USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	    /* Configure USART3 interrupt */
   // NVIC_SetPriority(DMA1_Channel3_IRQn, 0x0);
  //	NVIC_EnableIRQ(DMA1_Channel3_IRQn);

	USART_Cmd(USART3, ENABLE);

}
//-----------------------------------------------------------------------------------
unsigned int uart2FindEscapedPack()
{
	unsigned int i = u2Fifo.rxCurrent;

	while (i != (UARTRX_FIFO_SIZE - DMA_GetCurrDataCounter(DMA1_Channel3)))
	{
		if (u2Fifo.rxBuf[i] == 0x55)
		{
			i++;
			i &= UARTRX_FIFO_SIZE_MASK;
			if (i == (DMA1_Channel3->CMAR - (uint32_t) &u2Fifo.rxBuf[0]))
				return 0;//return (isStart & isEnd);

			if (u2Fifo.rxBuf[i] == 0x03)
			{
				return 1;
			}
		}
		i++;
		i &= UARTRX_FIFO_SIZE_MASK;
	}
	return 0;

}
//-----------------------------------------------------------------------------------

int uart2GetChar(unsigned char *ch)
{
	if (u2Fifo.rxCurrent != (UARTRX_FIFO_SIZE - DMA_GetCurrDataCounter(DMA1_Channel3)))
	{
		*ch = u2Fifo.rxBuf[u2Fifo.rxCurrent];
		u2Fifo.rxCurrent++;
		u2Fifo.rxCurrent &= UARTRX_FIFO_SIZE_MASK;
		return 1;
	}
	return 0;
}
//-----------------------------------------------------------------------------------
int uart2PutChar(unsigned char ch)
{

	//	IEC2bits.DMA4IE = 0;
	u2Fifo.txBufB[u2Fifo.txCurrentEnd] = ch;
	u2Fifo.txCurrentEnd++;
	u2Fifo.txCurrentEnd &= UARTTX_FIFOB_SIZE_MASK;

	if (u2Fifo.txCurrentEnd == u2Fifo.txCurrentStart)
	{
		u2Fifo.txCurrentEnd--;
		u2Fifo.txCurrentEnd &= UARTTX_FIFOB_SIZE_MASK;
		//		IEC2bits.DMA4IE = 1;
		return 0;
	}
	//	IEC2bits.DMA4IE = 1;
	return 1;
}
//-----------------------------------------------------------------------------------
void GPIOforUSART3cfg(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
//                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // Tx on PC10 as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Rx on PC11 as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed =GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
        GPIO_InitStructure.GPIO_Speed = 0;
           GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
           GPIO_Init(GPIOB, &GPIO_InitStructure);

}

//-----------------------------------------------------------------------------------
void uart2TxTask()
{
	unsigned int cnt = 0;
	// If DMA COMPLITE
	if ((DMA1_Channel2->CCR & ((uint32_t) 0x00000001)) == 1)
	{
		while ((u2Fifo.txCurrentStart != u2Fifo.txCurrentEnd) && (cnt< UARTTX_FIFO_SIZE))
		{
			u2Fifo.txBuf[cnt] = u2Fifo.txBufB[u2Fifo.txCurrentStart];
			u2Fifo.txCurrentStart++;
			u2Fifo.txCurrentStart &= UARTTX_FIFOB_SIZE_MASK;
			cnt++;
		}
		if (cnt > 0)
		{
			uartTxDMAConfiguration(USART3, DMA1_Channel2, u2Fifo.txBuf, cnt,1);
			DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);
				//}
				/* Enable USART_Tx DMA Tansmit request */
				/* Configure DMA1_Channel_Tx interrupt */
				NVIC_SetPriority(DMA1_Channel2_IRQn, 0x02);
				NVIC_EnableIRQ(DMA1_Channel2_IRQn);
		}
	}
}
//-----------------------------------------------------------------------------------
unsigned int uart2ReadEscaped(unsigned char *dsn, unsigned int max_len)
{

	unsigned short cnt = 0, crc = 0;
	unsigned char v, *ptr = dsn;
	if (uart2FindEscapedPack() == 0)
	{
		return 0;
	}
	while (uart2GetChar(&v) && (max_len > cnt))
	{
		if (v == 0x55)
		{
			if (!uart2GetChar(&v))
				return 0;
			if (v == 0x1) // start pack here;
			{
				ptr = dsn;
				cnt = 0;
				crc = 0;
				continue;
			}
			else if (v == 0x2)
			{
				v = 0x55;
			}
			else if (v == 0x3)
			{
				ptr--;
				if (((crc - *ptr) & 0x00FF) != *ptr)
					return 0; // crc fail
				return cnt;
			}
		}
		*ptr = v;
		ptr++;
		crc += v;
		cnt++;
	}
	return 0; //  error here
}
//-----------------------------------------------------------------------------------
unsigned int uart2Read(unsigned char *dsn, unsigned int max_len)
{
	unsigned short cnt = 0, crc = 0;
	unsigned char v, *ptr = dsn;
	/*
	u2Fifo.rxBuf[0] = '$';
	for (int i = 0; i < 32; i++)
	{
		v = u2Fifo.rxBuf[i];
		*ptr = v;
		ptr++;
	}
return 32;
*/
	while (uart2GetChar(&v) && (max_len > cnt))
	{
		*ptr = v;
		ptr++;
		crc += v;
		cnt++;
	}
	return cnt; //  error here
}

//-----------------------------------------------------------------------------------
void uart2WriteEscaped(unsigned char *src, unsigned int len)
{
	unsigned char ch;
	unsigned char crc = 0;//getCRC(src, len);
	unsigned int rezult;
	if (len == 0)
		return;
	uart2PutChar(0x55);
	uart2PutChar(0x01);
	while (len--)
	{
		ch = *src;
		rezult = uart2PutChar(ch);
		if (ch == 0x55)
		{
			rezult = uart2PutChar(0x02);
		}
		src++;
	}
	ch = crc;
	rezult = uart2PutChar(ch);
	if (ch == 0x55)
	{
		rezult = uart2PutChar(0x02);
	}

	rezult = uart2PutChar(0x55);
	rezult = uart2PutChar(0x03);
	uart2TxTask();
}
void uart2Write(unsigned char *src, unsigned int len)
{
	unsigned char ch;
	unsigned int rezult;
	if (len == 0)
	return;
	while (len--)
	{
		ch = *src;
		rezult = uart2PutChar(ch);
		src++;
	}
	uart2TxTask();
}


// UART 1 DMA INTERRUPT
void DMA1_Channel2_IRQHandler(void)
{
    /* Disable DMA1_Channel2 transfer*/
	DMA_Cmd(DMA1_Channel2, ENABLE);
	/*  Clear DMA1_Channel2 Transfer Complete Flag*/
	DMA_ClearFlag(DMA1_FLAG_TC2);
	uart2TxTask();
}

void DMA1_Channel3_IRQHandler(void)
{
    /* Disable DMA1_Channel2 transfer*/
	DMA_Cmd(DMA1_Channel2, DISABLE);
	/*  Clear DMA1_Channel2 Transfer Complete Flag*/
	DMA_ClearFlag(DMA1_FLAG_TC3);//

	u2Fifo.rxCurrent+=0;
	int tempTest = DMA_GetCurrDataCounter(DMA1_Channel3);
	tempTest +=0;
//   DMARxDataReceived();
   uart2TxTask();
}
//-------------------------------------------
void uartTxDMAConfiguration(USART_TypeDef *uart, DMA_Channel_TypeDef *dmatx,
		uint8_t *txBuf, uint32_t len, bool ie)
{
	DMA_InitTypeDef DMA_InitStructure;
	/* DMA1 Channel (triggered by USART_Tx event) Config */
	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &uart->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) txBuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = len;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel2, ENABLE);

	NVIC_SetPriority(DMA1_Channel2_IRQn, 0x01);
	NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
 * @brief  Configures the DMA.
 * @param  uart, dmatx,dmarx,interrupt enable
 * @retval : None
 */



void uartRxDMAConfiguration(USART_TypeDef *uart, DMA_Channel_TypeDef *dmarx,
		unsigned char *rxBuf, uint32_t len)
{
	DMA_InitTypeDef DMA_InitStructure;


	DMA_DeInit(DMA1_Channel3);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &uart->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) rxBuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = len;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel3, ENABLE);//dmarx
    //DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);

    NVIC_SetPriority(DMA1_Channel3_IRQn, 0x02);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

