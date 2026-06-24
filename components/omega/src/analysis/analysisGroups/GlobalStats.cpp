#include "analysisGroups/GlobalStats.h"
//#include "AnalysisOrchestrator.h"
#include <iostream>

namespace OMEGA {

GlobalStats::GlobalStats(const std::string &GroupName,
               Config &AnalysisGroupOptions,
               Analysis *AnalysisManager) {


   Error Err1;
   Error Err2;


   std::vector<std::string> VarList;

   // Get field list
   Err1 = AnalysisGroupOptions.get("Fields", VarList);
   CHECK_ERROR_ABORT(Err1, "GlobalStats: Fields list not found in Config");

   // Get statistics operators list
   std::vector<std::string> OpList;
   Err1 = AnalysisGroupOptions.get("SpatialStats", OpList);
   CHECK_ERROR_ABORT(Err1, "GlobalStats: SpatialStats list not found in Config");

   // Get temporal reduction periods (optional)
   std::vector<std::string> PeriodList;
   Err1 = AnalysisGroupOptions.get("ReductionPeriod", PeriodList);

   // Get discrete sampling frequencies (optional)
   std::vector<std::string> SampleFreqList;
   Err2 = AnalysisGroupOptions.get("SampleFreq", SampleFreqList);

   // At least one temporal specification must be present
   if (Err1.isFail() and Err2.isFail()) {
      ABORT_ERROR("GlobalStats: Error reading both ReductionPeriod and "
                  "SampleFreq from Config, at least one must be present");
   }

   for (const auto &VarName: VarList) {
      for (const auto &OpName: OpList) {

         std::string OperatorType = "Spatial" + OpName;
         std::string ChainStr;

         std::string NewOpChainName = VarName + "_Spatial" + OpName; 

         // Create temporal reduction chains (spatial op + temporal reduction)
         for (const auto &Period: PeriodList) {
            ChainStr = VarName + "_" + OperatorType + "_TimeMean" + Period;

            // Store metadata for stream creation
            OpChainInfos.push_back(OpChainInfo{ChainStr, Period, true});

//            OpChainStrings.push_back(NewOpChainName + "_TimeMean" + Period);

            // Parse and build the operator chain
            AnalysisManager->parseChainAndBuildOps(ChainStr);

         }

         // Create discrete sampling chains (spatial op only, no temporal
         // temporal reduction)
         if (!SampleFreqList.empty()) {
            ChainStr = VarName + "_" + OperatorType;
            // Parse and build the operator chain if Ops were not built above
            AnalysisManager->parseChainAndBuildOps(ChainStr);
         }
         for (const auto &SampleFreq : SampleFreqList) {

            // Store metadata for stream creation
            OpChainInfos.push_back(OpChainInfo{ChainStr, SampleFreq, false});

//            OpChainStrings.push_back(NewOpChainName);
         }
//         }

//         std::cout << "global stats: " << VarName << " | op: " << OpName << std::endl;
//         auto Op = AnalysisOpFactory::createOp("Spatial" + OpName, VarName, AnalysisGroupOptions);
//         AnalysisManager->registerAnalysisOp(OperatorType, {VarName}, AnalysisGroupOptions);

      }
   }

//   for (const auto &OpChain: OpChainStrings) {
//      std::cout << "op chain: " << OpChain << std::endl;
//      AnalysisManager->parseChainAndBuildOps(OpChain);
//   }

   createAnalysisGroupStreams(GroupName, AnalysisGroupOptions, AnalysisManager);


//   std::vector<OperatorNode*> SpatialNodes = AnalysisManager->getOpNodes();
   

//   for (const auto &StreamName: StreamNames) {
//   for (const auto &OutputStream: OutputStreams) {
//      std::cout << StreamName << std::endl;
//      std::cout << OutputStream.StreamName << " " << OutputStream.IntervalStr << std::endl;
         //if (OutputStream.IsTimeReduction) {
         //   AnalysisManager->registerAnalysisOp("time_mean", {VarName + "_" + OperatorType}, makeOpConfig(opParam("Period", OutputStream.IntervalStr)));
         //}

//   }
//   }

}

} // end namespace OMEGA
