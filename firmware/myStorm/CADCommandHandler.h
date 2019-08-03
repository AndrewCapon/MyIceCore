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
	virtual bool streamData(uint8_t *data, uint32_t len, bool bEndStream) = 0;
	virtual bool init(uint8_t uSubCommand) = 0;
};



#endif /* MYSTORM_CADCOMMANDHANDLER_H_ */
