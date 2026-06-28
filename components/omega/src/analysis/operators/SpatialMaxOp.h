#ifndef OMEGA_GLOBALMAXOP_H
#define OMEGA_GLOBALMAXOP_H

///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

///
template<typename ArrayT>
class SpatialMaxOp : public AnalysisOperator {
 public:

   using ScalarT = typename ArrayT::non_const_value_type;

   ///
   SpatialMaxOp(const std::vector<std::string> &UpstreamNames, Config Options) 
       : AnalysisOperator("SpatialMax") {

      InputNames = UpstreamNames;

      std::string OutputFieldName = InputNames[0] + "_SpatialMax";
      OutputNames = {OutputFieldName};
      InstanceName = OutputFieldName;

      OutputData = typename Array1D<ScalarT>::type(OutputNames[0], 1);

      I4 NDims = 1;
      std::vector<std::string> DimNames(NDims);
      DimNames[0] = "Scalar";
      auto ScalarDim = Dimension::create(DimNames[0], 1);

      auto OutputField = Field::create(
         OutputNames[0],
         "Spatial maximum of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<ScalarT>::max() / 10,// Min valid value
         std::numeric_limits<ScalarT>::max(), // Max valid value
         -std::numeric_limits<ScalarT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

      OutputField->template attachData<typename Array1D<ScalarT>::type>(OutputData);

   } // end constructor

   ///
   void compute(const TimeInstant &TimeStamp) override {

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->getDataArray<ArrayT>();

      std::vector<std::string> InputDimNames;

      InputField->getDimNames(InputDimNames);

      I4 NDims = InputDimNames.size();

      Array2DReal MaskArray;

      std::string IndexSpaceName = InputDimNames[std::max(0, NDims - 2)];

      if (IndexSpaceName == "NCells") {
         MaskArray = VCoord->CellMask;
      } else if (IndexSpaceName == "NEdges") {
         MaskArray = VCoord->EdgeMask;
      } else if (IndexSpaceName == "NVertices") {
         MaskArray = VCoord->VertexMask;
      } else {
         ABORT_ERROR("");
      }

      SpatialMax = globalMaskedMax(InputData, MaskArray, Comm);

      deepCopy(OutputData, SpatialMax);

      LastComputed = TimeStamp;
      FieldComputed = true;
   } // end compute

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<ScalarT>::type OutputData;

   ScalarT SpatialMax;

}; // end class SpatialMaxOp

} // namespace OMEGA

#endif
