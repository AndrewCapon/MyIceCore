###############################################################################
#                                                                             #
# Copyright 2016 myStorm Copyright and related                                #
# rights are licensed under the Solderpad Hardware License, Version 0.51      #
# (the “License”); you may not use this file except in compliance with        #
# the License. You may obtain a copy of the License at                        #
# http://solderpad.org/licenses/SHL-0.51. Unless required by applicable       #
# law or agreed to in writing, software, hardware and materials               #
# distributed under this License is distributed on an “AS IS” BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or             #
# implied. See the License for the specific language governing                #
# permissions and limitations under the License.                              #
#                                                                             #
###############################################################################

TTY  = /dev/cu.usbmodem00000000001A1


chip.bin: hdl/* MyTopLevel.v 
#	yosys -q -p "synth_ice40 -blif chip.blif" hdl/*.v MyTopLevel.v
#	arachne-pnr -m 100 -r -d 8k -P tq144:4k -p hdl/chip.pcf chip.blif -o chip.txt
#	icepack chip.txt chip.bin
	yosys -q -p "synth_ice40 -top chip -json chip.json" hdl/*.v MyTopLevel.v 
	nextpnr-ice40 --package tq144:4k --hx8k --json chip.json --pcf hdl/chip.pcf --asc chip.asc --freq 25
	icepack chip.asc chip.bin                                      


MyTopLevel.v: src/main/scala/mylib/*.scala
	sbt "runMain mylib.MyTopLevelVerilog"

.PHONY: upload
upload:
	stty -f $(TTY) raw
	cat chip.bin > $(TTY)
	sleep 1
	cat flash.bin chip.bin > $(TTY)
	sleep 1
	cat reset.bin > $(TTY)

.PHONY: clean
clean:
	$(RM) -f chip.blif chip.txt chip.ex chip.bin
