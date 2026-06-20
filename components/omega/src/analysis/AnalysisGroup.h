#ifndef OMEGA_ANALYSISGROUP_H
#define OMEGA_ANALYSISGROUP_H

//===----------------------------------------------------------------------===//
#include "Analysis.h"
#include "Config.h"
#include "IOStream.h"
#include <string>
#include <sstream>

namespace OMEGA {

/// The AnalysisGroup class ...
class AnalysisGroup {

 public:

   ///
   virtual ~AnalysisGroup() = default;

   ///
   std::string getName();

   ///
   void createAnalysisGroupStreams(
      const std::string &GroupName,
      Config &AnalysisGroupOptions,
      Analysis *AnalysisManager);

 protected:

   ///
   struct StreamParams {
      StreamParams()
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
      void apply(const std::map<std::string, std::string> &Overrides) {
         for (const auto& [Key, Value] : Overrides) {
            auto It = Params.find(Key);

            if (It == Params.end()) {
               ABORT_ERROR("Analysis: Unknown Stream config parameter, {}", Key);
            }

            It->second = Value;
         }
      }

      ///
      Config toConfig() const {
         Config Cfg;
         for (const auto& [Key, Value] : Params) {
            if (!Value.empty()) {
               Cfg.add(Key, Value);
            }
         }

         // Contents will be added after operators are built
         std::vector<std::string> EmptyStrVec{""};
         Cfg.add("Contents", EmptyStrVec);

         return Cfg;
      }

      ///
      std::map<std::string, std::string> Params;
   };

   ///
   std::string GroupName;

   ///
   std::vector<AnalysisStream> OutputStreams;
   std::vector<std::string> StreamNames;
   std::vector<std::pair<std::string, TimeInterval>> AveragingPeriods;
   std::vector<std::pair<std::string, TimeInterval>> SamplingPeriods;

};

} // end namespace OMEGA

#endif
