
module sdram (
	clk_clk,
	reset_reset_n,
	sdram_addr,
	sdram_ba,
	sdram_cas_n,
	sdram_cke,
	sdram_cs_n,
	sdram_dq,
	sdram_dqm,
	sdram_ras_n,
	sdram_we_n,
	sdram_slave_address,
	sdram_slave_byteenable_n,
	sdram_slave_chipselect,
	sdram_slave_writedata,
	sdram_slave_read_n,
	sdram_slave_write_n,
	sdram_slave_readdata,
	sdram_slave_readdatavalid,
	sdram_slave_waitrequest);	

	input		clk_clk;
	input		reset_reset_n;
	output	[12:0]	sdram_addr;
	output	[1:0]	sdram_ba;
	output		sdram_cas_n;
	output		sdram_cke;
	output		sdram_cs_n;
	inout	[15:0]	sdram_dq;
	output	[1:0]	sdram_dqm;
	output		sdram_ras_n;
	output		sdram_we_n;
	input	[23:0]	sdram_slave_address;
	input	[1:0]	sdram_slave_byteenable_n;
	input		sdram_slave_chipselect;
	input	[15:0]	sdram_slave_writedata;
	input		sdram_slave_read_n;
	input		sdram_slave_write_n;
	output	[15:0]	sdram_slave_readdata;
	output		sdram_slave_readdatavalid;
	output		sdram_slave_waitrequest;
endmodule
