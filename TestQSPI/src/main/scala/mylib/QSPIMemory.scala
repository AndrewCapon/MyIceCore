package mylib

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._
import spinal.lib.io.{TriStateOutput, TriStateArray, TriState, InOutWrapper}
import scala.util.Random



class QSPIMemory extends Component{
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
  val memorySize = 1024*8
  val addrBits = log2Up(memorySize)

  val mem = Mem(UInt(16 bits), wordCount = memorySize);
  val addrCounter = Reg(UInt(addrBits bits)) init(0)


  // inout reading/writing
  val reading = Reg(Bool) init(True)
  when(reading){
    io.qd.writeEnable := 0
  } otherwise{
    io.qd.writeEnable := 3
  }


  // in and out
  val sendByte = Reg(UInt(8 bits)).addTag(crossClockDomain) init (0)
  qspiSlaveCtrl.io.txPayload := sendByte.asBits;

  //alternative to SpinalHDL StateMachine which is slow!
  object FSMState extends SpinalEnum {
    val sWait, sDecode, sDecodeAddressMSB, sDecodeAddressLSB, sReceiveSkipDummyByte, sReceiveLSB, sReceiveMSB, sSendSkipDummyByte, sSendLSB, sSendMSB = newElement()
  }

  val stateNext  = Reg(FSMState()) init(FSMState.sWait)
  val skipState  = Reg(FSMState()) init(FSMState.sWait)

  val receiveLSB = Reg(UInt(8 bits)) init(0)
  val sendWord = Reg(UInt(16 bits)) init(0)

  switch(stateNext){
    is(FSMState.sWait){
      reading := True
      sendByte := 0xff // DEBUG
      when(io.ss === False){
        stateNext := FSMState.sDecode
        sendByte := 0
        sendWord := 0
      }
    }

    is(FSMState.sDecode){
      reading := True
      sendByte := 0xff // DEBUG
      when(qspiSlaveCtrl.io.rxReady)
      {
        switch(qspiSlaveCtrl.io.rxPayload ){
          is(0x01){
            skipState := FSMState.sReceiveSkipDummyByte
            stateNext := FSMState.sDecodeAddressMSB
          }
          is(0x02){
            skipState := FSMState.sSendSkipDummyByte
            stateNext := FSMState.sDecodeAddressMSB
          }
          // default{ // Causes issues??
          //   stateNext := FSMState.sWait
          // }
        }
      }
    }
 
    is(FSMState.sDecodeAddressMSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        addrCounter := qspiSlaveCtrl.io.rxPayload.asUInt.resized
        stateNext := FSMState.sDecodeAddressLSB
      }
    }

    is(FSMState.sDecodeAddressLSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        addrCounter := (addrCounter ## qspiSlaveCtrl.io.rxPayload).asUInt.resized
        stateNext := skipState
      }
    }

    is(FSMState.sReceiveSkipDummyByte){
      when(qspiSlaveCtrl.io.rxReady)
      {
        stateNext := FSMState.sReceiveLSB
      } 
    }

    is(FSMState.sSendSkipDummyByte){ // TX is always 1 byte ahead
      reading := False
      sendWord := mem(addrCounter)
      sendByte := sendWord(15 downto 8)
      stateNext := FSMState.sSendMSB
    }

    is(FSMState.sReceiveLSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        receiveLSB := qspiSlaveCtrl.io.rxPayload.asUInt.resized
        stateNext := FSMState.sReceiveMSB
      } elsewhen(io.ss){
        stateNext := FSMState.sWait
      } 
    }

    is(FSMState.sReceiveMSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        val receiveWord = (qspiSlaveCtrl.io.rxPayload ## receiveLSB).asUInt

        mem(addrCounter) := receiveWord
        stateNext := FSMState.sReceiveLSB
        addrCounter := addrCounter +1;
      } elsewhen(io.ss){
        stateNext := FSMState.sWait
      }
    }

    is(FSMState.sSendLSB){
      reading := False
      when(qspiSlaveCtrl.io.txReady)
      {
        sendWord := mem(addrCounter)
        sendByte := sendWord(15 downto 8)
        stateNext := FSMState.sSendMSB
      } elsewhen(io.ss){ 
        stateNext := FSMState.sWait
      }
    }

    is(FSMState.sSendMSB){
      reading := False
      when(qspiSlaveCtrl.io.txReady)
      {
        sendByte := sendWord(7 downto 0)
        addrCounter := addrCounter +1;
        stateNext := FSMState.sSendLSB
      } elsewhen(io.ss){
        stateNext := FSMState.sWait
      }
    }
  }
}


// Debug 16 bit version from here down
class QSPIMemoryDebug extends Component{
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
  val memorySize = 1024*8
  val addrBits = log2Up(memorySize)

  def testData = for(addr <- 0 until (memorySize -1)) yield{
    // if(addr== 0)
    //   U(0x0102)
    // else
    //   U(0x0304)
    U(addr)
  }
  val mem =  Mem(UInt(16 bits),initialContent = testData)
//  val mem = Mem(UInt(16 bits), wordCount = memorySize);
  val addrCounter = Reg(UInt(addrBits bits)) init(0)
 // val addrCounterDbg = Reg(UInt(addrBits bits)) init(0)


