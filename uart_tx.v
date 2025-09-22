module uart_tx (
    input clk,
    input rst_n,
    input trigger,
    input [7:0] data_in,
    output reg tx,
    output reg busy
);

    parameter BAUD_DIV = 434;  // BAUD_DIV = 50M / 115200 ≈ 434
    parameter TICK_DIV = 27; // Số chu kỳ clock mỗi tick (~27)

    localparam STATE_IDLE  = 2'd0, STATE_START = 2'd1, STATE_DATA  = 2'd2, STATE_STOP  = 2'd3;

    reg [1:0] state;
    reg [12:0] clk_counter;
    reg [3:0] tick_counter;
    reg [3:0] bit_index;
    reg [7:0] tx_shift;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state        <= STATE_IDLE;
            tx           <= 1'b1;
            clk_counter  <= 13'd0;
            tick_counter <= 4'd0;
            bit_index    <= 4'd0;
            busy         <= 1'b0;
            tx_shift     <= 8'd0;
        end else begin
            case (state)
                STATE_IDLE: begin
                    tx           <= 1'b1;
                    clk_counter  <= 13'd0;
                    tick_counter <= 4'd0;
                    bit_index    <= 4'd0;
                    busy         <= 1'b0;
                    if (trigger) begin
                        busy      <= 1'b1;
                        tx_shift  <= data_in;
                        state     <= STATE_START;
                    end
                end
                STATE_START: begin
                    tx <= 1'b0;
                    if (clk_counter < TICK_DIV - 1'b1) begin
                        clk_counter <= clk_counter + 1'b1;
                    end else begin
                        clk_counter <= 13'd0;
                        if (tick_counter < 4'd15) begin
                            tick_counter <= tick_counter + 1'b1;
                        end else begin
                            tick_counter <= 4'd0;
                            state <= STATE_DATA;
                        end
                    end
                end
                STATE_DATA: begin
                    tx <= tx_shift[0];
                    if (clk_counter < TICK_DIV - 1'b1) begin
                        clk_counter <= clk_counter + 1'b1;
                    end else begin
                        clk_counter <= 13'd0;
                        if (tick_counter < 4'd15) begin
                            tick_counter <= tick_counter + 1'b1;
                        end else begin
                            tick_counter <= 4'd0;
                            tx_shift <= {1'b0, tx_shift[7:1]};
                            if (bit_index < 4'd7) begin
                                bit_index <= bit_index + 1'b1;
                            end else begin
                                bit_index <= 4'd0;
                                state <= STATE_STOP;
                            end
                        end
                    end
                end
                STATE_STOP: begin
                    tx <= 1'b1;
                    if (clk_counter < TICK_DIV - 1'b1) begin
                        clk_counter <= clk_counter + 1'b1;
                    end else begin
                        clk_counter <= 13'd0;
                        if (tick_counter < 4'd15) begin
                            tick_counter <= tick_counter + 1'b1;
                        end else begin
                            tick_counter <= 4'd0;
                            state <= STATE_IDLE;
                        end
                    end
                end
                default: state <= STATE_IDLE;
            endcase
        end
    end
endmodule