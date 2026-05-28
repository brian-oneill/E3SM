#ifndef OMEGA_GLOBALMAXOP_H
#define OMEGA_GLOBALMAXOP_H


//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Field.h"
#include "Reductions.h"

namespace OMEGA {

template<typename TT>
class GlobalMaxOp : public AnalysisOperator {
 public:

   GlobalMaxOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "global_max";

      InputNames = {"PseudoThickness"};

      std::string OutputFieldName = InstanceName + "_global_max";
      OutputNames = {OutputFieldName};

      // Initialize tracking variables
      FieldComputed = false;
      LastComputed = TimeInstant();

   }

   ~GlobalMaxOp() override = default;


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
         "Global maximum of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<TT>::max() + 1.,// Min valid value
         std::numeric_limits<TT>::max(), // Max valid value
         -std::numeric_limits<TT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

   }

   void compute(const TimeInstant &TimeStamp) override {

      auto InputField = Field::get(InputNames[0]);
      const I4 NDims = InputField->getNumDims();

      auto InputData = Field::getFieldDataArray<typename Array_t<2,TT>::type>(InputNames[0]);

      GlobalMax = globalMaxVal(InputData, Comm);

      deepCopy(OutputData, GlobalMax);

   }

   TT getVal() {return GlobalMax;}

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type matching
   /// input
   // Anlys1DVariant OutputData;
   typename Array1D<TT>::type OutputData;

   TT GlobalMax;

};

} // namespace OMEGA

#endif

