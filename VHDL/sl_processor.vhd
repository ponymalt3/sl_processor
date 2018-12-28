library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.sl_structs_p.all;
use work.qfp32_unit_p.all;
use work.qfp32_misc_p.all;
use work.qfp32_add_p.all;
use work.qfp_p.all;
use work.wishbone_p.all;

entity sl_processor is

  generic (
    LocalMemSizeInKB : natural := 4;
    CodeStartAddr  : natural := 0;
    EnableDebugMemPort : boolean := false);
  
  port (
    clk_i     : in std_ulogic;
    mem_clk_i : in std_ulogic;
    reset_n_i : in std_ulogic;

    core_en_i : in std_ulogic;
    core_reset_n_i : in std_ulogic;

    code_addr_o : out unsigned(15 downto 0);
    code_re_o : out std_ulogic;
    code_data_i : in std_ulogic_vector(15 downto 0);

    ext_master_i : in  wb_master_ifc_in_t;
    ext_master_o : out wb_master_ifc_out_t;

    debug_slave_i : in  wb_slave_ifc_in_t;
    debug_slave_o : out wb_slave_ifc_out_t;

    executed_addr_o : out unsigned(15 downto 0));

end entity sl_processor;

architecture rtl of sl_processor is

  signal core_clk : std_ulogic;
  signal core_en_1d : std_ulogic;

  signal alu_en        : std_ulogic;
  signal alu_cmd       : std_ulogic_vector(3 downto 0);
  signal alu_op_a      : reg_raw_t;
  signal alu_op_b      : reg_raw_t;
  signal alu_data      : sl_alu_t;
  signal cp_addr       : reg_pc_t;
  signal rp0_addr      : reg_addr_t;
  signal rp0_dout      : reg_raw_t;
  signal rp0_en        : std_ulogic;
  signal rp1_addr      : reg_addr_t;
  signal rp1_dout      : reg_raw_t;
  signal wp_addr       : reg_addr_t;
  signal wp_din        : reg_raw_t;
  signal wp_we         : std_ulogic;

  signal qfp_cmd      : qfp_cmd_t;
  signal qfp_ready   : std_ulogic;
  signal qfp_result   : std_ulogic_vector(31 downto 0);
  signal qfp_cmp_gt   : std_ulogic;
  signal qfp_cmp_z   : std_ulogic;
  signal qfp_complete : std_ulogic;
  signal qfp_int_result   : std_ulogic_vector(31 downto 0);
  signal qfp_idle : std_ulogic;

  signal alu_en2 : std_ulogic;
  signal multi_cycle_op : std_ulogic;

  signal rp_stall : std_ulogic;

  signal ext_mem_addr : unsigned(31 downto 0);
  signal ext_mem_dout : std_ulogic_vector(31 downto 0);
  signal ext_mem_din : std_ulogic_vector(31 downto 0);
  signal ext_mem_rw : std_ulogic;
  signal ext_mem_en : std_ulogic;
  signal ext_mem_stall : std_ulogic;

  constant InternalMemAddrWidth : natural := log2(LocalMemSizeInKB*1024/4);
  
  signal mem_we : std_ulogic;
  signal mem_addr : unsigned(InternalMemAddrWidth-1 downto 0);
  signal mem1_we : std_ulogic;
  signal mem1_addr : unsigned(InternalMemAddrWidth-1 downto 0);
  signal mem_complete : std_ulogic;
  signal mem_slave_ack : std_ulogic;

  signal ext_mem_disable : std_ulogic;

