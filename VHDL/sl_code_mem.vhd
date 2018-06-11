library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_p.all;

entity sl_code_mem is
  
  generic (
    SizeInKBytes : natural := 8);

  port (
    clk_i      : in  std_ulogic;
    reset_n_i  : in  std_ulogic;

    p0_addr_i    : in  unsigned(15 downto 0);
    p0_code_o    : out std_ulogic_vector(15 downto 0);
    p0_re_i      : in  std_ulogic;
    p0_reset_n_i : in  std_ulogic;
    p1_addr_i    : in  unsigned(15 downto 0);
    p1_code_o    : out std_ulogic_vector(15 downto 0);
    p1_re_i      : in  std_ulogic;
    p1_reset_n_i : in  std_ulogic;
    p2_addr_i    : in  unsigned(15 downto 0);
    p2_code_o    : out std_ulogic_vector(15 downto 0);
    p2_re_i      : in  std_ulogic;
    p2_reset_n_i : in  std_ulogic;
    p3_addr_i    : in  unsigned(15 downto 0);
    p3_code_o    : out std_ulogic_vector(15 downto 0);
    p3_re_i      : in  std_ulogic;
    p3_reset_n_i : in  std_ulogic;
    
    wb_slave_i : in  wb_slave_ifc_in_t;
    wb_slave_o : out wb_slave_ifc_out_t);
    
end entity sl_code_mem;

architecture rtl of sl_code_mem is

  signal mem0_addr : unsigned(15 downto 0);
  signal mem1_addr : unsigned(15 downto 0);
  signal mem0_we : std_ulogic;
  signal state : std_ulogic;
  signal ack : std_ulogic;

  signal raddr0 : unsigned(15 downto 0);
  signal raddr1 : unsigned(15 downto 0);
  signal rdata0 : std_ulogic_vector(15 downto 0);
  signal rdata1 : std_ulogic_vector(15 downto 0);

  signal p2_addr : unsigned(15 downto 0);
  signal p3_addr : unsigned(15 downto 0);
  signal p2_addr2 : unsigned(15 downto 0);
  signal p3_addr2 : unsigned(15 downto 0);

  type mem_t is array (natural range <>) of std_ulogic_vector(15 downto 0);
  signal mem : mem_t(SizeInKBytes*512-1 downto 0);

  signal code_data_at_0 : std_ulogic_vector(15 downto 0);
  signal load_code_data_0 : std_ulogic;

begin  -- architecture rtl

  process (clk_i) is
  begin  -- process
   if rising_edge(clk_i) then  -- rising clock edge
     if mem0_we = '1' then
       mem(to_integer(mem0_addr)) <= wb_slave_i.dat(15 downto 0);
     end if;
     raddr0 <= mem0_addr;
     raddr1 <= mem1_addr;
   end if;
  end process;

  rdata0 <= mem(to_integer(raddr0));
  rdata1 <= mem(to_integer(raddr1));

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= '0';
      ack <= '0';
      p0_code_o <= (others => '0');
      p1_code_o <= (others => '0');
      load_code_data_0 <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      
      if state = '0' then
        if p0_re_i = '1' then
          p0_code_o <= rdata0;
        end if;
        if p1_re_i = '1' then
          p1_code_o <= rdata1;
        end if;
      end if;

      if p0_reset_n_i = '0' or load_code_data_0 = '1' then
        p0_code_o <= code_data_at_0;
      end if;
      
      if p1_reset_n_i = '0' or load_code_data_0 = '1' then
        p1_code_o <= code_data_at_0;
      end if;
      
      load_code_data_0 <= '0';
      if mem0_we = '1' and mem0_addr = to_unsigned(0,16) then
        code_data_at_0 <= wb_slave_i.dat(15 downto 0);
        load_code_data_0 <= '1';
      end if;

      state <= not state;

      ack <= '0';
      if wb_slave_i.cyc = '1' and wb_slave_i.stb = '1' and wb_slave_i.we = '1' and state = '1' then
        ack <= '1';
      end if;
      
    end if;
  end process;

  process (ack, p0_addr_i, p1_addr_i, p2_addr_i, p3_addr_i, state, wb_slave_i) is
  begin  -- process
    mem0_we <= '0';
    mem0_addr <= p0_addr_i;
    mem1_addr <= p1_addr_i;

    if state = '0' then
      mem0_addr <= p2_addr_i;
      mem1_addr <= p3_addr_i;
    end if;

    if wb_slave_i.cyc = '1' and wb_slave_i.stb = '1' and wb_slave_i.we = '1' then
      mem0_addr <= wb_slave_i.adr(15 downto 0);
      mem0_we <= '1';
    end if;

    wb_slave_o.ack <= ack;
    wb_slave_o.stall <= not ack;
    wb_slave_o.err <= '0';
    wb_slave_o.dat <= (others => '0');
    
  end process;

  p2_code_o <= rdata0;
  p3_code_o <= rdata1;

end architecture rtl;
