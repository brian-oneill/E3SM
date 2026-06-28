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

      auto InputData = InputField->template getDataArray<ArrayT>();

      std::vector<std::string> InputDimNames;

      InputField->getDimNames(InputDimNames);

      I4 NDims = InputDimNames.size();

      Array2DReal MaskArray;

      std::string IndexSpaceName = InputDimNames[std::max(0, NDims - 2)];

      I4 NOwned = 0;
      I4 NVertLayers = 0;
      
      NVertLayers = VCoord->NVertLayers;

      if (IndexSpaceName == "NCells") {
         MaskArray = VCoord->CellMask;
         NOwned = Mesh->NCellsOwned;
      } else if (IndexSpaceName == "NEdges") {
         MaskArray = VCoord->EdgeMask;
         NOwned = Mesh->NEdgesOwned;
      } else if (IndexSpaceName == "NVertices") {
         MaskArray = VCoord->VertexMask;
         NOwned = Mesh->NVerticesOwned;
      } else {
         ABORT_ERROR("");
      }

      // Create IndxRange to exclude halo cells
      // For InputData: depends on rank (could be 1D, 2D, 3D+)
      std::vector<I4> indxRange;

      if (NDims == 1) {
         // 1D array: just horizontal dimension
         indxRange = {0, NOwned - 1};
      } else if (NDims == 2) {
         // 2D array: (Horiz, Vert)
         indxRange = {0, NOwned - 1, 0, NVertLayers - 1};
      } else {
         // 3D+ array: (Extra dims..., Horiz, Vert)
         indxRange.resize(2 * NDims);
         for (I4 i = 0; i < NDims - 2; ++i) {
            indxRange[2*i] = 0;
            indxRange[2*i + 1] = InputData.extent(i) - 1;
         }
         // Horizontal dimension (second to last)
         indxRange[2*(NDims-2)] = 0;
         indxRange[2*(NDims-2) + 1] = NOwned - 1;
         // Vertical dimension (last)
         indxRange[2*(NDims-1)] = 0;
         indxRange[2*(NDims-1) + 1] = NVertLayers - 1;
      }

      if (NDims == 1) {
         // For 1D arrays (horizontal only), use the k=0 column of the 2D mask.
         // k=0 represents whether the column is active at all, which is the
         // correct mask to use when there is no vertical dimension.
         // We copy into a contiguous Array1D member to avoid LayoutStride views
         // that are incompatible with the reduction functions.
         if (Mask1D.size() == 0)
            Mask1D = typename Array1D<Real>::type("Mask1D", MaskArray.extent(0));
         auto LocalMaskArray = MaskArray;
         auto LocalMask1D    = Mask1D;
         parallelFor(
             {static_cast<I4>(MaskArray.extent(0))},
             KOKKOS_LAMBDA(int I) { LocalMask1D(I) = LocalMaskArray(I, 0); });
         SpatialMax = globalMaskedMax(InputData, Mask1D, Comm, &indxRange);
      } else {
         SpatialMax = globalMaskedMax(InputData, MaskArray, Comm, &indxRange);
      }

      deepCopy(OutputData, SpatialMax);

      LastComputed = TimeStamp;
      FieldComputed = true;
   } // end compute

 private:

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<ScalarT>::type OutputData;

   ScalarT SpatialMax;

   /// Contiguous 1D mask array (k=0 column of the 2D mask) used for 1D inputs.
   /// Allocated lazily on first compute to avoid LayoutStride subviews that are
   /// incompatible with the reduction functions.
   typename Array1D<Real>::type Mask1D;

}; // end class SpatialMaxOp

} // namespace OMEGA

#endif
