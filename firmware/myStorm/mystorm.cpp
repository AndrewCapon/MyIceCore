/*
 * Configure the ICE40 with new bitstreams:
 *	- repeatedly from usbcdc
*/
/*
Copyright (c) 2019, Alan Wood All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "main.h"
#include "usbd_cdc_if.h"
#include "stm32f7xx_hal.h"
#include "errno.h"
#include "mystorm.h"

#define CADFirmware


#ifdef CADFirmware
#include "CADCommandHandler.h"
#include "CADCommandStream.h"
#include "CADUtilsHandler.h"
#include "CADFlashHandler.h"
#include "CADFpgaHandler.h"
#endif

#ifdef __cplusplus
 extern "C" {
#endif

extern SPI_HandleTypeDef hspi3;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;

// #define DMA_BYTES 2048
// #define DMAH (DMA_BYTES / 2)
// uint8_t rxdmabuf[DMA_BYTES];
// uint8_t urxdmabuf[64];
// uint8_t urxlen = 0;
// uint8_t dmai = 0;
// int dmao = DMAH - 1;

static int mode = 0;
static int err = 0;
uint8_t errors = 0;

uint32_t rxi,rxo;
uint8_t  rxb[RX];

/* functions */
void cdc_puts(char *s);
void flash_SPI_Enable(void);
void flash_SPI_Disable(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
uint8_t flash_id(char *buf, int len);
uint8_t error_report(char *buf, int len);

/* Interrupts */
static int8_t usbcdc_rxcallback(uint8_t *data, uint32_t *len);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

/* Classses */
class Fpga {
	uint32_t NBYTES;
	uint8_t state;
	uint32_t nbytes;
	union SIG sig =  {0x7E, 0xAA, 0x99, 0x7E} ;

	public:
		Fpga(uint32_t img_size);
		uint8_t reset(uint8_t bit_src);
		uint8_t config(void);
		uint8_t write(uint8_t *p, uint32_t len);
		uint8_t stream(uint8_t *data, uint32_t len);

};

class Flash {
	SPI_HandleTypeDef *spi;

	public:
	 Flash(SPI_HandleTypeDef *hspi);
	 uint8_t write(uint8_t *p, uint32_t len);
	 uint8_t read(uint8_t *p, uint32_t len);
	 uint8_t write_read(uint8_t *tx, uint8_t *rx, uint32_t len);
};


/* global objects */

#ifdef CADFirmware
extern "C" void __cxa_pure_virtual() { while (1); }

CADFpgaHandler  					g_fpgaHandler(IMGSIZE);
CADFlashHandler 					g_flashHandler(&hspi3);
CADUtilsHandler						g_utilsHandler;
CADCommandStream 					g_commandStream;
#else
Fpga Ice40(IMGSIZE);
#endif
/*
 * Setup function (called once at powerup)
 */
void
setup(void)
{
	mode_led_high();

#ifdef CADFirmware
	g_commandStream.AddCommandHandler(CADCommandStream::cn7, &g_fpgaHandler);
	g_commandStream.AddCommandHandler(CADCommandStream::cn1, &g_flashHandler);
	g_commandStream.AddCommandHandler(CADCommandStream::cn2, &g_utilsHandler);

	g_fpgaHandler.reset(FLASH1);
	HAL_Delay(1000);
	if (!gpio_ishigh(ICE40_CDONE))
	{
		err = ICE_ERROR;
		cdc_puts("Flash Boot Error\n");
	}
	else
	{
		status_led_low();
	}

#else
	// Initiate Ice40 boot from flash
	Ice40.reset(FLASH1);
	HAL_Delay(1000);
	if(!gpio_ishigh(ICE40_CDONE)){
		err = ICE_ERROR;
		cdc_puts("Flash Boot Error\n");
	} else {
		status_led_low();
	}
#endif

	flash_SPI_Disable();

	cdc_puts(VER);
	cdc_puts("\n");
	cdc_puts("Setup done\n");

	USBD_Interface_fops_FS.Receive = &usbcdc_rxcallback;
	HAL_TIM_Base_Start_IT(&htim6);

	// if(err = HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxdmabuf, DMA_BYTES))
	// 	mode_led_low();

	if(err = HAL_UART_Receive_IT(&huart1, (uint8_t *)(rxb + rxi), 1));
		//mode_led_low();


	err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	errors += err ? 1 : 0;
}


/*
 * Loop function (called repeatedly)
 *	- wait for the start of a bitstream to be received on uart1 or usbcdc
 *	- receive the bitstream and pass it to the ice40
 */
uint32_t uTick = 0;
uint32_t uOldTick = 0;

void
loop(void)
{
	char buffer[16];
	// uint8_t b = 0;

	// if (err) {
	// 	status_led_toggle();
	// 	HAL_Delay(100);
	// 	if(gpio_ishigh(MODE_BOOT)) {
	// 		err = 0;
	// 		status_led_low();
	// 	}
	// }
	// if(err) {
	// 	error_report(buffer, 16);
	// 	cdc_puts(buffer);
	// 	err = 0;
	// }

	//cdc_puts("Waiting for USB serial\n");

	uTick = HAL_GetTick();
	if(uTick > uOldTick+1000)
	{
		cdc_puts("Tick changed\n");
		uOldTick = uTick;
	}

#ifdef CADFirmware
	if(gpio_ishigh(MODE_BOOT))
	{
		mode_led_toggle();
		mode = mode ? 0 : 1;

		if(g_flashHandler.CheckId())
		{
			cdc_puts((char *)"AT25SF041 Flash Found\n");

//			uint8_t rxBuffer[256]= {0};
//			uint8_t txBuffer[256]= {0};
////			for(int i=0; i < 256; i++)
////				txBuffer[i] = i;
////
////			g_flash.Erase64K(0);
////
////			for(int p=0; p < 16; p++)
////				g_flash.WritePage(p, txBuffer);
////
////			for(int p=0; p < 16; p++)
////			{
////				g_flash.ReadPage(p, rxBuffer);
////				cdc_puts((char *)"AT25SF041 Flash Found\n");
////			}
//
//			for(uint32_t uPage = 0; uPage < (140000/256); uPage++)
//			{
//					g_flash.ReadPage(uPage, rxBuffer);
//					CDC_Transmit_FS(rxBuffer, 256);
//					mode_led_toggle();
//			}
//
//			//cdc_puts((char *)"Finished\n");
//


//			strcpy((char *)txBuffer, "Test String");
//			uint32_t uLen = strlen((char *)txBuffer);
//
////			g_flash.ReadData(0, rxBuffer, uLen);
//
//			g_flash.ErashFlash();
//
////			g_flash.WriteData(0, txBuffer, uLen);
////
////			g_flash.ReadData(0, rxBuffer, uLen);
//
////			if(rxBuffer[0]=='T')
////				cdc_puts("Read Ok\n");
////			else
////				cdc_puts("Read failed\n");
//
//			if(g_flash.CheckId())
//				cdc_puts((char *)"END AT25SF041 Flash Found\n");
//			else
//				cdc_puts((char *)"END AT25SF041 Flash not Found\n");
//

		}
		else
			cdc_puts((char *)"ERROR: AT25SF041 Flash Not Found\n");

		HAL_Delay(1000);
	}
#else
	if(gpio_ishigh(MODE_BOOT)) {
		mode_led_toggle();
		mode = mode ? 0 : 1;

		if(err) {
			error_report(buffer, 16);
			cdc_puts(buffer);
			err = 0;
		} else {
			// Eventually flash writing will go here, for now just report flash id
			if(flash_id(buffer, 16))
				cdc_puts(buffer);
		}

		HAL_Delay(1000);
	}
#endif
}
/*
 * Write a string to usbcdc, and to uart1 if not detached,
 * adding an extra CR if it ends with LF
 */
void cdc_puts(char *s){
	char *p;

	for (p = s; *p; p++);
	err = CDC_Transmit_FS((uint8_t *)s, p - s);
	errors += err ? 1 : 0;
	if (p > s && p[-1] == '\n')
		cdc_puts("\r");
}

//void flash_SPI_Enable(void){
//
//	HAL_GPIO_DeInit(GPIOB, SPI3_SCK_Pin|SPI3_MISO_Pin|SPI3_MOSI_Pin);
//
//	GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//	/* Peripheral clock enable */
//	__HAL_RCC_SPI3_CLK_ENABLE();
//
//	__HAL_RCC_GPIOB_CLK_ENABLE();
//
//	GPIO_InitStruct.Pin = SPI3_SCK_Pin|SPI3_MISO_Pin|SPI3_MOSI_Pin;
//	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
//	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//}
//
//void flash_SPI_Disable(void){
//
//
//	__HAL_RCC_SPI3_CLK_DISABLE();
//
//
//	HAL_GPIO_DeInit(GPIOB, SPI3_SCK_Pin|SPI3_MISO_Pin|SPI3_MOSI_Pin);
//
//	GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//	GPIO_InitStruct.Pin = SPI3_MISO_Pin | SPI3_SCK_Pin;
//	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//	HAL_GPIO_Init(SPI3_MISO_GPIO_Port, &GPIO_InitStruct);
//
//	GPIO_InitStruct.Pin = SPI3_MOSI_Pin;
//	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//	HAL_GPIO_Init(SPI3_MISO_GPIO_Port, &GPIO_InitStruct);
//
//}

void flash_SPI_Enable(void){

  HAL_GPIO_DeInit(SPI3_MISO_GPIO_Port, SPI3_MISO_Pin);
  HAL_GPIO_DeInit(SPI3_SCK_GPIO_Port, SPI3_SCK_Pin);

  HAL_SPI_MspInit(&hspi3);
}

void flash_SPI_Disable(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  HAL_SPI_MspDeInit(&hspi3);

  GPIO_InitStruct.Pin = SPI3_MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI3_MISO_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPI3_SCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI3_SCK_GPIO_Port, &GPIO_InitStruct);
}

