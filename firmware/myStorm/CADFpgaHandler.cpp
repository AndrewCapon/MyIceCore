/*
 * CADFpgaHandler.cpp
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#include "CADFpgaHandler.h"
#include "mystorm.h"

CADFpgaHandler::CADFpgaHandler(uint32_t img_size)
{
	m_uImageSize = img_size;
}

uint8_t CADFpgaHandler::reset(uint8_t bit_src)
{
	int timeout = 100;

	gpio_low (ICE40_CRST);

	// Determine FPGA image source, config Flash
	switch (bit_src)
	{
	case MCNTRL: // STM32 master prog, disable flash
		gpio_low (ICE40_SPI_CS);
		protect_flash();
		hold_flash();
		break;
	case FLASH0: // CBSDEL=00 Flash
		gpio_high(ICE40_SPI_CS);
		hold_flash();
		protect_flash();
		break;
	case FLASH1: // CBSDEL=01 Flash
		gpio_high(ICE40_SPI_CS);
		release_flash();
		protect_flash();
		break;
	}

	while (timeout--)
		if (gpio_ishigh(ICE40_CRST))
			return TIMEOUT;

	gpio_high(ICE40_CRST);

	// Neede for STM src
	timeout = 100;
	while (gpio_ishigh (ICE40_CDONE))
	{
		if (--timeout == 0)
			return TIMEOUT;
	}
	// certainly need this delay for STM as src, not sure about flash boot
	timeout = 12800;
	while (timeout--)
		if (gpio_ishigh (ICE40_SPI_CS))
			return TIMEOUT;

	free_flash();
	release_flash();
	return OK;
}

uint8_t CADFpgaHandler::config(void)
{
	uint8_t b = 0;

	for (int timeout = 100; !gpio_ishigh(ICE40_CDONE); timeout--)
	{
		if (timeout == 0)
		{
			//cdc_puts("CDONE not set\n");
			return ICE_ERROR;
		}
		write(&b, 1);
	}

	for (int i = 0; i < 7; i++)
		write(&b, 1);

	gpio_high (ICE40_SPI_CS);

	return OK;
}

uint8_t CADFpgaHandler::write(uint8_t *p, uint32_t len)
{
	uint32_t i;
	uint8_t ret, b, d;

	ret = HAL_OK;
	for (i = 0; i < len; i++)
	{
		d = *p++;
		for (b = 0; b < 8; b++)
		{
			if (d & 0x80)
			{
				HAL_GPIO_WritePin(GPIOB, SPI3_SCK_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, SPI3_MISO_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOB, SPI3_SCK_Pin, GPIO_PIN_SET);
			}
			else
			{
				HAL_GPIO_WritePin(GPIOB, SPI3_MISO_Pin | SPI3_SCK_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, SPI3_SCK_Pin, GPIO_PIN_SET);
			}
			d <<= 1;
		}
		gpio_high (SPI3_SCK);
	}
	return ret;
}


bool CADFpgaHandler::init(uint8_t uSubCommand)
{
	bool bResult = false;
	uint8_t err;

	m_uBytesHandled = 0;
	status_led_high();
	flash_SPI_Disable();
	if (err = reset(MCNTRL))
		flash_SPI_Enable();
	else
	{
		m_uBytesHandled += 4;
		write((uint8_t *) &sig, 4);
		bResult = true;
	}

	return bResult;
}

bool CADFpgaHandler::streamData(uint8_t *data, uint32_t len)
{
	bool bResult = true;
	uint8_t err;

	m_uBytesHandled += len;
	write(data, len);

	if (m_uBytesHandled >= m_uImageSize)
	{
		if (err = config())
			status_led_high();
		else
			status_led_low();

		flash_SPI_Enable();
		bResult = false;
	}

	return bResult;
}

