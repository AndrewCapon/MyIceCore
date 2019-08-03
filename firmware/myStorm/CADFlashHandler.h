/*
 * CADFlashHandler.h
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADFLASHHANDLER_H_
#define MYSTORM_CADFLASHHANDLER_H_

#include "CAD.h"

#include "CADCommandHandler.h"

class CADFlashHandler: public CADCommandHandler
{
public:
	CADFlashHandler(SPI_HandleTypeDef *hspi);

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
	//void Disable(void);



	uint8_t write(uint8_t *p, uint32_t len);
	uint8_t read(uint8_t *p, uint32_t len);
	uint8_t write_read(uint8_t *tx, uint8_t *rx, uint32_t len);

	virtual bool streamData(uint8_t *data, uint32_t len);
	virtual bool init(uint8_t uSubCommand);

	SPI_HandleTypeDef *spi;

	void WriteEnable(void);

	uint32_t m_uCur256BytePage;
	uint16_t m_uBufferPos;
	uint8_t  m_buffer[256];
};

#endif /* MYSTORM_CADFLASHHANDLER_H_ */
