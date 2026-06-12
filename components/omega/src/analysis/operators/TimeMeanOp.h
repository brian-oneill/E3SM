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

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->getDataArray<ArrayT>();

      auto NDims = InputField->getNumDims();

      auto DimNames = InputField->getDimNames();

      
      auto OutputField = Field::create(
         OutputNames[0],
         "Time average of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<ScalarT>::max() / 10,// Min valid value
         std::numeric_limits<ScalarT>::max(), // Max valid value
         -std::numeric_limits<ScalarT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

      AccumArray = decltype(InputData)(OutputNames[0] + "_accumulator", InputData.layout());

      OutputData = 

   }

   void compute(const TimeInstant &TimeStamp) override {
      NSize = static_cast<I4>(AccumArray.size());
      parallelFor(
          {NSize}, KOKKOS_LAMBDA(const int FlatIdx) {
             AccumArray.data()[FlatIdx] += Input.data()[FlatIdx];
          });
      ++NAccum;
   }

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   ArrayT AccumArray;
   ArrayT OutputData;;
   I4 NAccum;
   I4 NSize;


};

} // end namespace OMEGA

#endif
