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

  wire clk200;
  wire clk100;
  wire clk50;
  wire clk25;
  wire clk12_5;
  wire clk6_25;
  
  wire locked;

  pll clock_200 (
    .clock_in(clk),
    .clock_out(clk200),
    .locked(locked)
  );
  
  frequency_divider_by2 clock_100(
    .clk(clk200),
    .out_clk(clk100)
  );

  frequency_divider_by2 clock_50(
    .clk(clk100),
    .out_clk(clk50)
  );

frequency_divider_by2 clock_25(
    .clk(clk50),
    .out_clk(clk25)
  );

frequency_divider_by2 clock_12_5(
    .clk(clk25),
    .out_clk(clk12_5)
  );

frequency_divider_by2 clock_6_25(
    .clk(clk12_5),
    .out_clk(clk6_25)
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
    .clk(clk6_25),
    .reset(0),
  );


endmodule
