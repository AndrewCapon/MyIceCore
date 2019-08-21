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


class MyTopLevel extends Component{
  val io = new Bundle 
  {
    val qd    = master(TriStateArray(2))
    val ss    = in Bool
    val sclk  = in Bool
  }
  
  // QSPI Slave Control
  val qspiSlaveCtrl = new QspiSlaveCtrlDual()
  qspiSlaveCtrl.io.qdin   := io.qd.read
  io.qd.write             := qspiSlaveCtrl.io.qdout
  qspiSlaveCtrl.io.ss     := io.ss
  qspiSlaveCtrl.io.sclk   := io.sclk
  qspiSlaveCtrl.io.resetn := False

  // RAM
  val mem = Mem(UInt(8 bits), wordCount = 256)

  // inout reading/writing
  val reading = Reg(Bool) init(True)
  when(reading){
    io.qd.writeEnable := 0
  } otherwise{
    io.qd.writeEnable := 3
  }


  // in and out
  val sendByte = Reg(UInt(8 bits)).addTag(crossClockDomain)
  qspiSlaveCtrl.io.txPayload := sendByte.asBits;

  //alternative to SpinalHDL StateMachine which is slow!
  object FSMState extends SpinalEnum {
    val sWait, sDecode, sDecodeReceiveAddress, sDecodeSendAddress, sReceive, sSend = newElement()
  }

  val stateNext = Reg(FSMState()) init(FSMState.sWait)

  val counter = Reg(UInt(8 bits))
  val skipByte = Reg(Bool) init(True)

  reading := True
  val rxReadyRise = qspiSlaveCtrl.io.rxReady.rise()
  val txReadyRise = qspiSlaveCtrl.io.txReady.rise()

