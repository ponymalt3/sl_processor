#pragma concept

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <set>

class IncludeResolver
{
public:
  IncludeResolver(const std::string &filename)
  {
    resolvedCode_=resolveIncludes(filename);    
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
  
protected:
  std::string resolveIncludes(const std::string &filename)
  {
    std::cout<<"filename: "<<(filename)<<"\n";
    std::fstream f(filename,std::ios::in);
    
    if(!f.is_open())
    {
      std::cout<<"File '"<<(filename)<<"' not found\n";
      return "";
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
    }
    
    std::regex regex("include\\((.*)\\)");
    std::smatch m;
    while(std::regex_search(data,m,regex))
    {
      std::string toReplace=m[0].str();
      
      if(alreadyIncludedFiles_.find(toReplace) != alreadyIncludedFiles_.end())
      {
        continue;
      }
      
      alreadyIncludedFiles_.insert(toReplace);
      std::string replaceData=resolveIncludes(path + "/" + m[1].str());
      data.replace(data.find(toReplace),toReplace.size(),replaceData);
    }
    
    return data;
  }
  
  std::string resolvedCode_;
  std::set<std::string> alreadyIncludedFiles_;  
};
