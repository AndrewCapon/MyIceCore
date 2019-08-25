package mylib

import spinal.core._
import spinal.lib._
import spinal.lib.{BufferCC, master, IMasterSlave}
import spinal.lib.io.{TriStateOutput, TriStateArray, TriState}
import spinal.lib.fsm._


// case class QspiSlaveCtrlDual16() extends Component{
//   val io = new Bundle {
//     val sclk = in Bool
//     val qdin = in Bits(2 bits)
//     val qdout = out Bits(2 bits)
//     val ss   = in Bool
//     val resetn = in Bool

//     val txPayload = in Bits(16 bits)
//     val rxPayload = out Bits(16 bits)

//     val rxReady = out Bool
//     val txReady = out Bool

//   }

//   val qspiRxCoreClockDomain = ClockDomain(io.sclk, io.ss);
//   val qspiRxArea = new ClockingArea(qspiRxCoreClockDomain){
//     val readyFlag = Reg(Bool) init False
//     val halfReadyFlag = Reg(Bool) init False
//     val counter = Reg(UInt(3 bits)) init 0
//     val buffer = Reg(Bits(16 bits)).addTag(crossClockDomain) init 0
//     // shift into buffer
//     buffer := (buffer ## io.qdin(1) ## io.qdin(0)).resized

//     // increment bit counter
//     counter := counter +1

//     // readyflag
//     readyFlag := counter === 7

//     // assignments
//     io.rxPayload := buffer
//   }
  

//   val qspiTxClockDomainConfig = ClockDomainConfig(clockEdge = FALLING)
//   val qspiTxCoreClockDomain = ClockDomain(io.sclk, io.ss, config = qspiTxClockDomainConfig)
//   val qspiTxArea = new ClockingArea(qspiTxCoreClockDomain){
//     val readyFlag = Reg(Bool) init False
//     val counter = Reg(UInt(3 bits)) init(0)
//     val rspBit0 = Reg(Bool) init False
//     val rspBit1 = Reg(Bool) init False

//   // write out to qd
//     rspBit1 := io.txPayload(15 - (counter<<1))
//     rspBit0 := io.txPayload(14 - (counter<<1))

//     // increment bit counter
//     counter := counter + 1

//     // readyflag
//     readyFlag := counter === 7

//     // assignments
//     io.qdout(0) := rspBit0
//     io.qdout(1) := rspBit1
//   }

//   // Fabric Clock Domain
//   val syncedRxReadyFlag = BufferCC(qspiRxArea.readyFlag)
//   val syncedTxReadyFlag = BufferCC(qspiTxArea.readyFlag)

//   // asignments
//   io.rxReady := syncedRxReadyFlag
//   io.txReady := syncedTxReadyFlag
// }



case class QspiSlaveCtrlDual() extends Component{
  val io = new Bundle {
    val sclk = in Bool
    val qdin = in Bits(2 bits)
    val qdout = out Bits(2 bits)
    val ss   = in Bool
    val resetn = in Bool

    val txPayload = in Bits(8 bits)
    val rxPayload = out Bits(8 bits)

    val rxReady = out Bool
    val txReady = out Bool
  }

  val qspiRxCoreClockDomain = ClockDomain(io.sclk, io.ss);
  val qspiRxArea = new ClockingArea(qspiRxCoreClockDomain){
    val readyFlag = Reg(Bool) init False
    val counter = Reg(UInt(2 bits)) init 0
    val buffer = Reg(Bits(8 bits)).addTag(crossClockDomain) init 0

    // shift into buffer
    buffer := (buffer ## io.qdin(1) ## io.qdin(0)).resized

    // increment bit counter
    counter := counter +1

    // readyflag
    readyFlag := counter === 3

    // assignments
    io.rxPayload := buffer
  }
  

  val qspiTxClockDomainConfig = ClockDomainConfig(clockEdge = FALLING)
  val qspiTxCoreClockDomain = ClockDomain(io.sclk, io.ss, config = qspiTxClockDomainConfig)
  val qspiTxArea = new ClockingArea(qspiTxCoreClockDomain){
    val readyFlag = Reg(Bool) init False
    val counter = Reg(UInt(2 bits)) init(0)
    val rspBit0 = Reg(Bool) init False
    val rspBit1 = Reg(Bool) init False
    
    // write out to qd
    rspBit1 := io.txPayload(7 - (counter<<1))
    rspBit0 := io.txPayload(6 - (counter<<1))

    // increment bit counter
    counter := counter +1

    // readyflag
    readyFlag := counter === 3

    // assignments
    io.qdout(0) := rspBit0
    io.qdout(1) := rspBit1
  }

  // Fabric Clock Domain
  val syncedRxReadyFlag = BufferCC(qspiRxArea.readyFlag)
  val syncedTxReadyFlag = BufferCC(qspiTxArea.readyFlag)

  // asignments
  io.rxReady := syncedRxReadyFlag
  io.txReady := syncedTxReadyFlag

}





case class QspiSlaveCtrl() extends Component{
  val io = new Bundle {
    val sclk = in Bool
    val io0  = in Bool
    val io1  = out Bool
    val ss   = in Bool
    val resetn = in Bool

    val txPayload = in Bits(8 bits)
    val rxPayload = out Bits(8 bits)

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
  val counter = Reg(UInt(3 bits)) init(0)
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

// Fabric Clock Domain
val syncedRxReadyFlag = BufferCC(qspiRxArea.readyFlag)
val syncedTxReadyFlag = BufferCC(qspiTxArea.readyFlag)

// asignments
io.rxReady := syncedRxReadyFlag
io.txReady := syncedTxReadyFlag
}

