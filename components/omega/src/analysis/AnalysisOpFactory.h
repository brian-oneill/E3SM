#ifndef OMEGA_ANALYSISOPFACTORY_H
#define OMEGA_ANALYSISOPFACTORY_H

//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Config.h"

#include <string>

namespace OMEGA {

// Define array types for analysis operators
#define OMEGA_ANALYSIS_ARRAY_TYPES(X)                                    \
   /* 1D Arrays */                                                       \
   X(ArrayDataType::I4, 1, ArrayMemLoc::Both, Array1DI4)                 \
   X(ArrayDataType::I4, 1, ArrayMemLoc::Device, Array1DI4)               \
   X(ArrayDataType::I4, 1, ArrayMemLoc::Host, HostArray1DI4)             \
   X(ArrayDataType::I8, 1, ArrayMemLoc::Both, Array1DI8)                 \
   X(ArrayDataType::I8, 1, ArrayMemLoc::Device, Array1DI8)               \
   X(ArrayDataType::I8, 1, ArrayMemLoc::Host, HostArray1DI8)             \
   X(ArrayDataType::R4, 1, ArrayMemLoc::Both, Array1DR4)                 \
   X(ArrayDataType::R4, 1, ArrayMemLoc::Device, Array1DR4)               \
   X(ArrayDataType::R4, 1, ArrayMemLoc::Host, HostArray1DR4)             \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Both, Array1DR8)                 \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Device, Array1DR8)               \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Host, HostArray1DR8)             \
   /* 2D Arrays */                                                       \
   X(ArrayDataType::I4, 2, ArrayMemLoc::Both, Array2DI4)                 \
   X(ArrayDataType::I4, 2, ArrayMemLoc::Device, Array2DI4)               \
   X(ArrayDataType::I4, 2, ArrayMemLoc::Host, HostArray2DI4)             \
   X(ArrayDataType::I8, 2, ArrayMemLoc::Both, Array2DI8)                 \
   X(ArrayDataType::I8, 2, ArrayMemLoc::Device, Array2DI8)               \
   X(ArrayDataType::I8, 2, ArrayMemLoc::Host, HostArray2DI8)             \
   X(ArrayDataType::R4, 2, ArrayMemLoc::Both, Array2DR4)                 \
   X(ArrayDataType::R4, 2, ArrayMemLoc::Device, Array2DR4)               \
   X(ArrayDataType::R4, 2, ArrayMemLoc::Host, HostArray2DR4)             \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Both, Array2DR8)                 \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Device, Array2DR8)               \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Host, HostArray2DR8)             \
   /* 3D Arrays */                                                       \
   X(ArrayDataType::I4, 3, ArrayMemLoc::Both, Array3DI4)                 \
   X(ArrayDataType::I4, 3, ArrayMemLoc::Device, Array3DI4)               \
   X(ArrayDataType::I4, 3, ArrayMemLoc::Host, HostArray3DI4)             \
   X(ArrayDataType::I8, 3, ArrayMemLoc::Both, Array3DI8)                 \
   X(ArrayDataType::I8, 3, ArrayMemLoc::Device, Array3DI8)               \
   X(ArrayDataType::I8, 3, ArrayMemLoc::Host, HostArray3DI8)             \
   X(ArrayDataType::R4, 3, ArrayMemLoc::Both, Array3DR4)                 \
   X(ArrayDataType::R4, 3, ArrayMemLoc::Device, Array3DR4)               \
   X(ArrayDataType::R4, 3, ArrayMemLoc::Host, HostArray3DR4)             \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Both, Array3DR8)                 \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Device, Array3DR8)               \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Host, HostArray3DR8)             \
   /* 4D Arrays */                                                       \
   X(ArrayDataType::I4, 4, ArrayMemLoc::Both, Array4DI4)                 \
   X(ArrayDataType::I4, 4, ArrayMemLoc::Device, Array4DI4)               \
   X(ArrayDataType::I4, 4, ArrayMemLoc::Host, HostArray4DI4)             \
   X(ArrayDataType::I8, 4, ArrayMemLoc::Both, Array4DI8)                 \
   X(ArrayDataType::I8, 4, ArrayMemLoc::Device, Array4DI8)               \
   X(ArrayDataType::I8, 4, ArrayMemLoc::Host, HostArray4DI8)             \
   X(ArrayDataType::R4, 4, ArrayMemLoc::Both, Array4DR4)                 \
   X(ArrayDataType::R4, 4, ArrayMemLoc::Device, Array4DR4)               \
   X(ArrayDataType::R4, 4, ArrayMemLoc::Host, HostArray4DR4)             \
   X(ArrayDataType::R8, 4, ArrayMemLoc::Both, Array4DR8)                 \
   X(ArrayDataType::R8, 4, ArrayMemLoc::Device, Array4DR8)               \
   X(ArrayDataType::R8, 4, ArrayMemLoc::Host, HostArray4DR8)             \
   /* 5D Arrays */                                                       \
   X(ArrayDataType::I4, 5, ArrayMemLoc::Both, Array5DI4)                 \
   X(ArrayDataType::I4, 5, ArrayMemLoc::Device, Array5DI4)               \
   X(ArrayDataType::I4, 5, ArrayMemLoc::Host, HostArray5DI4)             \
   X(ArrayDataType::I8, 5, ArrayMemLoc::Both, Array5DI8)                 \
   X(ArrayDataType::I8, 5, ArrayMemLoc::Device, Array5DI8)               \
   X(ArrayDataType::I8, 5, ArrayMemLoc::Host, HostArray5DI8)             \
   X(ArrayDataType::R4, 5, ArrayMemLoc::Both, Array5DR4)                 \
   X(ArrayDataType::R4, 5, ArrayMemLoc::Device, Array5DR4)               \
   X(ArrayDataType::R4, 5, ArrayMemLoc::Host, HostArray5DR4)             \
   X(ArrayDataType::R8, 5, ArrayMemLoc::Both, Array5DR8)                 \
   X(ArrayDataType::R8, 5, ArrayMemLoc::Device, Array5DR8)               \
   X(ArrayDataType::R8, 5, ArrayMemLoc::Host, HostArray5DR8)

