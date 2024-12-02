/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
 * File Name          : i2c_ee.c
 * Author             : MCD Application Team
 * Version            : V2.0.3
 * Date               : 09/22/2008
 * Description        : This file provides a set of functions needed to manage the
 *                      communication between I2C peripheral and I2C M24C08 EEPROM.
 ********************************************************************************
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "i2c_ee.h"
#include "portmacro.h"
#include "board.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define I2C_Speed              100000
#define I2C1_SLAVE_ADDRESS7    0xA2
#define I2C_PageSize           32

#define MIOS32_IIC0_SCL_PORT    GPIOB
#define MIOS32_IIC0_SCL_PIN     GPIO_Pin_8
#define MIOS32_IIC0_SDA_PORT    GPIOB
#define MIOS32_IIC0_SDA_PIN     GPIO_Pin_9

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u16 EEPROM_ADDRESS;

/* Private function prototypes -----------------------------------------------*/
void GPIO_Configuration(void);
void I2C_Configuration(void);

/*******************************************************************************
 * Function Name  : I2C_Configuration
 * Description    : I2C Configuration
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C_Configuration(void) {

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	/* GPIO Periph clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	I2C_InitTypeDef I2C_InitStructure;

	/* I2C configuration */
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = I2C1_SLAVE_ADDRESS7;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;

	/* I2C Peripheral Enable */
	I2C_Cmd(I2C1, ENABLE);
	/* Apply I2C configuration after enabling it */
	I2C_Init(I2C1, &I2C_InitStructure);
	I2C_AcknowledgeConfig(I2C1, ENABLE);
}

/*******************************************************************************
 * Function Name  : I2C_EE_Init
 * Description    : Initializes peripherals used by the I2C EEPROM driver.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C_EE_Init() {
	/* GPIO configuration */
	//GPIO_Configuration();
	/* I2C configuration */
	I2C_Configuration();

	/* depending on the EEPROM Address selected in the i2c_ee.h file */

	/* Select the EEPROM Block0 to write on */
	EEPROM_ADDRESS = 0xA0;
}

/*******************************************************************************
 * Function Name  : I2C_EE_BufferWrite
 * Description    : Writes buffer of data to the I2C EEPROM.
 * Input          : - pBuffer : pointer to the buffer  containing the data to be
 *                    written to the EEPROM.
 *                  - WriteAddr : EEPROM's internal address to write to.
 *                  - NumByteToWrite : number of bytes to write to the EEPROM.
 * Output         : None
 * Return         : None
 *******************************************************************************/
bool I2C_EE_BufferWrite(u8 *pBuffer, u16 WriteAddr, u16 NumByteToWrite) {
	BKP_WriteBackupRegister(BKP_DR7, 21);
	u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;

	Addr = WriteAddr % I2C_PageSize;
	count = I2C_PageSize - Addr;
	NumOfPage = NumByteToWrite / I2C_PageSize;
	NumOfSingle = NumByteToWrite % I2C_PageSize;

	/* If WriteAddr is I2C_PageSize aligned  */
	if (Addr == 0) {
		/* If NumByteToWrite  I2C_PageSize */
		if (NumOfPage == 0) {
			I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
			I2C_EE_WaitEepromStandbyState();
		}
		/* If NumByteToWrite > I2C_PageSize */
		else {
			while (NumOfPage--) {
				I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize);
				I2C_EE_WaitEepromStandbyState();
				WriteAddr += I2C_PageSize;
				pBuffer += I2C_PageSize;
			}

			if (NumOfSingle != 0) {
				I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
				I2C_EE_WaitEepromStandbyState();
			}
		}
	}
	/* If WriteAddr is not I2C_PageSize aligned  */
	else {
		/* If NumByteToWrite  I2C_PageSize */
		if (NumOfPage == 0) {
			I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
			I2C_EE_WaitEepromStandbyState();
		}
		/* If NumByteToWrite > I2C_PageSize */
		else {
			NumByteToWrite -= count;
			NumOfPage = NumByteToWrite / I2C_PageSize;
			NumOfSingle = NumByteToWrite % I2C_PageSize;

			if (count != 0) {
				I2C_EE_PageWrite(pBuffer, WriteAddr, count);
				I2C_EE_WaitEepromStandbyState();
				WriteAddr += count;
				pBuffer += count;
			}

			while (NumOfPage--) {
				I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize);
				I2C_EE_WaitEepromStandbyState();
				WriteAddr += I2C_PageSize;
				pBuffer += I2C_PageSize;
			}
			if (NumOfSingle != 0) {
				I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
				I2C_EE_WaitEepromStandbyState();
			}
		}
	}
	return true;
}

/*******************************************************************************
 * Function Name  : I2C_EE_ByteWrite
 * Description    : Writes one byte to the I2C EEPROM.
 * Input          : - pBuffer : pointer to the buffer  containing the data to be
 *                    written to the EEPROM.
 *                  - WriteAddr : EEPROM's internal address to write to.
 * Output         : None
 * Return         : None
 *******************************************************************************/
