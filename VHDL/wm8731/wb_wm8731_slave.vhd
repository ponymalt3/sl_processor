library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library work;
use work.wishbone_p.all;

entity wb_wm8731_slave is
  
  port (
    clk_i : in std_ulogic;
    reset_n_i : in std_ulogic;

    slave_i : in wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t;

    -- wm8731
    cfg_addr_o  : out  std_ulogic_vector(6 downto 0);
    cfg_data_o  : out  std_ulogic_vector(8 downto 0);
    cfg_idle_i  : in std_ulogic;
    cfg_en_o    : out  std_ulogic;
    cfg_error_i : in std_ulogic;

    dsp_dacl_o    : out  std_ulogic_vector(23 downto 0);
    dsp_dacr_o    : out  std_ulogic_vector(23 downto 0);
    dsp_dac_sync_i : in std_ulogic;
    
    dsp_adcl_i    : in std_ulogic_vector(23 downto 0);
    dsp_adcr_i    : in std_ulogic_vector(23 downto 0);    
    dsp_adc_sync_i : in std_ulogic);
end wb_wm8731_slave;

architecture rtl of wb_wm8731_slave is

  type sfr_t is (SFR_NONE,SFR_RD_CTRL,SFR_WR_CTRL,SFR_RD_DATL,SFR_WR_DATL,SFR_RD_DATR,SFR_WR_DATR);

  constant ADDR_CTRL : unsigned(1 downto 0) := to_unsigned(0,2);
  constant ADDR_DATL : unsigned(1 downto 0) := to_unsigned(1,2);
  constant ADDR_DATR : unsigned(1 downto 0) := to_unsigned(2,2);

  signal sfr_cmd : sfr_t;

  signal datl_in_reg : std_ulogic_vector(23 downto 0);
  signal datr_in_reg : std_ulogic_vector(23 downto 0);
  signal datl_out_reg : std_ulogic_vector(23 downto 0);
  signal datr_out_reg : std_ulogic_vector(23 downto 0);

  signal ch_full_l : std_ulogic;
  signal ch_full_r : std_ulogic;
  
  signal adc_sync : std_ulogic;
  signal dac_sync : std_ulogic;
  
  signal wm8731_conf_en : std_ulogic;

  signal slave_out : wb_slave_ifc_out_t;
  
begin  -- rtl

  slave_o <= slave_out;

  process (slave_i,slave_out)
  begin  -- process
    sfr_cmd <= SFR_NONE;

    if slave_i.cyc = '1' and slave_i.stb = '1' and slave_out.ack = '0' then
      case slave_i.adr(1 downto 0) is
        when ADDR_CTRL =>
          if slave_i.we = '1' then
            sfr_cmd <= SFR_WR_CTRL;
          else
            sfr_cmd <= SFR_RD_CTRL;
          end if;
        when ADDR_DATL =>
          if slave_i.we = '1' then
            sfr_cmd <= SFR_WR_DATL;
          else
            sfr_cmd <= SFR_RD_DATL;
          end if;
        when ADDR_DATR =>
          if slave_i.we = '1' then
            sfr_cmd <= SFR_WR_DATR;
          else
            sfr_cmd <= SFR_RD_DATR;
          end if;
        when others => null;
      end case;
    end if;
  end process;

  -- sfr_ctrl x x x x x dac_sync adc_sync error idle
 
  process (clk_i, reset_n_i)
   begin  -- process
     if reset_n_i = '0' then            -- asynchronous reset (active low)
       dsp_dacl_o <= (others => '0');
       dsp_dacr_o <= (others => '0');
       adc_sync <= '0';
       dac_sync <= '0';
       ch_full_l <= '0';
       ch_full_r <= '0';
       slave_out <= ((others => '0'),'0','0','1');
       datl_in_reg <= (others => '0');
       datr_in_reg <= (others => '0');
       datl_out_reg <= (others => '0');
       datr_out_reg <= (others => '0');
     elsif clk_i'event and clk_i = '1' then  -- rising clock edge
       slave_out.ack <= slave_out.stall;
       slave_out.stall <= '0';
       slave_out.err <= '0';
       case sfr_cmd is
         when SFR_RD_CTRL =>
           slave_out.dat <= (31 downto 4 => '0') & dac_sync & adc_sync & cfg_error_i & cfg_idle_i;
           dac_sync <= '0';
           adc_sync <= '0';           
         when SFR_WR_DATL =>
           datl_in_reg <= slave_i.dat(23 downto 0);
           ch_full_l <= '1';
         when SFR_WR_DATR =>
           datr_in_reg <= slave_i.dat(23 downto 0);
           ch_full_r <= '1';
         when SFR_RD_DATL =>
           slave_out.dat <= X"60" & datl_out_reg;
         when SFR_RD_DATR =>
           slave_out.dat <= X"60" & datr_out_reg;
         when SFR_NONE =>
           slave_out.ack <= '0';
           slave_out.stall <= '1';
         when others => null;
       end case;

       if dsp_dac_sync_i = '1' then
         dac_sync <= '1';
         dsp_dacl_o <= (others => '0');
         dsp_dacr_o <= (others => '0');
       elsif dsp_adc_sync_i = '1' then
         adc_sync <= '1';
         datl_out_reg <= dsp_adcl_i;
         datr_out_reg <= dsp_adcr_i;
       end if;

       if ch_full_l = '1' and ch_full_r = '1' then
         dsp_dacl_o <= datl_in_reg;
         dsp_dacr_o <= datr_in_reg;
         ch_full_l <= '0';
         ch_full_r <= '0';
       end if;
       
     end if;
   end process;

   process(cfg_idle_i, sfr_cmd, slave_i, wm8731_conf_en)
   begin  -- process

     wm8731_conf_en <= '0';

     if slave_i.dat(30 downto 29) = "11" or slave_i.dat(3) = '1' then
       wm8731_conf_en <= '1';
     end if;
     
     cfg_en_o <= '0';
     cfg_addr_o <= slave_i.dat(22 downto 16);
     cfg_data_o <= slave_i.dat(8 downto 0);

     if sfr_cmd = SFR_WR_CTRL and wm8731_conf_en = '1' then
       cfg_en_o <= cfg_idle_i;
     end if;
     
   end process;

end rtl;
