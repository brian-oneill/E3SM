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
   TimeMeanOp(const std::vector<std::string> &UpstreamNames, Config Options)
       : AnalysisOperator("TimeMean") {

      InputNames = UpstreamNames;

      std::string AvgPeriod;
      Options.get("Period", AvgPeriod);
      std::string OutputFieldName = InputNames[0] + "_TimeMean" + AvgPeriod;
//      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};
      InstanceName = OutputFieldName;

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->template getDataArray<ArrayT>();

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

      OutputData = decltype(InputData)(OutputNames[0] + "_out", InputData.layout());

      OutputField->template attachData<ArrayT>(OutputData);

      NumAccum = 0;
      PeriodAlarm = nullptr;
      IsNewPeriod = true;

   } // end constructor

   ///
   void setPeriodAlarm(Alarm *Alarm) override {
      PeriodAlarm = Alarm;
   }

   ///
   void compute(const TimeInstant &TimeStamp) override {
      OMEGA_SCOPE(LocOutputData, OutputData);

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->template getDataArray<ArrayT>();

      // Accumulate: add current array state
      if (IsNewPeriod) {
         NumAccum = 1;
         deepCopy(OutputData, InputData);
         IsNewPeriod = false;
      } else {
         parallelFor(
             {ArraySize}, KOKKOS_LAMBDA(const int FlatIdx) {
                LocOutputData.data()[FlatIdx] += InputData.data()[FlatIdx];
             });
         ++NumAccum;
         // Check if we should finalize
         bool ShouldFinalize =
             (PeriodAlarm != nullptr && PeriodAlarm->isRinging());
         if (ShouldFinalize) {
            // Compute mean to finalize output
            Real InvNumAccum = 1.0 / static_cast<Real>(NumAccum);
            parallelFor(
                {ArraySize}, KOKKOS_LAMBDA(const int FlatIdx) {
                   LocOutputData.data()[FlatIdx] *= InvNumAccum;
                });
            IsNewPeriod = true; // next compute starts fresh
         }
      }
      LastComputed = TimeStamp;
      FieldComputed = true;
   } // end compute

 private:

   ArrayT OutputData;
   I4 NumAccum;
   I4 ArraySize;

   Alarm *PeriodAlarm;
   bool IsNewPeriod;

}; // end class TimeMeanOp

} // end namespace OMEGA

#endif
