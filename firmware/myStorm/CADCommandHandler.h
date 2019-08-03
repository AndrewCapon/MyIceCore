/*
 * CADCommandHandler.h
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADCOMMANDHANDLER_H_
#define MYSTORM_CADCOMMANDHANDLER_H_

#include "CAD.h"

class CADCommandHandler
{
public:
	virtual bool streamData(uint8_t *data, uint32_t len) = 0;
	virtual bool init(uint8_t uSubCommand) = 0;

	uint32_t GetBytesHandled(void)
	{
		return m_uBytesHandled;
	}

protected:
	uint32_t m_uBytesHandled = 0;
};



#endif /* MYSTORM_CADCOMMANDHANDLER_H_ */
