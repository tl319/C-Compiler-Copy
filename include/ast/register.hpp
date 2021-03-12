#ifndef register_hpp
#define register_hpp

#include <string>

class Register
{
private:
  const std::string RegName;
  std::string varname;
  bool isused;
public:
  Register(const std::string& RegName, const std::string& varname ="", bool isused = false) : RegName(RegName), varname(varname), isused(isused) {}
  const std::string getName() const {return RegName;}
  std::string getVarName() const {return varname;}
  void setVarName(const std::string& varname) {this->varname = varname;}
  bool isUsed() const {return isused;}
  void setIsused(bool isused) {this->isused = isused;}

};

#endif
