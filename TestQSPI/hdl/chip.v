/******************************************************************************
*                                                                             *
* Copyright 2016 myStorm Copyright and related                                *
* rights are licensed under the Solderpad Hardware License, Version 0.51      *
* (the “License”); you may not use this file except in compliance with        *
* the License. You may obtain a copy of the License at                        *
* http://solderpad.org/licenses/SHL-0.51. Unless required by applicable       *
* law or agreed to in writing, software, hardware and materials               *
* distributed under this License is distributed on an “AS IS” BASIS,          *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or             *
* implied. See the License for the specific language governing                *
* permissions and limitations under the License.                              *
*                                                                             *
******************************************************************************/

module frequency_divider_by2 ( clk ,out_clk );
output reg out_clk;
input clk ;
always @(posedge clk)
begin
     out_clk <= ~out_clk;	
end
endmodule

module chip (
    // 25Hz clock input
    input  clk,

    // qspi
    input  io0,
    output io1,
    input  dcs,
    input  dsck,

    // led outputs
    output [3:0] led,

    // gpio
    output dbg_io0,
    output dbg_io1,
    output dbg_ss,
    output dbg_sclk,
    output dbg_1,
    output dbg_2,
    output dbg_3,
    output dbg_4,

  );

  wire clk16;
  wire clk8;
  wire clk4;
  wire locked;

  pll clock_16 (
    .clock_in(clk),
    .clock_out(clk16),
    .locked(locked)
  );
  
  frequency_divider_by2 clock_8(
    .clk(clk16),
    .out_clk(clk8)
  );

  frequency_divider_by2 clock_4(
    .clk(clk8),
    .out_clk(clk4)
  );

  MyTopLevel top_level (
    .io_leds(led),
    .io_io0(io0),
    .io_io1(io1),
    .io_ss(dcs),
    .io_sclk(dsck),

    .io_dbg_io0(dbg_io0),
    .io_dbg_io1(dbg_io1),
    .io_dbg_ss(dbg_ss),
    .io_dbg_sclk(dbg_sclk),

    .io_dbg_1(dbg_1),
    .io_dbg_2(dbg_2),
    .io_dbg_3(dbg_3),
    .io_dbg_4(dbg_4),
    .clk(clk8),
    .reset(0),
  );


endmodule
