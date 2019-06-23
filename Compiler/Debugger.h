#pragma once

#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "../SLProcessor.h"

#include "DebuggerInterface.h"

class Debugger
{
public:
  Debugger(DebuggerInterface *interface,
           const SymbolMap &globalSymbols,
           const std::map<std::string,CodeGen::_FunctionInfo> &functionMap,
           const std::map<uint32_t,uint32_t> &addrToLineMap,
           const std::map<uint32_t,std::pair<std::string,uint32_t>> &lineToFileMap,
           const std::map<std::string,std::vector<std::string>> &filesAsLineVector,
           const uint16_t *code,
           uint32_t codeSize,
           uint32_t codeAddr)
  :  global_(globalSymbols)
  {
    interface_=interface;
    functionMap_=functionMap;
    addrToLineMap_=addrToLineMap;
    lineToFileMap_=lineToFileMap;
    filesAsLineVector_=filesAsLineVector;
    
    code_=code;
    codeSize_=codeSize;
    codeAddr_=codeAddr;
    
    for(auto &i : functionMap_)
    {
      addrToFunctionMap_.insert(std::make_pair(i.second.address_,i.first));
    }
    
    for(auto &i : addrToLineMap_)
    {
      lineToAddrMap_.insert(std::make_pair(i.second,i.first));
    }
    
    for(auto &i : lineToFileMap_)
    {
      fileToLineMap_.insert(std::make_pair(i.second.first,std::make_pair(i.first,i.second.second)));
    }
    
    callInfoValid_=false;
  }
  
  void loadCode()
  {
    interface_->writeCode(code_,codeSize_);
    interface_->reset();
  }
  
  std::string command(const std::string &command)
  {
    if(command.substr(0,9) == "show mem[")
    {
      std::stringstream ss(command.substr(9));
      uint32_t from;
      uint32_t to;
      ss>>from;
      ss>>to;
      std::istream::char_type delim;
      ss.read(&delim,1);
      if(from > to || delim != ']')
      {
        return "invalid format '" + command.substr(5) + "'";
      }
      
      uint32_t numWords=to-from+1;
      uint32_t buffer[numWords];
      if(!interface_->readMem(from,numWords,buffer))
      {
        return "fetch memory at addr " + std::to_string(from) + "failed";
      }
      
      std::stringstream ss2;
      for(uint32_t i=0;i<numWords;++i)
      {
        ss2<<qfp32_t::initFromRaw(buffer[i])<<"\n";
      }
      
      return ss2.str().substr(0,ss2.str().length()-1);      
    }
    if(command.substr(0,5) == "show ")
    {
      std::string subCommand=command.substr(5);
      
      if(subCommand.substr(0,9) == "callstack")
      {
        return getCallStack();
      }
      else
      {
        return getSymbolInfo(subCommand);
      }
    }
    else if(command.substr(0,4) == "show")
    {
      return getCodeInfo(150);
    }
    else if(command.substr(0,4) == "step" || command.substr(0,4) == "run ")
    {
      uint32_t codeAddr=interface_->getPC()+1;
      
      if(command.substr(0,4) == "run ")
      {
        if(command.at(4) >= '0' && command.at(4) <= '9')
        {
          std::stringstream ss(command.substr(4));
          ss >> codeAddr;
        }
        else
        {
          codeAddr=lineStringToAddr(command.substr(4));
        }
        
        if(codeAddr > codeSize_ || codeAddr == 0xFFFFFFFF)
        {
          return std::string("invalid string '") + command.substr(4) + "'";
        }
      }
      
      callInfoValid_=false;
      
      DebuggerInterface::_Run result=interface_->runToAddr(codeAddr);
      
      switch(result.result_)
      {
      case DebuggerInterface::_Run::ACCESS_FAULT_READ:
        return std::string("read fault at ") + std::to_string(result.addr_);
      case DebuggerInterface::_Run::ACCESS_FAULT_WRITE:
        return std::string("write fault at ") + std::to_string(result.addr_);
      case DebuggerInterface::_Run::BREAKPOINT_HIT:
        return std::string("hit break at ") + std::to_string(result.addr_);
      case DebuggerInterface::_Run::OK:
        return "ok";
      default:
        return "error";        
      }
    }
    else if(command.substr(0,5) == "reset")
    {
      interface_->reset();      
      return "ok";
    }
    else if(command.substr(0,5) == "badd ")
    {
      uint32_t codeAddr=lineStringToAddr(command.substr(5));
      
      bool breakpointOk=interface_->addBreakpoint(codeAddr);
      if(!breakpointOk)
      {
        return "failed to set breakpoint '" + command.substr(5) + "'";
      }
          
      return "ok";
    }
    else if(command.substr(0,4) == "brm ")
    {
      uint32_t codeAddr=lineStringToAddr(command.substr(4));
      
      bool breakpointOk=interface_->removeBreakpoint(codeAddr);
      if(!breakpointOk)
      {
        return "failed to remove breakpoint '" + command.substr(4) + "'";
      }
          
      return "ok";
    }
    else if(command.substr(0,4) == "quit")
    {
      return "";
    }
    
    return "invalid command";
  }
  
