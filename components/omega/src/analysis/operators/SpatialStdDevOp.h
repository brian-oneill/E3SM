#ifndef OMEGA_STDDEV_H
#define OMEGA_STDDEV_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

///
template<typename ArrayT>
class SpatialStdDevOp : public AnalysisOperator {
 public:

   using ScalarT = typename ArrayT::non_const_value_type;

   ///
   SpatialStdDevOp(const std::vector<std::string> &UpstreamNames, Config Options)
       : AnalysisOperator("SpatialStdDev") {

      InputNames = {UpstreamNames[0], UpstreamNames[0] + "_SpatialMean"};

      std::string OutputFieldName = InputNames[0] + "_SpatialStdDev";
      OutputNames = {OutputFieldName};
      InstanceName = OutputFieldName;

      OutputData = typename Array1D<ScalarT>::type(OutputNames[0], 1);

      I4 NDims = 1;
      std::vector<std::string> DimNames(NDims);
      DimNames[0] = "Scalar";
      auto ScalarDim = Dimension::create(DimNames[0], 1);

      auto OutputField = Field::create(
         OutputNames[0],
         "Standard deviation of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         0,                      // Min valid value
         std::numeric_limits<ScalarT>::max(), // Max valid value
         -std::numeric_limits<ScalarT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

      OutputField->template attachData<typename Array1D<ScalarT>::type>(OutputData);

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->template getDataArray<ArrayT>();

      WorkArray = decltype(InputData)(OutputNames[0] + "_work_array", InputData.layout());

   } // end constructor

   ///
   void compute(const TimeInstant &TimeStamp) override {

      OMEGA_SCOPE(LocWorkArray, WorkArray);

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->template getDataArray<ArrayT>();

      std::vector<std::string> InputDimNames;

      InputField->getDimNames(InputDimNames);

      I4 NDims = InputDimNames.size();

      Array2DReal MaskArray;

      std::string IndexSpaceName = InputDimNames[std::max(0, NDims - 2)];

      I4 NOwned = 0;
      I4 NVertLayers = VCoord->NVertLayers;

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
      // For MaskArray: always 2D (Horiz, Vert)
      std::vector<I4> indxRange;

      if (NDims == 1) {
         // 1D array: just horizontal dimension
         indxRange = {0, NOwned - 1};
      } else if (NDims == 2) {
         // 2D array: (Horiz, Vert)
         indxRange = {0, NOwned - 1, 0, NVertLayers - 1};
      } else {
         // 3D+ array: (Extra dims..., Horiz, Vert)
         // Need to include all indices for extra dimensions, then restrict horiz
         indxRange.resize(2 * NDims);
         for (I4 i = 0; i < NDims - 2; ++i) {
            indxRange[2*i]     = 0;
            indxRange[2*i + 1] = InputData.extent(i) - 1;
         }
         // Horizontal dimension (second to last)
         indxRange[2*(NDims-2)]     = 0;
         indxRange[2*(NDims-2) + 1] = NOwned - 1;
         // Vertical dimension (last)
         indxRange[2*(NDims-1)]     = 0;
         indxRange[2*(NDims-1) + 1] = NVertLayers - 1;
      }

      // IndxRange for mask (always 2D)
      std::vector<I4> maskIndxRange = {0, NOwned - 1, 0, NVertLayers - 1};

      auto MeanField = Field::get(InputNames[1]);
      auto MeanVal = MeanField->template getDataArray<typename Array1D<ScalarT>::type>();

      // Fill WorkArray with squared differences (no mask — globalMaskedSum applies it)
      I4 NSize = static_cast<I4>(InputData.size());
      const int Arr1Rank = InputData.rank;
      parallelFor(
          {NSize}, KOKKOS_LAMBDA(const int flat_idx) {
             int horizIdx = 0;
             int vertIdx = 0;

             if (Arr1Rank == 1) {
                horizIdx = flat_idx;
             } else if (Arr1Rank == 2) {
                horizIdx = flat_idx / InputData.extent(1);
                vertIdx  = flat_idx % InputData.extent(1);
             } else {
                int idx_last_two = flat_idx % (InputData.extent(Arr1Rank - 2) *
                                               InputData.extent(Arr1Rank - 1));
                horizIdx = idx_last_two / InputData.extent(Arr1Rank - 1);
                vertIdx  = idx_last_two % InputData.extent(Arr1Rank - 1);
             }

             auto Diff = InputData.data()[flat_idx] - MeanVal(0);
             LocWorkArray.data()[flat_idx] = Diff * Diff;

          });

      ScalarT WorkSum;
      ScalarT MaskSum;
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
         WorkSum = globalMaskedSum(WorkArray, Mask1D, Comm, &indxRange);
         MaskSum = globalSum(Mask1D, Comm, &indxRange);
      } else {
         WorkSum = globalMaskedSum(WorkArray, MaskArray, Comm, &indxRange);
         MaskSum = globalSum(MaskArray, Comm, &maskIndxRange);
      }

      auto Variance = WorkSum / MaskSum;
      auto StdDev = std::sqrt(Variance);

      deepCopy(OutputData, StdDev);

      LastComputed = TimeStamp;
      FieldComputed = true;
   } // end compute

 private:

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<ScalarT>::type OutputData;

   ArrayT WorkArray;

   ScalarT StdDev;

   /// Contiguous 1D mask array (k=0 column of the 2D mask) used for 1D inputs.
   /// Allocated lazily on first compute to avoid LayoutStride subviews that are
   /// incompatible with the reduction functions.
   typename Array1D<Real>::type Mask1D;

}; // end class SpatialStdDevOp

} // end namespace OMEGA

#endif
