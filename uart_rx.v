module uart_rx (
    input clk,               // Xung clock 50 MHz
    input rst_n,             // Reset active-low
    input rx,                // Đầu vào UART RX
    output reg [7:0] data_out, // Dữ liệu nhận được
    output reg valid          // Báo hiệu dữ liệu hợp lệ
);

    parameter BAUD_DIV = 434;  // BAUD_DIV = 50M / 115200 ≈ 434
    parameter TICK_DIV = 27; // Số chu kỳ clock mỗi tick (~27)

    localparam STATE_IDLE  = 2'd0, STATE_START = 2'd1, STATE_DATA  = 2'd2, STATE_STOP  = 2'd3;

    reg [1:0] state;
    reg [12:0] clk_counter;
    reg [3:0] tick_counter;
    reg [3:0] bit_index;
    reg [7:0] rx_shift;
    reg rx_sync1, rx_sync2; // Đồng bộ hóa tín hiệu rx

    // Đồng bộ hóa tín hiệu rx
    always @(*) begin
        if (!rst_n) begin
            rx_sync1 <= 1'b1;
            rx_sync2 <= 1'b1;
        end else begin
            rx_sync1 <= rx;
            rx_sync2 <= rx_sync1;
        end
    end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state       <= STATE_IDLE;
            clk_counter <= 13'd0;
            tick_counter <= 4'd0;
            bit_index   <= 4'd0;
            rx_shift    <= 8'd0;
            data_out    <= 8'd0;
            valid       <= 1'b0;
        end else begin
            case (state)
                STATE_IDLE: begin
                    valid       <= 0;
                    clk_counter <= 13'd0;
                    tick_counter <= 4'd0;
                    bit_index   <= 4'd0;
                    if (rx_sync2 == 1'b0) begin
                        state <= STATE_START;
                    end
                end
                STATE_START: begin
                    if (clk_counter < TICK_DIV - 1'b1) begin
                        clk_counter <= clk_counter + 13'd1;
                    end else begin
                        clk_counter <= 13'd0;
                        tick_counter <= tick_counter + 4'd1;
                        if (tick_counter == 4'd8) begin
                            if (rx_sync2 == 1'b0) begin
                                state <= STATE_DATA;
                            end else begin
                                state <= STATE_IDLE;
                            end
                        end
                    end
                end
                STATE_DATA: begin
                    if (clk_counter < TICK_DIV - 1'b1) begin
                        clk_counter <= clk_counter + 13'd1;
                    end else begin
                        clk_counter <= 13'd0;
                        tick_counter <= tick_counter + 4'd1;
                        if (tick_counter == 4'd8) begin
                            rx_shift[bit_index] <= rx_sync2;
                            if (bit_index < 4'd7) begin
                                bit_index <= bit_index + 4'd1;
                            end else begin
                                bit_index <= 4'd0;
                                state <= STATE_STOP;
                            end
                        end
                    end
                end
                STATE_STOP: begin
                    if (clk_counter < TICK_DIV - 1'b1) begin
                        clk_counter <= clk_counter + 13'd1;
                    end else begin
                        clk_counter <= 13'd0;
                        tick_counter <= tick_counter + 4'd1;
                        if (tick_counter == 4'd8) begin
                            if (rx_sync2 == 1'b1) begin
                                data_out <= rx_shift;
                                valid <= 1'b1;
                            end
                            state <= STATE_IDLE;
                        end
                    end
                end
                default: state <= STATE_IDLE;
            endcase
        end
    end
endmodule