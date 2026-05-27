//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "analysisOperators/GlobalMaxOp.h"
#include <limits>

namespace OMEGA {

GlobalMaxOp::GlobalMaxOp(const std::string &Name, const Config &Options)
    : Mesh(nullptr), VCoord(nullptr) {
   
   // Set operator type
   OperatorTypeName = "global_max";
   
   // Set instance name
   InstanceName = Name;
   
//   // Parse input field name from configuration
//   // Expected format: "global_max(field_name)" 
//   // For this example, we'll assume the input name is provided directly
//   std::string InputFieldName;
//   int Err = Options.get("InputField", InputFieldName);
//   if (Err != 0) {
//      LOG_ERROR("GlobalMaxOp: InputField not specified in configuration");
//      return;
//   }
//   InputNames = {InputFieldName};
   InputNames = {"PseudoThickness"};
   
   // Construct output field name
   OutputFieldName = InstanceName + "_global_max";
   OutputNames = {OutputFieldName};
   
   // Initialize compute tracking
   FieldComputed = false;
   LastComputed = TimeInstant(); // Invalid time
}

void GlobalMaxOp::initialize(const Config *Options,
                             const HorzMesh *MeshIn,
                             const VertCoord *VCoordIn) {
   
   int Err = 0;
   
   // Store mesh and decomposition
   Mesh = MeshIn;
   VCoord = VCoordIn;
   
   // Validate that input field exists
   auto *InputField = Field::getField(InputNames[0]);
   if (InputField == nullptr) {
      ABORT_ERROR("GlobalMaxOp::initialize: Input field '{}' not found",
                InputNames[0]);
   }
   
   ArrayDataType inputType = InputField->getType();

   // Allocate output array (single value per rank, but we'll only use rank 0)
   OutputData = Array1DReal("GlobalMax_" + InstanceName, 1);

   switch(inputType) {
       case ArrayDataType::R4:
           OutputData = Array1DR4(InstanceName + "_out_array", 1);
           break;
       case ArrayDataType::R8:
           OutputData = Array1DR8(InstanceName + "_out_array", 1);
           break;
       case ArrayDataType::I4:
           OutputData = Array1DI4(InstanceName + "_out_array", 1);
           break;
       case ArrayDataType::I8:
           OutputData = Array1DI8(InstanceName + "_out_array", 1);
           break;
       default:
           ABORT_ERROR("GlobalMaxOp::initialize: Unknown or unsupported array type");
   }

   std::vector<std::string> DimNames(1);
   DimNames[0] = "Scalar"
   auto ScalarDim = Dimension::create(DimNames[0], 1);

(

   auto OutputField = Field::create(
       OutputFieldName,           // Field name
       "Global maximum of " + InputNames[0], // Description
       "",                     // Units (inherited from input)
       "",                     // Standard name
       0,                      // Min valid value
       std::numeric_limits<Real>::max(), // Max valid value
       -std::numeric_limits<Real>::max(), // Fill value
       DimNames,                 // Dimension names
       DimLengths                // Dimension lengths
   );
   
   if (Err != 0) {
      LOG_ERROR("GlobalMaxOp::initialize: Failed to create output field '{}'",
                OutputFieldName);
      return;
   }
}




} // end namespace OMEGA

//===----------------------------------------------------------------------===//
