/*
 * SpinalHDL
 * Copyright (c) Dolu, All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

package mylib

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._
import spinal.lib.io.{TriStateOutput, TriStateArray, TriState, InOutWrapper}
import scala.util.Random


// class MyTopLevel extends Component{
//   val io = new Bundle 
//   {
//     val qd    = master(TriStateArray(2))
//     val ss    = in Bool
//     val sclk  = in Bool
//   }

//   val memoryCtrl = new QSPIMemory()
//   memoryCtrl.io <> io
// }


// DEBUG Version
class MyTopLevel extends Component{
  val io = new Bundle 
  {
    val leds       = out UInt(4 bits)

    val qd    = master(TriStateArray(2))
    val ss    = in Bool
    val sclk  = in Bool

    val dbg_io0  = out Bool
    val dbg_io1  = out Bool
    val dbg_ss   = out Bool
    val dbg_sclk = out Bool

    val dbg_1    = out Bool
    val dbg_2    = out Bool
    val dbg_3    = out Bool
    val dbg_4    = out Bool

    val dbgByte  = out Bits(8 bits)

    val dummy_clk = in Bool
  }

  val memoryCtrl = new QSPIMemoryDebug()
  memoryCtrl.io <> io
}




//Define a custom SpinalHDL configuration with synchronous reset instead of the default asynchronous one. This configuration can be resued everywhere
object MySpinalConfig extends SpinalConfig(defaultConfigForClockDomains = ClockDomainConfig(resetKind = SYNC))

//Generate the MyTopLevel's Verilog using the above custom configuration.
object MyTopLevelVerilog 
{
  def main(args: Array[String]) 
  {
    MySpinalConfig.generateVerilog(new MyTopLevel)
  }
}






