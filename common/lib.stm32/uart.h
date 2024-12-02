/*
 * uart.h
 *
 *  Created on: Mar 24, 2011
 *      Author: baron
 */
#ifndef UART_H
#define UART_H

#include "board.h"
/*
uint8_t RxBuffer[256];
uint8_t RxPointer;
uint8_t RxStatus;
uint8_t RxLength;
*/
#define UARTRX_FIFO_SIZE 1024
#define UARTRX_FIFO_SIZE_MASK (UARTRX_FIFO_SIZE-1)

//#define UARTTX_FIFO_SIZE 1024+3
#define UARTTX_FIFO_SIZE 64
#define UARTTX_FIFO_SIZE_MASK (UARTTX_FIFO_SIZE-1)

#define UARTTX_FIFOB_SIZE       UARTRX_FIFO_SIZE
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


#define  USART3_TX_DMA            DMA1_Channel2
#define  USART3_TX_DMA_IRQn       DMA1_Channel2_IRQn
#define  USART3_TX_DMA_IRQHandler DMA1_Channel2_IRQHandler

#define  USART3_RX_DMA          DMA1_Channel3
#define  USART3_RX_DMA_IRQn     DMA1_Channel3_IRQn

#define  USART2_TX_DMA            DMA1_Channel7
#define  USART2_TX_DMA_IRQn       DMA1_Channel7_IRQn
#define  USART2_TX_DMA_IRQHandler DMA1_Channel7_IRQHandler

#define  USART2_RX_DMA          DMA1_Channel6
#define  USART2_RX_DMA_IRQn     DMA1_Channel6_IRQn

#define  USART1_TX_DMA            DMA1_Channel4
#define  USART1_TX_DMA_IRQn       DMA1_Channel4_IRQn
#define  USART1_TX_DMA_IRQHandler DMA1_Channel4_IRQHandler

#define  USART1_RX_DMA          DMA1_Channel5
#define  USART1_RX_DMA_IRQn     DMA1_Channel5_IRQn



extern void uart1Init(uint32_t brd);
extern void uart2Init(uint32_t brd);
extern void uart3Init(uint32_t brd);
extern void uart3WriteEscaped(unsigned char *src, unsigned int len);
extern void uart1Write(unsigned char *src, unsigned int len);
extern void uart3Write(unsigned char *src, unsigned int len);
extern void TestSend();

extern unsigned int uart3ReadEscaped(unsigned char *dsn, unsigned int max_len);
extern unsigned int uart1Read(unsigned char *dsn, unsigned int max_len);
extern unsigned int uart2Read(unsigned char *dsn, unsigned int max_len);
extern unsigned int uart3Read(unsigned char *dsn, unsigned int max_len);

void DMA1_Channel4_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);
void DMA1_Channel2_IRQHandler(void);
void DMA1_Channel3_IRQHandler(void);

extern void DMARxDataCounterSet(int counter);
extern void DMARxDataCounterInc();
extern int DMARxDataCounterGet();




#endif /*UART_H*/
