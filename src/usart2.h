/*
 * uart.h
 *
 *  Created on: Mar 24, 2011
 *      Author: baron
 */
#ifndef USART2_H
#define USART2_H
#include "board.h"

#define UARTRX_FIFO_SIZE 1024
#define UARTRX_FIFO_SIZE_MASK (UARTRX_FIFO_SIZE-1)
#define UARTTX_FIFO_SIZE 64

#define UARTTX_FIFOB_SIZE UARTRX_FIFO_SIZE
#define UARTTX_FIFOB_SIZE_MASK (UARTTX_FIFOB_SIZE-1)


typedef struct
{
	uint32_t rxCurrent;
	uint32_t txCurrentStart;
	uint32_t txCurrentEnd;
	uint8_t rxBuf[UARTRX_FIFO_SIZE];
	uint8_t txBuf[UARTTX_FIFO_SIZE];
	uint8_t txBufB[UARTTX_FIFOB_SIZE];
} UART_FIFO_STR;

#include "board.h"
#define  USART3_TX_DMA            DMA1_Channel2
#define  USART3_TX_DMA_IRQn       DMA1_Channel2_IRQn
#define  USART3_TX_DMA_IRQHandler DMA1_Channel2_IRQHandler


#define  USART3_RX_DMA          DMA1_Channel3
#define  USART3_RX_DMA_IRQn     DMA1_Channel3_IRQn

typedef struct ReceivedMsgStr
{
	bool flag;
	bool Readflag;
	uint8_t buffer[512];
	uint16_t size;//8
};

extern struct ReceivedMsgStr ReceivedMsg;

extern void uart2Init(uint32_t brd);
extern void uart2WriteEscaped(unsigned char *src, unsigned int len);
extern void uart2Write(unsigned char *src, unsigned int len);
extern void USART5TIMConfigure(uint16_t aTIMtime);


extern unsigned int uart2ReadEscaped(unsigned char *dsn, unsigned int max_len);
extern unsigned int uart2Read(unsigned char *dsn, unsigned int max_len);
extern void USART2_IRQHandler(void);
void DMA1_Channel2_IRQHandler(void);
void DMA1_Channel3_IRQHandler(void);

#endif /*UART_H*/
