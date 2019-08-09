/*
 * CADDataStream.h
 *
 *  Created on: 3 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADDATASTREAM_H_
#define MYSTORM_CADDATASTREAM_H_
#include "CAD.h"

class CADDataStream
{
public:
	CADDataStream() : m_pData(NULL), m_uLen(0), m_pOverflowData(m_overflowBuffer), m_uOverflowLen(0)
	{

	}

	bool AddData(uint8_t *pData, uint32_t uLen)
	{
		bool bResult = true;
		if (m_uLen)
		{
			// existing data left over
			if(m_uOverflowLen + m_uLen > 64)
			{
				// bad, out of leftover memory
				bResult = false;
			}
			else
			{
				memcpy(m_overflowBuffer + m_uOverflowLen, m_pData + m_uLen, m_uLen);
				m_uOverflowLen += uLen;
			}
		}

		if(bResult)
		{
			m_pData = pData;
			m_uLen  = uLen;
		}

		return bResult;
	}

	uint32_t GetStreamLength(void)
	{
		uint32_t uLength;

		if(m_uOverflowLen)
			uLength = m_uOverflowLen;
		else
			uLength = m_uLen;

		return uLength;
	}

	bool GetStreamData(uint8_t **ppData, uint32_t *puLen)
	{
		bool bResult = true;

		if(m_uOverflowLen)
		{
			*ppData = m_pOverflowData;
			*puLen = m_uOverflowLen;
			m_uOverflowLen = 0;
			m_pOverflowData = m_overflowBuffer;
		}
		else
		{
			if(m_uLen)
			{
				*ppData = m_pData;
				*puLen = m_uLen;
				m_uLen = 0;
			}
			else
			{
				bResult = false;
			}
		}

		return bResult;
	}

	bool GetDataOfLength(void *pData, uint8_t uLen)
	{
		bool bResult = false;
		uint8_t *pByteData = (uint8_t *)pData;

		if(m_uOverflowLen + m_uLen >= uLen)
		{
			while(uLen--)
			{
				if(m_uOverflowLen)
				{
					*pByteData++ = *m_pOverflowData++;
					m_uOverflowLen--;
				}
				else
				{
					*pByteData++ = *m_pData++;
					m_uLen--;
				}
			}
		}

		return bResult;
	}

	bool Get(uint32_t &uValue)
	{
		return GetDataOfLength(&uValue, 4);
	}


private:
	uint8_t		*m_pData;
	uint32_t 	m_uLen;

	uint8_t		m_overflowBuffer[64];
	uint8_t   *m_pOverflowData;
	uint32_t 	m_uOverflowLen;

};

#endif /* MYSTORM_CADDATASTREAM_H_ */
