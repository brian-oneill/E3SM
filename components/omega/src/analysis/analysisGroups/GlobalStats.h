#ifndef OMEGA_GLOBALSTATS_H
#define OMEGA_GLOBALSTATS_H

#include "Config.h"
#include "operators/Ops.h"
#include <string>

namespace OMEGA {

class AnalysisOrchestrator;

class GlobalStats {
 public:

   GlobalStats(const std::string &Name,
               Config &Options,
               AnalysisOrchestrator *Orchestrator);

   ~GlobalStats() = default;


};

} // end namespace OMEGA

#endif
