package mylib

import spinal.core._
import spinal.lib._
import spinal.lib.{BufferCC, master, IMasterSlave}

case class ClockTestCtrl() extends Component{
  val io = new Bundle {
    val clk  = in Bool  // same as component clk
    val qClk = in Bool  // qspi clock
    val dbg1 = out Bool
    val dbg2 = out Bool
    val dbg3 = out Bool
    val dbg4 = out Bool

    val resetn = in Bool
  }

  val qspiCoreClockDomain = ClockDomain(io.qClk, io.resetn);

  val qspiArea = new ClockingArea(qspiCoreClockDomain){
    // simple test 8 bit counter, flag on full

    val readyFlag = Bool;

    var counter = Counter(8)
    readyFlag := RegNext(counter.willOverflow)
    counter.increment;
  }

  
  val syncedReadyFlag = BufferCC(qspiArea.readyFlag)

  io.dbg1 := io.clk
  io.dbg2 := io.qClk
  io.dbg3 := qspiArea.readyFlag
  io.dbg4 := syncedReadyFlag
}
