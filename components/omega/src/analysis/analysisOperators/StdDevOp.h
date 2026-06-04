#ifndef OMEGA_STDDEV_H
#define OMEGA_STDDEV_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

template<typename ArrayType>
class StdDevOp : public AnalysisOperator {
 public:

   using scalar_type = typename ArrayType::non_const_value_type;

   StdDevOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "standard_dev";

      InputNames = {"PseudoThickness", "PseudoThickness_global_mean"};

      std::string OutputFieldName = InstanceName + "_stddev";
      OutputNames = {OutputFieldName};

      // Initialize tracking variables
      FieldComputed = false;
      LastComputed = TimeInstant();

   }

   ~StdDevOp() override = default;

   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

      OutputData = typename Array1D<scalar_type>::type(InstanceName + "_out", 1);

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
         std::numeric_limits<scalar_type>::max(), // Max valid value
         -std::numeric_limits<scalar_type>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

   }

   void compute(const TimeInstant &TimeStamp) override {

      auto InputField = Field::get(InputNames[0]);

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

      dispatchFieldArray(*InputField, ComputeStdDev{Comm, MaskArray, StdDev});

      deepCopy(OutputData, StdDev);

   }

   scalar_type getVal() {return StdDev;}

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<scalar_type>::type OutputData;

   scalar_type StdDev;

   struct ComputeStdDev {
      MPI_Comm Comm;
      Array2DReal MaskArray;
      scalar_type &LocStdDev;
   
      template <typename ArrayT>
      void operator()(ArrayT InputData) const {
         using ValueT = typename ArrayT::non_const_value_type;
   
         if constexpr (!std::is_same_v<ValueT, scalar_type>) {
            ABORT_ERROR("StdDevOp: input field scalar type does not match "
                        "operator scalar type");
         }



         auto ValSum = globalWeightedSum(InputData, MaskArray, Comm);
         auto MaskSum = globalSum(MaskArray, Comm);

         LocStdDev = ValSum/MaskSum;
      }
   };

};

} // end namespace OMEGA

#endif
