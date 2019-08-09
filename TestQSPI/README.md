# Spinal HDL Template project for myStorm IceCore and Visual Studio Code

## Requirements

* SBT.

* Verilator.

* Visual Studio Code with the following plugins installed:

  * Verilog HDL/SystemVerilog : https://marketplace.visualstudio.com/items?itemName=mshr-h.VerilogHDL
 
  * Scala (Metals) : https://marketplace.visualstudio.com/items?itemName=scalameta.metals

  * Scala Syntax (official) : https://marketplace.visualstudio.com/items?itemName=scala-lang.scala



## Installation

Clone this repository or download the zip.

Open the folder in Visual Studio Code and when asked import the build system.

After the import is completed you need to update the TTY variable in the Makefile to point to the correct device for your IceStorm.


## Visual Studio Code Tasks

There are three tasks defined:

Build Task  : This will run the SBT build if necessary and then build the bitstream

Test Task   : This run the SBT simulation

Upload Task : Thus will update the bitstream to the IceCore


In Visual studio there is already a keyboard shortcut for build, you can edit the keyBindings.json file to add shortcuts for Test and Upload as follows:

```
// Place your key bindings in this file to override the defaultsauto[]
[
  {
    "key": "shift+cmd+r",
    "command": "workbench.action.tasks.test"
  },
  {
    "key": "shift+cmd+u",
    "command": "workbench.action.tasks.runTask",
    "args": "upload"
  }
]
```


## Build System

The Makefile will use any verilog (*.v) files in the hdl folder, the top level file is chip.v and the contraints are in chip.pcf.

Spinal HDL files are stored in src/main/scala/mylib. The top level file is MyTopLevel.scala.




## Info
This template is based on https://github.com/SpinalHDL/SpinalTemplateSbt

