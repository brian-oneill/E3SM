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
   SpatialStdDevOp(const std::string &UpstreamName, const Config &Options)
       : AnalysisOperator("spatial_stddev") {

      InputNames = {UpstreamName, UpstreamName + "_spatial_mean"};

      std::string OutputFieldName = InputNames[0] + "_spatial_stddev";
//      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};
      InstanceName = OutputFieldName;

   } // end constructot

   ///
   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

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

      auto InputData = InputField->getDataArray<ArrayT>();

      WorkArray = decltype(InputData)(OutputNames[0] + "_work_array", InputData.layout()); 

   } // end initialize

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

      const int Arr1Rank = InputData.rank;
      const int Arr2Rank = MaskArray.rank;

      auto MeanField = Field::get(InputNames[1]);
      auto MeanVal = MeanField->getDataArray<typename Array1D<ScalarT>::type>();

      std::cout << "meanval: " << MeanVal(0) << std::endl;

      I4 NSize = static_cast<I4>(InputData.size());
      parallelFor(
          {NSize}, KOKKOS_LAMBDA(const int flat_idx) {
             int remaining = flat_idx;
             int horizIdx = 0;
             int vertIdx = 0;

             if (Arr1Rank == 1) {
                horizIdx = flat_idx;
             } else if (Arr1Rank == 2) {
                horizIdx = flat_idx / InputData.extent(1);
                vertIdx = flat_idx % InputData.extent(1);
             } else {
                int idx_last_two = flat_idx % (InputData.extent(Arr1Rank - 2) *
                                                InputData.extent(Arr1Rank - 1));
                horizIdx = idx_last_two / InputData.extent(Arr1Rank - 1);
                vertIdx = idx_last_two % InputData.extent(Arr1Rank - 1);
             }
   
             int arr2_idx = 0;
             if (Arr2Rank == 1) {
                arr2_idx = horizIdx;
             } else {
                arr2_idx = horizIdx * MaskArray.extent(1) + vertIdx;
             }

             auto Diff = MaskArray.data()[arr2_idx] * (InputData.data()[flat_idx] - MeanVal(0));
             WorkArray.data()[flat_idx] = Diff * Diff;

          });

      auto WorkSum = globalSum(WorkArray, Comm);
      auto MaskSum = globalSum(MaskArray, Comm);

      auto Variance = WorkSum / MaskSum;
      auto StdDev = std::sqrt(Variance);
         


//      dispatchFieldArray(*InputField, ComputeStdDev{Comm, MaskArray, StdDev});

      deepCopy(OutputData, StdDev);

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

   ArrayT WorkArray;

   ScalarT StdDev;

}; // end class SpatialStdDevOp

} // end namespace OMEGA

#endif
