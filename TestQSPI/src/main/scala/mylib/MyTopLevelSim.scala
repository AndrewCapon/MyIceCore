package mylib

import spinal.core._
import spinal.sim._
import spinal.core.sim._

import scala.util.Random


//MyTopLevel's testbench
object MyTopLevelSim {

  def toBinary(i: Int, digits: Int = 4) = String.format("%" + digits + "s", i.toBinaryString).replace(' ', '0')

  def main(args: Array[String]) {
    // SimConfig.withWave.doSim(new QSPISlaveDebug){dut =>
    //   // //Fork a process to generate the reset and the clock on the dut
    //   println("Starting Sim (clock 10ns 100 mhz)")
    //   dut.clockDomain.forkStimulus(period = 10)
    //   dut.clockDomain.assertReset()
    //   dut.io.ss #= true
    //   dut.io.sclk #= true

    //   sleep(1000)
    //   dut.clockDomain.deassertReset()

    //   sleep(1000)


    //   for(iters <- 0 to 1000){
    //     dut.io.ss #= false

    //     for(sclk <- 0 to 47){
    //       dut.io.sclk #= false
    //       sleep(200)
    //       dut.io.sclk #= true
    //       sleep(200)
    //     }

    //     if(dut.io.dbgByte.toInt != 0x0b){
    //       println("Test Failed", dut.io.dbgByte.toInt)
    //     }

    //     dut.io.ss #= true
    //     sleep(1000)
    //   }

    //   println("Sim Template Finished")
    // }
  }
}
