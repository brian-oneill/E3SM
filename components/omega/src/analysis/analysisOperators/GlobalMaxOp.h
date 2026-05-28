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




   }

   void compute(const TimeInstant &TimeStamp) override {

   }

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type matching
   /// input
   // Anlys1DVariant OutputData;
   Array1D<TT> OutputData;


};

} // namespace OMEGA

#endif

