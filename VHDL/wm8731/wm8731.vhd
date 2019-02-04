library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.i2c_p.all;

entity wm8731 is

  generic (
    ClkPerMCLK : natural := 4);
  
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    wm_mclk_o    : out std_ulogic;      -- master clk
    wm_bclk_o    : out std_ulogic;      -- bitstream clk
    wm_adc_lrc_o : out std_ulogic;      -- clk
    wm_dac_lrc_o : out std_ulogic;
    wm_adc_i     : in  std_ulogic;
    wm_dac_o     : out std_ulogic;

    -- i2c
    wm_sclk : out   std_ulogic;
    wm_sdat : inout std_logic;

    -- user interface
    cfg_addr_i  : in  std_ulogic_vector(6 downto 0);
    cfg_data_i  : in  std_ulogic_vector(8 downto 0);
    cfg_idle_o  : out std_ulogic;
    cfg_en_i    : in  std_ulogic;
    cfg_error_o : out std_ulogic;

    dsp_dacl_i    : in  std_ulogic_vector(23 downto 0);
    dsp_dacr_i    : in  std_ulogic_vector(23 downto 0);
    dsp_dac_sync_o : out std_ulogic;
    
    dsp_adcl_o    : out std_ulogic_vector(23 downto 0);
    dsp_adcr_o    : out std_ulogic_vector(23 downto 0);    
    dsp_adc_sync_o : out std_ulogic;
    dbg_o : out std_ulogic_vector(7 downto 0));

end wm8731;

architecture rtl of wm8731 is

  constant SR_8kHZ  : std_ulogic_vector(3 downto 0) := "0011";
  constant SR_32kHZ : std_ulogic_vector(3 downto 0) := "0110";
  constant SR_44kHZ : std_ulogic_vector(3 downto 0) := "1000";
  constant SR_48kHZ : std_ulogic_vector(3 downto 0) := "0000";
  constant SR_88kHZ : std_ulogic_vector(3 downto 0) := "1111";
  constant SR_96kHZ : std_ulogic_vector(3 downto 0) := "0111";

  constant SR_Count_8kHZ  : integer range 0 to 15 := 12; -- "1100"  -- 11000000000
  constant SR_Count_32kHZ : integer range 0 to 15 := 3;  -- "0011"  -- 00110000000
  constant SR_Count_48kHZ : integer range 0 to 15 := 2;  -- "0010"  -- 00100000000
  constant SR_Count_96kHZ : integer range 0 to 15 := 1;  -- "0001"  -- 00010000000

  constant wm8731_addr : std_ulogic_vector(6 downto 0) := "0011010";

  constant MCLK_RISING : integer := 0;
  constant MCLK_FALLING : integer := ClkPerMCLK/2;

  constant wm_init : i2c_config_t := (I2C_Start(wm8731_addr),
                                      I2C_Data("00001110"),  -- digital audio interface control
                                      I2C_Data("00011011"),  -- set sample bits to 24 Bit
                                      I2C_Start(wm8731_addr),
                                      I2C_Data("00010000"),  -- sample rate control
                                      I2C_Data("00000000"),  -- set sample rate to 48kHZ
                                      I2C_End);

  type fsm_t is (FSM_IDLE,FSM_CFG_REG,FSM_CFG_DAT,FSM_CFG_UPDATE);

  signal state : fsm_t;
  signal mclk : std_ulogic;

  signal adc_reg : std_ulogic_vector(47 downto 0);
  signal dac_reg : std_ulogic_vector(47 downto 0);
  signal cfg_reg : std_ulogic_vector(15 downto 0);
  signal samp_rate : std_ulogic_vector(3 downto 0);

  signal mclk_count : integer range 0 to 3;  -- counter for generating mclk
  signal sync_count : unsigned(10 downto 0);  -- counting mclk clock cycles
  signal sync_count_max : std_ulogic_vector(10 downto 0);

  -- adc and dac same data width
  
  signal i2c_active : std_ulogic;
  signal i2c_busy : std_ulogic;
  signal i2c_start : std_ulogic;
  signal i2c_din : std_ulogic_vector(7 downto 0);
  signal i2c_we : std_ulogic;
  signal i2c_ack : std_ulogic;
  
  signal i2c_error : std_ulogic;
  signal cfg_valid : std_ulogic;
  signal cfg_din : std_ulogic_vector(8 downto 0);
  
  signal sr_count : integer range 0 to 2048;
  
  signal i2c_reconf_en : std_ulogic;  
  signal cfg_sr_valid : std_ulogic;  
  signal frame_en : std_ulogic;
  signal frame_sync : std_ulogic;
  signal frame_full : std_ulogic;
  signal lrc : std_ulogic;
  signal clk_edge : std_ulogic_vector(1 downto 0);
  signal clk_edge_1d : std_ulogic_vector(1 downto 0);

  alias cfg_addr : std_ulogic_vector(6 downto 0) is cfg_reg(15 downto 9);
  alias cfg_data : std_ulogic_vector(8 downto 0) is cfg_reg(8 downto 0);