/// The AnalysisOpFactory
class AnalysisOpFactory{
 public:

   using CreatorFunc = std::function<std::unique_ptr<AnalysisOperator>(
       const std::vector<std::string> &UpstreamNames, Config Options)>;

   /// Register an operator type. Associates a sting label with a constructor
   /// for an AnalysisOperator templated on a particular Array type
   static void registerOperator(
       const std::string &Label, ///< [in] label for an operator type
       CreatorFunc Creator       ///< [in] constructor for an AnalysisOperator
   );

   /// Create an operator instance
   static std::unique_ptr<AnalysisOperator> createOp(
       const std::string &OpType,
       const std::vector<std::string> &UpstreamNames,
       Config Options
   );

   /// Query available operators (for validation, error messages)
   static std::vector<std::string> availableOperators();

   /// Check if operator type is registered
   static bool hasOperator(const std::string &Type);

   /// Register all array type variants of a template operator class
   template<template<typename> class OperatorTemplate>
   static void registerAllArrayVariants(const std::string& baseName) {
      #define REGISTER_VARIANT(dtype, rank, memloc, ArrayT) \
         registerOperator( \
            baseName + "_" #ArrayT + "_" #memloc, \
            [](const std::vector<std::string> &names, Config c) { \
               return std::make_unique<OperatorTemplate<ArrayT>>(names, c); \
            });
      
      OMEGA_ANALYSIS_ARRAY_TYPES(REGISTER_VARIANT)
      #undef REGISTER_VARIANT
   } // end registerAllArrayVariants


 private:
   /// The Registry regsiters all the AnalysisOperator variants. It is
   /// implemented as a Meyer's Singleton, so it is guaranteed to be
   ///  initialized before use
   static std::map<std::string, CreatorFunc>& registry() {
      static std::map<std::string, CreatorFunc> Registry;
      return Registry;
   }

   ///
   static std::string getArrayTypeName(ArrayDataType DType,
                                       I4 Rank,
                                       ArrayMemLoc MemLoc);

}; // end class AnalysisOpFactory

} // end namespace OMEGA

/// Usage in operator implementation file:
///   REGISTER_ANALYSIS_OPERATOR(GlobalMinOp, "global_min");
///
/// This creates a static initializer that registers the operator
/// before main() runs.
#define REGISTER_ANALYSIS_OPERATOR(Type, Name) \
  namespace { \
    static bool registered_##Type = []() { \
      ::OMEGA::AnalysisOpFactory::registerOperator(Name, \
        [](const std::string& n, ::OMEGA::Config c) { \
          return std::make_unique<Type>(n, c); \
        }); \
      return true; \
    }(); \
  }

#endif
