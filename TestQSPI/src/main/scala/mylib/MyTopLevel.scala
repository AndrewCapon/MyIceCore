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

//Hardware definition

// class MyTopLevel extends Component{
//   val io = new Bundle 
//   {
//     val leds       = out UInt(4 bits)

//     val io0   = in Bool
//     val io1   = out Bool
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

//     val dummy_clk = in Bool
//   }


//   val clockTestCtrl = new ClockTestCtrl();
//   clockTestCtrl.io.clk  := io.dummy_clk;
//   clockTestCtrl.io.qClk := io.sclk;
//   clockTestCtrl.io.resetn := False;

//   val ledGlow = new LedGlow(24);

//   val fsm = new StateMachine{
//     val stateWaitSS   = new State with EntryPoint
//     val stateDecode   = new State
//     val stateReceive  = new State
//     val stateSend     = new State

//     val counter = Counter(256)

//     val sendByte = Reg(Bits(8 bits))

//     stateWaitSS
//       .whenIsActive{
//         // mem(counter) := counter.asBits;
//         // counter.increment()
//         when(io.ss === False){
//           sendByte := 0xff
//           goto(stateDecode)
//         }
//       }
//     stateDecode
//       .whenIsActive{

//         when(io.ss){
//           goto(stateWaitSS)
//         }
//       }

//     stateReceive
//       .onEntry(counter := 0)
//       .whenIsActive{

//         when(io.ss){
//           goto(stateWaitSS)
//         }
//       }

//     stateSend
//       .onEntry(counter := 0)
//       .whenIsActive{
//         sendByte := 0
//         counter.increment()

//         when(io.ss){
//           goto(stateWaitSS)
//         }
//       }
//   }

//   io.leds(0) := ledGlow.io.led;
//   io.leds(1) := False
//   io.leds(2) := False
//   io.leds(3) := False

//   io.dbg_io0  := False;
//   io.dbg_io1  := False;
//   io.dbg_ss   := False;
//   io.dbg_sclk := False;

//   io.dbg_1 := clockTestCtrl.io.dbg1;
//   io.dbg_2 := clockTestCtrl.io.dbg2;
//   io.dbg_3 := clockTestCtrl.io.dbg3;
//   io.dbg_4 := clockTestCtrl.io.dbg4;

//   io.io1 := False;
// }

// FSM Speed test

// Proper One

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
  
  // QSPI Slave Control
  val qspiSlaveCtrl = new QspiSlaveCtrlDual()
  qspiSlaveCtrl.io.qdin   := io.qd.read
  io.qd.write             := qspiSlaveCtrl.io.qdout
  qspiSlaveCtrl.io.ss     := io.ss
  qspiSlaveCtrl.io.sclk   := io.sclk
  qspiSlaveCtrl.io.resetn := False

  // RAM
  def testData = for(addr <- 0 until 255) yield{
     U(0xBA)
   }
  val mem =  Mem(UInt(8 bits),initialContent = testData)
  
  // inout reading/writing
  val reading = Reg(Bool) init(True)
  when(reading){
    io.qd.writeEnable := 0
  } otherwise{
    io.qd.writeEnable := 3
  }


  // debug signals
  io.dbg_io0  := io.qd.read(0)
  io.dbg_io1  := io.qd.read(1)
  io.dbg_ss   := io.ss
  io.dbg_sclk := io.sclk
  io.leds := 0x00
  

  


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

  io.dbg_1 := rxReadyRise;
  io.dbg_2 := txReadyRise;
  when(reading){
    io.dbg_3 := io.qd.read(0)
    io.dbg_4 := io.qd.read(1)
  } otherwise{
    io.dbg_3 := io.qd.write(0)
    io.dbg_4 := io.qd.write(1)
  }
//  io.dbgByte := sendByte.asBits
  io.dbgByte := qspiSlaveCtrl.io.rxPayload
//  io.dbgByte := counter.asBits

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
        // when(skipByte){
        //   skipByte := False
        // } otherwise{
          mem(counter) := qspiSlaveCtrl.io.rxPayload.asUInt;
          counter:=counter+1
        // }
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




// working single version
// val qspiSlaveCtrl = new QspiSlaveCtrl()

//    def testData = for(addr <- 0 until 255) yield{
//      U(0xBA)
//    }
//   val mem =  Mem(UInt(8 bits),initialContent = testData)
  
