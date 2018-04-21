library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

library work;
use work.sl_structs_p.all;
use work.sl_dec_p.all;
use work.sl_dec_ex_p.all;
use work.sl_execute_p.all;
use work.sl_control_p.all;
use work.sl_state_p.all;

entity sl_core is
  port (
    clk_i           : in std_ulogic;
    reset_n_i       : in std_ulogic;

    -- alu
    alu_en_o : out std_ulogic;
    alu_cmd_o : out std_ulogic_vector(2 downto 0);
    alu_op_a_o : out reg_raw_t;
    alu_op_b_o : out reg_raw_t;
    alu_i : in sl_alu_t;

    -- code mem
    cp_addr_o : out reg_pc_t;
    cp_din_i : in std_ulogic_vector(15 downto 0);
    
    -- external full mem interface
    ext_mem_addr_o : out reg_addr_t;
    ext_mem_dout_o : out reg_raw_t;
    ext_mem_din_i : in reg_raw_t;
    ext_mem_rw_o : out std_ulogic;
    ext_mem_en_o : out std_ulogic;
    ext_mem_stall_i : in std_ulogic;

    -- read ports (falling edge)
    rp0_addr_o : out reg_addr_t;
    rp0_din_i : in reg_raw_t;
    rp1_addr_o : out reg_addr_t;
    rp1_din_i : in reg_raw_t;
    rp_stall_o : out std_ulogic;
    
    -- write port (positive edge)
    wp_addr_o : out reg_addr_t;
    wp_dout_o : out reg_raw_t;
    wp_we_o : out std_ulogic;

    executed_addr_o : out unsigned(15 downto 0));
  
end entity sl_core;

architecture rtl of sl_core is

  signal proc : sl_processor_t;

  signal fetch_next : sl_code_fetch_t;
  signal mem1_next : sl_mem1_t;
  signal mem2_next : sl_mem2_t;
  signal dec_next : sl_decode_t;  
  signal decex_next : sl_decode_ex_t;
  signal exec_next : sl_exec_t;
  signal ctrl_next : sl_stall_ctrl_t;
  signal state_next : sl_state_t;

  signal rp0_addr : reg_addr_t;
  signal rp1_addr : reg_addr_t;

  alias mem0_reg : std_ulogic_vector(31 downto 0) is rp0_din_i;
  alias mem1_reg : std_ulogic_vector(31 downto 0) is rp1_din_i;

