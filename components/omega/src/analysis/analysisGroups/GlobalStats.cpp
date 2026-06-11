#include "analysisGroups/GlobalStats.h"
//#include "AnalysisOrchestrator.h"
#include <iostream>

namespace OMEGA {

GlobalStats::GlobalStats(const std::string &Name,
               Config &Options,
               AnalysisOrchestrator *Orchestrator) {


   Error Err;

   std::vector<std::string> VarList;

   Err = Options.get("Fields", VarList);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Fields list not found in Config");

   std::vector<std::string> OpList;
   Err = Options.get("Stats", OpList);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Stats list not found in Config");

   std::vector<std::string> PeriodList;
   Err = Options.get("Period", PeriodList);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Period list not found in Config");
   
   std::string FilenameStr;
   Err = Options.get("Filename", FilenameStr);
   CHECK_ERROR_ABORT(Err, "GlobalStats: Filename not found in Config");
   std::string FilenamePrefix;
   std::string FilenameTemplate;
   size_t Pos = FilenameStr.find("$");
   if (Pos != std::string::npos) {
      FilenamePrefix = FilenameStr.substr(0, Pos-1);
      FilenameTemplate = FilenameStr.substr(Pos-1);
   } else {
      FilenamePrefix = FilenameStr;
   }

   
   for (const auto &PeriodName: PeriodList) {
      std::string FreqStr;
      std::string UnitsStr;
      Pos = PeriodName.find_first_not_of("0123456789");
      if (Pos != std::string::npos) {
         FreqStr = PeriodName.substr(0, Pos);
         UnitsStr = PeriodName.substr(Pos);
      }
      if (FreqStr == "" or UnitsStr == "") {
         ABORT_ERROR("GlobalStats: Invalid period found in Config, {}", PeriodName);
      }
      if (UnitsStr.back() != 's') {
         UnitsStr += 's';
      }
      std::cout << "freq, unit: " << FreqStr << " " << UnitsStr << std::endl;

      AnalysisStreamCfg StreamCfg;
      StreamCfg.Params["Filename"] = FilenamePrefix + "_" + PeriodName + FilenameTemplate;
      StreamCfg.Params["Freq"] = FreqStr;
      StreamCfg.Params["FreqUnits"] = UnitsStr;
//      std::cout << "Filename: " << StreamCfgParams.Filename << std::endl;

      auto NewStreamCfg = StreamCfg.toConfig();
      Clock DummyClock;
      IOStream::create(Name + "_" + PeriodName, NewStreamCfg, DummyClock);

      for (const auto &VarName: VarList) {
         for (const auto &OpName: OpList) {

//            std::cout << "global stats: " << VarName << " | op: " << OpName << std::endl;
//            auto Op = AnalysisOpFactory::createOp("spatial_" + OpName, VarName, Options);
            Orchestrator->registerAnalysisOp(VarName, "spatial_" + OpName, Options);

         }
      }
   }

}

} // end namespace OMEGA
