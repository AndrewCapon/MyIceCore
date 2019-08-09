package mylib

import spinal.core._
import spinal.sim._
import spinal.core.sim._

import scala.util.Random


//MyTopLevel's testbench
object MyTopLevelSim {

  def toBinary(i: Int, digits: Int = 4) = String.format("%" + digits + "s", i.toBinaryString).replace(' ', '0')

  def main(args: Array[String]) {
    SimConfig.withWave.doSim(new MyTopLevel){dut =>
      //Fork a process to generate the reset and the clock on the dut
      println("Starting Sim (clock 40ns)")
      dut.clockDomain.forkStimulus(period = 40)

      for(i <- 0 to 999)
      {
        sleep(25000)
        println("leds = " + toBinary(dut.io.leds.toInt))
      }

      println("Sim Template Finished")
    }
  }
}
