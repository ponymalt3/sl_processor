library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.sl_structs_p.all;
use work.qfp32_unit_p.all;
use work.qfp32_misc_p.all;
use work.qfp32_add_p.all;
use work.qfp_p.all;

entity sl_processor is
  
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    reset_core_n_i : in std_ulogic;

    code_addr_o : out unsigned(15 downto 0);
    code_data_i : in std_ulogic_vector(15 downto 0);

    ext_mem_addr_o : out unsigned(31 downto 0);
    ext_mem_dout_o : out std_ulogic_vector(31 downto 0);
    ext_mem_din_i : in std_ulogic_vector(31 downto 0);
    ext_mem_rw_o : out std_ulogic;
    ext_mem_en_o : out std_ulogic;
    ext_mem_stall_i : in std_ulogic;

    mem_addr_i : in unsigned(15 downto 0);
    mem_din_i : in std_ulogic_vector(31 downto 0);
    mem_dout_o : out std_ulogic_vector(31 downto 0);
    mem_we_i : in std_ulogic;
    mem_complete_o : out std_ulogic;

    executed_addr_o : out unsigned(15 downto 0));

end entity sl_processor;

architecture rtl of sl_processor is

  signal clk2 : std_ulogic;
  signal reset_core_n : std_ulogic;

  signal alu_en        : std_ulogic;
  signal alu_cmd       : std_ulogic_vector(2 downto 0);
  signal alu_op_a      : reg_raw_t;
  signal alu_op_b      : reg_raw_t;
  signal alu_data      : sl_alu_t;
  signal cp_addr       : reg_pc_t;
  signal ext_mem_addr  : reg_addr_t;
  signal ext_mem_dout  : reg_raw_t;
  signal ext_mem_din   : reg_raw_t;
  signal ext_mem_rw    : std_ulogic;
  signal ext_mem_en    : std_ulogic;
  signal ext_mem_stall : std_ulogic;
  signal rp0_addr      : reg_addr_t;
  signal rp0_dout       : reg_raw_t;
  signal rp1_addr      : reg_addr_t;
  signal rp1_dout       : reg_raw_t;
  signal wp_addr       : reg_addr_t;
  signal wp_din       : reg_raw_t;
  signal wp_we         : std_ulogic;

  signal rp0_addr_vec      : std_ulogic_vector(15 downto 0);
  signal rp1_addr_vec      : std_ulogic_vector(15 downto 0);
  signal wp_addr_vec      : std_ulogic_vector(15 downto 0);
  signal mem_addr_vec   : std_ulogic_vector(15 downto 0);

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

begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      clk2 <= '1';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if reset_core_n_i = '1' then
        clk2 <= not clk2;
      end if;
    end if;
  end process;

  reset_core_n <= reset_n_i;

  sl_core_1: entity work.sl_core
    port map (
      clk_i           => clk2,
      reset_n_i       => reset_core_n,
      alu_en_o        => alu_en,
      alu_cmd_o       => alu_cmd,
      alu_op_a_o      => alu_op_a,
      alu_op_b_o      => alu_op_b,
      alu_i           => alu_data,
      cp_addr_o       => cp_addr,
      cp_din_i        => code_data_i,
      ext_mem_addr_o  => ext_mem_addr_o,
      ext_mem_dout_o  => ext_mem_dout_o,
      ext_mem_din_i   => ext_mem_din_i,
      ext_mem_rw_o    => ext_mem_rw_o,
      ext_mem_en_o    => ext_mem_en_o,
      ext_mem_stall_i => ext_mem_stall_i,
      rp0_addr_o      => rp0_addr,
      rp0_din_i       => rp0_dout,
      rp1_addr_o      => rp1_addr,
      rp1_din_i       => rp1_dout,
      wp_addr_o       => wp_addr,
      wp_dout_o       => wp_din,
      wp_we_o         => wp_we,
      executed_addr_o => executed_addr_o);

  code_addr_o <= unsigned(cp_addr);

  rp0_addr_vec <= To_StdULogicVector(std_logic_vector(rp0_addr(15 downto 0)));
  rp1_addr_vec <= To_StdULogicVector(std_logic_vector(rp1_addr(15 downto 0)));
  wp_addr_vec <= To_StdULogicVector(std_logic_vector(wp_addr(15 downto 0)));
  mem_addr_vec <= To_StdULogicVector(std_logic_vector(mem_addr_i));

  multi_port_mem_1: entity work.multi_port_mem
    generic map (
      SizeInKBytes => 4)
    port map (
      clk_i         => clk_i,
      reset_n_i     => reset_n_i,
      r0_addr_i     => wp_addr_vec,
      r0_din_i      => wp_din,
      r0_we_i       => wp_we,
      r0_idle_o     => open,
      f0_addr_i     => rp0_addr_vec,
      f0_dout_o     => rp0_dout,
      f0_complete_o => open,
      r1_addr_i     => mem_addr_vec,
      r1_din_i      => mem_din_i,
      r1_dout_o     => mem_dout_o,
      r1_we_i       => mem_we_i,
      r1_complete_o => mem_complete_o,
      f1_addr_i     => rp1_addr_vec,
      f1_dout_o     => rp1_dout,
      f1_complete_o => open);

  qfp_unit_1: entity work.qfp_unit
    generic map (
      config => qfp_config_add+qfp_config_mul+qfp_config_div)
    port map (
      clk_i      => clk2,
      reset_n_i  => reset_n_i,
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

  process (alu_cmd, multi_cycle_op, qfp_cmp_gt, qfp_cmp_z,
           qfp_complete, qfp_idle, qfp_int_result, qfp_ready, qfp_result) is
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
     when others => qfp_cmd <= (QFP_UNIT_NONE,"00");
   end case;

  end process;

  process (clk2, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      qfp_idle <= '1';
    elsif clk2'event and clk2 = '1' then  -- rising clock edge
      if qfp_ready = '1' and alu_en2 = '1' and alu_cmd /= CMD_MOV then
        qfp_idle <= '0';
      end if;
      if qfp_complete = '1' then
        qfp_idle <= '1';
      end if;
    end if;
  end process;

end architecture rtl;