begin  -- architecture rtl

  process (dec_next, ctrl_next, proc, rp0_addr, rp1_addr) is
  begin  -- process
    ext_mem_addr_o <= proc.state.addr(1); -- read addr
    ext_mem_dout_o <= proc.state.result;
    ext_mem_en_o <= dec_next.mem_ex;
    ext_mem_rw_o <= '0';

    if proc.decex.wr_en = '1' and proc.decex.wr_ext = '1' and proc.state.enable(S_EXEC) = '1' then
      ext_mem_addr_o <= proc.decex.wr_addr;
      ext_mem_en_o <= '1';
      ext_mem_rw_o <= '1';
    end if;

    wp_addr_o <= proc.decex.wr_addr;
    wp_dout_o <= proc.state.result;
    wp_we_o <= proc.decex.wr_en and not proc.decex.wr_ext and proc.state.enable(S_EXEC);

    rp0_addr <= proc.state.addr(to_integer(unsigned'("" & dec_next.mux_ad0)));
    rp1_addr <= proc.state.addr(to_integer(unsigned'("" & dec_next.mux_ad1)));

    rp_stall_o <= ctrl_next.stall_decex;
      
    rp0_addr_o <= rp0_addr;
    rp1_addr_o <= rp1_addr;

    if dec_next.en_irs = '1' then
      rp1_addr_o <= X"0000" & dec_next.irs_addr;
    end if;

    cp_addr_o <= proc.state.pc;

    -- alu
    alu_en_o <= proc.state.enable(S_EXEC);
    alu_cmd_o <= proc.decex.cmd;
    
  end process;

  alu_op_a_o <= mem0_reg when proc.decex.mux0 = MUX1_MEM else proc.state.result;
  alu_op_b_o <= proc.decex.memX when proc.decex.wr_ext = '1' else mem1_reg;

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      executed_addr_o <= to_unsigned(0,16);
      proc.fetch <= ((others => '0'),to_unsigned(0,16));
      proc.dec <= ('0','0','0','0','0','0','0',(others => '0'),(others => '0'),(others => '0'),(others => '0'),'0','0','0','0','0','0','0','0','0','0',(others => '0'),'0',to_unsigned(0,16),'0',to_unsigned(0,16),'0',to_unsigned(0,16),'0','0');
      proc.decex <= ("000",(others => '0'),(others => '0'),(others => '0'),(others => '0'),(others => '0'),'0',to_unsigned(0,32),'0','0','0',"00",'0',"00",'0','0','0',(others => '0'),'0');
      proc.state <= (to_unsigned(0,16),((others => '0'),(others => '0')),to_unsigned(0,32),"001","100",'0',(others => '0'),(others => '0'),'0','0','0');  
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge

      proc.state <= state_next;

      if ctrl_next.stall_decex = '0' then
        proc.fetch <= fetch_next;
        proc.dec <= dec_next;
        proc.decex <= decex_next;
      else
        -- dont change pc
        proc.state.pc <= proc.state.pc;
      end if;

      -- instr retired
      if proc.state.enable(S_EXEC) = '1' then
        executed_addr_o <= proc.dec.cur_pc;
      end if;
      
    end if;
  end process;

  process (alu_i, cp_din_i, ctrl_next, dec_next, decex_next, exec_next,
           ext_mem_din_i, ext_mem_stall_i, mem1_next, mem2_next, proc,
           rp0_din_i, rp1_addr, rp1_din_i, state_next) is
  begin  -- process

    mem1_next.external_data <= ext_mem_din_i;
    mem2_next <= ((rp1_din_i,rp0_din_i),rp1_addr);
    if dec_next.en_irs = '1' then
      mem2_next.wr_addr <= X"0000" & dec_next.irs_addr;
    end if;
    
    fetch_next <= (cp_din_i,proc.state.pc);
    dec_next <= sl_dec(proc);
    decex_next <= sl_dec_ex(proc,dec_next,mem1_next,mem2_next,ext_mem_stall_i);
    exec_next <= sl_execute(proc,dec_next,alu_i,ext_mem_stall_i);
    ctrl_next <= sl_control(proc,decex_next.stall,exec_next.stall,exec_next.exec_next,exec_next.flush);
    state_next <= sl_state_update(proc,dec_next,exec_next,ctrl_next,'0',(others => '0'));

    if proc.state.enable(S_DECEX) = '1' and exec_next.exec_next = '1' and
      ctrl_next.stall_decex = '0' and ctrl_next.flush_pipeline = '0' then
      state_next.addr(0) <= proc.state.addr(0) + unsigned'("" & state_next.inc_ad0);
      state_next.addr(1) <= proc.state.addr(1) + unsigned'("" & state_next.inc_ad1);
    end if;

    if proc.state.enable(S_EXEC) = '1' and proc.decex.wb_en = '1' then
      case proc.decex.wb_reg is
        when WBREG_AD0 => state_next.addr(0) <= unsigned(exec_next.int_result);
        when WBREG_AD1 => state_next.addr(1) <= unsigned(exec_next.int_result);
        when WBREG_IRS => state_next.irs <= unsigned(exec_next.int_result);
        when others => null;
      end case;      
    end if;

    -- handle fast loop counter
    if proc.state.enable(S_EXEC) = '1' and proc.dec.loop1 = '1' then
      state_next.loop_count <= exec_next.int_result;
      state_next.loop_count(31) <= '1';
    end if;

    if exec_next.complete = '1' or (proc.state.enable(S_EXEC) = '1' and proc.decex.wb_en = '1' and proc.decex.wb_reg = WBREG_NONE) then
      state_next.result <= exec_next.result;
    end if;

    state_next.stall_exec_1d <= ctrl_next.stall_exec;
    
  end process;

end architecture rtl;
