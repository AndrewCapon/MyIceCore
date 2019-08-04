/*
 * CADCommandHandler.h
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADCOMMANDHANDLER_H_
#define MYSTORM_CADCOMMANDHANDLER_H_

#include "CAD.h"
#include "CADDataStream.h"

class CADCommandHandler
{
public:
	typedef enum { srError, srContinue, srFinish, srNeedData } StreamResult;

	static constexpr char *StreamResultStrings[4] = { (char *)"Error:", (char *)"Continue:", (char *)"Finish:", (char *)"NeedData:" };

	static char *GetStreamResultString(StreamResult result)
	{
		return CADCommandHandler::StreamResultStrings[result];
	}

	virtual StreamResult 	streamData(CADDataStream &dataStream) = 0;
	virtual bool 	 				init(uint8_t uSubCommand) = 0;

	uint32_t GetBytesHandled(void)
	{
		return m_uBytesHandled;
	}

protected:
	uint32_t m_uBytesHandled = 0;
};



#endif /* MYSTORM_CADCOMMANDHANDLER_H_ */