  switch(stateNext){
    is(FSMState.sWait){
      when(io.ss){
        stateNext := FSMState.sWait
      } otherwise {
        stateNext := FSMState.sDecode
        sendByte := 0x88
      }
    }

    is(FSMState.sDecode){
      when(rxReadyRise)
      {
        skipByte := True
        switch(qspiSlaveCtrl.io.rxPayload ){
          is(0x01){
            stateNext := FSMState.sDecodeReceiveAddress
          }
          is(0x02){
            stateNext := FSMState.sDecodeSendAddress
          }
          default{
            stateNext := FSMState.sWait
          }
        }
      } otherwise {
        stateNext := FSMState.sDecode
      }
    }

    is(FSMState.sDecodeReceiveAddress){
      when(rxReadyRise)
      {
        counter := qspiSlaveCtrl.io.rxPayload.asUInt;
        stateNext := FSMState.sReceive
      } otherwise{
        stateNext := FSMState.sDecodeReceiveAddress
      }
    }

    is(FSMState.sDecodeSendAddress){
      when(rxReadyRise)
      {
        counter := qspiSlaveCtrl.io.rxPayload.asUInt;
        stateNext := FSMState.sSend
      } otherwise{
        stateNext := FSMState.sDecodeSendAddress
      }
    }

    is(FSMState.sReceive){
      when(rxReadyRise)
      {
        mem(counter) := qspiSlaveCtrl.io.rxPayload.asUInt;
        counter:=counter+1
      }      

      when(io.ss){
        stateNext := FSMState.sWait
      } otherwise {
        stateNext := FSMState.sReceive
      }
    }

    is(FSMState.sSend){
      reading := False
      when(txReadyRise)
      {
        when(skipByte){
          skipByte := False
          sendByte := mem(counter)
        } otherwise{
          sendByte := mem(counter)
          counter := counter+1
        }
      }      

      when(io.ss){
        stateNext := FSMState.sWait
      } otherwise{
        stateNext := FSMState.sSend
      }
    }
  }
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



// Debug version from here down

// class MyTopLevel extends Component{
//   val io = new Bundle 
//   {
//     val leds       = out UInt(4 bits)

//     val qd    = master(TriStateArray(2))
//     val ss    = in Bool
//     val sclk  = in Bool

//     val dbg_io0  = out Bool
//     val dbg_io1  = out Bool
//     val dbg_ss   = out Bool
//     val dbg_sclk = out Bool

//     val dbg_1    = out Bool
//     val dbg_2    = out Bool
//     val dbg_3    = out Bool
//     val dbg_4    = out Bool

//     val dbgByte  = out Bits(8 bits)

//     val dummy_clk = in Bool

    
//   }
  
//   // QSPI Slave Control
//   val qspiSlaveCtrl = new QspiSlaveCtrlDual()
//   qspiSlaveCtrl.io.qdin   := io.qd.read
//   io.qd.write             := qspiSlaveCtrl.io.qdout
//   qspiSlaveCtrl.io.ss     := io.ss
//   qspiSlaveCtrl.io.sclk   := io.sclk
//   qspiSlaveCtrl.io.resetn := False

//   // RAM
//   def testData = for(addr <- 0 until 255) yield{
//      U(0xBA)
//    }
//   val mem =  Mem(UInt(8 bits),initialContent = testData)
  
//   // inout reading/writing
//   val reading = Reg(Bool) init(True)
//   when(reading){
//     io.qd.writeEnable := 0
//   } otherwise{
//     io.qd.writeEnable := 3
//   }


//   // debug signals
//   io.dbg_io0  := io.qd.read(0)
//   io.dbg_io1  := io.qd.read(1)
//   io.dbg_ss   := io.ss
//   io.dbg_sclk := io.sclk
//   io.leds := 0x00
  
 


//   // in and out
//   val sendByte = Reg(UInt(8 bits)).addTag(crossClockDomain)
//   qspiSlaveCtrl.io.txPayload := sendByte.asBits;

//   //alternative to SpinalHDL StateMachine which is slow!
//   object FSMState extends SpinalEnum {
//     val sWait, sDecode, sDecodeReceiveAddress, sDecodeSendAddress, sReceive, sSend = newElement()
//   }

//   val stateNext = Reg(FSMState()) init(FSMState.sWait)

//   val counter = Reg(UInt(8 bits))
//   val skipByte = Reg(Bool) init(True)

//   reading := True
//   val rxReadyRise = qspiSlaveCtrl.io.rxReady.rise()
//   val txReadyRise = qspiSlaveCtrl.io.txReady.rise()

//   io.dbg_1 := rxReadyRise;
//   io.dbg_2 := txReadyRise;
//   when(reading){
//     io.dbg_3 := io.qd.read(0)
//     io.dbg_4 := io.qd.read(1)
//   } otherwise{
//     io.dbg_3 := io.qd.write(0)
//     io.dbg_4 := io.qd.write(1)
//   }
// //  io.dbgByte := sendByte.asBits
//   io.dbgByte := qspiSlaveCtrl.io.rxPayload
// //  io.dbgByte := counter.asBits

//   switch(stateNext){
//     is(FSMState.sWait){
//       when(io.ss){
//         stateNext := FSMState.sWait
//       } otherwise {
//         stateNext := FSMState.sDecode
//         sendByte := 0x88
//       }
//     }

//     is(FSMState.sDecode){
//       when(rxReadyRise)
//       {
//         skipByte := True
//         switch(qspiSlaveCtrl.io.rxPayload ){
//           is(0x01){
//             stateNext := FSMState.sDecodeReceiveAddress
//           }
//           is(0x02){
//             stateNext := FSMState.sDecodeSendAddress
//           }
//           default{
//             stateNext := FSMState.sWait
//           }
//         }
//       } otherwise {
//         stateNext := FSMState.sDecode
//       }
//     }

//     is(FSMState.sDecodeReceiveAddress){
//       when(rxReadyRise)
//       {
//         counter := qspiSlaveCtrl.io.rxPayload.asUInt;
//         stateNext := FSMState.sReceive
//       } otherwise{
//         stateNext := FSMState.sDecodeReceiveAddress
//       }
//     }

//     is(FSMState.sDecodeSendAddress){
//       when(rxReadyRise)
//       {
//         counter := qspiSlaveCtrl.io.rxPayload.asUInt;
//         stateNext := FSMState.sSend
//       } otherwise{
//         stateNext := FSMState.sDecodeSendAddress
//       }
//     }

//     is(FSMState.sReceive){
//       when(rxReadyRise)
//       {
//         mem(counter) := qspiSlaveCtrl.io.rxPayload.asUInt;
//         counter:=counter+1
//       }      

//       when(io.ss){
//         stateNext := FSMState.sWait
//       } otherwise {
//         stateNext := FSMState.sReceive
//       }
//     }

//     is(FSMState.sSend){
//       reading := False
//       when(txReadyRise)
//       {
//         when(skipByte){
//           skipByte := False
//           sendByte := mem(counter)
//         } otherwise{
//           sendByte := mem(counter)
//           counter := counter+1
//         }
//       }      

//       when(io.ss){
//         stateNext := FSMState.sWait
//       } otherwise{
//         stateNext := FSMState.sSend
//       }
//     }
//   }
// }
