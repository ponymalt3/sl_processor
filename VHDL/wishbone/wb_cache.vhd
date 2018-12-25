library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;
use work.sl_misc_p.all;

entity wb_cache is
  generic (
    WordsPerLine  : natural := 8;
    NumberOfLines : natural := 48;
    WriteTrough   : boolean := false);
  
  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;
    
    addr_i     : in    unsigned(31 downto 0);
    din_i      : in    std_ulogic_vector(31 downto 0);
    dout_o     : out   std_ulogic_vector(31 downto 0);
    en_i       : in    std_ulogic;
    we_i       : in    std_ulogic;
    complete_o : out   std_ulogic;
    err_o      : out   std_ulogic;

    snooping_addr_i : in unsigned(31 downto 0);
    snooping_en_i : std_ulogic;
    
    master_out_i : in wb_master_ifc_in_t;
    master_out_o : out wb_master_ifc_out_t);

end entity wb_cache;

architecture rtl of wb_cache is

  constant WordIndexBits : natural := log2(WordsPerLine);
  constant LineIndexBits : natural := log2(NumberOfLines);
  constant MaxCount : unsigned(WordIndexBits-1 downto 0) := to_unsigned(2**WordsPerLine-1,WordIndexBits);
  constant BurstSize : unsigned(5 downto 0) := to_unsigned(to_integer(MaxCount),6);

  type state_t is (ST_IDLE,ST_WRITE_TROUGH,ST_FETCH,ST_FETCH_AFTER_WB,ST_WRITEBACK_PRE,ST_WRITEBACK,ST_WRITEBACK_WAIT);

  signal state : state_t;
  signal count : unsigned(WordIndexBits-1 downto 0);
  signal tag_init_complete : std_ulogic;

  signal mem_addr : unsigned(LineIndexBits+WordIndexBits-1 downto 0);
  signal mem_din : std_ulogic_vector(31 downto 0);
  signal mem_we : std_ulogic;
  signal mem1_addr : unsigned(LineIndexBits+WordIndexBits-1 downto 0);
  signal mem1_din : std_ulogic_vector(31 downto 0);
  signal mem1_we : std_ulogic;
  signal mem1_dout : std_ulogic_vector(31 downto 0);
  signal mem_write_en : std_ulogic;
  signal mem1_write_en : std_ulogic;
  signal mem1_addr_ov_bit : std_ulogic;

  signal cache_hit : std_ulogic;
  signal wb_fetch : std_ulogic;
  signal wb_writeback : std_ulogic;
  
  signal tag_index : unsigned(LineIndexBits-1 downto 0); 
  signal tag_in : std_ulogic_vector(31 downto 0);
  signal tag_we : std_ulogic;
  signal tag_out : std_ulogic_vector(31 downto 0);
  signal tag1_re_and_invalidate : std_ulogic;
  signal tag1_we : std_ulogic;
  signal tag1_in : std_ulogic_vector(31 downto 0);
  signal tag1_out : std_ulogic_vector(31 downto 0);
  signal tag1_index : unsigned(LineIndexBits-1 downto 0);
  signal inv_addr : unsigned(31-LineIndexBits downto 0);

  signal wb_en : std_ulogic;
  signal wb_we : std_ulogic;
  signal wb_addr : unsigned(31 downto 0);
  signal wb_din : std_ulogic_vector(31 downto 0);
  signal wb_dout : std_ulogic_vector(31 downto 0);
  signal wb_complete : std_ulogic;
  signal wb_dready : std_ulogic;
  signal wb_err : std_ulogic;
  signal wb_burst : unsigned(5 downto 0);

  signal fetch_ignore_counter : unsigned(WordIndexBits-1 downto 0);
  signal fetch_ignore_addr : unsigned(WordIndexBits-1 downto 0);
  signal line_active : std_ulogic;
  signal active_line_addr : unsigned(31 downto 0);
  signal en_and_line_inactive : std_ulogic;
  signal we_1d : std_ulogic;
  signal pending_write : std_ulogic;
  signal pending_write_1d : std_ulogic;
  signal addr_ov_bit : std_ulogic;

  signal mem_clk : std_ulogic;
  signal xxx : std_ulogic;

