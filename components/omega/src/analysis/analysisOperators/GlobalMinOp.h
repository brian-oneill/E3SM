#ifndef OMEGA_GLOBALMINOP_H
#define OMEGA_GLOBALMINOP_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

template<typename ArrayType>
class GlobalMinOp : public AnalysisOperator {
 public:

   using TT = typename ArrayType::non_const_value_type;

   GlobalMinOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "global_min";

      InstanceName = Name;
      InputNames = {InstanceName};

      std::string OutputFieldName = InstanceName + "_global_min";
      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};

      // Initialize tracking variables
      FieldComputed = false;
      LastComputed = TimeInstant();

   }

   ~GlobalMinOp() override {
      if (Field::exists(OutputNames[0]))
         Field::destroy(OutputNames[0]);
   }

   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

      OutputData = typename Array1D<TT>::type(OutputNames[0], 1);

      I4 NDims = 1;
      std::vector<std::string> DimNames(NDims);
      DimNames[0] = "Scalar";
      auto ScalarDim = Dimension::create(DimNames[0], 1);

      auto OutputField = Field::create(
         OutputNames[0],
         "Global minimum of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<TT>::max() / 10,// Min valid value
         std::numeric_limits<TT>::max(), // Max valid value
         -std::numeric_limits<TT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

      OutputField->template attachData<typename Array1D<TT>::type>(OutputData);

   }

   void compute(const TimeInstant &TimeStamp) override {

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->getDataArray<ArrayType>();

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

//      dispatchFieldArray(*InputField, ComputeGlobalMin{Comm, MaskArray, GlobalMin});
      GlobalMin = globalWeightedMin(InputData, MaskArray, Comm);

      deepCopy(OutputData, GlobalMin);

   }

//   TT getVal() {return GlobalMin;}

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<TT>::type OutputData;

   TT GlobalMin;

   struct ComputeGlobalMin {
      MPI_Comm Comm;
      Array2DReal MaskArray;
      TT &GlobMin;
   
      template <typename ArrayT>
      void operator()(ArrayT InputData) const {
         using ValueT = typename ArrayT::non_const_value_type;
   
         if constexpr (!std::is_same_v<ValueT, TT>) {
            ABORT_ERROR("GlobalMinOp: input field scalar type does not match "
                        "operator scalar type");
         } else {
            GlobMin = globalWeightedMin(InputData, MaskArray, Comm);
         }
      }
   };

};

} // namespace OMEGA
#endif