begin  -- architecture rtl

  sl_core_1: entity work.sl_core
    generic map (
      ExtAddrThreshold => LocalMemSizeInKB*(1024/4),
      CodeStartAddr  => CodeStartAddr)
    port map (
      clk_i           => clk_i,
      reset_n_i       => core_reset_n_i,
      en_i            => core_en_i,
      alu_en_o        => alu_en,
      alu_cmd_o       => alu_cmd,
      alu_op_a_o      => alu_op_a,
      alu_op_b_o      => alu_op_b,
      alu_i           => alu_data,
      cp_addr_o       => cp_addr,
      cp_re_o         => code_re_o,
      cp_din_i        => code_data_i,
      ext_mem_addr_o  => ext_mem_addr,
      ext_mem_dout_o  => ext_mem_dout,
      ext_mem_din_i   => ext_mem_din,
      ext_mem_rw_o    => ext_mem_rw,
      ext_mem_en_o    => ext_mem_en,
      ext_mem_stall_i => ext_mem_stall,
      rp0_addr_o      => rp0_addr,
      rp0_din_i       => rp0_dout,
      rp0_en_o        => rp0_en,
      rp1_addr_o      => rp1_addr,
      rp1_din_i       => rp1_dout,
      rp_stall_o      => rp_stall,
      wp_addr_o       => wp_addr,
      wp_dout_o       => wp_din,
      wp_we_o         => wp_we,
      executed_addr_o => executed_addr_o);

  code_addr_o <= unsigned(cp_addr);

  sl_dpram_1: entity work.sl_dpram
    generic map (
      SizeInBytes         => LocalMemSizeInKB*1024,
      SizeOfElementInBits => 32)
    port map (
      clk_i     => clk_i,
      reset_n_i => reset_n_i,
      p0_addr_i => mem_addr,
      p0_din_i  => wp_din,
      p0_dout_o => rp0_dout,
      p0_we_i   => wp_we,
      p1_addr_i => mem1_addr,
      p1_din_i  => debug_slave_i.dat,
      p1_dout_o => rp1_dout,
      p1_we_i   => mem1_we);

  mem_addr <= wp_addr(InternalMemAddrWidth-1 downto 0) when wp_we = '1' else rp0_addr(InternalMemAddrWidth-1 downto 0);
  mem_we <= wp_we;

  mem1_addr <= rp1_addr(InternalMemAddrWidth-1 downto 0) when not EnableDebugMemPort or core_en_i = '1' else debug_slave_i.adr(InternalMemAddrWidth-1 downto 0);
  mem1_we <= '1' when EnableDebugMemPort and debug_slave_i.cyc = '1' and debug_slave_i.stb = '1' and debug_slave_i.we = '1' else '0';
  debug_slave_o.dat <= rp1_dout when EnableDebugMemPort else (others => '0');

  debug_slave_o.ack <= mem_slave_ack;
  debug_slave_o.stall <= not mem_slave_ack;
  debug_slave_o.err <= to_ulogic(not EnableDebugMemPort);

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      mem_slave_ack <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      mem_complete <= debug_slave_i.cyc and debug_slave_i.stb;
      if EnableDebugMemPort and debug_slave_i.cyc = '1' and debug_slave_i.stb = '1' and mem_complete = '0' then
        mem_slave_ack <= '1';
      else
        mem_slave_ack <= '0';
      end if;
    end if;
  end process;

  qfp_unit_1: entity work.qfp_unit
    generic map (
      config => qfp_config_add+qfp_config_mul+qfp_config_div+qfp_config_math)
    port map (
      clk_i      => clk_i,
      reset_n_i  => core_reset_n_i,
      en_i       => core_en_i,
      cmd_i      => qfp_cmd,
      ready_o    => qfp_ready,
      start_i    => alu_en2,
      regA_i     => alu_op_a,
      regB_i     => alu_op_b,
      result_o   => qfp_result,
      cmp_gt_o   => qfp_cmp_gt,
      cmp_z_o    => qfp_cmp_z,
      complete_o => qfp_complete);

  qfp_int_result <= "000" & To_StdULogicVector(std_logic_vector(fast_shift(unsigned(alu_op_a(28 downto 0)),to_integer(unsigned(not alu_op_a(30 downto 29)))*8,'1')));

  alu_en2 <= '1' when alu_en = '1' and alu_cmd /= CMD_CMP else '0'; 

  process (alu_cmd, multi_cycle_op, qfp_cmp_gt, qfp_cmp_z, qfp_complete,
           qfp_idle, qfp_int_result, qfp_ready, qfp_result) is
  begin  -- process
   alu_data.result <= qfp_result;
   alu_data.int_result <= qfp_int_result;
   alu_data.complete <= qfp_complete;
   alu_data.same_unit_ready <= qfp_ready;
   alu_data.idle <= qfp_idle and not multi_cycle_op;
   alu_data.cmp_lt <= not qfp_cmp_gt and not qfp_cmp_z;
   alu_data.cmp_eq <= qfp_cmp_z;

   multi_cycle_op <= '0';
   case alu_cmd is
     when CMD_MOV => qfp_cmd <= (QFP_UNIT_NONE,"00");
     when CMD_SUB => qfp_cmd <= (QFP_UNIT_ADD,QFP_SCMD_SUB); multi_cycle_op <= '1';
     when CMD_ADD => qfp_cmd <= (QFP_UNIT_ADD,QFP_SCMD_ADD); multi_cycle_op <= '1';
     when CMD_MUL => qfp_cmd <= (QFP_UNIT_MUL,"00"); multi_cycle_op <= '1';
     when CMD_DIV => qfp_cmd <= (QFP_UNIT_DIV,"00"); multi_cycle_op <= '1';
     when CMD_LOG2 => qfp_cmd <= (QFP_UNIT_MATH,"00");
     when CMD_SHFT => qfp_cmd <= (QFP_UNIT_MATH,"01");
     when others => qfp_cmd <= (QFP_UNIT_NONE,"00");
   end case;

  end process;

  process (clk_i, core_reset_n_i) is
  begin  -- process
    if core_reset_n_i = '0' then             -- asynchronous reset (active low)
      qfp_idle <= '1';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if core_en_i = '1' then
        if qfp_ready = '1' and alu_en2 = '1' and qfp_cmd.unit /= QFP_UNIT_NONE then
          qfp_idle <= '0';
        end if;
        if qfp_complete = '1' then
          qfp_idle <= '1';
        end if;
      end if;
    end if;
  end process;

  ext_master_o <= (ext_mem_addr,ext_mem_dout,ext_mem_rw,"1111",ext_mem_en and not ext_mem_disable,ext_mem_en);
  ext_mem_din <= ext_master_i.dat;

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      ext_mem_disable <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if ext_master_i.stall = '0' then
        ext_mem_disable <= '1';
      end if;

      if ext_master_i.ack = '1' or ext_master_i.err = '1' then
        ext_mem_disable <= '0';
      end if;
    end if;
  end process;

  ext_mem_stall <= not ext_master_i.ack and ext_mem_en;
  
end architecture rtl;
