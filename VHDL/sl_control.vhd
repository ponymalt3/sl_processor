library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.sl_structs_p.all;

package sl_control_p is

  function sl_control (
    proc : sl_processor_t;
    stall_decex : std_ulogic;
    stall_exec : std_ulogic;
    exec_en : std_ulogic;
    flush_pipeline : std_ulogic;
    fetch_valid : std_ulogic)
    return sl_stall_ctrl_t;

end package sl_control_p;

package body sl_control_p is

  function sl_control (
    proc : sl_processor_t;
    stall_decex : std_ulogic;
    stall_exec : std_ulogic;
    exec_en : std_ulogic;
    flush_pipeline : std_ulogic;
    fetch_valid : std_ulogic)
    return sl_stall_ctrl_t is
    variable ctrl : sl_stall_ctrl_t;
    variable enable : std_ulogic_vector(2 downto 1);
  begin
    
    --ctrl.exec_en := exec_en;

    -- mask with active stages
    ctrl.stall_exec := stall_exec and proc.state.enable(S_EXEC);
    ctrl.stall_decex := (stall_decex and proc.state.enable(S_DECEX)) or ctrl.stall_exec;

    --ctrl.stall_decex := ctrl.stall_decex or ctrl.stall_exec;

    ctrl.enable := proc.state.enable;

    disable_exec := not proc.state.exec_next;

    if ctrl.stall_decex = '0' then
      ctrl.enable := '1' & fetch_valid & ctrl.enable(1); -- shift in 1

      if exec_en = '0' and proc.state.enable(S_EXEC) = '1' then
        ctrl.enable(S_EXEC) := '0'; -- disable exec
      elsif disable_exec = '1' then
        ctrl.enable(S_EXEC) := '0';
      end if;
    elsif ctrl.stall_exec = '0' then
      ctrl.enable(S_EXEC) := '0'; -- disable exec
    end if;

    ctrl.flush_pipeline := flush_pipeline and proc.state.enable(S_EXEC);

    if ctrl.flush_pipeline = '1' then
      ctrl.enable := (others => '0');
      ctrl.enable(S_FETCH) := '1';
    end if;

    return ctrl;
  end function;

end package body sl_control_p;