  uint32_t lineStringToAddr(const std::string &line)
  {
    uint32_t pos=line.find(":");
    if(pos == std::string::npos)
    {
      return 0xFFFFFFFF;
    }
    
    std::string file=line.substr(0,pos);
    
    uint32_t offsetInFile=0;
    std::stringstream ss(line.substr(pos+1));
    ss>>offsetInFile;
    
    --offsetInFile; //make zero based offset
    
    auto it=fileToLineMap_.find(file);
    
    if(it == fileToLineMap_.end())
    {
      return 0xFFFFFFFF;
    }
    
    std::pair<uint32_t,uint32_t> lineInfo={0U,0U};//line,offset cause file can exist more than once
    while(it != fileToLineMap_.end() && it->first == file)
    {
      std::cout<<"entry ["<<(it->first)<<"]:\n  "<<(it->second.first)<<"\n  "<<(it->second.second)<<"\n";
      if(it->second.first > lineInfo.first && it->second.second <= offsetInFile)
      {
        lineInfo=it->second;
      }
      ++it;
    }
    
    uint32_t totalLine=lineInfo.first-lineInfo.second+offsetInFile;
    
    auto j=lineToAddrMap_.lower_bound(totalLine);
    if(j == lineToAddrMap_.end())
    {
      return 0xFFFFFFFF;
    }
    
    return j->second;
  }
  
  std::string getFunctionFromAddr(uint32_t addr)
  {
    auto it=--(addrToFunctionMap_.upper_bound(addr));
    
    CodeGen::_FunctionInfo func=functionMap_.find(it->second)->second;
    
    if(func.address_ > addr || addr > (func.address_+func.size_))
    {
      return "";
    }
    
    return it->second;
  }
  
  uint32_t getLineFromAddr(uint32_t addr)
  {
    auto it=addrToLineMap_.lower_bound(addr);
    
    if(it == addrToLineMap_.end())
    {
      return 0xFFFFFFFF;
    }
    
    return it->second;
  }
  
  std::string getSymbolInfo(const std::string &symbol)
  {
    //get IRS
    auto callInfo=fetchCallInfo();
    
    uint32_t irs=callInfo.back().irsAddr_;
    
    //get context
    std::string function=getFunctionFromAddr(interface_->getPC());
    
    if(function.length() == 0)
    {
      return "no context found";
    }
    
    const CodeGen::_FunctionInfo &fi=functionMap_.find(function)->second;
  
    uint32_t symRef=fi.symbols_->findSymbol(Stream::String(symbol.c_str(),0,symbol.length()));
    SymbolMap::_Symbol sym;
    
    if(symRef == SymbolMap::InvalidLink)
    {
      symRef=global_.findSymbol(Stream::String(symbol.c_str(),0,symbol.length()));
         
      if(symRef == SymbolMap::InvalidLink)
      {
        return std::string("symbol '") + symbol + "' not found in context <" + function + ">";
      }
      
      sym=global_[symRef];
    }
    else
    {
      sym=(*(fi.symbols_))[symRef];
    }
    
    bool markMaybeInvalid=interface_->getPC() > sym.lastAccess_;
    
    uint32_t numElements=1;
    if(sym.flagIsArray_)
    {
      numElements=sym.allocatedSize_;
    }
    
    qfp32_t data[numElements];
    if(sym.flagConst_ == 0)
    {
      bool readOk=interface_->readMem(irs+sym.allocatedAddr_,numElements,reinterpret_cast<uint32_t*>(data));
    
      if(!readOk)
      {
        return std::string("reading '") + symbol + "' failed";
      }
    }
    else
    {
      data[0]=sym.constValue_.toRealQfp32();
    }
    
    std::stringstream ss;
    ss << symbol << " = ";
    if(sym.flagIsArray_)
    {
      ss<<"[";
    }
    
    if(markMaybeInvalid)
    {
      ss<<"\x1B[31m";
    }
    
    for(uint32_t i=0;i<numElements;++i)
    {
      ss<<data[i];
      if(i < (numElements-1))
      {
        ss<<" ";
      }
    }
    
    ss<<"\x1B[0m";
    
    if(sym.flagIsArray_)
    {
      ss<<"]";
    }
    
    return ss.str();          
  }
  