//   qspiSlaveCtrl.io.io0    := io.io0
//   qspiSlaveCtrl.io.ss     := io.ss
//   qspiSlaveCtrl.io.sclk   := io.sclk
//   qspiSlaveCtrl.io.resetn := False
//   io.io1                  := qspiSlaveCtrl.io.io1
  

//   io.dbg_io0  := io.io0
//   io.dbg_io1  := io.io1
//   io.dbg_ss   := io.ss
//   io.dbg_sclk := io.sclk

//   io.leds := 0x0f;

  

//   io.dbg_1 := False
//   io.dbg_2 := False
//   io.dbg_3 := False
//   io.dbg_4 := False

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
//       when(qspiSlaveCtrl.io.rxReady.rise())
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
//       when(qspiSlaveCtrl.io.rxReady.rise())
//       {
//         counter := qspiSlaveCtrl.io.rxPayload.asUInt;
//         stateNext := FSMState.sReceive
//       } otherwise{
//         stateNext := FSMState.sDecodeReceiveAddress
//       }
//     }

//     is(FSMState.sDecodeSendAddress){
//       when(qspiSlaveCtrl.io.rxReady.rise())
//       {
//         counter := qspiSlaveCtrl.io.rxPayload.asUInt;
//         stateNext := FSMState.sSend
//       } otherwise{
//         stateNext := FSMState.sDecodeSendAddress
//       }
//     }

//     is(FSMState.sReceive){
//       when(qspiSlaveCtrl.io.rxReady.rise())
//       {
//         when(skipByte){
//           skipByte := False
//         } otherwise{
//           mem(counter) := qspiSlaveCtrl.io.rxPayload.asUInt;
//           counter:=counter+1
//         }
//       }      

//       when(io.ss){
//         stateNext := FSMState.sWait
//       } otherwise {
//         stateNext := FSMState.sReceive
//       }
//     }

//     is(FSMState.sSend){
//       when(qspiSlaveCtrl.io.txReady.rise())
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

   

//   // val fsm = new StateMachine{
//   //   val stateWait                   = new State with EntryPoint
//   //   val stateDecode                 = new State
//   //   val stateDecodeReceiveAddress   = new State
//   //   val stateDecodeSendAddress      = new State
//   //   val stateReceive                = new State
//   //   val stateSend                   = new State

//   //   val counter = Counter(256)
//   //   val skipByte = Reg(Bool) init(True)

//   //   stateWait
//   //   .whenIsActive{
//   //     when(!io.ss){
//   //       sendByte := 0x88
//   //       goto(stateDecode)
//   //     }
//   //   }

//   //   stateDecode
//   //     .whenIsActive{
//   //       when(qspiSlaveCtrl.io.rxReady.rise())
//   //       {
//   //         skipByte := True
  
//   //         switch(qspiSlaveCtrl.io.rxPayload ){
//   //           is(0x01){
//   //             goto(stateDecodeReceiveAddress)
//   //           }
//   //           is(0x02){
//   //             goto(stateDecodeSendAddress)
//   //           }
//   //           default{
//   //             goto(stateWait)
//   //           }
//   //         }
//   //       }
//   //     }

//   //   stateDecodeReceiveAddress
//   //     .whenIsActive{
//   //       when(qspiSlaveCtrl.io.rxReady.rise())
//   //       {
//   //         counter := qspiSlaveCtrl.io.rxPayload.asUInt;
//   //         goto(stateReceive)
//   //       }
//   //     }

//   //    stateDecodeSendAddress
//   //     .whenIsActive{
//   //       when(qspiSlaveCtrl.io.rxReady.rise())
//   //       {
//   //         counter := qspiSlaveCtrl.io.rxPayload.asUInt;
//   //         goto(stateSend)
//   //       }
//   //     }

//   //   stateReceive
//   //     .whenIsActive{
//   //       when(qspiSlaveCtrl.io.rxReady.rise())
//   //       {
//   //         when(skipByte){
//   //           skipByte := False
//   //         } otherwise{
//   //           mem(counter) := qspiSlaveCtrl.io.rxPayload.asUInt;
//   //           counter.increment()
//   //         }
//   //       }      
  
//   //       when(io.ss){
//   //         goto(stateDecode)
//   //       }
//   //     }

