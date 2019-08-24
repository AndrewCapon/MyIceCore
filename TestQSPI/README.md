# Dual QSPI Slave example with 256 byte memory

On the STM side you need QSPI setup like this:

```
static void MX_QUADSPI_Init(void)
{
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 6;
  hqspi.Init.FifoThreshold = 1;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 7;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_8_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_3;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
}
```

Functions to send and receive data:

```
bool sendDoubleQSPI(uint8_t *puBuffer, uint8_t uAddr, uint16_t uLen)
{
  QSPI_CommandTypeDef     sCommand;

  sCommand.InstructionMode   = QSPI_INSTRUCTION_2_LINES;
  sCommand.Instruction       = 0x01;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_2_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  sCommand.AddressMode       = QSPI_ADDRESS_2_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_8_BITS;

  sCommand.Address           = uAddr;
  sCommand.NbData            = uLen;


  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  	return false;

  if(HAL_QSPI_Transmit(&hqspi, puBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  	return false;

  return true;
}

bool receiveDoubleQSPI(uint8_t *puBuffer, uint8_t uAddr, uint16_t uLen)
{
  QSPI_CommandTypeDef     sCommand;

  sCommand.InstructionMode   = QSPI_INSTRUCTION_2_LINES;
  sCommand.Instruction       = 0x02;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_2_LINES;
  sCommand.DummyCycles       = 4;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  sCommand.AddressMode       = QSPI_ADDRESS_2_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_8_BITS;

  sCommand.Address           = uAddr;
  sCommand.NbData            = uLen;


  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  	return false;

  if(HAL_QSPI_Receive(&hqspi, puBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  	return false;

  return true;
}
```

And a simple testbed, place this in loop():

```
  uint8_t txData1[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  uint8_t rxData1[16]	= {0};
  
  if(!sendDoubleQSPI(txData1, 0x00, 16))
    Error_Handler();

  if(!receiveDoubleQSPI(rxData1, 0x00, 16))
    Error_Handler();

  bool bDiff = false;
  for(int i =0; i < 16; i++)
  {
  	if(txData1[i] != rxData1[i])
  		bDiff = true;
  }

  if(bDiff)
    cdc_puts((char *)"*");
  else
    cdc_puts((char *)".");

```


## Requirements

* SBT.

* Verilator.

* Visual Studio Code with the following plugins installed:

  * Verilog HDL/SystemVerilog : https://marketplace.visualstudio.com/items?itemName=mshr-h.VerilogHDL
 
  * Scala (Metals) : https://marketplace.visualstudio.com/items?itemName=scalameta.metals

  * Scala Syntax (official) : https://marketplace.visualstudio.com/items?itemName=scala-lang.scala



## Installation

Clone this repository or download the zip.

Open the folder in Visual Studio Code and when asked import the build system.

After the import is completed you need to update the TTY variable in the Makefile to point to the correct device for your IceStorm.


## Visual Studio Code Tasks

There are three tasks defined:

Build Task  : This will run the SBT build if necessary and then build the bitstream

Test Task   : This run the SBT simulation

Upload Task : Thus will update the bitstream to the IceCore


In Visual studio there is already a keyboard shortcut for build, you can edit the keyBindings.json file to add shortcuts for Test and Upload as follows:

```
// Place your key bindings in this file to override the defaultsauto[]
[
  {
    "key": "shift+cmd+r",
    "command": "workbench.action.tasks.test"
  },
  {
    "key": "shift+cmd+u",
    "command": "workbench.action.tasks.runTask",
    "args": "upload"
  }
]
```


## Build System

The Makefile will use any verilog (*.v) files in the hdl folder, the top level file is chip.v and the contraints are in chip.pcf.

Spinal HDL files are stored in src/main/scala/mylib. The top level file is MyTopLevel.scala.




## Info
This template is based on https://github.com/SpinalHDL/SpinalTemplateSbt

