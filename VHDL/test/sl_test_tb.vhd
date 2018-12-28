library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all; -- read std_ulogic etc

library std;
use std.textio.all;

use work.sl_misc_p.all;
use work.wishbone_p.all;

entity sl_test_tb is
end entity sl_test_tb;

architecture behav of sl_test_tb is

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
  signal code_data     : std_ulogic_vector(15 downto 0);
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
  
  signal master_in : wb_master_ifc_in_array_t(1 downto 0);
  signal master_out : wb_master_ifc_out_array_t(1 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(2 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(2 downto 0);

  signal proc_master_out : wb_master_ifc_out_t;

begin  -- architecture Behav

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
      code_re_o       => code_en,
      code_data_i     => code_data,
      ext_master_i    => master_in(0),
      ext_master_o    => proc_master_out,
      debug_slave_i   => slave_in(0),
      debug_slave_o   => slave_out(0),
      executed_addr_o => executed_addr);

  master_out(0) <= (
    to_unsigned(1,23) & proc_master_out.adr(8 downto 0),
    proc_master_out.dat,
    proc_master_out.we,
    proc_master_out.sel,
    proc_master_out.stb,
    proc_master_out.cyc);

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
      MasterConfig => (wb_master("ext_mem"),wb_master("core_mem ext_mem code_mem")),
      SlaveMap     => (wb_slave("core_mem",0,512),wb_slave("ext_mem",512,512),wb_slave("code_mem",1024,1024)))
    port map (
      clk_i       => sl_clk,
      reset_n_i   => reset_n,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

  sl_code_mem_1: entity work.sl_code_mem
    generic map (
      SizeInKBytes => 2)
    port map (
      p2_code_o  => open,
      clk_i        => sl_clk,
      reset_n_i    => reset_n,
      p0_addr_i    => code_addr,
      p0_code_o    => code_data,
      p0_re_i      => code_en,
      p0_reset_n_i => reset_n,
      p1_addr_i    => to_unsigned(0,16),
      p1_code_o    => open,
      p1_re_i      => '0',
      p1_reset_n_i => '0',
      p2_addr_i    => code_addr,
      p2_re_i      => code_en,
      p2_reset_n_i => reset_n,
      p3_addr_i    => to_unsigned(0,16),
      p3_code_o    => open,
      p3_re_i      => '0',
      p3_reset_n_i => '0',
      wb_slave_i   => slave_in(2),
      wb_slave_o   => slave_out(2));

  process (sl_clk, reset_n) is
  begin  -- process
    if reset_n = '0' then               -- asynchronous reset (active low)
      ext_slave <= ((others => '0'),'0','0','0');
      ext_mem_pending <= '0';
    elsif sl_clk'event and sl_clk = '1' then  -- rising clock edge
      if slave_in(1).cyc = '1' and (slave_in(1).stb = '1' or ext_mem_pending = '1') then
        ext_slave.ack <= not ext_mem_stall;
         
        if slave_in(1).we = '1' and ext_mem_pending = '0' then
          ext_mem(to_integer(slave_in(1).adr)) <= slave_in(1).dat;
        else
          ext_slave.dat <= ext_mem(to_integer(slave_in(1).adr));
        end if;
      end if;

      if slave_in(1).cyc = '1' and slave_in(1).stb = '1' then
        ext_mem_pending <= '1';
      elsif ext_mem_stall = '0' then
        ext_mem_pending <= '0';
      end if;

      -- sideload while proc master accessing this memory
      if master_out(0).cyc = '1' and mem_we = '1' and mem_addr >= to_unsigned(512,32) and mem_addr < to_unsigned(1024,32) then
        ext_mem(to_integer(mem_addr)-512) <= mem_din;
      end if;
    end if;
  end process;

  slave_out(1) <= (ext_slave.dat,ext_slave.ack,ext_slave.err,ext_mem_stall);

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

    mem_addr <= to_unsigned(0,16);
    mem_din <= (others => '0');
    mem_we <= '0';
    mem_en <= '0';

    ext_mem_stall <= '0';
    
    file_open(test_file,"test.vector");
    
    wait for 33 ns;

    reset_n <= '1';

    wait until rising_edge(clk);

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

          enable_core <= '0';

          -- add additional cycle for mem system so that sync handling for core enable is also checked
          wait for 1 ps;
          sl_clk <= '1';
          wait for 0.1 ns;
          sl_clk <= '0';

          for i in 0 to 1023 loop
            mem_addr <= to_unsigned(1024+i,16);
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
          core_reset_n <= '0';
          wait for 33 ns;
          core_reset_n <= '1';
        when X"0001" =>      
          j := 0;
          enable_core <= '0';
          read_ok := true;
          while read_ok and dummy /= LF loop
            hread(l,data16);
            mem_addr <= to_unsigned(1024+j,16);
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
          enable_core <= '1';
          write(output,LF & "  " & integer'image(j) & " code words loaded");
        when X"0002" | X"0005" =>
          ext_mem_stall <= '0';
          if entry_type = X"0005" then
            ext_mem_stall <= '1';
          end if;
          hread(l,data32);
          j := to_integer(unsigned(data32));
          write(output,LF & "  run " & integer'image(j) & " cycles");
          for i in 0 to j*2-1 loop
            sl_clk <= '1';
            wait for 10ns;
            sl_clk <= '0';
            wait for 10ns;
          end loop;  -- i
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
        when X"000F" =>
          hread(l,data32);
          mem_addr <= unsigned(data32(15 downto 0));

          wait for 1 ps;
          -- generate clock for memory
          enable_core <= '0';

          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';
          wait for 1 ns;

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
        when others => null;
      end case;
    end loop;

    file_close(test_file);

    wait;
    
  end process;

end architecture behav;
