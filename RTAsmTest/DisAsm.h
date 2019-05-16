#pragma once

#include <string>
#include <sstream>

class DisAsm
{
public:
  static std::string getStringFromCode(uint16_t *code,uint32_t size)
  {
    std::string result="";
    for(uint32_t i=0;i<size;++i)
    {
      std::stringstream ss;
      ss<<i;
      result+=ss.str()+") ";
      std::string s;
      
      if((s=movInstrToString(code[i])) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if((s=opInstrToString(code[i])) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if((s=cmpInstrToString(code[i])) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if((s=gotoInstrToString(code[i],i)) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if((s=loadInstrToString(code[i],((i+1)<size)?code[i+1]:0,((i+2)<size)?code[i+2]:0)) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if((s=unaryInstrToString(code[i])) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if((s=loopInstrToString(code[i])) != "invalid")
      {
        result+=s + "\n";
        continue;
      }
      if(code[i] == 0xFFFF)
      {
        result+="nop\n";
        continue;
      }
      
      result+="invalid\n";
    }

    return result;
  }
  
  static std::string wbRegToString(uint32_t wbReg)
  {
    switch(wbReg&0x03)
    {
      case 0: return "a0";
      case 1: return "a1";
      case 2: return "irs";
      case 3: return "result";
    }
  }
  
  static std::string irsOffsetToString(uint32_t code)
  {
    return valueToString((code>>2)&0x1FF);
  }
  
  static std::string valueToString(uint32_t value)
  {
    std::stringstream ss;
    ss << value;
    return ss.str();
  }
  
  static std::string movInstrToString(uint16_t code)
  {
    if((code&0xE000) == 0x8000)
    {
      if(code&0x1000)
      {
        return std::string() + "[IRS+" + irsOffsetToString(code) + "] = result";
      }
      else
      {
        return wbRegToString(code&0x03) + " = [IRS+" + irsOffsetToString(code) + "]";
      }      
    }
    
    if((code&0xFF80) == 0xF000)
    {
      std::string s=wbRegToString((code>>5)&3);
      
      if(((code>>1)&1) == SLCode::MUX1_RESULT)
      {
        s+=" = result";
      }
      else
      {
        s+=" = [a" + valueToString((code>>2)&1) + ((code&0x10)?"++]":"]");
      }
      
      return s;
    }
    
    if((code&0xFF80) == 0xF080)
    {
      std::string s="[a" + valueToString(code&1) + (code&0x10?"++":"") + "]";
      
      if(((code>>3)&1) == SLCode::MUX2_IRS && ((code>>1)&1) == SLCode::MUX1_MEM)
      {
         return s + " = [a" + valueToString((code>>2)&1) + (code&0x10?"++":"") + "]";
      }
      
      return s + " = result";
    }
    
    return "invalid";
  }
  
  static std::string opToString(uint32_t op)
  {
    switch(op)
    {
      case 0: return "mov";
      case 1: return "cmp";
      case 2: return "+";
      case 3: return "-";
      case 4: return "*";
      case 5: return "/";
      case 6: return "unused";
      case 7: return "unused";
      case SLCode::CMD_SHFT: return "shft";
      case SLCode::CMD_LOG2: return "log2";
      default: return "invalid";
    }
  }
  
  static std::string derefAddrToString(uint32_t index,bool inc)
  {
    return "[a" + valueToString(index) + (inc?"++":"") + "]";
  }
  
  static std::string muxAToString(uint32_t mux,uint32_t aIndex,bool aInc)
  {
    return (mux == 0)?"result":derefAddrToString(aIndex,aInc);
  }
  
  static std::string muxBToString(uint32_t mux,uint32_t aIndex,bool aInc)
  {
    return mux?"invalid":derefAddrToString(aIndex,aInc);
  }
  
  static std::string opInstrToString(uint16_t code)
  {
    if((code&0x8000) == 0)
    {
      return std::string("result = ") + muxAToString(code&2,code&1,code&0x800) + " " + opToString((code>>12)&7) + " [IRS+" + irsOffsetToString(code) + "]";
    }
    
    if((code&0xF000) == 0xC000)
    {
      return std::string("result = ") + muxAToString(code&2,(code>>8)&1,code&0x200) + " " + opToString((code>>5)&7) + " " + muxBToString(code&8,(code>>2)&1,code&0x10);
    }
    
    return "invalid";
  }
  
  static std::string cmpModeToString(uint32_t cmpMode)
  {
    switch(cmpMode&3)
    {
      case 0: return "==";
      case 1: return "!=";
      case 2: return "<";
      case 3: return "<=";
    }
  }
  
  static std::string cmpInstrToString(uint16_t code)
  {
    if((code&0xF000) == 0xD000)
    {
      return std::string() + "result " + cmpModeToString(code&3) + " [IRS+" + irsOffsetToString(code) + "]";
    }
    
    return "invalid";
  }
  
  static std::string gotoInstrToString(uint16_t code,uint32_t addr)
  {
    if((code&0xF001) == 0xA001)
    {
      int32_t jmp=((code>>2)&0x1FF)+(((code>>11)&1)*0xFFFFFE00);      
      return std::string() + "goto " + valueToString(addr+jmp); 
    }
    
    if((code&0xF001) == 0xA000)
    {     
      return std::string() + "goto result"; 
    }
    
    return "invalid";
  }
  
  static std::string loadInstrToString(uint16_t code1,uint16_t code2=0,uint16_t code3=0)
  {
    if((code1&0xF000) == 0xB000)
    {
      uint32_t raw=((code1&0x7FF)<<19) + ((code1&0x800)<<20);
      
      if((code2&0xF000) == 0xB000)
      {
        raw+=((code2&0x7FF)<<8);
        raw+=((code2&0x800)<<19);
        
        if((code3&0xF000) == 0xB000)
        {
          raw+=(code3&0x0FF);
        }
      }
      
      std::stringstream ss;
      ss<<qfp32_t::initFromRaw(raw);
      return std::string("result = ") + ss.str();
    }
    
    return "invalid";
  }
  
  static std::string unaryInstrToString(uint16_t code)
  {
    if((code&0xFFC0) == 0xF100)
    {
      if(code&1)
      {
        return "result = neg result";
      }
      
      if(code&4)
      {
        return "result = trunc result";
      }
      
      switch(8+((code>>3)&0x7))
      {
      case SLCode::CMD_LOG2:
        return "result = log2(result)";
      default:
        return "result = unkown operation";
      }
    }
    
    return "invalid";
  }
  
  static std::string loopInstrToString(uint16_t code)
  {
    if((code&0xFFC0) == 0xF140)
    {
      return "loop result";
    }
    
    return "invalid";
  }
};