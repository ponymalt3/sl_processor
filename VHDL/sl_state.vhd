library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.sl_structs_p.all;

package sl_state_p is
  
  function sl_state_update (
    proc : sl_processor_t;
    decode : sl_decode_t;
    exec   : sl_exec_t;
    ctrl : sl_stall_ctrl_t;
    pc_stall : std_ulogic)
    return sl_state_t;

end package sl_state_p;

package body sl_state_p is

  function sl_state_update (
    proc : sl_processor_t;
    decode : sl_decode_t;
    exec   : sl_exec_t;
    ctrl : sl_stall_ctrl_t;
    pc_stall : std_ulogic)
    return sl_state_t is
    variable state : sl_state_t;
  begin
    state := proc.state;
    
    state.stall_exec_1d := '0';
    state.enable := ctrl.enable;
    state.inc_ad0 := '0';
    state.inc_ad1 := '0';

    if decode.inc_ad0 = '1' and proc.state.enable(S_DEC) = '1' then
      state.inc_ad0 := '1';
    end if;

    if decode.inc_ad1 = '1' and proc.state.enable(S_DEC) = '1' then
      state.inc_ad1 := '1';
    end if;

    state.pc := state.pc+to_unsigned(to_integer(unsigned'("" & not pc_stall)),16);

    if proc.decex.goto = '1' and proc.state.enable(S_EXEC) = '1' then -- goto cannot stall!
      if proc.dec.goto_const = '1' then
        state.pc := proc.dec.jmp_target_pc;
      else
        state.pc := unsigned(exec.int_result(15 downto 0));-- conversion from qfp
      end if;
    end if;

    if proc.dec.goto_const = '1' and proc.dec.jmp_back = '1' and proc.state.enable(S_EXEC) = '1' then
      if proc.state.loop_count /= X"00000000" then
        state.loop_count := "000" & To_StdULogicVector(std_logic_vector(unsigned(proc.state.loop_count(28 downto 0))-1));
      end if;
    end if;

    -- update load state
    if proc.state.enable(S_EXEC) = '1' and proc.dec.valid = '1' then --- check
      if proc.decex.load = '1' then
        state.load_state := state.load_state(1 downto 0) & '0';
      else
        state.load_state := "001";
      end if;
    end if;

    --state.exec_next := not ctrl.disable_exec;
    state.exec_next := proc.state.exec_next;
    if proc.fetch.valid = '0' and exec.exec_next = '0' then
      state.exec_next := '0';
    elsif proc.fetch.valid = '1' then-- dec.valid
      state.exec_next := '1';
    end if;

    return state;
  end function;
  
end package body sl_state_p;
