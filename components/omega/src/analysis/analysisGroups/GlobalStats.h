#ifndef OMEGA_GLOBALSTATS_H
#define OMEGA_GLOBALSTATS_H

//===----------------------------------------------------------------------===//

#include "Analysis.h"
#include "AnalysisGroup.h"
//#include "AnalysisOrchestrator.h"
#include "Config.h"
#include "operators/Ops.h"
#include <string>

namespace OMEGA {

//class AnalysisOrchestrator;

class GlobalStats : public AnalysisGroup {
 public:

   GlobalStats(const std::string &GroupName,
               Config &AnalysisGroupOptions,
               Analysis *AnalysisManager);

   ~GlobalStats() = default;


};

} // end namespace OMEGA

#endif
