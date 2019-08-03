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


char* ultoa( unsigned long value, char *string, int radix )
{
  char tmp[33];
  char *tp = tmp;
  long i;
  unsigned long v = value;
  char *sp;

  if ( string == NULL )
  {
    return 0;
  }

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  sp = string;


  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;

  return string;
}

void logNumber(char *pStr, uint32_t uNum)
{
	char buffer[32];
	ultoa(uNum, buffer, 10);

	cdc_puts(pStr);
	cdc_puts(buffer);
	cdc_puts((char *)"\n");

}

void CADCommandStream::LogTimeTaken(uint32_t uBytesHandled)
{
	// log time taken
	char timeBuffer[128];
	char bytesBuffer[128];

	uint32_t uTimeTaken = HAL_GetTick() - m_uDispatchTime;

	ultoa(uTimeTaken, timeBuffer,10);
	ultoa(uBytesHandled, bytesBuffer,10);

	char buffer[256];
	strcpy(buffer, bytesBuffer);
	strcat(buffer, " bytes handled, took: ");
	strcat(buffer, timeBuffer);
	strcat(buffer, "ms\n");

	cdc_puts(buffer);
}

uint8_t CADCommandStream::stream(uint8_t *data, uint32_t len)
{
	uint8_t *pScanPos = data;
	uint8_t uCommandByte = 0;

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
		m_uDispatchTime = HAL_GetTick();

		m_pCurrentCommandHandler = m_commandHandlers[uCommandByte>>4];
		if(m_pCurrentCommandHandler)
		{
			if(m_pCurrentCommandHandler->init(uCommandByte & 0xf))
				m_state = stProcessing;
			else
			{
				m_state = stDetect;
				LogTimeTaken(0);
			}
		}
		else
			m_state = stDetect; // No handler go back to detect state
	}

	if(m_state == stProcessing)
	{
		if(!m_pCurrentCommandHandler->streamData(pScanPos, len - (pScanPos - data)))
		{
			// handler has finished go back to detect state
			m_state = stDetect;
			LogTimeTaken(m_pCurrentCommandHandler->GetBytesHandled());
		}
	}


	return m_state != stDetect;
}

uint32_t CADCommandStream::GetTimeTaken(void)
{
	return HAL_GetTick() - m_uDispatchTime;
}
