#ifndef OMEGA_TIMEMEANOP_H
#define OMEGA_TIMEMEANOP_H

///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"

namespace OMEGA {

///
template<typename ArrayT>
class TimeMeanOp : public AnalysisOperator {
 public:

   using ScalarT = typename ArrayT::non_const_value_type;

   ///
   TimeMeanOp(const std::vector<std::string> &UpstreamNames, Config Options) {

      // Set operator type
      OperatorTypeName = "time_mean";

      InputNames = UpstreamNames;

      std::string AvgPeriod;
      Options.get("Period", AvgPeriod);
      std::string OutputFieldName = InputNames[0] + "_time_mean";
//      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};
      InstanceName = OutputFieldName;


      CompPhase = TemporalPhase::Accumulate;
      Finalized = false;

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

   } // end constructor

   ///
   void initialize(Config Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

   } // end initialize

   ///
   void compute(const TimeInstant &TimeStamp) override {
      if (CompPhase == TemporalPhase::Accumulate) {
         parallelFor(
             {ArraySize}, KOKKOS_LAMBDA(const int FlatIdx) {
                AccumArray.data()[FlatIdx] += InputData.data()[FlatIdx];
             });
         ++NumSamples;
      }

      LastComputed = TimeStamp;
      FieldComputed = true;
   } // end compute

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

}; // end class TimeMeanOp

} // end namespace OMEGA

#endif
