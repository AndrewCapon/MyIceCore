package mylib

import spinal.core._
import spinal.lib._
import spinal.lib.{BufferCC, master, IMasterSlave}
import spinal.lib.io.{TriStateOutput, TriStateArray, TriState}
import spinal.lib.fsm._

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
  val resetn = in Bool
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

case class QspiSlaveCtrl3() extends Component{
  val io = new Bundle {
    val sclk = in Bool
    val io0  = in Bool
    val io1  = out Bool
    val ss   = in Bool
    val resetn = in Bool

    val txPayload = in Bits(8 bits)
    val rxPayload = out Bits(8 bits)

    val dbg1    = out Bool
    val dbg2    = out Bool
    val dbg3    = out Bool
    val dbg4    = out Bool


    val rxReady = out Bool
    val txReady = out Bool
  }

  val qspiRxCoreClockDomain = ClockDomain(io.sclk, io.ss);
  val qspiRxArea = new ClockingArea(qspiRxCoreClockDomain){
    val readyFlag = Reg(Bool) init False
    val counter = Reg(UInt(3 bits)) init 0
    val buffer = Reg(Bits(8 bits)).addTag(crossClockDomain) init 0
    val rspBit = Reg(Bool)  init False


    // shift into buffer
    buffer := (buffer ## io.io0).resized

    // increment bit counter
    counter := counter +1

    // readyflag
    readyFlag := counter === 7

    // assignments
    io.rxPayload := buffer

  }

val qspiTxClockDomainConfig = ClockDomainConfig(clockEdge = FALLING)
val qspiTxCoreClockDomain = ClockDomain(io.sclk, io.ss, config = qspiTxClockDomainConfig)
val qspiTxArea = new ClockingArea(qspiTxCoreClockDomain){
  val readyFlag = Reg(Bool) init False
  val counter = Reg(UInt(3 bits)) init 0
  val buffer = Reg(Bits(8 bits)).addTag(crossClockDomain) init 0
  val rspBit = Reg(Bool) init False


  // write out to io1
  rspBit := io.txPayload(7 - counter)

  // increment bit counter
  counter := counter +1

  // readyflag
  readyFlag := counter === 0x00

  // assignments
  io.io1 := rspBit
}

io.dbg1 := False;
io.dbg2 := False;
io.dbg3 := False;
io.dbg4 := False;

// Fabric Clock Domain
val syncedRxReadyFlag = BufferCC(qspiRxArea.readyFlag)
val syncedTxReadyFlag = BufferCC(qspiTxArea.readyFlag)

// asignments
io.rxReady := syncedRxReadyFlag
io.txReady := syncedTxReadyFlag



}


// case class QspiSlaveCtrl2(generics : QspiSlaveCtrlGenerics) extends Component{
//   import generics._

//   val io = QspiSlaveCtrlIo(generics)

//   // SPI Clock Domain
//   val qspiCoreClockDomain = ClockDomain(io.spi.sclk, io.resetn);

//   val qspiArea = new ClockingArea(qspiCoreClockDomain){
//     val spi = io.spi.slaveResync();

//     val readyFlag = Bool;
//     val io1 = Bool;
//     val counter = Counter(dataWidth*2)
//     val buffer = Reg(Bits(dataWidth bits))
//     val rspBit = Reg(Bool);
//     // simple state machine for shifting bits into buffer and out of tx.payload
//     when(spi.ss){
//       counter.clear();
//     } otherwise {
//       // shift into buffer
//       buffer := (buffer ## spi.io0).resized

//       // write out to io1
//       rspBit := io.tx.payload(dataWidth - 1 - (counter >> 1))
  
//       // increment bit counter
//       counter.increment();
//     }

//     // assignments
//     readyFlag := RegNext(counter.willOverflow)
//     spi.io1.write := rspBit
//     spi.io1.writeEnable := !spi.ss
// }

  
//   // Fabric Clock Domain
//   val syncedReadyFlag = BufferCC(qspiArea.readyFlag)


//   // val byteFsm = new StateMachine{
//   //   val stateReady   = new State with EntryPoint
//   //   val stateWaiting = new State

//   //   stateReady
//   //     .whenIsActive{
//   //       // byte in and out
//   //       io.rx.valid := True
//   //       io.tx.valid := True
//   //       goto(stateWaiting)
//   //     }

//   //   stateWaiting
//   //     .whenIsActive{
//   //       io.rx.valid := False
//   //       io.tx.valid := False
//   //       when(!syncedReadyFlag){
//   //         goto(stateReady)
//   //       }
//   //     }
//   // }

//   // asignments
//   io.rx.valid := syncedReadyFlag
//   io.rx.payload := qspiArea.buffer

//   io.tx.ready := syncedReadyFlag
//   io.txError := io.tx.ready && !io.tx.valid

//   io.dbg1 := io.rx.valid
//   io.dbg2 := io.tx.ready
//   io.dbg3 := io.txError
//   io.dbg4 := False

// }

case class QspiSlaveCtrl(generics : QspiSlaveCtrlGenerics) extends Component{
  import generics._

  val io = QspiSlaveCtrlIo(generics)

  //Input filter
  val spi = io.spi.slaveResync()
  val normalizedSclkEdges = (spi.sclk ^ io.kind.cpol ^ io.kind.cpha).edges()

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

