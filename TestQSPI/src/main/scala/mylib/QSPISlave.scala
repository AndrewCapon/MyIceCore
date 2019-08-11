package mylib

import spinal.core._
import spinal.lib._
import spinal.lib.{BufferCC, master, IMasterSlave}
import spinal.lib.io.{TriStateOutput, TriStateArray, TriState}


case class QspiSlaveCtrlGenerics(dataWidth : Int = 8){
}

case class QspiKind() extends Bundle {
  val cpol = Bool
  val cpha = Bool
}

case class QspiSlaveCtrlIo(generics : QspiSlaveCtrlGenerics) extends Bundle{
  import generics._
  val kind = in(QspiKind())
  val rx = master Flow(Bits(dataWidth bits))
  val tx = slave Stream(Bits(dataWidth bits))
  val txError = out Bool
  val ssFilted = out Bool
  val spi = master(QspiSlave(generics))
  val dbg1 = out Bool
  val dbg2 = out Bool
  val dbg3 = out Bool
  val dbg4 = out Bool
}

case class QspiSlave(generics : QspiSlaveCtrlGenerics, useSclk : Boolean = true) extends Bundle with IMasterSlave{
  val sclk = if(useSclk)Bool else null
  val io0  = Bool
  val io1  = TriStateOutput(Bool)
  val ss   = Bool

  override def asMaster(): Unit = {
    in(sclk, io0)
    in(ss)
    out(io1)
  }

  def slaveResync() : QspiSlave = {
    val ret = cloneOf(this)
    if(useSclk) ret.sclk := BufferCC(this.sclk)
    ret.ss   := BufferCC(this.ss)
    ret.io0 := BufferCC(this.io0)
    this.io1.write := ret.io1.write
    this.io1.writeEnable := ret.io1.writeEnable
    ret
  }
}

case class QspiSlaveCtrl(generics : QspiSlaveCtrlGenerics) extends Component{
  import generics._

  val io = QspiSlaveCtrlIo(generics)

  //Input filter
  val spi = io.spi.slaveResync()
  val normalizedSclkEdges = (spi.sclk ^ io.kind.cpol ^ io.kind.cpha).edges()
  val shit = spi.sclk ^ io.kind.cpol ^ io.kind.cpha;

  //FSM
  val counter = Counter(dataWidth*2)
  val buffer = Reg(Bits(dataWidth bits))

  // io.dbg1 := False
  // io.dbg2 := False
  // io.dbg3 := False
  // io.dbg4 := False

  when(spi.ss){
    counter.clear()
  } otherwise {
    when(normalizedSclkEdges.rise){
      buffer := (buffer ## spi.io0).resized
    }
    when(normalizedSclkEdges.toggle){
      counter.increment()
    }
  }

  //IO
  io.ssFilted := spi.ss

  io.rx.valid := RegNext(counter.willOverflow)
  io.rx.payload := buffer

  io.tx.ready := counter.willOverflow || spi.ss
  io.txError := io.tx.ready && !io.tx.valid

  val rspBit = io.tx.payload(dataWidth - 1 - (counter >> 1))
  val rspBitSampled = RegNextWhen(rspBit, normalizedSclkEdges.fall)
  spi.io1.writeEnable := !spi.ss
  spi.io1.write := io.kind.cpha ? rspBitSampled | rspBit

  io.dbg1 := io.rx.valid
  io.dbg2 := io.tx.ready
  io.dbg3 := io.txError
  io.dbg4 := rspBit
}

