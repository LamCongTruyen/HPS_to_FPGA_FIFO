//============================================================
// M10K module for testing
//============================================================

module M10K_256_32( 
    output reg [31:0] q,
    input [31:0] d,
    input [11:0] write_address, read_address,
    input we, clk
);
	 // force M10K ram style
    reg [31:0] mem [4095:0]  /* synthesis ramstyle = "no_rw_check, M10K" */;
	 
    always @ (posedge clk) begin
        if (we) begin
            mem[write_address] <= d;
        end
        q <= mem[read_address]; // q doesn't get d in this clock cycle
    end
endmodule