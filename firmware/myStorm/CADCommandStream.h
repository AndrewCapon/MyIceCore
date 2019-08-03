/*
 * CADCommandStream.h
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADCOMMANDSTREAM_H_
#define MYSTORM_CADCOMMANDSTREAM_H_

#include "CAD.h"

class CADCommandHandler;

class CADCommandStream
{
public:
	CADCommandStream(void)
	{
		Init();
	};

	typedef enum { cn0, cn1, cn2, cn3, cn4, cn5, cn6, cn7, cn8, cn9, cnA, cnB, cnC, cnD, cnE, cnF } CommandNibble;

	void Init(void);
	void AddCommandHandler(CommandNibble uCommandNibble, CADCommandHandler *pCommandHandler);

	uint8_t  stream(uint8_t *data, uint32_t len);
	uint32_t GetTimeTaken(void);

private:
	typedef enum { stDetect, stDetectCommandByte, stDispatch, stProcessing, stError } StreamState;

	StreamState m_state;

	uint8_t			m_header[3] = {0x7e, 0xaa, 0x99};
	uint8_t			m_uHeaderScanPos = 0;

	CADCommandHandler		*m_pCurrentCommandHandler;
	CADCommandHandler 	*m_commandHandlers[16] = {0};

	uint32_t m_uDispatchTime;

	void LogTimeTaken(uint32_t uBytesHandled);

};

#endif /* MYSTORM_CADCOMMANDSTREAM_H_ */
