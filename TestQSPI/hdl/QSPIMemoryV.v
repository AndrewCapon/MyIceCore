 
//`define DEBUG

module QSPIMemoryV(
  input  [1:0]      QD_READ,
  output [1:0]      QD_WRITE,
  output reg [1:0]  QD_WRITE_ENABLE,

  input         SS,
  input         SCLK,

  input         CLK,
  input         RST,

`ifdef DEBUG
  output [3:0]  LEDS,
  output        reg DBG_IO0,
  output        reg DBG_IO1,
  output        reg DBG_SS,
  output        reg DBG_SCLK,
  
  output        DBG_1,
  output        DBG_2,
  output        DBG_3,
  output        DBG_4,

  output [7:0]  DBG_BYTE
`endif
);


///////////////////////////////////////////////////////////////////////
// memory initialised with test data
///////////////////////////////////////////////////////////////////////
reg [15:0] mem [0:8191];
reg [12:0] addrCounter;
initial begin
    $readmemb("MyTopLevel.v_toplevel_memoryCtrl_mem.bin",mem);
end
 




///////////////////////////////////////////////////////////////////////
// tristate stuff
///////////////////////////////////////////////////////////////////////
reg reading;

always @ (*) begin
  if(reading)begin
    QD_WRITE_ENABLE = (2'b00);
  end else begin
    QD_WRITE_ENABLE = (2'b11);
  end
end





///////////////////////////////////////////////////////////////////////
// debug code
///////////////////////////////////////////////////////////////////////
`ifdef DEBUG
always @ (*) begin
  DBG_SS = SS;
  DBG_SCLK = SCLK;
  if(reading)begin
    DBG_IO0 = QD_READ[0];
    DBG_IO1 = QD_READ[1];
  end else begin
    DBG_IO0 = QD_WRITE[0];
    DBG_IO1 = QD_WRITE[1];
  end
end

assign LEDS = 4'b0001;

assign DBG_1 = spi_rxready;
assign DBG_2 = spi_txready;
assign DBG_3 = reading;
assign DBG_4 = 0;

assign DBG_BYTE = spi_rxdata;
`endif





///////////////////////////////////////////////////////////////////////
// spi data
///////////////////////////////////////////////////////////////////////
reg [7:0] spi_txdata, spi_rxdata;
wire spi_txready, spi_rxready;
reg [15:0] sendWord;
reg [7:0] receiveLSB;


///////////////////////////////////////////////////////////////////////
// state machine
///////////////////////////////////////////////////////////////////////
parameter sWait                 = 0;
parameter sDecodeCommand        = 1;
parameter sDecodeAddressMSB     = 2;
parameter sDecodeAddressLSB     = 3;
parameter sDispatchCommand      = 4;
parameter sReceiveLSB           = 5;
parameter sReceiveMSB           = 6;
parameter sSendLSB              = 7;
parameter sSendMSB              = 8;
parameter sTestSend             = 9;
parameter sTestReceive          = 10;

reg [3:0] stateNext;
reg [7:0] commandId;

always @ (posedge CLK) begin 
  if (RST | SS) begin
    addrCounter <= 0;
    reading <= 1;
    //receiveLSB <= 8'hff;
    stateNext <= sWait;
    sendWord <= 0;
  end else begin
    case(stateNext)
      ///////////////////////////////////////////////////////////////////////
      // Wait State
      ///////////////////////////////////////////////////////////////////////
      sWait : begin
        reading <= 1;
        spi_txdata <= 8'hff; // Debug
        if(SS == 0) begin
          stateNext <= sDecodeCommand; 
          spi_txdata <= 0;
        end
      end // sWait

      ///////////////////////////////////////////////////////////////////////
      // Decode Command state
      ///////////////////////////////////////////////////////////////////////
      sDecodeCommand : begin
        spi_txdata <= 8'hAA; // DEBUG
        if(spi_rxready) begin
          commandId <= spi_rxdata;
          stateNext <= sDecodeAddressMSB;
        end
      end // sDecodeCommand


      ///////////////////////////////////////////////////////////////////////
      // Decode address MSB state (4 bis)
      ///////////////////////////////////////////////////////////////////////
      sDecodeAddressMSB : begin
        if(spi_rxready) begin
          addrCounter[11:8] <= spi_rxdata[3:0];
          stateNext <= sDecodeAddressLSB;
        end
      end // sDecodeAddressMSB
      

      ///////////////////////////////////////////////////////////////////////
      // Decode address LSB state (8 bits)
      ///////////////////////////////////////////////////////////////////////
      sDecodeAddressLSB : begin
        if(spi_rxready) begin
          addrCounter[7:0] <= spi_rxdata;
          stateNext <= sDispatchCommand;
        end
      end // sDecodeAddressMSB


      ///////////////////////////////////////////////////////////////////////
      // DispatchCommand
      ///////////////////////////////////////////////////////////////////////
      sDispatchCommand : begin
        case(commandId)
          // writing memory
          8'h01 : begin
            stateNext <= sReceiveLSB;
          end

          // reading memory
          8'h02 : begin
            stateNext <= sSendLSB;
          end

          default : begin // Bad CommandID
            stateNext <= sWait;
          end
        endcase
      end

      ///////////////////////////////////////////////////////////////////////
      // Receive data LSB
      ///////////////////////////////////////////////////////////////////////
      sReceiveLSB : begin
        reading <= 1;
        if(spi_rxready) begin
          receiveLSB <= spi_rxdata;
          stateNext <= sReceiveMSB;
        end 
      end 


      ///////////////////////////////////////////////////////////////////////
      // Receive data MSB
      ///////////////////////////////////////////////////////////////////////
      sReceiveMSB : begin
        reading <= 1;
        if(spi_rxready) begin
          mem[addrCounter] <= {spi_rxdata, receiveLSB};
          stateNext <= sReceiveLSB;
          addrCounter <= addrCounter +1;
        end
      end
  

      ///////////////////////////////////////////////////////////////////////
      // send data LSB
      ///////////////////////////////////////////////////////////////////////
      sSendLSB : begin
        reading <= 0;
        if(spi_txready) begin
          sendWord <= mem[addrCounter];
          spi_txdata <= sendWord[15:8];
          stateNext <= sSendMSB;
        end
      end


      ///////////////////////////////////////////////////////////////////////
      // send data MSB
      ///////////////////////////////////////////////////////////////////////
      sSendMSB : begin
        reading <= 0;
        if(spi_txready) begin
          spi_txdata <= sendWord[7:0];
          addrCounter <= addrCounter +1;
          stateNext <= sSendLSB;
        end
      end


      ///////////////////////////////////////////////////////////////////////
      // test send data
      ///////////////////////////////////////////////////////////////////////
      sTestSend : begin
        reading <= 0;
        if(spi_txready) begin
          spi_txdata <= addrCounter[7:0];
          addrCounter <= addrCounter +1;
        end
      end

      ///////////////////////////////////////////////////////////////////////
      // Receive data LSB
      ///////////////////////////////////////////////////////////////////////
      sTestReceive : begin
        reading <= 1;
        if(spi_rxready) begin
          receiveLSB <= spi_rxdata;
        end 
      end 

      ///////////////////////////////////////////////////////////////////////
      // default 
      ///////////////////////////////////////////////////////////////////////
      default : begin
        stateNext <= sWait;
      end
    endcase
  end
end


// ///////////////////////////////////////////////////////////////////////
// // state machine
// ///////////////////////////////////////////////////////////////////////
// parameter sWait                 = 0;
// parameter sDecode               = 1;
// parameter sDecodeAddressMSB     = 2;
// parameter sDecodeAddressLSB     = 3;
// parameter sReceiveSkipDummyByte = 4;
// parameter sReceiveLSB           = 5;
// parameter sReceiveMSB           = 6;
// parameter sSendSkipDummyByte    = 7;
// parameter sSendLSB              = 8;
// parameter sSendMSB              = 9;
// parameter sTestSend             = 10;
// parameter sTestReceive          = 11;

// reg [3:0] stateNext;
// reg [3:0] stateSkip;

// always @ (posedge CLK) begin 
//   if (RST | SS) begin
//     addrCounter <= 0;
//     reading <= 1;
//     //receiveLSB <= 8'hff;
//     stateNext <= sTestSend; // sWait;
//     sendWord <= 0;
//   end else begin
//     case(stateNext)
//       ///////////////////////////////////////////////////////////////////////
//       // Wait State
//       ///////////////////////////////////////////////////////////////////////
//       sWait : begin
//         reading <= 1;
//         spi_txdata <= 8'hff; // Debug
//         if(SS == 0) begin
//           stateNext <= sDecode; 
//           spi_txdata <= 0;
//         end
//       end // sWait

//       ///////////////////////////////////////////////////////////////////////
//       // Decode state
//       ///////////////////////////////////////////////////////////////////////
//       sDecode : begin
//         spi_txdata <= 8'hAA; // DEBUG
//         if(spi_rxready) begin
//           case(spi_rxdata)
//             // writing memory
//             8'h01 : begin
//               stateSkip <= sReceiveSkipDummyByte;
//               stateNext <= sDecodeAddressMSB;
//             end

//             // reading memory
//             8'h02 : begin
//               stateSkip <= sSendSkipDummyByte;
//               stateNext <= sDecodeAddressMSB;
//             end

//             default : begin
//               stateNext <= sWait;
//             end
//           endcase
//         end
//       end // sDecode


//       ///////////////////////////////////////////////////////////////////////
//       // Decode address MSB state (4 bis)
//       ///////////////////////////////////////////////////////////////////////
//       sDecodeAddressMSB : begin
//         if(spi_rxready) begin
//           addrCounter[11:8] <= spi_rxdata[3:0];
//           stateNext <= sDecodeAddressLSB;
//         end
//       end // sDecodeAddressMSB
      

//       ///////////////////////////////////////////////////////////////////////
//       // Decode address LSB state (8 bits)
//       ///////////////////////////////////////////////////////////////////////
//       sDecodeAddressLSB : begin
//         if(spi_rxready) begin
//           addrCounter[7:0] <= spi_rxdata;
//           stateNext <= stateSkip;
//         end
//       end // sDecodeAddressMSB


//       ///////////////////////////////////////////////////////////////////////
//       // skip dummy byte for receive
//       ///////////////////////////////////////////////////////////////////////
//       sReceiveSkipDummyByte: begin
//         if(spi_rxready) begin
//           stateNext <= sReceiveLSB;
//         end
//       end


//       ///////////////////////////////////////////////////////////////////////
//       // skip dummy byte for send
//       ///////////////////////////////////////////////////////////////////////
//       sSendSkipDummyByte : begin // TX is always 1 byte ahead
//         reading <= 0;
//         sendWord <= mem[addrCounter];
//         spi_txdata <= sendWord[15:8];
//         stateNext <= sSendMSB;
//       end
    

//       ///////////////////////////////////////////////////////////////////////
//       // Receive data LSB
//       ///////////////////////////////////////////////////////////////////////
//       sReceiveLSB : begin
//         reading <= 1;
//         if(spi_rxready) begin
//           receiveLSB <= spi_rxdata;
//           stateNext <= sReceiveMSB;
//         end 
//       end 


//       ///////////////////////////////////////////////////////////////////////
//       // Receive data MSB
//       ///////////////////////////////////////////////////////////////////////
//       sReceiveMSB : begin
//         reading <= 1;
//         if(spi_rxready) begin
//           mem[addrCounter] <= {spi_rxdata, receiveLSB};
//           stateNext <= sReceiveLSB;
//           addrCounter <= addrCounter +1;
//         end
//       end
  

//       ///////////////////////////////////////////////////////////////////////
//       // send data LSB
//       ///////////////////////////////////////////////////////////////////////
//       sSendLSB : begin
//         reading <= 0;
//         if(spi_txready) begin
//           sendWord <= mem[addrCounter];
//           spi_txdata <= sendWord[15:8];
//           stateNext <= sSendMSB;
//         end
//       end


//       ///////////////////////////////////////////////////////////////////////
//       // send data MSB
//       ///////////////////////////////////////////////////////////////////////
//       sSendMSB : begin
//         reading <= 0;
//         if(spi_txready) begin
//           spi_txdata <= sendWord[7:0];
//           addrCounter <= addrCounter +1;
//           stateNext <= sSendLSB;
//         end
//       end


//       ///////////////////////////////////////////////////////////////////////
//       // test send data
//       ///////////////////////////////////////////////////////////////////////
//       sTestSend : begin
//         reading <= 0;
//         if(spi_txready) begin
//           spi_txdata <= addrCounter[7:0];
//           addrCounter <= addrCounter +1;
//         end
//       end

//       ///////////////////////////////////////////////////////////////////////
//       // Receive data LSB
//       ///////////////////////////////////////////////////////////////////////
//       sTestReceive : begin
//         reading <= 1;
//         if(spi_rxready) begin
//           receiveLSB <= spi_rxdata;
//         end 
//       end 

//       ///////////////////////////////////////////////////////////////////////
//       // default 
//       ///////////////////////////////////////////////////////////////////////
//       default : begin
//         stateNext <= sWait;
//       end
//     endcase
//   end
// end


qspislave_tx #(.DWIDTH(2)) QT (
  .clk(CLK),
  .txdata(spi_txdata),
  .txready(spi_txready),
  .QCK(SCLK),
  .QSS(SS),
  .QD(QD_WRITE)
);

qspislave_rx #(.DWIDTH(2)) QR (
  .clk(CLK),
  .rxdata(spi_rxdata),
  .rxready(spi_rxready),
  .QCK(SCLK),
  .QSS(SS),
  .QD(QD_READ)
);

endmodule