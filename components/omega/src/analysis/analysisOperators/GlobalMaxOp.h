#ifndef OMEGA_GLOBALMAXOP_H
#define OMEGA_GLOBALMAXOP_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

template<typename ArrayType>
class GlobalMaxOp : public AnalysisOperator {
 public:

   using TT = typename ArrayType::non_const_value_type;

   GlobalMaxOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "global_max";

      InstanceName = Name;
      InputNames = {InstanceName};

      std::string OutputFieldName = InstanceName + "_global_max";
      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};

      // Initialize tracking variables
      FieldComputed = false;
      LastComputed = TimeInstant();

   }

   ~GlobalMaxOp() override {
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
         "Global maximum of " + InputNames[0], // Description
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

      GlobalMax = globalMaxVal(InputData, Comm);

//      dispatchFieldArray(*InputField, ComputeGlobalMax{Comm, GlobalMax});

      deepCopy(OutputData, GlobalMax);

   }

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<TT>::type OutputData;

   TT GlobalMax;

   struct ComputeGlobalMax {
      MPI_Comm Comm;
      TT &GlobMax;
   
      template <typename ArrayT>
      void operator()(ArrayT InputData) const {
         using ValueT = typename ArrayT::non_const_value_type;
   
         if constexpr (!std::is_same_v<ValueT, TT>) {
            ABORT_ERROR("GlobalMaxOp: input field scalar type does not match "
                        "operator scalar type");
         } else {
            GlobMax = globalMaxVal(InputData, Comm);
         }
      }
};

};

} // namespace OMEGA

#endif
