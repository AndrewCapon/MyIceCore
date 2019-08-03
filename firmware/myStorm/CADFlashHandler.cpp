/*
 * CADFlashHandler.cpp
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#include "CADFlashHandler.h"
#include "mystorm.h"

CADFlashHandler::CADFlashHandler(SPI_HandleTypeDef *hspi)
{
	spi = hspi;
}

#define MYTIMEOUT 1000

uint8_t CADFlashHandler::write(uint8_t *p, uint32_t len)
{
	return HAL_SPI_Transmit(spi, p, len, MYTIMEOUT);
}

uint8_t CADFlashHandler::read(uint8_t *p, uint32_t len)
{
	return HAL_SPI_Receive(spi, p, len, MYTIMEOUT);
}

uint8_t CADFlashHandler::write_read(uint8_t *tx, uint8_t *rx, uint32_t len)
{
	return HAL_SPI_TransmitReceive(spi, tx, rx, len, MYTIMEOUT);
}

void CADFlashHandler::Enable(void)
{
	uint8_t uCommand = 0xAB;
	gpio_low (ICE40_SPI_CS);
	write(&uCommand, 1);
	gpio_high(ICE40_SPI_CS);
}


bool CADFlashHandler::CheckId(void)
{
	uint8_t uCommand = 0x9F;
	uint8_t response[3] = { 0, 0, 0 };

	gpio_low (ICE40_SPI_CS);
	write(&uCommand, 1);
	read(response, 3);
	gpio_high(ICE40_SPI_CS);

	return ((response[0] == 0x1F) && (response[1] == 0x84) && (response[2] == 0x01));
}

bool CADFlashHandler::init(uint8_t uSubCommand)
{
	bool bResult = false;
	InitBufferedWrite();
	Enable();

	if (CheckId())
	{
		switch(uSubCommand)
		{
			case 0: // erase flash
				EraseFlash();
				bResult = false;
			break;

			case 1: // Program flash with bitsream
				Erase64K(0);
				Erase64K(1);
				Erase64K(2);
				bResult = true;
			break;

			case 2: // Query flash
					cdc_puts((char *)"AT25SF041 Flash Found\n");
			break;

			default:
				bResult = false;
		}
	}
	else
		cdc_puts((char *)"Error: AT25SF041 Flash NotFound\n");


	return bResult;
}

void CADFlashHandler::InitBufferedWrite(void)
{
	m_uBytesHandled = 0;
	m_uCur256BytePage = 0;
	m_uBufferPos = 0;
}

void CADFlashHandler::BufferedWrite(uint8_t *pData, uint32_t uLen)
{
	m_uBytesHandled += uLen;

	while (uLen)
	{
		uint16_t uRemaining = 256 - m_uBufferPos;
		if (uLen < uRemaining)
		{
			memcpy(m_buffer + m_uBufferPos, pData, uLen);
			m_uBufferPos += uLen;
			uLen = 0;
		}
		else
		{
			memcpy(m_buffer + m_uBufferPos, pData, uRemaining);
			m_uBufferPos += uRemaining;
			pData += uRemaining;
			uLen -= uRemaining;
		}

		if (m_uBufferPos == 256)
		{
			// ok write to flash
			WritePage(m_uCur256BytePage, m_buffer);

			m_uCur256BytePage++;
			m_uBufferPos = 0;
		}

	}
}

void CADFlashHandler::FlushBuffer(void)
{
	if (m_uBufferPos)
		WritePage(m_uCur256BytePage, m_buffer);
}

bool CADFlashHandler::streamData(uint8_t *data, uint32_t len)
{
	bool bResult = true;

	if (!m_uBytesHandled) // bodge lookat doing properly
	{
		// skip 4 byte header
		data += 4;
		len -= 4;
	}

	BufferedWrite(data, len);

	if (m_uBytesHandled >= IMGSIZE)
	{
//		if(err = config())
//			status_led_high();
//		else
//			status_led_low();

		FlushBuffer();
		bResult = false;
	}

	return bResult;

}

bool CADFlashHandler::IsReady(void)
{
	uint8_t uCommand = 0x05;
	uint8_t response;

	gpio_low (ICE40_SPI_CS);
	write(&uCommand, 1);
	read(&response, 1);
	gpio_high(ICE40_SPI_CS);

	return (!(response & 1));
}

void CADFlashHandler::PollForReady(void)
{
	while (!IsReady())
		;
}

bool CADFlashHandler::WritePage(uint16_t uPage, uint8_t *pData)
{
	WriteEnable();

	uint8_t uMSB = (uPage >> 8) & 0xFF;
	uint8_t uLSB = uPage & 0xFF;

	gpio_low (ICE40_SPI_CS);
	uint8_t command[4] = { 0x02, uMSB, uLSB, 0x00 };
	write(command, 4);

	write(pData, 256);
	gpio_high(ICE40_SPI_CS);

	PollForReady();

	return(true);
}

bool CADFlashHandler::ReadPage(uint16_t uPage, uint8_t *pData)
{
	uint8_t uMSB = (uPage >> 8) & 0xFF;
	uint8_t uLSB = uPage & 0xFF;

	gpio_low (ICE40_SPI_CS);
	uint8_t command[4] = { 0x03, uMSB, uLSB, 0x00 };
	write(command, 4);

	read(pData, 256);
	gpio_high(ICE40_SPI_CS);

	return(true);
}

bool CADFlashHandler::WriteData(uint32_t uAddr, uint8_t *pData, uint32_t uLen)
{
	WriteEnable();

	// just address 0 at the moment
	gpio_low (ICE40_SPI_CS);
	uint8_t command[4] = { 0x02, 0x00, 0x00, 0x00 };
	write(command, 4);

	write(pData, uLen);
	gpio_high(ICE40_SPI_CS);


	PollForReady();
	return(true);
}

bool CADFlashHandler::ReadData(uint32_t uAddr, uint8_t *pData, uint32_t uLen)
{
	PollForReady();

	// just address 0 at the moment
	gpio_low (ICE40_SPI_CS);
	uint8_t command[4] = { 0x03, 0x00, 0x00, 0x00 };
	write(command, 4);

	read(pData, uLen);
	gpio_high(ICE40_SPI_CS);

	return(true);
}

void CADFlashHandler::WriteEnable(void)
{
	uint8_t uCommand = 0x06;

	gpio_low (ICE40_SPI_CS);
	write(&uCommand, 1);
	gpio_high(ICE40_SPI_CS);

}

void CADFlashHandler::EraseFlash(void)
{
	PollForReady();

	WriteEnable();

	uint8_t uCommand = 0x60;

	gpio_low (ICE40_SPI_CS);
	write(&uCommand, 1);
	gpio_high(ICE40_SPI_CS);


	PollForReady();

}

void CADFlashHandler::Erase64K(uint8_t uPage)
{
	PollForReady();

	WriteEnable();

	uint8_t command[] = { 0xD8, uPage, 0, 0 };

	gpio_low (ICE40_SPI_CS);
	write(command, 4);
	gpio_high(ICE40_SPI_CS);

	PollForReady();

}