bool I2C_EE_ByteWrite(u8 *pBuffer, u16 WriteAddr) {
	if (initMK == 1) {
		int counter = RETRY_NUMBER;
		/* Send STRAT condition */
		I2C_GenerateSTART(I2C1, ENABLE);
		BKP_WriteBackupRegister(BKP_DR7, 22);
		/* Test on EV5 and clear it */
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);

		counter = RETRY_NUMBER;

		while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Test on EV6 and clear it */

		//SR2_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);

		/* Send the EEPROM's internal address to write to */
		I2C_SendData(I2C1, (WriteAddr >> 8) & 0xFF);
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		I2C_SendData(I2C1, WriteAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))

		{
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send the byte to be written */
		I2C_SendData(I2C1, *pBuffer);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;
	} else {
		int counter = RETRY_NUMBER;
		/* Send STRAT condition */
		I2C_GenerateSTART(I2C1, ENABLE);
		BKP_WriteBackupRegister(BKP_DR7, 22);
		/* Test on EV5 and clear it */
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send the EEPROM's internal address to write to */
		I2C_SendData(I2C1, (WriteAddr >> 8) & 0xFF);
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		I2C_SendData(I2C1, WriteAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send the byte to be written */
		I2C_SendData(I2C1, *pBuffer);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;

	}

}

/*******************************************************************************
 * Function Name  : I2C_EE_PageWrite
 * Description    : Writes more than one byte to the EEPROM with a single WRITE
 *                  cycle. The number of byte can't exceed the EEPROM page size.
 * Input          : - pBuffer : pointer to the buffer containing the data to be
 *                    written to the EEPROM.
 *                  - WriteAddr : EEPROM's internal address to write to.
 *                  - NumByteToWrite : number of bytes to write to the EEPROM.
 * Output         : None
 * Return         : None
 *******************************************************************************/
bool I2C_EE_PageWrite(u8 *pBuffer, u16 WriteAddr, u8 NumByteToWrite) {
	if (initMK == 1) {
		int counter = RETRY_NUMBER;
		/* While the bus is busy */
		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);

		/* Read I2C1 SR1 register*/
		// SR2_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);
		//  I2C_ClearFlag(I2C1, I2C_FLAG_ADDR);
		/* Send the EEPROM's internal address to write to */
		I2C_SendData(I2C1, (WriteAddr >> 8) & 0xFF);

		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		I2C_SendData(I2C1, WriteAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;

		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))

		{
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* While there is data to be written */
		while (NumByteToWrite--) {
			/* Send the current byte */
			I2C_SendData(I2C1, *pBuffer);

			/* Point to the next byte to be written */
			pBuffer++;
			counter = RETRY_NUMBER;
			while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE))
				;
			{
				if (counter-- <= 0)
					return I2C_EE_Relaunch();
			}

			/* Test on EV8 and clear it */

		}

		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;
	} else {
		int counter = RETRY_NUMBER;
		/* While the bus is busy */
		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send the EEPROM's internal address to write to */
		I2C_SendData(I2C1, (WriteAddr >> 8) & 0xFF);
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		I2C_SendData(I2C1, WriteAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* While there is data to be written */
		while (NumByteToWrite--) {
			/* Send the current byte */
			I2C_SendData(I2C1, *pBuffer);

			/* Point to the next byte to be written */
			pBuffer++;

			/* Test on EV8 and clear it */
			counter = RETRY_NUMBER;
			while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
				if (counter-- <= 0)
					return I2C_EE_Relaunch();
			}
		}

		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;
	}
}

/*******************************************************************************
 * Function Name  : I2C_EE_BufferRead
 * Description    : Reads a block of data from the EEPROM.
 * Input          : - pBuffer : pointer to the buffer that receives the data read
 *                    from the EEPROM.
 *                  - ReadAddr : EEPROM's internal address to read from.
 *                  - NumByteToRead : number of bytes to read from the EEPROM.
 * Output         : None
 * Return         : None
 *******************************************************************************/
