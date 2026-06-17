//===----------------------------------------------------------------------===//
#include "AnalysisGroup.h"

namespace OMEGA {
//AnalysisGroup::AnalysisGroup(const std::string &Name,
//                             Config &Options,
//                             AnalysisOrchestrator *Orchestrator) {}

//------------------------------------------------------------------------------
std::string AnalysisGroup::getName() {
   return GroupName;
}

//------------------------------------------------------------------------------
const std::vector<std::string> AnalysisGroup::createStreamsForAnalysisGroup(
    const std::string &GroupName,
    Config &AnalysisGroupCfg,
    Analysis *AnalysisPtr) {

   Error Err1;
   Error Err2;
   std::vector<std::string> StreamNames;

   // Fetch the time averaging periods
   std::vector<std::string> AvgPeriodList;
   Err1 = AnalysisGroupCfg.get("AvgPeriod", AvgPeriodList);
   std::vector<std::string> SampleFreqList;
   Err2 = AnalysisGroupCfg.get("SampleFreq", SampleFreqList);
   if (Err1.isFail() and Err2.isFail()) {
      ABORT_ERROR("Analysis: Error reading both AvgPeriod and SampleFreq from "
                  "Config, at least one must be present");
   }

//   CHECK_ERROR_ABORT(Err, "Analysis: Period list not found in Config");

   std::string FilenameStr;
   Err1 = AnalysisGroupCfg.get("Filename", FilenameStr);
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

   for (const auto &PeriodName: AvgPeriodList) {
//      std::string FreqStr;
//      std::string UnitsStr;
//      Pos = PeriodName.find_first_not_of("0123456789");
//      if (Pos != std::string::npos) {
//         FreqStr = PeriodName.substr(0, Pos);
//         UnitsStr = PeriodName.substr(Pos);
//      }
//      if (FreqStr == "" or UnitsStr == "") {
//         ABORT_ERROR("Analysis: Invalid period found in Config, {}", PeriodName);
//      }
//      if (UnitsStr.back() != 's') {
//         UnitsStr += 's';
//      }
//      std::cout << "freq, unit: " << FreqStr << " " << UnitsStr << std::endl;
      std::vector<std::string> ParsedStr = parseFreqStr(PeriodName);

      AnalysisStreamCfg StreamCfg;
      StreamCfg.Params["Filename"] = FilenamePrefix + "_" + PeriodName + "Avg" + FilenameTemplate;
      StreamCfg.Params["Freq"] = ParsedStr[0];
      StreamCfg.Params["FreqUnits"] = ParsedStr[1];
//      std::cout << "Filename: " << StreamCfgParams.Filename << std::endl;

      auto NewStreamCfg = StreamCfg.toConfig();
      auto RefClock = AnalysisPtr->getModelClock();
      std::string NewStreamName = GroupName + "_" + PeriodName + "Avg";
      IOStream::create(NewStreamName, NewStreamCfg, RefClock);
      StreamNames.push_back(NewStreamName);
   }
   for (const auto &SampleName: SampleFreqList) {
      std::vector<std::string> ParsedStr = parseFreqStr(SampleName);

      AnalysisStreamCfg StreamCfg;
      StreamCfg.Params["Filename"] = FilenamePrefix + "_" + SampleName + "Samples" + FilenameTemplate;
      StreamCfg.Params["Freq"] = ParsedStr[0];
      StreamCfg.Params["FreqUnits"] = ParsedStr[1];

      auto NewStreamCfg = StreamCfg.toConfig();
      auto RefClock = AnalysisPtr->getModelClock();
      std::string NewStreamName = GroupName + "_" + SampleName + "Samples";
      IOStream::create(NewStreamName, NewStreamCfg, RefClock);
      StreamNames.push_back(NewStreamName);
   }

   return StreamNames;
} // end createStreamsForAnalysisGroup

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
   std::cout << "freq, unit: " << DigitStr << " " << UnitsStr << std::endl;

   return {DigitStr, UnitsStr};

}

} // end namespace OMEGA
