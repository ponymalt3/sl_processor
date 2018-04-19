library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.sl_structs_p.all;

package sl_execute_p is
  
  function sl_execute (
    proc : sl_processor_t;
    decode : sl_decode_t;
    alu : sl_alu_t;
    ext_mem_stall : std_ulogic)
    return sl_exec_t;

end package sl_execute_p;

package body sl_execute_p is

  function sl_execute (
    proc : sl_processor_t;
    decode : sl_decode_t;
    alu : sl_alu_t;
    ext_mem_stall : std_ulogic)
    return sl_exec_t is

    variable exec : sl_exec_t;
  begin
    exec.result := alu.result;
    exec.complete := alu.complete;

    -- fragmented load
    if proc.state.enable(S_EXEC) = '1' and proc.decex.load = '1' then
      exec.complete := '1';
      case proc.state.load_state is
        when "001" =>
          exec.result(31) := proc.decex.load_data(11);
          exec.result(30) := '0';
          exec.result(29 downto 19) := proc.decex.load_data(10 downto 0);
          exec.result(18 downto 0) := (others => '0');
        when "010" =>
          exec.result := proc.state.result;
          exec.result(30) := proc.decex.load_data(11);
          exec.result(18 downto 8) := proc.decex.load_data(10 downto 0);
        when "100" =>
          exec.result := proc.state.result;
          exec.result(7 downto 0) := proc.decex.load_data(7 downto 0);
        when others => null;
      end case;
    end if;

    if proc.state.enable(S_EXEC) = '1' and proc.decex.neg = '1' then
      exec.result := not proc.state.result(31) & proc.state.result(30 downto 0);
      exec.complete := '1';
    end if;

    exec.int_result := alu.int_result;

    exec.exec_next := '1';
    if proc.state.enable(S_EXEC) = '1' and proc.decex.cmp = '1' then
      case proc.decex.cmp_mode is
        when CMP_EQ => exec.exec_next := alu.cmp_eq;
        when CMP_NEQ => exec.exec_next := not alu.cmp_eq;
        when CMP_LT => exec.exec_next := alu.cmp_lt;
        when CMP_LE => exec.exec_next := alu.cmp_lt or alu.cmp_eq;
        when others => null;
      end case;
    end if;

    exec.stall := '0';

    if alu.complete = '0' and alu.idle = '0' then
      exec.stall := '1';
    end if;

    if proc.decex.wr_en = '1' and proc.decex.wr_ext = '1' then
      exec.stall := ext_mem_stall;
    end if;
    
    exec.flush := proc.decex.goto;

    return exec;

  end function; 

end package body sl_execute_p;
