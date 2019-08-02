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

//#define TIMINGS

#include "main.h"
#include "usbd_cdc_if.h"
#include "stm32f7xx_hal.h"
#include "errno.h"
#include "mystorm.h"

extern "C" void __cxa_pure_virtual() { while (1); }

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
static void cdc_puts(char *s);
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

class CommandHandler
{
public:
	virtual bool streamData(uint8_t *data, uint32_t len, bool bEndStream) = 0;
	virtual bool init(void) = 0;
};

class Count : public CommandHandler
{
public:
	virtual bool streamData(uint8_t *data, uint32_t len, bool bEndStream);
	virtual bool init(void);

#ifdef TIMINGS
	inline bool streamDataInlined(uint8_t *data, uint32_t len, bool bEndStream)
	{
		m_uCount+= len;
		if(bEndStream)
		{
			char buffer[128];
			//sprintf(buffer, "%lu bytes received\n", m_uCount);
			cdc_puts(buffer);
			return false;
		}
		else
			return true;
	}
#endif

private:
	volatile uint32_t m_uCount;
};








class Fpga : public CommandHandler {
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
		virtual bool streamData(uint8_t *data, uint32_t len, bool bEndStream);
		virtual bool init(void);


#ifdef TIMINGS
		inline bool streamDataInlined(uint8_t *data, uint32_t len, bool bEndStream){
			bool bResult = true;

			nbytes += len;
			write(data, len);

			if(nbytes >= NBYTES)
			{
				if(err = config())
					status_led_high();
				else
					status_led_low();
				flash_SPI_Enable();
				bResult = false;
			}

			return bResult;
		}
#endif

};








class Flash: public CommandHandler
{
public:
	Flash(SPI_HandleTypeDef *hspi);

	bool CheckId(void);
	void EraseFlash(void);
	void Erase64K(uint8_t uPage);

	bool WriteData(uint32_t uAddr, uint8_t *pData, uint32_t uLen);
	bool ReadData(uint32_t uAddr, uint8_t *pData, uint32_t uLen);

	bool WritePage(uint16_t uPage, uint8_t *pData);
	bool ReadPage(uint16_t uPage, uint8_t *pData);

	void PollForReady(void);
	bool IsReady(void);

	void InitBufferedWrite();
	void BufferedWrite(uint8_t *pData, uint32_t uLen);
	void FlushBuffer(void);


private:

	void Enable(void);
	void Disable(void);



	uint8_t write(uint8_t *p, uint32_t len);
	uint8_t read(uint8_t *p, uint32_t len);
	uint8_t write_read(uint8_t *tx, uint8_t *rx, uint32_t len);

	virtual bool streamData(uint8_t *data, uint32_t len, bool bEndStream);
	virtual bool init(void);

	SPI_HandleTypeDef *spi;

	void WriteEnable(void);

	uint32_t m_uCur256BytePage;
	uint16_t m_uBufferPos;
	uint32_t m_uTotalBytes;
	uint8_t  m_buffer[256];
};

class CommandStream
{
public:
	CommandStream(void)
	{
		Init();
	};

	void Init(void);
	void AddCommandHandler(uint8_t uCommandByte, CommandHandler *pCommandHandler);

	uint8_t stream(uint8_t *data, uint32_t len);

private:
	typedef enum { stDetect, stDetectCommandByte, stDispatch, stProcessing, stError } StreamState;

	StreamState m_state;

	uint8_t			m_header[3] = {0x7e, 0xaa, 0x99};
	uint8_t			m_uHeaderScanPos = 0;

	CommandHandler		*m_pCurrentCommandHandler;
	CommandHandler 		*m_commandHandlers[256] = {0};
};


/* global objects */
Fpga  					Ice40(IMGSIZE);
Flash 					g_flash(&hspi3);
Count						g_count;
CommandStream 	g_commandStream;

/*
 * Setup function (called once at powerup)
 */

#ifdef TIMINGS
volatile uint32_t uInlineFpgaTime;
volatile uint32_t uVirtualFpgalTime;
volatile uint32_t uInlineCountTime;
volatile uint32_t uVirtualCountTime;
volatile uint32_t uStop;
volatile uint32_t uCount1;
volatile uint32_t uCount2;

