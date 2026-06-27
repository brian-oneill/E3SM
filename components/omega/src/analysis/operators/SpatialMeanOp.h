#ifndef OMEGA_GLOBALMEANOP_H
#define OMEGA_GLOBALMEANOP_H

///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

///
template<typename ArrayT>
class SpatialMeanOp : public AnalysisOperator {
 public:

   using ScalarT = typename ArrayT::non_const_value_type;

   ///
   SpatialMeanOp(const std::vector<std::string> &UpstreamNames, Config Options)
       : AnalysisOperator("SpatialMean") {


      InputNames = UpstreamNames;

      std::string OutputFieldName = InputNames[0] + "_SpatialMean";
//      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};
      InstanceName = OutputFieldName;

      OutputData = typename Array1D<ScalarT>::type(OutputNames[0], 1);

      I4 NDims = 1;
      std::vector<std::string> DimNames(NDims);
      DimNames[0] = "Scalar";
      auto ScalarDim = Dimension::create(DimNames[0], 1);

      auto OutputField = Field::create(
         OutputNames[0],
         "Spatial mean of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<ScalarT>::max() / 10,// Min valid value
         std::numeric_limits<ScalarT>::max(), // Max valid value
         -std::numeric_limits<ScalarT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

      OutputField->template attachData<typename Array1D<ScalarT>::type>(OutputData);

   }  // end constructor

   ///
   void compute(const TimeInstant &TimeStamp) override {

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->getDataArray<ArrayT>();

      std::vector<std::string> InputDimNames;

      InputField->getDimNames(InputDimNames);

      I4 NDims = InputDimNames.size();

      Array2DReal MaskArray;

      std::string IndexSpaceName = InputDimNames[std::max(0, NDims - 2)];

      I4 NCellsOwned = 0;
      I4 NVertLayers = 0;
      
      if (IndexSpaceName == "NCells") {
         MaskArray = VCoord->CellMask;
         NCellsOwned = Mesh->NCellsOwned;
         NVertLayers = VCoord->NVertLevels;
      } else if (IndexSpaceName == "NEdges") {
         MaskArray = VCoord->EdgeMask;
         NCellsOwned = Mesh->NEdgesOwned;
         NVertLayers = VCoord->NVertLevels;
      } else if (IndexSpaceName == "NVertices") {
         MaskArray = VCoord->VertexMask;
         NCellsOwned = Mesh->NVerticesOwned;
         NVertLayers = VCoord->NVertLevels;
      } else {
         ABORT_ERROR("");
      }

      // Create IndxRange to exclude halo cells
      // For InputData: depends on rank (could be 1D, 2D, 3D+)
      // For MaskArray: always 2D (Horiz, Vert)
      std::vector<I4> indxRange;
      
      if (NDims == 1) {
         // 1D array: just horizontal dimension
         indxRange = {0, NCellsOwned - 1};
      } else if (NDims == 2) {
         // 2D array: (Horiz, Vert)
         indxRange = {0, NCellsOwned - 1, 0, NVertLayers - 1};
      } else {
         // 3D+ array: (Extra dims..., Horiz, Vert)
         // Need to include all indices for extra dimensions, then restrict horiz
         indxRange.resize(2 * NDims);
         for (I4 i = 0; i < NDims - 2; ++i) {
            indxRange[2*i] = 0;
            indxRange[2*i + 1] = InputData.extent(i) - 1;
         }
         // Horizontal dimension (second to last)
         indxRange[2*(NDims-2)] = 0;
         indxRange[2*(NDims-2) + 1] = NCellsOwned - 1;
         // Vertical dimension (last)
         indxRange[2*(NDims-1)] = 0;
         indxRange[2*(NDims-1) + 1] = NVertLayers - 1;
      }
      
      // IndxRange for mask (always 2D)
      std::vector<I4> maskIndxRange = {0, NCellsOwned - 1, 0, NVertLayers - 1};

      auto ValSum = globalWeightedSum(InputData, MaskArray, Comm, &indxRange);
      auto MaskSum = globalSum(MaskArray, Comm, &maskIndxRange);

      SpatialMean = ValSum/MaskSum;

      deepCopy(OutputData, SpatialMean);

      LastComputed = TimeStamp;
      FieldComputed = true;
   } // end compute

   ScalarT getVal() {return SpatialMean;}

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<ScalarT>::type OutputData;

   ScalarT SpatialMean;

}; // end class SpatialMeanOp

} // end namespace OMEGA

#endif