//   //   stateSend
//   //     .whenIsActive{
//   //       when(qspiSlaveCtrl.io.txReady.rise())
//   //       {
//   //         when(skipByte){
//   //           skipByte := False
//   //           sendByte := mem(counter)
//   //         } otherwise{
//   //           sendByte := mem(counter)
//   //           counter.increment()
//   //         }
//   //       }      
  
//   //       when(io.ss){
//   //         goto(stateDecode)
//   //       }
//   //     }
//   // }


// }








// class MyTopLevel extends Component{
//   val io = new Bundle 
//   {
//     val leds       = out UInt(4 bits)

//     val io0   = in Bool
//     val io1   = out Bool
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

//     val dummy_clk = in Bool

    
//   }
  
//   val ledGlow = new LedGlow(24);
//   val qspiSlaveCtrl = new QspiSlaveCtrl(QspiSlaveCtrlGenerics(8))

//   val mem = Mem(Bits(8 bits),wordCount = 256)

//   qspiSlaveCtrl.io.kind.cpha  := True
//   qspiSlaveCtrl.io.kind.cpol  := True
//   qspiSlaveCtrl.io.tx.valid   := True;
//   qspiSlaveCtrl.io.spi.io0    := io.io0
//   qspiSlaveCtrl.io.spi.ss     := io.ss
//   qspiSlaveCtrl.io.spi.sclk   := io.sclk
//   qspiSlaveCtrl.io.resetn     := False
//   io.io1                      := qspiSlaveCtrl.io.spi.io1.write;
  

//   io.leds(0) := True;            // blue
//   io.leds(1) := False;   // green
//   io.leds(2) := True;  // yellow
//   io.leds(3) := True;  // red

//   io.dbg_io0  := io.io0
//   io.dbg_io1  := io.io1
//   io.dbg_ss   := io.ss
//   io.dbg_sclk := io.sclk

//   io.dbg_1 := qspiSlaveCtrl.io.dbg1;
//   io.dbg_2 := qspiSlaveCtrl.io.dbg2;
//   io.dbg_3 := qspiSlaveCtrl.io.dbg3;
//   io.dbg_4 := qspiSlaveCtrl.io.dbg4;



//   val rxValidEdges = qspiSlaveCtrl.io.rx.valid.edges()
//   val txReadyEdges = qspiSlaveCtrl.io.tx.ready.edges()
  
//   //io.dbg_1 := qspiSlaveCtrl.io.tx.ready
//   //io.dbg_2 := txReadyEdges.rise



//   val sendByte = Reg(Bits(8 bits))
//   qspiSlaveCtrl.io.tx.payload := sendByte;

//   val fsm = new StateMachine{
//     val stateWaitSS   = new State with EntryPoint
//     val stateDecode   = new State
//     val stateReceive  = new State
//     val stateSend     = new State

//     val counter = Counter(256)


//     stateWaitSS
//       .whenIsActive{
//         // mem(counter) := counter.asBits;
//         // counter.increment()
//         when(io.ss === False){
//           sendByte := 0xff
//           goto(stateDecode)
//         }
//       }
//     stateDecode
//       .whenIsActive{
//         when(rxValidEdges.rise)
//         {
//           when(qspiSlaveCtrl.io.rx.payload === 0x01){
//             sendByte := 0x01
//             goto(stateReceive)
//           } otherwise{
//               when(qspiSlaveCtrl.io.rx.payload === 0x02){
//                 sendByte := 0x02
//                 goto(stateSend)
//               } otherwise {
//                 sendByte := 0x03
//               }
//           }
//         } otherwise {
//           sendByte := 0x04
//         }     

//         // when(io.ss){
//         //   goto(stateWaitSS)
//         // }
//       }

//     stateReceive
//       .onEntry(counter := 0)
//       .whenIsActive{
//         when(rxValidEdges.rise)
//         {
//           mem(counter) := qspiSlaveCtrl.io.rx.payload;
//           counter.increment()
//         }      

//         when(io.ss){
//           goto(stateDecode)
//         }
//       }

//     stateSend
//       .onEntry(counter := 0)
//       .whenIsActive{
//         when(txReadyEdges.rise || counter === 0)
//         {
//           sendByte := mem(counter)
//           counter.increment()
//         }      

//         when(io.ss){
//           goto(stateDecode)
//         }
//       }
//   }

// }
 
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
