#ifndef OMEGA_ANALYSISGROUP_H
#define OMEGA_ANALYSISGROUP_H

#include "Config.h"
#include "IOStream.h"
#include <string>

namespace OMEGA {

class AnalysisGroup {

 public:

//   AnalysisGroup(const std::string &Name,
//                 Config &Options,
//                 AnalysisOrchestrator *Orchestrator);

   virtual ~AnalysisGroup() = default;

   std::string getName();

 protected:

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

      std::map<std::string, std::string> Params;
   };

   std::string GroupName;

};

} // end namespace OMEGA

#endif
