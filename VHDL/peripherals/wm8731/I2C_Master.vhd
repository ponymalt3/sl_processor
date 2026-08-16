library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package i2c_p is

  type i2c_instr_t is (INSTR_START, INSTR_DATA, INSTR_STOP, INSTR_END);  -- instructions

  type i2c_config_t is array (natural range <>) of std_ulogic_vector(9 downto 0);

  function I2C_Start (                              -- always write
    constant addr : std_ulogic_vector(6 downto 0))  -- addr
    return std_ulogic_vector;

  function I2C_Data (
    constant data : std_ulogic_vector(7 downto 0))  -- data
    return std_ulogic_vector;

  function I2C_RStart (                             -- always write
    constant addr : std_ulogic_vector(6 downto 0))  -- addr
    return std_ulogic_vector;

  function I2C_Stop
    return std_ulogic_vector;

  function I2C_End
    return std_ulogic_vector;
    
  function To_ULogic(
    constant data : boolean)  -- data
    return std_ulogic;

end i2c_p;

package body i2c_p is

  function I2C_Start (
    constant addr : std_ulogic_vector(6 downto 0))
    return std_ulogic_vector is

  begin
    return To_StdUlogicVector(std_logic_vector(to_unsigned(i2c_instr_t'pos(INSTR_START),2))) & '1' & addr;
  end function;

  function I2C_Data (
    constant data : std_ulogic_vector(7 downto 0))
    return std_ulogic_vector is

  begin
    return To_StdUlogicVector(std_logic_vector(to_unsigned(i2c_instr_t'pos(INSTR_DATA),2))) & Data;
  end function;

  function I2C_RStart (
    constant addr : std_ulogic_vector(6 downto 0))
    return std_ulogic_vector is

  begin
    return To_StdUlogicVector(std_logic_vector(to_unsigned(i2c_instr_t'pos(INSTR_START),2))) & '1' & addr;
  end function;

  function I2C_Stop
    return std_ulogic_vector is

  begin
    return To_StdUlogicVector(std_logic_vector(to_unsigned(i2c_instr_t'pos(INSTR_STOP),2))) & "00000000";
  end function;

  function I2C_End
    return std_ulogic_vector is

  begin
    return To_StdUlogicVector(std_logic_vector(to_unsigned(i2c_instr_t'pos(INSTR_END),2))) & "00000000";
  end function;
  
  function To_ULogic(
    constant data : boolean)  -- data
    return std_ulogic is
  begin
    if data = TRUE then
      return '1';
    else
      return '0';
    end if;
  end function;

end package body i2c_p;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.i2c_p.all;

entity I2C_Master is
  
  generic (
    Baudrate  : natural := 200000;
    ClockFreq : natural := 50000000;
    Config    : i2c_config_t := (I2C_End,I2C_End));

  port (
    clk_i : in std_ulogic;
    reset_n_i : in std_ulogic;
    --------------------------------------------------------------------------- 
    active_o : out std_ulogic;          -- bus is active
    busy_o : out std_ulogic;            -- master is transmitting/receiving (master might although be active)                         
    ---------------------------------------------------------------------------
    start_i : in std_ulogic;            -- send start signal on bus
    addr_i : in std_ulogic_vector(6 downto 0);
    rw_i : in std_ulogic;
    ---------------------------------------------------------------------------
    stop_i : in std_ulogic;             -- send stop signal on bus
    ---------------------------------------------------------------------------
    din_i :in  std_ulogic_vector(7 downto 0);
    we_i : in std_ulogic;               -- send data on bus
    ---------------------------------------------------------------------------
    dout_o : out std_ulogic_vector(7 downto 0);
    re_i : in std_ulogic;               -- receive data from bus
    ---------------------------------------------------------------------------
    ack_i : in std_ulogic;              -- acknowledge when receiving
    ack_o : out std_ulogic;             -- acknowledge when sending
    ---------------------------------------------------------------------------
    SCL : out std_ulogic;             -- I2C Clock Line
    SDA : inout std_logic;
    dbg_o : out std_ulogic_vector(7 downto 0));             -- I2C Data Line
end I2C_Master;

architecture rtl of I2C_Master is

  type fsm_t is (FSM_IDLE,FSM_ACTIVE,FSM_ACTIVE_PRE,FSM_START,FSM_STOP,FSM_READ,FSM_READ_ACK,FSM_WRITE,FSM_WRITE_ACK);

  signal state : fsm_t;
  signal data_reg : std_ulogic_vector(7 downto 0);
  signal active : std_ulogic;
  signal bus_mode : std_ulogic;

  signal count : integer range 0 to 8;
  
  constant i2c_clk_div : integer := ClockFreq/(Baudrate*4);

  constant i2c_clk_phase_0 : integer := i2c_clk_div*0;
  constant i2c_clk_phase_90 : integer := i2c_clk_div*1;
  constant i2c_clk_phase_180 : integer := i2c_clk_div*2;
  constant i2c_clk_phase_270 : integer := i2c_clk_div*3;

  signal phase : integer range 0 to 4*i2c_clk_div-1;

  type clk_phase_t is (PHASE_0,PHASE_90,PHASE_180,PHASE_270,PHASE_OTHER);  -- phase

  signal i2c_clk_phase : clk_phase_t;

  signal ack_in : std_ulogic;
  signal ack_out_en : std_ulogic;
  
  signal sel_reg : std_ulogic_vector(2 downto 0);

  signal conf_instr : std_ulogic_vector(9 downto 0);
  signal conf_instr_ptr : integer range 0 to Config'length;
  signal conf_instr_dec : i2c_instr_t;
  signal conf_data : std_ulogic_vector(7 downto 0);
  signal conf_state : fsm_t;
  signal conf_en : std_ulogic;

  constant conf_init : boolean := Config'length > 1;

  signal tri_data : std_ulogic;
  signal tri_en : std_ulogic;
  signal sda_in : std_ulogic;
  
begin  -- rtl

  process (clk_i, reset_n_i)
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      phase <= i2c_clk_phase_270;
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if state = FSM_IDLE and start_i = '0' and phase = i2c_clk_phase_0 then
        phase <= i2c_clk_phase_0;
      elsif state = FSM_ACTIVE and phase = i2c_clk_phase_270 then
      else
        if phase = (4*i2c_clk_div)-1 then  --360
          phase <= 0;
        else
          phase <= phase+1;
        end if;
      end if;      
    end if;
  end process;

  process(ack_in, active, conf_en, conf_instr, conf_instr_dec,
                  conf_instr_ptr, phase, state)
  begin  -- process

    ack_o <= ack_in;    
    busy_o <= '1';
    active_o <= active;
   
    if (state = FSM_IDLE and conf_en = '0') or state = FSM_ACTIVE then
      busy_o <= '0';
    end if;

    if phase = i2c_clk_phase_0 then
      i2c_clk_phase <= PHASE_0;
    elsif phase = i2c_clk_phase_90 then
      i2c_clk_phase <= PHASE_90;
    elsif phase = i2c_clk_phase_180 then
      i2c_clk_phase <= PHASE_180;
    elsif phase = i2c_clk_phase_270 then
      i2c_clk_phase <= PHASE_270;
    else
      i2c_clk_phase <= PHASE_OTHER;
    end if;

    conf_instr <= Config(conf_instr_ptr);
    conf_instr_dec <= i2c_instr_t'val(to_integer(unsigned(conf_instr(9 downto 8))));

    conf_data <= (others => '0');

    case conf_instr_dec is
      when INSTR_START =>
        conf_state <= FSM_START;
        conf_data <= conf_instr(6 downto 0) & '0';
      when INSTR_DATA =>
        conf_state <= FSM_WRITE;
        conf_data <= conf_instr(7 downto 0);
      when INSTR_STOP =>
        conf_state <= FSM_STOP;
      when INSTR_END =>
        conf_state <= FSM_IDLE;
      when others => null;
    end case;
    
  end process;
  
  dbg_o(0) <= active;
  dbg_o(1) <= bus_mode;
  dbg_o(2) <= ack_in;
  dbg_o(3) <= sda_in;
  
  dbg_o(7 downto 5) <= (others => '0');
  
  process (SDA,tri_en,tri_data)
  begin  -- process
    if tri_en = '1' then
      SDA <= 'Z';
      sda_in <= SDA;
      else
       SDA <= tri_data;
       sda_in <= SDA;
    end if;
  end process;
                 
  process (clk_i, reset_n_i)
  begin  -- process
    if reset_n_i = '0' then                 -- asynchronous reset (active low)
      SCL <= '1';
      dbg_o(4) <= '1';
      tri_data <= '1';
      tri_en <= '0';
      state <= FSM_IDLE;
      active <= '0';
      ack_in <= '0';
      ack_out_en <= '0';
      bus_mode <= '0';
      count <= 0;
      conf_instr_ptr <= 0;     
      dout_o <= (others => '0');
      sel_reg <= (others => '0');
      data_reg <= (others => '0');     
      conf_en <= To_ULogic(conf_init);
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge 
      -- when configuration
      if conf_en = '1' and (state = FSM_IDLE or state = FSM_ACTIVE_PRE) then
        state <= conf_state;
        data_reg <= conf_data;
        if conf_instr_dec = INSTR_END then
          conf_en <= '0';
        else
          conf_instr_ptr <= conf_instr_ptr+1;
        end if;       
      else
        case state is
          when FSM_IDLE =>
            if start_i = '1' then
              data_reg <= addr_i & rw_i;
              state <= FSM_START;
            end if;            
          when FSM_ACTIVE =>
            if ack_out_en = '0' or bus_mode = '1' then        --bus_mode = write
              if start_i = '1' then
                data_reg <= addr_i & rw_i;
                state <= FSM_START;
              elsif stop_i = '1' then
                state <= FSM_STOP;
              elsif we_i = '1' and bus_mode = '1' then
                data_reg <= din_i;
                state <= FSM_WRITE;
              elsif re_i = '1' and bus_mode = '0' then
                state <= FSM_READ;
                ack_out_en <= '1';
              end if;
            elsif (start_i or stop_i or re_i) = '1' then  -- bus_mode = read
              data_reg <= addr_i & rw_i;
              state <= FSM_WRITE_ACK;                     -- only if ack_out_en is '1' otherwise switch to FSM_READ (see above)
              sel_reg <= start_i & stop_i & re_i;
            end if;
          when FSM_ACTIVE_PRE =>
            if active = '0' and ack_in = '1' then
              state <= FSM_STOP; -- error: no slave responds to address
            else
              state <= FSM_ACTIVE;
              active <= '1';
              ack_out_en <= active;     -- enable acknowledge output only if master is already active
            end if;    
          when others => null;
        end case;       
      end if;
      
      
      case i2c_clk_phase is
        
        --generate clk
        -----------------------------------------------------------------------
        when PHASE_0 =>
          SCL <= '1';
          dbg_o(4) <= '1';                   
          
        --generate states
        -----------------------------------------------------------------------
        when PHASE_90 =>
          case state is
            when FSM_START =>
              tri_data <= '0';
              tri_en <= '0';
              state <= FSM_WRITE;
              bus_mode <= not(data_reg(0));  -- rw
              active <= '0';              
            when FSM_READ =>
              count <= count+1;
              data_reg <= data_reg(6 downto 0) & sda_in;             
              if count = 7 then
                state <= FSM_ACTIVE_PRE;
                dout_o <= data_reg;
                count <= 0;
              end if;
            when FSM_READ_ACK =>
              ack_in <= sda_in;
              state <= FSM_ACTIVE_PRE;              
            when FSM_WRITE =>
              count <= count+1;              
              if count = 7 then
                state <= FSM_READ_ACK;
                count <= 0;
              end if;
            when FSM_WRITE_ACK =>
              if sel_reg(2) = '1' then  --restart
                state <= FSM_START;
              elsif sel_reg(1) = '1' then  --stop
                state <= FSM_STOP;
              else
                state <= FSM_READ;
              end if;              
            when FSM_STOP =>
              tri_data <= '1';
              tri_en <= '0';
              state <= FSM_IDLE;
              active <= '0';          
            when others => null;
          end case;

        -- generate clk
        -----------------------------------------------------------------------
        when PHASE_180 =>
          SCL <= '0';
          dbg_o(4) <= '0';                    
          
        -- generate output
        -----------------------------------------------------------------------
        when PHASE_270 =>
          if state = FSM_WRITE then
            tri_data <= data_reg(7);
            tri_en <= '0';
            data_reg <= data_reg(6 downto 0) & '0';
          elsif state = FSM_WRITE_ACK then
            tri_data <= ack_i;
            tri_en <= '0';
          elsif state = FSM_READ or state = FSM_READ_ACK then
            tri_data <= '0';
            tri_en <= '1';
          elsif state = FSM_STOP then
            tri_data <= '0';
            tri_en <= '0';
          elsif state = FSM_START then
            tri_data <= '1';
            tri_en <= '0';
          else
            tri_data <= '1';
            tri_en   <= '0';            
          end if;
          
        when others => null;
      end case;      
    end if;
  end process;

end rtl;
