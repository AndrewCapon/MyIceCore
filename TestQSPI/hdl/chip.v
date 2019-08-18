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
  wire clk50;
  wire clk25;
  wire clk10;
  wire clk12_5;
  wire clk6_25;
  wire clk3_125;
  wire real_clk_12.5;
  wire real_clk_6_25;
  
  wire locked;

  // pll20 clock_20 (
  //   .clock_in(clk),
  //   .clock_out(clk200),
  //   .locked(locked)
  // );

  // frequency_divider_by2 clock_10(
  //   .clk(clk20),
  //   .out_clk(clk10)
  // );

  // frequency_divider_by2 clock_5(
  //   .clk(clk10),
  //   .out_clk(clk5)
  // );

//`define FASTCLOCK
`ifdef FASTCLOCK
  pll275 clock_275 (
    .clock_in(clk),
    .clock_out(clk275),
    .locked(locked)
  );
  wire useClk = clk275;
`else
  pll200 clock_200 (
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

  frequency_divider_by2 clock_3_125(
    .clk(clk6_25),
    .out_clk(clk3_125)
  );
   
  wire useClk = clk25;
`endif
 
//`define VERILOG

`ifdef VERILOG
  reg rdbg;

  assign prb00 = spi_rxready;
  assign prb01 = spi_txready;
  assign prb02 = rxReadyChange;
  assign prb03 = useClk;

  assign prb04 = io0;
  assign prb05 = io1;
  assign prb06 = dcs;
  assign prb07 = dsck;

  assign prb08 = 0;
  assign prb09 = 0;
  assign prb10 = 0;
  assign prb11 = 0;

  assign prb12 = 0;
  assign prb13 = 0;
  assign prb14 = 0;
  assign prb15 = 0;

  reg [3:0] qdin;
  reg [3:0] qdout;

  reg [7:0] spi_rxdata;
  wire spi_rxready;

  reg [7:0] spi_txdata;
  wire spi_txready;

 
  // assign dbg_1 = spi_rxready;
  // assign dbg_2 = spi_rxready;
  // assign dbg_3 = 0;
  // assign dbg_4 = 0;

  assign led[0] = 0;
  assign led[1] = 0;
  assign led[2] = 0;
  assign led[3] = 0;


  reg [3:0] counter;
  reg rxReadyChange;

  always @(posedge useClk)
  begin
    qdin <= {io0,io0,io0,io0};
  end

  assign io1 = qdout[0];
 
  always @(posedge useClk)
  begin
    //spi_txdata <= 'h55;
    if(spi_rxready) begin
      rxReadyChange <= !rxReadyChange;
      if(spi_rxdata == 'h01) begin
        spi_txdata <='h01;
        rdbg <= 1;
      end else if (spi_rxdata == 'h02) begin
        spi_txdata <='h02;
        rdbg <= 0;
      end
    end
  end

  qspislave_rx QR (
    .clk(useClk),
    .rxdata(spi_rxdata),
    .rxready(spi_rxready),
    .QCK(dsck),
    .QSS(dcs),
    .QD(qdin),
//    .dbg({prb04, prb05, prb06, prb07})
  );

  qspislave_tx QT (
    .clk(useClk),
    .txdata(spi_txdata),
    .txready(spi_txready),
    .QCK(dsck),
    .QSS(dcs),
    .QD(qdout),
//    .dbg({prb06, prb07})
  );

`else
assign prb08 = 0;
assign prb09 = 0;
assign prb10 = 0;
assign prb11 = 0;

assign prb12 = 0;
assign prb13 = 0;
assign prb14 = 0;
assign prb15 = 0;

 

MyTopLevel top_level (
  .io_leds(led),
  .io_io0(io0),
  .io_io1(io1),
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

  .clk(useClk),
  .io_dummy_clk(useClk),

  .reset(0),
);
`endif

endmodule
