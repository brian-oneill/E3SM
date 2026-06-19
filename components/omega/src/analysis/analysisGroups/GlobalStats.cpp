#include "analysisGroups/GlobalStats.h"
//#include "AnalysisOrchestrator.h"
#include <iostream>

namespace OMEGA {

GlobalStats::GlobalStats(const std::string &GroupName,
               Config &Options,
               Analysis *AnalysisObj) {


   Error Err;

   std::vector<std::string> VarList;

   Err = Options.get("Fields", VarList);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Fields list not found in Config");

   std::vector<std::string> OpList;
   Err = Options.get("Stats", OpList);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Stats list not found in Config");

   std::vector<std::string> PeriodList;
   Err = Options.get("AvgPeriod", PeriodList);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Period list not found in Config");
   
   createAnalysisGroupStreams(GroupName, Options, AnalysisObj);

//   for (const auto &StreamName: StreamNames) {
   for (const auto &OutputStream: OutputStreams) {
//      std::cout << StreamName << std::endl;
//      std::cout << OutputStream.StreamName << " " << OutputStream.IntervalStr << std::endl;

      for (const auto &VarName: VarList) {
         for (const auto &OpName: OpList) {

//            std::cout << "global stats: " << VarName << " | op: " << OpName << std::endl;
//            auto Op = AnalysisOpFactory::createOp("spatial_" + OpName, VarName, Options);
            AnalysisObj->registerAnalysisOp("spatial_" + OpName, {VarName}, Options);

         }
      }
   }
//   }

}

} // end namespace OMEGA
