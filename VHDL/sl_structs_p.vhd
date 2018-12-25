library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package sl_structs_p is

  subtype reg_pc_t is unsigned(15 downto 0);
  subtype reg_raw_t is std_ulogic_vector(31 downto 0);
  subtype reg_addr_t is unsigned(31 downto 0);

  type reg_addr_array_t is array (natural range <>) of reg_addr_t;
  type reg_raw_array_t is array (natural range <>) of reg_raw_t;
  
  type req_qfp_t is record
    sign : std_ulogic;
    exp  : unsigned(1 downto 0);
    mant : std_ulogic_vector(28 downto 0);
  end record req_qfp_t;

  constant CMD_MOV : std_ulogic_vector(3 downto 0) := "0000";
  constant CMD_CMP : std_ulogic_vector(3 downto 0) := "0001";
  constant CMD_ADD : std_ulogic_vector(3 downto 0) := "0010";
  constant CMD_SUB : std_ulogic_vector(3 downto 0) := "0011";
  constant CMD_MUL : std_ulogic_vector(3 downto 0) := "0100";
  constant CMD_DIV : std_ulogic_vector(3 downto 0) := "0101";
  constant CMD_LOG2 : std_ulogic_vector(3 downto 0) := "1000";
  constant CMD_SHFT : std_ulogic_vector(3 downto 0) := "1001";
  constant CMD_INVALID : std_ulogic_vector(3 downto 0) := "1111";

  constant CMP_EQ : std_ulogic_vector(1 downto 0) := "00";
  constant CMP_NEQ : std_ulogic_vector(1 downto 0) := "01";
  constant CMP_LT : std_ulogic_vector(1 downto 0) := "10";
  constant CMP_LE : std_ulogic_vector(1 downto 0) := "11";

  constant MUX1_RESULT : std_ulogic := '0';
  constant MUX1_MEM : std_ulogic := '1';
  constant MUX2_MEM : std_ulogic := '0';
  constant MUX2_IRS : std_ulogic := '1';

  constant WBREG_AD0 : std_ulogic_vector(1 downto 0) := "00";
  constant WBREG_AD1 : std_ulogic_vector(1 downto 0) := "01";
  constant WBREG_IRS : std_ulogic_vector(1 downto 0) := "10";
  constant WBREG_NONE : std_ulogic_vector(1 downto 0) := "11";

  type sl_mem1_t is record
    external_data : reg_raw_t;
  end record sl_mem1_t;

  type sl_mem2_t is record
    read_data : reg_raw_array_t(1 downto 0);
    wr_addr : reg_addr_t;
  end record sl_mem2_t;
  
  type sl_code_fetch_t is record
    data : std_ulogic_vector(15 downto 0);
    pc : reg_pc_t;
  end record sl_code_fetch_t;

  type sl_decode_t is record
    mux_ad0 : std_ulogic;
    mux_ad1 : std_ulogic;
    mux_a : std_ulogic;  -- shortcut RESULT
    mux_b : std_ulogic;  -- select LOOP for MEM only
    en_mem : std_ulogic;
    en_irs : std_ulogic;
    en_reg : std_ulogic;
    cmd : std_ulogic_vector(3 downto 0);
    wb_reg : std_ulogic_vector(1 downto 0);
    c_data : std_ulogic_vector(9 downto 0);
    c_data_ext : std_ulogic_vector(1 downto 0);
    en_ad0 : std_ulogic;
    en_ad1 : std_ulogic;

    goto : std_ulogic;
    goto_const : std_ulogic;
    load : std_ulogic;
    cmp : std_ulogic;
    neg : std_ulogic;
    trunc : std_ulogic;
    wait1 : std_ulogic;
    signal1 : std_ulogic;
    loop1 : std_ulogic;

    cmp_mode : std_ulogic_vector(1 downto 0);
    cmp_noX_cy : std_ulogic;
    irs_addr : unsigned(15 downto 0);
    mem_ex : std_ulogic;
    cur_pc : reg_pc_t;
    jmp_back : std_ulogic;
    jmp_target_pc : reg_pc_t;
    inc_ad0 : std_ulogic;
    inc_ad1 : std_ulogic;
  end record sl_decode_t;

  type sl_decode_ex_t is record
    cmd : std_ulogic_vector(3 downto 0);
    
    memX : reg_raw_t;

    mux0 : std_ulogic;

    wr_addr : reg_addr_t;
    wr_en : std_ulogic;
    wr_ext : std_ulogic;
    wb_en : std_ulogic;
    wb_reg : std_ulogic_vector(1 downto 0);

    cmp : std_ulogic;
    cmp_mode : std_ulogic_vector(1 downto 0);

    goto : std_ulogic;

    neg : std_ulogic;
    trunc : std_ulogic;

    load : std_ulogic;
    load_data : std_ulogic_vector(11 downto 0);

    stall : std_ulogic;
  end record sl_decode_ex_t;

  constant S_FETCH : natural := 2;
  constant S_DEC : natural := 1;
  constant S_DECEX : natural := 1;
  constant S_EXEC : natural := 0;

  type sl_state_t is record
    pc : reg_pc_t;
    addr : reg_addr_array_t(1 downto 0);
    irs : reg_addr_t;

    load_state : std_ulogic_vector(2 downto 0);

    enable : std_ulogic_vector(2 downto 0);
    stall_exec_1d : std_ulogic;

    loop_count : reg_raw_t;
    
    result : reg_raw_t;
    result_prefetch : std_ulogic; -- fetch result data when ready and set this flag (also used for const loading)

    inc_ad0 : std_ulogic;
    inc_ad1 : std_ulogic;
    
  end record sl_state_t;

  type sl_alu_t is record
    result : reg_raw_t;
    int_result : reg_raw_t;
    complete : std_ulogic;
    same_unit_ready : std_ulogic;
    idle : std_ulogic;
    cmp_lt : std_ulogic;
    cmp_eq : std_ulogic;
  end record sl_alu_t;

  type sl_exec_t is record
    result : reg_raw_t;
    complete : std_ulogic;
    int_result : reg_raw_t;
    exec_next : std_ulogic;
    stall : std_ulogic;
    flush : std_ulogic;
  end record sl_exec_t;

  type sl_stall_ctrl_t is record
    stall_decex : std_ulogic;
    stall_exec : std_ulogic;
    flush_pipeline : std_ulogic;
    enable : std_ulogic_vector(2 downto 0);
  end record sl_stall_ctrl_t;

  type sl_processor_t is record
    state  : sl_state_t;
    fetch : sl_code_fetch_t;
    dec    : sl_decode_t;
    decex : sl_decode_ex_t;
  end record sl_processor_t;

end package sl_structs_p;