begin

  mem_clk <= not clk_i;

  m9k_1: entity work.m9k_2
    generic map (
      SizeInBytes => NumberOfLines*4,
      SizeOfElementInBits => 32)
    port map (
      clk_i     => mem_clk,
      reset_n_i => reset_n_i,
      p0_addr_i => tag_index,
      p0_din_i  => tag_out,
      p0_dout_o => tag_in,
      p0_we_i   => tag_we,
      p1_addr_i => tag1_index,
      p1_din_i  => tag1_out,
      p1_dout_o => tag1_in,
      p1_we_i   => tag1_we);

    m9k_2: entity work.m9k_2
    generic map (
      SizeInBytes => NumberOfLines*WordsPerLine*4,
      SizeOfElementInBits => 32)
    port map (
      clk_i     => mem_clk,
      reset_n_i => reset_n_i,
      p0_addr_i => mem_addr(LineIndexBits+WordIndexBits-1 downto 0),
      p0_din_i  => mem_din,
      p0_dout_o => dout_o,
      p0_we_i   => mem_we,
      p1_addr_i => mem1_addr(LineIndexBits+WordIndexBits-1 downto 0),
      p1_din_i  => mem1_din,
      p1_dout_o => mem1_dout,
      p1_we_i   => mem1_we);

  mem_we <= mem_write_en;
  mem1_din <= wb_dout when state = ST_FETCH or state = ST_FETCH_AFTER_WB else mem_din;
  mem1_we <= '1' when mem1_write_en = '1' or ((state = ST_FETCH or state = ST_FETCH_AFTER_WB) and fetch_ignore_counter = to_unsigned(0,WordIndexBits)) else '0';
  
  process (clk_i, reset_n_i) is
    variable mem1_addr_word_next : unsigned(WordIndexBits downto 0);
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= ST_IDLE;
      count <= to_unsigned(0,WordIndexBits);
      tag_init_complete <= '0';
      tag_out <= (others => '0');
      tag_we <= '0';
      tag1_re_and_invalidate <= '0';
      tag1_out <= (others => '0');
      tag1_we <= '1';
      tag1_index <= to_unsigned(0,LineIndexBits);
      inv_addr <= to_unsigned(0,32-LineIndexBits);
      mem_din <= (others => '0');
      mem1_addr <= to_unsigned(0,LineIndexBits+WordIndexBits);
      mem_write_en <= '0';
      mem1_write_en <= '0';
      active_line_addr <= to_unsigned(0,32);
      fetch_ignore_counter <= to_unsigned(0,WordIndexBits);
      fetch_ignore_addr <= to_unsigned(0,WordIndexBits);
      mem1_addr_ov_bit <= '0';
      pending_write_1d <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge

      pending_write_1d <= pending_write;
      mem_din <= din_i;

      if state = ST_IDLE then
        mem1_addr <= mem_addr;
        mem1_addr_ov_bit <= '0';
        active_line_addr <= addr_i;
        fetch_ignore_counter <= to_unsigned(0,WordIndexBits);
        if WriteTrough and en_i = '1' and we_i = '1' then
          state <= ST_WRITE_TROUGH;
        end if;
        if wb_writeback = '1' then
          fetch_ignore_addr <= mem_addr(WordIndexBits-1 downto 0);
          if we_i = '1' then
            fetch_ignore_counter <= to_unsigned(1,WordIndexBits);
            fetch_ignore_addr <= mem_addr(WordIndexBits-1 downto 0)+1;
          end if;
          state <= ST_WRITEBACK_PRE;
          mem1_addr <= unsigned(tag_in(LineIndexBits+WordIndexBits-1 downto WordIndexBits)) & mem_addr(WordIndexBits-1 downto 0);
          count <= MaxCount;
        elsif wb_fetch = '1' then
          if we_i = '1' then
            fetch_ignore_counter <= to_unsigned(1,WordIndexBits);
          end if;
          state <= ST_FETCH;
          mem1_addr <= mem_addr;
          count <= MaxCount;
        end if;
      elsif state = ST_WRITE_TROUGH and wb_complete = '1' then
        state <= ST_IDLE;
      elsif state = ST_WRITEBACK_PRE then
        state <= ST_WRITEBACK;
      elsif state = ST_WRITEBACK_WAIT and wb_complete = '1' then
        state <= ST_FETCH_AFTER_WB;
      elsif wb_dready = '1' then -- state /= ST_IDLE
        count <= count-1;
        mem1_addr_word_next := ('0' & mem1_addr(WordIndexBits-1 downto 0))+1;
        mem1_addr_ov_bit <= mem1_addr_ov_bit or mem1_addr_word_next(WordIndexBits);
        mem1_addr(WordIndexBits-1 downto 0) <= mem1_addr_word_next(WordIndexBits-1 downto 0);
        if (state = ST_FETCH or state = ST_FETCH_AFTER_WB) and fetch_ignore_counter /= to_unsigned(0,WordIndexBits) then
          fetch_ignore_counter <= fetch_ignore_counter-1;
        end if;        
        if count = to_unsigned(0,WordIndexBits) then
          case state is
            when ST_WRITEBACK =>
              count <= MaxCount;
              state <= ST_WRITEBACK_WAIT;
              mem1_addr_ov_bit <= '0';
            when ST_FETCH | ST_FETCH_AFTER_WB =>
              assert wb_complete = '1' report "master does not behave as expected" severity error;
              state <= ST_IDLE;
            when others => null;
          end case;
        end if;
      end if;

      if state = ST_WRITEBACK or state = ST_WRITEBACK_WAIT then
        if en_i = '1' and we_i = '1' then
          --fetch_ignore_counter <= fetch_ignore_counter+1;
          fetch_ignore_addr <= fetch_ignore_addr+1;
        end if;
      end if;

      -- mem write
      mem_write_en <= '0';
      mem1_write_en <= '0';
      if cache_hit = '1' and we_i = '1' and line_active = '0' then
        if state = ST_IDLE then
          mem1_write_en <= '1';
        elsif mem_write_en /= '1' then
          mem_write_en <= '1';
        end if;
      end if;

      -- tag control
      if state = ST_FETCH and wb_dready = '1' and count = MaxCount then
        -- update on fetch
        tag_out <= to_stdUlogicVector(std_logic_vector(addr_i(31 downto 2))) & we_1d & '1';
        tag_we <= '1';
      elsif state = ST_IDLE and wb_writeback = '1' then
        -- update on fetch for write
        tag_out <= to_stdUlogicVector(std_logic_vector(addr_i(31 downto 2))) & we_1d & '1';
        tag_we <= '1';
      else
        tag_we <= '0';
      end if;

      if tag_init_complete = '0' then
        -- initialize
        tag1_we <= '1';
        tag1_index <= tag1_index+1;
        if tag1_index = to_unsigned(NumberOfLines-1,LineIndexBits) then
          tag_init_complete <= '1';
        end if;
      elsif WriteTrough then
        -- bus snooping
        tag1_we <= '0';
        if tag1_re_and_invalidate = '1' then
          if unsigned(tag1_in(31 downto LineIndexBits)) = inv_addr then
            tag1_we <= '1';
          end if;
          tag1_re_and_invalidate <= '0';
        elsif snooping_en_i = '1' then
          inv_addr <= snooping_addr_i(31 downto LineIndexBits);
          tag1_index <= snooping_addr_i(LineIndexBits+WordIndexBits-1 downto WordIndexBits);
          tag1_out <= (others => '0');
          tag1_re_and_invalidate <= '1';
        end if;
      elsif cache_hit = '1' and we_i = '1' then
         -- mark as dirty
        tag1_out <= tag_in(31 downto 2) & '1' & tag_in(0);
        tag1_we <= '1';
        tag1_index <= tag_index;
      else
        tag1_we <= '0';
      end if;
      
    end if;
  end process;

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      en_and_line_inactive <= '0';
      we_1d <= '0';
    elsif clk_i'event and clk_i = '0' then  -- falling clock edge
      en_and_line_inactive <= en_i and not line_active;
      we_1d <= we_i;
    end if;
  end process;

  process (active_line_addr, addr_i, addr_ov_bit, cache_hit, count, din_i,
           en_and_line_inactive, en_i, mem1_addr(WordIndexBits-1 downto 0),
           mem1_addr_ov_bit, mem1_dout, mem1_write_en, mem_write_en,
           pending_write, pending_write_1d, state, tag_in(0), tag_in(1),
           tag_in(31 downto WordIndexBits), tag_index, wb_complete, wb_fetch,
           wb_writeback, we_1d, we_i, xxx) is
  begin  -- process

    mem_addr <= tag_index & addr_i(WordIndexBits-1 downto 0);
    tag_index <= addr_i(LineIndexBits+WordIndexBits-1 downto WordIndexBits);

    cache_hit <= '0';
    if addr_i(31 downto WordIndexBits) = unsigned(tag_in(31 downto WordIndexBits)) and tag_in(0) = '1' then
      cache_hit <= '1'; -- match
    end if;

    wb_fetch <= en_i and not pending_write and not cache_hit and not tag_in(1); -- clean
    wb_writeback <= en_i and not pending_write and not cache_hit and tag_in(1); -- dirty

    if WriteTrough then
      wb_fetch <= en_i and not we_i and not cache_hit;
      wb_writeback <= '0';
    end if;

    wb_en <= '0';
    wb_we <= '0';
    wb_addr <= to_unsigned(0,32);
    wb_din <= mem1_dout;

    if WriteTrough and en_i = '1' and we_i = '1' then
      -- pass through
      wb_en <= '1';
      wb_we <= '1';
      wb_addr <= addr_i;
      wb_din <= din_i;
    elsif not WriteTrough and wb_writeback = '1' and state = ST_WRITEBACK_PRE then
      -- trigger writeback
      wb_en <= '1';
      wb_we <= '1';
      wb_addr <= unsigned(tag_in(31 downto WordIndexBits)) & addr_i(WordIndexBits-1 downto 0);
    elsif (wb_fetch = '1' and state = ST_IDLE) then
       -- trigger fetch
      wb_en <= '1';
      wb_we <= '0';
      wb_addr <= addr_i;
    elsif (wb_complete = '1' and (state = ST_WRITEBACK or state = ST_WRITEBACK_WAIT)) then
       -- trigger fetch after wb
      wb_en <= '1';
      wb_we <= '0';
      wb_addr <= active_line_addr;
    end if;

    xxx <= '0';
    if active_line_addr(LineIndexBits-1 downto WordIndexBits) = addr_i(LineIndexBits-1 downto WordIndexBits) then
      xxx <= '1';
    end if;

    -- check if pending write conflicts with read
    pending_write <= '0';
    if state = ST_IDLE and pending_write_1d = '0' and mem1_write_en = '1' and xxx = '1' then
      pending_write <= '1';
    end if;

    addr_ov_bit <= '0';
    if active_line_addr(WordIndexBits-1 downto 0) > addr_i(WordIndexBits-1 downto 0) then
      addr_ov_bit <= '1';
    end if;

    -- check if current line is active (fetching/writing back)
    line_active <= '0'; 
    if state /= ST_IDLE and addr_i(31 downto WordIndexBits) = active_line_addr(31 downto WordIndexBits) then
