#ifndef OMEGA_ANALYSISGROUP_H
#define OMEGA_ANALYSISGROUP_H

//===----------------------------------------------------------------------===//
#include "Analysis.h"
#include "Config.h"
#include "IOStream.h"
#include <string>

namespace OMEGA {

/// The AnalysisGroup class ...
class AnalysisGroup {

 public:

//   AnalysisGroup(const std::string &Name,
//                 Config &Options,
//                 AnalysisOrchestrator *Orchestrator);

   ///
   virtual ~AnalysisGroup() = default;

   ///
   std::string getName();

   ///
   const std::vector<std::string> createStreamsForAnalysisGroup(
       const std::string &GroupName,
       Config &AnalysisGroupCfg,
       Analysis *AnalysisPtr);

 protected:

   ///
   struct AnalysisStreamCfg {
      AnalysisStreamCfg()
         : Params{
            {"UsePointerFile", "false"},
            {"PointerFilename", ""},
            {"Filename", ""},
            {"Mode", "write"},
            {"IfExists", "append"},
            {"Precision", "double"},
            {"Freq", ""},
            {"FreqUnits", ""},
            {"FileFreq", ""},
            {"FileFreqUnits", ""},
            {"UseStartEnd", "false"},
            {"StartTime", ""},
            {"EndTime", ""},
         }
      {}

      ///
      Config toConfig() const {
         Config Cfg;
         for (const auto& [key, value] : Params) {
            if (!value.empty()) {
               Cfg.add(key, value);
            }
         }

         std::vector<std::string> EmptyStrVec{""};
         Cfg.add("Contents", EmptyStrVec);

         return Cfg;
      }

      ///
      std::map<std::string, std::string> Params;
   };

   ///
   std::vector<std::string> parseFreqStr(const std::string &FreqStr);

   ///
   std::string GroupName;

};

} // end namespace OMEGA

#endif
