#ifndef OMEGA_GLOBALSUMOP_H
#define OMEGA_GLOBALSUMOP_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

template<typename TT>
class GlobalMeanOp : public AnalysisOperator {
 public:

   GlobalMeanOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "global_sum";

      InputNames = {"PseudoThickness"};

      std::string OutputFieldName = InstanceName + "_global_sum";
      OutputNames = {OutputFieldName};

      // Initialize tracking variables
      FieldComputed = false;
      LastComputed = TimeInstant();

   }

   ~GlobalMeanOp() override = default;

   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

      OutputData = typename Array1D<TT>::type(InstanceName + "_out", 1);

      I4 NDims = 1;
      std::vector<std::string> DimNames(NDims);
      DimNames[0] = "Scalar";
      auto ScalarDim = Dimension::create(DimNames[0], 1);

      auto OutputField = Field::create(
         OutputNames[0],
         "Global sum of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<TT>::max() / 10,// Min valid value
         std::numeric_limits<TT>::max(), // Max valid value
         -std::numeric_limits<TT>::max(), // Fill value
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

      dispatchFieldArray(*InputField, ComputeGlobalMean{Comm, MaskArray, GlobalMean});

      deepCopy(OutputData, GlobalMean);

   }

   TT getVal() {return GlobalMean;}

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<TT>::type OutputData;

   TT GlobalMean;

   struct ComputeGlobalMean {
      MPI_Comm Comm;
      Array2DReal MaskArray;
      TT &GlobMean;
   
      template <typename ArrayT>
      void operator()(ArrayT InputData) const {
         using ValueT = typename ArrayT::non_const_value_type;
   
         if constexpr (!std::is_same_v<ValueT, TT>) {
            ABORT_ERROR("GlobalMinOp: input field scalar type does not match "
                        "operator scalar type");
         }
         TT ValSum = globalSum(InputData, Comm);
//         auto MaskSum = globalSum(MaskArray, Comm);

         GlobMean = ValSum;
      }
   };

};

} // end namespace OMEGA

#endif