uint32_t TimeCalls(CommandHandler *pCommandHandler, uint32_t uIters)
{
	uint8_t buffer[64];

	uint32_t uStartTicks = HAL_GetTick();
	for(int i=0; i < uIters; i++)
		pCommandHandler->streamData(buffer, 64, false);
	uint32_t uEndTicks = HAL_GetTick();


	return uEndTicks - uStartTicks;
}
#endif

void
setup(void)
{
	mode_led_high();

	g_commandStream.AddCommandHandler(0x7e, &Ice40);
	g_commandStream.AddCommandHandler(0x01, &g_flash);
	g_commandStream.AddCommandHandler(0x02, &g_count);

	// Initiate Ice40 boot from flash
	Ice40.reset(FLASH1);
	HAL_Delay(1000);
	if(!gpio_ishigh(ICE40_CDONE)){
		err = ICE_ERROR;
		cdc_puts("Flash Boot Error\n");
	} else {
		status_led_low();
	}
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




#ifdef TIMINGS

#define FPGA_ITERS 1000
#define COUNT_ITERS 1000000

	// timings
	uint8_t buffer[64];
	CommandHandler *pFpgaHandler = &Ice40;
	CommandHandler *pCountHandler = &g_count;

	// inlined
	uint32_t uStartTicks = HAL_GetTick();
	for(int i=0; i < FPGA_ITERS; i++)
		Ice40.streamDataInlined(buffer, 64, false);
	uint32_t uInlineFpgaTicks = HAL_GetTick();

	for(int i=0; i < COUNT_ITERS; i++)
			g_count.streamDataInlined(buffer, 64, false);
	uint32_t uInlineCountTicks = HAL_GetTick();

	uInlineFpgaTime = uInlineFpgaTicks-uStartTicks;
	uInlineCountTime = uInlineCountTicks - uInlineFpgaTicks;

	uVirtualFpgalTime  = TimeCalls(pFpgaHandler, FPGA_ITERS);
	uVirtualCountTime = TimeCalls(pCountHandler, COUNT_ITERS);

	uStop = 1;
#endif

	err = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	errors += err ? 1 : 0;
}


/*
 * Loop function (called repeatedly)
 *	- wait for the start of a bitstream to be received on uart1 or usbcdc
 *	- receive the bitstream and pass it to the ice40
 */
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

	if(gpio_ishigh(MODE_BOOT)) {


		mode_led_toggle();
		mode = mode ? 0 : 1;

		if(g_flash.CheckId())
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


			// Eventually flash writing will go here, for now just report flash id

//		char buffer[16];
//		if(flash_id(buffer, 16))
//			cdc_puts(buffer);

		HAL_Delay(1000);
	}
}
/*
 * Write a string to usbcdc, and to uart1 if not detached,
 * adding an extra CR if it ends with LF
 */
static void cdc_puts(char *s){
	char *p;

	for (p = s; *p; p++);
	err = CDC_Transmit_FS((uint8_t *)s, p - s);
	errors += err ? 1 : 0;
	if (p > s && p[-1] == '\n')
		cdc_puts("\r");
}


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
//uint8_t flash_id(char *buf, int len){
//	int r, i;
//	int l1 = len - 1;
//	Flash flash(&hspi3);
//	uint8_t uCommand = 0xAB;
//	uint8_t response[3] = {0,0,0};
//
//	release_flash();
//	free_flash();
//	flash_SPI_Enable();
//
//	gpio_low(ICE40_SPI_CS);
//	flash.write(&uCommand,1);
//	flash.read(response,3);
//	gpio_high(ICE40_SPI_CS);
//
//	uCommand = 0x9F;
//	gpio_low(ICE40_SPI_CS);
//	flash.write(&uCommand,1);
//	flash.read(response,3);
//	gpio_high(ICE40_SPI_CS);
//
//	//create a Hex like string from response bytes
//	for(r = 0, i = 0; r < l1; r+=5, i++){
//		buf[r] = '0';
//		buf[r+1] = 'x';
//		buf[r+2] = TO_HEX(((response[i] & 0xF0) >> 4));
//		buf[r+3] = TO_HEX((response[i] & 0x0F));
//		buf[r+4] = ',';
//	}
//	buf[l1 -1] = '\n';
//	buf[l1] = '\0';
//	flash_SPI_Disable();
//
//	return len;
//}

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

