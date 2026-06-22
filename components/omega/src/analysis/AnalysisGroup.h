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

   /// Structure to store metadata about operator chains
   struct OpChainInfo {
      std::string ChainStr;      // Operator instance name (e.g., "Temperature_SpatialMean_TimeMean1day")
      std::string FreqStr;       // Frequency/period string (e.g., "1day", "6hour")
      bool IsTimeAvg;            // true for time average, false for discrete samples
   };

   /// The StreamParams struct serves as a template for creating output streams
   /// associated with an AnalysisGroup
   struct StreamParams {
      // Params is a string-to-string map of all the possible options used in
      // IOStream creation. When a StreamParams instance is created, it has
      // the following values by default
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
      // If optional stream config options are given for the AnalysisGroup,
      // apply will take the options overwrite the default values above
      void apply(const std::map<std::string, std::string> &Overrides) {
         for (const auto& [Key, Value] : Overrides) {
            auto It = Params.find(Key);

            if (It == Params.end()) {
               ABORT_ERROR("Analysis: Unknown Stream config parameter, {}", Key);
            }

            It->second = Value;
         }
      }

      // Creates a Config object to be passed to IOStream::create
      Config toConfig() const {
         Config Cfg;
         // Loop over the Params, add only options where Value is not empty
         for (const auto& [Key, Value] : Params) {
            if (!Value.empty()) {
               Cfg.add(Key, Value);
            }
         }

         // Contents are added after stream is created, leave empty for now
         std::vector<std::string> EmptyStrVec{""};
         Cfg.add("Contents", EmptyStrVec);

         return Cfg;
      }

      // Map of config options used to create output streams for the
      // AnalysisGroup
      std::map<std::string, std::string> Params;
   };

   /// Name of this AnalysisGroup
   std::string GroupName;

   /// Vector storing metadata for all operator chains in this group
   std::vector<OpChainInfo> OpChainInfos;

   ///
   std::vector<std::string> OpChainStrings;

};

} // end namespace OMEGA

#endif
