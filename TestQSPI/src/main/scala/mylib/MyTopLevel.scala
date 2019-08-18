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
//   val stateWaitSS   = new State with EntryPoint
//   val stateDecode   = new State
//   val stateReceive  = new State
//   val stateSend     = new State

//   val counter = Counter(256)

//   val sendByte = Reg(Bits(8 bits))

//   stateWaitSS
//     .whenIsActive{
//       // mem(counter) := counter.asBits;
//       // counter.increment()
//       when(io.ss === False){
//         sendByte := 0xff
//         goto(stateDecode)
//       }
//     }
//   stateDecode
//     .whenIsActive{

//       when(io.ss){
//         goto(stateWaitSS)
//       }
//     }

//   stateReceive
//     .onEntry(counter := 0)
//     .whenIsActive{

//       when(io.ss){
//         goto(stateWaitSS)
//       }
//     }

//   stateSend
//     .onEntry(counter := 0)
//     .whenIsActive{
//       sendByte := 0
//       counter.increment()

//       when(io.ss){
//         goto(stateWaitSS)
//       }
//     }
// }

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


class MyTopLevel extends Component{
  val io = new Bundle 
  {
    val leds       = out UInt(4 bits)

    val io0   = in Bool
    val io1   = out Bool
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

    val dummy_clk = in Bool

    
  }
  
  val ledGlow = new LedGlow(24);
  val qspiSlaveCtrl = new QspiSlaveCtrl3()

  val mem = Mem(Bits(8 bits),wordCount = 256)

  qspiSlaveCtrl.io.io0    := io.io0
  qspiSlaveCtrl.io.ss     := io.ss
  qspiSlaveCtrl.io.sclk   := io.sclk
  qspiSlaveCtrl.io.resetn := False
  io.io1                  := qspiSlaveCtrl.io.io1
  

  // io.leds(0) := !io.io0;            // blue
  // io.leds(1) := !io.io1;   // green
  // io.leds(2) := !io.ss;  // yellow
  // io.leds(3) := !io.sclk;  // red

  io.dbg_io0  := io.io0
  io.dbg_io1  := io.io1
  io.dbg_ss   := io.ss
  io.dbg_sclk := io.sclk

  io.leds(0) := qspiSlaveCtrl.io.dbg1;
  io.leds(1) := qspiSlaveCtrl.io.dbg2;
  io.leds(2) := qspiSlaveCtrl.io.dbg3;
  io.leds(3) := qspiSlaveCtrl.io.dbg4;



  //val rxValidEdges = qspiSlaveCtrl.io.rx.valid.edges()
  // val rxReadyEdges = qspiSlaveCtrl.io.rxReady.edges()
  // val txReadyEdges = qspiSlaveCtrl.io.txReady.edges()
  
  val rxReadyChange = Reg(Bool) init False
  val txReadyChange = Reg(Bool) init False


  // io.dbg_1 := qspiSlaveCtrl.io.rxReady
  // io.dbg_2 := rxReadyChange
  // io.dbg_3 := qspiSlaveCtrl.io.txReady
  // io.dbg_4 := txReadyChange
  io.dbg_1 := False
  io.dbg_2 := False
  io.dbg_3 := False
  io.dbg_4 := False

  val sendByte = Reg(Bits(8 bits)).addTag(crossClockDomain)
  qspiSlaveCtrl.io.txPayload := sendByte;

  when(qspiSlaveCtrl.io.rxReady.rise()){
    rxReadyChange := ~rxReadyChange
    // when(qspiSlaveCtrl.io.rxPayload === 0x01){
    //   sendByte := 0x01
    // } otherwise when (qspiSlaveCtrl.io.rxPayload === 0x02){
    //   sendByte := 0x02
    // }
  }

  when(qspiSlaveCtrl.io.txReady.rise()){
    txReadyChange := ~txReadyChange
  }  

  // alternative to StateMachine()
  object FSMState extends SpinalEnum {
    val sDecode, sReceive, sSend = newElement()
  }

  val stateNext = Reg(FSMState())
  stateNext := FSMState.sDecode

  val counter = Counter(256)
  val skipByte = Reg(Bool) init(True)

  switch(stateNext){
    is(FSMState.sDecode){
      when(qspiSlaveCtrl.io.rxReady.rise())
      {
        counter := 0
        skipByte := True
  
        when(qspiSlaveCtrl.io.rxPayload === 0x01){
          stateNext := FSMState.sReceive
        } otherwise when(qspiSlaveCtrl.io.rxPayload === 0x02){
          stateNext := FSMState.sSend
        }
      }      
    }

    is(FSMState.sReceive){
      stateNext := FSMState.sReceive
      when(qspiSlaveCtrl.io.rxReady.rise())
      {
        when(skipByte){
          skipByte := False
        } otherwise{
          io.dbg_3 := True
          mem(counter) := qspiSlaveCtrl.io.rxPayload;
          counter.increment()
        }
      }      

      when(io.ss){
        stateNext := FSMState.sDecode
      }
    }

    is(FSMState.sSend){
      stateNext := FSMState.sSend
      when(qspiSlaveCtrl.io.txReady.rise())
      {
        when(skipByte){
          skipByte := False
          sendByte := mem(counter)
        } otherwise{
          io.dbg_4 := True
          sendByte := mem(counter)
          counter.increment()
        }
      }      

      when(io.ss){
        stateNext := FSMState.sDecode
      }
    }
  }


  // val fsm = new StateMachine{
  //   val stateWaitSS   = new State with EntryPoint
  //   val stateDecode   = new State
  //   val stateReceive  = new State
  //   val stateSend     = new State

  //   val counter = Counter(256)
  //   val skipByte = Reg(Bool) init(True)

  //   stateWaitSS
  //     .whenIsActive{
  //       io.dbg_1 := True
  //       sendByte := 0xfc
  //       when(io.ss === False){
  //         sendByte := 0xff
  //         goto(stateDecode)
  //       }
  //     }
  //   stateDecode
  //     .whenIsActive{
  //       io.dbg_2 := True
  //       sendByte := 0xfe
  //       when(qspiSlaveCtrl.io.rxReady.rise())
  //       {
  //         when(qspiSlaveCtrl.io.rxPayload === 0x01){
  //           goto(stateReceive)
  //         } otherwise when(qspiSlaveCtrl.io.rxPayload === 0x02){
  //           goto(stateSend)
  //         }
  //       }      

  //       // when(io.ss){
  //       //   goto(stateWaitSS)
  //       // }
  //     }

  //   stateReceive
  //     .onEntry(counter := 0)
  //     .onEntry(skipByte := True)
  //     .whenIsActive{
  //       sendByte := 0xfd
  //       when(qspiSlaveCtrl.io.rxReady.rise())
  //       {
  //         when(skipByte){
  //           skipByte := False
  //         } otherwise{
  //           io.dbg_3 := True
  //           mem(counter) := qspiSlaveCtrl.io.rxPayload;
  //           counter.increment()
  //         }
  //       }      

  //       when(io.ss){
  //         goto(stateDecode)
  //       }
  //     }

  //   stateSend
  //     .onEntry(counter := 0)
  //     .onEntry(skipByte := True)
  //     .whenIsActive{
  //       when(qspiSlaveCtrl.io.txReady.rise())
  //       {
  //         when(skipByte){
  //           skipByte := False
  //           sendByte := mem(counter)
  //         } otherwise{
  //           io.dbg_4 := True
  //           sendByte := mem(counter)
  //           counter.increment()
  //         }
  //       }      

  //       when(io.ss){
  //         goto(stateDecode)
  //       }
  //     }
  // }
}












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
