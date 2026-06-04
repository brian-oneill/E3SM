#ifndef OMEGA_ANALYSISOPFACTORY_H
#define OMEGA_ANALYSISOPFACTORY_H

#include "AnalysisOperator.h"
#include "Config.h"

#include <string>

namespace OMEGA {

class AnalysisOpFactory{
 public:
   using CreatorFunc = std::function<std::unique_ptr<AnalysisOperator>(
       const std::string &Name, const Config &Options)>;

   /// Register an operator type
   static void registerOperator(const std::string &Type, CreatorFunc Creator);

   /// Create an operator instance
   static std::unique_ptr<AnalysisOperator> createOp(const std::string &Type,
                                                   const std::string &Name,
                                                   const Config &Options);

   /// Query available operators (for validation, error messages)
   static std::vector<std::string> availableOperators();

   /// Check if operator type is registered
   static bool hasOperator(const std::string &Type);

 private:
   // Static map of registered operators
   static std::map<std::string, CreatorFunc> Registry;

};

}

///
/// Usage in operator implementation file:
///   REGISTER_DIAG_OPERATOR(GlobalMinOp, "global_min");
///
/// This creates a static initializer that registers the operator
/// before main() runs.
#define REGISTER_DIAG_OPERATOR(Type, Name) \
  namespace { \
    static bool registered_##Type = []() { \
      OMEGA::AnalysisOpFactory::registerOperator(Name, \
        [](const std::string& n, const ::OMEGA::Config& c) { \
          return std::make_unique<Type>(n, c); \
        }); \
      return true; \
    }(); \
  }

#endif
