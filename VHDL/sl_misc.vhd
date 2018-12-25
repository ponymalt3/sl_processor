library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package sl_misc_p is

  function extend (
    value        : std_ulogic;
    repeat_count : natural)
    return std_ulogic_vector;  

  function to_hstring (
    data : std_ulogic_vector)
    return string;

    -- implements a barrel shifter
  function fast_shift (
    data_in : unsigned;
    shft : natural;
    mode  : std_ulogic; -- left=0
    extend : std_ulogic := '0')
    return unsigned;

   function to_ulogic (
    cond : boolean)
    return std_ulogic;

  function min (
    a : natural;
    b : natural)
    return natural;

  function max (
    a : natural;
    b : natural)
    return natural;

  function log2 (
    x : natural)
    return natural;  

end package sl_misc_p;

package body sl_misc_p is

  function extend (
    value        : std_ulogic;
    repeat_count : natural)
    return std_ulogic_vector is

    variable result : std_ulogic_vector(repeat_count-1 downto 0);
  begin
    for i in 0 to repeat_count-1 loop
      result(i) := value;
    end loop;  -- i
    return result;
  end function;

  function to_hstring (
    data : std_ulogic_vector)
    return string is
    variable s : string(1 to (data'length+3)/4);
    variable slice : std_ulogic_vector(3 downto 0);
    constant hex : string := "0123456789abcdef";
  begin
    for i in 0 to s'length-1 loop
      if data'length-1 >= i*4+3 then
        slice := data(i*4+3 downto i*4);
      else
        slice(3-(i*4+3-data'length-1) downto 0) := data(min(i*4+3,data'length-1) downto i*4);
      end if; 
      s(s'length-i) := hex(to_integer(unsigned(slice))+1);
    end loop;  -- i

    return s;
  end function;

    -- implements a barrel shifter
  function fast_shift (
    data_in : unsigned;
    shft : natural;
    mode  : std_ulogic; -- left=0
    extend : std_ulogic := '0')
    return unsigned is
    
    constant size_lg2 : integer := log2(data_in'length);
   
    variable shft_rem : natural;
    variable data : unsigned(data_in'length-1 downto 0);
  begin  -- fast_shift

    shft_rem := shft;
    
    data := data_in;
    
    for i in size_lg2-1 downto 0 loop
      if shft_rem >= 2**i then
        case mode is
          when '0' => -- shift left
            data := shift_left(data,2**i);
          when '1' => -- shift right 
            data := shift_right(data,2**i);
            -- overwrite top bits with extend
            data(data'length-1 downto (data'length-2**i)) := (others => extend);
          when others => null;
        end case;
        shft_rem := shft_rem-2**i;
      end if;
    end loop;  -- i
    
    return data;
    
  end fast_shift;

  function to_ulogic (
    cond : boolean)
    return std_ulogic is
  begin
    if cond then
      return '1';
    end if;

    return '0';
  end to_ulogic;

  function min (
    a : natural;
    b : natural)
    return natural is
  begin  -- function min
    if a < b then
      return a;
    end if;
    return b;
  end function min;

  function max (
    a : natural;
    b : natural)
    return natural is
  begin  -- function max
    if a > b then
      return a;
    end if;
    return b;
  end function max;

  function log2 (
    x : natural)
    return natural is
    
    variable i : natural;
    variable result : natural;
    
  begin  -- log2
    result := 0;

    if x <= 1 then
      return result;
    end if;
    
    while x > 2**result loop
      result := result+1;
    end loop;
    
    return result;
    
  end log2;

  end package body sl_misc_p;
