#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <set>
#include <map>
#include <vector>

class IncludeResolver
{
public:
  IncludeResolver(const std::string &filename)
  {
    curLine_=0;
    resolveIncludes(filename);    
  }
  
  void storeResolvedCode(const std::string &filename)
  {
    std::fstream f(filename,std::ios::out|std::ios::trunc);
    
    if(f.is_open())
    {
      f << resolvedCode_;
    }
  }
  
  std::string getResolvedCode() const
  {
    return resolvedCode_;
  }
  
  
  const std::map<uint32_t,std::string>& getLineToFileMap() const
  {
    return lineToFileMap_;
  }
  
  const std::map<std::string,std::vector<std::string>>& getFilesAsLineVectors() const
  {
    return includedFiles_;
  }
  
protected:
  void resolveIncludes(const std::string &file)
  {
    std::string filename=file;
    
    std::cout<<"filename: "<<(filename)<<"\n";
    std::fstream f(filename,std::ios::in);
    
    if(!f.is_open())
    {
      std::cout<<"File '"<<(filename)<<"' not found\n";
      return;
    }
    
    std::string data;
    
    f.seekg(0,std::ios::end);
    uint32_t size=f.tellg();
    f.seekg(0,std::ios::beg);
    
    data.resize(size);
    f.read(const_cast<char*>(data.c_str()),size);    

    std::string path="";
    std::string::size_type slashPos=filename.rfind('/');
    if(slashPos != std::string::npos)
    {
      path=filename.substr(0,slashPos);
      filename=filename.substr(slashPos+1);
    }
    
    std::vector<std::string> &lines=includedFiles_.insert(std::make_pair(filename,std::vector<std::string>())).first->second;
    lineToFileMap_.insert(std::make_pair(curLine_,filename));
    
    std::regex regex("include\\((.*)\\)");
    
    uint32_t beg=0;
    uint64_t pos=data.find('\n');
    while(pos != std::string::npos)
    {
      std::string line=data.substr(beg,pos-beg+1);
      lines.push_back(line);
      beg=pos+1;
      pos=data.find('\n',beg);
      
      std::cout<<"line: "<<(line)<<"\n";
      
      std::smatch m;
      if(std::regex_search(line,m,regex))
      {
        std::string toReplace=m[0].str();
        std::cout<<"toReplace: "<<(toReplace)<<"\n";
        
        line.replace(line.find(toReplace),toReplace.length(),"");
        resolvedCode_+=line;
        ++curLine_;
        
        //check if already included
        if(includedFiles_.find(toReplace) != includedFiles_.end())
        {          
          continue;
        }
        
        resolveIncludes(path + "/" + m[1].str());
        lineToFileMap_.insert(std::make_pair(curLine_,filename));
      }
      else
      {
        resolvedCode_+=line;
        ++curLine_;
      }
    }
  }
  
  std::string resolvedCode_;
  uint32_t curLine_;
  std::set<std::string> alreadyIncludedFiles_;
  std::map<std::string,std::vector<std::string> > includedFiles_;
  std::map<uint32_t,std::string> lineToFileMap_;
};
