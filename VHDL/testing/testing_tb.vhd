library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all; -- read std_ulogic etc

library std;
use std.textio.all;

use work.sl_misc_p.all;
use work.wishbone_p.all;

entity testing_tb is
end entity testing_tb;

architecture behav of testing_tb is

  type code_mem_t is array (natural range <>) of std_ulogic_vector(15 downto 0);
  type mem_t is array (natural range <>) of std_ulogic_vector(31 downto 0);

  signal clk      : std_ulogic := '0';
  signal reset_n  : std_ulogic;
  signal enable_core : std_ulogic;
  signal core_reset_n : std_ulogic;

  file test_file : text;

  signal sl_clk        : std_ulogic := '0';
  signal code_addr     : unsigned(15 downto 0);
  signal code_en : std_ulogic;
  signal code_data     : std_ulogic_vector(31 downto 0);
  signal mem_addr      : unsigned(15 downto 0);
  signal mem_din       : std_ulogic_vector(31 downto 0);
  signal mem_dout      : std_ulogic_vector(31 downto 0);
  signal mem_we        : std_ulogic;
  signal mem_en        : std_ulogic;
  signal mem_complete  : std_ulogic;
  signal executed_addr : unsigned(15 downto 0);

  signal ext_mem_stall : std_ulogic;
  signal ext_mem : mem_t(511 downto 0);
  signal ext_slave : wb_slave_ifc_out_t;
  signal ext_mem_pending : std_ulogic;

  signal code_mem : mem_t(4095 downto 0);
  signal code_slave : wb_slave_ifc_out_t;
  
  signal master_in : wb_master_ifc_in_array_t(2 downto 0);
  signal master_out : wb_master_ifc_out_array_t(2 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(3 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(3 downto 0);

  signal proc_master_out : wb_master_ifc_out_t;

  signal code_complete : std_ulogic;
  signal code_data2     : std_ulogic_vector(15 downto 0);

  signal mem_clk : std_ulogic;
  signal cache_inv : std_ulogic;
  signal cache_en : std_ulogic;
  signal cache_addr : unsigned(31 downto 0);
  signal cache_prefetch_addr : unsigned(31 downto 0);
  signal code_stall : std_ulogic;
  
  signal force_proc_bus_off : std_ulogic;

begin  -- architecture Behav

  mem_clk <= not sl_clk;

  sl_processor_1: entity work.sl_processor
    generic map (
      LocalMemSizeInKB => 2,
      CodeStartAddr  => 0,
      EnableDebugMemPort => true)
    port map (
      clk_i           => sl_clk,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n,
      core_en_i       => enable_core,
      core_reset_n_i  => core_reset_n,
      code_addr_o     => code_addr,
      code_stall_i    => code_stall,
      code_data_i     => code_data(15 downto 0),
      ext_master_i    => master_in(0),
      ext_master_o    => proc_master_out,
      debug_slave_i   => slave_in(0),
      debug_slave_o   => slave_out(0),
      executed_addr_o => executed_addr);

  code_stall <= not code_complete;

  master_out(0) <= (
    proc_master_out.adr+to_unsigned(512,32),
    proc_master_out.dat,
    proc_master_out.we,
    proc_master_out.sel,
    proc_master_out.stb,
    proc_master_out.cyc and not force_proc_bus_off);

  -- clock generation
  clk <= not clk after 10 ns;

  wb_master_1: entity work.wb_master
    port map (
      clk_i        => sl_clk,
      reset_n_i    => reset_n,
      addr_i(15 downto 0) => mem_addr,
      addr_i(31 downto 16) => X"0000",
      din_i        => mem_din,
      dout_o       => mem_dout,
      en_i         => mem_en,
      we_i         => mem_we,
      complete_o   => mem_complete,
      err_o        => open,
      master_out_i => master_in(1),
      master_out_o => master_out(1));

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (wb_master("ext_mem xorshift"),wb_master("core_mem ext_mem code_mem"),wb_master("code_mem")),
      SlaveMap     => (wb_slave("core_mem",0,512),
                       wb_slave("ext_mem",512,512),
                       wb_slave("code_mem",4096,4096),
                       wb_slave("xorshift",16#F00100#,256)))
    port map (
      clk_i       => sl_clk,
      reset_n_i   => reset_n,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

  wb_xorshift_slave_1: entity work.wb_xorshift_slave
    port map (
      clk_i     => sl_clk,
      reset_n_i => reset_n,
      slave_i   => slave_in(3),
      slave_o   => slave_out(3));

  wb_cache_1: entity work.wb_cache
    generic map (
      WordsPerLine  => 8,
      NumberOfLines => 128,
      WriteTrough   => true)
    port map (
      clk_i           => sl_clk,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n,
      addr_i          => cache_addr,
      din_i           => (others => '0'),
      dout_o          => code_data,
      en_i            => cache_en,
      we_i            => '0',
      complete_o      => code_complete,
      err_o           => open,
      snooping_addr_i(11 downto 0) => slave_in(2).adr(11 downto 0),
      snooping_addr_i(31 downto 12) => "00000000000000000001",
      snooping_en_i   => cache_inv,
      master_out_i    => master_in(2),
      master_out_o    => master_out(2));

  cache_inv <= slave_in(2).cyc and slave_in(2).stb and slave_in(2).we;

  cache_addr <= ((31 downto 13 => '0') & '1' & code_addr(11 downto 0)) or cache_prefetch_addr;

  process (sl_clk, reset_n) is
  begin  -- process
    if reset_n = '0' then               -- asynchronous reset (active low)
      ext_slave <= ((others => '0'),'0','0','0');
      ext_mem_pending <= '0';
    elsif sl_clk'event and sl_clk = '1' then  -- rising clock edge
      ext_slave.ack <= not ext_mem_stall;
      if slave_in(1).cyc = '1' and (slave_in(1).stb = '1' or ext_mem_pending = '1') then
        --ext_slave.ack <= ;
         
        if slave_in(1).we = '1' and ext_mem_pending = '0' then
          ext_mem(to_integer(slave_in(1).adr)) <= slave_in(1).dat;
        else
          --ext_slave.dat <= ext_mem(to_integer(slave_in(1).adr));
        end if;
      end if;

      if slave_in(1).cyc = '1' and slave_in(1).stb = '1' then
        ext_mem_pending <= '1';
      elsif ext_mem_stall = '0' then
        ext_mem_pending <= '0';
      end if;

      -- sideload while proc master accessing this memory
      if master_out(1).cyc = '1' and mem_we = '1' and mem_addr >= to_unsigned(512,32) and mem_addr < to_unsigned(1024,32) then
        ext_mem(to_integer(mem_addr)-512) <= mem_din;
      end if;
    end if;
  end process;
  
  slave_out(1) <= (ext_mem(to_integer(slave_in(1).adr)),ext_slave.ack,ext_slave.err,ext_mem_stall);

  process (sl_clk, reset_n) is
  begin  -- process
    if reset_n = '0' then               -- asynchronous reset (active low)
      code_slave <= ((others => '0'),'0','0','0');
    elsif sl_clk'event and sl_clk = '1' then  -- rising clock edge
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

  -- waveform generation
  process
    variable l : line;
    variable data16 : std_ulogic_vector(15 downto 0);
    variable data32 : std_ulogic_vector(31 downto 0);
    variable dummy : character;
    variable dummy2 : string(1 to 1);
    variable entry_type : std_logic_vector(15 downto 0);
    variable read_ok : boolean;
    variable j : integer;
    variable mem_result : std_ulogic_vector(31 downto 0);
  begin
    reset_n <= '0';
    enable_core <= '0';
    core_reset_n <= '1';
    cache_en <= '0';

    mem_addr <= to_unsigned(0,16);
    mem_din <= (others => '0');
    mem_we <= '0';
    mem_en <= '0';

    ext_mem_stall <= '0';
    
    file_open(test_file,"test.vector");
    
    wait for 33 ns;

    reset_n <= '1';

    wait until rising_edge(clk);

    -- initialize cache
    for i in 0 to 256 loop
      wait for 1 ps;
      sl_clk <= '1';
      wait for 0.1 ns;
      sl_clk <= '0';
      wait for 0.1 ns;
    end loop;  -- i

    while not endfile(test_file) loop

      -- read entry from file
      readline(test_file,l);
      
      hread(l,entry_type);
      read(l,dummy);
      case entry_type is
        when X"0000" =>
          --
          write(output,LF & LF & "Testcase [");
          read(l,dummy2(1),read_ok);
          while read_ok loop
            write(output,dummy2);
            read(l,dummy2(1),read_ok);
          end loop;
          write(output,"]: ");

          force_proc_bus_off <= '0';

          enable_core <= '0';

          -- add additional cycle for mem system so that sync handling for core enable is also checked
          wait for 1 ps;
          sl_clk <= '1';
          wait for 0.1 ns;
          sl_clk <= '0';

          for i in 0 to 4096 loop
            mem_addr <= to_unsigned(4096+i,16);
            mem_din(15 downto 0) <= X"FFFF";
            mem_we <= '1';
            mem_en <= '1';
            wait for 1 ps;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            mem_we <= '0';
            mem_en <= '0';
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            wait for 1 ps;
          end loop;  -- i

          mem_din <= (others => '0');
          for i in 0 to 255 loop
            mem_addr <= to_unsigned(i,16);
            mem_we <= '1';
            mem_en <= '1';
            
            -- generate clock for memory
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;

            mem_we <= '0';
            mem_en <= '0';
            
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
          end loop;  -- i

          write(output,LF & "  clear mem complete");

          enable_core <= '1';
          cache_en <= '1';
          core_reset_n <= '0';
          wait for 33 ns;
          core_reset_n <= '1';
        when X"0001" =>      
          j := 0;
          enable_core <= '0';
          cache_en <= '0';
          wait for 1 ps;
          read_ok := true;
          while read_ok and dummy /= LF loop
            hread(l,data16);
            mem_addr <= to_unsigned(4096+j,16);
            mem_din(15 downto 0) <= data16;
            
            mem_we <= '1';
            mem_en <= '1';
            wait for 1 ps;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            mem_we <= '0';
            mem_en <= '0';            
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            --wait for 1 ps;
            read(l,dummy,read_ok);
            j:=j+1;
          end loop;
          
          write(output,LF & "  " & integer'image(j) & " code words loaded");
          
          cache_en <= '1';
          for i in 0 to j-1 loop
            cache_prefetch_addr <= to_unsigned(i,32);
            wait for 1 ns;
            while code_complete = '0' loop
              sl_clk <= '1';
              wait for 0.1 ns;
              sl_clk <= '0';
              wait for 0.1 ns;
            end loop;
          end loop;  -- i
          cache_prefetch_addr <= to_unsigned(0,32);
          enable_core <= '1';
          
        when X"0002" | X"0005" =>
          ext_mem_stall <= '0';
          if entry_type = X"0005" then
            ext_mem_stall <= '1';
          end if;
          hread(l,data32);
          j := to_integer(unsigned(data32));
          write(output,LF & "  run " & integer'image(j) & " cycles");
          while j > 0 loop
            sl_clk <= '1';
            wait for 10ns;
            sl_clk <= '0';
            wait for 1 ns;
            enable_core <= code_complete;
            wait for 9ns;
            if enable_core = '1' then
              j := j-1;
            end if;
          end loop;
          --ext_mem_stall <= '0';
        when X"0003" =>
          ext_mem_stall <= '0';
          hread(l,data32);
          j := to_integer(unsigned(data32));
          write(output,LF & "  run until addr " & integer'image(j));
          while to_integer(unsigned(executed_addr)) < j  loop
            sl_clk <= '1';
            wait for 10ns;
            sl_clk <= '0';
            wait for 10ns;
          end loop;  -- i
          write(output,"  complete");
        when X"0004" =>
          hread(l,data32);
          mem_addr <= unsigned(data32(15 downto 0));
          
          hread(l,data32);
          mem_din <= data32;

          wait for 1 ps;

          write(output,LF & "  write data " & to_hstring(mem_din) & " at addr " & integer'image(to_integer(mem_addr)));

          enable_core <= '0';
          cache_en <= '0';
          wait for 1 ps;

          -- generate clock for memory
          mem_we <= '1';
          mem_en <= '1';

          wait for 1 ns;

          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;
          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;

          mem_we <= '0';
          mem_en <= '0';

          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;
          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;

          enable_core <= '1';
          cache_en <= '1';
        when X"000F" =>
          hread(l,data32);
          mem_addr <= unsigned(data32(15 downto 0));

          wait for 1 ps;
          -- generate clock for memory
          enable_core <= '0';
          wait for 1 ps;

          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';
          wait for 1 ns;

          force_proc_bus_off <= '1';
          mem_en <= '1';

          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';
          wait for 1 ns;
          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';
          wait for 1 ns;
          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';

          mem_en <= '0';
          
          if mem_complete = '1' then
            mem_result := mem_dout;
          end if;
          
          wait for 1 ns;
          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';
          
          if mem_complete = '1' then
            mem_result := mem_dout;
          end if;

          wait for 1 ns;
          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';

          wait for 1 ns;

          hread(l,data32);
      
          assert mem_result = data32 report LF & "  expect FAILED " & to_hstring(mem_result) & " != " & to_hstring(data32) & LF severity error;
  
          enable_core <= '1';
          cache_en <= '1';
        when others => null;
      end case;
    end loop;

    file_close(test_file);

    wait;
    
  end process;

end architecture behav;
