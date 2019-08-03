#include <string.h>
#include <stdlib.h>

#include "main.h"

extern "C"
{
	extern void cdc_puts(char *s);
	extern void flash_SPI_Enable(void);
	extern void flash_SPI_Disable(void);
}
