library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity top is

  port (
    clock_50  : in    std_ulogic;
    sw        : in    std_ulogic_vector(3 downto 0);
    key       : in    std_ulogic_vector(1 downto 0);
    led       : out   std_ulogic_vector(7 downto 0);
    dram_addr : out   std_ulogic_vector(12 downto 0);
    dram_dq   : inout std_ulogic_vector(15 downto 0);
    gpio_0    : inout std_ulogic_vector(33 downto 0);
    gpio_1    : inout std_ulogic_vector(33 downto 0);
    gpio_2    : inout std_ulogic_vector(12 downto 0));   
  
end entity top;

architecture rtl of top is

  constant ST_IDLE : natural := 0;
  constant ST_ADDR : natural := 1;
  constant ST_LEN : natural := 2;
  constant ST_ACK : natural := 3;
  constant ST_DISPATCH : natural := 4;
  constant ST_READ : natural := 5;
  constant ST_WRITE : natural := 6;
  constant ST_CORE : natural := 7;
  constant ST_PING : natural := 8;

  signal wbs_in    : wb_slave_ifc_in_t;
  signal wbs_out   : wb_slave_ifc_out_t;

  signal ext_master_in  : wb_master_ifc_in_t;
  signal ext_master_out : wb_master_ifc_out_t;
  signal ext_master_out2 : wb_master_ifc_out_t;
  signal code_master_in  : wb_master_ifc_in_t;
  signal code_master_out : wb_master_ifc_out_t;
  signal master_in : wb_master_ifc_in_array_t(1 downto 0);
  signal master_out : wb_master_ifc_out_array_t(1 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(0 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(0 downto 0);

  signal wb_en : std_ulogic;
  signal wb_re : std_ulogic;
  signal wb_we : std_ulogic;
  signal wb_dout : std_ulogic_vector(31 downto 0);
  signal wb_addr : unsigned(31 downto 0);
  signal wb_complete : std_ulogic;

  signal addr : unsigned(15 downto 0);
  signal len : unsigned(15 downto 0);
  signal state : integer;
  signal cmd : std_ulogic_vector(7 downto 0);
  signal rdata : std_ulogic_vector(31 downto 0);
  signal rstate : std_ulogic_vector(3 downto 0);
  signal wdata : std_ulogic_vector(31 downto 0);
  signal wstate : std_ulogic_vector(3 downto 0);

  signal core_en : std_ulogic_vector(7 downto 0);
  signal core_reset : std_ulogic_vector(7 downto 0);

  signal uart_cr : std_ulogic;
  signal uart_cw : std_ulogic;
  signal uart_we : std_ulogic;
  signal uart_din : std_ulogic_vector(7 downto 0);
  signal uart_dout : std_ulogic_vector(7 downto 0);
  signal uart_rxd : std_ulogic;
  signal uart_txd : std_ulogic;

  signal clk : std_ulogic;-- is clock_50;
  alias reset_n : std_ulogic is key(0);

  signal debug : std_ulogic_vector(7 downto 0);  
  signal mem_clk : std_ulogic;
  signal mem_clk2 : std_ulogic;
  

begin  -- architecture rtl

  pll_1: entity work.pll
    port map (
      inclk0 => clock_50,
		c0 => mem_clk,
		c1 => clk);

  sl_cluster_1: entity work.sl_cluster
    generic map (
      LocalMemSizeInKB  => 2,
      ExtMemSizeInKB    => 8,
      CodeMemSizeInKB   => 8)
    port map (
      clk_i          => clk,
      mem_clk_i      => mem_clk,
      reset_n_i      => reset_n,
      core_en_i      => core_en(3 downto 0),
      core_reset_n_i => core_reset(3 downto 0),
      ext_master_i   => ext_master_in,
      ext_master_o   => ext_master_out,
      code_master_i  => code_master_in,
      code_master_o  => code_master_out,
      debug_o => debug);

wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("mem"),
        wb_master("mem")),
      SlaveMap     => (
      (0 => wb_slave("mem",0,4096))))
      
    port map (
      clk_i       => clk,
      reset_n_i   => reset_n,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

    master_out(0) <= wbs_in;
    wbs_out <= master_in(0);
    master_out(1) <= code_master_out;
    code_master_in <= master_in(1);

  wb_mem_1: entity work.wb_mem
    generic map (
      MemSizeInKB => 32)
    port map (
      clk_i     => clk,
      mem_clk_i => mem_clk,
      reset_n_i => reset_n,
      slave0_i   => ext_master_out2,
      slave0_o   => ext_master_in,
      slave1_i   => slave_in(0),
      slave1_o   => slave_out(0));

  ext_master_out2 <=
    (ext_master_out.adr+to_unsigned(2048,32),
     ext_master_out.dat,
     ext_master_out.we,
     ext_master_out.sel,
     ext_master_out.stb,
     ext_master_out.cyc);

  UART_1: entity work.UART
    generic map (
      BaudRate   => 115200,
      ClockFreq  => 50000000,
      Bits       => 8,
      ParityMode => 0)
    port map (
      clk     => clk,
      reset_n => reset_n,
      din_i   => uart_din,
      we_i    => uart_we,
      cw_o    => uart_cw,
      dout_o  => uart_dout,
      re_i    => '1',
      cr_o    => uart_cr,
      RxD     => uart_rxd,
      TxD     => uart_txd);

  wb_master_1: entity work.wb_master
    port map (
      clk_i        => clk,
      reset_n_i    => reset_n,
      addr_i       => wb_addr,
      din_i        => rdata,
      dout_o       => wb_dout,
      en_i         => wb_en,
      we_i         => wb_we,
      complete_o   => wb_complete,
      err_o        => open,
      master_out_i => wbs_out,
      master_out_o => wbs_in);

  process (clk, reset_n) is
  begin  -- process
    if reset_n = '0' then               -- asynchronous reset (active low)
      addr <= to_unsigned(0,16);
      len <= to_unsigned(0,16);
      rdata <= (others => '0');
      rstate <= "0000";
      wdata <= (others => '0');
      wstate <= "0000";
      core_reset <= (others => '1');
      core_en <=(others => '0');
      state <= ST_IDLE;
      cmd <= X"00";
    elsif clk'event and clk = '1' then  -- rising clock edge
      if uart_cr = '1' then
        rdata <= rdata(23 downto 0) & uart_dout;
        rstate <= rstate(2 downto 0) & '1';
      end if;

      if uart_cw = '1' then
        wdata <= wdata(23 downto 0) & X"00";
        wstate <= wstate(2 downto 0) & '0';
      end if;
      
      case state is
        when ST_IDLE =>
          if rstate /= "0000" then
            cmd <= rdata(7 downto 0);
            rstate <= "0000";
            if uart_dout(7) = '1' then
              state <= ST_ADDR;
            else
              state <= ST_DISPATCH;
            end if;
          end if;
        when ST_ADDR =>
          if rstate(1 downto 0) = "11" then
            addr <= unsigned(rdata(15 downto 0));
            state <= ST_LEN;
            rstate <= "0000";
          end if;
        when ST_LEN =>
          if rstate(1 downto 0) = "11" then
            len <= unsigned(rdata(15 downto 0));
            state <= ST_DISPATCH;
            rstate <= "0000";
          end if;
        when ST_DISPATCH =>
          state <= state+to_integer(unsigned(cmd(6 downto 0)));
        when ST_ACK =>
          if wstate = "0000" then
            state <= ST_IDLE;
            wstate <= "1000";
            wdata(31 downto 24) <= cmd;
          end if;
        when ST_CORE =>
          if rstate(1 downto 0) = "11" then
            core_en <= rdata(7 downto 0) and not rdata(15 downto 8);
            core_reset <= not rdata(15 downto 8);
            state <= ST_ACK;
            rstate <= "0000";
          end if;
        when ST_PING => state <= ST_ACK;
        when others => null;
      end case;
      
      if (wb_re = '1' and wb_complete = '1') or wb_we = '1' then
        addr <= addr+1;
        len <= len-1;
        if len = to_unsigned(1,16) then
          state <= ST_ACK;              
        end if;
      end if;

      if wb_we = '1' then
        rstate <= "0000";
      end if;

      if wb_re = '1' and wb_complete = '1' then
        wdata <= wb_dout;
        wstate <= "1111";
      end if;
        
    end if;
  end process;

  wb_re <= '1' when wstate = "0000" and state = ST_READ else '0';
  wb_we <= '1' when rstate = "1111" and state = ST_WRITE else '0';
  wb_en <= wb_re or wb_we;
  wb_addr <= to_unsigned(0,16) & addr;

  uart_we <= '1' when wstate /= "0000" else '0';
  uart_din <= wdata(31 downto 24);

  gpio_0(23) <= uart_txd;
  uart_rxd <= gpio_0(22);
  
  led <= core_en(3 downto 0) & debug(3 downto 0);--wstate & to_stdUlogicVector(std_logic_vector(to_unsigned(state,4)));

end architecture rtl;
