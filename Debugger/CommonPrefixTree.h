#pragma once

#include <map>
#include <list>
#include <string>

class CommonPrefixTree
{
public:
  CommonPrefixTree()
  {
    root_=new _Entry();
  }
  
  ~CommonPrefixTree()
  {
    std::list<_Entry*> nodesToDelete;
    nodesToDelete.push_back(root_);
    while(nodesToDelete.size() > 0)
    {
      _Entry *e=nodesToDelete.back();
      nodesToDelete.pop_back();
      
      for(auto &i : e->links_)
      {
        nodesToDelete.push_back(i.second);
      }
      
      delete e;
    }
  }
  
  void insert(const std::string &s)
  {
    std::string extS=s;
    extS.resize(s.size()+1,'\0');
    root_->insert(extS);    
  }
  
  std::string getCommonPrefix(std::string s)
  {
    _Entry *e=root_;
    while(true)
    {
      uint32_t dif=difStrings(s,e->common_);
      
      if(dif == e->common_.length() && dif < s.length())
      {
        auto it=e->links_.find(s.at(e->common_.length()));
        if(it != e->links_.end())
        {
          s=s.substr(e->common_.length()+1);
          e=it->second;
          continue;
        }
        return "";      
      }
      
      if(dif == s.length())
      {
        //leaf have a '\0' at the end => remove
        std::string result=e->common_.substr(s.length());
        if(e->links_.size() == 0)
        {
          result=result.substr(0,result.size()-1);
        }
        return result;
      }
      
      return "";
    }
  }
  
protected:
  static uint32_t difStrings(const std::string &a,const std::string &b)
  {
    uint32_t i=0;
    for(;i<std::min(a.length(),b.length());++i)
    {
      if(a[i] != b[i])
      {
        return i;
      }
    }
    
    return i;
  }
  
  struct _Entry
  {
    void insert(const std::string &s)
    {
      uint32_t i=CommonPrefixTree::difStrings(s,common_);
      
      if(common_.length() == i)
      {
        //common prefix
        std::string s2=s.substr(common_.length()+1);
        
        if(links_.find(s.at(i)) != links_.end())
        {
          links_.find(s.at(i))->second->insert(s2);
        }
        else
        {
          _Entry *e=new _Entry();
          e->common_=s2;
          links_.insert(std::make_pair(s.at(i),e));
        }
        return;
      }
      
      //split this node
      _Entry *e1=new _Entry();
      e1->common_=common_.substr(i+1);
      e1->links_=links_;
      
      links_.clear();
      links_.insert(std::make_pair(common_.at(i),e1));
      
      _Entry *e2=new _Entry();
      e2->common_=s.substr(i+1);
      
      links_.insert(std::make_pair(s.at(i),e2));
      
      common_=common_.substr(0,i);

      return;      
    }
    
    std::string common_;
    std::map<char,_Entry*> links_;
  };
  
  _Entry *root_;
};