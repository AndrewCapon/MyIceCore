/*
 * CADFpgaHandler.h
 *
 *  Created on: 2 Aug 2019
 *      Author: andrewcapon
 */

#ifndef MYSTORM_CADFPGAHANDLER_H_
#define MYSTORM_CADFPGAHANDLER_H_

#include "CAD.h"

#include "CADCommandHandler.h"

class CADFpgaHandler: public CADCommandHandler
{
public:
	CADFpgaHandler(uint32_t img_size);
	uint8_t reset(uint8_t bit_src);
	uint8_t config(void);
	uint8_t write(uint8_t *p, uint32_t len);
	uint8_t stream(uint8_t *data, uint32_t len);

	bool write(CADDataStream &dataStream);

	virtual StreamResult streamData(CADDataStream &dataStream);
	virtual bool init(uint8_t uSubCommand);

#ifdef TIMINGS
		inline bool streamDataInlined(uint8_t *data, uint32_t len)
		{
			bool bResult = true;

			nbytes += len;
			write(data, len);

			if(nbytes >= NBYTES)
			{
				if(err = config())
					status_led_high();
				else
					status_led_low();
				flash_SPI_Enable();
				bResult = false;
			}

			return bResult;
		}
#endif

private:
	uint32_t m_uImageSize;
	uint8_t sig[4] = { 0x7E, 0xAA, 0x99, 0x7E };

};

#endif /* MYSTORM_CADFPGAHANDLER_H_ */
