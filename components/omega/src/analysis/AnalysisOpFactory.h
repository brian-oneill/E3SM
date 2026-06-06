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

   /// Create an operator instance
   static std::unique_ptr<AnalysisOperator> createOp(const std::string &Type,
                                                     const std::string &InputName,
                                                     const std::string &Name,
                                                     const Config &Options);

   /// Query available operators (for validation, error messages)
   static std::vector<std::string> availableOperators();

   /// Check if operator type is registered
   static bool hasOperator(const std::string &Type);

   /// Register all array type variants of a template operator class
   template<template<typename> class OperatorTemplate>
   static void registerAllArrayVariants(const std::string& baseName) {
      #define REGISTER_VARIANT(dtype, rank, memloc, ArrayT) \
         registerOperator( \
            baseName + "_" #ArrayT, \
            [](const std::string& n, const Config& c) { \
               return std::make_unique<OperatorTemplate<ArrayT>>(n, c); \
            });
      
      OMEGA_ANALYSIS_ARRAY_TYPES(REGISTER_VARIANT)
      #undef REGISTER_VARIANT
   }


 private:
   // Static map of registered operators
   static std::map<std::string, CreatorFunc> Registry;
   // Meyer's Singleton: guaranteed to be initialized before use
   static std::map<std::string, CreatorFunc>& registry() {
      static std::map<std::string, CreatorFunc> s_registry;
      return s_registry;
   }

   static std::string getArrayTypeName(ArrayDataType DType,
                                       I4 Rank,
                                       ArrayMemLoc MemLoc);



};

}

// Define array types for analysis operators (1D, 2D, 3D only)
#define OMEGA_ANALYSIS_ARRAY_TYPES(X)                                    \
   /* 1D Arrays */                                                       \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Both, Array1DR8)                 \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Device, Array1DR8)               \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Host, HostArray1DR8)             \
   X(ArrayDataType::I4, 1, ArrayMemLoc::Both, Array1DI4)                 \
   X(ArrayDataType::I4, 1, ArrayMemLoc::Device, Array1DI4)               \
   X(ArrayDataType::I4, 1, ArrayMemLoc::Host, HostArray1DI4)             \
   X(ArrayDataType::I8, 1, ArrayMemLoc::Both, Array1DI8)                 \
   X(ArrayDataType::I8, 1, ArrayMemLoc::Device, Array1DI8)               \
   X(ArrayDataType::I8, 1, ArrayMemLoc::Host, HostArray1DI8)             \
   X(ArrayDataType::R4, 1, ArrayMemLoc::Both, Array1DR4)                 \
   X(ArrayDataType::R4, 1, ArrayMemLoc::Device, Array1DR4)               \
   X(ArrayDataType::R4, 1, ArrayMemLoc::Host, HostArray1DR4)             \
   /* 2D Arrays */                                                       \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Both, Array2DR8)                 \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Device, Array2DR8)               \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Host, HostArray2DR8)             \
   X(ArrayDataType::I4, 2, ArrayMemLoc::Both, Array2DI4)                 \
   X(ArrayDataType::I4, 2, ArrayMemLoc::Device, Array2DI4)               \
   X(ArrayDataType::I4, 2, ArrayMemLoc::Host, HostArray2DI4)             \
   X(ArrayDataType::I8, 2, ArrayMemLoc::Both, Array2DI8)                 \
   X(ArrayDataType::I8, 2, ArrayMemLoc::Device, Array2DI8)               \
   X(ArrayDataType::I8, 2, ArrayMemLoc::Host, HostArray2DI8)             \
   X(ArrayDataType::R4, 2, ArrayMemLoc::Both, Array2DR4)                 \
   X(ArrayDataType::R4, 2, ArrayMemLoc::Device, Array2DR4)               \
   X(ArrayDataType::R4, 2, ArrayMemLoc::Host, HostArray2DR4)             \
   /* 3D Arrays */                                                       \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Both, Array3DR8)                 \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Device, Array3DR8)               \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Host, HostArray3DR8)             \
   X(ArrayDataType::I4, 3, ArrayMemLoc::Both, Array3DI4)                 \
   X(ArrayDataType::I4, 3, ArrayMemLoc::Device, Array3DI4)               \
   X(ArrayDataType::I4, 3, ArrayMemLoc::Host, HostArray3DI4)             \
   X(ArrayDataType::I8, 3, ArrayMemLoc::Both, Array3DI8)                 \
   X(ArrayDataType::I8, 3, ArrayMemLoc::Device, Array3DI8)               \
   X(ArrayDataType::I8, 3, ArrayMemLoc::Host, HostArray3DI8)             \
   X(ArrayDataType::R4, 3, ArrayMemLoc::Both, Array3DR4)                 \
   X(ArrayDataType::R4, 3, ArrayMemLoc::Device, Array3DR4)               \
   X(ArrayDataType::R4, 3, ArrayMemLoc::Host, HostArray3DR4)

///
/// Usage in operator implementation file:
///   REGISTER_DIAG_OPERATOR(GlobalMinOp, "global_min");
///
/// This creates a static initializer that registers the operator
/// before main() runs.
#define REGISTER_DIAG_OPERATOR(Type, Name) \
  namespace { \
    static bool registered_##Type = []() { \
      ::OMEGA::AnalysisOpFactory::registerOperator(Name, \
        [](const std::string& n, const ::OMEGA::Config& c) { \
          return std::make_unique<Type>(n, c); \
        }); \
      return true; \
    }(); \
  }

#endif
