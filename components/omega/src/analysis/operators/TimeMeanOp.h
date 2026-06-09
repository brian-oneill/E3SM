#ifndef OMEGA_TIMEMEANOP_H
#define OMEGA_TIMEMEANOP_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"

namespace OMEGA {

template<typename ArrayT>
class TimeMeanOp : public AnalysisOperator {
 public:

   using ScalarT = typename ArrayT::non_const_value_type;

   TimeMeanOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "time_mean";

      InstanceName = Name;
      InputNames = {Name};

      std::string OutputFieldName = InstanceName + "_time_mean";
      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};


   }

   ~TimeMeanOp() override {
   }


   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {
   }

   void compute(const TimeInstant &TimeStamp) override {
   }

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   ArrayT Accumulator;


};

} // end namespace OMEGA

#endif
