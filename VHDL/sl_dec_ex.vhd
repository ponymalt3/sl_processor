library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.sl_structs_p.all;

package sl_dec_ex_p is

  function sl_dec_ex (
    proc : sl_processor_t;
    decode : sl_decode_t;
    mem1 : sl_mem1_t;
    mem2 : sl_mem2_t;
    ext_mem_stall : std_ulogic)
    return sl_decode_ex_t;

end package sl_dec_ex_p;

package body sl_dec_ex_p is

  function sl_dec_ex (
    proc : sl_processor_t;
    decode : sl_decode_t;
    mem1 : sl_mem1_t;
    mem2 : sl_mem2_t;
    ext_mem_stall : std_ulogic)
    return sl_decode_ex_t is

    variable decode_ex : sl_decode_ex_t;
  begin

    decode_ex.mux0 := decode.mux_a;

    --decode_ex.mem0 := mem2.read_data(0);
    --decode_ex.mem1 := mem2.read_data(1);
    decode_ex.memX := mem1.external_data;

    decode_ex.wr_addr := mem2.wr_addr;
    decode_ex.wr_en := decode.en_mem;
    decode_ex.wr_ext := decode.mem_ex;

    decode_ex.wb_en := decode.en_reg;
    decode_ex.wb_reg := decode.wb_reg;

    decode_ex.load := decode.load;
    decode_ex.load_data := decode.c_data & decode.c_data_ext(1 downto 0); --  correct?? (decodeComb.cData_<<2) + decodeComb.cDataExt_

    decode_ex.cmd := decode.cmd;
    decode_ex.cmp := decode.cmp;
    decode_ex.cmp_mode := decode.cmp_mode;

    decode_ex.goto := decode.goto;
    decode_ex.neg := decode.neg;
    decode_ex.trunc := decode.trunc;

    if proc.state.loop_count(28 downto 1) = X"0000000" and (proc.state.loop_count(31) = '1' or proc.state.loop_count(0) = '1') then
      decode_ex.goto := '0';
    end if;
    
    decode_ex.stall := '0';
                                                                         
    -- stall because of pending write to address reg
    if proc.state.enable(S_EXEC) = '1' and proc.decex.wb_en = '1' then

      if decode.en_ad0 = '1' and ('0' & decode.mux_ad0) = proc.decex.wb_reg then
        decode_ex.stall := '1';
      end if;

      if decode.en_ad1 = '1' and ('0' & decode.mux_ad1) = proc.decex.wb_reg then
        decode_ex.stall := '1';
      end if;

      if decode.en_mem = '1' and ('0' & decode.mux_ad1) = proc.decex.wb_reg then
        decode_ex.stall := '1';
      end if;

      -- stall if irs reg will be written
      if decode.en_irs = '1' and proc.decex.wb_reg = WBREG_IRS then
        decode_ex.stall := '1';
      end if;

    end if;

    if decode.mux_a = MUX1_MEM and proc.decex.wr_en = '1' and proc.state.enable(S_EXEC) = '1' then
      decode_ex.stall := '1';
    end if;

    -- ext write in progress
    if decode.en_ad1 = '1' and decode.mem_ex = '1' and
      ((proc.decex.wr_en = '1' and proc.decex.wr_ext = '1' and proc.state.enable(S_EXEC) = '1') or ext_mem_stall = '1') then
        decode_ex.stall := '1';
    end if;
  
  return decode_ex;

 end function;

end package body sl_dec_ex_p;
