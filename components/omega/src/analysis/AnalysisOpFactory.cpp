#include "AnalysisOpFactory.h"

namespace OMEGA {

std::map<std::string, AnalysisOpFactory::CreatorFunc> AnalysisOpFactory::Registry;

void AnalysisOpFactory::registerOperator(const std::string& Type, 
                                          CreatorFunc creator) {

   // Check for duplicate registration
   if (Registry.find(Type) != Registry.end()) {
      ABORT_ERROR(
         "AnalysisOpFactory: Operator type {} is already registered", Type);
   }

  Registry[Type] = creator;
}

std::unique_ptr<AnalysisOperator> 
AnalysisOpFactory::createOp(const std::string& Type,
                          const std::string& Name,
                          const Config& Cfg) {
   auto it = Registry.find(Type);
   
   if (it == Registry.end()) {
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

}
