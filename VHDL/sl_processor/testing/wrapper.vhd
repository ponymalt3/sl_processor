library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.sl_misc_p.all;
use work.wishbone_p.all;

-- Structural wrapper for integration testing (no cache — code memory is
-- served combinationally so tests run without a prefetch phase).
entity testing_wrapper is
  port (
    clk_i                : in  std_ulogic;
    reset_n_i            : in  std_ulogic;
    enable_core_i        : in  std_ulogic;
    core_reset_n_i       : in  std_ulogic;
    ext_mem_stall_i      : in  std_ulogic;
    force_proc_bus_off_i : in  std_ulogic;

    mem_addr_i     : in  unsigned(15 downto 0);
    mem_din_i      : in  std_ulogic_vector(31 downto 0);
    mem_we_i       : in  std_ulogic;
    mem_en_i       : in  std_ulogic;
    mem_dout_o     : out std_ulogic_vector(31 downto 0);
    mem_complete_o : out std_ulogic;

    executed_addr_o : out unsigned(15 downto 0)
  );
end entity testing_wrapper;

architecture behav of testing_wrapper is

  type mem_t is array (natural range <>) of std_ulogic_vector(31 downto 0);

  signal mem_clk        : std_ulogic;
  signal code_addr      : unsigned(15 downto 0);

  signal master_in      : wb_master_ifc_in_array_t(1 downto 0);
  signal master_out_all : wb_master_ifc_out_array_t(1 downto 0);
  signal slave_in       : wb_slave_ifc_in_array_t(3 downto 0);
  signal slave_out      : wb_slave_ifc_out_array_t(3 downto 0);

  signal proc_master_out : wb_master_ifc_out_t;

  signal ext_mem         : mem_t(511 downto 0)  := (others => (others => '0'));
  signal ext_slave       : wb_slave_ifc_out_t;
  signal ext_mem_pending : std_ulogic;

  signal code_mem        : mem_t(4095 downto 0) := (others => (others => '0'));
  signal code_slave      : wb_slave_ifc_out_t;
  signal code_word       : std_ulogic_vector(15 downto 0);

begin

  mem_clk <= not clk_i;

  -- Disconnect processor from wishbone when force_proc_bus_off_i = '1'
  master_out_all(0) <= (
    proc_master_out.adr + to_unsigned(512, 32),
    proc_master_out.dat,
    proc_master_out.we,
    proc_master_out.sel,
    proc_master_out.stb,
    proc_master_out.cyc and not force_proc_bus_off_i);

  sl_processor_1 : entity work.sl_processor
    generic map (
      LocalMemSizeInKB   => 2,
      CodeStartAddr      => 0,
      EnableDebugMemPort => true)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      core_en_i       => enable_core_i,
      core_reset_n_i  => core_reset_n_i,
      code_addr_o     => code_addr,
      code_stall_i    => '0',
      code_data_i     => code_word,
      ext_master_i    => master_in(0),
      ext_master_o    => proc_master_out,
      debug_slave_i   => slave_in(0),
      debug_slave_o   => slave_out(0),
      executed_addr_o => executed_addr_o);

  wb_master_1 : entity work.wb_master
    port map (
      clk_i               => clk_i,
      reset_n_i           => reset_n_i,
      addr_i(15 downto 0) => mem_addr_i,
      addr_i(31 downto 16) => X"0000",
      din_i               => mem_din_i,
      dout_o              => mem_dout_o,
      en_i                => mem_en_i,
      we_i                => mem_we_i,
      complete_o          => mem_complete_o,
      err_o               => open,
      master_out_i        => master_in(1),
      master_out_o        => master_out_all(1));

  wb_ixs_1 : entity work.wb_ixs
    generic map (
      MasterConfig => (wb_master("ext_mem xorshift"),
                       wb_master("core_mem ext_mem code_mem")),
      SlaveMap     => (wb_slave("core_mem",  0,         512),
                       wb_slave("ext_mem",   512,       512),
                       wb_slave("code_mem",  4096,      4096),
                       wb_slave("xorshift",  16#F00100#, 256)))
    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => master_out_all,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

  wb_xorshift_slave_1 : entity work.wb_xorshift_slave
    port map (
      clk_i     => clk_i,
      reset_n_i => reset_n_i,
      slave_i   => slave_in(3),
      slave_o   => slave_out(3));

  -- Code memory: written via wishbone (test master), read combinationally by processor
  process (clk_i, reset_n_i) is
  begin
    if reset_n_i = '0' then
      code_slave <= ((others => '0'), '0', '0', '0');
    elsif rising_edge(clk_i) then
      code_slave.ack <= '0';
      if slave_in(2).cyc = '1' and slave_in(2).stb = '1' then
        code_slave.ack <= '1';
        if slave_in(2).we = '1' then
          code_mem(to_integer(slave_in(2).adr)) <= slave_in(2).dat;
        else
          code_slave.dat <= code_mem(to_integer(slave_in(2).adr));
        end if;
      end if;
    end if;
  end process;

  slave_out(2) <= code_slave;
  code_word    <= code_mem(to_integer(code_addr))(15 downto 0) when reset_n_i = '1' else (others => '0');

  -- External memory: written/read via wishbone, optional stall
  process (clk_i, reset_n_i) is
  begin
    if reset_n_i = '0' then
      ext_slave      <= ((others => '0'), '0', '0', '0');
      ext_mem_pending <= '0';
    elsif rising_edge(clk_i) then
      ext_slave.ack <= not ext_mem_stall_i;
      if slave_in(1).cyc = '1' and (slave_in(1).stb = '1' or ext_mem_pending = '1') then
        if slave_in(1).we = '1' and ext_mem_pending = '0' then
          ext_mem(to_integer(slave_in(1).adr)) <= slave_in(1).dat;
        else
          ext_slave.dat <= ext_mem(to_integer(slave_in(1).adr));
        end if;
      end if;
      if slave_in(1).cyc = '1' and slave_in(1).stb = '1' then
        ext_mem_pending <= '1';
      elsif ext_mem_stall_i = '0' then
        ext_mem_pending <= '0';
      end if;
    end if;
  end process;

  slave_out(1) <= (ext_mem(to_integer(slave_in(1).adr)),
                   ext_slave.ack, ext_slave.err, ext_mem_stall_i);

end architecture behav;