#define COMMAND_HANDLER

#ifdef COMMAND_HANDLER
	if(*len)
	{
		if(!g_commandStream.stream(data, *len))
		{
			// HAL_UART_Transmit(&huart1, data, *len, HAL_UART_TIMEOUT_VALUE);
			// mode_led_toggle();

			err = HAL_UART_Transmit_DMA(&huart1, data, *len);
			return USBD_OK;
			//if(temp) mode_led_low();
		}
	}
#else
	if(*len)
		if(!Ice40.stream(data, *len)){
			// HAL_UART_Transmit(&huart1, data, *len, HAL_UART_TIMEOUT_VALUE);
			// mode_led_toggle();


			HAL_UART_Transmit_DMA(&huart1, data, *len);
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





/////////////////////////////////////////////////////////////////////////////
// Fpga
/////////////////////////////////////////////////////////////////////////////

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

bool Fpga::init(void)
{
	bool bResult = false;
	nbytes = 0;
	status_led_high();
	flash_SPI_Disable();
	if (err = reset(MCNTRL))
		flash_SPI_Enable();
	else
	{
		nbytes+=4;
		write((uint8_t *)&sig, 4);
		bResult = true;
	}

	return bResult;
}

bool Fpga::streamData(uint8_t *data, uint32_t len, bool bEndStream){
	bool bResult = true;

	nbytes += len;
	write(data, len);

	if(bEndStream || (nbytes >= NBYTES))
	{
		if(err = config())
			status_led_high();
		else
			status_led_low();
		flash_SPI_Enable();
		bResult = false;
	}

	return bResult;
}





/////////////////////////////////////////////////////////////////////////////
// Flash
/////////////////////////////////////////////////////////////////////////////

Flash::Flash(SPI_HandleTypeDef *hspi){
	spi = hspi;
}

/* Get flash id example flash coms */
//uint8_t flash_id(char *buf, int len){
//	int r, i;
//	int l1 = len - 1;
//	Flash flash(&hspi3);
//	uint8_t uCommand = 0xAB;
//	uint8_t response[3] = {0,0,0};
//
//	release_flash();
//	free_flash();
//	flash_SPI_Enable();
//
//	gpio_low(ICE40_SPI_CS);
//	flash.write(&uCommand,1);
//	flash.read(response,3);
//	gpio_high(ICE40_SPI_CS);
//
//	uCommand = 0x9F;
//	gpio_low(ICE40_SPI_CS);
//	flash.write(&uCommand,1);
//	flash.read(response,3);
//	gpio_high(ICE40_SPI_CS);
//
//	//create a Hex like string from response bytes
//	for(r = 0, i = 0; r < l1; r+=5, i++){
//		buf[r] = '0';
//		buf[r+1] = 'x';
//		buf[r+2] = TO_HEX(((response[i] & 0xF0) >> 4));
//		buf[r+3] = TO_HEX((response[i] & 0x0F));
//		buf[r+4] = ',';
//	}
//	buf[l1 -1] = '\n';
//	buf[l1] = '\0';
//	flash_SPI_Disable();
//
//	return len;
//}

uint8_t Flash::write(uint8_t *p, uint32_t len){
	return HAL_SPI_Transmit(spi,p, len, HAL_UART_TIMEOUT_VALUE);
}

uint8_t Flash::read(uint8_t *p, uint32_t len){
	return HAL_SPI_Receive(spi, p, len, HAL_UART_TIMEOUT_VALUE);
}

uint8_t Flash::write_read(uint8_t *tx, uint8_t *rx, uint32_t len){
	return HAL_SPI_TransmitReceive(spi, tx, rx, len, HAL_UART_TIMEOUT_VALUE);
}

void Flash::Enable(void)
{
	release_flash();
	free_flash();
	flash_SPI_Enable();

	uint8_t uCommand = 0xAB;
	gpio_low(ICE40_SPI_CS);
	write(&uCommand,1);
	gpio_high(ICE40_SPI_CS);
}

void Flash::Disable(void)
{
	flash_SPI_Disable();
}

bool Flash::CheckId(void)
{
	Enable();

	uint8_t uCommand = 0x9F;
	uint8_t response[3] = {0,0,0};

	gpio_low(ICE40_SPI_CS);
	write(&uCommand,1);
	read(response,3);
	gpio_high(ICE40_SPI_CS);

	Disable();

	return((response[0]==0x1F) &&  (response[1]==0x84) && (response[2]==0x01));
}

bool Flash::init(void)
{
	bool bResult = false;

	if(CheckId())
	{
		EraseFlash();
//		Erase64K(0);
//		Erase64K(1);
//		Erase64K(2);

		InitBufferedWrite();

		bResult = true;
	}

	return bResult;
}

void Flash::InitBufferedWrite(void)
{
	m_uTotalBytes = 0;
	m_uCur256BytePage = 0;
	m_uBufferPos = 0;
}

void Flash::BufferedWrite(uint8_t *pData, uint32_t uLen)
{
	m_uTotalBytes+= uLen;

	while (uLen)
	{
		uint16_t uRemaining = 256 - m_uBufferPos;
		if(uLen < uRemaining)
		{
			memcpy(m_buffer+m_uBufferPos, pData, uLen);
			m_uBufferPos += uLen;
			uLen = 0;
		}
		else
		{
			memcpy(m_buffer+m_uBufferPos, pData, uRemaining);
			m_uBufferPos += uRemaining;
			pData += uRemaining;
			uLen -= uRemaining;
		}

		if(m_uBufferPos == 256)
		{
			// ok write to flash
			WritePage(m_uCur256BytePage, m_buffer);

			m_uCur256BytePage++;
			m_uBufferPos=0;
		}

	}
}

void Flash::FlushBuffer(void)
{
	if(m_uBufferPos)
		WritePage(m_uCur256BytePage, m_buffer);
}

bool Flash::streamData(uint8_t *data, uint32_t len, bool bEndStream)
{
	bool bResult = true;

	if(!m_uTotalBytes) // bodge lookat doing properly
	{
		// skip 4 byte header
		data+=4;
		len-=4;
	}

	BufferedWrite(data, len);

	if(bEndStream || (m_uTotalBytes >= (IMGSIZE)))
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

bool Flash::IsReady(void)
{

	Enable();
	uint8_t uCommand = 0x05;
	uint8_t response;

	gpio_low(ICE40_SPI_CS);
	write(&uCommand,1);
	read(&response,1);
	gpio_high(ICE40_SPI_CS);

	Disable();

	return (!(response & 1));
}

void Flash::PollForReady(void)
{
	while(!IsReady())
		;
}

bool Flash::WritePage(uint16_t uPage, uint8_t *pData)
{
	WriteEnable();

	uint8_t uMSB = (uPage >> 8) & 0xFF;
	uint8_t uLSB = uPage &0xFF;


	gpio_low(ICE40_SPI_CS);
	uint8_t command[4] = {0x02, uMSB, uLSB,0x00};
	write(command,4);

	write(pData, 256);
	gpio_high(ICE40_SPI_CS);

	Disable();

	PollForReady();

}



bool Flash::ReadPage(uint16_t uPage, uint8_t *pData)
{
	Enable();

	uint8_t uMSB = (uPage >> 8) & 0xFF;
	uint8_t uLSB = uPage &0xFF;


	gpio_low(ICE40_SPI_CS);
	uint8_t command[4] = {0x03, uMSB, uLSB, 0x00};
	write(command,4);

	read(pData, 256);
	gpio_high(ICE40_SPI_CS);

	Disable();
}



bool Flash::WriteData(uint32_t uAddr, uint8_t *pData, uint32_t uLen)
{
	WriteEnable();

	// just address 0 at the moment
	gpio_low(ICE40_SPI_CS);
	uint8_t command[4] = {0x02,0x00,0x00,0x00};
	write(command,4);

	write(pData, uLen);
	gpio_high(ICE40_SPI_CS);

	Disable();

	PollForReady();
}

bool Flash::ReadData(uint32_t uAddr, uint8_t *pData, uint32_t uLen)
{
	PollForReady();

	Enable();

	// just address 0 at the moment
	gpio_low(ICE40_SPI_CS);
	uint8_t command[4] = {0x03,0x00,0x00,0x00};
	write(command,4);

	read(pData, uLen);
	gpio_high(ICE40_SPI_CS);

	Disable();
}

void Flash::WriteEnable(void)
{
	Enable();

	uint8_t uCommand = 0x06;

	gpio_low(ICE40_SPI_CS);
	write(&uCommand,1);
	gpio_high(ICE40_SPI_CS);

}

void Flash::EraseFlash(void)
{
	PollForReady();

	WriteEnable();

	uint8_t uCommand = 0x60;

	gpio_low(ICE40_SPI_CS);
	write(&uCommand,1);
	gpio_high(ICE40_SPI_CS);

	Disable();

	PollForReady();

}

void Flash::Erase64K(uint8_t uPage)
{
	PollForReady();

	WriteEnable();

	uint8_t command[] = { 0xD8, uPage, 0, 0};

	gpio_low(ICE40_SPI_CS);
	write(command,4);
	gpio_high(ICE40_SPI_CS);

	Disable();

	PollForReady();

}




bool Count::init(void)
{
	cdc_puts("Count::init()\n");
	m_uCount = 0;
	return true;
}

bool Count::streamData(uint8_t *data, uint32_t len, bool bEndStream)
{
	m_uCount+= len;
	if(bEndStream)
	{
		char buffer[128];
		//sprintf(buffer, "%lu bytes received\n", m_uCount);
		cdc_puts(buffer);
		return false;
	}
	else
		return true;
}



/////////////////////////////////////////////////////////////////////////////
// CommandStream
/////////////////////////////////////////////////////////////////////////////

void CommandStream::Init(void)
{
	m_state = stDetect;
	m_uHeaderScanPos = 0;
}

void CommandStream::AddCommandHandler(uint8_t uCommandByte, CommandHandler *pCommandHandler)
{
	// no error checking
	m_commandHandlers[uCommandByte] = pCommandHandler;
}


uint8_t CommandStream::stream(uint8_t *data, uint32_t len)
{
	uint8_t *pScanPos = data;
	uint8_t uCommandByte = 0;
	bool    bEndStream = len < 64;

	if(m_state == stDetect)
	{
		bool bFound = false;
		while ((!bFound) && (pScanPos < (data+len)))
		{
			if(*pScanPos++ == m_header[m_uHeaderScanPos])
			{
				m_uHeaderScanPos++;
				if(m_uHeaderScanPos == 3)
				{
					// header found
					bFound = true;
				}
			}
			else
				m_uHeaderScanPos = 0;
		}

		if(bFound)
		{
			// next command byte could be in next call
			if(pScanPos > (data + len))
				m_state = stDetectCommandByte;
			else
			{
				uCommandByte = *pScanPos++;
				m_state = stDispatch;
			}
		}
	}
	else
	{
		if(m_state == stDetectCommandByte)
		{
			uCommandByte = *pScanPos++;
			m_state = stDispatch;
		}
	}

	if(m_state == stDispatch)
	{
		m_pCurrentCommandHandler = m_commandHandlers[uCommandByte];
		if(m_pCurrentCommandHandler)
		{
			if(m_pCurrentCommandHandler->init())
				m_state = stProcessing;
			else
				m_state = stDetect;
		}
		else
			m_state = stDetect; // No handler go back to detect state
	}

	if(m_state == stProcessing)
	{
		if(!m_pCurrentCommandHandler->streamData(pScanPos, len - (pScanPos - data), bEndStream))
		{
			// handler has finished go back to detect state
			m_state = stDetect;
		}
	}

	return m_state != stDetect;
}

#ifdef __cplusplus
}
#endif

