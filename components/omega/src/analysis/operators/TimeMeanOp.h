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

      CompPhase = TemporalPhase::Accumulate;
      Finalized = false;
   }

//   ~TimeMeanOp() override {
//      if (Field::exists(OutputNames[0]))
//         Field::destroy(OutputNames[0]);
//   }


   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

      auto InputField = Field::get(InputNames[0]);

      InputData = InputField->getDataArray<ArrayT>();

      auto NDims = InputField->getNumDims();

      std::vector<std::string> DimNames;
      InputField->getDimNames(DimNames);

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

      ArraySize = static_cast<I4>(InputData.size());

      AccumArray = decltype(InputData)(OutputNames[0] + "_accumulator", InputData.layout());

      OutputData = decltype(InputData)(OutputNames[0] + "_out", InputData.layout());

      OutputField->template attachData<ArrayT>(OutputData);

      CompPhase = TemporalPhase::Accumulate;

   }

   void compute(const TimeInstant &TimeStamp) override {
      if (CompPhase == TemporalPhase::Accumulate) {
      parallelFor(
          {ArraySize}, KOKKOS_LAMBDA(const int FlatIdx) {
             AccumArray.data()[FlatIdx] += InputData.data()[FlatIdx];
          });
      ++NumSamples;
      }
   }

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   TemporalPhase CompPhase;
   bool Finalized;
   
   ArrayT InputData;
   ArrayT AccumArray;
   ArrayT OutputData;
   I4 NumSamples;
   I4 ArraySize;

   Alarm PeriodAlarm;
   Alarm IntervalAlarm;

};

} // end namespace OMEGA

#endif
