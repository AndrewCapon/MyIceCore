/*
 * CADCountHandler.h
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADUTILSHANDLER_H_
#define MYSTORM_CADUTILSHANDLER_H_

#include "CAD.h"

#include "CADCommandHandler.h"

class CADUtilsHandler : public CADCommandHandler
{
public:
	virtual bool streamData(uint8_t *data, uint32_t len, bool bEndStream);
	virtual bool init(uint8_t uSubCommand);

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

#endif /* MYSTORM_CADUTILSHANDLER_H_ */
