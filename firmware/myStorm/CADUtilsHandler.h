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
	virtual bool streamData(uint8_t *data, uint32_t len);
	virtual bool init(uint8_t uSubCommand);

#ifdef TIMINGS
	inline bool streamDataInlined(uint8_t *data, uint32_t len)
	{
		m_uCount+= len;
	}
#endif

private:
	volatile uint32_t m_uCount;
};

#endif /* MYSTORM_CADUTILSHANDLER_H_ */
