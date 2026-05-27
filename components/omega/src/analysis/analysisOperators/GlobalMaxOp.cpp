//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "analysisOperators/GlobalMaxOp.h"

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
   
   // Allocate output array (single value per rank, but we'll only use rank 0)
   OutputData = Array1DReal("GlobalMax_" + InstanceName, 1);
//   Kokkos::deep_copy(OutputData, -std::numeric_limits<Real>::max());
//
//   // Create and register output field in Field registry
//   // Global max is a scalar, so dimensions are (1)
//   std::vector<std::string> DimNames = {"scalar"};
//   std::vector<I4> DimLengths = {1};
//   
//   Err = Field::create(
//       OutputFieldName,           // Field name
//       "Global maximum of " + InputNames[0], // Description
//       "same as input",          // Units (inherited from input)
//       "same as input",          // Standard name
//       0.0,                      // Min valid value
//       std::numeric_limits<Real>::max(), // Max valid value
//       -std::numeric_limits<Real>::max(), // Fill value
//       DimNames,                 // Dimension names
//       DimLengths,               // Dimension lengths
//       OutputData                // Data array
//   );
//   
//   if (Err != 0) {
//      LOG_ERROR("GlobalMaxOp::initialize: Failed to create output field '{}'",
//                OutputFieldName);
//      return;
//   }
//   
//   LOG_INFO("GlobalMaxOp: Initialized operator '{}' for input '{}'",
//            InstanceName, InputNames[0]);
}




} // end namespace OMEGA

//===----------------------------------------------------------------------===//
