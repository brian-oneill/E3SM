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
   Analysis *AnalysisManager
) {

   Error Err1;
   Error Err2;

   // Check for optional stream config parameters in AnalysisGroup config
   Config AnalysisStreamOptions("Stream");
   Err1 = AnalysisGroupOptions.get(AnalysisStreamOptions);
   std::map<std::string, std::string> ParamOverrides;
   if (Err1.isSuccess()) {
      for (Config::Iter It = AnalysisStreamOptions.begin(); 
           It != AnalysisStreamOptions.end(); ++It) {
         std::string Key = It->first.as<std::string>();
         std::string Value;
         AnalysisStreamOptions.get(Key, Value);
         ParamOverrides[Key] = Value;
      }
   }

   // Create an instance of the StreamParams struct and apply optional overrides
   StreamParams StreamCfg;
   StreamCfg.apply(ParamOverrides);

   // Get filename from config, and parse it into a prefix and a timestamp template
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

   // Group operator chains by their stream characteristics
   // Map: StreamName -> list of operator instance names
   std::map<std::string, std::vector<std::string>> StreamToOpNames;
   
   for (const auto &Info : OpChainInfos) {
      std::string StreamName;
      
      if (Info.IsTimeReduction) {
         StreamName = GroupName + "_" + Info.FreqStr + "TimeStats";
      } else {
         StreamName = GroupName + "_" + Info.FreqStr + "Samples";
      }
      
//      std::cout << StreamName << " : " << Info.ChainStr << std::endl;
      StreamToOpNames[StreamName].push_back(Info.ChainStr);
   }

   // Create streams and associate operators
   for (const auto &[StreamName, OpNames] : StreamToOpNames) {
      
      // Determine stream type from name
      bool IsTimeReduction = (StreamName.find("TimeStats") != std::string::npos);
      
      // Extract period string from stream name
      // e.g., "GlobalStats_1DayTimeStats" -> "1Day"
      size_t UnderscorePos = StreamName.find_last_of("_");
      std::string PeriodWithSuffix = StreamName.substr(UnderscorePos + 1);
      std::string PeriodStr;
      if (IsTimeReduction) {
         PeriodStr = PeriodWithSuffix.substr(0, PeriodWithSuffix.find("TimeStats"));
      } else {
         PeriodStr = PeriodWithSuffix.substr(0, PeriodWithSuffix.find("Samples"));
      }
      
      // Parse period string into frequency and units
      std::vector<std::string> ParsedStr = parseFreqStr(PeriodStr);
      I4 Freq = std::stoi(ParsedStr[0]);
      TimeUnits FreqUnits = TimeUnitsFromString(ParsedStr[1]);
      TimeInterval PeriodInterval(Freq, FreqUnits);
      
      // For time-averaged streams, validate against RestartWrite interval
      if (IsTimeReduction) {
         auto RestartAlarm = IOStream::getAlarm("RestartWrite");
         bool IsDivisible = RestartAlarm->getInterval()->isDivisibleBy(PeriodInterval);
         if (!IsDivisible) {
            ABORT_ERROR("Analysis: The RestartWrite interval is not divisible by "
                       "the averaging period, {} {}. Currently, temporal averaging "
                       "is only available over intervals where "
                       "RestartPeriod % PeriodInterval == 0", 
                       ParsedStr[0], ParsedStr[1]);
         }
      }

      // Configure stream parameters
      StreamCfg.Params["Filename"] = FilenamePrefix + "_" + PeriodStr + 
                                     (IsTimeReduction ? "TimeStats" : "Samples") + FilenameTemplate;
      StreamCfg.Params["Freq"] = ParsedStr[0];
      StreamCfg.Params["FreqUnits"] = ParsedStr[1];

      // Create the IOStream
      auto NewStreamCfg = StreamCfg.toConfig();
      auto RefClock = AnalysisManager->getModelClock();
      IOStream::create(StreamName, NewStreamCfg, RefClock);
      
      // Get the created stream
      auto Stream = IOStream::get(StreamName);
      
      // Associate operators with stream and populate contents
      auto OpNodes = AnalysisManager->getOpNodes();
      for (auto *Node : OpNodes) {
         std::string OpInstanceName = Node->Op->getName();
         
         // Check if this operator is in our list for this stream
         if (std::find(OpNames.begin(), OpNames.end(), OpInstanceName) != OpNames.end()) {
            // Associate operator with stream
            Node->StreamNames.push_back(StreamName);
            
            // Add operator's output fields to stream contents
            for (const auto &FieldName : Node->Op->getOutputFieldNames()) {
               Stream->addField(FieldName);
            }
         }
      }
      
   }

} // end createAnalysisGroupStreams

} // end namespace OMEGA