/* Get flash id example flash coms */
uint8_t flash_id(char *buf, int len){
	int r, i;
	int l1 = len - 1;
	Flash flash(&hspi3);
	uint8_t uCommand = 0xAB;
	uint8_t response[3] = {0,0,0};

	release_flash();
	free_flash();
	flash_SPI_Enable();

	gpio_low(ICE40_SPI_CS);
	flash.write(&uCommand,1);
	flash.read(response,3);
	gpio_high(ICE40_SPI_CS);

	uCommand = 0x9F;
	gpio_low(ICE40_SPI_CS);
	flash.write(&uCommand,1);
	flash.read(response,3);
	gpio_high(ICE40_SPI_CS);

	//create a Hex like string from response bytes
	for(r = 0, i = 0; r < l1; r+=5, i++){
		buf[r] = '0';
		buf[r+1] = 'x';
		buf[r+2] = TO_HEX(((response[i] & 0xF0) >> 4));
		buf[r+3] = TO_HEX((response[i] & 0x0F));
		buf[r+4] = ',';
	}
	buf[l1 -1] = '\n';
	buf[l1] = '\0';
	flash_SPI_Disable();

	return len;
}

/* errors to char */
uint8_t error_report(char *buf, int len){
	buf[0] = '0';
	buf[1] = 'x';
	buf[2] = TO_HEX(((errors & 0xF0) >> 4));
	buf[3] = TO_HEX((errors & 0x0F));
	buf[4] = '-';
	buf[5] = 'E';
	buf[6] = 'R';
	buf[7] = 'R';
	buf[8] = 'S';
	buf[9] = '\n';
	buf[10] = '\0';

	return 6;
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin);
  //HAL_GPIO_TogglePin(MODE_LED_GPIO_Port, MODE_LED_Pin);
}