bool I2C_EE_BufferRead(u8 *pBuffer, u16 ReadAddr, u16 NumByteToRead) {
	if (initMK == 1) {
		int counter = RETRY_NUMBER;
		vu16 SR2_Tmp;
		/* While the bus is busy */

		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */

		counter = RETRY_NUMBER;
		while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Read I2C1 SR1 register*/
		SR2_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);

		//

		/* Clear EV6 by setting again the PE bit */
		I2C_Cmd(I2C1, ENABLE);

		/* Send the EEPROM's internal address to write to */
		I2C_SendData(I2C1, (ReadAddr >> 8) & 0xFF);

		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		I2C_SendData(I2C1, ReadAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send STRAT condition a second time */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for read */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Receiver);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* While there is data to be read */
		counter = RETRY_NUMBER;

		while (NumByteToRead) {
			if (NumByteToRead == 1) {
				/* Disable Acknowledgement */
				I2C_AcknowledgeConfig(I2C1, DISABLE);

				/* Send STOP Condition */
				I2C_GenerateSTOP(I2C1, ENABLE);
			}

			/* Test on EV7 and clear it */
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
				/* Read a byte from the EEPROM */
				*pBuffer = I2C_ReceiveData(I2C1);

				/* Point to the next location where the byte read will be saved */
				pBuffer++;

				/* Decrement the read bytes counter */
				NumByteToRead--;

			}

			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Enable Acknowledgement to be ready for another reception */
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		return true;
	} else {
		int counter = RETRY_NUMBER;
		/* While the bus is busy */

		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Clear EV6 by setting again the PE bit */
		I2C_Cmd(I2C1, ENABLE);

		/* Send the EEPROM's internal address to write to */
		I2C_SendData(I2C1, (ReadAddr >> 8) & 0xFF);
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		I2C_SendData(I2C1, ReadAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send STRAT condition a second time */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Send EEPROM address for read */
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Receiver);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* While there is data to be read */
		counter = RETRY_NUMBER;

		while (NumByteToRead) {
			if (NumByteToRead == 1) {
				/* Disable Acknowledgement */
				I2C_AcknowledgeConfig(I2C1, DISABLE);

				/* Send STOP Condition */
				I2C_GenerateSTOP(I2C1, ENABLE);
			}

			/* Test on EV7 and clear it */
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
				/* Read a byte from the EEPROM */
				*pBuffer = I2C_ReceiveData(I2C1);

				/* Point to the next location where the byte read will be saved */
				pBuffer++;

				/* Decrement the read bytes counter */
				NumByteToRead--;

			}

			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Enable Acknowledgement to be ready for another reception */
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		return true;

	}

}

/*******************************************************************************
 * Function Name  : I2C_EE_WaitEepromStandbyState
 * Description    : Wait for EEPROM Standby state
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
bool I2C_EE_WaitEepromStandbyState(void) {

	if (initMK == 1) {
		vu16 SR1_Tmp = 0;
		int counter = RETRY_NUMBER;

		//do
		//{
		/* Send START condition */
		//I2C_GenerateSTART(I2C1, ENABLE);
		/* Read I2C1 SR1 register */
		//SR1_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);
		/* Send EEPROM address for write */
		// I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);
		// if(counter-- <= 0) return I2C_EE_Relaunch();
		//}while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0x0002));
		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		/* Send START condition*/
		I2C_GenerateSTART(I2C1, ENABLE);
		/* Read I2C1 SR1 register*/
		SR1_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);

		I2C_ClearFlag(I2C1, I2C_FLAG_ADDR);

		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		/* Send EEPROM address for write*/
		I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS, I2C_Direction_Transmitter);
		/*{
		 int i = 0xfff;
		 while(i --);
		 }*/
		counter = RETRY_NUMBER;

		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Clear AF flag */
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);

		/* STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;
	} else {
		vu16 SR1_Tmp = 0;
		int counter = RETRY_NUMBER;
		do {
			/* Send START condition */
			I2C_GenerateSTART(I2C1, ENABLE);
			/* Read I2C1 SR1 register */
			SR1_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);
			/* Send EEPROM address for write */
			I2C_Send7bitAddress(I2C1, EEPROM_ADDRESS,
					I2C_Direction_Transmitter);
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		} while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0x0002));

		/* Clear AF flag */
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);

		/* STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;
	}

}

bool I2C_EE_Relaunch(void) {
	if (initMK == 1) {
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);

		if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {

			I2C_GenerateSTOP(I2C1, ENABLE);
			//vTaskDelay(100);
		}
	} else {

		I2C_Cmd(I2C1, DISABLE);
		GPIO_InitTypeDef GPIO_InitStructure;

		// reconfigure IIC pins to push pull
		GPIO_StructInit(&GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

		GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SCL_PIN;
		GPIO_Init(MIOS32_IIC0_SCL_PORT, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SDA_PIN;
		GPIO_Init(MIOS32_IIC0_SDA_PORT, &GPIO_InitStructure);

		u32 i;
		//vTaskDelay(20);
		for (i = 0; i < 16; i++) {
			/*Reset the SDA Pin*/
			GPIO_ResetBits(GPIOB, MIOS32_IIC0_SDA_PIN);
			// vTaskDelay(20);
			/*Reset the SCL Pin*/
			GPIO_ResetBits(GPIOB, MIOS32_IIC0_SCL_PIN);
			// vTaskDelay(20);
			/*Set the SCL Pin*/
			GPIO_SetBits(GPIOB, MIOS32_IIC0_SCL_PIN);
			// vTaskDelay(20);
			/*Set the SDA Pin*/
			GPIO_SetBits(GPIOB, MIOS32_IIC0_SDA_PIN);
			// vTaskDelay(20);
		}

		GPIO_StructInit(&GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;

		GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SCL_PIN;
		GPIO_Init(MIOS32_IIC0_SCL_PORT, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SDA_PIN;
		GPIO_Init(MIOS32_IIC0_SDA_PORT, &GPIO_InitStructure);

		I2C_Cmd(I2C1, ENABLE);

		I2C_AcknowledgeConfig(I2C1, ENABLE);
		vTaskDelay(100);

		return false;
	}

}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
