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
AnalysisOpFactory::createOp(const std::string &OpType,
                            const std::string &InputName,
                            const Config &Options) {


   // Get FieldPtr and extract metadata
   auto FieldPtr = Field::get(InputName);
   if (!FieldPtr) {
       ABORT_ERROR("Field '{}' not found for operator creation", InputName);
   }
   
   ArrayDataType DType = FieldPtr->getType();
   int Rank = FieldPtr->getNumDims();
   ArrayMemLoc MemLoc = FieldPtr->getMemoryLocation();
   
   
   // Map metadata to array type name
   std::string arrayTypeName = getArrayTypeName(DType, Rank, MemLoc);
   
   // Build fully-qualified operator type
   std::string FullOpType = OpType + "_" + arrayTypeName;

   std::cout << "full op type name: " << FullOpType << std::endl;

   auto &Reg = registry();

   auto it = Reg.find(FullOpType);
   
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
      
      ABORT_ERROR("Operator type {} not found", FullOpType);
   }
   
   // Call the registered creator function
   return it->second(InputName, Options);
}

std::unique_ptr<AnalysisOperator> 
AnalysisOpFactory::createOp(const std::string &OpType,
         const std::string &InputName,
         const std::string &Name,
         const Config &Options) {
    
    // Get FieldPtr and extract metadata
    auto FieldPtr = Field::get(InputName);
    if (!FieldPtr) {
        ABORT_ERROR("Field '{}' not found for operator creation", InputName);
    }
    
    ArrayDataType DType = FieldPtr->getType();
    int Rank = FieldPtr->getNumDims();
    ArrayMemLoc MemLoc = FieldPtr->getMemoryLocation();
    
    // Map metadata to array type name
    std::string arrayTypeName = getArrayTypeName(DType, Rank, MemLoc);
    
    // Build fully-qualified operator type
    std::string FullOpType = OpType + "_" + arrayTypeName;

   std::cout << "full op type name: " << FullOpType << std::endl;

   auto &Reg = registry();

   auto it = Reg.find(FullOpType);
   
   if (it == Reg.end()) {
    
      ABORT_ERROR("Operator not found");
   }
   
   // Call the registered creator function
   return it->second(Name, Options);
}


bool AnalysisOpFactory::hasOperator(const std::string &Type) {
   auto &Reg = registry();
   return Reg.find(Type) != Reg.end();
}


std::string AnalysisOpFactory::getArrayTypeName(ArrayDataType DType, int Rank, ArrayMemLoc MemLoc) {
   // Use similar logic to dispatchFieldArray but return the type name string
   #define TRY_ARRAY_TYPE(dt, r, ml, ArrayT) \
       if (DType == dt && Rank == r && MemLoc == ml) { \
           return std::string(#ArrayT) + "_" + #ml; \
       }
   
   OMEGA_ANALYSIS_ARRAY_TYPES(TRY_ARRAY_TYPE)
   
   #undef TRY_ARRAY_TYPE
   ABORT_ERROR("Unsupported array type/Rank/location: DType={}, Rank={}, MemLoc={}",
                static_cast<int>(DType), Rank, static_cast<int>(MemLoc));

   return {};
}

}