-- maybe entry better name    
      line_active <= '1';

      if WriteTrough and state = ST_WRITE_TROUGH then
        line_active <= '0';
      end if;

      if (addr_ov_bit & addr_i(WordIndexBits-1 downto 0)) < (mem1_addr_ov_bit & mem1_addr(WordIndexBits-1 downto 0)) then
        if state = ST_FETCH or state = ST_FETCH_AFTER_WB then
          line_active <= '0';
        elsif not WriteTrough and state = ST_WRITEBACK and count = MaxCount-1 and we_1d = '1' then
          line_active <= '0';
        end if;
      end if;
    end if;

    complete_o <= '0';

    --if tag_we = '1' and count = MaxCount-1 and pending_write = '0' and (state = ST_FETCH or state = ST_FETCH_AFTER_WB) then
    --  complete_o <= '1';
    --end if;
    
    if not WriteTrough and en_and_line_inactive = '1' and cache_hit = '1' and
      (we_1d = '0' or state = ST_IDLE or mem_write_en = '1') and pending_write = '0' then
      complete_o <= '1';
    end if;

    if WriteTrough and en_and_line_inactive = '1'
      and ((we_1d = '0' and cache_hit = '1' and pending_write = '0') or (we_1d = '1' and state = ST_IDLE)) then
      complete_o <= '1';
    end if;
    
  end process;

  wb_master_1: entity work.wb_master
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      addr_i       => wb_addr,
      din_i        => wb_din,
      dout_o       => wb_dout,
      en_i         => wb_en,
      burst_i      => wb_burst,
      we_i         => wb_we,
      dready_o     => wb_dready,
      complete_o   => wb_complete,
      err_o        => wb_err,
      master_out_i => master_out_i,
      master_out_o => master_out_o);

  wb_burst <= BurstSize when not WriteTrough or we_1d = '0' else to_unsigned(0,6);

end architecture rtl;
