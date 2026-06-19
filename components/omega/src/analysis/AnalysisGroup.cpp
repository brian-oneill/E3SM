//===----------------------------------------------------------------------===//
#include "AnalysisGroup.h"

namespace OMEGA {

//------------------------------------------------------------------------------
std::string AnalysisGroup::getName() {
   return GroupName;
}

//------------------------------------------------------------------------------
void AnalysisGroup::createAnalysisGroupStreams(
    const std::string &GroupName,
    Config &AnalysisGroupOptions,
    Analysis *AnalysisPtr) {

   Error Err1;
   Error Err2;

   // Check for optional stream config parameters in AnalysisGroup config
   Config AnalysisStreamOptions("Stream");
   Err1 = AnalysisGroupOptions.get(AnalysisStreamOptions);
   std::map<std::string, std::string> ParamOverrides;
   if (Err1.isSuccess()) {
      for (Config::Iter It = AnalysisStreamOptions.begin(); It != AnalysisStreamOptions.end(); ++It) {
         std::string Key = It->first.as<std::string>();
         std::string Value;
         AnalysisStreamOptions.get(Key, Value);
         ParamOverrides[Key] = Value;
      }
   }

   // Create an instance of the StreamParams struct and apply overrides
   StreamParams StreamCfg;
   StreamCfg.apply(ParamOverrides);

   // Fetch the temporal averaging and sampling periods
   std::vector<std::string> AvgPeriodList;
   Err1 = AnalysisGroupOptions.get("AvgPeriod", AvgPeriodList);
   std::vector<std::string> SampleFreqList;
   Err2 = AnalysisGroupOptions.get("SampleFreq", SampleFreqList);
   if (Err1.isFail() and Err2.isFail()) {
      ABORT_ERROR("Analysis: Error reading both AvgPeriod and SampleFreq from "
                  "Config, at least one must be present");
   }

   // Get filename from config, and parse it into a prefix and a
   // timestamp template
   std::string FilenameStr;
   Err1 = AnalysisGroupOptions.get("Filename", FilenameStr);
   CHECK_ERROR_ABORT(Err1, "Analysis: Filename not found in Config");
   std::string FilenamePrefix;
   std::string FilenameTemplate;
   size_t Pos = FilenameStr.find("$");
   if (Pos != std::string::npos) {
      FilenamePrefix = FilenameStr.substr(0, Pos-1);
      FilenameTemplate = FilenameStr.substr(Pos-1);
   } else {
      FilenamePrefix = FilenameStr;
   }

   // Loop over list of temporal averaging periods and create a separate stream
   // for each period in this group
   for (const auto &PeriodName: AvgPeriodList) {
      // The period is given as a single string beginning with numerals and
      // ending with a character string of a unit of time, e.g. 1day, 2months.
      // Break the string into separate components for adding to the Stream and
      // create a TimeInterval to compare with the RestartWrite interval.
      std::vector<std::string> ParsedStr = parseFreqStr(PeriodName);
      I4 Freq = std::stoi(ParsedStr[0]);
      TimeUnits FreqUnits = TimeUnitsFromString(ParsedStr[1]);
      TimeInterval PeriodInterval(Freq, FreqUnits);
      AveragingPeriods.push_back({PeriodName, PeriodInterval});

      // Fetch the RestartWrite alarm and check if the interval is evenly
      // divisible by the averaging period interval.
      auto RestartAlarm = IOStream::getAlarm("RestartWrite");
      bool IsDivisible = RestartAlarm->getInterval()->isDivisibleBy(PeriodInterval);
      if (!IsDivisible) {
         ABORT_ERROR("Analysis: The RestartWrite interval is not divisible by the averaging period, {} {}. Currently, temporal averaging is only available over intervals where RestartPeriod % PeriodInterval == 0", ParsedStr[0], ParsedStr[1]);
      }

      StreamCfg.Params["Filename"] = FilenamePrefix + "_" + PeriodName + "Avg" + FilenameTemplate;
      StreamCfg.Params["Freq"] = ParsedStr[0];
      StreamCfg.Params["FreqUnits"] = ParsedStr[1];

      auto NewStreamCfg = StreamCfg.toConfig();
      auto RefClock = AnalysisPtr->getModelClock();
      std::string NewStreamName = GroupName + "_" + PeriodName + "Avg";
      IOStream::create(NewStreamName, NewStreamCfg, RefClock);
      StreamNames.push_back(NewStreamName);
      OutputStreams.push_back(
          AnalysisStream(NewStreamName, PeriodName, PeriodInterval, true));
   }

   // Loop over list of discrete sampling frequencies and create a separate
   // stream for each period in this group.
   for (const auto &SampleName: SampleFreqList) {
      std::vector<std::string> ParsedStr = parseFreqStr(SampleName);
      I4 Freq = std::stoi(ParsedStr[0]);
      TimeUnits FreqUnits = TimeUnitsFromString(ParsedStr[1]);
      TimeInterval SampleInterval(Freq, FreqUnits);
      SamplingPeriods.push_back({SampleName, SampleInterval});

      StreamCfg.Params["Filename"] = FilenamePrefix + "_" + SampleName + "Samples" + FilenameTemplate;
      StreamCfg.Params["Freq"] = ParsedStr[0];
      StreamCfg.Params["FreqUnits"] = ParsedStr[1];

      auto NewStreamCfg = StreamCfg.toConfig();
      auto RefClock = AnalysisPtr->getModelClock();
      std::string NewStreamName = GroupName + "_" + SampleName + "Samples";
      IOStream::create(NewStreamName, NewStreamCfg, RefClock);
      StreamNames.push_back(NewStreamName);
      OutputStreams.push_back(
          AnalysisStream(NewStreamName, SampleName, SampleInterval, false));
   }

} // end createAnalysisGroupStreams

//------------------------------------------------------------------------------
std::vector<std::string> AnalysisGroup::parseFreqStr(const std::string &FreqStr) {

   std::string DigitStr;
   std::string UnitsStr;
   size_t Pos = FreqStr.find_first_not_of("0123456789");
   if (Pos != std::string::npos) {
      DigitStr = FreqStr.substr(0, Pos);
      UnitsStr = FreqStr.substr(Pos);
   }
   if (FreqStr == "" or UnitsStr == "") {
      ABORT_ERROR("Analysis: Invalid frequency string found in Config: {}", FreqStr);
   }
   if (UnitsStr.back() != 's') {
      UnitsStr += 's';
   }

   return {DigitStr, UnitsStr};

}

} // end namespace OMEGA