begin  -- rtl

  I2C_Master_1 : entity work.I2C_Master
    generic map (
      Baudrate  => 100000,
      ClockFreq => 50000000,
      Config    => wm_init)
    port map (
      clk_i     => clk_i,
      reset_n_i => reset_n_i,
      active_o  => i2c_active,
      busy_o    => i2c_busy,
      start_i   => i2c_start,
      addr_i    => wm8731_addr,
      rw_i      => '0',
      stop_i    => '0',
      din_i     => i2c_din,
      we_i      => i2c_we,
      dout_o    => open,
      re_i      => '0',
      ack_i     => '1',
      ack_o     => i2c_ack,
      SCL       => wm_sclk,
      SDA       => wm_sdat,
      dbg_o      => dbg_o);

  p_cfg : process (clk_i, reset_n_i)
  begin  -- process p_cfg
    if reset_n_i = '0' then                 -- asynchronous reset (active low)
      state <= FSM_IDLE;
      i2c_error <= '0';
      samp_rate <= To_StdULogicVector(std_logic_vector(to_unsigned(SR_Count_48kHZ-1,4)));
      cfg_reg <= (others => '0');
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      case state is
        when FSM_IDLE =>
          if cfg_en_i = '1' and i2c_busy = '0' then
            i2c_error <= not(cfg_valid);
            if cfg_valid = '1' then
              state <= FSM_CFG_REG;
              cfg_reg <= cfg_addr_i & cfg_din;
            end if;
          end if;
        when FSM_CFG_REG =>
          if i2c_active = '1' and i2c_busy = '0' then
            state <= FSM_CFG_DAT;
          end if;
        when FSM_CFG_DAT =>
          if i2c_active = '1' and i2c_busy = '0' then
            state <= FSM_CFG_UPDATE;
          end if;
        when FSM_CFG_UPDATE =>
          if i2c_active = '1' and i2c_busy = '0' then
            if cfg_addr = "0000111" then
              --samp_bits <= cfg_data(3 downto 2);  -- sample bits
            elsif cfg_addr = "0001000" then
              -- sample rate
              samp_rate <= To_StdULogicVector(std_logic_vector(to_unsigned(sr_count,4)));
            elsif cfg_addr = "0001111" then
              --cfg_error_o <= '0';
            end if;
            state <= FSM_IDLE;
          end if;
        when others => null;
      end case;
    end if;
  end process p_cfg;

  p_cfg2 : process (cfg_addr, cfg_addr_i, cfg_data, cfg_data_i, cfg_en_i,
                    cfg_reg, cfg_sr_valid, i2c_active, i2c_busy, i2c_error,
                    state)
  begin  -- process p_cfg
    i2c_din <= (others => '0');
    i2c_reconf_en <= '0';
    cfg_idle_o <= '0';
    i2c_we <= '0';
    i2c_start <= '0';
    i2c_din <= (others => '0');

    case state is
      when FSM_IDLE =>
        i2c_start <= cfg_en_i and not(i2c_busy);
        cfg_idle_o <= not(i2c_busy);        
        
      when FSM_CFG_REG =>
        i2c_din <= cfg_reg(15 downto 8);
        i2c_we <= i2c_active and not(i2c_busy);
        
      when FSM_CFG_DAT =>
        i2c_din <= cfg_reg(7 downto 0);
        i2c_we <= i2c_active and not(i2c_busy);

      when FSM_CFG_UPDATE =>
        if cfg_addr = "0001111" and i2c_active = '1' and i2c_busy = '0' then
          i2c_reconf_en <= '1';
        end if;      
 
      when others => null;
    end case;

    cfg_error_o <= i2c_error;

    sr_count <= 0;

    cfg_valid <= '1';
    
    case cfg_data(5 downto 2) is
      when SR_8kHZ => sr_count <= SR_Count_8kHZ-1;   -- 1536;
      when SR_32kHZ => sr_count <= SR_Count_32kHZ-1;  -- 384;
      when SR_48kHZ => sr_count <= SR_Count_48kHZ-1;  -- 256;
      when SR_96kHZ => sr_count <= SR_Count_96kHZ-1;  -- 128;
      when others => null;  -- invalid
    end case;

    --check whether cfg_data_i is valid
    cfg_sr_valid <= '0';
    if cfg_data_i(5 downto 2) = SR_8kHZ or
       cfg_data_i(5 downto 2) = SR_32kHZ or
       cfg_data_i(5 downto 2) = SR_48kHZ or
       cfg_data_i(5 downto 2) = SR_96kHZ then
      cfg_sr_valid <= '1';              -- allowed sample rates
    end if;

    cfg_din <= cfg_data_i;
    if cfg_addr_i = "0000111" then
      cfg_din <= "00001" & "10" & "11";  -- digital audio interface control
    elsif cfg_addr_i = "0001000" then
      cfg_din <= "000" & cfg_data_i(5 downto 2) & "00";  -- sample rate control
      cfg_valid <= cfg_sr_valid;
    elsif cfg_addr_i = "0001111" and (i2c_active = '1' and i2c_busy = '0') then
      --i2c_reconf_en <= '1';         -- reset wm8731 device
      cfg_valid <= '0';
    end if;
  end process p_cfg2;


  p_clk : process(clk_i, reset_n_i)
  begin  -- process p_dat
    if reset_n_i = '0' then                 -- asynchronous reset (active low)
      mclk <= '0';
      mclk_count <= 0;
      sync_count <= to_unsigned(0,11);
      clk_edge_1d <= "00";
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if mclk_count = ClkPerMCLK-1 then
        mclk_count <= 0;
        if sync_count = unsigned(sync_count_max) then
          sync_count <= to_unsigned(0,11);
        else
          sync_count <= sync_count+1;
        end if;
      else
        mclk_count <= mclk_count+1;
      end if;

      if clk_edge(0) ='1' then
        mclk <= '1';
      elsif clk_edge(1) = '1' then
        mclk <= '0';       
      end if;

      clk_edge_1d <= clk_edge;
    end if;
  end process p_clk;

  p_dat : process(clk_i, reset_n_i)
  begin  -- process p_dat
    if reset_n_i = '0' then                 -- asynchronous reset (active low)
      lrc <= '0';
      frame_en <= '0';
      dac_reg <= (others => '0');
      adc_reg <= (others => '0');
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if clk_edge(0) = '1' then         -- rising edge
        if frame_en = '1' and lrc = '0' then           
          adc_reg <= adc_reg(46 downto 0) & wm_adc_i; -- adc data point
          if frame_full = '1' then
            frame_en <= '0';
          end if;
        end if;
      elsif clk_edge(1) = '1' then      -- falling edge
        lrc <= frame_sync;
        -- start of frame
        if frame_sync = '1' then
          dac_reg <= dsp_dacl_i(23 downto 0) & dsp_dacr_i(23 downto 0);
          frame_en <= '1';
        elsif frame_en = '1' and frame_full = '0' and lrc = '0' then      -- dac_dat
          dac_reg <= dac_reg(46 downto 0) & '0';
        end if;
      end if;
      
    end if;
  end process p_dat;

  sync_count_max <= To_StdULogicVector(std_logic_vector(samp_rate)) & "1111111";

  p_out : process (lrc, frame_full, frame_sync, mclk, mclk_count, sync_count, dac_reg, adc_reg, clk_edge, clk_edge_1d)
  begin  -- process p_out
    frame_sync <= '0';
    frame_full <= '0';

    if sync_count(10 downto 6) = to_unsigned(0,5) then
      if sync_count(5 downto 0) = to_unsigned(0,6) then
        frame_sync <= '1';
      elsif sync_count(5 downto 0) = to_unsigned(49,6) then  -- total data bits + 1
        frame_full <= '1';
      end if;
    end if;

    clk_edge <= "00";
    
    if mclk_count = MCLK_RISING then
      clk_edge(0) <= '1';
    elsif mclk_count = MCLK_FALLING then
      clk_edge(1) <= '1';      
    end if;

    dsp_dac_sync_o <= clk_edge(1) and frame_sync;
    dsp_adc_sync_o <= clk_edge_1d(0) and frame_full;

    wm_dac_o <= dac_reg(47);

    wm_mclk_o <= mclk;
    wm_bclk_o <= mclk;
    
    dsp_adcl_o <= adc_reg(47 downto 24);
    dsp_adcr_o <= adc_reg(23 downto 0);
  
    wm_dac_lrc_o <= lrc;
    wm_adc_lrc_o <= lrc;
    
  end process p_out;  
  
end rtl;
