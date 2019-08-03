/*
 * CADCountHandler.cpp
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#include "CADUtilsHandler.h"


bool CADUtilsHandler::init(uint8_t uSubCommand)
{
	switch(uSubCommand)
	{
		case 0: // reset
			NVIC_SystemReset();
		break;
	}

	return false;
}

bool CADUtilsHandler::streamData(uint8_t *data, uint32_t len)
{
	return false;
}
