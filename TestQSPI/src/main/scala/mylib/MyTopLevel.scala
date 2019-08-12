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

class MyTopLevel extends Component 
{
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

  }
  
  val ledGlow = new LedGlow(24);
  val qspiSlaveCtrl = new QspiSlaveCtrl(QspiSlaveCtrlGenerics(8))

  val mem = Mem(Bits(8 bits),wordCount = 256)

  qspiSlaveCtrl.io.kind.cpha  := False
  qspiSlaveCtrl.io.kind.cpol  := False
  qspiSlaveCtrl.io.tx.valid   := True;
  qspiSlaveCtrl.io.spi.io0    := io.io0
  qspiSlaveCtrl.io.spi.ss     := io.ss
  qspiSlaveCtrl.io.spi.sclk   := io.sclk
  io.io1                      := qspiSlaveCtrl.io.spi.io1.write;
  

  io.leds(0) := !io.io0;            // blue
  io.leds(1) := !io.io1;   // green
  io.leds(2) := !io.ss;  // yellow
  io.leds(3) := !io.sclk;  // red

  io.dbg_io0  := io.io0
  io.dbg_io1  := io.io1
  io.dbg_ss   := io.ss
  io.dbg_sclk := io.sclk

  //io.dbg_1 := False // qspiSlaveCtrl.io.dbg1;
  //io.dbg_2 := False // qspiSlaveCtrl.io.dbg2;
  io.dbg_3 := False // qspiSlaveCtrl.io.dbg3;
  io.dbg_4 := False // qspiSlaveCtrl.io.dbg4;



  val rxValidEdges = qspiSlaveCtrl.io.rx.valid.edges()
  val txReadyEdges = qspiSlaveCtrl.io.tx.ready.edges()
  
  io.dbg_1 := qspiSlaveCtrl.io.tx.ready
  io.dbg_2 := txReadyEdges.rise


  val sendByte = Reg(Bits(8 bits))
  qspiSlaveCtrl.io.tx.payload := sendByte;

  val fsm = new StateMachine{
    val stateWaitSS   = new State with EntryPoint
    val stateDecode   = new State
    val stateReceive  = new State
    val stateSend     = new State

    val counter = Counter(256)

    stateWaitSS
      .whenIsActive{
        // mem(counter) := counter.asBits;
        // counter.increment()

        when(io.ss === False){
          sendByte := 0
          goto(stateDecode)
        }
      }
    stateDecode
      .whenIsActive{
        when(rxValidEdges.rise)
        {
          when(qspiSlaveCtrl.io.rx.payload === 0x01){
            goto(stateReceive)
          } otherwise when(qspiSlaveCtrl.io.rx.payload === 0x02){
            goto(stateSend)
          }
        }      

        when(io.ss){
          goto(stateWaitSS)
        }
      }

    stateReceive
      .onEntry(counter := 0)
      .whenIsActive{
        when(rxValidEdges.rise)
        {
          mem(counter) := qspiSlaveCtrl.io.rx.payload;
          counter.increment()
        }      

        when(io.ss){
          goto(stateWaitSS)
        }
      }

    stateSend
      .onEntry(counter := 0)
      .whenIsActive{
        io.dbg_3 := True;
        when(txReadyEdges.rise || counter === 0)
        {
          io.dbg_4 := True;
          sendByte := mem(counter)
          counter.increment()
        }      

        when(io.ss){
          goto(stateWaitSS)
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
