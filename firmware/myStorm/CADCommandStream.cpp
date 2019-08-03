/*
 * CADCommandStream.cpp
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#include "CADCommandStream.h"
#include "CADCommandHandler.h"

void CADCommandStream::Init(void)
{
	m_state = stDetect;
	m_uHeaderScanPos = 0;
}

void CADCommandStream::AddCommandHandler(CADCommandStream::CommandNibble uCommandNibble, CADCommandHandler *pCommandHandler)
{
	m_commandHandlers[uCommandNibble] = pCommandHandler;
}


uint8_t CADCommandStream::stream(uint8_t *data, uint32_t len)
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
		m_pCurrentCommandHandler = m_commandHandlers[uCommandByte>>4];
		if(m_pCurrentCommandHandler)
		{
			if(m_pCurrentCommandHandler->init(uCommandByte & 0xf))
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