  std::string getCodeInfo(int32_t numLinesShow)
  {
    int32_t line=interface_->getPC();//addrToLineMap_.lower_bound(interface_->getPC())->second;
    
    int32_t from=std::max(line-numLinesShow,0);
    int32_t to=std::min(line+numLinesShow,(int32_t)codeSize_);
    
    std::vector<std::string> disasm=DisAsm::getLinesFromCode(code_+from,to-from+1);
    
    uint32_t x=disasm.size();
    
    std::string result;
    
    uint32_t j=from;
    auto i=addrToLineMap_.lower_bound(from);
    while(j <= to)
    {
      std::string s;
      if(addrToFunctionMap_.find(j) != addrToFunctionMap_.end())
      {
        result+="\x1B[36mfunction " + addrToFunctionMap_.find(j)->second + "\n\x1B[0m";
      }
      
      if(i != addrToLineMap_.end() && j == i->first)
      {
        uint32_t codeLine=i->second;
        ++i;
        
        if(codeLine == 0)
        {
          s="\x1B[36mentry vector";
        }
        else if(disasm[j-from] != "nop")
        {
          auto file=--(lineToFileMap_.upper_bound(codeLine));
          int32_t codeOffset=codeLine-file->first+file->second.second;
          std::string name=file->second.first;
          s+="\x1B[36m"+filesAsLineVector_.find(name)->second[codeOffset];
          s.replace(s.rfind("\n"),1,"  ");
          s+="(" + name + ":" + std::to_string(codeOffset+1) + ")";
        }
      }
      
      std::string out=std::to_string(j)+". "+disasm[j-from];
      if(j == line)
      {
        out="\x1B[31m"+out;
      }

      while(out.length() < 30) out.push_back(' ');
      out+=s+"\x1B[0m";
      result+=out;
      
      if(j != to)
      {
        result+="\n";
      }
      
      ++j;
    }
  
    return result;
  }
  
  std::string getCallStack()
  {
    auto callInfo=fetchCallInfo();
    
    std::string result;
    for(auto &i : callInfo)
    {
      std::string function=getFunctionFromAddr(i.addr_);
      uint32_t line=getLineFromAddr(i.addr_)+1;//make one based
      auto it=--lineToFileMap_.upper_bound(line);
      std::string file=it->second.first;
      line-=it->first-it->second.second;
      result+=function + " (" + file + ":" + std::to_string(line) + ")\n";
    }
    
    return result.substr(0,result.size()-1);
  }
  
  struct _CallInfo
  {    
    uint32_t addr_;
    uint32_t irsAddr_;
  };
  
  std::vector<_CallInfo> extractCallInfo(const uint32_t *localMem,uint32_t size)
  {
    std::vector<_CallInfo> callInfo;

    callInfo.push_back({0,0});
    
    int32_t curIRS=0;
    for(int32_t i=0;i<(size-4);++i)
    {
      if(localMem[i+2] == qfp32_t(curIRS).getAsRawUint())
      {
        int32_t jmpBackAddr=(int32_t)qfp32_t::initFromRaw(localMem[i+1]);
        if(jmpBackAddr >= 5 && jmpBackAddr < codeSize_ &&  code_[jmpBackAddr-1] == SLCode::Goto::create())
        {
          //check addr
          std::pair<qfp32_t,uint32_t> callAddr=getValueFromLoad(jmpBackAddr-2);
          
          if(addrToFunctionMap_.find((int32_t)(callAddr.first)) == addrToFunctionMap_.end())
          {
            continue;
          }
          
          if(localMem[i+0] != qfp32_t(i).getAsRawUint())
          {
            continue;
          }
          
          std::pair<qfp32_t,uint32_t> retAddr=getValueFromLoad(jmpBackAddr-2-callAddr.second-5);
          
          if(((int32_t)(retAddr.first)) != jmpBackAddr)
          {
            continue;
          }
          
          curIRS=(int32_t)qfp32_t::initFromRaw(localMem[i+0]);
          (--callInfo.end())->addr_=jmpBackAddr;
          callInfo.push_back({0,curIRS});
        }
      }
    }
    
    (--callInfo.end())->addr_=interface_->getPC();
    
    return callInfo;
  }
  
  std::pair<qfp32_t,uint32_t> getValueFromLoad(uint32_t addr)
  {
    _Instr instr[3]={{0,0},{0,0},{0,0}};
    int32_t i=0;
    for(;i<3;++i)
    {
      _Instr cur={code_[addr-i],0};
      if(!cur.isLoadAddr())
      {
        break;
      }
      
      instr[2]=instr[1];
      instr[1]=instr[0];
      instr[0]=cur;
    }
    
    return {_Instr::restoreValueFromLoad(instr[0],instr[1],instr[2]),i};
  }
  
  const std::vector<_CallInfo>& fetchCallInfo()
  {
    if(!callInfoValid_)
    {
      uint32_t buffer[512];
      if(interface_->readMem(0,512,buffer))
      {
        callInfoCache_=std::move(extractCallInfo(buffer,512));
        callInfoValid_=true;
      }
    }
    
    return callInfoCache_;
  }
  
protected:
  DebuggerInterface *interface_;
  SymbolMap global_;
  std::map<std::string,CodeGen::_FunctionInfo> functionMap_;
  std::map<uint32_t,std::pair<std::string,uint32_t> > lineToFileMap_;
  std::map<uint32_t,std::string> addrToFunctionMap_;
  std::map<uint32_t,uint32_t> addrToLineMap_;
  std::map<uint32_t,uint32_t> lineToAddrMap_;
  std::map<std::string,std::vector<std::string> > filesAsLineVector_;
  std::multimap<std::string,std::pair<uint32_t,uint32_t> > fileToLineMap_;
  const uint16_t *code_;
  uint32_t codeSize_;
  uint32_t codeAddr_;
  bool callInfoValid_;
  std::vector<_CallInfo> callInfoCache_;
};