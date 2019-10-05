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
 
`define DEBUG
`ifdef DEBUG
//debug version here down
module chip (
    // 25Hz clock input
    input  clk,

    inout  [1:0] qd,
    input  dcs,
    input  dsck,

    // led outputs
    output [3:0] led,

    // gpio
    output prb00,
    output prb01,
    output prb02,
    output prb03,
    output prb04,
    output prb05,
    output prb06,
    output prb07,
    output prb08,
    output prb09,
    output prb10,
    output prb11,
    output prb12,
    output prb13,
    output prb14,
    output prb15,
  );

  wire clk275;
  wire clk200;
  wire clk100;
  wire clk80;
  wire clk95;
  wire locked;
  
  reg rst;

  reg [12:0] resetCounter;
  always @ (posedge clk) begin
    if(resetCounter < 1024) begin
      rst <= 1;
      resetCounter <= resetCounter+1;
    end else begin
      rst <= 0;
    end
  end 
 
  // pll80 clock_80 (
  //   .clock_in(clk),
  //   .clock_out(clk80),
  //   .locked(locked)
  // );
 
  // pll95 clock_95 (
  //   .clock_in(clk),
  //   .clock_out(clk95),
  //   .locked(locked)
  // );

 
  pll200 clock_200 (
    .clock_in(clk),
    .clock_out(clk200),
    .locked(locked)
  );
  
  frequency_divider_by2 clock_100(
    .clk(clk200),
    .out_clk(clk100)
  ); 
       
  wire useClk = clk;


  wire [1:0] io_qd_read, io_qd_write, io_qd_writeEnable;

  SB_IO #(
    .PIN_TYPE(6'b 1010_01),
    .PULLUP(1'b0)
  ) qd1 [1:0] (
    .PACKAGE_PIN(qd),
    .OUTPUT_ENABLE(io_qd_writeEnable),
    .D_OUT_0(io_qd_write),
    .D_IN_0(io_qd_read)
  );

  MyTopLevel top_level (
    .io_leds(led),
    .io_qd_read(io_qd_read),
    .io_qd_write(io_qd_write),
    .io_qd_writeEnable(io_qd_writeEnable),
    .io_ss(dcs),
    .io_sclk(dsck),

    .io_dbg_1(prb00),
    .io_dbg_2(prb01),
    .io_dbg_3(prb02),
    .io_dbg_4(prb03),

    .io_dbg_io0(prb04),
    .io_dbg_io1(prb05),
    .io_dbg_ss(prb06),
    .io_dbg_sclk(prb07),

    .io_dbgByte({prb08, prb09, prb10, prb11, prb12, prb13, prb14, prb15}),

    .clk(useClk),
    .io_dummy_clk(useClk),

    .reset(rst),
  );

`else // not DEBUG

// no debug  version
module chip (
    // 25Hz clock input
    input  clk,

    inout  [1:0] qd,
    input  dcs,
    input  dsck,

    // led outputs
    output [3:0] led,

  );

  wire clk275;
  wire clk200;
  wire clk100;
  wire clk95;
  wire locked;
  
  reg rst;

  reg [12:0] resetCounter;
  always @ (posedge clk) begin
    if(resetCounter < 1024) begin
      rst <= 1;
      resetCounter <= resetCounter+1;
    end else begin
      rst <= 0;
    end
  end

  pll95 clock_95 (
    .clock_in(clk),
    .clock_out(clk95),
    .locked(locked)
  );


  // pll200 clock_200 (
  //   .clock_in(clk),
  //   .clock_out(clk200),
  //   .locked(locked)
  // );
  
  // frequency_divider_by2 clock_100(
  //   .clk(clk200),
  //   .out_clk(clk100)
  // ); 
       
  wire useClk = clk;

  assign led = 'hf;
   
  wire [1:0] io_qd_read, io_qd_write, io_qd_writeEnable;

  SB_IO #(
    .PIN_TYPE(6'b 1010_01),
    .PULLUP(1'b0)
  ) qd1 [1:0] (
    .PACKAGE_PIN(qd),
    .OUTPUT_ENABLE(io_qd_writeEnable),
    .D_OUT_0(io_qd_write),
    .D_IN_0(io_qd_read)
  );

  MyTopLevel top_level (
    .io_qd_read(io_qd_read),
    .io_qd_write(io_qd_write),
    .io_qd_writeEnable(io_qd_writeEnable),
    .io_ss(dcs),
    .io_sclk(dsck),
    .clk(useClk),
    .reset(rst),
  );

`endif // DEBUG



endmodule