/*
 * Interrupt callback when a packet has been read from usbcdc
 */
static int8_t usbcdc_rxcallback(uint8_t *data, uint32_t *len){

	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &data[0]);
#ifdef CADFirmware
	if(*len)
	{
		if(!g_commandStream.stream(data, *len))
		{
			err = HAL_UART_Transmit_DMA(&huart1, data, *len);
			return USBD_OK;
		}
	}

#else
	if(*len)
		if(!Ice40.stream(data, *len)){
			// HAL_UART_Transmit(&huart1, data, *len, HAL_UART_TIMEOUT_VALUE);
			// mode_led_toggle();

			err = HAL_UART_Transmit_DMA(&huart1, data, *len);
			return USBD_OK;
			//if(temp) mode_led_low();
		}
#endif

	err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	errors += err ? 1 : 0;

	return USBD_OK;
}

/**
  * @brief Tx Transfer completed callbacks
  * @param huart uart handle
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  mode_led_toggle();
  err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  errors += err ? 1 : 0;
}


/**
  * @brief UART error callbacks
  * @param huart uart handle
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
	err = huart->ErrorCode;
	errors += err ? 1 : 0;
	// cdc_puts("Uart error ");
	// cdc_puts('0' + huart->ErrorCode);
	// cdc_puts("\n");
}

// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
// {	
//   /* Prevent unused argument(s) compilation warning */
//   UNUSED(huart);
//   CDC_Transmit_FS((unsigned char *)rxdmabuf, DMA_BYTES);
//   if(err = HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxdmabuf, DMA_BYTES))
// 		mode_led_low();
// }

// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
// {	
//   /* Prevent unused argument(s) compilation warning */
//   UNUSED(huart);
//   int dmas = HAL_UART_Receive_DMA(&huart1, (uint8_t *)&rxdmabuf[dmai], DMAH);
//   CDC_Transmit_FS((unsigned char *)&rxdmabuf[dmao], DMAH);
//   if(dmas == HAL_OK) {
// 	  dmao = dmai;
// 	  dmai = dmai ? 0 : DMAH -1;
// }


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){	
  /* Prevent unused argument(s) compilation warning */
	UNUSED(huart);
  	rxi = (++rxi == RX) ? 0 : rxi;
	err = HAL_UART_Receive_IT(huart, (uint8_t *)(rxb + rxi), 1);
	errors += err ? 1 : 0;
}

/**
  * @brief  TIM period elapsed callback
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	uint32_t bs, bp;

	//mode_led_toggle();

	if(rxo != rxi){
		bs = (rxo > rxi) ? RX - rxo : rxi - rxo;
		bp = rxo;
		if(CDC_Transmit_FS(&rxb[bp], bs) == 0U)
			rxo = (rxo == RX) ? 0 : rxo + bs;
	}
}


Fpga::Fpga(uint32_t img_size){
	NBYTES = img_size;
	state = DETECT;
}

uint8_t Fpga::reset(uint8_t bit_src){
	int timeout = 100;

	gpio_low(ICE40_CRST);

	// Determine FPGA image source, config Flash
	switch(bit_src){
		case MCNTRL : // STM32 master prog, disable flash
			gpio_low(ICE40_SPI_CS);
			protect_flash();
			hold_flash();
			break;
		case FLASH0 : // CBSDEL=00 Flash
			gpio_high(ICE40_SPI_CS);
			hold_flash();
			protect_flash();
			break;
		case FLASH1 : // CBSDEL=01 Flash
			gpio_high(ICE40_SPI_CS);
			release_flash();
			protect_flash();
			break;
	}

	while(timeout--)
		if(gpio_ishigh(ICE40_CRST))
			return TIMEOUT;

	gpio_high(ICE40_CRST);

	// Neede for STM src
	timeout = 100;
	while (gpio_ishigh(ICE40_CDONE)) {
		if (--timeout == 0)
			return TIMEOUT;
	}
	// certainly need this delay for STM as src, not sure about flash boot
	timeout = 12800;
	while(timeout--)
		if(gpio_ishigh(ICE40_SPI_CS))
			return TIMEOUT;

	free_flash();
	release_flash();
	return OK;
}

uint8_t Fpga::config(void){
	uint8_t b = 0;

	for (int timeout = 100; !gpio_ishigh(ICE40_CDONE); timeout--) {
		if (timeout == 0) {
			//cdc_puts("CDONE not set\n");
			return ICE_ERROR;
		}
		write(&b, 1);
	}

	for (int i = 0; i < 7; i++)
		write(&b, 1);

	gpio_high(ICE40_SPI_CS);

	return OK;
}

uint8_t Fpga::write(uint8_t *p, uint32_t len){
	uint32_t i;
	uint8_t ret,b,d;

	ret = HAL_OK;
	for(i = 0; i < len; i++)
	{
		d = *p++;
		for(b = 0; b < 8; b++){
			if(d & 0x80) {
				HAL_GPIO_WritePin(GPIOB,SPI3_SCK_Pin,GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB,SPI3_MISO_Pin,GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOB,SPI3_SCK_Pin,GPIO_PIN_SET);
			} else {
				HAL_GPIO_WritePin(GPIOB,SPI3_MISO_Pin | SPI3_SCK_Pin,GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB,SPI3_SCK_Pin,GPIO_PIN_SET);
			}
			d <<= 1;
		}
		gpio_high(SPI3_SCK);
	}
	return ret;
}

uint8_t Fpga::stream(uint8_t *data, uint32_t len){
	uint32_t *word;
	uint8_t *img;


	switch(state) {
		case DETECT: // Lets look for the Ice40 image or just pass bytes to Uart
			img = data + 4;
			word = (uint32_t *) img;
			if(*word == sig.word){ // We are inside the 1st 4 bytes Ice40 image
				nbytes = 0;
				status_led_high();
				flash_SPI_Disable();
				if (err = reset(MCNTRL))
					flash_SPI_Enable();
				else { // Write bytes (assumes *len < NBYTES)
					nbytes += len - 4;
					write(img, len - 4);
					state = PROG;
				}
			} else
				return 0;
			break;
		case PROG: // We are now in the Ice40 image
			nbytes += len;
			write(data, len);
			break;
	}

	if(nbytes >= NBYTES) {
		if(err = config())
			status_led_high();
		else
			status_led_low();
		flash_SPI_Enable();
		state = DETECT;
	}

	errors += err ? 1 : 0;

	return len;
}


Flash::Flash(SPI_HandleTypeDef *hspi){
	spi = hspi;
}

uint8_t Flash::write(uint8_t *p, uint32_t len){
	return HAL_SPI_Transmit(spi,p, len, HAL_UART_TIMEOUT_VALUE);
}

uint8_t Flash::read(uint8_t *p, uint32_t len){
	return HAL_SPI_Receive(spi, p, len, HAL_UART_TIMEOUT_VALUE);
}

uint8_t Flash::write_read(uint8_t *tx, uint8_t *rx, uint32_t len){
	return HAL_SPI_TransmitReceive(spi, tx, rx, len, HAL_UART_TIMEOUT_VALUE);
}


#ifdef __cplusplus
}
#endif


///*
// * Configure the ICE40 with new bitstreams:
// *	- repeatedly from usbcdc
//*/
///*
//Copyright (c) 2019, Alan Wood All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//Redistributions of source code must retain the above copyright notice, this list
//of conditions and the following disclaimer.
//
//Redistributions in binary form must reproduce the above copyright notice, this
//list of conditions and the following disclaimer in the documentation and/or
//other materials provided with the distribution.
//
//Neither the name of the copyright holder nor the names of its contributors may
//be used to endorse or promote products derived from this software without
//specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// */
//
////#define TIMINGS
//
//#include "main.h"
//#include "usbd_cdc_if.h"
//#include "stm32f7xx_hal.h"
//#include "errno.h"
//#include "mystorm.h"
//
//extern "C" void __cxa_pure_virtual() { while (1); }
//
//#ifdef __cplusplus
// extern "C" {
//#endif
//
//extern SPI_HandleTypeDef hspi3;
//extern USBD_HandleTypeDef hUsbDeviceFS;
//extern UART_HandleTypeDef huart1;
//extern TIM_HandleTypeDef htim6;
//
//// #define DMA_BYTES 2048
//// #define DMAH (DMA_BYTES / 2)
//// uint8_t rxdmabuf[DMA_BYTES];
//// uint8_t urxdmabuf[64];
//// uint8_t urxlen = 0;
//// uint8_t dmai = 0;
//// int dmao = DMAH - 1;
//
//static int mode = 0;
//static int err = 0;
//uint8_t errors = 0;
//
//uint32_t rxi,rxo;
//uint8_t  rxb[RX];
//
///* functions */
//static void cdc_puts(char *s);
//void flash_SPI_Enable(void);
//void flash_SPI_Disable(void);
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
//uint8_t flash_id(char *buf, int len);
//uint8_t error_report(char *buf, int len);
//
///* Interrupts */
//static int8_t usbcdc_rxcallback(uint8_t *data, uint32_t *len);
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
//void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
//
///* Classses */
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
///* global objects */
//Fpga  					Ice40(IMGSIZE);
//Flash 					g_flash(&hspi3);
//Count						g_count;
//CommandStream 	g_commandStream;
//
///*
// * Setup function (called once at powerup)
// */
//
//#ifdef TIMINGS
//volatile uint32_t uInlineFpgaTime;
//volatile uint32_t uVirtualFpgalTime;
//volatile uint32_t uInlineCountTime;
//volatile uint32_t uVirtualCountTime;
//volatile uint32_t uStop;
//volatile uint32_t uCount1;
//volatile uint32_t uCount2;
//
//uint32_t TimeCalls(CommandHandler *pCommandHandler, uint32_t uIters)
//{
//	uint8_t buffer[64];
//
//	uint32_t uStartTicks = HAL_GetTick();
//	for(int i=0; i < uIters; i++)
//		pCommandHandler->streamData(buffer, 64, false);
//	uint32_t uEndTicks = HAL_GetTick();
//
//
//	return uEndTicks - uStartTicks;
//}
//#endif
//
//void
//setup(void)
//{
//	mode_led_high();
//
//	g_commandStream.AddCommandHandler(0x7e, &Ice40);
//	g_commandStream.AddCommandHandler(0x01, &g_flash);
//	g_commandStream.AddCommandHandler(0x02, &g_count);
//
//	// Initiate Ice40 boot from flash
//	Ice40.reset(FLASH1);
//	HAL_Delay(1000);
//	if(!gpio_ishigh(ICE40_CDONE)){
//		err = ICE_ERROR;
//		cdc_puts("Flash Boot Error\n");
//	} else {
//		status_led_low();
//	}
//	flash_SPI_Disable();
//
//	cdc_puts(VER);
//	cdc_puts("\n");
//	cdc_puts("Setup done\n");
//
//	USBD_Interface_fops_FS.Receive = &usbcdc_rxcallback;
//	HAL_TIM_Base_Start_IT(&htim6);
//
//	// if(err = HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxdmabuf, DMA_BYTES))
//	// 	mode_led_low();
//
//	if(err = HAL_UART_Receive_IT(&huart1, (uint8_t *)(rxb + rxi), 1));
//		//mode_led_low();
//
//
//
//
//#ifdef TIMINGS
//
//#define FPGA_ITERS 1000
//#define COUNT_ITERS 1000000
//
//	// timings
//	uint8_t buffer[64];
//	CommandHandler *pFpgaHandler = &Ice40;
//	CommandHandler *pCountHandler = &g_count;
//
//	// inlined
//	uint32_t uStartTicks = HAL_GetTick();
//	for(int i=0; i < FPGA_ITERS; i++)
//		Ice40.streamDataInlined(buffer, 64, false);
//	uint32_t uInlineFpgaTicks = HAL_GetTick();
//
//	for(int i=0; i < COUNT_ITERS; i++)
//			g_count.streamDataInlined(buffer, 64, false);
//	uint32_t uInlineCountTicks = HAL_GetTick();
//
//	uInlineFpgaTime = uInlineFpgaTicks-uStartTicks;
//	uInlineCountTime = uInlineCountTicks - uInlineFpgaTicks;
//
//	uVirtualFpgalTime  = TimeCalls(pFpgaHandler, FPGA_ITERS);
//	uVirtualCountTime = TimeCalls(pCountHandler, COUNT_ITERS);
//
//	uStop = 1;
//#endif
//
//	err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
//	errors += err ? 1 : 0;
//}
//
//
///*
// * Loop function (called repeatedly)
// *	- wait for the start of a bitstream to be received on uart1 or usbcdc
// *	- receive the bitstream and pass it to the ice40
// */
//void
//loop(void)
//{
//	char buffer[16];
//	// uint8_t b = 0;
//
//	// if (err) {
//	// 	status_led_toggle();
//	// 	HAL_Delay(100);
//	// 	if(gpio_ishigh(MODE_BOOT)) {
//	// 		err = 0;
//	// 		status_led_low();
//	// 	}
//	// }
//	// if(err) {
//	// 	error_report(buffer, 16);
//	// 	cdc_puts(buffer);
//	// 	err = 0;
//	// }
//
//	//cdc_puts("Waiting for USB serial\n");
//
//	if(gpio_ishigh(MODE_BOOT)) {
//
//
//		mode_led_toggle();
//		mode = mode ? 0 : 1;
//
//		if(g_flash.CheckId())
//		{
//			cdc_puts((char *)"AT25SF041 Flash Found\n");
//
////			uint8_t rxBuffer[256]= {0};
////			uint8_t txBuffer[256]= {0};
//////			for(int i=0; i < 256; i++)
//////				txBuffer[i] = i;
//////
//////			g_flash.Erase64K(0);
//////
//////			for(int p=0; p < 16; p++)
//////				g_flash.WritePage(p, txBuffer);
//////
//////			for(int p=0; p < 16; p++)
//////			{
//////				g_flash.ReadPage(p, rxBuffer);
//////				cdc_puts((char *)"AT25SF041 Flash Found\n");
//////			}
////
////			for(uint32_t uPage = 0; uPage < (140000/256); uPage++)
////			{
////					g_flash.ReadPage(uPage, rxBuffer);
////					CDC_Transmit_FS(rxBuffer, 256);
////					mode_led_toggle();
////			}
////
////			//cdc_puts((char *)"Finished\n");
////
//
//
////			strcpy((char *)txBuffer, "Test String");
////			uint32_t uLen = strlen((char *)txBuffer);
////
//////			g_flash.ReadData(0, rxBuffer, uLen);
////
////			g_flash.ErashFlash();
////
//////			g_flash.WriteData(0, txBuffer, uLen);
//////
//////			g_flash.ReadData(0, rxBuffer, uLen);
////
//////			if(rxBuffer[0]=='T')
//////				cdc_puts("Read Ok\n");
//////			else
//////				cdc_puts("Read failed\n");
////
////			if(g_flash.CheckId())
////				cdc_puts((char *)"END AT25SF041 Flash Found\n");
////			else
////				cdc_puts((char *)"END AT25SF041 Flash not Found\n");
////
//
//		}
//		else
//			cdc_puts((char *)"ERROR: AT25SF041 Flash Not Found\n");
//
//
//			// Eventually flash writing will go here, for now just report flash id
//
////		char buffer[16];
////		if(flash_id(buffer, 16))
////			cdc_puts(buffer);
//
//		HAL_Delay(1000);
//	}
//}
///*
// * Write a string to usbcdc, and to uart1 if not detached,
// * adding an extra CR if it ends with LF
// */
//static void cdc_puts(char *s){
//	char *p;
//
//	for (p = s; *p; p++);
//	err = CDC_Transmit_FS((uint8_t *)s, p - s);
//	errors += err ? 1 : 0;
//	if (p > s && p[-1] == '\n')
//		cdc_puts("\r");
//}
//
//
////void flash_SPI_Enable(void){
////
////	HAL_GPIO_DeInit(GPIOB, SPI3_SCK_Pin|SPI3_MISO_Pin|SPI3_MOSI_Pin);
////
////	GPIO_InitTypeDef GPIO_InitStruct = {0};
////
////	/* Peripheral clock enable */
////	__HAL_RCC_SPI3_CLK_ENABLE();
////
////	__HAL_RCC_GPIOB_CLK_ENABLE();
////
////	GPIO_InitStruct.Pin = SPI3_SCK_Pin|SPI3_MISO_Pin|SPI3_MOSI_Pin;
////	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
////	GPIO_InitStruct.Pull = GPIO_NOPULL;
////	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
////	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
////	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
////
////}
////
////void flash_SPI_Disable(void){
////
////
////	__HAL_RCC_SPI3_CLK_DISABLE();
////
////
////	HAL_GPIO_DeInit(GPIOB, SPI3_SCK_Pin|SPI3_MISO_Pin|SPI3_MOSI_Pin);
////
////	GPIO_InitTypeDef GPIO_InitStruct = {0};
////
////	GPIO_InitStruct.Pin = SPI3_MISO_Pin | SPI3_SCK_Pin;
////	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
////	GPIO_InitStruct.Pull = GPIO_NOPULL;
////	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
////	HAL_GPIO_Init(SPI3_MISO_GPIO_Port, &GPIO_InitStruct);
////
////	GPIO_InitStruct.Pin = SPI3_MOSI_Pin;
////	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
////	GPIO_InitStruct.Pull = GPIO_NOPULL;
////	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
////	HAL_GPIO_Init(SPI3_MISO_GPIO_Port, &GPIO_InitStruct);
////
////}
//
//
//void flash_SPI_Enable(void){
//
//  HAL_GPIO_DeInit(SPI3_MISO_GPIO_Port, SPI3_MISO_Pin);
//  HAL_GPIO_DeInit(SPI3_SCK_GPIO_Port, SPI3_SCK_Pin);
//
//  HAL_SPI_MspInit(&hspi3);
//}
//
//void flash_SPI_Disable(void){
//  GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//  HAL_SPI_MspDeInit(&hspi3);
//
//  GPIO_InitStruct.Pin = SPI3_MISO_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(SPI3_MISO_GPIO_Port, &GPIO_InitStruct);
//
//  GPIO_InitStruct.Pin = SPI3_SCK_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(SPI3_SCK_GPIO_Port, &GPIO_InitStruct);
//}
//
///* Get flash id example flash coms */
////uint8_t flash_id(char *buf, int len){
////	int r, i;
////	int l1 = len - 1;
////	Flash flash(&hspi3);
////	uint8_t uCommand = 0xAB;
////	uint8_t response[3] = {0,0,0};
////
////	release_flash();
////	free_flash();
////	flash_SPI_Enable();
////
////	gpio_low(ICE40_SPI_CS);
////	flash.write(&uCommand,1);
////	flash.read(response,3);
////	gpio_high(ICE40_SPI_CS);
////
////	uCommand = 0x9F;
////	gpio_low(ICE40_SPI_CS);
////	flash.write(&uCommand,1);
////	flash.read(response,3);
////	gpio_high(ICE40_SPI_CS);
////
////	//create a Hex like string from response bytes
////	for(r = 0, i = 0; r < l1; r+=5, i++){
////		buf[r] = '0';
////		buf[r+1] = 'x';
////		buf[r+2] = TO_HEX(((response[i] & 0xF0) >> 4));
////		buf[r+3] = TO_HEX((response[i] & 0x0F));
////		buf[r+4] = ',';
////	}
////	buf[l1 -1] = '\n';
////	buf[l1] = '\0';
////	flash_SPI_Disable();
////
////	return len;
////}
//
///* errors to char */
//uint8_t error_report(char *buf, int len){
//	buf[0] = '0';
//	buf[1] = 'x';
//	buf[2] = TO_HEX(((errors & 0xF0) >> 4));
//	buf[3] = TO_HEX((errors & 0x0F));
//	buf[4] = '-';
//	buf[5] = 'E';
//	buf[6] = 'R';
//	buf[7] = 'R';
//	buf[8] = 'S';
//	buf[9] = '\n';
//	buf[10] = '\0';
//
//	return 6;
//}
//
///**
//  * @brief  EXTI line detection callbacks.
//  * @param  GPIO_Pin Specifies the pins connected EXTI line
//  * @retval None
//  */
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
//  /* Prevent unused argument(s) compilation warning */
//  UNUSED(GPIO_Pin);
//  //HAL_GPIO_TogglePin(MODE_LED_GPIO_Port, MODE_LED_Pin);
//}
//
///*
// * Interrupt callback when a packet has been read from usbcdc
// */
//static int8_t usbcdc_rxcallback(uint8_t *data, uint32_t *len){
//
//	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &data[0]);
//
//#define COMMAND_HANDLER
//
//#ifdef COMMAND_HANDLER
//	if(*len)
//	{
//		if(!g_commandStream.stream(data, *len))
//		{
//			// HAL_UART_Transmit(&huart1, data, *len, HAL_UART_TIMEOUT_VALUE);
//			// mode_led_toggle();
//
//			err = HAL_UART_Transmit_DMA(&huart1, data, *len);
//			return USBD_OK;
//			//if(temp) mode_led_low();
//		}
//	}
//#else
//	if(*len)
//		if(!Ice40.stream(data, *len)){
//			// HAL_UART_Transmit(&huart1, data, *len, HAL_UART_TIMEOUT_VALUE);
//			// mode_led_toggle();
//
//
//			HAL_UART_Transmit_DMA(&huart1, data, *len);
//			return USBD_OK;
//			//if(temp) mode_led_low();
//		}
//#endif
//	err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
//	errors += err ? 1 : 0;
//
//	return USBD_OK;
//}
//
///**
//  * @brief Tx Transfer completed callbacks
//  * @param huart uart handle
//  * @retval None
//  */
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
//  /* Prevent unused argument(s) compilation warning */
//  UNUSED(huart);
//  mode_led_toggle();
//  err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
//  errors += err ? 1 : 0;
//}
//
//
///**
//  * @brief UART error callbacks
//  * @param huart uart handle
//  * @retval None
//  */
//void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
//  /* Prevent unused argument(s) compilation warning */
//  UNUSED(huart);
//	err = huart->ErrorCode;
//	errors += err ? 1 : 0;
//	// cdc_puts("Uart error ");
//	// cdc_puts('0' + huart->ErrorCode);
//	// cdc_puts("\n");
//}
//
//// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//// {
////   /* Prevent unused argument(s) compilation warning */
////   UNUSED(huart);
////   CDC_Transmit_FS((unsigned char *)rxdmabuf, DMA_BYTES);
////   if(err = HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxdmabuf, DMA_BYTES))
//// 		mode_led_low();
//// }
//
//// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//// {
////   /* Prevent unused argument(s) compilation warning */
////   UNUSED(huart);
////   int dmas = HAL_UART_Receive_DMA(&huart1, (uint8_t *)&rxdmabuf[dmai], DMAH);
////   CDC_Transmit_FS((unsigned char *)&rxdmabuf[dmao], DMAH);
////   if(dmas == HAL_OK) {
//// 	  dmao = dmai;
//// 	  dmai = dmai ? 0 : DMAH -1;
//// }
//
//
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
//  /* Prevent unused argument(s) compilation warning */
//	UNUSED(huart);
//  	rxi = (++rxi == RX) ? 0 : rxi;
//	err = HAL_UART_Receive_IT(huart, (uint8_t *)(rxb + rxi), 1);
//	errors += err ? 1 : 0;
//}
//
///**
//  * @brief  TIM period elapsed callback
//  * @param  htim: TIM handle
//  * @retval None
//  */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
//	uint32_t bs, bp;
//
//	//mode_led_toggle();
//
//	if(rxo != rxi){
//		bs = (rxo > rxi) ? RX - rxo : rxi - rxo;
//		bp = rxo;
//		if(CDC_Transmit_FS(&rxb[bp], bs) == 0U)
//			rxo = (rxo == RX) ? 0 : rxo + bs;
//	}
//}
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////
//// Fpga
///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////
//// Flash
///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////
//// CommandStream
///////////////////////////////////////////////////////////////////////////////
//
//
//#ifdef __cplusplus
//}
//#endif
//
