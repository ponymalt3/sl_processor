library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all; -- read std_ulogic etc

library std;
use std.textio.all;

use work.sl_misc_p.all;

entity sl_test_tb is
end entity sl_test_tb;

architecture Behav of sl_test_tb is

  type code_mem_t is array (natural range <>) of std_ulogic_vector(15 downto 0);
  type mem_t is array (natural range <>) of std_ulogic_vector(31 downto 0);

  signal clk      : std_ulogic := '0';
  signal reset_n  : std_ulogic;
  signal reset_core_n : std_ulogic;

  file test_file : text;
  signal code_mem : code_mem_t(255 downto 0);

  signal sl_clk        : std_ulogic := '0';
  signal code_addr     : unsigned(15 downto 0);
  signal code_data     : std_ulogic_vector(15 downto 0);
  signal mem_addr      : unsigned(15 downto 0);
  signal mem_din       : std_ulogic_vector(31 downto 0);
  signal mem_dout      : std_ulogic_vector(31 downto 0);
  signal mem_we        : std_ulogic;
  signal mem_complete  : std_ulogic;
  signal executed_addr : unsigned(15 downto 0);

  signal ext_mem_addr  : unsigned(31 downto 0);
  signal ext_mem_dout  : std_ulogic_vector(31 downto 0);
  signal ext_mem_din   : std_ulogic_vector(31 downto 0);
  signal ext_mem_rw    : std_ulogic;
  signal ext_mem_en    : std_ulogic;
  signal ext_mem_stall : std_ulogic;

  signal ext_mem : mem_t(1023 downto 0);
  signal mem_we_2 : std_ulogic;

begin  -- architecture Behav

  sl_processor_1: entity work.sl_processor
    port map (
      clk_i           => sl_clk,
      reset_n_i       => reset_n,
      reset_core_n_i  => reset_core_n,
      code_addr_o     => code_addr,
      code_data_i     => code_data,
      ext_mem_addr_o  => ext_mem_addr,
      ext_mem_dout_o  => ext_mem_dout,
      ext_mem_din_i   => ext_mem_din,
      ext_mem_rw_o    => ext_mem_rw,
      ext_mem_en_o    => ext_mem_en,
      ext_mem_stall_i => ext_mem_stall,
      mem_addr_i      => mem_addr,
      mem_din_i       => mem_din,
      mem_dout_o      => mem_dout,
      mem_we_i        => mem_we_2,
      mem_complete_o  => mem_complete,
      executed_addr_o => executed_addr);

  mem_we_2 <= '1' when mem_we = '1' and mem_addr < to_unsigned(512,16) else '0';

  -- clock generation
  clk <= not clk after 10 ns;

  code_data <= code_mem(to_integer(code_addr));

  process (clk, reset_n) is
  begin  -- process
    if reset_n = '0' then               -- asynchronous reset (active low)
      ext_mem_din <= (others => '0');
    elsif clk'event and clk = '1' then  -- rising clock edge
      if ext_mem_en = '1' then
        if ext_mem_rw = '1' then
          ext_mem(to_integer(ext_mem_addr)) <= ext_mem_dout;
        else
          ext_mem_din <= ext_mem(to_integer(ext_mem_addr));
        end if;
      end if;

      if mem_addr >= to_unsigned(512,16) and mem_we = '1' then
        ext_mem(to_integer(mem_addr)) <= mem_din;        
      end if;
    end if;
  end process;

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
    reset_core_n <= '0';

    mem_addr <= to_unsigned(0,16);
    mem_din <= (others => '0');
    mem_we <= '0';

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

          for i in 0 to code_mem'length-1 loop
            code_mem(i) <= X"FFFF";
          end loop;  -- i

          reset_core_n <= '0';
          mem_din <= (others => '0');
          for i in 0 to 255 loop
            mem_addr <= to_unsigned(i,16);
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            mem_we <= '1';
            
            -- generate clock for memory
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;
            sl_clk <= '1';
            wait for 0.1 ns;
            sl_clk <= '0';
            wait for 0.1 ns;

            mem_we <= '0';
          end loop;  -- i

          write(output,LF & "  clear mem complete");

          reset_core_n <= '1';
          reset_n <= '0';
          wait for 33 ns;
          reset_n <= '1';
        when X"0001" =>      
          j := 0;
          read_ok := true;
          while read_ok and dummy /= LF loop
            hread(l,data16);
            code_mem(j) <= data16;
            read(l,dummy,read_ok);
            j:=j+1;
          end loop;
          write(output,LF & "  " & integer'image(j) & " code words loaded");
        when X"0002" | X"0005" =>
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
          ext_mem_stall <= '0';
        when X"0003" =>
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
        when X"0004" =>
          hread(l,data32);
          mem_addr <= unsigned(data32(15 downto 0));
          hread(l,data32);
          mem_din <= data32;

          wait for 1 ps;

          write(output,LF & "  write data " & to_hstring(mem_din) & " at addr " & integer'image(to_integer(mem_addr)));

          reset_core_n <= '0';

          -- generate clock for memory
          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;
          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;

          mem_we <= '1';

          wait for 1 ns;

          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;
          sl_clk <= '1';
          wait for 10 ns;
          sl_clk <= '0';
          wait for 10 ns;
          reset_core_n <= '1';

          mem_we <= '0';
        when X"000F" =>
          hread(l,data32);
          mem_addr <= unsigned(data32(15 downto 0));

          wait for 1 ps;
          -- generate clock for memory
          reset_core_n <= '0';

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
          wait for 1 ns;
          sl_clk <= '1';
          wait for 1 ns;
          sl_clk <= '0';
          wait for 1 ns;

          if mem_addr < to_unsigned(512,16) then
            mem_result := mem_dout;
          else
            mem_result := ext_mem(to_integer(mem_addr));
          end if;

          hread(l,data32);
          
          assert mem_result = data32 report LF & "  expect FAILED " & to_hstring(mem_result) & " != " & to_hstring(data32) & LF severity error;
  
          reset_core_n <= '1';
        when others => null;
      end case;
    end loop;

    file_close(test_file);

    wait;
    
  end process;

end architecture Behav;