  // inout reading/writing
  val reading = Reg(Bool) init(True)
  when(reading){
    io.qd.writeEnable := 0
    io.dbg_io0  := io.qd.read(0)
    io.dbg_io1  := io.qd.read(1)
  } otherwise{
    io.dbg_io0  := io.qd.write(0)
    io.dbg_io1  := io.qd.write(1)
    io.qd.writeEnable := 3
  }


  // in and out
  val sendByte = Reg(UInt(8 bits)).addTag(crossClockDomain) init (0)
  qspiSlaveCtrl.io.txPayload := sendByte.asBits;

  //alternative to SpinalHDL StateMachine which is slow!
  object FSMState extends SpinalEnum {
    val sWait, sDecode, sDecodeAddressMSB, sDecodeAddressLSB, sReceiveSkipDummyByte, sReceiveLSB, sReceiveMSB, sSendSkipDummyByte, sSendLSB, sSendMSB = newElement()
  }

  val stateNext     = Reg(FSMState()) init(FSMState.sWait)
  val skipState  = Reg(FSMState()) init(FSMState.sWait)

  // debug signals
  io.dbg_ss   := io.ss
  io.dbg_sclk := io.sclk
  io.leds := 0x0
  io.dbg_1 := qspiSlaveCtrl.io.rxReady
  io.dbg_2 := qspiSlaveCtrl.io.txReady
  io.dbg_3 := reading
  io.dbg_4 := False
  

  val receiveLSB = Reg(UInt(8 bits)) init(0)
  val sendWord = Reg(UInt(16 bits)) init(0)

  io.dbgByte := sendByte.asBits // stateNext.asBits.resized // qspiSlaveCtrl.io.dbg

  when(sendWord === 0x0001){
    io.dbg_4 := True
  }

  switch(stateNext){
    is(FSMState.sWait){
      reading := True
      sendByte := 0xff // DEBUG
      when(io.ss === False){
        stateNext := FSMState.sDecode
        sendByte := 0
        sendWord := 0
      }
    }

    is(FSMState.sDecode){
      reading := True
      sendByte := 0xff // DEBUG
      when(qspiSlaveCtrl.io.rxReady)
      {
        switch(qspiSlaveCtrl.io.rxPayload ){
          is(0x01){
            skipState := FSMState.sReceiveSkipDummyByte
            stateNext := FSMState.sDecodeAddressMSB
          }
          is(0x02){
            skipState := FSMState.sSendSkipDummyByte
            stateNext := FSMState.sDecodeAddressMSB
          }
          // default{ // Causes issues??
          //   stateNext := FSMState.sWait
          // }
        }
      }
    }
 
    is(FSMState.sDecodeAddressMSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        addrCounter := qspiSlaveCtrl.io.rxPayload.asUInt.resized
        stateNext := FSMState.sDecodeAddressLSB
      }
    }

    is(FSMState.sDecodeAddressLSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        addrCounter := (addrCounter ## qspiSlaveCtrl.io.rxPayload).asUInt.resized
        stateNext := skipState
      }
    }

    is(FSMState.sReceiveSkipDummyByte){
      //reading := False
      when(qspiSlaveCtrl.io.rxReady)
      {
        stateNext := FSMState.sReceiveLSB
      } 
    }

    is(FSMState.sSendSkipDummyByte){ // TX is always 1 byte ahead
      reading := False
      sendWord := mem(addrCounter)
      sendByte := sendWord(15 downto 8)
      stateNext := FSMState.sSendMSB
    }

    is(FSMState.sReceiveLSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        receiveLSB := qspiSlaveCtrl.io.rxPayload.asUInt.resized
        stateNext := FSMState.sReceiveMSB
      } elsewhen(io.ss){
        stateNext := FSMState.sWait
      } 
    }

    is(FSMState.sReceiveMSB){
      reading := True
      when(qspiSlaveCtrl.io.rxReady)
      {
        val receiveWord = (qspiSlaveCtrl.io.rxPayload ## receiveLSB).asUInt

        mem(addrCounter) := receiveWord
        stateNext := FSMState.sReceiveLSB
        addrCounter := addrCounter +1;
      } elsewhen(io.ss){
        stateNext := FSMState.sWait
      }
    }

    is(FSMState.sSendLSB){
      reading := False
      when(qspiSlaveCtrl.io.txReady)
      {
        sendWord := mem(addrCounter)
        sendByte := sendWord(15 downto 8)
        stateNext := FSMState.sSendMSB
      } elsewhen(io.ss){ 
        stateNext := FSMState.sWait
      }
    }

    is(FSMState.sSendMSB){
      reading := False
      when(qspiSlaveCtrl.io.txReady)
      {
        sendByte := sendWord(7 downto 0)
        addrCounter := addrCounter +1;
        stateNext := FSMState.sSendLSB
      } elsewhen(io.ss){
        stateNext := FSMState.sWait
      }
    }
  }
}
