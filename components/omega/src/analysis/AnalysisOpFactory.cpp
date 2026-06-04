#include "AnalysisOpFactory.h"
#include <iostream>

namespace OMEGA {

std::map<std::string, AnalysisOpFactory::CreatorFunc> AnalysisOpFactory::Registry;

void AnalysisOpFactory::registerOperator(const std::string& Type, 
                                          CreatorFunc creator) {

   auto &Reg = registry();

   std::cout << "REGISTER: Operator '" << Type << "' registered\n";

   // Check for duplicate registration
   if (Reg.find(Type) != Reg.end()) {
      ABORT_ERROR(
         "AnalysisOpFactory: Operator type {} is already registered", Type);
   }

  Reg[Type] = creator;
}

std::unique_ptr<AnalysisOperator> 
AnalysisOpFactory::createOp(const std::string& Type,
                          const std::string& Name,
                          const Config& Cfg) {

   auto &Reg = registry();

   auto it = Reg.find(Type);
   
   if (it == Reg.end()) {
//      // Build helpful error message with suggestions
//      std::ostringstream oss;
//      oss << "DiagOperatorFactory: Unknown operator type '" << type << "'.\n";
//      oss << "Available operators: ";
      
//      auto available = availableOperators();
//      for (size_t i = 0; i < available.size(); ++i) {
//         oss << available[i];
//         if (i < available.size() - 1) oss << ", ";
//      }
      
      ABORT_ERROR("Operator not found");
   }
   
   // Call the registered creator function
   return it->second(Name, Cfg);
}

bool AnalysisOpFactory::hasOperator(const std::string& Type) {
   auto &Reg = registry();
   return Reg.find(Type) != Reg.end();
}

}
