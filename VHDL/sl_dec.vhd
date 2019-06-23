library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.sl_structs_p.all;
use work.sl_misc_p.all;

package sl_dec_p is

  function sl_dec (
    proc : sl_processor_t;
    ExtAddrThreshold : natural)
    return sl_decode_t;

end package sl_dec_p;

package body sl_dec_p is

  function sl_dec (
    proc : sl_processor_t;
    ExtAddrThreshold : natural)
    return sl_decode_t is

    variable decode : sl_decode_t;
    variable inc_ad0 : std_ulogic;
    variable inc_ad1 : std_ulogic;
    variable data : std_ulogic_vector(15 downto 0);
  begin

    data := proc.fetch.data;

    decode.mux_ad0 := data(0);
    decode.mux_a := data(1);
    decode.mux_ad1 := data(2);
    decode.mux_b := data(3);

    decode.en_irs := '0';
    decode.en_mem := '0';
    decode.en_reg := '0';

    -- need addr for memory read port X
    decode.en_ad0 := '0';
    decode.en_ad1 := '0';

    decode.wb_reg := data(6 downto 5);

    inc_ad0 := '0';
    inc_ad1 := '0';

    decode.c_data := data(11 downto 2);
    decode.c_data_ext := data(1 downto 0);

    decode.cmd := CMD_MOV;

    decode.goto := '0';
    decode.goto_const := '0';
    decode.load := '0';
    decode.cmp := '0';
    decode.neg := '0';
    decode.trunc := '0';
    decode.wait1 := '0';
    decode.signal1 := '0';
    decode.loop1 := '0';

    decode.cmp_mode := data(1 downto 0);
    decode.cmp_noX_cy := data(11);

    if data(15) = '0' then
      decode.cmd(2 downto 0) := data(14 downto 12);
      inc_ad1 := data(11);
      decode.en_irs := '1';
      decode.en_ad0 := to_ulogic(decode.mux_a = MUX1_MEM);
      if decode.mux_a = MUX1_RESULT then
        decode.cmd(3) := data(0);
      end if;
    else
      case to_integer(unsigned(data(14 downto 12))) is
        when 0 => -- MOVIRS1
          decode.en_irs := '1';
          decode.wb_reg := data(1 downto 0);
          decode.en_reg := '1';
          decode.mux_a := MUX1_MEM; -- no wait for result
        when 1 => -- MOVIRS2
          decode.en_irs := '1';
          decode.en_mem := '1';
        when 2 => -- GOTO
          decode.goto := '1';
          decode.goto_const := data(0);
        when 3 => -- LOAD
          decode.load := '1';
          decode.mux_a := MUX1_RESULT;
        when 4 => -- OP
          decode.cmd := data(8 downto 5);
          inc_ad0 := data(4);
          inc_ad1 := data(9);
          decode.en_ad0 := to_ulogic(decode.mux_a = MUX1_MEM);
          decode.en_ad1 := '1';
        when 5 => -- CMP
          decode.cmp := '1';
          decode.cmd := CMD_CMP;
          decode.en_irs := '1';
          decode.mux_a := MUX1_RESULT;
          decode.mux_b := MUX2_MEM;
        when 7 =>
          case to_integer(unsigned(data(11 downto 6))) is
            when 0 | 1 => -- MOVDATA2(1)
              decode.en_reg := '1';
              inc_ad0 := data(4);
              decode.en_ad1 := to_ulogic(decode.wb_reg = WBREG_NONE); -- result reg as dest
            when 2 => -- MOVDATA2(2)
              decode.en_mem := '1';
              decode.en_ad0 := to_ulogic(decode.mux_a = MUX1_MEM);
              decode.en_ad1 := '1';
              inc_ad0 := data(4);
              inc_ad1 := to_ulogic(decode.mux_a = MUX1_MEM and data(4) = '1');
            when 3 => -- SIG
              decode.wait1 := data(10);
              decode.signal1 := not data(10);
            when 4 => -- UNARY
              decode.neg := data(0);
              decode.trunc := data(2);
              decode.cmd := '1' & data(5 downto 3);
            when 5 => -- LOOP
              decode.loop1 := '1';
            when others => decode.mux_a := MUX1_RESULT;
          end case;
          
        when others => null;
      end case;
    end if;

    -- maybe not necessary
    if decode.en_irs = '1' then
      decode.mux_b := MUX2_MEM;
    end if;

    decode.mem_ex := not decode.en_irs and to_ulogic(proc.state.addr(to_integer(unsigned'("" & decode.mux_ad1))) >= to_unsigned(ExtAddrThreshold,16));

    decode.cur_pc := proc.fetch.pc;
    decode.irs_addr := proc.state.irs(15 downto 0)+((15 downto 9 => '0') & unsigned(data(10 downto 2))); -- irs offset

    -- pre calculate jmp target
    decode.jmp_back := decode.c_data(9);
    decode.jmp_target_pc := proc.fetch.pc(15 downto 0)+(unsigned(extend(decode.c_data(9),7)) & unsigned(decode.c_data(8 downto 0)));
    inc_ad1 := inc_ad1 or (not data(15) and inc_ad0);

    -- addr inc
    decode.inc_ad0 := (inc_ad0 and not decode.mux_ad1) or (inc_ad1 and not decode.mux_ad0);
    decode.inc_ad1 := (inc_ad0 and decode.mux_ad1) or (inc_ad1 and decode.mux_ad0); 

    return decode;
    
  end function;

end package body sl_dec_p;
